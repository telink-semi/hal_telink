/********************************************************************************************************
 * @file    ots_client.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    04,2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#include "ots_internal.h"

#define UTC_DATE_TIME_SIZE      7
#define OACP_CONTROL_POINT_SIZE 21
#define OLCP_CONTROL_POINT_SIZE 7
#define OTS_OP_TIMEOUT_SEC      30

static const blc_gapc_discList_t         discOts;
static const blc_gapc_reconnList_t       reconnOts;
_attribute_ble_data_retention_ static u8 l2capOtsCocBuffer[COC_MODULE_BUFFER_SIZE(STACK_PRF_ACL_CONN_MAX_NUM, STACK_PRF_ACL_CONN_MAX_NUM, 0, OTSC_L2CAP_MTU)];

static int blt_otsc_init(u8 initType, const void *param);
static int blt_otsc_connect(u16 connHandle, prf_acl_state_enum connState);
static int blt_otsc_discovery(u16 connHandle);
static int blt_otsc_loop(u16 connHandle);
static int blt_otsc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param);
void       blt_otsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

_attribute_ble_data_retention_ /* retention TODO: */
    blc_ots_client_ctrl_t ots_client_ctrl = {
        .process =
            {
                      .pNext       = NULL,
                      .id          = ESL_OTS_CLIENT,
                      .usedAclRole = 0,
                      .init        = blt_otsc_init,
                      .connect     = blt_otsc_connect,
                      .discov      = blt_otsc_discovery,
                      .loop        = blt_otsc_loop,
                      .store       = blt_otsc_nv_store,
                      },
};

blc_otsc_t *blt_otsc_getClientInst(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle);

    return idx >= 0 ? ots_client_ctrl.pOtsClient[idx] : NULL;
}

static void blt_otsc_l2capInit(void)
{
    blc_coc_initParam_t regParam = {
        .MTU           = OTSC_L2CAP_MTU,
        .SPSM          = OTS_L2CAP_SPSM,
        .createConnCnt = STACK_PRF_ACL_CONN_MAX_NUM,
        .cocCidCnt     = STACK_PRF_ACL_CONN_MAX_NUM,
    };

    blc_l2cap_registerCocModule(&regParam, l2capOtsCocBuffer, sizeof(l2capOtsCocBuffer));
}

static int blt_otsc_init(u8 initType, const void *param)
{
    (void)param;
#if (0)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_otsc_t)), blc_otsc_t);
#endif

    if (initType == PRF_PROC_INIT) {
        BLT_OTS_LOG("Client init");
        blt_otsc_cleanBuf();

        for (int i = 0; i < STACK_PRF_ACL_CONN_MAX_NUM; i++) {
            ots_client_ctrl.pOtsClient[i] = blt_otsc_getClientBuf(i);

            memset(ots_client_ctrl.pOtsClient[i], 0, sizeof(*ots_client_ctrl.pOtsClient[i]));
            ots_client_ctrl.pOtsClient[i]->objectName      = blc_otsc_getObjectNameBuf(i);
            ots_client_ctrl.pOtsClient[i]->objectNameWrBuf = blc_otsc_getObjectNameWrBuf(i);
            ots_client_ctrl.pOtsClient[i]->objectType      = blc_otsc_getObjectTypeBuf(i);
            foreach_arr(j, ots_client_ctrl.pOtsClient[i]->objectFilter)
            {
                ots_client_ctrl.pOtsClient[i]->objectFilter[j] = blc_otsc_getObjectFilterBuf(i, j);
            }
            foreach_arr(j, ots_client_ctrl.pOtsClient[i]->objectFilterWrBuf)
            {
                ots_client_ctrl.pOtsClient[i]->objectFilterWrBuf[j] = blc_otsc_getObjectFilterWrBuf(i, j);
            }
        }

        blt_otsc_l2capInit();
    }
    //  else if (initType == PRF_PROC_DEINIT) {
    //      BLT_OTS_LOG("Client deinit");
    //  }

    return 0;
}

static int blt_otsc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if (connState == PRF_ACL_STATE_DISCONN) {
        blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

        BLT_OTS_LOG("Disconnect: 0x%x", connHandle);

        memset(client, 0, OFFSETOF(blc_otsc_t, objectName));
        client->objectName->len      = 0;
        client->objectNameWrBuf->len = 0;
        client->objectType->len      = 0;
        foreach_arr(i, client->objectFilter)
        {
            client->objectFilter[i]->len = 0;
        }
        foreach_arr(i, client->objectFilterWrBuf)
        {
            client->objectFilterWrBuf[i]->len = 0;
        }
    } else {
        BLT_OTS_LOG("Connect: 0x%x", connHandle);
    }

    return 0;
}

static int blt_otsc_discovery(u16 connHandle)
{
    if (blc_prf_checkDiscoveryBusy(connHandle)) {
        return 0;
    }

    if (blc_prf_checkReconnectFlag(connHandle)) {
        blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

        if (client->ntfInput.startHdl) {
            if (blc_gapc_registerReconnectService(connHandle, &reconnOts) == BLE_SUCCESS) {
                blc_prf_sendServiceDiscoveryFoundEvent(connHandle, ESL_OTS_CLIENT, client->ntfInput.startHdl, client->ntfInput.endHdl);
                blc_prf_setDiscoveryStatusBusy(connHandle);
            }
        } else {
            blc_prf_sendServiceDiscoveryFailEvent(connHandle, ESL_OTS_CLIENT);
            blc_prf_setDiscoveryStatusFinish(connHandle);
        }
    } else if (blc_gapc_registerDiscoveryService(connHandle, &discOts) == BLE_SUCCESS) {
        blc_prf_setDiscoveryStatusBusy(connHandle);
    }

    return 0;
}

static void blt_otsc_timerLoop(blt_otsc_timer_t *timer)
{
    if (!timer->active) {
        return;
    }

    if (!clock_time_exceed(timer->tick, 1000 * 1000)) {
        return;
    }

    timer->secRemaining--;
    timer->tick = clock_time();
    if (timer->secRemaining) {
        return;
    }

    timer->active = false;
    if (timer->cb) {
        timer->cb(timer->data);
    }
}

static void blt_otsc_loopOacpTimer(u16 connHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    blt_otsc_timerLoop(&client->oacpTimer);
}

static void blt_otsc_loopOlcpTimer(u16 connHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    blt_otsc_timerLoop(&client->olcpTimer);
}

static int blt_otsc_loop(u16 connHandle)
{
    blt_otsc_loopOacpTimer(connHandle);
    blt_otsc_loopOlcpTimer(connHandle);

    return 0;
}

static int blt_otsc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (nvState == PRF_NV_STATE_STORE) {
        if (client && client->ntfInput.startHdl) {
            blt_ots_nv_info_t nvInfo;

            blt_prf_storeClientHdl(&nvInfo.att, client, &client->otsObjectChangedCccHdl);
            nvInfo.otsObjectFirstCreatedProperties = client->otsObjectFirstCreatedProperties;
            nvInfo.otsObjectLastModifiedProperties = client->otsObjectLastModifiedProperties;
            nvInfo.otsObjectNameProperties         = client->otsObjectNameProperties;
            nvInfo.otsObjectPropertiesProperties   = client->otsObjectPropertiesProperties;
            nvInfo.objectListFilterCnt             = client->objectListFilterCnt;
            U8_TO_STREAM(param->dataPtr, sizeof(blt_ots_nv_info_t));
            U8_TO_STREAM(param->dataPtr, ESL_OTS_CLIENT);
            STR_TO_STREAM(param->dataPtr, &nvInfo, sizeof(blt_ots_nv_info_t));
            param->currentTotalLen += 2 + sizeof(blt_ots_nv_info_t);
        }
    } else if (nvState == PRF_NV_STATE_LOAD) {
        blt_ots_nv_info_t *nvInfo = (blt_ots_nv_info_t *)param->dataPtr;

        blt_prf_loadClientHdl(client, &nvInfo->att, &client->otsObjectChangedCccHdl);
        client->otsObjectFirstCreatedProperties = nvInfo->otsObjectFirstCreatedProperties;
        client->otsObjectLastModifiedProperties = nvInfo->otsObjectLastModifiedProperties;
        client->otsObjectNameProperties         = nvInfo->otsObjectNameProperties;
        client->otsObjectPropertiesProperties   = nvInfo->otsObjectPropertiesProperties;
        client->objectListFilterCnt             = nvInfo->objectListFilterCnt;
        client->ntfInput.ntfOrIndFunc           = blt_otsc_dataInput;
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
    }

    return 0;
}

void blc_esl_registerOTSControlClient(const blc_otsc_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t *)&ots_client_ctrl, param);
}

static void blt_otsc_timerReload(blt_otsc_timer_t *timer, u8 secTimeout, blt_otsc_timer_cb_t cb, void *data)
{
    timer->active       = true;
    timer->secRemaining = secTimeout;
    timer->cb           = cb;
    timer->data         = data;
    timer->tick         = clock_time();
}

