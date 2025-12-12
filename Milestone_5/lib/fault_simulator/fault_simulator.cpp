#include "fault_simulator.h"
#include "config.h"
#include "api_client.h"
#include "error_handler.h"
#include "event_logger.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

// Current fault configuration
static fault_config_t current_fault = {FAULT_NONE, EXCEPTION_ILLEGAL_FUNCTION, 0, false};

// Current test sequence index
static int current_fault_index = 0;
static int current_exception_index = 0;

// Fault sequence for testing (cycles through all fault types)
static const fault_type_t fault_sequence[] = {
    FAULT_EXCEPTION,
    FAULT_CRC_ERROR,
    FAULT_CORRUPT,
    FAULT_PACKET_DROP,
    FAULT_DELAY
};

// Exception code sequence for testing
static const exception_code_t exception_sequence[] = {
    EXCEPTION_ILLEGAL_FUNCTION,
    EXCEPTION_ILLEGAL_DATA_ADDRESS,
    EXCEPTION_ILLEGAL_DATA_VALUE,
    EXCEPTION_SLAVE_DEVICE_FAILURE,
    EXCEPTION_ACKNOWLEDGE,
    EXCEPTION_SLAVE_DEVICE_BUSY,
    EXCEPTION_MEMORY_PARITY_ERROR,
    EXCEPTION_GATEWAY_PATH_UNAVAILABLE,
    EXCEPTION_GATEWAY_TARGET_FAILED
};

#define FAULT_SEQUENCE_COUNT (sizeof(fault_sequence) / sizeof(fault_sequence[0]))
#define EXCEPTION_SEQUENCE_COUNT (sizeof(exception_sequence) / sizeof(exception_sequence[0]))

void fault_simulator_init(void) {
    current_fault.fault_type = FAULT_NONE;
    current_fault.exception_code = EXCEPTION_ILLEGAL_FUNCTION;
    current_fault.delay_ms = 0;
    current_fault.enabled = false;
    current_fault_index = 0;
    current_exception_index = 0;
    
    Serial.println(F("[FAULT_SIM] Fault simulator initialized"));
    log_event("FAULT_SIM_INIT", "Fault simulator initialized");
}

bool fault_simulator_trigger(fault_type_t fault_type, exception_code_t exception_code, uint32_t delay_ms) {
    // Build JSON request body
    JsonDocument doc;
    
    // Set error type
    switch (fault_type) {
        case FAULT_EXCEPTION:
            doc["errorType"] = "EXCEPTION";
            doc["exceptionCode"] = (int)exception_code;
            doc["delayMs"] = 0;
            break;
        case FAULT_CRC_ERROR:
            doc["errorType"] = "CRC_ERROR";
            doc["exceptionCode"] = 0;
            doc["delayMs"] = 0;
            break;
        case FAULT_CORRUPT:
            doc["errorType"] = "CORRUPT";
            doc["exceptionCode"] = 0;
            doc["delayMs"] = 0;
            break;
        case FAULT_PACKET_DROP:
            doc["errorType"] = "PACKET_DROP";
            doc["exceptionCode"] = 0;
            doc["delayMs"] = 0;
            break;
        case FAULT_DELAY:
            doc["errorType"] = "DELAY";
            doc["exceptionCode"] = 0;
            doc["delayMs"] = delay_ms;
            break;
        case FAULT_NONE:
        default:
            Serial.println(F("[FAULT_SIM] Invalid fault type"));
            return false;
    }
    
    // Serialize JSON to string
    String json_body;
    serializeJson(doc, json_body);
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[FAULT_SIM] WiFi not connected"));
        log_error(ERROR_WIFI_DISCONNECTED, "WiFi not connected for fault trigger");
        return false;
    }
    
    // Send API request to set error flag using HTTPClient directly
    HTTPClient http;
    String url = String(API_BASE_URL) + "/api/user/error-flag/add";
    
    http.begin(url);
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.addHeader(F("Content-Type"), F("application/json"));
    http.addHeader(F("Authorization"), API_KEY);
    
    int http_code = http.POST(json_body);
    http.end();
    
    // Check HTTP status code (API returns 200 OK with empty body on success)
    if (http_code == HTTP_CODE_OK) {
        // Update current fault configuration
        current_fault.fault_type = fault_type;
        current_fault.exception_code = exception_code;
        current_fault.delay_ms = delay_ms;
        current_fault.enabled = true;
        
        Serial.printf("[FAULT_SIM] ✓ Triggered fault: %s", fault_type_to_string(fault_type));
        if (fault_type == FAULT_EXCEPTION) {
            Serial.printf(" (Exception: %s)", exception_code_to_string(exception_code));
        } else if (fault_type == FAULT_DELAY) {
            Serial.printf(" (Delay: %u ms)", (unsigned int)delay_ms);
        }
        Serial.println();
        
        // Log the event
        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg), "Fault triggered: %s", fault_type_to_string(fault_type));
        log_event("FAULT_TRIGGERED", log_msg);
        
        return true;
    } else if (http_code > 0) {
        Serial.printf("[FAULT_SIM] HTTP error: %d\n", http_code);
        log_error(ERROR_HTTP_FAILED, "Failed to set error flag");
        return false;
    } else {
        Serial.println(F("[FAULT_SIM] HTTP request failed"));
        log_error(ERROR_HTTP_TIMEOUT, "Failed to set error flag");
        return false;
    }
}

