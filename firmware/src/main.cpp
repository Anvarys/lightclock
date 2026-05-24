#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <MUIU8g2.h>
#include <Adafruit_MPR121.h>

#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

#define I2C0_SDA 4
#define I2C0_SCL 5

#define MPR121_0_IRQ 10
#define MPR121_1_IRQ 11
#define MPR121_2_IRQ 20

#define MPR121_AUTOCONFIG true

Adafruit_MPR121 mpr121_0 = Adafruit_MPR121();
Adafruit_MPR121 mpr121_1 = Adafruit_MPR121();
Adafruit_MPR121 mpr121_2 = Adafruit_MPR121();


U8G2_SSD1309_128X64_NONAME0_F_HW_I2C display(
  U8G2_R2, // 180 degrees rotation because the screen is flipped
  U8X8_PIN_NONE,
  I2C0_SCL,
  I2C0_SDA
);

volatile bool irqFired = false;

bool circleClicked = false;
uint8_t circleClickedId = 0;

enum ControlButton {
  SQUARE, DOWN, UP, SELECT
};
bool controlsClicked = false;
ControlButton controlButtonClicked = SQUARE;

void onIRQ() {
  irqFired = true;
}

void setup() {
  display.begin();

  Wire.begin();

  mpr121_0.begin(0x5A, &Wire, MPR121_TOUCH_THRESHOLD_DEFAULT, MPR121_RELEASE_THRESHOLD_DEFAULT, MPR121_AUTOCONFIG);
  mpr121_1.begin(0x5B, &Wire, MPR121_TOUCH_THRESHOLD_DEFAULT, MPR121_RELEASE_THRESHOLD_DEFAULT, MPR121_AUTOCONFIG);
  mpr121_2.begin(0x5C, &Wire, MPR121_TOUCH_THRESHOLD_DEFAULT, MPR121_RELEASE_THRESHOLD_DEFAULT, MPR121_AUTOCONFIG);

  pinMode(MPR121_0_IRQ, INPUT_PULLUP);
  pinMode(MPR121_1_IRQ, INPUT_PULLUP);
  pinMode(MPR121_2_IRQ, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(MPR121_0_IRQ), onIRQ, FALLING);
  attachInterrupt(digitalPinToInterrupt(MPR121_1_IRQ), onIRQ, FALLING);
  attachInterrupt(digitalPinToInterrupt(MPR121_2_IRQ), onIRQ, FALLING);
}


void updateMPR121s() {
  if (!irqFired) return;
  irqFired = false;

  int16_t maxDelta = 0;
  uint8_t maxDeltaId = -1;

  for (uint8_t i = 0; i < 24; i++) {
    int16_t delta = 0;

    if (i < 12) {
      delta = mpr121_0.filteredData(i) - mpr121_0.baselineData(i);
    } else {
      delta = mpr121_1.filteredData(i-12) - mpr121_1.baselineData(i-12);
    }

    if (delta >= MPR121_TOUCH_THRESHOLD_DEFAULT) {
      if (maxDelta < delta) {
        maxDelta = delta;
        maxDeltaId = i;
      }
    }
  }

  if (maxDeltaId != -1) {
    circleClicked = true;
    circleClickedId = maxDeltaId;
  } else {
    circleClicked = false;
  }

  maxDelta = 0;
  maxDeltaId = -1;

  for (uint8_t i = 0; i < 4; i++) {
    int16_t delta = mpr121_0.filteredData(i) - mpr121_0.baselineData(i);

    if (delta >= MPR121_TOUCH_THRESHOLD_DEFAULT) {
      if (maxDelta < delta) {
        maxDelta = delta;
        maxDeltaId = i;
      }
    }
  }

  if (maxDeltaId != -1) {
    controlsClicked = true;
    controlButtonClicked = (ControlButton) maxDeltaId;
  }
}


void loop() {
  updateMPR121s();
}