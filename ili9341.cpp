/*******************************************************************************
 *
 *  name: ili9341.cpp
 *  date: Dec 15, 2019
 *  auth: andrew
 *  desc:
 *
 ******************************************************************************/

// ----------------------------------------------------------------- includes --

#include <libstusb4500.h>

#include "font/font_DroidSansMono.h"
#include "font/font_DroidSans.h"
#include "font/font_AwesomeF000.h"
#include "font/font_AwesomeF080.h"
#include "font/font_AwesomeF200.h"
#include "font/font_Nunito.h"

#include "ili9341.h"
#include "global.h"

// ---------------------------------------------------------- private defines --

#define __ILI9341_COLOR_BACKGROUND__         ILI9341_BLACK
#define __ILI9341_COLOR_DOCK_BORDER__        ILI9341_WHITE
#define __ILI9341_COLOR_CMD_BORDER__         ILI9341_WHITE
#define __ILI9341_COLOR_CMD_NORMAL__         ILI9341_WHITE
#define __ILI9341_COLOR_CMD_ACTIVE__         ILI9341_GREEN
#define __ILI9341_COLOR_VBUS_PROMPT__        ILI9341_ORANGE
#define __ILI9341_COLOR_GROUP_CAPTION__      ILI9341_GREEN

#define __ILI9341_COLOR_SNK_LABEL__          ILI9341_WHITE
#define __ILI9341_COLOR_SNK_VALUE__          ILI9341_WHITE

#define __ILI9341_COLOR_SRC_CAP_REQ__        ILI9341_YELLOW
#define __ILI9341_COLOR_SRC_CAP_REQ_SYM__    ILI9341_YELLOW
#define __ILI9341_COLOR_SRC_CAP_NONE__       ILI9341_RED

#define __ILI9341_COLOR_SRC_CAP_ICON__       ILI9341_BLACK
#define __ILI9341_COLOR_SRC_SEL_ICON__       ILI9341_DARKGREY
#define __ILI9341_COLOR_SRC_ACT_ICON__       ILI9341_WHITE // not used (dynamic)

#define __ILI9341_COLOR_SRC_CAP_TEXT__       ILI9341_WHITE
#define __ILI9341_COLOR_SRC_SEL_TEXT__       ILI9341_BLACK
#define __ILI9341_COLOR_SRC_ACT_TEXT__       ILI9341_BLACK // not used (dynamic)

#define __ILI9341_COLOR_VBUS_OTHER__         ILI9341_WHITE
#define __ILI9341_COLOR_VBUS_ZERO__          ILI9341_RED
#define __ILI9341_COLOR_VBUS_LOW__           ILI9341_PURPLE
#define __ILI9341_COLOR_VBUS_USB__           ILI9341_ORANGE
#define __ILI9341_COLOR_VBUS_LOW_MED__       ILI9341_CYAN
#define __ILI9341_COLOR_VBUS_MED__           ILI9341_BLUE
#define __ILI9341_COLOR_VBUS_MED_HIGH__      ILI9341_MAGENTA
#define __ILI9341_COLOR_VBUS_HIGH__          ILI9341_GREEN

#define __ILI9341_COLOR_VBUS_0V__            __ILI9341_COLOR_VBUS_ZERO__
#define __ILI9341_COLOR_VBUS_5V__            __ILI9341_COLOR_VBUS_USB__
#define __ILI9341_COLOR_VBUS_9V__            __ILI9341_COLOR_VBUS_LOW_MED__
#define __ILI9341_COLOR_VBUS_12V__           __ILI9341_COLOR_VBUS_MED__
#define __ILI9341_COLOR_VBUS_15V__           __ILI9341_COLOR_VBUS_MED_HIGH__
#define __ILI9341_COLOR_VBUS_20V__           __ILI9341_COLOR_VBUS_HIGH__

#define __ILI9341_MARGIN_PX__                    4U

#define __ILI9341_DOCK_DEPTH_PX__               60U
#define __ILI9341_DOCK_RADIUS__                  6U

#define __ILI9341_SNK_PDO_DEPTH_PX__           140U
#define __ILI9341_SNK_PDO_RADIUS__               6U

#define __ILI9341_CMD_INSET_PX__                 6U

#define __ILI9341_VBUS_INSET_PX__               16U
#define __ILI9341_VBUS_MONITOR_OFFSET_PX__      34U

#define __ILI9341_COMMAND_INDEX_INVALID__      0xFF
#define __ILI9341_PDO_NUMBER_INVALID__         0xFF

#define __ILI9341_SRC_CAP_REQUEST_TIMEOUT_MS__ 3000

// ----------------------------------------------------------- private macros --

/* nothing */

// ------------------------------------------------------------ private types --

typedef struct
{
  uint8_t  number;
  uint16_t voltage_mV;
  uint16_t current_mA;

  ili9341_frame_t frame;
  uint8_t selected;
  uint8_t active;
}
ili9341_source_capability_t;

typedef struct
{
  uint8_t  number;
  uint16_t voltage_mV;
  uint16_t current_mA;

  ili9341_frame_t frame;
}
ili9341_sink_capability_t;

struct ili9341
{
  ILI9341_t3            *tft;
  XPT2046_Calibrated    *touch;
  ina260_t              *vmon;
  ili9341_orientation_t  orientation;
  ili9341_size_t         screen_size;

  ina260_sample_t current_vbus;

  uint16_t bg_color;

  uint16_t touch_interrupt_pin;

  uint8_t src_cap_count = 0U;
  ili9341_source_capability_t src_cap[__STUSB4500_NVM_SOURCE_PDO_MAX__];
  ili9341_frame_t src_cap_frame;
  uint8_t sel_src_cap_pdo;
  uint8_t act_src_cap_pdo;

  uint8_t  src_cap_determined;
  uint8_t  src_cap_requesting;
  uint32_t src_cap_request_time;
  uint8_t  src_cap_request_sym;

  ili9341_frame_t cmd_frame[icCOUNT];

  ili9341_point_t cmd_bar_pos;
  ili9341_size_t  cmd_frame_size;
  ili9341_size_t  cmd_icon_size;

  ili9341_command_mask_t cmd_sel;

  ili9341_frame_t snk_pdo_frame;
  ili9341_sink_capability_t snk_pdo[__STUSB4500_NVM_SINK_PDO_COUNT__];
  ili9341_sink_capability_t snk_rdo;

  ili9341_point_t vbus_mon_pos;

  ili9341_command_callback_t toggle_power_pressed;
  ili9341_command_callback_t cycle_power_pressed;
  ili9341_command_callback_t set_power_pressed;
  ili9341_command_callback_t get_source_cap_pressed;
};

