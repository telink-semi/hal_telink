/********************************************************************************************************
 * @file    hci_event.c
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
#include "stack/ble/controller/ble_controller.h"

int hci_disconnectionComplete_evt(u8 status, u16 connHandle, u8 reason)
{
    u8                              result[4];
    hci_disconnectionCompleteEvt_t *pEvt = (hci_disconnectionCompleteEvt_t *)result;

    pEvt->status     = status;
    pEvt->connHandle = connHandle;
    pEvt->reason     = reason;

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] Conn disconnect Evt", &pEvt->connHandle, 3);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_DISCONNECTION_COMPLETE, result, 4);
}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
    int
    hci_cmdComplete_evt(u8 numHciCmds, u8 opCode_ocf, u8 opCode_ogf, u8 paraLen, u8 *para, u8 *result)
{
    (void)numHciCmds;
    hci_cmdCompleteEvt_evtParam_t *pEvt = (hci_cmdCompleteEvt_evtParam_t *)result;

    pEvt->numHciCmds = 1;
    pEvt->opCode_OCF = opCode_ocf;
    pEvt->opCode_OGF = opCode_ogf;
    smemcpy(pEvt->returnParas, para, paraLen);

    return (paraLen + 3);
}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
    int
    hci_cmdStatus_evt(u8 numHciCmds, u8 opCode_ocf, u8 opCode_ogf, u8 status, u8 *result)
{
    hci_cmdStatusEvt_evtParam_t *pEvt = (hci_cmdStatusEvt_evtParam_t *)result;

    pEvt->status     = status;
    pEvt->numHciCmds = numHciCmds;
    pEvt->opCode_OCF = opCode_ocf;
    pEvt->opCode_OGF = opCode_ogf;
    return 4;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
    int
    hci_numberOfCompletePacket_evt(u16 connHandle, u8 numOfCmpConn)
{
    int                   connhand_cnt = 1;
    u8                    evt_buffer[1 + 1 * sizeof(numCmpPktParamRet_t)];
    hci_numOfCmpPktEvt_t *pEvt = (hci_numOfCmpPktEvt_t *)evt_buffer;

    pEvt->numHandles                = connhand_cnt;
    pEvt->retParams[0].connHandle   = connHandle;
    pEvt->retParams[0].numOfCmpPkts = numOfCmpConn;

    //It must be ensured that Num_Of_Complete_Evt is sent successfully
    int result = blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_NUM_OF_COMPLETE_PACKETS, evt_buffer, 1 + 4 * connhand_cnt);
    if (result != 0) {
        BLMS_ERR_DEBUG(DBG_HCI_FIFO, 0xCC110623);
    }

    return result;
}

int hci_le_connectionComplete_evt(u8 status, u16 connHandle, u8 role, u8 peerAddrType, u8 *peerAddr, u16 connInterval, u16 periphr_Latency, u16 supervisionTimeout, u8 masterClkAccuracy)
{
    u8 result[sizeof(hci_le_connectionCompleteEvt_t)];

    hci_le_connectionCompleteEvt_t *pEvt = (hci_le_connectionCompleteEvt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_CONNECTION_COMPLETE;
    pEvt->status       = status;
    pEvt->connHandle   = connHandle;
    pEvt->role         = role;
    pEvt->peerAddrType = peerAddrType;
    smemcpy(pEvt->peerAddr, peerAddr, BLE_ADDR_LEN);
    pEvt->connInterval       = connInterval;
    pEvt->peripheralLatency  = periphr_Latency;
    pEvt->supervisionTimeout = supervisionTimeout;
    pEvt->masterClkAccuracy  = masterClkAccuracy;

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] Conn complete Evt", &pEvt->connHandle, 12);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_connectionCompleteEvt_t));
}

int hci_le_connectionUpdateComplete_evt(u8 status, u16 connHandle, u16 connInterval, u16 connLatency, u16 supervisionTimeout)
{
    u8                                    result[sizeof(hci_le_connectionUpdateCompleteEvt_t)];
    hci_le_connectionUpdateCompleteEvt_t *pEvt = (hci_le_connectionUpdateCompleteEvt_t *)result;

    pEvt->subEventCode       = HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE;
    pEvt->status             = status;
    pEvt->connHandle         = connHandle;
    pEvt->connInterval       = connInterval;
    pEvt->connLatency        = connLatency;
    pEvt->supervisionTimeout = supervisionTimeout;


    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, 10);
}

int hci_le_readRemoteFeaturesComplete_evt(u8 status, u16 connHandle, u8 *feature)
{
    u8                                      result[sizeof(hci_le_readRemoteFeaturesCompleteEvt_t)];
    hci_le_readRemoteFeaturesCompleteEvt_t *pEvt = (hci_le_readRemoteFeaturesCompleteEvt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE;
    pEvt->status       = status;
    pEvt->connHandle   = connHandle;
    smemcpy(pEvt->feature, feature, LL_FEATURE_SIZE);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_readRemoteFeaturesCompleteEvt_t));
}

int hci_le_phyUpdateComplete_evt(u16 connhandle, u8 status, u8 new_phy)
{
    u8                             result[sizeof(hci_le_phyUpdateCompleteEvt_t)];
    hci_le_phyUpdateCompleteEvt_t *pEvt = (hci_le_phyUpdateCompleteEvt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_PHY_UPDATE_COMPLETE;
    pEvt->status       = status;
    pEvt->connHandle   = connhandle;
    pEvt->tx_phy       = new_phy;
    pEvt->rx_phy       = new_phy;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, 6);
}

int hci_le_longTermKeyRequest_evt(u16 connHandle, u8 *random, u16 ediv, u8 *result)
{
    hci_le_longTermKeyRequestEvt_t *pEvt     = (hci_le_longTermKeyRequestEvt_t *)result; // = (hci_le_longTermKeyRequestEvt_t *)ev_buf_allocate(MEDIUM_BUFFER);
    int                             paramLen = 13;

    pEvt->subEventCode = HCI_SUB_EVT_LE_LONG_TERM_KEY_REQUESTED;
    pEvt->connHandle   = connHandle;
    smemcpy(pEvt->random, random, 8);
    pEvt->ediv = ediv;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, paramLen);
}

int hci_le_readLocalP256KeyComplete_evt(u8 *localP256Key, u8 status)
{
    u8                                    result[66];
    hci_le_readLocalP256KeyCompleteEvt_t *pEvt     = (hci_le_readLocalP256KeyCompleteEvt_t *)result;
    int                                   paramLen = 66;

    pEvt->status       = status ? BLE_SUCCESS : HCI_ERR_UNSPECIFIED_ERROR;
    pEvt->subEventCode = HCI_SUB_EVT_LE_READ_LOCAL_P256_KEY_COMPLETE;
    smemcpy(pEvt->localP256Key, localP256Key, 64);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, paramLen);
}

int hci_le_generateDHKeyComplete_evt(u8 *DHkey, u8 status)
{
    u8                                 result[34];
    hci_le_generateDHKeyCompleteEvt_t *pEvt     = (hci_le_generateDHKeyCompleteEvt_t *)result;
    int                                paramLen = 34;

    pEvt->subEventCode = HCI_SUB_EVT_LE_GENERATE_DHKEY_COMPLETE;
    pEvt->status       = status ? BLE_SUCCESS : HCI_ERR_INVALID_HCI_CMD_PARAMS;
    smemcpy(pEvt->DHKey, DHkey, 32);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, paramLen);
}

int hci_le_enhancedConnectionComplete_evt(u8 status, u16 connHandle, u8 role, u8 peerAddrType, u8 *peerAddr, u8 *localRpa, u8 *peerRpa, u16 connInterval, u16 connLatency, u16 supervisionTimeout, u8 masterClkAccuracy)
{
    u8                                result[sizeof(hci_le_enhancedConnCompleteEvt_t)];
    hci_le_enhancedConnCompleteEvt_t *pEvt = (hci_le_enhancedConnCompleteEvt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_ENHANCED_CONNECTION_COMPLETE;
    pEvt->status       = status;
    pEvt->connHandle   = connHandle;
    pEvt->role         = role;
    pEvt->PeerAddrType = peerAddrType;
    smemcpy(pEvt->PeerAddr, peerAddr, BLE_ADDR_LEN);
    smemcpy(pEvt->localRslvPrivAddr, localRpa, BLE_ADDR_LEN);
    smemcpy(pEvt->Peer_RslvPrivAddr, peerRpa, BLE_ADDR_LEN);
    pEvt->connInterval      = connInterval;
    pEvt->connLatency       = connLatency;
    pEvt->superTimeout      = supervisionTimeout;
    pEvt->masterClkAccuracy = masterClkAccuracy;

#if (DBG_PRVC_CONN_EN)
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] LE Enhanced Conn complete Evt", &pEvt->status, sizeof(hci_le_enhancedConnCompleteEvt_t) - 1); //form peer address
#else

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] LE Enhanced Conn complete Evt", &pEvt->connHandle, 10);
#endif

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_enhancedConnCompleteEvt_t));
}

int hci_le_enhancedConnectionComplete_evt_v2(u8 status, u16 connHandle, u8 role, u8 peerAddrType, u8 *peerAddr, u8 *localRpa, u8 *peerRpa, u16 connInterval, u16 connLatency, u16 supervisionTimeout, u8 masterClkAccuracy, u8 advertisingHandle, u16 syncHandle)
{
    u8                                   result[sizeof(hci_le_enhancedConnCompleteEvt_t_V2)];
    hci_le_enhancedConnCompleteEvt_t_V2 *pEvt = (hci_le_enhancedConnCompleteEvt_t_V2 *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_ENHANCED_CONNECTION_COMPLETE_V2;
    pEvt->status       = status;
    pEvt->connHandle   = connHandle;
    pEvt->role         = role;
    pEvt->PeerAddrType = peerAddrType;
    smemcpy(pEvt->PeerAddr, peerAddr, BLE_ADDR_LEN);
    smemcpy(pEvt->localRslvPrivAddr, localRpa, BLE_ADDR_LEN);
    smemcpy(pEvt->Peer_RslvPrivAddr, peerRpa, BLE_ADDR_LEN);
    pEvt->connInterval       = connInterval;
    pEvt->connLatency        = connLatency;
    pEvt->superTimeout       = supervisionTimeout;
    pEvt->masterClkAccuracy  = masterClkAccuracy;
    pEvt->Advertising_Handle = advertisingHandle;
    pEvt->Sync_Handle        = syncHandle;
#if (DBG_PRVC_CONN_EN)
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] LE Enhanced Conn complete Evt", &pEvt->status, sizeof(hci_le_enhancedConnCompleteEvt_t) - 1); //form peer address
#else

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] LE Enhanced Conn complete Evt", &pEvt->connHandle, 10);
#endif

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_enhancedConnCompleteEvt_t_V2));
}

/*
The HCI_LE_Directed_Advertising_Report event indicates that directed
advertisements have been received where the advertiser is using a resolvable
private address for the TargetA field of the advertising PDU which the
Controller is unable to resolve and the Scanning_Filter_Policy is equal to 0x02
or 0x03, see Section 7.8.10. Direct_Address_Type and Direct_Address specify
the address the directed advertisements are being directed to. Address_Type
and Address specify the address of the advertiser sending the directed
advertisements.
*/
int hci_le_directAdvertisingReport_evt(u8 addr_type, u8 *addr, u8 *direct_addr, s8 rssi)
{
    u8                        result[sizeof(hci_le_directAdvRptEvt_t)];
    hci_le_directAdvRptEvt_t *pEvt = (hci_le_directAdvRptEvt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_DIRECT_ADVERTISE_REPORT;
    pEvt->num_reports  = 1;
    pEvt->event_type   = ADV_REPORT_EVENT_TYPE_DIRECT_IND;
    pEvt->addr_type    = addr_type;
    smemcpy(pEvt->address, addr, BLE_ADDR_LEN);
    pEvt->direct_addr_type = BLE_ADDR_RANDOM;
    smemcpy(pEvt->direct_address, direct_addr, BLE_ADDR_LEN);
    pEvt->rssi = rssi;

#if 0 //(DBG_PRVC_LEGSCAN_EN)
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] LE Direct ADV Report Evt", &pEvt->addr_type, 14);
#else
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] LE Direct ADV Report Evt", pEvt, sizeof(hci_le_directAdvRptEvt_t));
#endif

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_directAdvRptEvt_t));
}

