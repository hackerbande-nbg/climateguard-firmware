#ifndef __LED_H__
#define __LED_H__

#include <Arduino.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void set_led(bool on);
    void toggle_led();
    void setup_led();

#ifdef __cplusplus
}
#endif

#endif //__LED_H__