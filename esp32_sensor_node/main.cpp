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

// Network Configurations
const char* ssid = "165JAP5_2.4G";          // <-- Your Wi-Fi Name
const char* password = "309969san";        // <-- Your Wi-Fi Password
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
    if (message == "FIRE_DETECTED") {
      cameraEmergencyActive = true;
      triggerAlarm();
    } else if (message == "SAFE") {
      cameraEmergencyActive = false;
      float currentTemp = dht.readTemperature();
      if (isnan(currentTemp) || currentTemp <= 40.0) {
        clearAlarm();
      }
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

    if (isnan(temp)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    Serial.print("Current Room Temperature: ");
    Serial.print(temp);
    Serial.println(" *C");

    char tempStr[8];
    dtostrf(temp, 6, 2, tempStr);
    client.publish("room/telemetry/temperature", tempStr);

    if (temp > 40.0) {
      Serial.println("CRITICAL HEAT DETECTED BY SENSOR!");
      client.publish("room/safety/status", "HIGH_TEMP");
      triggerAlarm();
    } else {
      if (!cameraEmergencyActive) {
        clearAlarm();
        client.publish("room/safety/status", "SAFE");
      }
    }
  }
}