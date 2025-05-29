/********************************************************************************************************
 * @file    ullhid_client.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "ullhid_internal.h"
#include "ullhid_client_buf.h"
#include "stack/ble/host/gatt/tlk_malloc_stack.h"

static int blt_ullhidc_init(u8 initType, const void *param);
static int blt_ullhidc_connect(u16 connHandle, prf_acl_state_enum connState);
static int blt_ullhidc_discovery(u16 connHandle);
static int blt_ullhidc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param);

static void blt_ullhidc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

static const blc_gapc_discList_t discUllhid;
#define BLC_ULLHID_START_SDP(connHandle) blc_gapc_registerDiscoveryService(connHandle, &discUllhid)

static const blc_gapc_reconnList_t reconnUllhid;
#define BLC_ULLHID_START_RECONN(connHandle) blc_gapc_registerReconnectService(connHandle, &reconnUllhid)
#if ((!defined(HOST_V2_ENABLE)))
_attribute_ble_data_retention_ struct blc_ullhid_client_ctrl ullhid_client_ctrl = {
    .process = {
                .pNext       = NULL,
                .id          = ULLHID_CLIENT,
                .usedAclRole = 0,
                .init        = blt_ullhidc_init,
                .connect     = blt_ullhidc_connect,
                .discov      = blt_ullhidc_discovery,
                .loop        = NULL,
                .store       = blt_ullhidc_nv_store,
                },
};
#else
static const struct blc_prf_process_params s_ullhid_client_process_params = {
    .id          = ULLHID_CLIENT,
    .usedAclRole = PRF_GAP_ACL_CENTRAL,
    .init        = blt_ullhidc_init,
    .connect     = blt_ullhidc_connect,
    .discovery   = blt_ullhidc_discovery,
    .store       = blt_ullhidc_nv_store,
};

_attribute_ble_data_retention_ struct blc_ullhid_client_ctrl ullhid_client_ctrl = {
    .process = {
                .next       = SLIST_HEAD_INITIALIZER(),
                .prf_params = &s_ullhid_client_process_params,
                },
};
#endif

void blc_hid_registerULLHIDControlClient(const struct blc_ullhidc_regParam *param)
{
#if ((!defined(HOST_V2_ENABLE)))
    blc_prf_registerServiceModule(PRF_GAP_ACL_CENTRAL, (blc_prf_proc_t *)&ullhid_client_ctrl, param);
#else
    blc_prf_registerServiceModule((struct blc_prf_process *)&ullhid_client_ctrl, param);
#endif
}

static int blt_ullhidc_init(u8 initType, const void *param)
{
#if (0)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_ullhid_client)), blc_ullhid_client);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_ullhid_client_ctrl)), blc_ullhid_client_ctrl);
#endif
    (void)param;

    if (initType == PRF_PROC_INIT) {
        BLT_ULLHID_LOG("Client init");
    }
    //  else if (initType == PRF_PROC_DEINIT) {
    //      BLT_ULLHID_LOG("Client deinit");
    //  }
    return 0;
}

static struct blc_ullhid_client *blt_ullhidc_getClientInst(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return ullhid_client_ctrl.pUllhidClient[idx];
}

static int blt_ullhidc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle

    if (connState == PRF_ACL_STATE_DISCONN) {
        BLT_ULLHID_LOG("Disconnect:0x%x", connHandle);
        ULLHID_FREE(ullhid_client_ctrl.pUllhidClient[idx]);
        ullhid_client_ctrl.pUllhidClient[idx] = NULL;
    } else {
        BLT_ULLHID_LOG("Connect:0x%x", connHandle);
        ullhid_client_ctrl.pUllhidClient[idx] = ULLHID_MALLOC(sizeof(struct blc_ullhid_client));
    #if ((!defined(HOST_V2_ENABLE)))
        memset(ullhid_client_ctrl.pUllhidClient[idx], 0, sizeof(struct blc_ullhid_client));
    #else
        if (ullhid_client_ctrl.pUllhidClient[idx] != NULL) {
            memset(ullhid_client_ctrl.pUllhidClient[idx], 0, sizeof(struct blc_ullhid_client));
        }
    #endif    
}

    return 0;
}

static int blt_ullhidc_discovery(u16 connHandle)
{
    BLC_BASIC_SDP_DISCOVERY(connHandle, ULLHID, ullhid);
}

static int blt_ullhidc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param)
{
    BLC_BASIC_NV_STORE(connHandle, ULLHID, ullhid, operationHdl);
    return 0;
}
#if ((!defined(HOST_V2_ENABLE)))
static att_err_t blt_ullhidc_checkLeHidOperationModeParam(u8 *pData, u8 length)
{
    if (pData == NULL || length < sizeof(struct ullhid_operation_mode_pdu_format)) {
        return ATT_ERR_VALUE_NOT_ALLOWED;
    }

    struct ullhid_operation_mode_pdu_format *operation = (struct ullhid_operation_mode_pdu_format *)pData;
    int                                      err       = ATT_SUCCESS;

    if (operation->opcode == ULL_HID_OPCODE_SELECT_HYBRID_MODE) {
        //select hybrid mode field, opcode(1B)+CIG ID(1B)+CIS ID(1B)+Interval(2B)+n*(indices(1B)), 8+n
        if (length > sizeof(struct ullhid_select_hybrid_mode_format) || length <= ULLHID_SELECT_HYBRID_MODE_HEAD_SIZE) {
            err = ATT_ERR_VALUE_NOT_ALLOWED;
        }
    } else if (operation->opcode == ULL_HID_OPCODE_SELECT_DEFAULT_MODE) {
        if (length != sizeof(struct ullhid_select_default_mode_format)) {
            err = ATT_ERR_VALUE_NOT_ALLOWED;
        }
    } else {
        err = ATT_ERR_VALUE_NOT_ALLOWED;
    }

    return err;
}
#else
static att_err_t blt_ullhidc_checkLeHidOperationModeParam(u8 *pData, u8 length)
{
    if(pData == NULL || length < sizeof(struct ullhid_operation_mode_pdu_format))
    {
        return (att_err_t)ULL_HID_ERR_UNSUPPORTED_FEATURE;//ATT_ERR_VALUE_NOT_ALLOWED;
    }

    struct ullhid_operation_mode_pdu_format *operation = (struct ullhid_operation_mode_pdu_format *)pData;
    int                                      err       = ATT_SUCCESS;

    if(operation->opcode == ULL_HID_OPCODE_SELECT_HYBRID_MODE)
    {
        //select hybrid mode field, opcode(1B)+CIG ID(1B)+CIS ID(1B)+Interval(2B)+n*(indices(1B)), 8+n
        if( length > sizeof(struct ullhid_select_hybrid_mode_format) || length <= ULLHID_SELECT_HYBRID_MODE_HEAD_SIZE) {
            err = ULL_HID_ERR_UNSUPPORTED_FEATURE;//ATT_ERR_VALUE_NOT_ALLOWED;
        }
    }
    else if(operation->opcode == ULL_HID_OPCODE_SELECT_DEFAULT_MODE)
    {
        if(length != sizeof(struct ullhid_select_default_mode_format))
        {
            err = ULL_HID_ERR_UNSUPPORTED_FEATURE;//ATT_ERR_VALUE_NOT_ALLOWED;
        }
    }
    else
    {
        err = ULL_HID_ERR_OPCODE_OUTSIDE_RANGE;//ATT_ERR_VALUE_NOT_ALLOWED;
    }

    return err;
}
#endif
static void blt_ullhidc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    BLT_ULLHID_LOG("receive data, connHandle:0x%x, attHdl:0x%x, value:%s", connHandle, attHdl, hex_to_str(val, valLen));

    struct blc_ullhid_client *client = blt_ullhidc_getClientInst(connHandle);

    if (client->operationHdl != attHdl || blt_ullhidc_checkLeHidOperationModeParam(val, valLen) != ATT_SUCCESS) {
        return;
    }

    if (*val == ULL_HID_OPCODE_SELECT_DEFAULT_MODE) {
        blt_prf_sendEvent(connHandle, ULLHIDC_EVT_SELECT_DEFAULT_MODE, NULL, 0);
    } else if (*val == ULL_HID_OPCODE_SELECT_HYBRID_MODE) {
        u8 indicesSize = valLen - ULLHID_SELECT_HYBRID_MODE_HEAD_SIZE;

        struct ullhid_select_hybrid_mode_format *pHybrid = (struct ullhid_select_hybrid_mode_format *)val;

        if (blt_ullhid_checkSelectHybridMode(&client->properties, client->propertiesLen, pHybrid->suppInterval, &pHybrid->indices[0], indicesSize) != ATT_SUCCESS) {
            return;
        }

        struct ullhidc_selectHybridModeEvt evt;

        evt.reportInterval = blc_ullhid_convertReportIntervalBit(pHybrid->suppInterval.intervals);
        evt.reportCount    = indicesSize;
    #if ((!defined(HOST_V2_ENABLE)))
        evt.CIG_ID         = pHybrid->CIG_ID;
        evt.CIS_ID         = pHybrid->CIS_ID;
        for (int i = 0; i < indicesSize; i++) {
            evt.reportIndex[i] = pHybrid->indices[i];
            evt.reportInfo[i]  = client->properties.hybridModeReport[pHybrid->indices[i]];
        }
    #else
        evt.deviceToHostMaxSduSize = pHybrid->deviceToHostMaxSduSize;
        evt.hostToDeviceMaxSduSize = pHybrid->hostToDeviceMaxSduSize;
        // client CIG ID and CIS ID is unused.
        for (int i = 0; i < indicesSize; i++) {
            evt.reportInfo[i].reportID       = client->properties.hybridModeReport[pHybrid->indices[i].index].reportID;
            evt.reportInfo[i].reportType     = client->properties.hybridModeReport[pHybrid->indices[i].index].reportType;
            evt.reportInfo[i].powerSavingCfm = pHybrid->indices[i].cfmEnable;
            evt.reportInfo[i].repetition     = pHybrid->indices[i].repetitionEnable;
        }
   #endif
        blt_prf_sendEvent(connHandle, ULLHIDC_EVT_SELECT_HYBRID_MODE, &evt, sizeof(struct ullhidc_selectHybridModeEvt));
    }
}

/***************************ULL-HID sdp discovery*******************************/
void blc_ullhid_displayProperties(struct blc_ullhid_properties_format *properties, u16 propertiesLen)
{
    const char *temp[] = {"NoSupp", "Supp"};
    BLT_ULLHID_LOG("Features:Device Mode Change:%s", temp[properties->deviceModeChange]);
    BLT_ULLHID_LOG("Supported Report Interval: 1ms:%s, 2ms:%s, 3ms:%s, 4ms:%s, 5ms:%s, 1.25ms:%s, 2.5ms:%s, 3.75ms:%s",
                   temp[properties->suppInterval.interval_1ms],
                   temp[properties->suppInterval.interval_2ms],
                   temp[properties->suppInterval.interval_3ms],
                   temp[properties->suppInterval.interval_4ms],
                   temp[properties->suppInterval.interval_5ms],
                   temp[properties->suppInterval.interval_1_25ms],
                   temp[properties->suppInterval.interval_2_5ms],
                   temp[properties->suppInterval.interval_3_75ms]);

    BLT_ULLHID_LOG("max SDU size , Device to Host:%d, Host to Device:%d", properties->deviceToHostMaxSduSize, properties->hostToDeviceMaxSduSize);
    BLT_ULLHID_LOG("Hybrid Mode ULL Reports information");

    for (int i = 0; i < ((propertiesLen - ULLHID_PROPERTIES_HEAD_SIZE) >> 1); i++) {
        BLT_ULLHID_LOG("[%d] Report ID:%02d, Type:%s, PowerSavingConfirmation:%s, Repetition:%s", i, properties->hybridModeReport[i].reportID, properties->hybridModeReport[i].reportType == ULL_HID_REPORT_TYPE_INPUT ? "Input" : "Output", temp[properties->hybridModeReport[i].powerSavingCfm], temp[properties->hybridModeReport[i].repetition]);
    }
}

