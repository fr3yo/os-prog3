/*
 * ud_thread.h
 *
 * User-level thread library - Public API
 *
 * This header should be included by YOUR applications (such as test programs).
 * It is equivalent to pthread.h for POSIX threads.
 *
 * IMPORTANT: This file should ONLY contain function prototypes and type definitions
 * that are visible to user applications. Internal implementation details should
 * go in t_lib.h
 */

#ifndef UD_THREAD_H
#define UD_THREAD_H

/* ========== Phase 1: Basic Thread Operations ========== */

/*
 * t_init - Initialize the thread library
 *
 * TODO: You need to implement this function to:
 *   1. Set up the "running" queue (initially with main thread)
 *   2. Set up the "ready" queue (initially empty)
 *   3. Create a TCB for the main thread with:
 *      - thread_id = 0
 *      - thread_priority = 1 (low priority)
 *   4. Insert main thread's TCB into the running queue
 *   5. (Phase 2) Set up the signal handler for SIGALRM
 */
void t_init(void);

/*
 * t_shutdown - Shut down the thread library
 *
 * TODO: You need to implement this function to:
 *   1. Free all dynamically allocated memory
 *   2. This includes: all TCBs, stacks, contexts, and queues
 *   3. Prevent memory leaks!
 */
void t_shutdown(void);

/*
 * t_create - Create a new thread
 *
 * @func: Start function for the thread (signature: void func(int))
 * @thr_id: Thread ID to assign to this thread
 * @pri: Priority level (0 = high priority, 1 = low priority)
 *
 * TODO: You need to implement this function to:
 *   1. Allocate a new TCB structure
 *   2. Allocate a stack for the new thread (use STACK_SIZE from t_lib.h)
 *   3. Allocate and initialize a ucontext_t structure
 *   4. Use getcontext() to initialize the context
 *   5. Set up the stack in the context (uc_stack.ss_sp, uc_stack.ss_size)
 *   6. Use makecontext() to set the start function
 *   7. Add the new TCB to the END of the appropriate ready queue based on priority
 *   8. Parent thread should continue execution after t_create() returns
 *
 * NOTE: Make this function atomic using sighold() and sigrelse() in Phase 2
 */
void t_create(void (*func)(int), int thr_id, int pri);

/*
 * t_terminate - Terminate the calling thread
 *
 * TODO: You need to implement this function to:
 *   1. Remove the calling thread's TCB from the running queue
 *   2. Free all resources associated with the thread:
 *      - Stack
 *      - Context
 *      - TCB structure
 *      - (Phase 4) Message queue and semaphores
 *   3. Resume execution of the next thread in the ready queue using setcontext()
 *   4. This function should NEVER return!
 *
 * IMPORTANT: Every thread MUST call t_terminate() before it finishes!
 *
 * NOTE: Make this function atomic using sighold() and sigrelse() in Phase 2
 */
void t_terminate(void);

/*
 * t_yield - Voluntarily yield the CPU to another thread
 *
 * TODO: You need to implement this function to:
 *   1. Move the calling thread from running queue to the END of its ready queue
 *   2. Select the first thread from the ready queue (check high priority first)
 *   3. Make that thread the new running thread
 *   4. Use swapcontext() to switch from current thread to the new thread
 *   5. (Phase 2) Cancel the current timer using ualarm(0, 0)
 *   6. (Phase 2) Start a new timer for the new thread
 *
 * NOTE: Make this function atomic using sighold() and sigrelse() in Phase 2
 */
void t_yield(void);


/* ========== Phase 3: Semaphore Operations ========== */

/*
 * Opaque semaphore type
 * The actual structure is defined in t_lib.h as sem_t_internal
 * Applications only see this as an opaque pointer
 */
typedef void sem_t;

/*
 * sem_init - Create and initialize a semaphore
 *
 * @sp: Pointer to semaphore pointer (will be allocated by this function)
 * @count: Initial count value for the semaphore
 *
 * Returns: 0 on success, -1 on failure
 *
 * TODO: You need to implement this function to:
 *   1. Allocate memory for a sem_t_internal structure
 *   2. Initialize count to the provided value
 *   3. Initialize the queue of blocked threads to NULL
 *   4. Set *sp to point to the new semaphore
 *   5. Return 0 on success, -1 if allocation fails
 */
int sem_init(sem_t **sp, unsigned int count);

/*
 * sem_wait - Wait (P operation) on a semaphore
 *
 * @sp: Semaphore to wait on
 *
 * TODO: You need to implement this function to:
 *   1. Decrement the semaphore count
 *   2. If count < 0:
 *      a. Block the current thread by removing it from running queue
 *      b. Add the thread to the semaphore's wait queue
 *      c. Schedule the next ready thread
 *   3. If count >= 0, continue execution
 *
 * NOTE: This function MUST be atomic! Use sighold() and sigrelse()
 */
void sem_wait(sem_t *sp);

/*
 * sem_signal - Signal (V operation) on a semaphore
 *
 * @sp: Semaphore to signal
 *
 * TODO: You need to implement this function to:
 *   1. Increment the semaphore count
 *   2. If count <= 0 (meaning there were blocked threads):
 *      a. Remove the first thread from the semaphore's wait queue
 *      b. Add it to the appropriate ready queue based on priority
 *   3. Signaling thread continues execution (Mesa semantics)
 *
 * NOTE: This function MUST be atomic! Use sighold() and sigrelse()
 */
void sem_signal(sem_t *sp);

