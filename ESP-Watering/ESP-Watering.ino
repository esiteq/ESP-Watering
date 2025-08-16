#include <WiFiManager.h>
#include "Button2.h"
#include <PubSubClient.h>  // MQTT library
#include <NTPClient.h>     // Time library
#include <WiFiUdp.h>       // UDP for NTP
#include <EEPROM.h>        // EEPROM library
#include "defaults.h"    // Default settings

#define RELAY_PIN D1     // Relay pin (GPIO5 - stable, no boot functions)
#define BUTTON_PIN D2    // Button pin

// EEPROM addresses (WiFiManager uses 0-200, so we use the end)
#define EEPROM_MAX_WATER_TIME_ADDR 500  // Address for max_water_time storage (4 bytes, 500-503)
#define EEPROM_SIZE 512                 // EEPROM size for initialization
// EEPROM map:
// 0-199:   WiFiManager (SSID, password, custom parameters)
// 500-503: max_water_time (int, 4 bytes)
// 504-511: reserved for future settings

// Button2 object
Button2 button;

// WiFiManager object
WiFiManager wm;

// MQTT objects
WiFiClient espClient;
PubSubClient mqtt(espClient);

// NTP objects
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, 0, NTP_UPDATE_INTERVAL);  // Update from config

// MQTT custom parameters (GLOBAL as in official example!)
WiFiManagerParameter custom_mqtt_host("mqtt_host", "MQTT Host", DEFAULT_MQTT_HOST, 40);
WiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT User", DEFAULT_MQTT_USER, 32);
WiFiManagerParameter custom_mqtt_pass("mqtt_pass", "MQTT Pass", DEFAULT_MQTT_PASS, 32);

// Relay variables
bool relayState = false;
unsigned long relayOffTime = 0;  // Time when relay should be turned off

// MQTT settings (stored after configuration)
String mqtt_host = DEFAULT_MQTT_HOST;  // Default
String mqtt_user = DEFAULT_MQTT_USER;  // Default
String mqtt_pass = DEFAULT_MQTT_PASS;  // Default
String deviceId;  // Device ID

// MQTT topics
const char* topic_water = "water";        // Main topic for commands and responses
const char* topic_water_set = "water/set"; // Topic for settings
const char* topic_water_log = "water/log"; // Topic for logging

// MQTT connection state for non-blocking retries
unsigned long lastMqttAttempt = 0;
const unsigned long mqttRetryInterval = 5000;  // 5 seconds between retries
int mqttFailCount = 0;
const int maxMqttFailCount = 10;  // Log warning after 10 failures

// Maximum watering time variable (now stored in EEPROM)
int max_water_time = DEFAULT_MAX_WATER_TIME;  // Default from config

// Simple connectivity check function
bool isConnected() {
  return (WiFi.status() == WL_CONNECTED && mqtt.connected());
}

// Function to save max_water_time to EEPROM
void saveMaxWaterTimeToEEPROM(int value) {
  EEPROM.put(EEPROM_MAX_WATER_TIME_ADDR, value);
  EEPROM.commit();
  Serial.println("EEPROM: max_water_time saved: " + String(value));
}

// Function to read max_water_time from EEPROM
int readMaxWaterTimeFromEEPROM() {
  int value;
  EEPROM.get(EEPROM_MAX_WATER_TIME_ADDR, value);
  
  Serial.print("EEPROM: Reading from address " + String(EEPROM_MAX_WATER_TIME_ADDR) + ": ");
  
  // Check if value is valid (from 1 to 3600 seconds - up to 1 hour)
  if (value < 1 || value > 3600) {
    Serial.println("Value missing or invalid (" + String(value) + ")");
    Serial.println("EEPROM: Setting default: " + String(DEFAULT_MAX_WATER_TIME) + " seconds");
    value = DEFAULT_MAX_WATER_TIME;  // Default from config
    saveMaxWaterTimeToEEPROM(value);  // Save default value
  } else {
    Serial.println(String(value) + " seconds");
    Serial.println("EEPROM: max_water_time successfully read");
  }
  
  return value;
}

