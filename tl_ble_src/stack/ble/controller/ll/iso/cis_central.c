/********************************************************************************************************
 * @file    cis_central.c
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


#if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER)


_attribute_ble_data_retention_ ll_cig_mst_t *global_pCigMst = NULL; //global CIG parameter pointer
_attribute_ble_data_retention_ ll_cig_mst_t *latest_pCigMst = NULL; // last used CIG parameter pointer
_attribute_ble_data_retention_ ll_cig_mst_t *blt_pCigMst    = NULL;

_attribute_ble_data_retention_ _attribute_aligned_(4) cis_mas_para_t cisMas_param;


/* move to header file later begin ***********************************************/

    #if (FAST_SETTLE)
        #define CIS_T_MSS 180 //need debug later
    #else
        #define CIS_T_MSS 250 //need debug later
    #endif


    // two method both OK now
    #if 1 //now use this method
        #define TX_STL_CTX_1M                            TX_STL_BTX_1ST_PKT_SET_1M
        #define TX_STL_CTX_2M                            TX_STL_BTX_1ST_PKT_SET_2M
        #define TX_STL_CTX_CODED                         TX_STL_BTX_1ST_PKT_SET_CODED

        #define CIS_TX_TRIGGER_TO_PKT_IN_AIR_DISTANCE_US TX_STL_BTX_1ST_PKT_REAL
    #else
        #define TX_STL_CTX_1M                            (TX_STL_AUTO_MODE_1M - 20)
        #define TX_STL_CTX_2M                            (TX_STL_AUTO_MODE_2M - 20)
        #define TX_STL_CTX_CODED                         (TX_STL_AUTO_MODE_CODED - 20)
        #define CIS_TX_TRIGGER_TO_PKT_IN_AIR_DISTANCE_US (TX_STL_CTX_1M + PRMBL_EXTRA_1M * 8)
    #endif


    /* 50uS(ACL master use) is not enough for CTX, testing debug code 0x99AB0000 relative(20220720 SiHui) */
    #define IRQ_CTX_DELAY_US            70

    #define IRQ_CTX_SEND_DELAY_US       (IRQ_CTX_DELAY_US + CIS_TX_TRIGGER_TO_PKT_IN_AIR_DISTANCE_US)
    #define CIGMST_EARLY_SET_US         IRQ_CTX_SEND_DELAY_US


    #define DBG_CISCONN_TRACK_EN        0


    #define IXIT_MAX_CIS_NUM_IN_PER_CIG 2

/* move to header file later begin ***********************************************/


/*****************************************************************************/


ble_sts_t blc_ll_initCisCentralModule_initCigParametersBuffer(u8 *pCigParamBuf, int cig_num)
{
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(CIS_IN_CIGM_NUM_MAX), cis_central);
    STATIC_ASSERT_FILE(CIG_PARAM_LEN == sizeof(ll_cig_mst_t), cis_central);

    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_cig_mst_t)), cis_central);
    #endif


    LL_FEATURE_MASK_0 |= LL_FEATURE_MASK_CONNECTED_ISOCHRONOUS_STREAM_MASTER;

    /* Special protection code for use */
    if (pm_check_info) {
        ll_cis_master_ctrl_handler = blt_ll_cis_master_control_pdu_process;
        ll_cig_mst_irq_task_cb     = blt_cig_mst_interrupt_task;
        ll_cig_mst_mlp_task_cb     = blt_cig_mst_mainloop_task;
    }

    blmsParam.cis_cen_en = 1; //can only use 1 or 0, for "blc_hci_read Local Supported Commands"


    if (cig_num > LL_CIG_MST_NUM_MAX) {
        return LL_ERR_INVALID_PARAMETER;
    }


    global_pCigMst = (ll_cig_mst_t *)pCigParamBuf;

    bltCisMng.maxNum_cig_mst = cig_num;

    ll_cig_mst_t *pCigMaster;
    for (int i = 0; i < bltCisMng.maxNum_cig_mst; i++) {
        pCigMaster = (ll_cig_mst_t *)(global_pCigMst + i);

        //set some default value
        pCigMaster->cig_master_index = i; //global status, never change

        pCigMaster->cig_ID_mas = CIG_ID_INVALID;


        for (int j = 0; j < CIG_MST_FIFONUM; j++) {
            pCigMaster->cigTsk_fifo[j].scheTask_oft = TSKOFT_CIG_MST + i;
            pCigMaster->cigTsk_fifo[j].scheTask_idx = i;
            pCigMaster->cigTsk_fifo[j].scheTask_flg = TSKFLG_CIG_MST | TSKFLG_BSLOT_ALIGN;
        }

        blt_ll_setSchedulerTaskPriority(TSKOFT_CIG_MST + i, TASK_PRIORITY_HIGH_THRES);
    }


    return BLE_SUCCESS;
}

_attribute_ram_code_ int blt_cig_mst_interrupt_task(int flag, void *p)
{
    int cig_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    #if 1
    if (flag & FLAG_SCHEDULE_CIGMST_START) {
        blt_cig_mst_start(cig_idx);
    } else if (flag & FLAG_SCHEDULE_CTX_START) {
        blt_ctx_start();
    } else if (flag & FLAG_SCHEDULE_CTX_POST) {
        //tlkapi_send_string_u32s(DBG_CISCONN_TRACK_EN, "cis conn 6", blt_pCisConn->cis_index, blt_pCisConn->cis_connHandle, blt_pCisConn, 0);
        blt_ctx_post(blt_pCisConn);
    } else if (flag & FLAG_SCHEDULE_CIG_SET1ST_AP) {
        blt_cig_mst_set_first_anchor_point();
    } else if (flag & FLAG_SCHEDULE_BUILD) {
        blt_ll_buildCigSchedulerLinklist();
    } else if (flag & FLAG_INSERT_SCHTSK_CONFLICT) {
        sch_task_t *pTgtTsk       = (sch_task_t *)p;
        u8          tgtTskFlg     = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
        u8          curSchTaskOft = TSKOFT_CIG_MST + cig_idx;
        (void)tgtTskFlg; //remove compiler warning
        #if (SCH_TASK_PRIORITY_IN_CB_EN)
        s32 pri_taskCur = bltPri.pri_cal[curSchTaskOft];
        s32 pri_taskTra = bltPri.pri_cal[pTgtTsk->scheTask_oft];
        //priority higher than exist task, can insert target task
        if (pri_taskCur > pri_taskTra) {
            #if (ULL_FOR_CIS_EN) //CIS build task after ACL central task
            if (tgtTskFlg == TSKFLG_ACL_MASTER) {
                tlkapi_send_string_data(0, "[cis_mst]abandon, acl_task proc", &bltPri.csctvAbandonCnt[pTgtTsk->scheTask_oft], 2);
                st_ll_conn_t *pAclConn = (st_ll_conn_t *)&blms[pTgtTsk->scheTask_idx];
                if ((1 && (aclConn_param.connSync & (1 << pTgtTsk->scheTask_idx))) || ((u32)(clock_time() - pAclConn->conn_tick) > ((pAclConn->conn_timeout * 3) >> 2))) {
                    return 0; //abandon CIG task
                }
            }
            #endif
            return 1;
        }
        #endif

        tlkapi_send_string_data(0, "[cig_mst]insertTsk conflict, tgtTsk=", &tgtTskFlg, 1);
    }
    #else
    switch (flag) {
    case FLAG_SCHEDULE_CIGMST_START:
    {
        blt_cig_mst_start(cig_idx);
    } break;

    case FLAG_SCHEDULE_CTX_START:
    {
        blt_ctx_start();
    } break;

    case FLAG_SCHEDULE_CTX_POST:
    {
        blt_ctx_post(blt_pCisConn);
    } break;

    case FLAG_SCHEDULE_CIG_SET1ST_AP:
    {
        blt_cig_mst_set_first_anchor_point();
    } break;

    case FLAG_SCHEDULE_BUILD:
    {
        blt_ll_buildCigSchedulerLinklist();
    } break;

    case FLAG_INSERT_SCHTSK_CONFLICT:
    {
        sch_task_t *pTgtTsk       = (sch_task_t *)p;
        u8          tgtTskFlg     = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
        u8          curSchTaskOft = TSKOFT_CIG_MST + cig_idx;

        #if (SCH_TASK_PRIORITY_IN_CB_EN)
        s32 pri_taskCur = bltPri.pri_cal[curSchTaskOft];
        s32 pri_taskTra = bltPri.pri_cal[pTgtTsk->scheTask_oft];
        //priority higher than exist task, can insert target task
        if (pri_taskCur > pri_taskTra) {
            return 1;
        }
        #endif

        tlkapi_send_string_data(0, "[cig_mst]insertTsk conflict, tgtTsk=", &tgtTskFlg, 1);
    } break;

    default:
        break;
    }
    #endif

    return 0;
}


    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
//though CIS master sending data no need consider 150uS(TX first), but I will use one binary to test CIS slave and CIS master
//when CIS slave is sending data, this main_loop is running.
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    int
    blt_cig_mst_mainloop_task(int flag, void *p)
{
    if (flag == FLAG_MODULE_RESET) {
        blt_ll_reset_cig_mst();
    } else if (flag == FLAG_MODULE_MAINLOOP) {
        blt_ll_cigMstMainloop();
    } else if (flag == FLAG_CIS_CREATE_CANCEL) {
        return blt_ll_createCisCancel((ll_cis_conn_t *)p);
    }
    #if (FIX_CIS_CREATE_CMD_ERR_DUE_TO_PREVIOUS_CREATE_NOT_CLEAR_WHEN_ACL_TERMINATE)
    else if (flag == FLAG_ACL_MLP_DISCONNECT_EVT) {
        st_ll_conn_t *pAclConn = (st_ll_conn_t *)p;
        for (int i = 0; i < bltCisMng.maxNum_cisMaster; i++) {
            if ((bltCisMng.cisFlow_pending & BIT(i)) && !(pAclConn->cisEstablish_msk & BIT(i))) {
                bltCisMng.cisFlow_pending &= ~BIT(i);

                ll_cis_conn_t *pCisConn   = (ll_cis_conn_t *)(global_pCisConn + i);
                ll_cig_mst_t  *pCigMaster = (ll_cig_mst_t *)(global_pCigMst + pCisConn->clink_cig_idx);

                pCigMaster->cism_create_pending_msk = 0;
                pCisConn->createCmd                 = 0;
                pCisConn->cisFlowFlg                = CIS_FLOW_IDLE;
            }
        }
    }
    #endif
    return 0;
}

_attribute_noinline_ void blt_ll_reset_cig_mst(void)
{
    ll_cig_mst_t *pCigMaster;
    for (int i = 0; i < bltCisMng.maxNum_cig_mst; i++) {
        pCigMaster = (ll_cig_mst_t *)(global_pCigMst + i);

        pCigMaster->cig_ID_mas = CIG_ID_INVALID;

        pCigMaster->cism_estab_cnt = 0;
        pCigMaster->cism_estab_msk = 0;
    }
}

ble_sts_t blt_hci_checkCigPhyParams(u8 PHY_m2s, u8 PHY_s2m)
{
    /*
    If the Host sets, in the PHY_C_To_P[i] or PHY_P_To_C[i] parameters, a bit for
    a PHY that the Controller does not support, including a bit that is reserved for
    future use, the Controller shall return the error code Unsupported Feature or
    Parameter Value (0x11).
    */
    if (PHY_m2s < PHY_PREFER_1M || PHY_m2s > PHY_PREFER_CODED || PHY_s2m < PHY_PREFER_1M || PHY_s2m > PHY_PREFER_CODED) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    /*
    If the Controller does not support asymmetric PHYs and the Host sets
    PHY_C_To_P[i] to a different value than PHY_P_To_C[i], the Controller shall
    return the error code Unsupported Feature or Parameter Value (0x11).
     */
    if (PHY_m2s != PHY_s2m) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    #if 0
        if((((LL_FEATURE_MASK_0) & (LL_FEATURE_ENABLE_LE_2M_PHY<<8))==0) &&
                ((cisCfg->phy_m2s & BIT(1)) ||  (cisCfg->phy_s2m & BIT(1)))){
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }

        if(((LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_CODED_PHY<<11))==0) &&
                ((cisCfg->phy_m2s & BIT(2))||   (cisCfg->phy_s2m & BIT(2))) ){
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    #endif

    return BLE_SUCCESS;
}

