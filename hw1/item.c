#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "item.h"

struct flock lock_init(int type, off_t offset, int whence, off_t len)
{
	struct flock lock;
	lock.l_type = type;
	lock.l_start = offset;
	lock.l_whence = whence;
	lock.l_len = len;
 
	return lock;
}

int is_item(const char* str)
{
	int i = 0;
	while(str[i] != '\0'){
		if(!isdigit(str[i]))
			return 0;
	return 1;
	}
}

int can_read(int fd, request r)
{
	struct flock lock = lock_init(F_WRLCK, sizeof(Item)*(r.item - 1), SEEK_SET, sizeof(Item));

	fcntl(fd, F_GETLK, &lock);

	if(lock.l_type == F_UNLCK)
		return 1;
	else return -1;
}

int can_write(int fd, request* requestP , int cur, int maxfd)
{
	for(int i = 5; i < maxfd; i++)
		if(i != cur && requestP[i].item == requestP[cur].item &&
		   requestP[i].wait_for_write == 1)
				return -1;

	struct flock lock = lock_init(F_WRLCK, sizeof(Item)*(requestP[cur].item - 1), SEEK_SET, sizeof(Item));

	if(fcntl(fd, F_SETLK, &lock) == -1)
		return -1;
	
	requestP[cur].wait_for_write = 1;
	return 1;
}

void unlock(int fd, request r)
{
	struct flock lock = lock_init(F_UNLCK, sizeof(Item)*(r.item - 1), SEEK_SET, sizeof(Item));

	fcntl(fd, F_SETLK, &lock);
	
	return;
}

int write_itemlist(int fd, request r)
{
	char cmd[1024];
	int i = 0;

	while(r.buf[i] != '\0' && r.buf[i] != ' '){
		cmd[i] = r.buf[i];
		i++;
	}

	cmd[i] = '\0';
	int num = atoi(r.buf + i + 1);
	Item item;
	lseek(fd, sizeof(Item)*(r.item - 1), SEEK_SET);
	read(fd, &item, sizeof(Item));
	
	if(strcmp(cmd, "sell") == 0){
		item.amount += num;
		lseek(fd, sizeof(Item)*(r.item - 1), SEEK_SET);
		write(fd, &item, sizeof(Item));
		return 1;
	}
	else if(strcmp(cmd, "buy") == 0){
		if(num <= item.amount){
			item.amount -= num;
			lseek(fd, sizeof(Item)*(r.item - 1), SEEK_SET);
			write(fd, &item, sizeof(Item));
			return 1;
		}
		else return 0;
	}
	else if(strcmp(cmd, "price") == 0){
		if(num >= 0){
			item.price = num;
			lseek(fd, sizeof(Item)*(r.item - 1), SEEK_SET);
			write(fd, &item, sizeof(Item));
			return 1;
		}
		else return 0;
	}
	return 0;
}
