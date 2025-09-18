#include <Arduino.h>
#include <WebServer.h>
#include <FastLED.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "styles.h"
#include "website_html.h"

//-------------------------define--------------------------------
#define red_light 2

// ---------------------- LED STRIP CONFIG ----------------------
#define DATA_PIN 32
#define NUM_LEDS 4
#define COLOR_ORDER GRB
#define CHIPSET WS2812B
#define BRIGHTNESS 60
#define VOLTS 5
#define MAX_AMPS 500
CRGB leds[NUM_LEDS];

// -------------------- CONFIG --------------------
#define NUM_SENSORS 2

// --- Motor pins ---
#define MOTOR_IN1 19
#define MOTOR_IN2 18
#define MOTOR_ENA 21
#define MOTOR_IN3 5
#define MOTOR_IN4 17
#define MOTOR_ENB 16

// --- Ultrasonic pins ---
const int trigPins[NUM_SENSORS] = {27, 25};
const int echoPins[NUM_SENSORS] = {26, 33}; // Avoid GPIO0 for echo/boot pin
long sonarDist[NUM_SENSORS] = {0, 0};

// --- Boat direction (NEW) ---
enum BoatDir { NONE, FROM_LEFT, FROM_RIGHT };
BoatDir boatDirection = NONE;
bool boatCleared = false;

// --- Traffic light / timing ---
const unsigned long OPEN_TIME_MS = 3000;
bool openingStarted = false;
unsigned long openingStart = 0;
unsigned long prev_time = 0;
int speed = 255;

// -------------------- WiFi & Server --------------------
const char *ssid = "G54_ESP32";
const char *password = "12345678";
WebServer server(80);

// ---------------------- STATE MACHINE ----------------------
enum state { CLOSE, OPENING, OPEN, CLOSING };
state current_state = CLOSE;

enum sub_state { GREEN, YELLOW, RED, HAZARDS };

// ---------------------- TRAFFIC LIGHTS ----------------------
void setTrafficLights(sub_state colorState) {
  switch (colorState) {
    case GREEN:
      // Cars green, boats red
      leds[0] = CRGB::Green;
      leds[1] = CRGB::Green;
      leds[2] = CRGB::Red;
      leds[3] = CRGB::Red;
      break;

    case YELLOW:
      // Transition state (cars yellow, boats red)
      leds[0] = CRGB::Orange;
      leds[1] = CRGB::Orange;
      leds[2] = CRGB::Red;
      leds[3] = CRGB::Red;
      break;

    case RED:
      // Cars red, boats green (bridge up)
      leds[0] = CRGB::Red;
      leds[1] = CRGB::Red;
      leds[2] = CRGB::Green;
      leds[3] = CRGB::Green;
      break;

    default:
      break;
  }
  FastLED.show();
}

// -------------------- MOTOR FUNCTIONS --------------------
void motorUp() {
  digitalWrite(MOTOR_IN1, HIGH); digitalWrite(MOTOR_IN2, LOW);
  analogWrite(MOTOR_ENA, speed);
  digitalWrite(MOTOR_IN3, HIGH); digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENB, speed);
}

void motorDown() {
  digitalWrite(MOTOR_IN1, LOW);  digitalWrite(MOTOR_IN2, HIGH);
  analogWrite(MOTOR_ENA, speed);
  digitalWrite(MOTOR_IN3, LOW);  digitalWrite(MOTOR_IN4, HIGH);
  analogWrite(MOTOR_ENB, speed);
}

void motorStop() {
  digitalWrite(MOTOR_IN1, LOW); digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW); digitalWrite(MOTOR_IN4, LOW);
}

// -------------------- Blocking ultrasonic read --------------------
long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}

void readSingleSensor(int sensorIndex) {
  digitalWrite(trigPins[sensorIndex], LOW);
  delayMicroseconds(2);
  digitalWrite(trigPins[sensorIndex], HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPins[sensorIndex], LOW);

  long duration = pulseIn(echoPins[sensorIndex], HIGH, 30000); // 30 ms timeout
  sonarDist[sensorIndex] = (duration > 0) ? microsecondsToCentimeters(duration) : 999;
}

// ---------------------- SERVER HANDLERS ----------------------
void handleRoot() { server.send(200, "text/html", MAIN_page); }
void handleCSS()  { server.send(200, "text/css", STYLE_CSS); }

// ---------------------- SETUP SERVER ----------------------
void setupServer() {
  server.on("/", handleRoot);
  server.on("/styles.css", handleCSS);

  server.on("/led/on", []() {
    digitalWrite(red_light, HIGH);
    for (int i = 0; i < 3; i++) leds[i] = CRGB::Red;
    FastLED.show();
    Serial.println("LEDS ON!");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "light's On");
  });

  server.on("/led/off", []() {
    digitalWrite(red_light, LOW);
    for (int i = 0; i < 3; i++) leds[i] = CRGB::Green;
    FastLED.show();
    Serial.println("LEDS OFF!");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "light's off");
  });

  server.on("/motor/start", []() {
    motorUp();
    Serial.println("Motors On");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "Motor On");
  });

  server.on("/motor/stop", []() {
    motorStop();
    Serial.println("Motors Off");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "Motor Off");
  });

  // Ultrasonic REST endpoints
  server.on("/ultrasonic/1", []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json",
                String("{\"distance_cm\":") + sonarDist[0] + "}");
  });
  server.on("/ultrasonic/2", []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json",
                String("{\"distance_cm\":") + sonarDist[1] + "}");
  });

  server.begin();
  Serial.println("HTTP server started");
}