static void blt_otsc_timerStop(blt_otsc_timer_t *timer)
{
    memset(timer, 0, sizeof(*timer));
}

static void blt_otsc_oacpTimeoutExpired(void *data)
{
    u16         connHandle = (unsigned int)(uintptr_t)data;
    blc_otsc_t *client     = blt_otsc_getClientInst(connHandle);

    client->oacpIndInProgress = false;

    blt_prf_sendEvent(connHandle, ESL_EVT_OTSC_OBJECT_ACTION_CONTROL_POINT_TIMEOUT_EVT, NULL, 0);
}

static void blt_otsc_olcpTimeoutExpired(void *data)
{
    u16         connHandle = (unsigned int)(uintptr_t)data;
    blc_otsc_t *client     = blt_otsc_getClientInst(connHandle);

    client->olcpIndInProgress = false;

    blt_prf_sendEvent(connHandle, ESL_EVT_OTSC_OBJECT_LIST_CONTROL_POINT_TIMEOUT_EVT, NULL, 0);
}

static void blt_otsc_startOacpTimer(u16 connHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    blt_otsc_timerReload(&client->oacpTimer, OTS_OP_TIMEOUT_SEC, blt_otsc_oacpTimeoutExpired, (void *)((uintptr_t)connHandle));
}

static void blt_otsc_startOlcpTimer(u16 connHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    blt_otsc_timerReload(&client->olcpTimer, OTS_OP_TIMEOUT_SEC, blt_otsc_olcpTimeoutExpired, (void *)((uintptr_t)connHandle));
}

static void blt_otsc_stopOacpTimer(u16 connHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    blt_otsc_timerStop(&client->oacpTimer);
}

static void blt_otsc_stopOlcpTimer(u16 connHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    blt_otsc_timerStop(&client->olcpTimer);
}

static void blt_oscp_oacpIndication(u16 connHandle, u8 *val, u16 valLen)
{
    u8                                      buf[OACP_CONTROL_POINT_SIZE];
    blc_otsc_objectActionControlPointEvt_t *evt    = (blc_otsc_objectActionControlPointEvt_t *)buf;
    blc_otsc_t                             *client = blt_otsc_getClientInst(connHandle);

    blt_otsc_stopOacpTimer(connHandle);
    client->oacpIndInProgress = false;

    if (valLen < 3 || valLen > sizeof(buf)) {
        BLT_OTS_LOG("Invalid length of OACP indication");
        return;
    }

    STREAM_TO_U8(evt->rsp[0].opCode, val);
    STREAM_TO_U8(evt->rsp[0].requestedOpCode, val);
    STREAM_TO_U8(evt->rsp[0].resultCode, val);
    memcpy(evt->rsp[0].params, val, valLen - 3);

    blt_prf_sendEvent(connHandle, ESL_EVT_OTSC_OBJECT_ACTION_CONTROL_POINT_EVT, buf, sizeof(*evt) + (valLen - 3));
}

static void blt_oscp_olcpIndication(u16 connHandle, u8 *val, u16 valLen)
{
    u8                                    buf[OLCP_CONTROL_POINT_SIZE];
    blc_otsc_objectListControlPointEvt_t *evt    = (blc_otsc_objectListControlPointEvt_t *)buf;
    blc_otsc_t                           *client = blt_otsc_getClientInst(connHandle);

    blt_otsc_stopOlcpTimer(connHandle);
    client->olcpIndInProgress = false;

    if (valLen < 3 || valLen > sizeof(buf)) {
        BLT_OTS_LOG("Invalid length of OLCP indication");
        return;
    }

    STREAM_TO_U8(evt->rsp[0].opCode, val);
    STREAM_TO_U8(evt->rsp[0].requestedOpCode, val);
    STREAM_TO_U8(evt->rsp[0].resultCode, val);
    memcpy(evt->rsp[0].params, val, valLen - 3);

    blt_prf_sendEvent(connHandle, ESL_EVT_OTSC_OBJECT_LIST_CONTROL_POINT_EVT, buf, sizeof(*evt) + (valLen - 3));
}

static void blt_oscp_objectChangedIndication(u16 connHandle, u8 *val, u16 valLen)
{
    blc_otsc_objectChangedEvt_t evt;

    if (valLen != ((sizeof(evt.flags) + sizeof(evt.id)))) {
        BLT_OTS_LOG("Invalid length of Object Changed indication");
        return;
    }

    STREAM_TO_U8(evt.flags, val);
    memcpy(evt.id.objectId, val, sizeof(evt.id.objectId));

    blt_prf_sendEvent(connHandle, ESL_EVT_OTSC_OBJECT_CHANGED_EVT, (u8 *)&evt, sizeof(evt));
}

void blt_otsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (attHdl == client->otsObjectActionControlPointHdl) {
        blt_oscp_oacpIndication(connHandle, val, valLen);
    } else if (attHdl == client->otsObjectListControlPointHdl) {
        blt_oscp_olcpIndication(connHandle, val, valLen);
    } else if (attHdl == client->otsObjectChangedHdl) {
        blt_oscp_objectChangedIndication(connHandle, val, valLen);
    }
}

static void blt_otsc_displayInfo(u16 connHandle, blc_otsc_t *client)
{
    BLT_OTS_LOG("OTS sdp over connHandle: 0x%x, start handle 0x%04X, end handle 0x%04X", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
    BLT_OTS_LOG("OTS Feature: 0x%04x Object Name: 0x%04x", client->otsFeatureHdl, client->otsObjectNameHdl);
    BLT_OTS_LOG("Object Type: 0x%04x Object Size: 0x%04x", client->otsObjectTypeHdl, client->otsObjectSizeHdl);
    BLT_OTS_LOG("Object First Created: 0x%04x Object Last Modified: 0x%04x", client->otsObjectFirstCreatedHdl, client->otsObjectLastModifiedHdl);
    BLT_OTS_LOG("Object Id: 0x%04x Object Properties: 0x%04x", client->otsObjectIdHdl, client->otsObjectPropertiesHdl);
    BLT_OTS_LOG("OACP: 0x%04x OACP CCC: 0x%04x", client->otsObjectActionControlPointHdl, client->otsObjectActionControlPointCccHdl);
    BLT_OTS_LOG("OLCP: 0x%04x OLCP CCC: 0x%04x", client->otsObjectListControlPointHdl, client->otsObjectListControlPointCccHdl);
    BLT_OTS_LOG("Object List Filter: [0]:0x%04x [1]:0x%04x [2]:0x%04x", client->otsObjectListFilterHdl[0], client->otsObjectListFilterHdl[1], client->otsObjectListFilterHdl[2]);
    BLT_OTS_LOG("Object Changed: 0x%04x Object Changed CCC: 0x%04x", client->otsObjectChangedHdl, client->otsObjectChangedCccHdl);
}

static void blt_otsc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (count == 0xFF) {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, ESL_OTS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_OTS_LOG("ERR:not found OTS");
        return;
    }

    if (count == 0) {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, ESL_OTS_CLIENT);
        blt_otsc_displayInfo(connHandle, client);
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return;
    }

    client->ntfInput.startHdl     = startHandle;
    client->ntfInput.endHdl       = endHandle;
    client->ntfInput.ntfOrIndFunc = blt_otsc_dataInput;
    BLT_OTS_LOG("INFO: OTS connHandle: 0x%x, startHandle: 0x%x, EndHandle: 0x%x", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, ESL_OTS_CLIENT, startHandle, endHandle);
}

static const blc_gapc_discService_t otsService = {
    .uuid = UUID16_INIT(SERVICE_UUID_OBJECT_TRANSFER),
    .sfun = blt_otsc_foundService,
};

static void blt_otsc_foundOtsFeatureChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_READ) {
        client->otsFeatureHdl = valueHandle;
    }

    BLT_OTS_LOG("OTS Feature connHandle: 0x%x, properties: 0x%x, value: 0x%04x", connHandle, properties, valueHandle);
}

static void blt_otsc_foundObjectNameChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_READ) {
        client->otsObjectNameHdl        = valueHandle;
        client->otsObjectNameProperties = properties;
    }

    BLT_OTS_LOG("Object Name connHandle: 0x%x, properties: 0x%x, value: 0x%04x", connHandle, properties, valueHandle);
}

static void blt_otsc_foundObjectTypeChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_READ) {
        client->otsObjectTypeHdl = valueHandle;
    }

    BLT_OTS_LOG("Object Type connHandle: 0x%x, properties: 0x%x, value: 0x%04x", connHandle, properties, valueHandle);
}

static void blt_otsc_foundObjectSizeChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_READ) {
        client->otsObjectSizeHdl = valueHandle;
    }

    BLT_OTS_LOG("Object Size connHandle: 0x%x, properties: 0x%x, value: 0x%04x", connHandle, properties, valueHandle);
}

