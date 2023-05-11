#include <Arduino.h>
#include <InternetConnection.h>
#include "esp32-hal-cpu.h"

// cervenby S09 start 13.2. v 21:30 - 14.2. 23:30 - 26 hodin
// DD0503MA start 15.2. 7:30 - 16.2. 13:45 - 30 hodin
// 403MA start 18.2. 7:30 - 19.2. 11:45 - 28 hodin

// DD0503MA bez senzoru start 20.2. 7:30 - 15:30 - 32 hodin

InternetConnection connection;
MeteoData meteoData;
PowerController powerController;
MagneticLockController magneticLockController;

void sendDataToInternet();
void checkMagneticLockAlarm();

// alarm section
void sendDataToBlynkIfAlarm();

// TODO: podpora 5.1V z baterky a modemu -> voltage divider 4.7kΩ a 22kΩ, zere pak 191uA https://electronics.stackexchange.com/questions/404230/do-voltage-dividers-waste-battery
// https://github.com/stephaneAG/SIM800L/blob/master/README.md

void setup()
{
  Serial.begin(115200);

  // // save battery, setup lower CPU frequency (default is 240Mhz)
  // setCpuFrequencyMhz(80);
  // Serial.println("Get CPU Frequency: " + String(getCpuFrequencyMhz()) + "Mhz");

  connection.initialize();

  meteoData.initializeSensors();

  // set first data for magnetic locks, other in timers..
  magneticLockController.setData();
  Serial.println("Setup done, send first data");
}

void loop()
{
  sendDataToInternet();
  Serial.println("First data sended, BYE");
  esp_deep_sleep(connection.deepSleepInterval * 1000000);
}

void sendDataToInternet()
{
  Serial.println("Start initialize Blynk connection");
  if (connection.initializeConnection())
  {
    Serial.println("Setting sensors data");
    meteoData.setData();
    powerController.setData();
    // gyroscope and magnetic locks data are set in other timer more often, so we have actual data

    Serial.println("Sending data to Blynk");
    connection.sendDataToBlynk(meteoData, powerController, magneticLockController);
    connection.disconnect();
  }
  else
  {
    Serial.println("No internet/blynk connection");
    connection.disconnect();
  }
}

void checkMagneticLockAlarm()
{
  magneticLockController.setData();
  if (magneticLockController.isOk())
  {
    // update blynk data and turn alarm off
    if (connection.isAlarm)
    {
      connection.setMagneticLockControllerDataToBlynkIfAlarm(magneticLockController);
    }
    connection.isAlarm = false;
  }
  else
  {
    connection.isAlarm = true;
    connection.alarmMagneticController(magneticLockController);
  }
}

void sendDataToBlynkIfAlarm()
{
  connection.setMagneticLockControllerDataToBlynkIfAlarm(magneticLockController);
}
