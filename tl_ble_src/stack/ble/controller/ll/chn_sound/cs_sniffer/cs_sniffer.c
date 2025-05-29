/********************************************************************************************************
 * @file    cs_sniffer.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/controller/ble_controller.h"

#include "cs_sniffer.h"
#include "algorithm/hadm/gcc10/cs_cal.h"


#if (LL_CS_SNIFFER_MODE_ENABLE)

_attribute_ble_data_retention_ _attribute_aligned_(4) volatile cs_sniffer_param_t csSniffer_param;

void blc_ll_initCsSnifferMainNode_module(u8 totalNodeNum)
{
    if (totalNodeNum > 7) {
        csSniffer_param.totalNodeNum = 1; //default only main node
    } else {
        csSniffer_param.totalNodeNum = totalNodeNum;
    }

    /* MainNode curNodeIdx must be fixed to 0 */
    csSniffer_param.curNodeIdx = 0;

    csSniffer_param.curProcedureCountIdx = 0;
}

void blc_ll_initCsSnifferSubNode_module(u8 currentNodeIdx)
{
    csSniffer_param.totalNodeNum = 1; //default only main node

    /* SubNode curNodeIdx must be greater than 0 and less than 7 */
    if ((currentNodeIdx == 0) || (currentNodeIdx > 6)) {
        csSniffer_param.curNodeIdx = 1;
    } else {
        csSniffer_param.curNodeIdx = currentNodeIdx;
    }

    csSniffer_param.curProcedureCountIdx = 0;
}

u32 blc_ll_getCsSecurityParam(u16 connHandle, u8 *csSecurityParam)
{
    u8            conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t *pc_conn  = (st_ll_conn_t *)&blms[conn_idx];

    cs_param_t *pCsParam = &pc_conn->csParam;

    smemcpy(csSecurityParam, pCsParam->CS_IV, 16);
    smemcpy(csSecurityParam + 16, pCsParam->CS_IN, 8);
    smemcpy(csSecurityParam + 24, pCsParam->CS_PV, 16);

    tlkapi_send_string_data(DBG_CS_DATA_EN, "[STK][CS] pCsParam->Security", pCsParam->CS_IV, 40);

    return 40;
}

u32 blc_ll_getCsProceduretTimingParam(u16 connHandle, u8 ConfigID, u8 *csProceduretTimingParam)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;

    u8           cfgIdx = blt_ll_getCsConfigById(connHandle, ConfigID);
    cs_config_t *pCsCfg = gCsMng.gGlobal_pCsCfg + cfgIdx;

    smemcpy(csProceduretTimingParam, (u8 *)&pCsCfg->connEventCount, 2);
    smemcpy(csProceduretTimingParam + 2, (u8 *)&pCsCfg->csOft_us, 4);
    smemcpy(csProceduretTimingParam + 6, (u8 *)&pCsCfg->PHY, 1);
    smemcpy(csProceduretTimingParam + 7, (u8 *)&csSniffer_param.totalNodeNum, 1);
    smemcpy(csProceduretTimingParam + 8, (u8 *)&pCsCfg->sch_early_us, 2);
    smemcpy(csProceduretTimingParam + 10, (u8 *)&bltCsLocalSupportCap.T_SW_Time_Supported, 1);
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];
    smemcpy(csProceduretTimingParam + 11, (u8 *)&pc->csRemoteSupCap.T_SW_Time_Supported, 1);

    tlkapi_send_string_u32s(DBG_CS_DATA_EN, "[STK][CS] Sniffer ProcedureTiming", pCsCfg->connEventCount, pCsCfg->csOft_us, pCsCfg->PHY, csSniffer_param.totalNodeNum,
                            pCsCfg->sch_early_us, bltCsLocalSupportCap.T_SW_Time_Supported, pc->csRemoteSupCap.T_SW_Time_Supported);

    return 12;
}

int findIndex(int targetValue, const u8 array[], int arraySize)
{
    for (int i = 0; i < arraySize; i++) {
        if (array[i] == targetValue) {
            return i;
        }
    }

    return -1;
}

