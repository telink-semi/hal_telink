/********************************************************************************************************
 * @file    pcl.c
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


#if (LL_FEATURE_ENABLE_POWER_CONTROL)

_attribute_ble_data_retention_ ll_pcl_cb_t global_PclCb[LL_PCL_CB_NUMS];

_attribute_aligned_(4) ll_pcl_ctrl_handler_t ll_pcl_ctrl_handler = NULL;

static u8       blt_ll_pclCalcPathLoss(st_ll_conn_t *pAclConn);
static u8       blt_ll_pclCalcAprVal(st_ll_conn_t *pAclConn);
static u8       blt_ll_pclCalcPwrLimitInfo(s8 txPwr);
static u8       blt_ll_pclCalcPathLossZone(st_ll_conn_t *pAclConn);
static s8       blt_ll_pclChgTxPwr(st_ll_conn_t *pAclConn, u8 phyIdx, s8 delta);
static pc_phy_t blt_ll_pclGetTxPwrLvlPhyFromConnPhy(ll_conn_phy_t *pConnPhy);
static int      blt_ll_pclInterruptTask(int flag, void *p);
static int      blt_ll_pclMainloopTask(int flag, void *p);
static void     blt_ll_pclReset(void);
static void     blt_ll_pclMainloop(void);
static void     blt_ll_pclInitParamsAftAclConnect(st_ll_conn_t *pAclConn);
static void     blt_ll_pclRecordAclRcvdRSSI(st_ll_conn_t *pAclConn, s8 currRcvdRssi);
static void     blt_ll_pclRecordCisRcvdRSSI(ll_cis_conn_t *pCisConn, s8 currRcvdRssi);
static void     blt_ll_pclAutoInitiateReqProc(st_ll_conn_t *pAclConn, ll_cis_conn_t *pCisConn);
static void     blt_ll_pclPathLossMonitorProc(st_ll_conn_t *pAclConn);
static void     blt_ll_pclPwrChgIndAftPhyUpt(st_ll_conn_t *pAclConn);
static void     blt_ll_pclInitParamsAftCisConnect(st_ll_conn_t *pAclConn, ll_cis_conn_t *pCisConn);
static void     blt_ll_pclPwrChgIndAftCisEst(st_ll_conn_t *pAclConn, ll_cis_conn_t *pCisConn);

static ble_sts_t blt_ll_pclControlPduProc(st_ll_conn_t *pAclConn, u8 opcode, u8 *pLlCtrlPkt);
static ble_sts_t blt_ll_pclSendReqProc(u16 connHandle, u8 phy, s8 delta, s8 txPwr);
static ble_sts_t blt_ll_pclSendRspProc(u16 connHandle, u8 limitInfo, s8 delta, s8 txPwr, u8 apr);
static ble_sts_t blt_ll_pclSendChgIndProc(u16 connHandle, u8 phy, u8 limitInfo, s8 delta, s8 txPwr, bool phychg);

void blc_ll_initPCL_module(void)
{
    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_pcl_cb_t)), pcl);
    #endif

    LL_FEATURE_MASK_1 |= (LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST << 1);
    LL_FEATURE_MASK_1 |= (LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST << 2);
    LL_FEATURE_MASK_1 |= (LL_FEATURE_ENABLE_POWER_LOSS_MONITORING << 3);

    ll_pcl_ctrl_handler    = blt_ll_pclControlPduProc;
    ll_acl_pcl_irq_task_cb = blt_ll_pclInterruptTask;
    ll_acl_pcl_mlp_task_cb = blt_ll_pclMainloopTask;

    blmsParam.pwr_ctrl_en = 1;

    /*      Configure default TX power level
     * set it by user configure API: blc_ll_setDefaultTxPowerLevel
     * if do not call the above API, below default value will be used:
     * //blmsParam.dftTxPwrLvl = 0; //default: 0dBm
     * //blmsParam.dftTxPwrLvlIdx = RF_POWER_P0dBm  */

    //Initialize the minimum acceptable RSSI (RSSImin)
    blmsParam.rssiMin = LL_PWR_CTRL_RSSI_MIN_VALUE; //TODO: Add a API to configure it's value.

    for (int i = 0; i < LL_PCL_CB_NUMS; i++) {
        smemset(&global_PclCb[i], 0, sizeof(ll_pcl_cb_t));
    }
}

static inline ll_pcl_cb_t *blt_ll_findAvailablePcl(u16 connIdx)
{
    (void)connIdx;
    assert(connIdx < LL_MAX_ACL_CONN_NUM);
    #if (LL_PCL_CB_NUMS == LL_MAX_ACL_CONN_NUM)
    return (global_PclCb + connIdx); //pcl_occpied is not used
    #elif (LL_PCL_CB_NUMS == 1)
    ll_pcl_cb_t *cur_pPcl = &global_PclCb[0];
    if (cur_pPcl->pcl_occpied == 0) {
        cur_pPcl->pcl_occpied = 1;
        return cur_pPcl;
    }
    return NULL;
    #else
    (void)connIdx;
    ll_pcl_cb_t *cur_pPcl = NULL;
    for (int i = 0; i < LL_PCL_CB_NUMS; i++) {
        cur_pPcl = global_PclCb + i;
        if (cur_pPcl->pcl_occpied == 0) {
            cur_pPcl->pcl_occpied = 1;
            return cur_pPcl;
        }
    }
    return NULL;
    #endif
}

_attribute_ram_code_ int blt_ll_pclInterruptTask(int flag, void *p)
{
    int conn_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;
    //  u16 connHandle = conn_idx | BLM_CONN_HANDLE;
    st_ll_conn_t *pAclConn = (st_ll_conn_t *)&blms[conn_idx];

    flag &= ~(u32)FLAG_SCHEDULE_TASK_IDX_MASK;

    switch (flag) {
    case FLAG_PCL_INIT_AFT_ACL_CONN:
    {
        blt_ll_pclInitParamsAftAclConnect(pAclConn);
    } break;
    case FLAG_PCL_MONITORING_ACL_RX_RSSI:
    {
        /* Need to record acl_conn rx rssi, hci_read_rssi_cmd may used it. */
        blt_ll_pclRecordAclRcvdRSSI(blms_pconn, *(s8 *)p);
    #if (1) //optimized latter, process in mainloop
        /* ACL auto monitoring RSSI to send LL_PC_REQ. (*(s8*)p => rssi) */
        blt_ll_pclAutoInitiateReqProc(pAclConn, NULL); /* phy type: le_phy_type_t */
    #endif
    } break;
    case FLAG_PCL_MONITORING_CIS_RX_RSSI:
    { //Notice: called in cis rx IRQ, blt_pCisConn can be used here
        /* record cis_rx rssi */
        blt_ll_pclRecordCisRcvdRSSI(blt_pCisConn, *(s8 *)p);
    #if (1) //optimized latter, process in mainloop
        /* CIS auto monitoring RSSI to send LL_PC_REQ. (*(s8*)p => rssi) */
        blt_ll_pclAutoInitiateReqProc(pAclConn, blt_pCisConn);
    #endif
    } break;
    case FLAG_PCL_MONITORING_PATH_LOSS:
    {
        /* hci cmd configure: path loss monitoring reporting evt concerned  */
        blt_ll_pclPathLossMonitorProc(pAclConn);
    } break;
    case FLAG_PCL_PWR_CHG_AFT_PHY_UPT:
    {
        blt_ll_pclPwrChgIndAftPhyUpt(pAclConn);
    } break;
    case FLAG_PCL_PWR_CHG_AFT_CIS_EST:
    {
        blt_ll_pclPwrChgIndAftCisEst(pAclConn, (ll_cis_conn_t *)p);
    } break;
    default:
        break;
    }

    return 0;
}

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    int
    blt_ll_pclMainloopTask(int flag, void *p)
{
    /* Attention: only 'FLAG_PCL_INIT_AFT_CIS_CONN' use this. */
    int           conn_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;
    st_ll_conn_t *pAclConn = (st_ll_conn_t *)&blms[conn_idx];
    flag &= ~(u32)FLAG_SCHEDULE_TASK_IDX_MASK;

    switch (flag) {
    case FLAG_MODULE_RESET:
    {
        blt_ll_pclReset();
    } break;
    case FLAG_MODULE_MAINLOOP:
    {
        blt_ll_pclMainloop();
    } break;
    case FLAG_PCL_INIT_AFT_CIS_CONN:
    {
        /* cis connect exec in loop, so we do this in loop too. */
        blt_ll_pclInitParamsAftCisConnect(pAclConn, (ll_cis_conn_t *)p);
    } break;
    #if (0) //optimized latter, EBQ test not good if processed in loop
    case FLAG_PCL_ACL_AUTO_INITIA_REQ:
    {
        blt_ll_pclAutoInitiateReqProc(pAclConn, NULL);
    } break;
    case FLAG_PCL_CIS_AUTO_INITIA_REQ:
    {
        blt_ll_pclAutoInitiateReqProc(pAclConn, (ll_cis_conn_t *)p);
    } break;
    #endif
    default:
        break;
    }

    return 0;
}