int hci_le_encryptChange_evt(u16 connhandle, u8 encrypt_en)
{
    u8                         result[4] = {0};
    hci_le_encryptEnableEvt_t *pEvt      = (hci_le_encryptEnableEvt_t *)result;

    pEvt->status            = BLE_SUCCESS;
    pEvt->connHandle        = connhandle;
    pEvt->encryption_enable = encrypt_en;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_ENCRYPTION_CHANGE, result, 4);
}

//for HDT
int hci_encryptChangeV3_evt(u16 connhandle, u8 encrypt_en, u8 encrypt_keysize, u8 mic_length, u8 pfs_dbg_key)
{
    u8                         result[8] = {0};
    hci_encryptEnableV3Evt_t *pEvt      = (hci_encryptEnableV3Evt_t *)result;

    pEvt->encryptEnable.status            = BLE_SUCCESS;
    pEvt->encryptEnable.connHandle        = connhandle;
    pEvt->encryptEnable.encryption_enable = encrypt_en;
    pEvt->encryptionKeySize               = encrypt_keysize;
    pEvt->micLength                       = mic_length;
    pEvt->pfs_dbg_key                     = pfs_dbg_key;
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_ENCRYPTION_CHANGE_V3, result, 7);
}

int hci_le_encryptKeyRefresh_evt(u16 connhandle)
{
    u8                             result[3] = {0};
    hci_le_encryptKeyRefreshEvt_t *pEvt      = (hci_le_encryptKeyRefreshEvt_t *)result;

    pEvt->status     = BLE_SUCCESS;
    pEvt->connHandle = connhandle;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_ENCRYPTION_KEY_REFRESH, result, 3);
}

