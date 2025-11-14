/*
 * t_lib.h
 *
 * Internal header for UD Thread Library implementation
 *
 * This header contains internal data structures and helper functions.
 * It should NOT be included by application programs - only by t_lib.c
 *
 * Applications should only include ud_thread.h
 */

#ifndef T_LIB_H
#define T_LIB_H

#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

/* ========== Constants ========== */

#define STACK_SIZE 8192           /* Stack size for each thread (8KB) */
#define TIME_QUANTUM 10000        /* Time quantum in microseconds (10ms) */
#define HIGH_PRIORITY 0           /* High priority level (for Phase 2) */
#define LOW_PRIORITY 1            /* Low priority level (for Phase 2) */

/* ========== Forward Declarations ========== */

typedef struct tcb tcb;
typedef struct sem_t_internal sem_t_internal;
typedef struct messageNode messageNode;
typedef struct mbox_internal mbox_internal;

/* ========== Thread Control Block (TCB) ========== */

/*
 * TCB - Thread Control Block
 * This is the main data structure for each thread
 *
 * TODO for Phase 1: You need to understand and use:
 *   - thread_id: Unique identifier for the thread
 *   - thread_priority: 0 (high) or 1 (low) for Phase 2
 *   - thread_context: Pointer to ucontext_t (for context switching)
 *   - stack: Pointer to the thread's stack (malloc'd)
 *   - next: Pointer to next TCB (for linked list queues)
 *
 * TODO for Phase 4: You need to add/use:
 *   - msg_sem: Counting semaphore for blocking receive
 *   - msg_queue: Queue of incoming messages for this thread
 *   - mbox_lock: Binary semaphore to protect msg_queue access
 */
struct tcb {
    int thread_id;                /* Unique thread identifier */
    int thread_priority;          /* Priority: 0=high, 1=low */
    ucontext_t *thread_context;   /* Saved thread context */
    void *stack;                  /* Pointer to thread stack (malloc'd) */

    /* Phase 4: Message passing structures */
    sem_t_internal *msg_sem;      /* Semaphore for message passing (counting) */
    messageNode *msg_queue;       /* Queue of messages for this thread */
    sem_t_internal *mbox_lock;    /* Lock for message queue access */

    struct tcb *next;             /* Next TCB in queue (for linked lists) */
};

/* ========== Semaphore (Internal Definition) ========== */

/*
 * sem_t_internal - Internal semaphore structure (Phase 3)
 *
 * TODO: You need to implement semaphore operations using this structure:
 *   - count: The semaphore counter
 *            > 0: Number of available resources
 *            < 0: Absolute value = number of blocked threads
 *   - q: Queue of blocked threads (linked list of TCBs)
 */
struct sem_t_internal {
    int count;                    /* Semaphore counter */
    tcb *q;                       /* Queue of blocked threads */
};

/* ========== Message Node (Phase 4) ========== */

/*
 * messageNode - A single message in a queue
 *
 * TODO for Phase 4: Understand this structure:
 *   - message: Copy of the actual message string (malloc'd)
 *   - len: Length of the message
 *   - sender: Thread ID of the sender
 *   - receiver: Thread ID of the receiver
 *   - send_sem: For synchronous send (extra credit)
 *   - next: Next message in the queue
 */
struct messageNode {
    char *message;                /* Copy of the message (malloc'd) */
    int len;                      /* Length of the message */
    int sender;                   /* TID of sender thread */
    int receiver;                 /* TID of receiver thread */
    int received;                 /* Flag for synchronous send (extra credit) */
    sem_t_internal *send_sem;     /* Semaphore for blocking send (extra credit) */
    struct messageNode *next;     /* Pointer to next message */
};

/* ========== Mailbox (Internal Definition for Phase 4) ========== */

/*
 * mbox_internal - Internal mailbox structure
 *
 * TODO for Phase 4: Mailbox operations:
 *   - msg: Head of the message queue (linked list)
 *   - mbox_sem: Binary semaphore used as a lock (count=1)
 */
struct mbox_internal {
    messageNode *msg;             /* Message queue (linked list) */
    sem_t_internal *mbox_sem;     /* Semaphore used as lock */
};

/* ========== Global Variables ========== */

/*
 * These global variables maintain the state of the thread library
 *
 * TODO for Phase 1: You need to declare and use these in t_lib.c:
 *   - running: Pointer to the currently running thread's TCB
 *   - ready_q_high: Head of high priority ready queue (Phase 2)
 *   - ready_q_low: Head of low priority ready queue (Phase 2)
 *
 * NOTE: In Phase 1, you can use a single ready queue. In Phase 2, you need two.
 *
 * TODO for Phase 2: You also need:
 *   - timer_enabled: Flag to enable/disable timer interrupts
 */
extern tcb *running;              /* Currently running thread */
extern tcb *ready_q_high;         /* Ready queue for high priority (level 0) */
extern tcb *ready_q_low;          /* Ready queue for low priority (level 1) */
extern int timer_enabled;         /* Flag: is timer interrupt enabled? */

/* ========== Helper Functions for Queue Management ========== */

/*
 * add_to_queue - Add a TCB to the end of a queue
 *
 * @queue: Pointer to the queue head pointer
 * @node: TCB to add to the queue
 *
 * TODO: Implement this helper function to:
 *   1. If queue is empty (*queue == NULL), set *queue = node
 *   2. Otherwise, traverse to the end and add node there
 *   3. Set node->next = NULL
 *
 * This is used for ready queues, semaphore wait queues, etc.
 */
