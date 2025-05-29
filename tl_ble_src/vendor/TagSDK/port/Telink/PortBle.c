/* ***************************************************************************
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of Samsung
 * Electronics Co., Ltd. ("Confidential Information").
 * You shall not disclose such Confidential Information and shall use it
 * only in accordance with the terms of the license agreement you entered
 * into with Samsung Electronics Co., Ltd. ("SAMSUNG")
 * SAMSUNG MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SAMSUNG SHALL NOT BE
 * LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 *
 ****************************************************************************/

#include "TagConfig.h"

#include "TagErrorType.h"

#include "PortBle.h"

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app_att.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG
#define BT_GATT_ERR(result) ATT_ERR_INVALID_PDU
#define  UNUSEDARG(x)  ((void )x);
#define attTableMaxSize  (TAG_CHAR_END*4)
#define MY_ATT_TABLE_MAX_NUM  attTableMaxSize
#define UUID_128_LEN  16

_attribute_ble_data_retention_ u8 tagHandleTable[MY_ATT_TABLE_MAX_NUM] = { 0 };
u16 default_conn = 0;
_attribute_ble_data_retention_ u8 connect_pair_flag[ACL_PERIPHR_MAX_NUM] = {0};

extern attribute_t my_Attributes[];
extern int att_custom_read_rsp_len ;

TagError_t PortBleAddGattDbService(ServiceType service_type)
{
    TagError_t ret = TAG_ERROR_NONE;
    TAG_LOG_D("PortBleAddGattDbService(%d) %d ", service_type);
    return ret;
}


static int onboarding_svc_attr_read_callback(u16 conn_handle, u16 handle, int tagCharIndex, void *buf, u16 len)
{
    UNUSEDARG(buf);
    UNUSEDARG(len);
    TAG_LOG_D("onboarding_svc_attr_read_callback handle[%d] %d", handle,tagCharIndex);
    int result = 0;

    if (gTagChar[tagCharIndex].callback_flag & READ_CALLBACK_FLAG)
    {
        BleEvent *event;
        event = (BleEvent *)TagMalloc(sizeof(BleEvent));
        memset(event, '\0', sizeof(BleEvent));


        event->eventType = BleAttributeRead;
        event->eventData.gattData.portConnHandle.conn_idx = conn_handle;
        event->eventData.gattData.portConnHandle.auth_result = TAG_ERROR_NONE;
        event->eventData.gattData.portAttrInfo.handle = handle;
        event->eventData.gattData.portAttrInfo.tagCharIndex = tagCharIndex;
        event->eventData.gattData.charIndex = tagCharIndex;

        TagBleCallback(event);

        result = event->eventData.gattData.portConnHandle.auth_result;

        TagFree(event);

        if (result)
        {
            return BT_GATT_ERR(result);
        }
    }
    return BT_GATT_ERR(result);
}

static int onboarding_svc_attr_write_callback(u16 conn_handle, u16 handle, int tagCharIndex, const void *buf, u16 len, u16 offset)
{
    TAG_LOG_D("onboarding_svc_attr_write_callback handle[%d] %d %d", handle,tagCharIndex,len);
    int result = 0;

    if (offset + len > gTagChar[tagCharIndex].maxValueLength)
    {
        return ATT_ERR_INVALID_OFFSET;
    }
    TAG_LOG_D("%c %c %c", (*(u8*)buf), (*((u8*)buf+1)), (*((u8*)buf+2)));

    if (gTagChar[tagCharIndex].callback_flag & WRITE_CALLBACK_FLAG)
    {
        BleEvent *event;
        event = TagMalloc(sizeof(BleEvent));
        memset(event, '\0', sizeof(BleEvent));

        event->eventType = BleAttributeWritten;
        event->eventData.gattData.portConnHandle.conn_idx = conn_handle;
        event->eventData.gattData.portConnHandle.auth_result = TAG_ERROR_NONE;
        event->eventData.gattData.portAttrInfo.handle = handle;
        event->eventData.gattData.portAttrInfo.tagCharIndex = tagCharIndex;
        event->eventData.gattData.charIndex = tagCharIndex;
        event->eventData.gattData.needResponse = true;

        event->eventData.gattData.value = TagMalloc(len);
        memcpy(event->eventData.gattData.value, buf, len);
        event->eventData.gattData.valueLength = len;

        TagBleCallback(event);

        result = event->eventData.gattData.portConnHandle.auth_result;

        TagFree(event->eventData.gattData.value);
        TagFree(event);

        if (result != TAG_BLE_ERROR_ATT_NO_ERROR)
        {
            return BT_GATT_ERR(result);
        }
    }

    return result;
}

