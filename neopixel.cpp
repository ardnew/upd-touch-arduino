/*******************************************************************************
 *
 *  name: neopixel.cpp
 *  date: Dec 13, 2019
 *  auth: andrew
 *  desc:
 *
 ******************************************************************************/

// ----------------------------------------------------------------- includes --

#include "neopixel.h"
#include "global.h"

// ---------------------------------------------------------- private defines --

#define __NEOPIXEL_PULSE_DELTA__     0x08
#define __NEOPIXEL_PULSE_DIMMEST__   0x20
#define __NEOPIXEL_PULSE_BRIGHTEST__ 0x20

// ----------------------------------------------------------- private macros --

/* nothing */

// ------------------------------------------------------------ private types --

/* nothing */

// ------------------------------------------------------- exported variables --

/* nothing */

// -------------------------------------------------------- private variables --

/* nothing */

// ---------------------------------------------- private function prototypes --

#ifdef __cplusplus
extern "C" {
#endif

inline uint8_t same_color(neopixel_color_t a, neopixel_color_t b);
inline uint8_t color_clip(int16_t c);
int8_t next_pulse(neopixel_color_t *color, int8_t dir);
uint8_t next_wheel(neopixel_color_t *color, uint8_t pos);

#ifdef __cplusplus
}
#endif

// ------------------------------------------------------- exported functions --

neopixel_t *neopixel_new(uint16_t pin, neopixel_mode_t mode, neopixel_show_t show,
    neopixel_color_t color, uint8_t bright, uint32_t delay)
{
  info(ilInfo, "creating neopixel ...");

  neopixel_t *np = (neopixel_t *)malloc(sizeof(neopixel_t));
  if (NULL != np) {

    np->pixel  = new Adafruit_NeoPixel(1, pin, NEO_GRB + NEO_KHZ800);
    np->pin    = pin;
    np->mode   = mode;
    np->show   = show;
    np->color  = color;
    np->bright = bright;
    np->delay  = delay;
    np->last   = 0U;
    np->pulse  = color;
    np->wheel  = color;
    np->change = 1U;
    np->chmod  = 1U;

    np->pixel->begin();
    np->pixel->clear();
    np->pixel->setBrightness(bright);

    info(ilInfo, "ok");
  }
  else {
    info(ilError, "failed: malloc()");
  }
  return np;
}

void neopixel_update(neopixel_t *np)
{
  static int8_t  dir = -1;
  static uint8_t pos =  0;

  if (NULL == np)
    { return; }

  if (nsDoNotShow == np->show)
    { return; }

  uint32_t curr = millis();
  if (curr - np->last < np->delay)
    { return; }

  np->last = curr;

  neopixel_color_t color;
  switch (np->mode) {
    default:
    case nmFixed:
      color = np->color;
      break;

    case nmPulse:
      if (0U != np->chmod)
        { np->pulse = np->color; }
      dir = next_pulse(&(np->pulse), dir);
      color = np->pulse;
      np->change = 1U;
      break;

    case nmFabulous:
      if (0U != np->chmod)
        { np->wheel = np->color; }
      pos = next_wheel(&(np->wheel), pos);
      color = np->wheel;
      np->change = 1U;
      break;
  }

  if (0U == np->change)
    { return; }

  //info(ilInfo, "updating pixel %i", 0);
  np->pixel->setPixelColor(0,
      np->pixel->Color(
          color_clip(color.red),
          color_clip(color.green),
          color_clip(color.blue)));
  np->pixel->show();
  np->change = 0U;
  np->chmod  = 0U;
}

void neopixel_set_show(neopixel_t *np, neopixel_show_t show)
{
  if (NULL == np)
    { return; }

  if (show == np->show)
    { return; }

  switch (show) {
    case nsDoNotShow:
      np->pixel->clear();
      np->pixel->show();
      break;

    default:
    case nsShow:
      break;
  }

  np->show   = show;
  np->change = 1U;
  np->chmod  = 1U;
  neopixel_update(np);
}

