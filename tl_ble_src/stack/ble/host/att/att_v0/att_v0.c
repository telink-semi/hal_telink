/********************************************************************************************************
 * @file    att_v0.c
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


////////////////////////////////////////////////////////
// l2cap use for ATT_OP_PREPARE_WRITE_REQ
////////////////////////////////////////////////////////

//l2cap buffer max: header(2)+l2cap_len(2)+cid(2)+ATT_MTU_MAX(250).
#define L2CAP_RX_BUFF_LEN_MAX (256)

_attribute_ble_data_retention_ u8 blt_buff_prepare_write[L2CAP_RX_BUFF_LEN_MAX] = {0};

_attribute_ble_data_retention_ att_pre_write_buff_t att_pre_write = {
    .buff       = blt_buff_prepare_write, //dft buffer
    .buffMaxLen = L2CAP_RX_BUFF_LEN_MAX,  //dft buffer length
};


_attribute_ble_data_retention_ attribute_t *gAttributes = 0;


_attribute_ble_data_retention_ att_mng_t bltAtt;


_attribute_ble_data_retention_
    rf_packet_att_errRsp_t pkt_errRsp = {
        0x02,                               // type
        sizeof(rf_packet_att_errRsp_t) - 2, // rf_len
        sizeof(rf_packet_att_errRsp_t) - 6, // l2cap_len
        4,                                  // chanId
        ATT_OP_ERROR_RSP,                   // opcode
        0,                                  // errOpcode
        0,                                  // errHandle
        ATT_ERR_ATTR_NOT_FOUND,             // errReason
};


_attribute_data_retention_ u8 att_WriteReqReject_en = 0;


_attribute_data_retention_ u8 att_ReadReqReject_en = 0;

#if DOCKKIT_MODIFY_ATT_RSP_LEN_EN
_attribute_data_retention_ int att_custom_read_rsp_len = 0;

//This API is not a common used or standard API. So supposed to extern it on app layer.
void blc_att_setReadRsp_len(int len)
{
    att_custom_read_rsp_len = len;
}
#endif


void bls_att_setAttributeTable(u8 *p)
{
    if (p) {
        gAttributes = (attribute_t *)p;
    }
}

static inline int uuid_match(u8 uuidLen, u8 *uuid1, u8 *uuid2)
{
    if (2 == uuidLen && uuid1[0] == uuid2[0] && uuid1[1] == uuid2[1]) {
        return 1;
    }

    u32 *uuid1_temp = (u32 *)uuid1;
    u32 *uuid2_temp = (u32 *)uuid2;

    if (16 == uuidLen && (uuid1_temp[0] == uuid2_temp[0]) && (uuid1_temp[1] == uuid2_temp[1]) && (uuid1_temp[2] == uuid2_temp[2]) && (uuid1_temp[3] == uuid2_temp[3])) {
        return 1;
    }
    return 0;
}

attribute_t *l2cap_att_search(u16 sh, u16 eh, u8 *attUUID, u16 *h)
{
    if (sh == gAttributes[0].attNum) {
        return 0; // ??????
    }

    eh = eh < gAttributes[0].attNum ? eh : gAttributes[0].attNum;
    while (sh <= eh) {
        attribute_t *pAtt = &gAttributes[sh];
        if (uuid_match(pAtt->uuidLen, pAtt->uuid, attUUID)) {
            *h = sh;
            return pAtt;
        }
        ++sh;
    }
    return 0;
}

void blc_att_setMtureqSendingTime_after_connCreate(int time_ms)
{
    l2cap_buff_s.mtuReqSendTimeUs = l2cap_buff_m.mtuReqSendTimeUs = time_ms * 1000;
}

//set connMaxRxOctets of master
ble_sts_t blc_att_setCentralRxMtuSize(u16 cen_mtu_size)
{
    /**
     * Remove the upper limit, otherwise, the order of calling API must be considered
     */
    //  if(master_mtu_size+6 > l2cap_buff_m.max_rx_size)  //6 = header(2)+l2cap_len(2)+CID(2), align 4bytes
    if (cen_mtu_size < ATT_MTU_SIZE) {
        return GATT_ERR_DATA_LENGTH_EXCEED_MTU_SIZE;
    }

    l2cap_buff_m.init_MTU = cen_mtu_size;

    return BLE_SUCCESS;
}

//set connMaxRxOctets of slave
ble_sts_t blc_att_setPeripheralRxMtuSize(u16 per_mtu_size)
{
    /**
     * Remove the upper limit, otherwise, the order of calling API must be considered
     */
    //  if(slave_mtu_size+6 > l2cap_buff_s.max_rx_size)  //6 = header(2)+l2cap_len(2)+CID(2), align 4bytes
    if (per_mtu_size < ATT_MTU_SIZE) {
        return GATT_ERR_DATA_LENGTH_EXCEED_MTU_SIZE;
    }

    l2cap_buff_s.init_MTU = per_mtu_size;

    return BLE_SUCCESS;
}

////////////multiple master and multiple slave////////
ble_sts_t blc_att_requestMtuSizeExchange(u16 connHandle, u16 mtu_size)
{
    if (((connHandle & BLM_CONN_HANDLE) && (!blc_att_setCentralRxMtuSize(mtu_size))) || ((connHandle & BLS_CONN_HANDLE) && (!blc_att_setPeripheralRxMtuSize(mtu_size)))) {
        u8                            mtu_exchange[16]; // 9 + 4(mic)
        rf_packet_att_mtu_exchange_t *pReq = (rf_packet_att_mtu_exchange_t *)mtu_exchange;
        pReq->type                         = 2;         //llid
        pReq->rf_len                       = 7;
        pReq->l2capLen                     = 0x0003;
        pReq->chanId                       = 0x0004;
        pReq->opcode                       = ATT_OP_EXCHANGE_MTU_REQ;
        pReq->mtu[0]                       = U16_LO(mtu_size);
        pReq->mtu[1]                       = U16_HI(mtu_size);
        if (ll_push_tx_fifo_handler(connHandle | HANDLE_STK_FLAG, mtu_exchange)) {
            return BLE_SUCCESS;
        } else {
            return LL_ERR_TX_FIFO_NOT_ENOUGH;
        }
    }
    return GATT_ERR_INVALID_PARAMETER;
}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
    void
    blt_att_procMtuExgPending(u16 connHandle)
{
    u8             conn_idx     = (connHandle & CONN_IDX_MASK);
    st_ll_conn_t  *pc           = (st_ll_conn_t *)(u32)&blms[conn_idx];
    gap_ms_para_t *pGap_ms_para = &gap_ms_para[conn_idx];

    pl2cap_buff = (pc->aclRole == ACL_ROLE_CENTRAL) ? &l2cap_buff_m : &l2cap_buff_s;

    if (!blt_ll_isEncryptionBusy(connHandle) &&
        pc->conn_established_tick && clock_time_exceed(pc->conn_established_tick, pl2cap_buff->mtuReqSendTimeUs)) {
        typedef ble_sts_t (*mtuSizeExcFunc)(u16 connHandle, u16 mtu_size);
        mtuSizeExcFunc mtuSizeExcProc = pGap_ms_para->mtu_exg_pending == 1 ?
                                            blc_att_requestMtuSizeExchange :
                                            blt_gattc_mtuSizeExchangeReq;

        if (mtuSizeExcProc(connHandle, pl2cap_buff->init_MTU) == BLE_SUCCESS) {
            pGap_ms_para->mtu_exg_pending = 0;
        }
    }
}

