# ESP-Watering

A simple and smart watering system for home plants or garden beds using ESP8266 or ESP32 microcontroller.

## Features

- **Remote Control**: MQTT-based control over WiFi
- **Manual Control**: Physical button for local operation
- **Smart Safety**: Maximum watering time protection with water level monitoring
- **Water Level Detection**: Float sensor prevents dry running
- **Soil Moisture Monitoring**: Analog sensor (A0) for soil moisture tracking
- **Time Logging**: All actions logged with timestamps
- **Persistent Settings**: Configuration stored in EEPROM
- **Easy Setup**: WiFiManager for wireless configuration
- **Real-time Clock**: NTP time synchronization

## Hardware Requirements

- ESP8266 NodeMCU or ESP32 board
- 5V/12V relay module
- Push button (normally open)
- Water level float sensor (NC/NO type)
- Soil moisture sensor (analog)
- Water pump or solenoid valve
- 12V power supply + DC-DC step down converter (1A rated, 12V→5V)
- Breadboard or PCB for connections
- Connecting wires and resistors
- Project enclosure (waterproof recommended)

## Wiring Diagram

| Component | ESP8266 Pin | Notes |
|-----------|-------------|-------|
| Relay Module | D1 (GPIO5) | Controls water pump/valve |
| Button | D2 (GPIO4) | Manual control (with pull-up) |
| Water Level Sensor | D5 (GPIO14) | Tank water detection (LOW=empty, HIGH=ok) |
| Soil Moisture Sensor | A0 | Analog soil moisture reading (0-1024) |
| Power Supply | 5V/3.3V + GND | Board power supply |

### Power Supply Options
**Option 1: 12V + DC-DC Step Down Converter (Recommended)**
- 12V input for pump
- DC-DC step down module (LM2596 or similar) provides efficient 5V for ESP8266 and relay
- 1A rating sufficient for system (ESP + relay consume ~200mA)
- High efficiency (85-95%), minimal heat generation
- No heat sink required
- Adjustable output voltage (set to exactly 5.0V)

**Option 2: Dual Supply**
- 12V for pump
- Separate 5V supply for electronics

## Step-by-Step Installation Guide

### 1. Hardware Assembly

1. **Prepare the enclosure**:
   - Choose waterproof enclosure if system will be outdoors
   - Plan cable entry points
   - Ensure proper ventilation for electronics

2. **Wire the power supply**:
   ```
   12V DC → DC-DC Step Down IN+ → DC-DC Step Down OUT+ (5V) → ESP8266 VIN
           → DC-DC Step Down IN- → DC-DC Step Down OUT- (GND) → ESP8266 GND
   ```
   - Adjust DC-DC output to exactly 5V using onboard potentiometer
   - No heat sink required due to high efficiency

3. **Connect the relay**:
   - VCC → 5V
   - GND → GND  
   - IN → D1 (GPIO5)
   - COM/NO → Pump power switching

4. **Wire the sensors**:
   - Button: One side to D2, other to GND
   - Float sensor: Signal to D5, power as needed
   - Soil sensor: Analog out to A0, VCC to 3.3V, GND to GND

5. **Connect the pump**:
   - Through relay contacts
   - Ensure proper pump voltage (5V/12V)
   - Add flyback diode for inductive loads

### 2. Software Setup