//for HDT
int hci_encryptKeyRefreshV2_evt(u16 connhandle, u8 mic_length ,u8 pfs_dbg_key)
{
    u8                             result[6] = {0};
    hci_encryptKeyRefreshV2Evt_t *pEvt      = (hci_encryptKeyRefreshV2Evt_t *)result;

    pEvt->keyFresh.status     = BLE_SUCCESS;
    pEvt->keyFresh.connHandle = connhandle;
    pEvt->micLength           = mic_length;
    pEvt->pfs_dbg_key         = pfs_dbg_key;
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_ENCRYPTION_KEY_REFRESH_COMPLETE_V2, result, 5);
}


int hci_le_channel_selection_algorithm_evt(u16 connhandle, u8 channel_selection_alg)
{
    u8                              result[4] = {0};
    hci_le_chnSelectAlgorithmEvt_t *pEvt      = (hci_le_chnSelectAlgorithmEvt_t *)result;
    pEvt->subEventCode                        = HCI_SUB_EVT_LE_CHANNEL_SELECTION_ALGORITHM;
    pEvt->connHandle                          = connhandle;
    pEvt->channel_selection_algorithm         = channel_selection_alg;

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] LE Chn_Sel_Alg Evt", &pEvt->channel_selection_algorithm, 1);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, 4);
}

int hci_le_data_len_update_evt(u16 connhandle, u16 effTxOctets, u16 effRxOctets, u16 maxtxtime, u16 maxrxtime)
{
    u8                            result[12];
    hci_le_dataLengthChangeEvt_t *pEvt = (hci_le_dataLengthChangeEvt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_DATA_LENGTH_CHANGE;
    pEvt->connHandle   = connhandle;
    pEvt->maxTxOct     = effTxOctets;
    pEvt->maxTxtime    = maxtxtime;
    pEvt->maxRxOct     = effRxOctets;
    pEvt->maxRxtime    = maxrxtime;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, 11);
}


#if 0
int hci_le_advertising_set_terminated_evt(u8 status, u8 advertising_handle, u16 connection_handle, u8 num_complete_extended_adv_events)
{
    u8 result[6] = {0};
    hci_le_advSetTerminatedEvt_t*    pEvt = (hci_le_advSetTerminatedEvt_t*) result;
    pEvt->subEventCode = HCI_SUB_EVT_LE_ADVERTISING_SET_TERMINATED;
    pEvt->status = status;
    pEvt->advHandle = advertising_handle;
    pEvt->connHandle = connection_handle;
    pEvt->num_compExtAdvEvt = num_complete_extended_adv_events;
    return blc_hci_send_event (HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, 6);
}
#endif


int hci_remoteNateReqComplete_evt(u8 *bd_addr)
{
    u8 result[8] = {0}; // not standard
    result[0]    = BLE_SUCCESS;
    smemcpy(result + 1, bd_addr, 6);
    result[7] = 0;
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_REMOTE_NAME_REQ_COMPLETE, result, 8);
}

#if 0 //(LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)

int hci_le_extendedAdvertisingReport_evt (xxx)
{

}


#endif


int hci_le_periodicAdvSyncEstablished_evt(u8 status, u16 syncHandle, u8 advSID, u8 advAddrType, u8 advAddress[6], u8 advPHY, u16 perdAdvItvl, u8 advClkAccuracy)
//int hci_le_periodicAdvSyncEstablished_evt (u8 status, u16 syncHandle, extadv_id_t* pId, u8 advPHY, u16 perdAdvItvl, u8 advClkAccuracy)
{
    hci_le_periodicAdvSyncEstablishedEvt_t Evt;
    Evt.subEventCode = HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED;
    Evt.status       = status;
    Evt.syncHandle   = syncHandle;
    Evt.advSID       = advSID;
    Evt.advAddrType  = advAddrType;
    smemcpy(Evt.advAddr, advAddress, 6);
    Evt.advPHY         = advPHY;
    Evt.perdAdvItvl    = perdAdvItvl;
    Evt.advClkAccuracy = advClkAccuracy;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8 *)&Evt, sizeof(hci_le_periodicAdvSyncEstablishedEvt_t));
}

int hci_le_periodicAdvSyncEstablished_evt_v2(u8 status, u16 syncHandle, u8 advSID, u8 advAddrType, u8 advAddress[6], u8 advPHY, u16 perdAdvItvl, u8 advClkAccuracy, u8 num_subevent, u8 subevent_intvl, u8 rsp_slot_delay, u8 rsp_slot_spacing)
{
    hci_le_periodicAdvSyncEstablishedEvtV2_t Evt;
    Evt.subEventCode = HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED_V2;
    Evt.status       = status;
    Evt.syncHandle   = syncHandle;
    Evt.advSID       = advSID;
    Evt.advAddrType  = advAddrType;
    smemcpy(Evt.advAddr, advAddress, 6);
    Evt.advPHY           = advPHY;
    Evt.perdAdvItvl      = perdAdvItvl;
    Evt.advClkAccuracy   = advClkAccuracy;
    Evt.num_subevent     = num_subevent;
    Evt.subevent_intvl   = subevent_intvl;
    Evt.rsp_slot_delay   = rsp_slot_delay;
    Evt.rsp_slot_spacing = rsp_slot_spacing;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8 *)&Evt, sizeof(hci_le_periodicAdvSyncEstablishedEvtV2_t));
}

int hci_le_periodicAdvReport_evt(u8 subEventCode, u16 syncHandle, u8 txPower, u8 RSSI, u8 cteType, u8 dataStatus, u8 dataLength, u8 *data)
{
    (void)subEventCode;
    u8                             result[sizeof(hci_le_periodicAdvReportEvt_t) + 246] = {0};
    hci_le_periodicAdvReportEvt_t *pEvt                                                = (hci_le_periodicAdvReportEvt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT;
    pEvt->syncHandle   = syncHandle;
    pEvt->txPower      = txPower;
    pEvt->RSSI         = RSSI;
    pEvt->cteType      = cteType;
    pEvt->dataStatus   = dataStatus;
    pEvt->dataLength   = dataLength;
    smemcpy(pEvt->data, data, dataLength);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8 *)&pEvt, sizeof(hci_le_periodicAdvSyncEstablishedEvt_t) - 1 + dataLength);
}

