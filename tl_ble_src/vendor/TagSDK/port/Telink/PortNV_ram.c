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

#if 0
#include "TagConfig.h"

#include "TagErrorType.h"
#include "TagDebug.h"
/* TagSDK related headers */
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "PortNV.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

#define RAM_NV_RW_ITEM_LAST (TAG_NV_RW_ITEM_LAST)

typedef struct
{
    uint8_t region; // TODO_FIX
    TagNVItem_t item;

    size_t maxSize;  /* Actual data Max size, not include HEAD_INFO */
    u32 offset; // TODO:FIX
} PortNVDataInfo_t;

typedef struct __attribute__((packed))
{
    uint16_t valCRC;
    uint16_t valSize;
} PortNVDataHead_packed_t;

static PortNVDataInfo_t portNVDataRW[TAG_NV_RW_ITEM_CNT_MAX(RAM_NV_RW_ITEM_LAST)] = {
#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
    [TAG_NV_RW_ITEM_IDX(TAG_NV_BUTTON_PUSH_ACTION)] = {
        .maxSize = 1,
        .offset = 0,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_BUTTON_HOLD_ACTION)] = {
        .maxSize = 1,
        .offset = 8,
    },
#endif // TAG_ACCESSORY_OPTION_BUTTON_ACTION
    [TAG_NV_RW_ITEM_IDX(TAG_NV_E2E_ENCRYPTION)] = {
        .maxSize = 1,
        .offset = 16,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_E2E_ENCRYPTION_KEY_AES)] = {
        .maxSize = 100,
        .offset = 24,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_PREMATURE_OFFLINE_TIMEOUT)] = {
        .maxSize = 2,
        .offset = 132,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_OVERMATURE_OFFLINE_TIMEOUT)] = {
        .maxSize = 4,
        .offset = 140,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_RINGTONE_NAME)] = {
        .maxSize = 61,
        .offset = 152,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_RINGTONE_DATA)] = {
        .maxSize = 1100,
        .offset = 220,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_MAX_ALLOWED_BLE_CONNECTION)] = {
        .maxSize = 1,
        .offset = 1328,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_MASTER_SECRET_AES)] = {
        .maxSize = 32,
        .offset = 1336,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_SOUND_VOLUME)] = {
        .maxSize = 1,
        .offset = 1376,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_REGION)] = {
        .maxSize = 1,
        .offset = 1384,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_ONBOARDED)] = {
        .maxSize = 1,
        .offset = 1392,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_OTA_MODE_ENABLED)] = {
        .maxSize = 1,
        .offset = 1400,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_PRIVACY_ID_SEED)] = {
        .maxSize = 8,
        .offset = 1408,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_PRIVACY_ID_IV)] = {
        .maxSize = 16,
        .offset = 1424,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_BOOT_REASON)] = {
        .maxSize = 1,
        .offset = 1448,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_SAVED_RTC)] = {
        .maxSize = 8,
        .offset = 1456,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_BLE_IRK)] = {
        .maxSize = 16,
        .offset = 1472,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_RINGTONE_DATA_SIZE)] = {
        .maxSize = 2,
        .offset = 1496,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_NUMBER_OF_PRIVACY_ID)] = {
        .maxSize = 4,
        .offset = 1504,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_ACTIVITY_MODE)] = {
        .maxSize = 1,
        .offset = 1516,
    },
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
    [TAG_NV_RW_ITEM_IDX(TAG_NV_UWB_GROUP_DELAY_CALIBRATED)] = {
        .maxSize = 1,
        .offset = 1524,
    },
#endif
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
    [TAG_NV_RW_ITEM_IDX(TAG_NV_FLASH_LOGGING_BLOCK_POS)] = {
        .maxSize = 1,
        .offset = 1532,
    },
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
    [TAG_NV_RW_ITEM_IDX(TAG_NV_BATTERY_LEVEL)] = {
        .maxSize = 1,
        .offset = 1540,
    },
    [TAG_NV_RW_ITEM_IDX(TAG_NV_TX_POWER)] = {
        .maxSize = 1,
        .offset = 1548,
    },
};

