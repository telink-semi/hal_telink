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

#ifndef TAGSDK_INC_TAGNV_H_
#define TAGSDK_INC_TAGNV_H_

#include <stdint.h>

#include "TagErrorType.h"
#include "TagNVItem.h"

#define TAG_NV_E2E_ENCRYPTION_KEY_AES_MAX_SZ    (100)
#define TAG_NV_RINGTONE_NAME_MAX_SZ             (60 + 1)
#define TAG_NV_RINGTONE_DATA_MAX_SZ             (1100)
#define TAG_NV_MASTER_SECRET_AES_MAX_SZ         (32)
#define TAG_NV_MODEL_NAME_MAX_SZ                (31 + 1)
#define TAG_NV_VENDOR_ID_MAX_SZ                 (39 + 1)
#define TAG_NV_MANUFACTURER_ID_MAX_SZ           (4)
#define TAG_NV_MANUFACTURER_NAME_MAX_SZ         (31 + 1)
#define TAG_NV_SETUP_ID_MAX_SZ                  (3 + 1)
#define TAG_NV_SERIAL_NUMBER_MAX_SZ             (31 + 1)
#define TAG_NV_PRIVATE_KEY_CURVED_MAX_SZ        (32)
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
#define TAG_NV_UWB_CH_TXPWR_INDEX_MAX_SZ        (1)
#endif
#define TAG_NV_DEFAULT_RINGTONE_DATA_MAX_SZ     (660)
#define TAG_NV_PRIVACY_ID_SEED_MAX_SZ           (8)
#define TAG_NV_PRIVACY_ID_IV_MAX_SZ             (16)
#define TAG_NV_NUMBER_OF_PRIVACY_ID_MAX_SZ      (4)
#define TAG_NV_BLE_IRK_MAX_SZ                   (16)

/* use to check with conf/TagOnboardingConfig.h values */
#define DEFAULT_CONF_MNMN "MNMN"
#define DEFAULT_CONF_MNID "MNID"
#define DEFAULT_CONF_VID "VID"
#define DEFAULT_CONF_MODEL_NAME "NAME"

#ifdef TAG_CONFIG_USE_DEVICE_INFO_HEADER
/* use to check with conf/TagDeviceInfo.h values */
#define DEFAULT_DEVINFO_SECKEY "privateKey_here"
#define DEFAULT_DEVINFO_SERIAL "serialNumber_here"
#endif

/**
 * @brief Contains a representation of "NV" data
 */
