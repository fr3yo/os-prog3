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
 * threads) to implement the 2-level queue scheduler in Phase 2.  The
 * `timer_enabled` flag indicates whether the preemption timer should
 * be used.  When zero, calls to start_timer() do nothing.  When
 * non-zero, start_timer() arms the interval timer to deliver a
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

/*
 * We maintain a list of all stacks allocated for threads.  When a
 * thread is created its stack pointer is recorded on this list via
 * record_stack().  Rather than freeing a thread’s stack while it is
 * still running (which would lead to use-after-free errors), we defer
 * freeing the memory until t_shutdown() is called.  At shutdown the
 * free_all_stacks() helper walks the list and frees every stack.
 */
typedef struct stack_node {
    void *stack;
    struct stack_node *next;
} stack_node;

static stack_node *stack_list = NULL;

/* Record a newly allocated stack so it can be freed later. */
static void record_stack(void *stack) {
    stack_node *node = (stack_node *)malloc(sizeof(stack_node));
    if (node != NULL) {
        node->stack = stack;
        node->next = stack_list;
        stack_list = node;
    }
}

/* Free all recorded stacks and reset the list. */
static void free_all_stacks(void) {
    stack_node *node = stack_list;
    while (node != NULL) {
        stack_node *next = node->next;
        free(node->stack);
        free(node);
        node = next;
    }
    stack_list = NULL;
}


/* ========== Helper Functions: Queue Management ========== */

/*
 * add_to_queue - Add a TCB to the end of a queue
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
 * For phases 1–3 we don't actually need this, so it's left as a stub.
 */
tcb *get_tcb_by_id(int tid) {
    (void)tid;
    return NULL;
}


/* ========== Helper Functions: Timer Management (Phase 2) ========== */

void start_timer(void) {
    /*
     * Arm the interval timer to deliver a SIGALRM after TIME_QUANTUM
     * microseconds.  Only do this when timer interrupts are enabled.
     */
    if (timer_enabled) {
        ualarm(TIME_QUANTUM, 0);
    }
}

void stop_timer(void) {
    /* Cancel any pending alarm. */
    ualarm(0, 0);
}


/* ========== Scheduler (Phase 2) ========== */

/*
 * schedule - Context switch to the next ready thread
 *
 * 2-level queue:
 *   1. Try ready_q_high
 *   2. Fall back to ready_q_low
 *   3. If both empty, return
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

    /* Save the pointer to the currently running thread before switching. */
    tcb *prev = running;
    running = next;

    /* Start the timer for the newly scheduled thread. */
    start_timer();

    if (prev == NULL) {
        /* First ever dispatch or previous thread gone. */
        setcontext(running->thread_context);
        /* setcontext does not return on success */
    } else {
        /* Save the current context and switch to the new one. swapcontext() will
         * return here when the new thread yields or is preempted. */
        swapcontext(prev->thread_context, running->thread_context);
        /* When we resume here, the previous thread has been scheduled again.
         * Restore the running pointer to reflect this thread and restart its timer. */
        running = prev;
        /* Rearm the timer for the resumed thread. */
        start_timer();
    }
}

/*
 * timer_handler - Signal handler for SIGALRM (Phase 2)
 *
 * Preempt current thread and run someone else.
 */
void timer_handler(int sig) {
    (void)sig;  /* Unused */

    if (running == NULL) {
        return;
    }

    /* Move the currently running thread to its ready queue.  Do not
     * clear the running pointer here; schedule() needs to know the
     * previous thread so it can save its context with swapcontext(). */
    tcb *preempted = running;
    if (preempted->thread_priority == HIGH_PRIORITY) {
        add_to_queue(&ready_q_high, preempted);
    } else {
        add_to_queue(&ready_q_low, preempted);
    }

    /* Dispatch another thread.  When schedule() returns here we will
     * resume the preempted thread after its time slice expires. */
    schedule();
}


/* ========== Phase 1: Basic Thread Operations ========== */

void t_init(void) {
    /*
     * Initialize the user-level thread library.  This sets up the
     * initial running thread (the main thread) and clears the ready
     * queues.  The main thread receives a thread ID of 0 and a low
     * priority (1).
     */
    tcb *main_thread = (tcb *)malloc(sizeof(tcb));
    if (main_thread == NULL) {
        perror("t_init: malloc failed for main TCB");
        exit(1);
    }
    main_thread->thread_id = 0;
    main_thread->thread_priority = LOW_PRIORITY;
    main_thread->stack = NULL;             /* main uses existing stack */
    main_thread->next = NULL;
    main_thread->msg_sem = NULL;
    main_thread->msg_queue = NULL;
    main_thread->mbox_lock = NULL;

    main_thread->thread_context = (ucontext_t *)malloc(sizeof(ucontext_t));
    if (main_thread->thread_context == NULL) {
        perror("t_init: malloc failed for main context");
        free(main_thread);
        exit(1);
    }
    if (getcontext(main_thread->thread_context) == -1) {
        perror("t_init: getcontext failed");
        free(main_thread->thread_context);
        free(main_thread);
        exit(1);
    }

    running = main_thread;
    ready_q_high = NULL;
    ready_q_low = NULL;

    if (signal(SIGALRM, timer_handler) == SIG_ERR) {
        perror("t_init: failed to register timer handler");
        /* if this fails, preemption just won't work right */
    }
    timer_enabled = 1;
    start_timer();
}

