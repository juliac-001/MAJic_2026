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
Adafruit_BNO055 bno;
sensors_event_t event;

void tcaselect(uint8_t i) {
  if (i > 3) return;

  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);

  int ret = Wire.endTransmission();

  if (ret != 0) {
    Serial.print("Mux error: ");
    Serial.println(ret);
    while (1) delay(10);
  }
}

void stopMotors() {
  FLmotor.speed(0);
  BLmotor.speed(0);
  FRmotor.speed(0);
  BRmotor.speed(0);
}

void straight() {
  FLmotor.speed(50);
  BLmotor.speed(50);
  FRmotor.speed(50);
  BRmotor.speed(50);
}

void back() {
  FLmotor.speed(-50);
  BLmotor.speed(-50);
  FRmotor.speed(-50);
  BRmotor.speed(-50);
}

int target = 0;
const int turn_range = 5;
int targetSpeed = 60;
int turndegree = 90;

bool inRange(float angle, float low, float high) {
  if (low <= high) {
    return angle >= low && angle <= high;
  }
  return angle >= low || angle <= high;
}

float hightarget(float target) {
  float high = 0;
  if (target + turn_range > 360)
    high = (target + turn_range) - 360;
  else
   high = target + turn_range;
  return high;
}

float lowtarget(float target) {
  float low = 0;
  if (target - turn_range < 0)
    low = (target - turn_range) + 360;
  else
    low = target - turn_range;
  return low;
}

void right() {
  tcaselect(imu);
  bno.getEvent(&event);
  tone(22, 200, 200);
  target = fmod(event.orientation.x + turndegree + 360, 360);

  float h_target = hightarget(target);
  float l_target = lowtarget(target);

  while (true) {
    bno.getEvent(&event);

    Serial.print("BNO: ");
    Serial.print(event.orientation.x);

    Serial.print("\tTarget: ");
    Serial.println(target);

    if (inRange(event.orientation.x, l_target, h_target)) {
      break;
    }

    BRmotor.speed(-targetSpeed);
    FRmotor.speed(-targetSpeed);
    BLmotor.speed(targetSpeed);
    FLmotor.speed(targetSpeed);
  }

  stopMotors();
  delay(500);
}

void left() {
  tcaselect(imu);
  bno.getEvent(&event);
  tone(22, 100, 200);
  target = fmod(event.orientation.x - turndegree + 360, 360);

  float h_target = hightarget(target);
  float l_target = lowtarget(target);

  while (true) {

    bno.getEvent(&event);

    Serial.print("BNO: ");
    Serial.print(event.orientation.x);
    Serial.print("\tTarget: ");
    Serial.println(target);

    if (inRange(event.orientation.x, l_target, h_target)) {
      break;
    }

    BRmotor.speed(targetSpeed);
    FRmotor.speed(targetSpeed);
    BLmotor.speed(-targetSpeed);
    FLmotor.speed(-targetSpeed);
  }

  stopMotors();
  delay(500);
}

void doublegreen() {
  back();
  delay(500);

  tcaselect(imu);
  bno.getEvent(&event);
  tone(22, 100, 200);

  target = fmod(event.orientation.x + 180 + 360, 360);

  float h_target = hightarget(target);
  float l_target = lowtarget(target);

  while (true) {

    bno.getEvent(&event);

    Serial.print("BNO: ");
    Serial.print(event.orientation.x);
    Serial.print("\tTarget: ");
    Serial.println(target);

    if (inRange(event.orientation.x, l_target, h_target)) {
      break;
    }

    BRmotor.speed(targetSpeed);
    FRmotor.speed(targetSpeed);
    BLmotor.speed(-targetSpeed);
    FLmotor.speed(-targetSpeed);
  }

  stopMotors();
  delay(500);
}