u16 att_Find_end_group(u16 sh, u16 eh, u16 uuid)
{
    u16 s = sh + 1;
    // retrun value change from "0" to "sh": if the last attribute is just primary service, the group end handle is sh
    // change from ">=" to ">" : if the last two attributes are primary service and character, the group end handle is not sh
    //  if(s >= gAttributes[0].attNum) return 0;
    if (s > gAttributes[0].attNum) {
        return sh; // add by yuexin, review by qinghua
    }
    eh = eh < gAttributes[0].attNum ? eh : gAttributes[0].attNum;

    while (s <= eh) {
        attribute_t *pAtt = &gAttributes[s];

        if (uuid == DECLARATIONS_UUID_CHARACTERISTIC) {
            if ((*(u16 *)pAtt->uuid == DECLARATIONS_UUID_CHARACTERISTIC) || (*(u16 *)pAtt->uuid == DECLARATIONS_UUID_SECONDARY_SERVICE) || (*(u16 *)pAtt->uuid == DECLARATIONS_UUID_PRIMARY_SERVICE)) {
                return (s - 1);
            }
        } else {
            if ((*(u16 *)pAtt->uuid == DECLARATIONS_UUID_SECONDARY_SERVICE) || (*(u16 *)pAtt->uuid == DECLARATIONS_UUID_PRIMARY_SERVICE)) {
                return (s - 1);
            }
        }
        ++s;
    }

    return gAttributes[0].attNum; //return the handle of end att
}

attribute_t *att_find_by_type_value_search(u16 sh, u16 eh, u8 *attUUID, u8 *value, u16 len, u16 *ret_fh, u16 *ret_geh)
{
    //  if(sh == gAttributes[0].attNum) return 0;
    if (sh > gAttributes[0].attNum) {
        return 0; // change from "=" to ">", find by tainxian, review qinghua,yafei
    }
    eh = eh < gAttributes[0].attNum ? eh : gAttributes[0].attNum;

    while (sh <= eh) {
        attribute_t *pAtt = &gAttributes[sh];

        if (uuid_match(2, pAtt->uuid, attUUID)) // attribute type 2bytes
        {
            if (len <= pAtt->attrLen) {
                if (!memcmp(value, pAtt->pAttrValue, len)) {
                    if ((*(u16 *)attUUID == DECLARATIONS_UUID_PRIMARY_SERVICE) || (*(u16 *)attUUID == DECLARATIONS_UUID_SECONDARY_SERVICE) || (*(u16 *)attUUID == DECLARATIONS_UUID_CHARACTERISTIC)) {
                        *ret_geh = att_Find_end_group(sh, gAttributes[0].attNum, *(u16 *)attUUID);
                    } else {
                        *ret_geh = sh;
                    }

                    *ret_fh = sh;
                    return pAtt;
                }
            }
        }
        ++sh;
    }
    return 0;
}

attribute_t *att_read_by_group_type_request_search(u16 sh, u16 eh, u8 *attUUID, u8 attUUID_len, u16 *ret_fh, u16 *ret_geh)
{
    //  if(sh == gAttributes[0].attNum) return 0;

    // if the last handle in attribute table is just a primary service,it may cause some err that can't read this handle correctly
    // fix it by changing ">=" to ">",add by yuexin,review by qinghua
    if (sh > gAttributes[0].attNum) {
        return 0;
    }

    eh = eh < gAttributes[0].attNum ? eh : gAttributes[0].attNum;

    while (sh <= eh) {
        attribute_t *pAtt = &gAttributes[sh];

        if (uuid_match(attUUID_len, pAtt->uuid, attUUID)) // TODO type 2bytes if pAtt->uuidLen <16
        {
            *ret_geh = att_Find_end_group(sh, gAttributes[0].attNum, *(u16 *)attUUID);

            *ret_fh = sh;
            return pAtt;
        }
        ++sh;
    }
    return 0;
}

void blc_att_setPrepareWriteBuffer(u8 *p, u16 len)
{
    if (p) {
        att_pre_write.buff       = (u8 *)p;
        att_pre_write.buffMaxLen = len;
    }
}

u8 *bls_att_l2capAttCmdHandler(u16 connHandle, u8 *pkt)
{
    if (!gAttributes) {
        return 0;
    }

    rf_packet_l2cap_req_t *req          = (rf_packet_l2cap_req_t *)pkt;
    gap_ms_para_t         *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);

    if (pGap_ms_para == NULL) {
        return 0;
    }


    my_dump_str_data(HANDLE_VALUE_CUSTOM_DBG_EN, "ATT Req raw", &req->opcode, req->l2capLen);

#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en) {
        blt_att_processAttHandle_in_attCmd(req);
    }
