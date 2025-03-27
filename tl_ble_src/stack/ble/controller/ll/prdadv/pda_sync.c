/********************************************************************************************************
 * @file    pda_sync.c
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


#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)

//Not need to distinguish different SID. create sync should not send again before established or failed.
//because create_sync_cacel command has not any parameter.
_attribute_aligned_(4)  ll_prdadv_sync_t        bltPdaSync;

#if (PDA_SYNC_TIMING_ADJUST_EN)
_attribute_aligned_(4)  pda_syncTiming_t        pda_sync_timingAdjust[TSKNUM_PDA_SYNC];
#endif


//_attribute_aligned_(4)    extadv_id_t             pdaList_tbl[PERDADV_LIST_SIZE]; //periodic ADV list table
_attribute_aligned_(4)  pda_list_t              pdaList_tbl[PERDADV_LIST_SIZE]; //periodic ADV list table

_attribute_aligned_(4)  pda_cache_t             pdaCache_tbl[PERDADV_CACHE_NUM];       //periodic ADV device table, overwrite old one if overflow

_attribute_aligned_(4)  st_pda_sync_t           pdAsync_tbl[TSKNUM_PDA_SYNC];
_attribute_aligned_(4)  st_pda_sync_t           *blt_pPdAsync = NULL;


#define                 PDA_TRUNCATED_EVT_SIZE              8
le_periodAdvReportEvt_t pdaScan_truncatedEvt[TSKNUM_PDA_SYNC];


//_attribute_aligned_(4)    st_pda_sync_t           *blt_pPdA_syncing = NULL;


#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
__attribute__((section(".data"))) 
#else
const
#endif
u16   max_err_us_in_1S[8] = {500, 250, 150, 100, 75, 50, 30, 20};


_attribute_noinline_
void        blc_ll_initPeriodicAdvertisingSynchronization_module(void)
{
    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_prdadv_sync_t)), pda_sync);
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(st_pda_sync_t)), pda_sync);
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(pda_list_t)), pda_sync);
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(pda_cache_t)), pda_sync);
    #endif

    blc_ll_init2MPhyCodedPhy_feature();
    blc_ll_initChannelSelectionAlgorithm_2_feature();
    ll_pda_sync_irq_task_cb = blt_pda_sync_interrupt_task;
    ll_pda_sync_mlp_task_cb = blt_pda_sync_mainloop_task;

    ll_pda_sync_pawr_sync_common_cb = blt_ll_pdaSync_pawrSync_info_process;

    bltPdaSync.pdaSync_timeout_us = 5000 * 1000;

    /* Secondary Channel Scan RX buffer initialize
     * Periodic ADV Sync module may enabled without Ext_Scan module enable, when Periodic Advertising Sync Transfer used.
     * So here we need also initialize secondary RX FIFO.
     */
//  scan_secRxFifo.p = (u8 *)scan_sec_chn_rx_fifo;
    scan_secRxFifo.num = SCAN_SECCHN_RXFIFO_NUM;
    scan_secRxFifo.mask = SCAN_SECCHN_RXFIFO_MASK;
    scan_secRxFifo.rptr = scan_secRxFifo.wptr = 0;
    bltExtScn.scan_rx_sec_chn_dma_buff = (u32)(scan_secRxFifo.p + (scan_secRxFifo.wptr & SCAN_SECCHN_RXFIFO_MASK) * SCAN_SECCHN_RXFIFO_SIZE);
    bltExtScn.scan_rx_sec_chn_dma_size = (SCAN_SECCHN_RXFIFO_SIZE>>4);

    blmsParam.pda_sync_en = 1;

    st_pda_sync_t *pPdA_sync;
    for(int i=0; i<TSKNUM_PDA_SYNC; i++){
        pPdA_sync = (st_pda_sync_t *)&pdAsync_tbl[i];
        pPdA_sync->pda_index = i;
        pPdA_sync->createSyncType = PDA_CREATE_SYNC_BY_HCICMD;
        pPdA_sync->sync_adv_dup_filter = 0xFFFF;

        for(int j=0; j<PRDADV_SYNC_FIFONUM; j++){
            pPdA_sync->schTsk_fifo[j].scheTask_oft = TSKOFT_PDA_SYNC + i;
            pPdA_sync->schTsk_fifo[j].scheTask_idx = i;
            pPdA_sync->schTsk_fifo[j].scheTask_flg = TSKFLG_PDA_SYNC;
        }

        //blt_ll_setSchedulerTaskPriority( TSKOFT_PDA_SYNC + i, TASK_PRIORITY_PDA_SYNCING_DFT );

        blt_ll_setSchedulerTaskPriority( TSKOFT_PDA_SYNC + i, TASK_PRIORITY_LOW );
    }

//  pda_cache_t *pPdA_cache;
    for (int i=0; i<PERDADV_CACHE_NUM; i++)
    {
//      pPdA_cache = (pda_cache_t *)&pdaCache_tbl[i];
//      pPdA_cache->cach_index = i;
        pdaCache_tbl[i].cach_index = i;
    }







    blt_reset_pda_sync();
}


_attribute_noinline_
void blt_reset_pda_sync(void)
{
    bltPdaSync.pdA_list_num = 0;

    blmsParam.pda_syncing_flg = 0;

    bltPdaSync.pdA_cacheNum = 0;
//  bltPdaSync.prA_dev_newest_idx = PERDADV_CACHE_NUM - 1;

    bltPdaSync.prdadv_seqnum = 0;
//  bltPdaSync.pdASync_customTmoExpire = 0;

    for (int i=0; i<TSKNUM_PDA_SYNC; i++){
        pdAsync_tbl[i].sync_state = SYNC_STATE_IDLE;
        pdAsync_tbl[i].createSyncType = PDA_CREATE_SYNC_BY_HCICMD;
        pdAsync_tbl[i].sync_adv_dup_filter = 0xFFFF;
        pdAsync_tbl[i].sync_lost = 0;
        pdAsync_tbl[i].terminate = 0;
        pdAsync_tbl[i].sync_establish = 0;
        pda_sync_timingAdjust[i].sSlot_offset = 0;
    }

    for (int i=0; i<PERDADV_CACHE_NUM; i++){
        pdaCache_tbl[i].cach_flag = CACHE_FLAG_IDLE;
        pdaCache_tbl[i].seq_number = 0;
    }
}










