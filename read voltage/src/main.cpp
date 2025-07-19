#include <Wire.h>
#include "HT_SSD1306Wire.h"
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
    pinMode(ADC_CTRL,OUTPUT);
    // digitalWrite(ADC_CTRL, LOW);
    digitalWrite(ADC_CTRL, HIGH);
    delay(ADC_READ_STABILIZE); // let GPIO stabilize
    int analogValue = analogRead(Read_VBAT_Voltage);
    // Calculate battery voltage using ADC value, reference voltage, and resistor divider
    // VADC_IN1 = analogValue / 4095 * 3.3
    // VBAT = VADC_IN1 * 4.9
    float vadc = (float)analogValue / 4095.0 * 3.3;
    float voltage = vadc * 4.9;
    return voltage;
}

void setup() {
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