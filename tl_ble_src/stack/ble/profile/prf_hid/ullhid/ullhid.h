/********************************************************************************************************
 * @file    ullhid.h
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

// ULL-HID: Ultra Low Latency Human Interface Device Service
// ULL-HIDC: Ultra Low Latency Human Interface Device Service Client.
// ULL-HIDS: Ultra Low Latency Human Interface Device Service

/******************************* ULL-HID Common Start **********************************************************************/
#if ((!defined(HOST_V2_ENABLE)))
#define ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT 8
#else
#define ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT 9
#endif
struct hybridModeUllReport
{
    u8 reportID;               //0 mean the end.

    struct
    {                          //additional_info
        u8 reportType     : 1; //0x00:Input,   0x01:Output
        u8 powerSavingCfm : 1;
        u8 repetition     : 1;
        u8 rfu            : 5;
    };
};

/*
 * ULL-HID Properties characteristic format.
 * Filed                            Data Type       Size(in octets)
 * Features                         boolean[8]      1
 * Available Report Intervals       boolean[16]     2
 * Hybrid Mode ULL Reports          uint16[0-16]    0 to 16
 */
#if ((!defined(HOST_V2_ENABLE)))
union ullhid_supp_report_intervals
{ // supported report intervals.

    struct
    {
        u16 interval_1ms    : 1; // Bit 0: 1ms
        u16 interval_2ms    : 1; // Bit 1: 2ms
        u16 interval_3ms    : 1; // Bit 2: 3ms
        u16 interval_4ms    : 1; // Bit 3: 4ms
        u16 interval_5ms    : 1; // Bit 4: 5ms
        u16 interval_1_25ms : 1; // Bit 5: 1.25ms
        u16 interval_2_5ms  : 1; // Bit 6: 2.5ms
        u16 interval_3_75ms : 1; // Bit 7: 3.75ms
        u16 intervalRFU     : 8; // Bit 8-15: RFU
    };

    u16 intervals;
};
#else
union ullhid_supp_report_intervals
{ // supported report intervals.

    struct
    {
        u16 interval_1ms    : 1; // Bit 0: 1ms
        u16 interval_2ms    : 1; // Bit 1: 2ms
        u16 interval_3ms    : 1; // Bit 2: 3ms
        u16 interval_4ms    : 1; // Bit 3: 4ms
        u16 interval_5ms    : 1; // Bit 4: 5ms
        u16 interval_1_25ms : 1; // Bit 5: 1.25ms
        u16 interval_2_5ms  : 1; // Bit 6: 2.5ms
        u16 interval_3_75ms : 1; // Bit 7: 3.75ms
        u16 interval_7_5ms  : 1; // Bit 8: 7.5ms
        u16 intervalRFU     : 7; // Bit 8-15: RFU
    };

    u16 intervals;
};
#endif
/*
 * Features Field
 */
#define ULLHID_PROPERTIES_HEAD_SIZE 5

struct blc_ullhid_properties_format
{
    struct
    { // Features
        u8 deviceModeChange : 1;
        u8 featureRFU       : 7;
    };
    union ullhid_supp_report_intervals suppInterval;
#if ((!defined(HOST_V2_ENABLE)))
    u8                                 deviceToHostMaxSduSize;
    u8                                 hostToDeviceMaxSduSize;
    struct hybridModeUllReport         hybridModeReport[ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT];
#else
    u8                                 deviceToHostMaxSduSize;
    u8 deviceToHostPerferSduSize;
    u8                                 hostToDeviceMaxSduSize;
    u8 hostToDevicePerferSduSize;
    struct hybridModeUllReport         hybridModeReport[ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT];
    u8                                 reportCount;
#endif
} __attribute__((packed));

/*
 * Supported Available Report Interval,
 * 1ms, 2ms, 3ms, 4ms, 5ms, 1.25ms, 2.5ms, 3.75ms
 */

/*
 * @brief       ULL-HID convert report interval bit value to interval_us.
 * @param[in]   range 0 to 7.
 * @return      report interval, unit us, 0xFFFFFFFF mean not support.
 */
u32 blc_ullhid_convertReportIntervalBit(u16 intervalBit);

/*
 * @brief       ULL-HID convert report interval_us to interval bit value.
 * @param[in]   report interval, unit us, Enumerated value 1000us, 2000us, 3000us, 4000us, 5000us, 1250us, 2500us, 3750us.
 * @return      report interval bit value range 0 to 7, 0xFF mean not support.
 */
u8 blc_ullhid_convertReportInterval(u32 interval);

#define ULL_HID_REPORT_TYPE_INPUT       0x00
#define ULL_HID_REPORT_TYPE_OUTPUT      0x01

#define CHECK_ULL_HID_REPORT_TYPE(type) ((type) == ULL_HID_REPORT_TYPE_INPUT || (type) == ULL_HID_REPORT_TYPE_OUTPUT)

/******************************* ULL-HID Common End **********************************************************************/


/******************************* ULL-HID Client Start **********************************************************************/

//ULL-HID Client Event ID
enum
{
    HID_EVT_ULLHIDC_START = HID_EVT_TYPE_ULLHID_CLIENT,
    ULLHIDC_EVT_SELECT_HYBRID_MODE,  //refer to struct ullhidc_selectHybridModeEvt
    ULLHIDC_EVT_SELECT_DEFAULT_MODE, //refer to NULL.
};
#if ((!defined(HOST_V2_ENABLE)))
struct ullhidc_selectHybridModeEvt
{                                              //Event ID: ULLHIDS_EVT_SELECT_HYBRID_MODE
    u32                        reportInterval; //unit 1us
    u8                         reportCount;    //range 0 to 8
    u8                         CIG_ID;
    u8                         CIS_ID;
    u8                         reportIndex[ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT];
    struct hybridModeUllReport reportInfo[ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT];
} __attribute__((packed));
#else
struct hybrid_mode_report_info
{
    u8 reportID;   //0 mean the end.
    u8 reportType; //0x00:Input,   0x01:Output
    u8 powerSavingCfm;
    u8 repetition;
};

