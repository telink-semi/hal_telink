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

#ifndef TAGSDK_INC_TAGBLECHARACTERISTIC_H_
#define TAGSDK_INC_TAGBLECHARACTERISTIC_H_

#include <stdbool.h>

#include "TagConfig.h"
#include "TagErrorType.h"

#include "PortBleDataType.h"

#define NONE_CALLBACK_FLAG 0
#define READ_CALLBACK_FLAG 0x01 << 0
#define WRITE_CALLBACK_FLAG 0x01 << 1
#define WRITE_WO_RSP_CALLBACK_FLAG 0x01 << 2

#define TAG_SDK_DESIRED_TX_POWER        (8)
#define TAG_ADV_TX_POWER_OOB            (-12)
#if defined(PORT_MAX_TX_POWER) && (PORT_MAX_TX_POWER < TAG_SDK_DESIRED_TX_POWER)
#define TAG_ADV_TX_POWER_CONNECTED      PORT_MAX_TX_POWER
#define TAG_ADV_TX_POWER_DISCONNECTED   PORT_MAX_TX_POWER
#else
#define TAG_ADV_TX_POWER_CONNECTED      TAG_SDK_DESIRED_TX_POWER
#define TAG_ADV_TX_POWER_DISCONNECTED   TAG_SDK_DESIRED_TX_POWER
#endif /* PORT_MAX_TX_POWER */

typedef enum
{
    PROP_NONE = 0,                      /* No Properties selected. */
    PROP_BROADCAST = 0x01 << 0,         /* Characteristic can be broadcast. */
    PROP_READ = 0x01 << 1,              /* Characteristic can be read. */
    PROP_WRITE_WO_RSP = 0x01 << 2,      /* Characteristic can be written without response. */
    PROP_WRITE = 0x01 << 3,             /* Characteristic can be written with response. */
    PROP_NOTIFY = 0x01 << 4,            /* Characteristic can be notified. */
    PROP_INDICATE = 0x01 << 5,          /* Characteristic can be indicated. */
    PROP_AUTH_SIGNED_WRITE = 0x01 << 6, /* Characteristic can be written with signed data. */
    PROP_EXTENDED = 0x01 << 7           /* Extended Characteristic properties. */
} TagBleGattProperties;

typedef enum
{
    TAG_CHAR_ENC_TYPE_NONE, /* Encryption is not required */
    TAG_CHAR_ENC_TYPE_CK,   /* Encryption is required with command key */
    TAG_CHAR_ENC_TYPE_NCK   /* Encryption is required with non owner command key */
} TagCharEncryptionType;

typedef enum
{
    AUTH_SERVICE,
    CONTROL_SERVICE,
    ONBOARDING_SERVICE
} ServiceType;

typedef enum
{
    TAG_CHAR_START,
    AUTH_CHAR_START = TAG_CHAR_START,
    AUTH_CIPHER = AUTH_CHAR_START,
    AUTH_NONCE,
    AUTH_ENCRYPTED_DATA,
    AUTH_CHAR_END
} AuthCharacteristic;

