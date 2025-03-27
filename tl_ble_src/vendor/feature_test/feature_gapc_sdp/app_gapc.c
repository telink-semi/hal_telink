/********************************************************************************************************
 * @file    app_gapc.c
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


#if (FEATURE_TEST_MODE == TEST_GAPC_SDP)

typedef struct{
    u16 deviceNameLen;
    u8 deviceName[30];
    u8 batteryLevel;
    u16 appearance;
    u8 ceneralAddress;
} app_gap_info_t;

static app_gap_info_t app_gap_info;

static void app_gap_initGAPServiceSdp(u16 connHandle);
static void app_gap_initGapReconn(u16 connHandle);
static void app_gap_initVcpSdp(u16 connHandle);
static void app_gap_initVcpReconnect(u16 connHandle);

void app_gap_init(u16 connHandle)
{
    BLT_APP_LOG("");
    app_gap_initGAPServiceSdp(connHandle);
}

/**********************GAP Service SDP discovery start***************/
static const blc_gapc_discList_t discGap;

static void app_gap_initGAPServiceSdp(u16 connHandle)
{
    blc_gapc_registerDiscoveryService(connHandle, &discGap);
}

static void app_gap_displayGapInfo(void)
{
    BLT_APP_LOG("GAP discovery ending");
    BLT_APP_STR_LOG("[APP]:device name", app_gap_info.deviceName, app_gap_info.deviceNameLen);
    BLT_APP_LOG("Battery level:%d", app_gap_info.batteryLevel);
}

static void app_gap_discGapServiceCb(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    if(count == 0)
    {
        app_gap_displayGapInfo();
        app_gap_initGapReconn(connHandle);
        return ;
    }
    BLT_APP_LOG("GAP found service info");

    BLT_APP_LOG("   -Attribute start handle is:0x%x", startHandle);
    BLT_APP_LOG("   -end handle is:0x%x ", endHandle);
}

static void app_gap_foundDeviceNameCharCb(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    BLT_APP_LOG("GAP found device name");
    BLT_APP_LOG("   -service num:0x%x properties:0x%x handle:0x%x", serviceCount, properties, valueHandle);
}

static void app_gap_deviceNameStartReadCb(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    *read = app_gap_info.deviceName;
    *readLen = &app_gap_info.deviceNameLen;
    *readMaxSize = sizeof(app_gap_info.deviceName);
}

static void app_gap_foundBatteryLevelCharCb(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    BLT_APP_LOG("GAP found battery level");
    BLT_APP_LOG("   -service num:0x%x properties:0x%x handle:0x%x", serviceCount, properties, valueHandle);
}

static void app_gap_foundBatteryLevelDescCb(u16 connHandle, uuid_t* uuid, u16 attrHandle)
{
    BLT_APP_STR_LOG("[APP}Battery level desc value is", uuid->uuidVal.u, uuid->uuidLen);
    BLT_APP_LOG("   -handle:0x%x", attrHandle);
}

static void app_gap_batteryLevelStartReadCb(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    *read = &app_gap_info.batteryLevel;
    *readLen = NULL;
    *readMaxSize = sizeof(app_gap_info.batteryLevel);
}

static void app_gap_foundUnknownCharCb(u16 connHandle, uuid_t* uuid, u8 properties, u16 valueHandle)
{
    BLT_APP_STR_LOG("[APP]GAP found unknown characteristic uuid", uuid->uuidVal.u, uuid->uuidLen);
    BLT_APP_LOG("   -properties:0x%x handle:0x%x", properties, valueHandle);
}


static const blc_gapc_discService_t disGapService = {
    .uuid = UUID16_INIT(SERVICE_UUID_GENERIC_ACCESS),
    .sfun = app_gap_discGapServiceCb,
};

static const blc_gapc_discChar_t disGapChar[] = {
    {
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_DEVICE_NAME),
        .cfun = app_gap_foundDeviceNameCharCb,
        .rfun = app_gap_deviceNameStartReadCb,
    },
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_BATTERY_LEVEL),
        .cfun = app_gap_foundBatteryLevelCharCb,
        .dfun = app_gap_foundBatteryLevelDescCb,
        .rfun = app_gap_batteryLevelStartReadCb,
    },
};

static const blc_gapc_discList_t discGap = {
    .maxServiceCount = 1,
    .service = &disGapService,
    .includeTable = {
        .size = 0,
    },
    .characteristicTable = {
        .size = ARRAY_SIZE(disGapChar),
        .characteristic = disGapChar,
        .ufun = app_gap_foundUnknownCharCb,
    },
};
/**********************GAP Service SDP discovery end***************/

