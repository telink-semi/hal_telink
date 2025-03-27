/********************************************************************************************************
 * @file    smp.c
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
#include "stack/ble/ble.h"


#if OS_SUP_EN
#include "stack/ble/os_sup/os_sup.h"
#include "stack/ble/os_sup/os_sup_stack.h"
#endif

/*
 *  smp Pairing own confirm
 * */
_attribute_ble_data_retention_  u8 smpOwnPairingConfirm[16] = {0};

/*
 *  smp   Pairing own IRK
 * */
_attribute_ble_data_retention_  u8 smpPairingOwnIRK[16] = {
        0x97, 0x74, 0x24, 0x67, 0x62, 0x42, 0x81, 0x14, 0x57, 0x20, 0x42, 0x53, 0x32, 0x37, 0x32, 0x74
};

/*
 *  smp   Pairing own CSK
 * */
_attribute_ble_data_retention_  u8 smpPairingOwnCSRK[16] = {
        0x33, 0x21, 0x12, 0x34, 0x29, 0x78, 0x64, 0x54, 0x56, 0x07, 0x82, 0x58, 0x09, 0x79, 0x86, 0x19
};


// [0] is for master,  [1...LL_MAX_ACL_PER_NUM] is for slave
_attribute_ble_data_retention_  _attribute_aligned_(4) smp_prop_t       blt_smpProp[1 + LL_MAX_ACL_PER_NUM];

// [0] is for master, [1...LL_MAX_ACL_PER_NUM] is for slave
_attribute_ble_data_retention_  _attribute_aligned_(4) smp_param_own_t  smp_param_own[1 + LL_MAX_ACL_PER_NUM];
_attribute_ble_data_retention_  _attribute_aligned_(4) smp_param_peer_t smp_param_peer[1 + LL_MAX_ACL_PER_NUM];

_attribute_ble_data_retention_  _attribute_aligned_(4) smp_st_t         smp_sts_param[LL_MAX_ACL_CONN_NUM];  //SMP status parameters

_attribute_ble_data_retention_  smp_init_handler_t  func_smp_init = NULL;
_attribute_ble_data_retention_  smp_delete_handler_t smp_delete_cb = NULL;

_attribute_ble_data_retention_  smp_st_t         *blms_p_sts  = NULL;
_attribute_ble_data_retention_  smp_prop_t       *blms_p_prop = NULL;
_attribute_ble_data_retention_  smp_param_own_t  *blms_p_own  = NULL;
_attribute_ble_data_retention_  smp_param_peer_t *blms_p_peer = NULL;

_attribute_ble_data_retention_  smp_conn_complete_callback_t smp_conn_complete_cb = NULL;


_attribute_ble_data_retention_  _attribute_aligned_(4) secReq_ctl_t blc_SecReq_ctrl  = {
        (SecReq_IMM_SEND<<4) | SecReq_IMM_SEND,  // <7:4> reConn   <3:0> newConn
        0,
        0,
};



_attribute_ble_data_retention_  smp_mng_t         smpMng = {
        .master_max_bondNum            =        SMP_MASTER_BONDING_DEVICE_MAX_NUM,
        .slave_max_bondNum             =        SMP_SLAVE_BONDING_DEVICE_MAX_NUM,
        .loc_irk_str                   =        LOCIRK_BINDING_WITH_DEVICE,
        .dynamic_cfg_smp               =        0,
};


/*
 *  smp responder signal packet.
 * */

_attribute_ble_data_retention_  smp2llcap_type_t smpResSignalPkt = {
        0x02,       // type
        0x15,       //rf len
        0x0011, //l2cap len
        0x0006,  // l2chn id
        0,
        {0},
};


_attribute_ble_data_retention_  smp_sc_cmd_handler_t     func_smp_sc_proc = NULL;
_attribute_ble_data_retention_  smp_sc_pushPkt_handler_t func_smp_sc_pushPkt_proc = NULL;


#if (SMP_LEGACY_LTK_VERIFICATION_EN)
    _attribute_ble_data_retention_ u8 smp_setLtkVerification = 0;
    void    blc_smp_setLtkVerificationEnable(u8 en)
    {
        smp_setLtkVerification = en;
    }
#endif

#if (SMP_REAL_ENCRYPTION_BUSY_ENABLE)
    _attribute_ble_data_retention_ u8 real_encryption_busy_enable = 0;
    void    blc_smp_setRealEncryptionBusyEnable(u8 en)
    {
        real_encryption_busy_enable = en;
    }
#endif


/**************************************************
 *  used for set smp responder packet data and return .
 */
// @@@@@  length can optimize to save ramcode
u8* blt_smp_pushPairingFailed(u8 failReason)
{
    smpResSignalPkt.l2capLen = 0x02;                //l2cap_len
    smpResSignalPkt.data[0] = failReason;           //confirm value failed
    smpResSignalPkt.opcode = SMP_OP_PAIRING_FAIL;
    smpResSignalPkt.rf_len = smpResSignalPkt.l2capLen + 4;
    return (u8*)&smpResSignalPkt;
}


/***********************************************************************************************
 This function is executed in Event CallBack,
 can not use previous blms_p_sts/blms_p_prop/blms_p_own/blms_p_peer can be used here

 ***********************************************************************************************/
int blt_smp_llEncryptionDone(u16 connHandle, u8 status, u8 enc_enable) //register encryption done event in GAP event callBack
{
    u8 is_master = connHandle & BLM_CONN_HANDLE;
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    u8 slave_dev_idx = blt_gap_getSlaveDeviceIndex_by_connIdx(conn_idx);
    smp_st_t *pSmpStsParam  = (smp_st_t *)&smp_sts_param[conn_idx];

    if(status==BLE_SUCCESS && enc_enable){ //encryption done success
        u8 *pCurBondNum = 0;
        if(is_master){  //Master
            pCurBondNum = (u8*)&smpMStblBondDevice.master_cur_bondNum;
        }
        else{  //Slave
            pCurBondNum = (u8*)&smpMStblBondDevice.slave_cur_bondNum[slave_dev_idx];
        }

        int reConnect = SMP_STANDARD_PAIR;

    #if (SMP_DATABASE_INFO_SOURCE == SMP_INFO_STORAGE_IN_FLASH)
        if(smpMStblBondDevice.keyIndex[conn_idx] < *pCurBondNum){ //auto connect when key match
            reConnect = SMP_FAST_CONNECT;

            if(pSmpStsParam->smp_phase_chk != PAIRING_PHASE_IDLE){
                #if (DBG_SMP_ERR_EN)
                    //printf("not possible0(hdl:0x%x), %d\n", connHandle, pSmpStsParam->smp_phase_chk);
                    SMP_ERR_DEBUG(0x66000011);
                #endif
            }
        }
        else{  //First Pair
            if(pSmpStsParam->smp_phase_chk == PAIRING_PHASE_2_ENC){
                pSmpStsParam->smp_phase_chk = PAIRING_PHASE_2_OK;
            }
            else{ //Can be removed after debugging
                #if (DBG_SMP_ERR_EN)
                    //printf("not possible1(hdl:0x%x), %d\n", connHandle, pSmpStsParam->smp_phase_chk);
                    SMP_ERR_DEBUG(0x66000022);
                #endif
            }

            //key distribute trigger condition
            if(is_master){  //Master
                if(!pSmpStsParam->smp_DistributeKeyRecv.keyIni) {
                    smp_sts_param[conn_idx].key_distribute = 1;
                }
            }
            else{  //Slave
                smp_sts_param[conn_idx].key_distribute = 1;
            }

        }
    #endif

        if(reConnect == SMP_FAST_CONNECT){
            blt_smp_setCertTimeoutTick(connHandle, 0);
        }

        if(gap_eventMask & GAP_EVT_MASK_SMP_CONN_ENCRYPTION_DONE){
            u8 param_evt[4];
            gap_smp_connEncDoneEvt_t *pEvt = (gap_smp_connEncDoneEvt_t *)param_evt;
            pEvt->connHandle = connHandle;
            pEvt->re_connect = reConnect;

            blc_gap_send_event( GAP_EVT_SMP_CONN_ENCRYPTION_DONE, param_evt, sizeof(gap_smp_pairingBeginEvt_t) );
        }

        if(  (gap_eventMask & GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE) && reConnect == SMP_FAST_CONNECT){
            u8 param_evt[4];
            gap_smp_securityProcessDoneEvt_t *pEvt = (gap_smp_securityProcessDoneEvt_t *)param_evt;
            pEvt->connHandle = connHandle;
            pEvt->re_connect = SMP_FAST_CONNECT;

            blc_gap_send_event( GAP_EVT_SMP_SECURITY_PROCESS_DONE, param_evt, sizeof(gap_smp_securityProcessDoneEvt_t) );
        }
    }
    else{
        blt_smp_encChangeEvt(status, connHandle, enc_enable);
    }

    return 1;
}

#if(LL_PAUSE_ENC_FIX_EN)
int blt_smp_llEncryptionPause(u16 connHandle) //register encryption pause callback in API: blc_gap_init
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    smp_st_t *pSmpSts  = (smp_st_t *)&smp_sts_param[conn_idx];

    pSmpSts->key_distribute = 0;

    return 0;
}
#endif







/**************************************************************************************************
blt_smp_sendInfo called by
1. blt_smp_proc_pairing_loop              <- Pairing Loop Entry

global blms_p_sts/blms_p_prop/blms_p_own/blms_p_peer can be used here
 **************************************************************************************************/
u8 *blt_smp_sendInfo(u16 connHandle)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    blms_p_sts  = (smp_st_t *)&smp_sts_param[conn_idx];

    //GaoQiu optimize performance
    switch(blms_p_sts->smpDistributeKeyOrder)
    {
    case SMP_TRANSPORT_SPECIFIC_KEY_START:
        blms_p_sts->smpDistributeKeyOrder = SMP_OP_ENC_INFO;//LTK
        if(blms_p_sts->smp_DistributeKeySend.encKey){
            return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_ENC_INFO);
        }
        break;
    case SMP_OP_ENC_INFO://EDIV and Random
        blms_p_sts->smpDistributeKeyOrder = SMP_OP_ENC_IINFO;
        if(blms_p_sts->smp_DistributeKeySend.encKey){
            blms_p_sts->smp_DistributeKeySend.encKey = 0;
            return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_ENC_IDX);
        }
        break;
    case SMP_OP_ENC_IINFO://IRK
        blms_p_sts->smpDistributeKeyOrder = SMP_OP_ENC_IADR;
        if(blms_p_sts->smp_DistributeKeySend.idKey){
            return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_ENC_IINFO);
        }
        break;
    case SMP_OP_ENC_IADR://Id address
        blms_p_sts->smpDistributeKeyOrder = SMP_OP_ENC_SIGN;
        if(blms_p_sts->smp_DistributeKeySend.idKey){
            blms_p_sts->smp_DistributeKeySend.idKey = 0;
            return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_ENC_IADR);
        }
        break;
    case SMP_OP_ENC_SIGN://CSRK
        blms_p_sts->smpDistributeKeyOrder = SMP_OP_ENC_END;
        if(blms_p_sts->smp_DistributeKeySend.sign){
            blms_p_sts->smp_DistributeKeySend.sign = 0;
            return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_ENC_SIGN);
        }
        break;
    case SMP_OP_ENC_END:
        blms_p_sts->smpDistributeKeyOrder = SMP_TRANSPORT_SPECIFIC_KEY_END;
        //if sending key and receiving key all completed, process pairing end
        if(!blms_p_sts->smp_DistributeKeySend.keyIni && !blms_p_sts->smp_DistributeKeyRecv.keyIni) {
            blt_smp_saveBondingKey(connHandle);
        }
        break;
    default:break;
    }
    return NULL;
}








#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
void blt_smp_proc_pairing_loop(u16 connHandle)
{
    int smp_master_role = (connHandle & BLM_CONN_HANDLE);  // smp_master_role no need set now,
    u8 conn_idx = connHandle & CONN_IDX_MASK;

    ///////////////// Pairing Loop Entry ////////////////////
    u8 smp_prop_idx = smp_master_role ? 0: (blt_gap_getSlaveDeviceIndex_by_connIdx(conn_idx) + 1);
    u8 smp_status_idx = smp_master_role ? 0 : (conn_idx - LL_MAX_ACL_CEN_NUM + 1);
#if (SMP_PERIPHERAL_LEVEL_CFG_SEPARATE_EN)
    if(smpMng.dynamic_cfg_smp) //attention: conflict with multiple local device function !!!
    {
        blms_p_prop  = (smp_prop_t *)&blt_smpProp[smp_status_idx];
    }
    else
#endif
    {
        blms_p_prop  = (smp_prop_t *)&blt_smpProp[smp_prop_idx];
    }
    blms_p_own  = (smp_param_own_t *)&smp_param_own[smp_status_idx];
    blms_p_peer = (smp_param_peer_t *)&smp_param_peer[smp_status_idx];
    blms_p_sts  = (smp_st_t *)&smp_sts_param[conn_idx];


    if(blms_p_sts->tk_status & TK_ST_CONFIRM_PENDING){
        if( blms_p_sts->tk_status & TK_ST_UPDATE ){
            blms_p_sts->tk_status = 0;
            u8* pr = blt_smp_pushSmpCmdPkt (connHandle, SMP_OP_PAIRING_CONFIRM);
            ll_push_tx_fifo_handler (connHandle | HANDLE_STK_FLAG, pr);
        }
        else{
            //smp timeout, process in: blc_smp_certTimeoutLoopEvt
        }
    }

    if(blms_p_sts->tk_status & TK_ST_NUMERIC_COMPARE)
    {
        if( blms_p_sts->tk_status & TK_ST_NUMERIC_CHECK_YES){
            if(blms_p_sts->tk_status & TK_ST_NUMERIC_DHKEY_FAIL_PENDING){
                blms_p_sts->tk_status = 0;


                //pairing fail event to tell upper layer
                blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_DHKEY_CHECK_FAIL);  //pairing end with failure

                u8* pr = blt_smp_pushPairingFailed (PAIRING_FAIL_REASON_DHKEY_CHECK_FAIL);
                ll_push_tx_fifo_handler (connHandle | HANDLE_STK_FLAG, pr);
            }
            else if(blms_p_sts->tk_status & TK_ST_NUMERIC_DHKEY_SUCC_PENDING){
                blms_p_sts->tk_status = 0;

                u8* pr = blt_smp_pushSmpCmdPkt (connHandle, SMP_OP_PAIRING_DHKEY);
                ll_push_tx_fifo_handler (connHandle | HANDLE_STK_FLAG, pr);

                if(!smp_master_role){
                    //smp4.2 or above, after exchange smp pairing DH key check, then transport specific keys distribution
                    blms_p_sts->smpDistributeKeyOrder = SMP_TRANSPORT_SPECIFIC_KEY_START;
                }
            }
        }
        else if( blms_p_sts->tk_status & TK_ST_NUMERIC_CHECK_NO){
            blms_p_sts->tk_status = 0;

            //pairing fail event to tell upper layer
            blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_NUMERIC_FAILED);  //pairing end with failure

            //See the Core_v5.0(Vol 3/Part H/3.5.5/Pairing Failed) for more information.
            //NOTICE: test by smart phone, master send unspecified reason when press "NO" button!
            u8* pr = blt_smp_pushPairingFailed (PAIRING_FAIL_REASON_NUMERIC_FAILED); // unsure??
            ll_push_tx_fifo_handler (connHandle | HANDLE_STK_FLAG, pr);
        }
        else{
            //smp timeout, process in: blc_smp_certTimeoutLoopEvt
        }
    }





    /* SMP key distribute(Master and Slave) ------------------------------*/
    if(blms_p_sts->key_distribute){

        if( blt_ll_getRealTxFifoNumber(connHandle) == 0 )
        {
            u8 *pr = (u8 *)blt_smp_sendInfo(connHandle); //send encryption info
            if (pr){
                ll_push_tx_fifo_handler (connHandle | HANDLE_STK_FLAG, pr);
            }
            //Some DistributeKey bit masks may be 0, and the pr return value is NULL, so be aware here.
            if(blms_p_sts->smpDistributeKeyOrder == SMP_TRANSPORT_SPECIFIC_KEY_END){
                blms_p_sts->key_distribute = 0;
            }
        }
    }
}






/***********************************************************************************************
 This function is executed in Event CallBack,
 can not use previous blms_p_sts/blms_p_prop/blms_p_own/blms_p_peer can be used here

 ***********************************************************************************************/