static int auth_svc_attr_read_callback(u16 conn_handle, u16 handle, int tagCharIndex, void * buf, u16 len, u16 offset)
{
    UNUSEDARG(buf)
    UNUSEDARG(len)
    UNUSEDARG(offset)
    TAG_LOG_D("auth_svc_attr_read_callback handle[%d] %d", handle,tagCharIndex);
    if (gTagChar[tagCharIndex].callback_flag & READ_CALLBACK_FLAG)
    {
        BleEvent *event;
        event = (BleEvent *)TagMalloc(sizeof(BleEvent));
        memset(event, '\0', sizeof(BleEvent));

        event->eventType = BleAttributeRead;
        event->eventData.gattData.portConnHandle.conn_idx = conn_handle;
        event->eventData.gattData.portConnHandle.auth_result = TAG_BLE_ERROR_ATT_NO_ERROR;
        event->eventData.gattData.portAttrInfo.handle = handle;
        event->eventData.gattData.portAttrInfo.tagCharIndex = tagCharIndex;
        event->eventData.gattData.charIndex = tagCharIndex;

        TagBleError_t result = TagBleCallback(event);

//        memcpy(my_Attributes[handle].pAttrValue  + offset, event->eventData.gattData.value, event->eventData.gattData.valueLength);
//        my_Attributes[handle].attrLen  = event->eventData.gattData.valueLength;
//        result = event->eventData.gattData.portConnHandle.auth_result;

        TagFree(event);

        if (result)
        {
            return BT_GATT_ERR(result);
        }
    }
    return ATT_SUCCESS;
}

static int auth_svc_attr_write_callback(u16 conn_handle, u16 handle, int tagCharIndex, const void *buf, u16 len, u16 offset)
{
    TAG_LOG_D("auth_svc_attr_write_callback handle[%d] %d %d", handle,tagCharIndex,len);

    if (offset + len > gTagChar[tagCharIndex].maxValueLength)
    {
        return ATT_ERR_INVALID_OFFSET;
    }

    memcpy(my_Attributes[handle].pAttrValue  + offset, buf, len);

    if (gTagChar[tagCharIndex].callback_flag & WRITE_CALLBACK_FLAG)
    {
        BleEvent *event;
        event = TagMalloc(sizeof(BleEvent));
        memset(event, '\0', sizeof(BleEvent));

        event->eventType = BleAttributeWritten;
        event->eventData.gattData.portConnHandle.conn_idx = conn_handle;
        event->eventData.gattData.portAttrInfo.handle = handle;
        event->eventData.gattData.portAttrInfo.tagCharIndex = tagCharIndex;
        event->eventData.gattData.charIndex = tagCharIndex;
        event->eventData.gattData.value = my_Attributes[handle].pAttrValue;
        event->eventData.gattData.valueLength = len;
        event->eventData.gattData.needResponse = true;

        TagBleError_t rt = TagBleCallback(event);
        TagFree(event);
        if(rt != TAG_BLE_ERROR_ATT_NO_ERROR)
            return ATT_ERR_INVALID_PDU;

    }

    return ATT_SUCCESS;
}


static int tag_svc_attr_read_callback(u16 conn_handle, u16 handle, int tagCharIndex, void *buf, u16 len, u16 offset)
{
    UNUSEDARG(buf)
    UNUSEDARG(len)
    UNUSEDARG(offset)
    TAG_LOG_D("tag_svc_attr_read_callback handle[0x%x] %d", handle,tagCharIndex);
    int result = 0;
    if (gTagChar[tagCharIndex].callback_flag & READ_CALLBACK_FLAG)
    {
        BleEvent *event;
        event = (BleEvent *)TagMalloc(sizeof(BleEvent));
        memset(event, '\0', sizeof(BleEvent));

        event->eventType = BleAttributeRead;
        event->eventData.gattData.portConnHandle.conn_idx = conn_handle;
        event->eventData.gattData.portConnHandle.auth_result = TAG_BLE_ERROR_ATT_NO_ERROR;
        event->eventData.gattData.portAttrInfo.handle = handle;
        event->eventData.gattData.portAttrInfo.tagCharIndex = tagCharIndex;
        event->eventData.gattData.charIndex = tagCharIndex;

        TagBleCallback(event);


//        memcpy(my_Attributes[handle].pAttrValue  + offset, event->eventData.gattData.value, event->eventData.gattData.valueLength);
//        my_Attributes[handle].attrLen  = event->eventData.gattData.valueLength;
        result = event->eventData.gattData.portConnHandle.auth_result;

        TagFree(event);

        if (result)
        {
            return BT_GATT_ERR(result);
        }
    }

    return result;
}