typedef enum
{
    CTRL_CHAR_START = AUTH_CHAR_END,
    CTRL_BATTERY = CTRL_CHAR_START,
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
    CTRL_RINGTONE,
    CTRL_RINGTONE_VOLUME,
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#if defined(TAG_ACCESSORY_OPTION_UPDATE_RINGTONE) && (TAG_ACCESSORY_OPTION_UPDATE_RINGTONE == 1)
    CTRL_RINGTONE_UPDATE,
#endif /* TAG_ACCESSORY_OPTION_UPDATE_RINGTONE */
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
    CTRL_DEBUG_TAG,
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
    CTRL_NFC_LOST_MESSAGE_URL,
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */
#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
    CTRL_BUTTON,
#endif // TAG_ACCESSORY_OPTION_BUTTON_ACTION
    CTRL_TIME_INFORMATION,
    CTRL_FACTORY_RESET,
    CTRL_E2E_ENCRYPTION,
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
    CTRL_UWB_POWER,
    CTRL_UWB_PARAM,
#endif
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
    CTRL_LED_BLINKING,
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
    CTRL_FIRMWARE_VERSION,
#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
    CTRL_FIRMWARE_TRANSFER,
#endif // TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE
    CTRL_BLE_CONNECTION_SETTING,
    CTRL_SPECIFICATION_VERSION,
#if defined(TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT) && (TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT == 1)
    CTRL_BLE_PAIRING_CONTROL,
#endif // TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT
#if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
    CTRL_POWER_SAVING_MODE,
#endif // TAG_ACCESSORY_OPTION_POWER_SAVING_MODE
    CTRL_BLE_PRIVACY_ID_SETTING,
    CTRL_CHAR_OWNER_END = CTRL_BLE_PRIVACY_ID_SETTING,
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
    CTRL_RINGTONE_NON_OWNER,
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
    CTRL_UWB_POWER_NON_OWNER,
    CTRL_UWB_PARAM_NON_OWNER,
#endif
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
    CTRL_LED_BLINKING_NON_OWNER,
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
    CTRL_CHAR_END
} ControlCharacteristic;

#define CTRL_CHAR_NON_OWNER_START (CTRL_CHAR_OWNER_END + 1)

typedef enum
{
    ONBD_CHAR_START = CTRL_CHAR_END,
    ONBD_DEVICE_FIRMWARE_VERSION = ONBD_CHAR_START,
    ONBD_BLE_SC_CAPABILITY,
    ONBD_HASHED_SERIAL_NUMBER,
    ONBD_CONFIRM_STATUS,
    ONBD_MNMN,
    ONBD_VID,
    ONBD_IDENTIFIER,
    ONBD_CONFIGURATION_VERSION,
    ONBD_SUPPORTED_CIPHER,
    ONBD_SELECTED_CIPHER,
    ONBD_SEED,
    ONBD_NUMBER_OF_PRIVACY_ID,
    ONBD_SETUP_COMPLETE,
    ONBD_SUPPORTED_CONFIRM_METHOD_LIST,
    ONBD_SELECTED_CONFIRM_METHOD,
    ONBD_CONFIRM_RESULT,
    ONBD_SERIAL_CONFIRM,
    ONBD_CLOUD_PUBLIC_KEY,
    ONBD_RANDOM_VALUE,
    ONBD_REGION,
    ONBD_MODEL_NAME,
    ONBD_LOGGING,
    ONBD_PRIVACY_ID_VECTOR,
    ONBD_CHAR_END,
    TAG_CHAR_END = ONBD_CHAR_END,
    TAG_CHAR_INVALID
} OnboardingCharacteristic;

extern const unsigned char uuid_auth_service[16];
extern const unsigned char uuid_onboarding_service[2];
extern const unsigned char uuid_tag_service[2];

typedef struct
{
    const unsigned char *pUuid;
    unsigned char properties;
    uint16_t maxValueLength;
    uint16_t initialValueLength;
    const uint8_t *aInitialValue;
    uint16_t handle;
    uint16_t handleV;
    uint16_t handleCccd;
    uint8_t callback_flag;
    TagCharEncryptionType encryptionType;
} TagBleCharacteristicStruct;

extern TagBleCharacteristicStruct gTagChar[TAG_CHAR_END];

bool TagCharIsEncrypted(uint8_t charIndex);
uint8_t TagCharGetIndexFromHandle(uint16_t handle);
uint16_t TagCharGetCccdNumAttr(ServiceType service_type);
TagError_t TagBleGattRegisterService(ServiceType service_type);
TagError_t TagBleGattRegisterCharacteristic(ServiceType service_type);
TagError_t TagBleGattUnregisterService(ServiceType service_type);
TagError_t TagBleGattUnregisterCharacteristic(ServiceType service_type);
TagError_t TagBleInitGATTDB(void);

#endif /* TAGSDK_INC_TAGBLECHARACTERISTIC_H_ */