// Function to determine daylight saving time in Ukraine
bool isDST() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * timeinfo = gmtime(&rawtime);
  
  int month = timeinfo->tm_mon + 1;  // tm_mon: 0-11
  int day = timeinfo->tm_mday;
  int weekday = timeinfo->tm_wday;   // 0=Sunday
  
  // Daylight saving time: last Sunday of March - last Sunday of October
  if (month < 3 || month > 10) return false;      // January, February, November, December
  if (month > 3 && month < 10) return true;       // April - September
  
  // March or October - need to check Sunday
  if (month == 3) {
    // Last Sunday of March
    int lastSunday = 31 - ((5 * 31 / 4 + 4) % 7);
    return day >= lastSunday;
  } else {
    // Last Sunday of October
    int lastSunday = 31 - ((5 * 31 / 4 + 1) % 7);
    return day < lastSunday;
  }
}

// Function to get Ukrainian time
String getUkrainianTime() {
  timeClient.update();
  
  time_t rawtime = timeClient.getEpochTime();
  if (isDST()) {
    rawtime += 10800;  // UTC+3 (daylight saving time)
  } else {
    rawtime += 7200;   // UTC+2 (standard time)
  }
  
  struct tm * timeinfo = gmtime(&rawtime);
  
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  return String(timeStr);
}

// Function to log actions to MQTT (without console spam)
void logAction(String action) {
  String logMessage = getUkrainianTime() + " : " + action;
  
  // Always log to Serial
  Serial.println("LOG: " + logMessage);
  
  // Send to MQTT if connected
  if (mqtt.connected()) {
    mqtt.publish(topic_water_log, logMessage.c_str());
    mqtt.loop();
    delay(50);  // Short delay to ensure delivery
  }
}

// Temporary empty function to avoid compilation errors (remove if not needed)
void logOfflineAction(String action) {
  // This function is deprecated - use logAction() instead
  logAction(action);
}

// Safe relay control functions with connectivity check
bool relayOn(bool bypassConnectivityCheck = false) {
  // Check connectivity before starting new watering (unless bypassed for manual button)
  if (!bypassConnectivityCheck && !isConnected()) {
    logOfflineAction("Watering start blocked - no WiFi/MQTT connection");
    Serial.println("ERROR: Cannot start watering without WiFi/MQTT connection");
    return false;  // Failed to start
  }
  
  digitalWrite(RELAY_PIN, LOW);
  relayState = true;
  Serial.println("Relay ON");
  return true;  // Successfully started
}

void relayOff() {
  // Always allow stopping watering, even without connectivity
  digitalWrite(RELAY_PIN, HIGH);
  relayState = false;
  Serial.println("Relay OFF");
}

