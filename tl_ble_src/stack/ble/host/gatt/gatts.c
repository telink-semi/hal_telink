/********************************************************************************************************
 * @file    gatts.c
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
#include "stack/ble/host/att/atts.h"
#include "gatts.h"
#include "stack/ble/host/gatt/tlk_list_stack.h"
#include "common/tl_queue.h"
#include "stack/ble/host/gatt/tlk_malloc_stack.h"

#define BLT_GATTS_DEBUG(fmt, ...)               BLT_HOST_DBUG(DBG_GATTS_LOG, "[GATTS]"fmt, ##__VA_ARGS__)

#define GATTS_MALLOC(len)                       malloc_nonreten(len)
#define GATTS_FREE(ptr)                         free_nonreten(ptr)

static int blt_gatts_getAttributeServicePriorityCb(void* node)
{
    return ((atts_group_t*)node)->startHandle;
}

_attribute_ble_data_retention_
SPLIST_DEF(gAttSrvGroup, blt_gatts_getAttributeServicePriorityCb);

atts_group_t *blc_gatts_getAttributeServiceGroup(u16 connHandle)
{
    (void)connHandle;
    return (atts_group_t*)gAttSrvGroup.list.slh_first;
}

void blc_gatts_addAttributeServiceGroup(atts_group_t *pGroup)
{
    SPLIST_INSERT_NODE(&gAttSrvGroup, pGroup);
}

void blc_gatts_removeAttributeServiceGroup(u16 startHandle)
{
    SPLIST_DELETE_PRIO(&gAttSrvGroup, startHandle);
}

ble_sts_t blc_gatts_getAttributeInformationByHandle(u16 connHandle, u16 handle, u8** attrValue, u16** attrValueLen)
{
    const atts_attribute_t * pAttr = blt_atts_findByHandle(connHandle, handle, NULL);
    if(pAttr == NULL)
    {
        return LE_AUDIO_SERVER_INVALID_HANDLE;
    }

    if(attrValue)
        *attrValue = pAttr->attrValue;
    if(attrValueLen)
        *attrValueLen = pAttr->attrValueLen;

    return BLE_SUCCESS;
}

u8* blc_gatts_getAttributeValueByHandle(u16 connHandle, u16 handle)
{
    u8* value = NULL;
    blc_gatts_getAttributeInformationByHandle(connHandle, handle, &value, NULL);
    return value;
}

u8* blc_gatts_getReportReferenceValue(u16 connHandle, u16 handle)
{
    do{
        handle++;       //first skip attribute value handle.
        const atts_attribute_t * pAttr = blt_atts_findByHandle(connHandle, handle, NULL);
        if(pAttr == NULL)
        {
            return NULL;
        }

        if(blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, declarationsCharacteristicUuid))
        {
            return NULL;
        }

        if(blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, descriptorReportReferenceUuid))
        {
            return pAttr->attrValue;
        }

    }while(true);

    return NULL;
}

#define DATABASE_HASH_MAX_SIZE              100

bool blc_gatts_calculateDatabaseHash(u16 connHandle, u8* databaseHash)
{
    atts_group_t *pGroup = blc_gatts_getAttributeServiceGroup(connHandle);

    u8 htvDataBuf[DATABASE_HASH_MAX_SIZE];
    int htvDataLen = 0;
    u8* htvData = htvDataBuf;
    blc_aes_cmac_context_t aesCmac;

    memset(&aesCmac, 0, sizeof(blc_aes_cmac_context_t));
    u8 key[16];
    memset(key, 0, 16);

    blc_crypto_alg_aes_cmac_init_key(&aesCmac, key);

    if(pGroup == NULL)
    {
        return false;
    }

    for(; pGroup != NULL; pGroup = pGroup->pNext)
    {
        const atts_attribute_t* pAttr = pGroup->pAttr;
        if(pAttr == NULL)
            continue;
        for(int i=0; i<pGroup->endHandle-pGroup->startHandle + 1; i++)
        {
            //Attribute Handle, Attribute Type, Attribute value
            if(blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, declarationsPrimaryServiceUuid) ||
                    blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, declarationsSecondaryServiceUuid) ||
                    blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, declarationsIncludeUuid) ||
                    blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, descriptorCharacteristicExtendedPropertiesUuid))
            {
                U16_TO_STREAM(htvData, pGroup->startHandle + i);
                STR_TO_STREAM(htvData, pAttr->uuid, pAttr->uuidLen);
                STR_TO_STREAM(htvData, pAttr->attrValue, *pAttr->attrValueLen);
                htvDataLen += 2 + pAttr->uuidLen + (*pAttr->attrValueLen);
            }
            //Attribute Handle, Attribute Type,
            else if(blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, descriptorCharacteristicUserDescriptionUuid) ||
                    blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, descriptorClientCharacteristicConfigurationUuid) ||
                    blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, descriptorServerCharacteristicConfigurationUuid) ||
                    blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, descriptorCharacteristicPresentationFormatUuid) ||
                    blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, descriptorCharacteristicAggregateFormatUuid))
            {
                U16_TO_STREAM(htvData, pGroup->startHandle + i);
                STR_TO_STREAM(htvData, pAttr->uuid, pAttr->uuidLen);
                htvDataLen += 2 + pAttr->uuidLen;
            }
            //Attribute Handle, Attribute Type, Attribute value
            else if (blt_atts_uuidCmp(pAttr, ATT_16_UUID_LEN, declarationsCharacteristicUuid)) {
                if (pAttr->settings & ATTS_SET_ATTR_VALUE_PROPERTIES) {
                    U16_TO_STREAM(htvData, pGroup->startHandle + i);
                    STR_TO_STREAM(htvData, pAttr->uuid, pAttr->uuidLen);
                    U8_TO_STREAM(htvData, *pAttr->attrValue);   //Characteristic Properties
                    U16_TO_STREAM(htvData, pGroup->startHandle + i + 1);    //Characteristic Value Handle
                    const atts_attribute_t *pNextAttr = pAttr + 1;
                    STR_TO_STREAM(htvData, pNextAttr->uuid, pNextAttr->uuidLen);    //Characteristic UUID
                    htvDataLen += 2 + pAttr->uuidLen + 1 + 2 + pNextAttr->uuidLen;
                } else {
                    U16_TO_STREAM(htvData, pGroup->startHandle + i);
                    STR_TO_STREAM(htvData, pAttr->uuid, pAttr->uuidLen);
                    STR_TO_STREAM(htvData, pAttr->attrValue, *pAttr->attrValueLen);
                    htvDataLen += 2 + pAttr->uuidLen + (*pAttr->attrValueLen);
                }

            }
            pAttr++;

            if(htvDataLen > 16)
            {
                int j=0;
                for( ; j<htvDataLen-16; j+=16)      //
                {
                    blc_crypto_alg_aes_cmac_block(&aesCmac, htvDataBuf+j);
                }
                memcpy(htvDataBuf, htvDataBuf+j, htvDataLen-j);
                htvDataLen -= j;
                htvData = htvDataBuf + htvDataLen;
            }
        }
    }

    blc_crypto_alg_aes_cmac_finish(&aesCmac, htvDataBuf, htvDataLen);
    for (int i = 0; i < 16; i++){
        databaseHash[15 - i] = aesCmac.mac[i];
    }

    return true;
}

#define GATTS_NTF_MSG_NUM           20

typedef struct{
    u16 connHandle;
    queue_t ntfMsg;
} blt_gatts_ntfQueue_t;

_attribute_ble_data_retention_
blt_gatts_ntfQueue_t gattsNtfHead[LL_MAX_ACL_CONN_NUM] = {0};
_attribute_ble_data_retention_
gatts_notify_cfg_t gattsNtfMSg[GATTS_NTF_MSG_NUM];

static void blt_gatts_releaseNtfMsg(u16 connHandle)
{
    if(blt_ll_isAclhdlInvalid(connHandle))
    {
        return;
    }

    gatts_notify_cfg_t* msg = (gatts_notify_cfg_t*)queue_deq(&gattsNtfHead[connHandle&0x0F].ntfMsg);
    msg->attrhandle = 0x00;
}

void blc_gatts_notifyLoop(void)
{
    for(int i=0; i<LL_MAX_ACL_CONN_NUM; i++)
    {
        gatts_notify_cfg_t* pNtfCfg = (gatts_notify_cfg_t*)gattsNtfHead[i].ntfMsg.head;
        if(pNtfCfg)
        {
            u16 connHandle = gattsNtfHead[i].connHandle;
            if(blc_atts_sendHandleValueNotify(connHandle, pNtfCfg->attrhandle, pNtfCfg->value, pNtfCfg->valueLen) == BLE_SUCCESS)
            {
                GATTS_FREE(pNtfCfg->value);
                blt_gatts_releaseNtfMsg(connHandle);
            }
        }
    }
}

void blc_gatts_notifyDisconnect(u16 connHandle)
{
    if(blt_ll_isAclhdlInvalid(connHandle))
    {
        return;
    }

    queue_t* pNtfCfg = &gattsNtfHead[connHandle&0x0F].ntfMsg;
    gatts_notify_cfg_t* msg = NULL;
    do{
        msg = (gatts_notify_cfg_t*)queue_deq(pNtfCfg);
        if(msg)
        {
            msg->attrhandle = 0;
            GATTS_FREE(msg->value);
        }

    }while(msg);
}

ble_sts_t blc_gatts_notify(u16 connHandle, gatts_notify_cfg_t* pNtfCfg)
{
    if(blt_ll_isAclhdlInvalid(connHandle))
    {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    gattsNtfHead[connHandle&0x0F].connHandle = connHandle;
    queue_t* queue = &gattsNtfHead[connHandle&0x0F].ntfMsg;
    queue_enq(queue, (queue_item_t*)pNtfCfg);
    return BLE_SUCCESS;
}

static gatts_notify_cfg_t* blt_gattc_getNewNtfMsg(u16 connHandle)
{
    if(blt_ll_isAclhdlInvalid(connHandle))
    {
        return NULL;
    }

    for(int i=0; i<GATTS_NTF_MSG_NUM; i++)
    {
        if(gattsNtfMSg[i].attrhandle)
            continue;
        return &gattsNtfMSg[i];
    }
    return NULL;
}

ble_sts_t blc_gatts_notifyAttr(u16 connHandle, u16 handle)
{
    u8* value = NULL;
    u16* len = NULL;
    if(blc_gatts_getAttributeInformationByHandle(connHandle, handle, &value, &len) != BLE_SUCCESS)
    {
        return GATT_ERR_INVALID_PARAMETER;
    }

    return blc_gatts_notifyValue(connHandle, handle, value, len == NULL? 0: *len);
}

ble_sts_t blc_gatts_notifyValue(u16 connHandle, u16 handle, u8* value, u16 valueLen)
{
    gatts_notify_cfg_t* msg = blt_gattc_getNewNtfMsg(connHandle);
    if(msg == NULL)
        return GATT_ERR_INVALID_PARAMETER;

    if(!handle)
        return GATT_ERR_INVALID_PARAMETER;

    if(valueLen == 0)
    {
        msg->attrhandle = handle;
        msg->value = NULL;
        msg->valueLen = 0;
    }
    else
    {
        u8 *pValue = GATTS_MALLOC(valueLen);

        if(pValue == NULL)
        {
            return GATT_ERR_NOTIFY_INDICATION_BUSY;
        }

        msg->attrhandle = handle;
        msg->value = pValue;
        msg->valueLen = valueLen;
        memcpy(pValue, value, valueLen);
    }

    return blc_gatts_notify(connHandle, msg);
}

typedef struct __attribute__((packed)){
    struct gap_stateChangeNode aclState;
    gatts_cfmCb cb;
    u16 scid;
} gattsInd_t;

static _attribute_ble_data_retention_
SLIST_DEF(gattsIndList);

static ble_sts_t blt_atts_sendHandleValueIndicate (gattsIndValue_t* ind)
{
    if(ind->attrHandle == 0 || ind->value == NULL){
        return GATT_ERR_INVALID_PARAMETER;
    }

    u16 mtu = blt_gap_getScidMtu(ind->connHandle, L2CAP_CID_ATTR_PROTOCOL);

    if(mtu-ATT_VALUE_IND_LEN < ind->valueLen)
    {
        return GATT_ERR_INVALID_PARAMETER;
    }

    if(blt_ll_isAclhdlInvalid(ind->connHandle))
    {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    //HandleValueIndicate format
    u8 format[3];
    format[0] = ATT_OP_HANDLE_VALUE_IND;
    format[1] = U16_LO(ind->attrHandle);
    format[2] = U16_HI(ind->attrHandle);

    return blt_l2cap_pushData_2_controller(ind->connHandle, L2CAP_CID_ATTR_PROTOCOL, format, 3, ind->value, ind->valueLen);
}

void blt_gatts_recvIndCfm(u16 connHandle, u16 scid)
{
    gattsInd_t* gattsInd = blt_host_getAclConn(&gattsIndList, connHandle, scid);

    if(gattsInd == NULL)    return ;

    gatts_cfmCb cb = gattsInd->cb;

    blt_gap_unregAclConnState(&gattsInd->aclState);
    blt_host_freeAclConn(&gattsIndList, gattsInd);

    if(cb) cb(connHandle, scid);
}

static void blc_gatts_indicateValueAclStateCallback(u16 connHandle, GAP_STATE_ENUM state, void* node)
{
    if(state == GAP_STATE_ACL_DISCONNECTED)
    {
        gattsInd_t* gattsInd = (gattsInd_t*)((u8*)node - OFFSETOF(gattsInd_t, aclState));

        gatts_cfmCb cb = gattsInd->cb;

        blt_gap_unregAclConnState(&gattsInd->aclState);
        blt_host_freeAclConn(&gattsIndList, gattsInd);

        if(cb) cb(connHandle, gattsInd->scid);
    }
    BLT_GATTS_DEBUG("INF: send indicate, connHandle[0x%04x], acl state is %d", connHandle, state);
}

ble_sts_t blc_gatts_indicateValue(gattsIndValue_t* ind)
{
    if(blt_host_getAclConn(&gattsIndList, ind->connHandle, ind->scid)) {
        return GATT_ERR_NOTIFY_INDICATION_BUSY;
    }

    gattsInd_t* gattsInd = blt_host_mallocAclConn(&gattsIndList, ind->connHandle, ind->scid, sizeof(gattsInd_t));

    if(gattsInd == NULL) {
        return GATT_ERR_NOTIFY_INDICATION_BUSY;
    }

    ble_sts_t status = blt_atts_sendHandleValueIndicate(ind);

    if(status != BLE_SUCCESS) {
        blt_host_freeAclConn(&gattsIndList, gattsInd);
        return status;
    }

    BLT_GATTS_DEBUG("send indicate, connHandle[0x%04x], attrHandle[0x%04x]", ind->connHandle, ind->attrHandle);

    gattsInd->cb = ind->cb;
    gattsInd->aclState.cb = blc_gatts_indicateValueAclStateCallback;
    gattsInd->scid = ind->scid;
    blt_gap_regAclConnState(&gattsInd->aclState);

    return BLE_SUCCESS;
}

