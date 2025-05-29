/********************************************************************************************************
 * @file    mcs_client.c
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


static int  blt_mcp_disconnect(u16 connHandle);
static void blt_gmcsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

static const blc_gapc_discList_t discMcp;
#define BLC_GMCS_START_SDP(connHandle) blc_gapc_registerDiscoveryService(connHandle, &discMcp)

static const blc_gapc_reconnList_t reconnMcp;
#define BLC_GMCS_START_RECONN(connHandle) blc_gapc_registerReconnectService(connHandle, &reconnMcp)

_attribute_ble_data_retention_
    blc_mcp_client_ctrl_t mcp_client_ctrl = {
        .process = {
                    .pNext       = NULL,
                    .id          = AUDIO_GMCS_CLIENT,
                    .usedAclRole = 0,
                    .init        = blt_mcp_init,
                    .connect     = blt_mcp_connect,
                    .discov      = blt_mcp_discovery,
                    .loop        = NULL,
                    .store       = blt_mcp_nv_store,
                    },
};

void blc_audio_registerMediaControlClient(const blc_mcpc_regParam_t *param)
{
    blc_prf_registerServiceModule(BLT_GMCS_PTS_BQB_EN ? PRF_GAP_ACL_UNSPECIF : PRF_GAP_ACL_PERIPHERAL, (blc_prf_proc_t *)&mcp_client_ctrl, param);
}

int blt_mcp_init(u8 initType, const void *param)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_mcp_client_t)), blc_mcp_client_t);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_gmcs_client_t)), blc_gmcs_client_t);
#endif
    (void)param;

    if (initType == PRF_PROC_INIT) {
        for (int i = 0; i < gAppAudioAclMaxNum; i++) {
            mcp_client_ctrl.pMcpClient[i] = blt_mcp_getClientcontrolBuffer(i);
            /* Clear VCS Client parameters  */
            memset(mcp_client_ctrl.pMcpClient[i], 0, sizeof(blc_mcp_client_t));
            /* Initialize Pointer buffer */
        }
    } else if (initType == PRF_PROC_DEINIT) {
    }
    return 0;
}

blc_mcp_client_t *blt_mcp_getClientInst(u16 connHandle)
{
    int ret = blt_prf_getAclRole(connHandle);
    if (ret < 0 || ((!BLT_GMCS_PTS_BQB_EN) && ret == ACL_ROLE_CENTRAL)) {
        BLT_MCS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if (ret >= 0) {
            /* Media Control Client : GAP and GAP Peripheral  */
            /* CAP_v1.0.pdf Page15: MCP Media Control Client GAP Peripheral */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_GMCS_CLIENT, ret);
        }

        return NULL;
    }

    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return mcp_client_ctrl.pMcpClient[idx];
}

blc_gmcs_client_t *blt_gmcsc_getClientInst(u16 connHandle)
{
    blc_mcp_client_t *client = blt_mcp_getClientInst(connHandle);
    if (client == NULL) {
        return NULL;
    }
    return &client->gmcs;
}

int blt_mcp_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if (connState == PRF_ACL_STATE_DISCONN) {
        blt_mcp_disconnect(connHandle);
        BLT_MCS_LOG("Disconnect: 0x%x", connHandle);
    } else {
        BLT_MCS_LOG("Connect: 0x%x", connHandle);
    }
    return 0;
}

int blt_mcp_discovery(u16 connHandle)
{
    BLC_COMMON_SDP_DISCOVERY(connHandle, GMCS, gmcs);
}

int blt_mcp_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param)
{
    BLC_COMMON_NV_STORE(connHandle, GMCS, gmcs, contentControlIDHdl);
    return 0;
}

static int blt_mcp_disconnect(u16 connHandle)
{
    if (blt_ll_isAclHandleOutOfRange(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_mcp_client_t *mcp = blt_mcp_getClientInst(connHandle);

    for (int i = 0; i < mcp->mcsClientCount; i++) {
        memset(mcp->mcs[i], 0, sizeof(blc_mcs_client_t));
    }

    memset(mcp, 0, sizeof(blc_mcp_client_t));

    return BLE_SUCCESS;
}

void blt_gmcsc_recvMediaPlayerNameNtf(u16 connHandle, u8 *val, u16 valLen)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    if (valLen > 50) {
        valLen = 50;
    }
    client->mediaPlayerNameLen = valLen;
    memcpy(client->mediaPlayerName, val, valLen);

    blc_mcsc_mediaPlayerNameEvt_t pEvt =
        {
            .connHandle   = connHandle,
            .mediaNameLen = valLen,
        };
    memcpy(pEvt.mediaName, val, valLen);
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_PLAYER_NAME, (u8 *)&pEvt, sizeof(blc_mcsc_mediaPlayerNameEvt_t));
}

void blt_gmcsc_recvMediaTrackChangedNtf(u16 connHandle, u8 *val, u16 valLen)
{
    (void)val;
    (void)valLen;
    blc_mcsc_mediaTrackChangedEvt_t pEvt =
        {
            .connHandle = connHandle,
        };
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_TRACK_CHANGED, (u8 *)&pEvt, sizeof(blc_mcsc_mediaTrackChangedEvt_t));
}

void blt_gmcsc_recvMediaTrackTitleNtf(u16 connHandle, u8 *val, u16 valLen)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    if (valLen > 50) {
        valLen = 50;
    }
    client->trackTitleLen = valLen;
    memcpy(client->trackTitle, val, valLen);

    blc_mcsc_mediaTrackTitleEvt_t pEvt =
        {
            .connHandle    = connHandle,
            .trackTitleLen = valLen,
        };
    memcpy(pEvt.trackTitle, val, valLen);
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_TRACK_TITLE, (u8 *)&pEvt, sizeof(blc_mcsc_mediaTrackTitleEvt_t));
}

void blt_gmcsc_recvMediaTrackDurationNtf(u16 connHandle, u8 *val, u16 valLen)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    if (valLen != 4) {
        return;
    }

    memcpy((u8 *)&client->trackDuration, val, valLen);

    blc_mcsc_mediaTrackDurationEvt_t pEvt =
        {
            .connHandle = connHandle,
        };
    memcpy((u8 *)&pEvt.trackDuration, val, valLen);
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_TRACK_DURATION, (u8 *)&pEvt, sizeof(blc_mcsc_mediaTrackDurationEvt_t));
}

void blt_gmcsc_recvMediaTrackPositionNtf(u16 connHandle, u8 *val, u16 valLen)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    if (valLen != 4) {
        return;
    }
    memcpy((u8 *)&client->trackPosition, val, valLen);

    blc_mcsc_mediaTrackPositionEvt_t pEvt =
        {
            .connHandle = connHandle,
        };
    memcpy((u8 *)&pEvt.trackPosition, val, valLen);
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_TRACK_POSITION, (u8 *)&pEvt, sizeof(blc_mcsc_mediaTrackPositionEvt_t));
}

