
#include <WiFi.h>

const char* ssid = "SerioX";
const char* password = "testpass";

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\nESP32-S3 WiFi Test");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n\nWiFi Connected!");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  Serial.print("Signal Strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connection active!");
  } else {
    Serial.println("WiFi disconnected!");
  }

  delay(5000);
}
