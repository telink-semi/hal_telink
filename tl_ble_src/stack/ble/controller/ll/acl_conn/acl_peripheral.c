/********************************************************************************************************
 * @file    acl_peripheral.c
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

#if (BLE_LLMIC_CONCURRENT_EN)
#include "stack/ble/controller/ll/llmic/llmic.h"
#include "stack/ble/controller/ll/llmic/llmic_internal.h"
#endif



#if (LL_ACL_PER_EN)







//only for slave
_attribute_ble_data_retention_  _attribute_aligned_(4)  st_lls_conn_t   blmsSlave[LL_MAX_ACL_PER_NUM];

_attribute_ble_data_retention_  st_lls_conn_t   *bls_pconn = NULL;
_attribute_ble_data_retention_  int             bls_conn_sel = 0;



_attribute_noinline_
void        blc_ll_initAclPeriphrRole_module(void)
{
    ll_adv_2_slave_cb   =   blt_s_connect;

    ll_acl_slave_irq_task_cb = blt_acl_slave_interrupt_task;
    ll_acl_slave_mlp_task_cb = blt_acl_slave_mainloop_task;

    blmsParam.acl_slave_en = 1;

    for(int i=0; i<LL_MAX_ACL_PER_NUM; i++){
        bls_pconn = (st_lls_conn_t *)&blmsSlave[i];

        bls_pconn->acl_slv_Index = i;

        for(int j=0; j<ACL_SLAVE_FIFONUM; j++){
            bls_pconn->aclTsk_fifo[j].scheTask_oft = TSKOFT_ACL_SLAVE + i;
            bls_pconn->aclTsk_fifo[j].scheTask_idx = LL_MAX_ACL_CEN_NUM + i;
            bls_pconn->aclTsk_fifo[j].scheTask_flg = TSKFLG_ACL_SLAVE;
        }
    }
}



#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
int blt_acl_slave_interrupt_task (int flag, void*p)
{
    int conn_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_SCHEDULE_START){
        #if (BLE_LLMIC_CONCURRENT_EN)
            if(bltLlmic.occupy_cur_task)
            {
                blt_llmic_quick_brx(conn_idx);
            }
            else
        #endif
            {
                blt_brx_start(conn_idx, p);
            }
    }
    else if(flag & FLAG_SCHEDULE_DONE){
        blt_brx_post();
    }
    else if(flag & FLAG_SCHEDULE_BUILD){
        #if (BLE_LLMIC_CONCURRENT_EN)
            blt_llmic_build_acl_slave_schedule();
        #else
            blt_ll_build_acl_slave_schedule();
        #endif
    }
#if BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE
    else if(flag & FLAG_ACL_SLAVE_CHECK_UPDATE_CMD_DEC){
        blt_acl_slave_slotgap_procUpdateReq();
    }
#endif
    else if(flag & FLAG_INSERT_SCHTSK_CONFLICT){
        #if(SCH_TASK_PRIORITY_IN_CB_EN)
            sch_task_t *pTgtTsk = (sch_task_t *)p;
            u8 curSchTaskOft = TSKOFT_ACL_CONN + conn_idx;

            s32 pri_taskCur = bltPri.pri_cal[curSchTaskOft];
            s32 pri_taskTra = bltPri.pri_cal[pTgtTsk->scheTask_oft];
             //priority higher than exist task, can insert target task
            if(pri_taskCur > pri_taskTra){
                return 1;
            }
            else{
            #if (LL_ASYNC_LEA_EN || ULL_FOR_CIS_EN) //CIS build task before ACL peripheral task
                st_ll_conn_t *pAclConn = (st_ll_conn_t*)&blms[conn_idx];
                if((1 && (aclConn_param.connSync & (1<<conn_idx))) || ((u32)(clock_time() - pAclConn->conn_tick) > ((pAclConn->conn_timeout*3)>>2)))
                {
                    //1. create ACL connection; 2. ACL connection monitoring timeout > ACL connection Timeout *3/4
                    return 1; //select ACL peripheral task, abandon other high priority task
                }
            #endif
            }
        #else
            return 0;
        #endif

    }
    return 0;
}






_attribute_noinline_
int blt_acl_slave_mainloop_task (int flag, void*p)
{
    int conn_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_MODULE_RESET){
        blt_ll_reset_acl_slave();
    }
    else if(flag & FLAG_ACL_SLAVE_CLEAR_SLEEP_LATENCY){
        blt_acl_slave_clear_sleep_latency(conn_idx);
    }
    else if(flag & FLAG_MODULE_SET_HOST_CHM){
        blt_ll_ctrlAclSlvChClassUpd((u8*)p);
    }

    return 0;
}




void  blt_ll_reset_acl_slave(void)
{
    #if (BLMS_PM_ENABLE)
        blmsPm.slave_idx_calib = 0xFF;
        blmsPm.slave_no_sleep = 0;
    #endif

    for(u8 slave_idx=0; slave_idx<LL_MAX_ACL_PER_NUM; slave_idx++){
        st_lls_conn_t *ps = (st_lls_conn_t*)&blmsSlave[slave_idx];
        (void)ps; //remove warning when BLMS_PM_ENABLE is 0

        #if (BLMS_PM_ENABLE)
            ps->slave_sleep_flg = 0;
            ps->latency_wakeup_flg = 0;
            ps->latency_available = 0;
            ps->sleep_sys_ms = 0;
            ps->sleep_32k_rc = 0;
        #endif
    }

}



ble_sts_t blc_ll_initAclPeriphrTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number, int conn_number)
{
    bltempParam.ll_aclTxSlvFifo_set = 1;

    /* Different process for different MCU: ******************************************/
    if( fifo_number == 9 ){
        blt_s_txfifo.depth = 3;
        blt_s_txfifo.real_num = 9;
        blt_s_txfifo.logic_num = 8;
        blt_s_txfifo.mask = 7;
    }
    else if( fifo_number == 17 ){
        blt_s_txfifo.depth = 4;
        blt_s_txfifo.real_num = 17;
        blt_s_txfifo.logic_num = 16;
        blt_s_txfifo.mask = 15;
    }
    else if( fifo_number == 33 ){
        blt_s_txfifo.depth = 5;
        blt_s_txfifo.real_num = 33;
        blt_s_txfifo.logic_num = 32;
        blt_s_txfifo.mask = 31;
    }
    else{
        //4, 2 is too small
        return LL_ERR_INVALID_PARAMETER;
    }



    /* size must be 16*n */
    if( (fifo_size & 15) == 0){
        blt_s_txfifo.size = fifo_size;
//      blt_s_txfifo.size_div_16 = fifo_size>>4;
    }
    else{
        return LL_ERR_INVALID_PARAMETER;
    }

    if(fifo_size<48){
        return LL_ERR_INVALID_PARAMETER;
    }
#if (MCU_HARDWARE_TX_FIFO_4K_BYTES_LIMITATION)  //only B91 have this limitation
    if((fifo_number-1)*fifo_size>=4096)
    {
        return LL_ERR_INVALID_PARAMETER;
    }
#endif


    blt_s_txfifo.conn_full_size = fifo_size * fifo_number;


    blt_s_txfifo.p_base = pTxbuf;


    for(int i=0; i<conn_number; i++){
        u8 *pBuff_Default = blt_s_txfifo.p_base + i * fifo_size * fifo_number;
        smemcpy( pBuff_Default, (const void *)blms_tx_empty_packet, 6);
    }


    for(int i=ACL_CONN_IDX_PER0; i<(ACL_CONN_IDX_PER0 + conn_number); i++){
        blms[i].max_fifo_num = fifo_number - 1;
    }

    /**********************************************************************************/

    return BLE_SUCCESS;
}




int blt_ll_ctrlAclSlvChClassUpd(unsigned char *pChm)
{
#if (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION)
    if(ll_acl_chnclass_mlp_task_cb){
        ll_acl_chnclass_mlp_task_cb(FLAG_MODULE_SET_HOST_CHM, pChm); //blt_chnclass_mainloop_task
    }
#else
    (void)pChm;
#endif

    return 1;
}


u8          blc_ll_getExtendedAdvHandleForAclConnection(u16 connHandle)
{
    if( (connHandle >= BLS_HANDLE_MIN) && (connHandle < (BLS_HANDLE_MAX_ADD_1)) ){ //Slave
        st_ll_conn_t* pAclConn = (st_ll_conn_t *)&blms[connHandle & CONN_IDX_MASK];
        return pAclConn->adv_handle;
    }
    else{ //Master
        return INVALID_ADVHD_FLAG;
    }
}




/* u8 is enough
 * Coded PHY use S8 to calculate, because peer device may use S8
 */
 #if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
__attribute__((section(".data"), aligned(4))) 
#else
const
#endif
u8 pdu_27b_sslot_num[4] = {0, PAYLOAD_27B_NOENT_1MPHY_SSLOT_NUM, PAYLOAD_27B_NOENT_2MPHY_SSLOT_NUM, PAYLOAD_27B_NOENT_CODED_S8_SSLOT_NUM};



