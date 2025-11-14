/*
 * test_phase3.c
 *
 * Test Phase 3: Semaphore operations
 * Tests: sem_init, sem_wait, sem_signal, sem_destroy
 * Demonstrates mutual exclusion and synchronization
 */

#include <stdio.h>
#include "ud_thread.h"

/* Shared counter protected by semaphore */
int shared_counter = 0;
sem_t *mutex = NULL;        /* Binary semaphore for mutual exclusion */
sem_t *barrier = NULL;      /* Barrier synchronization */

void producer_thread(int id) {
    printf("Producer %d: Starting\n", id);

    for (int i = 0; i < 3; i++) {
        /* Enter critical section */
        sem_wait(mutex);
        printf("Producer %d: In critical section, incrementing counter\n", id);
        shared_counter++;
        printf("Producer %d: Counter = %d\n", id, shared_counter);
        sem_signal(mutex);
        /* Exit critical section */

        t_yield();
    }

    printf("Producer %d: Terminating\n", id);
    t_terminate();
}

void consumer_thread(int id) {
    printf("Consumer %d: Starting\n", id);

    for (int i = 0; i < 3; i++) {
        /* Enter critical section */
        sem_wait(mutex);
        printf("Consumer %d: In critical section, reading counter\n", id);
        printf("Consumer %d: Counter = %d\n", id, shared_counter);
        sem_signal(mutex);
        /* Exit critical section */

        t_yield();
    }

    printf("Consumer %d: Terminating\n", id);
    t_terminate();
}

void barrier_thread(int id) {
    printf("Barrier thread %d: Before barrier\n", id);

    /* Wait at barrier */
    sem_wait(barrier);
    printf("Barrier thread %d: Passed barrier\n", id);

    t_terminate();
}

int main(void) {
    printf("=== Phase 3 Test: Semaphore Operations ===\n\n");

    t_init();

    /* Test 1: Mutual exclusion */
    printf("--- Test 1: Mutual Exclusion ---\n");
    sem_init(&mutex, 1);  /* Binary semaphore */

    t_create(producer_thread, 1, 1);
    t_create(consumer_thread, 2, 1);
    t_create(producer_thread, 3, 1);

    /* Let them run */
    for (int i = 0; i < 15; i++) {
        t_yield();
    }

    printf("\nFinal counter value: %d (should be 6)\n", shared_counter);
    sem_destroy(&mutex);

    /* Test 2: Barrier synchronization */
    printf("\n--- Test 2: Barrier Synchronization ---\n");
    printf("Creating 3 threads that wait at barrier\n");

    sem_init(&barrier, 0);  /* Initial count 0 - all threads will block */

    t_create(barrier_thread, 4, 1);
    t_create(barrier_thread, 5, 1);
    t_create(barrier_thread, 6, 1);

    t_yield();
    printf("Main: All threads should be blocked at barrier\n");

    printf("Main: Releasing threads one by one\n");
    sem_signal(barrier);  /* Release thread 4 */
    t_yield();

    sem_signal(barrier);  /* Release thread 5 */
    t_yield();

    sem_signal(barrier);  /* Release thread 6 */
    t_yield();

    printf("Main: All barrier threads released\n");

    sem_destroy(&barrier);

    printf("\nMain: Shutting down\n");
    t_shutdown();

    printf("=== Phase 3 Test Completed ===\n");
    return 0;
}