void blt_ll_pclReset(void)
{
    st_ll_conn_t  *pAclConn = NULL;
    ll_cis_conn_t *pCisConn = NULL;
    for (int conn_idx = 0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) {
        pAclConn         = (st_ll_conn_t *)&blms[conn_idx];
        pAclConn->pPclCb = NULL;
        pAclConn->rssi   = LL_RSSI_METRIC_VALUE; //default: RSSI can not be read
    }

    for (int i = 0; i < bltCisMng.maxNum_cisConn; i++) {
        pCisConn       = (ll_cis_conn_t *)(global_pCisConn + i);
        pCisConn->rssi = LL_RSSI_METRIC_VALUE; //default: RSSI can not be read
    }

    for (int i = 0; i < LL_PCL_CB_NUMS; i++) {
        ll_pcl_cb_t *pPclCb = &global_PclCb[i];
    #if (0)
        pPclCb->pcl_occpied                        = 0;
        pPclCb->pwrRptRemote                       = 0;
        pPclCb->pwrRptLocal                        = 0;
        pPclCb->pc_sendReq                         = 0;
        pPclCb->pcl_chg_ind_irq_pending            = 0;
        pPclCb->autoMonitor.curTimeSpent           = 0;
        pPclCb->pcl_cis_curTimeSpent               = 0;
        pPclCb->autoMinitorState                   = LL_PWR_CTRL_AUTO_MONITORING_DISABLED;
        pPclCb->autoMonitor.sendPcReqIrqPending    = 0;
        pPclCb->pathLoss.curTimeSpent              = 0;
        pPclCb->pathLoss.sendReq2StartMonitoring   = 0;
        pPclCb->pathLoss.pathLossRptEvtloopPending = 0;
        pPclCb->pathLossRptState                   = LL_PWR_CTRL_PATHLOSS_RPTING_DISABLED;
    #else //optimized
        smemset(pPclCb, 0, sizeof(ll_pcl_cb_t));
    #endif
    }
}

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    void
    blt_ll_pclMainloop(void)
{
    //ACL (PCL) concerned main_loop
    st_ll_conn_t *pc = NULL;
    for (int conn_idx = 0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) {
        pc                  = (st_ll_conn_t *)&blms[conn_idx];
        ll_pcl_cb_t *pPclCb = pc->pPclCb;
        if (pPclCb == NULL) {
            continue;
        }

        if (pc->connState == CONN_STATUS_ESTABLISH) {
            bool sendfilter = 1 && (!pc->ll_rsp_timeout_tick && !pc->ll_enc_busy) && (!pPclCb->pc_sendReq);

            //Process IRQ send LL_Power_Request flag
            if (sendfilter && pPclCb->autoMonitor.sendPcReqIrqPending) {
                u8 currTxPwrPhy = pPclCb->autoMonitor.sendPcReqTxPwrPhy;
                u8 curTxPower   = pPclCb->phyTxPwrLvl[currTxPwrPhy - 1];
                curTxPower      = (curTxPower == LL_PWR_CTRL_TXPWR_UNMNGED) ? pPclCb->usedPhyTxPwr : curTxPower;
                my_dump_str_data(DBG_LL_PCL_EN, "AutoInitiatePCLReq", &pPclCb->autoMonitor.sendDeltaMark, 1);

                if (blt_ll_pclSendReqProc(pc->acl_conHandle, currTxPwrPhy, pPclCb->autoMonitor.sendDeltaMark, curTxPower) == BLE_SUCCESS) {
                    pPclCb->autoMonitor.sendPcReqIrqPending = 0;
                }
            }
            //Process IRQ LL_Power_Change_Indication flag
            else if (sendfilter && pPclCb->pcl_chg_ind_irq_pending) {
                u8 limitInfo = blt_ll_pclCalcPwrLimitInfo(pPclCb->pcl_chg_tx_pwr);
                if (blt_ll_pclSendChgIndProc(pc->acl_conHandle, pPclCb->pcl_chg_lephy, limitInfo, 0, pPclCb->pcl_chg_tx_pwr, TRUE) != BLE_SUCCESS) {
                    /* send PCL_PWR_CHG_IND failed */
                }
                pPclCb->pcl_chg_ind_irq_pending = 0;
            }
            //Process IRQ send path loss monitoring pending event
            else if (pPclCb->pathLoss.pathLossRptEvtIrqPending) {
                if (hci_le_eventMask & HCI_LE_EVT_MASK_PATH_LOSS_THRESHOLD) {
                    my_dump_str_data(DBG_LL_PCL_EN, ">>hci_le_pathLossThreshold_evt235", 0, 0);
                    //Note: Current Path loss and zone enter ware Calculated in the IRQ, not need do it again in the loop.
                    hci_le_pathLossThreshold_evt(pc->acl_conHandle, pPclCb->pathLoss.curPathLoss, pPclCb->pathLoss.curZone);
                }
                pPclCb->pathLoss.pathLossRptEvtIrqPending = 0;
            }
            //Process Mainloop path loss monitoring pending event
            else if (pPclCb->pathLoss.pathLossRptEvtloopPending) {
                if (hci_le_eventMask & HCI_LE_EVT_MASK_PATH_LOSS_THRESHOLD) {
                    my_dump_str_data(DBG_LL_PCL_EN, ">>hci_le_pathLossThreshold_evt244", 0, 0);
                    u8 curPathLoss = blt_ll_pclCalcPathLoss(pc);
                    my_dump_str_data(DBG_LL_PCL_EN, "curPathLoss0", &curPathLoss, 1);
                    u8 zoneEntered = pPclCb->pathLoss.curZone = blt_ll_pclCalcPathLossZone(pc);
                    my_dump_str_data(DBG_LL_PCL_EN, "curZone0", &zoneEntered, 1);
                    hci_le_pathLossThreshold_evt(pc->acl_conHandle, curPathLoss, zoneEntered);
                }
                pPclCb->pathLoss.pathLossRptEvtloopPending = 0;
            }
        }
    }
}

static s8 blt_ll_pclGetRfActualTxPwr(s8 txPwr, bool needComp)
{
    /*VBAT*/
    if (txPwr >= 9) {
        txPwr = 9;
    } else if (txPwr >= 8) {
        txPwr = 8;
    } else if (txPwr >= 7) {
        txPwr = 7;
    } else if (txPwr >= 6) {
        txPwr = 6;
    }
    /*VANT*/
    else if (txPwr >= 5) {
        txPwr = 5;
    } else if (txPwr >= 4) {
        txPwr = 4;
    } else if (txPwr >= 3) {
        txPwr = 3;
    } else if (txPwr >= 2) {
        txPwr = 2;
    } else if (txPwr >= 1) {
        txPwr = 1;
    } else if (txPwr >= 0) {
        txPwr = 0;
    } else if (txPwr >= -4) {
        txPwr = -4;
    } else if (txPwr >= -8) {
        txPwr = -8;
    } else if (txPwr >= -12) {
        txPwr = -12;
    } else if (txPwr >= -18) {
        txPwr = -18;
    } else {
        txPwr = -23;
    }

    if (needComp) {
        txPwr += blt_ll_getRfTxPathComp();
    }

    return txPwr;
}

static s8 blt_ll_pclIncRfTxPwr(s8 reqPwr, s8 delta)
{
    /* Always increase one step */
    if (delta > 0) {
        /*VBAT*/
        if (reqPwr > 8) {
            reqPwr = 9;
        } else if (reqPwr > 7) {
            reqPwr = 8;
        } else if (reqPwr > 6) {
            reqPwr = 7;
        } else if (reqPwr > 5) {
            reqPwr = 6;
        }
        /*VANT*/
        else if (reqPwr > 4) {
            reqPwr = 5;
        } else if (reqPwr > 3) {
            reqPwr = 4;
        } else if (reqPwr > 2) {
            reqPwr = 3;
        } else if (reqPwr > 1) {
            reqPwr = 2;
        } else if (reqPwr > 0) {
            reqPwr = 1;
        } else if (reqPwr > -4) {
            reqPwr = 0;
        } else if (reqPwr > -8) {
            reqPwr = -4;
        } else if (reqPwr > -12) {
            reqPwr = -8;
        } else if (reqPwr > -18) {
            reqPwr = -12;
        } else if (reqPwr > -23) {
            reqPwr = -18;
        } else {
            reqPwr = -23;
        }
    }
    /* decrease to higher step */
    else if (delta < 0) {
        /*VANT*/
        if (reqPwr <= -23) {
            reqPwr = -23;
        } else if (reqPwr <= -18) {
            reqPwr = -18;
        } else if (reqPwr <= -12) {
            reqPwr = -12;
        } else if (reqPwr <= -8) {
            reqPwr = -8;
        } else if (reqPwr <= -4) {
            reqPwr = -4;
        } else if (reqPwr <= 0) {
            reqPwr = 0;
        } else if (reqPwr <= 1) {
            reqPwr = 1;
        } else if (reqPwr <= 2) {
            reqPwr = 2;
        } else if (reqPwr <= 3) {
            reqPwr = 3;
        } else if (reqPwr <= 4) {
            reqPwr = 4;
        } else if (reqPwr <= 5) {
            reqPwr = 5;
        }
        /*VBAT*/
        else if (reqPwr <= 6) {
            reqPwr = 6;
        } else if (reqPwr <= 7) {
            reqPwr = 7;
        } else if (reqPwr <= 8) {
            reqPwr = 8;
        } else {
            reqPwr = 9;
        }
    } else {
        /* No change. */
    }

    return reqPwr;
}

/*
 * Switch from connPHY to txPwrLvlPHY
 */
static pc_phy_t blt_ll_pclGetTxPwrLvlPhyFromConnPhy(ll_conn_phy_t *pConnPhy)
{
    //conPhy: refer to 'le_phy_type_t'
    //txPwrLvlPhy: refer to 'pc_phy_t'

    u8 txPhy = pConnPhy->conn_cur_phy;
    if (pConnPhy->conn_cur_phy == BLE_PHY_CODED && pConnPhy->conn_cur_CI == LE_CODED_S2) {
        txPhy = txPhy + 1; //S8:3;  S2:4
    }

    return txPhy;
}

static inline void blt_ll_pclRecordAclRcvdRSSI(st_ll_conn_t *pAclConn, s8 currRcvdRssi) //Used in ACL rx IRQ handler
{
    /* read register value */
    currRcvdRssi = rf_get_rssi();

    if (pAclConn->rssi == LL_RSSI_METRIC_VALUE) {
        pAclConn->rssi = currRcvdRssi;
    } else {
        pAclConn->rssi = (pAclConn->rssi + currRcvdRssi) >> 1;
    }

    pAclConn->rssi += blt_ll_getRfRxPathComp();
}

static inline void blt_ll_pclRecordCisRcvdRSSI(ll_cis_conn_t *pCisConn, s8 currRcvdRssi) //Used in ACL rx IRQ handler
{
    /* read register value */
    currRcvdRssi = rf_get_rssi();

    if (pCisConn->rssi == LL_RSSI_METRIC_VALUE) {
        pCisConn->rssi = currRcvdRssi;
    } else {
        pCisConn->rssi = (pCisConn->rssi + currRcvdRssi) >> 1;
    }

    pCisConn->rssi += blt_ll_getRfRxPathComp();
}

    /*
 * Calculate path loss of a connection
 */
    #if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
    #endif
    static u8
    blt_ll_pclCalcPathLoss(st_ll_conn_t *pAclConn)
{
    assert(pAclConn->pPclCb != NULL);
    s8 txPower  = pAclConn->pPclCb->peerTxPwrLvl;
    s8 lastRssi = pAclConn->rssi;

    //my_dump_str_data(DBG_LL_PCL_EN, "peerTXLvl", &txPower, 1);
    //my_dump_str_data(DBG_LL_PCL_EN, "RSSI", &lastRssi, 1);

    /* Refer to <<CSS_v9>>: pathloss = Tx Power Level C RSSI */
    return (u8)(max(txPower, lastRssi) - min(txPower, lastRssi));
}

/*
 * Calculate Apr for Power Control, only used in Power Control RSP field
 */
static u8 blt_ll_pclCalcAprVal(st_ll_conn_t *pAclConn)
{
    /*
     * APR = 0,                 if RSSIcurr  RSSImin
     * APR = RSSIcurr C RSSImin,if RSSIcurr > RSSImin
     */
    if (pAclConn->rssi <= blmsParam.rssiMin) {
        return 0;
    } else {
        return (pAclConn->rssi - blmsParam.rssiMin);
    }
}

