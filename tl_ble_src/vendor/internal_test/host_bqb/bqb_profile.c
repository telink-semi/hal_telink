/********************************************************************************************************
 * @file    bqb_profile.c
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

#include "app.h"
#include "app_ui.h"
#include "app_config.h"
#include "bqb_profile.h"

#if (INTER_TEST_MODE == TEST_HOST_BQB)

    #define BQB_GATT_START_HDL 0x0800
    #define BQB_GATT_END_HDL   (BQB_GATT_MAX_HDL - 1)

enum
{
    BQB_GATT_SVC_HDL = BQB_GATT_START_HDL,
    BQB_GATT_ATTR1_CD_HDL,
    BQB_GATT_ATTR1_DP_HDL,
    BQB_GATT_ATTR2_CD_HDL,
    BQB_GATT_ATTR2_DP_HDL,
    BQB_GATT_ATTR3_CD_HDL,
    BQB_GATT_ATTR3_DP_HDL,
    BQB_GATT_ATTR3_CCC_HDL,
    BQB_GATT_ATTR4_CD_HDL,
    BQB_GATT_ATTR4_DP_HDL,
    BQB_GATT_ATTR4_CCC_HDL,
    BQB_GATT_ATTR4_CHAR_FORMAT_HDL,
    BQB_GATT_ATTR4_AGG_FORMAT_HDL,
    BQB_GATT_MAX_HDL,
};

const u8 bqbServiceUuid[ATT_16_UUID_LEN] = {U16_TO_BYTES(0xFFF0)};

static const u8 bqbAttr1CharVal[]             = SERVICE_CHAR_READ(BQB_GATT_ATTR1_DP_HDL, 0xFFF1);
u8              defaultAttrVal[512]           = {0x01, 0x02, 0x03, 0x04, 0x05};
u16             defaultAttrValLen             = 50;
const u8        bqbAttr1Uuid[ATT_16_UUID_LEN] = {U16_TO_BYTES(0xFFF1)};

static const u8 bqbAttr2CharVal[]             = SERVICE_CHAR_READ_WRITE_WRITEWITHOUT(BQB_GATT_ATTR2_DP_HDL, 0xFFF2);
const u8        bqbAttr2Uuid[ATT_16_UUID_LEN] = {U16_TO_BYTES(0xFFF2)};

static const u8 bqbAttr3CharVal[]             = SERVICE_CHAR_READ_NOTIFY(BQB_GATT_ATTR3_DP_HDL, 0xFFF3);
const u8        bqbAttr3Uuid[ATT_16_UUID_LEN] = {U16_TO_BYTES(0xFFF3)};

static const u8 bqbAttr4CharVal[]             = SERVICE_CHAR_READ_NOTIFY(BQB_GATT_ATTR4_DP_HDL, 0xFFF4);
const u8        bqbAttr4Uuid[ATT_16_UUID_LEN] = {U16_TO_BYTES(0xFFF4)};

u8 defaultAttr3Val[100] = {
    0x15,
    0x16,
    0x17,
    0x18,
    0x19,
    0x1a,
};
u16 defaultAttr3ValLen = 10;

static const u8  attr3CCC[100] = {0, 0};
static const u16 attr3CCCLen   = 2;

u8  defaultAttr4Val[100] = {0x15, 0x16};
u16 defaultAttr4ValLen   = 2;

typedef struct
{
    u8  format;
    u8  exponent;
    u16 unit;
    u8  nameSpace;
    u16 description;
} characteristic_format_value_def_t;

static characteristic_format_value_def_t attr4ValFormat = {
    .format      = 0x06, //uint16
    .exponent    = 0,
    .unit        = 0x0001,
    .nameSpace   = 0,
    .description = 0x0002,
};
static u16 attr4ValFormatLen = sizeof(characteristic_format_value_def_t);

static u16 attr4AggFormat[5] = {BQB_GATT_ATTR1_DP_HDL, BQB_GATT_ATTR3_DP_HDL, BQB_GATT_ATTR4_DP_HDL};

static u16 attr4AggFormatLen = 6;

static const atts_attribute_t bqtGattList[] =
    {

        ATTS_PRIMARY_SERVICE(bqbServiceUuid),

        ATTS_CHARACTER_DEFINE(bqbAttr1CharVal),
        ATTS_CHAR_UUID_READ_POINT_CB(bqbAttr1Uuid, 512, defaultAttrVal),

        ATTS_CHARACTER_DEFINE(bqbAttr2CharVal),
        //  {},
        ATTS_CHAR_UUID_RDWR_POINT(bqbAttr2Uuid, 100, defaultAttrVal, ATTS_SET_WRITE_CBACK | ATTS_SET_READ_CBACK | ATTS_SET_ALLOW_WRITE | ATTS_SET_VARIABLE_LEN),

        //  ATTS_CHAR_UUID_RDWR_POINT_RWCB(bqbAttr2Uuid, 512, defaultAttrVal),

        ATTS_CHARACTER_DEFINE(bqbAttr3CharVal),
        ATTS_CHAR_UUID_READ_POINT_CB(bqbAttr3Uuid, 50, defaultAttr3Val),
        //  ATTS_CHAR_UUID_RDWR_POINT(bqbAttr3Uuid, 50, defaultAttr3Val, ATTS_SET_WRITE_CBACK | ATTS_SET_READ_CBACK | ATTS_SET_ALLOW_WRITE | ATTS_SET_VARIABLE_LEN),

        ATTS_CCC_DEFINE(attr3CCC),

        //  ATTS_CHAR_UUID_RDWR_POINT(descriptorCharacteristicAggregateFormatUuid, 100, attr3CCC, ATTS_SET_VARIABLE_LEN | ATTS_SET_ALLOW_WRITE),

        ATTS_CHARACTER_DEFINE(bqbAttr4CharVal),
        ATTS_CHAR_UUID_READ_POINT_CB(bqbAttr4Uuid, 50, defaultAttr4Val),

        ATTS_CCC_DEFINE(attr3CCC),
        ATTS_CHAR_UUID_READ_ENTITY_NOCB(descriptorCharacteristicPresentationFormatUuid, attr4ValFormat),
        ATTS_CHAR_UUID_READ_POINT_NOCB(descriptorCharacteristicAggregateFormatUuid, 100, attr4AggFormat),
};

u8 read_err = ATT_SUCCESS;

int bqb_gatt_read_cb(u16 connHandle, u8 opcode, u16 attrHandle, u8 **outValue, u16 *outValueLen)
{
    tlkapi_send_string_data(APP_GATT_LOG_EN, "read cb", &read_err, 1);

    #if GATT_SR_GAS_BV_05_c
    u8 err   = read_err;
    read_err = ATT_SUCCESS;
    return err;
    #else
    return read_err;
    #endif
}

int bqb_gatt_write_cb(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    tlkapi_send_string_data(APP_GATT_LOG_EN, "write cb", &read_err, 1);
    return read_err;
}

_attribute_ble_data_retention_ static atts_group_t svcBqbGattGroup =
    {
        NULL,
        (atts_attribute_t *)bqtGattList,
        bqb_gatt_read_cb,
        bqb_gatt_write_cb,
        BQB_GATT_START_HDL,
        BQB_GATT_END_HDL};

u16 gAppBqbConnHandle = 0;

void blc_svc_addBqbGattGroup(void)
{
    blc_gatts_addAttributeServiceGroup(&svcBqbGattGroup);
}

int app_bqb_debug(unsigned char *p, int len);

void app_bqb_init(void)
{
    //blc_smp_setSecurityLevel_slave(No_Security);
    blc_smp_configSecurityRequestSending(SecReq_NOT_SEND, SecReq_NOT_SEND, 0);
    blc_smp_configPairingRequestSending(PairReq_AUTO_SEND, PairReq_AUTO_SEND);

    myudb_register_hci_debug_cb(app_bqb_debug);

    usb_send_str_data("app_bqb_init", 0, 0);
}

void app_bqb_connect(u16 aclHandle)
{
    gAppBqbConnHandle = aclHandle;

    tlkapi_printf(0, "app_bqb_connect %x", aclHandle);
}

void app_bqb_disconn(u16 aclHandle)
{
    tlkapi_printf(0, "app_bqb_disconnect %x", aclHandle);
}

void app_bqb_handler(void)
{
}

typedef int (*bqb_debug_cb_t)(unsigned char *p, int len);

typedef struct
{
    u16            minCase;
    u16            maxCase;
    bqb_debug_cb_t cb;
} bqb_debug_t;

int gatt_gar_cb(unsigned char *p, int len)
{
    tlkapi_send_string_data(APP_GATT_LOG_EN, "GAR CB", NULL, 0);

    read_err = p[3];

    return 0;
}

int gatt_gan_cb(unsigned char *p, int len)
{
    tlkapi_send_string_data(APP_GATT_LOG_EN, "GAN CB", NULL, 0);

    blc_atts_sendHandleValueNotify(0x44, BQB_GATT_ATTR3_DP_HDL, defaultAttr3Val, 10);

    return 0;
}

int gatt_gan_bv_02_cb(unsigned char *p, int len)
{
    tlkapi_send_string_data(APP_GATT_LOG_EN, "GAN CB", NULL, 0);
    extern ble_sts_t blc_atts_sendMultHandleValueNtf(u16 connHandle, atts_multHandleNtf_t * ntf, int count);

    atts_multHandleNtf_t ntf[2] = {
        {
         .handle = BQB_GATT_ATTR4_DP_HDL,
         .length = 2,
         .value  = defaultAttr3Val,
         },
        {
         .handle = BQB_GATT_ATTR3_DP_HDL,
         .length = 4,
         .value  = defaultAttr3Val + 3,
         },
    };


    blc_atts_sendMultHandleValueNtf(0x44, ntf, 2);

    return 0;
}

int gatt_gai_cb(unsigned char *p, int len)
{
    tlkapi_send_string_data(APP_GATT_LOG_EN, "GAN CB", NULL, 0);

    blc_atts_sendHandleValueIndicate(0x44, BQB_GATT_ATTR3_DP_HDL, defaultAttr3Val, 10);

    return 0;
}

int gatt_gas_bv_01_c(unsigned char *p, int len)
{
    static int count = 0;

    tlkapi_send_string_data(APP_GATT_LOG_EN, "gas BV-01-C", &count, 4);
    count = (count + 1) & 0x01;
    if (count == 1) {
        blc_svc_addBasGroup();
        blc_svc_calculateDatabaseHash();
    } else {
        u16 value[2] = {BAS_START_HDL, BAS_END_HDL};
        blc_atts_sendHandleValueIndicate(0x44, GATT_SERVICE_CHANGED_DP_HDL, (u8 *)value, 4);
    }


    return 0;
}

//blc_gattc_writeAttributeValue

gattc_write_cfg_t clWriteCfg;
u8                writeDataBuff[256];

void gatt_cl_write_attr_value_cb(u16 connHandle, u8 err, struct gattc_write_cfg *params)
{
    tlkapi_printf(APP_GATT_LOG_EN, "write attr value callback, err is %d", err);
}

int gatt_cl_write_attr_value(unsigned char *p, int len)
{
    p              = p + 3; //skip head & value
    u16 connHandle = 0x0000;
    STREAM_TO_U16(connHandle, p);
    if (connHandle == 0) {
        blc_ll_disconnect(gAppBqbConnHandle, HCI_ERR_REMOTE_USER_TERM_CONN);
        return 0;
    }
    STREAM_TO_U16(clWriteCfg.handle, p);
    STREAM_TO_U16(clWriteCfg.length, p);
    STREAM_TO_U8(clWriteCfg.withoutRsp, p);
    STREAM_TO_STR(writeDataBuff, p, min(clWriteCfg.length, 256));
    //  memset(writeDataBuff, clWriteCfg.length, 256);
    clWriteCfg.offset = 0x0000;
    clWriteCfg.data   = writeDataBuff;
    clWriteCfg.func   = gatt_cl_write_attr_value_cb;
    ble_sts_t status  = blc_gattc_writeAttributeValue(connHandle, &clWriteCfg);
    tlkapi_printf(APP_GATT_LOG_EN, "connHandle %x %x %d %d", connHandle, clWriteCfg.handle, clWriteCfg.length, clWriteCfg.withoutRsp);
    tlkapi_printf(APP_GATT_LOG_EN, "gatt layer send write attr value result is %d", status);
    return 0;
}

u8 gatt_cl_read_attr_value_cb(u16 connHandle, u8 err, gatt_read_data_t *rdData, struct gattc_read_cfg *params)
{
    if (err) {
        tlkapi_printf(APP_GATT_LOG_EN, "read attr value callback, err is %d", err);
        return GATT_PROC_END;
    }
    tlkapi_printf(APP_GATT_LOG_EN, "read value state is %d, data(%d) is %s", rdData->rdState, rdData->dataLen, hex_to_str(rdData->dataVal, rdData->dataLen));

    if (params->hdlCnt == 1) {
        tlkapi_printf(APP_GATT_LOG_EN, "read sing data(%d) is %s", *params->single.wBuffLen, hex_to_str(params->single.wBuff, *params->single.wBuffLen));
    }

    return GATT_PROC_CONT;
}

gattc_read_cfg_t clReadCfg;
u16              readAttrLen;

uuid_t readUuid;
u16    readHandle[2];

int gatt_cl_read_attr_value(unsigned char *p, int len)
{
    p              = p + 3; //skip head & value
    u16 connHandle = 0x0000;
    STREAM_TO_U16(connHandle, p);
    if (connHandle == 0) {
        blc_ll_disconnect(gAppBqbConnHandle, HCI_ERR_REMOTE_USER_TERM_CONN);
        return 0;
    }
    STREAM_TO_U16(clReadCfg.hdlCnt, p);
    if (clReadCfg.hdlCnt == 0) {
        STREAM_TO_U16(clReadCfg.byUuid.startHdl, p);
        STREAM_TO_U16(clReadCfg.byUuid.endHdl, p);
        readUuid.uuidLen = len - 11;
        STREAM_TO_STR(readUuid.uuidVal.u, p, len - 11);
        clReadCfg.byUuid.uuid = &readUuid;
    } else if (clReadCfg.hdlCnt == 1) {
        STREAM_TO_U16(clReadCfg.single.handle, p);
        clReadCfg.single.offset   = 0;
        clReadCfg.single.wBuff    = writeDataBuff;
        clReadCfg.single.maxLen   = sizeof(writeDataBuff);
        clReadCfg.single.wBuffLen = &readAttrLen;
    } else {
        STREAM_TO_U8(clReadCfg.multiple.variable, p);
        STREAM_TO_U16(readHandle[0], p);
        STREAM_TO_U16(readHandle[1], p);
        clReadCfg.multiple.handles = readHandle;
    }
    clReadCfg.func   = gatt_cl_read_attr_value_cb;
    ble_sts_t status = blc_gattc_readAttributeValue(connHandle, &clReadCfg);
    tlkapi_printf(APP_GATT_LOG_EN, "gatt layer send read attr value result is %d", status);
    return 0;
}

gattc_sdp_cfg_t clDisc;
uuid_t          discUuid;

u8 gatt_cl_discovery_cb(u16 connHandle, gatt_attr_t *attr, struct gattc_sdp_cfg *params)
{
    return GATT_PROC_CONT;
}

int gatt_cl_discovery(unsigned char *p, int len)
{
    memset(&clDisc, 0, sizeof(gattc_sdp_cfg_t));
    p              = p + 3; //skip head & value
    u16 connHandle = 0x0000;
    STREAM_TO_U16(connHandle, p);
    if (connHandle == 0) {
        blc_ll_disconnect(gAppBqbConnHandle, HCI_ERR_REMOTE_USER_TERM_CONN);
        return 0;
    }
    STREAM_TO_U8(clDisc.type, p);

    STREAM_TO_U16(clDisc.startHdl, p);
    STREAM_TO_U16(clDisc.endHdl, p);

    discUuid.uuidLen = len - 10;
    STREAM_TO_STR(discUuid.uuidVal.u, p, len - 10);
    clDisc.uuid = &discUuid;

    clDisc.func      = gatt_cl_discovery_cb;
    ble_sts_t status = blc_gattc_discovery(connHandle, &clDisc);

    tlkapi_printf(APP_GATT_LOG_EN, "gatt layer send discovery result is %d", status);
    return 0;
}

bqb_debug_t debug_fun[] = {
    {GATT_SR_GAR_BI_03_C,      GATT_SR_GAR_BI_03_C,      gatt_gar_cb             },
    {GATT_SR_GAN_BV_01_C,      GATT_SR_GAN_BV_01_C,      gatt_gan_cb             },
    {GATT_SR_GAN_BV_02_C,      GATT_SR_GAN_BV_02_C,      gatt_gan_bv_02_cb       },
    {GATT_SR_GAI_BV_01_C,      GATT_SR_GAI_BV_01_C,      gatt_gai_cb             },
    {GATT_SR_GAS_BV_01_C,      GATT_SR_GAS_BV_01_C,      gatt_gas_bv_01_c        },
    {GATT_CL_WRITE_ATTR_VALUE, GATT_CL_WRITE_ATTR_VALUE, gatt_cl_write_attr_value},
    {GATT_CL_READ_ATTR_VALUE,  GATT_CL_READ_ATTR_VALUE,  gatt_cl_read_attr_value },
    {GATT_CL_DISCOVERY,        GATT_CL_DISCOVERY,        gatt_cl_discovery       },
};

int app_bqb_debug(unsigned char *p, int len)
{
    my_dump_str_data(0, "app_bqb_debug receive", p, len);

    if (p[0] != 0x11) {
        return 1;
    }

    u16 test_case = ((p[1] << 8) | p[2]);
    //u16 attHandle;

    for (int i = 0; i < ARRAY_SIZE(debug_fun); i++) {
        if (test_case >= debug_fun[i].minCase && test_case <= debug_fun[i].maxCase) {
            debug_fun[i].cb(p, len);
        }
    }

    switch (test_case) {
    /******************** UI ********************/
    #if BQB_UI_OPERATION_ENABLE
    case BQB_LE_START_PAIR: //cmd: 11 00 00
    {
        central_pairing_enable = 1;
        tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Pair begin", 0, 0);
    } break;
    case BQB_LE_START_UNPAIR: //cmd: 11 00 01
    {
        /*Here is just Telink Demonstration effect. Cause the demo board has limited key to use, only one "un_pair" key is
             available. When "un_pair" key pressed, we will choose and un_pair one device in connection state */
        if (acl_conn_central_num) {               //at least 1 central connection exist

            if (!central_disconnect_connhandle) { //if one central un_pair disconnection flow not finish, here new un_pair not accepted

                /* choose one central connection to disconnect */
                for (int i = 0; i < ACL_CENTRAL_MAX_NUM; i++) {               //peripheral index is from 0 to "ACL_CENTRAL_MAX_NUM - 1"
                    if (conn_dev_list[i].conn_state) {
                        central_unpair_enable = conn_dev_list[i].conn_handle; //mark connHandle on central_unpair_enable
                        tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Unpair", &central_unpair_enable, 2);
                        break;
                    }
                }
            }
        }

        if (acl_conn_periphr_num) { //at least 1 central connection exist
            blc_ll_disconnect(gAppBqbConnHandle, HCI_ERR_REMOTE_USER_TERM_CONN);
        }
    } break;
    case BQB_LE_REBOOT_DEV: //cmd: 11 00 02
    {
        tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] reboot device", 0, 0);
        while (tlkapi_debug_isBusy()) {
            tlkapi_debug_handler();
        }
        sleep_ms(50);
        start_reboot();        //reboot the MCU
    } break;

    case BQB_LE_ENTER_PINCODE: //cmd: 11 00 03 XX XX XX  (SM/PER/OOB/BV-04-C  need this)
    {
        u32 tk = bstream_to_u24_le(p + 3);
        //          blc_smp_sendKeypressNotify(gAppBqbConnHandle, KEYPRESS_NTF_PKE_START);

        if (blc_smp_setTK_by_PasskeyEntry(gAppBqbConnHandle, tk)) {
            tlkapi_printf(APP_HOST_EVT_LOG_EN, "[APP][EVT] Set TK SUCC: <<%d>>", tk);
            //              blc_smp_sendKeypressNotify(gAppBqbConnHandle, KEYPRESS_NTF_PKE_COMPLETED);
        } else {
            tlkapi_printf(APP_HOST_EVT_LOG_EN, "[APP][EVT] Set TK FAIL: <<%d>>", tk);
        }
    } break;

    #endif /******************** UI ********************/

    /******************** GAP ********************/
    #if BQB_GAP_TESTCASE_ENABLE
    case GAP_BROB_BCST_BV_01_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "GAP_BROB_BCST_BV_01_C", 0, 0);

    } break;

    #endif
    /******************** GATT ********************/
    #if BQB_GATT_TESTCASE_ENABLE
    case GATT_CL_GAC_BV_01_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "GATT_CL_GAC_BV_01_C", 0, 0);

    } break;

    #endif
    /******************** L2CAP ********************/
    #if BQB_L2CAP_TESTCASE_ENABLE
    case L2CAP_COS_CFC_BV_01_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "L2CAP_COS_CFC_BV_01_C", 0, 0);

    } break;

    #endif /******************** L2CAP ********************/

    /******************** SM ********************/
    #if BQB_SM_TESTCASE_ENABLE
    ///////////////////////////// Central ////////////////////////////
    case SM_CEN_PROT_BV_01_C: //cmd1: 11 13 01 ; cmd2: 11 00 00 ; cmd3 11 00 01
    case SM_CEN_JW_BV_05_C:
    case SM_CEN_JW_BI_04_C:
    case SM_CEN_JW_BI_01_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_CEN_PROT_BV_01_C ~ SM_CEN_JW_BI_01_C", &test_case, 2);
    } break;

    case SM_CEN_PKE_BV_01_C: //cmd1: 11 13 05 ; cmd2: 11 00 00 ; cmd3 11 00 01
    case SM_CEN_PKE_BV_04_C:
    case SM_CEN_PKE_BI_01_C:
    case SM_CEN_PKE_BI_02_C:
    case SM_CEN_OOB_BV_05_C:
    case SM_CEN_OOB_BV_07_C:
    case SM_CEN_EKS_BV_01_C:
    case SM_CEN_EKS_BI_01_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_CEN_PKE_BV_01_C ~ SM_CEN_EKS_BI_01_C", 0, 0);
        blc_smp_setSecurityLevel_master(Authenticated_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Non_Bondable_Mode, 0, LE_Legacy_Pairing, 0, 0, IO_CAPABILITY_DISPLAY_ONLY);
        blc_smp_setDefaultPinCode(123456);
        blc_smp_smpParamInit();
    } break;

    case SM_CEN_KDU_BI_01_C: //cmd1: 11 13 0D; cmd2: 11 00 00 ; cmd3 11 00 01
    case SM_CEN_KDU_BI_02_C:
    case SM_CEN_KDU_BI_03_C:
    case SM_CEN_KDU_BV_05_C:
    case SM_CEN_KDU_BV_06_C:
    case SM_CEN_KDU_BV_10_C:
    case SM_CEN_SIP_BV_02_C:
    case SM_CEN_SCJW_BV_01_C:
    case SM_CEN_SCJW_BI_01_C:

    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_CEN_KDU_BI_01_C ~ SM_CEN_SCJW_BI_01_C", 0, 0);
        blc_smp_setSecurityLevel(Unauthenticated_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Non_Bondable_Mode, 0, LE_Secure_Connection, 0, 0, IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
        //blc_smp_setEcdhDebugMode(debug_mode);
        blc_smp_smpParamInit();
    } break;

    case SM_CEN_SCPK_BV_01_C: //cmd1: 11 13 16; cmd2: 11 00 00 ; cmd3 11 00 01
    case SM_CEN_SCPK_BV_04_C:
    case SM_CEN_SCPK_BI_01_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_CEN_SCPK_BV_01_C ~ SM_CEN_SCPK_BI_01_C", 0, 0);
        blc_smp_setSecurityLevel_master(Authenticated_LE_Secure_Connection_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Non_Bondable_Mode, 0, LE_Secure_Connection, 0, 1, IO_CAPABILITY_DISPLAY_ONLY);
        blc_smp_setDefaultPinCode(123456);
        //blc_smp_setEcdhDebugMode(debug_mode);
        blc_smp_smpParamInit();
    } break;

    case SM_CEN_SCPK_BI_02_C: //cmd1: 11 13 19; cmd2: 11 00 00 ; cmd3 11 00 01
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_CEN_SCPK_BI_02_C", 0, 0);
        blc_smp_setSecurityLevel_master(Authenticated_LE_Secure_Connection_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Bondable_Mode, 0, LE_Secure_Connection, 0, 0, IO_CAPABILITY_DISPLAY_ONLY);
        blc_smp_setDefaultPinCode(123456);
        //blc_smp_setEcdhDebugMode(debug_mode);
        blc_smp_smpParamInit();
    } break;

    case SM_CEN_OOB_BV_01_C: //cmd1: 11 13 1A; cmd2: 11 00 00 ; cmd3 11 00 01
    case SM_CEN_OOB_BV_03_C:
    case SM_CEN_OOB_BV_09_C:
    case SM_CEN_OOB_BI_01_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_CEN_OOB_BV_01_C ~ SM_CEN_OOB_BI_01_C", 0, 0);
        blc_smp_setSecurityLevel(Authenticated_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Non_Bondable_Mode, 1, LE_Legacy_Pairing, 1, 0, IO_CAPABILITY_KEYBOARD_ONLY);
        blc_smp_smpParamInit();
    } break;
    //SCOOB not support
    case SM_CEN_SCOB_BV_01_C: //cmd1: 11 13 1E; cmd2: 11 00 00 ; cmd3 11 00 01
    case SM_CEN_SCOB_BV_04_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_CEN_SCOB_BV_01_C ~ SM_CEN_SCOB_BV_04_C", 0, 0);
        blc_smp_setSecurityLevel(Authenticated_LE_Secure_Connection_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Non_Bondable_Mode, 1, LE_Secure_Connection, 0, 0, IO_CAPABILITY_KEYBOARD_ONLY);
        blc_smp_smpParamInit();
    } break;

    case SM_CEN_SCOB_BI_01_C: //cmd1: 11 13 20; cmd2: 11 00 00 ; cmd3 11 00 01
    case SM_CEN_SCOB_BI_04_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_CEN_SCOB_BI_01_C ~ SM_CEN_SCOB_BI_04_C", 0, 0);
        blc_smp_setSecurityLevel(Authenticated_LE_Secure_Connection_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Non_Bondable_Mode, 1, LE_Secure_Connection, 1, 0, IO_CAPABILITY_KEYBOARD_ONLY);
        blc_smp_smpParamInit();
    } break;

    ///////////////////////////// Peripheral ////////////////////////////
    case SM_PER_PROT_BV_02_C: //cmd1: 11 13 22
    case SM_PER_JW_BV_02_C:
    case SM_PER_JW_BI_03_C:
    case SM_PER_JW_BI_02_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_PER_PROT_BV_02_C ~ SM_PER_JW_BI_02_C", &test_case, 2);
    } break;

    case SM_PER_PKE_BV_02_C: //cmd1: 11 13 26
    case SM_PER_PKE_BI_03_C:
    case SM_PER_EKS_BV_02_C:
    case SM_PER_EKS_BI_02_C:
    case SM_PER_PKE_BV_05_C: //cmd1: 11 13 27
    {
        u8 mitm = p[2] == 0x27 ? 1 : 0;
        tlkapi_send_string_data(APP_LOG_EN, "SM_PER_PKE_BV_02_C ~ SM_PER_PKE_BV_05_C", 0, 0);
        blc_smp_setSecurityLevel_master(Authenticated_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Non_Bondable_Mode, mitm, LE_Legacy_Pairing, 0, 0, IO_CAPABILITY_DISPLAY_ONLY);
        blc_smp_setDefaultPinCode(123456);
        blc_smp_smpParamInit();
    } break;

    case SM_PER_OOB_BV_02_C: //cmd1: 11 13 2B
    case SM_PER_OOB_BV_04_C:
    case SM_PER_OOB_BV_10_C:
    case SM_PER_OOB_BI_02_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_PER_OOB_BV_02_C ~ SM_PER_OOB_BI_02_C", 0, 0);
        blc_smp_setSecurityLevel(Authenticated_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Non_Bondable_Mode, 1, LE_Legacy_Pairing, 1, 0, IO_CAPABILITY_KEYBOARD_ONLY);
        blc_smp_smpParamInit();
    } break;

    case SM_PER_OOB_BV_6_C: //cmd1: 11 13 3F
    case SM_PER_OOB_BV_8_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_PER_OOB_BV_6_C ~ SM_PER_OOB_BV_8_C", 0, 0);
        blc_smp_setSecurityLevel(Authenticated_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Non_Bondable_Mode, 1, LE_Legacy_Pairing, 0, 0, IO_CAPABILITY_KEYBOARD_ONLY);
        blc_smp_smpParamInit();
    } break;

        //SCOOB not support
        //TODO:

    case SM_PER_KDU_BV_01_C: //cmd1: 11 13 2F
    case SM_PER_KDU_BV_02_C:
    case SM_PER_KDU_BV_07_C:
    case SM_PER_KDU_BV_08_C:
    case SM_PER_KDU_BI_01_C:
    case SM_PER_KDU_BI_02_C:
    case SM_PER_KDU_BI_03_C:
    case SM_PER_SCJW_BV_03_C:
    case SM_PER_SCJW_BI_02_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_PER_KDU_BV_01_C ~ SM_PER_KDU_BI_03_C", 0, 0);
        blc_smp_setSecurityLevel(Unauthenticated_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Bondable_Mode, 0, LE_Secure_Connection, 0, 0, IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
        blc_smp_smpParamInit();
    } break;

    case SM_PER_SIP_BV_01_C: //cmd1: 11 13 38
    case SM_PER_SIE_BV_01_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_PER_SIP_BV_01_C ~ SM_PER_SIE_BV_01_C", 0, 0);
        blc_smp_configSecurityRequestSending(SecReq_PEND_SEND, SecReq_PEND_SEND, 1000);
        blc_smp_setSecurityLevel(Authenticated_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Bondable_Mode, 0, LE_Legacy_Pairing, 0, 0, IO_CAPABILITY_KEYBOARD_ONLY);
        blc_smp_smpParamInit();
    } break;

    case SM_PER_SCJW_BV_02_C: //cmd1: 11 13 3A
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_PER_SCJW_BV_02_C", 0, 0);
        blc_smp_setSecurityLevel(Authenticated_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Non_Bondable_Mode, 0, LE_Secure_Connection, 0, 0, IO_CAPABILITY_KEYBOARD_ONLY);
        blc_smp_smpParamInit();
    } break;

    case SM_PER_SCPK_BV_02_C: //cmd1: 11 13 3B
    case SM_PER_SCPK_BV_03_C:
    case SM_PER_SCPK_BI_03_C:
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_PER_SCPK_BV_02_C ~ SM_PER_SCPK_BI_03_C", 0, 0);
        blc_smp_setSecurityLevel_master(Authenticated_LE_Secure_Connection_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Non_Bondable_Mode, 0, LE_Secure_Connection, 0, 1, IO_CAPABILITY_DISPLAY_ONLY);
        blc_smp_setDefaultPinCode(123456);
        //blc_smp_setEcdhDebugMode(debug_mode);
        blc_smp_smpParamInit();
    } break;

    case SM_PER_SCPK_BI_04_C: //cmd1: 11 13 3E
    {
        tlkapi_send_string_data(APP_LOG_EN, "SM_PER_SCPK_BI_04_C", 0, 0);
        blc_smp_setSecurityLevel_master(Authenticated_LE_Secure_Connection_Pairing_with_Encryption);
        blc_smp_setSecurityParameters(Non_Bondable_Mode, 1, LE_Secure_Connection, 0, 0, IO_CAPABILITY_KEYBOARD_ONLY);
        //blc_smp_setEcdhDebugMode(debug_mode);
        blc_smp_smpParamInit();
    } break;

    #endif /******************** SM ********************/

    default:
        //          blc_ll_disconnect(gAppBqbConnHandle, HCI_ERR_REMOTE_USER_TERM_CONN);
        break;
    }


    return 0;
}

