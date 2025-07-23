#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <driver/adc.h>
#include <esp_adc_cal.h>
// SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

#define Read_VBAT_Voltage 1
#define ADC_CTRL 37 // Heltec GPIO to toggle VBatt read connection …
// Also, take care NOT to have ADC read connection
// in OPEN DRAIN when GPIO goes HIGH
#define ADC_READ_STABILIZE 10 // in ms (delay from GPIO control and ADC connections times)

float readBatLevel() {
  // First working version - seems a bit low, 3,77 V
    // pinMode(ADC_CTRL,OUTPUT);
    // // digitalWrite(ADC_CTRL, LOW);
    // digitalWrite(ADC_CTRL, HIGH);
    // delay(ADC_READ_STABILIZE); // let GPIO stabilize
    // int analogValue = analogRead(Read_VBAT_Voltage);
    // // Calculate battery voltage using ADC value, reference voltage, and resistor divider
    // // VADC_IN1 = analogValue / 4095 * 3.3
    // // VBAT = VADC_IN1 * 4.9
    // float vadc = (float)analogValue / 4095.0 * 3.3;
    // float voltage = vadc * 4.9;
    // return voltage;

    // Perplexity 1 - Result: 1,35 V
    // pinMode(ADC_CTRL, OUTPUT);
    // digitalWrite(ADC_CTRL, HIGH);  // V3.2 benötigt HIGH
    // delay(ADC_READ_STABILIZE);
    
    // int analogValue = analogRead(Read_VBAT_Voltage);
    
    // // Kalibrierter Multiplikator für Arduino's Standard ADC_ATTEN_DB_11
    // // Beginne mit diesem Wert und verfeinere durch Vergleich mit Multimeter
    // float voltage = analogValue * 0.00141;
    
    // return voltage;

    // Perplexity 2 - Result: 4,03 V
    // pinMode(ADC_CTRL, OUTPUT);
    // digitalWrite(ADC_CTRL, HIGH);
    // delay(ADC_READ_STABILIZE);
    
    // int analogValue = analogRead(Read_VBAT_Voltage);
    
    // // Für ADC_0db: effektiver Bereich ist 0-950mV
    // // Formel: voltage = (analogValue / 4095) * 0.95 * 4.9
    // float voltage = analogValue * 0.001137;
    
    // return voltage;

    // Perplexity 3 - Result: 4,05 V
    pinMode(ADC_CTRL, OUTPUT);
    digitalWrite(ADC_CTRL, HIGH);
    delay(ADC_READ_STABILIZE);
    
    // ESP32's kalibrierte Spannungsmessung verwenden
    int millivolts = analogReadMilliVolts(Read_VBAT_Voltage);
    
    // In tatsächliche Batteriespannung umwandeln
    float voltage = (millivolts / 1000.0) * 4.9;
    
    return voltage;
}

void setup() {
    // Perplexity 2
    // ADC für optimalen Batteriespannungsbereich konfigurieren
    // analogSetPinAttenuation(Read_VBAT_Voltage, ADC_0db);  // 0-950mV Bereich
    // // analogSetWidth(12);  // 12-Bit Auflösung

    Serial.begin(115200);

    // initialize OLED
    VextON();
    delay(100);
    display.init();
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 20, "Battery Test");
    display.display();
    delay(2000);
}

void loop() {
float v;
v = readBatLevel();
display.clear();
display.drawString(0, 20, String(v) + " Volt");
display.display();
delay(5000);
}