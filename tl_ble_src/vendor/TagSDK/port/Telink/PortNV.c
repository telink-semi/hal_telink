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
#if 1
/* TagSDK related headers */
#include "TagConfig.h"

#include "TagErrorType.h"
#include "TagDebug.h"
#include "TagNV.h"
/* TagSDK related headers */
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "PortNV.h"
#include "app_nv.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

#define TAG_NV_AGING_CNT_ID     (TAG_NV_VENDOR_ITEM_LAST + 1)

#define TAG_NV_FLASH_OFFSET CFG_ADR_TAG_2M_FLASH

typedef struct {
    unsigned short int data_id;
} PortNvsNVParam_t;

#define TAG_NV_RO_OFFSET (TAG_NV_FLASH_OFFSET + 0)
#define TAG_NV_RO_SIZE 1024

#define TAG_NV_RW_OFFSET (TAG_NV_RO_OFFSET+TAG_NV_RO_SIZE)
#define TAG_NV_RW_SIZE 2048

#define TAG_NV_VERDOR_OFFSET (TAG_NV_RW_OFFSET + TAG_NV_RW_SIZE)
#define TAG_NV_VERDOR_SIZE 1024

#define RAM_NV_RW_ITEM_LAST (TAG_NV_RW_ITEM_LAST)
#define RAM_NV_RO_ITEM_LAST (TAG_NV_RO_ITEM_LAST)

#define NV_MAX_BUF_SIZE 1100


