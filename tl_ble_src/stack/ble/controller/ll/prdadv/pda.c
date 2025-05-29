/********************************************************************************************************
 * @file    pda.c
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

#if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER || LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC || LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING)


_attribute_ble_data_retention_ _attribute_aligned_(4) st_pda_t *blt_pPda = NULL;


    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER || LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
_attribute_ram_code_ int blt_ll_pdaSync_pawrSync_info_process(sync_info_t *pSyncInf, u8 *pPAwR_syncTimeInfo)
{
    if (pSyncInf->syncPktOffset && pSyncInf->itvl > 5) // >= 7.5mS   PERADV_INTERVAL_7P5MS
    {
        extadv_id_t *cur_pAdv = &blt_pSecChnScn->peerAdv_id;

        pda_cache_t *p_pda_cache_idle   = NULL;
        pda_cache_t *p_pda_cache_oldest = NULL;
        pda_cache_t *pPdA_cache         = NULL;

        bltPdaSync.prdadv_seqnum++;
        u32 oldest_seqnum     = BIT(31);
        int existed_cache_dev = 0;

        for (int i = 0; i < PERDADV_CACHE_NUM; i++) {
            pPdA_cache = (pda_cache_t *)&pdaCache_tbl[i];

            if (pPdA_cache->cach_flag == CACHE_FLAG_OCCUPIED) {
                if (!smemcmp(cur_pAdv, &pPdA_cache->pda_dev_id, sizeof(extadv_id_t)) ||
                    blt_ll_searchAddrInWhiteListTbl(pPdA_cache->pda_dev_id.adrType, pPdA_cache->pda_dev_id.addr)) {
                    existed_cache_dev      = 1;
                    pPdA_cache->seq_number = bltPdaSync.prdadv_seqnum; //update

                    //for PAST rpa used
                    pPdA_cache->record_advA_adrType = blt_pSecChnScn->record_advA_adrType;
                    smemcpy(pPdA_cache->record_advA_addr, blt_pSecChnScn->record_advA_addr, BLE_ADDR_LEN);
                    //my_dump_str_data(0, "record1", blt_pSecChnScn->record_advA_addr, 6);
                    break;
                } else {
                    if (pPdA_cache->seq_number < oldest_seqnum) {
                        oldest_seqnum      = pPdA_cache->seq_number;
                        p_pda_cache_oldest = pPdA_cache;
                    }
                }
            } else if (pPdA_cache->cach_flag == CACHE_FLAG_IDLE) {
                if (!p_pda_cache_idle) {
                    p_pda_cache_idle = pPdA_cache; //use the first cache table
                }
            } else {                               //CACHE_FLAG_SYNCING & CACHE_FLAG_SYNCED
                //my_dump_str_data(DBG_PDA_SYNC_LOGIC, "sync_ing", 0, 0);
                break; //optimize later
            }
        }


        if (existed_cache_dev) {
        } else {
            if (p_pda_cache_idle) {
                pPdA_cache = p_pda_cache_idle;
                //              bltPdaSync.pdA_cacheNum ++;
            } else if (p_pda_cache_oldest) {
                pPdA_cache = p_pda_cache_oldest;
            }

            pPdA_cache->cach_flag = CACHE_FLAG_OCCUPIED;
            smemcpy(&pPdA_cache->pda_dev_id, cur_pAdv, sizeof(extadv_id_t));
            //for PAST rpa used
            pPdA_cache->record_advA_adrType = blt_pSecChnScn->record_advA_adrType;
            smemcpy(pPdA_cache->record_advA_addr, blt_pSecChnScn->record_advA_addr, BLE_ADDR_LEN);
            //my_dump_str_data(0, "record2", blt_pSecChnScn->record_advA_addr, 6);
        }

        /* sync window widen, normal 0us */
        pPdA_cache->syncWwUs           = 0;
        pPdA_cache->prdphy             = blt_pSecChnScn->secchn_phy;
        pPdA_cache->seq_number         = bltPdaSync.prdadv_seqnum; //update
        pPdA_cache->header_tick_backup = bltRxPkt.rx_header_tick;
        smemcpy(&pPdA_cache->sncInf, pSyncInf, sizeof(sync_info_t));

        #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
        pPdA_cache->pawr_acad.num_subevent = 0; //will be used to judge whether is PAwR sync. so every time need to clear its value.
        if (pPAwR_syncTimeInfo != NULL) {
            smemcpy(&pPdA_cache->pawr_acad, &pPAwR_syncTimeInfo[2], sizeof(pawr_acad_t));
            pPdA_cache->pawr_acad_valid = 1;
        } else {
            pPdA_cache->pawr_acad_valid = 0;
        }
        #else
        (void)pPAwR_syncTimeInfo;
        #endif
        //my_dump_str_data(DBG_PDA_SYNC_LOGIC, "new sync_info", &pSyncInf->evtCounter, 2);

        //      u32 distance_us = pPdA_cache->sncInf.syncPktOffset * (pPdA_cache->sncInf.offsetUnit == EXT_ADV_PDU_SYNC_OFFSET_UNITS_300_US ? 300 : 30);
        //      u32 dis_ms = distance_us/1000;
        //      my_dump_str_data(DBG_PDA_SYNC_LOGIC, "dis ms", &dis_ms, 4);


        if (blmsParam.pda_syncing_flg) ///set in blc_ll_periodicAdvertisingCreateSync
        {
            st_pda_sync_t *pPdA_sync = (st_pda_sync_t *)&pdAsync_tbl[bltScn.pda_syncing_idx];
            if (pPdA_sync->sync_state == SYNC_STATE_WAIT_SYNC_INFO) {
                int sync_dev_match = 0;

                if (pPdA_sync->sync_specify) {
                    if (!smemcmp(cur_pAdv, &pPdA_sync->pda_id, sizeof(extadv_id_t))) {
                        sync_dev_match = 1;
                    }
                } else {
                    for (int i = 0; i < bltPdaSync.pdA_list_num; i++) {
                        if (!pdaList_tbl[i].synced_mark && !smemcmp(&pdaList_tbl[i].list_dev_id, cur_pAdv, sizeof(extadv_id_t))) {
                            sync_dev_match = 1;
                            break;
                        }
                    }
                }

                if (sync_dev_match) {
                    pPdA_cache->cach_flag = CACHE_FLAG_SYNCING;
                    //DBG_C HN4_TOGGLE;
                    blt_pda_sync_analyze_prdadv_info(pPdA_sync, pPdA_cache);
                    pPdA_sync->sync_state = SYNC_STATE_SYNC_INFO_MATCH;
                }
            }
        }


        return TRUE; //a valid sync_info stored in cache
    }


    return FALSE;
}
    #endif //#if(LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER || LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)


