/********************************************************************************************************
 * @file    ll_sche.c
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

_attribute_ble_data_retention_  _attribute_aligned_(4)  sch_man_t   bltSche;

_attribute_ble_data_retention_  _attribute_aligned_(4)  pri_mng_t   bltPri;

_attribute_ble_data_retention_  _attribute_aligned_(4)  ll_future_task_t    bltFutTask; //future task




_attribute_ble_data_retention_  sch_task_t  bltSche_header = {
        .begin = (s32)-BIT(21), //-2,  //begin     BIT(21) about 40S
        .end   = (s32)-BIT(20), //-1,  //end         BIT(20) about 20S

};






_attribute_ram_code_
int blt_ll_linkAllPostTask(sch_task_t *pTsk_cur, sch_task_t *pStart_schTsk, int start_idx, int task_num_max)
{

    for(int i=start_idx; i<task_num_max; i++){
        pTsk_cur->next = (pStart_schTsk + i);
        pTsk_cur = pTsk_cur->next;
        //Insertions without conflicts may require no prioritization adjustment
        //blt_ll_incSchedulerTaskCalPriority((pStart_schTsk + i)->scheTask_oft, - bltPri.step_final[(pStart_schTsk + i)->scheTask_oft] );
        bltPri.csctvAbandonCnt[(pStart_schTsk + i)->scheTask_oft] = 0;

        u8 insertTskFlg = (pStart_schTsk + i)->scheTask_flg & TSKFLG_VALID_MASK;
        (void)insertTskFlg; //remove compiler warning
        my_dump_str_data(DBG_EXTADV_TIMING, "A0", &insertTskFlg, 1);
    }

    pTsk_cur->next = NULL;

    return 1;
}

#if (SCH_TASK_PRIORITY_IN_CB_EN || LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)
/* Too many switch-cases cause jump tables in Flash. Use -fno-jump-tables compiling option for file to solve.
 * Refer to zhangjian's email: <<risc-v platform debugging attention problems>> */
/* Return 1: resolve conflict succeed, otherwise failed. */
_attribute_ram_code_
u8 blt_ll_resolve_insertSchTskConflict(sch_task_t *pInsertTsk, sch_task_t *pTgtTsk)
{
    u8 insertTskIdx = pInsertTsk->scheTask_idx;
    u32 insertTskFlg = pInsertTsk->scheTask_flg & TSKFLG_VALID_MASK;

    u8 r = 0;
    switch (insertTskFlg){
        case TSKFLG_ACL_MASTER:
            #if (LL_ACL_CEN_EN)
                //if(ll_acl_master_irq_task_cb) //not judge, to save RamCode
                {
                    r = ll_acl_master_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_acl_master_interrupt_task()
                }
            #endif
            break;
        case TSKFLG_ACL_SLAVE:
            #if (LL_ACL_PER_EN)
                //if(ll_acl_slave_irq_task_cb) //not judge, to save RamCode
                {
                    r = ll_acl_slave_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_acl_slave_interrupt_task()
                }
            #endif
            break;
        case TSKFLG_CIG_MST:
            #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
                //if(ll_cig_mst_irq_task_cb) //not judge, to save RamCode
                {
                    r = ll_cig_mst_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_cig_mst_interrupt_task()
                }
            #endif
            break;
        case TSKFLG_CIG_SLV:
            #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
                //if(ll_cis_slv_irq_task_cb) //not judge, to save RamCode
                {
                    r = ll_cis_slv_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_cig_slv_interrupt_task()
                }
            #endif
            break;
        case TSKFLG_LEG_ADV:
                //if(ll_leg_adv_irq_task_cb)  //not judge, to save RamCode
                {
                    r = ll_leg_adv_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_leg_adv_interrupt_task()
                }
            break;
        case TSKFLG_EXT_ADV:
            #if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)
                //if(ll_ext_adv_irq_task_cb)  //not judge, to save RamCode
                {
                    r = ll_ext_adv_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_ext_adv_interrupt_task()
                }
            #endif
            break;
        case TSKFLG_AUX_ADV:
            #if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)
                //if(ll_ext_adv_irq_task_cb)  //not judge, to save RamCode
                {
                    r = ll_ext_adv_irq_task_cb(FLAG_INSERT_AUXADV_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_ext_adv_interrupt_task()
                }
            #endif
            break;
        case TSKFLG_PERD_ADV:
            #if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING)
                //if(ll_prd_adv_irq_task_cb)  //not judge, to save RamCode
                {
                    r = ll_prd_adv_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_prd_adv_interrupt_task()
                }
            #endif
            break;
        case TSKFLG_PAWRA_SUB:
            #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER) //PAwR-Advertiser- subevent_task
                //if(ll_pawra_sub_irq_task_cb)  //not judge, to save RamCode
                {
                    r = ll_pawra_sub_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_pawra_subx_interrupt_task()
                }
            #endif
            break;
        case TSKFLG_PAWRA_RSP:
            #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER) //PAwR-Advertiser- response_slots_task
                //if(ll_pawra_rsp_irq_task_cb)  //not judge, to save RamCode
                {
                    r = ll_pawra_rsp_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_pawra_rsp_interrupt_task()
                }
            #endif
            break;
        case TSKFLG_PRICHN_SCAN:
            //not needed
            break;
        case TSKFLG_SECCHN_SCAN:
            #if (LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)
                //if(ll_ext_scan_irq_task_cb)  //not judge, to save RamCode
                {
                    r = ll_ext_scan_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_ext_scan_interrupt_task()
                }
            #endif
            break;
        case TSKFLG_PDA_SYNC:
            #if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
                //if(ll_pda_sync_irq_task_cb)  //not judge, to save RamCode
                {
                    r = ll_pda_sync_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_pda_sync_interrupt_task()
                }
            #endif
            break;
        case TSKFLG_BIG_BCST:
            #if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
                //if(ll_big_bcst_irq_task_cb) //not judge, to save RamCode
                {
                    r = ll_big_bcst_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_big_bcst_interrupt_task()
                }
            #endif
            break;
        case TSKFLG_BIG_SYNC:
            #if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)
                //if(ll_big_sync_irq_task_cb) //not judge, to save RamCode
                {
                    r = ll_big_sync_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);  // blt_big_sync_interrupt_task()
                }
            #endif
            break;
        case TSKFLG_PAWRS_SUB:
            #if(LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
                //if(ll_pawr_sync_sub_irq_task_cb)
                {
                    r = ll_pawr_sync_sub_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk, NULL); //blt_pawr_sync_sub_interrupt_task
                }
            #endif
            break;
        case TSKFLG_CS:
            #if(LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
                if(ll_chn_sounding_irq_task_cb){
                    r= ll_chn_sounding_irq_task_cb(FLAG_INSERT_SCHTSK_CONFLICT|insertTskIdx, (void*)pTgtTsk);
                }
            #endif
            break;
        default:
            break;
    }
    return r;
}
#endif /*!< SCH_TASK_PRIORITY_IN_CB_EN */

