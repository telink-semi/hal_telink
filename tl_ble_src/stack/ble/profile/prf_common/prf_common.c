#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "stack/ble/host/gatt/tlk_list_stack.h"

static void blt_prf_storeNvData(u16 connHandle);
static int blt_prf_getPriorityValueCb(void* node);
static void blt_prf_aclChangeStateCb(u16 connHandle, GAP_STATE_ENUM state, void* node);
static void blt_prf_svcQHandler(u16 connHandle, prf_proc_type_enum procType);
static void blt_prf_sendSecurityDoneEvt(u16 connHandle);
static void blt_prf_sendAclConnEvt(hci_le_enhancedConnCompleteEvt_t *pData);
static void blt_prf_sendAclDisconnEvt(hci_disconnectionCompleteEvt_t *pData);
static void blt_prf_sendCltSdpOverEvt(u16 connHandle);
static int blt_prf_hciEventCb(u32 h, u8 *p, int n);
static void blt_prf_loadNvData(u16 connHandle);

_attribute_ble_data_retention_ blt_prf_control_t gProfileCtrl;

_attribute_ble_data_retention_
static SPLIST_DEF(profileCommonClient, blt_prf_getPriorityValueCb);

_attribute_ble_data_retention_
static SPLIST_DEF(profileCommonServer, blt_prf_getPriorityValueCb);

_attribute_ble_data_retention_
static struct gap_stateChangeNode prfModuleCallBack = {
    .cb = blt_prf_aclChangeStateCb
};

_attribute_ble_data_retention_
static struct gap_hciEventNode prfHciEventCallBack = {
    .cb = blt_prf_hciEventCb,
};

static int blt_prf_getPriorityValueCb(void* node)
{
    return ((blc_prf_proc_t*)node)->id;
}

static void blt_prf_aclChangeStateCb(u16 connHandle, GAP_STATE_ENUM state, void* node)
{
    (void)node;
#if (LL_ASYNC_LEA_EN)
    if(IS_ASYNC_LEA_LINK(connHandle))
    {
        return;
    }
#endif
    switch(state)
    {
    case GAP_STATE_ACL_CONNECTED:
        blt_prf_svcQHandler(connHandle, PRF_PROC_CONN);
        break;
    case GAP_STATE_ACL_DISCONNECTED:
        blt_prf_svcQHandler(connHandle, PRF_PROC_DISCONN);
        int aclIdx = blc_prf_getAclConnectIndex(connHandle);
        gProfileCtrl.writeCb[aclIdx] = NULL;
        gProfileCtrl.readCb[aclIdx] = NULL;
        break;
    case GAP_STATE_SECURITY_DONE:
        /* app event callback */
        blt_prf_sendSecurityDoneEvt(connHandle);
        blt_prf_svcQHandler(connHandle, PRF_PROC_DISCOVERY);
        if(prf_store_used)
        {
            blt_prf_loadNvData(connHandle);
        }
        break;
    default:
        break;
    }
}