ble_sts_t blt_hci_checkCigParams(hci_le_setCigParam_cmdParam_t *pcmdParam)
{
    //invalid CIG
    if (pcmdParam->cig_id > CIG_ID_MAX) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    u32 sdu_interval_m2s = (pcmdParam->sdu_int_m2s[0]) | (pcmdParam->sdu_int_m2s[1] << 8) | (pcmdParam->sdu_int_m2s[2] << 16);
    u32 sdu_interval_s2m = (pcmdParam->sdu_int_s2m[0]) | (pcmdParam->sdu_int_s2m[1] << 8) | (pcmdParam->sdu_int_s2m[2] << 16);

    if ((sdu_interval_m2s < 0x000FF) || (sdu_interval_m2s > 0xFFFFF) || (sdu_interval_s2m < 0x000FF) || (sdu_interval_s2m > 0xFFFFF)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    if (pcmdParam->sca > 0x07 || pcmdParam->packing > PACK_INTERLEAVED || pcmdParam->framing > CIS_FRAMED) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if ((pcmdParam->max_trans_lat_m2s < 0x0005) || (pcmdParam->max_trans_lat_m2s > 0x0FA0) || (pcmdParam->max_trans_lat_s2m < 0x0005) || (pcmdParam->max_trans_lat_s2m > 0x0FA0)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if ((pcmdParam->cis_count < 1) || (pcmdParam->cis_count > 0x1F)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    int repeat_cig = blt_ll_searchExistingCigId(pcmdParam->cig_id) != CIG_ID_INVALID;


    cigParam_cisCfg_t *pCisCfg;
    for (int i = 0; i < pcmdParam->cis_count; i++) {
        pCisCfg = (cigParam_cisCfg_t *)&pcmdParam->cisCfg[i];

        if (pCisCfg->cis_id > CIS_ID_MAX) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }

        if (pCisCfg->max_sdu_m2s > 0x0FFF || pCisCfg->max_sdu_s2m > 0x0FFF) {
            //tlkapi_send_string_data(0, "sdu err", 0, 0);
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }

        /*
        If a CIS configuration that is being modified has a data path set in the Central to Peripheral direction and the Host has
        specified that Max_SDU_C_To_P[i] shall be set to zero, the Controller shall return the error code Command Disallowed (0x0C)

        If a CIS configuration that is being modified has a data path set in the Peripheral to Central direction and the Host
        has specified that Max_SDU_P_To_C[i] shall be set to zero, the Controller shall return the error code Command Disallowed (0x0C)
        */
        //HCI/CIS/BV-10-C
        if ((pCisCfg->max_sdu_m2s == 0 || pCisCfg->max_sdu_s2m == 0) && repeat_cig) {
            ll_cis_conn_t *pCisConn;
            for (int j = 0; j < latest_pCigMst->cism_set_cnt; j++) {
                pCisConn = (ll_cis_conn_t *)(global_pCisConn + latest_pCigMst->cism_set_order[j]);
                if (pCisConn->cis_ID == pCisCfg->cis_id) {
                    if (pCisCfg->max_sdu_m2s == 0 && (pCisConn->cis_dapth_setup & DATA_PATH_INPUT_FLAG)) {
                        return HCI_ERR_CMD_DISALLOWED;
                    } else if (pCisCfg->max_sdu_s2m == 0 && (pCisConn->cis_dapth_setup & DATA_PATH_OUTPUT_FLAG)) {
                        return HCI_ERR_CMD_DISALLOWED;
                    }
                }
            }
        }

        if (pCisCfg->rtn_m2s > 0x0f || pCisCfg->rtn_s2m > 0x0f) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }

        u8 ret_status = blt_hci_checkCigPhyParams(pCisCfg->phy_m2s, pCisCfg->phy_s2m);
        if (ret_status != BLE_SUCCESS) {
            tlkapi_send_string_data(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] phy set ERR", 0, 0);
            return ret_status;
        }


        u32 sdu_min_us_m2s, sdu_min_us_s2m;
        if (pCisCfg->phy_m2s == PHY_PREFER_1M) {
            sdu_min_us_m2s = pCisCfg->max_sdu_m2s * 8;
            sdu_min_us_s2m = pCisCfg->max_sdu_s2m * 8;
        } else if (pCisCfg->phy_m2s == PHY_PREFER_2M) {
            sdu_min_us_m2s = pCisCfg->max_sdu_m2s * 4;
            sdu_min_us_s2m = pCisCfg->max_sdu_s2m * 4;
        } else {
            sdu_min_us_m2s = pCisCfg->max_sdu_m2s * 16;
            sdu_min_us_s2m = pCisCfg->max_sdu_s2m * 16;
        }


        /*
         * Unsupported Feature or Parameter Value (0x11), error code Memory Capacity Exceeded
           (0x07), error code Connection Rejected Due to Limited Resources (0x0D), or error code Connection
           Limit Exceeded (0x09).
         */
        if (sdu_interval_m2s < sdu_min_us_m2s || sdu_interval_s2m < sdu_min_us_s2m) {
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }

        u32 cigSycDly_min_us = sdu_min_us_m2s + sdu_min_us_s2m + 150; //TIFS
        if ((pcmdParam->max_trans_lat_m2s * 1000) < cigSycDly_min_us || (pcmdParam->max_trans_lat_s2m * 1000) < cigSycDly_min_us) {
            //tlkapi_send_string_data(0, "max tranlatency ERROR", 0, 0);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }


    return BLE_SUCCESS;
}

ble_sts_t blt_hci_checkCigTestParams(hci_le_setCigParamTest_cmdParam_t *pcmdParam)
{
    u32 sdu_interval_m2s = (pcmdParam->sdu_int_m2s[0]) | (pcmdParam->sdu_int_m2s[1] << 8) | (pcmdParam->sdu_int_m2s[2] << 16);
    u32 sdu_interval_s2m = (pcmdParam->sdu_int_s2m[0]) | (pcmdParam->sdu_int_s2m[1] << 8) | (pcmdParam->sdu_int_s2m[2] << 16);

    if ((sdu_interval_m2s < 0x000FF) || (sdu_interval_m2s > 0xFFFFF) || (sdu_interval_s2m < 0x000FF) || (sdu_interval_s2m > 0xFFFFF)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    if (pcmdParam->sca > 0x07 || pcmdParam->packing > PACK_INTERLEAVED || pcmdParam->framing > CIS_FRAMED) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if ((pcmdParam->cis_count < 1) || (pcmdParam->cis_count > 0x32)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    cigParamTest_cisCfg_t *pCisCfg;
    for (int i = 0; i < pcmdParam->cis_count; i++) {
        pCisCfg = (cigParamTest_cisCfg_t *)&pcmdParam->cisCfg[i];

        if (pCisCfg->cis_id > CIS_ID_MAX) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }

        if (pCisCfg->max_sdu_m2s > 0x0FFF || pCisCfg->max_sdu_s2m > 0x0FFF) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }


        u8 ret_status = blt_hci_checkCigPhyParams(pCisCfg->phy_m2s, pCisCfg->phy_s2m);
        if (ret_status != BLE_SUCCESS) {
            tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "phy set ERR", 0, 0);
            return ret_status;
        }

        u32 sdu_min_us_m2s, sdu_min_us_s2m;
        if (pCisCfg->phy_m2s == PHY_PREFER_1M) {
            sdu_min_us_m2s = pCisCfg->max_sdu_m2s * 8;
            sdu_min_us_s2m = pCisCfg->max_sdu_s2m * 8;
        } else if (pCisCfg->phy_m2s == PHY_PREFER_2M) {
            sdu_min_us_m2s = pCisCfg->max_sdu_m2s * 4;
            sdu_min_us_s2m = pCisCfg->max_sdu_s2m * 4;
        } else {
            sdu_min_us_m2s = pCisCfg->max_sdu_m2s * 16;
            sdu_min_us_s2m = pCisCfg->max_sdu_s2m * 16;
        }

        if (sdu_interval_m2s < sdu_min_us_m2s || sdu_interval_s2m < sdu_min_us_s2m) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }

    return BLE_SUCCESS;
}

void swap_u32(u32 *p, int n)
{
    int i, c;
    for (i = 0; i < n / 2; i++) {
        c            = p[i];
        p[i]         = p[n - 1 - i];
        p[n - 1 - i] = c;
    }
}

void blt_ll_cism_allocate_common(ll_cis_conn_t *pCisConn, ll_cig_mst_t *pCigMaster)
{
    pCisConn->cis_occupied = 1;
    pCisConn->createCmd    = 0;

    pCisConn->cis_dapth_setup = 0; //HCI/CIS/BI-10-C, must clear when CIS allocated, too late if clear in connect_common
    pCisConn->dpId            = Data_Path_Disable;

    pCisConn->clink_cig_idx = pCigMaster->cig_master_index;
    pCisConn->link_cigid    = pCigMaster->cig_ID_mas;
    pCisConn->cis_frame     = pCigMaster->cig_frame;
}

ble_sts_t blc_hci_le_setCigCommon(u8 cig_id, int cis_count)
{
    (void)cis_count; //unused, remove warning

    /*If the CIG_ID does not exist, then the
    Controller shall create a new CIG. Otherwise, the Controller shall modify or add
    CIS(s) in the CIG that is identified by the CIG_ID and update all the parameters
    that apply to the CIG.   */
    if (blt_ll_searchExistingCigId(cig_id) != CIG_ID_INVALID) { //a repeated CIG

        /*
         * If the Host issues this command after any CISes in the CIG have been created,
            the Controller shall return the error code Command Disallowed (0x0C)
         */
        if (latest_pCigMst->cism_estab_cnt) {
            return HCI_ERR_CMD_DISALLOWED;
        }


        if (!latest_pCigMst->config_state) { //HCI/CIS/BI-13-C & HCI/CIS/BV-02-C test this logic
            return HCI_ERR_CMD_DISALLOWED;
        }
    } else {
        if (blt_ll_AllocateNewCigId(cig_id) != CIG_ID_INVALID) { //pay attention: when this function runs, latest_pCigMst & latest_cigId updated
            //1. CIG master not enough
            //2. CIS connection is not enough
            if (bltCisMng.curNum_cig_mst >= bltCisMng.maxNum_cig_mst) {
                return HCI_ERR_CONN_LIMIT_EXCEEDED;
            }

            bltCisMng.curNum_cig_mst++;
            latest_pCigMst->config_state = 1;
        } else { //new CIG allocate
            tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "cig id allocate error", 0, 0);
            return HCI_ERR_MEM_CAP_EXCEEDED;
        }
    }


    /* if code runs to here, new CIG_ID is allocated, latest_pCigMst is available */
    ll_cig_mst_t *pCigMaster = latest_pCigMst;

    pCigMaster->cig_ID_mas = cig_id;


    ll_cis_conn_t *pCisConn;
    for (int i = 0; i < pCigMaster->cism_set_cnt; i++) {
        pCisConn               = (ll_cis_conn_t *)(global_pCisConn + pCigMaster->cism_set_order[i]);
        pCisConn->cis_occupied = 0;
    }


    pCigMaster->cism_set_cnt = 0;
    pCigMaster->cism_set_msk = 0;

    pCigMaster->cism_create_msk         = 0;
    pCigMaster->cism_create_pending_msk = 0;
    pCigMaster->cism_estab_cnt          = 0;
    pCigMaster->cism_estab_msk          = 0;

    pCigMaster->cism_task_msk = 0;
    pCigMaster->cism_task_cnt = 0;
    for (int i = 0; i < CIS_IN_CIGM_NUM_MAX; i++) {
        pCigMaster->cis_alloc_exist[i]   = 0;
        pCigMaster->cis_alloc_markIdx[i] = CIS_ID_INVALID;
    }
    pCigMaster->pcisPos_earliest = pCigMaster->pcisPos_latest = NULL;

    return BLE_SUCCESS;
}

int ceil(float x)
{
    int y = (x + 0.999999) / 1;

    return y;
}

u8                             cigTempBuff[SET_CIG_COMMON_PARAM_LENGTH + sizeof(cigParam_cisCfg_t) * IXIT_MAX_CIS_NUM_IN_PER_CIG];
hci_le_setCigParam_cmdParam_t *pGlobalParam = (hci_le_setCigParam_cmdParam_t *)cigTempBuff;


    #define CIG_TYPE_NEW          1
    #define CIG_TYPE_ADD          2
    #define CIG_TYPE_REPLACE_ALL  3
    #define CIG_TYPE_REPLACE_PART 4
u8 cis_nse_test = 0;

void blc_set_cis_nse(u8 nse)
{
    cis_nse_test = nse;
}

ble_sts_t blc_hci_le_setCigParams(hci_le_setCigParam_cmdParam_t *pCmdParam, hci_le_setCigParam_retParam_t *pRetParam)
{
    /* If the Status return parameter is non-zero, then the state of the CIG and its CIS
    configurations shall not be changed by the command. If the CIG did not already
    exist, it shall not be created */
    #if (BQB_TEST_EN)
    if (pCmdParam->cis_count <= IXIT_MAX_CIS_NUM_IN_PER_CIG) //corresponding to IXIT
    #else
    if (pCmdParam->cis_count <= CIS_IN_CIGM_NUM_MAX)
    #endif
    {
        tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Set_Cig_Param", pCmdParam, SET_CIG_COMMON_PARAM_LENGTH + sizeof(cigParam_cisCfg_t) * pCmdParam->cis_count);
    } else {
        tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Cig Param cis count too big", &pCmdParam->cis_count, 1);


        if (pCmdParam->cis_count < CIS_COUNT_MAX_VALUE) {
            /*If the Host attempts to set CIS parameters that exceed the maximum
            supported connections in the Controller, the Controller shall return the error
            code Connection Limit Exceeded (0x09). */

            /* HCI/CIS/BI-05-C [Connected Isochronous Stream Using Non-Test Command, Central, Reject Invalid Parameters]
             * (Status: 0x07, 0x09, 0x0D, OR 0x11) */
            pRetParam->status = HCI_ERR_CONN_LIMIT_EXCEEDED;
            return HCI_ERR_CONN_LIMIT_EXCEEDED;

        } else { //20230323, HCI/CIS/BV-05-C, cis count = 0x20, expect 0x12 returned. So old BQB bin(202209) can not pass new EBQ version
            pRetParam->status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }


    ble_sts_t ret_status = blt_hci_checkCigParams(pCmdParam);
    if (ret_status != BLE_SUCCESS) {
        //BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99910000);
        pRetParam->status = ret_status;
        //tlkapi_send_string_data(0, "Set_Cig_Param param err", 0, 0);
        return ret_status;
    }


    st_ll_conn_t *pAclConn = NULL;
    for (int i = ACL_CONN_IDX_CEN0; i < blmsParam.max_master_num; i++) {
        if (blms[i].connState == CONN_STATUS_ESTABLISH) {
            pAclConn = (st_ll_conn_t *)&blms[i];
            break;
        }
    }

    if (pAclConn == NULL) {
        pRetParam->status = HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
        tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] No ACL master", 0, 0);
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }


    int cig_type = 0;

    ll_cig_mst_t *pCigMaster = NULL;
    if (blt_ll_searchExistingCigId(pCmdParam->cig_id) != CIG_ID_INVALID) { //a repeated CIG

        pCigMaster = latest_pCigMst;
        /*  If the Host issues this command after any CISes in the CIG have been created,
            the Controller shall return the error code Command Disallowed (0x0C)
         */
        if (latest_pCigMst->cism_estab_cnt) {
            pRetParam->status = HCI_ERR_CMD_DISALLOWED;
            return HCI_ERR_CMD_DISALLOWED;
        }

        if (!latest_pCigMst->config_state) { //HCI/CIS/BI-13-C & HCI/CIS/BV-02-C test this logic
            pRetParam->status = HCI_ERR_CMD_DISALLOWED;
            return HCI_ERR_CMD_DISALLOWED;
        }

        tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] repeated CIG", 0, 0);


        if (pGlobalParam->cis_count) {
            //tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "exist cis cnt", &pGlobalParam->cis_count, 1);

            int same_cis_num = 0;
            u8  sameCis_index[CIS_IN_CIGM_NUM_MAX];
            for (int i = 0; i < pCmdParam->cis_count; i++) {
                //tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "cis_ID new", &pCmdParam->cisCfg[i].cis_id, 1);

                for (int j = 0; j < pGlobalParam->cis_count; j++) {
                    //tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "cis_ID exist", &pGlobalParam->cisCfg[j].cis_id, 1);

                    if (pCmdParam->cisCfg[i].cis_id == pGlobalParam->cisCfg[j].cis_id) {
                        tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] same CIS ID", &pCmdParam->cisCfg[i].cis_id, 1);
                        sameCis_index[same_cis_num] = j;
                        same_cis_num++;
                        break;
                    }
                }
            }

            if (same_cis_num == 0) {
                cig_type = CIG_TYPE_ADD;
            } else if (same_cis_num == pCmdParam->cis_count) {
                if (same_cis_num == pCigMaster->cism_set_cnt) {
                    cig_type = CIG_TYPE_REPLACE_ALL;
                    tlkapi_send_string_u8s(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] CIG TYPE replace all", pGlobalParam->cis_count, pCmdParam->cis_count, same_cis_num, 0);
                } else {
                    cig_type = CIG_TYPE_REPLACE_PART;
                    tlkapi_send_string_u8s(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] CIG TYPE replace part", pGlobalParam->cis_count, pCmdParam->cis_count, same_cis_num, 0);
                }
            } else {
                pRetParam->status = HCI_ERR_CMD_DISALLOWED;
                tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] can not process this now", &same_cis_num, 1);
                return HCI_ERR_CMD_DISALLOWED;
            }


            if (cig_type == CIG_TYPE_ADD || cig_type == CIG_TYPE_REPLACE_PART) {
                if (smemcmp(pGlobalParam, pCmdParam, SET_CIG_COMMON_PARAM_LENGTH - 1)) {
                    tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] common param not same, exit", 0, 0);
                    pRetParam->status = HCI_ERR_CONN_LIMIT_EXCEEDED;
                    return HCI_ERR_CONN_LIMIT_EXCEEDED;
                }
            }

            if (cig_type == CIG_TYPE_ADD) {
                u8 cis_total_cnt = pGlobalParam->cis_count + pCmdParam->cis_count;
                if (cis_total_cnt > IXIT_MAX_CIS_NUM_IN_PER_CIG) {
                    tlkapi_send_string_u8s(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] cis count exceed", pGlobalParam->cis_count, pCmdParam->cis_count, cis_total_cnt, 0);
                    pRetParam->status = HCI_ERR_CONN_LIMIT_EXCEEDED;
                    return HCI_ERR_CONN_LIMIT_EXCEEDED;
                }


                tlkapi_send_string_u8s(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] CIG TYPE add", pGlobalParam->cis_count, pCmdParam->cis_count, cis_total_cnt, 0);
                for (int i = 0; i < pCmdParam->cis_count; i++) {
                    smemcpy(&pGlobalParam->cisCfg[pGlobalParam->cis_count + i], &pCmdParam->cisCfg[i], sizeof(cigParam_cisCfg_t));
                }

                pGlobalParam->cis_count = cis_total_cnt;

                tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] param total", pGlobalParam, SET_CIG_COMMON_PARAM_LENGTH + sizeof(cigParam_cisCfg_t) * cis_total_cnt);
            } else if (cig_type == CIG_TYPE_REPLACE_PART) {
                for (int i = 0; i < same_cis_num; i++) {
                    smemcpy(&pGlobalParam->cisCfg[sameCis_index[i]], &pCmdParam->cisCfg[i], sizeof(cigParam_cisCfg_t));
                }
                tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] param new", pGlobalParam, SET_CIG_COMMON_PARAM_LENGTH + sizeof(cigParam_cisCfg_t) * pGlobalParam->cis_count);
            }
        } else {
            tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] ERROR cism set cnt", 0, 0);
            BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99B20000);
        }

    } else if (blt_ll_AllocateNewCigId(pCmdParam->cig_id) != CIG_ID_INVALID) { //pay attention: when this function runs, latest_pCigMst & latest_cigId updated

        pCigMaster = latest_pCigMst;

        //1. CIG master not enough
        //2. CIS connection is not enough
        if (bltCisMng.curNum_cig_mst >= bltCisMng.maxNum_cig_mst) {
            pRetParam->status = HCI_ERR_CONN_LIMIT_EXCEEDED;
            return HCI_ERR_CONN_LIMIT_EXCEEDED;
        }

        bltCisMng.curNum_cig_mst++;
        latest_pCigMst->config_state = 1;


        cig_type = CIG_TYPE_NEW;
        tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] CIG TYPE new", 0, 0);
        smemcpy(pGlobalParam, pCmdParam, SET_CIG_COMMON_PARAM_LENGTH + sizeof(cigParam_cisCfg_t) * pCmdParam->cis_count);
    } else {
        tlkapi_send_string_data(DBG_CIS_MASTER_LOGIC, "[CISC][PAR] cig id allocate error", 0, 0);
        pRetParam->status = HCI_ERR_MEM_CAP_EXCEEDED;
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }


    pCigMaster->cig_ID_mas = pCmdParam->cig_id; //TODO, change to API, manage CG ID more carefully

    if (pCigMaster->cism_set_cnt) {
        ll_cis_conn_t *pCis;
        for (int i = 0; i < pCigMaster->cism_set_cnt; i++) {
            pCis               = (ll_cis_conn_t *)(global_pCisConn + pCigMaster->cism_set_order[i]);
            pCis->cis_occupied = 0;
        }
    }

    pCigMaster->ull_used     = 0;
    pCigMaster->cism_set_cnt = 0;
    pCigMaster->cism_set_msk = 0;

    pCigMaster->cism_create_msk         = 0;
    pCigMaster->cism_create_pending_msk = 0;
    pCigMaster->cism_estab_cnt          = 0;
    pCigMaster->cism_estab_msk          = 0;

    pCigMaster->cism_task_msk = 0;
    pCigMaster->cism_task_cnt = 0;
    for (int i = 0; i < CIS_IN_CIGM_NUM_MAX; i++) {
        pCigMaster->cis_alloc_exist[i]   = 0;
        pCigMaster->cis_alloc_markIdx[i] = CIS_ID_INVALID;
    }
    pCigMaster->pcisPos_earliest = pCigMaster->pcisPos_latest = NULL;


    /* important */
    hci_le_setCigParam_cmdParam_t *pBackupParam = pCmdParam;
    if (cig_type == CIG_TYPE_ADD || cig_type == CIG_TYPE_REPLACE_PART) {
        pCmdParam = pGlobalParam;
    }


    u32 aclmMster_intvl_us = pAclConn->conn_intvl_n_1m25 * 1250;


    pCigMaster->sca          = pCmdParam->sca;
    pCigMaster->cism_packing = pCmdParam->packing;
    pCigMaster->cig_frame    = pCmdParam->framing;
    smemcpy(&pCigMaster->sdu_int_loca, pCmdParam->sdu_int_m2s, 3);
    smemcpy(&pCigMaster->sdu_int_peer, pCmdParam->sdu_int_s2m, 3);

    /*code below: parameters decided by Host in "LE Set CIG Parameters Test command",
                  but controller can decide by itself when use "LE Set CIG Parameters command" */
    // Parameters set by "LE Set CIG Parameters Test command": FT/Iso_interval/NSE[i]/Max_PDU[i]/BN[i]

    //extern    u8  app_cig_param[];
    //tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "DEBUG 991D 1",     pCigMaster, app_cig_param, 0, 0);


    u32 max_trans_lat_loca = pCmdParam->max_trans_lat_m2s * 1000;
    u32 max_trans_lat_peer = pCmdParam->max_trans_lat_m2s * 1000;

    #if (DBG_CIS_CENTRAL_PARAM)
    tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] cis cnt,pack,frame,acl int", pCmdParam->cis_count, pCmdParam->packing, pCmdParam->framing, blt_debug_hex_2_dec_display(aclmMster_intvl_us));

    tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] SDU int, max_tran_lat", blt_debug_hex_2_dec_display(pCigMaster->sdu_int_loca), blt_debug_hex_2_dec_display(pCigMaster->sdu_int_peer), blt_debug_hex_2_dec_display(max_trans_lat_loca), blt_debug_hex_2_dec_display(max_trans_lat_peer));
    #endif


    u8 sdu_mul_Of_1250 = 0;
    //u8 isoInt_mul_of_sduInt = 0; //mark for framed
    u32 sdu_int_multiple_1;

    /* e.g. sdu interval is 10mS/15ms,  ISO interval at least 30mS */
    if ((pCigMaster->sdu_int_loca % 1250) == 0 && (pCigMaster->sdu_int_peer % 1250) == 0) {
        sdu_mul_Of_1250    = 1;
        sdu_int_multiple_1 = zuixiao_gongbeishu(pCigMaster->sdu_int_loca / 1250, pCigMaster->sdu_int_peer / 1250, 0);
        sdu_int_multiple_1 *= 1250;
        u32 oct_sdu_mul = blt_debug_hex_2_dec_display(sdu_int_multiple_1);
        (void)oct_sdu_mul; //remove compiler warning
        tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "[CISC][PAR] sdu_int_multiple", &oct_sdu_mul, 4);
    }


    /*
     UNFRAMED:  Transport_Latency = CIG_Sync_Delay + FT * ISO_Interval - SDU_Interval
       FRAMED:  Transport_Latency = CIG_Sync_Delay + FT * ISO_Interval + SDU_Interval

       assume that: cgSyDly_ft_IsoInt = CIG_Sync_Delay + FT * ISO_Interval,  then

     UNFRAMED:  cgSyDly_ft_IsoInt = Transport_Latency + SDU_Interval
       FRAMED:  cgSyDly_ft_IsoInt = Transport_Latency - SDU_Interval
     */
    u32 cgSyDly_ft_IsoInt_loca = 0, cgSyDly_ft_IsoInt_peer = 0, cgSyDly_ft_IsoInt_min = 0;
    u32 isoIntvlUs_min = 0, isoIntvlUs_max = 0;
    u32 isoIntvlUs_traverse  = 0;
    u32 isoIntvlTra_frame[8] = {0};
    int isoIntvlTra_cnt      = 0;


    /*
    If the Framing parameter is set to 1 then the CIS Data PDUs
    of the specified CISes shall be framed. If the Framing parameter is set to 0 the
    CIS Data PDUs of a given CIS may be either unframed or framed
     */
    if (pCigMaster->cig_frame == CIS_UNFRAMED) { //ISO interval is equal to or an integer multiple of SDU interval
        cgSyDly_ft_IsoInt_loca = max_trans_lat_loca + pCigMaster->sdu_int_loca;
        cgSyDly_ft_IsoInt_peer = max_trans_lat_peer + pCigMaster->sdu_int_peer;
        cgSyDly_ft_IsoInt_min  = min2(cgSyDly_ft_IsoInt_loca, cgSyDly_ft_IsoInt_peer);


        if (sdu_mul_Of_1250) {
            isoIntvlUs_min = sdu_int_multiple_1;
        } else {
            /* e.g. sdu interval is 8mS/12ms, can not divided by 1250, result is 24mS */
            sdu_int_multiple_1 = zuixiao_gongbeishu(pCigMaster->sdu_int_loca, pCigMaster->sdu_int_peer, 1);

            /* e.g. sdu_int_multiple_1 is 24000uS, can not div by 1250, ISO interval at least 120000(24000*5, 1250*96) */
            isoIntvlUs_min = zuixiao_gongbeishu(sdu_int_multiple_1, 1250, 1);

            if (isoIntvlUs_min == 0 || isoIntvlUs_min > 4000000) {
                //BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99B10000);
                tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "[CISC][PAR] sdu interval value too strange, unframed change to framed", 0, 0);
                pCigMaster->cig_frame = CIS_FRAMED;
            }
        }

        if (pCigMaster->cig_frame == CIS_UNFRAMED) {
            isoIntvlUs_max = aclmMster_intvl_us;

            /* assume that FT = 1, cgSyDly_ft_IsoInt = CIG_Sync_Delay + ISO_Interval, so ISO_Interval < cgSyDly_ft_IsoInt */
            if (isoIntvlUs_min > cgSyDly_ft_IsoInt_loca || isoIntvlUs_min > cgSyDly_ft_IsoInt_peer) {
                BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99B00000);
            }
        }
    }


    if (pCigMaster->cig_frame == CIS_FRAMED) {
        cgSyDly_ft_IsoInt_loca = max_trans_lat_loca - pCigMaster->sdu_int_loca;
        cgSyDly_ft_IsoInt_peer = max_trans_lat_peer - pCigMaster->sdu_int_peer;
        cgSyDly_ft_IsoInt_min  = min2(cgSyDly_ft_IsoInt_loca, cgSyDly_ft_IsoInt_peer);

        if (sdu_mul_Of_1250) {
            /* assume that CigSyDly max to ISO interval, FT is 1, so iSO_interval < (cgSyDly_ft_IsoInt/2)*/
            int mul_iso = cgSyDly_ft_IsoInt_min / sdu_int_multiple_1;
            if (mul_iso >= 2) {
                isoIntvlUs_min = sdu_int_multiple_1;
    #if 1 //BQB
                isoIntvlUs_traverse                = sdu_int_multiple_1;
                isoIntvlTra_frame[isoIntvlTra_cnt] = sdu_int_multiple_1;
                isoIntvlTra_cnt                    = 1;
    #else
                //2: 1 ISO interval most,  4: 3 ISO_interval most,   7: 6 ISO interval most
                int upper_limit = min2(mul_iso, 8);
                for (int i = 1; i < upper_limit; i++) {
                    u32 isoIntvlUs_temp = sdu_int_multiple_1 * i;
                    if (isoIntvlUs_temp > aclmMster_intvl_us) {
                        break;
                    } else if ((aclmMster_intvl_us % isoIntvlUs_temp) == 0) {
                        isoIntvlTra_frame[isoIntvlTra_cnt] = isoIntvlUs_temp;
                        isoIntvlTra_cnt++;
                        u32 oct_int_temp = blt_debug_hex_2_dec_display(isoIntvlUs_temp);
                        tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "framed, mul of sdu_int, add 1", &oct_int_temp, 4);
                    }
                }
    #endif
            }
        }


        if (isoIntvlTra_cnt) {
            //isoInt_mul_of_sduInt = 1;
            isoIntvlUs_max  = min2(cgSyDly_ft_IsoInt_min, aclmMster_intvl_us); //BQB
            u32 oct_sdu_mul = blt_debug_hex_2_dec_display(sdu_int_multiple_1);
            (void)oct_sdu_mul;                                                 //remove compiler warning
            tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "[CISC][PAR] framed, use multiple of SDU interval", &oct_sdu_mul, 4);
        } else {
            tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "[CISC][PAR] framed, use division of ACL master interval", &aclmMster_intvl_us, 4);

            /* assume CIG_Sync_Delay is 0, cgSyDly_ft_IsoInt = FT * ISO_Interval
             * FT minimum 1, so ISO_Interval < cgSyDly_ft_IsoInt */
            isoIntvlUs_max           = min2(cgSyDly_ft_IsoInt_min, aclmMster_intvl_us);
            int aclMaster_intvl_1m25 = pAclConn->conn_intvl_n_1m25;
            u32 isoIntvlUs_temp;
            int div_temp;
            for (int i = 1; i < aclMaster_intvl_1m25; i++) {
                if (i * i > aclMaster_intvl_1m25) {
                    break;
                }

                if ((aclMaster_intvl_1m25 % i) == 0) {
                    div_temp        = aclMaster_intvl_1m25 / i;
                    isoIntvlUs_temp = div_temp * 1250;
                    if (isoIntvlUs_temp < isoIntvlUs_max) {
    #if 1 //BQB
                        isoIntvlUs_traverse                = isoIntvlUs_temp;
                        isoIntvlUs_min                     = isoIntvlUs_traverse;
                        isoIntvlTra_frame[isoIntvlTra_cnt] = isoIntvlUs_temp;
                        isoIntvlTra_cnt++;
    #else
                        isoIntvlTra_frame[isoIntvlTra_cnt] = isoIntvlUs_temp;
                        isoIntvlTra_cnt++;
                        u32 oct_int_temp = blt_debug_hex_2_dec_display(isoIntvlUs_temp);
                        tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "framed, div of acl_int, add 1", &oct_int_temp, 4);
                        if (isoIntvlTra_cnt >= 8) {
                            break;
                        }
    #endif
                    } else { //transport latency can not meet
                        continue;
                    }
                }
            }

            if (isoIntvlTra_cnt) {
                swap_u32(isoIntvlTra_frame, isoIntvlTra_cnt);
            }
        }


        if (isoIntvlTra_cnt == 0) {
            tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "[CISC][PAR] framed, can not find available ISO interval", 0, 0);
            pRetParam->status = HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
            return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
        }
    }


    tlkapi_send_string_u32s(DBG_SET_CIG_PARAMS, "[CISC][PAR] iso interval cal", blt_debug_hex_2_dec_display(isoIntvlUs_min), blt_debug_hex_2_dec_display(isoIntvlUs_max), blt_debug_hex_2_dec_display(cgSyDly_ft_IsoInt_loca), blt_debug_hex_2_dec_display(cgSyDly_ft_IsoInt_peer));


    ///// find available CIS connection for current CIG ///////////////////
    int            new_cis_cnt    = 0;
    int            return_cis_cnt = 0;
    ll_cis_conn_t *pCisConn;


    u32 se_length_us[CIS_IN_CIGM_NUM_MAX];
    u8  nse_min_cis[CIS_IN_CIGM_NUM_MAX];
    u8  cig_nse_min = 0;
    for (int cis_idx = 0; cis_idx < bltCisMng.maxNum_cisMaster; cis_idx++) {
        pCisConn = (ll_cis_conn_t *)(global_pCisConn + cis_idx);

        if (pCisConn->cis_occupied == 0) { //available

            blt_ll_cism_allocate_common(pCisConn, pCigMaster);
            pCisConn->cis_ID = pCmdParam->cisCfg[new_cis_cnt].cis_id;

            pCisConn->max_sdu_loca = pCmdParam->cisCfg[new_cis_cnt].max_sdu_m2s;
            pCisConn->max_sdu_peer = pCmdParam->cisCfg[new_cis_cnt].max_sdu_s2m;

            if (pCigMaster->cig_frame == CIS_UNFRAMED) {
                if (pCisConn->max_sdu_loca > CIS_PDU_MAX) {
                    /* e.g. sdu = 601, 601/251 = 2, at least 3 packet, 601/3=200,
                            yushu is 1, use 201;  if 600, yushu is 0, use 200 */
                    int packet_num         = (pCisConn->max_sdu_loca + CIS_PDU_MAX - 1) / CIS_PDU_MAX;
                    int shang              = pCisConn->max_sdu_loca / packet_num;
                    int yushu              = pCisConn->max_sdu_loca % packet_num;
                    pCisConn->max_pdu_loca = shang + (yushu ? 1 : 0);
                } else {
                    pCisConn->max_pdu_loca = pCisConn->max_sdu_loca;
                }

                if (pCisConn->max_sdu_peer > CIS_PDU_MAX) {
                    int packet_num         = (pCisConn->max_sdu_peer + CIS_PDU_MAX - 1) / CIS_PDU_MAX;
                    int shang              = pCisConn->max_sdu_peer / packet_num;
                    int yushu              = pCisConn->max_sdu_peer % packet_num;
                    pCisConn->max_pdu_peer = shang + (yushu ? 1 : 0);
                } else {
                    pCisConn->max_pdu_peer = pCisConn->max_sdu_peer;
                }


                if (pCisConn->max_pdu_loca) { //in case that PDU is 0(SDU is 0)
                    pCisConn->sdu_split_pdu_loca = (pCisConn->max_sdu_loca + pCisConn->max_pdu_loca - 1) / pCisConn->max_pdu_loca;
                } else {
                    pCisConn->sdu_split_pdu_loca = 0;
                }

                if (pCisConn->max_pdu_peer) { //in case that PDU is 0(SDU is 0)
                    pCisConn->sdu_split_pdu_peer = (pCisConn->max_sdu_peer + pCisConn->max_pdu_peer - 1) / pCisConn->max_pdu_peer;
                } else {
                    pCisConn->sdu_split_pdu_peer = 0;
                }


                tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] sdu, pdu", blt_debug_hex_2_dec_display(pCisConn->max_sdu_loca), blt_debug_hex_2_dec_display(pCisConn->max_sdu_peer), blt_debug_hex_2_dec_display(pCisConn->max_pdu_loca), blt_debug_hex_2_dec_display(pCisConn->max_pdu_peer));


                u8 bnLoca_min = pCisConn->sdu_split_pdu_loca * (isoIntvlUs_min / pCigMaster->sdu_int_loca);
                u8 bnPeer_min = pCisConn->sdu_split_pdu_peer * (isoIntvlUs_min / pCigMaster->sdu_int_peer);

                tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] split num, bn min", pCisConn->sdu_split_pdu_loca, pCisConn->sdu_split_pdu_peer, bnLoca_min, bnPeer_min);

                nse_min_cis[new_cis_cnt] = max2(bnLoca_min, bnPeer_min);
                if (nse_min_cis[new_cis_cnt] == 0) { //in case that both BN is 0
                    nse_min_cis[new_cis_cnt] = 1;
                }
                if (nse_min_cis[new_cis_cnt] > cig_nse_min) {
                    cig_nse_min = nse_min_cis[new_cis_cnt];
                }
            } else { //CIS_FRAMED
                //for(int i=0; i<isoIntvlTra_cnt; i++ )
                {
                    u32 isoInt_cur = isoIntvlTra_frame[0];

                    u8 bnLoca;
                    if (pCisConn->max_sdu_loca) {
    #if (EBQ_8280_CHECK_PDU_EN)
                        /*Expecting BN / (Max_PDU - 2) >= ceil(F) / 5 + ceil(F / Max_SDU) where F = (1 + MaxDrift) / ISO_Interval / SDU_Interval */
                        float F             = (1.0001 * isoInt_cur) / pCigMaster->sdu_int_loca;
                        u32   iso_data_loca = ceil(F) * 5 + ceil(F * pCisConn->max_sdu_loca) + 2;
                        tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] local F, ceilF, PDU", ceil(F), ceil(F) * 5, blt_debug_hex_2_dec_display(ceil(F * pCisConn->max_sdu_loca)), blt_debug_hex_2_dec_display(iso_data_loca));
    #else
                        u8 extra_loca_byte;
                        if (isoInt_cur > pCigMaster->sdu_int_loca) {
                            extra_loca_byte = 2;
                        } else {
                            extra_loca_byte = (pCigMaster->sdu_int_loca / isoInt_cur) * 2;
                        }

                        u32 iso_data_loca = (pCisConn->max_sdu_loca + 5 + extra_loca_byte) * isoInt_cur / pCigMaster->sdu_int_loca + 1;
                        /* EBQ_2022-1.8280 error for LL/CIS/CEN/BV-36-C, 20220906
                         * Expecting BN / (Max_PDU - 2) >= ceil(F) / 5 + ceil(F / Max_SDU) where F = (1 + MaxDrift) / ISO_Interval / SDU_Interval   Failure
                           Expected: greater than or equal to 35.0d
                           Actual: 34
                           Max_PDU is 32, above calculation result is 36
                         * */
                        if (((int)(iso_data_loca - pCisConn->max_sdu_loca)) < 5) {
                            iso_data_loca = pCisConn->max_sdu_loca + 5;
                        }
    #endif

                        if (iso_data_loca <= CIS_PDU_MAX) {
                            bnLoca                 = 1;
                            pCisConn->max_pdu_loca = iso_data_loca;
                        } else {
                            int packet_num         = (iso_data_loca + CIS_PDU_MAX - 1) / CIS_PDU_MAX;
                            int shang              = pCisConn->max_sdu_loca / packet_num;
                            int yushu              = pCisConn->max_sdu_loca % packet_num;
                            pCisConn->max_pdu_loca = shang + (yushu ? 1 : 0);
                            bnLoca                 = packet_num;
                        }
                    } else {
                        bnLoca                 = 0;
                        pCisConn->max_pdu_loca = 0;
                    }
                    pCisConn->bn_loca = bnLoca; //BQB


                    u8 bnPeer;
                    if (pCisConn->max_sdu_peer) {
    #if (EBQ_8280_CHECK_PDU_EN)
                        /*Expecting BN / (Max_PDU - 2) >= ceil(F) / 5 + ceil(F / Max_SDU) where F = (1 + MaxDrift) / ISO_Interval / SDU_Interval */
                        float F             = (1.0001 * isoInt_cur) / pCigMaster->sdu_int_peer;
                        u32   iso_data_peer = ceil(F) * 5 + ceil(F * pCisConn->max_sdu_peer) + 2;
                        tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] peer F, ceilF, PDU", ceil(F), ceil(F) * 5, blt_debug_hex_2_dec_display(ceil(F * pCisConn->max_sdu_peer)), blt_debug_hex_2_dec_display(iso_data_peer));
    #else
                        u8 extra_peer_byte;
                        if (isoInt_cur > pCigMaster->sdu_int_peer) {
                            extra_peer_byte = 2;
                        } else {
                            extra_peer_byte = (pCigMaster->sdu_int_peer / isoInt_cur) * 2;
                        }

                        u32 iso_data_peer = (pCisConn->max_sdu_peer + 5 + 1 + extra_peer_byte) * isoInt_cur / pCigMaster->sdu_int_peer;
                        if (((int)(iso_data_peer - pCisConn->max_sdu_peer)) < 5) {
                            iso_data_peer = pCisConn->max_sdu_peer + 5;
                        }
    #endif

                        if (iso_data_peer <= CIS_PDU_MAX) {
                            bnPeer                 = 1;
                            pCisConn->max_pdu_peer = iso_data_peer;
                        } else {
                            int packet_num         = (iso_data_peer + CIS_PDU_MAX - 1) / CIS_PDU_MAX;
                            int shang              = pCisConn->max_sdu_peer / packet_num;
                            int yushu              = pCisConn->max_sdu_peer % packet_num;
                            pCisConn->max_pdu_peer = shang + (yushu ? 1 : 0);
                            bnPeer                 = packet_num;
                        }
                    } else {
                        bnPeer                 = 0;
                        pCisConn->max_pdu_peer = 0;
                    }
                    pCisConn->bn_peer = bnPeer; //BQB


                    tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] sdu, pdu", blt_debug_hex_2_dec_display(pCisConn->max_sdu_loca), blt_debug_hex_2_dec_display(pCisConn->max_sdu_peer), blt_debug_hex_2_dec_display(pCisConn->max_pdu_loca), blt_debug_hex_2_dec_display(pCisConn->max_pdu_peer));


                    nse_min_cis[new_cis_cnt] = max2(bnLoca, bnPeer);
                    if (nse_min_cis[new_cis_cnt] == 0) { //in case that both BN is 0
                        nse_min_cis[new_cis_cnt] = 1;
                    }
                    if (nse_min_cis[new_cis_cnt] > cig_nse_min) {
                        cig_nse_min = nse_min_cis[new_cis_cnt];
                    }

                    tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] bn", bnLoca, bnPeer, 0, 0);


    #if 1 //BQB
                    pCisConn->bn_loca = bnLoca;
                    pCisConn->bn_peer = bnPeer;
    #endif
                }
            } //end of CIS_FRAMED


            /* Our Controller only supports asymmetric PHYs. */
            pCisConn->phy_ms = pCmdParam->cisCfg[new_cis_cnt].phy_m2s;

            /* Our Controller only supports asymmetric PHYs. */
            if (pCisConn->phy_ms == PHY_PREFER_1M) {
                pCisConn->curCisPhy = BLE_PHY_1M;
            } else if (pCisConn->phy_ms == PHY_PREFER_2M) {
                pCisConn->curCisPhy = BLE_PHY_2M;
            } else {
                pCisConn->curCisPhy = BLE_PHY_CODED; //take it as S8
            }

            /* calculate potential task timing.
             * TX & RX max PDU translate time, uS
             * use latest ACL master encryption state, not very accurate, just rough method */
            u8  mic_len      = pAclConn->crypt.enable ? 4 : 0;
            u8  mic_len_loca = pCisConn->max_pdu_loca ? mic_len : 0;
            u8  mic_len_peer = pCisConn->max_pdu_peer ? mic_len : 0;
            int tx_rx_max_us = blt_phy_getRfPacketTime_us(pCisConn->max_pdu_loca + mic_len_loca, pCisConn->curCisPhy, LE_CODED_S8) +
                               blt_phy_getRfPacketTime_us(pCisConn->max_pdu_peer + mic_len_peer, pCisConn->curCisPhy, LE_CODED_S8);

            u32 mptm_tifs_mpts        = tx_rx_max_us + BLE_T_IFS;
            se_length_us[new_cis_cnt] = mptm_tifs_mpts + CIS_T_MSS;


    #if (DBG_CIS_CENTRAL_PARAM)
            tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] mptm_mpts, se_length", blt_debug_hex_2_dec_display(mptm_tifs_mpts), blt_debug_hex_2_dec_display(se_length_us[new_cis_cnt]), blt_debug_hex_2_dec_display(0), blt_debug_hex_2_dec_display(0));
    #endif


            pCigMaster->cism_set_msk |= BIT(cis_idx);
            pCigMaster->cism_set_order[pCigMaster->cism_set_cnt] = cis_idx;
            pCigMaster->cism_set_cnt++;

            for (int i = 0; i < pBackupParam->cis_count; i++) {
                if (pCisConn->cis_ID == pBackupParam->cisCfg[i].cis_id) {
                    pRetParam->cis_connHandle[return_cis_cnt++] = pCisConn->cis_connHandle;
                }
            }

            new_cis_cnt++;
            if (new_cis_cnt >= pCmdParam->cis_count) {
                break;
            }
        }
    }


    /* now only process all CIS share a same NSE, NSE maximum value 31 in BLE Spec */
    u32 task_total_us = 0;
    int cur_cis_cnt   = 0;
    for (int i = 0; i < pCigMaster->cism_set_cnt; i++) {
        u8 idx      = pCigMaster->cism_set_order[i];
        pCisConn    = (ll_cis_conn_t *)(global_pCisConn + idx);
        u32 task_us = cig_nse_min * se_length_us[cur_cis_cnt];
        task_total_us += task_us;

        cur_cis_cnt++;
    }


    /* total NSE not exceed 32,
     * single CIS NSE not exceed 12(BQB IXIT value now)
     * single CIS BN not exceed 8(BQB IXIT value now)
     * here nse_thres is bn_thres
     */
    u32 cig_nse_thres = min2(8, (32 / pCigMaster->cism_set_cnt));

    u32 cigSyDly_min = 0;
    u32 cig_task_us;
    u8  nse_cig_traverse = 0;
    u8  nse_final_use    = 0;
    u8  ftLoca_max       = 1;
    u8  ftpeer_max       = 1;
    int allocate         = 0;

    if (pCigMaster->cig_frame == CIS_UNFRAMED) {
        int stop_traverse = 0;
        int traverse_cnt  = 0;

        while (1) { //1*interval_us_min, 2*interval_us_min, 3*interval_us_min ......
            traverse_cnt++;
            isoIntvlUs_traverse = isoIntvlUs_min * traverse_cnt;
            nse_cig_traverse    = cig_nse_min * traverse_cnt;
            cig_task_us         = task_total_us * traverse_cnt;
            cigSyDly_min        = cig_task_us - CIS_T_MSS;

            //          if(aclmMster_intvl_us % isoIntvlUs_traverse){ //trigger bug when aclmMster_intvl_us not multiple of ISO interval_min
            //              continue;
            //          }

            tlkapi_send_string_u32s(DBG_SET_CIG_PARAMS, "[CISC][PAR] ISO interval traverse", traverse_cnt, blt_debug_hex_2_dec_display(isoIntvlUs_traverse), blt_debug_hex_2_dec_display(cig_task_us), nse_cig_traverse);

            if (isoIntvlUs_traverse > isoIntvlUs_max) {
                tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "[CISC][PAR] traverse stop, exceed ACL interval", 0, 0);
                break;
            } else if (nse_cig_traverse > cig_nse_thres) {
                tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "[CISC][PAR] traverse stop, nse too big", 0, 0);
                break;
            } else if ((cigSyDly_min + 1 * isoIntvlUs_traverse) > cgSyDly_ft_IsoInt_min) {
                tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "[CISC][PAR] traverse stop, exceed transport latency", 0, 0);
                break;
            }


            if (cigSyDly_min + 1500 < isoIntvlUs_traverse) {
                /*
                 UNFRAMED:  Transport_Latency = CIG_Sync_Delay + FT * ISO_Interval - SDU_Interval
                   FRAMED:  Transport_Latency = CIG_Sync_Delay + FT * ISO_Interval + SDU_Interval

                   assume that: cgSyDly_ft_IsoInt = CIG_Sync_Delay + FT * ISO_Interval,  then

                 UNFRAMED:  cgSyDly_ft_IsoInt = Transport_Latency + SDU_Interval
                   FRAMED:  cgSyDly_ft_IsoInt = Transport_Latency - SDU_Interval
                 */
                for (int k = 1; k <= 4; k++) {
                    u8 cur_nse = nse_cig_traverse * k;
                    if (cur_nse < cig_nse_thres) {
                        u32 cur_task_us = cig_task_us * k;
                        cigSyDly_min    = cur_task_us - CIS_T_MSS;
                        if (cigSyDly_min + 1500 < isoIntvlUs_traverse) {
                            int diff_loca = (int)(cgSyDly_ft_IsoInt_loca - cigSyDly_min);
                            int diff_peer = (int)(cgSyDly_ft_IsoInt_peer - cigSyDly_min);
                            if (diff_loca < (int)isoIntvlUs_traverse || diff_peer < (int)isoIntvlUs_traverse) {
                                tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "[CISC][PAR] cigSyDly can not bigger, tran_latency too small, FT will be 0", 0, 0);
                                stop_traverse = 1;
                                break;
                            }

                            ftLoca_max = diff_loca / isoIntvlUs_traverse;
                            ftpeer_max = diff_peer / isoIntvlUs_traverse;

                            allocate      = 1;
                            nse_final_use = cur_nse;


                            u32 trnsp_lat_loca = cigSyDly_min + ftLoca_max * isoIntvlUs_traverse - pCigMaster->sdu_int_loca;
                            u32 trnsp_lat_peer = cigSyDly_min + ftpeer_max * isoIntvlUs_traverse - pCigMaster->sdu_int_peer;
                            (void)trnsp_lat_loca; //remove compiler warning
                            (void)trnsp_lat_peer; //remove compiler warning
                            //tlkapi_send_string_u32s(DBG_SET_CIG_PARAMS, "allocate success", blt_debug_hex_2_dec_display(isoIntvlUs_traverse), nse, ftLoca_max, ftpeer_max);
                            tlkapi_send_string_u32s(DBG_SET_CIG_PARAMS, "[CISC][PAR] unframed, allocate OK", blt_debug_hex_2_dec_display(isoIntvlUs_traverse), blt_debug_hex_2_dec_display(cig_task_us), nse_final_use, blt_debug_hex_2_dec_display(cur_task_us));
                            tlkapi_send_string_u32s(DBG_SET_CIG_PARAMS, "[CISC][PAR] ft, trns_latncy", ftLoca_max, ftpeer_max, blt_debug_hex_2_dec_display(trnsp_lat_loca), blt_debug_hex_2_dec_display(trnsp_lat_peer));


                            if (traverse_cnt == 1) { //BQB is enough
                                stop_traverse = 1;
                            }
                        }
                    }
                }
            }


            if (stop_traverse) {
                break;
            }
        }


    } else { //CIS_FRAMED
        int stop_traverse = 0;
        int traverse_cnt  = 1;
        nse_cig_traverse  = cig_nse_min * traverse_cnt;
        cig_task_us       = task_total_us * traverse_cnt;
        cigSyDly_min      = cig_task_us - CIS_T_MSS;

        tlkapi_send_string_u32s(DBG_SET_CIG_PARAMS, "[CISC][PAR] ISO interval traverse", traverse_cnt, blt_debug_hex_2_dec_display(isoIntvlUs_traverse), blt_debug_hex_2_dec_display(cig_task_us), blt_debug_hex_2_dec_display(nse_cig_traverse));

        if (isoIntvlUs_traverse > isoIntvlUs_max) {
            tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "[CISC][PAR] traverse stop, exceed ACL interval", 0, 0);
            stop_traverse = 1;
        } else if (nse_cig_traverse > cig_nse_thres) {
            tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "[CISC][PAR] traverse stop, nse too big", 0, 0);
            stop_traverse = 1;
        } else if ((cigSyDly_min + 1 * isoIntvlUs_traverse) > cgSyDly_ft_IsoInt_min) {
            tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "[CISC][PAR] traverse stop, exceed transport latency", 0, 0);
            stop_traverse = 1;
        }


        if (!stop_traverse && (cigSyDly_min + 1500 < isoIntvlUs_traverse)) {
            /*
             UNFRAMED:  Transport_Latency = CIG_Sync_Delay + FT * ISO_Interval - SDU_Interval
               FRAMED:  Transport_Latency = CIG_Sync_Delay + FT * ISO_Interval + SDU_Interval

               assume that: cgSyDly_ft_IsoInt = CIG_Sync_Delay + FT * ISO_Interval,  then

             UNFRAMED:  cgSyDly_ft_IsoInt = Transport_Latency + SDU_Interval
               FRAMED:  cgSyDly_ft_IsoInt = Transport_Latency - SDU_Interval
             */
            for (int k = 1; k <= 4; k++) {
                u8 cur_nse = nse_cig_traverse * k;
                if (cur_nse < cig_nse_thres) {
                    u32 cur_task_us = cig_task_us * k;
                    cigSyDly_min    = cur_task_us - CIS_T_MSS;
                    if (cigSyDly_min + 1500 < isoIntvlUs_traverse) {
                        int diff_loca = (int)(cgSyDly_ft_IsoInt_loca - cigSyDly_min);
                        int diff_peer = (int)(cgSyDly_ft_IsoInt_peer - cigSyDly_min);
                        if (diff_loca < (int)isoIntvlUs_traverse || diff_peer < (int)isoIntvlUs_traverse) {
                            tlkapi_send_string_data(DBG_SET_CIG_PARAMS, "[CISC][PAR] cigSyDly can not bigger, tran_latency too small, FT will be 0", 0, 0);
                            stop_traverse = 1;
                            break;
                        }

                        ftLoca_max = diff_loca / isoIntvlUs_traverse;
                        ftpeer_max = diff_peer / isoIntvlUs_traverse;

                        allocate      = 1;
                        nse_final_use = cur_nse;

                        u32 trnsp_lat_loca = cigSyDly_min + ftLoca_max * isoIntvlUs_traverse + pCigMaster->sdu_int_loca;
                        u32 trnsp_lat_peer = cigSyDly_min + ftpeer_max * isoIntvlUs_traverse + pCigMaster->sdu_int_peer;
                        (void)trnsp_lat_loca; //remove compiler warning
                        (void)trnsp_lat_peer; //remove compiler warning
                        tlkapi_send_string_u32s(DBG_SET_CIG_PARAMS, "[CISC][PAR] framed, allocate OK", blt_debug_hex_2_dec_display(isoIntvlUs_traverse), blt_debug_hex_2_dec_display(cig_task_us), nse_final_use, blt_debug_hex_2_dec_display(cur_task_us));
                        tlkapi_send_string_u32s(DBG_SET_CIG_PARAMS, "[CISC][PAR] ft, trns_latncy", ftLoca_max, ftpeer_max, blt_debug_hex_2_dec_display(trnsp_lat_loca), blt_debug_hex_2_dec_display(trnsp_lat_peer));


                        if (k == 2) { //BQB is enough
                            stop_traverse = 1;
                            break;
                        }
                    }
                }
            }
        }
    }


    if (!allocate) {
        tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] alloc fail", 0, 0, 0, 0);
        pRetParam->status = HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }


    for (int i = 0; i < pCigMaster->cism_set_cnt; i++) {
        u8 idx   = pCigMaster->cism_set_order[i];
        pCisConn = (ll_cis_conn_t *)(global_pCisConn + idx);

        if (pCigMaster->cig_frame == CIS_UNFRAMED) {
            pCigMaster->cism_intvl_us = isoIntvlUs_traverse;
            pCisConn->iso_intvl_us    = pCigMaster->cism_intvl_us;
            pCisConn->iso_intvl_tick  = pCigMaster->cism_intvl_us * SYSTEM_TIMER_TICK_1US;

            if (cis_nse_test) {
                pCisConn->nse = cis_nse_test;
            } else {
                pCisConn->nse = nse_final_use;
            }
            pCisConn->bn_loca = pCisConn->sdu_split_pdu_loca * (isoIntvlUs_traverse / pCigMaster->sdu_int_loca);
            pCisConn->bn_peer = pCisConn->sdu_split_pdu_peer * (isoIntvlUs_traverse / pCigMaster->sdu_int_peer);

            pCisConn->ft_loca = ftLoca_max;
            pCisConn->ft_peer = ftpeer_max;

        } else {
            pCigMaster->cism_intvl_us = isoIntvlUs_traverse;
            pCisConn->iso_intvl_us    = pCigMaster->cism_intvl_us;
            pCisConn->iso_intvl_tick  = pCigMaster->cism_intvl_us * SYSTEM_TIMER_TICK_1US;

            pCisConn->nse = nse_final_use;
            //          pCisConn->bn_loca = ;
            //          pCisConn->bn_peer = ;

            pCisConn->ft_loca = ftLoca_max;
            pCisConn->ft_peer = ftpeer_max;
        }


        tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] iso int, nse, pdu", blt_debug_hex_2_dec_display(pCigMaster->cism_intvl_us), pCisConn->nse, blt_debug_hex_2_dec_display(pCisConn->max_pdu_loca), blt_debug_hex_2_dec_display(pCisConn->max_pdu_peer));

        tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] bn, ft", pCisConn->bn_loca, pCisConn->bn_peer, pCisConn->ft_loca, pCisConn->ft_peer);
    }


    /* when FT = 1: min TX change = nse/bn,  rtn = nse/bn - 1
     * when FT add 1, TX change add nse
     * so  rtn = (ft-1)*nse + (nse/bn - 1)    */


    pCigMaster->cism_isoIntvl      = pCigMaster->cism_intvl_us / 1250;
    pCigMaster->cism_bSlotInterval = pCigMaster->cism_isoIntvl * 2;  //1.25mS -> 625 uS
    pCigMaster->cism_sSlotInterval = pCigMaster->cism_isoIntvl * 64; //1.25mS -> 19.5 uS
    pCigMaster->cism_intvl_us      = pCigMaster->cism_isoIntvl * 1250;


    pRetParam->status    = BLE_SUCCESS;
    pRetParam->cig_id    = pBackupParam->cig_id;    //CIG_ID
    pRetParam->cis_count = pBackupParam->cis_count; //CIS_Count


    tlkapi_send_string_data(IUT_HCI_LOG_EN, "@return ", pRetParam, 3 + 2 * pBackupParam->cis_count);


    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setCigParamsTest(hci_le_setCigParamTest_cmdParam_t *pCmdParam, hci_le_setCigParam_retParam_t *pRetParam)
{
    #if (BQB_TEST_EN)
    if (pCmdParam->cis_count <= IXIT_MAX_CIS_NUM_IN_PER_CIG) //corresponding to IXIT
    #else
    if (pCmdParam->cis_count <= CIS_IN_CIGM_NUM_MAX)
    #endif
    {
        tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Set_Cig_Params_Test", pCmdParam, 15 + sizeof(cigParamTest_cisCfg_t) * pCmdParam->cis_count);
    } else {
        tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Cig Param cis count too big", &pCmdParam->cis_count, 1);

        /*
         If the Host attempts to set CIS parameters that exceed the maximum
        supported connections in the Controller, the Controller shall return the error
        code Connection Limit Exceeded (0x09).
         */
        pRetParam->status = HCI_ERR_CONN_LIMIT_EXCEEDED;
        return HCI_ERR_CONN_LIMIT_EXCEEDED;
    }

    /*
    If the Host issues this command when the CIG is not in the configurable state,
    the Controller shall return the error code Command Disallowed (0x0C).

    If the Host attempts to create a CIG or set parameters that exceed the
    maximum supported resources in the Controller, the Controller shall return the
    error code Memory Capacity Exceeded (0x07).

    If the Host attempts to set an invalid combination of CIS parameters, the
    Controller shall return the error code Unsupported Feature or Parameter Value
    (0x11).
    */

    ble_sts_t ret_status = blt_hci_checkCigTestParams(pCmdParam);
    if (ret_status != BLE_SUCCESS) {
        BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99910000);
        pRetParam->status = ret_status;
        return ret_status;
    }


    ret_status = blc_hci_le_setCigCommon(pCmdParam->cig_id, pCmdParam->cis_count);
    if (ret_status != BLE_SUCCESS) {
        pRetParam->status = ret_status;
        return ret_status;
    }


    ll_cig_mst_t *pCigMaster = latest_pCigMst; //important
    pCigMaster->ull_used     = 0;
    pCigMaster->sca          = pCmdParam->sca;
    pCigMaster->cism_packing = pCmdParam->packing;
    pCigMaster->cig_frame    = pCmdParam->framing;
    smemcpy(&pCigMaster->sdu_int_loca, pCmdParam->sdu_int_m2s, 3);
    smemcpy(&pCigMaster->sdu_int_peer, pCmdParam->sdu_int_s2m, 3);


    /*code below: parameters decided by Host in "LE Set CIG Parameters Test command",
                  but controller can decide by itself when use "LE Set CIG Parameters command" */
    // Parameters set by "LE Set CIG Parameters Test command": FT/Iso_interval/NSE[i]/Max_PDU[i]/BN[i]
    pCigMaster->ft_m2s             = pCmdParam->ft_m2s;
    pCigMaster->ft_s2m             = pCmdParam->ft_s2m;
    pCigMaster->cism_isoIntvl      = pCmdParam->iso_intvl;
    pCigMaster->cism_bSlotInterval = pCmdParam->iso_intvl * 2;  //1.25mS -> 625 uS
    pCigMaster->cism_sSlotInterval = pCmdParam->iso_intvl * 64; //1.25mS -> 19.5 uS
    pCigMaster->cism_intvl_us      = pCmdParam->iso_intvl * 1250;
    /////////////////////////////////////////////////////////////////////////////////////////////


    ///// find available CIS connection for current CIG ///////////////////
    #if (DBG_CIS_CENTRAL_PARAM)
    tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] cis cnt, iso int, sdu int", pCmdParam->cis_count, blt_debug_hex_2_dec_display(pCmdParam->iso_intvl * 1250), blt_debug_hex_2_dec_display(pCigMaster->sdu_int_loca), blt_debug_hex_2_dec_display(pCigMaster->sdu_int_peer));

    tlkapi_send_string_u8s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] ft m/s, pack, frame", pCmdParam->ft_m2s, pCmdParam->ft_s2m, pCmdParam->packing, pCmdParam->framing);
    #endif


    int            new_cis_cnt = 0;
    ll_cis_conn_t *pCisConn;

    for (int cis_idx = 0; cis_idx < bltCisMng.maxNum_cisMaster; cis_idx++) {
        pCisConn = (ll_cis_conn_t *)(global_pCisConn + cis_idx);

        if (pCisConn->cis_occupied == 0) { //available

            blt_ll_cism_allocate_common(pCisConn, pCigMaster);
            pCisConn->cis_ID = pCmdParam->cisCfg[new_cis_cnt].cis_id;


            pCisConn->nse          = pCmdParam->cisCfg[new_cis_cnt].nse;
            pCisConn->phy_ms       = pCmdParam->cisCfg[new_cis_cnt].phy_m2s;
            pCisConn->max_sdu_loca = pCmdParam->cisCfg[new_cis_cnt].max_sdu_m2s;
            pCisConn->max_sdu_peer = pCmdParam->cisCfg[new_cis_cnt].max_sdu_s2m;
            pCisConn->max_pdu_loca = pCmdParam->cisCfg[new_cis_cnt].max_pdu_m2s;
            pCisConn->max_pdu_peer = pCmdParam->cisCfg[new_cis_cnt].max_pdu_s2m;
            pCisConn->bn_loca      = pCmdParam->cisCfg[new_cis_cnt].bn_m2s;
            pCisConn->bn_peer      = pCmdParam->cisCfg[new_cis_cnt].bn_s2m;
            pCisConn->ft_loca      = pCigMaster->ft_m2s;
            pCisConn->ft_peer      = pCigMaster->ft_s2m;

            pCisConn->iso_intvl_us   = pCigMaster->cism_intvl_us;
            pCisConn->iso_intvl_tick = pCigMaster->cism_intvl_us * SYSTEM_TIMER_TICK_1US;

    #if (DBG_CIS_CENTRAL_PARAM)
            tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] sdu pdu", blt_debug_hex_2_dec_display(pCisConn->max_sdu_loca), blt_debug_hex_2_dec_display(pCisConn->max_sdu_peer), blt_debug_hex_2_dec_display(pCisConn->max_pdu_loca), blt_debug_hex_2_dec_display(pCisConn->max_pdu_peer));
            tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] nse phy bn ", pCisConn->nse, pCisConn->phy_ms, pCisConn->bn_loca, pCisConn->bn_peer);

    #endif


            /* Our Controller only supports asymmetric PHYs. */
            if (pCisConn->phy_ms == PHY_PREFER_1M) {
                pCisConn->curCisPhy = BLE_PHY_1M;
            } else if (pCisConn->phy_ms == PHY_PREFER_2M) {
                pCisConn->curCisPhy = BLE_PHY_2M;
            } else {
                pCisConn->curCisPhy = BLE_PHY_CODED; //take it as S8
            }


            pCigMaster->cism_set_msk |= BIT(cis_idx);
            pCigMaster->cism_set_order[pCigMaster->cism_set_cnt] = cis_idx;
            pCigMaster->cism_set_cnt++;

            pRetParam->cis_connHandle[new_cis_cnt] = pCisConn->cis_connHandle;
            new_cis_cnt++;
            if (new_cis_cnt >= pCmdParam->cis_count) {
                break;
            }
        }
    }

    if (pCigMaster->cism_set_cnt != pCmdParam->cis_count) {
        BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99940000 | pCigMaster->cism_set_cnt << 8 | pCmdParam->cis_count);
    }


    pRetParam->status    = BLE_SUCCESS;
    pRetParam->cig_id    = pCmdParam->cig_id;    //CIG_ID
    pRetParam->cis_count = pCmdParam->cis_count; //CIS_Count

    tlkapi_send_string_data(IUT_HCI_LOG_EN, "@return ", pRetParam, 3 + 2 * pCmdParam->cis_count);


    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setCigParams_V3(hci_le_setCigParamV3_cmdParam_t *pCmdParam, hci_le_setCigParam_retParam_t *pRetParam)
{

    return BLE_SUCCESS;
}
ble_sts_t blc_hci_le_setCigParamsTest_v3(hci_le_setCigParamTestV3_cmdParam_t *pCmdParam, hci_le_setCigParam_retParam_t *pRetParam)
{

    return BLE_SUCCESS;
}

    #if (ULL_FOR_CIS_EN)
