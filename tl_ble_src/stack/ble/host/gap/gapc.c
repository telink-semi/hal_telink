/********************************************************************************************************
 * @file    gapc.c
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
#include "stack/ble/host/gatt/tlk_malloc_stack.h"

#define GAPC_MALLOC(size)                   malloc_nonreten(size)
#define GAPC_FREE(ptr)                      free_nonreten(ptr)

#define GAPC_MAX_SERVICE_NUM                4
#define GAPC_MAX_INCLUDE_NUM                8

const char *uuidFormat(const uuid_t *uuid)
{
#if DBG_GAPC_LOG
    u8 u128[ATT_128_UUID_LEN];
    for(int i=0; i<uuid->uuidLen; i++)
    {
        u128[i] = uuid->uuidVal.u[uuid->uuidLen-i-1];
    }
    return hex_to_str(u128, uuid->uuidLen);
#else
    (void)uuid;
    return '\0';
#endif
}

typedef enum{
    DISCOVERY_STATE_IDLE    = 0,
    DISCOVERY_STATE_FIND_SERVICE,
    DISCOVERY_STATE_FIND_INCLUDE,
    DISCOVERY_STATE_FIND_INCLUDE_END,
    DISCOVERY_STATE_FIND_SERVICE_CHAR,
    DISCOVERY_STATE_FIND_INCLUDE_CHAR,
    DISCOVERY_STATE_END,

    RECONNECT_STATE_GET_RECONN_SERVICE,
    RECONNECT_STATE_GET_RECONN_SERVICE_END,
    RECONNECT_STATE_GET_RECONN_INCL,
    RECONNECT_STATE_GET_RECONN_INCL_END
} blt_gapc_discovery_state_enum;

typedef struct{
    u16 startHandle;
    u16 endHandle;
    uuid_t *uuid;
} blt_gapc_discAttrInfo_t;

typedef struct{
    u8 properties;
    union {
        u8 setting;
        struct {
            bool subscribeNtf   :1;
            bool subscribeInd   :1;
            bool findDecs       :1;
            bool readValue      :1;
        };
    };
    u16 handle;
    uuid_t *uuid;
} blt_gapc_disc_last_char_info_t;

typedef struct{
    u8 state;
    union {
        u8 infoSize;
        struct {
            u8 includeSize      :5;
            u8 serviceSize      :3;
        };
    };
    union {
        u8 infoIndex;
        struct {
            u8 includeIndex     :5;
            u8 serviceIndex     :3;
        };
    };
    u16 connHandle;
    blt_gapc_disc_last_char_info_t lastChar;
    blt_gapc_discAttrInfo_t info[GAPC_MAX_SERVICE_NUM + GAPC_MAX_INCLUDE_NUM];
    u16 startHandle;

    struct single_priority_list gattcMsg;
    union {
        const blc_gapc_discList_t *list;
        const blc_gapc_reconnList_t *reconnList;
    };
} blt_gapc_discovery_t;

typedef enum{
    GAPC_MSG_NONE,
    GAPC_MSG_SDP_CFG,
    GAPC_MSG_SUB_CCC,
    GAPC_MSG_READ_CHAR,
} blt_gapc_msg_type_enum;

typedef enum{
    GAPC_MSG_NULL,
    GAPC_MSG_STORED,
    GAPC_MSG_ANALYSIS,
} blt_gapc_msg_state_enum;

typedef struct blt_gapc_msg{
    struct blt_gapc_msg *pNext;
    u8 state;
    u8 type;
    u16 connHandle;
    union{
        gattc_sdp_cfg_t sdpCfg;
        gattc_sub_ccc_cfg_t cccCfg;

        struct{
            gapc_read_func_t rdCb;   /* profile users callback used */
            gattc_read_cfg_t readCfg; /* gapc used */
        };

    };
} blt_gapc_msg_t;


_attribute_ble_data_retention_
blt_gapc_discovery_t gGapcDisc[GAPC_DISCOVERY_MAX_NUM];


static blt_gapc_discovery_t* blt_gapc_createDisc(u16 connHandle)
{
    for(int i=0; i<GAPC_DISCOVERY_MAX_NUM; i++)
    {
        if(gGapcDisc[i].connHandle == connHandle)
        {
            if(gGapcDisc[i].state == DISCOVERY_STATE_IDLE)
            {
                return &gGapcDisc[i];
            }
            return NULL;
        }
    }
    for(int i=0; i<GAPC_DISCOVERY_MAX_NUM; i++)
    {
        if(gGapcDisc[i].state == DISCOVERY_STATE_IDLE)
        {
            return &gGapcDisc[i];
        }
    }
    return NULL;
}

static blt_gapc_discovery_t* blt_gapc_getDisc(u16 connHandle)
{
    for(int i=0; i<GAPC_DISCOVERY_MAX_NUM; i++)
    {
        if(gGapcDisc[i].connHandle == connHandle)
        {
            return &gGapcDisc[i];
        }
    }
    return NULL;
}

static blt_gapc_msg_t* blt_gapc_mallocNewGapMsg(void)
{
    blt_gapc_msg_t* msg = GAPC_MALLOC(sizeof(blt_gapc_msg_t));

    if(msg) msg->state = GAPC_MSG_STORED;
    return msg;
}

static blt_gapc_msg_t* blt_gapc_popGapMsgHeader(u16 connHandle)
{
    blt_gapc_discovery_t* gapDisc = blt_gapc_getDisc(connHandle);

    blt_gapc_msg_t* msg = SPLIST_DELETE_HEAD(&gapDisc->gattcMsg.list, blt_gapc_msg_t);

    return msg;
}

static void blt_gapc_freeGapMsgHeader(u16 connHandle)
{
    blt_gapc_msg_t* msg = blt_gapc_popGapMsgHeader(connHandle);
    GAPC_FREE(msg);
}


static ble_sts_t blt_gapc_dealGapcMsg(blt_gapc_msg_t * msg)
{
    if(!msg->state)
        return GAP_ERR_INVALID_PARAMETER;
    ble_sts_t status = BLE_SUCCESS;
    if(msg->type == GAPC_MSG_SDP_CFG)
    {
        status = blc_gattc_discovery(msg->connHandle, &msg->sdpCfg);
    }
    else if(msg->type == GAPC_MSG_SUB_CCC)
    {
        status = blc_gattc_writeSubscribeCCCRequest(msg->connHandle, &msg->cccCfg);
    }
    else if(msg->type == GAPC_MSG_READ_CHAR)
    {
        status = blc_gattc_readAttributeValue(msg->connHandle, &msg->readCfg);
    }

    if(status == BLE_SUCCESS)
        msg->state = GAPC_MSG_ANALYSIS;
    return status;

}

static int blt_gapc_getMsgPriority(void* node)
{
    blt_gapc_msg_t *elem = (blt_gapc_msg_t*)node;

    if(elem->state == GAPC_MSG_ANALYSIS)        return GAPC_MSG_NONE;

    return elem->type;
}

static ble_sts_t blt_gapc_addGapcMsg(struct single_priority_list *list, blt_gapc_msg_t *msg)
{
    SPLIST_INSERT_NODE(list, msg);
    return BLE_SUCCESS;
}



