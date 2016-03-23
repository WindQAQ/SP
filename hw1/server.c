#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include "item.h"

#define ERR_EXIT(a) { perror(a); exit(1); }

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";
const char* islocked = "This item is locked.\n";
const char* modifiable = "This item is modifiable.\n";
const char* failed = "Operation failed.\n";

// Forwards

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

static int handle_read(request* reqP);
// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error

int main(int argc, char** argv) {
    int i, ret;

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[512];
    int buf_len;

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Get file descripter table size and initize request table
    maxfd = getdtablesize();
    maxfd = (maxfd > FD_SETSIZE)? FD_SETSIZE: maxfd;
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);
	
	// Open "item_list"
	
	file_fd = open("item_list", O_RDWR);

	fd_set readfd, allset;
	FD_ZERO(&allset);
	
	FD_SET(svr.listen_fd, &allset);

	while (1) {
        // TODO: Add IO multiplexing

		readfd = allset;
		select(maxfd, &readfd, NULL, NULL, NULL);
       	 
		// Check new connection
		if(FD_ISSET(svr.listen_fd, &readfd)){
			clilen = sizeof(cliaddr);
        	conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
			FD_SET(conn_fd, &allset);
			if (conn_fd < 0) {
           		if (errno == EINTR || errno == EAGAIN) continue;  // try again
           		if (errno == ENFILE) {
            	   	(void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
               		continue;
   	        	}
     	     	ERR_EXIT("accept")
        	}
        	requestP[conn_fd].conn_fd = conn_fd;
        	strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
   			fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
		}
		
		Item item;
		char str[1024];

		for(int i = 5; i < maxfd; i++){
			if(FD_ISSET(requestP[i].conn_fd, &readfd) && requestP[i].wait_for_write == 0){
				ret = handle_read(&requestP[i]); // parse data from client to requestP[conn_fd].buf
				if(requestP[i].buf_len > 0){
					if(!is_item(requestP[i].buf)){
#ifdef READ_SERVER
						sprintf(buf,"%s : %s\n",accept_read_header,requestP[i].buf);
						write(requestP[i].conn_fd, buf, strlen(buf));
#else		
						sprintf(buf,"%s : %s\n",accept_write_header,requestP[i].buf);
						write(requestP[i].conn_fd, buf, strlen(buf));
#endif
						FD_CLR(i, &allset);
						close(requestP[i].conn_fd);
						free_request(&requestP[i]);
					}
					requestP[i].item = atoi(requestP[i].buf);
#ifdef READ_SERVER
					if(can_read(file_fd, requestP[i]) == 1){
						lseek(file_fd, sizeof(Item)*(requestP[i].item - 1), SEEK_SET);
						read(file_fd, &item, sizeof(Item));
						sprintf(str, "item%d $%d remain: %d\n", item.id, item.price, item.amount);
						write(requestP[i].conn_fd, str, strlen(str));
					}
					else write(requestP[i].conn_fd, islocked, strlen(islocked));
#else
					if(can_write(file_fd, requestP, i, maxfd) == 1){
						write(requestP[i].conn_fd, modifiable, strlen(modifiable));
						continue;
					}
					else write(requestP[i].conn_fd, islocked, strlen(islocked));
#endif
					FD_CLR(i, &allset);
					close(requestP[i].conn_fd);
					free_request(&requestP[i]);
				}
				if (ret < 0) {
					fprintf(stderr, "bad request from %s\n", requestP[i].host);
					continue;
				}	
			}
			else if(FD_ISSET(requestP[i].conn_fd, &readfd) && requestP[i].wait_for_write == 1){
				ret = handle_read(&requestP[i]);
				if(requestP[i].buf_len > 0){
					if(write_itemlist(file_fd, requestP[i])){}
					else write(requestP[i].conn_fd, failed, strlen(failed));
					
					unlock(file_fd, requestP[i]);
					requestP[i].wait_for_write = 0;
					FD_CLR(i, &allset);
					close(requestP[i].conn_fd);
					free_request(&requestP[i]);
				}
				if(ret < 0) {
					fprintf(stderr, "bad request from %s\n", requestP[i].host);
					continue;
				}
			}
		}
    }
    free(requestP);
    return 0;
}


// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void* e_malloc(size_t size);


static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->item = 0;
    reqP->wait_for_write = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error
static int handle_read(request* reqP) {
    int r;
    char buf[512];

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
	char* p1 = strstr(buf, "\015\012");
	int newline_len = 2;
	// be careful that in Windows, line ends with \015\012
	if (p1 == NULL) {
		p1 = strstr(buf, "\012");
		newline_len = 1;
		if (p1 == NULL) {
			ERR_EXIT("this really should not happen...");
		}
	}
	size_t len = p1 - buf + 1;
	memmove(reqP->buf, buf, len);
	reqP->buf[len - 1] = '\0';
	reqP->buf_len = len-1;
    return 1;
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }
}

static void* e_malloc(size_t size) {
    void* ptr;

    ptr = malloc(size);
    if (ptr == NULL) ERR_EXIT("out of memory");
    return ptr;
}
