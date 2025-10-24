#include <Arduino.h>
#include "config.h"
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <Wire.h>

#define Read_VBAT_Voltage 1
#define ADC_CTRL 37 // Heltec GPIO to toggle VBatt read connection
#define ADC_READ_STABILIZE 10 // in ms (delay from GPIO control and ADC connections times)

Adafruit_BME280 bme;  // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();

unsigned long lastSendTime = 0;

/* Reads battery voltage and returns it as float (V) */
float readBatteryVoltage() {
  digitalWrite(ADC_CTRL, HIGH);
  delay(ADC_READ_STABILIZE);
  int millivolts = analogReadMilliVolts(Read_VBAT_Voltage);
  digitalWrite(ADC_CTRL, LOW);
  // Convert to actual battery voltage
  return (millivolts / 1000.0) * 4.9;
}

/* Sends sensor data via serial in JSON format */
void sendSensorData() {
  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);

  // Read battery voltage
  float voltage = readBatteryVoltage();

  // Create JSON output
  Serial.print("{");
  Serial.print("\"temperature\":");
  Serial.print(temp_event.temperature, 2);
  Serial.print(",\"humidity\":");
  Serial.print(humidity_event.relative_humidity, 2);
  Serial.print(",\"pressure\":");
  Serial.print(pressure_event.pressure, 2);
  Serial.print(",\"voltage\":");
  Serial.print(voltage, 2);
  Serial.print(",\"timestamp\":");
  Serial.print(millis());
  Serial.println("}");
}

void setup() {
  pinMode(ADC_CTRL, OUTPUT);
  
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize
  
  Serial.println("ClimateGuard Serial Firmware");
  Serial.println("============================");
  
  // Initialize BME280 sensor
  if (!bme.begin(0x76)) {
    Serial.println("ERROR: Could not find a valid BME280 sensor, check wiring!");
    while (1) delay(10);
  }
  
  Serial.println("BME280 sensor initialized successfully");
  Serial.println("Starting sensor readings...");
  Serial.println();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Send sensor data at regular intervals
  if (currentTime - lastSendTime >= SERIAL_OUTPUT_INTERVAL) {
    sendSensorData();
    lastSendTime = currentTime;
  }
  
  // Small delay to prevent CPU hogging
  delay(100);
}