void blc_ullhidc_displayProperties(u16 connHandle)
{
    struct blc_ullhid_client *client = blt_ullhidc_getClientInst(connHandle);
    blc_ullhid_displayProperties(&client->properties, client->propertiesLen);
}

static void blt_ullhidc_displayInfo(u16 connHandle, struct blc_ullhid_client *client)
{
    BLT_ULLHID_LOG("ULL-HID sdp over connHandle[0x%x]", connHandle);
    BLT_ULLHID_LOG("ULL HID Properties:[handle: 0x%x] LE HID Operation Mode:[handle: 0x%x]", client->propertiesHdl, client->operationHdl);
    blc_ullhid_displayProperties(&client->properties, client->propertiesLen);
}

BLT_BASIC_SDP_DISCOVERY_SERVICE(ullhid, ULLHID)
BLT_DEFINE_ULLHID_DISCOVERY_FOUND_CHAR(properties)
BLT_DEFINE_ULLHID_DISCOVERY_START_READ(properties)
BLT_DEFINE_ULLHID_DISCOVERY_FOUND_CHAR(operation)

static const blc_gapc_discService_t ullhidService = {
    .uuid = UUID16_INIT(SERVICE_UUID_ULL_HID),
    .sfun = blt_ullhidc_foundService,
};

