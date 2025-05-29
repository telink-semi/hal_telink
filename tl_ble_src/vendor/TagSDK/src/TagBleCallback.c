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

/* TagSDK */
#include "TagAuthService.h"
#include "TagBleCallback.h"
#include "TagCore.h"
#include "TagFwUpdate.h"
#include "TagOnboardingService.h"
#include "TagSecurity.h"
#include "TagSoundPlayer.h"
#include "TagUtil.h"
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
#include "TagLedBlink.h"
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

/* Porting Layer */
#include "PortBle.h"
#include "PortOs.h"

#ifdef LOG_PREFIX
#undef LOG_PREFIX
#endif
#define LOG_PREFIX "BleC"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

STATIC_FUNCTION bool isParing(void);

STATIC_FUNCTION void connectionParamRecoveryTimeoutHandler(TagTaskWorkParam param)
{
    /* for future use */
    PortTimerHandle_t timer = param;
    EndUserDevice *bleDevice;
    bleDevice = (EndUserDevice *)PortTimerGetTimerId(timer);

    if (bleDevice == NULL)
    {
        TAG_LOG_E("Connection Parameters Recovery Timer was deleted");
        return;
    }

#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
    FwUpdateState_t state = FwUpdateGetState();

    /* skip recovery routine when ble setting is changed by firmware update function  */
    if (state == FW_UPDATE_STATE_TRANSFER_IN_PROGRESS)
    {
        TAG_LOG_I("Delay recovery routine by firmware update state(%d)", state);
        PortTimerReset(timer, 0);
        return;
    }
#endif /* TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE */

    TAG_LOG_I("ParamUpdate recovery timeout [%d]", bleDevice->deviceId);
    UpdateConnectionParameters(bleDevice,
                               CONVERT_CONNECTION_INTERVAL_TO_API_UNIT(DEFAULT_CONNECTION_INTERVAL_MIN),
                               CONVERT_CONNECTION_INTERVAL_TO_API_UNIT(DEFAULT_CONNECTION_INTERVAL_MAX),
                               DEFAULT_CONNECTION_LATENCY,
                               CONVERT_CONNECTION_SUPERVISION_TO_API_UNIT(DEFAULT_CONNECTION_SUPERVISION_TIMEOUT));
}

STATIC_FUNCTION void connectionParamRecoveryTimeoutCallback(PortTimerHandle_t timer)
{
    TagPutPostWork(connectionParamRecoveryTimeoutHandler, timer);
}

STATIC_FUNCTION void connectionParamRetryTimeoutHandler(TagTaskWorkParam param)
{
    /* for future use */
    PortTimerHandle_t timer = param;
    EndUserDevice *bleDevice;
    bleDevice = (EndUserDevice *)PortTimerGetTimerId(timer);
    TagError_t result;

    TAG_LOG_I("%s called", __func__);
    if (bleDevice == NULL)
    {
        TAG_LOG_E("Connection Parameters Retry Timer was deleted");
        return;
    }

    if (bleDevice->connectionParamIntervalMin != 0 &&
             bleDevice->connectionParamUpdateRetryCount > 0)
    {
        result = PortBleRequestConnectionParameters(&bleDevice->portConnHandle,
                                                 bleDevice->connectionParamIntervalMin,
                                                 bleDevice->connectionParamIntervalMax,
                                                 bleDevice->connectionParamSlaveLatency,
                                                 bleDevice->connectionParamTimeoutMultiplier);
        bleDevice->connectionParamUpdateRetryCount--;
        TAG_LOG_I("Retry setParam %d~%d Lat:%d TO:%d R:%d Res:%d",
                bleDevice->connectionParamIntervalMin, bleDevice->connectionParamIntervalMax,
                bleDevice->connectionParamSlaveLatency, bleDevice->connectionParamTimeoutMultiplier,
                bleDevice->connectionParamUpdateRetryCount, result);
        if (result != TAG_ERROR_NONE)
        {
            bleDevice->connectionParamIntervalMin = 0;
        }
    }
}

STATIC_FUNCTION void connectionParamRetryTimeoutCallback(PortTimerHandle_t timer)
{
    TagPutPostWork(connectionParamRetryTimeoutHandler, timer);
}

STATIC_FUNCTION void unknownBleConnectionTimeoutHandler(TagTaskWorkParam param)
{
    PortTimerHandle_t timer = param;
    EndUserDevice *endUserDevice = gTagContext->endUserDevices;

    while (endUserDevice)
    {
        if (endUserDevice->unknownBleConnectionTimer == timer)
        {
            break;
        }
        endUserDevice = endUserDevice->next;
    }

    if (endUserDevice != NULL && endUserDevice->deviceType == END_USER_DEVICE_UNKNOWN)
    {
        TAG_LOG_E("Unknown Ble connection expired");
        endUserDevice->unknownBleConnectionTimer = NULL;
        PortBleGapDisconnect(&endUserDevice->portConnHandle);
    }

    PortTimerDelete(timer, 0);
}

STATIC_FUNCTION void unknownBleConnectionTimeoutCallback(PortTimerHandle_t timer)
{
    TagPutPostWork(unknownBleConnectionTimeoutHandler, timer);
}

