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

#include <string.h>

#include "TagConfig.h"

#include "TagDebug.h"
#include "TagErrorType.h"
#include "TagNV.h"
#include "TagOnboardingConfig.h"
#include "TagSecurity.h"

#include "PortNV.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

#ifdef TAG_CONFIG_USE_DEVICE_INFO_HEADER
#include "TagDeviceInfo.h"
#endif

#define MINIMUM_SERIAL_LENGTH (11)

#ifdef LOG_PREFIX
#undef LOG_PREFIX
#endif
#define LOG_PREFIX "NV"

typedef struct
{
    bool isStr;
    bool isInteger;
    size_t bufSz;
} tagROBufInfo_t;

typedef struct
{
    bool hasInit;
    bool isPtrData;
    TagNVData_t data;
} tagRWInfo_t;

STATIC_VARIABLE int numberOfItemHasInit;
STATIC_VARIABLE bool tagNVInitDone;
STATIC_VARIABLE PortNVSupportedInfo_t nvSupportedInfo = {
    .lastSupportedRW = TAG_NV_INVALID_ITEM,
    .lastSupportedRO = TAG_NV_INVALID_ITEM,
    .lastSupportedVendor = TAG_NV_INVALID_ITEM,
};

static const tagROBufInfo_t tagROBufInfo[TAG_NV_RO_ITEM_CNT_MAX(TAG_NV_RO_ITEM_LAST)] = {
    [TAG_NV_RO_ITEM_IDX(TAG_NV_MODEL_NAME)] = {
        .isStr = true,
        .bufSz = TAG_NV_MODEL_NAME_MAX_SZ,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_VENDOR_ID)] = {
        .isStr = true,
        .bufSz = TAG_NV_VENDOR_ID_MAX_SZ,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_MANUFACTURER_ID)] = {
        .isStr = false,
        .bufSz = TAG_NV_MANUFACTURER_ID_MAX_SZ,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_MANUFACTURER_NAME)] = {
        .isStr = true,
        .bufSz = TAG_NV_MANUFACTURER_NAME_MAX_SZ,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_SETUP_ID)] = {
        .isStr = true,
        .bufSz = TAG_NV_SETUP_ID_MAX_SZ,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_SERIAL_NUMBER)] = {
        .isStr = true,
        .bufSz = TAG_NV_SERIAL_NUMBER_MAX_SZ,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_PRIVATE_KEY_CURVED)] = {
        .isStr = false,
        .bufSz = TAG_NV_PRIVATE_KEY_CURVED_MAX_SZ,
    },
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
    [TAG_NV_RO_ITEM_IDX(TAG_NV_UWB_CH5_TXPWR_INDEX)] = {
        .isStr = false,
        .isInteger = true,
        .bufSz = TAG_NV_UWB_CH_TXPWR_INDEX_MAX_SZ,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_UWB_TXPWR_INDEX)] = {
        .isStr = false,
        .isInteger = true,
        .bufSz = TAG_NV_UWB_CH_TXPWR_INDEX_MAX_SZ,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_UWB_TXPWR_PEAK_DELTA)] = {
        .isStr = false,
        .isInteger = true,
        .bufSz = TAG_NV_UWB_CH_TXPWR_INDEX_MAX_SZ,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_UWB_TXPWR_GREF_INDEX)] = {
        .isStr = false,
        .isInteger = true,
        .bufSz = TAG_NV_UWB_CH_TXPWR_INDEX_MAX_SZ,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_UWB_FREQ_OFFSET_SIGN)] = {
        .isStr = false,
        .isInteger = true,
        .bufSz = TAG_NV_UWB_CH_TXPWR_INDEX_MAX_SZ,
    },
    [TAG_NV_RO_ITEM_IDX(TAG_NV_UWB_FREQ_OFFSET_VALUE)] = {
        .isStr = false,
        .isInteger = true,
        .bufSz = TAG_NV_UWB_CH_TXPWR_INDEX_MAX_SZ,
    },
#endif
};

