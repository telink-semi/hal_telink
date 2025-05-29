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

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "TagConfig.h"

#include "TagControlService.h"
#include "TagCore.h"
#include "TagDebug.h"
#include "TagNV.h"
#include "TagSoundPlayer.h"
#include "TagUtil.h"

#include "PortBuzzerControl.h"
#include "PortOs.h"
#include "PortTime.h"
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
#include "PortUwb.h"
#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */
#include "PortSleep.h"

#ifdef LOG_PREFIX
#undef LOG_PREFIX
#endif
#define LOG_PREFIX "SND"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

typedef struct
{
    uint32_t frequency;
    uint32_t playTime;
    uint32_t resonance;
} SoundControl_t;

/*
 * (TAG_NV_RINGTONE_DATA_MAX_SZ / sizeof(MusicData_t))
 * + sizeof(MUSIC_DATA_CONFIRM) + sizeof(MUSIC_DATA_SHORT_MUTE) + sizeof(MUSIC_DATA_SHORT_MUTE)
 */
#define SOUND_CONTROL_QUEUE_LENGTH (188)

STATIC_VARIABLE PortTimerHandle_t soundPlayTimer;
STATIC_VARIABLE PortTimerHandle_t soundResTimer;
STATIC_VARIABLE PortQueueHandle_t soundCtrlQueue;
STATIC_VARIABLE volatile bool gSoundrPlaying;

STATIC_FUNCTION void soundCtrlWork(void);
STATIC_FUNCTION void soundResonanceTimeoutCallback(PortTimerHandle_t timer);
STATIC_FUNCTION void soundPlaytimeTimeoutCallback(PortTimerHandle_t timer);
STATIC_FUNCTION void setSoundPlaying(bool playing);

#define OCTAVE_OFFSET 4
#define OCTAVE_SIZE 4
#define SCORE_SIZE 6
#define RESONANCE_SCALE 5
#define MUTE_FREQ 0
#define SCALED_RESONANCE(x) (x / 5)
#define DO_NOT_USE_TIMEOUT (0)
#define T_PLAYING_CHECK_MS (20)
#define T_HW_CONTROL_MARGIN_MS (5)

typedef enum
{
    SOUND_NOTE_A,
    SOUND_NOTE_A_SHARP,
    SOUND_NOTE_B,
    SOUND_NOTE_C,
    SOUND_NOTE_C_SHARP,
    SOUND_NOTE_D,
    SOUND_NOTE_D_SHARP,
    SOUND_NOTE_E,
    SOUND_NOTE_F,
    SOUND_NOTE_F_SHARP,
    SOUND_NOTE_G,
    SOUND_NOTE_G_SHARP,
    SOUND_NOTE_MAX
} SoundNote_t;

typedef struct playerContents
{
    const uint8_t *data;
    size_t dataLen;
    uint32_t timeoutMs;
    uint32_t playStartTimeMs;
} PlayerContents_t;

#pragma pack(push, 1)
typedef struct
{
    uint8_t note[3];
    uint16_t playTime;       /* millisecond */
    uint8_t scaledResonance; /* millisecond divided by RESONANCE_SCALE(5) */
} MusicData_t;
#pragma pack(pop)

STATIC_VARIABLE const MusicData_t MUSIC_DATA_BOOTING[] = {
    {"F_4", 70, SCALED_RESONANCE(30)},
    {"C_5", 70, SCALED_RESONANCE(30)},
    {"F_5", 70, SCALED_RESONANCE(30)}};

STATIC_VARIABLE const MusicData_t MUSIC_DATA_RESET_BOOTING[] = {
    {"F_4", 70, SCALED_RESONANCE(30)},
    {"C_5", 70, SCALED_RESONANCE(30)},
    {"F_5", 70, SCALED_RESONANCE(30)},
    {"F_5", 70, SCALED_RESONANCE(30)},
    {"C_6", 70, SCALED_RESONANCE(30)},
    {"F_6", 70, SCALED_RESONANCE(30)}};

STATIC_VARIABLE const MusicData_t MUSIC_DATA_OFF[] = {
    {"F_5", 70, SCALED_RESONANCE(30)},
    {"C_5", 70, SCALED_RESONANCE(30)},
    {"F_4", 70, SCALED_RESONANCE(30)},
};

STATIC_VARIABLE const MusicData_t MUSIC_DATA_PRESS[] = {
    {"G_5", 55, SCALED_RESONANCE(20)}};

STATIC_VARIABLE const MusicData_t MUSIC_DATA_NO_KEY[] = {
    {"DS5", 110, SCALED_RESONANCE(10)},
    {"BRK", 100, SCALED_RESONANCE(0)},
    {"DS5", 110, SCALED_RESONANCE(10)}};

STATIC_VARIABLE const MusicData_t MUSIC_DATA_ERROR[] = {
    {"DS6", 70, SCALED_RESONANCE(30)},
    {"A_5", 70, SCALED_RESONANCE(30)},
    {"BRK", 200, SCALED_RESONANCE(0)},
    {"DS6", 70, SCALED_RESONANCE(30)},
    {"A_5", 70, SCALED_RESONANCE(30)}};

STATIC_VARIABLE const MusicData_t MUSIC_DATA_CONNECTED[] = {
    {"F_4", 70, SCALED_RESONANCE(30)},
    {"C_5", 90, SCALED_RESONANCE(30)}};

STATIC_VARIABLE const MusicData_t MUSIC_DATA_DISCONNECTED[] = {
    {"C_5", 70, SCALED_RESONANCE(30)},
    {"F_4", 90, SCALED_RESONANCE(30)}};

STATIC_VARIABLE const MusicData_t MUSIC_DATA_CONFIRM[] = {
    {"F_4", 70, SCALED_RESONANCE(30)},
    {"F_5", 70, SCALED_RESONANCE(70)}};

