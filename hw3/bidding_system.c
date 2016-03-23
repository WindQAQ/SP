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

const char* LOG_FILE = "bidding_system_log";
const char* ORDI = "ordinary\n";
const char* TERM = "terminate\n";
const int sig[] = {SIGINT, SIGUSR1, SIGUSR2};

pid_t pid;
int logfd;
int counter[3];
struct timespec interval[3];

void process(int type)
{
	counter[type]++;

	char buf[MAX];
	sprintf(buf, "receive %d %d\n", type, counter[type]);
	write(logfd, buf, strlen(buf));

	struct timespec req;
	req = interval[type];

	while(nanosleep(&req, &req) == -1)
		if(errno == EINTR)
			continue;
	
	kill(pid, sig[type]);
	sprintf(buf, "finish %d %d\n", type, counter[type]);
	write(logfd, buf, strlen(buf));
}

void member(int signo)
{
	process(1);
}

void VIP(int signo)
{
	process(2);
}

void init_time()
{
	interval[0].tv_sec = 1;
	interval[0].tv_nsec = 0;
	interval[1].tv_sec = 0;
	interval[1].tv_nsec = 500000000;
	interval[2].tv_sec = 0;
	interval[2].tv_nsec = 200000000;
}

int cmpstr(const char* str)
{
	int i = 0;
	while(ORDI[i] != '\0'){
		if(str[i] != ORDI[i])
			return 0;
		i++;
	}
	return 1;
}

int main(int argc, char* argv[])
{
	logfd = open(LOG_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	
	init_time();

	memset(counter, 0, sizeof(counter));

	struct sigaction usr1, usr2;
	
	usr1.sa_handler = member;
	sigemptyset(&usr1.sa_mask);
	usr1.sa_flags = 0;
	sigaction(SIGUSR1, &usr1, NULL);

	usr2.sa_handler = VIP;
	sigemptyset(&usr2.sa_mask);
	sigaddset(&usr2.sa_mask, SIGUSR1);
	usr2.sa_flags = 0;
	sigaction(SIGUSR2, &usr2, NULL);

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
		close(logfd);

		execlp("./customer", "./customer", argv[1], NULL);
	}

	close(pfd[0][1]);
	close(pfd[1][0]);

	char buf[MAX];
	while(1){
		int ret = read(pfd[0][0], buf, MAX);
		if(ret < 0 && errno == EINTR)
			continue;
		if(ret == 0)
			break;
		if(cmpstr(buf))
			process(0);
	}

	wait(NULL);
	write(logfd, TERM, strlen(TERM));
	close(logfd);
	
	return 0;
}