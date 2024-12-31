/**
 * S660 Drink Holder LED
 * 
 * @Hardwares: M5NanoC6
 * @Platform Version: Arduino ESP32 Board Manager v2.1.3
 */

#include <M5Unified.h>
#include <EspEasyLED.h>
#include <EspEasyGPIOInterrupt.h>
#include <EspEasyTask.h>

#define MODE_NUM 2
#define MODE1_COLOR_NUM 6
#define TASK_PRIORITY_PERFORM 20

//-------------------------------

EspEasyLED::color_t mode1colors[] = {
  EspEasyLEDColor::RED,
  EspEasyLEDColor::GREEN,
  EspEasyLEDColor::BLUE,
  EspEasyLEDColor::CYAN,
  EspEasyLEDColor::MAGENTA,
  EspEasyLEDColor::SKYBLUE
};

EspEasyLED statusLed(GPIO_NUM_20, 1, 40);
EspEasyLED perfomanceLed(GPIO_NUM_1, 1, 255); // GROVE - RGB LED

EspEasyTask performanceTask;

EspEasyGPIOInterrupt magSwInterrupt;

bool state = false;
uint8_t mode = 0;
uint8_t mode1color = 0;

//-------------------------------

void startPeformance();
void stopPerformance();

void performanceTask1();
void performanceTask2();
void onMagSwChanged();

//-------------------------------

void setup() {
  M5.begin();
  pinMode(GPIO_NUM_2, INPUT_PULLUP); // GROVE - Magnet SW
  pinMode(GPIO_NUM_19, OUTPUT); // Bult-in RGB LED PWR
  pinMode(GPIO_NUM_7, OUTPUT); // BLUE LED

  // performanceTask.begin(performanceTask, 2, APP_CPU_NUM);
  magSwInterrupt.begin(onMagSwChanged, GPIO_NUM_2, CHANGE);

  // All off
  digitalWrite(GPIO_NUM_19, LOW); // LED off
  statusLed.clear();
  perfomanceLed.clear();

  randomSeed(analogRead(0)); 
  
  // initialize complete
  digitalWrite(GPIO_NUM_7, HIGH); // LED on

  // first run
  onMagSwChanged();
}

void loop() {
  M5.delay(10);

  M5.update();

  if (M5.BtnA.wasSingleClicked()) {
    mode++;
    if (MODE_NUM <= mode) {
      mode = 0;
    }
    stopPerformance();
    startPeformance();
  }

  // MODE1 Change color
  if (M5.BtnA.wasDoubleClicked()) {
    mode1color++;
    if (MODE1_COLOR_NUM <= mode) {
      mode = 0;
    }
    if (mode == 0) {
      stopPerformance();
      startPeformance();
    }
  }

  // if (M5.BtnA.wasSingleClicked()) state = !state;
}

void startPeformance() {
  digitalWrite(GPIO_NUM_19, HIGH); // LED on
  statusLed.showColor(EspEasyLEDColor::GREEN);
  switch (mode) {
    case 0: 
      performanceTask.begin(performanceTask1, TASK_PRIORITY_PERFORM, ESP_EASY_TASK_CPU_NUM);
      break;
    case 1: 
      performanceTask.begin(performanceTask2, TASK_PRIORITY_PERFORM, ESP_EASY_TASK_CPU_NUM);
      break;
  }
}

void stopPerformance() {
    digitalWrite(GPIO_NUM_19, LOW); // LED off
    performanceTask.resume(); // stop LED
    perfomanceLed.clear();
}

void onMagSwChanged() {
  // set state by Magnet Button or Manual button
  state = digitalRead(GPIO_NUM_2) == LOW;
  if (state) {
    startPeformance();
  } else {
    stopPerformance();
  }
}

/** LED mode : paripi */
void performanceTask1() {
  perfomanceLed.setColor(0, mode1colors[mode1color]);
  while(1){
    for(uint8_t i=0; i<100; i++) {
      perfomanceLed.setBrightness(i);
      perfomanceLed.show();
      delay(50);
    }
    for(uint8_t i=100; i>0; i--) {
      perfomanceLed.setBrightness(i);
      perfomanceLed.show();
      delay(50);
    }
  }
}

/** LED mode : paripi */
void performanceTask2() {
  uint8_t brightness = 10;
  while(1){
    uint8_t r = random(256);
    uint8_t g = random(256);
    uint8_t b = random(256);
    perfomanceLed.setColor(0, r, g, b);
    perfomanceLed.setBrightness(brightness);
    perfomanceLed.show();
    delay(500);
    brightness++;
    if (100 < brightness) brightness = 10;
  }
}