typedef struct {
    union {
        void *ptrData;                  /**< @brief to re-indicate pointer type data such as *e2eEncryptionKeyAES */

        /* RW items, read/write by TagSDK core */
        uint8_t buttonAction;             /**< @brief to indicate TAG_NV_BUTTON_xxx_ACTION type data */
        uint8_t e2eEncryption;            /**< @brief to indicate TAG_NV_E2E_ENCRYPTION type data */
        uint8_t *e2eEncryptionKeyAES;    /**< @brief to indicate TAG_NV_E2E_ENCRYPTION_KEY_AES type data */
        uint16_t prematureOfflineTout;    /**< @brief to indicate TAG_NV_PREMATURE_OFFLINE_TIMEOUT type data */
        uint32_t overmatureOfflineTout;    /**< @brief to indicate TAG_NV_OVERMATURE_OFFLINE_TIMEOUT type data */
        char *ringtoneName;                /**< @brief to indicate TAG_NV_RINGTONE_NAME type data */
        uint8_t *ringtoneData;            /**< @brief to indicate TAG_NV_RINGTONE_DATA type data */
        uint8_t maxAllowedConn;            /**< @brief to indicate TAG_NV_MAX_ALLOWED_BLE_CONNECTION type data */
        uint8_t *masterSecretAES;        /**< @brief to indicate TAG_NV_MASTER_SECRET_AES type data */
        uint8_t soundVolume;            /**< @brief to indicate TAG_NV_SOUND_VOLUME type data */
        uint8_t region;                    /**< @brief to indicate TAG_NV_REGION type data */
        uint8_t onboarded;              /**< @brief to indicate TAG_NV_ONBOARDED type data */
        uint8_t otaModeEnabled;         /**< @brief to indicate TAG_NV_OTA_MODE_ENABLED type data */
        uint8_t *privacyIdSeed;         /**< @brief to indicate TAG_NV_PRIVACY_ID_SEED type data */
        uint8_t *privacyIdIv;           /**< @brief to indicate TAG_NV_PRIVACY_ID_IV type data */
        uint8_t bootReason;             /**< @brief to indicate TAG_NV_BOOT_REASON type data */
        uint64_t savedRTC;              /**< @brief to indicate TAG_NV_SAVED_RTC type data */
        uint8_t *bleIrk;                /**< @brief to indicate TAG_NV_BLE_IRK type data */
        uint16_t ringtoneDataSz;        /**< @brief to indicate TAG_NV_RINGTONE_DATA_SIZE type data */
        uint32_t numberOfPrivacyId;     /**< @brief to indicate TAG_NV_NUMBER_OF_PRIVACY_ID type data */
        uint8_t activityMode;           /**< @brief to indicate TAG_NV_ACTIVITY_MODE type data */
        int8_t txPower;                   /**< @brief to indicate TAG_NV_TX_POWER type data */
        uint8_t flashLoggingBlockPos;    /**< @brief to indicate TAG_NV_FLASH_LOGGING_BLOCK_POS type data */
        uint8_t batteryLevel;           /**< @brief to indicate TAG_NV_BATTERY_LEVEL type data */
//#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
        uint8_t uwbGroupDelayCalibrated;/**< @brief to indicate TAG_NV_UWB_GROUP_DELAY_CALIBRATED type data */
        /* Factory(RO) items, write at once by vendor */
        uint8_t UWBCh5TxPwrIdx;         /**< @brief to indicate TAG_NV_UWB_CH5_TXPWR_INDEX type data */
        uint8_t UWBTxPwrIdx;            /**< @brief to indicate TAG_NV_UWB_TXPWR_INDEX type data */
        uint8_t UWBTxPwrPeakDelta;      /**< @brief to indicate TAG_NV_UWB_TXPWR_PEAK_DELTA type data */
        uint8_t UWBTxPwrGrefIdx;        /**< @brief to indicate TAG_NV_UWB_TXPWR_GREF_INDEX type data */
        uint8_t UWBFreqOffsetSign;      /**< @brief to indicate TAG_NV_UWB_FREQ_OFFSET_SIGN type data */
        uint32_t UWBFreqOffsetValue;    /**< @brief to indicate TAG_NV_UWB_FREQ_OFFSET_VALUE type data */
//#endif
        /* Factory(RO) items, write at once by vendor */
        char *modelName;                /**< @brief to indicate TAG_NV_MODEL_NAME type data */
        char *vendorId;                 /**< @brief to indicate TAG_NV_VENDOR_ID type data */
        char *manufacturerId;           /**< @brief to indicate TAG_NV_MANUFACTURER_ID type data */
        char *manufacturerName;         /**< @brief to indicate TAG_NV_MANUFACTURER_NAME type data */
        char *setupId;                  /**< @brief to indicate TAG_NV_SETUP_ID type data */
        char *serialNumber;             /**< @brief to indicate TAG_NV_SERIAL_NUMBER type data */
        uint8_t *privateKeyCurved;      /**< @brief to indicate TAG_NV_PRIVATE_KEY_CURVED type data */

        /* For Vendor's specific data handle */
        void *vendorData;               /**< @brief to indicate TAG_NV_VENDOR_DATA type data */
    } data;

    size_t dataLength; /**< @brief Actual data size, such as strlen */
} TagNVData_t;

/**
 * @brief Contains a enumeration values for types of TagOnBoardingConfig.h
 */
typedef enum {
    TAG_ONBOARD_CONF_TYPE_START = 0,
    TAG_ONBOARD_CONF_SETUP_ID = TAG_ONBOARD_CONF_TYPE_START,
    TAG_ONBOARD_CONF_MODEL_NAME,
    TAG_ONBOARD_CONF_VENDOR_ID,
    TAG_ONBOARD_CONF_MANUFACTURER_ID,
    TAG_ONBOARD_CONF_MANUFACTURER_NAME,
    TAG_ONBOARD_CONF_TYPE_MAX,
} TagOnboardConfStr_t;

/**
 * @brief Contains a enumeration values for types of TagDeviceInfo.h
 */
typedef enum {
    TAG_DEVICE_INFO_TYPE_START = 0,
    TAG_DEVICE_INFO_SERIAL = TAG_DEVICE_INFO_TYPE_START,
    TAG_DEVICE_INFO_PRIVATE_KEY_CURVED,
    TAG_DEVICE_INFO_TYPE_MAX,
} TagDeviceInfoType_t;

/**
 * @brief Contains "TagDeviceInfo" data such as private key & serial
 */
