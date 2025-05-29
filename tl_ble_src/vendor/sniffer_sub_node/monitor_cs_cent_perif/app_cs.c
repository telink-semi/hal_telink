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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app.h"
#include "app_cs.h"
#include "app_buffer.h"
#include "app_sub_node.h"
#include "math.h"

#include "algorithm/hadm/gcc10/cs_cal.h"


#if (MAIN_NODE_ROLE_SELECT == MAIN_NODE_CS_PERIPHERAL_CENTRAL)

#ifndef isnan
    #define isnan(d) (d != d)
#endif

#ifndef isinf
    #define isinf(d) (isnan((d - d)) && !isnan(d))
#endif

typedef enum
{
    INITIATOR_ROLE = 0,
    REFLECTOR_ROLE = 1,
} app_ranging_role_t;

app_cs_config_t app_cs_config[APP_CS_CONFIG_NUM] = {0};

/**
 * @brief      BLE CS config complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_config_complete_event_handle(u8 *p)
{
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s\r\n", __FUNCTION__);
    hci_le_csConfigCompleteEvt_t *ptr = (hci_le_csConfigCompleteEvt_t *)p;

    #if (CS_TLK_ALGO2_EN)
//    blc_Algo2_CopyConfigCompleteData(&ptr->Main_Mode, sizeof(hci_le_csConfigCompleteEvt_t)-6);
    #endif
}

/**
 * @brief      BLE CS procedure enable complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_procedure_enable_complete_event_handle(u8 *p)
{
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s\r\n", __FUNCTION__);
    hci_le_csProcedureEnableCompleteEvt_t *ptr = (hci_le_csProcedureEnableCompleteEvt_t *)p;

    #if (CS_TLK_ALGO2_EN)
//    blc_Algo2_CopyProcedureEnableCompleteData(ptr->Tone_Antenna_Config_Selection, ptr->Selected_TX_Power, ptr->Subevent_Len[0] | (ptr->Subevent_Len[1] << 8) | (ptr->Subevent_Len[2] << 16), ptr->Subevents_Per_Event, ptr->Event_Interval, ptr->Procedure_Interval, ptr->Procedure_Count);
    #endif
}

/**
 * @brief      BLE CS subevent result event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_event_handle(u8 *p)
{
    //tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s\r\n", __FUNCTION__);
    hci_le_csSubeventResultEvt_t *ptr = (hci_le_csSubeventResultEvt_t *)p;

    #if (APP_CS_SUBEVENT_LOG_EN)
    hci_le_csSubeventResultEvt_t *pCsSubevent = (hci_le_csSubeventResultEvt_t *)p;

    /* print subevent data to txt file to calculate distance with other company */
    u8 tempBuff[258];
    tempBuff[0] = 0x04; //type
    tempBuff[1] = 0x3E; //event_code
    tempBuff[2] = n;    // total_len
    blc_app_memory_copy(tempBuff + 3, &pCsSubevent->Subevent_Code, n, sizeof(tempBuff), 0x12280000 | __LINE__);
    tlkapi_printf(APP_CS_SUBEVENT_LOG_EN, "subevent len***:%d\r\n", n);
    tlkapi_send_string_data(APP_CS_SUBEVENT_LOG_EN, "cs subevent", tempBuff, n + 3);
    #endif
}

/**
 * @brief      BLE CS subevent result continue event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_continue_event_handle(u8 *p)
{
    //tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s\r\n", __FUNCTION__);
    hci_le_csSubeventResultContinueEvt_t *ptr = (hci_le_csSubeventResultContinueEvt_t *)p;

    #if (APP_CS_SUBEVENT_LOG_EN)
    hci_le_csSubeventResultContinueEvt_t *pCsSubevent = (hci_le_csSubeventResultContinueEvt_t *)p;

    /* print subevent continue data to txt file to calculate distance with other company */
    u8 tempBuff[258];
    tempBuff[0] = 0x04;
    tempBuff[1] = 0x3E; //event_code
    tempBuff[2] = n;    // total_len
    blc_app_memory_copy(tempBuff + 3, &pCsSubevent->Subevent_Code, n, sizeof(tempBuff), 0x12290000 | __LINE__);
    tlkapi_printf(APP_CS_SUBEVENT_LOG_EN, "continue subevent len***:%d\r\n", n);
    tlkapi_send_string_data(APP_CS_SUBEVENT_LOG_EN, "cs continue subevent", tempBuff, n + 3);
    #endif
}

