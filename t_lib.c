/*
 * t_lib.c
 *
 * User-level Thread Library Implementation
 *
 * TODO: Implement all the functions declared in ud_thread.h
 * Follow the assignment specification and hints in t_lib.h
 */

#include "t_lib.h"
#include "ud_thread.h"

/* ========== Global Variables ========== */

/*
 * The scheduler maintains a few global pieces of state.  The pointer
 * `running` refers to the currently executing thread.  When a thread
 * yields or terminates, it is removed from the running state and
 * placed onto an appropriate ready queue.  We maintain two ready
 * queues (one for high priority threads and one for low priority
 * threads) to implement the 2‑level queue scheduler in Phase 2.  The
 * `timer_enabled` flag indicates whether the preemption timer should
 * be used.  When zero, calls to start_timer() do nothing.  When
 * non‑zero, start_timer() arms the interval timer to deliver a
 * SIGALRM after TIME_QUANTUM microseconds.
 *
 * All pointers are initialized to NULL and timer_enabled is cleared
 * until t_init() sets it.  Because these are global variables, they
 * are declared here and defined exactly once.  They are referenced
 * from t_lib.h using the extern declarations.
 */
tcb *running = NULL;
tcb *ready_q_high = NULL;
tcb *ready_q_low = NULL;
int timer_enabled = 0;


/* ========== Helper Functions: Queue Management ========== */

/*
 * add_to_queue - Add a TCB to the end of a queue
 *
 * TODO: Implement this function
 *
 * ALGORITHM:
 *   1. Check if node is NULL (safety check)
 *   2. Set node->next = NULL (it will be the last node)
 *   3. If queue is empty (*queue == NULL):
 *      - Set *queue = node (node becomes the head)
 *   4. Else (queue has nodes):
 *      - Traverse to the end: current = *queue; while (current->next != NULL) ...
 *      - Add node at end: current->next = node
 */
