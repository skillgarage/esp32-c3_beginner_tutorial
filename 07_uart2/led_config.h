#pragma once

#include "driver/gpio.h"
#include <stdbool.h>

#define LED_GPIO GPIO_NUM_6

void led_init(void);
void led_set(bool on);