static int tag_svc_attr_write_callback(u16 conn_handle, u16 handle, int tagCharIndex, const void *buf, u16 len, u16 offset)
{
    TAG_LOG_D("tag_svc_attr_write_callback handle[%x] %d %d", handle,tagCharIndex,len);
    int result = 0;

    if (offset + len > gTagChar[tagCharIndex].maxValueLength)
    {
        return ATT_ERR_INVALID_OFFSET;
    }

    if ((gTagChar[tagCharIndex].callback_flag & WRITE_CALLBACK_FLAG) || (gTagChar[tagCharIndex].callback_flag & WRITE_WO_RSP_CALLBACK_FLAG))
    {
        BleEvent *event;
        event = TagMalloc(sizeof(BleEvent));
        memset(event, '\0', sizeof(BleEvent));

        event->eventType = BleAttributeWritten;
        event->eventData.gattData.portConnHandle.conn_idx = conn_handle;
        event->eventData.gattData.portAttrInfo.handle = handle;
        event->eventData.gattData.portAttrInfo.tagCharIndex = tagCharIndex;
        event->eventData.gattData.charIndex = tagCharIndex;
        if (gTagChar[tagCharIndex].callback_flag & WRITE_CALLBACK_FLAG)
        {
            event->eventData.gattData.needResponse = true;
        }
        else
        {
            event->eventData.gattData.needResponse = false;
        }

        event->eventData.gattData.value = TagMalloc(len);
        memcpy(event->eventData.gattData.value, buf, len);
        event->eventData.gattData.valueLength = len;

        result = TagBleCallback(event);
//        result = event->eventData.gattData.portConnHandle.auth_result;

        TagFree(event->eventData.gattData.value);
        TagFree(event);
        if (result)
        {
            return  ATT_ERR_INVALID_PDU;
        }
    }

    return result;
}


static void default_ccc_cfg_changed(u16 conn_handle, u16 handle, int tagCharIndex, u16 value)
{
    TAG_LOG_I("default_ccc_cfg_changed handle[%d] %d", handle,tagCharIndex);
    BleEvent *event;
    event = TagMalloc(sizeof(BleEvent));
    memset(event, '\0', sizeof(BleEvent));
    int result = 0;
    event->eventType = BleAttributeWritten;
    event->eventData.gattData.portConnHandle.conn_idx = conn_handle;
    event->eventData.gattData.portAttrInfo.handle = handle;
    event->eventData.gattData.portAttrInfo.tagCharIndex = tagCharIndex;
    event->eventData.gattData.charIndex = tagCharIndex;
    event->eventData.gattData.needResponse = true;
    event->eventData.gattData.value = TagMalloc(2);
    *(u16*)(event->eventData.gattData.value) = value;
    event->eventData.gattData.valueLength = 2;
    TagBleCallback(event);
    result = event->eventData.gattData.portConnHandle.auth_result;
    TagFree(event->eventData.gattData.value);
    TagFree(event);
    if (result)
    {
        TAG_LOG_E("Error result(%d) of %s", result);
    }
}

int portble_writeccc(u16 connHandle, app_ble_rf_packet_att_write_t *p)
{
    u16  att_handle = p->handle;
    int  tagCharIndex = tagHandleTable[att_handle - 1];
    TAG_LOG_D("portble_writeccc [%d] %d %d", att_handle,tagCharIndex,connHandle);
//    connHandle &= 0x0f;
    u16 data = *((u16 *)&p->value);
    memcpy(my_Attributes[att_handle].pAttrValue,(u8 *)&p->value,2);
    default_ccc_cfg_changed(connHandle,att_handle,tagCharIndex,data);
    return 0;
}

int PortBle_writeData(u16 connHandle, app_ble_rf_packet_att_write_t *p)
{
    u16  att_handle = p->handle;
    int  tagCharIndex = tagHandleTable[att_handle];
    TAG_LOG_D("PortBle_writeData[%d] %d %d", att_handle,tagCharIndex,connHandle);
    u8 * buf = &(p->value);
    u16 len = p->l2capLen - 3;
//    connHandle &= 0x0f;
    if(tagCharIndex>=AUTH_CHAR_START && tagCharIndex<= AUTH_CHAR_END)
    {
       return auth_svc_attr_write_callback(connHandle,att_handle,tagCharIndex,buf,len,0);
    }
    else if(tagCharIndex>=CTRL_CHAR_START && tagCharIndex<= CTRL_CHAR_END)
    {
        return tag_svc_attr_write_callback(connHandle,att_handle,tagCharIndex,buf,len,0);
    }
    else if(tagCharIndex>=ONBD_CHAR_START && tagCharIndex<= ONBD_CHAR_END)
    {
        return onboarding_svc_attr_write_callback(connHandle,att_handle,tagCharIndex,buf,len,0);
    }
    return ATT_ERR_INVALID_HANDLE;
}

