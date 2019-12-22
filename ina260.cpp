/*******************************************************************************
 *
 *  name: ina260.cpp
 *  date: Dec 13, 2019
 *  auth: andrew
 *  desc:
 *
 ******************************************************************************/

// ----------------------------------------------------------------- includes --

#include "ina260.h"
#include "global.h"

// ---------------------------------------------------------- private defines --

/* nothing */

// ----------------------------------------------------------- private macros --

/* nothing */

// ------------------------------------------------------------ private types --

/* nothing */

// ------------------------------------------------------- exported variables --

ina260_sample_t const INA260_SAMPLE_INVALID = (ina260_sample_t){ 0, 0, 0U };

// -------------------------------------------------------- private variables --

/* nothing */

// ---------------------------------------------- private function prototypes --

#ifdef __cplusplus
extern "C" {
#endif

/* nothing */

#ifdef __cplusplus
}
#endif

// ------------------------------------------------------- exported functions --

ina260_t *ina260_new(Adafruit_INA260 *ina260)
{
  ina260_t *dev = NULL;

  if (NULL == (dev = (ina260_t *)malloc(sizeof(ina260_t)))) {
    info(ilError, "failed: malloc()");
    return NULL;
  }

  dev->ina260 = ina260;

  if (!(dev->ina260->begin())) {
    info(ilError, "failed: INA260 begin()");
    return NULL;
  }

  dev->ina260->setMode(INA260_MODE_CONTINUOUS);

  return dev;
}

int32_t ina260_voltage(ina260_t *dev)
{
  if (NULL == dev)
    { return 0U; }

  float mv = dev->ina260->readBusVoltage();

  return (int32_t)(mv + 0.5F);
}

int32_t ina260_current(ina260_t *dev)
{
  if (NULL == dev)
    { return 0U; }

  float ma = dev->ina260->readCurrent();

  return (int32_t)(ma + 0.5F);
}

bool ina260_sample(ina260_t *dev, ina260_sample_t *sample)
{
  if ((NULL == dev) || (NULL == sample))
    { return false; }

  uint32_t time = millis();
  int32_t voltage, current;
  bool ready = time - sample->time > __INA260_CONTINUOUS_REFRESH_MS__;

  if (ready) {

    voltage = ina260_voltage(dev);
    current = ina260_current(dev);

    if (in_range_i32(voltage,
          __INA260_VOLTAGE_MIN_MV__,
          __INA260_VOLTAGE_MAX_MV__)
          &&
        in_range_i32(current,
          __INA260_CURRENT_MIN_MV__,
          __INA260_CURRENT_MAX_MV__)) {

      sample->voltage_mV = voltage;
      sample->current_mA = current;
      sample->time       = time;
      return true;
    }
  }
  return false;
}

// -------------------------------------------------------- private functions --

/* nothing */
