#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "stack/ble/host/gatt/tlk_list_stack.h"
#if ((!defined(HOST_V2_ENABLE)))
#include "stack/ble/host/gatt/tlk_timer_stack.h"

static int  blt_prf_getPriorityValueCb(void* node);
static void blt_prf_aclChangeStateCb(u16 connHandle, GAP_STATE_ENUM state, void* node);
static void blt_prf_svcQHandler(u16 connHandle, prf_proc_type_enum procType);
static void blt_prf_sendSecurityDoneEvt(u16 connHandle);
static void blt_prf_sendAclConnEvt(hci_le_connectionCompleteEvt_t *pData);
static void blt_prf_sendAclConnEvt_enhanced_v1(hci_le_enhancedConnCompleteEvt_t *pData);
static void blt_prf_sendAclConnEvt_enhanced_v2(hci_le_enhancedConnCompleteEvt_t_V2 *pData);
static void blt_prf_sendAclDisconnEvt(hci_disconnectionCompleteEvt_t *pData);
static void blt_prf_sendAclConnUpdateEvt(hci_le_connectionUpdateCompleteEvt_t *pData);
static void blt_prf_sendLeAdvertisingData(event_adv_report_t *pData);
static void blt_prf_sendCltSdpOverEvt(u16 connHandle);
static int  blt_prf_hciEventCb(u32 h, u8 *p, int n);
static void blt_prf_loadNvData(u16 connHandle);


_attribute_ble_data_retention_ blt_prf_control_t gProfileCtrl;

_attribute_ble_data_retention_ static SPLIST_DEF(profileCommonClient, blt_prf_getPriorityValueCb);

_attribute_ble_data_retention_ static SPLIST_DEF(profileCommonServer, blt_prf_getPriorityValueCb);

_attribute_ble_data_retention_ static struct gap_stateChangeNode prfModuleCallBack = {
    .cb = blt_prf_aclChangeStateCb};

_attribute_ble_data_retention_ static struct gap_hciEventNode prfHciEventCallBack = {
    .cb = blt_prf_hciEventCb,
};

static int blt_prf_getPriorityValueCb(void *node)
{
    return ((blc_prf_proc_t *)node)->id;
}
#else
//#include "stack/ble/host/ble_host_internal.h"

/**
 *   @brief  Initialize the profile common control block.
 */
SLIST_HEAD(blt_prf_process_list, blc_prf_process);

/********************* profile common control block *******************/

static void blt_prf_aclChangeStateCb(u16 connHandle, GAP_STATE_ENUM state, void *node);
static int  blt_prf_hciEventCb(u32 h, u8 *p, int n);

_attribute_ble_data_retention_ static blt_prf_control_t gProfileCtrl;

_attribute_ble_data_retention_ static struct blt_prf_process_list s_ble_profile_process_client_list;
_attribute_ble_data_retention_ static struct blt_prf_process_list s_ble_profile_process_server_list;

_attribute_ble_data_retention_ static struct gap_stateChangeNode prfModuleCallBack = {
    .cb = blt_prf_aclChangeStateCb};

_attribute_ble_data_retention_ static struct gap_hciEventNode prfHciEventCallBack = {
    .cb = blt_prf_hciEventCb,
};

static void blt_prf_aclStateHandler(u16 connHandle, prf_acl_state_enum state);
static void blt_prf_startSdpDiscovery(u16 connHandle);

static void blt_prf_startDiscovery(u16 connHandle);
static void blt_prf_sendSecurityDoneEvt(u16 connHandle);
static void blt_prf_sendAclConnEvt(hci_le_connectionCompleteEvt_t *pData);
static void blt_prf_sendAclConnEvt_enhanced_v1(hci_le_enhancedConnCompleteEvt_t *pData);
static void blt_prf_sendAclConnEvt_enhanced_v2(hci_le_enhancedConnCompleteEvt_t_V2 *pData);
static void blt_prf_sendAclDisconnEvt(hci_disconnectionCompleteEvt_t *pData);
static void blt_prf_sendAclConnectIntervalUpdate(hci_le_connectionUpdateCompleteEvt_t *pData);
static void blt_prf_sendLeAdvertisingData(event_adv_report_t *pData);
static void blt_prf_sendCltSdpOverEvt(u16 connHandle);
static void blt_prf_loadNvData(u16 connHandle);
#endif // endif of #if ((!defined(HOST_V2_ENABLE)))
static void blt_prf_storeNvData(u16 connHandle, bool forceFlag);

static void blt_prf_sendLeCsReadRemoteSupCapCompleteEvt(hci_le_readRemoteSupCapCompleteEvt_t *pEvt);
static void blt_prf_sendLeCsRead_RemoteFaeTableCompleteEvt(hci_le_readRemoteFAETableCompleteEvt_t *pEvt);
static void blt_prf_sendLeCsConfigCompleteEvt(hci_le_csConfigCompleteEvt_t *pEvt);
static void blt_prf_sendLeCsSecurityEnableCompleteEvt(hci_le_csSecurityEnableCompleteEvt_t *pEvt);
static void blt_prf_sendLeCsProcedureEnableCompleteEvt(hci_le_csProcedureEnableCompleteEvt_t *pEvt);
static void blt_prf_sendLeCsSubeventResultEvt(hci_le_csSubeventResultEvt_t *pEvt);
static void blt_prf_sendLeCsSubeventResultContinueEvt(hci_le_csSubeventResultContinueEvt_t *pEvt);

