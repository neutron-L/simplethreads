/* Simplethreads Instructional Thread Package
 *
 * sthread_user.c - Implements the sthread API using user-level threads.
 *
 *    You need to implement the routines in this file.
 *
 * Change Log:
 * 2002-04-15        rick
 *   - Initial version.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sthread.h>
#include <sthread_ctx.h>
#include <sthread_queue.h>
#include <sthread_user.h>

// 需要里面的一些错误码
#include <errno.h>
#include <signal.h>

#define SIG_JOIN 512
#define SIG_YIELD 513
#define SIG_EXIT 514

enum ThreadStatus {
    RUNNING,
    RUNNABLE,
    BLOCK,
    TERMINATE,
};

static sthread_queue_t ready_queue, terminate_queue;
static sthread_t running_thread;
static int next_id;

struct _sthread {
    sthread_ctx_t *saved_ctx;
    /* Add your fields to the thread data structure here */
    enum ThreadStatus status;
    int tid;
    sthread_start_func_t start_routine;
    void *arg;

    void *ret;
    int joinable;

    sthread_queue_t wait_queue;
};

/* 自定义的一些静态函数 */
static void schedule(int signum);
static void sthread_free(sthread_t thread);
static void sthread_stub(void);

/*********************************************************************/
/* Part 1: Creating and Scheduling Threads                           */
/*********************************************************************/

void sthread_user_init(void) {
    ready_queue = sthread_new_queue();
    terminate_queue = sthread_new_queue();
    running_thread = (sthread_t)calloc(1, sizeof(struct _sthread));
    running_thread->status = RUNNING;
    running_thread->tid = next_id++;
    running_thread->saved_ctx = sthread_new_blank_ctx();
}

sthread_t sthread_user_create(sthread_start_func_t start_routine, void *arg,
                              int joinable) {
    sthread_t thread = (sthread_t)malloc(sizeof(struct _sthread));
    thread->saved_ctx = sthread_new_ctx(sthread_stub);
    thread->status = RUNNABLE;
    thread->tid = next_id++;
    thread->start_routine = start_routine;
    thread->arg = arg;

    thread->ret = NULL;
    thread->joinable = joinable;
    thread->wait_queue = sthread_new_queue();

    sthread_enqueue(ready_queue, thread);

    return thread;
}

void sthread_user_exit(void *ret) {
    sthread_t wait_thread = NULL;
    // 检查join的线程
    while ((wait_thread = sthread_dequeue(running_thread->wait_queue))) {
        wait_thread->status = RUNNABLE;
        sthread_enqueue(ready_queue, wait_thread);
    }

    // 加入terminate队列等待资源释放
    running_thread->status = TERMINATE;
    running_thread->ret = ret;
    printf("%d exit\n", running_thread->tid);
    sthread_enqueue(terminate_queue, running_thread);
    schedule(SIG_EXIT);
}

void *sthread_user_join(sthread_t t) {
    if (t->tid == running_thread->tid) {
        return EDEADLK;
    } else if (!t->joinable) {
        return EINVAL;
    } 
    
    if (t->status != TERMINATE) {
        // 等待
        printf("%d join %d\n", running_thread->tid, t->tid);
        running_thread->status = BLOCK;
        sthread_enqueue(t->wait_queue, running_thread);
        schedule(SIG_JOIN);
    } 

    // 找到线程并回收它
    sthread_t thread;
    int size = sthread_queue_size(terminate_queue);
    while ((thread = sthread_dequeue(terminate_queue))->tid != t->tid && size-- > 0) {
        sthread_enqueue(terminate_queue, thread);
    }

    if (size <= 0) {
        printf("cannot find terminate thread %d\n", t->tid);
        exit(1);
    }
    void * ret = thread->ret;
    sthread_free(thread);
    return ret;
}

void sthread_user_yield(void) {
    // 调度
    running_thread->status = RUNNABLE;
    sthread_enqueue(ready_queue, running_thread);
    schedule(SIG_YIELD);
}

static void schedule(int signum) {
    // 默认头部的线程就是当前执行的线程（RUNNING）
    sthread_t thread;

    thread = sthread_dequeue(ready_queue);
    assert(thread->status == RUNNABLE);
    thread->status = RUNNING;
    if (signum == SIG_YIELD) {
        running_thread->status = RUNNABLE;
        sthread_enqueue(ready_queue, running_thread);
    }
    sthread_t temp = thread;
    thread = running_thread;
    running_thread = temp;
    sthread_switch(thread->saved_ctx, running_thread->saved_ctx);

    // while ((thread = sthread_dequeue(terminate_queue))) {
    //     sthread_free(thread);
    //     continue;
    // }
}

static void sthread_free(sthread_t thread) {
    sthread_free_ctx(thread->saved_ctx);
    sthread_free_queue(thread->wait_queue);
    free(thread);
}

static void sthread_stub(void) {
    void *ret = (void *)running_thread->start_routine(running_thread->arg);
    sthread_user_exit(ret);
}

/* Add any new part 1 functions here */

/*********************************************************************/
/* Part 2: Synchronization Primitives                                */
/*********************************************************************/

struct _sthread_mutex {
    /* Fill in mutex data structure */
};

sthread_mutex_t sthread_user_mutex_init() {
    return NULL;
}

void sthread_user_mutex_free(sthread_mutex_t lock) {
}

void sthread_user_mutex_lock(sthread_mutex_t lock) {
}

void sthread_user_mutex_unlock(sthread_mutex_t lock) {
}

struct _sthread_cond {
    /* Fill in condition variable structure */
};

sthread_cond_t sthread_user_cond_init(void) {
    return NULL;
}

void sthread_user_cond_free(sthread_cond_t cond) {
}

void sthread_user_cond_signal(sthread_cond_t cond) {
}

void sthread_user_cond_broadcast(sthread_cond_t cond) {
}

void sthread_user_cond_wait(sthread_cond_t cond,
                            sthread_mutex_t lock) {
}
