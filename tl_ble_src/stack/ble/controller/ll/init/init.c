/********************************************************************************************************
 * @file    init.c
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


#if (LL_ACL_CEN_EN)


_attribute_ble_data_retention_ u32 blms_timeout_connectDevice = 4000000; //default 4 S


/* define in Flash */
const u32 interMask_tbl[13] = {
    0,                // 0
    INTV_MSK_1_TIME,  // 1
    INTV_MSK_2_TIME,  // 2
    INTV_MSK_3_TIME,  // 3
    INTV_MSK_4_TIME,  // 4
    0,                // 5
    INTV_MSK_6_TIME,  // 6
    0,                // 7
    INTV_MSK_8_TIME,  // 8
    0,                // 9
    0,                // 10
    0,                // 11
    INTV_MSK_12_TIME, // 12
};


_attribute_ble_data_retention_ _attribute_aligned_(4) rf_packet_ll_init_t pkt_init = {
    rf_tx_packet_dma_len(34 + 2), //dma_len: rf_len + 2

    LL_TYPE_CONNECT_REQ, // type
    0, // RFU
    0, // ChSel: only valid in ADV_IND/ADV_DIRECT_IND/CONNECT_IND, other packet set 0'b
    0, // txAddr
    0, // rxAddr

    34, // rf_len: sizeof (rf_packet_ll_init_t) - 6
    {0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5}, // scanA
    {0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5}, // advA
    0xaa5555aa, // access code
    {0x55, 0x55, 0x55}, // crcinit[3]
    BLMS_WINSIZE, // wsize:
    6, // woffset
    0x0020, // interval: 32 * 1.25 ms = 40 ms
    0x0000, // latency
    0x0064, // timeout: 1 second
    {0xff, 0xff, 0xff, 0xff, 0x1f}, // chm[5]
    0xac, // hop: initial channel - 12.  SCA: 31ppm-50ppm
};

void blt_ll_initInitiatingCommon(void)
{
    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_init_t)), init);
    #endif

    ll_prichn_initPkt_cb = blt_prichn_procInitPkt;

    ll_init_mlp_task_cb = blt_init_mainloop_task;
}

int blt_init_mainloop_task(int flag)
{
    if (flag == FLAG_MODULE_MAINLOOP) {
        blt_ll_procInitiateConnectionTimeout();
    } else if (flag == FLAG_MODULE_RESET) {
        blmsParam.create_connection            = 0;
        bltScn.initiate_going                  = 0;
        aclConn_param.tick_connectDevice       = 0;
        blmsParam.scanInitEn_union.leg_init_en = 0;
        blmsParam.scanInitEn_union.ext_init_en = 0;

    #if (CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE)
        aclConn_etbsh.crtConn_cur_cnt = 0;
            //no need clear "re_create_conn_tick" because it is used under condition "tick connectDevice not zero"
    #endif
    }

    return 0;
}