/**
 * @brief      Get cs config buffer by acl connect handle and config ID
 * @param[in]  connhandle ACL connect handle
 * @param[in]  Config_ID  config ID
 * @return     Pointer to unused CS config buffer
 */
app_cs_config_t *blc_getCSConfig(u16 connHandle, u8 Config_ID)
{
    int idx = 0;
    for (idx = 0; idx < APP_CS_CONFIG_NUM; idx++) {
        if (app_cs_config[idx].Connection_Handle == connHandle && app_cs_config[idx].Config_ID == Config_ID && app_cs_config[idx].valid == TRUE) {
            return app_cs_config + idx;
        }
    }
    return NULL;
}

/**
 * @brief       for calculate the ranging data of one procedure.
 * @param[in]   connHandle: ACL handle..
 * @param[in]   *pData: ranging data
 * @param[out] *distance: if it is mode1, only one distance.
 *                        if it is mode2, could be 1~3 distances return.
 * @return      0     - the result of distance is valid.
 *              other - the result of distance is invalid.
 */
s32 blc_calcRangData(u16 connHandle, u8 *pRangingData, float *distance)
{
    blc_rasc_ranging_data_evt_t *rangingData    = (blc_rasc_ranging_data_evt_t *)pRangingData;
    u16                          rangingCounter = rangingData->rangingCounter;
    //tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] rangingCounter=%d\r\n", rangingCounter);

    s32 retVal = 0;

    // 1.check procedure data. delete abort subevent data. restore remote step data(protocol to procedure)
    blt_ras_proc_ctrl_t remoteProcCtrl;
    blt_ras_proc_ctrl_t localProcCtrl;
    memset(&remoteProcCtrl, 0, sizeof(blt_ras_proc_ctrl_t));
    memset(&localProcCtrl, 0, sizeof(blt_ras_proc_ctrl_t));
    retVal = blc_restoreProcedureData(connHandle, &remoteProcCtrl, &localProcCtrl, pRangingData);
    if (retVal) {
        return retVal;
    }

    // 2.calculate distance
    blt_ras_proc_ctrl_t *localProcedure = blc_getLocalProcedureData(connHandle, rangingCounter);
    app_cs_config_t     *cfg            = blc_getCSConfig(connHandle, localProcedure->procedureHead.data.proCountCfgID);
    if (cfg->Role == INITIATOR_ROLE) {
        retVal = csCalculateDistance(connHandle, &localProcCtrl, &remoteProcCtrl, cfg->Main_Mode, distance);
    } else {
        retVal = csCalculateDistance(connHandle, &remoteProcCtrl, &localProcCtrl, cfg->Main_Mode, distance);
    }

    return retVal;
}

float lastValidDist     = 0;
float lastValidFiltDist = 0;

/**
 * @brief      CS procedure data ready to calculate distance.
 * @param[in]  connHandle       ACL connect handle
 * @param[in]  *pData           ranging data
 * @param[in]  dataLen          length of data
 * @return     0x00
 */
