/*
 * test_phase4_mailbox.c
 *
 * Test Phase 4: Mailbox operations
 * Tests: mbox_create, mbox_deposit, mbox_withdraw, mbox_destroy
 */

#include <stdio.h>
#include <string.h>
#include "ud_thread.h"

mbox *shared_mailbox = NULL;

void sender_thread(int id) {
    char msg[100];

    printf("Sender %d: Starting\n", id);

    for (int i = 0; i < 3; i++) {
        snprintf(msg, sizeof(msg), "Message %d from sender %d", i, id);
        printf("Sender %d: Depositing: '%s'\n", id, msg);
        mbox_deposit(shared_mailbox, msg, strlen(msg));
        t_yield();
    }

    printf("Sender %d: Terminating\n", id);
    t_terminate();
}

void receiver_thread(int id) {
    char msg[100];
    int len;

    printf("Receiver %d: Starting\n", id);

    for (int i = 0; i < 5; i++) {
        mbox_withdraw(shared_mailbox, msg, &len);

        if (len > 0) {
            msg[len] = '\0';
            printf("Receiver %d: Withdrew: '%s' (len=%d)\n", id, msg, len);
        } else {
            printf("Receiver %d: No message available\n", id);
        }

        t_yield();
    }

    printf("Receiver %d: Terminating\n", id);
    t_terminate();
}

int main(void) {
    printf("=== Phase 4 Test: Mailbox Operations ===\n\n");

    t_init();

    /* Create mailbox */
    printf("Main: Creating mailbox\n");
    if (mbox_create(&shared_mailbox) != 0) {
        printf("Failed to create mailbox\n");
        return 1;
    }

    /* Create sender and receiver threads */
    printf("Main: Creating sender threads\n");
    t_create(sender_thread, 1, 1);
    t_create(sender_thread, 2, 1);

    printf("Main: Creating receiver thread\n");
    t_create(receiver_thread, 3, 1);

    /* Let threads run */
    printf("\nMain: Yielding to let threads run\n\n");
    for (int i = 0; i < 20; i++) {
        t_yield();
    }

    /* Destroy mailbox */
    printf("\nMain: Destroying mailbox\n");
    mbox_destroy(&shared_mailbox);

    printf("Main: Shutting down\n");
    t_shutdown();

    printf("=== Phase 4 Mailbox Test Completed ===\n");
    return 0;
}