STATIC_FUNCTION void paringRequestTimeoutHandler(TagTaskWorkParam param)
{
    PortTimerHandle_t timer = param;
    EndUserDevice *bleDevice;
    bleDevice = (EndUserDevice *)PortTimerGetTimerId(timer);

    if (bleDevice == NULL)
    {
        TAG_LOG_E("Paring request device is disconnected");
        return;
    }

    TAG_LOG_I("Paring Request timeout");
    bleDevice->isParing = 0;
}

STATIC_FUNCTION void paringRequestTimeoutCallback(PortTimerHandle_t timer)
{
    TagPutPostWork(paringRequestTimeoutHandler, timer);
}

STATIC_FUNCTION bool isParing(void)
{
    EndUserDevice *iter = gTagContext->endUserDevices;

    while (iter)
    {
        if (iter->isParing)
        {
            return true;
        }
        iter = iter->next;
    }

    return false;
}

STATIC_VARIABLE TagBleDeviceId lastDeviceId = 0;
STATIC_FUNCTION int tagBleConnected(BleEvent *event)
{
    EndUserDevice *endUserDevice = NULL;

    if (gTagContext->currentConnections >= gTagContext->maxAllowedConnections)
    {
        TAG_LOG_E("Exceed connections - %u", gTagContext->currentConnections);
        PortBleGapDisconnect(&event->eventData.connectionData.portConnHandle);
    }
    else if (gTagContext->state == TAG_STATE_OUT_OF_BOX && gTagContext->currentConnections >= 1)
    {
        TAG_LOG_I("No more connections in OOB state");
        PortBleGapDisconnect(&event->eventData.connectionData.portConnHandle);
    }
    else if (isParing())
    {
        TAG_LOG_I("Paring is ongoing. Reject other connection.");
        PortBleGapDisconnect(&event->eventData.connectionData.portConnHandle);
    }
    else
    {
        /* Create and add end user device struct */
        endUserDevice = TagMalloc(sizeof(EndUserDevice));
        if (endUserDevice == NULL)
        {
            TAG_LOG_E("Failed to alloc endUserDevice");
            return -1;
        }
        memset(endUserDevice, '\0', sizeof(EndUserDevice));
        if (event->eventData.connectionData.isBond)
        {
            TAG_LOG_I("Paired connection");
            endUserDevice->isPaired = 1;
            gTagContext->pairedConnection = 1;
        }
        endUserDevice->deviceId = lastDeviceId++;
        if (lastDeviceId == LIMIT_DEVICE_ID_VALUE)
        {
            lastDeviceId = 0;
        }
        endUserDevice->seqNoT = 1;
        endUserDevice->indicationQueue = PortQueueCreate(INDICATION_QUEUE_LENGTH, sizeof(IndicationData_t));
        if (endUserDevice->indicationQueue == NULL)
        {
            TAG_LOG_E("Failed to create queue");
            TagFree(endUserDevice);
            return -1;
        }
        endUserDevice->connectionParmRecoveryTimer = PortTimerCreate("ConnectionParmRecoveryTimeout",                                                 /* Text name. */
                                                                     CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(T_CONNECTION_PARAM_UPDATE_RECOVERY_TIMEOUT)), /* Timer period. */
                                                                     false,                                                                           /* Disable auto reload. */
                                                                     endUserDevice,                                                                   /* ID as tagContext */
                                                                     connectionParamRecoveryTimeoutCallback);                                         /* The callback function. */
        endUserDevice->initConnectionParmRecoveryTimer = PortTimerCreate("InitConnectionParmRecoveryTimeout",                                         /* Text name. */
                                                                         CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(T_CONNECTION_PARAM_UPDATE_TIMEOUT)),      /* Timer period. */
                                                                         false,                                                                       /* Disable auto reload. */
                                                                         endUserDevice,                                                               /* ID as tagContext */
                                                                         connectionParamRecoveryTimeoutCallback);                                     /* The callback function. */
        endUserDevice->connectionParmRetryTimer = PortTimerCreate("ConnectionParmRetryTimeout",                                                       /* Text name. */
                                                                  CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(T_CONNECTION_PARAM_UPDATE_RETRY_TIMEOUT)),       /* Timer period. */
                                                                  false,                                                                              /* Disable auto reload. */
                                                                  endUserDevice,                                                                      /* ID as tagContext */
                                                                  connectionParamRetryTimeoutCallback);                                               /* The callback function. */

        endUserDevice->unknownBleConnectionTimer = PortTimerCreate("UnknownBleConnectionTimeout",                                                 /* Text name. */
                                                                   CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(UNKNOWN_BLE_CONNECTION_TIMEOUT)),           /* Timer period. */
                                                                   false,                                                                         /* Disable auto reload. */
                                                                   0,                                                                             /* ID as tagContext */
                                                                   unknownBleConnectionTimeoutCallback);                                          /* The callback function. */

        endUserDevice->paringRequestTimer = PortTimerCreate("ParingRequestTimeout",                                                                   /* Text name. */
                                                            CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(PARING_REQUEST_TIMEOUT)),                              /* Timer period. */
                                                            false,                                                                                    /* Disable auto reload. */
                                                            endUserDevice,                                                                            /* ID as tagContext */
                                                            paringRequestTimeoutCallback);                                                            /* The callback function. */
        PortBleCopyConnHandle(&endUserDevice->portConnHandle, &event->eventData.connectionData.portConnHandle);
        if (gTagContext->endUserDevices == NULL)
        {
            gTagContext->endUserDevices = endUserDevice;
        }
        else
        {
            EndUserDevice *tmp = gTagContext->endUserDevices;
            while (tmp->next)
            {
                tmp = tmp->next;
            }
            tmp->next = endUserDevice;
        }
        TagAuthConnectionInit(gSecuContext, endUserDevice);

        PortTimerStart(endUserDevice->unknownBleConnectionTimer, 0);

        gTagContext->needAdvAddrChanged = 1;
        TAG_LOG_I("BleConnected (%u), n:%u", endUserDevice->deviceId, gTagContext->currentConnections);
    }
    TagRefreshAdv();

    TAG_MEM_CHECK("Ble connection");
    return 0;
}