/*******************GATT client discovery all character**************************/
enum
{
    GATTC_MSG_DISCOVERY_PRIMARY_SERVICE,
    GATTC_MSG_DISCOVERY_CHAR,
    GATTC_MSG_DISCOVERY_INCLUDE_UUID,
};

enum
{
    APP_MSG_NULL,
    APP_MSG_STORED,
    APP_MSG_ANALYSIS,
};

typedef struct app_gatt_msg
{
    struct app_gatt_msg *pNext;
    u8                   state;
    u8                   type;
    u16                  connHandle;

    union
    {
        gattc_sdp_cfg_t     sdpCfg;
        gattc_sub_ccc_cfg_t cccCfg;
        gattc_read_cfg_t    readCfg; /* gapc used */
    };
} app_gatt_msg_t;

_attribute_ble_data_retention_
    app_gatt_msg_t gAppMsg[100];

queue_t gattcMsgQueue;

static app_gatt_msg_t *blt_app_getNewGattcMsg(void)
{
    for (int i = 0; i < ARRAY_SIZE(gAppMsg); i++) {
        if (gAppMsg[i].state) {
            continue;
        }
        gAppMsg[i].state = APP_MSG_STORED;
        return &gAppMsg[i];
    }
    return NULL;
}

static ble_sts_t app_addGattcMsg(queue_t *queue, app_gatt_msg_t *msg)
{
    app_gatt_msg_t *elem = (app_gatt_msg_t *)queue->head;
    app_gatt_msg_t *prev = NULL;

    while (elem != NULL) {
        if (elem->state != APP_MSG_ANALYSIS && msg->type < elem->type) {
            break;
        }
        prev = elem;
        elem = elem->pNext;
    }

    queue_insert(queue, msg, prev);

    return BLE_SUCCESS;
}