static void blt_otsc_foundObjectFirstCreatedChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_READ) {
        client->otsObjectFirstCreatedHdl        = valueHandle;
        client->otsObjectFirstCreatedProperties = properties;
    }

    BLT_OTS_LOG("Object First Created connHandle: 0x%x, properties: 0x%x, value: 0x%04x", connHandle, properties, valueHandle);
}

static void blt_otsc_foundObjectLastModifiedChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_READ) {
        client->otsObjectLastModifiedHdl        = valueHandle;
        client->otsObjectLastModifiedProperties = properties;
    }

    BLT_OTS_LOG("Object Last Modified connHandle: 0x%x, properties: 0x%x, value: 0x%04x", connHandle, properties, valueHandle);
}

static void blt_otsc_foundObjectIdChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_READ) {
        client->otsObjectIdHdl = valueHandle;
    }

    BLT_OTS_LOG("Object ID connHandle: 0x%x, properties: 0x%x, value: 0x%04x", connHandle, properties, valueHandle);
}

static void blt_otsc_foundObjectPropertiesChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_READ) {
        client->otsObjectPropertiesHdl        = valueHandle;
        client->otsObjectPropertiesProperties = properties;
    }

    BLT_OTS_LOG("Object Properties connHandle: 0x%x, properties: 0x%x, value: 0x%04x", connHandle, properties, valueHandle);
}

static void blt_otsc_foundObjectActionControlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)serviceCount;

    if ((properties & CHAR_PROP_WRITE) && (properties & CHAR_PROP_INDICATE)) {
        client->otsObjectActionControlPointHdl = valueHandle;
    }

    BLT_OTS_LOG("Object Action Control Point connHandle: 0x%x, properties: 0x%x, value: 0x%04x", connHandle, properties, valueHandle);
}

static void blt_otsc_foundObjectListControlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)serviceCount;

    if ((properties & CHAR_PROP_WRITE) && (properties & CHAR_PROP_INDICATE)) {
        client->otsObjectListControlPointHdl = valueHandle;
    }

    BLT_OTS_LOG("Object List Control Point connHandle: 0x%x, properties: 0x%x, value: 0x%04x", connHandle, properties, valueHandle);
}

static void blt_otsc_foundObjectListFilterChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)serviceCount;

    if ((properties & CHAR_PROP_WRITE) && (properties & CHAR_PROP_READ) &&
        (client->objectListFilterCnt <= (sizeof(client->otsObjectListFilterHdl) / sizeof(client->otsObjectListFilterHdl)[0]))) {
        client->otsObjectListFilterHdl[client->objectListFilterCnt] = valueHandle;

        BLT_OTS_LOG("Object List Filter[%d] connHandle: 0x%x, properties: 0x%x, value: 0x%04x", client->objectListFilterCnt, connHandle, properties, valueHandle);

        client->objectListFilterCnt++;
    }
}

static void blt_otsc_foundObjectChangedChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)serviceCount;

    if ((properties & CHAR_PROP_INDICATE)) {
        client->otsObjectChangedHdl = valueHandle;
    }

    BLT_OTS_LOG("Object Changed connHandle: 0x%x, properties: 0x%x, value: 0x%04x", connHandle, properties, valueHandle);
}
#if BLC_OTSC_DISCOVERY_READ_ATTRS
static void blt_otsc_readCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    u16 handle = pRdCfg->single.handle;

    (void)connHandle;

    if (err) {
        if (err == GATT_ERR_DATA_LENGTH_EXCEED_MEM_RESTRICTION) {
            BLT_OTS_LOG("Can't read value due to memory restrictions, handle 0x%04X", handle);
        } else {
            BLT_OTS_LOG("Read handle: 0x%04x err: 0x%02x", handle, err);
        }
    }
}

static void blt_otsc_readOtsFeatureChar(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)attrHandle;

    *read        = client->otsFeature;
    *readLen     = &client->otsFeatureSize;
    *readMaxSize = sizeof(client->otsFeature);
    *rdCbFunc    = blt_otsc_readCb;
}

static void blt_otsc_readObjectNameChar(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)attrHandle;

    *read        = client->objectName->val;
    *readLen     = &client->objectName->len;
    *readMaxSize = gAppOtscObjectNameMaxSize;
    *rdCbFunc    = blt_otsc_readCb;
}

static void blt_otsc_readObjectTypeChar(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)attrHandle;

    *read        = client->objectType->val;
    *readLen     = &client->objectType->len;
    *readMaxSize = gAppOtscObjectTypeMaxSize;
    *rdCbFunc    = blt_otsc_readCb;
}

static void blt_otsc_readObjectSizeChar(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)attrHandle;

    *read        = client->objectSize;
    *readLen     = &client->objectSizeLen;
    *readMaxSize = sizeof(client->objectSize);
    *rdCbFunc    = blt_otsc_readCb;
}

static void blt_otsc_readObjectFirstCreatedChar(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)attrHandle;

    *read        = client->objectFirstCreated;
    *readLen     = &client->objectFirstCreatedLen;
    *readMaxSize = sizeof(client->objectFirstCreated);
    *rdCbFunc    = blt_otsc_readCb;
}

static void blt_otsc_readObjectLastModifiedChar(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)attrHandle;

    *read        = client->objectLastModified;
    *readLen     = &client->objectLastModifiedLen;
    *readMaxSize = sizeof(client->objectLastModified);
    *rdCbFunc    = blt_otsc_readCb;
}

static void blt_otsc_readObjectIdChar(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)attrHandle;

    *read        = client->objectId;
    *readLen     = &client->objectIdLen;
    *readMaxSize = sizeof(client->objectId);
    *rdCbFunc    = blt_otsc_readCb;
}

static void blt_otsc_readObjectPropertiesChar(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)attrHandle;

    *read        = client->objectProperties;
    *readLen     = &client->objectPropertiesLen;
    *readMaxSize = sizeof(client->objectProperties);
    *rdCbFunc    = blt_otsc_readCb;
}

static void blt_otsc_readObjectListFilterChar(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    blc_otsc_t           *client = blt_otsc_getClientInst(connHandle);
    otsClientCharValue_t *charValue;

    (void)attrHandle;

    if (client->objectListFilterIdx >= client->objectListFilterCnt) {
        BLT_OTS_LOG("Object Filter char error: 0x%04x", connHandle);
        return;
    }

    charValue    = client->objectFilter[client->objectListFilterIdx++];
    *read        = charValue->val;
    *readLen     = &charValue->len;
    *readMaxSize = gAppOtscObjectFilterMaxSize;
    *rdCbFunc    = blt_otsc_readCb;
}
#else
    #define blt_otsc_readObjectListFilterChar   NULL
    #define blt_otsc_readObjectPropertiesChar   NULL
    #define blt_otsc_readObjectIdChar           NULL
    #define blt_otsc_readObjectLastModifiedChar NULL
    #define blt_otsc_readObjectFirstCreatedChar NULL
    #define blt_otsc_readObjectSizeChar         NULL
    #define blt_otsc_readObjectTypeChar         NULL
    #define blt_otsc_readObjectNameChar         NULL
    #define blt_otsc_readOtsFeatureChar         NULL
#endif

static void blt_otsc_subscribedObjectActionControlPointCcc(u16 connHandle, u16 cccHandle, u8 result)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)result;

    client->otsObjectActionControlPointCccHdl = cccHandle;
}

static void blt_otsc_subscribedObjectListControlPointCcc(u16 connHandle, u16 cccHandle, u8 result)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)result;

    client->otsObjectListControlPointCccHdl = cccHandle;
}

static void blt_otsc_subscribedObjectChangedCcc(u16 connHandle, u16 cccHandle, u8 result)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)result;

    client->otsObjectChangedCccHdl = cccHandle;
}

