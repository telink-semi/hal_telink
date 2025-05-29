/********************************************************************************************************
 * @file    app_att.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "TagConfig.h"
#include "app.h"
#include "app_att.h"
#include "app_ui.h"
//#include "smart_uuid.h"


#define TAG_SECURITY_CIPHER_ALIGN_LEN(x)    ((x) + (16 - ((x) % 16)))
#define ENC_LEN_FUNC TAG_SECURITY_CIPHER_ALIGN_LEN
#define SEQ_LEN_FUNC(x) ((x) + 4)
#define VER_STR_LEN_CONST (11)
#define BLE_CONN_SET_LEN_CONST (8)
#define TAG_MAX_MTU     (243)
#define MNMN_SIZE    32
#define MODEL_NAME_SIZE    32

typedef struct {
    uint16_t value_handle; // char value_handle
    uint8_t *data;
    uint8_t len;
} ble_gatts_write_t;

typedef struct {
    uint16_t value_handle; // char value handle
} ble_gatts_read_t;

/*
 * GATTS event callback parameters union
 * */
typedef struct {
    uint16_t conn_handle;
    union {
        ble_gatts_write_t write;
        ble_gatts_read_t read;
    };
} ble_gatts_evt_param_t;

typedef enum{
    NOTIFY_CMD_CODE = 1,
    INDICATION_CMD_CODE = 2,
} operation_cmd_enum_t;

////////////////////////////////////////// peripheral-role ATT service concerned ///////////////////////////////////////////////
typedef struct
{
    /** Minimum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
    u16 intervalMin;
    /** Maximum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
    u16 intervalMax;
    /** Number of LL latency connection events (0x0000 - 0x03e8) */
    u16 latency;
    /** Connection Timeout (0x000A - 0x0C80 * 10 ms) */
    u16 timeout;
} gap_periConnectParams_t;

static const u16 clientCharacterCfgUUID = GATT_UUID_CLIENT_CHAR_CFG;

static const u16 userdesc_UUID = GATT_UUID_CHAR_USER_DESC;

static const u16 serviceChangeUUID = GATT_UUID_SERVICE_CHANGE;

static const u16 my_primaryServiceUUID = GATT_UUID_PRIMARY_SERVICE;

static const u16 my_characterUUID = GATT_UUID_CHARACTER;

static const u16 my_devNameUUID = GATT_UUID_DEVICE_NAME;

static const u16 my_gapServiceUUID = SERVICE_UUID_GENERIC_ACCESS;

static const u16 my_appearanceUUID = GATT_UUID_APPEARANCE;

static const u16 my_periConnParamUUID = GATT_UUID_PERI_CONN_PARAM;

static const u16 my_appearance = GAP_APPEARANCE_UNKNOWN;

static const u16 my_gattServiceUUID = SERVICE_UUID_GENERIC_ATTRIBUTE;

static const gap_periConnectParams_t my_periConnParameters = {20, 40, 0, 1000};

_attribute_ble_data_retention_ static u16 serviceChangeVal[2] = {0};

_attribute_ble_data_retention_ static u8 serviceChangeCCC[2] = {0, 0};

static const u8 my_devName[] = {'p', 'e', 'r', 'i', 'p', 'h', 'r', '_', 'd', 'e', 'm', 'o'};

/* Authentication Service */
const unsigned char uuid_auth_service_t[16] = {0x7C, 0xA8, 0x9D, 0x48, 0x8A, 0x39, 0x19, 0x82, 0x73, 0x46, 0xA8, 0x6A, 0x73, 0x5E, 0xDD, 0xEE};

/* Authentication Service's Characteristics */
const unsigned char uuid_cipher_t[16] = {0x5D, 0xDF, 0xF5, 0xC2, 0x70, 0x0A, 0xD4, 0xAD, 0xFA, 0x4E, 0x8C, 0x15, 0xFD, 0x8B, 0xF9, 0x50};
const unsigned char uuid_nonce_t[16] = {0x7C, 0x3A, 0x23, 0x35, 0x57, 0x3D, 0x9D, 0x9B, 0x73, 0x47, 0x38, 0x5B, 0x1C, 0xE3, 0x2B, 0xA1};
const unsigned char uuid_encrypted_data_t[16] = {0x55, 0x89, 0x4E, 0x9D, 0xA3, 0x5C, 0xCE, 0x9E, 0x5E, 0x46, 0x52, 0xB9, 0xF6, 0x81, 0xBE, 0x4E};

/* Onboarding Service */
const unsigned char uuid_onboarding_service_t[2] = {0x59, 0xfd};

/* Onboarding Service's Characteristics */
const unsigned char uuid_device_firmware_version_t[16] = {0x30, 0xC0, 0xF1, 0xA3, 0x97, 0x7F, 0x97, 0x9F, 0x40, 0x42, 0xCB, 0x6C, 0x2A, 0x8D, 0xC4, 0x30};
const unsigned char uuid_ble_sc_capability_t[16] = {0xED, 0xCB, 0x36, 0x99, 0x89, 0x93, 0x29, 0x94, 0x0A, 0x4A, 0xFA, 0x1D, 0x89, 0x25, 0x3A, 0xBE};
const unsigned char uuid_hashed_serial_number_t[16] = {0x5D, 0x46, 0x56, 0x23, 0xC3, 0x04, 0x04, 0xB8, 0xF4, 0x4B, 0x42, 0xF4, 0xB1, 0x6D, 0xC1, 0x6A};
const unsigned char uuid_confirm_status_t[16] = {0x0D, 0x2F, 0xEE, 0x59, 0xCC, 0xFB, 0x12, 0xAC, 0xC1, 0x43, 0xB3, 0x17, 0x05, 0xF8, 0x99, 0xF2};
const unsigned char uuid_mnmn_t[16] = {0x06, 0xEE, 0x86, 0xDC, 0x36, 0xE9, 0x81, 0x9D, 0xEB, 0x43, 0x01, 0xD2, 0x18, 0x28, 0x05, 0x04};
const unsigned char uuid_vid_t[16] = {0xE6, 0x17, 0xB4, 0x41, 0x17, 0x81, 0x21, 0xB0, 0xD1, 0x49, 0x90, 0x58, 0xEC, 0x8B, 0xB0, 0x77};
const unsigned char uuid_identifier_t[16] = {0xCE, 0x85, 0x49, 0xA6, 0x32, 0x4F, 0x32, 0x9C, 0x29, 0x49, 0x6D, 0x1C, 0x38, 0x1E, 0xA1, 0x08};
const unsigned char uuid_configuration_version_t[16] = {0x27, 0xA0, 0xA8, 0xC8, 0x7C, 0x6F, 0x24, 0x84, 0x0C, 0x49, 0x1C, 0x24, 0x92, 0x12, 0x76, 0x12};
const unsigned char uuid_supported_cipher_t[16] = {0xB6, 0x22, 0x81, 0x65, 0x42, 0x00, 0xD5, 0x92, 0x41, 0x48, 0x7E, 0x25, 0x4C, 0x7A, 0x5F, 0x5B};
const unsigned char uuid_selected_cipher_t[16] = {0x92, 0x37, 0x32, 0x87, 0x6D, 0x79, 0xFA, 0x98, 0xF6, 0x4F, 0xB8, 0x87, 0x74, 0x11, 0xA3, 0x6E};
const unsigned char uuid_seed_t[16] = {0x0A, 0x48, 0x7C, 0xB5, 0xCE, 0xF3, 0x18, 0xBB, 0x44, 0x41, 0xE1, 0xBB, 0x83, 0xDD, 0x9D, 0xD1};
const unsigned char uuid_number_of_privacy_id_t[16] = {0x44, 0x6A, 0xBD, 0x75, 0x2A, 0xDC, 0xD7, 0xAF, 0x12, 0x4D, 0x40, 0x1F, 0x94, 0xC3, 0x34, 0x75};
const unsigned char uuid_setup_complete_t[16] = {0x29, 0x52, 0x09, 0x7C, 0x7F, 0x54, 0xAE, 0xA0, 0xDC, 0x48, 0xF6, 0x8A, 0xE6, 0xCC, 0xC8, 0xBC};
const unsigned char uuid_supported_confirm_method_list_t[16] = {0xDE, 0xC9, 0x4F, 0x97, 0x5D, 0x57, 0x56, 0xAE, 0x57, 0x4C, 0x4A, 0x03, 0x57, 0xD3, 0x3B, 0xB0};
const unsigned char uuid_selected_confirm_method_t[16] = {0xCA, 0xCA, 0x8C, 0x9F, 0x4D, 0x13, 0xAB, 0x81, 0x44, 0x46, 0x5E, 0xCF, 0xE1, 0x3F, 0x7A, 0xB5};
const unsigned char uuid_confirm_result_t[16] = {0x35, 0x7D, 0xDC, 0x6F, 0x78, 0x9A, 0x98, 0xBC, 0xB5, 0x42, 0x9E, 0x0D, 0xCB, 0xDF, 0xB0, 0x89};
const unsigned char uuid_serial_confirm_t[16] = {0xAE, 0xBF, 0x2B, 0xC8, 0x14, 0x80, 0xCB, 0x9F, 0x3A, 0x48, 0xC1, 0x3A, 0xF1, 0xF3, 0x1E, 0x66};
const unsigned char uuid_cloud_public_key_t[16] = {0xB2, 0x6B, 0xCF, 0xEE, 0x2F, 0x49, 0x18, 0xA1, 0xC6, 0x44, 0x21, 0x68, 0x29, 0x46, 0x75, 0xB5};
const unsigned char uuid_random_value_t[16] = {0x10, 0x62, 0xF3, 0x90, 0x27, 0x4C, 0xAD, 0x9D, 0xDA, 0x4B, 0x0B, 0x3D, 0x4E, 0xD1, 0xE8, 0xD0};
const unsigned char uuid_region_t[16] = {0x6D, 0xF4, 0x7E, 0x9C, 0x8C, 0xFC, 0xB8, 0xA4, 0xDE, 0x44, 0xB8, 0xDC, 0x51, 0xAA, 0xBF, 0xBE};
const unsigned char uuid_model_name_t[16] = {0x71, 0xDD, 0xBC, 0x73, 0x98, 0x27, 0x7C, 0xBA, 0x2C, 0x46, 0xBC, 0xF3, 0xE4, 0xFE, 0x52, 0x53};
const unsigned char uuid_logging_t[16] = {0x13, 0x64, 0xCE, 0x18, 0xEB, 0x7D, 0x1B, 0xB4, 0x4F, 0x4A, 0xAB, 0x69, 0x35, 0x20, 0xBC, 0x17};
const unsigned char uuid_privacy_id_vector_t[16] = {0x81, 0xD8, 0xEE, 0x48, 0x95, 0xB6, 0xB2, 0xB9, 0x86, 0x47, 0x43, 0x38, 0xBA, 0xE6, 0xD6, 0xAB};

