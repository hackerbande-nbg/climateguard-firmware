import secrets

import datetime
import os
import requests
import serial
import serial.tools.list_ports
import subprocess
import shutil
import time
from dotenv import load_dotenv
import re


DEVEUI_MAGIC = 0x42  # Magic number to validate devEui in EEPROM
ACK_BYTE = 0xAA  # Acknowledgment byte
ERROR_BYTE = 0xEE  # Error byte

# Load environment variables from .env file
load_dotenv()

# Load values from environment variables
api_key = os.getenv('TTN_API_KEY')
if not api_key:
    raise ValueError("TTN_API_KEY environment variable not set")

# Load climateguard API key
climateguard_api_key = os.getenv('CLIMATEGUARD_API_KEY')
if not climateguard_api_key:
    raise ValueError("CLIMATEGUARD_API_KEY environment variable not set")

app_id = os.getenv('TTN_APP_ID')
if not app_id:
    raise ValueError("TTN_APP_ID environment variable not set")


device_prefix = os.getenv('DEVICE_PREFIX', 'auto')
application_server_address = os.getenv('APPLICATION_SERVER_ADDRESS', 'eu1.cloud.thethings.network')
lorawan_version = os.getenv('LORAWAN_VERSION', '1.0.2')
lorawan_phy_version = os.getenv('LORAWAN_PHY_VERSION', 'PHY_V1_0_2_REV_B')
frequency_plan_id = os.getenv('FREQUENCY_PLAN_ID', 'EU_863_870')
supports_join = os.getenv('SUPPORTS_JOIN', 'true').lower() == 'true'
baudrate = int(os.getenv('BAUD_RATE', '115200'))

# Load climateguard API URL
climateguard_api_url = os.getenv('CLIMATEGUARD_API_URL')
if not climateguard_api_url:
    raise ValueError("CLIMATEGUARD_API_URL environment variable not set")

# Post a new device to the climateguard API
def post_device_to_climateguard(device_name, dev_eui):
    """
    Register a new device in the climateguard API.
    :param device_name: Name of the device (string)
    :param dev_eui: Device EUI as hex string (with or without colons)

    :return: Response object or None
    """
    url = climateguard_api_url.rstrip('/') + '/devices'
    payload = {
        "name": device_name,
        "deveui": dev_eui.replace(":", ""),
    }
    headers = {
        "X-API-Key": climateguard_api_key,
        "Content-Type": "application/json"
    }
    try:
        response = requests.post(url, json=payload, headers=headers)
        if response.status_code in (200, 201):
            print(f"Device posted to climateguard API: {device_name}")
        else:
            print(f"Failed to post device to climateguard API: {response.status_code} - {response.text}")
        return response
    except Exception as e:
        print(f"Error posting device to climateguard API: {e}")
        return None
    
# Write a 16-byte appKey into the EEPROM of an attached ESP32
def write_appkey_to_eeprom_with_retry(port, baudrate, appkey_bytes, max_retries=12):
    """
    Attempt to write the appKey to EEPROM, retrying every 5 seconds if unsuccessful.
    :param port: Serial port to communicate with the device.
    :param baudrate: Baud rate for the serial communication.
    :param appkey_bytes: 16-byte appKey to write.
    :param max_retries: Maximum number of retries (default: 12, i.e., 1 minute).
    :return: True if successful, False otherwise.
    """
    retries = 0
    while retries < max_retries:
        try:
            with serial.Serial(port, baudrate, timeout=2) as ser:
                # Command byte for appKey
                data = bytearray([0xF0])
                data.extend(appkey_bytes)
                ser.write(data)
                print(f"Sending appKey to ESP32: {appkey_bytes.hex()}")
                # Wait for acknowledgment
                response = ser.read()
                if response and response[0] == ACK_BYTE:
                    print("appKey write confirmed by ESP32")
                    return True
                else:
                    print("No confirmation received from ESP32 for appKey write")
        except serial.SerialException as e:
            print(f"Failed to write appKey to EEPROM: {e}")
        retries += 1
        elapsed_time = retries * 5
        print(f"Retrying to write appKey... {elapsed_time} seconds elapsed.")
        time.sleep(5)
    print("Failed to write appKey after maximum retries.")
    return False
    

