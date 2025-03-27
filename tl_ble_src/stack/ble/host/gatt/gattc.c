/********************************************************************************************************
 * @file    gattc.c
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
#include "stack/ble/host/gatt/tlk_list_stack.h"
#include "stack/ble/host/gatt/tlk_timer_stack.h"
#include "stack/ble/host/gatt/tlk_malloc_stack.h"

#define GATTC_MALLOC(size)                  malloc_nonreten(size)
#define GATTC_FREE(ptr)                     free_nonreten(ptr)

typedef void (*gatt_rsp_func_t)(u16 connHdl, u8 err, attr_pkt_t *attr, u16 attrLen, void *userData);

typedef struct {
    gatt_rsp_func_t attRspFunc;
    void            *userData; //keep gatt_xxx_params initiator pointer
    u16             expectRecvOp;
    u16             connHandle;
    struct soft_timer gattcReqTimer;
} gattcConn_t;

/*  */
static _attribute_ble_data_retention_
struct single_list gattc_subCccEntry[LL_MAX_ACL_CONN_NUM];

static _attribute_ble_data_retention_
gattcConn_t* gattcConn[LL_MAX_ACL_CONN_NUM] = {NULL, };

static void blt_gattc_aclStateChangeCb(u16 connHandle, GAP_STATE_ENUM state, void* node);

static _attribute_ble_data_retention_
struct gap_stateChangeNode gattcStateChange = {
    .next = NULL,
    .cb = blt_gattc_aclStateChangeCb,
};

////////////////////////// Internal API declarations ///////////////////////////
static void blt_gattc_discoveryNext(u16 connHandle, u16 lastHandle, gattc_sdp_cfg_t *pSdpCfg);
static int blt_gattc_attRequestTimerCb(void* arg);

#define GATTC_SET_PENDING(connHandle, opcode, func, funcData)       \
        gattcConn_t* pGattcConn = GATTC_MALLOC(sizeof(gattcConn_t));    \
        gattcConn[connHandle&0x0f] = pGattcConn;    \
        pGattcConn->attRspFunc = func;  \
        pGattcConn->expectRecvOp = opcode;  \
        pGattcConn->connHandle = connHandle;    \
        pGattcConn->userData = (void*)funcData;     \
        pGattcConn->gattcReqTimer.timer = GATT_PROCEDURE_TIMEOUT;   \
        pGattcConn->gattcReqTimer.arg = pGattcConn; \
        pGattcConn->gattcReqTimer.cb = blt_gattc_attRequestTimerCb; \
        soft_timer_add(&pGattcConn->gattcReqTimer);     \
        blt_gap_regAclConnState(&gattcStateChange)  \


#define GATTC_CHECK_PENDING             \
if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS)  \
    return HCI_ERR_UNKNOWN_CONN_ID; \
else if(gattcConn[connHandle&0x0f]) \
    return GATT_ERR_DATA_PENDING_DUE_TO_SERVICE_DISCOVERY_BUSY

static void blt_gattc_clearAttr(u16 connHandle)
{
    gattcConn_t* pGattcConn = gattcConn[connHandle&0x0f];
    soft_timer_delete(&pGattcConn->gattcReqTimer);
    GATTC_FREE(pGattcConn);
    gattcConn[connHandle&0x0f] = NULL;
    for(unsigned int i=0; i<ARRAY_SIZE(gattcConn); i++)
    {
        if(gattcConn[i])    return ;
    }
    blt_gap_unregAclConnState(&gattcStateChange);
}

static void blt_gattc_aclStateChangeCb(u16 connHandle, GAP_STATE_ENUM state, void* node)
{
    (void)node;
    if(state == GAP_STATE_ACL_DISCONNECTED)
    {
        blt_gattc_clearAttr(connHandle);
    }
}

static int blt_gattc_attRequestTimerCb(void* arg)
{
    gattcConn_t* pGattcConn = (gattcConn_t*) arg;

    u16 connHandle = pGattcConn->connHandle;

    gatt_rsp_func_t pfunc = pGattcConn->attRspFunc;
    void* param = pGattcConn->userData;

    if(pfunc)
    {
        pfunc(connHandle, GATT_TIMEOUT_ATT_ERROR, NULL, 0, param);
    }

    blt_gattc_clearAttr(connHandle);

    return 0;
}

u16 blt_gattc_exchangeMtu_rsp(u16 connHandle, u16 mtu)
{
    gattcConn_t* pGattcConn = gattcConn[connHandle&0x0f];

    if(pGattcConn->expectRecvOp != ATT_OP_EXCHANGE_MTU_RSP) {
        return 0;
    }

    my_dump_str_data(DBG_GATTC_LOG, "[GATTC]---> ATT MTU Size Exchange Response", 0, 0);

    mtu = blt_gap_recvRemoteMtu(connHandle, mtu);

    blt_gattc_clearAttr(connHandle);

    return mtu;
}