int blt_smp_setAddress(u16 connHandle, u8* p)
{
    int is_master = (connHandle & BLM_CONN_HANDLE);
    u8 conn_idx  = connHandle & CONN_IDX_MASK;
    u8 smp_status_idx = is_master ? 0: (conn_idx - LL_MAX_ACL_CEN_NUM + 1);

    u8 slave_dev_idx = blt_gap_getSlaveDeviceIndex_by_connIdx(conn_idx);

#if (SMP_SEC_LEVEL_CHECK_EN)
    u8 smp_prop_idx = is_master ? 0: (slave_dev_idx + 1);
    blms_p_prop  = (smp_prop_t *)&blt_smpProp[smp_prop_idx];
#endif

    u8 prop_idx;
    #if (SMP_PERIPHERAL_LEVEL_CFG_SEPARATE_EN)
        if(smpMng.dynamic_cfg_smp){
            if(!is_master)
            {
                if(smp_conn_complete_cb){
                    smp_conn_complete_cb(connHandle);
                }
            }
            prop_idx = smp_status_idx;
        }
        else
    #endif
        {
            prop_idx = is_master ? 0: (slave_dev_idx + 1);
        }

    if((blt_smpProp[prop_idx].security_level == No_Authentication_No_Encryption)){
        return 0;
    }

    smp_param_own_t  *pSmpOwn  = (smp_param_own_t  *)&smp_param_own[smp_status_idx];
    smp_param_peer_t *pSmpPeer = (smp_param_peer_t *)&smp_param_peer[smp_status_idx];

    smp_st_t  *pSmpSts  = (smp_st_t  *)&smp_sts_param[conn_idx];

    if(is_master){
        pSmpPeer->peer_addr_type = (p[0] & BIT(7)) ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
        smemcpy (pSmpPeer->peer_conn_addr, p + 8, 6);                           //slave address

        pSmpOwn->own_addr_type = (p[0] & BIT(6)) ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
        smemcpy (pSmpOwn->own_conn_addr, p + 2, 6);                     //initiate address

    }
    else{
        pSmpPeer->peer_addr_type = p[0] & BIT(6) ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
        smemcpy (pSmpPeer->peer_conn_addr, p + 2, 6);                           //initiate address

        pSmpOwn->own_addr_type = p[0] & BIT(7) ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
        smemcpy (pSmpOwn->own_conn_addr, p + 8, 6);                     //slave address
    }


#if (SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY)
    st_ll_conn_t* pAclConn = (st_ll_conn_t*)&blms[conn_idx];

    #if (LL_FEATURE_ENABLE_LOCAL_RPA)
        if(pAclConn->conn_locUseRpa)
        {
            pSmpOwn->cur_rpa = 1;
            smemcpy(pSmpOwn->own_irk, pAclConn->pRslvlst_conn->localIRK, 16);
        }
        else
    #endif
        {
            pSmpOwn->cur_rpa = 0;
            /* here we do not calculate local_irk, because AES encryption may cost some time,
             * consider that this function is called when connect, timing is urgent
             * we can calculate when send IRK with "cur rpa" & "blc_smpMng.loc irk_str"
             */
        }

        pSmpOwn->idenAdr_type = bltMac.idenAdr_cur_type;
        smemcpy(pSmpOwn->idenAdr_addr, bltMac.idenAdr_cur_addr, BLE_ADDR_LEN);
#endif


#if (SMP_DATABASE_INFO_SOURCE == SMP_INFO_STORAGE_IN_FLASH)
#if (CUSTOM_SMP_STORAGE)
    if(smp_info_custom_load_cb)
    {
        smp_info_custom_load_cb(connHandle);
    }
#endif
    u32 flash_addr = blt_smp_searchBondingDevice_by_PeerMacAddr(is_master, slave_dev_idx, pSmpPeer->peer_addr_type, pSmpPeer->peer_conn_addr);
    if(flash_addr){
        smpMStblBondDevice.addrIndex[conn_idx] = blt_smp_getBondingIndex_by_FlashAddr(is_master, slave_dev_idx, flash_addr);

        u8 bondFlg = blt_smp_getBondingFlag_by_FlashAddr(flash_addr);
        smpMStblBondDevice.pairing_status[conn_idx] = (bondFlg & FLAG_SMP_PAIRING_STATUS_MASK)>>4;  // BIT<5:4> of bondFlag
    }
    else{
        smpMStblBondDevice.addrIndex[conn_idx] = ADDR_NOT_BONDED; //0->not bonding
        smpMStblBondDevice.pairing_status[conn_idx] = No_Bonded_No_LTK;
    }
#endif


    my_dump_str_data(SMP_DBG_EN, "SMP bond", &flash_addr, 4);

    smpMStblBondDevice.keyIndex[conn_idx] = KEY_FLAG_IDLE;   //if keyIndex used, must clear it to "0xFF" here

    pSmpSts->save_key_flag = 0;

#if (SMP_SEC_LEVEL_CHECK_EN)
    /* 0: the current link does not support SMP pairing; 1: the current link support SMP pairing */
    /* Immediately after the connection is established, the smp security level configuration value
     * may be modified. To adapt to different master links corresponding to different security levels. */
    pSmpSts->support_smp = blms_p_prop->security_level != No_Authentication_No_Encryption;
#endif

    /*
     * This parameter is set only if the 1st pairing(set to 1 if both M and S support SC).
     * It should be 0 when it's connected back.
     */
    pSmpOwn->sc_pairing = 0;

    //smp phase check clear
    pSmpSts->smp_phase_chk = PAIRING_PHASE_IDLE;

    /*
     * if use SC_passkey entry, global variable is used to count 20 repetitions over SMP.
     */
    pSmpOwn->sc_passkey_cnt = 0;

    //careful
    memset(pSmpOwn->pairing_tk, 0, 16);


    if(is_master){  //Master
        /**********************************************************************************************************
        process pairing request
        **********************************************************************************************************/
        // merge form old single master "tbl_bond_slave_search"
        blt_smpTrig.manual_smp_start = 0;
        blt_smpTrig.smp_begin_flg = 0;
        blt_smpTrig.smp_start_pending = BIT(conn_idx); //The previous smp pairing must end before the next pairing start.

        /* It does not matter whether the security level is modified when connecting back, the security level
         * of the bound device is still valid. There is no need to verify the security level here. */
        if(smpMStblBondDevice.addrIndex[conn_idx] != ADDR_NOT_BONDED){ //bonded device re-connect
            my_dump_str_data(SMP_DBG_EN, "Mst SMP re-connect", 0, 0);
            smpMStblBondDevice.master_curIndex = smpMStblBondDevice.addrIndex[conn_idx];
            smpMStblBondDevice.isBond_fastSmp = FlAG_BOND;
            #if (SMP_SEC_LEVEL_CHECK_EN)
                pSmpSts->support_smp = 1; /* bonded device re-connect : support smp */
            #endif
            if(blt_smpTrig.trigger_mask & MASTER_TRIGGER_SMP_AUTO_CONNECT){
                blt_smpTrig.manual_smp_start = 1;
                my_dump_str_data(SMP_DBG_EN, "  Mst Set SMP send auto enc flg", 0, 0);
            }
        }
        else{
            #if (SMP_SEC_LEVEL_CHECK_EN)
                /* As long as the security level > mode1_level1 is configured before connection, the SMP process is allowed. */
                if(pSmpSts->support_smp) // same as: blms_p_prop->security_level != No_Authentication_No_Encryption
            #endif
            { 
                my_dump_str_data(SMP_DBG_EN, "Mst SMP 1st pair", 0, 0);
                if(blt_smpTrig.trigger_mask & MASTER_TRIGGER_SMP_FIRST_PAIRING){
                    blt_smpTrig.manual_smp_start = 1;
                    my_dump_str_data(SMP_DBG_EN, "  Mst Set SMP send start pairing flg", 0, 0);
                }
            }

            smpMStblBondDevice.isBond_fastSmp = 0;
        }
    }
    else{  //Slave
        /**********************************************************************************************************
        process security request sending according to: if device bonded previously  & SecReq sending strategy configured by user
        **********************************************************************************************************/
            u8 secReqCfg = 0;  //new device connect

            /* It does not matter whether the security level is modified when connecting back, the security level
             * of the bound device is still valid. There is no need to verify the security level here. */
            if(smpMStblBondDevice.addrIndex[conn_idx] != ADDR_NOT_BONDED){  //bonded device re-connect
                secReqCfg = blc_SecReq_ctrl.secReq_conn & 0xF0;   //see <7:4>
                my_dump_str_data(SMP_DBG_EN, "Slv SMP reconnect", 0, 0);
            }
            else{   //new device connect
                #if (SMP_SEC_LEVEL_CHECK_EN)
                    /* As long as the security level > mode1_level1 is configured before connection, the SMP process is allowed. */
                    if(blms_p_prop->security_level != No_Authentication_No_Encryption)
                #endif
                {
                    my_dump_str_data(SMP_DBG_EN, "Slv SMP 1st pair", 0, 0);
                    secReqCfg = blc_SecReq_ctrl.secReq_conn & 0x0F;   //see <3:0>
                }
            }

            if( secReqCfg & (SecReq_IMM_SEND<<4 | SecReq_IMM_SEND) ){
                blc_SecReq_ctrl.secReq_pending &= ~BIT(conn_idx); //clear security request pending flag
                blt_smp_sendSecurityRequest(connHandle);
                #if OS_SUP_EN
                if(flash_addr && blt_isOsSupEnable()){   //Because this setting times out, mainloop keeps running. This will affect other tasks.
                //In the backlink case, the central does not need to reply to the peripheral. If the timeout is set, an PAIRING_FAIL_REASON_PAIRING_TIMEOUT event can be reported.
                    blt_smp_setCertTimeoutTick(connHandle, 0);
                }
                #endif
                my_dump_str_data(SMP_DBG_EN, "  Slv send SecReq immediately after connection established", 0, 0);
            }
            else if( secReqCfg & (SecReq_PEND_SEND<<4 | SecReq_PEND_SEND)){
                blc_SecReq_ctrl.secReq_pending |= BIT(conn_idx);
                my_dump_str_data(SMP_DBG_EN, "  SLv Set SecReq pending flg", 0, 0);
            }
        /*********************************************************************************************************/
    }


    return 0;
}







/***********************************************************************************************
 Only slave can trigger this function

 This function  is executed in Event CallBack,
 can not use previous blms_p_sts/blms_p_prop/blms_p_own/blms_p_peer can be used here

 ***********************************************************************************************/
int bls_smp_llGetLtkReq(u16 connHandle, u8* random, u16 ediv) //register get LTK function in controller
{

    if(connHandle & BLM_CONN_HANDLE){  //more safer
        return 0;
    }


    u8 conn_idx = connHandle & CONN_IDX_MASK;
    u8 slave_dev_idx = blt_gap_getSlaveDeviceIndex_by_connIdx(conn_idx);

    u8 smp_status_idx = (conn_idx - LL_MAX_ACL_CEN_NUM + 1);  //no need concern master
    smp_param_own_t  *pSmpOwn  = (smp_param_own_t  *)&smp_param_own[smp_status_idx];
    smp_param_peer_t *pSmpPeer = (smp_param_peer_t *)&smp_param_peer[smp_status_idx];
    smp_st_t *pSmpSts  = (smp_st_t *)&smp_sts_param[conn_idx];



    //not send security request after LL_ENC_REQ received
    blc_SecReq_ctrl.secReq_pending &= ~BIT(conn_idx);  //clear security request pending flag
    my_dump_str_data(SMP_DBG_EN, "  SLv Clr SecReq pending flg", 0, 0);


    u8 randParam[10] = {0};
    u8 const_u8_10_zero[10] = {0};
    smemcpy (randParam, random, 8);
    randParam[8] = ediv;
    randParam[9] = ediv >> 8;


    if(!memcmp(const_u8_10_zero, randParam, 10)) // EDIV+ RAND are all zero
    {
        u8 smp_Stk[16] = {0};
        u8 max_key_size = pSmpOwn->encrypt_key_size;

        /*
         * This parameter is set only if the 1st pairing(set to 1 if both M and S support SC).
         * It should be 0 when it's connected back.
         */
        if(pSmpOwn->sc_pairing)
        {
            /*
             * Secure Connection Pairing:
             * ......
             * (omit)
             * M->S Pairing DHKey Check: none
             * S->M Pairing DHKey Check: Pairing DHKey Check marked
             * M->S LL_ENC_REQ: Encryption begin
             */
            if(pSmpSts->smp_phase_chk & BIT(SMP_OP_PAIRING_DHKEY)){
                pSmpSts->smp_phase_chk = PAIRING_PHASE_2_ENC;

                //////////////////// secure connections 1st //////////////////
                //printf("secure connections 1st\n");
                smemcpy(smp_Stk, pSmpOwn->own_ltk, max_key_size);
                blt_hci_ltkRequestReply(connHandle, smp_Stk);
            }
            else{ //LTK get error
                pSmpSts->smp_phase_chk = PAIRING_PHASE_IDLE;
                blt_hci_ltkRequestNegativeReply(connHandle);
                //printf("LTK get error1\n");
            }
        }
        else{
            /*
             * there are two cases:
             * case1: legacy pairing 1st ( smp_phase_record & SMP_OP_PAIRING_RANDOM must be True)
             * case2: secure connections back
             */
            if(pSmpSts->smp_phase_chk & BIT(SMP_OP_PAIRING_RANDOM)) //legacy pairing 1st
            {
                pSmpSts->smp_phase_chk = PAIRING_PHASE_2_ENC;

                //////////////////// legacy pairing 1st ////////////////////
                //printf("legacy pairing 1st\n");

                u8 smp_Stk_temp[16] = {0};
                //aes_encryption_le in blt_crypto_alg_c1/blt_crypto_alg_s1 need critical data 4B aligned
                //STK = s1(TK, Srand, Mrand)
                blt_crypto_alg_s1(smp_Stk_temp, pSmpOwn->pairing_tk, pSmpPeer->peer_pairing_rand, pSmpOwn->own_rand);//generate session key
                smemcpy(smp_Stk, smp_Stk_temp, max_key_size);

                blt_hci_ltkRequestReply(connHandle, smp_Stk);

                smpMStblBondDevice.keyIndex[conn_idx] = KEY_FLAG_NEW; //mark that STK generated in pairing procedure, not previous existed keys

                //The addr can match, but the master uses a new key. It is possible that: 1. The previous key is stored incorrectly;
                //2. The master side unpair, and the slave side does not know it, so the old bonding information should be deleted here
                if(smpMStblBondDevice.addrIndex[conn_idx] < smpMStblBondDevice.slave_cur_bondNum[slave_dev_idx]){
                    blt_smp_deleteBondingInfo_by_Index(0, slave_dev_idx, smpMStblBondDevice.addrIndex[conn_idx], TRUE);
                    smpMStblBondDevice.addrIndex[conn_idx] = ADDR_NEW_BONDED; //update
                }
            }
            else if(pSmpSts->smp_phase_chk == PAIRING_PHASE_IDLE)
            {
                //////////////////// secure connections back //////////////////////
                //printf("secure connections back\n");
                #if (CUSTOM_DARWIN_FMN_ENABLE)
                    //FMN get its own LTK
                    if (custom_darwin_fmn.sec_info_req_cb) {
                        custom_darwin_fmn.sec_info_req_cb(connHandle);
                    } else
                #endif
                    {
                        #if (SMP_DATABASE_INFO_SOURCE == SMP_INFO_STORAGE_IN_FLASH)
                #if(BROADCOM_WORKAROUND)
                            //u32 flash_addr = blt_smp_searchBondingDevice_by_PeerMacAddr(0, slave_dev_idx, pSmpPeer->peer_addr_type, pSmpPeer->peer_conn_addr);
                            //if(flash_addr)
                            if(1){
                                u8 load_ltk[16] = {0};
//                              flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, local_peer_ltk),     16, load_ltk);
//                              #if(CS_IOP_EN)
//                                      my_dump_str_data(1, "LTK",load_ltk, 16);
//                              #endif
                                blt_hci_ltkRequestReply(connHandle, load_ltk);

//                              smpMStblBondDevice.keyIndex[conn_idx] = blt_smp_getBondingIndex_by_FlashAddr(0, slave_dev_idx, flash_addr);
                            }
                            else{ // LTK get error

                                //printf("LTK get error2\n");
                                blt_hci_ltkRequestNegativeReply(connHandle);
                                smpMStblBondDevice.keyIndex[conn_idx] = KEY_FLAG_FAIL;

                                //The addr can match, but the matching key cannot be found (key missing). There is a problem with the key on
                                //the slave side. You need to delete the previous bonding information directly, otherwise the new pairing will
                                //cause the slave side to store multiple sets of pairing information for devices with the same mac address ,
                                //Causing the wrong information to be read later.
                                if(smpMStblBondDevice.addrIndex[conn_idx] < smpMStblBondDevice.slave_cur_bondNum[slave_dev_idx]){
                                    blt_smp_deleteBondingInfo_by_Index(0, slave_dev_idx, smpMStblBondDevice.addrIndex[conn_idx], true);
                                    smpMStblBondDevice.addrIndex[conn_idx] = ADDR_DELETE_BOND; //update
                                }
                            }
                        #else
                            u32 flash_addr = blt_smp_searchBondingDevice_by_PeerMacAddr(0, slave_dev_idx, pSmpPeer->peer_addr_type, pSmpPeer->peer_conn_addr);
                            if(flash_addr){
                                u8 load_ltk[16] = {0};
                                flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, local_peer_ltk),     16, load_ltk);

                                tlkapi_send_string_data(CS_IOP_EN || (stkLog_mask & STK_LOG_SMP_LTK), "[SMP][LTK] ", load_ltk, 16);
                                blt_hci_ltkRequestReply(connHandle, load_ltk);

                                smpMStblBondDevice.keyIndex[conn_idx] = blt_smp_getBondingIndex_by_FlashAddr(0, slave_dev_idx, flash_addr);
                            }
                            else{ // LTK get error
                                //printf("LTK get error2\n");
                                blt_hci_ltkRequestNegativeReply(connHandle);
                                smpMStblBondDevice.keyIndex[conn_idx] = KEY_FLAG_FAIL;

                                //The addr can match, but the matching key cannot be found (key missing). There is a problem with the key on
                                //the slave side. You need to delete the previous bonding information directly, otherwise the new pairing will
                                //cause the slave side to store multiple sets of pairing information for devices with the same mac address ,
                                //Causing the wrong information to be read later.
                                if(smpMStblBondDevice.addrIndex[conn_idx] < smpMStblBondDevice.slave_cur_bondNum[slave_dev_idx]){
                                    blt_smp_deleteBondingInfo_by_Index(0, slave_dev_idx, smpMStblBondDevice.addrIndex[conn_idx], true);
                                    smpMStblBondDevice.addrIndex[conn_idx] = ADDR_DELETE_BOND; //update
                                }
                            }
                    #endif
                        #else
                            // find keys in your storage media
                        #endif
                    }
            }
            else{ //LTK get error
                //printf("LTK get error3(hdl:0x%x):0x%x\n", connHandle, pSmpSts->smp_phase_chk);
                pSmpSts->smp_phase_chk = PAIRING_PHASE_IDLE;
                blt_hci_ltkRequestNegativeReply(connHandle);
            }
        }
    }
    else{
        //////////////////// legacy pairing back ////////////////////
        //printf("legacy pairing back\n");
        u8 load_ltk[16] = {0};
        u32 flash_addr = bls_smp_loadLTK_by_EdivRand(slave_dev_idx, ediv, random, load_ltk);

        #if (SMP_LEGACY_LTK_VERIFICATION_EN)
            if(smp_setLtkVerification){
                u16 crc16_ = blt_Crc16ComputeInternal(load_ltk, 14);
                u16 ltkverf = load_ltk[14] | (load_ltk[15]<<8);
                if(crc16_ != ltkverf){
                    flash_addr = 0;
                    my_dump_str_data(SMP_DBG_EN, "[ltk] crc16_", &crc16_, 2);
                    my_dump_str_data(SMP_DBG_EN, "[ltk] ltkverf", &ltkverf, 2);
                }
            }
        #endif

        if(flash_addr){
            blt_hci_ltkRequestReply(connHandle, load_ltk);
            smpMStblBondDevice.keyIndex[conn_idx] = blt_smp_getBondingIndex_by_FlashAddr(0, slave_dev_idx, flash_addr);
        }
        else{ // LTK get error

            #if (LL_PAUSE_ENC_FIX_EN)

                bool needSpecialProc = FALSE;

                if(pSmpSts->smp_phase_chk == PAIRING_PHASE_2_OK){
                    needSpecialProc = TRUE;
                }

                //Notice: if it is a slave role, the value of ediv_rand is saved
                //using its own parameters, otherwise, the value of the peer device is used.
                if(needSpecialProc && (pSmpPeer->peer_ediv == ediv && !memcmp(pSmpPeer->peer_random, random, 8))){
                    blt_hci_ltkRequestReply(connHandle,  pSmpOwn->own_ltk);
                    //keyIndex: it seems useless here, can be removed, tyf add 23-11-17
                    smpMStblBondDevice.keyIndex[conn_idx] = blt_smp_getBondingIndex_by_FlashAddr(0, slave_dev_idx, flash_addr);
                }
                else
            #endif
                {
                    #if SAMSUNG_WORKAROUND
                        memset(load_ltk, 0, 16);
                        blt_hci_ltkRequestReply(connHandle, load_ltk);
                    #else
                        //printf("LTK get error4\n");
                        blt_hci_ltkRequestNegativeReply(connHandle);
                        smpMStblBondDevice.keyIndex[conn_idx] = KEY_FLAG_FAIL;  // mark that key not found in a re_connect process

                        //The addr can match, but the matching key cannot be found (key missing). There is a problem with the key on
                        //the slave side. You need to directly delete the previous bonding information, otherwise the new pairing will
                        //cause the slave side to store multiple sets of pairing information for devices with the same mac address ,
                        //Causing the wrong information to be read later.
                        if(smpMStblBondDevice.addrIndex[conn_idx] < smpMStblBondDevice.slave_cur_bondNum[slave_dev_idx]){
                            blt_smp_deleteBondingInfo_by_Index(0, slave_dev_idx, smpMStblBondDevice.addrIndex[conn_idx], true);
                            smpMStblBondDevice.addrIndex[conn_idx] = ADDR_DELETE_BOND; //update
                        }
                    #endif
                }
        }
    }

    return 1;
}