#endif

    my_dump_str_data(HANDLE_VALUE_CUSTOM_DBG_EN, "ATT Req sdk", &req->opcode, req->l2capLen);

    if (req->opcode < ATT_OP_WRITE_REQ) {
        tlkapi_send_string_data((stkLog_mask & STK_LOG_ATT_RX), "[ATT][RX] ATT Req", &req->chanId, req->l2capLen + 2);
    }


    u8 *r                = 0;
    pkt_errRsp.errReason = ATT_ERR_ATTR_NOT_FOUND;
    pkt_errRsp.errHandle = 0;
    u8 *ptx_buff         = blt_l2cap_get_s_tx_buff(connHandle);

    ((rf_packet_att_data_t *)ptx_buff)->type   = 2; //ll data
    ((rf_packet_att_data_t *)ptx_buff)->chanid = 4; //att

    switch (req->opcode) {
    case ATT_OP_READ_BY_GROUP_TYPE_REQ:
    {
        attribute_t                        *pAtt;
        rf_packet_att_readByType_t         *p   = (rf_packet_att_readByType_t *)req;
        rf_packet_att_readByGroupTypeRsp_t *rsp = (rf_packet_att_readByGroupTypeRsp_t *)ptx_buff;
        //      pkt_errRsp.errReason = ATT_SUCCESS;

        pGap_ms_para->att_service_discover_tick = clock_time() | 1;

        //u16 sh = p->startingHandle, eh = p->endingHandle;
        u16 sh = p->startingHandle;
        u16 eh = p->endingHandle;

        u16 attUUID        = p->attType[0] | (p->attType[1] << 8);
        u16 groupEndHandle = sh;
        u16 i              = 0;
        u32 attrLen        = 0;
        u8  uuid_len       = 0;

        if ((sh != 0) && (sh <= eh)) {
            if ((attUUID == DECLARATIONS_UUID_PRIMARY_SERVICE) || (attUUID == DECLARATIONS_UUID_SECONDARY_SERVICE) || (attUUID == DECLARATIONS_UUID_CHARACTERISTIC)) {
                u16 total_pkt = 2;

                if (p->l2capLen == 21) {
                    uuid_len = 16;
                } else if (p->l2capLen == 7) {
                    uuid_len = 2;
                }
                if (uuid_len) {
                    while ((pAtt = att_read_by_group_type_request_search(sh, eh, (u8 *)&attUUID, uuid_len, &sh, &groupEndHandle))) {
                        if (attrLen && attrLen != pAtt->attrLen) {
                            break;
                        }

                        attrLen = pAtt->attrLen;
                        if (total_pkt > pGap_ms_para->effective_MTU - (4 + attrLen)) {
                            break;
                        }

                        u16 start_hl = sh;
                        u16 end_hl   = groupEndHandle;

#if (ATT_HANDLE_VALUE_CUSTOM_EN)
                        if (bltAtt.attHl_custom_en) {
                            start_hl = blt_att_change_sdkAttHandle_to_customAttHandle(sh);
                            end_hl   = blt_att_change_sdkAttHandle_to_customAttHandle(groupEndHandle);
                        }
#endif

                        rsp->data[i++] = start_hl;
                        rsp->data[i++] = (start_hl >> 8) & 0xff;
                        rsp->data[i++] = end_hl;
                        rsp->data[i++] = (end_hl >> 8) & 0xff;
                        memcpy((u8 *)&rsp->data[i], pAtt->pAttrValue, pAtt->attrLen);

                        total_pkt += (4 + attrLen);
                        i += (pAtt->attrLen);
                        sh = groupEndHandle + 1;
                        if (sh > eh) {
                            break;
                        }
                    }
                } else {
                    pkt_errRsp.errReason = ATT_ERR_INVALID_ATTR_VALUE_LEN;
                }
            } else {
                pkt_errRsp.errReason = ATT_ERR_UNSUPPORTED_GRP_TYPE;
            }
        } else {
            pkt_errRsp.errReason = ATT_ERR_INVALID_HANDLE;
        }

        if (i > 0) {
            rsp->l2capLen = 2 + i;
            rsp->rf_len   = rsp->l2capLen + 4;
            rsp->opcode   = ATT_OP_READ_BY_GROUP_TYPE_RSP;
            rsp->datalen  = attrLen + 4;
            r             = (u8 *)(rsp);
        } else {
            pkt_errRsp.errOpcode = ATT_OP_READ_BY_GROUP_TYPE_REQ;
            pkt_errRsp.errHandle = p->startingHandle;
            pkt_errRsp.errReason = ATT_ERR_ATTR_NOT_FOUND;
            r                    = (u8 *)&pkt_errRsp;
        }

    } break;
    case ATT_OP_FIND_BY_TYPE_VALUE_REQ:
    {
        attribute_t                   *pAtt;
        rf_packet_att_findByTypeReq_t *p        = (rf_packet_att_findByTypeReq_t *)req;
        rf_packet_att_findByTypeRsp_t *rsp      = (rf_packet_att_findByTypeRsp_t *)ptx_buff;
        pkt_errRsp.errReason                    = ATT_ERR_ATTR_NOT_FOUND;
        pkt_errRsp.errHandle                    = p->startingHandle;
        pGap_ms_para->att_service_discover_tick = clock_time() | 1;


        u16 sh = p->startingHandle;
        u16 eh = p->endingHandle;
        //u16 findByType_startHandle = sh; //unused RMV
        u16 groupEndHandle = sh;
        u16 attUUID        = (p->attType[1] << 8) | p->attType[0];
        u16 i              = 0;

        //my_dump_str_data(0, "ATT_OP_FIND_BY_TYPE_VALUE_REQ 01:", &sh, 2);

        if (sh != 0 && sh <= eh) {
            while ((pAtt = att_find_by_type_value_search(sh, eh, (u8 *)(&attUUID), p->attValue, p->l2capLen - 7, &sh, &groupEndHandle))) {
                if ((i * 2) + 1 > pGap_ms_para->effective_MTU - 4) {
                    break;
                }

                u16 start_hl = sh;
                u16 end_hl   = groupEndHandle;

#if (ATT_HANDLE_VALUE_CUSTOM_EN)
                if (bltAtt.attHl_custom_en) {
                    start_hl = blt_att_change_sdkAttHandle_to_customAttHandle(sh);
                    end_hl   = blt_att_change_sdkAttHandle_to_customAttHandle(groupEndHandle);
                }
#endif

                rsp->data[i++] = start_hl;
                rsp->data[i++] = end_hl;

                sh = groupEndHandle + 1;
                if (sh > eh) {
                    break;
                }
            }
        } else {
            pkt_errRsp.errReason = ATT_ERR_INVALID_HANDLE;
        }

        if (i > 0) {
            rsp->l2capLen = (i * 2) + 1;
            rsp->rf_len   = rsp->l2capLen + 4;
            rsp->opcode   = ATT_OP_FIND_BY_TYPE_VALUE_RSP;
            r             = (u8 *)(rsp);
        } else {
            pkt_errRsp.errOpcode = ATT_OP_FIND_BY_TYPE_VALUE_REQ;
            pkt_errRsp.errReason = ATT_ERR_ATTR_NOT_FOUND;
            r                    = (u8 *)(&pkt_errRsp);
        }
    } break;
    case ATT_OP_READ_BY_TYPE_REQ:
    {
        rf_packet_att_readByType_t    *p   = (rf_packet_att_readByType_t *)req;
        rf_packet_att_readByTypeRsp_t *rsp = (rf_packet_att_readByTypeRsp_t *)ptx_buff;
        attribute_t                   *pAtt;

        pGap_ms_para->att_service_discover_tick = clock_time() | 1;

        u16 sh = p->startingHandle, eh = p->endingHandle;
        u16 i          = 0;
        u8  uuidLen    = 0;
        u8  uuidReqLen = 0;
        uuidReqLen     = (p->l2capLen == 0x0015) ? 16 : 2;
        u8 attUUID[16];
        memcpy(attUUID, (u8 *)(p->attType), uuidReqLen);

        //pkt_errRsp.errReason = (p->startingHandle >(p->endingHandle))?(ATT_ERR_INVALID_HANDLE):0x00;

        pkt_errRsp.errReason = (sh == 0 || sh > eh) ? (ATT_ERR_INVALID_HANDLE) : 0x00;
        if (pkt_errRsp.errReason == 0) {
            if ((uuidReqLen == 2) && (attUUID[0] == U16_LO(DECLARATIONS_UUID_CHARACTERISTIC)) && (attUUID[1] == U16_HI(DECLARATIONS_UUID_CHARACTERISTIC))) {
                while ((pAtt = l2cap_att_search(sh, eh, (u8 *)(attUUID), &sh))) {
                    if (uuidLen && uuidLen != (pAtt + 1)->uuidLen) {
                        break;
                    }
                    if (i + uuidLen + 5 > pGap_ms_para->effective_MTU - 2) {
                        break;
                    }
/*
                     * //Although the new method is relatively clean and efficient,
                     * the old writing method can avoid some mistakes of the upper
                     * users, so it is recommended to use the old method.
                     *
                     * reviewed by qinghua.fan,kai.jia
                     */
#if 1
                    rsp->data[i++] = sh;
                    rsp->data[i++] = (sh >> 8) & 0xff;
                    sh++;
                    rsp->data[i++] = pAtt->pAttrValue[0];
                    ++pAtt;
                    rsp->data[i++] = sh;
                    rsp->data[i++] = (sh >> 8) & 0xff;
                    sh++;
                    memcpy(&rsp->data[i], pAtt->uuid, pAtt->uuidLen);
                    uuidLen = pAtt->uuidLen;
                    i += pAtt->uuidLen;
#else //New Logic
                    u16 att_hl = sh;

    #if (ATT_HANDLE_VALUE_CUSTOM_EN)
                    if (bltAtt.attHl_custom_en) {
                        att_hl = blt_att_change_sdkAttHandle_to_customAttHandle(sh);
                    }
    #endif

                    rsp->data[i++] = att_hl;
                    rsp->data[i++] = (att_hl >> 8) & 0xff;
                    memcpy(&rsp->data[i], pAtt->pAttrValue, (pAtt + 1)->uuidLen + 3);
                    uuidLen = (pAtt + 1)->uuidLen;
                    i += (pAtt + 1)->uuidLen + 3;
                    sh += 2;
#endif
                }
                rsp->datalen = uuidLen + 5;
            } else {
                if ((pAtt = l2cap_att_search(sh, eh, (u8 *)(attUUID), &sh))) {
                    if (uuidReqLen && uuidReqLen != pAtt->uuidLen) {
                        break;
                    }
                    u8 read_perm = gAttributes[sh].perm & (ATT_PERMISSIONS_AUTHOR_READ | ATT_PERMISSIONS_AUTHEN_READ | ATT_PERMISSIONS_ENCRYPT_READ | ATT_PERMISSIONS_READ);
                    if (read_perm == 0) {
                        pkt_errRsp.errReason = ATT_ERR_READ_NOT_PERMITTED;
                        pkt_errRsp.errOpcode = ATT_OP_READ_BY_TYPE_REQ;
                        pkt_errRsp.errHandle = sh;
                        r                    = (u8 *)(&pkt_errRsp);
                        break;
                    }

                    u16 att_hl = sh;

#if (ATT_HANDLE_VALUE_CUSTOM_EN)
                    if (bltAtt.attHl_custom_en) {
                        att_hl = blt_att_change_sdkAttHandle_to_customAttHandle(sh);
                    }
#endif

                    rsp->data[0] = att_hl;
                    rsp->data[1] = att_hl >> 8;

                    /*
                     * ATT_MTU_SIZE - Opcode(1B) - length(1B) - Handle(2B) = AttUserData(19B)
                     */
                    u16 avaAttUsrLen  = (pGap_ms_para->effective_MTU - 4);
                    u8  pAtt_used_len = pAtt->attrLen > avaAttUsrLen ? avaAttUsrLen : pAtt->attrLen;

                    memcpy(&rsp->data[2], pAtt->pAttrValue, pAtt_used_len);
                    i = 2 + pAtt_used_len;
                }
                rsp->datalen = i;
            }
        }

        if (i > 0) {
            rsp->l2capLen = i + 2;
            rsp->rf_len   = rsp->l2capLen + 4;
            rsp->opcode   = ATT_OP_READ_BY_TYPE_RSP;
            r             = (u8 *)(rsp);
        } else {
            if (!pkt_errRsp.errReason) {
                pkt_errRsp.errReason = ATT_ERR_ATTR_NOT_FOUND;
            }
            pkt_errRsp.errOpcode = ATT_OP_READ_BY_TYPE_REQ;
            pkt_errRsp.errHandle = sh;
            r                    = (u8 *)(&pkt_errRsp);
        }
    } break;
    case ATT_OP_FIND_INFO_REQ:
    {
        rf_packet_att_readByType_t    *p   = (rf_packet_att_readByType_t *)req;
        rf_packet_att_readByTypeRsp_t *rsp = (rf_packet_att_readByTypeRsp_t *)ptx_buff;
        attribute_t                   *pAtt;

        pGap_ms_para->att_service_discover_tick = clock_time() | 1;

        u16 sh           = p->startingHandle;
        u16 eh           = p->endingHandle < gAttributes[0].attNum ? p->endingHandle : gAttributes[0].attNum;
        int i            = 0;
        u8  tem_attLen   = 18;
        u8  format       = 1;
        u8  last_uuidLen = 0;
        if (eh > gAttributes[0].attNum) {
            eh = gAttributes[0].attNum;
        }
        while (sh <= eh) {
            if (i + tem_attLen > pGap_ms_para->effective_MTU - 2) {
                break;
            }
            pAtt = &gAttributes[sh];
            /*************In a single RSP uuid len should same*****************/
            if (!last_uuidLen) {
                last_uuidLen = pAtt->uuidLen;
            }
            if (last_uuidLen != pAtt->uuidLen) {
                break;
            }
            last_uuidLen = pAtt->uuidLen;

            u16 att_hl = sh;

#if (ATT_HANDLE_VALUE_CUSTOM_EN)
            if (bltAtt.attHl_custom_en) {
                att_hl = blt_att_change_sdkAttHandle_to_customAttHandle(sh);
            }
#endif

            rsp->data[i++] = att_hl;
            rsp->data[i++] = att_hl >> 8;

            if (pAtt->uuidLen == 2) {
                rsp->data[i++] = pAtt->uuid[0];
                rsp->data[i++] = pAtt->uuid[1];
                tem_attLen     = 4;
            } else { //modified by june
                memcpy(&rsp->data[i], pAtt->uuid, pAtt->uuidLen);
                i += 16;
                tem_attLen = 18;
                format     = 2;
            }
            sh++;
        }
        if (i) {
            rsp->l2capLen = i + 2;
            rsp->rf_len   = rsp->l2capLen + 4;
            rsp->opcode   = ATT_OP_FIND_INFO_RSP;
            rsp->datalen  = format;
            r             = (u8 *)(rsp);
        } else {
            pkt_errRsp.errOpcode = ATT_OP_FIND_INFO_REQ;
            pkt_errRsp.errHandle = sh;
            pkt_errRsp.errReason = ATT_ERR_ATTR_NOT_FOUND;
            r                    = (u8 *)(&pkt_errRsp);
        }
    } break;
    case ATT_OP_WRITE_CMD:
    case ATT_OP_WRITE_REQ:
    {
        rf_packet_att_write_t *p    = (rf_packet_att_write_t *)req;
        u16                    h    = p->handle;
        attribute_t           *pAtt = &gAttributes[h];
        pkt_errRsp.errOpcode        = req->opcode;
        pkt_errRsp.errHandle        = p->handle;

        //      printf("write cmd: 0x%x, handle: 0x%x\r\n", p->opcode, p->handle | (p->handle<<8));
        if (h <= gAttributes[0].attNum) {
            u8 gatt_perm = gAttributes[h].perm;
            if (!(gatt_perm & ATT_PERMISSIONS_WRITE)) {
                pkt_errRsp.errReason = ATT_ERR_WRITE_NOT_PERMITTED;

                /*
                 * For ATT_WRITE_CMD, No ATT_ERROR_RSP or ATT_WRITE_RSP PDUs shall be sent in response
                 * to this command. If the server cannot write this attribute for any reason the
                 * command shall be ignored
                 */
                r = (req->opcode == ATT_OP_WRITE_CMD) ? NULL : (u8 *)(&pkt_errRsp);
                break;
            }


            if (gatt_perm & ATT_PERMISSIONS_SECURITY) {
                u8 att_err = blt_gatt_requestServiceAccess(connHandle, gatt_perm);
                if (att_err) {
                    pkt_errRsp.errReason = att_err;
                    r                    = (u8 *)(&pkt_errRsp);
                    break;
                }
            }


            if (ATT_OP_WRITE_REQ == req->opcode) {
                rf_packet_att_writeRsp_t *rsp = (rf_packet_att_writeRsp_t *)ptx_buff;

                rf_packet_att_writeRsp_t pkt_writeRsp = {
                    0x02,                                 // type
                    sizeof(rf_packet_att_writeRsp_t) - 2, // rf_len
                    sizeof(rf_packet_att_writeRsp_t) - 6, // l2cap_len
                    4,                                    // chanId
                    ATT_OP_WRITE_RSP,
                };

                memcpy(rsp, &pkt_writeRsp, sizeof(rf_packet_att_writeRsp_t));
                r = (u8 *)rsp;
                //              r = (u8*)&pkt_writeRsp;
            }
            if (pAtt->w) {
                u8 errReason = pAtt->w(connHandle, p);
                if (errReason && att_WriteReqReject_en) {
                    pkt_errRsp.errReason = errReason;
                    r                    = (req->opcode == ATT_OP_WRITE_CMD) ? NULL : (u8 *)(&pkt_errRsp);
                    break;
                }
            } else {
                if (p->l2capLen >= 3) {
                    u16 len = p->l2capLen - 3;
                    //pAtt->attrLen = len;
                    if (len > pAtt->attrLen) {
                        pkt_errRsp.errReason = ATT_ERR_INVALID_ATTR_VALUE_LEN;
                        r                    = (u8 *)(&pkt_errRsp);
                        break;
                    }
                    memcpy(pAtt->pAttrValue, &p->value, len);
                }
            }
        } else {
            pkt_errRsp.errReason = ATT_ERR_INVALID_HANDLE;
            r                    = (u8 *)(&pkt_errRsp);
        }

    } break;

    case ATT_OP_PREPARE_WRITE_REQ:
    {
        rf_packet_att_write_t *p = (rf_packet_att_write_t *)req;
        u16                    h = p->handle;
        pkt_errRsp.errOpcode     = req->opcode;
        pkt_errRsp.errHandle     = p->handle;
        if (h <= gAttributes[0].attNum) {
            u8 gatt_perm = gAttributes[h].perm;
            if (!(gatt_perm & ATT_PERMISSIONS_WRITE)) {
                pkt_errRsp.errReason = ATT_ERR_WRITE_NOT_PERMITTED;
                r                    = (u8 *)(&pkt_errRsp);
                break;
            }

            if (gatt_perm & ATT_PERMISSIONS_SECURITY) {
                u8 att_err = blt_gatt_requestServiceAccess(connHandle, gatt_perm);
                if (att_err) {
                    pkt_errRsp.errReason = att_err;
                    r                    = (u8 *)(&pkt_errRsp);
                    break;
                }
            }

            int offset = p->value | (*(&p->value + 1)) << 8;

            unsigned short prepare_pkt_len = 0;
            if (offset == 0) {
                att_pre_write.handle = connHandle;
                att_pre_write.offset = 9; //init prepare queue length
                memcpy(att_pre_write.buff, p, 9);
            }

            if (att_pre_write.handle != connHandle) {
                pkt_errRsp.errReason = ATT_ERR_PREPARE_QUEUE_FULL;
                r                    = (u8 *)(&pkt_errRsp);
                break;
            }

            prepare_pkt_len = att_pre_write.offset;
            prepare_pkt_len += p->l2capLen - 5;     //opcode,handle,offset

            if (prepare_pkt_len > att_pre_write.buffMaxLen) {
                prepare_pkt_len -= p->l2capLen - 5; //opcode,handle,offset
                pkt_errRsp.errReason = ATT_ERR_PREPARE_QUEUE_FULL;
                r                    = (u8 *)(&pkt_errRsp);
                break;
            }

            att_pre_write.offset = prepare_pkt_len;
            memcpy(att_pre_write.buff + 9 + offset, &p->value + 2, p->l2capLen - 5);
            rf_packet_att_write_t *pw = (rf_packet_att_write_t *)att_pre_write.buff;
            pw->l2capLen              = offset + p->l2capLen - 2; //subtract offset

            rf_packet_att_write_t *writePrepRsp = p;
            writePrepRsp->opcode                = ATT_OP_PREPARE_WRITE_RSP;
#if (ATT_HANDLE_VALUE_CUSTOM_EN)
            if (bltAtt.attHl_custom_en) {
                writePrepRsp->handle = blt_att_change_sdkAttHandle_to_customAttHandle(h);
            }
#endif
            r = (u8 *)writePrepRsp;
        } else {
            pkt_errRsp.errReason = ATT_ERR_INVALID_HANDLE;
            r                    = (u8 *)(&pkt_errRsp);
        }
    } break;

    case ATT_OP_EXECUTE_WRITE_REQ:
    {
        rf_packet_att_executeWriteReq_t *rst = (rf_packet_att_executeWriteReq_t *)req;
        rf_packet_att_writeRsp_t        *rsp = (rf_packet_att_writeRsp_t *)ptx_buff;

        rf_packet_att_writeRsp_t pkt_execute_writeRsp = {
            0x02,                                 // type
            sizeof(rf_packet_att_writeRsp_t) - 2, // rf_len
            sizeof(rf_packet_att_writeRsp_t) - 6, // l2cap_len
            4,                                    // chanId
            ATT_OP_EXECUTE_WRITE_RSP,
        };

        memcpy(rsp, &pkt_execute_writeRsp, sizeof(rf_packet_att_writeRsp_t));
        r = (u8 *)rsp;

        if (att_pre_write.handle != connHandle) {
            pkt_errRsp.errReason = ATT_ERR_INVALID_HANDLE;
            r                    = (u8 *)(&pkt_errRsp);
        }

        if (rst->flags == 0x01) //imm write all pending prepared values
        {
            rf_packet_att_write_t *p    = (rf_packet_att_write_t *)att_pre_write.buff;
            u16                    h    = p->handle;
            attribute_t           *pAtt = &gAttributes[h];

            pkt_errRsp.errOpcode = ATT_OP_EXECUTE_WRITE_REQ;
            pkt_errRsp.errHandle = p->handle;

            if (h <= gAttributes[0].attNum) {
                if (pAtt->w) {
                    pAtt->w(connHandle, p);
                } else {
                    if (p->l2capLen >= 3) {
                        u16 len = p->l2capLen - 3;
                        //pAtt->attrLen = len;
                        if (len > pAtt->attrLen) {
                            pkt_errRsp.errReason = ATT_ERR_INVALID_ATTR_VALUE_LEN;
                            r                    = (u8 *)(&pkt_errRsp);
                            break;
                        }

                        memcpy(pAtt->pAttrValue, &p->value, len);
                    }
                }
            } else {
                pkt_errRsp.errReason = ATT_ERR_INVALID_HANDLE;
                r                    = (u8 *)(&pkt_errRsp);
            }
        } else if (rst->flags == 0x00) { //cancel
            att_pre_write.offset = 0;
            att_pre_write.handle = 0xffff;
        }
    } break;

    case ATT_OP_SIGNED_WRITE_CMD:
    {
#if 0
        rf_packet_att_write_t *p = (rf_packet_att_write_t*)req;
        u16 h = p->handle;
        attribute_t *pAtt = &gAttributes[h];
        u16 len = p->l2capLen - 15;
        smp_secSigInfo_t rec_siginfo;
        if(h <= gAttributes[0].attNum)
        {
            memcpy(rec_siginfo,p->value+len,12);
            if(smp_sign_verify(rec_siginfo)==0) //todo:smp_sign_verify to be implemented
            {
                if(p->l2capLen >= 3)
                {
                    memcpy(pAtt->pAttrValue, &p->value, len);
                }
            }
        }
#endif
    } break;
    case ATT_OP_READ_REQ:
    case ATT_OP_READ_BLOB_REQ:
    {
        rf_packet_att_readBlob_t *p = (rf_packet_att_readBlob_t *)req;


        u16          h       = p->handle;
        attribute_t *pAtt    = &gAttributes[h];
        pkt_errRsp.errOpcode = req->opcode;
        pkt_errRsp.errHandle = p->handle;
        if (h <= gAttributes[0].attNum) {
            u8 gatt_perm = gAttributes[h].perm;
            if (!(gatt_perm & ATT_PERMISSIONS_READ)) //no read permission
            {
                pkt_errRsp.errReason = ATT_ERR_READ_NOT_PERMITTED;
                r                    = (u8 *)(&pkt_errRsp);
                break;
            }

            if (gatt_perm & ATT_PERMISSIONS_SECURITY) {
                u8 att_err = blt_gatt_requestServiceAccess(connHandle, gatt_perm);
                if (att_err) {
                    pkt_errRsp.errReason = att_err;
                    r                    = (u8 *)(&pkt_errRsp);
                    break;
                }
            }


            if (pAtt->r) {
                u8 errReason = pAtt->r(connHandle, p);
                if (errReason && att_ReadReqReject_en) {
                    pkt_errRsp.errReason = errReason;
                    r                    = (u8 *)(&pkt_errRsp);
                    break;
                }
            }

            rf_packet_att_readRsp_t *rsp  = (rf_packet_att_readRsp_t *)ptx_buff;
            u8                      *psrc = pAtt->pAttrValue;
#if DOCKKIT_MODIFY_ATT_RSP_LEN_EN
            int len                 = att_custom_read_rsp_len ? att_custom_read_rsp_len : pAtt->attrLen;
            att_custom_read_rsp_len = 0;
#else
            int len = pAtt->attrLen;
#endif
            if (req->opcode == ATT_OP_READ_BLOB_REQ) {
                rsp->opcode = ATT_OP_READ_BLOB_RSP;

                u16 offset = p->offset;

                if (offset > pAtt->attrLen) {
                    pkt_errRsp.errReason = ATT_ERR_INVALID_OFFSET;
                    r                    = (u8 *)(&pkt_errRsp);
                    break;
                }
                len -= offset;
                psrc += offset;
            } else {
                rsp->opcode = ATT_OP_READ_RSP;
            }

            if (len > (pGap_ms_para->effective_MTU - 1)) {
                len = pGap_ms_para->effective_MTU - 1;
            } else if (len < 0) {
                len = 0;
            }

            memcpy(rsp->value, psrc, len);
            rsp->l2capLen = len + 1;
            rsp->rf_len   = rsp->l2capLen + 4;
            r             = (u8 *)(rsp);
        } else {
            pkt_errRsp.errReason = ATT_ERR_INVALID_HANDLE;
            r                    = (u8 *)(&pkt_errRsp);
        }
    } break;

#if (0) //not used
    case ATT_OP_READ_MULTI_REQ:
    {
        rf_packet_l2cap_req_t   *p        = (rf_packet_l2cap_req_t *)req;
        rf_packet_att_readRsp_t *rsp      = (rf_packet_att_readRsp_t *)ptx_buff;
        u8                      *pData    = p->data;
        u8                       dataLen  = p->l2capLen - 1;
        u8                       aHandles = dataLen / 2;
        if (aHandles == 0 || dataLen > pGap_ms_para->effective_MTU - 1) {
            pkt_errRsp.errReason = ATT_ERR_ATTR_NOT_FOUND;
            r                    = (u8 *)(&pkt_errRsp);
            return r;
        }

        pkt_errRsp.errOpcode = ATT_OP_READ_MULTI_REQ;
        pkt_errRsp.errHandle = ((u16)p->data[1] << 8) | (p->data[0]);

        u8 *buffer        = &rsp->opcode;
        u16 buffLen       = 0, ahandle;
        buffer[buffLen++] = ATT_OP_READ_MULTIPLE_RSP;

        int maxRspValLen = pGap_ms_para->effective_MTU - 1;

        while (aHandles--) {
            STREAM_TO_U16(ahandle, pData);
            if (ahandle == 0 || ahandle > gAttributes[0].attNum) {
                buffLen = 1;
                break;
            }

            u8 gatt_perm = gAttributes[ahandle].perm;
            if (!(gatt_perm & ATT_PERMISSIONS_READ)) //no read permission
            {
                pkt_errRsp.errReason = ATT_ERR_READ_NOT_PERMITTED;
                r                    = (u8 *)(&pkt_errRsp);
                break;
            }

            if (gatt_perm & ATT_PERMISSIONS_SECURITY) {
                u8 att_err = blt_gatt_requestServiceAccess(connHandle, gatt_perm);
                if (att_err) {
                    pkt_errRsp.errReason = att_err;
                    r                    = (u8 *)(&pkt_errRsp);
                    break;
                }
            }

            /* If the Set Of Values parameter is longer than (ATT_MTU-1) then only the first (ATT_MTU-1) octets shall be included in this response. */
            u16 attrLen = gAttributes[ahandle].attrLen;
            maxRspValLen -= (attrLen + 0);
            if (maxRspValLen < 0) {
                attrLen += maxRspValLen;
                memcpy(buffer + buffLen, gAttributes[ahandle].pAttrValue, attrLen);
                buffLen += attrLen;
                break;
            }

            memcpy(buffer + buffLen, gAttributes[ahandle].pAttrValue, attrLen);
            buffLen += attrLen;
        }

        if (buffLen == 1) {
            pkt_errRsp.errReason = ATT_ERR_ATTR_NOT_FOUND;
            r                    = (u8 *)(&pkt_errRsp);
        }

        rsp->l2capLen = buffLen;
        r             = (u8 *)(rsp);
    } break;
#endif

    case ATT_OP_HANDLE_VALUE_CFM:
    {
        if (gap_eventMask & GAP_EVT_MASK_GATT_HANDLE_VALUE_CONFIRM) {
#if (CUSTOM_DARWIN_FMN_ENABLE)
            if (custom_darwin_fmn.darwin_fmn_enable) {
                u16 report_handle[2];
                report_handle[0] = pGap_ms_para->indicate_handle;
                report_handle[1] = connHandle;
                blc_gap_send_event(GAP_EVT_GATT_HANDLE_VALUE_CONFIRM, (u8 *)&report_handle ,sizeof(report_handle)); //app_host_event_callback
            } else
#endif
            {
                u16 report_handle[2];
                report_handle[0] = connHandle;
                report_handle[1] = pGap_ms_para->indicate_handle;
                blc_gap_send_event ( GAP_EVT_GATT_HANDLE_VALUE_CONFIRM, (u8 *)&report_handle ,sizeof(report_handle));
            }
        }

        pGap_ms_para->indicate_handle = 0;
    } break;

    case ATT_OP_READ_MULTIPLE_VARIABLE_REQ:
    {
        rf_packet_l2cap_req_t   *p        = (rf_packet_l2cap_req_t *)req;
        rf_packet_att_readRsp_t *rsp      = (rf_packet_att_readRsp_t *)ptx_buff;
        u8                      *pData    = p->data;
        u8                       dataLen  = p->l2capLen - 1;
        u8                       aHandles = dataLen / 2;
        if (aHandles == 0 || dataLen > pGap_ms_para->effective_MTU - 1) {
            pkt_errRsp.errReason = ATT_ERR_ATTR_NOT_FOUND;
            r                    = (u8 *)(&pkt_errRsp);
            return r;
        }

        pkt_errRsp.errOpcode = ATT_OP_READ_MULTIPLE_VARIABLE_REQ;
        pkt_errRsp.errHandle = ((u16)p->data[1] << 8) | (p->data[0]);

        u8 *buffer        = &rsp->opcode;
        u16 buffLen       = 0, ahandle;
        buffer[buffLen++] = ATT_OP_READ_MULTIPLE_VARIABLE_RSP;

        int maxRspValLen = pGap_ms_para->effective_MTU - 1;

        while (aHandles--) {
            STREAM_TO_U16(ahandle, pData);
            if (ahandle == 0 || ahandle > gAttributes[0].attNum) {
                buffLen = 1;
                break;
            }

            u8 gatt_perm = gAttributes[ahandle].perm;
            if (!(gatt_perm & ATT_PERMISSIONS_READ)) //no read permission
            {
                pkt_errRsp.errReason = ATT_ERR_READ_NOT_PERMITTED;
                r                    = (u8 *)(&pkt_errRsp);
                break;
            }

            if (gatt_perm & ATT_PERMISSIONS_SECURITY) {
                u8 att_err = blt_gatt_requestServiceAccess(connHandle, gatt_perm);
                if (att_err) {
                    pkt_errRsp.errReason = att_err;
                    r                    = (u8 *)(&pkt_errRsp);
                    break;
                }
            }

            /* If the Set Of Values parameter is longer than (ATT_MTU-1) then only the first (ATT_MTU-1) octets shall be included in this response. */
            u16 attrLen = gAttributes[ahandle].attrLen;
            maxRspValLen -= (attrLen + 2);
            if (maxRspValLen < 0) {
                attrLen += maxRspValLen;

                buffer[buffLen++] = attrLen & 0x00FF;
                buffer[buffLen++] = (attrLen & 0xFF00) >> 8;
                memcpy(buffer + buffLen, gAttributes[ahandle].pAttrValue, attrLen);
                buffLen += attrLen;
                break;
            }

            buffer[buffLen++] = attrLen & 0x00FF;
            buffer[buffLen++] = (attrLen & 0xFF00) >> 8;
            memcpy(buffer + buffLen, gAttributes[ahandle].pAttrValue, attrLen);
            buffLen += attrLen;
        }

        if (buffLen == 1) {
            pkt_errRsp.errReason = ATT_ERR_ATTR_NOT_FOUND;
            r                    = (u8 *)(&pkt_errRsp);
        }

        rsp->l2capLen = buffLen;
        r             = (u8 *)(rsp);
    } break;

    default:
        pkt_errRsp.errOpcode = req->opcode;
        pkt_errRsp.errHandle = 0;
        pkt_errRsp.errReason = ATT_ERR_REQ_NOT_SUPPORTED;
        r                    = (u8 *)&pkt_errRsp;
        break;
    }

