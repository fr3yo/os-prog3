/*
 * rendezvous.c
 *
 * Solution to Rendezvous Problem (OSTEP Chapter 31, Question 2)
 *
 * Problem: Two threads each execute two statements (a1, a2 and b1, b2).
 * Constraint: a1 must happen before b2, and b1 must happen before a2.
 *
 * Thread A:          Thread B:
 *   a1                 b1
 *   a2                 b2
 *
 * Constraint: a1 < b2 and b1 < a2
 * Solution: Use two semaphores for signaling completion of a1 and b1
 *
 * TODO: Implement this using your semaphore implementation from Phase 3
 */

#include <stdio.h>
#include "ud_thread.h"

sem_t *a1_done = NULL;   /* Signals that a1 has completed */
sem_t *b1_done = NULL;   /* Signals that b1 has completed */

/*
 * Completion semaphore used by each worker thread to notify the
 * main thread that it has finished execution.  The main thread
 * waits on this semaphore for the number of worker threads to
 * ensure all threads have terminated before cleaning up.  Without
 * this join mechanism, the main thread could call t_shutdown()
 * while worker threads are still running, leading to memory
 * leaks that valgrind will report.  See the assignment README
 * for further discussion.
 */
sem_t *done_sem = NULL;

void thread_a(int id) {
    (void)id;
    printf("Thread A: Starting\n");

    /* Statement a1 */
    printf("Thread A: Executing a1\n");
    /* Simulate some work */
    for (volatile int i = 0; i < 1000000; i++);

    /*
     * TODO: Signal that a1 is done
     * HINT: Use sem_signal() on the appropriate semaphore
     */
    printf("Thread A: a1 completed, signaling\n");
    /* Signal Thread B that a1 has completed */
    sem_signal(a1_done);

    /*
     * TODO: Wait for b1 to complete before executing a2
     * HINT: Use sem_wait() on the appropriate semaphore
     */
    printf("Thread A: Waiting for b1 to complete\n");
    /* Wait until Thread B signals completion of b1 */
    sem_wait(b1_done);

    /* Statement a2 - can only execute after b1 */
    printf("Thread A: Executing a2 (b1 has completed)\n");
    /* Simulate some work */
    for (volatile int i = 0; i < 1000000; i++);

    printf("Thread A: Terminating\n");
    /* Signal completion to main before terminating. */
    sem_signal(done_sem);
    t_terminate();
}

void thread_b(int id) {
    (void)id;
    printf("Thread B: Starting\n");

    /* Statement b1 */
    printf("Thread B: Executing b1\n");
    /* Simulate some work */
    for (volatile int i = 0; i < 1000000; i++);

    /*
     * TODO: Signal that b1 is done
     * HINT: Use sem_signal() on the appropriate semaphore
     */
    printf("Thread B: b1 completed, signaling\n");
    /* Signal Thread A that b1 has completed */
    sem_signal(b1_done);

    /*
     * TODO: Wait for a1 to complete before executing b2
     * HINT: Use sem_wait() on the appropriate semaphore
     */
    printf("Thread B: Waiting for a1 to complete\n");
    /* Wait until Thread A signals completion of a1 */
    sem_wait(a1_done);

    /* Statement b2 - can only execute after a1 */
    printf("Thread B: Executing b2 (a1 has completed)\n");
    /* Simulate some work */
    for (volatile int i = 0; i < 1000000; i++);

    printf("Thread B: Terminating\n");
    /* Signal completion to main before terminating. */
    sem_signal(done_sem);
    t_terminate();
}

int main(void) {
    printf("=== Rendezvous Problem Solution ===\n");
    printf("Constraint: a1 must complete before b2, and b1 must complete before a2\n\n");

    t_init();

    /*
     * Initialize semaphores.  Both semaphores start with count 0 so
     * that calls to sem_wait() will block until the corresponding
     * sem_signal() occurs.
     */
    sem_init(&a1_done, 0);
    sem_init(&b1_done, 0);

    /* Initialise completion semaphore for thread joins.  Starts
     * at zero so that sem_wait(done_sem) will block until a
     * worker signals it. */
    sem_init(&done_sem, 0);

    /* Create threads */
    t_create(thread_a, 1, 1);
    t_create(thread_b, 2, 1);

    /*
     * Wait for both worker threads to signal completion.  This
     * effectively joins on the threads to ensure they have
     * terminated before we clean up.
     */
    sem_wait(done_sem);
    sem_wait(done_sem);

    /*
     * Cleanup – destroy the semaphores to free memory and wake any
     * waiting threads.  This is important to avoid memory leaks.
     */
    sem_destroy(&a1_done);
    sem_destroy(&b1_done);
    /* Destroy completion semaphore */
    sem_destroy(&done_sem);

    t_shutdown();

    printf("\n=== Rendezvous Problem Completed ===\n");
    printf("Verify output shows: a1 and b1 execute first, then a2 and b2\n");
    return 0;
}
