/********************************************************************************************************
 * @file    scan.c
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

#if (LL_ACL_CEN_EN || LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)

#define         NUM_OF_SCAN_DEVICE              16

#if (NEW_SCAN_DEVICE_STORAGE)
    _attribute_ble_data_retention_  _attribute_aligned_(4)  scan_adv_t blt_scan_device[NUM_OF_SCAN_DEVICE];
#else
    _attribute_ble_data_retention_  _attribute_aligned_(4)  u8 blt_scan_device[NUM_OF_SCAN_DEVICE][8];
#endif

#if(!SCAN_BACKOFF_FEATURE_EN)
#define         NUM_OF_SCAN_RSP_DEVICE          8
_attribute_ble_data_retention_  _attribute_aligned_(4)  u8 blt_scanRsp_device[NUM_OF_SCAN_RSP_DEVICE][8];
#endif


_attribute_ble_data_retention_  _attribute_aligned_(4)  ll_scn_t  bltScn;

#if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN)
    _attribute_ble_data_retention_  _attribute_aligned_(4)  ll_scn_align_t  bltAlignScn;
#endif

/*
 * only when brx_end/btx_end/scan_end and primary scan buffer is empty can entry low power.
 * so _attribute_data_no_init_ is safe.
 */

_attribute_ble_data_retention_      _attribute_aligned_(4)  u8 scan_pri_chn_rx_fifo[ SCAN_PRICHN_RXFIFO_SIZE * SCAN_PRICHN_RXFIFO_NUM]; //scan primary channel RX FIFO

_attribute_ble_data_retention_  _attribute_aligned_(4)  st_prichn_scn_t         priChnScn_tbl[SCAN_PRICHN_RXFIFO_NUM];

_attribute_ble_data_retention_  _attribute_aligned_(4)  st_prichn_scn_t         *blt_pPrichnScn = NULL;


_attribute_ble_data_retention_  _attribute_aligned_(4)  ll_rx_pkt_callback_t    ll_prichn_initPkt_cb = NULL;

_attribute_ble_data_retention_  _attribute_aligned_(4)  ll_rx_pkt_callback_t    ll_extadv_pkt_cb = NULL;



/* SCAN_REQ and AUX_SCAN_REQ RF format are all same, only difference is: one in primary channel, the other is in secondary channel */
_attribute_ble_data_retention_
rf_packet_scan_req_t    pkt_scanReq = {
        rf_tx_packet_dma_len(12 + 2),           //dma_len
        LL_TYPE_SCAN_REQ,                       // type  LL_TYPE_SCAN_REQ and LL_TYPE_AUX_SCAN_REQ both 0x03
        0,                                      // RFU
        0,                                      // ChSel: only valid in ADV_IND/ADV_DIRECT_IND/CONNECT_IND, other packet set 0'b
        0,                                      // txAddr
        0,                                      // rxAddr

        12,                                     // rf_len
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,},  // scanA
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,},   //advA
};




_attribute_ram_code_
int blt_prichn_scan_interrupt_task (int flag)
{
    if(flag & FLAG_SCHEDULE_START){
        blt_set_prichn_scan_start();
    }
    else if(flag & FLAG_SCHEDULE_DONE){
        blt_ll_prichn_scan_post();
    }
    else if(flag & FLAG_SCHEDULE_BUILD){
        return blt_ll_buildPrimaryChannelScanTask();
    }
    else if(flag & FLAG_SCHEDULE_PRICHN_SCAN_INSERT){
        return blt_ll_prichn_scan_insert();
    }
#if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN)
    else if(flag & FLAG_SCHEDULE_PRICHN_SCAN_ALIGN_BUILD){
        blt_ll_prichn_scan_align_build();
    }
#endif

    return 0;
}


void blt_ll_initScanningCommon(void)
{

    #if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_scn_t)), scan);
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(st_prichn_scn_t)), scan);
    #endif

    ll_irq_scan_rx_pri_chn_cb = irq_scan_rx_primary_channel;
    ll_prichn_scan_irq_task_cb = blt_prichn_scan_interrupt_task;

    smemcpy(pkt_scanReq.scanA, bltMac.macAddress_public, BLE_ADDR_LEN);

    //Primary Channel Scan RX buffer initialize
    scan_priRxFifo.p = (u8 *)scan_pri_chn_rx_fifo;
    scan_priRxFifo.rptr = scan_priRxFifo.wptr = 0;

    bltScn.scan_rx_pri_chn_dma_buff = (u32)scan_priRxFifo.p;



    bltScn.bSlot_mark_scan = (u32)-BIT(20);

    for(int i=0;i<SCNTSK_FIFO_NUM;i++){
        bltScn.scnTsk_fifo[i].scheTask_oft = TSKOFT_PRICHN_SCAN;
        bltScn.scnTsk_fifo[i].scheTask_flg = TSKFLG_PRICHN_SCAN | TSKFLG_BSLOT_ALIGN;
    }

    #if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN)
        for(int i=0;i<SCNALIGN_FIFO_NUM;i++){
            bltAlignScn.scnAlign_fifo[i].scheTask_oft = TSKOFT_PRICHN_SCAN;
            bltAlignScn.scnAlign_fifo[i].scheTask_flg = TSKFLG_SCAN_ALIGN | TSKFLG_BSLOT_ALIGN;
        }
    #endif

    blt_ll_setSchedulerTaskPriority( TSKOFT_PRICHN_SCAN, TASK_PRIORITY_LOW );  //primary scan default priority very low


    blt_set_scan_default();
}



_attribute_noinline_
void blt_set_scan_default(void)
{
    /* if user forget to set Scan parameters, default value: */
    bltScn.chn_index = CHN_INDEX_INIT;
    bltScn.phy_index = PHY_INDEX_1M;
    bltScn.scanPhy_msk = SCAN_PHY_1M;
    bltScn.scan_type = SCAN_TYPE_PASSIVE;
    bltScn.scan_filterPolicy = SCAN_FP_ALLOW_ADV_ANY;
    bltScn.scan_percent = 128;
    bltScn.scanInterval = SCAN_INTERVAL_100MS;
    bltScn.scnInterval_tick = SCAN_INTERVAL_100MS * SYSTEM_TIMER_TICK_625US - 1000*SYSTEM_TIMER_TICK_1US;

    bltScn.early_stop_tick = 0;
    bltScn.last_scan_end_time = 0;
    bltScn.ownAddrType = 0;

    bltScn.scan_ownAddr_random = 0;
}






#if (NEW_SCAN_DEVICE_STORAGE)

scan_adv_t* blt_ll_filterAdvDevice (u8 type, u8 * mac)
{
    if (!mac)
    {
        bltScn.duplicate_filter = type;
        bltScn.scanDevice_num = 0;
        return NULL;
    }

    if (bltScn.duplicate_filter)
    {
        for (int i=0; i<bltScn.scanDevice_num; i++)
        {
            if (type == blt_scan_device[i].adr_type && smemcmp (mac, blt_scan_device[i].addr, BLE_ADDR_LEN) == 0)
            {
                return NULL;
            }
        }
    }



    if (bltScn.scanDevice_num >= NUM_OF_SCAN_DEVICE)
    {
        bltScn.scanDevice_num = NUM_OF_SCAN_DEVICE - 1;
        smemcpy (&blt_scan_device[0], &blt_scan_device[1], sizeof(scan_adv_t) * (NUM_OF_SCAN_DEVICE - 1));
    }
    blt_scan_device[bltScn.scanDevice_num].adr_type = type;
    smemcpy (blt_scan_device[bltScn.scanDevice_num].addr, mac, BLE_ADDR_LEN);
    bltScn.scanDevice_num++;

    return 0;
}


#else