static ble_sts_t blt_gapc_createDiscDescriptors(blt_gapc_discovery_t* disc, u16 endHdl);
static ble_sts_t blt_gapc_createReadAttrValue(u16 connHandle, u16 handle, u8* wBuff, u16* wBuffLen, u16 maxLen, gapc_read_func_t rdCbFunc);
static ble_sts_t blt_gapc_createSubscribeCCC(u16 connHandle, uuid_t* uuid, u16 handle, u16 value);

static blc_gapc_discChar_t* blt_gapc_checkCharUuidTable(const blc_gapc_discCharTable_t* charTb, uuid_t* uuid)
{
    if(!uuid)
        return NULL;

    u8 charSize = charTb->size;
    blc_gapc_discChar_t* characteristic = (blc_gapc_discChar_t*)(size_t)charTb->characteristic;

    for(int i=0; i<charSize; i++)
    {
        if(!blc_uuid_cmp(&characteristic->uuid, uuid))
        {
            return characteristic;
        }
        characteristic++;
    }
    return NULL;
}

static blc_gapc_discChar_t* blt_gapc_checkCharUuid(u16 connHandle, uuid_t* uuid)
{
    if(!uuid)
        return NULL;

    blt_gapc_discovery_t* gapDisc = blt_gapc_getDisc(connHandle);
    return blt_gapc_checkCharUuidTable(&gapDisc->list->characteristicTable, uuid);
}

static blc_gapc_discChar_t* blt_gapc_checkAllCharUuid(u16 connHandle, uuid_t* uuid)
{
    if(!uuid)
        return NULL;

    blc_gapc_discChar_t* characteristic = blt_gapc_checkCharUuid(connHandle, uuid);
    if(characteristic)
        return characteristic;
    blt_gapc_discovery_t* gapDisc = blt_gapc_getDisc(connHandle);

    u8 charSize = gapDisc->list->includeTable.size;

    for(int i=0; i<charSize; i++)
    {
        const blc_gapc_discInclude_t* discIncl = gapDisc->list->includeTable.include[i];
        characteristic = blt_gapc_checkCharUuidTable(&discIncl->characteristic, uuid);
        if(characteristic)
        {
            return characteristic;
        }
    }
    return NULL;
}

static const blc_gapc_discInclude_t* blt_gapc_checkIncludeUuid(u16 connHandle, uuid_t* uuid)
{
    blt_gapc_discovery_t* gapDisc = blt_gapc_getDisc(connHandle);

    u8 charSize = gapDisc->list->includeTable.size;

    for(int i=0; i<charSize; i++)
    {
        const blc_gapc_discInclude_t* discIncl = gapDisc->list->includeTable.include[i];
        if(!blc_uuid_cmp(&discIncl->uuid, uuid))
        {
            return discIncl;
        }
    }

    return NULL;
}




static ble_sts_t blt_gapc_createSdpMsg(blt_gapc_discovery_t* disc, uuid_t* uuid, u16 startHdl, u16 endHdl, u8 type, u8 properties, gattc_sdp_func_t func)
{
    blt_gapc_msg_t* msg = blt_gapc_mallocNewGapMsg();
    if(!msg)
        return GAP_ERR_INVALID_PARAMETER;

    msg->type = GAPC_MSG_SDP_CFG;
    msg->connHandle = disc->connHandle;
    gattc_sdp_cfg_t *pSdpCfg = &msg->sdpCfg;
    pSdpCfg->uuid = uuid;
    pSdpCfg->type = type;
    pSdpCfg->properties = properties;
    pSdpCfg->startHdl = startHdl;
    pSdpCfg->endHdl = endHdl;
    pSdpCfg->func = func;
    blt_gapc_addGapcMsg(&disc->gattcMsg, msg);

    return BLE_SUCCESS;
}

//////////////////////////////GAPC sdp discovery service uuid start///////////////////////////
static u8 blt_gapc_discServiceCb(u16 connHandle, gatt_attr_t *attr, gattc_sdp_cfg_t *params)
{
    (void)params;
    blt_gapc_discovery_t* gapDisc = blt_gapc_getDisc(connHandle);

    if(attr == NULL)
    {
        BLT_GAPC_DEBUG("    found service finish");
        gapDisc->lastChar.handle = ATT_HANDLE_NONE;
        blt_gapc_freeGapMsgHeader(connHandle);
        if(!gapDisc->serviceSize)
        {
            BLT_GAPC_DEBUG("ERR: not found any service");
            gapc_foundService_func_t sfun = gapDisc->list->service->sfun;
            memset((u8*)gapDisc, 0, sizeof(blt_gapc_discovery_t));
            if(sfun)
            {
                sfun(connHandle, 0xFF, ATT_HANDLE_NONE, ATT_HANDLE_NONE);
            }
        }
        return GATT_PROC_END;
    }

    gatt_service_val_t *prim_service = (gatt_service_val_t *)attr->user_data;

    if(gapDisc->list->service->sfun)
    {
        gapDisc->list->service->sfun(connHandle, gapDisc->serviceSize + 1, attr->handle, prim_service->endHdl);
    }

    gapDisc->info[gapDisc->serviceSize].startHandle = attr->handle;
    gapDisc->info[gapDisc->serviceSize].endHandle = prim_service->endHdl;
    gapDisc->info[gapDisc->serviceSize].uuid = (uuid_t*)(size_t)prim_service->uuid;

    gapDisc->serviceSize ++;

    BLT_GAPC_DEBUG("found service info, connHandle:%x service size:%x startHandle:%x endHandle:%x ", connHandle, gapDisc->serviceSize, attr->handle, prim_service->endHdl);

    if(gapDisc->serviceSize >= GAPC_DISC_MAX_ATTR_INFO || gapDisc->serviceSize >= gapDisc->list->maxServiceCount)
    {
        gapDisc->lastChar.handle = ATT_HANDLE_NONE;
#if DBG_GAPC_LOG
        u8 serviceSize = gapDisc->serviceSize;
        if(gapDisc->serviceSize >= GAPC_DISC_MAX_ATTR_INFO){
            BLT_GAPC_DEBUG("ERR:found service finish, service size[%d] greater than 2", serviceSize);
        }
        else if(gapDisc->list->maxServiceCount > 1){
            BLT_GAPC_DEBUG("ERR:found service finish, service size[%d] greater than maxServiceCount[%d]", serviceSize, GAPC_DISC_MAX_ATTR_INFO);
        }
        else {
            BLT_GAPC_DEBUG("found service finish.");
        }
#endif
        blt_gapc_freeGapMsgHeader(connHandle);
        return GATT_PROC_END;
    }
    return GATT_PROC_CONT;
}

static ble_sts_t blt_gapc_creatDiscServiceMsg(blt_gapc_discovery_t* disc)
{
    BLT_GAPC_DEBUG("create discovery service message, UUID is 0x%s", uuidFormat(&disc->list->service->uuid));
    return blt_gapc_createSdpMsg(disc, (uuid_t*)(size_t)&disc->list->service->uuid, ATT_HANDLE_START, ATT_HANDLE_MAX,
            GATT_DISCOVER_PRIMARY, 0, blt_gapc_discServiceCb);
}