static s8 blt_ll_pclChgTxPwr(st_ll_conn_t *pAclConn, u8 phyIdx, s8 delta)
{
    ll_pcl_cb_t *pPclCb = pAclConn->pPclCb;
    assert(pPclCb != NULL);

    s8 reqTxPower, curTxPower, newTxPower;
    if (pPclCb->phyTxPwrLvl[phyIdx - 1] == LL_PWR_CTRL_TXPWR_UNAVA) {
        /* The current controller does not support this phy. */
        my_dump_str_data(DBG_LL_PCL_EN, "The current controller does not support this phy", 0, 0);
        return 0;
    } else if (pPclCb->phyTxPwrLvl[phyIdx - 1] == LL_PWR_CTRL_TXPWR_UNMNGED) {
        curTxPower = pPclCb->usedPhyTxPwr;
        reqTxPower = pPclCb->usedPhyTxPwr + delta;
        my_dump_str_data(DBG_LL_PCL_EN, "curTxPower", &curTxPower, 1);
        my_dump_str_data(DBG_LL_PCL_EN, "reqTxPower", &reqTxPower, 1);
    } else {
        curTxPower = blt_ll_pclGetRfActualTxPwr(pPclCb->phyTxPwrLvl[phyIdx - 1], FALSE);
        reqTxPower = pPclCb->phyTxPwrLvl[phyIdx - 1] + delta;
        my_dump_str_data(DBG_LL_PCL_EN, "curTxPower'", &curTxPower, 1);
        my_dump_str_data(DBG_LL_PCL_EN, "reqTxPower'", &reqTxPower, 1);
    }

    /* LL_PCL_REQ's delta: A value of 0x7F indicates a request to increase to the maximum power level. */

    /* Overflow catch condition. e.g.:s8(127+9) = -120 */
    if ((delta > 0) && (reqTxPower < curTxPower)) {
        reqTxPower = LL_PWR_CTRL_TXPWR_MAX;
        my_dump_str_data(DBG_LL_PCL_EN, "Overflow catch condition:reqTxPower'", &reqTxPower, 1);
    }

    newTxPower = blt_ll_pclIncRfTxPwr(reqTxPower, delta);
    my_dump_str_data(DBG_LL_PCL_EN, "newTxPower", &newTxPower, 1);

    /* Update txPower. */
    pPclCb->phyTxPwrLvl[phyIdx - 1] = newTxPower;

    /* Update current txPower if necessary. */
    u8 currTxPwrPhy = blt_ll_pclGetTxPwrLvlPhyFromConnPhy(&pAclConn->connPhyCtrl);
    if (phyIdx == currTxPwrPhy) {
        pPclCb->usedPhyTxPwr = newTxPower;
        my_dump_str_data(DBG_LL_PCL_EN, "usedPhyTxPwr1", &pPclCb->usedPhyTxPwr, 1);
        pAclConn->currRfPwrIdx = rf_ble_get_tx_pwr_idx(newTxPower);
    }

    return newTxPower - curTxPower;
}

/*
 * Calculate sender is at the minimum or maximum supported power
 */
static u8 blt_ll_pclCalcPwrLimitInfo(s8 txPwr)
{
    s8 min, max;
    blc_ll_readSuppTxPower(&min, &max);

    if (txPwr == min) {
        return LL_PWR_CTRL_LIMIT_MIN_BIT;
    } else if (txPwr == max) {
        return LL_PWR_CTRL_LIMIT_MAX_BIT;
    } else {
        return 0;
    }
}

    /*
 * Calculate path loss of a connection
 */
    #if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
    #endif
    static u8
    blt_ll_pclCalcPathLossZone(st_ll_conn_t *pAclConn)
{
    ll_pcl_cb_t *pPclCb = pAclConn->pPclCb;
    assert(pPclCb != NULL);

    u8 pathLoss = blt_ll_pclCalcPathLoss(pAclConn);
    u8 curZone  = pPclCb->pathLoss.curZone;

    u8 highUpperBound = pPclCb->pathLoss.highThreshold + pPclCb->pathLoss.highHysteresis;
    u8 highLowerBound = pPclCb->pathLoss.highThreshold - pPclCb->pathLoss.highHysteresis;

    u8 lowUpperBound = pPclCb->pathLoss.lowThreshold + pPclCb->pathLoss.lowHysteresis;
    u8 lowLowerBound = pPclCb->pathLoss.lowThreshold - pPclCb->pathLoss.lowHysteresis;

    switch (curZone) {
    case LL_PWR_CTRL_PATH_LOSS_ZONE_LOW:
    {
        if (pathLoss > highUpperBound) {
            return LL_PWR_CTRL_PATH_LOSS_ZONE_HIGH;
        } else if (pathLoss > lowUpperBound) {
            return LL_PWR_CTRL_PATH_LOSS_ZONE_MID;
        } else {
            return LL_PWR_CTRL_PATH_LOSS_ZONE_LOW;
        }
    }

    case LL_PWR_CTRL_PATH_LOSS_ZONE_MID:
    {
        if (pathLoss > highUpperBound) {
            return LL_PWR_CTRL_PATH_LOSS_ZONE_HIGH;
        } else if (pathLoss <= lowLowerBound) {
            return LL_PWR_CTRL_PATH_LOSS_ZONE_LOW;
        } else {
            return LL_PWR_CTRL_PATH_LOSS_ZONE_MID;
        }
    }

    case LL_PWR_CTRL_PATH_LOSS_ZONE_HIGH:
    {
        if (pathLoss <= lowLowerBound) {
            return LL_PWR_CTRL_PATH_LOSS_ZONE_LOW;
        } else if (pathLoss < highLowerBound) {
            return LL_PWR_CTRL_PATH_LOSS_ZONE_MID;
        } else {
            return LL_PWR_CTRL_PATH_LOSS_ZONE_HIGH;
        }
    }

    default:
        //Path loss enter invalid zone
        return LL_PWR_CTRL_PATH_LOSS_ZONE_MID;
    }
}

