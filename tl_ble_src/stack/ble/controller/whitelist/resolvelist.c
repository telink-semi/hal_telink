/********************************************************************************************************
 * @file    resolvelist.c
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


#if (LL_FEATURE_ENABLE_PRIVACY)


//TODO: RPA calculate will use AES HW module in IRQ, may conflict with AES in main_loop


_attribute_ble_data_retention_ ll_ResolvingListTbl_t blRslvLst;

_attribute_ble_data_retention_ u32 ll_resolvRpaTmrTick  = 0;
_attribute_ble_data_retention_ u32 rpaTmrCntBased1sUnit = 0;

    /*
 * brief : RPA generation & refresh generation strategy
 *
 * consider issue:
 *  1. ADV/SCAN_REQ/CONNECT_REQ are send in IRQ, generate/refresh when using it in IRQ is not good,
 *     cost too much IRQ time, re_use AES risk
 *  2. if generate and refresh in mainLoop, when BLE IRQ task need using RPA, maybe no RPA generated due to IRQ timing
 *     earlier than mainLoop refreshing.
 *  3. for all resolving list entry, we can not know which entry is used(only advertising can know,
 *     scanning & initiating need see peer device address), if refresh RPA for all entry all times,
 *     may be a wasting of time and resource.
 *
 *  final solution:
 *  1. generate RPA for valid localIRK & peerIRK whenever adding a device entry to resolving list,
 *     to make sure that when BLE IRK task running, RPA can be used.
 *  2. when a resolving list entry is first time used, use a variable to mark it, update the RPA timer start.
 *     only refresh RPA for entry which have this mark. When entry removed, clear the mark
 *
 */


    /* We know that local RPA should refresh when timeout.
 * When Advertising, should targetA(initA) of ADV_DIRECT_IND & AUX_ADV_IND refresh at timeout point ?
 * SiHui & YaFei think that no need.
 * But EBQ test_case "LL/SEC/BV-13-C" shows that if targetA not refresh, error. So we must refresh targetA too.
 * refer EBQ test_log "SEC_ADV_BV_13_targetA_RPA_no_refresh_fail"
 */
    #define PEER_GENERATED_RPA_REFRESH_WHEN_TIMEOUT 1 //must be 1

void blt_ll_initResolvingList(void)
{
    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_resolv_list_t)), resolvlist);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_ResolvingListTbl_t)), resolvlist);
    #endif


    blRslvLst.addrRlEn = 0; //disable private address resolution

    /* default: 15min, 900 S */
    blRslvLst.rl_timeout_100S_num = 9;
    blRslvLst.rl_timeout_us       = 0;

    //blRslvLst.rpa_use_matrix = 0;  //default value is 0
    blRslvLst.rlSize = MAX_RESOLVING_LIST_SIZE;

    ll_resolv_list_t *pRL;
    for (int i = 0; i < blRslvLst.rlSize; i++) {
        pRL = (ll_resolv_list_t *)&blRslvLst.rlList[i];

        pRL->rl_idx = i;
        //pRL->dev_iden_valid = 0; //default value is 0
        //pRL->rpaUsed = 0; //default value is 0
    }
}

_attribute_no_inline_ void blt_ll_resolvRefreshRpa(ll_resolv_list_t *pRL)
{
    if (pRL && pRL->dev_iden_valid) {
        my_dump_str_data(DBG_PRVC_RL_EN, "[PRV][RL] RPA refresh", &pRL->rl_idx, 1);
        //timer reset
        pRL->rpa_timeout_100S_cnt = blRslvLst.rl_timeout_100S_num;
        pRL->rpa_start_tick       = clock_time();

        if (pRL->localIrk_valid) {
            blt_ll_resolvGenerateRpa(pRL->localIRK, pRL->rlLocalRpa);
        }

    #if (PEER_GENERATED_RPA_REFRESH_WHEN_TIMEOUT) //depends on macro
        if (pRL->peerIrk_valid) {
            blt_ll_resolvGenerateRpa(pRL->peerIRK, pRL->genrt_peerRpa);
        }
    #endif
    }
}

