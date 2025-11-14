# Programming Assignment 3: User-Level Thread Library

## CISC 361 - Operating Systems

---

## Assignment Objective

In this project, you will design and implement a **user-level thread library** which supports thread creation, thread scheduling, thread synchronization, and inter-thread communication (message passing and mailbox) functions.

You will implement this library in **4 phases** over the course of the assignment. The last phase (Phase 4) is OPTIONAL. If implemented, you will get BONUS.

---

## Table of Contents
1. [Assignment Specification](#assignment-specification)
2. [Getting Started](#getting-started)
3. [Phase-by-Phase Implementation Guide](#phase-by-phase-implementation-guide)
4. [Important Concepts](#important-concepts)
5. [Testing Strategy](#testing-strategy)
6. [Debugging Tips](#debugging-tips)
7. [Grading and Submission](#grading-and-submission)

---

## Assignment Specification

### Phase 1: Thread Library Initialization and Basic Operations

**Functions to implement:**

1. **`void t_init()`**
   - Initialize the thread library by setting up the "running" and "ready" queues
   - Create TCB of the "main" thread with `thread_priority = 1` (low) and `thread_id = 0`
   - Insert main thread into the running queue

2. **`void t_shutdown()`**
   - Shut down the thread library by freeing all **dynamically allocated memory**
   - Must prevent memory leaks!

3. **`void t_create(void (*func)(int), int thr_id, int pri)`**
   - Create a thread with priority `pri` and start function `func`
   - `thr_id` is passed as argument to the thread function
   - Function `func` should be of type `void func(int)`
   - TCB of newly created thread is added to the **END** of the ready queue
   - Parent thread continues execution after `t_create()` returns

4. **`void t_terminate()`**
   - Terminate the calling thread
   - Remove (and free) its TCB from the running queue
   - Resume execution of thread in head of ready queue via `setcontext()`
   - **Every thread must end with a call to `t_terminate()`**

5. **`void t_yield()`**
   - Calling thread voluntarily relinquishes the CPU
   - Thread is placed at the END of the ready queue
   - First thread in ready queue resumes execution

**Thread Control Block (TCB) Structure:**

```c
struct tcb {
    int         thread_id;
    int         thread_priority;
    ucontext_t *thread_context;
    struct tcb *next;
};

typedef struct tcb tcb;
```

**Header File Organization:**

- **`ud_thread.h`**: Public API header to be included by applications
  - Contains ONLY function prototypes (like `pthread.h`)
  - Should NOT contain internal implementation details

- **`t_lib.h`**: Internal implementation header
  - Includes `ucontext.h`, `stdlib.h`, etc.
  - Declares internal structures like `tcb`
  - Should NOT be visible to applications

---

### Phase 2: CPU Scheduling and Time Slicing

**Scheduling Policy:**

1. **2-Level Queue (2LQ)**
   - Level 0: High priority queue
   - Level 1: Low priority queue (default for main thread)
   - Threads in low priority queue execute ONLY when high priority queue is empty

2. **Round-Robin (RR) within each level**
   - Time quantum: 10,000 microseconds (10ms)
   - Use `ualarm()` to simulate timer interrupts
   - When SIGALRM is caught, perform context switching
   - Before switching to new thread, schedule another SIGALRM via `ualarm()`
   - When thread yields before quantum expires, cancel scheduled SIGALRM via `ualarm(0,0)`

**Atomic Operations:**

The following functions MUST be made **atomic** by disabling timer interrupts:
- `t_init()`
- `t_terminate()`
- `t_shutdown()`
- `t_yield()`

Use `sighold(SIGALRM)` and `sigrelse(SIGALRM)` to disable/enable interrupts.

---

### Phase 3: Thread Synchronization with Semaphores

**Functions to implement:**

1. **`int sem_init(sem_t **sp, unsigned int sem_count)`**
   - Create a new semaphore pointed to by `sp`
   - Initialize count to `sem_count`
   - Return 0 on success, -1 on failure

2. **`void sem_wait(sem_t *sp)`**
   - Current thread does a wait (P operation) on semaphore
   - May block if semaphore count goes negative

3. **`void sem_signal(sem_t *sp)`**
   - Current thread does a signal (V operation) on semaphore
   - Follow **Mesa semantics**: signaling thread continues, first waiting thread becomes ready

4. **`void sem_destroy(sem_t **sp)`**
   - Free all memory related to the semaphore
   - If threads are queued, move them to ready queue

**Semaphore Structure (in t_lib.h):**

```c
typedef struct {
    int count;
    tcb *q;
} sem_t_internal;
```

**Type Definition (in ud_thread.h):**

```c
typedef void sem_t;  // Opaque type for applications
```

**Atomicity Requirement:**

`sem_wait()` and `sem_signal()` MUST be atomic. Use `sighold(SIGALRM)` and `sigrelse(SIGALRM)`.

**Homework Problems (Required):**

Implement the following using your thread library and semaphores:

1. **Rendezvous Problem** (OSTEP Chapter 31, Question 2)
   - File: `rendezvous.c`
   - Two threads must synchronize at a rendezvous point

2. **Partial Order Problem**
   - File: `partialorder.c`
   - Multiple threads with ordering constraints

---

### Phase 4: Inter-Thread Communication [OPTIONAL; BONUS PHASE]

#### Mailbox Functions (Non-blocking)

1. **`int mbox_create(mbox **mb)`**
   - Create a mailbox pointed to by `mb`
   - Return 0 on success, -1 on failure

2. **`void mbox_destroy(mbox **mb)`**
   - Destroy mailbox and free all related state

3. **`void mbox_deposit(mbox *mb, char *msg, int len)`**
   - Deposit message `msg` of length `len` into mailbox

4. **`void mbox_withdraw(mbox *mb, char *msg, int *len)`**
   - Withdraw first message from mailbox
   - Caller allocates space for received message
   - If no message available, set `*len = 0`
   - **Non-blocking**: returns immediately
   - Messages withdrawn in FIFO order

**Mailbox Structure:**

```c
struct messageNode {
    char *message;             // copy of the message
    int  len;                  // length of the message
    int  sender;               // TID of sender thread
    int  receiver;             // TID of receiver thread
    struct messageNode *next;  // pointer to next node
};

typedef struct {
    struct messageNode *msg;   // message queue
    sem_t *mbox_sem;           // used as lock
} mbox;
```

#### Message Passing Functions (Asynchronous Communication)

1. **`void send(int tid, char *msg, int len)`**
   - Send message to thread with ID `tid`
   - `msg` points to start of message
   - `len` specifies message length in bytes
   - All messages are character strings
   - **Asynchronous**: sender continues immediately

2. **`void receive(int *tid, char *msg, int *len)`**
   - Wait for and receive a message
   - Input: `*tid` specifies sender (0 = accept from any thread, X = accept only from thread X)
   - Output: `*tid` set to actual sender's TID
   - Caller allocates space for message
   - **Blocking**: waits until matching message arrives
   - Messages received in FIFO order

**Implementation Notes:**

The assignment suggests implementing message passing in two steps:

1. **Non-blocking receive()**:
   - Search thread's message queue for matching message
   - Use binary semaphore to protect queue access
   - Return immediately if no match

2. **Non-discriminatory, blocking receive()**:
   - Use counting semaphore in TCB to block when no messages
   - `send()` signals recipient's semaphore
   - Semaphore count tracks number of messages

#### Synchronous Communication

1. **`void block_send(int tid, char *msg, int len)`**
   - Like `send()`, but caller waits until recipient receives message
   - Implements rendezvous communication

2. **`void block_receive(int *tid, char *msg, int *len)`**
   - Like `receive()`, but caller must specify sender

---

## How to Get Started

### Understanding User Context

The `ucontext` API is the foundation of this thread library. Understanding it thoroughly is critical for success.

#### What is a Context?

A **user context** (`ucontext_t`) is a data structure that represents the complete execution state of a thread, including:

- **CPU registers** (general purpose, special purpose)
- **Program counter (PC)** - the address of the next instruction to execute
- **Stack pointer (SP)** - the current position in the stack
- **Signal mask** - which signals are blocked
- **Stack information** - location and size of the stack

When you perform a context switch, you're essentially freezing one thread's execution state and resuming another's.

#### The Four ucontext Functions

**1. `int getcontext(ucontext_t *ucp)`**

Saves the **current** execution context into `*ucp`.

```c
ucontext_t context;
getcontext(&context);  // Captures current state (registers, PC, SP, etc.)
```

- Returns 0 on success, -1 on failure
- Captures the state **at the moment** `getcontext()` is called
- Like taking a snapshot of the CPU's current state

**2. `int setcontext(const ucontext_t *ucp)`**

Restores a previously saved context and **never returns**.

```c
setcontext(&saved_context);  // Resume execution from saved_context
// THIS LINE NEVER EXECUTES - control transfers to saved_context
```

- If successful, **does not return** to the caller
- Execution continues from where `getcontext()` captured the context
- Think of it as "teleporting" to a saved execution state
- Only returns (-1) if there's an error

**3. `void makecontext(ucontext_t *ucp, void (*func)(), int argc, ...)`**

Modifies a context to execute a **new function** with a **new stack**.

```c
ucontext_t new_context;
getcontext(&new_context);  // Get a template context

// Set up new stack
new_context.uc_stack.ss_sp = malloc(8192);      // Stack memory
new_context.uc_stack.ss_size = 8192;            // Stack size
new_context.uc_stack.ss_flags = 0;
new_context.uc_link = NULL;                     // What to do when func returns

// Make it execute my_function(42)
makecontext(&new_context, (void (*)())my_function, 1, 42);

// Now setcontext(&new_context) will start executing my_function(42)
```

- `ucp`: Context to modify (must be initialized with `getcontext()` first!)
- `func`: Function to execute (cast to `void (*)()`)
- `argc`: Number of integer arguments to pass to `func`
- `...`: The actual arguments (must all fit in `int`)

**Important notes about makecontext():**
- Must call `getcontext()` on `ucp` first to initialize it
- Must set `uc_stack` before calling `makecontext()`
- `uc_link` determines what happens when `func` returns:
  - `NULL`: process exits
  - Another context: execution continues there
- Arguments must be type `int` (use casting for pointers on 32-bit systems)

**4. `int swapcontext(ucontext_t *oucp, const ucontext_t *ucp)`**

Atomically saves current context and switches to another.

```c
swapcontext(&old_context, &new_context);
// Saves current state to old_context
// Resumes execution from new_context
// Returns here when someone switches back to old_context
```

- Equivalent to `getcontext(oucp)` followed by `setcontext(ucp)`, but **atomic**
- Returns 0 on success, -1 on failure
- **Does return** (unlike `setcontext()`), but only when execution switches back

#### How Context Switching Works

Here's a concrete example of how these functions enable multithreading:

```c
ucontext_t main_context, thread1_context, thread2_context;
char stack1[8192], stack2[8192];

void thread1_func(int id) {
    printf("Thread 1 running\n");
    swapcontext(&thread1_context, &thread2_context);  // Switch to thread 2
    printf("Thread 1 running again\n");
    swapcontext(&thread1_context, &main_context);     // Back to main
}

void thread2_func(int id) {
    printf("Thread 2 running\n");
    swapcontext(&thread2_context, &main_context);     // Back to main
}

int main() {
    // Create thread 1 context
    getcontext(&thread1_context);
    thread1_context.uc_stack.ss_sp = stack1;
    thread1_context.uc_stack.ss_size = 8192;
    thread1_context.uc_link = NULL;
    makecontext(&thread1_context, (void (*)())thread1_func, 1, 1);

    // Create thread 2 context
    getcontext(&thread2_context);
    thread2_context.uc_stack.ss_sp = stack2;
    thread2_context.uc_stack.ss_size = 8192;
    thread2_context.uc_link = NULL;
    makecontext(&thread2_context, (void (*)())thread2_func, 1, 2);

    // Start thread 1
    swapcontext(&main_context, &thread1_context);

    printf("Back in main\n");
    return 0;
}
```

**Output:**
```
Thread 1 running
Thread 2 running
Back in main
Thread 1 running again
Back in main
```

#### Why swapcontext() is Critical

For `t_yield()`, you need to:
1. Save the **current** thread's context (so you can resume it later)
2. Load the **next** thread's context (to run it now)

If you used `getcontext()` followed by `setcontext()`, there's a race condition. `swapcontext()` does both **atomically**.

#### Common Pitfalls

1. **Forgetting to call getcontext() before makecontext()**
   ```c
   // WRONG - context not initialized!
   ucontext_t ctx;
   ctx.uc_stack.ss_sp = malloc(8192);
   makecontext(&ctx, func, 0);  // Undefined behavior!

   // CORRECT
   ucontext_t ctx;
   getcontext(&ctx);  // Initialize first!
   ctx.uc_stack.ss_sp = malloc(8192);
   makecontext(&ctx, func, 0);
   ```

2. **Stack not allocated for new threads**
   - Main thread uses existing stack
   - New threads need their own stack (allocate with malloc)

3. **Wrong function pointer cast**
   ```c
   void my_func(int id) { ... }

   // WRONG
   makecontext(&ctx, my_func, 1, id);

   // CORRECT
   makecontext(&ctx, (void (*)())my_func, 1, id);
   ```

4. **Using setcontext() when you should use swapcontext()**
   - `setcontext()`: Never returns (use for t_terminate)
   - `swapcontext()`: Saves current context and switches (use for t_yield)

#### Read the Man Pages

These contain critical details:

```bash
man getcontext
man makecontext
man signal
man ualarm
man sighold
```

#### Understand the Concepts

Before coding, make sure you understand:
- What is saved in a context (registers, stack pointer, program counter)
- How context switching enables multithreading
- The difference between cooperative (yield) and preemptive (timer) scheduling
- How signals and timers enable preemption
- Why atomic operations are necessary (race conditions)

### Prerequisites

- GCC compiler
- Make utility
- POSIX-compatible system (Linux or macOS)
- Understanding of C memory management
- Familiarity with pointers and linked lists

---

## Project Structure

```
student-assignment/
├── ud_thread.h              # Public API (DO NOT MODIFY)
├── t_lib.h                  # Internal structures (DO NOT MODIFY)
├── t_lib.c                  # YOUR IMPLEMENTATION GOES HERE
├── test_phase1.c            # Phase 1 test
├── test_phase2.c            # Phase 2 test
├── test_phase3.c            # Phase 3 test
├── test_phase4_mailbox.c    # Phase 4 mailbox test
├── test_phase4_messaging.c  # Phase 4 messaging test
├── rendezvous.c             # Homework problem (add semaphore calls)
├── partialorder.c           # Homework problem (add semaphore calls)
├── Makefile                 # Build system
└── README.md                # This file
```

**Files YOU need to edit:**
- `t_lib.c` - All function implementations
- `rendezvous.c` - Add semaphore synchronization calls
- `partialorder.c` - Add semaphore synchronization calls
- `Makefile` - Uncomment test targets as you complete phases

**Files you should NOT modify:**
- `ud_thread.h` - Public API (already complete)
- `t_lib.h` - Internal structures (already complete)
- Test files (unless creating your own)

---

## Phase-by-Phase Implementation Guide

### Phase 1: Basic Thread Operations

**Goal:** Implement basic thread creation and cooperative scheduling

**Step-by-step approach:**

#### Step 1: Implement Queue Helper Functions

Start with the basic queue operations in `t_lib.c`. These functions manage linked lists of TCBs.

**`void add_to_queue(tcb **queue, tcb *node)`**

Algorithm:
```
1. Check if node is NULL → return early
2. Set node->next = NULL (ensure clean state)
3. If queue is empty (*queue == NULL):
     Set *queue = node (node becomes the only element)
4. Else:
     Traverse to the END of the queue
     Append node to the last element's next pointer
```

Key points:
- This adds to the **END** (FIFO order for round-robin)
- Handle the empty queue case separately
- Make sure to traverse to find the last node

**`tcb *remove_from_queue(tcb **queue)`**

Algorithm:
```
1. If queue is empty (*queue == NULL):
     Return NULL
2. Save pointer to first node
3. Update *queue to point to second node (*queue = (*queue)->next)
4. Set removed node's next pointer to NULL
5. Return the removed node
```

Key points:
- This removes from the **FRONT** (FIFO dequeue)
- Don't forget to clear the removed node's next pointer
- Handle empty queue gracefully

**Test these functions first!** They are used everywhere. Write a simple test in main() before proceeding.

#### Step 2: Implement t_init()

**Purpose:** Initialize the thread library and create a TCB for the main thread.

**Algorithm:**
```
1. Allocate TCB for main thread using malloc()
   - Check for allocation failure

2. Initialize main thread's TCB fields:
   - thread_id = 0 (as per spec)
   - thread_priority = 1 (LOW_PRIORITY, as per spec)
   - stack = NULL (main thread uses existing stack)
   - next = NULL

3. Allocate ucontext_t for main thread's context
   - Check for allocation failure
   - Call getcontext() to save current execution state
   - Check getcontext() return value for errors

4. Set global "running" pointer to main_thread

5. Initialize ready queues to NULL:
   - ready_q_high = NULL
   - ready_q_low = NULL

6. (Phase 2 only) Set up signal handling:
   - Register SIGALRM handler using signal()
   - Set timer_enabled flag
   - Start the timer
```

**Key points:**
- Main thread has special ID (0) and priority (LOW)
- Main thread doesn't need a separate stack (uses program's main stack)
- Always check malloc() and getcontext() for errors
- Use perror() and exit() on critical failures
- Phase 2 additions will be commented out initially

#### Step 3: Implement t_create()

**Purpose:** Create a new thread with the specified function, ID, and priority.

This is the most complex function in Phase 1!

**Algorithm:**
```
1. (Phase 2) Disable interrupts with sighold(SIGALRM)

2. Allocate TCB for new thread:
   - malloc(sizeof(tcb))
   - If fails, print error and return
   - Initialize: thread_id = thr_id, thread_priority = pri, next = NULL

3. Allocate ucontext_t for thread's context:
   - malloc(sizeof(ucontext_t))
   - If fails, free TCB and return
   - Call getcontext() to initialize the context
   - If getcontext() fails, cleanup and return

4. Allocate stack for new thread:
   - malloc(STACK_SIZE)  // Typically 8192 bytes
   - If fails, free context and TCB, then return
   - Store pointer in TCB's stack field

5. Configure the context's stack:
   - Set uc_stack.ss_sp = allocated stack pointer
   - Set uc_stack.ss_size = STACK_SIZE
   - Set uc_stack.ss_flags = 0
   - Set uc_link = NULL (or address of cleanup context)

6. Use makecontext() to set the entry point:
   - First arg: pointer to the context
   - Second arg: function pointer (MUST CAST to void(*)())
   - Third arg: number of int arguments (1 in our case)
   - Fourth arg: the thread_id value to pass to func

   Example: makecontext(ctx, (void(*)())func, 1, thr_id);

7. Add new thread to appropriate ready queue:
   - If pri == HIGH_PRIORITY (0): add to ready_q_high
   - Else: add to ready_q_low
   - Use your add_to_queue() helper function

8. (Phase 2) Re-enable interrupts with sigrelse(SIGALRM)
```

**Critical details:**
- **Memory management:** On ANY failure, free previously allocated resources
- **makecontext() cast:** Function pointer MUST be cast to `(void (*)())`
- **Stack ownership:** New threads need their own stack (main thread doesn't)
- **uc_link:** Setting to NULL means process exits if thread function returns (threads should call t_terminate() instead)
- **Queue selection:** HIGH_PRIORITY = 0, LOW_PRIORITY = 1

#### Step 4: Implement t_yield()

**Purpose:** Current thread voluntarily gives up CPU to another thread.

**Algorithm:**
```
1. (Phase 2) Make atomic:
   - sighold(SIGALRM) to disable interrupts
   - stop_timer() to cancel current quantum

2. Check if there are other threads:
   - If both ready_q_high AND ready_q_low are NULL:
       No other threads exist
       (Phase 2) Restart timer
       (Phase 2) sigrelse(SIGALRM)
       Return (current thread continues)

3. Save pointer to current running thread

4. Move current thread to its ready queue:
   - If current->thread_priority == HIGH_PRIORITY:
       Add to ready_q_high
   - Else:
       Add to ready_q_low
   - Use add_to_queue() helper

5. Select next thread (2-level priority):
   - If ready_q_high is NOT empty:
       Remove from ready_q_high
   - Else:
       Remove from ready_q_low
   - Set running = selected thread

6. (Phase 2) Start timer for new thread

7. Perform context switch:
   - Use swapcontext(current_context, next_context)
   - This saves current state AND switches to next thread
   - Control returns here when this thread runs again

8. (Phase 2) sigrelse(SIGALRM) to re-enable interrupts
```

**Key points:**
- Use **swapcontext()** not setcontext() (we need to save current thread)
- HIGH priority threads run before LOW priority threads
- This implements cooperative multitasking (Phase 1)
- Phase 2 adds preemption via timers

#### Step 5: Implement t_terminate()

**Purpose:** Terminate the calling thread and free its resources.

**Algorithm:**
```
1. (Phase 2) Make atomic:
   - sighold(SIGALRM)
   - stop_timer()

2. Safety check:
   - If running == NULL, return

3. Save pointer to terminating thread
   - Set running = NULL

4. Free all resources for terminating thread:
   - Free stack (if not NULL)
   - Free thread_context (if not NULL)
   - Free TCB itself

5. Select next thread to run:
   - If ready_q_high is not empty:
       Remove from ready_q_high
   - Else if ready_q_low is not empty:
       Remove from ready_q_low
   - Set running = selected thread

6. Switch to next thread:
   - (Phase 2) start_timer() for new thread
   - Use setcontext(next_thread->context)
   - This NEVER RETURNS (we freed the current thread!)

7. (Phase 2) sigrelse(SIGALRM)
```

**Key points:**
- Use **setcontext()** not swapcontext() (we're not coming back!)
- Free resources BEFORE switching contexts
- Check for NULL before freeing
- Every thread function MUST call t_terminate()
- Main thread's stack is NULL (don't free it!)

#### Step 6: Implement t_shutdown()

**Purpose:** Clean up all thread library resources (prevent memory leaks!).

**Algorithm:**
```
1. (Phase 2) Disable interrupts:
   - sighold(SIGALRM)
   - stop_timer()
   - timer_enabled = 0

2. Free running thread (usually main thread):
   - If running != NULL:
       Free stack (if not NULL)
       Free thread_context
       Free TCB
       Set running = NULL

3. Free all threads in high priority queue:
   - While ready_q_high is not empty:
       Remove thread from queue
       Free stack (if not NULL)
       Free thread_context
       Free TCB

4. Free all threads in low priority queue:
   - While ready_q_low is not empty:
       Remove thread from queue
       Free stack (if not NULL)
       Free thread_context
       Free TCB

5. (Phase 2) sigrelse(SIGALRM)
```

**Key points:**
- Must free ALL allocated memory
- Check valgrind for memory leaks: `valgrind --leak-check=full ./test_phase1`
- Remember: main thread's stack is NULL (don't free NULL!)
- Free in correct order: contents first, then container

**Testing Phase 1:**

```bash
make clean
make all
./test_phase1
```

**Expected behavior:**
- Threads should create successfully
- Threads should yield to each other in round-robin fashion
- All threads should terminate cleanly
- No memory leaks (check with valgrind)

**Common Phase 1 Issues:**

1. **Forgetting `t_terminate()`** - Every thread function MUST end with `t_terminate()`
2. **Stack overflow** - Increase `STACK_SIZE` if needed
3. **NULL pointer handling** - Check queue functions handle empty queues
4. **Memory leaks** - Use valgrind to find leaks

---

### Phase 2: Scheduling and Time Slicing

**Goal:** Add preemptive scheduling with 2-level priority queue and time slicing

**What changes from Phase 1:**

1. Add timer interrupt handling
2. Make library functions atomic
3. Implement 2-level queue scheduler
4. Add preemption via SIGALRM

**Step-by-step approach:**

#### Step 1: Implement Timer Functions

**`void start_timer(void)`**

Algorithm:
```
If timer_enabled is true:
    Call ualarm(TIME_QUANTUM, 0)
    TIME_QUANTUM = 10000 microseconds (10ms)
```

**`void stop_timer(void)`**

Algorithm:
```
Call ualarm(0, 0) to cancel any pending alarm
```

#### Step 2: Implement schedule()

**Purpose:** Select and switch to the next thread (2-level queue scheduling).

Algorithm:
```
1. Determine next thread:
   - If ready_q_high is not empty:
       next_thread = remove from ready_q_high
   - Else if ready_q_low is not empty:
       next_thread = remove from ready_q_low
   - Else:
       No threads available, return

2. Save previous running thread pointer

3. Update running pointer to next_thread

4. Start timer for new thread

5. Context switch:
   - If prev_thread is NULL:
       Use setcontext() (first time scheduling)
   - Else:
       Use swapcontext() (save previous, run next)
```

#### Step 3: Implement timer_handler()

**Purpose:** Signal handler called when time quantum expires.

Algorithm:
```
1. Ignore signal parameter (required by signal API)

2. Safety check: if running == NULL, return

3. Save current thread pointer

4. Set running = NULL

5. Move preempted thread to its ready queue:
   - If thread_priority == HIGH_PRIORITY:
       Add to ready_q_high
   - Else:
       Add to ready_q_low

6. Call schedule() to select next thread
```

**Key point:** This runs asynchronously when SIGALRM fires!

#### Step 4: Update t_init() for Phase 2

Add these lines to the end of `t_init()`:

```
1. Register signal handler:
   signal(SIGALRM, timer_handler)

2. Enable timer:
   timer_enabled = 1

3. Start initial timer:
   start_timer()
```

#### Step 5: Make Functions Atomic

Add signal blocking to prevent race conditions:

**Functions that need atomicity:**
- `t_create()`
- `t_yield()`
- `t_terminate()`
- `t_shutdown()`

**Pattern to use:**
```
At beginning of function:
    sighold(SIGALRM);    // Block SIGALRM
    stop_timer();        // Cancel pending alarm (for some functions)

At end of function (before return):
    start_timer();       // Restart timer (for some functions)
    sigrelse(SIGALRM);   // Unblock SIGALRM
```

**Why?** Prevents timer interrupts during critical sections (queue manipulation, context switching).

**Testing Phase 2:**

```bash
# Uncomment test_phase2 in Makefile first
make clean
make all
./test_phase2
```

**Expected behavior:**
- High priority threads run before low priority threads
- Time slicing causes preemption during busy loops
- Round-robin within each priority level

---

### Phase 3: Semaphores

**Goal:** Implement counting semaphores for thread synchronization

**Semaphore Algorithm:**

```
sem_wait (P operation):
    count--
    if (count < 0):
        block thread (add to semaphore queue)

sem_signal (V operation):
    count++
    if (count <= 0):
        wake one thread (move to ready queue)
```

**Implementation:**

#### Step 1: Implement sem_init()

**Purpose:** Create and initialize a semaphore.

Algorithm:
```
1. Allocate sem_t_internal structure
   - If malloc fails, return -1

2. Initialize fields:
   - sem->count = count (initial value passed in)
   - sem->q = NULL (no threads waiting yet)

3. Cast to sem_t* and store in *sp

4. Return 0 (success)
```

**Note:** The opaque type `sem_t` hides `sem_t_internal` from applications.

#### Step 2: Implement sem_wait()

**Purpose:** P operation (down/wait) - may block if count goes negative.

Algorithm:
```
1. Make atomic: sighold(SIGALRM)

2. Cast sp to sem_t_internal*

3. Decrement count: sem->count--

4. If count < 0 (resource not available):
     a. Save current running thread pointer
     b. Set running = NULL
     c. Add current thread to semaphore's wait queue (sem->q)
     d. Call schedule() to run next thread
        (This blocks the current thread)

5. Release atomicity: sigrelse(SIGALRM)
```

**Key point:** Negative count = number of waiting threads

#### Step 3: Implement sem_signal()

**Purpose:** V operation (up/signal) - may wake a waiting thread.

Algorithm:
```
1. Make atomic: sighold(SIGALRM)

2. Cast sp to sem_t_internal*

3. Increment count: sem->count++

4. If count <= 0 (there were waiting threads):
     a. Remove one thread from semaphore's wait queue
     b. If thread exists:
          - Add to ready_q_high if HIGH_PRIORITY
          - Add to ready_q_low if LOW_PRIORITY
        (Thread is now ready to run)

5. Signaling thread continues (Mesa semantics)

6. Release atomicity: sigrelse(SIGALRM)
```

**Key point:** Mesa semantics = signaler doesn't yield CPU

#### Step 4: Implement sem_destroy()

**Purpose:** Free semaphore and wake all waiting threads.

Algorithm:
```
1. Make atomic: sighold(SIGALRM)

2. Cast *sp to sem_t_internal*

3. While semaphore's wait queue is not empty:
     a. Remove thread from sem->q
     b. Add thread to appropriate ready queue based on priority

4. Free the semaphore structure

5. Set *sp = NULL

6. Release atomicity: sigrelse(SIGALRM)
```

**Key point:** All waiting threads become runnable when semaphore is destroyed.

#### Step 5: Complete Homework Problems

**rendezvous.c:**
- Add `sem_init()` calls to create semaphores
- Add `sem_wait()` and `sem_signal()` calls in appropriate places
- Add `sem_destroy()` calls to clean up

**partialorder.c:**
- Similar to rendezvous, but with more complex ordering

**Testing Phase 3:**

```bash
# Uncomment test_phase3 in Makefile
make clean
make all
./test_phase3
make test_homework
```

---

### Phase 4: Inter-Thread Communication [OPTIONAL; BONUS PHASE]

**Goal:** Implement mailboxes and message passing

**Architecture:**

Each thread's TCB has:
- `msg_queue`: Linked list of incoming messages
- `msg_sem`: Counting semaphore (count = number of messages)
- `mbox_lock`: Binary semaphore (protects queue access)

**Implementation:**

#### Step 1: Implement get_tcb_by_id()

**Purpose:** Find a thread's TCB by its thread ID (needed for message passing).

Algorithm:
```
1. Check if running thread matches:
   - If running != NULL AND running->thread_id == tid:
       Return running

2. Search high priority queue:
   - Traverse ready_q_high
   - For each node:
       If thread_id == tid, return that TCB

3. Search low priority queue:
   - Traverse ready_q_low
   - For each node:
       If thread_id == tid, return that TCB

4. If not found anywhere, return NULL
```

**Note:** You may also need to search semaphore wait queues in a full implementation.

#### Step 2: Implement Mailbox Functions

**`int mbox_create(mbox **mb)`**

Algorithm:
```
1. Allocate mbox structure
   - Return -1 if malloc fails

2. Initialize fields:
   - msg = NULL (no messages yet)
   - Create binary semaphore (count = 1) for mbox_sem
     This semaphore acts as a lock

3. Store pointer in *mb

4. Return 0 (success)
```

**`void mbox_destroy(mbox **mb)`**

Algorithm:
```
1. While message queue is not empty:
     - Remove message node
     - Free message content
     - Free node

2. Destroy the mbox_sem semaphore

3. Free the mbox structure

4. Set *mb = NULL
```

**`void mbox_deposit(mbox *mb, char *msg, int len)`**

Algorithm:
```
1. Lock mailbox: sem_wait(mb->mbox_sem)

2. Create new messageNode:
   - Allocate node
   - Allocate and copy message content (len bytes)
   - Set len, sender (running->thread_id), receiver (-1)

3. Add message to END of mailbox queue (FIFO)

4. Unlock mailbox: sem_signal(mb->mbox_sem)
```

**`void mbox_withdraw(mbox *mb, char *msg, int *len)`**

Algorithm:
```
1. Lock mailbox: sem_wait(mb->mbox_sem)

2. If message queue is empty:
     - Set *len = 0
     - Unlock and return

3. Remove first message from queue

4. Copy message content to msg buffer

5. Set *len to message length

6. Free message content and node

7. Unlock mailbox: sem_signal(mb->mbox_sem)
```

#### Step 3: Implement Message Passing

**`void send(int tid, char *msg, int len)`**

Algorithm:
```
1. Find recipient's TCB using get_tcb_by_id(tid)
   - If not found, return (error)

2. Lock recipient's message queue: sem_wait(recipient->mbox_lock)

3. Create messageNode:
   - Allocate node and copy message
   - Set sender = running->thread_id
   - Set receiver = tid
   - Set len

4. Add to END of recipient's msg_queue (FIFO)

5. Signal recipient's message semaphore:
   sem_signal(recipient->msg_sem)
   This tells recipient a message arrived

6. Unlock: sem_signal(recipient->mbox_lock)
```

**`void receive(int *tid, char *msg, int *len)`**

This is the most complex Phase 4 function!

Algorithm:
```
1. Loop forever:

   2. Wait for message arrival:
      sem_wait(running->msg_sem)
      This blocks until someone sends a message

   3. Lock message queue:
      sem_wait(running->mbox_lock)

   4. Search for matching message:
      - If *tid == 0: accept from ANY sender
      - If *tid == X: accept only from sender X

      Traverse msg_queue to find first matching message

   5. If matching message found:
      a. Remove from queue
      b. Copy message content to msg buffer
      c. Set *len to message length
      d. Set *tid to actual sender's ID
      e. Free message and node
      f. Unlock: sem_signal(running->mbox_lock)
      g. Return (success!)

   6. If no match (sender discrimination):
      a. Unlock: sem_signal(running->mbox_lock)
      b. Continue loop (wait for another message)
```

**Key points:**
- Blocking: `sem_wait(msg_sem)` blocks until message arrives
- Sender discrimination: `*tid == 0` means accept any, else specific sender
- FIFO order within messages from same sender

**Testing Phase 4:**

```bash
# Uncomment Phase 4 tests in Makefile
make clean
make all
make test4
```

---

## Important Concepts

### ucontext API

```c
int getcontext(ucontext_t *ucp);        // Save current context
int setcontext(const ucontext_t *ucp);  // Restore context (never returns)
int swapcontext(ucontext_t *oucp,       // Save and restore atomically
                const ucontext_t *ucp);
void makecontext(ucontext_t *ucp,       // Create new context
                 void (*func)(),
                 int argc, ...);
```

### Signal Handling

```c
signal(SIGALRM, timer_handler);  // Register handler
ualarm(10000, 0);                // Schedule alarm in 10ms
sighold(SIGALRM);                // Disable signal
sigrelse(SIGALRM);               // Enable signal
```

---

## Testing Strategy

### Progressive Testing

1. **Phase 1**: Test with 1, 2, then many threads
2. **Phase 2**: Test single priority, then mixed priorities
3. **Phase 3**: Test binary semaphore, then counting semaphore
4. **Phase 4**: Test 2 threads, then broadcast patterns [OPTIONAL; BONUS PHASE]

### Memory Leak Checking

```bash
valgrind --leak-check=full ./test_phase1
```

Look for "definitely lost" - these are real leaks!

---

## Debugging Tips

### Use GDB

```bash
gcc -g test_phase1.c -L. -ludthread -o test_phase1
gdb ./test_phase1
(gdb) break t_yield
(gdb) run
(gdb) backtrace
```

### Add Debug Prints

```c
#ifdef DEBUG
    printf("[DEBUG] Thread %d yielding\n", running->thread_id);
#endif
```

---

## Grading and Submission

### Grading Rubric

| Component | Weight | Description |
|-----------|--------|-------------|
| Phase 1 & 2: Thread operations and scheduling | 100 pts | Init, create, terminate, 2LQ, RR, time slicing |
| Phase 3: Semaphores | 50 pts | All semaphore operations + homework problems |
| **Total** | **150 pts** | |
| Phase 4: IPC | [50 pts] | Mailbox and message passing -  [OPTIONAL; BONUS PHASE] |

### Submission Checklist

- [ ] All phases compile without errors
- [ ] `make clean && make all` works
- [ ] All tests pass
- [ ] No memory leaks (verified with valgrind)
- [ ] Code is well-commented
- [ ] Homework problems completed (rendezvous, partial order)
- [ ] Makefile includes all test targets

### Build Commands

```bash
make all           # Build everything
make clean         # Remove artifacts
make test          # Run all tests
make test1         # Run Phase 1 test
make test2         # Run Phase 2 test
make test3         # Run Phase 3 test
make test4         # Run Phase 4 tests
make test_homework # Run homework problems
```

---

## Final Tips

1. **Start early!** Each phase builds on the previous
2. **Test incrementally** - don't wait until end
3. **Read man pages** - they contain critical info
4. **Draw pictures** of your queues and context switches
5. **Use valgrind** to catch memory leaks
6. **Comment as you code**, not at the end
7. **Ask for help early** if stuck

Good luck! 🎓