STATIC_FUNCTION int tagBleDisconnected(BleEvent *event)
{
    EndUserDevice *endUserDevice = NULL;

    endUserDevice = TagFindEndUserDeviceFromPortHandle(&event->eventData.disconnectionData.portConnHandle);
    if (endUserDevice == NULL)
    {
        TAG_LOG_E("No Matched Devices");
    }
    else
    {
        TAG_LOG_I("link disconnected (%u), CNum %u", endUserDevice->deviceId, gTagContext->currentConnections);
        if (endUserDevice->isPaired)
        {
            TAG_LOG_I("Paired link disconnected");
            gTagContext->pairedConnection = 0;
        }
        if (endUserDevice->deviceType == END_USER_DEVICE_OWNER)
        {
            gTagContext->currentConnections--;
            if (gTagContext->state == TAG_STATE_CONNECTED && gTagContext->currentConnections == 0)
            {
                TagTransferState(TAG_STATE_PREMATURE_OFFLINE);
            }
            else if (gTagContext->state == TAG_STATE_OUT_OF_BOX && gTagContext->currentConnections == 0)
            {
                ResetOOBFastAdvertisingTimer();
            }
        }
        if (endUserDevice->deviceType == END_USER_DEVICE_NON_OWNER)
        {
            gTagContext->nonOwnerConnections--;
        }
        PortTimerSetTimerId(endUserDevice->connectionParmRecoveryTimer, NULL);
        PortTimerDelete(endUserDevice->connectionParmRecoveryTimer, 0);
        PortTimerSetTimerId(endUserDevice->initConnectionParmRecoveryTimer, NULL);
        PortTimerDelete(endUserDevice->initConnectionParmRecoveryTimer, 0);
        PortTimerSetTimerId(endUserDevice->connectionParmRetryTimer, NULL);
        PortTimerDelete(endUserDevice->connectionParmRetryTimer, 0);
        PortTimerSetTimerId(endUserDevice->paringRequestTimer, NULL);
        PortTimerDelete(endUserDevice->paringRequestTimer, 0);
        if (endUserDevice->unknownBleConnectionTimer)
        {
            PortTimerDelete(endUserDevice->unknownBleConnectionTimer, 0);
            endUserDevice->unknownBleConnectionTimer = NULL;
        }
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

        EndUserDevice *tmp = gTagContext->endUserDevices;
        if (tmp->deviceId == endUserDevice->deviceId)
        {
            gTagContext->endUserDevices = gTagContext->endUserDevices->next;
        }
        else
        {
            while (tmp->next->deviceId != endUserDevice->deviceId)
            {
                tmp = tmp->next;
            }
            tmp->next = endUserDevice->next;
        }
        PortBleDestroyConnHandle(&endUserDevice->portConnHandle);
        TagFree(endUserDevice);
        gTagContext->needAdvAddrChanged = 1;
    }
    TagRefreshAdv();

    if (gTagContext->currentConnections == 0)
    {
        if (TagSoundIsRingtonePlaying())
        {
            TAG_LOG_D("No end user device connected. Stop ringtone");
            TagSoundPlayStop();
        }

#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
        if (TagLedBlinkIsBlinking())
        {
            TAG_LOG_I("Led Blinking Stop");
            TagLedBlinkCtrlStop();
        }
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

    }
    TagOnboardingCheckOnboardingFailRecovery(event);

    TAG_MEM_CHECK("Ble disconnection");
    return 0;
}