_attribute_noinline_
ble_sts_t   blc_ll_periodicAdvertisingCreateSync ( option_msk_t options, u8 adv_sid, u8 adv_adrType, u8 *adv_addr, u16 skip, sync_tm_t sync_timeout, u8 sync_cte_type)
{
    (void)sync_cte_type; //unused, remove warning

    //HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands]
    if(IS_LEGACY_SCAN_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_SCAN_VALID;


    u8 temp_buffer[sizeof(extadv_id_t)];
    extadv_id_t *cur_pAdv = (extadv_id_t *)temp_buffer;
    cur_pAdv->sid = adv_sid;
    cur_pAdv->adrType = adv_adrType;
    smemcpy(cur_pAdv->addr, adv_addr, BLE_ADDR_LEN);

    st_pda_sync_t *pPdA_sync = NULL;
    pda_cache_t   *pPdA_cache = NULL;

    int cache_info_exist = 0;
    int return_status = BLE_SUCCESS;




    u32 r = irq_disable(); //IRQ & MainLoop variables interact

    /* If the Host issues this command when another HCI_LE_Periodic_Advertising_Create_Sync command is pending (see page
        2625), the Controller shall return the error code Command Disallowed (0x0C).*/
    if(blmsParam.pda_syncing_flg){
        return_status = HCI_ERR_CMD_DISALLOWED;
    }
    /* If the Host issues this command and the Controller has insufficient resources to handle any more periodic advertising trains,
     * the Controller shall return the error code Memory Capacity Exceeded (0x07). */
    else if (bltPdaSync.pdA_synced_num >= TSKNUM_PDA_SYNC || blmsParam.new_conn_forbidden || bltScn.initiate_going)
    {
        return_status = HCI_ERR_MEM_CAP_EXCEEDED;
    }

    irq_restore(r); //IRQ & MainLoop variables interact

    if(return_status != BLE_SUCCESS){
        return return_status;
    }




    int sync_adv_from_list = (options & SYNC_ADV_FROM_LIST) ? 1 : 0;

    /* If bit 1 of the Options parameter is set to 1 and the Controller does not support the
     * HCI_LE_Set_Periodic_Advertising_Receive_Enable command, the Controller shall return the error code Connection Failed
     * to be Established Synchronization Timeout (0x3E). */
    if(0){
        return HCI_ERR_CONN_FAILED_TO_ESTABLISH;
    }

    int i,j;
    for(i=0; i<TSKNUM_PDA_SYNC; i++)  //use "LL_MAX_ACL_CEN_NUM" here
    {
        st_pda_sync_t * cur_p_pda_sync = (st_pda_sync_t *)&pdAsync_tbl[i];
        /* current device is in sync_ed state */
        if( cur_p_pda_sync->sync_state == SYNC_STATE_SYNCED){
            if(!sync_adv_from_list){
                cur_pAdv = (extadv_id_t *)temp_buffer;
                if(!smemcmp (&cur_p_pda_sync->pda_id, cur_pAdv, sizeof(extadv_id_t))){

                    /* If the Host issues this command with bit 0 of Options not set and with Advertising_SID, Advertising_Address_Type,
                     * and Advertiser_Address the same as those of a periodic advertising train that the Controller is already synchronized to,
                     * the Controller shall return the error code Connection Already Exists (0x0B). */
                    return HCI_ERR_CONN_ALREADY_EXISTS;
                    //break;
                }
            }
        }
        else if(cur_p_pda_sync->sync_state == SYNC_STATE_IDLE){
            if(!pPdA_sync){
                pPdA_sync = cur_p_pda_sync;
            }
        }
    }

    if(!pPdA_sync){
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }

#if (LL_FEATURE_ENABLE_LE_AOA_AOD)
    pPdA_sync->create_sync_cte_type = sync_cte_type;
    pPdA_sync->sync_wrong_cte_type = 0;
    if((sync_cte_type & NOT_SYNC_ALL_PDA) == NOT_SYNC_ALL_PDA){
        return HCI_ERR_CMD_DISALLOWED;
    }
#endif

    if(sync_adv_from_list){
        int pda_list_dev_synced_num = 0;
        for(i=0;i<bltPdaSync.pdA_list_num;i++){
            if(pdaList_tbl[i].synced_mark){
                pda_list_dev_synced_num ++;
            }
        }
        /* all device is sync_ed */
        if(pda_list_dev_synced_num >= bltPdaSync.pdA_list_num){
            return HCI_ERR_CONN_ALREADY_EXISTS;
        }
    }



    pda_cache_t pda_cache;

    int end_idx = sync_adv_from_list ? bltPdaSync.pdA_list_num : 1;
    for(i=0; i<end_idx; i++)
    {
        if(sync_adv_from_list){
            cur_pAdv = (extadv_id_t *)&pdaList_tbl[i];
        }
        else{           
            cur_pAdv = (extadv_id_t *)temp_buffer;
        }


        for(j=0;j<PERDADV_CACHE_NUM;j++)
        {
            u32 r1 = irq_disable();

            pda_cache = pdaCache_tbl[j];

            irq_restore(r1);

            pPdA_cache = &pda_cache;

            //if host terminate sync SID=1, then create sync SID=1. terminate only set cach_flag to CACHE_FLAG_IDLE to indicate invalid.
            //but all cache information are still in the pdaCache_tbl[]. so "create sync" may use the wrong information,such as head_tick etc.
            //HCI/DDI/BI-04-C. notice: require several times to reproduce.
            if(pPdA_cache->cach_flag != CACHE_FLAG_OCCUPIED){
                continue;
            }

            if(!smemcmp (&pPdA_cache->pda_dev_id, cur_pAdv, sizeof(extadv_id_t)) ||\
               blt_ll_searchAddrInWhiteListTbl(pPdA_cache->pda_dev_id.adrType,  pPdA_cache->pda_dev_id.addr) ){

                /* PAST time pass check not need */
                if(pPdA_cache->syncWwUs || !clock_time_exceed(pPdA_cache->header_tick_backup, 2457600*2)){
                    cache_info_exist = 1;
                    break;
                }else{
                    my_dump_str_data(0, "sync anchor point timing err", 0, 0);
                }
            }
        }
/*
 *   curr_time -  x > 2.457s*2
 */
        if(cache_info_exist){
            break;
        }
    }


    pPdA_sync->pda_rx.update_map = 0; //clr
    pPdA_sync->terminate = 0;
    pPdA_sync->terminate_pending = 0;

    /* whether HCI_LE_Periodic_Advertising_Report events for this
     * periodic advertising train are initially enabled or disabled. */
    u8 pda_reportInitEn = options & REPORTING_INITIALLY_DIS ? 0 : 1;
    /* whether duplicate reports are filtered or not. */
    u8 sync_rpt_dup_filtered = (options & DUPLICATE_FILTERING_INITIALLY_EN) ? 1 : 0;

    //determine whether to report periodic adv event.
    pPdA_sync->sync_rcv_enable = 0;
    if(pda_reportInitEn){
        pPdA_sync->sync_rcv_enable = REPORTING_EN;
        my_dump_str_data(0, "enable rpt periodic adv event", 0, 0);
    }
//    else{
//        pPdA_sync->sync_rcv_enable = REPORTING_DIS;
//        my_dump_str_data(0, "disable rpt periodic adv event", 0, 0);
//    }

    //determine duplicate reports are filtered or not.
    if(sync_rpt_dup_filtered){
        pPdA_sync->sync_rcv_enable |= DUPLICATE_FILTERING_EN;
        my_dump_str_data(0, "enable duplicate reports filter", 0, 0);
    }
//    else{
//        pPdA_sync->sync_rcv_enable |= DUPLICATE_FILTERING_DIS;
//        my_dump_str_data(0, "disable duplicate reports filter", 0, 0);
//    }

    blmsParam.pda_syncing_flg = 1;
    //two PDA SID can not use this variable at the same time.only command create_sync and create_sync_cancel to use.
    //create_sync_cancel not use sync_handle,i.e. cancel is paired with create. not other relevant command between the two commands.
    bltScn.pda_syncing_idx = pPdA_sync->pda_index;
    pPdA_sync->sync_state  = SYNC_STATE_WAIT_SYNC_INFO;
    pPdA_sync->sync_specify = !sync_adv_from_list;
    pPdA_sync->max_pda_skip = skip;
    pPdA_sync->sync_timeout_tick = sync_timeout * 10000 * SYSTEM_TIMER_TICK_1US;
    if(!sync_adv_from_list){
        smemcpy (&pPdA_sync->pda_id, cur_pAdv, sizeof(extadv_id_t));
    }

    if(cache_info_exist){
        u32 r1 = irq_disable(); //IRQ & MainLoop variables interact
        pPdA_cache->cach_flag = CACHE_FLAG_SYNCING;
        blt_pda_sync_analyze_prdadv_info(pPdA_sync, pPdA_cache); //32M, 25uS test by SiHui 20210425
        pPdA_sync->sync_state = SYNC_STATE_SYNC_INFO_MATCH;
        irq_restore(r1); //IRQ & MainLoop variables interact
    }
    else{

    }


    bltPdaSync.tick_pda_sync = clock_time () | 1;  // to guarantee not "0"
    bltPdaSync.pdASync_customTmoExpire = 0;
    return BLE_SUCCESS;
}


_attribute_no_inline_
ble_sts_t   blc_hci_le_periodic_advertising_create_sync(hci_le_periodicAdvCreateSync_cmdParam_t* cmdPara)
{
    return blc_ll_periodicAdvertisingCreateSync(cmdPara->Options, cmdPara->Advertising_SID, cmdPara->Advertiser_Address_Type, cmdPara->Advertiser_Address, \
                                                cmdPara->Skip, cmdPara->Sync_Timeout, cmdPara->Sync_CTE_Type);
}



_attribute_noinline_
int blt_pda_syncing_finish(void)//1. createSync_cancel;2. createSync_timeout
{
    int finish_valid = 0;

    u32 r = irq_disable(); //IRQ & MainLoop variables interact

    if (blmsParam.pda_syncing_flg ){
        st_pda_sync_t *pPdA_sync = (st_pda_sync_t*)&pdAsync_tbl[bltScn.pda_syncing_idx];


        if(pPdA_sync->sync_state & SYNC_STATE_SYNCING){

            pdaCache_tbl[pPdA_sync->mapping_cache_idx].cach_flag = CACHE_FLAG_IDLE;

            st_secchn_scn_t *pSecChnScn = &secChnScn_tbl[pPdA_sync->mapping_auxscan_idx];
            pSecChnScn->occupied = 0;
            pSecChnScn->pdaSync_flag = 0;

            //debug
            if(pPdA_sync->mapping_cache_idx >= PERDADV_CACHE_NUM || \
               pPdA_sync->mapping_auxscan_idx >= TSKNUM_SECCHN_SCAN ){
                BLMS_ERR_DEBUG(DBG_PDA_SYNC_LOGIC, 0xFD000000 | pPdA_sync->mapping_cache_idx<<8 | pPdA_sync->mapping_auxscan_idx);
            }
        }

        pPdA_sync->sync_state = SYNC_STATE_IDLE;
        blmsParam.pda_syncing_flg = 0;

    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
        if(pPdA_sync->pawr_acad_check){
            if( bltSche.task_mask & (TSKMSK_PAWRS_SUB_0<<bltScn.pda_syncing_idx) ){
                blt_sche_disableTask(TSKMSK_PAWRS_SUB_0<<bltScn.pda_syncing_idx);
                blmsParam.state_chng |= STATE_CHANGE_PAWR_SYNC;
            }
        }
        else
    #endif
        {
            if( bltSche.task_mask & (TSKMSK_PDA_SYNC_0<<bltScn.pda_syncing_idx) ){
                blt_sche_disableTask(TSKMSK_PDA_SYNC_0<<bltScn.pda_syncing_idx);
                blmsParam.state_chng |= STATE_CHANGE_PDA_SYNC;
            }
        }

        finish_valid = 1;
    }

    irq_restore(r); //IRQ & MainLoop variables interact

    bltPdaSync.tick_pda_sync = 0;

    return finish_valid;
}


_attribute_noinline_
void blt_pda_sync_check_timeout(void)
{
    if (clock_time_exceed (bltPdaSync.tick_pda_sync, bltPdaSync.pdaSync_timeout_us)){

        u8 status = blt_pda_syncing_finish(); //status only two value: 0 or 1

        bltPdaSync.pdASync_customTmoExpire = BIT(7)|status;

        my_dump_str_data(DBG_PDA_SYNC_LOGIC, "pda sync timeout", &bltScn.pda_syncing_idx, 1);

    }
}



_attribute_noinline_
ble_sts_t   blc_ll_periodicAdvertisingCreateSyncCancel (void)
{
    //HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands]
    if(IS_LEGACY_SCAN_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_SCAN_VALID;

    //if custom set the timeout(bltPdaSync.pdaSync_timeout_us = 5000 * 1000) time has expired, then host send pda create sync cancel command.
    if(bltPdaSync.pdASync_customTmoExpire & 0x01){ //finish valid

        if(hci_le_eventMask & HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_ESTABLISHED){
        #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
            if(bltPdaSync.pawr_acad_check){

                bltPdaSync.pawr_acad_check = 0;
                hci_le_periodicAdvSyncEstablished_evt_v2(HCI_ERR_UNKNOWN_CONN_ID, 0, 0, 0, 0, 0, 0, 0, 0,0,0,0);
            }
            else
        #endif
            {
                hci_le_periodicAdvSyncEstablished_evt(HCI_ERR_UNKNOWN_CONN_ID, 0, 0, 0, 0, 0, 0, 0);
            }
        }

        return BLE_SUCCESS;
    }

    if (blmsParam.pda_syncing_flg) //|| bltPdaSync.pdASync_customTmoExpire
    {
        if(blt_pda_syncing_finish()){
            if(hci_le_eventMask & HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_ESTABLISHED){

            #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
                if(bltPdaSync.pawr_acad_check){

                    bltPdaSync.pawr_acad_check = 0;
                    hci_le_periodicAdvSyncEstablished_evt_v2(HCI_ERR_OP_CANCELLED_BY_HOST, 0, 0, 0, 0, 0, 0, 0, 0,0,0,0);
                }
                else
            #endif
                {
                    hci_le_periodicAdvSyncEstablished_evt(HCI_ERR_OP_CANCELLED_BY_HOST, 0, 0, 0, 0, 0, 0, 0);
                }
            }
        }

        return BLE_SUCCESS;
    }
    else
    {
        /* If the Host issues this command while no HCI_LE_Periodic_Advertising_Create_Sync command is pending,
         * the Controller shall return the error code Command Disallowed (0x0C). */
        return  HCI_ERR_CMD_DISALLOWED;
    }
}


_attribute_ram_code_
void   blc_ll_pdaRxFifoPacketProc(st_secchn_scn_t* pSecChnScn){
    u32 r = irq_disable();

    u8 rx_idx = scan_secRxFifo.rptr;

    //traverse the FIFO packet to find all related packet.
    while(rx_idx != scan_secRxFifo.wptr)
    {
        u8* raw_pkt = (u8 *) (scan_secRxFifo.p + SCAN_SECCHN_RXFIFO_SIZE * (rx_idx & SCAN_SECCHN_RXFIFO_MASK));

        u8 secTblIdx  = raw_pkt[2]&(~SECCHN_IDX_MARK);    // index stored on raw_pkt[2]
        st_secchn_scn_t* cur_pPdascan   = (st_secchn_scn_t *)&secChnScn_tbl[secTblIdx];

        //here can also compare cur_pPdascan == pSecChnScn, but 4B address may cost long time than u8
        if(cur_pPdascan->scnIndex == pSecChnScn->scnIndex){
            raw_pkt[3] = SCANRX_FLAG_DATA_DROP;
        }

        rx_idx ++;
    }

    //if there are future task(such as chain packet), need to remove that.
    blt_remove_aux_scan_future_task(pSecChnScn->scnIndex);

    pSecChnScn->scan_rx_flag = 0;

    irq_restore(r);
}

_attribute_noinline_
ble_sts_t   blc_ll_periodicAdvertisingTerminateSync (u16 sync_handle)
{

    //HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands]
    if(IS_LEGACY_SCAN_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_SCAN_VALID;

    /* If the periodic advertising train corresponding to the Sync_Handle parameter does not exist,
     * then the Controller shall return the error code Unknown Advertising Identifier (0x42). */
    if(blt_isSyncHandleValid(sync_handle)){

        //pdAsync_tbl[sync_handle & 3].terminate = 1;
        st_pda_sync_t*   pPdAsync   = (st_pda_sync_t*)&pdAsync_tbl[sync_handle & 3];
        st_secchn_scn_t* pSecChnScn = (st_secchn_scn_t*)&secChnScn_tbl[pPdAsync->mapping_auxscan_idx];

        pPdAsync->terminate = 1;
        pPdAsync->sync_adv_dup_filter = 0xFFFF;
        pPdAsync->createSyncType = PDA_CREATE_SYNC_BY_HCICMD;


        blc_ll_pdaRxFifoPacketProc(pSecChnScn);

        //This restriction not guarantee that termination must be successful. how to do??? pending then mainloop processing???
        //but pending how to guarantee immediately processing. pda_sync_post need to process???
        if(blms_state == BLMS_STATE_NONE ||  (blms_state & BLMS_STATE_PRICHN_SCAN_S)){

            ///not wait pda_sync_post,cause create_sync can be sent by the host before pda_sync_post.
            ///so need to immediately process pda_sync termination event.
            ///////////////////terminate processing///////////////////////////
            pPdAsync->sync_state = SYNC_STATE_IDLE;

            pSecChnScn->occupied = 0;//blt_release_secchn_scan(blt_pSecChnScn, bltExtScn.auxadv_sel);   //clear occupied
            pSecChnScn->pdaSync_flag = 0;

            pdaCache_tbl[pPdAsync->mapping_cache_idx].cach_flag = CACHE_FLAG_IDLE;

        #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
            if(pPdAsync->pawr_acad_check){
                blt_sche_disableTask(TSKMSK_PAWRS_SUB_0<<pPdAsync->pda_index);
                for(int j=0; j<PRDADV_SYNC_FIFONUM; j++){
                    blt_pPdAsync->schTsk_fifo[j].scheTask_oft = TSKOFT_PDA_SYNC + blt_pPdAsync->pda_index;
                    blt_pPdAsync->schTsk_fifo[j].scheTask_flg = TSKFLG_PDA_SYNC;
                }
            }
            else
        #endif
            {
                blt_sche_disableTask(TSKMSK_PDA_SYNC_0<<pPdAsync->pda_index);
            }

            blmsParam.state_chng |= STATE_CHANGE_PDA_SYNC;

            //////// immediately  to process/////////
            u32 cur_tick = clock_time();
            if(tick1_exceed_tick2(systimer_get_irq_capture(), cur_tick + 8*SYSTEM_TIMER_TICK_1MS)){
                systick_irq_trigger = SYS_IRQ_TRIG_SCHE_INSERT;
                systimer_set_irq_capture(cur_tick + 1*SYSTEM_TIMER_TICK_1MS);
            }
        }
        else{
            pPdAsync->terminate_pending = 1; //mainloop to processing.
        }
    }
    else{
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }

    return BLE_SUCCESS;
}



ble_sts_t   blc_ll_addDeviceToPeriodicAdvertiserList (u8 adv_adrType, u8 *adv_addr, u8 adv_sid)
{

    //HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands]
    if(IS_LEGACY_SCAN_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_SCAN_VALID;


    /* If the Host issues this command when an HCI_LE_Periodic_Advertising_Create_Sync command is pending,
     * the Controller shall return the error code Command Disallowed (0x0C). */
    if(blmsParam.pda_syncing_flg){
        return HCI_ERR_CMD_DISALLOWED;
    }


    /* When a Controller cannot add an entry to the Periodic Advertiser list because
       the list is full, the Controller shall return the error code Memory Capacity Exceeded (0x07). */
    if(bltPdaSync.pdA_list_num >=PERDADV_LIST_SIZE){
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }
    else{
        u8 temp_buffer[sizeof(extadv_id_t)];
        extadv_id_t *cur_pAdv = (extadv_id_t *)temp_buffer;
        cur_pAdv->sid = adv_sid;
        cur_pAdv->adrType = adv_adrType;
        smemcpy(cur_pAdv->addr, adv_addr, BLE_ADDR_LEN);

        /* If the entry is already on the list, the Controller shall return the error code
         * Invalid HCI Command Parameters (0x12) */
        for(int i=0;i<bltPdaSync.pdA_list_num;i++){
            if(!smemcmp (&pdaList_tbl[i].list_dev_id, cur_pAdv, sizeof(extadv_id_t))){
                return HCI_ERR_INVALID_HCI_CMD_PARAMS;
            }
        }

        smemcpy (&pdaList_tbl[bltPdaSync.pdA_list_num].list_dev_id, cur_pAdv, sizeof(extadv_id_t));
        /* process synced_mark */
        for(int i=0; i<TSKNUM_PDA_SYNC; i++)
        {
            st_pda_sync_t * pPdA_sync = (st_pda_sync_t *)&pdAsync_tbl[i];
            if( pPdA_sync->sync_state == SYNC_STATE_SYNCED){
                if(!smemcmp (&pPdA_sync->pda_id, cur_pAdv, sizeof(extadv_id_t))){
                    pdaList_tbl[bltPdaSync.pdA_list_num].synced_mark = 1;
                    break;
                }
            }
        }
        bltPdaSync.pdA_list_num ++;
    }

    return BLE_SUCCESS;
}



ble_sts_t   blc_ll_removeDeviceFromPeriodicAdvertiserList (u8 adv_adrType, u8 *adv_addr, u8 adv_sid)
{

    //HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands]
    if(IS_LEGACY_SCAN_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_SCAN_VALID;


    /* If the Host issues this command when an HCI_LE_Periodic_Advertising_Create_Sync command is pending,
     * the Controller shall return the error code Command Disallowed (0x0C). */
    if(blmsParam.pda_syncing_flg){
        return HCI_ERR_CMD_DISALLOWED;
    }


    u8 temp_buffer[sizeof(extadv_id_t)];
    extadv_id_t *cur_pAdv = (extadv_id_t *)temp_buffer;
    cur_pAdv->sid = adv_sid;
    cur_pAdv->adrType = adv_adrType;
    smemcpy(cur_pAdv->addr, adv_addr, BLE_ADDR_LEN);


    for(int i=0;i<bltPdaSync.pdA_list_num;i++){
        if(!smemcmp (&pdaList_tbl[i].list_dev_id, cur_pAdv, sizeof(extadv_id_t))){
            /* bltPdaSync.pdA_list_num-i-1 definitely >= 0 */
            smemcpy(&pdaList_tbl[i], &pdaList_tbl[i+1], (bltPdaSync.pdA_list_num-i-1)*sizeof(pda_list_t));
            bltPdaSync.pdA_list_num --;
            return BLE_SUCCESS;
        }
    }


    /* When a Controller cannot remove an entry from the Periodic Advertiser list because it is not found,
     * the Controller shall return the error code Unknown Advertising Identifier (0x42). */
    return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
}


ble_sts_t   blc_ll_clearPeriodicAdvertiserList (void)
{
    //HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands]
    if(IS_LEGACY_SCAN_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_SCAN_VALID;


    /* If the Host issues this command when an HCI_LE_Periodic_Advertising_Create_Sync command is pending,
     * the Controller shall return the error code Command Disallowed (0x0C). */
    if(blmsParam.pda_syncing_flg){
        return HCI_ERR_CMD_DISALLOWED;
    }


    bltPdaSync.pdA_list_num = 0;

    return BLE_SUCCESS;
}


ble_sts_t   blc_ll_readPeriodicAdvertiserListSize (u8 *perdAdvListSize)
{
    //HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands]
    if(IS_LEGACY_SCAN_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_SCAN_VALID;

    *perdAdvListSize = PERDADV_LIST_SIZE;

    return BLE_SUCCESS;
}







#if (SUB_INTERVAL_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int blt_pda_sync_main_loop(void)
{

    st_pda_sync_t *pPdA_sync;
    for(int pda_idx=0; pda_idx<TSKNUM_PDA_SYNC; pda_idx++)
    {
        pPdA_sync = (st_pda_sync_t*)&pdAsync_tbl[pda_idx];
        u16 sync_handle = BLT_SYNC_HANDLE | pda_idx;
        if(pPdA_sync->sync_establish == SYNC_ESTABLISHED_BY_HCICMD){
            if(hci_le_eventMask & HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_ESTABLISHED)
            {
                u8 status = BLE_SUCCESS;
                if(pPdA_sync->sync_state == SYNC_STATE_IDLE){
                    status = HCI_ERR_CONN_FAILED_TO_ESTABLISH;
                }

                #if (LL_FEATURE_ENABLE_LE_AOA_AOD)
                    if(pPdA_sync->sync_wrong_cte_type){
                        status = pPdA_sync->sync_wrong_cte_type;
                    }
                #endif

            #if(LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
                if(pPdA_sync->pawr_acad_check){
                    hci_le_periodicAdvSyncEstablished_evt_v2(status, sync_handle, pPdA_sync->pda_id.sid, pPdA_sync->pda_id.adrType, \
                                                             pPdA_sync->pda_id.addr, pPdA_sync->pda_rx.pda_phy, pPdA_sync->pda_interval, pPdA_sync->sca,\
                                                             pPdA_sync->pawr_acadInfo.num_subevent, pPdA_sync->pawr_acadInfo.subevent_intvl,\
                                                             pPdA_sync->pawr_acadInfo.rsp_slot_delay, pPdA_sync->pawr_acadInfo.rsp_slot_spacing);
                }else
            #endif
                {
                    hci_le_periodicAdvSyncEstablished_evt(status, sync_handle, pPdA_sync->pda_id.sid, pPdA_sync->pda_id.adrType, \
                                                          pPdA_sync->pda_id.addr, pPdA_sync->pda_rx.pda_phy, pPdA_sync->pda_interval, pPdA_sync->sca);
                }

            }

            pPdA_sync->sync_establish = 0;
        }

        if(pPdA_sync->sync_lost){
            if(hci_le_eventMask & HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_LOST)
            {
                hci_le_periodicAdvSyncLost_evt(sync_handle);
            }

            pPdA_sync->pda_rx.update_map = 0; //clr
            pPdA_sync->sync_lost = 0;
            pPdA_sync->createSyncType = PDA_CREATE_SYNC_BY_HCICMD;
        }

        ////////////////////////////////////
        if(pPdA_sync->terminate_pending){

            ///not wait pda_sync_post,cause create_sync can be sent by the host before pda_sync_post.
            ///so need to immediately process pda_sync termination event.
            if(blms_state == BLMS_STATE_NONE ||  (blms_state & BLMS_STATE_PRICHN_SCAN_S)){
                pPdA_sync->terminate_pending = 0;

                ///////////////////terminate processing///////////////////////////
                st_secchn_scn_t* pSecChnScn = (st_secchn_scn_t*)&secChnScn_tbl[pPdA_sync->mapping_auxscan_idx];

                pPdA_sync->sync_state = SYNC_STATE_IDLE;

                pSecChnScn->occupied = 0;//blt_release_secchn_scan(blt_pSecChnScn, bltExtScn.auxadv_sel);   //clear occupied
                pSecChnScn->pdaSync_flag = 0;
                pdaCache_tbl[pPdA_sync->mapping_cache_idx].cach_flag = CACHE_FLAG_IDLE;

            #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
                if(pPdA_sync->pawr_acad_check){
                    blt_sche_disableTask(TSKMSK_PAWRS_SUB_0<<pPdA_sync->pda_index);
                    //later will use other process
                    for(int j=0; j<PRDADV_SYNC_FIFONUM; j++){
                        blt_pPdAsync->schTsk_fifo[j].scheTask_oft = TSKOFT_PDA_SYNC + blt_pPdAsync->pda_index;
                        blt_pPdAsync->schTsk_fifo[j].scheTask_flg = TSKFLG_PDA_SYNC;
                    }
                }
                else
            #endif
                {
                    blt_sche_disableTask(TSKMSK_PDA_SYNC_0<<pPdA_sync->pda_index);
                }

                blmsParam.state_chng |= STATE_CHANGE_PDA_SYNC;

                //////// immediately  to process/////////
                u32 cur_tick = clock_time();
                //8ms refer to xiaomi controller's 10ms requirement. PDA can also refer to that.
                if(tick1_exceed_tick2(systimer_get_irq_capture(), cur_tick + 8*SYSTEM_TIMER_TICK_1MS)){
                    systick_irq_trigger = SYS_IRQ_TRIG_SCHE_INSERT;
                    systimer_set_irq_capture(cur_tick + 1*SYSTEM_TIMER_TICK_1MS);
                }
            }

        }
    }




    //----------periodic ADV create_sync timeout check -----------------------------------
    if(bltPdaSync.tick_pda_sync){
        blt_pda_sync_check_timeout();
    }


    return 0;
}







bool blt_isSyncHandleValid (u16 sync_handle)
{
    if (!(sync_handle & BLT_SYNC_HANDLE) || (sync_handle & 3) >= TSKNUM_PDA_SYNC || \
        pdAsync_tbl[sync_handle & 3].sync_state != SYNC_STATE_SYNCED )
    {
        return FALSE;
    }

    return TRUE;
}


_attribute_ram_code_
void blt_ll_pda_sync_sslot_reset(void){

    st_pda_sync_t *pPdA_sync;
    bigInfor_para_t *pBigInfor_para;

    for(int i=0; i<TSKNUM_PDA_SYNC; i++)
    {
        pPdA_sync = (st_pda_sync_t *)&pdAsync_tbl[i];

        pBigInfor_para = &pPdA_sync->bigInfor_para;
        pBigInfor_para->sSlot_idx_Rx -= bltSche.sSlot_idx_past; //just for big sync. big use periodic's sSlot_idx_Rx to sync.
    }
}



_attribute_ram_code_
int blt_ll_period_bisAcad_process(u8* bigInfor)
{
    u32 curBigInfoLen = bigInfor[0];// len + type + bigInfor

    //The length of the BIGInfo is 33 octets for an unencrypted BIG and 57 octets for an encrypted BIG
    if(!((curBigInfoLen ==34) || (curBigInfoLen==58))){

        my_dump_str_data(DBG_PDA_SYNC_LOGIC, "ACAD len not match BIGInfor", &curBigInfoLen, 4);
        return 0;
    }

    u8  encrypt = (curBigInfoLen==58)?1:0; //bigInfor[0] BIG len = type(1) + bigInfor(57)

    bigInfor_para_t *pBigInfor_para = &blt_pPdAsync->bigInfor_para;

    // in case of, hci create BisSync using bigEventCnt = N in BigInfor A, then crate task using the corresponding sSlot,But Before BigSync Start,
    // Receiving another BigInfor B with bigEventCnt = N+1, then BigSync start will use BigEventCnt = N+1 to judge the jump cnt
    if(!pBigInfor_para->creating_bisSync_flag){

        pBigInfor_para->bigInfor_flag = 1;
        pBigInfor_para->bigEncrypt = encrypt;
        pBigInfor_para->bigInfor_rx_tick = bltRxPkt.rx_header_tick;
        pBigInfor_para->bSlot_idx_Rx = bltSche.bSlot_idx_irq_real + (bltRxPkt.rx_header_tick - bltSche.bSlot_tick_irq_real)/SYSTEM_TIMER_TICK_625US;
        pBigInfor_para->sSlot_idx_Rx = bltSche.sSlot_idx_irq_real + (bltRxPkt.rx_header_tick - bltSche.sSlot_tick_irq_real)*SSLOT_TICK_REVERSE;; //SYSTEM_TIMER_TICK_625US
        smemcpy(pBigInfor_para->bigInfo, &bigInfor[2], curBigInfoLen-1);//

        DBG_CHN10_TOGGLE;DBG_CHN10_TOGGLE;DBG_CHN10_TOGGLE;DBG_CHN10_TOGGLE;
        DBG_FANQH_CHN10_TOGGLE;DBG_FANQH_CHN10_TOGGLE;DBG_FANQH_CHN10_TOGGLE;DBG_FANQH_CHN10_TOGGLE;

//      my_dump_str_u32s(BIS_SNC_BV_10, "rec BigSync Slot", pBigInfor_para->bSlot_idx_Rx, pBigInfor_para->sSlot_idx_Rx,0,0);

        return (BLT_SYNC_HANDLE|bltPdaSync.pdA_sync_sel);
    }
    else{
        return 0;
    }
}

_attribute_ram_code_
int blt_ll_period_chmUptAcad_process(u8* pChmUptAcad)
{
    pda_sync_chmupt_t * pChmUpt = (pda_sync_chmupt_t*)pChmUptAcad;

    if(!(blt_pPda->update_map & PDA_UPDATE_MAP) && pChmUpt->len+1 == sizeof(pda_sync_chmupt_t)){

        /**
         *  blt_calBit1Number should be in ramcode(called in IRQ) if enable BQB_TEST_EN !!!
         *  In actual production, the BQB_TEST_EN in the SDK is closed, so the API: blt_calBit1Number
         *  is not placed on the ramcode section to save ramcode consumption. Add by tuyf 24/03/08, comfirm with QW
         **/
        #if BQB_TEST_EN
            u8 *pChm = pChmUpt->chm;
            u32 chanMapLow = (u32)pChm[0] + ((u32)pChm[1] << 8) + ((u32)pChm[2] << 16) + ((u32)pChm[3] << 24);
            u32 chanMapHigh = (u32)pChm[4]&0x1F;
            u8 cnt = blt_calBit1Number(chanMapLow) + blt_calBit1Number(chanMapHigh);

            if(cnt < 2 || (pChm[4]&0xe0) != 0){
                return 0;
            }
        #endif

        s16 diff_inst = pChmUpt->instant - blt_pPda->paEvtCnt;
        if(diff_inst > 0 ){
            blt_pPda->update_map |= PDA_UPDATE_MAP;
            smemcpy ( blt_pPda->nextChn.chmTbl, pChmUpt->chm, 5);
            blt_pPda->prd_map_inst_next = pChmUpt->instant;
            csa2_calculateMapInfo(&blt_pPda->nextChn);
            my_dump_str_data(0, "Periodic Adv SYNC CHANNEL_MAP mark", 0, 0);
        }

    }

    return 0;
}

_attribute_ram_code_
int blt_pda_sync_interrupt_task (int flag, void *p)
{
    int index = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_SCHEDULE_START){
        DBG_CHN12_HIGH;
        //DBG_QIUWEI_CHN1_HIGH;

        DBG_FANQH_CHN2_HIGH;
        #if (SL01_pdachn_scn)
            log_task_begin_irq(SL_STACK_EXTSCAN_BASIC_TIMING_EN, SL01_pdachn_scn);
        #endif
        blt_pda_sync_start(index);
    }
    else if(flag & FLAG_SCHEDULE_DONE){
        blt_pda_sync_post();
        DBG_CHN12_LOW;
        //DBG_QIUWEI_CHN1_LOW;
        DBG_FANQH_CHN2_LOW;
        #if (SL01_pdachn_scn)
            log_task_end_irq(SL_STACK_EXTSCAN_BASIC_TIMING_EN, SL01_pdachn_scn);
        #endif
    }
    else if(flag & FLAG_PRDADV_SYNC_RX){
        DBG_CHN12_TOGGLE;
        blt_pda_sync_rx(p);
        DBG_CHN12_TOGGLE;
        DBG_FANQH_CHN1_TOGGLE;
    }
    else if(flag & FLAG_SCHEDULE_BUILD){
        blt_pda_sync_build_task();
    }
    else if(flag & FLAG_PRDADV_SYNC_ACAD_BIGINFOR){
        return blt_ll_period_bisAcad_process((u8*)p);
    }
    else if(flag & FLAG_PRDADV_SYNC_ACAD_CHMUPT){
        return blt_ll_period_chmUptAcad_process((u8*)p);
    }
    else if(flag == FLAG_SCHEDULE_SSLOT_RESET){
        blt_ll_pda_sync_sslot_reset();
    }
    else if(flag & FLAG_INSERT_SCHTSK_CONFLICT){
        sch_task_t *pTgtTsk = (sch_task_t *)p;
        u8 tgtTskFlg = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
        (void)tgtTskFlg;
        u8 curSchTaskOft = TSKOFT_PDA_SYNC + index;

        my_dump_str_data(0, "[per_sync]insertTsk conflict, tgtTsk=", &tgtTskFlg, 1);
#if(LL_BIS_SYNC_TEST)
        if(tgtTskFlg == TSKFLG_BIG_SYNC){
            return 0;
        }
#endif

        #if(SL08_pdaSync_conflict)
        log_b8_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SL08_pdaSync_conflict, tgtTskFlg);
        #endif

        #if(SCH_TASK_PRIORITY_IN_CB_EN)
            s32 pri_taskCur = bltPri.pri_cal[curSchTaskOft];
            s32 pri_taskTra = bltPri.pri_cal[pTgtTsk->scheTask_oft];
             //priority higher than exist task, can insert target task
            if(pri_taskCur > pri_taskTra){
                return 1;
            }
        #endif




        //Task scheduler has been abandoned bigger than 5 times
        if(bltPri.csctvAbandonCnt[curSchTaskOft] >= 5){
            my_dump_str_data(0, "[per_sync]consecutive abandon count", &bltPri.csctvAbandonCnt[curSchTaskOft], 2)
            return 1; /* 1:conflict resolved; 0: insert task failed */
        }
    #if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
        else if(tgtTskFlg == TSKFLG_BIG_BCST && (global_pBigBcst+pTgtTsk->scheTask_idx)->big_sc_mask){
            my_dump_str_data(0, "[per_sync]abandon, bis_SC proc", &bltPri.csctvAbandonCnt[curSchTaskOft], 2)
            return 0;
        }
    #endif
    }

    return 0;
}



_attribute_noinline_
int blt_pda_sync_mainloop_task (int flag)
{
    if(flag == FLAG_MODULE_MAINLOOP){
        blt_pda_sync_main_loop();
    }
    else if(flag == FLAG_PRDADV_DATA_REPORT){
        return blt_pda_sync_prdadv_data_report();
    }
    else if(flag == FLAG_MODULE_RESET){
        blt_reset_pda_sync();
    }

    return 0;
}



_attribute_ram_code_ void blt_pda_sync_analyze_prdadv_info(st_pda_sync_t * pPdA_sync, pda_cache_t *pPdA_cache)
{

    if(1)
    {

        st_pda_t *p_curPda = (st_pda_t *) &pPdA_sync->pda_rx;
        p_curPda->pda_phy = pPdA_cache->prdphy;
        p_curPda->paEvtCnt = pPdA_cache->sncInf.evtCounter;
        p_curPda->paAccessAddr = pPdA_cache->sncInf.AA;
        smemcpy(&p_curPda->paCrcInit, pPdA_cache->sncInf.crcInit, 3);

        p_curPda->chnIdentifier = (p_curPda->paAccessAddr>>16) ^ (p_curPda->paAccessAddr&0xffff); //channel identifier
        pPdA_sync->sca = pPdA_cache->sncInf.chm[4]>>5;
        pPdA_cache->sncInf.chm[4] &= 0x1F;
        smemcpy ( p_curPda->chnParam.map.chmTbl, pPdA_cache->sncInf.chm, 5);
        csa2_calculateMapInfo(&p_curPda->chnParam.map);

        pPdA_sync->pda_interval = pPdA_cache->sncInf.itvl;
        pPdA_sync->pdaInterval_tick = pPdA_sync->pda_interval*SYSTEM_TIMER_TICK_1250US;
        p_curPda->bSlot_prdadv_itvl =  pPdA_sync->pda_interval<<1; //     1.25mS unit -> 625uS unit
        p_curPda->sSlot_prdadv_itvl = pPdA_sync->pda_interval<<6;  //*64: 1.25mS unit -> sSlot unit

        #if (LL_FEATURE_ENABLE_PRIVACY) /*  update pda_sync table's addr */
            pPdA_sync->record_advA_adrType = pPdA_cache->record_advA_adrType;
            smemcpy(pPdA_sync->record_advA_addr, pPdA_cache->record_advA_addr, BLE_ADDR_LEN);
        #endif

        #if(LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
            pPdA_sync->pawr_acad_check = 0;
            bltPdaSync.pawr_acad_check = 0;

            //recode subevent information, such as response AA, subevent number,slot spacing, etc.
            // both pad_interval and subevent_intvl are in unit of 1.25ms. round down is right.
            if(pPdA_cache->pawr_acad_valid){
                if( (pPdA_sync->pda_interval/pPdA_cache->pawr_acad.subevent_intvl) > pPdA_cache->pawr_acad.num_subevent ){
                    smemcpy(&pPdA_sync->pawr_acadInfo, &pPdA_cache->pawr_acad, sizeof(pawr_acad_t));

                    //subevent_intvl is in unit of 1.25ms
                    u32 tmpSubevtIntvl_bSlot = pPdA_sync->pawr_acadInfo.subevent_intvl;
                    pPdA_sync->subevtIntvl_sSlot = tmpSubevtIntvl_bSlot<<6;
                    pPdA_sync->pawr_acad_check = 1; //here need to judge whether parameters are right. later will add.
                    bltPdaSync.pawr_acad_check = 1;
                }else{
                    my_dump_str_data(DBG_PAwR_SYNC_LOGIC, "pawr acac and syncInfo parameter error", 0, 0);
                }
            }

        #endif

        u32 distance_us = pPdA_cache->sncInf.syncPktOffset * (pPdA_cache->sncInf.offsetUnit == EXT_ADV_PDU_SYNC_OFFSET_UNITS_300_US ? 300 : 30);
        if(pPdA_cache->sncInf.offsetAdjust == 1){
            distance_us += 2457600;
        }


        //      u16 dis_ms = distance_us/1000;
        //      my_dump_str_data(DBG_PDA_SYNC_LOGIC, "distance ms", &dis_ms, 2);

        u32 pda_expect_tick =  pPdA_cache->header_tick_backup + distance_us*SYSTEM_TIMER_TICK_1US;


//      u16 raw_cnter = p_curPda->paEvtCnt;
        u32 pastSyncWwUs = pPdA_cache->syncWwUs;
        u32 next_tick = clock_time() + 800*SYSTEM_TIMER_TICK_1US;
        if(tick1_exceed_tick2(next_tick, pda_expect_tick)){
            int jump = (next_tick - pda_expect_tick)/pPdA_sync->pdaInterval_tick + 1;
            pda_expect_tick += jump * pPdA_sync->pdaInterval_tick;
            p_curPda->paEvtCnt += jump;
            distance_us += jump * pPdA_sync->pda_interval * 1250;
            /* 1000000 approximately equal to 1024*1024 = 2^20 */
            if(pastSyncWwUs){
                my_dump_str_data(0, "jump", &jump, 4);
                pastSyncWwUs += (max_err_us_in_1S[pPdA_sync->sca] * jump * pPdA_sync->pda_interval * 1250 + 999999) /1000000;
            }
        }


        //attention: now do not consider local tolerance
        if(distance_us < 1000 * SYSTEM_TIMER_TICK_1US){
            pPdA_sync->tolerance_pda_us = 30;
        }
        else{
            /* long timing, consider accurate tolerance */
            if(pPdA_cache->sncInf.offsetUnit == EXT_ADV_PDU_SYNC_OFFSET_UNITS_300_US){
                pPdA_sync->tolerance_pda_us = 300;
            }
            else{
                pPdA_sync->tolerance_pda_us = 30;
            }

            pPdA_sync->max_err_us_per_second = max_err_us_in_1S[pPdA_sync->sca];

            /* 1000000 approximately equal to 1024*1024 = 2^20 */
            pPdA_sync->tolerance_pda_us += pPdA_sync->max_err_us_per_second*distance_us>>20;
        }
        u32 pda_pkt_us = 0;
        if(pPdA_sync->pda_rx.pda_phy == BLE_PHY_CODED){
            pda_pkt_us = 17040;   //biggest rfLen 255
        }
        else{ //1M or 2M
            #if LL_FEATURE_ENABLE_LE_AOA_AOD
                pda_pkt_us = 2120 + (blmsParam.cte_connLess_en ? 160 : 0);  //2280 = 2120(biggest rfLen 255) + 160(CTE)
            #else
                pda_pkt_us = 2120;  //biggest rfLen 255
            #endif
        }

        /* if PAST used, PAST recipient will calculate 'the syncWwUs', here we NOT use this value, check it latter */
        u32 pastSync1stRxWw = 0;
        if(1 && pastSyncWwUs){
            pPdA_sync->tolerance_pda_us = max(pastSyncWwUs, pPdA_sync->tolerance_pda_us);
            pastSync1stRxWw = (pda_pkt_us + PDASYNC_TAIL_MARGIN_US)>>1;
        }

        pPdA_sync->sync_early_set_us = pPdA_sync->tolerance_pda_us + EXTSCAN_PREPARE_US;
        pda_expect_tick -= pPdA_sync->sync_early_set_us*SYSTEM_TIMER_TICK_1US;

        if(pPdA_cache->sncInf.offsetUnit == EXT_ADV_PDU_SYNC_OFFSET_UNITS_300_US){ //can not exceed the duration
            //Version 5.3 | Vol 6, Part B page 2696.not start any earlier than syncPacketWindowOffset after the reference point
            //no later than syncPacketWindowOffset plus one Offset unit after the reference point.
            //so here +300
            pPdA_sync->pdaSync_srx1stTimeout_us = pPdA_sync->sync_early_set_us + bltPHYs.prmb_ac_us + 150 + 300 + pastSync1stRxWw;
        }else{
            pPdA_sync->pdaSync_srx1stTimeout_us = pPdA_sync->sync_early_set_us + bltPHYs.prmb_ac_us + 150 + pastSync1stRxWw;
        }

        int n_bSlot = (pda_expect_tick - bltSche.bSlot_tick_irq_real)/SYSTEM_TIMER_TICK_625US;
        p_curPda->bSlot_mark_prdadv = bltSche.bSlot_idx_irq_real + n_bSlot - p_curPda->bSlot_prdadv_itvl;

        int n_sSlot = (pda_expect_tick - bltSche.sSlot_tick_irq_real)*SSLOT_TICK_REVERSE;
        pPdA_sync->sSlot_mark_prdadv = bltSche.sSlot_idx_irq_real + n_sSlot - p_curPda->sSlot_prdadv_itvl;

#if 0 //later will process
        if(pPdA_sync->pawr_acad_check){
            blmsParam.state_chng |= STATE_CHANGE_PAWR_SYNC;
            blt_sche_enableTask(TSKMSK_PAWRS_SUB_0<<bltScn.pda_syncing_idx);
        }else{
            blmsParam.state_chng |= STATE_CHANGE_PDA_SYNC;
            blt_sche_enableTask(TSKMSK_PDA_SYNC_0<<bltScn.pda_syncing_idx);
        }
#else
        blmsParam.state_chng |= STATE_CHANGE_PDA_SYNC;
        blt_sche_enableTask(TSKMSK_PDA_SYNC_0<<bltScn.pda_syncing_idx);
#endif

        u32 pda_sync_itvl = pPdA_cache->sncInf.itvl * 1250 * SYSTEM_TIMER_TICK_1US;
        int interval_weight = 0; //0~80
        if(pda_sync_itvl < CONN_INTERVAL_100MS){
            interval_weight = CONN_INTERVAL_100MS - pda_sync_itvl;
        }
        /* small interval get big priority, cause it can be sync_ed with less duration, better for timing*/
        blt_ll_setSchedulerTaskPriority(TSKOFT_PDA_SYNC + bltScn.pda_syncing_idx, TASK_PRIORITY_PDA_SYNCING_DFT + interval_weight);

    }

}




_attribute_ram_code_
void blt_pda_sync_build_task(void)
{

    u32 i,j;

    st_pda_sync_t *pPdA_sync;
    st_pda_t *p_curPda;

    int int_jump_task;
    s32 sSlot_start_task;


    for(i=0; i<TSKNUM_PDA_SYNC; i++)
    {
        if( bltSche.task_mask & (TSKMSK_PDA_SYNC_0<<i) )
        {
            pPdA_sync = (st_pda_sync_t *)&pdAsync_tbl[i];
            p_curPda = (st_pda_t *) &pPdA_sync->pda_rx;

            ////////////////////////////////////////////////////////
        #if (PDA_SYNC_TIMING_ADJUST_EN)
            if(pda_sync_timingAdjust[i].sSlot_offset){
                pPdA_sync->sSlot_mark_prdadv += pda_sync_timingAdjust[i].sSlot_offset;
                pda_sync_timingAdjust[i].sSlot_offset = 0;
            }
        #endif
            ////////////////////////////////////////////////////////

            if(bltSche.build_index == 0){
                if(bltSche.sSlot_idx_reset == 1){
                    pPdA_sync->sSlot_mark_prdadv -= bltSche.sSlot_idx_past;
                }
            }


            if( pPdA_sync->sSlot_mark_prdadv >= bltSche.sSlot_idx_next){
                int_jump_task = 0;
                sSlot_start_task = pPdA_sync->sSlot_mark_prdadv + p_curPda->sSlot_prdadv_itvl;
            }
            else
            {
                int_jump_task = (bltSche.sSlot_idx_next - 1 - pPdA_sync->sSlot_mark_prdadv)/p_curPda->sSlot_prdadv_itvl;
                sSlot_start_task = pPdA_sync->sSlot_mark_prdadv + (int_jump_task + 1)*p_curPda->sSlot_prdadv_itvl;
            }


            if(sSlot_start_task >= bltSche.sSlot_endIdx_dft){ //to save some time for big interval
                continue; //attention: can not use break !!!
            }




            if(pPdA_sync->sync_state == SYNC_STATE_SYNC_INFO_MATCH){
                pPdA_sync->sync_state = SYNC_STATE_SYNCING;

                /* find a aux_scan table to use */
                st_secchn_scn_t *cur_pauxscn = NULL, *available_pauxscn = NULL;
                for(int idx=0; idx<TSKNUM_SECCHN_SCAN; idx++){
                    cur_pauxscn = (st_secchn_scn_t *)&secChnScn_tbl[idx];
                    if(cur_pauxscn->occupied){

                    }
                    else{
                        if(!available_pauxscn){
                            available_pauxscn = cur_pauxscn;
                            break;
                        }
                    }
                }


                if(available_pauxscn){

                }
                else{
                    /* no available aux_scan resource, force to use last one */
                    available_pauxscn = cur_pauxscn;
                    blt_remove_aux_scan_future_task(TSKNUM_SECCHN_SCAN - 1);

                    //TODO: SiHui, process data report completeness
                }

                //my_dump_str_data(DBG_PDA_SYNC_LOGIC, "aux table", &available_pauxscn->scnIndex, 1);
                available_pauxscn->occupied = 1;  //occupy this resource until sync timeout or sync lost

            #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
                if(pPdA_sync->pawr_acad_check){
                    available_pauxscn->pdaSync_flag = PAWR_PACKET_FLAG;  //must clear when sync_cancel or sync_timeout or sync_lost
                }else{
                    available_pauxscn->pdaSync_flag = PADVB_PACKET_FLAG;  //must clear when sync_cancel or sync_timeout or sync_lost
                }
            #else
                available_pauxscn->pdaSync_flag = PADVB_PACKET_FLAG;  //must clear when sync_cancel or sync_timeout or sync_lost
            #endif

                available_pauxscn->pdaSync_idx = pPdA_sync->pda_index; //pda_index: 0, 1 for support two pda.
                available_pauxscn->secchn_phy = p_curPda->pda_phy;

                /* search "PRD_ADV Special process 1" you will find the reason */
                available_pauxscn->scan_advMode = LL_EXTADV_MODE_NON_CONN_NON_SCAN;

                //available_pauxscn->rfLen_max = 255;
                //available_pauxscn->tolerance_peer_us = pPdA_sync->tolerance_pda_us;
                //available_pauxscn->scan_early_set_us = pPdA_sync->sync_early_set_us;


                if(p_curPda->pda_phy == BLE_PHY_CODED){
                    //aux_pkt_us = 3984;
                    //aux_pkt_max_us = 17040;
                    //cur_pauxscn->scan_duration_flag = DURATION_FLAG_MIN_TIME;
                    pPdA_sync->pda_pkt_us = 17040;   //biggest rfLen 255
                    //available_pauxscn->scan_duration_flag = DURATION_FLAG_MAX_TIME;
                }
                else{ //1M or 2M
                    #if LL_FEATURE_ENABLE_LE_AOA_AOD
                        pPdA_sync->pda_pkt_us = 2120 + (blmsParam.cte_connLess_en ? 160 : 0);  //2280 = 2120(biggest rfLen 255) + 160(CTE)
                    #else
                        pPdA_sync->pda_pkt_us = 2120;//biggest rfLen 255
                    #endif
                    //available_pauxscn->scan_duration_flag = DURATION_FLAG_MAX_TIME;
                }

                //TODO: for tolerance, duration should add twice of it
                pPdA_sync->pda_duration_us = pPdA_sync->sync_early_set_us + pPdA_sync->pda_pkt_us + PDASYNC_TAIL_MARGIN_US;
//              p_curPda->sSlot_duration_pda = (pPdA_sync->pda_duration_us + SLOT_PROCESS_MAX_US)*SSLOT_TICK_REVERSE;

                p_curPda->sSlot_duration_pda = (pPdA_sync->pda_duration_us + SLOT_PROCESS_MAX_US)*SSLOT_US_REVERSE;

                pPdA_sync->mapping_auxscan_idx = available_pauxscn->scnIndex;
                pPdA_sync->sync_err_cnt = 0;
                //blt_ll_setSchedulerTaskPriority( TSKOFT_PDA_SYNC + i, TASK_PRIORITY_PDA_SYNCING_DFT );

                #if PDA_SYNC_EBQ
                    pPdA_sync->sync_report_allow = 0;
                #endif
            }






            int new_task_cnt = 0;
            for(j=0;j<PRDADV_SYNC_FIFONUM;j++){

                sch_task_t  *pCur_schTask = (sch_task_t *)&pPdA_sync->schTsk_fifo[j];

                pCur_schTask->begin = sSlot_start_task + j*p_curPda->sSlot_prdadv_itvl;
                pCur_schTask->end = pCur_schTask->begin + p_curPda->sSlot_duration_pda - 1;

                if( pCur_schTask->begin >=  bltSche.sSlot_endIdx_dft){  //new task beyond correct range, finish
                    break;
                }
                else if(pCur_schTask->end < bltSche.sSlot_endIdx_dft){ //new task in correct range
                    new_task_cnt ++;
                }
                else{ //new task across "sSlot_endIdx_dft"
                    //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                    if(bltPri.pri_cal[TSKOFT_PDA_SYNC + i] > bltPri.priMax_value){
                        bltPri.priMax_value = bltPri.pri_cal[TSKOFT_PDA_SYNC + i];
                        bltPri.priMax_index = TSKOFT_PDA_SYNC + i;
                        bltSche.sSlot_endIdx_maxPri = pCur_schTask->begin;
                    }

                    break;
                }
            }



            if(new_task_cnt){
                //blt_ll_incSchedulerTaskCalPriority( TSKOFT_PDA_SYNC + i, 2 * int_jump_task );
                blt_ll_addTask2ExistLinklist( &pPdA_sync->schTsk_fifo[0], new_task_cnt);
            }
        }
    }

}


_attribute_ram_code_
void blt_pda_sync_start(u8 index)
{
    bltPdaSync.pdA_sync_sel = index;
    blt_pPdAsync = (st_pda_sync_t *)&pdAsync_tbl[index];
    blt_pPda = (st_pda_t *) &blt_pPdAsync->pda_rx;
    blt_pSecChnScn = (st_secchn_scn_t *)&secChnScn_tbl[blt_pPdAsync->mapping_auxscan_idx];

    /* 1. special case, when sync_lost, remove task_mask, but not update link_list immediately, task may exist
     *    in rest of 80mS link_list timing, use sync_state_idle to control task not execute
     * 2. RX FIFO not released, can not scan, must abandon this aux_scan task  */
    if( blt_pPdAsync->sync_state == SYNC_STATE_IDLE || \
       ((u8)(scan_secRxFifo.wptr - scan_secRxFifo.rptr) & 31)  >= SCAN_SECCHN_RXFIFO_NUM)
    {
        systimer_set_irq_capture(bltSche.sSlot_tick_irq + 100 * SYSTEM_TIMER_TICK_1US);
        blmsParam.rf_fsm_busy = 0;
    }
    else
    {
        #if (LL_FEATURE_ENABLE_LE_AOA_AOD)
        if(cte_connLess_switchPattern[bltPdaSync.pdA_sync_sel].cte_sample_en){
            if((blt_pPdAsync->sync_cte_type == AOA_TYPE) && (cte_connLess_switchPattern[bltPdaSync.pdA_sync_sel].cte_slot_duration == SWITCH_SAMPLE_SLOT_1US)){
                aoa_set_sample_slot_time(SAMPLE_1US_SLOT);
                DBG_CHN10_HIGH;
            }
            else{
                aoa_set_sample_slot_time(SAMPLE_NORMAL_SLOT);
                DBG_CHN10_HIGH;
                DBG_CHN10_TOGGLE;
                DBG_CHN10_TOGGLE;
            }

            rf_set_aoa_aod_trx_mode(RF_RX_ADV_AOA_EN);
            cte_connLess_switchPattern[bltPdaSync.pdA_sync_sel].cte_rx_mode_en = 1;
        }
        #endif

        //gpio change may cost some time, so place here and there will be decades us before RF start.
        if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON); }

        int tmp_jump = blt_pda_start_common_1();
        if(tmp_jump > 0){
            blt_pPdAsync->sync_err_cnt += tmp_jump;
        }

        rf_ble_set_rx_settle(RX_SETTLE_US);

        rf_start_fsm(FSM_SRX, NULL, clock_time());


//      blt_pSecChnScn->tolerance_peer_us = blt_pPdAsync->tolerance_pda_us;
//      blt_pSecChnScn->scan_early_set_us = blt_pPdAsync->sync_early_set_us;
//      blt_pSecChnScn->rfLen_max = 255;

        //rf_set_1st_rx_timeout(blt_pSecChnScn->scan_early_set_us + bltPHYs.prmb_ac_us + 50);
//      rf_set_1st_rx_timeout(blt_pPdAsync->sync_early_set_us + bltPHYs.prmb_ac_us + 50);

        u16 pdaSync_1stRxTm_margin = blt_pPda->pda_phy == BLE_PHY_CODED ? 300:0;
        if(blt_pPdAsync->pdaSync_srx1stTimeout_us){ //when the first sync. get the syncInfor from aux_adv_ind and the offset maybe + offsetUnit.
            rf_set_1st_rx_timeout(blt_pPdAsync->pdaSync_srx1stTimeout_us + pdaSync_1stRxTm_margin);
            if(blt_pPdAsync->sync_state == SYNC_STATE_SYNCED){
                blt_pPdAsync->pdaSync_srx1stTimeout_us = 0;
            }else{
                #if(LL_CON_PER_BV98C_AND_CON_CEN_BV94C)
                    //+60us step/per lost CI
                    blt_pPdAsync->pdaSync_srx1stTimeout_us += (blt_pPdAsync->sync_err_cnt ? 2 : 0)*60; // enlarge max 6 * 60us,  *2: left -60 ~ right +60
                #endif
            }
        }else{
            rf_set_1st_rx_timeout(blt_pPdAsync->sync_early_set_us + bltPHYs.prmb_ac_us + 150 + pdaSync_1stRxTm_margin);
        }

        ble_rf_set_rx_dma((u8*)bltExtScn.scan_rx_sec_chn_dma_buff, bltExtScn.scan_rx_sec_chn_dma_size);
        //rf_set_rx_maxlen(blt_pSecChnScn->rfLen_max);
        rf_set_rx_maxlen(255);


#if 0   //just for LL/DDI/SCN/BV-21-C,not report too much packet.EBQ not ability to process.now can set the EBQ periodic chain num to implement.
        //later maybe need to implement skip function.
        if( blt_pPdAsync->max_pda_skip && (blt_pPdAsync->skip_cnt++&0x01) ){
            systimer_set_irq_capture(clock_time()+ 30*SYSTEM_TIMER_TICK_1US);//after 30us,run blt_pda_sync_post,drop one time rx.
        }else{
            //systimer_set_irq_capture(bltSche.sSlot_tick_irq + blt_pSecChnScn->scan_duration_us*SYSTEM_TIMER_TICK_1US);
            systimer_set_irq_capture(bltSche.sSlot_tick_irq + blt_pPdAsync->pda_duration_us*SYSTEM_TIMER_TICK_1US);
        }
#else
        //systimer_set_irq_capture(bltSche.sSlot_tick_irq + blt_pSecChnScn->scan_duration_us*SYSTEM_TIMER_TICK_1US);
        systimer_set_irq_capture(bltSche.sSlot_tick_irq + blt_pPdAsync->pda_duration_us*SYSTEM_TIMER_TICK_1US);
#endif

        blmsParam.rf_fsm_busy = 1;

        //bltExtScn.prdAdv_syncFlg = 1;

        blt_pSecChnScn->tolerance_peer_us = blt_pPdAsync->tolerance_pda_us;
        blt_pSecChnScn->aux_expect_tick = bltSche.sSlot_tick_irq + blt_pPdAsync->sync_early_set_us*SYSTEM_TIMER_TICK_1US;

        static int mark = 0;
        if(!mark){
            my_dump_str_u32s(DBG_PDA_SYNC_TIMING, "first timing", blt_pSecChnScn->tolerance_peer_us, \
                              blt_pPdAsync->sync_early_set_us, bltPHYs.prmb_ac_us, 0);
        }
        mark = 1;

    }



    blt_pda_start_common_2();
    blt_pPdAsync->sSlot_mark_prdadv = bltSche.sSlot_idx_irq_real;

    /* logic setting executing after SRX setting to save time */

    auxScnCmnParam.rx_received = 0;

#if PDA_SYNC_EBQ
    if(blt_pPdAsync->sync_state == SYNC_STATE_SYNCED){
        blt_pPdAsync->sync_report_allow = 1;    // PERIODIC_ADVERTISING_REPORT must be later than PERIODIC_ADVERTISING_SYNC_ESTABLISHED
    }
#endif

    blms_state = BLMS_STATE_PDA_SYNC_S;
    systick_irq_trigger = SYS_IRQ_TRIG_PDA_SYNC_POST;


#if (PDA_SYNC_TIMING_ADJUST_EN)
    pda_sync_timingAdjust[index].rx_1st_tick = 0;
    pda_sync_timingAdjust[index].timing_update = 0;
#endif
    //first AUX
    blt_pSecChnScn->scan_rx_flag = 0;
    blt_pSecChnScn->aux_chain_flag = 0;
    blt_pSecChnScn->peerAdvA_exist = 0;
    blt_pSecChnScn->peerTargetA_exist = 0;
    blt_pSecChnScn->advrpt_hold_dat_len = 0;
    blt_pSecChnScn->perdAdv_interval  = 0;


    blt_pSecChnScn->aux_scan_cnt = 1;


    blt_pPda->paEvtCnt++;  //important here

#if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
    blt_pPdAsync->prePawrEvtCnt = blt_pPda->paEvtCnt;
#endif
}


_attribute_ram_code_
void blt_pda_sync_post(void)
{
#if (LL_FEATURE_ENABLE_LE_AOA_AOD)
    rf_set_aoa_aod_trx_mode(RF_AOA_OFF);
    cte_connLess_switchPattern[bltPdaSync.pdA_sync_sel].cte_rx_mode_en = 0;
    DBG_CHN10_LOW;
#endif

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_OFF); }

    if(blmsParam.rf_fsm_busy){

        #if 0
                /* timing extending function */
                if(blt_pSecChnScn->scan_duration_flag == DURATION_FLAG_MIN_TIME && rf_receiving_flag()){
                    //1M:        16B 128uS
                    //Coded S8   16B  1024uS
                    //TODO
                    //systimer_set_irq_capture(bltSche.sSlot_tick_irq + blt_pSecChnScn->scan_duration_us*SYSTEM_TIMER_TICK_1US);
                    return;
                }
        #endif

        STOP_RF_STATE_MACHINE;
        blmsParam.rf_fsm_busy = 0;
        blmsParam.delay_clear_rf_status = 1;
    }


    //blt_pPdAsync->pda_expect_tick = (auxScnCmnParam.rx_received ? bltRxPkt.rx_header_tick : blt_pPdAsync->pda_expect_tick) + blt_pPdAsync->pda_expect_tick;
    if(auxScnCmnParam.rx_received){
        blt_pPdAsync->sync_err_cnt = 0;
        blt_pPdAsync->sync_rx_tick = clock_time();
    }
    else{
        blt_pPdAsync->sync_err_cnt ++;

        if(blt_pPdAsync->sync_state == SYNC_STATE_SYNCED){
            if(blt_pPdAsync->sync_err_cnt > 10){
                blt_ll_setSchedulerTaskPriority(TSKOFT_PDA_SYNC + blt_pPdAsync->pda_index, TASK_PRIORITY_LOW);
                blt_pPdAsync->sync_err_cnt = 5;
            }
            else{
                blt_ll_incSchedulerTaskPriority(TSKOFT_PDA_SYNC + blt_pPdAsync->pda_index, bltPri.step_final[TSKOFT_PDA_SYNC + blt_pPdAsync->pda_index] );
            }
        }
    }

    if(blt_pPdAsync->sync_state == SYNC_STATE_SYNCING){
        if(auxScnCmnParam.rx_received){

            blt_pPdAsync->sync_state = SYNC_STATE_SYNCED;
            if(blt_pPdAsync->createSyncType == PDA_CREATE_SYNC_BY_PAST){
                blt_pPdAsync->sync_establish = SYNC_ESTABLISHED_BY_PAST;
                my_dump_str_data(0, "SYNC_ESTABLISHED_BY_PAST", 0, 0);
            }
            else{
                blt_pPdAsync->sync_establish = SYNC_ESTABLISHED_BY_HCICMD; //send sync_established event later and the status is successful.
            }

            blt_pPdAsync->bigInfor_para.creating_bisSync_flag=0;


            blmsParam.pda_syncing_flg = 0;
            bltPdaSync.tick_pda_sync = 0;
            pdaCache_tbl[blt_pPdAsync->mapping_cache_idx].cach_flag = CACHE_FLAG_SYNCED;

            #if (LL_FEATURE_ENABLE_LE_AOA_AOD)
                blt_pPdAsync->sync_cte_type = 0xFF;
            #endif

            //my_dump_str_data(DBG_PDA_SYNC_LOGIC, "pda sync established ", &blt_pPda->paEvtCnt, 2);
            my_dump_str_data(DBG_AOA_AOD_LOGIC, "pda sync established ", &blt_pPda->paEvtCnt, 2);

            /* 1000000 approximately equal to 1024*1024 = 2^20 */
            blt_pPdAsync->tolerance_pda_us = blt_pPdAsync->max_err_us_per_second*blt_pPdAsync->pda_interval*1250>>20;

            blt_pPdAsync->tolerance_pda_us += 30;  //maybe no need

            blt_pPdAsync->sync_early_set_us = blt_pPdAsync->tolerance_pda_us + EXTSCAN_PREPARE_US;
            u32 pda_expect_tick = bltRxPkt.rx_header_tick + blt_pPdAsync->pdaInterval_tick - blt_pPdAsync->sync_early_set_us*SYSTEM_TIMER_TICK_1US;

            blt_pPdAsync->pda_duration_us = blt_pPdAsync->sync_early_set_us + blt_pPdAsync->pda_pkt_us + PDASYNC_TAIL_MARGIN_US;
            blt_pPda->sSlot_duration_pda = (blt_pPdAsync->pda_duration_us + SLOT_PROCESS_MAX_US)*SSLOT_US_REVERSE;

            int n_sSlot;
            n_sSlot = (pda_expect_tick - bltSche.sSlot_tick_irq_real)*SSLOT_TICK_REVERSE;
            blt_pPdAsync->sSlot_mark_prdadv = bltSche.sSlot_idx_irq_real + n_sSlot - blt_pPda->sSlot_prdadv_itvl;

            //sSlot_mark convert to bSlot mark
            int bSlot_offset = blt_pPdAsync->sSlot_mark_prdadv>>5;
            blt_pPda->bSlot_mark_prdadv  = bltSche.bSlot_idx_start + bSlot_offset;

            #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
                if(blt_pPdAsync->pawr_acad_check){
                    blc_ll_switch2PAwR_syncSubevt0(blt_pPdAsync);//here only run one time when established.
                }
            #endif

            blt_sche_addUpdate(SLOT_UPDT_SLAVE_SYNC_DONE);
        }
        else{
            //SYNC_STATE_SYNCING and not receive PDA advertisement
            if(blt_pPdAsync->sync_err_cnt >= 6){//BLUETOOTH CORE SPECIFICATION Version 5.2 | Vol 1, Part F page 376---2.59
                if(blt_pPdAsync->createSyncType == PDA_CREATE_SYNC_BY_PAST){
                    blt_pPdAsync->sync_establish = SYNC_ESTABLISHED_BY_PAST;
                    my_dump_str_data(0, "SYNC_ESTABLISHED_BY_PAST: fail", 0, 0);
                }
                else{
                    blt_pPdAsync->sync_establish = SYNC_ESTABLISHED_BY_HCICMD; //send sync_established event later and the status is ESTABLISHED/SYNCHRONIZATION TIMEOUT(0x3E)
                    my_dump_str_data(0, "SYNC_ESTABLISHED_BY_HCICMD: fail", 0, 0);
                }

                blt_pPdAsync->sync_state = SYNC_STATE_IDLE;


                blt_pSecChnScn->occupied = 0;
                blt_pSecChnScn->pdaSync_flag = 0;

                pdaCache_tbl[blt_pPdAsync->mapping_cache_idx].cach_flag = CACHE_FLAG_IDLE;

                //the following need to be implemented according to blt_pda_syncing_finish.
                blmsParam.pda_syncing_flg = 0;
                bltPdaSync.tick_pda_sync = 0;
                /////////////////////////////////////

                blt_sche_disableTask(TSKMSK_PDA_SYNC_0<<blt_pPdAsync->pda_index);
                blmsParam.state_chng |= STATE_CHANGE_PDA_SYNC;
                blt_ll_setSchedulerTaskPriority(TSKOFT_PDA_SYNC + blt_pPdAsync->pda_index, TASK_PRIORITY_LOW);
            }else{
                #if(LL_CON_PER_BV98C_AND_CON_CEN_BV94C)
                    my_dump_str_data(0, "Extended sync window: sync NO.", &blt_pPdAsync->sync_err_cnt, 1);

                    blt_pPdAsync->sync_early_set_us += 60; //+60us step/per CI
                    #if(1) //EBQ's bug
                    /*
                     * The bug of EBQ is that the syncoffset is too large by about 400us. Here the anchor
                     * point moves in the positive direction, and the window can be expanded backward.
                     */
                        blt_pPdAsync->sync_early_set_us += 60; //+60us step/per CI
                        blt_pPdAsync->sSlot_mark_prdadv += 3; //+60us
                    #else
                        blt_pPdAsync->sync_early_set_us += 60; //+60us step/per CI
                        blt_pPdAsync->sSlot_mark_prdadv -= 3; //-60us , left -60 ~ right +60
                    #endif

                    //for tolerance, duration should add twice of it
                    blt_pPdAsync->pda_duration_us = 2*blt_pPdAsync->sync_early_set_us + blt_pPdAsync->pda_pkt_us + PDASYNC_TAIL_MARGIN_US;
                    blt_pPda->sSlot_duration_pda = (blt_pPdAsync->pda_duration_us + SLOT_PROCESS_MAX_US)*SSLOT_US_REVERSE;

                    blt_sche_addUpdate(SLOT_UPDT_SLAVE_SSLOT_ADJUST);
                #endif
            }
        }
    }
    else if(blt_pPdAsync->sync_state == SYNC_STATE_SYNCED){

        if(auxScnCmnParam.rx_received){
            //blt_pPdAsync->sync_rx_tick = clock_time();

            #if (PDA_SYNC_TIMING_ADJUST_EN)
                u8 syncHandle = bltPdaSync.pdA_sync_sel;

                if(pda_sync_timingAdjust[syncHandle].rx_1st_tick){
                    u32 tick_offset_1st_rx = pda_sync_timingAdjust[syncHandle].rx_1st_tick - bltSche.sSlot_tick_irq_real;
                    #if (BLMS_PM_ENABLE)
                        u32 tick_offset_expect = blmsParam.min_tolerance_us * SYSTEM_TIMER_TICK_1US + (BRX_EARLY_SET_TICK + BRX_HALF_MARGIN_TICK);
                    #else
                        u32 tick_offset_expect = BRX_EARLY_SET_TICK + BRX_HALF_MARGIN_TICK;
                    #endif
                        pda_sync_timingAdjust[syncHandle].sSlot_offset = (signed int)(tick_offset_1st_rx - tick_offset_expect)*SSLOT_TICK_REVERSE;

                    if(pda_sync_timingAdjust[syncHandle].sSlot_offset < -1 || pda_sync_timingAdjust[syncHandle].sSlot_offset > 2){
                        //DBG_C HN10_TOGGLE;
                        pda_sync_timingAdjust[syncHandle].timing_update = 1;
                        blt_sche_addUpdate(SLOT_UPDT_SLAVE_SSLOT_ADJUST);
                    }
                    else{
                        //DBG_C HN11_TOGGLE;
                    }

                    if(pda_sync_timingAdjust[syncHandle].timing_update){

                    #if 0
                        st_pda_sync_t *pPdA_sync;
                        st_pda_t *p_curPda;

                        pPdA_sync = (st_pda_sync_t *)&pdAsync_tbl[syncHandle];
                        p_curPda = (st_pda_t *) &pPdA_sync->pda_rx;

                        if(p_curPda->pda_phy == BLE_PHY_CODED){
                            blt_pPdAsync->pda_duration_us = 17040 + PDASYNC_TAIL_MARGIN_US + 20; ////biggest rfLen 255
                        }else{
                            blt_pPdAsync->pda_duration_us = 2120 + PDASYNC_TAIL_MARGIN_US + 20; ////biggest rfLen 255
                        }
                    #else
                        blt_pPdAsync->pda_duration_us = 4000;//3ms. Currently only consider 1M situation.
                    #endif
                        blt_pPda->sSlot_duration_pda = (blt_pPdAsync->pda_duration_us + SLOT_PROCESS_MAX_US)*SSLOT_US_REVERSE;
                    }
                }
            #endif
        }
        else if((u32)(clock_time() - blt_pPdAsync->sync_rx_tick) > blt_pPdAsync->sync_timeout_tick){

            blt_pPdAsync->sync_state = SYNC_STATE_IDLE;

            blt_pPdAsync->sync_lost = 1;
            blt_pPdAsync->sync_adv_dup_filter = 0xFFFF;

            blt_pSecChnScn->occupied = 0;
            //blt_release_secchn_scan(blt_pSecChnScn, bltExtScn.auxadv_sel);   //clear occupied
            blt_pSecChnScn->pdaSync_flag = 0;

            pdaCache_tbl[blt_pPdAsync->mapping_cache_idx].cach_flag = CACHE_FLAG_IDLE;

            blt_sche_disableTask(TSKMSK_PDA_SYNC_0<<blt_pPdAsync->pda_index);
            blmsParam.state_chng |= STATE_CHANGE_PDA_SYNC;

            /////////////////////////////////////
            blmsParam.pda_syncing_flg = 0;
            bltPdaSync.tick_pda_sync = 0;
        }
    }
    else{
        //debug
        if(blt_pPdAsync->sync_state != SYNC_STATE_IDLE){
            BLMS_ERR_DEBUG(DBG_PDA_SYNC_LOGIC, 0xFD010000 | blt_pPdAsync->sync_state);
        }
    }


    blt_ll_calculate_sSlot_next(clock_time() + SLOT_PROCESS_MAX_TICK);


    blms_state = BLMS_STATE_PDA_SYNC_E;
}


