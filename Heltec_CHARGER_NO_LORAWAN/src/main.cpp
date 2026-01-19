/*
 * Minimal Firmware for Heltec LoRa 3.2
 * Purpose: Battery Charging Only - No functionality
 * 
 * This firmware does nothing except allow the board to charge batteries.
 * Deep sleep mode is enabled to minimize power consumption.
  */

#include <Arduino.h>
#include <esp_sleep.h>

// Deep sleep duration in seconds (wake up every hour to check status)
#define DEEP_SLEEP_DURATION 3600  // 1 hour in seconds

void setup() {
  // Minimal initialization
  Serial.begin(115200);
  delay(100);  // Give serial time to initialize
  
  Serial.println("Heltec LoRa 3.2 - Battery Charging Mode");
  Serial.println("Entering deep sleep to minimize power consumption");
  Serial.printf("Will wake up every %d seconds\n", DEEP_SLEEP_DURATION);
  
  delay(1000);  // Give time to see the message
  
  // Configure deep sleep timer wake-up
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION * 1000000ULL);  // Convert to microseconds
  
  // Enter deep sleep
  Serial.println("Going to sleep now...");
  Serial.flush();  // Ensure message is sent before sleeping
  esp_deep_sleep_start();
}

void loop() {
  // This will never be reached because of deep sleep
}