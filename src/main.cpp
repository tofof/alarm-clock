#include <Arduino.h>
#include "secrets.h"

// WiFi
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
WiFiUDP ntpUDP;
WiFiClient wifiClient;
void setup_wifi();

// Audio
#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
AudioFileSourceSPIFFS *file;
AudioGeneratorMP3 *mp3;
AudioOutputI2S *out;
char path[7];
int count=0;
void setup_sound();

unsigned long nextTime = 0;

// MQTT
#include "PubSubClient.h"
#define TOPIC_TIME "homeassistant/device/bedalarm/time"
#define TOPIC_ENABLE "homeassistant/device/bedalarm/enable"
#define TOPIC_SNOOZE "homeassistant/device/bedalarm/snooze"
PubSubClient mqttClient(wifiClient);
void setup_mqtt();
void callback_mqtt(char*, byte*, unsigned int);

// Alarm Itself
bool alarmingNow = false;


// 7-segment display
#include "TM1637TinyDisplay.h"
#define DIO 12
#define CLK 13
TM1637TinyDisplay display(CLK, DIO);

// Data from Animator Tool
const uint8_t IDLE_ANIMATION[38][4] = {
  { 0x10, 0x00, 0x00, 0x00 },  // Frame 0
  { 0x18, 0x00, 0x00, 0x00 },  // Frame 1
  { 0x1c, 0x00, 0x00, 0x00 },  // Frame 2
  { 0x5c, 0x00, 0x00, 0x00 },  // Frame 3
  { 0x5c, 0x10, 0x00, 0x00 },  // Frame 4
  { 0x5c, 0x18, 0x00, 0x00 },  // Frame 5
  { 0x5c, 0x1c, 0x00, 0x00 },  // Frame 6
  { 0x5c, 0x5c, 0x00, 0x00 },  // Frame 7
  { 0x5c, 0x5c, 0x10, 0x00 },  // Frame 8
  { 0x5c, 0x5c, 0x18, 0x00 },  // Frame 9
  { 0x5c, 0x5c, 0x1c, 0x00 },  // Frame 10
  { 0x5c, 0x5c, 0x5c, 0x00 },  // Frame 11
  { 0x5c, 0x5c, 0x5c, 0x10 },  // Frame 12
  { 0x5c, 0x5c, 0x5c, 0x18 },  // Frame 13
  { 0x5c, 0x5c, 0x5c, 0x1c },  // Frame 14
  { 0x5c, 0x5c, 0x5c, 0x5c },  // Frame 15
  { 0x5c, 0x5c, 0x5c, 0x5c },  // Frame 16
  { 0x5c, 0x5c, 0x5c, 0x5c },  // Frame 17
  { 0x5c, 0x5c, 0x5c, 0x5c },  // Frame 18
  { 0x5c, 0x5c, 0x5c, 0x1c },  // Frame 19
  { 0x5c, 0x5c, 0x5c, 0x18 },  // Frame 20
  { 0x5c, 0x5c, 0x5c, 0x10 },  // Frame 21
  { 0x5c, 0x5c, 0x5c, 0x00 },  // Frame 22
  { 0x5c, 0x5c, 0x1c, 0x00 },  // Frame 23
  { 0x5c, 0x5c, 0x18, 0x00 },  // Frame 24
  { 0x5c, 0x5c, 0x10, 0x00 },  // Frame 25
  { 0x5c, 0x5c, 0x00, 0x00 },  // Frame 26
  { 0x5c, 0x1c, 0x00, 0x00 },  // Frame 27
  { 0x5c, 0x18, 0x00, 0x00 },  // Frame 28
  { 0x5c, 0x10, 0x00, 0x00 },  // Frame 29
  { 0x5c, 0x00, 0x00, 0x00 },  // Frame 30
  { 0x1c, 0x00, 0x00, 0x00 },  // Frame 31
  { 0x18, 0x00, 0x00, 0x00 },  // Frame 32
  { 0x10, 0x00, 0x00, 0x00 },  // Frame 33
  { 0x00, 0x00, 0x00, 0x00 },  // Frame 34
  { 0x00, 0x00, 0x00, 0x00 },  // Frame 35
  { 0x00, 0x00, 0x00, 0x00 },  // Frame 36
  { 0x00, 0x00, 0x00, 0x00 }   // Frame 37
};

