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

        dev->touch->begin();

        ili9341_fill_screen(dev, ILI9341_BLACK);
      }
    }
  }

  return dev;
}

void ili9341_draw(ili9341_t *dev)
{
  if (NULL == dev)
    { return; }

  if (itpPressed == dev->touch_pressed) {
    TS_Point p = dev->touch->getPoint();
    info(ilInfo, "pressed = ( %u, %u ) [%u]\n", p.x, p.y, p.z);
  }
}

void ili9341_fill_rect(ili9341_t *dev, ili9341_color_t color,
    uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  // adjust bounds to clip any area outside screen dimensions:
  //  1. rect origin beyond screen dimensions, nothing to draw
  if ((x >= dev->screen_size.width) || (y >= dev->screen_size.height))
    { return; }
  //  2. rect width beyond screen width, reduce rect width
  if ((x + w - 1) >= dev->screen_size.width)
    { w = dev->screen_size.width - x; }
  //  3. rect height beyond screen height, reduce rect height
  if ((y + h - 1) >= dev->screen_size.height)
    { h = dev->screen_size.height - y; }

}

void ili9341_fill_screen(ili9341_t *dev, ili9341_color_t color)
{
  ili9341_fill_rect(dev, color,
      0, 0, dev->screen_size.width, dev->screen_size.height);
}

void ili9341_draw_char(ili9341_t *dev, uint16_t x, uint16_t y,
    ili9341_color_t fg_color, ili9341_color_t bg_color, char ch)
{

}

void ili9341_draw_string(ili9341_t *dev, uint16_t x, uint16_t y,
    ili9341_color_t fg_color, ili9341_color_t bg_color,
    ili9341_word_wrap_t word_wrap, char str[])
{

}

void ili9341_touch_interrupt(ili9341_t *dev)
{
  // read the new/incoming state of the touch screen
  ili9341_touch_pressed_t pressed = ili9341_touch_pressed(dev);

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

ili9341_touch_pressed_t ili9341_touch_pressed(ili9341_t *dev)
{
  if (NULL == dev)
    { return itpNONE; }

  // no filtering or normalization, single read of the interrupt pin
  if (0U == digitalRead(dev->touch_interrupt_pin))
    { return itpPressed; }
  else
    { return itpNotPressed; }
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
