/********************************************************************************************************
 * @file    aics_client_buf.c
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


const int gAppAicsCltInstNum = ACL_CENTRAL_MAX_NUM*APP_AUDIO_AICS_CLIENT_MAX_INSTANCE_NUM;

_attribute_ble_data_retention_
blc_aics_client_t gAicsClient[ACL_CENTRAL_MAX_NUM*APP_AUDIO_AICS_CLIENT_MAX_INSTANCE_NUM];


/**
 * @brief       aics get client control buffer.
 * @param[in]   instIndx: ACL connect index.
 * @return      aics client control buffer pointer.
 */
blc_aics_client_t *blt_aicsc_getClientControlBuffer(u16 connHandle, u16 startHandle, u16 endHandle)
{
    for(int i=0; i<gAppAicsCltInstNum; i++) {
        if(gAicsClient[i].connHandle == connHandle &&
            gAicsClient[i].ntfInput.startHdl == startHandle &&
            gAicsClient[i].ntfInput.endHdl == endHandle)
            return &gAicsClient[i];
    }

    for(int i=0; i<gAppAicsCltInstNum; i++) {
        if(gAicsClient[i].useFlag)
            continue;
        gAicsClient[i].useFlag = true;
        gAicsClient[i].connHandle = connHandle;
        gAicsClient[i].ntfInput.startHdl = startHandle;
        gAicsClient[i].ntfInput.endHdl = endHandle;
        return &gAicsClient[i];
    }
    return NULL;
}

/**
 * @brief       aics clean all client control buffer.
 * @param[in]   none.
 * @return      none.
 */
void blt_aicsc_cleanAllClientControlBuffer(void)
{
    memset(gAicsClient, 0, sizeof(blc_aics_client_t)*gAppAicsCltInstNum);
}