// ------------------------------------------------------- exported variables --

ili9341_point_t const POINT_INVALID = (ili9341_point_t){ {0xFFFF}, {0xFFFF} };
ili9341_point_t const SIZE_INVALID  =  (ili9341_size_t){ {0xFFFF}, {0xFFFF} };
ili9341_frame_t const FRAME_INVALID = (ili9341_frame_t){
    POINT_INVALID, SIZE_INVALID };

ili9341_source_capability_t const SRC_CAP_INVALID =
    (ili9341_source_capability_t) {
      __ILI9341_PDO_NUMBER_INVALID__, 0U, 0U, FRAME_INVALID, 0U, 0U
    };

ili9341_sink_capability_t const SNK_PDO_INVALID =
    (ili9341_sink_capability_t) {
      __ILI9341_PDO_NUMBER_INVALID__, 0U, 0U, FRAME_INVALID
    };

// -------------------------------------------------------- private variables --

static char const COMMAND_CHAR[icCOUNT] = { // from font_AwesomeF000.h
    0x11, // icTogglePower
    0x21, // icCyclePower
    0x19, // icGetSourceCap
    0x46  // icSetPower
};

// ---------------------------------------------- private function prototypes --

static ili9341_size_t ili9341_screen_size(ili9341_orientation_t orientation);
static ili9341_point_contained_t ili9341_frame_contains(
    ili9341_t *dev, ili9341_frame_t *frame, ili9341_point_t point);
static ili9341_point_t ili9341_cursor_position(ili9341_t *dev);

static void ili9341_clear_source_capabilities_frame(ili9341_t *dev);
static ili9341_frame_t ili9341_draw_source_capabilities(ili9341_t *dev,
    ili9341_frame_t dock_frm);
static uint8_t ili9341_containing_source_capability(ili9341_t *dev,
    ili9341_point_t pos);

static ili9341_point_t ili9341_draw_commands(ili9341_t *dev,
    ili9341_point_t cmd_bar_pos, ili9341_command_mask_t selected);
static ili9341_command_mask_t ili9341_containing_command(ili9341_t *dev,
    ili9341_point_t pos);

static void ili9341_clear_sink_capabilities_frame(ili9341_t *dev);
static void ili9341_draw_sink_capabilities(ili9341_t *dev,
    ili9341_frame_t snk_frm, uint8_t cable);

//static ili9341_point_t ili9341_draw_vbus_prompt(ili9341_t *dev,
//    ili9341_point_t cmd_bar_pos);
static ili9341_point_t ili9341_draw_vbus(ili9341_t *dev,
    ili9341_point_t vbus_pos);

static uint16_t ili9341_voltage_color(int32_t voltage);

// ------------------------------------------------------- exported functions --

ili9341_t *ili9341_new(
    ILI9341_t3            *tft,
    XPT2046_Calibrated    *touch,
    ina260_t              *vmon,
    ili9341_orientation_t  orientation,
    uint16_t touch_interrupt_pin)
{
  ili9341_t *dev = NULL;

  if ((orientation > isoNONE) && (orientation < isoCOUNT)) {

    if (NULL != (dev = (ili9341_t *)malloc(sizeof(ili9341_t)))) {

      dev->tft   = tft;
      dev->touch = touch;
      dev->vmon  = vmon;

      dev->orientation = orientation;
      dev->screen_size = ili9341_screen_size(orientation);

      dev->current_vbus = INA260_SAMPLE_INVALID;

      dev->bg_color = __ILI9341_COLOR_BACKGROUND__;

      dev->touch_interrupt_pin = touch_interrupt_pin;

      dev->src_cap_frame = FRAME_INVALID;
      dev->sel_src_cap_pdo = __ILI9341_PDO_NUMBER_INVALID__;
      dev->act_src_cap_pdo = __ILI9341_PDO_NUMBER_INVALID__;

      dev->src_cap_determined   = 0U;
      dev->src_cap_requesting   = 0U;
      dev->src_cap_request_time = 0U;
      dev->src_cap_request_sym  = 0U;

      for (uint8_t i = 0; i < icCOUNT; ++i)
        { dev->cmd_frame[i] = FRAME_INVALID; }

      dev->cmd_bar_pos    = POINT_INVALID;
      dev->cmd_frame_size = SIZE_INVALID;
      dev->cmd_icon_size  = SIZE_INVALID;

      dev->cmd_sel = 0U;

      dev->snk_pdo_frame  = FRAME_INVALID;
      for (uint8_t i = 0; i < __STUSB4500_NVM_SINK_PDO_COUNT__; ++i)
        { dev->snk_pdo[i] = SNK_PDO_INVALID; }

      dev->vbus_mon_pos   = POINT_INVALID;

      dev->toggle_power_pressed   = NULL;
      dev->cycle_power_pressed    = NULL;
      dev->set_power_pressed      = NULL;
      dev->get_source_cap_pressed = NULL;

      // -----------------------------------------------------------------------

      dev->tft->begin();
      dev->tft->setRotation(orientation);
      dev->touch->begin();
      dev->touch->setRotation(orientation);

      // the XPT2046_Touchscreen library (Paul Stoffregen) has been modified to
      // support coordinate system calibration using 3-point calibration as
      // defined in the paper from Fang & Chang (TI):
      //   http://www.ti.com/lit/an/slyt277/slyt277.pdf
      //
      // the modifications are based on the following post:
      //   http://www.ars-informatica.ca/eclectic/xpt2046-touch-screen-three-point-calibration/
      //
      // the calibration parameters were calculated by manual observation at
      // the following three points:
      //   ( 23,  47) -> (3442, 3141)
      //   (125,  88) -> (2315, 2507)
      //   (231, 162) -> (1118, 1329)
      dev->touch->calibrate(
          -0.093109, 0.004628, 328, -0.002430, -0.060349, 244,
          dev->screen_size.width, dev->screen_size.height);
    }
  }

  return dev;
}

void crossHair(ili9341_t *dev, uint16_t x, uint16_t y) {
  if (NULL == dev)
    { return; }
  dev->tft->drawCircle(       x,     y, 6, ILI9341_WHITE);
  dev->tft->drawFastHLine(x - 4,     y, 9, ILI9341_WHITE);
  dev->tft->drawFastVLine(    x, y - 4, 9, ILI9341_WHITE);
}

