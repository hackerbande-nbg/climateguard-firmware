"""
ClimateGuard Web Reader
=======================
Reads sensor data from Heltec ESP32 device via serial port
and displays it on a web page with real-time updates.

Usage:
    python web_reader.py [COM_PORT] [BAUD_RATE]

Example:
    python web_reader.py COM3 115200
    
Then open http://localhost:5000 in your browser
"""

import serial as pyserial
import serial.tools.list_ports
import json
import argparse
import sys
import threading
import time
from datetime import datetime
from flask import Flask, render_template, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

# Global variable to store latest sensor data
latest_data = {
    'temperature': None,
    'humidity': None,
    'pressure': None,
    'voltage': None,
    'timestamp': None,
    'device_timestamp': None,
    'error': None
}

# Serial connection
ser = None
serial_thread = None
running = False

def parse_arguments():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(
        description='Web-based display for ClimateGuard sensor data'
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
        '--host',
        default='127.0.0.1',
        help='Web server host (default: 127.0.0.1)'
    )
    parser.add_argument(
        '--webport',
        type=int,
        default=5000,
        help='Web server port (default: 5000)'
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

def read_serial_data():
    """Background thread to continuously read serial data"""
    global latest_data, ser, running
    
    while running:
        try:
            if ser and ser.is_open:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                if line and line.startswith('{') and line.endswith('}'):
                    try:
                        data = json.loads(line)
                        latest_data['temperature'] = data.get('temperature')
                        latest_data['humidity'] = data.get('humidity')
                        latest_data['pressure'] = data.get('pressure')
                        latest_data['voltage'] = data.get('voltage')
                        latest_data['device_timestamp'] = data.get('timestamp')
                        latest_data['timestamp'] = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                        latest_data['error'] = None
                        
                        print(f"[{latest_data['timestamp']}] T: {data.get('temperature', 'N/A')}°C, "
                              f"H: {data.get('humidity', 'N/A')}%, "
                              f"P: {data.get('pressure', 'N/A')}hPa, "
                              f"V: {data.get('voltage', 'N/A')}V")
                        
                    except json.JSONDecodeError as e:
                        print(f"JSON parse error: {e}")
                        latest_data['error'] = f"JSON parse error: {e}"
                        
        except Exception as e:
            print(f"Serial read error: {e}")
            latest_data['error'] = str(e)
            time.sleep(1)  # Wait a bit before retrying

@app.route('/')
def index():
    """Serve the main page"""
    return render_template('index.html')

@app.route('/api/data')
def get_data():
    """API endpoint to get latest sensor data"""
    return jsonify(latest_data)

def main():
    global ser, serial_thread, running
    
    args = parse_arguments()
    
    print("ClimateGuard Web Reader")
    print("=======================")
    
    # Get the port to use (auto-detect or user select)
    port = get_serial_port(args.port)
    
    print(f"Serial Port: {port}")
    print(f"Baud rate: {args.baudrate}")
    print(f"Timeout: {args.timeout}s")
    print()
    
    try:
        # Open serial connection
        ser = pyserial.Serial(
            port=port,
            baudrate=args.baudrate,
            timeout=args.timeout
        )
        
        # Allow time for connection to stabilize
        time.sleep(2)
        ser.reset_input_buffer()
        
        print("Serial connection established!")
        print()
        
        # Start serial reading thread
        running = True
        serial_thread = threading.Thread(target=read_serial_data, daemon=True)
        serial_thread.start()
        
        # Start web server
        print(f"Starting web server on http://{args.host}:{args.webport}")
        print(f"Open http://{args.host}:{args.webport} in your browser")
        print()
        print("Press Ctrl+C to stop")
        print()
        
        app.run(host=args.host, port=args.webport, debug=False)
        
    except pyserial.SerialException as e:
        print(f"\nError opening serial port: {e}")
        print("\nAvailable ports:")
        ports = serial.tools.list_ports.comports()
        for port in ports:
            print(f"  - {port.device}: {port.description}")
        sys.exit(1)
        
    except KeyboardInterrupt:
        print("\n\nStopping web reader...")
        running = False
        if ser and ser.is_open:
            ser.close()
        print("Goodbye!")
        sys.exit(0)
    finally:
        running = False
        if ser and ser.is_open:
            ser.close()

if __name__ == '__main__':
    main()