int blt_ll_filterAdvDevice (u8 type, u8 * mac)
{
    if (!mac)
    {
        bltScn.duplicate_filter = type;
        bltScn.scanDevice_num = 0;
        return 0;
    }
    if (!bltScn.duplicate_filter)
    {
        return 0;
    }
    for (int i=0; i<bltScn.scanDevice_num; i++)
    {
        if (blt_scan_device[i][0] && type == blt_scan_device[i][1] && smemcmp (mac, blt_scan_device[i] + 2, BLE_ADDR_LEN) == 0)
        {
            return 1;
        }
    }
    if (bltScn.scanDevice_num >= NUM_OF_SCAN_DEVICE)
    {
        bltScn.scanDevice_num = NUM_OF_SCAN_DEVICE - 1;
        smemcpy (blt_scan_device[0], blt_scan_device[1], 8 * (NUM_OF_SCAN_DEVICE - 1));
    }
    blt_scan_device[bltScn.scanDevice_num][0] = 1;
    blt_scan_device[bltScn.scanDevice_num][1] = type;
    smemcpy (blt_scan_device[bltScn.scanDevice_num] + 2, mac, BLE_ADDR_LEN);
    bltScn.scanDevice_num++;

    return 0;
}

#endif


#if(SCAN_BACKOFF_FEATURE_EN)
/**
 * <<Core5.2>> Refer to Vol6, Part B, Section 4.4.3.2. Active scanning
 *  backoff procedure, note: BLE4.0 already has scan backoff feature.
 */
#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
void blt_ll_scanReqBackoff(ll_scn_t *pScanCB, bool rcvRspSts)
{
    assert(pScanCB->backoffCount == 0);

    if(rcvRspSts == TRUE){
        pScanCB->scnRspFail = 0;
        if(++pScanCB->scnRspSucc == 2){
            pScanCB->scnRspSucc = 0;
            pScanCB->upperLimit >>= 1;
            if(pScanCB->upperLimit == 0){
                pScanCB->upperLimit = 1;
            }
            //my_dump_str_data(STACK_DUMP_EN, "succ num >= 2", 0, 0);
        }
        //my_dump_str_data(STACK_DUMP_EN, "Succ upperLimit", &pScanCB->upperLimit, 2);
    }
    else{
        pScanCB->scnRspSucc = 0;
        if(++pScanCB->scnRspFail >= 2){
            pScanCB->scnRspFail = 0;
            pScanCB->upperLimit <<= 1;
            if (pScanCB->upperLimit > 256) {
                pScanCB->upperLimit = 256;
            }
            //my_dump_str_data(STACK_DUMP_EN, "fail num >= 2", 0, 0);
        }
        //my_dump_str_data(STACK_DUMP_EN, "fail upperLimit", &pScanCB->upperLimit, 2);
    }

    /* backoffCount = [1..upperLimit] */
//  u16 random = rand()&0xffff;
//  my_dump_str_data(STACK_DUMP_EN, "random", &random, 2);
//  pScanCB->backoffCount = (random & (pScanCB->upperLimit - 1)) + 1;
    pScanCB->backoffCount = ((u16)rand() & (pScanCB->upperLimit - 1)) + 1;
    //my_dump_str_data(STACK_DUMP_EN, "backoffCount", &pScanCB->backoffCount, 2);

    assert(pScanCB->backoffCount <= 256);
}

#else

#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
int blt_ll_addScanRspDevice(u8 type, u8 *mac)
{
    if (bltScn.scanRspDevice_num >= NUM_OF_SCAN_RSP_DEVICE)
    {
        bltScn.scanRspDevice_num = NUM_OF_SCAN_RSP_DEVICE - 1;
        smemcpy (blt_scanRsp_device[0], blt_scanRsp_device[1], 8 * (NUM_OF_SCAN_RSP_DEVICE - 1));
    }
    blt_scanRsp_device[bltScn.scanRspDevice_num][1] = type;
    smemcpy (blt_scanRsp_device[bltScn.scanRspDevice_num] + 2, mac, BLE_ADDR_LEN);
    bltScn.scanRspDevice_num++;

    return 1;
}

void blt_ll_clearScanRspDevice(void)
{
    bltScn.scanRspDevice_num = 0;
}


_attribute_ram_code_ bool blt_ll_isScanRspReceived(u8 type, u8 *mac)
{
    u16 *mac16= (u16 *)mac;

    for (int i=0; i<bltScn.scanRspDevice_num; i++)
    {
        u16 *dev16 = (u16*)(blt_scanRsp_device[i] + 2);
        if (type == blt_scanRsp_device[i][1] && MAC_MATCH16(mac16, dev16))
        {
            return 1;
        }
    }


    return 0;
}
#endif


_attribute_ram_code_ void blt_setScan_cal_chn_phy (int next_chn)
{
    int scan_window_hit;
    if(bltScn.chn_index == CHN_INDEX_INIT){
        scan_window_hit = 1;
    }
    else{
        scan_window_hit = (u32)(clock_time() - bltScn.tick_scan) > bltScn.scnInterval_tick;
    }

    if (scan_window_hit){
        bltScn.tick_scan = clock_time ();
    }

    if(scan_window_hit || next_chn){
        bltScn.chn_index ++;
        DBG_SIHUI_CHN11_TOGGLE;
        if(bltScn.chn_index == 3){ //channel 37/38/39 executed, jump to next PHY
            bltScn.chn_index = 0;
        }

    #if (0) //The current processing method will affect the connection. not find good solution yet, later will process--todo qiuwei.
        //When set 1M PHY and Coded PHY at the same time,during primary scan,the PHY switch should be random, can not be regular.
        //Avoid scan PHY and peer advertise PHY are always different, that not scan any advertise packets.
        //If only set one mask bit, i.e. only use 1M or Coded, not need to use random.
        if( ( (bltScn.scanPhy_msk&SCAN_PHY_1M_CODED) == SCAN_PHY_1M_CODED) && ((rand()&0x01) == 0) ){ //LL/DDI/SCN/BV-23-C;;LL/DDI/SCN/BV-62-C
            return ;
        }
    #endif
        if(bltScn.chn_index == 0 && (IS_EXTENDED_SCAN_VALID))
        { //extended_scan
            /* jump to next PHY*/
            do{
                bltScn.phy_index = (bltScn.phy_index + 1)%3;
            }while( (bltScn.scanPhy_msk & BIT(bltScn.phy_index)) == 0);

            bltScn.scan_type = bltScn.extScanType[bltScn.phy_index];
            bltScn.scan_percent = bltScn.scanPercent[bltScn.phy_index];
            bltScn.scanInterval = bltScn.scanInter[bltScn.phy_index];
            bltScn.scnInterval_tick = bltScn.scanInte_tick[bltScn.phy_index];
        }
    }

#if 0  //debug 1M/Coded PHY timing and channel
    if(bltScn.phy_index == PHY_INDEX_1M){
        DBG_C HN4_TOGGLE;
    }
    else{
        DBG_C HN5_TOGGLE;
    }
    if(bltScn.chn_index == 0){
        DBG_C HN6_TOGGLE;
    }
    else if(bltScn.chn_index == 1){
        DBG_C HN7_TOGGLE;
    }
    else if(bltScn.chn_index == 2){
        DBG_C HN8_TOGGLE;
    }
#endif
}


_attribute_ram_code_ void blt_setScan_enter_manual_rx(void)
{

#if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
    if(ll_phy_switch_cb){
        u8 new_phy_type = bltScn.phy_index + 1; //phy_index + 1 -> "le_phy_type_t"
        ll_phy_switch_cb(new_phy_type, LE_CODED_S8); ///rf_ble_switch_phy
    }
#endif

    u8 chn;

    if(blmsParam.scanInitEn_union.leg_scan_init_en)
    {
        chn = blc_legadv_channel[bltScn.chn_index];
    }
    else
    {
#if (LL_ASYNC_LEA_EN)
        if(bltScn.asyncScanIndex)
        {
            chn = blc_asyncAdv_channel[bltScn.chn_index];
        }
        else
        {
            chn = blc_extadv_channel[bltScn.chn_index];
        }
#else
        chn = blc_extadv_channel[bltScn.chn_index];
#endif

    }

    rf_set_tx_rx_off ();
    rf_set_ble_channel (chn);
    rf_set_ble_access_code_adv ();
    rf_set_ble_crc_adv ();

    blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_CUSTOMER_OR_DEFAULT, 0);

    rf_set_rx_maxlen(37);  //legADV max data length 37, extADV on primary channel data length smaller than 37

    rf_ble_set_rx_settle(RX_SETTLE_US);
    rf_ble_csem_set_tx_rx_settle(0, 0, RX_SETTLE_US);

