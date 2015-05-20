/* This file was adapted from GeekOS "kthread.h":

    Copyright (c) 2001,2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
    Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software,
    and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
    ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
    TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
    PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
    SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
    FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
    AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
    THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "x86.h"
#include "gdt.h"
#include "mem.h"
#include "int.h"
#include "timer.h"
#include "string.h"
#include "thread.h"

/* List of all threads in the system */
static thread_t* allThreadHead;

/* Queue of runnable threads */
static thread_queue_t runQueue;

/* Queue of sleeping threads */
static thread_queue_t sleepQueue;

/* Queue of finished threads needing disposal */
static thread_queue_t graveyardQueue;

/* Queue for reaper thread to communicate with dead threads */
static thread_queue_t reaperWaitQueue;

/* global, currently-running thread */
thread_t *g_current_thread = NULL;

/* When set, interrupts will not cause a new thread to be scheduled */
static volatile bool g_preemption_disabled;

/* Counter for keys that access thread-local data
 * (Based on POSIX threads' thread-specific data) */
static unsigned int tlocalKeyCounter;

/* Array of destructors for correspondingly keyed thread-local data */
static tlocal_destructor_t tlocalDestructors[MAX_TLOCAL_KEYS];

typedef void (thread_launch_func_t)(void);

/*
 * Add a thread to the list of all threads
 */
static void allThreadsAdd(thread_t* thread) {
    KASSERT(thread);

    thread->listNext = NULL;
    thread_t **t = &allThreadHead;
    while (*t != NULL) {
        KASSERT(*t != thread);
        t = &(*t)->listNext;
    }
    *t = thread;
}

/*
 * Remove a thread from the list of all threads
 */
static void allThreadsRemove(thread_t* thread) {
    KASSERT(thread);

    thread_t** t = &allThreadHead;
    while (*t != NULL) {
        if (thread == *t) {
            *t = (*t)->listNext;
            break;
        }
        t = &(*t)->listNext;
    }
    thread->listNext = NULL;
}


/*
 * 'Empty' a thread queue.
 */
void threadQueueClear(thread_queue_t* queue) {
    KASSERT(queue);
    queue->head = NULL;
    queue->tail = NULL;
}

bool threadQueueEmpty(thread_queue_t* queue) {
    KASSERT(queue);
    if (queue->head == NULL && queue->tail == NULL) {
        return true;
    }
    return false;
}

static bool containsThread(thread_queue_t* queue, thread_t* thread) {
    KASSERT(!interruptsEnabled());
    KASSERT(queue);
    KASSERT(thread);

    thread_t *cur = queue->head;
    while (cur) {
        if (cur == thread) {
            return true;
        }
        cur = cur->queueNext;
    }

    KASSERT(thread != queue->tail);

    return false;
}

static void enqueueThread(thread_queue_t* queue, thread_t* thread) {
    KASSERT(!interruptsEnabled());
    KASSERT(queue);
    KASSERT(thread);

    /* ensure thread points to no next thread */
    thread->queueNext = NULL;

    /* overkill - make sure thread is not already in queue */
    KASSERT(!containsThread(queue, thread));

    if (NULL == queue->head) {
        KASSERT(NULL == queue->tail);
        queue->head = thread;
        queue->tail = thread;
    } else if (queue->head == queue->tail) {
        KASSERT(NULL == queue->head->queueNext);
        queue->tail->queueNext = thread;
        queue->tail = thread;
    } else {
        queue->tail->queueNext = thread;
        queue->tail = thread;
    }
}

/*
 * Very carefully remove all instances of a thread from a queue
 */
static void dequeueThread(thread_queue_t* queue, thread_t* thread) {
    KASSERT(!interruptsEnabled());
    KASSERT(queue);
    KASSERT(thread);

    thread_t* cur = queue->head;
    thread_t* prev = NULL;
    while (cur) {
        if (thread == cur) {
            if (cur == queue->head) {
                /* remove head */
                queue->head = cur->queueNext;
            } else {
                /* remove thread in the middle */
                prev->queueNext = cur->queueNext;
            }
        } else {
            /* increment 'prev' only if cur was NOT removed */
            prev = cur;
        }
        cur = cur->queueNext;
    }

    queue->tail = prev;

    /* ensure thread is no longer pointing to its former 'next' thread */
    thread->queueNext = NULL;
}

/*
 * Clean up thread-local data.
 * Calls destructors *repeatedly* until all thread-local data is NULL.
 * Assumes interrupts disabled.
 */