/*
 * return 0: all the task insert succeed
 * other value: indicate the fail task number
 */
_attribute_ram_code_
int blt_ll_addTask2ExistLinklist( sch_task_t *pStart_schTsk, int task_num_max)
{
    sch_task_t  *pTsk_cur = NULL;
    sch_task_t  *pExtLkTsk_left = NULL;  //exist linkList left
    sch_task_t  *pExtLkTsk_right = NULL; //exist linkList right
    sch_task_t  *pExtLkTsk_next = NULL;

    pExtLkTsk_left = pExtLkTsk_right = bltSche.pTask_head;
    int task_abandon_cnt = 0;

    for(int i=0; i<task_num_max; i++ ){

        pTsk_cur = pStart_schTsk + i;
        //locate pLeft
        pExtLkTsk_next = pExtLkTsk_left;
        while(pExtLkTsk_next->end < pTsk_cur->begin){
            pExtLkTsk_left = pExtLkTsk_next;
            pExtLkTsk_next = pExtLkTsk_next->next;
            if(pExtLkTsk_next == NULL){
                blt_ll_linkAllPostTask(pExtLkTsk_left, pStart_schTsk, i, task_num_max);
                return 0; //Insert sch tsk OK
            }
        }

        //locate pRight
        //TODO: once see code stall in this loop when ACL slave slot allocation, maybe the master tail NULL bug(now fixed) triggers
        while( (pExtLkTsk_right != NULL) && (pExtLkTsk_right->begin <= pTsk_cur->end)){
            pExtLkTsk_right = pExtLkTsk_right->next;
        }



        int task_conflict_num = 0;
        u8 task_offset[MAX_CONFLICT_NUM + 1];  //attention: should add 1
        sch_task_t  *pTsk_traverse = pExtLkTsk_left->next;
        int curSchTaskOft = pTsk_cur->scheTask_oft;

        #if(SCH_TASK_PRIORITY_IN_CB_EN == 0)
            s32 pri_taskCur = bltPri.pri_cal[curSchTaskOft];
        #endif

        while(pTsk_traverse != pExtLkTsk_right){

            #if(SCH_TASK_PRIORITY_IN_CB_EN == 0)
                s32 pri_taskTra = bltPri.pri_cal[pTsk_traverse->scheTask_oft];
                 //priority lower than exist task, can not replace, abandon current task
                if(pri_taskCur <= pri_taskTra){
                    break;
                }
            #else
                if(!blt_ll_resolve_insertSchTskConflict(pTsk_cur, pTsk_traverse)){//0 insert fail, 1: insert success
                    break;
                }
            #endif

            task_offset[task_conflict_num ++] = pTsk_traverse->scheTask_oft;
            if( task_conflict_num > MAX_CONFLICT_NUM){
                break;
            }
            pTsk_traverse = pTsk_traverse->next;
        }

#if(CS_IOP_EN)
        if(pTsk_traverse == pExtLkTsk_right && ((task_conflict_num <= MAX_CONFLICT_NUM )|| (pStart_schTsk->scheTask_flg==TSKFLG_CS))){
#else
        //current task priority higher than all task traversed, and conflict number not exceed 4, can replace all other task
        if(pTsk_traverse == pExtLkTsk_right && task_conflict_num <= MAX_CONFLICT_NUM){
#endif

            if(task_conflict_num){
                blt_ll_addTask2AbandonTaskLinklist(pExtLkTsk_left->next, task_conflict_num); //attention: must execute before pExtLkTsk_left->next changed
                pTsk_cur->cover_other = 1; // add cover other task mark
            }

            pExtLkTsk_left->next = pTsk_cur;
            pTsk_cur->next = pExtLkTsk_right;

            blt_ll_incSchedulerTaskCalPriority( curSchTaskOft, - bltPri.step_final[curSchTaskOft] );
            bltPri.csctvAbandonCnt[curSchTaskOft] = 0;
            for(int j=0; j<task_conflict_num; j++){
                bltPri.csctvAbandonCnt[task_offset[j]]++;
                blt_ll_incSchedulerTaskCalPriority( task_offset[j], bltPri.step_final[task_offset[j]]*3 );
            }

            u8 insertTskFlg = pTsk_cur->scheTask_flg & TSKFLG_VALID_MASK;
            (void)insertTskFlg; //remove compiler warning
            my_dump_str_data(DBG_EXTADV_TIMING, "A1", &insertTskFlg, 1);
        }
        else{ //abandon current task
            task_abandon_cnt++;
            blt_ll_addTask2AbandonTaskLinklist(pTsk_cur, 1);
            blt_ll_incSchedulerTaskCalPriority( curSchTaskOft, bltPri.step_final[curSchTaskOft]*2 );
            bltPri.csctvAbandonCnt[curSchTaskOft]++;

            /* add cover other task mark */
            sch_task_t  *pTsk_keep = pExtLkTsk_left->next;
            pTsk_keep->cover_other = 1;
            if(task_conflict_num){
                task_conflict_num --;
                while(task_conflict_num){
                    pTsk_keep = pTsk_keep->next;
                    if(pTsk_keep == NULL){
                        BLMS_ERR_DEBUG(SCH_DEBUG_EN, 0xFF110000);
                    }
                    pTsk_keep->cover_other = 1;
                    task_conflict_num --;
                }
            }
        }


    }

    return task_abandon_cnt;
}





_attribute_ram_code_
int blt_ll_addTask2AbandonTaskLinklist( sch_task_t *pStart_schTsk, int task_num)
{
    (void)pStart_schTsk;(void)task_num;
    return 0;
}



#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_
#endif
static inline void blt_ll_cal_sslot_diff_next(void){
    int mod = bltSche.sSlot_idx_next & 0x1F;  //bltSche.sSlot_idx_next%32;
    if(mod){
        bltSche.sSlot_diff_next = (32 - mod);
    }
    else{
        bltSche.sSlot_diff_next = 0;
    }
}






static inline void blt_ll_combine_linklayer_task(void)
{

#if (LL_RSSI_SNIFFER_MASTER_ENABLE)
    if( bltSche.task_mask & TSKMSK_SNIFM_SEEK_ALL ){
        //if(ll_acl_sniffer_mst_irq_task_cb)
        {
            ll_acl_sniffer_mst_irq_task_cb(FLAG_SCHEDULE_BUILD); // blt_acl_sniffer_mst_irq_task() blt_ll_buildAclSnifferMstSeekSchLinklist()
        }
    }
#endif

#if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
    if( bltSche.task_mask & TSKMSK_SNIFS_SEEK_ALL ){
        //if(ll_acl_sniffer_slv_irq_task_cb) //not judge, to save RamCode
        {
            ll_acl_sniffer_slv_irq_task_cb(FLAG_SCHEDULE_BUILD); // blt_acl_sniffer_slv_irq_task() blt_ll_buildAclSnifferSlvSeekSchLinklist()
        }
    }
#endif

#if (LL_ACL_CEN_EN)
    if( bltSche.task_mask & TSKMSK_ACL_MASTER_ALL ){
        //if(ll_acl_master_irq_task_cb) //not judge, to save RamCode
        {
            ll_acl_master_irq_task_cb(FLAG_SCHEDULE_BUILD, NULL);  // blt_acl_master_interrupt_task() blt_ll_buildAclMasterSchedulerLinklist()
        }
    }
#endif


#if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    if(ll_cis_conn_irq_task_cb) //must judge
    {
        ll_cis_conn_irq_task_cb(FLAG_CIS_SCHEDULER_TASK, NULL);  // blt_cis_conn_interrupt_task()  blt_cis_scheduler_task()
    }
#endif


#if (LL_ACL_PER_EN)
    if( bltSche.task_mask & TSKMSK_ACL_SLAVE_ALL ){
        //if(ll_acl_slave_irq_task_cb) //not judge, to save RamCode
        {
            ll_acl_slave_irq_task_cb(FLAG_SCHEDULE_BUILD, NULL);  // blt_acl_slave_interrupt_task() blt_ll_build_acl_slave_schedule()
        }
    }
#endif


#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING)
    if(bltSche.task_mask & TSKMSK_PERD_ADV_ALL){
        //if(ll_prd_adv_irq_task_cb)  //not judge, to save RamCode
        {
            #if (SLEV_pda_adv_build)
            log_event_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SLEV_pda_adv_build);
            #endif
            ll_prd_adv_irq_task_cb(FLAG_SCHEDULE_BUILD, NULL);  // blt_prd_adv_interrupt_task() blt_ll_buildPerdAdvSchedulerLinklist
        }
    }

    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER) //PAwR-Advertiser- response_slots_task
        if(bltSche.task_mask & TSKMSK_PAWRA_SUB_ALL){
            //if(ll_pawra_sub_irq_task_cb)  //not judge, to save RamCode
            {
                ll_pawra_sub_irq_task_cb(FLAG_SCHEDULE_BUILD, NULL);  // blt_pawra_subx_interrupt_task() blt_pawra_subx_sch_build
            }
        }
        if(bltSche.task_mask & TSKMSK_PAWRA_RSP_ALL){
            //if(ll_pawra_rsp_irq_task_cb)  //not judge, to save RamCode
            {
                ll_pawra_rsp_irq_task_cb(FLAG_SCHEDULE_BUILD, NULL);  // blt_pawra_rsp_interrupt_task() blt_pawra_rsp_sch_build
            }
        }
    #endif

