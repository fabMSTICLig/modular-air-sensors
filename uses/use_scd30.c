/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
*/

#ifdef MODULE_SCD30
#include "periph/i2c.h"
#include "ztimer.h"
#include "airqual_common.h"
#include "dlpp.h"

#include "scd30.h"
#include "scd30_params.h"
#include "scd30_internal.h"

#include "use_scd30.h"

static kernel_pid_t sender_pid;
static mutex_t *    sender_mutex;
static char         rcv_thread_stack[THREAD_STACKSIZE_MAIN/2];
static uint8_t      buffer[USE_SCD30_BUFFER_SIZE];
static mutex_t      buffer_mutex;
static ztimer_t     timer;
static mutex_t      timer_mutex = MUTEX_INIT_LOCKED;
static airmsg_t     airmsg;
static msg_t        msg;

#ifdef USE_SCD30_TH_ON
static uint8_t      buffer_th[USE_SCD30_TH_BUFFER_SIZE];
static mutex_t      buffer_th_mutex;
static airmsg_t     airmsg_th;
static msg_t        msg_th;

#endif

static scd30_t scd30_dev;
static scd30_params_t params = SCD30_PARAMS;
static scd30_measurement_t result;



static void _timer_cb(void *arg)
{
  ztimer_set(ZTIMER_SEC, &timer, USE_SCD30_INTERVAL_SECS);
  mutex_unlock(arg);
}


static void * scd30_thread(void *arg)
{
  (void)arg;


  scd30_init(&scd30_dev, &params);
  uint16_t pressure_compensation = SCD30_DEF_PRESSURE;
  uint16_t interval = USE_SCD30_INTERVAL_SECS;
  scd30_set_param(&scd30_dev, SCD30_INTERVAL, USE_SCD30_INTERVAL_SECS);
  scd30_set_param(&scd30_dev, SCD30_START, pressure_compensation);
  uint16_t value = 0;
  scd30_get_param(&scd30_dev, SCD30_INTERVAL, &value);
  printf("[test]Interval: %u s\n", value);
  scd30_start_periodic_measurement(&scd30_dev, &interval,
      &pressure_compensation);

  timer.callback = _timer_cb;
  timer.arg = &timer_mutex;
  ztimer_set(ZTIMER_SEC, &timer, USE_SCD30_INTERVAL_SECS);

  int8_t ret=0;
  int8_t running=true;
  while(running)
  {
    mutex_lock(sender_mutex);
    ret = scd30_read_periodic(&scd30_dev, &result);
    if(ret==SCD30_OK)
    {
      printf("SCD30 co2_concentration %d\r\n",(uint16_t)result.co2_concentration);
      mutex_lock(&buffer_mutex);
      dlpp(USE_SCD30_CHANNEL, buffer,'H',1, (uint16_t)result.co2_concentration);
      mutex_unlock(&buffer_mutex);
      int retmsg = msg_try_send(&msg, sender_pid);
      if(retmsg != 1) printf("SCD30 sendfail %d", retmsg);
#ifdef USE_SCD30_TH_ON
      printf("SCD30 temp %d, hum %d\r\n",(int16_t)(result.temperature*10), (int16_t)(result.relative_humidity*10));
      mutex_lock(&buffer_th_mutex);
      dlpp(USE_SCD30_TH_CHANNEL, buffer_th,'h',2, (int16_t)(result.temperature*10), (int16_t)(result.relative_humidity*10));
      mutex_unlock(&buffer_th_mutex);
      retmsg = msg_try_send(&msg_th, sender_pid);
      if(retmsg != 1) printf("SCD30 TH sendfail %d", retmsg);
#endif
    }else if(ret == -ENXIO )
    {
      puts("SCD30 disconnected");
      running=false;
    }
    else{
      printf("SCD30 err  %d\r\n",ret);
    }
    mutex_unlock(sender_mutex);
    mutex_lock(&timer_mutex);

  }

  return NULL;
}

void init_use_scd30(kernel_pid_t sender_pid_p, mutex_t * sender_mutex_p){

  sender_pid=sender_pid_p;
  sender_mutex = sender_mutex_p;
  mutex_init(&buffer_mutex);
  airmsg.buffer=buffer;
  airmsg.mutex=&buffer_mutex;
  msg.content.ptr=&airmsg;
  
#ifdef USE_SCD30_TH_ON
  mutex_init(&buffer_th_mutex);
  airmsg_th.buffer=buffer_th;
  airmsg_th.mutex=&buffer_th_mutex;
  msg_th.content.ptr=&airmsg_th;
#endif

  char dummy[1];
  int retval;
  i2c_acquire(params.i2c_dev);
  while (-EAGAIN == (retval = i2c_read_byte(params.i2c_dev, params.i2c_addr, dummy, 0))) {
    /* retry until bus arbitration succeeds */
  }

  int8_t present=0;
  switch (retval) {
    case 0:
      present=1;
      break;
    case -ENXIO:
      /* No ACK --> no device */
      puts("SCD30 not connected");
      break;
    default:
      /* Some unexpected error */
      puts("Error I2C SCD30");
      break;
  }
  i2c_release(params.i2c_dev);

  if(present)
  {
    puts("SCD30 start thread");
    thread_create(rcv_thread_stack, sizeof(rcv_thread_stack),
        USE_SCD30_THREAD_PRIORITY, 0,
        scd30_thread, NULL, "scd30_thread");
  }

}
#endif
