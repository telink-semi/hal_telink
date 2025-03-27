/********************************************************************************************************
 * @file    hid_internal.h
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



enum{
    HID_CONTROL_POINT_ENTER_SUSPEND = 0x00,
    HID_CONTROL_POINT_EXIT_SUSPEND,
};

/*
 * HID: ATT handle information: 7byte
 */
struct blt_hid_att_hdl{
    u16 baseHandle;
    u8 endHdl;
    u8 protocolModeHdl;
    u8 bootKeyboardInputReportHdl;
    u8 bootKeyboardOutputReportHdl;
    u8 bootMouseInputReportHdl;
    u8 HIDInformationHdl;
    u8 HIDControlPointHdl;
    u8 reportMapHdl;
}__attribute__((packed));

struct blt_hid_nv_info{
    struct blt_hid_att_hdl att;
    u8 protocolModeProp;
    u8 bootKeyboardInputReportProp;
    u8 bootKeyboardOutputReportProp;
    u8 bootMouseInputReportProp;
    u8 HIDInformationProp;
    u8 HIDControlPointProp;
    u8 reportMapProp;
    struct __attribute__((packed)){
        u8 attrHandle;
        u8 properties;
        u8 cccHandle;
        u8 reportReferenceHandle;
        struct hid_reportReferenceValue reportReferenceValue;
    }reportCharInfo[HID_SUPPORT_REPORT_HANDLE_MAX];
    u8 reportCount;
}__attribute__((packed));

#define BLT_HID_LOG(fmt, ...)                   BLC_BASIC_PRF_LOG(DBG_PRF_MASK_HID_LOG, "[HID]"fmt, ##__VA_ARGS__)

#define HID_MALLOC(size)                        malloc_nonreten((size))
#define HID_FREE(ptr)                           free_nonreten(ptr)

#define BLT_DEFINE_HID_DISCOVERY_FOUND_CHAR(characteristic)             BLT_DEFINE_PRF_DISCOVERY_FOUND_CHAR_PROP(hid, HID, characteristic)
#define BLT_DEFINE_HID_DISCOVERY_START_READ(characteristic)             BLT_DEFINE_PRF_DISCOVERY_START_READ(hid, HID, characteristic)
#define BLT_DEFINE_HID_DISCOVERY_START_READ_FIX_LEN(characteristic)     BLT_DEFINE_PRF_DISCOVERY_START_READ_FIX_LEN(hid, HID, characteristic)


#define BLT_HID_RECONNECT_GET_INFO_READ(characteristic)                 BLT_DEFINE_PRF_RECONNECT_GET_INFO(hid, CHAR_PROP_READ, characteristic)
#define BLT_HID_RECONNECT_CHAR(characteristic)                          BLT_PRF_RECONNECT_READ_CHAR(hid, characteristic)

#define BLT_HID_DISCOVERY_WRITE_CHAR(uuid, characteristic)              BLT_PRF_DISCOVERY_WRITE_CHAR(hid, uuid, characteristic)
#define BLT_HID_DISCOVERY_READ_NOTIFY_CHAR(uuid, characteristic)        BLT_PRF_DISCOVERY_READ_NOTIFY_CHAR(hid, uuid, characteristic)
#define BLT_HID_DISCOVERY_READ_CHAR(uuid, characteristic)               BLT_PRF_DISCOVERY_READ_CHAR(hid, uuid, characteristic)


#define BLT_HID_SERVER_INIT_HANDLE(characteristic)                      BLT_PRF_SERVER_INIT_HANDLE(hid, HID, characteristic)
#define BLT_HID_SERVER_FIND_CHAR(characteristic, uuid)                  BLT_PRF_SERVER_FIND_CHAR(hid, characteristic, uuid)


#define BLT_HID_READ_ATTR_VALUE(charName)                               BLT_PRF_READ_ATTR_VALUE(hid, HID, charName##Hdl, charName, charName##Len)
#define BLT_HID_READ_ATTR_VALUE_FIX_LEN(charName)                       BLT_PRF_READ_ATTR_VALUE_FIX_LEN(hid, HID, charName##Hdl, charName)

#define BLT_HID_GET_ATTR_VALUE(characteristic)                          BLT_PRF_GET_ATTR_VALUE(hid, characteristic)
#define BLT_HID_GET_ATTR_VALUE_FIX_LEN(characteristic)                  BLT_PRF_GET_ATTR_VALUE_FIX_LEN(hid, characteristic)


#define BLT_HID_WRITE_ATTR_VALUE_FIX_LEN(charName)                      BLT_PRF_WRITE_ATTR_VALUE_FIX_LEN(hid, HID, charName##Hdl, charName)
#define BLT_HID_WRITE_ATTR_VALUE_WITHOUT_RSP_FIX_LEN(charName)          BLT_PRF_WRITE_ATTR_VALUE_WITHOUT_RSP_FIX_LEN(hid, HID, charName##Hdl, charName)