static const blc_gapc_discChar_t otsChar[] = {
    {
        .readValue = true,
        .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OTS_FEATURE),
        .cfun      = blt_otsc_foundOtsFeatureChar,
        .rfun      = blt_otsc_readOtsFeatureChar,
    },
    {
        .readValue = true,
        .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_NAME),
        .cfun      = blt_otsc_foundObjectNameChar,
        .rfun      = blt_otsc_readObjectNameChar,
    },
    {
        .readValue = true,
        .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_TYPE),
        .cfun      = blt_otsc_foundObjectTypeChar,
        .rfun      = blt_otsc_readObjectTypeChar,
    },
    {
        .readValue = true,
        .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_SIZE),
        .cfun      = blt_otsc_foundObjectSizeChar,
        .rfun      = blt_otsc_readObjectSizeChar,
    },
    {
        .readValue = true,
        .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_FIRST_CREATED),
        .cfun      = blt_otsc_foundObjectFirstCreatedChar,
        .rfun      = blt_otsc_readObjectFirstCreatedChar,
    },
    {
        .readValue = true,
        .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_LAST_MODIFIED),
        .cfun      = blt_otsc_foundObjectLastModifiedChar,
        .rfun      = blt_otsc_readObjectLastModifiedChar,
    },
    {
        .readValue = true,
        .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_ID),
        .cfun      = blt_otsc_foundObjectIdChar,
        .rfun      = blt_otsc_readObjectIdChar,
    },
    {
        .readValue = true,
        .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_PROPERTIES),
        .cfun      = blt_otsc_foundObjectPropertiesChar,
        .rfun      = blt_otsc_readObjectPropertiesChar,
    },
    {
        .subscribeInd = true,
        .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_ACTION_CONTROL_POINT),
        .cfun         = blt_otsc_foundObjectActionControlPointChar,
        .scfun        = blt_otsc_subscribedObjectActionControlPointCcc,
    },
    {
        .subscribeInd = true,
        .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_LIST_CONTROL_POINT),
        .cfun         = blt_otsc_foundObjectListControlPointChar,
        .scfun        = blt_otsc_subscribedObjectListControlPointCcc,
    },
    {
        .readValue = true,
        .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_LIST_FILTER),
        .cfun      = blt_otsc_foundObjectListFilterChar,
        .rfun      = blt_otsc_readObjectListFilterChar,
    },
    {
        .subscribeInd = true,
        .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_CHANGED),
        .cfun         = blt_otsc_foundObjectChangedChar,
        .scfun        = blt_otsc_subscribedObjectChangedCcc,
    },
};

static const blc_gapc_discList_t discOts = {
    .maxServiceCount = 1,
    .service         = &otsService,
    .includeTable =
        {
                       .size = 0,
                       },
    .characteristicTable =
        {
                       .size           = ARRAY_SIZE(otsChar),
                       .characteristic = otsChar,
                       },
};

static bool blt_otsc_recService(u16 connHandle, int count)
{
    if (count == 0) {
        blc_otsc_t *client = blt_otsc_getClientInst(connHandle);
        blt_otsc_displayInfo(connHandle, client);
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, ESL_OTS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    return count <= 1;
}

static int blt_otsc_getOtsFeatureGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->otsFeatureHdl;

    return 1;
}

static int blt_otsc_getObjectNameGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    charInfo->properties  = client->otsObjectNameProperties;
    charInfo->valueHandle = client->otsObjectNameHdl;

    return 1;
}

static int blt_otsc_getObjectTypeGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->otsObjectTypeHdl;

    return 1;
}

static int blt_otsc_getObjectSizeGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->otsObjectSizeHdl;

    return 1;
}

static int blt_otsc_getObjectFirstCreatedGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    charInfo->properties  = client->otsObjectFirstCreatedProperties;
    charInfo->valueHandle = client->otsObjectFirstCreatedHdl;

    return 1;
}

static int blt_otsc_getObjectLastModifiedGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    charInfo->properties  = client->otsObjectLastModifiedProperties;
    charInfo->valueHandle = client->otsObjectLastModifiedHdl;

    return 1;
}

static int blt_otsc_getObjectIdInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->otsObjectIdHdl;

    return 1;
}

static int blt_otsc_getObjectPropertiesInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    charInfo->properties  = client->otsObjectPropertiesProperties;
    charInfo->valueHandle = client->otsObjectPropertiesHdl;

    return 1;
}

static int blt_otsc_getObjectActionControlPointInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    charInfo->properties  = (CHAR_PROP_WRITE | CHAR_PROP_INDICATE);
    charInfo->valueHandle = client->otsObjectActionControlPointHdl;
#if BLC_OTSC_WRITE_CCC_ON_RECONNECT
    charInfo->cccHandle = client->otsObjectActionControlPointCccHdl;
#else
    charInfo->cccHandle = 0;
#endif

    return 1;
}

static int blt_otsc_getObjectListControlPointInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    charInfo->properties  = (CHAR_PROP_WRITE | CHAR_PROP_INDICATE);
    charInfo->valueHandle = client->otsObjectListControlPointHdl;
#if BLC_OTSC_WRITE_CCC_ON_RECONNECT
    charInfo->cccHandle = client->otsObjectListControlPointCccHdl;
#else
    charInfo->valueHandle = 0;
#endif

    return 1;
}

static int blt_otsc_getObjectListFilterGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    for (u8 i = 0; i < client->objectListFilterCnt; i++) {
        charInfo->properties  = (CHAR_PROP_WRITE | CHAR_PROP_READ);
        charInfo->valueHandle = client->otsObjectListFilterHdl[i];
        charInfo++;
    }

    return client->objectListFilterCnt;
}

static int blt_otsc_getObjectChangedInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_INDICATE;
    charInfo->valueHandle = client->otsObjectChangedHdl;
#if BLC_OTSC_WRITE_CCC_ON_RECONNECT
    charInfo->cccHandle = client->otsObjectChangedCccHdl;
#else
    charInfo->cccHandle = 0;
#endif

    return 1;
}

static const blc_gapc_reconnChar_t reOtsChar[] = {
    {
        .ifun = blt_otsc_getOtsFeatureGetInfo,
        .rfun = blt_otsc_readOtsFeatureChar,
    },
    {
        .ifun = blt_otsc_getObjectNameGetInfo,
        .rfun = blt_otsc_readObjectNameChar,
    },
    {
        .ifun = blt_otsc_getObjectTypeGetInfo,
        .rfun = blt_otsc_readObjectTypeChar,
    },
    {
        .ifun = blt_otsc_getObjectSizeGetInfo,
        .rfun = blt_otsc_readObjectSizeChar,
    },
    {
        .ifun = blt_otsc_getObjectFirstCreatedGetInfo,
        .rfun = blt_otsc_readObjectFirstCreatedChar,
    },
    {
        .ifun = blt_otsc_getObjectLastModifiedGetInfo,
        .rfun = blt_otsc_readObjectLastModifiedChar,
    },
    {
        .ifun = blt_otsc_getObjectIdInfo,
        .rfun = blt_otsc_readObjectIdChar,
    },
    {
        .ifun = blt_otsc_getObjectPropertiesInfo,
        .rfun = blt_otsc_readObjectPropertiesChar,
    },
    {
        .ifun = blt_otsc_getObjectActionControlPointInfo,
    },
    {
        .ifun = blt_otsc_getObjectListControlPointInfo,
    },
    {
        .ifun = blt_otsc_getObjectListFilterGetInfo,
        .rfun = blt_otsc_readObjectListFilterChar,
    },
    {
        .ifun = blt_otsc_getObjectChangedInfo,
    },
};

static const blc_gapc_reconnList_t reconnOts = {
    .resfun = blt_otsc_recService,
    .charTb =
        {
                 .size           = ARRAY_SIZE(reOtsChar),
                 .characteristic = reOtsChar,
                 },
    .inclSize = 0,
};

ble_sts_t blc_otsc_getOtsFeature(u16 connHandle, blc_ots_feature_t *otsFeature)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);
    u8         *value;

    if (!client || !client->otsFeatureHdl || !otsFeature || client->otsFeatureSize != sizeof(*otsFeature)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    value = client->otsFeature;

    STREAM_TO_U32(otsFeature->oacpFeatures, value);
    STREAM_TO_U32(otsFeature->olcpFeatures, value);

    return BLE_SUCCESS;
}

ble_sts_t blc_otsc_getObjectName(u16 connHandle, u8 *name, u16 *length)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (!client || !client->otsObjectNameHdl || !name || !length) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *length = client->objectName->len;
    memcpy(name, client->objectName->val, client->objectName->len);

    return BLE_SUCCESS;
}

ble_sts_t blc_otsc_getObjectType(u16 connHandle, uuid_t *type)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (!client || !client->otsObjectTypeHdl || !type) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    switch (client->objectType->len) {
    case ATT_16_UUID_LEN:
        BYTE_TO_UINT16(type->uuidVal.u16, client->objectType->val);
        break;
    case ATT_128_UUID_LEN:
        memcpy(type->uuidVal.u128, client->objectType->val, sizeof(type->uuidVal.u128));
        break;
    default:
        return GATT_ERR_INVALID_PARAMETER;
    }

    type->uuidLen = client->objectType->len;

    return BLE_SUCCESS;
}

ble_sts_t blc_otsc_getObjectSize(u16 connHandle, blc_ots_object_size_t *size)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);
    u8         *value;

    if (!client || !client->otsObjectSizeHdl || !size || (client->objectSizeLen != sizeof(client->objectSize))) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    value = client->objectSize;
    STREAM_TO_U32(size->currentSize, value);
    STREAM_TO_U32(size->allocatedSize, value);

    return BLE_SUCCESS;
}