static int app_cs_procedure_data(u16 connHandle, u8 *pData, u16 dataLen)
{
    (void)dataLen;

    DBG_SNIF_CHN9_HIGH;

    blc_rasc_ranging_data_evt_t *rangingData    = (blc_rasc_ranging_data_evt_t *)pData;
    u16                          rangingCounter = rangingData->rangingCounter;

    float distance1       = 0;
    float distance2       = 0;
    float dis1_filt_first = 0;
    float dis2_filt_first = 0;
    float dis1_filt       = 0;
    float dis2_filt       = 0;

    float distance[CS_DISTANCE_TYPE_SUPPORT_MAX] = {0.0f};
    extern s32 blc_calcRangData(u16 connHandle, u8 * pRangingData, float *distance);
    s32        retval = blc_calcRangData(connHandle, pData, distance);

    if (retval == CS_DIST_SUCCESS) {
        #if (1)
        snif_sub_node_cs_distacne_process(connHandle, rangingCounter, distance[0], distance[1]);
        #else
            #if (MEDIAN_FILTER_ENABLE) // MEDIAN_FILTER_ENABLE
        dis1_filt = medianFilterRealTime(distance1, medianWin, &median_count, MEDIAN_WIN_SIZE);
        dis2_filt = medianFilterRealTime(distance2, medianWin, &median_count, MEDIAN_WIN_SIZE);
            #endif


            #if (KALMAN_FILTER_ENABLE)     // KALMAN_FILTER_ENABLE

        if (isnan(distance1)) {
            distance1 = lastValidDist;
        } else {
            lastValidDist = distance1;
        }

        dis1_filt_first = filtFirst(distance1);
        dis2_filt_first = filtFirst(distance2);

        dis1_filt = kalmanFilter_update(kf1, dis1_filt_first);
        dis2_filt = kalmanFilter_update(kf2, dis2_filt_first);

        if (isnan(dis1_filt)) {
            dis1_filt = lastValidFiltDist;
        } else {
            lastValidFiltDist = dis1_filt;
        }
            #endif
        //        tlkapi_printf(1, "dis: %f, %f; dis filter1: %f, %f; kalman: %f, %f",
        //                distance1,distance2,dis1_filt_first,dis2_filt_first,dis1_filt, dis2_filt);

        tlkapi_printf(APP_CS_LOG_EN, "Phase: %f, dis filter1: %f; kalman: %f\r\n", distance1, dis1_filt_first, dis1_filt);

        tlkapi_printf(APP_CS_LOG_EN, "MUSIC: %f, dis filter2: %f; kalman: %f\r\n", distance2, dis2_filt_first, dis2_filt);

        #endif
    } else {
        if (retval == CS_DIST_ERR_STEPS_NUMS_ZEROS) {
            tlkapi_send_string_u32s(APP_CS_LOG_EN, "[APP][CS] distance error, steps number zero", retval);
        #if UI_CONTROL_ENABLE
            app_parse_printf("distance error, steps number zero\r\n");
        #endif
        } else if (retval == CS_DIST_ERR_STEPS_NUM_NOT_ENOUGH) {
            tlkapi_send_string_u32s(APP_CS_LOG_EN, "[APP][CS] distance error, steps number not enough", retval);
        #if UI_CONTROL_ENABLE
            app_parse_printf("distance error, steps number not enough\r\n");
        #endif
        } else if (retval == CS_DIST_ERR_RAS_RANGING_DATA_WRONG) {
            tlkapi_send_string_u32s(APP_CS_LOG_EN, "[APP][CS] distance error, ranging data error", retval);
        #if UI_CONTROL_ENABLE
            app_parse_printf("distance error, ranging data error\r\n");
        #endif
        } else if (retval == CS_DIST_ERR_ALGO_MASK_NOT_SET) {
            tlkapi_send_string_u32s(APP_CS_LOG_EN, "distance error, ranging data error", retval);
        #if UI_CONTROL_ENABLE
            app_parse_printf("distance error, algorithm mask not set\r\n");
        #endif
        } else {
            tlkapi_send_string_u32s(APP_CS_LOG_EN, "[APP][CS] distance error, unknown reason", retval);
        #if UI_CONTROL_ENABLE
            app_parse_printf("distance error, unknown reason\r\n");
        #endif
        }
    }
    DBG_SNIF_CHN9_LOW;
    return 0;
}