void add_to_queue(tcb **queue, tcb *node) {
    /*
     * Append the given node to the end of the list pointed to by
     * `queue`.  If the queue is empty, the node becomes the head.
     * Always terminate the node's next pointer to avoid accidental
     * linkage into other lists.  A NULL node is ignored because
     * there's nothing to enqueue.
     */
    if (node == NULL || queue == NULL) {
        return;
    }
    node->next = NULL;
    if (*queue == NULL) {
        /* First element in the queue */
        *queue = node;
    } else {
        /* Traverse to the last element and append */
        tcb *cur = *queue;
        while (cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = node;
    }
}

/*
 * remove_from_queue - Remove and return the first TCB from a queue
 *
 * TODO: Implement this function
 *
 * ALGORITHM:
 *   1. If queue is empty (*queue == NULL):
 *      - Return NULL
 *   2. Save the first node: node = *queue
 *   3. Update queue head: *queue = node->next
 *   4. Disconnect node: node->next = NULL
 *   5. Return node
 */
tcb *remove_from_queue(tcb **queue) {
    /*
     * Remove the first element from the queue and return it.  If the
     * queue is empty or the pointer itself is NULL, return NULL.
     */
    if (queue == NULL || *queue == NULL) {
        return NULL;
    }
    tcb *node = *queue;
    *queue = node->next;
    node->next = NULL;
    return node;
}

/*
 * get_tcb_by_id - Find a thread by its ID (needed for Phase 4)
 *
 * TODO: Implement this function for Phase 4
 *
 * ALGORITHM:
 *   1. Check if running thread matches the ID
 *   2. Search through ready_q_high
 *   3. Search through ready_q_low
 *   4. Return TCB if found, NULL otherwise
 */
tcb *get_tcb_by_id(int tid) {
    /*
     * Find a thread's TCB given its thread ID.  For phases 1–3 we do
     * not need message passing, so simply return NULL.  A complete
     * implementation for Phase 4 would need to search the running
     * thread and both ready queues.  Returning NULL here avoids
     * unused parameter warnings and lets the code compile without
     * implementing optional features.
     */
    (void)tid;
    return NULL;
}


/* ========== Helper Functions: Timer Management (Phase 2) ========== */

/*
 * start_timer - Start the interval timer for time slicing
 *
 * TODO: Implement this function in Phase 2
 *
 * ALGORITHM:
 *   1. Check if timer_enabled is true
 *   2. Call ualarm(TIME_QUANTUM, 0) to schedule SIGALRM
 *
 * HINT: ualarm(microseconds, interval) - use interval=0 for single-shot
 */
void start_timer(void) {
    /*
     * Arm the interval timer to deliver a SIGALRM after TIME_QUANTUM
     * microseconds.  Only do this when timer interrupts are enabled.
     * The call to ualarm schedules a single alarm (interval of zero).
     */
    if (timer_enabled) {
        /* Set an interval timer for the next time slice */
        ualarm(TIME_QUANTUM, 0);
    }
}

/*
 * stop_timer - Cancel the interval timer
 *
 * TODO: Implement this function in Phase 2
 *
 * ALGORITHM:
 *   1. Call ualarm(0, 0) to cancel any pending alarm
 */
void stop_timer(void) {
    /* Cancel any pending alarm.  Passing zero values to ualarm
     * disables the timer entirely. */
    ualarm(0, 0);
}


/* ========== Scheduler (Phase 2) ========== */

/*
 * schedule - Context switch to the next ready thread
 *
 * TODO: Implement this function
 *
 * PHASE 1 VERSION (single queue):
 *   1. Get next thread from ready queue
 *   2. If no threads, return
 *   3. Make it the running thread
 *   4. Use setcontext() or swapcontext() to switch
 *
 * PHASE 2 VERSION (2-level queue):
 *   1. Try to get thread from ready_q_high first
 *   2. If empty, try ready_q_low
 *   3. If both empty, return
 *   4. Save prev_thread = running
 *   5. Set running = next_thread
 *   6. Start timer for new thread
 *   7. If prev_thread is NULL: use setcontext()
 *   8. Else: use swapcontext(prev_thread->context, running->context)
 */
void schedule(void) {
    /*
     * Select the next thread to run.  High priority threads are
     * selected before low priority threads.  If no threads are ready
     * we simply return and allow the current thread (if any) to
     * continue.  Otherwise we context switch to the chosen thread
     * using swapcontext() if there was a previous running thread or
     * setcontext() if this is the initial dispatch.
     */
    /* Choose the next thread from high priority queue first */
    tcb *next = remove_from_queue(&ready_q_high);
    if (next == NULL) {
        /* No high priority thread, fall back to low priority */
        next = remove_from_queue(&ready_q_low);
    }
    if (next == NULL) {
        /* Nothing to schedule */
        return;
    }

    /* Save the pointer to the currently running thread.  It may be
     * NULL if schedule() is called from t_init() before any thread
     * has run or after the last thread has terminated. */
    tcb *prev = running;
    running = next;

    /* Start the timer for the newly scheduled thread */
    start_timer();

    if (prev == NULL) {
        /* No previously running thread, so just restore the new
         * thread's context.  setcontext() does not return. */
        setcontext(running->thread_context);
        /* Should never reach here */
    } else {
        /* Save the current context in prev->thread_context and switch
         * to the new one atomically.  swapcontext returns to this
         * point when the new thread yields or terminates. */
        swapcontext(prev->thread_context, running->thread_context);
    }
}

/*
 * timer_handler - Signal handler for SIGALRM (Phase 2)
 *
 * TODO: Implement this function in Phase 2
 *
 * ALGORITHM:
 *   1. Check if running thread exists
 *   2. Move running thread to appropriate ready queue (based on priority)
 *   3. Set running = NULL
 *   4. Call schedule() to switch to next thread
 *
 * NOTE: This function is called automatically when SIGALRM is raised
 */
void timer_handler(int sig) {
    /*
     * Timer interrupt handler: preempt the currently running thread
     * when its time quantum expires.  Move the running thread to the
     * end of its appropriate ready queue and invoke the scheduler to
     * select the next thread to run.  Because this handler is
     * executed in signal context, we must be careful to only perform
     * minimal operations here.  The global variables and queues are
     * protected by the signal mask inherited from ualarm, so there is
     * no explicit sighold/sigrelse in the handler.
     */
    (void)sig; /* Unused parameter */
    if (running == NULL) {
        return;
    }
    /* Move the currently running thread to its ready queue */
    tcb *preempted = running;
    running = NULL;
    if (preempted->thread_priority == HIGH_PRIORITY) {
        add_to_queue(&ready_q_high, preempted);
    } else {
        add_to_queue(&ready_q_low, preempted);
    }
    /* Context switch to next ready thread */
    schedule();
}


/* ========== Phase 1: Basic Thread Operations ========== */

/*
 * t_init - Initialize the thread library
 *
 * TODO: Implement this function
 *
 * ALGORITHM:
 *   1. Allocate TCB for main thread using malloc()
 *   2. Set thread_id = 0, thread_priority = LOW_PRIORITY (1)
 *   3. Allocate ucontext_t for main thread
 *   4. Call getcontext() to save current context
 *   5. Set stack = NULL (main uses default stack)
 *   6. Initialize message passing fields to NULL (Phase 4)
 *   7. Set running = main_thread
 *   8. Initialize ready queues to NULL
 *   9. (Phase 2) Set up signal handler: signal(SIGALRM, timer_handler)
 *   10. (Phase 2) Set timer_enabled = 1 and start_timer()
 *
 * ERROR HANDLING:
 *   - Check if malloc() returns NULL
 *   - Check if getcontext() returns -1
 */
void t_init(void) {
    /*
     * Initialize the user‑level thread library.  This sets up the
     * initial running thread (the main thread) and clears the ready
     * queues.  The main thread receives a thread ID of 0 and a low
     * priority (1).  We allocate a TCB and context structure for the
     * main thread so that it can participate in context switches.  We
     * also install the timer handler and start the interval timer.
     */
    /* Allocate and initialize the main thread's TCB */
    tcb *main_thread = (tcb *)malloc(sizeof(tcb));
    if (main_thread == NULL) {
        perror("t_init: malloc failed for main TCB");
        exit(1);
    }
    main_thread->thread_id = 0;
    main_thread->thread_priority = LOW_PRIORITY;
    main_thread->stack = NULL;             /* Main uses existing stack */
    main_thread->next = NULL;
    main_thread->msg_sem = NULL;
    main_thread->msg_queue = NULL;
    main_thread->mbox_lock = NULL;

    /* Allocate a ucontext_t structure for the main thread */
    main_thread->thread_context = (ucontext_t *)malloc(sizeof(ucontext_t));
    if (main_thread->thread_context == NULL) {
        perror("t_init: malloc failed for main context");
        free(main_thread);
        exit(1);
    }
    /* Save the current execution context into main_thread->thread_context */
    if (getcontext(main_thread->thread_context) == -1) {
        perror("t_init: getcontext failed");
        free(main_thread->thread_context);
        free(main_thread);
        exit(1);
    }

    /* Initialize global scheduler state */
    running = main_thread;
    ready_q_high = NULL;
    ready_q_low = NULL;

    /* Install the timer handler and enable the timer.  We register
     * timer_handler for SIGALRM and enable timer interrupts.  If
     * signal() fails, we abort because correct operation depends on
     * handling SIGALRM. */
    if (signal(SIGALRM, timer_handler) == SIG_ERR) {
        perror("t_init: failed to register timer handler");
        /* Continue anyway but preemption will not work */
    }
    timer_enabled = 1;
    /* Start the first time slice for the main thread */
    start_timer();
}

/*
 * t_create - Create a new thread
 *
 * TODO: Implement this function
 *
 * ALGORITHM:
 *   1. (Phase 2) Disable interrupts: sighold(SIGALRM)
 *   2. Allocate new TCB using malloc()
 *   3. Set thread_id = thr_id, thread_priority = pri
 *   4. Allocate ucontext_t using malloc()
 *   5. Call getcontext() to initialize context
 *   6. Allocate stack: malloc(STACK_SIZE)
 *   7. Set up context stack:
 *      - context->uc_stack.ss_sp = stack
 *      - context->uc_stack.ss_size = STACK_SIZE
 *      - context->uc_stack.ss_flags = 0
 *      - context->uc_link = NULL
 *   8. Call makecontext(context, (void(*)())func, 1, thr_id)
 *   9. Initialize message passing fields to NULL (Phase 4)
 *   10. Add TCB to appropriate ready queue based on priority:
 *       - If pri == HIGH_PRIORITY (0): add to ready_q_high
 *       - If pri == LOW_PRIORITY (1): add to ready_q_low
 *   11. (Phase 2) Re-enable interrupts: sigrelse(SIGALRM)
 *
 * ERROR HANDLING:
 *   - Check all malloc() calls for NULL
 *   - Check getcontext() for -1
 */
void t_create(void (*func)(int), int thr_id, int pri) {
    /*
     * Create a new thread with the given start function, ID and
     * priority.  Threads are created in a blocked state and placed on
     * the appropriate ready queue; the parent thread continues
     * execution immediately.  All dynamic allocations are checked
     * and cleaned up on failure to avoid leaks.  In Phase 2 we
     * temporarily block SIGALRM to ensure that the ready queues are
     * modified atomically.  Since preemption is disabled while
     * signals are held, we do not stop the timer explicitly here.
     */
    /* Block SIGALRM while manipulating global queues */
    sighold(SIGALRM);

    /* Allocate a new TCB */
    tcb *new_thread = (tcb *)malloc(sizeof(tcb));
    if (new_thread == NULL) {
        perror("t_create: malloc failed for TCB");
        sigrelse(SIGALRM);
        return;
    }
    new_thread->thread_id = thr_id;
    new_thread->thread_priority = pri;
    new_thread->next = NULL;
    new_thread->msg_sem = NULL;
    new_thread->msg_queue = NULL;
    new_thread->mbox_lock = NULL;

    /* Allocate a ucontext_t structure */
    new_thread->thread_context = (ucontext_t *)malloc(sizeof(ucontext_t));
    if (new_thread->thread_context == NULL) {
        perror("t_create: malloc failed for context");
        free(new_thread);
        sigrelse(SIGALRM);
        return;
    }
    /* Initialize the context */
    if (getcontext(new_thread->thread_context) == -1) {
        perror("t_create: getcontext failed");
        free(new_thread->thread_context);
        free(new_thread);
        sigrelse(SIGALRM);
        return;
    }
    /* Allocate the stack for the new thread */
    void *stack = malloc(STACK_SIZE);
    if (stack == NULL) {
        perror("t_create: malloc failed for stack");
        free(new_thread->thread_context);
        free(new_thread);
        sigrelse(SIGALRM);
        return;
    }
    /* Set up the context's stack */
    new_thread->thread_context->uc_stack.ss_sp = stack;
    new_thread->thread_context->uc_stack.ss_size = STACK_SIZE;
    new_thread->thread_context->uc_stack.ss_flags = 0;
    new_thread->thread_context->uc_link = NULL;
    /* Save pointer to stack so we can free later */
    new_thread->stack = stack;
    /* Create the new context to execute the function */
    makecontext(new_thread->thread_context, (void (*)())func, 1, thr_id);

    /* Add the new thread to the end of the appropriate ready queue */
    if (pri == HIGH_PRIORITY) {
        add_to_queue(&ready_q_high, new_thread);
    } else {
        add_to_queue(&ready_q_low, new_thread);
    }

    /* Re‑enable SIGALRM */
    sigrelse(SIGALRM);
}

/*
 * t_yield - Voluntarily yield the CPU
 *
 * TODO: Implement this function
 *
 * ALGORITHM:
 *   1. (Phase 2) Disable interrupts: sighold(SIGALRM)
 *   2. (Phase 2) Cancel timer: stop_timer()
 *   3. Check if running thread exists
 *   4. Check if there are threads to yield to (any ready queue non-empty)
 *   5. If no threads to yield to:
 *      - (Phase 2) Restart timer and return
 *   6. Move running thread to end of its ready queue (based on priority)
 *   7. Set running = NULL
 *   8. Call schedule() to switch to next thread
 *   9. (Phase 2) Re-enable interrupts when we resume: sigrelse(SIGALRM)
 *
 * NOTE: In Phase 1, you can simplify without sighold/sigrelse and timer
 */
void t_yield(void) {
    /*
     * Voluntarily yield the CPU to another ready thread.  We move
     * the currently running thread to the end of its ready queue and
     * schedule the next thread.  If no other threads are ready, we
     * simply restart the timer and return.  Operations that modify
     * shared data structures are protected by blocking SIGALRM.
     */
    /* If there is no running thread, nothing to yield */
    if (running == NULL) {
        return;
    }

    /* Disable timer interrupts and cancel the current timer */
    sighold(SIGALRM);
    stop_timer();

    /* Check if any thread is ready to run */
    if (ready_q_high == NULL && ready_q_low == NULL) {
        /* No other threads; restart timer for current thread and return */
        start_timer();
        sigrelse(SIGALRM);
        return;
    }

    /* Move the running thread to the end of its appropriate ready queue */
    tcb *cur = running;
    running = NULL;
    if (cur->thread_priority == HIGH_PRIORITY) {
        add_to_queue(&ready_q_high, cur);
    } else {
        add_to_queue(&ready_q_low, cur);
    }

    /* Schedule the next thread.  When this call returns via
     * swapcontext(), this thread will resume execution just after
     * schedule() and we will re‑enable interrupts below. */
    schedule();

    /* Upon returning from swapcontext() (thread resumed) we
     * re‑enable SIGALRM.  The timer should already have been
     * restarted by the scheduler for the resumed thread. */
    sigrelse(SIGALRM);
}

/*
 * t_terminate - Terminate the calling thread
 *
 * TODO: Implement this function
 *
 * ALGORITHM:
 *   1. (Phase 2) Disable interrupts: sighold(SIGALRM)
 *   2. (Phase 2) Stop timer: stop_timer()
 *   3. Save pointer to terminating thread
 *   4. Set running = NULL
 *   5. Free thread resources:
 *      - Free stack (if not NULL)
 *      - Free message queue (Phase 4)
 *      - Free semaphores (Phase 4)
 *      - Free context
 *      - Free TCB
 *   6. Call schedule() to run next thread
 *   7. (Phase 2) Re-enable interrupts: sigrelse(SIGALRM)
 *
 * NOTE: This function should never return!
 * The schedule() call will context switch to another thread.
 */
void t_terminate(void) {
    /*
     * Terminate the calling thread.  Free all resources associated
     * with the thread, remove it from the running state and dispatch
     * another thread.  The function does not return; control
     * transfers to the next scheduled thread.  Interrupts are
     * disabled during manipulation of shared data.
     */
    if (running == NULL) {
        /* Nothing to terminate */
        return;
    }
    /* Block SIGALRM and stop the timer */
    sighold(SIGALRM);
    stop_timer();

    /* Save pointer to current thread so we can free it */
    tcb *to_free = running;
    running = NULL;

    /* Free the thread's stack (NULL for main thread) */
    if (to_free->stack != NULL) {
        free(to_free->stack);
        to_free->stack = NULL;
    }
    /* Free the ucontext structure */
    if (to_free->thread_context != NULL) {
        free(to_free->thread_context);
        to_free->thread_context = NULL;
    }
    /* Free any message passing structures (not used in phases 1–3) */
    /* Free the TCB itself */
    free(to_free);

    /* Re‑enable SIGALRM before switching to the next thread.  We
     * release the signal here because schedule() may call
     * setcontext(), which never returns; releasing afterwards would
     * never be executed. */
    sigrelse(SIGALRM);

    /* Dispatch the next ready thread.  schedule() will call
     * setcontext() and never return. */
    schedule();

    /* Should never reach here */
}

/*
 * t_shutdown - Shutdown the thread library
 *
 * TODO: Implement this function
 *
 * ALGORITHM:
 *   1. (Phase 2) Disable interrupts and stop timer
 *   2. Free the running thread (if exists)
 *   3. Free all threads in ready_q_high
 *   4. Free all threads in ready_q_low
 *   5. For each thread, free:
 *      - Stack
 *      - Message queue and all messages (Phase 4)
 *      - Semaphores (Phase 4)
 *      - Context
 *      - TCB
 *
 * HINT: Use a loop like:
 *   while (ready_q_high != NULL) {
 *       tcb *thread = remove_from_queue(&ready_q_high);
 *       // free thread resources...
 *   }
 */
void t_shutdown(void) {
    /*
     * Clean up all resources allocated by the thread library.  This
     * function disables the timer, frees the running thread and
     * empties both ready queues.  It also ensures that any pending
     * waiting threads on semaphores are not leaked (though message
     * passing is unimplemented here).  After shutdown, the scheduler
     * state is reset to initial conditions.
     */
    /* Disable interrupts and the timer */
    sighold(SIGALRM);
    stop_timer();
    timer_enabled = 0;

    /* Free the currently running thread (usually the main thread) */
    if (running != NULL) {
        if (running->stack != NULL) {
            free(running->stack);
            running->stack = NULL;
        }
        if (running->thread_context != NULL) {
            free(running->thread_context);
            running->thread_context = NULL;
        }
        free(running);
        running = NULL;
    }

    /* Free all threads in the high priority ready queue */
    while (ready_q_high != NULL) {
        tcb *thr = remove_from_queue(&ready_q_high);
        if (thr != NULL) {
            if (thr->stack != NULL) {
                free(thr->stack);
                thr->stack = NULL;
            }
            if (thr->thread_context != NULL) {
                free(thr->thread_context);
                thr->thread_context = NULL;
            }
            free(thr);
        }
    }

    /* Free all threads in the low priority ready queue */
    while (ready_q_low != NULL) {
        tcb *thr = remove_from_queue(&ready_q_low);
        if (thr != NULL) {
            if (thr->stack != NULL) {
                free(thr->stack);
                thr->stack = NULL;
            }
            if (thr->thread_context != NULL) {
                free(thr->thread_context);
                thr->thread_context = NULL;
            }
            free(thr);
        }
    }

    /* Release SIGALRM */
    sigrelse(SIGALRM);
}


/* ========== Phase 3: Semaphore Operations ========== */

/*
 * sem_init - Create and initialize a semaphore
 *
 * TODO: Implement this function in Phase 3
 *
 * ALGORITHM:
 *   1. Allocate sem_t_internal structure using malloc()
 *   2. Set count = count parameter
 *   3. Set q = NULL (no blocked threads initially)
 *   4. Set *sp = (sem_t *)sem
 *   5. Return 0 on success, -1 on malloc failure
 */
int sem_init(sem_t **sp, unsigned int count) {
    /*
     * Allocate and initialize a counting semaphore.  The external
     * interface presents sem_t as an opaque pointer, but internally
     * we use the sem_t_internal structure defined in t_lib.h.  On
     * success we return 0 and store the allocated semaphore in *sp;
     * on failure we return -1 and leave *sp unchanged.  No need to
     * disable interrupts here since this allocation is thread local.
     */
    if (sp == NULL) {
        return -1;
    }
    sem_t_internal *sem = (sem_t_internal *)malloc(sizeof(sem_t_internal));
    if (sem == NULL) {
        return -1;
    }
    sem->count = (int)count;
    sem->q = NULL;
    *sp = (sem_t *)sem;
    return 0;
}

/*
 * sem_wait - Wait (P operation) on semaphore
 *
 * TODO: Implement this function in Phase 3
 *
 * ALGORITHM:
 *   1. Disable interrupts: sighold(SIGALRM)
 *   2. Decrement count: sem->count--
 *   3. If count < 0:
 *      a. Block current thread:
 *         - Remove from running (set running = NULL)
 *         - Add to semaphore's wait queue (sem->q)
 *      b. Call schedule() to run next thread
 *   4. Re-enable interrupts: sigrelse(SIGALRM)
 *
 * NOTE: Must be atomic!
 */
void sem_wait(sem_t *sp) {
    /*
     * Decrement the semaphore and block the current thread if the
     * resulting count is negative.  The wait operation must be
     * atomic with respect to timer interrupts.  We use sighold to
     * block SIGALRM while manipulating the semaphore and the ready
     * queues.  If the thread blocks, we call schedule() to run the
     * next ready thread.  When schedule returns (when this thread is
     * unblocked), we re‑enable interrupts.
     */
    if (sp == NULL) {
        return;
    }
    sem_t_internal *sem = (sem_t_internal *)sp;
    /* Disable SIGALRM */
    sighold(SIGALRM);

    /* Decrement the count */
    sem->count--;
    if (sem->count < 0) {
        /* Block the running thread */
        tcb *cur = running;
        running = NULL;
        /* Add to the semaphore's wait queue */
        add_to_queue(&sem->q, cur);
        /* Schedule the next ready thread.  When this call returns,
         * this thread has been unblocked and the signal mask will
         * still be held. */
        schedule();
    }
    /* Re‑enable SIGALRM */
    sigrelse(SIGALRM);
}

/*
 * sem_signal - Signal (V operation) on semaphore
 *
 * TODO: Implement this function in Phase 3
 *
 * ALGORITHM:
 *   1. Disable interrupts: sighold(SIGALRM)
 *   2. Increment count: sem->count++
 *   3. If count <= 0 (there were blocked threads):
 *      a. Remove first thread from semaphore's wait queue
 *      b. Add it to appropriate ready queue (based on priority)
 *   4. Signaling thread continues (Mesa semantics)
 *   5. Re-enable interrupts: sigrelse(SIGALRM)
 *
 * NOTE: Must be atomic!
 */
void sem_signal(sem_t *sp) {
    /*
     * Increment the semaphore count and, if necessary, wake a
     * waiting thread.  The signal operation must be atomic.  When a
     * thread is woken, it is placed onto the end of its ready queue
     * based on its priority.  The signaling thread continues
     * execution (Mesa semantics).  We hold SIGALRM during the
     * operation to avoid race conditions with the timer.
     */
    if (sp == NULL) {
        return;
    }
    sem_t_internal *sem = (sem_t_internal *)sp;
    sighold(SIGALRM);
    sem->count++;
    if (sem->count <= 0) {
        /* Unblock one thread from the semaphore queue */
        tcb *thr = remove_from_queue(&sem->q);
        if (thr != NULL) {
            /* Add to appropriate ready queue based on priority */
            if (thr->thread_priority == HIGH_PRIORITY) {
                add_to_queue(&ready_q_high, thr);
            } else {
                add_to_queue(&ready_q_low, thr);
            }
        }
    }
    sigrelse(SIGALRM);
}

/*
 * sem_destroy - Destroy a semaphore
 *
 * TODO: Implement this function in Phase 3
 *
 * ALGORITHM:
 *   1. Disable interrupts: sighold(SIGALRM)
 *   2. While semaphore's wait queue is not empty:
 *      a. Remove thread from wait queue
 *      b. Add to appropriate ready queue
 *   3. Free the semaphore structure
 *   4. Set *sp = NULL
 *   5. Re-enable interrupts: sigrelse(SIGALRM)
 */
void sem_destroy(sem_t **sp) {
    /*
     * Destroy a semaphore by waking all waiting threads and freeing
     * the semaphore structure.  Threads removed from the semaphore
     * queue are placed on the end of their ready queue.  SIGALRM is
     * held to ensure atomicity.  After freeing, set the pointer to
     * NULL.
     */
    if (sp == NULL || *sp == NULL) {
        return;
    }
    sem_t_internal *sem = (sem_t_internal *)(*sp);
    sighold(SIGALRM);
    /* Move all waiting threads to ready queues */
    tcb *thr;
    while ((thr = remove_from_queue(&sem->q)) != NULL) {
        if (thr->thread_priority == HIGH_PRIORITY) {
            add_to_queue(&ready_q_high, thr);
        } else {
            add_to_queue(&ready_q_low, thr);
        }
    }
    /* Free the semaphore */
    free(sem);
    *sp = NULL;
    sigrelse(SIGALRM);
}


/* ========== Phase 4: Mailbox Operations ========== */

/*
 * mbox_create - Create a mailbox
 *
 * TODO: Implement this function in Phase 4
 *
 * ALGORITHM:
 *   1. Allocate mbox_internal structure
 *   2. Set msg = NULL (no messages initially)
 *   3. Create binary semaphore for lock: sem_init(&mbox_sem, 1)
 *   4. Set *mb = mailbox
 *   5. Return 0 on success, -1 on failure
 */
int mbox_create(mbox **mb) {
    // TODO: Implement this function in Phase 4
    (void)mb;  // Remove when implementing
    return -1;  // Replace this
}

/*
 * mbox_destroy - Destroy a mailbox
 *
 * TODO: Implement this function in Phase 4
 *
 * ALGORITHM:
 *   1. Free all messages in the message queue
 *   2. Destroy the semaphore
 *   3. Free the mailbox structure
 *   4. Set *mb = NULL
 */
void mbox_destroy(mbox **mb) {
    // TODO: Implement this function in Phase 4
    (void)mb;  // Remove when implementing
}

/*
 * mbox_deposit - Deposit a message (non-blocking)
 *
 * TODO: Implement this function in Phase 4
 *
 * ALGORITHM:
 *   1. Lock mailbox: sem_wait(mailbox->mbox_sem)
 *   2. Allocate messageNode
 *   3. Allocate and copy message string (length + 1 for null terminator)
 *   4. Set message fields (len, sender, receiver)
 *   5. Add to end of mailbox message queue
 *   6. Unlock mailbox: sem_signal(mailbox->mbox_sem)
 */
void mbox_deposit(mbox *mb, char *msg, int len) {
    // TODO: Implement this function in Phase 4
    (void)mb; (void)msg; (void)len;  // Remove when implementing
}

/*
 * mbox_withdraw - Withdraw a message (non-blocking)
 *
 * TODO: Implement this function in Phase 4
 *
 * ALGORITHM:
 *   1. Lock mailbox: sem_wait(mailbox->mbox_sem)
 *   2. If mailbox is empty:
 *      a. Set *len = 0
 *      b. Unlock and return
 *   3. If mailbox has messages:
 *      a. Remove first message from queue
 *      b. Copy message to msg buffer
 *      c. Set *len
 *      d. Free message node
 *   4. Unlock mailbox: sem_signal(mailbox->mbox_sem)
 */
void mbox_withdraw(mbox *mb, char *msg, int *len) {
    // TODO: Implement this function in Phase 4
    (void)mb; (void)msg; (void)len;  // Remove when implementing
}


/* ========== Phase 4: Message Passing ========== */

/*
 * send - Send a message (asynchronous)
 *
 * TODO: Implement this function in Phase 4
 *
 * ALGORITHM:
 *   1. Disable interrupts: sighold(SIGALRM)
 *   2. Find recipient thread using get_tcb_by_id(tid)
 *   3. If recipient not found, return
 *   4. Initialize recipient's msg_sem if NULL: sem_init(&msg_sem, 0)
 *   5. Initialize recipient's mbox_lock if NULL: sem_init(&mbox_lock, 1)
 *   6. Lock recipient's message queue: sem_wait(mbox_lock)
 *   7. Create messageNode and copy message
 *   8. Add to end of recipient->msg_queue
 *   9. Unlock: sem_signal(mbox_lock)
 *   10. Signal recipient: sem_signal(msg_sem)
 *   11. Re-enable interrupts: sigrelse(SIGALRM)
 *   12. Sender continues (asynchronous!)
 */
void send(int tid, char *msg, int len) {
    // TODO: Implement this function in Phase 4
    (void)tid; (void)msg; (void)len;  // Remove when implementing
}

/*
 * receive - Receive a message (blocking)
 *
 * TODO: Implement this function in Phase 4
 *
 * ALGORITHM:
 *   1. Initialize running thread's msg_sem if NULL
 *   2. Initialize running thread's mbox_lock if NULL
 *   3. Loop until matching message found:
 *      a. Wait for message: sem_wait(msg_sem) - BLOCKS if no messages
 *      b. Lock queue: sem_wait(mbox_lock)
 *      c. Search msg_queue for matching message:
 *         - If *tid == 0: accept any sender (first message)
 *         - If *tid != 0: accept only if message->sender == *tid
 *      d. If found:
 *         - Remove from queue
 *         - Copy to msg buffer
 *         - Set *tid = message->sender, *len = message->len
 *         - (Extra credit) Signal send_sem if exists
 *         - Free message node
 *         - Unlock and return
 *      e. If not found:
 *         - Unlock and continue loop
 */
void receive(int *tid, char *msg, int *len) {
    // TODO: Implement this function in Phase 4
    (void)tid; (void)msg; (void)len;  // Remove when implementing
}

/*
 * block_send - Synchronous send (EXTRA CREDIT)
 *
 * TODO: Implement this function for extra credit
 *
 * ALGORITHM: Similar to send(), but:
 *   1. Create semaphore in message node: sem_init(&send_sem, 0)
 *   2. After adding message and signaling recipient
 *   3. Wait for reception: sem_wait(send_sem)
 *   4. This blocks sender until receiver calls receive()
 */
void block_send(int tid, char *msg, int len) {
    // TODO: Implement this function for extra credit
    (void)tid; (void)msg; (void)len;  // Remove when implementing
}

/*
 * block_receive - Receive from specific sender (EXTRA CREDIT)
 *
 * TODO: Implement this function for extra credit
 *
 * ALGORITHM: Same as receive() but tid must be non-zero
 */
void block_receive(int *tid, char *msg, int *len) {
    // TODO: Implement this function for extra credit
    (void)tid; (void)msg; (void)len;  // Remove when implementing
}
