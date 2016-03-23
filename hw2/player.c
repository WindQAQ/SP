/*b03902042 宋子維*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>

int main(int argc, char *argv[])
{
	int host_id = atoi(argv[1]);
	int random_key = atoi(argv[3]);
	char player_index;
	sscanf(argv[2], "%c", &player_index);
	// FIFO name
	char player_FIFO[512], host_FIFO[512];
	sprintf(player_FIFO, "host%d_%c.FIFO", host_id, player_index);
	sprintf(host_FIFO, "host%d.FIFO", host_id);
	//open FIFO
	int FIFO_R = open(player_FIFO, O_RDONLY);
	int FIFO_W = open(host_FIFO, O_WRONLY);
	// i/o multiplexing
	fd_set read_set, write_set, MASTER_R, MASTER_W;
	FD_ZERO(&MASTER_R);
	FD_ZERO(&MASTER_W);
	FD_SET(FIFO_R, &MASTER_R);

	int max_fd = (FIFO_R > FIFO_W)? FIFO_R: FIFO_W;
	int message[4];
	int money = 0;
	int cnt = 0;

	while(cnt < 10){
		read_set = MASTER_R;
		write_set = MASTER_W;
		select(max_fd+1, &read_set, &write_set, NULL, NULL);
		if(FD_ISSET(FIFO_R, &read_set)){
			char buffer[512];
			if(read(FIFO_R, buffer, 512) > 0){
				sscanf(buffer, "%d %d %d %d\n", &message[0], &message[1], &message[2], &message[3]);
				if(player_index == 'A' + cnt % 4)
					money = message[cnt % 4];
				FD_CLR(FIFO_R, &MASTER_R);
				FD_SET(FIFO_W, &MASTER_W);
			}
		}
		if(FD_ISSET(FIFO_W, &write_set)){
			char output[512];
			sprintf(output, "%c %d %d\n", player_index, random_key, money);
			write(FIFO_W, output, strlen(output));
			fsync(FIFO_W);
			FD_CLR(FIFO_W, &MASTER_W);
			FD_SET(FIFO_R, &MASTER_R);
			money = 0;
			cnt++;
		}
	}

	close(FIFO_R);
	close(FIFO_W);
	
	return 0;
}