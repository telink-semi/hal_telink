/********************************************************************************************************
 * @file    vcs_server.c
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

static blc_vcs_server_t *blt_vcss_getServerInst(u16 connHandle);

static void blt_vcp_serviceInit(const blc_vcss_regParam_t *param);
static int  blt_vcp_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen);
void        blt_vcss_initVolSetting(u8 volSetting);
void        blt_vcss_initMute(bool mute);

#define VCS_VOLUME_STATE_HANDLE(connHandle) (blt_vcss_getServerInst(connHandle)->volumeStateHdl)
#define VCS_VOLUME_FLAGS_HANDLE(connHandle) (blt_vcss_getServerInst(connHandle)->volumeFlagsHdl)


/* There shall be no more than one instance of the Volume Control Service (VCS) on a device. */
_attribute_ble_data_retention_
    blc_vcp_server_ctrl_t vcp_server_ctrl = {
        .process = {
                    .pNext       = NULL,
                    .id          = AUDIO_VCP_SERVER,
                    .usedAclRole = 0,
                    .init        = blt_vcss_init,
                    .connect     = blt_vcss_connect,
                    .discov      = NULL,
                    .loop        = NULL,
                    },
};

void blc_audio_registerVCSControlServer(const blc_vcss_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_PERIPHERAL, (blc_prf_proc_t *)&vcp_server_ctrl, param);
}

int blt_vcss_init(u8 initType, const void *param)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_vcs_server_t)), blc_vcs_server_t);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_vcp_server_t)), blc_vcp_server_t);
#endif

    if (initType == PRF_PROC_INIT) {
        BLT_VCS_LOG("Server init");
        blc_svc_addVcpGroup();
        blt_vcp_serviceInit(param);
        blc_svc_vcpCbackRegister(NULL, blt_vcp_writeCback);
    }
    //  else if (initType == PRF_PROC_DEINIT) {
    //      BLT_VCS_LOG("Server deinit");
    //  }
    return 0;
}

int blt_vcss_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if (connState == PRF_ACL_STATE_DISCONN) {
        BLT_VCS_LOG("Disconnect:0x%x", connHandle);
    } else {
        BLT_VCS_LOG("Connect:0x%x", connHandle);
    }
    return 0;
}

blc_vcp_server_t *blt_vcp_getServerInst(u16 connHandle)
{
#if (0)
    int ret = blt_prf_getAclRole(connHandle);
    if (ret < 0 || ret == ACL_ROLE_CENTRAL) {
        BLT_VCS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if (ret >= 0) {
            /* VCP Volume Renderer GAP Peripheral */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_VCP_SERVER, ret);
        }

        return NULL;
    }
#else
    (void)connHandle;
#endif

    return &vcp_server_ctrl.server;
}

static blc_vcs_server_t *blt_vcss_getServerInst(u16 connHandle)
{
    blc_vcp_server_t *server = blt_vcp_getServerInst(connHandle);
    if (server == NULL) {
        return NULL;
    }

    return &server->vcsServer;
}

static void blt_vcss_initVolumeStateChar(atts_foundCharParam_t *p, void *input)
{
    blc_vcs_server_t *vcss = &((blc_vcp_server_t *)input)->vcsServer;
    if (p->num > 0) {
        BLT_VCS_LOG("ERR: Volume State char too many");
        return;
    }
    vcss->volumeStateHdl = p->charHandle;
}

static void blt_vcss_initVolumeCtrlPointChar(atts_foundCharParam_t *p, void *input)
{
    blc_vcs_server_t *vcss = &((blc_vcp_server_t *)input)->vcsServer;
    if (p->num > 0) {
        BLT_VCS_LOG("ERR: Volume Control Point char too many");
        return;
    }
    vcss->volCtrlPointHdl = p->charHandle;
}

static void blt_vcss_initVolumeFlagsChar(atts_foundCharParam_t *p, void *input)
{
    blc_vcs_server_t *vcss = &((blc_vcp_server_t *)input)->vcsServer;
    if (p->num > 0) {
        BLT_VCS_LOG("ERR: Volume Flags char too many");
        return;
    }
    vcss->volumeFlagsHdl = p->charHandle;
}