bool fault_simulator_trigger_next(void) {
    // Cycle through fault types
    fault_type_t next_fault = fault_sequence[current_fault_index];
    
    bool success = false;
    if (next_fault == FAULT_DELAY) {
        // Use 5000ms delay for testing
        success = fault_simulator_trigger(next_fault, EXCEPTION_ILLEGAL_FUNCTION, 5000);
    } else if (next_fault == FAULT_EXCEPTION) {
        // Use current exception in sequence
        success = fault_simulator_trigger(next_fault, exception_sequence[current_exception_index], 0);
    } else {
        success = fault_simulator_trigger(next_fault, EXCEPTION_ILLEGAL_FUNCTION, 0);
    }
    
    if (success) {
        // Move to next fault in sequence
        current_fault_index = (current_fault_index + 1) % FAULT_SEQUENCE_COUNT;
        
        Serial.printf("[FAULT_SIM] Next fault index: %d/%d\n", 
                     current_fault_index, (int)FAULT_SEQUENCE_COUNT);
    }
    
    return success;
}

bool fault_simulator_trigger_next_exception(void) {
    // Cycle through exception codes
    exception_code_t next_exception = exception_sequence[current_exception_index];
    
    bool success = fault_simulator_trigger(FAULT_EXCEPTION, next_exception, 0);
    
    if (success) {
        // Move to next exception in sequence
        current_exception_index = (current_exception_index + 1) % EXCEPTION_SEQUENCE_COUNT;
        
        Serial.printf("[FAULT_SIM] Next exception index: %d/%d\n", 
                     current_exception_index, (int)EXCEPTION_SEQUENCE_COUNT);
    }
    
    return success;
}

bool fault_simulator_trigger_test_sequence(void) {
    // This function triggers all fault types once and returns to start
    // Useful for automated testing
    
    Serial.println(F("[FAULT_SIM] Starting test sequence..."));
    Serial.printf("[FAULT_SIM] Testing fault %d/%d\n", 
                 current_fault_index + 1, (int)FAULT_SEQUENCE_COUNT);
    
    return fault_simulator_trigger_next();
}

void fault_simulator_reset(void) {
    current_fault.fault_type = FAULT_NONE;
    current_fault.enabled = false;
    
    Serial.println(F("[FAULT_SIM] Fault simulator reset"));
    log_event("FAULT_SIM_RESET", "Fault simulator reset to normal operation");
}

fault_config_t fault_simulator_get_config(void) {
    return current_fault;
}

bool fault_simulator_is_enabled(void) {
    return current_fault.enabled;
}

const char* fault_type_to_string(fault_type_t fault_type) {
    switch (fault_type) {
        case FAULT_NONE:
            return "NONE";
        case FAULT_EXCEPTION:
            return "EXCEPTION";
        case FAULT_CRC_ERROR:
            return "CRC_ERROR";
        case FAULT_CORRUPT:
            return "CORRUPT";
        case FAULT_PACKET_DROP:
            return "PACKET_DROP";
        case FAULT_DELAY:
            return "DELAY";
        default:
            return "UNKNOWN";
    }
}

const char* exception_code_to_string(exception_code_t exception_code) {
    switch (exception_code) {
        case EXCEPTION_ILLEGAL_FUNCTION:
            return "ILLEGAL_FUNCTION (0x01)";
        case EXCEPTION_ILLEGAL_DATA_ADDRESS:
            return "ILLEGAL_DATA_ADDRESS (0x02)";
        case EXCEPTION_ILLEGAL_DATA_VALUE:
            return "ILLEGAL_DATA_VALUE (0x03)";
        case EXCEPTION_SLAVE_DEVICE_FAILURE:
            return "SLAVE_DEVICE_FAILURE (0x04)";
        case EXCEPTION_ACKNOWLEDGE:
            return "ACKNOWLEDGE (0x05)";
        case EXCEPTION_SLAVE_DEVICE_BUSY:
            return "SLAVE_DEVICE_BUSY (0x06)";
        case EXCEPTION_MEMORY_PARITY_ERROR:
            return "MEMORY_PARITY_ERROR (0x08)";
        case EXCEPTION_GATEWAY_PATH_UNAVAILABLE:
            return "GATEWAY_PATH_UNAVAILABLE (0x0A)";
        case EXCEPTION_GATEWAY_TARGET_FAILED:
            return "GATEWAY_TARGET_FAILED (0x0B)";
        default:
            return "UNKNOWN";
    }
}
