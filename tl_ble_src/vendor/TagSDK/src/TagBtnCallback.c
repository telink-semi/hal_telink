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

#include "TagBtnCallback.h"
#include "TagCore.h"
#include "TagFwUpdate.h"
#include "TagOnboardingService.h"
#include "TagSoundPlayer.h"
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
#include "TagLedBlink.h"
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

/* port related headers */
#include "PortOs.h"

#ifdef LOG_PREFIX
#undef LOG_PREFIX
#endif
#define LOG_PREFIX "BTN"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)

static TagError_t sendButtonEventIndication(TagContext *tagContext, uint8_t event)
{
    TagControlServiceData *data;
    EndUserDevice *endUserDevice;
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;

    endUserDevice = tagContext->endUserDevices;
    if (endUserDevice == NULL)
    {
        TAG_LOG_D("No end user device");
        return TAG_ERROR_NONE;
    }

    data = AllocateTagControlServiceData(1);
    if (data == NULL)
    {
        TAG_LOG_E("Failed to allocate data");
        return TAG_ERROR_MEM_ALLOC;
    }

    data->aValue[0] = event;

    while (endUserDevice)
    {
        if (endUserDevice->deviceType == END_USER_DEVICE_OWNER)
        {
            bleError = TagControlSendIndication(endUserDevice, CTRL_BUTTON, data, ENCRYPTION_REQUIRED);
            if (bleError)
            {
                TAG_LOG_E("Failed to send button indication (0x%x) to id-0x%x (%d)", event, endUserDevice->deviceId, bleError);
                tagError = TAG_ERROR_BLE_EVENT_NOTIFY;
            }
            else
            {
                TAG_LOG_D("Button event 0x%x has sent to id-0x%x", event, endUserDevice->deviceId);
            }

            break;
        }
        endUserDevice = endUserDevice->next;
    }

    FreeTagControlServiceData(data);

    return tagError;
}
#endif // TAG_ACCESSORY_OPTION_BUTTON_ACTION

static bool isTagStateConnected(TagContext *tagContext)
{
    if (tagContext->state == TAG_STATE_CONNECTED)
    {
        return true;
    }
    return false;
}

static void processButtonEventEnabled(TagContext *tagContext, uint8_t event)
{
    bool isPlaying = TagSoundIsRingtonePlaying();
    bool isBlinking = 0;

#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
    isBlinking = TagLedBlinkIsBlinking();
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

    if (isPlaying || isBlinking)
    {
        if (isPlaying)
        {
            TAG_LOG_I("RingStop");
            TagSoundPlayStop();
        }
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
        if (isBlinking)
        {
            TAG_LOG_I("BlinkingStop");
            TagLedBlinkCtrlStop();
        }
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
        return;
    }

    if (isTagStateConnected(tagContext))
    {
        TagError_t tagError = TAG_ERROR_NONE;

#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
        tagError = sendButtonEventIndication(tagContext, event);
#endif // TAG_ACCESSORY_OPTION_BUTTON_ACTION
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("send button event (0x%x) indication failed (%d)", event, tagError);
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_ERROR), SOUND_NOT_PLAYED, "Failed to play ERROR sound");
        }
        else
        {
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_CONFIRM), SOUND_NOT_PLAYED, "Failed to play CONFIRM sound");
        }
    }
    else
    {
        if (tagContext->state == TAG_STATE_PREMATURE_OFFLINE || tagContext->state == TAG_STATE_OFFLINE || tagContext->state == TAG_STATE_OVERMATURE_OFFLINE)
        {
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_CONNECTING), SOUND_NOT_PLAYED, "Failed to play CONNECTING sound");
            PortTimerReset(tagContext->requestAdvIntervalTimer, 0);
            tagContext->requestAdvIntervalActiveFlag = 1;
            PortTimerSetTimerId(tagContext->requestAdvIntervalTimer, (void *)((uint32_t)event));
            TagRefreshAdv();
        }
    }
}

static bool isHoldButtonEventEnabled(TagContext *tagContext)
{
    return tagContext->holdButtonEnabled ? true : false;
}

static bool isPushButtonEventEnabled(TagContext *tagContext)
{
    return tagContext->pushButtonEnabled ? true : false;
}

static void processButtonEventDisabled(TagContext *tagContext)
{
    /*
     * The only difference between connect and disconnected state is sending ringtone stop indication.
     * This would be treated by AppSoundPlayStop() inside.
     */
    bool isPlaying = TagSoundIsRingtonePlaying();
    bool isBlinking = 0;

#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
    isBlinking = TagLedBlinkIsBlinking();
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

    if (isPlaying || isBlinking)
    {
        if (isPlaying)
        {
            TAG_LOG_I("RingStop");
            TagSoundPlayStop();
        }
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
        if (isBlinking)
        {
            TAG_LOG_I("BlinkingStop");
            TagLedBlinkCtrlStop();
        }
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
    }
    else
    {
        CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_NO_KEY), SOUND_NOT_PLAYED, "Failed to play NO KEY sound");
    }
}