/* Ram based NV specific values : offset + maxSize + headerSz */
#define MAX_RAM_NV_TOTAL_SIZE (1548 + 1 + 4)

 unsigned char ramNV[MAX_RAM_NV_TOTAL_SIZE] = {0};
static unsigned int ramAgingCnt;

typedef struct
{
    TagNVItem_t item;
    PortNVDataInfo_t *nvInfo;
} RamNVParam_t;

static RamNVParam_t ramNVParam;

TagError_t PortNVInit(PortNVSupportedInfo_t *nvSupportedInfo)
{
    TAG_LOG_I ("PortNVInit 0x%x 0x%x ", nvSupportedInfo,ramNV);
    if (nvSupportedInfo == NULL)
    {
        return TAG_ERROR_NV_INVALID_DATA;
    }

    memset(ramNV, 0x00, MAX_RAM_NV_TOTAL_SIZE);

    /*
     * This NV port-layer sample codes can support RW items only
     * It supports RW items up to TAG_NV_NUMBER_OF_PRIVACY_ID
     */
    nvSupportedInfo->lastSupportedRW = RAM_NV_RW_ITEM_LAST;
    nvSupportedInfo->lastSupportedRO = TAG_NV_INVALID_ITEM;
    nvSupportedInfo->lastSupportedVendor = TAG_NV_INVALID_ITEM;

    return TAG_ERROR_NONE;
}

TagError_t PortNVDeinit(void)
{
    memset(ramNV, 0x00, MAX_RAM_NV_TOTAL_SIZE);
    return TAG_ERROR_NONE;
}

//static PortNVHandle_t* PortNVAcquireParam(TagNVItem_t item)
//{
//    PortNvsNVParam_t *data;
//
//    data = (PortNvsNVParam_t*)TagMalloc(sizeof(PortNvsNVParam_t));
//    if (NULL == data)
//    {
//        TAG_LOG_E("Malloc failed");
//        return NULL;
//    }
//    data->data_id = item;
//    return (PortNVHandle_t*)data;
//}

PortNVHandle_t *PortNVOpen(TagNVItem_t item, PortNVOpenMode_t mode)
{
    PortNVHandle_t *handle = NULL;

    if (ramNVParam.nvInfo != NULL)
    {
        return NULL;
    }

    if (mode == PORT_NV_EXIST)
    {
        /* Input arg's mode mis-match error */
        /* TAG_ERROR_NOT_SUPPORTED */
        return NULL;
    }

    if (item <= RAM_NV_RW_ITEM_LAST)
    {
        ramNVParam.nvInfo = &portNVDataRW[TAG_NV_RW_ITEM_IDX(item)];
        ramNVParam.item = item;
    }
    else
    {
        return NULL;
    }

    handle = (PortNVHandle_t *)&ramNVParam;

    return handle;
}

void PortNVClose(PortNVHandle_t *handle)
{
    RamNVParam_t *nvParam;

    if (handle == NULL)
    {
        /* TAG_ERROR_INVALID_ARG */
        return;
    }

    nvParam = (RamNVParam_t *)handle;
    nvParam->item = TAG_NV_INVALID_ITEM;
    nvParam->nvInfo = NULL;

    return;
}

static TagError_t rwNVRead(RamNVParam_t *nvParam, void *dataBuf, size_t bufSz, size_t *readSz)
{
    PortNVDataHead_packed_t *iHead;
    PortNVDataInfo_t *nvInfo = NULL;
    u32 offset;

    nvInfo = nvParam->nvInfo;
    if (nvInfo == NULL)
    {
        return TAG_ERROR_INVALID_ARG;
    }

    offset = nvInfo->offset;

    iHead = (PortNVDataHead_packed_t *)&ramNV[offset];

    if ((iHead->valSize == 0) && (iHead->valCRC == 0))
    {
        return TAG_ERROR_NV_NOT_EXIST;
    }

    if (iHead->valSize > bufSz)
    {
        return TAG_ERROR_NV_NOT_ENOUGH_SPACE;
    }

    memcpy(dataBuf, (const void *)&ramNV[offset + 4], iHead->valSize);
    *readSz = (size_t)iHead->valSize;

    return TAG_ERROR_NONE;
}

