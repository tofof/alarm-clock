#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include "secrets.h"

// Libraries for Audio
#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorAAC.h"
#include "AudioOutputI2SNoDAC.h"
#include "AudioFileSourcePROGMEM.h"

// Libraries for NTP Clock
#include "NTPClient.h"
#include "TimeLib.h"
//#include "Timezone.h"

// Libraries for MQTT
#include "PubSubClient.h"
#include "ArduinoJson.h"

#define MILLI_PER_MIN ((time_t)(SECS_PER_MIN * 1000))
WiFiUDP ntpUDP;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

AudioFileSourceSPIFFS *file;
AudioGeneratorAAC *aac;
AudioOutputI2SNoDAC *out;

int hours, minutes, seconds;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "us.pool.ntp.org", 0, 1*MILLI_PER_MIN);

void setup_wifi();
void setup_ntp();

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  while(!Serial) {}; //wait
  setup_wifi();
  digitalWrite(LED_BUILTIN, LOW); //pullup means 0 is full brightness
  timeClient.begin();
  
  SPIFFS.begin();
  file = new AudioFileSourceSPIFFS("/reveille.aac");

  audioLogger = &Serial;
  aac = new AudioGeneratorAAC();
  out = new AudioOutputI2SNoDAC();

  aac->begin(file, out);
}

void loop()
{
  if (aac->isRunning()) {
    aac->loop();
  } else {
    if(timeClient.update()) { //uses interval specified at initialization
      Serial.println("Time Updated");
    } 

    hours = timeClient.getHours();
    if (hours < 10) Serial.print("0");
    Serial.print(hours);
    
    Serial.print(":");
    
    minutes = timeClient.getMinutes();
    if (minutes < 10) Serial.print("0");
    Serial.print(minutes);
    
    Serial.print(":");
    
    seconds = timeClient.getSeconds();
    if (seconds < 10) Serial.print("0");
    Serial.print(seconds);

    Serial.println();    
    delay(1000);
  }
}

void setup_wifi() {
  Serial.println();
  WiFi.hostname("AlarmClock");
  WiFi.setPhyMode(WIFI_PHY_MODE_11B);
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);    
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  Serial.print(" WiFi connected on IP address ");
  Serial.println(WiFi.localIP());
}