struct ullhidc_selectHybridModeEvt
{                                                  //Event ID: ULLHIDS_EVT_SELECT_HYBRID_MODE
    u32                            reportInterval; //unit 1us
    u8 deviceToHostMaxSduSize;
    u8 hostToDeviceMaxSduSize;
    u8                             reportCount;    //range 0 to 8
    struct hybrid_mode_report_info reportInfo[ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT];
} __attribute__((packed));
#endif
enum
{
    ULLHID_CLIENT_ERROR_START = PRF_MODULE_ERROR,
    ULLHIDC_ERROR_NO_ULL_HID_PROPERTIES_HDL,
    ULLHIDC_ERROR_NO_LE_HID_OPERATION_MODE_HDL,
};

struct blc_ullhidc_regParam
{
};

/**
 * @brief       for user to register ultra low latency human interface device service control client module.
 * @param[in]   param - currently not used, fixed NULL.
 * @return      none.
 */
void blc_hid_registerULLHIDControlClient(const struct blc_ullhidc_regParam *param);

//ULL-HID Client Read Characteristic Value Operation API
int blc_ullhidc_readUllhidProperties(u16 connHandle, prf_read_cb_t readCb);

//ULL-HID Client Get Characteristic Value Operation API
int blc_ullhidc_getUllhidProperties(u16 connHandle, struct blc_ullhid_properties_format *properties, u16 *propertiesLen);

//ULL-HID Client Write Characteristic Value Operation API
int blc_ullhidc_writeLeHidOperationMode(u16 connHandle, u8 *operation, u16 operationLen, prf_write_cb_t writeCb);
#if ((!defined(HOST_V2_ENABLE)))
struct ullhid_select_hybrid_param
{
    u8                                 CIG_ID;
    u8                                 CIS_ID;
    union ullhid_supp_report_intervals suppInterval;
    u8                                 indicesCnt;
    u8                                 indices[ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT];
};
#else
struct hybrid_report_param {
    u8 index : 3;
    u8 rfu : 3;
    u8 cfmEnable : 1;
    u8 repetitionEnable : 1;
}__attribute__((packed));

struct ullhid_select_hybrid_param
{
    u8                                 CIG_ID;
    u8                                 CIS_ID;
    union ullhid_supp_report_intervals suppInterval;
    u8 deviceToHostMaxSduSize;
    u8 hostToDeviceMaxSduSize;
    u8                                 indicesCnt;
    struct hybrid_report_param         indices[ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT];
};
#endif
int blc_ullhidc_writeSelectHybridMode(u16 connHandle, struct ullhid_select_hybrid_param *param, prf_write_cb_t writeCb);
int blc_ullhidc_writeSelectDefaultMode(u16 connHandle, prf_write_cb_t writeCb);

//ULL-HID Client Get Characteristic Value Operation API
/******************************* ULL-HID Client End **********************************************************************/

/******************************* ULL-HID Server Start **********************************************************************/
//ULL-HID Server Event ID
enum
{
    HID_EVT_ULLHIDS_START = HID_EVT_TYPE_ULLHID_SERVER,
    ULLHIDS_EVT_SELECT_HYBRID_MODE,  //refer to struct ullhids_selectHybridModeEvt
    ULLHIDS_EVT_SELECT_DEFAULT_MODE, //refer to NULL.
};

struct blc_ullhids_regParam
{
    struct blc_ullhid_properties_format properties;
};

enum
{
    ULLHID_SERVER_ERROR_START = PRF_MODULE_ERROR,
    ULLHID_ERROR_DEVICE_ALREADY_MODE,
};
#if ((!defined(HOST_V2_ENABLE)))
struct ullhids_selectHybridModeEvt
{                                              //Event ID: ULLHIDS_EVT_SELECT_HYBRID_MODE
    u32                        reportInterval; //unit 1us
    u8                         reportCount;    //range 0 to 8
    u8                         CIG_ID;
    u8                         CIS_ID;
    struct hybridModeUllReport reportInfo[ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT];
};
#else
struct ullhids_selectHybridModeEvt
{                                                  //Event ID: ULLHIDS_EVT_SELECT_HYBRID_MODE
    u32                            reportInterval; //unit 1us
    u8                             reportCount;    //range 0 to 8
    u8                             CIG_ID;
    u8                             CIS_ID;
    struct hybrid_mode_report_info reportInfo[ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT];
};
#endif
/**
 * @brief       for user to register ultra low latency human interface device service control server module.
 * @param[in]   param - server initial parameter.
 * @return      none.
 */
void blc_hid_registerULLHIDControlServer(const struct blc_ullhids_regParam *param);

//ULL-HID Server Get Characteristic Value Operation API

//ULL-HID Server Update Characteristic Value Operation API
int blc_ullhids_indSelectHybridMode(u16 connHandle, struct ullhid_select_hybrid_param *param, prf_ind_cb_t cb);
int blc_ullhids_indSelectDefaultMode(u16 connHandle, prf_ind_cb_t cb);

//ULL-HID Server change mode.
void blc_ullhids_setHybridMode(void);

void blc_ullhids_setDefaultMode(void);

/******************************* ULL-HID Server End **********************************************************************/
