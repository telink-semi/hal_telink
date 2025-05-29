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

#include "TagCore.h"
#include "TagErrorType.h"
#include "TagFwUpdate.h"
#include "TagLedBlink.h"
#include "TagNV.h"
#include "TagSecurity.h"
#include "TagSoundPlayer.h"
#include "TagUtil.h"
#include "TagVersion.h"
#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
#include "TagNFC.h"
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */

#include "PortBle.h"
#include "PortFwUpdate.h"
#include "PortTime.h"
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
#include "PortUwb.h"
#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */

#ifdef LOG_PREFIX
#undef LOG_PREFIX
#endif
#define LOG_PREFIX "CTRL"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

#define T_SOUND_NV_WRITING_DELAY_MS (150)

STATIC_FUNCTION TagBleError_t timeInformationWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagNVData_t nvData;
    TagError_t error = 0;

    if (data->aValue[0] == TIME_INFORMATION_SET_UTC_TIME_OPCODE)
    {
        uint64_t utcTime, timePassed;
        uint64_t remainTimeForNextCounter;

        utcTime = readUint64LittleEndian(&(data->aValue[1]));
        if (utcTime < AGING_COUNTER_NORM_UTC)
        {
            TAG_LOG_I("Old timestamp passed from end user %u", (uint32_t)utcTime);
            utcTime = AGING_COUNTER_NORM_UTC;
        }
        timePassed = utcTime - AGING_COUNTER_NORM_UTC;
        gTagContext->agingCounter = timePassed / AGING_COUNTER_INTERVAL; /* divided by AGING_COUNTER_INTERVAL */
        error = TagNVSetAgingCnt(gTagContext->agingCounter);
        if (error != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to save aging counter %d", error);
        }

        /* Adjust RTC and set timer next expire time */
        PortTimeSetRtcTime(utcTime);

        remainTimeForNextCounter = (gTagContext->agingCounter + 1) * AGING_COUNTER_INTERVAL + AGING_COUNTER_NORM_UTC - utcTime;

        PortTimerChangePeriod(gTagContext->agingCounterTimer, CONVERT_SEC_TO_TICKS(remainTimeForNextCounter), 0);
        TAG_LOG_I("Update AC(%u), rtc: %u remainSec : %u", gTagContext->agingCounter, (uint32_t)PortTimeGetRtcTime(), (uint32_t)remainTimeForNextCounter);
    }
    else if (data->aValue[0] == TIME_INFORMATION_SET_PREMATURE_OFFLINE_TIMEOUT_OPCODE)
    {
        uint16_t receivedPrematureOfflineTimeout = readUint16LittleEndian(&data->aValue[1]);
        if (!(receivedPrematureOfflineTimeout >= 1 && receivedPrematureOfflineTimeout <= 3600))
        {
            TAG_LOG_E("Received premature offline timeout(%u) out of range(1-3600)", receivedPrematureOfflineTimeout);
            return TAG_BLE_ERROR_INVALID_PDU;
        }
        gTagContext->prematureOfflineTimeout = receivedPrematureOfflineTimeout;
        nvData.dataLength = 2;
        nvData.data.prematureOfflineTout = gTagContext->prematureOfflineTimeout;
        error = TagNVStore(TAG_NV_PREMATURE_OFFLINE_TIMEOUT, &nvData);
        if (error != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to store TAG_NV_PREMATURE_OFFLINE_TIMEOUT %d", error);
            return TAG_BLE_ERROR_UNDEFINED_ERROR;
        }
        PortTimerChangePeriod(gTagContext->prematureTimer, CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(gTagContext->prematureOfflineTimeout)), 0);
        /* This function is called in CONNECTED state, So stop timer after change period */
        PortTimerStop(gTagContext->prematureTimer, 0);
        TAG_LOG_D("Set premature offline timeout %d secs", gTagContext->prematureOfflineTimeout);
    }
    else if (data->aValue[0] == TIME_INFORMATION_SET_OFFLINE_TIMEOUT_OPCODE)
    {
        uint32_t receivedOfflineTimeout = readUint32LittleEndian(&data->aValue[1]);
        if (!(receivedOfflineTimeout >= 1 && receivedOfflineTimeout <= 259200))
        {
            TAG_LOG_E("Received offline timeout(%u) out of range(1-259200)", receivedOfflineTimeout);
            return TAG_BLE_ERROR_INVALID_PDU;
        }
        gTagContext->offlineTimeout = receivedOfflineTimeout;
        nvData.dataLength = 4;
        nvData.data.overmatureOfflineTout = gTagContext->offlineTimeout;
        error = TagNVStore(TAG_NV_OVERMATURE_OFFLINE_TIMEOUT, &nvData);
        if (error != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to store TAG_NV_OVERMATURE_OFFLINE_TIMEOUT %d", error);
            return TAG_BLE_ERROR_UNDEFINED_ERROR;
        }
        PortTimerChangePeriod(gTagContext->offlineTimer, CONVERT_SEC_TO_TICKS(gTagContext->offlineTimeout), 0);

        /* This function is called in CONNECTED state, So stop timer after change period */
        PortTimerStop(gTagContext->offlineTimer, 0);
        TAG_LOG_D("Set offline timeout %d secs", gTagContext->offlineTimeout);
    }
    else if (data->aValue[0] == TIME_INFORMATION_SET_OVERMATURE_PRIVACYID_INTERVAL_OPCODE)
    {
        if (data->aValue[1] >= 0x03 && data->aValue[1] <= 0xff)
        {
            TAG_LOG_E("Invalid privacy interval code(0x%02x)", data->aValue[1]);
            return TAG_BLE_ERROR_INVALID_PDU;
        }
        gTagContext->overmaturePrivacyIdInterval = data->aValue[1];
        TAG_LOG_D("Set overmature offline privacyId interval 0x%x", gTagContext->overmaturePrivacyIdInterval);
    }
    else if (data->aValue[0] == TIME_INFORMATION_NOTIFY_TIME_OPCODE)
    {
        /* Just return TAG_BLE_ERROR_ATT_NO_ERROR. Will send indication at timeInformationWritePostProcess */
        status = TAG_BLE_ERROR_ATT_NO_ERROR;
    }
    else
    {
        status = TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    return status;
}

STATIC_FUNCTION void timeInformationWritePostProcess(EndUserDevice *endUserDevice, TagControlServiceData *writtenData)
{
    TagError_t ret = 0;
    TagControlServiceData *data;
    uint64_t utcTime;
    size_t offset = 0;

    if (writtenData->aValue[0] == TIME_INFORMATION_NOTIFY_TIME_OPCODE)
    {
        data = AllocateTagControlServiceData(TIME_INFORMATION_TIME_INFORMATION_LENGTH);
        if (data == NULL)
        {
            TAG_LOG_E("Failed to allocate memory (%d)", __LINE__);
            return;
        }

        utcTime = PortTimeGetRtcTime();
        offset += writeLittleEndian(data->aValue, 0, TIME_INFORMATION_NOTIFY_TIME_OPCODE, sizeof(uint8_t));
        offset += writeLittleEndian(data->aValue, offset, utcTime, sizeof(uint64_t));
        offset += writeLittleEndian(data->aValue, offset, gTagContext->prematureOfflineTimeout, sizeof(uint16_t));
        offset += writeLittleEndian(data->aValue, offset, gTagContext->offlineTimeout, sizeof(uint32_t));
        offset += writeLittleEndian(data->aValue, offset, gTagContext->overmaturePrivacyIdInterval, sizeof(uint8_t));
        ret = TagControlSendIndication(endUserDevice, CTRL_TIME_INFORMATION, data, ENCRYPTION_REQUIRED);
        if (ret != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to send indication (%d)", __LINE__);
        }
        FreeTagControlServiceData(data);
    }
}

#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
STATIC_FUNCTION TagBleError_t uwbPowerWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t error = TAG_ERROR_NONE;

    if (data->aValue[0] == UWB_POWER_DEACTIVATE)
    {
        error = PortUwbSetPower(UWB_POWER_MODE_DEACTIVATE, endUserDevice->uwbSessionId);
        if (error != TAG_ERROR_NONE)
        {
            status = TAG_BLE_ERROR_UNDEFINED_ERROR;
        }
    }
    else if (data->aValue[0] == UWB_POWER_ACTIVATE)
    {
        if (gTagContext->tagPSM == TAG_PSM_POWER_SAVE) {
            TAG_LOG_E("Power Saving Mode. Reject UWB power-on");
            return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
        }
        error = PortUwbSetPower(UWB_POWER_MODE_ACTIVATE, 0);
        if (error == TAG_ERROR_UWB_LOW_BAT) {
            status = TAG_BLE_ERROR_INSUFFICIENT_BATTERY;
        } else if (error != TAG_ERROR_NONE)
        {
            status = TAG_BLE_ERROR_UNDEFINED_ERROR;
        }
    }
    else
    {
        TAG_LOG_E("Not supported type %d", data->aValue[0]);
        status = TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    TAG_LOG_I("uwbPowerWriteProcess: status=%d, error=%d sessionId=%08X power=%d", status, error, endUserDevice->uwbSessionId, data->aValue[0]);
    return status;
}

STATIC_FUNCTION TagBleError_t uwbPowerReadProcess(EndUserDevice *endUserDevice, TagControlServiceData **outputData)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagControlServiceData *data;
    UwbPowerMode_t mode;
    TagError_t ret = TAG_ERROR_NONE;

    ret = PortUwbGetPower(&mode);

    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to get power %d", ret);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    data = AllocateTagControlServiceData(1);
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate memory (%d)", __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    data->aValue[0] = mode;

    *outputData = data;

    return status;
}

