/********************************************************************************************************
 * @file    app_cs.c
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
#include "stack/ble/ble.h"
#include "app_cs.h"
#if (FREERTOS_ENABLE)
#include "app_port_freertos.h"
#endif

/**
 * @brief      BLE CS  read remote support capabilities complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_read_remote_support_capabilities_complete_event_handle(u16 aclHandle, u8 *p, u16 n)
{
    u8                                      pcmd[sizeof(hci_le_cs_setDefaultSetting_cmdParam_t)] = {0};
    u8                                      pret[sizeof(hci_le_cs_setDefaultSetting_retParam_t)] = {0};
    hci_le_cs_setDefaultSetting_cmdParam_t *para                                                 = (hci_le_cs_setDefaultSetting_cmdParam_t *)pcmd;
    hci_le_readRemoteSupCapCompleteEvt_t   *preadRemoteCapComplete                               = (hci_le_readRemoteSupCapCompleteEvt_t *)p;

    para->Connection_Handle         = preadRemoteCapComplete->Connection_Handle;
    para->Role_Enable               = CS_REFLECTOR_ROLE;
    para->Max_TX_Power              = 0;
    para->CS_SYNC_Antenna_Selection = 1;

    blc_hci_le_cs_setDefaultSettings(para, (hci_le_cs_setDefaultSetting_retParam_t *)pret);
}

/**
 * @brief      BLE CS config complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_config_complete_event_handle(u16 aclHandle, u8 *p, u16 n)
{
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s", __FUNCTION__);
    hci_le_csConfigCompleteEvt_t *ptr = (hci_le_csConfigCompleteEvt_t *)p;

    blc_rap_csConfigComplete(ptr);
}

/**
 * @brief      BLE CS procedure enable complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_procedure_enable_complete_event_handle(u16 aclHandle, u8 *p, u16 n)
{
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s", __FUNCTION__);
    hci_le_csProcedureEnableCompleteEvt_t *ptr = (hci_le_csProcedureEnableCompleteEvt_t *)p;

    blc_ras_csProcedureEnComplete(ptr); //inform ras data layer
}

/**
 * @brief      BLE CS subevent result event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_event_handle(u16 aclHandle, u8 *p, u16 n)
{
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s", __FUNCTION__);
    hci_le_csSubeventResultEvt_t *ptr = (hci_le_csSubeventResultEvt_t *)p;
    blc_ras_csSubeventResultData(ptr);
}

/**
 * @brief      BLE CS subevent result continue event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_continue_event_handle(u16 aclHandle, u8 *p, u16 n)
{
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s", __FUNCTION__);
    hci_le_csSubeventResultContinueEvt_t *ptr = (hci_le_csSubeventResultContinueEvt_t *)p;
    blc_ras_csSubeventResultContinueData(ptr);
}

static int app_cs_local_ranging_data(u16 connHandle, u8 *pData, u16 dataLen)
{
    (void)connHandle;
    (void)dataLen;
    blc_rasc_local_ranging_data_evt_t *evt = (blc_rasc_local_ranging_data_evt_t *)pData;
    tlkapi_printf(APP_CS_LOG_EN, "connHandle is %d, length is %d", evt->connHandle, evt->dataLen);
    tlkapi_send_string_data(APP_CS_LOG_EN, "value is ", evt->dataPtr, evt->dataLen);
    return 0;
}

static const app_prf_evtCb_t csPeripheralEvt[] = {
    {CS_EVT_LOCAL_RANGING_DATA, app_cs_local_ranging_data},
};
