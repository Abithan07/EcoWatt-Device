#include "disable_unused_peripherals.h"

void PeripheralKiller::disableAll() {
    disableBluetooth();
    disableADC1();
    disableI2C();
    disableSPI();
    disablePWM();
    disableRMT();
    disableTouchSensors();
}

void PeripheralKiller::disableBluetooth() {
    btStop();
}

void PeripheralKiller::disableADC1() {
    adc_power_off();
}

void PeripheralKiller::disableI2C() {
    i2c_driver_delete(I2C_NUM_0);
    i2c_driver_delete(I2C_NUM_1);
}

void PeripheralKiller::disableSPI() {
    spi_bus_free(HSPI_HOST);
    spi_bus_free(VSPI_HOST);
}

void PeripheralKiller::disablePWM() {
    // LEDC has 8 channels (0-7) per speed mode. 
    // Iterating to 16 was incorrect and caused invalid enum values.
    for (int ch = 0; ch < 8; ch++) {
        ledc_stop(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)ch, 0);
        ledc_stop(LEDC_LOW_SPEED_MODE, (ledc_channel_t)ch, 0);
    }
}

void PeripheralKiller::disableRMT() {
    for (int ch = 0; ch < 8; ch++) {
        rmt_driver_uninstall((rmt_channel_t)ch);
    }
}

void PeripheralKiller::disableTouchSensors() {
    touch_pad_deinit();
}

bool PeripheralKiller::checkAllDisabled() {
    return checkBluetooth() &&
           checkADC1() &&
           checkI2C() &&
           checkSPI() &&
           checkPWM() &&
           checkRMT() &&
           checkTouchSensors();
}

bool PeripheralKiller::checkBluetooth() {
    return !btStarted();
}

bool PeripheralKiller::checkADC1() {
    int raw = adc1_get_raw(ADC1_CHANNEL_0);
    return (raw == ESP_ERR_INVALID_STATE);
}

bool PeripheralKiller::checkI2C() {
    esp_err_t err = i2c_set_timeout(I2C_NUM_0, 1000);
    return (err == ESP_ERR_INVALID_STATE);
}

bool PeripheralKiller::checkSPI() {
    spi_bus_config_t cfg = {};
    esp_err_t err = spi_bus_initialize(HSPI_HOST, &cfg, 0);
    if (err == ESP_ERR_INVALID_STATE) return true;
    if (err == ESP_OK) {
        spi_bus_free(HSPI_HOST);
        return false;
    }
    return false;
}

bool PeripheralKiller::checkPWM() {
    ledc_channel_config_t ch = {};
    esp_err_t err = ledc_channel_config(&ch);
    return (err == ESP_ERR_INVALID_STATE);
}

bool PeripheralKiller::checkRMT() {
    esp_err_t err = rmt_driver_install(RMT_CHANNEL_0, 0, 0);
    if (err == ESP_ERR_INVALID_STATE) return true;
    if (err == ESP_OK) {
        rmt_driver_uninstall(RMT_CHANNEL_0);
        return false;
    }
    return false;
}

bool PeripheralKiller::checkTouchSensors() {
    uint16_t val = 0;
    esp_err_t err = touch_pad_read_raw_data(TOUCH_PAD_NUM0, &val);
return (err == ESP_ERR_INVALID_STATE);
}
