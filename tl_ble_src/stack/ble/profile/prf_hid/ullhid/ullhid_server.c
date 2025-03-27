/********************************************************************************************************
 * @file    bas_server.c
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

#include "ullhid_internal.h"
#include "ullhid_server_buf.h"

static int blt_ullhids_init(u8 initType, const void* param);
static int blt_ullhids_connect(u16 connHandle, prf_acl_state_enum connState);
static void blt_ullhids_serviceInit(const struct blc_ullhids_regParam* param);
static void blt_ullhids_setUllHidProperties(const struct blc_ullhid_properties_format* properties);
static int blt_ullhids_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8* writeValue, u16 valueLen);

_attribute_ble_data_retention_
struct blc_ullhid_server_ctrl ullhid_server_ctrl = {
    .process = {
        .pNext = NULL,
        .id = ULLHID_SERVER,
        .usedAclRole = 0,
        .init = blt_ullhids_init,
        .connect = blt_ullhids_connect,
        .discov = NULL,
        .loop = NULL,
    },
};

/**
 * @brief       register ultra low latency HID server controller.
 * @param[in]   param: initial parameter.
 * @return      none.
 */
void blc_hid_registerULLHIDControlServer(const struct blc_ullhids_regParam *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t*)&ullhid_server_ctrl, param);
}

static int blt_ullhids_init(u8 initType, const void* param)
{
#if(0)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_ullhid_server)), blc_ullhid_server);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_ullhid_server_ctrl)), blc_ullhid_server_ctrl);
#endif

    if(initType == PRF_PROC_INIT) {
        BLT_ULLHID_LOG("Server init");
        blc_svc_addUllhidGroup();
        blc_svc_ullhidCbackRegister(blt_ullhids_writeCback);
        blt_ullhids_serviceInit(param);
        blc_ullhids_setDefaultMode();
    }
//  else if (initType == PRF_PROC_DEINIT) {
//      blc_svc_removeUllhidGroup();
//      BLT_ULLHID_LOG("Server deinit");
//  }
    return 0;
}

static int blt_ullhids_connect(u16 connHandle, prf_acl_state_enum connState)
{
    (void)connHandle;
    if(connState == PRF_ACL_STATE_DISCONN) {
        blc_ullhids_setDefaultMode();
    }

    return 0;
}

static struct blc_ullhid_server* blt_ullhids_getCtrl(u16 connHandle)
{
    (void)connHandle;
    return &ullhid_server_ctrl.ullhidServer;
}

#define ULL_HID_PROPERTIES_HANDLE(connHandle)               (blt_ullhids_getCtrl(connHandle)->propertiesHdl)
#define LE_HID_OPERATION_MODE_HANDLE(connHandle)            (blt_ullhids_getCtrl(connHandle)->operationHdl)

/****************ULL-HID server init all characteristic handle***********************/
BLT_ULLHID_SERVER_INIT_HANDLE(properties)
BLT_ULLHID_SERVER_INIT_HANDLE(operation)

static const atts_findCharList_t ullhidsChar[] = {
    BLT_ULLHID_SERVER_FIND_CHAR(properties, characteristicUllHidPropertiesUuid),
    BLT_ULLHID_SERVER_FIND_CHAR(operation, characteristicLeHidOperationModeUuid),
};

const struct blc_ullhids_regParam defaultUllhidParam = {
    .properties = {
        .suppInterval.interval_5ms = 1,
    }
};

static void blt_ullhids_serviceInit(const struct blc_ullhids_regParam* param)
{
    struct blc_ullhid_server *server = blt_ullhids_getCtrl(PRF_RFU_CONN_HANDLE);
    blc_atts_findCharacteristicByServiceUuid(serviceUllhidUuid, ATT_16_UUID_LEN, ullhidsChar, ARRAY_SIZE(ullhidsChar), server);
    BLT_ULLHID_LOG("Handle information, ULL HID Properties:0x%x, LE HID Operation Mode:0x%x",   server->operationHdl, server->operationHdl);
    const struct blc_ullhids_regParam* ullhidParam = param;

    if(ullhidParam == NULL) { //use default parameters
        ullhidParam = &defaultUllhidParam;
    }
    blt_ullhids_setUllHidProperties(&ullhidParam->properties);
}

