#include "matrix_keyboard.h"
#include <LiquidCrystal.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>

//LCD D4-7 are Digital Pins 5,4,3, and 8 respectively
/*
LCD Pins:
VSS Ground
VDD 5V
V0 Potentiometer
RS DP12
RW Ground
E DP11
D0-3 No Connection
A Resistor -> 5V
K Ground

VL53L0X Pins: 
5V
GND
SCL Analog 5
SDA Analog 4

FLOW Meter Pins: 
5V
GND
Yellow DP2

MOSFETS:
Pump - DP6
Bypass Line - DP9
Mainline - DP10
*/

//-----------Devices--------------------
LiquidCrystal lcd(12, 11, 5, 4, 3, 8);
Adafruit_VL53L0X sensor;
MatrixKeyboard key_board(0x65);
//--------------------------------------

//----------Pins------------------------
const byte flowPin = 2;
const byte pumpPin = 6;
const byte bypassPin = 9;
const byte mainlinePin = 10;
//--------------------------------------

//--------------Constants---------------
const float TIP_TO_SENSOR_MM = 170.78f;
const float MAX_DENSITY_SPREAD = 0.004f;
const float calibrationFactor = 17.0f;

const unsigned long flowPeriodMs = 1000;
const unsigned long primeTimeMs = 10000;
const unsigned long overlapTimeMs = 150;
const unsigned long densityPeriodMs = 500;
const unsigned long densityTimeoutMs = 15000;

// Store calibration tables in flash to save Uno SRAM.
// Entries at the same index correspond to one another.
const float tipDistanceMM[] PROGMEM = {
  53.89, 57.80, 61.57, 66.72, 72.69,
  78.89, 83.49, 87.10, 90.27, 94.96,
  98.31, 101.88, 106.24, 109.43, 112.62
};

const float density[] PROGMEM = {
  0.990, 0.996, 1.002, 1.010, 1.020,
  1.030, 1.038, 1.044, 1.050, 1.058,
  1.064, 1.070, 1.078, 1.084, 1.090
};
//--------------------------------------

const size_t POINT_COUNT =
    sizeof(tipDistanceMM) / sizeof(tipDistanceMM[0]);

//---------------Variables--------------
volatile unsigned long pulseCount = 0;

unsigned long lastFlowTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastTelemetryTime = 0;
unsigned long lastDensityAttempt = 0;
unsigned long phaseStartedAt = 0;
unsigned long densitySearchStartedAt = 0;

unsigned long totalMilliLitres = 0;
unsigned long dispenseStartMl = 0;
unsigned long totalGrams = 0;
float fractionalMilliLitres = 0.0f;

float targetkg = 0.0f;  //input
float lockedDensity = 0.0f;
bool densityLocked = false;

float recentDensities[3];
byte readingCount = 0;
byte nextIndex = 0;

bool pumpState = false;
bool bypassState = false;
bool mainlineState = false;

bool enteringTarget = false;
unsigned int enteredKg = 0;
//--------------------------------------

enum Phase { IDLE, PRIMING, OVERLAP, RUNNING };
Phase phase = IDLE;

char serialCommand[24];
byte serialCommandLength = 0;

//----------Interpolation----------------------------------------
bool interpolateDensity(float distanceFromTip, float &result) {
  float first = pgm_read_float(&tipDistanceMM[0]);
  float last = pgm_read_float(&tipDistanceMM[POINT_COUNT - 1]);

  if (distanceFromTip < first || distanceFromTip > last) {
    return false;
  }

  for (size_t i = 0; i < POINT_COUNT - 1; i++) {
    float lowerDistance = pgm_read_float(&tipDistanceMM[i]);
    float upperDistance = pgm_read_float(&tipDistanceMM[i + 1]);

    if (distanceFromTip <= upperDistance) {
      float lowerDensity = pgm_read_float(&density[i]);
      float upperDensity = pgm_read_float(&density[i + 1]);
      float fraction =
          (distanceFromTip - lowerDistance) /
          (upperDistance - lowerDistance);

      result = lowerDensity +
               fraction * (upperDensity - lowerDensity);
      return true;
    }
  }

  return false;
}
//--------------------------------------


//-------------------Helper Functions----
void pulseCounter() {
  pulseCount++;
}

void setPump(bool on) {
  pumpState = on;
  digitalWrite(pumpPin, on ? HIGH : LOW);
}

void setBypass(bool open) {
  bypassState = open;
  digitalWrite(bypassPin, open ? HIGH : LOW);
}

void setMainline(bool open) {
  mainlineState = open;
  digitalWrite(mainlinePin, open ? HIGH : LOW);
}

void stopSystem() {
  phase = IDLE;
  setPump(false);
  setMainline(false);
  setBypass(false);
}
//--------------------------------------

