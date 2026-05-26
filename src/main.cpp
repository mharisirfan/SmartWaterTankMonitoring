#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Servo.h>

// WiFi
#define STASSID "IOTteamAlpha"
#define STAPSK  "iotempire"

const char* ssid = STASSID;
const char* password = STAPSK;
const char* hostname = "water-tank-d1mini";

// MQTT
const char* mqtt_server = "192.168.15.2";
const int mqtt_port = 1883;
const char* mqtt_topic = "water_tank/status";

WiFiClient espClient;
PubSubClient client(espClient);

// Pins
#define BUZZER_PIN D1
#define WATER_SENSOR_PIN D2
#define SERVO_PIN D4

// Ultrasonic wiring
#define TRIG_PIN D6
#define ECHO_PIN D7

Servo tankServo;

// Tank calibration
const float SENSOR_TO_BOTTOM_CM = 15.13;
const float FULL_WATER_HEIGHT_CM = 11.5;
const float MOTOR_START_HEIGHT_CM = 1.5;

// Robustness settings
const int REQUIRED_STABLE_READINGS = 3;
const unsigned long RED_SENSOR_CONFIRM_MS = 1000;
const unsigned long PUBLISH_INTERVAL = 2000;

bool motorOn = true;
int highCount = 0;
int lowCount = 0;

unsigned long redStartTime = 0;
bool redConfirmed = false;

unsigned long lastPublishTime = 0;

void setupWiFi();
void setupOTA();
void reconnectMQTT();

float getDistanceCM();
float getAverageDistanceCM();

void buzzerAlert();
void motorOnPosition();
void motorOffPosition();

void publishTankStatus(float distance, float waterHeight, int level, String status, bool redConfirmed);

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("Booting Smart Water Tank System");

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(WATER_SENSOR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(BUZZER_PIN, LOW);

  tankServo.attach(SERVO_PIN);
  motorOnPosition();

  setupWiFi();
  setupOTA();

  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Setup complete");
}

void loop() {
  ArduinoOTA.handle();

  if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    reconnectMQTT();
  }

  client.loop();

  unsigned long now = millis();

  if (now - lastPublishTime >= PUBLISH_INTERVAL) {
    lastPublishTime = now;

    float distance = getAverageDistanceCM();

    float waterHeight = SENSOR_TO_BOTTOM_CM - distance;

    if (waterHeight < 0) waterHeight = 0;
    if (waterHeight > FULL_WATER_HEIGHT_CM) waterHeight = FULL_WATER_HEIGHT_CM;

    int waterLevel = (waterHeight / FULL_WATER_HEIGHT_CM) * 100;

    bool redRaw = digitalRead(WATER_SENSOR_PIN) == HIGH;

    if (redRaw) {
      if (redStartTime == 0) {
        redStartTime = millis();
      }

      if (millis() - redStartTime >= RED_SENSOR_CONFIRM_MS) {
        redConfirmed = true;
      }
    } else {
      redStartTime = 0;
      redConfirmed = false;
    }

    String status = "NORMAL";

    if (redConfirmed) {
      status = "FULL_RED_SENSOR_CONFIRMED";
      buzzerAlert();
      motorOffPosition();
      motorOn = false;
      highCount = 0;
      lowCount = 0;
    } 
    else {
      if (waterHeight >= FULL_WATER_HEIGHT_CM) {
        highCount++;
      } else {
        highCount = 0;
      }

      if (waterHeight <= MOTOR_START_HEIGHT_CM) {
        lowCount++;
      } else {
        lowCount = 0;
      }

      if (motorOn && highCount >= REQUIRED_STABLE_READINGS) {
        status = "TARGET_FULL_STABLE";
        motorOffPosition();
        motorOn = false;
        highCount = 0;
      } 
      else if (!motorOn && lowCount >= REQUIRED_STABLE_READINGS) {
        status = "LOW_STABLE";
        motorOnPosition();
        motorOn = true;
        lowCount = 0;
      }
    }

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.print(" cm | Water height: ");
    Serial.print(waterHeight);
    Serial.print(" cm | Level: ");
    Serial.print(waterLevel);
    Serial.print("% | Red raw: ");
    Serial.print(redRaw ? "YES" : "NO");
    Serial.print(" | Red confirmed: ");
    Serial.print(redConfirmed ? "YES" : "NO");
    Serial.print(" | Status: ");
    Serial.print(status);
    Serial.print(" | Motor: ");
    Serial.println(motorOn ? "ON" : "OFF");

    publishTankStatus(distance, waterHeight, waterLevel, status, redConfirmed);
  }
}

float getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return SENSOR_TO_BOTTOM_CM;
  }

  float distance = duration * 0.0343 / 2;

  if (distance <= 0 || distance > 400) {
    return SENSOR_TO_BOTTOM_CM;
  }

  return distance;
}

float getAverageDistanceCM() {
  float sum = 0;
  int validCount = 0;

  for (int i = 0; i < 5; i++) {
    float d = getDistanceCM();

    if (d > 2 && d < SENSOR_TO_BOTTOM_CM + 5) {
      sum += d;
      validCount++;
    }

    delay(40);
  }

  if (validCount == 0) {
    return SENSOR_TO_BOTTOM_CM;
  }

  return sum / validCount;
}

void buzzerAlert() {
  tone(BUZZER_PIN, 2000);
  delay(700);
  noTone(BUZZER_PIN);
}

void motorOnPosition() {
  tankServo.write(0);
}

void motorOffPosition() {
  tankServo.write(90);
}

void setupWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.hostname(hostname);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setupOTA() {
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword("iotempower");

  ArduinoOTA.onStart([]() {
    Serial.println("OTA update started");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA update complete");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", progress / (total / 100));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]\n", error);
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void reconnectMQTT() {
  static unsigned long lastAttempt = 0;

  if (millis() - lastAttempt < 5000) {
    return;
  }

  lastAttempt = millis();

  Serial.print("Connecting to MQTT... ");

  String clientId = "WaterTankD1Mini-";
  clientId += String(random(0xffff), HEX);

  if (client.connect(clientId.c_str())) {
    Serial.println("connected");
  } else {
    Serial.print("failed, rc=");
    Serial.println(client.state());
  }
}

void publishTankStatus(float distance, float waterHeight, int level, String status, bool redConfirmed) {
  if (!client.connected()) {
    return;
  }

  String payload = "{";
  payload += "\"distance_cm\":" + String(distance, 2) + ",";
  payload += "\"water_height_cm\":" + String(waterHeight, 2) + ",";
  payload += "\"level\":" + String(level) + ",";
  payload += "\"status\":\"" + status + "\",";
  payload += "\"red_sensor_confirmed\":\"" + String(redConfirmed ? "WATER_DETECTED" : "NO_WATER") + "\",";
  payload += "\"motor\":\"" + String(motorOn ? "ON" : "OFF") + "\"";
  payload += "}";

  client.publish(mqtt_topic, payload.c_str());
}