/********************************************************************************************************
 * @file    app_audio_volume.c
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

#if (INTER_TEST_MODE == TEST_LE_AUDIO_SWTICH_SERVER)

/**
 * @brief       Volume event callback in APP layer,used to inform user about 'Volume level'
 * @param[in]   connHandle - ACL connect handle.
 * @param[in]   evtID      - Volume event ID.
 * @param[in]   pData      - Additional data.
 * @param[in]   dataLen    - Additional data length.
 * @return      none
 */
void app_volume_event_callback(u16 connHandle, int evtID, u8 *pData, u16 dataLen)
{
    switch(evtID)
    {
        case AUDIO_EVT_VCSS_CHANGED_VOLUME_STATE:
        {
            blc_vcss_volumeStateChangeEvt_t* pEvt = (blc_vcss_volumeStateChangeEvt_t*)pData;
            if(pEvt->mute)
            {
                //codec mute
                tlkapi_printf(APP_LOG_EN,"volume mute");
            }
            else
            {
                //dac configure volume
                tlkapi_printf(APP_LOG_EN,"volume setting %d",pEvt->volumeSetting);
            }
        }
    }
}
#endif
