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
#include <XPT2046_Calibrated.h>
#include <SPI.h>

#include "ina260.h"

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

typedef ili9341_two_dimension_t ili9341_point_t;
typedef ili9341_two_dimension_t ili9341_size_t;

typedef struct
{
  ili9341_point_t origin;
  ili9341_size_t  size;
}
ili9341_frame_t;

typedef enum
{
  ipcNONE = -1,
  ipcNotContained, // = 0
  ipcContained,    // = 1
  ipcCOUNT         // = 2
}
ili9341_point_contained_t;

typedef uint16_t ili9341_color_t;

typedef enum
{
  // orientation is based on position of board pins when looking at the screen
  isoNONE                     = -1,
  isoBottom, isoPortrait      = isoBottom, // = 0
  isoRight,  isoLandscape     = isoRight,  // = 1
  isoTop,    isoPortraitFlip  = isoTop,    // = 2
  isoLeft,   isoLandscapeFlip = isoLeft,   // = 3
  isoCOUNT                                 // = 4
}
ili9341_orientation_t;

typedef enum
{
  itpNONE = -1,
  itpNotPressed, // = 0
  itpPressed,    // = 1
  itpCOUNT       // = 2
}
ili9341_touch_pressed_t;

typedef uint8_t ili9341_touch_pressure_t;

typedef enum
{
  itcNONE = -1,
  itcDidNotChange, // = 0
  itcDidChange,    // = 1
  itcCOUNT         // = 2
}
ili9341_touch_changed_t;

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

typedef enum
{
  isNONE = -1,
  isOK,    // = 0
  isERROR, // = 1
  isCOUNT  // = 2
}
ili9341_status_t;

typedef uint32_t ili9341_command_mask_t;

typedef enum
{
  icNONE         = 0U,
  icTogglePower  = 1 << 0U,
  icCyclePower   = 1 << 1U,
  icGetSourceCap = 1 << 2U,
  icSetPower     = 1 << 3U,
  icCOUNT        =      4U
}
ili9341_command_t;

typedef void (*ili9341_command_callback_t)(ili9341_t *, void *);

// ------------------------------------------------------- exported variables --

extern ili9341_point_t const POINT_INVALID;
extern ili9341_size_t const SIZE_INVALID;
extern ili9341_frame_t const FRAME_INVALID;

// ------------------------------------------------------- exported functions --

#ifdef __cplusplus
extern "C" {
#endif

ili9341_t *ili9341_new(
    ILI9341_t3            *tft,
    XPT2046_Calibrated    *touch,
    ina260_t              *vmon,
    ili9341_orientation_t  orientation,
    uint16_t touch_interrupt_pin);

inline bool ili9341_point_equals(ili9341_point_t a, ili9341_point_t b)
  { return (a.x == b.x) && (a.y == b.y); }
inline bool ili9341_point_valid(ili9341_point_t p)
  { return !ili9341_point_equals(p, POINT_INVALID); }

inline bool ili9341_size_equals(ili9341_size_t a, ili9341_size_t b)
  { return (a.width == b.width) && (a.height == b.height); }
inline bool ili9341_size_valid(ili9341_size_t s)
  { return !ili9341_size_equals(s, SIZE_INVALID); }

inline bool ili9341_frame_equals(ili9341_frame_t *a, ili9341_frame_t *b)
  { return ili9341_point_equals(a->origin, b->origin) &&
            ili9341_size_equals(a->size, b->size); }
inline bool ili9341_frame_valid(ili9341_frame_t *f)
  { return !ili9341_frame_equals(f, (ili9341_frame_t *)&FRAME_INVALID); }

void ili9341_draw_template(ili9341_t *dev);
void ili9341_draw(ili9341_t *dev);

ili9341_touch_pressed_t ili9341_touch_pressed(
    ili9341_t *dev, ili9341_point_t *pos, uint8_t *pressure);
void ili9341_set_command_pressed(ili9341_t *dev,
    ili9341_command_mask_t command, ili9341_command_callback_t callback);

void ili9341_add_source_capability(
    ili9341_t *dev, uint8_t number, uint16_t voltage_mV, uint16_t current_mA);
void ili9341_init_request_source_capabilities(ili9341_t *dev);
void ili9341_remove_all_source_capabilities(ili9341_t *dev);

void ili9341_set_sink_capability(
    ili9341_t *dev, uint8_t number, uint16_t voltage_mV, uint16_t current_mA);
void ili9341_set_sink_capability_requested(
    ili9341_t *dev, uint8_t number, uint16_t voltage_mV, uint16_t current_mA);
void ili9341_remove_all_sink_capabilities(ili9341_t *dev);
void ili9341_redraw_all_sink_capabilities(ili9341_t *dev, uint8_t cable);

#ifdef __cplusplus
}
#endif

#endif // __ILI9341_H__