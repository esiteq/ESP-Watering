# ESP-Watering

A simple and smart watering system for home plants or garden beds using ESP8266 or ESP32 microcontroller.

## Features

- **Remote Control**: MQTT-based control over WiFi
- **Manual Control**: Physical button for local operation
- **Smart Safety**: Maximum watering time protection
- **Time Logging**: All actions logged with timestamps
- **Persistent Settings**: Configuration stored in EEPROM
- **Easy Setup**: WiFiManager for wireless configuration
- **Real-time Clock**: NTP time synchronization

## Hardware Requirements

- ESP8266 NodeMCU or ESP32 board
- 5V/12V relay module
- Push button
- Water pump or solenoid valve
- Power supply
- Connecting wires

## Wiring Diagram

| Component | ESP8266 Pin | Description |
|-----------|-------------|-------------|
| Relay Module | D1 (GPIO5) | Controls water pump/valve |
| Button | D2 (GPIO4) | Manual control |
| Power | 5V/3.3V | Board power supply |
| Ground | GND | Common ground |

## Installation

1. **Hardware Assembly**:
   - Connect relay module to pin D1
   - Connect button between D2 and GND
   - Connect water pump to relay output
   - Power the ESP8266 board

2. **Software Setup**:
   - Install Arduino IDE
   - Add ESP8266 board support
   - Install required libraries:
     - WiFiManager
     - PubSubClient
     - NTPClient
     - Button2
   - Download and compile the code

3. **Initial Configuration**:
   - Upload code to ESP8266
   - Connect to "ESP-Watering" WiFi network (no password required)
   - Configure your WiFi and MQTT settings
   - Device will automatically connect and start working

## Usage

### Manual Control
- **Short Button Press**: Toggle relay ON/OFF (requires WiFi+MQTT for starting new watering)
- **Long Button Press** (5+ seconds): Reset WiFi settings

### Offline Operation
When WiFi or MQTT connection is lost:
- **Cannot start new watering** (safety feature for remote devices)
- **Can stop current watering** (safety override always available)
- **Manual button reports connection status** via Serial
- **Running watering continues normally** until timer expires
- **System logs offline actions** locally

### MQTT Control
The system subscribes to MQTT topics for remote control:

#### Commands (Topic: `water`)
- `on` - Start watering (uses maximum time limit)
- `off` - Stop watering immediately
- `status` - Get current relay status
- `time` - Get current time
- `max` - Get maximum watering time setting
- `[number]` - Water for specific seconds (e.g., `30` for 30 seconds)

#### Settings (Topic: `water/set`)
- `max,[seconds]` - Set maximum watering time (e.g., `max,300` for 5 minutes)

#### Logging (Topic: `water/log`)
All actions are logged here with timestamps.

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
```

## Configuration

### Default Settings
All default settings are stored in `defaults.h`:

```cpp
#define DEFAULT_MQTT_HOST "mqtt.host"
#define DEFAULT_MQTT_USER "mqtt.user"
#define DEFAULT_MQTT_PASS "mqtt.pass"
#define DEFAULT_MAX_WATER_TIME 300  // 5 minutes
```

### Runtime Configuration
- **WiFi Settings**: Configure via captive portal
- **MQTT Settings**: Set server, username, password via web interface
- **Watering Time**: Adjustable via MQTT commands, stored in EEPROM

## Safety Features

- **Maximum Time Limit**: Prevents overwatering (default: 5 minutes)
- **Auto Shutoff**: Timer automatically stops watering
- **Manual Override**: Physical button always works
- **Settings Persistence**: Maximum time setting survives power cycles
- **Stable GPIO**: Uses GPIO5 (D1) to avoid boot-time relay activation
- **Remote Device Safe**: Never auto-resets WiFi settings

## Technical Details

- **Microcontroller**: ESP8266 (NodeMCU) or ESP32
- **Time Zone**: Configured for Ukraine (UTC+2/UTC+3 with DST)
- **EEPROM Usage**: Settings stored at address 500-503
- **Boot Safety**: Uses GPIO5 instead of GPIO2 to prevent startup glitches
- **Communication**: WiFi + MQTT protocol
- **Logging**: Real-time action logging with timestamps

## Project Structure

```
esp-watering/
├── esp-watering.ino          # Main application code
├── defaults.h                # Configuration file
└── README.md                 # This file
```

## Troubleshooting

### WiFi Connection Issues
- Hold button for 5+ seconds to reset WiFi settings
- Look for "ESP-Watering" network to reconfigure (no password required)

### MQTT Connection Issues
- Check MQTT server settings in web portal
- Verify network connectivity
- Check MQTT credentials

### Relay Not Working
- Verify wiring to pin D1 (GPIO5)
- Check relay module power supply
- Test with manual button press
- Check MQTT connection for remote commands

### MQTT Connection Issues
- Check MQTT server settings in web portal
- Verify network connectivity
- Check MQTT credentials
- Commands work only when MQTT is connected

## License

This project is open source. Feel free to modify and distribute.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.