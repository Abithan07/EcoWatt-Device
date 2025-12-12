#ifndef DISABLE_UNUSED_PERIPHERALS_H
#define DISABLE_UNUSED_PERIPHERALS_H

#include <Arduino.h>
#include "driver/ledc.h"
#include "driver/rmt.h"
#include "driver/touch_pad.h"

/**
 * PeripheralKiller - Conservative power-saving peripheral disabling
 * 
 * CONSERVATIVE APPROACH:
 * - Only disables peripherals that are explicitly unused
 * - Does NOT disable SPI (WiFi depends on it)
 * - Does NOT disable I2C (may be used by sensors)
 * - Does NOT disable ADC (may be used for voltage monitoring)
 * - Does NOT disable Bluetooth (btStop() on uninitialized BT can crash)
 * 
 * SAFE TO DISABLE:
 * - Touch sensors (not used in this application)
 * - PWM/LEDC (not controlling any outputs)
 * - RMT (not using WS2812 LEDs or IR)
 */
class PeripheralKiller {
public:
    static void disableAll();
    static bool checkAllDisabled();

private:
    static void disablePWM();
    static void disableRMT();
    static void disableTouchSensors();

    static bool checkPWM();
    static bool checkRMT();
    static bool checkTouchSensors();
};

#endif