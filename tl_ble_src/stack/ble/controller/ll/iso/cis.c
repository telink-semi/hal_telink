/********************************************************************************************************
 * @file    cis.c
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


#if (LL_FEATURE_ENABLE_CONNECTED_ISO)

_attribute_aligned_(4) ll_cis_mng_t bltCisMng;
_attribute_aligned_(4) cis_conn_para_t cisConn_param;
cis_tx_pdu_fifo_t bltCisPduTxfifo;
iso_rx_pdu_fifo_t bltCisPduRxfifo;

_attribute_ble_data_retention_ blt_ll_pushIsoDataFun blt_ll_pushCisDataFun = NULL;


_attribute_aligned_(4) ll_cis_conn_t *global_pCisConn = NULL; //global CIS connection parameter data pointer
_attribute_aligned_(4) ll_cis_conn_t *blt_pCisConn    = NULL;


_attribute_aligned_(4) st_ll_conn_t *blt_pAclConn;

_attribute_aligned_(4) cis_rxEvt_fifo_t bltCisRxEvt;

rf_packet_ll_data_t *pCurrCisPdu;

//u8 gCisNullPdu[6] = {0x01, 0, 0x80, 0, 0x40, 0};  //dma_len = 2, <6>: NPI = 1
rf_packet_ll_data_t gCisNullPdu = {
    .dma_len                                = rf_tx_packet_dma_len(2),
    .llPhysChnPdu.llPduHdr.cisPduHdr.npi    = 1,
    .llPhysChnPdu.llPduHdr.cisPduHdr.sn     = 0,
    .llPhysChnPdu.llPduHdr.cisPduHdr.rf_len = 0,
};


rf_packet_ll_data_t gCisEmptyPdu = {
    .dma_len                                = rf_tx_packet_dma_len(2),
    .llPhysChnPdu.llPduHdr.cisPduHdr.npi    = 0,
    .llPhysChnPdu.llPduHdr.cisPduHdr.sn     = 0,
    .llPhysChnPdu.llPduHdr.cisPduHdr.rf_len = 0,
};


//ll_cis_conn_t     AA_cis[2];  //just for debug

    #define CIS_RX_EVT_FIFO_SIZE sizeof(iso_rx_evt_t)
    #define CIS_RX_EVT_FIFO_NUM  32

/**
 * @brief   CIS RX evt buffer. size & number defined in app_buffer.h
 * CIS RX EVT FIFO is shared by all connections to hold LinkLayer RF RX ISO data, user should define this buffer
 * if either CIS connection master role or CIS connection peripheral role is used.
 */
u8 app_cis_rxEvtfifo[CIS_RX_EVT_FIFO_SIZE * CIS_RX_EVT_FIFO_NUM];

ble_sts_t blt_ll_pushCisData(u16 connHandle, iso_pb_flag_t PB_Flag, u8 TS_Flag, u32 time_stamp, u16 seqnum, u16 total_len, u16 cur_len, u8 *pData);


    #if 0 //see sizeof(ll_cis_conn_t) in warning information
    char checker(int);
    char checkSizeOfInt[sizeof(ll_cis_conn_t)]={checker(&checkSizeOfInt)};
    #endif


ble_sts_t blc_ll_initCisConnModule_initCisConnParametersBuffer(u8 *pCisConnParaBuf, u32 cis_cen_num, u32 cis_per_num)
{
    STATIC_ASSERT_FILE(CIS_CONN_PARAM_LENGTH == sizeof(ll_cis_conn_t), cis);
    STATIC_ASSERT_FILE(CIS_TX_PDU_BUF_EXT_LEN == (sizeof(cis_tx_pdu_t) - sizeof(rf_packet_ll_data_t)), cis);

    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_cis_conn_t)), cis);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(cis_conn_para_t)), cis);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(iso_rx_evt_t)), cis);
        //STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(cis_tx_pdu_t)),  cis);// no need 4 align
    #endif


    #if (NOC_ACK_MAXCNT != 4 && NOC_ACK_MAXCNT != 8 && NOC_ACK_MAXCNT != 16)
        #error "NOC_ACK_MAXCNT error !!!"
    #endif


    if (cis_cen_num > (CIS_IN_CIGM_NUM_MAX * LL_CIG_MST_NUM_MAX) ||
        cis_per_num > (LL_CIS_IN_PER_CIG_SLV_NUM_MAX * LL_CIG_SLV_NUM_MAX)) {
        return LL_ERR_INVALID_PARAMETER;
    }


    blc_ll_init2MPhyCodedPhy_feature();               //need 2M/Coded PHY feature

    blc_ll_initChannelSelectionAlgorithm_2_feature(); //need CSA #2

    //CIS need this feature bit enable. for standard controller, bit will be cleared when received HCI_RESET command
    LL_FEATURE_MASK_1 |= LL_FEATURE_MASK_ISOCHRONOUS_CHANNELS;


    //insert here, move to other place later
    ll_cis_conn_irq_task_cb = blt_cis_conn_interrupt_task;
    ll_cis_conn_mlp_task_cb = blt_cis_conn_mainloop_task;
    ll_cis_cmd_task_cb      = blt_cis_cmd_process_task;

    ll_cis_map_update_cb  = blt_cis_update_chn_map;
    blt_ll_pushCisDataFun = blt_ll_pushCisData;


    int cis_conn_num = cis_cen_num + cis_per_num;


    #if 0 //debug
    global_pCisConn = (ll_cis_conn_t *)&AA_cis;
    #else
    global_pCisConn = (ll_cis_conn_t *)pCisConnParaBuf;
    #endif

    blmsParam.cis_en           = 1; //can only use 1 or 0, for "blc_hci_read Local Supported Commands"
    bltCisMng.maxNum_cisConn   = cis_conn_num;
    bltCisMng.maxNum_cisMaster = cis_cen_num;
    bltCisMng.maxNum_cisSlave  = cis_per_num;


    ll_cis_conn_t *pCisConn;
    for (int i = 0; i < cis_conn_num; i++) {
        pCisConn = (ll_cis_conn_t *)(global_pCisConn + i);
        if (i < bltCisMng.maxNum_cisMaster) {
            pCisConn->cisRole = CIS_ROLE_MASTER; //master cis role
        } else {
            pCisConn->cisRole = CIS_ROLE_SLAVE;  //slave cis role
        }

        //set some default value
        pCisConn->cis_index      = i;
        pCisConn->cis_connHandle = BLT_CIS_HANDLE | i; //0x0020,0x0021,0x0022,0x0023

        pCisConn->cis_ID = CIS_ID_INVALID;
    }

    return BLE_SUCCESS;
}

_attribute_ram_code_ int blt_cis_conn_interrupt_task(int flag, void *p)
{
    if (flag == FLAG_IRQ_RX) {
        irq_cis_rx();
    }
    #if 0 //no need TX logic now, so save some RamCode and timing
    else if(flag == FLAG_IRQ_TX){
        irq_cis_tx();
    }
    #endif
    else if (flag == FLAG_CIS_SCHEDULER_TASK) {
        blt_cis_scheduler_task();
    } else if (flag == FLAG_ACL_IRQ_TERMINATE) {
        st_ll_conn_t *pAclConn = (st_ll_conn_t *)p;
        for (int i = 0; i < bltCisMng.maxNum_cisConn; i++) {
            if (pAclConn->cisEstablish_msk & BIT(i)) {
                ll_cis_conn_t *pCisConn = (ll_cis_conn_t *)(global_pCisConn + i);
                if (!pCisConn->cis_termin_union.termin_pack) { //if no previous terminate
                    pCisConn->cis_termin_union.terminate_reason = pAclConn->conn_termin_union.terminate_reason;
                }
            }
        }
    }

    return 0;
}

_attribute_ram_code_ void blt_cis_scheduler_task(void)
{
    if (blmsParam.cig_mas_1st_sche_build_pending && (blmsParam.cis_1st_anchor_bSlot < bltSche.bSlot_endIdx_dft)) {
        //if(ll_cig_mst_irq_task_cb) //not judge, to save RamCode
        {
            tlkapi_send_string_data(DBG_CIS_1ST_AP_TIMING_EN, "[CIS][TIM] cism 1st ap create", 0, 0);
            ll_cig_mst_irq_task_cb(FLAG_SCHEDULE_CIG_SET1ST_AP, NULL); //blt_cig_mst_interrupt_task  blt_cig_mst_set_first_anchor_point
        }
    }

    if (bltSche.task_mask & TSKMSK_CIG_MASTER_ALL) { //attention: if support more than one CIG, should fix
        //if(ll_cig_mst_irq_task_cb) //not judge, to save RamCode
        {
            ll_cig_mst_irq_task_cb(FLAG_SCHEDULE_BUILD, NULL); // blt_cig_mst_interrupt_task()
        }
    }

    if (blmsParam.cig_slv_1st_sche_build_pending && (blmsParam.cis_1st_anchor_bSlot < bltSche.bSlot_endIdx_dft)) {
        //if(ll_cis_slv_irq_task_cb) //not judge, to save RamCode
        {
            tlkapi_send_string_data(DBG_CIS_1ST_AP_TIMING_EN, "[CIS][TIM] ciss 1st ap create, on  new slot map", 0, 0);
            //DBG_C HN12_TOGGLE;
            ll_cis_slv_irq_task_cb(FLAG_SCHEDULE_CIGSLV_GET1ST_AP, NULL); //blt_cig_slv_interrupt_task  blt_ll_calcCigSlv1stAndCis1stAnchorPoint
        }
    }

    if (bltSche.task_mask & TSKMSK_CIG_SLAVE_ALL) { //attention: when support more than one CIS slave, should fix
        //if(ll_cis_slv_irq_task_cb) //not judge, to save RamCode
        {
            ll_cis_slv_irq_task_cb(FLAG_SCHEDULE_CIGSLV_BUILD, NULL); // blt_cig_slv_interrupt_task()
        }
    }
}

    /* (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
 * only "blt_ll_cis_conn_mainloop" need in SRAM, so let this branch be first check with "if"
 * "blt_cis_conn_mainloop_task" not in SRAM to save some SRAM, consider Eagle have a big cache
 */
    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    int
    blt_cis_conn_mainloop_task(int flag, void *p)
{
    if (flag == (int)FLAG_MODULE_MAINLOOP) {
        blt_ll_cis_conn_mainloop();
    } else if (flag == (int)FLAG_CHECK_INIT) {
        return blt_ll_checkCisInit();
    } else if (flag == (int)FLAG_MODULE_RESET) {
        blt_ll_reset_cis_conn();
    } else if (flag == (int)FLAG_ACL_LOCAL_DISCONNECT) {
        //One ACL has 2 or more CISes, if the ACL disconnect, all CISes terminate
        st_ll_conn_t *pAclConn = (st_ll_conn_t *)p;
        for (int i = 0; i < bltCisMng.maxNum_cisConn; i++) {
            if (pAclConn->cisEstablish_msk & BIT(i)) {
                ll_cis_conn_t *pCisConn = (ll_cis_conn_t *)(global_pCisConn + i);
                if (!pCisConn->cis_termin_union.termin_pack) {
                    /*
                    As soon as the Link Layer has received or queued for transmission an
                    LL_TERMINATE_IND PDU all associated CISes shall be considered lost (see
                    Section 4.5.12). The Link Layer shall not send separate LL_CIS_-
                    TERMINATE_IND PDUs when the Host requests termination.
                     */
                    //blt_ll_cis_disconnect(pCisConn, pAclConn->reason_tmp);
                    pCisConn->cis_termin_union.local_terminate = HCI_ERR_CONN_TERM_BY_LOCAL_HOST;
                }
            }
        }
    }

    return 0;
}

init_err_t blt_ll_checkCisInit(void)
{
    if (bltempParam.ll_cisTxFifo_set) {
        if (bltCisPduTxfifo.cis_tx_pdu == NULL) {
            return LL_CIS_TX_BUF_PARAM_INVALID;
        }
    } else {                                                //CIS TX buffer not set
        if (blmsParam.cis_cen_en || blmsParam.cis_per_en) { //CIS master or slave module init_d but CIS TX buffer not set
            return LL_CIS_TX_BUF_NO_INIT;
        }
    }

    if (bltempParam.ll_cisRxFifo_set) {
        if (bltCisPduRxfifo.isoRxPdu == NULL) {
            return LL_CIS_RX_BUF_PARAM_INVALID;
        }
    } else {                                                //CIS RX buffer not set
        if (blmsParam.cis_cen_en || blmsParam.cis_per_en) { //CIS master or slave module init_d but CIS RX buffer not set
            return LL_CIS_RX_BUF_NO_INIT;
        }
    }

    if (bltempParam.ll_cisRxEvtFifo_set) {
        if (bltCisRxEvt.p == NULL) {
            return LL_CIS_RX_EVT_BUF_PARAM_INVALID;
        }
    } else {                                                //CIS RX Event buffer not set
        if (blmsParam.cis_cen_en || blmsParam.cis_per_en) { //CIS master or slave module init_d but CIS RX EVT buffer not set
            return LL_CIS_RX_EVT_BUF_NO_INIT;
        }
    }


    ll_cis_conn_t *pCisConn;
    for (int i = 0; i < bltCisMng.maxNum_cisConn; i++) {
        pCisConn = (ll_cis_conn_t *)(global_pCisConn + i);

        if (sduCisMng.in_fifo_b) {
            pCisConn->cis_sduInBuf = sduCisMng.in_fifo_b + (i * sduCisMng.max_in_fifo_size * sduCisMng.in_fifo_num);
        }
        if (sduCisMng.out_fifo_b) {
            pCisConn->cis_sduOutBuf = sduCisMng.out_fifo_b + (i * sduCisMng.max_out_fifo_size * sduCisMng.out_fifo_num);
        }
        if (bltCisPduTxfifo.cis_tx_pdu) {
            pCisConn->cis_txPduBuf = bltCisPduTxfifo.cis_tx_pdu + (i * bltCisPduTxfifo.fifo_size * bltCisPduTxfifo.fifo_num);
        }
    }


    #if 0 //no use, "blt_iso_proSduPacket" will overwrite "sdu_packet_t"
    if(sduCisMng.out_fifo_b){
        ll_cis_conn_t *pCisConn;

        for(int i=0; i < bltCisMng.maxNum_cisConn; i++){
            pCisConn = (ll_cis_conn_t *) (global_pCisConn + i);
            u8 *out_fifo_addr = sduCisMng.out_fifo_b + (i * sduCisMng.max_out_fifo_size * sduCisMng.out_fifo_num);
            for(int j=0; j < sduCisMng.out_fifo_num; j++){
                sdu_packet_t* pSduOut = (sdu_packet_t*)(out_fifo_addr + j*sduCisMng.max_out_fifo_size );
                pSduOut->isoHandle = pCisConn->cis_connHandle;
                tlkapi_send_string_u32s(0, "0xAAAA0000", pSduOut, &pSduOut->isoHandle, pSduOut->isoHandle, pCisConn->cis_connHandle);
            }
        }
    }
    #endif


    return INIT_SUCCESS;
}

void blt_ll_reset_cis_conn(void)
{
    bltCisMng.curNum_cig_mst   = 0;
    bltCisMng.curNum_cisMaster = 0;
    bltCisMng.curNum_cisSlave  = 0;

    bltCisMng.cisFlow_pending = 0;
    bltCisMng.cisFlow_idx     = INVALID_CIS_IDX;


    blmsParam.cig_slv_1st_sche_build_pending = 0; //fix HCI/CIS/BI-11 after HCI/CIS/BI-09
    blmsParam.cig_mas_1st_sche_build_pending = 0;
    blmsParam.cis_create_pending             = 0;

    cisConn_param.cis_sduDataNum = 0;

    ll_cis_conn_t *pCisConn;
    for (int i = 0; i < bltCisMng.maxNum_cisConn; i++) {
        pCisConn = (ll_cis_conn_t *)(global_pCisConn + i);

        pCisConn->cis_established = 0;

        if (pCisConn->pCisTestParam) {
            pCisConn->pCisTestParam->occupy = 0; //release buffer, maybe other CIS need use
            pCisConn->pCisTestParam         = NULL;
        }

        pCisConn->cis_occupied                 = 0;
        pCisConn->cisFlowFlg                   = CIS_FLOW_IDLE;
        pCisConn->cis_termin_union.termin_pack = 0; //clear: peer_terminate & local_terminate & terminate_reason
        pCisConn->conState                     = CONN_STATUS_DISCONNECT;
        pCisConn->updateMap_cmd                = 0;
        pCisConn->cisSchedFlg                  = 0;
        pCisConn->cis_expect_tick              = 0;


        pCisConn->discon_evt = 0; //if do not clear this, may send a event to host after hci_rest due to IRQ & mainLoop timing delay
        pCisConn->createCmd  = 0; //follow "discon_evt" code: clear for more secure
    }


    #if 0
    sdu_packet_t *pCis_sdu;
    int total_fifo_num = sduCisMng.in_fifo_num * bltCisMng.maxNum_cisConn;
    for(int i=0; i<total_fifo_num; i++ ){
        pCis_sdu = (sdu_packet_t*)(sduCisMng.in_fifo_b + total_fifo_num * sduCisMng.max_in_fifo_size);
        pCis_sdu->sduOffset = 0;
    }
    #endif

    #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER)
    if (ll_cig_mst_mlp_task_cb) {
        ll_cig_mst_mlp_task_cb(FLAG_MODULE_RESET, NULL); //blt_cig_mst_mainloop_task
    }
    #endif


    #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE)
    if (ll_cis_slv_mlp_task_cb) {
        ll_cis_slv_mlp_task_cb(FLAG_MODULE_RESET); //blt_cig_slv_mainloop_task
    }
    #endif
}