ble_sts_t blc_hci_le_setCigParamsULL(hci_le_setCigParamTest_cmdParam_t *pCmdParam, hci_le_setCigParam_retParam_t *pRetParam)
{
        #if (BQB_TEST_EN)
    if (pCmdParam->cis_count <= IXIT_MAX_CIS_NUM_IN_PER_CIG) //corresponding to IXIT
        #else
    if (pCmdParam->cis_count <= CIS_IN_CIGM_NUM_MAX)
        #endif
    {
        tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Set_Cig_Params_ULL", pCmdParam, 15 + sizeof(cigParamTest_cisCfg_t) * pCmdParam->cis_count);
    } else {
        tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] ERROR: Cig Param cis count too big", &pCmdParam->cis_count, 1);

        /*
         If the Host attempts to set CIS parameters that exceed the maximum
        supported connections in the Controller, the Controller shall return the error
        code Connection Limit Exceeded (0x09).
         */
        pRetParam->status = HCI_ERR_CONN_LIMIT_EXCEEDED;
        return HCI_ERR_CONN_LIMIT_EXCEEDED;
    }

    /*
    If the Host issues this command when the CIG is not in the configurable state,
    the Controller shall return the error code Command Disallowed (0x0C).

    If the Host attempts to create a CIG or set parameters that exceed the
    maximum supported resources in the Controller, the Controller shall return the
    error code Memory Capacity Exceeded (0x07).

    If the Host attempts to set an invalid combination of CIS parameters, the
    Controller shall return the error code Unsupported Feature or Parameter Value
    (0x11).
    */

    ble_sts_t ret_status = blt_hci_checkCigTestParams(pCmdParam);
    if (ret_status != BLE_SUCCESS) {
        BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99910000);
        pRetParam->status = ret_status;
        return ret_status;
    }

    /*************************************************************
     * For ULL:
     *
     *  BN = NSE
     *  FT = 1
     *  ISO_Interval = NSE x Sub_Interval
     *  SDU_Interval = Report_Interval = Sub_Interval
     *  ISO_Interval: one of 5, 6.25, 7.5, 8.75, 10, 11.25, 12.5, 13.75, 15, 16.25, 17.5, 18.75, or 20 ms
     *************************************************************/
    if (pCmdParam->packing == PACK_SEQUENTIAL) {
        if (pCmdParam->cis_count > 1) {
            for (int i = 0; i < pCmdParam->cis_count; i++) {
                if (pCmdParam->cisCfg[i].nse != 1) {
                    pRetParam->status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
                    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] ERROR: Cis Param PACK_SEQUENTIAL NSE != 1", 0, 0);
                    return HCI_ERR_INVALID_HCI_CMD_PARAMS;
                }
            }
        }
    } else { //PACK_INTERLEAVED
        if (pCmdParam->cis_count > 1) {
            for (int i = 0; i < pCmdParam->cis_count; i++) {
                if (pCmdParam->cisCfg[i].nse != pCmdParam->cis_count) {
                    pRetParam->status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
                    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] ERROR: Cis Param PACK_INTERLEAVED NSE != 1", 0, 0);
                    return HCI_ERR_INVALID_HCI_CMD_PARAMS;
                }
            }
        }
    }

    u32 iso_intvl_us = pCmdParam->iso_intvl * 1250;
    if (iso_intvl_us < 5000 || iso_intvl_us > 20000) {
        pRetParam->status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
        tlkapi_printf(IUT_HCI_LOG_EN, "[HCI][CMD] ERROR: Cis Param ISO_Interval [5ms~ 20ms] %d", iso_intvl_us);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }
    /* check BN, NSE, FT */
    //FT must equal to 1
    if (pCmdParam->ft_m2s != 1 || pCmdParam->ft_s2m != 1) {
        pRetParam->status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
        tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] ERROR: Cis Param FT != 1", 0, 0);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    cigParamTest_cisCfg_t *pCisCfg;
    u8                     bn_last, bn = 1;
    u32                    se_length_us_last  = 0;
    u32                    se_length_total_us = 0;
    //u32 se_length_us_tmp[pCmdParam->cis_count]; //gcc-99
    for (int i = 0; i < pCmdParam->cis_count; i++) {
        pCisCfg = (cigParamTest_cisCfg_t *)&pCmdParam->cisCfg[i];
        //BN must equal to NSE
        bn = max(pCisCfg->bn_m2s, pCisCfg->bn_s2m);
        if (bn == 0 || bn != pCisCfg->nse) {
            pRetParam->status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
            tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] ERROR: Cis Param BN != NSE", 0, 0);
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
        if (i >= 1) {
            if (bn_last != bn) { //different CIS use same BN and NSEs
                pRetParam->status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
                tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] ERROR: Cis Param BN error", 0, 0);
                return HCI_ERR_INVALID_HCI_CMD_PARAMS;
            }
        }
        bn_last = bn;

        /* Sub_Interval check */
        le_phy_type_t curCisPhy_m2s = BLE_PHY_CODED;
        if (pCisCfg->phy_m2s == PHY_PREFER_1M) {
            curCisPhy_m2s = BLE_PHY_1M;
        } else if (pCisCfg->phy_m2s == PHY_PREFER_2M) {
            curCisPhy_m2s = BLE_PHY_2M;
        }
        le_phy_type_t curCisPhy_s2m = BLE_PHY_CODED;
        if (pCisCfg->phy_s2m == PHY_PREFER_1M) {
            curCisPhy_s2m = BLE_PHY_1M;
        } else if (pCisCfg->phy_s2m == PHY_PREFER_2M) {
            curCisPhy_s2m = BLE_PHY_2M;
        }
        //use encryption mode to calculate SE_Length
        u32 tx_rx_max_us = blt_phy_getRfPacketTime_us(pCisCfg->max_pdu_m2s + 4, curCisPhy_m2s, LE_CODED_S8) +
                           blt_phy_getRfPacketTime_us(pCisCfg->max_pdu_s2m + 4, curCisPhy_s2m, LE_CODED_S8);
        if (i >= 1) {
            if (se_length_us_last != (tx_rx_max_us + BLE_T_IFS + CIS_T_MSS)) { //different CIS use same BN and NSEs
                pRetParam->status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
                tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] ERROR: Cis Param BN error", 0, 0);
                return HCI_ERR_INVALID_HCI_CMD_PARAMS;
            }
        }

        se_length_us_last = tx_rx_max_us + BLE_T_IFS + CIS_T_MSS;
        //se_length_us_tmp[i] = se_length_us_last;
        se_length_total_us += se_length_us_last;
    }
    u32 sub_intvl_min_us;
    if (pCmdParam->packing == PACK_SEQUENTIAL) {
        sub_intvl_min_us = se_length_us_last;
    } else {
        sub_intvl_min_us = se_length_total_us;
    }
    u32 sdu_int_m2s_us = (pCmdParam->sdu_int_m2s[0]) | (pCmdParam->sdu_int_m2s[1] << 8) | (pCmdParam->sdu_int_m2s[2] << 16);
    u32 sdu_int_s2m_us = (pCmdParam->sdu_int_s2m[0]) | (pCmdParam->sdu_int_s2m[1] << 8) | (pCmdParam->sdu_int_s2m[2] << 16);
    u32 sdu_int_us     = max(sdu_int_m2s_us, sdu_int_s2m_us);
    //ISO_Interval = NSE x Sub_Interval
    //SDU_Interval = Report_Interval = Sub_Interval
    if (sdu_int_us < sub_intvl_min_us) {
        pRetParam->status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
        tlkapi_printf(IUT_HCI_LOG_EN, "[HCI][CMD] ERROR: Cis Param SDU_Interval < Sub_Interval_Min:%d,%d", sdu_int_us, sub_intvl_min_us);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    // ISO_Interval = NSE x Sub_Interval
    // SDU_Interval = Report_Interval = Sub_Interval
    u8  sub_itvl_chk_err = TRUE;
    u32 sub_itvl_base    = iso_intvl_us / bn;

    if (sdu_int_us != sub_itvl_base) {
        sub_itvl_chk_err = 1;
        tlkapi_printf(IUT_HCI_LOG_EN, "[HCI][CMD] ULL: SDU_Interval = Report_Interval = Sub_Interval = %d us", sdu_int_us);
    } else {
        u32 mod_1ms    = sub_itvl_base % 1000;                             //1ms unit
        u32 num_1ms    = sub_itvl_base / 1000;
        u32 mod_1p25ms = sub_itvl_base % 1250;                             //1.25ms unit
        u32 num_1p25ms = sub_itvl_base / 1250;
        if (mod_1ms == 0 && (num_1ms >= 1 && num_1ms <= 5)) {              //[1ms ~ 5ms]
            sub_itvl_chk_err = FALSE;
        } else if (mod_1p25ms == 0 && (num_1p25ms >= 1 && num_1ms <= 4)) { //[1.25ms ~ 5ms]
            sub_itvl_chk_err = FALSE;
        } else {
            sub_itvl_chk_err = 2;
        }
    }

    if (sub_itvl_chk_err) {
        pRetParam->status = HCI_ERR_INVALID_HCI_CMD_PARAMS;
        tlkapi_printf(IUT_HCI_LOG_EN, "[HCI][CMD] ERROR: Cis Param Sub_Interval[1,2,3,4,5,1.25,2.5,3.75 ms]:%d", sub_itvl_chk_err);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    ret_status = blc_hci_le_setCigCommon(pCmdParam->cig_id, pCmdParam->cis_count);
    if (ret_status != BLE_SUCCESS) {
        pRetParam->status = ret_status;
        return ret_status;
    }


    ll_cig_mst_t *pCigMaster = latest_pCigMst; //important

    pCigMaster->ull_used     = 1;
    pCigMaster->sca          = pCmdParam->sca;
    pCigMaster->cism_packing = pCmdParam->packing;
    pCigMaster->cig_frame    = pCmdParam->framing;
    smemcpy(&pCigMaster->sdu_int_loca, pCmdParam->sdu_int_m2s, 3);
    smemcpy(&pCigMaster->sdu_int_peer, pCmdParam->sdu_int_s2m, 3);


    /*code below: parameters decided by Host in "LE Set CIG Parameters Test command",
                  but controller can decide by itself when use "LE Set CIG Parameters command" */
    // Parameters set by "LE Set CIG Parameters Test command": FT/Iso_interval/NSE[i]/Max_PDU[i]/BN[i]
    pCigMaster->ft_m2s             = pCmdParam->ft_m2s;
    pCigMaster->ft_s2m             = pCmdParam->ft_s2m;
    pCigMaster->cism_isoIntvl      = pCmdParam->iso_intvl;
    pCigMaster->cism_bSlotInterval = pCmdParam->iso_intvl * 2;    //1.25mS -> 625 uS
    pCigMaster->cism_sSlotInterval = pCmdParam->iso_intvl * 64;   //1.25mS -> 19.5 uS
    pCigMaster->cism_intvl_us      = pCmdParam->iso_intvl * 1250; //Time = N �� 1.25 ms
        /////////////////////////////////////////////////////////////////////////////////////////////


        ///// find available CIS connection for current CIG ///////////////////
        #if (DBG_CIS_CENTRAL_PARAM)
    tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] cis cnt, iso int, sdu int", pCmdParam->cis_count, blt_debug_hex_2_dec_display(pCmdParam->iso_intvl * 1250), blt_debug_hex_2_dec_display(pCigMaster->sdu_int_loca), blt_debug_hex_2_dec_display(pCigMaster->sdu_int_peer));

    tlkapi_send_string_u8s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] ft m/s, pack, frame", pCmdParam->ft_m2s, pCmdParam->ft_s2m, pCmdParam->packing, pCmdParam->framing);
        #endif


    int            new_cis_cnt = 0;
    ll_cis_conn_t *pCisConn;

    for (int cis_idx = 0; cis_idx < bltCisMng.maxNum_cisMaster; cis_idx++) {
        pCisConn = (ll_cis_conn_t *)(global_pCisConn + cis_idx);

        if (pCisConn->cis_occupied == 0) { //available

            blt_ll_cism_allocate_common(pCisConn, pCigMaster);

            pCisConn->cis_ID       = pCmdParam->cisCfg[new_cis_cnt].cis_id;
            pCisConn->nse          = pCmdParam->cisCfg[new_cis_cnt].nse;
            pCisConn->phy_ms       = pCmdParam->cisCfg[new_cis_cnt].phy_m2s;
            pCisConn->max_sdu_loca = pCmdParam->cisCfg[new_cis_cnt].max_sdu_m2s;
            pCisConn->max_sdu_peer = pCmdParam->cisCfg[new_cis_cnt].max_sdu_s2m;
            pCisConn->max_pdu_loca = pCmdParam->cisCfg[new_cis_cnt].max_pdu_m2s;
            pCisConn->max_pdu_peer = pCmdParam->cisCfg[new_cis_cnt].max_pdu_s2m;
            pCisConn->bn_loca      = pCmdParam->cisCfg[new_cis_cnt].bn_m2s;
            pCisConn->bn_peer      = pCmdParam->cisCfg[new_cis_cnt].bn_s2m;
            pCisConn->ft_loca      = pCigMaster->ft_m2s;
            pCisConn->ft_peer      = pCigMaster->ft_s2m;

            pCisConn->iso_intvl_us   = pCigMaster->cism_intvl_us;
            pCisConn->iso_intvl_tick = pCigMaster->cism_intvl_us * SYSTEM_TIMER_TICK_1US;

            pCisConn->sub_intvl_us = sdu_int_us;

        #if (DBG_CIS_CENTRAL_PARAM)
            tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] sdu pdu", blt_debug_hex_2_dec_display(pCisConn->max_sdu_loca), blt_debug_hex_2_dec_display(pCisConn->max_sdu_peer), blt_debug_hex_2_dec_display(pCisConn->max_pdu_loca), blt_debug_hex_2_dec_display(pCisConn->max_pdu_peer));
            tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] nse phy bn ", pCisConn->nse, pCisConn->phy_ms, pCisConn->bn_loca, pCisConn->bn_peer);

        #endif


            /* Our Controller only supports asymmetric PHYs. */
            if (pCisConn->phy_ms == PHY_PREFER_1M) {
                pCisConn->curCisPhy = BLE_PHY_1M;
            } else if (pCisConn->phy_ms == PHY_PREFER_2M) {
                pCisConn->curCisPhy = BLE_PHY_2M;
            } else {
                pCisConn->curCisPhy = BLE_PHY_CODED; //take it as S8
            }


            pCigMaster->cism_set_msk |= BIT(cis_idx);
            pCigMaster->cism_set_order[pCigMaster->cism_set_cnt] = cis_idx;
            pCigMaster->cism_set_cnt++;

            pRetParam->cis_connHandle[new_cis_cnt] = pCisConn->cis_connHandle;
            new_cis_cnt++;
            if (new_cis_cnt >= pCmdParam->cis_count) {
                break;
            }
        }
    }

    if (pCigMaster->cism_set_cnt != pCmdParam->cis_count) {
        BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99940000 | pCigMaster->cism_set_cnt << 8 | pCmdParam->cis_count);
    }


    pRetParam->status    = BLE_SUCCESS;
    pRetParam->cig_id    = pCmdParam->cig_id;    //CIG_ID
    pRetParam->cis_count = pCmdParam->cis_count; //CIS_Count

    tlkapi_send_string_data(IUT_HCI_LOG_EN, "@return ", pRetParam, 3 + 2 * pCmdParam->cis_count);


    return BLE_SUCCESS;
}
    #endif //end of ULL for CIS


