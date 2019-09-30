#include "MagneticLockController.h"

MagneticLockController::MagneticLockController()
{
    pinMode(LOCK_A_PIN, INPUT);
    pinMode(LOCK_B_PIN, INPUT);
    pinMode(LOCK_C_PIN, INPUT);
}

void MagneticLockController::setData()
{
    Serial.print("Magnetic lock A: ");
    setSensorData(&sensorA, LOCK_A_PIN);
    Serial.print("Magnetic lock B: ");
    setSensorData(&sensorB, LOCK_B_PIN);
    Serial.print("Magnetic lock C: ");
    setSensorData(&sensorC, LOCK_C_PIN);
}

void MagneticLockController::setSensorData(LockData *data, int pin)
{
    if (digitalRead(pin) == HIGH)
    {
        data->locked = true;
        data->status = "OK";
        Serial.println("OK");
    }
    else
    {
        data->locked = false;
        data->status = "ALARM!";
        Serial.println("UNLOCKED!");
    }
}

// Check if all lockers are locked
bool MagneticLockController::isOk()
{
    return sensorA.locked &&
           sensorB.locked &&
           sensorC.locked;
}

// alarm message for alarm notification
String MagneticLockController::getAlarmMessage()
{
    String message = "";
    return String(sensorA.locked ? "" : "A") + String(sensorB.locked ? "" : "B") + String(sensorC.locked ? "" : "C");
}