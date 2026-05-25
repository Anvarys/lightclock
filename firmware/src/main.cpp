#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <MUIU8g2.h>
#include <Adafruit_MPR121.h>
#include <DS3231.h>
#include <SPI.h>
#include <Ticker.h>

#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

#define CONTROLS_COOLDOWN 150 // ms

#define OE0   6
#define OE1   7
#define OE2   8
#define LE_DM 3
#define DM    2
#define TF    0

#define PWM_STEPS 256
#define PWM_FREQ  200
#define PWM_PERIOD_US (1000000 / (PWM_STEPS * PWM_FREQ))

#define BUZZER 22

#define I2C0_SDA 4
#define I2C0_SCL 5

#define MPR121_0_IRQ 10
#define MPR121_1_IRQ 11
#define MPR121_2_IRQ 20

#define MPR121_AUTOCONFIG true


// display

U8G2_SSD1309_128X64_NONAME0_F_HW_I2C display(
  U8G2_R2, // 180 degrees rotation because the screen is flipped
  U8X8_PIN_NONE,
  I2C0_SCL,
  I2C0_SDA
);


// RTC

DS3231 RTC;
bool h12;
bool hPM;

bool rtcInterruptFired = true;

// capacitive touch buttons

Adafruit_MPR121 mpr121_0 = Adafruit_MPR121();
Adafruit_MPR121 mpr121_1 = Adafruit_MPR121();
Adafruit_MPR121 mpr121_2 = Adafruit_MPR121();

volatile bool irqFired = false;

bool circleClicked = false;
uint8_t circleClickedId = 0;

enum ControlButton {
  CB_SQUARE, CB_DOWN, CB_UP, CB_SELECT
};
bool controlsClicked = false;
bool controlsJustClicked = false;
ControlButton controlButtonClicked = CB_SQUARE;
uint64_t lastControlsClickTime = 0;
uint64_t lastButtonActivity = 0;


// LEDs

uint8_t leds[24] = {0};
volatile uint8_t pwmCounter = 0;


// state management & UX

uint64_t lastTimeUpdateTime = 0; // the time of the last update of the time when the device is inactive

enum State {
  INACTIVE, HOME, ALARM_H, ALARM_M
};
State currentState = HOME;
uint8_t selector = 0;


// alarm

uint8_t hours = 0;
uint8_t minutes = 0;
bool alarmOn = false;

// config

uint64_t timeInactive = 60 * 1000; // how much time after last button press to set state to inactive (in ms)

void onIRQ() {
  irqFired = true;
}

void setup() {
  Wire.begin();

  // display
  display.begin();
  display.clearBuffer();

  // capacitive touch ICs
  mpr121_0.begin(0x5A, &Wire, MPR121_TOUCH_THRESHOLD_DEFAULT, MPR121_RELEASE_THRESHOLD_DEFAULT, MPR121_AUTOCONFIG);
  mpr121_1.begin(0x5B, &Wire, MPR121_TOUCH_THRESHOLD_DEFAULT, MPR121_RELEASE_THRESHOLD_DEFAULT, MPR121_AUTOCONFIG);
  mpr121_2.begin(0x5C, &Wire, MPR121_TOUCH_THRESHOLD_DEFAULT, MPR121_RELEASE_THRESHOLD_DEFAULT, MPR121_AUTOCONFIG);

  pinMode(MPR121_0_IRQ, INPUT_PULLUP);
  pinMode(MPR121_1_IRQ, INPUT_PULLUP);
  pinMode(MPR121_2_IRQ, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(MPR121_0_IRQ), onIRQ, FALLING);
  attachInterrupt(digitalPinToInterrupt(MPR121_1_IRQ), onIRQ, FALLING);
  attachInterrupt(digitalPinToInterrupt(MPR121_2_IRQ), onIRQ, FALLING);

  // RTC

  RTC.setClockMode(false);

  if (!RTC.oscillatorCheck()) {
    RTC.setEpoch(1779637745); // !!! modify this to your current time
  }

  RTC.setA1Time(
    0, 0, 0, 0, 0b1111, false, false, false
  );
  RTC.turnOnAlarm(1);
  RTC.checkIfAlarm(1);

  // LED driver

  pinMode(OE0, OUTPUT);
  pinMode(OE1, OUTPUT);
  pinMode(OE2, OUTPUT);
  pinMode(LE_DM, OUTPUT);
  pinMode(DM, OUTPUT);
  pinMode(TF, INPUT);

  digitalWrite(OE0, LOW);
  digitalWrite(OE1, LOW);
  digitalWrite(OE2, LOW);
  digitalWrite(LE_DM, LOW);
  digitalWrite(DM, LOW);

  SPI.begin();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
}

