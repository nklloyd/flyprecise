int FSR;
//mass = (known mass / known reading) * measured Voltage
//int analogRead = analogRead(A0);
float zeroMass = 49.8;
float zeroReading = 778;
float knownMass = 110.6;
float knownReading = 259;
float slope;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  slope = (knownMass - zeroMass) / (knownReading - zeroReading);
}

void loop() {
  // put your main code here, to run repeatedly:
  int reading = analogRead(A0);
  float mass = (reading) * slope;
    //if (mass < 0) mass = 0;

  Serial.print("Raw reading: ");
  Serial.println(reading);
  Serial.print("  Mass: ");
  Serial.print(mass);
  Serial.println(" g");

  delay(200);
}
