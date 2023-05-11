#ifndef __InternetConnection_H
#define __InternetConnection_H

#include <Update.h>
#include <HTTPClient.h>
#include <MeteoData.h>
#include <PowerController.h>
#include <MagneticLockController.h>

class InternetConnection
{
public:
  void initialize();
  bool modemReady;
  bool isAlarm;
  int deepSleepInterval = 285;
  bool initializeConnection();
  void disconnect();
  void sendDataToBlynk(MeteoData, PowerController, MagneticLockController);
  void alarmMagneticController(MagneticLockController);
  void blynkRunIfAlarm();
  void setMagneticLockControllerDataToBlynkIfAlarm(MagneticLockController);
  void printlnToTerminal(String);

private:
  void setMagneticLockControllerDataToBlynk(MagneticLockController);
  void setAlarmInfoToBlynk();
  void setI2CStatusVersion();
};

#endif