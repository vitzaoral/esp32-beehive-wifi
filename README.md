# ESP32 beehive (WiFi version)
Beehives online monitoring based on the ESP32 and others sensors. WiFi version.

> To build a project, you need to download all the necessary libraries and create the *settings.cpp* file in the *src* folder:
```c++
// Project settings
#include "IPAddress.h"

struct Settings
{
    const char *wifiSSID = "YYY";
    const char *wifiPassword = "ZZZ";
    const char *imageUploadScriptUrl = "http://example.com/upload.php";
    const char *version = "1.0.0";
    const char *firmwareVersionUrl = "http://example.com/version.txt";
    const char *firmwareBinUrl = "http://example.com/firmware.bin";
    IPAddress ip = IPAddress(192, 168, 43, 223);
    IPAddress gateway = IPAddress(192, 168, 43, 1);
    IPAddress subnet = IPAddress(255, 255, 255, 0);
};
```

### Currents list:

* [ESP32 WROOM-32](https://www.aliexpress.com/item/ESP32-ESP-32-ESP32S-ESP-32S-CP2102-Wireless-WiFi-Bluetooth-Development-Board-Micro-USB-Dual-Core/32867696371.html)
* TODO

### Save battery power:
* Set CPU frequency to 30% (80Mhz) - *setCpuFrequencyMhz(80);*
* Setup lowest WiFi power - *WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);*
* Setup static IP, Gateway and DNS (quicker WiFi connection) - *WiFi.config(settings.ip, settings.gateway, settings.subnet, settings.gateway);*
* When disconnect WiFi, turn off WiFi modem - *WiFi.disconnect(true);*

### Schema:
![Schema](https://github.com/vitzaoral/esp32-beehive-wifi/blob/master/schema/schema.png)

### Powering:
3v3 regulator - https://randomnerdtutorials.com/esp8266-voltage-regulator-lipo-and-li-ion-batteries/

### GMS antenna interferes with I2C bus communication:
TODO - https://electronics.stackexchange.com/questions/36906/radio-interferes-with-i2c-bus-communication

### PCB circuit:
* TODO

### Blynk:
* TODO