static ble_sts_t app_dealGattcMsg(app_gatt_msg_t *msg)
{
    if (!msg->state) {
        return GAP_ERR_INVALID_PARAMETER;
    }
    ble_sts_t status = BLE_SUCCESS;
    //  if(msg->type == GATTC_MSG_DISCOVERY_PRIMARY_SERVICE)
    {
        status = blc_gattc_discovery(msg->connHandle, &msg->sdpCfg);
    }
    if (status == BLE_SUCCESS) {
        msg->state = APP_MSG_ANALYSIS;
    }
    return status;
}

static app_gatt_msg_t *app_releaseGattcMsg(u16 connHandle)
{
    app_gatt_msg_t *msg = (app_gatt_msg_t *)queue_deq(&gattcMsgQueue);

    msg->state = APP_MSG_NULL;

    return msg;
}

u8 app_disocvery_char_uuid_cb(u16 connHandle, gatt_attr_t *attr, struct gattc_sdp_cfg *params)
{
    if (attr == NULL) {
        app_releaseGattcMsg(connHandle);
        return GATT_PROC_END;
    }

    gatt_chrc_t *characteristic = (gatt_chrc_t *)attr->user_data;
    tlkapi_printf(APP_GATTC_LOG_EN, "Found characteristic , attHandle is %x properties is %x, value Handle  is %x UUID is %s", characteristic->attrHdl, characteristic->properties, characteristic->valueHdl, hex_to_str(characteristic->uuid.uuidVal.u, characteristic->uuid.uuidLen));

    return GATT_PROC_CONT;
}

