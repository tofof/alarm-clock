#include <Arduino.h>
#include "AudioGeneratorAAC.h"
#include "AudioOutputI2SNoDAC.h"
#include "AudioFileSourcePROGMEM.h"
#include "reveille.h"

AudioFileSourcePROGMEM *in;
AudioGeneratorAAC *aac;
AudioOutputI2SNoDAC *out;

void setup()
{
  Serial.begin(115200);

  audioLogger = &Serial;
  in = new AudioFileSourcePROGMEM(reveille, sizeof(reveille));
  aac = new AudioGeneratorAAC();
  out = new AudioOutputI2SNoDAC();

  aac->begin(in, out);
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

