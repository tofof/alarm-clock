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
#define MILLI_PER_MIN ((time_t)(SECS_PER_MIN * 1000))

// Libraries for MQTT
#include "PubSubClient.h"
#define TOPIC_TIME "homeassistant/device/bedalarm/time"
#define TOPIC_ENABLE "homeassistant/device/bedalarm/enable"

WiFiUDP ntpUDP;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

AudioFileSourceSPIFFS *file;
AudioGeneratorAAC *aac;
AudioOutputI2SNoDAC *out;

bool alarmEnabled = false;
time_t alarmTime = 0;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "us.pool.ntp.org", 0, 20*MILLI_PER_MIN);

void setup_wifi();
void setup_ntp();
void setup_mqtt();
void callback_mqtt(char*, byte*, unsigned int);
void setup_sound();
void play_sound();

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  while(!Serial) {}; //wait
  setup_wifi();
  digitalWrite(LED_BUILTIN, LOW); //pullup means 0 is full brightness
  timeClient.begin();
  timeClient.update();
  setup_sound();
  setup_mqtt();
}

void loop()
{
  if (aac->isRunning()) {
     aac->loop();
  } else {
    if(now() < 1000) {
      Serial.println("Forcing time initialization");
      while(!timeClient.forceUpdate()) {};
      setTime(timeClient.getEpochTime());
    }
    if(timeClient.update()) { //uses interval specified at initialization
      Serial.println("Time Updated");
      setTime(timeClient.getEpochTime());
    }
    Serial.print(now());
    Serial.print("    Alarm at ");
    Serial.print(alarmTime);
    Serial.print(" ");
    Serial.print(alarmEnabled);
    Serial.print(" in ");
    Serial.print((alarmTime-now())/SECS_PER_HOUR);
    Serial.print(" hours.");
    Serial.println();
    mqttClient.loop(); //call loop 
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
  delay(1000);
}

void setup_mqtt() {
  mqttClient.setServer(MQTT_Server, MQTT_Port);
  mqttClient.setCallback(callback_mqtt);
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String mqttClientId = "bedalarm8266";
    if (mqttClient.connect(mqttClientId.c_str(), MQTT_User, MQTT_Password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" will try again in 5 seconds");
      delay(5000);
    }
  }
  if (mqttClient.subscribe("homeassistant/device/bedalarm/time", 1)) {
    Serial.println("Subscribed to time");
  } else {
    Serial.println("Couldn't subscribe to alarm time");
  }
  if (mqttClient.subscribe("homeassistant/device/bedalarm/enable", 1)) {
    Serial.println("Subscribed to enable");
  } else {
    Serial.println("Couldn't subscribe to alarm enable");
  }
}

void callback_mqtt(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp(topic, TOPIC_ENABLE) == 0) {
    if (payload[1] == 'n') { // "oN"
      alarmEnabled = true;
    } else if (payload[1] == 'f')  { // "oFf"
      alarmEnabled = false;
    } else {
      Serial.print("ERROR received '");
      for (unsigned int i=0;i<length;i++) Serial.print((char)payload[i]);
      Serial.println("'");
    }
  }
  if (strcmp(topic, TOPIC_TIME) == 0) {
    alarmTime = atoi((char*)payload);
  }
}

void setup_sound() {
  SPIFFS.begin();
  file = new AudioFileSourceSPIFFS("/reveille.aac");

  audioLogger = &Serial;
  aac = new AudioGeneratorAAC();
  out = new AudioOutputI2SNoDAC();
}

void play_sound() {
  aac->begin(file, out);
}