#endif

#if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
    if( bltSche.task_mask & TSKMSK_BIG_BCST_ALL ){
        //if(ll_big_bcst_irq_task_cb) //not judge, to save RamCode
        {
            #if (SLEV_bigBcst_build)
            log_event_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SLEV_bigBcst_build);
            #endif
            ll_big_bcst_irq_task_cb(FLAG_SCHEDULE_BIGBCST_BUILD, NULL);  // blt_big_bcst_interrupt_task()  blt_ll_buildBigBcstSchedulerLinklist
        }
    }
#endif

#if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)
    if( bltSche.task_mask & TSKMSK_BIG_SYNC_ALL ){
        //if(ll_big_sync_irq_task_cb) //not judge, to save RamCode
        {
            #if (SLEV_bigScan_build)
            log_event_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SLEV_bigScan_build);
            #endif
            ll_big_sync_irq_task_cb(FLAG_SCHEDULE_BIGSYNC_BUILD, NULL);  // blt_big_sync_interrupt_task()  blt_ll_buildBigSyncSchedulerLinklist
        }
    }
#endif

#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
    if(bltSche.task_mask & TSKMSK_PDA_SYNC_ALL){
        //if(ll_pda_sync_irq_task_cb)  //not judge, to save RamCode
        {
            #if (SLEV_pdaScan_build)
            log_event_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SLEV_pdaScan_build);
            #endif
            ll_pda_sync_irq_task_cb(FLAG_SCHEDULE_BUILD, NULL);  // blt_pda_sync_interrupt_task() //blt_pda_sync_build_task
        }
    }

    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
        if(bltSche.task_mask & TSKMSK_PAWRS_SUB_ALL){
            //if(blt_pawr_sync_sub_interrupt_task)
            {
                ll_pawr_sync_sub_irq_task_cb(FLAG_SCHEDULE_BUILD, NULL, NULL); //blt_pawr_sync_sub_interrupt_task() //blt_ll_PAwRsync_build_task
            }
        }
    #endif
#endif

#if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)
    if(bltSche.task_mask & TSKMSK_AUX_ADV_ALL){
        //if(ll_ext_adv_irq_task_cb)  //not judge, to save RamCode
        {
            #if (SLEV_eadv_aux_build)
            log_event_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SLEV_eadv_aux_build);
            #endif
            ll_ext_adv_irq_task_cb(FLAG_SCHEDULE_BUILD, NULL);  // blt_ext_adv_interrupt_task()  blt_ll_build_auxadv_task
        }
    }

    if(bltSche.task_mask & TSKMSK_EXT_ADV_ALL)
    {
        //if(ll_ext_adv_irq_task_cb)  //not judge, to save RamCode
        {
            #if (SLEV_eadv_ext_build)
            log_event_irq(SL_STACK_EXT_PRD_BASE_TIMING_EN, SLEV_eadv_ext_build);
            #endif

            ll_ext_adv_irq_task_cb(FLAG_SCHEDULE_EXTADV_BUILD, NULL);  // blt_ext_adv_interrupt_task()  blt_ll_build_extadv_task
        }

        #if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN && ANOTHER_BIG_INTV_EXTENDED_ADV)
            if(bltSche.task_mask == ((bltSche.task_en & TSKMSK_EXT_ADV_ALL)|TSKMSK_PRICHN_SCAN))
            {

                //if(ll_prichn_scan_irq_task_cb) //not judge, to save RamCode
                {
                     ll_prichn_scan_irq_task_cb(FLAG_SCHEDULE_PRICHN_SCAN_ALIGN_BUILD); //blt_prichn_scan_interrupt_task  blt_ll_prichn_scan_align_build
                }
            }
        #endif
    }
