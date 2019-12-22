/*******************************************************************************
 *
 *  name: ina260.h
 *  date: Dec 13, 2019
 *  auth: andrew
 *  desc:
 *
 ******************************************************************************/

#ifndef __INA260_H__
#define __INA260_H__

// ----------------------------------------------------------------- includes --

#include <Adafruit_INA260.h>

// ------------------------------------------------------------------ defines --

#define __INA260_VOLTAGE_ZERO_MAX_MV__        100
#define __INA260_VOLTAGE_VBUS_USB_MIN_MV__   4200
#define __INA260_VOLTAGE_VBUS_USB_MAX_MV__   5800
#define __INA260_VOLTAGE_VBUS_9V_MIN_MV__    8200
#define __INA260_VOLTAGE_VBUS_9V_MAX_MV__    9800
#define __INA260_VOLTAGE_VBUS_12V_MIN_MV__  11200
#define __INA260_VOLTAGE_VBUS_12V_MAX_MV__  12800
#define __INA260_VOLTAGE_VBUS_15V_MIN_MV__  14000
#define __INA260_VOLTAGE_VBUS_15V_MAX_MV__  16000
#define __INA260_VOLTAGE_VBUS_20V_MIN_MV__  19000
#define __INA260_VOLTAGE_VBUS_20V_MAX_MV__  21000

#define __INA260_VOLTAGE_MIN_MV__               0
#define __INA260_VOLTAGE_MAX_MV__           25000
#define __INA260_CURRENT_MIN_MV__               0
#define __INA260_CURRENT_MAX_MV__            6000

#define __INA260_CONTINUOUS_REFRESH_MS__      250

// ------------------------------------------------------------------- macros --

/* nothing */

// ----------------------------------------------------------- exported types --

typedef struct
{
  int32_t  voltage_mV;
  int32_t  current_mA;
  uint32_t time;
}
ina260_sample_t;

typedef struct
{
  Adafruit_INA260 *ina260;
}
ina260_t;


// ------------------------------------------------------- exported variables --

extern ina260_sample_t const INA260_SAMPLE_INVALID;

// ------------------------------------------------------- exported functions --

#ifdef __cplusplus
extern "C" {
#endif

ina260_t *ina260_new(Adafruit_INA260 *ina260);

int32_t ina260_voltage(ina260_t *dev);
int32_t ina260_current(ina260_t *dev);

bool ina260_sample(ina260_t *dev, ina260_sample_t *sample);

#ifdef __cplusplus
}
#endif

#endif // __INA260_H__