void blt_gmcsc_recvMediaPlaybackSpeedNtf(u16 connHandle, u8 *val, u16 valLen)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    if (valLen != 1) {
        return;
    }
    client->playbackSpeed = (s8)val[0];
    blc_mcsc_mediaPlaybackSpeedEvt_t pEvt =
        {
            .connHandle    = connHandle,
            .playbackSpeed = (s8)val[0],
        };

    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_PLAYBACK_SPEED, (u8 *)&pEvt, sizeof(blc_mcsc_mediaPlaybackSpeedEvt_t));
}

void blt_gmcsc_recvMediaSeekingSpeedNtf(u16 connHandle, u8 *val, u16 valLen)
{
    (void)valLen;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    client->seekingSpeed = (s8)val[0];
    blc_mcsc_mediaSeekingSpeedEvt_t pEvt =
        {
            .connHandle   = connHandle,
            .seekingSpeed = (s8)val[0],
        };

    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_SEEKING_SPEED, (u8 *)&pEvt, sizeof(blc_mcsc_mediaSeekingSpeedEvt_t));
}

void blt_gmcsc_recvMediaCurrentTrackObjectIdNtf(u16 connHandle, u8 *val, u16 valLen)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    if (valLen != 6) {
        return;
    }
    memcpy(client->currentTrackID.objectId, val, valLen);
    blc_mcsc_mediaCurrentTrackObjectIdEvt_t pEvt =
        {
            .connHandle = connHandle,
        };
    memcpy(pEvt.object.objectId, val, valLen);
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_CURRENT_TRACK_OBJECT_ID, (u8 *)&pEvt, sizeof(blc_mcsc_mediaCurrentTrackObjectIdEvt_t));
}

void blt_gmcsc_recvMediaNextTrackObjectIdNtf(u16 connHandle, u8 *val, u16 valLen)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    if (valLen != 6) {
        return;
    }
    memcpy(client->nextTrackID.objectId, val, valLen);
    blc_mcsc_mediaNextTrackObjectIdEvt_t pEvt =
        {
            .connHandle = connHandle,
        };
    memcpy(pEvt.object.objectId, val, valLen);
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_NEXT_TRACK_OBJECT_ID, (u8 *)&pEvt, sizeof(blc_mcsc_mediaNextTrackObjectIdEvt_t));
}

void blt_gmcsc_recvMediaParentGroupObjectIdNtf(u16 connHandle, u8 *val, u16 valLen)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    if (valLen != 6) {
        return;
    }
    memcpy(client->parentGroupID.objectId, val, valLen);
    blc_mcsc_mediaParentGroupObjectIdEvt_t pEvt =
        {
            .connHandle = connHandle,
        };
    memcpy(pEvt.object.objectId, val, valLen);
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_PARENT_GROUP_OBJECT_ID, (u8 *)&pEvt, sizeof(blc_mcsc_mediaParentGroupObjectIdEvt_t));
}

void blt_gmcsc_recvMediaCurrentGroupObjectIdNtf(u16 connHandle, u8 *val, u16 valLen)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    if (valLen != 6) {
        return;
    }
    memcpy(client->currentGroupID.objectId, val, valLen);
    blc_mcsc_mediaCurrentGroupObjectIdEvt_t pEvt =
        {
            .connHandle = connHandle,
        };
    memcpy(pEvt.object.objectId, val, valLen);
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_CURRENT_GROUP_OBJECT_ID, (u8 *)&pEvt, sizeof(blc_mcsc_mediaCurrentGroupObjectIdEvt_t));
}

void blt_gmcsc_recvMediaPlayingOrderNtf(u16 connHandle, u8 *val, u16 valLen)
{
    (void)valLen;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    u8                 order  = val[0];
    if (order > BLC_MCS_PLAYING_ORDER_SHUFFLE_REPEAT) {
        order = BLC_MCS_PLAYING_ORDER_RFU;
    }
    client->playingOrder = order;
    blc_mcsc_mediaPlayingOrderEvt_t pEvt =
        {
            .connHandle = connHandle,
            .order      = order,
        };
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_PLAYING_ORDER, (u8 *)&pEvt, sizeof(blc_mcsc_mediaPlayingOrderEvt_t));
}

void blt_gmcsc_recvMediaStateNtf(u16 connHandle, u8 *val, u16 valLen)
{
    (void)valLen;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    u8                 state  = val[0];
    if (state > GMCS_MEDIA_STATE_SEEKING) {
        state = GMCS_MEDIA_STATE_RFU;
    }

    client->mediaState = state;
    blc_mcsc_mediaStateEvt_t pEvt =
        {
            .connHandle = connHandle,
            .state      = state,
        };
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_STATE, (u8 *)&pEvt, sizeof(blc_mcsc_mediaStateEvt_t));
}

void blt_gmcsc_recvMediaCtrlPointNtf(u16 connHandle, u8 *val, u16 valLen)
{
    if (valLen < 2) {
        return;
    }
    blc_mcsc_mediaCtrlResultEvt_t pEvt = {
        .connHandle = connHandle,
        .op         = val[0],
        .result     = val[1],
    };
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_CTRL_RESULT, (u8 *)&pEvt, sizeof(blc_mcsc_mediaCtrlResultEvt_t));
}

void blt_gmcsc_recvMediaCtrlOpcodeSupportNtf(u16 connHandle, u8 *val, u16 valLen)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    if (valLen != 4) {
        return;
    }
    memcpy((u8 *)&client->mediaControlPointOpSupp, val, valLen);

    blc_mcsc_mediaCtrlOpcodeSupportEvt_t pEvt = {
        .connHandle = connHandle,
    };
    memcpy((u8 *)&pEvt.supportOpcode, val, valLen);
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_MEDIA_CTRL_OPCODE_SUPPORT, (u8 *)&pEvt, sizeof(blc_mcsc_mediaCtrlOpcodeSupportEvt_t));
}

void blt_gmcsc_recvSearchCtrlPointNtf(u16 connHandle, u8 *val, u16 valLen)
{
    if (valLen < 1) {
        return;
    }
    blc_mcsc_searchCtrlResultEvt_t pEvt = {
        .connHandle = connHandle,
        .result     = val[0],
    };
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_SEARCH_CTRL_RESULT, (u8 *)&pEvt, sizeof(blc_mcsc_searchCtrlResultEvt_t));
}

void blt_gmcsc_recvSearchResultsObjectIdNtf(u16 connHandle, u8 *val, u16 valLen)
{
    if (valLen != 6) {
        return;
    }

    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    memcpy(client->searchResultsID.objectId, val, valLen);

    blc_mcsc_searchResultObjectIdEvt_t pEvt = {
        .connHandle = connHandle,
    };
    memcpy(pEvt.object.objectId, val, valLen);
    blt_prf_sendEvent(connHandle, AUDIO_EVT_MCSC_SEARCH_RESULT_OBJECT_ID, (u8 *)&pEvt, sizeof(blc_mcsc_searchResultObjectIdEvt_t));
}