_attribute_ram_code_ bool blt_s_connect (rf_packet_connect_t * pInit, bool aux_conn)
{

#if (LL_RSSI_SNIFFER_MASTER_ENABLE)
    if(ll_acl_sniffer_mst_irq_task_cb){
        return FALSE;
    }
#endif

#if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
    if(ll_acl_sniffer_slv_irq_task_cb){
        return FALSE;
    }
#endif


#if (PERIPHR_CONNECT_CENTRAL_MAC_FILTER_EN)
    if(filter_mac_enable && smemcmp(pInit->initA + 3, filter_mac_address + 3, 3) != 0 ){
        //my_dump_str_data(1, "@@@@@ mac reject", pInit->initA, BLE_ADDR_LEN);
        return FALSE;  //no connect
    }
    else{
        //my_dump_str_data(1, "@@@@@ mac accept", pInit->initA, BLE_ADDR_LEN);
    }
#endif


    //add the dispatch when the connect packets is meaning less
    //interval: 7.5ms - 4s -> 6 - 3200
    // 0<= latency <= (Timeout/interval) - 1, timeout*10/(interval*1.25)= timeout*8/interval
    // 0 <= winOffset <= interval
    // 1.25 <= winSize <= min(10ms, interval - 1.25)    10ms/1.25ms = 8
    //Timeout: 100ms-32s   -> 10- 3200     timeout >= (latency+1)*interval*2
    //hop :bit<0-5> ,range is 5 -16;
    if(    pInit->interval < 6 || pInit->interval > 3200    \
        || pInit->winSize < 1  || pInit->winSize > 8 || pInit->winSize>=pInit->interval \
        || pInit->timeout < 10 || pInit->timeout > 3200     \
    /*  || pInit->winOffset > pInit->interval               \ */
        || pInit->hop < 5 || pInit->hop > 16){

        return FALSE;
    }
    if( !pInit->chm[0]){
        if( !pInit->chm[1] && !pInit->chm[2] &&  \
            !pInit->chm[3] && !pInit->chm[4]){

            return FALSE;
        }
    }
    if(pInit->latency){
        if(pInit->latency  >  ((pInit->timeout<<3)/pInit->interval)){

            return FALSE;
        }
    }

    //when the connection number has reached the max supported value, CAN NOT create connection.
    if(blmsParam.cur_slave_num == blmsParam.max_slave_num){
        tlkapi_send_string_data(0, "ERROR, slave number error", 0, 0);
        return FALSE;
    }

#if (!MULTIPLE_LOCAL_DEVICE_ENABLE)
    //LL/CON/ADV/BI-02-C    [Reject Existing Connection Request]
    if(blt_ll_isRepeatedAclConnDevice(ACL_CONN_IDX_PER0, ACL_CONN_IDX_PER0 + blmsParam.max_slave_num)){
        return FALSE;
    }
#endif

    for (blms_conn_sel=ACL_CONN_IDX_PER0; blms_conn_sel < (ACL_CONN_IDX_PER0 + blmsParam.max_slave_num); blms_conn_sel++)
    {
        if (!blms[blms_conn_sel].connState)
        {
            break;
        }
    }


    blms_pconn =  (st_ll_conn_t *)   &blms[blms_conn_sel];

    bls_conn_sel = blms_conn_sel - LL_MAX_ACL_CEN_NUM; //pay attention here
    bls_pconn =  (st_lls_conn_t *)&blmsSlave[bls_conn_sel];

    blms_pconn->peer_chnSel = (aux_conn == TRUE) ? 1 : pInit->chan_sel;

    bls_pconn->errFlag = 0;


    /* peer packet address must set before "blms connect common" !!! */
    blms_pconn->conn_peerPktA_type = pInit->txAddr;
    smemcpy(blms_pconn->conn_peerPktA, pInit->initA, BLE_ADDR_LEN);

    blms_connect_common(blms_pconn, pInit, aux_conn);

    #if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)
        if(IS_EXTENDED_ADV_VALID){ //Extended ADV
            blms_pconn->adv_handle = blt_pextadv->adv_handle; //for ext adv set terminate event.
            blms_pconn->num_completeTerminateEvt = blt_pextadv->run_ext_adv_evt;
        }
        else{ //Legacy ADV
            blms_pconn->adv_handle = INVALID_ADVHD_FLAG;
        }
    #endif

    blms_pconn->bSlot_interval = pInit->interval<<1;
    bls_pconn->sSlot_interval = blms_pconn->bSlot_interval<<5;  //*32

    blms_pconn->conn_tick = blc_rcvd_connReq_tick;

    /**
     * The value of transmitWindowDelay shall be 1.25 ms when a CONNECT_IND PDU is used, 2.5 ms
     * when an AUX_CONNECT_REQ PDU is used on an LE Uncoded PHY, and 3.75 ms when an AUX_CONNECT_REQ PDU
     * is used on the LE Coded PHY.
     */
    //AUX_CONNECT_REQ => transmitWindowDelay: 2.5ms(unCode PHY), 3.75ms(Coded PHY)
    u8 extra_n_1m25 = 1;
    if(aux_conn){
        extra_n_1m25 = bltPHYs.cur_llPhy == BLE_PHY_CODED ? 3: 2;
    }
    bls_pconn->connExpectTime = (pInit->winOffset + extra_n_1m25) * SYSTEM_TIMER_TICK_1250US + blc_rcvd_connReq_tick;

#if LL_RSSI_SNIFFER_SLAVE_ENABLE
    blms_pconn->conn_offset_next = pInit->winOffset;
#endif

    #if (BLMS_PM_ENABLE)
        int tolerance_us = 0;
        blt_ll_set_slave_conn_interval_level(blms_pconn, bls_pconn, pInit->interval);

        bls_pconn->latency_available = 0;
        bls_pconn->latency_wakeup_flg = 0;
        bls_pconn->sleep_sys_ms = 0;
        bls_pconn->sleep_32k_rc = 0;

        u16 diff = pInit->interval - pInit->winSize;
        if(diff < 4){
            //0 : not allowed in SPEC
            //1 : 250       500
            //2:  500       1000
            //3:  750       1500
            tolerance_us = diff* 250;
            blmsPm.slave_no_sleep |= (1<<bls_conn_sel);
        }
        else{
            tolerance_us = 1000;
        }

        if(pInit->winOffset == 0 && diff > 2){
            tolerance_us = 500;
            blmsPm.slave_no_sleep |= (1<<bls_conn_sel);
        }

        bls_pconn->conn_tolerance_us = blmsPm.pm_inited ? tolerance_us : 0;
    #else
        bls_pconn->conn_tolerance_us = 0;
    #endif


    bls_pconn->conn_start_time = bls_pconn->connExpectTime - BRX_LEFT_EARLY_TICK - bls_pconn->conn_tolerance_us*SYSTEM_TIMER_TICK_1US;


    int n_sSlot = (bls_pconn->conn_start_time - bltSche.sSlot_tick_irq_real)*SSLOT_TICK_REVERSE;
    bls_pconn->sSlot_mark_conn = bltSche.sSlot_idx_irq_real + n_sSlot - bls_pconn->sSlot_interval;
    /* old code not calculate "bSlot mark conn" for ACL peripheral
     * but we found a problem 20221012 by JiaKai:
     * when central use extreme timing for first packet (winOffset 0, and TX packet on beginning of winSize),
     * only 1.25mS before conn_req and first packet. Here we software running may lead first BRX task can not allocate. And this situation is not
     * considered in initial code. Old variable "conn_inst_32" is used to skip early packet for a new connection, which will lead channel error.
     * To solve the problem, we should consider that even first several task may miss, so we should calculate interval jumping for all connection event.
     * So here "bSlot mark_conn" is necessary.
     * */
    blms_pconn->bSlot_mark_conn = bltSche.bSlot_idx_start + bls_pconn->sSlot_mark_conn/32;


    /* fix BQB testcase LL/TIM/PER/BV-03-C  [Latest Transmission Start to Peripheral], SiHui 20240220
     * if no "pdu_27b_sslot_num", LL/TIM/PER/BV-03-C will error, reason:
     * master send first packet at the end edge of window size, our code do not consider this.
     * BRX early timing is very tense, especially when fast settle used
     * fix method: add 27B payload length for different PHYs
     * here only consider RX_27B, but not "RX_27B + TIFS + TX_27B", because: 1.timing is tense; 2. RX packat received
     * can make sure timestamp captured(1st_rx_tick), the next interval task timing window can be shorted to normal value,
     * then local device can receive and send normal data packet. */
    /* tolerance*2/19.53 = tolerances/10 */
    blms_pconn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pInit->winSize*64 + bls_pconn->conn_tolerance_us/10 + pdu_27b_sslot_num[bltPHYs.cur_llPhy];
    /* task window can not too close to interval, leave 300uS margin, sSlot 15*19.53 = 293 uS */
    u32 sSlot_rf_max = bls_pconn->sSlot_interval - 15;
    if(blms_pconn->sSlot_allocNum > sSlot_rf_max){
        blms_pconn->sSlot_allocNum = sSlot_rf_max;
    }


    bls_pconn->sSlot_offset = 0; //clear when connect, no not clear when terminate

    blms_pconn->sync_timing = SLAVE_SYNC_CONN_CREATE;


    #if (DBG_PM_TIMING)
        if(tick1_exceed_tick2(bltSche.sSlot_tick_irq_real, bls_pconn->conn_start_time)){
            BLMS_ERR_DEBUG(DBG_PM_TIMING, 0x33330001);
        }
    #endif


    blmsParam.cur_slave_num ++;


#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
//  if(ll_acl_subrate_irq_task_cb){
//      ll_acl_subrate_irq_task_cb(FLAG_ACL_SUBRATE_CONN_CB, (void*)blms_pconn);
//  }

    blt_ll_initSubrateByHandle(blms_pconn->acl_conHandle);// must call this API, regardless of whether the subrate module init or not
#endif

    blt_sche_addUpdate(SLOT_UPDT_SLAVE_CONN_CREATE);
    blt_sche_addTaskMask(TSKMSK_ACL_CONN_0<<blms_conn_sel);

#if (LL_ASYNC_LEA_EN)
    if(asyncCtrl.leaUsed&&blt_pextadv->asyncAdvIndex)
    {
        blms_pconn->async_lea_link = blms_pconn->acl_conHandle | BLM_ASYNC_HANDLE;
    }
#endif

    return TRUE;
}