#if (ATT_HANDLE_VALUE_CUSTOM_EN)
    if (bltAtt.attHl_custom_en && r == (u8 *)&pkt_errRsp) {
        pkt_errRsp.errHandle = blt_att_change_sdkAttHandle_to_customAttHandle(pkt_errRsp.errHandle);
    }
#endif


#if (HANDLE_VALUE_CUSTOM_DBG_EN)
    if (r) {
        rf_packet_l2cap_req_t *pRsp = (rf_packet_l2cap_req_t *)r;
        my_dump_str_data(HANDLE_VALUE_CUSTOM_DBG_EN, "ATT Rsp    ", &pRsp->opcode, pRsp->l2capLen);
    }
#endif


    if (r && (stkLog_mask & STK_LOG_ATT_TX)) {
        rf_packet_l2cap_req_t *pRsp = (rf_packet_l2cap_req_t *)r;
        if (pRsp->opcode < ATT_OP_WRITE_REQ) {
            tlkapi_send_string_data((stkLog_mask & STK_LOG_ATT_TX), "[ATT][TX] ATT Rsp", &pRsp->chanId, pRsp->l2capLen + 2);
        }
    }


    return r;
}

void blc_att_enableWriteReqReject(u8 WriteReqReject_en)
{
    att_WriteReqReject_en = WriteReqReject_en;
}

