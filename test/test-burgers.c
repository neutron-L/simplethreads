/*
 * test-mutex.c - Simple test of mutexes. Checks that they do, in fact,
 *                provide mutual exclusion.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sthread.h>

#define STACK_SIZE 32

static int N, M, burgers;
static int next_burger;
static int done;
static sthread_mutex_t mutex;
static sthread_cond_t not_empty;
static sthread_cond_t not_full;

static int stack[STACK_SIZE];
static int top;

static void *thread_cook(void *arg);
static void *thread_student(void *arg);

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("usage: %s <cooks> <students> <burgers>\n", argv[0]);
        return 0;
    }
    printf("Testing sthread_mutex_*, impl: %s\n",
           (sthread_get_impl() == STHREAD_PTHREAD_IMPL) ? "pthread" : "user");
    N = atoi(argv[1]);
    M = atoi(argv[2]);
    burgers = atoi(argv[4]);

    sthread_init();

    mutex = sthread_mutex_init();
    not_empty = sthread_cond_init();
    not_full = sthread_cond_init();

    sthread_t cooks[N];
    sthread_t students[M];

    for (int i = 0; i < N; ++i) {
        if ((cooks[i] = sthread_create(thread_cook, NULL, 0)) == NULL) {
            printf("sthread_create failed\n");
            exit(1);
        }
    }

    for (int i = 0; i < M; ++i) {
        if ((students[i] = sthread_create(thread_student, NULL, 0)) == NULL) {
            printf("sthread_create failed\n");
            exit(1);
        }
    }

    for (int i = 0; i < N; ++i) {
        sthread_join(cooks[i]);
    }

    for (int i = 0; i < M; ++i) {
        sthread_join(students[i]);
    }

    return 0;
}

static void *thread_cook(void *arg) {
    (void)arg;

    int done_ = 0;

    while (!done_) {
        sthread_mutex_lock(mutex);
        while (top == STACK_SIZE && !done) {
            sthread_cond_wait(not_full, mutex);
        }
        if (done) {
            done_ = done;
            sthread_cond_broadcast(not_empty);
        } else {
            printf("cook burger [%d]\n", next_burger);
            stack[top++] = next_burger++;
            if (next_burger == burgers) {
                done = 1;
                done_ = done;
                sthread_cond_broadcast(not_empty);
            } else {
                sthread_cond_signal(not_empty);
            }
        }
        sthread_mutex_unlock(mutex);

        sthread_yield();
    }
}

static void *thread_student(void *arg) {
    (void)arg;

    int done_ = 0;

    while (!done_) {
        sthread_mutex_lock(mutex);
        while (top == 0 && !done) {
            sthread_cond_wait(not_empty, mutex);
        }
        if (done) {
            done_ = done;
            sthread_cond_broadcast(not_full);
        } else {
            printf("cook burger [%d]\n", next_burger);
            stack[top++] = next_burger++;
            if (next_burger == burgers) {
                done = 1;
                done_ = done;
                sthread_cond_broadcast(not_full);
            } else {
                sthread_cond_signal(not_full);
            }
        }
        sthread_mutex_unlock(mutex);
        sthread_yield();
    }
}