//////////////////////////////GAPC sdp discovery service uuid ending///////////////////////////


//////////////////////////////GAPC sdp discovery include uuid start///////////////////////////
static void blt_gapc_sdpDiscFindIncludeEnd(blt_gapc_discovery_t* disc);
static u8 blt_gapc_discIncludeCb(u16 connHandle, gatt_attr_t *attr, gattc_sdp_cfg_t *params)
{
    (void)params;
    blt_gapc_discovery_t* gapDisc = blt_gapc_getDisc(connHandle);

    if(attr == NULL)
    {
        if(!gapDisc->includeSize)
        {
            BLT_GAPC_DEBUG("    found include finish, NOT FOUND");
            gapDisc->state = DISCOVERY_STATE_FIND_INCLUDE_END;
            blt_gapc_sdpDiscFindIncludeEnd(gapDisc);
        }
        blt_gapc_freeGapMsgHeader(connHandle);
        return GATT_PROC_END;
    }

    gatt_include_t* incl = (gatt_include_t*)attr->user_data;

    const blc_gapc_discInclude_t* discIncl = blt_gapc_checkIncludeUuid(connHandle, &incl->uuid);

    if(!discIncl)
    {
        BLT_GAPC_DEBUG("    found unknown include service UUID: 0x%s, startHandle:%x endHandle:%x",
                uuidFormat(&incl->uuid),incl->startHdl, incl->endHdl);
        if(gapDisc->list->includeTable.uifun)
        {
            gapDisc->list->includeTable.uifun(connHandle, &incl->uuid, incl->startHdl, incl->endHdl);
        }
        return GATT_PROC_CONT;
    }

    gapDisc->info[gapDisc->serviceSize + gapDisc->includeSize].startHandle = incl->startHdl;
    gapDisc->info[gapDisc->serviceSize + gapDisc->includeSize].endHandle = incl->endHdl;
    gapDisc->info[gapDisc->serviceSize + gapDisc->includeSize].uuid = (uuid_t*)(size_t)&discIncl->uuid;

    BLT_GAPC_DEBUG("    found include service, count:%d, UUID: 0x%s, startHandle:%x endHandle:%x", gapDisc->includeSize,
            uuidFormat(&incl->uuid), incl->startHdl, incl->endHdl
    );

    gapDisc->includeSize ++;

    if(gapDisc->includeSize >= GAPC_DISC_MAX_INCLUDE_INFO)
    {
        BLT_GAPC_DEBUG("    found include size[%d], greater than %d", gapDisc->includeSize, GAPC_DISC_MAX_INCLUDE_INFO);
        gapDisc->lastChar.handle = ATT_HANDLE_NONE;
        blt_gapc_freeGapMsgHeader(connHandle);
        return GATT_PROC_END;
    }

    return GATT_PROC_CONT;
}

static ble_sts_t blt_gapc_createDiscIncludeMsg(blt_gapc_discovery_t* disc, u16 startHdl, u16 endHdl)
{
    BLT_GAPC_DEBUG("create discovery include message, startHandle:%x endHandle:%x", startHdl, endHdl);
    return blt_gapc_createSdpMsg(disc, NULL, startHdl, endHdl,
            GATT_DISCOVER_INCLUDE, 0, blt_gapc_discIncludeCb);
}
//////////////////////////////GAPC sdp discovery include uuid ending///////////////////////////


//////////////////////////////GAPC sdp discovery characteristic uuid start///////////////////////////
static u8 blt_gapc_discCharCb(u16 connHandle, gatt_attr_t *attr, gattc_sdp_cfg_t *params)
{
    blt_gapc_discovery_t* gapDisc = blt_gapc_getDisc(connHandle);

    if(attr == NULL)
    {
        blt_gapc_freeGapMsgHeader(connHandle);
        if((gapDisc->lastChar.handle != ATT_HANDLE_NONE && gapDisc->lastChar.handle <= params->endHdl)
                && (gapDisc->lastChar.subscribeNtf || gapDisc->lastChar.subscribeInd || gapDisc->lastChar.findDecs))
            blt_gapc_createDiscDescriptors(gapDisc, params->endHdl);
        gapDisc->lastChar.handle = ATT_HANDLE_NONE;
        return GATT_PROC_END;
    }

    gatt_chrc_t *chrc = (gatt_chrc_t*)attr->user_data;

    BLT_GAPC_DEBUG("found characteristic UUID:0x%s, Properties:%x ValueHandle:%x", uuidFormat(&chrc->uuid),
            chrc->properties, chrc->valueHdl);

    blc_gapc_discChar_t* characteristic = blt_gapc_checkCharUuid(connHandle, &chrc->uuid);

    if(!characteristic)
    {
        BLT_GAPC_DEBUG("ERR: found unknown characteristic");
        if(gapDisc->list->characteristicTable.ufun)
        {
            gapDisc->list->characteristicTable.ufun(connHandle, &chrc->uuid, chrc->properties, chrc->valueHdl);
        }
    }
    else
    {
        if(characteristic->cfun)
        {
            characteristic->cfun(connHandle, 0, chrc->properties, chrc->valueHdl);
        }

        if(characteristic->readValue  && (chrc->properties & CHAR_PROP_READ) && characteristic->rfun)
        {
            BLT_GAPC_DEBUG("    characteristic[%x] will start read", chrc->valueHdl);
            u8* wBuff = NULL;
            u16* wBuffLen = NULL;
            u16 maxLen = 0;
            gapc_read_func_t rdCbFunc = NULL;
            characteristic->rfun(connHandle, chrc->valueHdl, &wBuff, &wBuffLen, &maxLen, &rdCbFunc);
            blt_gapc_createReadAttrValue(connHandle, chrc->valueHdl, wBuff, wBuffLen, maxLen, rdCbFunc);
        }
    }

    if(gapDisc->lastChar.handle != ATT_HANDLE_NONE && chrc->attrHdl != gapDisc->lastChar.handle )
    {
        if(gapDisc->lastChar.subscribeNtf || gapDisc->lastChar.subscribeInd || gapDisc->lastChar.findDecs) {
            blt_gapc_createDiscDescriptors(gapDisc, chrc->attrHdl - 1);
        }
    }

    gapDisc->lastChar.handle = chrc->valueHdl + 1;
    gapDisc->lastChar.uuid = characteristic? &characteristic->uuid: NULL;
    gapDisc->lastChar.setting = characteristic? characteristic->setting: 0x00;
    gapDisc->lastChar.properties = chrc->properties;

    return GATT_PROC_CONT;
}

static ble_sts_t blt_gapc_createDiscChar(blt_gapc_discovery_t* disc, u16 startHdl, u16 endHdl)
{
    BLT_GAPC_DEBUG("create discovery service characteristic message, startHandle:%x endHandle:%x", startHdl, endHdl);
    return blt_gapc_createSdpMsg(disc, NULL, startHdl, endHdl,
                GATT_DISCOVER_CHARACTERISTIC, 0, blt_gapc_discCharCb);
}
//////////////////////////////GAPC sdp discovery characteristic uuid ending///////////////////////////


