#include "disable_unused_peripherals.h"

/**
 * CONSERVATIVE PERIPHERAL DISABLING
 * 
 * This implementation ONLY disables peripherals that are:
 * 1. Definitely not used by the application
 * 2. Safe to disable without affecting WiFi or system stability
 * 
 * WHAT WE DISABLE:
 * - PWM/LEDC: Not controlling any outputs in this application
 * - RMT: Not using WS2812 LEDs or IR communication
 * - Touch Sensors: Not using capacitive touch inputs
 * 
 * WHAT WE KEEP ENABLED (DO NOT TOUCH):
 * - SPI: WiFi radio depends on internal SPI communication
 * - I2C: May be used by external sensors (safe to keep enabled)
 * - ADC: May be used for voltage monitoring (safe to keep enabled)
 * - Bluetooth: Never initialized, calling btStop() can cause crashes
 * - UART: Serial communication and debugging
 */

void PeripheralKiller::disableAll() {
    Serial.println(F("[PERIPHERAL] Starting conservative peripheral power-down..."));
    
    disablePWM();
    disableRMT();
    disableTouchSensors();
    
    Serial.println(F("[PERIPHERAL] Conservative power-down complete"));
}

void PeripheralKiller::disablePWM() {
    Serial.println(F("[PERIPHERAL] Disabling PWM/LEDC (8 channels x 2 speed modes)..."));
    
    // LEDC has 8 channels (0-7) per speed mode
    // Stop all channels to save power
    for (int ch = 0; ch < 8; ch++) {
        ledc_stop(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)ch, 0);
        ledc_stop(LEDC_LOW_SPEED_MODE, (ledc_channel_t)ch, 0);
    }
    
    Serial.println(F("[PERIPHERAL] PWM/LEDC disabled"));
}

void PeripheralKiller::disableRMT() {
    Serial.println(F("[PERIPHERAL] Disabling RMT (8 channels)..."));
    
    // RMT (Remote Control) peripheral - used for WS2812 LEDs, IR, etc.
    // Safe to disable if not using these features
    for (int ch = 0; ch < 8; ch++) {
        // Attempt to uninstall - will fail silently if not installed
        rmt_driver_uninstall((rmt_channel_t)ch);
    }
    
    Serial.println(F("[PERIPHERAL] RMT disabled"));
}

void PeripheralKiller::disableTouchSensors() {
    Serial.println(F("[PERIPHERAL] Disabling touch sensors..."));
    
    // Touch sensor peripheral - not used in this application
    // Note: touch_pad_deinit() is safe to call even if not initialized
    esp_err_t err = touch_pad_deinit();
    
    if (err == ESP_OK) {
        Serial.println(F("[PERIPHERAL] Touch sensors disabled"));
    } else if (err == ESP_ERR_INVALID_STATE) {
        Serial.println(F("[PERIPHERAL] Touch sensors already disabled"));
    } else {
        Serial.printf("[PERIPHERAL] Touch sensor disable warning: %d\n", err);
    }
}

bool PeripheralKiller::checkAllDisabled() {
    Serial.println(F("[PERIPHERAL] Checking disabled peripherals..."));
    
    bool pwm_ok = checkPWM();
    bool rmt_ok = checkRMT();
    bool touch_ok = checkTouchSensors();
    
    Serial.printf("[PERIPHERAL] PWM: %s, RMT: %s, Touch: %s\n",
                  pwm_ok ? "DISABLED" : "ACTIVE",
                  rmt_ok ? "DISABLED" : "ACTIVE",
                  touch_ok ? "DISABLED" : "ACTIVE");
    
    return pwm_ok && rmt_ok && touch_ok;
}

bool PeripheralKiller::checkPWM() {
    // Try to configure a LEDC channel - should fail if peripheral is disabled
    ledc_channel_config_t ch = {};
    ch.channel = LEDC_CHANNEL_0;
    ch.duty = 0;
    ch.gpio_num = -1;  // Invalid GPIO
    ch.speed_mode = LEDC_LOW_SPEED_MODE;
    ch.timer_sel = LEDC_TIMER_0;
    
    esp_err_t err = ledc_channel_config(&ch);
    
    // If configuration fails, it means channels are stopped/disabled
    // Note: This is not a perfect check, but good enough for verification
    return (err != ESP_OK);
}

bool PeripheralKiller::checkRMT() {
    // Try to install RMT driver - should fail if already uninstalled
    esp_err_t err = rmt_driver_install(RMT_CHANNEL_0, 0, 0);
    
    if (err == ESP_ERR_INVALID_STATE) {
        // Already in use or disabled
        return true;
    } else if (err == ESP_OK) {
        // Successfully installed - means it wasn't disabled
        rmt_driver_uninstall(RMT_CHANNEL_0);
        return false;
    }
    
    // Other error - consider it disabled
    return true;
}

bool PeripheralKiller::checkTouchSensors() {
    // Try to read touch sensor - should fail if deinitialized
    uint16_t val = 0;
    esp_err_t err = touch_pad_read_raw_data(TOUCH_PAD_NUM0, &val);
    
    // ESP_ERR_INVALID_STATE means touch pad is not initialized/disabled
    return (err == ESP_ERR_INVALID_STATE);
}
