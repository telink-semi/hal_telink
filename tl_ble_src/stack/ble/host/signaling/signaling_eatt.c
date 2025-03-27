/********************************************************************************************************
 * @file    signaling_eatt.c
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

#if (L2CAP_SERVER_FEATURE_SUPPORTED_EATT)

int blt_eatt_l2capEattRxHandle(l2cap_coc_cid_t* pCid)
{
    attConCb_t eattConCb = {
        .connHandle = pCid->connHandle,
        .scid = pCid->srcCID,
    };

    eattConCb.attTxBuff = pCid->sendTotalLen? NULL: (attr_pkt_t*)pCid->pTxSdu;

    tlkapi_printf(0, "recv pdu is %s, %x", hex_to_str(pCid->pRxSdu, pCid->recvLen), eattConCb.attTxBuff);
    u16 pduLen = blt_att_procAttrRxPkt(&eattConCb, (attr_pkt_t*)pCid->pRxSdu, pCid->recvLen);

    tlkapi_printf(0, "pdu len is %d, value is %s", pduLen, hex_to_str(eattConCb.attTxBuff, pduLen));

    if(pduLen)
    {
        ble_sts_t state = blc_l2cap_sendCocData(pCid->connHandle, pCid->srcCID, (u8*)eattConCb.attTxBuff, pduLen);
        tlkapi_printf(0, "send eatt data is %x", state);
    }
    return 0;
}

#endif