#if(LL_FEATURE_ENABLE_LE_CODED_PHY)
    rf_trigger_codedPhy_accesscode();
#endif

    //Switch dma rx buffer to SCAN's dma rx buffer
    ble_rf_set_rx_dma((u8*)bltScn.scan_rx_pri_chn_dma_buff, SCAN_PRICHN_RX_DMA_SIZE);


    CLEAR_ALL_RFIRQ_STATUS;
    blmsParam.rf_fsm_busy = 1;
    rf_set_rxmode ();

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON); }

}





_attribute_ram_code_ u32 blt_quick_tx_prepare(fsm_mode_e tx_mode, void* addr, u8 rf_len)
{
    /* Different process for different MCU: ******************************************/
    ble_rf_set_tx_dma(0, 17);
     //  This register exists in Kite/Vulture/826x, riscv architecture chips such as
     //  Eagle do not have this register, but DMA fifo can be turned off by forcibly
     //  setting DMA TX rptr = DMA TX wptr to use DMA default tx fifo.If the hardware
     //  TX rptr of DMA TX fifo == hardware TX wptr, then send DMA Tx default fifo,
     //  otherwise send DMA Tx fifo non-default area.
    HAL_REG_RF_DMA_FIFO_TX_RPTR = FLD_DMA_RPTR_CLR;
    /**********************************************************************************/

    /* make sure PHY switch before using "bltPHYs" */
    rf_ble_set_tx_settle(bltPHYs.tx_stl_tifs);
    rf_ble_csem_set_tx_rx_settle(0, bltPHYs.tx_stl_tifs, RX_SETTLE_US); //make sure old FSM state machine exit clearly before calling "blt quick_tx_prepare"
    STOP_RF_STATE_MACHINE;
    u32 t = hal_rf_get_rx_timestamp() + (rf_len*bltPHYs.peer_oneByte_us + bltPHYs.TIFS_offset_us) *SYSTEM_TIMER_TICK_1US;
    u32 diff = t - clock_time();
    if( diff > BIT(30) ){  //
        t = clock_time();
    }

    rf_start_fsm (tx_mode, addr, t);
//  reg_rf_irq_status = 0xFFFF; //clear all status
    rf_clr_irq_status(FLD_RF_IRQ_ALL); //add by Yafei,240530
    return t;
//  if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_TX_ON);  }
}






/*  0x00    0.1. Accept all none_direct ADV
 *          0.2. Accept directed ADV addressed to this device
 *
 *  0x01    1.1. Accept all none_direct ADV where advA is in whiteList
 *          1.2. Accept directed ADV addressed to this device
 *
 *  0x02    2.1. Accept all none_direct ADV
 *          2.2. Accept directed ADV addressed to this device
 *          2.3. Accept directed ADV where initA(targetA) is a RPA(regardless of resolving result success or fail)
 *
 *  0x03    3.1. Accept all none_direct ADV where advA is in whiteList
 *          3.2. Accept directed ADV addressed to this device
 *          3.3. Accept directed ADV where initA(targetA) is a RPA(regardless of resolving result success or fail) */




#define ACTIVE_SCAN_TX_SET                      BIT(0)

/* On every received ADV_IND, ADV_SCAN_IND, or scannable AUX_ADV_IND
 * PDU that is allowed by the scanner filter policy and for which a scan request
 * PDU is to be sent, the backoffCount is decremented by one until it reaches the
 * value of zero. The scan request PDU is only sent when backoffCount becomes zero. */
#define ACTIVE_SCAN_BACKOFF_CHANCE              BIT(1)

#define ACTIVE_SCAN_FINAL_EXECUTE               BIT(2)
#define ACTIVE_SCAN_SUCCESS                     BIT(3)


