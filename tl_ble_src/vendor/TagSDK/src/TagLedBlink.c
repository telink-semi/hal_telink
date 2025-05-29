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

#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)

#include "TagControlService.h"
#include "TagCore.h"
#include "TagDebug.h"
#include "TagLedBlink.h"

#include "PortLedBlink.h"
#include "PortOs.h"

#ifdef LOG_PREFIX
#undef LOG_PREFIX
#endif
#define LOG_PREFIX "LEDB"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

/* LED blinking period(msec) */
#define LED_BLINKING_PERIOD_MS (200)

/* LED blinking Timeout(msec) */
#define LED_BLINKING_TIMEOUT_MS (60 * 1000)

/* LED status,  true -> On,  false -> Off */
STATIC_VARIABLE bool ledOnOff = false;

/* LED blink status,  true -> blinking,  false -> Off */
STATIC_VARIABLE bool isLedBlinking = false;

STATIC_VARIABLE PortTimerHandle_t ledBlinkSwitchTimer;
STATIC_VARIABLE PortTimerHandle_t ledBlinkCtrlTimer;  // LED Blinking should be turn off after LED_BLINKING_TIMEOUT_MS

STATIC_FUNCTION void ledBlinkSwitchTimerHandler(PortTimerHandle_t timer);
STATIC_FUNCTION void ledBlinkCtrlTimerHandler(PortTimerHandle_t timer);
STATIC_FUNCTION TagError_t tagLEDBlinkingCtrlWork(bool blinkOnOffCtrl);
STATIC_FUNCTION TagError_t sendLedBlinkEventToBle(uint8_t blinking);

TagError_t TagLedBlinkCtrlInit(void)
{
    TAG_LOG_D("TagLedBlinkCtrlInit");
    ledBlinkSwitchTimer = PortTimerCreate("LedBlinkSwitch", LED_BLINKING_PERIOD_MS / 2, true, NULL, ledBlinkSwitchTimerHandler);
    if (ledBlinkSwitchTimer == NULL)
    {
        TAG_LOG_E("Failed to create LedBlink switch timer");
        return TAG_ERROR_INVALID_RESOURCE;
    }

    ledBlinkCtrlTimer = PortTimerCreate("LedBlinkCtrl", LED_BLINKING_TIMEOUT_MS, false, NULL, ledBlinkCtrlTimerHandler);
    if (ledBlinkCtrlTimer == NULL)
    {
        TAG_LOG_E("Failed to create LedBlink control timer");
        return TAG_ERROR_INVALID_RESOURCE;
    }

    PortLedBlinkHwCtrlInit();
    PortLedBlinkHwCtrlOff();

    return TAG_ERROR_NONE;
}

void TagLedBlinkCtrlReset(void)
{
    TAG_LOG_D("TagLedBlinkCtrlReset");
    PortTimerStop(ledBlinkSwitchTimer, 0);
    PortTimerStop(ledBlinkCtrlTimer, 0);
}

STATIC_FUNCTION void ledBlinkSwitchTimerHandler(PortTimerHandle_t timer)
{
    ledOnOff = !ledOnOff;

    // If LedBlinking was shutdown, ledOnOff should be always off
    if (isLedBlinking == false)
    {
        ledOnOff = false;
    }

    TAG_LOG_D("LED - %d", ledOnOff);

    if (ledOnOff)
    {
        PortLedBlinkHwCtrlOn();
    }
    else
    {
        PortLedBlinkHwCtrlOff();
    }
}

STATIC_FUNCTION void ledBlinkCtrlTimerHandler(PortTimerHandle_t timer)
{
    if (isLedBlinking)
    {
        TAG_LOG_D("LED Blinking timeout, stop!");
        TagLedBlinkCtrlStop();
    }
}


bool TagLedBlinkIsBlinking(void)
{
    return isLedBlinking;
}


TagError_t TagLedBlinkCtrlStart(void)
{
    TAG_LOG_D("TagLedBlinkCtrlStart");
    return tagLEDBlinkingCtrlWork(true);
}

