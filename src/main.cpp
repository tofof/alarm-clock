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

// NTP Time
#include "NTPClient.h"
#include "TimeLib.h"
#define MILLI_PER_MIN ((time_t)(SECS_PER_MIN * 1000))
NTPClient timeClient(ntpUDP, "us.pool.ntp.org", 0, 20*MILLI_PER_MIN);
void setup_ntp();
void update_time();

// MQTT
#include "PubSubClient.h"
#define TOPIC_TIME "homeassistant/device/bedalarm/time"
#define TOPIC_ENABLE "homeassistant/device/bedalarm/enable"
#define TOPIC_SNOOZE "homeassistant/device/bedalarm/snooze"
PubSubClient mqttClient(wifiClient);
void setup_mqtt();
void callback_mqtt(char*, byte*, unsigned int);

// Alarm Itself
bool alarmEnabled = false;
bool alarmingNow = false;
time_t alarmTime = 0;
time_t snoozeTime = 0;
void print_times();
void check_alarm();

// Pressure sensor
#define PRESSURE_THRESHOLD 600
#define PRESSURE_PIN A0
uint16_t pressure=0;
bool bedOccupied = false;
void update_pressure();

// 7-segment display
#include "TM1637TinyDisplay.h"
#define DIO 12
#define CLK 13
TM1637TinyDisplay display(CLK, DIO);


void setup()
{
  Serial.begin(115200);
  while(!Serial) {}; // wait
  //setup_wifi();
  //timeClient.begin();
  //timeClient.update();
  setup_sound();
  display.setBrightness(BRIGHT_7);
  pinMode(PRESSURE_PIN, INPUT);
  //setup_mqtt();
}

void loop()
{
  //if (WiFi.status() != WL_CONNECTED) setup_wifi();
  //if (!mqttClient.connected()) setup_mqtt();
  //mqttClient.loop();                        // need to receive mqtt to know if snooze happened
  update_pressure();
  //check_alarm();                            // need to evaluate whether we should be alarming
  if (mp3->isRunning()) {                   // need to continually loop without delay during playback
    if (!mp3->loop()) {
      Serial.println("Audio stopped");
      mp3->stop();
      delete file;
      delete mp3;
      delete out;
      setup_sound();
    }
    //Serial.println("Wake up! Rise and shine!");
  } else {
    //update_time();
    //print_times();
    delay(100);                            // only delay while not playing audio
  }
}

void update_pressure() {
  pressure = (pressure*9 + analogRead(PRESSURE_PIN))/10;  //smoothing
  bedOccupied = pressure > PRESSURE_THRESHOLD;
  display.showNumber(analogRead(PRESSURE_PIN));
  Serial.println(pressure);
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
  if (mqttClient.subscribe("homeassistant/device/bedalarm/time", 1)) {    //unix timestamp of next alarm time, 10m delay at rollover before sending tomorrow's date
    Serial.println("Subscribed to time");
  } else {
    Serial.println("Couldn't subscribe to alarm time");
  }
  if (mqttClient.subscribe("homeassistant/device/bedalarm/enable", 1)) {  //string "on" or "off" 
    Serial.println("Subscribed to enable");
  } else {
    Serial.println("Couldn't subscribe to alarm enable");
  }
  if (mqttClient.subscribe("homeassistant/device/bedalarm/snooze", 1)) {    //unix timestamp of end of most recently requested snooze/suspend
    Serial.println("Subscribed to snooze");
  } else {
    Serial.println("Couldn't subscribe to alarm snooze");
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
  if (strcmp(topic, TOPIC_SNOOZE) == 0) {
    snoozeTime = atoi((char*)payload);
    //if (snoozeTime > now()) alarmingNow = false;  // should be redundant with check_alarm() 
  }
}

void update_time() {
  if(now() < SECS_YR_2000) {
    Serial.println("Forcing time initialization");
    while(!timeClient.forceUpdate()) {};
    setTime(timeClient.getEpochTime());
  }
  if(timeClient.update()) { //uses interval specified at initialization
    Serial.println("Time Updated");
    setTime(timeClient.getEpochTime());
  }
}

void print_times() {
  Serial.print(now());
  Serial.print("    Alarm at ");
  Serial.print(alarmTime);
  Serial.print(" ");
  Serial.print(alarmEnabled);
  Serial.print(" in ");
  Serial.print((alarmTime-now())/SECS_PER_HOUR);
  Serial.print(" hours.");
  Serial.println();
}

void check_alarm() {
  if (alarmEnabled && now()>alarmTime && now()>snoozeTime && bedOccupied) {
    alarmingNow = true;
    if (!mp3->isRunning()) {
      mp3->begin(file, out);  //start or restart playback when not playing. Should restart immediately without a 1000ms delay given ordering of loop()
    }
  } else if (alarmingNow && (now()<snoozeTime || bedOccupied==false)) {  //don't use 1) toggling because of risk of not reenabling 2) time because of rollover behavior
    alarmingNow = false;
    mp3->stop();
  }
}