//////////////////////////////GAPC read characteristic attribute value start///////////////////////////
static u8 blt_gapc_readAttrValueCb(u16 connHandle, u8 err, gatt_read_data_t *rdData, struct gattc_read_cfg *params)
{
    blt_gapc_msg_t *pMsg = NULL;

    if(err) {
        BLT_GAPC_DEBUG("ERR: read attribute value err. errorCode:%x handle:%x", err, params->single.handle);
        pMsg = blt_gapc_popGapMsgHeader(connHandle);
        if(pMsg && pMsg->rdCb){
            pMsg->rdCb(connHandle, err, params);
        }
        GAPC_FREE(pMsg);
        return GATT_PROC_END;
    }

    if(rdData->rdState == GATT_RD_CONT)
        return GATT_PROC_CONT;

#if DBG_GAPC_LOG
    if(params->single.wBuffLen){
        BLT_GAPC_DEBUG("read attribute value is %s", hex_to_str(params->single.wBuff, *params->single.wBuffLen));
    }
    else if(params->single.maxLen){
        BLT_GAPC_DEBUG("read attribute value is %s", hex_to_str(params->single.wBuff, params->single.maxLen));
    }
#endif
    pMsg = blt_gapc_popGapMsgHeader(connHandle);
    if(pMsg && pMsg->rdCb){
        pMsg->rdCb(connHandle, err, params);
    }
    GAPC_FREE(pMsg);
    return GATT_PROC_END;
}

static ble_sts_t blt_gapc_createReadAttrValue(u16 connHandle, u16 handle, u8* wBuff, u16* wBuffLen, u16 maxLen, gapc_read_func_t rdCbFunc)
{
    blt_gapc_msg_t* msg = blt_gapc_mallocNewGapMsg();
    if(!msg)
        return GAP_ERR_INVALID_PARAMETER;

    BLT_GAPC_DEBUG("create read attribute value message, handle:%x wBuff:%08x wBuffLen:%08x maxLen:%x", handle, wBuff, wBuffLen, maxLen);

    blt_gapc_discovery_t* gapDisc = blt_gapc_getDisc(connHandle);

    msg->type = GAPC_MSG_READ_CHAR;
    msg->connHandle = connHandle;
    msg->rdCb = rdCbFunc;
    gattc_read_cfg_t *pReadCfg = &msg->readCfg;
    pReadCfg->func = blt_gapc_readAttrValueCb;
    pReadCfg->hdlCnt = 1;
    pReadCfg->single.handle = handle;
    pReadCfg->single.offset = 0;
    pReadCfg->single.wBuff = wBuff;
    pReadCfg->single.wBuffLen = wBuffLen;
    pReadCfg->single.maxLen = maxLen;
    blt_gapc_addGapcMsg(&gapDisc->gattcMsg, msg);
    return BLE_SUCCESS;
}
//////////////////////////////GAPC read characteristic attribute value ending///////////////////////////


//////////////////////////////GAPC discovery characteristic descriptor start///////////////////////////
static u8 blt_gapc_discDescriptorsCb(u16 connHandle, gatt_attr_t *attr, gattc_sdp_cfg_t *params)
{

    blc_gapc_discChar_t* characteristic = blt_gapc_checkAllCharUuid(connHandle, params->uuid);

    if(!characteristic)
    {
        BLT_GAPC_DEBUG("    ERR: discovery descriptor not found characteristic UUID is 0x%s", uuidFormat(params->uuid));
        blt_gapc_freeGapMsgHeader(connHandle);
        return GATT_PROC_END;
    }

    if(attr == NULL)
    {
        BLT_GAPC_DEBUG("    discovery descriptor[0x%s] not found/ending", uuidFormat(params->uuid));
        blt_gapc_freeGapMsgHeader(connHandle);
        if(characteristic->dfun)
        {
            characteristic->dfun(connHandle, NULL, 0);
        }
        return GATT_PROC_END;
    }

    if(characteristic->dfun)
    {
        characteristic->dfun(connHandle, (uuid_t*)(size_t)attr->uuid, attr->handle);
    }

    uuid_t uuid16_ccc = UUID16_INIT(DESCRIPTOR_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION);

    if(!blc_uuid_cmp(&uuid16_ccc, attr->uuid)) {
        u16 value = 0;
        if(characteristic->subscribeNtf && (params->properties & CHAR_PROP_NOTIFY)) {
            value |= BIT(0);
        }
        if(characteristic->subscribeInd && (params->properties & CHAR_PROP_INDICATE)){
            value |= BIT(1);
        }
        if(value) {
            blt_gapc_createSubscribeCCC(connHandle, params->uuid, attr->handle, value);
        }
    }
    return GATT_PROC_CONT;
}

static ble_sts_t blt_gapc_createDiscDescriptors(blt_gapc_discovery_t* disc, u16 endHdl)
{
    BLT_GAPC_DEBUG("discovery descriptor UUID is 0x%s, startHdl:%x endHdl:%x properties:%x", uuidFormat(disc->lastChar.uuid),
            disc->lastChar.handle, endHdl, disc->lastChar.properties
    );

    return blt_gapc_createSdpMsg(disc, disc->lastChar.uuid, disc->lastChar.handle, endHdl,
            GATT_DISCOVER_DESCRIPTOR, disc->lastChar.properties, blt_gapc_discDescriptorsCb);
}
//////////////////////////////GAPC discovery characteristic descriptor ending///////////////////////////


//////////////////////////////GAPC subscribe client characteristic configuration start///////////////////////////
static void blt_gapc_subscribeCccCb(u16 connHandle, u8 err, struct gattc_sub_ccc_cfg *params)
{
    blc_gapc_discChar_t* characteristic = blt_gapc_checkAllCharUuid(connHandle, params->uuid);

    BLT_GAPC_DEBUG("subscribe CCC UUID is 0x%s result:%d", uuidFormat(params->uuid), err);

    if(characteristic && characteristic->scfun) characteristic->scfun(connHandle, params->valueHdl, err);

    blt_gapc_freeGapMsgHeader(connHandle);
}

static ble_sts_t blt_gapc_createSubscribeCCC(u16 connHandle, uuid_t* uuid, u16 handle, u16 value)
{
    blt_gapc_msg_t* msg = blt_gapc_mallocNewGapMsg();
    if(!msg)
        return GAP_ERR_INVALID_PARAMETER;

    blt_gapc_discovery_t* gapDisc = blt_gapc_getDisc(connHandle);

    BLT_GAPC_DEBUG("create found subscribe CCC message, UUID is 0x%s, handle:%x, value:%x", uuidFormat(uuid), handle, value);

    msg->type = GAPC_MSG_SUB_CCC;
    msg->connHandle = connHandle;
    gattc_sub_ccc_cfg_t *pCccCfg = &msg->cccCfg;

    pCccCfg->uuid = uuid;
    pCccCfg->valueHdl = handle;
    pCccCfg->value = value;
    pCccCfg->func = blt_gapc_subscribeCccCb;
    blt_gapc_addGapcMsg(&gapDisc->gattcMsg, msg);
    return BLE_SUCCESS;
}
//////////////////////////////GAPC subscribe client characteristic configuration ending///////////////////////////