_attribute_no_inline_ int blt_ll_resolvRpaTimeoutLoop(void)
{
    ll_resolv_list_t *pRL;
    for (int i = 0; i < blRslvLst.rlSize; i++) {
        pRL = (ll_resolv_list_t *)&blRslvLst.rlList[i];

        if (pRL->rpaUsed) {
            if (pRL->rpa_timeout_100S_cnt && clock_time_exceed(pRL->rpa_start_tick, 100 * 1000 * 1000)) {
                pRL->rpa_timeout_100S_cnt--;
                pRL->rpa_start_tick = clock_time();
            }

            if (!pRL->rpa_timeout_100S_cnt && clock_time_exceed(pRL->rpa_start_tick, blRslvLst.rl_timeout_us)) {
                blt_ll_resolvRefreshRpa(pRL);
            }
        }
    }


    return 0;
}

/**
 * @brief      This function is used to add a device to resolving list
 * @param[in]  peerIdAddrType - Peer_Identity_Address_Type
 * @param[in]  peerIdAddr - Peer_Identity_Address
 * @param[in]  peer_irk - peer IRK
 * @param[in]  local_irk - local IRK
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_ll_addDeviceToResolvingList(ida_type_t peerIdAddrType, u8 *peerIdAddr, u8 *peer_irk, u8 *local_irk)
{
    /* Core_5.3
    When a Controller cannot add a device to the list because there is no space
    available, it shall return the error code Memory Capacity Exceeded (0x07).           Done !!!

    If an entry already exists in the resolving list with the same four parameter
    values, the Controller shall either reject the command or not add the device to
    the resolving list again and return success. If the command is rejected then the
    error code Invalid HCI Command Parameters (0x12) should be used.                     Done !!!

    If there is an existing entry in the resolving list with the same
    Peer_Identity_Address and Peer_Identity_Address_Type, or with the same
    Peer_IRK, the Controller should return the error code Invalid HCI Command
    Parameters (0x12).                                                                   Done !!!
    */

    if (blRslvLst.addrRlEn && !blt_ll_isResolvingListCommandAllowed()) {
        my_dump_str_data(DBG_PRVC_RL_EN, "task busy, not allowed add", 0, 0);
        return HCI_ERR_CMD_DISALLOWED;
    }

    if (blRslvLst.rlCnt >= blRslvLst.rlSize) { //there is no space available
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }

    if (peerIdAddrType > RANDOM_IDENTITY_ADDRESS) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    ll_resolv_list_t *pRL = NULL;

    for (int i = 0; i < blRslvLst.rlSize; i++) {
        pRL = &blRslvLst.rlList[i];
        if (pRL->dev_iden_valid) {
            if ((pRL->rlIdAddrType == peerIdAddrType && (!smemcmp(&pRL->rlIdAddr, peerIdAddr, BLE_ADDR_LEN))) &&
                !smemcmp(&pRL->peerIRK, peer_irk, 16) && !smemcmp(&pRL->localIRK, local_irk, 16)) {
                my_dump_str_data(DBG_PRVC_RL_EN, "[PRV][RL] Address same or IRK same with existed", 0, 0);
                return HCI_ERR_INVALID_HCI_CMD_PARAMS;
            }
        }
    }


    pRL = NULL;
    int i;
    for (i = 0; i < blRslvLst.rlSize; i++) {
        pRL = &blRslvLst.rlList[i];
        if (!pRL->dev_iden_valid) {
            break;
        }
    }

    #if (DBG_PRVC_RL_EN)
    if (pRL == NULL || i == blRslvLst.rlSize) {
        BLMS_ERR_DEBUG(DBG_PRVC_RL_EN, 0x11010000 | blRslvLst.rlSize);
    }
    #endif


    pRL->rlIdAddrType = peerIdAddrType;
    smemcpy(pRL->rlIdAddr, peerIdAddr, BLE_ADDR_LEN);

    u8 const_u8_16_zero[16] = {0};


    /* RPA generation & refresh generation strategy
     * generate RPA for valid localIRK & peerIRK whenever adding a device entry to resolving list */

    /* special design: if pointer is ZERO, has the same effect as 16 zero key */
    /* special design: if pointer is ZERO, has the same effect as 16 zero key */
    if (local_irk && smemcmp(local_irk, const_u8_16_zero, 16)) {
        //my_dump_str_data(DBG_PRVC_RL_EN, "local irk nonzero", 0, 0);
        pRL->localIrk_valid = 1;
        smemcpy(pRL->localIRK, local_irk, 16);
        blt_ll_resolvGenerateRpa(pRL->localIRK, pRL->rlLocalRpa);
    } else {
        pRL->localIrk_valid = 0;
    }


    if (peer_irk && smemcmp(peer_irk, const_u8_16_zero, 16)) {
        //my_dump_str_data(DBG_PRVC_RL_EN, "[PRV][RL] peer irk nonzero", 0, 0);
        pRL->peerIrk_valid = 1;
        smemcpy(pRL->peerIRK, peer_irk, 16);
        /* only targetA(initA) of ADV_DIRECT_IND & AUX_ADV_IND will use "genrt peerRpa"
         * generate a RPA when add device, make sure at least it's available */
        blt_ll_resolvGenerateRpa(pRL->peerIRK, pRL->genrt_peerRpa);
    } else {
        pRL->peerIrk_valid = 0;
    }


    pRL->dev_iden_valid    = 1;
    pRL->peerRpa_save_flag = 0;
    pRL->rlPrivMode        = NETWORK_PRIVACY_MODE; //The added device shall be set to Network Privacy mode.
    blRslvLst.rlCnt++;

    my_dump_str_u32s(DBG_PRVC_RL_EN, "[PRV][RL] RL add", blRslvLst.rlCnt, pRL->rl_idx, pRL->localIrk_valid, pRL->peerIrk_valid);

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_addDeviceToResolvingList(hci_le_addDeviceResolvinglist_cmdParam_t *cmdParam)
{
    #if (DBG_PRVC_RL_EN) //see information more clear
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Add_Device_RL", cmdParam, 7);
    my_dump_str_data(IUT_HCI_LOG_EN, "[PRV] IRK", cmdParam->peer_IRK, 32);
    #else
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Add_Device_RL", cmdParam, sizeof(hci_le_addDeviceResolvinglist_cmdParam_t));
    #endif

    return blc_ll_addDeviceToResolvingList(cmdParam->peer_identity_address_type, cmdParam->peer_identity_address, cmdParam->peer_IRK, cmdParam->local_IRK);
}

