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

#include "TagAuthService.h"
#include "TagCore.h"
#include "TagErrorType.h"
#include "TagFwUpdate.h"
#include "TagNV.h"
#include "TagNVItem.h"
#include "TagOnboardingService.h"
#include "TagSecurity.h"
#include "TagSoundPlayer.h"
#include "TagUtil.h"
#include "TagVersion.h"
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
#include "TagLedBlink.h"
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
#include "TagNFC.h"
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */

/* port related headers */
#include "PortBattery.h"
#include "PortBle.h"
#include "PortButton.h"
#include "PortOs.h"
#include "PortSleep.h"
#include "PortSystem.h"
#include "PortTime.h"
#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
#include "PortFwUpdate.h"
#endif // TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
#include "PortUwb.h"
#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */

#ifdef LOG_PREFIX
#undef LOG_PREFIX
#endif
#define LOG_PREFIX "CORE"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

#define AGING_COUNTER_CALCULATE_COMPENSATION_VALUE 3             /* 3 seconds margin for calculate aging counter to compensate rtc error */
#define TAG_BATTERY_CHECK_TIMER_PERIOD_SEC  7

TagContext *gTagContext;

STATIC_VARIABLE uint32_t overmaturePrivacyIdChangePeriod[3] = {96, 1, 2}; /* 0x00 : every 24 hours(96*15mins), 0x01 : every 15 mins, 0x02 : every 30 mins */

STATIC_VARIABLE bool initBattery = false;

TagError_t TagBatteryInit(void);

void TagUpdateAgingCounter(void)
{
    uint64_t utcTime;
    uint64_t remainTimeForNextCounter;
    TagError_t ret = TAG_ERROR_NONE;

#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
    if (PortFwUpdateStatus() == true)
    {
        TAG_LOG_E("Skip. FwUpdate is working");
        return;
    }
#endif // TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE

    utcTime = PortTimeGetRtcTime();
    if (utcTime < AGING_COUNTER_NORM_UTC)
    {
        TAG_LOG_E("RTC data is old %u", (uint32_t)utcTime);
        utcTime = AGING_COUNTER_NORM_UTC;
    }
    gTagContext->agingCounter = (utcTime + AGING_COUNTER_CALCULATE_COMPENSATION_VALUE - AGING_COUNTER_NORM_UTC) / AGING_COUNTER_INTERVAL;
    ret = TagNVSetAgingCnt(gTagContext->agingCounter);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to save aging counter %d", ret);
    }

    remainTimeForNextCounter = (gTagContext->agingCounter + 1) * AGING_COUNTER_INTERVAL + AGING_COUNTER_NORM_UTC - utcTime;

    PortTimerChangePeriod(gTagContext->agingCounterTimer, CONVERT_SEC_TO_TICKS(remainTimeForNextCounter), 0);

    gTagContext->needAdvAddrChanged = 1;

    /* PrivacyId check */
    if (gTagContext->state != TAG_STATE_OUT_OF_BOX &&
        gTagContext->state != TAG_STATE_OOB_DEEP_SLEEP &&
        !(gTagContext->state == TAG_STATE_OVERMATURE_OFFLINE &&
          gTagContext->agingCounter - gTagContext->privacyIdLastUpdateCounter < overmaturePrivacyIdChangePeriod[gTagContext->overmaturePrivacyIdInterval]))
    {
        TagSecurityBuffer_t privacyIdBuf = {0};
        ret = TagAuthGetPrivacyId(gSecuContext, &privacyIdBuf);
        if (ret != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Fail to create privacyID");
            return;
        }
        gTagContext->privacyIdLength = privacyIdBuf.len;
        memcpy(gTagContext->privacyId, privacyIdBuf.p, privacyIdBuf.len);
        gTagContext->privacyIdLastUpdateCounter = gTagContext->agingCounter;
        TagCryptoSecurityBufferFree(&privacyIdBuf);
    }

    /* Check battery change */
    if (TagUpdateBatteryLevel() && gTagContext->state == TAG_STATE_CONNECTED)
    {
        EndUserDevice *endUserDevice = gTagContext->endUserDevices;
        TagControlServiceData *data = NULL;
        data = AllocateTagControlServiceData(1);
        if (data == NULL)
        {
            TAG_LOG_E("Failed to allocate data");
        }
        else
        {
            data->aValue[0] = gTagContext->batteryLevel;
            while (endUserDevice)
            {
                if (endUserDevice->deviceType == END_USER_DEVICE_OWNER)
                {
                    TagControlSendIndication(endUserDevice, CTRL_BATTERY, data, ENCRYPTION_REQUIRED);
                }
                endUserDevice = endUserDevice->next;
            }
            FreeTagControlServiceData(data);
        }
    }

    TAG_LOG_D("RTC=%u, nc(%us), bat(%d)", (uint32_t)utcTime, (uint32_t)remainTimeForNextCounter, gTagContext->batteryLevel);
    TagRefreshAdv();
    TAG_MEM_CHECK("Update aging counter");
}

STATIC_FUNCTION void tagUpdateAgingCounter(TagTaskWorkParam param)
{
    TagUpdateAgingCounter();
}

STATIC_FUNCTION void agingCounterTimerCallback(PortTimerHandle_t timer)
{
    TagPutPostWork(tagUpdateAgingCounter, NULL);
}

STATIC_FUNCTION void prematureTimeoutHandler(TagTaskWorkParam param)
{
    TagTransferState(TAG_STATE_OFFLINE);
    TagRefreshAdv();
}

STATIC_FUNCTION void prematureTimeoutCallback(PortTimerHandle_t timer)
{
    TagPutPostWork(prematureTimeoutHandler, NULL);
}

STATIC_FUNCTION void offlineTimeoutHandler(TagTaskWorkParam param)
{
    TagTransferState(TAG_STATE_OVERMATURE_OFFLINE);
    TagRefreshAdv();
}

STATIC_FUNCTION void offlineTimeoutCallback(PortTimerHandle_t timer)
{
    TagPutPostWork(offlineTimeoutHandler, NULL);
}

STATIC_FUNCTION void OOBCompletedTimeoutHandler(TagTaskWorkParam param)
{
    gTagContext->OOBCompletedActiveFlag = 0;
    TagRefreshAdv();
}

STATIC_FUNCTION void OOBCompletedTimeoutCallback(PortTimerHandle_t timer)
{
    TagPutPostWork(OOBCompletedTimeoutHandler, NULL);
}

STATIC_FUNCTION void requestAdvIntervalTimeoutHandler(TagTaskWorkParam param)
{
    gTagContext->requestAdvIntervalActiveFlag = 0;
    TagRefreshAdv();
    CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_ERROR), SOUND_NOT_PLAYED, "Failed to play ERROR sound");
}

STATIC_FUNCTION void requestAdvIntervalTimeoutCallback(PortTimerHandle_t timer)
{
    TagPutPostWork(requestAdvIntervalTimeoutHandler, NULL);
}

STATIC_FUNCTION void OOBAdvFastIntervalTimeoutHandler(TagTaskWorkParam param)
{
    PortTimerHandle_t timer = param;

    /* Check timer is valid*/
    if (timer == gTagContext->OOBAdvFastIntervalTimer)
    {
        TAG_LOG_D("OOB fast interval advertising expired");
        gTagContext->OOBAdvFastIntervalTimer = NULL;
        if (gTagContext->onBoardingStatus == PRE_ONBOARDING)
        {
            TAG_LOG_D("OOB fast interval advertising expired");
#ifdef TAG_CONFIG_ENABLE_OOB_DEEP_SLEEP
            TagTransferState(TAG_STATE_OOB_DEEP_SLEEP);
#endif
            TagRefreshAdv();
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_OFF), SOUND_NOT_PLAYED, "Failed to play OFF sound");
            TagOnboardingReportButtonEvent(BUTTON_RELEASE);
        }
        else
        {
            TAG_LOG_D("OOB fast interval advertising expired, but restart timer because it's under onboarding");
            ResetOOBFastAdvertisingTimer();
        }
    }

    PortTimerDelete(timer, 0);
}

