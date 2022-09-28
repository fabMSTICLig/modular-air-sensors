#ifdef MODULE_SVM40
#include "periph/i2c.h"
#include "ztimer.h"
#include "airqual_common.h"
#include "dlpp.h"

#include "svm40_i2c.h"

#include "use_svm40.h"

static kernel_pid_t sender_pid;
static mutex_t *    sender_mutex;
static char         rcv_thread_stack[THREAD_STACKSIZE_MAIN/2];
static uint8_t      buffer[USE_SVM40_BUFFER_SIZE];
static mutex_t      buffer_mutex;
static ztimer_t     timer;
static mutex_t      timer_mutex = MUTEX_INIT_LOCKED;
static airmsg_t     airmsg;
static msg_t        msg;


static i2c_t i2c_dev;


static void _timer_cb(void *arg)
{
  ztimer_set(ZTIMER_SEC, &timer, USE_SVM40_INTERVAL_SECS);
  mutex_unlock(arg);
}


static void * svm40_thread(void *arg)
{
  (void)arg;


  svm40_start_continuous_measurement(i2c_dev);

  timer.callback = _timer_cb;
  timer.arg = &timer_mutex;
  ztimer_set(ZTIMER_SEC, &timer, USE_SVM40_INTERVAL_SECS);

  int16_t voc_index;
  int16_t humidity;
  int16_t temperature;
  int16_t error=0;
  int8_t running=true;
  while(running)
  {
    mutex_lock(sender_mutex);
    error = svm40_read_measured_values_as_integers(i2c_dev, &voc_index, &humidity,
        &temperature);
    if (!error) {
      printf("SVM40 Voc index: %i (index * 10)\n", voc_index);
      printf("SVM40 Humidity: %i milli %% RH\n", humidity * 10);
      printf("SVM40 Temperature: %i milli °C\n", (temperature >> 1) * 10);

      mutex_lock(&buffer_mutex);
      dlpp(USE_SVM40_CHANNEL, buffer,'h',3, humidity, temperature,
          voc_index);
      mutex_unlock(&buffer_mutex);
      int retmsg = msg_try_send(&msg, sender_pid);
      if(retmsg != 1) printf("SVM40 sendfail %d", retmsg);
    }
    else if(error == -ENXIO )
    {
      puts("SVM40 disconnected");
      running=false;
    }
    else{
      printf("SVM40 err  %d\r\n",error);
    }
    mutex_unlock(sender_mutex);
    mutex_lock(&timer_mutex);

  }

  return NULL;
}

void init_use_svm40(kernel_pid_t sender_pid_p, mutex_t * sender_mutex_p){

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
  while (-EAGAIN == (retval = i2c_read_byte(i2c_dev, SVM40_I2C_ADDRESS, dummy, 0))) {
    /* retry until bus arbitration succeeds */
  }

  int8_t present=0;
  switch (retval) {
    case 0:
      present=1;
      break;
    case -ENXIO:
      /* No ACK --> no device */
      puts("SVM40 not connected");
      break;
    default:
      /* Some unexpected error */
      puts("Error I2C SVM40");
      break;
  }
  i2c_release(i2c_dev);

  if(present)
  {
    puts("SVM40 start thread");
    thread_create(rcv_thread_stack, sizeof(rcv_thread_stack),
        USE_SVM40_THREAD_PRIORITY, 0,
        svm40_thread, NULL, "svm40_thread");
  }

}
#endif