/*
 * sem_destroy - Destroy a semaphore
 *
 * @sp: Pointer to semaphore pointer (will be freed by this function)
 *
 * TODO: You need to implement this function to:
 *   1. Move all threads from the semaphore's wait queue to ready queue
 *   2. Free the semaphore structure
 *   3. Set *sp to NULL
 *
 * NOTE: This function MUST be atomic! Use sighold() and sigrelse()
 */
void sem_destroy(sem_t **sp);


/* ========== Phase 4: Inter-Thread Communication ========== */

/*
 * Opaque mailbox type
 * The actual structure is defined in t_lib.h as mbox_internal
 */
typedef void mbox;

/*
 * mbox_create - Create a mailbox
 *
 * @mb: Pointer to mailbox pointer (will be allocated by this function)
 *
 * Returns: 0 on success, -1 on failure
 *
 * TODO: You need to implement this function to:
 *   1. Allocate memory for a mbox_internal structure
 *   2. Initialize the message queue to NULL
 *   3. Create a binary semaphore (count=1) to protect the mailbox
 *   4. Return 0 on success, -1 on failure
 */
int mbox_create(mbox **mb);

/*
 * mbox_destroy - Destroy a mailbox
 *
 * @mb: Pointer to mailbox pointer (will be freed by this function)
 *
 * TODO: You need to implement this function to:
 *   1. Free all messages in the mailbox's message queue
 *   2. Destroy the mailbox's semaphore
 *   3. Free the mailbox structure
 *   4. Set *mb to NULL
 */
void mbox_destroy(mbox **mb);

/*
 * mbox_deposit - Deposit a message into a mailbox (non-blocking)
 *
 * @mb: Mailbox to deposit into
 * @msg: Message string to deposit
 * @len: Length of the message in bytes
 *
 * TODO: You need to implement this function to:
 *   1. Acquire the mailbox lock (sem_wait on mbox_sem)
 *   2. Create a new messageNode
 *   3. Copy the message into the node
 *   4. Add the node to the END of the mailbox's message queue
 *   5. Release the mailbox lock (sem_signal on mbox_sem)
 *
 * NOTE: This is NON-BLOCKING - always returns immediately
 */
void mbox_deposit(mbox *mb, char *msg, int len);

/*
 * mbox_withdraw - Withdraw a message from a mailbox (non-blocking)
 *
 * @mb: Mailbox to withdraw from
 * @msg: Buffer to store the message (caller must allocate!)
 * @len: Pointer to store the message length
 *
 * TODO: You need to implement this function to:
 *   1. Acquire the mailbox lock
 *   2. If mailbox is empty:
 *      a. Set *len = 0
 *      b. Release lock and return
 *   3. If mailbox has messages:
 *      a. Remove the first message from the queue (FIFO order)
 *      b. Copy message to msg buffer
 *      c. Set *len to the message length
 *      d. Free the message node
 *      e. Release lock
 *
 * NOTE: This is NON-BLOCKING - returns immediately even if no message
 */
void mbox_withdraw(mbox *mb, char *msg, int *len);

/*
 * send - Send a message to a thread (asynchronous, non-blocking)
 *
 * @tid: Thread ID of the recipient
 * @msg: Message string to send
 * @len: Length of the message in bytes
 *
 * TODO: You need to implement this function to:
 *   1. Find the recipient thread's TCB (search all queues)
 *   2. Initialize the recipient's message queue and semaphores if needed
 *   3. Acquire the recipient's message queue lock
 *   4. Create a messageNode with the message
 *   5. Add to the END of recipient's message queue
 *   6. Release the lock
 *   7. Signal the recipient's message semaphore (to wake if waiting)
 *   8. Sender continues immediately (asynchronous!)
 */
void send(int tid, char *msg, int len);

/*
 * receive - Receive a message (blocking)
 *
 * @tid: Pointer to sender TID
 *       - Input: 0 means accept from ANY sender, X means accept only from sender X
 *       - Output: Will be set to the actual sender's TID
 * @msg: Buffer to store the message (caller must allocate!)
 * @len: Pointer to store the message length
 *
 * TODO: You need to implement this function to:
 *   1. Initialize message queue/semaphores for current thread if needed
 *   2. Loop until a matching message is found:
 *      a. Wait on the message semaphore (blocks if no messages)
 *      b. Acquire the message queue lock
 *      c. Search queue for matching message (check sender TID)
 *      d. If found:
 *         - Remove from queue
 *         - Copy to msg buffer
 *         - Set *tid and *len
 *         - Release lock and return
 *      e. If not found:
 *         - Release lock and continue loop
 *
 * NOTE: This is BLOCKING - will wait until a matching message arrives
 * NOTE: Messages are received in FIFO order
 */
void receive(int *tid, char *msg, int *len);

/*
 * block_send - Send a message and wait for reception (synchronous, blocking)
 *
 * EXTRA CREDIT - Optional implementation
 *
 * @tid: Thread ID of the recipient
 * @msg: Message string to send
 * @len: Length of the message in bytes
 *
 * TODO: Similar to send(), but:
 *   1. Create a semaphore in the message node (initialized to 0)
 *   2. After adding message to recipient's queue, wait on this semaphore
 *   3. Receiver must signal this semaphore when it receives the message
 *   4. Sender blocks until receiver gets the message (rendezvous)
 */
void block_send(int tid, char *msg, int len);

/*
 * block_receive - Receive a message from a specific sender (blocking)
 *
 * EXTRA CREDIT - Optional implementation
 *
 * @tid: Pointer to sender TID (must be non-zero for block_receive)
 * @msg: Buffer to store the message
 * @len: Pointer to store the message length
 *
 * TODO: Same as receive() but must specify sender (tid != 0)
 */
void block_receive(int *tid, char *msg, int *len);

#endif /* UD_THREAD_H */
