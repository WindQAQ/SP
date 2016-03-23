/*b03902042 宋子維*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>

int handle_announce(char player_index, int *message)
{
	int cur_index = player_index - 'A';
	int cur_money = message[cur_index];

	int flag = 1, max = -1;
	for(int i = 0; i < 4; i++)
		if(i != cur_index){
			if(message[i] >= message[cur_index])
				flag = 0;
			max = (max > message[i])? max: message[i];
		}

	if(flag == 1)
		return (max + 1);

	if(max != cur_money)
		return cur_money;

	for(int i = 3; i >= 0; i--)
		for(int j = 0; j < i; j++){
			if(message[j] < message[j+1]){
				int temp = message[j];
				message[j] = message[j+1];
				message[j+1] = temp;
			}
		}

	int second = -1;
	for(int i = 0; i < 4; i++){
		if(max != message[i]){
			second = i;
			break;
		}
	}

	if(second == -1)
		return cur_money;

	return message[second] + 1;
}

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

	srand(time(NULL));
	while(cnt < 10){
		read_set = MASTER_R;
		write_set = MASTER_W;
		select(max_fd+1, &read_set, &write_set, NULL, NULL);
		if(FD_ISSET(FIFO_R, &read_set)){
			char buffer[512];
			if(read(FIFO_R, buffer, 512) > 0){
				sscanf(buffer, "%d %d %d %d\n", &message[0], &message[1], &message[2], &message[3]);
				money = handle_announce(player_index, message);
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