static const tagRWInfo_t tagRWInfo[TAG_NV_RW_ITEM_CNT_MAX(TAG_NV_RW_ITEM_LAST)] = {
#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
    [TAG_NV_RW_ITEM_IDX(TAG_NV_BUTTON_PUSH_ACTION)] = {
        .hasInit = true,
        .data = {
            .data.buttonAction = 0x00,
            .dataLength = sizeof(((TagNVData_t *)0)->data.buttonAction),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_BUTTON_HOLD_ACTION)] = {
        .hasInit = true,
        .data = {
            .data.buttonAction = 0x00,
            .dataLength = sizeof(((TagNVData_t *)0)->data.buttonAction),
        },
    },
#endif // TAG_ACCESSORY_OPTION_BUTTON_ACTION

    [TAG_NV_RW_ITEM_IDX(TAG_NV_E2E_ENCRYPTION)] = {
        .hasInit = true,
        .data = {
            .data.e2eEncryption = 0x00,
            .dataLength = sizeof(((TagNVData_t *)0)->data.e2eEncryption),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_E2E_ENCRYPTION_KEY_AES)] = {
        .isPtrData = true,
        .data = {
            .dataLength = TAG_NV_E2E_ENCRYPTION_KEY_AES_MAX_SZ,
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_PREMATURE_OFFLINE_TIMEOUT)] = {
        .hasInit = true,
        .data = {
            .data.prematureOfflineTout = PREMATURE_TIMEOUT_PERIOD,
            .dataLength = sizeof(((TagNVData_t *)0)->data.prematureOfflineTout),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_OVERMATURE_OFFLINE_TIMEOUT)] = {
        .hasInit = true,
        .data = {
            .data.overmatureOfflineTout = OFFLINE_TIMEOUT_PERIOD,
            .dataLength = sizeof(((TagNVData_t *)0)->data.overmatureOfflineTout),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_RINGTONE_NAME)] = {
        .isPtrData = true,
        .data = {
            .dataLength = TAG_NV_RINGTONE_NAME_MAX_SZ,
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_RINGTONE_DATA)] = {
        .isPtrData = true,
        .data = {
            .dataLength = TAG_NV_RINGTONE_DATA_MAX_SZ,
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_MAX_ALLOWED_BLE_CONNECTION)] = {
        .hasInit = true,
        .data = {
            .data.maxAllowedConn = 2,
            .dataLength = sizeof(((TagNVData_t *)0)->data.maxAllowedConn),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_MASTER_SECRET_AES)] = {
        .isPtrData = true,
        .data = {
            .dataLength = TAG_NV_MASTER_SECRET_AES_MAX_SZ,
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_SOUND_VOLUME)] = {
        .hasInit = true,
        .data = {
            .data.soundVolume = 0x02,
            .dataLength = sizeof(((TagNVData_t *)0)->data.soundVolume),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_REGION)] = {
        .data = {
            .dataLength = sizeof(((TagNVData_t *)0)->data.region),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_ONBOARDED)] = {
        .hasInit = true,
        .data = {
            .data.onboarded = 0x00,
            .dataLength = sizeof(((TagNVData_t *)0)->data.onboarded),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_OTA_MODE_ENABLED)] = {
        .hasInit = true,
        .data = {
            .data.otaModeEnabled = 0x00,
            .dataLength = sizeof(((TagNVData_t *)0)->data.otaModeEnabled),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_PRIVACY_ID_SEED)] = {
        .isPtrData = true,
        .data = {
            .dataLength = TAG_NV_PRIVACY_ID_SEED_MAX_SZ,
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_PRIVACY_ID_IV)] = {
        .isPtrData = true,
        .data = {
            .dataLength = TAG_NV_PRIVACY_ID_IV_MAX_SZ,
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_BOOT_REASON)] = {
        .hasInit = true,
        .data = {
            .data.bootReason = TAG_BOOT_REASON_NORMAL,
            .dataLength = sizeof(((TagNVData_t *)0)->data.bootReason),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_SAVED_RTC)] = {
        .data = {
            .dataLength = sizeof(((TagNVData_t *)0)->data.savedRTC),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_BLE_IRK)] = {
        .isPtrData = true,
        .data = {
            .dataLength = TAG_NV_BLE_IRK_MAX_SZ,
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_RINGTONE_DATA_SIZE)] = {
        .data = {
            .dataLength = sizeof(((TagNVData_t *)0)->data.ringtoneDataSz),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_NUMBER_OF_PRIVACY_ID)] = {
        .data = {
            .data.numberOfPrivacyId = 1000,
            .dataLength = sizeof(((TagNVData_t *)0)->data.numberOfPrivacyId),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_ACTIVITY_MODE)] = {
        .hasInit = true,
        .data = {
            .data.activityMode = 0x00,
            .dataLength = sizeof(((TagNVData_t *)0)->data.activityMode),
        },
    },

#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
    [TAG_NV_RW_ITEM_IDX(TAG_NV_UWB_GROUP_DELAY_CALIBRATED)] = {
        .hasInit = true,
        .data = {
            .data.uwbGroupDelayCalibrated = 0x00,
            .dataLength = sizeof(((TagNVData_t *)0)->data.uwbGroupDelayCalibrated),
        },
    },
#endif
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
    [TAG_NV_RW_ITEM_IDX(TAG_NV_FLASH_LOGGING_BLOCK_POS)] = {
        .hasInit = true,
        .data = {
            .data.flashLoggingBlockPos = 0x00,
            .dataLength = sizeof(((TagNVData_t *)0)->data.flashLoggingBlockPos),
        },
    },
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */

    [TAG_NV_RW_ITEM_IDX(TAG_NV_BATTERY_LEVEL)] = {
        .hasInit = true,
        .data = {
            .data.batteryLevel = 0x00,
            .dataLength = sizeof(((TagNVData_t *)0)->data.batteryLevel),
        },
    },

    [TAG_NV_RW_ITEM_IDX(TAG_NV_TX_POWER)] = {
        .hasInit = true,
        .data = {
            .data.txPower = 4,
            .dataLength = sizeof(((TagNVData_t *)0)->data.txPower),
        },
    },
};

