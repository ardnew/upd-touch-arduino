/*******************************************************************************
 *
 *  name: neopixel.h
 *  date: Dec 13, 2019
 *  auth: andrew
 *  desc:
 *
 ******************************************************************************/

#ifndef __NEOPIXEL_H__
#define __NEOPIXEL_H__

// ----------------------------------------------------------------- includes --

#include <Adafruit_NeoPixel.h>

// ------------------------------------------------------------------ defines --

/* nothing */

// ------------------------------------------------------------------- macros --

#define RGB(r,g,b) (neopixel_color_t){ .red = (r), .green = (g), .blue = (b) }

// ----------------------------------------------------------- exported types --

typedef enum
{
  nsNONE = -1,
  nsDoNotShow, // = 0
  nsShow,      // = 1
  nsCOUNT      // = 2
}
neopixel_show_t;

typedef enum
{
  nmNONE = -1,
  nmFixed,    // = 0
  nmPulse,    // = 1
  nmFabulous, // = 2
  nmCOUNT     // = 3
}
neopixel_mode_t;

typedef struct
{ // 16-bit to support the pulse fading logic
  int16_t red;
  int16_t green;
  int16_t blue;
}
neopixel_color_t;

typedef struct
{
  Adafruit_NeoPixel *pixel;  // NeoPixel object
  uint16_t           pin;    // board pin number
  neopixel_mode_t    mode;   // the color change mode
  neopixel_show_t    show;   // whether or not neopixel is showing
  neopixel_color_t   color;  // current color of pixel
  uint8_t            bright; // total brightness (0x00 - 0xFF)
  uint32_t           wait;   // time to wait between color updates
  uint32_t           last;   // time of last color update
  neopixel_color_t   pulse;  // current pulse color
  neopixel_color_t   wheel;  // current wheel color
  uint8_t            change; // flag indicating we need to call show()
  uint8_t            chmod;  // flag indicating we need to reset pulse/wheel
}
neopixel_t;

// ------------------------------------------------------- exported variables --

/* nothing */

// ------------------------------------------------------- exported functions --

#ifdef __cplusplus
extern "C" {
#endif

neopixel_t *neopixel_new(uint16_t pin, neopixel_mode_t mode, neopixel_show_t show,
    neopixel_color_t color, uint8_t bright, uint32_t wait);

void neopixel_update(neopixel_t *np);

void neopixel_set_show(neopixel_t *np, neopixel_show_t show);
void neopixel_set_color(neopixel_t *np, neopixel_color_t color, uint8_t bright);
void neopixel_set_pulse(neopixel_t *np, neopixel_color_t color, uint8_t bright, uint32_t wait);
void neopixel_set_fabulous(neopixel_t *np, uint8_t bright, uint32_t wait);

#ifdef __cplusplus
}
#endif

#endif // __NEOPIXEL_H__