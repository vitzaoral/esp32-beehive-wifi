#include <Arduino.h>
#include <InternetConnection.h>
#include <Ticker.h>
#include "esp32-hal-cpu.h"

// cervenby S09 start 13.2. v 21:30 - 14.2. 23:30 - 26 hodin
// DD0503MA start 15.2. 7:30 - 16.2. 13:45 - 30 hodin
// 403MA start 18.2. 7:30 - 19.2. 11:45 - 28 hodin

// DD0503MA bez senzoru start 20.2. 7:30 - 15:30 - 32 hodin

InternetConnection connection;
MeteoData meteoData;
PowerController powerController;
//GyroscopeController gyroscopeController;
MagneticLockController magneticLockController;
//SirenController SirenController;

void sendDataToInternet();
//void checkIncomingCall();
//void checkGyroscopeAlarm();
void checkMagneticLockAlarm();

Ticker timerSendDataToInternet(sendDataToInternet, 288000);  // 4.8 min 288000
//Ticker timerCheckIncomingCall(checkIncomingCall, 5125);      // 5 sec
//Ticker timerGyroscopeAlarm(checkGyroscopeAlarm, 5321);       // 5 sec
Ticker timerMagneticLockAlarm(checkMagneticLockAlarm, 4321); // 4 sec

// alarm section
void sendDataToBlynkIfAlarm();
Ticker timerSendDataToBlynkIfAlarm(sendDataToBlynkIfAlarm, 20000); // 20 sec

// TODO: sound level measuring
// TODO: mic switchinng

// TODO: podpora 5.1V z baterky a modemu -> voltage divider 4.7kΩ a 22kΩ, zere pak 191uA https://electronics.stackexchange.com/questions/404230/do-voltage-dividers-waste-battery
// https://github.com/stephaneAG/SIM800L/blob/master/README.md

void setup()
{
  Serial.begin(115200);

  // save battery, setup lower CPU frequency (default is 240Mhz)
  setCpuFrequencyMhz(80);
  Serial.println("Get CPU Frequency: " + String(getCpuFrequencyMhz()) + "Mhz");

  connection.initialize();

  timerSendDataToInternet.start();
  //timerCheckIncomingCall.start();
  //timerGyroscopeAlarm.start();
  timerMagneticLockAlarm.start();
  timerSendDataToBlynkIfAlarm.start();

  meteoData.initializeSensors();

  // set first data for gyroscope and magnetic locks, other in timers..
  //gyroscopeController.setData();
  magneticLockController.setData();
  Serial.println("Setup done, send first data");
  sendDataToInternet();
  Serial.println("First data sended, start loop");
}

void loop()
{
  timerSendDataToInternet.update();
  //timerCheckIncomingCall.update();
  //timerGyroscopeAlarm.update();
  timerMagneticLockAlarm.update();
  timerSendDataToBlynkIfAlarm.update();

  connection.blynkRunIfAlarm();
}

void checkIncomingCall()
{
  connection.checkIncomingCall();
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
    connection.setMicrophoneGain();
    connection.sendDataToBlynk(meteoData, powerController, magneticLockController);
    connection.checkNewVersionAndUpdate();
    connection.disconnect();
  }
  else
  {
    Serial.println("No internet/blynk connection");
    connection.disconnect();
  }
}

// void checkGyroscopeAlarm()
// {
//   gyroscopeController.setData();
//   if (gyroscopeController.isOk())
//   {
//     // update blynk data and turn alarm off
//     if (connection.isAlarm)
//     {
//       connection.setGyroscopeControllerDataToBlynkIfAlarm(gyroscopeController);
//     }
//     connection.isAlarm = false;
//   }
//   else
//   {
//     connection.isAlarm = true;
//     connection.alarmGyroscopeController(gyroscopeController);
//   }
// }

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
  //connection.setGyroscopeControllerDataToBlynkIfAlarm(gyroscopeController);
  //connection.checkSirenAlarm();
}