int hci_le_periodicAdvSyncLost_evt(u16 syncHandle)
{
    hci_le_periodicAdvSyncLostEvt_t Evt;
    Evt.subEventCode = HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_LOST;
    Evt.syncHandle   = syncHandle;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8 *)&Evt, sizeof(hci_le_periodicAdvSyncLostEvt_t));
}

int hci_le_periodicAdvSyncTransferRcvd_evt(u8 status, u16 connHandle, u16 serviceData, u16 syncHandle, u8 advSID, u8 advAddrType, u8 advAddr[6], u8 advPHY, u16 perdAdvItvl, u8 advClkAccuracy)
{
    u8                                       result[sizeof(hci_le_periodicAdvSyncTransferRcvdEvt_t)] = {0};
    hci_le_periodicAdvSyncTransferRcvdEvt_t *pEvt                                                    = (hci_le_periodicAdvSyncTransferRcvdEvt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECEIVED;
    pEvt->status       = status;
    pEvt->connHandle   = connHandle;
    pEvt->serviceData  = serviceData;
    pEvt->syncHandle   = syncHandle;
    pEvt->advSID       = advSID;
    pEvt->advAddrType  = advAddrType;
    smemcpy(pEvt->advAddr, advAddr, 6);
    pEvt->advPHY         = advPHY;
    pEvt->perdAdvItvl    = perdAdvItvl;
    pEvt->advClkAccuracy = advClkAccuracy;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_periodicAdvSyncTransferRcvdEvt_t));
}

int hci_le_periodicAdvSyncTransferRcvd_evt_V2(u8 status, u16 connHandle, u16 serviceData, u16 syncHandle, u8 advSID, u8 advAddrType, u8 advAddr[6], u8 advPHY, u16 perdAdvItvl, u8 advClkAccuracy, pawr_acad_t *pPawrInfo)
{
    u8                                          result[sizeof(hci_le_periodicAdvSyncTransferRcvdEvt_V2_t)] = {0};
    hci_le_periodicAdvSyncTransferRcvdEvt_V2_t *pEvt                                                       = (hci_le_periodicAdvSyncTransferRcvdEvt_V2_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECEIVED_V2; //tomorrow go on...
    pEvt->status       = status;
    pEvt->connHandle   = connHandle;
    pEvt->serviceData  = serviceData;
    pEvt->syncHandle   = syncHandle;
    pEvt->advSID       = advSID;
    pEvt->advAddrType  = advAddrType;
    smemcpy(pEvt->advAddr, advAddr, 6);
    pEvt->advPHY           = advPHY;
    pEvt->perdAdvItvl      = perdAdvItvl;
    pEvt->advClkAccuracy   = advClkAccuracy;
    pEvt->num_subevt       = pPawrInfo->num_subevent;
    pEvt->subevent_intvl   = pPawrInfo->subevent_intvl;
    pEvt->rsp_slot_delay   = pPawrInfo->rsp_slot_delay;
    pEvt->rsp_slot_spacing = pPawrInfo->rsp_slot_spacing;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_periodicAdvSyncTransferRcvdEvt_V2_t));
}

int hci_le_cisEstablished_evt(u8 status, u16 cisHandle, u8 cigSyncDly[3], u8 cisSyncDly[3], u8 transLaty_m2s[3], u8 transLaty_s2m[3],
                              u8 phy_m2s, u8 phy_s2m, u8 nse, u8 bn_m2s, u8 bn_s2m, u8 ft_m2s, u8 ft_s2m, u16 maxPDU_m2s,
                              u16 maxPDU_s2m, u16 isoIntvl)
{
    hci_le_cisEstablishedEvt_t Evt;
    Evt.subEventCode = HCI_SUB_EVT_LE_CIS_ESTABLISHED;
    Evt.status       = status;
    Evt.cisHandle    = cisHandle;

    smemcpy(Evt.cigSyncDly, cigSyncDly, 3);
    smemcpy(Evt.cisSyncDly, cisSyncDly, 3);
    smemcpy(Evt.transLaty_m2s, transLaty_m2s, 3);
    smemcpy(Evt.transLaty_s2m, transLaty_s2m, 3);

    Evt.phy_m2s    = phy_m2s;
    Evt.phy_s2m    = phy_s2m;
    Evt.nse        = nse;
    Evt.bn_m2s     = bn_m2s;
    Evt.bn_s2m     = bn_s2m;
    Evt.ft_m2s     = ft_m2s;
    Evt.ft_s2m     = ft_s2m;
    Evt.maxPDU_m2s = maxPDU_m2s;
    Evt.maxPDU_s2m = maxPDU_s2m;
    Evt.isoIntvl   = isoIntvl;

    my_dump_str_u32s(0, "cis infor", Evt.nse, Evt.bn_m2s, Evt.bn_s2m, Evt.ft_m2s);

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] CIS Establish Evt", &Evt.status, sizeof(hci_le_cisEstablishedEvt_t) - 1);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8 *)&Evt, sizeof(hci_le_cisEstablishedEvt_t));
}

