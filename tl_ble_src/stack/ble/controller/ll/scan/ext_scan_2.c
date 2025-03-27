/********************************************************************************************************
 * @file    ext_scan_2.c
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


#if (LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)



#define     SCAN_TIME_COMPARE_SMALL(t1,t2)              ( (u32)((t2) - (t1)) < BIT(30)  )
#define     TICK_DIFFER_LESS_THAN(t1, t2, diff)          ( (u32)((t2) + (diff) - (t1)) < (diff)*2  )



/*           *  *      PHYs           timing(uS)
 *   1M PHY   :    (rf_len + 10) * 8,      // 10 = 1(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
 *   2M PHY   :    (rf_len + 11) * 4       // 11 = 2(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
 *   Coded PHY :    376 + (rf_len*8+43)*S  // 376uS = 80uS(preamble) + 256uS(Access Code) + 16uS(CI) + 24uS(TERM1)
 *                = rf_len*S*8 + 43*S + 376
 */
/*
    if(cur_pextscn->aux_secPhy == BLE_PHY_1M){
        if(cur_adv_mode == LL_EXTADV_MODE_NON_CONN_NON_SCAN){
            aux_pkt_us = (rf_len + 10)*8;
                       = rf_len*8 + 80
                    rf_len = 255 : 2040 + 80 = 2120 uS
                    rf_len = 128 : 1024 + 80 = 1104 uS
                    rf_len =  64 :  512 + 80 =  592 uS
                    rf_len =  32 :  256 + 80 =  336 uS
        }
        else if(cur_adv_mode == LL_EXTADV_MODE_CONN){
          initiating:
            aux_pkt_us = (rf_len + 34 + 14 + 10*3)*8 + 300; //aux_conn_req(34), aux_conn_rsp(14)
                       = rf_len*8 + 924
                    rf_len = 255 : 2040 + 924 = 2964 uS
                    rf_len = 128 : 1024 + 924 = 1948 uS
                    rf_len =  46 :  368 + 924 = 1292 uS    (15+31=46)

          no initiating:
            aux_pkt_us = (rf_len + 10)*8



            improve later:
                    for ADV_EXT_IND, no "AdvA" on connectable, can not know whether need send "AUX_CONNECT_REQ", so we can allocate timing for only
                    AUX_ADV_IND, then check if ext_init_en and "AdvA" match, if all match, use forcing add task(if priority high)

        }
        else{
          ACTIVE_SCAN:
                //17: 2 + AdvA(6) + TargetA(6) + ADI(2) + TX_POWER(1);  attention: here do not consider "ACAD" though it's optional, if peer device add "ACAD", timing may error
                //aux_scan_req(12), rf_len is for aux_scan_rsp
                aux_pkt_us = (17 + 12 + rf_len + 10*3)*8 + 300;
                           = rf_len*8 + 772
                        rf_len = 255 : 2040 + 772 = 2812 uS
                        rf_len = 128 : 1024 + 772 = 1796 uS


                                        SiHui design note:
                        for ADV_EXT_IND, no "AdvA" on scannable, can not know whether need send "AUX_SCAN_REQ", so we can allocate timing for only
                        AUX_ADV_IND, then check if active scan enable and "AdvA" match, if all match, check timing gap to decide if sending
                        "AUX_SCAN_REQ" (usually this command succeed once is enough, so no need consider too much timing priority).

          PASSIVE_SCAN:
                aux_pkt_us = (17   + 10)*8 = 216 uS  (do not consider "ACAD")
                aux_pkt_us = (1+64 + 10)*8 = 600 uS  (consider "ACAD", ext_header MAX length is 64)




        }
    }
    else if(cur_pextscn->aux_secPhy == BLE_PHY_2M){
        if(cur_adv_mode == LL_EXTADV_MODE_NON_CONN_NON_SCAN){
            aux_pkt_us = (rf_len + 11)*4;
                       = rf_len*4 + 44
                    rf_len = 255 : 1020 + 44 = 1064 uS
                    rf_len = 128 :  512 + 44 =  556 uS
        }
        else if(cur_adv_mode == LL_EXTADV_MODE_CONN){
            //336*4+300 = 1644
            aux_pkt_us = (rf_len + 34 + 14 + 11*3)*4 + 300; //aux_conn_req(34), aux_conn_rsp(14)
                       = rf_len*4 + 624
                    rf_len = 255 : 1020 + 624 = 1644 uS
                    rf_len = 128 :  512 + 624 = 1136 uS
                    rf_len =  46 :  184 + 624 =  808 uS    (15+31=46)
        }
        else{
            //317*4+300=1568
            aux_pkt_us = (17 + 12 + rf_len + 11*3)*4 + 300; //17: 2 + AdvA(6) + TargetA(6) + ADI(2) + TX_POWER(1); aux_scan_req(12), aux_scan_rsp(255)
                       = rf_len*4 + 544
                    rf_len = 255 : 1020 + 548 = 1568 uS
                    rf_len = 128 :  512 + 548 = 1060 uS

            remove "AUX_SCAN_REQ" and "AUX_SCAN_RSP"
                     aux_pkt_us = (17 + 11)*4 ; //17: 2 + AdvA(6) + TargetA(6) + ADI(2) + TX_POWER(1);
                                = 112 uS


            ACTIVE_SCAN:
            //17: 2 + AdvA(6) + TargetA(6) + ADI(2) + TX_POWER(1);  attention: here do not consider "ACAD" though it's optional, if peer device add "ACAD", timing may error
            //aux_scan_req(12), rf_len is for aux_scan_rsp
            aux_pkt_us = (17 + 12 + rf_len + 11*3)*4 + 300;
                       = rf_len*4 + 544
                    rf_len = 255 : 1020 + 548 = 1568 uS
                    rf_len = 128 :  512 + 548 = 1060 uS

            PASSIVE_SCAN:
                    aux_pkt_us = (17   + 11)*4 = 112 uS  (do not consider "ACAD")
                    aux_pkt_us = (1+64 + 11)*4 = 304 uS  (consider "ACAD", ext_header MAX length is 64)
        }
    }
    else{ //Coded S2/S8
        if(cur_adv_mode == LL_EXTADV_MODE_NON_CONN_NON_SCAN){
            aux_pkt_us = 376 + (rf_len*8 + 43)*S;
                       = rf_len*S*8 + 43*S + 376
       S2 : aux_pkt_us = rf_len*16 + 462
                    rf_len = 255 : 4080 + 462 = 4542 uS
                    rf_len = 128 : 2048 + 462 = 2510 uS

       S8 : aux_pkt_us = rf_len*64 + 720
                    rf_len = 255 : 16320 + 720 = 17040 uS
                    rf_len = 128 :  8192 + 720 =  8912 uS
                    rf_len =  64 :  4096 + 720 =  4816 uS
                    rf_len =  51 :  3264 + 720 =  3984 uS      //20(potential_MAX_length - Sync_info) + 31(legacy ADV data MAX length) = 51

        }
        else if(cur_adv_mode == LL_EXTADV_MODE_CONN){
            //1128 + (303*8 + 129)*8 + 300 = 1128 + 2553*8 + 300 = 21852
            aux_pkt_us = 376*3 + ((rf_len + 34 + 14)*8 + 43*3)*S + 300; //aux_conn_req(34), aux_conn_rsp(14)
                       = rf_len*S*8 + 513*S + 1428
       S2 : aux_pkt_us = rf_len*16 + 2454
                    rf_len = 255 : 4080 + 2454 = 6534 uS
                    rf_len = 128 : 2048 + 2454 = 4502 uS
                    rf_len =  46 :  736 + 2454 = 3190 uS    (15+31=46)

       S8 : aux_pkt_us = rf_len*64 + 4404
                    rf_len = 255 : 16320 + 5532 = 21852 uS
                    rf_len = 128 :  8192 + 5532 = 13724 uS
                    rf_len =  46 :  2944 + 5532 =  8476 uS    (15+31=46)

        }
        else{
          ACTIVE_SCAN:
                aux_pkt_us = 376*3 + ((17 + 12 + rf_len)*8 + 43*3)*S + 300; //17: 2 + AdvA(6) + TargetA(6) + ADI(2) + TX_POWER(1); aux_scan_req(12), aux_scan_rsp(255)
                           = rf_len*S*8 + 361*S + 1428

          PASSIVE_SCAN:
                aux_pkt_us = 376 + (rf_len*8+43)*S;

                aux_pkt_us = 376 + (17*8+43)*S    (do not consider "ACAD") //17: 2 + AdvA(6) + TargetA(6) + ADI(2) + TX_POWER(1);
                           = 376 + 179*S

                aux_pkt_us = 376 + (65*8+43)*S    (consider ACAD, 1+64)
                           = 376 + 563*S

           S2 ACTIVE_SCAN:
                aux_pkt_us = rf_len*16 + 2150
                        rf_len = 255 : 4080 + 2150 = 6230 uS
                        rf_len = 128 : 2048 + 2150 = 4198 uS

           S8 ACTIVE_SCAN:
                aux_pkt_us = rf_len*S*8 + 361*S + 1428
                           = rf_len*8*8 + 361*8 + 1428
                           = rf_len*64 + 4404
                        rf_len = 255 : 16320 + 4316 = 20636 uS
                        rf_len = 128 :  8192 + 4316 = 12508 uS
                        rf_len =  64 :  4096 + 4316 =  8412 uS

           S8 PASSIVE_SCAN:
                aux_pkt_us = 376 + 179*8 = 376 + 1432 = 1808 uS  (do not consider "ACAD")
                aux_pkt_us = 376 + 563*8 = 376 + 4504 = 4880 uS   (consider "ACAD", ext_header MAX length is 64)
        }
    }
 */

//u16 aux_max_us[3][3] = {{2120, 2964, 2812},{1064, 1644, 1568},{17040, 21852, 20636}};
#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
__attribute__((section(".data"))) 
#else
const
#endif
u8  aux_max_rflen[3] = {255, 255, 17};

#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
__attribute__((section(".data"))) 
#else
const
#endif
u16 aux_max_us[3][3] = {{2120, 2120, 216},{1064, 1064, 112},{17040, 17040, 1088}};


#define     ADV_LEN_OFFSET      0
#define     ADV_TYPE_OFFSET     1

_attribute_ram_code_
u8* blt_ext_scan_searchAcadInfos(u8* pAcad, int acad_len, data_type_t advType){

    u8* pAcadInfor = pAcad;

    for(int i = 0; i < acad_len; ){
        if(pAcadInfor[ADV_TYPE_OFFSET] == advType){
            return (u8*)pAcadInfor;
        }

        i += (pAcadInfor[ADV_LEN_OFFSET]+1);
        pAcadInfor = &pAcadInfor[i];
    }
    return NULL;
}





