/*
 * Copyright (C) 2022 LIG Laboratoire Informatique de Grenoble
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
*/

/**
 * @{
 *
 * @file
 * @brief       modular air sensor board
 *
 * @author      Germain Lemasson <germain.lemasson@univ-grenoble-alpes.fr>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "board.h"
#include "msg.h"
#include "thread.h"
#include "fmt.h"
#include "ztimer.h"

#include "net/loramac.h"
#include "semtech_loramac.h"

#include "sx126x.h"
#include "sx126x_netdev.h"
#include "sx126x_params.h"

#include "dlpp.h"
#include "airqual_common.h"


#define LEDON_PIN GPIO_PIN(PORT_B, 4)

#ifdef MODULE_SCD30
#include "use_scd30.h"
#endif
#ifdef MODULE_SCD4X
#include "use_scd4x.h"
#endif
#ifdef MODULE_SPS30
#include "use_sps30.h"
#endif
#ifdef MODULE_SHT3X
#include "use_sht3x.h"
#endif
#ifdef MODULE_SFA3X
#include "use_sfa3x.h"
#endif
#ifdef MODULE_SEN5X
#include "use_sen5x.h"
#endif
#ifdef MODULE_SGP40
#include "use_sgp40.h"
#endif
#ifdef MODULE_BME680
#include "use_bme680.h"
#endif
#ifdef MODULE_SVM40
#include "use_svm40.h"
#endif
#ifdef MODULE_DRIVER_GMXXX
#include "use_gmxxx.h"
#endif

#ifndef LORAMAC_BUFFER_SIZE
#define LORAMAC_BUFFER_SIZE            (50U)
#endif
#ifndef SENDER_INTERVAL_SECS
#define SENDER_INTERVAL_SECS            (30U)
#endif

#define SENDER_PRIORITY         (1U)

static kernel_pid_t sender_pid;
static kernel_pid_t main_pid;
static char sender_stack[THREAD_STACKSIZE_MAIN / 2];
static msg_t msg;
static ztimer_t     timer;
static mutex_t      timer_mutex = MUTEX_INIT_LOCKED;
static mutex_t      sender_mutex;

float g_temperature=25.0f;
float g_relative_humidity=50.0f;

extern semtech_loramac_t loramac;
static char print_buf[LORAMAC_APPKEY_LEN * 2 + 1];
static uint8_t deveui[LORAMAC_DEVEUI_LEN];
static uint8_t appeui[LORAMAC_APPEUI_LEN];
static uint8_t appkey[LORAMAC_APPKEY_LEN];

static uint8_t sender_buffer[LORAMAC_BUFFER_SIZE];
static uint8_t sender_cursor=0;

static void _timer_cb(void *arg)
{
  ztimer_set(ZTIMER_SEC, &timer, SENDER_INTERVAL_SECS);
  mutex_unlock(arg);
}

static void * sender_thread(void *arg)
{
  (void)arg;
  msg_t msgmain;
  int8_t fill=1;
  uint8_t size;
  msg_t msg_queue[8];
  msg_init_queue(msg_queue, 8);

  ztimer_sleep(ZTIMER_SEC,SENDER_INTERVAL_SECS/2);
  
  timer.callback = _timer_cb;
  timer.arg = &timer_mutex;
  ztimer_set(ZTIMER_SEC, &timer, SENDER_INTERVAL_SECS);

  //empty buffer
  while(msg_try_receive(&msg)==1);

  while(1)
  {
    mutex_lock(&sender_mutex);
    sender_cursor=0;
    fill=1;
    while(fill)
    {
      if(msg_try_receive(&msg)==1)
      {
        mutex_lock(MSG_MUTEX(msg));
        size = dlpp_get_size(MSG_BUFFER(msg));
        if(sender_cursor+size<LORAMAC_BUFFER_SIZE)
        {
          memcpy( sender_buffer+sender_cursor, MSG_BUFFER(msg), size );
          sender_cursor+=size;
        }
        else
        {
          fill=0;
        }
        mutex_unlock(MSG_MUTEX(msg));
        if(fill==0){
          //empty buffer
          while(msg_try_receive(&msg)==1);
        }
      }
      else
      {
        fill=0;
      }
    }
    if(sender_cursor)
    {
      msg_try_send(&msgmain, main_pid);
    }
    mutex_unlock(&sender_mutex);
    mutex_lock(&timer_mutex);
  }

  return NULL;
}


int main(void)
{
  gpio_init(LED0_PIN, GPIO_OUT);
  gpio_init(LEDON_PIN, GPIO_OUT);
  gpio_clear(LED0_PIN);
  gpio_set(LEDON_PIN);
  fmt_hex_bytes(deveui, CONFIG_LORAMAC_DEV_EUI_DEFAULT);
  fmt_hex_bytes(appeui, CONFIG_LORAMAC_APP_EUI_DEFAULT);
  fmt_hex_bytes(appkey, CONFIG_LORAMAC_APP_KEY_DEFAULT);
  semtech_loramac_set_deveui(&loramac, deveui);
  semtech_loramac_set_appeui(&loramac, appeui);
  semtech_loramac_set_appkey(&loramac, appkey);

  semtech_loramac_get_deveui(&loramac, deveui);
  fmt_bytes_hex(print_buf, deveui, LORAMAC_DEVEUI_LEN);
  print_buf[LORAMAC_DEVEUI_LEN * 2] = '\0';
  printf("DEVEUI: %s\n", print_buf);

  semtech_loramac_get_appkey(&loramac, appkey);
  fmt_bytes_hex(print_buf, appkey, LORAMAC_APPKEY_LEN);
  print_buf[LORAMAC_APPKEY_LEN * 2] = '\0';
  printf("APPKEY: %s\n", print_buf);

  if (!semtech_loramac_is_mac_joined(&loramac)) {
    /* Start the Over-The-Air Activation (OTAA) procedure to retrieve the
     * generated device address and to get the network and application session
     * keys.
     */
    puts("Starting join procedure");
    if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) != SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
      puts("Join procedure failed");
      while(1)
      {
        gpio_clear(LEDON_PIN);
        ztimer_sleep(ZTIMER_SEC,1);
        gpio_set(LEDON_PIN);
        ztimer_sleep(ZTIMER_SEC,1);
      }
      return 1;
    }
  }
  gpio_set(LED0_PIN);

  puts("Join procedure succeeded");
  semtech_loramac_set_tx_mode(&loramac, LORAMAC_TX_UNCNF);
  semtech_loramac_set_dr(&loramac, LORAMAC_DR_5);

  mutex_init(&sender_mutex);
  msg_t msg_queue[8];
  msg_init_queue(msg_queue, 8);
  main_pid = thread_getpid();
  sender_pid = thread_create(sender_stack, sizeof(sender_stack),
      SENDER_PRIORITY, 0,
      sender_thread, NULL, "sender_thread");