//for HDT
int hci_le_cisEstablishedV4_evt(u8 status, u16 cisHandle, u8 cigSyncDly[3], u8 cisSyncDly[3], u8 transLaty_m2s[3], u8 transLaty_s2m[3],
                                u8 phy_m2s, u8 phy_s2m, u8 nse, u8 bn_m2s, u8 bn_s2m, u8 ft_m2s, u8 ft_s2m, u16 maxPDU_m2s,
                                u16 maxPDU_s2m, u16 isoIntvl, u8 sub_interval[3], u16 maxSdu_c2p, u16 maxSdu_p2c, u8 sduIntvl_c2p[3],
                                u8 sduIntvl_p2c[3], u8 framing, u8 rates_c2p, u8 rates_p2c, u8 encrypt_en, u8 mic_length)
{
    hci_le_cisEstablishedV4Evt_t Evt;
    Evt.subEventCode = HCI_SUB_EVT_LE_CIS_ESTABLISHED_V4;
    Evt.status       = status;
    Evt.cisHandle    = cisHandle;

    smemcpy(Evt.cigSyncDly, cigSyncDly, 3);
    smemcpy(Evt.cisSyncDly, cisSyncDly, 3);
    smemcpy(Evt.transLaty_m2s, transLaty_m2s, 3);
    smemcpy(Evt.transLaty_s2m, transLaty_s2m, 3);
    smemcpy(Evt.sub_interval, sub_interval, 3);
    smemcpy(Evt.sduIntvl_c2p, sduIntvl_c2p, 3);
    smemcpy(Evt.sduIntvl_p2c, sduIntvl_p2c, 3);

    Evt.phy_m2s    = phy_m2s;
    Evt.phy_s2m    = phy_s2m;
    Evt.nse        = nse;
    Evt.bn_m2s     = bn_m2s;
    Evt.bn_s2m     = bn_s2m;
    Evt.ft_m2s     = ft_m2s;
    Evt.ft_s2m     = ft_s2m;
    Evt.maxPDU_m2s = maxPDU_m2s;
    Evt.maxPDU_s2m = maxPDU_s2m;
    Evt.isoIntvl   = isoIntvl;
    Evt.maxSdu_c2p = maxSdu_c2p;
    Evt.maxSdu_p2c = maxSdu_p2c;
    Evt.framing    = framing;
    Evt.rates_c2p  = rates_c2p;
    Evt.rates_p2c  = rates_p2c;
    Evt.encrypt_en = encrypt_en;
    Evt.mic_length = mic_length;

    my_dump_str_u32s(0, "cis infor", Evt.nse, Evt.bn_m2s, Evt.bn_s2m, Evt.ft_m2s);

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] CIS Establish Evt V4", &Evt.status, sizeof(hci_le_cisEstablishedV4Evt_t) - 1);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8 *)&Evt, sizeof(hci_le_cisEstablishedV4Evt_t));
}

int hci_le_cisReq_evt(u16 aclHandle, u16 cisHandle, u8 cigId, u8 cisId)
{
    hci_le_cisReqEvt_t Evt;
    Evt.subEventCode = HCI_SUB_EVT_LE_CIS_REQUEST;
    Evt.aclHandle    = aclHandle;
    Evt.cisHandle    = cisHandle;
    Evt.cigId        = cigId;
    Evt.cisId        = cisId;

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] CIS Req Evt", &Evt.aclHandle, sizeof(hci_le_cisReqEvt_t) - 1);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8 *)&Evt, sizeof(hci_le_cisReqEvt_t));
}

int hci_le_createBigComplete_evt(u8 status, u8 bigHandle, u8 bigSyncDly[3], u8 transLatyBig[3], u8 phy, u8 nse, u8 bn, u8 pto, u8 irc, u16 maxPDU, u16 isoIntvl, u8 numBis, u16 *bisHandles)
{
    //TODO: must check param: numBis <= LL_BIS_IN_PER_BIG_BCST_NUM_MAX

    u8                             result[19 + 2 * LL_BIS_IN_PER_BIG_BCST_NUM_MAX] = {0};
    hci_le_createBigCompleteEvt_t *pEvt                                            = (hci_le_createBigCompleteEvt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_CREATE_BIG_COMPLETE;
    pEvt->status       = status;
    pEvt->bigHandle    = bigHandle;
    smemcpy(pEvt->bigSyncDly, bigSyncDly, 3);
    smemcpy(pEvt->transLatyBig, transLatyBig, 3);
    pEvt->phy      = phy;
    pEvt->nse      = nse;
    pEvt->bn       = bn;
    pEvt->pto      = pto;
    pEvt->irc      = irc;
    pEvt->maxPDU   = maxPDU;
    pEvt->isoIntvl = isoIntvl;
    pEvt->numBis   = numBis;
    smemcpy((u8 *)pEvt->bisHandles, (u8 *)bisHandles, 2 * numBis);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, 19 + 2 * numBis);
}

//for HDT
int hci_le_createBigCompleteV2_evt(u8 status, u8 bigHandle, u8 bigSyncDly[3], u8 transLatyBig[3], u8 phy, u8 nse,u8 bn, u8 pto, u8 irc,
                                   u16 maxPDU, u16 isoIntvl, u16 rates, u8 encrypt_en, u8 mic_length, u8 numBis, u16 *bisHandles)
{
    //TODO: must check param: numBis <= LL_BIS_IN_PER_BIG_BCST_NUM_MAX

    u8                             result[23 + 2 * LL_BIS_IN_PER_BIG_BCST_NUM_MAX] = {0};
    hci_le_createBigCompleteV2Evt_t *pEvt                                            = (hci_le_createBigCompleteV2Evt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_CREATE_BIG_COMPLETE_V2;
    pEvt->status       = status;
    pEvt->bigHandle    = bigHandle;
    smemcpy(pEvt->bigSyncDly, bigSyncDly, 3);
    smemcpy(pEvt->transLatyBig, transLatyBig, 3);

    pEvt->phy        = phy;
    pEvt->nse        = nse;
    pEvt->bn         = bn;
    pEvt->pto        = pto;
    pEvt->irc        = irc;
    pEvt->maxPDU     = maxPDU;
    pEvt->isoIntvl   = isoIntvl;
    pEvt->rates      = rates;
    pEvt->encrypt_en = encrypt_en;
    pEvt->mic_length = mic_length;
    pEvt->numBis     = numBis;
    smemcpy((u8 *)pEvt->bisHandles, (u8 *)bisHandles, 2 * numBis);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, 23 + 2 * numBis);
}

int hci_le_terminateBigComplete_evt(u8 bigHandle, u8 reason)
{
    u8                                result[sizeof(hci_le_terminateBigCompleteEvt_t)];
    hci_le_terminateBigCompleteEvt_t *pEvt = (hci_le_terminateBigCompleteEvt_t *)result;
    pEvt->subEventCode                     = HCI_SUB_EVT_LE_TERMINATE_BIG_COMPLETE;
    pEvt->bigHandle                        = bigHandle;
    pEvt->reason                           = reason;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_terminateBigCompleteEvt_t));
}

int hci_le_bigSyncEstablished_evt(u8 status, u8 bigHandle, u8 transLatyBig[3], u8 nse, u8 bn, u8 pto, u8 irc, u16 maxPDU, u16 isoIntvl, u8 numBis, u16 *bisHandles)
{
    //TODO: must check param: numBis <= BIS_IN_PER_BIG_SYNC_NUM_MAX

    u8                              result[15 + 2 * LL_BIS_IN_PER_BIG_SYNC_NUM_MAX] = {0};
    hci_le_bigSyncEstablishedEvt_t *pEvt                                            = (hci_le_bigSyncEstablishedEvt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_BIG_SYNC_ESTABLISHED;
    pEvt->status       = status;
    pEvt->bigHandle    = bigHandle;
    smemcpy(pEvt->transLatyBig, transLatyBig, 3);
    pEvt->nse      = nse;
    pEvt->bn       = bn;
    pEvt->pto      = pto;
    pEvt->irc      = irc;
    pEvt->maxPDU   = maxPDU;
    pEvt->isoIntvl = isoIntvl;
    pEvt->numBis   = numBis;
    smemcpy((u8 *)pEvt->bisHandles, (u8 *)bisHandles, 2 * numBis);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, 15 + 2 * numBis);
}