static int blt_prf_hciEventCb(u32 h, u8 *p, int n)
{
    (void)n;
    if (h & HCI_FLAG_EVENT_BT_STD)      //Controller HCI event
    {
        u8 evtCode = h & 0xff;
        u8 subEvt_code = p[0];
        //------------ disconnect -------------------------------------
        if(evtCode == HCI_EVT_DISCONNECTION_COMPLETE)
        { //connection terminate
            hci_disconnectionCompleteEvt_t  *pDisConn = (hci_disconnectionCompleteEvt_t *)p;
            u16 connHandle = pDisConn->connHandle;
            /* ACL central OR peripheral disconnection */
            if (connHandle&BLT_ACL_CONN_HANDLE)
            {
            #if (LL_ASYNC_LEA_EN)
                if(IS_ASYNC_LEA_LINK(pDisConn->connHandle)){
                    return 0;
                }
            #endif

            #if (KMA_DONGLE_MASK)
                if(blt_audio_cap_ctrl.kmaMark)
                {
                    u8 cur_aclCentral_idx = blc_ll_getAclCentralIndex_arrangedTaskTiming_diffMode(pDisConn->connHandle);
                    if(cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx1&&cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx2\
                     &&cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx3&&cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx4)
                    {
                        return 0;
                    }
                }
            #endif
                blc_gatts_notifyDisconnect(connHandle);
                /* app event callback */
                blt_prf_sendAclDisconnEvt(pDisConn);
            }
        }
        else if(evtCode == HCI_EVT_LE_META)  //LE Event
        {
            //------HCI LE event: LE enhanced connection event -------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_ENHANCED_CONNECTION_COMPLETE)  // connection complete
            {
                hci_le_enhancedConnCompleteEvt_t *pConnEvt = (hci_le_enhancedConnCompleteEvt_t *)p;

            #if (LL_ASYNC_LEA_EN)
                if(IS_ASYNC_LEA_LINK(pConnEvt->connHandle)){
                    return 0;
                }
            #endif
            #if (KMA_DONGLE_MASK)
                if(blt_audio_cap_ctrl.kmaMark)
                {
                    u8 cur_aclCentral_idx = blc_ll_getAclCentralIndex_arrangedTaskTiming_diffMode(pConnEvt->connHandle);
                    if(cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx1&&cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx2\
                     &&cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx3&&cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx4)
                    {
                        return 0;
                    }
                }
            #endif
                /* app event callback */
                blt_prf_sendAclConnEvt(pConnEvt);
            }
        }
    }
    else//host event
    {
        u8 event = h & 0xFF;
        switch(event)
        {
            case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
            {
                gap_smp_securityProcessDoneEvt_t *pData = (gap_smp_securityProcessDoneEvt_t *)p;
                #if (KMA_DONGLE_MASK)
                if(blt_audio_cap_ctrl.kmaMark)
                {
                    u8 cur_aclCentral_idx = blc_ll_getAclCentralIndex_arrangedTaskTiming_diffMode(pData->connHandle);
                    if(cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx1&&cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx2\
                     &&cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx3&&cur_aclCentral_idx != blt_audio_cap_ctrl.aclIdx4)
                    {
                        return 0;
                    }
                }
                #else
                (void)pData; //remove warning
                #endif

            }
            break;

            default:
            break;
        }
    }
    return 0;
}

////////////////////////Profile initial and register function//////////////////////////
void blc_prf_initialModule(prf_evt_cb_t evtCb)
{
    blt_host_init();        //TODO: by qihang.mou
//  /* Special protection code for use */
//  if(pm_check_info){
//      blc_hci_le_setEventMask_cmd(0xFFFFFFFF);
//      blc_hci_le_setEventMask_2_cmd(0xFFFFFFFF);

        blt_gap_regAclConnState(&prfModuleCallBack);
        blt_gap_regHciEventCb(&prfHciEventCallBack);
        gProfileCtrl.evtCb = evtCb;
//  }
}

static void blt_prf_registerServiceNode(blc_prf_proc_t *pNode, const void* param)
{
    struct single_priority_list* list = pNode->id < PRF_SERVER_OFFSET ? &profileCommonClient : &profileCommonServer;

    SPLIST_INSERT_NODE(list, pNode);

    if (pNode->init) {
        pNode->init(PRF_PROC_INIT, param);
    }
}

static void blt_prf_unregisterServiceNode(blc_prf_proc_t *pNode, const void* param)
{
    struct single_priority_list* list = pNode->id < PRF_SERVER_OFFSET ? &profileCommonClient : &profileCommonServer;

    SPLIST_DELETE_NODE(list, pNode);

    if (pNode->init) {
        pNode->init(PRF_PROC_DEINIT, param);
    }
}

void blc_prf_registerServiceModule(prf_bound_acl_role_enum usedAclRole, blc_prf_proc_t *pSvc, const void *param)
{
    if (!(usedAclRole & PRF_GAP_ACL_UNSPECIF) || pSvc == NULL)
    {
        return;
    }
//  /* Special protection code for use */
//  if(pm_check_info){
        pSvc->usedAclRole |= usedAclRole;
        blt_prf_registerServiceNode(pSvc, param);
//  }
}

void blc_prf_unregisterServiceModule(blc_prf_proc_t *pSvc, const void *param)
{
    if (pSvc == NULL)
    {
        return;
    }

    blt_prf_unregisterServiceNode(pSvc, param);
}

