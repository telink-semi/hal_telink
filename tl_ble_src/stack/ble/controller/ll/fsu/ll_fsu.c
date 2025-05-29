/********************************************************************************************************
 * @file    ll_fsu.c
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
#include "stack/ble/controller/ble_controller.h"


#if (LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)

//extern blt_ll_ext_feature_set_t LL_LOCAL_EXT_FEATURE_SET;  //todo, add it after lijing process extended feature function

_attribute_ble_data_retention_
volatile u16 gFsuValidFsVal[3][5] = {
    {150, 150, FSU_ACL_MCES_DEFAULT, 150, 150}, //phy 1M, ACL_CP, ACL_PC, ACL_MCES, CIS_IFS, CIS_MSS
    {150, 150, FSU_ACL_MCES_DEFAULT, 150, 150}, //phy 2M, ACL_CP, ACL_PC, ACL_MCES, CIS_IFS, CIS_MSS
    {150, 150, FSU_ACL_MCES_DEFAULT, 150, 150}, //phy coded, ACL_CP, ACL_PC, ACL_MCES, CIS_IFS, CIS_MSS
};
_attribute_ble_data_retention_
volatile u16 gFsuPreFsVal[3][5] = {
    {150, 150, FSU_ACL_MCES_DEFAULT, 150, 150}, //phy 1M, ACL_CP, ACL_PC, ACL_MCES, CIS_IFS, CIS_MSS
    {150, 150, FSU_ACL_MCES_DEFAULT, 150, 150}, //phy 2M, ACL_CP, ACL_PC, ACL_MCES, CIS_IFS, CIS_MSS
    {150, 150, FSU_ACL_MCES_DEFAULT, 150, 150}, //phy coded, ACL_CP, ACL_PC, ACL_MCES, CIS_IFS, CIS_MSS
};

/* ext feature set mask support FSU */
//todo, add it after lijing process extended feature function
//#define LL_FEATURE_LOCAL_MASK_SUPPORTED_FSU      (LL_LOCAL_EXT_FEATURE_SET.extFeatSet[0][0] & LL_FEATURE_MASK_FRAME_SPACE_UPDATE)

/*
 *  Callback used by POWER_CONTROL
 */
_attribute_ble_data_retention_ ll_fsu_ctrl_handler_t ll_fsu_ctrl_handler = NULL;

static ble_sts_t blt_ll_fsu_rsp_handle(u16 connHandle, u8* pLlCtrlPkt);
static ble_sts_t blt_ll_fsu_req_handle(u16 connHandle, u8* pLlCtrlPkt);
static ble_sts_t blt_ll_fsuControlPduProc(st_ll_conn_t *pAclConn, u8 opcode, u8 *pLlCtrlPkt);

void blc_ll_initFrameSpaceUpdate_feature(void)
{
    //todo, add it after lijing process extended feature function
    //LL_LOCAL_EXT_FEATURE_SET.extFeatSet[0][0] |= LL_FEATURE_MASK_FRAME_SPACE_UPDATE;

    ll_fsu_ctrl_handler = blt_ll_fsuControlPduProc;

    blc_ll_init2MPhyCodedPhy_feature();               //need 2M/Coded PHY feature

    blmsParam.fsu_en = 1;
}