STATIC_FUNCTION int tagBleConnectionParamsUpdated(BleEvent *event)
{
    EndUserDevice *endUserDevice = gTagContext->endUserDevices;
    uint16_t connInterval = event->eventData.paramsData.connInterval;
    uint16_t connLatency = event->eventData.paramsData.connLatency;
    uint16_t supervisionTimeout = event->eventData.paramsData.supervisionTimeout;

    endUserDevice = TagFindEndUserDeviceFromPortHandle(&event->eventData.paramsData.portConnHandle);
    if (endUserDevice == NULL)
    {
        TAG_LOG_E("No Connected Devices");
        return -1;
    }

    if (event->eventData.paramsData.status == CON_PARAMS_UPDATE_FAIL) {
        TAG_LOG_I("Fail to update connection params");
        if (endUserDevice->connectionParamIntervalMin != 0 &&
                    endUserDevice->connectionParamUpdateRetryCount > 0)
        {
            TAG_LOG_I("Retry after 10s");
            PortTimerStart(endUserDevice->connectionParmRetryTimer, 0);
        }
        else
        {
            endUserDevice->connectionParamIntervalMin = 0;
        }
        return 0;
    }

    TAG_LOG_I("ParamUpdate (%u), Int=%u(%ums), Lat=%u, TO=%u", endUserDevice->deviceId, event->eventData.paramsData.connInterval * 1250, \
    event->eventData.paramsData.connInterval, event->eventData.paramsData.connLatency, event->eventData.paramsData.supervisionTimeout * 10);

    /* If requested param is not recovery param, check recovery logic */
    if (!(endUserDevice->connectionParamIntervalMin == CONVERT_CONNECTION_INTERVAL_TO_API_UNIT(DEFAULT_CONNECTION_INTERVAL_MIN) &&
          endUserDevice->connectionParamIntervalMax == CONVERT_CONNECTION_INTERVAL_TO_API_UNIT(DEFAULT_CONNECTION_INTERVAL_MAX) &&
          endUserDevice->connectionParamSlaveLatency == DEFAULT_CONNECTION_LATENCY &&
          endUserDevice->connectionParamTimeoutMultiplier == CONVERT_CONNECTION_SUPERVISION_TO_API_UNIT(DEFAULT_CONNECTION_SUPERVISION_TIMEOUT)))
    {
        if (!IsTagStateOOB(gTagContext) && (CONVERT_CONNECTION_INTERVAL_TO_API_UNIT(DEFAULT_CONNECTION_INTERVAL_MIN) > connInterval ||
                                            CONVERT_CONNECTION_INTERVAL_TO_API_UNIT(DEFAULT_CONNECTION_INTERVAL_MAX) < connInterval ||
                                            DEFAULT_CONNECTION_LATENCY != connLatency ||
                                            CONVERT_CONNECTION_SUPERVISION_TO_API_UNIT(DEFAULT_CONNECTION_SUPERVISION_TIMEOUT) != supervisionTimeout))
        {
            PortTimerChangePeriod(endUserDevice->connectionParmRecoveryTimer, CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(T_CONNECTION_PARAM_UPDATE_RECOVERY_TIMEOUT)), 0);
        }
    }
    else
    {
        if (CONVERT_CONNECTION_INTERVAL_TO_API_UNIT(DEFAULT_CONNECTION_INTERVAL_MIN) <= connInterval &&
            CONVERT_CONNECTION_INTERVAL_TO_API_UNIT(DEFAULT_CONNECTION_INTERVAL_MAX) >= connInterval &&
            DEFAULT_CONNECTION_LATENCY == connLatency &&
            CONVERT_CONNECTION_SUPERVISION_TO_API_UNIT(DEFAULT_CONNECTION_SUPERVISION_TIMEOUT) == supervisionTimeout)
        {
            PortTimerStop(endUserDevice->connectionParmRecoveryTimer, 0);
        }
    }
    if (endUserDevice->connectionParamIntervalMin != 0)
    {
        if (endUserDevice->connectionParamUpdateRetryCount > 0 &&
            (endUserDevice->connectionParamIntervalMin > connInterval ||
             endUserDevice->connectionParamIntervalMax < connInterval ||
             endUserDevice->connectionParamSlaveLatency != connLatency ||
             endUserDevice->connectionParamTimeoutMultiplier != supervisionTimeout))
        {
            TagError_t result;
            result = PortBleRequestConnectionParameters(&endUserDevice->portConnHandle,
                                                        endUserDevice->connectionParamIntervalMin,
                                                        endUserDevice->connectionParamIntervalMax,
                                                        endUserDevice->connectionParamSlaveLatency,
                                                        endUserDevice->connectionParamTimeoutMultiplier);
            endUserDevice->connectionParamUpdateRetryCount--;
            TAG_LOG_I("Retry setParam %d~%d Lat:%d TO:%d R:%d Res:%d",
                      endUserDevice->connectionParamIntervalMin, endUserDevice->connectionParamIntervalMax,
                      endUserDevice->connectionParamSlaveLatency, endUserDevice->connectionParamTimeoutMultiplier,
                      endUserDevice->connectionParamUpdateRetryCount, result);
            if (result != TAG_ERROR_NONE)
            {
                endUserDevice->connectionParamIntervalMin = 0;
            }
        }
        else
        {
            endUserDevice->connectionParamIntervalMin = 0;
        }
    }
    return 0;
}

STATIC_FUNCTION TagControlServiceData *getControlServiceData(EndUserDevice *endUserDevice, uint8_t charIndex, void *value, size_t valueLength)
{
    uint32_t seqNo;
    TagError_t error = TAG_ERROR_NONE;
    TagSecurityBuffer_t decryptedData = {0};
    TagControlServiceData *controlServiceData = NULL;

    if (TagCharIsEncrypted(charIndex))
    {
        error = TagAuthDecryptData(value, valueLength, &decryptedData, endUserDevice, endUserDevice->commandKeyType);
        if (error)
        {
            TAG_LOG_E("Failed to get plain event data");
            return NULL;
        }
        controlServiceData = AllocateTagControlServiceData(decryptedData.len - TAG_CONTROL_SEQUENCE_LENGTH);
        if (!controlServiceData)
        {
            TAG_LOG_E("Failed to allocate control service data");
            TagCryptoSecurityBufferFree(&decryptedData);
            return NULL;
        }
        memcpy(controlServiceData->aSeqValue, decryptedData.p, decryptedData.len);
        TagCryptoSecurityBufferFree(&decryptedData);
    }
    else
    {
        controlServiceData = AllocateTagControlServiceData(valueLength - TAG_CONTROL_SEQUENCE_LENGTH);
        if (!controlServiceData)
        {
            TAG_LOG_E("Failed to allocate control service data");
            return NULL;
        }
        memcpy(controlServiceData->aSeqValue, value, valueLength);
    }

    seqNo = readUint32LittleEndian(controlServiceData->aSeqValue);

    if (endUserDevice->seqNoE < seqNo)
    {
        endUserDevice->seqNoE = seqNo;
    }
    else
    {
        TAG_LOG_E("sequence number error seqno_E : %u last seqno_E %u", seqNo, endUserDevice->seqNoE);
        FreeTagControlServiceData(controlServiceData);
        return NULL;
    }

    return controlServiceData;
}

