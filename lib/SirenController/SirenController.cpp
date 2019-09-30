#include "SirenController.h"

// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0 0

// use 13 bit precission for LEDC timer
#define LEDC_TIMER_13_BIT 13

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ 5000

SirenController::SirenController()
{
    ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
    ledcAttachPin(BUZZER_PIN, LEDC_CHANNEL_0);
}

/// Turn siren on and off
void SirenController::runSiren()
{
    sirenOn();
    sirenOff();
}

void SirenController::sirenOn()
{
    float sinVal;
    int toneVal;
    for (byte t = 0; t < 10; t++)
    {
        for (byte x = 0; x < 180; x++)
        {
            sinVal = (sin(x * (3.1412 / 180)));
            toneVal = 3600 + (int(sinVal * 100)); // cim vyssi cislo, tim vyssi ton ale taky potisejsi
            ledcWriteTone(LEDC_CHANNEL_0, toneVal);
            delay(4); // 500 - vysoky ton
        }
    }
}

void SirenController::sirenOff()
{
    ledcWriteTone(LEDC_CHANNEL_0, 0);
}