/*cis_supplement_strategy: 2, Just insert NULL PDU only
 *                         0, insert Empty PDU
 *                         1, insert NULL PDU when the remaining subEvent more than BN in the CisEvent of payloadNum/BN;
 *                                  when CisEvent of payloadNum/BN has passed
 */
void blc_ll_setCisSupplementPDUStrategy(cis_pdu_strategy_t stgy)
{
    cisConn_param.cis_supplement_strategy = stgy;
}

/**
 * @brief      for user to initialize CIS ISO TX FIFO.
 * @param[in]  pRxbuf - TX FIFO buffer address(Tx buffer must concern all CISes).
 * @param[in]  fifo_size - TX FIFO size, size must be 4*n
 * @param[in]  fifo_number - TX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initCisTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number)
{
    bltempParam.ll_cisTxFifo_set = 1;

    /* number must be 2^n */
    if (IS_POWER_OF_2(fifo_number)) {
        bltCisPduTxfifo.fifo_num  = fifo_number;
        bltCisPduTxfifo.fifo_mask = fifo_number - 1;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    //remove the header offset
    u32 cis_fifo_size = fifo_size - DATA_LENGTH_ALIGN4(sizeof(cis_tx_pdu_t) - sizeof(rf_packet_ll_data_t));

    /* size must be 4*n */
    if ((cis_fifo_size & 3) == 0 && (fifo_size & 3) == 0) {
        bltCisPduTxfifo.fifo_size = fifo_size;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    bltCisPduTxfifo.cis_tx_pdu = pTxbuf;


    tlkapi_send_string_u32s(STACK_DUMP_EN, "CIS TX PDU buffer", fifo_size, cis_fifo_size, fifo_number, 0);

    return BLE_SUCCESS;
}

/**
 * @brief      for user to initialize CIS ISO RX FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size, size must be 4*n
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initCisRxFifo(u8 *pRxbuf, int fifo_size, int fifo_number)
{
    /* CIS RX EVT buffer init */
    blc_ll_initCisRxEvtFifo(app_cis_rxEvtfifo, CIS_RX_EVT_FIFO_SIZE, CIS_RX_EVT_FIFO_NUM);


    bltempParam.ll_cisRxFifo_set = 1;
    bltCisPduRxfifo.isoRxPdu     = NULL;

    /* number must be 2^n */
    if (IS_POWER_OF_2(fifo_number)) {
        bltCisPduRxfifo.fifo_num = fifo_number;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }


    /* size must be 4*n */
    if ((fifo_size & 3) == 0) {
        bltCisPduRxfifo.fifo_size = fifo_size;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    bltCisPduRxfifo.wptr     = 0;
    bltCisPduRxfifo.isoRxPdu = (rf_packet_ll_data_t *)pRxbuf;


    cisConn_param.cis_rx_dma_buff = (u32)pRxbuf;
    cisConn_param.cis_rx_dma_size = (bltCisPduRxfifo.fifo_size >> 4); //todo:optimize later

    tlkapi_send_string_u32s(STACK_DUMP_EN, "CIS RX PDU buffer", pRxbuf, fifo_size, fifo_number, 0);

    return BLE_SUCCESS;
}

/**
 * @brief      This function is used to initialize IAL CIS SDU in and out buffer.
 * @param[in]  in_fifo
 * @param[in]  in_fifo_size
 * @param[in]  in_fifo_num
 * @param[in]  out_fifo
 * @param[in]  out_fifo_size
 * @param[in]  out_fifo_num
 */
void blc_ll_initCisSduBuffer(u8 *in_fifo, int in_fifo_size, u8 in_fifo_num, u8 *out_fifo, int out_fifo_size, u8 out_fifo_num)
{
    sduCisMng.in_fifo_b        = in_fifo;
    sduCisMng.max_in_fifo_size = in_fifo_size;
    sduCisMng.in_fifo_num      = in_fifo_num;
    sduCisMng.in_fifo_mask     = in_fifo_num - 1;

    sduCisMng.out_fifo_b        = out_fifo;
    sduCisMng.max_out_fifo_size = out_fifo_size;
    sduCisMng.out_fifo_num      = out_fifo_num;
    sduCisMng.out_fifo_mask     = out_fifo_num - 1;

    tlkapi_send_string_u32s(STACK_DUMP_EN, "CIS SDU buffer", sduCisMng.max_in_fifo_size, in_fifo_num, sduCisMng.max_out_fifo_size, out_fifo_num);
}

/**
 * @brief      for user to initialize CIS RX EVT FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size, size must be 4*n
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initCisRxEvtFifo(u8 *pRxbuf, int fifo_size, int fifo_number)
{
    STATIC_ASSERT_FILE(CIS_RX_EVT_FIFO_SIZE == sizeof(iso_rx_evt_t), cis);

    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(iso_rx_evt_t)), cis);
    #endif

    bltempParam.ll_cisRxEvtFifo_set = 1;
    bltCisRxEvt.p                   = NULL;

    /* number must be 2^n */
    if (IS_POWER_OF_2(fifo_number)) {
        bltCisRxEvt.num  = fifo_number;
        bltCisRxEvt.mask = fifo_number - 1;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    /* size must be 4*n */
    if ((fifo_size & 3) == 0) {
        bltCisRxEvt.size = fifo_size;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    bltCisRxEvt.wptr = bltCisRxEvt.rptr = 0;
    bltCisRxEvt.p                       = pRxbuf;

    return BLE_SUCCESS;
}

ll_cis_conn_t *blt_ll_findCisByHandle(u16 cisHandle)
{
    for (int i = 0; i < bltCisMng.maxNum_cisConn; i++) {
        if (((global_pCisConn + i)->cis_connHandle == cisHandle) &&
            ((global_pCisConn + i)->cis_occupied)) {
            return (global_pCisConn + i);
        }
    }
    return NULL;
}

int blt_ll_cis_procCisConnectionEvent(u16 cisHandle, ll_cis_conn_t *pCisConn)
{
    //---------- disconnect event ------------------------------------------
    if (pCisConn->cisFlowFlg == CIS_FLOW_IDLE) {
    /* if code enable, test_case LL/CIS/PER/BV-31-C & LL/CIS/PER/BV-32-C pass but report warning:
         * "FlowControl received Hci Host Number Of Completed Packets Parameters Storage Empty for handle 34"
         * When disable, then test_case still pass, without warning.
         * So we think no need release buffer when disconnect, host should release by itself */
    #if 0
            /* release all buffer */
            int numberPkt = 0;
            while(pCisConn->cisNocpRptr != pCisConn->cisNocpWptr){
                numberPkt += pCisConn->cisNocPktCnt[pCisConn->cisNocpRptr++ & NOC_ACK_MASK];
            }

            if(numberPkt){
                hci_numberOfCompletePacket_evt(cisHandle, numberPkt);
                tlkapi_send_string_data(DBG_NUM_COM_PKT, "numComPkt, disconn", &numberPkt, 4);

        #if (DBG_CIS_LOGIC)
                    if(numberPkt > iso_param.iso_buf_num){
                        write_dbg32(0x0018, numberPkt);
                        BLMS_ERR_DEBUG(DBG_CIS_LOGIC, 0x99C00000);
                    }
        #endif
            }
    #endif


        if (hci_eventMask & HCI_EVT_MASK_DISCONNECTION_COMPLETE) {
            hci_disconnectionComplete_evt(BLE_SUCCESS, cisHandle, pCisConn->cis_termin_union.terminate_reason);
        }

        pCisConn->cis_termin_union.terminate_reason = 0;
        pCisConn->discon_evt                        = 0; //must clear it at the end

        pCisConn->createCmd = 0;                         //for master, should clear for all kinds of CIS disconnect and failed to establish

        /* clear some status */
        pCisConn->cis_established = 0;
        if (pCisConn->pCisTestParam) {
            pCisConn->pCisTestParam->occupy = 0; //release buffer, maybe other CIS need use
            pCisConn->pCisTestParam         = NULL;
        }
    }

    return 0;
}

_attribute_noinline_ int blt_cis_connect_common(st_ll_conn_t *pAclConn, ll_cis_conn_t *pCisConn)
{
    pCisConn->cisFlowFlg         = CIS_FLOW_CIS_SYNCING;
    pCisConn->cisEventCnt        = 0;
    pCisConn->cisEvtCnt_initJump = 0;


    /* channel map process */
    //ISO features must support CSA#2
    pCisConn->chnIdentifier = (pCisConn->cisAccessAddr >> 16) ^ (pCisConn->cisAccessAddr & 0xffff);

    // todo FANQH, if have received channel map update,but the connect instant have not pass
    smemcpy(&pCisConn->cis_chnParam.map, &pAclConn->acl_chnParam, sizeof(struct le_channel_map));

    //For every Data Physical Channel PDU and Connected Isochronous PDU, the shift register shall be preset
    //with the CRC initialization value set for the ACL connection and communicated in the CONNECT_IND or AUX_CONNECT_REQ PDU.
    pCisConn->cisCrcInit = pAclConn->aclCrcInit;


    //////////////////////////////////////////////////
    //   cisPayloadNum keep init
    //////////////////////////////////////////////////
    /// init ///
    pCisConn->cisSendPldNum = pCisConn->cisRcvdPldNum = 0;
    pCisConn->txNullPduFlag                           = TRUE;

    if (pCisConn->bn_loca) {
        pCisConn->nse_div_bnLoca = (pCisConn->nse) / pCisConn->bn_loca;
    }
    if (pCisConn->bn_peer) {
        pCisConn->nse_div_bnPeer = (pCisConn->nse) / pCisConn->bn_peer;
    }


    for (int i = 0; i < pCisConn->bn_loca; i++) {
        pCisConn->ftPt_loca[i] = pCisConn->nse - pCisConn->nse_div_bnLoca * (pCisConn->bn_loca - 1 - i);
    }

    for (int i = 0; i < pCisConn->bn_peer; i++) {
        pCisConn->ftPt_peer[i] = pCisConn->nse - pCisConn->nse_div_bnPeer * (pCisConn->bn_peer - 1 - i);
    }


    pCisConn->cis_rx_stream_start = 0;
    pCisConn->cis_tx_stream_start = 0;
    pCisConn->cig_next_tick       = 0;


    pCisConn->conState    = CONN_STATUS_COMPLETE; //complete event, not send to Host
    pCisConn->cis_timeout = pAclConn->conn_timeout;


    bltCisRxEvt.rptr = bltCisRxEvt.wptr = 0;

    //////////////////////////////////////////////////
    // Encryption parameters init
    //////////////////////////////////////////////////
    if (pAclConn->crypt.enable) {
        pCisConn->crypt.enable   = 1;
        pCisConn->crypt.mic_fail = 0;

        smemcpy(pCisConn->crypt.sk, pAclConn->crypt.sk, 16);

        //The IV is common for both Roles of an ACL and CIS.Generation of IV for a CIS or BIS: IV[31:0] shall equal
        //IVbase[31:0] XORed with the Access Address of the CIS or BIS while IV [63:32] shall equal IVbase[63:32].
        u32 iv[2] = {0};
        smemcpy(&iv, pAclConn->crypt.nonce.iv, 8);
        iv[0] ^= pCisConn->cisAccessAddr; //Generation of IV for a CIS or BIS
        smemcpy(pCisConn->crypt.nonce.iv, &iv, 8);

    } else {
        pCisConn->crypt.enable = 0;
    }


    pCisConn->updateMap_cmd = 0;

    /* can not clear terminate_reason, main_loop event callback need use, if new connect too quick, clearing will lead to reason lost */
    pCisConn->cis_termin_union.peer_terminate = pCisConn->cis_termin_union.local_terminate = 0;


    /* in cis_connect we set cis_used_tx_power and check if we need to send pwr_chg_ind. */
    #if (LL_FEATURE_ENABLE_POWER_CONTROL)
    if (ll_acl_pcl_mlp_task_cb) {
        ll_acl_pcl_mlp_task_cb(pCisConn->link_acl_index | FLAG_PCL_INIT_AFT_CIS_CONN, (void *)pCisConn); //blt_ll_pclMainloopTask
    }
    #endif

    //u8 taskOffset = pCisConn->cisRole == CIS_ROLE_MASTER ? (TSKOFT_CIG_MST + pAclConn->alink_cig_idx) : (TSKOFT_CIG_SLV + pAclConn->alink_cig_idx);
    //blt_ll_set_interval_level(taskOffset, pCisConn->iso_intvl_tick/SYSTEM_TIMER_TICK_1250US);
    //blt_ll_setSchedulerTaskPriority( taskOffset, TASK_PRIORITY_CONN_CREATE );

    return 0;
}

/**
 * @brief      This function is used to calculate parameter of cis when cis data path establish.
 * @param[in]  cis_connHandle
 */
void blt_cis_establish_common(ll_cis_conn_t *pCisConn)
{
    pCisConn->cis_established = 1;

    pCisConn->cisSduIn_wptr = pCisConn->cisSduIn_rptr = 0;
    pCisConn->cisSduOut_wptr = pCisConn->cisSduOut_rptr = 0;

    pCisConn->cisPduTxFifoWptr = pCisConn->cisPduTxFifoRptr = 0;


    sdu_packet_t *pCis_sdu;
    for (int i = 0; i < sduCisMng.in_fifo_num; i++) {
        pCis_sdu            = (sdu_packet_t *)(pCisConn->cis_sduInBuf + sduCisMng.max_in_fifo_size * i);
        pCis_sdu->sduOffset = 0;
    }

    #if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
    /* CIS use: send numOfCmpEvt after data transmitted OK or flushed */
    pCisConn->cisNocpWptr = pCisConn->cisNocpRptr = 0;
    smemset4((int *)pCisConn->cisNocTxWptr, 0, NOC_ACK_MAXCNT);
    #endif
    pCisConn->rx_lastPktSeqNum = 0;
    pCisConn->cis_rxSduStatus  = SDU_STATE_NEW;
    pCisConn->lossFlag         = 0;


    pCisConn->tx_lastPktSeqNum = 0;

    pCisConn->tx_lastpldNum = 0;
    pCisConn->tx_first_flag = 1;
    pCisConn->pCisTestParam = NULL;


    pCisConn->tx_numSdu2Pdu = (pCisConn->bn_loca * pCisConn->sdu_int_loca_us) / pCisConn->iso_intvl_us;
    pCisConn->rx_numSdu2Pdu = (pCisConn->bn_peer * pCisConn->sdu_int_peer_us) / pCisConn->iso_intvl_us;


    #if (DBG_CIS_PARAM)
    if ((pCisConn->cis_frame == CIS_UNFRAMED) && (pCisConn->bn_loca) && (pCisConn->tx_numSdu2Pdu == 0)) {
        tlkapi_send_string_u32s(DBG_CIS_PARAM, "[CIS][PAR] tx numSdu2Pdu ERR", pCisConn->bn_loca, pCisConn->sdu_int_loca_us, pCisConn->iso_intvl_us, 0);
        BLMS_ERR_DEBUG(DBG_CIS_LOGIC, 0x99C50000);
    }
    if ((pCisConn->cis_frame == CIS_UNFRAMED) && (pCisConn->bn_peer) && (pCisConn->rx_numSdu2Pdu == 0)) {
        tlkapi_send_string_u32s(DBG_CIS_PARAM, "[CIS][PAR] rx numSdu2Pdu ERR", pCisConn->bn_peer, pCisConn->sdu_int_peer_us, pCisConn->iso_intvl_us, 0);
        BLMS_ERR_DEBUG(DBG_CIS_LOGIC, 0x99C50000);
    }

    tlkapi_send_string_u32s(DBG_CIS_PARAM, "[CIS][PAR] numSdu2Pdu", pCisConn->tx_numSdu2Pdu, pCisConn->rx_numSdu2Pdu, 0, 0);
    #endif
}

//common code for CTX & CRX start, to save ram_code
_attribute_ram_code_ int blt_ll_cis_start_common_1(ll_cis_conn_t *pCisConn)
{
    //1. ACL connection parameters process
    blt_pAclConn = (st_ll_conn_t *)&blms[pCisConn->link_acl_index];


    if (pCisConn->cisSubEventCnt == 0) {
    #if (CIS_ADD_CIE)
        blt_pCisConn->local_cie = blt_pCisConn->bn_loca ? 0 : 1;
        blt_pCisConn->peer_cie  = blt_pCisConn->bn_peer ? 0 : 1;
    #endif

        rf_ble_switch_phy(blt_pCisConn->curCisPhy, LE_CODED_S8);
    }


    pCisConn->cisSubEventCnt++; //SubEventNum increments from 1


    /* special design: for terminate, forbidden TX & RX
     * SRX2TX: first RX timeout trigger
     * TX2RX:  RX timeout trigger */
    if (pCisConn->cis_termin_union.peer_terminate || pCisConn->cis_termin_union.local_terminate) {
        return 0;
    }

    /* code below must be hardware setting, no logic code */

    //2. RF channel select
    struct csa2_param *pChnParam = &pCisConn->cis_chnParam;

    /* process CIS master & slave channel map update */
    if (pCisConn->updateMap_cmd && tick1_exceed_tick2(clock_time(), pCisConn->updateMap_tick)) {
        pCisConn->updateMap_cmd = 0;

        smemcpy(&pChnParam->map, &blt_pAclConn->nextChn, sizeof(struct le_channel_map));
    }


    /* SiHui test 20220720: 48M clock, subEvent 1: 7 uS;  subEvent 2: 19 uS; other subEvent: 5 uS*/
    //DBG_C HN10_HIGH;
    u8 cis_cur_chn = blt_ll_generateNextChannel(pChnParam, pCisConn->cisEventCnt, pCisConn->chnIdentifier, pCisConn->cisSubEventCnt);
    //DBG_C HN10_LOW;


    // RF Hardware register setting
    rf_set_tx_rx_off();
    rf_set_ble_channel(cis_cur_chn);
    rf_set_ble_access_code((u8 *)&pCisConn->cisAccessAddr); //TODO: can use revert value to speed up setting action
    rf_set_ble_crc_value(pCisConn->cisCrcInit);

    blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_PCL, pCisConn->rfPwrLvlIdx);

    #if (LL_FEATURE_ENABLE_LE_CODED_PHY)
    rf_trigger_codedPhy_accesscode();
    #endif


    //Switch DMA RX buffer to CIS dam RX buffer
    ble_rf_set_rx_dma((u8 *)cisConn_param.cis_rx_dma_buff, cisConn_param.cis_rx_dma_size);
    rf_set_rx_maxlen(pCisConn->max_pdu_peer + 4);

    /* Different process for different MCU: ******************************************/
    ble_rf_set_tx_dma(0, 17);
    //  This register exists in Kite/Vulture/826x, riscv architecture chips such as
    //  Eagle do not have this register, but DMA fifo can be turned off by forcibly
    //  setting DMA TX rptr = DMA TX wptr to use DMA default tx fifo.If the hardware
    //  TX rptr of DMA TX fifo == hardware TX wptr, then send DMA Tx default fifo,
    //  otherwise send DMA Tx fifo non-default area.
    HAL_REG_RF_DMA_FIFO_TX_RPTR = FLD_DMA_RPTR_CLR;
    /**********************************************************************************/


    rf_ble_set_rx_settle(RX_SETTLE_US);

    return 0;
}

_attribute_ram_code_ int blt_ll_cis_start_common_2(ll_cis_conn_t *pCisConn)
{
    //prepare cis data to send(CIS NULL PDU or CIS Data PDU).
    blt_ll_cis_prepare_data_common(blt_pCisConn);

    #if (HW_AES_CCM_ALG_EN)
    if (pCisConn->crypt.enable) {
        blt_ll_setAesCcmPara(!blt_pCisConn->cisRole, blt_pCisConn->crypt.sk, blt_pCisConn->crypt.nonce.iv, 0xa3, blt_pCisConn->cisSendPldNum, blt_pCisConn->cisRcvdPldNum, 0);
    }
    #endif

    CLEAR_ALL_RFIRQ_STATUS;               //important: drop boundary RX packet


    if (pCisConn->cisSubEventCnt == 1) {  //first sub_event of current CIS event
        pCisConn->cis_receive_packet = 0; //RX with CRC correct
        pCisConn->cis_1st_rx_tick    = 0;

    #if (SL16_cis0_evtcnt)
        log_b16_irq(SL_STACK_CIS_BASIC_TIMING_EN, (SL16_cis0_evtcnt + ((pCisConn->cisRole) ? (pCisConn->cis_index) : (pCisConn->cis_index - bltCisMng.maxNum_cisMaster))), (u16)pCisConn->cisEventCnt);
    #endif
    }

    //  #if(CIS_ADD_CIE)
    //      pCisConn->cie_flag = pCisConn->peer_cie & pCisConn->local_cie;
    //  #endif

    cisConn_param.cis_rx_num = 0; //RX number (regardless of CRC correct or wrong)

    blmsParam.cis_create_pending &= ~BIT(cisConn_param.blt_cis_sel);

    blmsParam.rf_fsm_busy = 1;

    return 0;
}

/* this function only for CIS create */
void blt_ll_cis_start_jump(ll_cis_conn_t *pCisConn, int jump_num)
{
    pCisConn->cisEventCnt = pCisConn->cisEvtCnt_initJump = jump_num;

    // todo here should think about FT , fanqh
    if (pCisConn->bn_loca) {
        pCisConn->cisSendPldNum = pCisConn->cisEvtCnt_initJump * pCisConn->bn_loca;
    }

    if (pCisConn->bn_peer) {
        pCisConn->cisRcvdPldNum = pCisConn->cisEvtCnt_initJump * pCisConn->bn_peer;

    #if (SL16_cis0_rxPldNum)
        log_b16_irq(SL_STACK_CIS_RX_DATA_EN, SL16_cis0_rxPldNum + ((pCisConn->cisRole) ? (cisConn_param.blt_cis_sel) : (cisConn_param.blt_cis_sel - bltCisMng.maxNum_cisMaster)),
                    (iso_evtcnt_t)pCisConn->cisRcvdPldNum); //
    #endif
    }
}

//common code for CTX & CRX post, to save ram_code
_attribute_ram_code_ void blt_ll_cis_ft_event_jump(ll_cis_conn_t *pCisConn, u32 jumpCisEvtNum)
{
    //  BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99C30000);

    #if (SLEV_cis_jump)
    log_event_irq(SL_STACK_CIS_BASIC_TIMING_EN, SLEV_cis_jump);
        //log_b8_irq(SL_STACK_CIS_BASIC_TIMING_EN, SL08_cis_jump_num, jumpCisEvtNum);
    #endif


    u8 localBN = pCisConn->bn_loca;
    u8 peerBN  = pCisConn->bn_peer;
    u8 localFT = pCisConn->ft_loca;
    u8 peerFT  = pCisConn->ft_peer;

    /* The flush point of a PDU in a burst occurs immediately after U subevents in the CIS event with
     * cisEventCounter equal to (E + FT - 1), where:
     *  E = floor (cisPayloadNumber / BN)
        U = NSE - floor (NSE / BN) * (BN - 1 - cisPayloadNumber mod BN)

     * assume that: last_cisEventCnt = old_cisEventCnt + (jumpCisEvtNum - 1);
     *
     * calculate which cisPayloadNumber flush timeout point is: sub_event "NSE" on last_cisEventCnt
     * when U is "NSE", cisPayloadNumber mod BN must be: "BN - 1"
     *
     * E is: last_cisEvtCnt - (FT - 1) = last_cisEventCnt + 1 - FT  =  old_cisEventCnt + jumpCisEvtNum - FT
     * cisPayloadNumber = E * BN + (BN - 1) = (old_cisEventCnt + jumpCisEvtNum - FT) * BN + (BN - 1)
     * cisPayloadNumber = (old_cisEventCnt + jumpCisEvtNum - FT) * BN + (BN - 1);
     *
     * here old_cisEventCnt = pCisConn->cisEventCnt
     *      new_cisEventCnt = pCisConn->cisEventCnt + jumpCisEvtNum, so
     *
     * cisPayloadNumber = (new_cisEventCnt - FT) * BN + (BN - 1)
     */
    pCisConn->cis_expect_tick += jumpCisEvtNum * pCisConn->iso_intvl_tick;
    pCisConn->cisEventCnt += jumpCisEvtNum;

    //  if(pCisConn->cisEventCnt < localFT ||  pCisConn->cisEventCnt < peerFT ){
    //      BLMS_ERR_DEBUG(DBG_CIS_MASTER_LOGIC, 0x99C40000);
    //  }

    if (localBN) {
        if (pCisConn->cisEventCnt >= localFT) {
            iso_evtcnt_t last_tx_ft_pduNum = (iso_evtcnt_t)(pCisConn->cisEventCnt - localFT) * localBN + localBN - 1;
            if (pCisConn->cisSendPldNum <= last_tx_ft_pduNum) {
                pCisConn->cisSendPldNum = last_tx_ft_pduNum + 1;
                pCisConn->txNullPduFlag = TRUE;
                DBG_FANQH_CHN7_TOGGLE;
    #if (SL16_cis0_txCurSendPldNum)
                log_b16_irq(SL_STACK_CIS_TX_DATA_EN, SL16_cis0_txCurSendPldNum + ((pCisConn->cisRole) ? (pCisConn->cis_index) : (pCisConn->cis_index - bltCisMng.maxNum_cisMaster)), (u16)pCisConn->cisSendPldNum); //+pCisConn->cis_index
    #endif

            } else {
                // mark LastPduType do not change, keep last value
            }
        }
    }


    if (peerBN) {
        if (pCisConn->cisEventCnt >= peerFT) {
            iso_evtcnt_t last_rx_ft_pduNum = (pCisConn->cisEventCnt - peerFT) * peerBN + peerBN - 1;
            if (pCisConn->cisRcvdPldNum <= last_rx_ft_pduNum) { //
                pCisConn->cisRcvdPldNum = last_rx_ft_pduNum;    //last flush PayloadNum
                pCisConn->cis_expect_tick -= pCisConn->iso_intvl_tick;
                pCisConn->cisEventCnt -= 1;

                blt_cis_pushRxEvtInfoToFifo(pCisConn, NULL, last_rx_ft_pduNum - pCisConn->cisRcvdPldNum + 1);
                pCisConn->cisRcvdPldNum++; //the next expect PayloadNum

                DBG_FANQH_CHN1_TOGGLE;
    #if (SL16_cis0_rxPldNum)
                log_b16_irq(SL_STACK_CIS_RX_DATA_EN, SL16_cis0_rxPldNum + ((pCisConn->cisRole) ? (cisConn_param.blt_cis_sel) : (cisConn_param.blt_cis_sel - bltCisMng.maxNum_cisMaster)),
                            (iso_evtcnt_t)pCisConn->cisRcvdPldNum); //
    #endif

                pCisConn->cis_expect_tick += pCisConn->iso_intvl_tick;
                pCisConn->cisEventCnt += 1;
            }
        }
    }
    tlkapi_send_string_u32s(DBG_CIS_TX_DATA_FLOW_EN | DBG_CIS_RX_DATA_FLOW_EN, "Tx&Rx pld ft", pCisConn->cisEventCnt, pCisConn->cisSendPldNum, pCisConn->cisRcvdPldNum, 0);
}

/*
 * U = NSE - NSE/BN * (BN - 1 - payloadNum % BN)
 * for instance NSE = 6, BN = 3
    +----------+-----------+
    | subevent | flush_num |
    +----------+-----------+
    | <2       | 0         |
    | [2,4}    | 1         |
    | [4,6}    | 2         |
    | >=6      | 3         |
    +----------+-----------+
 *
 */
_attribute_ram_code_ int blt_ll_calFt_PDU_num(ll_cis_conn_t *pCisConn, u8 flushPoint[], u8 bn, u8 subEventJumpNum)
{
    u8 curSubEvent = pCisConn->cisSubEventCnt + subEventJumpNum;

    for (int i = 0; i < bn; i++) {
        if ((i == 0) && (curSubEvent < flushPoint[0])) {
            return 0;
        } else if ((i == bn - 1) && (curSubEvent >= flushPoint[i])) {
            return i + 1;
        } else if ((curSubEvent >= flushPoint[i]) && (curSubEvent < flushPoint[i + 1])) {
            return i + 1;
        }
    }

    return 0;
}

/*cis_supplement_strategy:
 *                         0, insert Empty PDU
 *                         1, Just insert NULL PDU only
 *                         2, insert NULL PDU when the remaining subEvent more than BN in the CisEvent of payloadNum/BN;
 *                                  when CisEvent of payloadNum/BN has passed
 */
_attribute_ram_code_ u8 blt_ll_cis_needInsetEmpty(ll_cis_conn_t *pCis)
{
    u8 ret = 0;

    if (cisConn_param.cis_supplement_strategy == CIS_PDU_STRATEGY1) { // strategy2 Just insert NULL PDU
        return 0;
    }
    iso_evtcnt_t cisE = pCis->cisSendPldNum / pCis->bn_loca;
    //strategy 0, insert Empty PDU (default)
    if (cisConn_param.cis_supplement_strategy == CIS_PDU_STRATEGY0) {
        if (cisE <= pCis->cisEventCnt) {
            ret = 1;
        }
    } else //cis_supplement_strategy==CIS_PDU_STRATEGY2
    {
        //Upper-layer data is allowed to arrive only one interval late
        if (cisE < pCis->cisEventCnt) {
            ret = 1;
        }
        //      if(((pCis->cisSendPldNum/pCis->cisEventCnt) == pCis->cisEventCnt) && ((pCis->nse - pCis->cisSubEventCnt+1)  <= pCis->bn_loca))
        //      {
        //          ret = 1;
        //      }
    }

    return ret;
}

_attribute_ram_code_ u8 *blt_ll_cis_prepare_data_common(ll_cis_conn_t *pCisConn)
{
    u8 send_data_pkt = 0;
    pCurrCisPdu      = &gCisNullPdu; //dft: CIS Null PDU, SN and LLID RSVD


    u8 localBN = pCisConn->bn_loca;

    pCisConn->insertEmtpy_flg = 0;

    if (pCisConn->conState == CONN_STATUS_ESTABLISH && localBN) {
        /*
         * 1.The payloadNum corresponding to the current data taken from the buff cannot be smaller than the locally maintained send payloadNum
         * 2.The payloadNum corresponding to the current data taken from the buff cannot be greater than [(current CisEventCnt+1)*BN-1]
         */
        while (pCisConn->cisPduTxFifoWptr != pCisConn->cisPduTxFifoRptr) {
            cis_tx_pdu_t *pCisTxPduPkt = (cis_tx_pdu_t *)(pCisConn->cis_txPduBuf + (pCisConn->cisPduTxFifoRptr & bltCisPduTxfifo.fifo_mask) * bltCisPduTxfifo.fifo_size);
            //cis_pdu_number > (E+1)*BN-1, indicate current PDU should send in next event
            if (pCisTxPduPkt->cis_pdu_number > ((u32)(pCisConn->cisEventCnt + 1)) * localBN - 1) {
                break;
            }
            if (pCisTxPduPkt->cis_pdu_number < pCisConn->cisSendPldNum) {
                //tlkapi_send_string_u32s(0, "PDU 2", pCisTxPduPkt->cis_pdu_number, pCisConn->cisSendPldNum, 0, 0);
                pCisConn->cisPduTxFifoRptr++; //The data has been sent, jump over

            } else if (pCisTxPduPkt->cis_pdu_number == pCisConn->cisSendPldNum) {
                send_data_pkt = 1;
                pCurrCisPdu   = &pCisTxPduPkt->isoTxPdu;

                tlkapi_send_string_u32s(DBG_CIS_TX_DATA_FLOW_EN, "[cis][tx] preData", pCisTxPduPkt->cis_pdu_number, pCisConn->cisPduTxFifoRptr, pCisConn->cisPduTxFifoWptr, pCisConn->cisEventCnt);

                break;
            } else { //It indicates that the intermediate HCI ISO DATA PDU is missing
                //dft send NULL PDU
                break;
            }
        }
    }

    #if (1)
    if ((!send_data_pkt) && (blt_ll_cis_needInsetEmpty(pCisConn))) {
        pCurrCisPdu = &gCisEmptyPdu;

        if (pCisConn->cis_frame == 0) //unframed
        {
            if ((pCisConn->tx_numSdu2Pdu == 1) || (pCisConn->cisSendPldNum % pCisConn->tx_numSdu2Pdu != 0)) {
                //llid =   start/continue
                pCurrCisPdu->llPhysChnPdu.llPduHdr.cisPduHdr.llid = ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU;
            } else {
                //llid= end/complete
                pCurrCisPdu->llPhysChnPdu.llPduHdr.cisPduHdr.llid = ISO_LLID_UNFRAMED_PDU_START_CONTI_FRAGMENT_SDU;
            }
        } else {
            pCurrCisPdu->llPhysChnPdu.llPduHdr.cisPduHdr.llid = ISO_LLID_FRAMED_PDU_SEGMENT_SDU;
        }
        send_data_pkt             = 1;
        pCisConn->insertEmtpy_flg = 1;

        if (pCisConn->cisSendPldNum > pCisConn->tx_lastpldNum) {
            pCisConn->tx_lastpldNum = pCisConn->cisSendPldNum;
        }


        #if (SLEV_cis_tx_padding)
        log_event_irq(SL_STACK_CIS_TX_DATA_EN, SLEV_cis_tx_padding);
        #endif
    }
    #endif

    pCisConn->locl_sn   = pCisConn->cisSendPldNum & BIT(0);
    pCisConn->locl_nesn = pCisConn->cisRcvdPldNum & BIT(0);

    if (send_data_pkt) {
        pCurrCisPdu->llPhysChnPdu.llPduHdr.cisPduHdr.sn = pCisConn->locl_sn;

        if (pCurrCisPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len) {
            //if(!pCisConn->cis_tx_stream_start) //not judge to save RamCode
            {
                pCisConn->cis_tx_stream_start = 1;
            }

    #if (SLEV_cis_tx_rfLen)
            log_event_irq(SL_STACK_CIS_TX_DATA_EN, SLEV_cis_tx_rfLen);
    #endif
        }

    #if (SL16_cis0_txPrePldNum)
        log_b16_irq(SL_STACK_CIS_TX_DATA_EN, SL16_cis0_txPrePldNum + ((pCisConn->cisRole) ? (pCisConn->cis_index) : (pCisConn->cis_index - bltCisMng.maxNum_cisMaster)), (u16)pCisConn->cisSendPldNum);
    #endif
    } else {
    #if (CIS_ADD_CIE)
        pCurrCisPdu->llPhysChnPdu.llPduHdr.cisPduHdr.cie = (blt_pCisConn->peer_cie & blt_pCisConn->local_cie) ? 1 : 0;
    #endif
    }

    pCurrCisPdu->llPhysChnPdu.llPduHdr.cisPduHdr.nesn = pCisConn->locl_nesn;
    pCisConn->txNullPduFlag                           = !send_data_pkt;


    #if (SL08_cis_snnesn)
    log_b8_irq(SL_STACK_CIS_BASIC_TIMING_EN, SL08_cis_snnesn, pCisConn->locl_sn << 1 | pCisConn->locl_nesn);
    #endif

    #if (SL16_cis_tx_header)
    log_b16_byte_irq(SL_STACK_CIS_BASIC_TIMING_EN, SL16_cis_tx_header, pCurrCisPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len, pCisConn->locl_sn << 1 | pCisConn->locl_nesn);
    #endif


    //Update send data hw fifo register addr
    rf_set_tx_packet_address((u32)pCurrCisPdu);


    return (u8 *)pCurrCisPdu;
}

//common code for CTX & CRX post, to save ram_code
_attribute_ram_code_ void blt_ll_cis_ft_subevent_commm(ll_cis_conn_t *pCisConn, u8 jump)
{
    u8 localBN = pCisConn->bn_loca;
    u8 peerBN  = pCisConn->bn_peer;
    u8 localFT = pCisConn->ft_loca;
    u8 peerFT  = pCisConn->ft_peer;

    if (localBN) {
        if ((pCisConn->cisEventCnt + 1) >= localFT) {
            iso_evtcnt_t txFlushEvt = pCisConn->cisEventCnt + 1 - localFT;
            int          num        = blt_ll_calFt_PDU_num(pCisConn, pCisConn->ftPt_loca, localBN, jump);

            iso_evtcnt_t tx_ft_pldNum = txFlushEvt * pCisConn->bn_loca + num;
            if (tx_ft_pldNum > pCisConn->cisSendPldNum) {
                pCisConn->cisSendPldNum = tx_ft_pldNum;
                pCisConn->txNullPduFlag = TRUE;
                DBG_FANQH_CHN7_TOGGLE;
            }
        }

    #if (SL16_cis0_txCurSendPldNum)
        log_b16_irq(SL_STACK_CIS_TX_DATA_EN, SL16_cis0_txCurSendPldNum + ((pCisConn->cisRole) ? (pCisConn->cis_index) : (pCisConn->cis_index - bltCisMng.maxNum_cisMaster)), (u16)pCisConn->cisSendPldNum); //+pCisConn->cis_index
    #endif
    }


    if (peerBN) {
        if ((pCisConn->cisEventCnt + 1) >= peerFT) {
            iso_evtcnt_t rxFlushEvt = pCisConn->cisEventCnt - peerFT + 1;
            s8           num        = blt_ll_calFt_PDU_num(pCisConn, pCisConn->ftPt_peer, peerBN, jump);

            if ((int)(rxFlushEvt * peerBN - 1 + num) < 0) {
                return;
            }

            iso_evtcnt_t rx_ft_pldNum = rxFlushEvt * peerBN - 1 + num;

            if (rx_ft_pldNum >= pCisConn->cisRcvdPldNum) {
                pCisConn->cisRcvdPldNum = rx_ft_pldNum; //last flush PayloadNum
                blt_cis_pushRxEvtInfoToFifo(pCisConn, NULL, rx_ft_pldNum - pCisConn->cisRcvdPldNum + 1);
                pCisConn->cisRcvdPldNum++;              //the next expect PayloadNum

                DBG_FANQH_CHN1_TOGGLE;

    #if (SL16_cis0_rxPldNum)
                log_b16_irq(SL_STACK_CIS_RX_DATA_EN, SL16_cis0_rxPldNum + ((pCisConn->cisRole) ? (cisConn_param.blt_cis_sel) : (cisConn_param.blt_cis_sel - bltCisMng.maxNum_cisMaster)),
                            (iso_evtcnt_t)pCisConn->cisRcvdPldNum); //
    #endif
            }
        } else {
        }

        tlkapi_send_string_u32s(DBG_CIS_TX_DATA_FLOW_EN | DBG_CIS_RX_DATA_FLOW_EN, "Tx&Rx pld ft", pCisConn->cisEventCnt, pCisConn->cisSendPldNum, pCisConn->cisRcvdPldNum, 0);
    }
}

_attribute_ram_code_ int blt_cis_pushRxEvtInfoToFifo(ll_cis_conn_t *pCisConn, rf_packet_ll_data_t *pIsoRxRawPkt, u8 jump_num)
{
    u8 next_buffer = 0;

    if (!pCisConn->cis_rx_stream_start) {
        return next_buffer;
    }

    iso_rx_evt_t *pIsoRxEvt = (iso_rx_evt_t *)(bltCisRxEvt.p + (bltCisRxEvt.wptr++ & bltCisRxEvt.mask) * bltCisRxEvt.size);
    #if (!FIX_CIS_EVT_OVERFLOW)
    bltCisRxEvt.wptr = (bltCisRxEvt.wptr & bltCisRxEvt.mask);
    #endif
    pIsoRxEvt->curRcvdPldNum = pCisConn->cisRcvdPldNum; //keep received pkt's cisPayloadNum
    pIsoRxEvt->link_idx      = pCisConn->cis_index;
    pIsoRxEvt->pCurrIsoRxPdu = NULL;

    if (pIsoRxRawPkt == NULL) {
        pIsoRxEvt->null_flag  = jump_num;
        pIsoRxEvt->payloadLen = 0;
        pIsoRxEvt->llid       = ISO_LLID_RESERVED;
    } else {
        pIsoRxEvt->null_flag  = 0;
        pIsoRxEvt->payloadLen = pIsoRxRawPkt->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len;
        pIsoRxEvt->llid       = pIsoRxRawPkt->llPhysChnPdu.llPduHdr.cisPduHdr.llid;
        if (pIsoRxEvt->payloadLen) {
            next_buffer              = 1;
            pIsoRxEvt->pCurrIsoRxPdu = pIsoRxRawPkt;
        }
    }


    tlkapi_send_string_u32s(DBG_CIS_RX_DATA, "[CIS RX] pushRxEvt", blt_debug_hex_2_dec_display(pCisConn->cisEventCnt), pIsoRxEvt->curRcvdPldNum, pIsoRxEvt->null_flag, pCisConn->cisSubEventCnt);

    if ((pIsoRxEvt->curRcvdPldNum % pCisConn->rx_numSdu2Pdu == 0) || (pIsoRxEvt->null_flag)) {
        u8 subEventOffset = 0;
        if (pCisConn->cisSubEventCnt >= 1) {
            subEventOffset = pCisConn->cisSubEventCnt - 1;
        }

        if (pCisConn->cisRole == CIS_ROLE_SLAVE) {
    #if (ISO_DATA_TIMESTAMP_UNIT_US_EN)
            // todo cis_expect_tick change in every subInterval, so should cal cisRef according subInterval
            pIsoRxEvt->cisRefAP = (pCisConn->cis_expect_tick - (subEventOffset)*pCisConn->sub_intvl_tick - (pCisConn->cisEventCnt - pIsoRxEvt->curRcvdPldNum / pCisConn->bn_peer) * pCisConn->iso_intvl_tick) / SYSTEM_TIMER_TICK_1US;
    #else
            // unit:System tick
            pIsoRxEvt->cisRefAP = (pCisConn->cis_expect_tick - (subEventOffset)*pCisConn->sub_intvl_tick - (pCisConn->cisEventCnt - pIsoRxEvt->curRcvdPldNum / pCisConn->bn_peer) * pCisConn->iso_intvl_tick);
    #endif
            tlkapi_send_string_u32s(0, "cisEvnt", blt_debug_hex_2_dec_display(pIsoRxEvt->curRcvdPldNum), blt_debug_hex_2_dec_display(pCisConn->cisEventCnt), blt_debug_hex_2_dec_display(pCisConn->cisSubEventCnt), blt_debug_hex_2_dec_display(pCisConn->bn_peer));

            tlkapi_send_string_u32s(0, "cisAef", blt_debug_hex_2_dec_display(pIsoRxEvt->cisRefAP), blt_debug_hex_2_dec_display(pCisConn->cis_expect_tick), blt_debug_hex_2_dec_display(pCisConn->sub_intvl_tick), blt_debug_hex_2_dec_display(pCisConn->iso_intvl_tick));
        } else {
    #if (ISO_DATA_TIMESTAMP_UNIT_US_EN)
            pIsoRxEvt->cisRefAP = (pCisConn->cis_expect_tick - (pCisConn->cisSubEventCnt - 1) * pCisConn->sub_intvl_tick - (pCisConn->cisEventCnt - pIsoRxEvt->curRcvdPldNum / pCisConn->bn_peer) * pCisConn->iso_intvl_tick) / SYSTEM_TIMER_TICK_1US; ///SYSTEM_TIMER_TICK_1US ;
    #else
            pIsoRxEvt->cisRefAP = (pCisConn->cis_expect_tick - (subEventOffset)*pCisConn->sub_intvl_tick - (pCisConn->cisEventCnt - pIsoRxEvt->curRcvdPldNum / pCisConn->bn_peer) * pCisConn->iso_intvl_tick); ///SYSTEM_TIMER_TICK_1US ;
    #endif

            tlkapi_send_string_u32s(0, "cisEvnt1", blt_debug_hex_2_dec_display(pCisConn->cis_connHandle), blt_debug_hex_2_dec_display(pIsoRxEvt->curRcvdPldNum), blt_debug_hex_2_dec_display(pCisConn->cis_expect_tick), blt_debug_hex_2_dec_display(pIsoRxEvt->cisRefAP));


            tlkapi_send_string_u32s(0, "cisEvnt2", blt_debug_hex_2_dec_display(pCisConn->cis_connHandle), blt_debug_hex_2_dec_display(pCisConn->cisEventCnt), blt_debug_hex_2_dec_display(pIsoRxEvt->curRcvdPldNum), blt_debug_hex_2_dec_display(pCisConn->iso_intvl_tick));
        }
    }

    return next_buffer;
}

//common code for CTX & CRX post, to save ram_code
_attribute_ram_code_ ble_sts_t blt_ll_cis_post_common(ll_cis_conn_t *pCisConn)
{
    if (blmsParam.rf_fsm_busy) {
        STOP_RF_STATE_MACHINE;
        blmsParam.rf_fsm_busy           = 0;
        blmsParam.delay_clear_rf_status = 1;
    }


    u8 ret_status = BLE_SUCCESS;

    if (pCisConn->conState == CONN_STATUS_COMPLETE) {
        if (pCisConn->cis_receive_packet) {
            pCisConn->conState = CONN_STATUS_ESTABLISH;

            if (pCisConn->cisFlowFlg == CIS_FLOW_CIS_SYNCING) {
                pCisConn->cisFlowFlg = CIS_FLOW_CIS_SYNC_SUCCESS;
                //tlkapi_send_string_u32s(0, "cis conn 7", pCisConn->cis_index, pCisConn->cis_connHandle, pCisConn, 0);
                blt_pAclConn->cisEstablish_msk |= BIT(pCisConn->cis_index);
            }

    #if (LL_FEATURE_ENABLE_POWER_CONTROL)
            if (ll_acl_pcl_irq_task_cb) {
                ll_acl_pcl_irq_task_cb(pCisConn->link_acl_index | FLAG_PCL_PWR_CHG_AFT_CIS_EST, (void *)pCisConn); //blt_ll_pclInterruptTask
            }
    #endif
        }

        //If the CIS supervision timer reaches 6 * ISO_Interval before the CIS is established, the CIS shall be considered lost.
        if (pCisConn->cisEventCnt > (pCisConn->cisEvtCnt_initJump + 5)) {
            if (pCisConn->cisFlowFlg == CIS_FLOW_CIS_SYNCING) {
                pCisConn->cisFlowFlg = CIS_FLOW_CIS_SYNC_FAIL;

                ret_status = HCI_ERR_CONN_FAILED_TO_ESTABLISH;

                tlkapi_send_string_data(DBG_CIS_TERMINATE, "cis conn not establish", 0, 0);
            }
        }
    }


    if (pCisConn->cis_termin_union.terminate_reason) { //terminate come from ACL connection
        ret_status = pCisConn->cis_termin_union.terminate_reason;
    } else if (pCisConn->cis_termin_union.peer_terminate) {
        ret_status = pCisConn->cis_termin_union.terminate_reason = pCisConn->cis_termin_union.peer_terminate;

        tlkapi_send_string_data(DBG_CIS_TERMINATE, "cis conn peer terminate", 0, 0);
    } else if (pCisConn->cis_termin_union.local_terminate) {
        //special: terminate_ind is on ACL connection, here no need see
        ret_status = pCisConn->cis_termin_union.terminate_reason = pCisConn->cis_termin_union.local_terminate;

        tlkapi_send_string_data(DBG_CIS_TERMINATE, "cis conn local terminate", 0, 0);
    } else if ((pCisConn->conState == CONN_STATUS_ESTABLISH) &&
               (u32)(clock_time() - pCisConn->cis_tick) > pCisConn->cis_timeout) {
        ret_status = pCisConn->cis_termin_union.terminate_reason = HCI_ERR_CONN_TIMEOUT;

        tlkapi_send_string_data(DBG_CIS_TERMINATE, "cis conn timeout", 0, 0);
    }

    if (ret_status != BLE_SUCCESS) {
        if (pCisConn->conState == CONN_STATUS_ESTABLISH) {
            blt_pAclConn->cisEstablish_msk &= ~BIT(pCisConn->cis_index);
        }
        pCisConn->conState = CONN_STATUS_DISCONNECT;

        if (pCisConn->cis_termin_union.terminate_reason) {
            pCisConn->discon_evt                      = 1;
            pCisConn->cis_termin_union.peer_terminate = pCisConn->cis_termin_union.local_terminate = 0;
            //log_b8_irq(SL_STACK_CIS_BASIC_TIMING_EN, SL08_cis_tmnt, pCisConn->cis_termin_union.terminate_reason);
        } else {
            //failed to establish do not generate disconnect event
        }
    } else {
    #if (CIS_CIE_CENTRAL_OPTIMIZE)
        if (pCisConn->cisRole == CIS_ROLE_SLAVE) {
            pCisConn->cie_flag = pCisConn->peer_cie & pCisConn->local_cie;
        }
    #else
        #if (CIS_ADD_CIE)
        pCisConn->cie_flag = pCisConn->peer_cie & pCisConn->local_cie;
        #endif
    #endif
        //Flush timeout check
        blt_ll_cis_ft_subevent_commm(pCisConn, 0);
    }

    return ret_status;
}

_attribute_ram_code_ void blt_cis_post_common_2(ll_cis_conn_t *pCisConn)
{
    (void)pCisConn; //unused, remove warning
    //  pCisConn->cis_expect_tick += pCisConn->sub_intvl_tick;
}

_attribute_noinline_
    ble_sts_t
    blt_ll_cis_disconnect(ll_cis_conn_t *pCisConn, u8 reason)
{
    if (pCisConn->cis_termin_union.termin_pack) { //previous terminate not finish
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }


    u8 acl_handle = pCisConn->link_acl_handle;

    if (blt_ll_isAclhdlInvalid(acl_handle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }
    st_ll_conn_t *pconn = (st_ll_conn_t *)&blms[acl_handle & CONN_IDX_MASK];


    u8                            temp_buff[sizeof(rf_packet_ll_cis_terminate_t)];
    rf_packet_ll_cis_terminate_t *pCtrlCisTermInd = (rf_packet_ll_cis_terminate_t *)temp_buff;

    pCtrlCisTermInd->type      = LLID_CONTROL;
    pCtrlCisTermInd->rf_len    = sizeof(rf_packet_ll_cis_terminate_t) - 2;
    pCtrlCisTermInd->opcode    = LL_CIS_TERMINATE_IND;
    pCtrlCisTermInd->cig_id    = pCisConn->link_cigid;
    pCtrlCisTermInd->cis_id    = pCisConn->cis_ID;
    pCtrlCisTermInd->errorCode = reason;


    if (blt_llmsPushLlCtrlPkt(acl_handle, LL_CIS_TERMINATE_IND, temp_buff)) { //push terminate packet OK

        pconn->connMarkTxFifoWptr = pconn->tx_wptr;

        if (reason >= HCI_ERR_REMOTE_USER_TERM_CONN && reason <= HCI_ERR_REMOTE_DEVICE_TERM_CONN_POWER_OFF) {
            pCisConn->cis_termin_union.local_terminate = HCI_ERR_CONN_TERM_BY_LOCAL_HOST;
        } else {
            pCisConn->cis_termin_union.local_terminate = reason;
        }

        tlkapi_send_string_u8s(DBG_CIS_TERMINATE, "local cis terminate", pCisConn->cis_connHandle, reason, 0, 0);
    } else {
        //tlkapi_send_string_data(0,"conn rej limited rsc", &reason, 1);
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }


    return BLE_SUCCESS;
}

ble_sts_t blc_ll_cis_disconnect(u16 cisHandle, u8 reason)
{
    /*
    If, on the Central, the Host issues this command before issuing the
    HCI_LE_Create_CIS command for the same CIS, then the Controller shall
    return the error code Command Disallowed (0x0C).                                    Done !!!

    If, on the Peripheral, the Host issues this command before the Controller has
    generated the HCI_LE_CIS_Established event for that CIS, then the Controller
    shall return the error code Command Disallowed (0x0C).                              Done !!!

    Note: As specified in Section 7.7.5, on the Central, the handle for a CIS
    remains valid even after disconnection and, therefore, the Host can recreate a
    disconnected CIS at a later point in time using the same connection handle
    */

    /*
    Core_5.2  "LE Create CIS command"
    If the Host issues this command before all the HCI_LE_CIS_Established
    events from the previous use of the command have been generated or the
    HCI_LE_Create_CIS command is cancelled using the HCI_Disconnect
    command, the Controller shall reject the command and return the error code
    Command Disallowed (0x0C).
    */

    ll_cis_conn_t *pCisConn = blt_isCisAllocated_by_handle(cisHandle);
    if (pCisConn) {
        if (pCisConn->cisRole == CIS_ROLE_MASTER) {
            if (pCisConn->createCmd) {
                if (pCisConn->cis_established) {
                    //can disconnect
                } else { //cancel "blc_hci_le_createCis", consider IRQ timing
                    if (ll_cig_mst_mlp_task_cb) {
                        // HCI/CIS/BV-02-C test this logic
                        return ll_cig_mst_mlp_task_cb(FLAG_CIS_CREATE_CANCEL, pCisConn); //blt_cig_mst_mainloop_task  blt_ll_createCisCancel
                    }
                }
            } else {
                return HCI_ERR_CMD_DISALLOWED; // HCI/CIS/BV-02-C test this logic
            }
        } else {                               //CIS Slave
            if (!pCisConn->cis_established) {  //HCI/CIS/BI-09-C
                return HCI_ERR_CMD_DISALLOWED;
            } else {
                //can disconnect
            }
        }


        return blt_ll_cis_disconnect(pCisConn, reason);

    } else {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }


    return BLE_SUCCESS;
}

//CIS group post common
_attribute_ram_code_ ble_sts_t blt_cisgrp_post_common(void)
{
    blt_ll_calculate_sSlot_next(clock_time() + (bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US) * SYSTEM_TIMER_TICK_1US);

    return BLE_SUCCESS;
}

_attribute_ram_code_ int irq_cis_rx(void)
{
    #if 1                               //optimize, to save RamCode
    u8 *raw_pkt = ble_curr_rx_dma_buff; //or cisConn_param.cis_rx_dma_buff
    #else
    u8 *raw_pkt = (u8 *)(((u8 *)bltCisPduRxfifo.isoRxPdu) + (bltCisPduRxfifo.wptr & (bltCisPduRxfifo.fifo_num - 1)) * bltCisPduRxfifo.fifo_size);
    #endif

    #if 0 //(CIS_DEBUG_EN)
        iso_rx_evt_t *pIsoRxEvt = (iso_rx_evt_t*)(bltCisRxEvt.p + (bltCisRxEvt.rptr & bltCisRxEvt.mask) * bltCisRxEvt.size);

        if((bltCisRxEvt.rptr != bltCisRxEvt.wptr) && ((u32)new_pkt== (u32)pIsoRxEvt->pCurrIsoRxPdu)){
            BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99210000);
        }
    #endif


    HAL_CLEAR_RF_RX_IRQ;

    //set to a idle buffer, in case of some risk to destroy existed data
    //todo:optimize later
    ble_rf_set_rx_dma((u8 *)glb_temp_rx_buff, 4); //64/16=4
    rf_set_rx_maxlen(10);                         //select a random small value


    u8 next_buffer = 0;
    raw_pkt[2]     = 0;

    /* "rx header tick" none zero is under condition CRC correct, so here do not check CRC by
       "RF_BLE_PACKET_VALIDITY_CHECK" to save RamCode and running timing
       Or we can use "bltRxPkt.crc correct" */
    if (bltRxPkt.rx_header_tick) {
        u8 rf_len = raw_pkt[DMA_RFRX_OFFSET_RFLEN];
        if (rf_len) {
            DBG_SIHUI_CHN7_TOGGLE;
    #if (SLEV_cis_rx_rfLen)
            log_event_irq(SL_STACK_CIS_BASIC_TIMING_EN, SLEV_cis_rx_rfLen);
    #endif
        } else {
    #if (SLEV_cis_rx)
            log_event_irq(SL_STACK_CIS_BASIC_TIMING_EN, SLEV_cis_rx);
    #endif
        }

        DBG_FANQH_CHN13_TOGGLE;

        rf_packet_ll_data_t *pCisRawPkt = (rf_packet_ll_data_t *)raw_pkt;

        blt_pCisConn->cis_receive_packet = 1;
        blt_pCisConn->closeIsoEvent      = pCisRawPkt->llPhysChnPdu.llPduHdr.cisPduHdr.cie;
        blt_pCisConn->cis_tick           = clock_time();


        u8 peerCurSN   = pCisRawPkt->llPhysChnPdu.llPduHdr.cisPduHdr.sn;
        u8 peerCurNESN = pCisRawPkt->llPhysChnPdu.llPduHdr.cisPduHdr.nesn;
        u8 peerCurNPI  = pCisRawPkt->llPhysChnPdu.llPduHdr.cisPduHdr.npi; //SN field is RFU in a CIS Null PDU

    #if (CIS_ADD_CIE)
        if (pCisRawPkt->llPhysChnPdu.llPduHdr.cisPduHdr.cie) {
            blt_pCisConn->local_cie = blt_pCisConn->peer_cie = 1;
        }
    #endif


        //////////////////////  cisRole: 1->master cis; 0->slave cis. //////////////////////////
        bool cisRoleMst = blt_pCisConn->cisRole == CIS_ROLE_MASTER;


    #if (CIS_CIE_CENTRAL_OPTIMIZE)
        if (cisRoleMst && (blt_pCisConn->peer_cie & blt_pCisConn->local_cie)) {
            blt_pCisConn->cie_flag = 1;
        }
    #endif

    #if (ADD_SUD_TIMESTAMP_EN)
        if (bltRxPkt.rx_header_tick && !cisConn_param.cis_rx_num && blms_state == BLMS_STATE_CRX_S) {
            if (!blt_pCisConn->cis_1st_rx_tick) {
        #if (DBG_CIS_TIMING)
            #if (CIS_WINDOW_WIDENING_FOR_BIG_PPM)
                if (tick1_out_range_of_tick2(bltRxPkt.rx_header_tick, blt_pCisConn->cis_expect_tick, 800 * SYSTEM_TIMER_TICK_1US))
            #else
                if (tick1_out_range_of_tick2(bltRxPkt.rx_header_tick, blt_pCisConn->cis_expect_tick, 500 * SYSTEM_TIMER_TICK_1US))
            #endif
                {
                    write_dbg32(0x0018, bltRxPkt.rx_header_tick);
                    write_dbg32(0x001C, blt_pCisConn->cis_expect_tick);
                    BLMS_ERR_DEBUG(DBG_CIS_TIMING, 0x99C10000);
                }
        #endif

                blt_pCisConn->cis_1st_rx_tick = bltRxPkt.rx_header_tick - (blt_pCisConn->cisSubEventCnt - 1) * blt_pCisConn->sub_intvl_tick;
                blt_pCisConn->cis_expect_tick = bltRxPkt.rx_header_tick; //update
            }
        }
    #endif


        //M and S send both CIS Null PDU, without processing the flow control and new received package processing.
        if (!blt_pCisConn->txNullPduFlag) {
            if (blt_pCisConn->bn_loca) //if BN == 0, means local device has no CIS Data PDU to send
            {
                ///////////////////////////////  ACK pkt  //////////////////////////////////////
                if (blt_pCisConn->locl_sn != peerCurNESN) {
                    DBG_FANQH_CHN11_TOGGLE;
                    /*
                     * Received NESN != local transmitSeqNum: locl sn-> current transmitSeqNum + 1  |
                     *                                        locl nesn-> No change
                     */
                    blt_pCisConn->cisSendPldNum++;
                    tlkapi_send_string_u32s(DBG_CIS_TX_DATA, "rx pdu ack", blt_pCisConn->cisSendPldNum, blt_pCisConn->locl_sn, peerCurNESN, 0);
                    blt_pCisConn->txNullPduFlag = TRUE;

    #if (CIS_ADD_CIE)
                    if ((blt_pCisConn->cisSendPldNum / blt_pCisConn->bn_loca) > blt_pCisConn->cisEventCnt) {
                        blt_pCisConn->local_cie = 1;
                    }
    #endif

    #if (SL16_cis0_txCurSendPldNum)
                    log_b16_irq(SL_STACK_CIS_TX_DATA_EN, SL16_cis0_txCurSendPldNum + ((blt_pCisConn->cisRole) ? (blt_pCisConn->cis_index) : (blt_pCisConn->cis_index - bltCisMng.maxNum_cisMaster)), (u16)blt_pCisConn->cisSendPldNum); //+blt_pCisConn->cis_index
    #endif


                    if (!blt_pCisConn->insertEmtpy_flg) {
                        if (blt_pCisConn->cisPduTxFifoWptr != blt_pCisConn->cisPduTxFifoRptr) {
                            blt_pCisConn->cisPduTxFifoRptr++; //point to next ISO data buffer, and prepare to send it
                            tlkapi_send_string_u32s(0, "rx ack", blt_pCisConn->cisEventCnt, blt_pCisConn->cisSubEventCnt, blt_pCisConn->cisPduTxFifoWptr, blt_pCisConn->cisPduTxFifoRptr);
                        } else {
                            tlkapi_send_string_u32s(DBG_CIS_TX_DATA, "PDU buf ERR", blt_pCisConn->cisPduTxFifoWptr, blt_pCisConn->cisPduTxFifoRptr, blt_pCisConn->cisEventCnt, blt_pCisConn->cisSubEventCnt);
                            BLMS_ERR_DEBUG(DBG_CIS_TX_DATA, 0x99C60000);
                        }
                    }

                    if (!cisRoleMst) { //only for cis slave role

                        blt_ll_cis_prepare_data_common(blt_pCisConn);
    #if (HW_AES_CCM_ALG_EN)
                        reg_rf_tx_ccm_pkt_cnt0_31 = blt_pCisConn->cisSendPldNum & 0xffffffff;
                            //todo reg_rf_tx_ccm_pkt_cnt32_37
    #endif
                    }
                }
            }
        }


        /*1. Data packet; 2.BN != 0; 3. new packet */ //if BN == 0, means peer device has no CIS Data PDU to send
        if (!peerCurNPI && blt_pCisConn->bn_peer && (peerCurSN == blt_pCisConn->locl_nesn)) {
            /*
             * Received SN = local nextExpectedSeqNum: locl sn-> No change  |
             *                                         locl nesn-> current nextExpectedSeqNum + 1
             */
            //DBG_C HN10_TOGGLE;
            //////////////////////////////////////////////////////////////////////////////////////////////
            //TODO: Note that after the end of RX, if the SN or NESN, or send new data needs to be set before
            //the TX preamble, otherwise the set parameters will not take effect, 5 preamble needs 40us.
            //////////////////////////////////////////////////////////////////////////////////////////////
            tlkapi_send_string_u32s(DBG_CIS_RX_DATA, "[CIS RX] IRQ RX", blt_pCisConn->cisEventCnt, blt_pCisConn->cisRcvdPldNum, blt_pCisConn->cis_rx_stream_start, rf_len);

            if (!blt_pCisConn->cis_rx_stream_start) {
                blt_pCisConn->cis_rx_stream_start = 1;
            }
            next_buffer = blt_cis_pushRxEvtInfoToFifo(blt_pCisConn, pCisRawPkt, 0); //must before RX PDU number update

    #if (FIX_CIS_EVT_OVERFLOW)
            if (((u8)(bltCisRxEvt.wptr - bltCisRxEvt.rptr) & 63) < bltCisRxEvt.num) //have enough evt
    #endif
            {
                blt_pCisConn->cisRcvdPldNum++;                                      //update peer payload_number after "blt_cis_push RxEvtInfoToFifo"
                DBG_FANQH_CHN12_TOGGLE;
    #if (SL16_cis0_rxPldNum)
                log_b16_irq(SL_STACK_CIS_RX_DATA_EN, SL16_cis0_rxPldNum + ((blt_pCisConn->cisRole) ? (cisConn_param.blt_cis_sel) : (cisConn_param.blt_cis_sel - bltCisMng.maxNum_cisMaster)), (iso_evtcnt_t)blt_pCisConn->cisRcvdPldNum);
    #endif
            }

    #if (CIS_ADD_CIE)
            //if peer BN have received all
            if ((blt_pCisConn->cisRcvdPldNum / blt_pCisConn->bn_peer) > blt_pCisConn->cisEventCnt) {
                blt_pCisConn->peer_cie = 1;

                tlkapi_send_string_u32s(0, "Clean Peer cie", blt_pCisConn->cisEventCnt, blt_pCisConn->cisSubEventCnt, blt_pCisConn->peer_cie, blt_pCisConn->local_cie);
            }
    #endif


            if (!cisRoleMst) {                                                               //only for CIS slave
                blt_pCisConn->locl_nesn                           = blt_pCisConn->cisRcvdPldNum & BIT(0);
                pCurrCisPdu->llPhysChnPdu.llPduHdr.cisPduHdr.nesn = blt_pCisConn->locl_nesn; //Update content as far as possible
            }

            //Mark it's a CIS ISO Data PDU(contains CIS Empty PDU)
            raw_pkt[2] = blt_pCisConn->cis_connHandle;

            /*
             * 1.If L > F, then the PDU you receive must either be L+1 (which you want) or a retransmission of L (which you can ignore), depending on SN.
             * 2.If L <= F, then the PDU you receive must be F+1, since it is forbidden to send any PDU that has been reached its flush point and it is forbidden
             * to send F+2 until F+1 is acknowledged or flushed. In this latter case only one SN value is valid; the other is forbidden.
             */
        }


    #if (CIS_ADD_CIE)
        if (!cisRoleMst) {
            pCurrCisPdu->llPhysChnPdu.llPduHdr.cisPduHdr.cie = blt_pCisConn->peer_cie & blt_pCisConn->local_cie;
            //          tlkapi_send_string_u32s(0, "cie", blt_pCisConn->cisEventCnt, blt_pCisConn->cisSubEventCnt, blt_pCisConn->peer_cie, blt_pCisConn->local_cie);
        }
    #endif


    #if (LL_FEATURE_ENABLE_POWER_CONTROL)
        if (ll_acl_pcl_irq_task_cb) {
            if (blt_pCisConn->conState == CONN_STATUS_ESTABLISH) {
                /* peer_coded_phy_ci is only used by PCL */
                if (bltPHYs.cur_llPhy == BLE_PHY_CODED) {
                    blt_pCisConn->peer_coded_phy_ci = bltPHYs.cur_peer_CI;
                }
                /* Monitoring CIS rx packet's RSSI value */
                s8 rssi = raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)] - 110;                                                //OR get rssi by HW register
                ll_acl_pcl_irq_task_cb(blt_pCisConn->link_acl_index | FLAG_PCL_MONITORING_CIS_RX_RSSI, (void *)&rssi); //blt_ll_pclInterruptTask
            }
        }
    #endif

    } else {
    }


    if (next_buffer) //update buffer
    {
        bltCisPduRxfifo.wptr++;
        u8 *new_pkt                   = (u8 *)(((u8 *)bltCisPduRxfifo.isoRxPdu) + (bltCisPduRxfifo.wptr & (bltCisPduRxfifo.fifo_num - 1)) * bltCisPduRxfifo.fifo_size);
        cisConn_param.cis_rx_dma_buff = (u32)new_pkt; //Reuse the last dma rx buffer
    }

    cisConn_param.cis_rx_num++;                       //do not care CRC
    raw_pkt[0] = 1;


    if (blt_pCisConn->cisRole == CIS_ROLE_SLAVE) {
        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_TX_ON);
        }
    }

    return 0;
}


    #if 0 //no need TX logic now, so save some RamCode and timing
