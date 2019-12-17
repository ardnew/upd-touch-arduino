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

// Grand Central M4
#define __GPIO_NEOPIXEL_PIN__ 8

// TFT / Touchscreen
#define __GPIO_TFT_VCC_PIN__    /* 3v3 */
#define __GPIO_TFT_GND_PIN__    /* GND */
#define __GPIO_TFT_CS_PIN__      5
#define __GPIO_TFT_RST_PIN__    11
#define __GPIO_TFT_DC_PIN__      9
#define __GPIO_TFT_MOSI_PIN__   25
#define __GPIO_TFT_SCK_PIN__    23
#define __GPIO_TFT_LED_PIN__    /* 3v3 */
#define __GPIO_TFT_MISO_PIN__   24
#define __GPIO_TOUCH_CLK_PIN__  23
#define __GPIO_TOUCH_CS_PIN__    6
#define __GPIO_TOUCH_MOSI_PIN__ 25
#define __GPIO_TOUCH_MISO_PIN__ 24
#define __GPIO_TOUCH_IRQ_PIN__  10
// USB PD controller
#define __GPIO_USBPD_GND_PIN__  /* GND */
#define __GPIO_USBPD_SCL_PIN__  15
#define __GPIO_USBPD_SDA_PIN__  14
#define __GPIO_USBPD_RST_PIN__  21
#define __GPIO_USBPD_VPP_PIN__  /* 3v3 */
#define __GPIO_USBPD_VCC_PIN__  /* GND */
#define __GPIO_USBPD_ATCH_PIN__ 12
#define __GPIO_USBPD_ALRT_PIN__ 13 // on Arduino digital 13 (LED)

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

/* nothing */

// ------------------------------------------------------- exported functions --

#ifdef __cplusplus
extern "C" {
#endif

void init_gpio();
void init_peripherals();

void info(info_level_t level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // __GLOBAL_H__