/**************************************************
 *  used for set smp responder packet data and return .
 */
/**************************************************************************************************
blt_smp_pushSmpCmdPkt called by
1. blt_smp_proc_pairing_loop              <- Pairing Loop Entry
2. blt_smp_sendInfo               <- Pairing Loop Entry

3. blt_smp_l2capSmpCmdHandler      <- SMP Data Entry

global blms_p_sts/blms_p_prop/blms_p_own/blms_p_peer can be used here
 **************************************************************************************************/
u8 *blt_smp_pushSmpCmdPkt (u16 connHandle, u8 type)
{
//  u8 conn_idx = connHandle & CONN_IDX_MASK;
    int smp_master_role = (connHandle & BLM_CONN_HANDLE);

    //The SMP Timer shall be reset when an L2CAP SMP command is queued for transmission.
    switch(type){
        case SMP_OP_PAIRING_REQ:
        {
            blms_p_sts->smp_timeout_start_tick = clock_time()|1;//reset timeout timer

            if(smp_master_role){
                smpResSignalPkt.l2capLen = 0x07;      //l2cap len
                smemcpy (&smpResSignalPkt.opcode, (u8*)&blms_p_own->pairing_req, 7);
            }
        }
        break;
        case SMP_OP_PAIRING_RSP:
        {
            blms_p_sts->smp_timeout_start_tick = clock_time()|1;

            if(!smp_master_role){
                smpResSignalPkt.l2capLen = 0x07;      //l2cap len
                smemcpy(&smpResSignalPkt.opcode, (u8*)&blms_p_own->pairing_rsp, 7);
            }
        }
        break;
        case SMP_OP_PAIRING_CONFIRM:
        {
            blms_p_sts->smp_timeout_start_tick = clock_time()|1;

            if(blms_p_own->sc_pairing) //secure connection enable
            {
                if(func_smp_sc_pushPkt_proc){
                    func_smp_sc_pushPkt_proc(connHandle, type); //blt_smp_sc_pushPkt_handler
                }
            }
            else{
                u8* ia = blms_p_peer->peer_conn_addr;
                u8 iat = blms_p_peer->peer_addr_type;
                u8* ra = blms_p_own->own_conn_addr;
                u8 rat = blms_p_own->own_addr_type;

                if(smp_master_role){
                    ia = blms_p_own->own_conn_addr;
                    iat = blms_p_own->own_addr_type;
                    ra = blms_p_peer->peer_conn_addr;
                    rat = blms_p_peer->peer_addr_type;
                }

                /* Calculate Sconfirm
                 *    Sconfirm = c1(TK, Srand,Pairing Request command, Pairing Response command,
                 *                  initiating device address type, initiating device address,
                 *                  responding device address type, responding device address)
                 **/
                //aes_encryption_le in blt_crypto_alg_c1/blt_crypto_alg_s1 need critical data 4B aligned
                blt_crypto_alg_c1(smpOwnPairingConfirm, blms_p_own->pairing_tk, blms_p_own->own_rand,
                              (u8*)&blms_p_own->pairing_rsp, (u8*)&blms_p_own->pairing_req,
                              iat, ia, rat, ra);
            }

            smemcpy(smpResSignalPkt.data, smpOwnPairingConfirm, 16);
            smpResSignalPkt.l2capLen = 0x11;
        }
        break;
        case SMP_OP_PAIRING_RANDOM:
        {
            blms_p_sts->smp_timeout_start_tick = clock_time()|1;

            //secure connection enable
            if(blms_p_own->sc_pairing)
            {
                if(func_smp_sc_pushPkt_proc){
                    func_smp_sc_pushPkt_proc(connHandle, type);
                }
            }
            else{
                smemcpy(smpResSignalPkt.data, blms_p_own->own_rand, 16);
            }

            smpResSignalPkt.l2capLen = 0x11;
        }
        break;
        case SMP_OP_ENC_INFO:  //distribute LTK
        {
            blms_p_sts->smp_timeout_start_tick = clock_time()|1;

            smpResSignalPkt.l2capLen = 0x11;
            smemcpy(smpResSignalPkt.data, blms_p_own->own_ltk, 16);
        }
        break;
        case SMP_OP_ENC_IDX:     //distribute EDIV and Random
        {
            blms_p_sts->smp_timeout_start_tick = clock_time()|1;

            //generate random value for EDIV,RAND
            u8 ediv_rand[10] = {0};

            /*
             * Standard method of generating EDIV: (currently the SDK uses random generation method)
             * dm(k, r) = e(k, r) mod 2^16,  Y = dm(DHK, Rand),  EDIV = Y xor DIV
             * EDIV = ( e(DHK, Rand || padding) mod 2^16)  xor 0xFFFF
             */
            generateRandomNum(10, ediv_rand); //EDIV(2Byte) + Rand(8Byte)

            smpResSignalPkt.l2capLen = 0x0b;
            smemcpy(smpResSignalPkt.data , ediv_rand, 10);

            //Notice: if it is a slave role, the value of ediv_rand is saved
            //using its own parameters, otherwise, the value of the peer device is used.
            if(!smp_master_role){
                blms_p_peer->peer_ediv = MAKE_U16(ediv_rand[1], ediv_rand[0]);//EDIV
                smemcpy(blms_p_peer->peer_random, ediv_rand + 2, 8);//RAND
            }
            else{
                //If the current connection is the master role, do not store its own EDIV
                //and RAND, only store the EDIV and RAND distributed by the slave device.
            }
        }
        break;
        case SMP_OP_ENC_IINFO:    //distribute IRK
        {
            blms_p_sts->smp_timeout_start_tick = clock_time()|1;

            smpResSignalPkt.l2capLen = 0x11;

            #if (SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY)
                if(blms_p_own->cur_rpa){
                    //local irk has already copied from resolving list in "blt_smp_setAddress"
                }
                else{
                    if(smpMng.loc_irk_str == LOCIRK_RANDOM_GENERATE){
                        generateRandomNum(16, blms_p_own->own_irk);
                    }
                    else{
                        u8 data[16]={0};
                        smemcpy(data, blms_p_own->idenAdr_addr, 6);
                        aes_encryption_le(smpPairingOwnIRK, data, blms_p_own->own_irk);
                    }
                }

                smemcpy(smpResSignalPkt.data, blms_p_own->own_irk, 16);
            #else
                /*
                 * TODO: There is a doubt here. The possible explanation is that all Telink's BLE SDKs use the same IRK, but for a device,
                 * its life cycle IRK can be fixed. Refer to the mobile phone method, just make sure that each different device has
                 * a different IRK.
                 */
                #if 1 //generate own irk by MAC address
                    u8 macPadding[16] = { 0 }; //must initialize
                    u8 own_addr_type = blt_ll_getOwnAddrType(connHandle);
                    u8 *pConnAddr  = blt_ll_getOwnMacAddr(connHandle, own_addr_type);
                    smemcpy(macPadding, pConnAddr, BLE_ADDR_LEN);
                    my_dump_str_data(SMP_DBG_EN, "macPadding", &macPadding, 16);
                    my_dump_str_data(SMP_DBG_EN, "key", smpPairingOwnIRK, 16);

                    aes_encryption_le(smpPairingOwnIRK, macPadding, blms_p_own->own_irk);

                    smemcpy(smpResSignalPkt.data, (u8 *)&(blms_p_own->own_irk[0]), 16);
                    my_dump_str_data(SMP_DBG_EN, "gen irk", blms_p_own->own_irk, 16);
                #else //walk around win10 bug
                    generateRandomNum(16, (u8*)&blms_p_own->own_irk[0]);
                    smemcpy(smpResSignalPkt.data, (u8 *)&(blms_p_own->own_irk[0]), 16);
                    #if 0//Old code
                        smemcpy(smpResSignalPkt.data, (u8 *)(smpPairingOwnIRK), 16);
                        smemcpy(blms_p_own->own_irk,(u8 *)(smpPairingOwnIRK), 16);
                    #endif
                #endif
            #endif
        }
        break;
        case SMP_OP_ENC_IADR:    //distribute bd_addr
        {
            blms_p_sts->smp_timeout_start_tick = clock_time()|1;

            smpResSignalPkt.l2capLen = 0x08;

            #if (SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY)
                smpResSignalPkt.data[0] = blms_p_own->idenAdr_type;
                smemcpy(smpResSignalPkt.data + 1, blms_p_own->idenAdr_addr , 6);
            #else
                /*
                 * core5.0 Vol6, PartB, page 2688
                 * If a device is using Resolvable Private Addresses Section 1.3.2.2, it shall also
                 * have an Identity Address that is either a Public or Random Static address type.
                 **/
                #if 0 // here, we use the conn_req's (slave) BDR maybe a mistake, if we(slave) use a RPA as its BDR.
                    smpResSignalPkt.data[0] = blms_p_own->own_addr_type;//address type
                    smemcpy(smpResSignalPkt.data + 1, blms_p_own->own_conn_addr , 6);
                #else // here we use Public BDR as its Identity Address
                    u8 own_addr_type = blt_ll_getOwnAddrType(connHandle);
                    u8 *pConnAddr  = blt_ll_getOwnMacAddr(connHandle, own_addr_type);
                    smpResSignalPkt.data[0] = own_addr_type;
                    smemcpy(smpResSignalPkt.data + 1, pConnAddr, 6);
                #endif
            #endif
        }
        break;
        case SMP_OP_ENC_SIGN: //distribute CSRK
        {
            blms_p_sts->smp_timeout_start_tick = clock_time()|1;

            //generate own CSRK
            for(int i=0; i<8; i++){
                blms_p_own->own_csrk[i] = ~blms_p_own->own_ltk[i];
            }

            smpResSignalPkt.l2capLen = 0x11;
            smemcpy(smpResSignalPkt.data, (u8*)&(blms_p_own->own_csrk[0]), 16);
        }
        break;
        case SMP_OP_PAIRING_FAIL:
        {
            smpResSignalPkt.l2capLen = 0x02;
            smpResSignalPkt.data[0] = 0x04; //confirm value failed
        }
        break;
        case SMP_OP_SEC_REQ:
        {
            if(!smp_master_role){
                smpResSignalPkt.l2capLen = 0x02;
                *smpResSignalPkt.data = blms_p_own->auth_req.authType;      //AuthReq
            }
        }
        break;

        case SMP_OP_PAIRING_PUBLIC_KEY:
        {
            blms_p_sts->smp_timeout_start_tick = clock_time()|1;

            if(func_smp_sc_pushPkt_proc){
                func_smp_sc_pushPkt_proc(connHandle, type);
            }

            return NULL; //must add, because PubKey has been pushed into the TX FIFO.
        }
        break;

        case SMP_OP_PAIRING_DHKEY:
        case SMP_OP_KEYPRESS_NOTIFICATION:
        {
            blms_p_sts->smp_timeout_start_tick = clock_time()|1;

            if(func_smp_sc_pushPkt_proc){
                func_smp_sc_pushPkt_proc(connHandle, type);
            }
        }
        break;


        default:
            break;
    }
    smpResSignalPkt.opcode = type;
    smpResSignalPkt.rf_len = smpResSignalPkt.l2capLen + 4;
    smpResSignalPkt.chanId = 0x0006;

#if DBG_L2CAP_BTSNOOP_LOG
    u8 pkt_l2cap_data1[smpResSignalPkt.rf_len + 10];
    pkt_l2cap_data1[0] = connHandle;
    pkt_l2cap_data1[1] = connHandle>>8;
    pkt_l2cap_data1[2] = (smpResSignalPkt.rf_len);
    pkt_l2cap_data1[3] = (smpResSignalPkt.rf_len)>>8;
    smemcpy(pkt_l2cap_data1+4, &smpResSignalPkt.l2capLen, smpResSignalPkt.rf_len);
    BLT_HOST_DBUG(DBG_L2CAP_BTSNOOP_LOG, "[BTSNOOP]tx l2cap data is 0x%s", hex_to_str(pkt_l2cap_data1, smpResSignalPkt.rf_len+4));

#endif

    return (u8*)&smpResSignalPkt;
}



















