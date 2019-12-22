/*******************************************************************************
 *
 *  name: upd-touch-arduino.ino
 *  date: Dec 13, 2019
 *  auth: andrew
 *  desc:
 *
 ******************************************************************************/

// ----------------------------------------------------------------- includes --

#include <Wire.h>

#include <libstusb4500.h>

#include "ina260.h"
#include "ili9341.h"
#include "global.h"

// ---------------------------------------------------------- private defines --

/* nothing */

// ----------------------------------------------------------- private macros --

/* nothing */

// ------------------------------------------------------------ private types --

/* nothing */

// ------------------------------------------------------- exported variables --

/* nothing */

// -------------------------------------------------------- private variables --

ina260_t           *vmon;
ili9341_t          *screen;
stusb4500_device_t *usbpd;

// ---------------------------------------------- private function prototypes --

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

// ------------------------------------------------------- exported functions --

void setup()
{
  init_gpio();
  init_peripherals();
}

void loop()
{
  ili9341_draw(screen);
  stusb4500_process_events(usbpd);
}

// -------------------------------------------------------- private functions --