STATIC_FUNCTION void OOBAdvFastIntervalTimeoutCallback(PortTimerHandle_t timer)
{
    TagPutPostWork(OOBAdvFastIntervalTimeoutHandler, timer);
}

TagError_t ResetOOBFastAdvertisingTimer(void)
{
    if (gTagContext->OOBAdvFastIntervalTimer)
    {
        PortTimerDelete(gTagContext->OOBAdvFastIntervalTimer, 0);
    }
    gTagContext->OOBAdvFastIntervalTimer = PortTimerCreate("OOBAdvFastIntervalTimeout",                                                    /* Text name. */
                                                           CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(T_OOB_ADVERTISE_FAST_INTERVAL_TIMEOUT_SEC)), /* Timer period. */
                                                           false,                                                                          /* Disable auto reload. */
                                                           0,                                                                              /* No USE */
                                                           OOBAdvFastIntervalTimeoutCallback);                                             /* The callback function. */

    if (!gTagContext->OOBAdvFastIntervalTimer)
    {
        TAG_LOG_E("Failed to create OOB fast interval advertising timer");
        return TAG_ERROR_MEM_ALLOC;
    }

    PortTimerStart(gTagContext->OOBAdvFastIntervalTimer, 0);
    return TAG_ERROR_NONE;
}

STATIC_FUNCTION void bootAdvFastIntervalTimeoutHandler(TagTaskWorkParam param)
{
    PortTimerHandle_t timer = param;

    /* Check timer is valid*/
    if (timer == gTagContext->bootAdvFastIntervalTimer)
    {
        TAG_LOG_D("boot fast interval advertising expired");
        gTagContext->bootAdvFastIntervalTimer = NULL;
        TagRefreshAdv();
    }

    PortTimerDelete(timer, 0);
}

STATIC_FUNCTION void bootAdvFastIntervalTimeoutCallback(PortTimerHandle_t timer)
{
    TagPutPostWork(bootAdvFastIntervalTimeoutHandler, timer);
}

TagError_t CreateBootFastAdvertisingOneShotTimer(void)
{
    if (gTagContext->bootAdvFastIntervalTimer)
    {
        PortTimerDelete(gTagContext->bootAdvFastIntervalTimer, 0);
    }
    gTagContext->bootAdvFastIntervalTimer = PortTimerCreate("BootAdvFastIntervalTimeout",                                                    /* Text name. */
                                                            CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(T_BOOT_ADVERTISE_FAST_INTERVAL_TIMEOUT_SEC)), /* Timer period. */
                                                            false,                                                                           /* Disable auto reload. */
                                                            0,                                                                               /* No USE */
                                                            bootAdvFastIntervalTimeoutCallback);                                             /* The callback function. */

    if (!gTagContext->bootAdvFastIntervalTimer)
    {
        TAG_LOG_E("Failed to create boot fast interval advertising Timer");
        return TAG_ERROR_MEM_ALLOC;
    }

    PortTimerStart(gTagContext->bootAdvFastIntervalTimer, 0);
    return TAG_ERROR_NONE;
}

STATIC_FUNCTION void batteryCheckTimerCallback(PortTimerHandle_t timer)
{
    static int count = 0;

    uint32_t batteryCapacity = 0;

    if (count >= 1)
    {
        if (PortTimerIsTimerActive(gTagContext->BatteryCheckTimer))
        {
            int isChanged = 0;

            PortTimerStop(gTagContext->BatteryCheckTimer, 0);
            PortTimerDelete(gTagContext->BatteryCheckTimer, 0);

            gTagContext->BatteryCheckTimer = NULL;

            initBattery = true;

            isChanged = TagUpdateBatteryLevel();

            if (isChanged == true)
            {
                TagRefreshAdv();
            }
            return;
        }
    }

    count++;

    PortBatteryGetLevel(&batteryCapacity);

    TAG_LOG_I("boot batCapacity: %u", batteryCapacity);
}

STATIC_FUNCTION void tagBleStart(void)
{
    TAG_LOG_I("tagBleStart");

    TagError_t ret = TAG_ERROR_NONE;

    /* Init Tag GATT services*/
    ret = TagBleInitGATTDB();
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to init gatt db (%d)", ret);
    }

    AuthServiceInit();

    if (gTagContext->state == TAG_STATE_OUT_OF_BOX)
    {
        ResetOOBFastAdvertisingTimer();
        gTagContext->onBoardingStatus = PRE_ONBOARDING;
        PortBleSetTxPower(PortBleTxPowerBootingAdvWeak, 0);
    }
    else
    {
        CreateBootFastAdvertisingOneShotTimer();
        gTagContext->onBoardingStatus = ONBOARDED;
        PortBleSetTxPower(PortBleTxPowerManual, gTagContext->txPower);
    }

    TagRefreshAdv();

#ifdef TAG_CONFIG_DEBUG_BLOCK_SLEEP_MODE
    TAG_LOG_D("Block entering sleep mode");
    PortSleepWakeLock(PORT_WAKELOCK_PERFORMANCE);
#endif

    TAG_LOG_I("BleEnd");
}

TagError_t InitTagContext(TagContext *tagContext)
{
    TAG_LOG_I("InitTagContext");

    uint32_t agingCounterIntervalSec;
    TagNVData_t nvData;
    TagError_t error;

    error = TagNVLoad(TAG_NV_PREMATURE_OFFLINE_TIMEOUT, &nvData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load TAG_NV_PREMATURE_OFFLINE_TIMEOUT");
        return TAG_ERROR_NV_INVALID_DATA;
    }
    tagContext->prematureOfflineTimeout = nvData.data.prematureOfflineTout;

    error = TagNVLoad(TAG_NV_OVERMATURE_OFFLINE_TIMEOUT, &nvData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load TAG_NV_OVERMATURE_OFFLINE_TIMEOUT");
        return TAG_ERROR_NV_INVALID_DATA;
    }
    tagContext->offlineTimeout = nvData.data.overmatureOfflineTout;

    tagContext->prematureTimer = PortTimerCreate("PrematureTimeout",                                                       /* Text name. */
                                                 CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(tagContext->prematureOfflineTimeout)), /* Timer period. */
                                                 false,                                                                    /* Disable auto reload. */
                                                 0,                                                                        /* No USE */
                                                 prematureTimeoutCallback);                                                /* The callback function. */

    tagContext->offlineTimer = PortTimerCreate("OfflineTimeout",                                 /* Text name. */
                                               CONVERT_SEC_TO_TICKS(tagContext->offlineTimeout), /* Timer period. */
                                               false,                                            /* Disable auto reload. */
                                               0,                                                /* No USE */
                                               offlineTimeoutCallback);                          /* The callback function. */

    tagContext->OOBCompletedTimer = PortTimerCreate("OOBCompletedTimeout",                                            /* Text name. */
                                                    CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(T_OOB_COMPLETED_TIMEOUT_SEC)), /* Timer period. */
                                                    false,                                                            /* Disable auto reload. */
                                                    0,                                                                /* No USE */
                                                    OOBCompletedTimeoutCallback);                                     /* The callback function. */

    agingCounterIntervalSec = AGING_COUNTER_INTERVAL;

    tagContext->agingCounterTimer = PortTimerCreate("AgingCounterTimer", CONVERT_SEC_TO_TICKS(agingCounterIntervalSec),
                                                    false, (void *)0, agingCounterTimerCallback);

    tagContext->requestAdvIntervalTimer = PortTimerCreate("RequestAdvIntervalTimeout",                                          /* Text name. */
                                                          CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(T_REQUEST_ADVERTISE_TIMEOUT_SEC)), /* Timer period. */
                                                          false,                                                                /* Disable auto reload. */
                                                          0,                                                                    /* ID is not used. */
                                                          requestAdvIntervalTimeoutCallback);                                   /* The callback function. */


    gTagContext->BatteryCheckTimer = PortTimerCreate("BatteryCheckTimer",                                                                /* Text name. */
                                                       CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(TAG_BATTERY_CHECK_TIMER_PERIOD_SEC)), /* Timer period. */
                                                       true,                                                                          /* Disable auto reload. */
                                                       0,                                                                              /* No USE */
                                                       batteryCheckTimerCallback);                                             /* The callback function. */

    return TAG_ERROR_NONE;
}

