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
#include <font_DroidSans.h>
#include <font_Nunito.h>

#include <libstusb4500.h>

#include "ili9341.h"
#include "global.h"

// ---------------------------------------------------------- private defines --

#define __ILI9341_COLOR_BACKGROUND__ ILI9341_BLACK

// ----------------------------------------------------------- private macros --

/* nothing */

// ------------------------------------------------------------ private types --

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

        dev->bg_color = __ILI9341_COLOR_BACKGROUND__;

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

        dev->touch->begin();
        dev->touch->setRotation(orientation);
      }
    }
  }

  return dev;
}

void ili9341_draw(ili9341_t *dev)
{
  if (NULL == dev)
    { return; }

  ili9341_two_dimension_t position;
  uint8_t pressure;

  // read the touch screen only one time per draw cycle. re-use the results as
  // arguments to all subroutines.
  ili9341_touch_pressed_t pressed =
    ili9341_touch_pressed(dev, &position, &pressure);

}

void ili9341_button_draw(
    ili9341_button_t *button,
    ili9341_touch_pressed_t pressed,
    ili9341_two_dimension_t position, uint8_t pressure)
{
  if (NULL == button)
    { return; }

  // the state of the touch screen for this current draw cycle
  if (pressed) {
    // the screen is being touched at this instant.

    // check if this cycle's touch event resides inside this button
    ili9341_point_contained_t contained =
        ili9341_button_contains(button, position);

    if (ipcContained == contained) {
      // this touch event resides inside this button's frame. check if this
      // button's current state (from previous cycle) indicates that it is
      // already being pressed.
      if (!(button->pressed)) {
        // the button was not pressed, so this cycle marks the first instant in
        // which it is being pressed. re-color the button accordingly.
        button->pressed = itpPressed;

        // it would be unfriendly to trigger touch event handlers when the
        // button transitions from not-touched to touched, as it does not give
        // the user an opportunity to cancel their touch (by dragging their
        // touch to a region outside of the button's bounding box). instead, in
        // the vast majority of cases, this event handler should not be used.
        // nevertheless, it is provided if needed for some reason.
        if (NULL != button->touch_down)
          { button->touch_down(button); }
      }
      else {
        // the button was already being pressed, no change to button necessary,
        // it is already colored/highlighted.
      }
    }
    else {
      // this touch event resides outside this button's frame. check if this
      // button's current state (from previous cycle) indicates that it was
      // previously being pressed.
      if (button->pressed) {
        // the button was being pressed, so this cycle marks the first instant
        // in which it is not being pressed. re-color the button accordingly.
        button->pressed = itpNotPressed;

        // in this case, the screen is being touched continuously as the button
        // transitioned from pressed to not-pressed. in other words, the user
        // dragged their touch from inside-to-outside of the button's bounding
        // box, indicating they did not want to trigger a touch event on the
        // button. therefore, do not call the event handler for button touches.
      }
      else {
        // the button was already not being pressed, no change to button
        // necessary, it is already not colored/highlighted.
      }
    }
  }
  else {
    // the screen is not being touched at this instant.

    // check if this button's current state (from previous cycle) indicates that
    // it was previously being pressed.
    if (button->pressed) {
      // the button was being pressed, so this cycle marks the firs instant in
      // which it is not being pressed. re-color the button accordingly.
      button->pressed = itpNotPressed;

      // in this case, the screen and button both transitioned from pressed to
      // not-pressed simultaneously. in other words, the user lifted their touch
      // off of the screen while pressing a button, indicating a normal button
      // touch event. therefore, call the event handler for button touches.
      if (NULL != button->touch_up)
        { button->touch_up(button); }
      info(ilInfo, "touched: %u\n", button->id);
    }
  }
}

