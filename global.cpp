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

#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>
#include <Arduino.h>

#include <libstusb4500.h>

#include "ili9341.h"
#include "global.h"

// ---------------------------------------------------------- private defines --

#define __SERIAL_BAUD__ 115200

#define __INFO_LENGTH_MAX__ 256

// ----------------------------------------------------------- private macros --

/* nothing */

// ------------------------------------------------------------ private types --

/* nothing */

// ------------------------------------------------------- exported variables --

extern ili9341_t *screen;
extern stusb4500_device_t *usbpd;

// -------------------------------------------------------- private variables --

const char *DEBUG_LEVEL_PREFIX[ilCOUNT] = {
  "[ ] ", "[*] ", "[!] "
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

void cable_attached(stusb4500_device_t *usb);
void cable_detached(stusb4500_device_t *usb);
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

  // TFT reset is active low, i'm choosing to keep it high with a digital pin
  // instead of a fixed pullup so that we can actually reset it if needed.
  digitalWrite(__GPIO_TFT_RST_PIN__, LOW);
  delay(50);
  digitalWrite(__GPIO_TFT_RST_PIN__, HIGH);
  delay(20);

  //attachInterrupt(
  //    digitalPinToInterrupt(__GPIO_TOUCH_IRQ_PIN__), touch_interrupt, CHANGE);
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

  info(ilInfo, "initializing ...\n");

  // --
  // I2C (bus master)
  Wire.begin();
  Wire.setClock(__STUSB4500_I2C_CLOCK_FREQUENCY__);

  // --
  // ILI9341 (TFT touch screen)
  screen = ili9341_new(
      new ILI9341_t3(
          __GPIO_TFT_CS_PIN__, __GPIO_TFT_DC_PIN__),
      new XPT2046_Touchscreen(
          __GPIO_TOUCH_CS_PIN__, __GPIO_TOUCH_IRQ_PIN__),
      isoTop,
      __GPIO_TOUCH_IRQ_PIN__,
      360, 100, 3920, 3560);

  ili9341_set_touch_pressed_begin(screen, touch_begin);
  ili9341_set_touch_pressed_end(screen, touch_end);

  // --
  // STUSB4500 (zoinks!)
  usbpd = stusb4500_device_new(
      &Wire, __STUSB4500_I2C_SLAVE_BASE_ADDR__, __GPIO_USBPD_RST_PIN__);

  stusb4500_set_cable_attached(usbpd, cable_attached);
  stusb4500_set_cable_detached(usbpd, cable_detached);
  stusb4500_set_source_capabilities_received(usbpd, capabilities_received);

  info(ilInfo, "initialization complete\n");

  // --

  stusb4500_device_init(usbpd);
  delay(200);
  stusb4500_set_power(usbpd, 9000, 3000);
  delay(200);
  stusb4500_pdo_description_t req = stusb4500_power_requested(usbpd);
  if (__STUSB4500_NVM_INVALID_PDO_INDEX__ != req.number) {
    info(ilInfo, "requested capability: (%d) %u mV, %u mA, (%u mA MAX)\n",
        req.number, req.voltage_mv, req.current_ma, req.max_current_ma);
  }
  else {
    info(ilWarn, "no capability requested!");
  }
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

void cable_attached(stusb4500_device_t *usb)
{
  info(ilInfo, "cable attached\n");
  stusb4500_select_power_usb_default(usb);
  stusb4500_get_source_capabilities(usb);
}

void cable_detached(stusb4500_device_t *usb)
{
  info(ilInfo, "cable detached\n");
  ili9341_remove_all_source_capabilities(screen);
}

void capabilities_request_begin(stusb4500_device_t *usb)
{
  //info(ilInfo, "capabilities request begin\n");
}

void capabilities_request_end(stusb4500_device_t *usb)
{
  //info(ilInfo, "capabilities request end\n");
}

void capabilities_received(stusb4500_device_t *usb)
{
  info(ilInfo, "capabilities received\n");

  char pdo_str[128];
  uint32_t volts;
  uint32_t amps;
  uint32_t watts;

  for (uint8_t i = 0; i < usb->usbpd_status.pdo_src_count; ++i) {

    volts = usb->usbpd_status.pdo_src[i].fix.Voltage * 50U;
    amps  = usb->usbpd_status.pdo_src[i].fix.Max_Operating_Current * 10U;
    watts = (uint32_t)((float)volts / 1000.0F * (float)amps / 1000.0F);

    snprintf(pdo_str, 128, " - [%u] %2lu mV, %1lu mA (%3lu W)",
        i + 1, volts, amps, watts);

    ili9341_add_source_capability(
        screen, i + 1, volts, amps, amps, NULL, NULL);

    info(ilInfo, "%s\n", pdo_str);
  }
}