static void tlocalExit(thread_t* thread) {
    KASSERT(!interruptsEnabled());
    KASSERT(thread);

    bool repeat = false;
    do {
        repeat = false;     // assume we don't need to repeat
        int idx;
        for (idx = 0; idx < MAX_TLOCAL_KEYS; idx++) {
            void* data = (void*)thread->tlocalData[idx];
            tlocal_destructor_t destructor = tlocalDestructors[idx];

            if (data != NULL && destructor != NULL) {
                thread->tlocalData[idx] = NULL;
                repeat = true;  // need to call all destructors again
                sti();
                tlocalDestructors[idx](data);
                cli();
            }
        }
    } while (repeat);
}


/*
 * Push a 32-bit value onto the thread's stack.
 * Used for initializing threads.
 */
static inline void pushDword(thread_t* thread, uint32_t value) {
    thread->esp -= 4;
    *(uint32_t*)thread->esp = value;
}

/*
 * Initialize members of a kernel thread
 */
static void initThread(thread_t* thread, void* stackPage,
        void* userStackPage, priority_t priority, bool detached) {
    static unsigned int next_free_id = 0;

    memset(thread, 0, sizeof(thread_t));

    thread->id = next_free_id++;

    thread->stackBase = stackPage;
    thread->esp = (uintptr_t)stackPage + PAGE_SIZE;
    thread->stackTop = thread->esp;

    thread->userStackBase = userStackPage;
    thread->userEsp = (userStackPage != NULL) ?
            (uintptr_t)userStackPage + PAGE_SIZE : 0;

    thread->priority = priority;
    thread->owner = detached ? NULL : g_current_thread;

    thread->refCount = detached ? 1 : 2;
    thread->alive = true;

    threadQueueClear(&thread->joinQueue);

    DEBUGF("new thread @ 0x%X, id: %d, esp: %X\n",
            thread, thread->id, thread->esp, thread->userEsp);
}

/*
 * Create new raw thread object.
 * @returns NULL if out of memory
 */
static thread_t* createThread(unsigned int priority, bool detached, bool usermode) {
    thread_t *thread = allocPage();
    DEBUGF("Allocated thread 0x%X\n", thread);
    if (!thread) {
        kprintf("Failed to allocate page for thread\n");
        return NULL;
    }

    void *stackPage = allocPage();
    if (!stackPage) {
        kprintf("Failed to allocate page for thread stack\n");
        freePage(thread);
        return NULL;
    }

    void *userStackPage = NULL;
    if (usermode) {
        userStackPage = allocPage();
        if (!userStackPage) {
            kprintf("Failed to allocate page for thread user stack\n");
            freePage(stackPage);
            freePage(thread);
            return NULL;
        }
    }

    initThread(thread, stackPage, userStackPage, priority, detached);

    allThreadsAdd(thread);

    return thread;
}

/*
 * Perform all necessary cleanup and destroy thread.
 * call with interrupts enabled.
 */
static void destroyThread(thread_t* thread) {
    KASSERT(thread);
    cli();

    freePage(thread->stackBase);
    if (thread->userStackBase) {
        freePage(thread->userStackBase);
    }
    freePage(thread);

    allThreadsRemove(thread);

    sti();
}

/*
 * pass thread to reaper for destruction
 * must be called with interrupts disabled
 */
static void reapThread(thread_t* thread) {
    KASSERT(!interruptsEnabled());
    KASSERT(thread);
    DEBUGF("reaping thread %d\n", thread->id);
    enqueueThread(&graveyardQueue, thread);
    wakeAll(&reaperWaitQueue);
}

/*
 * called when a reference to a thread is broken
 */
static void detachThread(thread_t* thread) {
    KASSERT(!interruptsEnabled());
    KASSERT(thread);
    KASSERT(thread->refCount > 0);

    DEBUGF("detaching thread %d\n", thread->id);

    --thread->refCount;
    if (thread->refCount == 0) {
        reapThread(thread);
    }
}

/*
 * Perform any necessary initialization before a thread start function
 * is executed. It currently only enables interrupts
 */
static void launchKernelThread(void) {
    /* DEBUGF("Launching thread %d\n", g_current_thread->id); */
    /* DEBUGF("%s\n", interruptsEnabled() ? "interrupts enabled\n" : "interrupts disabled\n"); */
    sti();
}

static void launchUserThread(void) {
    /* Now, 'start' user mode */
    extern void startUserMode();
    startUserMode();
}

/*
 * Shutdown a kernel thread if it exits by falling
 * off the end of its start function
 */
