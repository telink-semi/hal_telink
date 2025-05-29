/********************************************************************************************************
 * @file    ext_adv_set.c
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

#include "app_config.h"
#include "app.h"


#if (MAIN_NODE_ROLE_SELECT == MAIN_NODE_PERIPHERAL)


const u8 tbl_advData_0[] = {
    15,
    DT_COMPLETE_LOCAL_NAME,
    'P',
    'e',
    'r',
    'i',
    'p',
    'h',
    'e',
    'r',
    'a',
    'l',
    'D',
    'e',
    'm',
    'o',
    2,
    DT_FLAGS,
    0x05,
    3,
    DT_APPEARANCE,
    0x80,
    0x01,
};

const u8 tbl_scanRsp_0[] = {
    15,
    DT_COMPLETE_LOCAL_NAME,
    'P',
    'e',
    'r',
    'i',
    'p',
    'h',
    'e',
    'r',
    'a',
    'l',
    'D',
    'e',
    'm',
    'o',
};


const u8 tbl_advData_1[] = {
    15,
    DT_COMPLETE_LOCAL_NAME,
    'P',
    'e',
    'r',
    'i',
    'p',
    'h',
    'e',
    'r',
    'a',
    'l',
    'D',
    'e',
    'm',
    'o',
    2,
    DT_FLAGS,
    0x05,
    3,
    DT_APPEARANCE,
    0x80,
    0x01,
};

const u8 tbl_scanRsp_1[] = {
    15,
    DT_COMPLETE_LOCAL_NAME,
    'P',
    'e',
    'r',
    'i',
    'p',
    'h',
    'e',
    'r',
    'a',
    'l',
    'D',
    'e',
    'm',
    'o',
};

    #define APP_EXT_ADV_SETS_NUMBER     2  //user set value
    #define APP_EXT_ADV_DATA_LENGTH     31 //2048//1664//1024   //user set value
    #define APP_EXT_SCANRSP_DATA_LENGTH 31 //2048//1664//1024   //user set value

_attribute_iram_bss_ u8 app_extAdvSetParam_buf[ADV_SET_PARAM_LENGTH * APP_EXT_ADV_SETS_NUMBER];

_attribute_iram_noinit_data_ u8 app_extAdvData_buf[APP_EXT_ADV_DATA_LENGTH * APP_EXT_ADV_SETS_NUMBER];

_attribute_iram_noinit_data_ u8 app_extScanRspData_buf[APP_EXT_SCANRSP_DATA_LENGTH * APP_EXT_ADV_SETS_NUMBER];

void app_multiple_adv_set_register_buffer(void)
{
    blc_ll_initExtendedAdvModule_initExtendedAdvSetParamBuffer(app_extAdvSetParam_buf, APP_EXT_ADV_SETS_NUMBER);
    blc_ll_initExtendedAdvDataBuffer(app_extAdvData_buf, APP_EXT_ADV_DATA_LENGTH);
    blc_ll_initExtendedScanRspDataBuffer(app_extScanRspData_buf, APP_EXT_SCANRSP_DATA_LENGTH);
}

void app_multiple_adv_set(void)
{
    blc_ll_initChannelSelectionAlgorithm_2_feature();
    blc_ll_setDefaultConnCodingIndication(CODED_PHY_PREFER_S8);
    tlkapi_printf(APP_LOG_EN, "adv_set 1: Extended, Connectable_scannable 1M\n");
    blc_ll_setExtAdvParam(ADV_HANDLE0, ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED, ADV_INTERVAL_100MS, ADV_INTERVAL_100MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_10dBm, BLE_PHY_1M, 0, BLE_PHY_2M, ADV_SID_0, 0);

    blc_ll_setExtAdvData(ADV_HANDLE0, sizeof(tbl_advData_0), tbl_advData_0);
    blc_ll_setExtScanRspData(ADV_HANDLE0, sizeof(tbl_scanRsp_0), tbl_scanRsp_0);
    tlkapi_printf(APP_LOG_EN, "adv_set 2: Extended, Connectable_scannable coded S8\n");
    blc_ll_setExtAdvParam(ADV_HANDLE1, ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED, ADV_INTERVAL_100MS, ADV_INTERVAL_100MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_10dBm, BLE_PHY_CODED, 0, BLE_PHY_CODED, ADV_SID_1, 0);

    blc_ll_setExtAdvData(ADV_HANDLE1, sizeof(tbl_advData_1), tbl_advData_1);
    blc_ll_setExtScanRspData(ADV_HANDLE1, sizeof(tbl_scanRsp_1), tbl_scanRsp_1);


    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE1, 0, 0);
}

#endif
