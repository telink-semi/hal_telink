/********************************************************************************************************
 * @file    ullhid_internal.h
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
#pragma once

#if 0
enum {
    ULL_HID_ERR_OPCODE_OUTSIDE_RANGE = 0x81,
    ULL_HID_ERR_DEVICE_ALREADY_IN_REQUESTED_STATE = 0x82,
    ULL_HID_ERR_UNSUPPORTED_FEATURE = 0x83,
};
#else
enum {
    ULL_HID_ERR_OPCODE_OUTSIDE_RANGE = ATT_ERR_VALUE_NOT_ALLOWED,
    ULL_HID_ERR_DEVICE_ALREADY_IN_REQUESTED_STATE = ATT_ERR_VALUE_NOT_ALLOWED,
    ULL_HID_ERR_UNSUPPORTED_FEATURE = ATT_ERR_VALUE_NOT_ALLOWED,
};
#endif

enum {
    ULL_HID_REPORT_INTERVAL_1MS = 0x00,
    ULL_HID_REPORT_INTERVAL_2MS,
    ULL_HID_REPORT_INTERVAL_3MS,
    ULL_HID_REPORT_INTERVAL_4MS,
    ULL_HID_REPORT_INTERVAL_5MS,
    ULL_HID_REPORT_INTERVAL_1_25MS,
    ULL_HID_REPORT_INTERVAL_2_5MS,
    ULL_HID_REPORT_INTERVAL_3_75MS,
    ULL_HID_REPORT_INTERVAL_MAX,
};
#define CHECK_ULL_HID_REPORT_INTERVAL(interval)     ((interval) <= (ULL_HID_REPORT_INTERVAL_MAX - 1))

enum blt_ullhids_opcode_enum{
    ULL_HID_OPCODE_SELECT_HYBRID_MODE       = 0x01,
    ULL_HID_OPCODE_SELECT_DEFAULT_MODE,
};

struct ullhid_operation_mode_pdu_format{
    u8 opcode;  //blt_ullhids_opcode_enum
    u8 parameters[0];
};

#define ULLHID_SELECT_HYBRID_MODE_HEAD_SIZE         5

struct ullhid_select_hybrid_mode_format{
    u8 opcode;  //blt_ullhids_opcode_enum
    u8 CIG_ID;
    u8 CIS_ID;
    union ullhid_supp_report_intervals suppInterval;
    u8 indices[ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT];
}__attribute__((packed));

struct ullhid_select_default_mode_format{
    u8 opcode;  //blt_ullhids_opcode_enum
};

enum ullhid_server_mode{
    ULLHIDS_DEFAULT_MODE = 0,
    ULLHIDS_HYBRID_MODE,
};

/*
 * ULL-HID: ATT handle information: 7byte
 */
struct blt_ullhid_att_hdl{
    u16 baseHandle;
    u8 endHdl;
    u8 propertiesHdl;       //
    u8 operationHdl;    //Indicate
}__attribute__((packed));

struct blt_ullhid_nv_info{
    struct blt_ullhid_att_hdl att;
};

att_err_t blt_ullhid_checkSelectHybridMode(struct blc_ullhid_properties_format *properties, u16 propertiesLen,
        union ullhid_supp_report_intervals reportInterval, u8 *indices, u8 indicesSize);

#define BLT_ULLHID_LOG(fmt, ...)                    BLC_BASIC_PRF_LOG(DBG_PRF_MASK_ULL_HID_LOG, "[ULL]"fmt, ##__VA_ARGS__)


#define ULLHID_MALLOC(size)             malloc_nonreten((size))
#define ULLHID_FREE(ptr)                free_nonreten(ptr)

//Client
#define BLT_ULLHID_DISCOVERY_READ_CHAR(uuid, characteristic)        BLT_PRF_DISCOVERY_READ_CHAR(ullhid, uuid, characteristic)
#define BLT_ULLHID_DISCOVERY_IND_CHAR(uuid, characteristic)         BLT_PRF_DISCOVERY_IND_CHAR(ullhid, uuid, characteristic)
#define BLT_DEFINE_ULLHID_DISCOVERY_FOUND_CHAR(characteristic)      BLT_DEFINE_PRF_DISCOVERY_FOUND_CHAR(ullhid, ULLHID, characteristic)
#define BLT_DEFINE_ULLHID_DISCOVERY_START_READ(characteristic)      BLT_DEFINE_PRF_DISCOVERY_START_READ_WITH_LEN(ullhid, ULLHID, characteristic)
#define BLT_ULLHID_RECONNECT_GET_INFO_READ(characteristic)          BLT_DEFINE_PRF_RECONNECT_GET_INFO(ullhid, CHAR_PROP_READ, characteristic)
#define BLT_ULLHID_RECONNECT_CHAR(characteristic)                   BLT_PRF_RECONNECT_READ_CHAR(ullhid, characteristic)

#define BLT_ULLHID_READ_ATTR_VALUE(charName)                        BLT_PRF_READ_ATTR_VALUE_WITH_LEN(ullhid, ULLHID, charName##Hdl, charName, charName##Len)
#define BLT_ULLHID_WRITE_ATTR_VALUE_WITH_LEN(charName)              BLT_PRF_WRITE_ATTR_VALUE_WITH_LEN(ullhid, ULLHID, charName##Hdl, charName, charName##Len)
#define BLT_ULLHID_GET_ATTR_VALUE(characteristic)                   BLT_PRF_GET_ATTR_VALUE_WITH_LEN(ullhid, characteristic)


///server
#define BLT_ULLHID_SERVER_INIT_HANDLE(characteristic)           BLT_PRF_SERVER_INIT_HANDLE(ullhid, ULLHID, characteristic)
#define BLT_ULLHID_SERVER_FIND_CHAR(characteristic, uuid)       BLT_PRF_SERVER_FIND_CHAR(ullhid, characteristic, uuid)