void blc_att_enableReadReqReject(u8 ReadReqReject_en)
{
    att_ReadReqReject_en = ReadReqReject_en;
}


#if 0
void att_req_read_by_type (u8 *p, u16 start_attHandle, u16 end_attHandle, u8 *uuid, int uuid_len)
{
    p[0] = 2;
    p[1] = 9 + uuid_len;
    p[2] = 5 + uuid_len;
    p[3] = 0;
    p[4] = 4;
    p[5] = 0;
    p[6] = ATT_OP_READ_BY_TYPE_REQ;
    p[7] = start_attHandle;
    p[8] = start_attHandle >> 8;
    p[9] = end_attHandle;
    p[10] = end_attHandle >> 8;
    memcpy (p + 11, uuid, uuid_len);
}


void att_req_read_by_group_type (u8 *p, u16 start_attHandle, u16 end_attHandle, u8 *uuid, int uuid_len)
{
    if(uuid_len>16) uuid_len=16;
    p[0] = 2;
    p[1] = 9 + uuid_len;
    p[2] = 5 + uuid_len;
    p[3] = 0;
    p[4] = 4;
    p[5] = 0;
    p[6] = ATT_OP_READ_BY_GROUP_TYPE_REQ;
    p[7] = start_attHandle;
    p[8] = start_attHandle >> 8;
    p[9] = end_attHandle;
    p[10] = end_attHandle >> 8;
    memcpy (p + 11, uuid, uuid_len);
}

