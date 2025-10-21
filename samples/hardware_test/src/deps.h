#pragma once

#include "util.h"

#include <stddef.h>
#include <stdint.h>

#if CONFIG_SUT
#define LOG_MODULE_NAME sut
#elif CONFIG_TESTER
#define LOG_MODULE_NAME tester
#else
#error "Either CONFIG_SUT or CONFIG_TESTER must be defined"
#endif

extern char logs[5000];

int setup_gpio_test(const struct Request *request, struct Response *response);

int execute_gpio_test(const struct Request *request, struct Response *response);

int stop_gpio_test(const struct Request *request, struct Response *response);