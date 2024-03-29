#ifndef __MeteoData_H
#define __MeteoData_H

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_BME280.h>

#define SENSOR_A_PIN 19
#define SENSOR_B_PIN 18
#define SENSOR_C_PIN 5

struct TempAndHumidity
{
    float temperature;
    float humidity;
    float pressure;
};

class MeteoData
{
  public:
    // TempAndHumidity sensorA;
    // TempAndHumidity sensorB;
    // TempAndHumidity sensorC;
    TempAndHumidity sensorOutdoor;
    MeteoData();
    void initializeSensors();
    void setData();

  private:
    void setSensorData(TempAndHumidity *data);
    void printSensorData(TempAndHumidity *data);
};

#endif
