# name of the application
APPLICATION = mas

# If no BOARD is found in the environment, use this default:
BOARD ?= lora-e5-dev

DEVEUI ?= 0000000000000000
APPEUI ?= 0000000000000000
APPKEY ?= 00000000000000000000000000000000

LORA_DRIVER ?= sx126x_stm32wl
LORA_REGION ?= EU868

USEPKG += semtech-loramac
USEMODULE += $(LORA_DRIVER)
USEMODULE += auto_init_loramac
USEMODULE += fmt

USEMODULE += ztimer_sec
USEMODULE += ztimer_usec
USEMODULE += ztimer_msec
USEMODULE += printf_float
FEATURES_OPTIONAL += periph_rtc

#USEMODULE += sps30
USEMODULE += scd30
#USEMODULE += scd4x
USEMODULE += sht3x
#USEMODULE += sfa3x
USEMODULE += sen5x
#USEMODULE += sgp40
#USEMODULE += svm40
#USEMODULE += bme680_i2c
#USEMODULE += bme680_fp
USEMODULE += driver_gmxxx

USEMODULE += dlpp

#Do we use SCD30 temperature and humidity sensor (0 ignored, 1 used)
SCD30_TH_ON ?= 0

ifneq (,$(filter scd30,$(USEMODULE)))
  CFLAGS += -DUSE_SCD30_CHANNEL=1
  CFLAGS += -DUSE_SCD30_THREAD_PRIORITY=2
  CFLAGS += -DUSE_SCD30_INTERVAL_SECS=60
#Separate frame for temperature and humidity
ifeq ($(SCD30_TH_ON), 1)
  CFLAGS += -DUSE_SCD30_TH_ON=1
  CFLAGS += -DUSE_SCD30_TH_CHANNEL=2
endif
endif
ifneq (,$(filter sps30,$(USEMODULE)))
  CFLAGS += -DUSE_SPS30_CHANNEL=2
  CFLAGS += -DUSE_SPS30_THREAD_PRIORITY=3
  CFLAGS += -DUSE_SPS30_INTERVAL_SECS=30
endif
ifneq (,$(filter scd4x,$(USEMODULE)))
  CFLAGS += -DUSE_SCD4X_CHANNEL=3
  CFLAGS += -DUSE_SCD4X_THREAD_PRIORITY=4
  CFLAGS += -DUSE_SCD4X_INTERVAL_SECS=60
endif
ifneq (,$(filter sht3x,$(USEMODULE)))
  CFLAGS += -DUSE_SHT3X_CHANNEL=4
  CFLAGS += -DUSE_SHT3X_THREAD_PRIORITY=5
  CFLAGS += -DUSE_SHT3X_INTERVAL_SECS=60
endif
ifneq (,$(filter sfa3x,$(USEMODULE)))
  CFLAGS += -DUSE_SFA3X_CHANNEL=5
  CFLAGS += -DUSE_SFA3X_THREAD_PRIORITY=6
  CFLAGS += -DUSE_SFA3X_INTERVAL_SECS=60
endif
ifneq (,$(filter sen5x,$(USEMODULE)))
  CFLAGS += -DUSE_SEN5X_CHANNEL=6
  CFLAGS += -DUSE_SEN5X_THREAD_PRIORITY=7
  CFLAGS += -DUSE_SEN5X_INTERVAL_SECS=60
endif
ifneq (,$(filter sgp40,$(USEMODULE)))
  CFLAGS += -DUSE_SGP40_CHANNEL=7
  CFLAGS += -DUSE_SGP40_THREAD_PRIORITY=8
  CFLAGS += -DUSE_SGP40_INTERVAL_SECS=60
endif
ifneq (,$(filter bme680_i2c,$(USEMODULE)))
  CFLAGS += -DUSE_BME680_CHANNEL=8
  CFLAGS += -DUSE_BME680_THREAD_PRIORITY=9
  CFLAGS += -DUSE_BME680_INTERVAL_SECS=60
endif
ifneq (,$(filter svm40,$(USEMODULE)))
  CFLAGS += -DUSE_SVM40_CHANNEL=10
  CFLAGS += -DUSE_SVM40_THREAD_PRIORITY=10
  CFLAGS += -DUSE_SVM40_INTERVAL_SECS=60
endif
ifneq (,$(filter driver_gmxxx,$(USEMODULE)))
  CFLAGS += -DUSE_GMXXX_CHANNEL=11
  CFLAGS += -DUSE_GMXXX_THREAD_PRIORITY=11
  CFLAGS += -DUSE_GMXXX_INTERVAL_SECS=60
endif


# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

INCLUDES += -I$(CURDIR)/include

USEMODULE += main_uses
DIRS += $(CURDIR)/uses

include $(RIOTBASE)/Makefile.include


ifndef CONFIG_KCONFIG_USEMODULE_LORAWAN
  # OTAA compile time configuration keys
  CFLAGS += -DCONFIG_LORAMAC_APP_KEY_DEFAULT=\"$(APPKEY)\"
  CFLAGS += -DCONFIG_LORAMAC_APP_EUI_DEFAULT=\"$(APPEUI)\"
  CFLAGS += -DCONFIG_LORAMAC_DEV_EUI_DEFAULT=\"$(DEVEUI)\"
endif
