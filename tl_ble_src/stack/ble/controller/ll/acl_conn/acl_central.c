/********************************************************************************************************
 * @file    acl_central.c
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


#if (LL_ACL_CEN_EN)

    #define BTX_INVALID_SLOT 0xFFFF


//only for Master
_attribute_ble_data_retention_ _attribute_aligned_(4) st_llm_conn_t blmsMaster[LL_MAX_ACL_CEN_NUM];
_attribute_ble_data_retention_ _attribute_aligned_(4) blm_pkt_pending_t blmsMasterEncPktPending[LL_MAX_ACL_CEN_NUM];
_attribute_ble_data_retention_ _attribute_aligned_(4) st_llm_conn_t *blm_pconn;

_attribute_ble_data_retention_ _attribute_aligned_(4) acl_mas_para_t aclMas_param;

#if (LL_MAX_ACL_CEN_NUM <= 4)
#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
__attribute__((section(".data"), aligned(4))) 
#else
const
#endif
u8    bSlot_position_tbl[3][4] = {
     {0, 1, 0, 0},     // master number = 2:  1/1
     {0, 1, 2, 0},     // master number = 3:  1/3,  2/3
     {0, 2, 1, 3},     // master number = 4:  2/4,  1/4,  3/4
};
    #else

    #endif


void blc_ll_initAclCentralRole_module(void)
{
    ll_acl_master_irq_task_cb = blt_acl_master_interrupt_task;
    ll_acl_master_mlp_task_cb = blt_acl_master_mainloop_task;


    blmsParam.acl_master_en = 1;

    if (!aclMas_param.master_connInter) {
        aclMas_param.master_connInter = CONN_INTERVAL_31P25MS; // default value
    }

    for (int i = 0; i < LL_MAX_ACL_CEN_NUM; i++) {
        blm_pconn = (st_llm_conn_t *)&blmsMaster[i];
        //      blm_pconn->aclCen_index = i;

        for (int j = 0; j < ACL_MASTER_FIFONUM; j++) {
            blm_pconn->aclTsk_fifo[j].scheTask_oft = TSKOFT_ACL_MASTER + i;
            blm_pconn->aclTsk_fifo[j].scheTask_idx = i;
            blm_pconn->aclTsk_fifo[j].scheTask_flg = TSKFLG_BSLOT_ALIGN | TSKFLG_ACL_MASTER;
        }
    }


    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
        //      STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(acl_mas_para_t)), acl_central);
    #endif
}

_attribute_ram_code_ int blt_acl_master_interrupt_task(int flag, void *p)
{
    int conn_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if (flag & FLAG_SCHEDULE_START) {
        blt_btx_start(conn_idx, p);
    } else if (flag & FLAG_SCHEDULE_DONE) {
        blt_btx_post();
    } else if (flag & FLAG_SCHEDULE_BUILD) {
        blt_ll_buildAclMasterSchedulerLinklist();
    } else if (flag & FLAG_INSERT_SCHTSK_CONFLICT) {
        //sch_task_t *pTgtTsk = (sch_task_t *)p;
        //u8 tgtTskFlg = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
        //my_dump_str_data(DBG_EXTADV_TIMING, "[acl_mst]insertTsk conflict, tgtTsk=", &tgtTskFlg, 1);
    }

    return 0;
}

_attribute_noinline_ int blt_acl_master_mainloop_task(int flag, void *p)
{
    //int conn_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if (flag & FLAG_MODULE_SET_HOST_CHM) {
        blt_ll_ctrlAclMstChClassUpd((u8 *)p);
    } else if (flag & FLAG_MODULE_RESET) {
        blt_ll_reset_acl_master();
    } else if (flag & FLAG_CHECK_INIT) {
        blt_aclc_check_init();
    }

    return 0;
}

void blt_ll_reset_acl_master(void)
{
    for (int i = 0; i < LL_MAX_ACL_CEN_NUM; i++) {
        aclMas_param.position_mask[i]       = 0;
        aclMas_param.bSlot_mark_position[i] = 0;
    }
}

int blt_aclc_check_init(void)
{
    #if (ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN)
    if (aclMas_param.timCus_en) {
        if (aclMas_param.bSlotDurn_diffMod) {
            aclMas_param.bSlot_offset_pos0[0] = 0;
            for (int i = 1; i <= aclMas_param.timposn_max; i++) {
                aclMas_param.bSlot_offset_pos0[i] = aclMas_param.bSlot_offset_pos0[i - 1] + aclMas_param.bSlot_number[i - 1];
            }
        } else {
            for (int i = 0; i < blmsParam.max_master_num; i++) {
                aclMas_param.bSlot_number[i]      = aclMas_param.bSlotNum_sameMod;
                aclMas_param.bSlot_offset_pos0[i] = aclMas_param.bSlotNum_sameMod * i;
            }
        }


        my_dump_str_u8s(DBG_CUSTOM_ACLC_TIMING, "[CUS CEN] bSlot number", aclMas_param.bSlot_number[0], aclMas_param.bSlot_number[1], aclMas_param.bSlot_number[2], aclMas_param.bSlot_number[3])

            my_dump_str_u8s(DBG_CUSTOM_ACLC_TIMING, "[CUS CEN] bSlot offset", aclMas_param.bSlot_offset_pos0[0], aclMas_param.bSlot_offset_pos0[1], aclMas_param.bSlot_offset_pos0[2], aclMas_param.bSlot_offset_pos0[3])
    }
    #endif

    return INIT_SUCCESS;
}

ble_sts_t blc_ll_initAclCentralTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number, int conn_number)
{
    bltempParam.ll_aclTxMasFifo_set = 1;

    /* Different process for different MCU: ******************************************/
    if (fifo_number == 9) {
        blt_m_txfifo.depth     = 3;
        blt_m_txfifo.real_num  = 9;
        blt_m_txfifo.logic_num = 8;
        blt_m_txfifo.mask      = 7;
    } else if (fifo_number == 17) {
        blt_m_txfifo.depth     = 4;
        blt_m_txfifo.real_num  = 17;
        blt_m_txfifo.logic_num = 16;
        blt_m_txfifo.mask      = 15;
    } else if (fifo_number == 33) {
        blt_m_txfifo.depth     = 5;
        blt_m_txfifo.real_num  = 33;
        blt_m_txfifo.logic_num = 32;
        blt_m_txfifo.mask      = 31;
    } else {
        //4, 2 is too small
        return LL_ERR_INVALID_PARAMETER;
    }


    /* size must be 16*n */
    if ((fifo_size & 15) == 0) {
        blt_m_txfifo.size = fifo_size;
        //      blt_m_txfifo.size_div_16 = fifo_size>>4;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }
    if (fifo_size < 48) {
        return LL_ERR_INVALID_PARAMETER;
    }
    #if (MCU_HARDWARE_TX_FIFO_4K_BYTES_LIMITATION) //only B91 have this limitation
    if ((fifo_number - 1) * fifo_size >= 4096) {
        return LL_ERR_INVALID_PARAMETER;
    }
    #endif

    blt_m_txfifo.conn_full_size = fifo_size * fifo_number;


    blt_m_txfifo.p_base = pTxbuf;


    for (int i = 0; i < conn_number; i++) {
        u8 *pBuff_Default = blt_m_txfifo.p_base + i * fifo_size * fifo_number;
        smemcpy(pBuff_Default, (u8 *)blms_tx_empty_packet, 6);
    }


    for (int i = ACL_CONN_IDX_CEN0; i < conn_number; i++) {
        blms[i].max_fifo_num = fifo_number - 1;
    }


    /**********************************************************************************/

    return BLE_SUCCESS;
}

    #define TIMING_TYPE_CONNECT 1
    #define TIMING_TYPE_UPDATE  0

_attribute_ble_data_retention_ u32 bSlot_winOffsetStart; //no need retention
_attribute_ble_data_retention_ u32 btx_anchor_point;

