/********************************************************************************************************
 * @file    adv.c
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


_attribute_ble_data_retention_  u32     blt_advExpectTime;
_attribute_ble_data_retention_  u32     blc_rcvd_connReq_tick;


_attribute_ble_data_retention_  ll_adv_t    bltAdv;

_attribute_ble_data_retention_
rf_packet_adv_t pkt_Adv = {
        rf_tx_packet_dma_len(6+2),              // dma_len

        LL_TYPE_ADV_IND,                        // type
        0,                                      // RFU
        0,                                      // ChSel: only valid in ADV_IND/ADV_DIRECT_IND/CONNECT_IND, other packet set 0'b
        0,                                      // txAddr
        0,                                      // rxAddr

        6,                                      // rf_len
        {0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5},   // advA
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // data
};


_attribute_ble_data_retention_
rf_packet_scan_rsp_t    pkt_scanRsp = {
        rf_tx_packet_dma_len(6+2),              // dma_len

        LL_TYPE_SCAN_RSP,                       // type
        0,                                      // RFU
        0,                                      // ChSel: only valid in ADV_IND/ADV_DIRECT_IND/CONNECT_IND, other packet set 0'b
        0,                                      // txAddr
        0,                                      // rxAddr

        6,                                      // rf_len
        {0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5},   // advA
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // data
};












void blt_ll_initAdvertisingCommon(void)
{
    bltAdv.delay_sSlot_value = MAX_DELAY_10MS;
    bltAdv.delay_sSlot_mask = MAX_DELAY_10MS - 1;
}


void blt_ll_reset_adv_common(void)
{
    //necessary for BQB: consider that when host do not set ADV data or scan_rsp data, it expect controller using default length 0.
    pkt_Adv.dma_len = rf_tx_packet_dma_len(6+2);
    pkt_Adv.rf_len = 6;

    pkt_scanRsp.dma_len = rf_tx_packet_dma_len(6+2);
    pkt_scanRsp.rf_len = 6;
}




/*
      MAX_DELAY_10MS    = 0x1FF,  //10000us = 625us *16 = sSlot * 512
      MAX_DELAY_5MS     = 0x0FF,  // 5000us = 625us *8  = sSlot * 256
      MAX_DELAY_2P5MS   = 0x07F,  // 2500us = 625us *4  = sSlot * 128
      MAX_DELAY_0MS     = 0,
*/
void        blc_ll_setMaxAdvDelay_for_AdvEvent(adv_max_delay_t max_delay)
{
    if(max_delay == MAX_DELAY_10MS || max_delay == MAX_DELAY_5MS || max_delay == MAX_DELAY_2P5MS){
        bltAdv.delay_sSlot_value = max_delay;
        bltAdv.delay_sSlot_mask = max_delay - 1;
    }
    else if(max_delay == MAX_DELAY_0MS){
        bltAdv.delay_sSlot_value = max_delay;
        bltAdv.delay_sSlot_mask = 0;
    }
}



#if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE)

ble_sts_t blc_hci_le_setDataRelatedAddressChange(hci_le_setDataAddrChange_cmdParams_t *pCmdParam)
{
    /* core_5.3
    If extended advertising commands (see Section 3.1.1) are being used and the
    advertising set corresponding to the Advertising_Handle parameter does not
    exist, or if no command specified in Table 3.2 has been used, then the
    Controller shall return the error code Unknown Advertising Identifier (0x42).

    If legacy advertising commands are being used, the Controller shall ignore the
    Advertising_Handle parameter.
    */
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Set_Data_Addr_Change", pCmdParam, sizeof(hci_le_setDataAddrChange_cmdParams_t));

    if(IS_EXTENDED_ADV_VALID){
        if(ll_ext_adv_mlp_task_cb){
            return ll_ext_adv_mlp_task_cb(FLAG_EXTADV_SET_DATA_ADDR_CHANGE, pCmdParam); // blt_ext_adv_mainloop_task  blc_ll_setExtendedAdvDataRelatedAddressChange
        }
    }
    else if(IS_LEGACY_ADV_VALID){
        /* according to design style, here we should use LEGADV main_loop callback pointer to set
         * but we use a variable in common structure, can save a lot of flash code
         * */
        bltAdv.legAdv_chngReason = pCmdParam->reasons;
        return BLE_SUCCESS;
    }
    else{ //no ADV command
        /*  no command specified in Table 3.2( Version 5.3 | Vol 4, Part E,3.1.1 Legacy and extended advertising )
         *  has been used, then the Controller shall return the error code Unknown Advertising Identifier (0x42). */
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }

    return HCI_ERR_INVALID_HCI_CMD_PARAMS;
}

#endif
