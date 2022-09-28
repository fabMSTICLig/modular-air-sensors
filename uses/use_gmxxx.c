/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
*/

#ifdef MODULE_DRIVER_GMXXX
#include "periph/i2c.h"
#include "ztimer.h"
#include "dlpp.h"
#include "airqual_common.h"

#include "gmxxx.h"
#include "gmxxx_params.h"

#include "use_gmxxx.h"

static kernel_pid_t sender_pid;
static mutex_t *    sender_mutex;
static char         rcv_thread_stack[THREAD_STACKSIZE_MAIN/2];
static uint8_t      buffer[USE_GMXXX_BUFFER_SIZE];
static mutex_t      buffer_mutex;
static ztimer_t     timer;
static mutex_t      timer_mutex = MUTEX_INIT_LOCKED;
static airmsg_t     airmsg;
static msg_t        msg;

static gmxxx_dev_t gmxxx_dev;

static void _timer_cb(void *arg)
{
  ztimer_set(ZTIMER_SEC, &timer, USE_GMXXX_INTERVAL_SECS);
  mutex_unlock(arg);
}

static void * gmxxx_thread(void *arg)
{
  (void)arg;
  int8_t ret=0;
  int8_t running=true;

  if ((ret = gmxxx_init(&gmxxx_dev, &gmxxx_params[0])) != GMXXX_OK) {
    puts("GMXXX Initialization failed\n");
    return NULL;
  }
  
  timer.callback = _timer_cb;
  timer.arg = &timer_mutex;
  ztimer_set(ZTIMER_SEC, &timer, USE_GMXXX_INTERVAL_SECS);
  
  uint32_t gm102b;
  uint32_t gm302b;
  uint32_t gm502b;
  uint32_t gm702b;
  while(running)
  {
    mutex_lock(sender_mutex);
    if ((ret = gmxxx_get_GM102B(&gmxxx_dev, &gm102b)) == GMXXX_OK) {
         printf("GM102B: %ld\n",gm102b);
    }
    if ((ret |= gmxxx_get_GM302B(&gmxxx_dev, &gm302b)) == GMXXX_OK) {
         printf("GM302B: %ld\n",gm302b);
    }
    if ((ret |= gmxxx_get_GM502B(&gmxxx_dev, &gm502b)) == GMXXX_OK) {
         printf("GM502B: %ld\n",gm502b);
    }
    if ((ret |= gmxxx_get_GM702B(&gmxxx_dev, &gm702b)) == GMXXX_OK) {
         printf("GM702B: %ld\n",gm702b);
    }
    if(ret==GMXXX_OK)
    {
      mutex_lock(&buffer_mutex);
      dlpp(USE_GMXXX_CHANNEL, buffer,'H',4, (uint16_t)gm102b, (uint16_t)gm302b,(uint16_t)gm502b,(uint16_t)gm702b);
      mutex_unlock(&buffer_mutex);
      int retmsg = msg_try_send(&msg, sender_pid);
      if(retmsg != 1) printf("GMXXX sendfail %d", retmsg);
    }else if(ret == -ENXIO )
    {
      puts("GMXXX disconnected");
      running=false;
    }
    else{
      printf("GMXXX err  %d\r\n",ret);
    }
    mutex_unlock(sender_mutex);
    mutex_lock(&timer_mutex);
  }

  return NULL;
}

void init_use_gmxxx(kernel_pid_t sender_pid_p, mutex_t * sender_mutex_p){

  sender_pid=sender_pid_p;
  sender_mutex = sender_mutex_p;
  mutex_init(&buffer_mutex);
  airmsg.buffer=buffer;
  airmsg.mutex=&buffer_mutex;
  msg.content.ptr=&airmsg;
  
  char dummy[1];
  int retval;
  i2c_acquire(gmxxx_params[0].i2c_dev);
  while (-EAGAIN == (retval = i2c_read_byte(gmxxx_params[0].i2c_dev, gmxxx_params[0].i2c_addr, dummy, 0))) {
    /* retry until bus arbitration succeeds */
  }

  int8_t present=0;
  switch (retval) {
    case 0:
      present=1;
      break;
    case -ENXIO:
      /* No ACK --> no device */
      puts("GMXXX not connected");
      break;
    default:
      /* Some unexpected error */
      puts("Error I2C GMXXX");
      break;
  }
  i2c_release(gmxxx_params[0].i2c_dev);

  if(present)
  {
    puts("GMXXX start thread");
    thread_create(rcv_thread_stack, sizeof(rcv_thread_stack),
        USE_GMXXX_THREAD_PRIORITY, 0,
        gmxxx_thread, NULL, "gmxxx_thread");
  }

}
#endif