void blt_gmcsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    BLT_MCS_LOG("AttrHandle is 0x%x, val is %s", attHdl, hex_to_str(val, valLen));
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    if (client->mediaPlayerNameHdl == attHdl) {
        blt_gmcsc_recvMediaPlayerNameNtf(connHandle, val, valLen);
    } else if (client->trackChangedHdl == attHdl) {
        blt_gmcsc_recvMediaTrackChangedNtf(connHandle, val, valLen);
    } else if (client->trackTitleHdl == attHdl) {
        blt_gmcsc_recvMediaTrackTitleNtf(connHandle, val, valLen);
    } else if (client->trackDurationHdl == attHdl) {
        blt_gmcsc_recvMediaTrackDurationNtf(connHandle, val, valLen);
    } else if (client->trackPositionHdl == attHdl) {
        blt_gmcsc_recvMediaTrackPositionNtf(connHandle, val, valLen);
    } else if (client->playbackSpeedHdl == attHdl) {
        blt_gmcsc_recvMediaPlaybackSpeedNtf(connHandle, val, valLen);
    } else if (client->seekingSpeedHdl == attHdl) {
        blt_gmcsc_recvMediaSeekingSpeedNtf(connHandle, val, valLen);
    } else if (client->currentTrackObjectIDHdl == attHdl) {
        blt_gmcsc_recvMediaSeekingSpeedNtf(connHandle, val, valLen);
    } else if (client->nextTrackObjectIDHdl == attHdl) {
        blt_gmcsc_recvMediaNextTrackObjectIdNtf(connHandle, val, valLen);
    } else if (client->parentGroupObjectIDHdl == attHdl) {
        blt_gmcsc_recvMediaParentGroupObjectIdNtf(connHandle, val, valLen);
    } else if (client->currentGroupObjectIDHd == attHdl) {
        blt_gmcsc_recvMediaCurrentGroupObjectIdNtf(connHandle, val, valLen);
    } else if (client->playingOrderHdl == attHdl) {
        blt_gmcsc_recvMediaPlayingOrderNtf(connHandle, val, valLen);
    } else if (client->mediaStateHdl == attHdl) {
        blt_gmcsc_recvMediaStateNtf(connHandle, val, valLen);
    } else if (client->mediaControlPointHdl == attHdl) {
        blt_gmcsc_recvMediaCtrlPointNtf(connHandle, val, valLen);
    } else if (client->mediaControlPointOpSuppHdl == attHdl) {
        blt_gmcsc_recvMediaCtrlOpcodeSupportNtf(connHandle, val, valLen);
    } else if (client->searchControlPointHdl == attHdl) {
        blt_gmcsc_recvSearchCtrlPointNtf(connHandle, val, valLen);
    } else if (client->searchResultsObjectIDHdl == attHdl) {
        blt_gmcsc_recvSearchResultsObjectIdNtf(connHandle, val, valLen);
    }
}

static const char *blt_gmcsc_getObjectID(blc_object_id_t *id)
{
    u8 idTemp[6];
    for (int i = 0; i < 6; i++) {
        idTemp[5 - i] = id->objectId[i];
    }
    return hex_to_str(idTemp, 6);
}

static void blt_gmcsc_displayInfo(u16 connHandle, blc_gmcs_client_t *client)
{
    BLT_MCS_LOG("sdp over connHandle[0x%x]", connHandle);
    BLT_MCS_LOG("    INFO:Media player name is %.*s", client->mediaPlayerNameLen, client->mediaPlayerName);
    BLT_MCS_LOG("    INFO:Media player icon object ID is %s", blt_gmcsc_getObjectID(&client->mediaPlayerIconID));
    BLT_MCS_LOG("    INFO:Media player icon URL is %.*s", client->mediaPlayerIconURLLen, client->mediaPlayerIconURL);
    BLT_MCS_LOG("    INFO:Track Title is %s Track Duration is %0.2fs Track Position is %0.2fs", hex_to_str(client->trackTitle, client->trackTitleLen), client->trackDuration * 0.01, client->trackPosition * 0.01);
    BLT_MCS_LOG("    INFO:Playback speed is %d, Seeking speed is %d", client->playbackSpeed, client->seekingSpeed);
    BLT_MCS_LOG("    INFO:Some Object ID: current Track Segments[%s]", blt_gmcsc_getObjectID(&client->currentTrackSegID));
    BLT_MCS_LOG("        current Track[%s]", blt_gmcsc_getObjectID(&client->currentTrackID));
    BLT_MCS_LOG("        next Track[%s]", blt_gmcsc_getObjectID(&client->nextTrackID));
    BLT_MCS_LOG("        parent Group[%s]", blt_gmcsc_getObjectID(&client->parentGroupID));
    BLT_MCS_LOG("        current Group[%s]", blt_gmcsc_getObjectID(&client->currentGroupID));

    BLT_MCS_LOG("    INFO:Playing order: 0x%x, orderSupp:0x%04x", client->playingOrder, client->playingOrderSupp);
    BLT_MCS_LOG("    INFO:Media State is 0x%x, media Opcodes Supp:0x%08x", client->mediaState, client->mediaControlPointOpSupp);
    BLT_MCS_LOG("    INFO:Search Results Object ID is %s, CCID is %02x", blt_gmcsc_getObjectID(&client->searchResultsID), client->ccid[1]);

    //  BLT_MCS_LOG(" mediaPlayerNameHdl[0x%x]", client->mediaPlayerNameHdl);
    //  BLT_MCS_LOG(" mediaPlayerIconObjectIDHdl[0x%x]", client->mediaPlayerIconObjectIDHdl);
    //  BLT_MCS_LOG(" mediaPlayerIconURLHdl[0x%x]", client->mediaPlayerIconURLHdl);
    //  BLT_MCS_LOG(" trackChangedHdl[0x%x]", client->trackChangedHdl);
    //  BLT_MCS_LOG(" trackTitleHdl[0x%x]", client->trackTitleHdl);
    //  BLT_MCS_LOG(" trackDurationHdl[0x%x]", client->trackDurationHdl);
    //  BLT_MCS_LOG(" trackPositionHdl[0x%x]", client->trackPositionHdl);
    //  BLT_MCS_LOG(" playbackSpeedHdl[0x%x]", client->playbackSpeedHdl);
    //  BLT_MCS_LOG(" seekingSpeedHdl[0x%x]", client->seekingSpeedHdl);
    //  BLT_MCS_LOG(" currTrackSegObjectIDHdl[0x%x]", client->currentTrackSegObjectIDHdl);
    //  BLT_MCS_LOG(" currTrackObjectIDHdl[0x%x]", client->currentTrackObjectIDHdl);
    //  BLT_MCS_LOG(" parentGroupObjectIDHdl[0x%x]", client->parentGroupObjectIDHdl);
    //  BLT_MCS_LOG(" currGroupObjectIDHd[0x%x]", client->currentGroupObjectIDHd);
    //  BLT_MCS_LOG(" playingOrderHdl[0x%x]", client->playingOrderHdl);
    //  BLT_MCS_LOG(" playingOrdersSupportedHdl[0x%x]", client->playingOrdersSupportedHdl);
    //  BLT_MCS_LOG(" mediaStateHdl[0x%x]", client->mediaStateHdl);
    //  BLT_MCS_LOG(" mediaCtrlPntHdl[0x%x]", client->mediaControlPointHdl);
    //  BLT_MCS_LOG(" mediaCtrlPntOpSuppHdl[0x%x]", client->mediaControlPointOpSuppHdl);
    //  BLT_MCS_LOG(" searchCtrlPntHdl[0x%x]", client->searchControlPointHdl);
    //  BLT_MCS_LOG(" searchResultsObjectIDHdl[0x%x]", client->searchResultsObjectIDHdl);
    //  BLT_MCS_LOG(" contentCtrlIDHdl[0x%x]", client->contentControlIDHdl);
}