int blt_prf_sendEvent(u16 connHandle, int evtID, void *pData, u16 dataLen)
{
    if (gProfileCtrl.evtCb == NULL) {
        return -1;
    } else {
        return gProfileCtrl.evtCb(connHandle, evtID, pData, dataLen);
    }
}
////////////////////////Profile main loop function//////////////////////////
void blc_prf_main_loop(void)
{
    u16 connHandle;
    for (int conn_idx=0; conn_idx<LL_MAX_ACL_CONN_NUM; conn_idx++) {
        st_ll_conn_t *pAclConn = (st_ll_conn_t*)&blms[conn_idx];

        connHandle = pAclConn->acl_conHandle;
        if(pAclConn->connState) {
            blt_prf_svcQHandler(connHandle, PRF_PROC_LOOP);
        }
    }

    if(prf_store_used)
    {
        blt_prf_procBondingInfoIndexAlarm();
    }

    //TODO: currently used in LE Audio only
    blc_gapc_discoveryOrReconnectService_loop();
    blc_gatts_notifyLoop();

    soft_timer_process(MAINLOOP_ENTRY);
}


///////////////////////Profile Read/Write Attribute Value//////////////////

int blc_prf_readAttributeValue(u16 connHandle, gapc_read_cfg_t *pGapReCfg, prf_read_cb_t readCb)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if(aclIdx < 0){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    ble_sts_t status = blc_gapc_readAttributeValue(connHandle, pGapReCfg);
    if(status == BLE_SUCCESS)
    {
        gProfileCtrl.readCb[aclIdx] = readCb;
    }

    return status;
}

void blc_prf_readAttributeValueCallback(u16 connHandle, att_err_t err)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if(aclIdx < 0){
        return;
    }

    if(gProfileCtrl.readCb[aclIdx])
        gProfileCtrl.readCb[aclIdx](connHandle, err);
}

void blc_prf_readAttributeValueDefaultCallback(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    (void)pRdCfg;
    blc_prf_readAttributeValueCallback(connHandle, err);
}

int blc_prf_writeAttributeValue(u16 connHandle, gapc_write_cfg_t *pGapWrCfg, prf_write_cb_t writeCb)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if(aclIdx < 0){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    ble_sts_t status = blc_gapc_writeAttributeValue(connHandle, pGapWrCfg);

    if(status == BLE_SUCCESS)
    {
        gProfileCtrl.writeCb[aclIdx] = writeCb;
    }

    return status;
}

void blc_prf_writeAttributeValueCallback(u16 connHandle, att_err_t err)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if(aclIdx < 0){
        return;
    }

    if(gProfileCtrl.writeCb[aclIdx])
        gProfileCtrl.writeCb[aclIdx](connHandle, err);
}

void blc_prf_writeAttributeValueDefaultCallback(u16 connHandle, u8 err, void* data)
{
    (void)data;
    blc_prf_writeAttributeValueCallback(connHandle, err);
}

//////////////////////Profile Common Tools Function////////////////////////
int blc_prf_getAclConnectIndex(u16 connHandle)
{
    if (blt_ll_isAclHandleOutOfRange(connHandle) != BLE_SUCCESS){
        return -1;
    }

    /* DO NOT check connection state */
    u8 conn_idx = connHandle & CONN_IDX_MASK;

    if (conn_idx >= LL_MAX_ACL_CEN_NUM && (conn_idx - LL_MAX_ACL_CEN_NUM) < blmsParam.max_slave_num) {
        return blmsParam.max_master_num + (conn_idx - LL_MAX_ACL_CEN_NUM);
    } else if(conn_idx < LL_MAX_ACL_CEN_NUM && conn_idx < blmsParam.max_master_num) {
        return conn_idx;
    }

//  #if (DBG_PRF_AUD_LOG)
//      while(1){
//          wd_clear();
//          tlkapi_debug_handler();
//      }
//  #else
//      while(1);
//  #endif

    /* unlikely */
    return -1;
}

/* 1: acl central role; 0: acl peripheral role; others: err */
int blt_prf_getAclRole(u16 connHandle)
{
    if(blt_ll_isAclHandleOutOfRange(connHandle)) {
        return -1;
    }


    u8 central_role = connHandle & BLM_CONN_HANDLE;
    /* refer to: acl_connection_role_t */
    return central_role ? ACL_ROLE_CENTRAL : ACL_ROLE_PERIPHERAL;
}