u8 * blt_smp_l2capSmpCmdHandler(u16 connHandle, u8 * p)
{
    int smp_master_role = (connHandle & BLM_CONN_HANDLE);
    u8 conn_idx = connHandle & CONN_IDX_MASK;


    ///////////////// SMP Data Entry ////////////////////
    u8 smp_prop_idx = smp_master_role ? 0: (blt_gap_getSlaveDeviceIndex_by_connIdx(conn_idx) + 1);
    u8 smp_status_idx = smp_master_role ? 0 : (conn_idx - LL_MAX_ACL_CEN_NUM + 1);
#if (SMP_PERIPHERAL_LEVEL_CFG_SEPARATE_EN)
    if(smpMng.dynamic_cfg_smp) //attention: conflict with multiple local device function !!!
    {
        blms_p_prop  = (smp_prop_t *)&blt_smpProp[smp_status_idx];
    }
    else
#endif
    {
        blms_p_prop  = (smp_prop_t *)&blt_smpProp[smp_prop_idx];
    }

    blms_p_own  = (smp_param_own_t *)&smp_param_own[smp_status_idx];
    blms_p_peer = (smp_param_peer_t *)&smp_param_peer[smp_status_idx];
    blms_p_sts  = (smp_st_t *)&smp_sts_param[conn_idx];




    rf_packet_l2cap_req_t * req = (rf_packet_l2cap_req_t *)p;
    //tlkapi_send_string_data((stkLog_mask & DBG_LOG_SMP_RX), "[ATT][RX] SMP Req", &req->chanId, req->l2capLen + 2);
    u8 opcode = req->opcode;
    u8 param_evt[8];

    switch(opcode){

        case SMP_OP_PAIRING_REQ :
        {
            if(!smp_master_role)  //Slave
            {
                smemcpy((u8*)&blms_p_own->pairing_req, &req->opcode, 7);//[Note:]The following operations cannot be modified "blms_p_own->pairing_req"

                blms_p_own->peerKey_mask = 0;
                blms_p_sts->tk_status = 0;  //TK status clear
                blms_p_sts->bonding_enable = 0;
                blms_p_sts->key_distribute = 0;

                blc_SecReq_ctrl.secReq_pending &= ~BIT(conn_idx); //clear security request pending flag

                /***************************************************************
                 * Check parameters of both sides,  and determine :
                 * 1. which pairing used:  legacy pairing or Secure Connection
                 * 2. which stk generate methods used:  just works/OOB/pass_key
                 *    entry/numeric comparison
                 ***************************************************************/
                blms_p_own ->sc_pairing = (blms_p_own->pairing_req.authReq.SC && blms_p_own->pairing_rsp.authReq.SC) ? 1 : 0;
                blms_p_own->stk_method = blt_smp_getStkGenMethod(blms_p_own, blms_p_own->sc_pairing);

                /********************************************************************************
                  Pairing begin
                 *******************************************************************************/
                if(gap_eventMask & GAP_EVT_MASK_SMP_PAIRING_BEGIN){
                    gap_smp_pairingBeginEvt_t *pEvt = (gap_smp_pairingBeginEvt_t *)param_evt;
                    pEvt->connHandle  = connHandle;
                    pEvt->secure_conn = blms_p_own->sc_pairing;
                    pEvt->tk_method   = blms_p_own->stk_method;
                    blc_gap_send_event(GAP_EVT_SMP_PAIRING_BEGIN, param_evt, sizeof(gap_smp_pairingBeginEvt_t) );
                }

                #if (CUSTOM_DARWIN_FMN_ENABLE)
                    if (custom_darwin_fmn.pair_req_cb) {
                        u8 ret = custom_darwin_fmn.pair_req_cb();
                        if (ret) {  //FMN already paired
                            //pairing fail event to tell upper layer
                            blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_PAIRING_NOT_SUPPORTED);  //pairing end with failure
                            return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_PAIRING_NOT_SUPPORTED);
                        }
                    }
                #endif

            #if (SMP_SEC_LEVEL_CHECK_EN)
                bool leSecLvlChkPass = FALSE;
                u8 pairingFailReason = PAIRING_FAIL_REASON_AUTH_REQUIRE;
                my_dump_str_data(SMP_DBG_EN, "tk_method", &blms_p_own->stk_method, 1);

                /* mode 1 level1 */
                if (blms_p_prop->security_level == No_Authentication_No_Encryption){ //only this lowest security level, can not support pairing
                    my_dump_str_data(SMP_DBG_EN, "LE_Security_Mode_1_Level_1", 0, 0);
                    pairingFailReason = PAIRING_FAIL_REASON_PAIRING_NOT_SUPPORTED;
                }
                /* mode 1 level4 */
                else if((blms_p_prop->security_level & LE_Security_Mode_1_Level_4) == LE_Security_Mode_1_Level_4){
                    my_dump_str_data(SMP_DBG_EN, "  LE_Security_Mode_1_Level_4 check", 0, 0);
                    /* About SC only:
                     * If need to check SC level4 only (Notice: Here we refer to SC only corresponding to LE mode1 level4.)
                     * Refer to Core5.2 Spec | Vol 3, Part C page 1375
                     * A device may be in a Secure Connections Only mode. When in Secure Connections Only mode only security
                     * mode 1 level 4 shall be used except for services that only require security mode 1 level 1.
                     */
                    if(blms_p_own ->sc_pairing && PK_Init_Display_Resp_Input <= blms_p_own->stk_method && blms_p_own->stk_method <= Numeric_Comparison){
                        leSecLvlChkPass = TRUE;
                        my_dump_str_data(SMP_DBG_EN, "LE_Security_Mode_1_Level_4", 0, 0);
                        #if(SMP_DBG_EN)
                        if(blms_p_prop->IO_capability == IO_CAPABILITY_DISPLAY_ONLY){
                            //SC PK_Resp_Display_Init_Input
                            my_dump_str_data(SMP_DBG_EN, "SC PK_Resp_Display_Init_Input", 0, 0);
                        }
                        else if(blms_p_prop->IO_capability == IO_CAPABILITY_DISPLAY_YES_NO){
                            //SC PK_Resp_Display_Init_Input/SC Numeric_Comparison
                            my_dump_str_data(SMP_DBG_EN, "SC PK_Resp_Display_Init_Input/Numeric_Comparison", 0, 0);
                        }
                        else if(blms_p_prop->IO_capability == IO_CAPABILITY_KEYBOARD_ONLY){
                            //SC PK_Init_Display_Resp_Input or PK_BOTH_INPUT
                            my_dump_str_data(SMP_DBG_EN, "SC PK_Init_Display_Resp_Input or PK_BOTH_INPUT", 0, 0);
                        }
                        else if(blms_p_prop->IO_capability == IO_CAPABILITY_KEYBOARD_DISPLAY){
                            //SC PK_Resp_Display_Init_Input/PK_Init_Display_Resp_Input/Numeric_Comparison
                            my_dump_str_data(SMP_DBG_EN, "SC PK_Resp_Display_Init_Input/PK_Init_Display_Resp_Input/Numeric_Comparison", 0, 0);
                        }
                        #endif
                    }
                }
                /* mode 1 level3 */
                if(!leSecLvlChkPass && (blms_p_prop->security_level & LE_Security_Mode_1_Level_3) == LE_Security_Mode_1_Level_3){
                    my_dump_str_data(SMP_DBG_EN, "  LE_Security_Mode_1_Level_3 check", 0, 0);
                    if(!blms_p_own ->sc_pairing && PK_Init_Display_Resp_Input <= blms_p_own->stk_method && blms_p_own->stk_method <= OOB_Authentication){ //OOB reject latter
                        leSecLvlChkPass = TRUE;
                        my_dump_str_data(SMP_DBG_EN, "LE_Security_Mode_1_Level_3", 0, 0);
                        #if(SMP_DBG_EN)
                        if(blms_p_prop->IO_capability == IO_CAPABILITY_DISPLAY_ONLY || blms_p_prop->IO_capability == IO_CAPABILITY_DISPLAY_YES_NO){
                            //LG PK_Resp_Display_Init_Input
                            my_dump_str_data(SMP_DBG_EN, "LG PK_Resp_Display_Init_Input", 0, 0);
                        }
                        else if(blms_p_prop->IO_capability == IO_CAPABILITY_KEYBOARD_ONLY){
                            //LG PK_Init_Display_Resp_Input/PK_BOTH_INPUT
                            my_dump_str_data(SMP_DBG_EN, "LG PK_Init_Display_Resp_Input/PK_BOTH_INPUT", 0, 0);
                        }
                        else if(blms_p_prop->IO_capability == IO_CAPABILITY_KEYBOARD_DISPLAY){
                            //LG PK_Resp_Display_Init_Input or PK_Init_Display_Resp_Input
                            my_dump_str_data(SMP_DBG_EN, "LG PK_Resp_Display_Init_Input/PK_Init_Display_Resp_Input", 0, 0);
                        }
                        #endif
                    }
                }
                /* mode 1 level2 */
                if(!leSecLvlChkPass && (blms_p_prop->security_level & LE_Security_Mode_1_Level_2) == LE_Security_Mode_1_Level_2){
                    my_dump_str_data(SMP_DBG_EN, "  LE_Security_Mode_1_Level_2 check", 0, 0);
                    /* SC does not matter whether it is switched or not */
                    if((1 || !blms_p_own ->sc_pairing) && blms_p_own->stk_method == JustWorks){
                        leSecLvlChkPass = TRUE;
                        my_dump_str_data(SMP_DBG_EN, "LE_Security_Mode_1_Level_2", 0, 0);
                        my_dump_str_data(SMP_DBG_EN, "LG JustWorks/SC JustWorks", &blms_p_own ->sc_pairing, 1);
                    }
                }

                if(leSecLvlChkPass == FALSE){
                    my_dump_str_data(SMP_DBG_EN, "The negotiated security level does not meet the requirements, pairing failed", 0, 0);
                    //notify upper layer security level can not meet
                    blt_smp_procPairingEnd(connHandle, pairingFailReason);  //pairing end with failure
                    return blt_smp_pushPairingFailed (pairingFailReason);
                }
            #else
                //only this lowest security level, can not support pairing
                u8 slave_dev_idx = blt_gap_getSlaveDeviceIndex_by_connIdx(conn_idx);
                if (blms_p_prop->security_level == No_Authentication_No_Encryption ||
                        (smpMng.dev_exceed_max_strategy == NEW_DEVICE_REJECT_WHEN_PER_MAX_BONDING_NUM && smpMStblBondDevice.slave_cur_bondNum[slave_dev_idx] >= smpMng.slave_max_bondNum)){
                    //pairing fail event to tell upper layer
                    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_PAIRING_NOT_SUPPORTED);  //pairing end with failure

                    return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_PAIRING_NOT_SUPPORTED);
                }

                u8 level4only = ((blms_p_prop->security_level & LE_Security_Mode_1) == LE_Security_Mode_1_Level_4) ? 1 : 0;

                //if need to check SC level4 only (Notice: Here we refer to SC only corresponding to LE mode1 level4.)
                /* Refer to Core5.2 Spec | Vol 3, Part C page 1375
                 * A device may be in a Secure Connections Only mode. When in Secure Connections Only mode only security
                 * mode 1 level 4 shall be used except for services that only require security mode 1 level 1.
                 */
                if((!blms_p_own->sc_pairing) || (blms_p_own->sc_pairing && blms_p_own->stk_method == JustWorks)){
                    //if the local gap setting only support level4 only,we should response pairing failed
                    if(level4only){
                        //notify upper layer security level can not meet
                        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_AUTH_REQUIRE);  //pairing end with failure
                        return blt_smp_pushPairingFailed (PAIRING_FAIL_REASON_AUTH_REQUIRE);
                    }
                }
            #endif

                if(blms_p_own->pairing_req.maxEncrySize < ENCRYPTION_KEY_SIZE_MINIMUM){
                    //pairing fail event to tell upper layer
                    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_ENCRYPT_KEY_SIZE);  //pairing end with failure
                    return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_ENCRYPT_KEY_SIZE);
                }
                else if(blms_p_own->pairing_req.maxEncrySize > ENCRYPTION_KEY_SIZE_MAXIMUM) //encryption key size: 7~16
                {
                    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_INVALID_PARAMETER);
                    return blt_smp_pushPairingFailed (PAIRING_FAIL_REASON_INVALID_PARAMETER);
                }

                blms_p_own->encrypt_key_size =  min(blms_p_own->pairing_req.maxEncrySize, blms_p_own->pairing_rsp.maxEncrySize);
                blms_p_sts->bonding_enable = blms_p_prop->bonding_mode && blms_p_own->pairing_req.authReq.bondingFlag;  // determine bonding final here

                if(blms_p_own ->sc_pairing){ //secure connection enable
                    //Fix PTS case:GAP/SEC/SEM/BI-09-C: LE Mode1 level4, encryption key size must be 16
                    if((blms_p_own->stk_method != JustWorks) && (blms_p_own->encrypt_key_size != ENCRYPTION_KEY_SIZE_MAXIMUM))
                    {
                        //pairing fail event to tell upper layer
                        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_ENCRYPT_KEY_SIZE);
                        return blt_smp_pushPairingFailed (PAIRING_FAIL_REASON_ENCRYPT_KEY_SIZE);
                    }

                    blt_smp_setResponderKey(blms_p_own, 0, 1, 0); //TODO: when data signing OK later, add CSRK here

                    // Generate ECDH keys
                    if(func_smp_sc_proc){
                        func_smp_sc_proc(connHandle, p); //blt_smp_sc_handler
                    }
                }
                else
                {
                    //re-init Key Distribution bits

                    /*
                     * if 1st time use SC, then unpaired, and 2nd time(do not re-power) use LG, key distribution bit will be Err.
                     */
                    blt_smp_setResponderKey(blms_p_own, 1, 1, 0); //TODO: when data signing OK later, add CSRK here

                    blms_p_own->auth_req.SC = 0;
                    blms_p_own->pairing_rsp.authReq = blms_p_own->auth_req;
                }


                /***************************************************************
                 * Key Generate for a new Pairing
                 ***************************************************************/
                //Generate SRand
                generateRandomNum(16, blms_p_own->own_rand);

                if(!blms_p_own->sc_pairing){  //SC no need LTK here
                    generateRandomNum(16, blms_p_own->own_ltk);

                    #if (SMP_LEGACY_LTK_VERIFICATION_EN)
                        if(smp_setLtkVerification){
                            u16 crc16_ = blt_Crc16ComputeInternal(blms_p_own->own_ltk, 14);
                            blms_p_own->own_ltk[14] = crc16_ &0xFF;
                            blms_p_own->own_ltk[15] = (crc16_>>8) &0xFF;
                            my_dump_str_data(SMP_DBG_EN, "[ltk] crc16_", &crc16_, 2);
                        }
                    #endif
                }

                blms_p_own->pairing_rsp.initKeyDistribution.keyIni &= blms_p_own->pairing_req.initKeyDistribution.keyIni;
                blms_p_own->pairing_rsp.rspKeyDistribution.keyIni  &= blms_p_own->pairing_req.rspKeyDistribution.keyIni;

                blms_p_sts->smp_DistributeKeyRecv.keyIni = blms_p_own->pairing_rsp.initKeyDistribution.keyIni;   //s-role: key receive
                blms_p_sts->smp_DistributeKeySend.keyIni = blms_p_own->pairing_rsp.rspKeyDistribution.keyIni;    //s-role: key send

                /*
                 * M->S Pairing Req: phase1 begin
                 * S->M Pairing Rsp:
                 */
                blms_p_sts->smp_phase_chk = PAIRING_PHASE_1_OK; //SMP Phase stage 1 begin


                /*
                 ***************************************************************
                 * Send corresponding event to upper layer according to TK generate methods
                 ***************************************************************
                 */
                if(blms_p_own->stk_method == OOB_Authentication){
                    if(!blms_p_own->sc_pairing){
                        blms_p_sts->tk_status = TK_ST_REQUEST;
                        if(gap_eventMask & GAP_EVT_MASK_SMP_TK_REQUEST_OOB){
                            gap_smp_TkRequestOOBEvt_t* pEvt = (gap_smp_TkRequestOOBEvt_t*)param_evt;
                            pEvt->connHandle = connHandle;
                            blc_gap_send_event(GAP_EVT_SMP_TK_REQUEST_OOB, param_evt, sizeof(gap_smp_TkRequestOOBEvt_t));
                        }
                    }
                #if(!SMP_SC_OOB_EN)
                    else{
                        //SC OOB is not supported.
                        //pairing fail event to tell upper layer.
                        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_PAIRING_NOT_SUPPORTED);  //pairing end with failure
                        return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_PAIRING_NOT_SUPPORTED);
                    }
                #endif
                }
                else if(blms_p_own->stk_method == PK_Resp_Display_Init_Input){
                    //Responder generate TK value(0~999999) , should notify upper layer to display this number,
                    //then initiator input this number to set TK value upon watching the displayed value
                    int tk_set = blms_p_prop->passKeyEntryDftTK;
                    if(tk_set > 0 && tk_set <= 999999){
                        memset(blms_p_own->pairing_tk, 0, 16);
                        smemcpy(blms_p_own->pairing_tk, &tk_set, 4);
                    }
                    else{
                        #if (1)
                            generateRandomNum(4, (u8*)&tk_set);
                            if(tk_set <= 99999){ //"099999" -> Hex:0xF423F
                                tk_set += 100000;
                            }
                            tk_set &= 999999;//0~999999
                            memset(blms_p_own->pairing_tk, 0, 16);
                            smemcpy(blms_p_own->pairing_tk, &tk_set, 4);
                        #else
                            tk_set = rand() & 0xFFFF;  // should be "x % 1000000", here "x & 0xFFFF" equal to "x % 65536",
                                                       // use this to save code size and code run time
                        #endif
                    }

                    if(gap_eventMask & GAP_EVT_MASK_SMP_TK_DISPLAY){
                        gap_smp_TkDisplayEvt_t* pEvt = (gap_smp_TkDisplayEvt_t*)param_evt;
                        pEvt->connHandle = connHandle;
                        pEvt->tk_pincode = tk_set;
                        blc_gap_send_event(GAP_EVT_SMP_TK_DISPLAY, param_evt, sizeof(gap_smp_TkDisplayEvt_t));
                    }
                }

                if(!blms_p_own->sc_pairing){ //Legacy
                    if(blms_p_own->stk_method == PK_BOTH_INPUT || blms_p_own->stk_method == PK_Init_Display_Resp_Input){
                        // both sides should input TK value, here send TK request event to upper layer,
                        // expect upper layer call "blc_smp_setTK_by_PasskeyEntry"  to set TK value
                        blms_p_sts->tk_status = TK_ST_REQUEST;
                        if(gap_eventMask & GAP_EVT_MASK_SMP_TK_REQUEST_PASSKEY){
                            gap_smp_TkReqPassKeyEvt_t* pEvt = (gap_smp_TkReqPassKeyEvt_t*)param_evt;
                            pEvt->connHandle = connHandle;
                            blc_gap_send_event(GAP_EVT_SMP_TK_REQUEST_PASSKEY, param_evt, sizeof(gap_smp_TkReqPassKeyEvt_t));
                        }
                    }
                }

                blt_smp_setPairingBusy(connHandle, 1);

                #if SMP_REAL_ENCRYPTION_BUSY_ENABLE
                    if (!real_encryption_busy_enable)
                #endif
                    {
                        //Set in advance after Pairing_REQ/RSP interaction, and in Core Spec set after receiving LL_ENC_REQ
                        blt_ll_setEncryptionBusy(connHandle, 1);
                    }
                return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_RSP);
            }
            else{
                goto smpCmdProcErr;
            }
        }
        break;
        case SMP_OP_PAIRING_RSP :  // from responder(slave)
        {
            if(smp_master_role){
                smemcpy((u8*)&blms_p_own->pairing_rsp, (u8*)&req->opcode, 7);


                /***************************************************************
                 * Check parameters of both sides,  and determine :
                 * 1. which pairing used:  legacy pairing or Secure Connection
                 * 2. which stk generate methods used:  just works/OOB/pass_key
                 *    entry/numeric comparison
                 ***************************************************************/
                blms_p_own ->sc_pairing = (blms_p_own->pairing_req.authReq.SC && blms_p_own->pairing_rsp.authReq.SC) ? 1 : 0;
                blms_p_own->stk_method = blt_smp_getStkGenMethod(blms_p_own, blms_p_own->sc_pairing);

                /********************************************************************************
                  Pairing begin
                 *******************************************************************************/
                if(gap_eventMask & GAP_EVT_MASK_SMP_PAIRING_BEGIN){
                    gap_smp_pairingBeginEvt_t *pEvt = (gap_smp_pairingBeginEvt_t *)param_evt;
                    pEvt->connHandle = connHandle;
                    pEvt->secure_conn = blms_p_own->sc_pairing;
                    pEvt->tk_method = blms_p_own->stk_method;

                    blc_gap_send_event ( GAP_EVT_SMP_PAIRING_BEGIN, param_evt, sizeof(gap_smp_pairingBeginEvt_t) );
                }

            #if (SMP_SEC_LEVEL_CHECK_EN)
                bool leSecLvlChkPass = FALSE;
                my_dump_str_data(SMP_DBG_EN, "tk_method", &blms_p_own->stk_method, 1);

                /* mode 1 level1 */
                if (blms_p_prop->security_level == No_Authentication_No_Encryption){ //only this lowest security level, can not support pairing
                    my_dump_str_data(SMP_DBG_EN, "LE_Security_Mode_1_Level_1", 0, 0);
                    //not possible!!! can be removed
                }
                /* mode 1 level4 */
                else if((blms_p_prop->security_level & LE_Security_Mode_1_Level_4) == LE_Security_Mode_1_Level_4){
                    my_dump_str_data(SMP_DBG_EN, "  LE_Security_Mode_1_Level_4 check", 0, 0);
                    /* About SC only:
                     * If need to check SC level4 only (Notice: Here we refer to SC only corresponding to LE mode1 level4.)
                     * Refer to Core5.2 Spec | Vol 3, Part C page 1375
                     * A device may be in a Secure Connections Only mode. When in Secure Connections Only mode only security
                     * mode 1 level 4 shall be used except for services that only require security mode 1 level 1.
                     */
                    if(blms_p_own ->sc_pairing && PK_Init_Display_Resp_Input <= blms_p_own->stk_method && blms_p_own->stk_method <= Numeric_Comparison){
                        leSecLvlChkPass = TRUE;
                        my_dump_str_data(SMP_DBG_EN, "LE_Security_Mode_1_Level_4", 0, 0);
                        #if(SMP_DBG_EN)
                        if(blms_p_prop->IO_capability == IO_CAPABILITY_DISPLAY_ONLY){
                            //SC PK_Resp_Display_Init_Input
                            my_dump_str_data(SMP_DBG_EN, "SC PK_Init_Display_Resp_Input", 0, 0);
                        }
                        else if(blms_p_prop->IO_capability == IO_CAPABILITY_DISPLAY_YES_NO){
                            //SC PK_Resp_Display_Init_Input/SC Numeric_Comparison
                            my_dump_str_data(SMP_DBG_EN, "SC PK_Init_Display_Resp_Input/Numeric_Comparison", 0, 0);
                        }
                        else if(blms_p_prop->IO_capability == IO_CAPABILITY_KEYBOARD_ONLY){
                            //SC PK_Init_Display_Resp_Input or PK_BOTH_INPUT
                            my_dump_str_data(SMP_DBG_EN, "SC PK_Resp_Display_Init_Input or PK_BOTH_INPUT", 0, 0);
                        }
                        else if(blms_p_prop->IO_capability == IO_CAPABILITY_KEYBOARD_DISPLAY){
                            //SC PK_Resp_Display_Init_Input/PK_Init_Display_Resp_Input/Numeric_Comparison
                            my_dump_str_data(SMP_DBG_EN, "SC PK_Resp_Display_Init_Input/PK_Init_Display_Resp_Input/Numeric_Comparison", 0, 0);
                        }
                        #endif
                    }
                }
                /* mode 1 level3 */
                if(!leSecLvlChkPass && (blms_p_prop->security_level & LE_Security_Mode_1_Level_3) == LE_Security_Mode_1_Level_3){
                    my_dump_str_data(SMP_DBG_EN, "  LE_Security_Mode_1_Level_3 check", 0, 0);
                    if(!blms_p_own ->sc_pairing && PK_Init_Display_Resp_Input <= blms_p_own->stk_method && blms_p_own->stk_method <= OOB_Authentication){ //OOB reject latter
                        leSecLvlChkPass = TRUE;
                        my_dump_str_data(SMP_DBG_EN, "LE_Security_Mode_1_Level_3", 0, 0);
                        #if(SMP_DBG_EN)
                        if(blms_p_prop->IO_capability == IO_CAPABILITY_DISPLAY_ONLY || blms_p_prop->IO_capability == IO_CAPABILITY_DISPLAY_YES_NO){
                            //LG PK_Resp_Display_Init_Input
                            my_dump_str_data(SMP_DBG_EN, "LG PK_Init_Display_Resp_Input", 0, 0);
                        }
                        else if(blms_p_prop->IO_capability == IO_CAPABILITY_KEYBOARD_ONLY){
                            //LG PK_Init_Display_Resp_Input/PK_BOTH_INPUT
                            my_dump_str_data(SMP_DBG_EN, "LG PK_Resp_Display_Init_Input/PK_BOTH_INPUT", 0, 0);
                        }
                        else if(blms_p_prop->IO_capability == IO_CAPABILITY_KEYBOARD_DISPLAY){
                            //LG PK_Resp_Display_Init_Input or PK_Init_Display_Resp_Input
                            my_dump_str_data(SMP_DBG_EN, "LG PK_Resp_Display_Init_Input/PK_Init_Display_Resp_Input", 0, 0);
                        }
                        #endif
                    }
                }
                /* mode 1 level2 */
                if(!leSecLvlChkPass && (blms_p_prop->security_level & LE_Security_Mode_1_Level_2) == LE_Security_Mode_1_Level_2){
                    my_dump_str_data(SMP_DBG_EN, "  LE_Security_Mode_1_Level_2 check", 0, 0);
                    /* SC does not matter whether it is switched or not */
                    if((1 || !blms_p_own ->sc_pairing) && blms_p_own->stk_method == JustWorks){
                        leSecLvlChkPass = TRUE;
                        my_dump_str_data(SMP_DBG_EN, "LE_Security_Mode_1_Level_2", 0, 0);
                        my_dump_str_data(SMP_DBG_EN, "LG JustWorks/SC JustWorks", &blms_p_own ->sc_pairing, 1);
                    }
                }

                if(leSecLvlChkPass == FALSE){
                    my_dump_str_data(SMP_DBG_EN, "The negotiated security level does not meet the requirements, pairing failed", 0, 0);
                    //notify upper layer security level can not meet
                    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_AUTH_REQUIRE);  //pairing end with failure
                    return blt_smp_pushPairingFailed (PAIRING_FAIL_REASON_AUTH_REQUIRE);
                }
            #else
                u8 level4only = ((blms_p_prop->security_level & LE_Security_Mode_1) == LE_Security_Mode_1_Level_4) ? 1 : 0;

                //if need to check SC level4 only (Notice: Here we refer to SC only corresponding to LE mode1 level4.)
                if((!blms_p_own->sc_pairing) || (blms_p_own->sc_pairing && blms_p_own->stk_method == JustWorks)){
                    //if the local gap setting only support level4 only,we should response pairing failed
                    /* Refer to Core5.2 Spec | Vol 3, Part C page 1375
                     * A device may be in a Secure Connections Only mode. When in Secure Connections Only mode only security
                     * mode 1 level 4 shall be used except for services that only require security mode 1 level 1.
                     */
                    if(level4only){
                        //notify upper layer security level can not meet
                        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_AUTH_REQUIRE);  //pairing end with failure
                        return blt_smp_pushPairingFailed (PAIRING_FAIL_REASON_AUTH_REQUIRE);
                    }
                }
            #endif

                if(blms_p_own->pairing_rsp.maxEncrySize < ENCRYPTION_KEY_SIZE_MINIMUM){
                    //pairing fail event to tell upper layer
                    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_ENCRYPT_KEY_SIZE);  //pairing end with failure
                    return blt_smp_pushPairingFailed (PAIRING_FAIL_REASON_ENCRYPT_KEY_SIZE);
                }
                else if(blms_p_own->pairing_req.maxEncrySize > ENCRYPTION_KEY_SIZE_MAXIMUM) //encryption key size: 7~16
                {
                    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_INVALID_PARAMETER);
                    return blt_smp_pushPairingFailed (PAIRING_FAIL_REASON_INVALID_PARAMETER);
                }

                blms_p_own->encrypt_key_size = min(blms_p_own->pairing_req.maxEncrySize, blms_p_own->pairing_rsp.maxEncrySize);
                blms_p_sts->bonding_enable = blms_p_prop->bonding_mode && blms_p_own->pairing_rsp.authReq.bondingFlag;


                if(blms_p_own ->sc_pairing){ //secure connection enable
                    //Fix PTS case:GAP/SEC/SEM/BI-10-C: LE Mode1 level4, encryption key size must be 16
                    if((blms_p_own->stk_method != JustWorks) && (blms_p_own->encrypt_key_size != ENCRYPTION_KEY_SIZE_MAXIMUM))
                    {
                        //pairing fail event to tell upper layer
                        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_ENCRYPT_KEY_SIZE);
                        return blt_smp_pushPairingFailed (PAIRING_FAIL_REASON_ENCRYPT_KEY_SIZE);
                    }

                    blt_smp_setInitiatorKey(blms_p_own, 0, 1, 0); //TODO: when data signing OK later, add CSRK here

                    // Generate ECDH keys
                    if(func_smp_sc_proc){
                        func_smp_sc_proc(connHandle, p); //blt_smp_sc_handler
                    }
                }

                /*************************************************************
                 * Key Generate for a new Pairing
                 **************************************************************/
                //Generate MRand
                generateRandomNum(16, blms_p_own->own_rand);

                if(!blms_p_own->sc_pairing){  //SC no need LTK here
                    generateRandomNum(16, blms_p_own->own_ltk);
                }

                blms_p_sts->smp_DistributeKeySend.keyIni = (blms_p_own->pairing_rsp.initKeyDistribution.keyIni & blms_p_own->pairing_req.initKeyDistribution.keyIni);   //m-role: key send
                blms_p_sts->smp_DistributeKeyRecv.keyIni = (blms_p_own->pairing_rsp.rspKeyDistribution.keyIni & blms_p_own->pairing_req.rspKeyDistribution.keyIni);    //m-role: key receive

                #if SMP_REAL_ENCRYPTION_BUSY_ENABLE
                    if (!real_encryption_busy_enable)
                #endif
                    {
                        //Set in advance after Pairing_REQ/RSP interaction, and in Core Spec set after receiving LL_ENC_REQ
                        blt_ll_setEncryptionBusy(connHandle, 1);
                    }
                blt_smp_setPairingBusy(connHandle, 1);

                /*
                 * M->S Pairing Req: phase1 begin
                 * S->M Pairing Rsp:
                 */
                blms_p_sts->smp_phase_chk = PAIRING_PHASE_1_OK; //SMP Phase stage 1 begin



                #if(!SMP_SC_OOB_EN)
                    /********************************************************************************
                    Send corresponding event to upper layer according to TK generate methods
                     *******************************************************************************/
                    if(blms_p_own->stk_method == OOB_Authentication && blms_p_own->sc_pairing){
                        //SC OOB is not supported.
                        //pairing fail event to tell upper layer
                        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_PAIRING_NOT_SUPPORTED);  //pairing end with failure
                        return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_PAIRING_NOT_SUPPORTED);
                    }
                    else
                #endif
                    if(blms_p_own->stk_method == PK_Init_Display_Resp_Input){
                    //Initiator generate TK value(0~999999) , should notify upper layer to display this number,
                    //then responder input this number to set TK value upon watching the displayed value
                    int tk_set = blms_p_prop->passKeyEntryDftTK;
                    if(tk_set > 0 && tk_set <= 999999){
                        memset(blms_p_own->pairing_tk, 0, 16);
                        smemcpy(blms_p_own->pairing_tk, &tk_set, 4);
                    }
                    else{
                        #if (1)
                            generateRandomNum(4, (u8*)&tk_set);
                            if(tk_set <= 99999){ //"099999"
                                tk_set += 100000;
                            }
                            tk_set &= 999999;//0~999999
                            memset(blms_p_own->pairing_tk, 0, 16);
                            smemcpy(blms_p_own->pairing_tk, &tk_set, 4);
                        #else
                            tk_set = rand() & 0xFFFF;  // should be "x % 1000000", here "x & 0xFFFF" equal to "x % 65536",
                                                       // use this to save code size and code run time
                        #endif
                    }

                    if(gap_eventMask & GAP_EVT_MASK_SMP_TK_DISPLAY){
                        gap_smp_TkDisplayEvt_t* pEvt = (gap_smp_TkDisplayEvt_t*)param_evt;
                        pEvt->connHandle = connHandle;
                        pEvt->tk_pincode = tk_set;
                        blc_gap_send_event(GAP_EVT_SMP_TK_DISPLAY, param_evt, sizeof(gap_smp_TkDisplayEvt_t));
                    }
                }

                if(blms_p_own->sc_pairing) //secure connection enable
                {
                #if (SMP_SC_OOB_EN)
                    if(blms_p_own->stk_method == SC_OOB_Authentication){ //master SC OOB

                        if(gap_eventMask & GAP_EVT_MASK_SMP_REQUEST_SCOOB_DATA){
                            blms_p_sts->tk_status = TK_ST_REQUEST;
                            //oob data check
                            bool scOobLocalUsed, scOobRemoteUsed;
                            //see peer's flag decide own sc oob data used
                            scOobLocalUsed = (blms_p_own->pairing_rsp.oobDataFlag);
                            scOobRemoteUsed = (blms_p_own->pairing_req.oobDataFlag);

                            //clear
                            blms_p_own->scoob_local = NULL;
                            blms_p_own->scoob_remote = NULL;
                            blms_p_own->scoob_local_key = NULL;

                            gap_smp_requestScOobDataEvt_t* pEvt = (gap_smp_requestScOobDataEvt_t*)param_evt;
                            pEvt->connHandle = connHandle;
                            pEvt->scOobLocalUsed = scOobLocalUsed;
                            pEvt->scOobRemoteUsed = scOobRemoteUsed;
                            blc_gap_send_event(GAP_EVT_SMP_REQUEST_SCOOB_DATA, param_evt, sizeof(gap_smp_requestScOobDataEvt_t));

                            return NULL; //Attention: to do pending process, here do noting
                        }
                    }
                #endif
                    return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_PUBLIC_KEY);
                }
                else
                {
                    if(blms_p_own->stk_method == OOB_Authentication){
                        blms_p_sts->tk_status = TK_ST_REQUEST;
                        if(gap_eventMask & GAP_EVT_MASK_SMP_TK_REQUEST_OOB){
                            gap_smp_TkRequestOOBEvt_t* pEvt = (gap_smp_TkRequestOOBEvt_t*)param_evt;
                            pEvt->connHandle = connHandle;
                            blc_gap_send_event(GAP_EVT_SMP_TK_REQUEST_OOB, param_evt, sizeof(gap_smp_TkRequestOOBEvt_t));
                        }

                        if((blms_p_sts->tk_status & TK_ST_REQUEST) && !(blms_p_sts->tk_status & TK_ST_UPDATE)){
                            blms_p_sts->tk_status |= TK_ST_CONFIRM_PENDING;//pending
                            return NULL;
                        }
                    }
                    else if(blms_p_own->stk_method == PK_BOTH_INPUT || blms_p_own->stk_method == PK_Resp_Display_Init_Input){
                        blms_p_sts->tk_status = TK_ST_REQUEST;
                        if(gap_eventMask & GAP_EVT_MASK_SMP_TK_REQUEST_PASSKEY){
                            gap_smp_TkReqPassKeyEvt_t* pEvt = (gap_smp_TkReqPassKeyEvt_t*)param_evt;
                            pEvt->connHandle = connHandle;
                            blc_gap_send_event(GAP_EVT_SMP_TK_REQUEST_PASSKEY, param_evt, sizeof(gap_smp_TkReqPassKeyEvt_t));
                        }

                        if((blms_p_sts->tk_status & TK_ST_REQUEST) && !(blms_p_sts->tk_status & TK_ST_UPDATE)){
                            blms_p_sts->tk_status |= TK_ST_CONFIRM_PENDING;//pending
                            return NULL;
                        }
                    }

                    return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_CONFIRM);
                }
            }
            else{
                goto smpCmdProcErr;
            }
        }
        break;
        case SMP_OP_PAIRING_CONFIRM:
        {
            u8* smpProcErr = NULL;

            if(blms_p_own->sc_pairing) //secure connection enable
            {
                if(func_smp_sc_proc){
                    /*
                     * Attention: must not return, SC and LG use common proc, if here return Not NULL,
                     * means smp processing err.
                     */
                    smpProcErr = func_smp_sc_proc(connHandle, p);
                }
            }
            else
            {
                smemcpy (blms_p_peer->peer_confirm, req->data, 16);

                /*
                 * Legacy Pairing:
                 * M->S Pairing Req: phase1 begin
                 * S->M Pairing Rsp:
                 * M->S Pairing Confirm: (slave role)Pairing Confirm marked. phase2 begin
                 * S->M Pairing Confirm  (master role)Pairing Confirm marked
                 */
                if(blms_p_sts->smp_phase_chk != PAIRING_PHASE_1_OK){ //Ensure the integrity of the pairing process
                    goto smpCmdProcErr;
                }

                blms_p_sts->smp_phase_chk = BIT(SMP_OP_PAIRING_CONFIRM);
            }

            //SC and LG use common proc
            if(smp_master_role){
                if(smpProcErr){ //means smp phase check err
                    return smpProcErr;
                }
                return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_RANDOM);
            }
            else{
                int TK_ok = 1;
                if(blms_p_own->stk_method == PK_Init_Display_Resp_Input || blms_p_own->stk_method == PK_BOTH_INPUT || blms_p_own->stk_method == OOB_Authentication){
                    TK_ok = 0;
                    if(blms_p_sts->tk_status & TK_ST_REQUEST){  //has send TK request event to upper layer
                        if(blms_p_sts->tk_status & TK_ST_UPDATE){  //TK set by upper layer completed
                            TK_ok = 1;
                        }
                        else{
                            // check it in gap mainLoop, if TK set, send peer_confirm to peer device
                            blms_p_sts->tk_status |= TK_ST_CONFIRM_PENDING;
                        }
                    }
                    else{  //no need upper layer set TK
                        TK_ok = 1;
                    }
                }

                if(TK_ok){
                    return blt_smp_pushSmpCmdPkt (connHandle, SMP_OP_PAIRING_CONFIRM);
                }
            }
        }
        break;
        case SMP_OP_PAIRING_RANDOM:
        {
            if(blms_p_own->sc_pairing) //secure connection enable
            {
                if(func_smp_sc_proc){
                    return func_smp_sc_proc(connHandle, p);
                }
            }
            else
            {
                smemcpy(blms_p_peer->peer_pairing_rand, req->data, 16);

                u8 pairing_conf[16] = {0};

                if(smp_master_role){

                    /*
                     * Legacy Pairing:
                     * M->S Pairing Confirm:
                     * S->M Pairing Confirm: Pairing Confirm marked
                     * M->S Pairing Random:
                     * S->M Pairing Random: Pairing Random marked
                     * M->S LL_ENC_REQ: encryption start marked
                     */
                    if(!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_CONFIRM))){//Ensure the integrity of the pairing process
                        goto smpCmdProcErr;
                    }

                    blms_p_sts->smp_phase_chk = PAIRING_PHASE_2_ENC;

                    /* Checkout Sconfirm
                     *  Sconfirm = c1(TK, Srand, Pairing Request command, Pairing Response command,
                     *                   initiating device address type, initiating device address,
                     *                   responding device address type, responding device address)
                     **/
                    //aes_encryption_le in blt_crypto_alg_c1/blt_crypto_alg_s1 need critical data 4B aligned
                    blt_crypto_alg_c1(pairing_conf, blms_p_own->pairing_tk, blms_p_peer->peer_pairing_rand, (u8*)&blms_p_own->pairing_rsp,
                                  (u8*)&blms_p_own->pairing_req, blms_p_own->own_addr_type, blms_p_own->own_conn_addr,
                                  blms_p_peer->peer_addr_type, blms_p_peer->peer_conn_addr);

                    if (!memcmp(blms_p_peer->peer_confirm, pairing_conf, 16)){
                        u8 stk_temp[16];  //aes_encryption_le in blt_crypto_alg_c1/blt_crypto_alg_s1 need critical data 4B aligned
                        blt_crypto_alg_s1(stk_temp, blms_p_own->pairing_tk, blms_p_own->own_rand, blms_p_peer->peer_pairing_rand);
                        smemset(blms_p_peer->peer_ltk, 0, 16);
                        smemcpy(blms_p_peer->peer_ltk, stk_temp, blms_p_own->encrypt_key_size);

                        //smp4.0, after exchange smp random, then transport specific keys distribution
                        blms_p_sts->smpDistributeKeyOrder = SMP_TRANSPORT_SPECIFIC_KEY_START;

                        //send LL_ENC_REQ
                        blt_ll_startEncryption(connHandle, blms_p_peer->peer_ediv, blms_p_peer->peer_random, blms_p_peer->peer_ltk);

                        ///////////////  legacy pairing 1st  ////////////////
                        smpMStblBondDevice.keyIndex[conn_idx] = KEY_FLAG_NEW;  //mark that STK generated in pairing procedure, not previous existed keys
                    }
                    else{
                        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_CONFIRM_FAILED);
                        return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_CONFIRM_FAILED);
                    }
                }
                else{
                    /*
                     * Legacy Pairing:
                     * M->S Pairing Confirm: Pairing Confirm marked
                     * S->M Pairing Confirm:
                     * M->S Pairing Random: Pairing Random marked
                     * S->M Pairing Random:
                     */
                    if(!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_CONFIRM))){//Ensure the integrity of the pairing process
                        goto smpCmdProcErr;
                    }

                    blms_p_sts->smp_phase_chk = BIT(SMP_OP_PAIRING_RANDOM);

                    /* Checkout Mconfirm
                     *  Mconfirm = c1(TK, Mrand, Pairing Request command, Pairing Response command,
                     *                   initiating device address type, initiating device address,
                     *                   responding device address type, responding device address)
                     **/
                    //aes_encryption_le in blt_crypto_alg_c1/blt_crypto_alg_s1 need critical data 4B aligned
                    blt_crypto_alg_c1(pairing_conf, blms_p_own->pairing_tk, blms_p_peer->peer_pairing_rand, (u8*)&blms_p_own->pairing_rsp,
                                  (u8*)&blms_p_own->pairing_req, blms_p_peer->peer_addr_type, blms_p_peer->peer_conn_addr,
                                   blms_p_own->own_addr_type, blms_p_own->own_conn_addr);

                    if (!memcmp(blms_p_peer->peer_confirm, pairing_conf, 16)){
                        blms_p_sts->smpDistributeKeyOrder = SMP_TRANSPORT_SPECIFIC_KEY_START;
                        return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_RANDOM);
                    }
                    else{
                        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_CONFIRM_FAILED);  //pairing end with failure
                        return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_CONFIRM_FAILED);
                    }
                }
            }
        }
        break;
        case SMP_OP_ENC_INFO://LTK(16B) from peer device
        {

            blms_p_own->peerKey_mask |= BIT(0);

            if(smp_master_role){
                smemcpy(blms_p_peer->peer_ltk, req->data, 16);
            }
        }
        break;
        case SMP_OP_ENC_IDX://EDIV and Random from peer device
        {
            if(blms_p_own->peerKey_mask & BIT(0)){
                blms_p_own->peerKey_mask &= ~BIT(0);
            }
            else{ //check err: 1.LTK (lost) + 2.EDIV+RANDOM
                goto smpCmdProcErr;
            }

            blms_p_sts->smp_DistributeKeyRecv.encKey = 0;

            if(smp_master_role){
                //The master roles share a set of parameters, and it is important to note the reuse situation here.
                smemcpy((u8*)&blms_p_peer->peer_ediv, req->data , 2);
                smemcpy(blms_p_peer->peer_random, req->data + 2 , 8);

                //Slave send key completely. Master start key sending
                if(!blms_p_sts->smp_DistributeKeyRecv.keyIni){
                    blms_p_sts->key_distribute = 1;
                }
            }
            else{
                //[Note]: slave is not save EDIV and Random of master
                //if sending key and receiving key all completed, process pairing end
                if(!blms_p_sts->smp_DistributeKeyRecv.keyIni && !blms_p_sts->smp_DistributeKeySend.keyIni) {
                    blt_smp_saveBondingKey(connHandle);
                }
            }
        }
        break;
        case SMP_OP_ENC_IINFO://IRK from peer device
        {
            blms_p_own->peerKey_mask |= BIT(1);
            smemcpy(blms_p_peer->peer_irk, req->data, 16); // little endian
        }
        break;
        case SMP_OP_ENC_IADR://Identity Address (7B) from peer device
        {
            if(blms_p_own->peerKey_mask & BIT(1)){
                blms_p_own->peerKey_mask &= ~BIT(1);
            }
            else{ //check err: 1.IRK (lost) + 2.IADR
                goto smpCmdProcErr;
            }

            blms_p_sts->smp_DistributeKeyRecv.idKey = 0;

            blms_p_peer->peer_id_address_type = req->data[0];
            smemcpy(blms_p_peer->peer_id_address, req->data + 1, 6);

            if(smp_master_role){
                //if sending key and receiving key all completed, process pairing end
                if(!blms_p_sts->smp_DistributeKeyRecv.keyIni){
                    blms_p_sts->key_distribute = 1;

                    my_dump_str_data(SMP_DBG_EN, "key distribute begin", 0, 0);
                }
            }
            else{
                //It is found that the resolvable random address when the Redmi Note3 is paired, the public address given in
                //the key distribution, and the public address direct adv is not connected. The connectAddr and the identityAddr
                //may not be the same, so it needs to be distinguished. The resolving list needs to use this address.
                //Real master address from master

                //if sending key and receiving key all completed, process pairing end
                if(!blms_p_sts->smp_DistributeKeyRecv.keyIni && !blms_p_sts->smp_DistributeKeySend.keyIni) {
                    blt_smp_saveBondingKey(connHandle);
                }
            }
        }
        break;
        case SMP_OP_ENC_SIGN:   //CSRK from peer device
        {
            blms_p_sts->smp_DistributeKeyRecv.sign = 0;
            //smemcpy(blms_p_peer->peer_csrk, req->data , 16);//TODO:Currently not support CSRK

            if(smp_master_role){
                //if sending key and receiving key all completed, process pairing end
                if(!blms_p_sts->smp_DistributeKeyRecv.keyIni) {
                    blms_p_sts->key_distribute = 1;
                }
            }
            else{
                #if 0  // @@@ do later: process signCounter
                    swap128(smp_sign_info.csrk, blms_smpMng[idx].smp_param_peer.peer_csrk);
                    smp_sign_info.signCounter = 0xffffffff;
                #endif

                //if sending key and receiving key all completed, process pairing end
                if(!blms_p_sts->smp_DistributeKeyRecv.keyIni && !blms_p_sts->smp_DistributeKeySend.keyIni) {
                    blt_smp_saveBondingKey(connHandle);
                }
            }
        }
        break;
        case SMP_OP_PAIRING_FAIL:
        {
            //pairing fail event to tell upper layer
            blt_smp_procPairingEnd(connHandle, req->data[0]);  //pairing end with failure
        }
        break;


        case SMP_OP_SEC_REQ:  //SiHui has checked, code right, YaFei no need review here
        {
            if(smp_master_role){
            #if (SMP_SEC_LEVEL_CHECK_EN)
                /* After the connection is established, update smp_support according to the current security
                 * level configuration, 0: smp is not supported, 1: smp is supported, if it is detected that
                 * it is a bound device, smp_support is set to 1.*/
                if(blms_p_sts->support_smp)
            #else
                if(blms_p_prop->security_level != No_Authentication_No_Encryption)
            #endif
                {
                    blt_smpTrig.manual_smp_start = 0;  //clear, add by SiHui compared to old single master,
                                                       // to make sure "blt_smp_procCentralPairingRequest" not called all time

                    if(blt_smpTrig.smp_begin_flg){  //master has already send  pairing_req or enc_req
                        return NULL;
                    }

                    blt_smpTrig.smp_begin_flg = 1;
#if BROADCOM_WORKAROUND
                    if (1) {
                        my_dump_str_data(SMP_DBG_EN, "  re-connection, RF send LL_ENC Req", 0, 0);
                        blt_ll_startEncryption(connHandle, blms_p_peer->peer_ediv, blms_p_peer->peer_random, blms_p_peer->peer_ltk);
                    }
#else
                    if(smpMStblBondDevice.isBond_fastSmp & FlAG_BOND){//reconnection
                        u32 flash_addr = blc_smp_getBondingInfoCurStartAddr() + smpMStblBondDevice.master_bond_flash_idx[smpMStblBondDevice.master_curIndex];
                        flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, ediv),            2, (u8 *)&blms_p_peer->peer_ediv);
                        flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, random),          8, blms_p_peer->peer_random);
                        flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, local_peer_ltk), 16, blms_p_peer->peer_ltk);

                        tlkapi_send_string_data(CS_IOP_EN || (stkLog_mask & STK_LOG_SMP_LTK), "[SMP][LTK] ", blms_p_peer->peer_ltk, 16);

                        if( blms_p_peer->peer_ediv != U16_MAX && *(u32 *)&blms_p_peer->peer_random[0] != U32_MAX && *(u32 *)&blms_p_peer->peer_ltk[0] != U32_MAX){
                            smpMStblBondDevice.isBond_fastSmp |= FLAG_FASTSMP;
                        }
                    }

                    if(smpMStblBondDevice.isBond_fastSmp & FLAG_FASTSMP){ //reconnection, LL_ENC_REQ

                        my_dump_str_data(SMP_DBG_EN, "  re-connection, RF send LL_ENC Req", 0, 0);
                        blt_ll_startEncryption(connHandle, blms_p_peer->peer_ediv, blms_p_peer->peer_random, blms_p_peer->peer_ltk);

                        smpMStblBondDevice.keyIndex[conn_idx] = smpMStblBondDevice.addrIndex[conn_idx];
                    }