void add_to_queue(tcb **queue, tcb *node);

/*
 * remove_from_queue - Remove and return the first TCB from a queue
 *
 * @queue: Pointer to the queue head pointer
 *
 * Returns: The TCB that was removed, or NULL if queue was empty
 *
 * TODO: Implement this helper function to:
 *   1. If queue is empty, return NULL
 *   2. Save the first node: node = *queue
 *   3. Update queue head: *queue = (*queue)->next
 *   4. Set node->next = NULL
 *   5. Return node
 *
 * This is used for ready queues, semaphore wait queues, etc.
 */
tcb *remove_from_queue(tcb **queue);

/*
 * get_tcb_by_id - Find a thread's TCB given its thread ID
 *
 * @tid: Thread ID to search for
 *
 * Returns: Pointer to the TCB, or NULL if not found
 *
 * TODO for Phase 4: Implement this to search:
 *   1. The running thread
 *   2. Both ready queues (high and low priority)
 *   3. Return the TCB when found, NULL otherwise
 *
 * Needed for message passing (to find the recipient thread)
 */
tcb *get_tcb_by_id(int tid);

/* ========== Scheduling Functions (Phase 2) ========== */

/*
 * schedule - Perform context switch to the next ready thread
 *
 * TODO for Phase 2: Implement the 2-level queue scheduler:
 *   1. Select next thread:
 *      a. First check ready_q_high (high priority queue)
 *      b. If empty, check ready_q_low (low priority queue)
 *      c. If both empty, return (no threads to schedule)
 *   2. Remove selected thread from ready queue
 *   3. Make it the new running thread
 *   4. Start timer for the new thread
 *   5. Context switch:
 *      - If no previous thread (first switch): use setcontext()
 *      - If previous thread exists: use swapcontext()
 *
 * This function is called by:
 *   - t_yield() (voluntary)
 *   - t_terminate() (thread ending)
 *   - timer_handler() (preemption)
 */
void schedule(void);

/*
 * timer_handler - Signal handler for SIGALRM (timer interrupt)
 *
 * @sig: Signal number (will be SIGALRM)
 *
 * TODO for Phase 2: Implement preemptive scheduling:
 *   1. This function is called when the time quantum expires
 *   2. Move current thread to end of its ready queue (based on priority)
 *   3. Call schedule() to switch to next thread
 *
 * NOTE: This implements preemptive scheduling (time slicing)
 */
void timer_handler(int sig);

/*
 * start_timer - Start the interval timer
 *
 * TODO for Phase 2: Implement this to:
 *   1. Use ualarm(TIME_QUANTUM, 0) to schedule a SIGALRM
 *   2. Only start if timer_enabled is true
 *
 * HINT: ualarm(usec, interval) schedules alarm in microseconds
 *       Use interval=0 for single-shot (we'll restart after each switch)
 */
void start_timer(void);

/*
 * stop_timer - Cancel the interval timer
 *
 * TODO for Phase 2: Implement this to:
 *   1. Use ualarm(0, 0) to cancel any pending alarm
 *
 * Called when a thread yields early (before quantum expires)
 */
void stop_timer(void);

/* ========== Tips and Hints ========== */

/*
 * PHASE 1 TIPS:
 * -------------
 * 1. Start by implementing queue operations (add_to_queue, remove_from_queue)
 * 2. In t_init(), create the main thread's TCB and context
 * 3. In t_create(), use getcontext(), then modify the context:
 *    - Allocate stack: malloc(STACK_SIZE)
 *    - Set context->uc_stack.ss_sp = stack
 *    - Set context->uc_stack.ss_size = STACK_SIZE
 *    - Use makecontext(context, func, 1, arg) to set start function
 * 4. In t_yield() and t_terminate(), use swapcontext() or setcontext()
 * 5. Test with simple programs that create threads and yield
 *
 * PHASE 2 TIPS:
 * -------------
 * 1. Add signal(SIGALRM, timer_handler) in t_init()
 * 2. Use sighold(SIGALRM) and sigrelse(SIGALRM) to make functions atomic
 * 3. Implement 2-level queue: always schedule from high priority first
 * 4. In timer_handler(), move current thread to ready queue and schedule next
 * 5. Test with threads of different priorities
 *
 * PHASE 3 TIPS:
 * -------------
 * 1. Semaphore operations must be atomic (use sighold/sigrelse)
 * 2. In sem_wait(): count--; if (count < 0) block thread
 * 3. In sem_signal(): count++; if (count <= 0) wake a thread
 * 4. Blocked threads go into semaphore's wait queue
 * 5. Test with producer-consumer or mutex examples
 *
 * PHASE 4 TIPS:
 * -------------
 * 1. Each thread has its own message queue (msg_queue in TCB)
 * 2. Use a counting semaphore (msg_sem) for blocking receive
 * 3. Use a binary semaphore (mbox_lock) to protect queue access
 * 4. In send(): enqueue message, signal recipient's msg_sem
 * 5. In receive(): wait on msg_sem, lock queue, search for match, unlock
 * 6. Remember to initialize msg_sem and mbox_lock on first use
 * 7. Test with ping-pong or broadcast patterns
 */

#endif /* T_LIB_H */
