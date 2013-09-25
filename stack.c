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
//#include <pthread.h>

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

    printf("Start Test\n");
    geo_get_tms(&start_tms);
    /* Sleep */
    //select(0, NULL, NULL, NULL, &t);
    usleep(1);

    geo_get_tms(&end_tms);
    geo_get_delta_tms(&end_tms, &start_tms, &delta_tms);
    printf("Sleep Spend time: %usec %umsec\n", delta_tms.tv_sec, 
                delta_tms.tv_nsec / 1000000);
}

int main(int argc, const char *argv[])
{
#if 0
    pthread_t tid;
    pthread_attr_t attr;
    int err;


    pthread_attr_init(&attr);
	if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) {
        perror("pthread_attr_setinheritsched");
    }
    if (pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED)) {
        perror("pthread_attr_setdetachstate");
    }
	if (pthread_attr_setschedpolicy(&attr, SCHED_RR)) {
        perror("pthread_attr_setschedpolicy");
    }

    err = pthread_create(&tid, &attr, loop_t, NULL);
    if (err != 0) {
        printf("pthread_create: err=%d %s\n", err, strerror(errno));
    }

    pthread_join(tid, NULL);
#endif

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