static const blc_gapc_discChar_t ullhidChar[] = {
    BLT_ULLHID_DISCOVERY_READ_CHAR(CHARACTERISTIC_UUID_ULL_HID_PROPERTIES, properties),
    BLT_ULLHID_DISCOVERY_IND_CHAR(CHARACTERISTIC_UUID_LE_HID_OPERATION_MODE, operation),
};

static const blc_gapc_discList_t discUllhid = {
    .maxServiceCount = 1,
    .service         = &ullhidService,
    .includeTable    = {
                        .size = 0,
                        },
    .characteristicTable = {
                        .size           = ARRAY_SIZE(ullhidChar),
                        .characteristic = ullhidChar,
                        },
};

/***************************ULL-HID sdp discovery end*******************************/

/**********reconnect function start*********/
BLT_BASIC_RECONNECT_SERVICE(ullhid, ULLHID)
BLT_ULLHID_RECONNECT_GET_INFO_READ(properties)

static const blc_gapc_reconnChar_t reUllhidChar[] = {
    BLT_ULLHID_RECONNECT_CHAR(properties),
};

static const blc_gapc_reconnList_t reconnUllhid = {
    .resfun = blt_ullhidc_recService,
    .charTb = {
               .size           = ARRAY_SIZE(reUllhidChar),
               .characteristic = reUllhidChar,
               },
    .inclSize = 0,
};