int hci_le_bigSyncLost_evt(u8 bigHandle, u8 reason)
{
    hci_le_bigSyncLostEvt_t Evt;
    Evt.subEventCode = HCI_SUB_EVT_LE_BIG_SYNC_LOST;
    Evt.bigHandle    = bigHandle;
    Evt.reason       = reason;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8 *)&Evt, sizeof(hci_le_bigSyncLostEvt_t));
}

int hci_le_BigInfoAdvReport_evt(u16 syncHandle, u8 numBis, u8 nse, u16 IsoItvl, u8 bn, u8 pto, u8 irc, u16 maxPdu, u8 sduItvl[3], u16 maxSdu, u8 phy, u8 framing, u8 enc)
{
    hci_le_bigInfoAdvReportEvt_t Evt;

    Evt.subEventCode = HCI_SUB_EVT_LE_BIGINFO_ADVERTISING_REPORT;
    Evt.syncHandle   = syncHandle;
    Evt.numBis       = numBis;
    Evt.nse          = nse;
    Evt.IsoItvl      = IsoItvl; //in units of 1.25 ms.
    Evt.bn           = bn;
    Evt.pto          = pto;
    Evt.irc          = irc;
    Evt.maxPdu       = maxPdu;
    smemcpy(Evt.sduItvl, sduItvl, 3);
    Evt.maxSdu  = maxSdu;
    Evt.phy     = phy;
    Evt.framing = framing;
    Evt.enc     = enc;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8 *)&Evt, sizeof(hci_le_bigInfoAdvReportEvt_t));
}

int hci_le_pathLossThreshold_evt(u16 connHandle, u8 currPathLoss, u8 zoneEntered)
{
    u8                             result[sizeof(hci_le_pathLossThresholdEvt_t)] = {0};
    hci_le_pathLossThresholdEvt_t *pEvt                                          = (hci_le_pathLossThresholdEvt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_PATH_LOSS_THRESHOLD;
    pEvt->connHandle   = connHandle;
    pEvt->currPathLoss = currPathLoss;
    pEvt->zoneEntered  = zoneEntered;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_pathLossThresholdEvt_t));
}

int hci_le_transmitPwrRpting_evt(u8 status, u16 connHandle, u8 reason, u8 phy, s8 txPwrLvl, u8 txPwrLvlFlg, s8 delta)
{
    u8                             result[sizeof(hci_le_transmitPwrRptingEvt_t)] = {0};
    hci_le_transmitPwrRptingEvt_t *pEvt                                          = (hci_le_transmitPwrRptingEvt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_TRANSMIT_POWER_REPORTING;
    pEvt->status       = status;
    pEvt->connHandle   = connHandle;
    pEvt->reason       = reason;
    pEvt->phy          = phy;
    pEvt->txPwrLvl     = txPwrLvl;
    pEvt->txPwrLvlFlg  = txPwrLvlFlg;
    pEvt->delta        = delta;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_transmitPwrRptingEvt_t));
}

int hci_le_authPayloadTimeoutExpired_evt(u16 connHandle)
{
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_AUTH_PAYLOAD_TIMEOUT_EXPIRED, (u8 *)&connHandle, 2);
}

int hci_le_periodcAdvSubeventDataRequest_evt(u8 adv_handle, u8 subevt_start, u8 subevt_data_cnt)
{
    u8                                    result[sizeof(hci_le_periodicAdvSubevtDataReqEvt_t)] = {0};
    hci_le_periodicAdvSubevtDataReqEvt_t *pEvt                                                 = (hci_le_periodicAdvSubevtDataReqEvt_t *)result;

    pEvt->subEventCode    = HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SUBEVENT_DATA_REQUEST;
    pEvt->advHandle       = adv_handle;
    pEvt->subevtStart     = subevt_start;
    pEvt->subevtDataCount = subevt_data_cnt;

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_periodicAdvSubevtDataReqEvt_t));
}

int hci_le_readRemoteSupCapComplete_evt(u8 status, u16 connHandle, u8 *data)
{
    u8                                    result[sizeof(hci_le_readRemoteSupCapCompleteEvt_t)];
    hci_le_readRemoteSupCapCompleteEvt_t *pEvt = (hci_le_readRemoteSupCapCompleteEvt_t *)result;

    pEvt->Subevent_Code     = HCI_SUB_EVT_LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES_COMPLETE;
    pEvt->Status            = status;
    pEvt->Connection_Handle = connHandle;
    smemcpy((u8 *)&pEvt->Num_Config_Supported, data, sizeof(hci_le_readRemoteSupCapCompleteEvt_t) - 4);
    CS_HCI_LOG("[EVT][R_REMOTE_CAP]:%s", hex_to_str(result, sizeof(hci_le_readRemoteSupCapCompleteEvt_t)));
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_readRemoteSupCapCompleteEvt_t));
}

int hci_le_readRemoteFAETableComplete_evt(u8 status, u16 connHandle, u8 *table)
{
    u8                                      result[sizeof(hci_le_readRemoteFAETableCompleteEvt_t)];
    hci_le_readRemoteFAETableCompleteEvt_t *pEvt = (hci_le_readRemoteFAETableCompleteEvt_t *)result;

    pEvt->Subevent_Code     = HCI_SUB_EVT_LE_CS_READ_REMOTE_FAE_TABLE_COMPLETE;
    pEvt->Status            = status;
    pEvt->Connection_Handle = connHandle;
    smemcpy((u8 *)&pEvt->Remote_FAE_Table, table, 72);
    CS_HCI_LOG("[EVT][R_REM_FAE_TAB]:%s", hex_to_str(result, sizeof(hci_le_readRemoteFAETableCompleteEvt_t)));
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_readRemoteFAETableCompleteEvt_t));
}

int hci_le_csSecurityEnableComplete_evt(u8 status, u16 connHandle)
{
    u8                                    result[sizeof(hci_le_csSecurityEnableCompleteEvt_t)];
    hci_le_csSecurityEnableCompleteEvt_t *pEvt = (hci_le_csSecurityEnableCompleteEvt_t *)result;

    pEvt->Subevent_Code     = HCI_SUB_EVT_LE_CS_SECURITY_ENABLE_COMPLETE;
    pEvt->Status            = status;
    pEvt->Connection_Handle = connHandle;
    CS_HCI_LOG("[EVT][CS_SEC]:%s", hex_to_str(result, sizeof(hci_le_csSecurityEnableCompleteEvt_t)));
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_csSecurityEnableCompleteEvt_t));
}