TagError_t LoadTagContextFromNV(TagContext *tagContext)
{
    const char *mnIdStr;
    const char *setupIdStr;
    TagNVData_t nvData;
    TagError_t error;
    char serialNumber[4] = {
        0,
    };
    TagDeviceInfoData_t *serialInfo = NULL;

#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
    error = TagNVLoad(TAG_NV_BUTTON_PUSH_ACTION, &nvData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load TAG_NV_BUTTON_PUSH_ACTION");
        return TAG_ERROR_NV_INVALID_DATA;
    }
    tagContext->pushButtonEnabled = nvData.data.buttonAction;

    error = TagNVLoad(TAG_NV_BUTTON_HOLD_ACTION, &nvData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load TAG_NV_BUTTON_HOLD_ACTION");
        return TAG_ERROR_NV_INVALID_DATA;
    }
    tagContext->holdButtonEnabled = nvData.data.buttonAction;
#endif // TAG_ACCESSORY_OPTION_BUTTON_ACTION

    error = TagNVLoad(TAG_NV_E2E_ENCRYPTION, &nvData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load TAG_NV_E2E_ENCRYPTION");
        return TAG_ERROR_NV_INVALID_DATA;
    }
    tagContext->E2EEFlag = nvData.data.e2eEncryption;

    error = TagNVLoad(TAG_NV_MAX_ALLOWED_BLE_CONNECTION, &nvData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load TAG_NV_MAX_ALLOWED_BLE_CONNECTION");
        return TAG_ERROR_NV_INVALID_DATA;
    }
    tagContext->maxAllowedConnections = nvData.data.maxAllowedConn;

    /* init tag state */
    error = TagNVLoad(TAG_NV_ONBOARDED, &nvData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load TAG_NV_ONBOARDED");
        return TAG_ERROR_NV_INVALID_DATA;
    }

    if (nvData.data.onboarded == TAG_ONBOARDED_VALUE_ONBOARDED)
    {
        tagContext->state = TAG_STATE_OFFLINE;
    }
    else
    {
        tagContext->state = TAG_STATE_OUT_OF_BOX;
    }

    /* init regionId */
    if (tagContext->state == TAG_STATE_OFFLINE)
    {
        error = TagNVLoad(TAG_NV_REGION, &nvData);
        if (error != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to load TAG_NV_REGION");
            return TAG_ERROR_NV_INVALID_DATA;
        }
        tagContext->regionID = nvData.data.region;
    }

    /* init activity mode(power saving mode) */
    error = TagNVLoad(TAG_NV_ACTIVITY_MODE, &nvData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load TAG_NV_ACTIVITY_MODE");
        return TAG_ERROR_NV_INVALID_DATA;
    }
    tagContext->tagPSM = nvData.data.activityMode;

    /* init Tx Power */
    error = TagNVLoad(TAG_NV_TX_POWER, &nvData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load TAG_NV_TX_POWER");
        return TAG_ERROR_NV_INVALID_DATA;
    }
    tagContext->txPower = nvData.data.txPower;


    nvData.data.manufacturerId = TagMalloc(TAG_NV_MANUFACTURER_ID_MAX_SZ + 1);
    if (nvData.data.manufacturerId == NULL)
    {
        TAG_LOG_E("Failed to alloc for manufacturerId");
        return TAG_ERROR_MEM_ALLOC;
    }

    memset(nvData.data.manufacturerId, 0, TAG_NV_MANUFACTURER_ID_MAX_SZ + 1);

    error = TagNVLoad(TAG_NV_MANUFACTURER_ID, &nvData);
    if (error != TAG_ERROR_NONE)
    {
#if !defined(TAG_CONFIG_USE_ONBOARD_CONF_HEADER)
        TAG_LOG_I("Failed to load TAG_NV_MANUFACTURER_ID, Try to get from header");
#endif
        TagFree(nvData.data.manufacturerId);

        /* Try to get mnId from TagOnboardingConfig again */
        mnIdStr = TagGetOnboardConfStrPtr(TAG_ONBOARD_CONF_MANUFACTURER_ID);
        if (!mnIdStr)
        {
            TAG_LOG_E("Failed to load mnId from TagOnboardingConfig");
            return TAG_ERROR_NV_INVALID_DATA;
        }
        tagContext->MNID = TagUtilConvertStringToInt(mnIdStr, (int)strlen(mnIdStr));
    }
    else
    {
        tagContext->MNID = TagUtilConvertStringToInt(nvData.data.manufacturerId, (int)strlen(nvData.data.manufacturerId));
        TagFree(nvData.data.manufacturerId);
    }

    nvData.data.setupId = TagMalloc(TAG_NV_SETUP_ID_MAX_SZ);
    if (nvData.data.setupId == NULL)
    {
        TAG_LOG_E("Failed to alloc for setupId");
        return TAG_ERROR_MEM_ALLOC;
    }

    memset(nvData.data.setupId, 0, TAG_NV_SETUP_ID_MAX_SZ);

    error = TagNVLoad(TAG_NV_SETUP_ID, &nvData);
    if (error != TAG_ERROR_NONE)
    {
#if !defined(TAG_CONFIG_USE_ONBOARD_CONF_HEADER)
        TAG_LOG_I("Failed to load TAG_NV_SETUP_ID from NV. Try to get from header");
#endif
        TagFree(nvData.data.setupId);

        /* Try to get setupId from TagOnboardingConfig again */
        setupIdStr = TagGetOnboardConfStrPtr(TAG_ONBOARD_CONF_SETUP_ID);
        if (!setupIdStr)
        {
            TAG_LOG_I("Failed to load setupId from TagOnboardingConfig");
            return TAG_ERROR_NV_INVALID_DATA;
        }
        tagContext->setupID = TagUtilConvertStringToInt(setupIdStr, (int)strlen(setupIdStr));
    }
    else
    {
        tagContext->setupID = TagUtilConvertStringToInt(nvData.data.setupId, 3);
        TagFree(nvData.data.setupId);
    }

    serialInfo = TagAllocDeviceInfo(TAG_DEVICE_INFO_SERIAL);
    if (serialInfo == NULL)
    {
        TAG_LOG_E("Failed to alloc TagDeviceInfo for serial");
        return TAG_ERROR_MEM_ALLOC;
    }

    strncpy(serialNumber, ((char *)serialInfo->data) + (serialInfo->dataLength - 4), 4);

    tagContext->serialNumber = TagUtilConvertStringToInt(serialNumber, 4);
    TagFreeDeviceInfo(serialInfo);

    tagContext->needAdvAddrChanged = 1;
    TagUpdateBatteryLevel();

    tagContext->initialized = 1;
    return TAG_ERROR_NONE;
}

