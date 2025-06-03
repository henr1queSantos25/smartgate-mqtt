#include "pico/stdlib.h"

#define LED_GREEN 11
#define LED_BLUE 12
#define LED_RED 13


void setupLED(uint led);
void setup_pwm_led(uint led);
void setLeds(bool r, bool g, bool b);
void piscar_led(uint led);
void atualizar_fade_led(uint led);