/* Tag Service */
const unsigned char uuid_tag_service_t[2] = {0x5A, 0xfd};

/* Tag Service's Characteristics */
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
const unsigned char uuid_ringtone_t[16] =        {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x01, 0x00, 0xE3, 0xDE};
const unsigned char uuid_ringtone_volume_t[16] = {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x02, 0x00, 0xE3, 0xDE};
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
const unsigned char uuid_button_t[16] =          {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x03, 0x00, 0xE3, 0xDE};
#endif /* TAG_ACCESSORY_OPTION_BUTTON_ACTION */
const unsigned char uuid_battery_t[16] =         {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x04, 0x00, 0xE3, 0xDE};
const unsigned char uuid_time_information_t[16] = {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x05, 0x00, 0xE3, 0xDE};
const unsigned char uuid_factory_reset_t[16] =    {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x06, 0x00, 0xE3, 0xDE};
const unsigned char uuid_e2e_encryption_t[16] =   {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x07, 0x00, 0xE3, 0xDE};
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
const unsigned char uuid_uwb_power_t[16] =        {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x08, 0x00, 0xE3, 0xDE};
const unsigned char uuid_uwb_param_t[16] =        {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x09, 0x00, 0xE3, 0xDE};
#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_UPDATE_RINGTONE) && (TAG_ACCESSORY_OPTION_UPDATE_RINGTONE == 1)
const unsigned char uuid_ringtone_update_t[16] =        {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x0A, 0x00, 0xE3, 0xDE};
#endif /* TAG_ACCESSORY_OPTION_UPDATE_RINGTONE */
const unsigned char uuid_firmware_version_t[16] =       {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x0B, 0x00, 0xE3, 0xDE};
#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
const unsigned char uuid_firmware_transfer_t[16] =      {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x0C, 0x00, 0xE3, 0xDE};
#endif /* TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE */
const unsigned char uuid_ble_connection_setting_t[16] = {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x0D, 0x00, 0xE3, 0xDE};
const unsigned char uuid_specification_version_t[16] =  {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x0E, 0x00, 0xE3, 0xDE};
#if defined(TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT) && (TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT == 1)
const unsigned char uuid_ble_pairing_control_t[16] =    {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x0F, 0x00, 0xE3, 0xDE};
#endif /* TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT */

const unsigned char uuid_ble_privacy_id_setting_t[16] = {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x1A, 0x00, 0xE3, 0xDE};
#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
const unsigned char uuid_nfc_lost_message_url_t[16] =   {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x1B, 0x00, 0xE3, 0xDE};
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */
#if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
const unsigned char uuid_power_saving_mode_t[16] =      {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x1D, 0x00, 0xE3, 0xDE};
#endif /* TAG_ACCESSORY_OPTION_POWER_SAVING_MODE */
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
const unsigned char uuid_led_blinking_t[16] =           {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x1E, 0x00, 0xE3, 0xDE};
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
const unsigned char uuid_ringtone_non_owner_t[16] =     {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x20, 0x00, 0xE3, 0xDE};
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
const unsigned char uuid_uwb_power_non_owner_t[16] =        {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x21, 0x00, 0xE3, 0xDE};
const unsigned char uuid_uwb_param_non_owner_t[16] =        {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x22, 0x00, 0xE3, 0xDE};
#endif
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
const unsigned char uuid_led_blinking_non_owner_t[16] =     {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x24, 0x00, 0xE3, 0xDE};
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

const unsigned char uuid_debug_t[16] = {0x84, 0x41, 0x32, 0x16, 0xF2, 0x14, 0xAD, 0xB1, 0x96, 0x54, 0x2D, 0x18, 0x30, 0x00, 0xE3, 0xDE};


typedef struct __attribute__((packed))
{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u16 handle;
    u8  value;
} ble_rf_packet_att_write_t;


int PortBle_writeData(u16 connHandle, app_ble_rf_packet_att_write_t *p);
int PortBle_ReadData(u16 connHandle, app_rf_packet_att_readBlob_t *p);
int portble_writeccc(u16 connHandle, app_ble_rf_packet_att_write_t *p);

/////////////////////////////////// SmartTings Find Device - INIT_VALUE START ///////////////////////////////////////////

//////////////////////// AUTH_SERVICE //////////////////////////////////
u8 AuthCipherInitV[24] = {0};                                                                     // AUTH_CIPHER
u8 AuthNonceInitV[16] = {0};                                                                      // AUTH_NONCE
u8 AuthEncryptedDataInitV[16] = {0};                                                              // AUTH_ENCRYPTED_DATA

//////////////////////// CONTROL_SERVICE ///////////////////////////////
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
u8 CtrlRingtoneInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(1))] = {0};                                        // CTRL_RINGTONE
u8 CtrlRingtoneVolumeInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(1))] = {0};                                  // CTRL_RINGTONE_VOLUME
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
u8 CtrlButtonInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(2))] = {0};                                          // CTRL_BUTTON
#endif /* TAG_ACCESSORY_OPTION_BUTTON_ACTION */
u8 CtrlBatteryInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(1))] = {0};                                         // CTRL_BATTERY
u8 CtrlTimeInformationInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(20))] = {0};                                // CTRL_TIME_INFORMATION
u8 CtrlFactoryResetInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(1))] = {0};                                    // CTRL_FACTORY_RESET
u8 CtrlE2eEncryptionInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(1))] = {0};                                   // CTRL_E2E_ENCRYPTION
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
u8 CtrlUwbPowerInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(1))] = {0};                                         // CTRL_UWB_POWER
u8 CtrlUwbParamInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(36))] = {0};                                        // CTRL_UWB_PARAM
#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
u8 CtrlLedBlinkingInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(1))] = {0};                                      // CTRL_LED_BLINKING
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
#if defined(TAG_ACCESSORY_OPTION_UPDATE_RINGTONE) && (TAG_ACCESSORY_OPTION_UPDATE_RINGTONE == 1)
u8 CtrlRingtoneUpdateInitV[TAG_MAX_MTU - 3] = {0};                                                 // CTRL_RINGTONE_UPDATE
#endif /* TAG_ACCESSORY_OPTION_UPDATE_RINGTONE */
u8 CtrlFirmwareVersionInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(VER_STR_LEN_CONST))] = {0};                  // CTRL_FIRMWARE_VERSION
#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
u8 CtrlFirmwareTransferInitV[TAG_MAX_MTU - 3] = {0};                                               // CTRL_FIRMWARE_TRANSFER
#endif /* TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE */
u8 CtrlBleConnectionSettingInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(BLE_CONN_SET_LEN_CONST))] = {0};        // CTRL_BLE_CONNECTION_SETTING
u8 CtrlSpecificationVersionInitV[ENC_LEN_FUNC(VER_STR_LEN_CONST)] = {0};                           // CTRL_SPECIFICATION_VERSION
#if defined(TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT) && (TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT == 1)
u8 CtrlBlePairingControlInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(1))] = {0};                                // CTRL_BLE_PAIRING_CONTROL
#endif // TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT
u8 CtrlBlePrivacyIdSettingInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(20))] = {0};                             // CTRL_BLE_PRIVACY_ID_SETTING
#if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
u8 CtrlPowerSavingModeInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(1))] = {0};                                  // CTRL_POWER_SAVING_MODE
#endif // TAG_ACCESSORY_OPTION_POWER_SAVING_MODE
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
u8 CtrlDebugTagInitV[TAG_MAX_MTU - 3] = {0};                                                       // CTRL_DEBUG_TAG
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
u8 CtrlNfcLostMessageUrlInitV[TAG_MAX_MTU - 3] = {0};                                              // CTRL_NFC_LOST_MESSAGE_URL
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
u8 CtrlRingtoneNonOwnerInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(1))] = {0};                                 // CTRL_RINGTONE_NON_OWNER
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
u8 CtrlUwbPowerNonOwnerInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(1))] = {0};                                 // CTRL_UWB_POWER_NON_OWNER
u8 CtrlUwbParamNonOwnerInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(36))] = {0};                                // CTRL_UWB_PARAM_NON_OWNER
#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
u8 CtrlLedBlinkingNonOwnerInitV[ENC_LEN_FUNC(ENC_LEN_FUNC(1))] = {0};                              // CTRL_LED_BLINKING_NON_OWNER
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

