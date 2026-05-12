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

int cmap(int speed){
  if(speed > 0)
    return map(speed, 0, 255, 50, 255);
  else if(speed < 0)
    return map(speed, 0, -255, -50, -255);
  else
    return 0;
}

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
  pixels.setBrightness(100);
  pixels.show();

  for (int i = 0; i < NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(255, 255, 255));
  }

  pixels.show();
}

void loop()
{ 
  int fleft = Serial1.parseInt();
  int cfleft = cmap(fleft);
  int fright = Serial1.parseInt();
  int cfright = cmap(fright);
  int turn = Serial1.parseInt();

  int bleft = cfleft;
  int bright = cfright;

  
  /*else if (turn == 1)
    rightgr();
  else if (turn == 2)
    leftgr();
  else if (turn == 3)
    doublegr();
  else {
    FLmotor.speed(fleft);
    BLmotor.speed(bleft);
    FRmotor.speed(fright);
    BRmotor.speed(bright);
  }*/

  if(turn == 1 || turn == 2 || turn == 3){
    tone(22, 450, 500);
  }
  else if (turn == 4) { //grimg
    tone(22, 800, 500);
    FLmotor.speed(0);
    BLmotor.speed(0);
    FRmotor.speed(0);
    BRmotor.speed(0);
    delay(1000);
  }


  Serial.print(fleft);
  Serial.print("\t");
  Serial.print(fright);
  Serial.print("\t");
  //Serial.println(turn);
  Serial.print(cfleft);
  Serial.print("\t");
  Serial.print(cfright);
  Serial.println("\t");

  FLmotor.speed(cfleft);
  BLmotor.speed(bleft);
  FRmotor.speed(cfright);
  BRmotor.speed(bright);

  while (Serial1.available())
  {
    Serial1.read();
  }
}