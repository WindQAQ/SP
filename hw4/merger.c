#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

typedef struct info{
	int l, r;
} Info;

typedef struct pminfo{
	int l1, r1, l2, r2;
} pmInfo;

int* A;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t v = PTHREAD_MUTEX_INITIALIZER; 


int cmp(const void* a, const void* b)
{
	int pa = *(int*) a;
	int pb = *(int*) b;
	if(pa > pb) return 1;
	else if(pa < pb) return -1;
	else return 0;
}

void* msort(void *range)
{
	Info* prange = (Info*) range;
	int l = prange->l, r = prange->r;
	int size = r-l;
	
	pthread_mutex_lock(&m);
	printf("Handling elements:\n");
	for(int i = l; i < r; i++)
		printf("%d%s", A[i], (i == r-1)? "\n": " ");
	printf("Sorted %d elements.\n", size);
	pthread_mutex_unlock(&m);
	qsort(A+l, size, sizeof(int), cmp);

	pthread_exit(NULL);	
}

void* pmerge(void* range)
{
	pmInfo* prange = (pmInfo*) range;
	int l1 = prange->l1, r1 = prange->r1;
	int l2 = prange->l2, r2 = prange->r2;
	int size1 = r1 - l1, size2 = r2 - l2;
	
	int iteration1 = l1, iteration2 = l2, k = 0;
	int duplicate = 0;
	int* t;
	t = (int *) malloc((size1 + size2 + 4) * sizeof(int));
	
	while(iteration1 != r1 && iteration2 != r2){
		if(A[iteration1] < A[iteration2])
			t[k++] = A[iteration1++];
		else if(A[iteration1] > A[iteration2])
			t[k++] = A[iteration2++];
		else if(A[iteration1] == A[iteration2]){
			duplicate++;
			t[k++] = A[iteration1++];
		}
	}

	while(iteration1 != r1) t[k++] = A[iteration1++];

	while(iteration2 != r2) t[k++] = A[iteration2++];

	pthread_mutex_lock(&v);
	printf("Handling elements:\n");
	for(int i = l1; i < r2; i++)
		printf("%d%s", A[i], (i == r2-1)? "\n": " ");
	
	printf("Merged %d and %d elements with %d duplicates.\n", size1, size2, duplicate);
	memcpy(A+l1, t, sizeof(int) * (size1+size2));
	free(t);
	pthread_mutex_unlock(&v);
	
	pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
	int seg_size = atoi(argv[1]);	
	
	int n;	
	scanf("%d", &n);
	A = (int*) malloc((n+4) * sizeof(int));

	for(int i = 0; i < n; i++)
		scanf("%d", &A[i]);
	
	int thread_num = n / seg_size;
	if(n % seg_size != 0) thread_num++;

	pthread_t* tid;
	Info* range;
	int* size;
	tid = (pthread_t*) malloc((thread_num + 4) * sizeof(pthread_t));
	range = (Info*) malloc((thread_num + 4) * sizeof(Info));	
	size = (int*) malloc((thread_num + 4) * sizeof(int));	

	for(int i = 0; i < thread_num; i++){
		range[i].l = i * seg_size;
		range[i].r = (i + 1) * seg_size;
		if(i == thread_num - 1) range[i].r = n;
		size[i] = range[i].r - range[i].l;
		pthread_create(&tid[i], NULL, &msort, &range[i]);
	}
	
	for(int i = 0; i < thread_num; i++)
		pthread_join(tid[i], NULL);
		
	free(tid);
	free(range);	
	
	while(thread_num / 2 != 0){
		
		int odd = 0, origin;
		if(thread_num % 2 == 1){
			odd = 1;
			origin = thread_num;
		}

		thread_num /= 2;		

		pthread_t* tid;
		pmInfo* range;
		tid = (pthread_t*) malloc((thread_num + 4) * sizeof(pthread_t));
		range = (pmInfo*) malloc((thread_num + 4) * sizeof(pmInfo));
		
		int pre = 0;
		for(int i = 0; i < thread_num; i++){
			range[i].l1 = pre;
			range[i].r1 = range[i].l1 + size[2*i];
			range[i].l2 = range[i].r1;
			range[i].r2 = range[i].l2 + size[2*i+1];
			size[i] = range[i].r2 - range[i].l1;
			pre = range[i].r2;
			pthread_create(&tid[i], NULL, &pmerge, &range[i]);
		}
		
		for(int i = 0; i < thread_num; i++)
			pthread_join(tid[i], NULL);

		if(odd == 1) size[thread_num] = size[origin - 1];

		thread_num += odd;

		free(tid);
		free(range);
	}

	for(int i = 0; i < n; i++)
		printf("%d%s", A[i], (i == n-1)? "\n": " ");

	free(size);

	return 0;
}