static const atts_findCharList_t vcssChar[] = {
    {
     .charUuid    = characteristicVolumeStateUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_vcss_initVolumeStateChar,
     },
    {
     .charUuid    = characteristicVolumeControlPointUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_vcss_initVolumeCtrlPointChar,
     },
    {
     .charUuid    = characteristicVolumeFlagsUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_vcss_initVolumeFlagsChar,
     },
};

extern const atts_findInclList_t vocsService;
extern const atts_findInclList_t aicsService;

static const atts_findServiceList_t vcpVolRenderer = {
    .serviceUuidLen = ATT_16_UUID_LEN,
    .serviceUuid    = serviceVolumeControlUuid,
    .charSize       = ARRAY_SIZE(vcssChar),
    .charList       = vcssChar,
    .inclSize       = 2,
    .inclList[0]    = &vocsService,
    .inclList[1]    = &aicsService,
};

static void blt_vcp_serviceInit(const blc_vcss_regParam_t *param)
{
    blc_vcp_server_t *server = blt_vcp_getServerInst(0xFFFF);

    blc_atts_findCharacteristic(&vcpVolRenderer, server);
    BLT_VCS_LOG("Handle information, volState:0x%x VolCtrlPoint:0x%x VolFlags:0x%x", server->vcsServer.volumeStateHdl, server->vcsServer.volCtrlPointHdl, server->vcsServer.volumeFlagsHdl);

    const blc_vcss_regParam_t *volRenderRegParam = param;

    if (volRenderRegParam == NULL) { //use default parameters
        volRenderRegParam = &defaultVcpRendererParam;
    }

    const blc_vcs_regParam_t *vcsParam = &volRenderRegParam->vcsParam;
    server->vcsServer.volStep          = vcsParam->step;
    blt_vcss_initVolSetting(vcsParam->volume);
    blt_vcss_initMute(vcsParam->mute);

    u8 *ptr = (u8 *)(size_t)volRenderRegParam->aicsParam;

    for (int i = 0; i < server->aicsServerCnt; i++) {
        blc_aics_server_t *aics = server->aicsServer[i];
        blt_aicss_initParam(aics, ptr);
        ptr += sizeof(blc_aicss_regParam_t);
    }

    ptr = (u8 *)(size_t)volRenderRegParam->vocsParam;

    for (int i = 0; i < server->vocsServerCnt; i++) {
        blc_vocs_server_t *vocs = server->vocsServer[i];
        blt_vocss_initParam(vocs, ptr);
        ptr += sizeof(blc_vocss_regParam_t);
    }
}

typedef int (*volCtrlCb_fun)(blc_vcs_server_t *vcs, blc_vcs_volume_state_t *state, u8 operand);

typedef struct
{
    u8            opcode;
    u8            size;
    volCtrlCb_fun ctrlCb;
} blt_vcss_vol_ctrl_cmds_t;

static int blt_vcss_dealRelativeVolDown(blc_vcs_server_t *vcs, blc_vcs_volume_state_t *state, u8 operand)
{
    (void)operand;
    state->volSetting = state->volSetting - vcs->volStep < VCS_MIN_VOLUME_SETTING ?
                            VCS_MIN_VOLUME_SETTING :
                            state->volSetting - vcs->volStep;
    return 1;
}

static int blt_vcss_dealRelativeVolUp(blc_vcs_server_t *vcs, blc_vcs_volume_state_t *state, u8 operand)
{
    (void)operand;
    state->volSetting = state->volSetting + vcs->volStep > VCS_MAX_VOLUME_SETTING ?
                            VCS_MAX_VOLUME_SETTING :
                            state->volSetting + vcs->volStep;
    return 1;
}

static int blt_vcss_dealUnmuteVolDown(blc_vcs_server_t *vcs, blc_vcs_volume_state_t *state, u8 operand)
{
    (void)operand;
    state->mute       = VCS_MUTE_STATE_NOT_MUTED;
    state->volSetting = state->volSetting - vcs->volStep < VCS_MIN_VOLUME_SETTING ?
                            VCS_MIN_VOLUME_SETTING :
                            state->volSetting - vcs->volStep;
    return 1;
}

