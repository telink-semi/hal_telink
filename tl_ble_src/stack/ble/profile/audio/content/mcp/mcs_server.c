/********************************************************************************************************
 * @file    mcs_server.c
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


#define MCS_TRACK_DURATION_UNKNOWN     (-1)
#define MCS_TRACK_POSITION_UNAVAILABLE (-1)
#define MCS_VALID_PLAYING_ORDERS_MASK  (BLC_MCS_PLAYING_ORDER_SUPPORT_SINGLE_ONCE | BLC_MCS_PLAYING_ORDER_SUPPORT_SINGLE_REPEAT |    \
                                       BLC_MCS_PLAYING_ORDER_SUPPORT_IN_ORDER_ONCE | BLC_MCS_PLAYING_ORDER_SUPPORT_IN_ORDER_REPEAT | \
                                       BLC_MCS_PLAYING_ORDER_SUPPORT_OLDEST_ONCE | BLC_MCS_PLAYING_ORDER_SUPPORT_OLDEST_REPEAT |     \
                                       BLC_MCS_PLAYING_ORDER_SUPPORT_NEWEST_ONCE | BLC_MCS_PLAYING_ORDER_SUPPORT_NEWEST_REPEAT |     \
                                       BLC_MCS_PLAYING_ORDER_SUPPORT_SHUFFLE_ONCE | BLC_MCS_PLAYING_ORDER_SUPPORT_SHUFFLE_REPEAT)
#define SEARCH_CONTROL_POINT_EVT_BUF_SIZE 128

static void blt_gmcss_serviceInit(const blc_mcps_regParam_t *param);
static int  blt_gmcss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen);
static int  blt_gmcss_readCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 **outValue, u16 *outValueLen);
static int  blt_gmcss_connect(u16 connHandle, prf_acl_state_enum connState);

_attribute_ble_data_retention_
    blc_mcp_server_ctrl_t mcp_server_ctrl = {
        .process = {
                    .pNext       = NULL,
                    .id          = AUDIO_GMCS_SERVER,
                    .usedAclRole = 0,
                    .init        = blt_mcss_init,
                    .connect     = blt_gmcss_connect,
                    .discov      = NULL,
                    .loop        = NULL,
                    },
};

void blc_audio_registerMediaControlServer(const blc_mcps_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t *)&mcp_server_ctrl, param);
}

blc_mcp_server_t *blt_mcp_getServerInst(u16 connHandle)
{
#if (0)
    int ret = blt_prf_getAclRole(connHandle);
    if (ret < 0 || ret == ACL_ROLE_CENTRAL) {
        BLT_MCS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if (ret >= 0) {
            /* VCP Volume Renderer GAP Peripheral */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_VCP_VOLUME_RENDERER, ret);
        }

        return NULL;
    }
#else
    (void)connHandle;
#endif

    return &mcp_server_ctrl.mcpServer;
}

static blc_gmcs_server_t *blt_gmcss_getServerInst(u16 connHandle)
{
    blc_mcp_server_t *server = blt_mcp_getServerInst(connHandle);

    return server ? &server->gmcs : NULL;
}

int blt_mcss_init(u8 initType, const void *param)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_mcp_server_t)), blc_mcp_server_t);
#endif

    if (initType == PRF_PROC_INIT) {
        blc_svc_addGmcsGroup();
        blc_svc_gmcsCbackRegister(blt_gmcss_readCback, blt_gmcss_writeCback);
        BLT_MCS_LOG("Server init");
        blt_gmcss_serviceInit(param);
    }
    //  else if (initType == PRF_PROC_DEINIT) {
    //      blc_svc_removeGmcsGroup();
    //      BLT_MCS_LOG("Server Deinit");
    //  }
    return 0;
}

static int blt_gmcss_connect(u16 connHandle, prf_acl_state_enum connState)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);

    if (connState == PRF_ACL_STATE_DISCONN) {
        BLT_TBS_LOG("blt_gmcss_disconnect: 0x%x", connHandle);

        for (u8 i = 0; i < ARRAY_SIZE(gmcss->valueChanged); i++) {
            if (gmcss->valueChanged[i].active && gmcss->valueChanged[i].connHandle == connHandle) {
                memset(&gmcss->valueChanged[i], 0, sizeof(gmcss->valueChanged[i]));
                break;
            }
        }
    } else {
        BLT_TBS_LOG("blt_gmcss_connect: 0x%x", connHandle);

        for (u8 i = 0; i < ARRAY_SIZE(gmcss->valueChanged); i++) {
            if (!gmcss->valueChanged[i].active) {
                gmcss->valueChanged[i].active     = true;
                gmcss->valueChanged[i].connHandle = connHandle;

                // Require client to start read below characteristics with zero offset
                gmcss->valueChanged[i].mediaPlayerNameChanged = true;
                gmcss->valueChanged[i].trackTitleChanged      = true;
                break;
            }
        }
    }

    return 0;
}

static int blt_gmcss_setMediaPlayerName(u16 connHandle, u8 *mediaPlayerName, u16 mediaPlayerNameLength, bool notify)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    extern const u16   gmcsMediaPlayerNameMaxSize;
    u8                *value = NULL;
    u16               *len   = NULL;

    if (mediaPlayerNameLength > gmcsMediaPlayerNameMaxSize) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    blc_gatts_getAttributeInformationByHandle(connHandle, gmcss->mediaPlayerNameHandle, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    memcpy(value, mediaPlayerName, mediaPlayerNameLength);
    *len = mediaPlayerNameLength;

    for (u8 i = 0; i < ARRAY_SIZE(gmcss->valueChanged); i++) {
        if (gmcss->valueChanged[i].active) {
            gmcss->valueChanged[i].mediaPlayerNameChanged = true;
        }
    }

    if (notify) {
        u16 mtu = blt_gap_getEffectiveMTU(connHandle);

        if (mtu > 3) {
            return blc_gatts_notifyValue(connHandle, gmcss->mediaPlayerNameHandle, value, min(mtu - 3, *len));
        }
    }

    return BLE_SUCCESS;
}