_attribute_ram_code_
    u32
    blt_ll_calMasterInitiateTiming(int tim_type, st_llm_conn_t *pm)
{
    /*attention: can not use blm_pconn / blm_pconn / blms_conn_sel
     * in this function, cause conn_update_cmd will call it in main_loop */
    u32  btx_slot        = 0;
    int  pos_combine_en  = 0;
    int  pos_offset_num  = 0;
    bool allocate_result = FALSE;

    pm->init_pos_msk = bltInit.mas_intv_msk;
    if (blmsParam.cur_master_num == 0) {
    #if (LL_ASYNC_LEA_EN)
        if (asyncCtrl.windowOffsetBslot && asyncCtrl.connState) {
            btx_slot = asyncCtrl.windowOffsetBslot;
        } else {
            btx_slot = aclMas_param.bSlotNum_whole_inter >> 1;
        }
    #else
        btx_slot = aclMas_param.bSlotNum_whole_inter >> 1;
    #endif

    /**
         * only fixed BIS ISO interval = 10ms*n, ACL central Base Connection interval = 10ms * 2 * n, n = m * max_Central_count.
         * This scheme is limited to broadcasting BIS first and establishing a new ACL connection.
         * Scanning only can do during non-BIS broadcast cycles, Set first ACL packet to n*ISO_Interval , can avoid conflicts between ACLs and BIS.
         *
         */
    #if (BIS_CENTRAL_ACL_CENTRAL_TIMING_STAGGERED)
        btx_slot = 24;
    #endif


        btx_anchor_point = bSlot_winOffsetStart + btx_slot; //update when 0 master -> 1 master
        if (tim_type == TIMING_TYPE_CONNECT) {
            aclMas_param.bslot_1st_btx_mark = btx_anchor_point;
        } else {
            pm->bSlot_1stBtx_mark_update_hold = btx_anchor_point; //this value can not be zero
        }

        pm->init_pos_idx = 0;
        allocate_result  = TRUE;
        //my_dump_str_u32s(DBG_MASTER_CONN_UPDATE, "update 0", conn_idx, pm->init_pos_idx, bltInit.mas_intv_msk, aclMas_param.position_mask[0]);
    } else {
        //find which slot offset is available
        int pos_idx_1st_idle = 0x80;
        for (int i = 0; i < blmsParam.max_master_num; i++) {                                         //traverse all master slot
            if (aclMas_param.position_mask[i]) {
                if (aclMas_param.position_mask[i] != 0xFFFFFF && bltInit.mas_intv_msk != 0xFFFFFF) { //still have timing gap
                    for (int j = 0; j < bltInit.mas_intv_mul; j++) {
                        u32 new_mask = bltInit.mas_intv_msk << j;
                        if ((aclMas_param.position_mask[i] & new_mask) == 0) {                       //no overlap
                            pm->init_pos_idx = i;
                            pm->init_pos_msk = new_mask;
                            pos_offset_num   = j;                                                    //attention: pos_offset_num maybe "0"
                            pos_combine_en   = 1;
                            allocate_result  = TRUE;
                            break;                                                                   //find first available timing which can insert to combine, use it, break
                        }
                    }
                }
            } else {                      //idle
                if (pos_idx_1st_idle == 0x80) {
                    pos_idx_1st_idle = i; //mark first available idle timing
                    allocate_result  = TRUE;
                    //attention: do not break here, hope to find a insert timing(take higher priority)
                }
            }
        }

        if (allocate_result == TRUE) {
            if (!pos_combine_en) { //not combine, use a new position
                pm->init_pos_idx = pos_idx_1st_idle;
            }

            /* blmsParam.max_master_num smaller than 2 can never enter here */
            int slotNum_offset = bSlot_position_tbl[blmsParam.max_master_num - 2][pm->init_pos_idx] * aclMas_param.bSlotNum_piece_inter;
            /* slot_offset_1st_btx can not be "u8", which lead to conn_interval exceed 160mS error */
            int slot_offset_1st_btx = aclMas_param.bSlotNum_whole_inter - (bSlot_winOffsetStart - aclMas_param.bslot_1st_btx_mark) % aclMas_param.bSlotNum_whole_inter;
            btx_slot                = slot_offset_1st_btx + slotNum_offset;

            if (btx_slot > aclMas_param.bSlotNum_whole_inter) {
                btx_slot -= aclMas_param.bSlotNum_whole_inter;
            }

            btx_anchor_point = bSlot_winOffsetStart + btx_slot;

            if (pos_combine_en) { //not zero offset of current position, need find correct BTX anchor point
                u32 bSlot_distance = (btx_anchor_point - aclMas_param.bSlot_mark_position[pm->init_pos_idx]);

                if ((bSlot_distance % aclMas_param.bSlotNum_whole_inter) != 0) {
                    my_dump_str_u32s(ACL_MASTER_INITIATE, "err 1", btx_anchor_point, pm->init_pos_idx, aclMas_param.bSlot_mark_position[pm->init_pos_idx], bSlot_distance);
                    BLMS_ERR_DEBUG(ACL_MASTER_INITIATE, 0xCC010000);
                }

                int mas_intv_offset = (bSlot_distance / aclMas_param.bSlotNum_whole_inter) % bltInit.mas_intv_mul;
                int mas_intv_add;
                if (pos_offset_num >= mas_intv_offset) {
                    mas_intv_add = pos_offset_num - mas_intv_offset;
                } else {
                    mas_intv_add = pos_offset_num + bltInit.mas_intv_mul - mas_intv_offset;
                }
                btx_slot += (mas_intv_add * aclMas_param.bSlotNum_whole_inter);
                btx_anchor_point = bSlot_winOffsetStart + btx_slot; //update

                //my_dump_str_u32s(ACL_MASTER_INITIATE, "initiate 2_0", pos_offset_num, bSlot_distance, mas_intv_offset, mas_intv_add);
            }


            my_dump_str_u32s(ACL_MASTER_INITIATE, "initiate 2_1", pos_combine_en, pm->init_pos_idx, pos_offset_num, aclMas_param.position_mask[pm->init_pos_idx]);
        }
    }

    if (allocate_result == TRUE) {
        /* zero offset of a new position, mark it, later offset will use if any other BTX can insert to current position */
        if (!pos_combine_en) {
            if (tim_type == TIMING_TYPE_CONNECT) {
                aclMas_param.bSlot_mark_position[pm->init_pos_idx] = btx_anchor_point;
            } else {
                pm->bSlotMark_position_update_hold = btx_anchor_point; //this value can not be zero
            }
        }

        if (tim_type == TIMING_TYPE_CONNECT) {
            aclMas_param.position_mask[pm->init_pos_idx] |= pm->init_pos_msk; //combine with exist init_position
        }

        return btx_slot;
    } else {
        return BTX_INVALID_SLOT;
    }
}


    #if (ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN)

_attribute_ram_code_
    u32
    blt_ll_calCenTaskConcentratedInitTiming(int tim_type, st_llm_conn_t *pm)
{
    /*attention: can not use blm_pconn / blm_pconn / blms_conn_sel
     * in this function, cause conn_update_cmd will call it in main_loop */
    u32 btx_slot       = 0;
    u8  pos_offset_num = 0;

    int allocate_result = 0; //0: no space; 1: new space; 2: insert combine space


    pm->init_pos_msk = bltInit.mas_intv_msk;
    if (blmsParam.cur_master_num == 0) {
        btx_slot         = aclMas_param.bSlotNum_whole_inter >> 1; //select the middle of whole winOffset timing
        btx_anchor_point = bSlot_winOffsetStart + btx_slot;

        u32 mark_1st_btx_pos_bSlot = btx_anchor_point;
        if (aclMas_param.bSlotDurn_diffMod) {
            mark_1st_btx_pos_bSlot -= aclMas_param.cur_bslotOft_diffMod;
            pm->init_pos_idx = aclMas_param.cur_timPosn_diffMod;
        } else {
            pm->init_pos_idx = 0;
        }

        if (tim_type == TIMING_TYPE_CONNECT) {
            aclMas_param.bslot_1st_btx_mark = mark_1st_btx_pos_bSlot;
        } else {
            pm->bSlot_1stBtx_mark_update_hold = mark_1st_btx_pos_bSlot; //this value can not be zero
        }

        allocate_result = 1;

        my_dump_str_u32s(DBG_CUSTOM_ACLC_TIMING, "[CUS CEN] cal timing, 1st new", aclMas_param.aclc_idx_conn | pm->init_pos_idx << 8 | aclMas_param.cur_bslotOft_diffMod << 16, 0, 0, pm->init_pos_msk);
    } else {
        u8 tim_pos_start;
        u8 tim_pos_end;
        if (aclMas_param.bSlotDurn_diffMod) {
            tim_pos_start = tim_pos_end = aclMas_param.cur_timPosn_diffMod;
        } else {
            tim_pos_start = 0;
            tim_pos_end   = blmsParam.max_master_num;
        }

        u8 find_1st_idle_pos = 0;
        for (int i = tim_pos_start; i < (tim_pos_end + 1); i++) {
            if (aclMas_param.position_mask[i]) {
                if (aclMas_param.position_mask[i] != 0xFFFFFF && bltInit.mas_intv_msk != 0xFFFFFF) { //still have timing gap
                    for (int j = 0; j < bltInit.mas_intv_mul; j++) {
                        u32 new_mask = bltInit.mas_intv_msk << j;
                        if ((aclMas_param.position_mask[i] & new_mask) == 0) {                       //no overlap
                            pm->init_pos_idx = i;
                            pm->init_pos_msk = new_mask;
                            pos_offset_num   = j;                                                    //attention: pos_offset_num maybe "0"
                            allocate_result  = 2;
                            break;                                                                   //find first available timing which can insert to combine, use it, break
                        }
                    }
                }
            } else {                       //idle
                if (!find_1st_idle_pos) {
                    find_1st_idle_pos = 1; //mark first available idle timing
                    pm->init_pos_idx  = i;
                    allocate_result   = 1;
                    //attention: do not break here, hope to find a insert timing(take higher priority)
                }
            }

            if (allocate_result == 2) {
                break; //when find first insert combine space, use it, so break
            }
        }


        if (allocate_result) {
            u32 bSlot_cur_interval = bltInit.mas_intv_mul * aclMas_param.bSlotNum_whole_inter;

            /* slot_offset_1st_btx can not be "u8", which lead to conn_interval exceed 160mS error */
            int slot_offset_1st_btx = bSlot_cur_interval - ((u32)(bSlot_winOffsetStart - aclMas_param.bslot_1st_btx_mark)) % bSlot_cur_interval;

            btx_slot = slot_offset_1st_btx + aclMas_param.bSlot_offset_pos0[pm->init_pos_idx];
            if (btx_slot > bSlot_cur_interval) {
                btx_slot -= bSlot_cur_interval;
            }

            if (allocate_result == 2) {
                btx_slot += (pos_offset_num * aclMas_param.bSlotNum_whole_inter);
            }

            btx_anchor_point = bSlot_winOffsetStart + btx_slot;

            my_dump_str_u32s(DBG_CUSTOM_ACLC_TIMING, "[CUS CEN] cal timing, second", aclMas_param.aclc_idx_conn | pm->init_pos_idx << 8 | aclMas_param.cur_bslotOft_diffMod << 16, allocate_result, pos_offset_num, pm->init_pos_msk);
        }
    }

    if (allocate_result) {
        if (tim_type == TIMING_TYPE_CONNECT) {
            aclMas_param.position_mask[pm->init_pos_idx] |= pm->init_pos_msk; //combine with exist init_position
        }

        return btx_slot;
    } else {
        return BTX_INVALID_SLOT;
    }
}

    #endif


/*
 *BLE_PHY_1M:    (rf_len + 34 + 10*2)*8 + 150 = rf_len*8 + 582 //conn_req/aux_conn_req(34)
 *BLE_PHY_2M:    (rf_len + 34 + 11*2)*4 + 150 = rf_len*4 + 374 //aux_conn_req(34)
 BLE_PHY_CODED:  (rf_len + 34)*64 + 1440 + 150 = rf_len*64 + 3766 //aux_conn_req(34)
 *
 */