/**
 * @brief      This function is used to remove a device from resolving list
 * @param[in]  peerIdAddrType - Peer_Identity_Address_Type
 * @param[in]  peerIdAddr - Peer_Identity_Address
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_ll_removeDeviceFromResolvingList(ida_type_t peerIdAddrType, u8 *peerIdAddr)
{
    /*
    When a Controller cannot remove a device from the resolving list because it is
    not found, it shall return the error code Unknown Connection Identifier (0x02).
    */

    if (blRslvLst.addrRlEn && !blt_ll_isResolvingListCommandAllowed()) {
        my_dump_str_data(DBG_HCI_CIS_TEST, "task busy, not allowed remove", 0, 0);
        return HCI_ERR_CMD_DISALLOWED;
    }

    if (peerIdAddrType > RANDOM_IDENTITY_ADDRESS) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    ll_resolv_list_t *pRL = blt_ll_searchResolvingListEntry(peerIdAddrType, peerIdAddr);


    if (pRL) {
        pRL->dev_iden_valid = 0;
        blRslvLst.rpa_use_matrix &= ~BIT(pRL->rl_idx);
        pRL->rpaUsed = 0;
        blRslvLst.rlCnt--;
        my_dump_str_u32s(DBG_PRVC_RL_EN, "[PRV][RL] RL remove", blRslvLst.rlCnt, pRL->rl_idx, pRL, 0);
    } else { //not found
        my_dump_str_data(DBG_HCI_CIS_TEST, "not found in RL", 0, 0);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }


    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_removeDeviceFromResolvingList(le_identityAddress_t *cmdParam)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Remove_Device_RL", cmdParam, sizeof(le_identityAddress_t));

    return blc_ll_removeDeviceFromResolvingList(cmdParam->peer_identity_address_type, cmdParam->peer_identity_address);
}