u16 blt_gattc_handle_rsp(u16 connHandle, attr_pkt_t *attr, u16 attrLen)
{
    gattcConn_t* pGattcConn = gattcConn[connHandle&0x0f];

    u8 err = ATT_SUCCESS;

    if(attr->opcode == ATT_OP_ERROR_RSP)
    {
        blt_attr_errorRsp_t *pRsp = (blt_attr_errorRsp_t*)attr;
        //if ATT request and response command not -1.
        if(pRsp->reqOpcode != (pGattcConn->expectRecvOp - 1))
        {
            return 0;
        }
        err = pRsp->errorCode;
        my_dump_str_data(DBG_GATTC_LOG, "   [ATT]<--- ATT Error Response", (u8*)&pRsp->errorCode, 1);
        my_dump_str_data(DBG_GATTC_LOG, "       | Request Opcode In Error", (u8*)&pRsp->reqOpcode, 1);
        my_dump_str_data(DBG_GATTC_LOG, "       | Attribute Handle In Error", (u8*)&pRsp->attrHandle, 2);
        my_dump_str_data(DBG_GATTC_LOG, "       | Error Code", (u8*)&pRsp->errorCode, 1);
    }
    else if(attr->opcode != pGattcConn->expectRecvOp)
    {
        return 0;
    }

    gatt_rsp_func_t pfunc = pGattcConn->attRspFunc;
    void* param = pGattcConn->userData;
    blt_gattc_clearAttr(connHandle);

    if(pfunc)
    {
        pfunc(connHandle, err, attr, attrLen, param);
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  GATT_READ_BY_GROUP_TYPE_REQ / GATT_READ_BY_GROUP_TYPE_RSP
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void blt_gattc_readGroupRsp(u16 connHandle, u8 err, attr_pkt_t *pAttrRspPkt, u16 attrLen, void *userData)
{
    my_dump_str_data(DBG_GATTC_LOG, "[SDP]<--- Read By Group Type Response", 0, 0);

    gattc_sdp_cfg_t *pSdpCfg = userData;
    blt_attr_readByGroupTypeRsp_t *rsp = (blt_attr_readByGroupTypeRsp_t*)pAttrRspPkt;
    u16 length = attrLen - OFFSETOF(blt_attr_readByGroupTypeRsp_t, data); /* skip opcode && length */
    u8 pairLen = rsp->length;

    if (err) {
        my_dump_str_data(DBG_GATTC_LOG, "   | err", (u8*)&err, 1);
        goto done;
    }

    uuid_t uuid;

    /* Data can be either in UUID16 or UUID128 */
    switch (pairLen) {
        case 6: /* UUID16: sizeof(attr_group_data_list)+2 = 6 */
            uuid.uuidLen = ATT_16_UUID_LEN;
            my_dump_str_data(DBG_GATTC_LOG, "   |_Pair Length", (u8*)&pairLen, 1);
            break;
        case 20: /* UUID128: sizeof(attr_group_data_list)+16 =20 */
            uuid.uuidLen = ATT_128_UUID_LEN;
            my_dump_str_data(DBG_GATTC_LOG, "   |_Pair Length", (u8*)&pairLen, 1);
            break;
        default:
        my_dump_str_data(DBG_GATTC_LOG, "Invalid data len", (u8*)&rsp->length, 1);
        goto done;
    }

    /*/////////// parse service begin///////////// */
    uuid_t uuid_svc;
    gatt_attr_t attr;
    gatt_service_val_t attrSrvVal;
    u16 startHandle, endHandle = 0;
    struct attr_group_data_list *pAttrGrpData;
    u8 pairCnt = length / pairLen;

    /* Parse services found */
    for (int i = 0; i < pairCnt; i++) {
        pAttrGrpData = (struct attr_group_data_list*)((u8 *)rsp->data + i * pairLen);

        startHandle = pAttrGrpData->startHandle;
        if (!startHandle) {
            goto done;
        }

        endHandle = pAttrGrpData->endHandle;
        if (!endHandle || endHandle < startHandle) {
            goto done;
        }

        my_dump_str_data(DBG_GATTC_LOG, "   |_Triple", (u8*)&i, 1);
        my_dump_str_data(DBG_GATTC_LOG, "    |_Attribute Handle", (u8*)&startHandle, 2);
        my_dump_str_data(DBG_GATTC_LOG, "    |_End Grpup Handle", (u8*)&endHandle, 2);

        memcpy(uuid.uuidVal.u, pAttrGrpData->attrValue, uuid.uuidLen);
        my_dump_str_data(DBG_GATTC_LOG, "    |_Attribute Data", pAttrGrpData->attrValue, uuid.uuidLen);

        uuid_svc.uuidLen = ATT_16_UUID_LEN;
        if (pSdpCfg->type == GATT_DISCOVER_PRIMARY) {
            uuid_svc.uuidVal.u16 = DECLARATIONS_UUID_PRIMARY_SERVICE;
        } else {
            uuid_svc.uuidVal.u16 = DECLARATIONS_UUID_SECONDARY_SERVICE;
        }

        attrSrvVal.endHdl = endHandle;
        attrSrvVal.uuid = &uuid;

        /*-------------------+-----------------------------+-------------------+----------------------+
        | Attribute Handle  |       Attribute Type        | Attribute Value   | Attribute Permission |
        +-------------------+-----------------------------+-------------------+----------------------+
        | 0xNNNN            |      0x2800 - UUID for      |                   | Read Only,           |
        |                   |<<Primary Service>> OR 0x2801| 16-bit UUID or    | No Authentication,   |
        |                   |for <<Secondary Service>>    | 128-bit UUID      |     No Authorization |
        +-------------------+-----------------------------+-------------------+----------------------+
        | Service declaration                                                                        |
        +--------------------------------------------------------------------------------------------*/

        attr = (gatt_attr_t) {
            /* Attribute_handle: handle */
            .handle = startHandle,
            /* Initialize Attribute_types: UUID */
            .uuid = (uuid_t*)&uuid_svc,
            /* Attribute_value: User data */
            .user_data = &attrSrvVal,
        };

        if (pSdpCfg->func(connHandle, &attr, pSdpCfg) == GATT_PROC_END) {
            return;
        }
    }
    /*/////////// parse service end///////////// */

    blt_gattc_discoveryNext(connHandle, endHandle, pSdpCfg);
    return;

done:
    pSdpCfg->func(connHandle, NULL, pSdpCfg);
}

static ble_sts_t blt_gattc_readGroupReq(u16 connHandle, gattc_sdp_cfg_t *pSdpCfg)
{
    my_dump_str_data(DBG_GATTC_LOG, "[SDP]---> Read By Group Type Request", 0, 0);

    u8 uuidLen;
    u16 uuidVal,startAttHdl, endAttHdl;
    ble_sts_t status = BLE_SUCCESS;

    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    startAttHdl = pSdpCfg->startHdl;
    endAttHdl = pSdpCfg->endHdl;

    my_dump_str_data(DBG_GATTC_LOG, "   |_Starting Handle", (u8*)&startAttHdl, 2);
    my_dump_str_data(DBG_GATTC_LOG, "   |_Ending Handle", (u8*)&endAttHdl, 2);

    if (pSdpCfg->type == GATT_DISCOVER_PRIMARY) {
        uuidLen = ATT_16_UUID_LEN;
        uuidVal = DECLARATIONS_UUID_PRIMARY_SERVICE;
        my_dump_str_data(DBG_GATTC_LOG, "   |_Attribute Group Type: Primary Service", 0, 0);
    } else { // GATT_DISCOVER_SECONDARY
        uuidLen = ATT_16_UUID_LEN;
        uuidVal = DECLARATIONS_UUID_SECONDARY_SERVICE;
        my_dump_str_data(DBG_GATTC_LOG, "   |_Attribute Group Type: 2nd Service", 0, 0);
    }

    status = blc_attc_sendReadByGroupTypeRequest (connHandle, startAttHdl, endAttHdl, (u8*)&uuidVal, uuidLen);

    if (status == BLE_SUCCESS) {
        GATTC_SET_PENDING(connHandle, ATT_OP_READ_BY_GROUP_TYPE_RSP, blt_gattc_readGroupRsp, pSdpCfg);
    }

    return status;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  GATT_FIND_BY_TYPE_VALUE_REQ / GATT_FIND_BY_TYPE_VALUE_RSP
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void blt_gattc_findByTypeRsp(u16 connHandle, u8 err, attr_pkt_t *pAttrRspPkt, u16 attrLen, void *userData)
{
    my_dump_str_data(DBG_GATTC_LOG, "[SDP]<--- Find By Type Value Response", pAttrRspPkt, attrLen);

    gattc_sdp_cfg_t *pSdpCfg = userData;
    blt_attr_findByTypeValueRsp_t *rsp = (blt_attr_findByTypeValueRsp_t*)pAttrRspPkt;
    u16 length = attrLen - 1; /* skip opcode t*/

    if (err) {
        my_dump_str_data(DBG_GATTC_LOG, "   | err", (u8*)&err, 1);
        goto done;
    }

    u16 startAttHdl, endAttHdl = 0;
    u8 pairCnt = length / sizeof(struct attr_handle_group);
    uuid_t uuid_svc;
    gatt_attr_t attr;
    gatt_service_val_t attrSrvVal;
    struct attr_handle_group *attrHdlGrp = rsp->list;

    /*/////////// parse attributes found begin///////////// */
    for (int i = 0; i < pairCnt; i++) {

        startAttHdl = attrHdlGrp[i].startHandle;
        endAttHdl = attrHdlGrp[i].endHandle;

        my_dump_str_data(DBG_GATTC_LOG, "       |_Pair", (u8*)&i, 1);
        my_dump_str_data(DBG_GATTC_LOG, "    |_Starting Handle", (u8*)&startAttHdl, 2);
        my_dump_str_data(DBG_GATTC_LOG, "    |_Ending Handle", (u8*)&endAttHdl, 2);

        uuid_svc.uuidLen = ATT_16_UUID_LEN;
        if (pSdpCfg->type == GATT_DISCOVER_PRIMARY) {
            uuid_svc.uuidVal.u16 = DECLARATIONS_UUID_PRIMARY_SERVICE;
        } else {
            uuid_svc.uuidVal.u16 = DECLARATIONS_UUID_SECONDARY_SERVICE;
        }

        attrSrvVal.endHdl = endAttHdl;
        attrSrvVal.uuid = pSdpCfg->uuid;

        /*-------------------+-----------------------------+-------------------+------------------------+
        | Attribute Handle  |       Attribute Type        | Attribute Value   | Attribute Permission   |
        +-------------------+-----------------------------+-------------------+------------------------+
        | 0xNNNN            |      0x2800 - UUID for      |                   | Read Only,             |
        |                   |<<Primary Service>> OR 0x2801| 16-bit UUID or    | No Authentication,     |
        |                   |for <<Secondary Service>>    | 128-bit UUID      |     No Authorization   |
        +-------------------+-----------------------------+-------------------+------------------------+
        | Service declaration                                                                          |
        +----------------------------------------------------------------------------------------------*/

        attr = (gatt_attr_t) {
            /* Attribute_handle: handle */
            .handle = startAttHdl,
            /* Initialize Attribute_types: UUID */
            .uuid = (uuid_t*)&uuid_svc,
            /* Attribute_value: User data */
            .user_data = &attrSrvVal,
        };

        if (pSdpCfg->func(connHandle, &attr, pSdpCfg) == GATT_PROC_END) {
            return;
        }
    }
    /*/////////// parse attributes found end ///////////// */

    blt_gattc_discoveryNext(connHandle, endAttHdl, pSdpCfg);
    return;

done:
    pSdpCfg->func(connHandle, NULL, pSdpCfg);
}

static ble_sts_t blt_gattc_findTypeReq(u16 connHandle, gattc_sdp_cfg_t *pSdpCfg)
{
    u8 attrValLen;
    u8 *pAttrVal = NULL;
    u16 uuidVal, startAttHdl, endAttHdl;
    ble_sts_t status = BLE_SUCCESS;

    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    my_dump_str_data(DBG_GATTC_LOG, "[SDP]---> Find_By_Type_Value_Request", 0, 0);

    startAttHdl = pSdpCfg->startHdl;
    endAttHdl = pSdpCfg->endHdl;

    my_dump_str_data(DBG_GATTC_LOG, "   |_Starting Handle", (u8*)&startAttHdl, 2);
    my_dump_str_data(DBG_GATTC_LOG, "   |_Ending Handle", (u8*)&endAttHdl, 2);

    /* Attribute Type */
    if (pSdpCfg->type == GATT_DISCOVER_PRIMARY) {
        uuidVal = DECLARATIONS_UUID_PRIMARY_SERVICE;
        my_dump_str_data(DBG_GATTC_LOG, "   |_Attribute Type: Primary Service", 0, 0);
    } else {
        uuidVal = DECLARATIONS_UUID_SECONDARY_SERVICE;
        my_dump_str_data(DBG_GATTC_LOG, "   |_Attribute Type: Secondary Service", 0, 0);
    }

    /* Attribute Value */
    switch (pSdpCfg->uuid->uuidLen) {
        case ATT_16_UUID_LEN:
        case ATT_128_UUID_LEN:
            attrValLen = pSdpCfg->uuid->uuidLen;
            pAttrVal = (u8*)pSdpCfg->uuid->uuidVal.u;
            break;
        default:
            my_dump_str_data(DBG_GATTC_LOG, "Unknown UUID", pSdpCfg->uuid->uuidVal.u, pSdpCfg->uuid->uuidLen);
            return GATT_ERR_INVALID_PARAMETER;
    }

    my_dump_str_data(DBG_GATTC_LOG, "   |_UUID", pAttrVal, attrValLen);

    status = blc_attc_sendFindByTypeValueRequest(connHandle, startAttHdl, endAttHdl, uuidVal, pAttrVal, attrValLen);

    if (status == BLE_SUCCESS) {
        GATTC_SET_PENDING(connHandle, ATT_OP_FIND_BY_TYPE_VALUE_RSP, blt_gattc_findByTypeRsp, pSdpCfg);
    }

    return status;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  GATT_FIND_INFORMATION_REQ / GATT_FIND_INFORMATION_RSP
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void blt_gattc_findInfoRsp(u16 connHandle, u8 err, attr_pkt_t *pAttrRspPkt, u16 attrLen, void *userData)
{
    my_dump_str_data(DBG_GATTC_LOG, "[SDP]<--- Find Information Response", 0, 0);

    gattc_sdp_cfg_t *pSdpCfg = userData;

    if (err) {
        my_dump_str_data(DBG_GATTC_LOG, "   | err", (u8*)&err, 1);
        goto done;
    }

    blt_attr_findInfoRsp_t *rsp = (blt_attr_findInfoRsp_t*)pAttrRspPkt;
    u16 length = attrLen - OFFSETOF(blt_attr_findInfoRsp_t, infoData); /* skip opcode && format*/
    u16 len, attrHdl = 0;

    uuid_t uuid;

    union {
        struct att_info16 *i16;
        struct att_info128 *i128;
    } info;

    /* Data can be either in UUID16 or UUID128 */
    switch (rsp->format) {
        case ATT_INFO_FORMAT_16:
            uuid.uuidLen = ATT_16_UUID_LEN;
            len = sizeof(*info.i16);
            my_dump_str_data(DBG_GATTC_LOG, "   |_Format: 16bit UUIDs", 0, 0);
            break;
        case ATT_INFO_FORMAT_128:
            uuid.uuidLen = ATT_128_UUID_LEN;
            len = sizeof(*info.i128);
            my_dump_str_data(DBG_GATTC_LOG, "   |_Format: 128bit UUIDs", 0, 0);
            break;
        default:
            my_dump_str_data(DBG_GATTC_LOG, "Invalid format", (u8*)&rsp->format, 1);
            goto done;
    }

    /* Check if there is a least one descriptor in the response */
    if (length < len) {
        goto done;
    }

    /*/////////// parse descriptors found begin///////////// */
    int i;
    u8 *infoData;
    gatt_attr_t attr;
    bool skip = false;
    u8 pairCnt = 0; /* debug used only */
    for (i = length / len, infoData = rsp->infoData; i != 0; \
         i--, infoData = (u8 *)infoData + len) {

        info.i16 = (struct att_info16 *)infoData;
        attrHdl = info.i16->handle;

        pairCnt++;

        if (skip) {
            skip = false;
            continue;
        }

        switch (uuid.uuidLen) {
            case ATT_16_UUID_LEN:
                uuid.uuidVal.u16 = info.i16->uuid;
                break;
            case ATT_128_UUID_LEN:
                memcpy(uuid.uuidVal.u128, info.i128->uuid, 16);
                break;
            default:
                goto done;
        }

        my_dump_str_data(DBG_GATTC_LOG, "       |_Pair", &pairCnt, 1);
        my_dump_str_data(DBG_GATTC_LOG, "    |_Attribute Handle", (u8*)&attrHdl, 2);
        my_dump_str_data(DBG_GATTC_LOG, "    |_UUID", uuid.uuidVal.u, uuid.uuidLen);

        /* Skip if UUID is set but doesn't match */
//      if (pSdpCfg->uuid && blc_uuid_cmp(&uuid, pSdpCfg->uuid)) {
//          continue;
//      }

        if (pSdpCfg->type == GATT_DISCOVER_DESCRIPTOR) {
            /* Skip attributes that are not considered descriptors. */
            if (!blc_uuid_cmp(&uuid, UUID16_DEF(DECLARATIONS_UUID_PRIMARY_SERVICE)) ||
                !blc_uuid_cmp(&uuid, UUID16_DEF(DECLARATIONS_UUID_SECONDARY_SERVICE)) ||
                !blc_uuid_cmp(&uuid, UUID16_DEF(DECLARATIONS_UUID_INCLUDE))) {
                continue;
            }

            /* If Characteristic Declaration skip ahead as the next entry must be its value. */
            if (!blc_uuid_cmp(&uuid, UUID16_DEF(DECLARATIONS_UUID_CHARACTERISTIC))) {
                skip = true;
                continue;
            }
        }

        attr = (gatt_attr_t) {
            /* Attribute_handle: handle */
            .handle = attrHdl,
            /* Attribute_types: UUID */
            .uuid = &uuid,
            /* No user_data in this case */
        };

        if (pSdpCfg->func(connHandle, &attr, pSdpCfg) == GATT_PROC_END) {
            return;
        }
    }
    /*/////////// parse descriptors found end///////////// */

    blt_gattc_discoveryNext(connHandle, attrHdl, pSdpCfg);
    return;

done:
    pSdpCfg->func(connHandle, NULL, pSdpCfg);
}

static ble_sts_t blt_gattc_findInfoReq(u16 connHandle, gattc_sdp_cfg_t *pSdpCfg)
{
    u16 startAttHdl, endAttHdl;
    ble_sts_t status = BLE_SUCCESS;

    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    my_dump_str_data(DBG_GATTC_LOG, "[SDP]---> Find Information Request", 0, 0);

    startAttHdl = pSdpCfg->startHdl;
    endAttHdl = pSdpCfg->endHdl;

    my_dump_str_data(DBG_GATTC_LOG, "   |_Starting Handle", (u8*)&startAttHdl, 2);
    my_dump_str_data(DBG_GATTC_LOG, "   |_Ending Handle", (u8*)&endAttHdl, 2);

    status = blc_attc_sendFindInfoRequest(connHandle, startAttHdl, endAttHdl);

    if (status == BLE_SUCCESS) {
        GATTC_SET_PENDING(connHandle, ATT_OP_FIND_INFO_RSP, blt_gattc_findInfoRsp, pSdpCfg);
    }

    return status;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  GATT_READ_BY_TYPE_REQ / GATT_READ_BY_TYPE_RSP
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void blt_read_incUuid128Rsp(u16 connHandle, u8 err, attr_pkt_t *pAttrRspPkt, u16 attrLen, void *userData)
{
    my_dump_str_data(DBG_GATTC_LOG, "[SDP]<--- Read Response (Include UUID_128)", 0, 0);

    gatt_attr_t attr;
    gattc_sdp_cfg_t *pSdpCfg = userData;
    blt_attr_readRsp_t *rsp = (blt_attr_readRsp_t*)pAttrRspPkt;
    u16 length = attrLen - 1; /* skip opcode */

    if (err || length != ATT_128_UUID_LEN) {
        my_dump_str_data(DBG_GATTC_LOG, "   | err", (u8*)&err, 1);
        pSdpCfg->func(connHandle, NULL, pSdpCfg);
        return;
    }

    uuid_t uuid;
    u16 attrHdl = pSdpCfg->_included.attrHdl;
    uuid.uuidLen = ATT_128_UUID_LEN;
    memcpy(uuid.uuidVal.u128, rsp->value, ATT_128_UUID_LEN);

    /* Skip if UUID is set but doesn't match */
    if (pSdpCfg->uuid && blc_uuid_cmp(&uuid, pSdpCfg->uuid)) {
        goto next;
    }

    gatt_include_t incl;
    incl.startHdl = pSdpCfg->_included.startHdl;
    incl.endHdl = pSdpCfg->_included.endHdl;
    incl.uuid = uuid;

    /* Handle-Value pair:  att_hdl + include_declaration
     *                       (2B)           |_ startHdl (2B)
     *                                      |_ endHdl (2B)
     *                                      |_ UUID (16B)
     */
    my_dump_str_data(DBG_GATTC_LOG, "   |_Attribute Handle", (u8*)&attrHdl, 2);
    my_dump_str_data(DBG_GATTC_LOG, "   |_Include_Declaration",0, 0);
    my_dump_str_data(DBG_GATTC_LOG, "     |_Starting Handle", (u8*)&incl.startHdl, 2);
    my_dump_str_data(DBG_GATTC_LOG, "     |_Ending Handle", (u8*)&incl.endHdl, 2);
    my_dump_str_data(DBG_GATTC_LOG, "     |_UUID", incl.uuid.uuidVal.u, incl.uuid.uuidLen);

    /*-------------------+---------------------------+---------------------------+------------------------+
    | Attribute Handle  |       Attribute Type      |   Attribute Value         |   Attribute Permission |
    *-------------------+---------------------------+---------------------------+------------------------+
    | 0xNNNN            |0x2802 - UUID for          |Included  | End  | Service |       Read Only,       |
    |                   |  <<Include>>              |Service   |Group | UUID    |   No Authentication,   |
    |                   |                           |Attribute |Handle|         |   No Authorization     |
    |                   |                           |Handle    |      |         |                        |
    +-------------------+---------------------------+----------+------+---------+------------------------+
    | Include declaration                                                                                |
    +----------------------------------------------------------------------------------------------------*/

    attr = (gatt_attr_t) {
        /* Attribute_handle: handle */
        .handle = attrHdl,
        /* Attribute_types: UUID */
        .uuid = UUID16_DEF(DECLARATIONS_UUID_INCLUDE),
        /* Attribute_value: User data */
        .user_data = &incl,
    };

    if (pSdpCfg->func(connHandle, &attr, pSdpCfg) == GATT_PROC_END) {
        return;
    }

next:
    blt_gattc_discoveryNext(connHandle, attrHdl, pSdpCfg);
    return;
}

static ble_sts_t blt_read_incUuid128Req(u16 connHandle, gattc_sdp_cfg_t *pSdpCfg)
{
    ble_sts_t status = BLE_SUCCESS;

    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    my_dump_str_data(DBG_GATTC_LOG, "[SDP]---> Read Request(Include UUID_128)", 0, 0);

    u16 startAttHdl = pSdpCfg->_included.startHdl;
    my_dump_str_data(DBG_GATTC_LOG, "   |_Attribute Handle", (u8*)&pSdpCfg->startHdl, 2);
    status = blc_attc_sendReadRequest (connHandle, startAttHdl);

    if (status == BLE_SUCCESS) {
        GATTC_SET_PENDING(connHandle, ATT_OP_READ_RSP, blt_read_incUuid128Rsp, pSdpCfg);
    }

    return status;
}

static void blt_gattc_readTypeRsp(u16 connHandle, u8 err, attr_pkt_t *pAttrRspPkt, u16 attrLen, void *userData)
{
    my_dump_str_data(DBG_GATTC_LOG, "[SDP]<--- Read By Type Response", 0, 0);

    gattc_sdp_cfg_t *pSdpCfg = userData;
    blt_attr_readByTypeRsp_t *rsp = (blt_attr_readByTypeRsp_t*)pAttrRspPkt;
    u16 length = attrLen - OFFSETOF(blt_attr_readByTypeRsp_t, list); /* skip opcode && length */
    u8 pairLen = rsp->length;

    if (err) {
        my_dump_str_data(DBG_GATTC_LOG, "   | err", (u8*)&err, 1);
        goto done;
    }

    u16 attrHdl = 0;
    gatt_attr_t attr;
    u8 pairCnt = length / pairLen;
    struct attr_data_list *pAttrDataList;
    uuid_t uuid;

    if (pSdpCfg->type == GATT_DISCOVER_INCLUDE) {
        /*/////////// parse include service begin ///////////// */
#if (1)
        /* Data can be either in UUID16 or UUID128 */
        switch (pairLen) {
            case 8: /* UUID16 */
                uuid.uuidLen = ATT_16_UUID_LEN;
            break;
            case 6: /* UUID128 (not contain UUID field)*/
                uuid.uuidLen = ATT_128_UUID_LEN;
                /* Core 5.3 | Vol 3, Part G, page 1501
                * To get the included service UUID when the included service uses a 128-bit UUID,
                * the ATT_READ_REQ PDU is used. The Attribute Handle for the ATT_READ_REQ PDU is
                * the Attribute Handle of the included service.
                */
            break;
            default:
                my_dump_str_data(DBG_GATTC_LOG, "Invalid data len", (u8*)&pairLen, 1);
                goto done;
        }

        gatt_include_t incl;

        /* Parse characteristics found */
        for (int i = 0; i< pairCnt; i++) {
            pAttrDataList = (struct attr_data_list *)((u8*)rsp->list + i * pairLen);

            /* Attribute Handle */
            attrHdl = pAttrDataList->handle;
            /* Handle 0 is invalid */
            if (attrHdl == ATT_HANDLE_NONE) {
                goto done;
            }

            struct incl_attr_data {
                /* Service start handle. */
                u16 startHdl;
                /* Service end handle. */
                u16 endHdl;
                /* Service UUID. */
                union {
                    u8 u[0];
                    u16 uuid16;
                    u8 uuid128[ATT_128_UUID_LEN];
                };
            } * pInclAttrData __attribute__((packed));

            pInclAttrData = (struct incl_attr_data *)pAttrDataList->value;
            incl.startHdl = pInclAttrData->startHdl;
            incl.endHdl = pInclAttrData->endHdl;

            if (uuid.uuidLen == ATT_16_UUID_LEN) {
                uuid.uuidVal.u16 = pInclAttrData->uuid16;
                incl.uuid = uuid;
                /* Skip if UUID is set but doesn't match */
                if (pSdpCfg->uuid && blc_uuid_cmp(&uuid, pSdpCfg->uuid)) {
                    continue;
                }
            } else { /* uuid.uuidLen == ATT_128_UUID_LEN */
                pSdpCfg->_included.attrHdl = attrHdl;
                pSdpCfg->_included.startHdl = pInclAttrData->startHdl;
                pSdpCfg->_included.endHdl = pInclAttrData->endHdl;

                if(blt_read_incUuid128Req(connHandle, pSdpCfg) != BLE_SUCCESS){
                    my_dump_str_data(DBG_GATTC_LOG, "[incl]read UUID128 failed", (u8*)&incl.startHdl, 2);
                    goto done;
                }
            }

            /* Handle-Value pair:  att_hdl + include_declaration
             *                       (2B)           |_ startHdl (2B)
             *                                      |_ endHdl (2B)
             *                                      |_ UUID (2B)
             */
            my_dump_str_data(DBG_GATTC_LOG, "   |_Pair", (u8*)&i, 1);
            my_dump_str_data(DBG_GATTC_LOG, "    |_Attribute Handle", (u8*)&attrHdl, 2);
            my_dump_str_data(DBG_GATTC_LOG, "    |_Include_Declaration",0, 0);
            my_dump_str_data(DBG_GATTC_LOG, "      | Starting Handle", (u8*)&incl.startHdl, 2);
            my_dump_str_data(DBG_GATTC_LOG, "      | Ending Handle", (u8*)&incl.endHdl, 2);
            my_dump_str_data(DBG_GATTC_LOG, "      | UUID", incl.uuid.uuidVal.u, incl.uuid.uuidLen);

            /*-------------------+---------------------------+---------------------------+------------------------+
            | Attribute Handle  |       Attribute Type      |   Attribute Value         |   Attribute Permission |
            *-------------------+---------------------------+---------------------------+------------------------+
            | 0xNNNN            |0x2802 - UUID for          |Included  | End  | Service |       Read Only,       |
            |                   |  <<Include>>              |Service   |Group | UUID    |   No Authentication,   |
            |                   |                           |Attribute |Handle|         |   No Authorization     |
            |                   |                           |Handle    |      |         |                        |
            +-------------------+---------------------------+----------+------+---------+------------------------+
            | Include declaration                                                                                |
            +----------------------------------------------------------------------------------------------------*/

            attr = (gatt_attr_t) {
                /* Attribute_handle: handle */
                .handle = attrHdl,
                /* Attribute_types: UUID */
                .uuid = UUID16_DEF(DECLARATIONS_UUID_INCLUDE),
                /* Attribute_value: User data */
                .user_data = &incl,
            };

            if (pSdpCfg->func(connHandle, &attr, pSdpCfg) == GATT_PROC_END) {
                return;
            }
        }
        /*/////////// parse include service end///////////// */
#endif
    } else if (pSdpCfg->type == GATT_DISCOVER_CHARACTERISTIC) {
        /*/////////// parse characteristic begin ///////////// */
#if (1)
        /* Data can be either in UUID16 or UUID128 */
        switch (pairLen) {
            case 7: /* UUID16 */
                uuid.uuidLen = ATT_16_UUID_LEN;
                break;
            case 21: /* UUID128 */
                uuid.uuidLen = ATT_128_UUID_LEN;
                break;
            default:
                my_dump_str_data(DBG_GATTC_LOG, "Invalid data len", (u8*)&pairLen, 1);
                goto done;
        }

        gatt_chrc_t chrc;

        /* Parse characteristics found */
        for (int i = 0; i< pairCnt; i++) {
            pAttrDataList = (struct attr_data_list *)((u8*)rsp->list + i * pairLen);

            /* Attribute Handle */
            attrHdl = pAttrDataList->handle;
            /* Handle 0 is invalid */
            if (attrHdl == ATT_HANDLE_NONE) {
                goto done;
            }

            struct chrc_attr_data_t {
                /** GATT Characteristic Properties. */
                u8  properties;
                /** GATT Characteristic Value Attribute Handle. */
                u16 valueHdl;
                /** GATT Characteristic UUID. */
                union {
                    u8 u[0];
                    u16 uuid16;
                    u8 uuid128[ATT_128_UUID_LEN];
                };
            } *pChrcAttVal __attribute__((packed));

            pChrcAttVal = (struct chrc_attr_data_t *)pAttrDataList->value;

            /* Convert 'struct chrc_attr_data_t' to 'gatt_chrc_t' */
            chrc.attrHdl = pAttrDataList->handle;
            chrc.properties = pChrcAttVal->properties;
            chrc.valueHdl = pChrcAttVal->valueHdl;

            switch (uuid.uuidLen) {
                case ATT_16_UUID_LEN:
                    uuid.uuidVal.u16 = pChrcAttVal->uuid16;
                    break;
                case ATT_128_UUID_LEN:
                    memcpy(uuid.uuidVal.u128, pChrcAttVal->uuid128, ATT_128_UUID_LEN);
                    break;
                default:
                    goto done;
            }

            chrc.uuid = uuid;

            /* Characteristics-Value pair:  att_hdl + characteristics_declaration
             *                              (2B)            |_ properties (1B)
             *                                              |_ value_handle (2B)
             *                                              |_ UUID (2B OR 16B)
             */
            my_dump_str_data(DBG_GATTC_LOG, "   |_Pair", (u8*)&i, 1);
            my_dump_str_data(DBG_GATTC_LOG, "    |_Attribute Handle", (u8*)&attrHdl, 2);
            my_dump_str_data(DBG_GATTC_LOG, "    |_Chrc Properties", (u8*)&chrc.properties, 1);
            my_dump_str_data(DBG_GATTC_LOG, "    |_Chrc Value Handle", (u8*)&chrc.valueHdl, 2);
            my_dump_str_data(DBG_GATTC_LOG, "    |_Chrc UUID", chrc.uuid.uuidVal.u, uuid.uuidLen);

            /* Skip if UUID is set but doesn't match */
//          if (pSdpCfg->uuid && blc_uuid_cmp(&uuid, pSdpCfg->uuid)) {
//              continue;
//          }

            /*-------------------+---------------------------+------------------------------+------------------------+
            | Attribute Handle  |       Attribute Type      |   Attribute Value            |  Attribute Permission  |
            *-------------------+---------------------------+----------+---------+---------+------------------------+
            | 0xNNNN            |    0x2803 - UUID for      |Charac-   | Charac- | Charac- |    Read Only,          |
            |                   |    <<Characteristic>>     |teristic  |teristic |teristic |    No Authentication,  |
            |                   |                           |Properties|Attribute|  UUID   |   No Authorization     |
            |                   |                           |          | Handle  |         |                        |
            +-------------------+---------------------------+----------+---------+---------+------------------------+
            | Characteristic declaration                                                                            |
            +-------------------------------------------------------------------------------------------------------*/

            attr = (gatt_attr_t) {
                /* Attribute_handle: handle */
                .handle = attrHdl,
                /* Attribute_types: UUID */
                .uuid = UUID16_DEF(DECLARATIONS_UUID_CHARACTERISTIC),
                /* Attribute_value: User data */
                .user_data = &chrc,
            };

            if (pSdpCfg->func(connHandle, &attr, pSdpCfg) == GATT_PROC_END) {
                return;
            }
        }
        /*/////////// parse characteristic end///////////// */
#endif
    } else {
#if (1)
        /*/////////// parse standard characteristic descriptor begin ///////////// */
        if (pSdpCfg->uuid->uuidLen != ATT_16_UUID_LEN) {
            goto done;
        }

        union {
            gatt_ccc_t ccc;
            gatt_cpf_t cpf;
            gatt_cep_t cep;
            gatt_scc_t scc;
        } attrVal;

        u16 uuidVal = pSdpCfg->uuid->uuidVal.u16;

        /* Parse characteristics found */
        for (int i = 0; i< pairCnt; i++) {

            pAttrDataList = (struct attr_data_list *)((u8*)rsp->list + i * pairLen);
            /* Attribute Value */
            attrHdl = pAttrDataList->handle;
            /* Handle 0 is invalid */
            if (attrHdl == ATT_HANDLE_NONE) {
                goto done;
            }

            my_dump_str_data(DBG_GATTC_LOG, "   |_Pair", (u8*)&i, 1);
            my_dump_str_data(DBG_GATTC_LOG, "    |_Attribute Value", (u8*)&attrHdl, 2);

            switch (uuidVal) {
                case DESCRIPTOR_UUID_CHARACTERISTIC_EXTENDED_PROPERTIES: {
                    gatt_cep_t *cep = (gatt_cep_t *)pAttrDataList->value;
                    attrVal.cep.properties = cep->properties;
                    my_dump_str_data(DBG_GATTC_LOG, "    |_Chrc Extended Properties", (u8*)&cep->properties, 2);
                }
                    break;
                case DESCRIPTOR_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION: {
                    gatt_ccc_t *ccc = (gatt_ccc_t *)pAttrDataList->value;
                    attrVal.ccc.flags = ccc->flags;
                    my_dump_str_data(DBG_GATTC_LOG, "    |_Client Chrc  Config flags", (u8*)&ccc->flags, 2);
                }
                    break;
                case DESCRIPTOR_UUID_SERVER_CHARACTERISTIC_CONFIGURATION: {
                    gatt_scc_t *scc = (gatt_scc_t *)pAttrDataList->value;
                    attrVal.scc.flags = scc->flags;
                    my_dump_str_data(DBG_GATTC_LOG, "    |_Server Chrc Config flags", (u8*)&scc->flags, 2);
                }
                    break;
                case DESCRIPTOR_UUID_CHARACTERISTIC_PRESENTATION_FORMAT: {
                    gatt_cpf_t *cpf = (gatt_cpf_t *)pAttrDataList->value;
                    attrVal.cpf.format = cpf->format;
                    attrVal.cpf.exponent = cpf->exponent;
                    attrVal.cpf.unit = cpf->unit;
                    attrVal.cpf.name_space = cpf->name_space;
                    attrVal.cpf.description = cpf->description;
                    my_dump_str_data(DBG_GATTC_LOG, "    |_Chrc Presentation Format", 0, 0);
                    my_dump_str_data(DBG_GATTC_LOG, "      |_Format", (u8*)&cpf->format, 1);
                    my_dump_str_data(DBG_GATTC_LOG, "      |_Exponent", (u8*)&cpf->exponent, 1);
                    my_dump_str_data(DBG_GATTC_LOG, "      |_Unit", (u8*)&cpf->unit, 2);
                    my_dump_str_data(DBG_GATTC_LOG, "      |_Name space", (u8*)&cpf->name_space, 1);
                    my_dump_str_data(DBG_GATTC_LOG, "      |_Description", (u8*)&cpf->description, 2);
                }
                    break;
                default:
                    my_dump_str_data(DBG_GATTC_LOG, "Unsupported chrc descriptor UUIDs", (u8*)&uuidVal, 2);
                    goto done;
            }

            attr = (gatt_attr_t) {
                .uuid = pSdpCfg->uuid,
                .user_data = &attrVal,
                .handle = attrHdl,
            };

            if (pSdpCfg->func(connHandle, &attr, pSdpCfg) == GATT_PROC_END) {
                return;
            }
        }
        /*/////////// parse standard characteristic descriptor end ///////////// */
#endif
    }

    blt_gattc_discoveryNext(connHandle, attrHdl, pSdpCfg);
    return;

done:
    pSdpCfg->func(connHandle, NULL, pSdpCfg);
}

static ble_sts_t blt_gattc_readTypeReq(u16 connHandle, gattc_sdp_cfg_t *pSdpCfg)
{
    u8 uuidLen;
    u16 uuidVal, startAttHdl, endAttHdl;
    ble_sts_t status = BLE_SUCCESS;

    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    my_dump_str_data(DBG_GATTC_LOG, "[SDP]---> Read By Type Request", 0, 0);

    startAttHdl = pSdpCfg->startHdl;
    endAttHdl = pSdpCfg->endHdl;

    my_dump_str_data(DBG_GATTC_LOG, "   |_Starting Handle", (u8*)&startAttHdl, 2);
    my_dump_str_data(DBG_GATTC_LOG, "   |_Ending Handle", (u8*)&endAttHdl, 2);

    switch (pSdpCfg->type) {
        case GATT_DISCOVER_INCLUDE:
            uuidLen = ATT_16_UUID_LEN;
            uuidVal = DECLARATIONS_UUID_INCLUDE;
            my_dump_str_data(DBG_GATTC_LOG, "   |_Attribute Type: Include Service", 0, 0);
            break;
        case GATT_DISCOVER_CHARACTERISTIC:
            uuidLen = ATT_16_UUID_LEN;
            uuidVal = DECLARATIONS_UUID_CHARACTERISTIC;
            my_dump_str_data(DBG_GATTC_LOG, "   |_Attribute Type: Characteristic", 0, 0);
            break;
        default: /* Only 16-bit UUIDs supported */
            uuidLen = ATT_16_UUID_LEN;
            uuidVal = pSdpCfg->uuid->uuidVal.u16;
            my_dump_str_data(DBG_GATTC_LOG, "   |_Attribute Type: Others 16-bit UUIDs", &uuidVal, 2);
            break;
    }

    status = blc_attc_sendReadByTypeRequest (connHandle, startAttHdl, endAttHdl, (u8*)&uuidVal, uuidLen);

    if (status == BLE_SUCCESS) {
        GATTC_SET_PENDING(connHandle, ATT_OP_READ_BY_TYPE_RSP, blt_gattc_readTypeRsp, pSdpCfg);
    }

    return status;
}




/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  GATT_DISCOVERY_API FOR USERS
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void blt_gattc_discoveryNext(u16 connHandle, u16 lastHandle, gattc_sdp_cfg_t *pSdpCfg)
{
    /* Skip if last_handle is not set */
    if (lastHandle == ATT_HANDLE_NONE)
        goto discover;

    /* Continue from the last found handle */
    pSdpCfg->startHdl = lastHandle;
    if (pSdpCfg->startHdl < ATT_HANDLE_MAX) {
        pSdpCfg->startHdl++;
    } else {
        goto done;
    }

    /* Stop if over the range or the requests */
    if (pSdpCfg->startHdl > pSdpCfg->endHdl) {
        goto done;
    }

    discover:
    /* Discover next range */
    if (!blc_gattc_discovery(connHandle, pSdpCfg)) {
        return;
    }

done:
    pSdpCfg->func(connHandle, NULL, pSdpCfg);
}

ble_sts_t blc_gattc_discovery(u16 connHandle, gattc_sdp_cfg_t *pSdpCfg)
{
    /* Parameters check */
    GATTC_CHECK_PENDING;

    if ((pSdpCfg == NULL) ||  \
        (pSdpCfg->startHdl == ATT_HANDLE_NONE|| pSdpCfg->endHdl == ATT_HANDLE_NONE) || \
        (pSdpCfg->startHdl > pSdpCfg->endHdl)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    ble_sts_t state = GATT_ERR_INVALID_PARAMETER;

    switch (pSdpCfg->type) {
        case GATT_DISCOVER_PRIMARY:
        case GATT_DISCOVER_SECONDARY:
        {
            state = pSdpCfg->uuid? blt_gattc_findTypeReq(connHandle, pSdpCfg): blt_gattc_readGroupReq(connHandle, pSdpCfg);
        }break;
        case GATT_DISCOVER_STD_CHAR_DESC:
        {
            if (!(pSdpCfg->uuid && pSdpCfg->uuid->uuidLen == ATT_16_UUID_LEN &&
                (!blc_uuid_cmp(pSdpCfg->uuid, UUID16_DEF(DESCRIPTOR_UUID_CHARACTERISTIC_EXTENDED_PROPERTIES)) ||
                !blc_uuid_cmp(pSdpCfg->uuid, UUID16_DEF(DESCRIPTOR_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION)) ||
                !blc_uuid_cmp(pSdpCfg->uuid, UUID16_DEF(DESCRIPTOR_UUID_SERVER_CHARACTERISTIC_CONFIGURATION)) ||
                !blc_uuid_cmp(pSdpCfg->uuid, UUID16_DEF(DESCRIPTOR_UUID_CHARACTERISTIC_PRESENTATION_FORMAT))))) {
                state = GATT_ERR_INVALID_PARAMETER;
        }
        else
        {
            state = blt_gattc_readTypeReq(connHandle, pSdpCfg);
        }
        }break;
        case GATT_DISCOVER_INCLUDE:
        case GATT_DISCOVER_CHARACTERISTIC:
        {
            state = blt_gattc_readTypeReq(connHandle, pSdpCfg);
        }break;
        case GATT_DISCOVER_DESCRIPTOR:
        {
            /* Only descriptors can be filtered */
            if (pSdpCfg->uuid &&
                (!blc_uuid_cmp(pSdpCfg->uuid, UUID16_DEF(DECLARATIONS_UUID_PRIMARY_SERVICE)) ||
                !blc_uuid_cmp(pSdpCfg->uuid, UUID16_DEF(DECLARATIONS_UUID_SECONDARY_SERVICE)) ||
                !blc_uuid_cmp(pSdpCfg->uuid, UUID16_DEF(DECLARATIONS_UUID_INCLUDE)) ||
                !blc_uuid_cmp(pSdpCfg->uuid, UUID16_DEF(DECLARATIONS_UUID_CHARACTERISTIC)))) {
                state = GATT_ERR_INVALID_PARAMETER;
        }
        else
        {
            state = blt_gattc_findInfoReq(connHandle, pSdpCfg);
        }
        }break;
        case GATT_DISCOVER_ATTRIBUTE:
        {
            state = blt_gattc_findInfoReq(connHandle, pSdpCfg);
        }break;
        default:
        {
            my_dump_str_data(DBG_GATTC_LOG, "Invalid discovery type", (u8*)&pSdpCfg->type, 1);
        }break;
    }
    return state;
}









/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  GATT_EXCHANGE_MTU_REQ
//
/////////////////////////////////////////////////////////////////////////////////////////////////
ble_sts_t blt_gattc_mtuSizeExchangeReq(u16 connHandle, u16 mtuSize)
{

    GATTC_CHECK_PENDING;

    if (mtuSize < ATT_MTU_SIZE) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    ble_sts_t status = blc_attc_sendMtuSizeExchangeRequest (connHandle, mtuSize);

    if (status == BLE_SUCCESS) {
        my_dump_str_data(DBG_GATTC_LOG, "[GATTC]---> ATT MTU Size Exchange Request", 0, 0);
        GATTC_SET_PENDING(connHandle, ATT_OP_EXCHANGE_MTU_RSP, NULL, NULL);
    }

    return status;
}









/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  GATT_READ_MULTIPLE_VARIABLE_REQ / GATT_READ_MULTIPLE_VARIABLE_RSP
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void blt_gattc_readMultRsp(u16 connHandle, u8 err, attr_pkt_t *pAttrRspPkt, u16 attrLen, void *userData)
{
    my_dump_str_data(DBG_GATTC_LOG, "[GATTC]<--- Read Multiple Response", 0, 0);

    gattc_read_cfg_t *pRdCfg = userData;
    u8 *pData = pAttrRspPkt->data;
    u16 dataLen = attrLen - 1; /* skip opcode */

    if (err) {
        my_dump_str_data(DBG_GATTC_LOG, "   | err", (u8*)&err, 1);
        goto done;
    }

    gatt_read_data_t rdData = {
        .rdState = GATT_RD_CMPLT,
        .dataVal = pData,
        .dataLen = dataLen,
    };

    pRdCfg->func(connHandle, err, &rdData, pRdCfg);

    /* mark read as complete since read multiple is single response */
    /* return; Not need here */
done:
    pRdCfg->func(connHandle, err, NULL, pRdCfg);
}

static ble_sts_t blt_gattc_readMultReq(u16 connHandle, gattc_read_cfg_t *pRdCfg)
{
    my_dump_str_data(DBG_GATTC_LOG, "[GATTC]---> Read Multiple Request", 0, 0);

    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    ble_sts_t status = blc_attc_sendReadMultReq (connHandle, pRdCfg->hdlCnt, pRdCfg->multiple.handles);

    if (status == BLE_SUCCESS) {
        GATTC_SET_PENDING(connHandle, ATT_OP_READ_MULTIPLE_RSP, blt_gattc_readMultRsp, pRdCfg);
    }

    return status;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  GATT_READ_MULTIPLE_REQ / GATT_READ_MULTIPLE_RSP
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void blt_gattc_readMultVarRsp(u16 connHandle, u8 err, attr_pkt_t *pAttrRspPkt, u16 attrLen, void *userData)
{
    my_dump_str_data(DBG_GATTC_LOG, "[GATTC]<--- Read Multiple Variable Response", 0, 0);

    gattc_read_cfg_t *pRdCfg = userData;
    u16 dataLen = attrLen - 1; /* skip opcode */

    if (err) {
        my_dump_str_data(DBG_GATTC_LOG, "   | err", (u8*)&err, 1);
        goto done;
    }

    blt_attr_readMultiVarRsp_t *rsp = (blt_attr_readMultiVarRsp_t*)pAttrRspPkt;

    u8 i = 0;
    struct attr_value_tuple_list * pLenValTupleList;
    u16 tupleHdrLen = sizeof(pLenValTupleList->length);

    while(dataLen > tupleHdrLen) {
        pLenValTupleList = &rsp->list[i++];
        u16 tupleLen = pLenValTupleList->length + tupleHdrLen;

        /* If a Length Value Tuple is truncated, then the amount of Attribute
         * Value will be less than the value of the Value Length field. */
        if(dataLen < tupleLen) {
            tupleLen = dataLen;
        }

        gatt_read_data_t rdData = {
            .rdState = GATT_RD_CMPLT,
            .dataVal = pLenValTupleList->attrValue,
            .dataLen = tupleLen-tupleHdrLen,
        };

        pRdCfg->func(connHandle, 0, &rdData, pRdCfg);
        dataLen -= tupleLen;
    }

    /* mark read as complete since read multiple is single response */
    /* return; Not need here */

done:
    pRdCfg->func(connHandle, err, NULL, pRdCfg);
}

static ble_sts_t blt_gattc_readMultVarReq(u16 connHandle, gattc_read_cfg_t *pRdCfg)
{
    my_dump_str_data(DBG_GATTC_LOG, "[GATTC]---> Read Multiple Variable Request", 0, 0);

    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    ble_sts_t status = blc_attc_sendReadMultVarReq (connHandle, pRdCfg->hdlCnt, pRdCfg->multiple.handles);

    if (status == BLE_SUCCESS) {
        GATTC_SET_PENDING(connHandle, ATT_OP_READ_MULTIPLE_VARIABLE_RSP, blt_gattc_readMultVarRsp, pRdCfg);
    }

    return status;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  GATT_READ_REQ / GATT_READ_RSP
//                  GATT_READ_BLOB_REQ / GATT_READ_BLOB_RSP
//                  GATT_READ_BY_TYPE_REQ / GATT_READ_BY_TYPE_RSP
//  Notice:
//  GATT_READ_REQ/GATT_READ_BLOB_REQ/GATT_READ_BY_TYPE_REQ are all processed in API: blc_gattc_read directly.
//  GATT_READ_RSP/GATT_READ_BLOB_RSP/GATT_READ_BY_TYPE_RSP are all processed in API: blt_gattc_readRsp directly.
/////////////////////////////////////////////////////////////////////////////////////////////////
static void blt_gattc_readRsp(u16 connHandle, u8 err, attr_pkt_t *pAttrRspPkt, u16 attrLen, void *userData)
{
    gattc_read_cfg_t *pRdCfg = userData;
    u8 *pData = pAttrRspPkt->data;
    u16 dataLen = attrLen - 1; /* skip opcode */
    u8 readState;

    if (err) {
        my_dump_str_data(DBG_GATTC_LOG, "   | err", (u8*)&err, 1);
        goto done;
    }

    if (pRdCfg->hdlCnt == 0) { /* GATT Read Using Characteristic UUID RSP */
        my_dump_str_data(DBG_GATTC_LOG, "[GATTC]<--- Read By UUID Response", 0, 0);

    blt_attr_readByTypeRsp_t *rsp = (blt_attr_readByTypeRsp_t*)pAttrRspPkt;
    u16 length = attrLen - OFFSETOF(blt_attr_readByTypeRsp_t, list); /* skip opcode && length */
    u8 pairLen = rsp->length;
    u8 attrDataListHdr = OFFSETOF(struct attr_data_list, value);

        if (pairLen < attrDataListHdr) {
            my_dump_str_data(DBG_GATTC_LOG, "   | err", (u8*)&err, 1);
            goto done;
        }

        u16 attrHdl = 0;
        struct attr_data_list *pAttrDataList;

        /* Parse values found */
        for (pAttrDataList = rsp->list; length; length -= pairLen, \
             pAttrDataList = (struct attr_data_list *)((u8 *)pAttrDataList + pairLen)) {

            /* Attribute Handle */
            attrHdl = pAttrDataList->handle;
            /* Handle 0 is invalid */
            if (attrHdl == ATT_HANDLE_NONE) {
                goto done;
            }

            /* Update start_handle */
            pRdCfg->byUuid.startHdl = attrHdl;

            /* The ATT_READ_BY_TYPE_RSP PDU returns a list of Attribute Handle and
             * Attribute Value pairs corresponding to the first characteristics contained in the
             * handle range that will fit into the ATT_READ_BY_TYPE_RSP PDU.This procedure does
             * not return the complete list of all characteristics with the given characteristic UUID within the range of values.
             *
             * NOTICE:  here not clear, I'll check it latter tyf 22-11-01
             * If such an operation is required, then the Discover All Characteristics by UUID sub procedure shall be used.
             */


            u16 valLen = pairLen > length ? length - attrDataListHdr : pairLen - attrDataListHdr;
            readState = GATT_RD_CMPLT;

            gatt_read_data_t rdData = {
                .rdState = readState,
                .dataVal = pAttrDataList->value,
                .dataLen = valLen,
            };

            if (pRdCfg->func(connHandle, 0, &rdData, pRdCfg) == GATT_PROC_END) {
                return;
            }

            /* Check if long attribute */
            if (pairLen > length) {
                ble_sts_t status = blc_gattc_readAttributeValue(connHandle, pRdCfg);
                if (status != BLE_SUCCESS) {
                    err = status;
                    goto done;
                }

                return;
            }

            /* Stop if it's the last handle to be read */
            if (pRdCfg->byUuid.startHdl == pRdCfg->byUuid.endHdl) {
                goto done;
            }

            pRdCfg->byUuid.startHdl++;
        }

        return;
    } else { /* GATT Read_Rsp / Read_Blob_Rsp */

        /* debug log used */
        if (pRdCfg->single.offset) {  /* GATT Read Long Characteristic Values RSP */
            my_dump_str_data(DBG_GATTC_LOG, "[GATTC]<--- Read Blob Response", 0, 0);
        } else { /* GATT Read Characteristic Values RSP */
            my_dump_str_data(DBG_GATTC_LOG, "[GATTC]<--- Read Response", 0, 0);
        }

        /* Refer to <<Core5.3>> | Vol 3, Part F page 1434
         * The attribute value shall be set to the value of the attribute identified by the
         * attribute handle in the request. If the attribute value is longer than
         * (ATT_MTU-1) then the first (ATT_MTU-1) octets shall be included in this response.
         * Note: The ATT_READ_BLOB_REQ PDU (see Section 3.4.4.5) can be used to read the
         * remaining octets of a long attribute value.
         */
        gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);

        /* attr_pkt_data length < ATT_MTU -1 */
        if (dataLen < (pGap_ms_para->effective_MTU - 1)) {
            readState = GATT_RD_CMPLT;
        } else {
            readState = GATT_RD_CONT;
        }

        gatt_read_data_t rdData = {
            .rdState = readState,
            .dataVal = pData,
            .dataLen = dataLen,
        };

        if(pRdCfg->single.wBuff){
            s16 leftSpace = pRdCfg->single.maxLen - pRdCfg->single.offset;
            leftSpace < 0 ? (err = GATT_ERR_DATA_LENGTH_EXCEED_MEM_RESTRICTION) : (err = 0);
            smemcpy(pRdCfg->single.wBuff + pRdCfg->single.offset, pData, min(dataLen, leftSpace));
        }

        if(pRdCfg->single.wBuffLen)
            *pRdCfg->single.wBuffLen = pRdCfg->single.offset + dataLen;

        if (pRdCfg->func(connHandle, err, &rdData, pRdCfg) == GATT_PROC_END) {
            return;
        }

        if(readState == GATT_RD_CMPLT){
            return;
        }

        /* If attr_pkt_data '>' OR '==' ATT_MTU -1:
         * If the Characteristic Value has a fixed length that is not longer than (ATT_MTU �C 1),
         * then the server may respond to the first ATT_READ_BLOB_REQ_PDU with an ATT_ERROR_RSP
         * PDU with the Error Code parameter set to Attribute Not Long (0x0B).
         */
        pRdCfg->single.offset += dataLen;

        /* Continue to read the remaining octets of a long attribute value */
        ble_sts_t status = blc_gattc_readAttributeValue(connHandle, pRdCfg);
        if (status != BLE_SUCCESS) {
            err = status;
            goto done;
        }

        return;
    }

done:
    pRdCfg->func(connHandle, err, NULL, pRdCfg);
}





/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  GATT_READ_API FOR USERS
//
/////////////////////////////////////////////////////////////////////////////////////////////////
ble_sts_t blc_gattc_readAttributeValue(u16 connHandle, gattc_read_cfg_t *pRdCfg)
{
    /* Parameters check */
    GATTC_CHECK_PENDING;

    if ((pRdCfg == NULL) || pRdCfg->func == NULL) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    ble_sts_t status = BLE_SUCCESS;

    u8 opcodeMark;

    if (pRdCfg->hdlCnt > 1) {
        if (pRdCfg->multiple.variable) {
            return blt_gattc_readMultVarReq(connHandle, pRdCfg);
        } else {
            return blt_gattc_readMultReq(connHandle, pRdCfg);
        }
    } else if (pRdCfg->hdlCnt == 0) { /* GATT Read Using Characteristic UUID REQ*/
        my_dump_str_data(DBG_GATTC_LOG, "[GATTC]---> Read By UUID Request", 0, 0);
        opcodeMark = ATT_OP_READ_BY_TYPE_RSP;
        status = blc_attc_sendReadByTypeRequest (connHandle, pRdCfg->byUuid.startHdl, pRdCfg->byUuid.endHdl,
                                                    pRdCfg->byUuid.uuid->uuidVal.u, pRdCfg->byUuid.uuid->uuidLen);
    } else if (pRdCfg->single.offset) { /* GATT Read Long Characteristic Values REQ */
        my_dump_str_data(DBG_GATTC_LOG, "[GATTC]---> Read Blob Request", 0, 0);
        opcodeMark = ATT_OP_READ_BLOB_RSP;
        status = blc_attc_sendReadBlobRequest(connHandle, pRdCfg->single.handle, pRdCfg->single.offset);
    } else { /* GATT Read Characteristic Values REQ */
        my_dump_str_data(DBG_GATTC_LOG, "[GATTC]---> Read Request", 0, 0);
        opcodeMark = ATT_OP_READ_RSP;
        status = blc_attc_sendReadRequest(connHandle, pRdCfg->single.handle);
    }

    if (status == BLE_SUCCESS) {
        GATTC_SET_PENDING(connHandle, opcodeMark, blt_gattc_readRsp, pRdCfg);
    }

    return status;
}


















/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  GATT_WRITE_REQ / GATT_WRITE_RSP
//                  GATT_PREPARE_WRITE_REQ / GATT_PREPARE_WRITE_RSP
//                  GATT_EXECUTE_WRITE_REQ / GATT_EXECUTE_WRITE_RSP
//  Notice:
//  GATT_WRITE_REQ/GATT_PREPARE_WRITE_REQ/GATT_EXECUTE_WRITE_REQ are all processed in API: blc_gattc_writeAttributeValue directly.
//  GATT_WRITE_RSP/GATT_PREPARE_WRITE_RSP/GATT_EXECUTE_WRITE_RSP are all processed in API: blt_gattc_writeRsp directly.
/////////////////////////////////////////////////////////////////////////////////////////////////

static void blt_gattc_writeRsp(u16 connHandle, u8 err, attr_pkt_t *pAttrRspPkt, u16 attrLen, void *userData)
{
    /* Debug log used only */
    u8 opcode = pAttrRspPkt->opcode;
    if (opcode == ATT_OP_ERROR_RSP) {
        my_dump_str_data(DBG_GATTC_LOG, "[GATTC]<--- Error Response", 0, 0);
    } else if (opcode == ATT_OP_WRITE_RSP) {
        my_dump_str_data(DBG_GATTC_LOG, "[GATTC]<--- Write Response", 0, 0);
    } else if (opcode == ATT_OP_PREPARE_WRITE_RSP) {
        my_dump_str_data(DBG_GATTC_LOG, "[GATTC]<--- Prepare Write Response", 0, 0);
    } else if (opcode == ATT_OP_EXECUTE_WRITE_RSP) {
        my_dump_str_data(DBG_GATTC_LOG, "[GATTC]<--- Execute Write Response", 0, 0);
    } else {
        my_dump_str_data(DBG_GATTC_LOG, "[GATTC]<--- Unlikely Error", &opcode, 1);
        err = HCI_ERR_UNSPECIFIED_ERROR;
    }

    gattc_write_cfg_t *pWrCfg = userData;

    if (err) {
        my_dump_str_data(DBG_GATTC_LOG, "   | err", (u8*)&err, 1);
    }

    (void)pAttrRspPkt;
    (void)attrLen;

    pWrCfg->func(connHandle, err, pWrCfg);
}

static void blt_gattc_prepareWriteRsp(u16 connHandle, u8 err, attr_pkt_t *pAttrRspPkt, u16 attrLen, void *userData)
{
    my_dump_str_data(DBG_GATTC_LOG, "[GATTC]<--- Prepare Write Response", 0, 0);
    gattc_write_cfg_t *pWrCfg = userData;

    if (err) {
        my_dump_str_data(DBG_GATTC_LOG, "   | err", (u8*)&err, 1);
        goto done;
    }

    ble_sts_t status = BLE_SUCCESS;
    blt_attr_prepareWriteRsp_t *rsp = (blt_attr_prepareWriteRsp_t*)pAttrRspPkt;
    u16 valLen = attrLen - sizeof(blt_attr_prepareWriteRsp_t);

    if ((valLen > pWrCfg->length) || (rsp->valueOffset != pWrCfg->offset ) || (rsp->handle != pWrCfg->handle) || \
        memcmp(rsp->partAttrValue, pWrCfg->data, valLen) != 0) {
        my_dump_str_data(DBG_GATTC_LOG, "[GATTC]---> Cancel Write Request", 0, 0);
        status = blc_attc_sendExecuteWriteRequest(connHandle, pWrCfg->handle, ATT_EXEC_WRITE_CANCEL);
        if (status == BLE_SUCCESS) {
            GATTC_SET_PENDING(connHandle, ATT_OP_EXECUTE_WRITE_RSP, blt_gattc_writeRsp, pWrCfg);
            return;
        }

        err = status;
        goto done;
    }

    /* Update write configure parameters */
    pWrCfg->offset += valLen;
    pWrCfg->data = (u8 *)pWrCfg->data + valLen;
    pWrCfg->length -= valLen;

    /* If there is no more data to execute */
    if (!pWrCfg->length) {
        my_dump_str_data(DBG_GATTC_LOG, "[GATTC]---> Execute Write Request", 0, 0);
        status = blc_attc_sendExecuteWriteRequest(connHandle, pWrCfg->handle, ATT_EXEC_WRITE_ALL);
        if (status == BLE_SUCCESS) {
            GATTC_SET_PENDING(connHandle, ATT_OP_EXECUTE_WRITE_RSP, blt_gattc_writeRsp, pWrCfg);
            return;
        }

        err = status;
        goto done;
    }

    /* Continue to write the remaining data */
    err = blc_gattc_writeAttributeValue(connHandle, pWrCfg);
    if (err == BLE_SUCCESS) {
        return;
    }

done:
    pWrCfg->func(connHandle, err, pWrCfg);
}

static ble_sts_t blt_gattc_prepareWrite(u16 connHandle, gattc_write_cfg_t *pWrCfg)
{
    assert(pWrCfg != NULL);
    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    ble_sts_t status = BLE_SUCCESS;

    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);

    u16 attrValLenLimit = pGap_ms_para->effective_MTU - sizeof(blt_attr_prepareWriteRsp_t);

    u16 attrValLen = min(pWrCfg->length, attrValLenLimit);

    status = blc_attc_sendPrepareWriteRequest (connHandle, pWrCfg->handle, pWrCfg->offset, pWrCfg->data, attrValLen);
    my_dump_str_data(DBG_GATTC_LOG, "[GATTC]---> Prepare Write Request", 0, 0);

    if (status == BLE_SUCCESS) {
        GATTC_SET_PENDING(connHandle, ATT_OP_PREPARE_WRITE_RSP, blt_gattc_prepareWriteRsp, pWrCfg);
    }

    return status;
}




/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  GATT_WRITE_API FOR USERS
//
/////////////////////////////////////////////////////////////////////////////////////////////////
ble_sts_t blc_gattc_writeAttributeValue(u16 connHandle, gattc_write_cfg_t *pWrCfg)
{
    /* Parameters check */
    GATTC_CHECK_PENDING;

    if ((pWrCfg == NULL) || !pWrCfg->handle || \
        ((pWrCfg->withoutRsp == false) && pWrCfg->func == NULL)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    ble_sts_t status = BLE_SUCCESS;

    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);

    u16 attrValLen = pWrCfg->length;
    u16 attValLimitLen = (pGap_ms_para->effective_MTU - 3);

    /* If the length exceeds MTU-1, it will be truncated */
    /* Write command, not need 'pWrCfg->func'  */
    if(pWrCfg->withoutRsp == true) {
        attrValLen = attrValLen > attValLimitLen ? attValLimitLen : attrValLen;
        return blc_attc_sendWriteCommand (connHandle, pWrCfg->handle, pWrCfg->data, attrValLen);
    }

    /* attr_pkt_data length < ATT_MTU -1 */
    if (pWrCfg->offset || attrValLen > attValLimitLen) {
        return blt_gattc_prepareWrite(connHandle, pWrCfg);
    }

    my_dump_str_data(DBG_GATTC_LOG, "[GATTC]---> Write Request", 0, 0);
    status = blc_attc_sendWriteRequest(connHandle, pWrCfg->handle, pWrCfg->data, pWrCfg->length);

    if (status == BLE_SUCCESS) {
        GATTC_SET_PENDING(connHandle, ATT_OP_WRITE_RSP, blt_gattc_writeRsp, pWrCfg);
    }

    return status;
}











/////////////////////////////////////////////////////////////////////////////////////////////////
//
//                  GATT_SUBSCRIBE_API FOR USERS
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void blt_gattc_writeSubCccCb(u16 connHandle, u8 err, attr_pkt_t *pAttrRspPkt, u16 attrLen, void *userData)
{
    (void)attrLen;(void)pAttrRspPkt;
    gattc_sub_ccc_cfg_t *pSubCccCfg = userData;
    my_dump_str_data(DBG_GATTC_LOG, "[GATTC]<--- Write CCC Response", 0, 0);

    if (pSubCccCfg->func) {
        pSubCccCfg->func(connHandle, err, pSubCccCfg);
    }
}

ble_sts_t blc_gattc_writeSubscribeCCCRequest(u16 connHandle, gattc_sub_ccc_cfg_t *pSubCccCfg)
{
    GATTC_CHECK_PENDING;

    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);
    assert(pSubCccCfg != NULL);

    my_dump_str_data(DBG_GATTC_LOG, "[GATTC]---> Write CCC Request [cccHdl]", &pSubCccCfg->valueHdl, 2);

    ble_sts_t status = blc_attc_sendWriteRequest(connHandle, pSubCccCfg->valueHdl, (u8*)&pSubCccCfg->value, 2);

    if (status == BLE_SUCCESS) {
        GATTC_SET_PENDING(connHandle, ATT_OP_WRITE_RSP, blt_gattc_writeSubCccCb, pSubCccCfg);
    }

    return status;
}

bool blc_gattc_addSubscribeCCCNode(u16 connHandle, gattc_sub_ccc_msg_t *pSubNode)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS)
        return false;

    struct single_list* pSubL = &gattc_subCccEntry[connHandle&0x0f];

    SLIST_INSERT_NODE_HEAD(pSubL, pSubNode);

    return true;
}

void blt_gattc_notification(u16 connHandle, attr_pkt_t *attr, u16 attrLen)
{
    struct single_list* pSubL = &gattc_subCccEntry[connHandle&0x0f];
    blt_attr_handleValueNtf_t *pHdlValNtf = (blt_attr_handleValueNtf_t*)attr;
    struct single_list_node *cur = NULL;

    SLIST_FOREACH(cur, pSubL, next) {
        gattc_sub_ccc_msg_t *pTmpNode = (gattc_sub_ccc_msg_t*)cur;
        if(pHdlValNtf->handle >= pTmpNode->startHdl && pHdlValNtf->handle <= pTmpNode->endHdl)
        {
            if(pTmpNode->ntfOrIndFunc) {
                pTmpNode->ntfOrIndFunc(connHandle, pHdlValNtf->handle, pHdlValNtf->value, attrLen-3);
                break;
            }
        }
    }
}

void blt_gattc_multiNotification(u16 connHandle, attr_pkt_t *attr, u16 attrLen)
{
    struct single_list* pSubL = &gattc_subCccEntry[connHandle&0x0f];
    struct single_list_node *cur = NULL;
    blt_attr_multiHandleValueNtf_t *pMultiHdlValNtf = (blt_attr_multiHandleValueNtf_t*)attr;

    struct attr_ntf_value_tuple_list *ntfValTuple = &pMultiHdlValNtf->list[0];
    u16 attrDataLen = attrLen - 1; /* skip opcode[1B] */
    u16 ntfValTupleLen;

    while (attrDataLen > sizeof(struct attr_ntf_value_tuple_list)) {
        /* valid length check */
        ntfValTupleLen = sizeof(struct attr_ntf_value_tuple_list) + ntfValTuple->length;
        if (ntfValTupleLen > attrDataLen) {
            return;
        }
        /* process each notify value tuple */
        SLIST_FOREACH(cur, pSubL, next) {
            gattc_sub_ccc_msg_t *pTmpNode = (gattc_sub_ccc_msg_t*)cur;
            if(ntfValTuple->handle >= pTmpNode->startHdl && ntfValTuple->handle <= pTmpNode->endHdl)
            {
                if(pTmpNode->ntfOrIndFunc) {
                    pTmpNode->ntfOrIndFunc(connHandle, ntfValTuple->handle, ntfValTuple->value, ntfValTuple->length);
                    break;
                }
            }
        }

        /* get the next notify value tuple */
        attrDataLen -= ntfValTupleLen;
        ntfValTuple = (struct attr_ntf_value_tuple_list *)((u8*)ntfValTuple + ntfValTupleLen);
    }
}


void blc_gattc_cleanAllSubscribeCCCNode(u16 connHandle)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS)
        return ;

    struct single_list* pSubL = &gattc_subCccEntry[connHandle&0x0f];

    SLIST_FIRST(pSubL) = NULL;

}

void blc_gattc_removeSubscribeCCCNode(u16 connHandle, gattc_sub_ccc_msg_t *pSubNode)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS)
        return ;

    struct single_list* pSubL = &gattc_subCccEntry[connHandle&0x0f];

    SLIST_DELETE_NODE(pSubL, pSubNode);
}







