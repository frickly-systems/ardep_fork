# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_GEARSHIFT)
  zephyr_library()
  zephyr_library_sources(gearshift.c)
endif()
