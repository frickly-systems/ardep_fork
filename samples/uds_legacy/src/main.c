/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#if !CONFIG_UDS_LEGACY
#error "This sample requires the CONFIG_UDS_LEGACY option enabled"
#endif

int main(void) { printk("Hello UDS!\n"); }
