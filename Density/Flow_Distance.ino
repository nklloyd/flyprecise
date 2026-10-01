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

volatile byte pulseCount;

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

unsigned long oldTime;


void setup() {
  // put your setup code here, to run once:
  lcd.begin(16, 2);
  lcd.print("Starting sensor");

  Wire.begin();

  sensor.setTimeout(500);

  if(!sensor.init()) {
    lcd.clear();
    lcd.print("sensor Error");
    while(true){
      //stop if sensor not detected
    }
  }
  sensor.startContinuous();

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
  const int samples = 32;
  long total = 0;

  for(int i = 0; i < samples; i++) {
    total += sensor.readRangeContinuousMillimeters();
    delay(10);
  }
  int averageDistance = total / samples;

  lcd.setCursor(0,0);
  lcd.print("Distance: ");

  //lcd.setCursor(0,1);

  if(sensor.timeoutOccurred()) {
    lcd.print("Timeout");
  } else {
    lcd.print(sensor.readRangeContinuousMillimeters()); //output raw reading
    lcd.print(" mm");
  }

  lcd.setCursor(0,1);

  if((millis() - oldTime) > 1000) {
  detachInterrupt(sensorInterrupt);
  flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
  oldTime = millis();
  flowMilliLitres = (flowRate / 60) * 1000;
  totalMilliLitres += flowMilliLitres;
  unsigned int frac;

  lcd.print("Milliliters: ") ;
  lcd.print(int(totalMilliliters));

  pulseCount = 0;
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  //delay(1000);
  }
}

void pulseCounter() {
  pulseCount++;
}