#endif


    if(bltSche.task_mask & TSKMSK_LEG_ADV)
    {
        //if(ll_leg_adv_irq_task_cb)  //not judge, to save RamCode
        {
            ll_leg_adv_irq_task_cb(FLAG_SCHEDULE_LEGADV_BUILD, NULL);  // blt_leg_adv_interrupt_task()  blt_ll_buildLegacyAdvTask()
        }

        #if (PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN && !ANOTHER_BIG_INTV_EXTENDED_ADV)
            if(bltSche.task_mask == (TSKMSK_LEG_ADV | TSKMSK_PRICHN_SCAN))
            {
                //if(ll_prichn_scan_irq_task_cb) //not judge, to save RamCode
                {
                     ll_prichn_scan_irq_task_cb(FLAG_SCHEDULE_PRICHN_SCAN_ALIGN_BUILD); //blt_prichn_scan_interrupt_task  blt_ll_prichn_scan_align_build
                }
            }
        #endif
    }

#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
    if(bltSche.task_mask & TSKMSK_CS_ALL)
    {
        if(ll_chn_sounding_irq_task_cb){
            ll_chn_sounding_irq_task_cb(FLAG_SCHEDULE_BUILD, NULL); //blt_cs_interrupt_task (int flag, void *p)   blt_ll_rebuildCsSchedulerLinklist
        }
    }
#endif

}




_attribute_ram_code_
void blt_ll_noTask_build_proc(void)
{
    int bSlot_actLen = ((bltSche.sSlot_endIdx_maxPri + 31) - (bltSche.sSlot_idx_next + bltSche.sSlot_diff_next))>>5;  //bSLot actual length
    bltSche.bSlot_idx_next += bSlot_actLen;
    bltSche.sSlot_idx_next = bltSche.sSlot_endIdx_maxPri;
    bltSche.sSlot_tick_next = bltSche.sSlot_tick_start + bltSche.sSlot_endIdx_maxPri*SSLOT_TICK_NUM;

    blt_ll_cal_sslot_diff_next();

    bltSche.build_index ++;
    if(bltSche.build_index > 100){  //8S, debug
        BLMS_ERR_DEBUG(SCH_DEBUG_EN, 0xFF0D0000);
    }
}

#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
void blt_ll_buildTaskLinkList(void) //blt_ll_combine_linklayer_task
{
    bltPri.priMax_value = TASK_PRIORITY_LOW - 1;
    bltPri.priMax_index = 0;  //init_value
    bltSche.build_index = 0;

    while(1)
    {
        linklist_build_start:

        bltSche.bSlot_maxLen = SCHE_PRE_ALLOCATE_BSLOT_NUM - 9 + ((clock_time()>>3) & 7 );  //  (128 - 9) ~ (128 - 2)

        bltSche.bSlot_endIdx_dft = bltSche.bSlot_idx_next + bltSche.bSlot_maxLen;   //right align
        bltSche.sSlot_endIdx_dft = bltSche.sSlot_idx_next + bltSche.sSlot_diff_next + (bltSche.bSlot_maxLen<<5);  //  "<<5" = "*32"  //right align
        bltSche.sSlot_endIdx_maxPri = bltSche.sSlot_endIdx_dft;

        blt_ll_combine_linklayer_task();

        if(bltSche.pTask_head->next == NULL){
            /* if the tail is NULL,  */
            blt_ll_noTask_build_proc();
        }
        else{
            bltSche.lklt_taskNum = 0;
            sch_task_t  *pTsk_cur = bltSche.pTask_head;
            sch_task_t  *pTsk_next = pTsk_cur->next;

            while(1){
                if( pTsk_next == NULL ){  //tail
                    break;
                }


                if( pTsk_next->end >= bltSche.sSlot_endIdx_maxPri){//bltSche.sSlot_endIdx_dft
                    pTsk_cur->next = NULL; //fix, important, remove post task

                    if(bltSche.lklt_taskNum == 0){//make sure find one task at least.
                        blt_ll_noTask_build_proc();
                        goto linklist_build_start;
                    }

                    break;
                }

                //traverse to next
                pTsk_cur = pTsk_next;
                pTsk_next = pTsk_cur->next;
                bltSche.lklt_taskNum ++;
            }

            bltSche.lastTsk_endSslot = pTsk_cur->end;
            bltSche.lastTsk_endBslot = bltSche.bSlot_idx_start + (bltSche.lastTsk_endSslot>>5);  // ">>5" = "/32"
            bltSche.lastTsk_endTick = bltSche.sSlot_tick_start + (pTsk_cur->end + 1)*SSLOT_TICK_NUM;

            break;
        }
    }

}


u32 blt_ll_GetSchedulerNearIrqTick(void)
{
    return bltSche.sSlot_tick_irq;
}