_attribute_ram_code_ int irq_scan_rx_primary_channel(void) //blt_ext_adv_rx_process
{
    u8  raw_fifo_idx = (scan_priRxFifo.wptr++ & SCAN_PRICHN_RXFIFO_MASK);
#if 1 //optimize, to save RamCode
    u8* raw_pkt = ble_curr_rx_dma_buff;
#else
    u8* raw_pkt = (u8 *)(scan_priRxFifo.p + raw_fifo_idx * SCAN_PRICHN_RXFIFO_SIZE);
#endif

    /*
     * When FIFO is full, for example, rptr point to FIFO_0, wptr point to FIFO_0.
     * we abandon the oldest packet FIFO_0 and rptr++ to point to the next buffer FIFO_1. wptr point to this abandon buffer FIFO_0.
     * The destination is to make sure: rptr and wptr point to the different FIFO. that can be sure the FIFO being processed in mainloop
     *                                  will not be affected by RF receive.
     * For detailed explanation, please refer to the description "Scan Data Rf_len error            Summarized by SiHui 20221105"
     */
    volatile u8 tFIFO_full_flag = 0;
    if(((u8)(scan_priRxFifo.wptr - scan_priRxFifo.rptr) & 63)  >= SCAN_PRICHN_RXFIFO_NUM){
        scan_priRxFifo.rptr++; //make sure rptr and wptr point different fifo.
        tFIFO_full_flag = 1;
    }

    u8 * new_pkt = (u8 *)(scan_priRxFifo.p + (scan_priRxFifo.wptr & SCAN_PRICHN_RXFIFO_MASK) * SCAN_PRICHN_RXFIFO_SIZE);

    HAL_CLEAR_RF_RX_IRQ;


    #if (BQB_TEST_EN && SIHUI_FILTER_RSSI)  //for debug, remove other device ADV packet
        u8 rssi = raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)];
        if(rssi < 50){  // -60
            scan_priRxFifo.wptr --;
            return 0;
        }
        else{
            //my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "rssi", &rssi, 1);
        }
    #endif


    bltScn.scan_rx_pri_chn_dma_buff = (u32)new_pkt;
    ble_rf_set_rx_dma((u8*)bltScn.scan_rx_pri_chn_dma_buff, SCAN_PRICHN_RX_DMA_SIZE);


    int next_buffer = 0;
    raw_pkt[2] = 0;   //for data mark
    u8 rflen = raw_pkt[DMA_RFRX_OFFSET_RFLEN];


    /* "rx header tick" none zero is under condition CRC correct, so here do not check CRC by
       "RF_BLE_PACKET_VALIDITY_CHECK" to save RamCode and running timing
       Or we can use "bltRxPkt.crc correct" */
    if(bltRxPkt.rx_header_tick && (rflen <= 37) && (rflen >= 6))
    {
        //DBG_QIUWEI_CHN3_TOGGLE;
        rf_packet_adv_t * pAdv = (rf_packet_adv_t *) (raw_pkt + DMA_RFRX_LEN_HW_INFO);

        u32 advType_mask = BIT(pAdv->type);

        //optimize later: SiHui & QiuWei, save timing for TIFS, move to other place
        /* for both Legacy and Non_Connectable Non_Scannable without auxiliary packet,
         * should get prepared before "blt_ext_adv_rx_process" */
        blt_pPrichnScn = (st_prichn_scn_t *)&priChnScn_tbl[raw_fifo_idx];
        smemset4(blt_pPrichnScn, 0, sizeof(st_prichn_scn_t));//need to clear the value before

        bltScn.direct_adv = 0;
        if(advType_mask == TYPE_MASK_ADV_DIRECT_IND ){
            if(rflen == 12){
                bltScn.direct_adv = 1;
            }
            else{ //error ADV_DIRECT_IND
                advType_mask = 0;  //can not enter later branch
            }
        }


        if(advType_mask == TYPE_MASK_EXT_ADV)   /* ADV_EXT_IND */
        {
            my_dump_str_data(DBG_AUXSCAN_LOGIC_QW, "rev ext adv", 0, 0);

            if(ll_extadv_pkt_cb && (IS_EXTENDED_SCAN_VALID))  //must judge "ll_extadv_pkt_cb", can not save
            {
                #if (SLEV_primary_rx_extAdv)
                log_event_irq(SL_STACK_EXTSCAN_BASIC_TIMING_EN, SLEV_primary_rx_extAdv);
                #endif
                next_buffer = ll_extadv_pkt_cb(raw_pkt);  // blt_ext_adv_rx_process
            }
        }
        /* ADV_IND / ADV_DIRECT_IND / ADV_NONCONN_IND / ADV_SCAN_IND */
        else if(advType_mask & (TYPE_MASK_ADV_IND | TYPE_MASK_ADV_DIRECT_IND | TYPE_MASK_ADV_NONCONN_IND | TYPE_MASK_ADV_SCAN_IND) )
        {

            #if (SLEV_primary_rx_legAdv)
            log_event_irq(SL_STACK_EXTSCAN_BASIC_TIMING_EN, SLEV_primary_rx_legAdv);
            #endif

            if(bltScn.initiate_going && (advType_mask & (TYPE_MASK_ADV_IND | TYPE_MASK_ADV_DIRECT_IND)))
            {
                //if(ll_prichn_initPkt_cb) //not judge to save RamCode, "initiate_going" can guarantee
                {
                    if(ll_prichn_initPkt_cb(raw_pkt)){  // blt_prichn_procInitPkt
                        //initiate connection successfully
                        bltSche.sche_process_en = 1;
                        blmsParam.create_connection = 0;
                        bltScn.initiate_going = 0;

                        blms_state = BLMS_STATE_PRICHN_SCAN_E; //manual set Scan_post stage, very important!!!
                        systimer_clr_irq_status();
                        systimer_set_irq_capture(clock_time () + BIT(29));

                        #if (DYNAMIC_SCHE_CAL_TIME_EN)
                            bltSche.sche_tick_begin = clock_time();
                        #endif
                    }
                    else{//fail
                        if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON); }
                    }
                }
            }
            else
            {
                u8 activeScan_bit = 0;
                /* step 1. determine if a potential scan_req will be send with shortest timing, cause 150uS is very urgent.
                 * should use some basic logic variables or state, e.g. bltScn.scan_type, advType_mask.
                 * some logic which cost more timing should be judged after step 2(prepare TX FSM),
                 * e.g. whiteList filter, resolving RPA.
                 *
                 * tx_type & scanA 7 bytes will fill in this packet later, for timing urgent, filling action should ASAP
                 * consider RPA issue, filling should after resolving resolve because address may come from RL
                 * So after TX prepare we do RL/RPA/accept list first, then fill in pkt_scanReq, then check some other
                 * hardware & software logic which may stop scan_req sending.
                 * */
                if((bltScn.scan_type == SCAN_TYPE_ACTIVE) && (advType_mask & (TYPE_MASK_ADV_IND | TYPE_MASK_ADV_SCAN_IND)))
                {
                    /* prepare TX FSM quickly due to 150uS urgent timing */
                    rf_ble_tx_on();/* should set STX schedule timing ASAP */

                    /* For CSEM IP, need special process to disable RX continue mode, carefully! */
                    rf_ble_csem_close_rx_continue_mode();
                    /* Here add reset baseband for CSEM IP to keep RF mode safe, but add some delay before triggering STX mode, carefully! */
                    HAL_CSEM_IP_RESET_BASEBAND;

                    blt_quick_tx_prepare(FSM_STX, (void *)&pkt_scanReq, rflen);

                    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_TX_ON);  }

                    activeScan_bit |= ACTIVE_SCAN_TX_SET;
                }


                do{

                    //TODO: If resolving list used and filter policy enable, 2 problem need consider:
                    // 1. RPA calculate cost too many time, Scan tail margin time may need change
                    // 2. RPA calculate will use AES HW module in IRQ, may conflict with AES in main_loop
                    u8 txAddrType = pAdv->txAddr;
                    u8 rxAddrType = pAdv->rxAddr;
                    ll_resolv_list_t *pRL_match = NULL;

                    u8 rpa_resolve_err = 0;

                    blt_ll_addr_set_peer_address(0, txAddrType, pAdv->advA);

                    ///LL/SEC/SCN/BV-01-C   [Random Address Scanning, LE Encryption (With HCI LE Encrypt)]
                    if(blRslvLst.addrRlEn){

                        u8 advA_is_rpa = IS_RESOLVABLE_PRIVATE_ADDR(txAddrType, pAdv->advA);
                        if(advA_is_rpa){ //RPA
                            /* none direct ADV with policy 0/2: pass, no need resolve RPA for filter, but need IDA for ADV report event,
                             *                                  so resolving work is not wasting time
                             *      direct ADV same situation, no need check advA, just check if targetA(initA) addressed  to local,
                             *                                 but also need IDA for ADV report event
                             */
                            pRL_match = blt_ll_resolve_rpa(0, pAdv->advA, NULL);
                            if(pRL_match){  //resolving pass, have a RL entry
                                blt_ll_storePeerDeviceRpa(pRL_match, pAdv->advA);
                                blt_ll_addr_set_peer_address(1, pRL_match->rlIdAddrType, pRL_match->rlIdAddr);
                            }
                            else{
                                rpa_resolve_err = 1;
                            }
                        }
                        else{ //IDA
                            /* here "pRL_match" may be used later by scanA(RPA) in scan_req,
                             * so must can not be included in "NETWORK_PRIVACY IGNORE_IDA_CHECK" */
                            pRL_match = blt_ll_searchResolvingListEntry(txAddrType, pAdv->advA);

                            #if (NETWORK_PRIVACY_IGNORE_IDA_CHECK)
                                if(pRL_match && pRL_match->peerIrk_valid){ //peer device has distributed its IRK
                                    if(pRL_match->rlPrivMode == NETWORK_PRIVACY_MODE){ //not allowed  /* LL/DDI/SCN/BV-26-C */
                                        my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan, network privacy ignore IDA, stop", 0, 0);
                                        break; //stop
                                    }
                                    else{//DEVICE_PRIVACY_MODE, allowed  /* LL/DDI/SCN/BV-28-C */
                                        my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan, device privacy accept IDA", 0, 0);
                                    }
                                }
                            #endif
                        }
                    }


                    /* check if advA pass */
                    if(bltScn.scan_fp_wl){ //filter needed: direct ADV & none_direct ADV, , check accept list
                        /* 1. RPA can not resolve to a IDA, no change to use AL(accept list), fail
                         * 2. accept list filter fail */
                        if(rpa_resolve_err || !blt_ll_searchAddrInWhiteListTbl(bltAddr.peer_pka_or_ida_type, bltAddr.peer_pka_or_ida_addr)){
                            #if (DBG_PRVC_LEGSCAN_EN)
                                if(rpa_resolve_err){
                                    my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan, peer advA RPA resolve ERR, stop", bltAddr.peer_pka_or_ida_addr, 6);
                                }
                                else{
                                    my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan, peer advA not in AL, stop", bltAddr.peer_pka_or_ida_addr, 6);
                                }
                            #endif

                            break;
                        }
                        else{
                            my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan, peer advA in AL", bltAddr.peer_pka_or_ida_addr, 6);
                        }
                    }
                    else{ //none direct ADV, filter no need
                        //pass
                    }



                    /* direct ADV "ADV_DIRECT_IND", check if targetA address to local device */
                    bltScn.direct_initA_rpa_resolve_fail = 0;
                    if(bltScn.direct_adv){
                        /* ADV_IND, no change send scan_req, so here timing is not very urgent, can print some log */
                        rf_pkt_adv_direct_ind_t *p_adv_direct_ind = (rf_pkt_adv_direct_ind_t *)pAdv;
                        int targetA_is_rpa = IS_RESOLVABLE_PRIVATE_ADDR(rxAddrType, p_adv_direct_ind->targetA);
                        if(targetA_is_rpa){ //RPA
                            /* attention, different from ADV: here must use "pRL_match" locate by peer advA !!! */
                            if(pRL_match && blt_ll_resolve_rpa(1, p_adv_direct_ind->targetA, pRL_match)){ //resolve success, pass
                                /* here for "scan fp targetA rpaPass", originally no need resolve to save timing,
                                * but for "LL/DDI/SCN/BV-14-C" "HCI_LE_Direct_Advertising_Report_Event", we should detect resolving error */
                                my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan, ADV_DIRECT_IND targetA RPA resolve OK, match", p_adv_direct_ind->targetA, 6);
                            }
                            else{ //resolve Fail
                                if(bltScn.scan_fp_targetA_rpaPass){
                                    //for policy 0x02/0x03, even for RPA resolve fail, still accept
                                    //but should mark this, will use "HCI_LE_Direct_Advertising_Report_Event" later "LL/DDI/SCN/BV-14-C"
                                    bltScn.direct_initA_rpa_resolve_fail = 1;
                                    my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan, targetA RPA resolve ERR for policy 2/3, accept, direct ADV report", p_adv_direct_ind->targetA, 6);
                                }
                                else{
                                    my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan, ADV_DIRECT_IND targetA RPA resolve ERR, stop", p_adv_direct_ind->targetA, 6);
                                    break;  //stop
                                }
                            }
                        }
                        else{ //IDA
                            if(smemcmp(p_adv_direct_ind->targetA, bltScn.scan_mac_addr, BLE_ADDR_LEN) || rxAddrType != bltScn.scan_mac_type){
                                my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan, ADV_DIRECT_IND targetA IDA not match, stop", p_adv_direct_ind->targetA, 6);
                                break; //stop
                            }
                            else{
                                //pass
                                my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan, ADV_DIRECT_IND targetA IDA match", p_adv_direct_ind->targetA, 6);
                            }
                        }
                    }



                    if(activeScan_bit & ACTIVE_SCAN_TX_SET)
                    {
                        pkt_scanReq.rxAddr = txAddrType;
                        smemcpy(pkt_scanReq.advA, pAdv->advA, BLE_ADDR_LEN);

                        if(bltScn.scan_ownAddr_rpa && pRL_match && pRL_match->localIrk_valid){
                            pkt_scanReq.txAddr = BLE_ADDR_RANDOM;
                            smemcpy(pkt_scanReq.scanA, pRL_match->rlLocalRpa, BLE_ADDR_LEN);
                            blt_ll_resolvSetRpaInUse(pRL_match); //important: mark
                        }
                        else{
                            pkt_scanReq.txAddr = bltScn.scan_mac_type;
                            smemcpy(pkt_scanReq.scanA, bltScn.scan_mac_addr, BLE_ADDR_LEN);
                        }


                        /* task gap timing not enough, not distinguish whether create_connection == CONNECT_REQ_GOING. */
                        if( (u32)(systimer_get_irq_capture() - clock_time()) < (ACTIVE_SCAN_MAX_TICK+SCAN_BOUNDARY_MARGIN_COMMON_TICK) ||
                            (u32)(systimer_get_irq_capture() - clock_time()) > BIT(30)){
                            DBG_CHN3_TOGGLE;
                            my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan, time not enough, stop", 0, 0);
                            break;
                        }

                        /* Before we prepare to send a scan request, Check if backoff pass */
                        #if (LEGSCAN_SCANREQ_FLOW_CTRL == SCANRSP_DEVICE_TBL)
                            if(blt_ll_isScanRspReceived(txAddrType, pAdv->advA)){
                                my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "[PRV][SCN] legscan, time not enough, stop", 0, 0);
                                break;
                            }
                        #elif (LEGSCAN_SCANREQ_FLOW_CTRL == BACKOFF_ALGORITHM)
                            if (bltScn.backoffCount > 0){
                                bltScn.backoffCount --;
                                if(bltScn.backoffCount){
                                    activeScan_bit |= ACTIVE_SCAN_STACK_STOP;
                                }
                                else{
                                    activeScan_bit |= ACTIVE_SCAN_BACKOFF_CHANCE;
                                }
                            }
                        #endif
                    }


                    //for ADV packet: set address type, change RPA to IDA if needed
                    u8 peer_adv_type = bltAddr.peer_pka_or_ida_type;
                    if(bltAddr.peer_use_rpa){
                        peer_adv_type |= PEERATYPE_IDENTITY_MASK;
                        smemcpy(pAdv->advA, bltAddr.peer_pka_or_ida_addr, BLE_ADDR_LEN);
                    }
                    /* BIT(6..7), BIT(7) RX_ADDR no used later, so we can use it to mark */
                    peer_adv_type<<=6;
                    raw_pkt[DMA_RFRX_OFFSET_HEADER] &= 0x3F;
                    raw_pkt[DMA_RFRX_OFFSET_HEADER] |= peer_adv_type;


                    if(activeScan_bit & ACTIVE_SCAN_TX_SET)
                    {
                        activeScan_bit |= ACTIVE_SCAN_FINAL_EXECUTE;    /* no stop condition, will finally send scan_req */
                        HAL_CLEAR_RF_TX_IRQ;

                        volatile u32 *ph  = (u32 *) (new_pkt + DMA_RFRX_OFFSET_HEADER);
                        ph[0] = 0;  //clear mark

                        while ( !HAL_GET_RF_TX_IRQ && (u32)(clock_time() - bltRxPkt.rx_irq_tick) < 356 * SYSTEM_TIMER_TICK_1US){//176 + 150 + 30
                            if(usr_irq_handler_cb){usr_irq_handler_cb();}
                        }
                        //DBG_C HN4_TOGGLE;
                        //DBG_C HN4_TOGGLE;

                        /* Here add reset baseband for CSEM IP to keep RF mode safe, but add some delay before triggering RX continue mode, carefully!
                         * It seems safe to reset baseband, then enter RX continue mode.
                         * */
                        HAL_CSEM_IP_RESET_BASEBAND; //RF rx dma config keep, tx dma config lost after reset baseband for CSEM IP

                        rf_ble_tx_done();

                        /**
                         * For CSEM IP, We use special process to disable RX continue mode:
                         *  rf_ble_csem_close_rx_continue_mode();
                         *  HAL_CSEM_IP_RESET_BASEBAND;
                         *
                         * We can use two strategies to restore RX continue mode:
                         *
                         *  1: Because the reset_baseband will cause the TX DMA registers to be lost,
                         *  other RF registers will not be lost, we can use the original code,
                         *  need to ensure that the function: rf_set_rxmode can take effect;
                         *
                         *  2: we package a new function that restore RX continue mode, which
                         *  will involve the registers are re-set again, cumbersome, but
                         *  insurance, currently we use 1 strategy, the simplest way.
                         *  TODO: we need to test and verify;
                         **/
                        rf_set_rxmode();
                        if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }

                        u32 rx_begin_tick = clock_time();
                        HAL_CLEAR_RF_TX_IRQ;  //clear

                        while (!(*ph) && (u32)(clock_time() - rx_begin_tick) < 300 * SYSTEM_TIMER_TICK_1US){//150 + pkt(22*8) + 150 + margin(50)
                            if(usr_irq_handler_cb){usr_irq_handler_cb();}
                        }

                        if (*ph)
                        {
                            rx_begin_tick = clock_time ();
                                                                                // (31+10)*8 + margin(50)
                            while (!HAL_GET_RF_RX_IRQ && (clock_time() - rx_begin_tick) < 378 * SYSTEM_TIMER_TICK_1US){//150 + pkt(22*8) + 150 + margin(50)
                                if(usr_irq_handler_cb){usr_irq_handler_cb();}
                            }
                            STOP_RF_STATE_MACHINE;
                            HAL_CLEAR_RF_RX_IRQ;

                            //DBG_C HN5_TOGGLE;
                            //DBG_C HN5_TOGGLE;

                            new_pkt[1] = 0;
                            new_pkt[2] = 0;
                            if(RF_BLE_PACKET_VALIDITY_CHECK(new_pkt)) //CRC OK
                            {
                                rf_packet_scan_rsp_t * pRsp = (rf_packet_scan_rsp_t *) (new_pkt + DMA_RFRX_LEN_HW_INFO);
                                u16 *rspAdv16 = (u16*)pRsp->advA;
                                u16 *reqAdv16 = (u16*)pkt_scanReq.advA;

                                /* Version 5.3 | Vol 6, Part B, 6.2.1 Connectable and scannable undirected event type
                                If the advertiser processes the scan request, the advertiser's device address
                                (AdvA field) in the SCAN_RSP PDU shall be the same as the advertiser's
                                device address (AdvA field) in the SCAN_REQ PDU to which it is responding.
                                */
                                if(MAC_MATCH16(rspAdv16, reqAdv16) && pRsp->type == LL_TYPE_SCAN_RSP && pRsp->rf_len > 5){ //>= 6

                                    /* mark scan response data length, 6 = 6(advAddress) */
                                    new_pkt[1] = pRsp->rf_len - 6;

                                    u8 scan_fifo_idx = (scan_priRxFifo.wptr & SCAN_PRICHN_RXFIFO_MASK);
                                    st_prichn_scn_t * tmp_pPrichnScn= (st_prichn_scn_t *)&priChnScn_tbl[scan_fifo_idx];
                                    smemset(tmp_pPrichnScn, 0, sizeof(st_prichn_scn_t));//need to clear the value before
                                    tmp_pPrichnScn->prichn_phy = bltPHYs.cur_llPhy;

                                    scan_priRxFifo.wptr ++;
                                    /*
                                     * When FIFO is full, for example, rptr point to FIFO_0, wptr point to FIFO_0.
                                     * we abandon the oldest packet FIFO_0 and rptr++ to point to the next buffer FIFO_1. wptr point to this abandon buffer FIFO_0.
                                     * The destination is to make sure: rptr and wptr point to the different FIFO. that can be sure the FIFO being processed in mainloop
                                     *                                  will not be affected by RF receive.
                                     * For detailed explanation, please refer to the description "Scan Data Rf_len error            Summarized by SiHui 20221105"
                                     */
                                    if(((u8)(scan_priRxFifo.wptr - scan_priRxFifo.rptr) & 63)  >= SCAN_PRICHN_RXFIFO_NUM){
                                        scan_priRxFifo.rptr++; //make sure rptr and wptr point different fifo.
                                        tFIFO_full_flag = 1;
                                    }

                                    u8 * new_pkt2 = (u8 *)(scan_priRxFifo.p + (scan_priRxFifo.wptr & SCAN_PRICHN_RXFIFO_MASK) * SCAN_PRICHN_RXFIFO_SIZE); //set next buffer
                                    bltScn.scan_rx_pri_chn_dma_buff = (u32)new_pkt2;
                                    ble_rf_set_rx_dma((u8*)bltScn.scan_rx_pri_chn_dma_buff, SCAN_PRICHN_RX_DMA_SIZE);


                                    #if (LEGSCAN_SCANREQ_FLOW_CTRL == SCANRSP_DEVICE_TBL)
                                        blt_ll_addScanRspDevice(txAddrType, pAdv->advA);
                                    #endif


                                    //for scan_rsp: set address type, change RPA to IDA if needed
                                    if(bltAddr.peer_use_rpa){
                                        smemcpy(pRsp->advA, bltAddr.peer_pka_or_ida_addr, BLE_ADDR_LEN);
                                    }
                                    new_pkt[DMA_RFRX_OFFSET_HEADER] &= 0x3F;
                                    new_pkt[DMA_RFRX_OFFSET_HEADER] |= peer_adv_type;

                                    activeScan_bit |= ACTIVE_SCAN_SUCCESS;
                                }
                            }
                        }
                    }



                    next_buffer = 1;
                    blt_pPrichnScn->direct_flag = bltScn.direct_adv;
                    blt_pPrichnScn->directA_rpa_resolve_fail = bltScn.direct_initA_rpa_resolve_fail;
                    /* mark data length for extended scan:
                     *  direct ADV data length 0, otherwise data length is rfLen - 6(advAddress) */
                    raw_pkt[1] = bltScn.direct_adv ? 0 : (rflen - 6);

                }while(0);


                /* active scan finally not executed, but TX set previously, need restore manual RX to stop TX */
                if((activeScan_bit & (ACTIVE_SCAN_TX_SET | ACTIVE_SCAN_FINAL_EXECUTE)) == ACTIVE_SCAN_TX_SET)
                {
                    /* Here add reset baseband for CSEM IP to keep RF mode safe, but add some delay before triggering RX continue mode, carefully!
                     * It seems safe to reset baseband, then enter RX continue mode.
                     * */
                    HAL_CSEM_IP_RESET_BASEBAND; //RF rx dma config keep, tx dma config lost after reset baseband for CSEM IP

                    STOP_RF_STATE_MACHINE;
                    rf_ble_tx_done ();

                    /**
                     * For CSEM IP, We use special process to disable RX continue mode:
                     *  rf_ble_csem_close_rx_continue_mode();
                     *  HAL_CSEM_IP_RESET_BASEBAND;
                     *
                     * We can use two strategies to restore RX continue mode:
                     *
                     *  1: Because the reset_baseband will cause the TX DMA registers to be lost,
                     *  other RF registers will not be lost, we can use the original code,
                     *  need to ensure that the function: rf_set_rxmode can take effect;
                     *
                     *  2: we package a new function that restore RX continue mode, which
                     *  will involve the registers are re-set again, cumbersome, but
                     *  insurance, currently we use 1 strategy, the simplest way.
                     *  TODO: we need to test and verify;
                     **/
                    rf_set_rxmode ();
                    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }
                }

                #if (LEGSCAN_SCANREQ_FLOW_CTRL == BACKOFF_ALGORITHM)
                    if(activeScan_bit & ACTIVE_SCAN_BACKOFF_CHANCE ){
                        blt_ll_scanReqBackoff(&bltScn, (activeScan_bit & ACTIVE_SCAN_SUCCESS) ? TRUE : FALSE);
                    }
                #endif

            }
        }
    }//end of "rx header tick" &



    if (next_buffer)
    {
        /* for both Legacy & Non_Connectable Non_Scannable without auxiliary packet */
        blt_pPrichnScn->prichn_phy = bltPHYs.cur_llPhy;
    }
    else //reuse buffer
    {
        scan_priRxFifo.wptr--;
        bltScn.scan_rx_pri_chn_dma_buff = (u32)raw_pkt;
        ble_rf_set_rx_dma((u8*)bltScn.scan_rx_pri_chn_dma_buff, SCAN_PRICHN_RX_DMA_SIZE);

        if(tFIFO_full_flag){
            scan_priRxFifo.rptr--;
        }
    }