int hci_le_csConfigComplete_evt(u8 status, u16 connHandle, u8 *data)
{
    u8                            result[sizeof(hci_le_csConfigCompleteEvt_t)];
    hci_le_csConfigCompleteEvt_t *pEvt   = (hci_le_csConfigCompleteEvt_t *)result;
    cs_config_t                  *pCsCfg = (cs_config_t *)data;
    pEvt->Subevent_Code                  = HCI_SUB_EVT_LE_CS_CONFIG_COMPLETE;
    pEvt->Status                         = status;
    pEvt->Connection_Handle              = connHandle;
    pEvt->Config_ID                      = pCsCfg->Config_ID;
    pEvt->Action                         = pCsCfg->state;
    pEvt->Main_Mode                      = pCsCfg->Main_Mode;
    pEvt->Sub_Mode                       = pCsCfg->Sub_Mode;
    pEvt->Main_Mode_Min_Steps            = pCsCfg->Main_Mode_Min_Steps;
    pEvt->Main_Mode_Max_Steps            = pCsCfg->Main_Mode_Max_Steps;
    pEvt->Main_Mode_Repetition           = pCsCfg->Main_Mode_Repetition;
    pEvt->Mode_0_Steps                   = pCsCfg->Mode_0_Steps;
    pEvt->Role                           = pCsCfg->Role;
    pEvt->RTT_Type                       = pCsCfg->RTT_Type;
    pEvt->CS_SYNC_PHY                    = pCsCfg->CS_SYNC_PHY;
    pEvt->Channel_Map_Repetition         = pCsCfg->Channel_Map_Repetition;
    pEvt->ChSel                          = pCsCfg->ChSel;
    pEvt->Ch3c_Shape                     = pCsCfg->Ch3c_Shape;
    pEvt->Ch3c_Jump                      = pCsCfg->Ch3c_Jump;
    pEvt->Companion_Signal_Enable        = pCsCfg->Companion_Signal_Enable;
    pEvt->T_IP1_Time                     = pCsCfg->T_IP1_Us;
    pEvt->T_IP2_Time                     = pCsCfg->T_IP2_Us;
    pEvt->T_FCS_Time                     = pCsCfg->T_FCS_Us;
    pEvt->T_PM_Time                      = pCsCfg->T_PM_Us;
    smemcpy((u8 *)pEvt->Channel_Map, (u8 *)pCsCfg->Channel_Map, 10);
    CS_HCI_LOG("[EVT][CS_CFG]:%s", hex_to_str(result, sizeof(hci_le_csConfigCompleteEvt_t)));
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_csConfigCompleteEvt_t));
}

int hci_le_csProcedureEnableComplete_evt(u8 status, u16 connHandle, u8 *data)
{
    u8                                     result[sizeof(hci_le_csProcedureEnableCompleteEvt_t)];
    hci_le_csProcedureEnableCompleteEvt_t *pEvt   = (hci_le_csProcedureEnableCompleteEvt_t *)result;
    cs_config_t                           *pCsCfg = (cs_config_t *)data;
    pEvt->Subevent_Code                           = HCI_SUB_EVT_LE_CS_PROCEDURE_ENABLE_COMPLETE;
    pEvt->Status                                  = status;
    pEvt->Connection_Handle                       = connHandle;
    pEvt->Config_ID                               = pCsCfg->Config_ID;
    pEvt->state                                   = pCsCfg->cs_procedure_en;
    //  pEvt->Tone_Antenna_Config_Selection = pCsCfg->Tone_Antenna_Config_Selection;
    pEvt->Tone_Antenna_Config_Selection = pCsCfg->aci;
    pEvt->Selected_TX_Power             = pCsCfg->Selected_TX_Power;
    pEvt->Subevent_Len[0]               = (u8)(pCsCfg->Subevent_Len & 0xff);
    pEvt->Subevent_Len[1]               = (u8)((pCsCfg->Subevent_Len >> 8) & 0xff);
    pEvt->Subevent_Len[2]               = (u8)((pCsCfg->Subevent_Len >> 16) & 0xff);
    pEvt->Subevents_Per_Event           = pCsCfg->Subevents_Per_Event;
    pEvt->Subevent_Interval             = pCsCfg->subEvtIntvl_625us;
    pEvt->Event_Interval                = pCsCfg->Event_Interval;
    pEvt->Procedure_Interval            = pCsCfg->Procedure_Interval;
    pEvt->Procedure_Count               = pCsCfg->procMaxCountInstant;
    pEvt->Max_Procedure_Len             = pCsCfg->Max_Procedure_Len;
    if (pEvt->state) {
        CS_HCI_LOG("[EVT][CS_START]CS Start Success");
        pCsCfg->max_subEvtCnt = blt_cs_calcMaxProcLenSubevtCount(pCsCfg);
        CS_HCI_LOG("max subevent cnt based on max proceudre len is: %d", pCsCfg->max_subEvtCnt);
        pCsCfg->subEvtCnt = 0;
    } else {
        CS_HCI_LOG("[EVT][CS_TERMINATE]CS Terminate Success");
    }
    CS_HCI_LOG("[EVT][CS_PROC_EN]:%s", hex_to_str(result, sizeof(hci_le_csProcedureEnableCompleteEvt_t)));
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_csProcedureEnableCompleteEvt_t));
}

_attribute_ram_code_ int hci_le_csSubeventResult_evt(u16 connhandle, u8 config_id, u8 *data, u32 data_length)
{
    (void)config_id;
    (void)connhandle;
#if (0)
    u32                           result_len = 0;
    u8                            result[255]; //length < MAX HCIevent size
    hci_le_csSubeventResultEvt_t *pEvt = (hci_le_csSubeventResultEvt_t *)result;
    pEvt->Subevent_Code                = HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT;
    pEvt->Connection_Handle            = connhandle;
    pEvt->Config_ID                    = config_id;

    smemcpy((u8 *)pEvt->Step_Mode, data, data_length);
    result_len = sizeof(hci_le_csSubeventResultEvt_t) + data_length - 4;

    CS_HCI_LOG("[EVT]cs subevent result");
    CS_HCI_LOG("sub_evt_rst:%s", hex_to_str(result, result_len));
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, result_len);
#else
    //  CS_HCI_LOG("[EVT]cs subevent result");
    //  CS_HCI_LOG("sub_evt_rst len: %u", data_length);
    // if(data_length > 85){
    //  if(data_length > 170){
    //      CS_HCI_LOG("ser1:%s",hex_to_str(data, 85));
    //      CS_HCI_LOG("ser2:%s",hex_to_str(data+85, 85));
    //      CS_HCI_LOG("ser3:%s",hex_to_str(data+170, data_length-170));
    //  } else {
    //      CS_HCI_LOG("ser1:%s",hex_to_str(data, 85));
    //      CS_HCI_LOG("ser2:%s",hex_to_str(data+85, data_length-170));
    //  }
    // } else {
    //  CS_HCI_LOG("ser1:%s",hex_to_str(data, data_length));
    // }
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, data, data_length);
#endif
}

