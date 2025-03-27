/********************************************************************************************************
 * @file    user_config.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#pragma once

#ifndef __PROJECT_ACL_CONN_DEMO__
#define __PROJECT_ACL_CONN_DEMO__                   0
#endif

#ifndef __PROJECT_ACL_CEN_DEMO__
#define __PROJECT_ACL_CEN_DEMO__                    0
#endif

#ifndef __PROJECT_ACL_PER_DEMO__
#define __PROJECT_ACL_PER_DEMO__                    0
#endif

#ifndef __PROJECT_FEATURE_TEST__
#define __PROJECT_FEATURE_TEST__                    0
#endif

#ifndef __PROJECT_INTERNAL_TEST__
#define __PROJECT_INTERNAL_TEST__                   0
#endif

#ifndef __PROJECT_AUDIO_UNICAST_CLIENT__
#define __PROJECT_AUDIO_UNICAST_CLIENT__            0
#endif

#ifndef __PROJECT_AUDIO_UNICAST_SERVER__
#define __PROJECT_AUDIO_UNICAST_SERVER__            0
#endif

#ifndef __PROJECT_AUDIO_BROADCAST_SOURCE__
#define __PROJECT_AUDIO_BROADCAST_SOURCE__          0
#endif

#ifndef __PROJECT_AUDIO_BROADCAST_SINK__
#define __PROJECT_AUDIO_BROADCAST_SINK__            0
#endif

#ifndef __PROJECT_BLE_CONTROLLER__
#define __PROJECT_BLE_CONTROLLER__                  0
#endif

#ifndef __PROJECT_BQB_CONTROLLER__
#define __PROJECT_BQB_CONTROLLER__                  0
#endif

#ifndef __PROJECT_B91M_CONTROLLER__
#define __PROJECT_B91M_CONTROLLER__                 0
#endif

#ifndef __PROJECT_CIS_CEN__
#define __PROJECT_CIS_CEN__                         0
#endif

#ifndef __PROJECT_CIS_PER__
#define __PROJECT_CIS_PER__                         0
#endif

#ifndef __PROJECT_AUDIO_BROADCAST_ASSISTANT__
#define __PROJECT_AUDIO_BROADCAST_ASSISTANT__       0
#endif

#ifndef __PRODUCT_BIS_SOURCE_DEMO__
#define __PRODUCT_BIS_SOURCE_DEMO__                 0
#endif

#ifndef __PRODUCT_BIS_SINK_DEMO__
#define __PRODUCT_BIS_SINK_DEMO__                   0
#endif

#ifndef __PRODUCT_CIS_SOURCE_DEMO__
#define __PRODUCT_CIS_SOURCE_DEMO__                 0
#endif

#ifndef __PRODUCT_CIS_SINK_DEMO__
#define __PRODUCT_CIS_SINK_DEMO__                   0
#endif

#ifndef __PROJECT_CS_CEN_DEMO__
#define __PROJECT_CS_CEN_DEMO__                     0
#endif

#ifndef __PROJECT_CS_PER_DEMO__
#define __PROJECT_CS_PER_DEMO__                     0
#endif

#ifndef __PROJECT_PRODUCT_UNICAST_SERVER__
#define __PROJECT_PRODUCT_UNICAST_SERVER__          0
#endif

#ifndef __PROJECT_SNIF_MAIN_NODE__
#define __PROJECT_SNIF_MAIN_NODE__                  0
#endif

#ifndef __PROJECT_SNIF_SUB_NODE__
#define __PROJECT_SNIF_SUB_NODE__                   0
#endif

#ifndef __PROJECT_PRODUCT_ULL_HID_DEMOE__
#define __PROJECT_PRODUCT_ULL_HID_DEMOE__                   0
#endif

#if (__PROJECT_ACL_CONN_DEMO__)
    #include "vendor/acl_connection_demo/app_config.h"
#elif(__PROJECT_ACL_CEN_DEMO__)
    #include "vendor/acl_central_demo/app_config.h"
#elif(__PROJECT_ACL_PER_DEMO__)
    #include "vendor/acl_peripheral_demo/app_config.h"
#elif (__PROJECT_FEATURE_TEST__)
    #include "vendor/feature_test/app_config.h"
#elif (__PROJECT_INTERNAL_TEST__)
    #include "vendor/internal_test/app_config.h"
#elif (__PROJECT_AUDIO_UNICAST_CLIENT__)
    #include "vendor/audio_unicast_client/app_config.h"
#elif (__PROJECT_AUDIO_UNICAST_SERVER__)
    #include "vendor/audio_unicast_server/app_config.h"
#elif (__PROJECT_PRODUCT_UNICAST_SERVER__)
    #include "vendor/product_unicast_server/app_config.h"
#elif (__PROJECT_AUDIO_BROADCAST_SOURCE__)
    #include "vendor/audio_broadcast_source/app_config.h"
#elif (__PROJECT_AUDIO_BROADCAST_SINK__)
    #include "vendor/audio_broadcast_sink/app_config.h"
#elif(__PROJECT_BLE_CONTROLLER__)
    #include "vendor/ble_controller/app_config.h"
#elif(__PROJECT_BQB_CONTROLLER__)
    #include "vendor/BQB_controller/app_config.h"
#elif (__PROJECT_CIS_CEN__)
    #include "vendor/cis_central/app_config.h"
#elif (__PROJECT_CIS_PER__)
    #include "vendor/cis_peripheral/app_config.h"
#elif(__PROJECT_AUDIO_BROADCAST_ASSISTANT__)
    #include "vendor/audio_broadcast_assistant/app_config.h"
#elif(__PRODUCT_BIS_SOURCE_DEMO__)
    #include "vendor/product_bis_source/app_config.h"
#elif(__PRODUCT_BIS_SINK_DEMO__)
    #include "vendor/product_bis_sink/app_config.h"
#elif(__PRODUCT_CIS_SOURCE_DEMO__)
    #include "vendor/product_cis_source/app_config.h"
#elif(__PRODUCT_CIS_SINK_DEMO__)
    #include "vendor/product_cis_sink/app_config.h"
#elif (__PROJECT_CS_CEN_DEMO__)
    #include "vendor/cs_acl_central_demo/app_config.h"
#elif (__PROJECT_CS_PER_DEMO__)
    #include "vendor/cs_acl_peripheral_demo/app_config.h"
#elif (__PROJECT_SNIF_MAIN_NODE__)
    #include "vendor/sniffer_main_node/node_config.h"
#elif (__PROJECT_SNIF_SUB_NODE__)
    #include "vendor/sniffer_sub_node/node_config.h"
#elif (__PROJECT_PRODUCT_ULL_HID_DEMOE__)
    #include "vendor/product_ull_hid_demo/ull_hid_config.h"
#else
    #include "vendor/common/default_config.h"
#endif

