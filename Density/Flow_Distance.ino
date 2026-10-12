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

//pwm setup
const int mosfetPin6 = 6; //pump
const int mosfetPin9 = 9; //bypass
const int mosfetPin10 = 10; //mainline ouput
bool pumpState = false;
bool bypassState = false;
bool mainlineState = false;


void setup() {
  // put your setup code here, to run once:
  pinMode(mosfetPin6, OUTPUT);
  pinMode(mosfetPin9, OUTPUT);
  pinMode(mosfetPin10, OUTPUT);

  lcd.begin(16, 2);
  lcd.print("Starting sensor");
  Serial.begin(9600);

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
  //Serial.print("test");
  if (Serial.available() > 0) {
    char userInput = Serial.read(); 

    if (userInput == 'S') {
      pumpState = !pumpState; 
      bypassState = !bypassState; 
      analogWrite(mosfetPin6, pumpState ? 255 : 0);
      analogWrite(mosfetPin9, bypassState ? 255 : 0);
      Serial.println(pumpState ? "Turning on pump" : "Turning off pump");
      Serial.println(bypassState ? "Opening Bypass" : "Closing Bypass");
      delay(10000);
      bypassState = !bypassState; 
      analogWrite(mosfetPin9, bypassState ? 255 : 0);
      Serial.println(bypassState ? "Opening Bypass" : "Closing Bypass");
      delay(150);
      mainlineState = !mainlineState; 
      analogWrite(mosfetPin10, mainlineState ? 255 : 0);
      Serial.println(mainlineState ? "Opening Mainline" : "Closing Mainline");
    }

    if (userInput == 'T') {
      pumpState = !pumpState; 
      analogWrite(mosfetPin6, pumpState ? 255 : 0);
      Serial.println(pumpState ? "Turning on pump" : "Turning off pump");
    }
    
    // Toggle Bypass
    if (userInput == 'B') {
      bypassState = !bypassState; 
      analogWrite(mosfetPin9, bypassState ? 255 : 0);
      Serial.println(bypassState ? "Opening Bypass" : "Closing Bypass");
    }
    
    // Toggle Mainline
    if (userInput == 'M') {
      mainlineState = !mainlineState; 
      analogWrite(mosfetPin10, mainlineState ? 255 : 0);
      Serial.println(mainlineState ? "Opening Mainline" : "Closing Mainline");
    }
  //delay(1000);
  }

  for(int i = 0; i < samples; i++) {
    total += sensor.readRangeContinuousMillimeters();
    delay(10);
  }
  int averageDistance = total / samples;
  lcd.setCursor(0,0);
  lcd.print("                ");
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
  lcd.print(int(totalMilliLitres));

  pulseCount = 0;
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }

  

}

  void pulseCounter() {
    pulseCount++;
  }