def get_next_device_name():
    """
    Query the climateguard API for existing devices and find the next available device name
    with the prefix DEVICE_PREFIX and an incremental number.
    """
    devices_url = climateguard_api_url.rstrip('/') + '/devices'
    all_devices = []
    page = 1
    page_size = 100
    while True:
        params = {'page': page, 'limit': page_size}
        try:
            response = requests.get(devices_url, params=params)
            response.raise_for_status()
            devices = response.json()
        except Exception as e:
            print(f"Failed to fetch devices from {devices_url} (page {page}): {e}")
            break

        data = devices.get('data', [])
        if not data:
            break
        
        all_devices.extend(data)
        if len(data) < page_size:
            break
        page += 1

    # Find all device names that start with the prefix
    pattern = re.compile(rf"^{re.escape(device_prefix)}-(\d+)$")
    max_num = 0
    for device in all_devices:
        name = device.get('name') or device.get('device_id') or ''
        match = pattern.match(name)
        if match:
            num = int(match.group(1))
            if num > max_num:
                max_num = num
    next_num = max_num + 1
    return f"{device_prefix}-{next_num}"

# Define the API endpoint and headers
url = f"https://{application_server_address}/api/v3/applications/{app_id}/devices"
headers = {
    "Authorization": f"Bearer {api_key}",
    "Content-Type": "application/json"
}

def create_device_in_ttn(dev_eui, appkey):
    # Generate a device_id with prefix and next available number
    device_id = get_next_device_name()
    timestamp = datetime.datetime.now().strftime("%Y%m%d%H%M%S")

    # Remove colons from devEui
    dev_eui = dev_eui.replace(":", "")

    # 1. Initial POST to create the device with minimal fields
    post_payload = {
        "end_device": {
            "ids": {
                "application_ids": {
                    "application_id": app_id
                },
                "device_id": device_id,
                "dev_eui": dev_eui,
                "join_eui": "0000000000000000"
            },
            "network_server_address": application_server_address,
            "application_server_address": application_server_address,
            "join_server_address": application_server_address
        },
        "field_mask": {
            "paths": [
                "network_server_address",
                "application_server_address",
                "join_server_address"
            ]
        }
    }
    post_url = f"https://{application_server_address}/api/v3/applications/{app_id}/devices"
    response = requests.post(post_url, json=post_payload, headers=headers)
    if response.status_code not in (200, 201):
        print(f"Initial POST failed: {response.status_code} - {response.text}")
        exit(1)

    # 2. PUT to NS endpoint
    ns_url = f"https://{application_server_address}/api/v3/ns/applications/{app_id}/devices/{device_id}"
    ns_payload = {
        "end_device": {
            "frequency_plan_id": frequency_plan_id,
            "lorawan_version": lorawan_version,
            "lorawan_phy_version": lorawan_phy_version,
            "supports_join": supports_join,
            "multicast": False,
            "supports_class_b": False,
            "supports_class_c": False,
            "mac_settings": {
                "rx2_data_rate_index": 0,
                "rx2_frequency": "869525000"
            },
            "ids": {
                "join_eui": "0000000000000000",
                "dev_eui": dev_eui,
                "device_id": device_id,
                "application_ids": {
                    "application_id": app_id
                }
            }
        },
        "field_mask": {
            "paths": [
                "frequency_plan_id",
                "lorawan_version",
                "lorawan_phy_version",
                "supports_join",
                "multicast",
                "supports_class_b",
                "supports_class_c",
                "mac_settings.rx2_data_rate_index",
                "mac_settings.rx2_frequency",
                "ids.join_eui",
                "ids.dev_eui",
                "ids.device_id",
                "ids.application_ids.application_id"
            ]
        }
    }
    response = requests.put(ns_url, json=ns_payload, headers=headers)
    if response.status_code not in (200, 201):
        print(f"NS PUT failed: {response.status_code} - {response.text}")
        exit(1)

    # 3. PUT to AS endpoint
    as_url = f"https://{application_server_address}/api/v3/as/applications/{app_id}/devices/{device_id}"
    as_payload = {
        "end_device": {
            "ids": {
                "join_eui": "0000000000000000",
                "dev_eui": dev_eui,
                "device_id": device_id,
                "application_ids": {
                    "application_id": app_id
                }
            }
        },
        "field_mask": {
            "paths": [
                "ids.join_eui",
                "ids.dev_eui",
                "ids.device_id",
                "ids.application_ids.application_id"
            ]
        }
    }
    response = requests.put(as_url, json=as_payload, headers=headers)
    if response.status_code not in (200, 201):
        print(f"AS PUT failed: {response.status_code} - {response.text}")
        exit(1)

    # 4. PUT to JS endpoint
    js_url = f"https://{application_server_address}/api/v3/js/applications/{app_id}/devices/{device_id}"
    js_payload = {
        "end_device": {
            "ids": {
                "join_eui": "0000000000000000",
                "dev_eui": dev_eui,
                "device_id": device_id,
                "application_ids": {
                    "application_id": app_id
                }
            },
            "network_server_address": application_server_address,
            "application_server_address": application_server_address,
            "root_keys": {
                "app_key": {
                    "key": appkey.hex().upper()
                }
            }
        },
        "field_mask": {
            "paths": [
                "network_server_address",
                "application_server_address",
                "ids.join_eui",
                "ids.dev_eui",
                "ids.device_id",
                "ids.application_ids.application_id",
                "root_keys.app_key.key"
            ]
        }
    }
    response = requests.put(js_url, json=js_payload, headers=headers)
    if response.status_code not in (200, 201):
        print(f"JS PUT failed: {response.status_code} - {response.text}")
        exit(1)


    # Has been deactivated as it did not work on first try and we also don't use labels atm :)
    
    # # 5. Final PUT to main endpoint for label_ids (empty update)
    # put_url = f"https://{application_server_address}/api/v3/applications/{app_id}/devices/{device_id}"
    # put_payload = {
    #     "end_device": {},
    #     "field_mask": {
    #         "paths": ["label_ids"]
    #     }
    # }
    # response = requests.put(put_url, json=put_payload, headers=headers)
    # if response.status_code not in (200, 201):
    #     print(f"Final PUT failed: {response.status_code} - {response.text}")
    #     exit(1)

    print(f"Device created and updated successfully with EUI: {dev_eui}")


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

