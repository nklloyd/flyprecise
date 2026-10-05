#include <LiquidCrystal.h>
#include <Wire.h>
#include <VL53L0X.h>

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
VL53L0X sensor;

const byte flowPin = 2;
const byte pumpPin = 6;
const byte bypassPin = 9;
const byte mainlinePin = 10;

const float calibrationFactor = 7.9f;
const unsigned long flowPeriodMs = 1000;
const unsigned long primeTimeMs = 10000;
const unsigned long overlapTimeMs = 150;

volatile unsigned long pulseCount = 0;

unsigned long lastFlowTime;
unsigned long lastDisplayTime = 0;
unsigned long totalMilliLitres = 0;
float fractionalMilliLitres = 0;

bool pumpState = false;
bool bypassState = false;
bool mainlineState = false;

enum Phase { IDLE, PRIMING, OVERLAP, RUNNING };
Phase phase = IDLE;
unsigned long phaseStartedAt = 0;

uint16_t rawDistance = 0;
bool distanceTimedOut = false;

void pulseCounter() {
  pulseCount++;
}

void setPump(bool on) {
  pumpState = on;
  digitalWrite(pumpPin, on ? HIGH : LOW);
  Serial.println(on ? "Turning on pump" : "Turning off pump");
}

void setBypass(bool open) {
  bypassState = open;
  digitalWrite(bypassPin, open ? HIGH : LOW);
  Serial.println(open ? "Opening Bypass" : "Closing Bypass");
}

void setMainline(bool open) {
  mainlineState = open;
  digitalWrite(mainlinePin, open ? HIGH : LOW);
  Serial.println(open ? "Opening Mainline" : "Closing Mainline");
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

void handleCommand() {
  if (Serial.available() == 0) return;

  char command = Serial.read();

  if (command == 'S') {
    if (pumpState) stopSystem();
    else startSystem();
  } else if (command == 'P') {
    phase = IDLE;  // Manual command cancels the automatic sequence.
    setPump(!pumpState);
  } else if (command == 'B') {
    phase = IDLE;
    setBypass(!bypassState);
  } else if (command == 'M') {
    phase = IDLE;
    setMainline(!mainlineState);
  }
}

void updateSequence() {
  unsigned long now = millis();

  if (phase == PRIMING &&
      now - phaseStartedAt >= primeTimeMs) {
    setMainline(true);
    phase = OVERLAP;
    phaseStartedAt = now;
  } else if (phase == OVERLAP &&
             now - phaseStartedAt >= overlapTimeMs) {
    setBypass(false);
    phase = RUNNING;
  }
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
  totalMilliLitres += wholeMl;
  fractionalMilliLitres = intervalMl - wholeMl;
}

void updateDistance() {
  rawDistance = sensor.readRangeContinuousMillimeters();
  distanceTimedOut = sensor.timeoutOccurred();
}

// void updateDisplay() {
//   unsigned long now = millis();
//   if (now - lastDisplayTime < 250) return;
//   lastDisplayTime = now;

//   lcd.setCursor(0, 0);
//   lcd.print("                ");
//   lcd.setCursor(0, 0);

//   if (distanceTimedOut) {
//     lcd.print("Dist: Timeout");
//   } else {
//     lcd.print("Dist: ");
//     lcd.print(rawDistance);
//     lcd.print(" mm");
//   }

//   lcd.setCursor(0, 1);
//   lcd.print("                ");
//   lcd.setCursor(0, 1);
//   lcd.print("mL: ");
//   lcd.print(totalMilliLitres);
// }

void updateDisplay() {
  unsigned long now = millis();
  if (now - lastDisplayTime < 250) return;
  lastDisplayTime = now;

  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);

  lcd.print("D: ");
  if(!bypassState && mainlineState) {
    lcd.print(1.02);
  } else {
    lcd.print("----");
  }
  lcd.print("  L: ");
  lcd.print(totalMilliLitres*0.001, 2);

  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("kg: ");
  if(!bypassState && mainlineState) {
    lcd.print(totalMilliLitres*0.00102, 2);
  } else {
    lcd.print("----");
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
  Wire.begin();

  sensor.setTimeout(500);
  if (!sensor.init()) {
    lcd.clear();
    lcd.print("Sensor error");
    while (true) {}
  }

  sensor.startContinuous();

  lastFlowTime = millis();
  attachInterrupt(digitalPinToInterrupt(flowPin), pulseCounter, FALLING);
  lcd.clear();
}

void loop() {
  handleCommand();
  updateSequence();
  updateFlow();
  updateDistance();
  updateDisplay();
}