ble_sts_t blc_hci_le_createCis(hci_le_CreateCisParams_t *pCisPara)
{
    /*
    If any ACL_Connection_Handle[i] is not the handle of an existing ACL
    connection or any CIS_Connection_Handle[i] is not the handle of a CIS or CIS
    configuration, the Controller shall return the error code Unknown Connection
    Identifier (0x02).                                                                          Done !!!

    If the Host attempts to create a CIS that has already been created, the
    Controller shall return the error code Connection Already Exists (0x0B).                    Done !!!

    If two different elements of the CIS_Connection_Handle arrayed parameter
    identify the same CIS, the Controller shall return the error code Invalid HCI
    Command Parameters (0x12).                                                                  Done !!!

    If the Host issues this command before all the HCI_LE_CIS_Established
    events from the previous use of the command have been generated, the
    Controller shall return the error code Command Disallowed (0x0C).                           Done !!!

    If the Host issues this command on an ACL_Connection_Handle where the
    Controller is the Peripheral, the Controller shall return the error code Command
    Disallowed (0x0C)                                                                           Done !!!

    If the Host issues this command when the Connected Isochronous Stream
    (Host Support) feature bit (see [Vol 6] Part B, Section 4.6.27) is not set, the
    Controller shall return the error code Command Disallowed (0x0C).                           Done !!!
    */

    if (pCisPara->cis_count <= CIS_IN_CIGM_NUM_MAX) {
        tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Create_Cis", pCisPara, 1 + sizeof(cisConnParams_t) * pCisPara->cis_count);

        //private define
        if (pCisPara->cis_count == 0) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS; //0x12
        }
    } else {
        tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Create_Cis, cis count exceed", &pCisPara->cis_count, 1);
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }


    if (!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_ISOCHRONOUS_CHANNELS)) {
        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] no set ISO bit", 0, 0);
        return HCI_ERR_CMD_DISALLOWED; //0x0C
    }

    ll_cis_conn_t *pCisConn;
    st_ll_conn_t  *pAclConn = NULL; //give NULL to avoid compile warning


    u16 cis_handle[CIS_IN_CIGM_NUM_MAX];
    u16 acl_handle[CIS_IN_CIGM_NUM_MAX];
    u8  cis_conn_idx[CIS_IN_CIGM_NUM_MAX];
    u8  acl_conn_idx[CIS_IN_CIGM_NUM_MAX];
    u32 cis_idx_msk = 0; //initial value must be 0

    for (int i = 0; i < pCisPara->cis_count; i++) {
        acl_handle[i]   = pCisPara->cisConn[i].acl_handle;
        acl_conn_idx[i] = (acl_handle[i] & CONN_IDX_MASK);

        cis_handle[i]   = pCisPara->cisConn[i].cis_handle;
        cis_conn_idx[i] = (cis_handle[i] & BLT_CIS_IDX_MSK);
        cis_idx_msk |= BIT(cis_conn_idx[i]);


        if (blt_ll_isAclhdlInvalid(acl_handle[i]) == BLE_SUCCESS) {
            if (acl_conn_idx[i] >= LL_MAX_ACL_CEN_NUM) { //ACL slave
                tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] acl handle slave", &acl_handle[i], 2);
                return HCI_ERR_CMD_DISALLOWED;
            }
        } else {
            tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] acl handle error", &acl_handle[i], 2);
            return HCI_ERR_UNKNOWN_CONN_ID; //0x02
        }


        /* two different CIS handle identify the same CIS */
        for (int j = 0; j < i; j++) {
            if (cis_handle[i] == cis_handle[j]) {
                tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] repeated cis handle, exit", 0, 0);
                return HCI_ERR_INVALID_HCI_CMD_PARAMS; // HCI_CIS/BV-05-C test this logic
            }
        }

        /* correct range: 0x0020 ... 0x0020 | (bltCisMng.maxNum_cisMaster - 1) */
        if (cis_handle[i] < (BLT_CIS_HANDLE) || cis_handle[i] >= (BLT_CIS_HANDLE + bltCisMng.maxNum_cisMaster)) {
            tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] cis handle not exist", &cis_handle[i], 2);
            return HCI_ERR_UNKNOWN_CONN_ID; //0x02
        } else {
            pCisConn = (ll_cis_conn_t *)(global_pCisConn + cis_conn_idx[i]);
            if (!pCisConn->cis_occupied) {
                tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] cis handle not in configuration", &cis_handle[i], 2);
                return HCI_ERR_UNKNOWN_CONN_ID;
            } else if (pCisConn->createCmd) {       //
                tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] create a CIS that is already created", &cis_handle[i], 2);
                return HCI_ERR_CONN_ALREADY_EXISTS; // HCI/CIS/BV-02-C test this logic
            }
        }
    }


    /* though it's not specified, I believe all CIS in this command should be in one CIG */
    ll_cig_mst_t *pCigMaster = NULL;
    int           i;
    for (i = 0; i < bltCisMng.maxNum_cig_mst; i++) {
        pCigMaster = (ll_cig_mst_t *)(global_pCigMst + i);

        if (pCigMaster->cig_ID_mas != CIG_ID_INVALID) {
            if ((pCigMaster->cism_set_msk & cis_idx_msk) == cis_idx_msk) { //all CIS handles are in this CIG
                break;
            }
        }
    }

    if (i >= bltCisMng.maxNum_cig_mst) {       //CISes not allocated or CISes not in one CIG
        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] cis not alloced or not in one CIG", 0, 0);
        return HCI_ERR_UNKNOWN_CONN_ID;        //CIS_CEN_BV-51-C test it
    }

    if (pCigMaster->cism_create_pending_msk) { //can not create new CIS before established event
        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] previous cis create not done", &pCigMaster->cism_create_pending_msk, 1);
        return HCI_ERR_CMD_DISALLOWED;         //0x0C
    }


    /// When code runs here, at least one CIS should be issued ///
    ///////////////// All potential error parameters have processed, now we begin logic coding ////////////////
    for (i = 0; i < pCisPara->cis_count; i++) {
        pCisConn = (ll_cis_conn_t *)(global_pCisConn + cis_conn_idx[i]);

        if (!pCisConn->cis_occupied) {
            BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99950000); //previous code have protect this logic
        }

        //1. CIG parameters process

        //2. CIS connection parameters process
        pCisConn->link_acl_index  = acl_conn_idx[i];
        pCisConn->link_acl_handle = acl_handle[i];


        pAclConn = (st_ll_conn_t *)&blms[pCisConn->link_acl_index];


        blt_cis_calculateInterval(pCisConn, pAclConn->conn_intvl_n_1m25, pCigMaster->cism_isoIntvl);


        //3. ACL connection parameters process
        //for 1 ACL mapping 2 or more CIS, no need worry about overwrite, they must on same CIG
        pAclConn->alink_cig_idx = pCisConn->clink_cig_idx;


        /* TX & RX max PDU translate time, uS */
        u8  mic_len      = pAclConn->crypt.enable ? 4 : 0;
        u8  mic_len_loca = pCisConn->max_pdu_loca ? mic_len : 0;
        u8  mic_len_peer = pCisConn->max_pdu_peer ? mic_len : 0;
        int tx_rx_max_us = blt_phy_getRfPacketTime_us(pCisConn->max_pdu_loca + mic_len_loca, pCisConn->curCisPhy, LE_CODED_S8) +
                           blt_phy_getRfPacketTime_us(pCisConn->max_pdu_peer + mic_len_peer, pCisConn->curCisPhy, LE_CODED_S8);


        pCisConn->MPTM_TIFS_MPTS = tx_rx_max_us + BLE_T_IFS;
        pCisConn->se_length_us   = pCisConn->MPTM_TIFS_MPTS + CIS_T_MSS;
        pCisConn->se_length_tick = pCisConn->se_length_us * SYSTEM_TIMER_TICK_1US;
        pCisConn->cis_task_us    = pCisConn->se_length_us * pCisConn->nse;


        pCisConn->createCmd    = 1;
        pCisConn->createStatus = 0;
        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] first create", 0, 0);

        if (i == 0) { //CIS start from first
            //at least one CIS should be created
            bltCisMng.cisFlow_pending |= BIT(cis_conn_idx[0]);
            bltCisMng.cisFlow_idx = cis_conn_idx[0];
            pCisConn->cisFlowFlg  = CIS_FLOW_MASTER_START_NEW_CIS;
        }
    }

    pCigMaster->config_state = 0;
    pCigMaster->cism_create_msk |= cis_idx_msk; //attention: here use " |= "
    pCigMaster->cism_create_pending_msk = cis_idx_msk;


    pCigMaster->task_total_us      = 0;
    pCigMaster->se_length_total_us = 0;
    for (i = 0; i < pCigMaster->cism_set_cnt; i++) {
        u8 idx   = pCigMaster->cism_set_order[i];
        pCisConn = (ll_cis_conn_t *)(global_pCisConn + idx);

        if (!(pCigMaster->cism_create_msk & BIT(idx))) {
            /* TX & RX max PDU translate time, uS */
            u8  mic_len      = pAclConn->crypt.enable ? 4 : 0; //use one of specified CIS's ACL connection
            u8  mic_len_loca = pCisConn->max_pdu_loca ? mic_len : 0;
            u8  mic_len_peer = pCisConn->max_pdu_peer ? mic_len : 0;
            int tx_rx_max_us = blt_phy_getRfPacketTime_us(pCisConn->max_pdu_loca + mic_len_loca, pCisConn->curCisPhy, LE_CODED_S8) +
                               blt_phy_getRfPacketTime_us(pCisConn->max_pdu_peer + mic_len_peer, pCisConn->curCisPhy, LE_CODED_S8);

            pCisConn->MPTM_TIFS_MPTS = tx_rx_max_us + BLE_T_IFS;
            pCisConn->se_length_us   = pCisConn->MPTM_TIFS_MPTS + CIS_T_MSS;
            pCisConn->se_length_tick = pCisConn->se_length_us * SYSTEM_TIMER_TICK_1US;
            pCisConn->cis_task_us    = pCisConn->se_length_us * pCisConn->nse;
        }

        pCigMaster->task_total_us += pCisConn->cis_task_us;
        pCigMaster->se_length_total_us += pCisConn->se_length_us;


        tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] cis timing 1", blt_debug_hex_2_dec_display(pCisConn->MPTM_TIFS_MPTS), blt_debug_hex_2_dec_display(pCisConn->se_length_us), pCisConn->nse, blt_debug_hex_2_dec_display(pCisConn->cis_task_us));
    }


    return BLE_SUCCESS;
}

