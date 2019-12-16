/*******************************************************************************
 *
 *  name: global.cpp
 *  date: Dec 13, 2019
 *  auth: andrew
 *  desc:
 *
 ******************************************************************************/

// ----------------------------------------------------------------- includes --

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <Arduino.h>

#include <libstusb4500.h>

#include "ili9341.h"
#include "neopixel.h"
#include "global.h"

// ---------------------------------------------------------- private defines --

#define __SERIAL_BAUD__ 115200

#define __INFO_LENGTH_MAX__ 256

// ----------------------------------------------------------- private macros --

/* nothing */

// ------------------------------------------------------------ private types --

typedef struct
{
  neopixel_mode_t  mode;
  neopixel_color_t color;
  uint8_t          bright;
  uint32_t         delay;
}
neopixel_status_info_t;

// ------------------------------------------------------- exported variables --

extern ili9341_t *screen;
extern neopixel_t *pixel;
extern stusb4500_device_t *usbpd;

// -------------------------------------------------------- private variables --

const char *DEBUG_LEVEL_PREFIX[ilCOUNT] = {
  "[ ] ", "[*] ", "[!] "
};

const neopixel_status_info_t STATUS_INFO_COLOR[siCOUNT] = {
  /* siInitializing          */ { .mode = nmFixed,    .color = RGB(0x00, 0xFF, 0xFF), .bright =  30U, .delay =  0U }, // fixed teal mid
  /* siIdleDisconnected      */ { .mode = nmPulse,    .color = RGB(0xFF, 0x00, 0x00), .bright =  10U, .delay = 50U }, // pulse red dim slow
  /* siIdleConnected         */ { .mode = nmFabulous, .color = RGB(0x00, 0x00, 0x00), .bright =  20U, .delay = 10U }, // fabulous dim med
  /* siIdleConnectedNoSource */ { .mode = nmPulse,    .color = RGB(0xFF, 0x00, 0xFF), .bright =  10U, .delay = 50U }, // pulse purple dim slow
  /* siIdentifyingSource     */ { .mode = nmFixed,    .color = RGB(0x00, 0x00, 0xFF), .bright = 100U, .delay =  0U }  // fixed blue bright
};

// ---------------------------------------------- private function prototypes --

#ifdef __cplusplus
extern "C" {
#endif

void touch_interrupt();
void usbpd_attach();
void usbpd_alert();

void touch_begin(ili9341_t *tft);
void touch_end(ili9341_t *tft);

void update_pixel(neopixel_t *pix, neopixel_status_info_t info);

void cable_attached(stusb4500_device_t *usb);
void cable_detached(stusb4500_device_t *usb);
void capabilities_request_begin(stusb4500_device_t *usb);
void capabilities_request_end(stusb4500_device_t *usb);
void capabilities_received(stusb4500_device_t *usb);

#ifdef __cplusplus
}
#endif

// ------------------------------------------------------- exported functions --

void init_gpio()
{

  pinMode(__GPIO_TFT_CS_PIN__, OUTPUT);
  pinMode(__GPIO_TFT_RST_PIN__, OUTPUT);
  pinMode(__GPIO_TFT_DC_PIN__, OUTPUT);
  pinMode(__GPIO_TFT_MOSI_PIN__, OUTPUT);
  pinMode(__GPIO_TFT_SCK_PIN__, OUTPUT);
  pinMode(__GPIO_TFT_MISO_PIN__, INPUT);
  pinMode(__GPIO_TOUCH_CLK_PIN__, OUTPUT);
  pinMode(__GPIO_TOUCH_CS_PIN__, OUTPUT);
  pinMode(__GPIO_TOUCH_MOSI_PIN__, OUTPUT);
  pinMode(__GPIO_TOUCH_MISO_PIN__, INPUT);
  pinMode(__GPIO_TOUCH_IRQ_PIN__, INPUT);

  pinMode(__GPIO_USBPD_SCL_PIN__, OUTPUT);
  pinMode(__GPIO_USBPD_SDA_PIN__, OUTPUT);
  pinMode(__GPIO_USBPD_RST_PIN__, OUTPUT);
  pinMode(__GPIO_USBPD_ATCH_PIN__, INPUT_PULLUP);
  pinMode(__GPIO_USBPD_ALRT_PIN__, INPUT_PULLUP);

  attachInterrupt(
      digitalPinToInterrupt(__GPIO_TOUCH_IRQ_PIN__), touch_interrupt, CHANGE);
  attachInterrupt(
      digitalPinToInterrupt(__GPIO_USBPD_ATCH_PIN__), usbpd_attach, CHANGE);
  attachInterrupt(
      digitalPinToInterrupt(__GPIO_USBPD_ALRT_PIN__), usbpd_alert, FALLING);
}

