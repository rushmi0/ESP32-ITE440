#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "DHT.h"

#define DHTTYPE DHT11   // DHT 11
const int DHTPin = 27;

const int MoisturePin = 34;
const int LDR_SENSOR = 35;  // เพิ่มตัวแปรสำหรับ LDR Sensor

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

String GAS_ID = "AKfycbxYNkZIOt1YxUABcpvwpcIEjxwIVagz8e5ts_sMsiEiGF33aFVvua9CEI_l1OYNVGYa5A"; // apps script ID or deployment ID

void setup() {
  Serial.begin(115200);
  delay(500);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.println("");

  pinMode(ON_Board_LED, OUTPUT);
  pinMode(ON_Board_LED, INPUT);
  digitalWrite(ON_Board_LED, HIGH);

  Serial.print("Connecting");
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
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float f = dht.readTemperature(true);

    float m = analogRead(SOIL_MOISTURE_PIN);
    
    // อ่านค่าจาก LDR Sensor
    float ldrValue = analogRead(LDR_SENSOR); 

    if (isnan(h) || isnan(t) || isnan(f) || isnan(ldrValue)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    float hic = dht.computeHeatIndex(t, h, false);
    static char temperatureTemp[7];
    dtostrf(hic, 6, 2, temperatureTemp);
    
    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);

    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t Heat index: ");
    Serial.print(hic);
    Serial.println(" *C ");
    Serial.print("Soil Moisture: ");
    Serial.println(m);
    Serial.print("LDR Value: ");
    Serial.println(ldrValue);

    sendData(t, h, m, ldrValue);  // เพิ่ม ldrValue ในการส่งข้อมูล
  }
}

void sendData(float value, float value2, float value3, float ldrValue) {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);
  
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  float string_temp = value; 
  float string_humi = value2;
  float string_mois = value3;
  float string_ldr = ldrValue;
  String url = "/macros/s/" + GAS_ID + "/exec?temp=" + string_temp + "&humi=" + string_humi + "&soil=" + string_mois + "&ldr=" + string_ldr;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
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
  
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();
  client.stop();
}
