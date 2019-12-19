/*******************************************************************************
 *
 *  name: ili9341.h
 *  date: Dec 15, 2019
 *  auth: andrew
 *  desc:
 *
 ******************************************************************************/

#ifndef __ILI9341_H__
#define __ILI9341_H__

// ----------------------------------------------------------------- includes --

#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

// ------------------------------------------------------------------ defines --

/* nothing */

// ------------------------------------------------------------------- macros --

#define __ILI9341_COLOR565(r, g, b) \
    (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

// ----------------------------------------------------------- exported types --

typedef struct ili9341 ili9341_t;

typedef struct
{
  union
  {
    uint16_t x;
    uint16_t width;
  };
  union
  {
    uint16_t y;
    uint16_t height;
  };
}
ili9341_two_dimension_t;

typedef uint16_t ili9341_color_t;

typedef enum
{
  // orientation is based on position of board pins when looking at the screen
  isoNONE                     = -1,
  isoDown,   isoPortrait      = isoDown,  // = 0
  isoRight,  isoLandscape     = isoRight, // = 1
  isoUp,     isoPortraitFlip  = isoUp,    // = 2
  isoLeft,   isoLandscapeFlip = isoLeft,  // = 3
  isoCOUNT                                // = 4
}
ili9341_screen_orientation_t;

typedef enum
{
  itpNONE = -1,
  itpNotPressed, // = 0
  itpPressed,    // = 1
  itpCOUNT       // = 2
}
ili9341_touch_pressed_t;

typedef enum
{
  iwwNONE = -1,
  iwwTruncate, // = 0
  iwwCharWrap, // = 1
  iwwWordWrap, // = 2
  iwwCOUNT     // = 3
}
ili9341_word_wrap_t;

typedef enum
{
  issNONE = -1,
  issDisplayTFT,  // = 0
  issTouchScreen, // = 1
  issCOUNT        // = 2
}
ili9341_spi_slave_t;

typedef void (*ili9341_touch_callback_t)(ili9341_t *);

typedef enum
{
  isNONE = -1,
  isOK,    // = 0
  isERROR, // = 1
  isCOUNT  // = 2
}
ili9341_status_t;

// ------------------------------------------------------- exported variables --

/* nothing */

// ------------------------------------------------------- exported functions --

#ifdef __cplusplus
extern "C" {
#endif

ili9341_t *ili9341_new(
    ILI9341_t3          *tft,
    XPT2046_Touchscreen *touch,
    ili9341_screen_orientation_t orientation,
    uint16_t touch_interrupt_pin,
    uint16_t touch_coordinate_min_x,
    uint16_t touch_coordinate_min_y,
    uint16_t touch_coordinate_max_x,
    uint16_t touch_coordinate_max_y);

void ili9341_draw(ili9341_t *dev);

ili9341_touch_pressed_t ili9341_touch_pressed(ili9341_t *dev,
    ili9341_two_dimension_t *pos, uint8_t *pressure);

void ili9341_set_touch_pressed_begin(ili9341_t *dev, ili9341_touch_callback_t callback);
void ili9341_set_touch_pressed_end(ili9341_t *dev, ili9341_touch_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif // __ILI9341_H__