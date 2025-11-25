#include "event_logger.h"
#include "time_utils.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// Internal helper: Calculate event age in hours
static unsigned long get_event_age_hours(const String& timestamp) {
    // Simple age calculation based on current time
    // For production, implement proper ISO8601 parsing
    return 0; // Placeholder - events won't be deleted by age in this simple implementation
}

bool init_event_logger() {
    Serial.println("[EVENT_LOG] Initializing event logger...");
    
    // Mount SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("[EVENT_LOG] ERROR: Failed to mount SPIFFS");
        return false;
    }
    
    // Check if log file exists
    if (!SPIFFS.exists(EVENT_LOG_FILE_PATH)) {
        Serial.println("[EVENT_LOG] Creating new log file");
        File file = SPIFFS.open(EVENT_LOG_FILE_PATH, FILE_WRITE);
        if (!file) {
            Serial.println("[EVENT_LOG] ERROR: Failed to create log file");
            return false;
        }
        
        // Initialize with empty JSON array
        file.println("[");
        file.println("]");
        file.close();
        
        Serial.println("[EVENT_LOG] Log file created successfully");
        return true;
    }
    
    // Log file exists - check size and clean up if needed
    size_t file_size = get_log_file_size();
    Serial.printf("[EVENT_LOG] Existing log file: %d bytes\n", file_size);
    
    if (file_size > EVENT_LOG_MAX_SIZE_BYTES) {
        Serial.println("[EVENT_LOG] Log file exceeds size limit, performing cleanup");
        cleanup_old_events();
    }
    
    Serial.println("[EVENT_LOG] Event logger initialized successfully");
    return true;
}

bool log_event(const String& message, const String& context) {
    // Ensure SPIFFS is mounted
    if (!SPIFFS.begin(false)) {
        Serial.println("[EVENT_LOG] ERROR: SPIFFS not mounted");
        return false;
    }
    
    // Read existing log file
    File file = SPIFFS.open(EVENT_LOG_FILE_PATH, FILE_READ);
    if (!file) {
        Serial.println("[EVENT_LOG] ERROR: Cannot open log file for reading");
        return false;
    }
    
    String fileContent = file.readString();
    file.close();
    
    // Parse existing JSON array
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, fileContent);
    
    if (error) {
        Serial.printf("[EVENT_LOG] ERROR: JSON parse failed: %s\n", error.c_str());
        // Try to recover by creating new file
        file = SPIFFS.open(EVENT_LOG_FILE_PATH, FILE_WRITE);
        if (file) {
            file.println("[");
            file.println("]");
            file.close();
        }
        return false;
    }
    
    // Create new event object
    JsonObject newEvent = doc.add<JsonObject>();
    newEvent["ts"] = get_current_timestamp();
    newEvent["lvl"] = "ERROR";
    newEvent["msg"] = message;
    if (context.length() > 0) {
        newEvent["ctx"] = context;
    }
    
    // Write updated JSON back to file
    file = SPIFFS.open(EVENT_LOG_FILE_PATH, FILE_WRITE);
    if (!file) {
        Serial.println("[EVENT_LOG] ERROR: Cannot open log file for writing");
        return false;
    }
    
    serializeJsonPretty(doc, file);
    file.close();
    
    Serial.printf("[EVENT_LOG] ✅ Logged: %s", message.c_str());
    if (context.length() > 0) {
        Serial.printf(" (%s)", context.c_str());
    }
    Serial.println();
    
    // Check if cleanup is needed
    if (get_log_file_size() > EVENT_LOG_MAX_SIZE_BYTES) {
        Serial.println("[EVENT_LOG] Size limit exceeded, cleaning up...");
        cleanup_old_events();
    }
    
    return true;
}

bool cleanup_old_events() {
    Serial.println("[EVENT_LOG] Starting cleanup...");
    
    // Read current log file
    File file = SPIFFS.open(EVENT_LOG_FILE_PATH, FILE_READ);
    if (!file) {
        Serial.println("[EVENT_LOG] ERROR: Cannot open log file");
        return false;
    }
    
    String fileContent = file.readString();
    file.close();
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, fileContent);
    
    if (error) {
        Serial.printf("[EVENT_LOG] ERROR: JSON parse failed: %s\n", error.c_str());
        return false;
    }
    
    JsonArray events = doc.as<JsonArray>();
    int originalCount = events.size();
    
    // If still over size limit after time-based cleanup, remove oldest events
    // Keep only the newest 50% of events
    int targetCount = originalCount;
    size_t currentSize = fileContent.length();
    
    if (currentSize > EVENT_LOG_MAX_SIZE_BYTES) {
        targetCount = originalCount / 2;
        if (targetCount < 5) targetCount = 5; // Keep at least 5 events
        
        Serial.printf("[EVENT_LOG] Removing oldest events: %d -> %d\n", originalCount, targetCount);
        
        // Create new array with newest events
        JsonDocument newDoc;
        JsonArray newEvents = newDoc.to<JsonArray>();
        
        // Copy newest events (from the end of the array)
        int startIndex = originalCount - targetCount;
        for (int i = startIndex; i < originalCount; i++) {
            newEvents.add(events[i]);
        }
        
        // Write cleaned up log
        file = SPIFFS.open(EVENT_LOG_FILE_PATH, FILE_WRITE);
        if (!file) {
            Serial.println("[EVENT_LOG] ERROR: Cannot write cleaned log");
            return false;
        }
        
        serializeJsonPretty(newDoc, file);
        file.close();
        
        Serial.printf("[EVENT_LOG] Cleanup complete: removed %d events\n", originalCount - targetCount);
    } else {
        Serial.println("[EVENT_LOG] No cleanup needed");
    }
    
    return true;
}

size_t get_log_file_size() {
    if (!SPIFFS.exists(EVENT_LOG_FILE_PATH)) {
        return 0;
    }
    
    File file = SPIFFS.open(EVENT_LOG_FILE_PATH, FILE_READ);
    if (!file) {
        return 0;
    }
    
    size_t size = file.size();
    file.close();
    return size;
}

String read_all_events() {
    if (!SPIFFS.exists(EVENT_LOG_FILE_PATH)) {
        return "[]";
    }
    
    File file = SPIFFS.open(EVENT_LOG_FILE_PATH, FILE_READ);
    if (!file) {
        return "[]";
    }
    
    String content = file.readString();
    file.close();
    return content;
}

bool clear_all_events() {
    Serial.println("[EVENT_LOG] Clearing all events...");
    
    // Delete old file
    if (SPIFFS.exists(EVENT_LOG_FILE_PATH)) {
        SPIFFS.remove(EVENT_LOG_FILE_PATH);
    }
    
    // Create new empty log
    File file = SPIFFS.open(EVENT_LOG_FILE_PATH, FILE_WRITE);
    if (!file) {
        Serial.println("[EVENT_LOG] ERROR: Cannot create new log file");
        return false;
    }
    
    file.println("[");
    file.println("]");
    file.close();
    
    Serial.println("[EVENT_LOG] All events cleared");
    return true;
}

int get_event_count() {
    if (!SPIFFS.exists(EVENT_LOG_FILE_PATH)) {
        return 0;
    }
    
    File file = SPIFFS.open(EVENT_LOG_FILE_PATH, FILE_READ);
    if (!file) {
        return 0;
    }
    
    String content = file.readString();
    file.close();
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);
    
    if (error) {
        return 0;
    }
    
    return doc.as<JsonArray>().size();
}
