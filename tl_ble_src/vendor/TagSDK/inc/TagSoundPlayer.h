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

#ifndef TAGSDK_INC_SOUNDPLAYER_H_
#define TAGSDK_INC_SOUNDPLAYER_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "TagErrorType.h"
/**
 * @brief Contains a enumeration values for types of event sound
 */
typedef enum {
    SOUND_ITEM_BOOTING,
    SOUND_ITEM_RESET_BOOTING,
    SOUND_ITEM_OFF,
    SOUND_ITEM_PRESS,
    SOUND_ITEM_NO_KEY,
    SOUND_ITEM_ERROR,
    SOUND_ITEM_CONNECTED,
    SOUND_ITEM_DISCONNECTED,
    SOUND_ITEM_CONFIRM,
    SOUND_ITEM_CONNECTING,
    SOUND_ITEM_SYSTEM_CRITICAL,
    SOUND_ITEM_MAX
} SoundItem_t;

typedef enum {
    SOUND_INDICATION_RINGTONE_OFF,
    SOUND_INDICATION_RINGTONE_SIREN,
    SOUND_NOTIFICATION_RINGTONE_OFF,
    SOUND_NOTIFICATION_RINGTONE_SIREN,
    SOUND_NOTIFICATION_RINGTONE_VOLUME_MUTE,
    SOUND_NOTIFICATION_RINGTONE_VOLUME_NORMAL,
    SOUND_NOTIFICATION_RINGTONE_VOLUME_LOUD,
} SoundBleEvent_t;

typedef enum {
    SOUND_VOLUME_MUTE = 0x00, /* refer tag spec. 8.2.2 */
    SOUND_VOLUME_NORMAL = 0x01,
    SOUND_VOLUME_LOUD = 0x02,
} SoundVolume_t;

typedef enum
{
    SOUND_TYPE_NONE,
    SOUND_TYPE_RINGTONE_FOR_OWNER,
    SOUND_TYPE_RINGTONE_FOR_NON_OWNER,
    SOUND_TYPE_FEEDBACK_CONNECTING,
    SOUND_TYPE_FEEDBACK_OTHERS,
} SoundType_t;

#define SOUND_NOT_PLAYED    (0)
/**
 * @brief Initialize sound player
 *
 * @details This function initializes sound player including volume setting.
 * @return return TAG_ERROR_NONE for success. negative values for failure.
 *
 */
TagError_t TagSoundPlayerInit(void);

/**
 * @brief Reset sound player
 *
 * @details This function reset sound player including volume setting.
 * @return return TAG_ERROR_NONE for success. negative values for failure.
 *
 */
TagError_t TagSoundPlayerReset(void);

/**
 * @brief Deinitialize sound player
 *
 * @details This function deinitializes sound player including volume setting.
 *
 */
void TagSoundPlayerDeinit(void);

/**
 * @brief Play event sound item
 *
 * @details This function starts play event one of sound item from among the SoundItem_t items
 * @param[in] SoundItem_t type item.
 *            If the item is APP_SOUND_ITEM_SEARCHING or APP_SOUND_ITEM_CONNECTING,
 *            it will be played repeatedly. so it should be stopped by AppSoundPlayStop() API.
 *            Otherwise it just play once.
 * @return returns play time of 1 iteration as millisecond. 0 for failure.
 * @see AppSoundPlayStop
 *
 */
uint32_t TagSoundPlayItem(SoundItem_t item);

/**
 * @brief Check whether connecting sound is playing or not.
 *
 * @details This function could check whether connecting sound is playing or not
 *
 * @return returns true if connecting sound is playing. otherwise false
 * @see AppSoundPlayStop
 *
 */
bool TagSoundIsConnectingPlaying(void);