static void blt_gmcsc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    if (count == 0xFF) {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, AUDIO_GMCS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_MCS_LOG("ERR:not found GMCS");
        return;
    }

    if (count == 0) {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_GMCS_CLIENT);
        blt_gmcsc_displayInfo(connHandle, client);
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return;
    }

    client->ntfInput.startHdl     = startHandle;
    client->ntfInput.endHdl       = endHandle;
    client->ntfInput.ntfOrIndFunc = blt_gmcsc_dataInput;
    BLT_MCS_LOG("   INFO: GMCS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, AUDIO_GMCS_CLIENT, startHandle, endHandle);
}

static void blt_gmcsc_foundMediaPlayerNameChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client  = blt_gmcsc_getClientInst(connHandle);
    client->mediaPlayerNameHdl = valueHandle;
    BLT_MCS_LOG("media player name ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_mediaPlayerNameStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->mediaPlayerName[0];
    *readLen                  = &client->mediaPlayerNameLen;
    *readMaxSize              = sizeof(client->mediaPlayerName);
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundMediaPlayerIconObjectIdChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client          = blt_gmcsc_getClientInst(connHandle);
    client->mediaPlayerIconObjectIDHdl = valueHandle;
    BLT_MCS_LOG("media player icon object id ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_mediaPlayerIconObjectIdStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->mediaPlayerIconID;
    *readLen                  = NULL;
    *readMaxSize              = sizeof(blc_object_id_t);
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundMediaPlayerIconUrlChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client     = blt_gmcsc_getClientInst(connHandle);
    client->mediaPlayerIconURLHdl = valueHandle;
    BLT_MCS_LOG("media player icon URL ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_mediaPlayerIconUrlStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->mediaPlayerIconURL[0];
    *readLen                  = &client->mediaPlayerIconURLLen;
    *readMaxSize              = sizeof(client->mediaPlayerIconURL);
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundTrackChangedChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    client->trackChangedHdl   = valueHandle;
    BLT_MCS_LOG("track changed ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_foundTrackTitleChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    client->trackTitleHdl     = valueHandle;
    BLT_MCS_LOG("track title ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_trackTitleStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->trackTitle[0];
    *readLen                  = &client->trackTitleLen;
    *readMaxSize              = sizeof(client->trackTitleLen);
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundTrackDurationChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    client->trackDurationHdl  = valueHandle;
    BLT_MCS_LOG("track duration ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_trackDurationStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->trackDuration;
    *readLen                  = NULL;
    *readMaxSize              = 4;
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundTrackPositionChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    client->trackPositionHdl  = valueHandle;
    BLT_MCS_LOG("track position ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_trackPositionStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->trackPosition;
    *readLen                  = NULL;
    *readMaxSize              = 4;
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundPlaybackSpeedChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    client->playbackSpeedHdl  = valueHandle;
    BLT_MCS_LOG("playback speed ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_playbackSpeedStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->playbackSpeed;
    *readLen                  = NULL;
    *readMaxSize              = 1;
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundSeekingSpeedChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    client->seekingSpeedHdl   = valueHandle;
    BLT_MCS_LOG("seeking speed ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_seekingSpeedStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->seekingSpeed;
    *readLen                  = NULL;
    *readMaxSize              = 1;
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundCurrentTrackSegmentsObjectIdChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client          = blt_gmcsc_getClientInst(connHandle);
    client->currentTrackSegObjectIDHdl = valueHandle;
    BLT_MCS_LOG("current track segments object id ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_currentTrackSegmentsObjectIdStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->currentTrackSegID;
    *readLen                  = NULL;
    *readMaxSize              = sizeof(blc_object_id_t);
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundCurrentTrackObjectIdChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client       = blt_gmcsc_getClientInst(connHandle);
    client->currentTrackObjectIDHdl = valueHandle;
    BLT_MCS_LOG("current object id ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_currentTrackObjectIdStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->currentTrackID;
    *readLen                  = NULL;
    *readMaxSize              = sizeof(blc_object_id_t);
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundNextObjectIdChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client    = blt_gmcsc_getClientInst(connHandle);
    client->nextTrackObjectIDHdl = valueHandle;
    BLT_MCS_LOG("next object id ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_nextObjectIdStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->nextTrackID;
    *readLen                  = NULL;
    *readMaxSize              = sizeof(blc_object_id_t);
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundParentGroupObjectIdChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client      = blt_gmcsc_getClientInst(connHandle);
    client->parentGroupObjectIDHdl = valueHandle;
    BLT_MCS_LOG("parent group object id ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_parentGroupObjectIdStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->parentGroupID;
    *readLen                  = NULL;
    *readMaxSize              = sizeof(blc_object_id_t);
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundCurrentGroupObjectIdChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client      = blt_gmcsc_getClientInst(connHandle);
    client->currentGroupObjectIDHd = valueHandle;
    BLT_MCS_LOG("current group object id ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_currentGroupObjectIdStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->currentGroupID;
    *readLen                  = NULL;
    *readMaxSize              = sizeof(blc_object_id_t);
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundPlayingOrderChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    client->playingOrderHdl   = valueHandle;
    BLT_MCS_LOG("playing order ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_playingOrderStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->playingOrder;
    *readLen                  = NULL;
    *readMaxSize              = 1;
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundPlayingOrdersSupportedChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client         = blt_gmcsc_getClientInst(connHandle);
    client->playingOrdersSupportedHdl = valueHandle;
    BLT_MCS_LOG("playing order supported ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_playingOrdersSupportedStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->playingOrderSupp;
    *readLen                  = NULL;
    *readMaxSize              = 2;
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundMediaStateChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    client->mediaStateHdl     = valueHandle;
    BLT_MCS_LOG("media state ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_mediaStateStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->mediaState;
    *readLen                  = NULL;
    *readMaxSize              = 1;
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundMediaControlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client    = blt_gmcsc_getClientInst(connHandle);
    client->mediaControlPointHdl = valueHandle;
    BLT_MCS_LOG("media control point ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_foundMediaControlPointOpcodesSupportedChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client          = blt_gmcsc_getClientInst(connHandle);
    client->mediaControlPointOpSuppHdl = valueHandle;
    BLT_MCS_LOG("opcodes supported ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_mediaControlPointOpcodesSupportedStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->mediaControlPointOpSupp;
    *readLen                  = NULL;
    *readMaxSize              = 4;
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundSearchControlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client     = blt_gmcsc_getClientInst(connHandle);
    client->searchControlPointHdl = valueHandle;
    BLT_MCS_LOG("search control point ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_foundSearchResultsObjectIdChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client        = blt_gmcsc_getClientInst(connHandle);
    client->searchResultsObjectIDHdl = valueHandle;
    BLT_MCS_LOG("search results object id ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_searchResultsObjectIdStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->searchResultsID;
    *readLen                  = NULL;
    *readMaxSize              = sizeof(blc_search_control_point_t);
    *rdCbFunc                 = NULL;
}

static void blt_gmcsc_foundContentControlIdChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_gmcs_client_t *client   = blt_gmcsc_getClientInst(connHandle);
    client->contentControlIDHdl = valueHandle;
    BLT_MCS_LOG("content control id ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_gmcsc_contentControlIdStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->ccid[0];
    *readLen                  = NULL;
    *readMaxSize              = sizeof(client->ccid);
    *rdCbFunc                 = NULL;
}

static const blc_gapc_discService_t gmcsService = {
    .uuid = UUID16_INIT(SERVICE_UUID_GENERIC_MEDIA_CONTROL),
    .sfun = blt_gmcsc_foundService,
};

static const blc_gapc_discChar_t gmcsChar[] = {
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_MEDIA_PLAYER_NAME),
     .cfun         = blt_gmcsc_foundMediaPlayerNameChar,
     .rfun         = blt_gmcsc_mediaPlayerNameStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_MEDIA_PLAYER_ICON_OBJECT_ID),
     .cfun      = blt_gmcsc_foundMediaPlayerIconObjectIdChar,
     .rfun      = blt_gmcsc_mediaPlayerIconObjectIdStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_MEDIA_PLAYER_ICON_URL),
     .cfun      = blt_gmcsc_foundMediaPlayerIconUrlChar,
     .rfun      = blt_gmcsc_mediaPlayerIconUrlStartRead,
     },
    {
     .subscribeNtf = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_TRACK_CHANGED),
     .cfun         = blt_gmcsc_foundTrackChangedChar,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_TRACK_TITLE),
     .cfun         = blt_gmcsc_foundTrackTitleChar,
     .rfun         = blt_gmcsc_trackTitleStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_TRACK_DURATION),
     .cfun         = blt_gmcsc_foundTrackDurationChar,
     .rfun         = blt_gmcsc_trackDurationStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_TRACK_POSITION),
     .cfun         = blt_gmcsc_foundTrackPositionChar,
     .rfun         = blt_gmcsc_trackPositionStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_PLAYBACK_SPEED),
     .cfun         = blt_gmcsc_foundPlaybackSpeedChar,
     .rfun         = blt_gmcsc_playbackSpeedStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_SEEKING_SPEED),
     .cfun         = blt_gmcsc_foundSeekingSpeedChar,
     .rfun         = blt_gmcsc_seekingSpeedStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_CURRENT_TRACK_SEGMENTS_OBJECT_ID),
     .cfun      = blt_gmcsc_foundCurrentTrackSegmentsObjectIdChar,
     .rfun      = blt_gmcsc_currentTrackSegmentsObjectIdStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_CURRENT_TRACK_OBJECT_ID),
     .cfun         = blt_gmcsc_foundCurrentTrackObjectIdChar,
     .rfun         = blt_gmcsc_currentTrackObjectIdStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_NEXT_TRACK_OBJECT_ID),
     .cfun         = blt_gmcsc_foundNextObjectIdChar,
     .rfun         = blt_gmcsc_nextObjectIdStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_PARENT_GROUP_OBJECT_ID),
     .cfun         = blt_gmcsc_foundParentGroupObjectIdChar,
     .rfun         = blt_gmcsc_parentGroupObjectIdStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_CURRENT_GROUP_OBJECT_ID),
     .cfun         = blt_gmcsc_foundCurrentGroupObjectIdChar,
     .rfun         = blt_gmcsc_currentGroupObjectIdStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_PLAYING_ORDER),
     .cfun         = blt_gmcsc_foundPlayingOrderChar,
     .rfun         = blt_gmcsc_playingOrderStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_PLAYING_ORDERS_SUPPORTED),
     .cfun      = blt_gmcsc_foundPlayingOrdersSupportedChar,
     .rfun      = blt_gmcsc_playingOrdersSupportedStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_MEDIA_STATE),
     .cfun         = blt_gmcsc_foundMediaStateChar,
     .rfun         = blt_gmcsc_mediaStateStartRead,
     },
    {
     .subscribeNtf = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_MEDIA_CONTROL_POINT),
     .cfun         = blt_gmcsc_foundMediaControlPointChar,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_MEDIA_CTRL_POINT_OP_SUPPORTED),
     .cfun         = blt_gmcsc_foundMediaControlPointOpcodesSupportedChar,
     .rfun         = blt_gmcsc_mediaControlPointOpcodesSupportedStartRead,
     },
    {
     .subscribeNtf = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_SEARCH_CONTROL_POINT),
     .cfun         = blt_gmcsc_foundSearchControlPointChar,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_SEARCH_RESULTS_OBJECT_ID),
     .cfun         = blt_gmcsc_foundSearchResultsObjectIdChar,
     .rfun         = blt_gmcsc_searchResultsObjectIdStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_CONTENT_CONTROL_ID),
     .cfun      = blt_gmcsc_foundContentControlIdChar,
     .rfun      = blt_gmcsc_contentControlIdStartRead,
     },
};