bool IsTagStateOOB(TagContext *tagContext)
{
    if (tagContext->state == TAG_STATE_OUT_OF_BOX || tagContext->state == TAG_STATE_OOB_DEEP_SLEEP)
    {
        return true;
    }
    return false;
}

#define ADVTYPE_flag (1)
#define ADVTYPE_Incomplete16bitServiceList (2)
#define ADVTYPE_ServiceData16bit (22)
#define ADVTYPE_flag_BREDRNotSupported (1 << 2)

STATIC_FUNCTION PortBleAdvData *tagMakeAdvData(void)
{
    /* Advertising Data */
    static uint8_t tagAdvFlagData[TAG_ADVERTISING_FLAG_STRUCT_LENGTH];
    static uint8_t tagAdvServiceUUIDData[TAG_ADVERTISING_SERVICE_UUID_STRUCT_LENGTH];
    static uint8_t tagAdvServiceData[TAG_ADVERTISING_SERVICE_DATA_STRUCT_LENGTH];
    static PortBleAdvStruct tagAdvDataStruct[TAG_ADVERTISING_STRUCT_NUM] =
        {
            {TAG_ADVERTISING_FLAG_STRUCT_LENGTH + 1, ADVTYPE_flag, tagAdvFlagData},
            {TAG_ADVERTISING_SERVICE_UUID_STRUCT_LENGTH + 1, ADVTYPE_Incomplete16bitServiceList, tagAdvServiceUUIDData},
            {TAG_ADVERTISING_SERVICE_DATA_STRUCT_LENGTH + 1, ADVTYPE_ServiceData16bit, tagAdvServiceData}};
    static PortBleAdvData tagAdvertisingData = {TAG_ADVERTISING_STRUCT_NUM, tagAdvDataStruct};

    /* Refresh Advertising Flag Struct */
    writeLittleEndian(tagAdvFlagData, 0, ADVTYPE_flag_BREDRNotSupported, TAG_ADVERTISING_FLAG_STRUCT_LENGTH);

    if (gTagContext->state == TAG_STATE_OUT_OF_BOX)
    {
        /* Refresh Incomplete List of 16-bit Service Class UUIDs Struct */
        writeLittleEndian(tagAdvServiceUUIDData, 0, ONBOARDING_GATT_SERVICE_UUID, TAG_ADVERTISING_SERVICE_UUID_STRUCT_LENGTH);
        /* Refresh Service Data Struct */
        memset(tagAdvServiceData, '\0', TAG_ADVERTISING_SERVICE_DATA_STRUCT_LENGTH);
        writeLittleEndian(tagAdvServiceData, TAG_ADV_OOB_SERVICE_DATA_UUID_OFFSET,
                          ONBOARDING_GATT_SERVICE_UUID, TAG_ADV_OOB_SERVICE_DATA_UUID_LENGTH);
        writeLittleEndian(tagAdvServiceData, TAG_ADV_OOB_SERVICE_DATA_VERSION_OFFSET,
                          0x01, TAG_ADV_OOB_SERVICE_DATA_VERSION_LENGTH);
        writeLittleEndian(tagAdvServiceData, TAG_ADV_OOB_SERVICE_DATA_MNID_OFFSET,
                          gTagContext->MNID, TAG_ADV_OOB_SERVICE_DATA_MNID_LENGTH);
        writeLittleEndian(tagAdvServiceData, TAG_ADV_OOB_SERVICE_DATA_SETUPID_OFFSET,
                          gTagContext->setupID, TAG_ADV_OOB_SERVICE_DATA_SETUPID_LENGTH);
        /* UWB TAG always onboarding readiness flag is set */
        writeLittleEndian(tagAdvServiceData, TAG_ADV_OOB_SERVICE_DATA_READNESS_OFFSET,
                          ONBOARDING_READINESS_FLAG_ON, TAG_ADV_OOB_SERVICE_DATA_READNESS_LENGTH);
        writeLittleEndian(tagAdvServiceData, TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_LENGTH_OFFSET,
                          TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_LENGTH_VALUE, TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_LENGTH_LENGTH);
        writeLittleEndian(tagAdvServiceData, TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_TYPE_OFFSET,
                          TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_TYPE_VALUE, TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_TYPE_LENGTH);
        writeLittleEndian(tagAdvServiceData, TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_OFFSET,
                          gTagContext->serialNumber, TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_LENGTH);
    }
    else
    {
        uint8_t version;
        uint8_t regionId;
        TagError_t ret = 0;
        TagSecurityBuffer_t signatureSignKey = {0};
        TagSecurityBuffer_t signature = {0};

        /* Refresh Incomplete List of 16-bit Service Class UUIDs Struct */
        writeLittleEndian(tagAdvServiceUUIDData, 0, CONTROL_GATT_SERVICE_UUID, TAG_ADVERTISING_SERVICE_UUID_STRUCT_LENGTH);
        /* Refresh Service Data Struct */
        memset(tagAdvServiceData, '\0', TAG_ADVERTISING_SERVICE_DATA_STRUCT_LENGTH);
        writeLittleEndian(tagAdvServiceData, TAG_ADV_OTHER_STATES_SERVICE_DATA_UUID_OFFSET,
                          CONTROL_GATT_SERVICE_UUID, TAG_ADV_OTHER_STATES_SERVICE_DATA_UUID_LENGTH);
        if (gTagContext->state > TAG_STATE_CONNECTED)
        {
            /* 4 bits : version, 1 bit : advertisement type(Normal/Request),
             * 1 bit : 0(not connected), 2 bits : tag state */
            version = TAG_SERVICE_DATA_VERSION << TAG_SERVICE_DATA_VERSION_BIT_OFFSET |
                      gTagContext->requestAdvIntervalActiveFlag << TAG_ADV_TYPE_BIT_OFFSET |
                      (gTagContext->state - TAG_STATE_CONNECTED);
        }
        else
        {
            if (gTagContext->pairedConnection)
            {
                if (gTagContext->currentConnections >= gTagContext->maxAllowedConnections)
                {
                    /* 4 bits : version, 1 bit : advertisement type(Normal/Request),
                     * 1 bit : 1(connected), 2bits : number of connected devices  */
                    version = TAG_SERVICE_DATA_VERSION << TAG_SERVICE_DATA_VERSION_BIT_OFFSET |
                              gTagContext->requestAdvIntervalActiveFlag << TAG_ADV_TYPE_BIT_OFFSET |
                              1 << CONNECTED_BIT_OFFSET |
                              (gTagContext->currentConnections);
                }
                else
                {
                    /* 4 bits : version, 1 bit : advertisement type(Normal/Request),
                     * 3 bits : 0b100(a end user device is paired)  */
                    version = TAG_SERVICE_DATA_VERSION << TAG_SERVICE_DATA_VERSION_BIT_OFFSET |
                              gTagContext->requestAdvIntervalActiveFlag << TAG_ADV_TYPE_BIT_OFFSET |
                              0b100;
                }
            }
            else
            {
                /* 4 bits : version, 1 bit : advertisement type(Normal/Request),
                 * 1 bit : 1(connected), 2bits : number of connected devices  */
                version = TAG_SERVICE_DATA_VERSION << TAG_SERVICE_DATA_VERSION_BIT_OFFSET |
                          gTagContext->requestAdvIntervalActiveFlag << TAG_ADV_TYPE_BIT_OFFSET |
                          1 << CONNECTED_BIT_OFFSET |
                          (gTagContext->currentConnections);
            }
        }

        writeLittleEndian(tagAdvServiceData, TAG_ADV_OTHER_STATES_SERVICE_DATA_VERSION_OFFSET,
                          version, TAG_ADV_OTHER_STATES_SERVICE_DATA_VERSION_LENGTH);
        writeLittleEndian(tagAdvServiceData, TAG_ADV_OTHER_STATES_SERVICE_DATA_AGING_COUNTER_OFFSET,
                          gTagContext->agingCounter, TAG_ADV_OTHER_STATES_SERVICE_DATA_AGING_COUNTER_LENGTH);
        if (gTagContext->privacyIdLength == 0)
        {
            TagSecurityBuffer_t privacyIdBuf = {0};
            ret = TagAuthGetPrivacyId(gSecuContext, &privacyIdBuf);
            if (ret != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to create privacyID");
                return NULL;
            }
            gTagContext->privacyIdLength = privacyIdBuf.len;
            memcpy(gTagContext->privacyId, privacyIdBuf.p, privacyIdBuf.len);
            gTagContext->privacyIdLastUpdateCounter = gTagContext->agingCounter;
            TagCryptoSecurityBufferFree(&privacyIdBuf);
        }
        memcpy(&tagAdvServiceData[TAG_ADV_OTHER_STATES_SERVICE_DATA_PRIVACYID_OFFSET], gTagContext->privacyId, gTagContext->privacyIdLength);

        /* If battery level is unavailable, set battery info in adv data to MEDIUM(UNAVAILABLE) */
        if (gTagContext->batteryLevel == BATTERY_LEVEL_UNAVAILABLE)
        {
            regionId = (gTagContext->regionID & 0x0f) << 4 |
                       (gTagContext->E2EEFlag & 0x01) << 3 |
                       (TAG_UWB_SUPPORT) << 2 |
                       (BATTERY_LEVEL_MEDIUM & 0x03);
        }
        else
        {
            regionId = (gTagContext->regionID & 0x0f) << 4 |
                       (gTagContext->E2EEFlag & 0x01) << 3 |
                       (TAG_UWB_SUPPORT) << 2 |
                       (gTagContext->batteryLevel & 0x03);
        }
        writeLittleEndian(tagAdvServiceData, TAG_ADV_OTHER_STATES_SERVICE_DATA_REGIONID_OFFSET,
                          regionId, TAG_ADV_OTHER_STATES_SERVICE_DATA_REGIONID_LENGTH);

        ret = TagAuthGetTagKey(gSecuContext, TAG_SECURITY_NOUSE_DEVICE_ID,
                               TAG_SECURITY_TAG_KEY_TYPE_SK, &signatureSignKey);
        if (ret != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to create signature sign key");
            return NULL;
        }
        ret = TagCryptoAesFunction(signatureSignKey.p, signatureSignKey.len,
                                   gSecuContext->privacyIdIv.p, gSecuContext->privacyIdIv.len,
                                   TAG_SECURITY_KEY_TYPE_AES128, TAG_SECURITY_CIPHER_ENCRYPT,
                                   &tagAdvServiceData[TAG_ADV_OTHER_STATES_SERVICE_DATA_VERSION_OFFSET],
                                   TAG_ADV_OTHER_STATES_SERVICE_DATA_SIGNATURE_OFFSET - TAG_ADV_OTHER_STATES_SERVICE_DATA_VERSION_OFFSET, &signature);
        if (ret != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to create signature");
            TagCryptoSecurityBufferFree(&signatureSignKey);
            return NULL;
        }
        memcpy(&tagAdvServiceData[TAG_ADV_OTHER_STATES_SERVICE_DATA_SIGNATURE_OFFSET], signature.p, TAG_ADV_OTHER_STATES_SERVICE_DATA_SIGNATURE_LENGTH);
        TagCryptoSecurityBufferFree(&signatureSignKey);
        TagCryptoSecurityBufferFree(&signature);
    }

    return &tagAdvertisingData;
}