void init_peripherals()
{
#if defined(__WAIT_FOR_SERIAL__)
  while (!Serial) { continue; }
#endif

  // --
  // USB serial (debugging and general info)
  Serial.begin(__SERIAL_BAUD__);

  info(ilInfo, "initializing system resources ...\n");

  // --
  // I2C (bus master)
  Wire.begin();
  Wire.setClock(__STUSB4500_I2C_CLOCK_FREQUENCY__);

  // --
  // ILI9341 (TFT touch screen)
  screen = ili9341_new(
      new Adafruit_ILI9341(
          __GPIO_TFT_CS_PIN__, __GPIO_TFT_DC_PIN__, __GPIO_TFT_MOSI_PIN__,
          __GPIO_TFT_SCK_PIN__, __GPIO_TFT_RST_PIN__, __GPIO_TFT_MISO_PIN__),
      new XPT2046_Touchscreen(
          __GPIO_TOUCH_CS_PIN__, __GPIO_TOUCH_IRQ_PIN__),
      isoPortrait,
      __GPIO_TOUCH_IRQ_PIN__,
      150, 325, 3800, 4000);

  ili9341_set_touch_pressed_begin(screen, touch_begin);
  ili9341_set_touch_pressed_end(screen, touch_end);

  // --
  // Neopixel (on-board)
  pixel = neopixel_new(__GPIO_NEOPIXEL_PIN__,
      nmPulse, nsShow, STATUS_INFO_COLOR[siInitializing].color, 10, 25);

  // --
  // STUSB4500 (zoinks!)
  usbpd = stusb4500_device_new(
      &Wire, __STUSB4500_I2C_SLAVE_BASE_ADDR__, __GPIO_USBPD_RST_PIN__);

  stusb4500_set_cable_attached(usbpd, cable_attached);
  stusb4500_set_cable_detached(usbpd, cable_detached);
  stusb4500_set_source_capabilities_request_begin(usbpd, capabilities_request_begin);
  stusb4500_set_source_capabilities_request_begin(usbpd, capabilities_request_begin);
  stusb4500_set_source_capabilities_request_end(usbpd, capabilities_request_end);
  stusb4500_set_source_capabilities_received(usbpd, capabilities_received);

  // --

  info(ilInfo, "resource initialization complete, initializing STUSB4500 ...\n");

  stusb4500_device_init(usbpd);
  delay(1000);
  info(ilInfo, "done, setting power profile ...\n");
  stusb4500_set_power(usbpd, 5000, 3000);
  delay(1000);
  info(ilInfo, "done, verifying requested power profile ...\n");
  stusb4500_pdo_description_t req = stusb4500_power_requested(usbpd);
  if (__STUSB4500_NVM_INVALID_PDO_INDEX__ != req.number) {
    info(ilInfo, "requested capability: (%d) %u mV, %u mA, (%u mA MAX)\n",
        req.number, req.voltage_mv, req.current_ma, req.max_current_ma);
  }
  else {
    info(ilWarn, "no capability requested!");
  }
  info(ilInfo, "STUSB4500 initialization complete, entering runtime ...\n");
}