//////////////////////////////GAPC sdp discovery include characteristic uuid start///////////////////////////
static u8 blt_gapc_discInclCharCb(u16 connHandle, gatt_attr_t *attr, gattc_sdp_cfg_t *params)
{
    blt_gapc_discovery_t* gapDisc = blt_gapc_getDisc(connHandle);

    if(attr == NULL)
    {
        blt_gapc_freeGapMsgHeader(connHandle);
        if((gapDisc->lastChar.handle != ATT_HANDLE_NONE && gapDisc->lastChar.handle <= params->endHdl)
                && (gapDisc->lastChar.subscribeNtf || gapDisc->lastChar.subscribeInd || gapDisc->lastChar.findDecs))
            blt_gapc_createDiscDescriptors(gapDisc, params->endHdl);
        gapDisc->lastChar.handle = ATT_HANDLE_NONE;
        return GATT_PROC_END;
    }

    gatt_chrc_t *chrc = (gatt_chrc_t*)attr->user_data;

    BLT_GAPC_DEBUG("found include characteristic, UUID is 0x%s, properties:%x, valueHandle:%x", uuidFormat(&chrc->uuid) ,chrc->properties, chrc->valueHdl);

    const blc_gapc_discInclude_t* incl = blt_gapc_checkIncludeUuid(connHandle, params->uuid);

    blc_gapc_discChar_t* inclChar =blt_gapc_checkCharUuidTable(&incl->characteristic, &chrc->uuid);

    if(!inclChar)
    {
        if(incl->characteristic.ufun)
        {
            incl->characteristic.ufun(connHandle, &chrc->uuid, chrc->properties, chrc->valueHdl);
        }
    }
    else
    {
        if(inclChar->cfun)
        {
            inclChar->cfun(connHandle, 0, chrc->properties, chrc->valueHdl);
        }

        if(inclChar->readValue  && (chrc->properties & CHAR_PROP_READ) && inclChar->rfun)
        {
            u8* wBuff = NULL;
            u16* wBuffLen = NULL;
            gapc_read_func_t rdCbFunc = NULL;
            u16 maxLen = 0;
            inclChar->rfun(connHandle, chrc->valueHdl, &wBuff, &wBuffLen, &maxLen, &rdCbFunc);
            blt_gapc_createReadAttrValue(connHandle, chrc->valueHdl, wBuff, wBuffLen, maxLen, rdCbFunc);
        }
    }

    if(gapDisc->lastChar.handle != ATT_HANDLE_NONE && chrc->attrHdl != gapDisc->lastChar.handle )
    {
        if(gapDisc->lastChar.subscribeNtf || gapDisc->lastChar.subscribeInd || gapDisc->lastChar.findDecs)
            blt_gapc_createDiscDescriptors(gapDisc, chrc->attrHdl - 1);
    }

    gapDisc->lastChar.handle = chrc->valueHdl + 1;
    gapDisc->lastChar.uuid = inclChar? &inclChar->uuid: NULL;
    gapDisc->lastChar.setting = inclChar? inclChar->setting: 0x00;
    gapDisc->lastChar.properties = chrc->properties;

    return GATT_PROC_CONT;
}

static ble_sts_t blt_gapc_createDiscInclChar(blt_gapc_discovery_t* disc, blt_gapc_discAttrInfo_t* info)
{
    BLT_GAPC_DEBUG("create disc include characteristic message, startHandle:%x endHandle:%x", info->startHandle, info->endHandle);
    return blt_gapc_createSdpMsg(disc, info->uuid, info->startHandle, info->endHandle,
                GATT_DISCOVER_CHARACTERISTIC, 0, blt_gapc_discInclCharCb);
}
//////////////////////////////GAPC sdp discovery include characteristic uuid ending///////////////////////////

typedef void(*evtActFun_t)(blt_gapc_discovery_t* disc);

typedef enum{
    GAPC_FSM_EVT_INIT,
    GAPC_FSM_EVT_MSG_EMPTY,
    GAPC_FSM_EVT_NO_INCLUDE_INFO,
    GAPC_FSM_EVT_SERVICE_NO_INDEX,
    GAPC_FSM_EVT_END,

    GAPC_FSM_EVT_RECONN_INIT,
    GAPC_FSM_EVT_NO_RECONN_CHAR,
    GAPC_FSM_EVT_SERVICE_END,
    GAPC_FSM_EVT_SERVICE_CHAR_END,
} blt_gapc_fsm_evt_enum;

typedef struct{
    u8 curState;
    u8 event;
    u8 nextState;
    evtActFun_t cb;
} gapcFsmTable_t;

static void blt_gapc_sdpDiscFindService(blt_gapc_discovery_t* disc)
{
    blt_gapc_creatDiscServiceMsg(disc);
    BLT_GAPC_DEBUG("start discovery service information, connHandle:%x", disc->connHandle);
}

static void blt_gapc_sdpDiscFindInclude(blt_gapc_discovery_t* disc)
{
    BLT_GAPC_DEBUG("start discovery include information, connHandle:%x", disc->connHandle);
    if(disc->list->includeTable.size) {
        blt_gapc_discAttrInfo_t *info = &disc->info[disc->serviceIndex];
        blt_gapc_createDiscIncludeMsg(disc, info->startHandle, info->endHandle);
    }
    disc->serviceIndex ++;
}

static void blt_gapc_sdpDiscFindIncludeEnd(blt_gapc_discovery_t* disc)
{
    disc->serviceIndex = 0;
    BLT_GAPC_DEBUG("discovery include information finish, connHandle:%x", disc->connHandle);
}

static void blt_gapc_sdpDiscFindServiceChar(blt_gapc_discovery_t* disc)
{
    if(disc->list->characteristicTable.size)
    {
        blt_gapc_discAttrInfo_t *info = &disc->info[disc->serviceIndex];
        blt_gapc_createDiscChar(disc, info->startHandle, info->endHandle);
    }
    disc->serviceIndex ++;
    BLT_GAPC_DEBUG("find service characteristic information, connHandle:%x", disc->connHandle);
}

static void blt_gapc_sdpDiscFindIncludeChar(blt_gapc_discovery_t* disc)
{
    if(disc->list->includeTable.size && disc->includeSize) {
        blt_gapc_discAttrInfo_t *info = &disc->info[disc->serviceSize + disc->includeIndex];
        const blc_gapc_discInclude_t* discIncl = blt_gapc_checkIncludeUuid(disc->connHandle, info->uuid);

        if(discIncl->ifun && discIncl->ifun(disc->connHandle, info->startHandle, info->endHandle))
        {
            BLT_GAPC_DEBUG("    found include service UUID is 0x%s, startHandle:%x endHandle:%x", uuidFormat(info->uuid), info->startHandle, info->endHandle);
            blt_gapc_createDiscInclChar(disc, info);
        }
    }
    disc->includeIndex ++;
    BLT_GAPC_DEBUG("find include characteristic information, connHandle:%x", disc->connHandle);
}