int blc_gmcss_updateMediaPlayerName(u16 connHandle, u8 *mediaPlayerName, u16 mediaPlayerNameLength)
{
    return blt_gmcss_setMediaPlayerName(connHandle, mediaPlayerName, mediaPlayerNameLength, true);
}

static ble_sts_t setIdChar(const u8 id[6], u8 idLen, u16 connHandle, u16 handle, bool notify)
{
    u8  *value = NULL;
    u16 *len   = NULL;

    blc_gatts_getAttributeInformationByHandle(connHandle, handle, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    if (idLen) {
        memcpy(value, id, idLen);
    }
    *len = idLen;

    return notify ? blc_gatts_notifyAttr(connHandle, handle) : BLE_SUCCESS;
}

static int blt_gmcss_setIdChar(const blc_object_id_t *id, u16 connHandle, u16 handle, bool notify)
{
    if (id) {
        return setIdChar(id->objectId, sizeof(id->objectId), connHandle, handle, notify);
    }

    return setIdChar(NULL, 0, connHandle, handle, notify);
}

int blc_gmcss_updateMediaPlayerIconObjectId(u16 connHandle, const blc_object_id_t *mediaPlayerIconObjectId)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);

    return blt_gmcss_setIdChar(mediaPlayerIconObjectId, connHandle, gmcss->mediaPlayerIconObjectIDHandle, false);
}

int blc_gmcss_updateMediaPlayerIconURL(u16 connHandle, u8 *iconUrl, u8 iconUrlLen)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    extern const u16   gmcsMediaPlayerIconURLMaxSize;
    u8                *value = NULL;
    u16               *len   = NULL;

    if (iconUrlLen > gmcsMediaPlayerIconURLMaxSize) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    blc_gatts_getAttributeInformationByHandle(connHandle, gmcss->mediaPlayerIconURLHandle, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    if (iconUrlLen) {
        memcpy(value, iconUrl, iconUrlLen);
    }
    *len = iconUrlLen;

    return BLE_SUCCESS;
}

int blc_gmcss_updateTrackChanged(u16 connHandle)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);

    return blc_gatts_notifyAttr(connHandle, gmcss->trackChangedHandle);
}

static int blt_gmcss_setTrackTitle(u16 connHandle, u8 *trackTitle, u16 trackTitleLength, bool notify)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    extern const u16   gmcsTrackTitleMaxSize;
    u8                *value = NULL;
    u16               *len   = NULL;

    if (trackTitleLength > gmcsTrackTitleMaxSize) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    blc_gatts_getAttributeInformationByHandle(connHandle, gmcss->trackTitleHandle, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    if (trackTitleLength) {
        memcpy(value, trackTitle, trackTitleLength);
    }
    *len = trackTitleLength;

    for (u8 i = 0; i < ARRAY_SIZE(gmcss->valueChanged); i++) {
        if (gmcss->valueChanged[i].active) {
            gmcss->valueChanged[i].trackTitleChanged = true;
        }
    }

    if (notify) {
        u16 mtu = blt_gap_getEffectiveMTU(connHandle);

        if (mtu > 3) {
            return blc_gatts_notifyValue(connHandle, gmcss->trackTitleHandle, value, min(mtu - 3, *len));
        }
    }

    return BLE_SUCCESS;
}

int blc_gmcss_updateTrackTitle(u16 connHandle, u8 *trackTitle, u16 trackTitleLength)
{
    return blt_gmcss_setTrackTitle(connHandle, trackTitle, trackTitleLength, true);
}

static int blt_gmcss_setTrackDuration(u16 connHandle, s32 trackDuration, bool notify)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    s32               *value;

    value = (s32 *)blc_gatts_getAttributeValueByHandle(connHandle, gmcss->trackDurationHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    // "If the media player has no current track or the duration of the current track is unknown, the Track Duration
    // characteristic value shall be 0xFFFFFFFF. Otherwise, the duration of the track shall be zero or greater."
    // MCS specification 3.6 Track Duration
    *value = trackDuration < 0 ? MCS_TRACK_DURATION_UNKNOWN : trackDuration;

    return notify ? blc_gatts_notifyAttr(connHandle, gmcss->trackDurationHandle) : BLE_SUCCESS;
}

int blc_gmcss_updateTrackDuration(u16 connHandle, s32 duration)
{
    return blt_gmcss_setTrackDuration(connHandle, duration, true);
}

static int blt_gmcss_setTrackPosition(u16 connHandle, s32 trackPosition, bool notify)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    s32               *value;

    value = (s32 *)blc_gatts_getAttributeValueByHandle(connHandle, gmcss->trackPositionHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    // "The Track Position characteristic exposes the offset from the start of
    // the track to the current playing position. If the start of the track is not well
    // defined (such as in a live stream), the server sets a starting position (where
    // Track Position equals 0) or sets the value to UNAVAILABLE (0xFFFFFFFF)."
    // MCS specification 3.7 Track Position
    *value = trackPosition < 0 ? MCS_TRACK_POSITION_UNAVAILABLE : trackPosition;

    return notify ? blc_gatts_notifyAttr(connHandle, gmcss->trackPositionHandle) : BLE_SUCCESS;
}

int blc_gmcss_updateTrackPosition(u16 connHandle, s32 trackPosition)
{
    return blt_gmcss_setTrackPosition(connHandle, trackPosition, true);
}

int blc_gmcss_setTrackPosition(s32 trackPosition)
{
    return blt_gmcss_setTrackPosition(0xFFFF, trackPosition, false);
}

static int blt_gmcss_setMediaState(u16 connHandle, u8 mediaState, bool notify)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    u8                *value;

    if (mediaState > GMCS_MEDIA_STATE_SEEKING) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    value = blc_gatts_getAttributeValueByHandle(connHandle, gmcss->mediaStateHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *value = mediaState;

    return notify ? blc_gatts_notifyAttr(connHandle, gmcss->mediaStateHandle) : BLE_SUCCESS;
}

int blc_gmcss_updateMediaState(u16 connHandle, u8 mediaState)
{
    return blt_gmcss_setMediaState(connHandle, mediaState, true);
}

