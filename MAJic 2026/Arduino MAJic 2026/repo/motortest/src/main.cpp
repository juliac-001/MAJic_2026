#include <Arduino.h>
#include "common.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <string>
#include <time.h>
#include <stdlib.h>

#define PIN 29
#define NUMPIXELS 32

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

extern Motor BRmotor, FRmotor, BLmotor, FLmotor;

void setup()
{
  Serial.begin(115200);
  delay(2000);

  Serial1.setRX(17);
  Serial1.setTX(16);
  Serial1.begin(115200);

  Wire.setSDA(28);
  Wire.setSCL(29);
  Wire.begin();

  pinMode(20, INPUT);

  Serial.println("Press GP20 to begin...");
  while (digitalRead(20))
    delay(10);
  delay(1000);

  Serial1.write("123456");
  delay(500);

  pixels.begin();
  pixels.setBrightness(255);
  pixels.show();

  for (int i = 0; i < NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(255, 255, 255));
  }

  pixels.show();
}

void loop()
{ 
  FLmotor.speed(-50);
  BLmotor.speed(-50);
  FRmotor.speed(-50);
  BRmotor.speed(-50);
  delay(1000);

  FLmotor.speed(-200);
  BLmotor.speed(-200);
  FRmotor.speed(-200);
  BRmotor.speed(-200);
  delay(1000);

  FLmotor.speed(0);
  BLmotor.speed(0);
  FRmotor.speed(0);
  BRmotor.speed(0);
  delay(0);
}