#if OS_SUP_EN
    if(scan_priRxFifo.wptr != scan_priRxFifo.rptr){
        if(blt_os_semCountIncrementIrq_cb)
        {
            blt_os_semCountIncrementIrq_cb();
        }
    }
#endif

    /* for Kite/Vulture, this is must; for Eagle, no effect. So we keep code compatible*/
    raw_pkt[0] = 1;

    return 0;
}













#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
int blt_ll_buildPrimaryChannelScanTask(void) //no CONN slot task, so timing is enough, no need ram_code function
{
    s32 sSlot_start_scan;

    if( bltScn.early_stop_tick && clock_time_exceed(bltScn.early_stop_tick, bltScn.scanInterval *625) ){
        bltScn.early_stop_tick = 0;
    }

    if(bltScn.early_stop_tick){  //TODO: process scan percent later
        sSlot_start_scan = bltSche.sSlot_idx_next;
    }
    else{
        u32 bSlot_distance = (u32)(bltSche.bSlot_idx_next - bltScn.bSlot_mark_scan);
        if( bSlot_distance > (u32)(bltScn.scanInterval + 1) ){
            sSlot_start_scan = bltSche.sSlot_idx_next;
        }
        else{
            sSlot_start_scan = bltSche.sSlot_idx_next - bSlot_distance*32 + bltScn.scanInterval*32;
            if(sSlot_start_scan < bltSche.sSlot_idx_next){
                sSlot_start_scan = bltSche.sSlot_idx_next;
            }

            if(bSlot_distance == 0){
                sSlot_start_scan = bltSche.sSlot_idx_next;
            }
        }
    }






    int sSlot_num_scanInter = bltScn.scanInterval*32;



    sch_task_t  *pCur_schTask = NULL;

    //TODO: if just handle 1 scanTask, code below can optimize
    for(int i=0; i<SCNTSK_FIFO_NUM; i++){

        bltScn.scnTsk_fifo[i].begin = sSlot_start_scan + sSlot_num_scanInter*i;
        //for aux_scan future task insert, "end" is needed
        /* duration here is important for aux_scan insert design, use smallest scan timing for a potential longest packet
         * here can not know coded PHY or other PHY is used, use 2000uS margin, 2000uS -> 102.4 sSlot -> 103
         * S8 :  = rf_len*64 + 720  = 20(2+18)*64 + 720 = 2000
         */
        bltScn.scnTsk_fifo[i].end = bltScn.scnTsk_fifo[i].begin + 103;



        if(i == 0){
            bltSche.pTask_head->next = &bltScn.scnTsk_fifo[0];
            pCur_schTask = bltSche.pTask_head->next;
        }
        else{
            pCur_schTask->next = &bltScn.scnTsk_fifo[i];
            pCur_schTask = pCur_schTask->next;
        }
    }

    bltSche.pTask_next = bltSche.pTask_head->next;  //TODO SiHui, can not remove, unknown reason


    return 1;
}

