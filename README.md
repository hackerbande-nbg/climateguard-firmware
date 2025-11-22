# Climateguard Firmware
This Repo contains Firmware for the climateguard project as  well as a python tool to flash the firmware to the Board.

## Firmware
The Firmware was built using the PlatformIO extension with VS Code. 
Capabilities of the firmware:
- Create a Device EUI from the MAC and deliver it via Serial, if prompted with the right command (see Flasher)
- Read BME280 temperature sensor 
- Connect to The Things Network (TTN) and send the data to the TTN server

Open:
- There is another ID required to connect to TTN, this currently needs to be hardcoded in the firmware.

## Flashing Tool
The flashing tool will use the PlatformIO cli to flash the firmware to the board, retrieve the Device EUI from the board and create a TTN device with the EUI.