// MQTT callback function (without extra diagnostics)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  
  String topicStr = String(topic);
  Serial.println("MQTT: " + topicStr + " = " + message);
  
  if (topicStr == "water") {
    
    if (message == "on") {
      relayOn();
      relayOffTime = millis() + (max_water_time * 1000);
      logAction("Watering started for " + String(max_water_time) + " seconds");
    }
    else if (message == "off") {
      relayOff();
      relayOffTime = 0;
      logAction("Watering stopped");
    }
    else if (message == "status") {
      String status = relayState ? "on" : "off";
      mqtt.publish(topic_water, status.c_str());
      Serial.println("Status: " + status);
    }
    else if (message == "time") {
      String ukrainianTime = getUkrainianTime();
      logAction("Current time: " + ukrainianTime);
    }
    else if (message == "max") {
      // Respond with current max_water_time value in water/log (number only)
      String maxTimeStr = String(max_water_time);
      mqtt.publish(topic_water_log, maxTimeStr.c_str());
      Serial.println("Max watering time: " + maxTimeStr + " sec");
    }
    else {
      int seconds = message.toInt();
      if (seconds > 0) {
        if (seconds > max_water_time) {
          seconds = max_water_time;
          Serial.println("WARNING: Time limited to " + String(max_water_time) + " sec");
        }
        
        relayOn();
        relayOffTime = millis() + (seconds * 1000);
        logAction("Watering started for " + String(seconds) + " seconds");
      }
      else {
        Serial.println("ERROR: Unknown command: " + message);
      }
    }
  }
  else if (topicStr == "water/set") {
    // Handle settings commands
    if (message.startsWith("max,")) {
      int newMaxTime = message.substring(4).toInt();  // Get number after "max,"
      
      if (newMaxTime >= 1 && newMaxTime <= 3600) {  // From 1 second to 1 hour
        max_water_time = newMaxTime;
        saveMaxWaterTimeToEEPROM(max_water_time);
        
        // Respond with confirmation
        String response = "max_water_time_set:" + String(max_water_time);
        mqtt.publish(topic_water, response.c_str());
        
        logAction("Max watering time changed to " + String(max_water_time) + " seconds");
      } else {
        String errorMsg = "ERROR: max_water_time can only be 1-3600 sec (attempted to set " + String(newMaxTime) + ")";
        Serial.println(errorMsg);
        
        // Respond with error in MQTT
        mqtt.publish(topic_water, ("error:invalid_max_time:" + String(newMaxTime)).c_str());
        
        logAction("Error setting max_water_time: " + String(newMaxTime) + " (allowed 1-3600)");
      }
    } else {
      Serial.println("ERROR: Unknown settings command: " + message);
    }
  }
}

// Non-blocking MQTT connection function
void connectMQTT() {
  // Check if it's time to retry
  if (millis() - lastMqttAttempt < mqttRetryInterval) {
    return;  // Not time to retry yet
  }
  
  if (!mqtt.connected() && WiFi.status() == WL_CONNECTED) {
    lastMqttAttempt = millis();
    Serial.print("MQTT: Connecting...");
    
    String clientId = "ESP-watering-" + String(ESP.getChipId(), HEX);
    
    bool connected;
    if (mqtt_user.length() > 0) {
      connected = mqtt.connect(clientId.c_str(), mqtt_user.c_str(), mqtt_pass.c_str());
    } else {
      connected = mqtt.connect(clientId.c_str());
    }
    
    if (connected) {
      Serial.println(" connected!");
      mqtt.subscribe(topic_water);
      mqtt.subscribe(topic_water_set);
      Serial.println("MQTT: Subscribed to topics: water, water/set");
      Serial.println("Commands: on, off, status, time, max, or number of seconds");
      Serial.println("Settings: mosquitto_pub -t \"water/set\" -m \"max,XXX\"");
      mqttFailCount = 0;  // Reset failure counter
    } else {
      Serial.print(" failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" will retry in 5 sec");
      
      mqttFailCount++;
      if (mqttFailCount >= maxMqttFailCount) {
        Serial.println("MQTT: Warning - " + String(mqttFailCount) + " consecutive failures, continuing to retry");
        // Note: We continue retrying indefinitely for remote devices
      }
    }
  }
}

// Short click handler - toggles relay
void shortClick(Button2& btn) {
  Serial.println("BUTTON: Short click");
  
  if (relayState) {
    relayOff();
    relayOffTime = 0;
    logAction("Watering stopped by button");
  } else {
    relayOn();
    relayOffTime = millis() + (max_water_time * 1000);
    logAction("Watering started by button for " + String(max_water_time) + " seconds");
  }
}

// Long click handler - resets WiFi
void longClick(Button2& btn) {
  unsigned int time = btn.wasPressedFor();
  Serial.println("BUTTON: Long click");
  
  if (time >= LONG_CLICK_TIME) {
    Serial.println("WIFI: Resetting settings...");
    relayOff();
    relayState = false;
    wm.resetSettings();
    Serial.println("WIFI: Settings reset! Restarting...");
    delay(1000);
    ESP.restart();
  } else {
    Serial.print("BUTTON: Long press: ");
    Serial.print(time);
    Serial.println(" ms (need >" + String(LONG_CLICK_TIME) + "ms for reset)");
  }
}

// Callback function for saving MQTT parameters
void saveParamsCallback() {
  Serial.println("CONFIG: Get Params:");
  Serial.print(custom_mqtt_host.getID());
  Serial.print(" : ");
  Serial.println(custom_mqtt_host.getValue());
  Serial.print(custom_mqtt_user.getID());
  Serial.print(" : ");
  Serial.println(custom_mqtt_user.getValue());
  Serial.print(custom_mqtt_pass.getID());
  Serial.print(" : ");
  Serial.println(custom_mqtt_pass.getValue());
}

void setup() {
    WiFi.mode(WIFI_STA);
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println();
    Serial.println("ESP-Watering System");
    
    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // Read max_water_time from EEPROM
    max_water_time = readMaxWaterTimeFromEEPROM();
    
    deviceId = DEFAULT_DEVICE_PREFIX + String(ESP.getChipId(), HEX);
    Serial.println("Device ID: " + deviceId);
    Serial.println("Max watering time: " + String(max_water_time) + " seconds");
    
    pinMode(RELAY_PIN, OUTPUT);
    relayOff();
    
    button.begin(BUTTON_PIN);
    button.setClickHandler(shortClick);
    button.setLongClickHandler(longClick);
    button.setLongClickTime(LONG_CLICK_TIME);
    Serial.println("Button configured");
    
    // WiFiManager settings for remote devices
    wm.addParameter(&custom_mqtt_host);
    wm.addParameter(&custom_mqtt_user);
    wm.addParameter(&custom_mqtt_pass);
    
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);
    wm.setSaveParamsCallback(saveParamsCallback);
    
    // For remote devices: try to connect but don't stay in config portal forever
    wm.setConnectTimeout(30);  // 30 seconds timeout for connection
    wm.setConnectRetries(3);   // Try 3 times then continue with main program
    
    if(wm.autoConnect(WIFI_AP_NAME, WIFI_AP_PASS)){
        Serial.println("WiFi connected!");
    } else {
        Serial.println("WiFi: Failed to connect, but continuing operation");
        Serial.println("Device will keep trying to connect in background");
        Serial.println("Manual control via button remains available");
    }
}

