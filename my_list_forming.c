/*
  list-forming.c: 
  Each thread generates a data node, attaches it to a global list. This is reapeated for K times.
  There are num_threads threads. The value of "num_threads" is input by the student.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sched.h>

#define K 200

struct Node
{
    int data;
    struct Node* next;
};

struct list
{
     struct Node * header;
     struct Node * tail;
};

pthread_mutex_t    mutex_lock;

struct list *List;

void bind_thread_to_cpu(int cpuid) {
     cpu_set_t mask;
     CPU_ZERO(&mask);

     CPU_SET(cpuid, &mask);
     if (sched_setaffinity(0, sizeof(cpu_set_t), &mask)) {
         fprintf(stderr, "sched_setaffinity");
         exit(EXIT_FAILURE);
     }
}

struct Node* generate_data_node()
{
    struct Node *ptr;
    ptr = (struct Node *)malloc(sizeof(struct Node));    

    if( NULL != ptr ){
        ptr->next = NULL;
    }
    else {
        printf("Node allocation failed!\n");
    }
    return ptr;
}

void * producer_thread( void *arg)
{
    bind_thread_to_cpu(*((int*)arg));//bind this thread to a CPU

    struct Node * ptr;
    struct Node * local_head = NULL;    /* head of local list */
    struct Node * local_tail = NULL;    /* tail of local list */
    int counter = 0;  

    /* generate K nodes and build a local list (no locking needed here) */
    while( counter < K )
    {
        ptr = generate_data_node();

        if( NULL != ptr )
        {
            ptr->data = 1;  /* generate data */
            
            /* attach to local list */
            if( local_head == NULL )
            {
                local_head = local_tail = ptr;
            }
            else
            {
                local_tail->next = ptr;
                local_tail = ptr;
            }
        }
        ++counter;
    }
    
    /* now attach the entire local list to the global list (single lock acquisition) */
    if( local_head != NULL )
    {
        pthread_mutex_lock(&mutex_lock);
        
        if( List->header == NULL )
        {
            List->header = local_head;
            List->tail = local_tail;
        }
        else
        {
            List->tail->next = local_head;
            List->tail = local_tail;
        }
        
        pthread_mutex_unlock(&mutex_lock);
    }
}

int main(int argc, char* argv[])
{
    int i, num_threads;

    int NUM_PROCS;  /* number of CPU */
    int* cpu_array = NULL;

    struct Node  *tmp, *next;
    struct timeval starttime, endtime;

    num_threads = atoi(argv[1]); /* read num_threads from user */
    pthread_t producer[num_threads];
    NUM_PROCS = sysconf(_SC_NPROCESSORS_CONF);  /* get number of CPU */
    if( NUM_PROCS > 0)
    {
        cpu_array = (int *)malloc(NUM_PROCS*sizeof(int));
        if( cpu_array == NULL )
        {
            printf("Allocation failed!\n");
            exit(0);
        }
        else
        {
            for( i = 0; i < NUM_PROCS; i++)
               cpu_array[i] = i;
        }

    }

    pthread_mutex_init(&mutex_lock, NULL);

    List = (struct list *)malloc(sizeof(struct list));
    if( NULL == List )
    {
       printf("End here\n");
       exit(0);	
    }
    List->header = List->tail = NULL;

    gettimeofday(&starttime,NULL); /* get program start time */
    for( i = 0; i < num_threads; i++ )
    {
        pthread_create(&(producer[i]), NULL, (void *) producer_thread, &cpu_array[i%NUM_PROCS]); 
    }

    for( i = 0; i < num_threads; i++ )
    {
        if(producer[i] != 0)
        {
            pthread_join(producer[i],NULL);
        }
    }

    gettimeofday(&endtime,NULL); /* get the finish time */

    if( List->header != NULL )
    {
        next = tmp = List->header;
        while( tmp != NULL )
        {  
           next = tmp->next;
           free(tmp);
           tmp = next;
        }            
    }
    if( cpu_array!= NULL)
       free(cpu_array);
    
    /* calculate program runtime */
    printf("Total run time is %ld microseconds.\n", (endtime.tv_sec-starttime.tv_sec) * 1000000+(endtime.tv_usec-starttime.tv_usec));
    return 0; 
}