_attribute_ram_code_
void blt_pda_sync_rx(rf_pkt_ext_adv_t * pExtAdv)
{
    //u8 * raw_pkt;
    //rf_pkt_ext_adv_t * pExtAdv = (rf_pkt_ext_adv_t *) (raw_pkt + DMA_RFRX_LEN_HW_INFO);

    /* AdvA & TargetA & Sync Info can not exist,  CTE Info & Aux Ptr & Tx Power optional */
    if( pExtAdv->ext_hdr_len !=0 && ((pExtAdv->ext_hdr_flg & (EXTHD_BIT_ADVA | EXTHD_BIT_TARGETA | EXTHD_BIT_SYNC_INFO )) != 0) )
    {
        //TODO
    }


    u8 syncHandle = bltPdaSync.pdA_sync_sel;
    if(pda_sync_timingAdjust[syncHandle].rx_1st_tick){
        blt_pPda->lastPaAnchorPoint = pda_sync_timingAdjust[syncHandle].rx_1st_tick;
        blt_pPda->lastPaEvtCnt = blt_pPda->paEvtCnt - 1; //plus 1
    }

}


extern _attribute_aligned_(4) u8    extadv_pda_rpt_hold_data_buf[TSKNUM_SECCHN_SCAN][EXTADV_PDA_RPT_DATA_HOLD_MAX_LEN];


