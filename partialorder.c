/*
 * partialorder.c
 *
 * Solution to Partial Order Problem
 *
 * Problem: Given multiple threads executing statements, enforce a partial
 * ordering constraint using semaphores.
 *
 * Example constraint for 3 threads:
 *   Thread 1: a1, a2
 *   Thread 2: b1, b2
 *   Thread 3: c1, c2
 *
 * Partial order: a1 < b1 < c1 < a2, b2, c2
 * (a1 must complete before b1, b1 before c1, c1 before a2/b2/c2)
 *
 * TODO: Implement this using your semaphore implementation from Phase 3
 */

#include <stdio.h>
#include "ud_thread.h"

sem_t *a1_done = NULL;   /* Signals a1 completion */
sem_t *b1_done = NULL;   /* Signals b1 completion */
sem_t *c1_done = NULL;   /* Signals c1 completion */

void thread_a(int id) {
    (void)id;
    printf("Thread A: Starting\n");

    /* Statement a1 - can execute immediately */
    printf("Thread A: Executing a1 (first in sequence)\n");
    for (volatile int i = 0; i < 500000; i++);

    /*
     * TODO: Signal a1 completion
     * HINT: What semaphore should you signal?
     */
    printf("Thread A: a1 completed, signaling\n");
    /* Signal that a1 has completed so Thread B may proceed */
    sem_signal(a1_done);

    /*
     * TODO: Wait for c1 to complete (ensuring a1 < b1 < c1 < a2)
     * HINT: Which semaphore represents c1 completion?
     */
    printf("Thread A: Waiting for c1 to complete before a2\n");
    /* Wait until Thread C signals that c1 is done */
    sem_wait(c1_done);

    /* Statement a2 - executes after c1 */
    printf("Thread A: Executing a2 (c1 has completed)\n");
    for (volatile int i = 0; i < 500000; i++);

    printf("Thread A: Terminating\n");
    t_terminate();
}

void thread_b(int id) {
    (void)id;
    printf("Thread B: Starting\n");

    /*
     * TODO: Wait for a1 to complete before executing b1
     * HINT: Thread B must wait for Thread A's first statement
     */
    printf("Thread B: Waiting for a1 to complete before b1\n");
    /* Wait for Thread A's a1 to finish */
    sem_wait(a1_done);

    /* Statement b1 - executes after a1 */
    printf("Thread B: Executing b1 (a1 has completed)\n");
    for (volatile int i = 0; i < 500000; i++);

    /*
     * TODO: Signal b1 completion
     */
    printf("Thread B: b1 completed, signaling\n");
    /* Signal that b1 has completed so Thread C may proceed */
    sem_signal(b1_done);

    /*
     * TODO: Wait for c1 to complete
     */
    printf("Thread B: Waiting for c1 to complete before b2\n");
    /* Wait until Thread C signals c1 completion */
    sem_wait(c1_done);

    /* Statement b2 - executes after c1 */
    printf("Thread B: Executing b2 (c1 has completed)\n");
    for (volatile int i = 0; i < 500000; i++);

    printf("Thread B: Terminating\n");
    t_terminate();
}

void thread_c(int id) {
    (void)id;
    printf("Thread C: Starting\n");

    /*
     * TODO: Wait for b1 to complete before executing c1
     * HINT: Thread C must wait for Thread B's first statement
     */
    printf("Thread C: Waiting for b1 to complete before c1\n");
    /* Wait for Thread B's b1 to finish */
    sem_wait(b1_done);

    /* Statement c1 - executes after b1 */
    printf("Thread C: Executing c1 (b1 has completed)\n");
    for (volatile int i = 0; i < 500000; i++);

    /*
     * TODO: Signal c1 completion TWICE (for both thread A and B)
     * HINT: Both threads A and B are waiting for c1 to complete
     *       How many times should you signal c1_done?
     */
    printf("Thread C: c1 completed, signaling (twice for A and B)\n");
    /* Signal completion of c1 twice – once for A and once for B */
    sem_signal(c1_done);
    sem_signal(c1_done);

    /* Statement c2 - can execute immediately after c1 */
    printf("Thread C: Executing c2\n");
    for (volatile int i = 0; i < 500000; i++);

    printf("Thread C: Terminating\n");
    t_terminate();
}

int main(void) {
    printf("=== Partial Order Problem Solution ===\n");
    printf("Partial order constraint: a1 < b1 < c1 < a2, b2, c2\n");
    printf("Expected sequence: a1 -> b1 -> c1 -> a2/b2/c2 (any order)\n\n");

    t_init();

    /*
     * Initialize semaphores.  All semaphores start with count 0 so
     * that any calls to sem_wait() will block until signaled.  This
     * enforces the required ordering constraints between threads.
     */
    sem_init(&a1_done, 0);
    sem_init(&b1_done, 0);
    sem_init(&c1_done, 0);

    /* Create threads in arbitrary order to show synchronization works */
    t_create(thread_c, 3, 1);
    t_create(thread_a, 1, 1);
    t_create(thread_b, 2, 1);

    /* Let threads run */
    for (int i = 0; i < 15; i++) {
        t_yield();
    }

    /*
     * Cleanup – destroy semaphores to free memory and wake any
     * threads that might still be waiting.  This prevents memory
     * leaks in the semaphore implementation.
     */
    sem_destroy(&a1_done);
    sem_destroy(&b1_done);
    sem_destroy(&c1_done);

    t_shutdown();

    printf("\n=== Partial Order Problem Completed ===\n");
    printf("Verify output shows correct ordering: a1, b1, c1, then a2/b2/c2\n");
    return 0;
}
