#include <Arduino.h>

#define DEBUG 1
#define LED_PIN D4

// put function declarations here:
int myFunction(int, int);

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

int ledIntensity = 0;
int step = 1;

void loop() {
  analogWrite(LED_PIN, ledIntensity);
  delay(10);
  ledIntensity += step;
  if (ledIntensity % 255 == 0)
    step *= -1;
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}