int blc_gmcss_updateContentCtrlID(u16 connHandle, u8 ccid)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    u8                *value;
    value = blc_gatts_getAttributeValueByHandle(connHandle, gmcss->contentControlIDHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *value = ccid;

    return BLE_SUCCESS;
}

static int blt_gmcss_setMediaControlPointOpcodesSupported(u16 connHandle, u32 supportedOpcodes, bool notify)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    u32               *value;
    value = (u32 *)blc_gatts_getAttributeValueByHandle(connHandle, gmcss->mediaControlPointOpcodesSupportedHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *value = supportedOpcodes;

    return notify ? blc_gatts_notifyAttr(connHandle, gmcss->mediaControlPointOpcodesSupportedHandle) : BLE_SUCCESS;
}

int blc_gmcss_updateMediaCtrlPointOpSupp(u16 connHandle, u32 supportedOpcodes)
{
    return blt_gmcss_setMediaControlPointOpcodesSupported(connHandle, supportedOpcodes, true);
}

int blc_gmcss_updateMediaCtrlPoint(u16 connHandle, blc_mcs_mediaCtrlPointOpcode_enum op, u8 result)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    u8                *value;
    u16               *len;

    blc_gatts_getAttributeInformationByHandle(connHandle, gmcss->mediaControlPointHandle, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    value[0] = op;
    value[1] = result;
    *len     = 2;

    return blc_gatts_notifyAttr(connHandle, gmcss->mediaControlPointHandle);
}

static int blt_gmcss_setPlaybackSpeed(u16 connHandle, s8 p, bool notify)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    s8                *value;
    value = (s8 *)blc_gatts_getAttributeValueByHandle(connHandle, gmcss->playbackSpeedHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *value = p;

    return notify ? blc_gatts_notifyAttr(connHandle, gmcss->playbackSpeedHandle) : BLE_SUCCESS;
}

int blc_gmcss_updatePlaybackSpeed(u16 connHandle, s8 playbackSpeed)
{
    return blt_gmcss_setPlaybackSpeed(connHandle, playbackSpeed, true);
}

static int blt_gmcss_setSeekingSpeed(u16 connHandle, s8 p, bool notify)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    s8                *value;
    value = (s8 *)blc_gatts_getAttributeValueByHandle(connHandle, gmcss->seekingSpeedHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *value = p;

    return notify ? blc_gatts_notifyAttr(connHandle, gmcss->seekingSpeedHandle) : BLE_SUCCESS;
}

int blc_gmcss_updateSeekingSpeed(u16 connHandle, s8 seekingSpeed)
{
    return blt_gmcss_setSeekingSpeed(connHandle, seekingSpeed, true);
}

static int blt_gmcss_setPlayingOrdersSupported(u16 connHandle, u16 p, bool notify)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    u16               *value;

    if (p & ~(MCS_VALID_PLAYING_ORDERS_MASK)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    value = (u16 *)blc_gatts_getAttributeValueByHandle(connHandle, gmcss->playingOrdersSupportedHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *value = p;

    return notify ? blc_gatts_notifyAttr(connHandle, gmcss->playingOrdersSupportedHandle) : BLE_SUCCESS;
}

int blc_gmcss_updatePlayingOrdersSupported(u16 connHandle, u16 playingOrdersSupported)
{
    return blt_gmcss_setPlayingOrdersSupported(connHandle, playingOrdersSupported, true);
}

static int blt_gmcss_setPlayingOrder(u16 connHandle, u8 playingOrder, bool notify)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    u8                *value;

    if (playingOrder > BLC_MCS_PLAYING_ORDER_SHUFFLE_REPEAT || playingOrder < BLC_MCS_PLAYING_ORDER_SINGLE_ONCE) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    value = blc_gatts_getAttributeValueByHandle(connHandle, gmcss->playingOrderHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *value = playingOrder;

    return notify ? blc_gatts_notifyAttr(connHandle, gmcss->playingOrderHandle) : BLE_SUCCESS;
}

int blc_gmcss_updatePlayingOrder(u16 connHandle, u8 playingOrder)
{
    return blt_gmcss_setPlayingOrder(connHandle, playingOrder, true);
}

int blc_gmcss_updateCurrentTrackSegmentsObjectId(u16 connHandle, const blc_object_id_t *currentTrackSegmentsObjectId)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);

    return blt_gmcss_setIdChar(currentTrackSegmentsObjectId, connHandle, gmcss->currentTrackSegmentsObjectIDHandle, false);
}

int blc_gmcss_updateCurrentTrackObjectID(u16 connHandle, const blc_object_id_t *currentTrackObjectId)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);

    return blt_gmcss_setIdChar(currentTrackObjectId, connHandle, gmcss->currentTrackObjectIDHandle, true);
}

int blc_gmcss_updateNextTrackObjectID(u16 connHandle, const blc_object_id_t *nextTrackObjectId)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);

    return blt_gmcss_setIdChar(nextTrackObjectId, connHandle, gmcss->nextTrackObjectIDHandle, true);
}

int blc_gmcss_updateParentGroupObjectID(u16 connHandle, const blc_object_id_t *parentGroupObjectId)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);

    return blt_gmcss_setIdChar(parentGroupObjectId, connHandle, gmcss->parentGroupObjectIDHandle, true);
}

int blc_gmcss_updateCurrentGroupObjectID(u16 connHandle, const blc_object_id_t *currentGroupObjectId)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);

    return blt_gmcss_setIdChar(currentGroupObjectId, connHandle, gmcss->currentGroupObjectIDHandle, true);
}

int blc_gmcss_updateSearchCtrlPoint(u16 connHandle, u8 resultCode)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);
    u8                *res;

    res = (u8 *)blc_gatts_getAttributeValueByHandle(connHandle, gmcss->searchControlPointHandle);
    if (!res) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *res = resultCode;

    return blc_gatts_notifyAttr(connHandle, gmcss->searchControlPointHandle);
}

int blc_gmcss_updateSearchResObjectId(u16 connHandle, const blc_object_id_t *objectId)
{
    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);

    return blt_gmcss_setIdChar(objectId, connHandle, gmcss->searchResultsObjectIDHandle, true);
}