STATIC_FUNCTION TagBleError_t uwbParamWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *writtenData)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t error = TAG_ERROR_NONE;

    if (writtenData->cValueLength != sizeof(UwbParam_t))
    {
        TAG_LOG_E("Failed to get proper param size=%u, expected=%u", writtenData->cValueLength, sizeof(UwbParam_t));
        status = TAG_BLE_ERROR_INVALID_PDU;
        return status;
    }

    error = PortUwbSetParam((UwbParam_t *)writtenData->aValue);

    if (error == TAG_ERROR_UWB_LOW_BAT)
    {
        status = TAG_BLE_ERROR_INSUFFICIENT_BATTERY;
    }
    else if (error != TAG_ERROR_NONE)
    {
        status = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    else
    {
        endUserDevice->uwbSessionId = ((UwbParam_t *)writtenData->aValue)->sessionId;
    }

    TAG_LOG_I("uwbParamWriteProcess: status=%d, error=%d, sessionId=%08X", status, error, endUserDevice->uwbSessionId);
    return status;
}
#endif

#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
STATIC_FUNCTION void ledBlinkingWritePostProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    if (data->aValue[0] == LED_BLINKING_VALUE_OFF)
    {
        TAG_LOG_I("LED Blinking Stop(CMD)");
        if (TAG_ERROR_NONE != TagLedBlinkCtrlStop())
        {
            TAG_LOG_E("Failed to stop LED Blinking");
        }
    }
    else if (data->aValue[0] == LED_BLINKING_VALUE_ON)
    {

        if (endUserDevice->deviceType == END_USER_DEVICE_OWNER)
        {
            TAG_LOG_I("LED Blinking Start(OWN)");
        }
        else
        {
            TAG_LOG_I("LED Blinking Start(NON)");
        }

        if (TAG_ERROR_NONE != TagLedBlinkCtrlStart())
        {
            TAG_LOG_E("Failed to start LED Blinking");
        }

    }
}

STATIC_FUNCTION TagBleError_t ledBlinkingWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    if (data->cValueLength < 1)
    {
        return TAG_BLE_ERROR_INVALID_PDU;
    }

    if (data->aValue[0] != LED_BLINKING_VALUE_ON &&
        data->aValue[0] != LED_BLINKING_VALUE_OFF)
    {
        TAG_LOG_E("Not supported LED blinking command 0x%x", data->aValue[0]);
        return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    if (data->aValue[0] == LED_BLINKING_VALUE_ON &&
        gTagContext->tagPSM == TAG_PSM_POWER_SAVE &&
        endUserDevice->deviceType == END_USER_DEVICE_OWNER) {
        TAG_LOG_E("Can't blinking led during PSM");
        return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    return TAG_BLE_ERROR_ATT_NO_ERROR;
}
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
STATIC_FUNCTION void ringtoneWritePostProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    if (data->aValue[0] == RINGTONE_VALUE_OFF)
    {
        TAG_LOG_I("RingStop(CMD)");

        if (endUserDevice->deviceType == END_USER_DEVICE_OWNER)
        {
            TagSetLatestSoundType(SOUND_TYPE_RINGTONE_FOR_OWNER);
        }
        else
        {
            TagSetLatestSoundType(SOUND_TYPE_RINGTONE_FOR_NON_OWNER);
        }
        
        TagSoundPlayStop();
    }
    else if (data->aValue[0] == RINGTONE_VALUE_SIREN)
    {
        if (TagSoundIsRingtonePlaying() == false)
        {
            uint32_t soundResult;
            SoundBleEvent_t event;

            if (endUserDevice->deviceType == END_USER_DEVICE_OWNER)
            {
                event = SOUND_INDICATION_RINGTONE_SIREN;
                TAG_LOG_I("RingPlay(OWN)");
            }
            else
            {
                event = SOUND_NOTIFICATION_RINGTONE_SIREN;
                TAG_LOG_I("RingPlay(NON)");
            }

            soundResult = TagSoundPlayRingtone(RINGTONE_DEFAULT_TIMEOUT, event);
            if (soundResult == 0)
            {
                TAG_LOG_E("Failed to play ringtone");
            }
        }
        else
        {
            TAG_LOG_I("RingPlay(IGN)");
        }
    }
}

STATIC_FUNCTION TagBleError_t ringtoneWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    if (data->cValueLength < 1)
    {
        return TAG_BLE_ERROR_INVALID_PDU;
    }

    if (data->aValue[0] != RINGTONE_VALUE_OFF &&
        data->aValue[0] != RINGTONE_VALUE_SIREN)
    {
        TAG_LOG_E("Not supported ringtone command 0x%x", data->aValue[0]);
        return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    if (data->aValue[0] == RINGTONE_VALUE_SIREN &&
        gTagContext->tagPSM == TAG_PSM_POWER_SAVE &&
        endUserDevice->deviceType == END_USER_DEVICE_OWNER) {
        TAG_LOG_E("Can't play ringtone during PSM");
        return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

STATIC_FUNCTION TagBleError_t ringtoneVolumeWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    if (data->cValueLength < 1)
    {
        return TAG_BLE_ERROR_INVALID_PDU;
    }

    if (data->aValue[0] > (uint8_t)SOUND_VOLUME_LOUD)
    {
        TAG_LOG_E("Not supported volume command 0x%x", data->aValue[0]);
        return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

STATIC_FUNCTION void ringtoneVolumeWritePostProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagError_t error;

    error = TagSoundSetRingtoneVolume((SoundVolume_t)data->aValue[0]);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to set ringtone volume %d", error);
    }
}
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */

#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
#define SET_BUTTON_PUSH_EVENT_OPCODE 0x00
#define SET_BUTTON_PUSH_EVENT_DISABLE 0x00
#define SET_BUTTON_PUSH_EVENT_ENABLE 0x01
#define SET_BUTTON_HOLD_EVENT_OPCODE 0x01
#define SET_BUTTON_HOLD_EVENT_DISABLE 0x00
#define SET_BUTTON_HOLD_EVENT_ENABLE 0x01
STATIC_FUNCTION TagBleError_t buttonWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagNVData_t nvData;
    TagError_t error;

    if (data->cValueLength < 2)
    {
        return TAG_BLE_ERROR_INVALID_PDU;
    }

    if (data->aValue[0] == SET_BUTTON_PUSH_EVENT_OPCODE)
    {
        if (data->aValue[1] != SET_BUTTON_PUSH_EVENT_DISABLE &&
            data->aValue[1] != SET_BUTTON_PUSH_EVENT_ENABLE)
        {
            TAG_LOG_E("Not valid push event 0x%02x", data->aValue[1]);
            return TAG_BLE_ERROR_INVALID_PDU;
        }
        TAG_LOG_I("PUSH %s", data->aValue[1] ? "enabled" : "disabled");

        if (gTagContext->pushButtonEnabled != data->aValue[1])
        {
            nvData.dataLength = 1;
            nvData.data.buttonAction = data->aValue[1];
            error = TagNVStore(TAG_NV_BUTTON_PUSH_ACTION, &nvData);
            if (error != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to store TAG_NV_BUTTON_PUSH_ACTION");
                return TAG_BLE_ERROR_UNDEFINED_ERROR;
            }

            gTagContext->pushButtonEnabled = data->aValue[1];
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_CONFIRM), SOUND_NOT_PLAYED, "Failed to play CONFIRM sound");
        }
    }
    else if (data->aValue[0] == SET_BUTTON_HOLD_EVENT_OPCODE)
    {
        if (data->aValue[1] != SET_BUTTON_HOLD_EVENT_DISABLE &&
            data->aValue[1] != SET_BUTTON_HOLD_EVENT_ENABLE)
        {
            TAG_LOG_E("Not valid hold event 0x%02x", data->aValue[1]);
            return TAG_BLE_ERROR_INVALID_PDU;
        }
        TAG_LOG_I("HOLD %s", data->aValue[1] ? "enabled" : "disabled");

        if (gTagContext->holdButtonEnabled != data->aValue[1])
        {
            nvData.dataLength = 1;
            nvData.data.buttonAction = data->aValue[1];
            error = TagNVStore(TAG_NV_BUTTON_HOLD_ACTION, &nvData);
            if (error != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to store TAG_NV_BUTTON_HOLD_ACTION");
                return TAG_BLE_ERROR_UNDEFINED_ERROR;
            }
            gTagContext->holdButtonEnabled = data->aValue[1];
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_CONFIRM), SOUND_NOT_PLAYED, "Failed to play CONFIRM sound");
        }
    }
    else
    {
        TAG_LOG_E("Not supported button command 0x%x", data->aValue[0]);
        status = TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    return status;
}
#endif // TAG_ACCESSORY_OPTION_BUTTON_ACTION

#define E2EE_ON_COMMAND 0x01
#define E2EE_OFF_COMMAND 0x00
STATIC_FUNCTION TagBleError_t E2EEWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagNVData_t nvData;
    TagError_t error;

    if (data->aValue[0] == E2EE_ON_COMMAND)
    {
        TAG_LOG_D("E2EE ON");
        gTagContext->E2EEFlag = E2EE_ON_COMMAND;
    }
    else if (data->aValue[0] == E2EE_OFF_COMMAND)
    {
        TAG_LOG_D("E2EE OFF");
        gTagContext->E2EEFlag = E2EE_OFF_COMMAND;
    }
    else
    {
        TAG_LOG_E("Not supported E2EE command 0x%x", data->aValue[0]);
        return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    nvData.dataLength = 1;
    nvData.data.e2eEncryption = gTagContext->E2EEFlag;
    error = TagNVStore(TAG_NV_E2E_ENCRYPTION, &nvData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to store TAG_NV_E2E_ENCRYPTION");
        status = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    else
    {
        CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_CONFIRM), SOUND_NOT_PLAYED, "Failed to play CONFIRM sound");
    }

    return status;
}

