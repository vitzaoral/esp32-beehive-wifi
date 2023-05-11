#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPLalHw40y_"
#define BLYNK_DEVICE_NAME "Vcely baterky"
#define BLYNK_FIRMWARE_VERSION "2.1.0"

#include "InternetConnection.h"
#include <BlynkSimpleEsp32.h>
#include "../../src/settings.cpp"

// hard reset
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

Settings settings;

// alarm notifications are enabled
bool alarmEnabledNotifications = false;

// alarm is enabled
bool alarmIsEnabled = false;

#define SDA 21
#define SCL 22

// need lot of blynk virtual pins :)
#define BLYNK_USE_128_VPINS

// Synchronize settings from Blynk server with device when internet is connected
BLYNK_CONNECTED()
{
    Blynk.syncAll();
}

String overTheAirURL = "";

BLYNK_WRITE(InternalPinOTA)
{
    overTheAirURL = param.asString();

    Serial.println("OTA Started");
    overTheAirURL = param.asString();
    Serial.print("overTheAirURL = ");
    Serial.println(overTheAirURL);

    HTTPClient http;
    http.begin(overTheAirURL);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        Serial.println("Bad httpCode");
        return;
    }
    int contentLength = http.getSize();
    if (contentLength <= 0)
    {
        Serial.println("No contentLength");
        return;
    }
    bool canBegin = Update.begin(contentLength);
    if (!canBegin)
    {
        Serial.println("Can't begin update");
        return;
    }
    Client &client = http.getStream();
    int written = Update.writeStream(client);
    if (written != contentLength)
    {
        Serial.println("Bad contentLength");
        return;
    }
    if (!Update.end())
    {
        Serial.println("Update not ended");
        return;
    }
    if (!Update.isFinished())
    {
        Serial.println("Update not finished");
        return;
    }

    Serial.println("Update OK");
    ESP.restart();
}

// Restart ESP
BLYNK_WRITE(V0)
{
    if (param.asInt())
    {
        Blynk.virtualWrite(V0, false);

        // TODO: refactor, odstranit? https://github.com/espressif/arduino-esp32/issues/1563#issuecomment-401560601
        Serial.println("Check I2C");
        if (!digitalRead(SDA) || !digitalRead(SCL))
        { // bus in busy state
            log_w("invalid state sda=%d, scl=%d\n", digitalRead(SDA), digitalRead(SCL));
            Serial.print("invalid state SDA: ");
            Serial.println(digitalRead(SDA));
            Serial.print("invalid state SCL: ");
            Serial.println(digitalRead(SCL));
            digitalWrite(SDA, HIGH);
            digitalWrite(SCL, HIGH);
            delayMicroseconds(5);
            digitalWrite(SDA, HIGH);
            for (uint8_t a = 0; a < 9; a++)
            {
                delayMicroseconds(5);
                digitalWrite(SCL, LOW);
                delayMicroseconds(5);
                digitalWrite(SCL, HIGH);
                if (digitalRead(SDA))
                { // bus recovered, all done. resync'd with slave
                    break;
                }
            }
        }
        else
        {
            Serial.println("I2C OK");
        }
        Serial.println("Restarting, bye..");
        ESP.restart();
    }
}

// Turn on/off alarm notifications
BLYNK_WRITE(V30)
{
    alarmEnabledNotifications = param.asInt();
    Serial.println("Alarm notifications was " + String(alarmEnabledNotifications ? "enabled" : "disabled"));
}

int deepSleepIntervalStatic = 0;

// deep sleep interval in seconds
BLYNK_WRITE(V3)
{
    if (param.asInt())
    {
        deepSleepIntervalStatic = param.asInt();
        Serial.println("Deep sleep interval was set to: " + String(deepSleepIntervalStatic));
    }
}

// Turn on/off alarm
BLYNK_WRITE(V31)
{
    alarmIsEnabled = param.asInt();
    Serial.println("Alarm was " + String(alarmIsEnabled ? "enabled" : "disabled"));
}

void InternetConnection::initialize()
{
    // true if is actual alarm - run Blynk.run
    isAlarm = false;
}

bool InternetConnection::initializeConnection()
{
    int connAttempts = 0;
    Serial.println("\r\nTry connecting to: " + String(settings.wifiSSID));

    // save battery power, set lowest WiFi power
    WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);
    delay(1);

    // try config - quicker for WiFi connection
    // WiFi.config(settings.ip, settings.gateway, settings.subnet, settings.gateway);

    WiFi.begin(settings.wifiSSID, settings.wifiPassword);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        if (connAttempts > 20)
        {
            Serial.println("Error - couldn't connect to WIFI");
            return false;
        }

        connAttempts++;
    }

    delay(1000);

    Serial.println("\r\nConnecting to WIFI OK, connnecting to Blynk");

    if (!Blynk.connected())
    {
        Blynk.config(settings.blynkAuth);
        // timeout 6sec
        Blynk.connect(2000);
    }

    return Blynk.connected();
}

