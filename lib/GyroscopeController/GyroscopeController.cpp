#include "GyroscopeController.h"

// When SAD PIN is LOW, address is 0x1C, when is HIGH, address is 0x1D
MMA8452Q accel(0x1C);

GyroscopeController::GyroscopeController()
{
    // one of the sensors is read on the address 0x1C, others are set to the address 0x1D etc.
    pinMode(GYROSCOPE_A_PIN, OUTPUT);
    pinMode(GYROSCOPE_B_PIN, OUTPUT);
    pinMode(GYROSCOPE_C_PIN, OUTPUT);
    digitalWrite(GYROSCOPE_A_PIN, LOW);  // 0x1C
    digitalWrite(GYROSCOPE_B_PIN, HIGH); // 0x1D
    digitalWrite(GYROSCOPE_C_PIN, HIGH); // 0x1D
}

void GyroscopeController::initializeSensors()
{
    accel.init();
    if (!accel.available())
    {
        Serial.println("Could not find a valid Gyroscope sensor on oaddress 0x1C!");
    }
    else
    {
        Serial.println("Gyroscope sensors OK");
    }
}

void GyroscopeController::setData()
{
    int pause = 300;
    digitalWrite(GYROSCOPE_A_PIN, LOW);
    accel.init();
    delay(pause);
    Serial.print("Gyroscope A: ");
    setSensorData(&sensorA);
    printCalculatedAccels(&sensorA);
    digitalWrite(GYROSCOPE_A_PIN, HIGH);

    digitalWrite(GYROSCOPE_B_PIN, LOW);
    accel.init();
    delay(pause);
    Serial.print("Gyroscope B: ");
    setSensorData(&sensorB);
    printCalculatedAccels(&sensorB);
    digitalWrite(GYROSCOPE_B_PIN, HIGH);

    digitalWrite(GYROSCOPE_C_PIN, LOW);
    accel.init();
    delay(pause);
    Serial.print("Gyroscope C: ");
    setSensorData(&sensorC);
    printCalculatedAccels(&sensorC);
    digitalWrite(GYROSCOPE_C_PIN, HIGH);
    Serial.println();
}

void GyroscopeController::setSensorData(GyroscopeData *data)
{
    if (accel.available())
    {
        accel.read();
        byte orientation = accel.readPL();
        data->x = accel.cx;
        data->y = accel.cy;
        data->z = accel.cz;
        data->orientationType = orientation;
        data->isOk = orientation == LOCKOUT;
        data->orientation = GyroscopeController::setOrientation(orientation);
    }
    else
    {
        Serial.println("Gyroscope sensor not available!");
        data->isOk = false;
        data->orientation = "Not available!";
    }
}

String GyroscopeController::setOrientation(byte orientation)
{
    switch (orientation)
    {
    case PORTRAIT_U:
        return "Portrait Up!";
    case PORTRAIT_D:
        return "Portrait Down!";
    case LANDSCAPE_R:
        return "Landscape Right!";
    case LANDSCAPE_L:
        return "Landscape Left!";
    case LOCKOUT:
        return "OK - Flat";
    }
    return "";
}

void GyroscopeController::printCalculatedAccels(GyroscopeData *data)
{
    Serial.print(data->x, 3);
    Serial.print("\t");
    Serial.print(data->y, 3);
    Serial.print("\t");
    Serial.print(data->z, 3);
    Serial.print("\t");
    Serial.println(data->orientation);
}

// Check if all gyroscopes are in Flat position
bool GyroscopeController::isOk()
{
    return sensorA.isOk &&
           sensorB.isOk &&
           sensorC.isOk;
}

// alarm message for alarm notification
String GyroscopeController::getAlarmMessage()
{
    String message = "";
    return String(sensorA.isOk ? "" : "A") + String(sensorB.isOk ? "" : "B") + String(sensorC.isOk ? "" : "C");
}