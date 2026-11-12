#include <splash.h>
#include "matrix_keyboard.h"
#include <LiquidCrystal.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 8);
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
Adafruit_VL53L0X sensor;
MatrixKeyboard key_board(0x65);

const byte flowPin = 2;
const byte pumpPin = 6;
const byte bypassPin = 9;
const byte mainlinePin = 10;

const float TIP_TO_SENSOR_MM = 170.78f;
// Entries at the same position must correspond to each other.
const float tipDistanceMM[] = {
  53.89, 57.80, 61.57, 66.72, 72.69,
  78.89, 83.49, 87.10, 90.27, 94.96,
  98.31, 101.88, 106.24, 109.43, 112.62
};
// const float density[] = {
//   1.090, 1.084, 1.078, 1.070, 1.064,
//   1.058, 1.050, 1.044, 1.038, 1.030,
//   1.020, 1.010, 1.002, 0.996, 0.990
// };
const float density[] = {
  0.990, 0.996, 1.002, 1.010, 1.020,
  1.030, 1.038, 1.044, 1.050, 1.058,
  1.064, 1.070, 1.078, 1.084, 1.090
};
//const size_t STABLE_COUNT = 10;
const float MAX_DENSITY_SPREAD = 0.004f;
float recentDensities[3];
size_t readingCount = 0;
size_t nextIndex = 0;
bool densityLocked = false;
float lockedDensity = 0.0f;
const size_t POINT_COUNT = sizeof(tipDistanceMM) / sizeof(tipDistanceMM[0]);

const float calibrationFactor = 17.0f;
const unsigned long flowPeriodMs = 1000;
const unsigned long primeTimeMs = 10000;
const unsigned long overlapTimeMs = 150;

volatile unsigned long pulseCount = 0;

unsigned long lastFlowTime;
unsigned long lastDisplayTime = 0;
unsigned long totalMilliLitres = 0;
unsigned long totalGrams = 0;
float fractionalMilliLitres = 0;

float targetkg = 0.0; // Future input
unsigned long currentkg = 0.0;

bool pumpState = false;
bool bypassState = false;
bool mainlineState = false;

enum Phase { IDLE, PRIMING, OVERLAP, RUNNING };
Phase phase = IDLE;
unsigned long phaseStartedAt = 0;

uint16_t rawDistance = 0;
bool distanceTimedOut = false;

bool interpolateDensity(float distanceFromTip, float &result) {
  // Do not estimate beyond the calibrated range.
  if (distanceFromTip < tipDistanceMM[0] ||
      distanceFromTip > tipDistanceMM[POINT_COUNT - 1]) {
    return false;
  }

  for (size_t i = 0; i < POINT_COUNT - 1; i++) {
    if (distanceFromTip <= tipDistanceMM[i + 1]) {
      float fraction = (distanceFromTip - tipDistanceMM[i]) /
                       (tipDistanceMM[i + 1] - tipDistanceMM[i]);

      result = density[i] +
               fraction * (density[i + 1] - density[i]);
      return true;
    }
  }

  return false;
}

void pulseCounter() {
  pulseCount++;
}

void setPump(bool on) {
  pumpState = on;
  digitalWrite(pumpPin, on ? HIGH : LOW);
  //Serial.println(on ? "Turning on pump" : "Turning off pump");
}

void setBypass(bool open) {
  bypassState = open;
  digitalWrite(bypassPin, open ? HIGH : LOW);
  //Serial.println(open ? "Opening Bypass" : "Closing Bypass");
}

void setMainline(bool open) {
  mainlineState = open;
  digitalWrite(mainlinePin, open ? HIGH : LOW);
  //Serial.println(open ? "Opening Mainline" : "Closing Mainline");
}

void startSystem() {
  setBypass(true);
  setMainline(false);
  setPump(true);

  phase = PRIMING;
  phaseStartedAt = millis();
}

void stopSystem() {
  phase = IDLE;
  setPump(false);
  setMainline(false);
  setBypass(false);
}

// void handleCommand() {
//   if (Serial.available() == 0) return;

//   char command = Serial.read();

//   if (command == 'S') {
//     if (pumpState) stopSystem();
//   } else if (command == 'P') {
//     phase = IDLE;  // Manual command cancels the automatic sequence.
//     setPump(!pumpState);
//   } else if (command == 'B') {
//     phase = IDLE;
//     setBypass(!bypassState);
//   } else if (command == 'M') {
//     phase = IDLE;
//     setMainline(!mainlineState);
//   } else if (command == 'R') {
//     if (!pumpState) {
//       while(Serial.available() > 0) {
//         Serial.read(); // Flush input buffer
//       }
//       inputDisplay(targetkg);
//       //Serial.println("Enter target fuel in kg:");
      
