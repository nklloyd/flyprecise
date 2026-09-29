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
*/
VL53L0X sensor;
byte sensorPin = 2;
byte sensorInterrupt = digitalPinToInterrupt(sensorPin);


float calibrationFactor = 7.9;

unsigned long measurePeriod = 1000;

unsigned int pulseCount;

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

unsigned long oldTime;

const int mosfetPin9 = 9;


void setup() {
  // put your setup code here, to run once:
  lcd.begin(16, 2);
  lcd.print("Starting sensor");
  pinMode(mosfetPin9, OUTPUT);
  Serial.begin(115200);

  Wire.begin();

  // sensor.setTimeout(500);

  // if(!sensor.init()) {
  //   lcd.clear();
  //   lcd.print("sensor Error");
  //   while(true){
  //     //stop if sensor not detected
  //   }
  // }
  //sensor.startContinuous();

  delay(1000);
  lcd.clear();

  pinMode(sensorPin, INPUT_PULLUP);
  digitalWrite(sensorPin, HIGH);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;

  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}


void loop() {
  float pulsesPerLiter;
  float totalLitres;
  
  // const int samples = 32;
  // long total = 0;

  // for(int i = 0; i < samples; i++) {
  //   total += sensor.readRangeContinuousMillimeters();
  //   delay(10);
  // }
  // int averageDistance = total / samples;

  // lcd.setCursor(0,0);
  // lcd.print("Distance: ");

  //lcd.setCursor(0,1);

  // if(sensor.timeoutOccurred()) {
  //   lcd.print("Timeout");
  // } else {
  //   lcd.print(sensor.readRangeContinuousMillimeters()); //output raw reading
  //   lcd.print(" mm");
  // }

  lcd.setCursor(0,1);
  analogWrite(mosfetPin9, 255);

  if (millis() - oldTime >= 1000) {
    noInterrupts();
    
    //unsigned long pulses = pulseCount;
    //pulseCount = 0;
    interrupts();

    // unsigned long elapsed = millis() - oldTime;
    // oldTime = millis();

    // float litresThisInterval = pulses / pulsesPerLiter;
    // totalLitres += litresThisInterval;

    // float flowLitresPerMinute =
    //   litresThisInterval * (60000.0 / elapsed);

    //Serial.setCursor(0, 1);
    // Serial.print("Flow:");
    // Serial.print(flowLitresPerMinute, 2);
    // Serial.print(" L/m ");

    // //Serial.setCursor(0, 0);
    // Serial.print("Total:");
    // Serial.print(totalLitres, 2);
    // Serial.print(" L   ");

  }
  Serial.println();
  Serial.print("pulse count: ");
  Serial.print(pulseCount);
}

void pulseCounter() {
  pulseCount++;
}
