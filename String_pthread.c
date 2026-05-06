
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define NUM_THREADS	4
#define MAX 1024

void *sub_string(void *);
int readf(FILE *fp);
int total=0;
int nlocal,n1,n2;
char *s1,*s2;
FILE *fp;
pthread_mutex_t total_lock;

int main(int argc, char *argv[])
{
	int i,rc;
	pthread_t threads[NUM_THREADS];

	pthread_mutex_init(&total_lock,NULL);
	readf(fp);
	for(i=0;i<NUM_THREADS;i++){
		rc=pthread_create(&threads[i],NULL,sub_string,(void *)(intptr_t)i);
		if (rc){
			printf("ERROR: return error from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	for(i=0; i<NUM_THREADS; i++){
		rc = pthread_join(threads[i], NULL);
		if (rc){
			printf("ERROR: return error from pthread_join() is %d\n", rc);
			exit(-1);
		}
	}
	printf("the occurences of s2 in s1 is %d\n",total);
	pthread_exit(0);
}



int readf(FILE *fp)
{
	if((fp=fopen("strings.txt", "r"))==NULL){
		printf("ERROR: can't open string.txt!\n");
		return 0;
	}
	s1=(char *)malloc(sizeof(char)*MAX);
	if(s1==NULL){
		printf("ERROR: Out of memory!\n");
		return -1;
	}
	s2=(char *)malloc(sizeof(char)*MAX);
	if(s1==NULL){
		printf("ERROR: Out of memory\n");
		return -1;
	}
	/*read s1 s2 from the file*/
	s1=fgets(s1, MAX, fp);
	s2=fgets(s2, MAX, fp);
	n1=strlen(s1);  /*length of s1*/
	if(s1[n1-1] == '\n') n1--;  /* strip newline */
	n2=strlen(s2);  /*length of s2*/
	if(s2[n2-1] == '\n') n2--;  /* strip newline */
	nlocal=n1/NUM_THREADS;  /*data length held by process*/
	if(s1==NULL || s2==NULL ||n1<n2)  /*when error exit*/
		return -1;
}

void *sub_string(void *threadid) 	/*each process searches in the string with the step of nprocs until it reach or beyond*/ 
	/*the (n1-n2)th char which is the last possible beginning of the substring*/
{
	int tid = (int)(intptr_t)threadid;
	int start = tid * nlocal;	/* starting position for this thread */
	int end;					/* ending position for this thread */
	int i, j, k;
	int count = 0;
	int local_count = 0;		/* local counter for this thread */
	
	/* calculate the ending position */
	if(tid == NUM_THREADS - 1) {
		end = n1 - n2;			/* last thread searches to the end */
	} else {
		end = (tid + 1) * nlocal - 1;	/* exclusive boundary for this thread */
		/* extend end to include potential boundary matches */
		if(end + n2 - 1 < n1 - n2) {
			end = end + n2 - 1;	/* overlap to catch boundary substrings */
		}
	}
	
	/* search for matching substrings in the assigned region */
	for(i = start; i <= end && i <= (n1 - n2); i++) {
		count = 0;
		for(j = i, k = 0; k < n2; j++, k++) {
			if(*(s1 + j) != *(s2 + k)) {
				break;
			} else {
				count++;
			}
		}
		if(count == n2) {
			/* only count if this thread owns this position */
			if(tid == NUM_THREADS - 1 || i < (tid + 1) * nlocal) {
				local_count++;
			}
		}
	}
	
	/* add local count to global total (protected by mutex) */
	pthread_mutex_lock(&total_lock);
	total += local_count;
	pthread_mutex_unlock(&total_lock);
	
	pthread_exit(NULL);
}