STATIC_FUNCTION uint16_t getTagAdvInterval(void)
{
    uint16_t advInterval = 0;

    // Set advertisement interval
    switch (gTagContext->state) {

    case TAG_STATE_OUT_OF_BOX:
        if (gTagContext->OOBAdvFastIntervalTimer)
        {
            advInterval = T_OOB_ADVERTISE_INTERVAL_FAST;
        }
        else
        {
            advInterval = T_OOB_ADVERTISE_INTERVAL_NORMAL;
        }
        break;

    case TAG_STATE_CONNECTED:
        if ((gTagContext->currentConnections < gTagContext->maxAllowedConnections) && gTagContext->OOBCompletedActiveFlag)
        {
            advInterval = T_OOB_ADVERTISE_INTERVAL_FAST;
        }
        else if (gTagContext->tagPSM > TAG_PSM_NORMAL)
        {
            advInterval = T_POWER_SAVE_ADVERTISE_INTERVAL;
        }
        else
        {
            advInterval = T_NORMAL_ADVERTISE_INTERVAL;
        }
        break;

    case TAG_STATE_OOB_DEEP_SLEEP:
    case TAG_STATE_PREMATURE_OFFLINE:
    case TAG_STATE_OFFLINE:
    case TAG_STATE_OVERMATURE_OFFLINE:
    default:
        if (gTagContext->OOBCompletedActiveFlag || gTagContext->bootAdvFastIntervalTimer)
        {
            advInterval = T_OOB_ADVERTISE_INTERVAL_FAST;
        }
        else if (gTagContext->requestAdvIntervalActiveFlag)
        {
            advInterval = T_REQUEST_ADVERTISE_INTERVAL;
        }
        else if (gTagContext->tagPSM > TAG_PSM_NORMAL)
        {
            advInterval = T_POWER_SAVE_ADVERTISE_INTERVAL;
        }
        else
        {
            advInterval = T_NORMAL_ADVERTISE_INTERVAL;
        }
        break;
    }

    return advInterval;
}

STATIC_FUNCTION PortBleAdvParams *tagMakeAdvParams(void)
{
    uint16_t advInterval = 0;
    static PortBleAdvParams advParms;

    // Set advertisement type
    if (gTagContext->currentConnections < gTagContext->maxAllowedConnections)
    {
        advParms.advertisingType = PortBleAdvConnectableUndirected;
    }
    else
    {
        advParms.advertisingType = PortBleAdvNonConnectable;
    }

    // Set advertisement address type
    if (gTagContext->state == TAG_STATE_OUT_OF_BOX)
    {
        advParms.ownAddressType = PortBleAdvAddrTypePublic;
    }
    else
    {
        advParms.ownAddressType = PortBleAdvAddrTypeRandom;
    }

    // Set tx power
    if (gTagContext->state == TAG_STATE_OUT_OF_BOX)
    {
        advParms.txPower = TAG_ADV_TX_POWER_OOB;
    }
    else if (gTagContext->state == TAG_STATE_CONNECTED)
    {
        advParms.txPower = TAG_ADV_TX_POWER_CONNECTED;
    }
    else
    {
        advParms.txPower = TAG_ADV_TX_POWER_DISCONNECTED;
    }

    advInterval = getTagAdvInterval();
    advParms.maxInterval = advParms.minInterval = CONVERT_BLE_ADV_INTERVAL(advInterval);

    advParms.needAddrChange = gTagContext->needAdvAddrChanged;

    TAG_LOG_I("advP: t(%d), ot(%d), p(%d), i(%ums), ac(%u)", advParms.advertisingType, advParms.ownAddressType, advParms.txPower, advInterval, advParms.needAddrChange);

    return &advParms;
}