static void blt_mcss_initMediaPlayerNameChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Media Player Name char too many, max num is %d", p->num);
    } else {
        mcss->mediaPlayerNameHandle = p->charHandle;
    }
}

static void blt_mcss_initMediaPlayerIconObjectIDChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Media Player Icon Object ID char too many, max num is %d", p->num);
    } else {
        mcss->mediaPlayerIconObjectIDHandle = p->charHandle;
    }
}

static void blt_mcss_initMediaPlayerIconURLChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Media Player Icon URL char too many, max num is %d", p->num);
    } else {
        mcss->mediaPlayerIconURLHandle = p->charHandle;
    }
}

static void blt_mcss_initTrackChangedChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Track Changed char too many, max num is %d", p->num);
    } else {
        mcss->trackChangedHandle = p->charHandle;
    }
}

static void blt_mcss_initTrackTitleChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Track Title char too many, max num is %d", p->num);
    } else {
        mcss->trackTitleHandle = p->charHandle;
    }
}

static void blt_mcss_initTrackDurationChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Track Duration char too many, max num is %d", p->num);
    } else {
        mcss->trackDurationHandle = p->charHandle;
    }
}

static void blt_mcss_initTrackPositionChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Track Position char too many, max num is %d", p->num);
    } else {
        mcss->trackPositionHandle = p->charHandle;
    }
}

static void blt_mcss_initPlaybackSpeedChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Playback Speed char too many, max num is %d", p->num);
    } else {
        mcss->playbackSpeedHandle = p->charHandle;
    }
}

static void blt_mcss_initSeekingSpeedChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Seeking Speed char too many, max num is %d", p->num);
    } else {
        mcss->seekingSpeedHandle = p->charHandle;
    }
}

static void blt_mcss_initCurrentTrackSegmentsObjectIDChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Current TrackSegments Object ID char too many, max num is %d", p->num);
    } else {
        mcss->currentTrackSegmentsObjectIDHandle = p->charHandle;
    }
}

static void blt_mcss_initCurrentTrackObjectIDChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Current Track Object ID char too many, max num is %d", p->num);
    } else {
        mcss->currentTrackObjectIDHandle = p->charHandle;
    }
}

static void blt_mcss_initNextTrackObjectIdChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Next Track Object ID char too many, max num is %d", p->num);
    } else {
        mcss->nextTrackObjectIDHandle = p->charHandle;
    }
}

static void blt_mcss_initParentGroupObjectIDChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Parent Group Object ID char too many, max num is %d", p->num);
    } else {
        mcss->parentGroupObjectIDHandle = p->charHandle;
    }
}

static void blt_mcss_initCurrentGroupObjectIDChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Current Group Object ID char too many, max num is %d", p->num);
    } else {
        mcss->currentGroupObjectIDHandle = p->charHandle;
    }
}

static void blt_mcss_initPlayingOrderChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Playing Order char too many, max num is %d", p->num);
    } else {
        mcss->playingOrderHandle = p->charHandle;
    }
}

static void blt_mcss_initPlayingOrdersSupportedChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Playing Orders Supported char too many, max num is %d", p->num);
    } else {
        mcss->playingOrdersSupportedHandle = p->charHandle;
    }
}

static void blt_mcss_initMediaStateChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Media State char too many, max num is %d", p->num);
    } else {
        mcss->mediaStateHandle = p->charHandle;
    }
}

static void blt_mcss_initMediaControlPointChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Media Control Point char too many, max num is %d", p->num);
    } else {
        mcss->mediaControlPointHandle = p->charHandle;
    }
}

static void blt_mcss_initMediaControlPointOpcodesSupportedChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Media Control Point Opcodes Supported char too many, max num is %d", p->num);
    } else {
        mcss->mediaControlPointOpcodesSupportedHandle = p->charHandle;
    }
}

static void blt_mcss_initSearchResultsObjectIDChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Search Results Object ID char too many, max num is %d", p->num);
    } else {
        mcss->searchResultsObjectIDHandle = p->charHandle;
    }
}

static void blt_mcss_initSearchControlPointChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Search Control Point char too many, max num is %d", p->num);
    } else {
        mcss->searchControlPointHandle = p->charHandle;
    }
}

static void blt_mcss_initContentControlIDChar(atts_foundCharParam_t *p, void *input)
{
    blc_mcs_server_t *mcss = (blc_mcs_server_t *)input;
    if (p->num) {
        BLT_MCS_LOG("ERR: Content Control ID char too many, max num is %d", p->num);
    } else {
        mcss->contentControlIDHandle = p->charHandle;
    }
}

static const atts_findCharList_t mcssChar[] = {
    {
     .charUuid    = characteristicMediaPlayerNameUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initMediaPlayerNameChar,
     },
    {
     .charUuid    = characteristicMediaPlayerIconObjectIdUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initMediaPlayerIconObjectIDChar,
     },
    {
     .charUuid    = characteristicMediaPlayerIconUrlUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initMediaPlayerIconURLChar,
     },
    {
     .charUuid    = characteristicTrackChangedUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initTrackChangedChar,
     },
    {
     .charUuid    = characteristicTrackTitleUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initTrackTitleChar,
     },
    {
     .charUuid    = characteristicTrackDurationUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initTrackDurationChar,
     },
    {
     .charUuid    = characteristicTrackPositionUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initTrackPositionChar,
     },
    {
     .charUuid    = characteristicPlaybackSpeedUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initPlaybackSpeedChar,
     },
    {
     .charUuid    = characteristicSeekingSpeedUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initSeekingSpeedChar,
     },
    {
     .charUuid    = characteristicCurrentTrackSegmentsObjectIdUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initCurrentTrackSegmentsObjectIDChar,
     },
    {
     .charUuid    = characteristicCurrentTrackObjectIdUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initCurrentTrackObjectIDChar,
     },
    {
     .charUuid    = characteristicNextTrackObjectIdUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initNextTrackObjectIdChar,
     },
    {
     .charUuid    = characteristicParentGroupObjectIdUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initParentGroupObjectIDChar,
     },
    {
     .charUuid    = characteristicCurrentGroupObjectIdUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initCurrentGroupObjectIDChar,
     },
    {
     .charUuid    = characteristicPlayingOrderUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initPlayingOrderChar,
     },
    {
     .charUuid    = characteristicPlayingOrdersSupportedUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initPlayingOrdersSupportedChar,
     },
    {
     .charUuid    = characteristicMediaStateUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initMediaStateChar,
     },
    {
     .charUuid    = characteristicMediaControlPointUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initMediaControlPointChar,
     },
    {
     .charUuid    = characteristicMediaCtrlPointOpSupportedUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initMediaControlPointOpcodesSupportedChar,
     },
    {
     .charUuid    = characteristicSearchResultsObjectIdUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initSearchResultsObjectIDChar,
     },
    {
     .charUuid    = characteristicSearchControlPointUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initSearchControlPointChar,
     },
    {
     .charUuid    = characteristicMediaPlayerIconObjectTypeUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_mcss_initContentControlIDChar,
     },
};