static void blt_prf_aclChangeStateCb(u16 connHandle, GAP_STATE_ENUM state, void *node)
{
#if ((!defined(HOST_V2_ENABLE)))
    (void)node;
#if (LL_ASYNC_LEA_EN)
    if (IS_ASYNC_LEA_LINK(connHandle)) {
        return;
    }
#endif
    switch (state) {
    case GAP_STATE_ACL_CONNECTED:
        blt_prf_svcQHandler(connHandle, PRF_PROC_CONN);
        break;
    case GAP_STATE_ACL_DISCONNECTED:
        blt_prf_svcQHandler(connHandle, PRF_PROC_DISCONN);
        int aclIdx                   = blc_prf_getAclConnectIndex(connHandle);
        gProfileCtrl.writeCb[aclIdx] = NULL;
        gProfileCtrl.readCb[aclIdx]  = NULL;
        break;
    case GAP_STATE_SECURITY_DONE:
        /* app event callback */
        blt_prf_sendSecurityDoneEvt(connHandle);
        blt_prf_svcQHandler(connHandle, PRF_PROC_DISCOVERY);
        if (prf_store_used) {
            blt_prf_loadNvData(connHandle);
        }
        break;
    default:
        break;
    }
#else
    (void)node;
    switch (state) {
    case GAP_STATE_ACL_CONNECTED:
        blt_prf_aclStateHandler(connHandle, PRF_ACL_STATE_CONNECT);
        break;
    case GAP_STATE_ACL_DISCONNECTED:
        blt_prf_aclStateHandler(connHandle, PRF_ACL_STATE_DISCONN);
        blc_gattc_cleanAllSubscribeCCCNode(connHandle);
        int aclIdx                                 = blc_prf_getAclConnectIndex(connHandle);
        gProfileCtrl.writeCb[aclIdx]               = NULL;
        gProfileCtrl.readCb[aclIdx]                = NULL;
        gProfileCtrl.sdpCtrl[aclIdx].clientRdySdp  = 0;
        gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId = NULL;
        break;
    case GAP_STATE_SECURITY_DONE:
        /* app event callback */
        blt_prf_sendSecurityDoneEvt(connHandle);
        blt_prf_startSdpDiscovery(connHandle);

        if (prf_store_used) {
            blt_prf_loadNvData(connHandle);
        }
        break;
    default:
        break;
    }
#endif
}

static int blt_prf_hciEventCb(u32 h, u8 *p, int n)
{
    (void)n;
    if (h & HCI_FLAG_EVENT_BT_STD) //Controller HCI event
    {
        u8 evtCode     = h & 0xff;
        u8 subEvt_code = p[0];

        if (evtCode == HCI_EVT_DISCONNECTION_COMPLETE)
        {
            hci_disconnectionCompleteEvt_t *pDisConn   = (hci_disconnectionCompleteEvt_t *)p;
            u16                             connHandle = pDisConn->connHandle;
            /* ACL central OR peripheral disconnection */
            if (connHandle & BLT_ACL_CONN_HANDLE) {
            #if ((!defined(HOST_V2_ENABLE)))
                blc_gatts_notifyDisconnect(connHandle);
            #else
                blt_prf_sendAclDisconnEvt(pDisConn);
            #endif
            }
        }
        else if (evtCode == HCI_EVT_LE_META)
        {
            if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE)
            {
                hci_le_connectionCompleteEvt_t *pConnEvt = (hci_le_connectionCompleteEvt_t *)p;
                blt_prf_sendAclConnEvt(pConnEvt);
            }
            else if (subEvt_code == HCI_SUB_EVT_LE_ENHANCED_CONNECTION_COMPLETE)
            {
                hci_le_enhancedConnCompleteEvt_t *pConnEvt = (hci_le_enhancedConnCompleteEvt_t *)p;
                blt_prf_sendAclConnEvt_enhanced_v1(pConnEvt);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_ENHANCED_CONNECTION_COMPLETE_V2)
            {
                hci_le_enhancedConnCompleteEvt_t_V2 *pConnEvt = (hci_le_enhancedConnCompleteEvt_t_V2 *)p;
                blt_prf_sendAclConnEvt_enhanced_v2(pConnEvt);
            }
            else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE)
            {
                hci_le_connectionUpdateCompleteEvt_t *pConnUpdateEvt = (hci_le_connectionUpdateCompleteEvt_t *)p;
            #if ((!defined(HOST_V2_ENABLE)))
                blt_prf_sendAclConnUpdateEvt(pConnUpdateEvt);
            #else
                blt_prf_sendAclConnectIntervalUpdate(pConnUpdateEvt);
            #endif
            }
            else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)
            {
                event_adv_report_t *pAdvRpt = (event_adv_report_t *)p;
                blt_prf_sendLeAdvertisingData(pAdvRpt);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES_COMPLETE){

                blt_prf_sendLeCsReadRemoteSupCapCompleteEvt((hci_le_readRemoteSupCapCompleteEvt_t*)p);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_CS_READ_REMOTE_FAE_TABLE_COMPLETE){
                blt_prf_sendLeCsRead_RemoteFaeTableCompleteEvt((hci_le_readRemoteFAETableCompleteEvt_t *)p);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_CS_CONFIG_COMPLETE){
                blt_prf_sendLeCsConfigCompleteEvt((hci_le_csConfigCompleteEvt_t *)p);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_CS_SECURITY_ENABLE_COMPLETE){
                blt_prf_sendLeCsSecurityEnableCompleteEvt((hci_le_csSecurityEnableCompleteEvt_t *)p);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_CS_PROCEDURE_ENABLE_COMPLETE){
                blt_prf_sendLeCsProcedureEnableCompleteEvt((hci_le_csProcedureEnableCompleteEvt_t *)p);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT){
                blt_prf_sendLeCsSubeventResultEvt((hci_le_csSubeventResultEvt_t *)p);
            }
            else if(subEvt_code == HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT_CONTINUE){
                blt_prf_sendLeCsSubeventResultContinueEvt((hci_le_csSubeventResultContinueEvt_t *)p);
            }
        }
    }
    else //host event
    {
        u8 event = h & 0xFF;
        switch (event) {
        case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
        {
            gap_smp_securityProcessDoneEvt_t *pData = (gap_smp_securityProcessDoneEvt_t *)p;
            (void)pData; //remove warning
        } break;

        default:
            break;
        }
    }
    return 0;
}