_attribute_ram_code_
bool blt_ll_process_scan_privacy(ll_resolv_list_t **ppRL, u8 advA_type, u8* advA_addr, u8 directAdv, u8 initA_type, u8* initA_addr)
{

    do{
        u8 rpa_resolve_err = 0;

        blt_ll_addr_set_peer_address(0, advA_type, advA_addr);

        u8 advA_is_rpa = IS_RESOLVABLE_PRIVATE_ADDR(advA_type, advA_addr);
        if(advA_is_rpa){ //RPA
            /* none direct ADV with policy 0/2: pass, no need resolve RPA for filter, but need IDA for ADV report event,
             *                                  so resolving work is not wasting time
             *      direct ADV same situation, no need check advA, just check if targetA(initA) addressed  to local,
             *                                 but also need IDA for ADV report event
             */
            *ppRL = blt_ll_resolve_rpa(0, advA_addr, NULL);
            if(*ppRL){  //resolving pass, have a RL entry
                blt_ll_storePeerDeviceRpa(*ppRL, advA_addr);
                blt_ll_addr_set_peer_address(1, (*ppRL)->rlIdAddrType, (*ppRL)->rlIdAddr);
            }
            else{
                rpa_resolve_err = 1;
            }
        }
        else{ //IDA
            /* here "pRL_match" may be used later by scanA(RPA) in scan_req,
             * so must can not be included in "NETWORK_PRIVACY IGNORE_IDA_CHECK" */
            *ppRL = blt_ll_searchResolvingListEntry(advA_type, advA_addr);

            #if (NETWORK_PRIVACY_IGNORE_IDA_CHECK)
                if((*ppRL) && (*ppRL)->peerIrk_valid){ //peer device has distributed its IRK
                    if((*ppRL)->rlPrivMode == NETWORK_PRIVACY_MODE){ //not allowed  /* LL/DDI/SCN/BV-26-C */
                        my_dump_str_data(DBG_PRVC_EXTSCAN_EN, "extscan, network privacy ignore IDA, stop", 0, 0);
                        break; //stop
                    }
                    else{//DEVICE_PRIVACY_MODE, allowed  /* LL/DDI/SCN/BV-28-C */
                        my_dump_str_data(DBG_PRVC_EXTSCAN_EN, "extscan, device privacy accept IDA", 0, 0);
                    }
                }
            #endif
        }


        /* check if advA pass */
        if(bltScn.scan_fp_wl){ //filter needed: direct ADV & none_direct ADV, , check accept list
            /* 1. RPA can not resolve to a IDA, no change to use AL(accept list), fail
             * 2. accept list filter fail */
            if(rpa_resolve_err || !blt_ll_searchAddrInWhiteListTbl(bltAddr.peer_pka_or_ida_type, bltAddr.peer_pka_or_ida_addr)){
                #if (DBG_PRVC_EXTSCAN_EN)
                    if(rpa_resolve_err){
                        my_dump_str_data(DBG_PRVC_EXTSCAN_EN, "extscan, peer advA RPA resolve ERR, stop", bltAddr.peer_pka_or_ida_addr, 6);
                    }
                    else{
                        my_dump_str_data(DBG_PRVC_EXTSCAN_EN, "extscan, peer advA not in AL, stop", bltAddr.peer_pka_or_ida_addr, 6);
                    }
                #endif

                break;
            }
            else{
                my_dump_str_data(DBG_PRVC_EXTSCAN_EN, "extscan, peer advA in AL", bltAddr.peer_pka_or_ida_addr, 6);
            }
        }
        else{ //none direct ADV, filter no need
            //pass
        }



        /* direct ADV "ADV_DIRECT_IND", check if targetA address to local device */
        bltScn.direct_initA_rpa_resolve_fail = 0;
        if(directAdv){
            /* ADV_IND, no change send scan_req, so here timing is not very urgent, can print some log */
            int targetA_is_rpa = IS_RESOLVABLE_PRIVATE_ADDR(initA_type, initA_addr);
            if(targetA_is_rpa){ //RPA
                /* attention, different from ADV: here must use "pRL_match" locate by peer advA !!! */
                if((*ppRL) && blt_ll_resolve_rpa(1, initA_addr, *ppRL)){ //resolve success, pass
                    /* here for "scan fp targetA rpaPass", originally no need resolve to save timing,
                    * but for "LL/DDI/SCN/BV-14-C" "HCI_LE_Direct_Advertising_Report_Event", we should detect resolving error */
                    my_dump_str_data(DBG_PRVC_EXTSCAN_EN, "extscan, direct targetA RPA resolve OK, match", initA_addr, 6);
                }
                else{ //resolve Fail
                    if(bltScn.scan_fp_targetA_rpaPass){
                        //for policy 0x02/0x03, even for RPA resolve fail, still accept
                        //but should mark this, will use "HCI_LE_Direct_Advertising_Report_Event" later "LL/DDI/SCN/BV-14-C"
                        bltScn.direct_initA_rpa_resolve_fail = 1;
                        my_dump_str_data(DBG_PRVC_EXTSCAN_EN, "extscan, targetA RPA resolve ERR for policy 2/3, accept, direct ADV report", initA_addr, 6);
                    }
                    else{
                        my_dump_str_data(DBG_PRVC_EXTSCAN_EN, "extscan, direct targetA RPA resolve ERR, stop", initA_addr, 6);
                        break;  //stop
                    }
                }
            }
            else{ //IDA
                if(smemcmp(initA_addr, bltScn.scan_mac_addr, BLE_ADDR_LEN) || initA_type != bltScn.scan_mac_type){
                    my_dump_str_data(DBG_PRVC_EXTSCAN_EN, "extscan, direct targetA IDA not match, stop", initA_addr, 6);
                    break; //stop
                }
                else{
                    //pass
                    my_dump_str_data(DBG_PRVC_EXTSCAN_EN, "extscan, direct targetA IDA match", initA_addr, 6);
                }
            }
        }




        return TRUE;

    }while(0);


    return FALSE;
}







