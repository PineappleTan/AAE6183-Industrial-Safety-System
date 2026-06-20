#include <Arduino.h>  // <-- Required for PlatformIO C++ compilation
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Safe, Neutral Hardware Pin Definitions (Avoids Flash & Boot Strap Conflicts)
#define DHTPIN 4
#define DHTTYPE DHT11       // Change to DHT22 if using the white sensor module
#define GREEN_LED 18
#define RED_LED 19
#define BUZZER 21
#define ALARM_BUTTON 16     // <-- Button connected to GPIO 16 (active low)
#define False_Alarm 17

// Network Configurations
const char* ssid = "你爹的网";          // <-- Your Wi-Fi Name
const char* password = "korone1001";        // <-- Your Wi-Fi Password
const char* mqtt_server = "broker.hivemq.com"; 

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastMsgTime = 0;
bool cameraEmergencyActive = false;

// Function Prototypes
void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);
void triggerAlarm();
void clearAlarm();
void reconnect();

void setup() {
  Serial.begin(115200);
  delay(2000); // Gives your laptop serial monitor time to synchronize safely
  
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(ALARM_BUTTON, INPUT); // Button with internal pull-up resistor
  pinMode(False_Alarm, INPUT); // False Alarm Button with internal pull-up resistor
  
  // Set default initial state (Assuming standard Active-Low buzzer modules)
  digitalWrite(GREEN_LED, HIGH); 
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, HIGH); // Pull High immediately to keep Active-Low buzzers silent on boot
  
  dht.begin();
  setup_wifi();
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected successfully!");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  if (String(topic) == "room/safety/status") {
    if (message == "SAFE") {
      cameraEmergencyActive = false;
      float currentTemp = dht.readTemperature();
      if (isnan(currentTemp) || currentTemp <= 40.0) {
        if (digitalRead(ALARM_BUTTON) == HIGH) { // Active Low Button Press Detected
        Serial.println("EMERGENCY BUTTON ACTIVATED!");
        client.publish("room/safety/status", "EMERGENCY_BUTTON");
        triggerAlarm();
        unsigned long waitStart = millis();
        while (millis() - waitStart < 5000) {
          Serial.println("EMERGENCY BUTTON ACTIVATED!");
          client.publish("room/safety/status", "EMERGENCY_BUTTON");
          if (digitalRead(False_Alarm) == HIGH) { // Active Low False Alarm Button Press Detected
            digitalWrite(ALARM_BUTTON, 0); // Reset the button state to avoid multiple triggers
            clearAlarm();
          }
        }
        clearAlarm();
        }
      }
    } else if (message == "FIRE_DETECTED") {
      cameraEmergencyActive = true;
      triggerAlarm();
    }
    else {
      clearAlarm();
    }
  }
}

void triggerAlarm() {
  digitalWrite(GREEN_LED, LOW);  
  digitalWrite(RED_LED, HIGH);   
  digitalWrite(BUZZER, LOW);    // Pull Low to trigger active-low sound alerts
}

void clearAlarm() {
  digitalWrite(GREEN_LED, HIGH); 
  digitalWrite(RED_LED, LOW);    
  digitalWrite(BUZZER, HIGH);   // Pull High to silence active-low sound alerts
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32_Safety_Node_" + String(random(1000, 9999));
    if (client.connect(clientId.c_str())) {
      Serial.println("connected!");
      client.subscribe("room/safety/status");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); 

  unsigned long now = millis();
  if (now - lastMsgTime > 2000) {
    lastMsgTime = now;

    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temp)) {
      Serial.println("Failed to read temperature from DHT sensor!");
      return;
    }

    if (isnan(humidity)) {
      Serial.println("Failed to read humidity from DHT sensor!");
    }

    Serial.print("Current Room Temperature: ");
    Serial.print(temp);
    Serial.println(" *C");

    Serial.print("Current Room Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    char tempStr[8];
    dtostrf(temp, 6, 2, tempStr);
    client.publish("room/telemetry/temperature", tempStr);
    char humidityStr[8];
    dtostrf(humidity, 6, 2, humidityStr);
    client.publish("room/telemetry/humidity", humidityStr);

    if (temp > 40.0) {
      Serial.println("CRITICAL HEAT DETECTED BY SENSOR!");
      client.publish("room/safety/status", "HIGH_TEMP");
      triggerAlarm();
    } else {
      if (!cameraEmergencyActive) {
        clearAlarm();
      }
    }
  }
}