#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
int blt_brx_start (int conn_idx, void *p)
{
    (void)p; //unused, remove warning

//  DBG_TIANXIANG_CHN4_HIGH;
    blms_start_pre_process(conn_idx);

    bls_conn_sel = blms_conn_sel - LL_MAX_ACL_CEN_NUM; //pay attention here
    bls_pconn =  (st_lls_conn_t *)&blmsSlave[bls_conn_sel];


    #if (SL01_aclp_0)
        log_task_begin_irq(SL_STACK_ACL_BASIC_TIMING_EN, SL01_aclp_0 + bls_conn_sel);
    #endif

#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    blms_pconn->subrate_flag.bit.subrate_evt_flag= ((((sch_task_t*)p)->subrate_evt_flag))?1:0;
#endif


    blms_start_common_1(blms_pconn);

    if(1)
    {
    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
        my_dump_str_u32s(DBG_SUBRATE_EN, "brx start", blms_pconn->conn_inst, (u16)blms_pconn->subrate_flag.flagBits, (u32)p,0);
    #endif

        #ifdef HAL_CHIP_USE_CSEM_MODEM_IP
            /* Must be placed after PHY update processing is complete, because "conn cur_phy" is updated there */
            /* cost more time, can not set after "rf start_fsm" */
            #if(LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
                #if(HW_AES_CCM_ALG_EN)
                    #error "todo HW_AES_CCM_ALG_EN"
                #else
                    rf_ble_csem_set_tx_rx_settle(0, tx_stl_auto_mode[blms_pconn->connPhyCtrl.conn_cur_phy], RX_SETTLE_US);
                #endif
            #else
                rf_ble_csem_set_tx_rx_settle(0, TX_STL_AUTO_MODE_1M, RX_SETTLE_US);
            #endif
        #endif

        u8 *tx_buff = (u8*)(blt_s_txfifo.p_base + bls_conn_sel * blt_s_txfifo.conn_full_size);

        rf_start_fsm(FSM_BRX, tx_buff, clock_time());

        /* SiHui confirm with QiangKai 20230428: baseband digital setting can write after triggering FSM,
         * as long as writing quickly before old setting value take effect */
        if( aclConn_param.connSync & (1<<blms_conn_sel) ){
            rf_set_1st_rx_timeout(0xffffff);
        }
        else{
            rf_set_1st_rx_timeout(300 + bls_pconn->conn_tolerance_us*2 + bltPHYs.prmb_ac_us);
        }


        #ifndef HAL_CHIP_USE_CSEM_MODEM_IP
            /* Must be placed after PHY update processing is complete, because "conn cur_phy" is updated there */
            /* tx settle value take effect after RX OK for brx mode, so tx settle can be done anywhere */
            #if(LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
                #if(HW_AES_CCM_ALG_EN)
                //todo fanqh disable HW AES_CCM,TIFS= 149, if enable HW AES_CCM TIFS is 147
                    rf_ble_set_tx_settle(tx_stl_auto_mode[blms_pconn->connPhyCtrl.conn_cur_phy] +1);
                #else
                    rf_ble_set_tx_settle(tx_stl_auto_mode[blms_pconn->connPhyCtrl.conn_cur_phy] );
                #endif
            #else
                rf_ble_set_tx_settle(TX_STL_AUTO_MODE_1M);
            #endif
        #endif

        if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }

        #if 0  //debug, make BRX fail
            static int test_cnt = 0;
            test_cnt ++;
            if( (test_cnt & 3) == 0){
                rf_set_ble_access_code_value(0x12345678);
            }
        #endif
    }

    //these logic setting executing after BRX setting to save time
    blms_state = BLMS_STATE_BRX_S;
    systick_irq_trigger = SYS_IRQ_TRIG_BRX_POST;  //will set reg_system_tick_irq for brx_post immediately
    bls_pconn->timing_update = 0;
    bls_pconn->tick_1st_rx = 0;
    bls_pconn->sSlot_mark_conn = bls_pconn->sSlot_mark_brx = bltSche.sSlot_idx_irq_real;
    bls_pconn->sSlot_shift_tor = bls_pconn->conn_tolerance_us*SSLOT_US_REVERSE;
    bls_pconn->expectTimeMark = bls_pconn->connExpectTime = bltSche.sSlot_tick_irq + BRX_LEFT_EARLY_TICK + bls_pconn->conn_tolerance_us * SYSTEM_TIMER_TICK_1US;

    #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE || LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
        blms_pconn->ap_tick_mark = bls_pconn->connExpectTime;
    #endif


    #if (BLMS_PM_ENABLE)
        if(blmsPm.slave_idx_calib == 0xFF){
            blmsPm.slave_idx_calib = bls_conn_sel;
        }
    #endif
    blms_start_common_2(blms_pconn);

    return 0;
}





static inline void blt_acl_slave_latency_process(void)
{
#if (BLMS_PM_ENABLE && ACL_SLAVE_PM_LATENCY_EN)
    bls_pconn->latency_wakeup_flg = 0;
    bls_pconn->latency_available = 0;

    if(blms_pconn->conn_latency){
#if LL_FEATURE_ENABLE_CHANNEL_SOUNDING
        u8 no_latency = (blms_pconn->tx_wptr != blms_pconn->tx_rptr) || !bls_pconn->tick_1st_rx || blms_pconn->sync_timing || \
                                !blms_pconn->conn_receive_new_packet ||  blms_pconn->conn_termin_union.termin_pack || \
                                (blms_pconn->conn_update_union.update_cmd == 1 || blms_pconn->cs_pending||(blt_rxfifo.rptr != blt_rxfifo.wptr));
#else
        u8 no_latency = (blms_pconn->tx_wptr != blms_pconn->tx_rptr) || !bls_pconn->tick_1st_rx || blms_pconn->sync_timing || \
                                !blms_pconn->conn_receive_new_packet ||  blms_pconn->conn_termin_union.termin_pack || \
                                (blms_pconn->conn_update_union.update_cmd == 1);
#endif
        #if 0 //debug
            if(blms_pconn->tx_wptr != blms_pconn->tx_rptr || !bls_pconn->tick_1st_rx){

            }
            if(blms_pconn->sync_timing || !blms_pconn->conn_receive_new_packet){

            }
            if(blms_pconn->conn_termin_union.termin_pack || blms_pconn->conn_update_union.update_cmd == 1){

            }
        #endif

        u16 sys_latency = 0;
        u16 valid_latency = min(blms_pconn->conn_latency, blmsPm.user_latency);
        if(!no_latency){
            if( blms_pconn->conn_update_union.update_cmd){  //process conn_update/map_update/phy_update
                s16 rest_interval = blms_pconn->conn_inst_next - blms_pconn->conn_inst - 1;
                if(rest_interval > 0){
                    if(rest_interval < valid_latency){
                        sys_latency = rest_interval;
                    }
                }
                else{
                    no_latency = 1;
                }
            }
        }

        if(!no_latency){
            bls_pconn->latency_available = sys_latency ? sys_latency : valid_latency;
            bls_pconn->latency_wakeup_tick = bls_pconn->connExpectTime + bls_pconn->latency_available * blms_pconn->conn_intvl_tick;
        }
    }
    blmsPm.user_latency = 0xffff;
#endif
}