TagError_t PortNVRead(PortNVHandle_t *handle, void *dataBuf, size_t bufSz, size_t *readSz)
{
    TagError_t tagError = TAG_ERROR_NONE;
    RamNVParam_t *nvParam = NULL;

    if ((handle == NULL) || (dataBuf == NULL) || (readSz == NULL))
    {
        return TAG_ERROR_INVALID_ARG;
    }

    nvParam = (RamNVParam_t *)handle;
    if (nvParam->nvInfo == NULL)
    {
        return TAG_ERROR_NV_INVALID_PARAM;
    }

    if (nvParam->item <= RAM_NV_RW_ITEM_LAST)
    {
        tagError = rwNVRead(nvParam, dataBuf, bufSz, readSz);
    }
    else
    {
        tagError = TAG_ERROR_NOT_SUPPORTED;
    }

    return tagError;
}

TagError_t PortNVWrite(PortNVHandle_t *handle, const void *data, size_t dataSz, size_t *writtenSz)
{
    PortNVDataHead_packed_t *iHead = NULL;
    PortNVDataInfo_t *nvInfo = NULL;
    RamNVParam_t *nvParam = NULL;
    u32 offset;

    if ((handle == NULL) || (data == NULL) || (writtenSz == NULL))
    {
        return TAG_ERROR_INVALID_ARG;
    }

    nvParam = (RamNVParam_t *)handle;
    nvInfo = nvParam->nvInfo;

    if (nvInfo == NULL)
    {
        return TAG_ERROR_NV_INVALID_PARAM;
    }

    if (nvInfo->maxSize < dataSz)
    {
        return TAG_ERROR_NV_NOT_ENOUGH_SPACE;
    }

    offset = nvInfo->offset;
    iHead = (PortNVDataHead_packed_t *)&ramNV[offset];

    iHead->valSize = (uint16_t)dataSz;
    iHead->valCRC = 0xFF;

    memcpy((void *)&ramNV[offset + 4], data, dataSz);
    *writtenSz = dataSz;

    return TAG_ERROR_NONE;
}

TagError_t PortNVRemove(TagNVItem_t item)
{
    TagError_t tagError = TAG_ERROR_NONE;
    PortNVDataHead_packed_t *iHead;
    u32 offset;

    if (item == TAG_NV_RW_ITEM_ALL)
    {
        memset(ramNV, 0x00, MAX_RAM_NV_TOTAL_SIZE);
    }
    else if (item <= RAM_NV_RW_ITEM_LAST)
    {
        offset = portNVDataRW[TAG_NV_RW_ITEM_IDX(item)].offset;
        iHead = (PortNVDataHead_packed_t *)&ramNV[offset];
        iHead->valSize = 0;
        iHead->valCRC = 0;
    }
    else
    {
        tagError = TAG_ERROR_NOT_SUPPORTED;
    }

    return tagError;
}

TagError_t PortNVAccess(TagNVItem_t item, PortNVOpenMode_t mode)
{
    PortNVDataHead_packed_t *iHead;
    uint32_t offset;

    if (mode != PORT_NV_EXIST)
    {
        return TAG_ERROR_NOT_SUPPORTED;
    }

    if (item > RAM_NV_RW_ITEM_LAST)
    {
        return TAG_ERROR_NV_NOT_EXIST;
    }

    offset = portNVDataRW[TAG_NV_RW_ITEM_IDX(item)].offset;
    iHead = (PortNVDataHead_packed_t *)&ramNV[offset];

    /* No exist checking, depends on shal_nv_read_data() */
    if ((iHead->valSize == 0) && (iHead->valCRC == 0))
    {
        return TAG_ERROR_NV_NOT_EXIST;
    }
    else
    {
        return TAG_ERROR_NONE;
    }
}

TagError_t PortNVSetAgingCnt(unsigned int count)
{
    ramAgingCnt = count;

    return TAG_ERROR_NONE;
}

TagError_t PortNVGetAgingCnt(unsigned int *count)
{
    *count = ramAgingCnt;

    return TAG_ERROR_NONE;
}
#endif
