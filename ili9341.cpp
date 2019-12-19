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

#include <libstusb4500.h>

#include "ili9341.h"
#include "global.h"

// ---------------------------------------------------------- private defines --

#define __ILI9341_COLOR_BACKGROUND__ ILI9341_BLACK

#define __ILI9341_BUTTON_RADIUS__              5
#define __ILI9341_BUTTON_FG_COLOR_ENABLED__   ILI9341_WHITE
#define __ILI9341_BUTTON_BG_COLOR_ENABLED__   ILI9341_BLUE
#define __ILI9341_BUTTON_FG_COLOR_DISABLED__  ILI9341_LIGHTGREY
#define __ILI9341_BUTTON_BG_COLOR_DISABLED__  ILI9341_DARKGREY
#define __ILI9341_BUTTON_FG_COLOR_TOUCHED__   ILI9341_BLUE
#define __ILI9341_BUTTON_BG_COLOR_TOUCHED__   ILI9341_WHITE

// ----------------------------------------------------------- private macros --

/* nothing */

// ------------------------------------------------------------ private types --

struct ili9341_button
{
  ili9341_t               *dev;
  ili9341_two_dimension_t  origin;
  ili9341_two_dimension_t  size;
  ili9341_touch_pressed_t  was_pressed;
  char     *text;
  uint8_t   text_len;
  uint16_t  id;
  uint16_t  radius;
  uint16_t  fg_color_enb; // enabled color
  uint16_t  bg_color_enb;
  uint16_t  fg_color_dis; // disabled color
  uint16_t  bg_color_dis;
  uint16_t  fg_color_tch; // touched color
  uint16_t  bg_color_tch;

  ili9341_button_callback_t touch_down;
  ili9341_button_callback_t touch_up;
};

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

  uint8_t source_capability_button_count;
  ili9341_button_t *source_capability_button[__STUSB4500_NVM_SOURCE_PDO_MAX__];

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
static uint8_t ili9341_button_contains(ili9341_button_t *button,
    ili9341_two_dimension_t point);
static void ili9341_button_layout(ili9341_button_t *button,
    uint16_t fg_color, uint16_t bg_color);

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

        dev->source_capability_button_count = 0;
        for (uint8_t i = 0; i < __STUSB4500_NVM_SOURCE_PDO_MAX__; ++i)
          { dev->source_capability_button[i] = NULL; }

        dev->source_capability_button_count = 1;
        dev->source_capability_button[0] = ili9341_button_new(
            dev, "Text", 0,
            0, 0, dev->screen_size.width, dev->screen_size.height / 3,
            NULL, NULL);

        dev->touch_pressed       = itpNONE;
        dev->touch_pressed_begin = NULL;
        dev->touch_pressed_end   = NULL;

        dev->tft->begin();
        dev->tft->setRotation(orientation);
        dev->tft->setTextWrap(0U); // manually handle wrapping if needed
        dev->tft->fillScreen(dev->bg_color);
        dev->tft->setFont(DroidSansMono_10);
        dev->tft->setTextSize(1U);

        dev->touch->begin();
      }
    }
  }

  return dev;
}

ili9341_button_t *ili9341_button_new(
    ili9341_t *dev, char * const text, uint16_t id,
    uint16_t x, uint16_t y, uint16_t width, uint16_t height,
    ili9341_button_callback_t touch_down, ili9341_button_callback_t touch_up)
{
  ili9341_button_t *button = NULL;

  if ((NULL != dev) && (NULL != text)) {

    if (NULL != (button = (ili9341_button_t *)malloc(sizeof(ili9341_button_t)))) {

      button->dev              = dev;
      button->id               = id;
      button->origin.x         = x;
      button->origin.y         = y;
      button->size.width       = width;
      button->size.height      = height;
      button->was_pressed      = itpPressed;
      button->text_len         = strlen(text);
      button->text             = (char *)calloc(button->text_len + 1, sizeof(char));
      strncpy(button->text, text, button->text_len);
      button->radius           = 5U;
      button->fg_color_enb     = __ILI9341_BUTTON_FG_COLOR_ENABLED__;
      button->bg_color_enb     = __ILI9341_BUTTON_BG_COLOR_ENABLED__;
      button->fg_color_dis     = __ILI9341_BUTTON_FG_COLOR_DISABLED__;
      button->bg_color_dis     = __ILI9341_BUTTON_BG_COLOR_DISABLED__;
      button->fg_color_tch     = __ILI9341_BUTTON_FG_COLOR_TOUCHED__;
      button->bg_color_tch     = __ILI9341_BUTTON_BG_COLOR_TOUCHED__;
      button->touch_down       = touch_down;
      button->touch_up         = touch_up;
    }
  }

  return button;
}

