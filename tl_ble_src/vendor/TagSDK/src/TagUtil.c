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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "TagConfig.h"

#include "TagCore.h"
#include "TagFwUpdate.h"
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
#include "TagLedBlink.h"
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
#include "TagNFC.h"
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */
#include "TagNV.h"
#include "TagUtil.h"
#include "TagSecurity.h"
#include "TagSoundPlayer.h"

#include "PortBle.h"
#include "PortOs.h"
#include "PortSystem.h"
#include "PortTime.h"
#ifdef TAG_CONFIG_WIRELESS_FW_UPDATE_IN_SDK
#include "PortTime.h"
#endif /* TAG_CONFIG_WIRELESS_FW_UPDATE_IN_SDK */
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
#include "PortUwb.h"
#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

#define TAG_UTIL_RESET_DELAY_MS (150)

#ifdef TAG_CONFIG_DEBUG_PRINT_MEMORY_DUMP
static bool isPrint(char c)
{
    if (c >= '!' && c <= '~')
        return true;
    return false;
}

TagError_t TagUtilDumpMem(char *tag, uint8_t *buf, uint16_t len)
{
    const char *space = " ";
    int i, j;
    int width = 16;
    char lineBuf[76] = ""; // 76 = 16 * (3+1) + 8(%p) + 3 + 1(null);
    int offset = 0;

    if (buf == NULL || len == 0)
    {
        return TAG_ERROR_INVALID_ARG;
    }

    if (tag)
    {
        TAG_LOG_E("Dump '%s' (%d)", tag, len);
    }

    for (i = 0; i < len; i += width)
    {
        memset(lineBuf, 0, sizeof(lineBuf));
        offset = 0;
        offset += snprintf(lineBuf + offset, sizeof(lineBuf) - offset, "[%08x] ", (unsigned int)(buf + i));

        for (j = i; j < (i + width); j++)
        {
            if (j < len)
            {
                offset += snprintf(lineBuf + offset, sizeof(lineBuf) - offset, "%02x%s", buf[j], space);
            }
            else
            {
                offset += snprintf(lineBuf + offset, sizeof(lineBuf) - offset, "   ");
            }
        }

        for (j = i; j < (i + width); j++)
        {
            if (isPrint(buf[j]))
            {
                offset += snprintf(lineBuf + offset, sizeof(lineBuf) - offset, "%c", buf[j]);
            }
            else
            {
                offset += snprintf(lineBuf + offset, sizeof(lineBuf) - offset, "%c", '.');
            }

            if (j >= len)
            {
                break;
            }
        }
        TAG_LOG_E("%s", lineBuf);
    }
    return TAG_ERROR_NONE;
}
#endif

uint16_t TagUtilCrc16(uint8_t *pData, uint16_t lenData, uint16_t crcValueOld)
{
    uint8_t i = 0;
    uint8_t *data = pData;

    if (data == NULL)
    {
        return 0;
    }

    while (lenData--)
    {
        crcValueOld ^= *data++;

        for (i = 0; i < 8; i++)
        {
            if (crcValueOld & 1)
            {
                crcValueOld = (crcValueOld >> 1) ^ 0x8408;
            }
            else
            {
                crcValueOld = crcValueOld >> 1;
            }
        }
    }

    return crcValueOld;
}

static bool isHexDigit(char c)
{
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
        return true;
    return false;
}

TagError_t TagUtilConvertHexToString(unsigned char *dst, size_t dstSize,
                                     unsigned char *src, size_t srcSize, size_t *outlen)
{
    char tmp[3] = {0};
    int i, j;
    int val;
    TagError_t tagError = TAG_ERROR_NONE;

    if (!src || !dst || (dstSize < srcSize / 2) || srcSize % 2)
    {
        TAG_LOG_E("Invalid args");
        tagError = TAG_ERROR_INVALID_ARG;
        return tagError;
    }

    for (i = 0; i < srcSize; i++)
    {
        if (!isHexDigit(src[i]))
        {
            TAG_LOG_E("Wrong value");
            return TAG_ERROR_INVALID_ARG;
        }
    }

    for (i = 0, j = 0; i < srcSize - 1; i += 2, j++)
    {
        memcpy(tmp, src + i, 2);
        tmp[2] = 0;
        val = (unsigned char)strtol((const char *)tmp, NULL, 16);
        dst[j] = val;
    }
    if (outlen)
    {
        *outlen = j;
    }
    return tagError;
}