void ili9341_draw_template(ili9341_t *dev)
{
  if (NULL == dev)
    { return; }

  dev->tft->setTextWrap(0U); // manually handle wrapping if needed
  dev->tft->fillScreen(dev->bg_color);

  dev->src_cap_frame = (ili9341_frame_t){
      (ili9341_point_t){ __ILI9341_MARGIN_PX__, 2U * __ILI9341_MARGIN_PX__ },
      (ili9341_size_t){
        (uint16_t)(dev->screen_size.width - 2U * __ILI9341_MARGIN_PX__),
        __ILI9341_DOCK_DEPTH_PX__
      }
  };

  // dock containing the source capability icons
  dev->tft->drawRoundRect(
      dev->src_cap_frame.origin.x,
      dev->src_cap_frame.origin.y,
      dev->src_cap_frame.size.width,
      dev->src_cap_frame.size.height,
      __ILI9341_DOCK_RADIUS__,
      __ILI9341_COLOR_DOCK_BORDER__
  );

  // the source cap group caption
  static char const src_text[] = "SOURCE CAPABILITIES";

  dev->tft->setFont(Nunito_8);
  dev->tft->setTextSize(1);

  uint16_t src_text_w = dev->tft->measureTextWidth(src_text, strlen(src_text));
  uint16_t src_text_h = dev->tft->measureTextHeight(src_text, strlen(src_text));
  uint16_t src_text_x = 2U * __ILI9341_DOCK_RADIUS__;
  uint16_t src_text_y = __ILI9341_MARGIN_PX__;

  dev->tft->fillRect(src_text_x, src_text_y, src_text_w, src_text_h,
      __ILI9341_COLOR_BACKGROUND__);

  dev->tft->setCursor(src_text_x, src_text_y);
  dev->tft->setTextColor(__ILI9341_COLOR_GROUP_CAPTION__);
  dev->tft->print(src_text);

  // the command icons
  dev->cmd_bar_pos = ili9341_draw_commands(dev, dev->cmd_bar_pos, 0U);

  dev->snk_pdo_frame = (ili9341_frame_t){
      (ili9341_point_t){
        (uint16_t)(dev->screen_size.width -
          __ILI9341_SNK_PDO_DEPTH_PX__ - __ILI9341_MARGIN_PX__),
        (uint16_t)(dev->src_cap_frame.origin.y +
          dev->src_cap_frame.size.height + 2U * __ILI9341_MARGIN_PX__),
      },
      (ili9341_size_t){
        __ILI9341_SNK_PDO_DEPTH_PX__,
        (uint16_t)(dev->cmd_bar_pos.y -
          (dev->src_cap_frame.origin.y + dev->src_cap_frame.size.height +
          __ILI9341_MARGIN_PX__) - 3U * __ILI9341_MARGIN_PX__),
      }
  };

  // dock containing the SNK PDOs
  dev->tft->drawRoundRect(
      dev->snk_pdo_frame.origin.x,
      dev->snk_pdo_frame.origin.y,
      dev->snk_pdo_frame.size.width,
      dev->snk_pdo_frame.size.height,
      __ILI9341_SNK_PDO_RADIUS__,
      __ILI9341_COLOR_DOCK_BORDER__
  );

  // the SNK PDO group caption
  static char const snk_text[] = "STUSB4500 SNK PDO";

  dev->tft->setFont(Nunito_8);
  dev->tft->setTextSize(1);

  uint16_t snk_text_w = dev->tft->measureTextWidth(snk_text, strlen(snk_text));
  uint16_t snk_text_h = dev->tft->measureTextHeight(snk_text, strlen(snk_text));
  uint16_t snk_text_x = dev->snk_pdo_frame.origin.x + __ILI9341_DOCK_RADIUS__ + __ILI9341_MARGIN_PX__;
  uint16_t snk_text_y = dev->snk_pdo_frame.origin.y - __ILI9341_MARGIN_PX__;

  dev->tft->fillRect(snk_text_x, snk_text_y, snk_text_w, snk_text_h,
      __ILI9341_COLOR_BACKGROUND__);

  dev->tft->setCursor(snk_text_x, snk_text_y);
  dev->tft->setTextColor(__ILI9341_COLOR_GROUP_CAPTION__);
  dev->tft->print(snk_text);

  //ili9341_draw_sink_capabilities(dev, dev->snk_pdo_frame);
}

void ili9341_draw(ili9341_t *dev)
{
  if (NULL == dev)
    { return; }

  ili9341_point_t position;
  uint8_t pressure;

  // read the touch screen only one time per draw cycle. re-use the results as
  // arguments to all subroutines.
  ili9341_touch_pressed_t pressed =
    ili9341_touch_pressed(dev, &position, &pressure);

  if (itpNotPressed == pressed) {
    if (__ILI9341_PDO_NUMBER_INVALID__ != dev->sel_src_cap_pdo) {
      if (dev->act_src_cap_pdo == dev->sel_src_cap_pdo)
        { dev->act_src_cap_pdo = __ILI9341_PDO_NUMBER_INVALID__; }
      else
        { dev->act_src_cap_pdo = dev->sel_src_cap_pdo; }
    }
    if (0U != dev->cmd_sel) {
      switch (dev->cmd_sel) {
        case icTogglePower:
          if (NULL != dev->toggle_power_pressed) {
            dev->toggle_power_pressed(dev, NULL);
          }
          break;
        case icCyclePower:
          info(ilInfo, "cycle?");
          if (NULL != dev->cycle_power_pressed) {
            info(ilInfo, "cycle!");
            dev->cycle_power_pressed(dev, NULL);
          }
          break;
        case icSetPower:
          if (NULL != dev->set_power_pressed) {
            if (__ILI9341_PDO_NUMBER_INVALID__ != dev->act_src_cap_pdo) {
              dev->set_power_pressed(dev, &(dev->act_src_cap_pdo));
              dev->sel_src_cap_pdo = __ILI9341_PDO_NUMBER_INVALID__;
              dev->act_src_cap_pdo = __ILI9341_PDO_NUMBER_INVALID__;
            }
          }
          break;
        case icGetSourceCap:
          if (NULL != dev->get_source_cap_pressed) {
            dev->get_source_cap_pressed(dev, NULL);
          }
          break;
      }
      info(ilInfo, "send command: 0x%X", dev->cmd_sel);
    }
  }

  // the VBUS monitor
  if (ina260_sample(dev->vmon, &(dev->current_vbus)))
    { dev->vbus_mon_pos = ili9341_draw_vbus(dev, dev->vbus_mon_pos); }

  dev->sel_src_cap_pdo = ili9341_containing_source_capability(dev, position);
  dev->src_cap_frame = ili9341_draw_source_capabilities(
        dev, dev->src_cap_frame);

  dev->cmd_sel = ili9341_containing_command(dev, position);
  dev->cmd_bar_pos = ili9341_draw_commands(dev, dev->cmd_bar_pos, dev->cmd_sel);
}

