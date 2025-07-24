# Payload Versioning and Byte Layout

## Version Byte Selection

To avoid confusion with valid temperature MSBs, choose version bytes in the range 0x22–0xEF. Here are 10 good version bytes:

| Version Byte (Hex) | Decimal |
| ------------------ | ------- |
| 0x22               | 34      |
| 0x23               | 35      |
| 0x24               | 36      |
| 0x25               | 37      |
| 0x26               | 38      |
| 0x27               | 39      |
| 0x28               | 40      |
| 0x29               | 41      |
| 0x2A               | 42      |
| 0x2B               | 43      |
| ...                | ..      |
| 0xEF               | 239     |



<!-- PAYLOAD_LAYOUT_TABLE_START -->
## Payload Byte Layouts by Version

| Version | Version Byte | Temperature | Humidity | Pressure | Voltage |
| ------- | ------------ | ----------- | -------- | -------- | ------- |
| LEGACY  |              | 0,1         | 2,3      | 4,5,6    |         |
| v1      | 0x22         | 1,2         | 3,4      | 5,6,7    | 8,9     |
<!-- PAYLOAD_LAYOUT_TABLE_END -->



**Note:** All values are scaled by 100 (e.g., temperature -5.00°C is sent as -500).

## Example Payload (Version 0x22)

| Byte | Value          | Meaning                |
| ---- | -------------- | ---------------------- |
| 0    | 0x22           | Version                |
| 1-2  | 0xFE 0x0C      | Temperature (-5.00°C)  |
| 3-4  | 0x01 0x2C      | Humidity (30.0%)       |
| 5-7  | 0x09 0xC4 0x00 | Pressure (2500.00 hPa) |
| 8-9  | 0x0B 0xB8      | Voltage (30.00 V)      |

This layout ensures robust versioning and clear attribute positions for consumers.