TagError_t TagRefreshAdv(void)
{
    PortBleAdvData *bleAdvData = NULL;
    PortBleAdvParams *bleAdvParms = NULL;

    /* No Advertising */
    if ((gTagContext->state == TAG_STATE_CONNECTED && gTagContext->maxAllowedConnections == 1) ||
#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
        FwUpdateGetState() == FW_UPDATE_STATE_TRANSFER_IN_PROGRESS ||
#endif /* TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE */
        (gTagContext->state == TAG_STATE_OUT_OF_BOX && gTagContext->endUserDevices) ||
        gTagContext->state == TAG_STATE_OOB_DEEP_SLEEP ||
        gTagContext->factoryResetFlag)
    {
        PortBleStopAdv();
        gTagContext->needAdvAddrChanged = 1;
        return TAG_ERROR_NONE;
    }

    /* make Tag advertising data */
    bleAdvData = tagMakeAdvData();
    if (bleAdvData == NULL)
    {
        TAG_LOG_E("Failed to make advertising data");
        return TAG_ERROR_OPERATION_FAILURE;
    }

    /* make Tag advertising params */
    bleAdvParms = tagMakeAdvParams();
    if (bleAdvParms == NULL)
    {
        TAG_LOG_E("Failed to make advertising params");
        return TAG_ERROR_OPERATION_FAILURE;
    }

    PortBleStartAdv(bleAdvData, bleAdvParms);
    gTagContext->needAdvAddrChanged = 0;
    return TAG_ERROR_NONE;
}

void TagTransferState(TagState newState)
{
    if (gTagContext == NULL || gTagContext->prematureTimer == NULL ||
        gTagContext->offlineTimer == NULL)
    {
        TAG_LOG_E("Invalid parameters");
        return;
    }

    if (gTagContext->state == newState)
    {
        TAG_LOG_I("transfer same state");
        return;
    }

    /* stop all timers */
    PortTimerStop(gTagContext->prematureTimer, 0);
    PortTimerStop(gTagContext->offlineTimer, 0);

    if (gTagContext->state == TAG_STATE_OUT_OF_BOX && newState == TAG_STATE_CONNECTED)
    {
        PortBleSetTxPower(PortBleTxPowerOnboardedAdvStrong, 0);
        /* When state change from OOB to CONNECTED, update new battery info for refreshing */
        if (gTagContext->batteryLevel != BATTERY_LEVEL_UNAVAILABLE)
        {
            TagUpdateBatteryLevel();
        }
    }

    if (newState == TAG_STATE_PREMATURE_OFFLINE)
    {
        PortTimerReset(gTagContext->prematureTimer, 0);
    }
    else if (newState == TAG_STATE_OFFLINE)
    {
        PortTimerReset(gTagContext->offlineTimer, 0);
    }
    else if (newState == TAG_STATE_OVERMATURE_OFFLINE)
    {
       /* nothing */
    }
    else if (newState == TAG_STATE_CONNECTED)
    {
        if (TagSoundIsConnectingPlaying())
        {
            TAG_LOG_D("Stop connecting sound");
            TagSoundPlayStop();
        }
    }

    TAG_MEM_CHECK("State change");
    if ((gTagContext->offlineTimeout == OFFLINE_TIMEOUT_PERIOD) &&
        (gTagContext->prematureOfflineTimeout == PREMATURE_TIMEOUT_PERIOD))
    {
        TAG_LOG_I("state:%d->%d", gTagContext->state, newState);
    }
    else
    {
        TAG_LOG_I("state:%d->%d,TOut P:%d,O:%d",
                  gTagContext->state, newState, gTagContext->prematureOfflineTimeout, gTagContext->offlineTimeout);
    }

    gTagContext->state = newState;
}

void TagCreateBatteryCheckTimer(void)
{
    gTagContext->BatteryCheckTimer = PortTimerCreate("BatteryCheckTimer",                                                                /* Text name. */
                                                       CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(TAG_BATTERY_CHECK_TIMER_PERIOD_SEC)), /* Timer period. */
                                                       true,                                                                          /* Disable auto reload. */
                                                       0,                                                                              /* No USE */
                                                       batteryCheckTimerCallback);
}

TagError_t TagBatteryInit(void)
{
    PortTimerStart(gTagContext->BatteryCheckTimer, 0);

    return TAG_ERROR_NONE;
}

bool TagUpdateBatteryLevel(void)
{
    uint8_t batteryLevel;
    bool ret = false;

    uint32_t batteryCapacity = 0;

    if (initBattery == false) {
        TAG_LOG_I("Bat is checking");
        return false;
    }

    PortBatteryGetLevel(&batteryCapacity);

    if (batteryCapacity <= BATTERY_LEVEL_VERY_LOW_PERCENT)
    {
        batteryLevel = BATTERY_LEVEL_VERY_LOW;
    }
    else if (batteryCapacity <= BATTERY_LEVEL_LOW_PERCENT)
    {
        batteryLevel = BATTERY_LEVEL_LOW;
    }
    else if (batteryCapacity <= BATTERY_LEVEL_MEDIUM_PERCENT)
    {
        batteryLevel = BATTERY_LEVEL_MEDIUM;
    }
    else
    {
        batteryLevel = BATTERY_LEVEL_FULL;
    }

    if (gTagContext->batteryLevel != batteryLevel)
    {
        TAG_LOG_I("bat change %d->%d", gTagContext->batteryLevel, batteryLevel);

        ret = true;
    }
    gTagContext->batteryLevel = batteryLevel;

    TAG_LOG_I("bat lvl: %u", batteryLevel);

    return ret;
}

TagError_t TagPutPostWork(TagTaskWorkHandler handler, TagTaskWorkParam param)
{
    TagTaskWork *work;

    work = TagMalloc(sizeof(TagTaskWork));
    if (work == NULL)
    {
        TAG_LOG_E("Failed to allocate memory for Tag work");
        return TAG_ERROR_MEM_ALLOC;
    }
    memset(work, '\0', sizeof(TagTaskWork));
    work->handler = handler;
    work->param = param;
    work->next = NULL;

    if (gTagContext->taskWorkHead == NULL && gTagContext->taskWorkTail == NULL)
    {
        gTagContext->taskWorkHead = gTagContext->taskWorkTail = work;
    }
    else
    {
        gTagContext->taskWorkTail->next = work;
        gTagContext->taskWorkTail = work;
    }

    PortEventGroupSetBits(gTagContext->eventGroup, TAG_TASK_EVENT_SIGNAL);

    return TAG_ERROR_NONE;
}

#ifdef TAG_CONFIG_FACTORY_RESET_IN_SDK
#define MANUAL_FACTORY_RESET_BUTTON_TIMEOUT 5000     /* 5 seconds manual factory device button reset */
#define MANUAL_FACTORY_RESET_BUTTON_CHECK_PERIOD 200 /* 200ms period button check */
#endif

