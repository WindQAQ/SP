#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

const char* LOG_FILE = "customer_log";
const char* ORDI = "ordinary\n";
const char* format[]= {"send", "finish", "timeout"};
const int sig[] = {SIGUSR1, SIGUSR2};

typedef struct tts{
	struct itimerval t;
	int type;
	int serial;
} TTS;

struct itimerval limit[2], zero;
TTS timer[2];
int logfd;
int counter[3][3];
int cur_time;
int exit_status;

struct itimerval minus_alarm(struct itimerval, struct itimerval);
int cmp_alarm(struct itimerval, struct itimerval);

void process_log(int which, int type)
{
	int key = (which == 2)? 1: which;
	counter[key][type]++;
	char buf[MAX];
	sprintf(buf, "%s %d %d\n", format[which], type, counter[key][type]);
	write(logfd, buf, strlen(buf));
}

void ordinary(int signo)
{
	process_log(1, 0);
}

void member(int signo)
{
	process_log(1, 1);
}

void VIP(int signo)
{
	process_log(1, 2);
}

void sig_alarm(int signo)
{
	if(!exit_status){
		if(!(timer[0].t.it_value.tv_sec == 0 && timer[0].t.it_value.tv_usec == 0) && counter[1][timer[0].type] < timer[0].serial){
			close(1);
			process_log(2, timer[0].type);
			exit_status = 1;
			exit(0);
		}
		if(cmp_alarm(timer[1].t, zero) != 0){
			timer[1].t = minus_alarm(timer[1].t, timer[0].t);
			timer[0] = timer[1];
			memset(&timer[1], 0, sizeof(timer[1]));
			setitimer(ITIMER_REAL, &timer[0].t, NULL);
		}
	}
}

void set_time(struct timespec* req, double t)
{
	req->tv_sec = (int)t;
	req->tv_nsec = (long)((t - (int)t) * (double)1000000000);
}

void init_limit()
{
	limit[0].it_interval.tv_sec = 0;
	limit[0].it_interval.tv_usec = 0;
	limit[0].it_value.tv_sec = 1;
	limit[0].it_value.tv_usec = 0;
	limit[1].it_interval.tv_sec = 0;
	limit[1].it_interval.tv_usec = 0;
	limit[1].it_value.tv_sec = 0;
	limit[1].it_value.tv_usec = 300000;
}

int cmp_alarm(struct itimerval req, struct itimerval rem)
{
	if(req.it_value.tv_sec > rem.it_value.tv_sec)
		return 1;
	else if(req.it_value.tv_sec < rem.it_value.tv_sec)
		return -1;

	if(req.it_value.tv_usec > rem.it_value.tv_usec)
		return 1;
	else if(req.it_value.tv_usec < rem.it_value.tv_usec)
		return -1;
	
	return 0;
}

struct itimerval minus_alarm(struct itimerval x, struct itimerval y)
{
	struct itimerval temp;
	temp.it_value.tv_sec = x.it_value.tv_sec - y.it_value.tv_sec;
	temp.it_value.tv_usec = x.it_value.tv_usec - y.it_value.tv_usec;

	if(temp.it_value.tv_usec < 0){
		temp.it_value.tv_usec += 1000000;
		temp.it_value.tv_sec--;
	}

	return temp;
}

void swap_alarm(TTS* x, TTS* y)
{
	TTS temp = *x;
	*x = *y;
	*y = temp;
}

int check()
{
	for(int i = 0; i < 3; i++)
		if(counter[0][i] != counter[1][i])
			return 1;
	return 0;
}

int main(int argc, char* argv[])
{
	logfd = open(LOG_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	FILE* infile = fopen(argv[1], "rb");

	init_limit();

	struct sigaction sint, usr1, usr2, alrm;
	
	sint.sa_handler = ordinary;
	sigemptyset(&sint.sa_mask);
	sigaddset(&sint.sa_mask, SIGUSR1);
	sigaddset(&sint.sa_mask, SIGUSR2);
	sint.sa_flags = 0;
	sigaction(SIGINT, &sint, NULL);

	usr1.sa_handler = member;
	sigemptyset(&usr1.sa_mask);
	sigaddset(&usr1.sa_mask, SIGINT);
	sigaddset(&usr1.sa_mask, SIGUSR2);
	usr1.sa_flags = 0;
	sigaction(SIGUSR1, &usr1, NULL);

	usr2.sa_handler = VIP;
	sigemptyset(&usr2.sa_mask);
	sigaddset(&usr2.sa_mask, SIGINT);
	sigaddset(&usr2.sa_mask, SIGUSR1);
	usr2.sa_flags = 0;
	sigaction(SIGUSR2, &usr2, NULL);

	alrm.sa_handler = sig_alarm;
	sigemptyset(&alrm.sa_mask);
	sigaddset(&alrm.sa_mask, SIGINT);
	sigaddset(&alrm.sa_mask, SIGUSR1);
	sigaddset(&alrm.sa_mask, SIGUSR2);
	alrm.sa_flags = 0;
	sigaction(SIGALRM, &alrm, NULL);

	memset(counter, 0, sizeof(counter));
	memset(timer, 0, sizeof(timer));
	memset(&zero, 0, sizeof(zero));

	exit_status = 0;
	int itype;
	double dtime, cur_time = 0;
	char buf[MAX];
	struct itimerval old_timer;
	int k = 0;

	while(!exit_status){

		if(fscanf(infile, "%d %lf\n", &itype, &dtime) == EOF){
			if(errno == EINTR)
				continue;
			else break;
		}

		if(exit_status)
			break;

		struct timespec req;
		set_time(&req, dtime - cur_time);
		while(nanosleep(&req, &req) == -1){
			if(errno == EINTR)
				continue;
		}

		process_log(0, itype);

		if(itype == 0)
			write(1, ORDI, strlen(ORDI));
		else{
			kill(getppid(), sig[itype - 1]);

			setitimer(ITIMER_REAL, NULL, &old_timer);
			
			if(cmp_alarm(old_timer, zero) == 0){
				memset(timer, 0, sizeof(timer));
				timer[0].t = limit[itype - 1];
				timer[0].type = itype;
				timer[0].serial = counter[0][itype];
			}
			else{
				int flag = 0;
				int index[3] = {-1, -1, -1};
				for(int i = 1; i >= 0; i--)
					index[timer[i].type] = i;

				struct itimerval diff = minus_alarm(timer[0].t, old_timer);
				timer[0].t = old_timer;
				if(cmp_alarm(timer[1].t, zero) != 0)
					timer[1].t = minus_alarm(timer[1].t, diff);

				int key = (index[itype] == -1)? index[0]: index[itype];
				timer[key].t = limit[itype - 1];
				timer[key].type = itype;
				timer[key].serial = counter[0][itype];

				if(!(key == index[0] && key == 0) && cmp_alarm(timer[0].t, timer[1].t) == 1)
					swap_alarm(&timer[0], &timer[1]);
			}
			setitimer(ITIMER_REAL, &timer[0].t, NULL);
		}

		cur_time = dtime;
	}

	if(!exit_status)
		while(check());

	close(1);
	close(logfd);
	fclose(infile);

	return 0;
}