#define PORTNV_MALLOC(x)  TagMalloc(x)
#define PORTNV_FREE(x)  TagFree(x)
typedef struct
{
    size_t maxSize;  /* Actual data Max size, not include HEAD_INFO */
    uint32_t offset; // TODO:FIX
} PortNVDataInfo_t;
#define PORT_NV_SIZE_PAD 4
//static PortNVDataInfo_t portNVDataRW[TAG_NV_RW_ITEM_CNT_MAX(RAM_NV_RW_ITEM_LAST)] = {
//#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_BUTTON_PUSH_ACTION)] = {
//        .maxSize = 1,
//        .offset = 0,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_BUTTON_HOLD_ACTION)] = {
//        .maxSize = 1,
//        .offset = 8,
//    },
//#endif // TAG_ACCESSORY_OPTION_BUTTON_ACTION
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_E2E_ENCRYPTION)] = {
//        .maxSize = 1,
//        .offset = 16,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_E2E_ENCRYPTION_KEY_AES)] = {
//        .maxSize = 100,
//        .offset = 24,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_PREMATURE_OFFLINE_TIMEOUT)] = {
//        .maxSize = 2,
//        .offset = 132,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_OVERMATURE_OFFLINE_TIMEOUT)] = {
//        .maxSize = 4,
//        .offset = 140,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_RINGTONE_NAME)] = {
//        .maxSize = 61,
//        .offset = 152,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_RINGTONE_DATA)] = {
//        .maxSize = 1100,
//        .offset = 220,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_MAX_ALLOWED_BLE_CONNECTION)] = {
//        .maxSize = 1,
//        .offset = 1328,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_MASTER_SECRET_AES)] = {
//        .maxSize = 32,
//        .offset = 1336,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_SOUND_VOLUME)] = {
//        .maxSize = 1,
//        .offset = 1376,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_REGION)] = {
//        .maxSize = 1,
//        .offset = 1384,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_ONBOARDED)] = {
//        .maxSize = 1,
//        .offset = 1392,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_OTA_MODE_ENABLED)] = {
//        .maxSize = 1,
//        .offset = 1400,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_PRIVACY_ID_SEED)] = {
//        .maxSize = 8,
//        .offset = 1408,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_PRIVACY_ID_IV)] = {
//        .maxSize = 16,
//        .offset = 1424,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_BOOT_REASON)] = {
//        .maxSize = 1,
//        .offset = 1448,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_SAVED_RTC)] = {
//        .maxSize = 8,
//        .offset = 1456,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_BLE_IRK)] = {
//        .maxSize = 16,
//        .offset = 1472,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_RINGTONE_DATA_SIZE)] = {
//        .maxSize = 2,
//        .offset = 1496,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_NUMBER_OF_PRIVACY_ID)] = {
//        .maxSize = 4,
//        .offset = 1504,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_ACTIVITY_MODE)] = {
//        .maxSize = 1,
//        .offset = 1516,
//    },
//#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_UWB_GROUP_DELAY_CALIBRATED)] = {
//        .maxSize = 1,
//        .offset = 1524,
//    },
//#endif
//#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_FLASH_LOGGING_BLOCK_POS)] = {
//        .maxSize = 1,
//        .offset = 1532,
//    },
//#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_BATTERY_LEVEL)] = {
//        .maxSize = 1,
//        .offset = 1540,
//    },
//    [TAG_NV_RW_ITEM_IDX(TAG_NV_TX_POWER)] = {
//        .maxSize = 1,
//        .offset = 1548,
//    },
//};
//
//
static const  PortNVDataInfo_t portNVDataRO[TAG_NV_RO_ITEM_CNT_MAX(RAM_NV_RO_ITEM_LAST)] = {

    [TAG_NV_RO_ITEM_IDX(TAG_NV_MODEL_NAME)] = {
        .maxSize = TAG_NV_MODEL_NAME_MAX_SZ,
        .offset = 0,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_VENDOR_ID)] = {
        .maxSize = TAG_NV_VENDOR_ID_MAX_SZ,
        .offset = TAG_NV_MODEL_NAME_MAX_SZ + 4,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_MANUFACTURER_ID)] = {
        .maxSize = TAG_NV_MANUFACTURER_ID_MAX_SZ,
        .offset = TAG_NV_MODEL_NAME_MAX_SZ+TAG_NV_VENDOR_ID_MAX_SZ + TAG_NV_RO_ITEM_IDX(TAG_NV_MANUFACTURER_ID)*4,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_MANUFACTURER_NAME)] = {
        .maxSize = TAG_NV_MANUFACTURER_NAME_MAX_SZ,
        .offset = TAG_NV_MODEL_NAME_MAX_SZ+TAG_NV_VENDOR_ID_MAX_SZ+TAG_NV_MANUFACTURER_ID_MAX_SZ + TAG_NV_RO_ITEM_IDX(TAG_NV_MANUFACTURER_NAME)*4,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_SETUP_ID)] = {
        .maxSize = TAG_NV_SETUP_ID_MAX_SZ,
        .offset =  TAG_NV_MODEL_NAME_MAX_SZ+TAG_NV_VENDOR_ID_MAX_SZ+TAG_NV_MANUFACTURER_ID_MAX_SZ + TAG_NV_MANUFACTURER_NAME_MAX_SZ + TAG_NV_RO_ITEM_IDX(TAG_NV_SETUP_ID)*4,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_SERIAL_NUMBER)] = {
        .maxSize = TAG_NV_SERIAL_NUMBER_MAX_SZ,
        .offset = TAG_NV_MODEL_NAME_MAX_SZ+TAG_NV_VENDOR_ID_MAX_SZ+TAG_NV_MANUFACTURER_ID_MAX_SZ + TAG_NV_MANUFACTURER_NAME_MAX_SZ +TAG_NV_SETUP_ID_MAX_SZ+ TAG_NV_RO_ITEM_IDX(TAG_NV_SERIAL_NUMBER)*4,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_PRIVATE_KEY_CURVED)] = {
        .maxSize = TAG_NV_PRIVATE_KEY_CURVED_MAX_SZ,
        .offset = TAG_NV_MODEL_NAME_MAX_SZ+TAG_NV_VENDOR_ID_MAX_SZ+TAG_NV_MANUFACTURER_ID_MAX_SZ + TAG_NV_MANUFACTURER_NAME_MAX_SZ +TAG_NV_SETUP_ID_MAX_SZ +TAG_NV_SERIAL_NUMBER_MAX_SZ+ TAG_NV_RO_ITEM_IDX(TAG_NV_PRIVATE_KEY_CURVED)*4,
    },

};


TagError_t PortNVInit(PortNVSupportedInfo_t *nvSupportedInfo)
{
    if (nvSupportedInfo == NULL)
    {
        TAG_LOG_E("Failed to check nvSupportedInfo arg");
        return TAG_ERROR_NV_INVALID_DATA;
    }

    void app_tag_stoage_init(void);
    app_tag_stoage_init();
    /*
     * This NV port-layer sample codes can support RW items only
     * It supports RW items up to TAG_NV_RW_ITEM_LAST
     */
    nvSupportedInfo->lastSupportedRW = TAG_NV_RW_ITEM_LAST;
#if defined(TAG_CONFIG_USE_DEVICE_INFO_HEADER)
    nvSupportedInfo->lastSupportedRO = TAG_NV_INVALID_ITEM;
#else
    nvSupportedInfo->lastSupportedRO = TAG_NV_RO_ITEM_LAST;
#endif
    nvSupportedInfo->lastSupportedVendor = TAG_NV_INVALID_ITEM;

    return TAG_ERROR_NONE;
}

TagError_t PortNVDeinit(void)
{
    return TAG_ERROR_NONE;
}

static PortNVHandle_t* PortNVAcquireParam(TagNVItem_t item)
{
    PortNvsNVParam_t *data;

    data = (PortNvsNVParam_t*)TagMalloc(sizeof(PortNvsNVParam_t));
//    irq_restore(r);
    if (NULL == data)
    {
        TAG_LOG_E("Malloc failed");
        return NULL;
    }
    data->data_id = item;
    return (PortNVHandle_t*)data;
}

