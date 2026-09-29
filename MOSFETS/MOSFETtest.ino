//pwm setup
const int mosfetPin9 = 9;
const int mosfetPin10 = 10;

void setup() {
  // put your setup code here, to run once:
  pinMode(mosfetPin9, OUTPUT);
  pinMode(mosfetPin10, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  analogWrite(mosfetPin9, 255);
  analogWrite(mosfetPin10, 255);
  delay(10000);
  analogWrite(mosfetPin9, 0);
  analogWrite(mosfetPin10, 0);
  delay(10000);
}