_attribute_ram_code_ void irq_cis_tx(void)
{
    if(blt_pCisConn->cisRole == CIS_ROLE_MASTER){
        if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }
    }
}
    #endif


    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    int
    blt_ll_cis_conn_mainloop(void)
{
    ll_cis_conn_t *pCisConn;
    for (int cis_conn_idx = 0; cis_conn_idx < bltCisMng.maxNum_cisConn; cis_conn_idx++) {
        pCisConn = (ll_cis_conn_t *)(global_pCisConn + cis_conn_idx);

        //---------- process pending event ------------------------------------------
        if (pCisConn->discon_evt) {
            blt_ll_cis_procCisConnectionEvent(BLT_CIS_HANDLE | cis_conn_idx, pCisConn);
        }

        if (pCisConn->cis_established) {
            if (pCisConn->cisSduIn_wptr != pCisConn->cisSduIn_rptr) {
                blt_cis_tx_loop(pCisConn);
            }
    #if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
            /* CIS use: send numOfCmpEvt after data transmitted OK or flushed */
            if (pCisConn->cisNocpRptr != pCisConn->cisNocpWptr) {
                blt_cis_release_iso_data_buffer(pCisConn);
            }
    #endif
            if (pCisConn->pCisTestParam) {
                blt_cis_test_process(pCisConn);
            }

    #if (1)
            //      if(pCisConn->cisSduOut_rptr != pCisConn->cisSduOut_wptr)
            if (blt_hci_iso_data_handler && (pCisConn->cisSduOut_rptr != pCisConn->cisSduOut_wptr)) { // todo Temporary modification for TianXiang
                blt_cis_sdu_out_loop(pCisConn);
            }
    #else
            if (blc_hci_event_handler && (pCisConn->cis_established) && (pCisConn->cisSduOut_rptr != pCisConn->cisSduOut_wptr)) {
                sdu_packet_t *sdu = (sdu_packet_t *)(pCisConn->cis_sduOutBuf + sduCisMng.max_out_fifo_size * (pCisConn->cisSduOut_rptr & (sduCisMng.out_fifo_mask)));
                if (blc_hci_event_handler(pCisConn->cis_connHandle | HCI_FLAG_ISO_DATE_STD, (u8 *)sdu, sdu->iso_sdu_len) == 0) //blc_hci_send_data
                {
                    pCisConn->cisSduOut_rptr++;
                }
            }
    #endif
        }
    }


    if (bltCisRxEvt.rptr != bltCisRxEvt.wptr) {
        blt_cis_rx_event_loop();
    }


    #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER)
    if (ll_cig_mst_mlp_task_cb) {
        ll_cig_mst_mlp_task_cb(FLAG_MODULE_MAINLOOP, NULL); //blt_cig_mst_mainloop_task
    }
    #endif

    #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE)
    if (ll_cis_slv_mlp_task_cb) {
        ll_cis_slv_mlp_task_cb(FLAG_MODULE_MAINLOOP); //blt_cig_slv_mainloop_task
    }
    #endif
    return 0;
}


    #if (BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE && STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
    #endif
    int
    blt_cis_update_chn_map(int trigger_tick, void *p)
{
    st_ll_conn_t *pAclConn = (st_ll_conn_t *)p;
    for (int i = 0; i < bltCisMng.maxNum_cisConn; i++) {
        if (pAclConn->cisEstablish_msk & BIT(i)) {
            ll_cis_conn_t *pCisConn = (ll_cis_conn_t *)(global_pCisConn + i);
            if (!pCisConn->updateMap_cmd) { //if no previous update pending
                pCisConn->updateMap_cmd  = 1;
                pCisConn->updateMap_tick = (u32)trigger_tick;
            }
        }
    }

    return 0;
}