//extern const blc_gapc_discInclude_t discOts;

static const blc_gapc_discList_t discMcp = {
    .maxServiceCount = 1,
    .service         = &gmcsService,
    //    .includeTable = {
    //        .size = 1,
    //      .include[0] = &discOts,
    //    },
    .characteristicTable = {
                            .size           = ARRAY_SIZE(gmcsChar),
                            .characteristic = gmcsChar,
                            },
};

/**********reconnect function start*********/
static bool blt_gmcsc_reconnService(u16 connHandle, int count)
{
    if (count == 0) {
        blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
        blt_gmcsc_displayInfo(connHandle, client);
        BLT_MCS_LOG("  INFO: GMCS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_GMCS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    if (count > 1) {
        return false;
    }
    return true;
}

static int blt_gmcsc_mediaPlayerNameGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->mediaPlayerNameHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_mediaPlayerIconObjectIdGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->mediaPlayerIconObjectIDHdl;

    return 1;
}

static int blt_gmcsc_mediaPlayerIconUrlGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->mediaPlayerIconURLHdl;

    return 1;
}

static int blt_gmcsc_trackChangedGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    (void)connHandle;
    charInfo->properties = CHAR_PROP_NOTIFY;
    charInfo->cccHandle  = 0;

    return 1;
}