////////////////////////Profile initial and register function//////////////////////////
#if ((!defined(HOST_V2_ENABLE)))
void blc_prf_initialModule(prf_evt_cb_t evtCb,void *base, u32 size)
{
    blt_host_init(base,size); //TODO: by qihang.mou
                     //  /* Special protection code for use */
                     //  if(pm_check_info){
                     //      blc_hci_le_setEventMask_cmd(0xFFFFFFFF);
                     //      blc_hci_le_setEventMask_2_cmd(0xFFFFFFFF);

    blt_gap_regAclConnState(&prfModuleCallBack);
    blt_gap_regHciEventCb(&prfHciEventCallBack);
    gProfileCtrl.evtCb = evtCb;
}
#else
//void blc_prf_initialModule(prf_evt_cb_t evtCb)
void blc_prf_initialModule(prf_evt_cb_t evtCb,void *base, u32 size)
{
//    blt_host_init(); //TODO: by qihang.mou
    blt_host_init(base,size);
                     //  /* Special protection code for use */
                     //  if(pm_check_info){
                     //      blc_hci_le_setEventMask_cmd(0xFFFFFFFF);
                     //      blc_hci_le_setEventMask_2_cmd(0xFFFFFFFF);

    blt_gap_regAclConnState(&prfModuleCallBack);
    blt_gap_regHciEventCb(&prfHciEventCallBack);
    gProfileCtrl.evtCb = evtCb;
//    blc_prf_main_loop_cb = blc_prf_main_loop;
    //  }
}
#endif

#if ((!defined(HOST_V2_ENABLE)))
static void blt_prf_registerServiceNode(blc_prf_proc_t *pNode, const void *param)
{
    struct single_priority_list *list = pNode->id < PRF_SERVER_OFFSET ? &profileCommonClient : &profileCommonServer;

    SPLIST_INSERT_NODE(list, pNode);

    if (pNode->init) {
        pNode->init(PRF_PROC_INIT, param);
    }
}
#else
static void blt_prf_registerServiceNode(struct blc_prf_process *pNode, const void *param)
{
    struct blt_prf_process_list *list = pNode->prf_params->id < PRF_SERVER_OFFSET ?
                                            &s_ble_profile_process_client_list :
                                            &s_ble_profile_process_server_list;

    struct blc_prf_process *cur  = NULL;
    struct blc_prf_process *prev = NULL;
    SLIST_FOREACH(cur, list, next)
    {
        if (cur == pNode) {
            return;
        }

        if (cur->prf_params->id > pNode->prf_params->id) {
            break;
        }

        prev = cur;
    }

    if (prev == NULL) {
        SLIST_NEXT(pNode, next) = NULL;
        SLIST_INSERT_HEAD(list, pNode, next);
    } else {
        SLIST_INSERT_AFTER(prev, pNode, next);
    }

    if (pNode->prf_params->init) {
        pNode->prf_params->init(PRF_PROC_INIT, param);
    }
}
#endif

#if ((!defined(HOST_V2_ENABLE)))
static void blt_prf_unregisterServiceNode(blc_prf_proc_t *pNode, const void *param)
{
    struct single_priority_list *list = pNode->id < PRF_SERVER_OFFSET ? &profileCommonClient : &profileCommonServer;

    SPLIST_DELETE_NODE(list, pNode);

    if (pNode->init) {
        pNode->init(PRF_PROC_DEINIT, param);
    }
}
#else
static void blt_prf_unregisterServiceNode(struct blc_prf_process *pNode, const void *param)
{
    struct blt_prf_process_list *list = pNode->prf_params->id < PRF_SERVER_OFFSET ?
                                            &s_ble_profile_process_client_list :
                                            &s_ble_profile_process_server_list;

    struct blc_prf_process *cur = NULL;

    SLIST_FOREACH(cur, list, next)
    {
        if (cur == pNode) {
            SLIST_REMOVE(list, pNode, blc_prf_process, next);
            break;
        }
    }

    if (pNode->prf_params->init) {
        pNode->prf_params->init(PRF_PROC_DEINIT, param);
    }
}
#endif

#if ((!defined(HOST_V2_ENABLE)))
void blc_prf_registerServiceModule(prf_bound_acl_role_enum usedAclRole, blc_prf_proc_t *pSvc, const void *param)
{
    if (!(usedAclRole & PRF_GAP_ACL_UNSPECIF) || pSvc == NULL) {
        return;
    }
    //  /* Special protection code for use */
    //  if(pm_check_info){
    pSvc->usedAclRole |= usedAclRole;
    blt_prf_registerServiceNode(pSvc, param);
    //  }
}
#else
void blc_prf_registerServiceModule(struct blc_prf_process *pSvc, const void *param)
{
//    if (pSvc == NULL) {
//        return;
//    }
    //  /* Special protection code for use */
    //  if(pm_check_info){
    blt_prf_registerServiceNode(pSvc, param);
    //  }
}
#endif

#if ((!defined(HOST_V2_ENABLE)))
void blc_prf_unregisterServiceModule(blc_prf_proc_t *pSvc, const void *param)
{
    if (pSvc == NULL) {
        return;
    }

    blt_prf_unregisterServiceNode(pSvc, param);
}
#else
void blc_prf_unregisterServiceModule(struct blc_prf_process *pSvc, const void *param)
{
    if (pSvc == NULL) {
        return;
    }

    blt_prf_unregisterServiceNode(pSvc, param);
}
#endif