PortNVHandle_t* PortNVOpen(TagNVItem_t item, PortNVOpenMode_t mode)
{
    PortNVHandle_t *handle;

    if ((item > TAG_NV_VENDOR_ITEM_LAST) || (mode == PORT_NV_EXIST))
    {
        /* Input arg's mode mis-match error */
        /* TAG_ERROR_NV_NOT_SUPPORTED */
        return NULL;
    }

    if ((item >= TAG_NV_RO_ITEM_FIRST) && (item <= TAG_NV_RO_ITEM_LAST)
        && (PORT_NV_READWRITE == mode))
    {
        TAG_LOG_E("Wrong mode");
        return NULL;
    }

    handle = PortNVAcquireParam(item);
    if (handle == NULL)
    {
        TAG_LOG_E("Malloc failed!");
        return NULL;
    }
    return handle;
}

void PortNVClose(PortNVHandle_t *handle)
{
    if (NULL == handle)
    {
        TAG_LOG_E("Error handle");
        return;
    }
    PORTNV_FREE(handle);
    return;
}

TagError_t tag_flash_read( TagNVItem_t id,void *dataBuf, size_t bufSz, size_t *readSz)
{
    TagError_t rt = TAG_ERROR_NONE;
    int idx = id;
    if ((id >= TAG_NV_RO_ITEM_FIRST) && (id <= TAG_NV_RO_ITEM_LAST))
    {
        if(portNVDataRO [TAG_NV_RO_ITEM_IDX(id)].maxSize >bufSz )
        {
            *readSz = 0;
            rt = TAG_ERROR_NV_INVALID_PARAM;
        }
        else
        {
            size_t data_len = 0;
            flash_read_page(TAG_NV_RO_OFFSET  + portNVDataRO [TAG_NV_RO_ITEM_IDX(id)].offset, PORT_NV_SIZE_PAD,(u8*)&data_len);

            flash_read_page(TAG_NV_RO_OFFSET  + portNVDataRO [TAG_NV_RO_ITEM_IDX(id)].offset + PORT_NV_SIZE_PAD,portNVDataRO [TAG_NV_RO_ITEM_IDX(id)].maxSize ,(u8*)dataBuf);
            *readSz = data_len;
        }
    }
    else if ((id >= TAG_NV_RW_ITEM_FIRST) && (id <= TAG_NV_RW_ITEM_LAST))
    {
        *readSz = app_tag_stoage_get_data(idx,dataBuf,bufSz);
    }
    else if ((id >= TAG_NV_VENDOR_ITEM_FIRST) && (id <= TAG_NV_VENDOR_ITEM_LAST))
    {
        *readSz = app_tag_stoage_get_data(idx,dataBuf,bufSz);
    }
    else
    {
        rt = TAG_ERROR_NONE;
    }
    if(*readSz == 0)
        return TAG_ERROR_NV_NOT_EXIST;
    return rt;
}

TagError_t PortNVRead(PortNVHandle_t *handle, void *dataBuf, size_t bufSz, size_t *readSz)
{
//    TagError_t rt = TAG_ERROR_NONE;
    int rc = 0;
    unsigned short int id = ((PortNvsNVParam_t *)handle)->data_id;
    if (NULL == handle || NULL == dataBuf || NULL == readSz || 0 == bufSz)
    {
        TAG_LOG_E("Invalid input parameters!");
        return TAG_ERROR_NV_INVALID_PARAM;
    }
    TAG_LOG_D("PortNVRead %d %d",id,bufSz);
    switch (id)
    {
#if !defined(TAG_CONFIG_USE_DEVICE_INFO_HEADER)
        case TAG_NV_SERIAL_NUMBER:
        case TAG_NV_PRIVATE_KEY_CURVED:
        {
//            TAG_LOG_E("Please create new storage and save this item for commercial");
//            TAG_LOG_E("And then you need to add code here in order to read this item!!");
//            rt = TAG_ERROR_NV_OPERATION_FAIL;
            rc = tag_flash_read(id, dataBuf, bufSz,readSz);
        }
        break;
#endif
        default:
        {
            rc = tag_flash_read(id, dataBuf, bufSz,readSz);
            if (rc < 0)
            {
                if (rc == TAG_ERROR_NV_NOT_EXIST)
                {
                    TAG_LOG_D("File is not exist!");
//                    rt = TAG_ERROR_NV_NOT_EXIST;
                }
                TAG_LOG_E("Reading operation is failed!ERROR NUM: -%d", rc);
//                rt = TAG_ERROR_NV_OPERATION_FAIL;
            }
        }
        break;
    }
    TAG_LOG_I("PortNVRead %d %d",id,*readSz);
    tlkapi_send_string_data(APP_LOG_EN, "read ", dataBuf, *readSz);
    return TAG_ERROR_NONE;
}
static u8 write_buf[1120];
TagError_t PortNVWrite(PortNVHandle_t *handle, const void *data, size_t dataSz, size_t *writtenSz)
{
    int id = ((PortNvsNVParam_t *)handle)->data_id;
    if (NULL == handle || NULL == data || NULL == writtenSz || 0 == dataSz)
    {
        TAG_LOG_E("Invalid input parameters!");
        return TAG_ERROR_NV_INVALID_PARAM;
    }
    smemcpy(write_buf,data,dataSz);
    TAG_LOG_I("PortNVWrite %d %d",id,dataSz);
    tlkapi_send_string_data(APP_LOG_EN, "write ", data, dataSz);
    int rt =  app_tag_stoage_set_data(id, write_buf, dataSz);
    if (rt != 0)
    {
        TAG_LOG_E("Operation is failed! Error code is -%d %d", *writtenSz,rt);
        return TAG_ERROR_NV_OPERATION_FAIL;
    }
    *writtenSz = dataSz;
    return TAG_ERROR_NONE;
}

