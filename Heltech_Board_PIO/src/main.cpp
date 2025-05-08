/* 
Heltec Board which includes LoRaWan
temp sensor: 
- BME280 or
- DHT22
*/

#include "config.h"  // Include the new config file first
#include "LoRaWan_APP.h"
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <EEPROM.h>

// Define pins for the DHT sensor
#define DHTPIN 4  // Use GPIO 4
#define DHTTYPE DHT22 // DHT 22 (AM2302)

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

Adafruit_BME280 bme;  // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();

// Not in reverse order - derived from ESP32 MAC address
uint8_t devEui[8];
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// ToDo still hardcoded - change!
uint8_t appKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/* ABP para*/
// ToDo still hardcoded - change!
uint8_t nwkSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint32_t devAddr = (uint32_t)0x00000000;

/*LoraWan channelsmask, default channels 0-7*/
uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

/*LoraWan region, select the appropriate region manually*/
LoRaMacRegion_t loraWanRegion = LORAMAC_REGION_EU868; // Change to your region, e.g., LORAMAC_REGION_US915
// LoRaMacRegion_t loraWanRegion = ACTIVE_REGION; 

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 600000; // 600 seconds / 10 minutes

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

/* Prepares the payload of the frame */
static void prepareTxFrame(uint8_t port) {
  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);

  // Read humidity and temperature values from DHT sensor
  float dht_humidity = dht.readHumidity();
  float dht_temperature = dht.readTemperature(); // Celsius

  // Check if any reads failed and exit early (to try again).
  if (isnan(dht_humidity) || isnan(dht_temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Debug prints for DHT22 sensor values
  Serial.print("DHT22 - Temperature: ");
  Serial.print(dht_temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(dht_humidity);
  Serial.println(" %");

  // Convert BME280 data to integers
  int16_t temperature = (int16_t)(temp_event.temperature * 100);           // Scale to 2 decimal places
  uint16_t humidity = (uint16_t)(humidity_event.relative_humidity * 100);  // Scale to 2 decimal places
  uint32_t pressure = (uint32_t)(pressure_event.pressure * 100);           // Scale to 2 decimal places

  // Convert DHT22 data to integers
  int16_t dht_temp_int = (int16_t)(dht_temperature * 100);                 // Scale to 2 decimal places
  uint16_t dht_hum_int = (uint16_t)(dht_humidity * 100);                   // Scale to 2 decimal places

  // Debug prints for converted DHT22 values
  Serial.print("Converted DHT22 - Temperature: ");
  Serial.print(dht_temp_int);
  Serial.print(", Humidity: ");
  Serial.println(dht_hum_int);

  // Set AppData size
  appDataSize = 11;

  // Assemble payload
  appData[0] = (temperature >> 8) & 0xFF;  // BME280 Temperature, MSB
  appData[1] = temperature & 0xFF;         // BME280 Temperature, LSB
  appData[2] = (humidity >> 8) & 0xFF;     // BME280 Humidity, MSB
  appData[3] = humidity & 0xFF;            // BME280 Humidity, LSB
  appData[4] = (pressure >> 16) & 0xFF;    // BME280 Pressure, MSB
  appData[5] = (pressure >> 8) & 0xFF;     // BME280 Pressure, middle byte
  appData[6] = pressure & 0xFF;            // BME280 Pressure, LSB
  appData[7] = (dht_temp_int >> 8) & 0xFF; // DHT22 Temperature, MSB
  appData[8] = dht_temp_int & 0xFF;        // DHT22 Temperature, LSB
  appData[9] = (dht_hum_int >> 8) & 0xFF;  // DHT22 Humidity, MSB
  appData[10] = dht_hum_int & 0xFF;        // DHT22 Humidity, LSB

  // Print payload
  Serial.print("Payload: ");
  for (int i = 0; i < appDataSize; i++) {
    Serial.print(appData[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

#define DEVEUI_MAGIC 0x42  // Magic number to validate devEui in EEPROM
#define ACK_BYTE 0xAA  // Acknowledgment byte
#define ERROR_BYTE 0xEE  // Error byte

void setup() {
    Serial.begin(115200);
    EEPROM.begin(512);  // Initialize EEPROM with 512 bytes
    Serial.println("ESP32 is ready!");

    // Attempt to read devEui from EEPROM
    if (EEPROM.read(8) == DEVEUI_MAGIC) {  // Check magic number
        for (int i = 0; i < 8; i++) {
            devEui[i] = EEPROM.read(i);
        }
        Serial.println("devEui found in EEPROM.");
    } else {
        // ToDo find out what this does - I reimplemeted it by accident :O
        // #if (LORAWAN_DEVEUI_AUTO)
        //     LoRaWAN.generateDeveuiByChipID();
        // #endif
        Serial.println("devEui not found in EEPROM or invalid magic number. Generating a new one...");

        // Seed the random number generator
        randomSeed(esp_random());

        // Generate devEui
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);  // Read the ESP32's MAC address
        devEui[0] = random(0, 256);           // Random first byte
        devEui[1] = random(0, 256);           // Random second byte
        devEui[2] = mac[0];                   // Use all 6 bytes of the MAC address
        devEui[3] = mac[1];
        devEui[4] = mac[2];
        devEui[5] = mac[3];
        devEui[6] = mac[4];
        devEui[7] = mac[5];

        // Save the generated devEui to EEPROM
        for (int i = 0; i < 8; i++) {
            EEPROM.write(i, devEui[i]);
        }
        EEPROM.write(8, DEVEUI_MAGIC);  // Write magic number
        EEPROM.commit();
        Serial.println("Generated devEui saved to EEPROM.");
    }

    // Print the devEui for debugging
    Serial.print("devEui: ");
    for (int i = 0; i < 8; i++) {
        Serial.printf("%02X", devEui[i]);
        if (i < 7) Serial.print(":");
    }
    Serial.println();

    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    if (!bme.begin(0x76)) {
      Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
      while (1) delay(10);
    }
}

void loop() {

    // Check for incoming serial commands
    if (Serial.available()) {
        uint8_t cmd = Serial.read();  // Read the command byte
        if (cmd == 0xEE) {  // EEPROM write command
            // Wait for the length byte
            while (!Serial.available()) delay(1);
            uint8_t length = Serial.read();

            // Clear EEPROM
            for (int i = 0; i < 512; i++) {
                EEPROM.write(i, 0);
            }

            // Write the length to the first byte
            EEPROM.write(0, length);

            // Read and store the string
            int idx = 1;
            while (length > 0) {
                while (!Serial.available()) delay(1);
                EEPROM.write(idx++, Serial.read());
                length--;
            }

            // Commit the changes to EEPROM
            if (EEPROM.commit()) {
                Serial.write(ACK_BYTE);  // Send acknowledgment byte
                Serial.println("Data written to EEPROM successfully!");
            } else {
                Serial.write(ERROR_BYTE);  // Send error byte
                Serial.println("Failed to commit EEPROM changes");
            }
        } else if (cmd == 0xEF) {  // Command to return devEui
            if (EEPROM.read(8) == DEVEUI_MAGIC) {  // Validate magic number
                Serial.write(ACK_BYTE);  // Send acknowledgment byte
                for (int i = 0; i < 8; i++) {
                    Serial.write(devEui[i]);  // Send each byte of devEui
                }
            } else {
                Serial.write(ERROR_BYTE);  // Send error byte
            }
        }
    }

    switch (deviceState) {
        case DEVICE_STATE_INIT:
          {
                    // ToDo find out what this does - I reimplemeted it by accident :O
    // #if (LORAWAN_DEVEUI_AUTO)
    //         LoRaWAN.generateDeveuiByChipID();
    // #endif
            LoRaWAN.init(loraWanClass, loraWanRegion);
            //both set join DR and DR when ADR off
            LoRaWAN.setDefaultDR(3);
            break;
          }
        case DEVICE_STATE_JOIN:
          {
            LoRaWAN.join();
            break;
          }
        case DEVICE_STATE_SEND:
          {
            prepareTxFrame(appPort);
            LoRaWAN.send();
            deviceState = DEVICE_STATE_CYCLE;
            break;
          }
        case DEVICE_STATE_CYCLE:
          {
            // Schedule next packet transmission
            txDutyCycleTime = appTxDutyCycle + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
            LoRaWAN.cycle(txDutyCycleTime);
            deviceState = DEVICE_STATE_SLEEP;
            break;
          }
        case DEVICE_STATE_SLEEP:
          {
            LoRaWAN.sleep(loraWanClass);
            // // Set deep sleep timer for 10 minutes
            // esp_sleep_enable_timer_wakeup(appTxDutyCycle * 1000);
            // Serial.println("Going to sleep now...");
            // esp_deep_sleep_start();
    
            
            break;
          }
        default:
          {
            deviceState = DEVICE_STATE_INIT;
            break;
          }
      }

    delay(100);  // Add a small delay to avoid busy looping
}
