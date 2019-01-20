#ifndef _STRIP_CTRL_H_
#define _STRIP_CTRL_H_

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

enum StripMode {
  BOOT = 0,
  ACCESS_POINT,
  ERROR,
  CONNECTION_IN_PROGRESS,
};

class StripCtrl {
  public:
    StripCtrl(uint16_t pixelCount, uint8_t pin=6, neoPixelType type=NEO_GRB + NEO_KHZ800);
    ~StripCtrl();

    void show() {
      this->strip.show();
    };
    void begin() {
      this->strip.begin();
    };

    void on();
    void off();

    void setPixelsColor(uint32_t color);
    void setPixelsColor(uint32_t color, uint8_t brightness);
    void animate(StripMode mode);

  private:
    Adafruit_NeoPixel strip;
};

#endif