void blt_cis_calculateInterval(ll_cis_conn_t *pCisConn, u16 acl_interval, u16 iso_interval)
{
    /* check if ACL connection interval is multiple of ISO interval or  ISO interval is multiple of ACL connection interval */
    int mod;
    if (acl_interval > iso_interval) {
        mod = acl_interval % iso_interval;
    } else {
        mod = iso_interval % acl_interval;
    }

    if (mod) {
        pCisConn->align_with_acl = 0;
    } else {
        pCisConn->align_with_acl = 1;
    }

    int i;
    for (i = 1; i <= iso_interval; i++) {
        if ((acl_interval * i) % iso_interval == 0) {
            break;
        }
    }

    pCisConn->align_mul_coeff = i;
}


    #if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
_attribute_ram_code_ void blt_cis_mark_numOfcmpEvt_status(ll_cis_conn_t *pCisConn, int numOfcmpPkt)
{
    /* CIS use: send numOfCmpEvt after data transmitted OK or flushed */
    u8 wptr                      = pCisConn->cisNocpWptr & NOC_ACK_MASK;
    pCisConn->cisNocTxWptr[wptr] = pCisConn->cisPduTxFifoWptr;
    pCisConn->cisNocPktCnt[wptr] = numOfcmpPkt;

    tlkapi_send_string_u8s(DBG_NUM_COM_PKT, "numComPkt, mark", pCisConn->cisNocpWptr, pCisConn->cisPduTxFifoWptr, pCisConn->cisPduTxFifoRptr, numOfcmpPkt);
    pCisConn->cisNocpWptr++;
}
    #endif


    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    int
    blt_cis_tx_loop(ll_cis_conn_t *pCisConn)
{
    // Need to be adjusted according to the maximum execution time blt_cis_splitSdu2FramedPdu,this API should be
    //finished before CIG start


    if (pCisConn->cis_frame == CIS_UNFRAMED ||
        (pCisConn->cig_next_tick && clock_time_exceed(pCisConn->cig_next_tick, (pCisConn->iso_intvl_us - SPILT_SDU2PDU_PRE_PROCESS_US)))) {
        /****************split unframed packet/framed packet*******************************************************/
        while (pCisConn->cisSduIn_wptr != pCisConn->cisSduIn_rptr) {
            tlkapi_send_string_data(DBG_CIS_TX_DATA, "CIS SDU in", 0, 0);

            sdu_packet_t *sdu = (sdu_packet_t *)(pCisConn->cis_sduInBuf + sduCisMng.max_in_fifo_size * (pCisConn->cisSduIn_rptr & sduCisMng.in_fifo_mask));

            u8 numOfcmpPkt = 0; //initial value must be 0
            u8 ret_status  = BLE_SUCCESS;
            if (pCisConn->cis_frame == CIS_UNFRAMED) {
                ret_status = blt_cis_splitSdu2UnframedPdu(pCisConn, sdu, &numOfcmpPkt);
            } else {
                blt_cis_splitSdu2FramedPdu(pCisConn, &numOfcmpPkt);
            }

    #if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
            if (numOfcmpPkt && blmsParam.standard_hci_en) {
                blt_cis_mark_numOfcmpEvt_status(pCisConn, numOfcmpPkt);
            }
    #endif
            if (ret_status == LL_ERR_TX_FIFO_NOT_ENOUGH) {
                break;
            }
        }
    }

    return 0;
}


    #if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
        #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
        #else