STATIC_VARIABLE const MusicData_t MUSIC_DATA_CONNECTING[] = {
    {"G_4", 80, SCALED_RESONANCE(30)},
    {"A_4", 80, SCALED_RESONANCE(30)},
    {"B_4", 80, SCALED_RESONANCE(30)},
    {"BRK", 500, SCALED_RESONANCE(0)},
    {"G_4", 80, SCALED_RESONANCE(30)},
    {"A_4", 80, SCALED_RESONANCE(30)},
    {"B_4", 80, SCALED_RESONANCE(30)},
    {"BRK", 1300, SCALED_RESONANCE(0)},
};

STATIC_VARIABLE const MusicData_t MUSIC_DATA_SHORT_MUTE[] = {
    {"BRK", 40, SCALED_RESONANCE(0)}};

STATIC_VARIABLE const MusicData_t MUSIC_DATA_SYSTEM_CRITICAL[] = {
    {"C_6", 400, SCALED_RESONANCE(0)},
    {"BRK", 200, SCALED_RESONANCE(0)},
    {"C_6", 400, SCALED_RESONANCE(0)},
    {"BRK", 200, SCALED_RESONANCE(0)},
    {"C_6", 400, SCALED_RESONANCE(0)},
    {"BRK", 200, SCALED_RESONANCE(0)},
    {"C_6", 400, SCALED_RESONANCE(0)},
    {"BRK", 400, SCALED_RESONANCE(0)}};

#define DEFAULT_RINGTONE_NAME "Simple tone 01"

STATIC_VARIABLE const MusicData_t MUSIC_DATA_DEFAULT_RINGTONE[] = {
    {"BRK", 40, SCALED_RESONANCE(0)},
    {"B_5", 75, SCALED_RESONANCE(30)},
    {"CS6", 75, SCALED_RESONANCE(30)},
    {"DS6", 75, SCALED_RESONANCE(30)},
    {"E_6", 75, SCALED_RESONANCE(30)},
    {"FS6", 75, SCALED_RESONANCE(30)},
    {"B_6", 75, SCALED_RESONANCE(30)},
    {"B_5", 75, SCALED_RESONANCE(30)},
    {"CS6", 75, SCALED_RESONANCE(30)},
    {"DS6", 75, SCALED_RESONANCE(30)},
    {"E_6", 75, SCALED_RESONANCE(30)},
    {"FS6", 75, SCALED_RESONANCE(30)},
    {"B_6", 75, SCALED_RESONANCE(30)},
    {"BRK", 420, SCALED_RESONANCE(0)},
    {"E_6", 75, SCALED_RESONANCE(30)},
    {"FS6", 75, SCALED_RESONANCE(30)},
    {"GS6", 75, SCALED_RESONANCE(30)},
    {"A_6", 75, SCALED_RESONANCE(30)},
    {"B_6", 75, SCALED_RESONANCE(30)},
    {"E_7", 75, SCALED_RESONANCE(30)},
    {"E_6", 75, SCALED_RESONANCE(30)},
    {"FS6", 75, SCALED_RESONANCE(30)},
    {"GS6", 75, SCALED_RESONANCE(30)},
    {"A_6", 75, SCALED_RESONANCE(30)},
    {"B_6", 75, SCALED_RESONANCE(30)},
    {"E_7", 75, SCALED_RESONANCE(30)},
    {"BRK", 1280, SCALED_RESONANCE(0)},
    {"B_5", 75, SCALED_RESONANCE(30)},
    {"CS6", 75, SCALED_RESONANCE(30)},
    {"DS6", 75, SCALED_RESONANCE(30)},
    {"E_6", 75, SCALED_RESONANCE(30)},
    {"FS6", 75, SCALED_RESONANCE(30)},
    {"B_6", 75, SCALED_RESONANCE(30)},
    {"B_5", 75, SCALED_RESONANCE(30)},
    {"CS6", 75, SCALED_RESONANCE(30)},
    {"DS6", 75, SCALED_RESONANCE(30)},
    {"E_6", 75, SCALED_RESONANCE(30)},
    {"FS6", 75, SCALED_RESONANCE(30)},
    {"B_6", 75, SCALED_RESONANCE(30)},
    {"BRK", 420, SCALED_RESONANCE(0)},
    {"E_6", 75, SCALED_RESONANCE(30)},
    {"FS6", 75, SCALED_RESONANCE(30)},
    {"GS6", 75, SCALED_RESONANCE(30)},
    {"A_6", 75, SCALED_RESONANCE(30)},
    {"B_6", 75, SCALED_RESONANCE(30)},
    {"E_7", 75, SCALED_RESONANCE(30)},
    {"E_6", 75, SCALED_RESONANCE(30)},
    {"FS6", 75, SCALED_RESONANCE(30)},
    {"GS6", 75, SCALED_RESONANCE(30)},
    {"A_6", 75, SCALED_RESONANCE(30)},
    {"B_6", 75, SCALED_RESONANCE(30)},
    {"E_7", 75, SCALED_RESONANCE(30)},
    {"BRK", 2150, SCALED_RESONANCE(0)}
};

STATIC_VARIABLE PlayerContents_t SOUND_ITEMS[SOUND_ITEM_MAX] = {
    {(const uint8_t *)MUSIC_DATA_BOOTING, sizeof(MUSIC_DATA_BOOTING), 0, 0},
    {(const uint8_t *)MUSIC_DATA_RESET_BOOTING, sizeof(MUSIC_DATA_RESET_BOOTING), 0, 0},
    {(const uint8_t *)MUSIC_DATA_OFF, sizeof(MUSIC_DATA_OFF), 0, 0},
    {(const uint8_t *)MUSIC_DATA_PRESS, sizeof(MUSIC_DATA_PRESS), 0, 0},
    {(const uint8_t *)MUSIC_DATA_NO_KEY, sizeof(MUSIC_DATA_NO_KEY), 0, 0},
    {(const uint8_t *)MUSIC_DATA_ERROR, sizeof(MUSIC_DATA_ERROR), 0, 0},
    {(const uint8_t *)MUSIC_DATA_CONNECTED, sizeof(MUSIC_DATA_CONNECTED), 0, 0},
    {(const uint8_t *)MUSIC_DATA_DISCONNECTED, sizeof(MUSIC_DATA_DISCONNECTED), 0, 0},
    {(const uint8_t *)MUSIC_DATA_CONFIRM, sizeof(MUSIC_DATA_CONFIRM), 0, 0},
    {(const uint8_t *)MUSIC_DATA_CONNECTING, sizeof(MUSIC_DATA_CONNECTING), CONVERT_SEC_TO_MS(T_REQUEST_ADVERTISE_TIMEOUT_SEC), 0},
    {(const uint8_t *)MUSIC_DATA_SYSTEM_CRITICAL, sizeof(MUSIC_DATA_SYSTEM_CRITICAL), 0, 0},
};