//phy type: refer to 'pc_phy_t'
_attribute_noinline_
    ble_sts_t
    blt_ll_pclSendReqProc(u16 connHandle, u8 phy, s8 delta, s8 txPwr)
{
    my_dump_str_data(DBG_LL_PCL_EN, "Send:LL_POWER_CONTROL_REQ", &txPwr, 1);
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        my_dump_str_data(DBG_LL_PCL_EN, "HCI_ERR_UNKNOWN_CONN_ID", 0, 0);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    // Feature available check
    if (!(LL_FEATURE_MASK_1 & (LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST << 1))) {
        my_dump_str_data(DBG_LL_PCL_EN, "HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE0", 0, 0);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    u8 pc_phy = phy; /* keep phy temp */
    //phy type: refer to 'pc_phy_t'
    if (phy == BLE_PC_PHY_NONE || phy > BLE_PC_PHY_TOTAL) {
        my_dump_str_data(DBG_LL_PCL_EN, "HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE1", 0, 0);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    } else {
        /* switch from 'pc_phy_t' to 'pc_phy_bits_t'. */
        phy = BIT(phy - 1);
    }

    st_ll_conn_t *pc     = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_pcl_cb_t  *pPclCb = pc->pPclCb;
    assert(pPclCb != NULL);

    pPclCb->pc_reqPhy = pc_phy;
    pPclCb->pc_delta  = delta;

    /* Refer to <<Core5.3>>, Page 2719, 2.4.2.33 LL_POWER_CONTROL_REQ
     * TxPower shall be set to the sender's transmit power level for the PHY
     * indicated. The value is in dBm, represented as a signed integer. When set to
     * 127, it indicates that the value is unavailable. It shall not be set to 126
     */
    if (txPwr == LL_PWR_CTRL_TXPWR_UNMNGED) {
        txPwr                           = pPclCb->usedPhyTxPwr; /* here we use current ACL tx_pwr_lvl */
        pPclCb->phyTxPwrLvl[pc_phy - 1] = txPwr;
        my_dump_str_data(DBG_LL_PCL_EN, "     TxPwr:126, should set to 127 or valid TX power level(9~-23)", 0, 0);
        my_dump_str_data(DBG_LL_PCL_EN, "     Here use usedPhyTxPwr:", &txPwr, 1);
    }

    u8                        tmp[sizeof(rf_pkt_ll_pwr_ctrl_req_t)];
    rf_pkt_ll_pwr_ctrl_req_t *pReq = (rf_pkt_ll_pwr_ctrl_req_t *)tmp;
    pReq->llid                     = LLID_CONTROL;
    pReq->rf_len                   = 4;
    pReq->opcode                   = LL_POWER_CONTROL_REQ;
    pReq->phy                      = phy; //refer to 'pc_phy_bits_t'
    pReq->delta                    = delta;
    pReq->txPwr                    = txPwr;

    //always consider that: push TX FIFO may not success
    if (blt_llmsPushLlCtrlPkt(connHandle, LL_POWER_CONTROL_REQ, tmp)) {
        pc->ll_rsp_timeout_tick = clock_time() | 1;
        pPclCb->pc_sendReq      = 1;
    } else {
        BLMS_ERR_DEBUG(DBG_PCL_LOGIC, __LINE__);
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }

    return BLE_SUCCESS;
}

_attribute_noinline_
    ble_sts_t
    blt_ll_pclSendRspProc(u16 connHandle, u8 limitInfo, s8 delta, s8 txPwr, u8 apr)
{
    my_dump_str_data(DBG_LL_PCL_EN, "Send:LL_POWER_CONTROL_RSP", 0, 0);
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    st_ll_conn_t *pc     = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_pcl_cb_t  *pPclCb = pc->pPclCb;
    assert(pPclCb != NULL);
    (void)pPclCb;

    u8                        tmp[sizeof(rf_pkt_ll_pwr_ctrl_rsp_t)] = {0};
    rf_pkt_ll_pwr_ctrl_rsp_t *pRsp                                  = (rf_pkt_ll_pwr_ctrl_rsp_t *)tmp;
    pRsp->llid                                                      = LLID_CONTROL;
    pRsp->rf_len                                                    = 5;
    pRsp->opcode                                                    = LL_POWER_CONTROL_RSP;
    pRsp->limitInfo                                                 = limitInfo;
    //pRsp->limitInfo.rfu = 0;
    pRsp->delta = delta;
    pRsp->txPwr = txPwr;
    pRsp->APR   = apr;

    //always consider that: push TX FIFO may not success
    if (blt_llmsPushLlCtrlPkt(connHandle, LL_POWER_CONTROL_RSP, tmp)) {
        pc->ll_rsp_timeout_tick = 0;
        my_dump_str_data(DBG_LL_PCL_EN, "push TX FIFO OK", 0, 0);
    } else {
        BLMS_ERR_DEBUG(DBG_PCL_LOGIC, __LINE__);
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }

    return BLE_SUCCESS;
}

//phy type: refer to 'le_phy_type_t'
_attribute_noinline_
    ble_sts_t
    blt_ll_pclSendChgIndProc(u16 connHandle, u8 phy, u8 limitInfo, s8 delta, s8 txPwr, bool phychg)
{
    my_dump_str_data(DBG_LL_PCL_EN, "Send:LL_POWER_CHANGE_IND", 0, 0);
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    st_ll_conn_t *pc = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    //ll_conn_phy_t* pConnPhy = &pc->connPhyCtrl;
    ll_pcl_cb_t *pPclCb = pc->pPclCb;
    assert(pPclCb != NULL);

    /*
     * After the peer has sent at least one LL_POWER_CONTROL_REQ PDU, a
     * Link Layer shall send an autonomous notification consisting of an
     * LL_POWER_CHANGE_IND PDU each time that any of the following happens:
     *
     *      * It changes the power level autonomously on any PHY that it is managing power levels for.
     *      * It changes the maximum power level on its current transmit PHY to the current power level.
     *      * It starts managing the power level for a PHY.
     *      * It stops managing the power level for a PHY.
     */
    if (pPclCb->pc_peerReqRcvd == FALSE) {
        return HCI_ERR_CMD_DISALLOWED;
    }

    //phy type: refer to 'le_phy_type_t'
    if (phy < BLE_PHY_1M || phy > BLE_PHY_CODED) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    /* switch from 'le_phy_type_t' to 'pc_phy_bits_t'. */
    phy = BIT(phy - 1);

    bool send2ndChgInd = FALSE;
    /* Attempt to pack Coded (S2 && S8) in one chg_ind. */
    if (phychg == TRUE && (phy == BLE_PC_CODED_S8_BIT)) {
        if (pPclCb->phyTxPwrLvl[BLE_PC_PHY_CODED_S8 - 1] == pPclCb->phyTxPwrLvl[BLE_PC_PHY_CODED_S2 - 1]) {
            phy |= BLE_PC_CODED_S2_BIT;
        } else {
            send2ndChgInd = TRUE;
        }
    }

    u8                       tmp[sizeof(rf_pkt_ll_pwr_chg_ind_t)];
    rf_pkt_ll_pwr_chg_ind_t *pChgInd = (rf_pkt_ll_pwr_chg_ind_t *)tmp;
    pChgInd->llid                    = LLID_CONTROL;
    pChgInd->rf_len                  = 5;
    pChgInd->opcode                  = LL_POWER_CHANGE_IND;
    pChgInd->phy                     = phy;
    pChgInd->limitInfo               = limitInfo;
    //pChgInd->limitInfo.rfu = 0;
    pChgInd->delta = delta;
    pChgInd->txPwr = txPwr;

    //always consider that: push TX FIFO may not success
    if (!blt_llmsPushLlCtrlPkt(connHandle, LL_POWER_CHANGE_IND, tmp)) {
        BLMS_ERR_DEBUG(DBG_PCL_LOGIC, __LINE__);
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }

    /* For LE coded PHY, we need to send S2 txPower as well. */
    if (send2ndChgInd) {
        txPwr              = pPclCb->phyTxPwrLvl[BLE_PC_PHY_CODED_S2 - 1];
        pChgInd->phy       = BLE_PC_CODED_S2_BIT;
        pChgInd->limitInfo = blt_ll_pclCalcPwrLimitInfo(txPwr);
        ;
        //pChgInd->limitInfo.rfu = 0;
        pChgInd->delta = delta;
        pChgInd->txPwr = txPwr;

        if (!blt_llmsPushLlCtrlPkt(connHandle, LL_POWER_CHANGE_IND, tmp)) {
            BLMS_ERR_DEBUG(DBG_PCL_LOGIC, __LINE__);
            return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
        }
    }

    return BLE_SUCCESS;
}

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    ble_sts_t
    blt_ll_pclControlPduProc(st_ll_conn_t *pAclConn, u8 opcode, u8 *pLlCtrlPkt)
{
    ll_pcl_cb_t *pPclCb = pAclConn->pPclCb;

    u8 currTxPwrPhy = blt_ll_pclGetTxPwrLvlPhyFromConnPhy(&pAclConn->connPhyCtrl);

    // Feature available check
    if (opcode == LL_POWER_CONTROL_REQ) {
        my_dump_str_data(DBG_LL_PCL_EN, "Rcvd:LL_POWER_CONTROL_REQ", 0, 0);
        if (!(LL_FEATURE_MASK_1 & (LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST << 1))) {
            return LL_ERR_UNKNOWN_OPCODE;
        }

        rf_pkt_ll_pwr_ctrl_req_t *pPcReq = (rf_pkt_ll_pwr_ctrl_req_t *)pLlCtrlPkt;

        /*
         * If the PHY in the LL_POWER_CONTROL_REQ PDU is not supported by the
         * remote Link Layer in its transmit direction, the remote Link Layer shall respond
         * with an LL_REJECT_EXT_IND PDU with the ErrorCode set to Unsupported
         * LMP Parameter Value/Unsupported LL Parameter Value (0x20).
         */
        u8 phyIdx;
        for (phyIdx = BLE_PC_PHY_1M; phyIdx <= BLE_PC_PHY_TOTAL; phyIdx++) {
            if ((1 << (phyIdx - 1)) == pPcReq->phy) { //pPcReq->phy: refer to 'pc_phy_bits_t'
                break;
            }
        }
        if (phyIdx > BLE_PC_PHY_TOTAL) {
            return HCI_ERR_UNSUPPORTED_LMP_PARAM_VAL;
        }
        /*
         * If the TxPower in the LL_POWER_CONTROL_REQ PDU is set to 126, the
         * remote Link Layer shall respond with an LL_REJECT_EXT_IND_PDU with the
         * ErrorCode set to Invalid LL Parameters (0x1E)
         */
        if (pPcReq->txPwr == LL_PWR_CTRL_TXPWR_UNMNGED) {
            return HCI_ERR_INVALID_LMP_PARAMS;
        }

        my_dump_str_data(DBG_LL_PCL_EN, "     [req]currTxPwrPhy", &currTxPwrPhy, 1);
        my_dump_str_data(DBG_LL_PCL_EN, "     [req]pc_reqPhy", &phyIdx, 1);

        /* Update peer Tx power. */
        if (phyIdx == currTxPwrPhy) {
            pPclCb->peerTxPwrLvl = pPcReq->txPwr;
            my_dump_str_data(DBG_LL_PCL_EN, "     [req]peerTxPwrLvl", &pPcReq->txPwr, 1);
        }

        pPclCb->pc_peerReqRcvd = TRUE;
        pPclCb->pc_reqPhy      = phyIdx;
        s8 delta               = blt_ll_pclChgTxPwr(pAclConn, phyIdx, pPcReq->delta);
        s8 txPwr               = pPclCb->phyTxPwrLvl[phyIdx - 1];
        u8 limitInfo           = blt_ll_pclCalcPwrLimitInfo(txPwr);

        /*
         * If the Link Layer has sent an LL_POWER_CONTROL_REQ PDU and not yet received a
         * response, or has an LL_POWER_CONTROL_REQ PDU queued for transmission, and then
         * receives an LL_POWER_CONTROL_REQ PDU from the same peer device, it shall set
         * the APR field to 0xFF in its response to that PDU.
         */
        u8 apr = 0xFF;
        if (pPclCb->pc_sendReq) { //Already send PCL_REQ
            //do nothing
        }
        /*
         * A responding Link Layer may also set the APR field to 0xFF when it is not
         * managing the power level of the requested PHY, if it does not have a valid
         *  value to report, or if it does not support this field
         */
        else if (pPclCb->phyTxPwrLvl[phyIdx - 1] == LL_PWR_CTRL_TXPWR_UNAVA) {
            //blt_ll_pclChgTxPwr()  already managing the power level if the requested PHY is supported
            //do nothing
        } else {
            apr = blt_ll_pclCalcAprVal(pAclConn);
        }

        if (pPclCb->pwrRptLocal && delta != 0) {
            if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_TRANSMIT_POWER_REPORTING) {
                my_dump_str_data(DBG_LL_PCL_EN, ">>hci_le_transmitPwrRpting_evt815", 0, 0);
                hci_le_transmitPwrRpting_evt(BLE_SUCCESS, pAclConn->acl_conHandle, LL_PWR_CTRL_RPTING_REASON_LOCAL, pPclCb->pc_reqPhy, txPwr, limitInfo, delta);
            }
        }

        if (blt_ll_pclSendRspProc(pAclConn->acl_conHandle, limitInfo, delta, txPwr, apr) != BLE_SUCCESS) {
            /*
             * Note: At present, the task push must be successful. Here, we use the
             * controller's spare 2 fifos for control. If it is more serious,
             * it is best to check if it is unsuccessful and send it again in
             * the loop, which is troublesome. You can consider adding.
             * TODO later:
             */
        }
    } else if (opcode == LL_POWER_CONTROL_RSP) {
        my_dump_str_data(DBG_LL_PCL_EN, "Rcvd:LL_POWER_CONTROL_RSP", 0, 0);
        if (pPclCb->pc_sendReq) {
            pPclCb->pc_sendReq            = 0;
            pAclConn->ll_rsp_timeout_tick = 0;
        }

        if (!(LL_FEATURE_MASK_1 & (LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST << 1))) {
            return LL_ERR_UNKNOWN_OPCODE;
        }

        rf_pkt_ll_pwr_ctrl_rsp_t *pPcRsp = (rf_pkt_ll_pwr_ctrl_rsp_t *)pLlCtrlPkt;

        /* Refer to <<Core5.3>>, Page2720, 2.4.2.34 LL_POWER_CONTROL_RSP
         * Tx Power: When set to 126, it indicates that the sender is not currently managing
         * power for the requested PHY; in this case all other fields shall be ignored
         */
        if (pPcRsp->txPwr == LL_PWR_CTRL_TXPWR_UNMNGED) {
            my_dump_str_data(DBG_LL_PCL_EN, "     TxPwr:126, all other fields are ignored", 0, 0);
            return BLE_SUCCESS; //ignore this packet
        }

        if (pPclCb->rdRemoteTxPwr) {
            pPclCb->rdRemoteTxPwr = FALSE;

            if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_TRANSMIT_POWER_REPORTING) {
                my_dump_str_data(DBG_LL_PCL_EN, ">>hci_le_transmitPwrRpting_evt856", 0, 0);
                hci_le_transmitPwrRpting_evt(BLE_SUCCESS, pAclConn->acl_conHandle, LL_PWR_CTRL_RPTING_READ_REMOTE, pPclCb->pc_reqPhy, pPcRsp->txPwr, pPcRsp->limitInfo, pPcRsp->delta);
            }
        } else if (pPclCb->pwrRptLocal && (pPclCb->peerTxPwrLvl != pPcRsp->txPwr)) {
            if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_TRANSMIT_POWER_REPORTING) {
                my_dump_str_data(DBG_LL_PCL_EN, ">>hci_le_transmitPwrRpting_evt863", 0, 0);
                hci_le_transmitPwrRpting_evt(BLE_SUCCESS, pAclConn->acl_conHandle, LL_PWR_CTRL_RPTING_REASON_LOCAL, pPclCb->pc_reqPhy, pPcRsp->txPwr, pPcRsp->limitInfo, pPcRsp->txPwr - pPclCb->peerTxPwrLvl);
            }
        }

        pPclCb->pc_delta = pPcRsp->delta;

        //my_dump_str_data(DBG_LL_PCL_EN, "     [rsp]currTxPwrPhy", &currTxPwrPhy, 1);
        //my_dump_str_data(DBG_LL_PCL_EN, "     [rsp]pc_reqPhy", &pAclConn->pc_reqPhy, 1);


        /* for LL/PCL/PER/BV-52-C  [Path Loss Monitoring Unavailable - LE Coded PHY S=2 - Initiate, Peripheral] */
        if (1 && currTxPwrPhy == BLE_PC_PHY_CODED_S8 && pAclConn->peer_coded_phy_ci == LE_CODED_S2) {
            currTxPwrPhy = BLE_PC_PHY_CODED_S2;
            my_dump_str_data(DBG_LL_PCL_EN, "<<<peer use S2>>>", 0, 0);
        }

        //update peer's PC information
        if (currTxPwrPhy == pPclCb->pc_reqPhy) {
            pPclCb->peerTxPwrLvl = pPcRsp->txPwr;
            my_dump_str_data(DBG_LL_PCL_EN, "     [rsp]peerTxPwrLvl", &pPcRsp->txPwr, 1);

            if (pPclCb->peerTxPwrLvl != LL_PWR_CTRL_TXPWR_UNAVA &&
                pPclCb->peerTxPwrLvl != LL_PWR_CTRL_TXPWR_UNMNGED) {
                pPclCb->peerApr[pPclCb->pc_reqPhy - 1] = pPcRsp->APR;
                pPclCb->peerLimitInfo                  = pPcRsp->limitInfo;
            } else { /* unavailable , clear */
                pPclCb->peerApr[pPclCb->pc_reqPhy - 1] = 0;
                pPclCb->peerLimitInfo                  = 0;
            }

            if (pPclCb->pathLossRptState == LL_PWR_CTRL_PATHLOSS_RPTING_ENABLED) {
                bool need_send_evt = FALSE;
                u8   curPathLoss, zoneEntered;
                if (pPclCb->peerTxPwrLvl != LL_PWR_CTRL_TXPWR_UNAVA &&
                    pPclCb->peerTxPwrLvl != LL_PWR_CTRL_TXPWR_UNMNGED) {
                    if (pPclCb->pathLoss.sendReq2StartMonitoring) {
                        pPclCb->pathLoss.sendReq2StartMonitoring = 0;
                        my_dump_str_data(DBG_LL_PCL_EN, "sendReq2StartMonitoring=0'", 0, 0);
                        curPathLoss = blt_ll_pclCalcPathLoss(pAclConn);
                        zoneEntered = pPclCb->pathLoss.curZone = blt_ll_pclCalcPathLossZone(pAclConn);
                        need_send_evt                          = TRUE;
                    }
                }
                /* Refer to /LL/PCL/PER/BV-49-C */
                /* Refer to <<Core5.3 | Vol 6, Part B>>, page 2824
                 * The Controller may notify the Host when the path loss becomes unavailable. If
                 * so, it shall notify the Host when the path loss becomes available again as if it
                 * had just changed zones.
                 */
                else if (pPclCb->peerTxPwrLvl == LL_PWR_CTRL_TXPWR_UNAVA) {
                    curPathLoss   = 0xFF; //curPathLoss = blt_ll_pclCalcPathLoss(pAclConn);
                    zoneEntered   = LL_PWR_CTRL_PATH_LOSS_ZONE_MID;
                    need_send_evt = TRUE;
                }

                if (need_send_evt == TRUE) {
                    //Generate HCI event: HCI_LE_Path_Loss_Threshold
                    if (hci_le_eventMask & HCI_LE_EVT_MASK_PATH_LOSS_THRESHOLD) {
                        my_dump_str_data(DBG_LL_PCL_EN, ">>hci_le_pathLossThreshold_evt938", 0, 0);
                        my_dump_str_data(DBG_LL_PCL_EN, "curPathLoss1", &curPathLoss, 1);
                        my_dump_str_data(DBG_LL_PCL_EN, "curZone1", &zoneEntered, 1);
                        hci_le_pathLossThreshold_evt(pAclConn->acl_conHandle, curPathLoss, zoneEntered);
                    }
                }
            }
        }
    } else if (opcode == LL_POWER_CHANGE_IND) {
        my_dump_str_data(DBG_LL_PCL_EN, "Rcvd:LL_POWER_CHANGE_IND", 0, 0);
        if (!(LL_FEATURE_MASK_1 & (LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST << 2))) {
            return LL_ERR_UNKNOWN_OPCODE;
        }

        rf_pkt_ll_pwr_chg_ind_t *pPchgRsp = (rf_pkt_ll_pwr_chg_ind_t *)pLlCtrlPkt;

        /* Refer to <<LL_TS.p19.pdf>>: 4.13.3.14 Power Change Request using an invalid or unsupported PHY */
        u8 phyIdx;
        for (phyIdx = BLE_PC_PHY_1M; phyIdx <= BLE_PC_PHY_TOTAL; phyIdx++) {
            if ((1 << (phyIdx - 1)) == pPchgRsp->phy) { //pPchgRsp->phy: refer to 'pc_phy_bits_t'
                break;
            }
        }
        if (phyIdx > BLE_PC_PHY_TOTAL) {
            //ignore this packet, check it latter
            return BLE_SUCCESS; //HCI_ERR_UNSUPPORTED_LMP_PARAM_VAL;
        }


        if (pPclCb->pwrRptRemote && (pPclCb->peerTxPwrLvl != pPchgRsp->txPwr)) {
            if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_TRANSMIT_POWER_REPORTING) {
                my_dump_str_data(DBG_LL_PCL_EN, ">>hci_le_transmitPwrRpting_evt928", 0, 0);
                hci_le_transmitPwrRpting_evt(BLE_SUCCESS, pAclConn->acl_conHandle, LL_PWR_CTRL_RPTING_REASON_REMOTE, pPclCb->pc_reqPhy, pPchgRsp->txPwr, pPchgRsp->limitInfo, pPchgRsp->delta);
            }
        }

        if (pPchgRsp->phy == BIT(currTxPwrPhy - 1)) { //phy type: refer to 'pc_phy_bits_t'
            pPclCb->peerTxPwrLvl = pPchgRsp->txPwr;
        }
    } else if (opcode == LL_REJECT_IND_EXT) {
        rf_packet_ll_reject_ext_ind_t *pRejectExtInd = (rf_packet_ll_reject_ext_ind_t *)pLlCtrlPkt;

        if (pRejectExtInd->opcode == LL_POWER_CONTROL_REQ) {
            pAclConn->ll_rsp_timeout_tick = 0;
        }
    }

    return BLE_SUCCESS;
}

    /**
 * @brief      This function is used to actively set the TX power level, may trigger PWR_CHG_IND
 * @param[in]  connHandle - ACL connection handle.
 * @param[in]  phy - refer to 'pc_phy_t'
 * @param[in]  txPwrLvl - -23dBm ~ 9dBm
 * @return     status, 0x00:  succeed
 *                     other: failed
 * NOTE: * It changes the power level autonomously on any PHY that it is managing power levels for.
 */
    #if (0)
