#include "DHT.h"
#include <PubSubClient.h> // MQTT Bibliothek
#include <ESP8266WiFi.h>
#include <ArduinoJson.h> // JSON library

#define DHTPIN 2    // Digital Pin 2
#define DHTTYPE DHT11   // 

const char* ssid = "Quỳnh DT";
const char* password = "quynh1703";
const char* TOPIC_TEMPERATURE = "Nhiet do";
const char* TOPIC_HUMIDITY = "Do am";

const char* TOPIC = "iot/data";

const char* broker = "192.168.126.109";
const char* device = "ESP_8266";
const char* location = "---"; 

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

class Measure{
public:
  Measure()
  :temperature(0),
  humidity(0)
  
  {};

  bool operator==(Measure& rhs)const {
    if (rhs.temperature == temperature && rhs.humidity == humidity){
      return true;
    } else {
      return false;
    }
  }
  
  float temperature;
  float humidity;
  
};

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
  
void setup() {
  Serial.begin(115200);
  setup_wifi();
  Serial.println("DHT11 Procucer Init ...");
  dht.begin();

  client.setServer(broker, 1883); // Adresse des MQTT-Brokers
  // client.setCallback(callback);   // Handler für eingehende Nachrichten

  delay(1500);

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Measuree abfragen und rausblasen.
  if (timer(60 * 10)) {  // alle 10 Minuten eine Measurement ausgeben
      bool compare = false;
      measurement(compare);
  }
  else { // ... sonst bei Measureänderung alle 2 Seconds 
    if (timer(2)) {
      bool compare = true;
      measurement(compare);
    }
  }
}

void measurement(bool compare){

    // Measure einlesen
    Measure measure = readValue();
    // Measure ausgeben
    publishMeasure(measure, compare);
}

Measure readValue(){

  Measure measurement;

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Lỗi đọc từ cảm biến DHT!");
    return measurement;
  }

  measurement.temperature = t - 1;  
  measurement.humidity = h;

  return measurement;
}

/*
 *  Xuất gtri lên MQTT
 *  so sánh giá trị nếu có thay đổi
 */
void publishMeasure(const Measure& measure, bool compare ){

    static Measure alterMeasure;
    
    bool publish = true;
    if (compare) {
      // so sánh
      if (measure == alterMeasure) {
        publish = false;
      } 
      
      if (inBlacklist(measure)) {
        publish = false;
      }
    }
    
    if (publish) {

      alterMeasure = measure;
    
      if (client.connected()) {
        const int capacity = JSON_OBJECT_SIZE(4);
        StaticJsonDocument<capacity> jsonDoc;
        
  //   //   JsonObject root = jsonDoc.as<JsonObject>();
  //       jsonDoc["id"] = "riu";
  //       jsonDoc["data"] = measure.temperature;
  //       jsonDoc["unit"] = "°C";
  //       jsonDoc["device"] = device;
        
  // //      root["recordingdate"] = "";
  //       String sTem; 
  //       serializeJson(jsonDoc, sTem);   
  //       client.publish(TOPIC_TEMPERATURE,sTem.c_str());
  //       Serial.println("Temperature: ");
  //       serializeJson(jsonDoc,Serial);        
  //       Serial.println();
  //       jsonDoc.clear();
        
  //       // jsonDoc["id"] = WiFi.macAddress();
  //       jsonDoc["id"] = "riu";
  //       jsonDoc["data"] = measure.humidity;
  //       jsonDoc["unit"] = "%";
  //       jsonDoc["device"] = device;
        
  // //      root["recordingdate"] = "";
  //       String sHum;
  //       serializeJson(jsonDoc, sHum);   
  //       client.publish(TOPIC_HUMIDITY,sHum.c_str());
  //       Serial.println("Humidity: ");
  //       serializeJson(jsonDoc, Serial);        
  //       Serial.println();
  //       jsonDoc.clear();

        jsonDoc["id"] = "riu";
        jsonDoc["device"] = device;
        jsonDoc["temperature"] =  measure.temperature;
        jsonDoc["humidity"] = measure.humidity;
        String str;
        serializeJson(jsonDoc, str);  
        client.publish(TOPIC, str.c_str());
      }
    }
  }


  bool timer(unsigned int seconds){

    static unsigned long millis_0 = 0;
    
    bool ret = false;
    unsigned long millis_t = millis();
    unsigned long time = seconds * 1000;
    if (millis_t - millis_0 >= time) {
      millis_0 = millis_t;
      Serial.print("Thời gian chờ đợi: ");
      Serial.println(millis_0);
      ret = true;
    }
    return ret;
  }
  
  bool inBlacklist(Measure measure) {

    bool inBlacklistGefunden = false;
    
    static const int ARRAYGROESSE = 6;
    static Measure blacklist[ARRAYGROESSE];

    int i = 0;
    // in blacklist suchen ...
    for(i = 0; i < ARRAYGROESSE; i++ ) {
      if (measure == blacklist[i]){
        inBlacklistGefunden = true;
        break;
      }
    }

    // ... nicht in blacklist gefunden. => Daher in blacklist aufnehmen 
    if (!inBlacklistGefunden){
      // Alle rutschen einen Stuhl weiter. Der letzte wird überschrieben 
      for(i = ARRAYGROESSE - 1; i > 0; i-- ) {
        blacklist[i] = blacklist[i-1];
      }
       // Der Neue kommt auf den ersten Platz
      blacklist[0] = measure;
    }
  
    return inBlacklistGefunden;  
  }


void reconnect() {
  while (!client.connected()) {
    Serial.print("Versuch des MQTT Verbindungsaufbaus...");
    //Verbindungsversuch:
    if (client.connect("arduinoClient_Werner")) {
      delay(200);
      Serial.println("Erfolgreich verbunden!");

    } else { // Im Fehlerfall => Fehlermeldung und neuer Versuch
      Serial.print("Fehler, rc=");
      Serial.print(client.state());
      Serial.println(" Nächster Versuch in 5 Seconds");
      delay(5000);
    }
  }
}