STATIC_VARIABLE const uint16_t NOTE_FREQ_CONVERT_TABLE[OCTAVE_SIZE][SOUND_NOTE_MAX] = {
    {440, 466, 494, 262, 277, 294, 311, 330, 349, 370, 392, 415},             // Octave 4, A ~ G#
    {880, 932, 988, 523, 554, 587, 622, 659, 698, 740, 784, 831},             // Octave 5, A ~ G#
    {1760, 1865, 1976, 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661}, // Octave 6, A ~ G#
    {3520, 3729, 3951, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322}  // Octave 7, A ~ G#
};

STATIC_VARIABLE bool sSoundInitialized;
STATIC_VARIABLE uint8_t sRingtoneVolume; /* 0x00 for mute, 0x01 for normal, 0x02 for loud */
STATIC_VARIABLE PortTimerHandle_t sPlayerTimer;
STATIC_VARIABLE PlayerContents_t sPlayerContents;
STATIC_VARIABLE SoundType_t sLatestSoundType;
STATIC_VARIABLE char *sCustomRingtoneName;
STATIC_VARIABLE uint8_t *sCustomRingtoneData;
STATIC_VARIABLE uint16_t sCustomRingtoneDataLength;
STATIC_VARIABLE bool sCustomRingtoneLoadRequired;

STATIC_FUNCTION bool IsSoundPlaying(void)
{
    return gSoundrPlaying;
}


STATIC_FUNCTION void setSoundPlaying(bool playing)
{
    gSoundrPlaying = playing;
}

STATIC_FUNCTION void soundResonanceTimeoutCallback(PortTimerHandle_t timer)
{
    PortBuzzerHwCtrlMute();
}

STATIC_FUNCTION void soundPlaytimeTimeoutCallback(PortTimerHandle_t timer)
{
    PortBuzzerHwCtrlStop();

    if (PortQueueMessagesWaiting(soundCtrlQueue))
    {
        soundCtrlWork();
    }
    else
    {
        PortBuzzerClose();
        setSoundPlaying(false);
    }
}

STATIC_FUNCTION void soundCtrlWork(void)
{
    SoundControl_t control;

    if (PortQueueReceive(soundCtrlQueue, &control, 0) != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to receive soundCtrlQueue queue");
        PortBuzzerHwCtrlMute();
        setSoundPlaying(false);
        return;
    }

    if (control.resonance > 0)
    {
        if (PortTimerChangePeriod(soundPlayTimer, CONV_MS_TO_TICKS(control.playTime + control.resonance), 0) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed soundPlayTimer");
            return;
        }

        if (PortTimerChangePeriod(soundResTimer, CONV_MS_TO_TICKS(control.playTime), 0) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed soundResTimer");
            return;
        }
    }
    else
    {
        if (PortTimerChangePeriod(soundPlayTimer, CONV_MS_TO_TICKS(control.playTime), 0) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed soundPlayTimer");
            return;
        }
    }

    PortBuzzerHwCtrlStart(control.frequency);
    setSoundPlaying(true);
}


STATIC_FUNCTION TagError_t soundCtrlInit(void)
{
    soundCtrlQueue = PortQueueCreate(SOUND_CONTROL_QUEUE_LENGTH, sizeof(SoundControl_t));
    if (!soundCtrlQueue)
    {
        TAG_LOG_E("Failed queue");
        return TAG_ERROR_INVALID_RESOURCE;
    }

    soundPlayTimer = PortTimerCreate("SoundPlayer", 1, false, NULL, soundPlaytimeTimeoutCallback);
    if (soundPlayTimer == NULL)
    {
        TAG_LOG_E("Failed SoundPlayer");
        PortQueueDelete(soundCtrlQueue);
        soundCtrlQueue = NULL;
        return TAG_ERROR_INVALID_RESOURCE;
    }

    soundResTimer = PortTimerCreate("SoundRes", 1, false, NULL, soundResonanceTimeoutCallback);
    if (soundResTimer == NULL)
    {
        TAG_LOG_E("Failed SoundRes");
        PortQueueDelete(soundCtrlQueue);
        soundCtrlQueue = NULL;
        PortTimerDelete(soundPlayTimer, 0);
        soundPlayTimer = NULL;
        return TAG_ERROR_INVALID_RESOURCE;
    }

    PortBuzzerHwCtrlInit();

    return TAG_ERROR_NONE;
}


STATIC_FUNCTION void soundCtrlStop(void)
{
    CHECK_RESULT_NOT_EQ(PortQueueReset(soundCtrlQueue), TAG_ERROR_NONE, "Failed reset queue");

    setSoundPlaying(false);

    PortBuzzerHwCtrlStop();
    PortBuzzerClose();
}

STATIC_FUNCTION TagError_t soundCtrlWrite(uint32_t frequency, uint32_t playTime, uint32_t resonance)
{
    SoundControl_t control;

    control.frequency = frequency;
    control.playTime = playTime;
    control.resonance = resonance;

    if (PortQueueSend(soundCtrlQueue, &control, 0) != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed send queue");
        return TAG_ERROR_INVALID_RESOURCE;
    }

    if (!IsSoundPlaying())
    {
        soundCtrlWork();
    }
    return TAG_ERROR_NONE;
}

STATIC_FUNCTION bool isCustomRingtoneExist(void)
{
    return (sCustomRingtoneName != NULL) ? true : false;
}