_attribute_ram_code_ int blt_pda_cal_timing_common(st_pda_t *p)
{
    (void)p; //unused, remove warning

    return 0;
}

_attribute_ram_code_ int blt_pda_start_common_1(void)
{
    /* PHY switch, do not consider S2 */
#if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
    if (ll_phy_switch_cb) {
        ll_phy_switch_cb(blt_pPda->pda_phy, LE_CODED_S8); //rf_ble_switch_phy
    }
    if (bltPHYs.cur_llPhy == BLE_PHY_CODED) {
        rf_trigger_codedPhy_accesscode();
    }
#endif

    //--------------- interval jump process --------------------------------------------------------//
    int inter_jump_num = 0;
    if (1) {
        /* 3125 uS = 625uS *5 = 32*5 sSlot
         * consider slot adjust when low power enable, margin set to 5 slot is more safer, minimum connection interval 7.5mS is 12 Slot,
         * 5 slot margin can handle 3.125 mS timing shit, and maximum 10 slot 6.25mS error no risk for inter_jump_num */
        inter_jump_num = (bltSche.bSlot_idx_irq_real + 5 - blt_pPda->bSlot_mark_prdadv) / blt_pPda->bSlot_prdadv_itvl - 1;

        //my_dump_str_u32s(DBG_PDA_SYNC_LOGIC, "pda jump ", blt_pPda->bSlot_mark_prdadv, bltSche.bSlot_idx_irq_real, blt_pPda->bSlot_prdadv_itvl, inter_jump_num);
        if (inter_jump_num > 0) { //periodic_adv_interval jump happens
            blt_pPda->paEvtCnt += inter_jump_num;
        }
    }
    //--------------- channel map update ------------------------------------------------------------
    //pay attention here, PAD_STX slot may dropped, blt_pPda->paEvtCnt >= blt_pPda->prd_map_inst_next(consider 0xffff->0 problem, (u16).... < 1024 )
    if ((blt_pPda->update_map == PDA_UPDATE_MAP) && (u16)(blt_pPda->paEvtCnt - blt_pPda->prd_map_inst_next) < BIT(10)) {
        blt_pPda->update_map = 0;
        //blt_pPda->update_map = 0;// do this job in API: blt_pda_post_common.
        smemcpy(&blt_pPda->chnParam, &blt_pPda->nextChn, sizeof(struct le_channel_map));
        my_dump_str_data(0, "pad:chm update", &blt_pPda->paEvtCnt, 2);
    }

    u8 paChnIdx = ll_chn_index_calc_cb(&blt_pPda->chnParam.map, blt_pPda->paEvtCnt, blt_pPda->chnIdentifier);
    rf_set_tx_rx_off();
    rf_set_ble_channel(paChnIdx);
    rf_set_ble_access_code((u8 *)&blt_pPda->paAccessAddr);
    rf_set_ble_crc_value(blt_pPda->paCrcInit);

    blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_CUSTOMER_OR_DEFAULT, 0);

    #if (LL_FEATURE_ENABLE_LE_AOA_AOD)
    if (cte_connLess_switchPattern[bltPdaSync.pdA_sync_sel].cte_rx_mode_en) {
        blt_pPdAsync->sync_cte_chnIdx = paChnIdx;
    }
    #endif

    my_dump_str_u32s(DBG_PDA_SYNC_LOGIC, "pda syncing ", blt_pPda->paEvtCnt, paChnIdx, blt_pPda->paAccessAddr, 0);

    return inter_jump_num;
}