void app_disocvery_char_uuid(u16 connHandle, u16 startHandle, u16 endHandle)
{
    app_gatt_msg_t *msg = blt_app_getNewGattcMsg();

    msg->type       = GATTC_MSG_DISCOVERY_CHAR;
    msg->connHandle = connHandle;

    gattc_sdp_cfg_t *pSdpCfg = &msg->sdpCfg;
    pSdpCfg->type            = GATT_DISCOVER_CHARACTERISTIC;
    pSdpCfg->startHdl        = startHandle;
    pSdpCfg->endHdl          = endHandle;
    pSdpCfg->func            = app_disocvery_char_uuid_cb;
    app_addGattcMsg(&gattcMsgQueue, msg);
}

void app_disocvery_include_uuid(u16 connHandle, u16 startHandle, u16 endHandle);

u8 app_disocvery_include_uuid_cb(u16 connHandle, gatt_attr_t *attr, struct gattc_sdp_cfg *params)
{
    if (attr == NULL) {
        //      app_disocvery_char_uuid(connHandle, params->startHdl, params->endHdl);
        app_releaseGattcMsg(connHandle);
        return GATT_PROC_END;
    }

    tlkapi_printf(APP_GATTC_LOG_EN, "discovery include ending, startHandle is %x endingHanlde is %x", params->startHdl, params->endHdl);

    gatt_include_t *include = (gatt_include_t *)attr->user_data;
    tlkapi_printf(APP_GATTC_LOG_EN, "Found include ,startHandle is %x endingHanlde is %x UUID is %s", include->startHdl, include->endHdl, hex_to_str(include->uuid.uuidVal.u, include->uuid.uuidLen));
    app_disocvery_include_uuid(connHandle, include->startHdl, include->endHdl);
    return GATT_PROC_CONT;
}