/**********************GAP Service reconnect start***************/
static const blc_gapc_reconnList_t reconnGap;


static void app_gap_initGapReconn(u16 connHandle)
{
    blc_gapc_registerReconnectService(connHandle, &reconnGap);
}


static int app_gap_deviceNameGetCharInfoCb(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    charInfo->properties = CHAR_PROP_READ;
    charInfo->valueHandle = 0x03;
    return 1;
}

static int app_gap_batteryLevelGetCharInfoCb(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    charInfo->properties = CHAR_PROP_READ;
    charInfo->valueHandle = 11;
    return 1;
}

bool app_gap_GapReconn(u16 connHandle, int count)
{
    if(count == 0)
    {
        app_gap_displayGapInfo();
        app_gap_initVcpSdp(connHandle);
        return true;
    }
    if(count > 1)
        return false;
    return true;
}

static const blc_gapc_reconnChar_t reGapChar[] = {
    {
        .ifun = app_gap_deviceNameGetCharInfoCb,
        .rfun = app_gap_deviceNameStartReadCb,
    },
    {
        .ifun = app_gap_batteryLevelGetCharInfoCb,
        .rfun = app_gap_batteryLevelStartReadCb,
    },
};

static const blc_gapc_reconnList_t reconnGap = {
    .resfun = app_gap_GapReconn,
    .charTb.size = ARRAY_SIZE(reGapChar),
    .charTb.characteristic = reGapChar,
    .inclSize = 0,
};


bool app_gap_gattReconn(u16 connHandle, int count)
{
    if(count == 0)
    {
        tlkapi_printf(1, "sdp gap reconnect finish");
        return true;
    }
    if(count > 1)
        return false;
    return true;
}

typedef struct{
    u8 volState[3];
    u8 volOffsetState[3];
} app_vcp_info_t;

static app_vcp_info_t app_vcp_info;

/*************************VCP service SDP discovery start******************************/
static const blc_gapc_discList_t discVcp;

static void app_gap_initVcpSdp(u16 connHandle)
{
    blc_gapc_registerDiscoveryService(connHandle, &discVcp);
}

static void app_gap_displayVcsInfo(void)
{
    BLT_APP_LOG("[APP]:VCP discovery ending");
    BLT_APP_LOG("[APP]:volume state is:%d", bstream_to_u24_le(app_vcp_info.volState));
    BLT_APP_LOG("[APP]:volume offset state is ", bstream_to_u24_le(app_vcp_info.volOffsetState));
}

static void app_gap_discVcsServiceCb(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    if(count == 0)
    {
        app_gap_displayVcsInfo();
        app_gap_initVcpReconnect(connHandle);
        return ;
    }
    BLT_APP_LOG("VCP found VCS service info");

    BLT_APP_LOG("   -Attribute start handle is:0x%x", startHandle);
    BLT_APP_LOG("   -end handle is:0x%x", endHandle);
}

static void app_vcs_foundVolumeStateCharCb(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    BLT_APP_LOG("VCS found Volume State");
    BLT_APP_LOG("   -service num:0x%x properties:0x%x handle:0x%x", serviceCount, properties, valueHandle);
}

static void app_vocs_foundVolOffsetStateCharCb(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    BLT_APP_LOG("VOCS found Volume Offset State");
    BLT_APP_LOG("   -service num:0x%x properties:0x%x handle:0x%x", serviceCount, properties, valueHandle);
}

static void app_vcs_volumeStateStartReadCb(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    *read = app_vcp_info.volState;
    *readLen = NULL;
    *readMaxSize = sizeof(app_vcp_info.volState);
}

static void app_vocs_volOffsetStateStartReadCb(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    *read = app_vcp_info.volOffsetState;
    *readLen = NULL;
    *readMaxSize = sizeof(app_vcp_info.volOffsetState);
}

static void app_vcs_foundUnknownIncludeCb(u16 connHandle, uuid_t* uuid, u16 startHandle, u16 endHandle)
{
    BLT_APP_STR_LOG("[APP]VCP found unknown include uuid", uuid->uuidVal.u, uuid->uuidLen);
    BLT_APP_LOG("   -startHandle:0x%x endHandle:0x%x", startHandle, endHandle);
}

static void app_vcs_foundUnknownCharCb(u16 connHandle, uuid_t* uuid, u8 properties, u16 valueHandle)
{
    BLT_APP_STR_LOG("[APP]VCS found unknown characteristic uuid", uuid->uuidVal.u, uuid->uuidLen);
    BLT_APP_LOG("   -properties:0x%x handle:0x%x", properties, valueHandle);
}