/****************ULL-HID server init all characteristic handle end***********************/
static void blt_ullhids_setUllHidProperties(const struct blc_ullhid_properties_format* properties)
{
    u8* pProperties = NULL;
    u16* pPropertiesLen = NULL;
    blc_gatts_getAttributeInformationByHandle(PRF_RFU_CONN_HANDLE, ULL_HID_PROPERTIES_HANDLE(PRF_RFU_CONN_HANDLE), &pProperties, &pPropertiesLen);

    if(!pProperties || !pPropertiesLen) return ;

    int hybridModeSize = 0;

    for(; hybridModeSize<ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT; hybridModeSize++)
    {
        if(!CHECK_ULL_HID_REPORT_TYPE(properties->hybridModeReport[hybridModeSize].reportType)) {
            break;
        }

        if(properties->hybridModeReport[hybridModeSize].rfu != 0) {
            break;
        }

        if(properties->hybridModeReport[hybridModeSize].reportID == 0) {
            break;
        }
    }

    *pPropertiesLen = ULLHID_PROPERTIES_HEAD_SIZE + 2*hybridModeSize;
    memcpy(pProperties, properties, *pPropertiesLen);

    ((struct blc_ullhid_properties_format *)pProperties)->featureRFU = 0;
    ((struct blc_ullhid_properties_format *)pProperties)->suppInterval.intervalRFU = 0;
}

static att_err_t blt_ullhids_checkLeHidOperationModeParam(u8 mode, u8 *pData, u8 length)
{
    if(pData == NULL || length < sizeof(struct ullhid_operation_mode_pdu_format))
    {
        return ATT_ERR_VALUE_NOT_ALLOWED;
    }

    struct ullhid_operation_mode_pdu_format* operation = (struct ullhid_operation_mode_pdu_format*)pData;
    int err = ATT_SUCCESS;

    if(operation->opcode == ULL_HID_OPCODE_SELECT_HYBRID_MODE)
    {
        if (mode == ULLHIDS_HYBRID_MODE){
            err = ULL_HID_ERR_DEVICE_ALREADY_IN_REQUESTED_STATE;
        }

        //select hybrid mode field, opcode(1B)+CIG ID(1B)+CIS ID(1B)+Interval(2B)+n*(indices(1B)), 8+n
        if( length > sizeof(struct ullhid_select_hybrid_mode_format) || length <= ULLHID_SELECT_HYBRID_MODE_HEAD_SIZE) {
            err = ATT_ERR_VALUE_NOT_ALLOWED;
        }

    }
    else if(operation->opcode == ULL_HID_OPCODE_SELECT_DEFAULT_MODE)
    {
        if (mode == ULLHIDS_DEFAULT_MODE)
        {
            err = ULL_HID_ERR_DEVICE_ALREADY_IN_REQUESTED_STATE;
        }

        if(length != sizeof(struct ullhid_select_default_mode_format))
        {
            err = ATT_ERR_VALUE_NOT_ALLOWED;
        }
    }
    else
    {
        err = ULL_HID_ERR_OPCODE_OUTSIDE_RANGE;
    }

    return err;
}

static const u32 reportIntervalMap[] = {
    1000, 2000, 3000, 4000, 5000,
    1250, 2500, 3750,
};

