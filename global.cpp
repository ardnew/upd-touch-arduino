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

#include <Adafruit_INA260.h>
#include <ILI9341_t3.h>
#include <XPT2046_Calibrated.h>
#include <Arduino.h>

#include <libstusb4500.h>

#include "ina260.h"
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

extern ina260_t *vmon;
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

void toggle_power_pressed(ili9341_t *tft, void *data);
void cycle_power_pressed(ili9341_t *tft, void *data);
void set_power_pressed(ili9341_t *tft, void *data);
void get_source_cap_pressed(ili9341_t *tft, void *data);

void cable_attached(stusb4500_device_t *usb);
void cable_detached(stusb4500_device_t *usb);
void capabilities_received(stusb4500_device_t *usb);

void refresh_source_capabilities(stusb4500_device_t *usb);
void refresh_sink_capabilities(stusb4500_device_t *usb);

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

  info(ilInfo, "initializing ...");

  // --
  // I2C (bus master)
  Wire.begin();
  Wire.setClock(__STUSB4500_I2C_CLOCK_FREQUENCY__);

  // --
  // INA260 (voltage/current sensor)
  vmon = ina260_new(new Adafruit_INA260());

  // --
  // ILI9341 (TFT touch screen)
  screen = ili9341_new(
      new ILI9341_t3(
          __GPIO_TFT_CS_PIN__, __GPIO_TFT_DC_PIN__),
      new XPT2046_Calibrated(
          __GPIO_TOUCH_CS_PIN__, __GPIO_TOUCH_IRQ_PIN__),
      vmon,
      isoLeft,
      __GPIO_TOUCH_IRQ_PIN__);

  ili9341_set_command_pressed(screen, icTogglePower, toggle_power_pressed);
  ili9341_set_command_pressed(screen, icCyclePower, cycle_power_pressed);
  ili9341_set_command_pressed(screen, icSetPower, set_power_pressed);
  ili9341_set_command_pressed(screen, icGetSourceCap, get_source_cap_pressed);

  ili9341_draw_template(screen);
  ili9341_init_request_source_capabilities(screen);

  // --
  // STUSB4500 (zoinks!)
  usbpd = stusb4500_device_new(
      &Wire, __STUSB4500_I2C_SLAVE_BASE_ADDR__, __GPIO_USBPD_RST_PIN__);

  stusb4500_set_cable_attached(usbpd, cable_attached);
  stusb4500_set_cable_detached(usbpd, cable_detached);
  stusb4500_set_source_capabilities_received(usbpd, capabilities_received);

  info(ilInfo, "initialization complete");

  // --

  stusb4500_device_init(usbpd);
  delay(500);
  stusb4500_set_power(usbpd, 5000, 3000);
  //stusb4500_select_power_usb_default(usbpd);
  delay(200);
  stusb4500_pdo_description_t req = stusb4500_power_requested(usbpd);
  if (__STUSB4500_NVM_INVALID_PDO_INDEX__ != req.number) {
    info(ilInfo, "requested capability: (%d) %u mV, %u mA, (%u mA MAX)",
        req.number, req.voltage_mv, req.current_ma, req.max_current_ma);
    ili9341_set_sink_capability_requested(
        screen, req.number, req.voltage_mv, req.current_ma);
  }
  else {
    info(ilWarn, "no capability requested!");
  }

  stusb4500_cable_connected_t conn = stusb4500_cable_connected(usbpd);
  ili9341_redraw_all_sink_capabilities(screen, (uint8_t)conn);
}

void info(info_level_t level, const char *fmt, ...)
{
  static char buff[__INFO_LENGTH_MAX__] = { 0 };

  va_list arg;
  va_start(arg, fmt);
  vsnprintf(buff, __INFO_LENGTH_MAX__, fmt, arg);
  va_end(arg);

  Serial.print(DEBUG_LEVEL_PREFIX[level]);
  Serial.println(buff);
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
  info(ilInfo, "touch began");
}

void touch_end(ili9341_t *tft)
{
  info(ilInfo, "touch ended");
}

void toggle_power_pressed(ili9341_t *tft, void *data)
{
  static bool power_enabled = true;

  if (NULL != tft) {
    if (power_enabled) {
      info(ilInfo, "powering off");
      stusb4500_power_toggle(usbpd, sptPowerOff, srwNONE);
      ili9341_remove_all_source_capabilities(screen);
      ili9341_remove_all_sink_capabilities(screen);
    }
    else {
      info(ilInfo, "powering on");
      stusb4500_power_toggle(usbpd, sptPowerOn, srwDoNotWait);
    }
    power_enabled = !power_enabled;
  }
}

