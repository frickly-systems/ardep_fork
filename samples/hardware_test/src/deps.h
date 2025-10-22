#pragma once

#include "util.h"

#include <stddef.h>
#include <stdint.h>

#if CONFIG_SUT
#define LOG_MODULE_NAME sut
#define DEVICE_ROLE DEVICE_ROLE__SUT
#elif CONFIG_TESTER
#define LOG_MODULE_NAME tester
#define DEVICE_ROLE DEVICE_ROLE__TESTER
#else
#error "Either CONFIG_SUT or CONFIG_TESTER must be defined"
#endif

extern char logs[5000];

int setup_gpio_test(const struct Request* request, struct Response* response);

int execute_gpio_test(const struct Request* request, struct Response* response);

int stop_gpio_test(const struct Request* request, struct Response* response);

int setup_uart_test(const struct Request* request, struct Response* response);

int execute_uart_test(const struct Request* request, struct Response* response);

int stop_uart_test(const struct Request* request, struct Response* response);