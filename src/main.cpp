#include <Arduino.h>
#include "secrets.h"



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



// Alarm Itself
bool alarmEnabled = false;
bool alarmingNow = false;
bool bedOccupied = true; // debug temporary TODO FIXME
time_t alarmTime = 0;
time_t snoozeTime = 0;

void setup()
{
  Serial.begin(115200);
  while(!Serial) {}; // wait
  //setup_wifi();
  //timeClient.begin();
  //timeClient.update();
  setup_sound();
  //setup_mqtt();
}

void loop()
{
  //if (WiFi.status() != WL_CONNECTED) setup_wifi();
  //if (!mqttClient.connected()) setup_mqtt();
  //mqttClient.loop();                        // need to receive mqtt to know if snooze happened
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
  //  update_time();
    mp3->begin(file, out);
    Serial.println("Starting audio!");
    //print_times();
    //delay(1000);                            // only delay while not playing audio
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

