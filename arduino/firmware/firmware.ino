#include <Arduino.h>

// Pin configuration
#define PIN_VIB_SNS 0
#define PIN_AMB_SNS 1
#define PIN_PW_CTL 4
#define PIN_MT_LED 5
#define PIN_HL_LED 6
#define PIN_TL_LED 7

// Parameters
#define MAX_CNT 20        // timeout in second
#define VIB_THRESHOLD 100 // 0-1023, lower is more sensitive
#define BLINK_PERIOD_1 60
#define BLINK_PERIOD_2 40
#define BLINK_PERIOD_3 60
#define BLINK_PERIOD_4 840

#define METER_BRIGHTNESS 6
#define HEADLIGHT_DAY_BRIGHTNESS 6
#define HEADLIGHT_NIGHT_BRIGHTNESS 50
#define TAILLIGHT_BASE_BRIGHTNESS 8
#define TAILLIGHT_BRIGHTNESS 30

// This is working on 20MHz
// 22.7us per adc read
// (5 * 1000 * 100 times adc read took 11.35s)

// 5 * 40 * 1000 = 6.35s

void wait_ms(int ms, int *cnt) {

  ms = ms * 0.96; // Cancel out adc sampling delay

  for (int i = 0; i < ms; i++) {
    // 31.75us per loop
    {
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

  analogWrite(PIN_HL_LED, 127);

  wait_ms(100, &cnt);

  while (true) {
    const bool is_dark = true; // TODO: read from ambient sensor
    if (is_dark) {
      analogWrite(PIN_HL_LED, HEADLIGHT_NIGHT_BRIGHTNESS);
    } else {
      analogWrite(PIN_HL_LED, HEADLIGHT_DAY_BRIGHTNESS);
    }
    analogWrite(PIN_MT_LED, METER_BRIGHTNESS);

    analogWrite(PIN_TL_LED, TAILLIGHT_BRIGHTNESS);
    wait_ms(BLINK_PERIOD_1, &cnt);
    analogWrite(PIN_TL_LED, TAILLIGHT_BASE_BRIGHTNESS);
    wait_ms(BLINK_PERIOD_2, &cnt);
    analogWrite(PIN_TL_LED, TAILLIGHT_BRIGHTNESS);
    wait_ms(BLINK_PERIOD_3, &cnt);
    analogWrite(PIN_TL_LED, TAILLIGHT_BASE_BRIGHTNESS);
    wait_ms(BLINK_PERIOD_4, &cnt);
    cnt++;
    if (cnt >= MAX_CNT) {
      break;
    }
  }

  analogWrite(PIN_MT_LED, 0);
  analogWrite(PIN_HL_LED, 0);
  analogWrite(PIN_TL_LED, 0);
  digitalWrite(PIN_PW_CTL, LOW);
}

void loop() {}