#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
int     blt_ll_updateScheduler(void)//blt_ll_buildTaskLinkList
{
    int task_rebuild = 0;

    u64 task_other_than_scan = bltSche.task_mask & (~(TSKMSK_PRICHN_SCAN | TSKMSK_SECCHN_SCAN_ALL));
    if(bltSche.update || !bltSche.pTask_next){
        task_rebuild = 1;
        bltSche.task_rebuild = 1;
        my_dump_str_u32s(0, "reb", bltSche.update, bltSche.pTask_next, blms_state,0);
    }
    else{
        if( blmsParam.cig_slv_1st_sche_build_pending && (blmsParam.cis_1st_anchor_bSlot < bltSche.lastTsk_endBslot) ){
            //if(ll_cis_slv_irq_task_cb) //not judge, to save RamCode
            {
                my_dump_str_data(DBG_CIS_1ST_AP_TIMING_EN, "[CIS][TIM] ciss 1st ap create, in cur slot map", 0, 0);
                //DBG_C HN11_TOGGLE;
                task_rebuild = 1;
                //ll_cis_slv_irq_task_cb(FLAG_SCHEDULE_CIGSLV_GET1ST_AP, NULL); //blt_cig_slv_interrupt_task  blt_ll_calcCigSlv1stAndCis1stAnchorPoint
            }
        }
    }


    if(task_rebuild){

        #if (DYNAMIC_SCHE_CAL_TIME_EN)
            bltSche.cal_time_en = 1;
        #endif

#if 0
        for(int i= 0; i< PRI_TASK_NUM; i++){
            bltPri.pri_cal[i] = bltPri.pri_now[i]; //TODO: optimize, speed up
        }
#else   //Under -Os compile optimization, this for loop will be changed to memcpy(in Flash) by toolchain.
        smemcpy(bltPri.pri_cal, bltPri.pri_now, PRI_TASK_NUM*sizeof(pri_data_t));
#endif

        if(1)
        {
            #if (SLEV_sche_rebuild)
                log_event_irq(SL_STACK_SCHE_TIMING_EN, SLEV_sche_rebuild);
            #endif
            DBG_CHN13_TOGGLE;
            DBG_SIHUI_CHN13_TOGGLE;
            DBG_FANQH_CHN13_TOGGLE;

            //my_dump_str_u32s(0, "pri cal",    bltPri.pri_cal[TSKOFT_ACL_MASTER],  bltPri.pri_cal[TSKOFT_CIG_MST], 0, 0);

            bltSche.pTask_head->next = NULL;
            blt_ll_cal_sslot_diff_next();

            //wait until task LinkList is empty, there is 67-41=26 S time margin
             //BIT(16):65536*19.5uS = 1.28S; BIT(17):2.56S;; BIT(18):5.12S;  BIT(19):10.2S;BIT(20):20.4S; BIT(21):40.8S;  can not exceed 67S
            //if( (bltSche.sSlot_idx_next > BIT(17)) && !blmsParam.new_conn_forbidden){

            /*TODO: if depend on tail task NULL, one test(app adv enable&scan enable very frequently, UPDATE happens) may lead FF03 bug,
                    should consider timing exceed 130S, add a special process*/
            //if( (NULL == bltSche.pTask_next) && (bltSche.sSlot_idx_next > BIT(20)) && !blmsParam.new_conn_forbidden && !blmsParam.cis_create_pending )
        #if (1)  //use 2.5S to test potential risk
            if( (bltSche.sSlot_idx_next > (s32)BIT(17)) && !blmsParam.new_conn_forbidden && !blmsParam.connUptCmd_pending && !blmsParam.cis_create_pending )
        #else
            if( (bltSche.sSlot_idx_next > (s32)BIT(19)) && !blmsParam.new_conn_forbidden && !blmsParam.connUptCmd_pending && !blmsParam.cis_create_pending )
        #endif
            {
                //DBG_CHN12_TOGGLE;
//              DBG_SIHUI_CHN12_TOGGLE;

                #if (SLEV_sche_slotRst)
                    log_event_irq(SL_STACK_SCHE_TIMING_EN, SLEV_sche_slotRst);
                #endif
                //my_dump_str_data(0, "reset sSlot", &bltSche.sSlot_idx_next, 4);

                bltSche.sSlot_idx_reset = 1;

                bltSche.sSlot_idx_next += bltSche.sSlot_diff_next;

                int bSlot_length = bltSche.sSlot_idx_next/32;
                //bltSche.sSlot_idx_start = 0;  //always 0, so no need set
                bltSche.bSlot_idx_next = bltSche.bSlot_idx_start = bltSche.bSlot_idx_start + bSlot_length;
                bltSche.sSlot_tick_next = bltSche.sSlot_tick_start = bltSche.bSlot_tick_start = bltSche.bSlot_tick_start + bSlot_length*SYSTEM_TIMER_TICK_625US;

                bltSche.sSlot_idx_past = bltSche.sSlot_idx_next; //must update after calculating sSlot_length

                my_dump_str_data(0, "reset sSlot", &bltSche.sSlot_idx_next, 4);

                bltSche.sSlot_diff_next = 0;

                //////////////////////////////
                #if(LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
                    if(ll_pda_sync_irq_task_cb)
                    {
                        /*Must be placed below "bltSche.sSlot_idx_past = bltSche.sSlot_idx_next;"
                         *Now only periodic sync module use this. so now place in pda sync module.*/
                        ll_pda_sync_irq_task_cb(FLAG_SCHEDULE_SSLOT_RESET, NULL); //blt_pda_sync_interrupt_task
                    }
                #endif
            }
            else{
                bltSche.bSlot_idx_next = bltSche.bSlot_idx_start + ((bltSche.sSlot_idx_next+31)>>5);  // ">>5" = "/32"
                //my_dump_str_data(DBG_LEGADV_Ff0A, "bSlot_idx_next", &bltSche.bSlot_idx_next, 4);
            }


            #if(SCH_DEBUG_EN)
                // BIT(31)/625 = 3435974, 100000*19.5uS = 1.95S, 3435974 - 100000 = 0x32E726
                if(bltSche.sSlot_idx_next > (3435974 - 100000) ){
                    write_dbg32(DBG_SRAM_ADDR + 4, bltSche.sSlot_idx_next);
                    //my_dump_str_u32s(DBG_EXTSCAN_TIMING, "debug 6", bltSche.sSlot_idx_next, bltSche.sSlot_idx_irq_real, 0, 0);
                    BLMS_ERR_DEBUG(SCH_DEBUG_EN, 0xFF030000);
                }
            #endif

            //very important, and position is very reasonable
            if(bltSche.sSlot_idx_reset){
                bltSche.sSlot_idx_next = 0;
                bltSche.sSlot_idx_irq_real -= bltSche.sSlot_idx_past;  //refer to issue_list(SiHui)
                //my_dump_str_u32s(DBG_EXTSCAN_TIMING, "debug 5", bltSche.sSlot_idx_irq_real, 0, 0, 0);
            }



            if(bltSche.task_mask)
            {
                if(task_other_than_scan)
                {
                    blt_ll_buildTaskLinkList();
                }
                else
                {
                    if((bltSche.task_mask & TSKMSK_PRICHN_SCAN) == 0){ //no prichn scan
                        BLMS_ERR_DEBUG(DBG_EXTSCAN_LOGIC, 0xFE0B0000);
                    }

                    if(ll_prichn_scan_irq_task_cb){
                         ll_prichn_scan_irq_task_cb(FLAG_SCHEDULE_BUILD); //blt_prichn_scan_interrupt_task
                    }
                }
            }
            else{  //bltSche.task_mask == 0
                blmsParam.sche_run_flag = 0;
            }

            bltSche.pTask_pre = bltSche.pTask_head;
            bltSche.pTask_next = bltSche.pTask_head->next;
        }


        bltSche.update = 0;  //clear
    }





    bltSche.immediate_task = 0;




    if(bltSche.pTask_next) //at least one slot task exist in current link_list or an immediate task exist
    {

        #if (LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)
            if((bltSche.task_mask & TSKMSK_SECCHN_SCAN_ALL) || (bltSche.task_mask & TSKMSK_PAWRS_RSP_ALL)){
                //if(ll_ext_scan_irq_task_cb) //save RamCode
                {
                    ll_ext_scan_irq_task_cb(FLAG_SCHEDULE_SECCHN_SCAN_INSERT, NULL); // blt_ext_scan_interrupt_task
                }
            }
        #endif


        bltSche.sSlot_tick_irq = bltSche.sSlot_tick_start + bltSche.pTask_next->begin*SSLOT_TICK_NUM;

        my_dump_str_u32s(0, "cap", bltSche.pTask_next->begin, bltSche.sSlot_tick_irq, bltSche.pTask_next, bltSche.pTask_next->next);
        systick_irq_trigger = SYS_IRQ_TRIG_NEW_TASK;
        /* ctx/crx_start preset ctx/crx_post generation time point (system timer capture value preset),
         * if an end RF signal such as cmd_done is received, it ends early, and the difference between
         * the early end time point and the preset time point cannot be too large , Once the execution
         * of the task at the end of the premature exceeds the preset value, the system timer status is
         * set, and the timer capture value is set after the next task is obtained, which immediately
         * causes an interrupt. */
        #if (FIX_STIMER_SET_CAPTURE_ERR)
            /* set to a long time to avoid signal pulling which may lead to new IRQ no trigger */
            systimer_set_irq_capture(clock_time() ^ BIT(31));
        #endif
        systimer_clr_irq_status();
        systimer_set_irq_capture(bltSche.sSlot_tick_irq);


        #if (SCH_DEBUG_EN)
            /*                         T_now
             *                           | <------ 5 S ------> |
             *                 Error  -> | <---- Correct ----> | <- Error
             *
             */
            int debug_diff = (int)(systimer_get_irq_capture() - clock_time());

            #if (LL_RSSI_SNIFFER_MODE_ENABLE)
                #if (LL_RSSI_SNIFFER_SLAVE_ENABLE && LL_RSSI_SNIFFER_MASTER_ENABLE)
                    bool snif_used = (ll_acl_sniffer_mst_irq_task_cb || ll_acl_sniffer_slv_irq_task_cb) ? TRUE : FALSE;
                #elif (LL_RSSI_SNIFFER_MASTER_ENABLE)
                    bool snif_used = ll_acl_sniffer_mst_irq_task_cb ? TRUE : FALSE;
                #elif (LL_RSSI_SNIFFER_SLAVE_ENABLE)
                    bool snif_used = ll_acl_sniffer_slv_irq_task_cb ? TRUE : FALSE;
                #endif
                    if(snif_used){
                        debug_diff += 100*SYSTEM_TIMER_TICK_1US;
                    }
                    else{
                        debug_diff += 50*SYSTEM_TIMER_TICK_1US;
                    }
            #endif

            if(  debug_diff > 5*SYSTEM_TIMER_TICK_1S ||  debug_diff < 0)
            {
                DBG_CHN12_TOGGLE;
//              DBG_SIHUI_CHN12_TOGGLE;
                write_dbg32(DBG_SRAM_ADDR + 4, systimer_get_irq_capture());
                write_dbg32(DBG_SRAM_ADDR + 8, clock_time());
                //write_dbg32(DBG_SRAM_ADDR + 12, debug_diff); //Overlaps with other areas, resulting in an error
                BLMS_ERR_DEBUG(SCH_DEBUG_EN, 0xFF0A0000 | bltSche.pTask_next->scheTask_oft);
            }
        #endif
        #if (BLE_LLMIC_CONCURRENT_EN)
         extern void blt_llmic_updateNextTask(void);
         blt_llmic_updateNextTask();
        #endif
    }
    else
    {
        #if (BLE_LLMIC_CONCURRENT_EN)
        extern  void blt_ll_set_llmic_status(llmic_ble_sigl_e ble_signal);
        blt_ll_set_llmic_status(BLE_SIGL_IDLE);
        #endif
    }






    /* Insert Scan task if the left timing is enough */
    u32 tick_rest = (u32)(bltSche.sSlot_tick_irq -  clock_time());
    if(tick_rest < BIT(30))   //smaller than 1/4 circle(about 65 S for 16M sys_timer)
    {
        #if (LL_ACL_PER_EN && BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE)
            if(aclConn_param.updateCmd_pending && !aes_enc_dec_busy){
                //if(ll_acl_slave_irq_task_cb) //not judge, to save RamCode
                {
                    ll_acl_slave_irq_task_cb(FLAG_ACL_SLAVE_CHECK_UPDATE_CMD_DEC, NULL);  // blt_acl_slave_slotgap_procUpdateReq
                }
            }
            tick_rest = (u32)(bltSche.sSlot_tick_irq -  clock_time());
        #endif

        #if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER && BLS_PROC_BIS_SYNC_UPDATE_REQ_IN_IRQ_ENABLE)
            //It is best not to deal with ACLuptCmdPending and BISSYNCuptCmdPending at the same time. Because ASE_DEC takes a long time.
            if((!aclConn_param.updateCmd_pending) && bisSync_param.updateCmd_pending && !aes_enc_dec_busy){
                //if(ll_big_sync_irq_task_cb) //not judge, to save RamCode
                {
                    my_dump_str_data(0, "sch gap process bigctrl holding", 0, 0);
                    ll_big_sync_irq_task_cb(FLAG_BIS_SYNC_CHECK_UPDATE_CMD_DEC, NULL);  // blt_bisSync_slotgap_procUpdateReq
                }
            }
            tick_rest = (u32)(bltSche.sSlot_tick_irq -  clock_time());
        #endif

        int scan_available = 0;

        /* Not only the SCAN task, there are other tasks  */
        if( (bltSche.task_mask & TSKMSK_PRICHN_SCAN) && task_other_than_scan && (blms_state != BLMS_STATE_PRICHN_SCAN_E) )
        {
            if(ll_prichn_scan_irq_task_cb){
                 scan_available = ll_prichn_scan_irq_task_cb(FLAG_SCHEDULE_PRICHN_SCAN_INSERT);//blt_prichn_scan_interrupt_task
            }
        }

        #if (BLMS_PM_ENABLE)
            if( bltSche.pTask_next && !scan_available && tick_rest > (20 * SYSTEM_TIMER_TICK_1US)){  //not idle state
                blmsPm.sleep_allowed = 1;
                blmsPm.next_task_tick = bltSche.sSlot_tick_irq;
                blmsPm.pTask_wakeup = bltSche.pTask_next;
            }
        #else
            (void)scan_available;  //remove warning
        #endif
    }




    /* attention: make sure this must be executed, no return in previous code,
     *            and must reset in the end of "blt_ll_updateScheduler", cause some other module will use this flag */
    bltSche.sSlot_idx_reset = 0;

    bltSche.task_rebuild = 0;





    return 1;
}







