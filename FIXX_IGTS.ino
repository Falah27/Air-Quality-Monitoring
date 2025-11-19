#include <EEPROM.h>
#include <DHT.h> 
#include <FirebaseESP8266.h> 
#include <ESP8266WiFi.h> 
#include <WiFiManager.h>

#define DHTPIN D4 
#define DHTTYPE DHT11 
#define MQ_PIN A0 
#define BUZZER D0 
#define LED_GREEN D6 
#define LED_RED D5 

String path = "Alat4";
char ssid[32]; 
char password[32]; 

#define FIREBASE_HOST "https://lustrum-x-iot-default-rtdb.firebaseio.com/" 
#define FIREBASE_AUTH "AIzaSyDh07T-mREGb3WFNXHtwR8y3C53_YyLOdc" 

DHT dht(DHTPIN, DHTTYPE); 
FirebaseData firebaseData; 
WiFiManager wifiManager;

void saveCredentialsToEEPROM(const char* ssid, const char* password) {
  EEPROM.begin(512);
  EEPROM.put(0, ssid);
  EEPROM.put(strlen(ssid) + 1, password);
  EEPROM.commit();
  EEPROM.end();
}

void readCredentialsFromEEPROM(char* ssid, char* password) {
  EEPROM.begin(512);
  EEPROM.get(0, ssid);
  EEPROM.get(strlen(ssid) + 1, password);
  EEPROM.end();
}

void setup() {
  Serial.begin(115200); 
  dht.begin(); 

  pinMode(BUZZER, OUTPUT); 
  pinMode(LED_GREEN, OUTPUT); 
  pinMode(LED_RED, OUTPUT); 

  // Read WiFi credentials from EEPROM
  readCredentialsFromEEPROM(ssid, password);

  WiFi.begin(ssid, password); 
  Serial.print("Connecting to WiFi"); 
  while (WiFi.status() != WL_CONNECTED) { 
    if (!wifiManager.autoConnect("Alat4_config")) {
      Serial.println("Failed to connect and hit timeout");
      ESP.reset();
      delay(1000);
    }
    Serial.println("Connected to WiFi");

    // Save WiFi credentials to EEPROM
    saveCredentialsToEEPROM(ssid, password);
  }
  Serial.println("Connected to WiFi");

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH); 
  Firebase.reconnectWiFi(true); 
  Serial.println("Terhubung Firebase"); 
}

void loop() {
  while(WiFi.status()== WL_CONNECTED){
    float humidity = dht.readHumidity(); 
    float temperature = dht.readTemperature(); 
  
    if (isnan(humidity) || isnan(temperature)) { 
      Serial.println("Error membaca DHT11"); 
    } else {
      Serial.print("Suhu: "); 
      Serial.print(temperature);
      Serial.println(" °C"); 
      Serial.print("Kelembaban: ");
      Serial.print(humidity);
      Serial.println(" %");
    }
  
    int mqValue = analogRead(MQ_PIN);
    Serial.print("Nilai MQ135: ");
    Serial.println(mqValue); 
  
    // Mengirim data ke Firebase
    Firebase.setFloat(firebaseData, path + "/temperature", temperature);
    Firebase.setFloat(firebaseData, path + "/humidity", humidity);
    Firebase.setFloat(firebaseData, path + "/mq135", mqValue);
  
    if (mqValue > 300) {
      digitalWrite(BUZZER, HIGH);
    } else {
      digitalWrite(BUZZER, LOW);
    }
  
    if (temperature < 30.0) {
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW);
    } else {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, HIGH);
    }
    delay(2000);
  }

  // If WiFi connection is lost, attempt to reconnect
  Serial.println("WiFi disconnected. Reconnecting...");
  WiFi.begin(ssid, password);
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    attempt+  +;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nReconnected to WiFi");
  } else {
    Serial.println("\nFailed to reconnect. Resetting...");
    ESP.reset();
  }
}