void att_req_find_info(u8 *p, u16 start_attHandle, u16 end_attHandle)
{
    p[0] = 2;
    p[1] = 9;
    p[2] = 5;
    p[3] = 0;
    p[4] = 4;
    p[5] = 0;
    p[6] = ATT_OP_FIND_INFO_REQ;
    p[7] = start_attHandle;
    p[8] = start_attHandle >> 8;
    p[9] = end_attHandle;
    p[10] = end_attHandle >> 8;
}

void att_req_find_by_type (u8 *p, u16 start_attHandle, u16 end_attHandle, u8 *uuid, u8* attr_value, int len)
{
    p[0] = 2;
    p[1] = 11 + len;
    p[2] = 7 + len;
    p[3] = 0;
    p[4] = 4;
    p[5] = 0;
    p[6] = ATT_OP_FIND_BY_TYPE_VALUE_REQ;
    p[7] = start_attHandle;
    p[8] = start_attHandle >> 8;
    p[9] = end_attHandle;
    p[10] = end_attHandle >> 8;
    memcpy (p + 11, uuid, 2);
    memcpy (p + 13, attr_value, len);
}
//
void att_req_read (u8 *p, u16 attHandle)
{
    p[0] = 2;
    p[1] = 7;
    p[2] = 3;
    p[3] = 0;
    p[4] = 4;
    p[5] = 0;
    p[6] = ATT_OP_READ_REQ;
    p[7] = attHandle;
    p[8] = attHandle >> 8;
}