void ili9341_draw(ili9341_t *dev)
{
  if (NULL == dev)
    { return; }

  ili9341_touch_pressed_t pressed = itpNotPressed;
  if (dev->touch->touched())
    { pressed = itpPressed; }

  for (uint8_t i = 0; i < dev->source_capability_button_count; ++i) {
    if (NULL != dev->source_capability_button[i])
      { ili9341_button_draw(dev->source_capability_button[i], pressed); }
  }
}

void ili9341_button_draw(ili9341_button_t *button, ili9341_touch_pressed_t pressed)
{
  uint16_t x, y;
  uint8_t z;

  if (pressed) {

    button->dev->touch->readData(&x, &y, &z);

    ili9341_touch_pressed_t button_pressed =
        ili9341_button_contains(button, {x, y});

    if (itpPressed == button_pressed) {

      if (!(button->was_pressed)) {
        info(ilInfo, "draw button %u touched\n", button->id);
        ili9341_button_layout(button, button->fg_color_tch, button->bg_color_tch);
      }
      else {
        // button hold
      }
      // call event handler button press down
    }
    else {
      if (button->was_pressed) {
        // draw normal colors, do not call event handler
        info(ilInfo, "draw button %u not touched\n", button->id);
        ili9341_button_layout(button, button->fg_color_enb, button->bg_color_enb);
      }
      else {
        // screen hold
      }
    }
    button->was_pressed = button_pressed;
  }
  else {
    if (button->was_pressed) {
      info(ilInfo, "draw button %u not touched\n", button->id);
      ili9341_button_layout(button, button->fg_color_enb, button->bg_color_enb);
    }
    button->was_pressed = itpNotPressed;
  }
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

static uint8_t ili9341_button_contains(ili9341_button_t *button,
    ili9341_two_dimension_t point)
{

  if (NULL == button)
    { return 0U; }

  ili9341_two_dimension_t min  = button->dev->touch_coordinate_min;
  ili9341_two_dimension_t max  = button->dev->touch_coordinate_max;
  ili9341_two_dimension_t size = button->dev->screen_size;

  uint16_t x, y;

  switch (button->dev->orientation) {
    case isoPortrait:
    case isoPortraitFlip:
      y = (int16_t)map(point.x, min.x, max.x, size.width,  0);
      x = (int16_t)map(point.y, min.y, max.y, size.height, 0);
      break;

    case isoLandscape:
    case isoLandscapeFlip:
      x = (int16_t)map(point.x, min.x, max.x, size.height, 0);
      y = (int16_t)map(point.y, min.y, max.y, size.width,  0);
      break;

    default:
      return 0U;
  }

  return
    ((x >= button->origin.x) && (x <= (button->origin.x + button->size.width)))
     &&
    ((y >= button->origin.y) && (y <= (button->origin.y + button->size.height)));
}

static void ili9341_button_layout(ili9341_button_t *button,
    uint16_t fg_color, uint16_t bg_color)
{
  button->dev->tft->fillRoundRect(
    button->origin.x, button->origin.y, button->size.width, button->size.height,
    button->radius, bg_color);

  button->dev->tft->setTextColor(fg_color);
  uint16_t w = button->dev->tft->measureTextWidth(button->text);
  uint16_t h = button->dev->tft->measureTextHeight(button->text);

  button->dev->tft->setCursor(
    button->origin.x + (button->size.width  - w) / 2,
    button->origin.y + (button->size.height - h) / 2
  );
  button->dev->tft->print(button->text);

}