static inline bool blt_extScan_duration_proc(void){

    if(!bltScn.extScan_duration){
        return FALSE;
    }

    if(!bltScn.extScan_1stFlag){
        bltScn.extScan_1stFlag = 1;
        bltScn.extScan_startTick = clock_time()|0x01;
    }
    else if(clock_time_exceed(bltScn.extScan_startTick, (bltScn.extScan_duration/SYSTICK_NUM_PER_US) )){

        bltScn.extScan_1stFlag = 0;

        bltScn.durationPeriod_stateFlag |= EXTSCAN_DURATION_CHECK_PENDING;

        if(bltScn.extScan_period){
            bltScn.durationPeriod_stateFlag |= EXTSCAN_PERIOD_CHECK_PENDING;
        }

        blt_ll_prichn_scan_post();

        blmsParam.state_chng |= STATE_CHANGE_EXT_SCAN;
        blmsParam.scanInitEn_union.ext_scan_en  = 0;//disable scan.

        return TRUE;
    }

    return FALSE;
}


#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
void blt_set_prichn_scan_start(void)  //write in flash to save RamCode, Scan timing not critical
{

    blms_state = BLMS_STATE_PRICHN_SCAN_S;

    bltScn.bSlot_mark_scan = bltSche.bSlot_idx_irq_real;

    if(blt_extScan_duration_proc() == TRUE){
        return ;
    }


    blt_setScan_cal_chn_phy(1);  //do it before  "scanInterval" "scan_percent" used
    blt_setScan_enter_manual_rx();

    DBG_CHN0_HIGH;
    //DBG_QIUWEI_CHN0_HIGH;
    DBG_SIHUI_CHN0_HIGH;
    DBG_FANQH_CHN6_HIGH;
//  DBG_TIANXIANG_CHN0_HIGH;
    #if (SL01_scn_prichn)
        log_task_begin_irq(SL_STACK_IRQ_TIMING_EN, SL01_scn_prichn);
    #endif

    u32 scan_interval_tick =  bltScn.scanInterval*SYSTEM_TIMER_TICK_625US;

    /* task_mask only PRICHN & SECCHN exist, so not need too many time process link_list combine */
    if(blmsParam.create_connection == CONNECT_REQ_GOING){
        //TODO: Timing shortage, should consider this when connection too many, the last CONN_REQ is hard to send
        bltScn.scan_post_margin = (150 + 352 + 200) * SYSTEM_TIMER_TICK_1US;  //352: 1M PHY, conn_req timing (34+10)*8=352
    }
    else{
        bltScn.scan_post_margin = (100 + 200) * SYSTEM_TIMER_TICK_1US;
    }

    u32 scan_max_duration = scan_interval_tick - bltScn.scan_post_margin;
    //scan_post_tick overflow is considered, so 128 is preferred .
    //u32 scan_expect_duration = scan_interval_tick* bltScn.scan_percent >>7;  // /128
    u32 scan_expect_duration;
    if(scan_interval_tick < BIT(25)){  // *128 should consider u32 overflow, BIT(32)>>7 = BIT(25), about 2S
        scan_expect_duration = (scan_interval_tick*bltScn.scan_percent)>>7; // /128
        if(scan_interval_tick > scan_expect_duration ){
            scan_interval_tick = scan_expect_duration;
        }
    }
    else{
        scan_expect_duration = (scan_interval_tick>>7)*bltScn.scan_percent; // /128
    }

    bltScn.scan_post_tick = bltSche.sSlot_tick_irq + min2(scan_expect_duration, scan_max_duration);
    systimer_set_irq_capture(bltScn.scan_post_tick);

    systick_irq_trigger = SYS_IRQ_TRIG_PRICHN_SCAN_POST;

}


