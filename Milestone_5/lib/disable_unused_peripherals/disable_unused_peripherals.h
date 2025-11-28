#ifndef DISABLE_UNUSED_PERIPHERALS_H
#define DISABLE_UNUSED_PERIPHERALS_H


#include <Arduino.h>
#include "driver/adc.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/rmt.h"
#include "driver/touch_pad.h"


class PeripheralKiller {
public:
static void disableAll();
static bool checkAllDisabled();


private:
static void disableBluetooth();
static void disableADC1();
static void disableI2C();
static void disableSPI();
static void disablePWM();
static void disableRMT();
static void disableTouchSensors();


static bool checkBluetooth();
static bool checkADC1();
static bool checkI2C();
static bool checkSPI();
static bool checkPWM();
static bool checkRMT();
static bool checkTouchSensors();
};


#endif