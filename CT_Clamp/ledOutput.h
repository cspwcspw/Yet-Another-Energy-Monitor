
#pragma once

// if you don't have ihardware, do nothing ...

#if true
#define displayDouble(...)
#define ledBegin() 
#else

#include <LedControl.h>

// Create a new LedControl for 1 device ...
LedControl ledDisplay = LedControl(6, 4, 5, 1); // Data, Clk, CS, Numdevices

// This is about an 8-segment LED, not 8x8 pixels

const int numDigits = 8;  // this is for 8 position 8-segment leds, index 0 on the right.

void ledBegin()
{
  ledDisplay.shutdown(0, false);
  ledDisplay.setIntensity(0, 7); // 15 = brightest
}

void displayDouble(double x, int width, int precision, int atPos)  // atPos 0 is rightmost
{
  ledDisplay.clearDisplay(0);
  char buf[15];
  dtostrf(x, width + 1, precision, buf);
 // Serial.println(buf);
  int pBuf = 0;
  int pDisp = width+atPos-1;
  while (pBuf < width + 1) {
    char c = buf[pBuf++];
    bool showPoint = buf[pBuf] == '.';
    ledDisplay.setChar(0, pDisp--, c, showPoint);
    if (showPoint) pBuf++;
  }
}
#endif
