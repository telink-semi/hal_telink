/********************************************************************************************************
 * @file    acl_phy.c
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


#if OS_SUP_EN
    #include "stack/ble/os_sup/os_sup.h"
    #include "stack/ble/os_sup/os_sup_stack.h"
#endif

#ifndef CERT_SCHEME
    #define CERT_SCHEME 0
#endif

#if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)


ble_sts_t blc_ll_readPhy(u16 connHandle, hci_le_readPhyCmd_retParam_t *para)
{
    st_ll_conn_t  *pc       = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_conn_phy_t *pConnPhy = &pc->connPhyCtrl;

    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    para->status    = BLE_SUCCESS;
    para->handle[0] = U16_LO(connHandle); //connection handle 12bits meaningful
    para->handle[1] = U16_HI(connHandle) & 0x0E;
    para->tx_phy    = pConnPhy->conn_cur_phy;
    para->rx_phy    = pConnPhy->conn_cur_phy;

    return BLE_SUCCESS;
}

// if Coded PHY is used, this API set default S2/S8 mode for Connection
ble_sts_t blc_ll_setDefaultConnCodingIndication(le_ci_prefer_t prefer_CI)
{
    if (prefer_CI == CODED_PHY_PREFER_S2) {
        bltPHYs.dft_CI = LE_CODED_S2;
    } else if (prefer_CI == CODED_PHY_PREFER_S8) {
        bltPHYs.dft_CI = LE_CODED_S8;
    }


    return BLE_SUCCESS;
}

//////////////////////////////////////ll phy update////////////////////////////////////////////
ble_sts_t blc_ll_setDefaultPhy(le_phy_prefer_mask_t all_phys, le_phy_prefer_type_t tx_phys, le_phy_prefer_type_t rx_phys) //set for the device
{
    if (all_phys & PHY_TX_NO_PREFER) {
        bltPHYs.dft_tx_prefer_phys = 0;
    } else {
        bltPHYs.dft_tx_prefer_phys = (u8)tx_phys;
    }


    if (all_phys & PHY_RX_NO_PREFER) {
        bltPHYs.dft_rx_prefer_phys = 0;
    } else {
        bltPHYs.dft_rx_prefer_phys = (u8)rx_phys;
    }


    //do not support Asymmetric PHYs, dft_prefer_phy = dft_tx_prefer_phys & dft_rx_prefer_phys
    bltPHYs.dft_prefer_phy = bltPHYs.dft_tx_prefer_phys & bltPHYs.dft_rx_prefer_phys; //
    if (bltPHYs.dft_prefer_phy) {                                                     //at least 1 PHY is selected

        //code below have 2 functions:
        // 1. set "le_phy_type_t" according to "le_phy_prefer_type_t"
        // 2. in case that at least 2 kind of PHYs are preferred. In this situation, we select 1M->2M->Coded by order
        if (bltPHYs.dft_prefer_phy & PHY_PREFER_1M) {
            bltPHYs.dft_prefer_phy = BLE_PHY_1M;
        } else if (bltPHYs.dft_prefer_phy & PHY_PREFER_2M) {
            bltPHYs.dft_prefer_phy = BLE_PHY_2M;
#if LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT
        } else if (bltPHYs.dft_prefer_phy & PHY_PREFER_HDT) {
            bltPHYs.dft_prefer_phy = BLE_PHY_HDT;
#endif
        } else {
            bltPHYs.dft_prefer_phy = BLE_PHY_CODED;
        }
    }

    return BLE_SUCCESS;
}


    #if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
    #endif
    /**********************************************************************************/
    int
    blt_cfg_conn_phy_param(ll_conn_phy_t *pconn_phy, le_phy_type_t curPhy, le_coding_ind_t CI)
{
    pconn_phy->conn_cur_phy       = curPhy;
    pconn_phy->conn_cur_CI        = CI;
    pconn_phy->conn_next_CI       = 0;
    pconn_phy->phy_req_pending    = 0;
    pconn_phy->phy_req_trigger    = 0;
    pconn_phy->phy_update_pending = 0;

    pconn_phy->conn_last_phy = curPhy;

    return 0;
}

