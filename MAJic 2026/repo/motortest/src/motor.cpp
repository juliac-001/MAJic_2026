#include "common.h"

void Motor::speed(int val) {
    if (val > 0) {
        analogWrite(fpin, val);
        analogWrite(rpin, 0);
    }
    else {
        analogWrite(fpin, 0);
        analogWrite(rpin, -(val));
    }
}

Motor FLmotor{12,13}, BLmotor{14,15}, FRmotor{8,9}, BRmotor{10,11};