static TagError_t loadPortNV(TagNVItem_t item,
                             void *dataBuf, size_t bufSz, size_t *readSz)
{
    PortNVHandle_t *handle = NULL;
    TagError_t tagError;

    handle = PortNVOpen(item, PORT_NV_READONLY);
    if (handle == NULL)
    {
        TAG_LOG_E("Failed to execute PortNVOpen() for item(%d)/RO", (int)item);
        return TAG_ERROR_NV_PORT_OPEN_FAIL;
    }

    tagError = PortNVRead(handle, dataBuf, bufSz, readSz);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to execute PortNVRead() with bufSz(%u), error %d",
                  (unsigned int)bufSz, (int)tagError);
    }

    PortNVClose(handle);

    return tagError;
}

static TagError_t storePortNV(TagNVItem_t item,
                              const void *data, size_t dataSz, size_t *writtenSz)
{
    PortNVHandle_t *handle = NULL;
    TagError_t tagError;

    handle = PortNVOpen(item, PORT_NV_READWRITE);
    if (handle == NULL)
    {
        TAG_LOG_E("Failed to execute PortNVOpen() for item(%d)/RW", (int)item);
        return TAG_ERROR_NV_PORT_OPEN_FAIL;
    }

    tagError = PortNVWrite(handle, data, dataSz, writtenSz);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to execute PortNVWrite() with dataSz(%u), error %d",
                  (unsigned int)dataSz, (int)tagError);
    }

    PortNVClose(handle);

    return tagError;
}

STATIC_FUNCTION uint32_t setDefaultNV(uint32_t *updatedNV)
{
    TagNVItem_t lastPortRWItem;
    uint32_t succeedNV = 0;
    uint32_t failedNV = 0;
    TagError_t tagError;
    size_t writtenSz;
    int i;

    if (nvSupportedInfo.lastSupportedRW > TAG_NV_INVALID_ITEM)
    {
        lastPortRWItem = nvSupportedInfo.lastSupportedRW;
    }
    else
    {
        TAG_LOG_E("Failed to setup default-NV, there are no supported RW items");
        if (updatedNV != NULL)
        {
            *updatedNV = 0;
        }

        return 0;
    }

    numberOfItemHasInit = 0;

    for (i = TAG_NV_RW_ITEM_FIRST; i <= lastPortRWItem; i++)
    {
        if (tagRWInfo[TAG_NV_RW_ITEM_IDX(i)].hasInit == true)
        {
            numberOfItemHasInit++;
            tagError = PortNVAccess((TagNVItem_t)i, PORT_NV_EXIST);
            if (tagError == TAG_ERROR_NONE)
            {
                /* NV Has some meaningful value */
                continue;
            }
            else if (tagError == TAG_ERROR_NV_NOT_EXIST)
            {
                /* Set initial NV value */
                tagError = storePortNV((TagNVItem_t)i,
                                       (const void *)&tagRWInfo[TAG_NV_RW_ITEM_IDX(i)].data.data,
                                       tagRWInfo[TAG_NV_RW_ITEM_IDX(i)].data.dataLength, &writtenSz);
                if (tagError == TAG_ERROR_NONE)
                {
                    succeedNV = succeedNV | (1u << i);
                }
                else
                {
                    failedNV = failedNV | (1u << i);
                }
            }
            else
            {
                /* Unexpected error happened */
                failedNV = failedNV | (1u << i);
            }
        }
    }

    if (updatedNV != NULL)
    {
        *updatedNV = succeedNV;
    }

    return failedNV;
}

TagError_t TagNVInit(void)
{
    TagError_t tagError;

    if (tagNVInitDone == true)
    {
        return TAG_ERROR_NONE;
    }

    tagError = PortNVInit(&nvSupportedInfo);
    if (tagError == TAG_ERROR_NONE)
    {
        setDefaultNV(NULL);
        tagNVInitDone = true;
        TAG_LOG_D("TagNVInit() done");
    }
    else
    {
        TAG_LOG_E("Failed to execute PortNVInit(), error %d", (int)tagError);
    }

    return tagError;
}

