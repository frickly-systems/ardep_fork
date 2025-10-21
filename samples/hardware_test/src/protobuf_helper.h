/**
 * Copyright (c) Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "src/data.pb.h"
#include "util.h"

#include <string.h>

bool encode_string(pb_ostream_t *stream,
                   const pb_field_t *field,
                   void *const *arg);

int request_from_proto(const Request *proto);

int response_to_proto(const void *response,
                      uint8_t *buffer,
                      size_t buffer_size,
                      size_t *message_length);
