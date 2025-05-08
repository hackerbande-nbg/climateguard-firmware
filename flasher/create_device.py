import datetime
import os
import requests
import serial
import serial.tools.list_ports
import subprocess
import shutil
import time

DEVEUI_MAGIC = 0x42  # Magic number to validate devEui in EEPROM
ACK_BYTE = 0xAA  # Acknowledgment byte
ERROR_BYTE = 0xEE  # Error byte

# Load API key from environment variables
api_key = os.getenv('TTN_API_KEY')
if not api_key:
    raise ValueError("TTN_API_KEY environment variable not set")

app_id = os.getenv('TTN_APP_ID')
if not api_key:
    raise ValueError("TTN_APP_ID environment variable not set")

# Define the API endpoint and headers
url = f"https://eu1.cloud.thethings.network/api/v3/applications/{app_id}/devices"
headers = {
    "Authorization": f"Bearer {api_key}",
    "Content-Type": "application/json"
}

def create_device_in_ttn(dev_eui):

    # Generate a device_id with "auto" and a timestamp
    timestamp = datetime.datetime.now().strftime("%Y%m%d%H%M%S")
    device_id = f"auto-{timestamp}"

    # Remove colons from devEui
    dev_eui = dev_eui.replace(":", "")

    # Define the payload for creating a new device
    # ToDo Appkey missing
    # ToDo Lorawan details not visible on created device - problem?
    payload = {
    "end_device": {
        "ids": {
        "device_id": f"{device_id}",
        "dev_eui": f"{dev_eui}",
        "join_eui": "0000000000000000"
        },
        "name": f"Created via Python Flasher on {timestamp}",
        "network_server_address": "eu1.cloud.thethings.network",
        "application_server_address": "eu1.cloud.thethings.network",
        "lorawan_version": "1.0.2",
        "lorawan_phy_version": "PHY_V1_0_2_REV_B",
        "frequency_plan_id": "EU_863_870",
        "supports_join": True
    }
    }

    # Make the API request to create the device
    response = requests.post(url, json=payload, headers=headers)

    if response.status_code == 200:
        print(f"Device created successfully with EUI: {dev_eui}")
    else:
        print(f"Failed to create device: {response.status_code} - {response.text}")

# Write a test string into the EEPROM of an attached ESP32
def write_to_eeprom(port, baudrate, test_string):
    try:
        with serial.Serial(port, baudrate, timeout=1) as ser:
            # Convert string to bytes
            string_bytes = test_string.encode('utf-8')
            string_length = len(string_bytes)

            # Create payload: command byte + length byte + string
            data = bytearray([0xEE])  # Command byte
            data.append(string_length)  # Length byte
            data.extend(string_bytes)  # String data

            ser.write(data)
            print(f"Sending to EEPROM: {test_string} (length: {string_length})")

            # Wait for acknowledgment
            response = ser.read()
            if response and response[0] == 0xAA:
                print("Write confirmed by ESP32")
            else:
                print("No confirmation received from ESP32")

    except serial.SerialException as e:
        print(f"Failed to write to EEPROM: {e}")


def read_deveui_from_eeprom(port, baudrate):
    try:
        with serial.Serial(port, baudrate, timeout=1) as ser:
            # Send command to read devEui
            ser.write(b'\xEF')  # Command byte for EEPROM read
            time.sleep(0.1)  # Wait for the device to respond

            # Read acknowledgment byte
            ack = ser.read(1)
            if not ack or ack[0] == ERROR_BYTE:
                print("Failed to read devEui: Device returned ERROR.")
                return None
            elif ack[0] != ACK_BYTE:
                print(f"Unexpected response: {ack[0]}. Device might still be booting.")
                return None

            # Read devEui (8 bytes)
            dev_eui = ser.read(8)
            if len(dev_eui) == 8:
                dev_eui_hex = ":".join(f"{byte:02X}" for byte in dev_eui)
                print(f"Read devEui from EEPROM: {dev_eui_hex}")
                return dev_eui_hex
            else:
                print("Failed to read devEui: Incomplete response.")
                return None
    except serial.SerialException as e:
        print(f"Error reading devEui from EEPROM: {e}")
        return None

def read_deveui_from_eeprom_with_retry(port, baudrate, max_retries=12):
    """
    Attempt to read the devEui from EEPROM, retrying every 5 seconds if unsuccessful.

    :param port: Serial port to communicate with the device.
    :param baudrate: Baud rate for the serial communication.
    :param max_retries: Maximum number of retries (default: 12, i.e., 1 minute).
    :return: The devEui as a string if successful, None otherwise.
    """
    retries = 0
    while retries < max_retries:
        dev_eui = read_deveui_from_eeprom(port, baudrate)
        if dev_eui:
            return dev_eui
        retries += 1
        elapsed_time = retries * 5
        print(f"Retrying to read devEui... {elapsed_time} seconds elapsed.")
        time.sleep(5)
    print("Failed to retrieve devEui after maximum retries.")
    return None

def list_com_ports():
    ports = serial.tools.list_ports.comports()
    available_ports = [port.device for port in ports]
    if not available_ports:
        print("No COM ports available.")
        return None
    print("Available COM ports:")
    for i, port in enumerate(available_ports, start=1):
        print(f"{i}: {port}")
    return available_ports

def flash_platformio_project(project_dir, env_name=None):
    """
    Flash a PlatformIO project to an ESP device.

    :param project_dir: Path to the PlatformIO project directory.
    :param env_name: (Optional) Name of the PlatformIO environment to use.
    """
    try:
        # Change to the project directory
        os.chdir(project_dir)

        # Upload the firmware
        print("Building and Uploading the firmware to the ESP device...")
        upload_cmd = ["pio", "run", "--target", "upload"]
        if env_name:
            upload_cmd.extend(["-e", env_name])
        subprocess.run(upload_cmd, check=True)

        print("Firmware uploaded successfully!")

    except subprocess.CalledProcessError as e:
        print(f"Error during PlatformIO operation: {e}")
        exit(1)

# Example usage
if __name__ == "__main__":
    available_ports = list_com_ports()
    if (available_ports):
        if len(available_ports) == 1:
            esp32_port = available_ports[0]
            print(f"Automatically selected COM port: {esp32_port}")
        else:
            try:
                port_index = int(input("Select a COM port by number: ")) - 1
                esp32_port = available_ports[port_index]
            except (ValueError, IndexError):
                print("Invalid selection.")
                exit(1)
    else:
        print("No COM ports available.")
        exit(1)

    baudrate = 115200

    # Path to your PlatformIO project directory
    # ToDo this sucks
    project_directory = r"path\to\your\platformio\project"  # Update this to your project directory

    # Optional: Specify the environment name (if you have multiple environments in platformio.ini)
    environment_name = None  # e.g., "esp32dev"

    # Flash the firmware
    flash_platformio_project(project_directory, environment_name)

    # Read and log devEui from EEPROM with retry
    dev_eui = read_deveui_from_eeprom_with_retry(esp32_port, baudrate)
    if dev_eui:
        print(f"Device EUI: {dev_eui}")
        # Create the device in The Things Stack
        create_device_in_ttn(dev_eui)
    else:
        print("Unable to retrieve Device EUI.")