void t_create(void (*func)(int), int thr_id, int pri) {
    /*
     * Create a new thread with the given start function, ID and
     * priority.  Puts it on the appropriate ready queue.
     */
    sighold(SIGALRM);

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

    new_thread->thread_context = (ucontext_t *)malloc(sizeof(ucontext_t));
    if (new_thread->thread_context == NULL) {
        perror("t_create: malloc failed for context");
        free(new_thread);
        sigrelse(SIGALRM);
        return;
    }
    if (getcontext(new_thread->thread_context) == -1) {
        perror("t_create: getcontext failed");
        free(new_thread->thread_context);
        free(new_thread);
        sigrelse(SIGALRM);
        return;
    }

    void *stack = malloc(STACK_SIZE);
    if (stack == NULL) {
        perror("t_create: malloc failed for stack");
        free(new_thread->thread_context);
        free(new_thread);
        sigrelse(SIGALRM);
        return;
    }

    new_thread->thread_context->uc_stack.ss_sp = stack;
    new_thread->thread_context->uc_stack.ss_size = STACK_SIZE;
    new_thread->thread_context->uc_stack.ss_flags = 0;
    new_thread->thread_context->uc_link = NULL;
    new_thread->stack = stack;

    /*
     * Keep track of every allocated stack so that it can be freed
     * during t_shutdown().  We avoid freeing stacks when a thread
     * terminates because the terminating thread is still executing on
     * its own stack.  Deferring reclamation until shutdown prevents
     * use-after-free bugs and satisfies valgrind’s leak checker.
     */
    record_stack(stack);

    makecontext(new_thread->thread_context, (void (*)())func, 1, thr_id);

    if (pri == HIGH_PRIORITY) {
        add_to_queue(&ready_q_high, new_thread);
    } else {
        add_to_queue(&ready_q_low, new_thread);
    }

    sigrelse(SIGALRM);
}

/*
 * t_yield - Voluntarily yield the CPU
 */
void t_yield(void) {
    /*
     * Voluntarily yield the CPU to another ready thread.  We move
     * the currently running thread to the end of its ready queue and
     * schedule the next thread.  IMPORTANT: we do NOT clear 'running'
     * before calling schedule(), otherwise schedule() can't
     * swapcontext() out of us and we'd never resume after the yield.
     */
    if (running == NULL) {
        return;
    }

    sighold(SIGALRM);
    stop_timer();

    /* If nobody is ready, just keep running ourselves. */
    if (ready_q_high == NULL && ready_q_low == NULL) {
        start_timer();
        sigrelse(SIGALRM);
        return;
    }

    /* Put current thread at end of its ready queue. */
    tcb *cur = running;
    if (cur->thread_priority == HIGH_PRIORITY) {
        add_to_queue(&ready_q_high, cur);
    } else {
        add_to_queue(&ready_q_low, cur);
    }

    /* schedule() still sees 'running == cur', so it will treat us as
     * 'prev' and use swapcontext().  When we get CPU again, execution
     * continues right after this call. */
    schedule();
    /* After schedule returns via swapcontext(), we have been resumed.
     * Restart the timer for this thread before releasing SIGALRM. */
    start_timer();
    /* We're back (after being rescheduled).  Turn interrupts back on. */
    sigrelse(SIGALRM);
}

void t_terminate(void) {
    /*
     * Terminate the calling thread and never return.
     */
    if (running == NULL) {
        return;
    }

    sighold(SIGALRM);
    stop_timer();

    tcb *to_free = running;
    running = NULL;

    /*
     * Do not free the stack here.  The current thread is still
     * executing on its own stack, so freeing it now would lead to
     * undefined behaviour.  All stacks are reclaimed in
     * t_shutdown() via free_all_stacks().
     */
    if (to_free->thread_context != NULL) {
        free(to_free->thread_context);
        to_free->thread_context = NULL;
    }
    free(to_free);

    sigrelse(SIGALRM);

    /* Run next thread (setcontext inside schedule never returns). */
    schedule();
}