/////////////////////////////Profile traverse list///////////////////////
static void blt_prf_svcHandler(u16 connHandle, prf_proc_type_enum procType, blc_prf_proc_t* elem)
{
    st_ll_conn_t* pAclConn = blt_ll_getAclConnPtr(connHandle);
    /* pAclConn->aclRole refer to 'acl_connection_role_t' */
    prf_bound_acl_role_enum currAudAclRole = BIT(pAclConn->aclRole);

    while (elem != NULL) {
        /* It is necessary to verify whether the declared SVC node's LL_ROLE is
         * consistent with the current ACL connection role. */
        if (elem->usedAclRole & currAudAclRole) {
            switch (procType){
                /* process when le_connected */
                case PRF_PROC_CONN:
                    if (elem->connect) {
                        elem->connect(connHandle, PRF_ACL_STATE_CONNECT);
                    }
                break;
                /* process when le_disconnected */
                case PRF_PROC_DISCONN:
                    //BLT_AUD_LOG("DIS: elem_id:0x%x, role:0x%x", elem->id, elem->usedAclRole);
                    if (elem->connect) {
                        elem->connect(connHandle, PRF_ACL_STATE_DISCONN);
                    }
                break;
                /* process audio loop */
                case PRF_PROC_LOOP:
                    if (elem->loop) {
                        elem->loop(connHandle);     //blt_*_loop
                    }
                break;
                default:
                    return;
            }
        }

        elem = elem->pNext;
    }
}

static void blt_prf_svcQHandler(u16 connHandle, prf_proc_type_enum procType)
{
#if (LL_ASYNC_LEA_EN)
    if(IS_ASYNC_LEA_LINK(connHandle)){
        return;
    }
#endif

    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if (aclIdx < 0) {
        return;
    }

    st_ll_conn_t* pAclConn = blt_ll_getAclConnPtr(connHandle);

    if(procType == PRF_PROC_CONN)
    {
//      BLT_AUD_LOG("audio connected, addr is %s", addr_to_str(pAclConn->conn_peerPktA));

    }
    else if(procType == PRF_PROC_DISCONN)
    {
        /* Bonded ATT handles information and CCC subscription information to store in FLASH. TODO:XXX */

//      BLT_AUD_LOG("audio disconnected, addr is %s", addr_to_str(pAclConn->conn_peerPktA));
        blc_gattc_cleanAllSubscribeCCCNode(connHandle);

        gProfileCtrl.sdpCtrl[aclIdx].clientRdySdp = 0;
        gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId = NULL;
    }
    else if(procType == PRF_PROC_DISCOVERY)
    {
        if(SLIST_FIRST(&profileCommonClient.list)){
            gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId = (blc_prf_proc_t *)SLIST_FIRST(&profileCommonClient.list);
//          BLT_AUD_LOG("audio discovery[aclIdx:%d][sdpId:%d]", aclIdx, gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId->id);
        }
        return ;
    }
    else if(procType == PRF_PROC_LOOP)
    {
        /* pAclConn->aclRole refer to 'acl_connection_role_t' */
        prf_bound_acl_role_enum currAudAclRole = BIT(pAclConn->aclRole);
        blc_prf_proc_t * sNode = gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId;
        if(sNode)
        {
            if((sNode->usedAclRole & currAudAclRole) && sNode->discov)
            {
                sNode->discov(connHandle);
            }
            else
            {
                blc_prf_setDiscoveryStatusFinish(connHandle);
            }
        }
    }

    blt_prf_svcHandler(connHandle, procType, (blc_prf_proc_t *)SLIST_FIRST(&profileCommonClient.list));
    blt_prf_svcHandler(connHandle, procType, (blc_prf_proc_t *)SLIST_FIRST(&profileCommonServer.list));
}


/////////////////////////////Profile Client Common API//////////////////

void blc_prf_setDiscoveryStatusFinish(u16 connHandle)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if(aclIdx < 0){
        return;
    }

    blc_prf_proc_t * sNode = gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId;
    gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId = sNode->pNext;
    if(!sNode->pNext)
    {
        blt_prf_sendCltSdpOverEvt(connHandle);

        if(prf_store_used)
        {
            blt_prf_storeNvData(connHandle);
        }
    }
    gProfileCtrl.sdpCtrl[aclIdx].clientRdySdp = 0;
}

bool blc_prf_checkDiscoveryBusy(u16 connHandle)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if(aclIdx < 0){
        return FALSE;
    }

    return gProfileCtrl.sdpCtrl[aclIdx].clientRdySdp;
}

void blc_prf_setDiscoveryStatusBusy(u16 connHandle)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if(aclIdx < 0){
        return;
    }

    gProfileCtrl.sdpCtrl[aclIdx].clientRdySdp = 1;
}

