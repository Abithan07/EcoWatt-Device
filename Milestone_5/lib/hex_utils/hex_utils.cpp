#include "hex_utils.h"

// Hex character lookup table stored in PROGMEM (flash) to save RAM
const char HEX_CHARS_UPPER[] PROGMEM = "0123456789ABCDEF";

size_t hexStringToBytes(const String& hexStr, uint8_t* output, size_t maxLen) {
    return hexStringToBytes(hexStr.c_str(), hexStr.length(), output, maxLen);
}

size_t hexStringToBytes(const char* hexStr, size_t hexLen, uint8_t* output, size_t maxLen) {
    // Calculate number of bytes (2 hex chars = 1 byte)
    size_t len = hexLen / 2;
    if (len > maxLen) len = maxLen;
    
    // Convert each pair of hex characters to a byte
    for (size_t i = 0; i < len; i++) {
        output[i] = hexPairToByte(hexStr + (i * 2));
    }
    
    return len;
}

void bytesToHexString(const uint8_t* data, size_t len, char* output) {
    // Convert each byte to 2 hex characters
    for (size_t i = 0; i < len; i++) {
        output[i * 2] = pgm_read_byte(&HEX_CHARS_UPPER[data[i] >> 4]);
        output[i * 2 + 1] = pgm_read_byte(&HEX_CHARS_UPPER[data[i] & 0x0F]);
    }
    output[len * 2] = '\0';
}

String bytesToHexString(const uint8_t* data, size_t len) {
    String result;
    result.reserve(len * 2 + 1);
    
    char hex[3];
    for (size_t i = 0; i < len; i++) {
        snprintf(hex, 3, "%02X", data[i]);
        result += hex;
    }
    
    return result;
}

void uint16ToHex(uint16_t value, char* output) {
    output[0] = pgm_read_byte(&HEX_CHARS_UPPER[(value >> 12) & 0x0F]);
    output[1] = pgm_read_byte(&HEX_CHARS_UPPER[(value >> 8) & 0x0F]);
    output[2] = pgm_read_byte(&HEX_CHARS_UPPER[(value >> 4) & 0x0F]);
    output[3] = pgm_read_byte(&HEX_CHARS_UPPER[value & 0x0F]);
    output[4] = '\0';
}
