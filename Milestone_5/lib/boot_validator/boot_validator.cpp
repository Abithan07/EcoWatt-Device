#include "boot_validator.h"
#include <Preferences.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"

void check_boot_loop() {
    Preferences prefs;
    prefs.begin("boot", false);
    int boot_count = prefs.getInt("boot_count", 0);
    
    boot_count++;
    Serial.printf("[BOOT] Boot count: %d\n", boot_count);
    
    if (boot_count >= 3) {
        Serial.println("[BOOT] ⚠️ BOOT LOOP DETECTED - Attempting rollback to previous firmware");
        
        const esp_partition_t* running = esp_ota_get_running_partition();
        const esp_partition_t* previous = esp_ota_get_next_update_partition(NULL);
        
        Serial.printf("[BOOT] Current partition: %s\n", running->label);
        Serial.printf("[BOOT] Rolling back to: %s\n", previous->label);
        
        esp_err_t err = esp_ota_set_boot_partition(previous);
        if (err == ESP_OK) {
            Serial.println("[BOOT] ✅ Rollback partition set successfully");
            prefs.putInt("boot_count", 0);
            prefs.end();
            Serial.println("[BOOT] Restarting to previous firmware...");
            delay(2000);
            ESP.restart();
        } else {
            Serial.printf("[BOOT] ❌ Rollback failed: %s\n", esp_err_to_name(err));
            Serial.println("[BOOT] Device may be in unrecoverable state!");
        }
    }
    
    prefs.putInt("boot_count", boot_count);
    prefs.end();
}

void validate_boot_partition() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        Serial.printf("[BOOT] Partition: %s, OTA State: ", running->label);
        
        switch (ota_state) {
            case ESP_OTA_IMG_VALID:
                Serial.println("VALID (previously verified)");
                break;
            case ESP_OTA_IMG_PENDING_VERIFY:
                Serial.println("PENDING_VERIFY (new firmware - needs validation)");
                
                // Start validation process
                Preferences prefs;
                prefs.begin("boot", false);
                prefs.putULong("val_start", millis());
                prefs.putInt("val_stage", 0);  // Stage 0: Boot started
                prefs.end();
                
                Serial.println("[BOOT] ⏳ New firmware detected - validation started");
                Serial.println("[BOOT] Firmware will be committed after first successful upload");
                break;
            case ESP_OTA_IMG_INVALID:
                Serial.println("INVALID (should not happen - already booted)");
                break;
            case ESP_OTA_IMG_ABORTED:
                Serial.println("ABORTED (incomplete update)");
                break;
            default:
                Serial.printf("UNKNOWN (%d)\n", ota_state);
                break;
        }
    } else {
        Serial.println("[BOOT] Could not read OTA state");
    }
}

void mark_validation_checkpoint(int stage) {
    Preferences prefs;
    prefs.begin("boot", false);
    
    // Only update if we're in validation mode
    if (prefs.getInt("val_stage", -1) >= 0) {
        prefs.putInt("val_stage", stage);
        
        const char* stage_names[] = {
            "Boot started",
            "WiFi connected",
            "Config loaded",
            "API initialized"
        };
        
        if (stage >= 0 && stage < 4) {
            Serial.printf("[BOOT] ✅ Validation checkpoint %d: %s\n", stage, stage_names[stage]);
        }
    }
    
    prefs.end();
}

bool commit_firmware_if_pending() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            Serial.println("[BOOT] ✅ First upload successful - firmware validation complete");
            Serial.println("[BOOT] Committing new firmware as valid");
            
            esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
            if (err == ESP_OK) {
                Serial.println("[BOOT] ✅ Firmware marked as VALID - rollback cancelled");
                
                // Reset boot counter - firmware is now stable
                Preferences prefs;
                prefs.begin("boot", false);
                prefs.putInt("boot_count", 0);
                prefs.putInt("val_stage", -1);  // Clear validation state
                prefs.end();
                
                Serial.println("[BOOT] ✅ Boot counter reset - firmware fully validated");
                return true;
            } else {
                Serial.printf("[BOOT] ⚠️ Failed to mark firmware valid: %s\n", esp_err_to_name(err));
                return false;
            }
        }
    }
    
    return false;  // Not in PENDING_VERIFY state
}

void reset_boot_counter_for_new_firmware() {
    Serial.println("[FOTA] Resetting boot counter for new firmware validation");
    Preferences prefs;
    prefs.begin("boot", false);
    prefs.putInt("boot_count", 0);      // Reset to 0 so new firmware can track its boots
    prefs.putInt("val_stage", -1);      // Clear any old validation state
    prefs.end();
    
    Serial.println("[FOTA] New firmware will enter PENDING_VERIFY state");
    Serial.println("[FOTA] It will be validated and committed after first successful upload");
}