STATIC_FUNCTION TagBleError_t E2EEReadProcess(EndUserDevice *endUserDevice, TagControlServiceData **outputData)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagControlServiceData *data;

    data = AllocateTagControlServiceData(1);
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate memory (%d)", __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    data->aValue[0] = gTagContext->E2EEFlag;

    *outputData = data;

    return status;
}

#if defined(TAG_ACCESSORY_OPTION_UPDATE_RINGTONE) && (TAG_ACCESSORY_OPTION_UPDATE_RINGTONE == 1)
STATIC_VARIABLE RingtoneTransferInfo_t sRingtoneInfo = {
    .deviceId = RINGTONE_UPDATE_INVALID_DEVICEID,
    .totalRingtoneSize = 0,
    .totalRingtoneCRC16 = 0x0000,
    .ringtoneNameLength = 0,
    .ringtoneName = NULL,
    .ringtoneData = NULL,
    .recvDataLength = 0};

STATIC_FUNCTION void clearRingtoneUpdateInfo(void)
{
    sRingtoneInfo.deviceId = RINGTONE_UPDATE_INVALID_DEVICEID;
    if (sRingtoneInfo.ringtoneName)
    {
        TagFree(sRingtoneInfo.ringtoneName);
        sRingtoneInfo.ringtoneName = NULL;
    }
    if (sRingtoneInfo.ringtoneData)
    {
        TagFree(sRingtoneInfo.ringtoneData);
        sRingtoneInfo.ringtoneData = NULL;
    }
    sRingtoneInfo.recvDataLength = 0;
}

