#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX 256
#define SIGUSR3 SIGWINCH

typedef struct node{
	struct node* next;
	struct timeval deadline;
	struct timespec rem;
	int type;
	int begin;
	int serial;
} Node;

const char* LOG_FILE = "bidding_system_log";
const char* TERM = "terminate\n";
const int sig[] = {SIGUSR1, SIGUSR2, SIGUSR3};

pid_t pid;
FILE* logfile;
int counter[3];
struct timespec interval[3];
struct timeval start, limit[3];
Node *head;

void init()
{
	interval[0].tv_sec = 0;
	interval[0].tv_nsec = 500000000;
	interval[1].tv_sec = 1;
	interval[1].tv_nsec = 0;
	interval[2].tv_sec = 0;
	interval[2].tv_nsec = 200000000;

	limit[0].tv_sec = 2;
	limit[0].tv_usec = 0;
	limit[1].tv_sec = 3;
	limit[1].tv_usec = 0;
	limit[2].tv_sec = 0;
	limit[2].tv_usec = 300000;

	head = NULL;
}

int cmpstr(const char* str)
{
	int i = 0;
	while(TERM[i] != '\0'){
		if(str[i] != TERM[i])
			return 0;
		i++;
	}
	return 1;
}

struct timeval add_timeval(struct timeval x, struct timeval y)
{
	struct timeval temp;
	temp.tv_sec = x.tv_sec + y.tv_sec;
	temp.tv_usec = x.tv_usec + y.tv_usec;
	temp.tv_sec += temp.tv_usec / 1000000;
	temp.tv_usec %= 1000000;
	return temp;
}

int cmp_timeval(struct timeval x, struct timeval y)
{
	if(x.tv_sec > y.tv_sec)
		return 1;
	else if(x.tv_sec < y.tv_sec)
		return -1;
	else if(x.tv_usec > y.tv_usec)
		return 1;
	else if(x.tv_usec < y.tv_usec)
		return -1;

	return 0;
}

Node* insert_node(struct timeval x, struct timespec y, int type)
{
	Node* current = (Node *)malloc(sizeof(Node));
	current->deadline = x;
	current->rem = y;
	current->type = type;
	current->begin = 0;
	current->serial = ++counter[type];

	if(head == NULL){
		current->next = NULL;
		return current;
	}

	Node* previous = NULL, *ptr = head;
	while(ptr != NULL && cmp_timeval(ptr->deadline, x) == -1){
		previous = ptr;
		ptr = ptr->next;
	}

	if(previous == NULL){
		current->next = head;
		return current;
	}
	else{
		current->next = previous->next;
		previous->next = current;
		return head;
	}
}

void sig_handler(int signo)
{
	struct timeval cur, temp;
	gettimeofday(&cur, NULL);
	switch(signo){
		case SIGUSR1:
			temp = add_timeval(cur, limit[0]);
			head = insert_node(temp, interval[0], 0);
			break;
		case SIGUSR2:
			temp = add_timeval(cur, limit[1]);
			head = insert_node(temp, interval[1], 1);
			break;
		case SIGUSR3:
			temp = add_timeval(cur, limit[2]);
			head = insert_node(temp, interval[2], 2);
			break;
	}
}

int main(int argc, char* argv[])
{
	logfile = fopen(LOG_FILE, "wb");

	init();

	memset(counter, 0, sizeof(counter));

	struct sigaction act;

	act.sa_handler = sig_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	sigaction(SIGUSR1, &act, NULL);
	sigaction(SIGUSR2, &act, NULL);
	sigaction(SIGUSR3, &act, NULL);

	int pfd[2][2];
	for(int i = 0; i < 2; i++)
		pipe(pfd[i]);

	if((pid = fork()) == 0){
		dup2(pfd[1][0], 0);
		dup2(pfd[0][1], 1);

		close(pfd[0][0]);
		close(pfd[0][1]);
		close(pfd[1][0]);
		close(pfd[1][1]);
		fclose(logfile);

		execlp("./customer_EDF", "./customer_EDF", argv[1], NULL);
	}

	close(pfd[0][1]);
	close(pfd[1][0]);

	gettimeofday(&start, NULL);
	char buf[MAX];
	while(1){
		Node *ptr = head;

		if(head == NULL){
			int ret = read(pfd[0][0], buf, MAX);
			if(ret < 0 && errno == EINTR)
				continue;
			if(cmpstr(buf))
				break;
		}

		while(ptr != NULL){
			if(ptr->begin == 0){
				ptr->begin = 1;
				fprintf(logfile, "receive %d %d\n", ptr->type, ptr->serial);
			}
			
			if(nanosleep(&ptr->rem, &ptr->rem) == 0){
				kill(pid, sig[ptr->type]);
				fprintf(logfile, "finish %d %d\n", ptr->type, ptr->serial);
				Node* temp = head;
				head = head->next;
				ptr = ptr->next;
				free(temp);
			}
			else break;
		}
	}

	wait(NULL);
	fprintf(logfile, "%s", TERM);
	fclose(logfile);
	
	return 0;
}