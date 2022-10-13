/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
*/

#ifdef MODULE_SFA3X
#include "periph/i2c.h"
#include "ztimer.h"
#include "mas_common.h"
#include "dlpp.h"

#include "sfa3x_i2c.h"

#include "use_sfa3x.h"

static kernel_pid_t sender_pid;
static mutex_t *    sender_mutex;
static char         rcv_thread_stack[THREAD_STACKSIZE_MAIN/2];
static uint8_t      buffer[USE_SFA3X_BUFFER_SIZE];
static mutex_t      buffer_mutex;
static ztimer_t     timer;
static mutex_t      timer_mutex = MUTEX_INIT_LOCKED;
static airmsg_t     airmsg;
static msg_t        msg;


static i2c_t i2c_dev;


static void _timer_cb(void *arg)
{
  ztimer_set(ZTIMER_SEC, &timer, USE_SFA3X_INTERVAL_SECS);
  mutex_unlock(arg);
}


static void * sfa3x_thread(void *arg)
{
  (void)arg;
  int16_t error=0;


  error=sfa3x_device_reset(i2c_dev);
  if(error)
  {
      printf("SFA3X reset err  %d\r\n",error);
  }
  error=sfa3x_start_continuous_measurement(i2c_dev);
  if(error)
  {
      printf("SFA3X start measurment err  %d\r\n",error);
  }


  ztimer_sleep(ZTIMER_MSEC,500);

  timer.callback = _timer_cb;
  timer.arg = &timer_mutex;
  ztimer_set(ZTIMER_SEC, &timer, USE_SFA3X_INTERVAL_SECS);

  int16_t hcho;
  int16_t humidity;
  int16_t temperature;
  int8_t running=true;
  while(running)
  {
    mutex_lock(sender_mutex);
    error = sfa3x_read_measured_values(i2c_dev, &hcho, &humidity, &temperature);
    if(!error)
    {
      printf("SFA3X hcho %f\r\n",hcho/5.0f);
      mutex_lock(&buffer_mutex);
      dlpp(USE_SFA3X_CHANNEL, buffer,'H',1, (uint16_t)(hcho*2));
      mutex_unlock(&buffer_mutex);
      int retmsg = msg_try_send(&msg, sender_pid);
      if(retmsg != 1) printf("SFA3X sendfail %d", retmsg);
    }
    /*else if(error == -ENXIO )
    {
      puts("SFA3X disconnected");
      running=false;
    }*/
    else{
      printf("SFA3X err  %d\r\n",error);
    }
    mutex_unlock(sender_mutex);
    mutex_lock(&timer_mutex);

  }

  return NULL;
}

void init_use_sfa3x(kernel_pid_t sender_pid_p, mutex_t * sender_mutex_p){

  i2c_dev = I2C_DEV(0);

  sender_pid=sender_pid_p;
  sender_mutex = sender_mutex_p;
  mutex_init(&buffer_mutex);
  airmsg.buffer=buffer;
  airmsg.mutex=&buffer_mutex;
  msg.content.ptr=&airmsg;

  char dummy[1];
  int retval;
  i2c_acquire(i2c_dev);
  while (-EAGAIN == (retval = i2c_read_byte(i2c_dev, SFA3X_I2C_ADDRESS, dummy, 0))) {
    /* retry until bus arbitration succeeds */
  }

  int8_t present=0;
  switch (retval) {
    case 0:
      present=1;
      break;
    case -ENXIO:
      /* No ACK --> no device */
      puts("SFA3X not connected");
      break;
    default:
      /* Some unexpected error */
      puts("Error I2C SFA3X");
      break;
  }
  i2c_release(i2c_dev);

  if(present)
  {
    puts("SFA3X start thread");
    thread_create(rcv_thread_stack, sizeof(rcv_thread_stack),
        USE_SFA3X_THREAD_PRIORITY, 0,
        sfa3x_thread, NULL, "sfa3x_thread");
  }

}
#endif