//DEBUG
// 1600 1S    2^10 -> 0.64 S  2^13->5S 2^14->10S  2^15->20S
//#define           SLOT_INDEX_ALARM_LOW                            BIT(13) //5S
//#define           SLOT_INDEX_ALARM_HIGH                           BIT(14) //10S
//#define           SLOT_INDEX_INIT                                 0

//DEBUG
//#define           SLOT_INDEX_ALARM_LOW                            0xFFFFC000   //10S left
//#define           SLOT_INDEX_ALARM_HIGH                           0xFFFFE000   //5S  left
//#define           SLOT_INDEX_INIT                                 0xFFFF8000   //20S left

_attribute_noinline_
void blt_ll_reset_bSlot_idx(void)
{
    u32 r = irq_disable();

    if(blms_state & (BLMS_STATE_PRICHN_SCAN_S | BLMS_STATE_SECCHN_SCAN_S)){
        rf_set_tx_rx_off();
    }
    STOP_RF_STATE_MACHINE;


    extern void blt_hal_reset_baseband(void);
    blt_hal_reset_baseband();

    sleep_us(20);

    bltSche.task_mask = 0;
    blmsParam.sche_run_flag= 0;
    blmsParam.state_chng = 1;


    CLEAR_ALL_RFIRQ_STATUS;  //clear all RF IRQ
    systimer_clr_irq_status();
    systimer_irq_disable();

    irq_restore(r);
}