//////////////////////// ONBOARDING_SERVICE ////////////////////////////
u8 OnbdDeviceFirmwareVersionInitV[VER_STR_LEN_CONST + 1] = {0};                                    // ONBD_DEVICE_FIRMWARE_VERSION
u8 OnbdBleScCapabilityInitV[1] = {0};                                                              // ONBD_BLE_SC_CAPABILITY
u8 OnbdHashedSerialNumberInitV[32] = {0};                                                          // ONBD_HASHED_SERIAL_NUMBER
u8 OnbdConfirmStatusInitV[16] = {0};                                                               // ONBD_CONFIRM_STATUS
u8 OnbdMNMNInitV[MNMN_SIZE] = {0};                                                                 // ONBD_MNMN
u8 OnbdVidInitV[48] = {0};                                                                         // ONBD_VID
u8 OnbdIdentifierInitV[32] = {0};                                                                  // ONBD_IDENTIFIER
u8 OnbdConfigurationVersionInitV[16] = {0};                                                        // ONBD_CONFIGURATION_VERSION
u8 OnbdSupportedCipherInitV[32] = {0};                                                             // ONBD_SUPPORTED_CIPHER
u8 OnbdSelectedCipherInitV[32] = {0};                                                              // ONBD_SELECTED_CIPHER
u8 OnbdSeedInitV[32] = {0};                                                                        // ONBD_SEED
u8 OnbdNumberOfPrivacyIdInitV[32] = {0};                                                           // ONBD_NUMBER_OF_PRIVACY_ID
u8 OnbdSetupCompleteInitV[16] = {0};                                                               // ONBD_SETUP_COMPLETE
u8 OnbdSupportedConfirmMethodListInitV[32] = {0};                                                  // ONBD_SUPPORTED_CONFIRM_METHOD_LIST
u8 OnbdSelectedConfirmMethodInitV[32] = {0};                                                       // ONBD_SELECTED_CONFIRM_METHOD
u8 OnbdConfirmResultInitV[16] = {0};                                                               // ONBD_CONFIRM_RESULT
u8 OnbdSerialConfirmInitV[32] = {0};                                                               // ONBD_SERIAL_CONFIRM
u8 OnbdCloudPublicKeyInitV[32] = {0};                                                              // ONBD_CLOUD_PUBLIC_KEY
u8 OnbdRandomValueInitV[64] = {0};                                                                 // ONBD_RANDOM_VALUE
u8 OnbdRegionInitV[16] = {0};                                                                      // ONBD_REGION
u8 OnbdModelInitV[MODEL_NAME_SIZE] = {0};                                                          // ONBD_MODEL_NAME
u8 OnbdLoggingInitV[TAG_MAX_MTU - 3] = {0};                                                        // ONBD_LOGGING
u8 OnbdPrivacyIdVectorInitV[32] = {0};                                                             // ONBD_PRIVACY_ID_VECTOR
/////////////////////////////////// SmartTings Find Device - INIT_VALUE END ///////////////////////////////////////////

/////////////////////////////////// SmartTings Find Device - CCC START ///////////////////////////////////////////

//////////////////////// AUTH_SERVICE //////////////////////////////////
_attribute_ble_data_retention_ static u8 AuthNonceCCC[2] = {0, 0};                                          // AUTH_NONCE
_attribute_ble_data_retention_ static u8 AuthEncryptedDataCCC[2] = {0, 0};                                  // AUTH_ENCRYPTED_DATA

//////////////////////// CONTROL_SERVICE ///////////////////////////////
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
_attribute_ble_data_retention_ static u8 CtrlRingtoneCCC[2] = {0, 0};                                       // CTRL_RINGTONE
_attribute_ble_data_retention_ static u8 CtrlRingtoneVolumeCCC[2] = {0, 0};                                 // CTRL_RINGTONE_VOLUME
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
_attribute_ble_data_retention_ static u8 CtrlButtonCCC[2] = {0, 0};                                         // CTRL_BUTTON
#endif /* TAG_ACCESSORY_OPTION_BUTTON_ACTION */
_attribute_ble_data_retention_ static u8 CtrlBatteryCCC[2] = {0, 0};                                        // CTRL_BATTERY
_attribute_ble_data_retention_ static u8 CtrlTimeInformationCCC[2] = {0, 0};                                // CTRL_TIME_INFORMATION
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
_attribute_ble_data_retention_ static u8 CtrlUwbParamCCC[2] = {0, 0};                                       // CTRL_UWB_PARAM
#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
_attribute_ble_data_retention_ static u8 CtrlLedBlinkingCCC[2] = {0, 0};                                    // CTRL_LED_BLINKING
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
_attribute_ble_data_retention_ static u8 CtrlFirmwareTransferCCC[2] = {0, 0};;                              // CTRL_FIRMWARE_TRANSFER
#endif /* TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE */
_attribute_ble_data_retention_ static u8 CtrlBlePrivacyIdSettingCCC[2] = {0, 0};                            // CTRL_BLE_PRIVACY_ID_SETTING
#if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
_attribute_ble_data_retention_ static u8 CtrlPowerSavingModeCCC[2] = {0, 0};                                // CTRL_POWER_SAVING_MODE
#endif // TAG_ACCESSORY_OPTION_POWER_SAVING_MODE
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
_attribute_ble_data_retention_ static u8 CtrlDebugTagCCC[2] = {0, 0};                                       // CTRL_DEBUG_TAG
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
_attribute_ble_data_retention_ static u8 CtrlRingtoneNonOwnerCCC[2] = {0, 0};                               // CTRL_RINGTONE_NON_OWNER
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
_attribute_ble_data_retention_ static u8 CtrlUwbPowerNonOwnerCCC[2] = {0, 0};                               // CTRL_UWB_POWER_NON_OWNER
_attribute_ble_data_retention_ static u8 CtrlUwbParamNonOwnerCCC[2] = {0, 0};                               // CTRL_UWB_PARAM_NON_OWNER
#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
_attribute_ble_data_retention_ static u8 CtrlLedBlinkingNonOwnerCCC[2] = {0, 0};                            // CTRL_LED_BLINKING_NON_OWNER
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

//////////////////////// ONBOARDING_SERVICE ////////////////////////////
_attribute_ble_data_retention_ static u8 OnbdConfirmResultCCC[2] = {0, 0};                                  // ONBD_CONFIRM_RESULT
_attribute_ble_data_retention_ static u8 OnbdLoggingInitVCCC[2] = {0, 0};                                   // ONBD_LOGGING
/////////////////////////////////// SmartTings Find Device - CCC END ///////////////////////////////////////////

/////////////////////////////////// SmartTings Find Device - NAME START ///////////////////////////////////////////

//////////////////////// AUTH_SERVICE //////////////////////////////////
const u8 AuthCipherName[] = "AUTH_CIPHER";                                              // AUTH_CIPHER
const u8 AuthNonceName[] = "AUTH_NONCE";                                                // AUTH_NONCE
const u8 AuthEncryptedDataName[] = "AUTH_ENCRYPTED_DATA";                               // AUTH_ENCRYPTED_DATA

//////////////////////// CONTROL_SERVICE ///////////////////////////////
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
const u8 CtrlRingtoneName[] = "CTRL_RINGTONE";                                          // CTRL_RINGTONE
const u8 CtrlRingtoneVolumeName[] = "CTRL_RINGTONE_VOLUME";                             // CTRL_RINGTONE_VOLUME
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
const u8 CtrlButtonName[] = "CTRL_BUTTON";                                              // CTRL_BUTTON
#endif /* TAG_ACCESSORY_OPTION_BUTTON_ACTION */
const u8 CtrlBatteryName[] = "CTRL_BATTERY";                                            // CTRL_BATTERY
const u8 CtrlTimeInformationName[] = "CTRL_TIME_INFORMATION";                           // CTRL_TIME_INFORMATION
const u8 CtrlFactoryResetName[] = "CTRL_FACTORY_RESET";                                 // CTRL_FACTORY_RESET
const u8 CtrlE2eEncryptionName[] = "CTRL_E2E_ENCRYPTION";                               // CTRL_E2E_ENCRYPTION
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
const u8 CtrlUwbPowerName[] = "CTRL_UWB_POWER";                                         // CTRL_UWB_POWER
const u8 CtrlUwbParamName[] = "CTRL_UWB_PARAM";                                         // CTRL_UWB_PARAM
#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
const u8 CtrlLedBlinkingName[] = "CTRL_LED_BLINKING";                                   // CTRL_LED_BLINKING
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
#if defined(TAG_ACCESSORY_OPTION_UPDATE_RINGTONE) && (TAG_ACCESSORY_OPTION_UPDATE_RINGTONE == 1)
const u8 CtrlRingtoneUpdateName[] = "CTRL_RINGTONE_UPDATE";                             // CTRL_RINGTONE_UPDATE
#endif /* TAG_ACCESSORY_OPTION_UPDATE_RINGTONE */
const u8 CtrlFirmwareVersionName[] = "CTRL_FIRMWARE_VERSION";                           // CTRL_FIRMWARE_VERSION
#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
const u8 CtrlFirmwareTransferName[] = "CTRL_FIRMWARE_TRANSFER";                         // CTRL_FIRMWARE_TRANSFER
#endif /* TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE */
const u8 CtrlBleConnectionSettingName[] = "CTRL_BLE_CONNECTION_SETTING";                // CTRL_BLE_CONNECTION_SETTING
const u8 CtrlSpecificationVersionName[] = "CTRL_SPECIFICATION_VERSION";                 // CTRL_SPECIFICATION_VERSION
#if defined(TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT) && (TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT == 1)
const u8 CtrlBlePairingControlName[] = "CTRL_BLE_PAIRING_CONTROL";                      // CTRL_BLE_PAIRING_CONTROL
#endif // TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT
const u8 CtrlBlePrivacyIdSettingName[] = "CTRL_BLE_PRIVACY_ID_SETTING";                 // CTRL_BLE_PRIVACY_ID_SETTING
#if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
const u8 CtrlPowerSavingModeName[] = "CTRL_POWER_SAVING_MODE";                          // CTRL_POWER_SAVING_MODE
#endif // TAG_ACCESSORY_OPTION_POWER_SAVING_MODE
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
const u8 CtrlDebugTagName[] = "CTRL_DEBUG_TAG";                                         // CTRL_DEBUG_TAG
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
const u8 CtrlNfcLostMessageUrlName[] = "CTRL_NFC_LOST_MESSAGE_URL";                     // CTRL_NFC_LOST_MESSAGE_URL
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
const u8 CtrlRingtoneNonOwnerName[] = "CTRL_RINGTONE_NON_OWNER";                        // CTRL_RINGTONE_NON_OWNER
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
const u8 CtrlUwbPowerNonOwnerName[] = "CTRL_UWB_POWER_NON_OWNER";                       // CTRL_UWB_POWER_NON_OWNER
const u8 CtrlUwbParamNonOwnerName[] = "CTRL_UWB_PARAM_NON_OWNER";                       // CTRL_UWB_PARAM_NON_OWNER
#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
const u8 CtrlLedBlinkingNonOwnerName[] = "CTRL_LED_BLINKING_NON_OWNER";                 // CTRL_LED_BLINKING_NON_OWNER
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

