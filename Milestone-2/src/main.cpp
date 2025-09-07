#include <Arduino.h>
#include <wifi_manager.h>
#include <api_client.h>
#include <calculateCRC.h>
#include <checkCRC.h>
#include <decoder.h>
#include <requestFrame_generator.h>
#include <vector>

const unsigned long responseTimeout = 2000;
const int maxRetries = 3;

unsigned long pollTime = 5000;
unsigned long writeTime = 25000;
unsigned long uploadTime = 30000;

// Define gains and units for each register
const float gains[] = {10, 10, 100, 10, 10, 10, 10, 10, 1, 1};
const char* units[] = {"V", "A", "Hz", "V", "V", "A", "A", "°C", "%", "W"};


// String requestframeWithoutCRC_read = "110300000003";
std::vector<uint16_t> read_registers = {0x0000, 0x0001, 0x0002}; //  array of registers
uint8_t slaveAddress= 0x11;
String requestframeWithoutCRC_read;
String endpoint_read = "read";
uint8_t functionCode_read = 0x03;
String requestFrame_read;

// String requestFrameWithoutCRC_write = "110600080010";
std::vector<uint16_t> write_registers = {0x0008, 0x0010}; //  array of one register to write and its value
String requestFrameWithoutCRC_write;
String endpoint_write = "write";
uint8_t functionCode_write = 0x06;
String requestFrame_write;

std::vector<String> sampledRawData;

// Dummy function to upload data to the dashboard
void uploadToDashboard(const std::vector<String>& data) {
    Serial.println("Uploading data to dashboard...");
    for (const auto& item : data) {
        Serial.println(item);
    }
    Serial.println("Upload complete.");
}


String sendRequestWithRetry(const String& endpoint, const String& frame) {
    int retryCount = 0;
    String result;
    while (retryCount < maxRetries) {
        unsigned long startTime = millis();
        result = api_send_request(endpoint, frame);

        while (result.isEmpty() && millis() - startTime < responseTimeout) {
            delay(10);
        }

        if (!result.isEmpty()) {
            return result;
        }

        Serial.println("Timeout: No response received. Retrying...");
        retryCount++;
    }

    Serial.println("Error: Maximum retries reached. Request failed.");
    return "Failed";
}


// Function to append CRC to the request frame
String appendCRC(const String& requestFrameWithoutCRC) {
    int frameLength = requestFrameWithoutCRC.length() / 2;
    uint8_t frameBytes[frameLength];

    // Convert the hex string to a byte array
    for (int i = 0; i < frameLength; ++i) {
        frameBytes[i] = strtoul(requestFrameWithoutCRC.substring(i * 2, i * 2 + 2).c_str(), nullptr, 16);
    }

    uint16_t crc = calculateCRC(frameBytes, frameLength);

    // Convert CRC to hex string
    char crcHex[5];
    snprintf(crcHex, sizeof(crcHex), "%02X%02X", crc & 0xFF, (crc >> 8) & 0xFF);

    return requestFrameWithoutCRC + String(crcHex);
}


void setup() {
  Serial.begin(115200);
  Serial.println("EcoWatt!!!");

  // Generate Read Request Frame
  requestframeWithoutCRC_read = generateRequestFrame(slaveAddress, functionCode_read, read_registers);
  Serial.print("Request Frame for Read without CRC: ");
  Serial.println(requestframeWithoutCRC_read);

  // Calculate CRC for the requestframeWithoutCRC_read
  requestFrame_read = appendCRC(requestframeWithoutCRC_read);
  Serial.print("Request Frame for Read with CRC: ");
  Serial.println(requestFrame_read);

  // Generate Write Request Frame
  requestFrameWithoutCRC_write = generateRequestFrame(slaveAddress, functionCode_write, write_registers);
  Serial.print("Request Frame for Write without CRC: ");
  Serial.println(requestFrameWithoutCRC_write);

  // Calculate CRC for the requestFrameWithoutCRC_write
  requestFrame_write = appendCRC(requestFrameWithoutCRC_write);
  Serial.print("Request Frame for Write with CRC: ");
  Serial.println(requestFrame_write);

  if (!wifi_init()) {
        Serial.println("Failed to initialize WiFi. Restarting in 5 seconds...");
        delay(5000);
        ESP.restart();
    }
  
  // Test api_client
  if (api_init()) {
    Serial.println("API client initialized successfully.");
  } else {
    Serial.println("API client initialization failed.");
  }
}

void loop() {
  static unsigned long lastRunTime_read = 0;
  static unsigned long lastRunTime_write = 0;
  static unsigned long lastUploadTime = 0;
  unsigned long currentTime = millis();

  // Check if pollTime has passed
  if (currentTime - lastRunTime_read >= pollTime) {
    lastRunTime_read = currentTime;

    if (requestFrame_read) {
      Serial.print("Request Frame for Read: ");
      Serial.println(requestFrame_read);

      String result = sendRequestWithRetry(endpoint_read, requestFrame_read);
      Serial.print("API Response for Read request: ");
      Serial.println(result);

      // CRC validation
      if (result != "Failed" && checkCRC(result)) {

        // Store the raw value in sampledRawData
        sampledRawData.push_back(result);

        Serial.println("CRC check PASSED for Read request");
        // Decode response
        Serial.println("Decoding response frame: " + result);
        std::vector<uint16_t> registerValues = decodeResponse(result);

        if (!registerValues.empty()) {
          Serial.println("Processed Register Values:");
          for (size_t i = 0; i < registerValues.size(); ++i) {
            float processedValue = registerValues[i] / gains[i];
            Serial.print("Register ");
            Serial.print(i);
            Serial.print(": ");
            Serial.print(processedValue);
            Serial.print(" ");
            Serial.println(units[i]);
          }
        } else {
            Serial.println("Decoding failed.");
        }
      } else {
        Serial.println("CRC check FAILED for Read request");
      }
    } else {
      Serial.println("Read request frame is empty.");
    }
  }

  delay(100);

  // Check if writeTime has passed
  if (currentTime - lastRunTime_write >= writeTime) {
    lastRunTime_write = currentTime;

    if (requestFrame_write) {
      Serial.print("Request Frame for Write: ");
      Serial.println(requestFrame_write);

      String result = sendRequestWithRetry(endpoint_write, requestFrame_write);
      Serial.print("API Response for Write request: ");
      Serial.println(result);

      // CRC validation
      if (result != "Failed" && checkCRC(result)) {
        Serial.println("CRC check PASSED for Write request");
      } else {
        Serial.println("CRC check FAILED for Write request");
      }
    } else {
      Serial.println("Write request frame is empty.");
    }
  }

  delay(100);

  if (currentTime - lastUploadTime >= uploadTime) {
    lastUploadTime = currentTime;

    uploadToDashboard(sampledRawData);

    sampledRawData.clear();
  }
}