/*
 * assume that when stack call this function, cisHandle is valid, no need check again
 */
_attribute_no_inline_
    ble_sts_t
    blt_ll_createCisCancel(ll_cis_conn_t *pCisConn)
{
    //may need consider later: pCisConn->cis_reject_reason;
    u32 r = irq_disable();
    if (pCisConn->createStatus & CREATE_STATE_SET_1STAP) { //use IRQ task trigger disconnect
        pCisConn->cis_termin_union.local_terminate = HCI_ERR_CONN_TERM_BY_LOCAL_HOST;

        /* maybe change to established for a IRQ task due to timing diff,
         * clear "cis Flow Flag" can stop established event in main_loop */
        pCisConn->cisFlowFlg = CIS_FLOW_IDLE;
    } else {
        if (pCisConn->createStatus & CREATE_STATE_SEND_IND) {
            blmsParam.cig_mas_1st_sche_build_pending = 0; //clear this to stop 1st AP set
        }
        //      if(pCisConn->createStatus & CREATE_STATE_START){
        //
        //      }

        /* give a manual disconnect event */
        pCisConn->discon_evt                        = 1;
        pCisConn->cis_termin_union.terminate_reason = HCI_ERR_CONN_TERM_BY_LOCAL_HOST;
    }
    irq_restore(r);


    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Create_Cis_Cancel", 0, 0);

    st_ll_conn_t *pAclConn   = (st_ll_conn_t *)&blms[pCisConn->link_acl_index];
    pAclConn->ignore_cis_cmd = 1;

    ll_cig_mst_t *pCigMaster = (ll_cig_mst_t *)(global_pCigMst + pCisConn->clink_cig_idx);
    blt_ll_cis_master_cis_establish(pCisConn, pCigMaster, HCI_ERR_OP_CANCELLED_BY_HOST);


    return BLE_SUCCESS;
}

_attribute_no_inline_
    ble_sts_t
    blc_ll_removeCig(u8 cigId)
{
    /*
    The CIG_ID parameter contains the identifier of the CIG.
    This command shall delete the CIG_ID and also delete the
    Connection_Handles of the CIS configurations stored in the CIG.
    This command shall also remove the isochronous data paths that are
    associated with the Connection_Handles of the CIS configurations, which is
    equivalent to issuing the HCI_LE_Remove_ISO_Data_Path command (see
    Section 7.8.109).

    If the Host tries to remove a CIG which is in the active state, then the Controller
    shall return the error code Command Disallowed (0x0C).                                   Done !!!

    If the Host issues this command with a CIG_ID that does not exist, the
    Controller shall return the error code Unknown Connection Identifier (0x02).             Done !!!
    */

    if (cigId == CIG_ID_INVALID || blt_ll_searchExistingCigId(cigId) == CIG_ID_INVALID) {
        return HCI_ERR_UNKNOWN_CONN_ID; // HCI/CIS/BV-02-C test this logic
    }


    ll_cig_mst_t *pCigMaster = latest_pCigMst;


    /*If the Host tries to remove a CIG which has one or more CISes that are
    established, the Controller shall return the error code Command Disallowed
    (0x0C). */
    if (pCigMaster->cism_estab_cnt) {
        return HCI_ERR_CMD_DISALLOWED;
    }

    pCigMaster->cig_ID_mas = CIG_ID_INVALID;
    if (bltCisMng.curNum_cig_mst > LL_CIG_MST_NUM_MAX) {
        BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99A00000);
    }
    bltCisMng.curNum_cig_mst--;


    for (int i = 0; i < pCigMaster->cism_set_cnt; i++) {
        u8             idx      = pCigMaster->cism_set_order[i];
        ll_cis_conn_t *pCisConn = (ll_cis_conn_t *)(global_pCisConn + idx);
        pCisConn->cis_occupied  = 0; //release
    }


    pCigMaster->config_state = 0;

    /*
    This command shall also remove the isochronous data paths that are
    associated with the Connection_Handles of the CISes, which is equivalent to
    issuing the HCI_LE_Remove_ISO_Data_Path command (see Section
    7.8.109).
    */
    //TODO

    return BLE_SUCCESS;
}

_attribute_no_inline_
    ble_sts_t
    blc_hci_le_removeCig(u8 cigId, hci_le_removeCig_retParam_t *pRetParam)
{
    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Remove_Cig", &cigId, 1);

    pRetParam->status = blc_ll_removeCig(cigId);
    pRetParam->cig_id = cigId;

    return pRetParam->status;
}

//0xFF: no existing CIG
//other: cigId is same as existing CIG
int blt_ll_searchExistingCigId(u8 cur_cigId)
{
    for (int i = 0; i < bltCisMng.maxNum_cig_mst; i++) { //find existing CIG
        ll_cig_mst_t *cig_mst = global_pCigMst + i;
        if (cig_mst->cig_ID_mas == cur_cigId) {          //existing CIG_ID set match
            latest_pCigMst = global_pCigMst + i;
            return i;
        }
    }

    return CIG_ID_INVALID; //no CIG available
}

int blt_ll_AllocateNewCigId(u8 cur_cigId)
{
    (void)cur_cigId;                                              //unused, remove warning

    for (int i = 0; i < bltCisMng.maxNum_cig_mst; i++) {          //find new CIG
        if ((global_pCigMst + i)->cig_ID_mas == CIG_ID_INVALID) { //if CIG_ID_INVALID, this CIG can be allocated to new CigId
            latest_pCigMst = global_pCigMst + i;
            return i;
        }
    }

    return CIG_ID_INVALID; //no CIG available
}

_attribute_noinline_ void blt_ll_cism_check_other_cis_create(ll_cis_conn_t *pCisConn, ll_cig_mst_t *pCigMaster)
{
    if (pCigMaster->cism_create_pending_msk & BIT(pCisConn->cis_index)) {
        pCigMaster->cism_create_pending_msk &= ~BIT(pCisConn->cis_index);
    } else { //debug
        BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99960000 | pCigMaster->cism_create_pending_msk << 8 | pCisConn->cis_index);
    }

    if (pCigMaster->cism_create_pending_msk) { //other CIS need to be create

        for (int i = 0; i < bltCisMng.maxNum_cisMaster; i++) {
            if (pCigMaster->cism_create_pending_msk & BIT(i)) {
                bltCisMng.cisFlow_pending |= BIT(i);
                bltCisMng.cisFlow_idx = i;
                ll_cis_conn_t *pCis   = (ll_cis_conn_t *)(global_pCisConn + i); //can not use pCisConn here
                pCis->cisFlowFlg      = CIS_FLOW_MASTER_START_NEW_CIS;
                break;
            }
        }
    }
}

_attribute_noinline_ int blt_ll_cis_master_cis_establish(ll_cis_conn_t *pCisConn, ll_cig_mst_t *pCigMaster, ble_sts_t status)
{
    u8 cis_idx = pCisConn->cis_index;

    bltCisMng.cisFlow_pending &= ~BIT(cis_idx);
    bltCisMng.cisFlow_idx = INVALID_CIS_IDX;
    pCisConn->cisFlowFlg  = CIS_FLOW_IDLE;

    if (status == BLE_SUCCESS) {
        tlkapi_send_string_u32s(DBG_CISCONN_TRACK_EN, "cis conn 8", pCisConn->cis_index, pCisConn->cis_connHandle, pCisConn, 0);

        if (pCigMaster->cism_estab_msk & BIT(cis_idx)) {
            BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x999C0000 | pCigMaster->cism_estab_msk << 8 | cis_idx);
        }
        pCigMaster->cism_estab_msk |= BIT(cis_idx);
        pCigMaster->cism_estab_cnt++;

        pCigMaster->cis_ap_distan_mark_us[pCisConn->cism_alloc_position] = pCisConn->cig_ap_distan_us;
        pCigMaster->last_alloc_position                                  = pCisConn->cism_alloc_position;

        blt_cis_establish_common(pCisConn);
    } else if (status == HCI_ERR_CONN_FAILED_TO_ESTABLISH) { // Failed to Establish
        pCisConn->createCmd = 0;                             //important
    }


    blt_ll_cism_check_other_cis_create(pCisConn, pCigMaster);

#if LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT
    if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_HDT_LE_CIS_ESTABLISHED_V4) {
        hci_le_cisEstablishedV4_evt(status, pCisConn->cis_connHandle, (u8 *)&pCigMaster->cigm_sync_delay, (u8 *)&pCisConn->cis_sync_delay,
                                  (u8 *)&pCisConn->transLaty_m2s, (u8 *)&pCisConn->transLaty_s2m, pCisConn->curCisPhy, pCisConn->curCisPhy,
                                  pCisConn->nse, pCisConn->bn_loca, pCisConn->bn_peer, pCisConn->ft_loca, pCisConn->ft_peer,
                                  pCisConn->max_pdu_loca, pCisConn->max_pdu_peer, pCigMaster->cism_isoIntvl,(u8 *)&pCisConn->sub_intvl_us,
                                  pCisConn->max_sdu_loca, pCisConn->max_sdu_peer, (u8 *)&pCisConn->sdu_int_loca_us, (u8 *)&pCisConn->sdu_int_peer_us,
                                  pCisConn->cis_frame, pCisConn->rate_m2s, pCisConn->rate_s2m, pCisConn->enc_enable, pCisConn->mic_length);
    } else
#endif
    if (hci_le_eventMask & HCI_LE_EVT_MASK_CIS_ESTABLISHED) {
        hci_le_cisEstablished_evt(status, pCisConn->cis_connHandle, (u8 *)&pCigMaster->cigm_sync_delay, (u8 *)&pCisConn->cis_sync_delay,
                                  (u8 *)&pCisConn->transLaty_m2s, (u8 *)&pCisConn->transLaty_s2m, pCisConn->curCisPhy, pCisConn->curCisPhy,
                                  pCisConn->nse, pCisConn->bn_loca, pCisConn->bn_peer, pCisConn->ft_loca, pCisConn->ft_peer,
                                  pCisConn->max_pdu_loca, pCisConn->max_pdu_peer, pCigMaster->cism_isoIntvl);
    }

    return 1;
}

int blt_ll_cism_clear_establish_status(ll_cis_conn_t *pCisConn, ll_cig_mst_t *pCigMaster)
{
    u8 cis_idx = pCisConn->cis_index;
    if (pCigMaster->cism_estab_msk & BIT(cis_idx)) {
        pCigMaster->cism_estab_msk &= ~BIT(cis_idx);
        pCigMaster->cism_estab_cnt--;
    } else {
        BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99970000 | pCigMaster->cism_estab_msk << 8 | cis_idx);
    }

    pCisConn->cisFlowFlg = CIS_FLOW_IDLE;


    return 0;
}