_attribute_ram_code_
int blt_ll_prichn_scan_insert(void)
{


    if(blmsParam.create_connection == CONNECT_REQ_GOING){
        //TODO: Timing shortage, should consider this when connection too many, the last CONN_REQ is hard to send
        bltScn.scan_post_margin = (150 + 352 + SLOT_PROCESS_MAX_US) * SYSTEM_TIMER_TICK_1US;  //352: 1M PHY, conn_req timing (34+10)*8=352
    }
    else{
        bltScn.scan_post_margin = (100 + SLOT_PROCESS_MAX_US) * SYSTEM_TIMER_TICK_1US;;
    }


    blt_setScan_cal_chn_phy(0);  //do it before  "scanInterval" "scan_percent" used

    u32 cur_time = clock_time();
    u32 scan_tick_left = bltSche.sSlot_tick_irq - cur_time - bltScn.scan_post_margin;


    /* Coded PHY: consider rf_len = 8, coded S8 time = 8*64 + 720 = 1232
     * 1M PHY: (37+10)*8 = 376
     * 2M PHY: (37+11)*4 = 192 */
    u32 scan_tick_min = (bltScn.phy_index == PHY_INDEX_CODED ? 1250 : 400)*SYSTEM_TIMER_TICK_1US;

    if( (u32)(scan_tick_left - scan_tick_min) < BIT(30) ){
        #if 1 //fix
            bltScn.bSlot_mark_scan = bltSche.bSlot_idx_start + (u32)(clock_time() - bltSche.bSlot_tick_start)/SYSTEM_TIMER_TICK_625US;
        #else
            bltScn.bSlot_mark_scan = bltSche.bSlot_idx_next;    //must mark here to pass adv/scan on_off test
        #endif

        //if timing too short ( < 1000 uS), enter scan state immediately, otherwise calculate by scan percent
        u32 scan_duration = scan_tick_left;
        if(scan_tick_left > 2000 * SYSTEM_TIMER_TICK_1US){
            scan_duration = blt_ll_calculateScanDuration(scan_tick_left); //write in flash to save RamCode
        }


        blms_state = BLMS_STATE_PRICHN_SCAN_S;

        blt_setScan_enter_manual_rx();

        DBG_CHN0_HIGH;
        //DBG_QIUWEI_CHN0_HIGH;
        DBG_SIHUI_CHN0_HIGH;
        DBG_SIHUI_CHN6_HIGH;
//      DBG_TIANXIANG_CHN0_HIGH;
        #if (SL01_scn_prichn)
            log_task_begin_irq(SL_STACK_IRQ_TIMING_EN, SL01_scn_prichn);
        #endif

        systick_irq_trigger = SYS_IRQ_TRIG_PRICHN_SCAN_POST;
        bltScn.scan_post_tick = cur_time + scan_duration;
        systimer_set_irq_capture(bltScn.scan_post_tick);


        bltScn.last_scan_end_time = (u32)(clock_time() + scan_tick_left);  //must update here


        return 1;  //scan available
    }


    return 0; //scan unavailable
}