ble_sts_t blc_ll_pclSetPhyTxPwrLvl(u16 connHandle, u8 phy, u8 txPwrLvl)
{
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if (phy > BLE_PC_PHY_TOTAL) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    st_ll_conn_t *pc     = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_pcl_cb_t  *pPclCb = pc->pPclCb;
    if (pPclCb == NULL) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    u8 currTxPwrPhy = blt_ll_pclGetTxPwrLvlPhyFromConnPhy(&pc->connPhyCtrl);
    u8 oldPhyTxPwr  = pPclCb->usedPhyTxPwr;

    if (phy == BLE_PC_PHY_NONE) {
        phy = currTxPwrPhy;
    }

    u8 adjNewPhyTxLvl = blt_ll_pclGetRfActualTxPwr(txPwrLvl, FALSE);

    if (oldPhyTxPwr == adjNewPhyTxLvl) {
        return BLE_SUCCESS;
    } else {
        s8 delta     = adjNewPhyTxLvl - oldPhyTxPwr;
        u8 limitInfo = blt_ll_pclCalcPwrLimitInfo(adjNewPhyTxLvl);
        if (phy == currTxPwrPhy) {
            pPclCb->usedPhyTxPwr = adjNewPhyTxLvl; //update
            my_dump_str_data(DBG_LL_PCL_EN, "usedPhyTxPwr2", &pPclCb->usedPhyTxPwr, 1);
            pc->currRfPwrIdx = rf_ble_get_tx_pwr_idx(adjNewPhyTxLvl);

            if (LL_FEATURE_MASK_1 & LL_FEATURE_MASK_LE_POWER_CHANGE_INDICATION) {
                /* NOTICE: phy type: refer to 'le_phy_type_t' */
                u8 lePhy = pc->connPhyCtrl.conn_cur_phy;
                if (blt_ll_pclSendChgIndProc(connHandle, lePhy, limitInfo, delta, adjNewPhyTxLvl, FALSE) != BLE_SUCCESS) {
                    /* send PCL_PWR_CHG_IND failed */
                }
            }
        }

        pPclCb->phyTxPwrLvl[phy - 1] = adjNewPhyTxLvl;

        if (pPclCb->pwrRptLocal) {
            // reporting to the local Host of transmit power level changes in the local and
            //remote Controllers for the ACL connection identified by the Connection_Handle parameter
            if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_TRANSMIT_POWER_REPORTING) {
                my_dump_str_data(DBG_LL_PCL_EN, ">>hci_le_transmitPwrRpting_evt1005", 0, 0);
                hci_le_transmitPwrRpting_evt(BLE_SUCCESS, pc->acl_conHandle, LL_PWR_CTRL_RPTING_REASON_LOCAL, currTxPwrPhy, adjNewPhyTxLvl, limitInfo, delta);
            }
        }
    }

    return BLE_SUCCESS;
}
    #endif

    #if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
    #endif
    void
    blt_ll_pclInitParamsAftAclConnect(st_ll_conn_t *pAclConn) //called by blms_connect_common in IRQ
{
    /* Initialize PCL_CB Pointer */
    u8           conn_idx = pAclConn->acl_conIndex;
    ll_pcl_cb_t *pPclCb   = blt_ll_findAvailablePcl(conn_idx);
    pAclConn->pPclCb      = pPclCb;
    if (pPclCb == NULL) {
        LL_FEATURE_MASK_1 &= ~(LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST << 1);
        LL_FEATURE_MASK_1 &= ~(LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST << 2);
        LL_FEATURE_MASK_1 &= ~(LL_FEATURE_ENABLE_POWER_LOSS_MONITORING << 3);
        my_dump_str_data(DBG_LL_PCL_EN, "PCL feature not supported", 0, 0);
        return;
    } else {
        LL_FEATURE_MASK_1 |= (LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST << 1);
        LL_FEATURE_MASK_1 |= (LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST << 2);
        LL_FEATURE_MASK_1 |= (LL_FEATURE_ENABLE_POWER_LOSS_MONITORING << 3);
    }

    u8 phyTxPwrIdx = 0;
    u8 suppPhyBits = BLE_PC_1M_BIT;
    if ((LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_2M_PHY << 8))) {
        suppPhyBits |= BLE_PC_2M_BIT;
    }
    if ((LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_CODED_PHY << 11))) {
        //      ll_conn_phy_t* pConnPhy = &pAclConn->connPhyCtrl;
        //      suppPhyBits |=  pConnPhy->conn_cur_CI == LE_CODED_S8 ? BLE_PC_CODED_S8_BIT : BLE_PC_CODED_S2_BIT;
        suppPhyBits |= BLE_PC_CODED_S8_BIT | BLE_PC_CODED_S2_BIT; //both supported
    }

    /*
     * If a device starts to manage the power level for a PHY (e.g. because it has
     * become an active PHY) then the implementation shall choose an initial power level.
     */

    pPclCb->peerTxPwrLvl = LL_PWR_CTRL_TXPWR_UNAVA;                         //default, need to known by read or request
    for (u8 phy = BLE_PC_1M_BIT; phy <= BLE_PC_MAX_BIT; phy = phy << 1) {
        if (suppPhyBits & phy) {
            pPclCb->phyTxPwrLvl[phyTxPwrIdx++] = LL_PWR_CTRL_TXPWR_UNMNGED; //PHY available, but not management
        } else {
            pPclCb->phyTxPwrLvl[phyTxPwrIdx++] = LL_PWR_CTRL_TXPWR_UNAVA;   //PHY not available
        }
    }

    pPclCb->peerLimitInfo = 0;
    pPclCb->pwrRptRemote  = 0;
    pPclCb->pwrRptLocal   = 0;

    /* important!!! update by API: rf_set_power_level_index */
    extern unsigned char txPower_index;
    s8                   rfTxPower = rf_ble_get_tx_pwr_level(txPower_index);
    /* re-map from users setting */ /* update rfPwrLvlIdx */
    blmsParam.dftTxPwrLvl = rf_ble_get_tx_pwr_idx(rfTxPower);

    pPclCb->usedPhyTxPwr = blmsParam.dftTxPwrLvl;
    my_dump_str_data(DBG_LL_PCL_EN, "usedPhyTxPwr3", &pPclCb->usedPhyTxPwr, 1);
    pAclConn->currRfPwrIdx = rf_ble_get_tx_pwr_idx(blmsParam.dftTxPwrLvl);

    pAclConn->rssi = LL_RSSI_METRIC_VALUE; //default: RSSI can not be read

    //Path loss reporting is disabled when the connection is first created.
    pPclCb->pathLossRptState = LL_PWR_CTRL_PATHLOSS_RPTING_DISABLED;

    //Refer to <<LL_TS.p19.pdf>>,Figure 4.602: [Power Control Request C Initiate], Page 1222
    /* Power Control Request C Auto Initiate LL_PC_REQ concerned parameters initialization.
     *Refer to <<Core5.3 | Vol 6, Part A>>,Page 2639
     * When the LE Power Control Request feature is used on
     * a connection with long connection intervals, devices should use reliable RSSI
     * measurements from recent connection events to determine whether or not to
     * send power control requests. When a device is capable of adjusting its transmit
     * power level using the LE Power Control Request feature, the difference
     * between any two adjacent transmit power levels supported by the radio design
     * should be no greater than 8 dB.
     * */
    if (LL_FEATURE_MASK_1 & LL_FEATURE_MASK_LE_POWER_CTRL_REQUEST) { //needless current, here must be TRUE.
        //These parameter values are defined by Vendor-Special.
        pPclCb->autoMonitor.highThreshold       = LL_PWR_CTRL_AUTO_RSSI_HIGH_THRESHOLD;
        pPclCb->autoMonitor.lowThreshold        = LL_PWR_CTRL_AUTO_RSSI_LOW_THRESHOLD;
        pPclCb->autoMonitor.minTimeSpent        = LL_PWR_CTRL_AUTO_MIN_TIME;
        pPclCb->autoMonitor.reqDelta            = LL_PWR_CTRL_AUTO_REQUEST_VAL;
        pPclCb->autoMonitor.curTimeSpent        = 0;
        pPclCb->pcl_cis_curTimeSpent            = 0;
        pPclCb->autoMonitor.sendPcReqIrqPending = 0;
        //Controller need auto Initiate LL_PC_REQ according to the RSSI
        pPclCb->autoMinitorState = LL_PWR_CTRL_AUTO_MONITORING_ENABLED; //After the ACL connection connected, enable this feature immediately.
    }
}