if __name__ == "__main__":

    device_name = get_next_device_name()
    print(f"Next available device name: {device_name}")

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


    # Path to your PlatformIO project directory from environment variable
    project_directory = os.getenv('PLATFORMIO_PROJECT_PATH')
    if not project_directory:
        print("PLATFORMIO_PROJECT_PATH environment variable not set.")
        exit(1)

    # Optional: Specify the environment name (if you have multiple environments in platformio.ini)
    environment_name = None  # e.g., "esp32dev"

    # Flash the firmware
    flash_platformio_project(project_directory, environment_name)

    # Read and log devEui from EEPROM with retry
    dev_eui = read_deveui_from_eeprom_with_retry(esp32_port, baudrate)
    if dev_eui:
        print(f"Device EUI: {dev_eui}")
        # Generate a random 16-byte appKey
        appkey_bytes = secrets.token_bytes(16)
        print(f"Generated random appKey: {appkey_bytes.hex()}")

        # Write the appKey to the ESP32
        if write_appkey_to_eeprom_with_retry(esp32_port, baudrate, appkey_bytes):
            print("appKey successfully written to device.")
        else:
            print("Failed to write appKey to device.")
            exit(1)
        
        # Create the device in The Things Stack, using the generated appKey
        create_device_in_ttn(dev_eui, appkey_bytes)
    else:
        print("Unable to retrieve Device EUI.")
        exit(1)


    # Post the device to the climateguard API
    response = post_device_to_climateguard(device_name, dev_eui)
    if response and response.status_code in (200, 201):
        print(f"Device {device_name} successfully posted to climateguard API.")
    else:
        print("Failed to post device to climateguard API.")
        # Log response details if available
        if response:
            print(f"Response status code: {response.status_code}")
            print(f"Response text: {response.text}")


