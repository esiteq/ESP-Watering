// ===========================================
// DEFAULTS.H - Configuration file
// ESP-Watering System
// ===========================================

// MQTT default settings
#define DEFAULT_MQTT_HOST "mqtt.host"
#define DEFAULT_MQTT_USER "mqtt.user"
#define DEFAULT_MQTT_PASS "mqtt.pass"

// Other default settings
#define DEFAULT_MAX_WATER_TIME 300        // Default maximum watering time (5 minutes)
#define DEFAULT_DEVICE_PREFIX "watering_" // Device ID prefix
#define DEFAULT_WATER_SUFFIX ""           // Water topic suffix (e.g. "_3247" for unique topics)
#define MQTT_PORT 1883                    // Standard MQTT port

// WiFi configuration portal
#define WIFI_AP_NAME "ESP-Watering"       // WiFi access point name
#define WIFI_AP_PASS ""                   // WiFi access point password (empty = no password)
#define WIFI_PORTAL_TIMEOUT 60            // Configuration portal timeout (seconds)

// NTP settings
#define NTP_SERVER "time.nist.gov"        // NTP server
#define NTP_UPDATE_INTERVAL 28800000      // NTP update interval (8 hours in ms)

// System settings
#define SERIAL_BAUD_RATE 115200           // Serial port baud rate
#define LONG_CLICK_TIME 5000              // Long button press time (ms)