ble_sts_t blc_ll_readRssi(u16 connHandle, s8 *pOutRssi)
{
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    st_ll_conn_t *pc = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);

    /*
     * For an LE transport, a Connection_Handle is used as the Handle command
     * parameter and return parameter. The meaning of the RSSI metric is an
     * absolute receiver signal strength value in dBm to 6 dB accuracy.
     */
    assert(pOutRssi);

    *pOutRssi = pc->rssi + blt_ll_getRfRxPathComp(); /* If the RSSI cannot be read, the RSSI metric shall be set to 127. */

    return BLE_SUCCESS;
}

    #if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
    #endif
    void
    blt_ll_pclAutoInitiateReqProc(st_ll_conn_t *pAclConn, ll_cis_conn_t *pCisConn) //called in
{
    /* if peer ll_feature dosen't support PCL_req, we don't need to auto initiate ll_pcl_req */
    if (!(pAclConn->ll_remoteFeature1 & LL_FEATURE_MASK_LE_POWER_CTRL_REQUEST)) {
        return;
    }

    u8           txPwrLvlPhy;
    s8           currRcvdRssi;
    u8          *pCurTimeSpent = NULL;
    ll_pcl_cb_t *pPclCb        = pAclConn->pPclCb;
    if (pPclCb == NULL) {
        return;
    }

    if (pCisConn != NULL) { //cis phy
        currRcvdRssi = pCisConn->rssi;
    #if (DBG_LL_PCL_EN)
        s8        temp        = currRcvdRssi < 0 ? -currRcvdRssi : currRcvdRssi;
        static s8 lastCisRssi = 0;
        if (lastCisRssi != pCisConn->rssi) {
            my_dump_str_data(DBG_LL_PCL_EN, "RSSI[CIS]", &pCisConn->rssi, 1);
        }
        lastCisRssi = pCisConn->rssi;
    #endif
        //if use codedPHY, CI: S8 we fixed, not need to re-map txPwrLvlPhy
        txPwrLvlPhy = pCisConn->curCisPhy;

        //Notice: called in cis rx IRQ, blt_pCisConn can be used here
        /*  coded PHY patterns (S8/S2: S8/S8: S2/S8: S2/S2) are all allowed, we need to known peer's CI's value: S2 or S8 */
        if (1 && txPwrLvlPhy == BLE_PC_PHY_CODED_S8 && pCisConn->peer_coded_phy_ci == LE_CODED_S2) {
            txPwrLvlPhy = BLE_PC_PHY_CODED_S2;
            my_dump_str_data(DBG_LL_PCL_EN && 0, "<<<peer use S2>>>", 0, 0);
        }

        pCurTimeSpent = &pPclCb->pcl_cis_curTimeSpent;
    } else { //acl_phy
        currRcvdRssi = pAclConn->rssi;
    #if (DBG_LL_PCL_EN)
        s8        temp        = currRcvdRssi < 0 ? -currRcvdRssi : currRcvdRssi;
        static s8 lastAclRssi = 0;
        if (lastAclRssi != pAclConn->rssi) {
            my_dump_str_data(DBG_LL_PCL_EN, "RSSI[ACL]", &pAclConn->rssi, 1);
        }
        lastAclRssi = pAclConn->rssi;
    #endif

        pCurTimeSpent = &pPclCb->autoMonitor.curTimeSpent;
        txPwrLvlPhy   = pAclConn->connPhyCtrl.conn_cur_phy;
        if (txPwrLvlPhy == BLE_PHY_CODED && pAclConn->connPhyCtrl.conn_cur_CI == LE_CODED_S2) {
            txPwrLvlPhy = txPwrLvlPhy + 1; //S8:3;  S2:4
        }

        /*  coded PHY patterns (S8/S2: S8/S8: S2/S8: S2/S2) are all allowed, we need to known peer's CI's value: S2 or S8 */
        if (1 && txPwrLvlPhy == BLE_PC_PHY_CODED_S8 && pAclConn->peer_coded_phy_ci == LE_CODED_S2) {
            txPwrLvlPhy = BLE_PC_PHY_CODED_S2;
            my_dump_str_data(DBG_LL_PCL_EN && 0, "<<<peer use S2>>>", 0, 0);
        }
    }

    if (pPclCb->autoMinitorState && currRcvdRssi != LL_RSSI_METRIC_VALUE) {
        s8 reqDelta = 0;
        if (currRcvdRssi < pPclCb->autoMonitor.lowThreshold) { //lower
            /* After at least consecutive 'minTimeSpent' connection events, send a 'LL_PC_REQ' request */
            if (++(*pCurTimeSpent) > pPclCb->autoMonitor.minTimeSpent) {
                (*pCurTimeSpent) = 0;
                if (!(pPclCb->peerLimitInfo & LL_PWR_CTRL_LIMIT_MAX_BIT)) {
                    //RSSI too low, increase delta in LL_PC_REQ to increase the power
                    reqDelta = pPclCb->autoMonitor.reqDelta; //+
                    my_dump_str_data(DBG_LL_PCL_EN, "lowThreshold -60", &currRcvdRssi, 1);
                } else {
                    my_dump_str_data(DBG_LL_PCL_EN, "[Auto]peer limit max bit", 0, 0);
                }
            }
        } else if (currRcvdRssi > pPclCb->autoMonitor.highThreshold) { //higher
            /* After at least consecutive 'minTimeSpent' connection events, send a 'LL_PC_REQ' request */
            if (++(*pCurTimeSpent) > pPclCb->autoMonitor.minTimeSpent) {
                (*pCurTimeSpent) = 0;
                if (!(pPclCb->peerLimitInfo & LL_PWR_CTRL_LIMIT_MIN_BIT)) {
                    //RSSI too high, decrease delta in LL_PC_REQ to decrease the power
                    reqDelta = -pPclCb->autoMonitor.reqDelta; //-
                    my_dump_str_data(DBG_LL_PCL_EN, "highThreshold -38", &currRcvdRssi, 1);
                } else {
                    my_dump_str_data(DBG_LL_PCL_EN, "[Auto]peer limit min bit", 0, 0);
                }
            }
        } else {
            (*pCurTimeSpent) = 0;
        }

        /* Already send PCL_REQ OR delta == 0, do nothing */
        if (reqDelta && !pPclCb->pc_sendReq && !pPclCb->autoMonitor.sendPcReqIrqPending) {
            pPclCb->autoMonitor.sendDeltaMark       = reqDelta;
            pPclCb->autoMonitor.sendPcReqIrqPending = 1;
            pPclCb->autoMonitor.sendPcReqTxPwrPhy   = txPwrLvlPhy; //refer to 'pc_phy_t'
            my_dump_str_data(DBG_LL_PCL_EN, ",,,AutoInitiateReq,,,", &txPwrLvlPhy, 1);
        }
    }
}

    #if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
    #endif
    void
    blt_ll_pclPathLossMonitorProc(st_ll_conn_t *pAclConn) //called in ACL_xxx irq
{
    ll_pcl_cb_t *pPclCb = pAclConn->pPclCb;
    if (pPclCb == NULL) {
        return;
    }

    /* path loss monitoring is unavailable, skip monitoring process */
    if (pPclCb->peerTxPwrLvl == LL_PWR_CTRL_TXPWR_UNAVA) {
        return;
    }

    if (pPclCb->pathLossRptState == LL_PWR_CTRL_PATHLOSS_RPTING_ENABLED) {
        /* Waiting for LL_PC_REQ/RSP exchange finished, skip monitoring. */
        if (pPclCb->pathLoss.sendReq2StartMonitoring) {
            my_dump_str_data(DBG_LL_PCL_EN, "Waiting for LL_PCL_REQ/RSP exchange finished, skip monitoring", 0, 0);
            return;
        }

        u8  curZone      = pPclCb->pathLoss.curZone;
        u8  newZone      = blt_ll_pclCalcPathLossZone(pAclConn);
        u8 *pLastNewZone = &pPclCb->pathLoss.newZone;

        u8 PathLoss = blt_ll_pclCalcPathLoss(pAclConn);
        my_dump_str_data(DBG_LL_PCL_EN, "%%%PathLoss%%%", &PathLoss, 1);
        my_dump_str_data(DBG_LL_PCL_EN, "irq: curZone", &curZone, 1);
        my_dump_str_data(DBG_LL_PCL_EN, "irq: newZone", &newZone, 1);
        my_dump_str_data(DBG_LL_PCL_EN, "irq: lastNewZone", pLastNewZone, 1);

        if (newZone == curZone || (*pLastNewZone != newZone)) {
            pPclCb->pathLoss.curTimeSpent = 0;
            *pLastNewZone                 = newZone;
            my_dump_str_data(DBG_LL_PCL_EN, "irq: curTimeSpent=0", &newZone, 1);
            return;
        } else {
            my_dump_str_data(DBG_LL_PCL_EN, "irq: peerTXLvl", &pPclCb->peerTxPwrLvl, 1);
            my_dump_str_data(DBG_LL_PCL_EN, "irq: rcvd RSSI", &pAclConn->rssi, 1);
        }


        /* After at least consecutive 'minTimeSpent' connection events, report a PathLoss rpt event. */
        if (++pPclCb->pathLoss.curTimeSpent >= pPclCb->pathLoss.minTimeSpent) {
            pPclCb->pathLoss.curTimeSpent = 0;
            pPclCb->pathLoss.curZone      = newZone; //Update new zone

            //Path loss monitoring report event mark, do this in loop
            pPclCb->pathLoss.curPathLoss = blt_ll_pclCalcPathLoss(pAclConn);
            my_dump_str_data(DBG_LL_PCL_EN, "curPathLoss2", &pPclCb->pathLoss.curPathLoss, 1);
            pPclCb->pathLoss.pathLossRptEvtIrqPending = 1;
        }
    }
}

    #if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
    #endif
    void
    blt_ll_pclPwrChgIndAftPhyUpt(st_ll_conn_t *pAclConn) //called in irq: blms_start_common_1
{
    assert(pAclConn != NULL);
    ll_pcl_cb_t *pPclCb = pAclConn->pPclCb;
    if (pPclCb == NULL) {
        return;
    }

    ll_conn_phy_t *pConnPhy = &pAclConn->connPhyCtrl;
    /* next PHY will be used immediately */
    if (pConnPhy->conn_cur_phy != pConnPhy->conn_next_phy) {
        /* Switch from 'le_phy_type_t' to 'pc_phy_t' */
        u8 txPhy = pConnPhy->conn_next_phy;
        if (pConnPhy->conn_next_phy == BLE_PHY_CODED && pConnPhy->conn_next_CI == LE_CODED_S2) {
            txPhy = txPhy + 1; //S8:3;   S2:4
        }

        //Get selected next PHY's Tx power level
        u8 txPhyPwrLvl = pPclCb->phyTxPwrLvl[txPhy - 1];
        /* If a device starts to manage the power level for a PHY (e.g. because it has
         * become an active PHY) then the implementation shall choose an initial power level. */
        if (txPhyPwrLvl == LL_PWR_CTRL_TXPWR_UNMNGED) {
            txPhyPwrLvl                    = pPclCb->usedPhyTxPwr; /* here we use current ACL tx_pwr_lvl */
            pPclCb->phyTxPwrLvl[txPhy - 1] = txPhyPwrLvl;
            /*  the coded PHY(S2/S8) should use the same power level. */
            if (pConnPhy->conn_next_phy == BLE_PHY_CODED) {
                pPclCb->phyTxPwrLvl[BLE_PC_PHY_CODED_S8 - 1] = txPhyPwrLvl;
                pPclCb->phyTxPwrLvl[BLE_PC_PHY_CODED_S2 - 1] = txPhyPwrLvl;
            }
            /* Careful here: we pending this, send LL_PCL_CHG_IND in main_loop. */
            if (LL_FEATURE_MASK_1 & LL_FEATURE_MASK_LE_POWER_CHANGE_INDICATION) {
                pPclCb->pcl_chg_ind_irq_pending = 1;
                /* NOTICE: phy type: refer to 'le_phy_type_t' */
                pPclCb->pcl_chg_lephy  = pAclConn->connPhyCtrl.conn_next_phy;
                pPclCb->pcl_chg_tx_pwr = txPhyPwrLvl;
            }
        }
    }
}


    #if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
    #endif
    void
    blt_ll_pclPwrChgIndAftCisEst(st_ll_conn_t *pAclConn, ll_cis_conn_t *pCisConn) //called in irq: blt_ll_cis_post_common
{
    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    assert(pAclConn != NULL);
    assert(pCisConn != NULL);
    assert(pCisConn->link_acl_index == pAclConn->acl_conIndex);
    ll_pcl_cb_t *pPclCb = pAclConn->pPclCb;
    if (pPclCb == NULL) {
        return;
    }

    if (pPclCb->pcl_chg_cis_use) {
        pPclCb->pcl_chg_cis_use = 0;
        /* Careful here: we pending this, send LL_PCL_CHG_IND in main_loop. */
        pPclCb->pcl_chg_ind_irq_pending = 1;
        /* NOTICE: phy type: refer to 'le_phy_type_t' */
        pPclCb->pcl_chg_lephy  = pCisConn->curCisPhy; /* phy type: le_phy_type_t */
        pPclCb->pcl_chg_tx_pwr = pCisConn->cisUsedTxPwr;
    }

    #endif
}

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    void
    blt_ll_pclInitParamsAftCisConnect(st_ll_conn_t *pAclConn, ll_cis_conn_t *pCisConn) //called in loop: blt_cis_connect_common
{
    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    assert(pAclConn != NULL);
    assert(pCisConn != NULL);
    assert(pCisConn->link_acl_index == pAclConn->acl_conIndex);
    ll_pcl_cb_t *pPclCb = pAclConn->pPclCb;
    if (pPclCb == NULL) {
        return;
    }

    //ll_conn_phy_t* pConnPhy = &pAclConn->connPhyCtrl;

    /* Initialize txPhy and txPower. */
    u8 txPhy = pCisConn->curCisPhy; /* phy type: le_phy_type_t */
        /* Switch from 'le_phy_type_t' to 'pc_phy_t' */
        #if (0)                  //optimized, unused
    u8 own_cis_CI = LE_CODED_S8; /* Our CIS is fixed to use S8 mode in coded PHY mode */
    if (txPhy == BLE_PHY_CODED && own_cis_CI == LE_CODED_S2) {
        txPhy = txPhy + 1;       //S8:3;   S2:4
    }
        #endif

    s8 txPhyPwrLvl = pPclCb->phyTxPwrLvl[txPhy - 1];

    if ((txPhyPwrLvl == LL_PWR_CTRL_TXPWR_UNMNGED)) {
        /* If a device starts to manage the power level for a PHY (e.g. because it has
         * become an active PHY) then the implementation shall choose an initial power level. */
        txPhyPwrLvl                    = pPclCb->usedPhyTxPwr; /* here we use current ACL tx_pwr_lvl */
        pPclCb->phyTxPwrLvl[txPhy - 1] = txPhyPwrLvl;
        /*  the coded PHY(S2/S8) should use the same power level. */
        if (pCisConn->curCisPhy == BLE_PHY_CODED) {
            pPclCb->phyTxPwrLvl[BLE_PC_PHY_CODED_S8 - 1] = txPhyPwrLvl;
            pPclCb->phyTxPwrLvl[BLE_PC_PHY_CODED_S2 - 1] = txPhyPwrLvl;
        }

        if (pPclCb->pc_peerReqRcvd &&
            (LL_FEATURE_MASK_1 & LL_FEATURE_MASK_LE_POWER_CHANGE_INDICATION)) {
            pPclCb->pcl_chg_cis_use = 1;
        }
    }
    /* Use this value to set the HW RF TX_PWR register before cis sending and receiving */
    pCisConn->cisUsedTxPwr = txPhyPwrLvl;
    pCisConn->rfPwrLvlIdx  = rf_ble_get_tx_pwr_idx(txPhyPwrLvl);
    #endif
}