ble_sts_t blc_ll_setPhy(u16 connHandle, le_phy_prefer_mask_t all_phys, le_phy_prefer_type_t tx_phys, le_phy_prefer_type_t rx_phys, le_phy_option_prefer_t phy_options)
{
    st_ll_conn_t  *pc       = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_conn_phy_t *pConnPhy = &pc->connPhyCtrl;

    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if (all_phys == 0 && rx_phys != tx_phys) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }
#if LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT
    if ((rx_phys & 0xE8) || (tx_phys & 0xE8)) {
#else
    if ((rx_phys & 0xF8) || (tx_phys & 0xF8)) {
#endif
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    #if CERT_SCHEME
    //1.incline to change phy if a different phy bit is set along with current phy
    //2.anything asymmetric would return unsupported parameter
    volatile u8 tx_preferPhys = 0, rx_preferPhys = 0, comm_phy = 0, asym = 0; //its wired here, only values set normal, or
    //abnormal optimization happens

    if (all_phys == 1 || all_phys == 2) {
        asym = 1;
    } else if (all_phys == 3) {
        rx_preferPhys = tx_preferPhys = 0;
    } else if (all_phys == 0) {
        rx_preferPhys = rx_phys;
        tx_preferPhys = tx_phys;
    }

    if (tx_preferPhys == 0 && rx_preferPhys == 0 && all_phys == 3) {
        comm_phy = bltPHYs.dft_prefer_phy;
    } else if (tx_preferPhys != rx_preferPhys) {
        asym = 1;
    } else if (tx_preferPhys == rx_preferPhys) {
        comm_phy = tx_preferPhys;
    }
    //todo: if certain feature is not supported, then return unsupported parameters

    int cur_PHY_match = comm_phy;
    if (comm_phy && !asym) {
        pConnPhy->conn_prefer_phys = comm_phy;
        pConnPhy->phy_req_pending  = 1;
        pConnPhy->phy_req_trigger  = 1; //mark that Request triggered, it must generate PHY Update Event at last(even no PHY changes)
    } else {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }
    #else
    u8 tx_preferPhys, rx_preferPhys, comm_phy;

    if (all_phys & PHY_TX_NO_PREFER) {
        tx_preferPhys = 0;
    } else {
        tx_preferPhys = (u8)tx_phys;
    }

    if (all_phys & PHY_RX_NO_PREFER) {
        rx_preferPhys = 0;
    } else {
        rx_preferPhys = (u8)rx_phys;
    }


    if (rx_preferPhys != tx_preferPhys) {
        //hci_le_phyUpdateComplete_evt(connHandle, BLE_SUCCESS, pc->connPhyCtrl.conn_cur_phy);
        pc->irq_event1_union.phy_update_evt = 1;
        return BLE_SUCCESS;
    }

    comm_phy = tx_preferPhys & rx_preferPhys; //support symmetric PHYs only

    if (comm_phy == 0) {
        //hci_le_phyUpdateComplete_evt(connHandle, BLE_SUCCESS, pc->connPhyCtrl.conn_cur_phy);
        pc->irq_event1_union.phy_update_evt = 1;
        return BLE_SUCCESS;
    }

    //LL/CON/CEN/BV-42-C -- Expecting LL_PHY_REQ to have exactly one bit set
    if (comm_phy & BIT(BLE_PHY_1M - 1)) {
        comm_phy = BIT(BLE_PHY_1M - 1);
    } else if (comm_phy & BIT(BLE_PHY_2M - 1)) {
        comm_phy = BIT(BLE_PHY_2M - 1);
    } else if (comm_phy & BIT(BLE_PHY_CODED - 1)) {
        comm_phy = BIT(BLE_PHY_CODED - 1);
#if LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT
    } else if (comm_phy & BIT(BLE_PHY_HDT - 1)) {
        comm_phy = BIT(BLE_PHY_HDT - 1);
#endif
    } else {
        comm_phy = 0;
    }

    //no prefer PHYs || current using PHY is among preferred PHYs, PHY Update Complete Event is generated with status "BLE_SUCCESS"
    //NOTE:   PHY_type: 1 2 3,  prefer_PHYs: BIT(0) BIT(1) BIT(2),  so  prefer_PHYs = BIT(PHY_type - 1)
    int cur_PHY_match = comm_phy & BIT(pConnPhy->conn_cur_phy - 1);
    if (!comm_phy || cur_PHY_match) {
        pc->irq_event1_union.phy_update_evt = 1;
        blmsParam.phyupdtEvt_mask |= (1 << (connHandle & CONN_IDX_MASK));
    } else {
        pConnPhy->conn_prefer_phys = comm_phy;
        pConnPhy->phy_req_pending  = 1;
        pConnPhy->phy_req_trigger  = 1; //mark that Request triggered, it must generate PHY Update Event at last(even no PHY changes)
    }
    #endif


    #if 1
    //for both current using PHY is among preferred PHYs(no need PHY Update)   and   PHY Update is needed,
    //if new Coded PHY using, need update coding_ind according to "phy_options"
    if ((comm_phy & PHY_PREFER_CODED) && (phy_options & 0x03)) { // host preferred PHYs include Coded_PHY &&  host has preferred Coding Indication
        le_coding_ind_t new_CI = 0;
        if ((phy_options & 0x03) == CODEDPHY_PREFER_S2) {
            new_CI = LE_CODED_S2;
        } else if ((phy_options & 0x03) == CODEDPHY_PREFER_S8) {
            new_CI = LE_CODED_S8;
        }


        if (cur_PHY_match) {                 //current using PHY is among preferred PHYs
            pConnPhy->conn_cur_CI = new_CI;  //no PHY Update procedure, should update Coding Indication immediately
        } else {
            pConnPhy->conn_next_CI = new_CI; //update Coding Indication when PHY Update procedure complete
        }
    }
    #endif

#if LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT
    if ((comm_phy & PHY_PREFER_HDT) && (phy_options & 0x7c)) { // host preferred PHYs include Coded_PHY &&  host has preferred Coding Indication
        u8 hdt_rate = 0;
        if (phy_options & HDTPHY_PREFER_2M) {
            hdt_rate = HDT2M;
        }
        else if(phy_options & HDTPHY_PREFER_3M){
            hdt_rate = HDT3M;
        }
        else if(phy_options & HDTPHY_PREFER_4M){
            hdt_rate = HDT4M;
        }
        else if(phy_options & HDTPHY_PREFER_6M){
            hdt_rate = HDT6M;
        }
        else if(phy_options & HDTPHY_PREFER_7P5M){
            hdt_rate = HDT7P5M;
        }
        if (cur_PHY_match) {                 //current using PHY is among preferred PHYs
            pConnPhy->conn_cur_hdt_rate = hdt_rate;  //no PHY Update procedure, should update hdt rate immediately
        } else {
            pConnPhy->conn_next_hdt_rate = hdt_rate; //update hdt rate Indication when PHY Update procedure complete
        }
    }

#endif
    pc->ll_upd_flag |= PHY_UPD_FLAG;

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setPhy(hci_le_setPhyCmd_param_t *para) //set for each conn handle
{
    return blc_ll_setPhy(para->connHandle, para->all_phys, para->tx_phys, para->rx_phys, para->phy_options);
}

///change the variable bltData in this function later----qiuwei
_attribute_ram_code_ int blt_ll_updateConnPhy(u16 connHandle) //called in irq: blms_start_common_1
{
    st_ll_conn_t  *pc       = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_conn_phy_t *pConnPhy = &pc->connPhyCtrl;

    // LE Set PHY Command send by Host/Application, PHY Update Event must be generated
    // PHY changed, PHY Update Event must be generated
    #if CERT_SCHEME
    blc_tlkEvent_pending |= EVENT_MASK_PHY_UPDATE;
    if (blt_conn_phy.conn_cur_phy != blt_conn_phy.conn_next_phy) {
        ll_data_extension_t *pExt_data = &pc->ext_data;

        pExt_data->connEffectiveMaxRxOctets = pExt_data->connEffectiveMaxTxOctets = pExt_data->connInitialMaxTxOctets;
        blc_tlkEvent_pending |= EVENT_MASK_DATA_LEN_UPDATE;
    }
    #else
    if (pConnPhy->phy_req_trigger || pConnPhy->conn_cur_phy != pConnPhy->conn_next_phy) {
        pConnPhy->phy_req_trigger           = 0;
        pc->irq_event1_union.phy_update_evt = 1;
        blmsParam.phyupdtEvt_mask |= (1 << (connHandle & CONN_IDX_MASK));

        #if OS_SUP_EN
        if (blt_os_semCountIncrementIrq_cb) {
            blt_os_semCountIncrementIrq_cb();
        }
        #endif
    }
    #endif

    #if (LL_FEATURE_ENABLE_POWER_CONTROL)
    if (ll_acl_pcl_irq_task_cb) {
        ll_acl_pcl_irq_task_cb(blms_conn_sel | FLAG_PCL_PWR_CHG_AFT_PHY_UPT, NULL); //blt_ll_pclInterruptTask
    }
    #endif

    pConnPhy->conn_last_phy = pConnPhy->conn_cur_phy;

    pConnPhy->conn_cur_phy = pConnPhy->conn_next_phy; //new PHY used
    if (pConnPhy->conn_next_CI) {
        pConnPhy->conn_cur_CI  = pConnPhy->conn_next_CI;
        pConnPhy->conn_next_CI = 0;
    }

    //update PDU task when PHY change
    pc->phy_chged = 1;
    #if 1
    pc->pdu_task_us = pdu_27b_tifs_27b_us[pConnPhy->conn_cur_phy - 1][1 || pc->crypt.enable]; //TODO: simplify: calculated as encryption
    #elif 1
    pc->pdu_task_us = pConnPhy->conn_cur_phy == BLE_PHY_CODED ? PAYLOAD_27B_TIFS_27B_ENCRT_CODED_S8_US : PAYLOAD_27B_TIFS_27B_ENCRT_1MPHY_US;
    #else
    if (pConnPhy->conn_cur_phy == BLE_PHY_CODED) {
        if (blms_pconn->crypt.enable) {
            pc->pdu_task_us = PAYLOAD_27B_TIFS_27B_ENCRT_CODED_S8_US;
        } else {
            pc->pdu_task_us = PAYLOAD_27B_TIFS_27B_NOENT_CODED_S8_US;
        }

    } else {
        pc->pdu_task_us = PAYLOAD_27B_TIFS_27B_ENCRT_1MPHY_US;
    }
    #endif

    return 0;
}

_attribute_ram_code_ int blt_ll_switchConnPhy(u16 connHandle)
{
    st_ll_conn_t  *pc       = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_conn_phy_t *pConnPhy = &pc->connPhyCtrl;

    /* can guarantee that ll_phy_switch_cb none zero */
    ll_phy_switch_cb(pConnPhy->conn_cur_phy, pConnPhy->conn_cur_CI); //rf_ble_switch_phy

    return 0;
}

_attribute_noinline_ void blt_ll_sendPhyReq(u16 connHandle)
{
    st_ll_conn_t  *pc       = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_conn_phy_t *pConnPhy = &pc->connPhyCtrl;

    if (pConnPhy->phy_req_pending && !blt_ll_isEncryptionBusy(connHandle)) { //
        u8                       phy_req_dat[6];
        rf_pkt_ll_phy_req_rsp_t *pReq = (rf_pkt_ll_phy_req_rsp_t *)phy_req_dat;
        pReq->llid                    = LLID_CONTROL;
        pReq->rf_len                  = 3;
        pReq->opcode                  = LL_PHY_REQ;
        pReq->tx_phys = pReq->rx_phys = pConnPhy->conn_prefer_phys;

        if (blt_llmsPushLlCtrlPkt(connHandle, LL_PHY_REQ, phy_req_dat)) {
            pc->ll_rsp_timeout_tick   = clock_time() | 1;
            pConnPhy->phy_req_pending = 0;
        }
    }
}

_attribute_noinline_ void blt_ll_sendPhyUpdateInd(u16 connHandle)
{
    st_ll_conn_t  *pc       = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_conn_phy_t *pConnPhy = &pc->connPhyCtrl;

    if (pConnPhy->phy_update_pending && !blt_ll_isEncryptionBusy(connHandle)) { //  && blttcon.conn_update==0
        u8                          phy_update_dat[8];
        rf_pkt_ll_phy_update_ind_t *pUpdt = (rf_pkt_ll_phy_update_ind_t *)phy_update_dat;
        pUpdt->llid                       = LLID_CONTROL;
        pUpdt->rf_len                     = 5;
        pUpdt->opcode                     = LL_PHY_UPDATE_IND;
        pUpdt->m_to_s_phy = pUpdt->s_to_m_phy = pConnPhy->conn_updatePhy;

        u16 connInst_next = pc->conn_inst + 20 + pc->conn_latency;

        if (pUpdt->s_to_m_phy == 0) {
            connInst_next = 0;
        }

        pUpdt->instant0 = U16_LO(connInst_next);
        pUpdt->instant1 = U16_HI(connInst_next);

        if (blt_llmsPushLlCtrlPkt(connHandle, LL_PHY_UPDATE_IND, phy_update_dat)) { //connHandle no use, due to single connection
            pConnPhy->phy_update_pending = 0;
            pc->ll_rsp_timeout_tick      = 0;
            pc->conn_phy_inst_next       = connInst_next;

            if (pUpdt->s_to_m_phy == 0) {
                pc->conn_update_union.update_mark &= ~CONN_PHY_UPDATE_IND_CMD;
                if (pc->ll_upd_flag & PHY_UPD_FLAG) { //PHY_UPD_FLAG
                    pc->irq_event1_union.phy_update_evt = 1;
                    pc->ll_upd_flag &= ~PHY_UPD_FLAG;
                    pc->llcp_flag.bit.ll_phy_ind_send_flag = 0;
                    pc->llcp_flag.bit.ll_phy_req_rcvd_flag = 0;
                }
            } else {
                pc->conn_update_union.update_mark |= CONN_PHY_UPDATE_IND_CMD;
                pc->llcp_flag.bit.ll_phy_ind_send_flag = 1;
            }
        }
    }
}


_attribute_noinline_ void blt_ll_sendPhyUpdateInd_V2(u16 connHandle)
{
    st_ll_conn_t  *pc       = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_conn_phy_t *pConnPhy = &pc->connPhyCtrl;

    if (pConnPhy->phy_update_pending && !blt_ll_isEncryptionBusy(connHandle)) { //  && blttcon.conn_update==0
        u8                          phy_update_dat[8];
        rf_pkt_ll_phy_update_ind_v2_t *pUpdt = (rf_pkt_ll_phy_update_ind_v2_t *)phy_update_dat;
        pUpdt->llid                       = LLID_CONTROL;
        pUpdt->rf_len                     = 10;
        pUpdt->opcode                     = LL_PHY_UPDATE_IND_V2;
        pUpdt->m_to_s_phy = pUpdt->s_to_m_phy = pConnPhy->conn_updatePhy;

        u16 connInst_next = pc->conn_inst + 20 + pc->conn_latency;

        if (pUpdt->s_to_m_phy == 0) {
            connInst_next = 0;
        }

        pUpdt->instant0 = U16_LO(connInst_next);
        pUpdt->instant1 = U16_HI(connInst_next);

        if (blt_llmsPushLlCtrlPkt(connHandle, LL_PHY_UPDATE_IND_V2, phy_update_dat)) { //connHandle no use, due to single connection
            pConnPhy->phy_update_pending = 0;
            pc->ll_rsp_timeout_tick      = 0;
            pc->conn_phy_inst_next       = connInst_next;

            if (pUpdt->s_to_m_phy == 0) {
                pc->conn_update_union.update_mark &= ~CONN_PHY_UPDATE_IND_CMD;
                if (pc->ll_upd_flag & PHY_UPD_FLAG) { //PHY_UPD_FLAG
                    pc->irq_event1_union.phy_update_evt = 1;
                    pc->ll_upd_flag &= ~PHY_UPD_FLAG;
                    pc->llcp_flag.bit.ll_phy_ind_send_flag = 0;
                    pc->llcp_flag.bit.ll_phy_req_rcvd_flag = 0;
                }
            } else {
                pc->conn_update_union.update_mark |= CONN_PHY_UPDATE_IND_CMD;
                pc->llcp_flag.bit.ll_phy_ind_send_flag = 1;
            }
        }
    }
}
#endif // end of (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