static int blt_gmcsc_trackTitleGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->trackTitleHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_trackDurationGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->trackDurationHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_trackPositionGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->trackPositionHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_playbackSpeedGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->playbackSpeedHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_seekingSpeedGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->seekingSpeedHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_currentTrackSegmentsObjectIdGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->currentTrackSegObjectIDHdl;

    return 1;
}

static int blt_gmcsc_currentTrackObjectIdGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->currentTrackObjectIDHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_nextObjectIdGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->nextTrackObjectIDHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_parentGroupObjectIdGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->parentGroupObjectIDHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_currentGroupObjectIdGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->currentGroupObjectIDHd;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_playingOrderGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->playingOrderHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_playingOrdersSupportedGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->playingOrdersSupportedHdl;

    return 1;
}

static int blt_gmcsc_mediaStateGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->mediaStateHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_mediaControlPointGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    (void)connHandle;
    charInfo->properties = CHAR_PROP_NOTIFY;
    charInfo->cccHandle  = 0;

    return 1;
}

static int blt_gmcsc_mediaControlPointOpcodesSupportedGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->mediaControlPointOpSuppHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_searchControlPointGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    (void)connHandle;
    charInfo->properties = CHAR_PROP_NOTIFY;
    charInfo->cccHandle  = 0;

    return 1;
}

static int blt_gmcsc_searchResultsObjectIdGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->searchResultsObjectIDHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_gmcsc_contentControlIdGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->contentControlIDHdl;

    return 1;
}

static const blc_gapc_reconnChar_t reGmcsChar[] = {

    {
     .ifun = blt_gmcsc_mediaPlayerNameGetInfo,
     .rfun = blt_gmcsc_mediaPlayerNameStartRead,
     },

    {
     .ifun = blt_gmcsc_mediaPlayerIconObjectIdGetInfo,
     .rfun = blt_gmcsc_mediaPlayerIconObjectIdStartRead,
     },

    {
     .ifun = blt_gmcsc_mediaPlayerIconUrlGetInfo,
     .rfun = blt_gmcsc_mediaPlayerIconUrlStartRead,
     },
    {
     .ifun = blt_gmcsc_trackChangedGetInfo,
     },
    {
     .ifun = blt_gmcsc_trackTitleGetInfo,
     .rfun = blt_gmcsc_trackTitleStartRead,
     },

    {
     .ifun = blt_gmcsc_trackDurationGetInfo,
     .rfun = blt_gmcsc_trackDurationStartRead,
     },

    {
     .ifun = blt_gmcsc_trackPositionGetInfo,
     .rfun = blt_gmcsc_trackPositionStartRead,
     },

    {
     .ifun = blt_gmcsc_playbackSpeedGetInfo,
     .rfun = blt_gmcsc_playbackSpeedStartRead,
     },

    {
     .ifun = blt_gmcsc_seekingSpeedGetInfo,
     .rfun = blt_gmcsc_seekingSpeedStartRead,
     },

    {
     .ifun = blt_gmcsc_currentTrackSegmentsObjectIdGetInfo,
     .rfun = blt_gmcsc_currentTrackSegmentsObjectIdStartRead,
     },

    {
     .ifun = blt_gmcsc_currentTrackObjectIdGetInfo,
     .rfun = blt_gmcsc_currentTrackObjectIdStartRead,
     },

    {
     .ifun = blt_gmcsc_nextObjectIdGetInfo,
     .rfun = blt_gmcsc_nextObjectIdStartRead,
     },

    {
     .ifun = blt_gmcsc_parentGroupObjectIdGetInfo,
     .rfun = blt_gmcsc_parentGroupObjectIdStartRead,
     },

    {
     .ifun = blt_gmcsc_currentGroupObjectIdGetInfo,
     .rfun = blt_gmcsc_currentGroupObjectIdStartRead,
     },

    {
     .ifun = blt_gmcsc_playingOrderGetInfo,
     .rfun = blt_gmcsc_playingOrderStartRead,
     },

    {
     .ifun = blt_gmcsc_playingOrdersSupportedGetInfo,
     .rfun = blt_gmcsc_playingOrdersSupportedStartRead,
     },

    {
     .ifun = blt_gmcsc_mediaStateGetInfo,
     .rfun = blt_gmcsc_mediaStateStartRead,
     },
    {
     .ifun = blt_gmcsc_mediaControlPointGetInfo,
     },
    {
     .ifun = blt_gmcsc_mediaControlPointOpcodesSupportedGetInfo,
     .rfun = blt_gmcsc_mediaControlPointOpcodesSupportedStartRead,
     },
    {
     .ifun = blt_gmcsc_searchControlPointGetInfo,
     },
    {
     .ifun = blt_gmcsc_searchResultsObjectIdGetInfo,
     .rfun = blt_gmcsc_searchResultsObjectIdStartRead,
     },

    {
     .ifun = blt_gmcsc_contentControlIdGetInfo,
     .rfun = blt_gmcsc_contentControlIdStartRead,
     },
};

static const blc_gapc_reconnList_t reconnMcp = {
    .resfun = blt_gmcsc_reconnService,
    .charTb = {
               .size           = ARRAY_SIZE(reGmcsChar),
               .characteristic = reGmcsChar,
               },
    .inclSize = 0,
};

/**********reconnect function ending********/

/*************************************************************************
 *  GATTC Read Characteristics
 *  Read Media Information 4.5.1 M
Read Media Player Icon Object Information 4.5.2 O
Read Track Title 4.5.3 O
Read Track Duration 4.5.4 O
Read Track Position 4.5.5 O
Set Absolute Track Position 4.5.6 O
Set Relative Track Position 4.5.7 O
Read Playback Speed 4.5.8 O
Set Playback Speed 4.5.9 O
Read Seeking Speed 4.5.10 O
Read Current Track Segments Object Information 4.5.11 O
Read Current Track Object Information 4.5.12 O
Set Current Track Object ID 4.5.13 O
Read Next Track Object Information 4.5.14 O
Set Next Track Object ID 4.5.15 O
Track Discovery Discover by Current Group Object ID 4.5.16 O
Set Current Group Object ID 4.5.17 O
Read Parent Group Object Information 4.5.18 O
Read Playing Order 4.5.19 O
Set Playing Order 4.5.20 O
Read Playing Order Supported 4.5.21 O
Read Media State 4.5.22 O
Play Current Track 4.5.23 O
Pause Current Track 4.5.24 O
Fast Forward Fast Rewind 4.5.25 O
Stop Current Track 4.5.26 O
Move to Previous Segment 4.5.27 O
Move to Next Segment 4.5.28 O
Move to First Segment 4.5.29 O
Move to Last Segment 4.5.30 O
Move to Segment Number 4.5.31 O
Move to Previous Track 4.5.32 O
Move to Next Track 4.5.33 O
Move to First Track 4.5.34 O
Move to Last Track 4.5.35 O
Move to Track Number 4.5.36 O
Move to Previous Group 4.5.37 O
Move to Next Group 4.5.38 O
Move to First Group 4.5.39 O
Move to Last Group 4.5.40 O
Move to Group Number 4.5.41 O
Read Media Control Point Opcodes Supported 4.5.42 O
Search 4.5.43 O
Read Content Control ID 4.5.44 O
 *************************************************************************/
