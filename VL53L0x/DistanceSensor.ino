#include <LiquidCrystal.h>
#include <Wire.h>
#include <VL53L0X.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
VL53L0X sensor;


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
}


void loop() {
  const int samples = 32; //edit # of samples for greater precision
  long total = 0;

  for(int i = 0; i < samples; i++) {
    total += sensor.readRangeContinuousMillimeters();  //Can use other units of measurement
    delay(10);
  }
  int averageDistance = total / samples;

  lcd.setCursor(0,0);
  lcd.print("Distance:       ");

  lcd.setCursor(0,1);

  if(sensor.timeoutOccurred()) {
    lcd.print("Timeout       ");
  } else {
    lcd.print(averageDistance);
    lcd.print(" mm       ");
  }
  //delay(1000);
}