int         blt_ll_bigInfoAdvReport(unsigned char *raw_pkt){

    u8 syncHandle = *(u8*)(raw_pkt + DMA_RFRX_OFFSET_TIME_STAMP(raw_pkt));

    if(blt_isSyncHandleValid (syncHandle)==TRUE){

        u8 syncIdx = syncHandle & BLT_SYNC_IDX_MARK;

        st_pda_sync_t *pPda_sync = &pdAsync_tbl[syncIdx];
        bigInfo_t* pBigInfo = (bigInfo_t*)&pPda_sync->bigInfor_para.bigInfo;

        u8 sduItvl[3];
        u32 sduItvlUs = pBigInfo->sduItvl; //unit: us
        sduItvl[0] = U32_BYTE0(sduItvlUs);
        sduItvl[1] = U32_BYTE1(sduItvlUs);
        sduItvl[2] = U32_BYTE2(sduItvlUs);

        //BigInfo's PHY field use PHY Types: 0x00 LE 1M PHY; 0x01 LE 2M PHY; 0x02 LE Coded PHY, refer to Core5.2, Page2978 */
        u8 phy = ((pBigInfo->chm37Phy3[4]>>5)&0x07) + 1; //plus offset 1


        //LL/BIS/SNC/BI-02-C ,if Synchronized Receiver receive a unsupported PHY, IUT will not send BIG report event to host
        if(phy>3)
            return -1;

        u8 framing = (pBigInfo->bisPldCnt39Framing1[4]>>7) & 0x01;

        u8 encrypt = pPda_sync->bigInfor_para.bigEncrypt;

        if(hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_BIGINFO_ADVERTISING_REPORT)
        {
            //LE BIGInfo Adv Report event's PHY field use PHY: 0x01 LE 1M PHY; 0x02 LE 2M PHY; 0x03 LE Coded PHY, refer to Core5.2, Page2457
            hci_le_BigInfoAdvReport_evt(syncHandle, pBigInfo->numBis, pBigInfo->nse, pBigInfo->isoItvl, pBigInfo->bn, pBigInfo->pto, pBigInfo->irc,
                                        pBigInfo->maxPdu, sduItvl, pBigInfo->maxSdu, phy, framing, encrypt);
        }

        return 1;
    }

    return 0;
}