int blt_prf_sendEvent(u16 connHandle, int evtID, void *pData, u16 dataLen)
{
    if (gProfileCtrl.evtCb == NULL) {
        return -1;
    } else {
        return gProfileCtrl.evtCb(connHandle, evtID, pData, dataLen); // app_ble_profile_event_callback
    }
}

////////////////////////Profile main loop function//////////////////////////
#if (defined(HOST_V2_ENABLE))

void blc_prf_main_loop(void)
{
    if (prf_store_used) {
        blt_prf_procBondingInfoIndexAlarm();
    }
    //TODO: currently used in LE Audio only
    blc_gapc_discoveryOrReconnectService_loop();
    blc_gatts_notifyLoop();
}
#else
void blc_prf_main_loop(void)
{
    u16 connHandle;
    for (int conn_idx = 0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) {
        st_ll_conn_t *pAclConn = (st_ll_conn_t *)&blms[conn_idx];

        connHandle = pAclConn->acl_conHandle;
        if (pAclConn->connState) {
            blt_prf_svcQHandler(connHandle, PRF_PROC_LOOP);
        }
    }

    if (prf_store_used) {
        blt_prf_procBondingInfoIndexAlarm();
    }

    //TODO: currently used in LE Audio only
    blc_gapc_discoveryOrReconnectService_loop();
    blc_gatts_notifyLoop();
#if (BLT_SOFTWARE_TIMER_ENABLE)
    blt_soft_timer_process(MAINLOOP_ENTRY);
#else
    soft_timer_process(MAINLOOP_ENTRY);
#endif
}
#endif
///////////////////////Profile Read/Write Attribute Value//////////////////

int blc_prf_readAttributeValue(u16 connHandle, gapc_read_cfg_t *pGapReCfg, prf_read_cb_t readCb)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if (aclIdx < 0) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    ble_sts_t status = blc_gapc_readAttributeValue(connHandle, pGapReCfg);
    if (status == BLE_SUCCESS) {
        gProfileCtrl.readCb[aclIdx] = readCb;
    }

    return status;
}

void blc_prf_readAttributeValueCallback(u16 connHandle, att_err_t err)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if (aclIdx < 0) {
        return;
    }

    if (gProfileCtrl.readCb[aclIdx]) {
        gProfileCtrl.readCb[aclIdx](connHandle, err);
    }
}

void blc_prf_readAttributeValueDefaultCallback(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    (void)pRdCfg;
    blc_prf_readAttributeValueCallback(connHandle, err);
}

int blc_prf_writeAttributeValue(u16 connHandle, gapc_write_cfg_t *pGapWrCfg, prf_write_cb_t writeCb)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if (aclIdx < 0) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    ble_sts_t status = blc_gapc_writeAttributeValue(connHandle, pGapWrCfg);

    if (status == BLE_SUCCESS) {
        gProfileCtrl.writeCb[aclIdx] = writeCb;
    }

    return status;
}

void blc_prf_writeAttributeValueCallback(u16 connHandle, att_err_t err)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if (aclIdx < 0) {
        return;
    }

    if (gProfileCtrl.writeCb[aclIdx]) {
        gProfileCtrl.writeCb[aclIdx](connHandle, err);
    }
}

void blc_prf_writeAttributeValueDefaultCallback(u16 connHandle, u8 err, void *data)
{
    (void)data;
    blc_prf_writeAttributeValueCallback(connHandle, err);
}