void app_disocvery_include_uuid(u16 connHandle, u16 startHandle, u16 endHandle)
{
    app_gatt_msg_t *msg = blt_app_getNewGattcMsg();

    msg->type       = GATTC_MSG_DISCOVERY_INCLUDE_UUID;
    msg->connHandle = connHandle;

    gattc_sdp_cfg_t *pSdpCfg = &msg->sdpCfg;
    pSdpCfg->type            = GATT_DISCOVER_INCLUDE;
    pSdpCfg->startHdl        = startHandle;
    pSdpCfg->endHdl          = endHandle;
    pSdpCfg->func            = app_disocvery_include_uuid_cb;
    app_addGattcMsg(&gattcMsgQueue, msg);
}

u8 app_discovery_primary_service_cb(u16 connHandle, gatt_attr_t *attr, struct gattc_sdp_cfg *params)
{
    if (attr == NULL) {
        app_releaseGattcMsg(connHandle);
        return GATT_PROC_END;
    }
    gatt_service_val_t *prim_service = (gatt_service_val_t *)attr->user_data;

    app_disocvery_include_uuid(connHandle, attr->handle, prim_service->endHdl);

    tlkapi_printf(APP_GATTC_LOG_EN, "discovery primary service, startHandle is %x endingHanlde is %x UUID is %s", attr->handle, prim_service->endHdl, hex_to_str(prim_service->uuid->uuidVal.u, prim_service->uuid->uuidLen));

    return GATT_PROC_CONT;
}