_attribute_noinline_
void blt_ll_proc_bSlot_idx_overflow(void)
{
    if( (bltSche.bSlot_idx_start & SLOT_INDEX_ALARM_HIGH) == SLOT_INDEX_ALARM_HIGH){  //alarm high
        start_reboot();  //not work now
    }
    else if( (bltSche.bSlot_idx_start & SLOT_INDEX_ALARM_LOW) == SLOT_INDEX_ALARM_LOW){  //alarm low
        //find no connection time
        if( !(bltSche.task_mask & TSKMSK_ACL_CONN_ALL) ){
            blt_ll_reset_bSlot_idx();
        }

    }
}






#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
void blt_ll_irq_startScheduler(void)  //write in flash to save RamCode
{
    blms_state = BLMS_STATE_SCHE_START;

    bltSche.bSlot_idx_start = bltSche.bSlot_idx_next = SLOT_INDEX_INIT;

    bltSche.sSlot_idx_next = 0;
    bltSche.sSlot_idx_past = 0;
    bltSche.sSlot_idx_irq_real = 0;

#if (LL_RSSI_SNIFFER_MODE_ENABLE)

    #if (LL_RSSI_SNIFFER_SLAVE_ENABLE && LL_RSSI_SNIFFER_MASTER_ENABLE)
        bool snif_used = (ll_acl_sniffer_mst_irq_task_cb || ll_acl_sniffer_slv_irq_task_cb) ? TRUE : FALSE;
    #elif (LL_RSSI_SNIFFER_MASTER_ENABLE)
        bool snif_used = ll_acl_sniffer_mst_irq_task_cb ? TRUE : FALSE;
    #elif (LL_RSSI_SNIFFER_SLAVE_ENABLE)
        bool snif_used = ll_acl_sniffer_slv_irq_task_cb ? TRUE : FALSE;
    #endif

    if(snif_used)
    {
        bltSche.sSlot_tick_irq_real = bltSche.sSlot_tick_irq;
    }

    bltSche.bSlot_tick_start = bltSche.bSlot_tick_irq_real = bltSche.sSlot_tick_start = \
    bltSche.sSlot_tick_next = bltSche.sSlot_tick_irq = clock_time() + 300 * SYSTEM_TIMER_TICK_1US;
#else
    bltSche.bSlot_tick_start = bltSche.bSlot_tick_irq_real = bltSche.sSlot_tick_start = \
    bltSche.sSlot_tick_next = bltSche.sSlot_tick_irq = clock_time() + 1000 * SYSTEM_TIMER_TICK_1US;
#endif

}




// Adv enable & Scan enable can trigger this function
// only mainLoop trigger slot timing start, no need run in ram_code
_attribute_noinline_
int blt_ll_mainloop_startScheduler(void)
{
    systick_irq_trigger = SYS_IRQ_TRIG_SCHE_START;

    systimer_clr_irq_status();

    #if (DYNAMIC_SCHE_CAL_TIME_EN)
        bltSche.sche_process_us = 200; //set a new initial value
    #endif

#if BLE_LLMIC_CONCURRENT_EN
    /* main_loop trigger system timer scheduler start task: call function in irq: blt_ll_irq_startScheduler */
    bltSche.sSlot_tick_irq = clock_time() + 10 * SYSTEM_TIMER_TICK_1MS;
    systimer_set_irq_capture(bltSche.sSlot_tick_irq);
    extern  void blt_ll_set_llmic_status(llmic_ble_sigl_e ble_signal);
    blt_ll_set_llmic_status(BLE_SIGL_NORMAL);
#else
    /* main_loop trigger system timer scheduler start task: call function in irq: blt_ll_irq_startScheduler */
    bltSche.sSlot_tick_irq = clock_time() + 1000 * SYSTEM_TIMER_TICK_1US;
    systimer_set_irq_capture(bltSche.sSlot_tick_irq);
#endif
    blmsParam.sche_run_flag = 1;
    systimer_irq_enable();

    return 1;
}