#endif
                    else{ //Send Pairing Request, init to new SMP pairing
                        //if it is a SecReq packet triggered by the Master device itself, rf_len must be 0.
                        if(req->rf_len == 0x06){ //SecReq packet sent by Slave device
                            smp_authReq_t authReq;
                            authReq.authType= req->data[0];

                            /* Refer to Core5.2 Spec | Vol 3, Part C page 1375
                             * A device may be in a Secure Connections Only mode. When in Secure Connections Only mode only security
                             * mode 1 level 4 shall be used except for services that only require security mode 1 level 1.
                             */
                            u8 level4only = ((blms_p_prop->security_level & LE_Security_Mode_1) == LE_Security_Mode_1_Level_4) ? 1 : 0;
                            //BQB testing needs to be handled as follows
                            if(!authReq.SC && level4only){
                                //if the local gap setting only support level4 only,we should response pairing failed, notify
                                //upper layer security level can not meet.
                                blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_AUTH_REQUIRE);  //pairing end with failure
                                return blt_smp_pushPairingFailed (PAIRING_FAIL_REASON_AUTH_REQUIRE);
                            }
                        }

                        blms_p_sts->smp_timeout_start_tick = clock_time()|1;//reset timeout timer

                        /*
                         * This parameter is set only if the 1st pairing(set to 1 if both M and S support SC).
                         * It should be 0 when it's connected back.
                         */
                        blms_p_own->sc_pairing = 0;

                        blms_p_own->stk_method = 0;
                        blms_p_own->peerKey_mask = 0;
                        blms_p_sts->tk_status = TK_ST_IDLE;  //TK status clear
                        blms_p_sts->bonding_enable = 0;
                        blms_p_sts->key_distribute = 0;

                        //The master roles share a set of parameters, and it is important to note the reuse situation here.
                        blms_p_peer->peer_ediv = 0;
                        memset(blms_p_peer->peer_random, 0, 8);

                        //re-init Key Distribution bits
                        /*
                         * if 1st time use SC, then unpaired, and 2nd time(do not re-power) use LG, key distribution bit will be Err.
                         */
                        blt_smp_setResponderKey(blms_p_own, 1, 1, 0); //TODO: when data signing OK later, add CSRK here
                        blt_smp_setInitiatorKey(blms_p_own, 1, 1, 0); //TODO: when data signing OK later, add CSRK here

                        my_dump_str_data(SMP_DBG_EN, "  1st pairing, RF send Pairing Req", 0, 0);
                        return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_REQ);
                    }
                }
            }
        }
        break;

        case SMP_OP_PAIRING_PUBLIC_KEY:
        case SMP_OP_PAIRING_DHKEY:
        case SMP_OP_KEYPRESS_NOTIFICATION:
        {
            if(blms_p_own->sc_pairing)
            {
                if(func_smp_sc_proc){
                    return func_smp_sc_proc(connHandle, p);
                }
            }
            else
            {
                //pairing fail event to tell upper layer
                blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_CMD_NOT_SUPPORT);  //pairing end with failure

                return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_CMD_NOT_SUPPORT);
            }
        }
        break;

        default:
            break;
    }


    return NULL;