_attribute_no_inline_
        #endif
    int
    blt_cis_release_iso_data_buffer(ll_cis_conn_t *pCisConn)
{
    int numberPkt = 0;
    while (pCisConn->cisNocpRptr != pCisConn->cisNocpWptr) {
        u8 rptr      = pCisConn->cisNocpRptr & NOC_ACK_MASK;
        u8 mrkTxwptr = pCisConn->cisNocTxWptr[rptr];

        /* cisPduTxFifoRptr may change in IRQ */
        u32 r       = irq_disable();
        u8  deltaTx = (u8)(pCisConn->cisPduTxFifoRptr - mrkTxwptr);
        irq_restore(r);

        if (deltaTx < 128) { //match
            int cur_numPkt = pCisConn->cisNocPktCnt[rptr];
            numberPkt += cur_numPkt;
            //tlkapi_send_string_u8s(DBG_NUM_COM_PKT, "numComPkt, match", pCisConn->cisNocpRptr, pCisConn->cisPduTxFifoRptr, mrkTxwptr, cur_numPkt);
            pCisConn->cisNocpRptr++;
        } else {
            //if front buffer data not match, data behind must can not match either, can exit
            break;
        }
    }

    /* if disconnect happens, previous buffer release not need send to host
     * when test CIS/PER/BV-07-C, warning that unknown handle for num of complete event,
     * add "!pCisConn->discon_evt" to solve.  20220825 SiHui */
    if (numberPkt && pCisConn->conState && !pCisConn->cis_termin_union.local_terminate) {
        hci_numberOfCompletePacket_evt(pCisConn->cis_connHandle, numberPkt);
        //DBG_C HN11_TOGGLE;
        tlkapi_send_string_u8s(DBG_NUM_COM_PKT, "numComPkt, send", pCisConn->cisNocpRptr, pCisConn->cisPduTxFifoWptr, pCisConn->cisPduTxFifoRptr, numberPkt);
    }

    return 0;
}
    #endif

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    int
    blt_cis_rx_event_loop(void)
{
    /*******************************CIS******************************************************************/
    while (bltCisRxEvt.rptr != bltCisRxEvt.wptr) {
        iso_rx_evt_t *pIsoRxEvt = (iso_rx_evt_t *)(bltCisRxEvt.p + (bltCisRxEvt.rptr & bltCisRxEvt.mask) * bltCisRxEvt.size);

        ll_cis_conn_t *pCisConn = (ll_cis_conn_t *)(global_pCisConn + pIsoRxEvt->link_idx);

        u8 state = 0;
        /* SiHui question to QingHua: ISO test mode data no need setup data path ? */
        if ((blmsParam.standard_hci_en) && (pCisConn->dpId == Data_Path_Disable) && (pCisConn->pCisTestParam == NULL)) { // have not set data path, so discard this PDU
            state = 1;
        }

        //CIS Encryption
        if (pIsoRxEvt->payloadLen && pCisConn->crypt.enable) {
    #if (HW_AES_CCM_ALG_EN)
            pIsoRxEvt->pCurrIsoRxPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len -= 4;
    #else
            ble_crypt_para_t *pLeCryptCtrl = &pCisConn->crypt;
            pLeCryptCtrl->dec_pno          = pIsoRxEvt->curRcvdPldNum;
            /*
             * ll_ccm_enc: Master role must use 1, Slave role must use 0;
             * ll_ccm_dec: Master role must use 0, Slave role must use 1;
             */
            aes_enc_dec_busy = 1;

            /*
             * qinghua.fan
             * 48MHZ, 251bytes, cost 904us
             */
            DBG_FANQH_CHN4_HIGH;
            u8 st = aes_ll_ccm_decryption(&pIsoRxEvt->pCurrIsoRxPdu->llPhysChnPdu, pCisConn->cisRole == CIS_ROLE_MASTER ? 0 : 1, CRYPT_NONCE_TYPE_CIS, pLeCryptCtrl);
            DBG_FANQH_CHN4_LOW;
            aes_enc_dec_busy = 0;

            if (st) { //decrypt err

                state                    = 1;
                pCisConn->crypt.mic_fail = 1;
                /* here do not call "blc ll cis disconnect", because it may trigger CIS create cancel logic */
                blt_ll_cis_disconnect(pCisConn, HCI_ERR_CONN_TERM_MIC_FAILURE);

                /* BQB:
                 * for LL/CIS/PER/BV-27-C, lower tester fake a error data to trigger IUT send terminate with 0x3D,
                 * can not while here */
                //BLMS_ERR_DEBUG(1, 0xAA990000);
            }
    #endif
        }


        if (!state) {
            iso_rx_evt_t isoEvt;
            smemcpy(&isoEvt, pIsoRxEvt, sizeof(iso_rx_evt_t));
            do {
                if (pIsoRxEvt->null_flag) {
                    isoEvt.curRcvdPldNum = pIsoRxEvt->curRcvdPldNum - pIsoRxEvt->null_flag + 1;
                    isoEvt.cisRefAP      = pIsoRxEvt->cisRefAP - (pIsoRxEvt->null_flag - 1) * pCisConn->iso_intvl_tick;
                    isoEvt.null_flag     = 1;
                    isoEvt.pCurrIsoRxPdu = NULL;

                    pIsoRxEvt->null_flag--;
                }

                blt_ial_reassembleCisPdu2Sdu(pCisConn, &isoEvt);


            } while (pIsoRxEvt->null_flag);
        }

        bltCisRxEvt.rptr++;
    #if (!FIX_CIS_EVT_OVERFLOW)
        bltCisRxEvt.rptr = bltCisRxEvt.rptr & bltCisRxEvt.mask;
    #endif
        tlkapi_send_string_data(DBG_CIS_RX_DATA, "[CIS RX] popRxEvt", &pIsoRxEvt->curRcvdPldNum, 4);
    }


    return 0;
}


    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    void
    blt_cis_sdu_out_loop(ll_cis_conn_t *pCisConn)
{
    while (pCisConn->cisSduOut_wptr != pCisConn->cisSduOut_rptr) {
        tlkapi_send_string_u8s(DBG_CIS_RX_DATA, "[CIS RX] pop sduOut", pCisConn->cisSduOut_wptr, pCisConn->cisSduOut_rptr, 0, 0);

        sdu_packet_t *sdu = (sdu_packet_t *)(pCisConn->cis_sduOutBuf + sduCisMng.max_out_fifo_size * (pCisConn->cisSduOut_rptr & (sduCisMng.out_fifo_mask)));
        /*
            +--------------+--------------+------------+------------+------------+---------+----+----+--------------+
            | 2            | 2            | 4          | 2          | 1          | 1       | 1  | 1  | iso_sdu_len  |
            +--------------+--------------+------------+------------+------------+---------+----+----+--------------+
            | pkt_seq_num  | iso_sdu_len  | timestamp  | sduOffset  | numHciPkt  | pkt_st  | PB | TS | SDU_Data     |
            +--------------+--------------+------------+------------+------------+---------+----+----+--------------+

            HCI ISO out DATA format in telink
            +------+-------+---------+-----------------------+------------+----------------------+-----------------+----------+
            | 2    | 1     | 2       | 2                     | 4          | 2                    | 2               | n        |
            +------+-------+---------+-----------------------+------------+----------------------+-----------------+----------+
            | len  | type  | handle  | ISO_data_load_length  | timestamp  | packet_sequence_num  | iso_sdu_length  | sd_data  |
            +------+-------+---------+-----------------------+------------+----------------------+-----------------+----------+
             */

    #if (WALKAROUND_ISO_TIMESTAMP_EN)
        sdu->timestamp = pCisConn->cis_ap_tick + (sdu->pkt_seq_num) * pCisConn->iso_intvl_us;
        tlkapi_send_string_u32s(0, "sdu timestamp", pCisConn->cis_ap_tick, sdu->pkt_seq_num, sdu->timestamp, 0);
    #endif

        if (blt_iso_proSduPacket(sdu) == BLE_SUCCESS) {
            pCisConn->cisSduOut_rptr++;
        }
    }
}

