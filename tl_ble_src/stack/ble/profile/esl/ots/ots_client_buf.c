/********************************************************************************************************
 * @file    ots_client_buf.c
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

const u16 gAppOtscObjectNameMaxSize   = OTS_CLIENT_OBJECT_NAME_MAX_SIZE;
const u16 gAppOtscObjectTypeMaxSize   = OTS_CLIENT_OBJECT_TYPE_MAX_SIZE;
const u16 gAppOtscObjectFilterMaxSize = OTS_CLIENT_OBJECT_FILTER_MAX_SIZE;

_attribute_ble_data_retention_ blc_otsc_t              gOtsClient[STACK_PRF_ACL_CONN_MAX_NUM];
_attribute_ble_data_retention_ otsClientObjectName_t   gOtsObjectName[STACK_PRF_ACL_CONN_MAX_NUM];
_attribute_ble_data_retention_ otsClientObjectName_t   gOtsObjectNameWrBuf[STACK_PRF_ACL_CONN_MAX_NUM];
_attribute_ble_data_retention_ otsClientObjectType_t   gOtsObjectType[STACK_PRF_ACL_CONN_MAX_NUM];
_attribute_ble_data_retention_ otsClientObjectFilter_t gOtsObjectFilter[STACK_PRF_ACL_CONN_MAX_NUM][OTS_OBJECT_FILTER_CHAR_NUM];
_attribute_ble_data_retention_ otsClientObjectFilter_t gOtsObjectFilterWrBuf[STACK_PRF_ACL_CONN_MAX_NUM][OTS_OBJECT_FILTER_CHAR_NUM];

blc_otsc_t *blt_otsc_getClientBuf(u8 index)
{
#if (STACK_PRF_ACL_CONN_MAX_NUM > 0)
    return index >= STACK_PRF_ACL_CONN_MAX_NUM ? NULL : &gOtsClient[index];
#else
    (void)index;

    return NULL;
#endif
}

void blt_otsc_cleanBuf(void)
{
    memset(gOtsClient, 0, sizeof(gOtsClient));
}

otsClientCharValue_t *blc_otsc_getObjectNameBuf(u8 aclIdx)
{
#if (STACK_PRF_ACL_CONN_MAX_NUM > 0)
    return aclIdx >= STACK_PRF_ACL_CONN_MAX_NUM ? NULL : (otsClientCharValue_t *)&gOtsObjectName[aclIdx];
#else
    (void)aclIdx;

    return NULL;
#endif
}

otsClientCharValue_t *blc_otsc_getObjectTypeBuf(u8 aclIdx)
{
#if (STACK_PRF_ACL_CONN_MAX_NUM > 0)
    return aclIdx >= STACK_PRF_ACL_CONN_MAX_NUM ? NULL : (otsClientCharValue_t *)&gOtsObjectType[aclIdx];
#else
    (void)aclIdx;

    return NULL;
#endif
}

otsClientCharValue_t *blc_otsc_getObjectFilterBuf(u8 aclIdx, u8 idx)
{
#if (STACK_PRF_ACL_CONN_MAX_NUM > 0)
    return aclIdx >= STACK_PRF_ACL_CONN_MAX_NUM                                ? NULL :
           idx >= sizeof(gOtsObjectFilter[0]) / sizeof(gOtsObjectFilter[0][0]) ? NULL :
                                                                                 (otsClientCharValue_t *)&gOtsObjectFilter[aclIdx][idx];
#else
    (void)aclIdx;
    (void)idx;

    return NULL;
#endif
}

otsClientCharValue_t *blc_otsc_getObjectNameWrBuf(u8 aclIdx)
{
#if (STACK_PRF_ACL_CONN_MAX_NUM > 0)
    return aclIdx >= STACK_PRF_ACL_CONN_MAX_NUM ? NULL : (otsClientCharValue_t *)&gOtsObjectNameWrBuf[aclIdx];
#else
    (void)aclIdx;

    return NULL;
#endif
}

otsClientCharValue_t *blc_otsc_getObjectFilterWrBuf(u8 aclIdx, u8 idx)
{
#if (STACK_PRF_ACL_CONN_MAX_NUM > 0)
    return aclIdx >= STACK_PRF_ACL_CONN_MAX_NUM                                          ? NULL :
           idx >= sizeof(gOtsObjectFilterWrBuf[0]) / sizeof(gOtsObjectFilterWrBuf[0][0]) ? NULL :
                                                                                           (otsClientCharValue_t *)&gOtsObjectFilterWrBuf[aclIdx][idx];
#else
    (void)aclIdx;
    (void)idx;

    return NULL;
#endif
}