smpCmdProcErr:
    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_UNSPECIFIED_REASON);  //pairing end with failure
    return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_UNSPECIFIED_REASON);
}


































void blc_smp_smpParamInit(void)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(smp_prop_t)), smp);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(smp_param_own_t)), smp);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(smp_param_peer_t)), smp);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(smp_st_t)), smp);
#endif
#if (!SMP_PERIPHERAL_LEVEL_CFG_SEPARATE_EN)

#if (!SMP_SEC_LEVEL_CHECK_EN)
    u8 pairingEnable = 0;
    for(int i=0; i<(1+LL_MAX_ACL_PER_NUM); i++){
        if(blt_smpProp[i].security_level != No_Authentication_No_Encryption){
            pairingEnable = 1;
            break;
        }
    }

    if(pairingEnable)
#endif
    {
        func_smp_init = &blt_smp_setAddress;

        blt_llms_registerLtkReqEvtCb(bls_smp_llGetLtkReq); //register get LTK function in controller(user for slave)

        /////////////////////// multi-role SMP storage init /////////////////////////////////
        #if (SMP_DATABASE_INFO_SOURCE == SMP_INFO_STORAGE_IN_FLASH)
            blt_smp_initBondingInfoFromFlash(); //to get smpMStblBondDevice data from flash
        #else
            // smpMStblBondDevice data initialization from where your database stored
        #endif

        blt_smp_param_init();
    }

#else

    func_smp_init = &blt_smp_setAddress;

    blt_llms_registerLtkReqEvtCb(bls_smp_llGetLtkReq); //register get LTK function in controller(user for slave)

    /////////////////////// multi-role SMP storage init /////////////////////////////////
    #if (SMP_DATABASE_INFO_SOURCE == SMP_INFO_STORAGE_IN_FLASH)
        blt_smp_initBondingInfoFromFlash(); //to get smpMStblBondDevice data from flash
    #else
        // smpMStblBondDevice data initialization from where your database stored
    #endif

    blt_smp_param_init();
#endif
}

void blt_smp_encChangeEvt(u8 status, u16 connHandle, u8 enc_enable)
{
    u8 is_master = connHandle & BLM_CONN_HANDLE;
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    u8 slave_dev_idx = blt_gap_getSlaveDeviceIndex_by_connIdx(conn_idx);

    if(is_master && (status == HCI_ERR_PIN_KEY_MISSING || status == HCI_ERR_UNSUPPORTED_REMOTE_FEATURE)){
        // slave key missing, master shall clean smp information in flash, then terminate connection
        blc_ll_disconnect(connHandle, HCI_ERR_REMOTE_USER_TERM_CONN);

        #if (SMP_DATABASE_INFO_SOURCE == SMP_INFO_STORAGE_IN_FLASH)
            if(smpMStblBondDevice.keyIndex[conn_idx] < smpMStblBondDevice.master_cur_bondNum) //Automatically connect back with key matching
            {
                blt_smp_deleteBondingInfo_by_Index(is_master, slave_dev_idx, smpMStblBondDevice.keyIndex[conn_idx], true);
            }
        #endif
    }
    else if(status==BLE_SUCCESS && enc_enable){
        //do nothing
    }
}


u8 blc_smp_isWaitingToSetTK(u16 connHandle)
{
    u8 is_master = connHandle & BLM_CONN_HANDLE;
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    u8 smp_status_idx = (connHandle & BLM_CONN_HANDLE) ? 0: (conn_idx - LL_MAX_ACL_CEN_NUM + 1);
    smp_param_own_t *pSmpOwn = (smp_param_own_t *)&smp_param_own[smp_status_idx];

    if((is_master && pSmpOwn->stk_method == PK_Resp_Display_Init_Input) || \
      ((!is_master) && pSmpOwn->stk_method == PK_Init_Display_Resp_Input) || \
      pSmpOwn->stk_method == PK_BOTH_INPUT || pSmpOwn->stk_method == OOB_Authentication){
        if(!(smp_sts_param[conn_idx].tk_status & TK_ST_UPDATE)){
            if (smp_sts_param[conn_idx].tk_status & TK_ST_REQUEST){
                return 1;
            }
        }
    }

    return 0;
}


u8 blc_smp_isWaitingToCfmNumericComparison(u16 connHandle)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    u8 smp_status_idx = (connHandle & BLM_CONN_HANDLE) ? 0: (conn_idx - LL_MAX_ACL_CEN_NUM + 1);
    smp_param_own_t *pSmpOwn = (smp_param_own_t *)&smp_param_own[smp_status_idx];

    if(pSmpOwn->stk_method == Numeric_Comparison && \
       (smp_sts_param[conn_idx].tk_status & TK_ST_NUMERIC_COMPARE) && \
      (!(smp_sts_param[conn_idx].tk_status & TK_ST_NUMERIC_CHECK_YES)) && \
      (!(smp_sts_param[conn_idx].tk_status & TK_ST_NUMERIC_CHECK_NO))){
        return 1;
    }

    return 0;
}


