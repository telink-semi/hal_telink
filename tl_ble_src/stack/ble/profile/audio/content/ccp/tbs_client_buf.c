/********************************************************************************************************
 * @file    tbs_client_buf.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"



_attribute_ble_data_retention_
blc_ccp_client_t gCcp[ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM];


/**
 * @brief       CCP(GTBS/TBS) get client control buffer.
 * @param[in]   instIndx: ACL connect index.
 * @return      CCP(GTBS/TBS) client control buffer pointer.
 */
blc_ccp_client_t *blt_ccp_getClientControlBuffer(u8 instIdx)
{
    assert(instIdx < gAppAudioAclMaxNum);

    return &gCcp[instIdx];
}


