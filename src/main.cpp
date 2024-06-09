#include <Arduino.h>
#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorAAC.h"
#include "AudioOutputI2SNoDAC.h"
#include "AudioFileSourcePROGMEM.h"

AudioFileSourceSPIFFS *file;
AudioGeneratorAAC *aac;
AudioOutputI2SNoDAC *out;

void setup()
{
  Serial.begin(115200);
  delay(1000);
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