_attribute_ram_code_ int blt_pda_start_common_2(void)
{
    //update
    blt_pPda->bSlot_mark_prdadv = bltSche.bSlot_idx_irq_real;

    return 0;
}

_attribute_ram_code_ int blt_pda_post_common(void)
{
    blt_pPda->prdadv_send_cnt++;

    /* switch ACAD before instant-1 */
    if ((blt_pPda->update_map == PDA_UPDATE_MAP) && (u16)(blt_pPda->paEvtCnt + 1 - blt_pPda->prd_map_inst_next) < BIT(10)) {
        ////////////////// Update aux pkt's SyncInfo field //////////////////
        smemcpy(blt_pextadv->auxSyncInfo.chm, blt_pPda->nextChn.chmTbl, 5);     //[0:36]chm, : [37:39]sca
    #if BLMS_PM_ENABLE
        blt_pextadv->auxSyncInfo.chm[4] |= (SCA_MASTER_SLAVE_251_500_PPM << 5); //251PPM - 500PPM
    #else
        blt_pextadv->auxSyncInfo.chm[4] |= (SCA_MASTER_SLAVE_31_50_PPM << 5); //31PPM - 50PPM
    #endif

        u8 acad_used = PERD_ACAD_CHMUPT_DIS | blt_pPerdadv->acad_used;
        my_dump_str_data(0, "pad:chm upt, close chm acad", &acad_used, 1);

        blt_pPerdadv->acad_chaged = 0;
        blt_prdadv_updateAcadPram(blt_pPerdadv, acad_used);
        //Rebuild sch task table ASAP.
        blt_sche_addUpdate(SLOT_UPDT_SLOTTBL_RESCHED);

        my_dump_str_data(0, "   ", &blt_pPda->paEvtCnt, 2);
    }

    return 0;
}

#endif //#if(LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER || LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC || LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING)