_attribute_ram_code_ bool blt_ll_init_filter(int direct_adv, u8 txAddrType, u8 rxAddrType, u8 *pAdvA, u8 *pTargetA)
{
    do {
        ll_resolv_list_t *pRL_match = NULL;

        u8 rpa_resolve_err     = 0;
        u8 rpa_resolve_success = 0;

    #if (LL_FEATURE_ENABLE_LOCAL_RPA)
        blt_ll_addr_clear_local_rpa_flag();
    #endif
        blt_ll_addr_set_peer_address(0, txAddrType, pAdvA);
    #if (LL_FEATURE_ENABLE_PRIVACY)
        u8 advA_is_rpa = IS_RESOLVABLE_PRIVATE_ADDR(txAddrType, pAdvA);
        if (advA_is_rpa) { //RPA
            //todo: optimize: not all RPA need resolve(resolve need timing and cost AES conflict), to save some time
            if (bltInit.init_specify_peerAdvA_rpa) { //policy = 0, peer_address_type = 2 or 3 in Create Connection Command
                //this case, can use specified RL entry to resolve, save running time, because no need traverse all RL entry
                pRL_match = blt_ll_searchResolvingListEntry(bltInit.creatConCmd_peerAdrType, bltInit.creatConCmd_peerAddr);
                if (pRL_match && blt_ll_resolve_rpa(0, pAdvA, pRL_match)) {
                    /* for this situation:
                         * peer advA have already pass, if use "pRL_match" to find IDA, must be "bltInit.creatConCmd peerAddr"
                         * so do not compare with "bltInit.creatConCmd peerAddr" again.  in later code
                         * when check filter "INITIATE_FP_ADV_SPECIFY", "init_specify peerAdvA_rpa" can directly pass without any check.
                         * but we decide to not save this check for 2 reasons:
                         * 1. here we use "blt_ll_addr set_peer_address" to mark peerUsaRpa and "peer pka_or_ida_addr"
                         *    for later use(in acl connection)
                         * 2. As we have mark "creatConCmd peerAddr" to "peer pka_or_ida_addr", we can also check for
                         *    filter "INITIATE_FP_ADV_SPECIFY", though waste some timing(not too much), but can save RamCode and
                         *    code logic is more clear */
                    rpa_resolve_success = 1;
                    /*use "creatConCmd peerAddr" and "rlIdAddr" both correct, choose any one is OK */
                    //blt_ll_addr_set_peer_address(1, pRL_match->rlIdAddrType, pRL_match->rlIdAddr);
                    //blt_ll_storePeerDeviceRpa(pRL_match, pAdvA);
                } else {
                    rpa_resolve_err = 1;
                }
            } else {
                /* need traverse all RL entry to see if any one match, may cost too much time depending on RL entry number */
                pRL_match = blt_ll_resolve_rpa(0, pAdvA, NULL);
                if (pRL_match) {
                    rpa_resolve_success = 1;
                    //blt_ll_addr_set_peer_address(1, pRL_match->rlIdAddrType, pRL_match->rlIdAddr);
                    //blt_ll_storePeerDeviceRpa(pRL_match, pAdvA);
                } else {
                    if (bltInit.init_fp) { //INITIATE_FP_ADV_WL
                        rpa_resolve_err = 1;
                    } else {               //INITIATE_FP_ADV_SPECIFY
                        //do not set success, either do not set fail, need check later
                    }
                }
            }


            if (rpa_resolve_success) {
                blt_ll_storePeerDeviceRpa(pRL_match, pAdvA);
                blt_ll_addr_set_peer_address(1, pRL_match->rlIdAddrType, pRL_match->rlIdAddr);
                bltInit.pRslvlst_extInit = pRL_match; //for extended INIT only
            }

        } else {                                      //IDA
            /* here "pRL_match" may be used later by initA(RPA) in conn_req,
                 * so must can not be included in "NETWORK_PRIVACY IGNORE_IDA_CHECK" */
            pRL_match = blt_ll_searchResolvingListEntry(txAddrType, pAdvA);

        #if (NETWORK_PRIVACY_IGNORE_IDA_CHECK)
            /* check if network privacy mode ignore IDA exist */
            if (pRL_match && pRL_match->peerIrk_valid) {             //peer device has distributed its IRK
                if (pRL_match->rlPrivMode == NETWORK_PRIVACY_MODE) { //not allowed
                    /* LL/CON/INI/BV-18-C & LL/CON/INI/BV-19-C */
                    my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] leginit, network privacy ignore IDA, stop", 0, 0);
                    break; //stop
                } else {   //DEVICE_PRIVACY_MODE, allowed
                    /* LL/CON/INI/BV-20-C & LL/CON/INI/BV-21-C */
                    my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] leginit, device privacy accept IDA", 0, 0);
                }
            }
        #endif
        }
    #endif


        /* for ADV_IND & ADV_DIRECT_IND, check if advA pass */
        #if(LL_FEATURE_ENABLE_DECISION_BASED_ADVERTISING_FILTER)
        if ( bltInit.init_fp == EXT_INIT_FP_ADV_IGNORED_DECISION_AND_WL4OTHERPDU || bltInit.init_fp== EXT_INIT_FP_ADV_WL4ALLPDU ||\
           (blt_pSecChnScn->priAdvType != LL_TYPE_ADV_DECISION_IND && bltInit.init_fp == EXT_INIT_FP_ADV_DECISION_AND_WL4OTHERPDU) )
        #else
        if (bltInit.init_fp) //INITIATE_FP_ADV_WL, , filter needed, check accept list
        #endif
        {
            /* 1. RPA can not resolve to a IDA, no change to use AL(accept list), fail
             * 2. accept list filter fail */
            if (rpa_resolve_err || !blt_ll_searchAddrInWhiteListTbl(bltAddr.peer_pka_or_ida_type, bltAddr.peer_pka_or_ida_addr)) {
    #if (DBG_PRVC_INIT_EN)
                if (rpa_resolve_err) {
                    my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] peer advA RPA resolve ERR, stop", bltAddr.peer_pka_or_ida_addr, 6);
                } else {
                    my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] peer advA not in AL, stop", bltAddr.peer_pka_or_ida_addr, 6);
                }
    #endif

                break;
            }
        } else { //INITIATE_FP_ADV_SPECIFY, check if specified device
            if (rpa_resolve_err ||
                smemcmp(bltAddr.peer_pka_or_ida_addr, bltInit.creatConCmd_peerAddr, BLE_ADDR_LEN) ||
                bltAddr.peer_pka_or_ida_type != bltInit.creatConCmd_peerAdrType) {
    #if (DBG_PRVC_INIT_EN)
                if (rpa_resolve_err) {
                    my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] peer advA RPA resolve ERR, stop", bltAddr.peer_pka_or_ida_addr, 6);
                } else {
                    my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] peer advA not specified, stop", bltAddr.peer_pka_or_ida_addr, 6);
                }
    #endif

                break;
            } else {
                my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] peer advA match specified", bltAddr.peer_pka_or_ida_addr, 6);
            }
        }


    #if (LL_FEATURE_ENABLE_LOCAL_RPA)
        int local_initA_use_rpa = bltInit.init_ownAddr_rpa && pRL_match && pRL_match->localIrk_valid;
    #endif
        if (direct_adv) {         /* direct ADV "ADV_DIRECT_IND", check if targetA address to local device */
    #if (LL_FEATURE_ENABLE_LOCAL_RPA)
            int targetA_is_rpa = IS_RESOLVABLE_PRIVATE_ADDR(rxAddrType, pTargetA);
            if (targetA_is_rpa) { //RPA
                /* attention, different from ADV: here must use "pRL_match" locate by peer advA !!! */
                if (pRL_match && blt_ll_resolve_rpa(1, pTargetA, pRL_match)) {
                    //pass
                    my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] ADV_DIRECT_IND targetA RPA resolve OK, match", pTargetA, 6);
                } else {
                    my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] ADV_DIRECT_IND targetA RPA resolve ERR, stop", pTargetA, 6);
                    break;
                }
            } else {                       //IDA

                if (local_initA_use_rpa) { //special for INIT: local initA of conn_req use RPA, but peer advA is IDA, expose local device privacy !!!
                    my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] ADV_DIRECT_IND initA IDA expose local privacy, stop", 0, 0);
                    break;
                } else
    #endif
                {
                    if (smemcmp(pTargetA, bltInit.init_mac_addr, BLE_ADDR_LEN) || rxAddrType != bltInit.init_mac_type) {
                        my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] ADV_DIRECT_IND targetA IDA not match, stop", pTargetA, 6);
                        break;
                    } else {
                        my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] ADV_DIRECT_IND targetA IDA match", pTargetA, 6);
                    }
                }
    #if (LL_FEATURE_ENABLE_LOCAL_RPA)
            }
    #endif
        }


        pkt_init.rxAddr = txAddrType;
        smemcpy(pkt_init.advA, pAdvA, BLE_ADDR_LEN);

    #if (LL_FEATURE_ENABLE_LOCAL_RPA)
        if (local_initA_use_rpa) {
            pkt_init.txAddr = BLE_ADDR_RANDOM;
            smemcpy(pkt_init.initA, pRL_match->rlLocalRpa, BLE_ADDR_LEN);
            blt_ll_addr_mark_local_rpa(pRL_match); //important mark
            blt_ll_resolvSetRpaInUse(pRL_match);   //important mark
        } else
    #endif
        {
            pkt_init.txAddr = bltInit.init_mac_type;
            smemcpy(pkt_init.initA, bltInit.init_mac_addr, BLE_ADDR_LEN);
        }


        if (blt_ll_isRepeatedAclConnDevice(ACL_CONN_IDX_CEN0, blmsParam.max_master_num)) {
            my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] repeated device with other ACL master, stop", bltAddr.peer_pka_or_ida_addr, 6);
            break;
        }

        return TRUE;

    } while (0);


    return FALSE;
}