/**
 * @brief Play Ringtone
 *
 * @details This function starts repeated play ringtone repeatedly.
 *          If end user device send custom ringtone by GATT service. it would be played.
 *          otherwise firmware embedded default ringtone would be played.
 *          This function could decode ringtone note between C4 and B7 - Rounding as 262 and 3951 Hz
 * @param[in] timeoutSec in second
 *            Sound play would be stopped when the elapsed time reaches given timeout value.
 *            But this play can be stopped with calling AppSoundPlayStop() within timeout.
 *            if timeoutSec is zero, it will play repeatedly until AppSoundPlayStop() called.
 * @param[in] eventType Method of ringtone control feedback
 *            SOUND_INDICATION_RINGTONE_SIREN for Owner
 *            SOUND_NOTIFICATION_RINGTONE_SIREN for Non-owner
 * @return returns play time of 1 iteration as millisecond. 0 for failure.
 * @see AppSoundPlayStop
 *
 */
uint32_t TagSoundPlayRingtone(unsigned int timeoutSec, SoundBleEvent_t eventType);

/**
 * @brief Check whether ringtone is playing or not.
 *
 * @details This function could check whether ringtone is playing or not
 *
 * @return returns true if ringtone is playing. otherwise false
 * @see AppSoundPlayStop
 *
 */
bool TagSoundIsRingtonePlaying(void);

/**
 * @brief Stop play sound
 *
 * @details This function stops currently playing sound
 *
 */
void TagSoundPlayStop(void);

/**
 * @brief Set ringtone volume
 *
 * @details This function is for setting ringtone volume.
 * @param[in] 0x00 for mute, 0x01 for normal, 0x02 for loud
 * @return TAG_ERROR_NONE for success. otherwise for failure
 *
 */
TagError_t TagSoundSetRingtoneVolume(SoundVolume_t volume);

/**
 * @brief Get ringtone volume
 *
 * @details This function is for getting ringtone volume.
 * @param[out] 0x00 for mute, 0x01 for normal, 0x02 for loud
 * @return TAG_ERROR_NONE for success. otherwise for failure
 *
 */
TagError_t TagSoundGetRingtoneVolume(SoundVolume_t *volume);


/**
 * @brief Get ringtone name
 *
 * @details This function is for getting ringtone name
 * @param[out] outBuffer output buffer
 * @param[in] outBuffer output buffer size
 * @return TAG_ERROR_NONE for success. otherwise for failure
 *
 */
TagError_t TagSoundGetRingtoneName(char *outBuffer, size_t outBufferLen);


/**
 * @brief Load custom ringtone from NV
 *
 * @details This function loads custom ringtone name and data
 * @return TAG_ERROR_NONE for success. otherwise for failure
 *
 */
TagError_t TagSoundLoadCustomRingtone(void);

/**
 * @brief Clear loaded custom ringtone from memory
 *
 * @details This function cleans up custom ringtone name and data
 *
 */
void TagSoundClearCustomRingtone(void);

/**
 * @brief Check whether ringtone running change is required
 *
 * @details This function stops current playing ringtone sound and returns true
 *          Otherwise returns false.
 *
 */
bool TagSoundIsRunningRingtoneChangeAndPrepare(void);

/**
 * @brief Finish Ringtone running change
 *
 * @details This function should be called only when AppSoundIsRunningRingtoneChangeRequired()
 *          is called with true return values AND custom ringtone should be loaded before use it.
 *          This function play confirm sound with short mute, and play updated ringtone with 60s timeout
 *
 */
void TagSoundFinishRunningRingtoneChange(void);

/**
 * @brief Set latest sound type
 *
 * @details This function is for setting the latest SoundType_t item.
 * @param[in] SoundType_t type item.
 *            If the item is SOUND_TYPE_RINGTONE_FOR_OWNER or SOUND_TYPE_RINGTONE_FOR_NON_OWNER,
 *            it will sent notification when the sound is stopped and won't sent any notification otherwise
 *
 */
 void TagSetLatestSoundType(SoundType_t type);

#endif //TAGSDK_INC_SOUNDPLAYER_H_
