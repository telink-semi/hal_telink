/********************************************************************************************************
 * @file    l2cap.c
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
#include "stack/ble/host/ble_host.h"
#include "stack/ble/controller/ble_controller.h"


_attribute_ble_data_retention_  _attribute_aligned_(4) l2cap_buff_t l2cap_buff_m;
_attribute_ble_data_retention_  _attribute_aligned_(4) l2cap_buff_t l2cap_buff_s;
_attribute_ble_data_retention_  l2cap_buff_t *pl2cap_buff = NULL;
_attribute_ble_data_retention_  u8 *pblms_rxbuff = NULL;

///////////////////L2CAP Connection-oriented channel(CoC)/////////////////////////
_attribute_ble_data_retention_  coc_data_handler_t          coc_data_handler = NULL;
_attribute_ble_data_retention_  coc_disconnect_handler_t    coc_disconnect_handler = NULL;

///////////////////////////////////
// central and peripheral L2CAP buff manage
///////////////////////////////////
ble_sts_t   blc_l2cap_initAclCentralBuffer(u8 *pRxBuf, u16 rx_buf_size, u8 *pTxBuf, u16 tx_buf_size)
{
    l2cap_buff_m.rx_p = pRxBuf;
    l2cap_buff_m.max_rx_size = rx_buf_size;
    l2cap_buff_m.tx_p = pTxBuf;
    l2cap_buff_m.max_tx_size = tx_buf_size;

    return BLE_SUCCESS;
}

ble_sts_t   blc_l2cap_initAclPeripheralBuffer(u8 *pRxBuf, u16 rx_buf_size, u8 *pTxBuf, u16 tx_buf_size)
{
    l2cap_buff_s.rx_p = pRxBuf;
    l2cap_buff_s.max_rx_size = rx_buf_size;

    l2cap_buff_s.tx_p = pTxBuf;
    l2cap_buff_s.max_tx_size = tx_buf_size;

    return BLE_SUCCESS;
}

u16 blt_l2cap_getAclRxBufferSize(void)
{
    if(l2cap_buff_m.max_rx_size && l2cap_buff_s.max_rx_size)
        return min(l2cap_buff_s.max_rx_size, l2cap_buff_m.max_rx_size);

    return max(l2cap_buff_s.max_rx_size, l2cap_buff_m.max_rx_size);
}

u16 blt_l2cap_getAclPeripheralRxBufferSize(void)
{
    return l2cap_buff_s.max_rx_size;
}

//Get mtu buff of slave
u8*  blt_l2cap_get_s_tx_buff(u16 connHandle)
{
    u8 h = connHandle & CONN_IDX_MASK;

    if(h<LL_MAX_ACL_CEN_NUM || h>=LL_MAX_ACL_CONN_NUM)
        return NULL;
    else
    {
        u8 conn_slave_idx = (connHandle & CONN_IDX_MASK) - LL_MAX_ACL_CEN_NUM;

        return l2cap_buff_s.tx_p + conn_slave_idx*l2cap_buff_s.max_tx_size;
    }
}


//Get mtu buff of master
u8*  blt_l2cap_get_m_tx_buff(u16 connHandle)
{
    u8 h = connHandle & CONN_IDX_MASK;
    if(h>=LL_MAX_ACL_CEN_NUM)
        return NULL;
    else
    {
        u8 conn_master_idx = (connHandle & CONN_IDX_MASK);
        return l2cap_buff_m.tx_p + conn_master_idx*l2cap_buff_m.max_tx_size;
    }
}

u8* blt_l2cap_get_tx_buff(u16 connHandle)
{
    u8 h = connHandle & CONN_IDX_MASK;

    if((connHandle & BLM_CONN_HANDLE) && h<LL_MAX_ACL_CEN_NUM)
    {
        return l2cap_buff_m.tx_p + h*l2cap_buff_m.max_tx_size;
    }
    else if((connHandle & BLS_CONN_HANDLE) && h >= LL_MAX_ACL_CEN_NUM && h < LL_MAX_ACL_CONN_NUM)
    {
        return l2cap_buff_s.tx_p + (h-LL_MAX_ACL_CEN_NUM)*l2cap_buff_s.max_tx_size;
    }

    return NULL;

}

//reassembly link lay packet to l2cap SDU
u8 * blt_l2cap_pktPack(u16 connHandle, u8 * raw_pkt) //raw_pkt has been removed dma header 4B
{

    rf_packet_l2cap_req_t *pl = (rf_packet_l2cap_req_t *) (raw_pkt);
    u8 type = pl->type & 3;
    u8 idx = connHandle & CONN_IDX_MASK;

    if(connHandle & BLM_CONN_HANDLE) //master
    {
        pl2cap_buff = &l2cap_buff_m;
        pblms_rxbuff = l2cap_buff_m.rx_p + idx*l2cap_buff_m.max_rx_size;
    }
    else //slave
    {
        pl2cap_buff = &l2cap_buff_s;
        pblms_rxbuff = l2cap_buff_s.rx_p +(idx-LL_MAX_ACL_CEN_NUM)*l2cap_buff_s.max_rx_size;
    }

    //------------- LL L2CAP start packet ---------------------------------
    if (type == L2CAP_FIRST_PKT_C2H)
    {
        if (pl->rf_len == (pl->l2capLen + 4))//complete l2cap packet
        {
            return (u8 *)(raw_pkt);
        }
        else
        {
            if(pblms_rxbuff==NULL)
            {
//              write_reg32(0x40000,0x55555555);  //for debug
                return NULL;
            }

            if (pl->l2capLen + 4 > pl->rf_len)//start pkt,next pkt will come
            {
                u16 len = pl->rf_len + 2; //header
                pl->rf_len = pl->l2capLen + 4;

                memcpy(pblms_rxbuff, raw_pkt, len);
                U16_SET(pblms_rxbuff, len);
            }
            else
            {
                U16_SET(pblms_rxbuff, 0);
            }
        }
    }
    //------------- LL L2CAP continuous packet ---------------------------------
    else if (type == L2CAP_CONTINUING_PKT )
    {
        if((pblms_rxbuff==NULL)|| !U16_GET(pblms_rxbuff))
        {
//          write_reg32(0x40000,0x55555555);  //for debug
//          printf("ero,%x, %x", pblms_rxbuff[0],pblms_rxbuff);

            return NULL;
        }

        u16 att_len = U16_GET(pblms_rxbuff)-2 -4 + pl->rf_len;
        //if(att_len > blmsMtu[idx].effective_MTU)//received att_len > mtu_size
        if(att_len > pl2cap_buff->max_rx_size)
        {
//          printf("excess MTU");//todo add gap event

            U16_SET(pblms_rxbuff, 0);
            return (u8 *)pblms_rxbuff;
        }

        memcpy(pblms_rxbuff + U16_GET(pblms_rxbuff), raw_pkt + 2, pl->rf_len);
        U16_SET(pblms_rxbuff, U16_GET(pblms_rxbuff) + pl->rf_len);
        rf_packet_l2cap_req_t *ps = (rf_packet_l2cap_req_t *)pblms_rxbuff;
        if (U16_GET(pblms_rxbuff) >= ps->l2capLen + (2 + 4))
        {
//          printf("%d",U16_GET(pblms_rxbuff)-10-3);// to check l2cap packet reassembly
            U16_SET(pblms_rxbuff, 0);
            return (u8 *)pblms_rxbuff;
        }
    }
    return NULL;
}

static void blt_l2cap_smpDataControl(u16 connHandle, rf_packet_l2cap_t *ptrSMP)
{
    my_dump_str_data(RX_L2CAP_DATA_LOG, "RX SMP", (u8*)ptrSMP, ptrSMP->rf_len + 2);  //debug
    u8* pr = blt_smp_l2capSmpCmdHandler(connHandle, (u8*)ptrSMP);
    if (pr){
        ll_push_tx_fifo_handler (connHandle | HANDLE_STK_FLAG, pr); //remove ATT hold, send insufficient security
    }
}
static void blt_l2cap_signalDataControl(u16 connHandle, rf_packet_l2cap_t *ptrSig)
{
    my_dump_str_data(RX_L2CAP_DATA_LOG, "RX SIG", (u8*)ptrSig, ptrSig->rf_len + 2);  //debug
    extern int blt_signal_l2capSignalRxHandler (u16 connHandle, l2cap_pkt_t *ptrSignal);
    blt_signal_l2capSignalRxHandler(connHandle, (l2cap_pkt_t*)&ptrSig->l2capLen);

}
static void blt_l2cap_attrDataControl(u16 connHandle, rf_packet_l2cap_t *ptrAttr)
{
    my_dump_str_data(0, "RX ATT", (u8*)&ptrAttr->opcode, ptrAttr->l2capLen);  //debug
    if(gatt_data_handler && gatt_data_handler(connHandle, (u8*) ptrAttr)){
        my_dump_str_data(0, "l2cap att data channel was registered by user's callback", 0, 0);  //debug
        return ;
    }
    extern int blt_att_l2capAttRxHandler (u16 connHandle, l2cap_pkt_t *ptrAttr);
    blt_att_l2capAttRxHandler(connHandle, (l2cap_pkt_t*)&ptrAttr->l2capLen);
}
//process l2cap SDU
int blc_l2cap_pktHandler_5_3(u16 connHandle, u8 *raw_pkt) //raw_pkt has been removed dma header 4B
{
    //l2cap data packeted, make sure that user see complete l2cap data
    rf_packet_l2cap_t* pkt = (rf_packet_l2cap_t*) blt_l2cap_pktPack (connHandle, raw_pkt);
    if (pkt){
        BLT_HOST_DBUG(DBG_L2CAP_BTSNOOP_LOG, "[BTSNOOP]rx l2cap data is 0x%02x%02x%02x%02x%s", connHandle&0xFF, connHandle>>8, (pkt->l2capLen + 4)&0xFF, (pkt->l2capLen + 4)>>8, hex_to_str(&pkt->l2capLen, pkt->l2capLen + 4));
        if(pkt->chanId == L2CAP_CID_ATTR_PROTOCOL){     //CID ==> ATT
            tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] L2CAP_CID_ATTR_PROTOCOL", &(pkt->l2capLen), pkt->rf_len);
            blt_l2cap_attrDataControl(connHandle, pkt);
        }
        else if(pkt->chanId == L2CAP_CID_SIG_CHANNEL){      //CID ==> SIG
            tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] L2CAP_CID_SIG_CHANNEL", &(pkt->l2capLen), pkt->rf_len);
            blt_l2cap_signalDataControl(connHandle, pkt);
        }
        else if(pkt->chanId == L2CAP_CID_SMP){      //CID ==> SMP
            tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] L2CAP_CID_SMP", &(pkt->l2capLen), pkt->rf_len);
            blt_l2cap_smpDataControl(connHandle, pkt);
        }
    #if (LL_ASYNC_LEA_EN)
        else if(pkt->chanId == L2CAP_CID_NULL){         //CID ==> NULL
            if(asyncCtrl.leaUsed)
            {
                blt_l2cap_asyncLeaDataControl(connHandle, pkt);
            }
        }
    #endif
        else{
            #if L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN
            if(coc_data_handler)
                coc_data_handler(connHandle, (l2cap_pkt_t*)&pkt->l2capLen); //blt_l2cap_cocDataControl
            #endif
        }
    }
    return 0;
}



//process l2cap SDU
int blc_l2cap_pktHandler (u16 connHandle, u8 *raw_pkt) //raw_pkt has been removed dma header 4B
{
    
    //l2cap data packeted, make sure that user see complete l2cap data
    u8* pkt = blt_l2cap_pktPack (connHandle, raw_pkt);

    if (pkt){
        rf_packet_l2cap_t *ptrL2cap = (rf_packet_l2cap_t*)pkt;

        if(ptrL2cap->chanId == L2CAP_CID_ATTR_PROTOCOL){        //CID ==> ATT
            u8 opcode = ptrL2cap->opcode;
            tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] L2CAP_CID_ATTR_PROTOCOL", &(ptrL2cap->l2capLen), ptrL2cap->rf_len);
            my_dump_str_data(RX_L2CAP_DATA_LOG, "RX ATT", (u8*)ptrL2cap, ptrL2cap->rf_len + 2);  //debug

            if(opcode == ATT_OP_EXCHANGE_MTU_REQ || opcode == ATT_OP_EXCHANGE_MTU_RSP){ //att opcode m/s role common process
                rf_packet_att_mtu_exchange_t *pMtu = (rf_packet_att_mtu_exchange_t*)ptrL2cap;
                gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);

                if(pGap_ms_para==NULL)
                    return 1;

                if(connHandle & BLM_CONN_HANDLE) //master
                {
                    pl2cap_buff = &l2cap_buff_m;
                }
                else //slave
                {
                    pl2cap_buff = &l2cap_buff_s;
                }

                if(opcode ==  ATT_OP_EXCHANGE_MTU_REQ){
                    rf_packet_att_mtu_t pkt_mtu_rsp = {     //  spec 4.1 ,  3.4.7.1 Handle Value Notification
                        0x02,                                       // type
                        sizeof(rf_packet_att_mtu_t) - 2,            // rf_len
                        sizeof(rf_packet_att_mtu_t) - 6,            // l2cap_len
                        4,                                          // chanId
                        ATT_OP_EXCHANGE_MTU_RSP,
                        {ATT_MTU_SIZE, 0}
                    };

                    pkt_mtu_rsp.mtu[0] = pl2cap_buff->init_MTU &0xff;
                    pkt_mtu_rsp.mtu[1] = (pl2cap_buff->init_MTU>>8)&0xff;
                    u8 *pr = (u8*)&pkt_mtu_rsp;
                    blt_ll_pushTxfifoHold (connHandle, pr);
                }

                pGap_ms_para->mtu_exg_pending = 0;

                u16 peer_mtu_size = (pMtu->mtu[0] | pMtu->mtu[1]<<8);
                pGap_ms_para->effective_MTU = min(pl2cap_buff->init_MTU, peer_mtu_size);

                if(gap_eventMask & GAP_EVT_MASK_ATT_EXCHANGE_MTU){
                    u8 param_evt[8];
                    gap_gatt_mtuSizeExchangeEvt_t *pEvt = (gap_gatt_mtuSizeExchangeEvt_t *)param_evt;
                    pEvt->connHandle = connHandle;
                    pEvt->peer_MTU = peer_mtu_size;
                    pEvt->effective_MTU = pGap_ms_para->effective_MTU;

                    blc_gap_send_event ( GAP_EVT_ATT_EXCHANGE_MTU, param_evt, sizeof(gap_gatt_mtuSizeExchangeEvt_t) );
                }
            }
            //now process in stack only for slave which has a GATT service table. TODO: process slave multi_mac; master... by SiHui
            else if( (connHandle & BLS_CONN_HANDLE) && !(opcode & 0x01) ){
                // ATT_OP_FIND_INFO_REQ                0x04
                // ATT_OP_FIND_BY_TYPE_VALUE_REQ       0x06
                // ATT_OP_READ_BY_TYPE_REQ             0x08
                // ATT_OP_READ_REQ                     0x0a
                // ATT_OP_READ_BLOB_REQ                0x0c
                // ATT_OP_READ_MULTI_REQ               0x0e
                // ATT_OP_READ_BY_GROUP_TYPE_REQ       0x10
                // ATT_OP_WRITE_REQ                    0x12
                // ATT_OP_PREPARE_WRITE_REQ            0x16
                // ATT_OP_EXECUTE_WRITE_REQ            0x18
                // ATT_OP_HANDLE_VALUE_CFM             0x1e
                // ATT_OP_WRITE_CMD                    0x52
                // ATT_OP_SIGNED_WRITE_CMD             0xd2

                u8 *pr = bls_att_l2capAttCmdHandler (connHandle, pkt); //rf_packet_l2cap_t
                if (pr){

//                  blt_ll_pushTxfifoHold (connHandle, pr); //remove ATT hold, send insufficient security
                    l2cap_pkt_t *l2cap_pkt = (l2cap_pkt_t *)&((rf_packet_l2cap_t*)pr)->l2capLen;
                    if(BLE_SUCCESS != blt_l2cap_pushData_2_controller (connHandle, l2cap_pkt->cid, &l2cap_pkt->payload.att.opcode, 1, (u8*)l2cap_pkt->payload.att.data, l2cap_pkt->pduLen-1))
                    {
                        gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);
                        pGap_ms_para->pPendingPkt = l2cap_pkt;
                    }
                }

            }
            else{ //no matter master or slave, all data processed in application layer
                if(opcode == ATT_OP_HANDLE_VALUE_IND) {
                    blc_gatt_pushHandleValueConfirm(connHandle | HANDLE_STK_FLAG);
                }
                if(gatt_data_handler){
                    gatt_data_handler(connHandle, pkt);
                }
            }
        }
        else if(ptrL2cap->chanId == L2CAP_CID_SIG_CHANNEL){     //CID ==> SIG
            tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] L2CAP_CID_SIG_CHANNEL", &(ptrL2cap->l2capLen), ptrL2cap->rf_len);
            my_dump_str_data(RX_L2CAP_DATA_LOG, "RX SIG", (u8*)ptrL2cap, ptrL2cap->rf_len + 2);  //debug
            blt_l2cap_signalDataControl(connHandle, ptrL2cap);
        }
        else if(ptrL2cap->chanId == L2CAP_CID_SMP){     //CID ==> SMP
            tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] L2CAP_CID_SIG_CHANNEL", &(ptrL2cap->l2capLen), ptrL2cap->rf_len);
            my_dump_str_data(RX_L2CAP_DATA_LOG, "RX SMP", (u8*)ptrL2cap, ptrL2cap->rf_len + 2);  //debug

            u8* pr = blt_smp_l2capSmpCmdHandler(connHandle, (u8*)pkt);
            if (pr){
                ll_push_tx_fifo_handler (connHandle | HANDLE_STK_FLAG, pr); //remove ATT hold, send insufficient security
            }
        }
        else{
            #if L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN
            if(coc_data_handler)
                coc_data_handler(connHandle, (l2cap_pkt_t*)&ptrL2cap->l2capLen);    //blt_l2cap_cocDataControl
            #endif
        }
    }

    return 0;
}


#define PUSH_DATA_FUNC_OPTIMIZE                     1  //to save code size, cause this function is used very frequently





#if L2CAP_DATA_2_HCI_DATA_BUFFER_ENABLE
extern int hci_acl_data_buff_rest;

int blc_hci_handler_simulate (u8 *p, int n)
{
    if (p[0] == HCI_TYPE_ACL_DATA){
        blc_hci_receiveHostACLData( p[1] + (p[2]&15) * 256, (p[2]&0x30)>>4, (p[2]&0xc0)>>6, p+3);
    }
    return 1;
}
#endif

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
ble_sts_t  blt_l2cap_pushData_2_controller (u16 connHandle, u16 cid, u8 *format, int format_len, u8 *pDate, int data_len)
{
#if L2CAP_DATA_2_HCI_DATA_BUFFER_ENABLE

    if(hci_acl_data_buff_rest > 0){

        u8 hci_buffer[200];
        hci_buffer[0] = HCI_TYPE_ACL_DATA;
        hci_buffer[1] = U16_LO(connHandle);

        u8 PBFlag = 0;
        u8 BCFlag = 0;
        hci_buffer[2] = (U16_HI(connHandle) & 0x0F) | PBFlag<<4 | BCFlag<<6;

        int len = 4 + format_len + data_len;

        hci_buffer[3] = U16_LO(len);
        hci_buffer[4] = U16_HI(len);

        //l2cap len
        hci_buffer[5] = U16_LO(format_len + data_len);
        hci_buffer[6] = U16_HI(format_len + data_len);

        //CID
        hci_buffer[7] = U16_LO(cid);
        hci_buffer[8] = U16_HI(cid);

        smemcpy((hci_buffer+9), format, format_len);          // format data
        smemcpy((hci_buffer+9+format_len), pDate, data_len);  // data

        blc_hci_handler_simulate(hci_buffer, 1);


        hci_acl_data_buff_rest --;

        return BLE_SUCCESS;
    }
    else{
        return LL_ERR_TX_FIFO_NOT_ENOUGH;
    }

#else

    u8 conn_idx = connHandle & CONN_IDX_MASK;


#if (PUSH_DATA_FUNC_OPTIMIZE)
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];

    if(pc->connState == 0){
        return LL_ERR_CONNECTION_NOT_ESTABLISH;
    }
    else if( pc->ll_enc_busy ){  //for master, it's 0 all time,  we will do it later
        return  LL_ERR_ENCRYPTION_BUSY;
    }
#if SMP_REAL_ENCRYPTION_BUSY_ENABLE
    else if(bltAtt.smpPairingHoldATT && !real_encryption_busy_enable && blc_smp_isPairingBusy(connHandle) ){
        return  SMP_ERR_PAIRING_BUSY;
    }
#else
    else if(bltAtt.smpPairingHoldATT && blc_smp_isPairingBusy(connHandle) ){
        return  SMP_ERR_PAIRING_BUSY;
    }
#endif

#else

    if(blt_llms_getConnState(connHandle) == 0){
        return LL_ERR_CONNECTION_NOT_ESTABLISH;
    }
    else if( blt_ll_isEncryptionBusy(connHandle) ){  //for master, it's 0 all time,  we will do it later
        return  LL_ERR_ENCRYPTION_BUSY;
    }

#endif



    //Note: one question not consider now: if data comes from stack(HANDLE_STK_FLAG valid) and TX fifo not enough,
    //      data will drop without any buffer hold


/////////step 1, calculate TX FIFO number cost, if not enough, return error ////////////
    //u8 mtuSize = blc_att_getEffectiveMtuSize(connHandle);  //TELINK MTU no longer than 256, so 1 byte is enough

    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);
    if((pGap_ms_para==NULL) || (!format_len))
        return GAP_ERR_INVALID_PARAMETER;

    int PDU_max = pGap_ms_para->effective_MTU - format_len;  //e.g. MTU=23, HandValueNotify format_len=3(opcode, attHandle), 20 bytes max                                                //     MTU=50, 47 bytes max
    int extraDateLen = ( 4 + format_len);  //l2cap_len: 2 byte,  channel ID: 2 byte
    int cur_dataLen;


    int conn_MAX_TX_OCTETS = blt_llms_get_connEffectiveMaxTxOctets_by_connIdx(conn_idx);

    int pktNum = 0;
    if(data_len)
    {
        for(int i=0; i<data_len; i+=PDU_max){   // unpack raw data according to MTU  e.g. MTU = 50, data_len=128: 47 + 47 + 34
            cur_dataLen = (data_len - i) > PDU_max ? PDU_max : (data_len - i);

            // unpack MTU data according to connEffectiveMaxTxOctets
            pktNum += (cur_dataLen + format_len + 3)/conn_MAX_TX_OCTETS  + 1;   // (cur_dataLen + 4(l2cap_len+CID) + format_len - 1 )/CONN_MAX_TX_OCTETS + 1
        }
    }
    else
    {
        cur_dataLen = 0;
        pktNum = 1;
    }
    //TODO: Sihui, consider TX FIFO empty packet
    //Note: XXX >= YYY : same as XXX > fifo_num - empty_pkt - stack_used_fifos, TODO can be removed
#if (PUSH_DATA_FUNC_OPTIMIZE)
    if(((pc->tx_wptr - pc->tx_rptr) & 31) + pktNum >= ((connHandle & HANDLE_STK_FLAG) ? pc->max_fifo_num : (pc->max_fifo_num - BLMS_STACK_USED_TX_FIFO_NUM)) ){
        return LL_ERR_TX_FIFO_NOT_ENOUGH;
    }
#else
    u8 max_fifo_num = blt_llms_get_tx_fifo_max_num(connHandle);
    if(blc_ll_getTxFifoNumber(connHandle) + pktNum >= ((connHandle & HANDLE_STK_FLAG) ? max_fifo_num : (max_fifo_num - BLMS_STACK_USED_TX_FIFO_NUM)) ){
        return LL_ERR_TX_FIFO_NOT_ENOUGH;
    }
#endif


/////////////// step 2, push data to TX fifo ////////////
    #if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
        u8 pkt_l2cap_data[256];
    #else
        u8 pkt_l2cap_data[36];
    #endif

#if DBG_L2CAP_BTSNOOP_LOG
    u8 pkt_l2cap_data1[format_len + data_len + 10];
    pkt_l2cap_data1[0] = connHandle;
    pkt_l2cap_data1[1] = connHandle>>8;
    pkt_l2cap_data1[2] = (format_len+data_len+4);
    pkt_l2cap_data1[3] = (format_len+data_len+4)>>8;
    pkt_l2cap_data1[4] = (format_len+data_len);
    pkt_l2cap_data1[5] = (format_len+data_len)>>8;
    pkt_l2cap_data1[6] = (cid);
    pkt_l2cap_data1[7] = (cid)>>8;
    smemcpy(pkt_l2cap_data1+8, format, format_len);
    smemcpy(pkt_l2cap_data1+8+format_len, pDate, data_len);
    BLT_HOST_DBUG(DBG_L2CAP_BTSNOOP_LOG, "[BTSNOOP]tx l2cap data is 0x%s", hex_to_str(pkt_l2cap_data1, format_len+data_len+8));

#endif


    if(data_len)
    {
        int n;
        int max_TX_OCTETS_FIRST_PKT = conn_MAX_TX_OCTETS - extraDateLen;
        for(int i=0; i<data_len; i+=PDU_max){   // unpack raw data according to MTU  e.g. MTU = 50, data_len=128: 47 + 47 + 34

            cur_dataLen = (data_len - i) > PDU_max ? PDU_max : (data_len - i);

            //push first data packet
            n = cur_dataLen < max_TX_OCTETS_FIRST_PKT ? cur_dataLen : max_TX_OCTETS_FIRST_PKT;
            pkt_l2cap_data[0] = LLID_DATA_START;  //first data packet
            pkt_l2cap_data[1] = n + extraDateLen; //rf_len
            *(u16*)(pkt_l2cap_data + 2) = cur_dataLen + format_len; //l2cap_len
            *(u16*)(pkt_l2cap_data + 4) = cid;  //channel ID
            smemcpy((pkt_l2cap_data+6), format, format_len);  // format data
            smemcpy((pkt_l2cap_data+6+format_len), (pDate+i), n); // real data

            ll_push_tx_fifo_handler (connHandle, pkt_l2cap_data);


            //push rest data packet
            for (int j=n; j<cur_dataLen; j+=conn_MAX_TX_OCTETS)
            {
                n = (cur_dataLen - j) > conn_MAX_TX_OCTETS ? conn_MAX_TX_OCTETS : (cur_dataLen - j);
                pkt_l2cap_data[0] = LLID_DATA_CONTINUE; //continue data packet
                pkt_l2cap_data[1] = n;  //rf_len
                smemcpy((pkt_l2cap_data + 2), (pDate+i+j), n); // real data
                ll_push_tx_fifo_handler (connHandle, pkt_l2cap_data);//todo should check state
            }

        }
    }
    else
    {
        pkt_l2cap_data[0] = LLID_DATA_START;  //first data packet
        pkt_l2cap_data[1] = extraDateLen; //rf_len
        *(u16*)(pkt_l2cap_data + 2) = format_len;   //l2cap_len
        *(u16*)(pkt_l2cap_data + 4) = cid;  //channel ID
        smemcpy((pkt_l2cap_data+6), format, format_len);  // format data
        ll_push_tx_fifo_handler (connHandle, pkt_l2cap_data);
    }


    return BLE_SUCCESS;

#endif

}


///////////////////////////////////
// l2cap connection parameter update process
///////////////////////////////////
void  blc_l2cap_SendConnParamUpdateResponse(u16 connHandle, u8 req_id, conn_para_up_rsp result)
{
    u8 conn_update_rsp[16];  //12 + 4(mic)

    rf_packet_l2cap_connParaUpRsp_t *pRsp = (rf_packet_l2cap_connParaUpRsp_t *)conn_update_rsp;
    pRsp->llid = L2CAP_FIRST_PKT_C2H;
    pRsp->rf_len = 10;
    pRsp->l2capLen = 6;
    pRsp->chanId = L2CAP_CID_SIG_CHANNEL;
    pRsp->opcode = L2CAP_CONN_PARAM_UPDATE_RSP;
    pRsp->id = req_id;
    pRsp->data_len = 2;
    pRsp->result = result;

    ll_push_tx_fifo_handler (connHandle | HANDLE_STK_FLAG, conn_update_rsp); ///blt_llms_pushTxfifo
}


u8 bls_l2cap_requestConnParamUpdate (u16 connHandle, u16 min_interval, u16 max_interval, u16 latency, u16 timeout)
{
    if(connHandle & BLS_CONN_HANDLE)
    {
        gap_s_para_t  *pGap_s_para = bls_gap_getSlavePara(connHandle);

        if(pGap_s_para)
        {
            pGap_s_para->l2cap_connParaUpdateReq_minInterval = min_interval;
            pGap_s_para->l2cap_connParaUpdateReq_maxInterval = max_interval;
            pGap_s_para->l2cap_connParaUpdateReq_latency   = latency;
            pGap_s_para->l2cap_connParaUpdateReq_timeout   = timeout;
            pGap_s_para->l2cap_connParaUpReq_pending = 1;

            return 0;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }
}



u8   bls_l2cap_setMinimalUpdateReqSendingTime_after_connCreate(u16 connHandle, int time_ms)
{
    if(connHandle & BLS_CONN_HANDLE){

        gap_s_para_t  *pGap_s_para = bls_gap_getSlavePara(connHandle);

        if(pGap_s_para==NULL){
            return 1;
        }

        pGap_s_para->l2cap_connParaUpReqSendAfterConn_us = time_ms*1000;

        return 0;
    }
    else{
        return 1;
    }

}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
u8 blt_UpdateParameter_request (u16 connHandle)
{
    if(connHandle & BLS_CONN_HANDLE){

        gap_s_para_t  *pGap_s_para = bls_gap_getSlavePara(connHandle);

        if(pGap_s_para==NULL){
            return 1;
        }

        if(!blt_ll_isEncryptionBusy(connHandle) &&\
           ((u32)(clock_time() - blc_ll_getConnectionStartTick(connHandle) ) > (SYSTEM_TIMER_TICK_1US * pGap_s_para->l2cap_connParaUpReqSendAfterConn_us))){

            u8 connParaUpData[18];
            rf_packet_l2cap_connParaUpReq_t *pReq = (rf_packet_l2cap_connParaUpReq_t* )connParaUpData;
            pReq->llid = 2;
            pReq->rf_len = 16;
            pReq->l2capLen = 0x000c; ///
            pReq->chanId = 5;
            pReq->opcode = 0x12;  ///
            pReq->id = 1;
            pReq->data_len = 8;
            pReq->min_interval = pGap_s_para->l2cap_connParaUpdateReq_minInterval;
            pReq->max_interval = pGap_s_para->l2cap_connParaUpdateReq_maxInterval;
            pReq->latency = pGap_s_para->l2cap_connParaUpdateReq_latency;
            pReq->timeout = pGap_s_para->l2cap_connParaUpdateReq_timeout;


            if (ll_push_tx_fifo_handler(connHandle, connParaUpData)) {
                pGap_s_para->l2cap_connParaUpReq_pending =0;
            }
            else{
                return LL_ERR_TX_FIFO_NOT_ENOUGH;
            }
        }
        else{//Conditions are not enough
            return LL_ERR_CURRENT_STATE_NOT_SUPPORTED_THIS_CMD;
        }
        return 0;
    }
    else{
        return 1; ///not slave device
    }
}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
void blt_l2cap_processConnParamUpdateReq(u16 connHandle, host_acl_ms_t *pHostAclms)
{
    //if(pHostAclms->l2cap_connParaUpReq_pending)
    {
        //TODO SiHui: pHostAclms->l2cap_connParaUpdateReq_latency change to 0 now, PM later consider
        u16 conn_latency = 0;
        if(blt_ll_getMaxAclCentralNumber() == 1){
            conn_latency = pHostAclms->l2cap_connParaUpdateReq_latency;
        }

    #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
        conn_latency = min(pHostAclms->l2cap_connParaUpdateReq_latency, acl_mst_connParamUpdateRsp_latency_max);
    #endif

        if(BLE_SUCCESS == blc_ll_updateConnection(connHandle, pHostAclms->l2cap_connParaUpdateReq_minInterval, pHostAclms->l2cap_connParaUpdateReq_maxInterval, \
                conn_latency, pHostAclms->l2cap_connParaUpdateReq_timeout, 0, 0xFFFF)){

            pHostAclms->l2cap_connParaUpReq_pending = 0;
        }
        else if(clock_time_exceed(pHostAclms->l2cap_connParaUpReq_pending, 5*1000000)){ //5 S timeout
            pHostAclms->l2cap_connParaUpReq_pending = 0;
        }
    }
}


void blm_l2cap_processConnParamUpdatePending(u16 connHandle, u16 min_interval, u16 max_interval, u16 latency, u16 timeout)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    host_acl_ms_t *pHostAclms = (host_acl_ms_t *)&blhAclms[conn_idx];
    pHostAclms->l2cap_connParaUpdateReq_minInterval = min_interval;
    pHostAclms->l2cap_connParaUpdateReq_maxInterval = max_interval;
    pHostAclms->l2cap_connParaUpdateReq_latency = latency;
    pHostAclms->l2cap_connParaUpdateReq_timeout = timeout;
    pHostAclms->l2cap_connParaUpReq_pending = clock_time() | 1;
}


void blt_l2cap_para_pre_init(void)
{

    l2cap_buff_m.init_MTU = ATT_MTU_SIZE;
    l2cap_buff_s.init_MTU = ATT_MTU_SIZE;
    l2cap_buff_m.mtuReqSendTimeUs = 200*1000; //dft after 200ms
    l2cap_buff_s.mtuReqSendTimeUs = 200*1000; //dft after 200ms

    for(int i=0; i<LL_MAX_ACL_CONN_NUM;i++){
        gap_ms_para[i].effective_MTU =ATT_MTU_SIZE;
        gap_ms_para[i].mtu_exg_pending = 0;
        gap_ms_para[i].data_pending_time = 30; //10ms unit
    }

    for(int i=0; i<LL_MAX_ACL_PER_NUM;i++)
    {
        gap_s_para[i].l2cap_connParaUpReqSendAfterConn_us = 1000000;
    }

}