static ble_sts_t blt_otsc_parseDateTime(u8 *value, u16 len, blc_ots_utc_t *dateTime)
{
    if (UTC_DATE_TIME_SIZE != len) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    STREAM_TO_U16(dateTime->year, value);
    STREAM_TO_U8(dateTime->month, value);
    STREAM_TO_U8(dateTime->day, value);
    STREAM_TO_U8(dateTime->hour, value);
    STREAM_TO_U8(dateTime->minute, value);
    STREAM_TO_U8(dateTime->second, value);

    return BLE_SUCCESS;
}

static u16 blt_otsc_composeDateTime(u8 *value, blc_ots_utc_t *dateTime)
{
    u8 *ptr = value;

    U16_TO_STREAM(value, dateTime->year);
    U8_TO_STREAM(value, dateTime->month);
    U8_TO_STREAM(value, dateTime->day);
    U8_TO_STREAM(value, dateTime->hour);
    U8_TO_STREAM(value, dateTime->minute);
    U8_TO_STREAM(value, dateTime->second);

    return value - ptr;
}

ble_sts_t blc_otsc_getObjectFirstCreated(u16 connHandle, blc_ots_utc_t *dateTime)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (!client || !client->otsObjectFirstCreatedHdl || !dateTime) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    return blt_otsc_parseDateTime(client->objectFirstCreated, client->objectFirstCreatedLen, dateTime);
}

ble_sts_t blc_otsc_getObjectLastModified(u16 connHandle, blc_ots_utc_t *dateTime)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (!client || !client->otsObjectLastModifiedHdl || !dateTime) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    return blt_otsc_parseDateTime(client->objectFirstCreated, client->objectFirstCreatedLen, dateTime);
}

ble_sts_t blc_otsc_getObjectId(u16 connHandle, blc_ots_object_id_t *objectId)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (!client || !client->otsObjectIdHdl || !objectId || client->objectIdLen != sizeof(objectId->objectId)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    memcpy(objectId->objectId, client->objectId, client->objectIdLen);

    return BLE_SUCCESS;
}

ble_sts_t blc_otsc_getObjectProperties(u16 connHandle, u32 *properties)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (!client || !client->otsObjectIdHdl || !properties || client->objectPropertiesLen != sizeof(client->objectProperties)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    BYTES_TO_UINT32(*properties, client->objectProperties);

    return BLE_SUCCESS;
}

static ble_sts_t blt_otsc_parseObjectListFilter(u8 *data, u16 len, blc_ots_object_filter_hdr_t *filter)
{
    if (!len) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    switch (data[0]) {
    case BLC_OTS_FILTER_NO_FILTER:
        break;
    case BLC_OTS_FILTER_NAME_STARTS_WITH:
    case BLC_OTS_FILTER_NAME_ENDS_WITH:
    case BLC_OTS_FILTER_NAME_CONTAINS:
    case BLC_OTS_FILTER_NAME_IS_EXACTLY:
    {
        blc_ots_object_filter_name_starts_with_t *strFilter = (blc_ots_object_filter_name_starts_with_t *)filter;

        strFilter->length = len - 1;
        memcpy(strFilter->data, &data[1], strFilter->length);

        break;
    }
    case BLC_OTS_FILTER_OBJECT_TYPE:
    {
        blc_ots_object_filter_object_type_t *typeFilter = (blc_ots_object_filter_object_type_t *)filter;

        if ((len - 1) == ATT_16_UUID_LEN) {
            BYTE_TO_UINT16(typeFilter->type.uuidVal.u16, &data[1]);
        } else if ((len - 1) == ATT_128_UUID_LEN) {
            memcpy(typeFilter->type.uuidVal.u128, &data[1], sizeof(typeFilter->type.uuidVal.u128));
        } else {
            return GATT_ERR_INVALID_PARAMETER;
        }

        break;
    }
    case BLC_OTS_FILTER_CREATED_BETWEEN:
    case BLC_OTS_FILTER_MODIFIED_BETWEEN:
    {
        blc_ots_object_filter_created_between_t *timeBetweenFilter = (blc_ots_object_filter_created_between_t *)filter;

        if (len != sizeof(*timeBetweenFilter)) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        blt_otsc_parseDateTime(&data[1], UTC_DATE_TIME_SIZE, &timeBetweenFilter->timestamp1);
        blt_otsc_parseDateTime(&data[1 + UTC_DATE_TIME_SIZE], UTC_DATE_TIME_SIZE, &timeBetweenFilter->timestamp2);

        break;
    }
    case BLC_OTS_FILTER_CURRENT_SIZE_BETWEEN:
    case BLC_OTS_FILTER_ALLOCATED_SIZE_BETWEEN:
    {
        blc_ots_object_filter_current_size_between_t *sizeBetweenFilter = (blc_ots_object_filter_current_size_between_t *)filter;

        if (len != (1 + (2 * sizeof(u32)))) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        BYTES_TO_UINT32(sizeBetweenFilter->size1, &data[1]);
        BYTES_TO_UINT32(sizeBetweenFilter->size2, &data[1 + sizeof(u32)]);

        break;
    }
    case BLC_OTS_FILTER_MARKED_OBJECTS:
        break;
    default:
        return GATT_ERR_INVALID_PARAMETER;
    }

    filter->filter = data[0];

    return BLE_SUCCESS;
}

static u16 blt_otsc_composeObjectListFilter(u8 *data, u16 maxLen, blc_ots_object_filter_hdr_t *filter)
{
    u16 len = sizeof(u8); // at least filter octet

    if (!maxLen || !data) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    switch (filter->filter) {
    case BLC_OTS_FILTER_NO_FILTER:
        break;
    case BLC_OTS_FILTER_NAME_STARTS_WITH:
    case BLC_OTS_FILTER_NAME_ENDS_WITH:
    case BLC_OTS_FILTER_NAME_CONTAINS:
    case BLC_OTS_FILTER_NAME_IS_EXACTLY:
    {
        blc_ots_object_filter_name_starts_with_t *strFilter = (blc_ots_object_filter_name_starts_with_t *)filter;
        if (maxLen < (len + strFilter->length)) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        memcpy(&data[1], strFilter->data, strFilter->length);
        len += strFilter->length;

        break;
    }
    case BLC_OTS_FILTER_OBJECT_TYPE:
    {
        blc_ots_object_filter_object_type_t *typeFilter = (blc_ots_object_filter_object_type_t *)filter;
        u8                                  *ptr        = &data[1];

        if (maxLen < (len + typeFilter->type.uuidLen)) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        if (typeFilter->type.uuidLen == ATT_16_UUID_LEN) {
            U16_TO_STREAM(ptr, typeFilter->type.uuidVal.u16);
            len += sizeof(typeFilter->type.uuidVal.u16);
        } else if (typeFilter->type.uuidLen == ATT_128_UUID_LEN) {
            memcpy(ptr, typeFilter->type.uuidVal.u128, sizeof(typeFilter->type.uuidVal.u128));
            len += sizeof(typeFilter->type.uuidVal.u128);
        } else {
            return GATT_ERR_INVALID_PARAMETER;
        }

        break;
    }
    case BLC_OTS_FILTER_CREATED_BETWEEN:
    case BLC_OTS_FILTER_MODIFIED_BETWEEN:
    {
        blc_ots_object_filter_created_between_t *timeBetweenFilter = (blc_ots_object_filter_created_between_t *)filter;

        if (maxLen < (len + (UTC_DATE_TIME_SIZE * 2))) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        len += blt_otsc_composeDateTime(&data[len], &timeBetweenFilter->timestamp1);
        len += blt_otsc_composeDateTime(&data[len], &timeBetweenFilter->timestamp2);

        break;
    }
    case BLC_OTS_FILTER_CURRENT_SIZE_BETWEEN:
    case BLC_OTS_FILTER_ALLOCATED_SIZE_BETWEEN:
    {
        blc_ots_object_filter_current_size_between_t *sizeBetweenFilter = (blc_ots_object_filter_current_size_between_t *)filter;
        u8                                           *ptr               = &data[1];

        if (maxLen < (len + (sizeof(u32) * 2))) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        U32_TO_STREAM(ptr, sizeBetweenFilter->size1);
        U32_TO_STREAM(ptr, sizeBetweenFilter->size2);

        len += (ptr - &data[1]);

        break;
    }
    case BLC_OTS_FILTER_MARKED_OBJECTS:
        break;
    default:
        return GATT_ERR_INVALID_PARAMETER;
    }

    data[0] = filter->filter;

    return len;
}

ble_sts_t blc_otsc_getObjectListFilter(u16 connHandle, u8 id, blc_ots_object_filter_hdr_t *filter)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (!client || !client->otsObjectIdHdl || !filter || id >= ARRAY_SIZE(client->objectFilter)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    return blt_otsc_parseObjectListFilter(client->objectFilter[id]->val, client->objectFilter[id]->len, filter);
}

