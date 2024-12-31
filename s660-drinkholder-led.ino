/**
 * S660 Drink Holder LED
 * 
 * @Hardwares: M5NanoC6
 * @Platform Version: Arduino ESP32 Board Manager v2.1.3
 */

#include <M5Unified.h>
#include <EspEasyLED.h>

// FOR M5NanoC6

EspEasyLED statusLed(GPIO_NUM_20, 1, 40);
EspEasyLED perfomanceLed(GPIO_NUM_1, 1, 128); // GROVE - RGB LED

bool state = false;

void setup() {
  M5.begin();
  pinMode(GPIO_NUM_2, INPUT_PULLUP); // GROVE - SW
  pinMode(GPIO_NUM_19, OUTPUT); // RGBPWR

  digitalWrite(GPIO_NUM_7, HIGH); // LED on
}

void loop() {
  M5.delay(1);

  M5.update();

  if (M5.BtnA.wasPressed()) state = !state;
  int sw = digitalRead(GPIO_NUM_2);

  if (state || (sw == LOW)) {
    digitalWrite(GPIO_NUM_19, HIGH); // LED on
    statusLed.showColor(EspEasyLEDColor::GREEN);
    perfomanceLed.showColor(EspEasyLEDColor::BLUE);
  } else {
    digitalWrite(GPIO_NUM_19, LOW); // LED off
    perfomanceLed.clear();
  }

}
