#include "led.h"

#define LED_PIN 35

static bool led_status = false;

void set_led(bool on)
{
    digitalWrite(LED_PIN, on ? HIGH : LOW);
    led_status = on;
}

void toggle_led()
{
    led_status = !led_status;
    set_led(led_status);
}

void setup_led()
{
    pinMode(LED_PIN, OUTPUT);
    set_led(false);
}
