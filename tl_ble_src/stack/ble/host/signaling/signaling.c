/********************************************************************************************************
 * @file    signaling.c
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
#include "stack/ble/ble.h"


static const signal_rx_proc_t signal_unCoC_handlers[];
_attribute_ble_data_retention_ signal_rx_proc_t* signal_rx_handlers = (signal_rx_proc_t*)(size_t)signal_unCoC_handlers;
_attribute_ble_data_retention_ u8 sendIdentifier = 0;


/** Dispatch table for incoming Signal commands.  Must be ordered by signal code. */
static const signal_rx_proc_t signal_unCoC_handlers[] =
{
    SIGNAL_RX_STR(COMMAND_REJECT_RSP,               cmdRejectRsp),
    SIGNAL_RX_STR(CONN_PARAM_UPDATE_REQ,            connParamUpdateReq),
    SIGNAL_RX_STR(CONN_PARAM_UPDATE_RSP,            connParamUpdateRsp),
    {0, 0, NULL},   //package end
};

#if L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN
const signal_rx_proc_t signal_CoC_handlers[] =
{
    SIGNAL_RX_STR(COMMAND_REJECT_RSP,               cmdRejectRsp),
    SIGNAL_RX_STR(CONN_PARAM_UPDATE_REQ,            connParamUpdateReq),
    SIGNAL_RX_STR(CONN_PARAM_UPDATE_RSP,            connParamUpdateRsp),
    SIGNAL_RX_STR(DISCONNECTION_REQ,                disconnReq),
    SIGNAL_RX_STR(DISCONNECTION_RSP,                disconnRsp),
    SIGNAL_RX_STR(LE_CREDIT_BASED_CONNECTION_REQ,   leCreditBasedConnReq),
    SIGNAL_RX_STR(LE_CREDIT_BASED_CONNECTION_RSP,   leCreditBasedConnRsp),
    SIGNAL_RX_STR(FLOW_CONTROL_CREDIT_IND,          flowCtrlCreditInd),
    SIGNAL_RX_STR(CREDIT_BASED_CONNECTION_REQ,      creditBasedConnReq),
    SIGNAL_RX_STR(CREDIT_BASED_CONNECTION_RSP,      creditBasedConnRsp),
    SIGNAL_RX_STR(CREDIT_BASED_RECONFIGURE_REQ,     creditBasedRecfgReq),
    SIGNAL_RX_STR(CREDIT_BASED_RECONFIGURE_RSP,     creditBasedRecfgRsp),
    {0, 0, NULL},   //package end
#if 0
    {L2CAP_COMMAND_REJECT_RSP,              sizeof(blt_signal_commandFormat_t),         blt_signal_proc_cmdRejectRsp},
    {L2CAP_DISCONNECTION_REQ,               sizeof(blt_signal_disconnReq_t),            blt_signal_proc_disconnReq},
    {L2CAP_DISCONNECTION_RSP,               sizeof(blt_signal_disconnRsp_t),            blt_signal_proc_disconnRsp},
    {L2CAP_CONN_PARAM_UPDATE_REQ,           sizeof(blt_signal_connParamUpdateReq_t),    blt_signal_proc_connParamUpdateReq},
    {L2CAP_CONN_PARAM_UPDATE_RSP,           sizeof(blt_signal_connParamUpdateRsp_t),    blt_signal_proc_connParamUpdateRsp},
    {L2CAP_LE_CREDIT_BASED_CONNECTION_REQ,  sizeof(blt_signal_leCreditBasedConnReq_t),  blt_signal_proc_leCreditBasedConnReq},
    {L2CAP_LE_CREDIT_BASED_CONNECTION_RSP,  sizeof(blt_signal_leCreditBasedConnRsp_t),  blt_signal_proc_leCreditBasedConnRsp},
    {L2CAP_FLOW_CONTROL_CREDIT_IND,         sizeof(blt_signal_flowCtrlCreditInd_t),     blt_signal_proc_flowCtrlCreditInd},
    {L2CAP_CREDIT_BASED_CONNECTION_REQ,     sizeof(blt_signal_creditBasedConnReq_t),    blt_signal_proc_creditBasedConnReq},
    {L2CAP_CREDIT_BASED_CONNECTION_RSP,     sizeof(blt_signal_creditBasedConnRsp_t),    blt_signal_proc_creditBasedConnRsp},
    {L2CAP_CREDIT_BASED_RECONFIGURE_REQ,    sizeof(blt_signal_creditBasedRecfgReq_t),   blt_signal_proc_creditBasedRecfgReq},
    {L2CAP_CREDIT_BASED_RECONFIGURE_RSP,    sizeof(blt_signal_creditBasedRecfgRsp_t),   blt_signal_proc_creditBasedRecfgRsp},
    {0, 0, NULL},   //package end
#endif

};

#endif

static u16 blt_signal_procSignalRxPkt(signalConCb_t* pSignalConCb, signal_pkt_t* signal, u16 signalLen)
{
    if(signalLen < sizeof(blt_signal_commandFormat_t) || signalLen != signal->dataLen + sizeof(blt_signal_commandFormat_t))
    {
        return blt_signal_packageCmdRejectRspUnderstood(pSignalConCb->signalTxBuff, signal->identifier);
    }

    u8 code = signal->code;

    for (int i = 0; ; i++) {

        if(signal_rx_handlers[i].code == 0)
        {
            break;
        }

        if (code == signal_rx_handlers[i].code) {
            if (signalLen < signal_rx_handlers[i].expect_len) {
                tlkapi_printf(0, "signal rx length check err"); //debug
                return blt_signal_packageCmdRejectRspUnderstood(pSignalConCb->signalTxBuff, signal->identifier);
            } else {
                tlkapi_printf(0, "signal_rx_fn code is %d", code); //debug
                return signal_rx_handlers[i].signal_rx_fn(pSignalConCb, signal, signalLen);
            }
        }
    }

    return blt_signal_packageCmdRejectRspUnderstood(pSignalConCb->signalTxBuff, signal->identifier);
}

////////////////////////////////////////////////////////////////////////////
//                  att_rx_process (peripheral + central)
////////////////////////////////////////////////////////////////////////////
int blt_signal_l2capSignalRxHandler (u16 connHandle, l2cap_pkt_t *ptrSignal)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return 0;
    }

    u8 signalTxData[23];        //LE-U max 23 octets

    signalConCb_t signalConnCb = {
        .connHandle = connHandle,
        .signalTxBuff = (signal_pkt_t *)signalTxData,
    };

    u16 pduLen = blt_signal_procSignalRxPkt(&signalConnCb, &ptrSignal->payload.signal, ptrSignal->pduLen);

    if(pduLen){
        tlkapi_send_string_data(0, "TX Signal", (u8*)signalConnCb.signalTxBuff, pduLen); //debug
        blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_SIG_CHANNEL,
                &signalConnCb.signalTxBuff->code, sizeof(signal_pkt_t),
                signalConnCb.signalTxBuff->data, signalConnCb.signalTxBuff->dataLen);
    }

    return 0;
}

u8 blt_signal_getIdentifier(void)
{
    sendIdentifier = sendIdentifier == 0xFF? 0x01: sendIdentifier+1;

    return sendIdentifier;
}
