/* wasm: one thread (cpu1_host.c CORES=1) */
typedef int pthread_t, pthread_mutex_t, pthread_cond_t;
#define pthread_mutex_lock(m) ((void)(m), 0)
#define pthread_mutex_unlock(m) ((void)(m), 0)
#define pthread_cond_wait(c, m) ((void)(c), (void)(m), 0)
#define pthread_cond_signal(c) ((void)(c), 0)
#define pthread_mutex_init(m, a) ((void)(m), 0)
#define pthread_cond_init(c, a) ((void)(c), 0)
#define pthread_create(t, a, f, p) ((void)(t), (void)(f), (void)(p), -1)
#define pthread_join(t, r) ((void)(t), 0)