/* must use a different stack API for hci_reset */
_attribute_noinline_ void blt_ll_clearResolvingList(void)
{
    ll_resolv_list_t *pRL;
    for (int i = 0; i < blRslvLst.rlSize; i++) {
        pRL = (ll_resolv_list_t *)&blRslvLst.rlList[i];

        pRL->dev_iden_valid = 0;
        pRL->rpaUsed        = 0;
    }

    blRslvLst.rpa_use_matrix = 0;
    blRslvLst.rlCnt          = 0;
    blRslvLst.addrRlEn       = 0;

    /* back to default: 15min, 900 S */
    blRslvLst.rl_timeout_100S_num = 9;
    blRslvLst.rl_timeout_us       = 0;
}

_attribute_noinline_
    ble_sts_t
    blc_ll_clearResolvingList(void)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Clear_RL", 0, 0);

    if (blRslvLst.addrRlEn && !blt_ll_isResolvingListCommandAllowed()) {
        my_dump_str_data(DBG_HCI_CIS_TEST, "task busy, not allowed clear", 0, 0);
        return HCI_ERR_CMD_DISALLOWED;
    }

    blt_ll_clearResolvingList();

    return BLE_SUCCESS;
}

/**
 * @brief      This function is used to read resolving list size
 * @param[in]  none
 * @return     resolving list size
 */
int blc_ll_readResolvingListSize(void)
{
    return blRslvLst.rlSize;
}

