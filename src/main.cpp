#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include "secrets.h"

// Library for Audio
#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorAAC.h"
#include "AudioOutputI2SNoDAC.h"
#include "AudioFileSourcePROGMEM.h"

// Library for NTP Clock
#include <NTPClient.h>
//#include <TimeLib.h>

WiFiUDP ntpUDP;

AudioFileSourceSPIFFS *file;
AudioGeneratorAAC *aac;
AudioOutputI2SNoDAC *out;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", 25200, 600000);
unsigned long nextUpdTime;
int time_between_update; //in minutes int

void setup_wifi();

void setup()
{
  Serial.begin(115200);
  setup_wifi();
  
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
    Serial.printf("AAC done\n");
    delay(1000);
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  WiFi.hostname("AlarmClock");
  WiFi.setPhyMode(WIFI_PHY_MODE_11B);
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  Serial.print("WiFi connected on IP address ");
  Serial.println(WiFi.localIP());
}