int TagUtilConvertStringToInt(const char *src, int len)
{
    int i;
    int maxLen = sizeof(int);
    int val = 0;

    if (len < maxLen)
    {
        maxLen = len;
    }

    for (i = 0; i < maxLen; i++)
    {
        val = val + (src[i] << (8 * i));
    }

    return val;
}

char *TagUtilConvertIntToString(int val, char *dst, int len)
{
    int i;
    int maxLen = sizeof(int);
    memset(dst, 0, len);

    if (len < maxLen)
    {
        maxLen = len;
    }

    for (i = 0; i < maxLen; i++)
    {
        dst[i] = (val >> (8 * i)) & 0xff;
    }
    dst[maxLen] = 0;
    return dst;
}

TagError_t TagUtilSystemResetTagContext()
{
    TAG_LOG_I("ResetTagContext");

#if defined(__APP_BLE_INDICATION_SINGLE_QUEUE__)
    if (gTagContext->indicationQueue)
    {
        TagControlFreeQueuedIndicationData(gTagContext->indicationQueue);
        PortQueueDelete(gTagContext->indicationQueue);
    }
#endif

    gTagContext->onBoardingStatus = PRE_ONBOARDING;
    gTagContext->serialNumber = 0; // Load NV
    gTagContext->setupID = 0; // Load NV
    gTagContext->MNID = 0; // Load NV

    gTagContext->txPower = 0; // Load NV
    gTagContext->tagPSM = TAG_PSM_NORMAL; // Load NV

    gTagContext->factoryResetFlag = 0;
    gTagContext->requestAdvIntervalActiveFlag = 1;

    if (gTagContext->requestAdvIntervalTimer)
    {
        // Reset Timer ID
        PortTimerSetTimerId(gTagContext->requestAdvIntervalTimer, 0);
    }

    if (gTagContext->bootAdvFastIntervalTimer)
    {
        // Delete
        PortTimerSetTimerId(gTagContext->bootAdvFastIntervalTimer, NULL);
        PortTimerDelete(gTagContext->bootAdvFastIntervalTimer, 0);
    }

    if (gTagContext->OOBAdvFastIntervalTimer)
    {
        // Delete
        PortTimerSetTimerId(gTagContext->OOBAdvFastIntervalTimer, NULL);
        PortTimerDelete(gTagContext->OOBAdvFastIntervalTimer, 0);
    }

    gTagContext->OOBCompletedActiveFlag = 0;

    if (gTagContext->OOBCompletedTimer)
    {
        // Stop
        PortTimerStop(gTagContext->OOBCompletedTimer, 0);
    }

    // Delete EndUserDevice
    while (gTagContext->endUserDevices != NULL)
    {
        EndUserDevice* endUserDevice = gTagContext->endUserDevices;

#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
        if (endUserDevice->logParam)
        {
            TagLogParamDeinit(endUserDevice->logParam);
        }
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */

        if (endUserDevice->indicationQueue)
        {
            TagControlFreeQueuedIndicationData(endUserDevice->indicationQueue);
            PortQueueDelete(endUserDevice->indicationQueue);
        }

        if (endUserDevice->authParam)
        {
            TagAuthConnectionDeinit(endUserDevice->authParam);
            endUserDevice->authParam = NULL;
        }

        if (endUserDevice->unknownBleConnectionTimer)
        {
            PortTimerSetTimerId(endUserDevice->unknownBleConnectionTimer, NULL);
            PortTimerDelete(endUserDevice->unknownBleConnectionTimer, 0);
            endUserDevice->unknownBleConnectionTimer = NULL;
        }

        if (endUserDevice->paringRequestTimer)
        {
            PortTimerSetTimerId(endUserDevice->paringRequestTimer, NULL);
            PortTimerDelete(endUserDevice->paringRequestTimer, 0);
            endUserDevice->paringRequestTimer = NULL;
        }

        if (endUserDevice->connectionParmRetryTimer)
        {
            PortTimerSetTimerId(endUserDevice->connectionParmRetryTimer, NULL);
            PortTimerDelete(endUserDevice->connectionParmRetryTimer, 0);
            endUserDevice->connectionParmRetryTimer = NULL;
        }

        if (endUserDevice->initConnectionParmRecoveryTimer)
        {
            PortTimerSetTimerId(endUserDevice->initConnectionParmRecoveryTimer, NULL);
            PortTimerDelete(endUserDevice->initConnectionParmRecoveryTimer, 0);
            endUserDevice->initConnectionParmRecoveryTimer = NULL;
        }

        if (endUserDevice->connectionParmRecoveryTimer)
        {
            PortTimerSetTimerId(endUserDevice->connectionParmRecoveryTimer, NULL);
            PortTimerDelete(endUserDevice->connectionParmRecoveryTimer, 0);
            endUserDevice->connectionParmRecoveryTimer = NULL;
        }

        PortBleDestroyConnHandle(&endUserDevice->portConnHandle);

        gTagContext->endUserDevices = gTagContext->endUserDevices->next;

        TagFree(endUserDevice);
    }

    gTagContext->advParamUpdated = 0;
    gTagContext->needAdvAddrChanged = 0; // Load NV
    // gTagContext->offlineTimeout = 0; // Init Tag Context
    // gTagContext->prematureOfflineTimeout = 0; // Init Init Tag Context
    gTagContext->overmaturePrivacyIdInterval = 0;
    gTagContext->holdButtonEnabled = 0; // Load NV
    gTagContext->pushButtonEnabled = 0; // Load NV
    gTagContext->pairedConnection = 0;
    gTagContext->nonOwnerConnections = 0;
    gTagContext->currentConnections = 0;
    gTagContext->maxAllowedConnections = 0; // Load NV
    gTagContext->privacyIdLastUpdateCounter = 0;
    memset(gTagContext->privacyId, 0, sizeof(gTagContext->privacyId));
    gTagContext->privacyIdLength = 0;
    gTagContext->numberOfPrivacyId = 0;
    gTagContext->batteryLevel = BATTERY_LEVEL_UNAVAILABLE;
    gTagContext->E2EEFlag = 0; // Load NV
    gTagContext->regionID = 0; // Load NV
    // gTagContext->agingCounter = 0;
    gTagContext->state = TAG_STATE_OUT_OF_BOX; // Load NV

    gTagContext->mainTaskSoftKillSignal = 0;
    gTagContext->initialized = 0; // Load NV

    TAG_LOG_D("ResetTagContext finished!");

    return TAG_ERROR_NONE;
}