1. **Install Arduino IDE**:
   - Download from [arduino.cc](https://www.arduino.cc/en/software)
   - Install ESP8266 board package
   - Add board URL: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`

2. **Install required libraries**:
   ```
   WiFiManager by tzapu
   PubSubClient by Nick O'Leary  
   NTPClient by Fabrice Weinberg
   Button2 by Lennart Hennigs
   ```

3. **Configure the code**:
   - Edit `defaults.h` with your settings
   - Change MQTT server, credentials if needed
   - Adjust default watering time if required

4. **Compile and upload**:
   - Select board: "NodeMCU 1.0 (ESP-12E Module)"
   - Select correct COM port
   - Upload the code

### 3. Initial Configuration

1. **First boot**:
   - ESP8266 will create WiFi hotspot "ESP-Watering"
   - Connect with phone/laptop (no password required)
   - Browser should open automatically, or go to 192.168.4.1

2. **WiFi setup**:
   - Select your home WiFi network
   - Enter WiFi password
   - Configure MQTT settings:
     - Host: your MQTT broker
     - User: MQTT username
     - Password: MQTT password
   - Click "Save"

3. **Verify connection**:
   - Check serial monitor (115200 baud)
   - Wait for "WiFi connected" and "MQTT connected" messages
   - Test manual button operation

### 4. Calibration

1. **Test water level sensor**:
   - Monitor serial output
   - Check readings when tank is full vs empty
   - Verify "Low water" messages appear correctly

2. **Calibrate soil moisture sensor**:
   - Check A0 readings in serial monitor
   - Dry air: ~800-1000
   - Moist soil: ~500-700  
   - In water: ~400-500
   - Adjust thresholds as needed

3. **Test watering system**:
   - Use manual button first
   - Test MQTT commands
   - Verify maximum time limits work
   - Check emergency stop functionality

## Usage

### Manual Control
- **Short Button Press**: Toggle watering ON/OFF
- **Long Button Press** (5+ seconds): Reset WiFi settings

### MQTT Control

#### Commands (Topic: `water`)
- `on` - Start watering (uses maximum time limit)
- `off` - Stop watering immediately
- `status` - Get current system status
- `tank` - Get water tank status
- `soil` - Get current soil moisture reading
- `time` - Get current time
- `max` - Get maximum watering time setting
- `[number]` - Water for specific seconds (e.g., `30` for 30 seconds)

#### Settings (Topic: `water/set`)
- `max,[seconds]` - Set maximum watering time (e.g., `max,300` for 5 minutes)

#### Status Topics
- `water/status` - System status (idle, water start, water stop, low water)
- `water/tank` - Tank status (ok, empty)
- `water/soil` - Soil moisture value (0-1024, updated every minute)
- `water/log` - Action logging with timestamps

### Example MQTT Commands
```bash
# Start watering
mosquitto_pub -h mqtt.host -u mqtt.user -P mqtt.pass -t "water" -m "on"

# Water for 60 seconds
mosquitto_pub -h mqtt.host -u mqtt.user -P mqtt.pass -t "water" -m "60"

# Set maximum watering time to 10 minutes
mosquitto_pub -h mqtt.host -u mqtt.user -P mqtt.pass -t "water/set" -m "max,600"

# Get status
mosquitto_pub -h mqtt.host -u mqtt.user -P mqtt.pass -t "water" -m "status"

# Check tank water level
mosquitto_pub -h mqtt.host -u mqtt.user -P mqtt.pass -t "water" -m "tank"

# Get soil moisture reading
mosquitto_pub -h mqtt.host -u mqtt.user -P mqtt.pass -t "water" -m "soil"

# Subscribe to soil moisture (automatic updates every minute)
mosquitto_sub -h mqtt.host -u mqtt.user -P mqtt.pass -t "water/soil"
```

### Soil Moisture Calibration
- **Dry (air)**: ~800-1000
- **Moist soil**: ~500-700
- **Wet (in water)**: ~400-500

## Configuration

### Default Settings
All default settings are stored in `defaults.h`:

```cpp
#define DEFAULT_MQTT_HOST "mc.esiteq.com"
#define DEFAULT_MQTT_USER "water"
#define DEFAULT_MQTT_PASS "flows"
#define DEFAULT_MAX_WATER_TIME 300  // 5 minutes
#define DEFAULT_WATER_SUFFIX ""     // Topic suffix for multiple devices
```

### Runtime Configuration
- **WiFi Settings**: Configure via captive portal (long press button to reset)
- **MQTT Settings**: Set server, username, password via web interface
- **Watering Time**: Adjustable via MQTT commands, stored in EEPROM
- **Topic Suffix**: Add suffix to MQTT topics for multiple devices

## Safety Features

- **Maximum Time Limit**: Prevents overwatering (default: 5 minutes)
- **Water Level Protection**: Float sensor prevents dry running - system automatically stops watering when tank is empty
- **Auto Shutoff**: Timer automatically stops watering
- **Manual Override**: Physical button always works for emergency stop
- **Settings Persistence**: Maximum time setting survives power cycles
- **Stable GPIO**: Uses GPIO5 (D1) to avoid boot-time relay activation
- **Remote Device Safe**: Never auto-resets WiFi settings
- **Emergency Stop**: Immediate watering stop when water level drops

## Troubleshooting

### WiFi Connection Issues
- **Problem**: Cannot connect to WiFi
- **Solution**: Hold button for 5+ seconds to reset WiFi settings, reconfigure

### MQTT Connection Issues
- **Problem**: WiFi connected but no MQTT
- **Solution**: Check MQTT broker settings, verify credentials, check firewall

### Power Supply Issues
- **Problem**: ESP8266 resets randomly or doesn't boot
- **Solution**: 
  - Check DC-DC converter output voltage (should be exactly 5.0V)
  - Adjust using onboard potentiometer if needed
  - Verify 1A current rating of DC-DC module
  - Check all power connections

### Relay Not Working
- **Problem**: Manual button doesn't activate pump
- **Solution**: 
  - Check wiring to D1 (GPIO5)
  - Verify relay module power supply
  - Test relay with multimeter
  - Check pump power supply

### Sensor Issues
- **Problem**: Incorrect soil moisture readings
- **Solution**: 
  - Clean sensor contacts
  - Check wiring to A0
  - Recalibrate in dry air and water
  - Replace sensor if damaged

### Water Level Sensor
- **Problem**: Tank sensor not working
- **Solution**:
  - Check wiring to D5
  - Verify sensor type (NC/NO)
  - Test sensor continuity
  - Check pull-up resistor if needed

### Serial Monitor Debugging
- **Baud rate**: 115200
- **Look for**: WiFi connection status, MQTT connection, sensor readings
- **Commands**: Use serial monitor to verify all functions work locally

## Project Structure

```
esp-watering/
├── ESP-Watering.ino          # Main application code
├── defaults.h                # Configuration file
└── README.md                 # This file
```

## Optional: Home Assistant Integration

*If you want to add this system to your Home Assistant dashboard, follow these additional steps. This integration provides advanced automation, mobile notifications, and beautiful dashboards.*

### Prerequisites
- Home Assistant installation
- MQTT broker (can use the same as ESP device)
- Home Assistant MQTT integration

### Setup Instructions

#### 1️⃣ Add MQTT Integration to Home Assistant
1. Go to **Settings** → **Devices & Services** → **Add Integration**
2. Search for **MQTT** and select it
3. Enter your MQTT broker details:
   ```
   Broker: mc.esiteq.com  (or your broker)
   Port: 1883
   Username: water
   Password: flows
   ```
4. Click **Submit** → **Finish**

#### 2️⃣ Auto-Discovery Device Setup
Execute these commands in **Developer Tools** → **Services** to automatically create all entities:

**Soil Moisture Sensor:**
```yaml
service: mqtt.publish
data:
  topic: homeassistant/sensor/esp_watering_soil/config
  payload: >
    {
      "name": "ESP Watering Soil",
      "state_topic": "water/soil",
      "unit_of_measurement": "points",
      "device_class": "humidity",
      "icon": "mdi:water-percent",
      "unique_id": "esp_watering_soil",
      "device": {
        "identifiers": ["esp_watering"],
        "name": "ESP Watering System",
        "manufacturer": "DIY",
        "model": "ESP8266"
      }
    }
  retain: true
```

**System Status Sensor:**
```yaml
service: mqtt.publish
data:
  topic: homeassistant/sensor/esp_watering_status/config  
  payload: >
    {
      "name": "ESP Watering Status",
      "state_topic": "water/status", 
      "icon": "mdi:sprinkler",
      "unique_id": "esp_watering_status",
      "device": {
        "identifiers": ["esp_watering"],
        "name": "ESP Watering System"
      }
    }
  retain: true
```

**Water Tank Sensor:**
```yaml
service: mqtt.publish
data:
  topic: homeassistant/binary_sensor/esp_watering_tank/config
  payload: >
    {
      "name": "ESP Water Tank",
      "state_topic": "water/tank",
      "payload_on": "empty", 
      "payload_off": "ok",
      "device_class": "problem",
      "icon": "mdi:water-alert",
      "unique_id": "esp_watering_tank",
      "device": {
        "identifiers": ["esp_watering"],
        "name": "ESP Watering System"
      }
    }
  retain: true
```

**Watering Control Switch:**
```yaml
service: mqtt.publish
data:
  topic: homeassistant/switch/esp_watering_pump/config
  payload: >
    {
      "name": "ESP Watering Pump",
      "command_topic": "water",
      "payload_on": "on",
      "payload_off": "off", 
      "state_topic": "water/status",
      "state_on": "water start",
      "state_off": "idle",
      "icon": "mdi:water-pump",
      "unique_id": "esp_watering_pump",
      "device": {
        "identifiers": ["esp_watering"],
        "name": "ESP Watering System"
      }
    }
  retain: true
```

#### 3️⃣ Create Template Sensors
1. Go to **Settings** → **Devices & Services** → **Helpers** → **Create Helper**
2. Select **Template** → **Template a sensor**

**Soil Moisture Percentage:**
- **Name**: `Soil Moisture Percentage`
- **State template**:
  ```jinja2
  {% set raw = states('sensor.esp_watering_soil') | int(800) %}
  {% if raw > 840 %}0
  {% elif raw < 440 %}100
  {% else %}{{ ((840 - raw) / (840 - 440) * 100) | round(1) }}
  {% endif %}
  ```
- **Unit of measurement**: `%`
- **Icon**: `mdi:water-percent`

#### 4️⃣ Setup Automations
1. Go to **Settings** → **Automations & Scenes** → **Create Automation**

**Tank Refill Reminder:**
- **Trigger**: State change
  - **Entity**: `binary_sensor.esp_water_tank`
  - **To**: `on`
- **Action**: Call service
  - **Service**: `notify.mobile_app_your_phone`
  - **Title**: `🪣 Water Tank Alert`
  - **Message**: `Water tank is low! Time to refill.`

**Auto Watering Schedule:**
- **Trigger**: Time
  - **At**: `08:00:00`
- **Condition**: 
  - **Numeric state**: `sensor.soil_moisture_percentage` below `30`
  - **State**: `binary_sensor.esp_water_tank` is `off`
- **Action**: Call service
  - **Service**: `mqtt.publish`
  - **Topic**: `water`
  - **Payload**: `120` (2 minutes)

#### 5️⃣ Create Dashboard
1. Go to **Overview** → **Edit Dashboard** → **Add Card**

**Recommended cards:**
- **Gauge Card**: 
  - Entity: `sensor.soil_moisture_percentage`
  - Min: 0, Max: 100
  - Green: 50-100, Yellow: 30-50, Red: 0-30

- **Button Card**: 
  - Entity: `switch.esp_watering_pump`
  - Tap action: Toggle

- **Entities Card**:
  - Include all watering system entities

### Home Assistant Features
- 📊 **Real-time monitoring** of soil moisture and water levels
- 🤖 **Automated watering** based on schedule and soil conditions  
- 📱 **Mobile notifications** for tank refill reminders
- 📈 **Historical data** and trends with beautiful graphs
- 🎛️ **Manual control** through dashboard
- 🔧 **Easy configuration** through UI (no YAML editing required)
- 🌡️ **Weather integration** (prevent watering during rain)
- ⏰ **Smart scheduling** (avoid watering during hot midday)

## Technical Details

- **Microcontroller**: ESP8266 (NodeMCU) or ESP32
- **Time Zone**: Configured for Ukraine (UTC+2/UTC+3 with DST)
- **EEPROM Usage**: Settings stored at address 500-503
- **Boot Safety**: Uses GPIO5 instead of GPIO2 to prevent startup glitches
- **Communication**: WiFi + MQTT protocol
- **Logging**: Real-time action logging with timestamps
- **Update Rate**: Soil moisture published every 60 seconds
- **Safety Timeout**: Maximum watering time configurable (default 5 minutes)

## License

This project is open source. Feel free to modify and distribute.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## Changelog

### Version 2.0.0 (2025-08-18)
**Major Update: Water Level & Soil Moisture Monitoring**

#### 🆕 New Features
- **Water Level Detection**: Added float sensor on D5 pin for tank water monitoring
- **Soil Moisture Monitoring**: Added analog soil moisture sensor support (A0 pin)
- **New MQTT Topics**:
  - `water/soil` - Automatic soil moisture readings every minute
  - `water/tank` - Tank water status (ok/empty)
  - Enhanced `water/status` with "low water" state
- **New MQTT Commands**:
  - `tank` - Get current tank status
  - `soil` - Get instant soil moisture reading

#### 🔒 Enhanced Safety
- **Automatic Emergency Stop**: Watering stops immediately when tank becomes empty
- **Dry Run Prevention**: Cannot start watering when tank is empty
- **Water Status Variable**: `has_water` (0/1) for persistent water state tracking
- **Smart LED Indicators**: Emergency blinking when tank is empty

#### 🛠 Improvements
- **Enhanced Logging**: "Low water" and "Water restored" messages in logs
- **Cleaner Console Output**: Removed debug spam for production use
- **Better Status Tracking**: More accurate system status reporting
- **Improved Tank Sensor Logic**: Better detection of water level changes

#### 📊 Monitoring
- **Real-time Soil Data**: Continuous soil moisture monitoring with calibration info
- **Water Level Alerts**: Immediate notification when tank runs dry
- **Status Integration**: All sensors integrated into unified status system

#### 🔧 Technical Changes
- Added `updateWaterStatus()` function for centralized water state management
- Improved `buttonHandler()` for better sensor integration
- Enhanced MQTT callback with new command support
- Better initialization sequence for all sensors
- Optimized console output for production deployment