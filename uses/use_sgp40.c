#ifdef MODULE_SGP40
#include "periph/i2c.h"
#include "ztimer.h"
#include "dlpp.h"
#include "airqual_common.h"

#include "sensirion_common.h"
#include "sgp40_i2c.h"

#include "use_sgp40.h"


static kernel_pid_t sender_pid;
static mutex_t *    sender_mutex;
static char         rcv_thread_stack[THREAD_STACKSIZE_MAIN/2];
static uint8_t      buffer[USE_SGP40_BUFFER_SIZE];
static mutex_t      buffer_mutex;
static ztimer_t     timer;
static mutex_t      timer_mutex = MUTEX_INIT_LOCKED;
static airmsg_t     airmsg;
static msg_t        msg;

static i2c_t i2c_dev;
static uint16_t sgp_temperature;
static uint16_t sgp_relative_humidity;
extern float g_temperature;
extern float g_relative_humidity;

static void _timer_cb(void *arg)
{
  ztimer_set(ZTIMER_SEC, &timer, USE_SGP40_INTERVAL_SECS);
  mutex_unlock(arg);
}

static void * sgp40_thread(void *arg)
{
  (void)arg;

  timer.callback = _timer_cb;
  timer.arg = &timer_mutex;
  ztimer_set(ZTIMER_SEC, &timer, USE_SGP40_INTERVAL_SECS);

  uint16_t sraw_voc = 0;
  int8_t ret=0;
  int8_t running=true;
  while(running)
  {
    mutex_lock(sender_mutex);
    sgp_temperature=(g_temperature + 45) * 65535 / 175;
    sgp_relative_humidity= g_relative_humidity * 65535 / 100;
    ret = sgp40_measure_raw_signal(i2c_dev, sgp_relative_humidity,
                                 sgp_temperature, &sraw_voc);
    if(ret==NO_ERROR)
    {
      printf("SGP40 sraw_voc %d\r\n",sraw_voc);
      mutex_lock(&buffer_mutex);
      dlpp(USE_SGP40_CHANNEL, buffer,'H',1, sraw_voc);
      mutex_unlock(&buffer_mutex);
      int retmsg = msg_try_send(&msg, sender_pid);
      if(retmsg != 1) printf("SGP40 sendfail %d", retmsg);
    }else if(ret == -ENXIO )
    {
      puts("SGP40 disconnected");
      running=false;
    }
    else{
      printf("SGP40 err  %d\r\n",ret);
    }
    mutex_unlock(sender_mutex);
    mutex_lock(&timer_mutex);
  }

  sgp40_turn_heater_off(i2c_dev);
  return NULL;
}

void init_use_sgp40(kernel_pid_t sender_pid_p, mutex_t * sender_mutex_p){

  i2c_dev = I2C_DEV(0);
  
  sender_pid=sender_pid_p;
  sender_mutex = sender_mutex_p;
  mutex_init(&buffer_mutex);
  airmsg.buffer=buffer;
  airmsg.mutex=&buffer_mutex;
  msg.content.ptr=&airmsg;
  
  int retval;
  uint16_t serial_number[3];
  retval = sgp40_get_serial_number(i2c_dev, serial_number);
  int8_t present=0;
  switch (retval) {
    case 0:
      present=1;
      break;
    case -ENXIO:
      /* No ACK --> no device */
      puts("SGP40 not connected");
      break;
    default:
      /* Some unexpected error */
      puts("Error I2C SGP40");
      break;
  }

  if(present)
  {
    puts("SGP40 start thread");
    thread_create(rcv_thread_stack, sizeof(rcv_thread_stack),
        USE_SGP40_THREAD_PRIORITY, 0,
        sgp40_thread, NULL, "sgp40_thread");
  }

}
#endif