void cycle_power_pressed(ili9341_t *tft, void *data)
{
  info(ilInfo, "cycling power");
  if (NULL != tft) {
    ili9341_remove_all_source_capabilities(screen);
    ili9341_remove_all_sink_capabilities(screen);
    stusb4500_hard_reset(usbpd, srwDoNotWait);
  }
}

void set_power_pressed(ili9341_t *tft, void *data)
{
  if ((NULL != tft) && (NULL != data) && (NULL != usbpd)) {
    uint8_t pdo = *(uint8_t *)data;
    if (pdo < usbpd->usbpd_status.pdo_src_count) {
      uint32_t volts = usbpd->usbpd_status.pdo_src[pdo].fix.Voltage * 50U;
      uint32_t amps  = usbpd->usbpd_status.pdo_src[pdo].fix.Max_Operating_Current * 10U;
      uint32_t watts = (uint32_t)((float)volts / 1000.0F * (float)amps / 1000.0F);

      stusb4500_set_power(usbpd, volts, amps);
      refresh_sink_capabilities(usbpd);
    }
  }
}

void get_source_cap_pressed(ili9341_t *tft, void *data)
{
  if (NULL != tft) {
    stusb4500_soft_reset(usbpd, srwWaitReady);
    ili9341_init_request_source_capabilities(screen);
    delay(200);
    stusb4500_select_power_usb_default(usbpd);
    delay(500);
    stusb4500_get_source_capabilities(usbpd);
  }
}


void cable_attached(stusb4500_device_t *usb)
{
  info(ilInfo, "cable attached");
  ili9341_init_request_source_capabilities(screen);
  delay(200);
  stusb4500_select_power_usb_default(usb);
  delay(500);
  stusb4500_get_source_capabilities(usb);
}

void cable_detached(stusb4500_device_t *usb)
{
  info(ilInfo, "cable detached");
  ili9341_remove_all_source_capabilities(screen);
  ili9341_remove_all_sink_capabilities(screen);
  //stusb4500_hard_reset(usb, srwDoNotWait);
}

void capabilities_received(stusb4500_device_t *usb)
{
  info(ilInfo, "capabilities received");

  refresh_source_capabilities(usb);
  refresh_sink_capabilities(usb);
}

void refresh_source_capabilities(stusb4500_device_t *usb)
{
  uint32_t volts;
  uint32_t amps;
  uint32_t watts;

  ili9341_remove_all_source_capabilities(screen);

  info(ilInfo, "source capabilities:");

  for (uint8_t i = 0; i < usb->usbpd_status.pdo_src_count; ++i) {

    volts = usb->usbpd_status.pdo_src[i].fix.Voltage * 50U;
    amps  = usb->usbpd_status.pdo_src[i].fix.Max_Operating_Current * 10U;
    watts = (uint32_t)((float)volts / 1000.0F * (float)amps / 1000.0F);

    ili9341_add_source_capability(screen, i, volts, amps);

    info(ilInfo, " - [%u] %2lu mV, %1lu mA (%3lu W)",
        i + 1, volts, amps, watts);
  }
}

void refresh_sink_capabilities(stusb4500_device_t *usb)
{
  uint32_t volts;
  uint32_t amps;
  uint32_t watts;

  ili9341_remove_all_sink_capabilities(screen);

  info(ilInfo, "sink capabilities:");

  for (uint8_t i = 0; i < usb->usbpd_status.pdo_snk_count; ++i) {

    volts = usb->usbpd_status.pdo_snk[i].fix.Voltage * 50U;
    amps  = usb->usbpd_status.pdo_snk[i].fix.Operational_Current * 10U;
    watts = (uint32_t)((float)volts / 1000.0F * (float)amps / 1000.0F);

    ili9341_set_sink_capability(screen, i, volts, amps);

    info(ilInfo, " - [%u] %2lu mV, %1lu mA (%3lu W)",
        i + 1, volts, amps, watts);
  }

  stusb4500_pdo_description_t req = stusb4500_power_requested(usbpd);
  if (__STUSB4500_NVM_INVALID_PDO_INDEX__ != req.number) {
    info(ilInfo, "requested capability: (%d) %u mV, %u mA, (%u mA MAX)",
        req.number, req.voltage_mv, req.current_ma, req.max_current_ma);
    ili9341_set_sink_capability_requested(
        screen, req.number, req.voltage_mv, req.current_ma);
  }
  else {
    info(ilWarn, "no capability requested!");
  }

  stusb4500_cable_connected_t conn = stusb4500_cable_connected(usbpd);
  ili9341_redraw_all_sink_capabilities(screen, (uint8_t)conn);
}