ili9341_touch_pressed_t ili9341_touch_pressed(
    ili9341_t *dev, ili9341_point_t *position, uint8_t *pressure)
{
  if (dev->touch->tirqTouched()) {
    if (dev->touch->touched()) {
      dev->touch->readData(&(position->x), &(position->y), pressure);
      return itpPressed;
    }
  }
  return itpNotPressed;
//  static const uint16_t required = 5U;
//  uint16_t samples = 0U;
//
//  float avg_x = 0.0F;
//  float avg_y = 0.0F;
//  float avg_z = 0.0F;
//
//  uint16_t x, y;
//  uint8_t z;
//
//  for (uint16_t i = 0; i < required; ++i) {
//    if (dev->touch->tirqTouched()) {
//      if (dev->touch->touched()) {
//        dev->touch->readData(&x, &y, &z);
//        avg_x += (float)x;
//        avg_y += (float)y;
//        avg_z += (float)z;
//        ++samples;
//      }
//    }
//  }
//
//  if (samples == required) {
//    // poor man's rounding
//    position->x = (uint16_t)(avg_x / (float)samples + 0.5F);
//    position->y = (uint16_t)(avg_y / (float)samples + 0.5F);
//    *pressure   = ( uint8_t)(avg_z / (float)samples + 0.5F);
//    return itpPressed;
//  }
//  return itpNotPressed;
}

void ili9341_set_command_pressed(ili9341_t *dev,
    ili9341_command_mask_t command, ili9341_command_callback_t callback)
{
  if ((NULL == dev) || (NULL == callback))
    { return; }

  if ((command & icTogglePower) > 0) {
    dev->toggle_power_pressed = callback;
  }
  if ((command & icCyclePower) > 0) {
    dev->cycle_power_pressed = callback;
  }
  if ((command & icSetPower) > 0) {
    dev->set_power_pressed = callback;
  }
  if ((command & icGetSourceCap) > 0) {
    dev->get_source_cap_pressed = callback;
  }
}

void ili9341_add_source_capability(
    ili9341_t *dev, uint8_t number, uint16_t voltage_mV, uint16_t current_mA)
{
  if (NULL == dev)
    { return; }

  if (number < __STUSB4500_NVM_SOURCE_PDO_MAX__) {
    dev->src_cap[number].number     = number;
    dev->src_cap[number].voltage_mV = voltage_mV;
    dev->src_cap[number].current_mA = current_mA;
    dev->src_cap[number].frame      = FRAME_INVALID; // updated on draw
    dev->src_cap[number].selected   = 0U;
    dev->src_cap[number].active     = 0U;
    ++(dev->src_cap_count);
  }
}

void ili9341_init_request_source_capabilities(ili9341_t *dev)
{
  if (NULL == dev)
    { return; }

  dev->src_cap_requesting   = 1U;
  dev->src_cap_request_time = millis();
  dev->src_cap_request_sym  = 0U;

  ili9341_remove_all_source_capabilities(dev);

  static char const text[] = "analyzing source ...";

  dev->tft->setFont(DroidSans_12);
  dev->tft->setTextSize(1);

  static uint16_t text_w = 0U, text_h = 0U, text_x = 0U, text_y = 0U;
  if ((text_w == 0U) || (text_h == 0U)) {
    // only do these measurements one time, as the text does not change
    text_w = dev->tft->measureTextWidth(text, strlen(text) - 4U);
    text_h = dev->tft->measureTextHeight(text, strlen(text) - 4U);
    text_x = (uint16_t)(dev->src_cap_frame.origin.x +
      ((float)dev->src_cap_frame.size.width - text_w) / 2.0F + 0.5F);
    text_y = (uint16_t)(dev->src_cap_frame.origin.y +
      ((float)dev->src_cap_frame.size.height - text_h) / 2.0F + 0.5F);
  }

  dev->tft->setCursor(text_x, text_y);
  dev->tft->setTextColor(__ILI9341_COLOR_SRC_CAP_REQ__);
  dev->tft->print(text);
}

void ili9341_remove_all_source_capabilities(ili9341_t *dev)
{
  if (NULL == dev)
    { return; }

  for (uint8_t i = 0; i < __STUSB4500_NVM_SOURCE_PDO_MAX__; ++i)
    { dev->src_cap[i] = SRC_CAP_INVALID; }
  dev->sel_src_cap_pdo = __ILI9341_PDO_NUMBER_INVALID__;
  dev->act_src_cap_pdo = __ILI9341_PDO_NUMBER_INVALID__;
  dev->src_cap_count   = 0U;

  dev->src_cap_determined = 0U;

  ili9341_clear_source_capabilities_frame(dev);
}

void ili9341_set_sink_capability(
    ili9341_t *dev, uint8_t number, uint16_t voltage_mV, uint16_t current_mA)
{
  if (NULL == dev)
    { return; }

  if (number <= __STUSB4500_NVM_SINK_PDO_COUNT__) {
    dev->snk_pdo[number].number     = number;
    dev->snk_pdo[number].voltage_mV = voltage_mV;
    dev->snk_pdo[number].current_mA = current_mA;
  }
}

void ili9341_set_sink_capability_requested(
    ili9341_t *dev, uint8_t number, uint16_t voltage_mV, uint16_t current_mA)
{
  if (NULL == dev)
    { return; }

  if (number <= __STUSB4500_NVM_SINK_PDO_COUNT__) {
    dev->snk_rdo.number     = number;
    dev->snk_rdo.voltage_mV = voltage_mV;
    dev->snk_rdo.current_mA = current_mA;
  }
}

void ili9341_remove_all_sink_capabilities(ili9341_t *dev)
{
  if (NULL == dev)
    { return; }

  for (uint8_t i = 0U; i < __STUSB4500_NVM_SINK_PDO_COUNT__; ++i)
    { dev->snk_pdo[i].number = __ILI9341_PDO_NUMBER_INVALID__; }

  dev->snk_rdo.number = __ILI9341_PDO_NUMBER_INVALID__;

  ili9341_clear_sink_capabilities_frame(dev);
}

void ili9341_redraw_all_sink_capabilities(ili9341_t *dev, uint8_t cable)
{
  if (NULL == dev)
    { return; }

  if (ili9341_frame_valid(&(dev->snk_pdo_frame)))
    { ili9341_draw_sink_capabilities(dev, dev->snk_pdo_frame, cable); }
}

