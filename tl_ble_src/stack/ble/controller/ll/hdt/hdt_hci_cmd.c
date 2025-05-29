/********************************************************************************************************
 * @file    hdt_hci_cmd.c
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
#include "hdt_stack.h"
#include "stack/ble/controller/ble_controller.h"
#if LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT
hdt_defaultParam_t  hdt_dftParam = {
        .preferred_mic_length       = HDT_MIC_DFT_LENGTH,
        .packet_format              = HDT_FORMAT_ALL,
        .blocks_per_packet          = MAX_BLOCKS_PER_PACKET,
        .phy_rates                  = HDT_PHY_RATES_SUPPORTED,
        .max_tx_payload_window_size = MAX_TX_PAYLOAD_PER_PACKET_SUPPORTED,
        .max_rx_payload_window_size = MAX_RX_PAYLOAD_PER_PACKET_SUPPORTED,
};
hdt_testParam_t  hdt_testParam;
ble_sts_t blc_hci_le_setHdtDftParams(hci_le_setHdtDftParam_cmdParam_t *pCmdParam)
{
    hdt_dftParam.preferred_mic_length = pCmdParam->preferred_mic_length;
    HDT_HCI_LOG("[HDT_PARAM] success:%d",hdt_dftParam.preferred_mic_length);
    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setHdtParamsTest(hci_le_setHdtParamTest_cmdParam_t *pCmdParam,  hci_le_setHdtParamTest_retParam_t *pRetParam)
{
    pRetParam->conn_handle = pCmdParam->conn_handle;

    /*If the Host sends this command with a Connection_Handle that does not exist, or the Connection_Handle
      *is not for an ACL the Controller shall return the error code Unknown Connection Identifier (0x02).*/
    if (blt_ll_isAclhdlInvalid(pCmdParam->conn_handle)) {
        HDT_HCI_LOG("[HDT_PARAM Test] handle invalid:0x%x", pCmdParam->conn_handle);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    st_ll_conn_t *pc            = (st_ll_conn_t *)blt_ll_getAclConnPtr(pCmdParam->conn_handle);
    hdt_param_t   *pHDTParam    = &pc->hdtParam;

    /* If the Host issues this command for an ACL connection when the PHY of the connection is LE HDT PHY,
     * then the Controller shall return the error code Command Disallowed (0x0C).
     */
    if(pc->connPhyCtrl.conn_cur_phy == BLE_PHY_HDT){
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*
     * If the Host issues this command for a CIS after the CIS is established or for a BIG,
     * then the Controller shall return the error code Command Disallowed (0x0C).
     */
    //todo hdt



    /*If the Host issues this command with Packet_Format = 0x2 and the [LE HDT – Format 1]
     * feature is not supported, then the Controller shall return the error code Command Disallowed (0x0C).*/
    if((pCmdParam->packet_format == HDT_FORMAT1) && (hdt_dftParam.packet_format == HDT_FORMAT0))
    {
        return HCI_ERR_CMD_DISALLOWED;
    }

    pHDTParam->max_tx_payload_window_size = pCmdParam->max_tx_payload_window_size;
    pHDTParam->max_rx_payload_window_size = pCmdParam->max_rx_payload_window_size;
    pHDTParam->blocks_per_packet = pCmdParam->block_per_payload;
    pHDTParam->packet_format = pCmdParam->packet_format;
    HDT_HCI_LOG("[HDT_PARAM Test] success");
    return BLE_SUCCESS;
}

ble_sts_t blc_hci_refreshEncyptKey_V2(hci_refreshEncryptKeyV2_cmdParam_t *pCmdParam)
{
    /*If the Host sends this command with a Connection_Handle that does not exist, or the Connection_Handle
      *is not for an ACL the Controller shall return the error code Unknown Connection Identifier (0x02).*/
    if (blt_ll_isAclhdlInvalid(pCmdParam->conn_handle)) {
        HDT_HCI_LOG("[Enc_Key] handle invalid:0x%x", pCmdParam->conn_handle);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    st_ll_conn_t *pc            = (st_ll_conn_t *)blt_ll_getAclConnPtr(pCmdParam->conn_handle);
    pc->mic_length              = pCmdParam->mic_length;
    return BLE_SUCCESS;
}
#endif
