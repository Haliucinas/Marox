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

#ifndef MAROX_THREAD_H
#define MAROX_THREAD_H

#include "marox.h"

/* forward declaration for now */
struct user_context;

/* thread-local data */
enum { MAX_TLOCAL_KEYS = 128 };
typedef void (*tlocal_destructor_t)(void *);
typedef unsigned int tlocal_key_t;

/* global quantum (number of ticks before current thread yields) */
enum { THREAD_QUANTUM = 4 };

enum priority {
    PRIORITY_IDLE = 0,
    PRIORITY_USER = 1,
    PRIORITY_LOW  = 2,
    PRIORITY_NORMAL = 5,
    PRIORITY_HIGH = 10
};
typedef enum priority priority_t;

/* thread queues/lists */
struct thread_queue {
    struct thread* head;
    struct thread* tail;
};
typedef struct thread_queue thread_queue_t;


/* kernel thread definition */
struct thread {
    uint32_t esp;
    volatile uint32_t numTicks;
    uint32_t userEsp;
    uint32_t stackTop;

    priority_t priority;

    void* stackBase;
    void* userStackBase;
    struct thread* owner;
    int refCount;

    /* sleep */
    uint32_t sleepUntil;

    /* join()-related members */
    bool alive;
    struct thread_queue joinQueue;
    int exitCode;

    /* kernel thread ID and process ID */
    unsigned int id;

    /* link to next thread in current queue */
    struct thread* queueNext;

    /* link to all threads in system */
    struct thread* listNext;

    /* array of pointers to thread-local data */
    const void* tlocalData[MAX_TLOCAL_KEYS];
};
typedef struct thread thread_t;

/* Thread start functions must match this signature. */
typedef void (*thread_startFunc_t)(uint32_t arg);

struct mutex {
    bool locked;
    thread_t* owner;
    thread_queue_t waitQueue;
};
typedef struct mutex mutex_t;


int join(thread_t* thread);
void sleep(unsigned int milliseconds);
void yield(void);
//void exit(int exitCode) __attribute__ ((noreturn));
void exit(int exitCode);

void wait(thread_queue_t* waitQueue);
void makeRunnable(thread_t* thread);
void makeRunnableAtomic(thread_t* thread);

thread_t* spawnThread(thread_startFunc_t startFunction, uint32_t arg, priority_t priority, bool detached, bool usermode);

void schedule(void);
void schedulerInit();

void dumpThreadInfo(thread_t*);
void dumpAllThreadsList(void);

void mutexInit(mutex_t* mutex);
void mutexLock(mutex_t* mutex);
void mutex_unlock(mutex_t* mutex);
bool mutexHeld(mutex_t* mutex);

void threadQueueClear(thread_queue_t* queue);
bool threadQueueEmpty(thread_queue_t* queue);
void wakeAll(thread_queue_t* waitQueue);
void wakeOne(thread_queue_t* waitQueue);

thread_t* getCurrentThread(void);

void disablePreemption(void);
void enablePreemption(void);
bool preemptionEnabled(void);

/* Thread-local data functions */
bool tlocalCreate(tlocal_key_t* key, tlocal_destructor_t destructor);
void tlocalSet(tlocal_key_t key, const void* data);
void* tlocalGet(tlocal_key_t key);

#endif /* MAROX_THREAD_H */