static void blt_otsc_readAttrValCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (pRdCfg->single.handle == client->otsFeatureHdl) {
        BLT_OTS_LOG("OTS Feature read callback, status: 0x%02X", err);
    } else if (pRdCfg->single.handle == client->otsObjectTypeHdl) {
        BLT_OTS_LOG("Object Type read callback, status: 0x%02X", err);
    } else if (pRdCfg->single.handle == client->otsObjectSizeHdl) {
        BLT_OTS_LOG("Object Size read callback, status: 0x%02X", err);
    } else if (pRdCfg->single.handle == client->otsObjectFirstCreatedHdl) {
        BLT_OTS_LOG("Object First Modified read callback, status: 0x%02X", err);
    } else if (pRdCfg->single.handle == client->otsObjectLastModifiedHdl) {
        BLT_OTS_LOG("Object Last Modified read callback, status: 0x%02X", err);
    } else if (pRdCfg->single.handle == client->otsObjectIdHdl) {
        BLT_OTS_LOG("Object ID read callback, status: 0x%02X", err);
    } else if (pRdCfg->single.handle == client->otsObjectPropertiesHdl) {
        BLT_OTS_LOG("Object Properties read callback, status: 0x%02X", err);
    } else if (pRdCfg->single.handle == client->otsObjectListFilterHdl[0] || pRdCfg->single.handle == client->otsObjectListFilterHdl[1] ||
               pRdCfg->single.handle == client->otsObjectListFilterHdl[2]) {
        BLT_OTS_LOG("Object Filter List read callback, status: 0x%02X", err);
    }

    blc_prf_readAttributeValueCallback(connHandle, err);
}

static ble_sts_t blc_otsc_readAttrVal(u16 connHandle, blt_otsc_read_t rdType, prf_read_cb_t readCb)
{
    blc_otsc_t     *client = blt_otsc_getClientInst(connHandle);
    gapc_read_cfg_t pGapReCfg;

    if (!client) {
        BLT_OTS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    } else if (rdType >= BLT_OTSC_READ_MAX) {
        BLT_OTS_LOG("ERR: Invalid read type %d", rdType);
        return GATT_ERR_INVALID_PARAMETER;
    }

    pGapReCfg.handle = 0;
    pGapReCfg.func   = blt_otsc_readAttrValCb;

    switch (rdType) {
    case BLT_OTSC_READ_OBJECT_NAME:
    {
        pGapReCfg.handle   = client->otsObjectNameHdl;
        pGapReCfg.wBuff    = client->objectName->val;
        pGapReCfg.wBuffLen = &client->objectName->len;
        pGapReCfg.maxLen   = gAppOtscObjectNameMaxSize;
        break;
    }
    case BLT_OTSC_READ_OTS_FEATURE:
    {
        pGapReCfg.handle   = client->otsFeatureHdl;
        pGapReCfg.wBuff    = client->otsFeature;
        pGapReCfg.wBuffLen = &client->otsFeatureSize;
        pGapReCfg.maxLen   = sizeof(client->otsFeature);
        break;
    }
    case BLT_OTSC_READ_OBJECT_TYPE:
    {
        pGapReCfg.handle   = client->otsObjectNameHdl;
        pGapReCfg.wBuff    = client->objectName->val;
        pGapReCfg.wBuffLen = &client->objectName->len;
        pGapReCfg.maxLen   = gAppOtscObjectNameMaxSize;
        break;
    }
    case BLT_OTSC_READ_OBJECT_SIZE:
    {
        pGapReCfg.handle   = client->otsObjectSizeHdl;
        pGapReCfg.wBuff    = client->objectSize;
        pGapReCfg.wBuffLen = &client->objectSizeLen;
        pGapReCfg.maxLen   = sizeof(client->objectSize);
        break;
    }
    case BLT_OTSC_READ_OBJECT_FIRST_CREATED:
    {
        pGapReCfg.handle   = client->otsObjectFirstCreatedHdl;
        pGapReCfg.wBuff    = client->objectFirstCreated;
        pGapReCfg.wBuffLen = &client->objectFirstCreatedLen;
        pGapReCfg.maxLen   = sizeof(client->objectFirstCreated);
        break;
    }
    case BLT_OTSC_READ_OBJECT_LAST_MODIFIED:
    {
        pGapReCfg.handle   = client->otsObjectLastModifiedHdl;
        pGapReCfg.wBuff    = (u8 *)client->objectLastModified;
        pGapReCfg.wBuffLen = &client->objectLastModifiedLen;
        pGapReCfg.maxLen   = sizeof(client->objectLastModified);
        break;
    }
    case BLT_OTSC_READ_OBJECT_ID:
    {
        pGapReCfg.handle   = client->otsObjectIdHdl;
        pGapReCfg.wBuff    = (u8 *)client->objectId;
        pGapReCfg.wBuffLen = &client->objectIdLen;
        pGapReCfg.maxLen   = sizeof(client->objectId);
        break;
    }
    case BLT_OTSC_READ_OBJECT_PROPERTIES:
    {
        pGapReCfg.handle   = client->otsObjectPropertiesHdl;
        pGapReCfg.wBuff    = (u8 *)client->objectProperties;
        pGapReCfg.wBuffLen = &client->objectPropertiesLen;
        pGapReCfg.maxLen   = sizeof(client->objectProperties);
        break;
    }
    case BLT_OTSC_READ_OBJECT_LIST_FILTER_0:
    case BLT_OTSC_READ_OBJECT_LIST_FILTER_1:
    case BLT_OTSC_READ_OBJECT_LIST_FILTER_2:
    {
        pGapReCfg.handle   = client->otsObjectListFilterHdl[rdType - BLT_OTSC_READ_OBJECT_LIST_FILTER_0];
        pGapReCfg.wBuff    = (u8 *)client->objectFilter[rdType - BLT_OTSC_READ_OBJECT_LIST_FILTER_0]->val;
        pGapReCfg.wBuffLen = &client->objectFilter[rdType - BLT_OTSC_READ_OBJECT_LIST_FILTER_0]->len;
        pGapReCfg.maxLen   = gAppOtscObjectFilterMaxSize;
        break;
    }
    default:
        break;
    }

    if (pGapReCfg.handle == 0) {
        BLT_ESLS_LOG("ERR: handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    return blc_prf_readAttributeValue(connHandle, &pGapReCfg, readCb);
}

ble_sts_t blc_otsc_readOtsFeature(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_otsc_readAttrVal(connHandle, BLT_OTSC_READ_OTS_FEATURE, readCb);
}

ble_sts_t blc_otsc_readObjectName(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_otsc_readAttrVal(connHandle, BLT_OTSC_READ_OBJECT_NAME, readCb);
}

ble_sts_t blc_otsc_readObjectType(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_otsc_readAttrVal(connHandle, BLT_OTSC_READ_OBJECT_TYPE, readCb);
}

ble_sts_t blc_otsc_readObjectSize(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_otsc_readAttrVal(connHandle, BLT_OTSC_READ_OBJECT_SIZE, readCb);
}

ble_sts_t blc_otsc_readObjectFirstCreated(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_otsc_readAttrVal(connHandle, BLT_OTSC_READ_OBJECT_FIRST_CREATED, readCb);
}

ble_sts_t blc_otsc_readObjectLastModified(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_otsc_readAttrVal(connHandle, BLT_OTSC_READ_OBJECT_LAST_MODIFIED, readCb);
}

ble_sts_t blc_otsc_readObjectId(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_otsc_readAttrVal(connHandle, BLT_OTSC_READ_OBJECT_ID, readCb);
}

ble_sts_t blc_otsc_readObjectProperties(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_otsc_readAttrVal(connHandle, BLT_OTSC_READ_OBJECT_PROPERTIES, readCb);
}

ble_sts_t blc_otsc_readObjectListFilter(u16 connHandle, u8 id, prf_read_cb_t readCb)
{
    if (id > 2) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    return blc_otsc_readAttrVal(connHandle, BLT_OTSC_READ_OBJECT_LIST_FILTER_0 + id, readCb);
}

static void blt_otsc_oacpWriteCb(u16 connHandle, u8 err, void *data)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)data;

    client->writeInProgress = false;
    if (client->oacpIndInProgress) {
        if (err == ATT_SUCCESS) {
            blt_otsc_startOacpTimer(connHandle);
        } else {
            client->oacpIndInProgress = false;
        }
    }

    blc_prf_writeAttributeValueCallback(connHandle, err);
}

static void blt_otsc_olcpWriteCb(u16 connHandle, u8 err, void *data)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)data;

    client->writeInProgress = false;
    if (client->olcpIndInProgress) {
        if (err == ATT_SUCCESS) {
            blt_otsc_startOlcpTimer(connHandle);
        } else {
            client->olcpIndInProgress = false;
        }
    }

    blc_prf_writeAttributeValueCallback(connHandle, err);
}

