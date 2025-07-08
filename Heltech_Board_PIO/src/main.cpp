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
#include <EEPROM.h>

Adafruit_BME280 bme;  // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();

// devEUI will be auto generated, -D LORAWAN_DEVEUI_AUTO should be set
uint8_t devEui[8];
// appEui seems to be optional, set to all zeros
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[16];


/* ABP para*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda, 0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef, 0x67 };
uint32_t devAddr = (uint32_t)0x007e6ae1;

/*LoraWan channelsmask, default channels 0-7*/
uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = LORAMAC_REGION_EU868; 
// LoRaMacRegion_t loraWanRegion = ACTIVE_REGION; 

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 600000;

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
  /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */

  // sensors_event_t temp_event, pressure_event, humidity_event;
  // bme_temp->getEvent(&temp_event);
  // bme_pressure->getEvent(&pressure_event);
  // bme_humidity->getEvent(&humidity_event);

  // appDataSize = 4;
  // appData[0] = temp_event.temperature;
  // appData[1] = humidity_event.relative_humidity;
  // appData[2] = pressure_event.pressure;
  // appData[3] = 0x03;

  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);

  // Konvertiere Temperatur, Luftfeuchtigkeit und Druck in Integer
  int16_t temperature = (int16_t)(temp_event.temperature * 100);           // Skalierung auf 2 Dezimalstellen
  uint16_t humidity = (uint16_t)(humidity_event.relative_humidity * 100);  // Skalierung auf 2 Dezimalstellen
  uint32_t pressure = (uint32_t)(pressure_event.pressure * 100);           // Skalierung auf 2 Dezimalstellen

  // AppData-Größe festlegen
  appDataSize = 9;

  // Payload zusammenstellen
  appData[0] = (temperature >> 8) & 0xFF;  // Temperatur, MSB
  appData[1] = temperature & 0xFF;         // Temperatur, LSB
  appData[2] = (humidity >> 8) & 0xFF;     // Luftfeuchtigkeit, MSB
  appData[3] = humidity & 0xFF;            // Luftfeuchtigkeit, LSB
  appData[4] = (pressure >> 16) & 0xFF;    // Druck, MSB
  appData[5] = (pressure >> 8) & 0xFF;     // Druck, mittleres Byte
  appData[6] = pressure & 0xFF;            // Druck, LSB
  
  // // Spannung (Millivolt) auslesen
  // int analogVolts = analogReadMilliVolts(1);
  // appData[7] = (analogVolts >> 8) & 0xFF;  // Spannung, MSB
  // appData[8] = analogVolts & 0xFF;         // Spannung, LSB       

}

//if true, next uplink will add MOTE_MAC_DEVICE_TIME_REQ


void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  // Initialize EEPROM (size 512 bytes)
  EEPROM.begin(512);

  // Try to read appKey from EEPROM (address 16-31)
  bool appKeyValid = false;
  for (int i = 0; i < 16; i++) {
    appKey[i] = EEPROM.read(16 + i);
    if (appKey[i] != 0x00) appKeyValid = true;
  }
  if (!appKeyValid) {
    // If EEPROM is empty, use default key (same as before)
    uint8_t defaultAppKey[16] = { 0x51, 0x56, 0x65, 0xFA, 0x76, 0x40, 0xB8, 0x56, 0xA7, 0xE7, 0xB5, 0x88, 0x99, 0x9E, 0xC5, 0x20 };
    for (int i = 0; i < 16; i++) appKey[i] = defaultAppKey[i];
    Serial.println("appKey not found in EEPROM, using default.");
  } else {
    Serial.println("appKey loaded from EEPROM.");
  }
  Serial.print("appKey: ");
  for (int i = 0; i < 16; i++) {
    Serial.printf("%02X", appKey[i]);
    if (i < 15) Serial.print(":");
  }
  Serial.println();

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  Serial.println(F("find a valid BME280 sensor"));
  if (!bme.begin(0x76)) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (1) delay(10);
  }
}

void loop() {

  // Check for incoming serial commands to set appKey
  if (Serial.available()) {
    uint8_t cmd = Serial.read();
    switch (cmd) {
      case 0xF0:  // Command to set appKey
      {
        // Wait for 16 bytes (appKey)
        uint8_t keyBuf[16];
        int received = 0;
        unsigned long start = millis();
        while (received < 16 && (millis() - start) < 2000) { // 2s timeout
          if (Serial.available()) {
            keyBuf[received++] = Serial.read();
          }
        }
        if (received == 16) {
          // Store appKey in EEPROM (address 16-31)
          for (int i = 0; i < 16; i++) {
            EEPROM.write(16 + i, keyBuf[i]);
            appKey[i] = keyBuf[i];
          }
          if (EEPROM.commit()) {
            Serial.write(0xAA); // ACK
            Serial.println("appKey written to EEPROM successfully!");
          } else {
            Serial.write(0xEE); // ERROR
            Serial.println("Failed to commit appKey to EEPROM");
          }
        } else {
          Serial.write(0xEE); // ERROR
          Serial.println("Timeout or incomplete appKey received");
        }
        break;
      }
      default:
        // Unknown command, ignore or handle as needed
        break;
    }
  }

  switch (deviceState) {
    case DEVICE_STATE_INIT:
      {
#if (LORAWAN_DEVEUI_AUTO)
        LoRaWAN.generateDeveuiByChipID();
#endif
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

}