__attribute__((section(".data"), aligned(4))) u16 rx_connreq_offset_us[4] = {0, 582, 374, 3766};

_attribute_ram_code_ bool blms_m_connect(rf_packet_connect_t *pInit, u8 *raw_pkt)
{
    /////// remember that: STX for "pkt_init" is settled, all data of "pkt_init" should set ASAP.  ///////
    /////// If too late, this packet may be ERR  ///////

    /* The value of transmitWindowDelay shall be 1.25 ms when a CONNECT_IND PDU is used, 2.5 ms when an AUX_CONNECT_REQ PDU is used on an LE
    Uncoded PHY, and 3.75 ms when an AUX_CONNECT_REQ PDU is used on the LE Coded PHY. */
    int bSlot_num_txWinDly = 2; //CONNECT_IND, 1.25 mS = 625uS * 2
    if (bltInit.sec_chn_init) {
        bSlot_num_txWinDly = bltPHYs.cur_llPhy == BLE_PHY_CODED ? 6 : 4;
    }

    u32 rxAdv_connReq_total_us = raw_pkt[DMA_RFRX_OFFSET_RFLEN] * bltPHYs.peer_oneByte_us + rx_connreq_offset_us[bltPHYs.cur_llPhy];
    u32 systick_connReq_tail   = bltRxPkt.rx_header_tick + rxAdv_connReq_total_us * SYSTEM_TIMER_TICK_1US;

    /* Don't need a pointer callback function for now, it's relatively brief. No need */
    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
    if (blmsParam.create_connection == CONNECT_REQ_FOR_PAWR) { //&& blt_pPerdadv->num_subevents
        systick_connReq_tail = bltSche.bSlot_tick_irq_real + TLK_TX_TRIG_OFFSET * SYSTEM_TIMER_TICK_1US;
        systick_connReq_tail += blt_phy_getRfPacketTime_us(34, blt_pPerdadv->pda_tx.pda_phy, blt_pPerdadv->coding_ind);
        bltRxPkt.rx_irq_tick = clock_time();
    }
    #endif


    #if (!FIX_AUX_CONN_SLOT_IDX_CAL)
    int n_sSlot             = 1 + (systick_connReq_tail + SLOT_PROCESS_MAX_TICK - bltSche.sSlot_tick_irq_real) * SSLOT_TICK_REVERSE;
    bltSche.sSlot_idx_next  = bltSche.sSlot_idx_irq_real + n_sSlot;
    bltSche.sSlot_tick_next = bltSche.sSlot_tick_irq_real + n_sSlot * SSLOT_TICK_NUM;
    #endif

    // 7 = 2 (1.25 mS = 2 slot) + 5 ( half win_size *2 = 2.5*2 = 5 bSlot  )
    int n_bSlot          = bSlot_num_txWinDly + BSLOT_NUM_HALF_WINSIZE + 1 + (systick_connReq_tail - bltSche.bSlot_tick_irq_real) / SYSTEM_TIMER_TICK_625US;
    bSlot_winOffsetStart = bltSche.bSlot_idx_irq_real + n_bSlot;


    #if (ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN)
    if (aclMas_param.timCus_en) {
        blms_conn_sel = aclMas_param.aclc_idx_conn;
    } else
    #endif
    {
        for (blms_conn_sel = ACL_CONN_IDX_CEN0; blms_conn_sel < blmsParam.max_master_num; blms_conn_sel++) {
            if (!blms[blms_conn_sel].connState) {
                break;
            }
        }
    }


    #if 0 //this could never happen
    if(blms_conn_sel == blmsParam.max_master_num){
        my_dump_str_data(STACK_DUMP_EN, "ERROR, master number error", 0, 0);
        return FALSE;
    }
    #endif

    st_ll_conn_t  *pc = (st_ll_conn_t *)&blms[blms_conn_sel];
    st_llm_conn_t *pm = (st_llm_conn_t *)&blmsMaster[blms_conn_sel];

    u32 btx_slot;

    #if (ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN)
    if (aclMas_param.timCus_en) {
        btx_slot = blt_ll_calCenTaskConcentratedInitTiming(TIMING_TYPE_CONNECT, pm);
        //pm->aclc_idx_mark = aclMas_param.aclc_idx_conn;

        //          btx_slot = aclMas_param.bSlotNum_whole_inter>>1;  //select the middle of whole winOffset timing
        //          btx_anchor_point = bSlot_winOffsetStart + btx_slot;
        //          pm->init_pos_idx = 0;
        //          aclMas_param.position_mask[0] |= bltInit.mas_intv_msk;

    } else
    #endif
    {
        btx_slot = blt_ll_calMasterInitiateTiming(TIMING_TYPE_CONNECT, pm);
    }

    if (btx_slot == BTX_INVALID_SLOT) {
        BLMS_ERR_DEBUG(ACL_MASTER_INITIATE, 0xCC020000);
    }

    pkt_init.woffset = btx_slot >> 1;

    ////// when code running here, all data of "pkt_init" is set, packet sending is OK(but not finish) //////
    #if (LL_ASYNC_LEA_EN)
    if (bltScn.asyncScanIndex) {
        pInit->interval           = CONN_INTERVAL_10MS;
        asyncCtrl.aclIntervalTick = CONN_INTERVAL_10MS * SYSTEM_TIMER_TICK_1250US;
    }
    #endif

    pc->create_conn_status = BLE_SUCCESS;

    if (bltInit.sec_chn_init) {
        //if(ll_ext_init_irq_task_cb) //not judge to save RamCode
        {
            if (ll_ext_init_irq_task_cb(FLAG_SCHEDULE_EXTINIT_CHECK_CONNRSP)) {            // blt_ext_init_interrupt_task

            } else {
                aclMas_param.position_mask[pm->init_pos_idx] &= ~pm->init_pos_msk;         //important to recover
                if (blmsParam.create_connection == CONNECT_REQ_FOR_PAWR) {
                    pc->irq_event1_union.connect_evt = 1 | ENHANCED_CONN_FLAG_AUX_CONNECT; //CallBack process later in mainLoop
                    pc->create_conn_status           = HCI_ERR_CONN_FAILED_TO_ESTABLISH;
                }
                return FALSE;
            }
        }
    }


    ////// when code running here, connection complete happens //////

    pm->bSlot_1stBtx_mark_update_hold = 0; //clear when connect, more secure
    my_dump_str_u32s(ACL_MASTER_INITIATE, "initiate 1_2", aclMas_param.position_mask[0], aclMas_param.position_mask[1], aclMas_param.position_mask[2], aclMas_param.position_mask[3]);


    pc->bSlot_interval  = pkt_init.interval << 1; // 1.25 mS unit -> 625 uS unit
    pc->bSlot_mark_conn = btx_anchor_point - pc->bSlot_interval;

    /* PM use  conn_tick to calculate future BTX point, in case that in the middle of blms_m_connect and first BTX, conn_tick will be invalid value.
     * here bSlot mark may be smaller or bigger than bSlot_ idx_irq_real, so change to s32 */
    pc->conn_tick_mark = bltSche.bSlot_tick_irq_real + (int)(pc->bSlot_mark_conn - bltSche.bSlot_idx_irq_real) * SYSTEM_TIMER_TICK_625US;


    blt_sche_addTaskMask(TSKMSK_ACL_MASTER_0 << blms_conn_sel);
    blt_sche_addUpdate(SLOT_UPDT_MASTER_CONN);
    blmsParam.cur_master_num++;
    aclConn_param.tick_connectDevice = 0;


    rf_packet_adv_t *pAdv = (rf_packet_adv_t *)(raw_pkt + DMA_RFRX_LEN_HW_INFO);
    pc->peer_chnSel       = bltInit.sec_chn_init ? 1 : pAdv->chan_sel;

    pc->conn_tick = clock_time();

    /* attention: peer packet address must set before "blms connect common" !!! */
    pc->conn_peerPktA_type = pInit->rxAddr;
    smemcpy(pc->conn_peerPktA, pInit->advA, BLE_ADDR_LEN);

    blms_connect_common(pc, pInit, bltInit.sec_chn_init);

    pc->pdu_task_us = PAYLOAD_27B_TIFS_27B_ENCRT_1MPHY_US; //can only be 1M PHY when connected; encryption add some time very small, neglect

    blmsMasterEncPktPending[blms_conn_sel].wptr = blmsMasterEncPktPending[blms_conn_sel].rptr = 0;
    blmsMasterEncPktPending[blms_conn_sel].num                                                = BLM_ENC_PKT_PENDING_NUM;
    blmsMasterEncPktPending[blms_conn_sel].mask                                               = BLM_ENC_PKT_PENDING_NUM - 1;

    blmhostChnClassUpt.hostMapUptCmdPending &= ~BIT(blms_conn_sel);

    #if (SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY)
    /* for SMP: SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY, save idenAdr_type/idenAdr_addr */
    blt_ll_record_identity_address(bltInit.init_mac_type, bltInit.init_mac_addr);
    #endif

    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    //  if(ll_acl_subrate_irq_task_cb){
    //      ll_acl_subrate_irq_task_cb(FLAG_ACL_SUBRATE_CONN_CB, (void*)pc);
    //  }
    blt_ll_initSubrateByHandle(pc->acl_conHandle); // must call this API, regardless of whether the subrate module init or not
    #endif

    /*
     * If the Link Layer in the Central role supports receiving LL Control PDUs with a
     * CtrData field longer than 26 octets, it should initiate the Feature Exchange
     * procedure on each connection.
     *
     * EBQ test: IUT should not auto initiate FeatureReq. Actually this function is needed.
     */
    #if (!LONG_CTRL_PDUS_AUTO_FEATURE_REQ_DIS)
    if (blmsParam.past_en) {
        /* In BLE 5.3 version and below, and under the Central role:
         * Only PAST recipient LLCP meet the condition: receive LL Control PDUs */
        blt_ll_send_feature_req(pc);
    }
    #endif


    #if (FIX_AUX_CONN_SLOT_IDX_CAL)
    /* attention: do not use "blt_ll_calculate_sSlot_next" here, must use "sSlot_tick_irq_real" */
    int n_sSlot             = 1 + (clock_time() + (bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US) * SYSTEM_TIMER_TICK_1US - bltSche.sSlot_tick_irq_real) * SSLOT_TICK_REVERSE;
    bltSche.sSlot_idx_next  = bltSche.sSlot_idx_irq_real + n_sSlot;
    bltSche.sSlot_tick_next = bltSche.sSlot_tick_irq_real + n_sSlot * SSLOT_TICK_NUM;
    #endif

    #if (LL_ASYNC_LEA_EN)
    if (asyncCtrl.leaUsed && bltScn.asyncScanIndex) {
        pc->async_lea_link = pc->acl_conHandle | BLM_ASYNC_HANDLE;
    }
    #endif

    return TRUE;
}

