#include "decoder.h"

std::vector<uint16_t> decodeResponse(const String& responseFrame) {
    std::vector<uint16_t> registerValues;

    int frameLength = responseFrame.length() / 2;
    uint8_t responseBytes[frameLength];
    for (int i = 0; i < frameLength; ++i) {
        responseBytes[i] = strtoul(responseFrame.substring(i * 2, i * 2 + 2).c_str(), nullptr, 16);
    }

    uint8_t byteCount = responseBytes[2];

    if (frameLength < 3 + byteCount + 2) { // 3 bytes (Slave Address, Function Code, Byte Count) + Data + 2 bytes CRC
        Serial.println("ERROR: Invalid frame length or malformed frame.");
        return registerValues;
    }

    for (int i = 0; i < byteCount; i += 2) {
        uint16_t registerValue = (responseBytes[3 + i] << 8) | responseBytes[3 + i + 1];
        registerValues.push_back(registerValue);
    }

    return registerValues;
}