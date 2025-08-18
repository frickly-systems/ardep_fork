# Copyright (C) Frickly Systems GmbH
# Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(iso14229_common.c uds_new.c)

zephyr_linker_sources(SECTIONS iterables.ld)

zephyr_include_directories(.)