//todo SiHui: test running time in flash with 24m/32m/48 clock, we want to save some SRAM
_attribute_ram_code_ //must be RamCode, should execute ASAP, test data: 32M clock, 22us most 20101111
    bool
    blt_ll_calConnUpdateTiming(u8 conn_idx, st_ll_conn_t *pc, st_llm_conn_t *pm, rf_packet_connect_upd_req_t *pUpdate)
{
    (void)conn_idx;        //unused, remove warning
    u32 r = irq_disable(); //very important to disable IRQ

    /* ACL master task may abandon due to priority management, so here we need check if latest conn_inst is too far */
    u32 inst_passed = ((u32)(clock_time() - pc->conn_tick_mark)) / pc->conn_intvl_tick;

    if (inst_passed > 1024) {
        BLMS_ERR_DEBUG(DBG_MASTER_CONN_UPDATE, 0xDD030000);
    }

    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    int inst_diff = inst_passed + pc->conn_latency + 11 + pc->factor * 10; //TODO: can optimize conn_latency, but very difficult.
    #else
    int inst_diff = inst_passed + pc->conn_latency + 11; //TODO: can optimize conn_latency, but very difficult.
    #endif


    #if (LL_ASYNC_LEA_EN)
    if (pc->async_lea_link && asyncCtrl.connState) {
        inst_diff = inst_passed + pc->conn_latency + 6;
        if (pc->conn_inst_mark % 2) {
            inst_diff++;
        }
        bSlot_winOffsetStart = pc->bSlot_mark_conn + inst_diff * pc->bSlot_interval;
    } else {
        bSlot_winOffsetStart = pc->bSlot_mark_conn + inst_diff * pc->bSlot_interval + BSLOT_NUM_HALF_WINSIZE;
    }
    #else
    bSlot_winOffsetStart = pc->bSlot_mark_conn + inst_diff * pc->bSlot_interval + BSLOT_NUM_HALF_WINSIZE;
    #endif

    pUpdate->instant = pc->conn_inst_mark + inst_diff;
    //my_dump_str_u32s(DBG_MASTER_CONN_UPDATE, "conn inst", inst_diff, pc->conn_inst, pUpdate->instant, 0);


    /*******************************************************************************/
    /* remove old timing
      * init_pos_idx & init_pos_msk may change in later calculating, must backup. */
    u8  backup_pos_idx = pm->init_pos_idx;
    u32 backup_pos_msk = pm->init_pos_msk; // "u32" !!!
    aclMas_param.position_mask[backup_pos_idx] &= ~backup_pos_msk;
    blmsParam.cur_master_num--;
    /*******************************************************************************/

    /* attention: */
    int btx_slot = blt_ll_calMasterInitiateTiming(TIMING_TYPE_UPDATE, pm);

    int allocate_result;
    if (btx_slot != BTX_INVALID_SLOT) {
        allocate_result    = TRUE;
        pUpdate->winOffset = btx_slot >> 1;
    /*attention: winOffset different from slave:
         *slave use winOffset to locate RX window start point, but master know exact timing */
    #if (LL_ASYNC_LEA_EN)
        if (pc->async_lea_link && asyncCtrl.connState) {
            pc->bSlot_oft_num_next = btx_slot;
        } else {
            pc->bSlot_oft_num_next = BSLOT_NUM_HALF_WINSIZE + btx_slot;
        }
    #else
        pc->bSlot_oft_num_next = BSLOT_NUM_HALF_WINSIZE + btx_slot;
    #endif

    } else { //fail
        allocate_result = FALSE;
    }


    /*************************************************************/
    /* recover old timing */
    aclMas_param.position_mask[backup_pos_idx] |= backup_pos_msk;
    pm->updt_pos_idx = pm->init_pos_idx;
    pm->init_pos_idx = backup_pos_idx;
    pm->updt_pos_msk = pm->init_pos_msk;
    pm->init_pos_msk = backup_pos_msk;
    blmsParam.cur_master_num++;
    /*************************************************************/

    irq_restore(r); //must restore IRQ

    return allocate_result;
}

_attribute_ram_code_ int blt_btx_start(int conn_idx, void *p)
{
    (void)p;
    blms_start_pre_process(conn_idx);
    blm_pconn = (st_llm_conn_t *)&blmsMaster[blms_conn_sel]; //this should do before BTX trigger

    #if (SL01_aclc_0)
    log_task_begin_irq(SL_STACK_ACL_BASIC_TIMING_EN, SL01_aclc_0 + blms_conn_sel);
    #endif

    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    blms_pconn->subrate_flag.bit.subrate_evt_flag = ((((sch_task_t *)p)->subrate_evt_flag));
    #endif

    #if (OPTIMIZE_INSERT_EMPTY_EN)
    blms_pconn->llcp_flag.bit.peer_ack_flag = 0;
    #endif


    blms_start_common_1(blms_pconn);
    my_dump_str_u32s(DBG_SUBRATE_EN, "btx start", blms_pconn->conn_inst, p, ((((sch_task_t *)p)->next)), 0);

    if (1) {
        u32 tick = bltSche.bSlot_tick_irq_real + IRQ_BTX_DELAY_US * SYSTEM_TIMER_TICK_1US;

        /* debug, make BTX software timeout if no receive any RX packet
         * rx_timeout only BIT<0~11> valid, max value 4095uS, so do not use*/
        //reg_rf_ll_ctrl_1 &= ~FLD_RF_RX_TIMEOUT_EN; //debug

    #ifdef HAL_CHIP_USE_CSEM_MODEM_IP
        /* Must be placed after PHY update processing is complete, because "conn cur_phy" is updated there */
        /* cost more time, can not set after "rf start_fsm" */
        #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
        rf_ble_csem_set_tx_rx_settle(0, tx_stl_btx_1st_pkt[blms_pconn->connPhyCtrl.conn_cur_phy], RX_SETTLE_US);
        #else
        rf_ble_csem_set_tx_rx_settle(0, TX_STL_BTX_1ST_PKT_SET_1M, RX_SETTLE_US);
        #endif
    #endif


        u8 *tx_buff = (u8 *)(blt_m_txfifo.p_base + blms_conn_sel * blt_m_txfifo.conn_full_size);

        rf_start_fsm(FSM_BTX, tx_buff, tick);

    #ifndef HAL_CHIP_USE_CSEM_MODEM_IP
        //Must be placed after phy update processing is complete
        #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
        rf_ble_set_tx_wait(tx_stl_btx_1st_pkt[blms_pconn->connPhyCtrl.conn_cur_phy] - TX_FAST_SETTLE_TIME);
        rf_ble_set_tx_settle(TX_FAST_SETTLE_TIME);
        #else
        rf_ble_set_tx_wait(TX_STL_BTX_1ST_PKT_SET_1M - TX_FAST_SETTLE_TIME);
        rf_ble_set_tx_settle(TX_FAST_SETTLE_TIME);
        #endif
    #endif

        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_TX_ON);
        }


    #if 0 //debug, make BTX fail
            static int test_cnt = 0;
            test_cnt ++;
            if( (test_cnt & 1) == 0){
                rf_set_ble_access_code_value(0x12345678);
            }
    #endif
    }


    //these setting should do after BTX trigger, to speed up BTX timing
    blms_state          = BLMS_STATE_BTX_S;
    systick_irq_trigger = SYS_IRQ_TRIG_BTX_POST;


    blms_start_common_2(blms_pconn);

    //for CIS slave timing build  OR PAST recipient timing calculation
    //Used to accurately obtain the starting anchor point of the data packet corresponding to the current CE
    blms_pconn->ap_tick_mark = bltSche.sSlot_tick_irq + IRQ_BTX_SEND_DELAY_US * SYSTEM_TIMER_TICK_1US;
    //  blms_pconn->conn_inst_mark = blms_pconn->conn_inst; // do this in the above FUNC: blms_start_common_2
#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
    //cs only 1M & 2M
    if(bltPHYs.cur_llPhy == BLE_PHY_2M){
        blms_pconn->csParam.ap_tick_mark = bltSche.bSlot_tick_irq_real + (IRQ_BTX_DELAY_US + CS_TX_STL_BTX_1ST_PKT_REAL_2M)* SYSTEM_TIMER_TICK_1US;
    }
    else{
        blms_pconn->csParam.ap_tick_mark = bltSche.bSlot_tick_irq_real + (IRQ_BTX_DELAY_US + CS_TX_STL_BTX_1ST_PKT_REAL_1M)* SYSTEM_TIMER_TICK_1US;
    }
#endif
    return 1;
}

