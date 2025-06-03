#include "pico/stdlib.h"


void setupUltrasonicPins(uint trigPin, uint echoPin);
uint64_t getPulse(uint trigPin, uint echoPin);
uint64_t getCm(uint trigPin, uint echoPin);
uint64_t getInch(uint trigPin, uint echoPin);
uint64_t getCmFiltered(uint trigPin, uint echoPin, int samples);
