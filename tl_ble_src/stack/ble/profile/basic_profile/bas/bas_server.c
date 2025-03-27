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

#include "bas_internal.h"
#include "bas_server_buf.h"

static int blt_bass_init(u8 initType, const void* param);
static void blt_bass_serviceInit(const struct blc_bass_regParam* param);
static void blt_bass_setBatteryLevel(u8 batteryLevel);
static void blt_bass_setBatteryPowerState(u8 batteryPowerState);

_attribute_ble_data_retention_
struct blc_bas_server_ctrl bas_server_ctrl = {
    .process = {
        .pNext = NULL,
        .id = BAS_SERVER,
        .usedAclRole = 0,
        .init = blt_bass_init,
        .connect = NULL,
        .discov = NULL,
        .loop = NULL,
    },
};

void blc_basic_registerBASControlServer(const struct blc_bass_regParam *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t*)&bas_server_ctrl, param);
}

static int blt_bass_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_bas_server_ctrl)), blc_bas_server_ctrl);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_bas_server)), blc_bas_server);
#endif

    if(initType == PRF_PROC_INIT) {
        BLT_BAS_LOG("Server init");
        blc_svc_addBasGroup();
        blt_bass_serviceInit(param);
    }
//  else if (initType == PRF_PROC_DEINIT) {
//      blc_svc_removeBasGroup();
//      BLT_BAS_LOG("Server deinit");
//  }
    return 0;
}

static struct blc_bas_server* blt_bass_getCtrl(u16 connHandle)
{
    (void)connHandle;
    return &bas_server_ctrl.basServer;
}

#define BASS_BATTERY_LEVEL_HANDLE(connHandle)               (blt_bass_getCtrl(connHandle)->batteryLevelHdl)
#define BASS_BATTERY_POWER_STATE_HANDLE(connHandle)         (blt_bass_getCtrl(connHandle)->batteryPowerStateHdl)

/******************BAS server init all characteristic handle*************************/
BLT_BAS_SERVER_INIT_HANDLE(batteryLevel)
BLT_BAS_SERVER_INIT_HANDLE(batteryPowerState)

static const atts_findCharList_t bassChar[] = {
    BLT_BAS_SERVER_FIND_CHAR(batteryLevel, characteristicBatteryLevelUuid),
    BLT_BAS_SERVER_FIND_CHAR(batteryPowerState, characteristicBatteryPowerStateUuid),
};

const struct blc_bass_regParam defaultBasParam = {
    .batteryLevel = 100,
    .powerState = DEVICE_NO_CHARGING,
};

static void blt_bass_serviceInit(const struct blc_bass_regParam* param)
{
    struct blc_bas_server *server = blt_bass_getCtrl(PRF_RFU_CONN_HANDLE);
    blc_atts_findCharacteristicByServiceUuid(serviceBatteryUuid, ATT_16_UUID_LEN, bassChar, ARRAY_SIZE(bassChar), server);
    BLT_BAS_LOG("Handle information, Battery Level:0x%x, Battery Power State:0x%x", server->batteryLevelHdl, server->batteryPowerStateHdl);

    const struct blc_bass_regParam* basParam = param? param: &defaultBasParam;
    blt_bass_setBatteryLevel(basParam->batteryLevel);
    blt_bass_setBatteryPowerState(basParam->powerState);
}

/****************BAS server init all characteristic handle end***********************/

static u8* blt_bass_getBatteryLevel(u16 connHandle)
{
    return blc_gatts_getAttributeValueByHandle(connHandle, BASS_BATTERY_LEVEL_HANDLE(connHandle));
}

static void blt_bass_setBatteryLevel(u8 batteryLevel)
{
    if(!CHECK_BATTERY_LEVEL(batteryLevel))  return ;

    u8 *pBatteryLevel = blt_bass_getBatteryLevel(PRF_RFU_CONN_HANDLE);
    if(!pBatteryLevel)      return ;

    *pBatteryLevel = batteryLevel;
}

int blc_bass_updateBatteryLevel(u16 connHandle, u8 batteryLevel)
{
    blt_bass_setBatteryLevel(batteryLevel);
    return blc_gatts_notifyAttr(connHandle, BASS_BATTERY_LEVEL_HANDLE(connHandle));
}

u8 blc_bass_getBatteryLevel(void)
{
    return *blt_bass_getBatteryLevel(PRF_RFU_CONN_HANDLE);
}

static u8* blt_bass_getBatteryPowerState(u16 connHandle)
{
    return blc_gatts_getAttributeValueByHandle(connHandle, BASS_BATTERY_POWER_STATE_HANDLE(connHandle));
}

static void blt_bass_setBatteryPowerState(u8 batteryPowerState)
{
    u8 *pBatteryPowerState = blt_bass_getBatteryPowerState(PRF_RFU_CONN_HANDLE);
    if(!pBatteryPowerState)     return ;

    *pBatteryPowerState = batteryPowerState;
}

int blc_bass_updateBatteryPowerState(u16 connHandle, u8 batteryPowerState)
{
    blt_bass_setBatteryPowerState(batteryPowerState);
    return blc_gatts_notifyAttr(connHandle, BASS_BATTERY_POWER_STATE_HANDLE(connHandle));
}

u8 blc_bass_getBatteryPowerState(void)
{
    return *blt_bass_getBatteryPowerState(PRF_RFU_CONN_HANDLE);
}
