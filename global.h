/*******************************************************************************
 *
 *  name: global.h
 *  date: Dec 13, 2019
 *  auth: andrew
 *  desc:
 *
 ******************************************************************************/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

// ----------------------------------------------------------------- includes --

/* nothing */

// ------------------------------------------------------------------ defines --

//#define __WAIT_FOR_SERIAL__

#define FLOAT_ABSTOL 5.0e-6

// Teensy 4.0

// TFT / Touchscreen
#define __GPIO_TFT_VCC_PIN__    /* 3v3 */
#define __GPIO_TFT_GND_PIN__    /* GND */
#define __GPIO_TFT_CS_PIN__     10
#define __GPIO_TFT_RST_PIN__     2
#define __GPIO_TFT_DC_PIN__      9
#define __GPIO_TFT_MOSI_PIN__   11
#define __GPIO_TFT_SCK_PIN__    13
#define __GPIO_TFT_LED_PIN__    /* 3v3 */
#define __GPIO_TFT_MISO_PIN__   12
#define __GPIO_TOUCH_CLK_PIN__  13
#define __GPIO_TOUCH_CS_PIN__    8
#define __GPIO_TOUCH_MOSI_PIN__ 11
#define __GPIO_TOUCH_MISO_PIN__ 12
#define __GPIO_TOUCH_IRQ_PIN__   3
// USB PD controller
#define __GPIO_USBPD_GND_PIN__   /* GND */
#define __GPIO_USBPD_SCL_PIN__   19
#define __GPIO_USBPD_SDA_PIN__   18
#define __GPIO_USBPD_RST_PIN__   23
#define __GPIO_USBPD_VPP_PIN__   /* 3v3 */
#define __GPIO_USBPD_VCC_PIN__   /* GND */
#define __GPIO_USBPD_ATCH_PIN__  21
#define __GPIO_USBPD_ALRT_PIN__  20

// ------------------------------------------------------------------- macros --

/* nothing */

// ----------------------------------------------------------- exported types --

typedef enum
{
  ilNONE = -1,
  ilInfo,  // = 0
  ilWarn,  // = 1
  ilError, // = 2
  ilCOUNT  // = 3
}
info_level_t;

typedef enum
{
  siNONE = -1,
  siInitializing,
  siIdleDisconnected,
  siIdleConnected,
  siIdleConnectedNoSource,
  siIdentifyingSource,
  siCOUNT,
}
status_info_t;

// ------------------------------------------------------- exported variables --

extern status_info_t status_info;

// ------------------------------------------------------- exported functions --

#ifdef __cplusplus
extern "C" {
#endif

void init_gpio();
void init_peripherals();

void info(info_level_t level, const char *fmt, ...);

inline bool equals_float(float a, float b)
  { return fabsf(a - b) <= FLOAT_ABSTOL; }

inline bool in_range_u8(uint8_t val, uint8_t min, uint8_t max)
  { return (val >= min) && (val <= max); }

inline bool in_range_i8(int8_t val, int8_t min, int8_t max)
  { return (val >= min) && (val <= max); }

inline bool in_range_u16(uint16_t val, uint16_t min, uint16_t max)
  { return (val >= min) && (val <= max); }

inline bool in_range_i16(int16_t val, int16_t min, int16_t max)
  { return (val >= min) && (val <= max); }

inline bool in_range_u32(uint32_t val, uint32_t min, uint32_t max)
  { return (val >= min) && (val <= max); }

inline bool in_range_i32(int32_t val, int32_t min, int32_t max)
  { return (val >= min) && (val <= max); }

inline bool in_range_float(float val, float min, float max)
  { return ((val > min) || (equals_float(val, min)))
            &&
           ((val < max) || (equals_float(val, max))); }

#ifdef __cplusplus
}
#endif

#endif // __GLOBAL_H__