_attribute_no_inline_
int  blt_pda_sync_prdadv_data_report(void)
{
    u8 *pHolDataBuf = NULL;

    st_secchn_scn_t  *cur_pPdascan = NULL;
    rf_pkt_ext_adv_t *pPdaAdv      = NULL;
    le_periodAdvReportEvt_t* lePeriodAdvReportEvt = NULL;

    u8 temp_buff[256];  //process max length 255
    temp_buff[0] = HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT;

    u8* copySourceAddr = NULL;
    u8 aux_idx =0;
    //u8 rpt_2_exist = 0; //special case:when the holding data length is 2*PDAADV_RPT_DATA_LEN_MAX.but need 30 chain packet(247/8)--now not consider
    u8  new_advdat_len = 0; // < 256
    u16 new_advEvt_len = 0;

    u16 total_rptevt_len  = 0; //keep and clear
    u8 old_hold_data_len = 0;
    u8 new_hold_data_len = 0;

    u8 rpt_1_src_data_offset = 0; //report 1
    u8 rpt_2_src_data_offset = 0; //report 2, when
    u32 rpt_1_copy_data_len  = 0; //change to u8 at last

    u8* raw_pkt = (u8 *) (scan_secRxFifo.p + SCAN_SECCHN_RXFIFO_SIZE * (scan_secRxFifo.rptr & SCAN_SECCHN_RXFIFO_MASK));
    aux_idx  = raw_pkt[2]&(~SECCHN_IDX_MARK);    // index stored on raw_pkt[2]
    pPdaAdv  = (rf_pkt_ext_adv_t *) (raw_pkt + DMA_RFRX_LEN_HW_INFO);

    new_advdat_len = raw_pkt[1]; // raw_pkt[1] is pure ADV data length(not include header and extended header), already calculated in IRQ
    cur_pPdascan   = (st_secchn_scn_t *)&secChnScn_tbl[aux_idx];

    u16 extHdr_offset = 0;
    u16 adi_info = 0;

    /*
     * AdvA TargetA CTE_Info ADI Aux_Ptr Sync_Info Tx_Power ACAD AdvData
     *   X     X       O      O     O        X        O       O    O
     * AUX_SYNC_IND PDU
     */
    /* 1. CTE info can exist in "AUX_SYNC_IND" and it's "AUX_CHAIN_IND" */
    if(pPdaAdv->ext_hdr_len != 0){
        if(pPdaAdv->ext_hdr_flg & EXTHD_BIT_CTE_INFO){
            extHdr_offset += EXTHD_LEN_1_CTE;
        }
        /* 2. ADI */
        if(pPdaAdv->ext_hdr_flg & EXTHD_BIT_ADI){
            adi_info = *(u16 *)(pPdaAdv->data + extHdr_offset);  //have confirmed it's 2B aligned, can use "u16 *"
        }
    }

    u8 scanRx_flag = raw_pkt[3];

    if(scanRx_flag & SCANRX_FLAG_PDA){

        if(scanRx_flag & SCANRX_FLAG_DATA_DROP){
            return RTN_DROP;
        }

        st_pda_sync_t *pPdA_sync = (st_pda_sync_t*)&pdAsync_tbl[cur_pPdascan->pdaSync_idx];//bltPdaSync.pdA_sync_sel

        if(pPdA_sync->sync_establish != 0){//to avoid pda adv event report before pda established event.
            return RTN_BREAK;
        }

        if(pPdA_sync->terminate){

            cur_pPdascan->advrpt_hold_dat_len = 0;
            return RTN_DROP;
        }

        if(!(pPdA_sync->sync_rcv_enable & REPORTING_EN)){//if not allow to report
            return RTN_DROP;
        }

        #if (LL_FEATURE_ENABLE_LE_AOA_AOD)
            if(ll_aoa_aod_mlp_task_cb){
                ll_aoa_aod_mlp_task_cb(FLAG_AOA_AOD_CONNECTIONLESS_DATA_PROCESS); //blt_ll_aoa_aod_connectionless_data_process
            }
        #endif

        /////////////////////////////data hold process///////////////////////////////////////
        LASTPKT_HOLDDATA_NOTZERO:
        lePeriodAdvReportEvt = (le_periodAdvReportEvt_t *)&temp_buff[0];

        old_hold_data_len   = cur_pPdascan->advrpt_hold_dat_len;
        rpt_1_copy_data_len = new_advdat_len;
        new_advEvt_len      = PDAADV_INFO_LENGTH + new_advdat_len;
        total_rptevt_len += new_advEvt_len;

        if(old_hold_data_len){

            total_rptevt_len += old_hold_data_len;
            if(total_rptevt_len > 255){
                new_hold_data_len = total_rptevt_len - 255;
                total_rptevt_len = 255;
            }
        }
        else{
            if(total_rptevt_len > 255 ){

                new_hold_data_len = total_rptevt_len - 255;
                total_rptevt_len = 255;
            }
        }


        /*
         * spec description            :|-rfLen-|-extHdrLen!=0|-extHdrFlag-|--extHdr--|---AdvData---|
         *                             :|-rfLen-|-extHdrLen==0|--extHdr--|----------AdvData---------|
         * peerAdv_datOffset           :        |length from extHdrLen to the AdvDat|. the reference point is extHdrLen
         * struct 'rf_pkt_ext_adv_t'   :|-rfLen-|-extHdrLen-|-extHdrFlag-|--------data[253]-------|. data[] include extHdr and AdvData
         * rpt_1_src_data_offset, extHdrLen !=0 :                        |length from data[0] to AdvData|. the reference point is data[0]
         * rpt_1_src_data_offset, extHdrLen ==0 :           |length from extHdrFlag to AdvData|. the reference point is extHdrFlag.
         *
         * if extended header length != 0,peerAdv_datOffset - 2 , 2 indicate extHdrLen(1B) and extHdrFlag(1B).
         */
        rpt_1_src_data_offset = pPdaAdv->ext_hdr_len==0 ? 0 : (cur_pPdascan->peerAdv_datOffset - 2);

        if(old_hold_data_len || new_hold_data_len){

            pHolDataBuf = (u8 *)&extadv_pda_rpt_hold_data_buf[aux_idx][0];

            if(old_hold_data_len){
                smemcpy(lePeriodAdvReportEvt->data, pHolDataBuf, old_hold_data_len);
                new_advdat_len += old_hold_data_len;
            }

            if(new_hold_data_len){
                new_advdat_len = PDAADV_RPT_DATA_LEN_MAX;  //247, pay attention it changes here
                rpt_1_copy_data_len = PDAADV_RPT_DATA_LEN_MAX - old_hold_data_len;

                u8 pkt_hold_data_offset = rpt_2_src_data_offset = rpt_1_src_data_offset + rpt_1_copy_data_len;

                if(new_hold_data_len > PDAADV_RPT_DATA_LEN_MAX){  // > 247
                    /* eg. old hold data length 210 Byte, new data length 250byte,  250+24= 274,
                     * 274+215=489,  489-255=234, one hold packet not enough */
                    new_hold_data_len -= PDAADV_RPT_DATA_LEN_MAX;
                    pkt_hold_data_offset += PDAADV_RPT_DATA_LEN_MAX;
                    //rpt_2_exist = 1; //special case, now not consider
                }

                if(new_hold_data_len){ //can confirm that only sec_chn_scan trigger this
                    copySourceAddr = pPdaAdv->ext_hdr_len == 0 ? &pPdaAdv->ext_hdr_flg : pPdaAdv->data;
                    smemcpy(pHolDataBuf, copySourceAddr + pkt_hold_data_offset, new_hold_data_len);
                    cur_pPdascan->advrpt_hold_dat_len = new_hold_data_len; //pay attention: operate IRQ variable
                }
            }
        }
        if(!new_hold_data_len){
            cur_pPdascan->advrpt_hold_dat_len = 0; //if new_hold_data_len == 0, need to clear relevant variable.
        }

        copySourceAddr = pPdaAdv->ext_hdr_len == 0 ? &pPdaAdv->ext_hdr_flg : pPdaAdv->data;
        smemcpy(lePeriodAdvReportEvt->data + old_hold_data_len, copySourceAddr + rpt_1_src_data_offset, rpt_1_copy_data_len);
        //////////////////////////ending of hold data processing///////////////////

        lePeriodAdvReportEvt->tx_power    = 0x7F; //chain packet should be 0x7F--not available;LL/DDI/SCN/BV-79-C
        u8 lastPkt_holdData_flag = 0;
        if(scanRx_flag & SCANRX_FLAG_LAST_DATA){

            #if (PDA_SCAN_PENDING_FIX_EN)
            cur_pPdascan->scan_rx_flag &= ~SCANRX_FLAG_REPORT2HOST; //periodic adv's last packet need to clear the flag. aux_adv_ind not.
            #endif

            if(cur_pPdascan->advrpt_hold_dat_len){//LL/DDI/SCN/BV-79-C.If the last packet,but the hold data not zero,need to process this case.
                lePeriodAdvReportEvt->data_status = PDA_SYNC_REPORT_DATA_INCOMPLETE;
                lastPkt_holdData_flag = 1;
            }else{
                lePeriodAdvReportEvt->data_status = PDA_SYNC_REPORT_DATA_COMPLETE;
            }
        }
        else{
            lePeriodAdvReportEvt->data_status = PDA_SYNC_REPORT_DATA_INCOMPLETE;
            cur_pPdascan->scan_rx_flag |= SCANRX_FLAG_REPORT2HOST;
            if(scanRx_flag & SCANRX_FLAG_FIRST_DATA){
                lePeriodAdvReportEvt->tx_power    = cur_pPdascan->peerAdv_txPower;
            }
        }


        /////////////////////////////////////
        if(pPdA_sync->sync_rcv_enable & REPORTING_EN){ //needless, above already check this, remove latter

            //cause period adv report can support 248 bytes, whether or not to hold data.
            lePeriodAdvReportEvt->sync_handle = BLT_SYNC_HANDLE|cur_pPdascan->pdaSync_idx;//bltPdaSync.pdA_sync_sel;
            lePeriodAdvReportEvt->rssi        = raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)] - 110;

        #if (LL_FEATURE_ENABLE_LE_AOA_AOD)
            lePeriodAdvReportEvt->cte_type = pPdA_sync->sync_cte_type;
        #else
            lePeriodAdvReportEvt->cte_type = 0xff;  //default 0xff in 5.2
        #endif
            lePeriodAdvReportEvt->data_len    = total_rptevt_len - PDAADV_INFO_LENGTH;

            //prepare for truncated event. in case of hci not success.
            le_periodAdvReportEvt_t* pPdaScan_truncEvt = (le_periodAdvReportEvt_t*)&pdaScan_truncatedEvt[cur_pPdascan->pdaSync_idx];
            pPdaScan_truncEvt->rssi = lePeriodAdvReportEvt->rssi;
            pPdaScan_truncEvt->tx_power = lePeriodAdvReportEvt->tx_power;
            pPdaScan_truncEvt->sync_handle = lePeriodAdvReportEvt->sync_handle;
            ///////////////////////////
            if(pPdA_sync->sync_rcv_enable & DUPLICATE_FILTERING_EN){
                /*
                 * The Advertising Data ID (DID) is set by the advertiser to indicate to the scanner
                 * whether it can assume that the data contents in the AdvData are a duplicate of
                 * the previous AdvData sent in an earlier packet.
                 */
                u16 did = adi_info & 0xfff; /* DID:12bits | SID:4bits */
                if(pPdA_sync->sync_adv_dup_filter == did){
                    return RTN_DROP;
                }

                if(scanRx_flag & SCANRX_FLAG_LAST_DATA){ //the relevant chain packet not drop
                    pPdA_sync->sync_adv_dup_filter = did;
                }
            }
            //////////////////////////////////////////////////////////////////////
            if(hci_le_eventMask & HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_REPORT){
                if(BLE_SUCCESS != blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, temp_buff, total_rptevt_len)){
                    //we can make sure not run here---if( blc_hci_isHciTxFIFOfull() ).
                    //so not need to process truncate. if run here, code is error,need to debug.
                    my_dump_str_data(DBG_PDA_SYNC_LOGIC, "[pda scn]pda adv event send fail", 0, 0);
                }

                if(lastPkt_holdData_flag){
                    lastPkt_holdData_flag = 0;

                    new_advdat_len    = 0;//simulate rev new pkt,but the length is zero.
                    total_rptevt_len  = 0;
                    new_hold_data_len = 0;

                    goto LASTPKT_HOLDDATA_NOTZERO;
                }
            }
        } ///pPdA_sync->sync_rcv_enable & REPORTING_EN

        #if (LL_FEATURE_ENABLE_LE_AOA_AOD)
            // 7.7.65.21 LE Connectionless IQ Report event
            // If the PDU contains AdvData, then any HCI_LE_Periodic_Advertising_Report
            // event triggered by this PDU shall be generated before this event.
            if(ll_aoa_aod_mlp_task_cb){
                my_dump_str_u32s(DBG_AOA_AOD_LOGIC, "PDA_para", scanRx_flag, new_advdat_len, raw_pkt[5], raw_pkt[0]);
                ll_aoa_aod_mlp_task_cb(FLAG_AOA_AOD_CONNECTIONLESS_IQ_REPORT); //blt_ll_aoa_aod_connectionless_IQ_report
            }
        #endif
    } ///ending of (scanRx_flag & SCANRX_FLAG_PDA)


    /////////////////////////////////////
    if(scanRx_flag & SCANRX_FLAG_BIGINFOR){
        blt_ll_bigInfoAdvReport(raw_pkt);
    }
    return RTN_SUCCESS;
}


