#include "common.h"

VL53L0X tof;

void obstacleAvoid() {
    right(90);
    tcaselect(side);
    while (tof.readRangeContinuousMillimeters() > 100){
        straight();
        Serial.println(tof.readRangeContinuousMillimeters());
    }
    while (tof.readRangeContinuousMillimeters() < 100) {
        straight();
        Serial.println(tof.readRangeContinuousMillimeters());
    }
    
    straight();
    delay(1500);

    left(90);

    while (tof.readRangeContinuousMillimeters() > 100){
        straight();
    }

    while (tof.readRangeContinuousMillimeters() < 100) {

        straight();
    }

    straight();
    delay(1500);

    left(90);

    while (tof.readRangeContinuousMillimeters() > 100){
        straight();
    }

    right(90);

    stopMotors();
}

void obstacle_detection() {
    tcaselect(front);
    float distance = tof.readRangeContinuousMillimeters();
    if ((distance < 120) && (distance > 80)) {
        stopMotors();
        Serial.print(distance);
        if (tof.readRangeContinuousMillimeters() < 100)
            obstacleAvoid();
    }
}