_attribute_ram_code_ int blt_btx_post(void)
{
    /* must execute before any other operation, cause may return to deal with boundary RX */
    if (blms_post_pre_process() == FALSE) {
        return 1;
    }

    blms_state = BLMS_STATE_BTX_E;
    int result = blms_post_common_1(blms_pconn); // return 1: ACL terminate happens, 2: cis terminate, 0 : no terminate
    if (result == 1) {
    #if (SCAN_EN_MORE_STRATEGY)
        if (!bltScn.scan_en_strategy)
    #endif
        {
            if (blmsParam.cur_master_num == blmsParam.max_master_num && (blmsParam.scanInitEn_union.leg_scan_en || blmsParam.scanInitEn_union.ext_scan_en)) { //ext_scan no need do this
                blt_sche_addTaskMask(TSKMSK_PRICHN_SCAN);
                bltScn.last_scan_end_time = clock_time() | 1;
            }
        }

        blmsParam.cur_master_num--;

        blt_sche_removeTaskMask(TSKMSK_ACL_CONN_0 << blms_conn_sel);
        blt_sche_addUpdate(SLOT_UPDT_CONN_TERMINATE);
        aclMas_param.position_mask[blm_pconn->init_pos_idx] &= ~blm_pconn->init_pos_msk;

    #if (HW_AES_CCM_ALG_EN)

        if (blms_pconn->hw_aes_ccm_flag) {
            blms_pconn->hw_aes_ccm_flag = 0;
            reg_rf_tx_mode2 &= ~FLD_TLK_CRYPT_ENABLE;
        }
    #endif
    } else { //no terminate
        blt_ll_acl_conn_sync_process(blms_pconn->conn_receive_packet);

        blt_llms_update_fifo_sw();

    #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
        blm_pconn->tick_conn_expect = blms_pconn->conn_tick_mark + blms_pconn->conn_intvl_tick;
    #endif

    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)

        if ((blms_pconn->lastSubEventCnt >> 14) == 0x03) {
            blms_pconn->subrate_flag.bit.subrate_wrap_flag = 1;
        }

        //      my_dump_str_u32s(DBG_SUBRATE_EN, "btxPost", blms_pconn->conn_inst-1,blms_pconn->subrate_flag.bit.validDataRxTx_flag,blms_pconn->tx_rptr,blms_pconn->tx_wptr);

        my_dump_str_u32s(DBG_SUBRATE_EN, "btxPost", blms_pconn->conn_inst - 1, blms_pconn->subrate_flag.bit.subrate_trans_mode, bltSche.pTask_next, bltSche.pTask_next->next);


        if ((blms_pconn->subrate_flag.bit.subrate_trans_mode) && (blt_ll_isMarkFifoTxDone(blms_pconn))) {
            blms_pconn->subrate_flag.bit.subrate_trans_mode = 0;


            if (blms_pconn->subrate_flag.bit.subrate_evt_trige || blms_pconn->subrate_flag.bit.subrate_para_change_flag) {
                blms_pconn->subrate_flag.bit.subrate_evt_trige        = 0;
                blms_pconn->subrate_flag.bit.subrate_para_change_flag = 0;

                blms_pconn->subrate_flag.bit.subrate_update_evt = 1;
                blmsParam.subrateUpdtEvt_mask |= (1 << blms_pconn->acl_conIndex);
            }

            blms_pconn->factor           = blms_pconn->factor_next;
            blms_pconn->conti_num        = blms_pconn->conti_num_next;
            blms_pconn->per_latency      = blms_pconn->per_latency_next;
            blms_pconn->conn_timeout     = blms_pconn->subrate_timeout_next * 10 * SYSTEM_TIMER_TICK_1MS;
            blms_pconn->subrateBaseEvent = blms_pconn->subrateBaseEvent_next;
            blms_pconn->insertTsk        = 0;
            blt_sche_addUpdate(SLOT_UPDT_MASTER_SUBRATE_STATE_CHANGE);


        } else if (ll_acl_subrate_irq_task_cb && blms_pconn->conti_num) {
            ll_acl_subrate_irq_task_cb(FLAG_ACL_SUBRATE_INSERT_CONTI_TASK, blms_pconn); //blt_ll_subrate_insertContiTask
        }
    #endif
    }


    blms_post_common_2();

    #if (SL01_aclc_0)
    log_task_end_irq(SL_STACK_ACL_BASIC_TIMING_EN, SL01_aclc_0 + blms_conn_sel);
    #endif


    return 0;
}