static int blt_vcss_dealUnmuteVolUp(blc_vcs_server_t *vcs, blc_vcs_volume_state_t *state, u8 operand)
{
    (void)operand;
    state->mute       = VCS_MUTE_STATE_NOT_MUTED;
    state->volSetting = state->volSetting + vcs->volStep > VCS_MAX_VOLUME_SETTING ?
                            VCS_MAX_VOLUME_SETTING :
                            state->volSetting + vcs->volStep;
    return 1;
}

static int blt_vcss_dealSetAbsoluteVol(blc_vcs_server_t *vcs, blc_vcs_volume_state_t *state, u8 operand)
{
    (void)operand;
    (void)vcs;
    state->volSetting = operand;
    return 1;
}

static int blt_vcss_dealUnmute(blc_vcs_server_t *vcs, blc_vcs_volume_state_t *state, u8 operand)
{
    (void)operand;
    (void)vcs;
    state->mute = VCS_MUTE_STATE_NOT_MUTED;
    return 0;
}

static int blt_vcss_dealMute(blc_vcs_server_t *vcs, blc_vcs_volume_state_t *state, u8 operand)
{
    (void)operand;
    (void)vcs;
    state->mute = VCS_MUTE_STATE_MUTED;
    return 0;
}

static const blt_vcss_vol_ctrl_cmds_t vcssVolCtrl[] = {
    {VCS_OPCODE_RELATIVE_VOLUME_DOWN,        2, blt_vcss_dealRelativeVolDown},
    {VCS_OPCODE_RELATIVE_VOLUME_UP,          2, blt_vcss_dealRelativeVolUp  },
    {VCS_OPCODE_UNMUTE_RELATIVE_VOLUME_DOWN, 2, blt_vcss_dealUnmuteVolDown  },
    {VCS_OPCODE_UNMUTE_RELATIVE_VOLUME_UP,   2, blt_vcss_dealUnmuteVolUp    },
    {VCS_OPCODE_SET_ABSOLUTE_VOLUME,         3, blt_vcss_dealSetAbsoluteVol },
    {VCS_OPCODE_UNMUTE,                      2, blt_vcss_dealUnmute         },
    {VCS_OPCODE_MUTE,                        2, blt_vcss_dealMute           },
};

static int blt_vcss_writeCback(u16 connHandle, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    (void)attrHandle;

    //The data length must be equal to 2 or 3, and contains instructions and change_counter
    if (valueLen != 2 && valueLen != 3) {
        return ATT_ERR_INVALID_PDU;
    }

    blc_vcs_server_t *vcs = blt_vcss_getServerInst(connHandle);

    blc_vcs_volume_state_t *state = blc_vcss_getVolState(connHandle);
    blc_vcs_volume_flags_t *flags = blc_vcss_getVolFlags(connHandle);

    u8 opcode    = writeValue[0];
    u8 changeCnt = writeValue[1];

    if (state->changeCnt != changeCnt) {
        BLT_VCS_LOG("vcs write change counter error, write[0x%x] local[0x%x]", changeCnt, state->changeCnt);
        return VCS_ERRCODE_INVALID_CHANGE_COUNTER;
    }

    for (size_t i = 0; i < ARRAY_SIZE(vcssVolCtrl); i++) {
        if (opcode == vcssVolCtrl[i].opcode) {
            if (valueLen == vcssVolCtrl[i].size) {
                u8 stateTemp[2] = {state->volSetting, state->mute};
                u8 volIsChange  = vcssVolCtrl[i].ctrlCb(vcs, state, writeValue[2]);
                if ((stateTemp[0] == state->volSetting) && (stateTemp[1] == state->mute)) {
                    return ATT_SUCCESS;
                }

                blc_vcss_volumeStateChangeEvt_t evt;
                evt.volumeSetting = state->volSetting;
                evt.mute          = state->mute == VCS_MUTE_STATE_NOT_MUTED ? false : true;
                blt_prf_sendEvent(connHandle, AUDIO_EVT_VCSS_CHANGED_VOLUME_STATE, (u8 *)&evt, sizeof(blc_vcss_volumeStateChangeEvt_t));
                state->changeCnt++;
                blc_gatts_notifyAttr(connHandle, VCS_VOLUME_STATE_HANDLE(connHandle));
                if (volIsChange && (flags->volSettingPersisted & 0x01) == 0) {
                    flags->volSettingPersisted = 0x01;
                    blc_gatts_notifyAttr(connHandle, VCS_VOLUME_FLAGS_HANDLE(connHandle));
                }
                return ATT_SUCCESS;
            } else {
                return ATT_ERR_INVALID_PDU;
            }
        }
    }

    return VCS_ERRCODE_OPCODE_NOT_SUPPORTED;
}

