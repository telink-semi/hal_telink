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
#include "TagBleGattUUID.h"
#include "TagCore.h"
#include "TagFwUpdate.h"
#include "TagOnboardingService.h"
#include "TagSecurity.h"

#ifdef LOG_PREFIX
#undef LOG_PREFIX
#endif
#define LOG_PREFIX "CHAR"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

#define ENC_LEN TAG_SECURITY_CIPHER_ALIGN_LEN
#define SEQ_LEN(x) ((x) + 4)
#define VER_STR_LEN (11)
#define BLE_CONN_SET_LEN (8)

#define TAG_INIT_SIZE 1

uint8_t tag_init[TAG_INIT_SIZE] = {0};

TagBleCharacteristicStruct gTagChar[TAG_CHAR_END] =
    {
        /* AUTH */
        [AUTH_CIPHER] = {uuid_cipher, PROP_READ | PROP_WRITE, 24, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NONE},
        [AUTH_NONCE] = {uuid_nonce, PROP_WRITE | PROP_INDICATE, 16, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NONE},
        [AUTH_ENCRYPTED_DATA] = {uuid_encrypted_data, PROP_WRITE | PROP_INDICATE, 16, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NONE},

        /* TAG */
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
        [CTRL_RINGTONE] = {uuid_ringtone, PROP_READ | PROP_WRITE | PROP_INDICATE, ENC_LEN(SEQ_LEN(1)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [CTRL_RINGTONE_VOLUME] = {uuid_ringtone_volume, PROP_READ | PROP_WRITE | PROP_NOTIFY, ENC_LEN(SEQ_LEN(1)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
        [CTRL_BUTTON] = {uuid_button, PROP_WRITE | PROP_INDICATE, ENC_LEN(SEQ_LEN(2)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#endif // TAG_ACCESSORY_OPTION_BUTTON_ACTION
        [CTRL_BATTERY] = {uuid_battery, PROP_READ | PROP_INDICATE, ENC_LEN(SEQ_LEN(1)), TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [CTRL_TIME_INFORMATION] = {uuid_time_information, PROP_WRITE | PROP_INDICATE, ENC_LEN(SEQ_LEN(20)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [CTRL_FACTORY_RESET] = {uuid_factory_reset, PROP_WRITE, ENC_LEN(SEQ_LEN(1)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [CTRL_E2E_ENCRYPTION] = {uuid_e2e_encryption, PROP_READ | PROP_WRITE, ENC_LEN(SEQ_LEN(1)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
        [CTRL_UWB_POWER] = {uuid_uwb_power, PROP_READ | PROP_WRITE, ENC_LEN(SEQ_LEN(1)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [CTRL_UWB_PARAM] = {uuid_uwb_param, PROP_WRITE | PROP_INDICATE, ENC_LEN(SEQ_LEN(36)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#endif
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
        [CTRL_LED_BLINKING] = {uuid_led_blinking, PROP_READ | PROP_WRITE | PROP_INDICATE, ENC_LEN(SEQ_LEN(1)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
#if defined(TAG_ACCESSORY_OPTION_UPDATE_RINGTONE) && (TAG_ACCESSORY_OPTION_UPDATE_RINGTONE == 1)
        [CTRL_RINGTONE_UPDATE] = {uuid_ringtone_update, PROP_READ | PROP_WRITE, TAG_MAX_MTU - 3, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#endif /* TAG_ACCESSORY_OPTION_UPDATE_RINGTONE */
        [CTRL_FIRMWARE_VERSION] = {uuid_firmware_version, PROP_READ, ENC_LEN(SEQ_LEN(VER_STR_LEN)), TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
        [CTRL_FIRMWARE_TRANSFER] = {uuid_firmware_transfer, PROP_READ | PROP_WRITE_WO_RSP | PROP_INDICATE, TAG_MAX_MTU - 3, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_WO_RSP_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#endif // TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE
        [CTRL_BLE_CONNECTION_SETTING] = {uuid_ble_connection_setting, PROP_READ | PROP_WRITE, ENC_LEN(SEQ_LEN(BLE_CONN_SET_LEN)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [CTRL_SPECIFICATION_VERSION] = {uuid_specification_version, PROP_READ, SEQ_LEN(VER_STR_LEN), TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NONE},
#if defined(TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT) && (TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT == 1)
        [CTRL_BLE_PAIRING_CONTROL] = {uuid_ble_pairing_control, PROP_WRITE, ENC_LEN(SEQ_LEN(1)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#endif // TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT
        [CTRL_BLE_PRIVACY_ID_SETTING] = {uuid_ble_privacy_id_setting, PROP_WRITE | PROP_INDICATE, ENC_LEN(SEQ_LEN(20)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
        [CTRL_POWER_SAVING_MODE] = {uuid_power_saving_mode, PROP_READ | PROP_WRITE | PROP_INDICATE, ENC_LEN(SEQ_LEN(1)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#endif // TAG_ACCESSORY_OPTION_POWER_SAVING_MODE
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
#ifdef TAG_CONFIG_USE_NOTIFICATION_FOR_DEBUG
        [CTRL_DEBUG_TAG] = {uuid_debug, PROP_WRITE | PROP_NOTIFY, TAG_MAX_MTU - 3, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#else
        [CTRL_DEBUG_TAG] = {uuid_debug, PROP_WRITE | PROP_INDICATE, TAG_MAX_MTU - 3, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#endif
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
        [CTRL_NFC_LOST_MESSAGE_URL] = {uuid_nfc_lost_message_url, PROP_READ | PROP_WRITE, TAG_MAX_MTU - 3, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */

        /* TAG - overmature state */
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
        [CTRL_RINGTONE_NON_OWNER] = {uuid_ringtone_non_owner, PROP_READ | PROP_WRITE | PROP_NOTIFY, ENC_LEN(SEQ_LEN(1)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NCK},
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
        [CTRL_UWB_POWER_NON_OWNER] = {uuid_uwb_power_non_owner, PROP_READ | PROP_WRITE, ENC_LEN(SEQ_LEN(1)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NCK},
        [CTRL_UWB_PARAM_NON_OWNER] = {uuid_uwb_param_non_owner, PROP_WRITE | PROP_INDICATE, ENC_LEN(SEQ_LEN(36)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NCK},
#endif
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
        [CTRL_LED_BLINKING_NON_OWNER] = {uuid_led_blinking_non_owner, PROP_READ | PROP_WRITE | PROP_NOTIFY, ENC_LEN(SEQ_LEN(1)), TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG | READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NCK},
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

        /* ONBOARDING */
        [ONBD_DEVICE_FIRMWARE_VERSION] = {uuid_device_firmware_version, PROP_READ, VER_STR_LEN + 1, TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NONE},
        [ONBD_BLE_SC_CAPABILITY] = {uuid_ble_sc_capability, PROP_READ, 1, TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NONE},
        [ONBD_HASHED_SERIAL_NUMBER] = {uuid_hashed_serial_number, PROP_READ, 32, TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NONE},
        [ONBD_CONFIRM_STATUS] = {uuid_confirm_status, PROP_READ, 16, TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [ONBD_MNMN] = {uuid_mnmn, PROP_READ, MNMN_SIZE, TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [ONBD_VID] = {uuid_vid, PROP_READ, 48, TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [ONBD_IDENTIFIER] = {uuid_identifier, PROP_READ, 32, TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [ONBD_CONFIGURATION_VERSION] = {uuid_configuration_version, PROP_READ, 16, TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [ONBD_SUPPORTED_CIPHER] = {uuid_supported_cipher, PROP_READ, 32, TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NONE},
        [ONBD_SELECTED_CIPHER] = {uuid_selected_cipher, PROP_WRITE, 32, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NONE},
        [ONBD_SEED] = {uuid_seed, PROP_WRITE, 32, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [ONBD_NUMBER_OF_PRIVACY_ID] = {uuid_number_of_privacy_id, PROP_WRITE, 32, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [ONBD_SETUP_COMPLETE] = {uuid_setup_complete, PROP_WRITE, 16, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [ONBD_SUPPORTED_CONFIRM_METHOD_LIST] = {uuid_supported_confirm_method_list, PROP_READ, 32, TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [ONBD_SELECTED_CONFIRM_METHOD] = {uuid_selected_confirm_method, PROP_WRITE, 32, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [ONBD_CONFIRM_RESULT] = {uuid_confirm_result, PROP_INDICATE, 16, TAG_INIT_SIZE, tag_init, 0, 0, 0, NONE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [ONBD_SERIAL_CONFIRM] = {uuid_serial_confirm, PROP_WRITE, 32, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [ONBD_CLOUD_PUBLIC_KEY] = {uuid_cloud_public_key, PROP_WRITE, 32, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NONE},
        [ONBD_RANDOM_VALUE] = {uuid_random_value, PROP_WRITE, 64, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NONE},
        [ONBD_REGION] = {uuid_region, PROP_WRITE, 16, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
        [ONBD_MODEL_NAME] = {uuid_model_name, PROP_READ, MODEL_NAME_SIZE, TAG_INIT_SIZE, tag_init, 0, 0, 0, READ_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NONE},
        [ONBD_LOGGING] = {uuid_logging, PROP_WRITE | PROP_INDICATE, TAG_MAX_MTU - 3, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_NONE},
        [ONBD_PRIVACY_ID_VECTOR] = {uuid_privacy_id_vector, PROP_WRITE, 32, TAG_INIT_SIZE, tag_init, 0, 0, 0, WRITE_CALLBACK_FLAG, TAG_CHAR_ENC_TYPE_CK},
};

bool TagCharIsEncrypted(uint8_t charIndex)
{
    bool ret = false;

    if (charIndex >= TAG_CHAR_END)
    {
        return false;
    }

    if (gTagChar[charIndex].encryptionType == TAG_CHAR_ENC_TYPE_CK || gTagChar[charIndex].encryptionType == TAG_CHAR_ENC_TYPE_NCK)
    {
        ret = true;
    }

    return ret;
}

uint8_t TagCharGetIndexFromHandle(uint16_t handle)
{
    uint8_t i = 0;

    if (handle == 0)
    {
        return TAG_CHAR_INVALID;
    }

    for (i = TAG_CHAR_START; i < TAG_CHAR_END; i++)
    {
        if (gTagChar[i].handleV == handle || gTagChar[i].handleCccd == handle)
        {
            break;
        }
    }

    if (i == TAG_CHAR_END)
    {
        TAG_LOG_E("Failed to find characteristic index (handle=%u)", handle);
        i = TAG_CHAR_INVALID;
    }

    return i;
}

uint16_t TagCharGetCccdNumAttr(ServiceType service_type)
{
    int i = 0, initNum = 0, maxNum = 0;
    uint16_t cccdNumAttr = 0;

    switch (service_type)
    {
    case AUTH_SERVICE:
        initNum = AUTH_CHAR_START;
        maxNum = AUTH_CHAR_END;
        break;

    case CONTROL_SERVICE:
        initNum = CTRL_CHAR_START;
        maxNum = CTRL_CHAR_END;
        break;

    case ONBOARDING_SERVICE:
        initNum = ONBD_CHAR_START;
        maxNum = ONBD_CHAR_END;
        break;

    default:
        break;
    }

    for (i = initNum; i < maxNum; i++)
    {
        if (gTagChar[i].properties & PROP_INDICATE || gTagChar[i].properties & PROP_NOTIFY)
        {
            cccdNumAttr++;
        }
    }
    return cccdNumAttr;
}

#ifdef TAG_CONFIG_LOG_LEVEL_DEBUG
static void tagBleDumpGattDb(void)
{
    int i;

    TAG_LOG_D("%s, %s, %s", "index", "handleV", "handleCccd");
    for (i = TAG_CHAR_START; i < TAG_CHAR_END; i++)
    {
        TAG_LOG_D("%5u, %7u, %10u", i, gTagChar[i].handleV, gTagChar[i].handleCccd);
    }
}
#endif

TagError_t TagBleGattRegisterCharacteristic(ServiceType service_type)
{
    TagError_t ret = TAG_ERROR_NONE;

#if SUPPORT_GATT_UNREGISTER
    ret = PortBleAddGattDbCharacteristic(service_type);
#else
    if (gTagContext->state != TAG_STATE_OVERMATURE_OFFLINE)
    {
        ret = PortBleAddGattDbCharacteristic(service_type);
    }
#endif
    return ret;
}

TagError_t TagBleGattRegisterService(ServiceType service_type)
{
    TagError_t ret = TAG_ERROR_NONE;

    ret = PortBleAddGattDbService(service_type);

    return ret;
}

TagError_t TagBleGattUnregisterCharacteristic(ServiceType service_type)
{
    TagError_t ret = 0;

#if SUPPORT_GATT_UNREGISTER
    ret = PortBleRemoveGattDbCharacteristic(service_type);
#else
    TAG_LOG_D("Not support gatt unregister");
#endif
    return ret;
}

TagError_t TagBleGattUnregisterService(ServiceType service_type)
{
    TagError_t ret = TAG_ERROR_NONE;

#if SUPPORT_GATT_UNREGISTER
    ret = PortBleRemoveGattDbService(service_type);
#else
    TAG_LOG_D("Not support gatt unregister");
#endif
    return ret;
}

TagError_t TagBleInitGATTDB(void)
{
    TagError_t ret = TAG_ERROR_NONE;

    ret = TagBleGattRegisterService(AUTH_SERVICE);
    if (ret == TAG_ERROR_NONE)
    {
        ret = TagBleGattRegisterCharacteristic(AUTH_SERVICE);
    }
    if (ret != TAG_ERROR_NONE)
    {
        return ret;
    }

    ret = TagBleGattRegisterService(CONTROL_SERVICE);
    if (ret == TAG_ERROR_NONE)
    {
        ret = TagBleGattRegisterCharacteristic(CONTROL_SERVICE);
    }
    if (ret != TAG_ERROR_NONE)
    {
        return ret;
    }

    if (gTagContext->state == TAG_STATE_OUT_OF_BOX)
    {
        ret = TagBleGattRegisterService(ONBOARDING_SERVICE);
        if (ret == TAG_ERROR_NONE)
        {
            ret = TagBleGattRegisterCharacteristic(ONBOARDING_SERVICE);
        }
#ifdef TAG_CONFIG_LOG_LEVEL_DEBUG
        tagBleDumpGattDb();
#endif
    }

    return ret;
}