static void shutdownKernelThread(void) {
    /* DEBUGF("Shutting down thread %d\n", g_current_thread->id); */
    exit(0);
}

static void shutdownUserThread(void) {
    syscall_exit(0);
}

static void setupThreadStack(thread_t* thread,
        thread_startFunc_t startFunc, uint32_t arg, bool usermode) {
    uint32_t *esp = (uint32_t*)thread->esp;

    if (usermode) {
        /* Set up CPL=3 stack */
        uint32_t *uesp = thread->userEsp;
        KASSERT(uesp != NULL);

        /* push the arg to the thread start function */
        *--uesp = arg;

        /* push the address of the shutdownThread function as the
        * return address. this forces the thread to exit */
        *--uesp = shutdownUserThread;

        thread->userEsp = uesp;

        /* Set up CPL=0 stack */
        /* DEBUGF("Address of startFunc: %X\n", startFunc); */
        *--esp = startFunc;

        extern void startUserMode(void);
        *--esp = startUserMode;
    } else {
        /* push the arg to the thread start function */
        *--esp = arg;

        /* push the address of the shutdownThread function as the
        * return address. this forces the thread to exit */
        *--esp = shutdownKernelThread;

        *--esp = startFunc;

        *--esp = launchKernelThread;
    }

    /* push general registers */
    *--esp = 0;     /* eax */
    *--esp = 0;     /* ecx */
    *--esp = 0;     /* edx */
    *--esp = 0;     /* ebx */
    *--esp = 0;     /* ebp */
    *--esp = 0;     /* esi */
    *--esp = 0;     /* edi */

    /* update thread's ESP */
    thread->esp = esp;
}

static void idle(uint32_t arg) {
    (void)arg; /* prevent compiler warnings */
    DEBUG("Idle thread idling\n");
    while (true) {
        yield();
    }
}

static void reaper(uint32_t arg) {
    thread_t* thread;
    (void)arg; /* prevent compiler warnings */
    DEBUG("Reaper thread reaping\n");
    cli();

    while (true) {
        /* check if any threads need disposal */
        if ((thread = graveyardQueue.head) == NULL) {
            /* graveyard empty... wait for thread to die */
            wait(&reaperWaitQueue);
        } else {
            /* empty the graveyard queue */
            threadQueueClear(&graveyardQueue);

            /* re-enable interrupts. all threads needing disposal
             * have been disposed. */
            sti();
            yield();    /* allow other threads to run? */

            while (thread != 0) {
                thread_t* next = thread->queueNext;
                DEBUGF("Reaper destroying thread: 0x%x\n", thread);
                destroyThread(thread);
                thread = next;
            }

            /* disable interrupts again for another iteration of diposal */
            cli();
        }
    }
}

/*
 * Wake up any threads that are finished sleeping
 */
void wakeSleepers(void) {
    thread_t* next = NULL;
    thread_t* thread = sleepQueue.head;

    while (thread) {
        /* DEBUGF("attempting to wake thread %d\n", thread->id); */
        next = thread->queueNext;
        if (getTicks() >= thread->sleepUntil) {
            /* DEBUGF("waking thread %d (%u >= %u)\n", */
                    /* thread->id, getTicks(), thread->sleepUntil); */
            thread->sleepUntil = 0;
            /* time to wake up this sleeping thread...
             * so remove it from sleep queue and make it runnable */
            dequeueThread(&sleepQueue, thread); /* FIXME - dumb inefficient */
            makeRunnable(thread);
        } else {
            /* DEBUGF("thread %d still sleeping (%u < %u)\n", */
                    /* thread->id, getTicks(), thread->sleepUntil); */
        }
        thread = next;
    }
}

/*
 * Find best candidate thread in a thread queue.
 *
 * This currently returns the head of the thread queue,
 * so the scheduling algorithm is essentially FIFO
 */
static thread_t* findBest(thread_queue_t* queue) {
    KASSERT(queue);
    return queue->head;
}

thread_t* getNextRunnable(void) {
    thread_t* best = findBest(&runQueue);
    KASSERT(best);
    dequeueThread(&runQueue, best);

    return best;
}


/*
 * Determine a new key and set the destructor for thread-local data.
 *
 * @returns true on success, false on failure (no more keys)
 */
bool tlocalCreate(tlocal_key_t* key, tlocal_destructor_t destructor) {
    KASSERT(key);

    bool iFlag = begIntAtomic();

    if (tlocalKeyCounter >= MAX_TLOCAL_KEYS) {
        return false;
    }

    tlocalDestructors[tlocalKeyCounter] = destructor;
    *key = tlocalKeyCounter++;

    endIntAtomic(iFlag);

    return true;
}