void neopixel_set_color(neopixel_t *np, neopixel_color_t color, uint8_t bright)
{
  if (NULL == np)
    { return; }

  if ((nmFixed != np->mode) || !same_color(color, np->color)) {
    np->mode   = nmFixed;
    np->color  = color;
    np->change = 1U;
    np->chmod  = 1U;
  }
  np->show = nsShow;

  if (bright != np->bright) {
    np->bright = bright;
    np->pixel->setBrightness(bright);
  }
  neopixel_update(np);
}

void neopixel_set_pulse(neopixel_t *np, neopixel_color_t color, uint8_t bright, uint32_t delay)
{
  if (NULL == np)
    { return; }

  if ((nmPulse != np->mode) || !same_color(color, np->color)) {
    np->mode   = nmPulse;
    np->color  = color;
    np->change = 1U;
    np->chmod  = 1U;
  }
  np->show = nsShow;
  np->delay = delay;

  if (bright != np->bright) {
    np->bright = bright;
    np->pixel->setBrightness(bright);
  }
  neopixel_update(np);
}

void neopixel_set_fabulous(neopixel_t *np, uint8_t bright, uint32_t delay)
{
  if (NULL == np)
    { return; }

  if (nmFabulous != np->mode) {
    np->mode   = nmFabulous;
    np->change = 1U;
    np->chmod  = 1U;
  }
  np->show = nsShow;
  np->delay = delay;

  if (bright != np->bright) {
    np->bright = bright;
    np->pixel->setBrightness(bright);
  }
  neopixel_update(np);
}

// -------------------------------------------------------- private functions --

inline uint8_t same_color(neopixel_color_t a, neopixel_color_t b)
{
  return
    (a.red   == b.red)   &&
    (a.green == b.green) &&
    (a.blue  == b.blue)  ;
}

inline uint8_t color_clip(int16_t c)
{
  if (c < 0)
    { return 0x00; }
  else if (c > 255)
    { return 0xFF; }
  else
    { return (uint8_t)c; }
}

int8_t next_pulse(neopixel_color_t *color, int8_t dir)
{

#define DIMMEST(c) \
  ((__NEOPIXEL_PULSE_DELTA__ >= (c)) || ((c) <= __NEOPIXEL_PULSE_DIMMEST__))
#define DIMMEST_COLOR(c) \
    (DIMMEST((c)->red) && DIMMEST((c)->green) && DIMMEST((c)->blue))

#define BRIGHTEST(c) \
    ((__NEOPIXEL_PULSE_DELTA__ <= (c)) || ((c) >= __NEOPIXEL_PULSE_BRIGHTEST__))
#define BRIGHTEST_COLOR(c) \
    (BRIGHTEST((c)->red) && BRIGHTEST((c)->green) && BRIGHTEST((c)->blue))

  if (dir < 0) { // fade
    color->red   -= __NEOPIXEL_PULSE_DELTA__;
    color->green -= __NEOPIXEL_PULSE_DELTA__;
    color->blue  -= __NEOPIXEL_PULSE_DELTA__;
    if (DIMMEST_COLOR(color))
      { return 1; }
  }
  else { // brighten
    color->red   += __NEOPIXEL_PULSE_DELTA__;
    color->green += __NEOPIXEL_PULSE_DELTA__;
    color->blue  += __NEOPIXEL_PULSE_DELTA__;
    if (BRIGHTEST_COLOR(color))
      { return -1; }
  }
  return dir;

#undef DIMMEST
#undef BRIGHTEST
}

uint8_t next_wheel(neopixel_color_t *color, uint8_t pos)
{
  uint8_t next = pos + 1;

  pos = 0xFF - pos;
  if (pos < 0x55) {
    color->red   = 0x03 * pos;
    color->green = 0xFF - pos * 0x03;
    color->blue  = 0x00;
  }
  else if (pos < 0xAA) {
    pos -= 0x55;
    color->red   = 0xFF - pos * 0x03;
    color->green = 0x00;
    color->blue  = 0x03 * pos;
  }
  else {
    pos -= 0xAA;
    color->red   = 0x00;
    color->green = 0x03 * pos;
    color->blue  = 0xFF - pos * 0x03;
  }
  return next;
}