ble_sts_t blt_ll_createConnCommon(init_fp_t initiator_fp, own_addr_type_t ownAdrType, u8 peerAdrType, u8 *peerAddr)
{
    #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
    if (ll_acl_sniffer_mst_mlp_task_cb) {
        return LL_ERR_CURRENT_STATE_NOT_SUPPORTED_THIS_CMD;
    }
    #endif

    #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
    if (ll_acl_sniffer_slv_mlp_task_cb) {
        return LL_ERR_CURRENT_STATE_NOT_SUPPORTED_THIS_CMD;
    }
    #endif


    /* core_5.3, common error for HCI_LE_Create_Connection & LE Extended Create Connection

    If the Host issues this command when another HCI_LE_Create_Connection
    command is pending in the Controller, the Controller shall return the error code
    Command Disallowed (0x0C).                                                                  Done !!!
    If the Host issues this command when another HCI_LE_Extended_Create_-
    Connection command is pending in the Controller, the Controller shall return
    the error code Command Disallowed (0x0C).                                                   Done !!!


    If the Own_Address_Type parameter is set to 0x00 and the device does not
    have a public address, the Controller should return an error code which should
    be Invalid HCI Command Parameters (0x12).                                                   Ignore, controller always have public address

    If the Own_Address_Type parameter is set to 0x01 and the random address for
    the device has not been initialized using the HCI_LE_Set_Random_Address
    command, the Controller shall return the error code Invalid HCI Command
    Parameters (0x12).                                                                          Done !!!

    If the Own_Address_Type parameter is set to 0x02, the Initiator_Filter_Policy
    parameter is set to 0x00, the Controller's resolving list did not contain a
    matching entry, and the device does not have a public address, the Controller
    should return an error code which should be Invalid HCI Command Parameters
    (0x12).                                                                                     Ignore, controller always have public address

    If the Own_Address_Type parameter is set to 0x02, the Initiator_Filter_Policy
    parameter is set to 0x01, and the device does not have a public address, the
    Controller should return an error code which should be Invalid HCI Command
    Parameters (0x12).                                                                          Ignore, controller always have public address

    If the Own_Address_Type parameter is set to 0x03, the Initiator_Filter_Policy
    parameter is set to 0x00, the controller's resolving list did not contain a
    matching entry, and the random address for the device has not been initialized
    using the HCI_LE_Set_Random_Address command, the Controller shall return
    the error code Invalid HCI Command Parameters (0x12).

    If the Own_Address_Type parameter is set to 0x03, the Initiator_Filter_Policy
    parameter is set to 0x01, and the random address for the device has not been
    initialized using the HCI_LE_Set_Random_Address command, the Controller
    shall return the error code Invalid HCI Command Parameters (0x12).
    */


    #if (CENTRAL_CONNECT_PERIPHR_MAC_FILTER_EN)
    if (filter_mac_enable && (initiator_fp == INITIATE_FP_ADV_SPECIFY) && (smemcmp(peerAddr + 3, filter_mac_address + 3, 3) && smemcmp(peerAddr + 2, filter_mac_address + 2, 3))) {
        //my_dump_str_data(0,"mac drop", pa->mac, BLE_ADDR_LEN);
        return LL_ERR_INVALID_PARAMETER; //no connect
    }
    #endif


    if (blmsParam.create_connection) { //previous create command is pending in the Controller
        return HCI_ERR_CMD_DISALLOWED;
    }


    if (ownAdrType == OWN_ADDRESS_RESOLVE_PRIVATE_PUBLIC || ownAdrType == OWN_ADDRESS_RESOLVE_PRIVATE_RANDOM) {
    #if (LL_FEATURE_ENABLE_PRIVACY)

    #else
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    #endif
    } else if (ownAdrType == OWN_ADDRESS_RANDOM) {
        /*
        If the Own_Address_Type parameter is set to 0x01 and the random address for
        the device has not been initialized, the Controller shall return the error code
        Invalid HCI Command Parameters (0x12).
        */
        if (!(blmsParam.hci_cmd_mask & SET_RANDOM_ADDR_CMD_MASK)) {
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }
    }


    /****************************************************************************************************************************/
    int return_status = BLE_SUCCESS;

    u32 r = irq_disable();


    if (blmsParam.connUpdHighAuthorityDis && (blmsParam.cur_master_num >= blmsParam.max_master_num || blmsParam.pda_syncing_flg)) {
        return_status = HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    } else if (blmsParam.new_conn_forbidden || blmsParam.newConn_forbidden_master || blmsParam.cur_master_num >= blmsParam.max_master_num || blmsParam.pda_syncing_flg) {
        return_status = HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    } else {
        //Run #1384 - /LL/CON/INI/BI-03-C  [Duplicate Connection Request]
        if (initiator_fp == INITIATE_FP_ADV_SPECIFY) {
            u8 peer_type = (peerAdrType & ~PEERATYPE_IDENTITY_MASK); //2->0, 3->1
            /* "blt_ll_addr set_peer_address" is used in IRQ
             * special use here in mainLoop, temp set "peer_pka_or_ida_addr" to use "blt_ll isRepeatedAclConnDevice" */
            blt_ll_addr_set_peer_address(0, peer_type, peerAddr);
            if (blt_ll_isRepeatedAclConnDevice(ACL_CONN_IDX_CEN0, blmsParam.max_master_num)) {
                return_status = HCI_ERR_CONN_ALREADY_EXISTS;
            }
        }
    }

    irq_restore(r);

    if (return_status != BLE_SUCCESS) {
        return return_status;
    }


    /****************************************************************************************************************************/
    bltInit.own_addr_type    = ownAdrType;
    bltInit.init_ownAddr_rpa = (ownAdrType & OWN_ADDRESS_TYPE_RPA_MASK);

    if (ownAdrType & OWN_ADDRESS_TYPE_RANDOM_MASK) {
        bltInit.init_mac_type = BLE_ADDR_RANDOM;
        smemcpy(bltInit.init_mac_addr, bltMac.macAddress_random, BLE_ADDR_LEN);
    } else {
        bltInit.init_mac_type = BLE_ADDR_PUBLIC;
        smemcpy(bltInit.init_mac_addr, bltMac.macAddress_public, BLE_ADDR_LEN);
    }

    /*If the Own_Address_Type parameter is set to 0x01 and the random address for
    the device has not been initialized, the Controller shall return the error code
    Invalid HCI Command Parameters (0x12). */
    //Run #1479 - /HCI/CCO/BI-53-C  [Reject Invalid Create Connection Command, 0x03, 0x01 (Own Address = 0x03, Filter = 0x01)]
    if (bltInit.init_mac_type == BLE_ADDR_RANDOM && !(blmsParam.hci_cmd_mask & SET_RANDOM_ADDR_CMD_MASK)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    pkt_init.txAddr = bltInit.init_mac_type;
    smemcpy(pkt_init.initA, bltInit.init_mac_addr, BLE_ADDR_LEN);


    bltInit.init_fp                   = (u8)initiator_fp;
    bltInit.init_specify_peerAdvA_rpa = 0;
    if (initiator_fp == INITIATE_FP_ADV_SPECIFY) {
        bltInit.init_specify_peerAdvA_rpa = (peerAdrType & PEERATYPE_IDENTITY_MASK) ? 1 : 0;
    }

    bltInit.creatConCmd_peerAdrType = (peerAdrType & ~PEERATYPE_IDENTITY_MASK); //2->0, 3->1
    smemcpy(bltInit.creatConCmd_peerAddr, peerAddr, BLE_ADDR_LEN);


    #if (DBG_PRVC_INIT_EN)
    if (initiator_fp == INITIATE_FP_ADV_SPECIFY) {
        my_dump_str_u8s(DBG_PRVC_INIT_EN, "[PRV][INI] leginit SPF: Own_addr_type, peerAdrType", ownAdrType, peerAdrType, 0, 0);
        my_dump_str_data(DBG_PRVC_INIT_EN, "[PRV][INI] peerAddr", peerAddr, 6);
    } else {
        my_dump_str_u8s(DBG_PRVC_INIT_EN, "[PRV][INI] leginit AL: Own_addr_type", ownAdrType, 0, 0, 0);
    }
    #endif


    #if (ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN)
    if (aclMas_param.bSlotDurn_diffMod) {
        st_ll_conn_t *pAclConn;
        //st_llm_conn_t *pAclCen;
        for (int i = ACL_CONN_IDX_CEN0; i < blmsParam.max_master_num; i++) {
            pAclConn = (st_ll_conn_t *)&blms[i];
            //pAclCen = (st_llm_conn_t *) &blmsMaster[i];
            if (pAclConn->connState && pAclConn->acl_conIndex == bltInit.aclc_idx_init) {
                my_dump_str_data(DBG_CUSTOM_ACLC_TIMING, "[CUS CEN] current index is in connection state", &bltInit.aclc_idx_init, 1);
                return HCI_ERR_CMD_DISALLOWED;
            }
        }
    }
    #endif


    /* When code running here(BLE_SUCCESS will returned), initiation will start, to make initiation timing efficient,
     * any new slave not allowed to create*/
    r = irq_disable();
    if (1) { //return_status == BLE_SUCCESS
        if (IS_LEGACY_SCAN_VALID) {
            blmsParam.scanInitEn_union.leg_init_en = 1;
        } else {
            blmsParam.scanInitEn_union.ext_init_en = 1;
        }
        blmsParam.newConn_forbidden_slave = 1; //to guarantee no chance ADV->slave
    }
    irq_restore(r);


    #if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
    pkt_init.chan_sel = local_chsel;
    #endif


    /* generate access_code & crc_init in advance, to save time */
    #if (BQB_TSWR_LL_CON_INI_BV_27_C)
    u32 rand_aa = blt_ll_connCalcAccessAddr_v1();
    #else
    u32 rand_aa = blt_ll_connCalcAccessAddr_v2();
    #endif

    pkt_init.access_code = rand_aa;
    pkt_init.crcinit[0]  = rand_aa ^ 0;
    pkt_init.crcinit[1]  = (rand_aa >> 8) ^ 0xaa;
    pkt_init.crcinit[2]  = (rand_aa >> 16) ^ 0x55;

    //Channel map initialize
    smemcpy(pkt_init.chm, blmhostChnClassUpt.gLlChannelMap, 5);

    u8 tmpHop = (rand_aa & 0x07) + 5;               //hop: 5~12

    #if (BLMS_PM_ENABLE)
    if (blmsPm.sleep_mask & PM_SLEEP_ACL_CENTRAL) { //Known issue: not very strict
        pkt_init.hop = (SCA_MASTER_SLAVE_251_500_PPM << 5) | tmpHop;
    } else
    #endif
    {
        pkt_init.hop = (SCA_MASTER_SLAVE_21_30_PPM << 5) | tmpHop;
    }

    pkt_init.latency = 0; //do not process none zero latency now


    #if (CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE)
    if (aclConn_etbsh.crtConn_retry_num) {
        static u32 mark_create_conn_tick;
        if (aclConn_etbsh.crtConn_cur_cnt == 1) {
            mark_create_conn_tick = aclConn_param.tick_connectDevice = clock_time() & 0xFFFFFFFE;
        } else {
            aclConn_param.tick_connectDevice = mark_create_conn_tick;
        }
    } else
    #endif
    {
        /* BIT(0) is 0: special use. Though some MCU timer tick BIT<2..0> is 0(Kite/Vulture),
         * but can not guarantee all MCU is this rule, manual clear BIT<0> is safe. */
        aclConn_param.tick_connectDevice = clock_time() & 0xFFFFFFFE;
    }


    #if (ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN)
    if (aclMas_param.bSlotDurn_diffMod) {
        aclMas_param.aclc_idx_conn        = bltInit.aclc_idx_init;
        aclMas_param.cur_timPosn_diffMod  = aclMas_param.timposn_idx[aclMas_param.aclc_idx_conn];
        aclMas_param.cur_bslotOft_diffMod = aclMas_param.bSlot_offset_pos0[aclMas_param.cur_timPosn_diffMod];

        my_dump_str_u8s(DBG_CUSTOM_ACLC_TIMING, "[CUS CEN] create connection", aclMas_param.aclc_idx_conn, aclMas_param.cur_timPosn_diffMod, aclMas_param.bSlot_number[aclMas_param.cur_timPosn_diffMod], aclMas_param.cur_bslotOft_diffMod);
    }
    #endif


    return BLE_SUCCESS;
}

_attribute_ram_code_ int blt_prichn_procInitPkt(u8 *raw_pkt)
{
    if (blmsParam.new_conn_forbidden) {
        //do not initiate connection when peripheral is sync_ing
        return 0;
    } else {
        /* prepare TX FSM quickly due to 150uS urgent timing */
        rf_ble_tx_on(); /* should set STX schedule timing ASAP */

        /* For CSEM IP, need special process to disable RX continue mode, carefully! */
        rf_ble_csem_close_rx_continue_mode();
        /* Here add reset baseband for CSEM IP to keep RF mode safe, but add some delay before triggering STX mode, carefully! */
        HAL_CSEM_IP_RESET_BASEBAND;

        blt_quick_tx_prepare(FSM_STX, &pkt_init, raw_pkt[DMA_RFRX_OFFSET_RFLEN]);

        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_TX_ON);
        }

        return blt_legacy_initiate_process(raw_pkt);
    }
}