_attribute_ram_code_ int blt_ll_buildAclMasterSchedulerLinklist(void)
{
    u32 i, j; // j must be "u32", cause use "bslot_idx"

    st_ll_conn_t  *cur_pAclConn;
    st_llm_conn_t *cur_pAclMaster;

    int intvl_jump_acl;
    u32 bSlot_start_conn;
    int slot_master_num = 0;

    #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
    s32 sSlot_start_conn;
    #endif


    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    u16 inst_start_conn;
    u8  subeventFlag[ACL_MASTER_FIFONUM];
    #endif

    u8 AA_slot_map[SCHE_PRE_ALLOCATE_BSLOT_NUM];
    smemset4((int *)AA_slot_map, 0, SCHE_PRE_ALLOCATE_BSLOT_NUM);
    for (i = ACL_CONN_IDX_CEN0; i < blmsParam.max_master_num; i++) //optimize: use "max master_num" instead of LL_MAX_ACL_CEN_NUM
    {
        if (bltSche.task_mask & (TSKMSK_ACL_CONN_0 << i)) {
            cur_pAclConn                = (st_ll_conn_t *)&blms[i];
            cur_pAclMaster              = (st_llm_conn_t *)&blmsMaster[i];
            cur_pAclMaster->aclTsk_wptr = cur_pAclMaster->aclTsk_rptr = 0;

    #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
            if (ll_acl_sniffer_mst_irq_task_cb) {
                int sSlot_mark_update = 0;

                if (bltSche.build_index == 0) {
                    if (bltSche.sSlot_idx_reset == 1) {
                        cur_pAclMaster->sSlot_mark_conn -= bltSche.sSlot_idx_past;
                    }

                    if (!cur_pAclConn->sync_timing) {
                        if (cur_pAclMaster->sSlot_offset) {
                            cur_pAclMaster->sSlot_mark_conn += cur_pAclMaster->sSlot_offset;
                            cur_pAclMaster->sSlot_offset = 0;
                        }
        #if (BLMS_PM_ENABLE)
                        else {
                            if (cur_pAclConn->pm_error_us || cur_pAclMaster->conn_tolerance_us > blmsParam.min_tolerance_us) {
                                cur_pAclMaster->conn_tolerance_us = cur_pAclConn->pm_error_us + blmsParam.min_tolerance_us;
                                sSlot_mark_update                 = 1;
                            }
                        }
        #endif
                    }
                } else {
        #if (BLMS_PM_ENABLE)
                    if (!cur_pAclConn->sync_timing) {
                        cur_pAclMaster->conn_tolerance_us += blmsParam.min_tolerance_us;
                        sSlot_mark_update = 1;
                    }
        #endif
                }

        #if (BLMS_PM_ENABLE)
                if (sSlot_mark_update) {
                    if (cur_pAclMaster->conn_tolerance_us > cur_pAclMaster->tolerance_max_us) {
                        cur_pAclMaster->conn_tolerance_us = cur_pAclMaster->tolerance_max_us;
                    }

                    s32 sSlot_shift_new = cur_pAclMaster->conn_tolerance_us * SSLOT_US_REVERSE;
                    cur_pAclMaster->sSlot_mark_conn -= (sSlot_shift_new - cur_pAclMaster->sSlot_shift_tor);
                    cur_pAclMaster->sSlot_shift_tor = sSlot_shift_new;

            // tor*2 /sSlot_unit = tor*2 /(625/32) = tor*64/625
            #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
                    cur_pAclConn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[cur_pAclConn->connPhyCtrl.conn_cur_phy - 1][cur_pAclConn->crypt.enable] + cur_pAclMaster->conn_tolerance_us * 64 / 625;
            #else
                    cur_pAclConn->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[0][cur_pAclConn->crypt.enable] + cur_pAclMaster->conn_tolerance_us * 64 / 625;
            #endif
                }
        #endif

                if (cur_pAclMaster->sSlot_mark_conn >= bltSche.sSlot_idx_next) {
                    sSlot_start_conn = cur_pAclMaster->sSlot_mark_conn + cur_pAclMaster->sSlot_interval;
                    intvl_jump_acl   = 0;
                } else {
                    intvl_jump_acl   = (bltSche.sSlot_idx_next - 1 - cur_pAclMaster->sSlot_mark_conn) / cur_pAclMaster->sSlot_interval;
                    sSlot_start_conn = cur_pAclMaster->sSlot_mark_conn + (intvl_jump_acl + 1) * cur_pAclMaster->sSlot_interval;
                }


                if (sSlot_start_conn >= bltSche.sSlot_endIdx_dft) { //to save some time for big interval
                    continue;                                       //attention: can not use break !!!
                }

                u32 scheduler_use_us         = bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US;
                cur_pAclConn->sSlot_sche_use = scheduler_use_us * SSLOT_US_REVERSE;
                cur_pAclConn->sSlot_duration = cur_pAclConn->sSlot_allocNum + cur_pAclConn->sSlot_sche_use;

                int new_task_cnt = 0;
                for (j = 0; j < ACL_MASTER_FIFONUM; j++) {
                    sch_task_t *pCur_schTask = (sch_task_t *)&cur_pAclMaster->aclTsk_fifo[j];

                    pCur_schTask->begin = sSlot_start_conn + j * cur_pAclMaster->sSlot_interval;
                    pCur_schTask->end   = pCur_schTask->begin + cur_pAclConn->sSlot_duration - 1;

                    if (pCur_schTask->begin >= bltSche.sSlot_endIdx_dft) {     //new task beyond correct range, finish
                        break;
                    } else if (pCur_schTask->end < bltSche.sSlot_endIdx_dft) { //new task in correct range
                        cur_pAclMaster->aclTsk_wptr = j;
                        new_task_cnt++;
                    } else {                                                   //new task across "sSlot_endIdx_dft"

                        //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                        if (bltPri.pri_cal[TSKOFT_ACL_CONN + i] > bltPri.priMax_value) {
                            bltPri.priMax_value         = bltPri.pri_cal[TSKOFT_ACL_CONN + i];
                            bltPri.priMax_index         = TSKOFT_ACL_CONN + i;
                            bltSche.sSlot_endIdx_maxPri = pCur_schTask->begin;
                            tlkapi_send_string_u32s(SCHE_TIMING_IMPROVE_DBG_EN, "across IDX master", i, bltSche.sSlot_endIdx_dft, bltSche.sSlot_endIdx_maxPri, bltPri.priMax_value);
                        }

                        break;
                    }
                }

                if (new_task_cnt) {
                    if (cur_pAclConn->connUpt_inst_jump) {
                        //attention: can not clear "connUpt_inst_jump" here, BRX_start will use later
                        intvl_jump_acl += cur_pAclConn->connUpt_inst_jump;
                    }
                    blt_ll_incSchedulerTaskCalPriority(TSKOFT_ACL_CONN + i, bltPri.step_final[TSKOFT_ACL_CONN + i] * 2 * intvl_jump_acl);

                    blt_ll_addTask2ExistLinklist(&cur_pAclMaster->aclTsk_fifo[0], cur_pAclMaster->aclTsk_wptr + 1);
                }
            } else
    #endif
            {
                if (cur_pAclConn->bSlot_mark_conn >= bltSche.bSlot_idx_next) { //bSlot_mark_conn init in "blms_m_connect" may make this happen
                    bSlot_start_conn = cur_pAclConn->bSlot_mark_conn + cur_pAclConn->bSlot_interval;
                    intvl_jump_acl   = 0;
                } else {
                    intvl_jump_acl   = (bltSche.bSlot_idx_next - 1 - cur_pAclConn->bSlot_mark_conn) / cur_pAclConn->bSlot_interval;
                    bSlot_start_conn = cur_pAclConn->bSlot_mark_conn + (intvl_jump_acl + 1) * cur_pAclConn->bSlot_interval;
                }


                if (cur_pAclConn->connUpt_inst_jump) {
                    //attention: can not clear "connUpt_inst_jump" here, BTX_start will use later
                    intvl_jump_acl += cur_pAclConn->connUpt_inst_jump;
                }
                blt_ll_incSchedulerTaskCalPriority(TSKOFT_ACL_CONN + i, bltPri.step_final[TSKOFT_ACL_CONN + i] * 2 * intvl_jump_acl);

    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
                u16 inst_jump   = 0;
                inst_start_conn = cur_pAclConn->conn_inst + intvl_jump_acl; // next conn_inst
                my_dump_str_u32s(DBG_SUBRATE_EN, "jump", inst_start_conn, intvl_jump_acl, cur_pAclConn->conn_inst, cur_pAclConn->insertTsk);


                if ((cur_pAclConn->factor > 1) && (cur_pAclConn->insertTsk)) {
                    inst_jump = (u16)(inst_start_conn - cur_pAclConn->noDataEvtStart);

                    if (inst_jump < cur_pAclConn->conti_num) {
                        cur_pAclConn->insertTsk -= intvl_jump_acl;

                        my_dump_str_u32s(DBG_SUBRATE_EN, "inst_jump < cur_pAclConn->insertTsk", cur_pAclConn->insertTsk, inst_jump, 0, 0);
                    } else {
                        cur_pAclConn->insertTsk = 0;
                    }
                }
    #endif


                /* SiHui: consider update a new task add, so add some more time. here update may represent a task remove, neglect this
                 * give another margin here */
                cur_pAclConn->sSlot_allocNum = (IRQ_BTX_SEND_DELAY_US + cur_pAclConn->pdu_task_us + 20) * SSLOT_US_REVERSE;

    #if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
                // todo The acl post time is longer because the channel sounding is judged in the post
                u32 scheduler_use_us = bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US + 100;
    #else
                u32 scheduler_use_us = bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US;
    #endif

                cur_pAclConn->sSlot_sche_use = scheduler_use_us * SSLOT_US_REVERSE;
                cur_pAclConn->sSlot_duration = cur_pAclConn->sSlot_allocNum + cur_pAclConn->sSlot_sche_use + ACL_CMD_DONE_MANUAL_TRIGGER_STIMER_DELAY_US * SSLOT_US_REVERSE;

    #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER)
                cur_pAclConn->actual_txrx_sche_us = cur_pAclConn->pdu_task_us + scheduler_use_us;
    #endif


                int bSlot_duration = (cur_pAclConn->sSlot_duration + 31) >> 5; //">>5" = "/32"


    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)

                u32 inst                      = inst_start_conn - 1;
                cur_pAclConn->lastSubEventCnt = inst;

        #if (SCH_DEBUG_EN)
                u32 jumpLoop = 0;
        #endif


                while (1) {
                    u16               baseEventBak     = cur_pAclConn->subrateBaseEvent;
                    u16               baseEventNextBak = cur_pAclConn->subrateBaseEvent_next;
                    u16               insertTaskBak    = cur_pAclConn->insertTsk;
                    ll_subrate_flag_t subrate_flag_bak = cur_pAclConn->subrate_flag;

        #if (SCH_DEBUG_EN)
                    jumpLoop++;
                    if (jumpLoop > SCHE_PRE_ALLOCATE_BSLOT_NUM / 12) { //12 = 7.5ms/0.625ms
                        BLMS_ERR_DEBUG(SCH_DEBUG_EN, 0xFF140000);
                    }
        #endif

                    my_dump_str_u32s(DBG_SUBRATE_EN, "in", inst, cur_pAclConn->subrate_flag.bit.subrate_trans_mode, cur_pAclConn->factor, cur_pAclConn->subrate_flag.bit.conn_update_flag);
                    inst = blt_ll_subrate_getNextEvent(cur_pAclConn, inst);
                    j    = bSlot_start_conn + ((u16)((inst & 0xffff) - inst_start_conn)) * cur_pAclConn->bSlot_interval;


                    my_dump_str_u32s(DBG_SUBRATE_EN, "inst", inst, bltSche.bSlot_endIdx_dft, cur_pAclConn->subrateBaseEvent, j);


                    if (j < (bltSche.bSlot_endIdx_dft - bSlot_duration + 1)) {
                        subeventFlag[cur_pAclMaster->aclTsk_wptr] = inst & BIT(31) ? 1 : 0;
                        //DBG_C HN12_TOGGLE;
                        cur_pAclConn->lastSubEventCnt = inst & 0xffff;
                        if (cur_pAclConn->subrate_flag.bit.conn_update_flag &&
                            (cur_pAclConn->lastSubEventCnt == (cur_pAclConn->conn_para_inst_next - cur_pAclConn->subrate_flag.bit.conn_update_flag + 1))) {
                            cur_pAclConn->subrate_flag.bit.conn_update_flag--;
                        }


                        AA_slot_map[j - bltSche.bSlot_idx_next] = 0x80 | i << 4 | cur_pAclMaster->aclTsk_wptr;
                        my_dump_str_u32s(DBG_SUBRATE_EN, "s", inst, j, inst_start_conn, j - bltSche.bSlot_idx_next);

                        my_dump_str_u32s(ACL_MASTER_SCHE_DEBUG, "master", i, j - bltSche.bSlot_idx_next, AA_slot_map[j - bltSche.bSlot_idx_next], cur_pAclMaster->aclTsk_wptr);
                        cur_pAclMaster->aclTsk_wptr++;
                        blt_ll_incSchedulerTaskCalPriority(TSKOFT_ACL_CONN + i, -(bltPri.step_final[TSKOFT_ACL_CONN + i]));

                        //                      my_dump_str_u32s(DBG_SUBRATE_EN, "inst success", inst, cur_pAclConn->insertTsk, 0,0);


                        slot_master_num++;
                    } else // if((cur_pAclConn->factor>1) || (cur_pAclConn->factor_next>1))// just factor>1 need backup this variate
                    {
                        cur_pAclConn->subrate_flag = subrate_flag_bak;

                        cur_pAclConn->subrateBaseEvent      = baseEventBak;
                        cur_pAclConn->subrateBaseEvent_next = baseEventNextBak;
                        cur_pAclConn->insertTsk             = insertTaskBak;

                        break;
                    }
                }
    #else
                for (j = bSlot_start_conn; j < (bltSche.bSlot_endIdx_dft - bSlot_duration + 1); j += cur_pAclConn->bSlot_interval) {
                    AA_slot_map[j - bltSche.bSlot_idx_next] = 0x80 | i << 4 | cur_pAclMaster->aclTsk_wptr;
                    my_dump_str_u32s(ACL_MASTER_SCHE_DEBUG, "master", i, j - bltSche.bSlot_idx_next, AA_slot_map[j - bltSche.bSlot_idx_next], cur_pAclMaster->aclTsk_wptr);
                    cur_pAclMaster->aclTsk_wptr++;
                    blt_ll_incSchedulerTaskCalPriority(TSKOFT_ACL_CONN + i, -(bltPri.step_final[TSKOFT_ACL_CONN + i]));

        #if 0 //if ACL_MASTER_FIFONUM big enough, can jump this judge to save RamCode
                        if(cur_pAclMaster->aclTsk_wptr >= ACL_MASTER_FIFONUM){
                            break;
                        }
        #endif

                    slot_master_num++;
                }
    #endif


                //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                if (j < bltSche.bSlot_endIdx_dft) { //task across "bSlot_endIdx_dft"  //j > (bltSche.bSlot_endIdx_dft - bSlot_duration)
                    if (bltPri.pri_cal[TSKOFT_ACL_CONN + i] > bltPri.priMax_value) {
                        bltPri.priMax_value         = bltPri.pri_cal[TSKOFT_ACL_CONN + i];
                        bltPri.priMax_index         = TSKOFT_ACL_CONN + i;
                        bltSche.sSlot_endIdx_maxPri = bltSche.sSlot_endIdx_dft - (bltSche.bSlot_endIdx_dft - j) * 32;
                        my_dump_str_u32s(SCHE_TIMING_IMPROVE_DBG_EN, "across IDX master", i, bltSche.sSlot_endIdx_dft, bltSche.sSlot_endIdx_maxPri, bltPri.priMax_value);
                    }
                }
            }
        }
    }

    #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
    if (ll_acl_sniffer_mst_irq_task_cb) {
        return slot_master_num;
    }
    #endif

    int first_task = 1;

    int master_task_number = slot_master_num;
    if (slot_master_num) {
        //my_dump_str_u32s(DBG_SUBRATE_EN, "master_task_number", master_task_number, cur_pAclConn->insertTsk,0,0);
        /*The absolute value on the time axis corresponding to Task->begin:
        sSlot_tick_start + (Task->begin*625)/2, sSlot_idx_base is the relative value */
        u32 sSlot_idx_base = (bltSche.bSlot_idx_next - bltSche.bSlot_idx_start) * 32;

        sch_task_t *pCur_schTask = NULL;
        sch_task_t *pPre_schTask = NULL;

        for (i = 0; i < bltSche.bSlot_maxLen; i++) {
            if (AA_slot_map[i]) {
                u8 wptr        = AA_slot_map[i] & 0x0F;
                u8 conn_idx    = (AA_slot_map[i] & 0x70) >> 4;
                cur_pAclConn   = (st_ll_conn_t *)&blms[conn_idx];
                cur_pAclMaster = (st_llm_conn_t *)&blmsMaster[conn_idx];


                pCur_schTask = (sch_task_t *)&cur_pAclMaster->aclTsk_fifo[wptr];


                pCur_schTask->begin = sSlot_idx_base + i * 32;

    #if (BLE_LLMIC_CONCURRENT_EN)
                int acl_extend_sslot = blt_llmic_getSslotGapValue();
                pCur_schTask->end    = pCur_schTask->begin + cur_pAclConn->sSlot_duration + acl_extend_sslot - 1;
    #else
                pCur_schTask->end = pCur_schTask->begin + cur_pAclConn->sSlot_duration - 1;
    #endif

                pCur_schTask->cover_other = 0;

    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
                pCur_schTask->subrate_evt_flag = subeventFlag[wptr];
    #endif


                if (first_task) {
    #if BLE_LLMIC_CONCURRENT_EN
                    // If the current task starts is less than 5m from the task axis point.
                    int acl_extend_sslot = blt_llmic_getSslotGapValue();
                    if (acl_extend_sslot && (pCur_schTask->begin >= (bltSche.sSlot_idx_next + acl_extend_sslot))) {
                        first_task               = 0;
                        bltSche.pTask_head->next = pCur_schTask;
                    } else if (!acl_extend_sslot) {
                        first_task               = 0;
                        bltSche.pTask_head->next = pCur_schTask;
                    }
    #else
                    first_task               = 0;
                    bltSche.pTask_head->next = pCur_schTask;
    #endif
                } else {
                    pPre_schTask->next = pCur_schTask;
                }
                pPre_schTask = pCur_schTask;


                my_dump_str_u32s(ACL_MASTER_SCHE_DEBUG, "link", i, pCur_schTask->begin, pCur_schTask, aclMas_param.bSlotNum_whole_inter);

                slot_master_num--;
                if (slot_master_num == 0) {
                    pCur_schTask->next = NULL;
                    break;
                }


                int bSlot_jump;
                if (blmsParam.cur_master_num == 1) {
                    bSlot_jump = aclMas_param.bSlotNum_whole_inter; //speed up
                } else {
    #if (ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN)
                    if (aclMas_param.timCus_en) {
                        if (cur_pAclMaster->init_pos_idx == aclMas_param.timposn_max) {
                            bSlot_jump = aclMas_param.bSlotNum_whole_inter - aclMas_param.bSlot_offset_pos0[cur_pAclMaster->init_pos_idx];
                        } else {
                            bSlot_jump = aclMas_param.bSlot_number[cur_pAclMaster->init_pos_idx];
                        }
                    } else
    #endif
                    {
                        bSlot_jump = aclMas_param.bSlotNum_piece_inter;
                    }
                }

                i += (bSlot_jump - 1);
            }
        }
    }


    return master_task_number;
}