#if (0)
    static int app_cs_local_ranging_data(u16 connHandle, u8 *pData, u16 dataLen)
    {
        (void)connHandle;
        (void)dataLen;
        blc_rasc_local_ranging_data_evt_t *evt = (blc_rasc_local_ranging_data_evt_t *)pData;
        u16 rangingCounter;
        BYTE_TO_UINT16(rangingCounter, evt->dataPtr);
        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] app_cs_local_ranging_data CB: connHandle=0x%x, dataLen=%d, rangingCounter=%d", evt->connHandle, evt->dataLen, rangingCounter);
        tlkapi_send_string_data(APP_CS_LOG_EN, "APP][CS] app_cs_local_ranging_data CB data is ", evt->dataPtr, evt->dataLen);
        return 0;
    }

    static const app_prf_evtCb_t csClientEvt[] = {
        {CS_EVT_LOCAL_RANGING_DATA, app_cs_local_ranging_data},
    };
    PRF_EVT_CB(csClientEvt)

    #define HOST_MALLOC_BUFF_SIZE      ((RAS_PROCEDURE_COUNT * 2 + 1) * PROCEDURE_DATA_LEN + 4 * 1024)

    static u8 hostMallocBuffer[HOST_MALLOC_BUFF_SIZE];
#endif

/**
 * @brief      BLE channel sounding initialize.
 * @param      None
 * @return     None
 */
void app_channel_sounding_init(void)
{
    //Initialize local capabilities
    chn_sound_capabilities_t appCsLocalSupportCap = {
        .Num_Config_Supported                 = 1, //range 1-4
        .max_consecutive_procedures_supported = 0,
        .Num_Antennas_Supported               = NUM_ANT_SUPPORT,
        .Max_Antenna_Paths_Supported          = MAX_ANT_PATHS_SUPPORT,

        .Roles_Supported                   = CS_ROLE_DISABLE,
        .Mode_Types                        = 0,   //mandatory mode1 and mode 2
        .RTT_Capability                    = 0,   //150ns
        .RTT_AA_Only_N                     = 240,
        .RTT_Sounding_N                    = 240,
        .RTT_Random_Payload_N              = 240,
        .Optional_NADM_Sounding_Capability = 0,
        .Optional_NADM_Random_Capability   = 0,
        .Optional_CS_SYNC_PHYs_Supported   = 0 | BIT(1),  //just mandatory 1M PHY
        .Optional_Subfeatures_Supported    = 0,
        .Optional_T_IP1_Times_Supported    = 0,  //only support 145us
        .Optional_T_IP2_Times_Supported    = 0,  //only support 145us
        .Optional_T_FCS_Times_Supported    = 0,  //only support 150us
        .Optional_T_PM_Times_Supported     = CS_T_PM_20US,
        .T_SW_Time_Supported               = 10, //10us
        .Optional_TX_SNR_Capability        = 0xff,
    };
    blc_ll_initCsInitiatorModule(&appCsLocalSupportCap);
    blc_ll_initCsReflectorModule(&appCsLocalSupportCap);

    //Initialize CS buffer
    blc_ll_initCsConfigParam(app_CsConfigParam, APP_CS_CONFIG_NUM);
    blc_ll_initCsRxFifo(app_cs_rx_buf, CS_RX_FIFO_SIZE, CS_RX_FIFO_NUM);

    //Load calibration table for RTT.
    blc_loadCsCali_table(flash_sector_calibration + CALIB_OFFSET_CALI_TABLE_HEADER_INFO);

#if (ANTENNA_SWITCHING_AUTO_EN)
    //Initialize multi-antenas
    cs_ant_switch_config_t ant_cfg = {
        .ant_default_seq_value = 0,
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

    //Set cs use tx power level
    blc_cs_set_tx_power_level(CS_USE_TX_POWER_LEVEL);

#if (0)
    //Initialize RAS client
    blc_prf_initialModule(app_prf_eventCb, hostMallocBuffer, HOST_MALLOC_BUFF_SIZE);
    blc_rap_registerRasProfileControlClient(NULL);
#endif

    //Init cs ranging log
    blc_cs_initRangingLog();
}

#endif
