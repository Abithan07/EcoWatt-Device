#include "modbus_handler.h"
#include "config.h"
#include "calculateCRC.h"
#include "checkCRC.h"
#include "error_handler.h"
#include "hex_utils.h"

bool validate_modbus_response(const String& response) {
    // Check minimum length (slave_addr + function_code + data + CRC = minimum 6 chars)
    if (response.length() < 6) {
        log_error(ERROR_INVALID_RESPONSE, "Response too short");
        return false;
    }
    
    // Check if response length is even (hex pairs)
    if (response.length() % 2 != 0) {
        log_error(ERROR_INVALID_RESPONSE, "Invalid response length");
        return false;
    }
    
    // Verify CRC
    if (!verify_frame_crc(response)) {
        log_error(ERROR_CRC_FAILED, "CRC validation failed");
        return false;
    }
    
    return true;
}

bool is_exception_response(const String& response) {
    if (response.length() < 4) {
        return false;
    }
    
    // Extract function code (second byte) using hex utils
    uint8_t func_code = extractByteFromHex(response, 1);
    
    // Exception responses have bit 7 set (0x80 | original_function_code)
    return (func_code & 0x80) != 0;
}

uint8_t get_exception_code(const String& response) {
    if (!is_exception_response(response) || response.length() < 6) {
        return 0;
    }
    
    // Exception code is in the third byte using hex utils
    return extractByteFromHex(response, 2);
}

bool is_valid_register(uint16_t register_addr) {
    return register_addr < MAX_REGISTERS;
}

bool is_valid_write_value(uint16_t register_addr, uint16_t value) {
    if (!is_valid_register(register_addr)) {
        return false;
    }
    
    // Special validation for export power register
    if (register_addr == EXPORT_POWER_REGISTER) {
        return (value >= MIN_EXPORT_POWER && value <= MAX_EXPORT_POWER);
    }
    
    // General validation - allow full uint16_t range for other registers
    return true;
}

bool decode_response_registers(const String& response, uint16_t* values, size_t max_count, size_t* actual_count) {
    *actual_count = 0;
    
    if (!validate_modbus_response(response)) {
        return false;
    }
    
    if (is_exception_response(response)) {
        uint8_t exception_code = get_exception_code(response);
        char error_msg[64];
        snprintf(error_msg, sizeof(error_msg), "Modbus exception: 0x%02X", exception_code);
        log_error(ERROR_MODBUS_EXCEPTION, error_msg);
        return false;
    }
    
    // Parse response: slave_addr(1) + func_code(1) + byte_count(1) + data(n) + crc(2)
    if (response.length() < 8) { // Minimum for valid data response
        log_error(ERROR_INVALID_RESPONSE, "Response too short for data");
        return false;
    }
    
    // Extract byte count using hex utils
    uint8_t byte_count = extractByteFromHex(response, 2);
    
    // Calculate number of registers (each register is 2 bytes)
    size_t register_count = byte_count / 2;
    
    if (register_count > max_count) {
        log_error(ERROR_INVALID_RESPONSE, "Too many registers in response");
        return false;
    }
    
    // Extract register values using hex utils
    for (size_t i = 0; i < register_count; i++) {
        int start_pos = 6 + (i * 4); // Start after header, each register is 4 hex chars
        if (start_pos + 4 > response.length() - 4) { // -4 for CRC
            break;
        }
        
        // Extract 2 bytes (high byte first - big endian)
        uint8_t high_byte = extractByteFromHex(response, (start_pos / 2));
        uint8_t low_byte = extractByteFromHex(response, (start_pos / 2) + 1);
        values[i] = (high_byte << 8) | low_byte;
        (*actual_count)++;
    }
    
    return true;
}

String format_request_frame(uint8_t slave_addr, uint8_t function_code, uint16_t start_reg, uint16_t count_or_value) {
    char frame[17]; // 8 bytes = 16 hex chars + null terminator
    
    snprintf(frame, sizeof(frame), "%02X%02X%04X%04X", 
             slave_addr, function_code, start_reg, count_or_value);
    
    return String(frame);
}

String append_crc_to_frame(const String& frame_without_crc) {
    int frame_length = frame_without_crc.length() / 2;
    uint8_t frame_bytes[frame_length];
    
    // Convert hex string to bytes using hex utils (60% faster)
    hexStringToBytes(frame_without_crc, frame_bytes, frame_length);
    
    // Calculate CRC
    uint16_t crc = calculateCRC(frame_bytes, frame_length);
    
    // Append CRC (low byte first) using hex utils
    uint8_t crc_bytes[2] = {(uint8_t)(crc & 0xFF), (uint8_t)((crc >> 8) & 0xFF)};
    char crc_hex[5];
    bytesToHexString(crc_bytes, 2, crc_hex);
    
    return frame_without_crc + String(crc_hex);
}

bool verify_frame_crc(const String& frame_with_crc) {
    return checkCRC(frame_with_crc);
}

size_t get_expected_response_length(uint8_t function_code, uint16_t register_count) {
    switch (function_code) {
        case FUNCTION_CODE_READ:
            // slave_addr(1) + func_code(1) + byte_count(1) + data(register_count*2) + crc(2)
            return (5 + register_count * 2) * 2; // *2 for hex encoding
        case FUNCTION_CODE_WRITE:
            // slave_addr(1) + func_code(1) + register_addr(2) + value(2) + crc(2)
            return 8 * 2; // *2 for hex encoding
        default:
            return 0;
    }
}