void TagNVDeinit(void)
{
    TagError_t tagError;

    if (tagNVInitDone != true)
    {
        TAG_LOG_E("Not yet TagNVInit() done!!");
        return;
    }

    tagNVInitDone = false;
    nvSupportedInfo.lastSupportedRW = TAG_NV_INVALID_ITEM,
    nvSupportedInfo.lastSupportedRO = TAG_NV_INVALID_ITEM,
    nvSupportedInfo.lastSupportedVendor = TAG_NV_INVALID_ITEM,

    tagError = PortNVDeinit();
    if (tagError == TAG_ERROR_NONE)
    {
        TAG_LOG_I("TagNVDeInit() done");
    }
    else
    {
        TAG_LOG_E("Failed to execute PortNVDeInit(), error %d", (int)tagError);
    }

    return;
}

static bool isPortSupportedItem(TagNVItem_t item)
{
    if (item <= TAG_NV_RW_ITEM_LAST)
    {
        if ((TAG_NV_RW_ITEM_FIRST <= item) &&
            (nvSupportedInfo.lastSupportedRW >= item))
        {
            return true;
        }
    }
    else if (item <= TAG_NV_RO_ITEM_LAST)
    {
        if ((TAG_NV_RO_ITEM_FIRST <= item) &&
            (nvSupportedInfo.lastSupportedRO >= item))
        {
            return true;
        }
    }
    else if (item <= TAG_NV_VENDOR_ITEM_LAST)
    {
        if ((TAG_NV_VENDOR_ITEM_FIRST <= item) &&
            (nvSupportedInfo.lastSupportedVendor >= item))
        {
            return true;
        }
    }

    return false;
}

TagError_t TagNVLoad(TagNVItem_t item, TagNVData_t *data)
{
    TagError_t tagError;
    void *expDataBuf;
    size_t expBufSz = 0;
    size_t readSz = 0;
    bool isStr = false;

    if (tagNVInitDone != true)
    {
        TAG_LOG_E("Not yet TagNVInit() done to load!!");
        return TAG_ERROR_NV_NOT_INITIALIZED;
    }

    if (isPortSupportedItem(item) != true)
    {
        TAG_LOG_I("Port-layer does not support this item(%d)", (int)item);
        return TAG_ERROR_NV_NOT_SUPPORTED;
    }

    if (data == NULL)
    {
        TAG_LOG_E("Invalid input data for item(%d)", (int)item);
        return TAG_ERROR_INVALID_ARG;
    }

    tagError = PortNVAccess(item, PORT_NV_EXIST);
    if (tagError == TAG_ERROR_NV_NOT_EXIST)
    {
        TAG_LOG_I("noNV (%d) ", item);
        return tagError;
    }
    else if ((tagError != TAG_ERROR_NONE) &&
             (tagError != TAG_ERROR_NOT_SUPPORTED))
    {
        if ((item >= TAG_NV_RO_ITEM_FIRST) && (item <= TAG_NV_RO_ITEM_LAST))
        {
            TAG_LOG_I("Skip. we cannot check access info of RO item");
        }
        else
        {
            TAG_LOG_E("Failed to check NV item(%d) for loading", item);
            return tagError;
        }
    }

    if ((item >= TAG_NV_RW_ITEM_FIRST) && (item <= TAG_NV_RW_ITEM_LAST))
    {
        if (tagRWInfo[TAG_NV_RW_ITEM_IDX(item)].isPtrData == true)
        {
            expDataBuf = (void *)data->data.ptrData;
        }
        else
        {
            expDataBuf = (void *)&data->data;
        }

        expBufSz = tagRWInfo[TAG_NV_RW_ITEM_IDX(item)].data.dataLength;
        if (item == TAG_NV_RINGTONE_NAME)
        {
            isStr = true;
        }
    }
    else if ((item >= TAG_NV_RO_ITEM_FIRST) && (item <= TAG_NV_RO_ITEM_LAST))
    {
        if (tagROBufInfo[TAG_NV_RO_ITEM_IDX(item)].isInteger == true)
        {
            expDataBuf = (void *)&data->data;
         }
         else
         {
            expDataBuf = (void *)data->data.ptrData;
            expBufSz = tagROBufInfo[TAG_NV_RO_ITEM_IDX(item)].bufSz;
            isStr = tagROBufInfo[TAG_NV_RO_ITEM_IDX(item)].isStr;
         }
    }
    else if ((item >= TAG_NV_VENDOR_ITEM_FIRST) && (item <= TAG_NV_VENDOR_ITEM_LAST))
    {
        expDataBuf = (void *)data->data.vendorData;
        expBufSz = data->dataLength;
    }
    else
    {
        TAG_LOG_E("Not supported item(%d) to load", (int)item);
        return TAG_ERROR_NOT_SUPPORTED;
    }

    tagError = loadPortNV(item, expDataBuf, expBufSz, &readSz);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to execute loadPortNV(), error %d for item(%d)",
                  (int)tagError, (int)item);
        return tagError;
    }

    if (isStr == true)
    {
        /* NULL termination */
        uintptr_t dataBuffer = (uintptr_t) expDataBuf;
        if (expBufSz > readSz)
        {
            memset((void *) (dataBuffer + readSz), 0x00, 1);
        }
        else
        {
            memset((void *) (dataBuffer + (expBufSz - 1)), 0x00, 1);
        }
        data->dataLength = strlen(expDataBuf);
    }
    else
    {
        data->dataLength = readSz;
    }

    if (item != TAG_NV_MASTER_SECRET_AES)
    {
        TAG_LOG_I("rN (%d,%x)", (int)item, ((uint8_t *)expDataBuf)[0]);
    }

    return TAG_ERROR_NONE;
}

