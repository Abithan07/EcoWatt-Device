#include "wifi_manager.h"
#include "config.h"

bool wifi_init(void) {
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        init_time();
        return true;
    } else {
        Serial.println("\nWiFi connection failed");
        return false;
    }
}

bool wifi_is_connected(void) {
    return WiFi.status() == WL_CONNECTED;
}

void wifi_enable_modem_sleep(void) {
    // Enable modem sleep mode for WiFi power saving
    // In modem sleep, the WiFi modem is turned off between DTIM beacon intervals
    // This saves power while maintaining the WiFi connection
    if (WiFi.setSleep(true)) {
        Serial.println("WiFi modem sleep enabled");
    } else {
        Serial.println("Failed to enable WiFi modem sleep");
    }
}

void wifi_disable_modem_sleep(void) {
    // Disable modem sleep for full WiFi performance
    if (WiFi.setSleep(false)) {
        Serial.println("WiFi modem sleep disabled");
    } else {
        Serial.println("Failed to disable WiFi modem sleep");
    }
}