TagError_t TagSoundLoadCustomRingtone(void)
{
    TagNVData_t nvRingtoneName;
    TagNVData_t nvRingtoneData;
    TagNVData_t nvRingtoneSize;
    TagError_t error = TAG_ERROR_NONE;

    memset(&nvRingtoneName, '\0', sizeof(TagNVData_t));
    memset(&nvRingtoneData, '\0', sizeof(TagNVData_t));
    memset(&nvRingtoneSize, '\0', sizeof(TagNVData_t));

    // Load Ringtone Name
    nvRingtoneName.data.ringtoneName = TagMalloc(TAG_NV_RINGTONE_NAME_MAX_SZ);
    if (nvRingtoneName.data.ringtoneName == NULL)
    {
        TAG_LOG_E("Failed alloc memory (%d)", __LINE__);
        error = TAG_ERROR_MEM_ALLOC;
        goto exit;
    }
    memset(nvRingtoneName.data.ringtoneName, '\0', TAG_NV_RINGTONE_NAME_MAX_SZ);
    error = TagNVLoad(TAG_NV_RINGTONE_NAME, &nvRingtoneName);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_D("TAG_NV_RINGTONE_NAME");
        goto exit;
    }

    if (sCustomRingtoneName)
    {
        TagFree(sCustomRingtoneName);
        sCustomRingtoneName = NULL;
    }
    sCustomRingtoneName = TagMalloc(nvRingtoneName.dataLength + 1);
    if (sCustomRingtoneName == NULL)
    {
        TAG_LOG_E("Failed allo mem (%d)", __LINE__);
        error = TAG_ERROR_MEM_ALLOC;
        goto exit;
    }
    strncpy(sCustomRingtoneName, nvRingtoneName.data.ringtoneName, nvRingtoneName.dataLength + 1);
    TagFree(nvRingtoneName.data.ringtoneName);
    nvRingtoneName.data.ringtoneName = NULL;

    // Load Ringtone Data Size
    error = TagNVLoad(TAG_NV_RINGTONE_DATA_SIZE, &nvRingtoneSize);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_D("TAG_NV_RINGTONE_DATA_SIZE");
        goto exit;
    }
    sCustomRingtoneDataLength = nvRingtoneSize.data.ringtoneDataSz;

    // Load Ringtone Data
    nvRingtoneData.data.ringtoneData = TagMalloc(sCustomRingtoneDataLength);
    if (nvRingtoneData.data.ringtoneData == NULL)
    {
        TAG_LOG_E("Failed alloc mem %u (%d) ", sCustomRingtoneDataLength, __LINE__);
        error = TAG_ERROR_MEM_ALLOC;
        goto exit;
    }
    memset(nvRingtoneData.data.ringtoneData, '\0', sCustomRingtoneDataLength);
    error = TagNVLoad(TAG_NV_RINGTONE_DATA, &nvRingtoneData);
    if (error != TAG_ERROR_NONE)
    {
        TAG_LOG_D("TAG_NV_RINGTONE_DATA");
        goto exit;
    }

    if (sCustomRingtoneData)
    {
        TagFree(sCustomRingtoneData);
        sCustomRingtoneData = NULL;
    }
    sCustomRingtoneData = nvRingtoneData.data.ringtoneData;

    return TAG_ERROR_NONE;

exit:
    if (nvRingtoneName.data.ringtoneName)
    {
        TagFree(nvRingtoneName.data.ringtoneName);
    }

    if (sCustomRingtoneName)
    {
        TagFree(sCustomRingtoneName);
        sCustomRingtoneName = NULL;
    }

    sCustomRingtoneDataLength = 0;

    if (nvRingtoneData.data.ringtoneData)
    {
        TagFree(nvRingtoneData.data.ringtoneData);
        if (sCustomRingtoneData &&
            sCustomRingtoneData != nvRingtoneData.data.ringtoneData)
        {
            TagFree(sCustomRingtoneData);
        }
        sCustomRingtoneData = NULL;
    }

    if (error == TAG_ERROR_MEM_ALLOC)
    {
        sCustomRingtoneLoadRequired = true;
    }
    else
    {
        sCustomRingtoneLoadRequired = false;
    }
    return error;
}

void TagSoundClearCustomRingtone(void)
{
    if (sCustomRingtoneName)
    {
        TagFree(sCustomRingtoneName);
        sCustomRingtoneName = NULL;
    }
    if (sCustomRingtoneData)
    {
        TagFree(sCustomRingtoneData);
        sCustomRingtoneData = NULL;
    }
    sCustomRingtoneDataLength = 0;
}

STATIC_FUNCTION void setPlayerContents(const uint8_t *data, size_t dataLen, uint32_t timeoutMs, uint32_t playStartTimeMs)
{
    sPlayerContents.data = data;
    sPlayerContents.dataLen = dataLen;
    sPlayerContents.timeoutMs = timeoutMs;
    sPlayerContents.playStartTimeMs = playStartTimeMs;
}

void TagSetLatestSoundType(SoundType_t type)
{
    sLatestSoundType = type;
}

STATIC_FUNCTION SoundType_t getLatestSoundType(void)
{
    return sLatestSoundType;
}

STATIC_FUNCTION void setSoundInitialized(void)
{
    sSoundInitialized = true;
}

STATIC_FUNCTION bool getSoundInitialized(void)
{
    return sSoundInitialized;
}