TagError_t TagNVStore(TagNVItem_t item, TagNVData_t *data)
{
    TagError_t tagError;
    void *expDataBuf;
    size_t writtenSz = 0;

    if (tagNVInitDone != true)
    {
        TAG_LOG_E("Not yet TagNVInit() done to store!!");
        return TAG_ERROR_NV_NOT_INITIALIZED;
    }

    if (isPortSupportedItem(item) != true)
    {
        TAG_LOG_I("Port-layer does not support this item(%d)", (int)item);
        return TAG_ERROR_NV_NOT_SUPPORTED;
    }

    if (data == NULL)
    {
        TAG_LOG_E("Invalid input data to store, item(%d)", (int)item);
        return TAG_ERROR_INVALID_ARG;
    }

    if ((item >= TAG_NV_RW_ITEM_FIRST) && (item <= TAG_NV_RW_ITEM_LAST))
    {
        if (tagRWInfo[TAG_NV_RW_ITEM_IDX(item)].isPtrData == true)
        {
            expDataBuf = (void *)data->data.ptrData;
        }
        else
        {
            expDataBuf = (void *)&data->data;
        }
    }
    else if ((item >= TAG_NV_VENDOR_ITEM_FIRST) && (item <= TAG_NV_VENDOR_ITEM_LAST))
    {
        expDataBuf = (void *)data->data.vendorData;
    }
    else
    {
        return TAG_ERROR_NOT_SUPPORTED;
    }

    tagError = storePortNV(item, expDataBuf,
                           data->dataLength, &writtenSz);

    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to execute storePortNV(), error %d for item(%d), length(%u), wrtSz(%u)",
                  (int)tagError, (int)item, (unsigned int)data->dataLength, (unsigned int)writtenSz);
    }
    else
    {
        TAG_LOG_I("wN (%d,%x)", (int)item, ((uint8_t *)expDataBuf)[0]);
    }

    return tagError;
}

TagError_t TagNVRemove(TagNVItem_t item)
{
    TagError_t tagError;

    if (tagNVInitDone != true)
    {
        TAG_LOG_E("Not yet TagNVInit() done to remove!!");
        return TAG_ERROR_NV_NOT_INITIALIZED;
    }

    if (isPortSupportedItem(item) != true)
    {
        TAG_LOG_I("Port-layer does not support this item(%d)", (int)item);
        return TAG_ERROR_NV_NOT_SUPPORTED;
    }

    if ((item >= TAG_NV_RO_ITEM_FIRST) && (item <= TAG_NV_RO_ITEM_LAST))
    {
        return TAG_ERROR_NOT_SUPPORTED;
    }

    tagError = PortNVAccess(item, PORT_NV_EXIST);
    if (tagError == TAG_ERROR_NV_NOT_EXIST)
    {
        TAG_LOG_I("There is no NV item(%d) to remove", item);
        return tagError;
    }

    tagError = PortNVRemove(item);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to execute PortRemove(), error %d for item(%d)",
                  (int)tagError, (int)item);
    }
    else
    {
        TAG_LOG_I("TagNVRemove() done for item(%d)", (int)item);
    }

    return tagError;
}