int PortBle_ReadData(u16 connHandle, app_rf_packet_att_readBlob_t* p)
{
    u16  att_handle = p->handle;
    int  tagCharIndex = tagHandleTable[att_handle];
    u8 * buf = my_Attributes[att_handle].pAttrValue;
    u16 len = gTagChar[tagCharIndex].maxValueLength ;
    TAG_LOG_D("PortBle_ReadData[%d] %d %d", att_handle,tagCharIndex,connHandle);
//    connHandle &= 0x0f;
    if(tagCharIndex>=AUTH_CHAR_START && tagCharIndex<= AUTH_CHAR_END)
    {
        return auth_svc_attr_read_callback(connHandle,att_handle,tagCharIndex,buf,len,0);
    }
    else if(tagCharIndex>=CTRL_CHAR_START && tagCharIndex<= CTRL_CHAR_END)
    {
        return tag_svc_attr_read_callback(connHandle,att_handle,tagCharIndex,buf,len,0);
    }
    else if(tagCharIndex>=ONBD_CHAR_START && tagCharIndex<= ONBD_CHAR_END)
    {
        return onboarding_svc_attr_read_callback(connHandle,att_handle,tagCharIndex,buf,len);
    }
    return ATT_ERR_INVALID_HANDLE;
}


uint16_t find_handle_by_uuid_char(uint8_t *p_uuid, attribute_t *p_att)
{
    for(int i = 0; i < p_att->attNum; i++)
    {
        if(p_att[i].uuidLen == 16 && !memcmp(p_uuid, p_att[i].uuid, 16))
        {
            return i;
        }
    }
    return 0;
}

TagError_t PortBleAddGattDbCharacteristic(ServiceType service_type)
{
    TagError_t ret = TAG_ERROR_NONE;
    const uint8_t *pUuid;
    int i = 0, initNum = 0, maxNum = 0;
    u16 svcHandle = 0;
    u16 * p_svcHandl = NULL;

    switch(service_type)
    {
        case AUTH_SERVICE:
            initNum = AUTH_CHAR_START;
            maxNum = AUTH_CHAR_END;
            p_svcHandl = &(gTagContext->svcHandleAuth);
            break;

        case CONTROL_SERVICE:
            initNum = CTRL_CHAR_START;
            maxNum = CTRL_CHAR_END;
            p_svcHandl = &(gTagContext->svcHandleTag);
            break;

        case ONBOARDING_SERVICE:
            initNum = ONBD_CHAR_START;
            maxNum = ONBD_CHAR_END;
            p_svcHandl = &(gTagContext->svcHandleOnboarding);
            break;

        default:
            return TAG_ERROR_INVALID_ARG;
            break;
    }
    TAG_LOG_I ("PortBleAddGattDbCharacteristic %d [%d]-> %d", svcHandle,initNum,maxNum);
    u8 svc_find_flag = 0;
    for (i = initNum; i < maxNum; i++)
    {
        svcHandle = 0;
        pUuid = gTagChar[i].pUuid;
        svcHandle = find_handle_by_uuid_char(pUuid, g_MyAttributes);
        if (!svcHandle)
        {
            continue;
        }
        if(svc_find_flag == 0)
        {
            *p_svcHandl = svcHandle - 2;
            svc_find_flag = 1;
        }
        tagHandleTable[svcHandle] = i;
        gTagChar[i].handleV = svcHandle;
        if (((gTagChar[i].properties & PROP_INDICATE) == PROP_INDICATE) ||
            ((gTagChar[i].properties & PROP_NOTIFY) == PROP_NOTIFY))
        {
            gTagChar[i].handleCccd = (svcHandle + 1);
        }
        else
        {
             gTagChar[i].handleCccd = 0;
        }
    }
   return ret;
}

TagError_t PortBleRemoveGattDbService(ServiceType service_type)
{
    UNUSEDARG(service_type)
    return TAG_ERROR_NOT_SUPPORTED;
}

TagError_t PortBleRemoveGattDbCharacteristic(ServiceType service_type)
{
    UNUSEDARG(service_type)
    return TAG_ERROR_NOT_SUPPORTED;
}

static int portble_pairing_confirm(u16 connHandle);

int portble_pairing_req_cb(u16 connHandle)
{
//    connHandle &= 0x0f;
    return portble_pairing_confirm(connHandle);
}

void my_gatt_init(void);
TagError_t PortBleInit()
{
    blc_ll_setCustomFMNEnable(1, portble_pairing_req_cb, NULL);
    g_MyAttributes = my_Attributes;
    my_gatt_init();
    TAG_LOG_I("MY_ATT_TABLE_MAX_NUM %d\n", MY_ATT_TABLE_MAX_NUM);
    return TAG_ERROR_NONE;
}


TagError_t PortBleGapRemoveOtherBondings(PortBleConnInfo *connInfo)
{
    TagError_t err_code = TAG_ERROR_NONE;
    if (connInfo == NULL)
    {
        blc_smp_eraseAllBondingInfo();
    }
    return err_code;
}

TagError_t PortBleRequestConnectionParameters(PortBleConnInfo *connInfo, u16 intervalMin, u16 intervalMax, u16 slaveLatency, u16 timeoutMultiplier)
{
    bls_l2cap_requestConnParamUpdate(connInfo->conn_idx , intervalMin, intervalMax, slaveLatency, timeoutMultiplier);
    return TAG_ERROR_NONE;
}