static void blt_otsc_writeCb(u16 connHandle, u8 err, void *data)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    (void)data;

    client->writeInProgress = false;

    blc_prf_writeAttributeValueCallback(connHandle, err);
}

static ble_sts_t blt_otsc_write(u16 connHandle, blt_otsc_write_t wrType, u8 *data, u16 len, prf_write_cb_t writeCb)
{
    blc_otsc_t       *client = blt_otsc_getClientInst(connHandle);
    gapc_write_cfg_t  pGapWrCfg;
    gapc_write_func_t cb = NULL;
    ble_sts_t         status;
    u16               handle = 0;

    if (!client) {
        BLT_OTS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    } else if (wrType >= BLT_OTSC_WRITE_MAX) {
        BLT_OTS_LOG("ERR: Invalid write type %d", wrType);
        return GATT_ERR_INVALID_PARAMETER;
    } else if (client->writeInProgress) {
        BLT_OTS_LOG("ERR: Write in progress");
        return GATT_ERR_INVALID_PARAMETER;
    }

    switch (wrType) {
    case BLT_OTSC_WRITE_OBJECT_NAME:
        if (client->otsObjectNameProperties & CHAR_PROP_WRITE) {
            handle = client->otsObjectNameHdl;

            memcpy(client->objectNameWrBuf->val, data, len);
            client->objectNameWrBuf->len = len;

            data = client->objectNameWrBuf->val;
        }
        break;
    case BLT_OTSC_WRITE_OBJECT_FIRST_CREATED:
        if (client->otsObjectFirstCreatedHdl && (client->otsObjectFirstCreatedProperties & CHAR_PROP_WRITE)) {
            handle = client->otsObjectFirstCreatedHdl;
        }
        break;
    case BLT_OTSC_WRITE_OBJECT_LAST_MODIFIED:
        if (client->otsObjectLastModifiedHdl && (client->otsObjectLastModifiedProperties & CHAR_PROP_WRITE)) {
            handle = client->otsObjectLastModifiedHdl;
        }
        break;
    case BLT_OTSC_WRITE_OBJECT_PROPERTIES:
        if (client->otsObjectPropertiesHdl && (client->otsObjectPropertiesProperties & CHAR_PROP_WRITE)) {
            handle = client->otsObjectPropertiesHdl;
        }
        break;
    case BLT_OTSC_WRITE_OBJECT_ACTION_CONTROL_POINT:
        if (!client->oacpIndInProgress) {
            handle = client->otsObjectActionControlPointHdl;
            cb     = blt_otsc_oacpWriteCb;
        }
        break;
    case BLT_OTSC_WRITE_OBJECT_LIST_CONTROL_POINT:
        if (!client->olcpIndInProgress) {
            handle = client->otsObjectListControlPointHdl;
            cb     = blt_otsc_olcpWriteCb;
        }
        break;
    case BLT_OTSC_WRITE_OBJECT_LIST_FILTER_0:
    case BLT_OTSC_WRITE_OBJECT_LIST_FILTER_1:
    case BLT_OTSC_WRITE_OBJECT_LIST_FILTER_2:
        handle = client->otsObjectListFilterHdl[wrType - BLT_OTSC_WRITE_OBJECT_LIST_FILTER_0];
        if (handle) {
            memcpy(client->objectFilterWrBuf[wrType - BLT_OTSC_WRITE_OBJECT_LIST_FILTER_0]->val, data, len);
            client->objectFilterWrBuf[wrType - BLT_OTSC_WRITE_OBJECT_LIST_FILTER_0]->len = len;

            data = client->objectFilterWrBuf[wrType - BLT_OTSC_WRITE_OBJECT_LIST_FILTER_0]->val;
        }
        break;
    default:
        handle = 0;
    }

    if (!handle) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    pGapWrCfg.func       = cb ? cb : blt_otsc_writeCb;
    pGapWrCfg.handle     = handle;
    pGapWrCfg.data       = data;
    pGapWrCfg.length     = len;
    pGapWrCfg.withoutRsp = false;
    pGapWrCfg.cbData     = NULL;

    status = blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, writeCb);
    if (status == BLE_SUCCESS) {
        client->writeInProgress = true;
        if (handle == client->otsObjectActionControlPointHdl) {
            client->oacpIndInProgress = true;
        } else if (handle == client->otsObjectListControlPointHdl) {
            client->olcpIndInProgress = true;
        }
    }

    return status;
}

ble_sts_t blc_otsc_writeObjectName(u16 connHandle, u8 *name, u16 len, prf_write_cb_t cb)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (!client || len > gAppOtscObjectNameMaxSize || !name) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    return blt_otsc_write(connHandle, BLT_OTSC_WRITE_OBJECT_NAME, name, len, cb);
}

ble_sts_t blc_otsc_writeObjectFirstCreated(u16 connHandle, blc_ots_utc_t *utc, prf_write_cb_t cb)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);
    u8          data[UTC_DATE_TIME_SIZE];
    u16         len;

    if (!client || !utc) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    len = blt_otsc_composeDateTime(data, utc);

    return blt_otsc_write(connHandle, BLT_OTSC_WRITE_OBJECT_FIRST_CREATED, data, len, cb);
}

ble_sts_t blc_otsc_writeObjectLastModified(u16 connHandle, blc_ots_utc_t *utc, prf_write_cb_t cb)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);
    u8          data[UTC_DATE_TIME_SIZE];
    u16         len;

    if (!client || !utc) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    len = blt_otsc_composeDateTime(data, utc);

    return blt_otsc_write(connHandle, BLT_OTSC_WRITE_OBJECT_LAST_MODIFIED, data, len, cb);
}

ble_sts_t blc_otsc_writeObjectProperties(u16 connHandle, u32 properties, prf_write_cb_t cb)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);
    u8          data[] = {U32_TO_BYTES(properties)};

    if (!client) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    return blt_otsc_write(connHandle, BLT_OTSC_WRITE_OBJECT_PROPERTIES, data, sizeof(data), cb);
}

ble_sts_t blc_otsc_writeObjectFilterList(u16 connHandle, u8 id, blc_ots_object_filter_hdr_t *filter, prf_write_cb_t cb)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);
    u8          data[gAppOtscObjectFilterMaxSize];
    u16         len;

    if (!client || id > 2) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    len = blt_otsc_composeObjectListFilter(data, sizeof(data), filter);

    return blt_otsc_write(connHandle, BLT_OTSC_WRITE_OBJECT_LIST_FILTER_0 + id, data, len, cb);
}

ble_sts_t blc_otsc_writeObjectActionControlPoint(u16 connHandle, blc_ots_oacp_cmd_hdr_t *cmd, prf_write_cb_t cb)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);
    u8          buf[OACP_CONTROL_POINT_SIZE];
    u8         *ptr = &buf[1];

    if (!client) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    switch (cmd->opcode) {
    case BLC_OTS_OACP_OPCODE_CREATE:
    {
        blc_ots_oacp_create_t *create = (blc_ots_oacp_create_t *)cmd;

        U32_TO_STREAM(ptr, create->size);
        if (create->type.uuidLen == ATT_16_UUID_LEN) {
            U16_TO_STREAM(ptr, create->type.uuidVal.u16);
        } else if (create->type.uuidLen == ATT_16_UUID_LEN) {
            memcpy(ptr, create->type.uuidVal.u128, sizeof(create->type.uuidVal.u128));
            ptr += sizeof(create->type.uuidVal.u128);
        } else {
            return GATT_ERR_INVALID_PARAMETER;
        }

        break;
    }
    case BLC_OTS_OACP_OPCODE_ABORT:
    case BLC_OTS_OACP_OPCODE_DELETE:
        break;
    case BLC_OTS_OACP_OPCODE_CALCULATE_CHECKSUM:
    {
        blc_ots_oacp_calculate_checksum_t *calcChecksum = (blc_ots_oacp_calculate_checksum_t *)cmd;

        U32_TO_STREAM(ptr, calcChecksum->offset);
        U32_TO_STREAM(ptr, calcChecksum->len);

        break;
    }
    case BLC_OTS_OACP_OPCODE_EXECUTE:
    {
        blc_ots_oacp_execute_t *execute = (blc_ots_oacp_execute_t *)cmd;

        if (execute->len >= sizeof(buf)) {
            return GATT_ERR_DATA_LENGTH_EXCEED_MEM_RESTRICTION;
        }

        STR_TO_STREAM(ptr, execute->data, execute->len);

        break;
    }
    case BLC_OTS_OACP_OPCODE_READ:
    {
        blc_ots_oacp_read_t *read = (blc_ots_oacp_read_t *)cmd;

        U32_TO_STREAM(ptr, read->offset);
        U32_TO_STREAM(ptr, read->len);

        break;
    }
    case BLC_OTS_OACP_OPCODE_WRITE:
    {
        blc_ots_oacp_write_t *write = (blc_ots_oacp_write_t *)cmd;

        U32_TO_STREAM(ptr, write->offset);
        U32_TO_STREAM(ptr, write->len);
        U8_TO_STREAM(ptr, write->mode);

        break;
    }
    default:
        return GATT_ERR_INVALID_PARAMETER;
    }

    buf[0] = cmd->opcode;

    return blt_otsc_write(connHandle, BLT_OTSC_WRITE_OBJECT_ACTION_CONTROL_POINT, buf, ptr - buf, cb);
}