TagError_t TagNVFactoryReset(void)
{
    TagNVItem_t lastPortRWItem;
    uint32_t succeedNV = 0;
    uint32_t failedNV = 0;
    TagError_t tagError;
    int i;

    if (tagNVInitDone != true)
    {
        TAG_LOG_E("Not yet TagNVInit() done to reset!!");
        return TAG_ERROR_NV_NOT_INITIALIZED;
    }

    if (nvSupportedInfo.lastSupportedRW > TAG_NV_INVALID_ITEM)
    {
        lastPortRWItem = nvSupportedInfo.lastSupportedRW;
    }
    else
    {
        TAG_LOG_E("Failed to do factory-reset, there are no supported RW items");
        return TAG_ERROR_NV_NOT_SUPPORTED;
    }

#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
    TagNVData_t storedNVData;
    uint8_t pos = 0;

    if (TagNVLoad(TAG_NV_FLASH_LOGGING_BLOCK_POS, &storedNVData) == TAG_ERROR_NONE)
    {
        pos = storedNVData.data.flashLoggingBlockPos;
    }
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */

    /* Step 1: remove all with TAG_NV_RW_ITEM_ALL */
    tagError = PortNVRemove(TAG_NV_RW_ITEM_ALL);
    if (tagError != TAG_ERROR_NONE)
    {
        /* Step 2: remove one by one */
        for (i = TAG_NV_RW_ITEM_FIRST; i <= lastPortRWItem; i++)
        {
            tagError = PortNVAccess((TagNVItem_t)i, PORT_NV_EXIST);
            if (tagError == TAG_ERROR_NV_NOT_EXIST)
            {
                TAG_LOG_D("There is no NV item(%d) to remove", i);
                succeedNV = succeedNV | (1u << i);
                continue;
            }

            tagError = PortNVRemove((TagNVItem_t)i);
            if (tagError == TAG_ERROR_NONE)
            {
                succeedNV = succeedNV | (1u << i);
            }
            else
            {
                failedNV = failedNV | (1u << i);
            }
        }
    }

    if (failedNV == 0)
    {
        /* default reseting is optional */
        setDefaultNV(NULL);

#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
        TagNVData_t nvData;
        nvData.dataLength = 1;
        nvData.data.flashLoggingBlockPos = pos;
        TagNVStore(TAG_NV_FLASH_LOGGING_BLOCK_POS, &nvData);
        TAG_LOG_I("logPos:%d", pos);
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
        TAG_LOG_I("TagNVFactoryReset() done");

        return TAG_ERROR_NONE;
    }
    else
    {
        TAG_LOG_E("Failed to reset with succeedNV:0x%x, failedNV:0x%x",
                  succeedNV, failedNV);
        return TAG_ERROR_NV_FACTORY_RESET_FAIL;
    }
}

TagError_t TagNVSetAgingCnt(unsigned int count)
{
    TagError_t tagError;

    if (tagNVInitDone != true)
    {
        TAG_LOG_E("Not yet TagNVInit done for SetAgingCnt!!");
        return TAG_ERROR_NV_NOT_INITIALIZED;
    }

    tagError = PortNVSetAgingCnt(count);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to execute PortNVSetAgingCnt(), error %d for cnt(%u)",
                  (int)tagError, count);
    }

    return tagError;
}

TagError_t TagNVGetAgingCnt(unsigned int *count)
{
    TagError_t tagError;

    if (tagNVInitDone != true)
    {
        TAG_LOG_E("Not yet TagNVInit() done for GetAgingCnt!!");
        return TAG_ERROR_NV_NOT_INITIALIZED;
    }

    tagError = PortNVGetAgingCnt(count);
    if (tagError == TAG_ERROR_NV_NOT_EXIST)
    {
        TAG_LOG_I("There is no saved AgingCnt to get");
    }
    else if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to execute PortNVGetAgingCnt(), error %d",
                  (int)tagError);
    }

    return tagError;
}

