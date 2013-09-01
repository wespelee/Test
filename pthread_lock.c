#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <error.h>
#include <errno.h>
#include <time.h>

void get_tms(struct timespec *tms)
{
    struct timespec ts;
    int rc;

    if (tms == NULL) {
        return;
    }

    rc = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (rc == 0) {
        tms->tv_sec = ts.tv_sec;
        tms->tv_nsec = ts.tv_nsec;
    }
    else {
        printf("clock_gettime failed, set timestamp to 0");
        tms->tv_sec = 0;
        tms->tv_nsec = 0;
    }
}      

#define NSECS_IN_SEC 1000000000
#define MAX_UINT32 4294967295U  
void get_delta(struct timespec *new, struct timespec *old, struct timespec *delta)
{
    if (new->tv_sec >= old->tv_sec)
    {
        if (new->tv_nsec >= old->tv_nsec)
        {
            /* no roll-over in the sec and nsec fields, straight subtract */
            delta->tv_nsec = new->tv_nsec - old->tv_nsec;
            delta->tv_sec = new->tv_sec - old->tv_sec;
        }
        else
        {
            /* no roll-over in the sec field, but roll-over in nsec field */
            delta->tv_nsec = (NSECS_IN_SEC - old->tv_nsec) + new->tv_nsec;
            delta->tv_sec = new->tv_sec - old->tv_sec - 1;
        }
    }
    else
    {
        if (new->tv_nsec >= old->tv_nsec)
        {
            /* roll-over in the sec field, but no roll-over in the nsec field */
            delta->tv_nsec = new->tv_nsec - old->tv_nsec;
            delta->tv_sec = (MAX_UINT32 - old->tv_sec) + new->tv_sec + 1; /* +1 to account for time spent during 0 sec */
        }
        else
        {
            /* roll-over in the sec and nsec fields */
            delta->tv_nsec = (NSECS_IN_SEC - old->tv_nsec) + new->tv_nsec;
            delta->tv_sec = (MAX_UINT32 - old->tv_sec) + new->tv_sec;                                                                                                          
        }
    }
}

typedef struct ticket_lock {
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    unsigned long queue_head, queue_tail;
} ticket_lock_t;

ticket_lock_t mutex_lock = {PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0, 0};

void ticket_lock(ticket_lock_t *ticket, char *locker)
{
    unsigned long queue_me;
    struct timespec start_ts, end_ts, delta_ts;

    get_tms(&start_ts);
    pthread_mutex_lock(&ticket->mutex);
    queue_me = ticket->queue_tail++;
    while (queue_me != ticket->queue_head)
    {
        pthread_cond_wait(&ticket->cond, &ticket->mutex);
    }
    pthread_mutex_unlock(&ticket->mutex);
    get_tms(&end_ts);
    get_delta(&end_ts, &start_ts, &delta_ts);
    printf("locker:%s %usec %umsec\n", locker, delta_ts.tv_sec, delta_ts.tv_nsec / 1000000);
}

void ticket_unlock(ticket_lock_t *ticket, char *unlocker)
{
    pthread_mutex_lock(&ticket->mutex);
    ticket->queue_head++;
    pthread_cond_broadcast(&ticket->cond);
    pthread_mutex_unlock(&ticket->mutex);
    printf("unlocker:%s\n", unlocker);
}

void *func1(void *p)
{
    struct timeval tv;
    int ret;

    while (1) {
        ticket_lock(&mutex_lock, __FUNCTION__);
        tv.tv_sec = 0;
        tv.tv_usec = 26000;

        do {
            ret = select(1, NULL, NULL, NULL, &tv);
        } while ((ret == -1) && (errno == EINTR));
        ticket_unlock(&mutex_lock, __FUNCTION__);
    }
    return NULL;
}

void *func2(void *p)
{
    struct timeval tv;
    int ret;

    while (1) {
        ticket_lock(&mutex_lock, __FUNCTION__);
        tv.tv_sec = 0;
        tv.tv_usec = 4000;

        do {
            ret = select(1, NULL, NULL, NULL, &tv);
        } while ((ret == -1) && (errno == EINTR));
        ticket_unlock(&mutex_lock, __FUNCTION__);
    }

    return NULL;
}

int main(int argc, const char *argv[])
{
    pthread_t tid1;
    pthread_t tid2;

    pthread_create(&tid1, NULL, func1, NULL);
    pthread_create(&tid2, NULL, func2, NULL);
    
    while (1) {
        sleep(1);
    }
    return 0;
}