static void blt_gapc_sdpDiscIdleState(blt_gapc_discovery_t* disc)
{
    gapc_foundService_func_t sfun = disc->list->service->sfun;
    u16 connHandle = disc->connHandle;
    memset((u8*)disc, 0, sizeof(blt_gapc_discovery_t));
    BLT_GAPC_DEBUG("sdp discovery idle state, connHandle:%x", connHandle);
    if(sfun)
    {
        sfun(connHandle, 0, ATT_HANDLE_NONE, ATT_HANDLE_NONE);
    }
}

static void blt_gapc_sdpReconnDealChar(u16 connHandle, const blc_gapc_reconnChar_t* reconnChar)
{
    if(!reconnChar)
        return ;

    blc_gapc_charInfo_t charInfo[20];
    int charInfoCnt = 0;
    if(reconnChar->ifun && (charInfoCnt = reconnChar->ifun(connHandle, charInfo)))
    {
        BLT_GAPC_DEBUG("reconnect characteristic count is %d", charInfoCnt);

        for(int i=0; i<charInfoCnt; i++)
        {
            if((charInfo[i].properties & CHAR_PROP_READ) &&
                    charInfo[i].valueHandle && reconnChar->rfun)
            {
                u8* wBuff = NULL;
                u16* wBuffLen = NULL;
                u16 maxLen = 0;
                gapc_read_func_t rdCbFunc = NULL;
                reconnChar->rfun(connHandle, charInfo[i].valueHandle, &wBuff, &wBuffLen, &maxLen, &rdCbFunc);
                blt_gapc_createReadAttrValue(connHandle, charInfo[i].valueHandle, wBuff, wBuffLen, maxLen, rdCbFunc);
            }

            u16 value = 0;
            if(charInfo[i].properties & CHAR_PROP_NOTIFY) {
                value |= BIT(0);
            }
            if(charInfo[i].properties & CHAR_PROP_INDICATE){
                value |= BIT(1);
            }
            if(value && charInfo[i].cccHandle) {
                blt_gapc_createSubscribeCCC(connHandle, NULL, charInfo[i].cccHandle, value);
            }

        }
    }
}

static void blt_gapc_sdpReconnChar(u16 connHandle, const blc_gapc_reconnCharTable_t* charTable)
{
    const blc_gapc_reconnChar_t *pChar = charTable->characteristic;
    for(int i=0; i<charTable->size; i++)
    {
        blt_gapc_sdpReconnDealChar(connHandle, pChar);
        pChar ++;
    }
}

static void blt_gapc_sdpReconnGetService(blt_gapc_discovery_t* disc)
{
    u16 connHandle = disc->connHandle;
    BLT_GAPC_DEBUG("sdp reconnect get service count, connHandle:0x%x", connHandle);

    if(disc->reconnList->resfun)
    {
        disc->serviceIndex++;
        if(disc->reconnList->resfun(connHandle, disc->serviceIndex))
        {
            disc->serviceSize++;
        }
    }
    else
    {
        disc->serviceIndex ++;
        disc->serviceSize = 0x01;
    }

    if(disc->serviceSize >= disc->serviceIndex)
    {
        blt_gapc_sdpReconnChar(connHandle, &disc->reconnList->charTb);
    }
    else
    {
        disc->state = RECONNECT_STATE_GET_RECONN_SERVICE_END;   //FSM table;
    }
}

static void blt_gapc_sdpReconnGetIncl(blt_gapc_discovery_t* disc)
{
    u16 connHandle = disc->connHandle;
    BLT_GAPC_DEBUG("sdp reconnect get include count, connHandle:0x%x", connHandle);
    const blc_gapc_reconnList_t *list = disc->reconnList;

    if(list->inclSize == 0)
    {
        disc->state = RECONNECT_STATE_GET_RECONN_INCL_END;  //FSM table;
        return ;
    }

    if(list->inclSize > disc->includeIndex)
    {
        const blc_gapc_reconnInclTable_t* inclList = list->includeCharTb[disc->includeIndex];
        if(inclList->reifun)
        {
            if(inclList->reifun(connHandle, disc->includeSize + 1))
            {
                disc->includeSize ++;
                blt_gapc_sdpReconnChar(connHandle, &inclList->charTb);
            }
            else
            {
                disc->includeSize = 0;
                disc->includeIndex++;
            }
        }
        else
        {
            blt_gapc_sdpReconnChar(connHandle, &inclList->charTb);
            disc->includeIndex++;
        }
    }
    else
    {
        disc->state = RECONNECT_STATE_GET_RECONN_INCL_END;  //FSM table;
    }
}

static void blt_gapc_sdpDiscIdleState2(blt_gapc_discovery_t* disc)
{
    u16 connHandle = disc->connHandle;
    const blc_gapc_reconnList_t* list = disc->reconnList;
    memset((u8*)disc, 0, sizeof(blt_gapc_discovery_t));
    BLT_GAPC_DEBUG("sdp discovery idle state, connHandle:%x", connHandle);
    if(list->resfun)
    {
        list->resfun(connHandle, 0x00);         //reconnect service finish.
    }
}

static const gapcFsmTable_t gapcFsmTb[] = {
    {DISCOVERY_STATE_IDLE, GAPC_FSM_EVT_INIT, DISCOVERY_STATE_FIND_SERVICE, blt_gapc_sdpDiscFindService},
    {DISCOVERY_STATE_FIND_SERVICE, GAPC_FSM_EVT_NO_INCLUDE_INFO, DISCOVERY_STATE_FIND_SERVICE_CHAR, blt_gapc_sdpDiscFindServiceChar},
    {DISCOVERY_STATE_FIND_SERVICE, GAPC_FSM_EVT_MSG_EMPTY, DISCOVERY_STATE_FIND_INCLUDE, blt_gapc_sdpDiscFindInclude},
    {DISCOVERY_STATE_FIND_INCLUDE, GAPC_FSM_EVT_MSG_EMPTY, DISCOVERY_STATE_FIND_INCLUDE, blt_gapc_sdpDiscFindInclude},
    {DISCOVERY_STATE_FIND_INCLUDE, GAPC_FSM_EVT_SERVICE_NO_INDEX, DISCOVERY_STATE_FIND_INCLUDE_END, blt_gapc_sdpDiscFindIncludeEnd},

    {DISCOVERY_STATE_FIND_INCLUDE_END, GAPC_FSM_EVT_MSG_EMPTY, DISCOVERY_STATE_FIND_SERVICE_CHAR, blt_gapc_sdpDiscFindServiceChar},
    {DISCOVERY_STATE_FIND_SERVICE_CHAR, GAPC_FSM_EVT_MSG_EMPTY, DISCOVERY_STATE_FIND_SERVICE_CHAR, blt_gapc_sdpDiscFindServiceChar},

    {DISCOVERY_STATE_FIND_SERVICE_CHAR, GAPC_FSM_EVT_SERVICE_NO_INDEX, DISCOVERY_STATE_FIND_INCLUDE_CHAR, blt_gapc_sdpDiscFindIncludeChar},
    {DISCOVERY_STATE_FIND_SERVICE_CHAR, GAPC_FSM_EVT_END, DISCOVERY_STATE_IDLE, blt_gapc_sdpDiscIdleState},

    {DISCOVERY_STATE_FIND_INCLUDE_CHAR, GAPC_FSM_EVT_MSG_EMPTY, DISCOVERY_STATE_FIND_INCLUDE_CHAR, blt_gapc_sdpDiscFindIncludeChar},
    {DISCOVERY_STATE_FIND_INCLUDE_CHAR, GAPC_FSM_EVT_END, DISCOVERY_STATE_IDLE, blt_gapc_sdpDiscIdleState},

    {DISCOVERY_STATE_IDLE, GAPC_FSM_EVT_RECONN_INIT, RECONNECT_STATE_GET_RECONN_SERVICE, blt_gapc_sdpReconnGetService},
    {RECONNECT_STATE_GET_RECONN_SERVICE, GAPC_FSM_EVT_MSG_EMPTY, RECONNECT_STATE_GET_RECONN_SERVICE, blt_gapc_sdpReconnGetService},
    {RECONNECT_STATE_GET_RECONN_SERVICE_END, GAPC_FSM_EVT_MSG_EMPTY, RECONNECT_STATE_GET_RECONN_INCL, blt_gapc_sdpReconnGetIncl},
    {RECONNECT_STATE_GET_RECONN_INCL, GAPC_FSM_EVT_MSG_EMPTY, RECONNECT_STATE_GET_RECONN_INCL, blt_gapc_sdpReconnGetIncl},
    {RECONNECT_STATE_GET_RECONN_INCL_END, GAPC_FSM_EVT_MSG_EMPTY, DISCOVERY_STATE_IDLE, blt_gapc_sdpDiscIdleState2},
};

