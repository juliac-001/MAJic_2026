#pragma once
#include <Arduino.h>

//motors
struct Motor
{
    uint8_t fpin;
    uint8_t rpin;
    void speed(int val);
};

//Motor BRmotor, FRmotor, BLmotor, FLmotor;