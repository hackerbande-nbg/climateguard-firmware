"""
ClimateGuard Serial Reader
==========================
Reads sensor data from Heltec ESP32 device via serial port
and displays it in the console.

Usage:
    python serial_reader.py [COM_PORT] [BAUD_RATE]

Example:
    python serial_reader.py COM3 115200
"""

import serial as pyserial
import serial.tools.list_ports
import json
import argparse
import sys
from datetime import datetime
import time

def parse_arguments():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(
        description='Read and display sensor data from ClimateGuard device'
    )
    parser.add_argument(
        'port',
        nargs='?',
        default=None,
        help='Serial port (auto-detect if not specified)'
    )
    parser.add_argument(
        'baudrate',
        nargs='?',
        type=int,
        default=115200,
        help='Baud rate (default: 115200)'
    )
    parser.add_argument(
        '--timeout',
        type=float,
        default=5.0,
        help='Serial read timeout in seconds (default: 5.0)'
    )
    parser.add_argument(
        '--log',
        action='store_true',
        help='Save data to log file'
    )
    return parser.parse_args()

def get_serial_port(specified_port=None):
    """Get serial port - either specified, auto-detected, or user selected"""
    if specified_port:
        return specified_port
    
    # Get list of available ports
    ports = serial.tools.list_ports.comports()
    
    if len(ports) == 0:
        print("ERROR: No serial ports found!")
        print("Please connect your device and try again.")
        sys.exit(1)
    elif len(ports) == 1:
        # Exactly one port available - use it automatically
        selected_port = ports[0].device
        print(f"Auto-detected port: {selected_port} ({ports[0].description})")
        return selected_port
    else:
        # Multiple ports available - let user choose
        print(f"Found {len(ports)} serial ports:")
        print()
        for i, port in enumerate(ports, 1):
            print(f"  {i}. {port.device}: {port.description}")
        print()
        
        while True:
            try:
                choice = input("Select port number (or 'q' to quit): ").strip()
                if choice.lower() == 'q':
                    sys.exit(0)
                
                port_index = int(choice) - 1
                if 0 <= port_index < len(ports):
                    return ports[port_index].device
                else:
                    print(f"Invalid choice. Please enter a number between 1 and {len(ports)}.")
            except ValueError:
                print("Invalid input. Please enter a number or 'q' to quit.")
            except KeyboardInterrupt:
                print("\nCancelled.")
                sys.exit(0)

def format_sensor_data(data):
    """Format sensor data for console display"""
    output = []
    output.append("\n" + "="*60)
    output.append(f"Timestamp: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    output.append("-"*60)
    
    if 'temperature' in data:
        output.append(f"  Temperature:  {data['temperature']:6.2f} °C")
    if 'humidity' in data:
        output.append(f"  Humidity:     {data['humidity']:6.2f} %")
    if 'pressure' in data:
        output.append(f"  Pressure:     {data['pressure']:7.2f} hPa")
    if 'voltage' in data:
        output.append(f"  Battery:      {data['voltage']:6.2f} V")
    if 'timestamp' in data:
        output.append(f"  Device time:  {data['timestamp']} ms")
    
    output.append("="*60)
    return "\n".join(output)

def log_to_file(data):
    """Append sensor data to log file"""
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    with open('sensor_log.csv', 'a') as f:
        if f.tell() == 0:  # File is empty, write header
            f.write('timestamp,temperature,humidity,pressure,voltage\n')
        f.write(f"{timestamp},{data.get('temperature', '')},{data.get('humidity', '')},")
        f.write(f"{data.get('pressure', '')},{data.get('voltage', '')}\n")

def main():
    args = parse_arguments()
    
    print("ClimateGuard Serial Reader")
    print("===========================")
    
    # Get the port to use (auto-detect or user select)
    port = get_serial_port(args.port)
    
    print(f"Port: {port}")
    print(f"Baud rate: {args.baudrate}")
    print(f"Timeout: {args.timeout}s")
    if args.log:
        print("Logging: Enabled (sensor_log.csv)")
    print("\nWaiting for data...\n")
    
    try:
        # Open serial connection
        ser = pyserial.Serial(
            port=port,
            baudrate=args.baudrate,
            timeout=args.timeout
        )
        
        # Allow time for connection to stabilize
        time.sleep(2)
        
        # Flush any existing data in buffer
        ser.reset_input_buffer()
        
        while True:
            try:
                # Read line from serial
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                if not line:
                    continue
                
                # Try to parse as JSON
                if line.startswith('{') and line.endswith('}'):
                    try:
                        data = json.loads(line)
                        print(format_sensor_data(data))
                        
                        if args.log:
                            log_to_file(data)
                            
                    except json.JSONDecodeError as e:
                        print(f"JSON parse error: {e}")
                        print(f"Raw data: {line}")
                else:
                    # Print non-JSON lines (info messages, etc.)
                    print(f"[INFO] {line}")
                    
            except UnicodeDecodeError:
                # Skip lines with encoding issues
                continue
                    
    except pyserial.SerialException as e:
        print(f"\nError opening serial port: {e}")
        print("\nAvailable ports:")
        ports = serial.tools.list_ports.comports()
        for port in ports:
            print(f"  - {port.device}: {port.description}")
        sys.exit(1)
        
    except KeyboardInterrupt:
        print("\n\nStopping serial reader...")
        print("Goodbye!")
        sys.exit(0)
    finally:
        # Clean up serial connection if it exists
        try:
            if 'ser' in locals() and ser.is_open:
                ser.close()
        except:
            pass

if __name__ == '__main__':
    main()