STATIC_FUNCTION int tagBleCccdWritten(EndUserDevice *endUserDevice, uint8_t charIndex)
{
    TAG_LOG_I("ccc %u (%u)",  charIndex, endUserDevice->deviceId);

    if (CTRL_CHAR_START <= charIndex && charIndex < CTRL_CHAR_END)
    {
#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
        if (endUserDevice->deviceType == END_USER_DEVICE_OWNER &&
            gTagContext->requestAdvIntervalActiveFlag &&
            !endUserDevice->sentConnectionReason &&
            charIndex == CTRL_BUTTON)
        {
            uint32_t buttonEvent;
            TagControlServiceData *data;
            TagBleError_t bleRet = 0;

            endUserDevice->sentConnectionReason = 1;
            buttonEvent = (uint32_t)PortTimerGetTimerId(gTagContext->requestAdvIntervalTimer);
            data = AllocateTagControlServiceData(1);
            if (data == NULL)
            {
                TAG_LOG_E("Failed to allocate data");
                CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_ERROR), SOUND_NOT_PLAYED, "Failed to play ERROR sound");
            }
            else
            {
                data->aValue[0] = buttonEvent;
                bleRet = TagControlSendIndication(endUserDevice, CTRL_BUTTON, data, ENCRYPTION_REQUIRED);
                if (bleRet)
                {
                    TAG_LOG_E("Failed to send button indication %d", bleRet);
                }
                else
                {
                    TAG_LOG_D("Send button(%u) event after connection", buttonEvent);
                    gTagContext->requestAdvIntervalActiveFlag = 0;
                    PortTimerStop(gTagContext->requestAdvIntervalTimer, 0);
                    TagRefreshAdv();
                    CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_CONFIRM), SOUND_NOT_PLAYED, "Failed to play CONFIRM sound");
                }
                FreeTagControlServiceData(data);
            }
        }
#endif // TAG_ACCESSORY_OPTION_BUTTON_ACTION
    }

    return 0;
}

STATIC_FUNCTION TagBleError_t tagBleAttributeWritten(BleEvent *event)
{
    EndUserDevice *endUserDevice = TagFindEndUserDeviceFromPortHandle(&event->eventData.gattData.portConnHandle);
    uint8_t charIndex = TagCharGetIndexFromHandle(event->eventData.gattData.portAttrInfo.handle);
    TagBleError_t result = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    TagControlServiceData *controlServiceData = NULL;

    if (charIndex == TAG_CHAR_INVALID)
    {
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    if (gTagChar[charIndex].handleCccd == event->eventData.gattData.portAttrInfo.handle)
    {
        result = tagBleCccdWritten(endUserDevice, charIndex);
        goto sendStatus;
    }

#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
    if (charIndex != CTRL_FIRMWARE_TRANSFER)
    {
        TAG_LOG_I("wrt %u (%u)",  charIndex, endUserDevice->deviceId);
    }
#endif // TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE

    event->eventData.gattData.charIndex = charIndex;
    if ((AUTH_CHAR_START <= charIndex) && (charIndex < AUTH_CHAR_END))
    {
        BleEvent *authEvent = TagMalloc(sizeof(BleEvent));
        if (!authEvent)
        {
            TAG_LOG_E("Failed to alloc authEvent");
            result = TAG_BLE_ERROR_AUTHENTICATION_FAILURE;
            goto sendStatus;
        }
        memcpy(authEvent, event, sizeof(BleEvent));
        authEvent->eventData.gattData.value = TagMalloc(authEvent->eventData.gattData.valueLength);
        if (!authEvent->eventData.gattData.value)
        {
            TAG_LOG_E("Failed to alloc authEvent gattData value");
            result = TAG_BLE_ERROR_AUTHENTICATION_FAILURE;
            TagFree(authEvent);
            goto sendStatus;
        }
        memcpy(authEvent->eventData.gattData.value, event->eventData.gattData.value, authEvent->eventData.gattData.valueLength);

        result = AuthServiceWrittenCallback(authEvent);
        if (result != TAG_BLE_ERROR_ATT_NO_ERROR)
        {
            TagFree(authEvent->eventData.gattData.value);
            TagFree(authEvent);
            goto sendStatus;
        }
        tagError = TagPutPostWork(AuthServiceWrittenPostCallback, authEvent);
        if (tagError != TAG_ERROR_NONE)
        {
            result = TAG_BLE_ERROR_AUTHENTICATION_FAILURE;
            TagFree(authEvent->eventData.gattData.value);
            TagFree(authEvent);
            goto sendStatus;
        }
    }
    else if (CTRL_CHAR_START <= charIndex && charIndex < CTRL_CHAR_END)
    {
        /* TAG Service Write Process */
        controlServiceData = getControlServiceData(endUserDevice, charIndex, event->eventData.gattData.value, event->eventData.gattData.valueLength);
        if (controlServiceData)
        {
            result = TagControlServiceWrittenCallback(endUserDevice, charIndex, controlServiceData);
        }
        else
        {
            result = TAG_BLE_ERROR_INSUFFICIENT_ENCRYPTION;
        }
    }
    else if (ONBD_CHAR_START <= charIndex && charIndex < ONBD_CHAR_END)
    {
        result = TagOnboardingCallback(event);
    }

sendStatus:
    if (event->eventData.gattData.needResponse)
    {
        tagError = PortBleGattsSendAttrWrittenStatus(&event->eventData.gattData, result);
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to send Ble gatts attributeWrittenStatus[err:%d]", tagError);
            goto exit;
        }
    }

    if (result == TAG_BLE_ERROR_ATT_NO_ERROR
#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
        && charIndex != CTRL_FIRMWARE_TRANSFER
#endif /* TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE */
    )
    {
        if (CTRL_CHAR_START <= charIndex && charIndex < CTRL_CHAR_END && controlServiceData)
        {
            TagControlServiceGattData *gattData = TagMalloc(sizeof(TagControlServiceGattData));
            if (gattData)
            {
                gattData->deviceId = endUserDevice->deviceId;
                gattData->charIndex = charIndex;
                gattData->data = DuplicateTagControlServiceData(controlServiceData);
                TagPutPostWork(TagControlServiceWrittenPostCallback, gattData);
            }
            else
            {
                TAG_LOG_E("Failed to allocate TagControlServiceGattData");
            }
        }
    }

exit:
    if (controlServiceData)
    {
        FreeTagControlServiceData(controlServiceData);
    }

    return result;
}