u32 blc_ullhid_getReportIntervalBit(u16 intervalBit)
{
    for (int i=0; i<ARRAY_SIZE(reportIntervalMap); i++)
    {
        if(intervalBit & BIT(i))
        {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

u32 blc_ullhid_convertReportIntervalBit(u16 intervalBit)
{
    for (int i=0; i<ARRAY_SIZE(reportIntervalMap); i++)
    {
        if(intervalBit & BIT(i))
        {
            return reportIntervalMap[i];
        }
    }
    return 0xFFFFFFFF;
}

u8 blc_ullhid_convertReportInterval(u32 interval)
{
    for(size_t i=0; i<ARRAY_SIZE(reportIntervalMap); i++)
    {
        if(reportIntervalMap[i] == interval){
            return i;
        }
    }
    return 0xFF;
}

typedef att_err_t(*ullhids_dealOpcode)(u16 connHandle, u8*value, u16 valueLen);

att_err_t blt_ullhid_checkSelectHybridMode(struct blc_ullhid_properties_format *properties, u16 propertiesLen,
        union ullhid_supp_report_intervals reportInterval, u8 *indices, u8 indicesSize)
{
    if(reportInterval.intervalRFU != 0 || //Invalid report intervals, RFU is not zero.
            blt_calBit1Number_16bit(reportInterval.intervals) != 1 ||       // More than 1 bit is set to 1
            (reportInterval.intervals & properties->suppInterval.intervals) == 0 //Unsupported report interval.
            || indicesSize == 0 || indicesSize > ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT

    )
    {
        return ULL_HID_ERR_UNSUPPORTED_FEATURE;
    }

    u8 map = 0;

    for(int i=0; i<indicesSize; i++)
    {
        if(indices[i] >= ((propertiesLen-ULLHID_PROPERTIES_HEAD_SIZE)>>1))
        {
            return ULL_HID_ERR_UNSUPPORTED_FEATURE;
        }

        if(map & BIT(indices[i]))
        {
            return ULL_HID_ERR_UNSUPPORTED_FEATURE;
        }
        map |= BIT(indices[i]);
    }

    return ATT_SUCCESS;
}

static att_err_t blt_ullhids_checkSelectHybridMode(union ullhid_supp_report_intervals reportInterval, u8 *indices, u8 indicesSize)
{
    struct blc_ullhid_properties_format* pProperties = NULL;
    u16* pPropertiesLen = NULL;
    blc_gatts_getAttributeInformationByHandle(0xFFFF, ULL_HID_PROPERTIES_HANDLE(0xFFFF), (u8**)&pProperties, &pPropertiesLen);

    if(pPropertiesLen == NULL || pProperties == NULL || *pPropertiesLen <= ULLHID_PROPERTIES_HEAD_SIZE) return ATT_ERR_VALUE_NOT_ALLOWED;

    return blt_ullhid_checkSelectHybridMode(pProperties, *pPropertiesLen, reportInterval, indices, indicesSize);
}

static att_err_t blt_ullhids_recvSelectHybridMode(u16 connHandle, u8*value, u16 valueLen)
{
    u8 indicesSize = valueLen - ULLHID_SELECT_HYBRID_MODE_HEAD_SIZE;

    struct ullhid_select_hybrid_mode_format *pHybrid = (struct ullhid_select_hybrid_mode_format*)value;

    att_err_t err = blt_ullhids_checkSelectHybridMode(pHybrid->suppInterval, &pHybrid->indices[0], indicesSize);

    if(err != ATT_SUCCESS){
        return err;
    }

    struct blc_ullhid_properties_format* pProperties = NULL;
    u16* pPropertiesLen = NULL;
    blc_gatts_getAttributeInformationByHandle(0xFFFF, ULL_HID_PROPERTIES_HANDLE(0xFFFF), (u8**)&pProperties, &pPropertiesLen);

    struct ullhids_selectHybridModeEvt evt;

    evt.reportInterval = blc_ullhid_convertReportIntervalBit(pHybrid->suppInterval.intervals);
    evt.reportCount = indicesSize;
    evt.CIG_ID = pHybrid->CIG_ID;
    evt.CIS_ID = pHybrid->CIS_ID;
    for (int i=0; i<indicesSize; i++)
    {
        evt.reportInfo[i] = pProperties->hybridModeReport[pHybrid->indices[i]];
    }

    blt_prf_sendEvent(connHandle, ULLHIDS_EVT_SELECT_HYBRID_MODE, &evt, sizeof(struct ullhids_selectHybridModeEvt));

    return ATT_SUCCESS;
}

static att_err_t blt_ullhids_recvSelectDefaultMode(u16 connHandle, u8* value, u16 valueLen)
{
    (void)value;
    (void)valueLen;
    blt_prf_sendEvent(connHandle, ULLHIDS_EVT_SELECT_DEFAULT_MODE, NULL, 0);
    return ATT_SUCCESS;
}

static const ullhids_dealOpcode ullhidsFun[] = {
    NULL, blt_ullhids_recvSelectHybridMode, blt_ullhids_recvSelectDefaultMode,
};

static int blt_ullhids_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8* writeValue, u16 valueLen)
{
    (void)opcode;
    BLT_ULLHID_LOG("Write ConnHandle:0x%x, attrHandle:0x%x, value is %s", connHandle, attrHandle, hex_to_str(writeValue, valueLen));
    struct blc_ullhid_server *ullhids = blt_ullhids_getCtrl(connHandle);

    if(ullhids->operationHdl != attrHandle) {
        BLT_ULLHID_LOG("ERR: write attrHandle is 0x%x, correct handle is 0x%x", attrHandle, ullhids->operationHdl);
        return ATT_ERR_INVALID_HANDLE;
    }

    att_err_t err = blt_ullhids_checkLeHidOperationModeParam(ullhids->mode, writeValue, valueLen);

    if(err != ATT_SUCCESS)
    {
        return err;
    }

    opcode = writeValue[0];

    return ullhidsFun[opcode](connHandle, writeValue, valueLen);
}


int blc_ullhids_indSelectHybridMode(u16 connHandle, struct ullhid_select_hybrid_param *param, prf_ind_cb_t cb)
{
    if(blt_ullhids_checkSelectHybridMode(param->suppInterval, param->indices, param->indicesCnt) != ATT_SUCCESS)
    {
        return PRF_ERR_INVALID_PARAMETER;
    }

    struct blc_ullhid_server *ullhids = blt_ullhids_getCtrl(connHandle);

    if(ullhids->mode == ULLHIDS_HYBRID_MODE)
    {
        return ULLHID_ERROR_DEVICE_ALREADY_MODE;
    }

    struct ullhid_select_hybrid_mode_format mode = {
        .opcode = ULL_HID_OPCODE_SELECT_HYBRID_MODE,
        .CIG_ID = param->CIG_ID,
        .CIS_ID = param->CIS_ID,
        .suppInterval = param->suppInterval,
    };

    memcpy(mode.indices, param->indices, param->indicesCnt);

    gattsIndValue_t ind = {
        .connHandle = connHandle,
        .scid = L2CAP_CID_ATTR_PROTOCOL,
        .attrHandle = ullhids->operationHdl,
        .value = &mode,
        .valueLen = ULLHID_SELECT_HYBRID_MODE_HEAD_SIZE + param->indicesCnt,
        .cb = cb
    };
    return blc_gatts_indicateValue(&ind);
}

int blc_ullhids_indSelectDefaultMode(u16 connHandle, prf_ind_cb_t cb)
{
    struct blc_ullhid_server *ullhids = blt_ullhids_getCtrl(connHandle);

    if(ullhids->mode == ULLHIDS_DEFAULT_MODE)
    {
        return ULLHID_ERROR_DEVICE_ALREADY_MODE;
    }

    struct ullhid_select_default_mode_format mode = {
        .opcode = ULL_HID_OPCODE_SELECT_DEFAULT_MODE
    };

    gattsIndValue_t ind = {
        .connHandle = connHandle,
        .scid = L2CAP_CID_ATTR_PROTOCOL,
        .attrHandle = ullhids->operationHdl,
        .value = &mode,
        .valueLen = sizeof(struct ullhid_select_default_mode_format),
        .cb = cb
    };
    return blc_gatts_indicateValue(&ind);
}

void blc_ullhids_setHybridMode(void)
{
    struct blc_ullhid_server *ullhids = blt_ullhids_getCtrl(PRF_RFU_CONN_HANDLE);

    ullhids->mode = ULLHIDS_HYBRID_MODE;
}

void blc_ullhids_setDefaultMode(void)
{
    struct blc_ullhid_server *ullhids = blt_ullhids_getCtrl(PRF_RFU_CONN_HANDLE);

    ullhids->mode = ULLHIDS_DEFAULT_MODE;
}