_attribute_ram_code_ int blt_ext_adv_rx_process(u8 *raw_pkt)//primary channel rx processing.
{
    rf_pkt_ext_adv_t * pExtAdv = (rf_pkt_ext_adv_t *) (raw_pkt + DMA_RFRX_LEN_HW_INFO);


    u8  tx_power = TX_POWER_INFO_NOT_AVAILABLE;
    int extHdr_offset = 0;
    int extadv_auxadv_report = 0, more_adv_pkt_exist = 0, auxptr_info_correct = 0;
    u32 cur_pkt_duration_us = 0, aux_expectTick = 0, auxpkt_offset_us = 0;
    int aux_err_us = 0;
    u16 adi_info = 0;  //= 0 to solve warning "may be used uninitialized in this function"
    aux_ptr_t *p_curAux = NULL;  //= NULL to solve warning "may be used uninitialized in this function"


    int cur_adv_mode = pExtAdv->adv_mode;




    do{
        if(bltScn.initiate_going && cur_adv_mode != LL_EXTADV_MODE_CONN){
            break; //stop
        }
        if(blmsParam.pda_syncing_flg && cur_adv_mode != LL_EXTADV_MODE_NON_CONN_NON_SCAN){
            break; //stop
        }


        /* ACAD & advData can not exist in "ADV_EXT_IND"  &&  CTE info and Sync_Info can not exist in  "ADV_EXT_IND" */
        //if( (pExtAdv->rf_len ==  pExtAdv->ext_hdr_len + 1) && ((pExtAdv->ext_hdr_flg & (EXTHD_BIT_CTE_INFO | EXTHD_BIT_SYNC_INFO)) == 0))
        if(pExtAdv->rf_len != pExtAdv->ext_hdr_len + 1){ //no "Adv Data"
            my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR rf_len, ext_hdr_len", 0, 0);
            break; //stop
        }

        /* 1. advA */
        u8 prichnAdvA_exist = 0, prichnTargetA_exist = 0;  //must give initial 0
        if(pExtAdv->ext_hdr_flg & EXTHD_BIT_ADVA){
            prichnAdvA_exist = 1;
            extHdr_offset += EXTHD_LEN_6_ADVA;
        }
        /* 2. targetA */
        if(pExtAdv->ext_hdr_flg & EXTHD_BIT_TARGETA){
            prichnTargetA_exist = 1;
            extHdr_offset += EXTHD_LEN_6_TARGETA;
        }

        /* 3. CTE info can not exist in "ADV_EXT_IND", jump */
        /* 4. ADI */
        if(pExtAdv->ext_hdr_flg & EXTHD_BIT_ADI){
            adi_info = *(u16 *)(pExtAdv->data + extHdr_offset);  //have confirmed it's 2B aligned, can use "u16 *"
            extHdr_offset += EXTHD_LEN_2_ADI;
        }
        /* 5. Aux Ptr */
        if(pExtAdv->ext_hdr_flg & EXTHD_BIT_AUX_PTR){
            p_curAux = (aux_ptr_t *)(pExtAdv->data + extHdr_offset);

            /* timeStamp should be correct */
            if(p_curAux->chn_index < 37 && p_curAux->aux_phy < PHY_USED_AUXPTR_RFU_BEGIN){

                /* attention: bltPHYs.cur_peer_CI need previous "blt_ll_rx_start_tick_check" in RX IRQ */
                cur_pkt_duration_us = blt_phy_getRfPacketTime_us(pExtAdv->rf_len, bltPHYs.cur_llPhy, bltPHYs.cur_peer_CI);
                auxpkt_offset_us = p_curAux->aux_offset * (p_curAux->offset_unit == EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_300_US ? 300 : 30);


                /* 1000000 approximately equal to 1024*1024 = 2^20
                 * 500 ppm, aux_err_us none zero when auxpkt_offset_us >  2000 uS =  2 mS
                 *  50 ppm, aux_err_us none zero when auxpkt_offset_us > 20000 uS = 20 mS */
                aux_err_us = (p_curAux->ca == EXT_ADV_PDU_AUXPTR_CA_0_50_PPM ? 50 : 500)*auxpkt_offset_us>>20;


                //  |-- ext_packet --|-- at least 300us --|-- aux_packet --|
                //  |---     aux_offset * 30us/300us   ---|
                if(auxpkt_offset_us >= cur_pkt_duration_us + BLE_T_MAFS){
                    auxptr_info_correct = 1;
                    aux_expectTick = bltRxPkt.rx_header_tick + auxpkt_offset_us*SYSTEM_TIMER_TICK_1US;

                    #if(TASK_VERY_CLOSE_DROP_EN)//blt_ext_adv_rx_process
                        u32 auxAdvInd_diffCurTime = (u32)(aux_expectTick - clock_time() - 3*SYSTEM_TIMER_TICK_1US);//48M:750ns
                        if(auxAdvInd_diffCurTime < 250 * SYSTEM_TIMER_TICK_1US){
                            //for primary channel scan,only break and no report extended adv event, no need to process other logical.
                            //return 0 and the following code will process.
                            //auxptr_info_correct = 0;
                            break;
                        }
                    #endif
                }
            }

            if(!auxptr_info_correct){
                my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR aux ptr", 0, 0);
                break;  //stop
            }

            extHdr_offset += EXTHD_LEN_3_AUX_PTR;
        }
        /* 6. Sync info can not exist in "ADV_EXT_IND", jump */
        /* 7. Tx Power */
        if(pExtAdv->ext_hdr_flg & EXTHD_BIT_TX_POWER){
            //TODO: 1M PHY:evt_prop[6]; other PHY:   X
            tx_power = pExtAdv->data[extHdr_offset];
            extHdr_offset += EXTHD_LEN_1_TX_POWER;
        }


        /* no "ACAD", check extended header length and extended header content */
        if(pExtAdv->ext_hdr_len != extHdr_offset + 1){
            my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR ext_hdr_len", 0, 0);
            break;  //stop
        }





        /* Non_Connectable Non_Scannable without auxiliary packet */
        if(cur_adv_mode == LL_EXTADV_MODE_NON_CONN_NON_SCAN){
            /* Non_Connectable Non_Scannable with auxiliary packet */
            if(auxptr_info_correct){
                /* ADI mandatory, CTE Info & Sync Info can not exist, AdvA & TargetA & Tx Power optional */
                if( (pExtAdv->ext_hdr_flg & (EXTHD_BIT_CTE_INFO | EXTHD_BIT_ADI | EXTHD_BIT_SYNC_INFO)) == EXTHD_BIT_ADI){
                    more_adv_pkt_exist = 1;
                }
                else{
                    my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR none pkt with aux flags fail", 0, 0);
                    break;  //stop
                }
            }
            /* Non_Connectable Non_Scannable without auxiliary packet */
            else{
                /* AdvA mandatory, CTE Info & ADI & Sync Info can not exist, TargetA & Tx Power optional */
                if( (pExtAdv->ext_hdr_flg & (EXTHD_BIT_ADVA | EXTHD_BIT_CTE_INFO | EXTHD_BIT_ADI | EXTHD_BIT_SYNC_INFO)) == EXTHD_BIT_ADVA){
                    extadv_auxadv_report = 1;

                    my_dump_str_data(DBG_EXTSCAN_LOGIC, "pri chn, none pkt, without aux data", 0, 0);

                    /* for only Non_Connectable Non_Scannable without auxiliary packet */
                    raw_pkt[1] = 0;   //mark EXT_ADV data length: 0
                    blt_pPrichnScn->tx_power_rpt = tx_power;
                    blt_pPrichnScn->direct_flag = prichnTargetA_exist;
                }
                else{
                    my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR none pkt without aux flags fail", 0, 0);
                    break;  //stop
                }
            }
        }
        /* Connectable & Scannable */
        else if(cur_adv_mode < LL_EXTADV_MODE_RFU){   //(cur_adv_mode == LL_EXTADV_MODE_CONN || cur_adv_mode == LL_EXTADV_MODE_SCAN){
            /* ADI &  Aux Ptr mandatory, AdvA & TargetA & CTE Info & Sync Info can not exist, TargetA & Tx Power optional */
            if( (pExtAdv->ext_hdr_flg & (EXTHD_BIT_ADVA | EXTHD_BIT_TARGETA | EXTHD_BIT_CTE_INFO | EXTHD_BIT_ADI | EXTHD_BIT_AUX_PTR | EXTHD_BIT_SYNC_INFO)) \
                                                                                  ==    (EXTHD_BIT_ADI | EXTHD_BIT_AUX_PTR) ){
                //here "auxptr_info_correct" definitely be 1, timeStamp is correct
                more_adv_pkt_exist = 1;
            }
            else{
                my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR adv mode none 0 flags", 0, 0);
                break;  //stop
            }
        }
        else{ // 0x03
            break;  //stop
        }





        /* logic judge above do first, they do not cost too many time, then RPA & filter process.
         * here TIFS 150uS urgent timing requirement
         * "aux_irq_distance" "task_very_close" may influenced by RPA resolving if list table too big, consider later
         * no need consider ext_init here, because Connectable can not have advA & targetA */

        //extern int blt_ll_process_scan_privacy(u8 advA_type, u8* advA_addr, u8 directAdv, u8 initA_type, u8* initA_addr);
        /* here use summarized rules form table 2.4:
         * if targetA is existed, advA must be existed; if advA is not existed, targetA definitely not existed */
        ll_resolv_list_t *pRL_match = NULL;
        if(prichnAdvA_exist){
            //advA exist, can not be connectable, here only process for scan
            my_dump_str_data(DBG_PRVC_EXTSCAN_EN, "extscan, primary channel", 0, 0);
            if(!blt_ll_process_scan_privacy(&pRL_match, pExtAdv->txAddr, pExtAdv->data, prichnTargetA_exist, pExtAdv->rxAddr, pExtAdv->data + 6)){
                break; //stop
            }

            if(extadv_auxadv_report){ //only change adv type and address for reported ADV
                //for ADV packet: set address type, change RPA to IDA if needed
                blt_pPrichnScn->rpt_addr_type = bltAddr.peer_pka_or_ida_type;
                if(bltAddr.peer_use_rpa){
                    blt_pPrichnScn->rpt_addr_type |= PEERATYPE_IDENTITY_MASK;
                    smemcpy(pExtAdv->data, bltAddr.peer_pka_or_ida_addr, BLE_ADDR_LEN);
                }

                blt_pPrichnScn->directA_rpa_resolve_fail = bltScn.direct_initA_rpa_resolve_fail;
            }
        }






        if(more_adv_pkt_exist )
        {

            //my_dump_str_data(DBG_EXTSCAN_LOGIC, "pri chn ext_adv", (u8 *)(&pExtAdv->rf_len - 1), pExtAdv->rf_len + 2);

            u32 aux_irq_distance = (u32)(aux_expectTick - clock_time());

            //my_dump_str_u32s(DBG_EXTSCAN_LOGIC, "tim", auxpkt_offset_us, bltRxPkt.rx_header_tick, aux_expectTick, aux_irq_distance);


            if(aux_irq_distance > BIT(30) || aux_irq_distance < 100*SYSTEM_TIMER_TICK_1US )
            {
                my_dump_str_data(DBG_EXTSCAN_TIMING, "aux dis err", &aux_irq_distance, 4);
                BLMS_ERR_DEBUG(DBG_EXTSCAN_TIMING, 0xFE050000);
            }
            else
            {

                st_secchn_scn_t *cur_pauxscn = NULL, *available_pauxscn = NULL;
                ext_pkt_info_t extPktInfo;
                extPktInfo.extHdrFlg = pExtAdv->ext_hdr_flg;
                extPktInfo.aux_chnIdx = p_curAux->chn_index;
                extPktInfo.aux_ca = p_curAux->ca;
                extPktInfo.aux_offsetUnit = p_curAux->offset_unit;
                extPktInfo.aux_secPhy = p_curAux->aux_phy + 1;  //attention: "aux_phy_field_t" -> "le_phy_type_t"
                extPktInfo.ext_adiInfo = adi_info;
                extPktInfo.rsvd = 0;

                //compare with existed aux_adv, if same one and aux_timing is same, neglect
                u32 tick_window = extPktInfo.aux_offsetUnit == EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_300_US ? 500*SYSTEM_TIMER_TICK_1US : 50*SYSTEM_TIMER_TICK_1US;
                for(int idx=0; idx<TSKNUM_SECCHN_SCAN; idx++){
                    cur_pauxscn = (st_secchn_scn_t *)&secChnScn_tbl[idx];
                    if(cur_pauxscn->occupied){
                        /* AUX time same as old one */
                        if(TICK_DIFFER_LESS_THAN(aux_expectTick, cur_pauxscn->aux_expect_tick, tick_window))
                        {
                            /* other information match, we consider it's a same AUX indication */
                            if(smemcmp4( (u32 *)&cur_pauxscn->extPkt_info, (u32 *)&extPktInfo, sizeof(ext_pkt_info_t) )){
                                my_dump_str_data(DBG_EXTSCAN_TIMING, "attention: same aux task", 0, 0);

                                return 0; //existed guided EXT_IND, here can return, no ADV report
                            }
                            else{
                                //TODO: /* AUX packet match, but not a same task*/
                            }
                        }
                        my_dump_str_data(DBG_AUXSCAN_LOGIC_QW, "occupied", &idx, 4);
                    }
                    else{
                        if(!available_pauxscn){
                            available_pauxscn = cur_pauxscn; //backup first available AUX SCAN
                        }

                        if(bltExtScn.truncated_scan_msk & BIT(idx)){  //debug
                            BLMS_ERR_DEBUG(DBG_EXTSCAN_LOGIC, 0xFE0C0000 | idx);
                        }
                    }
                }


                if(available_pauxscn != NULL)  //find available AUX SCAN
                {
                    cur_pauxscn = available_pauxscn; //important
                    cur_pauxscn->rfLen_max = aux_max_rflen[cur_adv_mode]; //can guarantee "cur_adv_mode <= 2" in advance


                    u16 aux_pkt_us;
                    u16 aux_pkt_max_us = 0;
                    /* can confirm "p_curAux" assigned correctly and checked "aux_phy <=2" & "cur_adv_mode <= 2" in advance */
                    //aux_pkt_us += aux_max_us[p_curAux->aux_phy][cur_adv_mode];

                    /* for design quick, simplify some process as below, can improve later
                     * 1. 2M process same as 1M now
                     * 2. for Coded PHY, only consider S8
                     * 3. passive scan process same as active scan
                     * 4. for ConnecTable, do not consider if ext_init enable */
                    if(p_curAux->aux_phy == PHY_USED_AUXPTR_LE_CODED){
                        if(cur_adv_mode == LL_EXTADV_MODE_NON_CONN_NON_SCAN){
                            aux_pkt_us = 3984;
                            aux_pkt_max_us = 17040;
                        }
                        else if(cur_adv_mode == LL_EXTADV_MODE_CONN){
                            aux_pkt_us = 8476;
                            aux_pkt_max_us = 21852;
                            if(bltScn.initiate_going){

                            }
                            else{

                            }
                        }
                        else{ //scannable
                            aux_pkt_us = 8412;
                            aux_pkt_max_us = 20636;
                        }

                        cur_pauxscn->scan_duration_flag = DURATION_FLAG_MIN_TIME;
                    }
                    else{
                        if(cur_adv_mode == LL_EXTADV_MODE_NON_CONN_NON_SCAN){
                            aux_pkt_us = 2120;  //biggest rfLen 255
                            //aux_pkt_us = 168; //debug PDU 1 byte timing
                        }
                        else if(cur_adv_mode == LL_EXTADV_MODE_CONN){
                            if(bltScn.initiate_going){
                                aux_pkt_us = 2964;
                            }
                            else{
                                aux_pkt_us = 2120;
                            }
                        }
                        else{
                            aux_pkt_us = 2812;
                        }

                        cur_pauxscn->scan_duration_flag = DURATION_FLAG_MAX_TIME;
                    }

                #if(!TASK_VERY_CLOSE_DROP_EN)
                    int task_very_close = 0;//blt_ext_adv_rx_process

                    //attention: now do not consider local tolerance
                    if(aux_irq_distance < 250 * SYSTEM_TIMER_TICK_1US){ //very close, insert this task

                        task_very_close = 1;
                        cur_pauxscn->tolerance_peer_us = 0;
                        cur_pauxscn->scan_early_set_us = 0;
                        my_dump_str_data(DBG_EXTSCAN_TIMING, "ext adv rx task very close", 0, 0);
                    }
                    else
                #endif
                    {
                        if(aux_irq_distance < 1000 * SYSTEM_TIMER_TICK_1US){
                            cur_pauxscn->tolerance_peer_us = 0;
                        }
                        else{
                            /* long timing, consider accurate tolerance */
                            if(p_curAux->offset_unit == EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_300_US){
                                //max_length 2.45 S, if 500ppm, 1235 uS
                                cur_pauxscn->tolerance_peer_us = 300;
                            }
                            else{
                                //max_length 245 mS, if 500ppm, 123 uS
                                cur_pauxscn->tolerance_peer_us = 30;
                            }

                            cur_pauxscn->tolerance_peer_us += aux_err_us;
                        }
                    }

                    cur_pauxscn->scan_early_set_us = cur_pauxscn->tolerance_peer_us + EXTSCAN_PREPARE_US; //prepare_us value: debug later

                    cur_pauxscn->scan_duration_us = cur_pauxscn->scan_early_set_us + aux_pkt_us + EXTSCAN_TAIL_MARGIN_US;

                    cur_pauxscn->scan_duration_max_us = cur_pauxscn->scan_early_set_us + aux_pkt_max_us + EXTSCAN_TAIL_MARGIN_US;

                    u32 aux_irqTick = aux_expectTick - cur_pauxscn->scan_early_set_us * SYSTEM_TIMER_TICK_1US;
                    u32 task_duration_tick = (cur_pauxscn->scan_duration_us + SLOT_PROCESS_MAX_US)*SYSTEM_TIMER_TICK_1US;

                    if(p_curAux->offset_unit == EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_300_US){
                        cur_pauxscn->aux_pkt_1stRxTm_us = blt_pSecChnScn->scan_early_set_us + bltPHYs.prmb_ac_us + 150 + 300;
                    }else{
                        cur_pauxscn->aux_pkt_1stRxTm_us = blt_pSecChnScn->scan_early_set_us + bltPHYs.prmb_ac_us + 150;
                    }

                    blt_set_auxscan_enable(cur_pauxscn, 1);
                    blt_add_aux_scan_future_task(cur_pauxscn->scnIndex, cur_pauxscn->scnIndex + TSKOFT_SECCHN_SCAN, aux_irqTick, aux_irqTick + task_duration_tick);

                #if (LL_FEATURE_ENABLE_ADVERTISING_CODING_SELECTION)
                    cur_pauxscn->prichn_phy = bltPHYs.cur_peer_CI;
                    cur_pauxscn->secchn_phy = extPktInfo.aux_secPhy;//here should be wrong; not use extPktInfo. later will debug
                #else
                    cur_pauxscn->prichn_phy = bltPHYs.cur_llPhy;
                    cur_pauxscn->secchn_phy = extPktInfo.aux_secPhy;
                #endif
                    cur_pauxscn->scan_advMode = cur_adv_mode;
                    cur_pauxscn->peerAdv_id.sid = (adi_info>>12) & 0xFF;
                    cur_pauxscn->scan_adi = adi_info;
                    cur_pauxscn->next_chnIdx = p_curAux->chn_index;


                    //smemcpy(&cur_pauxscn->extPkt_info, &extPktInfo, sizeof(ext_pkt_info_t));
                    smemcpy4(&cur_pauxscn->extPkt_info, &extPktInfo, sizeof(ext_pkt_info_t));

                    cur_pauxscn->aux_expect_tick = aux_expectTick;
                    cur_pauxscn->aux_irq_tick = aux_irqTick;
                    cur_pauxscn->aux_scan_cnt = 0;  //very important


                    cur_pauxscn->ext_event_type_8bit = cur_adv_mode;  //BIT(0), 1: connectable;  BIT(1), 2: scannable






                    cur_pauxscn->prichn_advA_exist = prichnAdvA_exist;
                    if(prichnAdvA_exist){
                        cur_pauxscn->record_advA_adrType = pExtAdv->txAddr;
                        smemcpy(cur_pauxscn->record_advA_addr, pExtAdv->data, BLE_ADDR_LEN);

                        cur_pauxscn->advA_rpt_adrType = bltAddr.peer_pka_or_ida_type;
                        if(bltAddr.peer_use_rpa){
                            cur_pauxscn->advA_rpt_adrType |= PEERATYPE_IDENTITY_MASK;
                        }
                        smemcpy(cur_pauxscn->advA_rpt_addr, bltAddr.peer_pka_or_ida_addr, BLE_ADDR_LEN);

                        extHdr_offset = EXTHD_LEN_6_ADVA;
                    }
                    else{
                        extHdr_offset = 0;
                    }

                    cur_pauxscn->prichn_targetA_exist = prichnTargetA_exist;
                    if(prichnTargetA_exist){
                        cur_pauxscn->record_direct_adrType = pExtAdv->rxAddr;
                        smemcpy(cur_pauxscn->record_direct_addr, pExtAdv->data + extHdr_offset, BLE_ADDR_LEN);
                        cur_pauxscn->direct_rpa_resolve_fail = bltScn.direct_initA_rpa_resolve_fail;
                    }

                    cur_pauxscn->secchn_advA_exist = 0;
                    cur_pauxscn->secchn_targetA_exist = 0;




                    if(extadv_auxadv_report){ //only change adv type and address for reported ADV
                        //for ADV packet: set address type, change RPA to IDA if needed
                        blt_pPrichnScn->rpt_addr_type = bltAddr.peer_pka_or_ida_type;
                        if(bltAddr.peer_use_rpa){
                            blt_pPrichnScn->rpt_addr_type |= PEERATYPE_IDENTITY_MASK;
                            smemcpy(pExtAdv->data, bltAddr.peer_pka_or_ida_addr, BLE_ADDR_LEN);
                        }
                    }



                #if (!TASK_VERY_CLOSE_DROP_EN)
                    /* check if new AUX task before current primary scan task end time */
                    if(task_very_close){
                        bltSche.immediate_task = 1;
                        bltSche.sche_process_en = 1;
                        blt_ll_prichn_scan_post();
                        BLMS_ERR_DEBUG(DBG_EXTSCAN_TIMING, 0xFE090000);  //debug this situation later
                    }
                    else
                #endif
                    {
                        u32 new_tick =  aux_irqTick - bltScn.scan_post_margin;
                        if(tick1_exceed_tick2(bltScn.scan_post_tick, new_tick)){
                            bltScn.early_stop_tick = clock_time() | 1;
                            if(tick1_exceed_tick2(new_tick, clock_time())){
                                systimer_set_irq_capture(new_tick);
                            }
                            else{
                                bltSche.sche_process_en = 1;
                                blt_ll_prichn_scan_post();
                            }
                        }
                    }

                }
                else{
                    //abandon current EXT_ADV_IND
                }
            }
        }




        return extadv_auxadv_report;


    }while(0);




    return 0;
}





