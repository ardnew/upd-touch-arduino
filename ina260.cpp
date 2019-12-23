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

  (void)ina260_init(dev);

  return dev;
}

bool ina260_init(ina260_t *dev)
{
  if (NULL == dev)
    { return false; }

  dev->ina260 = new Adafruit_INA260();
  if (!(dev->ina260->begin())) {
    info(ilError, "failed: INA260 begin()");
    return false;
  }

  if (NULL == dev->ina260)
    { return false; }

  dev->ina260->setMode(INA260_MODE_CONTINUOUS);

  return true;
}

bool ina260_reset(ina260_t *dev)
{
  if ((NULL == dev) || (NULL == dev->ina260))
    { return false; }

  dev->ina260->reset();
  dev->ina260->setMode(INA260_MODE_CONTINUOUS);

  return true;
}

bool ina260_ready(ina260_t *dev)
{
  if ((NULL == dev) || (NULL == dev->ina260))
    { return false; }

  return dev->ina260->conversionReady();
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

//static inline float _pow10(int exp) { // lookup table for powers of 10
//#define POW10_MIN -5
//#define POW10_MAX  5
//  static const float P[11] = { // powers in range [-5, 5]
//    1.e-5F, 1.e-4F, 1.e-3F, 1.e-2F, 1.e-1F, 1.e0F, 1.e+1F, 1.e+2F, 1.e+3F, 1.e+4F, 1.e+5F
//  };
//  if (exp >= POW10_MIN && exp <= POW10_MAX) {
//    return P[exp + POW10_MAX];
//  }
//  return 0.0F; // error, 0 is not a power of 10
//#undef POW10_MIN
//#undef POW10_MAX
//}
//
//void ina260_fmt(char *str[], size_t len, uint8_t width, int32_t val, char const *ulo, char const *uhi, uint8_t dlo, uint8_t dhi)
//{
//  char const *unit = uhi;
//  uint32_t     abs = abs(val);
//  uint8_t      dig = dhi;
//  float     scaled = 0.001F;
//
//  if (abs < 1000) {
//    dig    = dlo;
//    unit   = ulo;
//    scaled = 1.0F;
//  }
//
//  scaled *= (float)abs;
//
//  if (dig < 1U)
//    { dig = 1U; }
//
//  uint8_t prec = dig - 1U; // trailing digits to show when value is 0
//  if (fabs(scaled) > _pow10(-dig)) {
//    prec = dig - (uint8_t)log10(fabsf(scaled)) - 1U;
//  }
//  if (prec > 8U) // most likely underflowed
//    { prec = 0U; }
//
//  //Serial.printf("[%*.*f %s]\n", width, prec, scaled, unit);
//
//  snprintf(*str, len, "%*.*f %s", width, prec, scaled, unit);
//
//}
//
//void ina260_fmt_voltage(ina260_t *dev, int32_t voltage_mV, char *str[], size_t len)
//{
//  ina260_fmt(str, len, 6, voltage_mV, "mV", "V", 3, 5);
//  //Serial.println(*str);
//}
//
//void ina260_fmt_current(ina260_t *dev, int32_t current_mA, char *str[], size_t len)
//{
//  ina260_fmt(str, len, 6, current_mA, "mA", "A", 2, 4);
//  //Serial.println(*str);
//}

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