STATIC_FUNCTION uint16_t decodeFreq(unsigned char note, unsigned char sharp, unsigned char octave)
{
    int noteIndex;
    unsigned char upperNote;

    if (sharp == 'R' || sharp == 'r')
    {
        return MUTE_FREQ;
    }
    else if (sharp != '_' && sharp != 'S' && sharp != 's')
    {
        TAG_LOG_E("Invalid %c%c%c", note, sharp, octave);
        return MUTE_FREQ;
    }

    if (octave < '4' || octave > '7')
    {
        TAG_LOG_E("Not supported %c%c%c", note, sharp, octave);
        return MUTE_FREQ;
    }

    if (note >= 'a')
    {
        upperNote = 'A' + (note - 'a');
    }
    else
    {
        upperNote = note;
    }

    switch (upperNote)
    {
    case 'A':
        noteIndex = SOUND_NOTE_A;
        break;
    case 'B':
        noteIndex = SOUND_NOTE_B;
        break;
    case 'C':
        noteIndex = SOUND_NOTE_C;
        break;
    case 'D':
        noteIndex = SOUND_NOTE_D;
        break;
    case 'E':
        noteIndex = SOUND_NOTE_E;
        break;
    case 'F':
        noteIndex = SOUND_NOTE_F;
        break;
    case 'G':
        noteIndex = SOUND_NOTE_G;
        break;
    default:
        TAG_LOG_E("Unknown %c%c%c[0x%x]", note, sharp, octave, upperNote);
        return MUTE_FREQ;
    }
    if (sharp == 'S' || sharp == 's')
    {
        if (upperNote != 'B' && upperNote != 'E')
        {
            noteIndex++;
        }
        else
        {
            TAG_LOG_E("Not existing %c#", note);
            return MUTE_FREQ;
        }
    }

    return NOTE_FREQ_CONVERT_TABLE[octave - '0' - OCTAVE_OFFSET][noteIndex];
}

STATIC_FUNCTION uint32_t tagSoundPlayData(const uint8_t *data, size_t dataLength)
{
    unsigned int i;
    size_t playLength;
    uint32_t totalPlayTimeMs = 0;

    playLength = (dataLength / SCORE_SIZE) * SCORE_SIZE;

    for (i = 0; i < playLength; i += SCORE_SIZE)
    {
        uint32_t freq = decodeFreq(data[i], data[i + 1], data[i + 2]);
        uint32_t length = (uint32_t)(data[i + 3] | (data[i + 4] << 8));
        uint32_t resonance = data[i + 5] * RESONANCE_SCALE;

        totalPlayTimeMs += length;
        totalPlayTimeMs += resonance;

        if (soundCtrlWrite(freq, length, resonance) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to play tone. let's stop");
            soundCtrlStop();
            return SOUND_NOT_PLAYED;
        }
    }

    return totalPlayTimeMs;
}

#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
STATIC_FUNCTION TagError_t sendSoundEventToBle(SoundBleEvent_t type)
{
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    TagControlServiceData *data;
    uint8_t charIndex;
    TagContext *context = gTagContext;
    EndUserDevice *endUserDevice;
    bool isNotification = false;

    if (context == NULL)
    {
        return TAG_ERROR_SND_NOT_INITIALIZED;
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
        TAG_LOG_E("Failed alloc mem (%d)", __LINE__);
        return TAG_ERROR_MEM_ALLOC;
    }

    switch (type)
    {
    case SOUND_INDICATION_RINGTONE_OFF:
        charIndex = CTRL_RINGTONE;
        data->aValue[0] = RINGTONE_VALUE_OFF;
        break;
    case SOUND_INDICATION_RINGTONE_SIREN:
        charIndex = CTRL_RINGTONE;
        data->aValue[0] = RINGTONE_VALUE_SIREN;
        break;
    case SOUND_NOTIFICATION_RINGTONE_OFF:
        charIndex = CTRL_RINGTONE_NON_OWNER;
        data->aValue[0] = RINGTONE_VALUE_OFF;
        isNotification = true;
        break;
    case SOUND_NOTIFICATION_RINGTONE_SIREN:
        charIndex = CTRL_RINGTONE_NON_OWNER;
        data->aValue[0] = RINGTONE_VALUE_SIREN;
        isNotification = true;
        break;
    case SOUND_NOTIFICATION_RINGTONE_VOLUME_MUTE:
        charIndex = CTRL_RINGTONE_VOLUME;
        data->aValue[0] = (uint8_t)SOUND_VOLUME_MUTE;
        isNotification = true;
        break;
    case SOUND_NOTIFICATION_RINGTONE_VOLUME_NORMAL:
        charIndex = CTRL_RINGTONE_VOLUME;
        data->aValue[0] = (uint8_t)SOUND_VOLUME_NORMAL;
        isNotification = true;
        break;
    case SOUND_NOTIFICATION_RINGTONE_VOLUME_LOUD:
        charIndex = CTRL_RINGTONE_VOLUME;
        data->aValue[0] = (uint8_t)SOUND_VOLUME_LOUD;
        isNotification = true;
        break;
    default:
        TAG_LOG_E("Invalid type %d", type);
        tagError = TAG_ERROR_INVALID_ARG;
        goto exit;
    }

    while (endUserDevice)
    {
        if (isNotification)
        {
            bleError = TagControlSendNotification(endUserDevice, charIndex, data, ENCRYPTION_REQUIRED);
        }
        else if (endUserDevice->deviceType == END_USER_DEVICE_OWNER)
        {
            bleError = TagControlSendIndication(endUserDevice, charIndex, data, ENCRYPTION_REQUIRED);
        }

        if (bleError != TAG_BLE_ERROR_ATT_NO_ERROR)
        {
            TAG_LOG_E("Failed to send %s to EndUserDevice-%u (%d)",
                      isNotification ? "notification" : "indication", endUserDevice->deviceId, bleError);
            tagError |= TAG_ERROR_BLE_EVENT_NOTIFY;
        }

        endUserDevice = endUserDevice->next;
    }

exit:
    FreeTagControlServiceData(data);

    return tagError;
}

STATIC_FUNCTION void soundTrySendRingtoneOffEvent(void)
{
    TagError_t tagError = TAG_ERROR_NONE;

    if (getLatestSoundType() == SOUND_TYPE_RINGTONE_FOR_OWNER)
    {
        tagError = sendSoundEventToBle(SOUND_INDICATION_RINGTONE_OFF);
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to send indication. error %d (%d)", tagError, __LINE__);
        }
    }
    else if (getLatestSoundType() == SOUND_TYPE_RINGTONE_FOR_NON_OWNER)
    {
        tagError = sendSoundEventToBle(SOUND_NOTIFICATION_RINGTONE_OFF);
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to send notification. error %d (%d)", tagError, __LINE__);
        }
    }
}
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */

STATIC_FUNCTION void soundPlayerTagPostWork(TagTaskWorkParam param)
{
    PlayerContents_t *contents = (PlayerContents_t *)PortTimerGetTimerId((PortTimerHandle_t)param);

    if (contents == NULL)
    {
        return;
    }

    if (contents->timeoutMs > 0)
    {
        // Timeout play scenario
        uint32_t timeElapsedMs;

        timeElapsedMs = PortTimeGetBootTimeMs() - contents->playStartTimeMs;
        if (timeElapsedMs >= contents->timeoutMs)
        {
            TAG_LOG_D("Timeout %u ms of %u ms (%d)", timeElapsedMs, contents->timeoutMs, __LINE__);
            TagSoundPlayStop();
            return;
        }
        else
        {
            uint32_t playTimeMs;
            uint32_t remainingTimeMs;

            // Workaround: If sending data to soundCtrlQueue is delayed due to some reason,
            // the buzzer is closed. So the sound does NOT play. This is to open the buzzer again
            // to avoid it if the buzzer is closed.
            if (!PortBuzzerGetOpenstate())
            {
                PortBuzzerOpen();
                CHECK_RESULT_NOT_EQ(PortBuzzerHwCtrlSetVolume(sRingtoneVolume), TAG_ERROR_NONE, "Failed to set volume");
            }

            TAG_LOG_D("Keep playing. %u ms of %u ms (%d)", timeElapsedMs, contents->timeoutMs, __LINE__);
            playTimeMs = tagSoundPlayData(contents->data, contents->dataLen);
            if (playTimeMs == 0)
            {
                TAG_LOG_E("Timeout Play Failed [%u, %u]", timeElapsedMs, contents->timeoutMs);
                TagSoundPlayStop();
                return;
            }

            remainingTimeMs = contents->timeoutMs - timeElapsedMs;

            if (remainingTimeMs > playTimeMs)
            {
                if (PortTimerChangePeriod(sPlayerTimer, CONV_MS_TO_TICKS(playTimeMs), 0) != TAG_ERROR_NONE)
                {
                    TAG_LOG_E("Failed to change timer %u ms (%d)", playTimeMs, __LINE__);
                    TagSoundPlayStop();
                }
            }
            else
            {
                if (PortTimerChangePeriod(sPlayerTimer, CONV_MS_TO_TICKS(remainingTimeMs), 0) != TAG_ERROR_NONE)
                {
                    TAG_LOG_E("Failed to change timer %u ms (%d)", remainingTimeMs, __LINE__);
                    TagSoundPlayStop();
                }
            }
        }
    }
    else
    {
        // One-shot play scenario
        if (IsSoundPlaying())
        {
            TAG_LOG_D("snd(%d): check again.", __LINE__);
            if (PortTimerChangePeriod(sPlayerTimer, CONV_MS_TO_TICKS(T_PLAYING_CHECK_MS), 0) != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Failed to change timer (%d)", __LINE__);
                TagSoundPlayStop();
            }
            return;
        }

        TagSoundPlayStop();
    }
}

STATIC_FUNCTION void soundPlayerTimerCallback(PortTimerHandle_t timer)
{
    if (TagPutPostWork(soundPlayerTagPostWork, timer) != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to register work - sound timer callback");
    }
}

STATIC_FUNCTION uint32_t soundPlayMusicWithTimeout(const uint8_t *data, size_t dataLen, unsigned int timeoutMs)
{
    uint32_t playTimeMs = SOUND_NOT_PLAYED;

    setPlayerContents(data, dataLen, timeoutMs, PortTimeGetBootTimeMs());

    TAG_LOG_D("Timeout %u ms (%d)", timeoutMs, __LINE__);

    playTimeMs = tagSoundPlayData(data, dataLen);
    if (playTimeMs == SOUND_NOT_PLAYED)
    {
        TAG_LOG_E("Failed play (%d)", __LINE__);
        return SOUND_NOT_PLAYED;
    }

    if (playTimeMs > timeoutMs)
    {
        if (PortTimerChangePeriod(sPlayerTimer, CONV_MS_TO_TICKS(timeoutMs), 0) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed timer %u ms (%d)", timeoutMs, __LINE__);
            TagSoundPlayStop();
            playTimeMs = SOUND_NOT_PLAYED;
        }
    }
    else
    {
        if (PortTimerChangePeriod(sPlayerTimer, CONV_MS_TO_TICKS(playTimeMs), 0) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed timer %u ms (%d)", playTimeMs, __LINE__);
            TagSoundPlayStop();
            playTimeMs = SOUND_NOT_PLAYED;
        }
    }

    return playTimeMs;
}

STATIC_FUNCTION uint32_t soundPlayMusic(const uint8_t *data, size_t dataLen)
{
    uint32_t playTimeMs = SOUND_NOT_PLAYED;

    setPlayerContents(data, dataLen, DO_NOT_USE_TIMEOUT, DO_NOT_USE_TIMEOUT);

    playTimeMs = tagSoundPlayData(data, dataLen);
    if (playTimeMs == SOUND_NOT_PLAYED)
    {
        TAG_LOG_E("Failed play sound (%d)", __LINE__);
        return SOUND_NOT_PLAYED;
    }

    if (PortTimerChangePeriod(sPlayerTimer, CONV_MS_TO_TICKS(playTimeMs + T_HW_CONTROL_MARGIN_MS), 0) != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed timer %u ms (%d)", playTimeMs, __LINE__);
        TagSoundPlayStop();
        playTimeMs = SOUND_NOT_PLAYED;
    }

    return playTimeMs;
}

bool TagSoundIsRunningRingtoneChangeAndPrepare(void)
{
    if (IsSoundPlaying())
    {
        TAG_LOG_D("Ringtone update during play");

        PortTimerStop(sPlayerTimer, PORT_MAX_DELAY);
        soundCtrlStop();
        return true;
    }
    return false;
}