//////////////////////Profile Common Tools Function////////////////////////
int blc_prf_getAclConnectIndex(u16 connHandle)
{
    if (blt_ll_isAclHandleOutOfRange(connHandle) != BLE_SUCCESS) {
        return -1;
    }

    /* DO NOT check connection state */
    u8 conn_idx = connHandle & CONN_IDX_MASK;

    if (conn_idx >= LL_MAX_ACL_CEN_NUM && (conn_idx - LL_MAX_ACL_CEN_NUM) < blmsParam.max_slave_num) {
        return blmsParam.max_master_num + (conn_idx - LL_MAX_ACL_CEN_NUM);
    } else if (conn_idx < LL_MAX_ACL_CEN_NUM && conn_idx < blmsParam.max_master_num) {
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
    if (blt_ll_isAclHandleOutOfRange(connHandle)) {
        return -1;
    }


    u8 central_role = connHandle & BLM_CONN_HANDLE;
    /* refer to: acl_connection_role_t */
    return central_role ? ACL_ROLE_CENTRAL : ACL_ROLE_PERIPHERAL;
}

#if (defined(HOST_V2_ENABLE))
static prf_bound_acl_role_enum blt_prf_getProcType(u16 connHandle)
{
    // TODO:
    u8 central_role = connHandle & BLM_CONN_HANDLE;
    return central_role ? PRF_GAP_ACL_CENTRAL : PRF_GAP_ACL_PERIPHERAL;
}
#endif

/////////////////////////////Profile traverse list///////////////////////
#if (defined(HOST_V2_ENABLE))
static void blt_prf_aclConnStateChangeHandler(u16 connHandle, prf_acl_state_enum state, struct blc_prf_process *elem)
{
    prf_bound_acl_role_enum currAudAclRole = blt_prf_getProcType(connHandle);

    while (elem != NULL) {
        /* It is necessary to verify whether the declared SVC node's LL_ROLE is
         * consistent with the current ACL connection role. */
        if ((elem->prf_params->usedAclRole & currAudAclRole) && elem->prf_params->connect) {
            elem->prf_params->connect(connHandle, state);
        }
        elem = elem->next.sle_next;
    }
}

static void blt_prf_aclStateHandler(u16 connHandle, prf_acl_state_enum state)
{
    blt_prf_aclConnStateChangeHandler(connHandle, state, SLIST_FIRST(&s_ble_profile_process_client_list));
    blt_prf_aclConnStateChangeHandler(connHandle, state, SLIST_FIRST(&s_ble_profile_process_server_list));
}
#endif

#if ((!defined(HOST_V2_ENABLE)))
static void blt_prf_svcHandler(u16 connHandle, prf_proc_type_enum procType, blc_prf_proc_t *elem)
{
    st_ll_conn_t *pAclConn = blt_ll_getAclConnPtr(connHandle);
    /* pAclConn->aclRole refer to 'acl_connection_role_t' */
    prf_bound_acl_role_enum currAudAclRole = BIT(pAclConn->aclRole);

    while (elem != NULL) {
        /* It is necessary to verify whether the declared SVC node's LL_ROLE is
         * consistent with the current ACL connection role. */
        if (elem->usedAclRole & currAudAclRole) {
            switch (procType) {
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
                    elem->loop(connHandle); //blt_*_loop
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
    if (IS_ASYNC_LEA_LINK(connHandle)) {
        return;
    }
#endif

    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if (aclIdx < 0) {
        return;
    }

    st_ll_conn_t *pAclConn = blt_ll_getAclConnPtr(connHandle);

    if (procType == PRF_PROC_CONN) {
        //      BLT_AUD_LOG("audio connected, addr is %s", addr_to_str(pAclConn->conn_peerPktA));

    } else if (procType == PRF_PROC_DISCONN) {
        /* Bonded ATT handles information and CCC subscription information to store in FLASH. TODO:XXX */

        //      BLT_AUD_LOG("audio disconnected, addr is %s", addr_to_str(pAclConn->conn_peerPktA));
        blc_gattc_cleanAllSubscribeCCCNode(connHandle);

        gProfileCtrl.sdpCtrl[aclIdx].clientRdySdp  = 0;
        gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId = NULL;
    } else if (procType == PRF_PROC_DISCOVERY) {
        if (SLIST_FIRST(&profileCommonClient.list)) {
            gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId = (blc_prf_proc_t *)SLIST_FIRST(&profileCommonClient.list);
            //          BLT_AUD_LOG("audio discovery[aclIdx:%d][sdpId:%d]", aclIdx, gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId->id);
        }
        return;
    } else if (procType == PRF_PROC_LOOP) {
        /* pAclConn->aclRole refer to 'acl_connection_role_t' */
        prf_bound_acl_role_enum currAudAclRole = BIT(pAclConn->aclRole);
        blc_prf_proc_t         *sNode          = gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId;
        if (sNode) {
            if ((sNode->usedAclRole & currAudAclRole) && sNode->discov) {
                sNode->discov(connHandle);
            } else {
                blc_prf_setDiscoveryStatusFinish(connHandle);
            }
        }
    }

    blt_prf_svcHandler(connHandle, procType, (blc_prf_proc_t *)SLIST_FIRST(&profileCommonClient.list));
    blt_prf_svcHandler(connHandle, procType, (blc_prf_proc_t *)SLIST_FIRST(&profileCommonServer.list));
}
#endif

/////////////////////////////Profile Client Common API//////////////////
#if (defined(HOST_V2_ENABLE))
static void blt_prf_startDiscovery(u16 connHandle)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);

    prf_bound_acl_role_enum currAudAclRole = blt_prf_getProcType(connHandle);
    struct blc_prf_process *sNode          = gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId;
    if (sNode) {
        // BLT_AUD_LOG("audio loop[aclIdx:%d][sdpId:%d]", aclIdx, sNode->id);
        // BLT_AUD_LOG("role:0x%x, currAudAclRole:0x%x", sNode->usedAclRole, currAudAclRole);
        if ((sNode->prf_params->usedAclRole & currAudAclRole) && sNode->prf_params->discovery) {
            sNode->prf_params->discovery(connHandle);
        } else {
            blc_prf_setDiscoveryStatusFinish(connHandle);
        }
    }
}

static void blt_prf_startSdpDiscovery(u16 connHandle)
{
    if (SLIST_FIRST(&s_ble_profile_process_client_list)) {
        int aclIdx                                 = blc_prf_getAclConnectIndex(connHandle);
        gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId = SLIST_FIRST(&s_ble_profile_process_client_list);
        //BLT_AUD_LOG("audio discovery[aclIdx:%d][sdpId:%d]", aclIdx, gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId->prf_params->id);
        blt_prf_startDiscovery(connHandle);
    }
}
#endif

void blc_prf_setDiscoveryStatusFinish(u16 connHandle)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if (aclIdx < 0) {
        return;
    }

#if ((!defined(HOST_V2_ENABLE)))
    blc_prf_proc_t *sNode                      = gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId;
    gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId = sNode->pNext;
    if (!sNode->pNext) {
#else
    struct blc_prf_process *sNode              = gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId;
    gProfileCtrl.sdpCtrl[aclIdx].currSvcNodeId = sNode->next.sle_next;
    if (!sNode->next.sle_next) {
#endif
        blt_prf_sendCltSdpOverEvt(connHandle);

        if (prf_store_used) {
            blt_prf_storeNvData(connHandle, false);
        }
    }
    gProfileCtrl.sdpCtrl[aclIdx].clientRdySdp = 0;
#if (defined(HOST_V2_ENABLE))
    blt_prf_startDiscovery(connHandle);
#endif
}

bool blc_prf_checkDiscoveryBusy(u16 connHandle)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if (aclIdx < 0) {
        return FALSE;
    }

    return gProfileCtrl.sdpCtrl[aclIdx].clientRdySdp;
}

void blc_prf_setDiscoveryStatusBusy(u16 connHandle)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if (aclIdx < 0) {
        return;
    }

    gProfileCtrl.sdpCtrl[aclIdx].clientRdySdp = 1;
}

