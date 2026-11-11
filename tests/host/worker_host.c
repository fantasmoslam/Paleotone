/* SPDX-License-Identifier: GPL-3.0-only */
/* Host-only worker double: real concurrent execution, delayed start and one
 * partially executed failed job. No mutex or allocation is used on the device. */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "amb.h"
static pthread_t th;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wake=PTHREAD_COND_INITIALIZER,finished=PTHREAD_COND_INITIALIZER;
static void (*job)(uint32_t);
static uint32_t arg;
static int due,done,mode,created,failed;
static unsigned calls;
static void *loop(void *unused)
{
    (void)unused;
    pthread_mutex_lock(&lock);
    for(;;){
        void (*fn)(uint32_t);uint32_t a;
        while(!due)pthread_cond_wait(&wake,&lock);
        fn=job;a=arg;due=0;
        pthread_mutex_unlock(&lock);
        if(mode==3 && !failed){struct timespec delay={0,50000};nanosleep(&delay,NULL);a=(a&0xffff0000u)|((a&65535u)/2u);}
        fn(a);
        pthread_mutex_lock(&lock);done=1;pthread_cond_signal(&finished);
    }
    return NULL;
}
static void report(void){fprintf(stderr,"worker mode %d: %u jobs, %u fallbacks\n",mode,calls,(unsigned)en_worker_faults);}
__attribute__((constructor)) static void setup(void)
{
    const char *s=getenv("WORKER_MODE");
    mode=s&&!strcmp(s,"sync")?1:s&&!strcmp(s,"async")?2:s&&!strcmp(s,"fail")?3:0;
    en_worker_on=(uint8_t)(mode!=0);atexit(report);
}
int en_worker_start(void (*fn)(uint32_t),uint32_t a)
{
    if(!mode)return 0;
    calls++;
    if(mode==1){fn(a);return 1;}
    if(!created){if(pthread_create(&th,NULL,loop,NULL))abort();created=1;}
    pthread_mutex_lock(&lock);job=fn;arg=a;done=0;due=1;pthread_cond_signal(&wake);pthread_mutex_unlock(&lock);
    return 1;
}
int en_worker_wait(void)
{
    if(mode==1)return 0;
    pthread_mutex_lock(&lock);while(!done)pthread_cond_wait(&finished,&lock);pthread_mutex_unlock(&lock);
    if(mode==3&&!failed){failed=1;en_worker_on=0;en_worker_faults++;return -1;}
    return 0;
}
