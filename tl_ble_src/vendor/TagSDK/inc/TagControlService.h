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

#ifndef TAGSDK_INC_TAGCONTROLSERVICE_H_
#define TAGSDK_INC_TAGCONTROLSERVICE_H_

#include "TagCore.h"
#include "TagSecurity.h"
#include "PortOs.h"

#define TAG_CONTROL_SEQUENCE_LENGTH 4 /* sequence number 4 bytes required */

#define FACTORY_RESET_COMMAND (0x01)
#define FACTORY_RESET_DELAY_TIME_FOR_WRITE_RESPONSE (2000) /* Guarantee time(ms) for write response sent to end user device*/

#define ENCRYPTION_REQUIRED (1)
#define ENCRYPTION_NOT_REQUIRED (0)

#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
#define LED_BLINKING_VALUE_OFF (0x00)
#define LED_BLINKING_VALUE_ON (0x01)
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

#define RINGTONE_VALUE_OFF (0x00)
#define RINGTONE_VALUE_SIREN (0x01)
#define RINGTONE_DEFAULT_TIMEOUT (60)

#define BUTTON_EVENT_PUSHED (0x01)
#define BUTTON_EVENT_HELD (0x02)
#define BUTTON_EVENT_PUSHED_2X (0x03)

#define TIME_INFORMATION_SET_UTC_TIME_OPCODE (0x00)
#define TIME_INFORMATION_SET_PREMATURE_OFFLINE_TIMEOUT_OPCODE (0x01)
#define TIME_INFORMATION_SET_OFFLINE_TIMEOUT_OPCODE (0x02)
#define TIME_INFORMATION_NOTIFY_TIME_OPCODE (0x03)
#define TIME_INFORMATION_SET_OVERMATURE_PRIVACYID_INTERVAL_OPCODE (0x04)
#define TIME_INFORMATION_TIME_INFORMATION_LENGTH (16)

#define RINGTONE_UPDATE_INFORMATION_OPCODE (0x00)
#define RINGTONE_UPDATE_DATA_OPCODE (0x01)
#define RINGTONE_NAME_MAX_SIZE (60)
#define RINGTONE_UPDATE_INVALID_DEVICEID (0xFF)
#define RINGTONE_INFO_OPCODE_OFFSET (0)
#define RINGTONE_INFO_TOTAL_SIZE_OFFSET (1)
#define RINGTONE_INFO_TOTAL_CRC16_OFFSET (3)
#define RINGTONE_INFO_NAME_LENGTH_OFFSET (5)
#define RINGTONE_INFO_NAME_OFFSET (6)
#define RINGTONE_DATA_OPCODE_OFFSET (0)
#define RINGTONE_DATA_OFFSET_OFFSET (1)
#define RINGTONE_DATA_SEG_DATA_LENGTH_OFFSET (3)
#define RINGTONE_DATA_SEG_DATA_OFFSET (5)

#define BLE_CONNECTION_SETTING_SET_MAX_ALLOWED_CONNECTIONS_OPCODE (0x00)
#define BLE_CONNECTION_SETTING_SET_PREFERRED_PARAM_OPCODE (0x01)
#define BLE_CONNECTION_SETTING_SET_PARAM_IDLE_OPCODE (0x02)

#define BLE_PREFERRED_PARAM_MIN_CONNECTION_INTERVAL (12)    /* 1 unit = 1.25 ms */
#define BLE_PREFERRED_PARAM_MAX_CONNECTION_INTERVAL (1600)  /* 1 unit = 1.25 ms */
#define BLE_PREFERRED_PARAM_MAX_SLAVE_LATENCY (9)
#define BLE_PREFERRED_PARAM_MIN_TIMEOUT_MULTIPLIER (200)    /* 1 unit = 10 ms */
#define BLE_PREFERRED_PARAM_MAX_TIMEOUT_MULTIPLIER (600)    /* 1 unit = 10 ms */

#define BLE_PARING_CONTROL_REQUEST_OPCODE (0x00)

#define BLE_PID_SETTING_UPDATE_OPCODE (0x00)
#define BLE_PID_SETTING_INDICATION_OPCODE (0x01)

#define BLE_PID_SETTING_UPDATE_OFFSET (0)
#define BLE_PID_SETTING_UPDATE_PID_POOL_SIZE_OFFSET (1)
#define BLE_PID_SETTING_UPDATE_IV_LENGTH_OFFSET (3)
#define BLE_PID_SETTING_UPDATE_IV_OFFSET (4)
#define BLE_PID_SETTING_UPDATE_LENGTH (20)