u8  blc_smp_setTK_by_PasskeyEntry(u16 connHandle, u32 pinCodeInput)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    u8 smp_status_idx = (connHandle & BLM_CONN_HANDLE) ? 0: (conn_idx - LL_MAX_ACL_CEN_NUM + 1);
    smp_param_own_t *pSmpOwn = (smp_param_own_t *)&smp_param_own[smp_status_idx];

    if(smp_sts_param[conn_idx].tk_status & TK_ST_REQUEST){
        memset(pSmpOwn->pairing_tk, 0, 16);
        if(pinCodeInput <= 999999){ //0~999999
            smemcpy(pSmpOwn->pairing_tk, &pinCodeInput, 4);
            smp_sts_param[conn_idx].tk_status |= TK_ST_UPDATE;

            return 1;
        }
    }

    return 0;
}


u8  blc_smp_setTK_by_OOB(u16 connHandle, u8 *oobData)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    u8 smp_status_idx = (connHandle & BLM_CONN_HANDLE) ? 0: (conn_idx - LL_MAX_ACL_CEN_NUM + 1);
    smp_param_own_t *pSmpOwn = (smp_param_own_t *)&smp_param_own[smp_status_idx];

    if(smp_sts_param[conn_idx].tk_status & TK_ST_REQUEST){
        smemcpy(pSmpOwn->pairing_tk, oobData, 16);
        smp_sts_param[conn_idx].tk_status |= TK_ST_UPDATE;
        return 1;
    }

    return 0;
}


void   blc_smp_setNumericComparisonResult(u16 connHandle, bool YES_or_NO)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    if(smp_sts_param[conn_idx].tk_status & TK_ST_NUMERIC_COMPARE){
        if(YES_or_NO){
            smp_sts_param[conn_idx].tk_status |= TK_ST_NUMERIC_CHECK_YES;
        }
        else{
            smp_sts_param[conn_idx].tk_status |= TK_ST_NUMERIC_CHECK_NO;
        }
    }
}


void   blc_smp_setSignBitEnable(u16 connHandle, bool enable)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    u8 smp_status_idx = (connHandle & BLM_CONN_HANDLE) ? 0: (conn_idx - LL_MAX_ACL_CEN_NUM + 1);
    smp_param_own_t *pSmpOwn = (smp_param_own_t *)&smp_param_own[smp_status_idx];

    pSmpOwn->sign_flag_en = enable;
}



/*
 * API used for set distribute key enable.
 * */
smp_keyDistribution_t blt_smp_setInitiatorKey(smp_param_own_t *pSmpOwn, u8 LTK_En, u8 IRK_En, u8 CSRK_En)
{
    smp_keyDistribution_t initKey ;
    initKey.keyIni = 0;
    initKey.encKey = LTK_En ? 1 : 0 ;
    initKey.idKey  = IRK_En ? 1 : 0 ;
    initKey.sign   = (CSRK_En||(pSmpOwn->sign_flag_en)) ? 1 : 0 ;

    pSmpOwn->pairing_req.initKeyDistribution.keyIni = initKey.keyIni;
    pSmpOwn->pairing_req.rspKeyDistribution.keyIni = initKey.keyIni;

    return initKey;
}

/*
 * API used for set distribute key enable.
 * */
smp_keyDistribution_t blt_smp_setResponderKey (smp_param_own_t *pSmpOwn, u8 LTK_En, u8 IRK_En, u8 CSRK_En)
{
    smp_keyDistribution_t rspKey ;
    rspKey.keyIni = 0;
    rspKey.encKey = LTK_En ? 1 : 0 ;
    rspKey.idKey  = IRK_En ? 1 : 0 ;
    rspKey.sign   = (CSRK_En||(pSmpOwn->sign_flag_en)) ? 1 : 0 ;

    pSmpOwn->pairing_rsp.initKeyDistribution.keyIni = rspKey.keyIni;
    pSmpOwn->pairing_rsp.rspKeyDistribution.keyIni = rspKey.keyIni;

    return rspKey;
}


/**
 *******************************************************************************
 * User API define
 *******************************************************************************
 */
//SMP parameter API method 1: all master and slave have same SMP configuration
void blc_smp_setSecurityLevel(le_security_mode_level_t  mode_level)
{
    for(int i=0; i<(1+LL_MAX_ACL_PER_NUM); i++){
        blt_smpProp[i].security_level = mode_level;
    }
}

//SMP parameter API method 2: all master have same SMP configuration
void blc_smp_setSecurityLevel_central(le_security_mode_level_t  mode_level)
{
    blt_smpProp[0].security_level = mode_level;
}

//SMP parameter API method 3: all slave have same SMP configuration
void blc_smp_setSecurityLevel_periphr (le_security_mode_level_t  mode_level)
{
    for(int i=1; i<(1+LL_MAX_ACL_PER_NUM); i++){
        blt_smpProp[i].security_level = mode_level;
    }
}

//SMP parameter API method 4: different slave device has different SMP configuration by device index
void blc_smp_setSecurityLevel_periphr_by_device_index(int slave_dev_ind, le_security_mode_level_t  mode_level)
{
    //do not need consider now
    blt_smpProp[slave_dev_ind+1].security_level = mode_level;
}

#if (SMP_SEC_LEVEL_CHECK_EN)
//SMP parameter API method 2: all master have same SMP configuration, after acl connected, it can be changed by user
le_security_mode_level_t blc_smp_getSecurityLevel_central(void)
{
    if(!blt_smpProp[0].security_level){
        return LE_Security_Mode_1_Level_1;
    }

    return blt_smpProp[0].security_level;
}

//SMP parameter API method 3: Notice: now all slave have same SMP configuration
le_security_mode_level_t blc_smp_getSecurityLevel_periphr(void)
{
    if(!blt_smpProp[1].security_level){
        return LE_Security_Mode_1_Level_1;
    }

    return blt_smpProp[1].security_level;
}

////SMP parameter API method 4: different slave device has different SMP configuration by device index
//le_security_mode_level_t blc_smp_getSecurityLevel_periphr_by_device_index(int slave_dev_ind)
//{
//  if(slave_dev_ind >= LL_MAX_ACL_PER_NUM){
//      return LE_Security_Mode_1_Level_1;
//  }
//  //do not need consider now
//  return blt_smpProp[slave_dev_ind+1].security_level;
//}
#endif

/*************************************************
 *  used for set SC flg (same as: Set Pairing Methods)
 * */

//SMP parameter API method 1: all master and slave have same SMP configuration
void blc_smp_setPairingMethods(pairing_methods_t  method)
{
    for(int i=0; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].pairing_method = method;
    }

    if (method == LE_Secure_Connection) // if support secure connection feature
    {
        func_smp_sc_proc         = blt_smp_sc_handler;
        func_smp_sc_pushPkt_proc = blt_smp_sc_pushPkt_handler;

        blt_ecc_init();
    }
}

//SMP parameter API method 2: all master have same SMP configuration
void blc_smp_setPairingMethods_central(pairing_methods_t  method)
{
    blt_smpProp[0].pairing_method = method;

    if (method == LE_Secure_Connection) // if support secure connection feature
    {
        func_smp_sc_proc         = blt_smp_sc_handler;
        func_smp_sc_pushPkt_proc = blt_smp_sc_pushPkt_handler;

        blt_ecc_init();
    }
}

//SMP parameter API method 3: all slave have same SMP configuration
void blc_smp_setPairingMethods_periphr(pairing_methods_t  method)
{
    for(int i=1; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].pairing_method = method;
    }

    if (method == LE_Secure_Connection) // if support secure connection feature
    {
        func_smp_sc_proc         = blt_smp_sc_handler;
        func_smp_sc_pushPkt_proc = blt_smp_sc_pushPkt_handler;

        blt_ecc_init();
    }
}

//SMP parameter API method 4: different slave device has different SMP configuration by device index
void blc_smp_setPairingMethods_periphr_by_device_index(int slave_dev_ind, pairing_methods_t  method)
{
    blt_smpProp[slave_dev_ind+1].pairing_method = method;

    if (method == LE_Secure_Connection) // if support secure connection feature
    {
        func_smp_sc_proc         = blt_smp_sc_handler;
        func_smp_sc_pushPkt_proc = blt_smp_sc_pushPkt_handler;

        blt_ecc_init();
    }
}


/*************************************************
 *  used for set IO capability
 * */
void blc_smp_setIoCapability(io_capability_t ioCapability)
{
    for(int i=0; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].IO_capability = ioCapability;
    }
}

void blc_smp_setIoCapability_central(io_capability_t ioCapability)
{
    blt_smpProp[0].IO_capability = ioCapability;
}

void blc_smp_setIoCapability_periphr(io_capability_t ioCapability)
{
    for(int i=1; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].IO_capability = ioCapability;
    }
}

void blc_smp_setIoCapability_periphr_by_device_index(int slave_dev_ind, io_capability_t ioCapability)
{
    blt_smpProp[slave_dev_ind+1].IO_capability = ioCapability;
}

/*************************************************
 *  used for set OOB flg
 * */
void blc_smp_enableOobAuthentication (int OOB_en)
{
    for(int i=0; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].oob_enable = OOB_en;
    }
}

void blc_smp_enableOobAuthentication_central (int OOB_en)
{
    blt_smpProp[0].oob_enable = OOB_en;
}

void blc_smp_enableOobAuthentication_periphr (int OOB_en)
{
    for(int i=1; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].oob_enable = OOB_en;
    }
}

void blc_smp_enableOobAuthentication_periphr_by_device_index(int slave_dev_ind, int OOB_en)
{
    blt_smpProp[slave_dev_ind+1].oob_enable = OOB_en;
}

/*************************************************
 *  used for set bonding mode flg
 * */
void blc_smp_setBondingMode(bonding_mode_t mode)
{
    for(int i=0; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].bonding_mode = mode;
    }
}

void blc_smp_setBondingMode_central(bonding_mode_t mode)
{
    blt_smpProp[0].bonding_mode = mode;
}

void blc_smp_setBondingMode_periphr(bonding_mode_t mode)
{
    for(int i=1; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].bonding_mode = mode;
    }
}
void blc_smp_setBondingMode_periphr_by_device_index(int slave_dev_ind, bonding_mode_t mode)
{
    blt_smpProp[slave_dev_ind+1].bonding_mode = mode;
}


/*************************************************
 *  used for set MITM flg
 * */
void blc_smp_enableAuthMITM (int MITM_en)
{
    for(int i=0; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].MITM_protection = MITM_en;
    }
}

void blc_smp_enableAuthMITM_central(int MITM_en)
{
    blt_smpProp[0].MITM_protection = MITM_en;
}

void blc_smp_enableAuthMITM_periphr(int MITM_en)
{
    for(int i=1; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].MITM_protection = MITM_en;
    }
}

void blc_smp_enableAuthMITM_periphr_by_device_index(int slave_dev_ind, int MITM_en)
{
    blt_smpProp[slave_dev_ind+1].MITM_protection = MITM_en;
}

/*************************************************
 *  used for enable keypress flag
 */
void blc_smp_enableKeypress(int keyPress_en)
{
    for(int i=0; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].keyPress_en = keyPress_en;
    }
}

void blc_smp_enableKeypress_central(int keyPress_en)
{
    blt_smpProp[0].keyPress_en = keyPress_en;
}

void blc_smp_enableKeypress_periphr(int keyPress_en)
{
    for(int i=1; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].keyPress_en = keyPress_en;
    }
}

void blc_smp_enableKeypress_periphr_by_device_index(int slave_dev_ind, int keyPress_en)
{
    blt_smpProp[slave_dev_ind+1].keyPress_en = keyPress_en;
}

/*************************************************
 *  used for ECDH debug mode select
 */
void blc_smp_setEcdhDebugMode(ecdh_keys_mode_t mode)
{
    for(int i=0; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].ecdh_debug_mode = mode;
    }
}

void blc_smp_setEcdhDebugMode_central(ecdh_keys_mode_t mode)
{
    blt_smpProp[0].ecdh_debug_mode = mode;
}

void blc_smp_setEcdhDebugMode_periphr(ecdh_keys_mode_t mode)
{
    for(int i=1; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].ecdh_debug_mode = mode;
    }
}

void blc_smp_setEcdhDebugMode_periphr_by_device_index(int slave_dev_ind, ecdh_keys_mode_t mode)
{
    blt_smpProp[slave_dev_ind+1].ecdh_debug_mode = mode;
}


/*************************************************
 *  When using the PasskeyEntry, set the default pincode
 *  displayed by our side and input by the other side.
 *  Note: If it is not the above method, setting the pincode is useless
 */
void blc_smp_setDefaultPinCode(u32 pinCodeInput)
{
    for(int i=0; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].passKeyEntryDftTK = pinCodeInput;
    }
}

void blc_smp_setDefaultPinCode_central(u32 pinCodeInput)
{
    blt_smpProp[0].passKeyEntryDftTK = pinCodeInput;
}

void blc_smp_setDefaultPinCode_periphr(u32 pinCodeInput)
{
    for(int i=1; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].passKeyEntryDftTK = pinCodeInput;
    }
}

void blc_smp_setDefaultPinCode_periphr_by_device_index(int slave_dev_ind, u32 pinCodeInput)
{
    blt_smpProp[slave_dev_ind+1].passKeyEntryDftTK = pinCodeInput;
}


int blt_smp_setSecurityParameters(smp_param_own_t *pSmpOwn, bonding_mode_t bond_mode,
                                 pairing_methods_t method, int MITM_en, int OOB_en,
                                 io_capability_t ioCapability,int keyPress_en)
{
/*
    u8 bondingFlag : 2;
    u8 MITM : 1;
    u8 SC   : 1;
    u8 keyPress: 1;
    u8 rsvd: 3;
*/
    u8 temp = bond_mode | MITM_en<<2 | method<<3 | keyPress_en<<4;
    pSmpOwn->auth_req.authType = temp;
    pSmpOwn->pairing_rsp.authReq.authType = temp;
    pSmpOwn->pairing_req.authReq.authType = temp;

    pSmpOwn->pairing_req.oobDataFlag = OOB_en;
    pSmpOwn->pairing_rsp.oobDataFlag = OOB_en;

    pSmpOwn->pairing_req.ioCapability = ioCapability;
    pSmpOwn->pairing_rsp.ioCapability = ioCapability;

    return 0;
}


/*************************************************
 *  used for SMP parameters settings
 */
void blc_smp_setSecurityParameters(bonding_mode_t mode, int MITM_en, pairing_methods_t method, int OOB_en,
                                     int keyPress_en, io_capability_t ioCapability)
{
    for(int i=0; i<(1+LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].IO_capability = ioCapability;
        blt_smpProp[i].oob_enable = OOB_en;
        blt_smpProp[i].bonding_mode = mode;
        blt_smpProp[i].MITM_protection = MITM_en;
        blt_smpProp[i].keyPress_en = keyPress_en;
        blt_smpProp[i].pairing_method = method;

    #if (SMP_SEC_LEVEL_CHECK_EN)
        smp_param_own_t *pSmpOwn = (smp_param_own_t *)&smp_param_own[i];
        // conFig all SMP parameters according to application's setting
        blt_smp_setSecurityParameters( pSmpOwn,
                                      blt_smpProp[i].bonding_mode, blt_smpProp[i].pairing_method, blt_smpProp[i].MITM_protection,
                                      blt_smpProp[i].oob_enable,   blt_smpProp[i].IO_capability, blt_smpProp[i].keyPress_en);
    #endif
    }

    if(method == LE_Secure_Connection) // if support secure connection feature
    {
        func_smp_sc_proc         = blt_smp_sc_handler;
        func_smp_sc_pushPkt_proc = blt_smp_sc_pushPkt_handler;

        blt_ecc_init();
    }
}

void blc_smp_setSecurityParameters_central(bonding_mode_t  bond_mode, int MITM_en, pairing_methods_t method, int OOB_en,
                                           int keyPress_en,io_capability_t ioCapability)
{
    blt_smpProp[0].IO_capability = ioCapability;
    blt_smpProp[0].oob_enable = OOB_en;
    blt_smpProp[0].bonding_mode = bond_mode;
    blt_smpProp[0].MITM_protection = MITM_en;
    blt_smpProp[0].keyPress_en = keyPress_en;
    blt_smpProp[0].pairing_method = method;

    if(method == LE_Secure_Connection) // if support secure connection feature
    {
        func_smp_sc_proc         = blt_smp_sc_handler;
        func_smp_sc_pushPkt_proc = blt_smp_sc_pushPkt_handler;

        blt_ecc_init();
    }

    #if (SMP_SEC_LEVEL_CHECK_EN)
        smp_param_own_t *pSmpOwn = (smp_param_own_t *)&smp_param_own[0];
        // conFig all SMP parameters according to application's setting
        blt_smp_setSecurityParameters( pSmpOwn,
                                      blt_smpProp[0].bonding_mode, blt_smpProp[0].pairing_method, blt_smpProp[0].MITM_protection,
                                      blt_smpProp[0].oob_enable,   blt_smpProp[0].IO_capability, blt_smpProp[0].keyPress_en);
    #endif
}

void blc_smp_setSecurityParameters_periphr(bonding_mode_t  bond_mode, int MITM_en, pairing_methods_t method, int OOB_en,
                                           int keyPress_en,io_capability_t ioCapability)
{
    for(int i=1; i<(1+LL_MAX_ACL_PER_NUM); i++){
        blt_smpProp[i].IO_capability = ioCapability;
        blt_smpProp[i].oob_enable = OOB_en;
        blt_smpProp[i].bonding_mode = bond_mode;
        blt_smpProp[i].MITM_protection = MITM_en;
        blt_smpProp[i].keyPress_en = keyPress_en;
        blt_smpProp[i].pairing_method = method;


        #if (SMP_SEC_LEVEL_CHECK_EN)
            smp_param_own_t *pSmpOwn = (smp_param_own_t *)&smp_param_own[i];
            // conFig all SMP parameters according to application's setting
            blt_smp_setSecurityParameters( pSmpOwn,
                                          blt_smpProp[i].bonding_mode, blt_smpProp[i].pairing_method, blt_smpProp[i].MITM_protection,
                                          blt_smpProp[i].oob_enable,   blt_smpProp[i].IO_capability, blt_smpProp[i].keyPress_en);
        #endif
    }

    if(method == LE_Secure_Connection) // if support secure connection feature
    {
        func_smp_sc_proc         = blt_smp_sc_handler;
        func_smp_sc_pushPkt_proc = blt_smp_sc_pushPkt_handler;

        blt_ecc_init();
    }
}

