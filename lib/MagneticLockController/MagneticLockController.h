#ifndef __MagneticLockController_H
#define __MagneticLockController_H

#include <Arduino.h>

#define LOCK_A_PIN 25
#define LOCK_B_PIN 26
#define LOCK_C_PIN 27

struct LockData
{
    bool locked;
    String status;
};

class MagneticLockController
{
  public:
    MagneticLockController();
    LockData sensorA;
    LockData sensorB;
    LockData sensorC;
    void setData();
    bool isOk();
    String getAlarmMessage();

  private:
    void setSensorData(LockData *data, int);
};

#endif