#define GET_ONBOARDING_CONFIG_STR(_type, __pre) ( \
    (_type == TAG_ONBOARD_CONF_SETUP_ID) ? (__pre##_setupId) : ((_type == TAG_ONBOARD_CONF_MODEL_NAME) ? (__pre##_modelName) : ((_type == TAG_ONBOARD_CONF_VENDOR_ID) ? (__pre##_vid) : ((_type == TAG_ONBOARD_CONF_MANUFACTURER_ID) ? (__pre##_mnId) : ((_type == TAG_ONBOARD_CONF_MANUFACTURER_NAME) ? (__pre##_mnmn) : NULL)))))

static bool isDefaultConf(TagOnboardConfStr_t type)
{
    switch (type)
    {
    case TAG_ONBOARD_CONF_MANUFACTURER_NAME:
        if (strncmp(conf_mnmn, DEFAULT_CONF_MNMN,
                    sizeof(DEFAULT_CONF_MNMN)) == 0)
        {
            return true;
        }
        break;

    case TAG_ONBOARD_CONF_MANUFACTURER_ID:
        if (strncmp(conf_mnId, DEFAULT_CONF_MNID,
                    sizeof(DEFAULT_CONF_MNID)) == 0)
        {
            return true;
        }
        break;

    case TAG_ONBOARD_CONF_VENDOR_ID:
        if (strncmp(conf_vid, DEFAULT_CONF_VID,
                    sizeof(DEFAULT_CONF_VID)) == 0)
        {
            return true;
        }
        break;

    case TAG_ONBOARD_CONF_MODEL_NAME:
        if (strncmp(conf_modelName, DEFAULT_CONF_MODEL_NAME,
                    sizeof(DEFAULT_CONF_MODEL_NAME)) == 0)
        {
            return true;
        }
        break;

    case TAG_ONBOARD_CONF_SETUP_ID:
        /* setupId can be load from NV */
    case TAG_ONBOARD_CONF_TYPE_MAX:
    default:
        TAG_LOG_D("Skip default TagOnboardingConfig checking for type : %d", type);
        break;
    }

    return false;
}

const char *TagGetOnboardConfStrPtr(TagOnboardConfStr_t type)
{
    const char *strPtr = NULL;

    if ((type < TAG_ONBOARD_CONF_TYPE_START) || (type >= TAG_ONBOARD_CONF_TYPE_MAX))
    {
        TAG_LOG_E("Not supported tagOnbardConfStr type : %d", type);
        return NULL;
    }

    if (isDefaultConf(type) == true)
    {
        if (type == TAG_ONBOARD_CONF_MODEL_NAME)
        {
            TAG_LOG_E("Default 'NAME' used, need to check TagOnboardingConfig.h file");
        }
        else
        {
            TAG_LOG_E("Failed to get string, need to change TagOnboardingConfig.h file");
            return NULL;
        }
    }

    strPtr = GET_ONBOARDING_CONFIG_STR(type, conf);
    TAG_LOG_D("[OnboardStr/%d] %s, sz:%d", type,
              (strPtr ? strPtr : "NULL"), (strPtr ? strlen(strPtr) : 0));

    return strPtr;
}

#if defined(TAG_CONFIG_USE_DEVICE_INFO_HEADER)
static bool isDefaultDevInfo(TagDeviceInfoType_t type)
{
    switch (type)
    {
    case TAG_DEVICE_INFO_PRIVATE_KEY_CURVED:
        if (strncmp(conf_device_seckey_curve25519, DEFAULT_DEVINFO_SECKEY,
                    sizeof(DEFAULT_DEVINFO_SECKEY)) == 0)
        {
            return true;
        }
        break;

    case TAG_DEVICE_INFO_SERIAL:
        if (strncmp(conf_device_serial_number, DEFAULT_DEVINFO_SERIAL,
                    sizeof(DEFAULT_DEVINFO_SERIAL)) == 0)
        {
            return true;
        }
        break;

    case TAG_DEVICE_INFO_TYPE_MAX:
    default:
        TAG_LOG_D("Skip default TagDeviceInfo checking for type : %d", type);
        break;
    }

    return false;
}
#endif

TagDeviceInfoData_t *TagAllocDeviceInfo(TagDeviceInfoType_t type)
{
    TagDeviceInfoData_t *infoData = NULL;
    size_t dataBufSz, srcSz;
    void *dataPtr = NULL;
#ifndef TAG_CONFIG_USE_DEVICE_INFO_HEADER
    bool isPortSupport = false;
    TagNVData_t nvData;
    TagError_t ret;
#endif

    if ((type < TAG_DEVICE_INFO_TYPE_START) || (type >= TAG_DEVICE_INFO_TYPE_MAX))
    {
        TAG_LOG_E("Not supported TagDeviceInfo type : %d", type);
        return NULL;
    }

#if defined(TAG_CONFIG_USE_DEVICE_INFO_HEADER)
    if (isDefaultDevInfo(type) == true)
    {
        TAG_LOG_E("Failed to get data, need to change TagDeviceInfo.h file");
        return NULL;
    }
#else
    if (tagNVInitDone != true)
    {
        TAG_LOG_E("Not yet TagNVInit() done to load DeviceInfo!!");
        return NULL;
    }

    if (type == TAG_DEVICE_INFO_SERIAL)
    {
        isPortSupport = isPortSupportedItem(TAG_NV_SERIAL_NUMBER);
    }
    else if (type == TAG_DEVICE_INFO_PRIVATE_KEY_CURVED)
    {
        isPortSupport = isPortSupportedItem(TAG_NV_PRIVATE_KEY_CURVED);
    }

    if (isPortSupport != true)
    {
        TAG_LOG_I("Port-layer does not support this DeviceInfoType(%d)", (int)type);
        return NULL;
    }
#endif

    infoData = TagMalloc(sizeof(TagDeviceInfoData_t));
    if (infoData == NULL)
    {
        TAG_LOG_E("Failed to allocate memory for infoData");
        return NULL;
    }
    memset(infoData, 0x00, sizeof(TagDeviceInfoData_t));

    infoData->type = type;
    switch (type)
    {
    case TAG_DEVICE_INFO_SERIAL:
#if defined(TAG_CONFIG_USE_DEVICE_INFO_HEADER)
        dataBufSz = strlen(conf_device_serial_number) + 1;
#else
        dataBufSz = TAG_NV_SERIAL_NUMBER_MAX_SZ;
#endif /* TAG_CONFIG_USE_DEVICE_INFO_HEADER */

        infoData->data = TagMalloc(dataBufSz);
        if (infoData->data == NULL)
        {
            TAG_LOG_E("Failed to allocate memory for infoData->data(t:%d)", (int)type);
            goto error_alloc_dev_info;
        }
        memset(infoData->data, 0x00, dataBufSz);

#if defined(TAG_CONFIG_USE_DEVICE_INFO_HEADER)
        // dataPtr = (void *)conf_device_serial_number;
        srcSz = strlen(conf_device_serial_number);

        memcpy(infoData->data, conf_device_serial_number, srcSz);
#else
        nvData.data.serialNumber = TagMalloc(TAG_NV_SERIAL_NUMBER_MAX_SZ);
        if (nvData.data.serialNumber == NULL)
        {
            TAG_LOG_E("Failed to alloc for serialNumber");
            goto error_alloc_dev_info;
        }

        memset(nvData.data.serialNumber, 0, TAG_NV_SERIAL_NUMBER_MAX_SZ);

        ret = TagNVLoad(TAG_NV_SERIAL_NUMBER, &nvData);
        if (ret != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to load s/n from NV.");

            goto error_alloc_dev_info;
        }

        infoData->data = nvData.data.serialNumber ;
        srcSz = nvData.dataLength;
#endif /* TAG_CONFIG_USE_DEVICE_INFO_HEADER */

        if (srcSz < MINIMUM_SERIAL_LENGTH)
        {
            TAG_LOG_E("Failed to check minimum Serial length(%d)", (int)srcSz);
            goto error_alloc_dev_info;
        }

        infoData->dataLength = srcSz;
        break;

    case TAG_DEVICE_INFO_PRIVATE_KEY_CURVED:
        dataBufSz = TAG_NV_PRIVATE_KEY_CURVED_MAX_SZ;

        infoData->data = TagMalloc(dataBufSz);
        if (infoData->data == NULL)
        {
            TAG_LOG_E("Failed to allocate memory for infoData->data(t:%d)", (int)type);
            goto error_alloc_dev_info;
        }
        memset(infoData->data, 0x00, dataBufSz);

#if defined(TAG_CONFIG_USE_DEVICE_INFO_HEADER)
        dataPtr = infoData->data;
        if (TagCryptoBase64Decode((const unsigned char *)conf_device_seckey_curve25519,
                                  strlen(conf_device_seckey_curve25519), dataPtr,
                                  TAG_NV_PRIVATE_KEY_CURVED_MAX_SZ, &srcSz) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to decode PrivateKeyCurved from DeviceInfo");
            goto error_alloc_dev_info;
        }
        else if (srcSz != TAG_SECURITY_ED25519_LEN)
        {
            TAG_LOG_E("Failed to check length, decoded length is abnormal(%d)", (int)srcSz);
            goto error_alloc_dev_info;
        }
#else
        nvData.data.privateKeyCurved = TagMalloc(TAG_NV_PRIVATE_KEY_CURVED_MAX_SZ);
        if (nvData.data.privateKeyCurved == NULL)
        {
            TAG_LOG_E("Failed to alloc for privateKeyCurved");
            goto error_alloc_dev_info;
        }

        memset(nvData.data.privateKeyCurved, 0, TAG_NV_PRIVATE_KEY_CURVED_MAX_SZ);

        ret = TagNVLoad(TAG_NV_PRIVATE_KEY_CURVED, &nvData);
        if (ret != TAG_ERROR_NONE)
        {
            TAG_LOG_I("Failed to load TAG_NV_PRIVATE_KEY_CURVED from NV. Try to get from header");
            goto error_alloc_dev_info;
        }

        infoData->data = nvData.data.privateKeyCurved ;
        srcSz = nvData.dataLength;

#endif /* TAG_CONFIG_USE_DEVICE_INFO_HEADER */

        infoData->dataLength = srcSz;
        break;

    case TAG_DEVICE_INFO_TYPE_MAX:
    default:
        TAG_LOG_E("Not support type(%d) to get deviceInfo", (int)type);
        goto error_alloc_dev_info;
    }

    return infoData;

error_alloc_dev_info:

    if (infoData->data != NULL)
    {
        TagFree(infoData->data);
        infoData->data = NULL;
    }

    if (infoData != NULL)
    {
        TagFree(infoData);
        infoData = NULL;
    }

    return NULL;
}

void TagFreeDeviceInfo(TagDeviceInfoData_t *data)
{
    if (data == NULL)
    {
        TAG_LOG_E("Invalid TagDeviceInfoData to free");
        return;
    }

    if (data->data != NULL)
    {
        TagFree(data->data);
        data->data = NULL;
    }

    TagFree(data);
    data = NULL;

    return;
}