void TagSoundFinishRunningRingtoneChange(void)
{
    PortBuzzerOpen();

    CHECK_RESULT_NOT_EQ(PortBuzzerHwCtrlSetVolume(sRingtoneVolume), TAG_ERROR_NONE, "Failed to set volume");

    tagSoundPlayData((const uint8_t *)MUSIC_DATA_SHORT_MUTE, sizeof(MUSIC_DATA_SHORT_MUTE));
    tagSoundPlayData((const uint8_t *)MUSIC_DATA_CONFIRM, sizeof(MUSIC_DATA_CONFIRM));
    tagSoundPlayData((const uint8_t *)MUSIC_DATA_SHORT_MUTE, sizeof(MUSIC_DATA_SHORT_MUTE));

    soundPlayMusicWithTimeout(sCustomRingtoneData, sCustomRingtoneDataLength, CONVERT_SEC_TO_MS(RINGTONE_DEFAULT_TIMEOUT));
}

uint32_t TagSoundPlayRingtone(unsigned int timeoutSec, SoundBleEvent_t eventType)
{
    const uint8_t *data;
    size_t dataLen = 0;
    uint32_t ret = SOUND_NOT_PLAYED;
    SoundType_t ringtoneType;
    TagError_t tagError = TAG_ERROR_NONE;

    if (eventType == SOUND_INDICATION_RINGTONE_SIREN)
    {
        ringtoneType = SOUND_TYPE_RINGTONE_FOR_OWNER;
    }
    else if (eventType == SOUND_NOTIFICATION_RINGTONE_SIREN)
    {
        ringtoneType = SOUND_TYPE_RINGTONE_FOR_NON_OWNER;
    }
    else
    {
        TAG_LOG_E("Invalid parameter. type %u (%d)", eventType, __LINE__);
        return SOUND_NOT_PLAYED;
    }

    if (timeoutSec == 0)
    {
        TAG_LOG_E("0s timeout (%d)", __LINE__);
        return SOUND_NOT_PLAYED;
    }

    if (!getSoundInitialized())
    {
        TAG_LOG_E("not initialized (%d)", __LINE__);
        return SOUND_NOT_PLAYED;
    }

#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
    UwbPowerMode_t curUwbPwrMode;
    PortUwbGetPower(&curUwbPwrMode);
    if (curUwbPwrMode == UWB_POWER_MODE_ACTIVATE)
    {
        TAG_LOG_I("Stop UWB before ring");
        PortUwbSetPowerOffDueToRing();
    }
#endif

    TagSetLatestSoundType(ringtoneType);

    if (isCustomRingtoneExist())
    {
        data = (const uint8_t *)sCustomRingtoneData;
        dataLen = (size_t)sCustomRingtoneDataLength;
    }
    else if (sCustomRingtoneLoadRequired)
    {
        TAG_LOG_I("Retry");
        tagError = TagSoundLoadCustomRingtone();
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed. use default %d", tagError);
            data = (const uint8_t *)MUSIC_DATA_DEFAULT_RINGTONE;
            dataLen = sizeof(MUSIC_DATA_DEFAULT_RINGTONE);
        }
        else
        {
            data = (const uint8_t *)sCustomRingtoneData;
            dataLen = (size_t)sCustomRingtoneDataLength;
        }
    }
    else
    {
        data = (const uint8_t *)MUSIC_DATA_DEFAULT_RINGTONE;
        dataLen = sizeof(MUSIC_DATA_DEFAULT_RINGTONE);
    }

    PortBuzzerOpen();

    CHECK_RESULT_NOT_EQ(PortBuzzerHwCtrlSetVolume(sRingtoneVolume), TAG_ERROR_NONE, "Failed to set volume");

    ret = soundPlayMusicWithTimeout(data, dataLen, CONVERT_SEC_TO_MS(timeoutSec));

#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
    tagError = sendSoundEventToBle(eventType);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed event %u. error %d (%d)", eventType, tagError, __LINE__);
    }
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */

    return ret;
}

bool TagSoundIsRingtonePlaying(void)
{
    if ((getLatestSoundType() == SOUND_TYPE_RINGTONE_FOR_OWNER || getLatestSoundType() == SOUND_TYPE_RINGTONE_FOR_NON_OWNER) && IsSoundPlaying())
    {
        return true;
    }
    return false;
}

uint32_t TagSoundPlayItem(SoundItem_t item)
{
    uint32_t result = 0;

    if (item < SOUND_ITEM_BOOTING || item >= SOUND_ITEM_MAX)
    {
        TAG_LOG_E("Invalid item %u (%d)", item, __LINE__);
        return SOUND_NOT_PLAYED;
    }
    else
    {
        TAG_LOG_D("Item %u (%d)", item, __LINE__);
    }

    if (!getSoundInitialized())
    {
        TAG_LOG_E("not initialized (%d)", __LINE__);
        return SOUND_NOT_PLAYED;
    }

    if (IsSoundPlaying())
    {
        TAG_LOG_D("Stop playing %u (%d)", getLatestSoundType(), __LINE__);
        TagSoundPlayStop();
    }

    PortBuzzerOpen();

    if (item == SOUND_ITEM_CONNECTING)
    {
        TagSetLatestSoundType(SOUND_TYPE_FEEDBACK_CONNECTING);
    }
    else
    {
        TagSetLatestSoundType(SOUND_TYPE_FEEDBACK_OTHERS);
    }

    /* item sound should be played with normal level */
    CHECK_RESULT_NOT_EQ(PortBuzzerHwCtrlSetVolume(SOUND_VOLUME_NORMAL), TAG_ERROR_NONE, "Failed to set volume as normal");

    if (SOUND_ITEMS[item].timeoutMs > 0)
    {
        result = soundPlayMusicWithTimeout(SOUND_ITEMS[item].data, SOUND_ITEMS[item].dataLen,
                                           SOUND_ITEMS[item].timeoutMs);
    }
    else
    {
        result = soundPlayMusic(SOUND_ITEMS[item].data, SOUND_ITEMS[item].dataLen);
    }

    return result;
}