#define BLE_PID_INDICATION_PID_INFORMATION_OPCODE (0x11)
#define BLE_PID_INDICATION_PID_INFORMATION_LENGTH (20)

#define PSM_SET_MODE_OPCODE (0x00)
#define PSM_SET_NORMAL (0x00)
#define PSM_SET_POWER_SAVE (0x01)

#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
#define UWB_POWER_ACTIVATE (0x01)
#define UWB_POWER_DEACTIVATE (0x00)
#endif

#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
#define DEBUG_START_LOG_TRANSFER_OPCODE 0x00
#define DEBUG_TRANSFER_LOG_INFORMATION_OPCODE 0x00
#define DEBUG_TRANSFER_LOG_INFORMATION_LENGTH 5
#define DEBUG_TRANSFER_LOG_DATA_OPCODE 0x01
#define DEBUG_TRANSFER_LOG_DATA_HEAD_LENGTH 7

#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
#define DEBUG_TRANSFER_CONNECTION_PARAM_INTERVAL_MIN (15)   /* 12 unit (15 ms) */
#define DEBUG_TRANSFER_CONNECTION_PARAM_INTERVAL_MAX (15)   /* 12 unit (15 ms) */
#define DEBUG_TRANSFER_CONNECTION_PARAM_LATENCY (0)         /* 0 latency */
#define DEBUG_TRANSFER_CONNECTION_PARAM_TIMEOUT (5000)      /* 5000 ms */
#endif

#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */

/* decrypted ble data packet */
typedef struct
{
    uint16_t cSeqLength; /* default is 4 bytes */
    uint8_t *aSeqValue;  /* aSeqValue includes sequence data + real value */

    uint16_t cValueLength; /* real value length */
    uint8_t *aValue;       /* NOTE : aValue should point aSeqValue + cSeqLength address */
} TagControlServiceData;

#define INDICATION_QUEUE_LENGTH (20)
#define INDICATION_EXPIRE_TIMEOUT_MS (3000) /* 3s */
#define IS_QUEUE_EMPTY(x) (PortQueueSpacesAvailable(x) == INDICATION_QUEUE_LENGTH ? true : false)
#define NUMBER_OF_QUEUED_INDICATION(x) (unsigned int)(INDICATION_QUEUE_LENGTH - PortQueueSpacesAvailable(x))

typedef struct
{
#if defined(__APP_BLE_INDICATION_SINGLE_QUEUE__)
    TagBleDeviceId deviceId;
#endif
    uint8_t charIndex;
    TagControlServiceData *data;
    uint8_t encryptFlag;
    uint8_t ignoreFlag;
} IndicationData_t;

typedef enum
{
    TAG_SEND_INDICATION,
    TAG_SEND_NOTIFICATION,
} TagBleSendingMode_t;

typedef struct
{
    TagBleDeviceId deviceId;
    uint16_t totalRingtoneSize;
    uint16_t totalRingtoneCRC16;
    uint8_t ringtoneNameLength;
    char *ringtoneName;
    uint8_t *ringtoneData;
    uint16_t recvDataLength;
} RingtoneTransferInfo_t;

typedef struct
{
    TagBleDeviceId deviceId;
    uint8_t charIndex;
    TagControlServiceData *data;
} TagControlServiceGattData;

TagControlServiceData *AllocateTagControlServiceData(size_t valueLength);
TagControlServiceData *DuplicateTagControlServiceData(TagControlServiceData *inputData);
void FreeTagControlServiceData(TagControlServiceData *data);

TagBleError_t TagControlServiceWrittenCallback(EndUserDevice *endUserDevice, uint8_t charIndex, TagControlServiceData *data);
void TagControlServiceWrittenPostCallback(TagTaskWorkParam param);
TagBleError_t TagControlServiceReadCallback(EndUserDevice *endUserDevice, uint8_t charIndex, uint8_t **readData, size_t *readDataLength);

void TagControlFreeQueuedIndicationData(PortQueueHandle_t queue);
TagError_t TagControlGattDbWriteAndSendIndication(EndUserDevice *endUserDevice, uint8_t charIndex, TagControlServiceData *data, uint8_t encryptFlag);
TagError_t TagControlSendIndication(EndUserDevice *endUserDevice, uint8_t charIndex, TagControlServiceData *data, uint8_t encryptFlag);
TagError_t TagControlSendImmediateIndication(EndUserDevice *endUserDevice, uint8_t charIndex, TagControlServiceData *data, uint8_t encryptFlag);
TagError_t TagControlSendNotification(EndUserDevice *endUserDevice, uint8_t charIndex, TagControlServiceData *data, uint8_t encryptFlag);

#endif /* TAGSDK_INC_TAGCONTROLSERVICE_H_ */