bool blt_ll_sendCisReq(st_ll_conn_t *pAclConn, ll_cis_conn_t *pCisConn, ll_cig_mst_t *pCigMaster)
{
    tlkapi_send_string_u32s(DBG_CISCONN_TRACK_EN, "cis conn 1", pCisConn->cis_index, pCisConn->cis_connHandle, pCisConn, 0);

    tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] LL_CIS_Req process", &pAclConn->acl_conHandle, 2);
    u8 cur_cis_idx = pCisConn->cis_index;

    u8                      temp_buff[sizeof(rf_packet_ll_cis_req_t)];
    rf_packet_ll_cis_req_t *pCtrlCisReq = (rf_packet_ll_cis_req_t *)temp_buff;
    pCtrlCisReq->type                   = LLID_CONTROL;
    pCtrlCisReq->rf_len                 = sizeof(rf_packet_ll_cis_req_t) - 2;
    pCtrlCisReq->opcode                 = LL_CIS_REQ;


    pCtrlCisReq->cigId  = pCigMaster->cig_ID_mas;                  // decided by host
    pCtrlCisReq->cisId  = pCisConn->cis_ID;                        // decided by host
    pCtrlCisReq->phyM2S = pCtrlCisReq->phyS2M = pCisConn->phy_ms;  // decided by host      le_phy_prefer_type_t


    pCtrlCisReq->maxSduM2S = pCisConn->max_sdu_loca;               // decided by host
    pCtrlCisReq->maxSduS2M = pCisConn->max_sdu_peer;               // decided by host
    pCtrlCisReq->rfu0 = pCtrlCisReq->rfu1 = 0;                     //very important !!!
    pCtrlCisReq->framed                   = pCigMaster->cig_frame; // decided by host
    pCisConn->sdu_int_loca_us             = pCigMaster->sdu_int_loca;
    pCisConn->sdu_int_peer_us             = pCigMaster->sdu_int_peer;


    smemcpy(pCtrlCisReq->sduIntvlM2S, &pCigMaster->sdu_int_loca, 3); // decided by host
    smemcpy(pCtrlCisReq->sduIntvlS2M, &pCigMaster->sdu_int_peer, 3); // decided by host


    #if 1
    tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] SDU PDU", pCisConn->max_sdu_loca, pCisConn->max_sdu_peer, pCisConn->max_pdu_loca, pCisConn->max_pdu_peer);
    #endif


    pCtrlCisReq->maxPduM2S = pCisConn->max_pdu_loca;
    pCtrlCisReq->maxPduS2M = pCisConn->max_pdu_peer;
    pCtrlCisReq->bnM2S     = pCisConn->bn_loca;
    pCtrlCisReq->bnS2M     = pCisConn->bn_peer;
    pCtrlCisReq->ftM2S     = pCisConn->ft_loca;
    pCtrlCisReq->ftS2M     = pCisConn->ft_peer;
    pCtrlCisReq->nse       = pCisConn->nse;
    pCtrlCisReq->isoIntvl  = pCigMaster->cism_isoIntvl;

    #if (ULL_FOR_CIS_EN)
    if (pCigMaster->ull_used) {
        //already have
    } else
    #endif
    {
        if (pCigMaster->cism_packing == PACK_SEQUENTIAL) {
            pCisConn->sub_intvl_us = pCisConn->se_length_us;
        } else {
            pCisConn->sub_intvl_us = pCigMaster->se_length_total_us;
        }
    }

    pCisConn->sub_intvl_tick     = pCisConn->sub_intvl_us * SYSTEM_TIMER_TICK_1US;
    pCisConn->cis_maxPossible_us = pCisConn->sub_intvl_us * (pCisConn->nse - 1) + pCisConn->MPTM_TIFS_MPTS;
    int oftMaxUs_spec            = pAclConn->conn_intvl_n_1m25 * 1250 - pCisConn->cis_maxPossible_us;
    if (oftMaxUs_spec < 1000) {
        BLMS_ERR_DEBUG(DBG_CIS_MASTER_TIMING, 0x99AD0000);
    }


    //blt_debug_hex_2_dec_display(pCisConn->iso_intvl_us - pCisConn->cis_maxPossible_us)
    tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] cis timing 2", blt_debug_hex_2_dec_display(pCisConn->MPTM_TIFS_MPTS), blt_debug_hex_2_dec_display(pCisConn->se_length_us), blt_debug_hex_2_dec_display(pCisConn->cis_maxPossible_us), blt_debug_hex_2_dec_display(oftMaxUs_spec));


    /* IRQ protect to make sure 2 values are a pair, in case IRQ coming when calculating which lead to timing error
     * use other variable to get and store, to decrease IRQ disabling time */
    u32 r                 = irq_disable();
    u32 acl_ap_markTick   = pAclConn->ap_tick_mark;
    u16 acl_conn_markInst = pAclConn->conn_inst_mark;
    irq_restore(r);


    /* attention that "ap tick mark" maybe a future tick in about 100~200uS, because
     * "ap_tick_mark = bltSche.sSlot_tick_irq + IRQ_BTX_SEND_DELAY_US*SYSTEM_TIMER_TICK_1US " is executed in btx start IRQ, main_loop code
     * may run in 100uS */
    u32 tick_now = clock_time();
    int aclEvent_past;
    if ((u32)(acl_ap_markTick - tick_now) < IRQ_BTX_SEND_DELAY_US * SYSTEM_TIMER_TICK_1US) {
        aclEvent_past = 0;
    } else {
        aclEvent_past = (tick_now - acl_ap_markTick) / pAclConn->conn_intvl_tick;
    }


    if (aclEvent_past > 100) {
        tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] ERROR", clock_time(), acl_ap_markTick, aclEvent_past, 0);
        write_dbg32(0x0018, aclEvent_past);
        BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x999A0000);
    }

    u8 delta_inter = 40;
    if (pAclConn->conn_intvl_n_1m25 > CONN_INTERVAL_30MS) {
        delta_inter = 10;
    } else if (pAclConn->conn_intvl_n_1m25 > CONN_INTERVAL_20MS) {
        delta_inter = 15;
    } else if (pAclConn->conn_intvl_n_1m25 > CONN_INTERVAL_15MS) {
        delta_inter = 20;
    } else if (pAclConn->conn_intvl_n_1m25 > CONN_INTERVAL_10MS) {
        delta_inter = 30;
    }

    //tlkapi_send_string_u32s(0, "AAAA1", tick_now, pAclConn->ap_tick_mark, aclEvent_past, delta_inter);


    #if (DBG_CREATE_FF0D_ERROR)
    int inst_inc         = 2;
    u16 future_aclEvtCnt = acl_conn_markInst + inst_inc;
    #else
    int inst_inc         = aclEvent_past + delta_inter;
    u16 future_aclEvtCnt = acl_conn_markInst + inst_inc;
    #endif

    cisConn_param.cis_1st_anchor_evtCnt = future_aclEvtCnt;
    /* when CIS connection not establish, use "cis_1st_anchor_tick" special: mark the ACL anchor point, not consider cis_offset  */
    cisConn_param.cis_1st_anchor_tick = pAclConn->ap_tick_mark + inst_inc * pAclConn->conn_intvl_tick;


    //tlkapi_send_string_u32s(0, "AAAA2", inst_inc, acl_conn_markInst, cisConn_param.cis_1st_anchor_evtCnt, cisConn_param.cis_1st_anchor_tick);


    int oftMinUs = 0, oftMaxUs = 0;
    pCisConn->cism_alloc_position = 0xFF;
    pCisConn->offset_us_const     = 0;
    if (pCigMaster->cism_estab_cnt == 0) {
        pCisConn->cism_alloc_position = 0;
        pCisConn->cig_ap_distan_us    = 0;
        pCigMaster->pcisPos_1st       = pCisConn; //mark

        u32 cigm_maxPossible_us = pCigMaster->task_total_us - CIS_T_MSS;
        int cigm_idle_us        = pCisConn->iso_intvl_us - cigm_maxPossible_us;
        int iso_use_us          = bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US + CIGMST_EARLY_SET_US;

        tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] cig timing", blt_debug_hex_2_dec_display(pCigMaster->task_total_us), blt_debug_hex_2_dec_display(cigm_maxPossible_us), blt_debug_hex_2_dec_display(iso_use_us), blt_debug_hex_2_dec_display(cigm_idle_us));

        u32 temp_cigm_idle_1 = cigm_idle_us;
        (void)temp_cigm_idle_1; //remove compiler warning
        cigm_idle_us -= iso_use_us;

        int offset_min_add_margin = 0;

        if (cigm_idle_us < 10) {
            BLMS_ERR_DEBUG(DBG_CIS_MASTER_TIMING, 0x99AC0000); //remove later
            cigm_idle_us = 0;
        } else {
            if (cigm_idle_us > 1000) {
                offset_min_add_margin = 500;
            } else if (cigm_idle_us > 500) {
                offset_min_add_margin = 200;
            } else if (cigm_idle_us > 200) {
                offset_min_add_margin = 100;
            }


            if (cigm_idle_us > 10000) {
                cigm_idle_us /= 2;
            } else if (cigm_idle_us > 5000) {
                cigm_idle_us -= 2000;
            } else if (cigm_idle_us > 2000) {
                cigm_idle_us -= 1000;
            }
        }

        u32 temp_cigm_idle_2 = cigm_idle_us;
        (void)temp_cigm_idle_2; //remove compiler warning
        if (pCigMaster->cism_packing == PACK_SEQUENTIAL) {
            pCigMaster->offset_range_us = cigm_idle_us / pCigMaster->cism_set_cnt;
        } else {
            pCigMaster->offset_range_us = cigm_idle_us;
        }

        if (pCigMaster->offset_range_us > 5000) {
            pCigMaster->offset_range_us = 5000;
        }


        tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] cig idle, range", blt_debug_hex_2_dec_display(temp_cigm_idle_1), blt_debug_hex_2_dec_display(temp_cigm_idle_2), blt_debug_hex_2_dec_display(offset_min_add_margin), blt_debug_hex_2_dec_display(pCigMaster->offset_range_us));


        if (pCigMaster->cism_packing == PACK_SEQUENTIAL) {
            pCigMaster->cigm_sync_delay = cigm_maxPossible_us + pCigMaster->offset_range_us * (pCigMaster->cism_set_cnt - 1);
        } else { //PACK_INTERLEAVED
            pCigMaster->cigm_sync_delay = cigm_maxPossible_us;
        }


        /* coded phy process for "actual_txrx_sche_us" */ //here influent CIS/CEN/BV_25
        if (pCisConn->curCisPhy == BLE_PHY_CODED) {
            pCigMaster->offset_range_us   = 0;
            pAclConn->actual_txrx_sche_us = 3000;
        }
        oftMinUs = pAclConn->actual_txrx_sche_us + CIGMST_EARLY_SET_US + 20 + offset_min_add_margin;


    #if 0 //when "ACL_MASTER_BASE_INTERVAL FOLLOW_UPPER_LAYER" enable, do not need this workaround.
          //And workaround method is not 100% accurate.  SiHui
                if(pCigMaster->cism_set_cnt > 1){

                    u32 aclM_base_intvl_us = aclMas_param.master_connInter * 1250;
                    if(blmsParam.cur_master_num > 1 && pCigMaster->cism_isoIntvl >= (2*aclMas_param.master_connInter) && \
                        pAclConn->conn_intvl_n_1m25 >= (2*aclMas_param.master_connInter)){
                        //workaround CIS_CEN_BV_10_C
                        oftMinUs += aclM_base_intvl_us;
                    }
                }
    #endif


    #if (ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN)
        //tlkapi_send_string_u32s(0, "offset_cus_en", pCigMaster->offset_cus_en, pCigMaster->offset_cus_aclcIdx, pAclConn->acl_conIndex, pCigMaster->offset_cus_us);
        if (pCigMaster->offset_cus_en && pCigMaster->offset_cus_aclcIdx == pAclConn->acl_conIndex) {
            if (oftMinUs < pCigMaster->offset_cus_us) {
                oftMinUs = pCigMaster->offset_cus_us;
            }
        }
    #endif

        oftMaxUs = oftMinUs + pCigMaster->offset_range_us;

        pCisConn->own_cisOffsetMin_us = oftMinUs;
        pCisConn->own_cisOffsetMax_us = oftMaxUs;

    #if (DBG_CREATE_FF0D_ERROR)
        pCisConn->own_cisOffsetMin_us = 3560;
        pCisConn->own_cisOffsetMax_us = 3560;
    #endif

        /*      tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] 1st cis", blt_debug_hex_2_dec_display(pAclConn->actual_txrx_sche_us), \
                                                                    blt_debug_hex_2_dec_display(pCigMaster->offset_range_us), \
                                                                    blt_debug_hex_2_dec_display(oftMinUs), \
                                                                    blt_debug_hex_2_dec_display(oftMaxUs));
 */

    } else {
        if (pCigMaster->cism_packing == PACK_INTERLEAVED) {
            pCisConn->offset_us_const = 1;
        }


        u32 r1 = irq_disable(); //important !!!
        for (int i = 0; i < CIS_IN_CIGM_NUM_MAX; i++) {
            if (!pCigMaster->cis_alloc_exist[i] && pCigMaster->cis_alloc_markIdx[i] == cur_cis_idx) {
                pCisConn->cism_alloc_position = i;
                pCisConn->cig_ap_distan_us    = pCigMaster->cis_ap_distan_mark_us[i]; //here ap_distan_us is final value
                pCisConn->offset_us_const     = 1;
                break;
            }
        }

        if (pCisConn->cism_alloc_position == 0xFF) {
            if (pCigMaster->last_alloc_position >= CIS_IN_CIGM_NUM_MAX) {
                BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99A90000);
            }
            pCisConn->cism_alloc_position = pCigMaster->last_alloc_position + 1;
            ll_cis_conn_t *pCisLast       = (global_pCisConn + pCigMaster->cis_alloc_markIdx[pCigMaster->last_alloc_position]);

            /* attention: ap_distan_us now is not the final value, will update when sending CIS_IND */
            if (pCigMaster->cism_packing == PACK_INTERLEAVED) {
                pCisConn->cig_ap_distan_us = pCisLast->cig_ap_distan_us + pCisLast->se_length_us;
            } else {
                pCisConn->cig_ap_distan_us = pCisLast->cig_ap_distan_us + pCisLast->cis_task_us;
            }
        }

        /* "cig_refer_point" use other variable to get and store, in case IRQ coming when calculating which lead to timing error */
        u32 cig_refer_point     = pCigMaster->cig_ref_point; //
        int isoInter_past       = (cisConn_param.cis_1st_anchor_tick - cig_refer_point) / pCisConn->iso_intvl_tick;
        u32 cis_ref_future_tick = cig_refer_point + (isoInter_past + 1) * pCisConn->iso_intvl_tick;

        oftMinUs = (cis_ref_future_tick + pCisConn->cig_ap_distan_us * SYSTEM_TIMER_TICK_1US - cisConn_param.cis_1st_anchor_tick) / SYSTEM_TIMER_TICK_1US;

        irq_restore(r1);              //important !!!


        if (oftMinUs > 1000 * 1000) { //assume that ACL interval no bigger than 1000 mS
    #if 0
            tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] offset min ERROR 1",    cisConn_param.cis_1st_anchor_tick, \
                                                                            cig_refer_point, \
                                                                            isoInter_past, \
                                                                            cis_ref_future_tick);

            tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] offset min ERROR 2",    blt_debug_hex_2_dec_display(oftMinUs), \
                                                                            blt_debug_hex_2_dec_display(pCisConn->cig_ap_distan_us), \
                                                                            blt_debug_hex_2_dec_display(0), \
                                                                            blt_debug_hex_2_dec_display(0));
    #endif

            BLMS_ERR_DEBUG(DBG_CIS_MASTER_TIMING, 0x99A60000);
        }
        if (oftMinUs < 500) {
            oftMinUs += pCisConn->iso_intvl_us;
        }

        if (oftMinUs > oftMaxUs_spec) {
            tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] offset min ERROR", blt_debug_hex_2_dec_display(oftMinUs), blt_debug_hex_2_dec_display(oftMaxUs_spec), blt_debug_hex_2_dec_display(pCisConn->cis_maxPossible_us), 0);
            BLMS_ERR_DEBUG(DBG_CIS_MASTER_TIMING, 0x99AF0000);
        }


        pCisConn->own_cisOffsetMin_us = oftMinUs;
        pCisConn->own_cisOffsetMax_us = oftMinUs + (pCisConn->offset_us_const ? 0 : pCigMaster->offset_range_us);


    #if (DBG_CIS_MASTER_TIMING)
        if (pCisConn->align_with_acl && pCisConn->link_acl_index == pCigMaster->pcisPos_1st->link_acl_index) {
            tlkapi_send_string_data(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] same ACL, align", 0, 0);
            if (oftMinUs != (pCisConn->cig_ap_distan_us + pCigMaster->pcisPos_1st->cisOffset_use)) {
                tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] erro timing", oftMinUs, pCisConn->cig_ap_distan_us, pCigMaster->pcisPos_1st->cisOffset_use, 0);
                BLMS_ERR_DEBUG(DBG_CIS_MASTER_TIMING, 0x999A0000);
            }
        }
    #endif
    }


    tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] ap_distan_us 1", pCisConn->cism_alloc_position, blt_debug_hex_2_dec_display(pCisConn->cig_ap_distan_us), 0, 0);

    if (pCisConn->nse == 1) { //SubInterval should set to 0 if NSE is 1
        pCtrlCisReq->subIntvl[0] = 0;
        pCtrlCisReq->subIntvl[1] = 0;
        pCtrlCisReq->subIntvl[2] = 0;
    } else {
        smemcpy(pCtrlCisReq->subIntvl, &pCisConn->sub_intvl_us, 3);
    }

    smemcpy(pCtrlCisReq->cisOffsetMin, &pCisConn->own_cisOffsetMin_us, 3);
    smemcpy(pCtrlCisReq->cisOffsetMax, &pCisConn->own_cisOffsetMax_us, 3);
    pCtrlCisReq->connEventCnt = cisConn_param.cis_1st_anchor_evtCnt; //similar to instant


    bool status = blt_llmsPushLlCtrlPkt(pAclConn->acl_conHandle, LL_CIS_REQ, (u8 *)pCtrlCisReq);

    if (status) {                          //always consider that: push TX FIFO may not success
        pCisConn->cisFlowFlg          = CIS_FLOW_MASTER_SEND_CIS_REQ;
        pAclConn->ignore_cis_cmd      = 0; //can process CIS_RSP
        pAclConn->ll_rsp_timeout_tick = clock_time() | 1;

    #if (DBG_CIS_CENTRAL_PARAM)
        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] LL_CIS_Req tx", &pCtrlCisReq->cigId, sizeof(rf_packet_ll_cis_req_t) - 3);
        tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] pdu, nse, sub_int", blt_debug_hex_2_dec_display(pCisConn->max_pdu_loca), blt_debug_hex_2_dec_display(pCisConn->max_pdu_peer), pCisConn->nse, blt_debug_hex_2_dec_display(pCisConn->sub_intvl_us));
        tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] bn ft", pCisConn->bn_loca, pCisConn->bn_peer, pCisConn->ft_loca, pCisConn->ft_peer);

        tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] iso int, OFFSET, acl_evt", blt_debug_hex_2_dec_display(pCigMaster->cism_intvl_us), blt_debug_hex_2_dec_display(pCisConn->own_cisOffsetMin_us), blt_debug_hex_2_dec_display(pCisConn->own_cisOffsetMax_us), blt_debug_hex_2_dec_display(pCtrlCisReq->connEventCnt));
    #endif
    }

    return status;
}

bool blt_ll_sendCisInd(st_ll_conn_t *pAclConn, ll_cis_conn_t *pCisConn, ll_cig_mst_t *pCigMaster)
{
    tlkapi_send_string_u32s(DBG_CISCONN_TRACK_EN, "cis conn 3", pCisConn->cis_index, pCisConn->cis_connHandle, pCisConn, 0);
    tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] LL_CIS_Ind tx", &pAclConn->acl_conHandle, 2);

    pCisConn->cisOffset_use = pCisConn->peer_cisOffsetMin_us;


    //int peerCisOffset_diff = pCisConn->peer_cisOffsetMax_us - pCisConn->peer_cisOffsetMin_us;


    #if 0 //SiHui 20230424 when debug kmlea dongle.  waste RF bandwidth, consider application not only one ACL master + one CIS master
    if(peerCisOffset_diff > 2000){
        pCisConn->cisOffset_use += 1000;
    }
    else if(peerCisOffset_diff< 200){
        pCisConn->cisOffset_use += (peerCisOffset_diff/2);
    }
    else{
        int div_200 = peerCisOffset_diff/200;
        pCisConn->cisOffset_use += (div_200 * 100);
    }
    #endif


    if (pCigMaster->cism_estab_cnt > 0) {
        /* update ap_distan_us here */
        pCisConn->cig_ap_distan_us += (pCisConn->cisOffset_use - pCisConn->own_cisOffsetMin_us);
    }

    //peerCisOffset_diff
    tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] ap_distan_us 2", blt_debug_hex_2_dec_display(0), blt_debug_hex_2_dec_display(pCisConn->cisOffset_use), blt_debug_hex_2_dec_display(pCisConn->cisOffset_use - pCisConn->own_cisOffsetMin_us), blt_debug_hex_2_dec_display(pCisConn->cig_ap_distan_us));


    pCisConn->cis_sync_delay = pCigMaster->cigm_sync_delay - pCisConn->cig_ap_distan_us;


    u8                      temp_buff[sizeof(rf_packet_ll_cis_req_t)];
    rf_packet_ll_cis_ind_t *pCtrlCisInd = (rf_packet_ll_cis_ind_t *)temp_buff;

    pCtrlCisInd->type          = LLID_CONTROL;
    pCtrlCisInd->rf_len        = sizeof(rf_packet_ll_cis_ind_t) - 2;
    pCtrlCisInd->opcode        = LL_CIS_IND;
    pCtrlCisInd->cisAccessAddr = blt_ll_connCalcAccessAddr_v2(); //Access Address of the CIS
    smemcpy(pCtrlCisInd->cisOffset, &pCisConn->cisOffset_use, 3);
    smemcpy(pCtrlCisInd->cigSyncDly, &pCigMaster->cigm_sync_delay, 3);
    smemcpy(pCtrlCisInd->cisSyncDly, &pCisConn->cis_sync_delay, 3);


    u32 tick_now = clock_time();
    /* make sure that first anchor tick is not in current 80mS map */
    #if (SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_80MS)
    u32 tick_safe = tick_now + 85 * SYSTEM_TIMER_TICK_1MS;
    #elif (SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_120MS)
    u32 tick_safe = tick_now + 125 * SYSTEM_TIMER_TICK_1MS;
    #elif (SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_160MS)
    u32 tick_safe = tick_now + 165 * SYSTEM_TIMER_TICK_1MS;
    #elif (SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_240MS)
    u32 tick_safe = tick_now + 245 * SYSTEM_TIMER_TICK_1MS;
    #else
        #error "add code here: margin tick"
    #endif


    #if (DBG_CREATE_FF0D_ERROR)
    u32 r = irq_disable();

    cisConn_param.cis_1st_anchor_evtCnt = pAclConn->conn_inst_mark + 1;
    cisConn_param.cis_1st_anchor_tick   = pAclConn->ap_tick_mark + 1 * pAclConn->conn_intvl_tick;

    irq_restore(r);
    #else
    u32 r = irq_disable();
    int aclEvent_past;
    //pay attention "ap tick mark" maybe a future tick in 100~200uS before tick_now
    if ((u32)(pAclConn->ap_tick_mark - tick_now) < IRQ_BTX_SEND_DELAY_US * SYSTEM_TIMER_TICK_1US) {
        aclEvent_past = 0;
    } else {
        aclEvent_past = (tick_now - pAclConn->ap_tick_mark) / pAclConn->conn_intvl_tick;
    }
    u16 eventCnt_now = pAclConn->conn_inst_mark + aclEvent_past;
    irq_restore(r);


    /* ACL connection event distance less than 5, RF transform chance may not enough consider RF loss
     * anchor point less than 80mS Slot LinkList */
    int entcnt_change = 0;
    u16 eventCnt_past = (u16)(eventCnt_now - cisConn_param.cis_1st_anchor_evtCnt);
    if ((u16)(eventCnt_past + 5) < BIT(14)) {
        entcnt_change = 1;
        tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CIS][TIM] evtcnt change, instant not enough", blt_debug_hex_2_dec_display(eventCnt_now), blt_debug_hex_2_dec_display(cisConn_param.cis_1st_anchor_evtCnt), tick_now, cisConn_param.cis_1st_anchor_tick);
    }
    if (!entcnt_change && tick1_exceed_tick2(tick_safe, cisConn_param.cis_1st_anchor_tick)) {
        entcnt_change = 1;
        tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] evtcnt change, 80ms not enough", blt_debug_hex_2_dec_display(eventCnt_now), blt_debug_hex_2_dec_display(cisConn_param.cis_1st_anchor_evtCnt), tick_now, cisConn_param.cis_1st_anchor_tick);
    }

    if (entcnt_change) { //
        int acl_evt_inc = 0;
        /* the event counter set in LL_CIS_REQ may overtime a lot, due to mainLoop big delay(e.g. 1 second)
         * here we need first find the nearest timing point after clock_time */
        if (eventCnt_past < BIT(14)) { //overtime
            int mod     = eventCnt_past % pCisConn->align_mul_coeff;
            int fill_up = 0;           //for mod is 0
            if (mod) {
                fill_up = pCisConn->align_mul_coeff - mod;
            }

            eventCnt_past += fill_up; //to be integer multiple of "align_mul_coeff"
            acl_evt_inc = eventCnt_past;

            cisConn_param.cis_1st_anchor_evtCnt += eventCnt_past;
            cisConn_param.cis_1st_anchor_tick += eventCnt_past * pAclConn->conn_intvl_tick;
        }


        while ((u16)((eventCnt_now + 5) - cisConn_param.cis_1st_anchor_evtCnt) < BIT(14) ||
               tick1_exceed_tick2(tick_safe, cisConn_param.cis_1st_anchor_tick)) {
            acl_evt_inc += pCisConn->align_mul_coeff;
            cisConn_param.cis_1st_anchor_evtCnt += pCisConn->align_mul_coeff;
            cisConn_param.cis_1st_anchor_tick += pCisConn->align_mul_coeff * pAclConn->conn_intvl_tick;
        }

        tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] evtcnt change", blt_debug_hex_2_dec_display(acl_evt_inc), blt_debug_hex_2_dec_display(cisConn_param.cis_1st_anchor_evtCnt), 0, 0);
    }
    #endif

    pCtrlCisInd->connEventCnt = cisConn_param.cis_1st_anchor_evtCnt;

    #if 0 //debug
        u16 delta_inter = cisConn_param.cis_1st_anchor_evtCnt - eventCnt_now;
        (void)delta_inter; //remove compiler warning
        tlkapi_send_string_u32s(CIS_FLOW_LOG_EN, "[CISC][FLW] ACL instant 2", blt_debug_hex_2_dec_display(eventCnt_now), \
                                                           blt_debug_hex_2_dec_display(cisConn_param.cis_1st_anchor_evtCnt), \
                                                           blt_debug_hex_2_dec_display(delta_inter), 0);
    #endif

    bool status = blt_llmsPushLlCtrlPkt(pAclConn->acl_conHandle, LL_CIS_IND, (u8 *)pCtrlCisInd);

    if (status) {
        pCisConn->createStatus |= CREATE_STATE_SEND_IND;

        cisConn_param.cis_1st_anchor_tick += pCisConn->cisOffset_use * SYSTEM_TIMER_TICK_1US;
        blmsParam.cis_1st_anchor_bSlot           = GET_BSLOT_IDX(cisConn_param.cis_1st_anchor_tick);
        blmsParam.cig_mas_1st_sche_build_pending = CIG_SLOT_BUILD_MSK | pCisConn->cis_index; //Marked value: cis_conn_idx


        /*
         * Transport_Latency_M_To_S = CIG_Sync_Delay + (FT_M_To_S) * ISO_Interval + SDU_Interval_M_To_S
         * Transport_Latency_S_To_M = CIG_Sync_Delay + (FT_S_To_M) * ISO_Interval + SDU_Interval_S_To_M
         */
        if (pCisConn->cis_frame == CIS_UNFRAMED) {
            pCisConn->transLaty_m2s = pCigMaster->cigm_sync_delay + pCisConn->ft_loca * pCigMaster->cism_intvl_us - pCigMaster->sdu_int_loca;
            pCisConn->transLaty_s2m = pCigMaster->cigm_sync_delay + pCisConn->ft_peer * pCigMaster->cism_intvl_us - pCigMaster->sdu_int_peer;
        } else {
            pCisConn->transLaty_m2s = pCigMaster->cigm_sync_delay + pCisConn->ft_loca * pCigMaster->cism_intvl_us + pCigMaster->sdu_int_loca;
            pCisConn->transLaty_s2m = pCigMaster->cigm_sync_delay + pCisConn->ft_peer * pCigMaster->cism_intvl_us + pCigMaster->sdu_int_peer;
        }

        pAclConn->ll_rsp_timeout_tick = 0;
        pCisConn->cisAccessAddr       = pCtrlCisInd->cisAccessAddr;
        blt_cis_connect_common(pAclConn, pCisConn);


    #if 0 //just for debug, must delete later
            static int cis_connect_cnt = 0;
            cis_connect_cnt ++;
            pCisConn->cis_timeout = pAclConn->conn_timeout + cis_connect_cnt * SYSTEM_TIMER_TICK_1S;
    #endif


    #if (DBG_CIS_1ST_AP_TIMING_EN)
        tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] cis ind, OFFSET", blt_debug_hex_2_dec_display(pCisConn->cisOffset_use), blt_debug_hex_2_dec_display(pCigMaster->cigm_sync_delay), blt_debug_hex_2_dec_display(pCisConn->cis_sync_delay), blt_debug_hex_2_dec_display(cisConn_param.cis_1st_anchor_evtCnt));
    #endif
    }

    return status;
}