static void blt_gapc_fsmDealEvt(blt_gapc_discovery_t* disc, blt_gapc_fsm_evt_enum evtID)
{
    for(size_t i = 0; i < ARRAY_SIZE(gapcFsmTb); i++)
    {
        if(gapcFsmTb[i].curState == disc->state && gapcFsmTb[i].event == evtID)
        {
            disc->state = gapcFsmTb[i].nextState;
            gapcFsmTb[i].cb(disc);
            break;
        }
    }
}

static void blt_gapc_discovery(blt_gapc_discovery_t* disc)
{
    blt_gapc_msg_t* msg = (blt_gapc_msg_t*)disc->gattcMsg.list.slh_first;
    if(msg && msg->state == GAPC_MSG_STORED)
    {
        blt_gapc_dealGapcMsg(msg);
    }
    else if(!msg && (disc->state <= DISCOVERY_STATE_END) && disc->infoSize)
    {
        if(disc->serviceSize && disc->serviceSize == disc->serviceIndex){   //
            if(!disc->includeSize || disc->includeSize == disc->includeIndex)
                blt_gapc_fsmDealEvt(disc, GAPC_FSM_EVT_END);
            else if(!disc->includeIndex)
                blt_gapc_fsmDealEvt(disc, GAPC_FSM_EVT_SERVICE_NO_INDEX);
            else if(disc->list->includeTable.size)
                blt_gapc_fsmDealEvt(disc, GAPC_FSM_EVT_MSG_EMPTY);
            else
                blt_gapc_fsmDealEvt(disc, GAPC_FSM_EVT_NO_INCLUDE_INFO);
        }
        else
            if(disc->list->includeTable.size)
                blt_gapc_fsmDealEvt(disc, GAPC_FSM_EVT_MSG_EMPTY);
            else
                blt_gapc_fsmDealEvt(disc, GAPC_FSM_EVT_NO_INCLUDE_INFO);
    }
    else if(!msg && disc->state > DISCOVERY_STATE_END)
    {
        blt_gapc_fsmDealEvt(disc, GAPC_FSM_EVT_MSG_EMPTY);
    }
}

static void blt_gapc_discoveryDisc(blt_gapc_discovery_t* disc)
{
    blt_gapc_msg_t* msg = NULL;
    do{
        msg = SPLIST_DELETE_HEAD(&disc->gattcMsg.list, blt_gapc_msg_t);
        if(msg)
        {
            GAPC_FREE(msg);
        }
    }while(msg);
    memset((u8*)disc, 0, sizeof(blt_gapc_discovery_t));
    BLT_GAPC_DEBUG("clear discovery");
}

ble_sts_t blc_gapc_registerReconnectService(u16 connHandle, const blc_gapc_reconnList_t *list)
{
    blt_gapc_discovery_t* gapcDisc = blt_gapc_createDisc(connHandle);
    if(!gapcDisc)
    {
        return GAP_ERR_STATE_NO_IDLE;
    }

    memset((u8*)gapcDisc, 0, sizeof(blt_gapc_discovery_t));
    gapcDisc->gattcMsg.cb = blt_gapc_getMsgPriority;
    gapcDisc->connHandle = connHandle;
    gapcDisc->reconnList = list;
    blt_gapc_fsmDealEvt(gapcDisc, GAPC_FSM_EVT_RECONN_INIT);
    return BLE_SUCCESS;
}

ble_sts_t blc_gapc_registerDiscoveryService(u16 connHandle, const blc_gapc_discList_t *list)
{
    blt_gapc_discovery_t* gapcDisc = blt_gapc_createDisc(connHandle);
    if(!gapcDisc)
    {
        return GAP_ERR_STATE_NO_IDLE;
    }
#if DBG_GAPC_LOG
    BLT_GAPC_DEBUG("init discovery, found service count is %x, UUID is 0x%s", list->maxServiceCount, uuidFormat(&list->service->uuid));
//  if(list->characteristicTable.size){
//      blc_gapc_discChar_t * pChar = (blc_gapc_discChar_t *)list->characteristicTable.characteristic;
//      for(int i=0; i<list->characteristicTable.size; i++) {
//          BLT_GAPC_DEBUG("    | characteristic UUID is 0x%s", uuidFormat(&pChar->uuid));
//          pChar++;
//      }
//  }
    if(list->includeTable.size){
        BLT_GAPC_DEBUG("    |include service count is %x", list->includeTable.size);
        for(int i=0; i<list->includeTable.size; i++) {
            blc_gapc_discInclude_t * pIncl = (blc_gapc_discInclude_t*)list->includeTable.include[i];
            BLT_GAPC_DEBUG("    |include service UUID is 0x%s", uuidFormat(&pIncl->uuid));
            if(pIncl->characteristic.size){
                blc_gapc_discChar_t * pChar = (blc_gapc_discChar_t *)pIncl->characteristic.characteristic;
                for(int i=0; i<pIncl->characteristic.size; i++) {
                    BLT_GAPC_DEBUG("        | include characteristic UUID is 0x%s", uuidFormat(&pChar->uuid));
                    pChar++;
                }
            }
        }
    }
#endif
    memset((u8*)gapcDisc, 0, sizeof(blt_gapc_discovery_t));
    gapcDisc->gattcMsg.cb = blt_gapc_getMsgPriority;
    gapcDisc->connHandle = connHandle;
    gapcDisc->list = list;
    blt_gapc_fsmDealEvt(gapcDisc, GAPC_FSM_EVT_INIT);
    return BLE_SUCCESS;
}

