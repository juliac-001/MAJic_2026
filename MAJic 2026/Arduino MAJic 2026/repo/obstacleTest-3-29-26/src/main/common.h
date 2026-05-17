#pragma once
#include <Wire.h>
#include <string>
#include <Servo.h>
#include <VL53L0X.h>
#include <Adafruit_BNO055.h>

#define TCAADDR 0x70
#define color 0
#define ltof 2
#define rtof 1
#define imu 1

// VL53L0X tof;

//motors
struct Motor
{
    uint8_t fpin;
    uint8_t rpin;
    void speed(int val);
};

//Motor BRmotor, FRmotor, BLmotor, FLmotor;

//obstacle
void obstacle_detection();
void tcaselect(uint8_t i);
void stopMotors();
void straight();
void left(int turndegree);
void right(int turndegree);
void doublegreen();
