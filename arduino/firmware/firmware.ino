#include <Arduino.h>

// ====  Pin configuration ====
#define PIN_VIB_SNS 0
#define PIN_AMB_SNS 1
#define PIN_PW_CTL 4
#define PIN_MT_LED 5
#define PIN_HL_LED 6
#define PIN_TL_LED 7

// ==== Parameters ====

// Timeout
#define MAX_CNT 180 // timeout in second

// Sensor
#define VIB_THRESHOLD 100 // 0-1023, lower is more sensitive
// 0-1023, lower is darker
#define AMB_THRESHOLD_NIGHT 700 // It's night if ambient light is below this
#define AMB_THRESHOLD_DAY 850   // It's day if ambient light is above this

// Flash control
#define BLINK_PERIOD_1 60  // FLASH
#define BLINK_PERIOD_2 40  // DIM
#define BLINK_PERIOD_3 60  // FLASH
#define BLINK_PERIOD_4 840 // DIM

// Brightness (0-255)
// Meter brightness is constant
#define METER_BRIGHTNESS 15
// Headlight brightness is different for day and night
#define HEADLIGHT_DAY_BRIGHTNESS 4
#define HEADLIGHT_NIGHT_BRIGHTNESS 30
// Taillight brightness is not changed in day or night but it blinks
#define TAILLIGHT_BASE_BRIGHTNESS 5
#define TAILLIGHT_BRIGHTNESS 24

bool is_dark = false;

// This is working on 20MHz
// 22.7us per adc read
// (5 * 1000 * 100 times adc read took 11.35s)

void wait_ms(int ms, int *cnt, bool drive_light) {

  ms = ms * 0.94; // Cancel out adc sampling delay

  for (int i = 0; i < ms; i++) {
    // 51.7us per loop
    {
      if (drive_light) {
        const int amb = analogRead(PIN_AMB_SNS); // 0-1023
        if (is_dark) {
          is_dark = amb < AMB_THRESHOLD_DAY;
        } else {
          is_dark = amb < AMB_THRESHOLD_NIGHT;
        }
        if (is_dark) {
          analogWrite(PIN_HL_LED, HEADLIGHT_NIGHT_BRIGHTNESS);
        } else {
          analogWrite(PIN_HL_LED, HEADLIGHT_DAY_BRIGHTNESS);
        }
      }

      const int val = analogRead(PIN_VIB_SNS); // 0-1023

      if (val > VIB_THRESHOLD) {
        *cnt = 0;
      }
    }
    delay(1);
  }
}

void setup() {
  pinMode(PIN_VIB_SNS, INPUT);
  pinMode(PIN_AMB_SNS, INPUT);
  pinMode(PIN_PW_CTL, OUTPUT);
  pinMode(PIN_MT_LED, OUTPUT);
  pinMode(PIN_HL_LED, OUTPUT);
  pinMode(PIN_TL_LED, OUTPUT);

  digitalWrite(PIN_PW_CTL, HIGH);

  int cnt = 0;

  analogWrite(PIN_HL_LED, HEADLIGHT_NIGHT_BRIGHTNESS);
  delay(60);
  analogWrite(PIN_HL_LED, HEADLIGHT_DAY_BRIGHTNESS);
  delay(40);
  analogWrite(PIN_HL_LED, HEADLIGHT_NIGHT_BRIGHTNESS);
  delay(60);
  analogWrite(PIN_HL_LED, HEADLIGHT_DAY_BRIGHTNESS);
  delay(200);

  while (true) {
    // Setup meter light
    analogWrite(PIN_MT_LED, METER_BRIGHTNESS);

    // Blink tail light
    analogWrite(PIN_TL_LED, TAILLIGHT_BRIGHTNESS);
    wait_ms(BLINK_PERIOD_1, &cnt, true);
    analogWrite(PIN_TL_LED, TAILLIGHT_BASE_BRIGHTNESS);
    wait_ms(BLINK_PERIOD_2, &cnt, true);
    analogWrite(PIN_TL_LED, TAILLIGHT_BRIGHTNESS);
    wait_ms(BLINK_PERIOD_3, &cnt, true);
    analogWrite(PIN_TL_LED, TAILLIGHT_BASE_BRIGHTNESS);
    wait_ms(BLINK_PERIOD_4, &cnt, true);

    // Check timeout
    cnt++;
    if (cnt >= MAX_CNT) {
      analogWrite(PIN_MT_LED, 0);
      analogWrite(PIN_HL_LED, 0);
      analogWrite(PIN_TL_LED, 0);

      // Wair for vibration
      while (true) {
        digitalWrite(PIN_PW_CTL, LOW);

        wait_ms(100, &cnt, false);
        if (cnt == 0) {
          break;
        }
      }
    }
  }
}

void loop() {}