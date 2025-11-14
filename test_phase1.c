/*
 * test_phase1.c
 *
 * Simple test for Phase 1: Basic thread operations
 *
 * This test creates a few threads and has them yield to each other.
 * Expected behavior: Threads should run in round-robin fashion as they yield.
 */

#include <stdio.h>
#include "ud_thread.h"

void thread_function(int id) {
    printf("Thread %d: Starting\n", id);

    /* Yield to demonstrate cooperative multitasking */
    printf("Thread %d: Yielding CPU\n", id);
    t_yield();

    printf("Thread %d: Resumed after yield\n", id);

    /* Yield again */
    t_yield();

    printf("Thread %d: Terminating\n", id);
    t_terminate();
}

int main(void) {
    printf("=== Phase 1 Test: Basic Thread Operations ===\n\n");

    /* Initialize thread library */
    printf("Main: Initializing thread library\n");
    t_init();

    /* Create threads with low priority (priority 1) */
    printf("Main: Creating thread 1\n");
    t_create(thread_function, 1, 1);

    printf("Main: Creating thread 2\n");
    t_create(thread_function, 2, 1);

    printf("Main: Creating thread 3\n");
    t_create(thread_function, 3, 1);

    /* Main thread yields to let other threads run */
    printf("Main: Yielding to other threads\n");
    t_yield();

    printf("Main: Back in main thread\n");
    t_yield();

    printf("Main: Back in main again\n");
    t_yield();

    printf("Main: All threads should have run\n");

    /* Give threads more time to complete */
    for (int i = 0; i < 10; i++) {
        t_yield();
    }

    /* Shutdown library */
    printf("\nMain: Shutting down thread library\n");
    t_shutdown();

    printf("=== Phase 1 Test Completed ===\n");
    return 0;
}