ble_sts_t blc_ll_readEnhancedTxPower(u16 connHandle, u8 phy, s8 *pOutCurTxPwrLvl, s8 *pOutMaxTxPwrLvl)
{
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if (phy == BLE_PC_PHY_NONE || phy > BLE_PC_PHY_TOTAL) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    #if (1) /* The upper caller ensures that the passed parameter is not empty */
    assert(pOutCurTxPwrLvl != NULL);
    assert(pOutMaxTxPwrLvl != NULL);
    #else
    if (pOutCurTxPwrLvl == NULL || pOutMaxTxPwrLvl == NULL) {
        return HCI_ERR_UNSPECIFIED_ERROR; //I added it myself, the spec does not specify.
    }
    #endif

    s8 minTxPwrLvl; //useless
    u8 txPhyPwrLvl;
    blc_ll_readSuppTxPower(&minTxPwrLvl, (s8 *)pOutMaxTxPwrLvl);

    st_ll_conn_t *pc     = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_pcl_cb_t  *pPclCb = pc->pPclCb;
    if (pPclCb == NULL) {
        /* important!!! update by API: rf_set_power_level_index */
        extern unsigned char txPower_index;
        txPhyPwrLvl = txPower_index;
    } else {
        //Get selected PHY's Tx power level
        txPhyPwrLvl = pPclCb->phyTxPwrLvl[phy - 1];
        if (txPhyPwrLvl == LL_PWR_CTRL_TXPWR_UNMNGED) {
            txPhyPwrLvl = pPclCb->usedPhyTxPwr;
        }
    }

    *pOutCurTxPwrLvl = blt_ll_pclGetRfActualTxPwr(txPhyPwrLvl, FALSE);

    return BLE_SUCCESS;
}

ble_sts_t blc_ll_readRemoteTxPwrLvl(u16 connHandle, u8 delta, u8 phy)
{
    /**
     * The Power Control Request procedure, when supported, is used to request a
     * remote Controller to adjust its transmit power level on a specified PHY by a given amount.
     *
     * Either the master or the slave Link Layer may initiate this procedure at any time
     * after entering the Connection State by sending an LL_POWER_CONTROL_REQ PDU
     *
     * The Link Layer can query the current transmit power level and acceptable
     * power reduction of the remote Controller by sending an LL_POWER_CONTROL_REQ
     * PDU with Delta set to zero.
     */

    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    //phy type: refer to 'pc_phy_t': range: 1~4
    if (phy == BLE_PC_PHY_NONE || phy > BLE_PC_PHY_TOTAL) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    // Feature available check
    if (!(LL_FEATURE_MASK_1 & (LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST << 1))) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    st_ll_conn_t *pc = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    //ll_conn_phy_t* pConnPhy = &pc->connPhyCtrl;
    ll_pcl_cb_t *pPclCb = pc->pPclCb;
    assert(pPclCb != NULL);

    if (pPclCb->phyTxPwrLvl[phy - 1] == LL_PWR_CTRL_TXPWR_UNMNGED) {
        pPclCb->phyTxPwrLvl[phy - 1] = pPclCb->usedPhyTxPwr;
    }
    my_dump_str_data(DBG_LL_PCL_EN, "blt_ll_pclSendReqProc2", &phy, 1);

    //Push Power Control Request in tx_fifo
    if (blt_ll_pclSendReqProc(connHandle, phy, delta, pPclCb->phyTxPwrLvl[phy - 1]) == BLE_SUCCESS) {
        pPclCb->rdRemoteTxPwr = TRUE;
    } else {
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }

    return BLE_SUCCESS;
}

