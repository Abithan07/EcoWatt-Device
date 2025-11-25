#ifndef BOOT_VALIDATOR_H
#define BOOT_VALIDATOR_H

#include <Arduino.h>

/**
 * @file boot_validator.h
 * @brief FOTA Rollback System - Boot Validation and Loop Detection
 * 
 * This library provides automatic rollback functionality to prevent device
 * bricking from faulty FOTA updates. It implements:
 * - Boot loop detection (3 strikes rule)
 * - Automatic rollback to previous firmware
 * - Firmware validation after first successful upload
 * - Validation checkpoint tracking
 * 
 * @author EcoWatt Team
 * @date 2025-11-26
 */

/**
 * @brief Check for boot loops and trigger rollback if needed
 * 
 * This function increments a boot counter on every restart. If the device
 * fails to boot successfully 3 times in a row, it automatically rolls back
 * to the previous firmware partition.
 * 
 * Call this function FIRST in setup() before any other initialization.
 * 
 * Boot Loop Detection:
 * - Boot 1: counter = 1
 * - Boot 2: counter = 2  
 * - Boot 3: counter = 3 → ROLLBACK TRIGGERED
 * - After rollback: counter = 0
 * 
 * @note Uses NVS Preferences namespace "boot" to store boot_count
 * @note Resets boot counter to 0 after successful rollback
 */
void check_boot_loop();

/**
 * @brief Validate the boot partition and detect new firmware
 * 
 * This function checks if the current firmware is in PENDING_VERIFY state,
 * which indicates a new FOTA update that needs validation. It starts the
 * validation process by initializing tracking variables.
 * 
 * Call this function SECOND in setup() after check_boot_loop().
 * 
 * OTA States:
 * - VALID: Previously verified firmware (safe)
 * - PENDING_VERIFY: New firmware awaiting validation
 * - INVALID: Failed validation (should not occur)
 * - ABORTED: Incomplete update
 * 
 * @note Uses NVS Preferences namespace "boot" to store validation state
 * @note Logs current partition (app0/app1) and OTA state to Serial
 */
void validate_boot_partition();

/**
 * @brief Mark a validation checkpoint during initialization
 * 
 * This function tracks critical initialization milestones to verify that
 * new firmware can successfully complete the boot process. Only active
 * when firmware is in PENDING_VERIFY state.
 * 
 * Validation Stages:
 * - Stage 0: Boot started (set by validate_boot_partition)
 * - Stage 1: WiFi connected
 * - Stage 2: Config loaded
 * - Stage 3: API initialized
 * 
 * Call this function after each critical initialization step in setup().
 * 
 * @param stage Checkpoint stage number (1-3)
 * 
 * @note Stage 0 is automatically set by validate_boot_partition()
 * @note Uses NVS Preferences namespace "boot" to store val_stage
 * @note Only updates if val_stage >= 0 (validation mode active)
 */
void mark_validation_checkpoint(int stage);

/**
 * @brief Commit new firmware as valid after successful operation
 * 
 * This function should be called after the first successful cloud upload
 * to mark the new firmware as VALID and cancel any potential rollback.
 * 
 * Call this function in the upload task after successful upload validation.
 * 
 * Actions performed:
 * - Checks if firmware is in PENDING_VERIFY state
 * - Calls esp_ota_mark_app_valid_cancel_rollback()
 * - Resets boot counter to 0
 * - Clears validation state
 * 
 * @return true if firmware was committed successfully, false otherwise
 * 
 * @note After this call, firmware state changes: PENDING_VERIFY → VALID
 * @note Uses NVS Preferences namespace "boot" to reset counters
 */
bool commit_firmware_if_pending();

/**
 * @brief Reset boot counter for new firmware validation
 * 
 * This function prepares the system for new firmware validation by
 * resetting the boot counter and clearing old validation state.
 * 
 * Call this function in FOTA code before restarting to new firmware.
 * 
 * Actions performed:
 * - Resets boot_count to 0
 * - Clears validation stage (val_stage = -1)
 * 
 * @note Uses NVS Preferences namespace "boot"
 * @note Should be called in fota.cpp before ESP.restart()
 */
void reset_boot_counter_for_new_firmware();

#endif // BOOT_VALIDATOR_H