STATIC_FUNCTION void tagMainTask(void *pvParameters)
{
    TAG_LOG_D("tagMainTask");

    TagError_t ret;
    uint64_t utcTime = 0;
    unsigned int savedAgingCounter = 0;

    PortPrepareMainTask();

#ifdef TAG_CONFIG_FACTORY_RESET_IN_SDK
    /* Check Manual Factory Reset. In case of Wireless OTA, skip this routine */
    if (PortButtonIsPressed() &&
        (gTagContext->bootReason == TAG_BOOT_REASON_INIT || gTagContext->bootReason == TAG_BOOT_REASON_NORMAL))
    {
        int factoryResetRemainTime = MANUAL_FACTORY_RESET_BUTTON_TIMEOUT;
        while (factoryResetRemainTime > 0)
        {
            factoryResetRemainTime -= MANUAL_FACTORY_RESET_BUTTON_CHECK_PERIOD;
            PortTaskDelay(CONV_MS_TO_TICKS(MANUAL_FACTORY_RESET_BUTTON_CHECK_PERIOD));
            if (!PortButtonIsPressed() || factoryResetRemainTime <= 0)
            {
                break;
            }
        }
        if (factoryResetRemainTime <= 0)
        {
            ret = TagFactoryReset(0);
            if (ret != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to execute Factory Reset");
            }
        }
    }
#endif

    PortSleepInit();

#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
    /* Execute UWB image update (if necessary) before BLE gets started in order to avoid bad user experience
       that during UWB image update the user may feel strange why there is BLE adv/connection but the tag does not respond to the owner handset. */
    PortUwbSwup();
#endif

    ret = LoadTagContextFromNV(gTagContext);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load tag context from NV %d", ret);
        CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_SYSTEM_CRITICAL),
                        SOUND_NOT_PLAYED, "Failed to play CRITICAL sound");
        goto tag_task_loop;
    }

    utcTime = PortTimeGetRtcTime();
    ret = TagNVGetAgingCnt(&savedAgingCounter);
    TAG_LOG_D("booting time info RTC=%u, NV agingCounter=%u, NV result=%d", (uint32_t)utcTime, savedAgingCounter, ret);
    if (utcTime > savedAgingCounter * AGING_COUNTER_INTERVAL + AGING_COUNTER_NORM_UTC)
    {
        uint64_t remainTimeForNextCounter;
        gTagContext->agingCounter = (utcTime - AGING_COUNTER_NORM_UTC) / AGING_COUNTER_INTERVAL;
        remainTimeForNextCounter = (gTagContext->agingCounter + 1) * AGING_COUNTER_INTERVAL + AGING_COUNTER_NORM_UTC - utcTime;
        TAG_LOG_I("RTC=%u, remain time for next counter %u", (uint32_t)utcTime, (uint32_t)remainTimeForNextCounter);
        PortTimerChangePeriod(gTagContext->agingCounterTimer, CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(remainTimeForNextCounter)), 0);
    }
    else
    {
        gTagContext->agingCounter = savedAgingCounter;
        PortTimeSetRtcTime(gTagContext->agingCounter * AGING_COUNTER_INTERVAL + AGING_COUNTER_NORM_UTC);
    }

    /* In OOB state, don't use aging counter */
    if (gTagContext->state == TAG_STATE_OUT_OF_BOX)
    {
        PortTimerStop(gTagContext->agingCounterTimer, 0);
    }
    else
    {
        PortTimerStart(gTagContext->agingCounterTimer, 0);
    }
    if (gTagContext->state == TAG_STATE_OFFLINE)
    {
        PortTimerStart(gTagContext->offlineTimer, 0);
    }

    tagBleStart();

#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
    ret = PortUwbStartMgr();
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("PortUwbStartMgr() Failed %d", ret);
    }
#endif

    if (PortSystemIsColdBoot() || gTagContext->bootReason != TAG_BOOT_REASON_INIT)
    {
        if (gTagContext->bootReason == TAG_BOOT_REASON_FACTORY_RESET)
        {
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_RESET_BOOTING), SOUND_NOT_PLAYED, "Fail RstSound");
        }
        else if (gTagContext->bootReason == TAG_BOOT_REASON_SILENT_RESET)
        {
            /* disable booting sound */
        }
        else
        {
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_BOOTING), SOUND_NOT_PLAYED, "Fail bootSound");
        }
    }

tag_task_loop:
    while (!gTagContext->mainTaskSoftKillSignal)
    {
        PortEventGroupWaitBits(gTagContext->eventGroup, TAG_TASK_EVENT_SIGNAL, true, false, PORT_MAX_DELAY);
        if (gTagContext->taskWorkHead != NULL)
        {
            TagTaskWork *work = gTagContext->taskWorkHead;
            work->handler(work->param);
            if (gTagContext->taskWorkHead == gTagContext->taskWorkTail)
            {
                gTagContext->taskWorkHead = NULL;
                gTagContext->taskWorkTail = NULL;
            }
            else
            {
                gTagContext->taskWorkHead = gTagContext->taskWorkHead->next;
            }
            TagFree(work);
            PortEventGroupSetBits(gTagContext->eventGroup, TAG_TASK_EVENT_SIGNAL);
        }
    }

    // task cleanup
    while (gTagContext->taskWorkHead)
    {
        TagFree(gTagContext->taskWorkHead);
        gTagContext->taskWorkHead = gTagContext->taskWorkHead->next;
    }

    PortTaskDelete(NULL);
}

TagResult_t TagInit(void)
{
    TagContext *tagContext = NULL;
    TagError_t ret;
    uint8_t bootReason = TAG_BOOT_REASON_INIT;
    TagNVData_t nvData;

    TAG_LOG_I("SDKVER=%s, FWVER=%s, Compiled at %s %s", TAGSDK_VERSION_STRING, DEVICE_FW_VERSION_STRING, __DATE__, __TIME__);

    tagContext = TagMalloc(sizeof(TagContext));
    if (tagContext == NULL)
    {
        TAG_LOG_E("Failed to alloc memory for Tag context");
        return TAG_RESULT_MEMORY_FAIL;
    }
    gTagContext = tagContext;
    memset(tagContext, '\0', sizeof(TagContext));
    gTagContext->taskWorkHead = NULL;
    gTagContext->taskWorkTail = NULL;

    ret = TagNVInit();
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to init NV");
        TagFree(tagContext);
        gTagContext = NULL;
        return TAG_RESULT_UNDEFINED_FAIL;
    }

    if (TagSoundPlayerInit() != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to init SoundPlayer");
    }
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
    if (TagLedBlinkCtrlInit() != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to init LedBlinking");
    }
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

    ret = InitTagContext(tagContext);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to init tagContext %u", ret);
        TagFree(tagContext);
        gTagContext = NULL;
        return TAG_RESULT_UNDEFINED_FAIL;
    }
    /* Load boot reason */
    ret = TagNVLoad(TAG_NV_BOOT_REASON, &nvData);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load TAG_NV_BOOT_REASON");
    }
    else
    {
        bootReason = nvData.data.bootReason;
        gTagContext->bootReason = bootReason;
        nvData.dataLength = 1;

        if (bootReason != TAG_BOOT_REASON_INIT)
        {
            nvData.data.bootReason = TAG_BOOT_REASON_INIT;
            ret = TagNVStore(TAG_NV_BOOT_REASON, &nvData);
            if (ret != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to recover boot reason");
            }
        }
    }
    TAG_LOG_D("Boot Reason : %d", bootReason);
    /* Load saved RTC from NV and set */
    ret = TagNVLoad(TAG_NV_SAVED_RTC, &nvData);
    if (ret == TAG_ERROR_NONE && nvData.data.savedRTC > AGING_COUNTER_NORM_UTC)
    {
        PortTimeSetRtcTime(nvData.data.savedRTC);
        TAG_LOG_I("Set time to %u", (uint32_t)nvData.data.savedRTC);

        nvData.dataLength = 8;
        nvData.data.savedRTC = 0;
        ret = TagNVStore(TAG_NV_SAVED_RTC, &nvData);
        if (ret != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to save RTC value");
        }
    }
    TagBatteryInit();

    TagCryptoInit();

#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
    FwUpdateInit();
#endif /* TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE */

#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
    ret = PortUwbInit();
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to init uwb");
        return TAG_RESULT_UNDEFINED_FAIL;
    }
#endif

    ret = PortBleInit();
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to init ble");
        return TAG_RESULT_UNDEFINED_FAIL;
    }

    ret = PortButtonInit();
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to init button");
        return TAG_RESULT_UNDEFINED_FAIL;
    }

    ret = PortTimeInit();
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to init time");
        return TAG_RESULT_UNDEFINED_FAIL;
    }

#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
    ret = TagNFCInit();
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to init nfc");
        return TAG_RESULT_UNDEFINED_FAIL;
    }
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */

    TAG_LOG_D("TagInit End");

    return TAG_RESULT_SUCCESS;
}

TagResult_t TagCleanup(void)
{
    TagError_t ret = TagFactoryReset(0);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to execute Factory Reset %u", ret);
        if (ret == TAG_ERROR_NV_FACTORY_RESET_FAIL || ret == TAG_ERROR_NV_NOT_INITIALIZED)
        {
            return TAG_RESULT_NV_LOAD_FAIL;
        }
        else
        {
            return TAG_RESULT_UNDEFINED_FAIL;
        }
    }

    return TAG_RESULT_SUCCESS;
}

EndUserDevice *TagFindEndUserDevice(TagBleDeviceId deviceId)
{
    EndUserDevice *endUserDevice = gTagContext->endUserDevices;

    while (endUserDevice)
    {
        if (endUserDevice->deviceId == deviceId)
        {
            break;
        }
        endUserDevice = endUserDevice->next;
    }

    return endUserDevice;
}

EndUserDevice *TagFindEndUserDeviceFromPortHandle(PortBleConnInfo *portConnHandle)
{
    EndUserDevice *endUserDevice = gTagContext->endUserDevices;

    while (endUserDevice)
    {
        if (PortBleIsEqualConnHandle(&endUserDevice->portConnHandle, portConnHandle))
        {
            break;
        }
        endUserDevice = endUserDevice->next;
    }

    return endUserDevice;
}

TagError_t TagFactoryReset(uint32_t delay_ms)
{
    TagError_t ret = TAG_ERROR_NONE;
    EndUserDevice *iter = gTagContext->endUserDevices;

    /* Delay before factory reset */
    gTagContext->factoryResetFlag = 1;

    PortTaskDelay(CONV_MS_TO_TICKS(delay_ms));

    /* Disconnect all connections before factory reset */
    while (iter)
    {
        PortBleGapDisconnect(&iter->portConnHandle);
        iter = iter->next;
    }

    PortTaskDelay(CONV_MS_TO_TICKS(delay_ms));

    PortBleGapRemoveOtherBondings(NULL);

#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
    ret = TagNFCSetLostMessageURL(NULL, 0);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("NFC reset failed");
    }
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */

    ret = TagNVFactoryReset();
    if (ret == TAG_ERROR_NONE)
    {
        TagUtilSystemReset(TAG_BOOT_REASON_FACTORY_RESET, "Factory Reset");
    }

    return ret;
}

TagError_t UpdateConnectionParameters(EndUserDevice *endUserDevice,
                                      uint16_t intervalMin, uint16_t intervalMax, uint16_t slaveLatency, uint16_t timeoutMultiplier)
{
    TagError_t ret = TAG_ERROR_NONE;

    /* If connectionParamIntervalMin is set, it means that there is another update negotiating */
    if (endUserDevice->connectionParamIntervalMin != 0)
    {
        endUserDevice->connectionParamIntervalMin = intervalMin;
        endUserDevice->connectionParamIntervalMax = intervalMax;
        endUserDevice->connectionParamSlaveLatency = slaveLatency;
        endUserDevice->connectionParamTimeoutMultiplier = timeoutMultiplier;
        endUserDevice->connectionParamUpdateRetryCount = CONNECTION_PARAM_UPDATE_RETRY_COUNT_MAX;
    }
    else
    {
        ret = PortBleRequestConnectionParameters(&endUserDevice->portConnHandle, intervalMin, intervalMax, slaveLatency, timeoutMultiplier);
        if (ret != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to update connection params ret=%d", ret);
            return ret;
        }
        else
        {
            endUserDevice->connectionParamIntervalMin = intervalMin;
            endUserDevice->connectionParamIntervalMax = intervalMax;
            endUserDevice->connectionParamSlaveLatency = slaveLatency;
            endUserDevice->connectionParamTimeoutMultiplier = timeoutMultiplier;
            endUserDevice->connectionParamUpdateRetryCount = CONNECTION_PARAM_UPDATE_RETRY_COUNT_MAX;
        }
    }

    return ret;
}

TagError_t PrepareFwUpdate(TagBleDeviceId peerDeviceId)
{
    EndUserDevice *endUserDevice = gTagContext->endUserDevices;
    EndUserDevice *fwUpdateEndUserDevice = NULL;
    TagError_t ret = TAG_ERROR_NONE;

    ret = PortBleStopAdv();
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to execute PortBleStopAdv %d", ret);
    }
    else
    {
        gTagContext->needAdvAddrChanged = 1;
    }

    /* disconnect other end user devices */
    while (endUserDevice)
    {
        if (endUserDevice->deviceId != peerDeviceId)
        {
            ret = PortBleGapDisconnect(&endUserDevice->portConnHandle);
            if (ret != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to excute PortBleStopAdv %d", ret);
            }
        }
        else
        {
            fwUpdateEndUserDevice = endUserDevice;
        }
        endUserDevice = endUserDevice->next;
    }

    if (!fwUpdateEndUserDevice)
    {
        TAG_LOG_E("Failed to find fwUpdateEndUserDevice");
        return TAG_ERROR_OPERATION_FAILURE;
    }

    /* Speed up data transfer */
    TAG_LOG_I("Set firmware update ParamUpdate %d~%d Lat:%d TO:%d",
              fwUpdateEndUserDevice->lastConnectionParamIntervalMin, fwUpdateEndUserDevice->lastConnectionParamIntervalMax,
              fwUpdateEndUserDevice->lastConnectionParamSlaveLatency, fwUpdateEndUserDevice->lastConnectionParamTimeoutMultiplier);
    UpdateConnectionParameters(fwUpdateEndUserDevice,
                               fwUpdateEndUserDevice->lastConnectionParamIntervalMin,
                               fwUpdateEndUserDevice->lastConnectionParamIntervalMax,
                               fwUpdateEndUserDevice->lastConnectionParamSlaveLatency,
                               fwUpdateEndUserDevice->lastConnectionParamTimeoutMultiplier);

    return TAG_ERROR_NONE;
}

TagResult_t TagStart(void)
{
    TagError_t ret = 0;
    if (gTagContext == NULL)
    {
        TAG_LOG_E("Tag is not initialized yet!");
        return TAG_RESULT_UNDEFINED_FAIL;
    }
    gTagContext->batteryLevel = BATTERY_LEVEL_UNAVAILABLE;
    gTagContext->mainTaskSoftKillSignal = 0;
    gTagContext->eventGroup = PortEventGroupCreate();
    ret = PortTaskCreate(tagMainTask, TAG_MAIN_TASK_NAME, TAG_MAIN_TASK_STACK_SIZE, NULL, TAG_MAIN_TASK_PRIORITY, &gTagContext->mainTaskHandle);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to create TAG main task");
        if (ret == TAG_ERROR_MEM_ALLOC)
        {
            return TAG_RESULT_MEMORY_FAIL;
        }
        else
        {
            return TAG_RESULT_UNDEFINED_FAIL;
        }
    }

    return TAG_RESULT_SUCCESS;
}
