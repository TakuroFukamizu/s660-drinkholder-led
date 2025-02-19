/**
 * S660 Drink Holder LED
 * 
 * @Hardwares: M5NanoC6
 * @Platform Version: Arduino ESP32 Board Manager v2.1.3
 * @Libraries: M5Unified, EspEasyUtils, ESP32-BLE_MIDI
 */

#include <M5Unified.h>
#include <EspEasyLED.h>
#include <EspEasyGPIOInterrupt.h>
#include <EspEasyTask.h>
// #include <BLEMidi.h>
#include <Preferences.h>

// NOTE: BLE MIDI is not working now at ESP32-C6
// https://github.com/h2zero/NimBLE-Arduino/issues/642

#define PREF_NS "mode_cfg"
#define PREF_MODE "i_mode"
#define PREF_MODE1_COLOR "i_mode1_color"

#define TASK_PRIORITY_PERFORM 20

#define MODE_NUM 3
#define MODE1_COLOR_NUM 6

#define BLE_DEVICE_NAME "S660-DRINKHOLDER-LED"
#define MIDI_CC_COLOR_R 1 // CC1 Set Red valuse(0-255)
#define MIDI_CC_COLOR_G 2 // CC2 Set Green valuse(0-255)
#define MIDI_CC_COLOR_B 3 // CC3 Set Blue valuse(0-255)
#define MIDI_MAX_BRIGHTNESS 255
#define MIDI_MAX_VALUE 127

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

bool midiLedOn = false;
uint8_t midiBrightness = MIDI_MAX_BRIGHTNESS;
EspEasyLED::color_t midiColor = EspEasyLEDColor::WHITE;

Preferences preferences;

//-------------------------------

void startPeformance();
void stopPerformance();

void ledBasicTask();
void ledParipiTask();
void ledBleMidiTask();
void onMagSwChanged();

void onBleConnected();
void onBleDisconnected();
void onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp);
void onNoteOff(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp);
void onControlChange(uint8_t channel, uint8_t controller, uint8_t value, uint16_t timestamp);

//-------------------------------

void setup() {
  M5.begin();
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_VERBOSE);
  // M5.Log.setEnableColor(m5::log_target_serial, true);

  pinMode(GPIO_NUM_2, INPUT_PULLUP); // GROVE - Magnet SW
  pinMode(GPIO_NUM_19, OUTPUT); // Bult-in RGB LED PWR
  pinMode(GPIO_NUM_7, OUTPUT); // BLUE LED

  preferences.begin(PREF_NS, true); // read-only mode
  if(preferences.isKey(PREF_MODE)) {
    mode = preferences.getUChar(PREF_MODE);
  }
  if(preferences.isKey(PREF_MODE1_COLOR)) {
    mode1color = preferences.getUChar(PREF_MODE1_COLOR);
  }
  preferences.end();

  magSwInterrupt.begin(onMagSwChanged, GPIO_NUM_2, CHANGE);

  // // Setup BLE MIDI
  // BLEMidiServer.begin(BLE_DEVICE_NAME);
  // BLEMidiServer.setOnConnectCallback(onBleConnected);
  // BLEMidiServer.setOnDisconnectCallback(onBleDisconnected);
  // BLEMidiServer.setNoteOnCallback(onNoteOn);
  // BLEMidiServer.setNoteOffCallback(onNoteOff);
  // BLEMidiServer.setControlChangeCallback(onControlChange);

  // All off
  digitalWrite(GPIO_NUM_19, LOW); // LED off
  statusLed.clear();
  perfomanceLed.clear();

  randomSeed(analogRead(0)); 
  
  // initialize complete
  digitalWrite(GPIO_NUM_7, HIGH); // LED on

  // first run
  onMagSwChanged();

  M5.Log(ESP_LOG_INFO, "init complete");
}

void loop() {
  M5.delay(10);

  M5.update();

  if (M5.BtnA.wasSingleClicked()) {
    mode++;
    if (MODE_NUM <= mode) {
      mode = 0;
    }
    M5.Log(ESP_LOG_INFO, "change mode");
    stopPerformance();
    startPeformance();

    // save config
    preferences.begin(PREF_NS, false); // read-write mode
    preferences.putUChar(PREF_MODE, mode);
    preferences.end();
  }

  // MODE1 Change color
  if (M5.BtnA.wasDoubleClicked() && mode == 0) {
    mode1color++;
    if (MODE1_COLOR_NUM <= mode1color) {
      mode1color = 0;
    }
    M5.Log(ESP_LOG_INFO, "change mode1 color");

    // save config
    preferences.begin(PREF_NS, false); // read-write mode
    preferences.putUChar(PREF_MODE1_COLOR, mode1color);
    preferences.end();
  }

  // if (M5.BtnA.wasSingleClicked()) state = !state;
}