_attribute_ram_code_
int blt_brx_post(void)
{
    /* must execute before any other operation, cause may return to deal with boundary RX */
    if(blms_post_pre_process() == FALSE){
        return 1;
    }
//  DBG_TIANXIANG_CHN4_LOW;
    #if (BLMS_PM_ENABLE)
        int brx_sync = blms_pconn->sync_timing;
    #endif

    blms_state = BLMS_STATE_BRX_E;

    int result = blms_post_common_1(blms_pconn);
    if ( result==1 ){  // return 1: terminate happens

        #if (LEG_ADV_EN_MORE_STRATEGY)
            if(!blmsParam.legadv_en_strategy)
        #endif
            {
                if(blmsParam.cur_slave_num == blmsParam.max_slave_num){  //
                    if(blmsParam.leg_adv_en){
                        blt_sche_addTaskMask(TSKMSK_LEG_ADV);
                    }
                }
            }

        blmsParam.cur_slave_num --;

        blt_sche_removeTaskMask(TSKMSK_ACL_CONN_0<<blms_conn_sel);  //pay attention here
        blt_sche_addUpdate(SLOT_UPDT_CONN_TERMINATE);   //triggers "bltSlot.update" valid

        #if (BLMS_PM_ENABLE)
            blmsPm.slave_no_sleep &= ~(1<<bls_conn_sel);
            if(blmsPm.slave_idx_calib == bls_conn_sel){
                blmsPm.slave_idx_calib = 0xFF;
            }
        #endif

        #if (BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE)
            blms_pconn->conn_pkt_dec_pending = 0;
            aclConn_param.updateCmd_pending &= ~BIT(bls_conn_sel);
        #endif

        #if (HW_AES_CCM_ALG_EN)
            blms_pconn->hw_aes_ccm_flag = 0;
            reg_rf_tx_mode2 &= ~FLD_TLK_CRYPT_ENABLE;
        #endif

    }
    else{   //no terminate
        blt_ll_acl_conn_sync_process(bls_pconn->tick_1st_rx);

        blt_llms_update_fifo_sw();

        /* value below are invalid when old conn_interval change to new conn_interval */
        //bls_pconn->connExpectTime = (bls_pconn->tick_1st_rx ? bls_pconn->tick_1st_rx : blms_pconn->conn_tick_mark) + blms_pconn->conn_intvl_tick;
        if(bls_pconn->tick_1st_rx){
            bls_pconn->expectTimeMark = bls_pconn->tick_1st_rx;
            bls_pconn->conn_offset_tick = bls_pconn->connExpectTime - bls_pconn->tick_1st_rx;
            bls_pconn->connExpectTime = bls_pconn->tick_1st_rx + blms_pconn->conn_intvl_tick;
        }
        else{
            bls_pconn->connExpectTime += blms_pconn->conn_intvl_tick;
        }



        /* update sync status according to RX packet receiving result */
        if(bls_pconn->tick_1st_rx)
        {
            //for CIS slave timing build  OR PAST recipient timing calculation
            //Used to accurately obtain the starting anchor point of the data packet corresponding to the current CE
            bls_pconn->evtCnt_mark_1strx = (blms_pconn->conn_inst - 1);
            bls_pconn->tick_mark_1strx = bls_pconn->tick_1st_rx;
            bls_pconn->bSlot_mark_1strx = GET_BSLOT_IDX(bls_pconn->tick_mark_1strx);
            bls_pconn->offsetUs_mark1stRx = ((bls_pconn->tick_mark_1strx - bltSche.bSlot_tick_irq_real)/SYSTEM_TIMER_TICK_1US)%625;
            //my_dump_str_data(0, "offsetUs_mark1stRx", &bls_pconn->offsetUs_mark1stRx, 2);

            if(blms_pconn->sync_timing)
            {
                blms_pconn->sync_timing = 0;
                bls_pconn->conn_tolerance_us = blmsParam.min_tolerance_us;
                bls_pconn->conn_start_time = bls_pconn->connExpectTime - BRX_LEFT_EARLY_TICK - bls_pconn->conn_tolerance_us * SYSTEM_TIMER_TICK_1US;
                int n_sSlot = (bls_pconn->conn_start_time - bltSche.sSlot_tick_irq_real)*SSLOT_TICK_REVERSE;
                bls_pconn->sSlot_mark_conn = bltSche.sSlot_idx_irq_real + n_sSlot - bls_pconn->sSlot_interval;

                bls_pconn->sSlot_offset = 0;
                bls_pconn->timing_update = 1;
                blt_sche_addUpdate(SLOT_UPDT_SLAVE_SYNC_DONE);

                #if (BLMS_PM_ENABLE)
                    blmsPm.slave_no_sleep &= ~(1<<bls_conn_sel);
                #endif
            }
            else{
                if(bls_pconn->conn_tolerance_us > blmsParam.min_tolerance_us){
                    bls_pconn->conn_tolerance_us = blmsParam.min_tolerance_us;
                    bls_pconn->conn_start_time = bls_pconn->connExpectTime - BRX_LEFT_EARLY_TICK - bls_pconn->conn_tolerance_us * SYSTEM_TIMER_TICK_1US;
                    int n_sSlot = (bls_pconn->conn_start_time - bltSche.sSlot_tick_irq_real)*SSLOT_TICK_REVERSE;
                    bls_pconn->sSlot_mark_conn = bltSche.sSlot_idx_irq_real + n_sSlot - bls_pconn->sSlot_interval;

                    bls_pconn->sSlot_offset = 0;
                    bls_pconn->timing_update = 1;
                    blt_sche_addUpdate(SLOT_UPDT_SLAVE_SYNC_DONE);
                }
                else{
                    u32 tick_offset_1st_rx = bls_pconn->tick_1st_rx - bltSche.sSlot_tick_irq_real;
                    u32 tick_offset_expect = blmsParam.min_tolerance_us * SYSTEM_TIMER_TICK_1US + BRX_LEFT_EARLY_TICK;
                    bls_pconn->sSlot_offset = (signed int)(tick_offset_1st_rx - tick_offset_expect)*SSLOT_TICK_REVERSE;

                    #if (BRX_HALF_MARGIN_SSLOT_NUM == 3)
                        if(bls_pconn->sSlot_offset < -1 || bls_pconn->sSlot_offset > 2)
                    #elif (BRX_HALF_MARGIN_SSLOT_NUM == 2)
                        if(bls_pconn->sSlot_offset < 0 || bls_pconn->sSlot_offset > 1)
                    #else
                        #error "add code here"
                    #endif
                        {
                            bls_pconn->conn_tolerance_us = blmsParam.min_tolerance_us;
                            bls_pconn->timing_update = 1;
                            blt_sche_addUpdate(SLOT_UPDT_SLAVE_SSLOT_ADJUST);

                            //my_dump_str_u32s(0,"offset1", bls_pconn->tick_1st_rx, bltSche.sSlot_tick_irq_real, tick_offset_1st_rx, tick_offset_expect);
                            //my_dump_str_u32s(1,"offset2", bls_pconn->sSlot_offset, 0, 0, 0);
                        }
                }
            }

            if(bls_pconn->timing_update || blms_pconn->phy_chged){
                bls_pconn->sSlot_shift_tor = blmsParam.min_tolerance_us*SSLOT_US_REVERSE;
                /* tolerance*2/19.53 = tolerances/10 */
                #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
                    blms_pconn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[blms_pconn->connPhyCtrl.conn_cur_phy - 1][blms_pconn->crypt.enable] + bls_pconn->conn_tolerance_us/10;
                #else
                    blms_pconn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[0][blms_pconn->crypt.enable] + bls_pconn->conn_tolerance_us/10;
                #endif
                blms_pconn->phy_chged = 0;
            }
        }
        else
        {

            if(blms_pconn->sync_timing == SLAVE_SYNC_CONN_UPDATE && blms_pconn->sync_num >= BLMS_CONN_UPDATE_BRX_MAX_TRY_NUM){
                blms_pconn->sync_timing = SLAVE_SYNC_CONN_UPT_FAIL;
                //aclConn_param.connSync &= ~(1<<blms_conn_sel);  //fix: can not reset here, slave still have chance sync, duration shorter
                blms_pconn->conn_update_union.update_mark &= ~CONN_UPDATE_PARAM_MASK;
                blt_sche_addUpdate(SLOT_UPDT_SLAVE_CONNUPDATE_FAIL);
                blt_ll_setSchedulerTaskPriority(TSKOFT_ACL_CONN + blms_conn_sel, TASK_PRIORITY_LOW);

                /* 1.25mS/2.5mS: RX window not change, cover whole winSize
                 * others: RX begin at middle window - 1.25mS, RX end at  middle window + 0.625mS, default duration 1.875mS
                 * consider conn_window extend, RX end and duration will be better */
                if(blms_pconn->conn_winsize_next > 2){
                    blms_pconn->bSlot_mark_conn += (blms_pconn->conn_winsize_next - 2);
                    bls_pconn->sSlot_mark_conn += (blms_pconn->conn_winsize_next*32 - 2*32);
                    blms_pconn->sSlot_allocNum = 3*32;  //1.875mS
                }

                #if (BLMS_PM_ENABLE)
                    blmsPm.slave_no_sleep &= ~(1<<bls_conn_sel);
                #endif
            }
        }

    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)

        if(ll_acl_subrate_irq_task_cb){

            if((blms_pconn->lastSubEventCnt>>14) ==0x03)
            {
                blms_pconn->subrate_flag.bit.subrate_wrap_flag = 1;
            }

            if(blms_pconn->conti_num>1){
                ll_acl_subrate_irq_task_cb(FLAG_ACL_SUBRATE_INSERT_CONTI_TASK, blms_pconn);//blt_ll_subrate_insertContiTask
            }

            my_dump_str_u32s(DBG_SUBRATE_EN, "brx_post", blms_pconn->conn_inst -1,blms_pconn->tx_wptr,blms_pconn->tx_rptr,blms_pconn->conti_num);
        }

    #endif

        #if (BLMS_PM_ENABLE)
            if(brx_sync){
                blt_brx_timing_init();
            }
            else{
                blt_brx_timing_update ();
            }

            bls_pconn->slave_sleep_flg = 0;
            bls_pconn->tick_last_1st_rx = bls_pconn->tick_1st_rx;
            blt_acl_slave_latency_process();    // slave latency process
        #endif
    }


    blms_post_common_2();

    #if (SL01_aclp_0)
        log_task_end_irq(SL_STACK_ACL_BASIC_TIMING_EN, SL01_aclp_0 + bls_conn_sel);
    #endif



    return 1;
}




