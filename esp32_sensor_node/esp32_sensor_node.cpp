#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN 23
#define DHTTYPE DHT11 // Change to DHT22 if needed
#define GREEN_LED 17
#define RED_LED 16
#define BUZZER 5

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "broker.hivemq.com"; // Free public broker

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  digitalWrite(GREEN_LED, HIGH); // Default Safe State
  digitalWrite(RED_LED, LOW);
  
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) { message += (char)payload[i]; }
  
  // If Edge AI Host detects fire, trigger hardware alarm immediately
  if (message == "FIRE_DETECTED" || message == "HIGH_TEMP") {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER, HIGH);
  } else if (message == "SAFE") {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32_Sensor_Node")) {
      client.subscribe("room/safety/status");
    } else { delay(5000); }
  }
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  float temp = dht.readTemperature();
  if (!isnan(temp)) {
    char tempStr[8];
    dtostrf(temp, 6, 2, tempStr);
    client.publish("room/telemetry/temperature", tempStr);

    // Local threshold check
    if (temp > 40.0) { // Dangerous level threshold
      client.publish("room/safety/status", "HIGH_TEMP");
    }
  }
  delay(2000); // Send data every 2 seconds
}