STATIC_FUNCTION bool sendQueuedIndicationIfExist(EndUserDevice *endUserDevice)
{
    if (!IS_QUEUE_EMPTY(endUserDevice->indicationQueue))
    {
        IndicationData_t queueData = {
            .ignoreFlag = true};

        do
        {
            if (IS_QUEUE_EMPTY(endUserDevice->indicationQueue))
            {
                break;
            }
            if (PortQueueReceive(endUserDevice->indicationQueue, &queueData, 0) != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to receive queue");
                break;
            }
        } while (queueData.ignoreFlag);

        if (queueData.ignoreFlag == false)
        {
            TagControlSendImmediateIndication(endUserDevice, queueData.charIndex, queueData.data, queueData.encryptFlag);
            FreeTagControlServiceData(queueData.data);

            return true;
        }
        else
        {
            TAG_LOG_D("All remaining queued indications are ignorable");
        }
    }
    return false;
}

STATIC_FUNCTION int tagBleHandleValueConfirmation(BleEvent *event)
{
    TagBleError_t result = TAG_BLE_ERROR_ATT_NO_ERROR;
    EndUserDevice *endUserDevice = TagFindEndUserDeviceFromPortHandle(&event->eventData.gattData.portConnHandle);

    if (gTagContext->onBoardingStatus < ONBOARDING_COMPLETE)
    {
        result = TagOnboardingTransferLogData(event);
        if (result != TAG_BLE_ERROR_ATT_NO_ERROR)
        {
            TAG_LOG_E("Logging data transfer fail");
            return result;
        }
    }
    else if (endUserDevice->controlServiceIndicationProgress)
    {
        endUserDevice->controlServiceIndicationProgress = 0;
        sendQueuedIndicationIfExist(endUserDevice);
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
        if (!endUserDevice->controlServiceIndicationProgress && endUserDevice->logging && endUserDevice->logParam)
        {
            TagControlServiceData *data;
            size_t offset = 0, maxLogSegmentSize, segmentedLogSize;
            uint16_t mtuSize;
            TagError_t ret = TAG_ERROR_NONE;

            if (endUserDevice->logParam->writtenLen >= TagLogGetSize(endUserDevice->logParam))
            {
                TAG_LOG_I("Log Done : %u / %u (%u + %u)",
                          endUserDevice->logParam->writtenLen,
                          endUserDevice->logParam->debugLogSize + endUserDevice->logParam->dumpSize,
                          endUserDevice->logParam->dumpSize,
                          endUserDevice->logParam->debugLogSize);
                TagLogParamDeinit(endUserDevice->logParam);
                endUserDevice->logParam = NULL;
                endUserDevice->logging = 0;
                return 0;
            }

            ret = PortBleGetMtu(&endUserDevice->portConnHandle, &mtuSize);
            if (ret != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to get MTU %d", ret);
                TagLogParamDeinit(endUserDevice->logParam);
                endUserDevice->logParam = NULL;
                endUserDevice->logging = 0;
                return 0;
            }

            data = AllocateTagControlServiceData(mtuSize);
            if (data == NULL)
            {
                TAG_LOG_E("Failed to allocate data");
                TagLogParamDeinit(endUserDevice->logParam);
                endUserDevice->logParam = NULL;
                endUserDevice->logging = 0;
                return 0;
            }

            data->aValue[offset++] = DEBUG_TRANSFER_LOG_DATA_OPCODE;
            offset += writeLittleEndian(data->aValue, offset, endUserDevice->logParam->writtenLen, sizeof(uint32_t));
            maxLogSegmentSize = (((mtuSize - 3) - (mtuSize - 3) % 16) - 16);
            segmentedLogSize = TagLogGetLog(endUserDevice->logParam, (char *)data->aValue + DEBUG_TRANSFER_LOG_DATA_HEAD_LENGTH, maxLogSegmentSize, LOG_FROM_FLASH);
            offset += writeLittleEndian(data->aValue, offset, segmentedLogSize, sizeof(uint16_t));
            data->cValueLength = segmentedLogSize + DEBUG_TRANSFER_LOG_DATA_HEAD_LENGTH;

            TagControlSendIndication(endUserDevice, CTRL_DEBUG_TAG, data, ENCRYPTION_REQUIRED);
            FreeTagControlServiceData(data);
        }
        else
        {
            TAG_LOG_I("cfm (%u)", endUserDevice->deviceId);
        }
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
    }

    return 0;
}