void att_req_read_blob (u8 *p, u16 attHandle, u16 offset)
{
    p[0] = 2;
    p[1] = 9;
    p[2] = 5;
    p[3] = 0;
    p[4] = 4;
    p[5] = 0;
    p[6] = ATT_OP_READ_BLOB_REQ;
    p[7] = attHandle;
    p[8] = attHandle >> 8;
    p[9] = offset;
    p[10] = offset >> 8;
}

void att_req_read_multi (u8 *p, u16* h, u8 n)
{
    if(n>10)
        n = 10;
    p[0] = 2;
    p[1] = 7 + 2*n;
    p[2] = 3 + 2*n;
    p[3] = 0;
    p[4] = 4;
    p[5] = 0;
    p[6] = ATT_OP_READ_MULTI_REQ;
    for(u8 i = 0; i<n; i++)
    {
        p[7+i] = *h;
        p[8+i] = *h >> 8;
        h++;
    }
}

void att_req_write (u8 *p, u16 attHandle, u8 *buf, int len)
{
    if (len > 20)
        len = 20;
    p[0] = 2;
    p[1] = 7 + len;
    p[2] = 3 + len;
    p[3] = 0;
    p[4] = 4;
    p[5] = 0;
    p[6] = ATT_OP_WRITE_REQ;
    p[7] = attHandle;
    p[8] = attHandle >> 8;
    memcpy (p + 9, buf, len);
}