void tlocalSet(tlocal_key_t key, const void* data) {
    KASSERT(key < tlocalKeyCounter);
    g_current_thread->tlocalData[key] = data;
}

void* tlocalGet(tlocal_key_t key) {
    KASSERT(key < tlocalKeyCounter);
    return (void*)g_current_thread->tlocalData[key];
}


void yield(void) {
    KASSERT(g_current_thread);
    cli();
    makeRunnable(g_current_thread);
    schedule();
    sti();
}

/*
 * Exit current thread and initiate a context switch
 */
void exit(int exitCode) {
    thread_t* current = g_current_thread;
    KASSERT(current);

    if (interruptsEnabled()) {
        cli();
    }

    current->exitCode = exitCode;
    current->alive = false;

    /* clean up thread-local data */
    tlocalExit(current);

    /* notify thread's possible owner */
    wakeAll(&current->joinQueue);

    /* remove thread's implicit reference to itself */
    detachThread(current);

    schedule();

    /* This thread will never be scheduled again, so it should
     * never get to this point */
    KASSERT(false);
}

/*
 * Wait for a thread to die.
 * Interrupts must be enabled.
 *
 * @returns thread exit code
 */
int join(thread_t* thread) {
    KASSERT(interruptsEnabled());

    KASSERT(thread);
    /* only the owner can join on a thread */
    KASSERT(thread->owner = g_current_thread);

    cli();

    while (thread->alive) {
        wait(&thread->joinQueue);
    }

    int exitcode = thread->exitCode;

    /* release reference to thread */
    detachThread(thread);

    sti();

    return exitcode;
}

/*
 * Places a thread on the sleep queue.
 * The thread will not become runnable until `ticks`
 * number of timer ticks have passed from the time
 * `sleep` is called.
 */
void sleep(unsigned int milliseconds) {
    KASSERT(g_current_thread);

    unsigned int ticks = milliseconds * TICKS_PER_SEC / 1000;
    if (ticks < 1) { ticks = 1; }

    bool iFlag = begIntAtomic();
    g_current_thread->sleepUntil = getTicks() + ticks;
    KASSERT(!interruptsEnabled());
    enqueueThread(&sleepQueue, g_current_thread);
    /* DEBUGF("thread %d sleeping until %u\n", g_current_thread->id, */
            /* g_current_thread->sleepUntil); */
    schedule();
    endIntAtomic(iFlag);
}

/*
 * Place current thread on a wait queue
 * Called with interrupts disabled.
 */
void wait(thread_queue_t* waitQueue) {
    KASSERT(!interruptsEnabled());
    KASSERT(waitQueue);
    KASSERT(g_current_thread);

    enqueueThread(waitQueue, g_current_thread);

    schedule();
}

/*
 * Wake up all threads waiting on a wait queue.
 * Called with interrupts disabled.
 */
void wakeAll(thread_queue_t* waitQueue) {
    thread_t* thread = waitQueue->head;
    thread_t* next = NULL;

    while (thread) {
        next = thread->queueNext;
        makeRunnable(thread);
        thread = next;
    }

    threadQueueClear(waitQueue);
}

/*
 * Wake up (one) thread that is the best candidate for running
 */
void wakeOne(thread_queue_t* waitQueue) {
    KASSERT(!interruptsEnabled());
    thread_t* best = findBest(waitQueue);
    if (best != NULL) {
        dequeueThread(waitQueue, best);
        makeRunnable(best);
    }
}

/*
 * Add thread to run queue so it will be scheduled
 */
void makeRunnable(thread_t* thread) {
    KASSERT(!interruptsEnabled());
    KASSERT(thread);
    enqueueThread(&runQueue, thread);
}

/*
 * Assumes interrupts are enabled before
 * atomically making thread runnable
 */
void makeRunnableAtomic(thread_t* thread) {
    KASSERT(thread);

    cli();
    makeRunnable(thread);
    sti();
}

/*
 * Schedule a runnable thread.
 * Called with interrupts disabled.
 * g_current_thread should already be place on another
 * queue (or left on run queue)
 */
extern void switchToThread(thread_t*);
void schedule(void) {
    KASSERT(!interruptsEnabled());
    KASSERT(!g_preemption_disabled);

    wakeSleepers();

    thread_t* runnable = getNextRunnable();

    KASSERT(runnable);
    KASSERT(g_current_thread);

    /* DEBUGF("switching from thread %d to thread %d\n", */
            /* g_current_thread->id, runnable->id); */
    switchToThread(runnable);
    //dumpThreadInfo(runnable);
}