enum{
    EXT_SCNRSP_INVALID,
    EXT_SCNRSP_NOEXIST_CHAIN_PKT,
    EXT_SCNRSP_EXIST_CHAIN_PKT,
    //EXT_ACTIVE_SCAN_LAST_CHAIN_RPT,
};

//the following variable need to check whether to clear.
//whether need to clear other variable. todo
_attribute_ram_code_
void secChnScn_auxScanRsp_procStart(st_secchn_scn_t* secChnScn){
    secChnScn->scan_rx_flag   = 0;
    secChnScn->aux_chain_flag = 0;
    secChnScn->peerAdvA_exist = 0;
    secChnScn->peerTargetA_exist   = 0;
    secChnScn->advrpt_hold_dat_len = 0;
    secChnScn->perdAdv_interval    = 0;
    secChnScn->peerAdv_txPower = 0x7F;
    secChnScn->scan_advMode = LL_EXTADV_MODE_NON_CONN_NON_SCAN; //aux_scan_rsp is NON_CONN NON_SCAN
}

_attribute_ram_code_
void secChnScn_auxScanRsp_procPost(st_secchn_scn_t* secChnScn)
{
    (void)secChnScn; //unused, remove warning
}
/*Scannable, can not include Aux Ptr and sync info.so only one packet,not chain packet.
 *but Aux scan response packet may include Aux Ptr.so Aux scan response packet may include chain packet.
 *Aux scan response packet is non-connectable and non-scannable.
 *so this API need to send "aux scan request" and parse the first "aux scan response"
 *Aux Adv Ind not include AdvData,so not to report it.
 *
 *first, I want to use this method that send scan request, then jump and continue to receive rx,
 *using the irq_scan_rx_secondary_channel API to process aux scan response packet.
 *but later found the second scan start use SRX,so the method is not available.
 *now not jump and wait scan response, then parse it. whether or not there is aux prt.
 */
_attribute_ram_code_ int blt_extendedActiveScan_proc(u8* raw_pkt, u8* new_pkt)//secChnScn_auxScanRsp_procStart
{
    (void)raw_pkt; //unused, remove warning


    /* prepare pkt_scanReq ASAP */
    pkt_scanReq.rxAddr = blt_pSecChnScn->record_advA_adrType;
    smemcpy(pkt_scanReq.advA, blt_pSecChnScn->record_advA_addr, BLE_ADDR_LEN); //copy advA from peer device

    #if (LL_FEATURE_ENABLE_LOCAL_RPA)
        if(bltScn.scan_ownAddr_rpa && blt_pSecChnScn->pRslvlst_extScn && blt_pSecChnScn->pRslvlst_extScn->localIrk_valid){
            pkt_scanReq.txAddr = BLE_ADDR_RANDOM;
            smemcpy(pkt_scanReq.scanA, blt_pSecChnScn->pRslvlst_extScn->rlLocalRpa, BLE_ADDR_LEN);
            blt_ll_resolvSetRpaInUse(blt_pSecChnScn->pRslvlst_extScn); //important: mark
        }
        else{
            pkt_scanReq.txAddr = bltScn.scan_mac_type;
            smemcpy(pkt_scanReq.scanA, bltScn.scan_mac_addr, BLE_ADDR_LEN);
        }
    #endif


    volatile u16 scanReq_timeout_us = 0;
    volatile u16 scanRsp_timeout_us = 0;
    volatile u16 scanRsp_ph_timeout_us = 0;

    if(bltPHYs.cur_llPhy == BLE_PHY_1M || bltPHYs.cur_llPhy == BLE_PHY_2M){
        scanReq_timeout_us = 356;  //(12+10)*8=176; 176 + 150 + 30
        scanRsp_timeout_us = 2200; //(255+10)*8 + margin(80), aux scan response can be max rf length.
        scanRsp_ph_timeout_us = 300; //need to confirm how to get this value.todo--qiuwei

    }else{//coded phy
        scanReq_timeout_us = 1660; //(13+10)*8*8 + 150 + 38
        scanRsp_timeout_us = 17100;//376 + (rfLen+2)*8*S + 24*S + 3*S + 60margin
        scanRsp_ph_timeout_us = 1000;//need to confirm how to get this value. todo--qiuwei
    }



    volatile u32 *ph  = (u32 *) (new_pkt + DMA_RFRX_OFFSET_HEADER);
    ph[0] = 0;  //clear mark
    while ( !HAL_GET_RF_TX_IRQ && (u32)(clock_time() - bltRxPkt.rx_irq_tick) < scanReq_timeout_us * SYSTEM_TIMER_TICK_1US);



    u32 rx_begin_tick = clock_time();
    HAL_CLEAR_RF_TX_IRQ;  //clear

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON); }

    u8 auxptr_info_correct = 0;
    u8 scnRsp_valid_flag = 1;
    u32 aux_expectTick = 0;
    //Scannable, not Aux Ptr and sync info.so only one packet. but Aux scan response packet maybe include Aux Ptr or sync Info.


    while (!(*ph) && (u32)(clock_time() - rx_begin_tick) < scanRsp_ph_timeout_us * SYSTEM_TIMER_TICK_1US); //150 + pkt(22*8) + 150 + margin(50)

    if (*ph)
    {
        rx_begin_tick = clock_time ();
        while (!HAL_GET_RF_RX_IRQ && (clock_time() - rx_begin_tick) < scanRsp_timeout_us * SYSTEM_TIMER_TICK_1US);//2200
        STOP_RF_STATE_MACHINE;
        HAL_CLEAR_RF_RX_IRQ;

        //it is used by the API blt_phy_getRfPacketTime_us to calculate the duration.
        //it is used by the API blt_ll_get_rx_packet_tick to calculate the rx header tick.
        blt_ll_rx_start_tick_check(); //record the scanRsp tick, not use the AUX_ADV_IND.


        if(RF_BLE_PACKET_VALIDITY_CHECK(new_pkt)) //CRC OK
        {
            u8 extHdr_offset = 0;
            rf_pkt_sec_scanrsp_t * pAuxScanRsp = (rf_pkt_sec_scanrsp_t *) (new_pkt + DMA_RFRX_LEN_HW_INFO);
            u16 *rspAdv16 = (u16*)pAuxScanRsp->advA;//AdvA is mandatory for aux scan response, so the first six byte is AdvA
            u16 *reqAdv16 = (u16*)pkt_scanReq.advA;

            /*  Version 5.3 | Vol 6, Part B, 6.2.1 Connectable and scannable undirected event type
            If the advertiser processes the scan request, the advertiser's device address
            (AdvA field) in the SCAN_RSP PDU shall be the same as the advertiser's
            device address (AdvA field) in the SCAN_REQ PDU to which it is responding.
            */
            if(MAC_MATCH16(rspAdv16, reqAdv16)){

                //find a aux_scan table to use for scan response packet
                st_secchn_scn_t *cur_pauxscn = NULL, *available_pauxscn = NULL;
                for(int idx=0; idx<TSKNUM_SECCHN_SCAN; idx++){
                    cur_pauxscn = (st_secchn_scn_t *)&secChnScn_tbl[idx];
                    if(!cur_pauxscn->occupied){
                        if(!available_pauxscn){
                            available_pauxscn = cur_pauxscn;
                            break;
                        }
                    }
                }

                if(!available_pauxscn){
                    scnRsp_valid_flag = 0;
                    my_dump_str_data(0, "not find secTbl", 0, 0);
                    goto SCAN_RSP_PKT_PROC_END;
                }

                //imitate the aux scan start processing
                blt_set_auxscan_enable(cur_pauxscn, 1);
                secChnScn_auxScanRsp_procStart(available_pauxscn);

                //////////////////////parse the aux scan response packet///////////////////////////////
                ///////////////////////////////////////////////////////////////////////////////////////
                /*1. first judge whether the aux scan response packet is valid*/
                if(pAuxScanRsp->ext_hdr_flg & (EXTHD_BIT_TARGETA|EXTHD_BIT_CTE_INFO|EXTHD_BIT_SYNC_INFO)){//BIT(1)/BIT(2)/BIT(5)
                    BLMS_ERR_DEBUG(1, 0xBB110002); //aux scan rsp not include these section.
                }

                /*2. advA. mandatory in aux scan response packet
                 * the structure 'rf_pkt_sec_scanrsp_t' has been changed and the advA must be exist in scan response.
                 * so take advA out as a variable. so extHdr_offset not need to + EXTHD_LEN_6_ADVA.
                 * */
                if( !(pAuxScanRsp->ext_hdr_flg & EXTHD_BIT_ADVA)){
                    BLMS_ERR_DEBUG(1, 0xBB110001);///must include AdvA.
                }

                /* 3. ADI. Option*/
                u16 adi_info = 0;
                if(pAuxScanRsp->ext_hdr_flg & EXTHD_BIT_ADI){
                    adi_info = *(u16 *)(pAuxScanRsp->data + extHdr_offset);  //have confirmed it's 2B aligned, can use "u16 *"

                    if(adi_info != blt_pSecChnScn->scan_adi){ //all secondary channel data should keep same ADI
                        scnRsp_valid_flag = 0;
                        my_dump_str_data(0, "scan adi error", 0, 0);
                    }
                    extHdr_offset += EXTHD_LEN_2_ADI;
                }
                /* 4. Aux Ptr. Option*/
                int aux_err_us = 0;
                aux_ptr_t *p_curAux = NULL;
                if(pAuxScanRsp->ext_hdr_flg & EXTHD_BIT_AUX_PTR){

                    p_curAux = (aux_ptr_t *)(pAuxScanRsp->data + extHdr_offset);

                    if(p_curAux->chn_index < 37 && p_curAux->aux_phy < PHY_USED_AUXPTR_RFU_BEGIN){


                        /* attention: bltPHYs.cur_peer_CI need previous "blt_ll_rx_start_tick_check" in RX IRQ */
                        u32 cur_pkt_duration_us = blt_phy_getRfPacketTime_us(pAuxScanRsp->rf_len, bltPHYs.cur_llPhy, bltPHYs.cur_peer_CI);
                        u32 auxpkt_offset_us = p_curAux->aux_offset * (p_curAux->offset_unit == EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_300_US ? 300 : 30);

                        aux_err_us = (p_curAux->ca == EXT_ADV_PDU_AUXPTR_CA_0_50_PPM ? 50 : 500)*auxpkt_offset_us>>20; ///now not consider
                        aux_expectTick = bltRxPkt.rx_header_tick + auxpkt_offset_us*SYSTEM_TIMER_TICK_1US;

                        #if (TASK_VERY_CLOSE_DROP_EN)
                            u32 auxScnRspChain_diffCurTime = (u32)(aux_expectTick - clock_time() - 2*SYSTEM_TIMER_TICK_1US);
                            if(auxScnRspChain_diffCurTime < 250*SYSTEM_TIMER_TICK_1US){//only report ext_adv_ind, not report aux_scan_rsp and it's chain pkt.
                                secChnScn_auxScanRsp_procStart(available_pauxscn); //clear the related variable.
                                blt_set_auxscan_enable(cur_pauxscn, 0);
                                goto SCAN_RSP_PKT_PROC_END;
                            }
                        #endif

                        //  |-- aux_packet --|-- at least 300us --|-- aux_packet --|
                        //  |---     aux_offset * 30us/300us   ---|
                        if(auxpkt_offset_us >= cur_pkt_duration_us + BLE_T_MAFS){
                            auxptr_info_correct = 1;
                        }
                        else{
                            scnRsp_valid_flag = 0;
                        }

                        // all secondary channel data should keep same PHY
                        if(p_curAux->aux_phy != (blt_pSecChnScn->secchn_phy - 1)){
                            scnRsp_valid_flag = 0;
                        }
                    }
                    else{
                        scnRsp_valid_flag = 0;
                    }
                    extHdr_offset += EXTHD_LEN_3_AUX_PTR;
                }

                /* 7. Tx Power */
                if(pAuxScanRsp->ext_hdr_flg & EXTHD_BIT_TX_POWER){
                    available_pauxscn->peerAdv_txPower = pAuxScanRsp->data[extHdr_offset];
                    extHdr_offset += EXTHD_LEN_1_TX_POWER;
                }

                available_pauxscn->peerAdv_datOffset = pAuxScanRsp->ext_hdr_len + 1;
                ////////////////ending of parse ////////////////////////////////////////////////////
                ////////////////////////////////////////////////////////////////////////////////////

                if(!scnRsp_valid_flag){
                    secChnScn_auxScanRsp_procStart(available_pauxscn); //clear the related variable.
                    blt_set_auxscan_enable(cur_pauxscn, 0); //need to release the relevant secChnScn_tbl
                    goto SCAN_RSP_PKT_PROC_END;
                }

                //////////////
                available_pauxscn->scan_rx_flag = SCANRX_FLAG_FIRST_DATA;
                if(!auxptr_info_correct){ //if not
                    available_pauxscn->scan_rx_flag |= SCANRX_FLAG_LAST_DATA;
                }

                available_pauxscn->ext_event_type_8bit = EXTADV_RPT_EVT_MASK_SCANNABLE | EXTADV_RPT_EVT_MASK_SCAN_RESPONSE;//pAuxScanRsp->adv_mode;
                new_pkt[1] = pAuxScanRsp->rf_len - pAuxScanRsp->ext_hdr_len - 1;//mark scan response data length, 6 = 6(advAddress)
                new_pkt[2] = (SECCHN_IDX_MARK | available_pauxscn->scnIndex);   //blt_pSecChnScn->scnIndex
                new_pkt[3] = available_pauxscn->scan_rx_flag;

                /////////////////here scnRsp_valid_flag = 1////////////////////
                available_pauxscn->prichn_phy     = blt_pSecChnScn->prichn_phy; //be same as the aux_adv_ind pkt
                available_pauxscn->secchn_phy     = blt_pSecChnScn->secchn_phy; //all secondary channel data should keep same PHY
                available_pauxscn->scan_adi       = blt_pSecChnScn->scan_adi;   //all secondary channel data should keep same ADI
                available_pauxscn->peerAdv_id.sid = blt_pSecChnScn->peerAdv_id.sid;

                //////////
                available_pauxscn->aux_scan_cnt = 1;//note

                ///set the next rx buffer
                scan_secRxFifo.wptr ++;
                u8* new_pkt2 = (u8 *)(scan_secRxFifo.p + (scan_secRxFifo.wptr & SCAN_SECCHN_RXFIFO_MASK) * SCAN_SECCHN_RXFIFO_SIZE); //set next buffer
                bltExtScn.scan_rx_sec_chn_dma_buff = (u32)new_pkt2;
                ble_rf_set_rx_dma((u8*)bltExtScn.scan_rx_sec_chn_dma_buff, bltExtScn.scan_rx_sec_chn_dma_size);



                //here all logic judge is done, current AUX_SCAN_RSP packet is valid now

                available_pauxscn->advA_rpt_adrType = blt_pSecChnScn->advA_rpt_adrType;
                smemcpy(available_pauxscn->advA_rpt_addr, blt_pSecChnScn->advA_rpt_addr, BLE_ADDR_LEN);
                available_pauxscn->total_targetA_exist = blt_pSecChnScn->total_targetA_exist; //LL/DDI/SCN/BV-63-C
                if(available_pauxscn->total_targetA_exist){
                    available_pauxscn->record_direct_adrType = blt_pSecChnScn->record_direct_adrType;
                    smemcpy(available_pauxscn->record_direct_addr, blt_pSecChnScn->record_direct_addr, BLE_ADDR_LEN);
                }



                //////////////////////////////////////////////////////////////////////
                //if the scnRsp has chain pkt, here will set the chain pkt's start tick or other variable.
                if(auxptr_info_correct){

                    u32 aux_irq_distance = (u32)(aux_expectTick - clock_time());

                    if(aux_irq_distance > BIT(30) || aux_irq_distance < 100*SYSTEM_TIMER_TICK_1US )
                    {
                        my_dump_str_data(0, "aux dis err", &aux_irq_distance, 4);
                        BLMS_ERR_DEBUG(DBG_EXTSCAN_TIMING, 0xFE050000);
                    }
                    else
                    {
                        cur_pauxscn = available_pauxscn;

                        u16 aux_pkt_us;
                        u16 aux_pkt_max_us = 0;

                        if(p_curAux->aux_phy == PHY_USED_AUXPTR_LE_CODED){
                            aux_pkt_us = 3984;
                            aux_pkt_max_us = 17040;
                            cur_pauxscn->scan_duration_flag = DURATION_FLAG_MIN_TIME;
                        }
                        else{
                            aux_pkt_us = 2120;  //biggest rfLen 255
                            cur_pauxscn->scan_duration_flag = DURATION_FLAG_MAX_TIME;
                        }

                    #if (!TASK_VERY_CLOSE_DROP_EN)
                        int task_very_close = 0;//blt_extendedActiveScan_proc
                        //attention: now do not consider local tolerance
                        if(aux_irq_distance < 250 * SYSTEM_TIMER_TICK_1US){ //very close, insert this task

                            task_very_close = 1;
                            cur_pauxscn->tolerance_peer_us = 0;
                            cur_pauxscn->scan_early_set_us = 0;

                            my_dump_str_data(0, "ext active scan task very close", 0, 0);
                        }
                        else
                    #endif
                        {
                            if(aux_irq_distance < 1000 * SYSTEM_TIMER_TICK_1US){
                                cur_pauxscn->tolerance_peer_us = 0;
                            }
                            else{
                                /* long timing, consider accurate tolerance */
                                if(p_curAux->offset_unit == EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_300_US){
                                    //max_length 2.45 S, if 500ppm, 1235 uS
                                    cur_pauxscn->tolerance_peer_us = 300;
                                }
                                else{
                                    //max_length 245 mS, if 500ppm, 123 uS
                                    cur_pauxscn->tolerance_peer_us = 30;
                                }

                                cur_pauxscn->tolerance_peer_us += aux_err_us;
                            }
                        }

                        cur_pauxscn->scan_early_set_us = cur_pauxscn->tolerance_peer_us + 57 + EXTSCAN_PREPARE_US; //prepare_us value: debug later
                        cur_pauxscn->scan_duration_us = cur_pauxscn->scan_early_set_us + aux_pkt_us + EXTSCAN_TAIL_MARGIN_US;
                        cur_pauxscn->scan_duration_max_us = cur_pauxscn->scan_early_set_us + aux_pkt_max_us + EXTSCAN_TAIL_MARGIN_US;

                        u32 aux_irqTick = aux_expectTick - cur_pauxscn->scan_early_set_us * SYSTEM_TIMER_TICK_1US;
                        u32 task_duration_tick = (cur_pauxscn->scan_duration_us + SLOT_PROCESS_MAX_US)*SYSTEM_TIMER_TICK_1US;

                        blt_add_aux_scan_future_task(cur_pauxscn->scnIndex, cur_pauxscn->scnIndex + TSKOFT_SECCHN_SCAN, aux_irqTick, aux_irqTick + task_duration_tick);
                        cur_pauxscn->next_chnIdx = p_curAux->chn_index;
                        cur_pauxscn->aux_expect_tick = aux_expectTick;
                        cur_pauxscn->aux_irq_tick = aux_irqTick;

                        cur_pauxscn->aux_scnRsp_chain_flag = 1;

                    #if (!TASK_VERY_CLOSE_DROP_EN)
                        /* check if new AUX task before current primary scan task end time */
                        if(task_very_close){
                            bltSche.immediate_task = 1;
                            BLMS_ERR_DEBUG(DBG_EXTSCAN_TIMING, 0xFE090000);  //debug this situation later
                            my_dump_str_data(0,"scnRsp very close", 0, 0);
                        }
                    #endif
                    }
                }//ending of if(auxptr_info_correct)

                //blt_add_aux_scan_future_task(cur_pauxscn->scnIndex, cur_pauxscn->scnIndex + TSKOFT_SECCHN_SCAN, aux_irqTick, aux_irqTick + task_duration_tick);
                secChnScn_auxScanRsp_procPost(available_pauxscn);
            } //ending of if(MAC_MATCH16(rspAdv16, reqAdv16))
            else{
                scnRsp_valid_flag = 0;
            }
        }else{//if(RF_BLE_PACKET_VALIDITY_CHECK(new_pkt))
            scnRsp_valid_flag = 0;
        }
    }
    else{//if (*ph):not receive rsp pkt

        scnRsp_valid_flag = 0;
    }

    //////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////
    SCAN_RSP_PKT_PROC_END:



    STOP_RF_STATE_MACHINE; //prevent aux scan request packet from sending.
    CLEAR_ALL_RFIRQ_STATUS;
    blmsParam.delay_clear_rf_status = 1;

    if(scnRsp_valid_flag){
        if(auxptr_info_correct){
            return EXT_SCNRSP_EXIST_CHAIN_PKT; //indicate there is aux chain packet.
        }

        return EXT_SCNRSP_NOEXIST_CHAIN_PKT;
    }

    return EXT_SCNRSP_INVALID;
}


