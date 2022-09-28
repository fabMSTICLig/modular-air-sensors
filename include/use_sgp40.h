
#ifndef USE_SGP40_H
#define USE_SGP40_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef USE_SGP40_INTERVAL_SECS
#define USE_SGP40_INTERVAL_SECS (30U)
#endif

#ifndef USE_SGP40_THREAD_PRIORITY
#define USE_SGP40_THREAD_PRIORITY (3U)
#endif

#ifndef USE_SGP40_CHANNEL
#define USE_SGP40_CHANNEL (3U)
#endif

#define USE_SGP40_BUFFER_SIZE (4U)

void init_use_sgp40(kernel_pid_t senderpid, mutex_t * sender_mutex);

#ifdef __cplusplus
}
#endif

#endif /* USE_SGP40_H */