void blc_ll_disableConnUpdHighAuthority(void)
{
    blmsParam.connUpdHighAuthorityDis = 1;
}

_attribute_noinline_ void blt_ll_procInitiateConnectionTimeout(void)
{
    if (aclConn_param.tick_connectDevice == 1) {
        aclConn_param.tick_connectDevice = 0;

        if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_ENHANCED_CONNECTION_COMPLETE_V2) {
            hci_le_enhancedConnectionComplete_evt_v2(HCI_ERR_UNKNOWN_CONN_ID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFFFF);
        } else if (hci_le_eventMask & HCI_LE_EVT_MASK_ENHANCED_CONNECTION_COMPLETE) {
            hci_le_enhancedConnectionComplete_evt(HCI_ERR_UNKNOWN_CONN_ID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        }
        /*If the cancellation was successful then, after the HCI_Command_Complete
          event for the HCI_LE_Create_Connection_Cancel command, either an LE
          Connection Complete or an HCI_LE_Enhanced_Connection_Complete event
          shall be generated. In either case, the event shall be sent with the error code
          Unknown Connection Identifier (0x02). */
        else if (hci_le_eventMask & HCI_LE_EVT_MASK_CONNECTION_COMPLETE) {
            u8                              evt_buffer[sizeof(hci_le_connectionCompleteEvt_t)];
            hci_le_connectionCompleteEvt_t *pComp = (hci_le_connectionCompleteEvt_t *)evt_buffer;

            pComp->subEventCode = HCI_SUB_EVT_LE_CONNECTION_COMPLETE; // sub code
            pComp->status       = HCI_ERR_UNKNOWN_CONN_ID;
            //rest parameters no need

            blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, evt_buffer, sizeof(hci_le_connectionCompleteEvt_t)); /////app_controller_event_callback
        }
    } else if (clock_time_exceed(aclConn_param.tick_connectDevice, blms_timeout_connectDevice)) {
        aclConn_param.tick_connectDevice = 0;

        int create_conn_fail = 0;

        if (blmsParam.create_connection == CONNECT_REQ_GOING) {
            u32 r = irq_disable();
            if (blmsParam.create_connection == CONNECT_REQ_GOING) {
                blmsParam.create_connection = 0; // create connection command auto disable
                bltScn.initiate_going       = 0;

                //my_dump_str_data(APP_DUMP_DBG_INITIATE_EN,"create conn timeout", 0, 0);

                blmsParam.scanInitEn_union.ext_init_en = 0;
                blmsParam.scanInitEn_union.leg_init_en = 0;
                blmsParam.state_chng |= STATE_CHANGE_INIT;

                create_conn_fail = 1;
            }
            irq_restore(r);
        }
    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
        else if (blmsParam.create_connection == CONNECT_REQ_FOR_PAWR) {
            u32 r = irq_disable();
            if (blmsParam.create_connection == CONNECT_REQ_FOR_PAWR) {
                blmsParam.create_connection = 0; // create connection command auto disable
            }
            irq_restore(r);
        }
    #endif

    #if (CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE)
        if (create_conn_fail) {
            if (aclConn_etbsh.cusConnEtbsh_en) {
                hci_tlk_createConnectionFail_evt(INIT_TIMEOUT, aclConn_etbsh.crtConn_cur_cnt);
                aclConn_etbsh.crtConn_cur_cnt     = 0;
                aclConn_etbsh.re_create_conn_tick = 0;
            }
        }
    #else
        (void)create_conn_fail; //remove warning
    #endif

    } else {
    #if (CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE)
        if (aclConn_etbsh.re_create_conn_tick && clock_time_exceed(aclConn_etbsh.re_create_conn_tick, 100)) {
            ll_createConn_t *pCmdParam = (ll_createConn_t *)&aclConn_etbsh.crtConn_buf;

            aclConn_etbsh.crtConn_cur_cnt++;
            if (BLE_SUCCESS == blt_ll_createConnection(pCmdParam->scan_inter, pCmdParam->scan_wind, pCmdParam->fp, pCmdParam->peerAddr_type, pCmdParam->peer_addr, pCmdParam->ownAddr_type, pCmdParam->conn_min, pCmdParam->conn_max, 0, pCmdParam->timeout, 0, 0)) {
            } else {
                if (aclConn_etbsh.crtConn_cur_cnt == (aclConn_etbsh.crtConn_retry_num + 1)) { //finish
                    hci_tlk_createConnectionFail_evt(CONNECT_FAIL, aclConn_etbsh.crtConn_cur_cnt);
                    aclConn_etbsh.crtConn_cur_cnt     = 0;
                    aclConn_etbsh.re_create_conn_tick = 0;
                } else { //will try again
                    aclConn_etbsh.re_create_conn_tick = clock_time() | 1;
                }
            }
        }
    #endif
    }
}