void att_req_signed_write_cmd (u8 *p, u16 attHandle, u8 *pd, int n, u8 *sign)
{
    if (n > 8)
        n = 8;
    p[0] = 2;
    p[1] = 19 + n;
    p[2] = 15 + n;
    p[3] = 0;
    p[4] = 4;
    p[5] = 0;
    p[6] = ATT_OP_SIGNED_WRITE_CMD;
    p[7] = attHandle;
    p[8] = attHandle >> 8;
    memcpy (p + 9, pd, n);
    memcpy (p+n+9, sign, 12);
}

void att_req_prep_write (u8 *p, u16 attHandle, u8 *pd, u16 offset, int len)
{
    if (len > 18)
        len = 18;
    p[0] = 2;
    p[1] = 7 + len;
    p[2] = 3 + len;
    p[3] = 0;
    p[4] = 4;
    p[5] = 0;
    p[6] = ATT_OP_PREPARE_WRITE_REQ;
    p[7] = attHandle;
    p[8] = attHandle >> 8;
    p[9] = offset;
    p[10] = offset>>8;
    memcpy (p + 9, pd, len);
}

void att_req_write_cmd (u8 *p, u16 attHandle, u8 *buf, int len)
{
    if (len > 20)
        len = 20;
    p[0] = 2;
    p[1] = 7 + len;
    p[2] = 3 + len;
    p[3] = 0;
    p[4] = L2CAP_CID_ATTR_PROTOCOL;
    p[5] = 0;
    p[6] = ATT_OP_WRITE_CMD;
    p[7] = attHandle;
    p[8] = attHandle >> 8;
    memcpy (p + 9, buf, len);
}


#endif ////ending of if 0
