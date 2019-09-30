#ifndef __GyroscopeController_H
#define __GyroscopeController_H

#include <Arduino.h>
#include <SparkFun_MMA8452Q.h>

#define GYROSCOPE_A_PIN 14
#define GYROSCOPE_B_PIN 12
#define GYROSCOPE_C_PIN 13

struct GyroscopeData
{
  float x;
  float y;
  float z;
  int orientationType;
  bool isOk;
  String orientation;
};

class GyroscopeController
{
public:
  GyroscopeController();
  GyroscopeData sensorA;
  GyroscopeData sensorB;
  GyroscopeData sensorC;
  bool isOk();
  void setData();
  void initializeSensors();
  String getAlarmMessage();

private:
  String setOrientation(byte);
  void setSensorData(GyroscopeData *data);
  void printCalculatedAccels(GyroscopeData *data);
};

#endif