#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
void blt_ll_procStateChange(void) //This function is called in IRQ, but can write in Flash to save SRAM
{
    int update_en = 0;

    u64 legadv_sche_en = (bltSche.task_mask & TSKMSK_LEG_ADV);
    u64 prichn_scan_sche_en = (bltSche.task_mask & TSKMSK_PRICHN_SCAN);

    if(blmsParam.leg_adv_en && !legadv_sche_en){
    #if (LEG_ADV_EN_MORE_STRATEGY)
        if(blmsParam.cur_slave_num < blmsParam.max_slave_num || blmsParam.legadv_en_strategy)
    #else
        if(blmsParam.cur_slave_num < blmsParam.max_slave_num)
    #endif
        {
            blt_sche_addTaskMask(TSKMSK_LEG_ADV);
            update_en = SLOT_UPDT_ADV_SCAN_STATE_CHANGE;
        }
    }
    else if(!blmsParam.leg_adv_en && legadv_sche_en){
        blt_sche_removeTaskMask(TSKMSK_LEG_ADV);
        #if (BLMS_PM_ENABLE && ACL_SLAVE_PM_LATENCY_EN)
            blmsPm.next_adv_tick = 0;
        #endif
        update_en = SLOT_UPDT_ADV_SCAN_STATE_CHANGE;
    }


#if (LL_ACL_CEN_EN || LL_FEATURE_ENABLE_LE_EXTENDED_SCAN || LL_FEATURE_SUPPORT_LE_LEGACY_SCANNING)
    if( blmsParam.scanInitEn_union.scn_init_en_pack && !prichn_scan_sche_en){
    #if SCAN_EN_MORE_STRATEGY
        if(blmsParam.cur_master_num < blmsParam.max_master_num || bltScn.scan_en_strategy == SCAN_STRATEGY_1)
    #else
        if(blmsParam.cur_master_num < blmsParam.max_master_num)
    #endif
        {
            blt_sche_addTaskMask(TSKMSK_PRICHN_SCAN);
            update_en = SLOT_UPDT_ADV_SCAN_STATE_CHANGE;
        }
    }
    else if( !blmsParam.scanInitEn_union.scn_init_en_pack && prichn_scan_sche_en){
        blt_sche_removeTaskMask(TSKMSK_PRICHN_SCAN);
        update_en = SLOT_UPDT_EXT_SCAN_DISABLE;
    }
#endif /*< #if (LL_ACL_CEN_EN || LL_FEATURE_ENABLE_LE_EXTENDED_SCAN || LL_FEATURE_SUPPORT_LE_LEGACY_SCANNING) */


    #if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING || LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING ||\
         LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC || LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)

        u64 diff_mask = bltSche.task_en  ^ bltSche.task_mask;
        if(diff_mask & (TSKMSK_EXT_ADV_ALL | TSKMSK_PERD_ADV_ALL | TSKMSK_PDA_SYNC_ALL | TSKMSK_PAWRS_SUB_ALL)){
            u64 task_on = bltSche.task_en & (TSKMSK_EXT_ADV_ALL | TSKMSK_PERD_ADV_ALL | TSKMSK_PDA_SYNC_ALL | TSKMSK_PAWRS_SUB_ALL);
            blt_sche_removeTaskMask( (TSKMSK_EXT_ADV_ALL | TSKMSK_PERD_ADV_ALL | TSKMSK_PDA_SYNC_ALL | TSKMSK_PAWRS_SUB_ALL) );
            blt_sche_addTaskMask( task_on );
            update_en = 1;
        }
    #endif


    #if (BLE_LLMIC_CONCURRENT_EN)
        if(bltLlmic.change_sch){
            bltLlmic.change_sch = 0;
            update_en = 1;
        }
    #endif


    if(update_en){
        blt_sche_addUpdate(SLOT_UPDT_ADV_SCAN_STATE_CHANGE);
    }

#if (LL_RSSI_SNIFFER_MASTER_ENABLE)
    if(ll_acl_sniffer_mst_irq_task_cb)
    {
        if(blmsParam.state_chng & STATE_CHANGE_ACL_SNIFM){
            ll_acl_sniffer_mst_irq_task_cb(FLAG_ACL_SNIFFER_SEEK);  // blt_acl_sniffer_mst_irq_task() blt_ll_sniffer_seek_anchor() blt_ll_sniffer_mst_seek_anchor()
        }
    }
#endif

#if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
    if(ll_acl_sniffer_slv_irq_task_cb)
    {
        if(blmsParam.state_chng & STATE_CHANGE_ACL_SNIFS){
            ll_acl_sniffer_slv_irq_task_cb(FLAG_ACL_SNIFFER_SEEK);  // blt_acl_sniffer_slv_irq_task() blt_ll_sniffer_seek_anchor() blt_ll_sniffer_slv_seek_anchor()
        }
    }
#endif

    blmsParam.state_chng = 0;
}








_attribute_ram_code_
void blt_ll_calculate_sSlot_next(u32 next_tick)
{
    #if (SCH_DEBUG_EN)
        if(tick1_exceed_tick2(bltSche.sSlot_tick_irq, next_tick)){
            write_dbg32(0x00018, bltSche.sSlot_tick_irq);
            write_dbg32(0x0001C, next_tick);
            BLMS_ERR_DEBUG(SCH_DEBUG_EN, 0xFF040000);
        }
    #endif

    int sSlot_cost_num = (u32)(next_tick - bltSche.sSlot_tick_irq)*SSLOT_TICK_REVERSE + 1;
    bltSche.sSlot_idx_next = (bltSche.sSlot_idx_irq_real + sSlot_cost_num);
    bltSche.sSlot_tick_next = bltSche.sSlot_tick_irq + sSlot_cost_num*SSLOT_TICK_NUM;
}







//increase priority due to packet loss & previous slot drop ...
_attribute_ram_code_
void blt_ll_incSchedulerTaskPriority(u8 task_offset, int inc)
{
    pri_data_t pri_new = bltPri.pri_now[task_offset] + inc;

    if(pri_new > TASK_PRIORITY_HIGH_THRES){
        pri_new = TASK_PRIORITY_HIGH_THRES;
    }
    else if(pri_new < TASK_PRIORITY_LOW){
        pri_new = TASK_PRIORITY_LOW;
    }

    bltPri.pri_now[task_offset] = pri_new;
}

_attribute_ram_code_
void blt_ll_incSchedulerTaskCalPriority(u8 task_offset, int inc)
{
    //yafei add this limitation.
    if(task_offset >= TSKOFT_EXT_ADV && task_offset < TSKOFT_AUX_ADV + TSKNUM_AUX_ADV) {
        //ExtAdv && AuxAdv task do not change priority. Use fixed priority
        return;
    }

    //qinghua add this limitation.
    if(task_offset >= TSKOFT_BIG_SYNC && task_offset < TSKOFT_BIG_SYNC + TSKNUM_BIG_SYNC) {
        return;
    }

    /* if raw priority bigger than high_thres, do not process */
    if(bltPri.pri_cal[task_offset] <= TASK_PRIORITY_HIGH_THRES){
        pri_data_t pri_new = bltPri.pri_cal[task_offset] + inc;

        if(pri_new > TASK_PRIORITY_HIGH_THRES){
            pri_new = TASK_PRIORITY_HIGH_THRES;
        }
        else if(pri_new < TASK_PRIORITY_LOW){
            pri_new = TASK_PRIORITY_LOW;
        }

        bltPri.pri_cal[task_offset] = pri_new;
    }
}
