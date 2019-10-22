#include "InternetConnection.h"
#include <BlynkSimpleEsp32.h>
#include "../../src/settings.cpp"

// SIM800L -> ESP32 wiring to UART2
// SIM800L RX -> ESP32 TX2
// SIM800L TX -> ESP32 RX2

// Hardware Serial - UART2
HardwareSerial gsmSerial(2);

Settings settings;

// variables for HTTP access
TinyGsm modem(gsmSerial);

// HTTP Clients for OTA over WiFi
WiFiClient client;

// Attach Blynk virtual serial terminal
WidgetTerminal terminal(V36);

// Siren alarm controller
SirenController sirenController;

// OTA - firmware file name for OTA update on the SPIFFS
const char firmwareFile[] = "/firmware.bin";
// OTA - Number of milliseconds to wait without receiving any data before we give up
const int otaNetworkTimeout = 30 * 1000;
// OTA - Number of milliseconds to wait if no data is available before trying again
const int otakNetworkDelay = 1000;

// alarm notifications are enabled
bool alarmEnabledNotifications = false;

// alarm is enabled
bool alarmIsEnabled = false;

// start OTA update process
bool startOTA = false;

// if siren is turn on/off
bool sirenAlarm = false;

// microphone gain
int microphoneGain = 0;

#define SDA 21
#define SCL 22

// need lot of blynk virtual pins :)
#define BLYNK_USE_128_VPINS

// Synchronize settings from Blynk server with device when internet is connected
BLYNK_CONNECTED()
{
    Blynk.syncAll();
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

// Turn on/off alarm
BLYNK_WRITE(V31)
{
    alarmIsEnabled = param.asInt();
    Serial.println("Alarm was " + String(alarmIsEnabled ? "enabled" : "disabled"));
}

// Terminal input
BLYNK_WRITE(V36)
{
    String valueFromTerminal = param.asStr();

    if (String("clear") == valueFromTerminal)
    {
        terminal.clear();
        terminal.println("CLEARED");
        terminal.flush();
    }
    else if (String("update") == valueFromTerminal)
    {
        terminal.clear();
        terminal.println("Start OTA enabled");
        terminal.flush();
        startOTA = true;
    }
    else if (valueFromTerminal != "\n" || valueFromTerminal != "\r" || valueFromTerminal != "")
    {
        terminal.println(String("unknown command: ") + valueFromTerminal);
        terminal.flush();
    }
}

// Siren input
BLYNK_WRITE(V37)
{
    sirenAlarm = param.asInt();
    Serial.println("Siren was " + String(sirenAlarm ? "enabled" : "disabled"));
}

// Microphone gain
BLYNK_WRITE(V38)
{
    microphoneGain = param.asInt();
}

void InternetConnection::initialize()
{
    // setup microphone turn off/on pins
    pinMode(MICROPHONE_A_PIN, OUTPUT);
    pinMode(MICROPHONE_B_PIN, OUTPUT);
    pinMode(MICROPHONE_C_PIN, OUTPUT);

    pinMode(MODEM_RESET_PIN, OUTPUT);
    restartModem();

    // true if is actual alarm - run Blynk.run
    isAlarm = false;
}

void InternetConnection::restartModem()
{
    Serial.println("Restarting modem");
    delay(1000);
    digitalWrite(MODEM_RESET_PIN, HIGH);
    delay(400);
    digitalWrite(MODEM_RESET_PIN, LOW);
    delay(400);

    Serial.println("Restarting modem - initial serial, factory default");
    // Set GSM module baud rate
    gsmSerial.begin(115200, SERIAL_8N1, 16, 17, false);
    modem.factoryDefault();

    Serial.println("Restarting modem - doing restart");

    if (!modem.restart())
    {
        Serial.println("Modem FAILED");
        modemReady = false;
    }
    else
    {
        Serial.println("Modem restart OK");
        modemReady = true;
        Serial.println("Modem info: " + modem.getModemInfo());
    }
}

void InternetConnection::checkIncomingCall()
{
    if (modemReady)
    {
        if (modem.callAnswer())
        {
            Serial.println("*** INCOMING CALL ***");
            processIncomingCall();
        }
    }
    else
    {
        Serial.println("Incomming call - modem not ready");
    }
}

void InternetConnection::processIncomingCall()
{
    int callTime = 30000;
    // int pauseTime = 100;

    Serial.println("Microphone A on");
    digitalWrite(MICROPHONE_A_PIN, HIGH);
    delay(callTime);
    digitalWrite(MICROPHONE_A_PIN, LOW);
    // modem.dtmfSend('A', 1000);
    // delay(pauseTime);

    // TODO: switching microphones
    // Serial.println("Microphone B on");
    // digitalWrite(MICROPHONE_B_PIN, HIGH);
    // delay(60000);
    // digitalWrite(MICROPHONE_B_PIN, LOW);
    // modem.dtmfSend('A', 1000);
    // delay(pauseTime);

    // Serial.println("Microphone C on");
    // digitalWrite(MICROPHONE_C_PIN, HIGH);
    // delay(callTime);
    // digitalWrite(MICROPHONE_C_PIN, LOW);

    Serial.println("*** BYE BYE ***");
    modem.callHangup();
}

bool InternetConnection::initializeConnection()
{
    int connAttempts = 0;
    Serial.println("\r\nTry connecting to: " + String(settings.wifiSSID));

    //save battery power, set lowest WiFi power
    WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);
    delay(1);

    // try config - quicker for WiFi connection
    WiFi.config(settings.ip, settings.gateway, settings.subnet, settings.gateway);

    WiFi.begin(settings.wifiSSID, settings.wifiPassword);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        if (connAttempts > 10)
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
        // blynk turn off siren
        if (sirenAlarm)
        {
            Blynk.virtualWrite(V37, false);
        }

        Blynk.disconnect();
        WiFi.disconnect(true);

        Serial.println("Disconnected OK");
    }
    else
    {
        Serial.println("Already disconnected");
    }

    // turn off siren alarm
    sirenAlarm = false;
}