bool blc_prf_checkReconnectFlag(u16 connHandle)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if(aclIdx < 0){
        return FALSE;
    }

    return gProfileCtrl.sdpCtrl[aclIdx].reconnFlag;
}


////////////////////storage////////
static int blt_prf_storeHandler(u16 connHandle, blc_prf_proc_t* elem, prf_nv_param_t* param)
{
    st_ll_conn_t* pAclConn = blt_ll_getAclConnPtr(connHandle);
    /* pAclConn->aclRole refer to 'acl_connection_role_t' */
    prf_bound_acl_role_enum currAudAclRole = BIT(pAclConn->aclRole);

    while (elem != NULL) {
        /* It is necessary to verify whether the declared SVC node's LL_ROLE is
         * consistent with the current ACL connection role. */
        if (elem->usedAclRole & currAudAclRole) {
            if(elem->store) {
                elem->store(connHandle, PRF_NV_STATE_STORE, param);
            }
        }
        elem = elem->pNext;
    }
    return 0;
}

void blt_audio_userNvData(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param)
{
    (void)connHandle;
    (void)nvState;
    (void)param;
}

static void blt_prf_storeNvData(u16 connHandle)
{
    u8 aclIdx = blc_prf_getAclConnectIndex(connHandle); //connHandle valid already
    if(gProfileCtrl.sdpCtrl[aclIdx].reconnFlag)
    {
        return ;
    }

    u8 nvData[AUD_NV_VALUE_SIZE];

    memset(nvData, 0, AUD_NV_VALUE_SIZE);

    prf_nv_param_t param = {
        .dataPtr = nvData,
        .currentTotalLen = 0,
    };

    blc_prf_proc_t* elem = (blc_prf_proc_t *)SLIST_FIRST(&profileCommonClient.list);
    blt_prf_storeHandler(connHandle, elem, &param);

    elem = (blc_prf_proc_t *)SLIST_FIRST(&profileCommonServer.list);
    blt_prf_storeHandler(connHandle, elem, &param);
    blt_audio_userNvData(connHandle, PRF_NV_STATE_STORE, &param);

    blt_prf_saveBondingInformationToFlash(connHandle, nvData, param.currentTotalLen);
}

static int blt_prf_loadHandler(u16 connHandle, u8 id, prf_nv_param_t* param)
{
    st_ll_conn_t* pAclConn = blt_ll_getAclConnPtr(connHandle);
    /* pAclConn->aclRole refer to 'acl_connection_role_t' */
    prf_bound_acl_role_enum currAudAclRole = BIT(pAclConn->aclRole);
    blc_prf_proc_t* elem = (blc_prf_proc_t *)SLIST_FIRST(&profileCommonClient.list);

    while (elem != NULL) {
        /* It is necessary to verify whether the declared SVC node's LL_ROLE is
         * consistent with the current ACL connection role. */
        if ((elem->usedAclRole & currAudAclRole) && (elem->id == id)) {
            if(elem->store) {
                elem->store(connHandle, PRF_NV_STATE_LOAD, param);
            }
        }

        elem = elem->pNext;
    }

    elem = (blc_prf_proc_t *)SLIST_FIRST(&profileCommonServer.list);

    while (elem != NULL) {
        /* It is necessary to verify whether the declared SVC node's LL_ROLE is
         * consistent with the current ACL connection role. */
        if ((elem->usedAclRole & currAudAclRole) && (elem->id == id)) {
            if(elem->store) {
                elem->store(connHandle, PRF_NV_STATE_LOAD, param);
            }
        }

        elem = elem->pNext;
    }

    return 0;

}

static void blt_prf_loadNvData(u16 connHandle)
{
    u8 nvData[AUD_NV_VALUE_SIZE];
    u16 nvDataLen = blt_prf_loadPairingInfoByAclHandle(connHandle, nvData);
    u8 aclIdx = blc_prf_getAclConnectIndex(connHandle); //connHandle valid already
    gProfileCtrl.sdpCtrl[aclIdx].reconnFlag = nvDataLen!=0;

    u8* ptr = nvData;
    prf_nv_param_t param;

    while(nvDataLen)
    {
        param.nvItemLen = ptr[0];
        param.dataPtr = ptr+2;
        if(ptr[1] == 0xFF)
        {
            blt_audio_userNvData(connHandle, PRF_NV_STATE_LOAD, &param);
        }
        else
        {
            blt_prf_loadHandler(connHandle, ptr[1], &param);
        }

        if(nvDataLen > (ptr[0] + 2)){
            nvDataLen -= (ptr[0] + 2);
            ptr += (ptr[0] + 2);
        }else{
            nvDataLen = 0;
        }
    }

}