typedef struct {
    TagDeviceInfoType_t type;   /**< @brief TagDeviceInfo data type */
    void *data;                 /**< @brief actual data */
    size_t dataLength;          /**< @brief data length */
} TagDeviceInfoData_t;

/**
 * @brief Initialization of TagNV module
 *
 * @details This function tries to initialize TagNV module.
 *          Internally, this function calls PortNVInit() to prepare for usage of
 *          target specific NV(non-volatile memory) implementation.
 *          And sets all default Tag's NV items to use them in TagCore
 * @return return TAG_ERROR_NONE for success, otherwise for failure
 *
 */
TagError_t TagNVInit(void);

/**
 * @brief Deinitialization of TagNV module
 *
 * @details This function tries to deinitialize TagNV module.
 *          Internally, this function calls PortNVDeinit() to release for usage of
 *          target specific NV(non-volatile memory) implementation.
 *
 */
void TagNVDeinit(void);

/**
 * @brief Load Tag's NV item
 *
 * @details This function tries to load each NV value from TagNV module.
 *          Internally, this function will call several port-layer functions such as
 *          PortNVAccess(), PortNVAcquireParam(), PortNVReleaseParam(),
 *          PortNVOpen(), PortNVRead(), PortNVClose(),.. to load actual NV data from
 *          the target specific NV(non-volatile memory) implementation.
 * @param[in] item Each type of TagNVItem_t to load it
 * @param[out] nvData A pointer for actual data to get each NV item.
 *                    It must have own pre-allocated memory for each pointer type data
 *                    such as *e2eEncryptionKeyAES, *ringtoneName cases and uses
 *                    TAG_NV_XXX_MAX_SZ definition to allocate the memory of the pointer type data
 * @return return TAG_ERROR_NONE for success, otherwise for failure
 * @code
 *  TagNVData_t nvRingtoneName;
 *  TagError_t tagError;
 *
 *  nvRingtoneName.data.ringtoneName = TagMalloc(TAG_NV_RINGTONE_NAME_MAX_SZ);
 *  tagError = TagNVLoad(TAG_NV_RINGTONE_NAME, &nvRingtoneName);
 *  if (tagError != TAG_ERROR_NONE)
 *  {
 *      TAG_LOG_E("Can't get RingtoneName (%d)", (int)tagError);
 *  }
 *  else
 *  {
 *      TAG_LOG_I("Get RingtoneName, size:%d", nvRingtoneName.dataLength);
 *  }
 * @endcode
 *
 */
TagError_t TagNVLoad(TagNVItem_t item, TagNVData_t *nvData);

/**
 * @brief Store Tag's NV item
 *
 * @details This function tries to store each NV value to TagNV module.
 *          Internally, this function will call several port-layer functions such as
 *          PortNVAccess(), PortNVAcquireParam(), PortNVReleaseParam(),
 *          PortNVOpen(), PortNVWrite(), PortNVClose(),.. to store actual NV data to
 *          the target specific NV(non-volatile memory) implementation.
 * @param[in] item Each type of TagNVItem_t to store it
 * @param[in] nvData A pointer for actual data to set each NV item.
 *                   It must have own pre-allocated memory & data-length for each pointer type data
 *                   such as *e2eEncryptionKeyAES, *ringtoneName cases and uses
 *                   TAG_NV_XXX_MAX_SZ definition to allocate the memory of the pointer type data
 * @return return TAG_ERROR_NONE for success, otherwise for failure
 * @code
 *  TagNVData_t nvRingtoneName;
 *  TagError_t tagError;
 *
 *  nvRingtoneName.data.ringtoneName = TagMalloc(TAG_NV_RINGTONE_DATA_MAX_SZ);
 *  memset(nvRingtoneName.data.ringtoneData, '\0', TAG_NV_RINGTONE_DATA_MAX_SZ);
 *
 *  strncpy(nvRingtoneName.data.ringtoneName, "My new Ringtone", (NV_RINGTONE_NAME_MAX_SZ - 1));
 *  nvRingtoneName.dataLength = strlen(nvRingtoneName.data.ringtoneName);
 *
 *  tagError = TagNVStore(TAG_NV_RINGTONE_NAME, &nvRingtoneName);
 *  if (tagError != TAG_ERROR_NONE)
 *  {
 *      TAG_LOG_E("Can't set RingtoneName (%d)", (int)tagError);
 *  }
 *  else
 *  {
 *      TAG_LOG_I("Set RingtoneName OK");
 *  }
 * @endcode
 *
 */