//--------------------------------------
void startSystem() {
  if (phase != IDLE || pumpState ||
      targetkg < 0.05f || targetkg > 20.0f) {
    return;
  }

  dispenseStartMl = totalMilliLitres;

  // Obtain a fresh density reading for this run.
  densityLocked = false;
  lockedDensity = 0.0f;
  totalGrams = 0;
  readingCount = 0;
  nextIndex = 0;

  setBypass(true);
  setMainline(false);
  setPump(true);

  phase = PRIMING;
  phaseStartedAt = millis();
}
//--------------------------------------


//--------------------------------------
void updateDensity() {
  if (densityLocked ||
      (phase != OVERLAP && phase != RUNNING)) {
    return;
  }

  unsigned long now = millis();
  if (now - lastDensityAttempt < densityPeriodMs) return;
  lastDensityAttempt = now;

  VL53L0X_RangingMeasurementData_t measurement;
  sensor.rangingTest(&measurement, false);

  if (measurement.RangeStatus != 0) return;

  float distanceFromTipMM =
      TIP_TO_SENSOR_MM - measurement.RangeMilliMeter;
  float currentDensity;

  if (!interpolateDensity(distanceFromTipMM, currentDensity)) {
    readingCount = 0;
    nextIndex = 0;
    return;
  }

  recentDensities[nextIndex] = currentDensity;
  nextIndex = (nextIndex + 1) % 3;

  if (readingCount < 3) readingCount++;

  if (readingCount == 3) {
    float minimum = recentDensities[0];
    float maximum = recentDensities[0];
    float sum = 0.0f;

    for (byte i = 0; i < 3; i++) {
      if (recentDensities[i] < minimum)
        minimum = recentDensities[i];
      if (recentDensities[i] > maximum)
        maximum = recentDensities[i];

      sum += recentDensities[i];
    }

    if (maximum - minimum <= MAX_DENSITY_SPREAD) {
      lockedDensity = sum / 3.0f;
      densityLocked = true;
      totalGrams =
          (unsigned long)(totalMilliLitres * lockedDensity);
    }
  }
}
//--------------------------------------

//--------------------------------------
void updateFlow() {
  unsigned long now = millis();
  unsigned long elapsedMs = now - lastFlowTime;
  if (elapsedMs < flowPeriodMs) return;

  noInterrupts();
  unsigned long pulses = pulseCount;
  pulseCount = 0;
  interrupts();

  lastFlowTime = now;

  float flowRateLpm =
      (1000.0f * pulses / elapsedMs) / calibrationFactor;

  float intervalMl =
      (flowRateLpm * elapsedMs / 60.0f) +
      fractionalMilliLitres;

  unsigned long wholeMl = (unsigned long)intervalMl;
  totalMilliLitres += wholeMl;
  fractionalMilliLitres = intervalMl - wholeMl;

  if (densityLocked) {
    totalGrams =
        (unsigned long)(totalMilliLitres * lockedDensity);
  }
}
//--------------------------------------

//--------------------------------------
void updateSequence() {
  unsigned long now = millis();

  if (phase == PRIMING &&
      now - phaseStartedAt >= primeTimeMs) {
    // Briefly open both paths before closing the bypass.
    setMainline(true);
    phase = OVERLAP;
    phaseStartedAt = now;
    densitySearchStartedAt = now;
  } else if (phase == OVERLAP &&
             now - phaseStartedAt >= overlapTimeMs) {
    setBypass(false);
    phase = RUNNING;
  }

  if (phase != OVERLAP && phase != RUNNING) return;

  if (!densityLocked) {
    if (now - densitySearchStartedAt >= densityTimeoutMs) {
      stopSystem();
    }
    return;
  }

  float dispensedKg =
      (totalMilliLitres - dispenseStartMl) *
      lockedDensity / 1000.0f;

  if (dispensedKg >= targetkg) {
    stopSystem();
  }
}
//--------------------------------------

//--------------------------------------
void inputDisplay(float kg) {
  lcd.setCursor(0, 0);
  lcd.print(F("                "));
  lcd.setCursor(0, 0);
  lcd.print(F("ENTER KG, # OK"));

  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("kg: "));
  lcd.print(kg, 0);
}
//--------------------------------------