/*                                      pdaSync_flag    aux_chain_flag      aux_scnRsp_chain_flag       scan_advMode
*
* 1. AUX_ADV_IND                             0               0                      0                     0/1/2
* 2. AUX_CHAIN_IND of AUX_ADV_IND            0               1                      0                     0/1/2
*
* 3. AUX_SYNC_IND                            1               0                      0                       0
* 4. AUX_CHAIN_IND of AUX_SYNC_IND           1               1                      0                       0
*
* 5. AUX_CHAIN_IND of AUX_SCAN_RSP           0               1                      1                not use this variable
*
*/
_attribute_ram_code_ int irq_scan_rx_secondary_channel(void)
{
    /* RX buffer overflow processed in aux_scan start, different from acl_conn process method */
#if 1 //optimize, to save RamCode
    u8* raw_pkt = ble_curr_rx_dma_buff;
    scan_secRxFifo.wptr ++;
#else
    u8 * raw_pkt = (u8 *)(scan_secRxFifo.p + (scan_secRxFifo.wptr++ & SCAN_SECCHN_RXFIFO_MASK) * SCAN_SECCHN_RXFIFO_SIZE);
#endif
    u8 * new_pkt = (u8 *)(scan_secRxFifo.p + (scan_secRxFifo.wptr   & SCAN_SECCHN_RXFIFO_MASK) * SCAN_SECCHN_RXFIFO_SIZE);
    HAL_CLEAR_RF_RX_IRQ;


    #if (BQB_TEST_EN && SIHUI_FILTER_RSSI)  //for debug, remove other device ADV packet
        u8 rssi = raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)];
        if(rssi < 50){  // -60
            scan_secRxFifo.wptr --;
            return 0;
        }
        else{
            //my_dump_str_data(DBG_PRVC_LEGSCAN_EN, "rssi", &rssi, 1);
        }
    #endif


    bltExtScn.scan_rx_sec_chn_dma_buff = (u32)new_pkt;
    ble_rf_set_rx_dma((u8*)bltExtScn.scan_rx_sec_chn_dma_buff, bltExtScn.scan_rx_sec_chn_dma_size);


    u8 next_buffer = 0;
    u8 initiate_start = 0;
    u8 scanreq_start = 0;

    /* "rx header tick" none zero is under condition CRC correct, so here do not check CRC by
       "RF_BLE_PACKET_VALIDITY_CHECK" to save RamCode and running timing
       Or we can use "bltRxPkt.crc correct" */
    if(bltRxPkt.rx_header_tick)
    {
        //DBG_QIUWEI_CHN4_TOGGLE;

        #if (SLEV_second_rx_adv)
        log_event_irq(SL_STACK_EXTSCAN_BASIC_TIMING_EN, SLEV_second_rx_adv);
        #endif

        rf_pkt_ext_adv_t * pExtAdv = (rf_pkt_ext_adv_t *) (raw_pkt + DMA_RFRX_LEN_HW_INFO);

    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
        if( (blt_pSecChnScn->pdaSync_flag & PAWR_PACKET_FLAG) && pExtAdv->type == LL_TYPE_CONNECT_REQ )//conn_req  //pExtAdv->rf_len == 34
        {
            #if (PAWR_SYNC_TIMING_ADJUST_EN)
                u8 syncHandle = bltPdaSync.pdA_sync_sel;
                if(!pawr_sync_timingAdjust[syncHandle].rx_1st_tick){
                    pawr_sync_timingAdjust[syncHandle].rx_1st_tick = bltRxPkt.rx_header_tick;
                }
            #endif


            if(ll_pawr_sync_sub_irq_task_cb){ //blt_ll_PAwRsync_auxConnInd_proc

                auxScnCmnParam.rx_received = 1;

                u8 rtnSta = ll_pawr_sync_sub_irq_task_cb(FLAG_PAWR_SYNC_RX_AUX_CONN_REQ, raw_pkt, NULL);

                if ( rtnSta == PAWR_CONN_RTN_ADDR_NO_MATCH || rtnSta == PAWR_CONN_RTN_FAIL){//not right address

                    scan_secRxFifo.wptr--;
                    bltExtScn.scan_rx_sec_chn_dma_buff = (u32)raw_pkt;
                    ble_rf_set_rx_dma((u8*)bltExtScn.scan_rx_sec_chn_dma_buff, bltExtScn.scan_rx_sec_chn_dma_size);
                }

                return 0;
            }
        }
    #endif

        /* initiate_going & pda_syncing_flg can not happen at same time,
         * control this at create_connection & create_sync API */
        if(bltScn.initiate_going){
            if(pExtAdv->adv_mode == LL_EXTADV_MODE_CONN){
                if(!blmsParam.new_conn_forbidden ){
                    blt_quick_tx_prepare(FSM_TX2RX, &pkt_init, pExtAdv->rf_len);
                    initiate_start = 1;

                    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_TX_ON); }

                    /* RX buffer DMA set */
                    ble_rf_set_rx_dma((u8*)glb_temp_rx_buff, 4);//64/16=4
                    rf_set_rx_maxlen(14); //aux_conn_rsp 14 Byte
                    rf_ble_set_rx_timeout(bltPHYs.prmb_ac_us + 150 + 20);  //leave 20 uS margin
                }
            }
            else{
                my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR init going only concern connectable", 0, 0);
                //TODO later, break; // stop
            }
        }
        else if(bltScn.scan_type == SCAN_TYPE_ACTIVE && pExtAdv->adv_mode == LL_EXTADV_MODE_SCAN){

            /* rf_ble_tx_on (); */ //commented out, because SCN 2nd channel dosen't use RX continue mode.

            /* prepare TX2RX FSM quickly due to 150uS urgent timing */
            blt_quick_tx_prepare(FSM_TX2RX, (void *)&pkt_scanReq, pExtAdv->rf_len);
            rf_ble_set_rx_timeout(bltPHYs.prmb_ac_us + 150 + 20);  //leave 20 uS margin

            scanreq_start = 1;
            if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_TX_ON);  }
        }



        raw_pkt[3] = 0;
        raw_pkt[2] = 0;
        raw_pkt[1] = 0;   //mark EXT_ADV data length

        my_dump_str_data(DBG_AUXSCAN_LOGIC_QW, "sec extAdv", 0, 0);

        u8 extadv_auxadv_report = 0, more_adv_pkt_exist = 0;
        int extHdr_offset = 0, targetA_offset = 0;
        u32 cur_pkt_duration_us = 0, aux_expectTick = 0, auxptr_info_correct = 0, auxpkt_offset_us = 0;
        int aux_err_us = 0;
        int cur_advdat_len = 0;
        u16 adi_info = 0;  //= 0 to solve warning "may be used uninitialized in this function"
        aux_ptr_t *p_curAux = NULL;  //= NULL to solve warning "may be used uninitialized in this function"
        sync_info_t *p_syncInfo = NULL;



        do{
            if(pExtAdv->type != LL_TYPE_ADV_EXT_IND){
                break; // stop
            }

            if(pExtAdv->ext_hdr_len == 0 && !blt_pSecChnScn->pdaSync_flag){
                break; //only aux_sync_ind and its chain packet's extended header length may be zero.
            }

            u8 aux_adv_1st_pkt = !blt_pSecChnScn->aux_chain_flag && !blt_pSecChnScn->pdaSync_flag;

            /* for AUX_CHAIN_IND, adv_mode = 0, for AUX_ADV_IND, it should keep same with ADV_EXT_IND */
            if(pExtAdv->adv_mode == (blt_pSecChnScn->aux_chain_flag ? LL_EXTADV_MODE_NON_CONN_NON_SCAN : blt_pSecChnScn->scan_advMode)){

            }
            else{
                my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR adv mode err", 0, 0);
                break; // stop
            }
            /* PRD_ADV Special process 1: for prd_adv sync, ADV_mode is always, we set "scan_advMode" to 0 when pda_sync, can make sure this code logic correct */



            /* 20: sSlot align may introduce 20uS error, 30uS is a safe margin*/
            u32 diff_margin = (blt_pSecChnScn->tolerance_peer_us + 30 + 220)*SYSTEM_TIMER_TICK_1US;
            if(TICK_DIFFER_LESS_THAN(blt_pSecChnScn->aux_expect_tick, bltRxPkt.rx_header_tick, diff_margin)){

            }
            else{
                /* aux_pkt RX timing not match with previous calculation */
                if(blt_pSecChnScn->pdaSync_flag){
                    //DBG_C HN8_TOGGLE;
                    //my_dump_str_u32s(DBG_PDA_SYNC_TIMING, "pda RX time no match", blt_pSecChnScn->aux_expect_tick, bltRxPkt.rx_header_tick, blt_pSecChnScn->tolerance_peer_us, blt_pPdAsync->sync_early_set_us);
                    my_dump_str_data(DBG_PDA_SYNC_TIMING, "data", pExtAdv->data, 16);
                }
                else{
                    my_dump_str_u32s(DBG_EXTSCAN_TIMING || DBG_EXTSCAN_ERR_PKT_EN, "aux RX time no match", blt_pSecChnScn->aux_expect_tick, bltRxPkt.rx_header_tick, 0, 0);
                }

                break; // stop
            }



            //////start ext_hdr_len judge///////
            if(pExtAdv->ext_hdr_len != 0){
                /* 1. advA: C1/C4, can not exist on both ADV_EXT_IND and AUX_ADV_IND */
                if(pExtAdv->ext_hdr_flg & EXTHD_BIT_ADVA){
                    if(!aux_adv_1st_pkt || blt_pSecChnScn->prichn_advA_exist){
                        #if (DBG_EXTSCAN_ERR_PKT_EN)
                            if(aux_adv_1st_pkt){
                                my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR, advA both in pri & sen chn !!!", 0, 0);
                            }
                            else{
                                my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR, can not have advA !!!", 0, 0);
                            }
                        #endif

                        break; // stop
                    }

                    if(aux_adv_1st_pkt){
                        blt_pSecChnScn->secchn_advA_exist = 1;
                    }
                    targetA_offset = EXTHD_LEN_6_ADVA;
                    extHdr_offset += EXTHD_LEN_6_ADVA;
                }

                /* 2. targetA:  C1/C2, at lease one exist, error if no one exist */
                if(pExtAdv->ext_hdr_flg & EXTHD_BIT_TARGETA){
                    if(!aux_adv_1st_pkt || blt_pSecChnScn->prichn_targetA_exist){
                        #if (DBG_EXTSCAN_ERR_PKT_EN)
                            if(aux_adv_1st_pkt){
                                my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR, targetA both in pri & sen chn !!!", 0, 0);
                            }
                            else{
                                my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR, can not have targetA !!!", 0, 0);
                            }
                        #endif

                        break; // stop
                    }

                    if(aux_adv_1st_pkt){
                        blt_pSecChnScn->secchn_targetA_exist = 1;
                    }
                    extHdr_offset += EXTHD_LEN_6_TARGETA;
                }


                /* 3. CTE info can exist in "AUX_SYNC_IND" and it's "AUX_CHAIN_IND" */
                #if (LL_FEATURE_ENABLE_LE_AOA_AOD)
                    if(pExtAdv->ext_hdr_flg & EXTHD_BIT_CTE_INFO){
                        u8 cte_info = *(u8*)(pExtAdv->data + extHdr_offset);
                        u8 cte_time = cte_info & 0x1F;
                        u8 cte_type = (cte_info & 0xC0) >> 6;

                        if((cte_time < CTE_TIME_MIN) || (cte_time > CTE_TIME_MAX) || (cte_type > AOD_TYPE_2US)){
                            break; // stop
                        }

                        if(blms_state == BLMS_STATE_PDA_SYNC_S){
                            if(((cte_type == AOA_TYPE) && (blt_pPdAsync->create_sync_cte_type & NOT_SYNC_AOA)) || \
                                ((cte_type == AOD_TYPE_1US) && (blt_pPdAsync->create_sync_cte_type & NOT_SYNC_AOD_1US)) || \
                                ((cte_type == AOD_TYPE_2US) && (blt_pPdAsync->create_sync_cte_type & NOT_SYNC_AOD_2US))){
                                blt_pPdAsync->sync_wrong_cte_type = HCI_ERR_UNSUPPORTED_REMOTE_FEATURE;
                                break; // stop
                            }
                            else{
                                blt_pPdAsync->sync_wrong_cte_type = 0;
                            }

                            blt_pPdAsync->sync_cte_type = cte_type;
                            blt_pPdAsync->sync_cte_time = cte_time;
                            blt_pPdAsync->sync_cte_EvtCnt = blt_pPda->paEvtCnt;
                            if(cte_connLess_switchPattern[bltPdaSync.pdA_sync_sel].cte_rx_mode_en){
                                blt_pPdAsync->sync_cte_IQ_valid = 1;
                            }
                            else{
                                blt_pPdAsync->sync_cte_IQ_valid = 0;
                            }
                            DBG_CHN9_TOGGLE;
                        }

                        my_dump_str_data(DBG_AOA_AOD_LOGIC, "irq RX CTE info", &cte_info, 1);

                        extHdr_offset += EXTHD_LEN_1_CTE;
                    }
                    else{
                        if(blms_state == BLMS_STATE_PDA_SYNC_S){
                            DBG_CHN9_TOGGLE;
                            blt_pPdAsync->sync_cte_type = 0xFF;
                            blt_pPdAsync->sync_cte_IQ_valid = 0;
                            DBG_CHN9_TOGGLE;
                        }
                    }
                #else
                    if(pExtAdv->ext_hdr_flg & EXTHD_BIT_CTE_INFO){
                        extHdr_offset += EXTHD_LEN_1_CTE;
                    }
                #endif

                /* 4. ADI */
                if(pExtAdv->ext_hdr_flg & EXTHD_BIT_ADI){
                    adi_info = *(u16 *)(pExtAdv->data + extHdr_offset);  //have confirmed it's 2B aligned, can use "u16 *"

                    //only aux pkt and pda chain pkt to check. other situation not check. V5.3 PDA can also include ADI.
                    if( !blt_pSecChnScn->pdaSync_flag || blt_pSecChnScn->aux_chain_flag){
                        if(adi_info != blt_pSecChnScn->scan_adi){ //all secondary channel data should keep same ADI
                            my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR adi conflict", 0, 0);
                            break; // stop
                        }
                    }

                    extHdr_offset += EXTHD_LEN_2_ADI;
                }
                /* 5. Aux Ptr */
                if(pExtAdv->ext_hdr_flg & EXTHD_BIT_AUX_PTR){

                    if(blt_pSecChnScn->scan_advMode){
                        break; // stop
                    }

                    p_curAux = (aux_ptr_t *)(pExtAdv->data + extHdr_offset);

                    if(p_curAux->aux_phy != (blt_pSecChnScn->secchn_phy - 1)){  // all secondary channel data should keep same PHY
                        break; // stop
                    }

                    if(p_curAux->chn_index < 37 && p_curAux->aux_phy < PHY_USED_AUXPTR_RFU_BEGIN){

                        u8 codedPhy_flag = raw_pkt[DMA_RFRX_OFFSET_STATUS(raw_pkt)]&0x10;//bit4 indicate long range. 0:S2;;;1:S8

                        cur_pkt_duration_us = blt_phy_getRfPacketTime_us(pExtAdv->rf_len, bltPHYs.cur_llPhy, codedPhy_flag?LE_CODED_S8:LE_CODED_S2);
                        auxpkt_offset_us = p_curAux->aux_offset * (p_curAux->offset_unit == 1 ? 300 : 30);

                        aux_err_us = (p_curAux->ca == EXT_ADV_PDU_AUXPTR_CA_0_50_PPM ? 50 : 500)*auxpkt_offset_us>>20;
                        aux_expectTick = bltRxPkt.rx_header_tick + auxpkt_offset_us*SYSTEM_TIMER_TICK_1US;

                        #if (TASK_VERY_CLOSE_DROP_EN)//irq_scan_rx_secondary_channel
                            //connectable/scannable pkt must not include auxPtr section. so not affect that processing.
                            //but if do this intentionally......
                            u32 auxChain_diffCurTime = (u32)(aux_expectTick - clock_time() - 4*SYSTEM_TIMER_TICK_1US);
                            if(auxChain_diffCurTime < 250*SYSTEM_TIMER_TICK_1US){
                                auxScnCmnParam.rx_received = 0; //xx_post will process the relevant logical. such as release resource/process truncated etc.
                                break;
                            }
                        #endif

                        //  |-- aux_packet --|-- at least 300us --|-- aux_packet --|
                        //  |---     aux_offset * 30us/300us   ---|
                        if(auxpkt_offset_us >= cur_pkt_duration_us + BLE_T_MAFS){
                            auxptr_info_correct = 1;
                        }
                    }


                    if(!auxptr_info_correct){
                        my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR, aux ptr error!!!", 0, 0);
                        break;  //stop
                    }


                    extHdr_offset += EXTHD_LEN_3_AUX_PTR;
                }

                /* 6. Sync info */
                if(pExtAdv->ext_hdr_flg & EXTHD_BIT_SYNC_INFO){
                    p_syncInfo = (sync_info_t *)(pExtAdv->data + extHdr_offset);
                    extHdr_offset += EXTHD_LEN_18_SYNC_INFO;
                }
                /* 7. Tx Power */
                if(pExtAdv->ext_hdr_flg & EXTHD_BIT_TX_POWER){
                    //TODO: 1M PHY:evt_prop[6]; other PHY:   X
                    blt_pSecChnScn->peerAdv_txPower = pExtAdv->data[extHdr_offset];
                    extHdr_offset += EXTHD_LEN_1_TX_POWER;
                }
                else{
                    blt_pSecChnScn->peerAdv_txPower = TX_POWER_INFO_NOT_AVAILABLE;
                }
            }
            //////ending of ext_hdr_len judge/////


            int cur_acad_len = pExtAdv->ext_hdr_len==0 ? 0 : (pExtAdv->ext_hdr_len - extHdr_offset - 1);//-1 indicate extended header flag(1B)
            cur_advdat_len = pExtAdv->rf_len - pExtAdv->ext_hdr_len - 1;//-1 indicate extended header length(1B)
            blt_pSecChnScn->peerAdv_datOffset = pExtAdv->ext_hdr_len + 1;//base address is rf_len of the header.


            if(cur_acad_len < 0 || cur_advdat_len < 0){ //ACAD optional for all AUX_ADV_IND
                my_dump_str_data(DBG_EXTSCAN_ERR_PKT_EN, "ERROR, acad len or advdata len error!!!", 0, 0);
                break; // stop
            }





            if(aux_adv_1st_pkt && !bltScn.initiate_going){ //only process for scan
                blt_pSecChnScn->total_advA_exist = blt_pSecChnScn->prichn_advA_exist || blt_pSecChnScn->secchn_advA_exist;
                blt_pSecChnScn->total_targetA_exist = blt_pSecChnScn->prichn_targetA_exist || blt_pSecChnScn->secchn_targetA_exist;

                if(blt_pSecChnScn->secchn_advA_exist){
                    ll_resolv_list_t *pRL_match = NULL;
                    my_dump_str_data(DBG_PRVC_EXTSCAN_EN, "extscan, secondary channel", 0, 0);
                    if(!blt_ll_process_scan_privacy(&pRL_match, pExtAdv->txAddr, pExtAdv->data, blt_pSecChnScn->total_targetA_exist, pExtAdv->rxAddr, pExtAdv->data + targetA_offset)){
                        break; //stop
                    }

                    blt_pSecChnScn->pRslvlst_extScn = pRL_match; //may be NULL

                    blt_pSecChnScn->record_advA_adrType = pExtAdv->txAddr;
                    smemcpy(blt_pSecChnScn->record_advA_addr, pExtAdv->data, BLE_ADDR_LEN);


                    blt_pSecChnScn->advA_rpt_adrType = bltAddr.peer_pka_or_ida_type;
                    if(bltAddr.peer_use_rpa){
                        blt_pSecChnScn->advA_rpt_adrType |= PEERATYPE_IDENTITY_MASK;
                    }
                    smemcpy(blt_pSecChnScn->advA_rpt_addr, bltAddr.peer_pka_or_ida_addr, BLE_ADDR_LEN);




                    blt_pSecChnScn->direct_rpa_resolve_fail = bltScn.direct_initA_rpa_resolve_fail;
                }


                if(blt_pSecChnScn->total_advA_exist){
                    /* Core_5.3, LE Periodic Advertising Create Sync command
                     * Advertiser_Address_Type: 0x00: Public Device Address or Public Identity Address
                     *                          0x01: Random Device Address or Random (static) Identity Address
                     * Advertiser_Address:  Public Device Address, Random Device Address, Public Identity
                                            Address, or Random (static) Identity Address of the advertiser  */
                    blt_pSecChnScn->peerAdv_id.adrType = (blt_pSecChnScn->advA_rpt_adrType & ~PEERATYPE_IDENTITY_MASK);
                    smemcpy(blt_pSecChnScn->peerAdv_id.addr, blt_pSecChnScn->advA_rpt_addr, BLE_ADDR_LEN);
                }
                else{ //anonymous advertisement
                    //todo: anonymous advertisement with targetA not processed now, accept
                    blt_pSecChnScn->advA_rpt_adrType = 0xFF;
                }


                if(blt_pSecChnScn->secchn_targetA_exist){
                    blt_pSecChnScn->record_direct_adrType = pExtAdv->rxAddr;
                    smemcpy(blt_pSecChnScn->record_direct_addr, pExtAdv->data + targetA_offset, BLE_ADDR_LEN);
                }
            }






            //u8 *pCurAcad = pExtAdv->data + extHdr_offset;

            if(blt_pSecChnScn->scan_advMode == LL_EXTADV_MODE_CONN)   //Connectable
            {
                /*
                 * Normally,here ext_hdr_len must not be zero. aux_sync_ind must not run here.
                 * AdvA & ADI mandatory,  CTE Info & Aux Ptr & Sync Info can not exist, TargetA & Tx Power optional
                 */
                if((pExtAdv->ext_hdr_flg & (EXTHD_BIT_ADVA | EXTHD_BIT_CTE_INFO | EXTHD_BIT_ADI | EXTHD_BIT_AUX_PTR | EXTHD_BIT_SYNC_INFO)) \
                                                                                      ==    (EXTHD_BIT_ADVA | EXTHD_BIT_ADI) ){
                    if(initiate_start && ll_secchn_initPkt_cb){

                        if(TRUE == ll_secchn_initPkt_cb(raw_pkt)){  // blt_secchn_procInitPkt
                            initiate_start = 2;

                            //initiate connection successfully
                            bltSche.sche_process_en = 1;
                            blmsParam.create_connection = 0;
                            bltScn.initiate_going = 0;

                            blms_state = BLMS_STATE_SECCHN_SCAN_E; //manual set Scan_post stage, very important!!!
                            systimer_clr_irq_status();
                            systimer_set_irq_capture(clock_time () + BIT(29));

                            blt_set_auxscan_enable(blt_pSecChnScn, 0);
                            //if disable FIX_AUX_CONN_SLOT_IDX_CAL, here need below code
                            ////blt_ll_calculate_sSlot_next(clock_time() + SLOT_PROCESS_MAX_TICK);
                        }

                        blmsParam.rf_fsm_busy = 0;
                    }

                    if(!bltScn.initiate_going && !initiate_start){
                        auxScnCmnParam.rx_received = 1;
                        extadv_auxadv_report = 1;

                        if(blt_pSecChnScn->aux_scan_cnt == 1){
                            blt_pSecChnScn->scan_rx_flag |= SCANRX_FLAG_FIRST_DATA;
                        }else{
                            blt_pSecChnScn->scan_rx_flag &= ~SCANRX_FLAG_FIRST_DATA;
                        }

                        if(!auxptr_info_correct){ //if not
                            blt_pSecChnScn->scan_rx_flag |= SCANRX_FLAG_LAST_DATA;
                        }
                    }
                }
            }
            else if(blt_pSecChnScn->scan_advMode == LL_EXTADV_MODE_SCAN)  //Scannable
            {
                /*
                 * Normally,here ext_hdr_len must not be zero. aux_sync_ind must not run here.
                 * AdvA & ADI mandatory,  CTE Info & Aux Ptr & Sync Info can not exist, TargetA & Tx Power optional
                 */
                if( (pExtAdv->ext_hdr_flg & (EXTHD_BIT_ADVA | EXTHD_BIT_CTE_INFO | EXTHD_BIT_ADI | EXTHD_BIT_AUX_PTR | EXTHD_BIT_SYNC_INFO)) \
                                                                                      ==    (EXTHD_BIT_ADVA | EXTHD_BIT_ADI) ){

                    if(pExtAdv->rf_len ==  pExtAdv->ext_hdr_len + 1){ //no "Adv Data"

                    }

                    if(cur_advdat_len == 0){  //no "ADV Data for Scannable"

                    }

                    /////////////////////////////////////////
                    //todo--qiuwei//EXT_SCNRSP_NOEXIST_CHAIN_PKT
                    if(scanreq_start && blt_pSecChnScn->total_advA_exist){
                        blt_extendedActiveScan_proc(raw_pkt, new_pkt);
                    }


                    //at least one packet:AUX_ADV_IND. At most two packet:AUX_ADV_IND and the first AUX_SCAN_RSP
                    //so rx_received must set 1.
                    auxScnCmnParam.rx_received = 1;
                    extadv_auxadv_report = 1; //directly report to host
                    blt_pSecChnScn->scan_rx_flag |= (SCANRX_FLAG_FIRST_DATA|SCANRX_FLAG_LAST_DATA);//AUX_ADV_IND(scannable) only one pkt
                    /////////////////////////////////////////

                    //TODO: AUX_SCAN_RSP receiving success or failure ...
                    #if(SCAN_BACKOFF_FEATURE_EN)
                        //blt_ll_scanReqBackoff(&bltScn, ...);
                    #else
                        //blt_ll_addScanRspDevice(...);
                    #endif
                }
            }
            /* Non_Connectable Non_Scannable with auxiliary packet */
            else
            {

                int nonscan_nonconn_adv_valid = 0;

                if(blt_pSecChnScn->pdaSync_flag) //AUX_SYNC_IND and it's AUX_CHAIN_IND //set in blt_pda_sync_build_task()
                {
                    if(blt_pSecChnScn->aux_chain_flag) //AUX_CHAIN_IND
                    {
                        /* AdvA & TargetA & Sync Info can not exist, Others optional
                         * here ADI not to judge For convenience,actually ADI may be exist(C3,mandatory if superior PDU present)
                         */
                        if( pExtAdv->ext_hdr_len == 0 || \
                            ((pExtAdv->ext_hdr_flg & (EXTHD_BIT_ADVA|EXTHD_BIT_TARGETA|EXTHD_BIT_SYNC_INFO))== 0 && cur_acad_len == 0) )
                        {
                            nonscan_nonconn_adv_valid = 1;
                            #if PDA_SYNC_EBQ
                                if(blt_pPdAsync->sync_report_allow){
                                    extadv_auxadv_report = 1;
                                }
                            #else
                                extadv_auxadv_report = 1;
                            #endif
                            blt_pSecChnScn->scan_rx_flag |= SCANRX_FLAG_PDA;
                        }
                    }
                    else //AUX_SYNC_IND or AUX_SYNC_SUBEVENT_IND
                    {
                        /* AdvA & TargetA & Sync Info can not exist,  CTE Info & Aux Ptr & Tx Power & ADI mandatory optional */
                        if(blt_pSecChnScn->pdaSync_flag & PADVB_PACKET_FLAG){ //PRDADV packet

                            if( pExtAdv->ext_hdr_len == 0 ||\
                                ((pExtAdv->ext_hdr_flg & (EXTHD_BIT_ADVA | EXTHD_BIT_TARGETA | EXTHD_BIT_SYNC_INFO )) == 0))
                            {
                                nonscan_nonconn_adv_valid = 1;
                                #if PDA_SYNC_EBQ
                                    if(blt_pPdAsync->sync_report_allow){
                                        extadv_auxadv_report = 1;
                                    }
                                #else
                                    extadv_auxadv_report = 1;
                                #endif
                                blt_pSecChnScn->scan_rx_flag |= SCANRX_FLAG_PDA;

                                #if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC && PDA_SYNC_TIMING_ADJUST_EN)
                                    u8 syncHandle = bltPdaSync.pdA_sync_sel;
                                    if(!pda_sync_timingAdjust[syncHandle].rx_1st_tick){
                                        pda_sync_timingAdjust[syncHandle].rx_1st_tick = bltRxPkt.rx_header_tick;
                                    }
                                #endif
                                //my_dump_str_data(DBG_PDA_SYNC_TIMING, "pda data", pExtAdv->data, 16);

                                #if(LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
                                    if(ll_pda_sync_irq_task_cb){
                                        ll_pda_sync_irq_task_cb(FLAG_PRDADV_SYNC_RX, raw_pkt); //blt_pda_sync_interrupt_task
                                    }
                                #endif

                                if(!blt_pSecChnScn->aux_chain_flag && cur_acad_len > 0){

                                    #if(LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)
                                        //acad: maybe include other type(DT_BIGINFO),such as local name.
                                        u8* pAcad = (u8*)(pExtAdv->data + extHdr_offset);

                                        u8* pChmUptAcad = blt_ext_scan_searchAcadInfos(pAcad, cur_acad_len, DT_CHM_UPT_IND);
                                        if(pChmUptAcad){ //chm upt need concerned
                                            if(ll_pda_sync_irq_task_cb){
                                                ll_pda_sync_irq_task_cb(FLAG_PRDADV_SYNC_ACAD_CHMUPT, pChmUptAcad); //blt_pda_sync_interrupt_task
                                            }
                                        }


                                        u8* pBisInfor = blt_ext_scan_searchAcadInfos(pAcad, cur_acad_len, DT_BIGINFO);
                                        if(pBisInfor){

                                            u8 bisInfor_buff[48]; ///just align for parameter use. 48 maybe changed according to actual.
                                            bigInfo_t* pBisAcadInfo = (bigInfo_t*)bisInfor_buff;
                                            smemcpy(pBisAcadInfo, pBisInfor, (pBisInfor[0]+1));  //pBisInfor[0] is len-->len(1B)+flag(1B)+data

                                            if(ll_pda_sync_irq_task_cb){ //blt_ll_period_bisAcad_process
                                                u8 bigInforIdx = ll_pda_sync_irq_task_cb(FLAG_PRDADV_SYNC_ACAD_BIGINFOR, pBisAcadInfo);//blt_pda_sync_interrupt_task  blt_ll_period_bisAcad_process

                                                if(bigInforIdx & BLT_SYNC_HANDLE){ // just to see if the return is correct

                                                    blt_pSecChnScn->scan_rx_flag |= SCANRX_FLAG_BIGINFOR;
                                                    *(u8*)(raw_pkt + DMA_RFRX_OFFSET_TIME_STAMP(raw_pkt)) = bltPdaSync.pdA_sync_sel | BLT_SYNC_HANDLE; ///record sync_handle
                                                }
                                            }
                                        }
                                    #endif
                                }
                            }
                        }
                    #if(LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
                        else if (blt_pSecChnScn->pdaSync_flag & PAWR_PACKET_FLAG){ //PAwR packet
                            if( pExtAdv->ext_hdr_len == 0 ||\
                               ((pExtAdv->ext_hdr_flg & (EXTHD_BIT_ADVA | EXTHD_BIT_TARGETA | EXTHD_BIT_SYNC_INFO | EXTHD_BIT_AUX_PTR )) == 0))
                            {
                                nonscan_nonconn_adv_valid = 1;
                                extadv_auxadv_report = 1;

                                blt_pSecChnScn->scan_rx_flag |= SCANRX_FALG_PAWR;

                                #if (PAWR_SYNC_TIMING_ADJUST_EN)
                                    u8 syncHandle = bltPdaSync.pdA_sync_sel;
                                    if(!pawr_sync_timingAdjust[syncHandle].rx_1st_tick){
                                        pawr_sync_timingAdjust[syncHandle].rx_1st_tick = bltRxPkt.rx_header_tick;
                                    }
                                #endif

                                if(ll_pawr_sync_sub_irq_task_cb){ //blt_ll_PAwRsync_auxSyncSubevtInd_Proc
                                    ll_pawr_sync_sub_irq_task_cb(FLAG_PAWR_SYNC_RX_AUX_SYNC_SUBEVT_IND, raw_pkt, &extHdr_offset); //blt_pawr_sync_sub_interrupt_task
                                }

                                if(cur_acad_len > 0){

                                    //acad: maybe include other type(DT_BIGINFO),such as local name.
                                    u8* pAcad = (u8*)(pExtAdv->data + extHdr_offset);

                                    ////PAwR use the pda channel update processing.
                                    u8* pChmUptAcad = blt_ext_scan_searchAcadInfos(pAcad, cur_acad_len, DT_CHM_UPT_IND);
                                    if(pChmUptAcad){ //chm upt need concerned
                                        if(ll_pda_sync_irq_task_cb){
                                            ll_pda_sync_irq_task_cb(FLAG_PRDADV_SYNC_ACAD_CHMUPT, pChmUptAcad); //blt_pda_sync_interrupt_task
                                        }
                                    }
                                }
                            }
                        }
                    #endif
                    }
                }
                else //AUX_ADV_IND and it's AUX_CHAIN_IND
                {
                    if(pExtAdv->ext_hdr_len == 0){
                        break;
                    }

                    if(blt_pSecChnScn->aux_chain_flag) //AUX_CHAIN_IND
                    {
                        if(!blt_pSecChnScn->aux_scnRsp_chain_flag){
                            /* ADI mandatory(C3, here must present), AdvA & TargetA & Sync Info can not exist, Others optional */
                            if( (pExtAdv->ext_hdr_flg & (EXTHD_BIT_ADVA | EXTHD_BIT_TARGETA | EXTHD_BIT_ADI | EXTHD_BIT_SYNC_INFO)) ==  EXTHD_BIT_ADI && \
                                cur_acad_len == 0 ){
                                nonscan_nonconn_adv_valid = 1;
                            }
                        }else{ //scan response pkt may only have AdvData
                            nonscan_nonconn_adv_valid = 1;
                        }
                    }
                    else //AUX_ADV_IND
                    {
                        /* ADI mandatory,  CTE Info can not exist, Others optional */
                        if( (pExtAdv->ext_hdr_flg & (EXTHD_BIT_ADI | EXTHD_BIT_CTE_INFO)) ==  EXTHD_BIT_ADI ){

                            //TODO: process ACAD
                            //analyze ACAD and judge whether there is "Periodic Advertising Response Timing Information"
                            u8* pAcad = (u8*)(pExtAdv->data + extHdr_offset);
                            u8* pPAwR_timingInfo = NULL;
                            if(cur_acad_len){
                                pPAwR_timingInfo = blt_ext_scan_searchAcadInfos(pAcad, cur_acad_len, DT_PA_RESPONSE_TIMING_INFORMATION);//DT_CHM_UPT_IND
                            }

                            #if(LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER || LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
                                if(p_syncInfo){
                                    blt_pSecChnScn->perdAdv_interval = p_syncInfo->itvl;

                                    if(ll_pda_sync_pawr_sync_common_cb){
                                        ll_pda_sync_pawr_sync_common_cb(p_syncInfo, pPAwR_timingInfo); //blt_ll_pdaSync_pawrSync_info_process
                                    }
                                }
                            #endif

                            nonscan_nonconn_adv_valid = 1;
                        }
                    }

                    if(nonscan_nonconn_adv_valid){

                    #if (EXTENDED_ADV_RPT_MANUAL_EN)
                        if(1)
                    #else
                        if(!blmsParam.pda_syncing_flg)//when prd_adv_sync, not report
                    #endif
                        {
                            extadv_auxadv_report = 1;
                        }
                    }
                }




                if(nonscan_nonconn_adv_valid){
                    auxScnCmnParam.rx_received = 1;

                    if(blt_pSecChnScn->aux_scan_cnt == 1){
                        blt_pSecChnScn->scan_rx_flag |= SCANRX_FLAG_FIRST_DATA;
                    }else{
                        blt_pSecChnScn->scan_rx_flag &= ~SCANRX_FLAG_FIRST_DATA;
                    }

                    if(auxptr_info_correct){
                        more_adv_pkt_exist = 1;
                    }
                    else{  //no AUX_PTR
                        #if (!EXTENDED_ADV_RPT_MANUAL_EN)
                            /* attention, when not report, need release aux_scan source*/
                            /* but periodic scan not release. multiple adv sets share blmsParam.pda_syncing_flg*/
                            /* pdaSync_flag is set in select secTbl, and cleared when terminate or sync lost.so pdaSync_flag can indicate pda pkt*/
                            if(blmsParam.pda_syncing_flg && !blt_pSecChnScn->pdaSync_flag){//blmsParam.pda_syncing_flg

                                //not report extended adv event to host, host can sync the pda quickly???
                                blt_set_auxscan_enable(blt_pSecChnScn, 0);//irq_scan_rx_secondary_channel
                                extadv_auxadv_report = 0;//not report LL/DDI/SCN/BV-25-C must require this.
                            }
                        #endif

                        blt_pSecChnScn->scan_rx_flag |= SCANRX_FLAG_LAST_DATA;
                    }
                }
            }


        }while(0);




        STOP_RF_STATE_MACHINE;
        CLEAR_ALL_RFIRQ_STATUS;

        if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_OFF); } //QW OK



        if(more_adv_pkt_exist)
        {

            //my_dump_str_data(DBG_EXTSCAN_LOGIC, "pri chn ext_adv", (u8 *)(&pExtAdv->rf_len - 1), pExtAdv->rf_len + 2);

            u32 aux_irq_distance = (u32)(aux_expectTick - clock_time());

            if(aux_irq_distance > BIT(30) || aux_irq_distance < 100*SYSTEM_TIMER_TICK_1US )
            {
                my_dump_str_data(DBG_EXTSCAN_TIMING, "aux dis err", &aux_irq_distance, 4);
                BLMS_ERR_DEBUG(DBG_EXTSCAN_TIMING, 0xFE050000);
            }
            else
            {

                st_secchn_scn_t *cur_pauxscn = blt_pSecChnScn;

                u16 aux_pkt_us;
                u16 aux_pkt_max_us = 0;

                if(p_curAux->aux_phy == PHY_USED_AUXPTR_LE_CODED){
                    aux_pkt_us = 3984;
                    aux_pkt_max_us = 17040;
                    cur_pauxscn->scan_duration_flag = DURATION_FLAG_MIN_TIME;
                }
                else{
                    aux_pkt_us = 2120;  //biggest rfLen 255
                    cur_pauxscn->scan_duration_flag = DURATION_FLAG_MAX_TIME;
                }

            #if (!TASK_VERY_CLOSE_DROP_EN)
                int task_very_close = 0;//irq_scan_rx_secondary_channel
                //attention: now do not consider local tolerance
                if(aux_irq_distance < 250 * SYSTEM_TIMER_TICK_1US){ //very close, insert this task

                    task_very_close = 1;
                    cur_pauxscn->tolerance_peer_us = 0;
                    cur_pauxscn->scan_early_set_us = 0;

                    my_dump_str_data(DBG_EXTSCAN_TIMING, "scan secondary channel task very close", 0, 0);
                }
                else
            #endif
                {
                    if(aux_irq_distance < 1000 * SYSTEM_TIMER_TICK_1US){
                        cur_pauxscn->tolerance_peer_us = 0;
                    }
                    else{
                        /* long timing, consider accurate tolerance */
                        if(p_curAux->offset_unit == EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_300_US){
                            //max_length 2.45 S, if 500ppm, 1235 uS
                            cur_pauxscn->tolerance_peer_us = 300;
                        }
                        else{
                            //max_length 245 mS, if 500ppm, 123 uS
                            cur_pauxscn->tolerance_peer_us = 30;
                        }

                        cur_pauxscn->tolerance_peer_us += aux_err_us;
                    }
                }

                cur_pauxscn->scan_early_set_us = cur_pauxscn->tolerance_peer_us + EXTSCAN_PREPARE_US; //prepare_us value: debug later
                cur_pauxscn->scan_duration_us = cur_pauxscn->scan_early_set_us + aux_pkt_us + EXTSCAN_TAIL_MARGIN_US;
                cur_pauxscn->scan_duration_max_us = cur_pauxscn->scan_early_set_us + aux_pkt_max_us + EXTSCAN_TAIL_MARGIN_US;


                if(p_curAux->offset_unit == EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_300_US){
                    cur_pauxscn->aux_pkt_1stRxTm_us = blt_pSecChnScn->scan_early_set_us + bltPHYs.prmb_ac_us + 150 + 300;
                }else{
                    cur_pauxscn->aux_pkt_1stRxTm_us = blt_pSecChnScn->scan_early_set_us + bltPHYs.prmb_ac_us + 150;
                }

                u32 aux_irqTick = aux_expectTick - cur_pauxscn->scan_early_set_us * SYSTEM_TIMER_TICK_1US;
                u32 task_duration_tick = (cur_pauxscn->scan_duration_us + SLOT_PROCESS_MAX_US)*SYSTEM_TIMER_TICK_1US;

                blt_add_aux_scan_future_task(cur_pauxscn->scnIndex, cur_pauxscn->scnIndex + TSKOFT_SECCHN_SCAN, aux_irqTick, aux_irqTick + task_duration_tick);

                //////////PDA can use ADI in spec v5.3
                if(pExtAdv->ext_hdr_flg & EXTHD_BIT_ADI){ //for aux_scan_rsp may be not include ADI section.
                    cur_pauxscn->peerAdv_id.sid = (adi_info>>12) & 0xFF;
                    cur_pauxscn->scan_adi = adi_info;
                }
                //////////

                cur_pauxscn->next_chnIdx = p_curAux->chn_index;
                cur_pauxscn->aux_expect_tick = aux_expectTick;
                cur_pauxscn->aux_irq_tick = aux_irqTick;

            #if (!TASK_VERY_CLOSE_DROP_EN)
                /* check if new AUX task before current primary scan task end time */
                if(task_very_close){
                    bltSche.immediate_task = 1;
                    BLMS_ERR_DEBUG(DBG_EXTSCAN_TIMING, 0xFE090000);  //debug this situation later
                }
            #endif
            }
        }




        if(extadv_auxadv_report){
            raw_pkt[3] = blt_pSecChnScn->scan_rx_flag;
            raw_pkt[2] = (SECCHN_IDX_MARK | blt_pSecChnScn->scnIndex);
            raw_pkt[1] = cur_advdat_len;   //mark EXT_ADV data length
            next_buffer = 1;
            #if (PDA_SCAN_PENDING_FIX_EN)
                if(blt_pSecChnScn->pdaSync_flag){
                    raw_pkt[0] = pdAsync_tbl[blt_pSecChnScn->pdaSync_idx].pda_rx.paEvtCnt&0xFF;//need to store pdaEvtCnt for pdaSync pending.
                }
            #endif
        }
    } //end of if(bltRxPkt.rx_header_tick)
    else
    {
        //RX timeStamp incorrect
        my_dump_str_data(DBG_EXTSCAN_TIMING, "aux RX time err", 0, 0);
    }


    if (!next_buffer)           //reuse buffer
    {
        scan_secRxFifo.wptr--;
        bltExtScn.scan_rx_sec_chn_dma_buff = (u32)raw_pkt;
        ble_rf_set_rx_dma((u8*)bltExtScn.scan_rx_sec_chn_dma_buff, bltExtScn.scan_rx_sec_chn_dma_size);
    }
    else{

    }

    #if (PDA_SCAN_PENDING_FIX_EN)
        if(!blt_pSecChnScn->pdaSync_flag){
            raw_pkt[0] = 1;
        }
    #else
        raw_pkt[0] = 1;
    #endif



    if(!blt_pSecChnScn->pdaSync_flag && initiate_start != 2){
        systick_irq_trigger = SYS_IRQ_TRIG_SECCHN_SCAN_POST;
        blmsParam.stimer_irq_process_en = 1;
    }


    return 0;
}









u8 blt_extscan_convert_direct_adr_type(u8 direct_adrType, u8 *direct_addr, u8 rpa_resole_fail)
{
    if(direct_adrType == BLE_ADDR_RANDOM){
        if(IS_RESOLVABLE_PRIVATE_ADDR(direct_adrType, direct_addr)){
            if(rpa_resole_fail){
                return DIRECT_ADDR_RPA_FAIL;
            }
            else{
                return bltScn.scan_ownAddr_random ? DIRECT_ADDR_RPA_RANDOM : DIRECT_ADDR_RPA_PUBLIC;
            }
        }
        else{
            return DIRECT_ADDR_NRPA_STATIC;
        }
    }
    else{
        return DIRECT_ADDR_PUBLIC;
    }
}



#endif // end of LL_FEATURE_ENABLE_LE_EXTENDED_SCAN
