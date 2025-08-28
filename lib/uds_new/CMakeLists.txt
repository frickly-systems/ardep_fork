# Copyright (C) Frickly Systems GmbH
# Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# Define base sources
set(UDS_SOURCES
    uds.c
    read_data_by_identifier.c
    write_memory_by_addr_impl.c
)

# Conditionally add ecu_reset.c
if(CONFIG_UDS_NEW_ENABLE_RESET)
    list(APPEND UDS_SOURCES ecu_reset.c)
endif()

zephyr_library_sources(${UDS_SOURCES})

zephyr_linker_sources(SECTIONS iterables.ld)

zephyr_include_directories(.)