_attribute_noinline_
int blt_acl_slave_clear_sleep_latency (u8 conn_idx)
{
#if (BLMS_PM_ENABLE)
    u32 r = irq_disable();  //must disable IRQ here

    st_lls_conn_t* ps = (st_lls_conn_t*)&blmsSlave[conn_idx - LL_MAX_ACL_CEN_NUM];

    if(ps->latency_available && blmsPm.wkpTsk_oft != WKPTASK_INVALID){

        st_ll_conn_t * pc = (st_ll_conn_t*)&blms[conn_idx];

        int jump =0;
        /* 3500uS consider:
         * long sleep max 3S, 1000ppm, 3000uS, add 500uS margin
         * min 7.5mS, 3500uS < half it
         */
        u32 t_next = clock_time() + 3500*SYSTEM_TIMER_TICK_1US;
        u32 last_expect_tick = ps->connExpectTime - pc->conn_intvl_tick;
        if(tick1_exceed_tick2(last_expect_tick, t_next)){
            BLMS_ERR_DEBUG(DBG_PM_LOGIC, 0x33310000);
        }

        jump = (t_next - last_expect_tick)/pc->conn_intvl_tick + 1;
        u32 new_expect_tick = last_expect_tick + jump*pc->conn_intvl_tick;

        if(tick1_exceed_tick2(blmsPm.wkpTsk_tick, new_expect_tick + 5000*SYSTEM_TIMER_TICK_1US)){

            blmsPm.wkpTsk_fifo.scheTask_oft = TSKOFT_ACL_SLAVE + ps->acl_slv_Index;
            blmsPm.wkpTsk_fifo.scheTask_idx = LL_MAX_ACL_CEN_NUM + ps->acl_slv_Index;
            blmsPm.wkpTsk_fifo.scheTask_flg = TSKFLG_ACL_SLAVE;
            //blmsPm.wkpTsk_fifo.taskFifo_idx = 0;  //not used now

            ps->conn_tolerance_us = pc->pm_error_us + (new_expect_tick - clock_time())*ps->ppm_idx/(10*SYSTEM_TIMER_TICK_1MS);
            if(ps->conn_tolerance_us > ps->tolerance_max_us){
                ps->conn_tolerance_us = ps->tolerance_max_us;
            }
            u32 acl_start_time = new_expect_tick - BRX_LEFT_EARLY_TICK - ps->conn_tolerance_us*SYSTEM_TIMER_TICK_1US;
            /* tor*2 /sSlot_unit = tor*2 /(625/32) = tor*64/625 */
            #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)

            pc->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[pc->connPhyCtrl.conn_cur_phy - 1][pc->crypt.enable] + ps->conn_tolerance_us*64/625;
           #else
           pc->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[0][pc->crypt.enable] + ps->conn_tolerance_us*64/625;
           #endif
            /* SiHui: here use scheduler process 15 small slot to simplify code, it's OK */
            pc->sSlot_sche_use = 15;   //give 19.5*15=292 uS
            pc->sSlot_duration = pc->sSlot_allocNum + pc->sSlot_sche_use;



            //ps->latency_wakeup_flg = 1;
            //ps->slave_sleep_flg = 1;
            blmsPm.wkpTsk_oft = TSKOFT_ACL_SLAVE + ps->acl_slv_Index;
            blmsPm.wkpTsk_tick = new_expect_tick;

            int n_sSlot = (acl_start_time - bltSche.sSlot_tick_irq_real)*SSLOT_TICK_REVERSE;
            blmsPm.wkpTsk_fifo.begin = bltSche.sSlot_idx_irq_real + n_sSlot;
            #if 0
                //blmsPm.wkpTsk_fifo.end = 0;  //no need set end
            #else
                /* ACL slave update use: slot_diff =  bltSche.pTask_cur->end + 1 - ps->conn_update_pre_sSlotIndex;
                 * if "end" not set, will error */
                blmsPm.wkpTsk_fifo.end = blmsPm.wkpTsk_fifo.begin + pc->sSlot_duration - 1;
            #endif
            //bltSche.pTask_next = &blmsPm.wkpTsk_fifo;  //no need set again
            #if 0
                //bug, sSlot not aligned, accumulate timing error little by little
                bltSche.sSlot_tick_irq = acl_start_time;
            #else
                bltSche.sSlot_tick_irq = bltSche.sSlot_tick_start + blmsPm.wkpTsk_fifo.begin*SSLOT_TICK_NUM;
            #endif
            //systick_irq_trigger = SYS_IRQ_TRIG_NEW_TASK; //no need set again

            /*During the connection process, the sleep cannot be immediately entered after the key awakens.
             *  You need to wait for an interval interaction to enter sleep normally,
             *  and there will be dozens of milliseconds without entering sleep.
             *  To solve this problem,update the next_task_tick time value here.*/
            blmsPm.next_task_tick = bltSche.sSlot_tick_irq;

            systimer_set_irq_capture(bltSche.sSlot_tick_irq);

        }
        else{
            //keep no change, wake_up task is nearby, this task maybe current ACL slave or other ACL slave/ADV
        }
    }


    ps->latency_available = 0;

    irq_restore(r);
#else
    (void)conn_idx;
#endif

    return 0;
}






#if (BLMS_PM_ENABLE)
_attribute_ram_code_
void blt_brx_timing_init(void)
{
    bls_pconn->brx_pkt_miss = 0;
    bls_pconn->brx_cal_synced = 0;
    bls_pconn->brx_cal_long_sleep_synced = 0;
    bls_pconn->ppm_idx = PPM_IDX_MAX;
    bls_pconn->ppm_cal_idx_last = PPM_IDX_SHORT_SLEEP_MIN;
}




