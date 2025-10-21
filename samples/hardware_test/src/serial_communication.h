#pragma once

#include "util.h"

#include <zephyr/device.h>

typedef int request_response_fn_t(const struct Request *request,
                                  struct Response *response);

int serial_communication_init(const struct device *uart,
                              request_response_fn_t *handler);

int serial_communication_start(void);

int serial_communication_stop(void);