/********************************************************************************************************
 * @file    esls_server_buf.c
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

const blc_esls_displayData_t defaultDisplayData[] = {
    {
        .displayType = BLC_ESLS_DISPLAY_TYPE_BLACK_WHITE,
        .height      = 128,
        .width       = 128,
    },
};

const blc_esls_sensorInformation_t defaultSensorInfo[] = {
    {
        .size        = BLC_ESLS_SENSOR_INFORMATION_SIZE_0,
        .sensorType0 = 1,
    },
};

const blc_esls_ledInformation_t defaultLedInfo[] = {
    {
        .type  = BLC_ESLS_LED_INFORMATION_MONOCHROME,
        .blue  = 0,
        .green = 0,
        .red   = 0,
    },
};

const blc_eslss_regParam_t defaultEslpsParam = {
    .displayDataNum        = ARRAY_SIZE(defaultDisplayData),
    .displayData           = defaultDisplayData,
    .ledInformationsNum    = ARRAY_SIZE(defaultLedInfo),
    .ledInfo               = defaultLedInfo,
    .sensorInformationsNum = ARRAY_SIZE(defaultSensorInfo),
    .sensorInfo            = defaultSensorInfo,
    .maxImageIndex         = 0,
};
