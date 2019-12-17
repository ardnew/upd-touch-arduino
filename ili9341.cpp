/*******************************************************************************
 *
 *  name: ili9341.cpp
 *  date: Dec 15, 2019
 *  auth: andrew
 *  desc:
 *
 ******************************************************************************/

// ----------------------------------------------------------------- includes --

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

/* nothing */

// ---------------------------------------------- private function prototypes --

static ili9341_two_dimension_t ili9341_screen_size(
    ili9341_screen_orientation_t orientation);

// ------------------------------------------------------- exported functions --

ili9341_t *ili9341_new(
    Adafruit_ILI9341    *tft,
    XPT2046_Touchscreen *touch,
    ili9341_screen_orientation_t orientation,
    uint16_t touch_interrupt_pin,
    uint16_t touch_coordinate_min_x,
    uint16_t touch_coordinate_min_y,
    uint16_t touch_coordinate_max_x,
    uint16_t touch_coordinate_max_y)
{
  ili9341_t *dev = NULL;

  if ((orientation > isoNONE) && (orientation < isoCOUNT)) {

    // we must either NOT support the touch interface, OR we must have valid
    // touch interface parameters
    if ( (touch_coordinate_min_x < touch_coordinate_max_x) &&
         (touch_coordinate_min_y < touch_coordinate_max_y) ) {

      if (NULL != (dev = (ili9341_t *)malloc(sizeof(ili9341_t)))) {

        dev->tft   = tft;
        dev->touch = touch;

        dev->orientation = orientation;
        dev->screen_size = ili9341_screen_size(orientation);

        dev->touch_interrupt_pin = touch_interrupt_pin;
        dev->touch_coordinate_min = (ili9341_two_dimension_t){
            {touch_coordinate_min_x},
            {touch_coordinate_min_y}
        };
        dev->touch_coordinate_max = (ili9341_two_dimension_t){
            {touch_coordinate_max_x},
            {touch_coordinate_max_y}
        };

        dev->touch_pressed       = itpNONE;
        dev->touch_pressed_begin = NULL;
        dev->touch_pressed_end   = NULL;


        dev->tft->begin();
        dev->tft->fillScreen(ILI9341_BLACK);

        dev->touch->begin();
      }
    }
  }

  return dev;
}

void ili9341_draw(ili9341_t *dev)
{
  if (NULL == dev)
    { return; }


  // read the new/incoming state of the touch screen
  ili9341_two_dimension_t pos;
  uint8_t pressure;
  ili9341_touch_pressed_t pressed = ili9341_touch_pressed(dev, &pos, &pressure);

  // switch path based on existing/prior state of the touch screen
  switch (dev->touch_pressed) {
    case itpNotPressed:
      if (itpPressed == pressed) {
        // state change, start of press
        if (NULL != dev->touch_pressed_begin)
          { dev->touch_pressed_begin(dev); }
      }
      break;

    case itpPressed:
      if (itpNotPressed == pressed) {
        // state change, end of press
        if (NULL != dev->touch_pressed_end)
          { dev->touch_pressed_end(dev); }
      }
      break;

    default:
      break;
  }

  // update the internal state with current state of touch screen
  if (pressed != dev->touch_pressed) {
    dev->touch_pressed = pressed;
  }
}

ili9341_touch_pressed_t ili9341_touch_pressed(ili9341_t *dev,
    ili9341_two_dimension_t *pos, uint8_t *pressure)
{
  if (NULL == dev)
    { return itpNONE; }

  // initial fast read pass
  if (dev->touch->tirqTouched()) {
    // if initial check succeeds, do a normalized read
    if (dev->touch->touched()) {
      dev->touch->readData(&(pos->x), &(pos->y), pressure);
      return itpPressed;
    }
  }

  pos->x    = 0;
  pos->y    = 0;
  *pressure = 0;

  return itpNotPressed;
}

void ili9341_set_touch_pressed_begin(ili9341_t *dev, ili9341_touch_callback_t callback)
{
  if ((NULL != dev) && (NULL != callback)) {
    dev->touch_pressed_begin = callback;
  }
}

void ili9341_set_touch_pressed_end(ili9341_t *dev, ili9341_touch_callback_t callback)
{
  if ((NULL != dev) && (NULL != callback)) {
    dev->touch_pressed_end = callback;
  }
}

// -------------------------------------------------------- private functions --

static ili9341_two_dimension_t ili9341_screen_size(
    ili9341_screen_orientation_t orientation)
{
  switch (orientation) {
    default:
    case isoDown:
      return (ili9341_two_dimension_t){ { .width = 240U }, { .height = 320U } };
    case isoRight:
      return (ili9341_two_dimension_t){ { .width = 320U }, { .height = 240U } };
    case isoUp:
      return (ili9341_two_dimension_t){ { .width = 240U }, { .height = 320U } };
    case isoLeft:
      return (ili9341_two_dimension_t){ { .width = 320U }, { .height = 240U } };
  }
}