_attribute_ram_code_
void blt_brx_timing_update(void)
{
    u8 cur_cal_synced = 0;
    u8 cur_cal_long_sleep_synced = 0;
    if(bls_pconn->tick_1st_rx)
    {
        bls_pconn->brx_pkt_miss = 0;
        #if (BLMS_PM_ENABLE)
        if(bls_pconn->slave_sleep_flg && bls_pconn->tick_last_1st_rx){

            int offset_cal_valid = pm_ble_get_latest_offset_cal_time() && !clock_time_exceed(pm_ble_get_latest_offset_cal_time(), 4000000);  //last cal_offset within 4 S
            if(offset_cal_valid){

                if(bls_pconn->sleep_sys_ms > 45){
                    cur_cal_synced = 1;

                    u8 ppm_dec = 0;
                    if (bls_pconn->brx_cal_synced < 6){
                        bls_pconn->brx_cal_synced ++;
                        if(bls_pconn->brx_cal_synced < 3){
                            ppm_dec = 1;
                        }
                        else{
                            ppm_dec = 2;
                        }
                    }
                    else{
                        ppm_dec = PPM_IDX_MAX;
                    }

                    s8 ppm_new = bls_pconn->ppm_idx - ppm_dec;
                    if(ppm_new < PPM_IDX_SHORT_SLEEP_MIN){
                        ppm_new = PPM_IDX_SHORT_SLEEP_MIN;
                    }
                    bls_pconn->ppm_idx = min(bls_pconn->ppm_idx, ppm_new);


                    if(bls_pconn->sleep_sys_ms > 495){  //for 500mS or above
                        cur_cal_long_sleep_synced = 1;

                        //65536*65536/10000 = 429496.7 = 26843 uS = 26mS
                        u32 tick_diff = bls_pconn->conn_offset_tick > 0 ? bls_pconn->conn_offset_tick : -bls_pconn->conn_offset_tick;
                        u32 ppm_cal_idx = tick_diff*10000/(bls_pconn->tick_1st_rx - bls_pconn->tick_last_1st_rx) + 1;
                        #if (DBG_PM_TIMING)
                            if(ppm_cal_idx > 20){   //2000 ppm
                                BLMS_ERR_DEBUG(DBG_PM_TIMING, 0x33320000 | (ppm_cal_idx & 0xffff));
                            }
                        #endif
                        bls_pconn->ppm_cal_idx_cur = ppm_cal_idx;

                        if (bls_pconn->brx_cal_long_sleep_synced < 3) {
                            bls_pconn->brx_cal_long_sleep_synced ++;
                        }

                        if(bls_pconn->ppm_cal_idx_cur > PPM_IDX_SHORT_SLEEP_MIN){
                            bls_pconn->ppm_idx = bls_pconn->ppm_cal_idx_cur;
                        }
                        else if(bls_pconn->brx_cal_long_sleep_synced > 1 && bls_pconn->ppm_idx <= PPM_IDX_SHORT_SLEEP_MIN){
                            bls_pconn->ppm_idx = max(bls_pconn->ppm_cal_idx_cur, bls_pconn->ppm_cal_idx_last);
                        }

                        bls_pconn->ppm_cal_idx_last = ppm_cal_idx;
                    }
                }
            }

            if(blmsPm.slave_idx_calib == bls_conn_sel){
                if(bls_pconn->sleep_sys_ms > 6){
                    pm_ble_cal_32k_rc_offset(bls_pconn->conn_offset_tick, bls_pconn->sleep_32k_rc);
                }
            }
        }
        #endif

        if(!cur_cal_synced){
            bls_pconn->brx_cal_synced = 0;
            if(bls_pconn->ppm_idx < PPM_IDX_SHORT_SLEEP_MIN){
                bls_pconn->ppm_idx ++;
            }
        }

        if(!cur_cal_long_sleep_synced){
            bls_pconn->brx_cal_long_sleep_synced = 0;
        }


        blms_pconn->pm_error_us = 0;
        bls_pconn->sleep_sys_ms = 0;
        bls_pconn->sleep_32k_rc = 0;
    }
    else
    {
        if (bls_pconn->brx_pkt_miss < 5){
            bls_pconn->brx_pkt_miss ++;

            /* add 100 ppm, then add 200 ppm */
            if(bls_pconn->brx_pkt_miss < 3){
                bls_pconn->ppm_idx += 1;
            }
            else{
                bls_pconn->ppm_idx += 2;
            }
        }
        else{
            bls_pconn->ppm_idx = PPM_IDX_MAX;
        }

        bls_pconn->brx_cal_synced = 0;
        bls_pconn->brx_cal_long_sleep_synced = 0;
    }



    if(bls_pconn->ppm_idx < PPM_IDX_LONG_SLEEP_MIN){
        bls_pconn->ppm_idx = PPM_IDX_LONG_SLEEP_MIN;
    }
    else if(bls_pconn->ppm_idx > PPM_IDX_MAX){
        bls_pconn->ppm_idx = PPM_IDX_MAX;
    }

}
#endif


_attribute_ram_code_
int blt_ll_build_acl_slave_schedule(void)
{
    u32 i,j;

    st_ll_conn_t        *cur_pAclConn;
    st_lls_conn_t       *cur_pAclSlave;
    int int_jump_acl;
    s32 sSlot_start_conn;
#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    u16 inst_start_conn;
#endif

    int slave_task_number = 0;

    for(i=ACL_CONN_IDX_PER0; i<LL_MAX_ACL_CONN_NUM; i++)
    {
        if( bltSche.task_mask & (TSKMSK_ACL_CONN_0<<i) )
        {
            cur_pAclConn  = (st_ll_conn_t *)&blms[i];
            cur_pAclSlave = (st_lls_conn_t *)&blmsSlave[i-LL_MAX_ACL_CEN_NUM];
            cur_pAclSlave->aclTsk_wptr = cur_pAclSlave->aclTsk_rptr = 0;

            #if (BLMS_PM_ENABLE)
                int sSlot_mark_update = 0;
            #endif

            if(bltSche.build_index == 0){
                if(bltSche.sSlot_idx_reset == 1){
                    cur_pAclSlave->sSlot_mark_conn -= bltSche.sSlot_idx_past;
                    cur_pAclSlave->sSlot_mark_brx -= bltSche.sSlot_idx_past;
                }

                if(!cur_pAclConn->sync_timing){
                    if(cur_pAclSlave->sSlot_offset){
                        cur_pAclSlave->sSlot_mark_conn += cur_pAclSlave->sSlot_offset;
                        if(cur_pAclConn->conn_update_union.update_mark & CONN_UPDATE_PARAM_MASK){
                            cur_pAclSlave->conn_update_pre_sSlotIndex += cur_pAclSlave->sSlot_offset;  //TODO: test
                        }
                        cur_pAclSlave->sSlot_offset = 0;
                    }
                    #if (BLMS_PM_ENABLE)
                    else{
                        if(cur_pAclConn->pm_error_us || cur_pAclSlave->conn_tolerance_us > blmsParam.min_tolerance_us){
                            cur_pAclSlave->conn_tolerance_us = cur_pAclConn->pm_error_us + blmsParam.min_tolerance_us;
                            sSlot_mark_update = 1;
                        }
                    }
                    #endif
                }
            }

            else
            {
                #if (BLMS_PM_ENABLE)
                    if(!cur_pAclConn->sync_timing){
                        cur_pAclSlave->conn_tolerance_us += blmsParam.min_tolerance_us;
                        sSlot_mark_update = 1;
                    }
                #endif
            }


            #if (BLMS_PM_ENABLE)
                if(sSlot_mark_update){
                    if(cur_pAclSlave->conn_tolerance_us > cur_pAclSlave->tolerance_max_us){
                        cur_pAclSlave->conn_tolerance_us = cur_pAclSlave->tolerance_max_us;
                    }

                    s32 sSlot_shift_new = cur_pAclSlave->conn_tolerance_us*SSLOT_US_REVERSE;
                    cur_pAclSlave->sSlot_mark_conn -= (sSlot_shift_new - cur_pAclSlave->sSlot_shift_tor);
                    cur_pAclSlave->sSlot_shift_tor = sSlot_shift_new;
                    #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
                    // tor*2 /sSlot_unit = tor*2 /(625/32) = tor*64/625
                    cur_pAclConn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[cur_pAclConn->connPhyCtrl.conn_cur_phy - 1][cur_pAclConn->crypt.enable] + cur_pAclSlave->conn_tolerance_us*64/625;
                    #else
                    // tor*2 /sSlot_unit = tor*2 /(625/32) = tor*64/625
                    cur_pAclConn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[0][cur_pAclConn->crypt.enable] + cur_pAclSlave->conn_tolerance_us*64/625;
                    #endif

                }
            #endif




            if( cur_pAclSlave->sSlot_mark_conn >= bltSche.sSlot_idx_next){//sSlot_mark_conn init in "blt_s_connect" may make this happen
                sSlot_start_conn = cur_pAclSlave->sSlot_mark_conn + cur_pAclSlave->sSlot_interval;
                int_jump_acl = 0;
            }
            else
            {
                int_jump_acl = (bltSche.sSlot_idx_next - 1 - cur_pAclSlave->sSlot_mark_conn)/cur_pAclSlave->sSlot_interval;

                sSlot_start_conn = cur_pAclSlave->sSlot_mark_conn + (int_jump_acl + 1)*cur_pAclSlave->sSlot_interval;
            }


        #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
            inst_start_conn = cur_pAclConn->conn_inst + int_jump_acl;// next conn_inst
        #endif

            if(sSlot_start_conn >= bltSche.sSlot_endIdx_dft){ //to save some time for big interval
                continue; //attention: can not use break !!!
            }

            #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
                u16 inst_jump =0;

                my_dump_str_u32s(0, "acl jump", int_jump_acl, inst_start_conn, inst_jump, cur_pAclConn->insertTsk);

                if((cur_pAclConn->factor>1) && (cur_pAclConn->insertTsk)){ // maintain insertTsk(continue event), whether the continueEvent have been jumped
                    inst_jump = (u16)(inst_start_conn - cur_pAclConn->noDataEvtStart);

                    if(inst_jump < cur_pAclConn->conti_num){
                        cur_pAclConn->insertTsk -= int_jump_acl;
                    }
                    else{
                        cur_pAclConn->insertTsk = 0;
                    }
                }
            #endif


            /* SiHui: consider update a new task add, so add some more time. here update may represent a task remove, neglect this
             * give another margin here */
            u32 scheduler_use_us = bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US;
            cur_pAclConn->sSlot_sche_use = scheduler_use_us * SSLOT_US_REVERSE;
            cur_pAclConn->sSlot_duration = cur_pAclConn->sSlot_allocNum + cur_pAclConn->sSlot_sche_use + ACL_CMD_DONE_MANUAL_TRIGGER_STIMER_DELAY_US * SSLOT_US_REVERSE;

            #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE)
                cur_pAclConn->actual_txrx_sche_us = BRX_RIGHT_USE_US + pdu_27b_tifs_27b_us[cur_pAclConn->connPhyCtrl.conn_cur_phy - 1][cur_pAclConn->crypt.enable] + scheduler_use_us; //984+scheduler_use_us

                if(cur_pAclConn->limit_txrx_sche_us && cur_pAclConn->limit_txrx_sche_us < cur_pAclConn->actual_txrx_sche_us){
                    //TODO: not consider low power for CIS
                    cur_pAclConn->sSlot_duration = BRX_LEFT_EARLY_SSLOT_NUM + cur_pAclConn->limit_txrx_sche_us * SSLOT_US_REVERSE + ACL_CMD_DONE_MANUAL_TRIGGER_STIMER_DELAY_US * SSLOT_US_REVERSE;
                }
            #endif




            int new_task_cnt = 0;

            #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
                u32 inst = inst_start_conn -1;

                u16 subrate_flag_bak;
                u16 baseEventBak ;
                u16 insertTaskBak ;
            #endif

            for(j=0;j<ACL_SLAVE_FIFONUM;j++){

                sch_task_t  *pCur_schTask = (sch_task_t *)&cur_pAclSlave->aclTsk_fifo[j];



            #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)

                subrate_flag_bak = cur_pAclConn->subrate_flag.flagBits;

                baseEventBak = cur_pAclConn->subrateBaseEvent;
                insertTaskBak = cur_pAclConn->insertTsk;

                cur_pAclConn->lastSubEventCnt = inst;
                my_dump_str_u32s(DBG_SUBRATE_EN, "Next Evt", inst, cur_pAclConn->subrateBaseEvent,inst_start_conn,0);
                inst = blt_ll_subrate_getNextEvent(cur_pAclConn, inst);

                pCur_schTask->begin = sSlot_start_conn + ((u16)((inst&0xffff)-inst_start_conn))*cur_pAclSlave->sSlot_interval;

            #else
                pCur_schTask->begin = sSlot_start_conn + j*cur_pAclSlave->sSlot_interval;

            #endif


                pCur_schTask->end = pCur_schTask->begin + cur_pAclConn->sSlot_duration - 1;
                pCur_schTask->cover_other = 0;




                if( pCur_schTask->begin >=  bltSche.sSlot_endIdx_dft){  //new task beyond correct range, finish
                    break;
                }
                else if(pCur_schTask->end < bltSche.sSlot_endIdx_dft){ //new task in correct range
                    cur_pAclSlave->aclTsk_wptr = j;
                    new_task_cnt ++;

                #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
                    pCur_schTask->subrate_evt_flag = inst&BIT(31)?1:0;

                    cur_pAclConn->lastSubEventCnt = inst&0xffff;

                    if(cur_pAclConn->subrate_flag.bit.conn_update_flag &&
                            (cur_pAclConn->lastSubEventCnt==(cur_pAclConn->conn_para_inst_next - cur_pAclConn->subrate_flag.bit.conn_update_flag+1)))
                    {
                        cur_pAclConn->subrate_flag.bit.conn_update_flag --;
                    }

                    my_dump_str_u32s(DBG_SUBRATE_EN, "insert success", inst, cur_pAclConn->subrate_flag.bit.conn_update_flag,pCur_schTask->subrate_evt_flag,cur_pAclConn->conn_para_inst_next);
                #endif

                    
                }
                else{ //new task across "sSlot_endIdx_dft"

                    //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                    if(bltPri.pri_cal[TSKOFT_ACL_CONN + i] > bltPri.priMax_value){
                        bltPri.priMax_value = bltPri.pri_cal[TSKOFT_ACL_CONN + i];
                        bltPri.priMax_index = TSKOFT_ACL_CONN + i;
                        bltSche.sSlot_endIdx_maxPri = pCur_schTask->begin;
                        my_dump_str_u32s(SCHE_TIMING_IMPROVE_DBG_EN,"across IDX salve", i, bltSche.sSlot_endIdx_dft, bltSche.sSlot_endIdx_maxPri, bltPri.priMax_value);
                    }

                    break;
                }

//              cur_pAclSlave->sSlot_mark_conn += j*pCur_schTask->begin;
            }

        #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
            if(j<ACL_SLAVE_FIFONUM)
            {
                cur_pAclConn->subrate_flag.flagBits = subrate_flag_bak;
                cur_pAclConn->subrateBaseEvent = baseEventBak;
                cur_pAclConn->insertTsk = insertTaskBak;

                my_dump_str_u32s(DBG_SUBRATE_EN, "bak", cur_pAclConn->conn_inst-1, cur_pAclConn->subrateBaseEvent,cur_pAclConn->insertTsk, cur_pAclConn->subrate_flag.flagBits);
            }
        #endif

            slave_task_number += new_task_cnt;
            if(new_task_cnt){
                int t = blt_ll_addTask2ExistLinklist( &cur_pAclSlave->aclTsk_fifo[0],cur_pAclSlave->aclTsk_wptr + 1);
                (void)t; //remove compiler warning
                my_dump_str_u32s(0, "addTsk", t, cur_pAclSlave->aclTsk_wptr + 1,0,0);
            }


        }
    }






    return slave_task_number;
}