TagError_t PortBleChangeAttrInfoByIndex(PortBleAttrInfo* attrInfo, int characteristicIndex)
{
    if (TAG_CHAR_START <= characteristicIndex && characteristicIndex < TAG_CHAR_END) {
        attrInfo->handle = gTagChar[characteristicIndex].handleV;
        attrInfo->tagCharIndex = characteristicIndex;
        return TAG_ERROR_NONE;
    }
    attrInfo->handle = 1;
    return TAG_ERROR_BLE_GATT_INVALID_CHARACTERISTIC;
}

TagError_t PortBleGapDisconnect(PortBleConnInfo* connInfo)
{
    if(blc_ll_disconnect(connInfo->conn_idx, HCI_ERR_CONN_TERM_BY_LOCAL_HOST) == BLE_SUCCESS)
    return TAG_ERROR_NONE;
    else
    return TAG_ERROR_COMMON_BASE;
}


void PortBle_indicate_handler(BleEvent *event)
{
    TagBleCallback(event);
    TagFree(event);
}

void PortBle_indicate_cb(u16 connHandle, uint8_t err)
{
//    connHandle &= 0x0f;
    TAG_LOG_I("Indication %s\n", err != 0U ? "fail" : "success");
    BleEvent *event;
    event = TagMalloc(sizeof(BleEvent));
    memset(event, '\0', sizeof(BleEvent));
    event->eventType = BleHandleValueConfirmation;
    event->eventData.gattData.portConnHandle.conn_idx  = connHandle;
    event->eventData.gattData.portConnHandle.auth_result = err;

    if(TAG_ERROR_NONE != TagPutPostWork(PortBle_indicate_handler, event))
    {
        TagBleCallback(event);
        TagFree(event);
    }
}


TagError_t PortBleGattSendIndication(PortBleConnInfo *connInfo, PortBleAttrInfo* attrInfo, unsigned char *attrValue, size_t attrValueLen)
{
    TAG_LOG_I("PortBleGattSendIndication");
    ble_sts_t rt = BLE_SUCCESS;
    rt = blc_gatt_pushHandleValueIndicate (connInfo->conn_idx, attrInfo->handle, attrValue, attrValueLen);
    if(GATT_ERR_DATA_PENDING_DUE_TO_SERVICE_DISCOVERY_BUSY == rt )
    {
          blc_gap_setSingleServerDataPendingTime_upon_ClientCmd(connInfo->conn_idx,0);
          rt = blc_gatt_pushHandleValueIndicate (connInfo->conn_idx, attrInfo->handle, attrValue, attrValueLen);
    }
    if(rt == BLE_SUCCESS)
    return TAG_ERROR_NONE;
    else
    {
        TAG_LOG_E("PortBleGattSendIndication %d %d 0x%x ",connInfo->conn_idx, attrInfo->handle,rt);
        return TAG_ERROR_NV_INVALID_PARAM;
    }
}

TagError_t PortBleGattSendNotification(PortBleConnInfo *connInfo, PortBleAttrInfo* attrInfo, unsigned char *attrValue, size_t attrValueLen)
{
    ble_sts_t rt = BLE_SUCCESS;
    TAG_LOG_I("PortBleGattSendNotification");
    rt = blc_gatt_pushHandleValueNotify (connInfo->conn_idx, attrInfo->handle, attrValue, attrValueLen);
    if(rt == BLE_SUCCESS)
    return TAG_ERROR_NONE;
    else
    {
        TAG_LOG_E("PortBleGattSendNotification %d %d 0x%x ",connInfo->conn_idx, attrInfo->handle,rt);
        return TAG_ERROR_NV_INVALID_PARAM;
    }
}

TagError_t PortBleGattsSendAttrReadStatus(BleGattData *gattData, TagBleError_t status)
{
    u16  att_handle = gattData->portAttrInfo.handle;
    gattData->portConnHandle.auth_result = status;
    memcpy(my_Attributes[att_handle].pAttrValue, gattData->value, gattData->valueLength);
//    my_Attributes[att_handle].attrLen  = gattData->valueLength;
    att_custom_read_rsp_len = gattData->valueLength;
    return TAG_ERROR_NONE;
}

TagError_t PortBleGattsSendAttrWrittenStatus(BleGattData *gattData, TagBleError_t status)
{
    gattData->portConnHandle.auth_result = status;
    return TAG_ERROR_NONE;

}

TagError_t PortBleCopyConnHandle(PortBleConnInfo* dest, PortBleConnInfo* src)
{
    if (!dest || !src)
    {
        return TAG_ERROR_INVALID_ARG;
    }
    dest->conn_idx = src->conn_idx;
    return TAG_ERROR_NONE;
}

TagError_t PortBleDestroyConnHandle(PortBleConnInfo* connInfo)
{
    UNUSEDARG(connInfo)
    return TAG_ERROR_NONE;
}