void setup1() {}

void updatePWM() {
  uint32_t state = 0;
  for (uint8_t channel = 0; channel < 24; channel++) {
    if (leds[channel] > pwmCounter) {
      state |= (1UL << (23 - channel));
    }
  }

  SPI.transfer((state >> 16) & 0xFF);
  SPI.transfer((state >> 8) & 0xFF);
  SPI.transfer(state & 0xFF);

  digitalWrite(LE_DM, HIGH);
  digitalWrite(LE_DM, LOW);

  pwmCounter++;
}

void playTone(int pin, int frequency, int duration) {
  int period = 1000000 / frequency;
  int halfPeriod = period / 2;
  long cycles = (long)frequency * duration / 1000;

  for (long i = 0; i < cycles; i++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(halfPeriod);
    digitalWrite(pin, LOW);
    delayMicroseconds(halfPeriod);
  }
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
    lastButtonActivity = millis();

    if (currentState == INACTIVE) {
      currentState = HOME;
    }
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
    if (controlsClicked == false) {
      controlsJustClicked = true;
    }
    controlsClicked = true;
    controlButtonClicked = (ControlButton) maxDeltaId;
    if (currentState == INACTIVE) {
      currentState = HOME;
    }
  }
}

void updateTimeOnDisplay() {
  if (currentState != INACTIVE) {return;}

  display.clearBuffer();
  display.setFont(u8g2_font_logisoso38_tn);
  display.drawStr(20, 10, RTC.getHour(h12, hPM) + ":" + RTC.getMinute());
  display.sendBuffer();
}

void handleControlClicks() {
  if (!controlsJustClicked || millis() - lastControlsClickTime < CONTROLS_COOLDOWN) {return;}
  controlsJustClicked = false;
  lastControlsClickTime = millis();

  switch (controlButtonClicked)
  {
  case CB_SELECT:
    switch (currentState)
    {
    case HOME:
      if (selector == 0) {
        currentState = ALARM_H;
      }
      break;
    
    case ALARM_H:
      hours = circleClickedId;
      currentState = ALARM_M;
      break;

    case ALARM_M:
      minutes = circleClickedId;
      alarmOn = true;
      currentState = HOME;
      break;
    
    default:
      break;
    }
    break;

  case CB_SQUARE:
    currentState = HOME;
    break;
  
  default:
    break;
  }
}

void handleAlarm() {
  if (alarmOn) {return;}

  if (millis() - lastButtonActivity < 2000) {
    alarmOn = false;
    return;
  }

  uint64_t curTime = RTClib::now().unixtime();
  uint64_t alarmTime = curTime - curTime % 86400 + minutes * 60 + hours * 3600;

  if (alarmTime - curTime < 1200) {
    uint16_t brightness = (1400 - (alarmTime - curTime)) / 5;
    if (brightness > 255) {
      brightness = 255;
    }

    memset(leds, brightness, sizeof(leds));
  }

  if (curTime - alarmTime >= 0) {
    playTone(BUZZER, 1000, 500);
  }
}

void onSecond() {
  if (millis() - lastButtonActivity > timeInactive) {
    currentState = INACTIVE;
  }

  updateTimeOnDisplay();
  handleAlarm();
}

void onRTCAlarmInterrup() {
  rtcInterruptFired = true;
}

void loop1() {
  updatePWM();
  delayMicroseconds(PWM_PERIOD_US);
}

void loop() {
  updateMPR121s();
  handleControlClicks();
  updateTimeOnDisplay();

  if (rtcInterruptFired) {
    rtcInterruptFired = false;
    RTC.checkIfAlarm(1);
    onSecond();
  }
}