/*
 * return value can only be 1/2/3/4/6/8/12/24
 */
//TODO: improve for better

int blt_init_calculateMasterIntervalMultiplier(u16 master_connInter, u16 conn_min, u16 conn_max)
{
    if (conn_max < (master_connInter * 3 / 2) || (conn_min <= master_connInter && conn_max < master_connInter * 2)) {
        return 1;
    } else if (conn_max < (master_connInter * 5 / 2) || (conn_min <= master_connInter * 2 && conn_max < master_connInter * 3)) {
        return 2;
    } else if (conn_max < (master_connInter * 7 / 2) || (conn_min <= master_connInter * 3 && conn_max < master_connInter * 4)) {
        return 3;
    } else if (conn_max < (master_connInter * 5) || (conn_min <= master_connInter * 4 && conn_max < master_connInter * 6)) {
        return 4;
    } else if (conn_max < (master_connInter * 7) || (conn_min <= master_connInter * 6 && conn_max < master_connInter * 8)) {
        return 6;
    } else if (conn_max < (master_connInter * 10) || (conn_min <= master_connInter * 8 && conn_max < master_connInter * 12)) {
        return 8;
    }
    /* conn interval max value 4S: 3200,   65536/3200 = 20, so *24 change to u32 */
    else if (conn_max < (master_connInter * 18) || (conn_min <= master_connInter * 12 && conn_max < (u32)(master_connInter * 24))) {
        return 12;
    } else {
        return 24;
    }
}


    #if (CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE)