#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
_attribute_ram_code_
#endif 
int blt_ll_set_slave_conn_interval_level (st_ll_conn_t *pc, st_lls_conn_t *ps, u16 conn_interval)
{
#if (BLMS_PM_ENABLE)
/***********************************************************************
                     tolerance_max_us       shift_margin
 6  ->  7.5   mS:       2   mS            5 bSlot: 3125 uS
 7  ->  8.75  mS:       2.5 mS            6 bSlot: 3750 uS
 8  -> 10     mS:       3   mS            7 bSlot: 4375 uS
 9  -> 11.25  mS:       3.5 mS            8 bSlot: 5000 uS
10  -> 12.5   mS:       4   mS            8 bSlot: 5000 uS
*************************************************************************/
    if(conn_interval < 10){     // < 12.5mS
        ps->tolerance_max_us = 500*(conn_interval - 2);
        pc->bSlot_shift_margin = conn_interval - 1;
    }
    else{
        ps->tolerance_max_us = 4000;
        pc->bSlot_shift_margin = 8;
    }
#else
    (void)pc;
    (void)ps;
    (void)conn_interval;
#endif


    return 1;
}








#if BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE


#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
void blt_acl_slave_rx_procUpdateReq(u8 *raw_pkt)
{
    u8 llid = raw_pkt[DMA_RFRX_OFFSET_HEADER] & 0x03;
    u8 rf_len = raw_pkt[DMA_RFRX_OFFSET_RFLEN];

    /* proc master update_req in irq
     * conn param update  rf_len = 12   encryption rf_len = 16
     * map update         rf_len = 8    encryption rf_len = 12
     * phy update         rf_len = 5    encryption rf_len = 9
     * LL_subrate_ind     rf_len = 11
     * attention: some other control cmd rf_len is 5, so must judge opcode after decryption
    */
    if( llid == 3 && (blms_pconn->crypt.enable ? (rf_len==9 || rf_len==12 || rf_len == 16 || rf_len==15)
                                               : (rf_len==5 || rf_len==8  || rf_len == 12 || rf_len==11)))
    {
        smemcpy(bls_pconn->blt_buff_conn, (u8*)(raw_pkt + DMA_RFRX_OFFSET_HEADER), 24);

        if(blms_pconn->crypt.enable)
        {
            blms_pconn->conn_pkt_rcvd_no = blms_pconn->conn_pkt_rcvd;
            blms_pconn->conn_pkt_dec_pending = bls_pconn->blt_buff_conn;
            aclConn_param.updateCmd_pending |= BIT(bls_conn_sel);
        }
        else
        {
            blt_ll_conn_chn_phy_update(blms_pconn, bls_pconn->blt_buff_conn);
        }
    }
}


#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
void blt_acl_slave_tx_procUpdateReq(void)
{

    /* stop more data no matter aes_dec busy or not
     * limit packet exchange when main_loop block */
    STOP_RF_STATE_MACHINE;

    /*
     * CSEM chip, The Tx IRQ signal appears before the last bit of the TX data packet
     * TX_PATH_DLY us (current version corresponds to 10us), add margin 5us. After the
     * delay, Make sure the TX data packet is sent completely over the air and then reset_baseband.
     */
    HAL_CSEM_IP_WAIT_TX_DONE;
    HAL_CSEM_IP_RESET_BASEBAND;


    if(!aes_enc_dec_busy){
        if(tick1_exceed_tick2(aclConn_param.task_end_tick - SLOT_PROCESS_MAX_TICK, clock_time() + AES_CCM_DEC_US*SYSTEM_TIMER_TICK_1US)){  //BOUNDARY_RX_RELOAD_TICK
            blt_ll_conn_chn_phy_update(blms_pconn, bls_pconn->blt_buff_conn);
            blms_pconn->conn_pkt_dec_pending = 0;
            aclConn_param.updateCmd_pending &= ~BIT(bls_conn_sel);
        }
    }

    /* force a brx_post nearby if no RF finish status */
    if( !(reg_rf_irq_status & BLMS_FLG_RF_CONN_DONE) ){
        systimer_set_irq_capture(clock_time () + 30*SYSTEM_TIMER_TICK_1US);
    }
}