void InternetConnection::sendDataToBlynk(
    MeteoData meteoData,
    PowerController powerController,
    GyroscopeController gyroscopeController,
    MagneticLockController magneticLockController)
{
    // create data to send to Blynk
    if (Blynk.connected())
    {
        int signalQuality = modem.getSignalQuality();
        Blynk.virtualWrite(V1, signalQuality);

        // battery data
        Blynk.virtualWrite(V2, modem.getBattPercent());
        float battVoltage = modem.getBattVoltage() / 1000.0F;
        Blynk.virtualWrite(V3, battVoltage);

        // meteo data A
        Blynk.virtualWrite(V4, meteoData.sensorA.humidity);
        Blynk.virtualWrite(V5, meteoData.sensorA.temperature);

        // meteo data B
        Blynk.virtualWrite(V6, meteoData.sensorB.humidity);
        Blynk.virtualWrite(V7, meteoData.sensorB.temperature);

        // solar power data
        Blynk.virtualWrite(V8, powerController.sensor_solar.loadVoltage);
        Blynk.virtualWrite(V9, powerController.sensor_solar.current_mA);
        Blynk.virtualWrite(V10, powerController.sensor_solar.power_mW);

        // battery power data
        Blynk.virtualWrite(V41, powerController.sensor_battery.loadVoltage);
        Blynk.virtualWrite(V42, powerController.sensor_battery.current_mA);
        Blynk.virtualWrite(V43, powerController.sensor_battery.power_mW);

        // setup signal quality decription
        getSignalQualityDescription(V11, signalQuality);

        // meteo data C
        Blynk.virtualWrite(V12, meteoData.sensorC.humidity);
        Blynk.virtualWrite(V13, meteoData.sensorC.temperature);

        // gyroscope data
        setGyroscopeControllerDataToBlynk(gyroscopeController);

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

void InternetConnection::setGyroscopeControllerDataToBlynk(GyroscopeController gyroscopeController)
{
    Blynk.virtualWrite(V14, gyroscopeController.sensorA.orientation);
    setAlarmCollor(V14, gyroscopeController.sensorA.isOk);

    Blynk.virtualWrite(V15, gyroscopeController.sensorB.orientation);
    setAlarmCollor(V15, gyroscopeController.sensorB.isOk);

    Blynk.virtualWrite(V16, gyroscopeController.sensorC.orientation);
    setAlarmCollor(V16, gyroscopeController.sensorC.isOk);

    setAlarmInfoToBlynk();
}

void InternetConnection::setMagneticLockControllerDataToBlynk(MagneticLockController magneticLockController)
{
    Blynk.virtualWrite(V19, magneticLockController.sensorA.status);
    setAlarmCollor(V19, magneticLockController.sensorA.locked);

    Blynk.virtualWrite(V20, magneticLockController.sensorB.status);
    setAlarmCollor(V20, magneticLockController.sensorB.locked);

    Blynk.virtualWrite(V21, magneticLockController.sensorC.status);
    setAlarmCollor(V21, magneticLockController.sensorC.locked);

    setAlarmInfoToBlynk();
}

// Signal quality description http://m2msupport.net/m2msupport/atcsq-signal-quality/
void InternetConnection::getSignalQualityDescription(int virtualPin, int quality)
{
    String color = "#ff0000";
    String message = "neznámá";

    if (quality < 10)
    {
        message = "mizivá";
        color = "#ff0000";
    }
    else if (quality < 15)
    {
        message = "dostačující";
        color = "#cc8400";
    }
    else if (quality < 20)
    {
        message = "dobrá";
        color = "#93bd38";
    }
    else if (quality < 25)
    {
        message = "výborná";
        color = "#008000";
    }
    else
    {
        message = "vynikající";
        color = "#008000";
    }
    Blynk.virtualWrite(virtualPin, message);

    // increases the time to send data, commented out..
    // Blynk.setProperty(virtualPin, "color", color);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// ALARM SECTION
////////////////////////////////////////////////////////////////////////////////////////////////////

void InternetConnection::setAlarmInfoToBlynk()
{
    Blynk.virtualWrite(V32, isAlarm ? "AKTUÁLNÍ ALARM!" : "OK");
    setAlarmCollor(V32, !isAlarm);

    Blynk.virtualWrite(V33, alarmEnabledNotifications ? "Alarm notifikace zapnuty" : "Alarm notifikace vypnuty");
    setAlarmCollor(V33, alarmEnabledNotifications);

    Blynk.virtualWrite(V34, alarmIsEnabled ? "Alarm zapnut" : "Alarm vypnut");
    setAlarmCollor(V34, alarmIsEnabled);

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

void InternetConnection::setAlarmCollor(int virtualPin, bool isOk)
{
    // increases the time to send data, commented out..
    // Blynk.setProperty(virtualPin, "color", String(isOk ? "#008000" : "#ff0000"));
}

void InternetConnection::setMagneticLockControllerDataToBlynkIfAlarm(MagneticLockController magneticLockController)
{
    if (isAlarm)
    {
        setMagneticLockControllerDataToBlynk(magneticLockController);
    }
}

void InternetConnection::setGyroscopeControllerDataToBlynkIfAlarm(GyroscopeController gyroscopeController)
{
    if (isAlarm)
    {
        setGyroscopeControllerDataToBlynk(gyroscopeController);
    }
}

void InternetConnection::checkSirenAlarm()
{
    if (sirenAlarm)
    {
        sirenController.runSiren();
        printlnToTerminal("Siréna spuštěna");
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

        if (alarmEnabledNotifications)
        {
            Blynk.notify("! ALARM ! Magnetický zámek je otevřen: " + magneticLockController.getAlarmMessage());
        }
    }
    else
    {
        // TODO: No Blynk connection, try send SMS or call?
        // res = modem.sendSMS(SMS_TARGET, String("Hello from ") + imei);
        // res = modem.callNumber(CALL_TARGET);
        Serial.println("ALARM but can't connected to Blynk");
    }
}

void InternetConnection::alarmGyroscopeController(GyroscopeController gyroscopeController)
{
    if (!alarmIsEnabled)
    {
        return;
    }

    Serial.println("\n!!! Gyroscope alarm !!!\n");

    if (!Blynk.connected())
    {
        initializeConnection();
    }

    if (Blynk.connected())
    {
        setGyroscopeControllerDataToBlynk(gyroscopeController);

        if (alarmEnabledNotifications)
        {
            Blynk.notify("! ALARM ! Gyroskop není ve správné poloze: " + gyroscopeController.getAlarmMessage());
        }
    }
    else
    {
        // TODO: No Blynk connection, try send SMS or call?
        // res = modem.sendSMS(SMS_TARGET, String("Hello from ") + imei);
        // res = modem.callNumber(CALL_TARGET);
        Serial.println("ALARM but can't connected to Blynk");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// OTA SECTION
////////////////////////////////////////////////////////////////////////////////////////////////////

void InternetConnection::checkNewVersionAndUpdate()
{
    Serial.println("Checking new firmware version..");
    if (!startOTA)
    {
        Serial.println("OTA - not setted");
        return;
    }
    else
    {
        startOTA = false;
    }

    printlnToTerminal("Start OTA, check internet connection");

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi is not connected!");
        return;
    }

    HTTPClient http;
    http.begin(client, settings.firmwareVersionUrl);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        String version = http.getString();
        Serial.println("Version: " + version);

        if (String(settings.version) != version)
        {
            printlnToTerminal("!!! START OTA UPDATE !!!");
            updateFirmware();
        }
        else
        {
            printlnToTerminal("Already on the latest version");
        }
    }
    else
    {
        printlnToTerminal("Failed verify version from server, status code: " + String(httpCode));
    }

    http.end();
    Serial.println("Restart after OTA, bye");
    delay(1000);
    ESP.restart();
}

void InternetConnection::updateFirmware()
{
    t_httpUpdate_return ret = httpUpdate.update(client, settings.firmwareBinUrl); /// FOR ESP32 HTTP OTA
    printlnToTerminal("OTA RESULT: " + String(ret));

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str()); /// FOR ESP32
        // char currentString[64];
        // sprintf(currentString, "\nHTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str()); /// FOR ESP32
        // Serial.println(currentString);
        break;

    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

    case HTTP_UPDATE_OK:
        Serial.println("OTA OK");
        break;
    }
    ESP.restart();
}

/// Println message to serial and Blynk terminal
void InternetConnection::printlnToTerminal(String message)
{
    Serial.println(message);
    terminal.println(message);
    terminal.flush();
}

void InternetConnection::setMicrophoneGain()
{
    if (modemReady)
    {
        // https://github.com/sui77/rotary-sim800/wiki/SW_ATCommands#atcmic---microphone-gain-setting
        modem.sendAT(GF("+CMIC=0,"), microphoneGain);
    }
}