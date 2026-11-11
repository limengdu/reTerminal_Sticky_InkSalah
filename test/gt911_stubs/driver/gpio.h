#pragma once
using gpio_num_t = int;
constexpr int GPIO_MODE_OUTPUT = 1, GPIO_MODE_INPUT = 0;
int gpio_set_direction(gpio_num_t, int);
int gpio_set_level(gpio_num_t, int);
int gpio_hold_dis(gpio_num_t);