//--------------------------------------
void handleCommand(char command) {
  if (command == '\0') return;

  if (enteringTarget) {
    if (command >= '0' && command <= '9') {
      unsigned int next =
          enteredKg * 10 + (command - '0');
      if (next <= 20) enteredKg = next;
    } else if (command == '*') {
      enteredKg /= 10;
    } else if (command == '#') {
      if (enteredKg >= 1 && enteredKg <= 20) {
        enteringTarget = false;
        targetkg = enteredKg;
        startSystem();
      }
    } else if (command == 'A') {
      enteringTarget = false;
    }
    return;
  }

  if (command == 'A') {
    if (phase != IDLE || pumpState) {
      stopSystem();
    } else {
      enteredKg = 0;
      enteringTarget = true;
    }
  } else if (phase == IDLE && command == 'B') {  //Toggle Pump
    setPump(!pumpState);
  } else if (phase == IDLE && command == 'C') {  //Toggle Mainline
    setMainline(!mainlineState);
  } else if (phase == IDLE && command == 'D') {  //Toggle Bypass Line
    setBypass(!bypassState);
  }
}
//--------------------------------------

//--------------------------------------
void readEspCommand() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n') {
      serialCommand[serialCommandLength] = '\0';
      serialCommandLength = 0;

      if (strcmp(serialCommand, "X") == 0) {
        enteringTarget = false;
        stopSystem();
      } else if (strncmp(serialCommand, "R,", 2) == 0 &&
                 phase == IDLE && !pumpState) {
        char *end;
        float kg = strtod(serialCommand + 2, &end);

        if (*end == '\0' &&
            kg >= 0.05f && kg <= 20.0f) {
          enteringTarget = false;
          targetkg = kg;
          startSystem();
        }
      }
    } else if (c != '\r') {
      if (serialCommandLength <
          sizeof(serialCommand) - 1) {
        serialCommand[serialCommandLength++] = c;
      } else {
        serialCommandLength = 0;
      }
    }
  }
}
//--------------------------------------

//*******************LCD DISPLAY**************************
//--------------------------------------
void welcomeDisplay() {
  lcd.setCursor(0, 0);
  lcd.print(F("                "));
  lcd.setCursor(0, 0);
  lcd.print(F("SELECT OPTION:"));

  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("A B C D"));
}
//--------------------------------------

//--------------------------------------
void updateDisplay() {
  lcd.setCursor(0, 0);
  lcd.print(F("                "));
  lcd.setCursor(0, 0);
  lcd.print(F("D:"));

  if (densityLocked) {
    lcd.print(lockedDensity, 3);
  } else {
    lcd.print(F("----"));
  }

  lcd.print(F(" L:"));
  lcd.print(totalMilliLitres / 1000.0f, 0);

  lcd.setCursor(0, 1);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("kg:"));

  if (densityLocked) {
    lcd.print(totalGrams / 1000.0f, 0);
  } else {
    lcd.print(F("----"));
  }

  lcd.print('/');
  lcd.print(targetkg, 0);
}
//--------------------------------------
//************************************************************

//--------------------------------------
void sendTelemetry() {
  if (millis() - lastTelemetryTime < 1000) return;
  lastTelemetryTime = millis();

  // T,valid,density_g_per_mL,mass_kg,volume_L,phase
  Serial.print(F("T,"));
  Serial.print(densityLocked ? 1 : 0);
  Serial.print(',');
  Serial.print(densityLocked ? lockedDensity : 0.0f, 4);
  Serial.print(',');
  Serial.print(
      densityLocked ? totalGrams / 1000.0f : 0.0f, 3);
  Serial.print(',');
  Serial.print(totalMilliLitres / 1000.0f, 3);
  Serial.print(',');
  Serial.println((int)phase);
}
//--------------------------------------

//--------------------------------------
void setup() {
  pinMode(pumpPin, OUTPUT);
  pinMode(bypassPin, OUTPUT);
  pinMode(mainlinePin, OUTPUT);

  digitalWrite(pumpPin, LOW);
  digitalWrite(bypassPin, LOW);
  digitalWrite(mainlinePin, LOW);

  pinMode(flowPin, INPUT_PULLUP);

  lcd.begin(16, 2);
  lcd.print(F("Starting sensor"));

  // D0/D1: commands from ESP and telemetry to ESP.
  Serial.begin(9600);
  Wire.begin();

  if (!sensor.begin()) {
    lcd.clear();
    lcd.print(F("Sensor error"));
    while (true) {}
  }

  key_board.Setup();

  lastFlowTime = millis();
  attachInterrupt(
      digitalPinToInterrupt(flowPin),
      pulseCounter,
      FALLING
  );

  lcd.clear();
}
//--------------------------------------

//--------------------------------------
void loop() {
  readEspCommand();
  handleCommand(key_board.GetTouchedKey());

  updateFlow();
  updateSequence();
  updateDensity();

  if (millis() - lastDisplayTime >= 250) {
    lastDisplayTime = millis();

    if (enteringTarget) {
      inputDisplay(enteredKg);
    } else if (targetkg == 0) {
      welcomeDisplay();
    } else {
      updateDisplay();
    }
  }

  sendTelemetry();
}
//--------------------------------------