bool blt_ll_rejectCisRsp(ll_cis_conn_t *pCisConn, ll_cig_mst_t *pCigMaster)
{
    if (!pCisConn->cis_reject_reason) {
        BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99A10000);
    }

    st_ll_conn_t *pAclConn = (st_ll_conn_t *)&blms[pCisConn->link_acl_index];

    //if push data error, should pending
    bool status = blt_llms_rejectInd(pCisConn->link_acl_handle, LL_CIS_RSP, pCisConn->cis_reject_reason, 1);
    if (status) {
        blt_ll_cis_master_cis_establish(pCisConn, pCigMaster, pCisConn->cis_reject_reason);

        tlkapi_send_string_u32s(CIS_FLOW_LOG_EN, "[CISC][FLW] LL_Reject_CIS_Rsp", pCisConn->cis_reject_reason, status, 0, 0);

        pAclConn->ll_rsp_timeout_tick = 0;
        pCisConn->cis_reject_reason   = 0;
    }

    return status;
}

ble_sts_t blt_ll_cis_master_control_pdu_process(st_ll_conn_t *pAclConn, u8 opcode, u8 *pLlCtrlPkt)
{
    ll_cis_conn_t *pCisConn;

    if (opcode == LL_CIS_RSP) //only master receive this opcode
    {
        /* createCis -> createCisCancel(disconnect command when CIS not established)
         * HCI/CIS/BV-02-C test this logic */
        if (pAclConn->ignore_cis_cmd) {
            return HCI_ERR_OP_CANCELLED_BY_HOST;
        }

        pAclConn->ll_rsp_timeout_tick = clock_time() | 1;

        if (bltCisMng.cisFlow_idx == INVALID_CIS_IDX) {
            BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99990000);
        }
        pCisConn = (ll_cis_conn_t *)(global_pCisConn + bltCisMng.cisFlow_idx);
        tlkapi_send_string_u32s(DBG_CISCONN_TRACK_EN, "cis conn 2", pCisConn->cis_index, pCisConn->cis_connHandle, pCisConn, 0);

        rf_packet_ll_cis_rsp_t *pCisRsp = (rf_packet_ll_cis_rsp_t *)pLlCtrlPkt;

        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] LL_CIS_RSP rx", &pCisRsp->cisOffsetMin, 8);


        pCisConn->peer_cisOffsetMin_us = MAKE_U24(pCisRsp->cisOffsetMin[2], pCisRsp->cisOffsetMin[1], pCisRsp->cisOffsetMin[0]);
        pCisConn->peer_cisOffsetMax_us = MAKE_U24(pCisRsp->cisOffsetMax[2], pCisRsp->cisOffsetMax[1], pCisRsp->cisOffsetMax[0]);


        tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] cis rsp OFFSET", blt_debug_hex_2_dec_display(pCisConn->peer_cisOffsetMin_us), blt_debug_hex_2_dec_display(pCisConn->peer_cisOffsetMax_us), blt_debug_hex_2_dec_display(pCisRsp->connEventCnt), pCisConn->align_with_acl);

        int peer_error = 0;

    #if 1 //CIS_CEN_BV-50
        int acl_diff = pCisRsp->connEventCnt - cisConn_param.cis_1st_anchor_evtCnt;
        if (acl_diff) {
            acl_diff = acl_diff > 0 ? acl_diff : -acl_diff;
            if ((acl_diff % pCisConn->align_mul_coeff) != 0) {
                peer_error = 1;
                tlkapi_send_string_data(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] ERROR, ACL evt_count not align", 0, 0);
            }
        }

        if (!peer_error) {
            u32 peer_oftMin_us = (int)pCisConn->peer_cisOffsetMin_us;
            u32 peer_oftMax_us = (int)pCisConn->peer_cisOffsetMax_us;
            if (peer_oftMin_us > peer_oftMax_us) {
                peer_error = 1;
                tlkapi_send_string_data(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] ERROR, offset min bigger than max", 0, 0);
            } else {
        #if 1
                if (peer_oftMin_us < pCisConn->own_cisOffsetMin_us) {
                    u32 jump = (pCisConn->own_cisOffsetMin_us - peer_oftMin_us + pCisConn->iso_intvl_us - 1) / pCisConn->iso_intvl_us;
                    u32 inc  = jump * pCisConn->iso_intvl_us;
                    peer_oftMin_us += inc;
                    peer_oftMax_us += inc;
                    pCisConn->peer_cisOffsetMin_us = peer_oftMin_us;
                    pCisConn->peer_cisOffsetMax_us = peer_oftMax_us;
                } else if (peer_oftMin_us >= (pCisConn->own_cisOffsetMin_us + pCisConn->iso_intvl_us)) {
                    u32 jump = (peer_oftMin_us - pCisConn->own_cisOffsetMin_us) / pCisConn->iso_intvl_us;
                    u32 dec  = jump * pCisConn->iso_intvl_us;
                    peer_oftMin_us -= dec;
                    peer_oftMax_us -= dec;
                    pCisConn->peer_cisOffsetMin_us = peer_oftMin_us;
                    pCisConn->peer_cisOffsetMax_us = peer_oftMax_us;
                }
        #else
                if (peer_oftMin_us > pCisConn->iso_intvl_us) {
                    tlkapi_send_string_data(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] try to adjust", 0, 0);
                    //int div = peer_oftMin_us/pCisConn->iso_intvl_us;
                    peer_oftMin_us = peer_oftMin_us % pCisConn->iso_intvl_us;
                    peer_oftMax_us = peer_oftMax_us % pCisConn->iso_intvl_us;
                    tlkapi_send_string_u32s(DBG_CIS_CENTRAL_PARAM, "[CISC][PAR] offset after adjust", blt_debug_hex_2_dec_display(peer_oftMin_us), blt_debug_hex_2_dec_display(peer_oftMax_us), 0, 0);

                    pCisConn->peer_cisOffsetMin_us = peer_oftMin_us;
                    pCisConn->peer_cisOffsetMax_us = peer_oftMax_us;
                }
        #endif
            }
        }
    #endif

        if (peer_error ||
            pCisConn->peer_cisOffsetMin_us < pCisConn->own_cisOffsetMin_us ||
            pCisConn->peer_cisOffsetMax_us > pCisConn->own_cisOffsetMax_us ||
            pCisConn->peer_cisOffsetMin_us > pCisConn->peer_cisOffsetMax_us) {
            tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] ERROR, cis offset not in range", 0, 0);
            /* attention: do not return here
             * if return none zero value, will use "public reject process" in acl_conn.c
             * set CIS_FLOW_MASTER_REJECT_CIS_RSP here, will call "blt_ll_rejectCisRsp" later to send reject */
            pCisConn->cis_reject_reason = HCI_ERR_CONN_REJ_LIMITED_RESOURCES; //TODO: 0x0D maybe not right
            pCisConn->cisFlowFlg        = CIS_FLOW_MASTER_REJECT_CIS_RSP;
        } else {
            pCisConn->cisFlowFlg = CIS_FLOW_MASTER_SEND_CIS_IND;
        }
    } else if (opcode == LL_REJECT_IND_EXT) {
        rf_packet_ll_reject_ext_ind_t *pRejectExtInd = (rf_packet_ll_reject_ext_ind_t *)pLlCtrlPkt;

        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] LL_REJECT_EXT rx", &pRejectExtInd->errCode, 1);

        //TODO:If either Link Layer sends or receives an LL_REJECT_EXT_IND PDU, it shall terminate the procedure immediately
        //     and not create the CIS Each Link Layer shall notify its host when the procedure is completed.

        if (pRejectExtInd->rejectOpcode == LL_CIS_REQ) { //master can receive

            if (bltCisMng.cisFlow_idx == INVALID_CIS_IDX) {
                BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x999B0000);
            }
            pCisConn = (ll_cis_conn_t *)(global_pCisConn + bltCisMng.cisFlow_idx);

            if (pCisConn->cisFlowFlg == CIS_FLOW_MASTER_SEND_CIS_REQ) {
                pAclConn->ll_rsp_timeout_tick = 0;
                ll_cig_mst_t *pCigMaster      = (ll_cig_mst_t *)(global_pCigMst + pCisConn->clink_cig_idx);
                blt_ll_cis_master_cis_establish(pCisConn, pCigMaster, pRejectExtInd->errCode);
            }
        }
    } else if (opcode == LL_CIS_TERMINATE_IND) {
        rf_packet_ll_cis_terminate_t *pTermInd = (rf_packet_ll_cis_terminate_t *)pLlCtrlPkt;

        for (int i = 0; i < bltCisMng.maxNum_cisMaster; i++) {
            if (pAclConn->cisEstablish_msk & BIT(i)) { //If multiple CISes are bound to the same ACL

                pCisConn = (ll_cis_conn_t *)(global_pCisConn + i);

                if (pCisConn->cis_ID == pTermInd->cis_id && pCisConn->link_cigid == pTermInd->cig_id) {
                    pCisConn->cis_termin_union.local_terminate = pTermInd->errorCode;
                }
            }
        }

        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISC][FLW] LL_CIS_TERMINATE_IND rx", &pTermInd->errorCode, 1);
    }


    return BLE_SUCCESS;
}


    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
//though CIS master sending data no need consider 150uS(TX first), but I will use one binary to test CIS slave and CIS master
//when CIS slave is sending data, this main_loop is running.
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    int
    blt_ll_cigMstMainloop(void)
{
    //CIS master pending condition:
    //1. wait to send LL_CIS_REQ
    //2. wait to send LL_CIS_IND
    //3. wait to send LL_REJCT_IND_EXT for LL_CIS_RSP

    ll_cis_conn_t *pCisConn;
    for (int cis_conn_idx = 0; cis_conn_idx < bltCisMng.maxNum_cisMaster; cis_conn_idx++) {
        pCisConn = (ll_cis_conn_t *)(global_pCisConn + cis_conn_idx);


        if (pCisConn->cisFlowFlg) {
            //TODO: check pAclConn->connState, for FIX_CIS_CREATE_CMD_ERR_DUE_TO_PREVIOUS_CREATE_NOT_CLEAR_WHEN_ACL_TERMINATE
            st_ll_conn_t *pAclConn   = (st_ll_conn_t *)&blms[pCisConn->link_acl_index];
            ll_cig_mst_t *pCigMaster = (ll_cig_mst_t *)(global_pCigMst + pCisConn->clink_cig_idx);

            if (pCisConn->cisFlowFlg == CIS_FLOW_MASTER_START_NEW_CIS) {
    #if 1
                /* LL/CIS/CEN/BV-56-C */
                if (pAclConn->llcp_flag.bit.ll_feat_exg_flag) {
                    if (pAclConn->ll_remoteFeature0 & LL_FEATURE_MASK_CONNECTED_ISOCHRONOUS_STREAM_SLAVE) {
                        blt_ll_sendCisReq(pAclConn, pCisConn, pCigMaster);
                    } else {
                        //todo: can not create, send a event to notify host ?
                        bltCisMng.cisFlow_pending &= ~BIT(pCisConn->cis_index);
                        bltCisMng.cisFlow_idx = INVALID_CIS_IDX;
                        pCisConn->cisFlowFlg  = CIS_FLOW_IDLE;

                        blt_ll_cism_check_other_cis_create(pCisConn, pCigMaster);
                    }
                } else if (!pAclConn->remoteFeatureReq) {
                    blt_ll_send_feature_req(pAclConn);
                }
    #else
                blt_ll_sendCisReq(pAclConn, pCisConn, pCigMaster);
    #endif
            } else if (pCisConn->cisFlowFlg == CIS_FLOW_MASTER_SEND_CIS_IND) {
                blt_ll_sendCisInd(pAclConn, pCisConn, pCigMaster);
            } else if (pCisConn->cisFlowFlg == CIS_FLOW_MASTER_REJECT_CIS_RSP) {
                blt_ll_rejectCisRsp(pCisConn, pCigMaster);
            } else if (pCisConn->cisFlowFlg == CIS_FLOW_CIS_SYNC_SUCCESS) {
                //tlkapi_send_string_u32s(0, "cis conn 9", pCisConn->cis_index, pCisConn->cis_connHandle, pCisConn, 0);
                blt_ll_cis_master_cis_establish(pCisConn, pCigMaster, BLE_SUCCESS);
            } else if (pCisConn->cisFlowFlg == CIS_FLOW_CIS_SYNC_FAIL) {
                blt_ll_cis_master_cis_establish(pCisConn, pCigMaster, HCI_ERR_CONN_FAILED_TO_ESTABLISH);
            } else if (pCisConn->cisFlowFlg == CIS_FLOW_CLEAR_ESTABLISH_STATUS) {
                blt_ll_cism_clear_establish_status(pCisConn, pCigMaster);
            }
        }
    }


    //tlkapi_send_string_u32s(DBG_CISCONN_TRACK_EN, "cis conn", pCisConn->cis_index, pCisConn->cis_connHandle, pCisConn, 0);


    return 1;
}

_attribute_ram_code_ static int blt_ll_find_next_cis_mst_subevent(int start_idx)
{
    int ret = 100; //important

    for (int i = start_idx; i < blt_pCigMst->cism_total_se_num; i++) {
        if (blt_pCigMst->cism_arrgmtMap_en_msk & BIT(i)) {
            u8             cur_cis_sel = blt_pCigMst->cism_arrgmtMap_cisIdx[i];
            ll_cis_conn_t *pCisConn    = (ll_cis_conn_t *)(global_pCisConn + cur_cis_sel);


            if (pCisConn->cisSchedFlg) {
    #if (CIS_ADD_CIE)
                if (pCisConn->cie_flag) { //peer_cie && pCisConn->local_cie
                    continue;
                }
    #endif

                cisConn_param.blt_cis_sel          = cur_cis_sel;
                blt_pCisConn                       = pCisConn;
                blt_pCigMst->cigm_se_idx           = blt_pCigMst->cism_arrgmtMap_seIdx[i];
                blt_pCigMst->cism_map_next_taskIdx = i + 1;

                return i;
            }
        }
    }

    return ret;
}

_attribute_ram_code_ int blt_cig_mst_start(int slotTask_idx)
{
    DBG_SIHUI_CHN8_HIGH;
    DBG_FANQH_CHN8_HIGH;
    #if (SL01_cis_group)
    log_task_begin_irq(SL_STACK_CIS_BASIC_TIMING_EN, SL01_cis_group);
    #endif


    //1.First locate the CIG that belongs to
    cisMas_param.blt_cigMstSel = slotTask_idx;
    blt_pCigMst                = (ll_cig_mst_t *)(global_pCigMst + cisMas_param.blt_cigMstSel);


    if (blt_pCigMst->first_ap_update) {
        u32 tx_trigger_tick               = blt_pCigMst->cigm_expect_tick - CIS_TX_TRIGGER_TO_PKT_IN_AIR_DISTANCE_US * SYSTEM_TIMER_TICK_1US;
        blt_pCigMst->tx_trigger_diff_tick = tx_trigger_tick - bltSche.sSlot_tick_irq;


        if (blt_pCigMst->tx_trigger_diff_tick > (IRQ_CTX_DELAY_US + 20 + 2) * SYSTEM_TIMER_TICK_1US ||
            blt_pCigMst->tx_trigger_diff_tick < (IRQ_CTX_DELAY_US - 2) * SYSTEM_TIMER_TICK_1US) {
            write_dbg32(0x0018, blt_pCigMst->cigm_expect_tick);
            write_dbg32(0x001C, bltSche.sSlot_tick_irq);
            tlkapi_send_string_u32s(DBG_CIS_MASTER_IRQ_TIMING_DUMPLOG, "tx trigger tick err", blt_pCigMst->cigm_expect_tick, bltSche.sSlot_tick_irq, blt_pCigMst->tx_trigger_diff_tick, 0);
            BLMS_ERR_DEBUG(DBG_CIS_MASTER_TIMING, 0x99AA0000);
        }
    }

    /* update every time, to make sure timing correct. If not do this, use first time set value by "first_ap_update", timing may become
     * unstable, if with low power it will be worse */
    blt_pCigMst->cigm_trigger_tick = bltSche.sSlot_tick_irq + blt_pCigMst->tx_trigger_diff_tick -
                                     blt_pCigMst->pcisPos_earliest->cig_ap_distan_us * SYSTEM_TIMER_TICK_1US;
    blt_pCigMst->cig_ref_point = blt_pCigMst->cigm_trigger_tick + CIS_TX_TRIGGER_TO_PKT_IN_AIR_DISTANCE_US * SYSTEM_TIMER_TICK_1US;

    #if (DBG_CIS_1ST_AP_TIMING_EN)
    if (blt_pCigMst->first_ap_update) {
        tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] first ap update", bltSche.sSlot_tick_irq, blt_pCigMst->cigm_expect_tick, blt_pCigMst->tx_trigger_diff_tick, blt_pCigMst->cigm_trigger_tick);
    }
    #endif


    #if 1
    int cigm_jump_num = (bltSche.bSlot_idx_irq_real + blt_pCigMst->bSlot_ap_offset + 4 - blt_pCigMst->bSlot_mark_cigm) / blt_pCigMst->cism_bSlotInterval - 1;
    if (blt_pCigMst->bSlot_ap_offset) {
        tlkapi_send_string_u8s(0, "BSLOT_ap_offset", blt_pCigMst->bSlot_ap_offset, 0, 0, 0);
    }
    blt_pCigMst->bSlot_ap_offset = 0;
    #elif 1 //for multiple CIS dynamic change, anchor point between 2 ISO event may less than one ISO interval, fix 20220826 CIS/CEN/BV-51
    int cigm_jump_num = (bltSche.bSlot_idx_irq_real + 4 - blt_pCigMst->bSlot_mark_cigm) / blt_pCigMst->cism_bSlotInterval;
    if (cigm_jump_num) {
        cigm_jump_num -= 1;
    }
    #else
    int cigm_jump_num = (bltSche.bSlot_idx_irq_real + 4 - blt_pCigMst->bSlot_mark_cigm) / blt_pCigMst->cism_bSlotInterval - 1;
    #endif


    if (cigm_jump_num) {
        if (blt_pCigMst->first_ap_update != TYPE_ADD_CIS) {
            blt_pCigMst->cigm_expect_tick += (cigm_jump_num * blt_pCigMst->cism_intvl_us * SYSTEM_TIMER_TICK_1US);
        }
        tlkapi_send_string_u32s(DBG_CIS_MASTER_IRQ_TIMING_DUMPLOG, "CIS jump", cigm_jump_num, 0, 0, 0);
    }

    if (tick1_out_range_of_tick2(blt_pCigMst->cigm_expect_tick, bltSche.sSlot_tick_irq, 500 * SYSTEM_TIMER_TICK_1US)) {
        tlkapi_send_string_u32s(DBG_CIS_MASTER_IRQ_TIMING_DUMPLOG, "OUT RANGE", blt_pCigMst->cigm_expect_tick, bltSche.sSlot_tick_irq, blt_pCigMst->first_ap_update, 0);
        BLMS_ERR_DEBUG(DBG_CIS_MASTER_TIMING, 0x99930000);
    }


    for (int i = 0; i < bltCisMng.maxNum_cisMaster; i++) {
        if (blt_pCigMst->cism_task_msk & BIT(i)) {
            ll_cis_conn_t *pCisConn  = (ll_cis_conn_t *)(global_pCisConn + i);
            pCisConn->cisSubEventCnt = 0;                //reset current SE index
            pCisConn->cig_next_tick  = clock_time() | 1; //todo: IAL used this, should optimize, SiHui
    #if (CIS_ADD_CIE)
            pCisConn->local_cie = 0;
            pCisConn->peer_cie  = 0;
            pCisConn->cie_flag  = 0;
    #endif

            int cis_task_trigger = 0;
            if (pCisConn->cis1stSchedAPbSlot && !pCisConn->cisSchedFlg) {
                tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] ap1", bltSche.bSlot_idx_irq_real, pCisConn->cis1stSchedAPbSlot, 0, 0);
                //BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x998C0000);

                u32 schTaskEndbSlot = bltSche.bSlot_idx_irq_real + (blt_pCigMst->cism_sSlotDuration >> 5) + 1;
                int bSlot_diff      = (int)(schTaskEndbSlot - pCisConn->cis1stSchedAPbSlot);
                if (bSlot_diff > 0) {
                    tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] ap2", bltSche.bSlot_idx_irq_real, blt_pCigMst->cism_sSlotDuration, schTaskEndbSlot, pCisConn->cis1stSchedAPbSlot);
                    //BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x998C0000);

                    cis_task_trigger      = 1;
                    pCisConn->cisSchedFlg = 1;
                    pCisConn->cis_tick    = clock_time();

    #if (WALKAROUND_ISO_TIMESTAMP_EN)
                    pCisConn->cis_ap_tick = 200;
    #endif

                    pCisConn->cis_jump_num = bSlot_diff / blt_pCigMst->cism_bSlotInterval;

                    if (pCisConn->cis_jump_num > 10) {
                        write_dbg32(0x0018, schTaskEndbSlot);
                        write_dbg32(0x001C, pCisConn->cis1stSchedAPbSlot);
                        BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99A30000 | pCisConn->cis_jump_num);
                    }

                    pCisConn->cis1stSchedAPbSlot = 0;
                }
            }


            if (!cis_task_trigger) {
                pCisConn->cis_jump_num = cigm_jump_num;
                if (pCisConn->cis_jump_num > 10) {
                    BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99830000 | pCisConn->cis_jump_num);
                }
            }
            if (pCisConn->cis_jump_num) {
                //DBG_C HN0_TOGGLE;
                blt_ll_cis_ft_event_jump(pCisConn, pCisConn->cis_jump_num);
            }


            if (pCisConn->cisSchedFlg) {
                pCisConn->cis_trigger_tick = blt_pCigMst->cigm_trigger_tick + pCisConn->cig_ap_distan_us * SYSTEM_TIMER_TICK_1US;
                pCisConn->cis_expect_tick  = pCisConn->cis_trigger_tick + CIS_TX_TRIGGER_TO_PKT_IN_AIR_DISTANCE_US * SYSTEM_TIMER_TICK_1US;
            }
        }
    }


    blt_pCigMst->sSlot_mark_cigm = bltSche.sSlot_idx_irq_real;
    blt_pCigMst->bSlot_mark_cigm = bltSche.bSlot_idx_irq_real;


    u8 se_idx = blt_ll_find_next_cis_mst_subevent(0);
    //tlkapi_send_string_u32s(DBG_CISCONN_TRACK_EN, "cis conn 5", blt_pCisConn->cis_index, blt_pCisConn->cis_connHandle, blt_pCisConn, 0);


    if (se_idx == 100) {
        tlkapi_send_string_data(DBG_CIS_MASTER_IRQ_LOGIC_DUMPLOG, "Big start task, se_dix == 100: impossible!!!", 0, 0);

        write_dbg32(0x0018, blt_pCigMst->cism_arrgmtMap_en_msk);
        write_dbg32(0x001C, *(u32 *)(&blt_pCigMst->cism_arrgmtMap_cisIdx[0]));
        BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x999D0000);
    } else {
        if (blt_pCigMst->cigm_se_idx != 1) {
            BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99A40000 | blt_pCigMst->cigm_se_idx);
        }

        cisMas_param.ctx_trigger_tick = blt_pCisConn->cis_trigger_tick;
        if (se_idx == 0) {
            u32 tick_margin = clock_time() + 10 * SYSTEM_TIMER_TICK_1US;

            /* this debug can determine if "IRQ_CTX_DELAY_US" is enough for CIS master code running
             * after sys_timer IRQ and before first CTX trigger */
            if (tick1_exceed_tick2(tick_margin, cisMas_param.ctx_trigger_tick)) {
                tlkapi_send_string_u32s(DBG_CIS_MASTER_IRQ_TIMING_DUMPLOG, "timing err 1", tick_margin, cisMas_param.ctx_trigger_tick, blt_pCigMst->cigm_expect_tick, bltSche.sSlot_tick_irq);
                BLMS_ERR_DEBUG(DBG_CIS_MASTER_TIMING, 0x99AB0000);
            }
            blt_ctx_start();
        } else {
            u32 ctx_t = cisMas_param.ctx_trigger_tick - 30 * SYSTEM_TIMER_TICK_1US;
            if (!tick1_exceed_tick2(ctx_t, clock_time())) {
                BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99870000);
            }
            systimer_set_irq_capture(ctx_t);
            systick_irq_trigger = SYS_IRQ_TRIG_CTX_START;
        }
    }


    blt_pCigMst->cis_delete      = 0;
    blt_pCigMst->cigm_finish     = 0;
    blt_pCigMst->first_ap_update = 0;


    return 1;
}