// -------------------------------------------------------- private functions --

static ili9341_size_t ili9341_screen_size(ili9341_orientation_t orientation)
{
  switch (orientation) {
    default:
    case isoBottom:
      return (ili9341_size_t){ { .width = 240U }, { .height = 320U } };
    case isoRight:
      return (ili9341_size_t){ { .width = 320U }, { .height = 240U } };
    case isoTop:
      return (ili9341_size_t){ { .width = 240U }, { .height = 320U } };
    case isoLeft:
      return (ili9341_size_t){ { .width = 320U }, { .height = 240U } };
  }
}

static ili9341_point_contained_t ili9341_frame_contains(
    ili9341_t *dev, ili9341_frame_t *frame, ili9341_point_t point)
{
  if ((NULL == dev) || (NULL == frame))
    { return ipcNONE; }

  uint16_t x = point.x;
  uint16_t y = point.y;

  uint8_t contained =
    ((x >= frame->origin.x) && (x <= (frame->origin.x + frame->size.width)))
     &&
    ((y >= frame->origin.y) && (y <= (frame->origin.y + frame->size.height)));

  if (0U != contained)
    { return ipcContained; }
  else
    { return ipcNotContained; }
}

static ili9341_point_t ili9341_cursor_position(ili9341_t *dev)
{
  if (NULL == dev)
    { return POINT_INVALID; }

  int16_t x, y;

  dev->tft->getCursor(&x, &y);

  if (x < 0) { x = 0; }
  if (y < 0) { y = 0; }

  return (ili9341_point_t){ {(uint16_t)x}, {(uint16_t)y} };
}

static void ili9341_clear_source_capabilities_frame(ili9341_t *dev)
{
  if (NULL == dev)
    { return; }

  dev->tft->fillRect(
      dev->src_cap_frame.origin.x + __ILI9341_MARGIN_PX__,
      dev->src_cap_frame.origin.y + __ILI9341_MARGIN_PX__,
      (uint16_t)(dev->src_cap_frame.size.width - 2U * __ILI9341_MARGIN_PX__),
      (uint16_t)(dev->src_cap_frame.size.height - 2U * __ILI9341_MARGIN_PX__),
      //dev->screen_size.width - 4 * __ILI9341_MARGIN_PX__,
      //__ILI9341_DOCK_DEPTH_PX__ - 2 * __ILI9341_MARGIN_PX__,
      __ILI9341_COLOR_BACKGROUND__
    );
}

static ili9341_frame_t ili9341_draw_source_capabilities(ili9341_t *dev,
    ili9341_frame_t dock_frm)
{
  if (NULL == dev)
    { return FRAME_INVALID; }

  uint32_t curr_time = millis();

  if (0U == dev->src_cap_determined) {

    if (0U != dev->src_cap_requesting) {

      if (curr_time - dev->src_cap_request_time >
            __ILI9341_SRC_CAP_REQUEST_TIMEOUT_MS__) {
        dev->src_cap_requesting = 0U; // timed out
        return dock_frm;
      }
      else if (dev->src_cap_count > 0U)
        { dev->src_cap_determined = 1U; }
      else {

        dev->tft->setFont(AwesomeF200_16);
        dev->tft->setTextColor(__ILI9341_COLOR_SRC_CAP_REQ_SYM__);

        static char const sym_wait[] = { 0x50, 0x51, 0x52, 0x53, 0x54 };
        static uint8_t const sym_count = 5U;
        static uint32_t sym_inc = 0;
        static uint16_t sym_w = 0U, sym_h = 0U, sym_x = 0U, sym_y = 0U;
        if ((sym_w == 0U) || (sym_h == 0U)) {
          // only do these measurements one time, as the text does not change
          sym_w = dev->tft->measureTextWidth(sym_wait, 1);
          sym_h = dev->tft->measureTextHeight(sym_wait, 1);
          sym_x = (uint16_t)(dev->src_cap_frame.origin.x +
            dev->src_cap_frame.size.width - 8 * __ILI9341_MARGIN_PX__ - sym_w);
          sym_y = (uint16_t)(dev->src_cap_frame.origin.y +
            ((float)dev->src_cap_frame.size.height - sym_h) / 2.0F + 0.5F);
        }

        if (0U == sym_inc) {
          dev->tft->fillRect(
              sym_x, sym_y, sym_w, sym_h, __ILI9341_COLOR_BACKGROUND__);
          dev->tft->setCursor(sym_x, sym_y);
          dev->tft->drawFontChar(sym_wait[dev->src_cap_request_sym]);
        }

        ++sym_inc;
        if (sym_inc > 250U) {
          sym_inc = 0U;
          dev->src_cap_request_sym =
              (dev->src_cap_request_sym + 1) % sym_count;
        }
      }
    }
    else {

      dev->src_cap_determined = 1U;
      ili9341_clear_source_capabilities_frame(dev);
      ili9341_clear_sink_capabilities_frame(dev);

      static char const nosrc[] = "(USB PD unavailable)";

      dev->tft->setFont(DroidSans_12);
      dev->tft->setTextSize(1);

      static uint16_t text_w = 0U, text_h = 0U, text_x = 0U, text_y = 0U;
      if ((text_w == 0U) || (text_h == 0U)) {
        // only do these measurements one time, as the text does not change
        text_w = dev->tft->measureTextWidth(nosrc, strlen(nosrc));
        text_h = dev->tft->measureTextHeight(nosrc, strlen(nosrc));
        text_x = (uint16_t)(dev->src_cap_frame.origin.x +
          ((float)dev->src_cap_frame.size.width - text_w) / 2.0F + 0.5F);
        text_y = (uint16_t)(dev->src_cap_frame.origin.y +
          ((float)dev->src_cap_frame.size.height - text_h) / 2.0F + 0.5F);
      }

      dev->tft->setCursor(text_x, text_y);
      dev->tft->setTextColor(__ILI9341_COLOR_SRC_CAP_NONE__);
      dev->tft->print(nosrc);
    }

  }

  if (0U == dev->src_cap_count)
    { return dock_frm; }

  uint16_t ico_w =
      (uint16_t)((float)(
        dock_frm.size.width -
        __ILI9341_MARGIN_PX__ * 4U - // outer margins
        __ILI9341_MARGIN_PX__ * (dev->src_cap_count - 1U) // separator margins
      )
      / dev->src_cap_count + 0.5);

  for (uint8_t i = 0U; i < dev->src_cap_count; ++i) {

    ili9341_source_capability_t *cap = &(dev->src_cap[i]);
    uint8_t redraw = 0U;

    if (__ILI9341_PDO_NUMBER_INVALID__ == cap->number)
      { continue; }

    if ( ((i == dev->sel_src_cap_pdo) && (0U == cap->selected))
          ||
         ((i != dev->sel_src_cap_pdo) && (1U == cap->selected)) )
      { ++redraw; }

    if ( ((i == dev->act_src_cap_pdo) && (0U == cap->active))
          ||
         ((i != dev->act_src_cap_pdo) && (1U == cap->active)) )
      { ++redraw; }

    cap->selected = !!(i == dev->sel_src_cap_pdo);
    cap->active   = !!(i == dev->act_src_cap_pdo);

    if (!ili9341_frame_valid(&(cap->frame)))
      { ++redraw; }

    if (redraw > 0U) {

      cap->frame = (ili9341_frame_t){
          (ili9341_point_t){
            (uint16_t)(dock_frm.origin.x +
                (__ILI9341_MARGIN_PX__ + ico_w) * i +
                2 * __ILI9341_MARGIN_PX__),
            (uint16_t)(dock_frm.origin.y + __ILI9341_MARGIN_PX__)
          },
          (ili9341_size_t){
            ico_w,
            (uint16_t)(dock_frm.size.height - __ILI9341_MARGIN_PX__ * 2U)
          }
      };

//     info(ilInfo, "redrawing: %u {%ux%u}@{%u,%u} sel=%u, act=%u",
//         i,
//         cap->frame.size.width,
//         cap->frame.size.height,
//         cap->frame.origin.x,
//         cap->frame.origin.y,
//         cap->selected,
//         cap->active);

      if (i < dev->src_cap_count - 1) {
        dev->tft->drawFastVLine(
            (uint16_t)(cap->frame.origin.x + cap->frame.size.width +
                (uint16_t)((float)__ILI9341_MARGIN_PX__ / 2.0 + 0.5F)),
            (uint16_t)(cap->frame.origin.y + 2 * __ILI9341_MARGIN_PX__),
            cap->frame.size.height - 4 * __ILI9341_MARGIN_PX__,
            __ILI9341_COLOR_DOCK_BORDER__
        );
      }

      uint16_t bg_color, fg_color;

      if (0U != cap->selected)
        { bg_color = __ILI9341_COLOR_SRC_SEL_ICON__;
          fg_color = __ILI9341_COLOR_SRC_SEL_TEXT__; }
//      else if (0U != cap->active)
//        { bg_color = __ILI9341_COLOR_SRC_ACT_ICON__;
//          fg_color = __ILI9341_COLOR_SRC_ACT_TEXT__; }
      else if (0U != cap->active)
        { bg_color = ili9341_voltage_color(cap->voltage_mV);
          fg_color = __ILI9341_COLOR_SRC_ACT_TEXT__; }
      else
        { bg_color = __ILI9341_COLOR_SRC_CAP_ICON__;
          fg_color = __ILI9341_COLOR_SRC_CAP_TEXT__; }

      dev->tft->fillRoundRect(
          (uint16_t)(cap->frame.origin.x + __ILI9341_MARGIN_PX__),
          (uint16_t)(cap->frame.origin.y + __ILI9341_MARGIN_PX__),
          (uint16_t)(cap->frame.size.width - 2U * __ILI9341_MARGIN_PX__),
          (uint16_t)(cap->frame.size.height - 2U * __ILI9341_MARGIN_PX__),
          __ILI9341_DOCK_RADIUS__,
          bg_color);

      dev->tft->setFont(DroidSans_12);
      dev->tft->setTextColor(fg_color);

      static char vbuf[8];
      static char cbuf[8];

      snprintf(vbuf, 8, "%.1fV", cap->voltage_mV / 1000.0);
      snprintf(cbuf, 8, "%.1fA", cap->current_mA / 1000.0);

      uint32_t vw = dev->tft->measureTextWidth(vbuf);
      uint32_t cw = dev->tft->measureTextWidth(cbuf);

      dev->tft->setCursor(
          (uint16_t)(cap->frame.origin.x + (cap->frame.size.width - vw) / 2.0F + 0.5F),
          (uint16_t)(cap->frame.origin.y + 3U * __ILI9341_MARGIN_PX__));
      dev->tft->printf(vbuf);

      dev->tft->setCursor(
          (uint16_t)(cap->frame.origin.x + (cap->frame.size.width - cw) / 2.0F + 0.5F),
          (uint16_t)(cap->frame.origin.y + cap->frame.size.height / 2.0F + __ILI9341_MARGIN_PX__));
      dev->tft->printf(cbuf);

    }
  }

  return dock_frm;
}

static uint8_t ili9341_containing_source_capability(ili9341_t *dev,
    ili9341_point_t pos)
{
  if (NULL == dev)
    { return icNONE; }

  pos.x -= 25U;
  pos.y -= 10U;

  ili9341_frame_t *frm;
  for (uint8_t i = 0; i < dev->src_cap_count; ++i) {
    frm = &(dev->src_cap[i].frame);
    if (ili9341_frame_valid(frm) && ili9341_frame_contains(dev, frm, pos)) {
      return i;
    }
  }
  return __ILI9341_PDO_NUMBER_INVALID__;
}

static ili9341_point_t ili9341_draw_commands(ili9341_t *dev,
    ili9341_point_t cmd_bar_pos, ili9341_command_mask_t selected)
{
  if (NULL == dev)
    { return POINT_INVALID; }

  dev->tft->setFont(AwesomeF000_20);
  dev->tft->setTextSize(1);

  if (!ili9341_point_valid(cmd_bar_pos)) {

    uint16_t cmd_all_h = dev->tft->measureTextHeight(COMMAND_CHAR, icCOUNT);
    uint16_t cmd_all_w = dev->tft->measureTextWidth(COMMAND_CHAR, icCOUNT);
    uint16_t cmd_ico_w = (uint16_t)((float)cmd_all_w / icCOUNT + 0.5F);

    uint16_t cmd_bar_y = dev->screen_size.height - 4U * __ILI9341_MARGIN_PX__ - cmd_all_h;
    uint16_t cmd_bar_x = __ILI9341_CMD_INSET_PX__;
    uint16_t cmd_bar_w = dev->screen_size.width - 2 *__ILI9341_CMD_INSET_PX__;

    uint16_t cmd_fra_w = (uint16_t)((float)cmd_bar_w / icCOUNT + 0.5F);

    // line separating command icons
    dev->tft->drawFastHLine(
        cmd_bar_x, cmd_bar_y, cmd_bar_w, __ILI9341_COLOR_CMD_BORDER__);

    dev->cmd_icon_size  = (ili9341_size_t){ {cmd_ico_w}, {cmd_all_h} };
    dev->cmd_frame_size = (ili9341_size_t){ {cmd_fra_w},
      { (uint16_t)(dev->screen_size.height - cmd_bar_y) }
    };

    dev->vbus_mon_pos = (ili9341_size_t){
      __ILI9341_VBUS_INSET_PX__,
      (uint16_t)(cmd_bar_y - __ILI9341_VBUS_MONITOR_OFFSET_PX__)
    };
  }

  uint16_t icon_width  = dev->cmd_icon_size.width;
  uint16_t frame_width = dev->cmd_frame_size.width;

  uint16_t cmd_sel_y = dev->screen_size.height - __ILI9341_MARGIN_PX__;
  uint16_t cmd_sel_w = frame_width - 2 * __ILI9341_MARGIN_PX__;

  uint16_t bar_start = __ILI9341_CMD_INSET_PX__;
  uint16_t bar_top   = dev->screen_size.height - dev->cmd_frame_size.height;

  uint16_t cmd_sel_x;
  uint16_t cmd_sel_b;
  uint16_t cmd_sel_c;
  uint8_t  active;
  uint8_t  index;

  for (uint8_t i = 0U; i < icCOUNT; ++i) {

    cmd_sel_x = bar_start + (i * frame_width) + __ILI9341_MARGIN_PX__;
    active    = (selected >> i) & 0x1;
    index     = i;

    if (active) {
      cmd_sel_b = __ILI9341_COLOR_CMD_ACTIVE__;
      cmd_sel_c = __ILI9341_COLOR_CMD_ACTIVE__;
    }
    else {
      cmd_sel_b = __ILI9341_COLOR_BACKGROUND__;
      cmd_sel_c = __ILI9341_COLOR_CMD_NORMAL__;
    }
    dev->tft->drawFastHLine(cmd_sel_x, cmd_sel_y, cmd_sel_w, cmd_sel_b);

    dev->cmd_frame[index] = (ili9341_frame_t){
        (ili9341_point_t){
          { (uint16_t)(bar_start +
              ((uint16_t)i * frame_width) +
              (uint16_t)((float)(frame_width - icon_width) / 2.0F)) },
          { (uint16_t)(bar_top + __ILI9341_MARGIN_PX__) }
        },
        dev->cmd_frame_size
    };

    dev->tft->setCursor(
        dev->cmd_frame[index].origin.x, dev->cmd_frame[index].origin.y);
    dev->tft->setTextColor(cmd_sel_c);
    dev->tft->printf("%c", COMMAND_CHAR[index]);
  }

  return (ili9341_point_t){ {bar_start}, {bar_top} };
}

static ili9341_command_mask_t ili9341_containing_command(ili9341_t *dev,
    ili9341_point_t pos)
{
  if (NULL == dev)
    { return icNONE; }

  pos.x += 5U;

  ili9341_command_mask_t cmd = icNONE;
  ili9341_frame_t *frm;
  for (uint8_t i = 0; i < icCOUNT; ++i) {
    frm = &(dev->cmd_frame[i]);
    if (ili9341_frame_valid(frm) && ili9341_frame_contains(dev, frm, pos)) {
      cmd |= (1 << i);
    }
  }
  return cmd;
}

static void ili9341_clear_sink_capabilities_frame(ili9341_t *dev)
{
  if (NULL == dev)
    { return; }

  dev->tft->fillRect(
      dev->snk_pdo_frame.origin.x + __ILI9341_MARGIN_PX__,
      dev->snk_pdo_frame.origin.y + __ILI9341_MARGIN_PX__,
      dev->snk_pdo_frame.size.width - 2U * __ILI9341_MARGIN_PX__,
      dev->snk_pdo_frame.size.height - 2U * __ILI9341_MARGIN_PX__,
      __ILI9341_COLOR_BACKGROUND__
  );
}

static void ili9341_draw_sink_capabilities(ili9341_t *dev,
    ili9341_frame_t snk_frm, uint8_t cable)
{
  if (NULL == dev)
    { return; }

  ili9341_clear_sink_capabilities_frame(dev);

  dev->tft->setFont(DroidSans_9);
  dev->tft->setTextSize(1);

  uint16_t top = snk_frm.origin.y + 4U * __ILI9341_MARGIN_PX__;
  uint16_t label_left = snk_frm.origin.x + 3U * __ILI9341_MARGIN_PX__;
  uint16_t value_left =
      snk_frm.origin.x + (uint16_t)(snk_frm.size.width / 2.0F + 0.5F);
      //- 2U * __ILI9341_MARGIN_PX__;
  uint16_t pdo_count = 0U;

  for (uint8_t i = 0U; i < __STUSB4500_NVM_SINK_PDO_COUNT__; ++i) {

    if (__ILI9341_PDO_NUMBER_INVALID__ != dev->snk_pdo[i].number) {

      dev->tft->setCursor(label_left, top);
      dev->tft->setTextColor(__ILI9341_COLOR_SNK_LABEL__);
      dev->tft->printf("PDO %u:", dev->snk_pdo[i].number);

      dev->tft->setCursor(value_left, top);
      dev->tft->setTextColor(__ILI9341_COLOR_SNK_VALUE__);
      dev->tft->printf("%.1fV %.1fA",
          (float)dev->snk_pdo[i].voltage_mV / 1000.0F,
          (float)dev->snk_pdo[i].current_mA / 1000.0F);

      top += dev->tft->fontCapHeight() + __ILI9341_MARGIN_PX__;

      ++pdo_count;
    }
  }

  if ((pdo_count > 0U) && (cable > 0U)) {
    top += (uint16_t)(dev->tft->fontCapHeight() / 2.0F + 0.5F) +
        __ILI9341_MARGIN_PX__;
    dev->tft->drawFastHLine(
      snk_frm.origin.x + (uint16_t)(snk_frm.size.width / 4.0F + 0.5F),
      top,
      (uint16_t)(snk_frm.size.width / 2.0F + 0.5F),
      __ILI9341_COLOR_DOCK_BORDER__
    );
    top += dev->tft->fontCapHeight() + __ILI9341_MARGIN_PX__;

    dev->tft->setCursor(label_left, top);
    dev->tft->setTextColor(__ILI9341_COLOR_SNK_LABEL__);
    dev->tft->printf("Orientation:");

    dev->tft->setCursor(value_left + 6U * __ILI9341_MARGIN_PX__, top);
    dev->tft->setTextColor(__ILI9341_COLOR_SNK_VALUE__);
    dev->tft->printf("CC%u", cable);
  }

//  if (__ILI9341_PDO_NUMBER_INVALID__ != dev->snk_rdo.number) {
//
//    dev->tft->setCursor(label_left, top);
//    dev->tft->setTextColor(__ILI9341_COLOR_SNK_LABEL__);
//    dev->tft->printf("REQ (%u):", dev->snk_rdo.number);
//
//    dev->tft->setCursor(value_left, top);
//    dev->tft->setTextColor(__ILI9341_COLOR_SNK_VALUE__);
//    dev->tft->printf("%.1fV %.1fA",
//        (float)dev->snk_rdo.voltage_mV / 1000.0F,
//        (float)dev->snk_rdo.current_mA / 1000.0F);
//  }
}