bool blc_prf_checkReconnectFlag(u16 connHandle)
{
    int aclIdx = blc_prf_getAclConnectIndex(connHandle);
    if (aclIdx < 0) {
        return FALSE;
    }

    return gProfileCtrl.sdpCtrl[aclIdx].reconnFlag;
}

////////////////////storage////////
#if ((!defined(HOST_V2_ENABLE)))
static int blt_prf_storeHandler(u16 connHandle, blc_prf_proc_t *elem, prf_nv_param_t *param)
{
    st_ll_conn_t *pAclConn = blt_ll_getAclConnPtr(connHandle);
    /* pAclConn->aclRole refer to 'acl_connection_role_t' */
    prf_bound_acl_role_enum currAudAclRole = BIT(pAclConn->aclRole);

    while (elem != NULL) {
        /* It is necessary to verify whether the declared SVC node's LL_ROLE is
         * consistent with the current ACL connection role. */
        if (elem->usedAclRole & currAudAclRole) {
            if (elem->store) {
                elem->store(connHandle, PRF_NV_STATE_STORE, param);
            }
        }
        elem = elem->pNext;
    }
    return 0;
}
#else
static int blt_prf_storeHandler(u16 connHandle, struct blc_prf_process *elem, prf_nv_param_t *param)
{
    st_ll_conn_t *pAclConn = blt_ll_getAclConnPtr(connHandle);
    /* pAclConn->aclRole refer to 'acl_connection_role_t' */
    prf_bound_acl_role_enum currAudAclRole = BIT(pAclConn->aclRole);

    while (elem != NULL) {
        /* It is necessary to verify whether the declared SVC node's LL_ROLE is
         * consistent with the current ACL connection role. */
        if (elem->prf_params->usedAclRole & currAudAclRole) {
            if (elem->prf_params->store) {
                elem->prf_params->store(connHandle, PRF_NV_STATE_STORE, param);
            }
        }
        elem = elem->next.sle_next;
    }
    return 0;
}
#endif

void blt_audio_userNvData(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param)
{
    (void)connHandle;
    (void)nvState;
    (void)param;
}

static void blt_prf_storeNvData(u16 connHandle, bool forceFlag)
{
    u8 aclIdx = blc_prf_getAclConnectIndex(connHandle); //connHandle valid already
    if ((gProfileCtrl.sdpCtrl[aclIdx].reconnFlag) && (!forceFlag))
    {
        return;
    }

    u8 nvData[AUD_NV_VALUE_SIZE];

    memset(nvData, 0, AUD_NV_VALUE_SIZE);

    prf_nv_param_t param = {
        .dataPtr         = nvData,
        .currentTotalLen = 0,
    };
#if ((!defined(HOST_V2_ENABLE)))
    blc_prf_proc_t *elem = (blc_prf_proc_t *)SLIST_FIRST(&profileCommonClient.list);
    blt_prf_storeHandler(connHandle, elem, &param);

    elem = (blc_prf_proc_t *)SLIST_FIRST(&profileCommonServer.list);
    blt_prf_storeHandler(connHandle, elem, &param);
    blt_audio_userNvData(connHandle, PRF_NV_STATE_STORE, &param);
#else

    struct blc_prf_process *elem = SLIST_FIRST(&s_ble_profile_process_client_list);
    blt_prf_storeHandler(connHandle, elem, &param);

    elem = SLIST_FIRST(&s_ble_profile_process_server_list);
    blt_prf_storeHandler(connHandle, elem, &param);

    blt_audio_userNvData(connHandle, PRF_NV_STATE_STORE, &param);
#endif
    blt_prf_saveBondingInformationToFlash(connHandle, nvData, param.currentTotalLen);
}

void blt_prf_updateNvData(u16 connHandle)
{
#if ((!defined(HOST_V2_ENABLE)))
    u8 nvData[AUD_NV_VALUE_SIZE];

    if (!prf_store_used) {
        return;
    }

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

    blc_prf_deletePairingInfoByConnHandle(connHandle);
    blt_prf_saveBondingInformationToFlash(connHandle, nvData, param.currentTotalLen);
#endif
}

void blt_prf_updatePairingInfoByAclHandle(u16 connHandle)
{
    u16 valueLen = 0;
    u32 flash_addr = blt_prf_searchBondingDeviceByAclHandle(connHandle, &valueLen);
    if (flash_addr && valueLen) {
        blt_prf_deleteBondingInfoByFlashAddress(flash_addr);
    }
    blt_prf_storeNvData(connHandle, true);
}