ble_sts_t blc_otsc_writeObjectListControlPoint(u16 connHandle, blc_ots_olcp_cmd_hdr_t *cmd, prf_write_cb_t cb)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);
    u8          buf[OLCP_CONTROL_POINT_SIZE];
    u8         *ptr = &buf[1];

    if (!client) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    switch (cmd->opcode) {
    case BLC_OTS_OLCP_OPCODE_FIRST:
    case BLC_OTS_OLCP_OPCODE_LAST:
    case BLC_OTS_OLCP_OPCODE_PREVIOUS:
    case BLC_OTS_OLCP_OPCODE_NEXT:
    case BLC_OTS_OLCP_OPCODE_REQUEST_NUMBER_OF_OBJECTS:
    case BLC_OTS_OLCP_OPCODE_CLEAR_MARKING:
        break;
    case BLC_OTS_OLCP_OPCODE_GOTO:
    {
        blc_ots_olcp_goto_t *go_to = (blc_ots_olcp_goto_t *)cmd;

        STR_TO_STREAM(ptr, &go_to->id, sizeof(go_to->id));
        break;
    }
    case BLC_OTS_OLCP_OPCODE_ORDER:
    {
        blc_ots_olcp_order_t *order = (blc_ots_olcp_order_t *)cmd;

        U8_TO_STREAM(ptr, order->order);

        break;
    }
    default:
        return GATT_ERR_INVALID_PARAMETER;
    }

    buf[0] = cmd->opcode;

    return blt_otsc_write(connHandle, BLT_OTSC_WRITE_OBJECT_LIST_CONTROL_POINT, buf, ptr - buf, cb);
}

int blc_otsc_hostEventCallback(u32 h, u8 *para, int n)
{
    u8 event = h & 0xFF;

    (void)n;

    switch (event) {
    case GAP_EVT_L2CAP_COC_CONNECT:
    {
        gap_l2cap_cocConnectEvt_t *connEvt = (gap_l2cap_cocConnectEvt_t *)para;
        blc_otsc_t                *client  = blt_otsc_getClientInst(connEvt->connHandle);

        BLT_OTS_LOG("COC connected: connHandle: 0x%04X, SPSM: 0x%04X, MTU: 0x%04X, SCID 0x%04X, DCID 0x%04X", connEvt->connHandle, connEvt->spsm, connEvt->mtu, connEvt->srcCid, connEvt->dstCid);

        if (client && client->l2capState == BLT_OTSC_L2CAP_STATE_CONNECTING && connEvt->spsm == OTS_L2CAP_SPSM) {
            blc_otsc_objectTransferChannelConnectedEvt_t pEvt = {
                .mtu = connEvt->mtu,
            };

            client->l2capState = BLT_OTSC_L2CAP_STATE_CONNECTED;
            client->scid       = connEvt->srcCid;
            client->dcid       = connEvt->dstCid;
            client->mtu        = connEvt->mtu;

            blt_prf_sendEvent(connEvt->connHandle, ESL_EVT_OTSC_OBJECT_TRANSFER_CHANNEL_CONNECTED, (u8 *)&pEvt, sizeof(pEvt));
        }

        break;
    }

    case GAP_EVT_L2CAP_COC_DISCONNECT:
    {
        gap_l2cap_cocDisconnectEvt_t *discEvt = (gap_l2cap_cocDisconnectEvt_t *)para;
        blc_otsc_t                   *client  = blt_otsc_getClientInst(discEvt->connHandle);

        BLT_OTS_LOG("COC disconnected: connHandle: 0x%04X, SCID 0x%04X, DCID 0x%04X", discEvt->connHandle, discEvt->srcCid, discEvt->dstCid);


        if (client && client->l2capState == BLT_OTSC_L2CAP_STATE_CONNECTED && client->dcid == discEvt->dstCid) {
            client->scid       = 0;
            client->dcid       = 0;
            client->mtu        = 0;
            client->l2capState = BLT_OTSC_L2CAP_STATE_IDLE;
            blt_prf_sendEvent(discEvt->connHandle, ESL_EVT_OTSC_OBJECT_TRANSFER_CHANNEL_DISCONNECTED, NULL, 0);
        }

        break;
    }
    case GAP_EVT_L2CAP_COC_RECV_DATA:
    {
        gap_l2cap_cocRecvDataEvt_t *recvDataEvt = (gap_l2cap_cocRecvDataEvt_t *)para;
        blc_otsc_t                 *client      = blt_otsc_getClientInst(recvDataEvt->connHandle);

        BLT_OTS_LOG("COC data received: connHandle: 0x%04X, DCID 0x%04X, length 0x%04X", recvDataEvt->connHandle, recvDataEvt->dstCid, recvDataEvt->length);

        if (client && client->l2capState == BLT_OTSC_L2CAP_STATE_CONNECTED && client->dcid == recvDataEvt->dstCid) {
            u8                                               buf[OTSC_L2CAP_MTU + sizeof(blc_otsc_objectTransferChannelDataReceivedEvt_t)];
            blc_otsc_objectTransferChannelDataReceivedEvt_t *evt = (blc_otsc_objectTransferChannelDataReceivedEvt_t *)buf;

            if (recvDataEvt->length > OTSC_L2CAP_MTU) {
                BLT_OTS_LOG("COC err: data larger that MTU: 0x%04X", recvDataEvt->length);
                break;
            }

            evt->len = recvDataEvt->length;
            memcpy(evt->data, recvDataEvt->data, recvDataEvt->length);

            blt_prf_sendEvent(recvDataEvt->connHandle, ESL_EVT_OTSC_OBJECT_TRANSFER_CHANNEL_DATA_RECEIVED, (u8 *)evt, sizeof(*evt) + evt->len);
        }

        break;
    }
    case GAP_EVT_L2CAP_COC_SEND_DATA_FINISH:
    {
        gap_l2cap_cocSendDataFinishEvt_t *sendDataFinishEvt = (gap_l2cap_cocSendDataFinishEvt_t *)para;
        blc_otsc_t                       *client            = blt_otsc_getClientInst(sendDataFinishEvt->connHandle);

        BLT_OTS_LOG("COC data sent: connHandle: 0x%04X, SCID 0x%04X", sendDataFinishEvt->connHandle, sendDataFinishEvt->srcCid);

        if (client && client->l2capState == BLT_OTSC_L2CAP_STATE_CONNECTED && client->scid == sendDataFinishEvt->srcCid) {
            blt_prf_sendEvent(sendDataFinishEvt->connHandle, ESL_EVT_OTSC_OBJECT_TRANSFER_CHANNEL_DATA_SENT, NULL, 0);
        }

        break;
    }
    case GAP_EVT_L2CAP_COC_RECONFIGURE:
    {
        gap_l2cap_cocReconfigureEvt_t *recfgEvt = (gap_l2cap_cocReconfigureEvt_t *)para;

        BLT_OTS_LOG("COC reconfigure evt: connHandle: 0x%04X, SCID 0x%04X, MTU 0x%04X", recfgEvt->connHandle, recfgEvt->srcCid, recfgEvt->mtu);

        break;
    }
    default:
        break;
    }

    return 0;
}

ble_sts_t blc_otsc_openObjectTransferChannel(u16 connHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);
    ble_sts_t   status;

    if (!client || (client->l2capState != BLT_OTSC_L2CAP_STATE_IDLE)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    status = blc_l2cap_createLeCreditBasedConnect(connHandle);
    if (status == BLE_SUCCESS) {
        client->l2capState = BLT_OTSC_L2CAP_STATE_CONNECTING;
    }

    return status;
}

ble_sts_t blc_otsc_closeObjectTransferChannel(u16 connHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (!client || (client->l2capState != BLT_OTSC_L2CAP_STATE_CONNECTED)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    return blc_l2cap_disconnectCocChannel(connHandle, client->scid);
}

ble_sts_t blc_otsc_writeToObjectTransferChannel(u16 connHandle, u16 length, u8 *data)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    if (!client || client->l2capState != BLT_OTSC_L2CAP_STATE_CONNECTED || length > client->mtu) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    return blc_l2cap_sendCocData(connHandle, client->scid, data, length);
}

bool blc_otsc_isObjectTransferChannelOpened(u16 connHandle)
{
    blc_otsc_t *client = blt_otsc_getClientInst(connHandle);

    return client ? (client->l2capState == BLT_OTSC_L2CAP_STATE_CONNECTED) : false;
}