bool PortBleIsEqualConnHandle(PortBleConnInfo* handle1, PortBleConnInfo* handle2)
{
    if (!handle1 || !handle2)
    {
        return false;
    }

    if (handle1->conn_idx == handle2->conn_idx)
    {
        return true;
    }

    return false;
}

/**
 * @brief   BLE Advertising data
 */
//u8 tbl_advData[31 +1] = {0};

/**
 * @brief   BLE Scan Response Packet data
 */
//u8 tbl_scanRsp[31 +1] = {0};

u8 tbl_advData[] = {
   17,
   DT_COMPLETE_LOCAL_NAME,
   'p',
   'p',
   'l',
   't',
   'i',
   '_',
   'c',
   'o',
   'n',
   'n',
   'n',
   'n',
   'n',
   'n',
   'n',
   '6',
   2,
   DT_FLAGS,
   0x05, // BLE limited discoverable mode and BR/EDR not supported
   3,
   DT_APPEARANCE,
   0x80,
   0x01, // 384, Generic Remote Control, Generic category
   5,
   DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID,
   0x12,
   0x18,
   0x0F,
   0x18, // incomplete list of service class UUIDs (0x1812, 0x180F)
};
//
///**
//* @brief   BLE Scan Response Packet data
//*/
const u8 tbl_scanRsp[] = {
           2,
           DT_FLAGS,
           0x04, // BLE limited discoverable mode and BR/EDR not supported
};

TagError_t PortBleStopAdv()
{
    blc_ll_setAdvEnable(BLC_ADV_DISABLE);
    return TAG_ERROR_NONE;
}
static  u8 local_irk[16] = {0x1,0x2,0x3};
_attribute_ble_data_retention_ static u8 adv_re_open_flag = 0;
_attribute_ble_data_retention_ static PortBleAdvParams pre_params;

