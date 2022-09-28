#ifdef MODULE_BME680
#include "periph/i2c.h"
#include "ztimer.h"
#include "airqual_common.h"
#include "dlpp.h"

#include "bme680.h"
#include "bme680_params.h"

#include "use_bme680.h"

static kernel_pid_t sender_pid;
static mutex_t *    sender_mutex;
static char         rcv_thread_stack[THREAD_STACKSIZE_MAIN/2];
static uint8_t      buffer[USE_BME680_BUFFER_SIZE];
static mutex_t      buffer_mutex;
static ztimer_t     timer;
static mutex_t      timer_mutex = MUTEX_INIT_LOCKED;
static airmsg_t     airmsg;
static msg_t        msg;


static bme680_t bme680_dev;
static struct bme680_field_data data;


static void _timer_cb(void *arg)
{
  ztimer_set(ZTIMER_SEC, &timer, USE_BME680_INTERVAL_SECS);
  mutex_unlock(arg);
}


static void * bme680_thread(void *arg)
{
  (void)arg;


  bme680_init(&bme680_dev, &bme680_params[0]);

  timer.callback = _timer_cb;
  timer.arg = &timer_mutex;
  ztimer_set(ZTIMER_SEC, &timer, USE_BME680_INTERVAL_SECS);

  int8_t ret=0;
  int8_t running=true;
  while(running)
  {
    mutex_lock(sender_mutex);
    /* trigger one measuerment */
    bme680_force_measurement(&bme680_dev);
    /* get the duration for the measurement */
    int duration = bme680_get_duration(&bme680_dev);
    /* wait for the duration */
    ztimer_sleep(ZTIMER_MSEC, duration);
    /* read the data */
    ret = bme680_get_data(&bme680_dev, &data);


    if (ret == 0 && bme680_dev.sensor.new_fields) {
      if (data.status & BME680_GASM_VALID_MSK) {
        printf("BME680 G = %.0f ohms\r\n", data.gas_resistance);
        mutex_lock(&buffer_mutex);
        dlpp(USE_BME680_CHANNEL, buffer,'I',1, (uint32_t)data.gas_resistance);
        mutex_unlock(&buffer_mutex);
        int retmsg = msg_try_send(&msg, sender_pid);
        if(retmsg != 1) printf("BME680 sendfail %d\r\n", retmsg);
      }
    }else if(ret == -ENXIO )
    {
      puts("BME680 disconnected");
      running=false;
    }
    else{
      printf("BME680 err  %d\r\n",ret);
    }
    mutex_unlock(sender_mutex);
    mutex_lock(&timer_mutex);

  }

  return NULL;
}

void init_use_bme680(kernel_pid_t sender_pid_p, mutex_t * sender_mutex_p){

  sender_pid=sender_pid_p;
  sender_mutex = sender_mutex_p;
  mutex_init(&buffer_mutex);
  airmsg.buffer=buffer;
  airmsg.mutex=&buffer_mutex;
  msg.content.ptr=&airmsg;

  char dummy[1];
  int retval;
  i2c_acquire(BME680_PARAM_I2C_DEV);
  while (-EAGAIN == (retval = i2c_read_byte(BME680_PARAM_I2C_DEV, BME680_PARAM_I2C_ADDR, dummy, 0))) {
    /* retry until bus arbitration succeeds */
  }

  int8_t present=0;
  switch (retval) {
    case 0:
      present=1;
      break;
    case -ENXIO:
      /* No ACK --> no device */
      puts("BME680 not connected");
      break;
    default:
      /* Some unexpected error */
      puts("Error I2C BME680");
      break;
  }
  i2c_release(BME680_PARAM_I2C_DEV);

  if(present)
  {
    puts("BME680 start thread");
    thread_create(rcv_thread_stack, sizeof(rcv_thread_stack),
        USE_BME680_THREAD_PRIORITY, 0,
        bme680_thread, NULL, "bme680_thread");
  }

}
#endif
