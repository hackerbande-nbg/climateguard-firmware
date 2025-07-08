# IDs
## Relevant IDs
Relevant in LoRaWAN OTAA mode, which we use.
- application_id
    - human readable name of the application
    - set in flasher
- device_id
    - must be unique across the application
    - set in flasher: get devices from climateguard backend and use next available ID for prefix
- device_name
    - does not need to be unique, but set equal to device_id in flasher
- join_eui / app_eui
    - not required, use 0000000000000000 hardcoded in firmware
- dev_eui
    - must be globally unique
    - auto create in firmware
- app_key
    - secret key for communication between device and application server
    - generate randomly and set in flasher
## Irrelevant IDs
These are only used for LoRaWAN ABP mode, which we don't intend to use.
- nwkSKey
- appSKey
- devAddr  