#if (0)
static void blt_gmcsc_readAttrValCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    if (err == GATT_ERR_DATA_LENGTH_EXCEED_MEM_RESTRICTION) {
        BLT_TBS_LOG("RD_CB INFO: ERR: Can not save all read values due to memory restrictions");
    } else if (err) {
        BLT_TBS_LOG("RD_CB INFO: ERR: read handle:[0x%x] err:[0x%x]", pRdCfg->single.handle, err);
        return;
    }

    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);


    u16 attHandle      = pRdCfg->single.handle;
    u8 *pAttVal        = pRdCfg->single.wBuff;
    u16 attValLen      = pRdCfg->single.wBuffLen == NULL ? pRdCfg->single.maxLen : *pRdCfg->single.wBuffLen;
    u16 validAttValLen = min(pRdCfg->single.maxLen, attValLen);
    if (attHandle == client->xxxx) {
        BLT_TBS_LOG("RD_CB INFO: ATT_HDL[0x%x] xxxx[%.*s]", attHandle, validAttValLen, pAttVal);
    }
}

static int blt_gmcsc_readAttrVal(u16 connHandle, blt_gmcs_read_enum rdType)
{
    BLT_TBS_LOG("blt_gmcsc_readAttrVal:%d", rdType);
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    } else if (rdType >= GTBS_READ_MAX) {
        BLT_TBS_LOG("ERR: Invalid read type %d", rdType);
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);


    return blc_gapc_readAttributeValue(connHandle, &pGapReCfg);
}

int blc_gmcsc_readReadMediaInfo(u16 connHandle)
{
}
#endif


/***********************************************/
typedef struct
{
    blc_mcs_mediaCtrlPointOpcode_enum op;
    u32                               supp;
} blt_gmcs_opcodeSuppTb_t;

const blt_gmcs_opcodeSuppTb_t gmcsOpSupp[] = {
    {BLC_MCS_OPCODE_PLAY,             0x00000001},
    {BLC_MCS_OPCODE_PAUSE,            0x00000002},
    {BLC_MCS_OPCODE_FAST_REWIND,      0x00000004},
    {BLC_MCS_OPCODE_FAST_FORWARD,     0x00000008},
    {BLC_MCS_OPCODE_STOP,             0x00000010},
    {BLC_MCS_OPCODE_MOVE_RELATIVE,    0x00000020},
    {BLC_MCS_OPCODE_PREVIOUS_SEGMENT, 0x00000040},
    {BLC_MCS_OPCODE_NEXT_SEGMENT,     0x00000080},
    {BLC_MCS_OPCODE_FIRST_SEGMENT,    0x00000100},
    {BLC_MCS_OPCODE_LAST_SEGMENT,     0x00000200},
    {BLC_MCS_OPCODE_GOTO_SEGMENT,     0x00000400},
    {BLC_MCS_OPCODE_PREVIOUS_TRACK,   0x00000800},
    {BLC_MCS_OPCODE_NEXT_TRACK,       0x00001000},
    {BLC_MCS_OPCODE_FIRST_TRACK,      0x00002000},
    {BLC_MCS_OPCODE_LAST_TRACK,       0x00004000},
    {BLC_MCS_OPCODE_GOTO_TRACK,       0x00008000},
    {BLC_MCS_OPCODE_PREVIOUS_GROUP,   0x00010000},
    {BLC_MCS_OPCODE_NEXT_GROUP,       0x00020000},
    {BLC_MCS_OPCODE_FIRST_GROUP,      0x00040000},
    {BLC_MCS_OPCODE_LAST_GROUP,       0x00080000},
    {BLC_MCS_OPCODE_GOTO_GROUP,       0x00100000},
};

int blc_gmcsc_checkMediaCtrlOpSupp(blc_mcs_mediaCtrlPointOpcode_enum opcode, u32 suppOp)
{
    for (size_t i = 0; i < ARRAY_SIZE(gmcsOpSupp); i++) {
        if (opcode == gmcsOpSupp[i].op) {
            return suppOp & gmcsOpSupp[i].supp ? AUDIO_ESUCC : AUDIO_ERR_OPCODE_NOT_SUPP;
        }
    }

    return AUDIO_ERR_OPCODE_RFU;
}

int blc_gmcsc_writeMediaControl(u16 connHandle, blc_mcs_mediaCtrlPointOpcode_enum opcode, u8 *param, u16 paramLen)
{
    BLT_MCS_LOG("blc_gmcsc_writeMediaControl");

    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLT_MCS_LOG("ERR: ACL handle invalid");
        return AUDIO_EHANDLE;
    } else if (opcode > BLC_MCS_OPCODE_GOTO_GROUP) {
        BLT_MCS_LOG("ERR: Invalid write opcode %d", opcode);
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    gapc_write_cfg_t   pGapWrCfg;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    audio_error_enum err = blc_gmcsc_checkMediaCtrlOpSupp(opcode, client->mediaControlPointOpSupp);

    if (err) {
        return err;
    }

    if (!client->mediaControlPointHdl) {
        return AUDIO_ERR_NOT_MEDIA_CTRL_POINT_HANDLE;
    }

    u8 mediaCtrlData[1 + paramLen];
    mediaCtrlData[0] = opcode;
    memcpy(mediaCtrlData + 1, param, paramLen);
    pGapWrCfg.func       = NULL;
    pGapWrCfg.handle     = client->mediaControlPointHdl;
    pGapWrCfg.data       = mediaCtrlData;
    pGapWrCfg.length     = 1 + paramLen;
    pGapWrCfg.withoutRsp = true;
    pGapWrCfg.cbData     = NULL;

    u32 state = blc_gapc_writeAttributeValue(connHandle, &pGapWrCfg);
    BLT_MCS_LOG("send res is %d", state);

    return AUDIO_ESUCC;
}

#define blc_gmcsc_writeMediaControlWithoutParam(connHandle, opcode)     blc_gmcsc_writeMediaControl(connHandle, opcode, NULL, 0)
#define blc_gmcsc_writeMediaControlWithParam(connHandle, opcode, param) blc_gmcsc_writeMediaControl(connHandle, opcode, (u8 *)&param, sizeof(param))

int blc_gmcsc_writeStartPlayingCurrentTrack(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_PLAY);
}