_attribute_noinline_
int blt_ll_procPdaScanTruncatedPend(void)
{
    if( !(hci_le_eventMask & HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_REPORT) ){
        return 0;
    }

    //process hold data. before send truncated event, need to send hold data to host.
    foreach(i, TSKNUM_SECCHN_SCAN){

        //here only process pda scan truncated pending.
        //not use 'scan_rx_flag' to pend,this method is error. later need to rewrite.
        if( !(secChnScn_tbl[i].scan_rx_flag & SCANRX_FLAG_PDASCAN_TRUNCATED_PEND) ){
            continue;
        }

        //if terminate, not send hold data or truncated event to host.
        u8 syncHandle = secChnScn_tbl[i].pdaSync_idx;
        st_pda_sync_t* pPdAsync = (st_pda_sync_t*)&pdAsync_tbl[syncHandle];
        if(pPdAsync->terminate){
            secChnScn_tbl[i].scan_rx_flag = 0;
            continue;
        }

        //step 1: if there are hold data, firstly send hold data to host. then send truncated event.
        u8 pdaScn_holdDatLen = secChnScn_tbl[i].advrpt_hold_dat_len;
        if( pdaScn_holdDatLen ){
            u8 pdaScn_holdDatOffset = secChnScn_tbl[i].advrpt_holdDataOffset;

            do{
                u8 temp_buff[256];
                pdaScn_holdDatLen = (pdaScn_holdDatLen > PDAADV_RPT_DATA_LEN_MAX) ? PDAADV_RPT_DATA_LEN_MAX : pdaScn_holdDatLen;

                le_periodAdvReportEvt_t* lePeriodAdvReportEvt = NULL;
                lePeriodAdvReportEvt = (le_periodAdvReportEvt_t *)&temp_buff[0];
                u8 *pHoldDataBuf = (u8 *)&extadv_pda_rpt_hold_data_buf[i][0];//246B

                smemcpy(lePeriodAdvReportEvt, &pdaScan_truncatedEvt[i], PDA_TRUNCATED_EVT_SIZE);//26

                lePeriodAdvReportEvt->sub_code    = HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT;
                lePeriodAdvReportEvt->sync_handle = BLT_SYNC_HANDLE|secChnScn_tbl[i].pdaSync_idx;
                lePeriodAdvReportEvt->cte_type    = 0xff;  //default 0xff in 5.2
                lePeriodAdvReportEvt->data_status = PDA_SYNC_REPORT_DATA_INCOMPLETE;

                lePeriodAdvReportEvt->data_len = pdaScn_holdDatLen;
                smemcpy(lePeriodAdvReportEvt->data, pHoldDataBuf + pdaScn_holdDatOffset, pdaScn_holdDatLen);

                /////later check
                if(0 == blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8*)&temp_buff, PDA_TRUNCATED_EVT_SIZE+pdaScn_holdDatLen) ){
                    if(pdaScn_holdDatLen > PDAADV_RPT_DATA_LEN_MAX){
                        pdaScn_holdDatLen -= PDAADV_RPT_DATA_LEN_MAX;
                        secChnScn_tbl[i].advrpt_hold_dat_len = pdaScn_holdDatLen;
                        secChnScn_tbl[i].advrpt_holdDataOffset= pdaScn_holdDatOffset = PDAADV_RPT_DATA_LEN_MAX;//hold data max number two.

                        my_dump_str_data(DBG_PDA_SYNC_LOGIC, "[pda scn]holdData need 2 times", 0, 0);
                    }else{
                        secChnScn_tbl[i].advrpt_hold_dat_len = pdaScn_holdDatLen = 0;
                        secChnScn_tbl[i].advrpt_holdDataOffset = pdaScn_holdDatOffset = 0;
                        my_dump_str_data(DBG_PDA_SYNC_LOGIC, "[pda scn]holdData need 1 time", 0, 0);
                    }
                }else{
                    //UART FIFO full, wait next mainloop to process. not run the following code,because next code also require UART FIFO.
                    return 1;
                }

            }while(pdaScn_holdDatLen);
        }

        //////////////////////////////////////////////////////////////
        //step 2: process truncated event
        pdaScan_truncatedEvt[i].sub_code    = HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT;
        pdaScan_truncatedEvt[i].cte_type    = 0xFF;//0:AOA; 1:AOD_1US; 2:AOD_2US; 0xFF:no CTE
        pdaScan_truncatedEvt[i].data_status = PDA_SYNC_REPORT_DATA_TRUNCATED;
        pdaScan_truncatedEvt[i].data_len    = 0; //clear adv report evt's dataLen
        pdaScan_truncatedEvt[i].sync_handle = (BLT_SYNC_HANDLE|secChnScn_tbl[i].pdaSync_idx);

        if(0 == blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8*)&pdaScan_truncatedEvt[i], PDA_TRUNCATED_EVT_SIZE) )
        {
            secChnScn_tbl[i].scan_rx_flag = 0;
            ///periodic can not release resource here.so not call API --- blt_set_auxscan_enable( (st_secchn_scn_t*)&secChnScn_tbl[i], 0);
            my_dump_str_data(DBG_PDA_SYNC_LOGIC, "[pda scn]truncated pend release", (u8*)&pdaScan_truncatedEvt[i], PDA_TRUNCATED_EVT_SIZE);
        }
    }

    return 1;
}