/*
 * Start a kernel thread with a function to execute, an unsigned
 * integer argument to that function, its priority, and whether
 * it should be detached from the current running thread.
 */
thread_t* spawnThread(thread_startFunc_t startFunc, uint32_t arg,
        priority_t priority, bool detached, bool usermode) {
    KASSERT(startFunc);

    thread_t* thread = createThread(priority, detached, usermode);
    KASSERT(thread);    /* was thread created? */

    setupThreadStack(thread, startFunc, arg, usermode);

    makeRunnableAtomic(thread);

    return thread;
}


/*
 * Initialize the scheduler.
 *
 * Initializes the main kernel thread, the Idle thread,
 * and a Reaper thread for cleaning up dead threads.
 */
void schedulerInit(void) {
    extern uintptr_t mainThreadAddr, kernelStackBottom;
    thread_t* mainThread = (thread_t*)&mainThreadAddr;
    KASSERT(mainThread);

    initThread(mainThread, (void*)&kernelStackBottom,
            NULL, PRIORITY_NORMAL, true);
    g_current_thread = mainThread;
    allThreadsAdd(g_current_thread);

    spawnThread(idle, 0, PRIORITY_IDLE, true, false);

    spawnThread(reaper, 0, PRIORITY_NORMAL, true, false);
}


void dumpThreadInfo(thread_t* th) {
    KASSERT(th);
    DEBUGF("esp: 0x%X\n", th->esp);
    DEBUGF("numTicks: %u\n", th->numTicks);
    DEBUGF("user esp: 0x%X\n", th->userEsp);
    DEBUGF("sleepUntil: %u\n", th->sleepUntil);
    DEBUGF("queueNext: 0x%0X\n", th->queueNext);
    DEBUGF("listNext: 0x%0X\n", th->listNext);
}

/*
 * Dumps debugging data for each thread in the list of all threads
 */
void dumpAllThreadsList(void) {
    thread_t* thread;
    int count = 0;
    bool iFlag = begIntAtomic();

    thread = allThreadHead;

    kprintf("[");
    while (thread != NULL) {
        ++count;
        kprintf("<%x %x>", (uintptr_t)thread, (uintptr_t)thread->listNext);
        thread = thread->listNext;
    }
    kprintf("]\n");
    kprintf("%d threads are running\n", count);

    endIntAtomic(iFlag);
}

/*
 * Returns pointer to currently running thread
 */
thread_t* getCurrentThread(void) {
    return g_current_thread;
}

static void mutexWait(mutex_t *mutex) {
    KASSERT(mutex);
    KASSERT(mutex->locked);
    KASSERT(g_preemption_disabled);

    cli();
    /* re-nable preemption since we're about to wait */
    g_preemption_disabled = false;
    /* wait on the mutex's wait queue */
    wait(&mutex->waitQueue);
    /* back in action, so politely re-enable preemption */
    g_preemption_disabled = true;
    sti();
}

void mutexInit(mutex_t* mutex) {
    KASSERT(mutex);
    mutex->locked = false;
    mutex->owner = NULL;
    threadQueueClear(&mutex->waitQueue);
}

void mutexLock(mutex_t* mutex) {
    KASSERT(interruptsEnabled());
    KASSERT(mutex);

    disablePreemption();

    KASSERT(!mutexHeld(mutex));

    while (mutex->locked) {
        mutexWait(mutex);
    }

    mutex->locked = true;
    mutex->owner = g_current_thread;

    enablePreemption();
}

void mutex_unlock(mutex_t* mutex) {
    KASSERT(interruptsEnabled());
    KASSERT(mutex);

    disablePreemption();

    KASSERT(mutexHeld(mutex));

    mutex->locked = false;
    mutex->owner = NULL;

    if (!threadQueueEmpty(&mutex->waitQueue)) {
        cli();
        wakeOne(&mutex->waitQueue);
        sti();
    }

    enablePreemption();
}

bool mutexHeld(mutex_t* mutex) {
    KASSERT(mutex);
    if (mutex->locked && mutex->owner == g_current_thread) {
        return true;
    }
    return false;
}

void disablePreemption(void) {
    g_preemption_disabled = true;
}

void enablePreemption(void) {
    g_preemption_disabled = false;
}

bool preemptionEnabled(void) {
    return !g_preemption_disabled;
}