void info(info_level_t level, const char *fmt, ...)
{
  static char buff[__INFO_LENGTH_MAX__] = { 0 };

  va_list arg;
  va_start(arg, fmt);
  vsnprintf(buff, __INFO_LENGTH_MAX__, fmt, arg);
  va_end(arg);

  Serial.print(DEBUG_LEVEL_PREFIX[level]);
  Serial.print(buff);
}

// -------------------------------------------------------- private functions --

void touch_interrupt()
{
  ili9341_touch_interrupt(screen);
}

void usbpd_attach()
{
  stusb4500_attach(usbpd);
}

void usbpd_alert()
{
  stusb4500_alert(usbpd);
}

void touch_begin(ili9341_t *tft)
{
  info(ilInfo, "touch began\n");
}

void touch_end(ili9341_t *tft)
{
  info(ilInfo, "touch ended\n");
}

void update_pixel(neopixel_t *pix, neopixel_status_info_t info)
{
  switch (info.mode) {
    case nmFixed:
      neopixel_set_color(pix, info.color, info.bright);
      break;

    case nmPulse:
      neopixel_set_pulse(pix, info.color, info.bright, info.delay);
      break;

    case nmFabulous:
      neopixel_set_fabulous(pix, info.bright, info.delay);
      break;

    default:
      break;
  }
  neopixel_update(pix);
}

void cable_attached(stusb4500_device_t *usb)
{
  info(ilInfo, "cable attached\n");
  update_pixel(pixel, STATUS_INFO_COLOR[siIdentifyingSource]);
  stusb4500_get_source_capabilities(usb);
  delay(1000);
  stusb4500_select_power_usb_default(usb);
  delay(1000);
  stusb4500_pdo_description_t req = stusb4500_power_requested(usb);
  if (__STUSB4500_NVM_INVALID_PDO_INDEX__ != req.number) {
    info(ilInfo, "requested capability: (%d) %u mV, %u mA, (%u mA MAX)\n",
        req.number, req.voltage_mv, req.current_ma, req.max_current_ma);
  }
  else {
    info(ilWarn, "no capability requested!");
  }
}

void cable_detached(stusb4500_device_t *usb)
{
  info(ilInfo, "cable detached\n");
  update_pixel(pixel, STATUS_INFO_COLOR[siIdleDisconnected]);
}

void capabilities_request_begin(stusb4500_device_t *usb)
{
  info(ilInfo, "capabilities request begin\n");
  update_pixel(pixel, STATUS_INFO_COLOR[siIdentifyingSource]);
}

void capabilities_request_end(stusb4500_device_t *usb)
{
  info(ilInfo, "capabilities request end\n");
  if (0U == usb->usbpd_status.pdo_src_count) {
    // no sources found
    info(ilWarn, "no capabilities found\n");
    update_pixel(pixel, STATUS_INFO_COLOR[siIdleConnectedNoSource]);
  }
  // else {
  //   // sources found
  //   update_pixel(pixel, STATUS_INFO_COLOR[siIdleConnected]);
  // }
}

void capabilities_received(stusb4500_device_t *usb)
{
  info(ilInfo, "capabilities received\n");
  update_pixel(pixel, STATUS_INFO_COLOR[siIdleConnected]);

  char pdo_str[32];
  float volts;
  float amps;
  uint16_t watts;

  for (uint8_t i = 0; i < usb->usbpd_status.pdo_src_count; ++i) {

    volts = (float)(usb->usbpd_status.pdo_src[i].fix.Voltage) / 20.0F;
    amps  = (float)(usb->usbpd_status.pdo_src[i].fix.Max_Operating_Current) / 100.0F;
    watts = (uint16_t)(volts * amps);

    snprintf(pdo_str, 32, "(%u) %4.1fV %4.1fA %2uW", i + 1, volts, amps, watts);

    info(ilInfo, "%s\n", pdo_str);
  }
}
