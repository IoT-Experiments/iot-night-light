#include "strip_ctrl.h"

StripCtrl::StripCtrl(uint16_t pixelCount, uint8_t pin, neoPixelType type) {
  this->strip = Adafruit_NeoPixel(pixelCount, pin, type);
}

StripCtrl::~StripCtrl() {

}

void StripCtrl::setPixelsColor(uint32_t color) {
  for(int i = 0; i < this->strip.numPixels(); i++) {
    this->strip.setPixelColor(i, color);
  }
}

void StripCtrl::setPixelsColor(uint32_t color, uint8_t brightness) {
  uint8_t r = (color >> 16) * brightness / 100.0f;
  uint8_t g = (color & 0x00ff00 >> 8) * brightness / 100.0f;
  uint8_t b = (color & 0x0000ff) * brightness / 100.0f;

  for(int i = 0; i < this->strip.numPixels(); i++) {
    this->strip.setPixelColor(i, Adafruit_NeoPixel::Color(r, g, b));
  }
}

void StripCtrl::on() {
  this->setPixelsColor(Adafruit_NeoPixel::Color(255, 255, 0));
  this->strip.setPixelColor(0, 255, 255, 255);
  this->strip.setPixelColor(1, 255, 255, 255);
  this->strip.setPixelColor(2, 255, 255, 255);
  this->strip.show();
}
void StripCtrl::off() {
  this->strip.setPixelColor(0, 0, 0, 0);
  this->strip.setPixelColor(1, 0, 0, 0);
  this->strip.setPixelColor(2, 0, 0, 0);
  this->strip.show();
}

void StripCtrl::animate(StripMode mode) {
  switch(mode) {
    case BOOT:
      this->setPixelsColor(Adafruit_NeoPixel::Color(64, 64, 0));
      this->strip.show();
    break;
    case ERROR:
      this->setPixelsColor(Adafruit_NeoPixel::Color(255, 0, 0));
      this->strip.show();
    break;
    case ACCESS_POINT:
    this->strip.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, 0));
    this->strip.setPixelColor(1, Adafruit_NeoPixel::Color(0, 0, 255));
    this->strip.setPixelColor(2, Adafruit_NeoPixel::Color(0, 0, 0));
      this->strip.show();
    break;
    case CONNECTION_IN_PROGRESS:
      this->strip.setPixelColor(0, Adafruit_NeoPixel::Color(255, 255, 255));
      this->strip.setPixelColor(1, Adafruit_NeoPixel::Color(0, 0, 255));
      this->strip.setPixelColor(2, Adafruit_NeoPixel::Color(255, 255, 255));
      this->strip.show();
    break;
  }
}
