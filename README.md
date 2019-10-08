# ESP32 beehive (WiFi version)
Beehives online monitoring based on the ESP32 and others sensors. WiFi version.

> To build a project, you need to download all the necessary libraries and create the *settings.cpp* file in the *src* folder:
```c++
// Project settings
struct Settings
{
    const char *wifiSSID = "YYY";
    const char *wifiPassword = "ZZZ";
    const char *imageUploadScriptUrl = "http://example.com/upload.php";
    const char *version = "1.0.0";
    const char *firmwareVersionUrl = "http://example.com/version.txt";
    const char *firmwareBinUrl = "http://example.com/firmware.bin";
};
```

### Currents list:

* [ESP32 WROOM-32](https://www.aliexpress.com/item/ESP32-ESP-32-ESP32S-ESP-32S-CP2102-Wireless-WiFi-Bluetooth-Development-Board-Micro-USB-Dual-Core/32867696371.html)
* TODO

### Schema:
TODO
![Schema](https://github.com/vitzaoral/esp32-beehive-wifi/blob/master/schema/schema.png)

### Powering:
3v3 regulator - https://randomnerdtutorials.com/esp8266-voltage-regulator-lipo-and-li-ion-batteries/

### GMS antenna interferes with I2C bus communication:
TODO - https://electronics.stackexchange.com/questions/36906/radio-interferes-with-i2c-bus-communication

### PCB circuit:
* TODO

### Blynk:
* TODO