#ifdef MODULE_SCD30
  init_use_scd30(sender_pid, &sender_mutex);
#endif
#ifdef MODULE_SCD4X
  init_use_scd4x(sender_pid, &sender_mutex);
#endif
#ifdef MODULE_SPS30
  init_use_sps30(sender_pid, &sender_mutex);
#endif
#ifdef MODULE_SHT3X
  init_use_sht3x(sender_pid, &sender_mutex);
#endif
#ifdef MODULE_SFA3X
  init_use_sfa3x(sender_pid, &sender_mutex);
#endif
#ifdef MODULE_SEN5X
  init_use_sen5x(sender_pid, &sender_mutex);
#endif
#ifdef MODULE_SGP40
  init_use_sgp40(sender_pid, &sender_mutex);
#endif
#ifdef MODULE_SVM40
  init_use_svm40(sender_pid, &sender_mutex);
#endif
#ifdef MODULE_BME680
  init_use_bme680(sender_pid, &sender_mutex);
#endif
#ifdef MODULE_DRIVER_GMXXX
  init_use_gmxxx(sender_pid, &sender_mutex);
#endif

  while(1)
  {
    msg_receive(&msg);
    mutex_lock(&sender_mutex);
    gpio_clear(LED0_PIN);
      uint8_t ret = semtech_loramac_send(&loramac, (uint8_t *)sender_buffer, sender_cursor);
      if(ret == SEMTECH_LORAMAC_DUTYCYCLE_RESTRICTED)
      {
          puts("DUTYCYCLE_RESTRICTED");
          gpio_set(LED0_PIN);
          ztimer_sleep(ZTIMER_SEC,1);
          gpio_clear(LED0_PIN);
          ztimer_sleep(ZTIMER_SEC,1);
          gpio_set(LED0_PIN);
          ztimer_sleep(ZTIMER_SEC,1);
          gpio_clear(LED0_PIN);
          ztimer_sleep(ZTIMER_SEC,1);
      }
      else if (ret != SEMTECH_LORAMAC_TX_DONE)  {
          printf("Cannot send message, ret code: %d\n",ret);
          gpio_set(LED0_PIN);
          ztimer_sleep(ZTIMER_SEC,1);
          gpio_clear(LED0_PIN);
          ztimer_sleep(ZTIMER_SEC,1);
      }
      gpio_set(LED0_PIN);

    printf("[%ld]SENDING\r\n", ztimer_now(ZTIMER_SEC));
    for(int i=0;i<sender_cursor;i++)
    {
      printf("0x%02X ",sender_buffer[i]);
    }
    puts("\r\n");
    mutex_unlock(&sender_mutex);
  }
  return 0;
}
