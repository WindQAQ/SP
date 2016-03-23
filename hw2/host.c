/*b03902042 宋子維*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>

typedef struct player{
	int id;
	int money;
	int score;
	int rank;
} Player;

int win_price(int *money)
{
	Player p[4];
	for(int i = 0; i < 4; i++){
		p[i].id = i;
		p[i].money = money[i];
	}
	int record[4] = {0};
	for(int i = 0; i < 4; i++)
		for(int j = i+1; j < 4; j++){
			if(p[i].money == p[j].money){
				record[i]++;
				record[j]++;
			}
		}

	Player tmp[4];
	int cnt = 0;
	for(int i = 0; i < 4; i++)
		if(record[i] == 0){
			tmp[cnt].id = p[i].id;
			tmp[cnt].money = p[i].money;
			cnt++;
		}

	if(cnt == 0)
		return -1;
	int id, max = -1000;
	for(int i = 0; i < cnt; i++)
		if(max < tmp[i].money){
			id = tmp[i].id;
			max = tmp[i].money;
		}
	return id;
}

void bubble_sort(Player *p)
{
	Player temp;
	for(int i = 3; i > 0; i--)
		for(int j = 0; j < i; j++)
			if(p[j].score < p[j+1].score){
				temp = p[j];
				p[j] = p[j+1];
				p[j+1] = temp;
			}
}

void handle_response(char *res, int *money)
{
	char c;
	int ran, mt;
	if(sscanf(res, "%c %d %d", &c, &ran, &mt) == 3)
		money[c - 'A'] = mt;
}

void handle_score(char *ori, Player *p, int *money)
{
	int index = win_price(money);
	if(index == -1)
		return;
	p[index].money -= money[index];
	p[index].score++;
	for(int i = 0; i < 4; i++)
		p[i].money += 1000;
	char buf[512];
	sprintf(buf, "%d %d %d %d\n", p[0].money, p[1].money, p[2].money, p[3].money);
	strcpy(ori, buf);
}

int main(int argc, char *argv[])
{
	int host_id = atoi(argv[1]);
	// variables for FIFO name
	char response[512];
	char mess[4][512];
	// create response FIFO
	sprintf(response, "host%d.FIFO", host_id);
	for(int i = 0; i < 4; i++)
		sprintf(mess[i], "host%d_%c.FIFO", host_id, 'A'+i);
	// random key
	srand(time(NULL));

	while(1){
		// read from pipe of bidding system(stdin)
		char p_buf[512];
		read(0, p_buf, 512);
		Player p[4];
		sscanf(p_buf, "%d %d %d %d\n", &p[0].id, &p[1].id, &p[2].id, &p[3].id);
		if(p[0].id == -1 && p[1].id == -1 && p[2].id == -1 && p[3].id == -1)
			break;
		// initialize money
		for(int i = 0; i < 4; i++){
			p[i].money = 1000;
			p[i].score = 0;
		}
		mkfifo(response, 0777);
		// fork to execute player
		int FIFO_W[4], max_fd = 0;
		fd_set MASTER_R, MASTER_W;
		FD_ZERO(&MASTER_W);
		FD_ZERO(&MASTER_R);
		pid_t pid;
		for(int i = 0; i < 4; i++){
			mkfifo(mess[i], 0777);
			int random = rand() % 65536;
			if((pid = fork()) == 0){
				char player_index[4];
				char random_str[16];
				sprintf(random_str, "%d", random);
				sprintf(player_index, "%c", 'A'+i);
				execlp("./player", "./player", argv[1], player_index, random_str, NULL);
			}
			else{
				FIFO_W[i] = open(mess[i], O_WRONLY);
				FD_SET(FIFO_W[i], &MASTER_W);
				max_fd = (max_fd > FIFO_W[i])? max_fd: FIFO_W[i];
			}
		}
		// open read FIFO
		int FIFO_R = open(response, O_RDONLY);
		max_fd = (max_fd > FIFO_R)? max_fd: FIFO_R;
		// i/o multiplexing on FIFO
		fd_set read_set, write_set;
		char write_str[512];
		strcpy(write_str, "1000 1000 1000 1000\n");
		int cnt = 0, already_r = 0;
		int money[4];

		while(cnt < 10){
			read_set = MASTER_R;
			write_set = MASTER_W;
			select(max_fd+1, &read_set, &write_set, NULL, NULL);
			for(int i = 0; i < 4; i++){
				if(FD_ISSET(FIFO_W[i], &write_set) && !FD_ISSET(FIFO_R, &MASTER_R)){
					write(FIFO_W[i], write_str, strlen(write_str));
					fsync(FIFO_W[i]);
					FD_CLR(FIFO_W[i], &MASTER_W);
					FD_SET(FIFO_R, &read_set);
					FD_SET(FIFO_R, &MASTER_R);
					break;
				}
			}
			if(FD_ISSET(FIFO_R, &read_set)){
				char buf[512];
				if(read(FIFO_R, buf, 512) > 0){
					handle_response(buf, money);
					FD_CLR(FIFO_R, &read_set);
					FD_CLR(FIFO_R, &MASTER_R);
					already_r++;
				}
			}
			if(already_r == 4){
				handle_score(write_str, p, money);
				for(int i = 0; i < 4; i++)
					FD_SET(FIFO_W[i], &MASTER_W);
				FD_CLR(FIFO_R, &MASTER_R);
				already_r = 0;
				cnt++;
			}
		}
		// wait for all children 
		for(int i = 0; i < 4; i++){
			wait(NULL);
		}
		// close and remove FIFO
		close(FIFO_R);
		unlink(response);
		for(int i = 0; i < 4; i++){
			close(FIFO_W[i]);
			unlink(mess[i]);
		}
		// deal with output
		bubble_sort(p);
		
		Player rank[32];
		int j = 1, pre = -1, k = 0;
		for(int i = 0; i < 4; i++){
			if(pre == p[i].score)
				k++;
			else{
				j += k;
				k = 1;
			}
			rank[i].id = p[i].id;
			rank[i].rank = j;
			pre = p[i].score;
		}
		// write into pipe of bidding_system(stdout)
		char output[512];
		sprintf(output, "%d %d\n%d %d\n%d %d\n%d %d\n",
						rank[0].id, rank[0].rank, rank[1].id, rank[1].rank, 
						rank[2].id, rank[2].rank, rank[3].id, rank[3].rank);
		write(1, output, strlen(output));
		fsync(1);
	}

	return 0;
}