bool blt_ll_pushAclChClassUpdPkt(u16 connHandle, u8 *pChm)
{
    st_ll_conn_t            *pAclConn = (st_ll_conn_t *)&blms[connHandle & CONN_IDX_MASK];
    u8                       pkt_conn_map_update[sizeof(rf_packet_chm_upd_req_t)];
    rf_packet_chm_upd_req_t *pChnMapReq = (rf_packet_chm_upd_req_t *)pkt_conn_map_update;

    //check if exist ACL channel map update pending process
    if (pAclConn->conn_update_union.update_mark & CONN_UPDATE_MAP) {
        return FALSE;
    }

    pChnMapReq->type   = LLID_CONTROL;
    pChnMapReq->rf_len = 8;
    pChnMapReq->opcode = LL_CHANNEL_MAP_REQ;
    smemcpy(pChnMapReq->chm, pChm, 5);

    #if BQB_TEST_EN
    pChnMapReq->instant = pAclConn->conn_inst + pAclConn->conn_latency + 40;
    #else
    pChnMapReq->instant = pAclConn->conn_inst + pAclConn->conn_latency + 10;
    #endif

    bool status = ll_push_tx_fifo_handler(connHandle | HANDLE_STK_FLAG, pkt_conn_map_update); /// blt_llms_pushTxfifo

    if (status) {
        pAclConn->conn_map_inst_next = pChnMapReq->instant;
        pAclConn->conn_update_union.update_mark |= CONN_UPDATE_MAP;

        smemcpy(pAclConn->nextChn.chmTbl, pChm, 5);

    #if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
        if (pAclConn->conn_chnsel) {
            csa2_calculateMapInfo(&pAclConn->nextChn);
        } else
    #endif
        {
            /* when calculate new channel map in BRX/BTX start, 70uS is used, so calculate table in advance */
            blt_csa1_calculateChannelTable(pChm, pAclConn->conn_chn_hop, pAclConn->nextChn.rempChmTbl);
        }


    #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER)
        if (ll_cis_map_update_cb && pAclConn->cisEstablish_msk) {
            u32 r            = irq_disable();
            u32 trigger_tick = pAclConn->ap_tick_mark + (pAclConn->conn_map_inst_next - pAclConn->conn_inst_mark) * pAclConn->conn_intvl_tick;
            irq_restore(r);
            ll_cis_map_update_cb(trigger_tick, pAclConn); // blt_cis_update_chn_map
        }
    #endif
    }

    return status;
}

int blt_ll_ctrlAclMstChClassUpd(unsigned char *pChm)
{
    u16 connHandle;
    for (int conn_idx = 0; conn_idx < LL_MAX_ACL_CEN_NUM; conn_idx++) {
        st_ll_conn_t *pAcl = (st_ll_conn_t *)&blms[conn_idx];
        connHandle         = BLM_CONN_HANDLE | conn_idx;          //Master

        if (pAcl->connState == CONN_STATUS_ESTABLISH) {
            if (!blt_ll_pushAclChClassUpdPkt(connHandle, pChm)) { //Ll push pkt Failed
                blmhostChnClassUpt.hostMapUptCmdPending |= BIT(conn_idx);
            }
            pAcl->ll_upd_flag |= CHN_MAP_FLAG;
        }
    }

    return 1;
}


    #if (ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN)


ble_sts_t blc_ll_setAclCentralTaskTimingArrangement(aclc_arng_en_t enable, aclc_slotDur_mod_t sd_mode, u8 slotDurNum_sameMode)
{
    aclMas_param.timCus_en = enable;
    if (enable) {
        aclMas_param.bSlotDurn_diffMod = sd_mode == ACLC_SLOT_DURN_DIFF;


        if (!aclMas_param.bSlotDurn_diffMod) { //same mode
            if (slotDurNum_sameMode > 1) {
                aclMas_param.bSlotNum_sameMod = slotDurNum_sameMode;
                aclMas_param.timposn_max      = blmsParam.max_master_num - 1;
            } else {
                return LL_ERR_INVALID_PARAMETER;
            }
        } else {
            aclMas_param.bSlotNum_sameMod = 0;
        }
    } else {
        aclMas_param.bSlotDurn_diffMod = 0;
    }


    return BLE_SUCCESS;
}