static void app_vocs_foundUnknownCharCb(u16 connHandle, uuid_t* uuid, u8 properties, u16 valueHandle)
{
    BLT_APP_STR_LOG("[APP]VOCS found unknown characteristic uuid", uuid->uuidVal.u, uuid->uuidLen);
    BLT_APP_LOG("   -properties:0x%x handle:0x%x", properties, valueHandle);
}

static bool app_vocs_foundService(u16 connHandle, u16 startHandle, u16 endHandle)
{
    static int count = 0;
    if(count)
        return false;
    count++;
    BLT_APP_LOG("VOCS found service");
    BLT_APP_LOG("   -startHandle:0x%x endHandle:0x%x", startHandle, endHandle);
    return true;
}

static const blc_gapc_discService_t discVcsService = {
    .uuid = UUID16_INIT(SERVICE_UUID_VOLUME_CONTROL),
    .sfun = app_gap_discVcsServiceCb,
};

static const blc_gapc_discChar_t discVcpInclVocs[] = {
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_VOLUME_OFFSET_STATE),
        .cfun = app_vocs_foundVolOffsetStateCharCb,
        .rfun = app_vocs_volOffsetStateStartReadCb,
    },
};

static const blc_gapc_discInclude_t discVcpIncl = {
    .uuid = UUID16_INIT(SERVICE_UUID_VOLUME_OFFSET_CONTROL),
    .characteristic = {
        .size = ARRAY_SIZE(discVcpInclVocs),
        .characteristic = discVcpInclVocs,
        .ufun = app_vocs_foundUnknownCharCb,
    },
    .ifun = app_vocs_foundService,
};

static const blc_gapc_discChar_t discVcsChar[] = {
    {
        .readValue = true,
        .subscribeNtf = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_VOLUME_STATE),
        .cfun = app_vcs_foundVolumeStateCharCb,
        .rfun = app_vcs_volumeStateStartReadCb,
    },
};

static const blc_gapc_discList_t discVcp = {
    .maxServiceCount = 1,
    .service = &discVcsService,
    .includeTable = {
        .size = 1,
        .include[0] = &discVcpIncl,
        .uifun = app_vcs_foundUnknownIncludeCb,
    },
    .characteristicTable = {
        .size = ARRAY_SIZE(discVcsChar),
        .characteristic = discVcsChar,
        .ufun = app_vcs_foundUnknownCharCb,
    },
};


/*************************VCP service SDP discovery end******************************/
/*************************VCP service reconnect start******************************/
static const blc_gapc_reconnList_t reconnVcp;

static void app_gap_initVcpReconnect(u16 connHandle)
{
    blc_gapc_registerReconnectService(connHandle, &reconnVcp);
}

bool app_vcp_reconnCallBack(u16 connHandle, int count)
{
    if(count == 0)
    {
        app_gap_displayVcsInfo();
        return false;
    }
    if(count > 1)
        return false;
    return true;
}

int app_vcs_volumeStateInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    charInfo->properties = CHAR_PROP_READ;
    charInfo->valueHandle = 164;
    return 1;
}

int app_vocs_volOffsetStateInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    charInfo->properties = CHAR_PROP_READ;
    charInfo->valueHandle = 178;
    return 1;
}

bool app_vocs_reconnGetIncl(u16 connHandle, int count)
{
    if(count > 1)
    {
        return false;
    }

    return true;
}

static const blc_gapc_reconnChar_t reconnVcpChar[] = {
    {
        .ifun = app_vcs_volumeStateInfo,
        .rfun = app_vcs_volumeStateStartReadCb,
    },
};

static const blc_gapc_reconnChar_t reconnVocsChar[] = {
    {
        .ifun = app_vocs_volOffsetStateInfo,
        .rfun = app_vocs_volOffsetStateStartReadCb,
    },
};

static const blc_gapc_reconnInclTable_t vocsIncl = {
    .reifun = app_vocs_reconnGetIncl,
    .charTb.size = ARRAY_SIZE(reconnVocsChar),
    .charTb.characteristic = reconnVocsChar,
};

static const blc_gapc_reconnList_t reconnVcp = {
    .resfun = app_vcp_reconnCallBack,
    .charTb.size = ARRAY_SIZE(reconnVcpChar),
    .charTb.characteristic = reconnVcpChar,
    .inclSize = 1,
    .includeCharTb[0] = &vocsIncl,
};

/*************************VCP service reconnect end******************************/

#endif //end of (FEATURE_TEST_MODE == ...)
