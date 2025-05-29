/********************************************************************************************************
 * @file    hci_cmd.c
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
#include "stack/ble/controller/ble_controller.h"

#include "hci_stack.h"

void hci_initHciMng(void) //Reset
{
    bltHciMng.aclDataPktLen      = 0;
    bltHciMng.aclDataPktTotalNum = 0;
    bltHciMng.curCmplPktNum      = 0;
    bltHciMng.flowCtrlEnable     = 0;
}

void hci_resetCurHostAvailBufNum(void) //Disconnect
{
    if (!bltHciMng.flowCtrlEnable) {
        return;
    }
    bltHciMng.curCmplPktNum = bltHciMng.aclDataPktTotalNum;
    //TODO: bltHciMng.aclDataPktLen
}

ble_sts_t hci_setControllerToHostFlowCtrl(u8 ctrl)
{
    switch (ctrl) {
    case 0x00:
    case 0x01:
        //use for ACL
        break;

    case 0x02:
        //TODO: use for SCO
    case 0x03:
        //TODO: use for SCO
    default:
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    bltHciMng.flowCtrlEnable = ctrl;
    return BLE_SUCCESS;
}

u8 hci_getControllerToHostFlowCtrlState(void)
{
    return bltHciMng.flowCtrlEnable;
}

u16 hci_getHostAvailBufNum(void)
{
    if (bltHciMng.flowCtrlEnable != 0) {
        return bltHciMng.curCmplPktNum;
    }
    return 0xFFFF;
}

void hci_reduceOneHostAvailBuf(void)
{
    if ((bltHciMng.curCmplPktNum > 0) && (bltHciMng.flowCtrlEnable != 0)) {
        bltHciMng.curCmplPktNum--;
    }
}

ble_sts_t hci_hostBufferSize(hci_hostBufferSize_cmdParam_t *cmdPara)
{
    bltHciMng.aclDataPktLen      = cmdPara->aclDataPktLen;
    bltHciMng.aclDataPktTotalNum = cmdPara->aclDataPktTotalNum;
    bltHciMng.curCmplPktNum      = cmdPara->aclDataPktTotalNum;

    return BLE_SUCCESS;
}

ble_sts_t hci_hostNumCompletedPackets(hci_hostNumOfCompletedPkt_cmdParam_t *CompPackCom)
{
    u16 num_packt = 0;

    if ((bltHciMng.flowCtrlEnable == 0) && (!CompPackCom->num_sets)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    for (int i = 0; i < CompPackCom->num_sets; i++) {
        //CompPackCom.complete_packets[i].Connection_Handle&CONN_IDX_MASK;
        num_packt += CompPackCom->completePktCfg[i].numPktCompleted;
    }
    bltHciMng.curCmplPktNum += num_packt;
    if (bltHciMng.curCmplPktNum > bltHciMng.aclDataPktTotalNum) {
        bltHciMng.curCmplPktNum = bltHciMng.aclDataPktTotalNum;
    }
    return BLE_SUCCESS;
}