STATIC_FUNCTION TagBleError_t tagBleAttributeRead(BleEvent *event)
{
    EndUserDevice *endUserDevice = TagFindEndUserDeviceFromPortHandle(&event->eventData.gattData.portConnHandle);
    uint8_t charIndex = TagCharGetIndexFromHandle(event->eventData.gattData.portAttrInfo.handle);
    TagBleError_t result = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    TagSecurityBuffer_t encryptedData = {0};
    uint8_t *readData = NULL;
    size_t readDataLength = 0;

    if (charIndex == TAG_CHAR_INVALID)
    {
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    TAG_LOG_I("rd %u (%u)", charIndex, endUserDevice->deviceId);
    event->eventData.gattData.charIndex = charIndex;
    if (AUTH_CHAR_START <= charIndex && charIndex < AUTH_CHAR_END)
    {
        result = AuthServiceReadCallback(event);
    }
    else if (CTRL_CHAR_START <= charIndex && charIndex < CTRL_CHAR_END)
    {
        result = TagControlServiceReadCallback(endUserDevice, charIndex, &readData, &readDataLength);
        if (result == TAG_BLE_ERROR_ATT_NO_ERROR && TagCharIsEncrypted(charIndex))
        {
            tagError = TagAuthEncryptData(readData, readDataLength, &encryptedData,
                                          endUserDevice, endUserDevice->commandKeyType);
            TagFree(readData);
            if (tagError != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to encrypt data %d (%d)", tagError, __LINE__);
                result = TAG_BLE_ERROR_INSUFFICIENT_ENCRYPTION;
            }
            else
            {
                event->eventData.gattData.value = encryptedData.p;
                event->eventData.gattData.valueLength = encryptedData.len;
            }
        }
        else
        {
            event->eventData.gattData.value = readData;
            event->eventData.gattData.valueLength = readDataLength;
        }
    }
    else if (ONBD_CHAR_START <= charIndex && charIndex < ONBD_CHAR_END)
    {
        result = TagOnboardingCallback(event);
    }

    tagError = PortBleGattsSendAttrReadStatus(&event->eventData.gattData, result);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to Ble gatts send attributeReadStatus[err:%d]", tagError);
    }
    if (result == TAG_BLE_ERROR_ATT_NO_ERROR && event->eventData.gattData.value)
    {
        TagFree(event->eventData.gattData.value);
    }

    return result;
}

STATIC_FUNCTION int tagBleConfirmationError(BleEvent *event)
{
    EndUserDevice *endUserDevice = TagFindEndUserDeviceFromPortHandle(&event->eventData.gattData.portConnHandle);

    if (endUserDevice->controlServiceIndicationProgress)
    {
        endUserDevice->controlServiceIndicationProgress = 0;
        sendQueuedIndicationIfExist(endUserDevice);
    }
    return 0;
}