void t_shutdown(void) {
    /*
     * Free all threads and stop timer.
     */
    sighold(SIGALRM);
    stop_timer();
    timer_enabled = 0;

    if (running != NULL) {
        /* Free the context and TCB for the running thread.  We do
         * not free the stack here; all stacks are released by
         * free_all_stacks() after this function finishes. */
        if (running->thread_context != NULL) {
            free(running->thread_context);
            running->thread_context = NULL;
        }
        free(running);
        running = NULL;
    }

    while (ready_q_high != NULL) {
        tcb *thr = remove_from_queue(&ready_q_high);
        if (thr != NULL) {
            /* Free context and TCB; stacks are freed later. */
            if (thr->thread_context != NULL) {
                free(thr->thread_context);
                thr->thread_context = NULL;
            }
            free(thr);
        }
    }

    while (ready_q_low != NULL) {
        tcb *thr = remove_from_queue(&ready_q_low);
        if (thr != NULL) {
            /* Free context and TCB; stacks are freed later. */
            if (thr->thread_context != NULL) {
                free(thr->thread_context);
                thr->thread_context = NULL;
            }
            free(thr);
        }
    }

    /* Now free all stacks that were allocated throughout execution. */
    free_all_stacks();

    sigrelse(SIGALRM);
}


/* ========== Phase 3: Semaphore Operations ========== */

int sem_init(sem_t **sp, unsigned int count) {
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
 * sem_wait - P operation
 */
void sem_wait(sem_t *sp) {
    /*
     * Decrement the semaphore and block the current thread if the
     * resulting count is negative.  The key trick: when we block, we
     * put the current thread on sem->q but we do NOT clear 'running'
     * before calling schedule(), so schedule() can swapcontext()
     * away from us and later resume us right after this call.
     */
    if (sp == NULL) {
        return;
    }

    sem_t_internal *sem = (sem_t_internal *)sp;

    sighold(SIGALRM);

    sem->count--;
    if (sem->count < 0) {
        /* Need to block this thread. */
        tcb *cur = running;

        /* Put it on semaphore wait queue. */
        add_to_queue(&sem->q, cur);

        /* Run someone else.  Because 'running' still points at cur,
         * schedule() will treat it as 'prev' and use swapcontext(). */
        schedule();
        /* When this thread is later signaled and scheduled, execution
         * continues right here, with SIGALRM still held.  Restart the timer
         * for this thread before releasing SIGALRM so that its time slice
         * resumes properly. */
        start_timer();
    }

    sigrelse(SIGALRM);
}

/*
 * sem_signal - V operation
 */
void sem_signal(sem_t *sp) {
    if (sp == NULL) {
        return;
    }
    sem_t_internal *sem = (sem_t_internal *)sp;
    sighold(SIGALRM);
    sem->count++;
    if (sem->count <= 0) {
        tcb *thr = remove_from_queue(&sem->q);
        if (thr != NULL) {
            if (thr->thread_priority == HIGH_PRIORITY) {
                add_to_queue(&ready_q_high, thr);
            } else {
                add_to_queue(&ready_q_low, thr);
            }
        }
    }
    sigrelse(SIGALRM);
}

void sem_destroy(sem_t **sp) {
    if (sp == NULL || *sp == NULL) {
        return;
    }
    sem_t_internal *sem = (sem_t_internal *)(*sp);
    sighold(SIGALRM);
    tcb *thr;
    while ((thr = remove_from_queue(&sem->q)) != NULL) {
        if (thr->thread_priority == HIGH_PRIORITY) {
            add_to_queue(&ready_q_high, thr);
        } else {
            add_to_queue(&ready_q_low, thr);
        }
    }
    free(sem);
    *sp = NULL;
    sigrelse(SIGALRM);
}


/* ========== Phase 4: (Optional, unimplemented) ========== */

int mbox_create(mbox **mb) {
    (void)mb;
    return -1;
}

void mbox_destroy(mbox **mb) {
    (void)mb;
}

void mbox_deposit(mbox *mb, char *msg, int len) {
    (void)mb;
    (void)msg;
    (void)len;
}

void mbox_withdraw(mbox *mb, char *msg, int *len) {
    (void)mb;
    (void)msg;
    (void)len;
}

void send(int tid, char *msg, int len) {
    (void)tid;
    (void)msg;
    (void)len;
}

void receive(int *tid, char *msg, int *len) {
    (void)tid;
    (void)msg;
    (void)len;
}

void block_send(int tid, char *msg, int len) {
    (void)tid;
    (void)msg;
    (void)len;
}

void block_receive(int *tid, char *msg, int *len) {
    (void)tid;
    (void)msg;
    (void)len;
}
