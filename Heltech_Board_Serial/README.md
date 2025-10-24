# ClimateGuard Serial Firmware

This is a modified version of the ClimateGuard firmware that sends sensor data via serial port instead of LoRaWAN.

## Features

- Reads temperature, humidity, and pressure from BME280 sensor
- Reads battery voltage
- Outputs data in JSON format via serial port
- Configurable output interval (default: 5 seconds)

## Hardware Requirements

- Heltec WiFi LoRa 32 V3 board
- BME280 sensor (I2C connection at address 0x76)

## Building and Uploading

### Using PlatformIO CLI:

```bash
# Build the firmware
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

### Using VS Code with PlatformIO:

1. Open this folder in VS Code
2. Click the PlatformIO icon in the sidebar
3. Click "Build" to compile
4. Click "Upload" to flash the device
5. Click "Monitor" to view serial output

## Configuration

You can modify the output interval in `include/config.h`:

```cpp
#define SERIAL_OUTPUT_INTERVAL 5000  // milliseconds
```

## Data Format

The firmware outputs sensor data in JSON format:

```json
{"temperature":22.45,"humidity":55.30,"pressure":1013.25,"voltage":4.15,"timestamp":12345}
```

Fields:
- `temperature`: Temperature in degrees Celsius
- `humidity`: Relative humidity in percent
- `pressure`: Atmospheric pressure in hPa
- `voltage`: Battery voltage in volts
- `timestamp`: Device uptime in milliseconds

## Reading the Data

Use the Python serial reader in the `serial_reader` folder to read and display the data:

```bash
cd ../serial_reader
pip install -r requirements.txt
python serial_reader.py COM3 115200
```

See the [serial_reader README](../serial_reader/README.md) for more details.

## Wiring

BME280 sensor should be connected via I2C:
- VCC -> 3.3V
- GND -> GND
- SDA -> SDA (GPIO 17 on Heltec V3)
- SCL -> SCL (GPIO 18 on Heltec V3)

## Differences from LoRaWAN Version

This version:
- ✅ Removed all LoRaWAN dependencies
- ✅ Outputs data via serial in JSON format
- ✅ Faster data rate (configurable, default 5 seconds vs 10 minutes)
- ✅ Simpler codebase without networking complexity
- ✅ Human-readable output format
- ❌ No wireless transmission capability
- ❌ Requires physical connection to read data