#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
void blt_acl_slave_slotgap_procUpdateReq(void)
{
    for(int i=0;i<LL_MAX_ACL_PER_NUM;i++){
        if(aclConn_param.updateCmd_pending & BIT(i)){

            if(tick1_exceed_tick2(bltSche.sSlot_tick_irq, clock_time() + AES_CCM_DEC_US*SYSTEM_TIMER_TICK_1US)){
                st_lls_conn_t* ps = (st_lls_conn_t*)&blmsSlave[i];
                st_ll_conn_t * pc = (st_ll_conn_t*)&blms[i + LL_MAX_ACL_CEN_NUM];
                blt_ll_conn_chn_phy_update(pc, ps->blt_buff_conn);
                aclConn_param.updateCmd_pending &= ~BIT(i);
                pc->conn_pkt_dec_pending = 0;
            }
            else{
                break;
            }
        }
    }
}



#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
int blt_ll_conn_chn_phy_update(st_ll_conn_t* pc, u8 *pkt)
{
    int st = 0;

    if (pc->crypt.enable)
    {
#if (HW_AES_CCM_ALG_EN)
    if(!pc->hw_aes_ccm_flag)
#endif
    {
        //my_dump_str_data(DBG_SLAVE_CONN_UPDATE, "data 1", pkt, 16);
        u32 bak = pc->crypt.dec_pno;
        pc->crypt.dec_pno = pc->conn_pkt_rcvd_no;
        aes_enc_dec_busy = 1;
        st = aes_ll_ccm_decryption((llPhysChnPdu_t *)pkt, 1, CRYPT_NONCE_TYPE_ACL, &pc->crypt); //slave role only
        aes_enc_dec_busy = 0;
        pc->crypt.dec_pno = bak;

        if(st)  //decrypt err
        {
            pc->crypt.mic_fail = 1;
            return st;
        }
    }

#if (HW_AES_CCM_ALG_EN)
    else{
        pkt[1] -= 4;
    }
#endif

    }



    st_lls_conn_t* ps = (st_lls_conn_t*)&blmsSlave[pc->acl_conIndex - LL_MAX_ACL_CEN_NUM];

    rf_packet_ll_control_t *pll = (rf_packet_ll_control_t *)pkt;

    if(pll->opcode == LL_CONNECTION_UPDATE_REQ)
    {
        if(pll->rf_len != 12){
            ps->errFlag |= SLV_FLAG_LEN_ERR;
            ps->unknownType = LL_CONNECTION_UPDATE_REQ;
            return st;
        }


        rf_packet_connect_upd_req_t *pUpdate = (rf_packet_connect_upd_req_t *)pkt;
        s16 diff_inst = pUpdate->instant - pc->conn_inst;
        if(diff_inst > 0){

            if(!(pc->conn_update_union.update_mark & (CONN_UPDATE_CMD | CONN_UPDATE_PENDING | CONN_UPDATE_NEARBY))){
                #if (DBG_SLAVE_CONN_UPDATE)
                    u8 conn_idx = pc->acl_conIndex;
                    if(conn_idx == 4){
                        DBG_C HN4_HIGH;
                    }
                    else if(conn_idx==5){
                        DBG_C HN5_HIGH;
                    }
                    else if(conn_idx==6){
                        DBG_C HN6_HIGH;
                    }
                    else{
                        DBG_C HN7_HIGH;
                    }
                #endif

                pc->conn_para_inst_next = pUpdate->instant;

                pc->conn_winsize_next = pUpdate->winSize;
                pc->conn_offset_next   = pUpdate->winOffset;
                pc->conn_intvl_next_n_1m25 = pUpdate->interval;
                pc->conn_latency_next = pUpdate->latency;
                pc->conn_timeout_next = pUpdate->timeout;

                pc->conn_inst_next = pc->conn_para_inst_next;
                pc->conn_update_union.update_cmd = 1;  //for slave PM
                pc->conn_update_union.update_mark |= CONN_UPDATE_CMD; //set flag at last is more safer, consider IRQ problem
                u32 r = irq_disable();
                ps->latency_available = 0;
                irq_restore(r);

                //DBG_C HN6_TOGGLE;DBG_C HN6_TOGGLE;
            }
        }
        else{
            //terminate with reason: instant passed
            ps->errFlag |= SLV_FLAG_INSTANT_PASS;
        }
    }
    else if(pll->opcode ==LL_CHANNEL_MAP_REQ)
    {
        rf_packet_chm_upd_req_t * pReq = (rf_packet_chm_upd_req_t*)pkt;

        if(pll->rf_len != 8){
            ps->errFlag |= SLV_FLAG_LEN_ERR;
            ps->unknownType = LL_CHANNEL_MAP_REQ;
            return st;
        }

        pc->conn_map_inst_next = pReq->instant;
        s16 diff_inst = pc->conn_map_inst_next - pc->conn_inst;


        if(diff_inst > 0){
            if(!(pc->conn_update_union.update_mark & CONN_UPDATE_MAP)){
                smemcpy ( pc->nextChn.chmTbl, pReq->chm, 5);

            #if(LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
                if(pc->conn_chnsel){
                    csa2_calculateMapInfo(&pc->nextChn);
                }
                else
            #endif
                {
                    /* when calculate new channel map in BRX/BTX start, 70uS is used, so calculate table in advance */
                    blt_csa1_calculateChannelTable ( pReq->chm, pc->conn_chn_hop, pc->nextChn.rempChmTbl);
                }

                pc->conn_inst_next = pc->conn_map_inst_next;
                pc->conn_update_union.update_cmd = 1;  //for slave PM
                pc->conn_update_union.update_mark |= CONN_UPDATE_MAP;  //set flag at last is more safer, consider IRQ problem
                u32 r1 = irq_disable();
                ps->latency_available = 0;
                irq_restore(r1);

                #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE)
                    if(ll_cis_map_update_cb && pc->cisEstablish_msk){
                        u32 r2 = irq_disable();
                        u32 trigger_tick = pc->ap_tick_mark + (pc->conn_map_inst_next - pc->conn_inst_mark)*pc->conn_intvl_tick;
                        irq_restore(r2);
                        ll_cis_map_update_cb(trigger_tick, pc); // blt_cis_update_chn_map
                    }
                #endif


            }
        }
        else{
            //terminate with reason: instant passed
            ps->errFlag |= SLV_FLAG_INSTANT_PASS;
        }
    }
#if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
    else if(pll->opcode == LL_PHY_UPDATE_IND)
    {
        if(pll->rf_len != 5){
            ps->errFlag |= SLV_FLAG_LEN_ERR;
            ps->unknownType = LL_PHY_UPDATE_IND;
            return st;
        }

        rf_pkt_ll_phy_update_ind_t* pUpdt = (rf_pkt_ll_phy_update_ind_t *)pkt;
        pc->ll_rsp_timeout_tick = 0;
        u8 comm_phy = pUpdt->m_to_s_phy & pUpdt->s_to_m_phy;

        if(comm_phy == 0){
            if(pc->ll_upd_flag){
                pc->irq_event1_union.phy_update_evt = 1;
            }
            pc->ll_upd_flag = 0;
            pc->llcp_flag.bit.ll_phy_req_rcvd_flag = 0;
            return st;
        }

        pc->conn_phy_inst_next = pUpdt->instant0 | (pUpdt->instant1<<8);

        s16 diff_inst = pc->conn_phy_inst_next - pc->conn_inst;
        if(diff_inst > 0){
            if(!(pc->conn_update_union.update_mark & CONN_PHY_UPDATE_IND_CMD)){
                if(comm_phy & PHY_PREFER_1M){
                    pc->connPhyCtrl.conn_next_phy = BLE_PHY_1M;
                }
                else if(comm_phy & PHY_PREFER_2M){
                    pc->connPhyCtrl.conn_next_phy = BLE_PHY_2M;
                }
                else if(comm_phy & PHY_PREFER_CODED){
                    pc->connPhyCtrl.conn_next_phy = BLE_PHY_CODED;
                }
                else{  //no PHY Update
                    pc->connPhyCtrl.conn_next_phy = pc->connPhyCtrl.conn_cur_phy;
                }

                pc->conn_inst_next = pc->conn_phy_inst_next;
                pc->conn_update_union.update_cmd = 1;  //for slave PM
                pc->conn_update_union.update_mark |= CONN_PHY_UPDATE_IND_CMD;
                u32 r = irq_disable();
                ps->latency_available = 0;
                irq_restore(r);
            }
        }
        else{
            //terminate with reason: instant passed
            ps->errFlag |= SLV_FLAG_INSTANT_PASS;
        }
    }
#endif

#if(LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    else if(pll->opcode == LL_SUBRATE_IND){

        if(ll_acl_subrate_ctrl_handler){
            ll_acl_subrate_ctrl_handler(pc, LL_SUBRATE_IND, pkt); //blt_ll_subrate_control_pdu_process
        }
    }
#endif
    else{  //some other opcode rf_len 5/8/12, do not process

    }


    return st;
}

#endif



#else  //else of LL_ACL_PER_EN

void        blc_ll_initAclPeriphrRole_module(void)
{

}

ble_sts_t blc_ll_initAclPeriphrTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number, int conn_number)
{
    return HCI_ERR_CMD_DISALLOWED;
}
#endif   //end of LL_ACL_PER_EN