static bool fsuValueCheck(u8 phyMask, u16 spacingType, u16 fs_max)
{
    u8 phyIdx = 0, spacingTypeIdx = 0;

    for(phyIdx=0; phyIdx<3; phyIdx++){
        if(phyMask & BIT(phyIdx)){
            for(spacingTypeIdx=0; spacingTypeIdx < 5; spacingTypeIdx++){
                if(spacingType & BIT(spacingTypeIdx)){
                    if(fs_max < gFsuPreFsVal[phyIdx][spacingTypeIdx]) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

static _attribute_ram_code_ void configValidFsValue(u8 phyMask, u16 spacingType, u16 fs_valid){

    u32 r = irq_disable();
    //1. store the FS value to previous variable.
    smemcpy(gFsuPreFsVal, gFsuValidFsVal, sizeof(gFsuValidFsVal));

    u8 phyIdx = 0;
    u8 spacingTypeIdx = 0;

    for(phyIdx=0; phyIdx<3; phyIdx++){
        if(phyMask&BIT(phyIdx)){

            for(spacingTypeIdx=0; spacingTypeIdx < 5; spacingTypeIdx++){
                if(spacingType & BIT(spacingTypeIdx)){
                    gFsuValidFsVal[phyIdx][spacingTypeIdx] = fs_valid;
                }
            }
        }
    }

    irq_restore(r);
}

_attribute_ram_code_ bool blt_ll_fsu_isFsValChanged(u8 phyMask, u16 spacingType)
{
    u8 phyIdx = 0;
    u8 spacingTypeIdx = 0;

    for (phyIdx = 0; phyIdx < 3; phyIdx++) {
        if (phyMask & BIT(phyIdx)){
            for (spacingTypeIdx = 0; spacingTypeIdx < 5; spacingTypeIdx++){
                if (spacingType & BIT(spacingTypeIdx)) {
                    if (gFsuValidFsVal[phyIdx][spacingTypeIdx] != gFsuPreFsVal[phyIdx][spacingTypeIdx]) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

fsu_cmplet_evt_t fsuCmpletEvt;

ble_sts_t  blt_ll_fsuControlPduProc(st_ll_conn_t *pAclConn, u8 opcode, u8 *pLlCtrlPkt)
{
    // Feature available check
    if (opcode == LL_FRAME_SPACE_REQ) {
        //tlkapi_printf(0, "Rcvd:LL_FRAME_SPACE_REQ");
        ble_sts_t status = blt_ll_fsu_req_handle(pAclConn->acl_conHandle, pLlCtrlPkt);

        if(status != BLE_SUCCESS && status != HCI_ERR_LMP_ERR_TRANSACTION_COLLISION) {
            pAclConn->fsu_param.fsu_error_code = status;
            pAclConn->fsu_param.fsu_procedure_status = FSU_LL_REJ_CMD_SEND;
            pAclConn->fsu_param.initiator = FSU_INITIATOR_PEER;
            //pAclConn->fsu_param.fsu_complete_evt = 1;
            //tlkapi_printf(0, "FSU_INITIATOR_PEER, blt_ll_fsu_req_handle err:0X%X, evt", status);
        }

        return status;
    } else if (opcode == LL_FRAME_SPACE_RSP) {
        //tlkapi_printf(0, "Rcvd:LL_FRAME_SPACE_RSP");
        ble_sts_t status = blt_ll_fsu_rsp_handle(pAclConn->acl_conHandle, pLlCtrlPkt);

        if(status != BLE_SUCCESS) {
            pAclConn->fsu_param.fsu_error_code = status;
            pAclConn->fsu_param.fsu_complete_evt = 1;
            //tlkapi_printf(0, "blt_ll_fsu_rsp_handle, err:0X%X", status);
        }

        return status;
    } else if (opcode == LL_REJECT_IND_EXT) {
        rf_packet_ll_reject_ext_ind_t *pRejectExtInd = (rf_packet_ll_reject_ext_ind_t *)pLlCtrlPkt;

        if (pRejectExtInd->rejectOpcode == LL_FRAME_SPACE_REQ) {
            pAclConn->ll_rsp_timeout_tick = 0;

            //tlkapi_printf(0, "Rcvd:LL_REJECT_IND_EXT(LL_FRAME_SPACE_REQ): errCode %d", pRejectExtInd->errCode);

            //if(pAclConn->fsu_param.fsu_procedure_status == FSU_REQ_CMD_SEND)
            {
                pAclConn->fsu_param.fsu_error_code = pRejectExtInd->errCode;

                if(pRejectExtInd->errCode == HCI_ERR_UNSUPPORTED_REMOTE_FEATURE) {
                    pAclConn->fsu_param.fsu_complete_evt = 1;
                    pAclConn->fsu_param.fsu_procedure_status = FSU_LL_REJ_CMD_REV_UNSUP_FEAT;
                    //tlkapi_printf(0, "FSU_LL_REJ_CMD_REV_UNSUP_FEAT, evt=1");
                }
                else if (pRejectExtInd->errCode == HCI_ERR_LMP_ERR_TRANSACTION_COLLISION) {
                    if(pAclConn->fsu_param.fsu_procedure_status == FSU_PROC_COLLISION_DETECTED) {
                        /* detect collision for LL_CIS_REQ, wait for next LL_CIS_IND finish then event to host */
                        pAclConn->fsu_param.fsu_complete_evt = 1;
                        pAclConn->fsu_param.fsu_procedure_status = FSU_LL_REJ_CMD_REV_COLLSION;
                        //tlkapi_printf(0, "detect collision for LL_CIS_REQ, wait for next LL_CIS_IND finish then event to host");
                    } else {
                        pAclConn->fsu_param.fsu_complete_evt = 1;
                        pAclConn->fsu_param.fsu_procedure_status = FSU_LL_REJ_CMD_REV_COLLSION;
                        //tlkapi_printf(0, "FSU_LL_REJ_CMD_REV_COLLSION, evt=1");
                    }

                    fsuCmpletEvt.fsu_cmplet_bk_errCode      = pAclConn->fsu_param.fsu_error_code;
                    fsuCmpletEvt.fsu_cmplet_bk_connHandle   = pAclConn->acl_conHandle;
                    fsuCmpletEvt.fsu_cmplet_bk_initiator    = FSU_INITIATOR_LOCAL_HOST;//pAclConn->fsu_param.initiator;
                    fsuCmpletEvt.fsu_cmplet_bk_fs_valid     = pAclConn->fsu_param.fs_valid;
                    fsuCmpletEvt.fsu_cmplet_bk_phyMask      = pAclConn->fsu_param.phyMask;
                    fsuCmpletEvt.fsu_cmplet_bk_spacingType  = pAclConn->fsu_param.spacingType;
                    fsuCmpletEvt.fsu_cmplet_bk_valid        = 0x01;
                    pAclConn->fsu_param.fsu_complete_evt    = 0;
                }
                else {
                    pAclConn->fsu_param.fsu_complete_evt = 1;
                    pAclConn->fsu_param.fsu_procedure_status = FSU_LL_REJ_CMD_REV_OTHERS;
                    //tlkapi_printf(0, "FSU_LL_REJ_CMD_REV_OTHERS, evt=1");
                }
            }
        }
    }

    return BLE_SUCCESS;
}

//HCI setting
ble_sts_t blc_ll_frameSpaceUpdate(u16 connHandle, u16 fs_min, u16 fs_max, u8 phyMask, u16 spacingType)
{
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if(fs_max < fs_min){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if( (fs_min > 0x2710) || (fs_max > 0x2710) ){ //0x2710 = 10000, unit 1us, and indicate 10ms
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if ( !(phyMask&BIT_RNG(0,2)) || !(spacingType&BIT_RNG(0,4)) ){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if(fs_min < SUPPORT_MIN_FS_US){
        fs_min = SUPPORT_MIN_FS_US; //our device support the min value
    }

    st_ll_conn_t *pc = blt_ll_getAclConnPtr(connHandle);

    u16 minDataUs = pdu_27b_tifs_27b_us[pc->connPhyCtrl.conn_cur_phy - 1][pc->crypt.enable];

    if(fs_max > (pc->conn_intvl_tick/SYSTEM_TIMER_TICK_1US -minDataUs) ){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    pc->fsu_param.fs_max = fs_max;
    pc->fsu_param.fs_min = fs_min;
    pc->fsu_param.phyMask = phyMask;
    pc->fsu_param.spacingType = spacingType;

    pc->fsu_param.fsu_pending = FSU_REQ_PENDING;

    if(pc->aclRole == ACL_ROLE_CENTRAL){
        pc->fsu_param.fsu_procedure_collision = FSU_PROCEDURE_START;
    }

    pc->fsu_param.initiator = FSU_INITIATOR_LOCAL_HOST; //TODO: not process FSU_INITIATOR_LOCAL_CONTROLLER
    //tlkapi_printf(0, "FSU_INITIATOR_LOCAL_HOST");
    
    return BLE_SUCCESS;
}
ble_sts_t blt_ll_fsu_req_handle(u16 connHandle, u8* pLlCtrlPkt)
{
    st_ll_conn_t *pc = blt_ll_getAclConnPtr(connHandle);

    if (!blt_ll_isEncryptionBusy(connHandle)) {
        u8 dat[8] = {0x03, 0x06, LL_FRAME_SPACE_RSP};

        u16 fs_min      = pLlCtrlPkt[3] | (pLlCtrlPkt[4] << 8);
        u16 fs_max      = pLlCtrlPkt[5] | (pLlCtrlPkt[6] << 8);
        u8  phyMask     = pLlCtrlPkt[7];
        u16 spacingType = pLlCtrlPkt[8] | (pLlCtrlPkt[9] << 8);

        spacingType = (spacingType & BIT_RNG(0, 4));
        phyMask = (phyMask & BIT_RNG(0, 2));

        if(pc->fsu_param.fsu_procedure_collision){
            return HCI_ERR_LMP_ERR_TRANSACTION_COLLISION;
        }

        if (fs_min < SUPPORT_MIN_FS_US || fs_max > SUPPORT_MAX_FS_US || fs_max < fs_min) {
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;//HCI_ERR_INVALID_LMP_PARAMS;
        }



        /*
            If either the PHYS or the Spacing_Types field of an LL_FRAME_SPACE_REQ PDU
            is set to 0, then the responding device shall reject the request by sending an
            LL_REJECT_EXT_IND PDU with the error code set to Invalid LL Parameters (0x1E).
        */
        if (spacingType == 0 || phyMask == 0) {
            return HCI_ERR_INVALID_LMP_PARAMS;
        }

        u16 minDataUs = pdu_27b_tifs_27b_us[pc->connPhyCtrl.conn_cur_phy - 1][pc->crypt.enable];
        if(fs_max > (pc->conn_intvl_tick/SYSTEM_TIMER_TICK_1US -minDataUs) ){
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
        /*
            If the resulting frame space value causes connIntervalRequired to exceed the
            connection interval and the change is being done on the existing ACL connection
            on the PHY in use, then the responding device shall reject the request by sending
            an LL_REJECT_EXT_IND PDU with the error code set to Unsupported Feature or
            Parameter Value (0x11)
        */
        if (0) { //TODO: check pc->conn_intvl_tick)
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
        /* 
            FS_Max shall be greater than or equal to the frame space value in use. If FS_Min
            and FS_Max in the LL_FRAME_SPACE_REQ PDU are both less than the frame space
            value in use for any of the selected frame space types and PHYs, then the responding
            device may reject the request by sending an LL_REJECT_EXT_IND PDU with the error
            code set to Unsupported Feature or Parameter Value (0x11). 
        */
        if (fs_max > SUPPORT_MAX_FS_US || fsuValueCheck(phyMask, spacingType, fs_max) == false) {
            //tlkapi_printf(0, "fs_max check failed: %d, 0x%x, 0x%x", fs_max, phyMask, spacingType);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }

        /*
            The responding device shall set the FS field of the LL_FRAME_SPACE_RSP PDU to
            a value between the FS_Min and FS_Max of the LL_FRAME_SPACE_REQ PDU, and
            should set it to the lowest value the responding device supports within that range.
        */
        if (fs_min >= SUPPORT_MIN_FS_US) {
            pc->fsu_param.fs_valid = fs_min;
        } else {
            pc->fsu_param.fs_valid = SUPPORT_MIN_FS_US;
        }
        
        pc->fsu_param.fs_min = fs_min;
        pc->fsu_param.fs_max = fs_max;
        pc->fsu_param.phyMask     = phyMask & BIT_RNG(0,2); //BIT_RNG(0,2) indicate we support all phy.
        pc->fsu_param.spacingType = spacingType & BIT_RNG(0, 4);//BIT_RNG(0, 4) indicate we support all spacing type.

        dat[3]  = pc->fsu_param.fs_valid;
        dat[4]  = pc->fsu_param.fs_valid>>8;

        dat[5]  = pc->fsu_param.phyMask;

        dat[6]  = pc->fsu_param.spacingType;
        dat[7]  = pc->fsu_param.spacingType>>8;

        pc->fsu_param.fsu_procedure_collision = FSU_PROCEDURE_START;

        if ( !blt_llmsPushLlCtrlPkt(connHandle, LL_FRAME_SPACE_RSP, dat)) {
            pc->fsu_param.fsu_pending = FSU_RSP_PENDING;
        }else{
            configValidFsValue(phyMask, spacingType, pc->fsu_param.fs_valid);
            pc->fsu_param.fsu_pending = 0;
            pc->fsu_param.fsu_complete_evt = 1;
            pc->fsu_param.initiator = FSU_INITIATOR_PEER;
            pc->fsu_param.fsu_procedure_collision = FSU_PROCEDURE_CMPLET;
        }

        pc->fsu_param.fsu_procedure_status = FSU_REQ_CMD_REV;

        return BLE_SUCCESS;
    }


    return LL_ERR_ENCRYPTION_BUSY;
}

ble_sts_t blt_ll_fsu_rsp_handle(u16 connHandle, u8* pLlCtrlPkt){
    u8            idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t *pc  = (st_ll_conn_t *)&blms[idx];

    u16 fs_valid = pLlCtrlPkt[3] | (pLlCtrlPkt[4] << 8);
    u8  phyMask = pLlCtrlPkt[5];

    u16 spacingType = pLlCtrlPkt[6] | (pLlCtrlPkt[7] << 8);
    if(fs_valid < SUPPORT_MIN_FS_US || fs_valid > SUPPORT_MAX_FS_US){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    /*
        All bits set to 0 in the PHYS and Spacing_Types fields of an LL_FRAME_SPACE_REQ
        PDU shall be set to 0 in the LL_FRAME_SPACE_RSP PDU sent in response. If a bit is
        set to 1 in the PHYS field or the Spacing_Types field of the LL_FRAME_SPACE_RSP
        PDU and the corresponding bit(s) are not set in the LL_FRAME_SPACE_REQ PDU,
        then this is considered invalid behavior
    */
    if((pc->fsu_param.phyMask & phyMask) != phyMask || (pc->fsu_param.spacingType & spacingType) != spacingType){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS; //TODO: do nothing seems is enough
    }
    /*  If all the frame space values for a spacing type will remain unchanged, then the
    corresponding field shall be set to 0.*/
    if (spacingType != 0) {
        pc->fsu_param.fs_valid = fs_valid;
        pc->fsu_param.phyMask = phyMask;
        pc->fsu_param.spacingType = spacingType;
        configValidFsValue(phyMask, spacingType, fs_valid);
    }


    pc->fsu_param.fsu_procedure_collision = FSU_PROCEDURE_CMPLET;
    pc->fsu_param.fsu_procedure_status = FSU_PROCEDURE_COMPLETE;
    pc->fsu_param.fsu_complete_evt = 1;
    pc->fsu_param.fsu_error_code = BLE_SUCCESS;

    return BLE_SUCCESS;
}





void blt_ll_fsu_mainloop_proc(u16 connHandle){

    u8            idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t *pc  = (st_ll_conn_t *)&blms[idx];

    if(pc->fsu_param.fsu_pending == FSU_REQ_PENDING){
        if (!blt_ll_isEncryptionBusy(connHandle)){

            u8 dat[10] = {0x03, 0x08, LL_FRAME_SPACE_REQ};
            dat[3]  = pc->fsu_param.fs_min;
            dat[4]  = pc->fsu_param.fs_min>>8;

            dat[5]  = pc->fsu_param.fs_max;
            dat[6]  = pc->fsu_param.fs_max >> 8;

            dat[7]  = pc->fsu_param.phyMask;

            dat[8]  = pc->fsu_param.spacingType;
            dat[9]  = pc->fsu_param.spacingType >> 8;

            if (blt_llmsPushLlCtrlPkt(connHandle, LL_FRAME_SPACE_REQ, dat)) {
                pc->fsu_param.fsu_pending = 0;
                pc->fsu_param.fsu_procedure_status = FSU_REQ_CMD_SEND;

                if(pc->aclRole == ACL_ROLE_PERIPHERAL){
                    fsuCmpletEvt.fsu_cmplet_bk_valid = 0;
                }
            }
        }
    }
    else if(pc->fsu_param.fsu_pending == FSU_RSP_PENDING){
        //blt_ll_fsu_req_handle(connHandle);
        u8 dat[8] = {0x03, 0x06, LL_FRAME_SPACE_RSP}; //type, rf len, opcode
        dat[3]  = pc->fsu_param.fs_valid;
        dat[4]  = pc->fsu_param.fs_valid>>8;

        dat[5]  = pc->fsu_param.phyMask;

        dat[6]  = pc->fsu_param.spacingType;
        dat[7]  = pc->fsu_param.spacingType>>8;


        if ( blt_llmsPushLlCtrlPkt(connHandle, LL_FRAME_SPACE_RSP, dat)) {
            pc->fsu_param.fsu_pending = 0;
            pc->fsu_param.fsu_procedure_status = FSU_RSP_CMD_SEND;
            pc->fsu_param.initiator = FSU_INITIATOR_PEER;
            pc->fsu_param.fsu_complete_evt = 1;
            pc->fsu_param.fsu_error_code = BLE_SUCCESS;
            pc->fsu_param.fsu_procedure_collision = FSU_PROCEDURE_CMPLET;


            configValidFsValue(pc->fsu_param.phyMask, pc->fsu_param.spacingType, pc->fsu_param.fs_valid);
        }
    }

    if(fsuCmpletEvt.fsu_cmplet_bk_valid){
        fsuCmpletEvt.fsu_cmplet_bk_valid = 0;

        pc->fsu_param.fsu_procedure_status = 0;

        if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_FRAME_SPACE_UPDATE_COMPLETE) {
            hci_le_frameSpaceUpdateComplete_evt(fsuCmpletEvt.fsu_cmplet_bk_errCode, fsuCmpletEvt.fsu_cmplet_bk_connHandle, \
                                                fsuCmpletEvt.fsu_cmplet_bk_initiator, fsuCmpletEvt.fsu_cmplet_bk_fs_valid, \
                                                fsuCmpletEvt.fsu_cmplet_bk_phyMask, fsuCmpletEvt.fsu_cmplet_bk_spacingType);
        }
    }
    /* FSU complete event */
    if (pc->fsu_param.fsu_complete_evt) {
        pc->fsu_param.fsu_complete_evt = 0;
        pc->fsu_param.fsu_procedure_status = 0;

        if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_FRAME_SPACE_UPDATE_COMPLETE) {
            hci_le_frameSpaceUpdateComplete_evt(pc->fsu_param.fsu_error_code, connHandle, pc->fsu_param.initiator, pc->fsu_param.fs_valid, pc->fsu_param.phyMask, pc->fsu_param.spacingType);
        }
    }
}


_attribute_ram_code_ void blc_ll_fsu_reset(u16 connHandle){

    st_ll_conn_t *pc = blt_ll_getAclConnPtr(connHandle);

    //set the two variable group as default value.
    u8 phyIdx = 0;
    u8 ifs_idx = 0;
    for(phyIdx=0; phyIdx<3; phyIdx++){
        for(ifs_idx=0; ifs_idx<5; ifs_idx++){
            gFsuValidFsVal[phyIdx][ifs_idx] = 150;
            gFsuPreFsVal[phyIdx][ifs_idx] = 150;
        }
    }

    for(phyIdx=0; phyIdx <3; phyIdx++){
        gFsuValidFsVal[phyIdx][2] = FSU_ACL_MCES_DEFAULT;
        gFsuPreFsVal[phyIdx][2]   = FSU_ACL_MCES_DEFAULT;
    }


    pc->fsu_param.fs_valid = 150;
    pc->fsu_param.fsu_pending = 0;
    pc->fsu_param.spacingType = 0;
    pc->fsu_param.phyMask = 0;
    pc->fsu_param.fsu_procedure_status = 0;
    pc->fsu_param.fsu_error_code = 0;
    pc->fsu_param.initiator = 0;
    pc->fsu_param.fsu_complete_evt = 0;
    pc->fsu_param.fsu_procedure_collision = FSU_PROCEDURE_CMPLET;
}

#endif //LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE
