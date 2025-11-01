#include "config.h"  // Include the new config file first
#include "LoRaWan_APP.h"
#include <Adafruit_BME280.h>

#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>

#include <Wire.h>               
#include "HT_SSD1306Wire.h"

// Try using Wire (bus 0) for the display since we initialize it properly
// Change back to use the default Wire bus for OLED
static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr, freq, SDA, SCL, geometry, rst


#define Read_VBAT_Voltage 1
#define ADC_CTRL 37 // Heltec GPIO to toggle VBatt read connection …
#define ADC_READ_STABILIZE 10 // in ms (delay from GPIO control and ADC connections times)

Adafruit_BME280 bme;  // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();

// devEUI will be auto generated, -D LORAWAN_DEVEUI_AUTO should be set
uint8_t devEui[8];
// appEui seems to be optional, set to all zeros
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
// uint8_t appKey[16];
uint8_t appKey[] = { 0xA7, 0x3D, 0x82, 0xC5, 0x76, 0x1F, 0xE9, 0x2B, 0x94, 0x5D, 0x7E, 0x0C, 0xF3, 0x68, 0xA1, 0xD4 };


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

/* Liest die Batteriespannung und gibt sie als float (V) zurück */
float readBatteryVoltage() {
  digitalWrite(ADC_CTRL, HIGH);
  delay(ADC_READ_STABILIZE);
  int millivolts = analogReadMilliVolts(Read_VBAT_Voltage);
  digitalWrite(ADC_CTRL, LOW);
  // In tatsächliche Batteriespannung umwandeln
  return (millivolts / 1000.0) * 4.9;
}

/* Update display with current sensor readings */
void updateDisplay() {
  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);
  float voltage = readBatteryVoltage();
  
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  
  // Title
  display.drawString(0, 0, "ClimateGuard Sensor");
  display.drawHorizontalLine(0, 12, 128);
  
  // Temperature
  display.drawString(0, 16, "Temp:");
  display.drawString(70, 16, String(temp_event.temperature, 1) + " C");
  
  // Humidity
  display.drawString(0, 28, "Humidity:");
  display.drawString(70, 28, String(humidity_event.relative_humidity, 1) + " %");
  
  // Pressure
  display.drawString(0, 40, "Pressure:");
  display.drawString(70, 40, String(pressure_event.pressure, 0) + " hPa");
  
  // Battery
  display.drawString(0, 52, "Battery:");
  display.drawString(70, 52, String(voltage, 2) + " V");
  
  display.display();
}

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

  // Spannung (Batteriespannung) auslesen
  float voltage = readBatteryVoltage();

  // Konvertiere Temperatur, Luftfeuchtigkeit und Druck in Integer
  int16_t temperature = (int16_t)(temp_event.temperature * 100);           // Skalierung auf 2 Dezimalstellen
  uint16_t humidity = (uint16_t)(humidity_event.relative_humidity * 100);  // Skalierung auf 2 Dezimalstellen
  uint32_t pressure = (uint32_t)(pressure_event.pressure * 100);           // Skalierung auf 2 Dezimalstellen
  uint16_t voltageInt = (uint16_t)(voltage * 100); // Skalierung auf 2 Dezimalstellen

  // AppData-Größe festlegen (1 version byte + 9 data bytes)
  appDataSize = 10;

  // Payload zusammenstellen
  appData[0] = 1; // Version byte
  appData[1] = (temperature >> 8) & 0xFF;  // Temperatur, MSB
  appData[2] = temperature & 0xFF;         // Temperatur, LSB
  appData[3] = (humidity >> 8) & 0xFF;     // Luftfeuchtigkeit, MSB
  appData[4] = humidity & 0xFF;            // Luftfeuchtigkeit, LSB
  appData[5] = (pressure >> 16) & 0xFF;    // Druck, MSB
  appData[6] = (pressure >> 8) & 0xFF;     // Druck, mittleres Byte
  appData[7] = pressure & 0xFF;            // Druck, LSB
  appData[8] = (voltageInt >> 8) & 0xFF;  // Spannung, MSB
  appData[9] = voltageInt & 0xFF;         // Spannung, LSB

}

//if true, next uplink will add MOTE_MAC_DEVICE_TIME_REQ


void setup() {
  Serial.begin(115200);
  pinMode(ADC_CTRL, OUTPUT);
  
  // Initialize EEPROM (size 512 bytes)
  EEPROM.begin(512);

  // Initialize MCU first
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  
  // DON'T initialize any Wire buses yet - let the display do it first
  Serial.println(F("Initializing display FIRST..."));
  
  // Power on and reset display
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);  // Power OFF first
  delay(100);
  digitalWrite(Vext, LOW);   // Power ON
  delay(200);
  
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, LOW);
  delay(50);
  digitalWrite(RST_OLED, HIGH);
  delay(50);
  
  // Let display.init() initialize its own I2C bus
  display.init();
  Serial.println(F("Display init() called"));
  
  display.displayOn();
  display.flipScreenVertically();
  display.setContrast(255);
  
  display.clear();
  display.drawRect(0, 0, 128, 64);
  display.fillRect(10, 10, 20, 20);
  display.display();
  delay(1000);
  
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 20, "ClimateGuard");
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 40, "Initializing...");
  display.display();
  
  Serial.println(F("Display initialized!"));
  delay(2000);
  
  // NOW initialize BME280 on Wire1 with different I2C pins
  Serial.println(F("Initializing BME280 sensor..."));
  Wire1.begin(41, 42);  // Wire1 for BME280 on GPIO 41/42
  delay(100);
  
  // Scan default I2C bus
  Serial.println(F("Scanning Wire1 I2C bus (BME280 bus)..."));
  byte error, address;
  int nDevices = 0;
  for(address = 1; address < 127; address++) {
    Wire1.beginTransmission(address);
    error = Wire1.endTransmission();
    if (error == 0) {
      Serial.print(F("I2C device found at address 0x"));
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }
  if (nDevices == 0)
    Serial.println(F("No I2C devices found on Wire1 bus\n"));
  else
    Serial.println(F("Wire1 I2C scan done\n"));
  
  // Try both common addresses on Wire1
  if (!bme.begin(0x76, &Wire1) && !bme.begin(0x77, &Wire1)) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (1) delay(10);
  }
  Serial.println(F("BME280 sensor found!"));
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
      case 0xEF:  // Command to read devEui
      {
        Serial.write(0xAA); // ACK
        for (int i = 0; i < 8; i++) {
          Serial.write(devEui[i]);
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
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_16);
        display.drawString(64, 24, "Joining...");
        display.display();

        LoRaWAN.join();
        break;
      }
    case DEVICE_STATE_SEND:
      {
        // Update display with current sensor data
        updateDisplay();
        delay(1000); // Show data for 1 second
        
        // Show sending status
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_16);
        display.drawString(64, 24, "Sending...");
        display.display();

        prepareTxFrame(appPort);
        LoRaWAN.send();
        deviceState = DEVICE_STATE_CYCLE;
        break;
      }
    case DEVICE_STATE_CYCLE:
      {
        // Show success message
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_16);
        display.drawString(64, 20, "Sent!");
        display.setFont(ArialMT_Plain_10);
        display.drawString(64, 40, "Next in 10 min");
        display.display();
        
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