TagError_t TagNVStore(TagNVItem_t item, TagNVData_t *nvData);

/**
 * @brief Remove Tag's NV item
 *
 * @details This function tries to remove NV item from TagNV module.
 *          Internally, this function will call several port-layer functions such as
 *          PortNVRemove(),.. to remove actual NV data from
 *          the target specific NV(non-volatile memory) implementation.
 * @param[in] item Each type of TagNVItem_t to remove it
 * @return return TAG_ERROR_NONE for success, otherwise for failure
 *
 */
TagError_t TagNVRemove(TagNVItem_t item);

/**
 * @brief Try to do Factory-reset
 *
 * @details This function tries to remove all RW based NV item.
 *          And reset again some NV items by the default value.
 *          Internally, this function will call several port-layer functions such as
 *          PortNVAccess(), PortNVAcquireParam(), PortNVReleaseParam(), PortNVRemove(),
 *          PortNVOpen(), PortNVWrite(), PortNVClose(),..
 *          to reset some NV data into the target specific NV(non-volatile memory) implementation.
 * @param[in] item Each type of TagNVItem_t to remove it
 * @return return TAG_ERROR_NONE for success, otherwise for failure
 *
 */
TagError_t TagNVFactoryReset(void);

/**
 * @brief Set Tag's Aging-count value
 *
 * @details This function tries to set Tag's Aging-count value to TagNV module.
 *          Internally, this function will call several port-layer functions such as
 *          PortNVSetAgingCnt(),.. to set Aging-count value to
 *          the target specific NV(non-volatile memory) implementation.
 *          TagCore tries to set(call) 'Aging-count value' every 15 minutes.
 * @param[in] count Aging-count value to set
 * @return return TAG_ERROR_NONE for success, otherwise for failure
 *
 */
TagError_t TagNVSetAgingCnt(unsigned int count);

/**
 * @brief Get Tag's saved Aging-count value
 *
 * @details This function tries to get Tag's saved Aging-count value from TagNV module.
 *          Internally, this function will call several port-layer functions such as
 *          PortNVGetAgingCnt(),.. to get Aging-count value from
 *          the target specific NV(non-volatile memory) implementation.
 *          TagCore will get latest saved 'Aging-count value' using this API.
 * @param[out] count Aging-count value to get
 * @return return TAG_ERROR_NONE for success, otherwise for failure
 *
 */
TagError_t TagNVGetAgingCnt(unsigned int *count);

/**
 * @brief Get TagOnboardingConfig.h based string constant
 *
 * @details This function tries to get string constant from the TagOnboardingConfig.h
 * @param[in] type Each type of TagOnboardConfStr_t to get it
 * @return return string constant poninter for success, NULL for failure
 *
 */
const char* TagGetOnboardConfStrPtr(TagOnboardConfStr_t type);

/**
 * @brief Allocate TagDeviceInfo.h or NV based Device-Information
 *
 * @details This function tries to allocate Device-Information from the TagOnboardingConfig.h or NV
 * @param[in] type Each type of TagDeviceInfoType_t to allocate it
 * @return return allocated pointer for success, NULL for failure
 * @code
 *  TagDeviceInfoData_t *serial = NULL;
 *
 *  serial = TagAllocDeviceInfo(TAG_DEVICE_INFO_SERIAL);
 *  if (serial != NULL)
 *  {
 *      TAG_LOG_E("Can't allocate Serial");
 *  }
 *  else
 *  {
 *      TAG_LOG_I("Allocate Serial, size(%d)", serial->dataLength);
 *  }
 * @endcode
 *
 */
TagDeviceInfoData_t* TagAllocDeviceInfo(TagDeviceInfoType_t type);

/**
 * @brief Try for free allocated Device-Information
 *
 * @details This function tries for free allocated Device-Information from the TagOnboardingConfig.h or NV
 * @param[in] data Allocated TagDeviceInfoType_t data for free
 * @code
 *  TagDeviceInfoData_t *serial = NULL;
 *
 *  serial = TagAllocDeviceInfo(TAG_DEVICE_INFO_SERIAL);
 *  if (serial != NULL)
 *  {
 *      TAG_LOG_E("Can't allocate Serial");
 *  }
 *  else
 *  {
 *      TAG_LOG_I("Allocate Serial, size(%d)", serial->dataLength);
 *      TagFreeDeviceInfo(serial);
 *  }
 * @endcode
 *
 */
void TagFreeDeviceInfo(TagDeviceInfoData_t *data);

#endif /* TAGSDK_INC_TAGNV_H_ */