bool TagSoundIsConnectingPlaying(void)
{
    if ((getLatestSoundType() == SOUND_TYPE_FEEDBACK_CONNECTING) && IsSoundPlaying())
    {
        return true;
    }
    return false;
}

void TagSoundPlayStop(void)
{
    if (IsSoundPlaying())
    {
        soundCtrlStop();
        PortTimerStop(sPlayerTimer, PORT_MAX_DELAY);
    }

#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
    soundTrySendRingtoneOffEvent();
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
}

TagError_t TagSoundSetRingtoneVolume(SoundVolume_t volume)
{
    TagError_t tagError = TAG_ERROR_NONE;
    TagNVData_t nvData;
    SoundBleEvent_t indicationType;

    if (!getSoundInitialized())
    {
        TAG_LOG_E("not initialized (%d)", __LINE__);
        return TAG_ERROR_SND_NOT_INITIALIZED;
    }

    switch (volume)
    {
    case SOUND_VOLUME_MUTE:
        indicationType = SOUND_NOTIFICATION_RINGTONE_VOLUME_MUTE;
        TAG_LOG_I("MUTE");
        break;
    case SOUND_VOLUME_NORMAL:
        indicationType = SOUND_NOTIFICATION_RINGTONE_VOLUME_NORMAL;
        TAG_LOG_I("NORMAL");
        break;
    case SOUND_VOLUME_LOUD:
        indicationType = SOUND_NOTIFICATION_RINGTONE_VOLUME_LOUD;
        TAG_LOG_I("LOUD");
        break;
    default:
        return TAG_ERROR_INVALID_ARG;
    }

    nvData.data.soundVolume = (uint8_t)volume;
    nvData.dataLength = 1;
    tagError = TagNVStore(TAG_NV_SOUND_VOLUME, &nvData);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed NV. error %d (%d)", tagError, __LINE__);
        return tagError;
    }

    if (TagSoundIsRingtonePlaying() == true)
    {
        CHECK_RESULT_NOT_EQ(PortBuzzerHwCtrlSetVolume(volume), TAG_ERROR_NONE, "Failed to set volume");
    }
    sRingtoneVolume = (uint8_t)volume;

#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
    tagError = sendSoundEventToBle(indicationType);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed event. type %u (%d)", indicationType, __LINE__);
    }
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */

    return tagError;
}

TagError_t TagSoundGetRingtoneVolume(SoundVolume_t *volume)
{
    if (volume == NULL)
    {
        return TAG_ERROR_INVALID_ARG;
    }

    if (!getSoundInitialized())
    {
        TAG_LOG_E("SoundPlayer isn't initialized (%d)", __LINE__);
        return TAG_ERROR_SND_NOT_INITIALIZED;
    }

    *volume = sRingtoneVolume;

    return TAG_ERROR_NONE;
}

TagError_t TagSoundGetRingtoneName(char *outBuffer, size_t outBufferLen)
{
    if (outBuffer == NULL || outBufferLen <= 4)
    {
        return TAG_ERROR_INVALID_ARG;
    }

    if (sCustomRingtoneName)
    {
        strncpy(outBuffer, sCustomRingtoneName, outBufferLen);
    }
    else
    {
        strncpy(outBuffer, DEFAULT_RINGTONE_NAME, outBufferLen);
    }

    return TAG_ERROR_NONE;
}

TagError_t TagSoundPlayerInit(void)
{
    TagNVData_t nvData;
    TagError_t tagError = TAG_ERROR_NONE;

    if (soundCtrlInit() != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed hardware(%d)", __LINE__);
        return TAG_ERROR_SND_NOT_INITIALIZED;
    }

    tagError = TagNVLoad(TAG_NV_SOUND_VOLUME, &nvData);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed error %d. Use loud (%d)", tagError, __LINE__);
        sRingtoneVolume = SOUND_VOLUME_LOUD;
    }
    else
    {
        sRingtoneVolume = nvData.data.soundVolume;
    }

    tagError = TagSoundLoadCustomRingtone();
    if (tagError && tagError != TAG_ERROR_NV_NOT_EXIST)
    {
        TAG_LOG_E("Failed ringtone (%d)", tagError);
    }

    sPlayerTimer = PortTimerCreate("TagSoundPlayer", 1, false, &sPlayerContents, soundPlayerTimerCallback);
    if (sPlayerTimer == NULL)
    {
        TAG_LOG_E("Failed timer (%d)", __LINE__);
        return TAG_ERROR_SND_INVALID_TIMER;
    }

    setSoundInitialized();

    return TAG_ERROR_NONE;
}

TagError_t TagSoundPlayerReset(void)
{
    TagNVData_t nvData;
    TagError_t tagError = TAG_ERROR_NONE;

    CHECK_RESULT_NOT_EQ(PortQueueReset(soundCtrlQueue), TAG_ERROR_NONE, "Failed reset queue");

    PortTimerChangePeriod(soundPlayTimer, 1, 0);
    /* This function is called in CONNECTED state, So stop timer after change period */
    PortTimerStop(soundPlayTimer, 0);

    PortTimerChangePeriod(soundResTimer, 1, 0);
    /* This function is called in CONNECTED state, So stop timer after change period */
    PortTimerStop(soundResTimer, 0);

    tagError = TagNVLoad(TAG_NV_SOUND_VOLUME, &nvData);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed error %d. Use loud (%d)", tagError, __LINE__);
        sRingtoneVolume = SOUND_VOLUME_LOUD;
    }
    else
    {
        sRingtoneVolume = nvData.data.soundVolume;
    }

    tagError = TagSoundLoadCustomRingtone();
    if (tagError && tagError != TAG_ERROR_NV_NOT_EXIST)
    {
        TAG_LOG_E("Failed ringtone (%d)", tagError);
    }

    PortTimerChangePeriod(sPlayerTimer, 1, 0);
    /* This function is called in CONNECTED state, So stop timer after change period */
    PortTimerStop(sPlayerTimer, 0);

    setSoundInitialized();

    return TAG_ERROR_NONE;
}
