#include <Arduino.h>
#include "common.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <string>
#include <time.h>
#include <stdlib.h>
#include <Adafruit_APDS9960.h>

extern sensors_event_t event;
extern Adafruit_BNO055 bno;
Adafruit_APDS9960 apds;
extern VL53L0X tof;

#define PIN 27
#define NUMPIXELS 32

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

extern Motor BRmotor, FRmotor, BLmotor, FLmotor;
uint16_t r, g, b, c;

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

  tcaselect(color);
  if (!apds.begin()) {
    Serial.println("APDS allocation failed");
    for (;;)
      ;
  }
  apds.enableColor(true);
  Serial.println("APDS allocation succeeded");

  for (int i = 0; i < 2; i++) {
    tcaselect(i);
    tof.setTimeout(500);
    if (!tof.init()) {
      Serial.println("tof failed");
      while (1)
        ;
    }
    tof.startContinuous();
    Serial.println("tof initialized");
  }

  tcaselect(imu);  
  if (!bno.begin()) {
        Serial.println("No BNO055 detected");
        while (1);
    }

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

  apds.getColorData(&r, &g, &b, &c);
  //Serial.print("Clear: ");
  //Serial.println(c);

  tcaselect(front);
  Serial.print("Front: ");
  Serial.print(tof.readRangeContinuousMillimeters());
  Serial.print("\t");
  tcaselect(side);
  Serial.print("Side: ");
  Serial.println(tof.readRangeContinuousMillimeters());

  if(turn == 1 || turn == 2 || turn == 3){
    tone(22, 450, 100);
  }
  else if (turn == 4) { //grimg
    tone(22, 800, 500);
    stopMotors();
    //delay(1000);
  }


  Serial.print(fleft);
  Serial.print("\t");
  Serial.print(fright);
  Serial.print("\t");
  Serial.println(turn);

  if (turn == 3){
    Serial.println("\n\nDOUBLE GREEN!\n\n");
    doublegreen();
  }
  else if(c > 350) {
    tone(22, 800, 500);
    Serial.println("\t\t\tSEEN SILVERRRRR");
    stopMotors();
  }
  else{
    tcaselect(imu);
    bno.getEvent(&event);
    Serial.print("Z-Direction: ");
    Serial.println(event.orientation.z);
    if (event.orientation.z > 10) {
      FLmotor.speed(cmap(fleft*2));
      BLmotor.speed(cmap(fleft*2));
      FRmotor.speed(cmap(fright*2));
      BRmotor.speed(cmap(fright*2));
    }
    else {
      FLmotor.speed(cfleft);
      BLmotor.speed(bleft);
      FRmotor.speed(cfright);
      BRmotor.speed(bright);
    }
  }
  
  obstacle_detection();

  while (Serial1.available())
  {
    Serial1.read();
  }
}