/**********reconnect function ending********/

/**********Read Characteristic Attribute Value*********/
int blc_ullhidc_readUllhidProperties(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_ULLHID_READ_ATTR_VALUE(properties);
}

/**********Read Characteristic Attribute Value End*********/

/**********Write Characteristic Attribute Value *********/
int blc_ullhidc_writeLeHidOperationMode(u16 connHandle, u8 *operation, u16 operationLen, prf_write_cb_t writeCb)
{
    BLT_ULLHID_WRITE_ATTR_VALUE_WITH_LEN(operation);
}

int blc_ullhidc_writeSelectHybridMode(u16 connHandle, struct ullhid_select_hybrid_param *param, prf_write_cb_t writeCb)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLT_ULLHID_LOG("ERR: ACL handle invalid");
        return PRF_HCI_ERROR_FLAG + HCI_ERR_UNKNOWN_CONN_ID;
    }

    //TODO: check input param.
    struct blc_ullhid_client *client = blt_ullhidc_getClientInst(connHandle);

    if (client == NULL || client->propertiesHdl == 0 || client->propertiesLen <= ULLHID_PROPERTIES_HEAD_SIZE) {
        return ULLHIDC_ERROR_NO_ULL_HID_PROPERTIES_HDL;
    }

    if (client->operationHdl == 0) {
        return ULLHIDC_ERROR_NO_LE_HID_OPERATION_MODE_HDL;
    }

    if (param->indicesCnt == 0 || param->indicesCnt > ULL_HID_HYBRID_MODE_ULL_REPORT_COUNT) {
        return PRF_COMMON_ERR_INPUT_PARAM_INVALID;
    }

    if (param->suppInterval.intervalRFU != 0 ||                                          //Invalid report intervals, RFU is not zero.
        blt_calBit1Number_16bit(param->suppInterval.intervals) != 1 ||                   // More than 1 bit is set to 1
        (param->suppInterval.intervals & client->properties.suppInterval.intervals) == 0 //Unsupported report interval.

    ) {
        return PRF_COMMON_ERR_INPUT_PARAM_INVALID;
    }

    u8 map = 0;
