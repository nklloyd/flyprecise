
/*python read_esp32.py 192.168.0.75 8080*/

#include <WiFi.h>

const char* ssid = "SerioX";
const char* password = "testpass";

const int serverPort = 8080;

WiFiServer server(serverPort);

const int unoRxPin = 5; // Feather RX <- level shifter <- Uno A1
const int unoTxPin = 6; // Not connected yet
WiFiClient websiteClient;
char telemetryLine[96];
size_t telemetryLength = 0;

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, unoRxPin, unoTxPin);
  delay(2000); 
  Serial.println("\nESP32 WiFi Test with TCP Server");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi ");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");

  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.printf("TCP server started on port %d\n", serverPort);
  Serial.printf("Connect to: %s:%d\n", WiFi.localIP().toString().c_str(), serverPort);
}

void loop() {
  // Keep accepting a Python/backend client without blocking Uno readings.
  if (!websiteClient || !websiteClient.connected()) {
    if (websiteClient) websiteClient.stop();
    WiFiClient incoming = server.available();
    if (incoming) websiteClient = incoming;
  }

  while (Serial1.available()) {
    char c = Serial1.read();

    if (c == '\n') {
      telemetryLine[telemetryLength] = '\0';

      if (websiteClient && websiteClient.connected() &&
          strncmp(telemetryLine, "T,", 2) == 0) {
        websiteClient.println(telemetryLine);
      }

      telemetryLength = 0;
    } else if (c != '\r') {
      if (telemetryLength < sizeof(telemetryLine) - 1)
        telemetryLine[telemetryLength++] = c;
      else
        telemetryLength = 0;
    }
  }
}