////////////////Profile Common event/////////
static void blt_prf_sendSecurityDoneEvt(u16 connHandle)
{
    blc_prf_securityDoneEvt_t pEvt;
    pEvt.aclHandle = connHandle;
    blt_prf_sendEvent(pEvt.aclHandle, PRF_EVTID_SMP_SECURITY_DONE, (u8*)&pEvt, sizeof(blc_prf_securityDoneEvt_t));
}

//////////////////////////////////////////////////////////////////////////////////
//          LE stack event wrapped by audio profile
//////////////////////////////////////////////////////////////////////////////////
static void blt_prf_sendAclConnEvt(hci_le_enhancedConnCompleteEvt_t *pData)
{
    blc_prf_aclConnEvt_t pEvt;
    pEvt.aclHandle = pData->connHandle;
    pEvt.connInterval = pData->connInterval;
    pEvt.PeerAddrType = pData->PeerAddrType;
    memcpy(pEvt.PeerAddr,pData->PeerAddr,6);
    blt_prf_sendEvent(pEvt.aclHandle, PRF_EVTID_ACL_CONNECT, (u8*)&pEvt, sizeof(blc_prf_aclConnEvt_t));
}

static void blt_prf_sendAclDisconnEvt(hci_disconnectionCompleteEvt_t *pData)
{
    blc_prf_aclDisconnEvt_t pEvt;
    pEvt.aclHandle = pData->connHandle;
    pEvt.reason = pData->reason;
    blt_prf_sendEvent(pEvt.aclHandle, PRF_EVTID_ACL_DISCONNECT, (u8*)&pEvt, sizeof(blc_prf_aclDisconnEvt_t));
}

void blt_prf_sendSvrGapRoleErrEvt(u16 connHandle, int svcId, acl_connection_role_t currAclRole)
{
    blc_prf_svrGapRoleErrorEvt_t pEvt = {
        .connHandle = connHandle,
        .svcId = svcId,
        .currAclRole = currAclRole
    };
//  BLT_AUD_LOG("ERR: Server gap role is wrong:svcId[0x%x] currAclRole[0x%x]", svcId, currAclRole);
    blt_prf_sendEvent(connHandle, PRF_EVTID_SERVICE_ACL_ROLE_FAIL, (u8*)&pEvt, sizeof(blc_prf_svrGapRoleErrorEvt_t));
}

void blt_prf_sendCltSdpOverEvt(u16 connHandle)
{
    blc_prf_sdpOverEvt_t sdpOverEvt;
    sdpOverEvt.aclHandle = connHandle;
    blt_prf_sendEvent(connHandle, PRF_EVTID_CLIENT_ALL_SDP_OVER, (u8*)&sdpOverEvt, sizeof(blc_prf_sdpOverEvt_t));
}

void blc_prf_sendServiceDiscoveryFailEvent(u16 connHandle, int svcId)
{
    blc_prf_sdpFailEvt_t pEvt = {
        .aclHandle = connHandle,
        .svcId = svcId
    };
    blt_prf_sendEvent(connHandle, PRF_EVTID_CLIENT_SDP_FAIL, (u8*)&pEvt, sizeof(blc_prf_sdpFailEvt_t));
}

void blc_prf_sendServiceDiscoveryFoundEvent(u16 connHandle, int svcId, u16 startHdl, u16 endHdl)
{
    blc_prf_sdpFoundEvt_t pEvt = {
        .aclHandle = connHandle,
        .svcId = svcId,
        .startHdl = startHdl,
        .endHdl = endHdl
    };
    blt_prf_sendEvent(connHandle, PRF_EVTID_CLIENT_SDP_FOUND, (u8*)&pEvt, sizeof(blc_prf_sdpFoundEvt_t));
}

void blc_prf_sendSingleServiceDiscoveryFinishEvent(u16 connHandle, int svcId)
{
    blc_prf_sdpEndEvt_t pEvt = {
        .aclHandle = connHandle,
        .svcId = svcId
    };
    blt_prf_sendEvent(connHandle, PRF_EVTID_CLIENT_SDP_END, (u8*)&pEvt, sizeof(blc_prf_sdpEndEvt_t));
}

