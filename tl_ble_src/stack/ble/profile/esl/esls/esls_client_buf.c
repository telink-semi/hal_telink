/********************************************************************************************************
 * @file    esls_client_buf.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    10,2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

const u16 gAppEslscLedInformationMaxSize     = ESLS_CLIENT_LED_INFORMATION_MAX_SIZE;
const u16 gAppEslscSensorInformationMaxSize  = ESLS_CLIENT_SENSOR_INFORMATION_MAX_SIZE;
const u16 gAppEslscDisplayInformationMaxSize = ESLS_CLIENT_DISPLAY_INFORMATION_MAX_SIZE;

_attribute_ble_data_retention_ /* retention TODO: */
    eslClientLEDInformation_t gEslsClientLedInformation[STACK_PRF_ACL_CONN_MAX_NUM];

_attribute_ble_data_retention_ /* retention TODO: */
    eslClientSensorInformation_t gEslsClientSensorInformation[STACK_PRF_ACL_CONN_MAX_NUM];

_attribute_ble_data_retention_ /* retention TODO: */
    eslClientDisplayInformation_t gEslsClientDisplayInformation[STACK_PRF_ACL_CONN_MAX_NUM];

_attribute_ble_data_retention_ /* retention TODO: */
    blc_esls_client_t gEslsClient[STACK_PRF_ACL_CONN_MAX_NUM];

blc_esls_client_t *blc_eslsc_getClientBuf(u8 index)
{
#if (STACK_PRF_ACL_CONN_MAX_NUM > 0)
    return index >= STACK_PRF_ACL_CONN_MAX_NUM ? NULL : &gEslsClient[index];
#else
    (void)index;

    return NULL;
#endif
}

eslClientCharValue_t *blc_eslsc_getLedInformationBuf(u8 aclIdx)
{
#if (STACK_PRF_ACL_CONN_MAX_NUM > 0)
    return aclIdx >= STACK_PRF_ACL_CONN_MAX_NUM ? NULL : (eslClientCharValue_t *)&gEslsClientLedInformation[aclIdx];
#else
    (void)aclIdx;

    return NULL;
#endif
}

eslClientCharValue_t *blc_eslsc_getSensorInformationBuf(u8 aclIdx)
{
#if (STACK_PRF_ACL_CONN_MAX_NUM > 0)
    return aclIdx >= STACK_PRF_ACL_CONN_MAX_NUM ? NULL : (eslClientCharValue_t *)&gEslsClientSensorInformation[aclIdx];
#else
    (void)aclIdx;

    return NULL;
#endif
}

eslClientCharValue_t *blc_eslsc_getDisplayInformationBuf(u8 aclIdx)
{
#if (STACK_PRF_ACL_CONN_MAX_NUM > 0)
    return aclIdx >= STACK_PRF_ACL_CONN_MAX_NUM ? NULL : (eslClientCharValue_t *)&gEslsClientDisplayInformation[aclIdx];
#else
    (void)aclIdx;

    return NULL;
#endif
}
