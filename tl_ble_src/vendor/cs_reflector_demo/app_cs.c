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
#include "app_buffer.h"
#if (FREERTOS_ENABLE)
#include "app_port_freertos.h"
#endif

/**
 * @brief      BLE CS  read remote support capabilities complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_read_remote_support_capabilities_complete_event_handle(u8 *p)
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
void app_le_cs_config_complete_event_handle(u8 *p)
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
void app_le_cs_procedure_enable_complete_event_handle(u8 *p)
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
void app_le_cs_subevent_result_event_handle(u8 *p)
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
void app_le_cs_subevent_result_continue_event_handle(u8 *p)
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
PRF_EVT_CB(csPeripheralEvt)

#define HOST_MALLOC_BUFF_SIZE ((RAS_PROCEDURE_COUNT + 1) * PROCEDURE_DATA_LEN + 4 * 1024)

static u8 hostMallocBuffer[HOST_MALLOC_BUFF_SIZE];

/**
 * @brief      BLE channel sounding initialize.
 * @param      None
 * @return     None
 */
void app_channel_sounding_init(void)
{
    //Initialize local capabilities
    chn_sound_capabilities_t appCsLocalSupportCap = {
        .Num_Config_Supported                 = 1,
        .max_consecutive_procedures_supported = 0,
        .Num_Antennas_Supported               = NUM_ANT_SUPPORT,
        .Max_Antenna_Paths_Supported          = MAX_ANT_PATHS_SUPPORT,
        .Roles_Supported                      = CS_REFLECTOR_ROLE,
        .Mode_Types                           = 0, //mandatory mode1 and mode 2
        .RTT_Capability                       = 0, //150ns
        .RTT_AA_Only_N                        = 240,
        .RTT_Sounding_N                       = 240,
        .RTT_Random_Payload_N                 = 240,
        .Optional_NADM_Sounding_Capability    = 0,
        .Optional_NADM_Random_Capability      = 0,
        .Optional_CS_SYNC_PHYs_Supported      = 0 | BIT(1), //just mandatory 1M PHY
        .Optional_Subfeatures_Supported       = 0,
        .Optional_T_IP1_Times_Supported       = 0, //only support 145us
        .Optional_T_IP2_Times_Supported       = 0, //only support 145us
        .Optional_T_FCS_Times_Supported       = 0, //only support 150us
        .Optional_T_PM_Times_Supported        = CS_T_PM_20US,
        .T_SW_Time_Supported                  = 10, //10us
        .Optional_TX_SNR_Capability           = 0xff,
    };
    blc_ll_initCsReflectorModule(&appCsLocalSupportCap);

    //Initialize CS buffer
    blc_ll_initCsConfigParam(app_CsConfigParam, APP_CS_CONFIG_NUM);
    blc_ll_initCsRxFifo(app_cs_rx_buf, CS_RX_FIFO_SIZE, CS_RX_FIFO_NUM);

    //Load calibration table for RTT.
    blc_loadCsCali_table(flash_sector_calibration + CALIB_OFFSET_CALI_TABLE_HEADER_INFO);

#if (ANTENNA_SWITCHING_AUTO_EN)
    //Initialize multi-antenas
    cs_ant_switch_config_t ant_cfg = {
        .ant_default_seq_value   = 0,
        .ant_ctrl_seq_base_value = ANTENNA_SWITCHING_CTRL_BASE,
    };

    rf_cs_ant_switch_ctrl ant_switch_ctrl[] = {
            ANTENNA_SWITCHING_SEL_0_PIN, ATSEL_0,
            ANTENNA_SWITCHING_SEL_1_PIN, ATSEL_1,
            #if(BOARD_SELECT != BOARD_721X_EVK_CIT314A102)
            ANTENNA_SWITCHING_SEL_2_PIN, ATSEL_2,
            #endif
    };
    blc_cs_antenna_switch_config_init(&ant_cfg);
    rf_cs_ant_switch_pin_init(ant_switch_ctrl, sizeof(ant_switch_ctrl)/sizeof(rf_cs_ant_switch_ctrl));
#endif

#if (MCU_CORE_TYPE == MCU_CORE_TL721X)
    blc_cs_disableGpioPinsFromD4ToD7();
#endif

    //Set cs use tx power level
    blc_cs_set_tx_power_level(CS_USE_TX_POWER_LEVEL);

    //RAS server initial
    blc_prf_initialModule(app_prf_eventCb, hostMallocBuffer, HOST_MALLOC_BUFF_SIZE);
    const blc_rass_regParam_t rasParam = {
#if (GOOGLE_CS_REFL_ROLE_EN)
        .ras_feature.realTimeProcedureDataSupport = 1,
#else
        .ras_feature.realTimeProcedureDataSupport = 0,
#endif
        .ras_feature.getLostProcedureDataSegmentsSupport = 1,
        .ras_feature.abortOperationSupport               = 1,
        .ras_feature.filterProcedureDataSupport          = 0,
    };
    blc_rap_registerRasProfileControlServer(&rasParam);
}