ble_sts_t blc_ll_setAclCentralTimingPositionSlotNumber_for_diffMode(int acl_cen_index, timposn_idx_t time_position_index, u8 slotDur_num)
{
    if (acl_cen_index < blmsParam.max_master_num && time_position_index < blmsParam.max_master_num) {
        aclMas_param.timposn_idx[acl_cen_index]        = time_position_index;
        aclMas_param.bSlot_number[time_position_index] = slotDur_num;

        if (time_position_index > aclMas_param.timposn_max) {
            aclMas_param.timposn_max = time_position_index;
        }
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    return BLE_SUCCESS;
}

ble_sts_t blc_ll_setAclCentralIndex_arrangedTaskTiming_diffMode(u8 cur_aclcen_idx)
{
    if (aclMas_param.bSlotDurn_diffMod && cur_aclcen_idx < blmsParam.max_master_num) {
        bltInit.aclc_idx_init = cur_aclcen_idx;

        my_dump_str_data(DBG_CUSTOM_ACLC_TIMING, "[CUS CEN] user set create acl cen index", &bltInit.aclc_idx_init, 1);
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    return BLE_SUCCESS;
}

u8 blc_ll_getAclCentralIndex_arrangedTaskTiming_diffMode(u16 acl_handle)
{
    if ((acl_handle >= BLM_HANDLE_MIN) && (acl_handle < (BLM_CONN_HANDLE + blmsParam.max_master_num))) {
        u8 conn_idx = acl_handle & CONN_IDX_MASK;
        return conn_idx;
    } else {
        return 0xFF;
    }
}

    #endif //end of ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN


st_llm_conn_t *blt_ll_isValidAclCentralHandle(u16 connHandle)
{
    st_llm_conn_t *pAclCen;

    if ((connHandle >= BLM_HANDLE_MIN) && (connHandle < (BLM_CONN_HANDLE + blmsParam.max_master_num))) {
        u8 conn_idx = connHandle & CONN_IDX_MASK;
        if (blms[conn_idx].connState) {
            pAclCen = (st_llm_conn_t *)&blmsMaster[conn_idx];

            return pAclCen;
        }
    }

    return NULL;
}

int blt_ll_calculate_peripheral_latency(int conn_interval, int conn_supervision_timeout, st_ll_conn_t *pc)
{
    (void)pc; //unused, remove warning

    /*
     * conn_interval: unit 1.25ms
     * conn_subrate_factor: unit 1
     * conn_supervision_timeout: unit 10ms
     * conn_peripheral_latency: unit 1
     *blt_ll_calculate_peripheral_latency
     * conn_supervision_timeout > (1 + conn_peripheral_latency) × conn_subrate_factor × conn_interval × 2
     * conn_interval conver from 1.25ms to 10ms: interval ÷ 8
     * conn_supervision_timeout > (1 + conn_peripheral_latency) × conn_subrate_factor × (conn_interval ÷ 8) × 2
     * conn_supervision_timeout > (1 + conn_peripheral_latency) × conn_subrate_factor × (conn_interval ÷ 4)
     * Multiply both sides by 4 to avoid division: 4 * conn_supervision_timeout > (1 + conn_peripheral_latency) × conn_subrate_factor × conn_interval
     */
    int left_side = conn_supervision_timeout * 4;
    int right_side;
    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
        right_side = pc->factor * conn_interval;
    #else
        right_side = conn_interval;
    #endif

    int quotient = left_side / right_side;
    int remainder = left_side % right_side;

    if (remainder < (right_side >> 1)) {
        quotient--;
    }

    int conn_peripheral_latency = quotient - 1;

    if (conn_peripheral_latency < 0) {
        conn_peripheral_latency = 0;
    }

    return conn_peripheral_latency;
}

ble_sts_t blc_ll_setAclCentralBaseConnectionInterval(conn_inter_t conn_interval)
{
    aclMas_param.master_connInter = conn_interval;

    /* update master bSlot number when master_connInter or max_master_num changes */
    if (aclMas_param.master_connInter && blmsParam.max_master_num) {
        blt_ll_calculateAclMasterBslotNumber();
    }

    return BLE_SUCCESS;
}

void blt_ll_calculateAclMasterBslotNumber(void)
{
    /**************************************************************************************************
 * interval         bSlotNum_whole_inter                              bSlotNum_piece_inter
 *     6 (   7.5mS)         12                      6                       3
 *    16 (   20 mS)         32                      16                      8 ( 5.0   mS)
 *    17 (21.25 mS)         34                      17                      8 ( 5.0   mS)
 *    18 (22.50 mS)         36                      18                      9 ( 5.625 mS)
 *    19 (23.75 mS)         38                      19                      9 ( 5.625 mS)
 *
 *************************************************************************************************/

    aclMas_param.bSlotNum_whole_inter = aclMas_param.master_connInter << 1;
    aclMas_param.bSlotNum_piece_inter = aclMas_param.bSlotNum_whole_inter / blmsParam.max_master_num;
}

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #endif
    ble_sts_t
    blc_ll_updateConnection(u16 connHandle, conn_inter_t conn_min, conn_inter_t conn_max, u16 conn_latency, conn_tm_t timeout, u16 ce_min, u16 ce_max)
{
    (void)ce_min; //unused, remove warning
    (void)ce_max; //unused, remove warning
    st_llm_conn_t *pm = blt_ll_isValidAclCentralHandle(connHandle);
    if (!pm) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if (conn_min > conn_max) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    u8            conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t *pc       = (st_ll_conn_t *)&blms[conn_idx];

    if (pc->ll_enc_busy) {
        return LL_ERR_ENCRYPTION_BUSY;
    }
    /* can not update connection when initiating:
     *                                            mas_intv_mul & mas_intv_msk can used in initiating and
     *
     * */
    if ((pc->conn_update_union.update_mark & CONN_UPDATE_PARAM_MASK) || blmsParam.create_connection || blmsParam.new_conn_forbidden || blmsParam.newConn_forbidden_master) {
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
        //      return 0xFE; //debug
    }


    bltInit.mas_intv_mul = blt_init_calculateMasterIntervalMultiplier(aclMas_param.master_connInter, conn_min, conn_max);
    if (bltInit.mas_intv_mul == 24) {
        bltInit.mas_intv_msk = INTV_MSK_24_TIME;
    } else {
        bltInit.mas_intv_msk = interMask_tbl[bltInit.mas_intv_mul];
    }


    #if (IMPROVE_MASTER_INTERVAL)
    u16 conn_inter_use = aclMas_param.master_connInter * bltInit.mas_intv_mul;
    if (conn_min <= conn_inter_use && conn_max >= conn_inter_use) { //totally meet host's requirement
        //do nothing
    } else { //not exactly host's requirement
        if (conn_min > aclMas_param.master_connInter) {
            int mod        = conn_max % aclMas_param.master_connInter;
            u16 conn_inter = conn_max - mod;
            if (conn_inter >= conn_min) {
                bltInit.mas_intv_mul = conn_inter / aclMas_param.master_connInter;
                bltInit.mas_intv_msk = 0xFFFFFF;
            }
        }
    }
    #endif


    u8 pkt_conn_para_update[16] = {
        LLID_CONTROL,             //llid
        12,                       //rf_len
        LL_CONNECTION_UPDATE_REQ, //opcode
        BLMS_WINSIZE,             //winsize
    };

    rf_packet_connect_upd_req_t *pUpdate = (rf_packet_connect_upd_req_t *)pkt_conn_para_update;

    #if (LL_ASYNC_LEA_EN)
    if (asyncCtrl.aclIntervalTick && asyncCtrl.connState) {
        pUpdate->interval = asyncCtrl.aclIntervalTick / SYSTEM_TIMER_TICK_1250US;
    } else {
        pUpdate->interval = aclMas_param.master_connInter * bltInit.mas_intv_mul;
    }
    #else
    pUpdate->interval = aclMas_param.master_connInter * bltInit.mas_intv_mul;
    #endif


    #if 0
    if(pUpdate->interval == pc->conn_intvl_n_1m25 && conn_latency == pc->conn_latency){ //same parameters, no need update
//      return 0xFD; //debug
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }
    #endif

    /* pUpdate->winOffset/pUpdate->instant/pc->bSlot_oft_num_next will set in follow function
     * premise: if calculate timing correct in this function, update must be done
     * cause: some critical data will change in this function ,which means later update timing will take effect */
    if (blt_ll_calConnUpdateTiming(conn_idx, pc, pm, pUpdate) == TRUE) {
        u16 latency_max =  blt_ll_calculate_peripheral_latency(pUpdate->interval, timeout, pc);
        if (conn_latency > latency_max) {
            conn_latency = latency_max;
        }

        pUpdate->latency = conn_latency;
        pUpdate->timeout = timeout;

        /* with "HANDLE_STK_FLAG", this data push will be success */
        if (ll_push_tx_fifo_handler(connHandle | HANDLE_STK_FLAG, pkt_conn_para_update)) { //push TX FIFO must success blt_llms_pushTxfifo
            //pc->conn_winsize_next = pUpdate->winSize; //master no need winSize
            pc->conn_para_inst_next = pUpdate->instant; // pUpdate->instant
            //pc->conn_offset_next = pUpdate->winOffset; // for master: bSlot_oft_num_next is used to instead of conn_offset_next
            pc->conn_intvl_next_n_1m25 = pUpdate->interval;
            pc->conn_latency_next      = pUpdate->latency;
            pc->conn_timeout_next      = pUpdate->timeout;

            pc->conn_update_union.update_mark |= CONN_UPDATE_CMD | CONN_UPDATE_EVT; //set flag at last is more safer, consider IRQ problem
            blmsParam.newConn_forbidden_master = 1;                                 //handle some special case: IRQ do not come, but another conn_ update_cmd is coming.

            pc->ll_upd_flag |= CONN_UPD_FLAG;

            //          my_dump_str_u32s(DBG_SUBRATE_EN, "Send conn_update####", pc->conn_inst, pUpdate->instant,0,0);

    #if (DBG_MASTER_CONN_UPDATE)
            if (conn_idx == 0) {
                DBG_C HN8_TOGGLE;
            } else if (conn_idx == 1) {
                DBG_C HN9_TOGGLE;
            } else if (conn_idx == 2) {
                DBG_C HN10_TOGGLE;
            } else if (conn_idx == 3) {
                DBG_C HN11_TOGGLE;
            }
    #endif

        } else {
            //BLMS_ERR_DEBUG(1, 0xFF010000);
        }

        return BLE_SUCCESS;
    } else {
        BLMS_ERR_DEBUG(DBG_MASTER_CONN_UPDATE, 0xCC080000);
    }


    return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    //  return 0xFF; //debug
}


    #if (CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE)
void blc_ll_customizeConnectionEstablishEvent(int enable)
{
    aclConn_etbsh.cusConnEtbsh_en = enable;
}
    #endif


#else  //else of LL_ACL_CEN_EN

void blc_ll_initAclCentralRole_module(void)
{
}

ble_sts_t blc_ll_initAclCentralTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number, int conn_number)
{
    (void)pTxbuf;
    (void)fifo_size;
    (void)fifo_number;
    (void)conn_number;

    return HCI_ERR_CMD_DISALLOWED;
}

ble_sts_t blc_ll_setAclCentralBaseConnectionInterval(conn_inter_t conn_interval)
{
    (void)conn_interval;

    return HCI_ERR_CMD_DISALLOWED;
}

ble_sts_t blc_ll_updateConnection(u16 connHandle, conn_inter_t conn_min, conn_inter_t conn_max, u16 conn_latency, conn_tm_t timeout, u16 ce_min, u16 ce_max)
{
    (void)connHandle;
    (void)conn_min;
    (void)conn_max;
    (void)conn_latency;
    (void)timeout;
    (void)ce_min;
    (void)ce_max;

    return HCI_ERR_CMD_DISALLOWED;
}


#endif //end of LL_ACL_CEN_EN
