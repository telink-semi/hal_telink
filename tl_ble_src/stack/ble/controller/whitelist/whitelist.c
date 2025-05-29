/********************************************************************************************************
 * @file    whitelist.c
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


_attribute_ble_data_retention_ ll_whiteListTbl_t ll_whiteList_tbl;

/**
 * @brief      This function is used to clear WhiteList
 * @param[in]  none
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_ll_clearWhiteList(void)
{
    ll_whiteList_tbl.wl_addr_tbl_index = 0;

    return BLE_SUCCESS;
}

/**
 * @brief      This function is used to add a device into WhiteList. The max number of devices in WhiteList is 4 (MAX_WHITE_LIST_SIZE).
 * @param[in]  adr_type - device address type
 * @param[in]  addr - device address
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_ll_addDeviceToWhiteList(u8 adr_type, u8 *addr)
{
    if (adr_type > BLE_ADDR_RANDOM) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if (blt_ll_searchAddrInWhiteListTbl(adr_type, addr)) {
        return BLE_SUCCESS;
    }


    if (ll_whiteList_tbl.wl_addr_tbl_index < MAX_WHITE_LIST_SIZE) {
        ll_whiteList_tbl.wl_addr_tbl[ll_whiteList_tbl.wl_addr_tbl_index].type = adr_type;
        smemcpy(ll_whiteList_tbl.wl_addr_tbl[ll_whiteList_tbl.wl_addr_tbl_index].address, addr, BLE_ADDR_LEN);
        ll_whiteList_tbl.wl_addr_tbl_index++;

        return BLE_SUCCESS;
    } else {
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }
}

ble_sts_t blc_hci_le_addDeviceToAcceptList(hci_le_addDeviceAcceptlist_cmdParam_t *pCmdParam)
{
    /* core_5.3
    This command shall not be used when:
        any advertising filter policy uses the Filter Accept List and advertising is enabled,

        the scanning filter policy uses the Filter Accept List and scanning is enabled,
            or
            the initiator filter policy uses the Filter Accept List and an
        HCI_LE_Create_Connection or HCI_LE_Extended_Create_Connection
            command is pending.

    When a Controller cannot add a device to the Filter Accept List because there
    is no space available, it shall return the error code Memory Capacity Exceeded
    (0x07).

    If the device is already in the Filter Accept List, the Controller should not add
    the device to the Filter Accept List again and should return success.

    Address shall be ignored when Address_Type is set to 0xFF.
    */
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Add_Device_Accept_List", pCmdParam, sizeof(hci_le_addDeviceAcceptlist_cmdParam_t));

    return blc_ll_addDeviceToWhiteList(pCmdParam->adr_type, pCmdParam->addr);
}

/**
 * @brief      This function is used to delete a device from WhiteList
 * @param[in]  type - device mac address type
 * @param[in]  addr - device mac address
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_ll_removeDeviceFromWhiteList(u8 adr_type, u8 *addr)
{
    wl_addr_t *pWL = blt_ll_searchAddrInWhiteListTbl(adr_type, addr);

    /*If has not found the addr in irk_table, or addr_table, return  ? */
    if (pWL) {
        /*If it is not the last addr stored in the table, need to move the last addr to this index*/
        if (!(pWL == &(ll_whiteList_tbl.wl_addr_tbl[ll_whiteList_tbl.wl_addr_tbl_index - 1]))) {
            smemcpy((u8 *)pWL, (u8 *)(&(ll_whiteList_tbl.wl_addr_tbl[ll_whiteList_tbl.wl_addr_tbl_index - 1])), sizeof(wl_addr_t));
        }

        ll_whiteList_tbl.wl_addr_tbl_index--;
    } else {
        //The standard ble_sts_t must be returned when passing authentication,
        //Here returns BLE_SUCCESS, which is considered a success,JingQiao encountered
        //the host mistakenly delete the WL device, after return non-BLE_SUCCESS,Host err.
    }


    return BLE_SUCCESS;
}

/**
 * @brief      This function is used to get WhiteList size
 * @param[out] pointer to size
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_ll_readWhiteListSize(hci_le_readWhiteListSizeCmd_retParam_t *retPara)
{
    //*returnPublicAddrListSize = MAX_WHITE_LIST_SIZE - ll_whiteList_tbl.wl_addr_tbl_index;

    // "the total number of white list entries that can stored in controller"
    retPara->status  = BLE_SUCCESS;
    retPara->wl_size = MAX_WHITE_LIST_SIZE;

    return BLE_SUCCESS;
}

/**
 * @brief   This function is used to check if address is existed in white list table
 * @param   None
 * @return  WL address
 */
/* must in RamCode:
 * 1. Some MCU,IRQ code must in RamCode to solve flash IRQ protect problem.
 * 2. timing is urgent for address filtering */
_attribute_ram_code_sec_noinline_ wl_addr_t *blt_ll_searchAddrInWhiteListTbl(u8 type, u8 *addr)
{
    wl_addr_t *pWL = NULL;
    for (int i = 0; i < ll_whiteList_tbl.wl_addr_tbl_index; i++) {
        pWL = (wl_addr_t *)&ll_whiteList_tbl.wl_addr_tbl[i];
        if (!smemcmp(pWL->address, addr, BLE_ADDR_LEN) && pWL->type == type) {
            return pWL;
        }
    }

    return NULL;
}
