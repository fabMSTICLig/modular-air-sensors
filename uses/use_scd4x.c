/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
*/

#ifdef MODULE_SCD4X
#include "periph/i2c.h"
#include "ztimer.h"
#include "mas_common.h"
#include "dlpp.h"

#include "scd4x_i2c.h"

#include "use_scd4x.h"

static kernel_pid_t sender_pid;
static mutex_t *    sender_mutex;
static char         rcv_thread_stack[THREAD_STACKSIZE_MAIN/2];
static uint8_t      buffer[USE_SCD4X_BUFFER_SIZE];
static mutex_t      buffer_mutex;
static ztimer_t     timer;
static mutex_t      timer_mutex = MUTEX_INIT_LOCKED;
static airmsg_t     airmsg;
static msg_t        msg;


static i2c_t i2c_dev;


static void _timer_cb(void *arg)
{
  ztimer_set(ZTIMER_SEC, &timer, USE_SCD4X_INTERVAL_SECS);
  mutex_unlock(arg);
}


static void * scd4x_thread(void *arg)
{
  (void)arg;
  int16_t error=0;

  // Clean up potential SCD40 states
  scd4x_start_periodic_measurement(i2c_dev);

  timer.callback = _timer_cb;
  timer.arg = &timer_mutex;
  ztimer_set(ZTIMER_SEC, &timer, USE_SCD4X_INTERVAL_SECS);

  int8_t running=true;
  while(running)
  {
    mutex_lock(sender_mutex);
    bool data_ready_flag = false;
    error = scd4x_get_data_ready_flag(i2c_dev, &data_ready_flag);
    if(error == -ENXIO )
    {
      puts("SCD4X disconnected");
      running=false;
      mutex_unlock(sender_mutex);
      break;
    }
    else if (error) {
      printf("SCD4X Error executing scd4x_get_data_ready_flag(): %i\n", error);
    }
    else if (data_ready_flag) {

      uint16_t co2;
      int32_t temperature;
      int32_t humidity;
      error = scd4x_read_measurement(i2c_dev, &co2, &temperature, &humidity);
      if (error) {
        printf("SCD4X Error executing scd4x_read_measurement(): %i\n", error);
      } else if (co2 == 0) {
        printf("SCD4X Invalid sample detected, skipping.\n");
      } else {
        printf("SCD4X CO2: %u\n", co2);
        mutex_lock(&buffer_mutex);
        dlpp(USE_SCD4X_CHANNEL, buffer,'H',1, co2);
        mutex_unlock(&buffer_mutex);
        int retmsg = msg_try_send(&msg, sender_pid);
        if(retmsg != 1) printf("SCD4X sendfail %d", retmsg);

      }
    }
    mutex_unlock(sender_mutex);
    mutex_lock(&timer_mutex);

  }
  return NULL;
}

void init_use_scd4x(kernel_pid_t sender_pid_p, mutex_t * sender_mutex_p){

  i2c_dev = I2C_DEV(0);

  sender_pid=sender_pid_p;
  sender_mutex = sender_mutex_p;
  mutex_init(&buffer_mutex);
  airmsg.buffer=buffer;
  airmsg.mutex=&buffer_mutex;
  msg.content.ptr=&airmsg;

  int retval;
  scd4x_wake_up(i2c_dev);
  scd4x_stop_periodic_measurement(i2c_dev);
  retval = scd4x_reinit(i2c_dev);

  int8_t present=0;
  switch (retval) {
    case 0:
      present=1;
      break;
    case -ENXIO:
      /* No ACK --> no device */
      puts("SCD4X not connected");
      break;
    case -EIO:
      puts("SCD4X EIO");
      break;
    case -ETIMEDOUT:
      puts("SCD4X ETIMEDOUT");
      break;
    case -EINVAL:
      puts("SCD4X EINVAL");
      break;
    case -EOPNOTSUPP:
      puts("SCD4X EOPNOTSUPP");
      break;
    case -EAGAIN:
      puts("SCD4X EAGAIN");
      break;

    default:
      /* Some unexpected error */
      printf("Error I2C SCD4X %d\r\n", retval);
      break;
  }
  if(present)
  {
    puts("SCD4X start thread");
    thread_create(rcv_thread_stack, sizeof(rcv_thread_stack),
        USE_SCD4X_THREAD_PRIORITY, 0,
        scd4x_thread, NULL, "scd4x_thread");
  }

}
#endif
