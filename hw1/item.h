typedef struct {
	int id;
	int amount;
	int price;
} Item;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by bufi
    // you don't need to change this.
	int item;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

int write_itemlist(int, request);

int is_item(const char*);

int can_read(int, request);

int can_write(int, request*, int ,int);

void unlock(int, request);