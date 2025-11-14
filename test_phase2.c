/*
 * test_phase2.c
 *
 * Test Phase 2: 2-Level Queue scheduling with time slicing
 * Tests: Priority scheduling (high vs low) and round-robin with preemption
 */

#include <stdio.h>
#include <unistd.h>
#include "ud_thread.h"

/* Counter for demonstrating time slicing */
volatile int counter = 0;

void high_priority_thread(int id) {
    printf("High priority thread %d: Starting (priority 0)\n", id);

    for (int i = 0; i < 3; i++) {
        printf("High priority thread %d: Iteration %d\n", id, i);
        /* Busy work to consume time quantum */
        for (volatile int j = 0; j < 100000; j++) {
            counter++;
        }
    }

    printf("High priority thread %d: Terminating\n", id);
    t_terminate();
}

void low_priority_thread(int id) {
    printf("Low priority thread %d: Starting (priority 1)\n", id);

    for (int i = 0; i < 3; i++) {
        printf("Low priority thread %d: Iteration %d\n", id, i);
        /* Busy work to consume time quantum */
        for (volatile int j = 0; j < 100000; j++) {
            counter++;
        }
    }

    printf("Low priority thread %d: Terminating\n", id);
    t_terminate();
}

int main(void) {
    printf("=== Phase 2 Test: 2-Level Queue and Time Slicing ===\n\n");

    t_init();

    /* Create mix of high and low priority threads */
    printf("Main: Creating low priority threads first\n");
    t_create(low_priority_thread, 1, 1);  /* Low priority */
    t_create(low_priority_thread, 2, 1);  /* Low priority */

    printf("Main: Creating high priority threads\n");
    t_create(high_priority_thread, 3, 0);  /* High priority */
    t_create(high_priority_thread, 4, 0);  /* High priority */

    printf("Main: Creating more low priority threads\n");
    t_create(low_priority_thread, 5, 1);  /* Low priority */

    printf("\nMain: Yielding - High priority threads should run first\n");
    printf("Expected: Threads 3,4 (high) run before threads 1,2,5 (low)\n\n");

    /* Let threads run */
    for (int i = 0; i < 10; i++) {
        t_yield();
    }

    printf("\nMain: Shutting down\n");
    t_shutdown();

    printf("=== Phase 2 Test Completed ===\n");
    printf("Note: Time slicing should cause preemption during busy loops\n");
    return 0;
}