TagError_t TagLedBlinkCtrlStop(void)
{
    TAG_LOG_D("TagLedBlinkCtrlStop");
    return tagLEDBlinkingCtrlWork(false);
}

STATIC_FUNCTION TagError_t tagLEDBlinkingCtrlWork(bool blinkOnOffCtrl)
{
    TagError_t tagError = TAG_ERROR_NONE;

    // Blinking status is right, do nothing
    if(blinkOnOffCtrl == isLedBlinking)
    {
        TAG_LOG_D("Blinking status is already %d, do nothing", isLedBlinking);
        return tagError;
    }

    if (blinkOnOffCtrl)
    {
        tagError = PortTimerStart(ledBlinkSwitchTimer, 0);
        if (tagError == TAG_ERROR_NONE)
        {
            isLedBlinking = true;

            tagError = sendLedBlinkEventToBle(LED_BLINKING_VALUE_ON);
            if (tagError != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Send BLE (LED On) failed: %d", tagError);
            }

            tagError = PortTimerStart(ledBlinkCtrlTimer, 0);
            if (tagError != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to start ledBlinkCtrlTimer!");
                return tagError;
            }
        }
    }
    else
    {
        tagError = PortTimerStop(ledBlinkSwitchTimer, 0);
        if (tagError == TAG_ERROR_NONE)
        {
            isLedBlinking = false;

            tagError = sendLedBlinkEventToBle(LED_BLINKING_VALUE_OFF);
            if (tagError != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Send BLE (LED Off) failed: %d", tagError);
            }

            tagError = PortTimerStop(ledBlinkCtrlTimer, 0);
            if (tagError != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to stop ledBlinkCtrlTimer!");
                return tagError;
            }

            // turn off the LED no matter what last LED status is
            ledOnOff = false;
            tagError = PortLedBlinkHwCtrlOff();
            if (tagError != TAG_ERROR_NONE)
            {
                TAG_LOG_E("LED Off HW Failed: %d", tagError);
                return tagError;
            }
        }
    }

    return tagError;
}

STATIC_FUNCTION TagError_t sendLedBlinkEventToBle(uint8_t blinking)
{
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    TagControlServiceData *data;
    TagContext *context = gTagContext;
    EndUserDevice *endUserDevice;
    bool isNotification = false;

    TAG_LOG_D("sendLedBlinkEventToBle");

    if (context == NULL)
    {
        TAG_LOG_E("gTagContext is missing!");
        return TAG_ERROR_INVALID_RESOURCE;
    }

    endUserDevice = context->endUserDevices;
    if (endUserDevice == NULL)
    {
        TAG_LOG_D("No end user device");
        return TAG_ERROR_NONE;
    }

    data = AllocateTagControlServiceData(1);
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate memory (%d)", __LINE__);
        return TAG_ERROR_MEM_ALLOC;
    }

    data->aValue[0] = blinking;

    while (endUserDevice)
    {
        if (endUserDevice->deviceType == END_USER_DEVICE_OWNER)
        {
            bleError = TagControlSendIndication(endUserDevice, CTRL_LED_BLINKING, data, ENCRYPTION_REQUIRED);
        }
        else if (endUserDevice->deviceType == END_USER_DEVICE_NON_OWNER)
        {
            bleError = TagControlSendNotification(endUserDevice, CTRL_LED_BLINKING_NON_OWNER, data, ENCRYPTION_REQUIRED);
        }

        if (bleError != TAG_BLE_ERROR_ATT_NO_ERROR)
        {
            TAG_LOG_E("Failed to send %s to EndUserDevice-%u (%d)",
                      isNotification ? "notification" : "indication", endUserDevice->deviceId, bleError);
            tagError |= TAG_ERROR_BLE_EVENT_NOTIFY;
        }

        endUserDevice = endUserDevice->next;
    }

    FreeTagControlServiceData(data);

    return tagError;
}


#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
