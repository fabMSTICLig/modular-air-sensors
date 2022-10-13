/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
*/

#ifdef MODULE_SEN5X
#include "periph/i2c.h"
#include "ztimer.h"
#include "mas_common.h"
#include "dlpp.h"

#include "sen5x_i2c.h"

#include "use_sen5x.h"

static kernel_pid_t sender_pid;
static mutex_t *    sender_mutex;
static char         rcv_thread_stack[THREAD_STACKSIZE_MAIN/2];
static uint8_t      buffer[USE_SEN5X_BUFFER_SIZE];
static mutex_t      buffer_mutex;
static ztimer_t     timer;
static mutex_t      timer_mutex = MUTEX_INIT_LOCKED;
static airmsg_t     airmsg;
static msg_t        msg;


static i2c_t i2c_dev;


static void _timer_cb(void *arg)
{
  ztimer_set(ZTIMER_SEC, &timer, USE_SEN5X_INTERVAL_SECS);
  mutex_unlock(arg);
}


static void * sen5x_thread(void *arg)
{
  (void)arg;


  sen5x_device_reset(i2c_dev);
  sen5x_start_measurement(i2c_dev);

  timer.callback = _timer_cb;
  timer.arg = &timer_mutex;
  ztimer_set(ZTIMER_SEC, &timer, USE_SEN5X_INTERVAL_SECS);

  uint16_t mass_concentration_pm1p0;
  uint16_t mass_concentration_pm2p5;
  uint16_t mass_concentration_pm4p0;
  uint16_t mass_concentration_pm10p0;
  int16_t ambient_humidity;
  int16_t ambient_temperature;
  int16_t voc_index;
  uint16_t voc_raw;
  int16_t nox_index;
  uint16_t nox_raw;
  int16_t error=0;
  int8_t running=true;
  while(running)
  {
    mutex_lock(sender_mutex);
    error = sen5x_read_measured_raw_values(i2c_dev, 
        &ambient_humidity, &ambient_temperature, &voc_raw, &nox_raw);
    error |= sen5x_read_measured_values(i2c_dev, 
        &mass_concentration_pm1p0, &mass_concentration_pm2p5,
        &mass_concentration_pm4p0, &mass_concentration_pm10p0,
        &ambient_humidity, &ambient_temperature, &voc_index, &nox_index);
    if(!error)
    {
      printf("SEN5X Mass concentration pm1p0: %.1f µg/m³\n",
          mass_concentration_pm1p0 / 10.0f);
      printf("SEN5X Mass concentration pm2p5: %.1f µg/m³\n",
          mass_concentration_pm2p5 / 10.0f);
      printf("SEN5X Mass concentration pm4p0: %.1f µg/m³\n",
          mass_concentration_pm4p0 / 10.0f);
      printf("SEN5X Mass concentration pm10p0: %.1f µg/m³\n",
          mass_concentration_pm10p0 / 10.0f);
      if (ambient_humidity == 0x7fff) {
        printf("SEN5X Ambient humidity: n/a\n");
      } else {
        printf("SEN5X Ambient humidity: %.1f %%RH\n",
            ambient_humidity / 100.0f);
      }
      if (ambient_temperature == 0x7fff) {
        printf("SEN5X Ambient temperature: n/a\n");
      } else {
        printf("SEN5X Ambient temperature: %.1f °C\n",
            ambient_temperature / 200.0f);
      }
      if (voc_index == 0x7fff) {
        printf("SEN5X Voc index: n/a\n");
      } else {
        printf("SEN5X Voc index: %.1f\n", voc_index / 10.0f);
      }
      if (nox_index == 0x7fff) {
        printf("SEN5X Nox index: n/a\n");
      } else {
        printf("SEN5X Nox index: %.1f\n", nox_index / 10.0f);
      }
      mutex_lock(&buffer_mutex);
      dlpp(USE_SEN5X_CHANNEL, buffer,'H',8, 
          (uint16_t)mass_concentration_pm1p0,
          (uint16_t)mass_concentration_pm2p5,
          (uint16_t)mass_concentration_pm4p0,
          (uint16_t)mass_concentration_pm10p0,
          (uint16_t)voc_index,
          (uint16_t)voc_raw,
          (uint16_t)nox_index,
          (uint16_t)nox_raw
          );
      mutex_unlock(&buffer_mutex);
      int retmsg = msg_try_send(&msg, sender_pid);
      if(retmsg != 1) printf("SEN5X sendfail %d", retmsg);
    }else if(error == -ENXIO )
    {
      puts("SEN5X disconnected");
      running=false;
    }
    else{
      printf("SEN5X err  %d\r\n",error);
    }
    mutex_unlock(sender_mutex);
    mutex_lock(&timer_mutex);

  }

  return NULL;
}

void init_use_sen5x(kernel_pid_t sender_pid_p, mutex_t * sender_mutex_p){

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
  while (-EAGAIN == (retval = i2c_read_byte(i2c_dev, SEN5X_I2C_ADDRESS, dummy, 0))) {
    /* retry until bus arbitration succeeds */
  }

  int8_t present=0;
  switch (retval) {
    case 0:
      present=1;
      break;
    case -ENXIO:
      /* No ACK --> no device */
      puts("SEN5X not connected");
      break;
    default:
      /* Some unexpected error */
      puts("Error I2C SEN5X");
      break;
  }
  i2c_release(i2c_dev);

  if(present)
  {
    puts("SEN5X start thread");
    thread_create(rcv_thread_stack, sizeof(rcv_thread_stack),
        USE_SEN5X_THREAD_PRIORITY, 0,
        sen5x_thread, NULL, "sen5x_thread");
  }

}
#endif
