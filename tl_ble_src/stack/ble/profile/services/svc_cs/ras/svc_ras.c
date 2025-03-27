/********************************************************************************************************
 * @file    svc_ras.c
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
#include "stack/ble/ble.h"

#define RAS_START_HDL                               SERVICE_TELINK_RAS_HDL
static const svc_ras_feature_t  rasFeatureValue ={
        .realTimeProcedureDataSupport               = 0,
        .getLostProcedureDataSegmentsSupport        = 1,
        .abortOperationSupport                      = 0,
        .filterProcedureDataSupport                 = 0,
        .pctPahseFormatSupport                      = 0,
};
static const u16 rasFeatureValueLen = sizeof(svc_ras_feature_t);

static const atts_attribute_t rasList[] =
{
    ATTS_PRIMARY_SERVICE(serviceRangingUuid),
    ATTS_CHAR_UUID_READ_ENTITY_NOCB(charPropRead, characteristicRasFeatureUuid, rasFeatureValue),

    ATTS_CHAR_UUID_NOTIF_ONLY(characteristicLiveRangingDataUuid),
    ATTS_COMMON_CCC_DEFINE,

    ATTS_CHAR_UUID_NOTIF_ONLY(characteristicStoredRangingDataUuid),
    ATTS_COMMON_CCC_DEFINE,

    ATTS_CHAR_UUID_ENCR_WRITE_NULL(charPropWriteIndicate, characteristicControlPointUuid),
    ATTS_COMMON_CCC_DEFINE,

    ATTS_CHAR_UUID_NOTIF_ONLY(characteristicRangingDataReadyUuid),
    ATTS_COMMON_CCC_DEFINE,

    ATTS_CHAR_UUID_NOTIF_ONLY(characteristicRangingDataOverwrittenUuid),
    ATTS_COMMON_CCC_DEFINE,

};

/* GAP group structure */
_attribute_ble_data_retention_ static atts_group_t svcRasGroup =
{
    NULL,
    rasList,
    NULL,
    NULL,
    RAS_START_HDL,
    0
};

void blc_svc_addRasGroup(void)
{
    svcRasGroup.endHandle = svcRasGroup.startHandle+ARRAY_SIZE(rasList)-1;
    blc_gatts_addAttributeServiceGroup(&svcRasGroup);
}

void blc_svc_removeRasGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(RAS_START_HDL);
}

void blc_svc_rasCbackRegister(atts_w_cb_t writeCback)
{
    svcRasGroup.writeCback = writeCback;
}