//       // while(Serial.available() == 0) {}
//       // targetkg = (Serial.parseFloat()*1.0);

//       char key = "Z";
//       while (key != "#") {
//         key = key_board.GetTouchedKey();
//         if (key != '\0') {
//           Serial.print(key);
//         }
//       }

//       inputDisplay(targetkg);
//       //Serial.println(targetkg);
//       delay(3000);
//       startSystem();
//     }
//   }
// }

void handleCommand(char command) {

  if (command == 'B') { // Toggle Bilge Pump
    phase = IDLE; 
    setPump(!pumpState);
  } else if (command == 'D') { // Toggle Density line
    phase = IDLE;
    setBypass(!bypassState);
    Serial.println("density");
  } else if (command == 'C') { // Toggle clearway output
    phase = IDLE;
    setMainline(!mainlineState);
    Serial.println("clearway");
  } else if (command == 'A') {
    if (pumpState) stopSystem();
    if (!pumpState) {
      while(Serial.available() > 0) {
        Serial.read(); // Flush input buffer
      }
      inputDisplay(targetkg);

      char key = 'Z';
      while (key != '#') {
        key = key_board.GetTouchedKey();
        if (key == '0' || key == '1' || key == '2' || key == '3' || key == '4'
         || key == '5' || key == '6' || key == '7' || key == '8' || key == '9') {
          int kg_inc = key - '0';
          targetkg = (targetkg*10) + kg_inc;
          if (key != '\0') {
            Serial.print(key);
          }
          inputDisplay(targetkg);
          delay(100); // Delay to prevent double taps
        } else if (key == '*') {
          int kg_dec = (int)targetkg % 10;
          targetkg = (targetkg - kg_dec) / 10;
          inputDisplay(targetkg);
          delay(100); // Delay to prevent double taps
        }
      }
      Serial.println();
      Serial.print("Target KG = ");
      Serial.println(targetkg);

      inputDisplay(targetkg);
      //Serial.println(targetkg);
      delay(3000);
      startSystem();
    }
  } else {
    return;
  }
}

void updateSequence() {
  unsigned long now = millis();

  if (phase == PRIMING &&
      now - phaseStartedAt >= primeTimeMs) {
    setBypass(false);
    Serial.println("BYPASS OFF");
    setMainline(true);
    Serial.println("MAINLINE ON");
    while (!densityLocked) {
      updateDensity();
    }
    Serial.println("DONE DENSITY UPDATE");
    phase = OVERLAP;
    phaseStartedAt = now;
  } else if (phase == OVERLAP &&
             now - phaseStartedAt >= overlapTimeMs) {
    setBypass(false);
    Serial.println("BYPASS OFF");
    phase = RUNNING;
  }

  if((totalGrams >= ((targetkg + currentkg)*1000 - 3900)) && (targetkg > 0)) {
    stopSystem();
    Serial.print(totalGrams);
    Serial.println("STOP");
    currentkg = totalGrams;
  }
}

void updateDensity() {
  Serial.println("BEGAN DENSITY UPDATE");
  if (densityLocked) {
    return;  // Keep the locked value until the Arduino is reset.
  }

  VL53L0X_RangingMeasurementData_t measurement;
  sensor.rangingTest(&measurement, false);

  if (measurement.RangeStatus != 0) {
    delay(500);
    return;  // Skip an occasional invalid sensor reading.
    Serial.println("INVALID SENSOR READING: ");
    //Serial.println(measurement, 2);
  }

  float distanceFromTipMM =
      TIP_TO_SENSOR_MM - measurement.RangeMilliMeter;

  float currentDensity;
  if (!interpolateDensity(distanceFromTipMM, currentDensity)) {
    readingCount = 0;  // Start a fresh window after an out-of-range reading.
    nextIndex = 0;
    //Serial.println("OUT OF RANGE: ");
    Serial.println(distanceFromTipMM, 2);
    delay(500);
    return;
  }

  Serial.print("Current density: ");
  Serial.println(currentDensity, 4);

  recentDensities[nextIndex] = currentDensity;
  nextIndex = (nextIndex + 1) % 3;

  if (readingCount < 3) {
    readingCount++;
  }

  if (readingCount == 3) {
    float minimum = recentDensities[0];
    float maximum = recentDensities[0];
    float sum = 0.0f;

    for (size_t i = 0; i < 3; i++) {
      if (recentDensities[i] < minimum) minimum = recentDensities[i];
      if (recentDensities[i] > maximum) maximum = recentDensities[i];
      sum += recentDensities[i];
    }

    if (maximum - minimum <= MAX_DENSITY_SPREAD) {
      lockedDensity = sum / 3;
      densityLocked = true;

      Serial.print("LOCKED density: ");
      Serial.println(lockedDensity, 4);
    }
    lockedDensity = currentDensity; // REMOVE
  }

  delay(500);
}