void app_discovery_init(u16 connHandle)
{
    //  return ;
    memset(gAppMsg, 0, sizeof(gAppMsg));

    app_gatt_msg_t *msg = blt_app_getNewGattcMsg();

    msg->type       = GATTC_MSG_DISCOVERY_PRIMARY_SERVICE;
    msg->connHandle = connHandle;

    gattc_sdp_cfg_t *pSdpCfg = &msg->sdpCfg;
    pSdpCfg->type            = GATT_DISCOVER_PRIMARY;
    pSdpCfg->startHdl        = 0x0001;
    pSdpCfg->endHdl          = 0xFFFF;
    pSdpCfg->func            = app_discovery_primary_service_cb;
    app_addGattcMsg(&gattcMsgQueue, msg);

    tlkapi_printf(1, "%x %s", gattcMsgQueue.head, hex_to_str(gattcMsgQueue.head, sizeof(app_gatt_msg_t)));
}

void app_discovery_loop(void)
{
    app_gatt_msg_t *msg = (app_gatt_msg_t *)gattcMsgQueue.head;

    //  tlkapi_printf(1, "%d %x %s", msg->type, gattcMsgQueue.head, hex_to_str(gattcMsgQueue.head, sizeof(app_gatt_msg_t)));

    if (msg && msg->state == APP_MSG_STORED) {
        //      tlkapi_printf(1, "msg is %s", hex_to_str(msg, sizeof(app_gatt_msg_t)));
        app_dealGattcMsg(msg);
    }
}

/*******************GATT client discovery all character end**************************/

#endif //#if (INTER_TEST_MODE == TEST_HOST_BQB)
