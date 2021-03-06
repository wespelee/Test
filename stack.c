#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <termios.h>
#include <linux/reboot.h>
#include <time.h>
#include <pthread.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */

#if 0
#define DUMP(func, call) \
    printf("%s: func = %p, called by = %p\n", __FUNCTION__, func, call)
#else
#define DUMP(func, call) {}
#endif

struct bd_frame_info {
    int max;
    int top;
    void *stack[1];
};

#define BDFI_MAX_DEPTH 128
#define BDFI_SIZE (sizeof(void *) * (BDFI_MAX_DEPTH - 1) + \
        sizeof(struct bd_frame_info))
#define SHOW_CORRUPTED_INFO 1
#define SHOW_THREAD_INFO 1

__thread struct bd_frame_info *__before_die_frame_info = NULL;

static void __attribute__((__no_instrument_function__))
__before_die_corrupted() {
#ifdef SHOW_CORRUPTED_INFO
    int i;
    struct bd_frame_info *bdfi;

    fprintf(stderr, "Stack is corrupted:\n");
    bdfi = __before_die_frame_info;
    for(i = bdfi->top - 1; i >= 0; i--) {
        fprintf(stderr, "\t%p\n", bdfi->stack[i]);
    }
#endif /* SHOW_CORRUPTED_INFO */
}

static void __attribute__((__no_instrument_function__))
__before_die_new_thread(struct bd_frame_info *bdfi) {
#ifdef SHOW_THREAD_INFO
    fprintf(stderr, "Thread ID = 0x%x, bdfi @ 0x%p pid=%d\n", pthread_self(), bdfi, syscall(SYS_gettid));
#endif /* SHOW_THREAD_INFO */
}

void __attribute__((__no_instrument_function__))
__cyg_profile_func_enter(void *this_fn, void *call_site) {
    struct bd_frame_info *bdfi;
    void *pparent;

    DUMP(this_fn, call_site);
    bdfi = __before_die_frame_info;
    if(bdfi == NULL) {
        /* First frame of this thread */
        bdfi = (struct bd_frame_info *)malloc(BDFI_SIZE);
        bdfi->max = BDFI_MAX_DEPTH;
        bdfi->top = 0;
        __before_die_frame_info = bdfi;
        __before_die_new_thread(bdfi);
    }

    if(bdfi->top >= bdfi->max)
        return;

    pparent = __builtin_return_address(2);
    if(bdfi->top > 0 && pparent != bdfi->stack[bdfi->top - 1]) {
        /* Corrupted stack? */
        __before_die_corrupted();
        return;
    }

    bdfi->stack[bdfi->top++] = call_site;
}

void __attribute__((__no_instrument_function__))
__cyg_profile_func_exit(void *this_fn, void *call_site) {
    struct bd_frame_info *bdfi;
    void *parent;

    DUMP(this_fn, call_site);
    bdfi = __before_die_frame_info;
    if(bdfi == NULL)
        return;

    parent = __builtin_return_address(1);
    if(parent != bdfi->stack[bdfi->top - 1]) {
        /* Corrupted stack? */
        __before_die_corrupted();
        return;
    }

    if(bdfi->top == 1) {
        /* Last frame of this thread */
        free(bdfi);
        __before_die_frame_info = NULL;
        return;
    }

    bdfi->top--;
}

/**************************************/
static inline void geo_get_tms(struct timespec *p)
{
    struct timespec ts;
    int ret;

    if (p == NULL) {
        return;
    }

    ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ret == 0) {
        p->tv_sec = ts.tv_sec;
        p->tv_nsec = ts.tv_nsec;
    }
    else {
        printf("clock_gettime failed, set timespec to 0");
        p->tv_sec = 0;
        p->tv_nsec = 0;
    }  
}

static inline time_t geo_get_tms_sec()
{
    struct timespec ts;
    geo_get_tms(&ts);

    return ts.tv_sec;
}

#define NSECS_PER_SEC       (1000000000)
#define TIMET_MAX           ((time_t)(~0U))
static inline void geo_get_delta_tms(const struct timespec *new_ts,
        const struct timespec *old_ts, struct timespec *delta)
{
    if (new_ts->tv_sec >= old_ts->tv_sec) {
        if (new_ts->tv_nsec >= old_ts->tv_nsec) {
            delta->tv_sec = new_ts->tv_sec - old_ts->tv_sec;
            delta->tv_nsec = new_ts->tv_nsec - old_ts->tv_nsec;
        }
        else {
            delta->tv_nsec = (NSECS_PER_SEC - old_ts->tv_nsec) + new_ts->tv_nsec;
            delta->tv_sec = new_ts->tv_sec - old_ts->tv_sec - 1;
        }
    }
    else {
        if (new_ts->tv_nsec >= old_ts->tv_nsec) {
            delta->tv_nsec = new_ts->tv_nsec - old_ts->tv_nsec;
            delta->tv_sec = (TIMET_MAX - old_ts->tv_sec) + new_ts->tv_sec + 1;
        }
        else {
            delta->tv_nsec = (NSECS_PER_SEC - old_ts->tv_nsec) + new_ts->tv_nsec;
            delta->tv_sec = (TIMET_MAX - old_ts->tv_sec) + new_ts->tv_sec;
        }
    }
}
void *loop_t(void *p)
{
    struct timespec start_tms, end_tms, delta_tms;
    struct timeval t;

    t.tv_sec = 0;
    t.tv_usec = 1;

    printf("tid=%d\n", syscall(SYS_gettid));
    printf("Start Test\n");
    geo_get_tms(&start_tms);
    /* Sleep */
    //select(0, NULL, NULL, NULL, &t);
    usleep(1000);

    geo_get_tms(&end_tms);
    geo_get_delta_tms(&end_tms, &start_tms, &delta_tms);
    printf("Sleep Spend time: %usec %umsec\n", delta_tms.tv_sec, 
                delta_tms.tv_nsec / 1000000);
}

int main(int argc, const char *argv[])
{
    pthread_t tid;
    pthread_attr_t attr;
    struct sched_param sched_param; 
    int err;

    pthread_attr_init(&attr);
	if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) {
        perror("pthread_attr_setinheritsched");
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
        perror("pthread_attr_setdetachstate");
    }
#if 1 
	if (pthread_attr_setschedpolicy(&attr, SCHED_RR)) {
        perror("pthread_attr_setschedpolicy");
    }
#endif
    sched_param.sched_priority = sched_get_priority_min(SCHED_RR);

    if (pthread_attr_setschedparam(&attr, &sched_param)) {
        perror("pthread_attr_setschedparam");
    }

    err = pthread_create(&tid, &attr, loop_t, NULL);
    if (err != 0) {
        printf("pthread_create: err=%d %s\n", err, strerror(errno));
        return 0;
    }

    pthread_join(tid, NULL);

    printf("main:%d\n", getpid());
    struct timespec start_tms, end_tms, delta_tms;
    geo_get_tms(&start_tms);
    /* Sleep */
    usleep(1000);

    geo_get_tms(&end_tms);
    geo_get_delta_tms(&end_tms, &start_tms, &delta_tms);
    printf("Sleep Spend time: %usec %umsec\n", delta_tms.tv_sec, 
                delta_tms.tv_nsec / 1000000);
    sleep(60);
    return 0;
}