static void blt_mcs_print_handles(blc_mcs_server_t *mcs)
{
    BLT_MCS_LOG("Handle information:");
    BLT_MCS_LOG("Media Player Name:0x%04x, Media Player Icon Object ID:0x%04x", mcs->mediaPlayerNameHandle, mcs->mediaPlayerIconObjectIDHandle);
    BLT_MCS_LOG("Media Player Icon URL:0x%04x, Track Changed:0x%04x", mcs->mediaPlayerIconURLHandle, mcs->trackChangedHandle);
    BLT_MCS_LOG("Track Title:0x%04x, Track Duration:0x%04x", mcs->trackTitleHandle, mcs->trackDurationHandle);
    BLT_MCS_LOG("Track Position:0x%04x, Playback Speed:0x%04x", mcs->trackPositionHandle, mcs->playbackSpeedHandle);
    BLT_MCS_LOG("Seeking Speed:0x%04x, Current Track Segments Object ID:0x%04x", mcs->seekingSpeedHandle, mcs->currentTrackSegmentsObjectIDHandle);
    BLT_MCS_LOG("Current Track Object ID:0x%04x, Next Track Object ID:0x%04x", mcs->currentTrackObjectIDHandle, mcs->nextTrackObjectIDHandle);
    BLT_MCS_LOG("Parent Group Object ID:0x%04x, Current Group Object ID:0x%04x", mcs->parentGroupObjectIDHandle, mcs->currentGroupObjectIDHandle);
    BLT_MCS_LOG("Playing Order:0x%04x, Playing Orders Supported:0x%04x", mcs->playingOrderHandle, mcs->playingOrdersSupportedHandle);
    BLT_MCS_LOG("Media State:0x%04x, Media Control Point:0x%04x", mcs->mediaStateHandle, mcs->mediaControlPointHandle);
    BLT_MCS_LOG("Media Control Point Opcodes Supported:0x%04x, Search Control Point:0x%04x", mcs->mediaControlPointOpcodesSupportedHandle, mcs->searchControlPointHandle);
    BLT_MCS_LOG("Search Results Object ID:0x%04x, Content Control ID:0x%04x", mcs->searchResultsObjectIDHandle, mcs->contentControlIDHandle);
}

static void blt_gmcss_serviceInit(const blc_mcps_regParam_t *param)
{
    blc_mcp_server_t *server = blt_mcp_getServerInst(0xFFFF);
    memset((u8 *)&server->gmcs, 0, sizeof(server->gmcs));
    blc_atts_findCharacteristicByServiceUuid(serviceGenericMediaControlUuid, ATT_16_UUID_LEN, mcssChar, ARRAY_SIZE(mcssChar), &server->gmcs);
    blt_mcs_print_handles(&server->gmcs);

    const blc_mcps_regParam_t *mcpsRegParam = param;

    if (mcpsRegParam == NULL) { //use default parameters
        mcpsRegParam = &defaultMcpsParam;
    }

    blt_gmcss_setMediaPlayerName(0xFFFF, mcpsRegParam->gmcsParam.mediaPlayerName, mcpsRegParam->gmcsParam.mediaPlayerNameLen, false);
    if (mcpsRegParam->gmcsParam.mediaPlayerIconObjectIdPresent) {
        blc_gmcss_updateMediaPlayerIconObjectId(0xFFFF, &mcpsRegParam->gmcsParam.mediaPlayerIconObjectId);
    } else {
        blc_gmcss_updateMediaPlayerIconObjectId(0xFFFF, NULL);
    }
    blc_gmcss_updateMediaPlayerIconURL(0xFFFF, mcpsRegParam->gmcsParam.mediaPlayerIconUrl, mcpsRegParam->gmcsParam.mediaPlayerIconUrlLen);
    blt_gmcss_setTrackTitle(0xFFFF, mcpsRegParam->gmcsParam.trackTitle, mcpsRegParam->gmcsParam.trackTitleLen, false);
    blt_gmcss_setTrackDuration(0xFFFF, mcpsRegParam->gmcsParam.trackDuration, false);
    blt_gmcss_setTrackPosition(0xFFFF, mcpsRegParam->gmcsParam.trackPosition, false);
    blt_gmcss_setMediaState(0xFFFF, mcpsRegParam->gmcsParam.mediaState, false);
    blc_gmcss_updateContentCtrlID(0xFFFF, mcpsRegParam->gmcsParam.CCID);
    blt_gmcss_setMediaControlPointOpcodesSupported(0xFFFF, mcpsRegParam->gmcsParam.mediaControlPointOpcodesSupported, false);
    blt_gmcss_setPlaybackSpeed(0xFFFF, mcpsRegParam->gmcsParam.playbackSpeed, false);
    blt_gmcss_setSeekingSpeed(0xFFFF, mcpsRegParam->gmcsParam.seekingSpeed, false);
    blt_gmcss_setPlayingOrdersSupported(0xFFFF, mcpsRegParam->gmcsParam.playingOrdersSupported, false);
    blt_gmcss_setPlayingOrder(0xFFFF, mcpsRegParam->gmcsParam.playingOrder, false);
    if (mcpsRegParam->gmcsParam.currentTrackSegmentsObjectIdPresent) {
        blc_gmcss_updateCurrentTrackSegmentsObjectId(0xFFFF, &mcpsRegParam->gmcsParam.currentTrackSegmentsObjectId);
    } else {
        blc_gmcss_updateCurrentTrackSegmentsObjectId(0xFFFF, NULL);
    }
    if (mcpsRegParam->gmcsParam.currentTrackObjectIdPresent) {
        blc_gmcss_updateCurrentTrackObjectID(0xFFFF, &mcpsRegParam->gmcsParam.currentTrackObjectId);
    } else {
        blc_gmcss_updateCurrentTrackObjectID(0xFFFF, NULL);
    }
    if (mcpsRegParam->gmcsParam.nextTrackObjectIdPresent) {
        blc_gmcss_updateNextTrackObjectID(0xFFFF, &mcpsRegParam->gmcsParam.nextTrackObjectId);
    } else {
        blc_gmcss_updateNextTrackObjectID(0xFFFF, NULL);
    }
    if (mcpsRegParam->gmcsParam.parentGroupObjectIdPresent) {
        blc_gmcss_updateParentGroupObjectID(0xFFFF, &mcpsRegParam->gmcsParam.parentGroupObjectId);
    } else {
        blc_gmcss_updateParentGroupObjectID(0xFFFF, NULL);
    }
    if (mcpsRegParam->gmcsParam.currentGroupObjectIdPresent) {
        blc_gmcss_updateCurrentGroupObjectID(0xFFFF, &mcpsRegParam->gmcsParam.currentGroupObjectId);
    } else {
        blc_gmcss_updateCurrentGroupObjectID(0xFFFF, NULL);
    }
}