//static ili9341_point_t ili9341_draw_vbus_prompt(ili9341_t *dev,
//    ili9341_point_t cmd_bar_pos)
//{
//  if (NULL == dev)
//    { return POINT_INVALID; }
//
//  static char voltage_txt[] = "VBUS: ";
//
//  dev->tft->setFont(DroidSans_12);
//  dev->tft->setTextSize(1);
//
//  uint16_t text_h =
//      dev->tft->measureTextHeight(voltage_txt, strlen(voltage_txt));
//  uint16_t text_x = __ILI9341_VBUS_INSET_PX__;
//  uint16_t text_y = cmd_bar_pos.y - text_h - __ILI9341_MARGIN_PX__;
//
//  dev->tft->setCursor(text_x, text_y);
//  dev->tft->setTextColor(__ILI9341_COLOR_VBUS_PROMPT__);
//  dev->tft->print(voltage_txt);
//
//  return ili9341_cursor_position(dev);
//}

static ili9341_point_t ili9341_draw_vbus(ili9341_t *dev,
    ili9341_point_t vbus_pos)
{
  if (NULL == dev)
    { return POINT_INVALID; }

#define BUF_SZ 32

  static char voltage_buf[BUF_SZ] = { '\0' };
  static char current_buf[BUF_SZ] = { '\0' };

  int32_t voltage = dev->current_vbus.voltage_mV;
  int32_t current = dev->current_vbus.current_mA;

  //uint32_t voltage_abs = abs(voltage);
  //uint32_t current_abs = abs(current);

  current /= 10U;

  snprintf(voltage_buf, BUF_SZ, "%4.1fV", (float)voltage / 1000.0);
  snprintf(current_buf, BUF_SZ, "%.2gA", (float)current / 100.0);

  dev->tft->setFont(DroidSansMono_18);
  dev->tft->setTextSize(1);

  dev->tft->fillRect(vbus_pos.x, vbus_pos.y,
      (uint16_t)(dev->screen_size.width / 2.0F + 0.5F),
      dev->cmd_bar_pos.y - vbus_pos.y,
      __ILI9341_COLOR_BACKGROUND__);

  uint16_t vbus_color = ili9341_voltage_color(voltage);

  dev->tft->setCursor(vbus_pos.x, vbus_pos.y);
  dev->tft->setTextColor(vbus_color);
  dev->tft->printf("%s %s", voltage_buf, current_buf);

  return vbus_pos;

#undef BUF_SZ
}

static uint16_t ili9341_voltage_color(int32_t voltage)
{

  uint16_t color = __ILI9341_COLOR_VBUS_OTHER__;
  uint32_t voltage_abs = abs(voltage);

  if (voltage_abs < __INA260_VOLTAGE_ZERO_MAX_MV__) {
    color = __ILI9341_COLOR_VBUS_0V__;
  }
  else if (in_range_u32(voltage_abs,
      __INA260_VOLTAGE_ZERO_MAX_MV__,
      __INA260_VOLTAGE_VBUS_USB_MIN_MV__)) {
    color = __ILI9341_COLOR_VBUS_LOW__;
  }
  else if (in_range_u32(voltage_abs,
      __INA260_VOLTAGE_VBUS_USB_MIN_MV__,
      __INA260_VOLTAGE_VBUS_USB_MAX_MV__)) {
    color = __ILI9341_COLOR_VBUS_5V__;
  }
  else if (in_range_u32(voltage_abs,
      __INA260_VOLTAGE_VBUS_USB_MAX_MV__,
      __INA260_VOLTAGE_VBUS_9V_MIN_MV__)) {
    color = __ILI9341_COLOR_VBUS_LOW_MED__;
  }
  else if (in_range_u32(voltage_abs,
      __INA260_VOLTAGE_VBUS_9V_MIN_MV__,
      __INA260_VOLTAGE_VBUS_9V_MAX_MV__)) {
    color = __ILI9341_COLOR_VBUS_9V__;
  }
  else if (in_range_u32(voltage_abs,
      __INA260_VOLTAGE_VBUS_9V_MAX_MV__,
      __INA260_VOLTAGE_VBUS_12V_MIN_MV__)) {
    color = __ILI9341_COLOR_VBUS_MED__;
  }
  else if (in_range_u32(voltage_abs,
      __INA260_VOLTAGE_VBUS_12V_MIN_MV__,
      __INA260_VOLTAGE_VBUS_12V_MAX_MV__)) {
    color = __ILI9341_COLOR_VBUS_12V__;
  }
  else if (in_range_u32(voltage_abs,
      __INA260_VOLTAGE_VBUS_12V_MAX_MV__,
      __INA260_VOLTAGE_VBUS_15V_MIN_MV__)) {
    color = __ILI9341_COLOR_VBUS_MED_HIGH__;
  }
  else if (in_range_u32(voltage_abs,
      __INA260_VOLTAGE_VBUS_15V_MIN_MV__,
      __INA260_VOLTAGE_VBUS_15V_MAX_MV__)) {
    color = __ILI9341_COLOR_VBUS_15V__;
  }
  else if (in_range_u32(voltage_abs,
      __INA260_VOLTAGE_VBUS_15V_MAX_MV__,
      __INA260_VOLTAGE_VBUS_20V_MIN_MV__)) {
    color = __ILI9341_COLOR_VBUS_HIGH__;
  }
  else if (in_range_u32(voltage_abs,
      __INA260_VOLTAGE_VBUS_20V_MIN_MV__,
      __INA260_VOLTAGE_VBUS_20V_MAX_MV__)) {
    color = __ILI9341_COLOR_VBUS_20V__;
  }
  else {
    color = __ILI9341_COLOR_VBUS_OTHER__;
  }

  return color;
}