//////////////////////// ONBOARDING_SERVICE ////////////////////////////
const u8 OnbdDeviceFirmwareVersionName[] = "ONBD_DEVICE_FIRMWARE_VERSION";              // ONBD_DEVICE_FIRMWARE_VERSION
const u8 OnbdBleScCapabilityName[] = "ONBD_BLE_SC_CAPABILITY";                          // ONBD_BLE_SC_CAPABILITY
const u8 OnbdHashedSerialNumberName[] = "ONBD_HASHED_SERIAL_NUMBER";                    // ONBD_HASHED_SERIAL_NUMBER
const u8 OnbdConfirmStatusName[] = "ONBD_CONFIRM_STATUS";                               // ONBD_CONFIRM_STATUS
const u8 OnbdMNMNName[] = "ONBD_MNMN";                                                  // ONBD_MNMN
const u8 OnbdVidName[] = "ONBD_VID";                                                    // ONBD_VID
const u8 OnbdIdentifierName[] = "ONBD_IDENTIFIER";                                      // ONBD_IDENTIFIER
const u8 OnbdConfigurationVersionName[] = "ONBD_CONFIGURATION_VERSION";                 // ONBD_CONFIGURATION_VERSION
const u8 OnbdSupportedCipherName[] = "ONBD_SUPPORTED_CIPHER";                           // ONBD_SUPPORTED_CIPHER
const u8 OnbdSelectedCipherName[] = "ONBD_SELECTED_CIPHER";                             // ONBD_SELECTED_CIPHER
const u8 OnbdSeedName[] = "ONBD_SEED";                                                  // ONBD_SEED
const u8 OnbdNumberOfPrivacyIdName[] = "ONBD_NUMBER_OF_PRIVACY_ID";                     // ONBD_NUMBER_OF_PRIVACY_ID
const u8 OnbdSetupCompleteName[] = "ONBD_SETUP_COMPLETE";                               // ONBD_SETUP_COMPLETE
const u8 OnbdSupportedConfirmMethodListName[] = "ONBD_SUPPORTED_CONFIRM_METHOD_LIST";   // ONBD_SUPPORTED_CONFIRM_METHOD_LIST
const u8 OnbdSelectedConfirmMethodName[] = "ONBD_SELECTED_CONFIRM_METHOD";              // ONBD_SELECTED_CONFIRM_METHOD
const u8 OnbdConfirmResultName[] = "ONBD_CONFIRM_RESULT";                               // ONBD_CONFIRM_RESULT
const u8 OnbdSerialConfirmName[] = "ONBD_SERIAL_CONFIRM";                               // ONBD_SERIAL_CONFIRM
const u8 OnbdCloudPublicKeyName[] = "ONBD_CLOUD_PUBLIC_KEY";                            // ONBD_CLOUD_PUBLIC_KEY
const u8 OnbdRandomValueName[] = "ONBD_RANDOM_VALUE";                                   // ONBD_RANDOM_VALUE
const u8 OnbdRegionName[] = "ONBD_REGION";                                              // ONBD_REGION
const u8 OnbdModelName[] = "ONBD_MODEL_NAME";                                           // ONBD_MODEL_NAME
const u8 OnbdLoggingName[] = "ONBD_LOGGING";                                            // ONBD_LOGGING
const u8 OnbdPrivacyIdVectorName[] = "ONBD_PRIVACY_ID_VECTOR";                          // ONBD_PRIVACY_ID_VECTOR
/////////////////////////////////// SmartTings Find Device - NAME END ///////////////////////////////////////////


//// GAP attribute values
static const u8 my_devNameCharVal[5] = {
    CHAR_PROP_READ,
    U16_LO(GenericAccess_DeviceName_DP_H),
    U16_HI(GenericAccess_DeviceName_DP_H),
    U16_LO(GATT_UUID_DEVICE_NAME),
    U16_HI(GATT_UUID_DEVICE_NAME)};
static const u8 my_appearanceCharVal[5] = {
    CHAR_PROP_READ,
    U16_LO(GenericAccess_Appearance_DP_H),
    U16_HI(GenericAccess_Appearance_DP_H),
    U16_LO(GATT_UUID_APPEARANCE),
    U16_HI(GATT_UUID_APPEARANCE)};
static const u8 my_periConnParamCharVal[5] = {
    CHAR_PROP_READ,
    U16_LO(CONN_PARAM_DP_H),
    U16_HI(CONN_PARAM_DP_H),
    U16_LO(GATT_UUID_PERI_CONN_PARAM),
    U16_HI(GATT_UUID_PERI_CONN_PARAM)};


//// GATT attribute values
static const u8 my_serviceChangeCharVal[5] = {
    CHAR_PROP_INDICATE,
    U16_LO(GenericAttribute_ServiceChanged_DP_H),
    U16_HI(GenericAttribute_ServiceChanged_DP_H),
    U16_LO(GATT_UUID_SERVICE_CHANGE),
    U16_HI(GATT_UUID_SERVICE_CHANGE)};


/////////////////////////////////// SmartTings Find Device - charVal START ///////////////////////////////////////////

//////////////////////// AUTH_SERVICE //////////////////////////////////
static const u8 AuthCipherCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE, };                                       // AUTH_CIPHER
static const u8 AuthNonceCharVal[1] = { CHAR_PROP_WRITE | CHAR_PROP_INDICATE, };                                    // AUTH_NONCE
static const u8 AuthEncryptedDataCharVal[1] = { CHAR_PROP_WRITE | CHAR_PROP_INDICATE, };                            // AUTH_ENCRYPTED_DATA