void blc_smp_setSecurityParameters_periphr_by_device_index(int slave_dev_ind, bonding_mode_t  bond_mode, int MITM_en, pairing_methods_t method,
                                                           int OOB_en, int keyPress_en,io_capability_t ioCapability)
{
    blt_smpProp[slave_dev_ind+1].IO_capability = ioCapability;
    blt_smpProp[slave_dev_ind+1].oob_enable = OOB_en;
    blt_smpProp[slave_dev_ind+1].bonding_mode = bond_mode;
    blt_smpProp[slave_dev_ind+1].MITM_protection = MITM_en;
    blt_smpProp[slave_dev_ind+1].keyPress_en = keyPress_en;
    blt_smpProp[slave_dev_ind+1].pairing_method = method;

    if(method == LE_Secure_Connection) // if support secure connection feature
    {
        func_smp_sc_proc         = blt_smp_sc_handler;
        func_smp_sc_pushPkt_proc = blt_smp_sc_pushPkt_handler;

        blt_ecc_init();
    }
}

/****************************************************************************************************
 *    API below is only available for BLE stack, user can not call !!!!!
 ***************************************************************************************************/
void blt_smp_setBondingMode(smp_param_own_t * pSmpOwn, bonding_mode_t mode)
{
    pSmpOwn->auth_req.bondingFlag = mode;
    pSmpOwn->pairing_rsp.authReq.bondingFlag = mode;
    pSmpOwn->pairing_req.authReq.bondingFlag = mode;
}

void blt_smp_setPairingMethods(smp_param_own_t * pSmpOwn, pairing_methods_t  method)
{
    pSmpOwn->auth_req.SC = method;
    pSmpOwn->pairing_rsp.authReq.SC = method;
    pSmpOwn->pairing_req.authReq.SC = method;
}

void blt_smp_enableOobAuthentication(smp_param_own_t * pSmpOwn, int OOB_en)
{
    pSmpOwn->pairing_req.oobDataFlag = OOB_en;
    pSmpOwn->pairing_rsp.oobDataFlag = OOB_en;
}


void blt_smp_enableAuthMITM(smp_param_own_t * pSmpOwn, int MITM_en)
{
    pSmpOwn->auth_req.MITM = MITM_en;
    pSmpOwn->pairing_rsp.authReq.MITM = MITM_en;
    pSmpOwn->pairing_req.authReq.MITM = MITM_en;
}

/*************************************************
 *  used for set IO capability
 * */
void blt_smp_setIoCapability(smp_param_own_t * pSmpOwn, io_capability_t ioCapability)
{
    pSmpOwn->pairing_req.ioCapability = ioCapability;
    pSmpOwn->pairing_rsp.ioCapability = ioCapability;
}

/*************************************************
 *  used for enable keypress flag
 */
void blt_smp_enableKeypress(smp_param_own_t * pSmpOwn, int keyPress_en)
{
    pSmpOwn->auth_req.keyPress = keyPress_en;

    pSmpOwn->pairing_rsp.authReq.keyPress = keyPress_en;
    pSmpOwn->pairing_req.authReq.keyPress = keyPress_en;
}



void blt_smp_param_pre_init(void)
{
    for(int i=ACL_CONN_IDX_CEN0; i<(1 + LL_MAX_ACL_PER_NUM); i++)
    {
        blt_smpProp[i].security_level = Unauthenticated_Pairing_with_Encryption;
        blt_smpProp[i].bonding_mode = Bondable_Mode;
        blt_smpProp[i].IO_capability = IO_CAPABILITY_NO_INPUT_NO_OUTPUT;
        blt_smpProp[i].ecdh_debug_mode = non_debug_mode;
        blt_smpProp[i].passKeyEntryDftTK = 0;

        //blt_smpProp[i].MITM_protection = 0;                   //default 0, can optimize, no need set
        //blt_smpProp[i].oob_enable = 0;                        //default 0, can optimize, no need set
        //blt_smpProp[i].pairing_method = LE_Legacy_Pairing;    //default 0, can optimize, no need set
        //blt_smpProp[i].keyPress_en = 0;                       //default 0, can optimize, no need set

    }

}


/*************************************************
 *  @brief      used for reset smp param to default value.
 */
int blt_smp_param_init (void)
{
    for(int i=0; i< (1 + LL_MAX_ACL_PER_NUM); i++ ){
        smp_param_own_t *pSmpOwn = (smp_param_own_t *)&smp_param_own[i];

        //memset (pSmpOwn->pairing_tk, 0, 16);  //default value is 0, no need do it to save code size
        pSmpOwn->pairing_req.code = SMP_OP_PAIRING_REQ;
        pSmpOwn->pairing_rsp.code = SMP_OP_PAIRING_RSP;

        pSmpOwn->pairing_req.maxEncrySize = 16;
        pSmpOwn->pairing_rsp.maxEncrySize = 16;

        blt_smp_setResponderKey(pSmpOwn, 1, 1, 0);  //@@@ to do: when data signing OK later, add CSRK here
        blt_smp_setInitiatorKey(pSmpOwn, 1, 1, 0);

        // conFig all SMP parameters according to application's setting
        blt_smp_setSecurityParameters( pSmpOwn,
                                      blt_smpProp[i].bonding_mode, blt_smpProp[i].pairing_method, blt_smpProp[i].MITM_protection,
                                      blt_smpProp[i].oob_enable,   blt_smpProp[i].IO_capability, blt_smpProp[i].keyPress_en);
    }
    return 0;
}

pairing_sts_t blt_smp_get_pairing_status(u16 connHandle)  //connHandle for future multiConnection, now use direct BLS_CONN_HANDLE is OK
{
    return smpMStblBondDevice.pairing_status[connHandle & CONN_IDX_MASK];
}


#if (SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY)

void     blc_smp_setLocalIrkGenerateStrategy(loc_irk_gen_str_t  str)
{
    smpMng.loc_irk_str = str;
}

#if (SMP_PERIPHERAL_LEVEL_CFG_SEPARATE_EN)

//attention: conflict with multiple local device function !!!
void blc_smp_setParametersConfigByConnHandleEnable(u8 enable)
{
    smpMng.dynamic_cfg_smp = enable;
}

void    blc_smp_registerConnectionCompleteCallBack(smp_conn_complete_callback_t cb)
{
    smp_conn_complete_cb = cb;
}


ble_sts_t blc_smp_setAclPeripheralPinCode(u16 connHandle, u32 pinCodeInput)
{

    u8 conn_idx = connHandle & CONN_IDX_MASK;
//  if((conn_idx < LL_MAX_ACL_CEN_NUM) &&(conn_idx >= LL_MAX_ACL_CONN_NUM))
//  {
//      return HCI_ERR_UNKNOWN_CONN_ID;
//  }
    tlkapi_send_string_data(SMP_DBG_EN, "blc_smp_setAclPeripheralPinCode pinCodeInput", &pinCodeInput, 4);
    blt_smpProp[conn_idx - LL_MAX_ACL_CEN_NUM + 1].passKeyEntryDftTK = pinCodeInput;
    tlkapi_send_string_data(SMP_DBG_EN, "blc_smp_setAclPeripheralPinCode conn_idx", &conn_idx, 1);
    return BLE_SUCCESS;
}


ble_sts_t blc_smp_setAclPeripheralParameters(u16 connHandle, smp_method_t expSmpMethod, bonding_mode_t bondable, bool keypress ,ecdh_keys_mode_t mode )
{
    if(BLE_SUCCESS != blt_ll_isAclhdlInvalid(connHandle)){
        tlkapi_send_string_data(SMP_DBG_EN, "connHandle invalid or ACL not connected", 0, 0);
        return SMP_ERR_INVALID_PARAMETER;
    }

    int smp_master_role = (connHandle & BLM_CONN_HANDLE);
    if(smp_master_role){
        tlkapi_send_string_data(SMP_DBG_EN, "Slave used only", 0, 0);
        return SMP_ERR_INVALID_PARAMETER;
    }


    u8 conn_idx = connHandle & CONN_IDX_MASK;
    u8 smp_prop_idx = (conn_idx - LL_MAX_ACL_CEN_NUM + 1);
#if (0)
    blt_smpProp[smp_prop_idx].security_level = mode_level;
    blt_smpProp[smp_prop_idx].ecdh_debug_mode = mode;
    blt_smpProp[smp_prop_idx].IO_capability = ioCapability;
    blt_smpProp[smp_prop_idx].oob_enable = OOB_en;
    blt_smpProp[smp_prop_idx].bonding_mode = bond_mode;
    blt_smpProp[smp_prop_idx].MITM_protection = MITM_en;
    blt_smpProp[smp_prop_idx].keyPress_en = keyPress_en;
    blt_smpProp[smp_prop_idx].pairing_method = method;

    if(method == LE_Secure_Connection) // if support secure connection feature
    {
        func_smp_sc_proc         = blt_smp_sc_handler;
        func_smp_sc_pushPkt_proc = blt_smp_sc_pushPkt_handler;

        blt_ecc_init();
    }

    blt_smp_param_init();
#else
    int MITM_en = 1;
    int OOB_en = 0;
    int SC_en = 0;
    int keyPress_en = 0;
    io_capability_t IO_capability = IO_CAPABILITY_NO_INPUT_NO_OUTPUT;
    le_security_mode_level_t secLevel = No_Authentication_No_Encryption;
    tlkapi_send_string_data(SMP_DBG_EN, "Expected pairing method", &expSmpMethod, 1);

    if(expSmpMethod == UNSPECIFIED){
        secLevel = LE_Security_Mode_1_Level_1;
    }
    if(expSmpMethod&(LESC_PKI|LESC_PKD|LESC_NC|LESC_OOB)){
        secLevel |= LE_Security_Mode_1_Level_4;
        if(blt_l2cap_getAclPeripheralRxBufferSize() < 65)
        {
            return SMP_ERR_SC_MTU_TOO_SHORT;
        }
    }
    if(expSmpMethod&(LEGACY_PKI|LEGACY_PKD|LEGACY_OOB)){
        secLevel |= LE_Security_Mode_1_Level_3;
    }
    if(expSmpMethod&(LEGACY_JW|LESC_JW)){
        secLevel |= LE_Security_Mode_1_Level_2;
    }

    //      LE_Security_Mode_1_Level_1
    //      LE_Security_Mode_1_Level_2
    //      LE_Security_Mode_1_Level_3
    //      LE_Security_Mode_1_Level_4
    //      LEGACY_JW   = BIT(1),       /* Legacy JustWorks */
    //      LESC_JW     = BIT(2),       /* LESC JustWorks */
    //      LEGACY_PKI  = BIT(3),       /* Legacy Passkey Entry input */
    //      LEGACY_PKD  = BIT(4),       /* Legacy Passkey Entry display */
    //      LESC_PKI    = BIT(5),       /* LESC Passkey Entry input */
    //      LESC_PKD    = BIT(6),       /* LESC Passkey Entry display */
    //      LESC_NC     = BIT(7),       /* LESC Numeric Comparison */
    //      LESC_OOB    = BIT(8),       /* LESC Out of Band */
    //      LEGACY_OOB  = BIT(9),       /* Legacy Out of Band */
    if (secLevel == No_Authentication_No_Encryption){
        blt_smpProp[smp_prop_idx].security_level = No_Authentication_No_Encryption;
        tlkapi_send_string_data(SMP_DBG_EN, "LE_Security_Mode_1_Level_1", 0, 0);
        return BLE_SUCCESS;
    }
    /* mode 1 level4 */
    if((secLevel & LE_Security_Mode_1_Level_4) == LE_Security_Mode_1_Level_4){
        tlkapi_send_string_data(SMP_DBG_EN, "   LE_Security_Mode_1_Level_4 check", 0, 0);

        keyPress_en = keypress;
        SC_en = 1;

        if(LESC_OOB & expSmpMethod){
            OOB_en = 1;
            keyPress_en = 0;
            //IO_capability = IO_CAPABILITY_NO_INPUT_NO_OUTPUT;
            tlkapi_send_string_data(SMP_DBG_EN, "IO_CAPABILITY_NO_INPUT_NO_OUTPUT", &IO_capability, 1);
        }
        else if(LESC_NC & expSmpMethod){
            keyPress_en = 0;
            IO_capability = IO_CAPABILITY_DISPLAY_YES_NO;
            tlkapi_send_string_data(SMP_DBG_EN, "IO_CAPABILITY_DISPLAY_YES_NO", &IO_capability, 1);
        }
        else if(LESC_PKD & expSmpMethod){
            IO_capability = IO_CAPABILITY_DISPLAY_ONLY;
            tlkapi_send_string_data(SMP_DBG_EN, "IO_CAPABILITY_DISPLAY_ONLY", &IO_capability, 1);
        }
        else if(LESC_PKI & expSmpMethod){
            IO_capability = IO_CAPABILITY_KEYBOARD_ONLY;
            tlkapi_send_string_data(SMP_DBG_EN, "IO_CAPABILITY_KEYBOARD_ONLY", &IO_capability, 1);
        }
    }// if((secLevel & LE_Security_Mode_1_Level_4) == LE_Security_Mode_1_Level_4){
    /* mode 1 level3 */
    else if((secLevel & LE_Security_Mode_1_Level_3) == LE_Security_Mode_1_Level_3){
        tlkapi_send_string_data(SMP_DBG_EN, "   LE_Security_Mode_1_Level_3 check", 0, 0);

        if(LEGACY_OOB & expSmpMethod){
            OOB_en = 1;
            //IO_capability = IO_CAPABILITY_NO_INPUT_NO_OUTPUT;
            tlkapi_send_string_data(SMP_DBG_EN, "IO_CAPABILITY_NO_INPUT_NO_OUTPUT", &IO_capability, 1);
        }
        else if(LEGACY_PKD & expSmpMethod){
            IO_capability = IO_CAPABILITY_DISPLAY_ONLY;
            tlkapi_send_string_data(SMP_DBG_EN, "IO_CAPABILITY_DISPLAY_ONLY", &IO_capability, 1);
        }
        else if(LEGACY_PKI & expSmpMethod){
            IO_capability = IO_CAPABILITY_KEYBOARD_ONLY;
            tlkapi_send_string_data(SMP_DBG_EN, "IO_CAPABILITY_KEYBOARD_ONLY", &IO_capability, 1);
        }
    } //    else if((secLevel & LE_Security_Mode_1_Level_3) == LE_Security_Mode_1_Level_3){
    /* mode 1 level2 */
    else if((secLevel & LE_Security_Mode_1_Level_2) == LE_Security_Mode_1_Level_2){
        tlkapi_send_string_data(SMP_DBG_EN, "   LE_Security_Mode_1_Level_2 check", 0, 0);

        //IO_capability = IO_CAPABILITY_NO_INPUT_NO_OUTPUT;
        MITM_en = 0;
        SC_en = (LESC_JW & expSmpMethod) ? 1 : 0;

        if(SC_en){
            tlkapi_send_string_data(SMP_DBG_EN, "SC JW", &IO_capability, 1);
        }
        else{
            tlkapi_send_string_data(SMP_DBG_EN, "LG JW", &IO_capability, 1);
        }
    } //else if((secLevel & LE_Security_Mode_1_Level_2) == LE_Security_Mode_1_Level_2){

    blt_smpProp[smp_prop_idx].security_level = secLevel;
    blt_smpProp[smp_prop_idx].ecdh_debug_mode = mode;
    blt_smpProp[smp_prop_idx].IO_capability = IO_capability;
    blt_smpProp[smp_prop_idx].oob_enable = OOB_en;
    blt_smpProp[smp_prop_idx].bonding_mode = bondable;
    blt_smpProp[smp_prop_idx].MITM_protection = MITM_en;
    blt_smpProp[smp_prop_idx].keyPress_en = keyPress_en;
    blt_smpProp[smp_prop_idx].pairing_method = SC_en;

    if(SC_en == LE_Secure_Connection) // if support secure connection feature
    {
        func_smp_sc_proc         = blt_smp_sc_handler;
        func_smp_sc_pushPkt_proc = blt_smp_sc_pushPkt_handler;

        blt_ecc_init();
    }

    //if exist, API: blc_smp_setSecurity can be called before or after API: blc_smp_peripheral_init
    //blt_smp_param_init()_begin

    smp_param_own_t *blms_p_own_s = (smp_param_own_t *)&smp_param_own[smp_prop_idx];

    //memset (blms_p_own->pairing_tk, 0, 16);  //default value is 0, no need do it to save code size
    blms_p_own_s->pairing_req.code = SMP_OP_PAIRING_REQ;
    blms_p_own_s->pairing_rsp.code = SMP_OP_PAIRING_RSP;

    blms_p_own_s->pairing_req.maxEncrySize = 16;
    blms_p_own_s->pairing_rsp.maxEncrySize = 16;

    blt_smp_setResponderKey(blms_p_own_s, 1, 1, 0);  //@@@ to do: when data signing OK later, add CSRK here
    blt_smp_setInitiatorKey(blms_p_own_s, 1, 1, 0);

    // conFig all SMP parameters according to application's setting
    blt_smp_setSecurityParameters( blms_p_own_s,
                                  blt_smpProp[smp_prop_idx].bonding_mode, blt_smpProp[smp_prop_idx].pairing_method, blt_smpProp[smp_prop_idx].MITM_protection,
                                  blt_smpProp[smp_prop_idx].oob_enable,   blt_smpProp[smp_prop_idx].IO_capability, blt_smpProp[smp_prop_idx].keyPress_en);
    //blt_smp_param_init()_end
#endif

    return BLE_SUCCESS;
}
#endif


#endif
