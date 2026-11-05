/*python read_esp32.py 192.168.0.75 8080*/

#include <WiFi.h>

const char* ssid = "SerioX";
const char* password = "testpass";

const int serverPort = 8080;

WiFiServer server(serverPort);

void setup() {
  Serial.begin(115200);
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
  WiFiClient client = server.available();

  if (client) {
    Serial.println("New client connected!");
    client.println("Hello! You are connected to ESP32 TCP server.");
    client.println("ESP32 is online. Sending status every 2 seconds...");
    client.println();

    while (client.connected()) {
      static unsigned long lastSend = 0;
      unsigned long now = millis();

      if (now - lastSend >= 2000) {
        lastSend = now;

        String status = "STATUS:";
        status += "IP:";
        status += WiFi.localIP().toString();
        status += "|";
        status += "RSSI:";
        status += String(WiFi.RSSI());
        status += "dBm";
        status += "|";
        status += "Connected:";
        status += WiFi.status() == WL_CONNECTED ? "YES" : "NO";

        client.println(status);
      }
    }
    Serial.println("Client disconnected");
    client.stop();
  }
}