int blc_ll_updateCsSnifferParam(u8 *cmd) //cmd - ACL sniffer sync command: refer to 'cs_sniffer_event_param_t'.
{
    int                       err   = CS_SNIFFER_UNKNOWN_SNIFHANDLE;
    cs_sniffer_event_param_t *param = (cs_sniffer_event_param_t *)cmd;

    u8            pc_conn_sel = param->snifHandle & CONN_IDX_MASK;
    st_ll_conn_t *pc_conn     = (st_ll_conn_t *)&blms[pc_conn_sel];

    if (pc_conn->connState != CONN_STATUS_ESTABLISH) {
        return CS_SNIFFER_CURRENT_STATE_NOT_SUPPORTED_THIS_CMD;
    }

    u8         *cs_event_param = param->event_data;
    cs_param_t *pCsParam       = &pc_conn->csParam;

    if (param->event_code == HCI_SUB_EVT_LE_CS_CONFIG_COMPLETE) {
        //hci_le_csConfigComplete_evt()
        if (param->status == 0) {
            hci_le_csConfigCompleteEvt_t *pCsEvt = (hci_le_csConfigCompleteEvt_t *)cmd;

            cs_config_t *pCsCfg = gCsMng.gGlobal_pCsCfg + pCsEvt->Config_ID;

            pCsCfg->aclHandle               = pCsEvt->Connection_Handle;
            pCsCfg->Config_ID               = pCsEvt->Config_ID;
            pCsCfg->state                   = pCsEvt->Action;
            pCsCfg->Main_Mode               = pCsEvt->Main_Mode;
            pCsCfg->Sub_Mode                = pCsEvt->Sub_Mode;
            pCsCfg->Main_Mode_Min_Steps     = pCsEvt->Main_Mode_Min_Steps;
            pCsCfg->Main_Mode_Max_Steps     = pCsEvt->Main_Mode_Max_Steps;
            pCsCfg->Main_Mode_Repetition    = pCsEvt->Main_Mode_Repetition;
            pCsCfg->Mode_0_Steps            = pCsEvt->Mode_0_Steps;
            pCsCfg->Role                    = pCsEvt->Role;
            pCsCfg->RTT_Type                = pCsEvt->RTT_Type;
            pCsCfg->CS_SYNC_PHY             = pCsEvt->CS_SYNC_PHY;
            pCsCfg->Channel_Map_Repetition  = pCsEvt->Channel_Map_Repetition;
            pCsCfg->ChSel                   = pCsEvt->ChSel;
            pCsCfg->Ch3c_Shape              = pCsEvt->Ch3c_Shape;
            pCsCfg->Ch3c_Jump               = pCsEvt->Ch3c_Jump;
            pCsCfg->Companion_Signal_Enable = pCsEvt->Companion_Signal_Enable;
            pCsCfg->T_IP1_Us                = pCsEvt->T_IP1_Time;
            pCsCfg->T_IP2_Us                = pCsEvt->T_IP2_Time;
            pCsCfg->T_FCS_Us                = pCsEvt->T_FCS_Time;
            pCsCfg->T_PM_Us                 = pCsEvt->T_PM_Time;
            smemcpy((u8 *)pCsCfg->Channel_Map, (u8 *)pCsEvt->Channel_Map, 10);

            if (pCsCfg->Role == CHANNEL_SOUNDING_ROLE_INITIATOR) {
                //blc_ll_initCsInitiatorModule(NULL); //no need
            } else if (pCsCfg->Role == CHANNEL_SOUNDING_ROLE_REFLECTOR) {
                //blc_ll_initCsReflectorModule(NULL); //no need
            } else {
                return CS_SNIFFER_PARAMETER_STATUS_FAILED;
            }

            smemcpy((u8 *)pCsCfg->Origin_Chn_Map, (u8 *)pCsCfg->Channel_Map, 10);
            blt_cs_extractEnableChnMap(pCsCfg->Channel_Map, pCsCfg->filteredChnArray, &pCsCfg->Chn_en_num);

            int arraySize, t_index;

            arraySize = sizeof(T_IP_US) / sizeof(T_IP_US[0]);
            t_index   = findIndex(pCsCfg->T_IP1_Us, T_IP_US, arraySize);
            if (t_index != -1) {
                pCsCfg->T_IP1 = t_index;
            } else {
                return CS_SNIFFER_PARAMETER_STATUS_FAILED;
            }

            t_index = findIndex(pCsCfg->T_IP2_Us, T_IP_US, arraySize);
            if (t_index != -1) {
                pCsCfg->T_IP2 = t_index;
            } else {
                return CS_SNIFFER_PARAMETER_STATUS_FAILED;
            }

            arraySize = sizeof(T_FCS_US) / sizeof(T_FCS_US[0]);
            t_index   = findIndex(pCsCfg->T_FCS_Us, T_FCS_US, arraySize);
            if (t_index != -1) {
                pCsCfg->T_FCS = t_index;
            } else {
                return CS_SNIFFER_PARAMETER_STATUS_FAILED;
            }

            arraySize = sizeof(T_PM_US) / sizeof(T_PM_US[0]);
            t_index   = findIndex(pCsCfg->T_PM_Us, T_PM_US, arraySize);
            if (t_index != -1) {
                pCsCfg->T_PM = t_index;
            } else {
                return CS_SNIFFER_PARAMETER_STATUS_FAILED;
            }

            pCsCfg->occupy = pCsCfg->state;

            #if (CS_TLK_ALGO2_EN)
                blc_Algo2_CopyConfigCompleteData(&pCsEvt->Main_Mode, sizeof(hci_le_csConfigCompleteEvt_t)-6);
            #endif

            tlkapi_send_string_data(DBG_CS_DATA_EN, "[STK][CS] Sniffer Config", pCsEvt, sizeof(hci_le_csConfigCompleteEvt_t));

            return CS_SNIFFER_PARAMETER_UPDATE;
        } else {
            return CS_SNIFFER_PARAMETER_STATUS_FAILED;
        }
    } else if (param->event_code == HCI_SUB_EVT_LE_CS_SECURITY_ENABLE_COMPLETE) {
        //hci_le_csSecurityEnableComplete_evt()
        if (param->status == 0) {
            smemcpy(&pCsParam->CS_IV[0], cs_event_param, 16);
            smemcpy(&pCsParam->CS_IN[0], cs_event_param + 16, 8);
            smemcpy(&pCsParam->CS_PV[0], cs_event_param + 24, 16);

            drbg = (drbg_param_t *)&pCsParam->drbg_data[0];
            drbg_instantiation_func_h9(pCsParam->CS_IV, pCsParam->CS_IN, pCsParam->CS_PV, &drbg->kdrbg[0], &drbg->vdrbg[0]);
            cs_drbg_init();

            tlkapi_send_string_data(DBG_CS_DATA_EN, "[STK][CS] Sniffer Security", pCsParam->CS_IV, 40);

            return CS_SNIFFER_PARAMETER_UPDATE;
        } else {
            return CS_SNIFFER_PARAMETER_STATUS_FAILED;
        }
    } else if (param->event_code == HCI_SUB_EVT_LE_CS_PROCEDURE_ENABLE_COMPLETE) {
        //hci_le_csProcedureEnableComplete_evt()
        if (param->status == 0) {
            hci_le_csProcedureEnableCompleteEvt_t *pCsEvt = (hci_le_csProcedureEnableCompleteEvt_t *)cmd;

            cs_config_t *pCsCfg = gCsMng.gGlobal_pCsCfg + pCsEvt->Config_ID;

            pCsCfg->aclHandle       = pCsEvt->Connection_Handle;
            pCsCfg->Config_ID       = pCsEvt->Config_ID;
            pCsCfg->cs_procedure_en = pCsEvt->state;

            if (pCsCfg->cs_procedure_en == 1) {
                pCsCfg->aci                 = pCsEvt->Tone_Antenna_Config_Selection;
                pCsCfg->Selected_TX_Power   = pCsEvt->Selected_TX_Power;
                pCsCfg->Subevent_Len        = MAKE_U24(pCsEvt->Subevent_Len[2], pCsEvt->Subevent_Len[1], pCsEvt->Subevent_Len[0]);
                pCsCfg->Subevents_Per_Event = pCsEvt->Subevents_Per_Event;
                pCsCfg->subEvtIntvl_625us   = pCsEvt->Subevent_Interval;
                pCsCfg->Event_Interval      = pCsEvt->Event_Interval;
                pCsCfg->Procedure_Interval  = pCsEvt->Procedure_Interval;
                //pCsCfg->procMaxCountInstant     = pCsEvt->Procedure_Count;
                pCsCfg->Max_Procedure_Len = pCsEvt->Max_Procedure_Len;

                //pCsCfg->Tx_Pwr_Delta = Pwr_Delta;

                if (!pCsCfg->procMaxCountInstant) {
                    pCsCfg->csProcCount  = 0;
                    pCsCfg->procMaxCount = 0;
                }
                pCsCfg->procMaxCount += pCsEvt->Procedure_Count;
                pCsCfg->procMaxCountInstant = pCsEvt->Procedure_Count;

                u8 conn_idx = pCsCfg->aclHandle & CONN_IDX_MASK;
                st_ll_conn_t* pAcl = (st_ll_conn_t*)&blms[conn_idx];

                u8  timingParam[12];
                u8 *sourceAddress = (u8 *)(pCsEvt) + sizeof(hci_le_csProcedureEnableCompleteEvt_t);
                smemcpy(timingParam, sourceAddress, 12);
                pCsCfg->connEventCount       = MAKE_U16(timingParam[1], timingParam[0]);
                pCsCfg->csOft_us             = MAKE_U32(timingParam[5], timingParam[4], timingParam[3], timingParam[2]);
                pCsCfg->PHY                  = timingParam[6];
                csSniffer_param.totalNodeNum = timingParam[7];
                pCsCfg->sch_early_us         = MAKE_U16(timingParam[9], timingParam[8]);
                bltCsLocalSupportCap.T_SW_Time_Supported = timingParam[10];
                pAcl->csRemoteSupCap.T_SW_Time_Supported = timingParam[11];

                pCsCfg->inst_start_proc = pCsCfg->connEventCount;
                pCsCfg->Tone_Antenna_Config_Selection = pCsCfg->aci;
                pCsCfg->Preferred_Peer_Ant            = BIT(0); //TODO
                pCsCfg->procMaxCount += pCsEvt->Procedure_Count;
                pCsCfg->sSlot_csSubIntvl = BSLOT_DUR_2_SSLOT_DUR(pCsCfg->subEvtIntvl_625us);
                pCsCfg->sSlotCsDuration  = (pCsCfg->Subevent_Len + SLOT_PROCESS_MAX_US + pCsCfg->sch_early_us) * SSLOT_US_REVERSE + 1;

                pCsCfg->max_subEvtCnt = blt_cs_calcMaxProcLenSubevtCount(pCsCfg);
                pCsCfg->subEvtCnt     = 0;

                pCsCfg->cs_procedure_measurement_en = 1;
                pc_conn->cs_pending |= (pCsCfg->Config_ID | CS_IDX_FLG); //TODO, need use pCsCfg->idx
                extern ble_sts_t blt_ll_calcStepDuration(cs_config_t * pCsCfg);
                blt_ll_calcStepDuration(pCsCfg);
                //u8 conn_idx = pCsCfg->aclHandle & CONN_IDX_MASK;
                //st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];
                //extern void blt_ll_cs_subevent_len_cal(st_ll_conn_t* pAclConn, u8 config_id);
                //blt_ll_cs_subevent_len_cal(pc, pCsCfg->Config_ID);
                //u32 subevent_len_t = 0;
                //u32 max_offset = min((pc->conn_intvl_n_1m25*1250 -1),4000000);
                //extern u32 blt_ll_cs_subevent_schedule_early_cal(cs_config_t *pCsCfg,u32 maxSubLen,u32 *pSubLen,u32 schedule_early_max);
                //blt_ll_cs_subevent_schedule_early_cal(pCsCfg, pCsCfg->Subevent_Len, &subevent_len_t, max_offset);

                #if (CS_TLK_ALGO2_EN)
                    blc_Algo2_CopyProcedureEnableCompleteData(pCsEvt->Tone_Antenna_Config_Selection, pCsEvt->Selected_TX_Power,
                                                              pCsEvt->Subevent_Len[0] | (pCsEvt->Subevent_Len[1] << 8) | (pCsEvt->Subevent_Len[2] << 16),
                                                              pCsEvt->Subevents_Per_Event, pCsEvt->Event_Interval, pCsEvt->Procedure_Interval, pCsEvt->Procedure_Count);
                #endif

                tlkapi_send_string_data(DBG_CS_DATA_EN, "[STK][CS] Sniffer Procedure Enable", pCsEvt, sizeof(hci_le_csProcedureEnableCompleteEvt_t));
                tlkapi_send_string_u32s(DBG_CS_DATA_EN, "[STK][CS] Sniffer ProcedureTiming", pCsCfg->connEventCount, pCsCfg->csOft_us, pCsCfg->PHY, csSniffer_param.totalNodeNum,
                                        pCsCfg->sch_early_us, bltCsLocalSupportCap.T_SW_Time_Supported, pAcl->csRemoteSupCap.T_SW_Time_Supported);
            } else {
                //pCsCfg->cs_procedure_measurement_en = 0;
                //pCsCfg->cs_procedure_en = 0;
                //trigger CS Terminate
                csFlowCtrl.csTermiFlag = Receive_Send_CS_Terminate_RSP;

                tlkapi_send_string_data(DBG_CS_DATA_EN, "[STK][CS] Sniffer Procedure Disable", 0, 0);
            }

            return CS_SNIFFER_PARAMETER_UPDATE;
        } else {
            return CS_SNIFFER_PARAMETER_STATUS_FAILED;
        }
    } else {
        err = CS_SNIFFER_PARAMETER_INVALID;
    }

    return err;
}

/**
 * @brief      CS get subNode index by csCounter.
 * @param[in]  csCounter   - CS rangingCounter or procedureCounter
 * @return     subNode index
 */
u8 blc_sniffer_getSubNodeIndexByCsCounter(u16 csCounter)
{
    u8 sub_node_index = CS_COUNTER_CONVERT_SUB_NODE_INDEX_INVALID;//0xFF

    if (csSniffer_param.totalNodeNum > 1) {
        /* node_index, 0:main node, 1~N:sub node */
        u8 node_index = (csCounter & CS_COUNTER_CONVERT_SUB_NODE_INDEX_MASK) % csSniffer_param.totalNodeNum;
        if (node_index) {
            /* 0~(N-1):sub node */
            sub_node_index = node_index - 1;
        }
    }

    return sub_node_index;
}

#endif /* end of LL_CS_SNIFFER_MODE_ENABLE */