static void systemResetWithoutReboot(TagBootReason reason, const char *debug_info)
{
    TagNVData_t nvData;
    TagError_t ret = 0;

    if (debug_info != NULL)
    {
        TAG_LOG_I("ResetWithoutReboot %u(%s)", reason, debug_info);
    }
    else
    {
        TAG_LOG_I("ResetWithoutReboot %u", reason);
    }

    TAG_MEM_CHECK("SystemReset");

    TagNVDeinit();

    // Tag Init
    ret = TagNVInit();
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to init NV");
        TagFree(gTagContext);
        gTagContext = NULL;
        return;
    }

    // Reset Tag Context
    TagUtilSystemResetTagContext();

    // Reset Tag Sound Player
    if (TagSoundPlayerReset() != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to reset SoundPlayer");
    }

    // Reset Tag LED
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
    TagLedBlinkCtrlReset();
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

    ret = TagNVLoad(TAG_NV_PREMATURE_OFFLINE_TIMEOUT, &nvData);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load TAG_NV_PREMATURE_OFFLINE_TIMEOUT");
        return;
    }
    gTagContext->prematureOfflineTimeout = nvData.data.prematureOfflineTout;

    PortTimerChangePeriod(gTagContext->prematureTimer, CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(gTagContext->prematureOfflineTimeout)), 0);
    /* This function is called in CONNECTED state, So stop timer after change period */
    PortTimerStop(gTagContext->prematureTimer, 0);

    ret = TagNVLoad(TAG_NV_OVERMATURE_OFFLINE_TIMEOUT, &nvData);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load TAG_NV_OVERMATURE_OFFLINE_TIMEOUT");
        return;
    }
    gTagContext->offlineTimeout = nvData.data.overmatureOfflineTout;

    PortTimerChangePeriod(gTagContext->offlineTimer, CONVERT_SEC_TO_TICKS(gTagContext->offlineTimeout), 0);
    /* This function is called in CONNECTED state, So stop timer after change period */
    PortTimerStop(gTagContext->offlineTimer, 0);

    if (!gTagContext->BatteryCheckTimer || gTagContext->BatteryCheckTimer == NULL)
    {
        TagCreateBatteryCheckTimer();
    }

    PortTimerStart(gTagContext->BatteryCheckTimer, 0);