_attribute_ram_code_
void    blt_ll_prichn_scan_post(void)
{
    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_OFF);  }

    rf_set_tx_rx_off();
    blmsParam.rf_fsm_busy = 0;
    DBG_CHN0_LOW;
    //DBG_QIUWEI_CHN0_LOW;
    DBG_SIHUI_CHN0_LOW;
    DBG_FANQH_CHN6_LOW;
//  DBG_TIANXIANG_CHN0_LOW;
    #if (SL01_scn_prichn)
        log_task_end_irq(SL_STACK_IRQ_TIMING_EN, SL01_scn_prichn);
    #endif

    /* very important to clear RX status: boundary RX packet may enter other state
       consider timing margin, we clear it after a while, so add a mark here, clear it later */
    blmsParam.delay_clear_rf_status = 1;

    blms_state = BLMS_STATE_PRICHN_SCAN_E;

    // to solve 1 bug
    if(blmsParam.create_connection == CONNECT_REQ_LEG_PENDING){
        blmsParam.create_connection = CONNECT_REQ_GOING;
        bltScn.initiate_going = LEG_INITIATE_GOING;
    }
    else if(blmsParam.create_connection == CONNECT_REQ_EXT_PENDING){
        blmsParam.create_connection = CONNECT_REQ_GOING;
        bltScn.initiate_going = EXT_INITIATE_GOING;
    }

    /* attention: here not using sS lot_idx_irq & sSlot_tick_irq, cause it's a systick_irq_trigger IRQ,
    these variables may not exist or correct.
    the reason is: primary channel scan start may triggered in some task gap(FLAG_SCHEDULE_PRICHN_SCAN_INSERT) */
    //2^32/2=2^31 = 65536*65536/2 = 134217728uS = 134S, consider sign bit, 67S
    int sSlot_diff_num = (u32)(  (clock_time() + (SLOT_PROCESS_MAX_TICK - 30*SYSTEM_TIMER_TICK_1US)) - bltSche.sSlot_tick_start)*SSLOT_TICK_REVERSE + 1;
    bltSche.sSlot_idx_next = 0  + sSlot_diff_num;  // bltSche.sSlot_idx_start always 0
    bltSche.sSlot_tick_next = bltSche.sSlot_tick_start + sSlot_diff_num*SSLOT_TICK_NUM;

}






#if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN)
_attribute_ram_code_
int blt_ll_prichn_scan_align_build(void)
{


#if (ANOTHER_BIG_INTV_EXTENDED_ADV)
    if( (bltAdScn.scanTask_Policy == 0x01)                      \
        || ((bltAdScn.extAdv_num <= bltAdScn.extAdv_num_thres)  \
        && (bltAdScn.extAdv_legacyMode)                         \
        && (bltAdScn.legadv_int > bltAdScn.legadv_int_thres)    \
        && ((bltScn.scanInterval*2) < bltAdScn.legadv_int))     )
#else
    if((bltAdScn.legadv_int > LEGADV_THRES) && ((bltScn.scanInterval*2) < bltAdScn.legadv_int))
#endif
    {
        int sSlot_num_scanInter = bltScn.scanInterval*32;
        s32 sSlot_start_scan;
        u32 bSlot_distance = (u32)(bltSche.bSlot_idx_next - bltScn.bSlot_mark_scan);
        if( bSlot_distance > bltScn.scanInterval){
            sSlot_start_scan = bltSche.sSlot_idx_next;
        }
        else{
            sSlot_start_scan = bltSche.sSlot_idx_next - bSlot_distance*32 + sSlot_num_scanInter;
            if(sSlot_start_scan < bltSche.sSlot_idx_next){
                sSlot_start_scan = bltSche.sSlot_idx_next;
            }
        }

        s32 sSlot_scan_align = sSlot_start_scan;
        int duration_sSlot = (bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US + 30)*SSLOT_US_REVERSE;
        int new_task_cnt = 0;
        for(int i=0; i<SCNALIGN_FIFO_NUM; i++){
            sch_task_t  *pCur_schTask = (sch_task_t *)&bltAlignScn.scnAlign_fifo[i];

            if(bltAdScn.legadv_alloc){
                if(sSlot_scan_align < (bltAdScn.legadv_sSlot - 103)){  //at least 2mS
                    //timing OK, no change
                }
                else if(sSlot_scan_align < (bltAdScn.legadv_sSlot + sSlot_num_scanInter)){
                    sSlot_scan_align = bltAdScn.legadv_sSlot + sSlot_num_scanInter;
                }
            }


            pCur_schTask->begin = sSlot_scan_align;
            pCur_schTask->end = pCur_schTask->begin + duration_sSlot - 1;
            pCur_schTask->cover_other = 0;


            if( pCur_schTask->begin >=  bltSche.sSlot_endIdx_dft){  //new task beyond correct range, finish
                break;
            }
            else if(pCur_schTask->end < bltSche.sSlot_endIdx_dft){ //new task in correct range
                new_task_cnt ++;
            }
            else{ //new task across "sSlot_endIdx_dft"

            }

            sSlot_scan_align += sSlot_num_scanInter;
        }


        if(new_task_cnt){
            blt_ll_addTask2ExistLinklist( &bltAlignScn.scnAlign_fifo[0], new_task_cnt);
        }
    }


    return 0;
}


#endif






#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
u32 blt_ll_calculateScanDuration (u32 scan_tick_left)
{
    u32 scan_duration = scan_tick_left;
    if(bltScn.last_scan_end_time ){
        u32 total_tick = (u32)( clock_time() - bltScn.last_scan_end_time) + scan_tick_left;
        if(total_tick < BIT(25)){  // *128 should consider u32 overflow, BIT(32)>>7 = BIT(25), about 2S
            u32 scan_expect_duration = (total_tick*bltScn.scan_percent)>>7; // /128
            if(scan_tick_left > scan_expect_duration ){
                scan_duration = scan_expect_duration;
            }
        }
        else{
            if(scan_tick_left < BIT(25)){
                scan_duration = (scan_tick_left*bltScn.scan_percent)>>7; // /128
            }else{
                scan_duration = (scan_tick_left>>7)*bltScn.scan_percent; // /128
            }
        }
    }
    else{
        if(scan_tick_left < BIT(25)){
            scan_duration = (scan_tick_left*bltScn.scan_percent)>>7; // /128
        }else{
            scan_duration = (scan_tick_left>>7)*bltScn.scan_percent; // /128
        }
    }

    return scan_duration;
}

#if (SCAN_EN_MORE_STRATEGY)

/*
 * extended scan and legacy scan use the same strategy.
 */
void        blc_ll_configScanEnableStrategy (scan_en_strtg_t scanStrategy)
{
    bltScn.scan_en_strategy = scanStrategy;
}

#endif

#if (LL_ASYNC_LEA_EN)

void blc_ll_asyncSetPrivateScanChannel (u8 chn0,u8 chn1,u8 chn2)
{
    bltScn.asyncScanIndex = 1;
    blc_asyncAdv_channel[0] = chn0;
    blc_asyncAdv_channel[1] = chn1;
    blc_asyncAdv_channel[2] = chn2;
}
#endif /*!< LL_ASYNC_LEA_EN */

#endif /*!< LL_ACL_CEN_EN || LL_FEATURE_ENABLE_LE_EXTENDED_SCAN */