TagError_t PortBleStartAdv(PortBleAdvData *advData, PortBleAdvParams *params)
{
        TAG_LOG_I("adv start %d %d",params->maxInterval,params->minInterval);
        u32 offset = 0;
        bool needAdvParamUpdated = 0;
        adv_inter_t intervalMin;
        adv_inter_t intervalMax;
        adv_type_t advType;
        own_addr_type_t ownAddrType;
        ble_sts_t rt =     BLE_SUCCESS;
        u8 adv_len = 0;
        u8 sd_len = 0;
//        tlkapi_send_string_data(APP_LOG_EN, "adv data ", tbl_advData, 31);
        memset(tbl_advData, '\0', sizeof(tbl_advData));
        if(NULL == advData)
            return TAG_ERROR_INVALID_ARG;
        tbl_advData[offset++] = TAG_ADVERTISING_FLAG_STRUCT_LENGTH + 1;
        tbl_advData[offset++] = DT_FLAGS;
        //memcpy(tbl_advData + offset, advData->aAdStructures[0].aData, TAG_ADVERTISING_FLAG_STRUCT_LENGTH);
        tbl_advData[offset] = 0x4;
        offset += TAG_ADVERTISING_FLAG_STRUCT_LENGTH;

        tbl_advData[offset++] = TAG_ADVERTISING_SERVICE_UUID_STRUCT_LENGTH + 1;
        tbl_advData[offset++] = DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID;
        memcpy(tbl_advData + offset, advData->aAdStructures[1].aData, TAG_ADVERTISING_SERVICE_UUID_STRUCT_LENGTH);
        offset += TAG_ADVERTISING_SERVICE_UUID_STRUCT_LENGTH;

        tbl_advData[offset++] = TAG_ADVERTISING_SERVICE_DATA_STRUCT_LENGTH + 1;
        tbl_advData[offset++] = DT_SERVICE_DATA;
        memcpy(tbl_advData + offset, advData->aAdStructures[2].aData, TAG_ADVERTISING_SERVICE_DATA_STRUCT_LENGTH);

        offset += TAG_ADVERTISING_SERVICE_DATA_STRUCT_LENGTH;

        adv_len = TAG_ADVERTISING_SERVICE_DATA_STRUCT_LENGTH + TAG_ADVERTISING_SERVICE_UUID_STRUCT_LENGTH +TAG_ADVERTISING_FLAG_STRUCT_LENGTH +6;
        sd_len = sizeof(tbl_scanRsp);


        needAdvParamUpdated = (pre_params.advertisingType != params->advertisingType) ||
                          (pre_params.minInterval != params->minInterval) ||
                          (pre_params.ownAddressType != params->ownAddressType);

        intervalMin = params->minInterval;
        intervalMax = params->maxInterval;
//        intervalMax = 3152;   //todo!!!!!!!!!!!!
//        intervalMin = 3152;
        advType = (adv_type_t)params->advertisingType;
        ownAddrType = (own_addr_type_t)params->ownAddressType;
        TAG_LOG_I("adv param %d %d %d %d %d %d %d",intervalMin,intervalMax,advType,ownAddrType,adv_len,sd_len,params->needAddrChange);
//
        tlkapi_send_string_data(APP_LOG_EN, "adv data ", tbl_advData, adv_len);
        tlkapi_send_string_data(APP_LOG_EN, "rsp data", tbl_scanRsp, sd_len);


        if (needAdvParamUpdated || params->needAddrChange)
        {
            if(adv_re_open_flag)
            rt =   blc_ll_setAdvEnable(BLC_ADV_DISABLE);
            adv_re_open_flag = 1;
            if(rt != BLE_SUCCESS)
            {
                TAG_LOG_E("BLC_ADV_DISABLE fail %02X",rt);
            }
            if(params->needAddrChange)
            {
                u8 mac_random_static[6] = {0x1,0x2,0x33,0x57,0x55,0x40};
                generateRandomNum(5, mac_random_static);
                mac_random_static[5] = 0x40;
                blc_ll_setRandomAddr(mac_random_static);
            }
            if (pre_params.ownAddressType != params->ownAddressType)
            {
                switch (params->ownAddressType)
                {
                case PortBleAdvAddrTypePublic:
                    blc_ll_setAddressResolutionEnable(0);
                    break;
                case PortBleAdvAddrTypeRandom:
                    break;
                }
            }
            rt = BLE_SUCCESS;
            rt= blc_ll_setAdvData(tbl_advData, adv_len);
            if(rt != BLE_SUCCESS)
            {
                TAG_LOG_E("blc_ll_setAdvData fail %02X",rt);
                return TAG_ERROR_INVALID_ARG;
            }
            rt = blc_ll_setScanRspData(tbl_scanRsp, 0);
            if(rt != BLE_SUCCESS)
            {
                TAG_LOG_E("blc_ll_setScanRspData fail %02X",rt);
                return TAG_ERROR_INVALID_ARG;
            }
            rt =  blc_ll_setAdvParam(intervalMin, intervalMax, advType, ownAddrType, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
            if(rt != BLE_SUCCESS)
            {
                TAG_LOG_E("blc_ll_setAdvParam fail %02X",rt);
                return TAG_ERROR_INVALID_ARG;
            }
            rt =  blc_ll_setAdvEnable(BLC_ADV_ENABLE); //ADV enable
            if(rt != BLE_SUCCESS)
            {
                TAG_LOG_E("blc_ll_setAdvEnable fail %02X",rt);
                return TAG_ERROR_INVALID_ARG;
            }

        } else
        {
            rt = BLE_SUCCESS;
            rt =  blc_ll_setAdvData(tbl_advData, adv_len);
            if(rt != BLE_SUCCESS)
            {
                TAG_LOG_E("blc_ll_setAdvData fail");
                return TAG_ERROR_INVALID_ARG;
            }
            rt =  blc_ll_setScanRspData(tbl_scanRsp, sd_len);
            if(rt != BLE_SUCCESS)
            {
                TAG_LOG_E("blc_ll_setScanRspData fail");
                return TAG_ERROR_INVALID_ARG;
            }
        }

        pre_params.advertisingType = params->advertisingType;
        pre_params.minInterval = params->minInterval;
        pre_params.ownAddressType = params->ownAddressType;

        return TAG_ERROR_NONE;

}

TagError_t PortBleGetMtu(PortBleConnInfo *connInfo, u16 *outMtu)
{
    UNUSEDARG(connInfo)
    if (outMtu == NULL)
    {
        return TAG_ERROR_INVALID_ARG;
    }
    *outMtu = TAG_MAX_MTU;
    return TAG_ERROR_NONE;
}

_attribute_data_retention_ s8 adv_tx_power = RF_POWER_P3dBm;
TagError_t PortBleSetTxPower(PortBleTxPowerScenarioType type, s8 txPower)
{
    if (type == PortBleTxPowerBootingAdvWeak)
    {
        adv_tx_power = RF_POWER_P0dBm;
    }
    else if (type == PortBleTxPowerBootingAdvStrong)
    {
        adv_tx_power = RF_POWER_P3dBm;
    }
    else if (type == PortBleTxPowerOnboardedAdvStrong)
    {
        adv_tx_power = RF_POWER_P3dBm;
    }
    else if (type == PortBleTxPowerManual)
    {
        adv_tx_power = txPower;
    }
    else
    {
        TAG_LOG_E("Not supported type(%u)", type);
        return TAG_ERROR_INVALID_ARG;
    }
     rf_set_power_level_index(adv_tx_power);
    return TAG_ERROR_NONE;
}

void test_flag(void)
{
 unsigned long read_addr = 0x40abc;
  u8 flash_buf[32];
  memset(flash_buf,0,32);
  flash_read_page(read_addr, 16, flash_buf);
  TAG_LOG_I("flash: %x,%x %x",flash_buf[0],flash_buf[1],flash_buf[2]);

}


TagError_t  PortBleGapBondingReply(PortBleConnInfo *connInfo, bool reply)
{

    TagError_t err_code;
    if(reply)
    {
        connect_pair_flag[connInfo->conn_idx & 0xf] = 0;
        return TAG_ERROR_NONE;
    }
    else
    {
        connect_pair_flag[connInfo->conn_idx &0xf] = 1;
        return TAG_ERROR_NOT_SUPPORTED;
    }
    return err_code;
}


_attribute_data_retention_ u8 ble_connection_count = 0;
// connection management
void portble_tag_connected(u16 connHandle,u8 err)
{
    UNUSEDARG(err)
    BleEvent *event;
//    connHandle &= 0x0f;
    default_conn = connHandle;
    ble_connection_count++;

    event = TagMalloc(sizeof(BleEvent));
    memset(event, '\0', sizeof(BleEvent));
    event->eventType = BleConnected;
    event->eventData.connectionData.portConnHandle.conn_idx = connHandle;
    if((connHandle & 0x0f) >= ACL_PERIPHR_MAX_NUM)
    {
          TAG_LOG_I("connHandle %d >=  ACL_PERIPHR_MAX_NUM \n", connHandle);
    }
    else
    connect_pair_flag[connHandle&0x0f] = 0;

    TagBleCallback(event);
    TagFree(event);
}

void portble_tag_disconnected(u16 connHandle, u8 reason)
{
    UNUSEDARG(reason)
    BleEvent *event;
//    connHandle &= 0x0f;
    connect_pair_flag[connHandle &0xf] = 0;
    ble_connection_count--;
    event = TagMalloc(sizeof(BleEvent));
    memset(event, '\0', sizeof(BleEvent));
    event->eventType = BleDisconnected;
    event->eventData.disconnectionData.portConnHandle.conn_idx = connHandle;

    TagBleCallback(event);
    TagFree(event);
}

void portble_le_param_updated(u16 connHandle, u16 interval, u16 latency, u16 timeout)
{
    BleEvent *event;
    event = TagMalloc(sizeof(BleEvent));
    memset(event, '\0',sizeof(BleEvent));
    event->eventType = BleConnectionParameterUpdated;
    event->eventData.paramsData.portConnHandle.conn_idx = connHandle;
    event->eventData.paramsData.connInterval = interval;
    event->eventData.paramsData.connLatency = latency;
    event->eventData.paramsData.supervisionTimeout = timeout;

    TagBleCallback(event);
    TagFree(event);
}


static int portble_pairing_confirm(u16 connHandle)
{
    BleEvent *event;
    event = TagMalloc(sizeof(BleEvent));
    memset(event, '\0', sizeof(BleEvent));
    event->eventType = BleBondingStatus;
    event->eventData.bondingStatusData.portConnHandle.conn_idx = connHandle;
    event->eventData.bondingStatusData.bondingStatus = BONDING_STATUS_REQUEST;

    TagBleError_t rt =  TagBleCallback(event);
    TagFree(event);
    if(rt  != TAG_BLE_ERROR_ATT_NO_ERROR || connect_pair_flag[connHandle&0xf])
        return PAIRING_FAIL_REASON_PAIRING_NOT_SUPPORTED;
    else
        return 0;

    TAG_LOG_I("Pairing confirmation required for %d\n", connHandle);
}

void portble_pairing_complete(u16 connHandle, bool bonded)
{
    BleEvent *event;
//    connHandle &= 0x0f;
    event = TagMalloc(sizeof(BleEvent));
    memset(event, '\0', sizeof(BleEvent));
    event->eventType = BleBondingStatus;
    event->eventData.bondingStatusData.portConnHandle.conn_idx = connHandle;
    if (bonded)
    {
        event->eventData.bondingStatusData.bondingStatus = BONDING_STATUS_SUCCESS_BOND;

    }
    else
    {
        event->eventData.bondingStatusData.bondingStatus = BONDING_STATUS_FAIL_BOND;
    }

    TagBleCallback(event);
    TagFree(event);

    TAG_LOG_I("Pairing completed: %d, bonded: %d\n", connHandle, bonded);
}

void portble_pairing_failed(u16 connHandle, int reason)
{
    BleEvent *event;
//    connHandle &= 0x0f;
    event = TagMalloc(sizeof(BleEvent));
    memset(event, '\0', sizeof(BleEvent));
    event->eventType = BleBondingStatus;
    event->eventData.bondingStatusData.portConnHandle.conn_idx = connHandle;
    event->eventData.bondingStatusData.bondingStatus = BONDING_STATUS_FAIL_BOND;

    TagBleCallback(event);
    TagFree(event);

    TAG_LOG_I("Pairing failed conn: %d, reason %d\n", connHandle, reason);
}

TagError_t PortEncryptionInit(void);
void portble_deepRtn_init(void)
{
    PortEncryptionInit();
}