ili9341_touch_pressed_t ili9341_touch_pressed(ili9341_t *dev,
    ili9341_two_dimension_t *position, uint8_t *pressure)
{
  static const uint16_t required = 64U;
  uint16_t samples = 0U;

  float avg_x = 0.0F, avg_y = 0.0F, avg_z = 0.0F;
  uint16_t x, y;
  uint8_t z;

  for (uint16_t i = 0; i < required; ++i) {

    if (dev->touch->tirqTouched()) {
      if (dev->touch->touched()) {
        dev->touch->readData(&x, &y, &z);
        avg_x += (float)x;
        avg_y += (float)y;
        avg_z += (float)z;
        ++samples;
      }
    }
  }

  if (samples == required) {
    // poor man's rounding
    position->x = (uint16_t)(avg_x / (float)samples + 0.5F);
    position->y = (uint16_t)(avg_y / (float)samples + 0.5F);
    *pressure   = ( uint8_t)(avg_z / (float)samples + 0.5F);
    return itpPressed;
  }
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

void ili9341_add_source_capability(
    ili9341_t *dev, uint8_t pdo_number,
    uint16_t voltage_mV, uint16_t current_mA, uint16_t current_max_mA,
    ili9341_button_callback_t touch_down, ili9341_button_callback_t touch_up)
{
  if (NULL == dev)
    { return; }

  ili9341_source_capability_button_t *cap;

  if (NULL != (cap = (ili9341_source_capability_button_t *)malloc(
      sizeof(ili9341_source_capability_button_t)))) {

    uint16_t count = dev->source_capability_button_count;

    cap->pdo_number     = pdo_number;
    cap->voltage_mV     = voltage_mV;
    cap->current_mA     = current_mA;
    cap->current_max_mA = current_max_mA;
                          // division first, hopefully to prevent overflow
    cap->power_W        = (voltage_mV / 1000U) * (current_mA / 1000U);

    dev->source_capability_button[dev->source_capability_button_count] = cap;
    ++(dev->source_capability_button_count);

    info(ilInfo, "added capability button (%p) with frame: {%ux%u}@{%u,%u}\n",
        cap,
        cap->button->origin.x, cap->button->origin.y,
        cap->button->size.width, cap->button->size.height);
  }
}

void ili9341_remove_all_source_capabilities(ili9341_t *dev)
{
  if (NULL == dev)
    { return; }

  for (uint8_t i = 0U; i < dev->source_capability_button_count; ++i) {
    ili9341_source_capability_button_t *cap = dev->source_capability_button[i];
    if (NULL != cap) {
      if (NULL != cap->button) {
        // then free up any resources
        free(cap->button);
      }
      free(cap);
    }
  }
  dev->source_capability_button_count = 0U;
}

// -------------------------------------------------------- private functions --

static ili9341_two_dimension_t ili9341_screen_size(
    ili9341_screen_orientation_t orientation)
{
  switch (orientation) {
    default:
    case isoBottom:
      return (ili9341_two_dimension_t){ { .width = 240U }, { .height = 320U } };
    case isoRight:
      return (ili9341_two_dimension_t){ { .width = 320U }, { .height = 240U } };
    case isoTop:
      return (ili9341_two_dimension_t){ { .width = 240U }, { .height = 320U } };
    case isoLeft:
      return (ili9341_two_dimension_t){ { .width = 320U }, { .height = 240U } };
  }
}

static ili9341_point_contained_t ili9341_button_contains(
    ili9341_button_t *button, ili9341_two_dimension_t point)
{
  if (NULL == button)
    { return ipcNONE; }

  ili9341_two_dimension_t min  = button->dev->touch_coordinate_min;
  ili9341_two_dimension_t max  = button->dev->touch_coordinate_max;
  ili9341_two_dimension_t size = button->dev->screen_size;

  uint16_t x, y;

  // translate the touch coordinates to screen coordinates based on orientation
  // and the calibrated touch screen extremes/edges
  switch (button->dev->orientation) {
    case isoPortrait:
    case isoPortraitFlip:
      x = (int16_t)map(point.x, min.x, max.x, size.width,  0);
      y = (int16_t)map(point.y, min.y, max.y, size.height, 0);
      break;

    case isoLandscape:
    case isoLandscapeFlip:
      x = (int16_t)map(point.x, min.x, max.x, size.height, 0);
      y = (int16_t)map(point.y, min.y, max.y, size.width,  0);
      break;

    default:
      return ipcNONE;
  }

  uint8_t contained =
    ((x >= button->origin.x) && (x <= (button->origin.x + button->size.width)))
     &&
    ((y >= button->origin.y) && (y <= (button->origin.y + button->size.height)));

  if (0U != contained)
    { return ipcContained; }
  else
    { return ipcNotContained; }
}