static void TagBtnPushedCallback(void)
{
    if (gTagContext == NULL)
    {
        TAG_LOG_E("Tag context is not initialized yet");
        return;
    }

    if (IsTagStateOOB(gTagContext))
    {
        if (gTagContext->state == TAG_STATE_OOB_DEEP_SLEEP)
        {
            TagTransferState(TAG_STATE_OUT_OF_BOX);
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_BOOTING), SOUND_NOT_PLAYED, "Failed to play BOOTING sound");
        }
        else
        {
            TagOnboardingReportButtonEvent(BUTTON_PRESS);
        }
        ResetOOBFastAdvertisingTimer();
        TagRefreshAdv();
        return;
    }

    if (isPushButtonEventEnabled(gTagContext) && gTagContext->tagPSM == TAG_PSM_NORMAL)
    {
        processButtonEventEnabled(gTagContext, BUTTON_EVENT_PUSHED);
    }
    else
    {
        processButtonEventDisabled(gTagContext);
    }
}

static void TagBtnDoublePushedCallback(void)
{
    if (gTagContext == NULL)
    {
        TAG_LOG_E("Tag context is not initialized yet");
        return;
    }

    TAG_MEM_CHECK("Button double push");

    if (IsTagStateOOB(gTagContext))
    {
        if (gTagContext->state == TAG_STATE_OOB_DEEP_SLEEP)
        {
            TagTransferState(TAG_STATE_OUT_OF_BOX);
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_BOOTING), SOUND_NOT_PLAYED, "Failed to play BOOTING sound");
        }
        else
        {
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_PRESS), SOUND_NOT_PLAYED, "Failed to play PRESS sound");
        }
        ResetOOBFastAdvertisingTimer();
        TagRefreshAdv();
        return;
    }

    /* Tag Specification - 8.2.3
     * The button event for push twice pattern
     * must be always enabled and cannot be changed.
     */
    if (gTagContext->tagPSM == TAG_PSM_NORMAL) {
        processButtonEventEnabled(gTagContext, BUTTON_EVENT_PUSHED_2X);
    } else {
        processButtonEventDisabled(gTagContext);
    }
}

static void TagBtnHoldCallback(void)
{
    if (gTagContext == NULL)
    {
        TAG_LOG_E("Tag context is not initialized yet");
        return;
    }

    if (IsTagStateOOB(gTagContext))
    {
        if (gTagContext->state == TAG_STATE_OOB_DEEP_SLEEP)
        {
            TagTransferState(TAG_STATE_OUT_OF_BOX);
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_BOOTING), SOUND_NOT_PLAYED, "Failed to play BOOTING sound");
        }
        else
        {
            TagOnboardingReportButtonEvent(BUTTON_PRESS);
        }
        ResetOOBFastAdvertisingTimer();
        TagRefreshAdv();
        return;
    }

    if (isHoldButtonEventEnabled(gTagContext) && gTagContext->tagPSM == TAG_PSM_NORMAL)
    {
        processButtonEventEnabled(gTagContext, BUTTON_EVENT_HELD);
    }
    else
    {
        processButtonEventDisabled(gTagContext);
    }
}

static bool isButtonEventIgnoreScenario(TagContext *context)
{
    if ((context == NULL)
#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
        || (FwUpdateGetState() != FW_UPDATE_STATE_IDLE)
#endif /* TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE */
    )
    {
        return true;
    }
    else
    {
        return false;
    }
}

STATIC_FUNCTION void systemButtonEventPostWork(TagTaskWorkParam param)
{
    ButtonEventType eventType = (ButtonEventType)param;

    if (isButtonEventIgnoreScenario(gTagContext))
    {
        TAG_LOG_I("Ignore event(%u)", eventType);
        return;
    }

    /* This context should not be held long. So call pending task */
    switch (eventType)
    {
    case EVENT_BUTTON_PUSHED:
        TAG_LOG_I("pushed");
        TagBtnPushedCallback();
        break;
    case EVENT_BUTTON_DOUBLE_PUSHED:
        TAG_LOG_I("double pushed");
        TagBtnDoublePushedCallback();
        break;
    case EVENT_BUTTON_HOLD:
        TAG_LOG_I("hold");
        TagBtnHoldCallback();
        break;
    case EVENT_BUTTON_LONG_HOLD:
        TAG_LOG_I("long hold");
        break;
    default:
        TAG_LOG_E("Unsupported event(%u)", eventType);
        break;
    }
}

void SystemButtonEventCallback(ButtonEventType eventType)
{
    if (gTagContext == NULL || !gTagContext->initialized)
    {
        TAG_LOG_E("Button type %d, Tag is not initialized yet!", eventType);
        return;
    }

    if (TagPutPostWork(systemButtonEventPostWork, (TagTaskWorkParam)eventType) != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to register work - button callback");
    }
}
