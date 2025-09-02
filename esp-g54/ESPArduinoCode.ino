#include <WiFi.h>
#include <WebServer.h>

// -------- WiFi ----------
const char* ssid = "Telstra3B0C8F";
const char* password = "njte7r7fys";

// -------- Motor pins --------
#define MOTOR_IN1 19
#define MOTOR_IN2 18
#define MOTOR_ENA 21

WebServer server(80); // HTTP server

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return; // already connected

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    // Reset ESP if unable to connect in 15 seconds
    if (millis() - startAttemptTime > 15000) {
      Serial.println("\nFailed to connect, restarting...");
      ESP.restart();
    }
  }

  Serial.println("\nConnected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);

  // -------- Motor setup --------
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);

  // Motor off initially
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_ENA, LOW);

  // Connect to WiFi
  connectWiFi();

  // -------- Web server endpoints --------
  server.on("/motor/start", []() {
    digitalWrite(MOTOR_IN1, HIGH);
    digitalWrite(MOTOR_IN2, LOW);
    digitalWrite(MOTOR_ENA, HIGH);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "Motor started");
  });

  server.on("/motor/stop", []() {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
    digitalWrite(MOTOR_ENA, LOW);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "Motor stopped");
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Ensure WiFi stays connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost! Reconnecting...");
    connectWiFi();
  }

  server.handleClient();
}