ble_sts_t blc_ll_setCreateConnectionRetryNumber(u8 number)
{
    if (aclConn_etbsh.cusConnEtbsh_en && number < 6) {
        aclConn_etbsh.crtConn_retry_num = number;

        return BLE_SUCCESS;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }
}
    #endif

/**
 * @brief      This function is used to cancel the HCI_LE_Create_Connection or HCI_LE_Extended_Create_Connection commands.
 *             This command shall only be issued after the HCI_LE_Create_Connection or HCI_LE_Extended_Create_Connection commands have been issued.
 * @param      none
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_ll_createConnectionCancel(void)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Create_Connection_Cancel", 0, 0);

    ble_sts_t ret_status = BLE_SUCCESS;

    u32 r = irq_disable();

    if (blmsParam.create_connection) // CONNECT_REQ_LEG_PENDING or CONNECT_REQ_EXT_PENDING or CONNECT_REQ_GOING
    {
        blmsParam.create_connection = 0;
        bltScn.initiate_going       = 0;

        blmsParam.scanInitEn_union.ext_init_en = 0;
        blmsParam.scanInitEn_union.leg_init_en = 0;
        blmsParam.state_chng |= STATE_CHANGE_INIT;
        /* 1: special use, for connection cancel success event, cause here can not send this event:
         * command complete for connection cancel CMD should sent back first*/
        aclConn_param.tick_connectDevice = 1;
        ret_status                       = BLE_SUCCESS;
    } else {
        /* If the HCI_LE_Create_Connection_Cancel command is sent to the Controller
            without a preceding LE_Create_Connection or
            HCI_LE_Extended_Create_Connection command, the Controller shall return
            an HCI_Command_Complete event with the error code Command Disallowed
            (0x0C). */
        ret_status = HCI_ERR_CMD_DISALLOWED;
    }

    irq_restore(r);


    return ret_status;
}

/**
 * @brief      This function is used to set the timeout of ACL connection establishment
 * @param[in]  timeout_ms - timeout of ACL connection establishment, unit: mS. If not set, default value is 4000 mS
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_ll_setCreateConnectionTimeout(u32 timeout_ms)
{
    blms_timeout_connectDevice = timeout_ms * 1000;
    return BLE_SUCCESS;
}

bool blc_ll_isInitiationBusy(void)
{
    //do not consider IRQ disable and restore here, though we know "create connection" may changed in IRQ
    return blmsParam.create_connection ? TRUE : FALSE;
}


#else //else of LL_ACL_CEN_EN

ble_sts_t blc_ll_createConnectionCancel(void)
{
    return HCI_ERR_CMD_DISALLOWED;
}

ble_sts_t blc_ll_setCreateConnectionTimeout(u32 timeout_ms)
{
    (void)timeout_ms;
    return HCI_ERR_CMD_DISALLOWED;
}


#endif
