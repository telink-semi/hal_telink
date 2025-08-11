/********************************************************************************************************
 * @file    cs_test_cmd.h
 *
 * @brief   This is the header file for BLE SDK
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
#ifndef STACK_BLE_CONTROLLER_LL_CHN_SOUND_CS_TEST_CMD_CS_TEST_CMD_H_
#define STACK_BLE_CONTROLLER_LL_CHN_SOUND_CS_TEST_CMD_CS_TEST_CMD_H_

#include "stack/ble/ble_format.h"

typedef struct __attribute__((packed))
{
    u8 channel_length;
    u8 channel[0];
} overConfig_bit0_is_set_t;

typedef struct __attribute__((packed))
{
    u8 channel_map[10];
    u8 channel_selection_type;
    u8 ch3c_shape;
    u8 ch3c_jump;
} overConfig_bit0_not_set_t;

typedef struct __attribute__((packed))
{
    u8 main_mode_steps;
} overConfig_bit2_is_set_t;

typedef struct __attribute__((packed))
{
    u8 t_pm_tone_ext;
} overConfig_bit3_is_set_t;

typedef struct __attribute__((packed))
{
    u8 tone_antenna_permutation;
} overConfig_bit4_is_set_t;

typedef struct __attribute__((packed))
{
    u32 cs_sync_AA_init;
    u32 cs_sync_AA_refl;
} overConfig_bit5_is_set_t;

typedef struct __attribute__((packed))
{
    u8 ss_marker1_pos;
    u8 ss_marker2_pos;
} overConfig_bit6_is_set_t;

typedef struct __attribute__((packed))
{
    u8 ss_marker_value;
} overConfig_bit7_is_set_t;

typedef struct __attribute__((packed))
{
    u8 cs_sync_payload_pattern;
    u8 cs_sync_user_payload[16];
} overConfig_bit8_is_set_t;

enum
{
    SS_Partern_0011           = 0x00,
    SS_Partern_1100           = 0x01,
    SS_Partern_0011_1100_Loop = 0x02,
};

enum
{
    CS_SYNC_PRBS9           = 0x00,
    CS_SYNC_Repeat_11110000 = 0x01,
    CS_SYNC_Repeat_10101010 = 0x02,
    CS_SYNC_PRBS15          = 0x03,
    CS_SYNC_Repeat_11111111 = 0x04,
    CS_SYNC_Repeat_00000000 = 0x05,
    CS_SYNC_Repeat_00001111 = 0x06,
    CS_SYNC_Repeat_01010101 = 0x07,
    CS_SYNC_USER_PAYLOAD    = 0x80,
};

enum
{
    SS_TASK_NO_SYNC_PAYLOAD = 0x01,
    RS_TASK_NO_SYNC_PAYLOAD = 0x02,
    SS_RS_TASK_SYNC_PAYLOAD = 0x03,
};

ble_sts_t blc_hci_le_cs_startCsTest(hci_le_cs_test_cmdParam_t *cmdPara);

ble_sts_t blc_hci_le_cs_endCsTest(void);


#endif /* STACK_BLE_CONTROLLER_LL_CHN_SOUND_CS_TEST_CMD_CS_TEST_CMD_H_ */
