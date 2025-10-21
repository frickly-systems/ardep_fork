# Copyright (C) Frickly Systems GmbH
# Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hardware_test)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

if(CONFIG_SUT)
    FILE(GLOB sut_sources src/sut/*.c)
    target_sources(app PRIVATE ${sut_sources})
elseif(CONFIG_TESTER)
    FILE(GLOB tester_sources src/tester/*.c)
    target_sources(app PRIVATE ${tester_sources})
endif()

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)
zephyr_nanopb_sources(app 
    src/data.proto
)
