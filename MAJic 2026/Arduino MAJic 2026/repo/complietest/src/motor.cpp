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

Motor BRmotor{12,13}, FRmotor{14,15}, BLmotor{8,9}, FLmotor{10,11};