_attribute_ram_code_ int hci_le_csSubeventResultContinue_evt(u16 connhandle, u8 config_id, u8 *data, u32 data_length)
{
    (void)config_id;
    (void)connhandle;
#if (0)
    u32                                   result_len = 0;
    u8                                    result[255]; //length < MAX HCIevent size
    hci_le_csSubeventResultContinueEvt_t *pEvt = (hci_le_csSubeventResultContinueEvt_t *)result;

    pEvt->Subevent_Code     = HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT_CONTINUE;
    pEvt->Connection_Handle = connhandle;
    pEvt->Config_ID         = config_id;

    smemcpy((u8 *)pEvt->Step_Mode, data, data_length);
    result_len = sizeof(hci_le_csSubeventResultContinueEvt_t) + data_length - 4;

    CS_HCI_LOG("[EVT]cs subevent result continue");
    CS_HCI_LOG("sub_evt_rst_cont:%s", hex_to_str(result, result_len));
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, result_len);
#else
    //  CS_HCI_LOG("[EVT]cs subevent result continue");
    //  CS_HCI_LOG("sub_evt_rst_cont len: %u", data_length);

    // if(data_length > 85){
    //  if(data_length > 170){
    //      CS_HCI_LOG("serc1:%s",hex_to_str(data, 85));
    //      CS_HCI_LOG("serc2:%s",hex_to_str(data+85, 85));
    //      CS_HCI_LOG("serc3:%s",hex_to_str(data+170, data_length-170));
    //  } else {
    //      CS_HCI_LOG("serc1:%s",hex_to_str(data, 85));
    //      CS_HCI_LOG("serc2:%s",hex_to_str(data+85, data_length-170));
    //  }
    // } else {
    //  CS_HCI_LOG("serc1:%s",hex_to_str(data, data_length));
    // }
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, data, data_length);
#endif
}

int hci_le_csTestEndComplete_evt(u8 status)
{
    u8                             result[sizeof(hci_le_csTestEndCompleteEvt_t)];
    hci_le_csTestEndCompleteEvt_t *pEvt = (hci_le_csTestEndCompleteEvt_t *)result;
    pEvt->Subevent_Code                 = HCI_SUB_EVT_LE_CS_TEST_END_COMPLETE;
    pEvt->Status                        = status;
    CS_HCI_LOG("[EVT]cs test end");
    CS_HCI_LOG("tst_end:%s", hex_to_str(result, sizeof(hci_le_csTestEndCompleteEvt_t)));
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_csTestEndCompleteEvt_t));
}

int hci_le_frameSpaceUpdateComplete_evt(u8 status, u16 connHandle, u8 initiator, u16 frameSpace, u8 phys, u16 spacingTypes)
{
    u8                             result[sizeof(hci_le_frameSpaceUpdateCompleteEvt_t)];
    hci_le_frameSpaceUpdateCompleteEvt_t *pEvt = (hci_le_frameSpaceUpdateCompleteEvt_t *)result;
    pEvt->subEventCode                  = HCI_SUB_EVT_LE_FRAME_SPACE_UPDATE_COMPLETE;
    pEvt->status                        = status;
    pEvt->connHandle                    = connHandle;
    pEvt->initiator                     = initiator;
    pEvt->frame_space                   = frameSpace;
    pEvt->phy_mask                      = phys;
    pEvt->spacing_types                 = spacingTypes;
    CS_HCI_LOG("[HCI][EVT]frame space update complete");
    CS_HCI_LOG("frame_space_upd:%s", hex_to_str(result, sizeof(hci_le_frameSpaceUpdateCompleteEvt_t)));
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_frameSpaceUpdateCompleteEvt_t));
}

int hci_le_monitoredAdvertisersReport_evt(u8 addr_type, u8* addr, u8 condition)
{
    u8                             result[sizeof(hci_le_monitioredAdvertisersReportEvt_t)];
    hci_le_monitioredAdvertisersReportEvt_t *pEvt = (hci_le_monitioredAdvertisersReportEvt_t *)result;
    pEvt->subEventCode                 = HCI_SUB_EVT_LE_MONITORED_ADVERTISERS_REPORT;
    pEvt->addr_type                        = addr_type;
    smemcpy(pEvt->address, addr, BLE_ADDR_LEN);
    pEvt->condition                        = condition;
    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_le_monitioredAdvertisersReportEvt_t));
}

/* Telink private event "LE Connection Establish Event" */
int hci_tlk_connectionEstablish_evt(u8 status, u16 connHandle, u8 role, u8 peerAddrType, u8 *peerAddr, u16 connInterval, u16 periphr_Latency, u16 supervisionTimeout, u8 masterClkAccuracy)
{
    u8 result[sizeof(hci_tlk_connectionEstablishEvt_t)];

    hci_tlk_connectionEstablishEvt_t *pEvt = (hci_tlk_connectionEstablishEvt_t *)result;

    pEvt->subEventCode = HCI_SUB_EVT_LE_CONNECTION_ESTABLISH;
    pEvt->status       = status;
    pEvt->connHandle   = connHandle;
    pEvt->role         = role;
    pEvt->peerAddrType = peerAddrType;
    smemcpy(pEvt->peerAddr, peerAddr, BLE_ADDR_LEN);
    pEvt->connInterval       = connInterval;
    pEvt->peripheralLatency  = periphr_Latency;
    pEvt->supervisionTimeout = supervisionTimeout;
    pEvt->masterClkAccuracy  = masterClkAccuracy;

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] Conn Establish Evt", &pEvt->connHandle, 12);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_tlk_connectionEstablishEvt_t));
}

/* Telink private event "LE Create Connection Fail Event" */
int hci_tlk_createConnectionFail_evt(u8 fail_reason, u8 create_conn_cnt)
{
    u8 result[sizeof(hci_tlk_createConnFailEvt_t)];

    hci_tlk_createConnFailEvt_t *pEvt = (hci_tlk_createConnFailEvt_t *)result;

    pEvt->subEventCode    = HCI_SUB_EVT_LE_CONNECTION_FAIL;
    pEvt->fail_reason     = fail_reason;
    pEvt->create_conn_cnt = create_conn_cnt;

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] Create Conn Fail Evt", &pEvt->fail_reason, 1);

    return blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, sizeof(hci_tlk_createConnFailEvt_t));
}