ble_sts_t blc_hci_le_readResolvingListSize(hci_le_readResolvingListSizeCmd_retParam_t *pRetParam)
{
    pRetParam->status  = BLE_SUCCESS;
    pRetParam->rl_size = blRslvLst.rlSize;

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_RL_Size", pRetParam, 2);

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_readPeerResolvableAddress(le_identityAddress_t *pCmdParam, hci_le_readPeerResolvableAddress_retParam_t *pRetParam)
{ /*
    When a Controller cannot find a Resolvable Private Address associated with
    the Peer Identity Address, or if the Peer Identity Address cannot be found in the
    resolving list, it shall return the error code Unknown Connection Identifier
    (0x02).
    */
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Peer_RPA", pCmdParam, sizeof(le_identityAddress_t));

    ll_resolv_list_t *pRL = blt_ll_searchResolvingListEntry(pCmdParam->peer_identity_address_type, pCmdParam->peer_identity_address);

    if (pRL) {
        pRetParam->status = BLE_SUCCESS;
        if (pRL->peerRpa_save_flag) {
            smemcpy(pRetParam->peer_res_addr, pRL->store_peerRpa, BLE_ADDR_LEN);
        } else {
            smemset(pRetParam->peer_res_addr, 0, BLE_ADDR_LEN);
        }
        my_dump_str_data(DBG_PRVC_RL_EN, "[PRV][RL] return peer RPA", pRetParam->peer_res_addr, 6);
    } else {
        pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
        smemset(pRetParam->peer_res_addr, 0, BLE_ADDR_LEN);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }


    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_readLocalResolvableAddress(le_identityAddress_t *pCmdParam, hci_le_readLocalResolvableAddress_retParam_t *pRetParam)
{
    /*
    When a Controller cannot find a Resolvable Private Address associated with
    the Peer Identity Address, or if the Peer Identity Address cannot be found in the
    resolving list, it shall return the error code Unknown Connection Identifier
    (0x02).
    */
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_local_RPA", pCmdParam, sizeof(le_identityAddress_t));

    ll_resolv_list_t *pRL = blt_ll_searchResolvingListEntry(pCmdParam->peer_identity_address_type, pCmdParam->peer_identity_address);

    if (pRL) {
        pRetParam->status = BLE_SUCCESS;
        if (pRL->localIrk_valid) {
            smemcpy(pRetParam->local_res_addr, pRL->rlLocalRpa, BLE_ADDR_LEN);
        } else {
            smemset(pRetParam->local_res_addr, 0, BLE_ADDR_LEN);
        }
        my_dump_str_data(DBG_PRVC_RL_EN, "[PRV][RL] return local RPA", pRetParam->local_res_addr, 6);
    } else {
        pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
        smemset(pRetParam->local_res_addr, 0, BLE_ADDR_LEN);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    return BLE_SUCCESS;
}

/*
 * @brief   API to  enable resolution of Resolvable Private Addresses in the Controller.
 *          This causes the Controller to use the resolving list whenever the Controller
 *          receives a local or peer Resolvable Private Address.
 *
 * */
ble_sts_t blc_ll_setAddressResolutionEnable(addr_res_en_t resolution_en)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Addr_Resolution_Enable", &resolution_en, 1);

    if (!blt_ll_isResolvingListCommandAllowed()) {
        my_dump_str_data(DBG_HCI_CIS_TEST, "task busy, not allowed set en", 0, 0);
        return HCI_ERR_CMD_DISALLOWED;
    }


    //refer to Core spec page2564: This command does not affect the generation of Resolvable Private Addresses.
    blRslvLst.addrRlEn = resolution_en;

    return BLE_SUCCESS;
}

/*
 * @brief   API to set the length of time the controller uses a Resolvable Private Address
 *          before a new resolvable private address is generated and starts being used.
 *          This timeout applies to all addresses generated by the controller
 *
 * */
ble_sts_t blc_ll_setResolvablePrivateAddressTimeout(u16 rpa_timeout_s)
{
    u16 timeout_s = rpa_timeout_s;
    (void)timeout_s; //remove compiler warning
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_RPA_Timeout", &timeout_s, 2);


    if (rpa_timeout_s > 0 && rpa_timeout_s < 3601) {
        blRslvLst.rl_timeout_100S_num = rpa_timeout_s / 100;
        blRslvLst.rl_timeout_us       = (rpa_timeout_s % 100) * 1000000;

        return BLE_SUCCESS;
    } else {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }
}

/*
 * @brief   API to used to allow the Host to specify the privacy mode to be used for a given
 *          entry on the resolving list.
 *
 * */
ble_sts_t blc_ll_setPrivacyMode(ida_type_t peerIdAddrType, u8 *peerIdAddr, privacy_mode_t privMode)
{
    /*
    If the device is not on the resolving list, the Controller shall return the error
    code Unknown Connection Identifier (0x02).
    */

    if (peerIdAddrType > RANDOM_IDENTITY_ADDRESS || privMode > DEVICE_PRIVACY_MODE) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if (blRslvLst.addrRlEn && !blt_ll_isResolvingListCommandAllowed()) {
        my_dump_str_data(DBG_HCI_CIS_TEST, "task busy, not allowed set mode", 0, 0);
        return HCI_ERR_CMD_DISALLOWED;
    }

    ll_resolv_list_t *pRL = blt_ll_searchResolvingListEntry(peerIdAddrType, peerIdAddr);

    if (pRL) {
        pRL->rlPrivMode = privMode;
    } else {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }


    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setPrivacyMode(hci_le_setPrivacyMode_cmdParam_t *cmdParam)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Privacy_Mode", cmdParam, sizeof(hci_le_setPrivacyMode_cmdParam_t));

    return blc_ll_setPrivacyMode(cmdParam->peer_identity_address_type, cmdParam->peer_identity_address, cmdParam->privacy_mode);
}

/**
 * Used to determine if the device is on the resolving list.
 *
 * @param addr_type Public address (0) or random address (1)
 * @param addr
 *
 * @return Pointer to resolving list entry or NULL if no entry found.
 */
_attribute_ram_code_ //must be RamCode, IRQ will use
    ll_resolv_list_t *
    blt_ll_searchResolvingListEntry(u8 addrType, u8 *addr)
{
    ll_resolv_list_t *pRL = NULL;

    for (int i = 0; i < blRslvLst.rlSize; i++) {
        pRL = &blRslvLst.rlList[i];
        if (pRL->dev_iden_valid && pRL->rlIdAddrType == addrType && (!smemcmp(&pRL->rlIdAddr, addr, BLE_ADDR_LEN))) {
            return pRL;
        }
    }

    return NULL;
}

_attribute_ram_code_ //must be RamCode, IRQ will use
    void
    blt_ll_resolvSetRpaInUse(ll_resolv_list_t *pRL)
{
    if (pRL->rpaUsed) {
    } else { //first time
        //timer reset
        pRL->rpa_timeout_100S_cnt = blRslvLst.rl_timeout_100S_num;
        pRL->rpa_start_tick       = clock_time();
        blRslvLst.rpa_use_matrix |= BIT(pRL->rl_idx);
        pRL->rpaUsed = 1;
        my_dump_str_u32s(DBG_PRVC_RL_EN, "[PRV][RL] RL entry first in use", pRL->rl_idx, pRL, blRslvLst.rpa_use_matrix, 0);
    }
}

/* stack API, when you call it, you need confirm "pRL" is correct, never be "NULL"
 * */
_attribute_ram_code_ //must be RamCode, IRQ will use
    void
    blt_ll_storePeerDeviceRpa(ll_resolv_list_t *pRL, u8 *peer_rpa)
{
    pRL->peerRpa_save_flag = 1;
    smemcpy(pRL->store_peerRpa, peer_rpa, BLE_ADDR_LEN);
}

bool blt_ll_resolvIsLocalRpaUsed(void)
{
    return blRslvLst.rpa_use_matrix;
}

_attribute_ram_code_ //must be RamCode, IRQ will use
    ll_resolv_list_t *
    blt_ll_resolve_rpa(int local, u8 *rpa, ll_resolv_list_t *pRL_in)
{
    if (!blRslvLst.addrRlEn) {
        return NULL;
    }

    u8 *pIrk;
    int irk_valid;
    #if 0         //optimize to save SRAM
    ll_resolv_list_t *pRL;
    for(int i = 0; i < blRslvLst.rlSize; i++){
        pRL = &blRslvLst.rlList[i];
        if(!pRL_in || (pRL_in == pRL)){  //if pRL_in is NULL, traverse all entry, else use only pRL_in
            if(pRL->dev_iden_valid){
                if(local){
                    pIrk = pRL->localIRK;
                    irk_valid = pRL->localIrk_valid;
                }
                else{
                    pIrk = pRL->peerIRK;
                    irk_valid = pRL->peerIrk_valid;
                }

                if(irk_valid && aes_resolve_irk_rpa(pIrk, rpa)){
                    return pRL;
                }
            }
        }
    }
    #else
    if (pRL_in) { //if pRL_in none zero, use only pRL_in
        pIrk = local ? pRL_in->localIRK : pRL_in->peerIRK;
        if (aes_resolve_irk_rpa(pIrk, rpa)) {
            return pRL_in;
        }
    } else { //if pRL_in is NULL, traverse all entry
        ll_resolv_list_t *pRL;
        for (int i = 0; i < blRslvLst.rlSize; i++) {
            pRL = &blRslvLst.rlList[i];
            if (pRL->dev_iden_valid) {
                if (local) {
                    pIrk      = pRL->localIRK;
                    irk_valid = pRL->localIrk_valid;
                } else {
                    pIrk      = pRL->peerIRK;
                    irk_valid = pRL->peerIrk_valid;
                }
                if (irk_valid && aes_resolve_irk_rpa(pIrk, rpa)) {
                    return pRL;
                }
            }
        }
    }
    #endif


    return NULL;
}

//       my_dump_str_data(RESOLVINGLIST_DUMP_DBG_EN, "peer irk:", 0, 0);
//         my_dump_str_data(RESOLVINGLIST_DUMP_DBG_EN, "local irk:", 0, 0);
void blt_ll_resolvGenerateRpa(u8 *irk, u8 *addr)
{
    // Resolvable private address:
    // LSB                                                     MSB
    // +--------------------------+----------------------+---+---+
    // |                          | Random part of prand | 1 | 0 |
    // +--------------------------+----------------------+---+---+
    // <--------+ hash +---------> <-----------+ prand +--------->
    //          (24 bits)                     (24 bits)
    u8 *prand = addr + 3;
    generateRandomNum(3, prand);
    prand[2] = (prand[2] & 0x3F) | 0x40;


    u8 aes_in[16];
    u8 aes_out[16];

    for (int i = 0; i < 16; i++) {
        if (i < 3) {
            aes_in[i] = prand[i];
        } else {
            aes_in[i] = 0;
        }
    }


    //DBG_C HN15_HIGH;
    //Eagle, 48M clock, flash code, 24uS test by SiHui 20220806
    aes_encryption_le(irk, aes_in, aes_out);
    //DBG_C HN15_LOW;

    /* The output of the random address function ah is: ah(h, r) = e(k, r') mod 2^24
     * The output of the security function e is then truncated to 24 bits by taking the least significant 24
     * bits of the output of e as the result of ah.
     */
    for (int i = 0; i < 3; i++) {
        addr[i] = aes_out[i];
    }
}


#endif // The end of #if (LL_FEATURE_ENABLE_PRIVACY)
