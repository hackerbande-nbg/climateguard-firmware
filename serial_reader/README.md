# ClimateGuard Serial Reader

Python utilities to read and display sensor data from the ClimateGuard device via serial connection.

## Features

- **Console Reader** (`serial_reader.py`): Display data in terminal with optional CSV logging
- **Web Reader** (`web_reader.py`): Beautiful real-time web dashboard with live updates

## Installation

1. Install Python dependencies:
```bash
pip install -r requirements.txt
```

## Console Reader Usage

### Basic usage (auto-detect port):
```bash
python serial_reader.py
```

### Specify port and baud rate:
```bash
python serial_reader.py COM5 115200
```

### Enable data logging to CSV file:
```bash
python serial_reader.py --log
```

### Full options:
```bash
python serial_reader.py COM3 115200 --timeout 5.0 --log
```

## Web Reader Usage

### Basic usage (auto-detect port):
```bash
python web_reader.py
```

Then open http://localhost:5000 in your browser to see the live dashboard!

### Specify port and baud rate:
```bash
python web_reader.py COM3 115200
```

### Custom host and web port:
```bash
python web_reader.py --host 0.0.0.0 --webport 8080
```

This will make the server accessible from other devices on your network at http://YOUR_IP:8080

## Arguments

### Console Reader
- `port`: Serial port name (auto-detect if not specified)
  - Windows: COM3, COM4, etc.
  - Linux/Mac: /dev/ttyUSB0, /dev/ttyACM0, etc.
- `baudrate`: Serial baud rate (default: 115200)
- `--timeout`: Serial read timeout in seconds (default: 5.0)
- `--log`: Enable logging to sensor_log.csv file

### Web Reader
- `port`: Serial port name (auto-detect if not specified)
- `baudrate`: Serial baud rate (default: 115200)
- `--timeout`: Serial read timeout in seconds (default: 5.0)
- `--host`: Web server host (default: 127.0.0.1)
- `--webport`: Web server port (default: 5000)

## Output Format

### Console Reader
The script displays sensor readings in a formatted table:

```
============================================================
Timestamp: 2025-10-24 14:30:25
------------------------------------------------------------
  Temperature:   22.45 °C
  Humidity:      55.30 %
  Pressure:    1013.25 hPa
  Battery:        4.15 V
  Device time:  12345 ms
============================================================
```

### Web Reader
The web interface shows:
- 📊 Real-time sensor data cards with color-coded gradients
- 🔄 Auto-refresh every second
- 📱 Responsive design for mobile and desktop
- 🎨 Beautiful gradient UI
- ⏱️ Last update timestamp and device uptime
- 🔴 Connection status indicator

## Port Auto-Detection

Both readers feature smart port detection:
- **0 ports found**: Error message and exit
- **1 port found**: Automatically uses it
- **Multiple ports found**: Interactive menu for selection

## Log File

When `--log` is enabled in console reader, data is saved to `sensor_log.csv`:
```
timestamp,temperature,humidity,pressure,voltage
2025-10-24 14:30:25,22.45,55.30,1013.25,4.15
```

## Screenshots

### Web Dashboard
The web interface features:
- Temperature (red gradient)
- Humidity (blue gradient)
- Pressure (green gradient)
- Battery voltage (orange gradient)
- Status indicators
- Device information panel

## Troubleshooting

- **Port not found**: Check that the device is connected and drivers are installed
- **No data received**: Verify the baud rate matches the firmware (115200)
- **Permission denied** (Linux): Add your user to the dialout group: `sudo usermod -a -G dialout $USER`
- **Web page not loading**: Check firewall settings or try a different port with `--webport`
- **Flask not found**: Make sure to install requirements: `pip install -r requirements.txt`

## Tips

- Use the web reader for real-time monitoring and demonstrations
- Use the console reader with `--log` for data collection and analysis
- Run the web reader with `--host 0.0.0.0` to access from other devices on your network