#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
    FwUpdateInit();
#endif /* TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE */

#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
    /* Execute UWB image update (if necessary) before BLE gets started in order to avoid bad user experience
       that during UWB image update the user may feel strange why there is BLE adv/connection but the tag does not respond to the owner handset. */
    PortUwbSwup();
#endif

    LoadTagContextFromNV(gTagContext);

#ifdef TAG_CONFIG_WIRELESS_FW_UPDATE_IN_SDK
    PortTimerReset(gTagContext->downloadAvailableTimer, 0);
#endif /* TAG_CONFIG_WIRELESS_FW_UPDATE_IN_SDK */

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

#if SUPPORT_GATT_UNREGISTER
    /* Init Tag GATT services*/
    ret = TagBleInitGATTDB();
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to init gatt db (%d)", ret);
    }
#endif /* SUPPORT_GATT_UNREGISTER */

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
}

static void systemResetAndReboot(TagBootReason reason, const char *debug_info)
{
    uint64_t utcTime = 0;
    TagNVData_t nvData;
    TagError_t ret = 0;

    /* Save boot reason to NV */
    if (reason >= TAG_BOOT_REASON_MAX)
    {
        TAG_LOG_E("Invalid Reason. Reason=%u", reason);
        reason = TAG_BOOT_REASON_NORMAL;
    }
    nvData.dataLength = 1;
    nvData.data.bootReason = reason;
    ret = TagNVStore(TAG_NV_BOOT_REASON, &nvData);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to save boot reason %d", ret);
    }

    /* Save RTC time to NV */
    utcTime = PortTimeGetRtcTime();
    nvData.dataLength = 8;
    nvData.data.savedRTC = utcTime;
    ret = TagNVStore(TAG_NV_SAVED_RTC, &nvData);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to save RTC value %d", ret);
    }

    if (debug_info != NULL)
    {
        TAG_LOG_I("reboot %u(%s)", reason, debug_info);
    }
    else
    {
        TAG_LOG_I("reboot %u", reason);
    }

    PortTaskDelay(CONV_MS_TO_TICKS(TAG_UTIL_RESET_DELAY_MS));

    TAG_MEM_CHECK("SystemReset");

    PortSystemReset(reason);
}

void TagUtilSystemReset(TagBootReason reason, const char *debug_info)
{
#if defined (TAG_CONFIG_FACTORY_RESET_WITHOUT_REBOOT)
    systemResetWithoutReboot(reason, debug_info);
#else
    systemResetAndReboot(reason, debug_info);
#endif
}

void TagUtilDelayBusy(uint32_t ms)
{
    PortTimeDelayBusy(ms);
}

void TagUtilBubbleSort(int* list, int n)
{
    int i, j, temp;

    for(i = n - 1; i > 0; i--) {
        for(j = 0; j < i; j++) {
            if(list[j] < list[j + 1]) {
                temp = list[j];
                list[j] = list[j + 1];
                list[j + 1] = temp;
            }
        }
    }
}