int blc_gmcsc_writePauseCurrentTrack(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_PAUSE);
}

int blc_gmcsc_writeFastRewindCurrentTrack(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_FAST_REWIND);
}

int blc_gmcsc_writeFastForwardCurrentTrack(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_FAST_FORWARD);
}

int blc_gmcsc_writeStopActivity(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_STOP);
}

int blc_gmcsc_writeMoveRelative(u16 connHandle, int offset)
{
    return blc_gmcsc_writeMediaControlWithParam(connHandle, BLC_MCS_OPCODE_MOVE_RELATIVE, offset);
}

int blc_gmcsc_writePreviousSegment(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_PREVIOUS_SEGMENT);
}

int blc_gmcsc_writeNextSegment(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_NEXT_SEGMENT);
}

int blc_gmcsc_writeFirstSegment(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_FIRST_SEGMENT);
}

int blc_gmcsc_writeLastSegment(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_LAST_SEGMENT);
}

int blc_gmcsc_writeGotoSegment(u16 connHandle, int n)
{
    return blc_gmcsc_writeMediaControlWithParam(connHandle, BLC_MCS_OPCODE_GOTO_SEGMENT, n);
}

int blc_gmcsc_writePreviousTrack(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_PREVIOUS_TRACK);
}

int blc_gmcsc_writeNextTrack(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_NEXT_TRACK);
}

int blc_gmcsc_writeFirstTrack(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_FIRST_TRACK);
}

int blc_gmcsc_writeLastTrack(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_LAST_TRACK);
}

int blc_gmcsc_writeGotoTrack(u16 connHandle, int n)
{
    return blc_gmcsc_writeMediaControlWithParam(connHandle, BLC_MCS_OPCODE_GOTO_TRACK, n);
}

int blc_gmcsc_writePreviousGroup(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_PREVIOUS_GROUP);
}

int blc_gmcsc_writeNextGroup(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_NEXT_GROUP);
}

int blc_gmcsc_writeFirstGroup(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_FIRST_GROUP);
}

int blc_gmcsc_writeLastGroup(u16 connHandle)
{
    return blc_gmcsc_writeMediaControlWithoutParam(connHandle, BLC_MCS_OPCODE_LAST_GROUP);
}

int blc_gmcsc_writeGotoGroup(u16 connHandle, int n)
{
    return blc_gmcsc_writeMediaControlWithParam(connHandle, BLC_MCS_OPCODE_GOTO_GROUP, n);
}

int blc_gmcsc_writeSearchControl(u16 connHandle, blc_mcs_searchCtrlPointType_enum opcode, char *param)
{
    gapc_write_cfg_t   pGapWrCfg;
    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);

    if (opcode >= BLC_MCS_SEARCH_TYPE_RFU) {
        return AUDIO_ERR_OPCODE_NOT_SUPP;
    }

    if (!client->searchControlPointHdl) {
        return AUDIO_ERR_NOT_SEARCH_CTRL_POINT_HANDLE;
    }

    int paramLen = param == NULL ? 0 : strlen(param);

    //MCS_v1.0 3.20 The maximum size of this characteristic value is 64 octets.
    if (paramLen > 62) {
        return AUDIO_ERR_PARAM_SIZE_ERR;
    }

    u8 searchCtrlData[64];
    searchCtrlData[0] = paramLen + 1;
    searchCtrlData[1] = opcode;
    memcpy(searchCtrlData + 2, param, paramLen);
    if ((opcode == BLC_MCS_SEARCH_TYPE_ONLY_TRACKS) || (opcode == BLC_MCS_SEARCH_TYPE_ONLY_GROUPS)) {
        searchCtrlData[0] = 1;
    }
    pGapWrCfg.func       = NULL;
    pGapWrCfg.handle     = client->searchControlPointHdl;
    pGapWrCfg.data       = searchCtrlData;
    pGapWrCfg.length     = 1 + searchCtrlData[0];
    pGapWrCfg.withoutRsp = true;
    pGapWrCfg.cbData     = NULL;

    u32 state = blc_gapc_writeAttributeValue(connHandle, &pGapWrCfg);
    BLT_MCS_LOG("send res is %d", state);

    return AUDIO_ESUCC;
}

int blc_gmcsc_writeSearchCtrlTrackName(u16 connHandle, char *trackName)
{
    return blc_gmcsc_writeSearchControl(connHandle, BLC_MCS_SEARCH_TYPE_TRACK_NAME, trackName);
}

int blc_gmcsc_writeSearchCtrlArtistName(u16 connHandle, char *artistName)
{
    return blc_gmcsc_writeSearchControl(connHandle, BLC_MCS_SEARCH_TYPE_ARTIST_NAME, artistName);
}

int blc_gmcsc_writeSearchCtrlAlbumName(u16 connHandle, char *albumName)
{
    return blc_gmcsc_writeSearchControl(connHandle, BLC_MCS_SEARCH_TYPE_ALBUM_NAME, albumName);
}

int blc_gmcsc_writeSearchCtrlGroupName(u16 connHandle, char *groupName)
{
    return blc_gmcsc_writeSearchControl(connHandle, BLC_MCS_SEARCH_TYPE_GROUP_NAME, groupName);
}

int blc_gmcsc_writeSearchCtrlEarliestYear(u16 connHandle, char *earliestYear)
{
    return blc_gmcsc_writeSearchControl(connHandle, BLC_MCS_SEARCH_TYPE_EARLIEST_YEAR, earliestYear);
}

int blc_gmcsc_writeSearchCtrlLatestYear(u16 connHandle, char *latestYear)
{
    return blc_gmcsc_writeSearchControl(connHandle, BLC_MCS_SEARCH_TYPE_LATEST_YEAR, latestYear);
}

int blc_gmcsc_writeSearchCtrlGenre(u16 connHandle, char *genre)
{
    return blc_gmcsc_writeSearchControl(connHandle, BLC_MCS_SEARCH_TYPE_GENRE, genre);
}

int blc_gmcsc_writeSearchCtrlOnlyTracks(u16 connHandle)
{
    return blc_gmcsc_writeSearchControl(connHandle, BLC_MCS_SEARCH_TYPE_ONLY_TRACKS, NULL);
}

int blc_gmcsc_writeSearchCtrlOnlyGroups(u16 connHandle)
{
    return blc_gmcsc_writeSearchControl(connHandle, BLC_MCS_SEARCH_TYPE_ONLY_GROUPS, NULL);
}

int blc_gmcsc_getMediaState(u16 connHandle, blc_mcs_mediaState_enum *mediaState)
{
    if (mediaState == NULL) {
        return AUDIO_EPARAM;
    }
    if (blt_ll_isAclHandleOutOfRange(connHandle)) {
        return AUDIO_EPARAM;
    }

    blc_gmcs_client_t *client = blt_gmcsc_getClientInst(connHandle);
    *mediaState               = client->mediaState;

    return AUDIO_ESUCC;
}