/* Animation Data - HGFEDCBA Map */
const uint8_t ALARM_ANIMATION[13][4] = {
  { 0x00, 0x00, 0x00, 0x00 },  // Frame 0
  { 0x00, 0x00, 0x00, 0x00 },  // Frame 1
  { 0x00, 0x00, 0x00, 0x00 },  // Frame 2
  { 0x00, 0x00, 0x00, 0x00 },  // Frame 3
  { 0x00, 0x02, 0x10, 0x00 },  // Frame 4
  { 0x00, 0x06, 0x30, 0x00 },  // Frame 5
  { 0x00, 0x16, 0x32, 0x00 },  // Frame 6
  { 0x00, 0x36, 0x36, 0x00 },  // Frame 7
  { 0x02, 0x36, 0x36, 0x10 },  // Frame 8
  { 0x06, 0x36, 0x36, 0x30 },  // Frame 9
  { 0x16, 0x36, 0x36, 0x32 },  // Frame 10
  { 0x36, 0x36, 0x36, 0x36 },  // Frame 11
  { 0x36, 0x36, 0x36, 0x36 }   // Frame 12
};

  


void setup()
{
  Serial.begin(115200);
  while(!Serial) {}; // wait
  display.setBrightness(BRIGHT_2);
  setup_wifi();
  setup_sound();
  setup_mqtt();
}

void loop()
{
  if (millis() > nextTime) {
    if (WiFi.status() != WL_CONNECTED) setup_wifi();
    if (!mqttClient.connected()) setup_mqtt();
    mqttClient.loop();                        // need to receive mqtt to know if snooze happened
    //display.showNumber(pressure);
    nextTime = millis() + 1000;
  }
  
  if (alarmingNow) {
    if (!display.Animate()) display.startAnimation(ALARM_ANIMATION, FRAMES(ALARM_ANIMATION), TIME_MS(25));  //startAn.. is nonblocking,  showAn.. presumably blocks
    if (mp3->isRunning()) {                   // need to continually loop without delay during playback
      if (!mp3->loop()) {
        Serial.println("Audio stopped");
        mp3->stop();
      }
    } else {
      setup_sound();
      Serial.println("Wake up! Rise and shine!");

      mp3->begin(file, out);  //start or restart playback when not playing. Should restart immediately without a 1000ms delay given ordering of loop()
    }
  } else {
    if (!display.Animate()) display.startAnimation(IDLE_ANIMATION, FRAMES(IDLE_ANIMATION), TIME_MS(100));  //startAn.. is nonblocking,  showAn.. presumably blocks
  }
}


void setup_sound() {
  SPIFFS.begin();
  sprintf(path, "/%d.mp3", count);
  file = new AudioFileSourceSPIFFS(path);
  if (count<5) count++;

  audioLogger = &Serial;
  mp3 = new AudioGeneratorMP3();
  out = new AudioOutputI2S();
}


void setup_wifi() {
  display.showString("WIFI");
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
  display.showString("MQTT");
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
  if (mqttClient.subscribe("homeassistant/device/bedalarm/enable", 1)) {  //string "on" or "off" 
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
      alarmingNow = true;
      display.stopAnimation();
      display.showString("On");
    } else if (payload[1] == 'f')  { // "oFf"
      alarmingNow = false;
      if (mp3->isRunning()) mp3->stop();
      count = 0;  //Reset volume
      display.stopAnimation();
      display.showString("Off");
    } else {
      Serial.print("ERROR received '");
      for (unsigned int i=0;i<length;i++) Serial.print((char)payload[i]);
      Serial.println("'");
    }
  }
}