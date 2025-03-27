/********************************************************************************************************
 * @file    vcd_def_bis.h
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
#ifndef VCD_DEF_CS_H_
#define VCD_DEF_CS_H_


#include "debug_cfg.h"

#if (VCD_DEFINE_SELECT == VCD_DEFINE_CS)


//log_event, ID: 0~31, 0 is reserved for timeStamp, 1 ~ 31 is available
#define         SLEV_timestamp              0  // SLEV 0, reserved, do not change it
#define         SLEV_irq_rx                 1
#define         SLEV_irq_tx                 2
#define         SLEV_irq_rfdone             3
#define         SLEV_irq_stimer             4

#define         SLEV_sche_rebuild           7
#define         SLEV_sche_slotRst           8

#define         SLEV_acl_rx                 10
#define         SLVE_CS_RX_IRQ              11



#define         SLEV_CS_event_insert        12




//log_tick, ID: 0~31, 0 is reserved for timeStamp, 1 ~ 31 is available
#define         SLET_timestamp              0  // SLET 0, reserved, do not change it




//log_task, ID: 0~31, id0 is reserved,  1 ~ 31 is available
// 1-bit data:
#define         SL01_rsvd                   0
#define         SL01_IRQ                    1

#define         SL01_scn_prichn             2
#define         SL01_leg_adv                3

#define         SL01_aclc_0                 4
#define         SL01_aclp_0                 6

#define         SL01_cs_refl_step_start_post     7
#define         SL01_cs_refl_step_stx_start_post 8
#define         SL01_cs_refl_rev_ok              9


#define        SL01_cs_subevent_0                   16


#define        SL01_cs_step_init_0                  18



// 8-bit data: cid0 - cid63
#define         SL08_rsvd                           0
#define         SL08_cis_snnesn                     1
#define         SL08_step_chnIdx                    2


// 16-bit data: sid0 - sid63
#define         SL16_reserved                       0
#define         SL16_acl_cen_evtCnt                 1
#define         SL16_acl_per_evtCnt                 2
#define         SL16_cs_proCnt                      3

#define         SL16_step_count                     4

#endif //end of "VCD_DEFINE SELECT"


#endif /* VCD_DEF_CS_H_ */