//////////////////////// CONTROL_SERVICE ///////////////////////////////
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
static const u8 CtrlRingtoneCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_INDICATE, };                // CTRL_RINGTONE
static const u8 CtrlRingtoneVolumeCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_NOTIFY, };            // CTRL_RINGTONE_VOLUME
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
const u8 CtrlButtonCharVal[1] = { CHAR_PROP_WRITE | CHAR_PROP_INDICATE, };                                          // CTRL_BUTTON
#endif /* TAG_ACCESSORY_OPTION_BUTTON_ACTION */
const u8 CtrlBatteryCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_INDICATE, };                                          // CTRL_BATTERY
const u8 CtrlTimeInformationCharVal[1] = { CHAR_PROP_WRITE | CHAR_PROP_INDICATE, };                                 // CTRL_TIME_INFORMATION
const u8 CtrlFactoryResetCharVal[1] = { CHAR_PROP_WRITE, };                                                         // CTRL_FACTORY_RESET
const u8 CtrlE2eEncryptionCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE, };                                       // CTRL_E2E_ENCRYPTION
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
const u8 CtrlUwbPowerCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE, };                                            // CTRL_UWB_POWER
const u8 CtrlUwbParamCharVal[1] = { CHAR_PROP_WRITE | CHAR_PROP_INDICATE, };                                        // CTRL_UWB_PARAM
#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
const u8 CtrlLedBlinkingCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_INDICATE, };                    // CTRL_LED_BLINKING
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
#if defined(TAG_ACCESSORY_OPTION_UPDATE_RINGTONE) && (TAG_ACCESSORY_OPTION_UPDATE_RINGTONE == 1)
const u8 CtrlRingtoneUpdateCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE, };                                      // CTRL_RINGTONE_UPDATE
#endif /* TAG_ACCESSORY_OPTION_UPDATE_RINGTONE */
const u8 CtrlFirmwareVersionCharVal[1] = { CHAR_PROP_READ, };                           // CTRL_FIRMWARE_VERSION
#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
const u8 CtrlFirmwareTransferCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE_WITHOUT_RSP | CHAR_PROP_INDICATE, };   // CTRL_FIRMWARE_TRANSFER
#endif /* TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE */
const u8 CtrlBleConnectionSettingCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE, };                                // CTRL_BLE_CONNECTION_SETTING
const u8 CtrlSpecificationVersionCharVal[1] = { CHAR_PROP_READ, };                                                  // CTRL_SPECIFICATION_VERSION
#if defined(TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT) && (TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT == 1)
const u8 CtrlBlePairingControlCharVal[1] = { CHAR_PROP_WRITE, };                                                    // CTRL_BLE_PAIRING_CONTROL
#endif // TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT
const u8 CtrlBlePrivacyIdSettingCharVal[1] = { CHAR_PROP_WRITE | CHAR_PROP_INDICATE, };                             // CTRL_BLE_PRIVACY_ID_SETTING
#if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
const u8 CtrlPowerSavingModeCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_INDICATE, };                // CTRL_POWER_SAVING_MODE
#endif // TAG_ACCESSORY_OPTION_POWER_SAVING_MODE
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
#ifdef TAG_CONFIG_USE_NOTIFICATION_FOR_DEBUG
const u8 CtrlDebugTagCharVal[1] = { CHAR_PROP_WRITE | CHAR_PROP_NOTIFY, };                                          // CTRL_DEBUG_TAG
#else
const u8 CtrlDebugTagCharVal[1] = { CHAR_PROP_WRITE | CHAR_PROP_INDICATE, };                                        // CTRL_DEBUG_TAG
#endif
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
const u8 CtrlNfcLostMessageUrlCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE, };                                   // CTRL_NFC_LOST_MESSAGE_URL
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
const u8 CtrlRingtoneNonOwnerCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_NOTIFY, };                 // CTRL_RINGTONE_NON_OWNER
#endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
const u8 CtrlUwbPowerNonOwnerCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE, };                                    // CTRL_UWB_POWER_NON_OWNER
const u8 CtrlUwbParamNonOwnerCharVal[1] = { CHAR_PROP_WRITE | CHAR_PROP_INDICATE, };                                // CTRL_UWB_PARAM_NON_OWNER
#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
const u8 CtrlLedBlinkingNonOwnerCharVal[1] = { CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_NOTIFY, };              // CTRL_LED_BLINKING_NON_OWNER
#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

//////////////////////// ONBOARDING_SERVICE ////////////////////////////
static const u8 OnbdDeviceFirmwareVersionCharVal[1] = { CHAR_PROP_READ, };                                          // ONBD_DEVICE_FIRMWARE_VERSION
static const u8 OnbdBleScCapabilityCharVal[1] = { CHAR_PROP_READ, };                                                // ONBD_BLE_SC_CAPABILITY
static const u8 OnbdHashedSerialNumberCharVal[1] = { CHAR_PROP_READ, };                                             // ONBD_HASHED_SERIAL_NUMBER
static const u8 OnbdConfirmStatusCharVal[1] = { CHAR_PROP_READ, };                                                  // ONBD_CONFIRM_STATUS
static const u8 OnbdMNMNCharVal[1] = { CHAR_PROP_READ, };                                                           // ONBD_MNMN
static const u8 OnbdVidCharVal[1] = { CHAR_PROP_READ, };                                                            // ONBD_VID
static const u8 OnbdIdentifierCharVal[1] = { CHAR_PROP_READ, };                                                     // ONBD_IDENTIFIER
static const u8 OnbdConfigurationVersionCharVal[1] = { CHAR_PROP_READ, };                                           // ONBD_CONFIGURATION_VERSION
static const u8 OnbdSupportedCipherCharVal[1] = { CHAR_PROP_READ, };                                                // ONBD_SUPPORTED_CIPHER
static const u8 OnbdSelectedCipherCharVal[1] = { CHAR_PROP_WRITE, };                                                // ONBD_SELECTED_CIPHER
static const u8 OnbdSeedCharVal[1] = { CHAR_PROP_WRITE, };                                                          // ONBD_SEED
static const u8 OnbdNumberOfPrivacyIdCharVal[1] = { CHAR_PROP_WRITE, };                                             // ONBD_NUMBER_OF_PRIVACY_ID
static const u8 OnbdSetupCompleteCharVal[1] = { CHAR_PROP_WRITE, };                                                 // ONBD_SETUP_COMPLETE
static const u8 OnbdSupportedConfirmMethodListCharVal[1] = { CHAR_PROP_READ, };                                     // ONBD_SUPPORTED_CONFIRM_METHOD_LIST
static const u8 OnbdSelectedConfirmMethodCharVal[1] = { CHAR_PROP_WRITE, };                                         // ONBD_SELECTED_CONFIRM_METHOD
static const u8 OnbdConfirmResultCharVal[1] = { CHAR_PROP_INDICATE, };                                              // ONBD_CONFIRM_RESULT
static const u8 OnbdSerialConfirmCharVal[1] = { CHAR_PROP_WRITE, };                                                 // ONBD_SERIAL_CONFIRM
static const u8 OnbdCloudPublicKeyCharVal[1] = { CHAR_PROP_WRITE, };                                                // ONBD_CLOUD_PUBLIC_KEY
static const u8 OnbdRandomValueCharVal[1] = { CHAR_PROP_WRITE, };                                                   // ONBD_RANDOM_VALUE
static const u8 OnbdRegionCharVal[1] = { CHAR_PROP_WRITE, };                                                        // ONBD_REGION
static const u8 OnbdModelCharVal[1] = { CHAR_PROP_READ, };                                                          // ONBD_MODEL_NAME
static const u8 OnbdLoggingCharVal[1] = { CHAR_PROP_WRITE | CHAR_PROP_INDICATE, };                                  // ONBD_LOGGING
static const u8 OnbdPrivacyIdVectorCharVal[1] = { CHAR_PROP_WRITE, };                                               // ONBD_PRIVACY_ID_VECTOR
/////////////////////////////////// SmartTings Find Device - charVal END ///////////////////////////////////////////


/////////////////////////////////// SmartTings Find Device - read/write callback END ///////////////////////////////////////////

#define SMART_CONTROL_DEFAULT_SERVICE (27)
#define SMART_CONTROL_OPTIONAL_SERVICE (                    \
    (12) * TAG_ACCESSORY_OPTION_RING_THE_TAG +              \
    (4) * TAG_ACCESSORY_OPTION_BUTTON_ACTION +              \
    (8) * TAG_ACCESSORY_OPTION_LED_BLINKING +               \
    (3) * TAG_ACCESSORY_OPTION_UPDATE_RINGTONE +            \
    (4) * TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE +            \
    (3) * TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT +          \
    (4) * TAG_ACCESSORY_OPTION_POWER_SAVING_MODE +          \
    (3) * TAG_ACCESSORY_OPTION_LOST_MESSAGE)

#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
#define SMART_CONTROL_SERVICE_USE_DEBUG (4)
#else
#define SMART_CONTROL_SERVICE_USE_DEBUG (0)
#endif
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
#define SMART_CONTROL_SERVICE_USE_UWB (14)
#else
#define SMART_CONTROL_SERVICE_USE_UWB (0)
#endif

#define SMART_CONTROL_SERVICE_TOTAL (   \
    SMART_CONTROL_DEFAULT_SERVICE +     \
    SMART_CONTROL_OPTIONAL_SERVICE +    \
    SMART_CONTROL_SERVICE_USE_DEBUG +   \
    SMART_CONTROL_SERVICE_USE_UWB)

// AUTH_SERVICE + CONTROL_SERVICE + ONBOARDING_SERVICE
#define SMART_AUTH_SERVICE          ((11) + (1))
#define SMART_ONBOARDING_SERVICE    ((71) + (1))
#define SMART_CONTROL_SERVICE       (SMART_CONTROL_SERVICE_TOTAL + (1))
#define SMART_SERVICE_TOTAL         (SMART_AUTH_SERVICE + SMART_ONBOARDING_SERVICE + SMART_CONTROL_SERVICE)


_attribute_ble_data_retention_ attribute_t* g_MyAttributes;
// TM : to modify

