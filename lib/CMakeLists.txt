# Copyright (C) Frickly Systems GmbH
# Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_ISO14229 iso14229)
add_subdirectory_ifdef(CONFIG_UDS_NEW uds_new)
if(CONFIG_UDS AND NOT CONFIG_UDS_USE_NEW_LIB)
    add_subdirectory(uds)
endif()
add_subdirectory_ifdef(CONFIG_CAN_ROUTER can_router)