TagError_t PortNVRemoveAll(void)
{
    TAG_LOG_D("PortNVRemoveAll !");
    for(int id = TAG_NV_RW_ITEM_FIRST;id < TAG_NV_RW_ITEM_LAST;id++)
    {
        app_tag_stoage_del_data(id);
    }

    for(int id = TAG_NV_VENDOR_ITEM_FIRST;id < TAG_NV_VENDOR_ITEM_LAST;id++)
    {
        app_tag_stoage_del_data(id);
    }
    return 0;
}


TagError_t PortNVRemove(TagNVItem_t item)
{
    TAG_LOG_D("PortNVRemove %d !",item);
    int idx = item;
    if (item > TAG_NV_VENDOR_ITEM_LAST)
    {
        TAG_LOG_E("Error item!");
        return TAG_ERROR_NV_INVALID_PARAM;
    }

    if (item == TAG_NV_RW_ITEM_ALL)
    {
        TAG_LOG_D("Remove all RW items");
        if (0 != PortNVRemoveAll())
        {
            return TAG_ERROR_NV_OPERATION_FAIL;
        }
        return TAG_ERROR_NONE;
    }

    if(0 != app_tag_stoage_del_data(idx))
    {
        return TAG_ERROR_NV_OPERATION_FAIL;
    }

    return TAG_ERROR_NONE;
}
static u8 temp_dataBuf[NV_MAX_BUF_SIZE];
TagError_t PortNVAccess(TagNVItem_t item, PortNVOpenMode_t mode)
{
    size_t readSz = 0;

    size_t bufSz = NV_MAX_BUF_SIZE;
    TAG_LOG_D("PortNVAccess %d %d!",item,mode);
    if (mode != PORT_NV_EXIST)
    {
        return TAG_ERROR_NV_NOT_SUPPORTED;
    }

    if (item >= TAG_NV_VENDOR_ITEM_LAST)
    {
        return TAG_ERROR_NV_NOT_SUPPORTED;
    }

    tag_flash_read(item, temp_dataBuf, bufSz,&readSz);
    if (readSz <= 0)
    {
        if (readSz == 0)
        {
            TAG_LOG_D("File is not exist %d!",item);
            return TAG_ERROR_NV_NOT_EXIST;
        }
        TAG_LOG_E("Reading operation is failed!");
        return TAG_ERROR_NV_OPERATION_FAIL;
    }

    return TAG_ERROR_NONE;
}

TagError_t PortNVSetAgingCnt(unsigned int count)
{
    int rc;
    int idx = TAG_NV_AGING_CNT_ID;
    unsigned int save_count = count;
    rc = app_tag_stoage_set_data(idx, &save_count, sizeof(unsigned int));
    if (rc != 0)
    {
        return TAG_ERROR_NV_OPERATION_FAIL;
    }
    return TAG_ERROR_NONE;
}

TagError_t PortNVGetAgingCnt(unsigned int *count)
{
    TagError_t rc;
    size_t readSz = 0;
    int idx = TAG_NV_AGING_CNT_ID;
    rc = tag_flash_read( idx, count, sizeof(unsigned int),&readSz);
    if (rc < 0 )
    {
        if(TAG_ERROR_NV_NOT_EXIST == rc)
        {
            return TAG_ERROR_NV_NOT_EXIST;
        }

        return TAG_ERROR_NV_OPERATION_FAIL;
    }
    else
        if(readSz == 0)
        {
            return TAG_ERROR_NV_NOT_EXIST;
        }
    return TAG_ERROR_NONE;
}

#endif