_attribute_ram_code_ void blt_cig_mst_post(void)
{
    blms_state = BLMS_STATE_CIG_E;

    if (!blt_pCigMst->cigm_finish) {
        blt_pCigMst->cigm_expect_tick += blt_pCigMst->cism_intvl_us * SYSTEM_TIMER_TICK_1US;

        if (blt_pCigMst->cis_delete) { //at least one CIS disconnect, but other CIS still work
            /* now only process CIS slave, so one CIS slave is left */
            if (blt_cig_mst_calculate_timing(blt_pCigMst, TYPE_DELETE_CIS)) {
                blt_sche_addUpdate(SLOT_UPDT_CIS_MASTER_CHANGE);
            }
        }


        for (int i = 0; i < bltCisMng.maxNum_cisMaster; i++) {
            if (blt_pCigMst->cism_task_msk & BIT(i)) {
                ll_cis_conn_t *pCis = (ll_cis_conn_t *)(global_pCisConn + i);
                if (pCis->cisSchedFlg) {
    #if (CIS_ADD_CIE)
                    blt_ll_cis_ft_subevent_commm(pCis, pCis->nse - pCis->cisSubEventCnt);
    #endif
                    pCis->cisEventCnt++;
                }
            }
        }
    }


    for (int i = 0; i < bltCisMng.maxNum_cisMaster; i++) {
        if (blt_pCigMst->cism_task_msk & BIT(i)) {
            ll_cis_conn_t *pCis = (ll_cis_conn_t *)(global_pCisConn + i);
            if (pCis->pCisTestParam && (pCis->pCisTestParam->isoTestMode == ISO_TEST_TRANSMIT_MODE) && (!pCis->pCisTestParam->tranMode.isoTestSendTick)) //&&
            {
                pCis->pCisTestParam->tranMode.isoTestSendTick = clock_time() | 1;
            }
        }
    }

    blt_ll_calculate_sSlot_next(clock_time() + (bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US) * SYSTEM_TIMER_TICK_1US);

    DBG_SIHUI_CHN8_LOW;
    DBG_FANQH_CHN8_LOW;
    #if (SL01_cis_group)
    log_task_end_irq(SL_STACK_CIS_BASIC_TIMING_EN, SL01_cis_group);
    #endif
}

_attribute_ram_code_ int blt_ctx_start(void)
{
    if (cisConn_param.blt_cis_sel == 0) {
        DBG_SIHUI_CHN9_HIGH;
        DBG_FANQH_CHN9_HIGH;
    } else if (cisConn_param.blt_cis_sel == 1) {
        DBG_SIHUI_CHN10_HIGH;
        DBG_FANQH_CHN10_HIGH;
    }

    #if (SL01_cis0)
    log_task_begin_irq(SL_STACK_CIS_BASIC_TIMING_EN, SL01_cis0 + cisConn_param.blt_cis_sel);
    #endif

    STOP_RF_STATE_MACHINE; //make sure state machine is clean

    /* SiHui test 20220720: 48M clock, subEvent 1: 14 uS;  subEvent 2: 24 uS; other subEvent: 11 uS*/
    //DBG_C HN9_HIGH;
    blt_ll_cis_start_common_1(blt_pCisConn);
    //DBG_C HN9_LOW;


    rf_start_fsm(FSM_TX2RX, NULL, cisMas_param.ctx_trigger_tick);

    /* attention that tx2rx mode do not have first RX timeout */
    rf_ble_set_rx_timeout(bltPHYs.prmb_ac_us + 150 + 20); //leave 20 uS margin

    rf_ble_set_tx_settle(tx_stl_btx_1st_pkt[blt_pCisConn->curCisPhy]);

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_TX_ON);
    }

    //these logic setting executing after CTX setting to save time
    blt_ll_cis_start_common_2(blt_pCisConn);

    blms_state = BLMS_STATE_CTX_S;

    //system trigger point: consider that RX IRQ must processed
    systick_irq_trigger = SYS_IRQ_TRIG_CTX_POST;


    /* old code (before 20230324) use 20uS for RX check margin after peer packet send over
     * It's risky because we did not consider that RX IRQ delay(about 20uS).
     * So we meet problem when test LL/CIS/CEN/BV-18-C LL/CIS/CEN/BV-29-C : 48M clock OK, 96M sometimes Fail
     * reason is that ctx_post stimer IRQ trigger, RX IRQ not come; but RX IRQ come when software is running
     * different clock running time is not same, 96M is fast, execute RX IRQ check before RX IRQ come. */
    u8 rx_check_margin;
    #if ((MCU_CORE_TYPE == MCU_CORE_B91) || (MCU_CORE_TYPE == MCU_CORE_B92))
    if (sys_clk.cclk == 96) {
        rx_check_margin = 30;
    } else if (sys_clk.cclk == 64) {
        rx_check_margin = 25;
    } else {
        rx_check_margin = 20;
    }
    #else
        #error "add rx check margin for other MCU !!!"
    #endif

    systimer_set_irq_capture(blt_pCisConn->cis_expect_tick + (blt_pCisConn->MPTM_TIFS_MPTS + rx_check_margin) * SYSTEM_TIMER_TICK_1US);


    #if 0
    u32 tx_send_tick = cisMas_param.ctx_trigger_tick + tx_stl_btx_1st_pkt[blt_pCisConn->curCisPhy]*SYSTEM_TIMER_TICK_1US;
    DBG_CHN_HIGH;
    while(tick1_exceed_tick2(tx_send_tick, clock_time()));
    DBG_CHN_LOW;
    #endif

    return 1;
}

_attribute_ram_code_ int blt_ctx_post(ll_cis_conn_t *pCisConn)
{
    blms_state      = BLMS_STATE_CTX_E;
    u8 cur_cis_mark = cisConn_param.blt_cis_sel;

    int cig_end = 0;

    #if (SL01_cis0)
    log_task_end_irq(SL_STACK_CIS_BASIC_TIMING_EN, SL01_cis0 + cisConn_param.blt_cis_sel);
    #endif

    int cis_status = blt_ll_cis_post_common(pCisConn);

    if (cis_status != BLE_SUCCESS) {
        blt_pCisConn->cisSchedFlg = 0;
        blt_pCigMst->cis_delete   = 1;

        u8 cis_sel = blt_pCisConn->cis_index;

        if (cis_status != HCI_ERR_CONN_FAILED_TO_ESTABLISH) {
            blt_pCisConn->cisFlowFlg = CIS_FLOW_CLEAR_ESTABLISH_STATUS;
        }

        if (blt_pCigMst->cis_alloc_exist[blt_pCisConn->cism_alloc_position] == 0) {
            BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99A50000);
        }
        blt_pCigMst->cis_alloc_exist[blt_pCisConn->cism_alloc_position] = 0;
        blt_pCigMst->cism_task_msk &= ~BIT(cis_sel);
        blt_pCigMst->cism_task_cnt--;
        if (blt_pCigMst->cism_task_cnt == 0) {
            for (int i = 0; i < CIS_IN_CIGM_NUM_MAX; i++) {
                blt_pCigMst->cis_alloc_markIdx[i] = CIS_ID_INVALID;
            }

            blt_pCigMst->pcisPos_earliest = blt_pCigMst->pcisPos_latest = NULL;

            blt_sche_addUpdate(SLOT_UPDT_CIS_MASTER_REMOVE);
            blt_sche_removeTaskMask(TSKMSK_CIG_MASTER_0 << cisMas_param.blt_cigMstSel);

            blt_pCigMst->cigm_finish = 1;
            cig_end                  = 1;
        }


        tlkapi_send_string_u32s(DBG_CIS_MASTER_IRQ_LOGIC_DUMPLOG, "[CISC][TIM] delete CIS", cis_sel, blt_pCigMst->cism_task_cnt, 0, 0);
    } else {
        pCisConn->cis_expect_tick += pCisConn->sub_intvl_tick; //todo: move to common
        pCisConn->cis_trigger_tick += pCisConn->sub_intvl_tick;
        blt_cis_post_common_2(pCisConn);
    }


    if (!cig_end) {
        int next_nse_idx = blt_ll_find_next_cis_mst_subevent(blt_pCigMst->cism_map_next_taskIdx);

        if (next_nse_idx < blt_pCigMst->cism_total_se_num) {
            /* debug code, remove later */
            u32 anchor_t = blt_pCigMst->cig_ref_point + (blt_pCisConn->cig_ap_distan_us + blt_pCisConn->sub_intvl_us * (blt_pCigMst->cigm_se_idx - 1)) * SYSTEM_TIMER_TICK_1US;
            if (tick1_out_range_of_tick2(blt_pCisConn->cis_expect_tick, anchor_t, 50 * SYSTEM_TIMER_TICK_1US)) {
                write_dbg32(0x0018, blt_pCisConn->cis_expect_tick);
                write_dbg32(0x001C, anchor_t);
                BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99180000);
            }


            //SiHui optimize: if timing very close, call ctx_start directly, not use sys_timer IRQ

            u32 tick_now = clock_time(); // + 10*SYSTEM_TIMER_TICK_1US;
            /* SiHui test 20220720: 48M clock, for ubEvent 2, calculate channel map 24uS,
             * sys_timer IRQ to "rf_start_fsm", GPIO pulse 30uS, consider IRQ hardware enter cost some more uS,
             * see timing error 3~5 uS when margin set 30uS.  these data is reasonable
             * here we use 50uS for more safety */
            u32 ctx_t = blt_pCisConn->cis_trigger_tick - 50 * SYSTEM_TIMER_TICK_1US;

    #if (CIS_T_MSS == 250)
            if (tick1_exceed_tick2(tick_now, ctx_t)) //time not enough
    #elif (FAST_SETTLE)
            if (tick1_exceed_tick2(tick_now - 10 * SYSTEM_TIMER_TICK_1US, ctx_t)) //time not enough
    #else
        #error "add check code for CIS_T_MSS"
    #endif
            {
                write_dbg32(0x0018, ctx_t);
                write_dbg32(0x001C, tick_now);
                BLMS_ERR_DEBUG(DBG_CIS_TIMING, 0x991F0000);
            }

            systimer_set_irq_capture(ctx_t);
            systick_irq_trigger           = SYS_IRQ_TRIG_CTX_START;
            cisMas_param.ctx_trigger_tick = blt_pCisConn->cis_trigger_tick;
        } else {
            cig_end = 1;
        }
    }

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_OFF);
    }

    if (cur_cis_mark == 0) {
        DBG_SIHUI_CHN9_LOW;
        DBG_FANQH_CHN9_LOW;
    } else if (cur_cis_mark == 1) {
        DBG_SIHUI_CHN10_LOW;
        DBG_FANQH_CHN10_LOW;
    }


    if (cig_end) {
        blt_cig_mst_post();
    }


    return 1;
}

_attribute_ram_code_ int blt_cig_mst_calculate_timing(ll_cig_mst_t *pCigMaster, int type)
{
    int ret_result                    = CIG_TASK_NO_CHANGE;
    pCigMaster->cism_total_se_num     = 0;
    pCigMaster->cism_arrgmtMap_en_msk = 0;
    ll_cis_conn_t *pCurCisCon;
    ll_cis_conn_t *pcisCur_earliest = NULL;
    ll_cis_conn_t *pcisCur_latest   = NULL;

    u8  cur_cisIndex, task_idx;
    int cis_exist_cnt = 0;
    for (int i = 0; i < pCigMaster->cism_set_cnt; i++) { //attention: cism_set_cnt but not cism_task_cnt !!!
        if (pCigMaster->cis_alloc_exist[i]) {
            cur_cisIndex = pCigMaster->cis_alloc_markIdx[i];
            pCurCisCon   = (ll_cis_conn_t *)(global_pCisConn + cur_cisIndex);

            for (int j = 0; j < pCurCisCon->nse; j++) {            //attention:  Only consider NSE same for all CISes
                if (pCigMaster->cism_packing == PACK_SEQUENTIAL) { //Sequential: e.g.: 112233
                    task_idx = pCigMaster->cism_total_se_num + j;
                } else {                                           //== PACK_INTERLEAVED  //Interleaved: e.g.: 123123
                    task_idx = cis_exist_cnt + j * pCigMaster->cism_task_cnt;
                }

                pCigMaster->cism_arrgmtMap_cisIdx[task_idx] = cur_cisIndex;
                pCigMaster->cism_arrgmtMap_seIdx[task_idx]  = j + 1;
                pCigMaster->cism_arrgmtMap_en_msk |= BIT(task_idx);
                tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISC][TIM] add map", task_idx, cur_cisIndex, j + 1, pCigMaster->cism_arrgmtMap_en_msk);
            }


            cis_exist_cnt++;
            pCigMaster->cism_total_se_num += pCurCisCon->nse;

            /* find earliest and latest CIS allocate position */
            if (!pcisCur_earliest) {
                pcisCur_earliest = pCurCisCon;
            }
            pcisCur_latest = pCurCisCon;
        }
    }

    if (pcisCur_latest == NULL) {
        BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99A70000);
    }

    ll_cis_conn_t *pcisBackUp_earliest = pCigMaster->pcisPos_earliest;
    if (pCigMaster->pcisPos_earliest != pcisCur_earliest || pCigMaster->pcisPos_latest != pcisCur_latest) {
        ret_result = CIG_TASK_DURATION_CHANGE;
        if (pCigMaster->pcisPos_earliest != pcisCur_earliest) {
            pCigMaster->first_ap_update = type;

            if (pCigMaster->pcisPos_earliest) { //at least an CIS existed
                s32 dis_us                  = (s32)(pCigMaster->pcisPos_earliest->cig_ap_distan_us - pcisCur_earliest->cig_ap_distan_us);
                pCigMaster->bSlot_ap_offset = dis_us / 625;
            }

            pCigMaster->pcisPos_earliest = pcisCur_earliest;
            ret_result                   = CIG_TASK_FIRST_AP_CHANGE;
        }


        pCigMaster->pcisPos_latest = pcisCur_latest;

        u32 task_us                    = (pcisCur_latest->cig_ap_distan_us - pcisCur_earliest->cig_ap_distan_us) + pcisCur_latest->cis_maxPossible_us;
        pCigMaster->cism_sSlotAllocNum = (CIGMST_EARLY_SET_US + task_us) * SSLOT_US_REVERSE + 1;
    }


    if (pCigMaster->first_ap_update) {
        if (type == TYPE_ADD_CIS) {
    #if 1 //temp fix some CIG missing when first CIS disconnect then reconnect, 20230214
            if (pCigMaster->cism_task_cnt == 1) {
                pCigMaster->cigm_expect_tick = cisConn_param.cis_1st_anchor_tick;
            } else {
                pCigMaster->cigm_expect_tick -= (pcisBackUp_earliest->cig_ap_distan_us - pCigMaster->pcisPos_earliest->cig_ap_distan_us) * SYSTEM_TIMER_TICK_1US;
            }
    #else
            pCigMaster->cigm_expect_tick = cisConn_param.cis_1st_anchor_tick;
    #endif
        } else { //TYPE_DELETE_CIS
            pCigMaster->cigm_expect_tick += (pCigMaster->pcisPos_earliest->cig_ap_distan_us - pcisBackUp_earliest->cig_ap_distan_us) * SYSTEM_TIMER_TICK_1US;
        }

        u32 cigm_start_tick         = pCigMaster->cigm_expect_tick - CIGMST_EARLY_SET_US * SYSTEM_TIMER_TICK_1US;
        int n_sSlot                 = (cigm_start_tick - bltSche.sSlot_tick_irq_real) * SSLOT_TICK_REVERSE;
        pCigMaster->sSlot_mark_cigm = bltSche.sSlot_idx_irq_real + n_sSlot - pCigMaster->cism_sSlotInterval;

        if (pCigMaster->cism_task_cnt == 1 && type == TYPE_ADD_CIS) {
            /* Add this 20221122
             * to make sure for first CIG: CIS jump calculating is also correct */
            pCigMaster->bSlot_mark_cigm = bltSche.bSlot_idx_start + pCigMaster->sSlot_mark_cigm / 32;
        }
    }

    return ret_result;
}

_attribute_ram_code_ void blt_cig_mst_set_first_anchor_point(void)
{
    u8 cis_conn_idx                          = blmsParam.cig_mas_1st_sche_build_pending & SLOT_BUILD_IDX_MSK; //Marked value: cis_conn_idx
    blmsParam.cig_mas_1st_sche_build_pending = 0;                                                             //clear, must after getting "cis_conn_idx"
    blmsParam.cis_create_pending |= BIT(cis_conn_idx);

    ll_cis_conn_t *pCisConn     = (ll_cis_conn_t *)(global_pCisConn + cis_conn_idx);
    u8             cigMas_index = pCisConn->clink_cig_idx;
    ll_cig_mst_t  *pCigMaster   = (ll_cig_mst_t *)(global_pCigMst + cigMas_index);


    pCigMaster->cis_alloc_exist[pCisConn->cism_alloc_position]   = 1;
    pCigMaster->cis_alloc_markIdx[pCisConn->cism_alloc_position] = pCisConn->cis_index;


    pCigMaster->cism_task_cnt++;
    pCigMaster->cism_task_msk |= BIT(cis_conn_idx);
    if (pCigMaster->cism_task_cnt > CIS_IN_CIGM_NUM_MAX) {
        BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x998A0000 | pCigMaster->cism_task_cnt);
    }

    blt_cig_mst_calculate_timing(pCigMaster, TYPE_ADD_CIS);


    if (pCigMaster->cism_task_cnt == 1) {
        blt_sche_addTaskMask(pm_check_info ? TSKMSK_CIG_MASTER_0 << cigMas_index : 0);
    }
    blt_ll_incSchedulerTaskCalPriority(TSKOFT_CIG_MST + cigMas_index, TASK_PRIORITY_HIGH_THRES);

    pCisConn->createStatus |= CREATE_STATE_SET_1STAP; //attention: Main_loop variable special set in IRQ
    pCisConn->cisSchedFlg          = 0;
    pCisConn->cis1stSchedAPbSlot   = blmsParam.cis_1st_anchor_bSlot;
    blmsParam.cis_1st_anchor_bSlot = 0;
}

_attribute_ram_code_ int blt_ll_buildCigSchedulerLinklist(void)
{
    int           i = 0, j = 0;
    int           new_task_cnt = 0;
    ll_cig_mst_t *pCigMaster   = NULL;
    s32           sSlot_start_cig;

    int int_jump_num;

    for (i = 0; i < bltCisMng.maxNum_cig_mst; i++) {
        if (bltSche.task_mask & (TSKMSK_CIG_MASTER_0 << i)) {
            pCigMaster              = (ll_cig_mst_t *)(global_pCigMst + i);
            pCigMaster->cigTsk_wptr = pCigMaster->cigTsk_rptr = 0;

            int cur_task_offset = TSKOFT_CIG_MST + i;
            blt_ll_setSchedulerTaskPriority(cur_task_offset, TASK_PRIORITY_HIGH_THRES);

            if (bltSche.sSlot_idx_reset == 1 && (bltSche.build_index == 0)) {
                pCigMaster->sSlot_mark_cigm -= bltSche.sSlot_idx_past;
                //tlkapi_send_string_u32s(DBG_CIS_MASTER_IRQ_TIMING_DUMPLOG, "sSlot_idx_rest",i, pCigMaster->sSlot_mark_cigm, 0, 0);
            }


            if (pCigMaster->sSlot_mark_cigm >= bltSche.sSlot_idx_next) {
                int_jump_num    = 0;
                sSlot_start_cig = pCigMaster->sSlot_mark_cigm + pCigMaster->cism_sSlotInterval;
            } else {
                int_jump_num    = (bltSche.sSlot_idx_next - 1 - pCigMaster->sSlot_mark_cigm) / pCigMaster->cism_sSlotInterval;
                sSlot_start_cig = pCigMaster->sSlot_mark_cigm + (int_jump_num + 1) * pCigMaster->cism_sSlotInterval;
            }

            //if(pCigMaster->cis_delete)
            //tlkapi_send_string_u32s(0,"CIM", pCigMaster->sSlot_mark_cigm, bltSche.sSlot_idx_next, sSlot_start_cig, int_jump_num);


            int sSlot_sche_use             = (bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US) * SSLOT_US_REVERSE;
            pCigMaster->cism_sSlotDuration = pCigMaster->cism_sSlotAllocNum + sSlot_sche_use;


            for (j = 0; j < CIG_MST_FIFONUM; j++) {
                sch_task_t *pCur_schTask = (sch_task_t *)&pCigMaster->cigTsk_fifo[j];

                pCur_schTask->begin = sSlot_start_cig + j * pCigMaster->cism_sSlotInterval;
                pCur_schTask->end   = pCur_schTask->begin + pCigMaster->cism_sSlotDuration - 1;

                if (pCur_schTask->begin >= bltSche.sSlot_endIdx_dft) { //new task beyond correct range, finish
                    //if(pCigMaster->cis_delete)
                    //tlkapi_send_string_u32s(0,"beyond", pCur_schTask->begin, bltSche.sSlot_endIdx_dft, bltSche.sSlot_endIdx_maxPri, bltPri.priMax_value);
                    break;
                } else if (pCur_schTask->end < bltSche.sSlot_endIdx_dft) { //new task in correct range
                    pCigMaster->cigTsk_wptr = j;
                    new_task_cnt++;
                } else {                                                   //new task across "sSlot_endIdx_dft"

                    //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                    if (bltPri.pri_cal[cur_task_offset] > bltPri.priMax_value) {
                        bltPri.priMax_value         = bltPri.pri_cal[cur_task_offset];
                        bltPri.priMax_index         = cur_task_offset;
                        bltSche.sSlot_endIdx_maxPri = pCur_schTask->begin;

                        //if(pCigMaster->cis_delete)
                        //tlkapi_send_string_u32s(0,"across", i, bltSche.sSlot_endIdx_dft, bltSche.sSlot_endIdx_maxPri, bltPri.priMax_value);
                    }

                    break;
                }

            } //for(j=0; j<CIG_MST_FIFONUM; j++)


            if (new_task_cnt) {
                blt_ll_addTask2ExistLinklist(&pCigMaster->cigTsk_fifo[0], pCigMaster->cigTsk_wptr + 1);
            }
        }
    }

    return 1;
}


    #if (ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN)

//now only support 1 CIG
ble_sts_t blc_ll_setCigTimingOffsetOfAclCentral(u8 acl_cen_index, u16 offset_custom_us)
{
    ll_cig_mst_t *pCigMaster = (ll_cig_mst_t *)(global_pCigMst + 0);

    pCigMaster->offset_cus_en      = 1;
    pCigMaster->offset_cus_aclcIdx = acl_cen_index;
    pCigMaster->offset_cus_us      = offset_custom_us;
    //  return LL_ERR_INVALID_PARAMETER;


    return BLE_SUCCESS;
}

    #endif


#endif //end of LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER
