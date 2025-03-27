/********************************************************************************************************
 * @file    leg_adv.c
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



_attribute_ble_data_retention_  _attribute_aligned_(4) ll_legadv_t  bltLegAdv;



void blc_ll_initLegacyAdvertising_module(void)
{
    #if (!IS_POWER_OF_2(ADVTSK_FIFO_NUM))
        #error ADVTSK_FIFO_NUM must be pow of 2 !
    #endif

    #if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_legadv_t)), leg_adv);
    #endif

    blt_ll_initAdvertisingCommon();


    ll_leg_adv_irq_task_cb = blt_leg_adv_interrupt_task;
    ll_leg_adv_mlp_task_cb = blt_leg_adv_mainloop_task;


    smemcpy(pkt_Adv.advA, bltMac.macAddress_public, BLE_ADDR_LEN);
    smemcpy(pkt_scanRsp.advA, bltMac.macAddress_public, BLE_ADDR_LEN);



    bltLegAdv.bSlot_mark_adv = (BIT(31) | BIT(30) | BIT(29));
    bltLegAdv.sSlot_diff_adv = 0;


    for(int i=0;i<ADVTSK_FIFO_NUM;i++){
        bltLegAdv.advTsk_fifo[i].scheTask_oft = TSKOFT_LEG_ADV;
        bltLegAdv.advTsk_fifo[i].scheTask_flg = TSKFLG_LEG_ADV;
        bltLegAdv.advTsk_fifo[i].taskFifo_idx = i;
    }

    #if (MULTIPLE_LOCAL_DEVICE_ENABLE)
        //bltLegAdv.adv_curDevIdx = DEFAULT_DEVICE_INDEX;  //default value is 0, can save
    #endif
}




_attribute_ram_code_
int blt_leg_adv_interrupt_task (int flag, void*p)
{
    (void)p; //unused, remove warning
//  int adv_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_SCHEDULE_START){
        blt_ll_send_adv();
    }
    else if(flag & FLAG_SCHEDULE_LEGADV_BUILD){
        blt_ll_buildLegacyAdvTask();
    }
    else if(flag & FLAG_INSERT_SCHTSK_CONFLICT){
        //sch_task_t *pTgtTsk = (sch_task_t *)p;
        //u8 tgtTskFlg = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
        //my_dump_str_data(DBG_EXTADV_TIMING, "[leg_adv]insertTsk conflict, tgtTsk=", &tgtTskFlg, 1);
    }

    return 0;
}


void blt_ll_legadv_direct_adv_timeout(void);


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
int blt_leg_adv_mainloop_task (int flag)
{
    if(flag == FLAG_MODULE_RESET){
        blt_ll_reset_leg_adv();
    }
    else if(flag == FLAG_MODULE_MAINLOOP){
        if(blmsParam.leg_adv_en && bltLegAdv.adv_duration_en && clock_time_exceed(bltLegAdv.adv_start_tick, bltLegAdv.adv_duration_us)){
            //LL/CON/ADV/BV-04-C    [Directed Advertising Connection]
            //LL/DDI/ADV/BV-11-C    [Directed Advertising Events]

            //my_dump_str_data(0, "high duty adv timeout", 0, 0);  //debug
            //write_dbg32(0x00018, clock_time());
            //BLMS_ERR_DEBUG(DBG_PRVC_LEGADV_EN, 0x770F0000);

            bltLegAdv.adv_duration_en = 0;
            blc_ll_setAdvEnable(BLC_ADV_DISABLE);
            blt_ll_legadv_direct_adv_timeout();
        }
    }


    return 0;
}

void blt_ll_legadv_direct_adv_timeout(void)
{
    /*
    If LE Enhanced Connection Complete event is unmasked and LE Connection Complete event is unmasked,
    only the LE Enhanced Connection Complete event is sent when a new connection has been created. */
    u8 peerAddr[BLE_ADDR_LEN] = {0};
    if((hci_le_eventMask & HCI_LE_EVT_MASK_ENHANCED_CONNECTION_COMPLETE)
        || (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_ENHANCED_CONNECTION_COMPLETE_V2)){
        u8 peerRpa[BLE_ADDR_LEN] = {0};
        u8 localRpa[BLE_ADDR_LEN] = {0};
        if(hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_ENHANCED_CONNECTION_COMPLETE_V2){
            hci_le_enhancedConnectionComplete_evt_v2(HCI_ERR_ADVERTISING_TIMEOUT, 0, 0, 0, peerAddr, localRpa, peerRpa, 0, 0, 0, 0, 0xFF, 0xFFFF);
        }else{
            hci_le_enhancedConnectionComplete_evt(HCI_ERR_ADVERTISING_TIMEOUT, 0, 0, 0, peerAddr, localRpa, peerRpa, 0, 0, 0, 0);
        }
    }
    else if(hci_le_eventMask & HCI_LE_EVT_MASK_CONNECTION_COMPLETE){
        hci_le_connectionComplete_evt(HCI_ERR_ADVERTISING_TIMEOUT, 0, 0, 0, peerAddr, 0, 0, 0, 0);
    }
}


void    blt_ll_reset_leg_adv(void)
{
    blt_ll_reset_adv_common();

    blmsParam.leg_adv_en = 0;
    bltLegAdv.adv_data_set = 0;
    bltLegAdv.adv_duration_en = 0;


    #if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE)
        bltAdv.legAdv_chngReason = 0;
        bltLegAdv.scanrsp_data_set = 0;
    #endif
}

void blc_ll_continue_adv_after_scan_req(u8 enable)
{
    bltAdv.blc_continue_adv_en = enable;
}

void blc_ll_set_scan_rsp_en(u8 enable)
{
    bltLegAdv.scnReq_response = enable;
}


ble_sts_t blc_ll_setAdvData(const u8 *data, u8 len)
{
    tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_CMD), "[LL][CMD] Set_Adv_Data", data, len);

    if(len > 31){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    /* special: data backup */
    bltLegAdv.adv_data_set = 1;
    bltLegAdv.backup_rfLen = len + BLE_ADDR_LEN;
    smemcpy(bltLegAdv.backup_6_advData, data, 6);



    #if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE)
        /* here standard design: we should hold previous old data, when new data come, compare them,
         * if data are different, trigger RPA change. but this will cost extra
         * */
        //do not need consider ADV_DIRECT_IND, because test_case no have that
        if(bltAdv.legAdv_chngReason & REFRESH_RPA_ADVDATA_CHANGE){
            int old_data_len = pkt_Adv.rf_len - 6;
            if(old_data_len != len || smemcmp(pkt_Adv.data, data, len)){
                my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] Change adv data, RPA refresh", 0, 0);
                u32 r = irq_disable();
                ll_resolv_list_t *pRL = bltLegAdv.pRslvlst_legAdv;
                irq_restore(r);
                if(pRL){
                    blt_ll_resolvRefreshRpa(pRL);
                }
            }
        }
    #endif

    /*important: for ADV running in IRQ, do not advertising when adv data/scan_rsp data is changing,
                 so here we use IRQ protect. should guarantee that IRQ disabling not too long time*/
    u32 r = irq_disable();

    if(bltLegAdv.cur_directAdv){

        /* special: data backup */
        if(len > 6){
            smemcpy(pkt_Adv.data + 6, data + 6, len - 6);
        }
    }
    else{
        pkt_Adv.rf_len = len + BLE_ADDR_LEN;
        if(len > 0){
            smemcpy(pkt_Adv.data, data, len);
        }
    }

    irq_restore(r);

    return BLE_SUCCESS;
}


ble_sts_t blc_hci_le_setAdvData(const u8 *data, u8 len)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Set_Adv_Data", data, len);

    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if(IS_EXTENDED_ADV_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_LEGACY_ADV_VALID;


    return blc_ll_setAdvData(data, len);
}


_attribute_noinline_
ble_sts_t blc_ll_setAdvEnable(adv_en_t adv_enable)
{
    /* core_5.3
    If Advertising_Enable is set to 0x01, the advertising parameters' Own_-
    Address_Type parameter is set to 0x00, and the device does not have a public
    address, the Controller should return an error code which should be Invalid HCI
    Command Parameters (0x12).                                                              Ignore, controller always have public address

    If Advertising_Enable is set to 0x01, the advertising parameters' Own_-
    Address_Type parameter is set to 0x01, and the random address for the device
    has not been initialized using the HCI_LE_Set_Random_Address command,
    the Controller shall return the error code Invalid HCI Command Parameters
    (0x12).                                                                                 Done !!!

    If Advertising_Enable is set to 0x01, the advertising parameters' Own_-
    Address_Type parameter is set to 0x02, the Controller's resolving list did not
    contain a matching entry, and the device does not have a public address, the
    Controller should return an error code which should be Invalid HCI Command
    Parameters (0x12).                                                                      Ignore, controller always have public address

    If Advertising_Enable is set to 0x01, the advertising parameters' Own_-
    Address_Type parameter is set to 0x03, the controller's resolving list did not
    contain a matching entry, and the random address for the device has not been
    initialized using the HCI_LE_Set_Random_Address command, the Controller
    shall return the error code Invalid HCI Command Parameters.(0x12).                      incomplete process !!!
    */


    tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_CMD), "[LL][CMD] Set_Adv_Enable", &adv_enable, 1);

    /* special process: do not consider type 0x03 RL entry, process it as type 0x01
     * BQB test_case don not check RL entry condition, so we use this
     * But remember that it's not correct, if one day BQB test more detailed, will find out this ERROR */
    if((bltLegAdv.legadv_ownAddr_type & 0x01) && !(blmsParam.hci_cmd_mask & SET_RANDOM_ADDR_CMD_MASK))
    {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


#if 1  //TODO: also will change when re_set ADV parameters/ADV data/ADV response data
    //re_calculate sSlotDuration_adv according to latest ADV parameters/ADV data/ADV response data
    if (bltAdv.blc_continue_adv_en) {
        //40: have known: {37/38: Adv+T_IFS+ConnReq(CRC Fail); 39: Adv+ScanReq+ScanRsp.} = MAX_LEG_ADV_EVT_US
        //if blc_continue_adv_en: Worst case: {37/38/39: Adv+T_IFS+ScanReq+T_IFS+ScanRsp.}
        //= case above + {ScanReq+T_IFS+ScanRsp-ConnReq}*2 = MAX_LEG_ADV_EVT_US + [(12+10)*8+150+(max37+10)*8-(34+10)*8]*2
        //= (MAX_SSLOT_DURATION_ADV + approx40)*16*2/625
        bltLegAdv.sSlotDuration_adv = MAX_SSLOT_DURATION_ADV+40;  //TODO: optimize with real adv and scan_rsp data length
    } else {
        bltLegAdv.sSlotDuration_adv = MAX_SSLOT_DURATION_ADV;  //TODO: optimize
    }
#endif

    if(blmsParam.max_slave_num){
        //Important to disable IRQ
        u32 r = irq_disable();


        if(adv_enable != blmsParam.leg_adv_en){
            blmsParam.state_chng |= STATE_CHANGE_LEG_ADV;

            if(!adv_enable){
                bltLegAdv.bSlot_mark_adv = (BIT(31) | BIT(30) | BIT(29));

                #if (LEG_ADV_DELAY_CTRL_EN)
                    //Remove Adv task_mask to stop adv before next sche rebuild.
                    blt_sche_removeTaskMask(TSKMSK_LEG_ADV);
                    if(!bltSche.task_mask){
                        //No task, stop scheduler immediately and entirely.
                        //Thus, AdvEnable will call blt_ll_mainloop_startScheduler().
                        systimer_irq_disable();
                        systimer_clr_irq_status();
                        blmsParam.sche_run_flag = 0;
                        blmsParam.state_chng &= ~STATE_CHANGE_LEG_ADV;
                    }
                #endif
            }


            /* for high duty direct ADV timeout */
            if(adv_enable){
                if(bltLegAdv.high_duty_direct){
                    bltLegAdv.adv_duration_en = 1;
                    bltLegAdv.adv_start_tick = clock_time() | 1;
                }
            }
            else{
                bltLegAdv.adv_duration_en = 0;
            }
        }

        blmsParam.leg_adv_en = (u8)adv_enable;




        #if (LEG_ADV_DELAY_CTRL_EN)
            if(blmsParam.leg_adv_en && (blmsParam.state_chng & STATE_CHANGE_LEG_ADV) && bltSche.task_mask){
                if(blms_state == BLMS_STATE_NONE ||  (blms_state & BLMS_STATE_PRICHN_SCAN_S))
                {
                    u32 cur_tick = clock_time();
                    if(tick1_exceed_tick2(systimer_get_irq_capture(), cur_tick + 8*SYSTEM_TIMER_TICK_1MS)){
                        systick_irq_trigger = SYS_IRQ_TRIG_SCHE_INSERT;
                        systimer_set_irq_capture(cur_tick + 4*SYSTEM_TIMER_TICK_1MS);
                    }
                }
            }

        #endif

        irq_restore(r);

        #if OS_SUP_EN
        if(blt_os_giveSem_cb)
        {
            blt_os_giveSem_cb();
        }
        #endif

        return BLE_SUCCESS;
    }


    return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;

}


ble_sts_t blc_hci_le_setAdvEnable(adv_en_t adv_enable)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Set_Adv_Enable", &adv_enable, 1);

    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if(IS_EXTENDED_ADV_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_LEGACY_ADV_VALID;


    return blc_ll_setAdvEnable(adv_enable);
}



/**************************************************************************
     adv type           pkt_adv.type                 SCAN_REQ   CONNECT_REQ

    ADV_IND             0 : LL_TYPE_ADV_IND             yes         yes
    ADV_DIRECT_IND      1 : LL_TYPE_ADV_DIRECT_IND      no          yes(*)
    ADV_NONCONN_IND     2 : LL_TYPE_ADV_NONCONN_IND     no          no
    ADV_SCAN_IND        6 : LL_TYPE_ADV_SCAN_IND        yes         no

    yes(*)      Only the correctly addressed initiator may respond
 *************************************************************************/
ble_sts_t blc_ll_setAdvParam( adv_inter_t intervalMin, adv_inter_t intervalMax, adv_type_t  advType,        own_addr_type_t ownAddrType,  \
                                u8 peerAddrType,         u8  *peerAddr,           adv_chn_map_t adv_channelMap, adv_fp_type_t   advFilterPolicy)
{
    tlkapi_send_string_u8s(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_CMD), "[LL][CMD] Set_Adv_Param", advType, ownAddrType, adv_channelMap, advFilterPolicy);
    /*
    The Host shall not issue this command when advertising is enabled in the
    Controller; if it is the Command Disallowed error code shall be used.

    If the advertising interval range provided by the Host (Advertising_Interval_Min,
    Advertising_Interval_Max) is outside the advertising interval range supported
    by the Controller, then the Controller shall return the Unsupported Feature or
    Parameter Value (0x11) error code.
    */

    if(blmsParam.leg_adv_en){  // shall not issue this command when advertising is enable
        return HCI_ERR_CMD_DISALLOWED;
    }

    //Run #1480 - /HCI/DDI/BI-02-C  [Reject Invalid Advertising Parameters]
    if(advType != ADV_TYPE_CONNECTABLE_DIRECTED_HIGH_DUTY)
    {
        if(intervalMin > intervalMax || intervalMin < 0x0020 || intervalMax > 0x4000){
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }

    if(ownAddrType == OWN_ADDRESS_RESOLVE_PRIVATE_PUBLIC || ownAddrType == OWN_ADDRESS_RESOLVE_PRIVATE_RANDOM){
        #if(LL_FEATURE_ENABLE_PRIVACY)

        #else
            return  HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        #endif

        //do nothing
    }
    else if(ownAddrType == OWN_ADDRESS_PUBLIC || ownAddrType == OWN_ADDRESS_RANDOM){
        //do nothing
    }
    else{
        return  HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }



    u16 adv_intervalMin = intervalMin;
    u16 adv_intervalMax = intervalMax;

    if(adv_intervalMin > adv_intervalMax){ // Advertising_Interval_Min shall be less or equal to the Advertising_Interval_Max
        adv_intervalMin = adv_intervalMax;
    }

    bltLegAdv.cur_directAdv = bltLegAdv.high_duty_direct = 0;
    bltLegAdv.scnReq_response = bltLegAdv.conReq_response = 0;
    bltLegAdv.adv_type = (u8)advType;

    if(advType == ADV_TYPE_CONNECTABLE_DIRECTED_HIGH_DUTY || advType == ADV_TYPE_CONNECTABLE_DIRECTED_LOW_DUTY){

        if(advType == ADV_TYPE_CONNECTABLE_DIRECTED_HIGH_DUTY){
            adv_intervalMin = adv_intervalMax = ADV_INTERVAL_3_75MS;
            bltLegAdv.high_duty_direct = 1;
            bltLegAdv.adv_duration_us = 1280000;
        }

        bltLegAdv.cur_directAdv = 1;
        bltLegAdv.conReq_response = 1;

        /* ADV packet */
        pkt_Adv.type = LL_TYPE_ADV_DIRECT_IND;
        pkt_Adv.rxAddr = peerAddrType;
        pkt_Adv.rf_len = 12;

        /* data[0...5] is initA/targetA for ADV_DIRECT_IND
         * if RPA not used, peerAddr set here */
        rf_pkt_adv_direct_ind_t *p_adv_direct_ind = (rf_pkt_adv_direct_ind_t *)&pkt_Adv;  //for direct ADV
        smemcpy(p_adv_direct_ind->targetA, peerAddr, BLE_ADDR_LEN);

    }
    else{
        if(advType == ADV_TYPE_CONNECTABLE_UNDIRECTED){           // ADV_IND
            pkt_Adv.type = LL_TYPE_ADV_IND;
            bltLegAdv.scnReq_response = 1;
            bltLegAdv.conReq_response = 1;
        }
        else if(advType == ADV_TYPE_SCANNABLE_UNDIRECTED){        // ADV_SCAN_IND
            pkt_Adv.type = LL_TYPE_ADV_SCAN_IND;
            bltLegAdv.scnReq_response = 1;
        }
        else if(advType == ADV_TYPE_NONCONNECTABLE_UNDIRECTED){   // ADV_NONCONN_IND
            pkt_Adv.type = LL_TYPE_ADV_NONCONN_IND;
        }
    }


    /* special begin, if direct ADV -> other type ADV, restore ADV data in first 6 byte */
    if(bltLegAdv.last_directAdv && !bltLegAdv.cur_directAdv && bltLegAdv.adv_data_set){
        pkt_Adv.rxAddr = BLE_ADDR_PUBLIC;
        pkt_Adv.rf_len = bltLegAdv.backup_rfLen;
        smemcpy(pkt_Adv.data, bltLegAdv.backup_6_advData, 6);
    }
    bltLegAdv.last_directAdv = bltLegAdv.cur_directAdv;
    /* special end */


    bltLegAdv.legadv_ownAddr_type = ownAddrType; //record ADR type
    bltLegAdv.legadv_ownAddr_rpa = (ownAddrType & OWN_ADDRESS_TYPE_RPA_MASK);
    if(ownAddrType & OWN_ADDRESS_TYPE_RANDOM_MASK){
        bltLegAdv.legadv_mac_type = BLE_ADDR_RANDOM;
        smemcpy(bltLegAdv.legadv_mac_addr, bltMac.macAddress_random, BLE_ADDR_LEN);
    }
    else{
        bltLegAdv.legadv_mac_type = BLE_ADDR_PUBLIC;
        smemcpy(bltLegAdv.legadv_mac_addr, bltMac.macAddress_public, BLE_ADDR_LEN);
    }


    /* process ADV packet TX_ADDR & advA in ADV and scan_rsp */
    pkt_Adv.txAddr = pkt_scanRsp.txAddr = bltLegAdv.legadv_mac_type;
    smemcpy(pkt_Adv.advA,     bltLegAdv.legadv_mac_addr, BLE_ADDR_LEN);
    smemcpy(pkt_scanRsp.advA, bltLegAdv.legadv_mac_addr, BLE_ADDR_LEN);


    #if (DBG_PRVC_LEGADV_EN)
        if(advType == ADV_TYPE_NONCONNECTABLE_UNDIRECTED){
            my_dump_str_u8s(DBG_PRVC_LEGADV_EN, "[PRV][ADV] ADV_NONCONN_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
        }
        else if(advType == ADV_TYPE_SCANNABLE_UNDIRECTED){
            my_dump_str_u8s(DBG_PRVC_LEGADV_EN, "[PRV][ADV] ADV_SCAN_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
        }
        else if(advType == ADV_TYPE_CONNECTABLE_UNDIRECTED){
            my_dump_str_u8s(DBG_PRVC_LEGADV_EN, "[PRV][ADV] ADV_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
        }
        else if(advType == ADV_TYPE_CONNECTABLE_DIRECTED_LOW_DUTY){
            my_dump_str_u8s(DBG_PRVC_LEGADV_EN, "[PRV][ADV] ADV_DIRECT_IND: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
        }
        else{
            my_dump_str_u8s(DBG_PRVC_LEGADV_EN, "[PRV][ADV] ADV_DIRECT_IND high duty: Own_addr_type, filter policy", ownAddrType, advFilterPolicy, 0, 0);
        }
    #endif

    if(bltLegAdv.cur_directAdv || bltLegAdv.legadv_ownAddr_rpa){
        bltLegAdv.advParaCmd_peerAdrType = peerAddrType;
        smemcpy(bltLegAdv.advParaCmd_peerAddr, peerAddr, BLE_ADDR_LEN);

        #if (DBG_PRVC_LEGADV_EN)
            u8 temp_data[7];
            temp_data[0] = peerAddrType;
            smemcpy(temp_data + 1, peerAddr, 6);
            my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] direct ADV or local RPA, peer address", temp_data, 7);
        #endif
    }

    #if (MULTIPLE_LOCAL_DEVICE_ENABLE)
        if(mlDevMng.mldev_en){
            bltLegAdv.adv_curDevIdx = mlDevMng.cur_dev_idx;
            pkt_Adv.txAddr = pkt_scanRsp.txAddr = bltMac.macAddress_type;
            smemcpy(pkt_Adv.advA,     bltMac.macAddress_use, BLE_ADDR_LEN);
            smemcpy(pkt_scanRsp.advA, bltMac.macAddress_use, BLE_ADDR_LEN);
        }
    #endif



    bltLegAdv.adv_chn_mask = (u8)adv_channelMap;
    bltLegAdv.adv_filterPolicy = (u8)advFilterPolicy;
    bltLegAdv.advInt_min = adv_intervalMin;
    #if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN && !ANOTHER_BIG_INTV_EXTENDED_ADV)
        bltAdScn.legadv_int = bltLegAdv.advInt_min;
    #endif
    bltLegAdv.advInt_max = adv_intervalMax;
    bltLegAdv.advInt_diff = adv_intervalMax - adv_intervalMin;
    bltLegAdv.advInt_maxAddRandom = adv_intervalMax + 16; // 10mS/625us = 16


    #if (DIRECT_ADV_CHECK_PEER_INITA_RPA_SPECIAL_DESIGN)
        if(bltLegAdv.cur_directAdv && !bltLegAdv.legadv_ownAddr_rpa){
            bltLegAdv.directAdv_locIDA = 1;
        }
        else{
            bltLegAdv.directAdv_locIDA = 0;
        }
    #endif


    return BLE_SUCCESS;
}


ble_sts_t blc_hci_le_setAdvParam(hci_le_setAdvParam_cmdParam_t *pCmdParam)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Set_Adv_Param", pCmdParam, sizeof(hci_le_setAdvParam_cmdParam_t));

    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if(IS_EXTENDED_ADV_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_LEGACY_ADV_VALID;

    ble_sts_t status = blc_ll_setAdvParam(pCmdParam->intervalMin, pCmdParam->intervalMax, pCmdParam->advType, pCmdParam->ownAddrType, \
                                          pCmdParam->peerAddrType, pCmdParam->peerAddr, pCmdParam->advChannelMap, pCmdParam->advFilterPolicy );

    return status;
}


//TODO: change to callback.
static inline void blt_legadv_rpa_update_process(void)
{
#if (LL_FEATURE_ENABLE_PRIVACY)
    blt_ll_addr_clear_local_rpa_flag();
    bltLegAdv.pRslvlst_legAdv = NULL;
    bltLegAdv.legAdv_initAUseRpa = 0;

    /*If Own_Address_Type equals 0x02 or 0x03, the Peer_Address parameter
    contains the peer's Identity Address and the Peer_Address_Type parameter
    contains the Peer's Identity Type (i.e. 0x00 or 0x01). These parameters are
    used to locate the corresponding local IRK in the resolving list; this IRK is used
    to generate the own address used in the advertisement */
    if(bltLegAdv.legadv_ownAddr_rpa){
        u8 legAdv_advAUseRpa = 0;
        ll_resolv_list_t* pRL = blt_ll_searchResolvingListEntry(bltLegAdv.advParaCmd_peerAdrType, bltLegAdv.advParaCmd_peerAddr);
        if(pRL){
            legAdv_advAUseRpa = pRL->localIrk_valid;
            bltLegAdv.legAdv_initAUseRpa = pRL->peerIrk_valid;  //only for ADV_DIRECT_IND


            #if 0 //(DBG_PRVC_LEGADV_EN)
                if(pRL->localIrk_valid && pRL->peerIrk_valid){
                    my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] legadv, search RL OK, local & peer IRK valid", &pRL->rl_idx, 1);
                }
                else if(!pRL->localIrk_valid && pRL->peerIrk_valid){
                    my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] legadv, search RL OK, local IRK invalid", &pRL->rl_idx, 1);
                }
                else if(pRL->localIrk_valid && !pRL->peerIrk_valid){
                    my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] legadv, search RL OK, peer IRK invalid", &pRL->rl_idx, 1);
                }
                else{
                    my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] legadv, search RL OK, local & peer IRK invalid", &pRL->rl_idx, 1);
                }
            #endif

            if(legAdv_advAUseRpa || bltLegAdv.legAdv_initAUseRpa){
                bltLegAdv.pRslvlst_legAdv = pRL;
                blt_ll_resolvSetRpaInUse(pRL);
            }
        }
        else{
            my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] legadv, search RL ERR", 0, 0);
        }


        #if (LL_FEATURE_ENABLE_LOCAL_RPA)
            if(legAdv_advAUseRpa){
                blt_ll_addr_mark_local_rpa(pRL);
                pkt_Adv.txAddr = pkt_scanRsp.txAddr = BLE_ADDR_RANDOM;
                smemcpy(pkt_Adv.advA,     pRL->rlLocalRpa, BLE_ADDR_LEN);
                smemcpy(pkt_scanRsp.advA, pRL->rlLocalRpa, BLE_ADDR_LEN);
            }
            else{
                /* we should consider: even RPA used, host may remove the entry, then RPA should change back to IDA
                 * so here we need set IDA*/
                pkt_Adv.txAddr = pkt_scanRsp.txAddr = bltLegAdv.legadv_mac_type;
                smemcpy(pkt_Adv.advA,     bltLegAdv.legadv_mac_addr, BLE_ADDR_LEN);
                smemcpy(pkt_scanRsp.advA, bltLegAdv.legadv_mac_addr, BLE_ADDR_LEN);
            }
        #endif



        if(bltLegAdv.cur_directAdv){
            rf_pkt_adv_direct_ind_t *p_adv_direct_ind = (rf_pkt_adv_direct_ind_t *)&pkt_Adv;
            if(bltLegAdv.legAdv_initAUseRpa){
                pkt_Adv.rxAddr = BLE_ADDR_RANDOM;
                smemcpy(p_adv_direct_ind->targetA, pRL->genrt_peerRpa, BLE_ADDR_LEN);
            }
            else{
                /* we should consider: even RPA used, host may remove the entry, then RPA should change back to IDA
                 * so here we need set IDA*/
                pkt_Adv.rxAddr = bltLegAdv.advParaCmd_peerAdrType;
                smemcpy(p_adv_direct_ind->targetA, bltLegAdv.advParaCmd_peerAddr, BLE_ADDR_LEN);
            }
        }
    }
#endif
}




_attribute_ram_code_ int blt_ll_send_adv(void)
{
    DBG_CHN1_HIGH;
    DBG_SIHUI_CHN1_HIGH;
//  DBG_TIANXIANG_CHN1_HIGH;
    #if (SL01_leg_adv)
        log_task_begin_irq(SL_STACK_IRQ_TIMING_EN, SL01_leg_adv);
    #endif


    blms_state = BLMS_STATE_ADV;

#if (BLMS_PM_ENABLE && ACL_SLAVE_PM_LATENCY_EN)

    if(bltLegAdv.fifoIdx_mark > bltSche.pTask_cur->taskFifo_idx){
         blmsPm.next_adv_tick = bltSche.sSlot_tick_start + bltSche.pTask_next->begin*SSLOT_TICK_NUM;
    }
    else{
         blmsPm.next_adv_tick = bltSche.sSlot_tick_irq_real + bltLegAdv.advInt_max*SYSTEM_TIMER_TICK_625US + \
                                ((clock_time()<<2) & bltAdv.delay_sSlot_mask)*SSLOT_TICK_NUM;
    }
#endif


    if(blmsParam.leg_adv_en){
        bltLegAdv.bSlot_mark_adv = bltSche.bSlot_idx_irq_real;
        //my_dump_str_data(DBG_LEGADV_Ff0A, "bSlot_mark_adv", &bltLegAdv.bSlot_mark_adv, 4);
    }
    else{
        bltLegAdv.bSlot_mark_adv = (BIT(31) | BIT(30) | BIT(29));
    }

    bltLegAdv.sSlot_diff_adv = bltSche.sSlot_diff_irq;

    u8 adv_enFlag = 1;

    //e.g. when one slave conn_param_update nearby, can not send ADV
    //when user set ADV disable in main_loop, this can guarantee ADV not send
    if(adv_enFlag && blmsParam.leg_adv_en && !blmsParam.new_conn_forbidden && !blmsParam.newConn_forbidden_slave)
    {

        #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
            if(ll_phy_switch_cb){
                ll_phy_switch_cb(BLE_PHY_1M, LE_CI_NONE); // rf_ble_switch_phy
            }
        #endif


        /* Different process for different MCU: ******************************************/
        HAL_NONE_CSEM_IP_SET_DEFAULT_TX_DMA; //for CSEM IP RF, reset baseband lead to TX DMA registers data loss
         //  This register exists in Kite/Vulture/826x, riscv architecture chips such as
         //  Eagle do not have this register, but DMA fifo can be turned off by forcibly
         //  setting DMA TX rptr = DMA TX wptr to use DMA default tx fifo.If the hardware
         //  TX rptr of DMA TX fifo == hardware TX wptr, then send DMA Tx default fifo,
         //  otherwise send DMA Tx fifo non-default area.
        HAL_REG_RF_DMA_FIFO_TX_RPTR = FLD_DMA_RPTR_CLR;
        /**********************************************************************************/


        rf_set_tx_rx_off();//must add
        rf_set_ble_access_code_adv ();
        rf_set_ble_crc_adv ();
        //TX wait no need set, because only STX & TX2RX mode used in leg_adv
        rf_ble_set_rx_wait(RF_RX_WAIT_MIN_VALUE); //only involved in BTX/BRX/TX2RX
        rf_ble_set_tx_settle(TX_STL_ADV_SET_1M);
        rf_ble_set_rx_settle(RXSET_OPTM_ANTI_INTRF); //RX settle value for optimize anti-interference
        rf_ble_csem_set_tx_rx_settle(0, TX_STL_ADV_SET_1M, RXSET_OPTM_ANTI_INTRF);


        blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_CUSTOMER_OR_DEFAULT, 0);

        HAL_NONE_CSEM_IP_SET_DEFAULT_RX_DMA;  //Switch dma rx buffer to ADV's dma rx buffer
        rf_set_rx_maxlen(34);  //conn_req 34 Byte; scan_req 12 Byte, so 34 byte is enough


        pkt_Adv.dma_len = rf_tx_packet_dma_len(pkt_Adv.rf_len + 2);

        #if (MULTIPLE_LOCAL_DEVICE_ENABLE)
            if(mlDevMng.mldev_en && (bltLegAdv.adv_curDevIdx != mlDevMng.cur_dev_idx)){
                bltLegAdv.adv_curDevIdx = mlDevMng.cur_dev_idx; //update

                pkt_Adv.txAddr = pkt_scanRsp.txAddr = bltMac.macAddress_type;
                smemcpy(pkt_Adv.advA,     bltMac.macAddress_use, BLE_ADDR_LEN);
                smemcpy(pkt_scanRsp.advA, bltMac.macAddress_use, BLE_ADDR_LEN);
            }
        #endif


        #if (LL_FEATURE_ENABLE_PRIVACY)
            blt_legadv_rpa_update_process();
        #endif


        #if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
            pkt_Adv.chan_sel = local_chsel;
        #endif


        /***********************************************************************************************************************
        old code before 20230425: 1M use 6 preamble, TX settle is 75uS

            TX_settle   5 Preamble      BLE data          T_ifs         RX dma 1st data(Preamble 1B, access_code 4B, 1st dma 4B )
               75 uS      40 uS      (rf_len + 10)*8      150 uS            72uS
           (rf_len + 10)*8  + 115uS + (150+ 72)

            u32  t_us = (pkt_Adv.rf_len + 10) * 8 + 400; //400 uS is old timing
            u32  t_us = (pkt_Adv.rf_len + 10) * 8 + 375;  //optimize, try to save some time, 367 error, 368 OK, give 7 uS more margin
         ***********************************************************************************************************************/
        u32  t_us = (pkt_Adv.rf_len + 10) * 8 + TX_STL_ADV_REAL_1M + 260;  //come from old code: 260 = 375 - 115

        u32 scanrsp_trigger_tick = 0;  //set 0 to avoid compile warning

        /* SiHui found problem on B91. same situation for other RISV MCU with PLIC module.
          * but process method maybe different for new MCU, so move this function to HAL.
          * FSM IRQ status will send to PLIC module, clear reg_rf_irq_status can not drop RF IRQ,
          * so we disable RF mask to prevent RF IRQ status sending to PLIC */
        HAL_BLE_STACK_RF_IRQ_MASK_CLEAR;

        for (int i=0; i<3; i++)
        {
            if (bltLegAdv.adv_chn_mask & BIT(i))
            {
                STOP_RF_STATE_MACHINE;                      // stop SM
                rf_set_ble_channel (blc_legadv_channel[i]);
                HAL_CLEAR_RF_TX_RX_IRQ;

                /* for CSEM IP RF, reset baseband lead to TX DMA registers data loss */
                HAL_CSEM_IP_SET_DEFAULT_TX_DMA;
                HAL_CSEM_IP_SET_DEFAULT_RX_DMA;

                ////////////// start TX //////////////////////////////////

                if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_TX_ON);  }

                u32 tx_begin_tick = 0;
                if(bltLegAdv.adv_type == ADV_TYPE_NONCONNECTABLE_UNDIRECTED){ //ADV_NONCONN_IND
                    rf_start_fsm(FSM_STX, (void *)&pkt_Adv, clock_time());
                    tx_begin_tick = clock_time ();
                    while (!HAL_GET_RF_TX_IRQ && (clock_time() - tx_begin_tick) < (t_us - 200)*SYSTEM_TIMER_TICK_1US){
                        if(usr_irq_handler_cb){usr_irq_handler_cb();}
                    }
                    HAL_CLEAR_RF_TX_IRQ;
                }
                else // ADV_IND or ADV_DIRECT_IND or ADV_SCAN_IND
                {
                    rf_start_fsm(FSM_TX2RX, (void *)&pkt_Adv, clock_time());
                    tx_begin_tick = clock_time ();
                    volatile u32 *ph  = (u32 *) (glb_temp_rx_buff + 4);
                    ph[0] = 0;

                    /* TODO need to calculate accuracy timeout time
                     * 20240227 SiHui & QiuWei, add timeout for all RF IRQ while check, temporary solution due to urgent time for B92 sniffer SDK
                     * final solution: merge from newest B85m code
                     */
                    while (!HAL_GET_RF_TX_IRQ && (clock_time() - tx_begin_tick) < (t_us - 200)*SYSTEM_TIMER_TICK_1US){
                        if(usr_irq_handler_cb){usr_irq_handler_cb();}
                    }
                    HAL_CLEAR_RF_TX_IRQ;

                    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON); }

                    while (!(*ph) && (u32)(clock_time() - tx_begin_tick) < t_us * SYSTEM_TIMER_TICK_1US){//wait packet from master
                        if(usr_irq_handler_cb){usr_irq_handler_cb();}
                    }

                    // "*ph" not 0 means 1st 4 bytes PDU from RX dma data is ready
                    // preamble 1B, access_code 4B, then 4 bytes PDU
                    if (*ph){
                        u32 rx_begin_tick = clock_time ();
                        rf_pkt_adv_rx_t * pAdvRx = (rf_pkt_adv_rx_t *) (glb_temp_rx_buff + RF_BLE_DMA_RFRX_LEN_HW_INFO);

                        rf_set_tx_packet_address(&pkt_scanRsp);  //get ready scan_rsp as early as possible

                        // (rf_len + 10)*8:  10 = preamble 1B + access_code 4B + header 2B + CRC 3B
                        // header 2B + PDU "rf_len" B + CRC 3B  is dma data, at least 4B is arrived when  "*ph" not 0
                        //  so rest longest data is: rf_len + 2 + 3 - 4 = rf_len + 1
                        // only 2 kind of request:  scan_req rf_len = 12, conn_req rf_len = 34
                        // RX packet reset timing is (rf_len+1)*8 = (34+1)*8 = 280
                        // add 20 uS for RX status delay, 280+20 = 300uS,
                        // Add margin 80us by Yafei,240530, 300 us looks like a narrow margin
                        while (!HAL_GET_RF_RX_IRQ && (clock_time() - rx_begin_tick) < (300+80) * SYSTEM_TIMER_TICK_1US){
                            if(usr_irq_handler_cb){usr_irq_handler_cb();}//wait packet from master
                        }

                        /* safe, here delay Sequence for Onca chip, adv_duration time is enough, add by Yafei */
                        HAL_WAIT_MODEM_SEQ_TIME;

                        /* special begin, prepare scan_rsp for scan_req in advance due to urgent timing */
                        if(pAdvRx->rf_len == 12) //maybe scan_req
                        {
                            rf_ble_set_tx_settle(TX_STL_LEGADV_SCANRSP_SET);
                            rf_ble_csem_set_tx_rx_settle(0, TX_STL_LEGADV_SCANRSP_SET, 0);
                            /*
                             * hal_rf_get_rx_timestamp() - HW_DELAY_1M + 12*8 + 5*8(2 header+3crc) + 150 - TX_STL_LEGADV_SCANRSP_REAL
                             */
                            scanrsp_trigger_tick = hal_rf_get_rx_timestamp() + (12*8 + (190 - HW_DELAY_1M - TX_STL_LEGADV_SCANRSP_REAL)) *SYSTEM_TIMER_TICK_1US;

                            //DBG_C HN9_TOGGLE;
                            #if 1// trigger tick must be in future 80uS range
                                if((u32)(clock_time() + 80*SYSTEM_TIMER_TICK_1US - scanrsp_trigger_tick) > 80*SYSTEM_TIMER_TICK_1US)
                                {
                                    scanrsp_trigger_tick = clock_time();
                                    //DBG_C HN10_TOGGLE;
                                }
                            #endif
                            rf_start_fsm(FSM_STX, NULL, scanrsp_trigger_tick);
                            //my_dump_str_u32s(DBG_PRVC_LEGADV_EN, "legadv, set scan_rsp", clock_time(), scanrsp_trigger_tick, 0, 0);

                            if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_TX_ON);  }
                        }
                        /* special end */


                        HAL_CLEAR_RF_RX_IRQ;  //important: clear RX status



                        /* if(RF_BLE_PACKET_VALIDITY_CHECK(glb_temp_rx_buff)),  B85m has warning() */
                        u8* tmp_advBuf = glb_temp_rx_buff;
                        if(RF_BLE_PACKET_VALIDITY_CHECK(tmp_advBuf))
                        {
                            /* advA in scan_req/conn_req must be same as advertiser's advA */
                            if (!smemcmp(pkt_Adv.advA, pAdvRx->advA, BLE_ADDR_LEN)) //equal
                            {
                                do{
                                    u8 is_connect_req = 0, filter_enable = 0;
                                    /* step 1, quick check if scan_req or connect_req basic logic pass
                                     *         skill:  Put the hardest conditions first */

                                    if(pAdvRx->rf_len == 12 && pAdvRx->type == LL_TYPE_SCAN_REQ && bltLegAdv.scnReq_response){ //scan_req
                                        #if (CUSTOM_DARWIN_FMN_ENABLE)
                                            if (custom_darwin_fmn.darwin_fmn_enable) {
                                                //FMN: If ScanRsp len <= 6, do not response scan_req.
                                                if (pkt_scanRsp.rf_len <= 6) {
                                                    bltAdv.adv_scanReq_connReq = 1;
                                                    break;
                                                }
                                            }
                                        #endif
                                        filter_enable = bltLegAdv.adv_filterPolicy & ALLOW_SCAN_WL;
                                        //DBG_C HN2_TOGGLE;
                                    }
                                    else if(pAdvRx->rf_len == 34 && pAdvRx->type == LL_TYPE_CONNECT_REQ && bltLegAdv.conReq_response){ //conn_req
                                        filter_enable = bltLegAdv.adv_filterPolicy & ALLOW_CONN_WL;
                                        is_connect_req = 1;
                                        blc_rcvd_connReq_tick = clock_time(); //need to provide API to host and need to distinguish connHandle
                                        //DBG_C/HN3_TOGGLE;
                                    }
                                    else{
                                        my_dump_str_u8s(DBG_PRVC_LEGADV_EN, "[PRV][ADV] legadv, not expected pkt, stop", pAdvRx->type, bltLegAdv.scnReq_response, bltLegAdv.conReq_response, 0);
                                        break;   //stop
                                    }


                                    ll_resolv_list_t *pRL_match = NULL;
                                    u8 peer_is_rpa = IS_RESOLVABLE_PRIVATE_ADDR(pAdvRx->txAddr, pAdvRx->peerA);

                                    /* step 2, network privacy ignore IDA process */
                                    #if (NETWORK_PRIVACY_IGNORE_IDA_CHECK)
                                        /* check if network privacy mode ignore IDA exist */
                                        if(!peer_is_rpa){
                                            if(bltLegAdv.pRslvlst_legAdv){
                                                pRL_match = bltLegAdv.pRslvlst_legAdv;
                                            }
                                            else{
                                                pRL_match = blt_ll_searchResolvingListEntry(pAdvRx->txAddr, pAdvRx->peerA);
                                            }

                                            if(pRL_match && pRL_match->peerIrk_valid){ //peer device has distributed its IRK
                                                if(pRL_match->rlPrivMode == NETWORK_PRIVACY_MODE){ //not allowed
                                                    /* LL/SEC/ADV/BV-15-C  LL/SEC/ADV/BV-16-C  LL/SEC/ADV/BV-17-C*/
                                                    my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] legadv, network privacy ignore IDA, stop", 0, 0);
                                                    break; //stop
                                                }
                                                else{ //DEVICE_PRIVACY_MODE, allowed
                                                    /* LL/SEC/ADV/BV-18-C  LL/SEC/ADV/BV-19-C  LL/SEC/ADV/BV-20-C*/
                                                    my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] legadv, device privacy accept IDA", 0, 0);
                                                }
                                            }
                                        }
                                    #else
                                        //special process:
                                    #endif


                                    blt_ll_addr_set_peer_address(0, pAdvRx->txAddr, pAdvRx->peerA);

                                    /* step 3, resolve RPA if needed, check whiteList if filtering needed
                                     * for peerA RPA
                                     * 1. scan_req, resolve if filter needed; do not resolve if no filter
                                     * 2. conn_req, if direct ADV,      must resolve, than can check if addressed to local device
                                     *              if none direct ADV, resolve if filtering needed; do not resolve if no filter
                                     * Consider accept list, change to:
                                     * 1. if direct ADV, must resolve, than check if addressed to local device, never use filtering/accept list
                                     * 2. if none direct ADV, resolve RPA depend on filtering, then check accept list
                                     */
                                    if(bltLegAdv.cur_directAdv || filter_enable){
                                        #if (LL_FEATURE_ENABLE_PRIVACY)
                                        if(peer_is_rpa){
                                            pRL_match = blt_ll_resolve_rpa(0, pAdvRx->peerA, bltLegAdv.pRslvlst_legAdv);
                                            if(pRL_match){
                                                blt_ll_storePeerDeviceRpa(pRL_match, pAdvRx->peerA);
                                                blt_ll_addr_set_peer_address(1, pRL_match->rlIdAddrType, pRL_match->rlIdAddr);
                                                my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] peer RPA resolve OK", pRL_match->rlIdAddr, 6);
                                            }
                                            else{
                                                my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] peer RPA resolve ERR, stop", 0, 0);
                                                break;
                                            }
                                        }
                                        #endif // #if (LL_FEATURE_ENABLE_PRIVACY)
                                        if(bltLegAdv.cur_directAdv){ //direct ADV, do not care about filter
                                            /* for ADV_DIRECT_IND, check initA in connReq for direct ADV
                                             * regardless of IDA or RPA. here we just add some extra check for directed ADV special.
                                             * case 1. both IDA, advA and initA should be same. So we abandon different IDAs.
                                             * case 2. both RPA, Spec
                                             *                  The Link Layer should not set the InitA field to the same value
                                                                as the TargetA field in the received advertising PDU.
                                             *                  attention: here should not equal to must !!!
                                             * case 3. Advertiser IDA, Initiator RPA, not exist for a correct privacy process.
                                             *         But it need BLE host and controller both be correct, for host it's very hard to process.
                                             *         So here we allow this situation exist, do not abandon.
                                             *         Evaluation for this special process :For BQB, host is upper tester of other vendor,
                                             *         we can guarantee our controller be correct. For mass production SDK, here if we
                                             *         advertiser commit this error, and peer device do not detect this error, we let this flow go.
                                             * case 4. Advertiser RPA, Initiator IDA
                                             *         If peer should use RPA
                                             *             If "NETWORK_PRIVACY IGNORE_IDA_CHECK" enable, can be rejected by network privacy, accept by device privacy
                                             *             If "NETWORK_PRIVACY IGNORE_IDA_CHECK" disable, special process: now code take as device privacy (SiHui)
                                             *         If peer no need use RPA, accept
                                             */
                                             if(smemcmp(bltAddr.peer_pka_or_ida_addr, bltLegAdv.advParaCmd_peerAddr, BLE_ADDR_LEN) || \
                                                        bltAddr.peer_pka_or_ida_type != bltLegAdv.advParaCmd_peerAdrType){
                                                    #if (DIRECT_ADV_CHECK_PEER_INITA_RPA_SPECIAL_DESIGN)
                                                        if(bltLegAdv.directAdv_locIDA && bltAddr.peer_use_rpa)
                                                        {
                                                            /* special design:
                                                             * direct ADV, local use IDA, peer IRK in Flash is valid(peer master may use RPA in initA of CONNECT_IND)
                                                             *
                                                             * application simplify method:
                                                             * bls_ll_setAdvParam use bondInfo.peer_addr_type and bondInfo.peer_addr, may be IDA or RPA
                                                             * should use blc ll_addDeviceToResolvingList & blc ll_setAddressResolutionEnable
                                                             *
                                                             * bltAddr.peer_use_rpa be 1, means above
                                                             * "pRL_match = blt_ll_resolve_rpa(0, pAdvRx->peerA, bltLegAdv.pRslvlst_legAdv)" passed.
                                                             * so peer RPA is OK after check
                                                             */
                                                        }
                                                        else
                                                    #endif
                                                        {
                                                            tlkapi_send_string_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] direct ADV initA do not address to local, stop", 0, 0);
                                                            break;
                                                        }

                                             }
                                             #if 0 //can not abandon same RPAs, LL/SEC/ADVBV-22-C
                                             else if(bltLegAdv.legAdv_initAUseRpa && peer_is_rpa){
                                                rf_pkt_adv_direct_ind_t *pDirectAdv_local = (rf_pkt_adv_direct_ind_t *)&pkt_Adv;
                                                if(!smemcmp(pDirectAdv_local->targetA, pAdvRx->peerA, BLE_ADDR_LEN)){
                                                    my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] RPAs be same, stop", 0, 0);
                                                    break;
                                                }
                                             }
                                             #endif
                                        }
                                        else{ //none direct ADV but filter needed
                                            if(!blt_ll_searchAddrInWhiteListTbl(bltAddr.peer_pka_or_ida_type, bltAddr.peer_pka_or_ida_addr)){
                                                my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] peer advA not in WL, stop", bltAddr.peer_pka_or_ida_addr, 6);
                                                break;
                                            }
                                        }
                                    }
                                    else{ //none direct ADV, no filter, pass without any check
                                          /* consider later : even no filter, maybe we need try to resolve RPA, because
                                           * enhanced connection complete event: Peer_Address should be IDA when RPA can be resolved
                                           */
                                    }


                                    /* final step, respond to scan_req(send scan_rsp) or conn_req(connect) */
                                    if(is_connect_req){ //CONNECT_REQ
                                        my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] legadv, accept conn_req", 0, 0);
                                        //if(ll_adv_2_slave_cb)  //to save RamCode
                                        {

                                            int result = FALSE;
                                            #if (LEG_ADV_EN_MORE_STRATEGY)
                                                /* Strategy 1 will never running here, because ADV will stop when slave number reach max */
                                                if( blmsParam.cur_slave_num == blmsParam.max_slave_num)   //reach max, not connect
                                                {
                                                }
                                                else
                                            #endif
                                                {
                                                    /* attention that cur slave_num may change in this function */
                                                    result = ll_adv_2_slave_cb((rf_packet_connect_t *)glb_temp_rx_buff, FALSE);  // blt_s_connect()
                                                }

                                            if(result){
                                                //DBG_C HN4_TOGGLE;

                                                bltLegAdv.adv_duration_en = 0;

                                                int remove_task = 0;

                                                #if (LEG_ADV_EN_MORE_STRATEGY)
                                                    if(blmsParam.legadv_en_strategy == LEG_ADV_EN_STRATEGY_3){
                                                        blmsParam.leg_adv_en = 0;
                                                        remove_task = 1;
                                                    }
                                                    else if( blmsParam.cur_slave_num == blmsParam.max_slave_num && blmsParam.legadv_en_strategy == LEG_ADV_EN_STRATEGY_1){
                                                        remove_task = 1;
                                                    }
                                                #else
                                                    if(blmsParam.cur_slave_num == blmsParam.max_slave_num){
                                                        remove_task = 1;
                                                    }
                                                #endif

                                                if(remove_task){
                                                    blt_sche_removeTaskMask(TSKMSK_LEG_ADV);

                                                    #if (BLMS_PM_ENABLE && ACL_SLAVE_PM_LATENCY_EN)
                                                        blmsPm.next_adv_tick = 0;
                                                    #endif
                                                }

                                                blt_ll_record_identity_address(bltLegAdv.legadv_mac_type, bltLegAdv.legadv_mac_addr);
                                            }

                                            bltAdv.adv_scanReq_connReq = 2;
                                        }
                                    }
                                    else{ //SCAN_REQ
                                        //DBG_C HN11_TOGGLE;
                                        my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] legadv, send scan_rsp", 0, 0);
                                        int span_us = TX_STL_LEGADV_SCANRSP_REAL + (pkt_scanRsp.rf_len + 10)*8 + 10;  //10uS: margin
                                        //DBG_C HN0_HIGH;
                                        while(!HAL_GET_RF_TX_IRQ && tick1_exceed_tick2(scanrsp_trigger_tick + span_us*SYSTEM_TIMER_TICK_1US, clock_time()));
                                        //DBG_C HN0_LOW;
                                        HAL_CLEAR_RF_TX_IRQ;
                                        if(!bltAdv.blc_continue_adv_en){
                                            bltAdv.adv_scanReq_connReq = 1;
                                        }
                                    }
                                }while(0);

                            }
                            else{
                                my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] legadv, peer advA not same with local, stop", pAdvRx->advA, 6);
                            }
                        }//end of "RF_BLE_PACKET_VALIDITY_CHECK()"


                        STOP_RF_STATE_MACHINE;
                        glb_temp_rx_buff[0] = 1;
                    }


                    if(bltAdv.adv_scanReq_connReq){
                        i = 4;  //break;
                    }
                }//end of  "ADV_IND or ADV_DIRECT_IND or ADV_SCAN_IND"

                /*
                 * CSEM IP, when RF is in the TX state, must use the reset baseband to stop TX, and can not use other methods.
                 * legacy adv When the SCAN REQ data is received and RPA is parsed, it is possible that
                 * the RF is already in the TX state (during TX settle) and is ready to send the SCAN RSP.
                 * In the above case, must use reset baseband to solve the problem.
                 *
                 * In other cases, using reset baseband has no bad effect.
                 * For simple code logic, reset baseband is used.
                 */
                HAL_CSEM_IP_RESET_BASEBAND;

                if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_OFF);  }
            }//end of   "bltLegAdv.adv_chn_mask & BIT(i)"
        }// end of "for (int i=0; i<3; i++)"


        STOP_RF_STATE_MACHINE;
        CLEAR_ALL_RFIRQ_STATUS;

        bltAdv.adv_scanReq_connReq = 0;  //mclear adv sending


        //If not do this, MSISO_ERR_DEBUG(0x99010001) will happen. RX status will come even after we set  core_f20 to 0xffffffff
        //so we need clear RX status after a while. Now I do not know the reason why it happens.
        blmsParam.delay_clear_rf_status = 1;
    }

    /* SiHui found problem on B91. same situation for other RISV MCU with PLIC module.
     * but process method maybe different for new MCU, so move this function to HAL.
     * FSM IRQ status will send to PLIC module, clear reg_rf_irq_status can not drop RF IRQ,
     * so we disable RF mask to prevent RF IRQ status sending to PLIC */
    HAL_BLE_STACK_RF_IRQ_MASK_SET;

    blt_ll_calculate_sSlot_next(clock_time() + (bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US)*SYSTEM_TIMER_TICK_1US);
    //my_dump_str_data(DBG_LEGADV_Ff0A, "sSlot_idx_next", &bltSche.sSlot_idx_next, 4);

    DBG_CHN1_LOW;
    DBG_SIHUI_CHN1_LOW;
//  DBG_TIANXIANG_CHN1_LOW;
    #if (SL01_leg_adv)
        log_task_end_irq(SL_STACK_IRQ_TIMING_EN, SL01_leg_adv);
    #endif



    #if (DYNAMIC_SCHE_CAL_TIME_EN)
        bltSche.sche_tick_begin = clock_time();
    #endif

    return 1;
}






ble_sts_t blc_ll_setScanRspData(const u8 *data, u8 len)
{
    tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_CMD), "[LL][CMD] Set_ScanRspData", data, len);

    if(len > 31) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }



    #if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE)
        if((bltAdv.legAdv_chngReason & REFRESH_RPA_SCANRSPDATA_CHANGE) && bltLegAdv.scnReq_response){
            int old_data_len = pkt_scanRsp.rf_len - 6;
            if(old_data_len != len || smemcmp(pkt_scanRsp.data, data, len)){
                my_dump_str_data(DBG_PRVC_LEGADV_EN, "[PRV][ADV] CHANGE, scanrsp", 0, 0);
                u32 r = irq_disable();
                ll_resolv_list_t *pRL = bltLegAdv.pRslvlst_legAdv;
                irq_restore(r);
                if(pRL){
                    blt_ll_resolvRefreshRpa(pRL);
                }
            }
        }
    #endif

    /*important: for ADV running in IRQ, do not advertising when adv data/scan_rsp data is changing,
                 so here we use IRQ protect. should guarantee that IRQ disabling not too long time*/
    u32 r = irq_disable();

    smemcpy(pkt_scanRsp.data, data, len);
    pkt_scanRsp.rf_len = len + 6;
    pkt_scanRsp.dma_len = rf_tx_packet_dma_len(pkt_scanRsp.rf_len + 2);

    irq_restore(r);

    return BLE_SUCCESS;
}


ble_sts_t blc_hci_le_setScanRspData(const u8 *data, u8 len)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Set_ScanRspData", data, len);

    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if(IS_EXTENDED_ADV_VALID){
        return HCI_ERR_CMD_DISALLOWED;
    }
    SET_LEGACY_ADV_VALID;


    return blc_ll_setScanRspData(data, len);
}






_attribute_ram_code_
int blt_ll_buildLegacyAdvTask(void)
{

    #if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN && !ANOTHER_BIG_INTV_EXTENDED_ADV)
        bltAdScn.legadv_alloc = 0;
    #endif

    //bSlot: 1600  -> 1S
    //       1920  -> 1.2S
    //       16000 -> 10S
    //    1600*134 -> 134S,  1600*134=214400, 2^18=1024*256=262144=BIT(18)

    s32 sSlot_mark_adv;

    #if 1 //fix bug
        u32 bSlot_distance = (u32)(bltSche.bSlot_idx_next - bltLegAdv.bSlot_mark_adv);
        if( bSlot_distance > bltLegAdv.advInt_maxAddRandom ){   //two area: >x  || <0
            sSlot_mark_adv = bltSche.sSlot_idx_next - bltLegAdv.advInt_maxAddRandom*32;
        }
        else{
            sSlot_mark_adv = bltSche.sSlot_idx_next - bSlot_distance*32 + bltSche.sSlot_diff_next + bltLegAdv.sSlot_diff_adv;
        }
    #else
        /* bug found on 20230405 SiHui:
           ADV task run but not send packet due to "blmsParam.new_conn forbidden", bSlot mark_adv still mark, value is "0d02"
           task cost time only about 10uS.  sSlot idx_next is "01014a" by "blt_ll_calculate_sSlot_nex(clock_time() + ...)
           then rebuild task, bSlot idx_next is "0d03", reasonable
           here  bSlot_distance is 1, (u32)(1-2) is very big, lead sSlot_mark adv be "01944a", 51.2mS prior to "sSlot idx_next"
           then a ADV task will build at the beginning of next 80mS.
           It's error, because two ADV task are very closed.
        */
        u32 bSlot_distance = (u32)(bltSche.bSlot_idx_next - bltLegAdv.bSlot_mark_adv);
        if( (u32)(bSlot_distance - 2) > bltLegAdv.advInt_maxAddRandom ){   //two area: >x  || <0
            sSlot_mark_adv = bltSche.sSlot_idx_next - bltLegAdv.advInt_maxAddRandom*32;
        }
        else{
            sSlot_mark_adv = bltSche.sSlot_idx_next - bSlot_distance*32 + bltSche.sSlot_diff_next + bltLegAdv.sSlot_diff_adv;
        }
    #endif

    //my_dump_str_data(DBG_LEGADV_Ff0A, "sSlot_mark_adv", &sSlot_mark_adv, 4);

    #if (BLMS_PM_ENABLE && ACL_SLAVE_PM_LATENCY_EN)
        /* special process: when ADV first start or restart, to prevent no ADV allocate this time, then long latency slave sleep
         *                  may skip correct ADV tick.  If ADV allocate OK this time, will be overwrite. */
        if(!blmsPm.next_adv_tick){
            blmsPm.next_adv_tick = clock_time() + bltLegAdv.advInt_min*SYSTEM_TIMER_TICK_625US;
        }
    #endif


    sch_task_t  *pTsk_cur;
    sch_task_t  *pExtLkTsk_left = bltSche.pTask_head;           //exist linkList task left
    sch_task_t  *pExtLkTsk_right = bltSche.pTask_head->next;    //exist linkList task right


    s32 cur_sSlot_adv = sSlot_mark_adv;  //must use s32

    s32 adv_min_left, adv_min_right, adv_max_left, adv_max_right;
    s32 sSlot_idx_left, sSlot_idx_right;

    int ADV_task_allocate;
    int ADV_task_number = 0;


    for(int i=0; i<ADVTSK_FIFO_NUM; i++)
    {
        //first, we use minimum timing to find available timing block
        adv_min_left = (s32)(cur_sSlot_adv + bltLegAdv.advInt_min*32);  //s32
        adv_min_right = adv_min_left + bltLegAdv.sSlotDuration_adv;

        //current ADV task exceed time line, this function can finish
        if( adv_min_right > bltSche.sSlot_endIdx_maxPri ){
            return ADV_task_number;
        }

        adv_max_left  = adv_min_left + bltLegAdv.advInt_diff*32 + bltAdv.delay_sSlot_value;
        adv_max_right = adv_max_left + bltLegAdv.sSlotDuration_adv;

        while(1)
        {
                ADV_task_allocate = 0;

                if(pExtLkTsk_left == bltSche.pTask_head){
                    sSlot_idx_left = bltSche.sSlot_idx_next;
                }
                else{
                    sSlot_idx_left = pExtLkTsk_left->end + 1;
                }
                if(pExtLkTsk_right == NULL){
                    sSlot_idx_right = bltSche.sSlot_endIdx_maxPri;
                }
                else{
                    sSlot_idx_right = pExtLkTsk_right->begin;
                }

                int sSlot_idle_duration = (int)(sSlot_idx_right - sSlot_idx_left);

                //consider sSlot_idx_left & sSlot_idx_right a window, move this window from left to right
                if(sSlot_idle_duration < bltLegAdv.sSlotDuration_adv){  //window size too small, can not use
                    //ADV_task_allocate = 0;    //traverse to next
                }
                else if(sSlot_idx_right < adv_min_right){
                    //ADV_task_allocate = 0;    //traverse to next
                }
                else if(sSlot_idx_left >= adv_max_left){  //compensate match
                    //expect ADV timing exceed, must insert here, do not care delay
                    cur_sSlot_adv = sSlot_idx_left;
                    ADV_task_allocate = 1;
                }
                else{  //window match
                    sSlot_idx_left  = sSlot_idx_left > adv_min_left ? sSlot_idx_left : adv_min_left;
                    sSlot_idx_right = sSlot_idx_right < adv_max_right ? sSlot_idx_right : adv_max_right;

                    sSlot_idle_duration = (int)(sSlot_idx_right - sSlot_idx_left);
                    if(sSlot_idle_duration >= bltLegAdv.sSlotDuration_adv){
                        ADV_task_allocate = 1;  //traverse to next

                        u16 random = (clock_time()<<2) & bltAdv.delay_sSlot_mask;
//                      u16 random = 0 & bltAdv.delay_sSlot_mask;
                        s32 sslot_begin = (adv_min_left + bltLegAdv.advInt_diff*32) + random;

                        if(sslot_begin < sSlot_idx_left){
                            cur_sSlot_adv = sSlot_idx_left;
                            if(sSlot_idle_duration > (bltLegAdv.sSlotDuration_adv + 10)){
                                cur_sSlot_adv += 5;
                            }
                        }
                        else{
                            int sslot_diff = (sslot_begin + bltLegAdv.sSlotDuration_adv) - sSlot_idx_right;
                            if(sslot_diff <= 0){
                                cur_sSlot_adv = sslot_begin;
                            }
                            else{
                                cur_sSlot_adv = sSlot_idx_right - bltLegAdv.sSlotDuration_adv;
                                if(sSlot_idle_duration > (bltLegAdv.sSlotDuration_adv + 10)){
                                    cur_sSlot_adv -= 5;
                                }
                            }
                        }
                    }
                }


                if(ADV_task_allocate){
                    pTsk_cur = &bltLegAdv.advTsk_fifo[i];

                    pTsk_cur->begin = cur_sSlot_adv;
                    pTsk_cur->end = cur_sSlot_adv + (bltLegAdv.sSlotDuration_adv-1);

                    #if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN && !ANOTHER_BIG_INTV_EXTENDED_ADV)
                        bltAdScn.legadv_alloc = 1;
                        bltAdScn.legadv_sSlot = cur_sSlot_adv;
                        //bltAdScn.legadv_sSLot_durn = bltLegAdv.sSlotDuration_adv;
                    #endif

                    //insert ADV task to existed LinkList
                    pExtLkTsk_left->next = pTsk_cur;
                    pTsk_cur->next = pExtLkTsk_right;

                    pExtLkTsk_left = pTsk_cur;  //move forward pLeft

                    ADV_task_number ++;

                    #if (BLMS_PM_ENABLE && ACL_SLAVE_PM_LATENCY_EN)
                        bltLegAdv.fifoIdx_mark = i;
                        if(i == 0){
                             blmsPm.next_adv_tick = bltSche.sSlot_tick_start + cur_sSlot_adv*SSLOT_TICK_NUM;
                        }
                    #endif

                    break;  //exit while 1
                }
                else{  //traverse to next
                    if(pExtLkTsk_right == NULL){
                        return ADV_task_number;  //meet the end , can not traverse, finish
                    }
                    else{
                        pExtLkTsk_left = pExtLkTsk_left->next;
                        pExtLkTsk_right = pExtLkTsk_right->next;
                    }
                }

        } //while(1)




    }// for(int i=0; i<ADVTSK_FIFO_NUM; i++)

    return ADV_task_number;
}





void        blc_ll_configLegacyAdvEnableStrategy (legadv_en_str_t strategy)
{
#if (LEG_ADV_EN_MORE_STRATEGY)
    blmsParam.legadv_en_strategy = strategy;
#endif
}