static int blt_gmcss_trackPositionWriteCback(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    blc_mcss_mediaTrackPositionEvt_t pEvt;

    if (valueLen != sizeof(s32)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    pEvt.position = *((s32 *)writeValue);

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GMCSS_TRACK_POSITION, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

typedef att_err_t (*mcs_mediaControlPointCb)(blc_gmcs_server_t *gmcss, u16 connHandle, u8 *value, u16 valueLen);

typedef struct
{
    u8                      opcode;
    u32                     opcodeSupportedMask;
    u16                     paramLenMin;
    u16                     paramLenMax;
    mcs_mediaControlPointCb cb;
} mcs_ControlPointMap_t;

static att_err_t blt_gmcss_mediaControlPointWriteCbackMoveRelative(blc_gmcs_server_t *gmcss, u16 connHandle, u8 *value, u16 valueLen)
{
    (void)gmcss;
    (void)valueLen;

    blc_mcss_mediaControlPointMoveRelativeEvt_t pEvt = {
        .opcode = BLC_MCS_OPCODE_MOVE_RELATIVE,
        .offset = *((s32 *)value),
    };

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GMCSS_MEDIA_CONTROL_POINT, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

static att_err_t blt_gmcss_mediaControlPointWriteCbackGotoSegment(blc_gmcs_server_t *gmcss, u16 connHandle, u8 *value, u16 valueLen)
{
    (void)gmcss;
    (void)valueLen;

    blc_mcss_mediaControlPointGotoSegmentEvt_t pEvt = {
        .opcode = BLC_MCS_OPCODE_GOTO_SEGMENT,
        .n      = *((s32 *)value),
    };

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GMCSS_MEDIA_CONTROL_POINT, (u8 *)&pEvt, sizeof(pEvt));
    return ATT_SUCCESS;
}

static att_err_t blt_gmcss_mediaControlPointWriteCbackGotoTrack(blc_gmcs_server_t *gmcss, u16 connHandle, u8 *value, u16 valueLen)
{
    (void)gmcss;
    (void)valueLen;

    blc_mcss_mediaControlPointGotoTrackEvt_t pEvt = {
        .opcode = BLC_MCS_OPCODE_GOTO_TRACK,
        .n      = *((s32 *)value),
    };

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GMCSS_MEDIA_CONTROL_POINT, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

static att_err_t blt_gmcss_mediaControlPointWriteCbackGotoGroup(blc_gmcs_server_t *gmcss, u16 connHandle, u8 *value, u16 valueLen)
{
    (void)gmcss;
    (void)valueLen;

    blc_mcss_mediaControlPointGotoGroupEvt_t pEvt = {
        .opcode = BLC_MCS_OPCODE_GOTO_GROUP,
        .n      = *((s32 *)value),
    };

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GMCSS_MEDIA_CONTROL_POINT, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

static const mcs_ControlPointMap_t gmcss_mediaControlPointMap[] = {
    {BLC_MCS_OPCODE_PLAY,             BLC_MCS_OPCODE_SUPPORT_PLAY,             0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_PAUSE,            BLC_MCS_OPCODE_SUPPORT_PAUSE,            0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_FAST_REWIND,      BLC_MCS_OPCODE_SUPPORT_FAST_REWIND,      0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_FAST_FORWARD,     BLC_MCS_OPCODE_SUPPORT_FAST_FORWARD,     0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_STOP,             BLC_MCS_OPCODE_SUPPORT_STOP,             0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_MOVE_RELATIVE,    BLC_MCS_OPCODE_SUPPORT_MOVE_RELATIVE,    sizeof(s32), sizeof(s32), blt_gmcss_mediaControlPointWriteCbackMoveRelative},
    {BLC_MCS_OPCODE_PREVIOUS_SEGMENT, BLC_MCS_OPCODE_SUPPORT_PREVIOUS_SEGMENT, 0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_NEXT_SEGMENT,     BLC_MCS_OPCODE_SUPPORT_NEXT_SEGMENT,     0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_FIRST_SEGMENT,    BLC_MCS_OPCODE_SUPPORT_FIRST_SEGMENT,    0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_LAST_SEGMENT,     BLC_MCS_OPCODE_SUPPORT_LAST_SEGMENT,     0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_GOTO_SEGMENT,     BLC_MCS_OPCODE_SUPPORT_GOTO_SEGMENT,     sizeof(s32), sizeof(s32), blt_gmcss_mediaControlPointWriteCbackGotoSegment },
    {BLC_MCS_OPCODE_PREVIOUS_TRACK,   BLC_MCS_OPCODE_SUPPORT_PREVIOUS_TRACK,   0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_NEXT_TRACK,       BLC_MCS_OPCODE_SUPPORT_NEXT_TRACK,       0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_FIRST_TRACK,      BLC_MCS_OPCODE_SUPPORT_FIRST_TRACK,      0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_LAST_TRACK,       BLC_MCS_OPCODE_SUPPORT_LAST_TRACK,       0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_GOTO_TRACK,       BLC_MCS_OPCODE_SUPPORT_GOTO_TRACK,       sizeof(s32), sizeof(s32), blt_gmcss_mediaControlPointWriteCbackGotoTrack   },
    {BLC_MCS_OPCODE_PREVIOUS_GROUP,   BLC_MCS_OPCODE_SUPPORT_PREVIOUS_GROUP,   0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_NEXT_GROUP,       BLC_MCS_OPCODE_SUPPORT_NEXT_GROUP,       0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_FIRST_GROUP,      BLC_MCS_OPCODE_SUPPORT_FIRST_GROUP,      0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_LAST_GROUP,       BLC_MCS_OPCODE_SUPPORT_LAST_GROUP,       0,           0,           NULL                                             },
    {BLC_MCS_OPCODE_GOTO_GROUP,       BLC_MCS_OPCODE_SUPPORT_GOTO_GROUP,       sizeof(s32), sizeof(s32), blt_gmcss_mediaControlPointWriteCbackGotoGroup   },
};

static int blt_gmcss_mediaControlPointWriteCback(blc_gmcs_server_t *gmcss, u16 connHandle, u8 *value, u16 valueLen)
{
    const mcs_ControlPointMap_t *handler = NULL;
    u32                         *supportedOpcodes;
    int                          ret = ATT_SUCCESS;
    u8                           result_code;

    if (valueLen == 0) {
        ret = ATT_ERR_INVALID_ATTR_VALUE_LEN;
        goto done;
    }

    supportedOpcodes = (u32 *)blc_gatts_getAttributeValueByHandle(connHandle, gmcss->mediaControlPointOpcodesSupportedHandle);
    if (!supportedOpcodes) {
        result_code = BLC_MCS_MEDIA_CTRL_RESULT_OP_NOT_SUPP;
        goto notify;
    }

    for (size_t i = 0; i < ARRAY_SIZE(gmcss_mediaControlPointMap); i++) {
        if (gmcss_mediaControlPointMap[i].opcode == value[0]) {
            handler = &gmcss_mediaControlPointMap[i];
            break;
        }
    }

    if (!handler || !(handler->opcodeSupportedMask & *supportedOpcodes)) {
        result_code = BLC_MCS_MEDIA_CTRL_RESULT_OP_NOT_SUPP;
        goto notify;
    }

    if (((valueLen - 1) < handler->paramLenMin) || ((valueLen - 1) > handler->paramLenMax)) {
        ret = ATT_ERR_INVALID_ATTR_VALUE_LEN;
        goto done;
    }

    if (!handler->cb) {
        blc_mcss_mediaControlPointEvt_t pEvt = {
            .opcode = value[0],
        };

        blt_prf_sendEvent(connHandle, AUDIO_EVT_GMCSS_MEDIA_CONTROL_POINT, (u8 *)&pEvt, sizeof(pEvt));

        ret = ATT_SUCCESS;
    } else {
        ret = handler->cb(gmcss, connHandle, &value[1], valueLen - 1);
    }

    goto done;
notify:
    blc_gmcss_updateMediaCtrlPoint(connHandle, value[0], result_code);
done:
    return ret;
}

static int blt_gmcss_playbackSpeedWriteCback(blc_gmcs_server_t *gmcss, u16 connHandle, u8 *value, u16 valueLen)
{
    (void)gmcss;
    blc_mcss_mediaPlaybackSpeedEvt_t pEvt;

    if (valueLen != 1) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    pEvt.speed = *((s8 *)value);

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GMCSS_PLAYBACK_SPEED, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

static int blt_gmcss_playingOrderWriteCback(blc_gmcs_server_t *gmcss, u16 connHandle, u8 *value, u16 valueLen)
{
    (void)gmcss;

    blc_mcss_mediaPlayingOrderEvt_t pEvt;

    if (valueLen != sizeof(u8)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    pEvt.order = value[0];

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GMCSS_PLAYING_ORDER, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

static int blt_gmcss_currentTrackObjectIdWriteCback(blc_gmcs_server_t *gmcss, u16 connHandle, u8 *value, u16 valueLen)
{
    (void)gmcss;

    blc_mcss_currentTrackObjectIdEvt_t pEvt;

    if (valueLen != 6) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    memcpy(pEvt.id.objectId, value, valueLen);

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GMCSS_CURRENT_TRACK_OBJECT_ID, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

static int blt_gmcss_nextTrackObjectIdWriteCback(blc_gmcs_server_t *gmcss, u16 connHandle, u8 *value, u16 valueLen)
{
    (void)gmcss;

    blc_mcss_nextTrackObjectIdEvt_t pEvt;

    if (valueLen != 6) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    memcpy(pEvt.id.objectId, value, valueLen);

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GMCSS_NEXT_TRACK_OBJECT_ID, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

static int blt_gmcss_currentGroupObjectIdWriteCback(blc_gmcs_server_t *gmcss, u16 connHandle, u8 *value, u16 valueLen)
{
    (void)gmcss;

    blc_mcss_currentGroupObjectIdEvt_t pEvt;

    if (valueLen != 6) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    memcpy(pEvt.id.objectId, value, valueLen);

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GMCSS_CURRENT_GROUP_OBJECT_ID, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

static int blt_gmcss_searchControlPointWriteCback(blc_gmcs_server_t *gmcss, u16 connHandle, u8 *value, u16 valueLen)
{
    (void)gmcss;

    u8                                       searchControlPointEvtBuf[SEARCH_CONTROL_POINT_EVT_BUF_SIZE];
    blc_mcss_currentSearchControlPointEvt_t *evt  = (blc_mcss_currentSearchControlPointEvt_t *)searchControlPointEvtBuf;
    blc_mcs_search_control_item_t           *item = evt->items;
    u16                                      size = sizeof(*evt);

    evt->numItems = 0;

    while (valueLen) {
        if (valueLen < 2) {
            // length and type fields are not present
            goto fail;
        }

        if (size + value[0] >= sizeof(searchControlPointEvtBuf)) {
            // event buffer is too small
            BLT_MCS_LOG("Search control point buffer too small");
            goto fail;
        }

        item->type        = value[1];
        item->paramLength = value[0] - 1;
        if (item->type > BLC_MCS_SEARCH_TYPE_ONLY_GROUPS ||
            item->type < BLC_MCS_SEARCH_TYPE_TRACK_NAME ||
            (valueLen - 2) < item->paramLength) {
            // parameter length is not valid
            goto fail;
        }

        memcpy(item->param, &value[2], item->paramLength);
        valueLen -= (item->paramLength + 2);
        value += (item->paramLength + 2);
        size += sizeof(*item) + item->paramLength;
        evt->numItems++;
        item = (blc_mcs_search_control_item_t *)((u8 *)item + sizeof(*item) + item->paramLength);
    }

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GMCSS_SEARCH_CONTROL_POINT, (u8 *)evt, size);
    goto done;

fail:
    // Set search result to zero
    blc_gmcss_updateSearchResObjectId(connHandle, NULL);
    // Send search control point notification
    blc_gmcss_updateSearchCtrlPoint(connHandle, BLC_MCS_SEARCH_CTRL_RESULT_FAILURE);
done:
    return ATT_SUCCESS;
}

static int blt_gmcss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    (void)opcode;

    blc_gmcs_server_t *gmcss = blt_gmcss_getServerInst(connHandle);

    if (attrHandle == gmcss->trackPositionHandle) {
        return blt_gmcss_trackPositionWriteCback(connHandle, writeValue, valueLen);
    } else if (attrHandle == gmcss->mediaControlPointHandle) {
        return blt_gmcss_mediaControlPointWriteCback(gmcss, connHandle, writeValue, valueLen);
    } else if (attrHandle == gmcss->playbackSpeedHandle) {
        return blt_gmcss_playbackSpeedWriteCback(gmcss, connHandle, writeValue, valueLen);
    } else if (attrHandle == gmcss->playingOrderHandle) {
        return blt_gmcss_playingOrderWriteCback(gmcss, connHandle, writeValue, valueLen);
    } else if (attrHandle == gmcss->currentTrackObjectIDHandle) {
        return blt_gmcss_currentTrackObjectIdWriteCback(gmcss, connHandle, writeValue, valueLen);
    } else if (attrHandle == gmcss->nextTrackObjectIDHandle) {
        return blt_gmcss_nextTrackObjectIdWriteCback(gmcss, connHandle, writeValue, valueLen);
    } else if (attrHandle == gmcss->currentGroupObjectIDHandle) {
        return blt_gmcss_currentGroupObjectIdWriteCback(gmcss, connHandle, writeValue, valueLen);
    } else if (attrHandle == gmcss->searchControlPointHandle) {
        return blt_gmcss_searchControlPointWriteCback(gmcss, connHandle, writeValue, valueLen);
    }
    return ATT_SUCCESS;
}

static int blt_gmcss_longValueReadCback(u16 connHandle, u8 opcode, u16 attrHandle, bool *valueChangedFlag, u8 **outValue, u16 *outValueLen)
{
    u8  *value = NULL;
    u16 *len   = NULL;

    // Treat blob reaquest as always non-zero offset read
    if (opcode == ATT_OP_READ_BLOB_REQ) {
        if (*valueChangedFlag) {
            return GMCS_ERRCODE_VALUE_CHANGED_DURING_READ_LONG;
        }
    } else {
        *valueChangedFlag = false;
    }

    blc_gatts_getAttributeInformationByHandle(connHandle, attrHandle, &value, &len);
    if (!value) {
        return ATT_ERR_UNLIKELY_ERR;
    }

    *outValueLen = *len;
    *outValue    = value;

    return ATT_SUCCESS;
}

static mcs_value_changed_conn_t *blt_gmcss_getValueChanged(blc_gmcs_server_t *gmcss, u16 connHandle)
{
    for (u8 i = 0; i < ARRAY_SIZE(gmcss->valueChanged); i++) {
        if (gmcss->valueChanged[i].active && gmcss->valueChanged[i].connHandle == connHandle) {
            return &gmcss->valueChanged[i];
        }
    }

    return NULL;
}

static int blt_gmcss_readCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 **outValue, u16 *outValueLen)
{
    blc_gmcs_server_t        *gmcss        = blt_gmcss_getServerInst(connHandle);
    mcs_value_changed_conn_t *valueChanged = blt_gmcss_getValueChanged(gmcss, connHandle);
    bool                      valueChangedFlag;
    int                       ret = ATT_SUCCESS;

    if (!valueChanged) {
        // Shouldn't happen
        return ATT_ERR_UNLIKELY_ERR;
    }

    if (attrHandle == gmcss->mediaPlayerNameHandle) {
        valueChangedFlag                     = valueChanged->mediaPlayerNameChanged;
        ret                                  = blt_gmcss_longValueReadCback(connHandle, opcode, attrHandle, &valueChangedFlag, outValue, outValueLen);
        valueChanged->mediaPlayerNameChanged = valueChangedFlag;
    } else if (attrHandle == gmcss->trackTitleHandle) {
        valueChangedFlag                = valueChanged->trackTitleChanged;
        ret                             = blt_gmcss_longValueReadCback(connHandle, opcode, attrHandle, &valueChangedFlag, outValue, outValueLen);
        valueChanged->trackTitleChanged = valueChangedFlag;
    }

    return ret;
}
