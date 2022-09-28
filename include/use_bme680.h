#ifndef USE_BME680_H
#define USE_BME680_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef USE_BME680_INTERVAL_SECS
#define USE_BME680_INTERVAL_SECS (30U)
#endif

#ifndef USE_BME680_THREAD_PRIORITY
#define USE_BME680_THREAD_PRIORITY (1U)
#endif

#ifndef USE_BME680_CHANNEL
#define USE_BME680_CHANNEL (1U)
#endif

#define USE_BME680_BUFFER_SIZE (6U)

void init_use_bme680(kernel_pid_t mainpid, mutex_t * sender_mutex);

#ifdef __cplusplus
}
#endif

#endif /* USE_BME680_H */