#if ((!defined(HOST_V2_ENABLE)))
static int blt_prf_loadHandler(u16 connHandle, u8 id, prf_nv_param_t *param)
{
    st_ll_conn_t *pAclConn = blt_ll_getAclConnPtr(connHandle);
    /* pAclConn->aclRole refer to 'acl_connection_role_t' */
    prf_bound_acl_role_enum currAudAclRole = BIT(pAclConn->aclRole);
    blc_prf_proc_t         *elem           = (blc_prf_proc_t *)SLIST_FIRST(&profileCommonClient.list);

    while (elem != NULL) {
        /* It is necessary to verify whether the declared SVC node's LL_ROLE is
         * consistent with the current ACL connection role. */
        if ((elem->usedAclRole & currAudAclRole) && (elem->id == id)) {
            if (elem->store) {
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
            if (elem->store) {
                elem->store(connHandle, PRF_NV_STATE_LOAD, param);
            }
        }

        elem = elem->pNext;
    }

    return 0;
}
#else
static int blt_prf_loadHandler(u16 connHandle, u8 id, prf_nv_param_t *param)
{
    st_ll_conn_t *pAclConn = blt_ll_getAclConnPtr(connHandle);
    /* pAclConn->aclRole refer to 'acl_connection_role_t' */
    prf_bound_acl_role_enum currAudAclRole = BIT(pAclConn->aclRole);
    struct blc_prf_process *elem           = SLIST_FIRST(&s_ble_profile_process_client_list);

    while (elem != NULL) {
        /* It is necessary to verify whether the declared SVC node's LL_ROLE is
         * consistent with the current ACL connection role. */
        if ((elem->prf_params->usedAclRole & currAudAclRole) && (elem->prf_params->id == id)) {
            if (elem->prf_params->store) {
                elem->prf_params->store(connHandle, PRF_NV_STATE_LOAD, param);
            }
        }

        elem = elem->next.sle_next;
    }

    elem = SLIST_FIRST(&s_ble_profile_process_server_list);

    while (elem != NULL) {
        /* It is necessary to verify whether the declared SVC node's LL_ROLE is
         * consistent with the current ACL connection role. */
        if ((elem->prf_params->usedAclRole & currAudAclRole) && (elem->prf_params->id == id)) {
            if (elem->prf_params->store) {
                elem->prf_params->store(connHandle, PRF_NV_STATE_LOAD, param);
            }
        }

        elem = elem->next.sle_next;
    }

    return 0;
}
#endif
static void blt_prf_loadNvData(u16 connHandle)
{
    u8  nvData[AUD_NV_VALUE_SIZE];
    u16 nvDataLen                           = blt_prf_loadPairingInfoByAclHandle(connHandle, nvData);
    u8  aclIdx                              = blc_prf_getAclConnectIndex(connHandle); //connHandle valid already
    if (aclIdx < 0) {
        return;
    }
    gProfileCtrl.sdpCtrl[aclIdx].reconnFlag = nvDataLen != 0;

    u8            *ptr = nvData;
    prf_nv_param_t param;

    while (nvDataLen) {
        param.nvItemLen = ptr[0];
        param.dataPtr   = ptr + 2;
        if (ptr[1] == 0xFF) {
            blt_audio_userNvData(connHandle, PRF_NV_STATE_LOAD, &param);
        } else {
            blt_prf_loadHandler(connHandle, ptr[1], &param);
        }

        if (nvDataLen > (ptr[0] + 2)) {
            nvDataLen -= (ptr[0] + 2);
            ptr += (ptr[0] + 2);
        } else {
            nvDataLen = 0;
        }
    }
}

////////////////Profile Common event/////////
static void blt_prf_sendSecurityDoneEvt(u16 connHandle)
{
    blc_prf_securityDoneEvt_t pEvt;
    pEvt.aclHandle = connHandle;
    blt_prf_sendEvent(pEvt.aclHandle, PRF_EVTID_SMP_SECURITY_DONE, (u8 *)&pEvt, sizeof(blc_prf_securityDoneEvt_t));
}

//////////////////////////////////////////////////////////////////////////////////
//          LE stack event wrapped by audio profile
//////////////////////////////////////////////////////////////////////////////////
static void blt_prf_sendAclConnEvt(hci_le_connectionCompleteEvt_t *pData)
{
    blc_prf_aclConnEvt_t pEvt;
    pEvt.aclHandle    = pData->connHandle;
    pEvt.connInterval = pData->connInterval;
    pEvt.PeerAddrType = pData->peerAddrType;
    memcpy(pEvt.PeerAddr, pData->peerAddr, 6);
    blt_prf_sendEvent(pEvt.aclHandle, PRF_EVTID_ACL_CONNECT, (u8 *)&pEvt, sizeof(blc_prf_aclConnEvt_t));
}

static void blt_prf_sendAclConnEvt_enhanced_v1(hci_le_enhancedConnCompleteEvt_t *pData)
{
    blc_prf_aclConnEvt_t pEvt;
    pEvt.aclHandle    = pData->connHandle;
    pEvt.connInterval = pData->connInterval;
    pEvt.PeerAddrType = pData->PeerAddrType;
    memcpy(pEvt.PeerAddr, pData->PeerAddr, 6);
    blt_prf_sendEvent(pEvt.aclHandle, PRF_EVTID_ACL_CONNECT, (u8 *)&pEvt, sizeof(blc_prf_aclConnEvt_t));
}
static void blt_prf_sendAclConnEvt_enhanced_v2(hci_le_enhancedConnCompleteEvt_t_V2 *pData)
{
    blc_prf_aclConnEvt_t pEvt;
    pEvt.aclHandle    = pData->connHandle;
    pEvt.connInterval = pData->connInterval;
    pEvt.PeerAddrType = pData->PeerAddrType;
    memcpy(pEvt.PeerAddr, pData->PeerAddr, 6);
    blt_prf_sendEvent(pEvt.aclHandle, PRF_EVTID_ACL_CONNECT, (u8 *)&pEvt, sizeof(blc_prf_aclConnEvt_t));
}
static void blt_prf_sendAclDisconnEvt(hci_disconnectionCompleteEvt_t *pData)
{
    blc_prf_aclDisconnEvt_t pEvt;
    pEvt.aclHandle = pData->connHandle;
    pEvt.reason    = pData->reason;
    blt_prf_sendEvent(pEvt.aclHandle, PRF_EVTID_ACL_DISCONNECT, (u8 *)&pEvt, sizeof(blc_prf_aclDisconnEvt_t));
}

static void blt_prf_sendAclConnUpdateEvt(hci_le_connectionUpdateCompleteEvt_t *pData)
{
    blt_prf_sendEvent(pData->connHandle, PRF_EVTID_ACL_CONNECT_UPDATE, (u8 *)&pData, sizeof(hci_le_connectionUpdateCompleteEvt_t));
}

static void blt_prf_sendLeAdvertisingData(event_adv_report_t *pData)
{
    blt_prf_sendEvent(0, PRF_EVTID_LE_ADVERTISING_REPORT, (u8 *)pData, sizeof(event_adv_report_t));
}

#if (defined(HOST_V2_ENABLE))
static void blt_prf_sendAclConnectIntervalUpdate(hci_le_connectionUpdateCompleteEvt_t *pData)
{
    blc_prf_aclConnectUpdateEvt_t pEvt;
    pEvt.aclHandle       = pData->connHandle;
    pEvt.connectInterval = pData->connInterval;
    blt_prf_sendEvent(pEvt.aclHandle, PRF_EVTID_ACL_CONNECT_UPDATE, (u8 *)&pEvt, sizeof(blc_prf_aclConnectUpdateEvt_t));
}
#endif

static void blt_prf_sendLeCsReadRemoteSupCapCompleteEvt(hci_le_readRemoteSupCapCompleteEvt_t *pEvt)
{
    blt_prf_sendEvent(pEvt->Connection_Handle, PRF_EVTID_LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES_COMPLETE, (u8 *)pEvt, sizeof(hci_le_readRemoteSupCapCompleteEvt_t));
}
static void blt_prf_sendLeCsRead_RemoteFaeTableCompleteEvt(hci_le_readRemoteFAETableCompleteEvt_t *pEvt)
{
    blt_prf_sendEvent(pEvt->Connection_Handle, PRF_EVTID_LE_CS_READ_REMOTE_FAE_TABLE_COMPLETE, (u8 *)pEvt, sizeof(hci_le_readRemoteFAETableCompleteEvt_t));
}
static void blt_prf_sendLeCsConfigCompleteEvt(hci_le_csConfigCompleteEvt_t *pEvt)
{
    blt_prf_sendEvent(pEvt->Connection_Handle, PRF_EVTID_LE_CS_CONFIG_COMPLETE, (u8 *)pEvt, sizeof(hci_le_csConfigCompleteEvt_t));
}

static void blt_prf_sendLeCsSecurityEnableCompleteEvt(hci_le_csSecurityEnableCompleteEvt_t *pEvt)
{
    blt_prf_sendEvent(pEvt->Connection_Handle, PRF_EVTID_LE_CS_SECURITY_ENABLE_COMPLETE, (u8 *)pEvt, sizeof(hci_le_csSecurityEnableCompleteEvt_t));
}

static void blt_prf_sendLeCsProcedureEnableCompleteEvt(hci_le_csProcedureEnableCompleteEvt_t *pEvt)
{
    blt_prf_sendEvent(pEvt->Connection_Handle, PRF_EVTID_LE_CS_PROCEDURE_ENABLE_COMPLETE, (u8 *)pEvt, sizeof(hci_le_csProcedureEnableCompleteEvt_t));
}

static void blt_prf_sendLeCsSubeventResultEvt(hci_le_csSubeventResultEvt_t *pEvt)
{
    blt_prf_sendEvent(pEvt->Connection_Handle, PRF_EVTID_LE_CS_SUBEVENT_RESULT, (u8 *)pEvt, sizeof(hci_le_csSubeventResultEvt_t));
}

static void blt_prf_sendLeCsSubeventResultContinueEvt(hci_le_csSubeventResultContinueEvt_t *pEvt)
{
    blt_prf_sendEvent(pEvt->Connection_Handle, PRF_EVTID_LE_CS_SUBEVENT_RESULT_CONTINUE, (u8 *)pEvt, sizeof(hci_le_csSubeventResultContinueEvt_t));
}

void blt_prf_sendSvrGapRoleErrEvt(u16 connHandle, int svcId, acl_connection_role_t currAclRole)
{
    blc_prf_svrGapRoleErrorEvt_t pEvt = {
        .connHandle  = connHandle,
        .svcId       = svcId,
        .currAclRole = currAclRole};
    //  BLT_AUD_LOG("ERR: Server gap role is wrong:svcId[0x%x] currAclRole[0x%x]", svcId, currAclRole);
    blt_prf_sendEvent(connHandle, PRF_EVTID_SERVICE_ACL_ROLE_FAIL, (u8 *)&pEvt, sizeof(blc_prf_svrGapRoleErrorEvt_t));
}

void blt_prf_sendCltSdpOverEvt(u16 connHandle)
{
    blc_prf_sdpOverEvt_t sdpOverEvt;
    sdpOverEvt.aclHandle = connHandle;
    blt_prf_sendEvent(connHandle, PRF_EVTID_CLIENT_ALL_SDP_OVER, (u8 *)&sdpOverEvt, sizeof(blc_prf_sdpOverEvt_t));
}

void blc_prf_sendServiceDiscoveryFailEvent(u16 connHandle, int svcId)
{
    blc_prf_sdpFailEvt_t pEvt = {
        .aclHandle = connHandle,
        .svcId     = svcId};
    blt_prf_sendEvent(connHandle, PRF_EVTID_CLIENT_SDP_FAIL, (u8 *)&pEvt, sizeof(blc_prf_sdpFailEvt_t));
}

void blc_prf_sendServiceDiscoveryFoundEvent(u16 connHandle, int svcId, u16 startHdl, u16 endHdl)
{
    blc_prf_sdpFoundEvt_t pEvt = {
        .aclHandle = connHandle,
        .svcId     = svcId,
        .startHdl  = startHdl,
        .endHdl    = endHdl};
    blt_prf_sendEvent(connHandle, PRF_EVTID_CLIENT_SDP_FOUND, (u8 *)&pEvt, sizeof(blc_prf_sdpFoundEvt_t));
}

void blc_prf_sendSingleServiceDiscoveryFinishEvent(u16 connHandle, int svcId)
{
    blc_prf_sdpEndEvt_t pEvt = {
        .aclHandle = connHandle,
        .svcId     = svcId};
    blt_prf_sendEvent(connHandle, PRF_EVTID_CLIENT_SDP_END, (u8 *)&pEvt, sizeof(blc_prf_sdpEndEvt_t));
}
