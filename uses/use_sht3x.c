/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
*/

#ifdef MODULE_SHT3X
#include "periph/i2c.h"
#include "ztimer.h"
#include "dlpp.h"
#include "mas_common.h"

#include "sht3x.h"
#include "sht3x_params.h"

#include "use_sht3x.h"

static kernel_pid_t sender_pid;
static mutex_t *    sender_mutex;
static char         rcv_thread_stack[THREAD_STACKSIZE_MAIN/2];
static uint8_t      buffer[USE_SHT3X_BUFFER_SIZE];
static mutex_t      buffer_mutex;
static ztimer_t     timer;
static mutex_t      timer_mutex = MUTEX_INIT_LOCKED;
static airmsg_t     airmsg;
static msg_t        msg;

static sht3x_dev_t sht3x_dev;
extern float g_temperature;
extern float g_relative_humidity;

static void _timer_cb(void *arg)
{
  ztimer_set(ZTIMER_SEC, &timer, USE_SHT3X_INTERVAL_SECS);
  mutex_unlock(arg);
}

static void * sht3x_thread(void *arg)
{
  (void)arg;
  int8_t ret=0;
  int8_t running=true;

  if ((ret = sht3x_init(&sht3x_dev, &sht3x_params[0])) != SHT3X_OK) {
    puts("SHT3X Initialization failed\n");
    return NULL;
  }
  
  timer.callback = _timer_cb;
  timer.arg = &timer_mutex;
  ztimer_set(ZTIMER_SEC, &timer, USE_SHT3X_INTERVAL_SECS);
  
  int16_t temp;
  int16_t hum;
  while(running)
  {
    mutex_lock(sender_mutex);
    ret = sht3x_read(&sht3x_dev, &temp, &hum);
    if(ret==SHT3X_OK)
    {
      printf("SHT3X T: %d.%d RH: %d.%d\r\n",
          temp / 100, temp % 100,
          hum / 100, hum % 100);
      g_temperature=temp/100.f;
      g_relative_humidity= hum/100.f;
      mutex_lock(&buffer_mutex);
      dlpp(USE_SHT3X_CHANNEL, buffer,'h',2, temp, hum);
      mutex_unlock(&buffer_mutex);
      int retmsg = msg_try_send(&msg, sender_pid);
      if(retmsg != 1) printf("SHT3X sendfail %d", retmsg);
    }else if(ret == -ENXIO )
    {
      puts("SHT3X disconnected");
      running=false;
    }
    else{
      printf("SHT3X err  %d\r\n",ret);
    }
    mutex_unlock(sender_mutex);
    mutex_lock(&timer_mutex);
  }

  return NULL;
}

void init_use_sht3x(kernel_pid_t sender_pid_p, mutex_t * sender_mutex_p){

  sender_pid=sender_pid_p;
  sender_mutex = sender_mutex_p;
  mutex_init(&buffer_mutex);
  airmsg.buffer=buffer;
  airmsg.mutex=&buffer_mutex;
  msg.content.ptr=&airmsg;
  
  char dummy[1];
  int retval;
  i2c_acquire(sht3x_params[0].i2c_dev);
  while (-EAGAIN == (retval = i2c_read_byte(sht3x_params[0].i2c_dev, sht3x_params[0].i2c_addr, dummy, 0))) {
    /* retry until bus arbitration succeeds */
  }

  int8_t present=0;
  switch (retval) {
    case 0:
      present=1;
      break;
    case -ENXIO:
      /* No ACK --> no device */
      puts("SHT3X not connected");
      break;
    default:
      /* Some unexpected error */
      puts("Error I2C SHT3X");
      break;
  }
  i2c_release(sht3x_params[0].i2c_dev);

  if(present)
  {
    puts("SHT3X start thread");
    thread_create(rcv_thread_stack, sizeof(rcv_thread_stack),
        USE_SHT3X_THREAD_PRIORITY, 0,
        sht3x_thread, NULL, "sht3x_thread");
  }

}
#endif
