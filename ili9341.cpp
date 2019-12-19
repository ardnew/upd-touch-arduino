/*******************************************************************************
 *
 *  name: ili9341.cpp
 *  date: Dec 15, 2019
 *  auth: andrew
 *  desc:
 *
 ******************************************************************************/

// ----------------------------------------------------------------- includes --

#include <font_DroidSansMono.h>

#include "ili9341.h"
#include "global.h"

// ---------------------------------------------------------- private defines --

#define __SCROLL_TOP__ 120

// ----------------------------------------------------------- private macros --

/* nothing */

// ------------------------------------------------------------ private types --

typedef uint8_t ili9341_touch_pressure_t;

typedef enum
{
  itcNONE = -1,
  itcDidNotChange, // = 0
  itcDidChange,    // = 1
  itcCOUNT         // = 2
}
ili9341_touch_changed_t;

typedef struct
{
  ili9341_touch_pressed_t  pressed;
  ili9341_two_dimension_t  position;
  ili9341_touch_pressure_t pressure;
  ili9341_touch_changed_t  changed;
}
ili9341_touch_status_t;

struct ili9341
{
  ILI9341_t3          *tft;
  XPT2046_Touchscreen *touch;

  ili9341_screen_orientation_t orientation;
  ili9341_two_dimension_t      screen_size;

  uint16_t bg_color;

  uint16_t touch_interrupt_pin;
  ili9341_two_dimension_t touch_coordinate_min;
  ili9341_two_dimension_t touch_coordinate_max;

  volatile ili9341_touch_pressed_t touch_pressed;
  ili9341_touch_callback_t touch_pressed_begin;
  ili9341_touch_callback_t touch_pressed_end;
};

// ------------------------------------------------------- exported variables --

/* nothing */

// -------------------------------------------------------- private variables --

/* nothing */

// ---------------------------------------------- private function prototypes --

static ili9341_two_dimension_t ili9341_screen_size(
    ili9341_screen_orientation_t orientation);
static void ili9341_init_scroll_box(ili9341_t *dev, uint16_t top, uint8_t text_size);

static ili9341_touch_status_t ili9341_update_touch_status(ili9341_t *dev);

// ------------------------------------------------------- exported functions --

ili9341_t *ili9341_new(
    ILI9341_t3          *tft,
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

        dev->bg_color = ILI9341_BLACK;

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
        dev->tft->setRotation(orientation);
        dev->tft->setTextWrap(0U); // manually handle wrapping if needed
        dev->tft->fillScreen(dev->bg_color);
        dev->tft->setFont(DroidSansMono_10);
        dev->tft->setTextSize(1U);

        dev->tft->setCursor(0U, 10U);
        dev->tft->printf("initializing (%u)\nfontCapHeight = %u\nfontLineSpace = %u\nfontGap = %u\n",
            millis(), dev->tft->fontCapHeight(), dev->tft->fontLineSpace(), dev->tft->fontGap());

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

  ili9341_update_touch_status(dev);
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

static ili9341_touch_status_t ili9341_update_touch_status(ili9341_t *dev)
{
  ili9341_touch_status_t status;

  // read the new/incoming state of the touch screen
  status.pressed =
      ili9341_touch_pressed(dev, &(status.position), &(status.pressure));

  // switch path based on existing/prior state of the touch screen
  switch (dev->touch_pressed) {
    case itpNotPressed:
      if (itpPressed == status.pressed) {
        // state change, start of press
        if (NULL != dev->touch_pressed_begin)
          { dev->touch_pressed_begin(dev); }
      }
      break;

    case itpPressed:
      if (itpNotPressed == status.pressed) {
        // state change, end of press
        if (NULL != dev->touch_pressed_end)
          { dev->touch_pressed_end(dev); }
      }
      break;

    default:
      break;
  }

  // update the internal state with current state of touch screen
  if (status.pressed != dev->touch_pressed) {
    status.changed = itcDidChange;
    dev->touch_pressed = status.pressed;
  }
  else {
    status.changed = itcDidNotChange;
  }
}