extern int blt_vocss_writeCback(u16 connHandle, u16 attrHandle, u8 *writeValue, u16 valueLen);

static int blt_vcp_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    (void)opcode;
    blc_vcs_server_t *server = blt_vcss_getServerInst(connHandle);
    if (attrHandle == server->volCtrlPointHdl) {
        return blt_vcss_writeCback(connHandle, attrHandle, writeValue, valueLen);
    }

    int err = ATT_SUCCESS;

    err = blt_vocss_writeCback(connHandle, attrHandle, writeValue, valueLen);
    if (err != ATT_ERR_INVALID_HANDLE) {
        return err;
    }

    err = blt_aicss_writeCback(connHandle, attrHandle, writeValue, valueLen);
    if (err != ATT_ERR_INVALID_HANDLE) {
        return err;
    }

    return ATT_ERR_INVALID_HANDLE;
}

blc_vcs_volume_state_t *blc_vcss_getVolState(u16 connHandle)
{
    return (blc_vcs_volume_state_t *)blc_gatts_getAttributeValueByHandle(connHandle, VCS_VOLUME_STATE_HANDLE(connHandle));
}

blc_vcs_volume_flags_t *blc_vcss_getVolFlags(u16 connHandle)
{
    return (blc_vcs_volume_flags_t *)blc_gatts_getAttributeValueByHandle(connHandle, VCS_VOLUME_FLAGS_HANDLE(connHandle));
}

void blt_vcss_initVolSetting(u8 volSetting)
{
    blc_vcs_volume_state_t *pState = blc_vcss_getVolState(0xFFFF);
    pState->volSetting             = volSetting;
}

void blt_vcss_initMute(bool mute)
{
    blc_vcs_volume_state_t *pState = blc_vcss_getVolState(0xFFFF);
    pState->mute                   = mute ? VCS_MUTE_STATE_MUTED : VCS_MUTE_STATE_NOT_MUTED;
}

static int blt_vcss_sendVolState(u16 connHandle)
{
    blc_vcs_volume_state_t *pState = blc_vcss_getVolState(0xFFFF);
    pState->changeCnt++;

    return blc_gatts_notifyAttr(connHandle, VCS_VOLUME_STATE_HANDLE(connHandle));
}

int blc_vcss_updateVolSetting(u16 connHandle, u8 volSetting)
{
    blt_vcss_initVolSetting(volSetting);
    return blt_vcss_sendVolState(connHandle);
}

int blc_vcss_updateMuteState(u16 connHandle, bool mute)
{
    blt_vcss_initMute(mute);
    return blt_vcss_sendVolState(connHandle);
}

int blc_vcss_updateVolState(u16 connHandle, u8 volSetting, bool mute)
{
    blt_vcss_initVolSetting(volSetting);
    blt_vcss_initMute(mute);
    return blt_vcss_sendVolState(connHandle);
}

int blc_vcss_updateVolFlags(u16 connHandle, blc_vcs_volume_flags_t flags)
{
    blc_vcs_volume_flags_t *pFlags = blc_vcss_getVolFlags(connHandle);
    *pFlags                        = flags;
    return blc_gatts_notifyAttr(connHandle, VCS_VOLUME_FLAGS_HANDLE(connHandle));
}