void startPeformance() {
  M5.Log(ESP_LOG_INFO, "startPeformance");
  digitalWrite(GPIO_NUM_19, HIGH); // LED on
  statusLed.showColor(EspEasyLEDColor::GREEN);
  switch (mode) {
    case 0: 
      M5.Log(ESP_LOG_DEBUG, "start basic performance task");
      performanceTask.begin(ledBasicTask, TASK_PRIORITY_PERFORM, ESP_EASY_TASK_CPU_NUM);
      break;
    case 1: 
      M5.Log(ESP_LOG_DEBUG, "start paripi performance task");
      performanceTask.begin(ledParipiTask, TASK_PRIORITY_PERFORM, ESP_EASY_TASK_CPU_NUM);
      break;
    case 2: 
      M5.Log(ESP_LOG_DEBUG, "start BLE-MIDI performance task");
      performanceTask.begin(ledBleMidiTask, TASK_PRIORITY_PERFORM, ESP_EASY_TASK_CPU_NUM);
      break;
  }
}

void stopPerformance() {
    M5.Log(ESP_LOG_INFO, "stopPerformance");
    digitalWrite(GPIO_NUM_19, LOW); // LED off
    performanceTask.suspend(); // stop LED Task
    perfomanceLed.clear(); // trun off LED
}

void onMagSwChanged() {
  // set state by Magnet Button or Manual button
  state = digitalRead(GPIO_NUM_2) == HIGH; // LOW: on(close), HIGH: off(away)
  if (state) {
    M5.Log(ESP_LOG_INFO, "onMagSwChanged => away");
    startPeformance();
  } else {
    M5.Log(ESP_LOG_INFO, "onMagSwChanged => close");
    stopPerformance();
  }
}

/** LED mode : basic */
void ledBasicTask() {
  while(1){
    for(uint8_t i=0; i<100; i++) {
      perfomanceLed.setColor(0, mode1colors[mode1color]); // set color realtime
      perfomanceLed.setBrightness(i);
      perfomanceLed.show();
      delay(50);
    }
    for(uint8_t i=100; i>0; i--) {
      perfomanceLed.setColor(0, mode1colors[mode1color]);
      perfomanceLed.setBrightness(i);
      perfomanceLed.show();
      delay(50);
    }
  }
}

/** LED mode : paripi */
void ledParipiTask() {
  while(1){
    uint8_t r = random(256);
    uint8_t g = random(256);
    uint8_t b = random(256);
    uint8_t brightness = random(90)+10;
    perfomanceLed.setColor(0, r, g, b);
    perfomanceLed.setBrightness(brightness);
    perfomanceLed.show();
    delay(200);
  }
}

/** LED mode : BLE MIDI */
void ledBleMidiTask() {
  while(1){
    if (midiLedOn) {
      perfomanceLed.setColor(0, midiColor);
      perfomanceLed.setBrightness(midiBrightness);
      perfomanceLed.show();
    } else {
      perfomanceLed.clear();
    }
    delay(100);
  }
}

//-------------------------------

// void onBleConnected() {
//   midiLedOn = false;
//   midiBrightness = MIDI_MAX_BRIGHTNESS;
//   midiColor = EspEasyLEDColor::WHITE;
// }

// void onBleDisconnected() {
//   midiLedOn = false;
//   perfomanceLed.clear();
// }

// void onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp) {
//   midiBrightness = (uint8_t)((float)velocity * (float)MIDI_MAX_BRIGHTNESS / (float)MIDI_MAX_VALUE);
//   latestNoteOnTimestamp = timestamp;
//   midiLedOn = true;
// }

// void onNoteOff(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp)
// {
//   midiLedOn = false;
// }

// void onControlChange(uint8_t channel, uint8_t controller, uint8_t value, uint16_t timestamp)
// {
//   switch (controller) {
//     case MIDI_CC_COLOR_R:
//       midiColor = { value, midiColor.G, midiColor.B };
//       break;
//     case MIDI_CC_COLOR_G:
//       midiColor = { midiColor.R, value, midiColor.B };
//       break;
//     case MIDI_CC_COLOR_B:
//       midiColor = { midiColor.R, midiColor.G, value };
//       break;
//   }
// }