void blc_gapc_discoveryOrReconnectService_loop(void)
{
    for(int i=0; i<GAPC_DISCOVERY_MAX_NUM; i++)
    {
        if(gGapcDisc[i].state != DISCOVERY_STATE_IDLE)
        {
            u16 connHandle = gGapcDisc[i].connHandle;

            u8 conn_idx = connHandle & CONN_IDX_MASK;

            st_ll_conn_t* pc = (st_ll_conn_t*)(u32)&blms[conn_idx];

            if(pc->connState == 0){
                return blt_gapc_discoveryDisc(&gGapcDisc[i]);
            }

            blt_gapc_discovery(&gGapcDisc[i]);
        }
    }
}

const uuid_t* blc_gapc_getDiscoveryServiceUUID(u16 connHandle)
{
    blt_gapc_discovery_t* gapDisc = blt_gapc_getDisc(connHandle);
    if(gapDisc->state == DISCOVERY_STATE_IDLE)
        return NULL;

    if(gapDisc->state < DISCOVERY_STATE_END)
        return &gapDisc->list->service->uuid;

    return &gapDisc->reconnList->serviceUuid;
}




//////////////////////////////GAPC sdp discovery ending///////////////////////////

typedef struct{
    gapc_write_func_t fun;
    void* cbData;
    gattc_write_cfg_t gattcCfg;
} gapc_write_ctrl_t;

_attribute_ble_data_retention_
gapc_write_ctrl_t gGapcWriteCfg[LL_MAX_ACL_CONN_NUM];
static struct gap_stateChangeNode gapcRWStateChange;

static void gapc_read_write_attribute_disconnect(u16 connHandle, GAP_STATE_ENUM state, void* node)
{
    (void)node;
    if(GAP_STATE_ACL_DISCONNECTED == state)
    {
        gattc_write_cfg_t* gattWrCfg = &gGapcWriteCfg[connHandle&0x0f].gattcCfg;
        gattWrCfg->func = NULL;
        blt_gap_unregAclConnState(&gapcRWStateChange);
    }
}

static struct gap_stateChangeNode gapcRWStateChange = {
    .next = NULL,
    .cb = gapc_read_write_attribute_disconnect,
};

static void blt_gapc_writeAttributeValueCallBack(u16 connHandle, u8 err, struct gattc_write_cfg *pWrCfg)
{
    (void)pWrCfg;
    blt_gap_unregAclConnState(&gapcRWStateChange);
    gattc_write_cfg_t* gattWrCfg = &gGapcWriteCfg[connHandle&0xf].gattcCfg;
    gattWrCfg->func = NULL;
    if(gGapcWriteCfg[connHandle&0xf].fun)
        gGapcWriteCfg[connHandle&0xf].fun(connHandle, err, gGapcWriteCfg[connHandle&0xf].cbData);
}

ble_sts_t blc_gapc_writeAttributeValue(u16 connHandle, gapc_write_cfg_t *pGapWrCfg)
{
    if(blt_ll_isAclhdlInvalid(connHandle))
    {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    gattc_write_cfg_t* gattWrCfg = &gGapcWriteCfg[connHandle&0xf].gattcCfg;
    if(gattWrCfg->func){
        return GAP_ERR_WRITE_BUSY;
    }
    if(pGapWrCfg->handle == 0x00) {
        return GAP_ERR_INVALID_PARAMETER;
    }

    gattWrCfg->func = blt_gapc_writeAttributeValueCallBack;
    gattWrCfg->handle = pGapWrCfg->handle;
    gattWrCfg->offset = 0x0000;
    gattWrCfg->data = pGapWrCfg->data;
    gattWrCfg->length = pGapWrCfg->length;
    gattWrCfg->withoutRsp = pGapWrCfg->withoutRsp;
    gGapcWriteCfg[connHandle&0xf].fun = pGapWrCfg->func;
    gGapcWriteCfg[connHandle&0xf].cbData = pGapWrCfg->cbData;

    ble_sts_t state = blc_gattc_writeAttributeValue(connHandle, gattWrCfg);
    if(state != BLE_SUCCESS || gattWrCfg->withoutRsp)
    {
        gattWrCfg->func = NULL;
    }
    else
    {
        blt_gap_regAclConnState(&gapcRWStateChange);
    }
    return state;
}


typedef struct{
    gapc_read_func_t fun;
    gattc_read_cfg_t gattcCfg;
} gapc_read_ctrl_t;

_attribute_ble_data_retention_
gapc_read_ctrl_t gGapcReadCfg[LL_MAX_ACL_CONN_NUM];

static u8 blt_gapc_readAttributeValueCallback(u16 connHandle, u8 err, gatt_read_data_t *rdData, gattc_read_cfg_t *pRdCfg)
{
    if(err != ATT_SUCCESS)
    {
        if(gGapcReadCfg[connHandle&0xf].fun)
            gGapcReadCfg[connHandle&0xf].fun(connHandle, err, pRdCfg);
        return GATT_PROC_END;
    }

    if (rdData->rdState == GATT_RD_CMPLT)
    {
        gattc_read_cfg_t* gattReCfg = &gGapcReadCfg[connHandle&0xf].gattcCfg;
        gattReCfg->func = NULL;
        blt_gap_unregAclConnState(&gapcRWStateChange);
        if(gGapcReadCfg[connHandle&0xf].fun)
            gGapcReadCfg[connHandle&0xf].fun(connHandle, err, pRdCfg);
    }
    return GATT_PROC_CONT;
}

ble_sts_t blc_gapc_readAttributeValue(u16 connHandle, gapc_read_cfg_t *pGapReCfg)
{
    if(blt_ll_isAclhdlInvalid(connHandle))
    {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    gattc_read_cfg_t* gattReCfg = &gGapcReadCfg[connHandle&0xf].gattcCfg;
    if(gattReCfg->func) {
        return GAP_ERR_WRITE_BUSY;
    }
    if(pGapReCfg->handle == 0x00) {
        return GAP_ERR_INVALID_PARAMETER;
    }
    gattReCfg->func = blt_gapc_readAttributeValueCallback;
    gattReCfg->hdlCnt = 1;
    gattReCfg->single.handle = pGapReCfg->handle;
    gattReCfg->single.offset = 0;
    gattReCfg->single.wBuff = pGapReCfg->wBuff;
    gattReCfg->single.wBuffLen = pGapReCfg->wBuffLen;
    gattReCfg->single.maxLen = pGapReCfg->maxLen;

    gGapcReadCfg[connHandle&0xf].fun = pGapReCfg->func;
    ble_sts_t state = blc_gattc_readAttributeValue(connHandle, gattReCfg);
    if(state != BLE_SUCCESS)
    {
        gattReCfg->func = NULL;
    }
    else
    {
        blt_gap_regAclConnState(&gapcRWStateChange);
    }
    return state;
}