bool blt_ll_cis_encryptPdu(ll_cis_conn_t *pCisConn, cis_tx_pdu_t *pdu)
{
    rf_packet_ll_data_t *pRfPdu     = &pdu->isoTxPdu;
    rf_cis_data_hdr_t   *pCisPduHdr = &pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr;

    if ((pCisPduHdr->rf_len > 0) && (pCisConn->crypt.enable)) {
        /* AES_CCM_Encryption in IRQ, AES_CCM_Decryption in main_loop maybe overlap!!! (IRQ protect)
        It's best to add protection, safety : save AES_CCM settings */
        ble_crypt_para_t cisCryptCtrlBackUp = pCisConn->crypt;

        ble_crypt_para_t *pLeCryptCtrl = &pCisConn->crypt;
        pLeCryptCtrl->enc_pno          = pdu->cis_pdu_number;
        /*
         * ll_ccm_enc: Master role must use 1, Slave role must use 0;
         * ll_ccm_dec: Master role must use 0, Slave role must use 1;
         */
        //tlkapi_send_string_data(0,"raw ISO pkt", pLeCryptCtrl->pllPhysChnPdu, pLeCryptCtrl->pllPhysChnPdu->llPduHdr.pduHdr.rf_len+2);
        //printf("Tx PN:%d\n", (u32)cisSeqNoInfo.cisPayloadNumber);
        aes_enc_dec_busy = 1;
        aes_ll_ccm_encryption(&pRfPdu->llPhysChnPdu, pCisConn->cisRole, CRYPT_NONCE_TYPE_CIS, &pCisConn->crypt);
        aes_enc_dec_busy = 0;

        //tlkapi_send_string_data(0,"enc ISO pkt", pLeCryptCtrl->pllPhysChnPdu, pLeCryptCtrl->pllPhysChnPdu->llPduHdr.pduHdr.rf_len+2);

        /* AES_CCM_Encryption in IRQ, AES_CCM_Decryption in main_loop maybe overlap!!! (IRQ protect)
        It's best to add protection, safety : restore AES_CCM settings */
        pCisConn->crypt = cisCryptCtrlBackUp;
    }

    return TRUE;
}