STATIC_FUNCTION TagBleError_t validateTotalRingtoneUpdateInfo(void)
{
    uint16_t computedCRC16;

    computedCRC16 = TagUtilCrc16(sRingtoneInfo.ringtoneData, sRingtoneInfo.recvDataLength, 0);
    if (computedCRC16 != sRingtoneInfo.totalRingtoneCRC16)
    {
        TAG_LOG_E("CRC mismatch: 0x%x vs 0x%x", computedCRC16, sRingtoneInfo.totalRingtoneCRC16);
        return TAG_BLE_ERROR_INVALID_PDU;
    }

    if (sRingtoneInfo.totalRingtoneSize >= TAG_NV_RINGTONE_DATA_MAX_SZ)
    {
        TAG_LOG_E("Too large ringtone data %u (%u)",
                  sRingtoneInfo.recvDataLength, sRingtoneInfo.totalRingtoneSize);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    if (sRingtoneInfo.ringtoneNameLength >= TAG_NV_RINGTONE_NAME_MAX_SZ)
    {
        TAG_LOG_E("Too large ringtone name %zu (%u)",
                  strlen(sRingtoneInfo.ringtoneName), sRingtoneInfo.ringtoneNameLength);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

STATIC_FUNCTION TagBleError_t saveRingtoneUpdateInfoToNV(void)
{
    TagNVData_t nvData;
    TagError_t error;

    // Save Ringtone Name
    nvData.data.ringtoneName = TagMalloc(sRingtoneInfo.ringtoneNameLength + 1);
    if (nvData.data.ringtoneName == NULL)
    {
        TAG_LOG_E("Failed to allocate memory %u (%d)", sRingtoneInfo.ringtoneNameLength, __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    memset(nvData.data.ringtoneName, '\0', sRingtoneInfo.ringtoneNameLength + 1);
    strncpy(nvData.data.ringtoneName, sRingtoneInfo.ringtoneName, sRingtoneInfo.ringtoneNameLength + 1);
    nvData.dataLength = strlen(nvData.data.ringtoneName);
    error = TagNVStore(TAG_NV_RINGTONE_NAME, &nvData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to save ringtone name to NV");
        TagFree(nvData.data.ringtoneName);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    TagFree(nvData.data.ringtoneName);

    // Save Ringtone Data
    nvData.data.ringtoneData = TagMalloc(sRingtoneInfo.totalRingtoneSize);
    if (nvData.data.ringtoneData == NULL)
    {
        TAG_LOG_E("Failed to allocate memory %u (%d)", sRingtoneInfo.totalRingtoneSize, __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    memcpy(nvData.data.ringtoneData, sRingtoneInfo.ringtoneData, sRingtoneInfo.totalRingtoneSize);
    nvData.dataLength = sRingtoneInfo.totalRingtoneSize;
    error = TagNVStore(TAG_NV_RINGTONE_DATA, &nvData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to save ringtone data to NV");
        TagFree(nvData.data.ringtoneData);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    TagFree(nvData.data.ringtoneData);

    // Save Ringtone Data Size
    nvData.data.ringtoneDataSz = sRingtoneInfo.totalRingtoneSize;
    nvData.dataLength = sizeof(uint16_t);
    error = TagNVStore(TAG_NV_RINGTONE_DATA_SIZE, &nvData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to save ringtone size to NV");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

STATIC_FUNCTION TagBleError_t startRingtoneUpdateSession(TagBleDeviceId deviceId, uint16_t totalRingtoneSize,
                                                uint16_t totalRingtoneCRC16, uint8_t ringtoneNameLength,
                                                uint8_t *ringtoneName)
{
    if (ringtoneNameLength > RINGTONE_NAME_MAX_SIZE)
    {
        TAG_LOG_E("Invalid ringtone name length %u", ringtoneNameLength);
        return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }
    if (ringtoneName[0] == 0x00)
    {
        TAG_LOG_E("Invalid ringtone name");
        return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    if (sRingtoneInfo.deviceId != RINGTONE_UPDATE_INVALID_DEVICEID)
    {
        TAG_LOG_E("Previous session (/w 0x%x) isn't completed. Discard existing data", sRingtoneInfo.deviceId);
    }

    sRingtoneInfo.deviceId = deviceId;
    sRingtoneInfo.totalRingtoneSize = totalRingtoneSize;
    sRingtoneInfo.totalRingtoneCRC16 = totalRingtoneCRC16;
    sRingtoneInfo.ringtoneNameLength = ringtoneNameLength;
    sRingtoneInfo.recvDataLength = 0;

    if (sRingtoneInfo.ringtoneName != NULL)
    {
        TagFree(sRingtoneInfo.ringtoneName);
    }
    sRingtoneInfo.ringtoneName = TagMalloc(ringtoneNameLength + 1);
    if (sRingtoneInfo.ringtoneName == NULL)
    {
        TAG_LOG_E("Failed to allocate memory %u (%d)", ringtoneNameLength, __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    memset(sRingtoneInfo.ringtoneName, '\0', ringtoneNameLength + 1);
    strncpy(sRingtoneInfo.ringtoneName, (char *)ringtoneName, ringtoneNameLength);

    if (sRingtoneInfo.ringtoneData != NULL)
    {
        TagFree(sRingtoneInfo.ringtoneData);
    }
    sRingtoneInfo.ringtoneData = TagMalloc(totalRingtoneSize);
    if (sRingtoneInfo.ringtoneData == NULL)
    {
        TAG_LOG_E("Failed to allocate memory %u (%d)", totalRingtoneSize, __LINE__);
        TagFree(sRingtoneInfo.ringtoneName);
        sRingtoneInfo.ringtoneName = NULL;
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    memset(sRingtoneInfo.ringtoneData, '\0', totalRingtoneSize);

    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

STATIC_FUNCTION TagBleError_t validateRingtoneData(uint16_t segRingtoneDataLength, uint8_t *segRingtoneData)
{
    uint16_t offset = 0;

    while (offset < segRingtoneDataLength)
    {
        if (offset + 6 > segRingtoneDataLength)
        {
            return TAG_BLE_ERROR_INVALID_PDU;
        }
        if (!(segRingtoneData[offset] >= 'A' && segRingtoneData[offset] <= 'G'))
        {
            return TAG_BLE_ERROR_INVALID_PDU;
        }
        offset++;
        if (!(segRingtoneData[offset] == 'S' || segRingtoneData[offset] == '_' || segRingtoneData[offset] == 'R'))
        {
            return TAG_BLE_ERROR_INVALID_PDU;
        }
        offset++;
        if (!((segRingtoneData[offset] >= '4' && segRingtoneData[offset] <= '7') || segRingtoneData[offset] == 'K'))
        {
            return TAG_BLE_ERROR_INVALID_PDU;
        }
        offset++;
        // Jump play time and resonance validation(No need)
        offset += 3;
    }

    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

STATIC_FUNCTION TagBleError_t updateRingtoneUpdateSession(TagBleDeviceId deviceId, uint16_t offset,
                                                 uint16_t segRingtoneDataLength, uint8_t *segRingtoneData)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;

    if (sRingtoneInfo.deviceId != deviceId)
    {
        TAG_LOG_E("Another transfer session 0x%x -> 0x%x", sRingtoneInfo.deviceId, deviceId);
        return TAG_BLE_ERROR_INSUFFICIENT_AUTHORIZATION;
    }

    if (sRingtoneInfo.ringtoneData == NULL)
    {
        TAG_LOG_E("No ringtone buffer");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    if (sRingtoneInfo.recvDataLength + segRingtoneDataLength < sRingtoneInfo.totalRingtoneSize)
    {
        if (segRingtoneDataLength % 16 != 0)
        {
            TAG_LOG_E("Ringtone data legth is not multiple of 16");
            return TAG_BLE_ERROR_INVALID_PDU;
        }
    }

    memcpy(&sRingtoneInfo.ringtoneData[offset], segRingtoneData, segRingtoneDataLength);
    sRingtoneInfo.recvDataLength += segRingtoneDataLength;

    if (sRingtoneInfo.recvDataLength >= sRingtoneInfo.totalRingtoneSize)
    {
        if (validateRingtoneData(sRingtoneInfo.recvDataLength, sRingtoneInfo.ringtoneData) == TAG_BLE_ERROR_ATT_NO_ERROR &&
            validateTotalRingtoneUpdateInfo() == TAG_BLE_ERROR_ATT_NO_ERROR)
        {
            bool runningChange = false;

            TAG_LOG_I("Ringtone Update");

            runningChange = TagSoundIsRunningRingtoneChangeAndPrepare();
            if (runningChange)
            {
                PortTaskDelay(CONV_MS_TO_TICKS(T_SOUND_NV_WRITING_DELAY_MS));
            }
            status = saveRingtoneUpdateInfoToNV();

            clearRingtoneUpdateInfo();
            TagSoundClearCustomRingtone();
            if (TagSoundLoadCustomRingtone() != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to load custom ringtone from NV");
                status = TAG_BLE_ERROR_UNDEFINED_ERROR;
                if (runningChange)
                {
                    TagSoundPlayStop();
                }
                goto exit;
            }

            if (runningChange)
            {
                TagSoundFinishRunningRingtoneChange();
            }
            else
            {
                CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_CONFIRM), SOUND_NOT_PLAYED, "Failed to play CONFIRM sound");
            }
        }
        else
        {
            status = TAG_BLE_ERROR_INVALID_PDU;
            TAG_LOG_E("Failed to validate downloaded ringtone");
            clearRingtoneUpdateInfo();
            if (TagSoundIsRingtonePlaying())
            {
                TagSoundPlayStop();
            }
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_ERROR), SOUND_NOT_PLAYED, "Failed to play ERROR sound");
        }
    }

exit:
    return status;
}

STATIC_FUNCTION TagBleError_t ringtoneUpdateWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;

    if (data->cValueLength < 5)
    {
        return TAG_BLE_ERROR_INVALID_PDU;
    }

    if (endUserDevice->deviceType != END_USER_DEVICE_OWNER)
    {
        return TAG_BLE_ERROR_INSUFFICIENT_AUTHORIZATION;
    }

    if (data->aValue[0] == RINGTONE_UPDATE_INFORMATION_OPCODE)
    {
        uint16_t totalRingtoneSize = readUint16LittleEndian(&data->aValue[RINGTONE_INFO_TOTAL_SIZE_OFFSET]);

        if (data->cValueLength < 6 + data->aValue[RINGTONE_INFO_NAME_LENGTH_OFFSET])
        {
            TAG_LOG_E("Invalid info length %u(%u)", data->cValueLength, 6 + data->aValue[RINGTONE_INFO_NAME_LENGTH_OFFSET]);
            return TAG_BLE_ERROR_INVALID_PDU;
        }

        if (totalRingtoneSize > TAG_NV_RINGTONE_DATA_MAX_SZ)
        {
            TAG_LOG_E("Total Ringtone Size(%u) exceed Maximum(%u)", totalRingtoneSize, TAG_NV_RINGTONE_DATA_MAX_SZ);
            return TAG_BLE_ERROR_INVALID_PDU;
        }

        status = startRingtoneUpdateSession(endUserDevice->deviceId,
                                            readUint16LittleEndian(&data->aValue[RINGTONE_INFO_TOTAL_SIZE_OFFSET]),
                                            readUint16LittleEndian(&data->aValue[RINGTONE_INFO_TOTAL_CRC16_OFFSET]),
                                            data->aValue[RINGTONE_INFO_NAME_LENGTH_OFFSET],
                                            &data->aValue[RINGTONE_INFO_NAME_OFFSET]);
    }
    else if (data->aValue[0] == RINGTONE_UPDATE_DATA_OPCODE)
    {
        uint16_t segRingtoneDataLength = readUint16LittleEndian(&data->aValue[RINGTONE_DATA_SEG_DATA_LENGTH_OFFSET]);
        uint16_t argumentsCRC16 = readUint16LittleEndian(&data->aValue[RINGTONE_DATA_SEG_DATA_OFFSET + segRingtoneDataLength]);
        uint16_t computedCRC16;

        if (data->cValueLength < 5 + segRingtoneDataLength + 2)
        {
            TAG_LOG_E("Invalid data length %u(%u)", data->cValueLength, 5 + segRingtoneDataLength + 2);
            return TAG_BLE_ERROR_INVALID_PDU;
        }

        computedCRC16 = TagUtilCrc16(&data->aValue[RINGTONE_DATA_OFFSET_OFFSET], segRingtoneDataLength + 4, 0);

        if (computedCRC16 != argumentsCRC16)
        {
            TAG_LOG_E("CRC mismatch: 0x%x vs 0x%x", argumentsCRC16, computedCRC16);
            clearRingtoneUpdateInfo();
            return TAG_BLE_ERROR_INVALID_PDU;
        }

        status = updateRingtoneUpdateSession(endUserDevice->deviceId,
                                             readUint16LittleEndian(&data->aValue[RINGTONE_DATA_OFFSET_OFFSET]),
                                             segRingtoneDataLength,
                                             &data->aValue[RINGTONE_DATA_SEG_DATA_OFFSET]);
    }
    else
    {
        TAG_LOG_E("Not supported ringtone update command 0x%x", data->aValue[0]);
        status = TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    return status;
}
#endif /* TAG_ACCESSORY_OPTION_UPDATE_RINGTONE */

STATIC_FUNCTION TagBleError_t bleConnectionSettingWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t ret = TAG_ERROR_NONE;

    if (data->aValue[0] == BLE_CONNECTION_SETTING_SET_MAX_ALLOWED_CONNECTIONS_OPCODE)
    {
        TAG_LOG_E("Not supported max-allowed-connection-set");
        return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }
    else if (data->aValue[0] == BLE_CONNECTION_SETTING_SET_PREFERRED_PARAM_OPCODE)
    {
        uint16_t intervalMin, intervalMax, timeoutMultiplier;
        uint8_t slaveLatency;

        intervalMin = readUint16LittleEndian(&data->aValue[1]);
        intervalMax = readUint16LittleEndian(&data->aValue[3]);
        slaveLatency = data->aValue[5];
        timeoutMultiplier = readUint16LittleEndian(&data->aValue[6]);

        if ((intervalMin < BLE_PREFERRED_PARAM_MIN_CONNECTION_INTERVAL) || (intervalMin > BLE_PREFERRED_PARAM_MAX_CONNECTION_INTERVAL) ||
            (intervalMax < BLE_PREFERRED_PARAM_MIN_CONNECTION_INTERVAL) || (intervalMax > BLE_PREFERRED_PARAM_MAX_CONNECTION_INTERVAL) ||
            (intervalMin > intervalMax) || (slaveLatency > BLE_PREFERRED_PARAM_MAX_SLAVE_LATENCY) ||
            (timeoutMultiplier < BLE_PREFERRED_PARAM_MIN_TIMEOUT_MULTIPLIER) || (timeoutMultiplier > BLE_PREFERRED_PARAM_MAX_TIMEOUT_MULTIPLIER))
        {
            return TAG_BLE_ERROR_UNDEFINED_ERROR;
        }

        endUserDevice->lastConnectionParamIntervalMin = intervalMin;
        endUserDevice->lastConnectionParamIntervalMax = intervalMax;
        endUserDevice->lastConnectionParamSlaveLatency = slaveLatency;
        endUserDevice->lastConnectionParamTimeoutMultiplier = timeoutMultiplier;
        ret = UpdateConnectionParameters(endUserDevice, intervalMin, intervalMax, slaveLatency, timeoutMultiplier);
        TAG_LOG_I("Set paramUpdate %d~%d Lat:%d TO:%d Res: %d",
                  intervalMin, intervalMax, slaveLatency, timeoutMultiplier, ret);
        if (ret != TAG_ERROR_NONE)
        {
            return TAG_BLE_ERROR_UNDEFINED_ERROR;
        }
    }
    else if (data->aValue[0] == BLE_CONNECTION_SETTING_SET_PARAM_IDLE_OPCODE)
    {
        ret = UpdateConnectionParameters(endUserDevice,
                                         CONVERT_CONNECTION_INTERVAL_TO_API_UNIT(DEFAULT_CONNECTION_INTERVAL_MIN),
                                         CONVERT_CONNECTION_INTERVAL_TO_API_UNIT(DEFAULT_CONNECTION_INTERVAL_MAX),
                                         DEFAULT_CONNECTION_LATENCY,
                                         CONVERT_CONNECTION_SUPERVISION_TO_API_UNIT(DEFAULT_CONNECTION_SUPERVISION_TIMEOUT));
        TAG_LOG_I("Set paramUpdate Idle %d", ret);
        if (ret != TAG_ERROR_NONE)
        {
            return TAG_BLE_ERROR_UNDEFINED_ERROR;
        }
    }
    else
    {
        TAG_LOG_E("Not supported ble connection setting command 0x%x", data->aValue[0]);
        return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    return status;
}

#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
STATIC_FUNCTION TagBleError_t DebugWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;

    if (data->aValue[0] == DEBUG_START_LOG_TRANSFER_OPCODE)
    {
        if (endUserDevice->logging)
        {
            TAG_LOG_E("Logging is already started");
            status = TAG_BLE_ERROR_UNDEFINED_ERROR;
        }
        else
        {
            TAG_LOG_E("Start logging");
            endUserDevice->logging = 1;
        }
    }
    else
    {
        TAG_LOG_E("Not supported debug command 0x%x", data->aValue[0]);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    return status;
}
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */

STATIC_FUNCTION bool isCharIndexAccessAllowed(EndUserDeviceType type, uint8_t charIndex)
{
    if (type == END_USER_DEVICE_OWNER)
    {
        if (!(CTRL_CHAR_START <= charIndex && charIndex < CTRL_CHAR_END))
        {
            TAG_LOG_E("Not allowed access for owner %u", charIndex);
            return false;
        }
    }
    else if (type == END_USER_DEVICE_NON_OWNER)
    {
        if (!(CTRL_CHAR_NON_OWNER_START <= charIndex && charIndex < CTRL_CHAR_END))
        {
            TAG_LOG_E("Not allowed access for non owner %u", charIndex);
            return false;
        }
    }
    else
    {
        TAG_LOG_E("Not allowed access for unknown user %u", charIndex);
        return false;
    }
    return true;
}

#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
STATIC_FUNCTION TagBleError_t firmwareUpdateWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagBleError_t ret = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t result = TAG_ERROR_NONE;

    if ((result = FwUpdateAttributeWrittenWithoutResponse(endUserDevice, data)) != TAG_ERROR_NONE)
    {
        TAG_LOG_E("FwUpdate process error : %d", result);
        ret = TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    return ret;
}
#endif // TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE

#if defined(TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT) && (TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT == 1)
STATIC_FUNCTION TagBleError_t blePairingControl(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagBleError_t ret = TAG_BLE_ERROR_ATT_NO_ERROR;
    EndUserDevice *iter = gTagContext->endUserDevices;

    if (data->aValue[0] == BLE_PARING_CONTROL_REQUEST_OPCODE)
    {
        TAG_LOG_E("Paring start");
        /* Disconnect other connections */
        while (iter)
        {
            if (iter->deviceId != endUserDevice->deviceId)
            {
                PortBleGapDisconnect(&iter->portConnHandle);
            }
            iter = iter->next;
        }
        endUserDevice->isParing = 1;
        PortTimerReset(endUserDevice->paringRequestTimer, 0);
    }

    return ret;
}
#endif // TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT

STATIC_FUNCTION TagBleError_t updatePidPoolSize(uint32_t poolSize)
{
    TagNVData_t nvData;
    TagNVItem_t type;
    int err = 0;

    nvData.data.numberOfPrivacyId = poolSize;
    nvData.dataLength = sizeof(nvData.data.numberOfPrivacyId);

    type = TAG_NV_NUMBER_OF_PRIVACY_ID;
    while (1)
    {
        if ((err = TagNVStore(type, &nvData)) == TAG_ERROR_NONE)
        {
            break;
        }
        TAG_LOG_E("Error Failed to set for (%d) with (%d)", type, err);
        PortTaskDelay(CONV_MS_TO_TICKS(300));
    }

    gSecuContext->numberOfPrivacyId = nvData.data.numberOfPrivacyId;

    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

STATIC_FUNCTION TagBleError_t updatePidIV(uint8_t *iv)
{
    TagNVData_t nvData;
    TagNVItem_t type;
    int err = 0;

    if (iv == NULL)
    {
        return TAG_BLE_ERROR_INVALID_PDU;
    }

    unsigned char *tmp = (unsigned char *)TagMalloc(TAG_SECURITY_IV_LEN);
    if (tmp == NULL)
    {
        TAG_LOG_E("failed to malloc");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    memcpy(tmp, iv, TAG_SECURITY_IV_LEN);

    nvData.data.privacyIdIv = tmp;
    nvData.dataLength = TAG_SECURITY_IV_LEN;

    type = TAG_NV_PRIVACY_ID_IV;

    while (1)
    {
        if ((err = TagNVStore(type, &nvData)) == TAG_ERROR_NONE)
        {
            break;
        }
        TAG_LOG_E("Error Failed to set for (%d) with (%d)", type, err);
        PortTaskDelay(CONV_MS_TO_TICKS(300));
    }

    if (gSecuContext->privacyIdIv.p != NULL)
    {
        TagFree(gSecuContext->privacyIdIv.p);
    }
    gSecuContext->privacyIdIv.p = tmp;
    gSecuContext->privacyIdIv.len = TAG_SECURITY_IV_LEN;

    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

STATIC_FUNCTION void pidInformationWritePostProcess(EndUserDevice *endUserDevice, TagControlServiceData *writtenData)
{
    if (writtenData->aValue[0] == BLE_PID_SETTING_INDICATION_OPCODE)
    {
        TagError_t ret = 0;
        TagControlServiceData *data;
        size_t offset = 0;

        data = AllocateTagControlServiceData(BLE_PID_INDICATION_PID_INFORMATION_LENGTH);
        if (data == NULL)
        {
            TAG_LOG_E("ctrl(%d): no memory", __LINE__);
            return;
        }

        offset += writeLittleEndian(data->aValue, 0, BLE_PID_INDICATION_PID_INFORMATION_OPCODE, sizeof(uint8_t));
        offset += writeLittleEndian(data->aValue, offset, gSecuContext->numberOfPrivacyId, sizeof(uint16_t));
        offset += writeLittleEndian(data->aValue, offset, TAG_SECURITY_IV_LEN, sizeof(uint8_t));
        memcpy(data->aValue + offset, gSecuContext->privacyIdIv.p, TAG_SECURITY_IV_LEN);

        ret = TagControlSendIndication(endUserDevice, CTRL_BLE_PRIVACY_ID_SETTING, data, ENCRYPTION_REQUIRED);
        if (ret)
        {
            TAG_LOG_E("ctrl(%d): PID indication fail", __LINE__);
        }
        FreeTagControlServiceData(data);
    }
}

STATIC_FUNCTION TagBleError_t blePrivacyIdSettingControl(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;

    if (data->aValue[0] == BLE_PID_SETTING_UPDATE_OPCODE)
    {
        uint16_t IvLen = data->aValue[BLE_PID_SETTING_UPDATE_IV_LENGTH_OFFSET];

        uint16_t poolSize = readUint16LittleEndian(&data->aValue[BLE_PID_SETTING_UPDATE_PID_POOL_SIZE_OFFSET]);

        if (IvLen > 0 && (poolSize > 0 && poolSize <= 65535))
        {
            if (IvLen != TAG_SECURITY_IV_LEN)
            {
                TAG_LOG_E("this IV len is not supported in Tag");
                return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
            }

            if (!strncmp((const char *)gSecuContext->privacyIdIv.p, (const char *)&data->aValue[BLE_PID_SETTING_UPDATE_IV_OFFSET], TAG_SECURITY_IV_LEN))
            {
                TAG_LOG_E("Data is same");
            }
            else
            {
                updatePidIV(&data->aValue[BLE_PID_SETTING_UPDATE_IV_OFFSET]);
            }

            if (poolSize == gSecuContext->numberOfPrivacyId)
            {
                TAG_LOG_E("Data is same");
            }
            else
            {
                updatePidPoolSize(poolSize);
            }
        }
        else if (IvLen == 0 && (poolSize > 0 && poolSize <= 65535))
        {
            updatePidPoolSize(poolSize);
        }
        else if (IvLen > 0 && poolSize == 0)
        {
            if (IvLen != TAG_SECURITY_IV_LEN)
            {
                TAG_LOG_E("this IV len is not supported in Tag");
                return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
            }
            updatePidIV(&data->aValue[BLE_PID_SETTING_UPDATE_IV_OFFSET]);
        }
        else
        {
            TAG_LOG_E("Unknown Pid setting command 0x%x", data->aValue[0]);
            return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
        }

        gTagContext->privacyIdLength = 0;
        TagRefreshAdv();
    }
    else if (data->aValue[0] == BLE_PID_SETTING_INDICATION_OPCODE)
    {
        TAG_LOG_E("Pid Setting Req");
    }
    else
    {
        TAG_LOG_E("Unknown ble connection setting command 0x%x", data->aValue[0]);
        return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }
    return status;
}

#if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
STATIC_FUNCTION TagBleError_t powerSavingModeWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    if (data->cValueLength < 2)
    {
        return TAG_BLE_ERROR_INVALID_PDU;
    }

    if (data->aValue[0] != PSM_SET_MODE_OPCODE)
    {
        TAG_LOG_E("Not supported power saving command 0x%x", data->aValue[0]);
        return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    switch (data->aValue[1])
    {
    case PSM_SET_NORMAL:
    case PSM_SET_POWER_SAVE:
        TAG_LOG_I("current power mode %d requested mode %d", gTagContext->tagPSM, data->aValue[1]);
        break;
    default:
        TAG_LOG_E("Not supported power saving mode 0x%x", data->aValue[1]);
        return TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    return TAG_BLE_ERROR_ATT_NO_ERROR;
}
#endif // TAG_ACCESSORY_OPTION_POWER_SAVING_MODE

#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
STATIC_FUNCTION TagBleError_t nfcLostMessageUrlWriteProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t ret = TAG_ERROR_NONE;

    size_t lostMessageUrlLen = 0;
    unsigned char *lostMessageUrl = NULL;

    lostMessageUrlLen = data->cValueLength;
    lostMessageUrl = TagMalloc(lostMessageUrlLen);
    if (lostMessageUrl == NULL)
    {
        ret = TAG_ERROR_MEM_ALLOC;
    }
    else
    {
        memcpy(lostMessageUrl, data->aValue, lostMessageUrlLen);
        ret = TagNFCSetLostMessageURL((char *)lostMessageUrl, lostMessageUrlLen);
        TagFree(lostMessageUrl);
    }

    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to set URL (%d)", ret);
        status = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    return status;
}
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */

TagBleError_t TagControlServiceWrittenCallback(EndUserDevice *endUserDevice, uint8_t charIndex, TagControlServiceData *data)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;

    if (isCharIndexAccessAllowed(endUserDevice->deviceType, charIndex) == false)
    {
        return TAG_BLE_ERROR_INSUFFICIENT_AUTHORIZATION;
    }

    switch (charIndex)
    {
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
    case CTRL_LED_BLINKING:
    case CTRL_LED_BLINKING_NON_OWNER:
        status = ledBlinkingWriteProcess(endUserDevice, data);
        break;
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
    case CTRL_RINGTONE:
    case CTRL_RINGTONE_NON_OWNER:
        status = ringtoneWriteProcess(endUserDevice, data);
        break;
    case CTRL_RINGTONE_VOLUME:
        status = ringtoneVolumeWriteProcess(endUserDevice, data);
        break;
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
    case CTRL_BUTTON:
        status = buttonWriteProcess(endUserDevice, data);
        break;
#endif // TAG_ACCESSORY_OPTION_BUTTON_ACTION
    case CTRL_TIME_INFORMATION:
        status = timeInformationWriteProcess(endUserDevice, data);
        break;
    case CTRL_FACTORY_RESET:
        if (data->aValue[0] == FACTORY_RESET_COMMAND)
        {
            status = TAG_BLE_ERROR_ATT_NO_ERROR;
        }
        else
        {
            status = TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
        }
        break;
    case CTRL_E2E_ENCRYPTION:
        status = E2EEWriteProcess(endUserDevice, data);
        break;
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
    case CTRL_UWB_POWER_NON_OWNER:
        /* Falling through */
    case CTRL_UWB_POWER:
        status = uwbPowerWriteProcess(endUserDevice, data);
        break;
    case CTRL_UWB_PARAM_NON_OWNER:
        /* Falling through */
    case CTRL_UWB_PARAM:
        status = uwbParamWriteProcess(endUserDevice, data);
        break;
#endif
#if defined(TAG_ACCESSORY_OPTION_UPDATE_RINGTONE) && (TAG_ACCESSORY_OPTION_UPDATE_RINGTONE == 1)
    case CTRL_RINGTONE_UPDATE:
        status = ringtoneUpdateWriteProcess(endUserDevice, data);
        break;
#endif /* TAG_ACCESSORY_OPTION_UPDATE_RINGTONE */
    case CTRL_BLE_CONNECTION_SETTING:
        status = bleConnectionSettingWriteProcess(endUserDevice, data);
        break;
#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
    case CTRL_FIRMWARE_TRANSFER:
        status = firmwareUpdateWriteProcess(endUserDevice, data);
        break;
#endif // TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE
#if defined(TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT) && (TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT == 1)
    case CTRL_BLE_PAIRING_CONTROL:
        status = blePairingControl(endUserDevice, data);
        break;
#endif // TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT
    case CTRL_BLE_PRIVACY_ID_SETTING:
        status = blePrivacyIdSettingControl(endUserDevice, data);
        break;
#if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
    case CTRL_POWER_SAVING_MODE:
        status = powerSavingModeWriteProcess(endUserDevice, data);
        break;
#endif // TAG_ACCESSORY_OPTION_POWER_SAVING_MODE
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
    case CTRL_DEBUG_TAG:
        status = DebugWriteProcess(endUserDevice, data);
        break;
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */

#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
    case CTRL_NFC_LOST_MESSAGE_URL:
        status = nfcLostMessageUrlWriteProcess(endUserDevice, data);
        break;
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */

    default:
        status = TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
        TAG_LOG_D("No handler in Tag Control service");
        break;
    }

    return status;
}

STATIC_FUNCTION void bleConnectionSettingWritePostProcess(EndUserDevice *endUserDevice, TagControlServiceData *writtenData)
{
    if (writtenData->aValue[0] == BLE_CONNECTION_SETTING_SET_MAX_ALLOWED_CONNECTIONS_OPCODE)
    {
        TagRefreshAdv();
    }
}

STATIC_FUNCTION void E2EEWritePostProcess(EndUserDevice *endUserDevice, TagControlServiceData *writtenData)
{
    TagRefreshAdv();
}

#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
STATIC_FUNCTION void DebugWritePostProcess(EndUserDevice *endUserDevice, TagControlServiceData *writtenData)
{
    TagControlServiceData *data;
    size_t offset = 0;
    size_t logSize = 0;

    /* Increase log transfer speed */
    UpdateConnectionParameters(endUserDevice,
                               CONVERT_CONNECTION_INTERVAL_TO_API_UNIT(DEBUG_TRANSFER_CONNECTION_PARAM_INTERVAL_MIN),
                               CONVERT_CONNECTION_INTERVAL_TO_API_UNIT(DEBUG_TRANSFER_CONNECTION_PARAM_INTERVAL_MAX),
                               DEBUG_TRANSFER_CONNECTION_PARAM_LATENCY,
                               CONVERT_CONNECTION_SUPERVISION_TO_API_UNIT(DEBUG_TRANSFER_CONNECTION_PARAM_TIMEOUT));

    data = AllocateTagControlServiceData(DEBUG_TRANSFER_LOG_INFORMATION_LENGTH);
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate data (%d)", __LINE__);
        endUserDevice->logging = 0;
        return;
    }

    if (endUserDevice->logParam)
    {
        TagLogParamDeinit(endUserDevice->logParam);
        endUserDevice->logParam = NULL;
    }
    endUserDevice->logParam = TagLogParamInit(TAG_LOG_TYPE_DEBUG, LOG_FROM_FLASH);
    if (!endUserDevice->logParam)
    {
        endUserDevice->logging = 0;
        FreeTagControlServiceData(data);
        return;
    }

    logSize = TagLogGetSize(endUserDevice->logParam);

    data->aValue[offset++] = DEBUG_TRANSFER_LOG_INFORMATION_OPCODE;
    offset += writeLittleEndian(data->aValue, offset, logSize, sizeof(uint32_t));
#ifdef TAG_CONFIG_USE_NOTIFICATION_FOR_DEBUG
    if (TagControlSendNotification(endUserDevice, CTRL_DEBUG_TAG, data, ENCRYPTION_REQUIRED) != TAG_ERROR_NONE)
#else
    if (TagControlSendIndication(endUserDevice, CTRL_DEBUG_TAG, data, ENCRYPTION_REQUIRED) != TAG_ERROR_NONE)
#endif
    {
        TAG_LOG_E("Failed to send DEBUG indication (%d))", __LINE__);
        TagLogParamDeinit(endUserDevice->logParam);
        endUserDevice->logParam = NULL;
        endUserDevice->logging = 0;
    }

    FreeTagControlServiceData(data);
}
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */

#if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
STATIC_FUNCTION void sendPowerSavingModeChangeIndication(void)
{
    EndUserDevice *endUserDevice = gTagContext->endUserDevices;
    TagError_t bleError = TAG_ERROR_NONE;
    TagControlServiceData *data = NULL;

    data = AllocateTagControlServiceData(1);
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate memory (%d)", __LINE__);
        return;
    }
    data->aValue[0] = gTagContext->tagPSM;
    while (endUserDevice)
    {
        bleError = TagControlSendIndication(endUserDevice, CTRL_POWER_SAVING_MODE, data, ENCRYPTION_REQUIRED);
        if (bleError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to send Indication to EndUserDevice-%u (%d)", endUserDevice->deviceId, bleError);
        }

        endUserDevice = endUserDevice->next;
    }
    FreeTagControlServiceData(data);
}

STATIC_FUNCTION void setPSM(void)
{
    if (TagSoundIsRingtonePlaying()) {
    TagSoundPlayStop();
    }
}

STATIC_FUNCTION void powerSavingModeWritePostProcess(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagPowerSavingMode requestedMode;
    TagNVData_t nvData;
    TagError_t error;

    switch (data->aValue[1])
    {
    case PSM_SET_NORMAL:
        requestedMode = TAG_PSM_NORMAL;
        break;
    case PSM_SET_POWER_SAVE:
        requestedMode = TAG_PSM_POWER_SAVE;
        break;
    default:
        TAG_LOG_E("Not supported power saving mode 0x%x", data->aValue[1]);
        return;
    }

    if (requestedMode != gTagContext->tagPSM)
    {
        nvData.dataLength = 1;
        nvData.data.activityMode = requestedMode;
        error = TagNVStore(TAG_NV_ACTIVITY_MODE, &nvData);
        if (error != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to store TAG_NV_ACTIVITY_MODE");
            return;
        }
        gTagContext->tagPSM = requestedMode;
if (gTagContext->tagPSM == TAG_PSM_POWER_SAVE) {
setPSM();
}
        TagRefreshAdv();
    }

    sendPowerSavingModeChangeIndication();
}
#endif // TAG_ACCESSORY_OPTION_POWER_SAVING_MODE

STATIC_FUNCTION void tagControlServiceWrittenPostCallback(EndUserDevice *endUserDevice, uint8_t charIndex, TagControlServiceData *data)
{
    /* code after BLE write response success if needed */
    switch (charIndex)
    {
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
    case CTRL_LED_BLINKING:
    case CTRL_LED_BLINKING_NON_OWNER:
        ledBlinkingWritePostProcess(endUserDevice, data);
        break;
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
    case CTRL_RINGTONE:
    case CTRL_RINGTONE_NON_OWNER:
        ringtoneWritePostProcess(endUserDevice, data);
        break;
    case CTRL_RINGTONE_VOLUME:
        ringtoneVolumeWritePostProcess(endUserDevice, data);
        break;
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
    case CTRL_FACTORY_RESET:
        if (TagFactoryReset(FACTORY_RESET_DELAY_TIME_FOR_WRITE_RESPONSE) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to run Factory Reset");
        }
        break;
    case CTRL_TIME_INFORMATION:
        timeInformationWritePostProcess(endUserDevice, data);
        break;
    case CTRL_BLE_CONNECTION_SETTING:
        bleConnectionSettingWritePostProcess(endUserDevice, data);
        break;
    case CTRL_E2E_ENCRYPTION:
        E2EEWritePostProcess(endUserDevice, data);
        break;
    case CTRL_BLE_PRIVACY_ID_SETTING:
        pidInformationWritePostProcess(endUserDevice, data);
        break;
#if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
    case CTRL_POWER_SAVING_MODE:
        powerSavingModeWritePostProcess(endUserDevice, data);
        break;
#endif // TAG_ACCESSORY_OPTION_POWER_SAVING_MODE
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
    case CTRL_DEBUG_TAG:
        DebugWritePostProcess(endUserDevice, data);
        break;
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
    default:
        TAG_LOG_D("No handler for %u", charIndex);
        break;
    }
}

void TagControlServiceWrittenPostCallback(TagTaskWorkParam param)
{
    TagControlServiceGattData *gattData = (TagControlServiceGattData *)param;
    EndUserDevice *endUserDevice = TagFindEndUserDevice(gattData->deviceId);
    uint8_t charIndex = gattData->charIndex;
    TagControlServiceData *data = gattData->data;

    if (endUserDevice == NULL)
    {
        TAG_LOG_E("No endUserDevice for %u (%d)", gattData->deviceId, __LINE__);
        return;
    }
    tagControlServiceWrittenPostCallback(endUserDevice, charIndex, data);

    FreeTagControlServiceData(gattData->data);
    TagFree(gattData);
}

#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
STATIC_FUNCTION TagBleError_t ledBlinkingReadProcess(EndUserDevice *endUserDevice, TagControlServiceData **outputData)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagControlServiceData *data;

    data = AllocateTagControlServiceData(1);
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate memory (%d)", __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    if (TagLedBlinkIsBlinking())
    {
        data->aValue[0] = LED_BLINKING_VALUE_ON;
    }
    else
    {
        data->aValue[0] = LED_BLINKING_VALUE_OFF;
    }

    *outputData = data;

    TAG_LOG_D("LED Blinking status - 0x%02x", data->aValue[0]);

    return status;
}
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
STATIC_FUNCTION TagBleError_t ringtoneReadProcess(EndUserDevice *endUserDevice, TagControlServiceData **outputData)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagControlServiceData *data;

    data = AllocateTagControlServiceData(1);
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate memory (%d)", __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    if (TagSoundIsRingtonePlaying())
    {
        data->aValue[0] = RINGTONE_VALUE_SIREN;
    }
    else
    {
        data->aValue[0] = RINGTONE_VALUE_OFF;
    }

    *outputData = data;

    TAG_LOG_D("Ringtone status - 0x%02x", data->aValue[0]);

    return status;
}

STATIC_FUNCTION TagBleError_t ringtoneVolumeReadProcess(EndUserDevice *endUserDevice, TagControlServiceData **outputData)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagControlServiceData *data;
    SoundVolume_t volume;
    TagError_t error;

    data = AllocateTagControlServiceData(1);
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate memory (%d)", __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    error = TagSoundGetRingtoneVolume(&volume);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to get sound volume");
        FreeTagControlServiceData(data);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    TAG_LOG_D("Volume 0x%02x", volume);

    data->aValue[0] = volume;

    *outputData = data;

    return status;
}
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */

STATIC_FUNCTION TagBleError_t batteryReadProcess(EndUserDevice *endUserDevice, TagControlServiceData **outputData)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagControlServiceData *data;

    data = AllocateTagControlServiceData(1);
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate memory (%d)", __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    data->aValue[0] = gTagContext->batteryLevel;

    *outputData = data;

    return status;
}

#if defined(TAG_ACCESSORY_OPTION_UPDATE_RINGTONE) && (TAG_ACCESSORY_OPTION_UPDATE_RINGTONE == 1)
STATIC_FUNCTION TagBleError_t ringtoneUpdateReadProcess(EndUserDevice *endUserDevice, TagControlServiceData **outputData)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagControlServiceData *data;
    char currentRingtoneName[TAG_NV_RINGTONE_NAME_MAX_SZ] = {
        0,
    };

    if (TagSoundGetRingtoneName(currentRingtoneName, sizeof(currentRingtoneName)) != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to get current ringtone name");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    data = AllocateTagControlServiceData(strlen(currentRingtoneName));
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate memory %zu (%d)", strlen(currentRingtoneName), __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    strncpy((char *)data->aValue, currentRingtoneName, strlen(currentRingtoneName));

    *outputData = data;

    return status;
}
#endif /* TAG_ACCESSORY_OPTION_UPDATE_RINGTONE */

STATIC_FUNCTION TagBleError_t firmwareVersionReadProcess(EndUserDevice *endUserDevice, TagControlServiceData **outputData)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagControlServiceData *data;

    data = AllocateTagControlServiceData(strlen(DEVICE_FW_VERSION_STRING));
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate memory (%d)", __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    memcpy((char *)data->aValue, DEVICE_FW_VERSION_STRING, strlen(DEVICE_FW_VERSION_STRING));

    *outputData = data;

    return status;
}

#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
STATIC_FUNCTION TagBleError_t firmwareTransferReadProcess(EndUserDevice *endUserDevice, TagControlServiceData **outputData)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagControlServiceData *data;

    data = AllocateTagControlServiceData(1);
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate memory (%d)", __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    data->aValue[0] = FwUpdateGetState();
    *outputData = data;

    return status;
}
#endif // TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE

STATIC_FUNCTION TagBleError_t bleConnectionSettingReadProcess(EndUserDevice *endUserDevice, TagControlServiceData **outputData)
{
    TagError_t ret = TAG_ERROR_NONE;
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagControlServiceData *data;
    uint16_t mtuSize;
    size_t offset = 0;

    data = AllocateTagControlServiceData(4);
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate memory (%d)", __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    ret = PortBleGetMtu(&endUserDevice->portConnHandle, &mtuSize);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to get MTU %d", ret);
        FreeTagControlServiceData(data);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    TAG_LOG_D("Mtu Size(%u) is %d", endUserDevice->deviceId, mtuSize);

    data->aValue[offset++] = MAX_ALLOWED_BLE_CONNECTIONS;
    offset += writeLittleEndian(data->aValue, offset, mtuSize, sizeof(uint16_t));
    data->aValue[offset] = PortFwUpdateGetMaxWriteWithoutResponse();

    *outputData = data;

    return status;
}

STATIC_FUNCTION TagBleError_t specificationVersionReadProcess(EndUserDevice *endUserDevice, TagControlServiceData **outputData)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagControlServiceData *data;

    data = AllocateTagControlServiceData(strlen(SPEC_VERSION_STRING));
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocated memory (%d)", __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    memcpy((char *)data->aValue, SPEC_VERSION_STRING, strlen(SPEC_VERSION_STRING));

    *outputData = data;

    return status;
}

#if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
STATIC_FUNCTION TagBleError_t powerSavingModeReadProcess(EndUserDevice *endUserDevice, TagControlServiceData **outputData)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagControlServiceData *data;

    data = AllocateTagControlServiceData(1);
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate memory (%d)", __LINE__);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    switch (gTagContext->tagPSM)
    {
    case TAG_PSM_NORMAL:
        data->aValue[0] = PSM_SET_NORMAL;
        break;
    case TAG_PSM_POWER_SAVE:
        data->aValue[0] = PSM_SET_POWER_SAVE;
        break;
    default:
        TAG_LOG_E("Invalid mode (%u)", gTagContext->tagPSM);
        data->aValue[0] = PSM_SET_NORMAL;
        break;
    }

    *outputData = data;

    TAG_LOG_D("PSM mode - 0x%02x", data->aValue[0]);

    return status;
}
#endif // TAG_ACCESSORY_OPTION_POWER_SAVING_MODE

TagBleError_t TagControlServiceReadCallback(EndUserDevice *endUserDevice, uint8_t charIndex, uint8_t **readData, size_t *readDataLength)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagControlServiceData *data = NULL;

    TAG_LOG_D("Characteristic %u (%u)", charIndex, endUserDevice->deviceId);
    if (isCharIndexAccessAllowed(endUserDevice->deviceType, charIndex) == false)
    {
        return TAG_BLE_ERROR_INSUFFICIENT_AUTHORIZATION;
    }

    switch (charIndex)
    {
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
    case CTRL_LED_BLINKING:
    case CTRL_LED_BLINKING_NON_OWNER:
        status = ledBlinkingReadProcess(endUserDevice, &data);
        break;
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
    case CTRL_RINGTONE:
    case CTRL_RINGTONE_NON_OWNER:
        status = ringtoneReadProcess(endUserDevice, &data);
        break;
    case CTRL_RINGTONE_VOLUME:
        status = ringtoneVolumeReadProcess(endUserDevice, &data);
        break;
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
    case CTRL_BATTERY:
        status = batteryReadProcess(endUserDevice, &data);
        break;
    case CTRL_E2E_ENCRYPTION:
        status = E2EEReadProcess(endUserDevice, &data);
        break;
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
    case CTRL_UWB_POWER_NON_OWNER:
    case CTRL_UWB_POWER:
        status = uwbPowerReadProcess(endUserDevice, &data);
        break;
#endif
#if defined(TAG_ACCESSORY_OPTION_UPDATE_RINGTONE) && (TAG_ACCESSORY_OPTION_UPDATE_RINGTONE == 1)
    case CTRL_RINGTONE_UPDATE:
        status = ringtoneUpdateReadProcess(endUserDevice, &data);
        break;
#endif /* TAG_ACCESSORY_OPTION_UPDATE_RINGTONE */
    case CTRL_FIRMWARE_VERSION:
        status = firmwareVersionReadProcess(endUserDevice, &data);
        break;
#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
    case CTRL_FIRMWARE_TRANSFER:
        status = firmwareTransferReadProcess(endUserDevice, &data);

        if (FwUpdateGetState() == FW_UPDATE_STATE_TRANSFER_IN_PROGRESS)
        {
            TAG_LOG_E("Firmware update is already stared");
            FwUpdateSendCommand(endUserDevice, FW_UPDATE_STATE_UPDATE_NOT_ALLOWED);
        }
        break;
#endif // TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE
    case CTRL_BLE_CONNECTION_SETTING:
        status = bleConnectionSettingReadProcess(endUserDevice, &data);
        break;
    case CTRL_SPECIFICATION_VERSION:
        status = specificationVersionReadProcess(endUserDevice, &data);
        break;
#if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
    case CTRL_POWER_SAVING_MODE:
        status = powerSavingModeReadProcess(endUserDevice, &data);
        break;
#endif // TAG_ACCESSORY_OPTION_POWER_SAVING_MODE
#ifdef TAG_CONFIG_USE_TX_POWER
    case CTRL_TX_POWER:
        status = txPowerReadProcess(endUserDevice, &data);
        break;
#endif

#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
    case CTRL_NFC_LOST_MESSAGE_URL:
        status = TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
        break;
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */

    default:
        TAG_LOG_D("No handler in Tag Control service");
        break;
    }

    if (status == TAG_BLE_ERROR_ATT_NO_ERROR && data != NULL)
    {
        *readData = TagMalloc(data->cSeqLength + data->cValueLength);
        *readDataLength = data->cSeqLength + data->cValueLength;
        memcpy(*readData, data->aSeqValue, data->cSeqLength + data->cValueLength);
        FreeTagControlServiceData(data);
    }

    return status;
}

STATIC_FUNCTION TagError_t tagControlBleSend(TagBleSendingMode_t mode, EndUserDevice *endUserDevice, uint8_t charIndex, TagControlServiceData *data, uint8_t encryptFlag, bool immediate)
{
    TagError_t ret = TAG_ERROR_NONE;
    TagSecurityBuffer_t encryptedData = {0};
    IndicationData_t queuedIndication;
    PortBleAttrInfo attrInfo;

    if (endUserDevice == NULL)
    {
        TAG_LOG_E("Invalid endUserDevice");
        return TAG_ERROR_INVALID_ARG;
    }

    if (endUserDevice->deviceType == END_USER_DEVICE_UNKNOWN)
    {
        TAG_LOG_E("Block sending event to unknown endUserDevice");
        return TAG_ERROR_INVALID_ARG;
    }

    writeLittleEndian(data->aSeqValue, 0, endUserDevice->seqNoT, TAG_CONTROL_SEQUENCE_LENGTH);

    if (encryptFlag)
    {
        ret = TagAuthEncryptData(data->aSeqValue, data->cSeqLength + data->cValueLength, &encryptedData,
                                 endUserDevice, endUserDevice->commandKeyType);
        if (ret)
        {
            TAG_LOG_E("Failed to encrypt data %d (%d)", ret, __LINE__);
            return ret;
        }
    }

    switch (mode)
    {
    case TAG_SEND_INDICATION:
        if (immediate)
        {
            TAG_LOG_I("ind-im (%u) %u. Q-%u",
                      endUserDevice->deviceId, charIndex, NUMBER_OF_QUEUED_INDICATION(endUserDevice->indicationQueue));

            queuedIndication.ignoreFlag = true;
            if (PortQueueSend(endUserDevice->indicationQueue, &queuedIndication, 0) != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to send queue (ignored). Queue space %lu", PortQueueSpacesAvailable(endUserDevice->indicationQueue));
                TAG_LOG_E("Reset queue %u", endUserDevice->deviceId);
                TagControlFreeQueuedIndicationData(endUserDevice->indicationQueue);
                PortQueueReset(endUserDevice->indicationQueue);
            }
            PortBleChangeAttrInfoByIndex(&attrInfo, charIndex);

            ret = PortBleGattSendIndication(&endUserDevice->portConnHandle, &attrInfo, encryptedData.p, encryptedData.len);
            if (ret != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to send indication %d", ret);
            }
        }
        else
        {
            memset(&queuedIndication, '\0', sizeof(IndicationData_t));

            if (IS_QUEUE_EMPTY(endUserDevice->indicationQueue))
            {
                queuedIndication.ignoreFlag = true;
                if (PortQueueSend(endUserDevice->indicationQueue, &queuedIndication, 0) != TAG_ERROR_NONE)
                {
                    TAG_LOG_E("Failed to send queue (ignored). Queue space %lu",
                              PortQueueSpacesAvailable(endUserDevice->indicationQueue));
                    TAG_LOG_E("Reset queue %u", endUserDevice->deviceId);
                    TagControlFreeQueuedIndicationData(endUserDevice->indicationQueue);
                    PortQueueReset(endUserDevice->indicationQueue);
                }
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
                if (!endUserDevice->logging)
                {
                    TAG_LOG_I("ind %u (%u)", charIndex, endUserDevice->deviceId);
                }
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
                PortBleChangeAttrInfoByIndex(&attrInfo, charIndex);

                ret = PortBleGattSendIndication(&endUserDevice->portConnHandle, &attrInfo, encryptedData.p, encryptedData.len);
                if (ret != TAG_ERROR_NONE)
                {
                    TAG_LOG_E("Failed to send indication %d", ret);
                }
            }
            else
            {
                TAG_LOG_E("ind-q (%u) %u. Q-%u", endUserDevice->deviceId,
                          charIndex, NUMBER_OF_QUEUED_INDICATION(endUserDevice->indicationQueue));
                queuedIndication.charIndex = charIndex;
                queuedIndication.data = DuplicateTagControlServiceData(data);
                queuedIndication.encryptFlag = encryptFlag;
                queuedIndication.ignoreFlag = false;
                if (PortQueueSend(endUserDevice->indicationQueue, &queuedIndication, 0) != TAG_ERROR_NONE)
                {
                    TAG_LOG_E("Failed to send queue (%u). Queue space %lu",
                              endUserDevice->deviceId, PortQueueSpacesAvailable(endUserDevice->indicationQueue));
                    ret = TAG_ERROR_OPERATION_FAILURE;
                    FreeTagControlServiceData(queuedIndication.data);
                    TagControlFreeQueuedIndicationData(endUserDevice->indicationQueue);
                    PortQueueReset(endUserDevice->indicationQueue);
                    break;
                }
            }
        }

        break;
    case TAG_SEND_NOTIFICATION:
        PortBleChangeAttrInfoByIndex(&attrInfo, charIndex);

        ret = PortBleGattSendNotification(&endUserDevice->portConnHandle, &attrInfo, encryptedData.p, encryptedData.len);
        if (ret != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to send notification %d", ret);
        }
        break;
    default:
        TAG_LOG_E("Not supported sending mode %d", mode);
        ret = TAG_ERROR_NOT_SUPPORTED;
        break;
    }

    if (encryptFlag)
    {
        TagCryptoSecurityBufferFree(&encryptedData);
    }

    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to send event. mode %d, result %d", mode, ret);
        return ret;
    }

    switch (mode)
    {
    case TAG_SEND_INDICATION:
        endUserDevice->controlServiceIndicationProgress = 1;
        endUserDevice->seqNoT++;
        break;
    case TAG_SEND_NOTIFICATION:
#ifdef TAG_CONFIG_USE_NOTIFICATION_FOR_DEBUG
        endUserDevice->controlServiceNotificationProgress = 1;
#endif
        endUserDevice->seqNoT++;
        break;
    default:
        ret = TAG_ERROR_NOT_SUPPORTED;
        break;
    }

    return ret;
}

void TagControlFreeQueuedIndicationData(PortQueueHandle_t queue)
{
    while (!IS_QUEUE_EMPTY(queue))
    {
        IndicationData_t queueData;

        if (PortQueueReceive(queue, &queueData, 0) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to receive queue");
            continue;
        }

        FreeTagControlServiceData(queueData.data);
    }
}

TagError_t TagControlSendIndication(EndUserDevice *endUserDevice, uint8_t charIndex, TagControlServiceData *data, uint8_t encryptFlag)
{
    return tagControlBleSend(TAG_SEND_INDICATION, endUserDevice, charIndex, data, encryptFlag, false);
}

TagError_t TagControlSendImmediateIndication(EndUserDevice *endUserDevice, uint8_t charIndex, TagControlServiceData *data, uint8_t encryptFlag)
{
    return tagControlBleSend(TAG_SEND_INDICATION, endUserDevice, charIndex, data, encryptFlag, true);
}

TagError_t TagControlSendNotification(EndUserDevice *endUserDevice, uint8_t charIndex, TagControlServiceData *data, uint8_t encryptFlag)
{
    return tagControlBleSend(TAG_SEND_NOTIFICATION, endUserDevice, charIndex, data, encryptFlag, false);
}

TagControlServiceData *AllocateTagControlServiceData(size_t valueLength)
{
    TagControlServiceData *data;

    data = TagMalloc(sizeof(TagControlServiceData));
    if (data == NULL)
    {
        return NULL;
    }
    data->cSeqLength = TAG_CONTROL_SEQUENCE_LENGTH;
    data->cValueLength = valueLength;

    data->aSeqValue = TagMalloc(valueLength + TAG_CONTROL_SEQUENCE_LENGTH);
    if (data->aSeqValue == NULL)
    {
        TagFree(data);
        return NULL;
    }
    memset(data->aSeqValue, '\0', valueLength + TAG_CONTROL_SEQUENCE_LENGTH);
    data->aValue = data->aSeqValue + data->cSeqLength;

    return data;
}

TagControlServiceData *DuplicateTagControlServiceData(TagControlServiceData *inputData)
{
    TagControlServiceData *outputData;

    if (inputData == NULL)
    {
        return NULL;
    }

    outputData = TagMalloc(sizeof(TagControlServiceData));
    if (outputData == NULL)
    {
        return NULL;
    }
    outputData->cSeqLength = inputData->cSeqLength;
    outputData->cValueLength = inputData->cValueLength;

    outputData->aSeqValue = TagMalloc(outputData->cValueLength + TAG_CONTROL_SEQUENCE_LENGTH);
    if (outputData->aSeqValue == NULL)
    {
        TagFree(outputData);
        return NULL;
    }
    memcpy(outputData->aSeqValue, inputData->aSeqValue, outputData->cValueLength + TAG_CONTROL_SEQUENCE_LENGTH);
    outputData->aValue = outputData->aSeqValue + outputData->cSeqLength;

    return outputData;
}

void FreeTagControlServiceData(TagControlServiceData *data)
{
    if (data != NULL && data->aSeqValue != NULL)
    {
        TagFree(data->aSeqValue);
    }

    if (data != NULL)
    {
        TagFree(data);
    }
}