void loop() {
    wm.process();
    button.loop();
    
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqtt.connected()) {
            connectMQTT();
        } else {
            mqtt.loop();
        }
    }
    
    // Check timer
    if (relayOffTime > 0 && millis() >= relayOffTime) {
        relayOff();
        relayOffTime = 0;
        logAction("Watering stopped (timer expired)");
    }
    
    // Time update (quietly, without spam)
    static unsigned long lastTimeUpdate = 0;
    if (millis() - lastTimeUpdate > 1800000) {
        lastTimeUpdate = millis();
        if (WiFi.status() == WL_CONNECTED) {
            timeClient.update();
            String ukrainianTime = getUkrainianTime();
            logAction("Time updated: " + ukrainianTime);
        }
    }
    
    static unsigned long lastCheck = 0;
    static bool wifiConnectedShown = false;
    
    if (millis() - lastCheck > 10000) {
        lastCheck = millis();
        
        if (WiFi.status() == WL_CONNECTED) {
            if (!wifiConnectedShown) {
                mqtt_host = custom_mqtt_host.getValue();
                mqtt_user = custom_mqtt_user.getValue();
                mqtt_pass = custom_mqtt_pass.getValue();
                
                mqtt.setServer(mqtt_host.c_str(), MQTT_PORT);
                mqtt.setCallback(mqttCallback);
                
                Serial.println("WiFi connected: " + WiFi.SSID());
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());
                Serial.println("MQTT settings:");
                Serial.println("   Host: " + mqtt_host);
                Serial.println("   User: " + mqtt_user);
                Serial.println("   Pass: " + String(mqtt_pass.length() > 0 ? "***" : "(none)"));
                
                timeClient.begin();
                Serial.println("NTP client started");
                
                connectMQTT();
                wifiConnectedShown = true;
            }
        } else {
            wifiConnectedShown = false;
            Serial.println("WiFi: Configuration portal or connecting");
        }
    }
}