/**
 * @brief      This function is used to enable or disable reports for the periodic advertising train
 *             identified by the Sync_Handle parameter
 * @param[in]  sync_handle - Sync_Handle identifying the periodic advertising train
 * @param[in]  enable
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
_attribute_noinline_
ble_sts_t   blc_ll_periodicAdvertisingReceiveEnable (u16 sync_handle, sync_adv_rcv_en_msk enable)
{
    //HCI/GEV/BV-03-C [Disallow Mixing Legacy and Extended Scanning Commands]
    if(IS_LEGACY_SCAN_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_EXTENDED_SCAN_VALID;

    /* If the periodic advertising train corresponding to the Sync_Handle parameter does not exist,
     * then the Controller shall return the error code Unknown Advertising Identifier (0x42). */
    if(blt_isSyncHandleValid(sync_handle)){
        /* BIT(0): Reporting enabled
            REPORTING_EN   = BIT(0),
            REPORTING_DIS  = 0,
           BIT(1): Duplicate filtering enabled
            DUPLICATE_FILTERING_EN   = BIT(1),
            DUPLICATE_FILTERING_DIS  = 0,
        */
        if(enable > SYNC_ADV_RCV_EN_MSK){
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
        pdAsync_tbl[sync_handle & BLT_SYNC_IDX_MARK].sync_rcv_enable = enable;
    }
    else{
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }

    return BLE_SUCCESS;
}




/**
 * @brief      This function is used to enable or disable reports for the periodic advertising train
 *             identified by the Sync_Handle parameter
 * @param[in]  sync_handle - Sync_Handle identifying the periodic advertising train
 * @param[in]  enable
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t   blc_hci_le_periodicAdvertisingReceiveEn(hci_le_setPeriodicAdvReceiveEnCmdParams_t *cmdPara)
{
    return blc_ll_periodicAdvertisingReceiveEnable(cmdPara->syncHandle, cmdPara->enable);
}




#endif
