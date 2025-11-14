/*
 * test_phase4_messaging.c
 *
 * Test Phase 4: Message passing operations
 * Tests: send (asynchronous) and receive (blocking)
 */

#include <stdio.h>
#include <string.h>
#include "ud_thread.h"

void ping_thread(int id) {
    char msg[100];
    char recv_msg[100];
    int len;
    int sender;

    printf("Ping thread %d: Starting\n", id);

    /* Send ping message to thread 2 */
    snprintf(msg, sizeof(msg), "PING from thread %d", id);
    printf("Ping %d: Sending '%s' to thread 2\n", id, msg);
    send(2, msg, strlen(msg));

    /* Wait for pong response */
    sender = 2;  /* Expecting response from thread 2 */
    printf("Ping %d: Waiting for PONG from thread 2\n", id);
    receive(&sender, recv_msg, &len);
    recv_msg[len] = '\0';
    printf("Ping %d: Received '%s' from thread %d\n", id, recv_msg, sender);

    printf("Ping %d: Terminating\n", id);
    t_terminate();
}

void pong_thread(int id) {
    char msg[100];
    char recv_msg[100];
    int len;
    int sender;

    printf("Pong thread %d: Starting\n", id);

    /* Receive two ping messages from any sender */
    for (int i = 0; i < 2; i++) {
        sender = 0;  /* Accept from any sender */
        printf("Pong %d: Waiting for PING from any sender\n", id);
        receive(&sender, recv_msg, &len);
        recv_msg[len] = '\0';
        printf("Pong %d: Received '%s' from thread %d\n", id, recv_msg, sender);

        /* Send pong response back */
        snprintf(msg, sizeof(msg), "PONG from thread %d", id);
        printf("Pong %d: Sending '%s' to thread %d\n", id, msg, sender);
        send(sender, msg, strlen(msg));
    }

    printf("Pong %d: Terminating\n", id);
    t_terminate();
}

void broadcast_sender(int id) {
    char msg[100];

    printf("Broadcaster %d: Starting\n", id);

    /* Send messages to multiple threads */
    snprintf(msg, sizeof(msg), "Broadcast message from thread %d", id);

    printf("Broadcaster %d: Sending to thread 4\n", id);
    send(4, msg, strlen(msg));

    printf("Broadcaster %d: Sending to thread 5\n", id);
    send(5, msg, strlen(msg));

    printf("Broadcaster %d: Terminating\n", id);
    t_terminate();
}

void broadcast_receiver(int id) {
    char recv_msg[100];
    int len;
    int sender;

    printf("Receiver %d: Starting\n", id);

    /* Receive message from any sender */
    sender = 0;
    printf("Receiver %d: Waiting for message\n", id);
    receive(&sender, recv_msg, &len);
    recv_msg[len] = '\0';
    printf("Receiver %d: Received '%s' from thread %d\n", id, recv_msg, sender);

    printf("Receiver %d: Terminating\n", id);
    t_terminate();
}

int main(void) {
    printf("=== Phase 4 Test: Message Passing ===\n\n");

    t_init();

    /* Test 1: Ping-Pong communication */
    printf("--- Test 1: Ping-Pong Communication ---\n");
    t_create(pong_thread, 2, 1);   /* Create pong first */
    t_create(ping_thread, 1, 1);   /* Ping to thread 2 */
    t_create(ping_thread, 3, 1);   /* Another ping to thread 2 */

    /* Let ping-pong complete */
    for (int i = 0; i < 10; i++) {
        t_yield();
    }

    /* Test 2: Broadcast communication */
    printf("\n--- Test 2: Broadcast Communication ---\n");
    t_create(broadcast_receiver, 4, 1);
    t_create(broadcast_receiver, 5, 1);
    t_create(broadcast_sender, 6, 1);

    /* Let broadcast complete */
    for (int i = 0; i < 10; i++) {
        t_yield();
    }

    printf("\nMain: Shutting down\n");
    t_shutdown();

    printf("=== Phase 4 Message Passing Test Completed ===\n");
    return 0;
}
