# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_GEARSHIFT_UDS_ADDRESS_PROVIDER gearshift_uds_address_provider.c)
zephyr_library_sources_ifdef(CONFIG_GEARSHIFT_CAN_LOG_ADDRESS_PROVIDER gearshift_can_log_address_provider.c)