void InternetConnection::disconnect()
{
    if (WiFi.isConnected())
    {
        Blynk.disconnect();
        WiFi.disconnect(true);

        Serial.println("Disconnected OK");
    }
    else
    {
        Serial.println("Already disconnected");
    }
}

void InternetConnection::sendDataToBlynk(
    MeteoData meteoData,
    PowerController powerController,
    // GyroscopeController gyroscopeController,
    MagneticLockController magneticLockController)
{
    // create data to send to Blynk
    if (Blynk.connected())
    {
        // solar power data
        Blynk.virtualWrite(V8, powerController.sensor_solar.loadVoltage);
        Blynk.virtualWrite(V9, powerController.sensor_solar.current_mA);
        Blynk.virtualWrite(V10, powerController.sensor_solar.power_mW);

        // battery power data
        Blynk.virtualWrite(V41, powerController.sensor_battery.loadVoltage);
        Blynk.virtualWrite(V42, powerController.sensor_battery.current_mA);
        Blynk.virtualWrite(V43, powerController.sensor_battery.power_mW);

        // set SDA/SCL status
        setI2CStatusVersion();

        // magnetic locks data
        setMagneticLockControllerDataToBlynk(magneticLockController);

        // outdoor temperature sensor
        Blynk.virtualWrite(V22, meteoData.sensorOutdoor.humidity);
        Blynk.virtualWrite(V23, meteoData.sensorOutdoor.temperature);
        Blynk.virtualWrite(V24, meteoData.sensorOutdoor.pressure);

        // set alarm info
        setAlarmInfoToBlynk();

        // WIFI info
        Blynk.virtualWrite(V39, "IP: " + WiFi.localIP().toString() + "|G: " + WiFi.gatewayIP().toString() + "|S: " + WiFi.subnetMask().toString() + "|DNS: " + WiFi.dnsIP().toString());
        Blynk.virtualWrite(V40, WiFi.RSSI());

        if (deepSleepIntervalStatic > 0)
        {
            deepSleepInterval = deepSleepIntervalStatic;
        }
        Blynk.virtualWrite(V4, String(deepSleepInterval));
        Serial.println("Sending data to Blynk - DONE");
    }
    else
    {
        Serial.println("Blynk is not connected");
    }
}

void InternetConnection::setI2CStatusVersion()
{
    // I2C status - SDA and SCL
    bool SDAisOK = digitalRead(SDA);
    bool SCLsOK = digitalRead(SCL);

    String message = (SDAisOK ? String("SDA OK, ") : String("SDA error, ")) +
                     (SCLsOK ? String("SCL OK, ") : String("SCL error, ")) +
                     String("Version: ") + settings.version;

    Blynk.virtualWrite(V17, message);
}

void InternetConnection::setMagneticLockControllerDataToBlynk(MagneticLockController magneticLockController)
{
    Blynk.virtualWrite(V19, magneticLockController.sensorA.status);
    Blynk.virtualWrite(V20, magneticLockController.sensorB.status);
    Blynk.virtualWrite(V21, magneticLockController.sensorC.status);
    setAlarmInfoToBlynk();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// ALARM SECTION
////////////////////////////////////////////////////////////////////////////////////////////////////

void InternetConnection::setAlarmInfoToBlynk()
{
    Blynk.virtualWrite(V32, isAlarm ? "AKTUÁLNÍ ALARM!" : "OK");
    Blynk.virtualWrite(V33, alarmEnabledNotifications ? "Alarm notifikace zapnuty" : "Alarm notifikace vypnuty");
    Blynk.virtualWrite(V34, alarmIsEnabled ? "Alarm zapnut" : "Alarm vypnut");

    if (isAlarm)
    {
        Serial.println("\n ALARM \n");
    }
}

void InternetConnection::blynkRunIfAlarm()
{
    if (alarmIsEnabled && isAlarm)
    {
        Blynk.run();
    }
}

void InternetConnection::setMagneticLockControllerDataToBlynkIfAlarm(MagneticLockController magneticLockController)
{
    if (isAlarm)
    {
        setMagneticLockControllerDataToBlynk(magneticLockController);
    }
}

void InternetConnection::alarmMagneticController(MagneticLockController magneticLockController)
{
    if (!alarmIsEnabled)
    {
        return;
    }

    Serial.println("\n!!! Magnetic alarm !!!\n");

    if (!Blynk.connected())
    {
        initializeConnection();
    }

    if (Blynk.connected())
    {
        setMagneticLockControllerDataToBlynk(magneticLockController);
    }
    else
    {
        // TODO: No Blynk connection, try send SMS or call?
        // res = modem.sendSMS(SMS_TARGET, String("Hello from ") + imei);
        // res = modem.callNumber(CALL_TARGET);
        Serial.println("ALARM but can't connected to Blynk");
    }
}

/// Println message to serial and Blynk terminal
void InternetConnection::printlnToTerminal(String message)
{
    Serial.println(message);
}