// ---------------------- SETUP ----------------------
void setup() {
  Serial.begin(115200);

  // Start ESP32 in Access Point mode
  Serial.println("Starting Access Point...");
  WiFi.softAP(ssid, password);
  Serial.print("AP SSID: "); 
  Serial.println(ssid);
  Serial.print("AP IP address: "); 
  Serial.println(WiFi.softAPIP());

  pinMode(red_light, OUTPUT); //set discrete LED pin

  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    digitalWrite(trigPins[i], LOW);
  }

  ArduinoOTA.setHostname("esp32-ota");
  ArduinoOTA
    .onStart([](){ Serial.println("OTA Starting..."); })
    .onEnd([](){ Serial.println("\nOTA End"); })
    .onProgress([](unsigned int progress, unsigned int total){
      Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
    .onError([](ota_error_t error){ Serial.printf("Error[%u]\n", error); });
  ArduinoOTA.begin();
  Serial.println("OTA ready");

  setupServer(); //separation of server setup

  pinMode(MOTOR_IN1, OUTPUT); pinMode(MOTOR_IN2, OUTPUT); pinMode(MOTOR_ENA, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT); pinMode(MOTOR_IN4, OUTPUT); pinMode(MOTOR_ENB, OUTPUT);

  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_AMPS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
}

// ---------------------- LOOP ----------------------
void loop() {
  // Handle web server and OTA updates every loop
  server.handleClient();
  ArduinoOTA.handle();

  // --- Ultrasonic reads ---
  readSingleSensor(0);   // Sensor 1 (left / detect)
  readSingleSensor(1);   // Sensor 2 (right / clear)
  long d0 = sonarDist[0];
  long d1 = sonarDist[1];

  // -------- Main bridge state machine --------
  switch (current_state) {

    case CLOSE:
      Serial.println("STATE: CLOSE (Bridge is down)");
      setTrafficLights(GREEN);  // Cars green, boats red

      // Open for a boat from either side
      if (d0 < 50) {
        boatDirection = FROM_LEFT;   // entering from Sensor 1 side
        current_state = OPENING;
        Serial.println("--> Boat detected from LEFT. Changing to OPENING.");
      } else if (d1 < 50) {
        boatDirection = FROM_RIGHT;  // entering from Sensor 2 side
        current_state = OPENING;
        Serial.println("--> Boat detected from RIGHT. Changing to OPENING.");
      }
      break;

    case OPENING:
      setTrafficLights(YELLOW); // Transition: cars yellow, boats red
      if (!openingStarted) {
        openingStarted = true;
        openingStart = millis();
        motorUp();                        // Start opening the bridge
        Serial.println("STATE: OPENING (Bridge is going up)");
      }
      if (millis() - openingStart >= OPEN_TIME_MS) {
        motorStop();
        openingStarted = false;
        current_state = OPEN;
        Serial.println("--> Bridge is now open.");
      }
      break;

    case OPEN:
      setTrafficLights(RED);  // Cars red, boats green (bridge up)
      Serial.println("STATE: OPEN (Bridge is up)");

      // Wait for boat to clear the opposite side depending on entry direction
      if (boatDirection == FROM_LEFT) {
        boatCleared = (d1 >= 50); // clear on Sensor 2
      } else if (boatDirection == FROM_RIGHT) {
        boatCleared = (d0 >= 50); // clear on Sensor 1
      } else {
        // Safety: if direction unknown, require both sides clear
        boatCleared = (d0 >= 50) && (d1 >= 50);
      }

      if (boatCleared) {
        current_state = CLOSING;
        Serial.println("--> Boat has cleared. Changing to CLOSING.");
      }
      break;

    case CLOSING:
      setTrafficLights(YELLOW); // Transition: cars yellow, boats red
      Serial.println("STATE: CLOSING (Bridge is going down)");
      motorDown();
      delay(3000);              // Simulate close time (blocking)
      motorStop();
      current_state = CLOSE;
      boatDirection = NONE;     // reset direction for next boat
      Serial.println("--> Bridge is now closed.");
      break;
  }

  // -------- Debugging output --------
  Serial.print("Sensor 1 (Detect/Left): ");
  Serial.print(d0);
  Serial.print(" cm | Sensor 2 (Clear/Right): ");
  Serial.print(d1);
  Serial.print(" cm | Direction: ");
  switch (boatDirection) {
    case NONE:       Serial.println("NONE"); break;
    case FROM_LEFT:  Serial.println("FROM_LEFT"); break;
    case FROM_RIGHT: Serial.println("FROM_RIGHT"); break;
  }

  Serial.println("Client disconnected.");
  Serial.println("------------------------------------");

  delay(1000);  // 1 second loop interval (note: blocks server/OTA briefly)
}
