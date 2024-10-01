#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "DHT.h"

#define DHTTYPE DHT11   // DHT 11
const int DHTPin = 27;

const int PIR_SENSOR_PIN = 13;  // กำหนดพินสำหรับ PIR Sensor
const int LED_PIN = 14;         // กำหนดพินสำหรับ LED

String t;
#define ON_Board_LED 26
#define SOIL_MOISTURE_PIN 34

const char* ssid = "อ่านหาพ่อง";
const char* password = "Metallica1215";

const char* host = "script.google.com";
const int httpsPort = 443;

DHT dht(DHTPin, DHTTYPE);
WiFiClientSecure client;

long now = millis();
long lastMeasure = 0;

String GAS_ID = "AKfycbyZ07tqrV4NPrpriELjdQE97_dFO6BL8VL0mu3N4KeLPgF_cKu9vArFzSiiswfCa0bZbw"; // apps script ID or deployment ID

void setup() {
  Serial.begin(115200);
  delay(500);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.println("");

  pinMode(ON_Board_LED, OUTPUT);
  pinMode(ON_Board_LED, INPUT);
  pinMode(PIR_SENSOR_PIN, INPUT);  // ตั้งค่า PIR Sensor เป็น input
  pinMode(LED_PIN, OUTPUT);        // ตั้งค่า LED เป็น output
  digitalWrite(ON_Board_LED, HIGH);

  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(ON_Board_LED, LOW);
    delay(250);
    digitalWrite(ON_Board_LED, HIGH);
    delay(250);
  }
  digitalWrite(ON_Board_LED, HIGH);
  
  Serial.println("");
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  
  client.setInsecure();
}

void loop() {
  now = millis();
  if (now - lastMeasure > 3000) {
    lastMeasure = now;

    // อ่านค่าจาก PIR Sensor
    int pirValue = digitalRead(PIR_SENSOR_PIN); 
    Serial.print("PIR Value: ");
    Serial.println(pirValue);

    if (isnan(pirValue)) {
      Serial.println("Failed to read from PIR sensor!");
      return;
    }

    // ตรวจสอบการเคลื่อนไหวของ PIR Sensor
    if (pirValue == HIGH) {
        Serial.println("Motion Detected!");  // แสดงเมื่อมีการตรวจจับการเคลื่อนไหว
        digitalWrite(LED_PIN, HIGH);         // เปิดไฟ LED เมื่อพบการเคลื่อนไหว
        //digitalWrite(LED_PIN, LOW);
    } else {
        Serial.println("No Motion");         // แสดงเมื่อไม่พบการเคลื่อนไหว
        digitalWrite(LED_PIN, LOW);          // ปิดไฟ LED เมื่อไม่พบการเคลื่อนไหว
        //digitalWrite(LED_PIN, HIGH);
    }


    // ส่งสถานะ PIR ไปยัง Google Apps Script
    sendData(pirValue);
  }
}

void sendData(int pirStatus) {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);
  
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  String url = "/macros/s/" + GAS_ID + "/exec?pir=" + pirStatus;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: ESP32\r\n" +
         "Connection: close\r\n\r\n");

  Serial.println("request sent");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("ESP32/Arduino CI successful!");
  } else {
    Serial.println("ESP32/Arduino CI has failed");
  }
  
  Serial.print("reply was: ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  client.stop();
}
