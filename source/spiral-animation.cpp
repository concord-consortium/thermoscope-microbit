#include "thermoscope.h"
#include "spiral-animation.h"

void displayAnimationBasic()
{
  display.clear();
  display.image.setPixelValue(0, 0, 255);
}

void displayAnimation()
{
  display.clear();
  display.print('C');
}

void displayAnimationEyes()
{
  display.clear();
  // display.print('C');
  display.image.setPixelValue(1, 0, 255);
  display.image.setPixelValue(3, 0, 255);
}

void displayAnimationSpiral()
{
  int8_t i = 0;
  int8_t bl = 0;
  int8_t al = 0;
  int8_t bx = 0;
  int8_t j = 0;
  int8_t bu = 0;
  int8_t au = 0;
  int8_t b = 0;
  int8_t a = 0;
  int8_t ax = 0;
  ax = 1;
  au = 4;
  bu = 4;
  j = 1;
  a = al;
  b = bl;
  // start off with some non initial point
  uint8_t oldA = a + 1;
  uint8_t oldB = b + 1;
  uint8_t oldOldA = a + 2;
  uint8_t oldOldB = b + 2;
  while(ble.getGapState().connected) {
      display.image.setPixelValue(a, b, 255);
      display.image.setPixelValue(oldOldA, oldOldB, 0);
      oldOldA = oldA;
      oldOldB = oldB;
      oldA = a;
      oldB = b;
      fiber_sleep(200);
      if (a >= au && ax > 0) {
          ax = 0;
          bx = 1;
      } else if (b >= bu && bx > 0) {
          ax = -1;
          bx = 0;
      } else if (a <= al && ax < 0) {
          ax = 0;
          bx = -1;
      } else if (b <= bl && bx < 0) {
          bx = 0;
          ax = 1;
      }
      a = a + ax;
      b = b + bx;
      if (i > 0) {
          if (a == bl && b == bl) {
              if (au == 1) {
                  j = 1;
              } else if (au == 4) {
                  j = -1;
              }
              au = au + j;
              bu = bu + j;
              i = 0;
          }
      } else {
          i += 1;
      }
  }

}