void updateFlow() {
  unsigned long now = millis();
  unsigned long elapsedMs = now - lastFlowTime;
  if (elapsedMs < flowPeriodMs) return;

  // Read and reset the interrupt-updated counter together.
  noInterrupts();
  unsigned long pulses = pulseCount;
  pulseCount = 0;
  interrupts();

  lastFlowTime = now;

  float flowRateLpm =
      (1000.0f * pulses / elapsedMs) / calibrationFactor;

  // L/min × elapsed milliseconds / 60 = millilitres.
  float intervalMl =
      (flowRateLpm * elapsedMs / 60.0f) + fractionalMilliLitres;

  unsigned long wholeMl = (unsigned long)intervalMl;
  totalMilliLitres += wholeMl*100; // CHANGE LATER: Scaled up by 10 for demo purposes
  fractionalMilliLitres = intervalMl - wholeMl;
  //totalGrams = totalMilliLitres*1.02;
  totalGrams = totalMilliLitres*lockedDensity;
}

// void updateDistance() {
//   //Serial.println("START DISTANCE UPDATE");
//   rawDistance = sensor.readRangeContinuousMillimeters();
//   //Serial.println(rawDistance);
//   distanceTimedOut = sensor.timeoutOccurred();
//   if (distanceTimedOut) {
//     //Serial.println("***DISTANCE TIMEOUT OCCURRED***");
//   }
// }

void welcomeDisplay() {
  unsigned long now = millis();
  if (now - lastDisplayTime < 250) return;
  lastDisplayTime = now;

  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);

  lcd.print("SELECT OPTION:");

  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("A B C D");
}

void inputDisplay(float kg) {
  unsigned long now = millis();
  //if (now - lastDisplayTime < 250) return;
  lastDisplayTime = now;

  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);

  lcd.print("ENTER KG TO FUEL");

  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("kg: ");
  if(kg > 0) {
    lcd.print(kg*1.0, 0);
  } else {
    lcd.print("----");
  }
}

void updateDisplay() {
  unsigned long now = millis();
  if (now - lastDisplayTime < 250) return;
  lastDisplayTime = now;

  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);

  lcd.print("D: ");
  if(!bypassState && mainlineState || !pumpState) {
    lcd.print(lockedDensity, 3);
  } else {
    lcd.print("----");
  }
  lcd.print("  L: ");
  lcd.print(totalMilliLitres*0.001, 0);

  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("kg: ");
  if(!bypassState && mainlineState || !pumpState) {
    lcd.print(totalGrams*0.001, 0);
    lcd.print("/");
    lcd.print(targetkg, 0);
  } else {
    lcd.print("----");
    lcd.print("/");
    lcd.print(targetkg, 0);
  }
}

void setup() {
  pinMode(pumpPin, OUTPUT);
  pinMode(bypassPin, OUTPUT);
  pinMode(mainlinePin, OUTPUT);
  digitalWrite(pumpPin, LOW);
  digitalWrite(bypassPin, LOW);
  digitalWrite(mainlinePin, LOW);

  pinMode(flowPin, INPUT_PULLUP);

  lcd.begin(16, 2);
  lcd.print("Starting sensor");

  Serial.begin(9600);
  while (!Serial) {};
  Wire.begin();
  if (!sensor.begin()) {
    Serial.println("VL53L0X not found");
    while (true) {}
  }
  key_board.Setup();
  Serial.println("Keyboard setup done");

  // sensor.setTimeout(500);
  // if (!sensor.init()) {
  //   lcd.clear();
  //   lcd.print("Sensor error");
  //   while (true) {}
  // }

  // sensor.startContinuous();

  lastFlowTime = millis();
  attachInterrupt(digitalPinToInterrupt(flowPin), pulseCounter, FALLING);
  lcd.clear();
}

// void loop() {
//   const char key = key_board.GetTouchedKey();
 
//   if (key != '\0') {
//     Serial.println(key);
//   }
// }

void loop() {
  char key = key_board.GetTouchedKey();
  if (key != '\0') { Serial.println(key); }
  //Serial.println("LOOP BEGAN");
  handleCommand(key);
  //Serial.println("COMMAND UPDATED");
  updateSequence();
  //Serial.println("SEQUENCE UPDATED");
  updateFlow();
  //Serial.println("FLOW UPDATED");
  //Serial.println(targetkg);
  //updateDistance();
  //Serial.println("DISTANCE UPDATED");
  //Serial.println(targetkg);
  if(targetkg == 0) {
    welcomeDisplay();
  } else {
    updateDisplay();
  }
  //Serial.println("DISPLAY UPDATED");
}