ble_sts_t blc_ll_setPathLossRptingParams(u16 connHandle, u8 highThresh, u8 highHyst, u8 lowThresh, u8 lowHyst, u16 minTime)
{
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    // Feature available check
    if (!(LL_FEATURE_MASK_1 & (LL_FEATURE_ENABLE_POWER_LOSS_MONITORING << 3))) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    /*
     * If the Host issues this command with High_Threshold+High_Hysteresis greater
     * than 0xFF or with Low_Threshold less than Low_Hysteresis, the Controller
     * shall return the error code Invalid HCI Command Parameters (0x12).
     */
    if ((((u16)highThresh + (u16)highHyst) > 0x00FF) || (lowThresh < lowHyst)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    /*
     * If the Host issues this command with Low_Threshold greater than
     * High_Threshold or with Low_Threshold+Low_Hysteresis greater than
     * High_ThresholdCHigh_Hysteresis, the Controller shall return the error code
     * Invalid HCI Command Parameters (0x12).
     */
    if ((lowThresh > highThresh) || ((lowThresh + lowHyst) > (highThresh - highHyst))) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    st_ll_conn_t *pc = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    //ll_conn_phy_t* pConnPhy = &pc->connPhyCtrl;
    ll_pcl_cb_t *pPclCb = pc->pPclCb;
    assert(pPclCb == NULL);

    u32 r = irq_disable();
    /*
     * The Min_Time_Spent parameter indicates the minimum time that the Controller
     * shall observe the path loss has crossed the threshold before the Controller
     * generates an event for the threshold crossing. The Host should specify a
     * suitable value based on the connection interval, subrate factor, and Peripheral
     * latency
     */
    smemset(&pPclCb->pathLoss, 0, sizeof(pPclCb->pathLoss));
    pPclCb->pathLoss.highThreshold  = highThresh;
    pPclCb->pathLoss.highHysteresis = highHyst;
    pPclCb->pathLoss.lowThreshold   = lowThresh;
    pPclCb->pathLoss.lowHysteresis  = lowHyst;
    pPclCb->pathLoss.minTimeSpent   = minTime;

    /*
     * If the Host issues this command when path loss monitoring is enabled, and if
     * the new parameters mean that the path loss is now in a different zone, an
     * HCI_LE_Path_Loss_Threshold event shall be generated as soon as possible
     * irrespective of the Min_Time_Spent parameter and the timer shall be reset.
     */
    if (pPclCb->pathLossRptState == LL_PWR_CTRL_PATHLOSS_RPTING_ENABLED) {
        u8 curZone = pPclCb->pathLoss.curZone;
        u8 newZone = blt_ll_pclCalcPathLossZone(pc);
        my_dump_str_data(DBG_LL_PCL_EN, "newZone1", &newZone, 1);
        //TODO: here calculated new Zone, loop do this again, it maybe different value, cause problem.
        if (newZone != curZone) {
            /*
             * If the Host issues this command with High_Threshold parameter set to 0xFF,
             * then the Controller shall not generate an HCI_LE_Path_Loss_Threshold event
             * with Zone_Entered set to 0x02.
             */
            if (highThresh == LL_PWR_CTRL_PATHLOSS_UNUSED_HIGH_THRESHOLD &&
                newZone == LL_PWR_CTRL_PATH_LOSS_ZONE_HIGH) {
                /* shall not generate path loss reporting event */
            } else {
                //Mark path loss monitoring pending event, process evt in the loop.
                pPclCb->pathLoss.pathLossRptEvtloopPending = 1;
            }
        }
    } else {
        /* default path loss zone: middle zone */
        pPclCb->pathLoss.curZone = LL_PWR_CTRL_PATH_LOSS_ZONE_MID;
        pPclCb->pathLossRptState = LL_PWR_CTRL_PATHLOSS_RPTING_READY;
    }

    irq_restore(r);

    return BLE_SUCCESS;
}

_attribute_noinline_
    ble_sts_t
    blc_ll_setPathLossRptingEnable(u16 connHandle, u8 enable)
{
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    // Feature available check
    if (!(LL_FEATURE_MASK_1 & (LL_FEATURE_ENABLE_POWER_LOSS_MONITORING << 3))) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    st_ll_conn_t *pc = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    //ll_conn_phy_t* pConnPhy = &pc->connPhyCtrl;
    ll_pcl_cb_t *pPclCb = pc->pPclCb;
    assert(pPclCb != NULL);

    if (pPclCb->pathLossRptState != LL_PWR_CTRL_PATHLOSS_RPTING_READY) {
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*
     * If the Enable parameter is set to 0x01 and no prior LE Power Control Request
     * procedure has been initiated on the ACL connection, then the Controller may
     * need to initiate a new LE Power Control Request procedure on that ACL
     */
    if (enable && !pPclCb->pc_sendReq && pPclCb->peerTxPwrLvl == LL_PWR_CTRL_TXPWR_UNAVA) {
        u8 currTxPwrPhy = blt_ll_pclGetTxPwrLvlPhyFromConnPhy(&pc->connPhyCtrl);
        my_dump_str_data(DBG_LL_PCL_EN, "blt_ll_pclSendReqProc3", &currTxPwrPhy, 1);

        if (blt_ll_pclSendReqProc(connHandle, currTxPwrPhy, 0, pPclCb->phyTxPwrLvl[currTxPwrPhy - 1]) == BLE_SUCCESS) {
            pPclCb->pathLoss.sendReq2StartMonitoring = 1;
            my_dump_str_data(DBG_LL_PCL_EN, "sendReq2StartMonitoring=1", 0, 0);
        }
    } else {
        //Already LL_PC_REQ/RSP exchanged, peerTxPwrLvl already acquired
        pPclCb->pathLoss.sendReq2StartMonitoring = 0;
        my_dump_str_data(DBG_LL_PCL_EN, "sendReq2StartMonitoring=0", 0, 0);

        //Mark path loss monitoring pending event, process evt in the loop.
        pPclCb->pathLoss.pathLossRptEvtloopPending = 1;
    }

    pPclCb->pathLossRptState = enable ? LL_PWR_CTRL_PATHLOSS_RPTING_ENABLED : LL_PWR_CTRL_PATHLOSS_RPTING_DISABLED;

    return BLE_SUCCESS;
}

_attribute_noinline_
    ble_sts_t
    blc_ll_setTxPwrRptingEnable(u16 connHandle, u8 localEn, u8 remoteEn)
{
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    // Feature available check
    if (!(LL_FEATURE_MASK_1 & (LL_FEATURE_ENABLE_POWER_CONTROL_REQUEST << 1))) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    st_ll_conn_t *pc = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    //ll_conn_phy_t* pConnPhy = &pc->connPhyCtrl;
    ll_pcl_cb_t *pPclCb = pc->pPclCb;
    assert(pPclCb != NULL);

    /*
     * When local reporting is enabled, the Controller shall generate an
     * HCI_LE_Transmit_Power_Reporting event with Reason 0x00 each time the
     * local transmit power level is changed.
     */
    pPclCb->pwrRptLocal = localEn;

    /*
     * When remote reporting is enabled, the Controller shall generate an
     * HCI_LE_Transmit_Power_Reporting event with Reason 0x01 each time it
     * becomes aware that the remote transmit power level has changed.
     */
    pPclCb->pwrRptRemote = remoteEn;

    /*
     * If the Remote_Enable parameter is set to 0x01 and no prior LE Power Control
     * Request procedure has been initiated on the ACL connection, then the
     * Controller shall initiate a new LE Power Control Request procedure on that ACL.
     */
    if (remoteEn && !pPclCb->pc_sendReq && pPclCb->peerTxPwrLvl == LL_PWR_CTRL_TXPWR_UNAVA) {
        u8 currTxPwrPhy = blt_ll_pclGetTxPwrLvlPhyFromConnPhy(&pc->connPhyCtrl);
        my_dump_str_data(DBG_LL_PCL_EN, "blt_ll_pclSendReqProc4", &currTxPwrPhy, 1);

        if (blt_ll_pclSendReqProc(connHandle, currTxPwrPhy, 0, pPclCb->phyTxPwrLvl[currTxPwrPhy - 1]) == BLE_SUCCESS) {
        }
    }

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_readRSSI(hci_readRssi_cmdParam_t *cmdPara, hci_readRssi_retParam_t *retPara)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] HCI_Read_RSSI", 0, 0);

    retPara->connHandle = cmdPara->connHandle;
    retPara->status     = blc_ll_readRssi(cmdPara->connHandle, &retPara->rssi);

    return retPara->status;
}

ble_sts_t blc_hci_le_readEnhancedTxPower(hci_le_rdTxPwrLvlCmdParams_t *cmdPara, hci_le_enRdTxPwrLvlRetParams_t *retPara)
{
    retPara->connHandle = cmdPara->connHandle;
    retPara->phy        = cmdPara->phy;
    retPara->status     = blc_ll_readEnhancedTxPower(cmdPara->connHandle, cmdPara->phy, &retPara->curTxPwrLvl, &retPara->maxTxPwrLvl);

    return retPara->status;
}

ble_sts_t blc_hci_le_readRemoteTxPwrLvl(hci_le_rdTxPwrLvlCmdParams_t *cmdPara)
{
    return blc_ll_readRemoteTxPwrLvl(cmdPara->connHandle, 0, cmdPara->phy);
}

ble_sts_t blc_hci_le_setPathLossRptingParams(hci_le_setPathLossRptingCmdParams_t *cmdPara, hci_le_setPathLossRptingRetParams_t *retPara)
{
    retPara->connHandle = cmdPara->connHandle;
    retPara->status     = blc_ll_setPathLossRptingParams(cmdPara->connHandle, cmdPara->highThresh, cmdPara->highHyst, cmdPara->lowThresh, cmdPara->lowHyst, cmdPara->minTime);

    return retPara->status;
}

ble_sts_t blc_hci_le_setPathLossRptingEnable(hci_le_setPathLossRptingEnCmdParams_t *cmdPara, hci_le_setPathLossRptingEnRetParams_t *retPara)
{
    retPara->connHandle = cmdPara->connHandle;
    retPara->status     = blc_ll_setPathLossRptingEnable(cmdPara->connHandle, cmdPara->enable);

    return retPara->status;
}

ble_sts_t blc_hci_le_setTxPwrRptingEnable(hci_le_setTxPwrRptingEnCmdParams_t *cmdPara, hci_le_setTxPwrRptingEnRetParams_t *retPara)
{
    retPara->connHandle = cmdPara->connHandle;
    retPara->status     = blc_ll_setTxPwrRptingEnable(cmdPara->connHandle, cmdPara->localEn, cmdPara->remoteEn);

    return retPara->status;
}


#endif //end of LL_FEATURE_ENABLE_POWER_CONTROL