const attribute_t my_Attributes[] = {

    {ATT_END_H - 1 + SMART_SERVICE_TOTAL,   0,                     0,  0,                                         NULL,                                            NULL,                                              NULL,                                         NULL}, // total num of attribute


    // 0001 - 0007  gap
    {7,                                     ATT_PERMISSIONS_READ,  2,  2,                                         (u8 *)(size_t)(&my_primaryServiceUUID),          (u8 *)(size_t)(&my_gapServiceUUID),                0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_devNameCharVal),                 (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(my_devNameCharVal),                 0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_devName),                        (u8 *)(size_t)(&my_devNameUUID),                 (u8 *)(size_t)(my_devName),                        0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_appearanceCharVal),              (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(my_appearanceCharVal),              0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_appearance),                     (u8 *)(size_t)(&my_appearanceUUID),              (u8 *)(size_t)(&my_appearance),                    0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_periConnParamCharVal),           (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(my_periConnParamCharVal),           0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_periConnParameters),             (u8 *)(size_t)(&my_periConnParamUUID),           (u8 *)(size_t)(&my_periConnParameters),            0,                                            0   },


    // 0008 - 000b gatt
    {4,                                     ATT_PERMISSIONS_READ,  2,  2,                                         (u8 *)(size_t)(&my_primaryServiceUUID),          (u8 *)(size_t)(&my_gattServiceUUID),               0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(my_serviceChangeCharVal),           (u8 *)(size_t)(&my_characterUUID),               (u8 *)(size_t)(my_serviceChangeCharVal),           0,                                            0   },
    {0,                                     ATT_PERMISSIONS_READ,  2,  sizeof(serviceChangeVal),                  (u8 *)(size_t)(&serviceChangeUUID),              (u8 *)(&serviceChangeVal),                         0,                                            0   },
    {0,                                     ATT_PERMISSIONS_RDWR,  2,  sizeof(serviceChangeCCC),                  (u8 *)(size_t)(&clientCharacterCfgUUID),         (u8 *)(serviceChangeCCC),                          0,                                            0   },


    /////////////////////////////////// 5. SmartTings Find Device ///////////////////////////////////////////

    ////////////////////////////////////// AUTH_SERVICE /////////////////////////////////////////////////////
    {SMART_AUTH_SERVICE, ATT_PERMISSIONS_READ,   2,  16,  (u8 *)(size_t)(&my_primaryServiceUUID),    (u8 *)(size_t)(&uuid_auth_service_t),   0,  0},
        // Offset: 001 - 003, AUTH_CIPHER
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(AuthCipherCharVal),          (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(AuthCipherCharVal),              0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(AuthCipherInitV),            (u8 *)(size_t)(&uuid_cipher_t),             (u8 *)(size_t)(&AuthCipherInitV),               (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(AuthCipherName),             (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(AuthCipherName),                 0,                      0   },
        // Offset: 004 - 007, AUTH_NONCE
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(AuthNonceCharVal),           (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(AuthNonceCharVal),               0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(AuthNonceInitV),             (u8 *)(size_t)(&uuid_nonce_t),              (u8 *)(size_t)(&AuthNonceInitV),                (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(AuthNonceCCC),               (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(AuthNonceCCC),                           (att_readwrite_callback_t)&portble_writeccc,                       0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(AuthNonceName),              (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(AuthNonceName),                  0,                      0   },
        // Offset: 008 - 011, AUTH_ENCRYPTED_DATA
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(AuthEncryptedDataCharVal),   (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(AuthEncryptedDataCharVal),       0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(AuthEncryptedDataInitV),     (u8 *)(size_t)(&uuid_encrypted_data_t),     (u8 *)(size_t)(&AuthEncryptedDataInitV),        (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(AuthEncryptedDataCCC),       (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(AuthEncryptedDataCCC),                   (att_readwrite_callback_t)&portble_writeccc,      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(AuthEncryptedDataName),      (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(AuthEncryptedDataName),          0,                      0   },

    ////////////////////////////////////// CONTROL_SERVICE /////////////////////////////////////////////////////
    {SMART_CONTROL_SERVICE, ATT_PERMISSIONS_READ,   2,  2,  (u8 *)(size_t)(&my_primaryServiceUUID), (u8 *)(size_t)(&uuid_tag_service_t),    0,  0},
    #if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
        // CTRL_RINGTONE
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlRingtoneCharVal),        (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlRingtoneCharVal),            0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(CtrlRingtoneInitV),          (u8 *)(size_t)(&uuid_ringtone_t),           (u8 *)(size_t)(&CtrlRingtoneInitV),             (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlRingtoneCCC),            (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlRingtoneCCC),                        (att_readwrite_callback_t)&portble_writeccc,                      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlRingtoneName),           (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlRingtoneName),               0,                      0   },
        // CTRL_RINGTONE_VOLUME
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlRingtoneVolumeCharVal),  (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlRingtoneVolumeCharVal),      0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(CtrlRingtoneVolumeInitV),    (u8 *)(size_t)(&uuid_ringtone_volume_t),     (u8 *)(size_t)(&CtrlRingtoneVolumeInitV),       (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlRingtoneVolumeCCC),      (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlRingtoneVolumeCCC),                  (att_readwrite_callback_t)&portble_writeccc,                       0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlRingtoneVolumeName),     (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlRingtoneVolumeName),         0,                      0   },
    #endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
    #if defined(TAG_ACCESSORY_OPTION_BUTTON_ACTION) && (TAG_ACCESSORY_OPTION_BUTTON_ACTION == 1)
        // CTRL_BUTTON
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlButtonCharVal),          (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlButtonCharVal),              0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(CtrlButtonInitV),            (u8 *)(size_t)(&uuid_button_t),             (u8 *)(size_t)(&CtrlButtonInitV),               (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlButtonCCC),              (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlButtonCCC),                          (att_readwrite_callback_t)&portble_writeccc,                      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlButtonName),             (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlButtonName),                 0,                      0   },
    #endif /* TAG_ACCESSORY_OPTION_BUTTON_ACTION */
        // CTRL_BATTERY
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlBatteryCharVal),         (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlBatteryCharVal),             0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(CtrlBatteryInitV),           (u8 *)(size_t)(&uuid_battery_t),            (u8 *)(size_t)(&CtrlBatteryInitV),              NULL,                   (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlBatteryCCC),             (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlBatteryCCC),                         (att_readwrite_callback_t)&portble_writeccc,                      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlBatteryName),            (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlBatteryName),                0,                      0   },
        // CTRL_TIME_INFORMATION
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlTimeInformationCharVal), (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlTimeInformationCharVal),     0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(CtrlTimeInformationInitV),   (u8 *)(size_t)(&uuid_time_information_t),   (u8 *)(size_t)(&CtrlTimeInformationInitV),      (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlTimeInformationCCC),     (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlTimeInformationCCC),                 (att_readwrite_callback_t)&portble_writeccc,                      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlTimeInformationName),    (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlTimeInformationName),        0,                      0   },
        // CTRL_FACTORY_RESET
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlFactoryResetCharVal),    (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlFactoryResetCharVal),        0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(CtrlFactoryResetInitV),      (u8 *)(size_t)(&uuid_factory_reset_t),      (u8 *)(size_t)(&CtrlFactoryResetInitV),         (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlFactoryResetName),       (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlFactoryResetName),           0,                      0   },
        // CTRL_E2E_ENCRYPTION
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlE2eEncryptionCharVal),   (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlE2eEncryptionCharVal),       0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(CtrlE2eEncryptionInitV),     (u8 *)(size_t)(&uuid_e2e_encryption_t),     (u8 *)(size_t)(&CtrlE2eEncryptionInitV),        (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlE2eEncryptionName),      (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlE2eEncryptionName),          0,                      0   },
    #ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
        // CTRL_UWB_POWER
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlUwbPowerCharVal),        (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlUwbPowerCharVal),            0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(CtrlUwbPowerInitV),          (u8 *)(size_t)(&uuid_uwb_power_t),          (u8 *)(size_t)(&CtrlUwbPowerInitV),             (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlUwbPowerName),           (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlUwbPowerName),               0,                      0   },
        // CTRL_UWB_PARAM
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlUwbParamCharVal),        (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlUwbParamCharVal),            0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(CtrlUwbParamInitV),          (u8 *)(size_t)(&uuid_uwb_param_t),          (u8 *)(size_t)(&CtrlUwbParamInitV),             (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlUwbParamCCC),            (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlUwbParamCCC),                        0,                      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlUwbParamName),           (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlUwbParamName),               0,                      0   },
    #endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */
    #if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
        // CTRL_LED_BLINKING
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlLedBlinkingCharVal),     (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlLedBlinkingCharVal),         0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(CtrlLedBlinkingInitV),       (u8 *)(size_t)(&uuid_led_blinking_t),       (u8 *)(size_t)(&CtrlLedBlinkingInitV),          (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlLedBlinkingCCC),         (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlLedBlinkingCCC),                     0,                      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlLedBlinkingName),        (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlLedBlinkingName),            0,                      0   },
    #endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */
    #if defined(TAG_ACCESSORY_OPTION_UPDATE_RINGTONE) && (TAG_ACCESSORY_OPTION_UPDATE_RINGTONE == 1)
        // CTRL_RINGTONE_UPDATE
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlRingtoneUpdateCharVal),  (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlRingtoneUpdateCharVal),      0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(CtrlRingtoneUpdateInitV),    (u8 *)(size_t)(&uuid_ringtone_update_t),    (u8 *)(size_t)(&CtrlRingtoneUpdateInitV),       (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlRingtoneUpdateName),     (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlRingtoneUpdateName),         0,                      0   },
    #endif /* TAG_ACCESSORY_OPTION_UPDATE_RINGTONE */
        // CTRL_FIRMWARE_VERSION
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlFirmwareVersionCharVal), (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlFirmwareVersionCharVal),     0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(CtrlFirmwareVersionInitV),   (u8 *)(size_t)(&uuid_firmware_version_t),   (u8 *)(size_t)(&CtrlFirmwareVersionInitV),      NULL,                   (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlFirmwareVersionName),    (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlFirmwareVersionName),        0,                      0   },
    #if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)
        // CTRL_FIRMWARE_TRANSFER
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlFirmwareTransferCharVal),(u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlFirmwareTransferCharVal),    0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(CtrlFirmwareTransferInitV),  (u8 *)(size_t)(&uuid_firmware_transfer_t),  (u8 *)(size_t)(&CtrlFirmwareTransferInitV),     (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlFirmwareTransferCCC),    (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlFirmwareTransferCCC),                (att_readwrite_callback_t)&portble_writeccc,                      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlFirmwareTransferName),   (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlFirmwareTransferName),       0,                      0   },
    #endif /* TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE */
        // CTRL_BLE_CONNECTION_SETTING
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlBleConnectionSettingCharVal),(u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlBleConnectionSettingCharVal),    0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(CtrlBleConnectionSettingInitV),  (u8 *)(size_t)(&uuid_ble_connection_setting_t),(u8 *)(size_t)(&CtrlBleConnectionSettingInitV),  (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlBleConnectionSettingName),   (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlBleConnectionSettingName),       0,                      0   },
        // CTRL_SPECIFICATION_VERSION
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlSpecificationVersionCharVal),(u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlSpecificationVersionCharVal),    0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(CtrlSpecificationVersionInitV),  (u8 *)(size_t)(&uuid_specification_version_t),(u8 *)(size_t)(&CtrlSpecificationVersionInitV),   NULL,                   (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlSpecificationVersionName),   (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlSpecificationVersionName),       0,                      0   },
    #if defined(TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT) && (TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT == 1)
        // CTRL_BLE_PAIRING_CONTROL
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlBlePairingControlCharVal),   (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlBlePairingControlCharVal),       0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(CtrlBlePairingControlInitV),     (u8 *)(size_t)(&uuid_ble_pairing_control_t),(u8 *)(size_t)(&CtrlBlePairingControlInitV),        (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlBlePairingControlName),      (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlBlePairingControlName),          0,                      0   },
    #endif /* TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT */
        // CTRL_BLE_PRIVACY_ID_SETTING
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlBlePrivacyIdSettingCharVal), (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlBlePrivacyIdSettingCharVal),     0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(CtrlBlePrivacyIdSettingInitV),   (u8 *)(size_t)(&uuid_ble_privacy_id_setting_t),(u8 *)(size_t)(&CtrlBlePrivacyIdSettingInitV),   (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlBlePrivacyIdSettingCCC),     (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlBlePrivacyIdSettingCCC),                 (att_readwrite_callback_t)&portble_writeccc,                      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlBlePrivacyIdSettingName),    (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlBlePrivacyIdSettingName),        0,                      0   },
    #if defined(TAG_ACCESSORY_OPTION_POWER_SAVING_MODE) && (TAG_ACCESSORY_OPTION_POWER_SAVING_MODE == 1)
        // CTRL_POWER_SAVING_MODE
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlPowerSavingModeCharVal), (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlPowerSavingModeCharVal),     0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(CtrlPowerSavingModeInitV),   (u8 *)(size_t)(&uuid_power_saving_mode_t),  (u8 *)(size_t)(&CtrlPowerSavingModeInitV),      (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlPowerSavingModeCCC),     (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlPowerSavingModeCCC),                 (att_readwrite_callback_t)&portble_writeccc,                      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlPowerSavingModeName),    (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlPowerSavingModeName),        0,                      0   },
    #endif /* TAG_ACCESSORY_OPTION_POWER_SAVING_MODE */
    #ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
        // CTRL_DEBUG_TAG
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlDebugTagCharVal),        (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlDebugTagCharVal),            0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(CtrlDebugTagInitV),          (u8 *)(size_t)(&uuid_debug_t),              (u8 *)(size_t)(&CtrlDebugTagInitV),             (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlDebugTagCCC),            (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlDebugTagCCC),                        (att_readwrite_callback_t)&portble_writeccc,                      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlDebugTagName),           (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlDebugTagName),               0,                      0   },
    #endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
    #if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
        // CTRL_NFC_LOST_MESSAGE_URL
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlNfcLostMessageUrlCharVal),(u8 *)(size_t)(&my_characterUUID),         (u8 *)(size_t)(CtrlNfcLostMessageUrlCharVal),   0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(CtrlNfcLostMessageUrlInitV), (u8 *)(size_t)(&uuid_nfc_lost_message_url_t),(u8 *)(size_t)(&CtrlNfcLostMessageUrlInitV),   (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlNfcLostMessageUrlName),  (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlNfcLostMessageUrlName),      0,                      0   },
    #endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */
    #if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
        // CTRL_RINGTONE_NON_OWNER
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlRingtoneNonOwnerCharVal),(u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlRingtoneNonOwnerCharVal),    0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(CtrlRingtoneNonOwnerInitV),  (u8 *)(size_t)(&uuid_ringtone_non_owner_t), (u8 *)(size_t)(&CtrlRingtoneNonOwnerInitV),     (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlRingtoneNonOwnerCCC),    (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlRingtoneNonOwnerCCC),                (att_readwrite_callback_t)&portble_writeccc,                      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlRingtoneNonOwnerName),   (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlRingtoneNonOwnerName),       0,                      0   },
    #endif /* TAG_ACCESSORY_OPTION_RING_THE_TAG */
    #ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
        // CTRL_UWB_POWER_NON_OWNER
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlUwbPowerNonOwnerCharVal),(u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlUwbPowerNonOwnerCharVal),    0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(CtrlUwbPowerNonOwnerInitV),  (u8 *)(size_t)(&uuid_uwb_power_t),          (u8 *)(size_t)(&CtrlUwbPowerNonOwnerInitV),     (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlUwbPowerNonOwnerName),   (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlUwbPowerNonOwnerName),       0,                      0   },
        // CTRL_UWB_PARAM_NON_OWNER
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlUwbParamNonOwnerCharVal),(u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlUwbParamNonOwnerCharVal),    0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(CtrlUwbParamNonOwnerInitV),  (u8 *)(size_t)(&uuid_uwb_param_t),          (u8 *)(size_t)(&CtrlUwbParamNonOwnerInitV),     (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlUwbParamNonOwnerCCC),    (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlUwbParamNonOwnerCCC),                (att_readwrite_callback_t)&portble_writeccc,                      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlUwbParamNonOwnerName),   (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlUwbParamNonOwnerName),       0,                      0   },
    #endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */
    #if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
        // CTRL_LED_BLINKING_NON_OWNER
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlLedBlinkingNonOwnerCharVal), (u8 *)(size_t)(&my_characterUUID),          (u8 *)(size_t)(CtrlLedBlinkingNonOwnerCharVal),     0,                      0   },              // prop
        {0,     ATT_PERMISSIONS_RDWR,   16, sizeof(CtrlLedBlinkingNonOwnerInitV),   (u8 *)(size_t)(&uuid_led_blinking_non_owner_t),(u8 *)(size_t)(&CtrlLedBlinkingNonOwnerInitV),   (att_readwrite_callback_t)&PortBle_writeData,     (att_readwrite_callback_t)&PortBle_ReadData},// value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(CtrlLedBlinkingNonOwnerCCC),     (u8 *)(size_t)(&clientCharacterCfgUUID),    (u8 *)(CtrlLedBlinkingNonOwnerCCC),                 (att_readwrite_callback_t)&portble_writeccc,                      0   },              // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(CtrlLedBlinkingNonOwnerName),    (u8 *)(size_t)(&userdesc_UUID),             (u8 *)(size_t)(CtrlLedBlinkingNonOwnerName),        0,                      0   },
    #endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

    ////////////////////////////////////// ONBOARDING_SERVICE /////////////////////////////////////////////////////
    {SMART_ONBOARDING_SERVICE,  ATT_PERMISSIONS_READ,   2,  2,  (u8 *)(size_t)(&my_primaryServiceUUID), (u8 *)(size_t)(&uuid_onboarding_service_t), 0,  0},
        // Offset: 001 - 003, ONBD_DEVICE_FIRMWARE_VERSION
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdDeviceFirmwareVersionCharVal),   (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdDeviceFirmwareVersionCharVal),       0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(OnbdDeviceFirmwareVersionInitV),     (u8 *)(size_t)(&uuid_device_firmware_version_t),    (u8 *)(size_t)(&OnbdDeviceFirmwareVersionInitV),        NULL,                   (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdDeviceFirmwareVersionName),      (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdDeviceFirmwareVersionName),          0,                      0   },
        // Offset: 004 - 006, ONBD_BLE_SC_CAPABILITY
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdBleScCapabilityCharVal),         (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdBleScCapabilityCharVal),             0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(OnbdBleScCapabilityInitV),           (u8 *)(size_t)(&uuid_ble_sc_capability_t),          (u8 *)(size_t)(&OnbdBleScCapabilityInitV),              NULL,                   (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdBleScCapabilityName),            (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdBleScCapabilityName),                0,                      0   },
        // Offset: 007 - 009, ONBD_HASHED_SERIAL_NUMBER
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdHashedSerialNumberCharVal),      (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdHashedSerialNumberCharVal),          0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(OnbdHashedSerialNumberInitV),        (u8 *)(size_t)(&uuid_hashed_serial_number_t),       (u8 *)(size_t)(&OnbdHashedSerialNumberInitV),           NULL,                   (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdHashedSerialNumberName),         (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdHashedSerialNumberName),             0,                      0   },
        // Offset: 010 - 012, ONBD_CONFIRM_STATUS
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdConfirmStatusCharVal),           (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdConfirmStatusCharVal),               0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(OnbdConfirmStatusInitV),             (u8 *)(size_t)(&uuid_confirm_status_t),             (u8 *)(size_t)(&OnbdConfirmStatusInitV),                NULL,                   (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdConfirmStatusName),              (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdConfirmStatusName),                  0,                      0   },
        // Offset: 013 - 015, ONBD_MNMN
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdMNMNCharVal),                    (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdMNMNCharVal),                        0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(OnbdMNMNInitV),                      (u8 *)(size_t)(&uuid_mnmn_t),                       (u8 *)(size_t)(&OnbdMNMNInitV),                         NULL,                   (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdMNMNName),                       (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdMNMNName),                           0,                      0   },
        // Offset: 016 - 018, ONBD_VID
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdVidCharVal),                     (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdVidCharVal),                         0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(OnbdVidInitV),                       (u8 *)(size_t)(&uuid_vid_t),                        (u8 *)(size_t)(&OnbdVidInitV),                          NULL,                   (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdVidName),                        (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdVidName),                            0,                      0   },
        // Offset: 019 - 021, ONBD_IDENTIFIER
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdIdentifierCharVal),              (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdIdentifierCharVal),                  0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(OnbdIdentifierInitV),                (u8 *)(size_t)(&uuid_identifier_t),                 (u8 *)(size_t)(&OnbdIdentifierInitV),                   NULL,                   (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdIdentifierName),                 (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdIdentifierName),                     0,                      0   },
        // Offset: 022 - 024, ONBD_CONFIGURATION_VERSION
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdConfigurationVersionCharVal),    (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdConfigurationVersionCharVal),        0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(OnbdConfigurationVersionInitV),      (u8 *)(size_t)(&uuid_configuration_version_t),      (u8 *)(size_t)(&OnbdConfigurationVersionInitV),         NULL,                   (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdConfigurationVersionName),       (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdConfigurationVersionName),           0,                      0   },
        // Offset: 025 - 027, ONBD_SUPPORTED_CIPHER
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSupportedCipherCharVal),         (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdSupportedCipherCharVal),             0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(OnbdSupportedCipherInitV),           (u8 *)(size_t)(&uuid_supported_cipher_t),           (u8 *)(size_t)(&OnbdSupportedCipherInitV),              NULL,                   (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSupportedCipherName),            (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdSupportedCipherName),                0,                      0   },
        // Offset: 028 - 030, ONBD_SELECTED_CIPHER
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSelectedCipherCharVal),          (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdSelectedCipherCharVal),              0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(OnbdSelectedCipherInitV),            (u8 *)(size_t)(&uuid_selected_cipher_t),            (u8 *)(size_t)(&OnbdSelectedCipherInitV),               (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSelectedCipherName),             (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdSelectedCipherName),                 0,                      0   },
        // Offset: 031 - 033, ONBD_SEED
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSeedCharVal),                    (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdSeedCharVal),                        0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(OnbdSeedInitV),                      (u8 *)(size_t)(&uuid_seed_t),                       (u8 *)(size_t)(&OnbdSeedInitV),                         (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSeedName),                       (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdSeedName),                           0,                      0   },
        // Offset: 034 - 036, ONBD_NUMBER_OF_PRIVACY_ID
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdNumberOfPrivacyIdCharVal),       (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdNumberOfPrivacyIdCharVal),           0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(OnbdNumberOfPrivacyIdInitV),         (u8 *)(size_t)(&uuid_number_of_privacy_id_t),       (u8 *)(size_t)(&OnbdNumberOfPrivacyIdInitV),            (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdNumberOfPrivacyIdName),          (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdNumberOfPrivacyIdName),              0,                      0   },
        // Offset: 037 - 039, ONBD_SETUP_COMPLETE
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSetupCompleteCharVal),           (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdSetupCompleteCharVal),               0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(OnbdSetupCompleteInitV),             (u8 *)(size_t)(&uuid_setup_complete_t),             (u8 *)(size_t)(&OnbdSetupCompleteInitV),                (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSetupCompleteName),              (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdSetupCompleteName),                  0,                      0   },
        // Offset: 040 - 042, ONBD_SUPPORTED_CONFIRM_METHOD_LIST
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSupportedConfirmMethodListCharVal),  (u8 *)(size_t)(&my_characterUUID),                    (u8 *)(size_t)(OnbdSupportedConfirmMethodListCharVal),  0,                0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(OnbdSupportedConfirmMethodListInitV),    (u8 *)(size_t)(&uuid_supported_confirm_method_list_t),(u8 *)(size_t)(&OnbdSupportedConfirmMethodListInitV),   NULL,             (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSupportedConfirmMethodListName),     (u8 *)(size_t)(&userdesc_UUID),                       (u8 *)(size_t)(OnbdSupportedConfirmMethodListName),     0,                0   },
        // Offset: 043 - 045, ONBD_SELECTED_CONFIRM_METHOD
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSelectedConfirmMethodCharVal),   (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdSelectedConfirmMethodCharVal),       0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(OnbdSelectedConfirmMethodInitV),     (u8 *)(size_t)(&uuid_selected_confirm_method_t),    (u8 *)(size_t)(&OnbdSelectedConfirmMethodInitV),        (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSelectedConfirmMethodName),      (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdSelectedConfirmMethodName),          0,                      0   },
        // Offset: 046 - 049, ONBD_CONFIRM_RESULT
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdConfirmResultCharVal),           (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdConfirmResultCharVal),               0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(OnbdConfirmResultInitV),             (u8 *)(size_t)(&uuid_confirm_result_t),             (u8 *)(size_t)(&OnbdConfirmResultInitV),                NULL,                   NULL},                  // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(OnbdConfirmResultCCC),               (u8 *)(size_t)(&clientCharacterCfgUUID),            (u8 *)(OnbdConfirmResultCCC),                           (att_readwrite_callback_t)&portble_writeccc,                       0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdConfirmResultName),              (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdConfirmResultName),                  0,                      0   },
        // Offset: 050 - 052, ONBD_SERIAL_CONFIRM
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSerialConfirmCharVal),           (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdSerialConfirmCharVal),               0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(OnbdSerialConfirmInitV),             (u8 *)(size_t)(&uuid_serial_confirm_t),             (u8 *)(size_t)(&OnbdSerialConfirmInitV),                (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdSerialConfirmName),              (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdSerialConfirmName),                  0,                      0   },
        // Offset: 053 - 055, ONBD_CLOUD_PUBLIC_KEY
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdCloudPublicKeyCharVal),          (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdCloudPublicKeyCharVal),              0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(OnbdCloudPublicKeyInitV),            (u8 *)(size_t)(&uuid_cloud_public_key_t),           (u8 *)(size_t)(&OnbdCloudPublicKeyInitV),               (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdCloudPublicKeyName),             (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdCloudPublicKeyName),                 0,                      0   },
        // Offset: 056 - 058, ONBD_RANDOM_VALUE
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdRandomValueCharVal),             (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdRandomValueCharVal),                 0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(OnbdRandomValueInitV),               (u8 *)(size_t)(&uuid_random_value_t),               (u8 *)(size_t)(&OnbdRandomValueInitV),                  (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdRandomValueName),                (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdRandomValueName),                    0,                      0   },
        // Offset: 059 - 061, ONBD_REGION
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdRegionCharVal),                  (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdRegionCharVal),                      0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(OnbdRegionInitV),                    (u8 *)(size_t)(&uuid_region_t),                     (u8 *)(size_t)(&OnbdRegionInitV),                       (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdRegionName),                     (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdRegionName),                         0,                      0   },
        // Offset: 062 - 064, ONBD_MODEL_NAME
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdModelCharVal),                   (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdModelCharVal),                       0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_READ,   16, sizeof(OnbdModelInitV),                     (u8 *)(size_t)(&uuid_model_name_t),                 (u8 *)(size_t)(&OnbdModelInitV),                        NULL,                   (att_readwrite_callback_t)&PortBle_ReadData},    // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdModelName),                      (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdModelName),                          0,                      0   },
        // Offset: 065 - 068, ONBD_LOGGING
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdLoggingCharVal),                 (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdLoggingCharVal),                     0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(OnbdLoggingInitV),                   (u8 *)(size_t)(&uuid_logging_t),                    (u8 *)(size_t)(&OnbdLoggingInitV),                      (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_RDWR,   2,  sizeof(OnbdLoggingInitVCCC),                (u8 *)(size_t)(&clientCharacterCfgUUID),            (u8 *)(OnbdLoggingInitVCCC),                            (att_readwrite_callback_t)&portble_writeccc,                      0   },                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdLoggingName),                    (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdLoggingName),                        0,                      0   },
        // Offset: 069 - 071, ONBD_PRIVACY_ID_VECTOR
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdPrivacyIdVectorCharVal),         (u8 *)(size_t)(&my_characterUUID),                  (u8 *)(size_t)(OnbdPrivacyIdVectorCharVal),             0,                      0   },                  // prop
        {0,     ATT_PERMISSIONS_WRITE,  16, sizeof(OnbdPrivacyIdVectorInitV),           (u8 *)(size_t)(&uuid_privacy_id_vector_t),          (u8 *)(size_t)(&OnbdPrivacyIdVectorInitV),              (att_readwrite_callback_t)&PortBle_writeData,     NULL},                  // value
        {0,     ATT_PERMISSIONS_READ,   2,  sizeof(OnbdPrivacyIdVectorName),            (u8 *)(size_t)(&userdesc_UUID),                     (u8 *)(size_t)(OnbdPrivacyIdVectorName),                0,                      0   },
};


void debug_att_table(void)
{
     tlkapi_printf(APP_LOG_EN, "[APP][TBL] ATT  %d,%d,%d", SMART_ONBOARDING_SERVICE, SMART_CONTROL_SERVICE,SMART_AUTH_SERVICE,sizeof(my_Attributes)/sizeof(attribute_t));
}
/**
 * @brief   GATT initialization.
 *          !!!Note: this function is used to register ATT table to BLE Stack.
 * @param   none.
 * @return  none.
 */
void my_gatt_init(void)
{
    debug_att_table();
#if 0
    u8 att_size = ARRAY_SIZE(my_Attributes);
    u8 att_head_size = my_Attributes[0].attNum;
    printf("\natt_size:%d %d\n", att_size, att_head_size);
#endif
    bls_att_setAttributeTable((u8 *)(size_t)my_Attributes);
}