#if ((!defined(HOST_V2_ENABLE)))
    for (int i = 0; i < param->indicesCnt; i++) {
        if (param->indices[i] >= ((client->propertiesLen - ULLHID_PROPERTIES_HEAD_SIZE) >> 1)) {
            return PRF_COMMON_ERR_INPUT_PARAM_INVALID;
        }

        if (map & BIT(param->indices[i])) {
            return PRF_COMMON_ERR_INPUT_PARAM_INVALID;
        }
        map |= BIT(param->indices[i]);
    }

    struct ullhid_select_hybrid_mode_format pHybrid = {
        .opcode                 = ULL_HID_OPCODE_SELECT_HYBRID_MODE,
        .CIG_ID                 = param->CIG_ID,
        .CIS_ID                 = param->CIS_ID,
        .suppInterval.intervals = param->suppInterval.intervals};

    memcpy(pHybrid.indices, param->indices, param->indicesCnt);
#else
    for (int i = 0; i < param->indicesCnt; i++) {
        int index = param->indices[i].index;
        if (index >= ((client->propertiesLen - ULLHID_PROPERTIES_HEAD_SIZE) >> 1)) {
            return PRF_COMMON_ERR_INPUT_PARAM_INVALID;
        }

        if (map & BIT(index)) {
            return PRF_COMMON_ERR_INPUT_PARAM_INVALID;
        }
        map |= BIT(index);
    }

    struct ullhid_select_hybrid_mode_format pHybrid = {
        .opcode                 = ULL_HID_OPCODE_SELECT_HYBRID_MODE,
        .CIG_ID                 = param->CIG_ID,
        .CIS_ID                 = param->CIS_ID,
        .suppInterval.intervals = param->suppInterval.intervals,
        .deviceToHostMaxSduSize = param->deviceToHostMaxSduSize,
        .hostToDeviceMaxSduSize = param->hostToDeviceMaxSduSize

    };
    for (int i = 0; i < param->indicesCnt; i++) {
        pHybrid.indices[i].index            = param->indices[i].index;
        pHybrid.indices[i].rfu              = 0;
        pHybrid.indices[i].cfmEnable        = param->indices[i].cfmEnable;
        pHybrid.indices[i].repetitionEnable = param->indices[i].repetitionEnable;
    }
#endif
    return blc_ullhidc_writeLeHidOperationMode(connHandle, (u8 *)&pHybrid, ULLHID_SELECT_HYBRID_MODE_HEAD_SIZE + param->indicesCnt, writeCb);
}

int blc_ullhidc_writeSelectDefaultMode(u16 connHandle, prf_write_cb_t writeCb)
{
    struct ullhid_select_default_mode_format selectDefault = {
        .opcode = ULL_HID_OPCODE_SELECT_DEFAULT_MODE};
    return blc_ullhidc_writeLeHidOperationMode(connHandle, (u8 *)&selectDefault, sizeof(struct ullhid_select_default_mode_format), writeCb);
}

/**********Write Characteristic Attribute Value End*********/

/**********Get Characteristic Attribute Value*********/

int blc_ullhidc_getUllhidProperties(u16 connHandle, struct blc_ullhid_properties_format *properties, u16 *propertiesLen)
{
    BLT_ULLHID_GET_ATTR_VALUE(properties);
}

/**********Get Characteristic Attribute Value End*********/