STATIC_FUNCTION int tagBleBondingStatus(BleEvent *event)
{
    EndUserDevice *endUserDevice = TagFindEndUserDeviceFromPortHandle(&event->eventData.bondingStatusData.portConnHandle);

    if (endUserDevice == NULL)
    {
        TAG_LOG_E("Not end user device");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    if (event->eventData.bondingStatusData.bondingStatus == BONDING_STATUS_REQUEST)
    {
        TAG_LOG_I("Paring request %s [%u]", PortTimerIsTimerActive(endUserDevice->paringRequestTimer) ? "Accepted" : "Rejected",
                  endUserDevice->deviceId);
        PortBleGapBondingReply(&endUserDevice->portConnHandle, PortTimerIsTimerActive(endUserDevice->paringRequestTimer));
        if (PortTimerIsTimerActive(endUserDevice->paringRequestTimer))
        {
            PortTimerStop(endUserDevice->paringRequestTimer, 0);
        }
    }
    else if (event->eventData.bondingStatusData.bondingStatus == BONDING_STATUS_SUCCESS_BOND)
    {
        TAG_LOG_I("Bonding success %u", endUserDevice->deviceId);
        PortBleGapRemoveOtherBondings(&endUserDevice->portConnHandle);
        endUserDevice->isPaired = 1;
        gTagContext->pairedConnection = 1;
        endUserDevice->isParing = 0;
        TagRefreshAdv();
    }
    else if (event->eventData.bondingStatusData.bondingStatus == BONDING_STATUS_FAIL_BOND)
    {
        TAG_LOG_I("Bonding fail %u", endUserDevice->deviceId);
        endUserDevice->isParing = 0;
    }
    else
    {
        endUserDevice->isParing = 0;
        TAG_LOG_E("Undefined status %u", event->eventData.bondingStatusData.bondingStatus);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    return 0;
}

#ifdef TAG_CONFIG_USE_NOTIFICATION_FOR_DEBUG
STATIC_FUNCTION TagBleError_t tagBleNotificationSent(BleEvent *event)
{
    TagBleError_t tagError = TAG_BLE_ERROR_ATT_NO_ERROR;
    EndUserDevice *endUserDevice = TagFindEndUserDeviceFromPortHandle(&event->eventData.gattData.portConnHandle);

    if (endUserDevice->controlServiceNotificationProgress)
    {
        endUserDevice->controlServiceNotificationProgress = 0;
        if (endUserDevice->logging && endUserDevice->logParam)
        {
            TagControlServiceData *data = NULL;
            size_t offset = 0, maxLogSegmentSize, segmentedLogSize;
            uint16_t mtuSize = 0;

            if (endUserDevice->logParam->writtenLen >= TagLogGetSize(endUserDevice->logParam))
            {
                TAG_LOG_I("Log Done: %u / %u (%u + %u)",
                            endUserDevice->logParam->writtenLen,
                            endUserDevice->logParam->debugLogSize + endUserDevice->logParam->dumpSize,
                            endUserDevice->logParam->dumpSize,
                            endUserDevice->logParam->debugLogSize);
                TagLogParamDeinit(endUserDevice->logParam);
                endUserDevice->logParam = NULL;
                endUserDevice->logging = 0;
                return tagError;
            }

            tagError = PortBleGetMtu(&endUserDevice->portConnHandle, &mtuSize);
            if (tagError != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to get MTU %d", tagError);
                TagLogParamDeinit(endUserDevice->logParam);
                endUserDevice->logParam = NULL;
                endUserDevice->logging = 0;
                return tagError;
            }

            data = AllocateTagControlServiceData(mtuSize);
            if (data == NULL)
            {
                TAG_LOG_E("Failed to allocate data");
                TagLogParamDeinit(endUserDevice->logParam);
                endUserDevice->logParam = NULL;
                endUserDevice->logging = 0;
                return TAG_BLE_ERROR_UNDEFINED_ERROR;
            }

            data->aValue[offset++] = DEBUG_TRANSFER_LOG_DATA_OPCODE;
            offset += writeLittleEndian(data->aValue, offset, endUserDevice->logParam->writtenLen, sizeof(uint32_t));
            maxLogSegmentSize = (((mtuSize - 3) - (mtuSize - 3) % 16) - 16);
            segmentedLogSize = TagLogGetLog(endUserDevice->logParam, (char *)data->aValue + DEBUG_TRANSFER_LOG_DATA_HEAD_LENGTH, maxLogSegmentSize, LOG_FROM_FLASH);
            offset += writeLittleEndian(data->aValue, offset, segmentedLogSize, sizeof(uint16_t));
            data->cValueLength = segmentedLogSize + DEBUG_TRANSFER_LOG_DATA_HEAD_LENGTH;

            TagControlSendNotification(endUserDevice, CTRL_DEBUG_TAG, data, ENCRYPTION_REQUIRED);

            FreeTagControlServiceData(data);

            return tagError;
        }
        else
        {
            TAG_LOG_I("noti (%u)", endUserDevice->deviceId);
        }
    }

    return tagError;
}
#endif

TagBleError_t TagBleCallback(BleEvent *event)
{
    EndUserDevice *endUserDevice = NULL;

    /* Update connection parameters timeout if there is interaction with a end user device */
    switch (event->eventType)
    {
    case BleHandleValueConfirmation:
    case BleAttributeWritten:
    case BleAttributeRead:
#ifdef TAG_CONFIG_USE_NOTIFICATION_FOR_DEBUG
    case BleNotificationSent:
#endif
        endUserDevice = TagFindEndUserDeviceFromPortHandle(&event->eventData.gattData.portConnHandle);
        if (endUserDevice == NULL)
        {
            TAG_LOG_E("No Connected Devices for event=%u", event->eventType);
            return TAG_BLE_ERROR_UNDEFINED_ERROR;
        }
        if (PortTimerIsTimerActive(endUserDevice->connectionParmRecoveryTimer) != false)
        {
            PortTimerChangePeriod(endUserDevice->connectionParmRecoveryTimer, CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(T_CONNECTION_PARAM_UPDATE_RECOVERY_TIMEOUT)), 0);
        }
        break;
    case BleConnected:
    case BleDisconnected:
    case BleConnectionParameterUpdated:
    case BleConfirmationError:
    case BleBondingStatus:
    case EndOfBleEvent:
    default:
        break;
    }

    switch (event->eventType)
    {
    case BleConnected:
        return tagBleConnected(event);
    case BleDisconnected:
        return tagBleDisconnected(event);
    case BleConnectionParameterUpdated:
        return tagBleConnectionParamsUpdated(event);
    case BleAttributeWritten:
        return tagBleAttributeWritten(event);
    case BleHandleValueConfirmation:
        return tagBleHandleValueConfirmation(event);
    case BleAttributeRead:
        return tagBleAttributeRead(event);
    case BleConfirmationError:
        return tagBleConfirmationError(event);
    case BleBondingStatus:
        return tagBleBondingStatus(event);
#ifdef TAG_CONFIG_USE_NOTIFICATION_FOR_DEBUG
    case BleNotificationSent:
       return tagBleNotificationSent(event);
#endif
    case EndOfBleEvent:
    default:
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    return TAG_BLE_ERROR_UNDEFINED_ERROR;
}
