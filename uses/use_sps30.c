#ifdef MODULE_SPS30
#include "periph/i2c.h"
#include "ztimer.h"
#include "dlpp.h"
#include "airqual_common.h"

#include "sps30.h"
#include "sps30_params.h"

#include "use_sps30.h"

#define SPS30_I2C_ADDR       (0x69)

#define TEST_START_DELAY_MS         (2)
#define SENSOR_RESET_DELAY_MS       (10)
#define SPS30_STARTUP_DELAY_MS     (10)
#define SENSOR_SLEEP_WAKE_DELAY_MS  (5)
#define POLL_FOR_READY_MS           (1)

static kernel_pid_t sender_pid;
static mutex_t *    sender_mutex;
static char         rcv_thread_stack[THREAD_STACKSIZE_MAIN/2];
static uint8_t      buffer[USE_SPS30_BUFFER_SIZE];
static mutex_t      buffer_mutex;
static ztimer_t     timer;
static mutex_t      timer_mutex = MUTEX_INIT_LOCKED;
static airmsg_t     airmsg;
static msg_t        msg;

static sps30_t sps30_dev;
static sps30_data_t data;

static void _timer_cb(void *arg)
{
  ztimer_set(ZTIMER_SEC, &timer, USE_SPS30_INTERVAL_SECS);
  mutex_unlock(arg);
}

static void * sps30_thread(void *arg)
{
  (void)arg;
  int8_t running=true;

  int ec;
  bool ready;
  ztimer_sleep(ZTIMER_MSEC,TEST_START_DELAY_MS);
  sps30_init(&sps30_dev, &sps30_params[0]);

  ec = sps30_start_measurement(&sps30_dev);
  ztimer_sleep(ZTIMER_MSEC, SPS30_STARTUP_DELAY_MS);

  timer.callback = _timer_cb;
  timer.arg = &timer_mutex;
  ztimer_set(ZTIMER_SEC, &timer, USE_SPS30_INTERVAL_SECS);
  
  puts("SPS30 Start Mesurment");
  while(running)
  {
    mutex_lock(sender_mutex);
    ready = sps30_data_ready(&sps30_dev, &ec);
    if(ready)
    {
      ec = sps30_read_measurement(&sps30_dev, &data);
      if(ec==SPS30_OK) 
      {
        printf("SPS30 %d %d %d %d\r\n",
            (uint16_t)(data.mc_pm1*1000), 
            (uint16_t)(data.mc_pm2_5*1000), 
            (uint16_t)(data.mc_pm4*1000), 
            (uint16_t)(data.mc_pm10*1000));
        mutex_lock(&buffer_mutex);
        dlpp(USE_SPS30_CHANNEL, buffer,'H',4, 
            (uint16_t)(data.mc_pm1*1000), 
            (uint16_t)(data.mc_pm2_5*1000), 
            (uint16_t)(data.mc_pm4*1000), 
            (uint16_t)(data.mc_pm10*1000));
        mutex_unlock(&buffer_mutex);
        int retmsg = msg_try_send(&msg, sender_pid);
        if(retmsg != 1) printf("SPS30 sendfail %d", retmsg);
      }
    }
    if(ec == SPS30_I2C_ERROR )
    {
      puts("SPS30 disconnected");
      running=false;
    }
    mutex_unlock(sender_mutex);
    mutex_lock(&timer_mutex);
  }
  ec = sps30_stop_measurement(&sps30_dev);

  return NULL;
}

void init_use_sps30(kernel_pid_t sender_pid_p, mutex_t * sender_mutex_p){

  sender_pid=sender_pid_p;
  sender_mutex = sender_mutex_p;
  mutex_init(&buffer_mutex);
  airmsg.buffer=buffer;
  airmsg.mutex=&buffer_mutex;
  msg.content.ptr=&airmsg;
  char dummy[1];
  int retval;
  i2c_acquire(sps30_params->i2c_dev);
  while (-EAGAIN == (retval = i2c_read_byte(sps30_params->i2c_dev, SPS30_I2C_ADDR, dummy, 0))) {
    /* retry until bus arbitration succeeds */
  }

  int8_t present=0;
  switch (retval) {
    case 0:
      present=1;
      break;
    case -ENXIO:
      /* No ACK --> no device */
      puts("SPS30 not connected");
      break;
    default:
      /* Some unexpected error */
      puts("Error I2C SPS30");
      break;
  }
  i2c_release(sps30_params->i2c_dev);

  if(present)
  {
    puts("SPS30 start thread");
    thread_create(rcv_thread_stack, sizeof(rcv_thread_stack),
        USE_SPS30_THREAD_PRIORITY, 0,
        sps30_thread, NULL, "sps30_thread");
  }

}
#endif
