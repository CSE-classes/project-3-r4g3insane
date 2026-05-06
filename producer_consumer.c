#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 5

/* Shared Circular Buffer */
char buffer[BUFFER_SIZE];
int in = 0;       
int out = 0;      
int count = 0;    

/* Flag to signal when the producer has finished reading the file */
int file_read_complete = 0;

/* Mutex and Condition Variables */
pthread_mutex_t mutex;
pthread_cond_t cond_full;  
pthread_cond_t cond_empty; 

/* Producer Thread Function */
void *producer_thread(void *arg) {
    FILE *file = fopen("message.txt", "r");
    if (file == NULL) {
        perror("Failed to open message.txt");
        exit(EXIT_FAILURE);
    }

    char ch;
    // Read the file character by character
    while ((ch = fgetc(file)) != EOF) {
        pthread_mutex_lock(&mutex);

        // Wait if the buffer is full
        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&cond_empty, &mutex);
        }

        // Insert character into the circular queue
        buffer[in] = ch;
        in = (in + 1) % BUFFER_SIZE;
        count++;

        // Signal the consumer that the buffer is no longer empty
        pthread_cond_signal(&cond_full);
        
        pthread_mutex_unlock(&mutex);
    }

    fclose(file);

    // Notify the consumer that no more data will be produced
    pthread_mutex_lock(&mutex);
    file_read_complete = 1;
    pthread_cond_signal(&cond_full); // Wake up the consumer if it's waiting for more data
    pthread_mutex_unlock(&mutex);

    return NULL;
}

/* Consumer Thread Function */
void *consumer_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);

        // Wait if the buffer is empty, UNLESS the producer is done
        while (count == 0 && !file_read_complete) {
            pthread_cond_wait(&cond_full, &mutex);
        }

        // If the buffer is empty and the producer is done, exit the loop
        if (count == 0 && file_read_complete) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        // Read character from the circular queue
        char ch = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;

        // Print the character sequentially
        printf("%c", ch);
        fflush(stdout); // Ensure immediate output to the console

        // Signal the producer that space is now available in the buffer
        pthread_cond_signal(&cond_empty);
        
        pthread_mutex_unlock(&mutex);
    }
    
    printf("\n"); // Add a newline at the very end of the output
    return NULL;
}

int main() {
    pthread_t producer, consumer;

    // Initialize synchronization primitives
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_full, NULL);
    pthread_cond_init(&cond_empty, NULL);

    // Create the producer and consumer threads
    if (pthread_create(&producer, NULL, producer_thread, NULL) != 0) {
        perror("Failed to create producer thread");
        return 1;
    }
    if (pthread_create(&consumer, NULL, consumer_thread, NULL) != 0) {
        perror("Failed to create consumer thread");
        return 1;
    }

    // Wait for both threads to finish execution
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    // Clean up synchronization primitives
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_full);
    pthread_cond_destroy(&cond_empty);

    return 0;
}