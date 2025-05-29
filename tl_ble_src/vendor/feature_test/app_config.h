/********************************************************************************************************
 * @file    app_config.h
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


#include "feature_config.h"


#if (FEATURE_TEST_MODE == TEST_LL_MD)
    #include "feature_ll_more_data/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_2M_CODED_PHY_CONNECTION)
    #include "feature_2M_coded_phy/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_LL_DLE)
    #include "feature_dle/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_WHITELIST)
    #include "feature_whitelist/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_GATT_API)
    #include "feature_gatt_api/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_EXT_ADV)
    #include "feature_ext_adv/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_PER_ADV)
    #include "feature_per_adv/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_PER_ADV_SYNC)
    #include "feature_per_adv_sync/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_CIS_CENTRAL)
    #include "feature_cis_master/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_CIS_PERIPHERAL)
    #include "feature_cis_slave/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_BLE_PHY)
    #include "feature_PHY_test/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_BIS_BCST)
    #include "feature_bis_bcst/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_BIS_SYNC)
    #include "feature_bis_sync/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_EXT_SCAN)
    #include "feature_ext_scan/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_MISC_FUNC)
    #include "feature_misc/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_GATTC_SDP)
    #include "feature_gattc_sdp/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_GAPC_SDP)
    #include "feature_gapc_sdp/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_OTA)
    #include "feature_ota/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_SOFT_TIMER)
    #include "feature_soft_timer/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_FEATURE_BACKUP)
    #include "feature_backup/app_config.h"
#elif (FEATURE_TEST_MODE == ISO_TEST_MODE)
    #include "feature_iso_test/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_SMP)
    #include "feature_smp/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_MULTIPLE_LOCAL_DEVICE)
    #include "feature_multi_local_dev/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_SUBRATE_CENTRAL)
    #include "feature_subrate_master/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_SUBRATE_PERIPHR)
    #include "feature_subrate_slave/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_PRIVACY_PERIPHERAL)
    #include "feature_privacy_peripheral/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_PRIVACY_CENTRAL)
    #include "feature_privacy_central/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_PAWR_ADV)
    #include "feature_pawr_adv/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_PAWR_SYNC)
    #include "feature_pawr_sync/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_FEATURE_DEBUG_LOG)
    #include "feature_debug_log/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_L2CAP_COC)
    #include "feature_l2cap_coc/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_CURRENT)
    #include "feature_current/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_AUTO)
    #include "feature_auto_test/app_config.h"
#elif (FEATURE_TEST_MODE == TEST_LONG_RANGE)
    #include "feature_long_range/app_config.h"
#else
#endif
