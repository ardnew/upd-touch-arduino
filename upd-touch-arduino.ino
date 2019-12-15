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

#include "neopixel.h"
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

neopixel_t *pixel;
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
  neopixel_update(pixel);
  stusb4500_process_events(usbpd);
}

// -------------------------------------------------------- private functions --