_attribute_noinline_ ll_cis_conn_t *blt_isCisEstablished_by_handle(u16 handle)
{
    if (handle >= BLT_CIS_HANDLE && handle < (BLT_CIS_HANDLE + bltCisMng.maxNum_cisConn)) {
        u8             cis_idx  = handle & BLT_CIS_IDX_MSK;
        ll_cis_conn_t *pCisConn = (ll_cis_conn_t *)(global_pCisConn + cis_idx);
        if (pCisConn->cis_established) {
            return pCisConn;
        }
    }

    return NULL;
}

_attribute_noinline_ ll_cis_conn_t *blt_isCisAllocated_by_handle(u16 handle)
{
    if (handle >= BLT_CIS_HANDLE && handle < (BLT_CIS_HANDLE + bltCisMng.maxNum_cisConn)) {
        u8             cis_idx  = handle & BLT_CIS_IDX_MSK;
        ll_cis_conn_t *pCisConn = (ll_cis_conn_t *)(global_pCisConn + cis_idx);
        if (pCisConn->cis_occupied) {
            return pCisConn;
        }
    }

    return NULL;
}

ble_sts_t blc_ll_cis_iso_transmit_test_cmd(u16 connHandle, u8 type)
{
    ll_cis_conn_t *pCis = blt_isCisAllocated_by_handle(connHandle);
    if (pCis == NULL || !pCis->cis_established) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    /*
 * If the Host issues this command when the value of the transmit BN parameter
 * of the CIS is set to zero, the Controller shall return the error code Unsupported
 * Feature or Parameter Value (0x11)
 */
    if (!pCis->bn_loca) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if (pCis->pCisTestParam == NULL) {
        pCis->pCisTestParam = blt_iso_test_allocateCtrlBlock();
        if (pCis->pCisTestParam == NULL) {
            BLMS_ERR_DEBUG(DBG_IAL_LOGIC, 0xAA020000);
            return HCI_ERR_CMD_DISALLOWED;
        }
    }


    if (pCis->cis_dapth_setup & BIT(Data_Dir_Input)) {
        return HCI_ERR_CMD_DISALLOWED;
    }


    iso_test_param_t *isoTest         = pCis->pCisTestParam;
    isoTest->isoTestMode              = ISO_TEST_TRANSMIT_MODE;
    isoTest->isoTest_payload_type     = type;
    isoTest->tranMode.isoTestSendTick = 0;
    isoTest->tranMode.send_pkt_cnt    = 0;

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_cis_iso_transmit_test_cmd(hci_le_isoTestCmdParams_t *pCmd, hci_le_isoTestRetParams_t *pRet)
{
    pRet->status      = blc_ll_cis_iso_transmit_test_cmd(pCmd->conn_handle, pCmd->payload_type);
    pRet->conn_handle = pCmd->conn_handle;

    return pRet->status;
}

ble_sts_t blc_ll_cis_iso_receive_mode(u16 connHandle, itest_payload_type_t pdu_type)
{
    ll_cis_conn_t *pCis = blt_isCisAllocated_by_handle(connHandle);

    if ((pCis == NULL) || (pCis->conState != CONN_STATUS_ESTABLISH)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }


    /*
     * If the Host issues this command when the value of the transmit BN parameter
     * of the CIS is set to zero, the Controller shall return the error code Unsupported
     * Feature or Parameter Value (0x11)
     */
    if (!pCis->bn_peer) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }


    if (pCis->pCisTestParam == NULL) {
        pCis->pCisTestParam = &gIsoTestPara[0];
    } else {
        if (pCis->pCisTestParam->isoTestMode == ISO_TEST_TRANSMIT_MODE) {
            return HCI_ERR_CMD_DISALLOWED;
        }
    }


    iso_test_param_t *isoTest = pCis->pCisTestParam;

    isoTest->isoTestMode          = ISO_RECEIVE_MODE;
    isoTest->isoTest_payload_type = pdu_type;
    isoTest->recMode.failedCnt    = 0;
    isoTest->recMode.missedCnt    = 0;
    isoTest->recMode.successCnt   = 0;
    isoTest->recMode.expectCnt    = 0;

    tlkapi_send_string_u32s(DBG_ISO_TEST_EN, "iso set rec mode", isoTest->isoTestMode, 0, 0, 0);

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_cis_iso_receive_test(hci_le_isoTestCmdParams_t *pCmdParam, hci_le_isoTestRetParams_t *pRetParam)
{
    ble_sts_t ret_status = blc_ll_cis_iso_receive_mode(pCmdParam->conn_handle, pCmdParam->payload_type);

    pRetParam->status      = ret_status;
    pRetParam->conn_handle = pCmdParam->conn_handle;

    return ret_status;
}

ble_sts_t blc_ll_cis_iso_read_test_count_cmd(u16 connHandle, hci_le_isoRxTestStatusParam_t *pRetParam)
{
    ll_cis_conn_t *pCis = blt_isCisAllocated_by_handle(connHandle);

    pRetParam->status = 0; //init ret
    if ((pCis == NULL) || (pCis->conState != CONN_STATUS_ESTABLISH)) {
        pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
    }

    iso_test_param_t *pIsoTest = pCis->pCisTestParam;

    if ((pIsoTest == NULL) || (pIsoTest->isoTestMode != ISO_RECEIVE_MODE)) {
        pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
    }

    if (pRetParam->status == BLE_SUCCESS) {
        pRetParam->failed_packet_count   = pIsoTest->recMode.failedCnt;
        pRetParam->miss_packet_count     = pIsoTest->recMode.missedCnt;
        pRetParam->received_packet_count = pIsoTest->recMode.successCnt;
    } else {
        pRetParam->failed_packet_count   = 0;
        pRetParam->miss_packet_count     = 0;
        pRetParam->received_packet_count = 0;
    }

    return pRetParam->status;
}

ble_sts_t blc_hci_cis_read_test_count_cmd(hci_le_isoReadTestCountsCmdParams_t *pcmd, hci_le_isoRxTestStatusParam_t *pRet)
{
    return blc_ll_cis_iso_read_test_count_cmd(pcmd->conn_handle, pRet);
}

ble_sts_t blc_ll_cis_iso_test_end_cmd(u16 connHandle, hci_le_isoTestEndStatusParam_t *pRetParam)
{
    iso_test_param_t *pIsoTest = NULL;

    pRetParam->status                = BLE_SUCCESS;
    pRetParam->conn_handle           = connHandle;
    pRetParam->failed_packet_count   = 0;
    pRetParam->miss_packet_count     = 0;
    pRetParam->received_packet_count = 0;

    ll_cis_conn_t *pCis = blt_isCisAllocated_by_handle(connHandle);

    if ((pCis == NULL) || (pCis->conState != CONN_STATUS_ESTABLISH)) {
        pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
    }

    pIsoTest = pCis->pCisTestParam;

    if ((pIsoTest == NULL) || (pIsoTest->isoTestMode == ISO_TEST_DISABLE)) {
        pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
    }

    pCis->cisSduIn_wptr    = pCis->cisSduIn_rptr;
    pCis->cisPduTxFifoWptr = pCis->cisPduTxFifoRptr;


    if ((pRetParam->status == BLE_SUCCESS) && (pIsoTest->isoTestMode == ISO_RECEIVE_MODE)) {
        pRetParam->failed_packet_count   = pIsoTest->recMode.failedCnt;
        pRetParam->miss_packet_count     = pIsoTest->recMode.missedCnt;
        pRetParam->received_packet_count = pIsoTest->recMode.successCnt;
    }

    if ((pRetParam->status == BLE_SUCCESS) && (pIsoTest != NULL)) {
        pIsoTest->isoTestMode = ISO_TEST_DISABLE;
    }


    return pRetParam->status;
}

ble_sts_t blc_hci_cis_iso_test_end_cmd(hci_le_isoTestEndCmdParams_t *pCmd, hci_le_isoTestEndStatusParam_t *pRet)
{
    return blc_ll_cis_iso_test_end_cmd(pCmd->conn_handle, pRet);
}

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    void
    blt_cis_test_process(ll_cis_conn_t *pCisConn)
{
    u16 max_sdu      = pCisConn->max_sdu_loca;
    u32 sdu_interval = pCisConn->sdu_int_loca_us;

    if (pCisConn->pCisTestParam->isoTestMode == ISO_TEST_TRANSMIT_MODE) {
        sdu_packet_t *sdu = (sdu_packet_t *)(pCisConn->cis_sduInBuf + sduCisMng.max_in_fifo_size * (pCisConn->cisSduIn_wptr & sduCisMng.in_fifo_mask));

        if (!blt_iso_test_transmit_mainloop(pCisConn->pCisTestParam, sdu_interval, max_sdu, sdu, pCisConn->cis_frame)) {
            sdu->numHciPkt = 0;
            pCisConn->cisSduIn_wptr++;

            DBG_SIHUI_CHN11_TOGGLE;
            tlkapi_send_string_u8s(IUT_HCI_LOG_EN, "[CIS][TST] TX_TEST", pCisConn->cis_connHandle, pCisConn->pCisTestParam->tranMode.send_pkt_cnt, pCisConn->cisSduIn_wptr, max_sdu);

    #if 0 //debug
                if((pCisConn->cis_connHandle == 0x22){
                    DBG_C HN11_TOGGLE;
                }
                else{
                    DBG_C HN12_TOGGLE;
                }
    #endif
        }
    } else if (pCisConn->pCisTestParam->isoTestMode == ISO_RECEIVE_MODE) {
        if (!pCisConn->cis_rx_stream_start) {
            pCisConn->cisSduOut_wptr = pCisConn->cisSduOut_rptr;
            return;
        }

        if (pCisConn->cisSduOut_rptr != pCisConn->cisSduOut_wptr) {
            sdu_packet_t *sdu = (sdu_packet_t *)(pCisConn->cis_sduOutBuf + sduCisMng.max_out_fifo_size * (pCisConn->cisSduOut_rptr & (sduCisMng.out_fifo_mask)));

            blt_iso_test_receive_mainloop(sdu, pCisConn->pCisTestParam, max_sdu, pCisConn->cis_frame);

            pCisConn->cisSduOut_rptr++;
        }
    }
}

_attribute_noinline_ int blt_cis_cmd_process_task(int opcode, void *pCmd, void *pRet)
{
    //todo: change to switch case

    /* pay attention that opcode include BR/EDR & LE, need consider carefully,
     * now only DISCONNECT CMD 0x06, not conflict with LE part. 20220803 */
    if (opcode == HCI_CMD_DISCONNECT) {
        hci_disconnect_cmdParam_t *pDiscon = (hci_disconnect_cmdParam_t *)pCmd;
        return blc_ll_cis_disconnect(pDiscon->connHandle, pDiscon->reason);
    } else if (opcode == HCI_CMD_LE_READ_ISO_TX_SYNC) {
        u8 *p          = (u8 *)pCmd;
        u16 cis_handle = *p | *(p + 1) << 8;
        return blc_ll_read_cis_tx_sync(cis_handle, (hci_le_readIsoTxSync_retParam_t *)pRet);
    } else if (opcode == HCI_CMD_LE_SETUP_ISO_DATA_PATH) {
        return blc_hci_le_setupCisDataPath((hci_le_setupIsoDataPath_cmdParam_t *)pCmd, (hci_le_setupIsoDataPath_retParam_t *)pRet);
    } else if (opcode == HCI_CMD_LE_REMOVE_ISO_DATA_PATH) {
        return blc_hci_le_removeCisDataPath((hci_le_rmvIsoDataPath_cmdParam_t *)pCmd, (hci_le_rmvIsoDataPath_retParam_t *)pRet);
    } else if (opcode == HCI_CMD_LE_ISO_TRANSMIT_TEST) {
        return blc_hci_cis_iso_transmit_test_cmd((hci_le_isoTestCmdParams_t *)pCmd, (hci_le_isoTestRetParams_t *)pRet);
    } else if (opcode == HCI_CMD_LE_ISO_RECEIVE_TEST) {
        return blc_hci_cis_iso_receive_test((hci_le_isoTestCmdParams_t *)pCmd, (hci_le_isoTestRetParams_t *)pRet);
    } else if (opcode == HCI_CMD_LE_ISO_READ_TEST_COUNTERS) {
        return blc_hci_cis_read_test_count_cmd((hci_le_isoReadTestCountsCmdParams_t *)pCmd, (hci_le_isoRxTestStatusParam_t *)pRet);
    } else if (opcode == HCI_CMD_LE_ISO_TEST_END) {
        return blc_hci_cis_iso_test_end_cmd((hci_le_isoTestEndCmdParams_t *)pCmd, (hci_le_isoTestEndStatusParam_t *)pRet);
    } else if (opcode == HCI_CMD_LE_ISO_DATA) {
        return blc_hci_le_pushCisData((iso_data_packet_t *)pCmd);
    }


    return 0;
}

ble_sts_t blc_ll_read_cis_tx_sync(u16 cisHandle, hci_le_readIsoTxSync_retParam_t *pRetParam)
{
    /*
    If the Host issues this command with a connection handle that does not exist,
    or the connection handle is not associated with a CIS or BIS, the Controller
    shall return the error code Unknown Connection Identifier (0x02).
    If the Host issues this command on an existing connection handle for a CIS or
    BIS that is not configured for transmitting SDUs, the Controller shall return the
    error code Command Disallowed (0x0C).
    If the Host issues this command before an SDU has been transmitted by the
    Controller, the Controller shall return the error code Command Disallowed
    (0x0C).
    */
    ll_cis_conn_t *pCisConn = blt_isCisEstablished_by_handle(cisHandle);
    if (pCisConn) {
        if (!pCisConn->bn_loca || !pCisConn->cis_tx_stream_start) {
            pRetParam->status = HCI_ERR_CMD_DISALLOWED;
        } else {
            pRetParam->status     = BLE_SUCCESS;
            pRetParam->connHandle = cisHandle;

            pRetParam->pkt_seqno = pCisConn->cisSendPldNum;
            pRetParam->tx_ts     = 0;
            u32 pkt_to           = 0;
            smemcpy(&pRetParam->time_offset, &pkt_to, 3);
        }
    } else {
        pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
    }

    return pRetParam->status;
}

ble_sts_t blc_ll_setupCisDataPath(u16 cis_handle, dat_path_dir_t dir, dat_path_id_t id, u8 cid_assignNum, u16 cidcompId, u16 cid_vendorDef, u32 control_dly, u8 codec_cfg_len, u8 codec_cfg1, u8 codec_cfg2, u8 codec_cfg3, u8 codec_cfg4)
{
    (void)cid_assignNum; //unused, remove warning
    (void)cidcompId;     //unused, remove warning
    (void)cid_vendorDef; //unused, remove warning
    (void)control_dly;   //unused, remove warning
    (void)codec_cfg_len; //unused, remove warning
    (void)codec_cfg1;    //unused, remove warning
    (void)codec_cfg2;    //unused, remove warning
    (void)codec_cfg3;    //unused, remove warning
    (void)codec_cfg4;    //unused, remove warning

    /*
     *  If the Host issues this command more than once for the same Connection_Handle and direction before issuing the HCI_LE_Remove_ISO_Data_-
        Path command for that Connection_Handle and direction, the Controller shall
        return the error code Command Disallowed (0x0C).                                        Done !!!

        If the Host issues this command for a CIS on a Peripheral before it has issued
        the HCI_LE_Accept_CIS_Request command for that CIS, then the Controller
        shall return the error code Command Disallowed (0x0C).                                  Done !!!

        If the Host issues this command for a vendor-specific data transport path that
        has not been configured using the HCI_Configure_Data_Path command, the
        Controller shall return the error code Command Disallowed (0x0C).

        If the Host attempts to set a data path with a Connection Handle that does not
        exist or that is not for a CIS, CIS configuration, or BIS, the Controller shall
        return the error code Unknown Connection Identifier (0x02).                             Done !!!

        If the Host attempts to set an output data path using a connection handle that is
        for an Isochronous Broadcaster, for an input data path on a Synchronized
        Receiver, or for a data path for the direction on a unidirectional CIS where BN
        is set to 0, the Controller shall return the error code Command Disallowed
        (0x0C).                                                                                 Done !!!

        If the Host issues this command with Codec_Configuration_Length non-zero
        and Codec_ID set to transparent air mode, the Controller shall return the error
        code Invalid HCI Command Parameters (0x12).

        If the Host issues this command with codec-related parameters that exceed the
        bandwidth and latency allowed on the established CIS or BIS identified by the
        Connection_Handle parameter, the Controller shall return the error code
        Invalid HCI Command Parameters (0x12).
     */
    //  if(codec_cfg_len > 4){
    //      return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    //  }


    ll_cis_conn_t *pCisConn = blt_isCisAllocated_by_handle(cis_handle);
    if (pCisConn) {
        if (pCisConn->cisRole == CIS_ROLE_SLAVE && pCisConn->ack_CisReq != CIS_REQ_ACCEPT) {
            return HCI_ERR_CMD_DISALLOWED;
        }


        /* para */
        if (dir == Data_Dir_Input) { /* input data path */
            //tlkapi_send_string_u32s(0, "DEBUG DATA IN", pCisConn->bn_loca, pCisConn->cis_dapth_setup, 0, 0);
            if (!pCisConn->bn_loca) {
                return HCI_ERR_CMD_DISALLOWED;
            } else if (pCisConn->cis_dapth_setup & DATA_PATH_INPUT_FLAG) {
                return HCI_ERR_CMD_DISALLOWED; // HCI/CIS/BV-02-C test this logic
            }

            pCisConn->cis_dapth_setup |= DATA_PATH_INPUT_FLAG;
        } else if (dir == Data_Dir_Output) { /* output data path */
            if (!pCisConn->bn_peer) {
                return HCI_ERR_CMD_DISALLOWED;
            } else if (pCisConn->cis_dapth_setup & DATA_PATH_OUTPUT_FLAG) {
                return HCI_ERR_CMD_DISALLOWED;
            }

            pCisConn->cis_dapth_setup |= DATA_PATH_OUTPUT_FLAG;
        }

        pCisConn->dpId = id;
    } else {
        return HCI_ERR_UNKNOWN_CONN_ID; // HCI/CIS/BV-02-C test this logic
    }


    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setupCisDataPath(hci_le_setupIsoDataPath_cmdParam_t *pCmdPara, hci_le_setupIsoDataPath_retParam_t *pRetParam)
{
    u32 controller_delay = pCmdPara->control_delay[0] | pCmdPara->control_delay[1] << 8 | pCmdPara->control_delay[2] << 16;
    pRetParam->status    = blc_ll_setupCisDataPath(pCmdPara->conn_handle, pCmdPara->data_path_dir, pCmdPara->data_path_id, pCmdPara->codec_id_assignNum, pCmdPara->codec_id_compId, pCmdPara->codec_id_vendorDef, controller_delay, pCmdPara->codec_config_len, pCmdPara->codec_config[0], pCmdPara->codec_config[1], pCmdPara->codec_config[2], pCmdPara->codec_config[3]);

    pRetParam->conn_handle = pCmdPara->conn_handle;


    return pRetParam->status;
}

ble_sts_t blc_ll_removeCisDataPath(u16 cis_handle, dp_dir_msk_t dir_mask)
{
    /*
    If the Host issues this command with a Connection_Handle that does not exist
    or is not for a CIS, CIS configuration, or BIS, the Controller shall return the
    error code Unknown Connection Identifier (0x02).                                    Done !!!

    If the Host issues this command for a data path that has not been set up (using
    the HCI_LE_Setup_ISO_Data_Path command), the Controller shall return the
    error code Command Disallowed (0x0C).                                               Done !!!
    */
    ll_cis_conn_t *pCisConn = blt_isCisAllocated_by_handle(cis_handle);
    if (pCisConn) {
        if (!(pCisConn->cis_dapth_setup & dir_mask)) {
            return HCI_ERR_CMD_DISALLOWED;
        }

        pCisConn->cis_dapth_setup &= ~dir_mask;
        pCisConn->dpId = Data_Path_Disable;
    } else {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }


    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_removeCisDataPath(hci_le_rmvIsoDataPath_cmdParam_t *pCmdPara, hci_le_rmvIsoDataPath_retParam_t *pRetParam)
{
    pRetParam->conn_handle = pCmdPara->conn_handle;
    pRetParam->status      = blc_ll_removeCisDataPath(pCmdPara->conn_handle, pCmdPara->dp_dir_mask);

    return pRetParam->status;
}

ble_sts_t blt_ll_pushCisData(u16 connHandle, iso_pb_flag_t PB_Flag, u8 TS_Flag, u32 time_stamp, u16 seqnum, u16 total_len, u16 cur_len, u8 *pData)
{
    #if (IUT_HCI_LOG_EN)
    tlkapi_send_string_u32s(IUT_HCI_LOG_EN, "HCI ISO info 2", connHandle, PB_Flag, total_len, cur_len);
    int dump_len = cur_len > 256 ? 256 : cur_len;
    tlkapi_send_string_data(IUT_HCI_LOG_EN, "@HCI_ISO_SDU in", pData, dump_len);
    #endif

    ll_cis_conn_t *pCisConn = blt_isCisEstablished_by_handle(connHandle);
    if (pCisConn == NULL || !pCisConn->cis_established) {
        tlkapi_send_string_u32s(HOST_HCI_ERR_LOG_EN, "HCI ERROR, CIS not establish", connHandle, 0, 0, 0);
        BLMS_ERR_DEBUG(HOST_HCI_ERR_LOG_EN, 0x99C80000);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    // cis channel parameter not allow transmit data
    if (!pCisConn->max_sdu_loca || !pCisConn->max_pdu_loca || !pCisConn->bn_loca) {
        tlkapi_send_string_u8s(HOST_HCI_ERR_LOG_EN, "HCI ERROR, cis data not allowed", pCisConn->max_sdu_loca, pCisConn->max_pdu_loca, pCisConn->bn_loca, pCisConn->max_sdu_loca);
        BLMS_ERR_DEBUG(HOST_HCI_ERR_LOG_EN, 0x99C80000);
        return LL_ERR_INVALID_PARAMETER;
    }

    // HCI_ISO_SDU_FIRST_FRAG or HCI_ISO_SDU_COMPLETE: total_len > max_in_fifo_size, total_len > max_sdu_loca
    if (!(PB_Flag & 1) && total_len > sduCisMng.max_in_fifo_size) {
        tlkapi_send_string_u32s(HOST_HCI_ERR_LOG_EN, "HCI ERROR, iso data too long", total_len, sduCisMng.max_in_fifo_size, pCisConn->max_sdu_loca, 0);
        BLMS_ERR_DEBUG(HOST_HCI_ERR_LOG_EN, 0x99C80000);
        return HCI_ERR_PACKET_TOO_LONG;
    }


    sdu_packet_t *iso_sdu = (sdu_packet_t *)(pCisConn->cis_sduInBuf + (pCisConn->cisSduIn_wptr & (sduCisMng.in_fifo_mask)) * sduCisMng.max_in_fifo_size);

    if (blt_iso_process_sdu_in_data(iso_sdu, PB_Flag, TS_Flag, time_stamp, seqnum, total_len, cur_len, pData)) {
        pCisConn->cisSduIn_wptr++;
    } else {
        if (blmsParam.standard_hci_en) {
            //blt_cis_mark_numOfcmpEvt_status(pCisConn, numOfcmpPkt);
        }
    }


    return BLE_SUCCESS;
}

int blc_ll_getCisSduInBufferFreeNum(u16 cisHandle)
{
    int            num      = 0;
    ll_cis_conn_t *pCisConn = blt_isCisEstablished_by_handle(cisHandle);

    if (pCisConn != NULL) {
        num = (pCisConn->cisSduIn_wptr - pCisConn->cisSduIn_rptr) & sduCisMng.in_fifo_mask;
    }
    return num;
}

    #if (!FANQH_OPTIMIZE_BIS_API)
// blc_ll_pushCisData
ble_sts_t blc_cis_sendData(u16 cisHandle, u8 *pData, u16 len)
{
    return blt_ll_pushCisData(cisHandle, HCI_ISO_SDU_COMPLETE, 0, 0, 0, len, len, pData);
}
    #endif


void blc_isoal_clearCisSdu(u16 cis_connHandle)
{
    u8             cis_sel   = cis_connHandle & BLT_CIS_IDX_MSK;
    ll_cis_conn_t *pCisConn  = (ll_cis_conn_t *)(global_pCisConn + cis_sel);
    pCisConn->cisSduOut_wptr = pCisConn->cisSduOut_rptr = 0;
    bltCisRxEvt.rptr = bltCisRxEvt.wptr = 0;
}


    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #endif
    ble_sts_t
    blc_hci_le_pushCisData(iso_data_packet_t *pIsoDatPkt)
{
    if (pIsoDatPkt->pb & 1) // HCI_ISO_SDU_CONTINUE_FRAG   HCI_ISO_SDU_LAST_FRAG
    {
        return blt_ll_pushCisData(pIsoDatPkt->connHandle, pIsoDatPkt->pb, 0, 0, 0, 0, pIsoDatPkt->iso_dat_len, pIsoDatPkt->p_ISO_data_load);
    } else                  // HCI_ISO_SDU_FIRST_FRAG   HCI_ISO_SDU_COMPLETE
    {
        if (pIsoDatPkt->ts) {
            iso_data_load_1_t *pIso_load = (iso_data_load_1_t *)pIsoDatPkt->p_ISO_data_load;
            if (pIsoDatPkt->pb == HCI_ISO_SDU_COMPLETE && (pIso_load->iso_sdu_len + 8) != pIsoDatPkt->iso_dat_len) {
                tlkapi_send_string_u32s(HOST_HCI_ERR_LOG_EN, "HCI ERROR, ISO data length not match ISO SDU length", pIsoDatPkt->iso_dat_len, pIso_load->iso_sdu_len, pIsoDatPkt->ts, 0);
                return HCI_ERR_INVALID_HCI_CMD_PARAMS;
            }

            return blt_ll_pushCisData(pIsoDatPkt->connHandle, pIsoDatPkt->pb, 1, pIso_load->timestamp, pIso_load->pkt_seq, pIso_load->iso_sdu_len, pIsoDatPkt->iso_dat_len - 8, pIso_load->iso_sdu);
        } else {
            iso_data_load_2_t *pIso_load = (iso_data_load_2_t *)pIsoDatPkt->p_ISO_data_load;
            if (pIsoDatPkt->pb == HCI_ISO_SDU_COMPLETE && (pIso_load->iso_sdu_len + 4) != pIsoDatPkt->iso_dat_len) {
                tlkapi_send_string_u32s(HOST_HCI_ERR_LOG_EN, "HCI ERROR, ISO data length not match ISO SDU length", pIsoDatPkt->iso_dat_len, pIso_load->iso_sdu_len, pIsoDatPkt->ts, 0);
                return HCI_ERR_INVALID_HCI_CMD_PARAMS;
            }

            return blt_ll_pushCisData(pIsoDatPkt->connHandle, pIsoDatPkt->pb, 0, 0, pIso_load->pkt_seq, pIso_load->iso_sdu_len, pIsoDatPkt->iso_dat_len - 4, pIso_load->iso_sdu);
        }
    }
}

/**
 * @brief      This function is get the next sdu fifo without removing it from FIFO
 * @param[in]  cis_connHandle - point to handle of cis.
 * @return      return the SDU point
 */
sdu_packet_t *blc_isoal_peekCisSdu(u16 cis_connHandle)
{
    sdu_packet_t  *sdu      = NULL;
    u8             cis_sel  = cis_connHandle & BLT_CIS_IDX_MSK;
    ll_cis_conn_t *pCisConn = (ll_cis_conn_t *)(global_pCisConn + cis_sel);

    if (pCisConn->conState != CONN_STATUS_ESTABLISH) {
        return NULL;
    }

    if (pCisConn->cisSduOut_wptr != pCisConn->cisSduOut_rptr) {
        sdu = (sdu_packet_t *)(pCisConn->cis_sduOutBuf + sduCisMng.max_out_fifo_size * (pCisConn->cisSduOut_rptr & (sduCisMng.out_fifo_mask)));
    }

    return sdu;
}

sdu_packet_t *blc_ll_popCisRxSduData(u16 cis_connHandle)
{
    sdu_packet_t  *sdu      = NULL;
    u8             cis_sel  = cis_connHandle & BLT_CIS_IDX_MSK;
    ll_cis_conn_t *pCisConn = (ll_cis_conn_t *)(global_pCisConn + cis_sel);

    if (pCisConn->conState != CONN_STATUS_ESTABLISH) {
        return NULL;
    }

    if (pCisConn->cisSduOut_wptr != pCisConn->cisSduOut_rptr) {
        sdu = (sdu_packet_t *)(pCisConn->cis_sduOutBuf + sduCisMng.max_out_fifo_size * (pCisConn->cisSduOut_rptr & (sduCisMng.out_fifo_mask)));

        //      if(tick1_exceed_tick2( clock_time(), sdu->timestamp))
        //      {
        pCisConn->cisSduOut_rptr++;
        //      }
        //      else{
        //          sdu= NULL;
        //      }
    }

    return sdu;
}

void blc_cis_get_tx_point(u8 role)
{
    if (role == 1) {
        while (1) {
            u32 clock_tick = clock_time();
            if ((clock_tick - blt_pCigMst->cigm_trigger_tick + 10 * 1000 * SYSTEM_TIMER_TICK_1US) < 300 * SYSTEM_TIMER_TICK_1US || (blt_pCigMst->cigm_trigger_tick + 10 * 1000 * SYSTEM_TIMER_TICK_1US - clock_tick) < 300 * SYSTEM_TIMER_TICK_1US) {
                break;
            }
        }
        delay_ms(1); //margin
    } else if (role == 0) {
        while (1) {
            u32 clock_tick = clock_time();
            if ((clock_tick - blt_pCisSlv->cigs_expect_tick + 10 * 1000 * SYSTEM_TIMER_TICK_1US) < 300 * SYSTEM_TIMER_TICK_1US || (blt_pCisSlv->cigs_expect_tick + 10 * 1000 * SYSTEM_TIMER_TICK_1US - clock_tick) < 300 * SYSTEM_TIMER_TICK_1US) {
                break;
            }
        }
        delay_ms(1); //margin
    }
}


#endif //end of LL_FEATURE_ENABLE_CONNECTED_ISO
