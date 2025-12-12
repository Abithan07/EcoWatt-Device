#ifndef FAULT_SIMULATOR_H
#define FAULT_SIMULATOR_H

#include <Arduino.h>

// Error types matching the API specification
typedef enum {
    FAULT_NONE,
    FAULT_EXCEPTION,
    FAULT_CRC_ERROR,
    FAULT_CORRUPT,
    FAULT_PACKET_DROP,
    FAULT_DELAY
} fault_type_t;

// Exception codes based on Modbus specification
typedef enum {
    EXCEPTION_ILLEGAL_FUNCTION = 0x01,
    EXCEPTION_ILLEGAL_DATA_ADDRESS = 0x02,
    EXCEPTION_ILLEGAL_DATA_VALUE = 0x03,
    EXCEPTION_SLAVE_DEVICE_FAILURE = 0x04,
    EXCEPTION_ACKNOWLEDGE = 0x05,
    EXCEPTION_SLAVE_DEVICE_BUSY = 0x06,
    EXCEPTION_MEMORY_PARITY_ERROR = 0x08,
    EXCEPTION_GATEWAY_PATH_UNAVAILABLE = 0x0A,
    EXCEPTION_GATEWAY_TARGET_FAILED = 0x0B
} exception_code_t;

// Fault configuration structure
typedef struct {
    fault_type_t fault_type;
    exception_code_t exception_code;  // Only used when fault_type is FAULT_EXCEPTION
    uint32_t delay_ms;                // Only used when fault_type is FAULT_DELAY
    bool enabled;
} fault_config_t;

// Initialize fault simulator
void fault_simulator_init(void);

// Enable a specific fault for the next request
// This sets the error flag via API to trigger on the next EcoWatt Device request
bool fault_simulator_trigger(fault_type_t fault_type, exception_code_t exception_code = EXCEPTION_ILLEGAL_FUNCTION, uint32_t delay_ms = 0);

// Trigger each fault type sequentially (cycles through all fault types)
bool fault_simulator_trigger_next(void);

// Reset to no fault
void fault_simulator_reset(void);

// Get current fault configuration
fault_config_t fault_simulator_get_config(void);

// Check if a fault is currently enabled
bool fault_simulator_is_enabled(void);

// Get fault type as string for logging
const char* fault_type_to_string(fault_type_t fault_type);

// Get exception code as string for logging
const char* exception_code_to_string(exception_code_t exception_code);

// Cycle through all exception codes for testing
bool fault_simulator_trigger_next_exception(void);

// Test all fault types in sequence
bool fault_simulator_trigger_test_sequence(void);

#endif
