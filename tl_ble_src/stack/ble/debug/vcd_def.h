/********************************************************************************************************
 * @file    vcd_def.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#ifndef VCD_DEF_H_
#define VCD_DEF_H_


#include "debug_cfg.h"

#if (VCD_DEFINE_SELECT == VCD_DEFINE_DEFAULT)


    //log_event, ID: 0~31, 0 is reserved for timeStamp, 1 ~ 31 is available
    #define SLEV_timestamp    0 // SLEV 0, reserved, do not change it
    #define SLEV_irq_rx       1
    #define SLEV_irq_tx       2
    #define SLEV_irq_rfdone   3
    #define SLEV_irq_stimer   4

    #define SLEV_sche_rebuild 7
    #define SLEV_sche_slotRst 8

    #define SLEV_acl_rx       10


    //log_tick, ID: 0~31, 0 is reserved for timeStamp, 1 ~ 31 is available
    #define SLET_timestamp 0 // SLET 0, reserved, do not change it


    //log_task, ID: 0~31, id0 is reserved,  1 ~ 31 is available
    // 1-bit data:
    #define SL01_rsvd       0
    #define SL01_IRQ        1

    #define SL01_scn_prichn 2
    #define SL01_leg_adv    3

    #define SL01_acl_0      4
    #define SL01_acl_1      5
    #define SL01_acl_2      6
    #define SL01_acl_3      7
    #define SL01_acl_4      8
    #define SL01_acl_5      9
    #define SL01_acl_6      10
    #define SL01_acl_7      11


    // 8-bit data: cid0 - cid63
    #define SL08_rsvd 0


    // 16-bit data: sid0 - sid63
    #define SL16_rsvd 0


#endif //end of "VCD_DEFINE SELECT"

#ifndef SLET_upt_cmd_1
    #define SLET_upt_cmd_1 0
#endif

#ifndef SLET_upt_cmd_2
    #define SLET_upt_cmd_2 0
#endif

#ifndef SLET_upt_cmd_3
    #define SLET_upt_cmd_3 0
#endif

#ifndef SLET_upt_cmd_4
    #define SLET_upt_cmd_4 0
#endif

#ifndef SLET_upt_sync_1
    #define SLET_upt_sync_1 0
#endif

#ifndef SLET_upt_sync_2
    #define SLET_upt_sync_2 0
#endif

#ifndef SLET_upt_sync_3
    #define SLET_upt_sync_3 0
#endif

#ifndef SLET_upt_sync_4
    #define SLET_upt_sync_4 0
#endif

#ifndef SLET_05_rx_crc
    #define SLET_05_rx_crc 0
#endif

#ifndef SLET_06_rx_1st
    #define SLET_06_rx_1st 0
#endif

#ifndef SLET_07_rx_new
    #define SLET_07_rx_new 0
#endif

#ifndef SLET_10_tx
    #define SLET_10_tx 0
#endif

#ifndef SLET_11_c_cmdDone
    #define SLET_11_c_cmdDone 0
#endif

#ifndef SLET_12_c_1stRxTmt
    #define SLET_12_c_1stRxTmt 0
#endif

#ifndef SLET_13_c_rxTmt
    #define SLET_13_c_rxTmt 0
#endif

#ifndef SLET_14_c_rxCrc2
    #define SLET_14_c_rxCrc2 0
#endif

#ifndef SLEV_txFifo_push
    #define SLEV_txFifo_push 0
#endif

#ifndef SLEV_txFifo_empty
    #define SLEV_txFifo_empty 0
#endif

#ifndef SLEV_txFifo_RX
    #define SLEV_txFifo_RX 0
#endif

#ifndef SLEV_txFifo_post
    #define SLEV_txFifo_post 0
#endif

#ifndef SLEV_test_event
    #define SLEV_test_event 0
#endif

#ifndef SL16_tf_hw_push
    #define SL16_tf_hw_push 0
#endif

#ifndef SL16_tf_sw_push
    #define SL16_tf_sw_push 0
#endif

#ifndef SL16_tf_hw_load1
    #define SL16_tf_hw_load1 0
#endif

#ifndef SL16_tf_sw_load1
    #define SL16_tf_sw_load1 0
#endif

#ifndef SL16_tf_hw_load2
    #define SL16_tf_hw_load2 0
#endif

#ifndef SL16_tf_sw_load2
    #define SL16_tf_sw_load2 0
#endif

#ifndef SL16_tf_hw_RX
    #define SL16_tf_hw_RX 0
#endif

#ifndef SL16_tf_sw_RX
    #define SL16_tf_sw_RX 0
#endif

#ifndef SL16_tf_hw_TX
    #define SL16_tf_hw_TX 0
#endif

#ifndef SL16_tf_sw_TX
    #define SL16_tf_sw_TX 0
#endif

#ifndef SL16_tf_hw_post
    #define SL16_tf_hw_post 0
#endif

#ifndef SL16_tf_sw_post
    #define SL16_tf_sw_post 0
#endif

#ifndef SL16_tf_save
    #define SL16_tf_save 0
#endif

#ifndef SL16_seq_notify
    #define SL16_seq_notify 0
#endif

#ifndef SL16_seq_write
    #define SL16_seq_write 0
#endif

#ifndef SL16_test_2B
    #define SL16_test_2B 0
#endif

#ifndef SLEV_timestamp
    #define SLEV_timestamp 0
#endif

#ifndef SLEV_irq_rx
    #define SLEV_irq_rx 0
#endif

#ifndef SLEV_irq_tx
    #define SLEV_irq_tx 0
#endif

#ifndef SLEV_irq_rfdone
    #define SLEV_irq_rfdone 0
#endif

#ifndef SLEV_irq_stimer
    #define SLEV_irq_stimer 0
#endif

#ifndef SLEV_sche_rebuild
    #define SLEV_sche_rebuild 0
#endif

#ifndef SLEV_sche_slotRst
    #define SLEV_sche_slotRst 0
#endif

#ifndef SLEV_acl_rx
    #define SLEV_acl_rx 0
#endif

#ifndef SLET_timestamp
    #define SLET_timestamp 0
#endif

#ifndef SL01_rsvd
    #define SL01_rsvd 0
#endif

#ifndef SL01_IRQ
    #define SL01_IRQ 0
#endif

#ifndef SL01_scn_prichn
    #define SL01_scn_prichn 0
#endif

#ifndef SL01_leg_adv
    #define SL01_leg_adv 0
#endif

#ifndef SL01_acl_0
    #define SL01_acl_0 0
#endif

#ifndef SL01_acl_1
    #define SL01_acl_1 0
#endif

#ifndef SL01_acl_2
    #define SL01_acl_2 0
#endif

#ifndef SL01_acl_3
    #define SL01_acl_3 0
#endif

#ifndef SL01_acl_4
    #define SL01_acl_4 0
#endif

#ifndef SL01_acl_5
    #define SL01_acl_5 0
#endif

#ifndef SL01_acl_6
    #define SL01_acl_6 0
#endif

#ifndef SL01_acl_7
    #define SL01_acl_7 0
#endif

#ifndef SL08_rsvd
    #define SL08_rsvd 0
#endif

#ifndef SL16_rsvd
    #define SL16_rsvd 0
#endif

#ifndef SLEV_eadv_ext_build
    #define SLEV_eadv_ext_build 0
#endif

#ifndef SLEV_pda_adv_build
    #define SLEV_pda_adv_build 0
#endif

#ifndef SLEV_eadv_aux_build
    #define SLEV_eadv_aux_build 0
#endif

#ifndef SL01_ext_adv
    #define SL01_ext_adv 0
#endif

#ifndef SL01_eadv_ext_ind
    #define SL01_eadv_ext_ind 0
#endif

#ifndef SL01_eadv_auxOrChain_ind
    #define SL01_eadv_auxOrChain_ind 0
#endif

#ifndef SL01_big_bcst
    #define SL01_big_bcst 0
#endif

#ifndef SL01_bis_bcst
    #define SL01_bis_bcst 0
#endif

#ifndef SL08_reserved
    #define SL08_reserved 0
#endif

#ifndef SL16_reserved
    #define SL16_reserved 0
#endif

#ifndef SLEV_eadv_aux_insert
    #define SLEV_eadv_aux_insert 0
#endif

#ifndef SLEV_bigBcst_build
    #define SLEV_bigBcst_build 0
#endif

#ifndef SLEV_bigScan_build
    #define SLEV_bigScan_build 0
#endif

#ifndef SLEV_pdaScan_build
    #define SLEV_pdaScan_build 0
#endif

#ifndef SLEV_auxscanFutrTsk_add
    #define SLEV_auxscanFutrTsk_add 0
#endif

#ifndef SLEV_primary_rx_extAdv
    #define SLEV_primary_rx_extAdv 0
#endif

#ifndef SLEV_primary_rx_legAdv
    #define SLEV_primary_rx_legAdv 0
#endif

#ifndef SLEV_second_rx_adv
    #define SLEV_second_rx_adv 0
#endif

#ifndef SLEV_bsync_rev
    #define SLEV_bsync_rev 0
#endif

#ifndef SLEV_bis0_rx_padding_pldNum
    #define SLEV_bis0_rx_padding_pldNum 0
#endif

#ifndef SLEV_bis1_rx_padding_pldNum
    #define SLEV_bis1_rx_padding_pldNum 0
#endif

#ifndef SLEV_bis0_rx_sdu_cmplt
    #define SLEV_bis0_rx_sdu_cmplt 0
#endif

#ifndef SLEV_bis1_rx_sdu_cmplt
    #define SLEV_bis1_rx_sdu_cmplt 0
#endif

#ifndef SLEV_bis0_rx_len
    #define SLEV_bis0_rx_len 0
#endif

#ifndef SLEV_bis1_rx_len
    #define SLEV_bis1_rx_len 0
#endif

#ifndef SL01_pdachn_scn
    #define SL01_pdachn_scn 0
#endif

#ifndef SL01_scn_secchn
    #define SL01_scn_secchn 0
#endif

#ifndef SL01_big_sync
    #define SL01_big_sync 0
#endif

#ifndef SL01_bsync0
    #define SL01_bsync0 0
#endif

#ifndef SL01_bsync1
    #define SL01_bsync1 0
#endif

#ifndef SL01_bsync0_tsk_jump
    #define SL01_bsync0_tsk_jump 0
#endif

#ifndef SL01_bsync1_tsk_jump
    #define SL01_bsync1_tsk_jump 0
#endif

#ifndef SL01_ext_scan_endis
    #define SL01_ext_scan_endis 0
#endif

#ifndef SL01_ext_adv_endis
    #define SL01_ext_adv_endis 0
#endif

#ifndef SL01_pda_adv_endis
    #define SL01_pda_adv_endis 0
#endif

#ifndef SL08_pdaSync_conflict
    #define SL08_pdaSync_conflict 0
#endif

#ifndef SL08_bisSync_conflict
    #define SL08_bisSync_conflict 0
#endif

#ifndef SL08_bisBcst_conflict
    #define SL08_bisBcst_conflict 0
#endif

#ifndef SL08_auxAdv_conflict
    #define SL08_auxAdv_conflict 0
#endif

#ifndef SL08_pdaAdv_conflict
    #define SL08_pdaAdv_conflict 0
#endif

#ifndef SL16_bigs_eventCnt
    #define SL16_bigs_eventCnt 0
#endif

#ifndef SL16_bis0_rx_pldNum
    #define SL16_bis0_rx_pldNum 0
#endif

#ifndef SL16_bis1_rx_pldNum
    #define SL16_bis1_rx_pldNum 0
#endif

#ifndef SL16_bis0_rx_pro
    #define SL16_bis0_rx_pro 0
#endif

#ifndef SL16_bis1_rx_pro
    #define SL16_bis1_rx_pro 0
#endif

#ifndef SL16_bis0_rxPdu2Sdu_st
    #define SL16_bis0_rxPdu2Sdu_st 0
#endif

#ifndef SL16_bis1_rxPdu2Sdu_st
    #define SL16_bis1_rxPdu2Sdu_st 0
#endif

#ifndef SL16_bis0_rx_sdu_len
    #define SL16_bis0_rx_sdu_len 0
#endif

#ifndef SL16_bis1_rx_sdu_len
    #define SL16_bis1_rx_sdu_len 0
#endif

#ifndef SLEV_cis_rx
    #define SLEV_cis_rx 0
#endif

#ifndef SLEV_cis_rx_rfLen
    #define SLEV_cis_rx_rfLen 0
#endif

#ifndef SLEV_cis_tx_rfLen
    #define SLEV_cis_tx_rfLen 0
#endif

#ifndef SLEV_cis_jump
    #define SLEV_cis_jump 0
#endif

#ifndef SLEV_cis_sdu_cmplt
    #define SLEV_cis_sdu_cmplt 0
#endif

#ifndef SLEV_iso_in
    #define SLEV_iso_in 0
#endif

#ifndef SLEV_iso_out
    #define SLEV_iso_out 0
#endif

#ifndef SLEV_iso_out_dat
    #define SLEV_iso_out_dat 0
#endif

#ifndef SL01_cis_group
    #define SL01_cis_group 0
#endif

#ifndef SL01_cis0
    #define SL01_cis0 0
#endif

#ifndef SL01_cis1
    #define SL01_cis1 0
#endif

#ifndef SL01_cis2
    #define SL01_cis2 0
#endif

#ifndef SL01_cis3
    #define SL01_cis3 0
#endif

#ifndef SL01_cis_4
    #define SL01_cis_4 0
#endif

#ifndef SL01_cis_5
    #define SL01_cis_5 0
#endif

#ifndef SL08_cis_snnesn
    #define SL08_cis_snnesn 0
#endif

#ifndef SL16_cis_tx_header
    #define SL16_cis_tx_header 0
#endif

#ifndef SL16_cis0_evtcnt
    #define SL16_cis0_evtcnt 0
#endif

#ifndef SL16_cis1_evtcnt
    #define SL16_cis1_evtcnt 0
#endif

#ifndef SL16_cis2_evtcnt
    #define SL16_cis2_evtcnt 0
#endif

#ifndef SL16_cis3_evtcnt
    #define SL16_cis3_evtcnt 0
#endif

#ifndef SL16_cis0_rxPldNum
    #define SL16_cis0_rxPldNum 0
#endif

#ifndef SL16_cis1_rxPldNum
    #define SL16_cis1_rxPldNum 0
#endif

#ifndef SL16_cis2_rxPldNum
    #define SL16_cis2_rxPldNum 0
#endif

#ifndef SL16_cis3_rxPldNum
    #define SL16_cis3_rxPldNum 0
#endif

#ifndef SL16_cis0_rxProPdu
    #define SL16_cis0_rxProPdu 0
#endif

#ifndef SL16_cis1_rxProPdu
    #define SL16_cis1_rxProPdu 0
#endif

#ifndef SL16_cis2_rxProPdu
    #define SL16_cis2_rxProPdu 0
#endif

#ifndef SL16_cis3_rxProPdu
    #define SL16_cis3_rxProPdu 0
#endif

#ifndef SL16_cis_rxPdu2Sdu_st
    #define SL16_cis_rxPdu2Sdu_st 0
#endif

#ifndef SL16_cis0_txSetPldNum
    #define SL16_cis0_txSetPldNum 0
#endif

#ifndef SL16_cis1_txSetPldNum
    #define SL16_cis1_txSetPldNum 0
#endif

#ifndef SL16_cis2_txSetPldNum
    #define SL16_cis2_txSetPldNum 0
#endif

#ifndef SL16_cis3_txSetPldNum
    #define SL16_cis3_txSetPldNum 0
#endif

#ifndef SL16_cis0_txCurSendPldNum
    #define SL16_cis0_txCurSendPldNum 0
#endif

#ifndef SL16_cis1_txCurSendPldNum
    #define SL16_cis1_txCurSendPldNum 0
#endif

#ifndef SL16_cis2_txCurSendPldNum
    #define SL16_cis2_txCurSendPldNum 0
#endif

#ifndef SL16_cis3_txCurSendPldNum
    #define SL16_cis3_txCurSendPldNum 0
#endif

#ifndef SL16_cis0_txPrePldNum
    #define SL16_cis0_txPrePldNum 0
#endif

#ifndef SL16_cis1_txPrePldNum
    #define SL16_cis1_txPrePldNum 0
#endif

#ifndef SL16_cis2_txPrePldNum
    #define SL16_cis2_txPrePldNum 0
#endif

#ifndef SL16_cis3_txPrePldNum
    #define SL16_cis3_txPrePldNum 0
#endif

#ifndef SLEV_cis0_sdu_cmplt
    #define SLEV_cis0_sdu_cmplt 0
#endif

#ifndef SLEV_cis1_sdu_cmplt
    #define SLEV_cis1_sdu_cmplt 0
#endif

#ifndef SLEV_cis0_rx_mic_fail
    #define SLEV_cis0_rx_mic_fail 0
#endif

#ifndef SLEV_cis1_rx_mic_fail
    #define SLEV_cis1_rx_mic_fail 0
#endif

#ifndef SLEV_cis_tx_padding
    #define SLEV_cis_tx_padding 0
#endif

#ifndef SL01_aclc_0
    #define SL01_aclc_0 0
#endif

#ifndef SL01_aclc_1
    #define SL01_aclc_1 0
#endif

#ifndef SL01_dbug0
    #define SL01_dbug0 0
#endif

#ifndef SL01_dbug1
    #define SL01_dbug1 0
#endif

#ifndef SL16_cis0_txSdu_len
    #define SL16_cis0_txSdu_len 0
#endif

#ifndef SL16_cis1_txSdu_len
    #define SL16_cis1_txSdu_len 0
#endif

#ifndef SL16_cis0_rxSdu_len
    #define SL16_cis0_rxSdu_len 0
#endif

#ifndef SL16_cis1_rxSdu_len
    #define SL16_cis1_rxSdu_len 0
#endif

#ifndef SL16_eqb_testcase_seqNum
    #define SL16_eqb_testcase_seqNum 0
#endif

#ifndef SL16_dbug0
    #define SL16_dbug0 0
#endif

#ifndef SL16_dbug1
    #define SL16_dbug1 0
#endif

#ifndef SLEV_app0
    #define SLEV_app0 0
#endif

#ifndef SLEV_app1
    #define SLEV_app1 0
#endif

#ifndef SLEV_app2
    #define SLEV_app2 0
#endif

#ifndef SLEV_app3
    #define SLEV_app3 0
#endif

#ifndef SLEV_app4
    #define SLEV_app4 0
#endif

#ifndef SLEV_app5
    #define SLEV_app5 0
#endif

#ifndef SLET_app0
    #define SLET_app0 0
#endif

#ifndef SLET_app1
    #define SLET_app1 0
#endif

#ifndef SLET_app2
    #define SLET_app2 0
#endif

#ifndef SLET_app3
    #define SLET_app3 0
#endif

#ifndef SLET_app4
    #define SLET_app4 0
#endif

#ifndef SL01_eadv_aux_sync
    #define SL01_eadv_aux_sync 0
#endif

#ifndef SL01_eadv_aux_chain
    #define SL01_eadv_aux_chain 0
#endif

#ifndef SL01_aclp_0
    #define SL01_aclp_0 0
#endif

#ifndef SL01_aclp_1
    #define SL01_aclp_1 0
#endif

#ifndef SL01_app0
    #define SL01_app0 0
#endif

#ifndef SL01_app1
    #define SL01_app1 0
#endif

#ifndef SL01_app2
    #define SL01_app2 0
#endif

#ifndef SL01_app3
    #define SL01_app3 0
#endif

#ifndef SL01_app4
    #define SL01_app4 0
#endif

#ifndef SL01_app5
    #define SL01_app5 0
#endif

#ifndef SL01_app6
    #define SL01_app6 0
#endif

#ifndef SL01_app7
    #define SL01_app7 0
#endif

#ifndef SL01_app8
    #define SL01_app8 0
#endif

#ifndef SLVE_CS_RX_IRQ
    #define SLVE_CS_RX_IRQ 0
#endif

#ifndef SLEV_CS_event_insert
    #define SLEV_CS_event_insert 0
#endif

#ifndef SL01_cs_refl_step_start_post
    #define SL01_cs_refl_step_start_post 0
#endif

#ifndef SL01_cs_refl_step_stx_start_post
    #define SL01_cs_refl_step_stx_start_post 0
#endif

#ifndef SL01_cs_refl_rev_ok
    #define SL01_cs_refl_rev_ok 0
#endif

#ifndef SL01_cs_subevent_0
    #define SL01_cs_subevent_0 0
#endif

#ifndef SL01_cs_step_init_0
    #define SL01_cs_step_init_0 0
#endif

#ifndef SL08_step_chnIdx
    #define SL08_step_chnIdx 0
#endif

#ifndef SL16_acl_cen_evtCnt
    #define SL16_acl_cen_evtCnt 0
#endif

#ifndef SL16_acl_per_evtCnt
    #define SL16_acl_per_evtCnt 0
#endif

#ifndef SL16_cs_proCnt
    #define SL16_cs_proCnt 0
#endif

#ifndef SL16_step_count
    #define SL16_step_count 0
#endif

#ifndef SLEV_PAWR_SYNC_DEBUG0
    #define SLEV_PAWR_SYNC_DEBUG0 0
#endif

#ifndef SLEV_PAWR_SYNC_DEBUG1
    #define SLEV_PAWR_SYNC_DEBUG1 0
#endif

#ifndef SLEV_PAWR_SYNC_DEBUG2
    #define SLEV_PAWR_SYNC_DEBUG2 0
#endif

#ifndef SLEV_PAWR_SYNC_DEBUG3
    #define SLEV_PAWR_SYNC_DEBUG3 0
#endif

#ifndef SLEV_PAWR_SYNC_DEBUG4
    #define SLEV_PAWR_SYNC_DEBUG4 0
#endif

#ifndef SLEV_PAWR_SYNC_DEBUG5
    #define SLEV_PAWR_SYNC_DEBUG5 0
#endif

#ifndef SLEV_PAWR_SYNC_RX
    #define SLEV_PAWR_SYNC_RX 0
#endif

#ifndef SLEV_PAWR_SYNC_TASK_BUILD
    #define SLEV_PAWR_SYNC_TASK_BUILD 0
#endif

#ifndef SL01_PAWR_SYNC_build
    #define SL01_PAWR_SYNC_build 0
#endif

#ifndef SL01_prd_adv
    #define SL01_prd_adv 0
#endif

#ifndef SL01_PADVB_ACAD_PAWR
    #define SL01_PADVB_ACAD_PAWR 0
#endif

#ifndef SL01_PAWR_SYNC_SUB
    #define SL01_PAWR_SYNC_SUB 0
#endif

#ifndef SL01_PAWR_SYNC_RSP_SLOT_TX
    #define SL01_PAWR_SYNC_RSP_SLOT_TX 0
#endif

#ifndef SL08_PAWR_SUBEVT_IDX
    #define SL08_PAWR_SUBEVT_IDX 0
#endif

#ifndef SL08_PDA_JUMP_CNT
    #define SL08_PDA_JUMP_CNT 0
#endif

#ifndef SL08_SUBEVT_JUMP_CNT
    #define SL08_SUBEVT_JUMP_CNT 0
#endif

#ifndef SL08_SUB_PDA_JUMP_CNT
    #define SL08_SUB_PDA_JUMP_CNT 0
#endif

#ifndef SL16_PAWR_EVT_CNT
    #define SL16_PAWR_EVT_CNT 0
#endif

#ifndef SL16_PAwR_chnIdx
    #define SL16_PAwR_chnIdx 0
#endif

#ifndef SL16_PDA_EVT_CNT
    #define SL16_PDA_EVT_CNT 0
#endif


#endif /* VCD_DEF_H_ */
