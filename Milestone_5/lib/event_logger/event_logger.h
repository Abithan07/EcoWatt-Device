#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H

#include <Arduino.h>
#include "config.h"

/**
 * @file event_logger.h
 * @brief SPIFFS-based Event Logging System
 * 
 * This library provides persistent error logging to SPIFFS for remote debugging.
 * Only ERROR events are logged (no success messages) to minimize write cycles
 * and conserve flash memory.
 * 
 * Features:
 * - Error-only logging (WiFi failures, FOTA errors, rollbacks, etc.)
 * - 24-hour retention policy
 * - 50 KB size limit with automatic rotation
 * - Timestamp-based event tracking
 * - Cloud upload support
 * 
 * Configuration constants are defined in config.h:
 * - EVENT_LOG_FILE_PATH: Path to log file in SPIFFS
 * - EVENT_LOG_MAX_SIZE_BYTES: Maximum log file size (50 KB)
 * - EVENT_LOG_RETENTION_HOURS: Log retention period (24 hours)
 * - EVENT_LOG_CLEANUP_INTERVAL_MS: Cleanup interval (1 hour)
 * 
 * @author EcoWatt Team
 * @date 2025-11-26
 */

// Event severity levels (only ERROR is logged)
enum EventLevel {
    EVENT_ERROR    // Critical errors that need attention
};

/**
 * @brief Initialize the event logging system
 * 
 * This function must be called once during system startup (in setup()).
 * It mounts SPIFFS, creates the log file if it doesn't exist, and performs
 * cleanup of old events.
 * 
 * Actions performed:
 * - Mounts SPIFFS filesystem
 * - Creates log file with JSON array structure
 * - Removes events older than 24 hours
 * - Removes oldest events if file exceeds 50 KB
 * 
 * @return true if initialization successful, false on failure
 * 
 * @note Call this BEFORE any log_event() calls
 * @note Safe to call multiple times (idempotent)
 */
bool init_event_logger();

/**
 * @brief Log an error event to SPIFFS
 * 
 * Appends an error event to the persistent log file with timestamp.
 * Only ERROR-level events are logged to minimize flash wear.
 * 
 * Event format:
 * {
 *   "ts": "2025-11-26T10:30:45Z",
 *   "lvl": "ERROR",
 *   "msg": "WiFi connection failed",
 *   "ctx": "Additional context"  // Optional
 * }
 * 
 * @param message Brief error description (e.g., "WiFi_FAIL", "FOTA_HASH_MISMATCH")
 * @param context Optional additional context (e.g., error code, details)
 * 
 * @return true if event was logged successfully, false on failure
 * 
 * @note Automatically triggers cleanup if file size exceeds 50 KB
 * @note Non-blocking operation (returns quickly)
 * 
 * @example
 * log_event("WiFi_FAIL", "SSID not found");
 * log_event("ROLLBACK", "Boot loop detected - count=3");
 * log_event("FOTA_FAIL", "Hash mismatch");
 */
bool log_event(const String& message, const String& context = "");

/**
 * @brief Clean up old events from the log file
 * 
 * Removes events older than 24 hours and enforces the 50 KB size limit.
 * Called automatically during init_event_logger() and after logging events.
 * 
 * Cleanup criteria:
 * - Remove events older than LOG_RETENTION_HOURS (24 hours)
 * - If file still > MAX_LOG_SIZE_BYTES, remove oldest events first
 * 
 * @return true if cleanup successful, false on failure
 * 
 * @note This function rewrites the entire log file (slow operation)
 * @note Called automatically when needed, rarely needs manual invocation
 */
bool cleanup_old_events();

/**
 * @brief Get current log file size
 * 
 * Returns the size of the event log file in bytes.
 * 
 * @return Size in bytes, or 0 if file doesn't exist
 */
size_t get_log_file_size();

/**
 * @brief Read all events from the log file
 * 
 * Reads the entire log file as a JSON string for upload or analysis.
 * 
 * @return JSON string containing all events, empty string on failure
 * 
 * @note May be large (up to 50 KB), use sparingly
 * @note Format: JSON array of event objects
 * 
 * @example
 * String logs = read_all_events();
 * // Upload logs to cloud...
 */
String read_all_events();

/**
 * @brief Clear all events from the log file
 * 
 * Deletes all events and resets the log file.
 * Use after successful cloud upload or for testing.
 * 
 * @return true if cleared successfully, false on failure
 * 
 * @note This is a destructive operation - cannot be undone
 * @note Creates a new empty log file after clearing
 */
bool clear_all_events();

/**
 * @brief Get number of events in the log
 * 
 * Counts the number of event objects in the log file.
 * 
 * @return Number of events, or 0 if file is empty/invalid
 */
int get_event_count();

#endif // EVENT_LOGGER_H
