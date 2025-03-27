/********************************************************************************************************
 * @file    chn_sound.c
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

#if OS_SUP_EN
#include "stack/ble/os_sup/os_sup.h"
#include "stack/ble/os_sup/os_sup_stack.h"
#endif

#if(LL_FEATURE_ENABLE_CHANNEL_SOUNDING)


u8 cs_fae_cmplt_reason = 0;  //todo optimize

ble_sts_t blc_ll_cs_setDefaultSetting(u16 handle, cs_role_t role_enable, u8 ant_sel, s8 max_tx_power);
ble_sts_t blc_ll_cs_setProcedureParam(hci_le_cs_setProcedureParame_cmdParam_t *pParam);
ble_sts_t blc_ll_cs_writeCachedRemoteFAE_table(u16 handle,u8* table);
ble_sts_t blc_ll_cs_writeCachedRemoteSupportedCap(
            hci_le_cs_writeCachedRemoteSupportedCap_cmdParam_t *pCS_param);

static u8 blt_csBitMsk2IdxDecending(u32 bitMsk, u8 len)
{
    for(int i=len; i>=0; i--){
        if( (bitMsk>>i) & BIT(0)){
            return i;
        }
    }
    return (len+1);
}

/**
 * @brief       This function is to convert channel map to channel array.
 * @param[in]   chm: channel map.
 *                  Filtered_channel: channel array
 *                  Filtered_channel_num: length of channel array
 * @return       result - 0:success 1:fail
 */
u8 blt_cs_getEnableChmNum(u8* chm)
{
    u8 enable_num = 0;
    //chm convert to chn map array
    for(unsigned int i=0;i<80;i++)
    {
        if(i==0||i==1||i==23||i==24||i==25||i==77||i==78)
            continue;
        if(chm[i/8]&(1<<(i%8)))
        {
            enable_num ++;
        }
    }
    return enable_num;
}


/**
 * @brief       This function is to convert channel map to channel array.
 * @param[in]   chm: channel map.
 *                  Filtered_channel: channel array
 *                  Filtered_channel_num: length of channel array
 * @return       result - 0:success 1:fail
 */
u8 blt_cs_extractEnableChnMap(u8* chm, u8* Filtered_channel)
{

    u8 chnEnableNum = 0;
    //chm convert to chn map array
    for(unsigned int i=0;i<80;i++)
    {
        if(i==0||i==1||i==23||i==24||i==25||i==77||i==78)
            continue;
        if(chm[i/8]&(1<<(i%8)))
        {
            Filtered_channel[chnEnableNum] = i;
            chnEnableNum++;
        }
    }
    return chnEnableNum;
}


ble_sts_t blt_cs_checkConfig(hci_le_cs_creatConfig_cmdParam_t *pConfig){
    /*If the Host sends this command with a Connection_Handle that does not exist, or the Connection_Handle
     *is not for an ACL the Controller shall return the error code Unknown Connection Identifier (0x02).*/
    if (blt_ll_isAclhdlInvalid(pConfig->Connection_Handle)){
        CS_HCI_LOG("[CHK_CFG] handle invalid:0x%x", pConfig->Connection_Handle);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if((pConfig->Config_ID>3) || (pConfig->Create_Context>1) || (pConfig->Main_Mode>3) || (pConfig->Main_Mode == 0)
            || ((pConfig->Sub_Mode>3) && (pConfig->Sub_Mode!=0xff))|| (pConfig->Sub_Mode==0))
    {
        CS_HCI_LOG("[CHK_CFG] Config_ID abnormal:0x%x,0x%x,0x%x,0x%x", pConfig->Config_ID,pConfig->Create_Context,pConfig->Main_Mode,pConfig->Sub_Mode);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if((pConfig->Main_Mode == 1) && (pConfig->Sub_Mode <=3)){
        CS_HCI_LOG("[CHK_CFG] invalid main mode and submode combination:0x%x,0x%x,0x%x,0x%x",pConfig->Main_Mode,pConfig->Sub_Mode);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if((pConfig->Main_Mode == 2) && (pConfig->Sub_Mode ==2)){
        CS_HCI_LOG("[CHK_CFG] invalid main mode and submode combination:0x%x,0x%x,0x%x,0x%x",pConfig->Main_Mode,pConfig->Sub_Mode);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }
    if((pConfig->Main_Mode == 3) && ((pConfig->Sub_Mode ==1)|| (pConfig->Sub_Mode ==3) )){
        CS_HCI_LOG("[CHK_CFG] invalid main mode and submode combination:0x%x,0x%x,0x%x,0x%x",pConfig->Main_Mode,pConfig->Sub_Mode);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if((pConfig->Main_Mode_Max_Steps == 0) || (pConfig->Main_Mode_Min_Steps == 0) ){
        CS_HCI_LOG("[CHK_CFG] main_mode step abnormal:0x%x,0x%x", pConfig->Main_Mode_Max_Steps,pConfig->Main_Mode_Min_Steps);
//      return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if((pConfig->Main_Mode_Repetition>3) || (pConfig->Mode_0_Steps == 0) || (pConfig->Mode_0_Steps>3)
                    || (pConfig->Role>1) || (pConfig->RTT_Type>6) ||(pConfig->CS_SYNC_PHY == 0)
                    || (pConfig->CS_SYNC_PHY>2) || (pConfig->ChSel>1) || (pConfig->Ch3c_Shape>1)){
        CS_HCI_LOG("[CHK_CFG] Role abnormal:0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",\
                pConfig->Main_Mode_Repetition,pConfig->Mode_0_Steps,pConfig->Role,\
                pConfig->RTT_Type, pConfig->CS_SYNC_PHY,pConfig->Ch3c_Shape,pConfig->ChSel);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if((pConfig->Ch3c_Jump <2 || pConfig->Ch3c_Jump>8) || (pConfig->Companion_Signal_Enable>1)){
        CS_HCI_LOG("[CHK_CFG] Ch3c_Jump abnormal:0x%x,0x%x,0x%x",pConfig->Ch3c_Jump,pConfig->Ch3c_Jump,pConfig->Companion_Signal_Enable);
//      return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }


    if((pConfig->Channel_Map_Repetition == 0)) {
        CS_HCI_LOG("[CHK_CFG] chn num abnormal:%d",pConfig->Channel_Map_Repetition);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    return BLE_SUCCESS;
}


/*
 * The HCI_LE_CS_Read_Local_Supported_Capabilities command allows a Host to
 * read the CS capabilities that are supported by the local Controller.
 * This command may be used along with the local supported features to provide
 * additional details of the supported CS capabilities.
 */
ble_sts_t   blc_hci_le_cs_readLocalSupportedCap(hci_le_cs_readLocalSupportedCap_retParam_t* pRetParam){
    CS_HCI_LOG("[CMD] CS read local cap");

    /*
     * If the Host issues this command when the Channel Sounding (Host Support) feature bit
     *(see [Vol 6] Part B, Section 4.6.33.X) is not set, then the Controller shall return the
     * error code Command Disallowed (0x0C).
     */
    if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)){
        CS_HCI_LOG("[CAP][R_LOC] feature not set");
        return HCI_ERR_CMD_DISALLOWED;
    }

    pRetParam->status                               = BLE_SUCCESS;
    pRetParam->Num_Config_Supported                 = bltCsLocalSupportCap.Num_Config_Supported;
    pRetParam->max_consecutive_procedures_supported = bltCsLocalSupportCap.max_consecutive_procedures_supported;
    pRetParam->Num_Antennas_Supported               = bltCsLocalSupportCap.Num_Antennas_Supported;
    pRetParam->Max_Antenna_Paths_Supported          = bltCsLocalSupportCap.Max_Antenna_Paths_Supported;
    pRetParam->Roles_Supported                      = bltCsLocalSupportCap.Roles_Supported;
    pRetParam->Mode_Types                           = bltCsLocalSupportCap.Mode_Types;
    pRetParam->RTT_Capability                       = bltCsLocalSupportCap.RTT_Capability;
    pRetParam->RTT_AA_Only_N                        = bltCsLocalSupportCap.RTT_AA_Only_N;
    pRetParam->RTT_Sounding_N                       = bltCsLocalSupportCap.RTT_Sounding_N;
    pRetParam->RTT_Random_Payload_N                 = bltCsLocalSupportCap.RTT_Random_Payload_N;
    pRetParam->Optional_NADM_Sounding_Capability    = bltCsLocalSupportCap.Optional_NADM_Sounding_Capability;
    pRetParam->Optional_NADM_Random_Capability      = bltCsLocalSupportCap.Optional_NADM_Random_Capability;
    pRetParam->Optional_CS_SYNC_PHYs_Supported      = bltCsLocalSupportCap.Optional_CS_SYNC_PHYs_Supported;
    pRetParam->Optional_Subfeatures_Supported       = bltCsLocalSupportCap.Optional_Subfeatures_Supported;
    pRetParam->Optional_T_IP1_Times_Supported       = bltCsLocalSupportCap.Optional_T_IP1_Times_Supported;
    pRetParam->Optional_T_IP2_Times_Supported       = bltCsLocalSupportCap.Optional_T_IP2_Times_Supported;
    pRetParam->Optional_T_FCS_Times_Supported       = bltCsLocalSupportCap.Optional_T_FCS_Times_Supported;
    pRetParam->Optional_T_PM_Times_Supported        = bltCsLocalSupportCap.Optional_T_PM_Times_Supported;
    pRetParam->T_SW_Time_Supported                  = bltCsLocalSupportCap.T_SW_Time_Supported;

    CS_HCI_LOG("[CAP][R_LOC] success");
    return pRetParam->status;
}


/*
 * The HCI_LE_CS_Read_Remote_Supported_Capabilities command allows a Host to
 * query the CS capabilities that are supported by the remote Controller.
 * If no Channel Sounding Capability Exchange procedure has been initiated on
 * the ACL connection specified by the Connection_Handle and if no prior
 * HCI_LE_CS_Write_Cached_Remote_Supported_Capabilities command has been
 * issued by the Host, then the Controller shall initiate a Channel Sounding
 * Capability Exchange procedure on the ACL. Otherwise, the Controller may use a
 * cached copy of the capabilities of the remote device
 */
ble_sts_t  blc_hci_le_cs_readRemoteSupportedCap(u16 handle){

    CS_HCI_LOG("[CMD] read remote cap");

    /*
     * If the Host issues this command when the Channel Sounding (Host Support) feature bit
     *(see [Vol 6] Part B, Section 4.6.33.X) is not set, then the Controller shall return the
     * error code Command Disallowed (0x0C).
     */
    if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)){
        CS_HCI_LOG("[CAP][R_REM]  feature not set");
        return HCI_ERR_CMD_DISALLOWED;
    }

    if (blt_ll_isAclhdlInvalid(handle)){
        CS_HCI_LOG("[CAP][R_REM] handle invalid:0x%x", handle);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    st_ll_conn_t* pAcl = (st_ll_conn_t*)blt_ll_getAclConnPtr(handle);
    cs_param_t *pCsParam = &pAcl->csParam;

    if(pCsParam->cs_cap_exchange){
        pCsParam->cs_cap_req  =  PROC_EVT_PENDING;
    }
    else if(pCsParam->cs_cap_req){//other place have triggered req, but rsp not come
        pCsParam->cs_cap_req  |=  PROC_EVT_PENDING;
    }
    else{
        pCsParam->cs_cap_req = PROC_SEND_REQ | PROC_EVT_PENDING;
    }


    if(!pAcl->llcp_flag.bit.ll_feat_exg_flag){
        if(!pAcl->remoteFeatureReq){
            blt_ll_send_feature_req(pAcl);
        }
        CS_HCI_LOG("[CAP][R_REM] feature need exch");
    }

    // reference RemoteSupportedFeatures
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
        blt_os_giveSem_cb();
    }
#endif
    return BLE_SUCCESS;
}


/*
 * The HCI_LE_CS_Write_Cached_Remote_Supported_Capabilities command allows a
 * Host to write the cached copy of the CS capabilities that are supported by
 * the remote Controller for the connection identified by the Connection_Handle
 * parameter
 */
ble_sts_t blc_hci_le_cs_writeCachedRemoteSupportedCap(
            hci_le_cs_writeCachedRemoteSupportedCap_cmdParam_t *pCS_param,
            hci_le_cs_writeCachedRemoteSupportedCap_retParam_t *pRetParam)
{

    CS_HCI_LOG("[CMD] write cache remote cap");
    pRetParam->status = blc_ll_cs_writeCachedRemoteSupportedCap(pCS_param);
    pRetParam->connection_handle = pCS_param->Connection_Handle;

    return pRetParam->status;
}


/*
 * The HCI_LE_CS_Security_Enable command is used by a Host to start or restart
 * the Channel Sounding Security Start procedure in the local Controller for the
 * ACL connection identified by the Connection_Handle parameter.
 */
ble_sts_t blc_hci_le_cs_security_enable(u16 connHandle){
    CS_HCI_LOG("[CMD] enable CS security");
    /*The Central or Peripheral shall not enable the CS Security Start procedure if the Channel Sounding (Host
    Support) feature bit is not set in the Controller.*/
    if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)){
        CS_HCI_LOG("[SEC] Host support not set");
        return HCI_ERR_CMD_DISALLOWED;
    }
    /*If the Host sends this command with a Connection_Handle that does not exist, or the Connection_Handle
     *is not for an ACL the Controller shall return the error code Unknown Connection Identifier (0x02).*/
    if (blt_ll_isAclhdlInvalid(connHandle)){
        CS_HCI_LOG("[SEC] handle invalid:0x%x", connHandle);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }
    st_ll_conn_t* pAcl = (st_ll_conn_t*)blt_ll_getAclConnPtr(connHandle);

    /*If the Host issues this command on a Connection_Handle where the Controller is
     *the Peripheral, then the Controller shall return the error code Command Disallowed (0x0C).*/
    if(pAcl->aclRole == ACL_ROLE_PERIPHERAL)
    {
        CS_HCI_LOG("[SEC] Peripheral role");
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*If the connection identified by the Connection_Handle parameter is not encrypted,
     *then the Controller shall return the error code Command Disallowed (0x0C).*/
    if(0 == pAcl->crypt.enable)
    {
        CS_HCI_LOG("[SEC] enc not complete");
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*If the Host issues this command when a CS procedure measurement is enabled for the specified
    Config_ID in the Controller, then the Controller shall return the error code Command Disallowed (0x0C).*/

    u8 check_cs_start = 0;
    for(int i = 0 ; i<gCsMng.max_num_cofig; i++)
    {
        cs_config_t *pCfg = gGlobal_pCsCfg + i;
        if((pCfg->occupy) && pCfg->cs_procedure_measurement_en  && (connHandle==pCfg->aclHandle))
        {
            check_cs_start = 1 + i;
            break;
        }
    }

    if(check_cs_start){
        CS_HCI_LOG("[SEC] measurement is enabled,cfg_id:0x%x",check_cs_start - 1);
        return HCI_ERR_CMD_DISALLOWED;
    }

    cs_param_t *pCsParam = &pAcl->csParam;
    if( pCsParam->cs_security_exchange){
        pCsParam->cs_security_enable = PROC_EVT_PENDING;
    }
    else{
        if(pCsParam->cs_security_enable){
            pCsParam->cs_security_enable |= PROC_EVT_PENDING;
        }
        else{
            pCsParam->cs_security_enable =  PROC_SEND_REQ | PROC_EVT_PENDING;// start or restart cs security start procedure
        }
    }
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
        blt_os_giveSem_cb();
    }
#endif
    return BLE_SUCCESS;
}


/*
 * The HCI_LE_CS_Set_Default_Settings command is used by a Host to set default CS
 * settings in the local Controller for the connection identified by the
 * Connection_Handle parameter. The default settings specify that all roles are
 * disabled in a Controller and CS_SYNC_Antenna_Selection is set to 0x01
 */
ble_sts_t blc_hci_le_cs_setDefaultSettings(hci_le_cs_setDefaultSetting_cmdParam_t *pSetting,
                                            hci_le_cs_setDefaultSetting_retParam_t * retParam)
{

    CS_HCI_LOG("[CMD] CS Set Default Settings");

    retParam->status = blc_ll_cs_setDefaultSetting(pSetting->Connection_Handle, pSetting->Role_Enable, pSetting->CS_SYNC_Antenna_Selection,
                                                        pSetting->Max_TX_Power);
    retParam->connection_handle = pSetting->Connection_Handle;

    return retParam->status;
}


/*
 * The HCI_LE_CS_Read_Remote_FAE_Table command is used by a Host to read the per-channel
 * mode 0 Frequency Actuation Error table of the remote Controller.
 */
ble_sts_t blc_hci_le_cs_readRemoteFAE_table(u16 connHandle){
    CS_HCI_LOG("[CMD] read remote FAE table");
    /* A Controller shall not allow a local Host to request a peer's FAE table if the
      Channel Sounding (Host Support) feature bit is not set in the Controller.*/

    if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)){
        CS_HCI_LOG("[FAE][R_REM] host not support");
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*If the Host sends this command with a Connection_Handle that does not exist, or the Connection_Handle
     *is not for an ACL the Controller shall return the error code Unknown Connection Identifier (0x02).*/
    if (blt_ll_isAclhdlInvalid(connHandle)){
        CS_HCI_LOG("[FAE][R_REM] handle invalid:0x%x", connHandle);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }
    st_ll_conn_t* pAcl = (st_ll_conn_t*)blt_ll_getAclConnPtr(connHandle);
    cs_param_t *pCsParam = &pAcl->csParam;
    chn_sound_capbilities_t *pcsRemoteSupCap = &pAcl->csRemoteSupCap;

    if((pCsParam->role_enable & CS_INITIATOR_ROLE) == 0){//todo double check,initiator role is allowed to send fae req
        CS_HCI_LOG("[FAE][R_REM] initiator role not enable:0x%x", pCsParam->role_enable);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;//todo
    }

    if(pCsParam->cs_fae_exchange){
        pCsParam->cs_fae_req = PROC_EVT_PENDING;
    }
    else{
        pCsParam->cs_fae_req =  PROC_SEND_REQ | PROC_EVT_PENDING;// LL CS FAE REQ
    }
    /*
     * If the remote Controller does not support a non-zero Frequency Actuation Error in the reflector role, then
        the Controller shall generate the LE_CS_Read_Remote_FAE_Table_Complete event with Status set to
        Unsupported Feature or Parameter Value (0x11).
     */
    if((pcsRemoteSupCap->Optional_Subfeatures_Supported & CS_No_FAE_SUPPORT)){// 1 is zero_FAE, 0 is non_zero FAE
        CS_HCI_LOG("[FAE][R_REM] CS_No_FAE_SUPPORT:0x%x", pcsRemoteSupCap->Optional_Subfeatures_Supported);
        pCsParam->cs_fae_req = PROC_EVT_PENDING;
        cs_fae_cmplt_reason = HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
        blt_os_giveSem_cb();
    }
#endif
    return BLE_SUCCESS;
}


/*
 * The HCI_LE_CS_Write_Cached_Remote_FAE_Table command is used by a Host to write a cached
 * copy of the per-channel mode 0 Frequency Actuation Error table of the remote device in
 * the local Controller.
 */
ble_sts_t blc_hci_le_cs_writeCachedRemoteFAE_table(u16 connHandle, u8* table,
                                                hci_le_cs_writeChchedRemoteFAE_retParam_t *retParam){

    CS_HCI_LOG("[CMD] write cache remote FAE table");
    retParam->status = blc_ll_cs_writeCachedRemoteFAE_table(connHandle, table);
    retParam->connection_handle = connHandle;

    return retParam->status;
}


ble_sts_t blt_cs_checkProcedureParam(hci_le_cs_setProcedureParame_cmdParam_t *pParam,cs_config_t *config){

    if(pParam->PHY != config->CS_SYNC_PHY){//todo need to check again
        CS_HCI_LOG("[CHK_PROC] phy abnormal:0x%x,0x%x",pParam->PHY,config->CS_SYNC_PHY);
        return 0xff;
    }

    if((pParam->Max_Procedure_Len == 0) || (pParam->Min_Procedure_Interval > pParam->Max_Procedure_Interval))
    {
        CS_HCI_LOG("[CHK_PROC] proc len abnormal:0x%x,0x%x,0x%x",pParam->Max_Procedure_Len,pParam->Min_Procedure_Interval,pParam->Max_Procedure_Interval);
        return 0xff;
    }

    u32 min_subeventLen_us = pParam->Min_Subevent_Len[0] | (pParam->Min_Subevent_Len[1] <<8) | (pParam->Min_Subevent_Len[2] <<16);
    u32 max_subeventLen_us = pParam->Max_Subevent_Len[0] | (pParam->Max_Subevent_Len[1] <<8) | (pParam->Max_Subevent_Len[2] <<16);
    if((min_subeventLen_us < 1250) || (min_subeventLen_us > 4000*1000) || (max_subeventLen_us < 1250) || (max_subeventLen_us > 4000*1000) || (max_subeventLen_us < min_subeventLen_us))
    {
        CS_HCI_LOG("[CHK_PROC] sub_evt_len abnormal:0x%x,0x%x",min_subeventLen_us,max_subeventLen_us);
        return 0xff;
    }
    if((pParam->Tone_Antenna_Config_Selection & 0xf8) || (pParam->PHY == 0) || (pParam->PHY > 2) || (pParam->Preferred_Peer_Antenna & 0xfc)){
        CS_HCI_LOG("[CHK_PROC] tone abnormal:0x%x,0x%x,0x%x",pParam->Tone_Antenna_Config_Selection,pParam->PHY,pParam->Preferred_Peer_Antenna);
        return 0xff;
    }

    return BLE_SUCCESS;
}


/*
 * The HCI_LE_CS_Create_Config command is used by a Host to create a new CS configuration
 * with the identifier Config_ID on the connection identified by the Connection_Handle in
 * the local and/or the remote Controller
 */
ble_sts_t blc_hci_le_cs_createConfig(hci_le_cs_creatConfig_cmdParam_t *pConfig){
    CS_HCI_LOG("[CMD] cs create config");
    if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)){// spec no indicate
        CS_HCI_LOG("[CFG][CR8] feature not set");
        return HCI_ERR_CMD_DISALLOWED;
    }

    st_ll_conn_t* pc = (st_ll_conn_t*)blt_ll_getAclConnPtr(pConfig->Connection_Handle);
    cs_param_t *pCsParam = &pc->csParam;
    chn_sound_capbilities_t *pCsRemoteSupCap = &pc->csRemoteSupCap;
    /* This exchange shall only occur after the peer device's CS capabilities are known*/
    if(pCsParam->cs_cap_exchange == 0){
        CS_HCI_LOG("[CFG][CR8] cs cap exchange not complete");
        return HCI_ERR_CMD_DISALLOWED;
    }

    ble_sts_t ret = blt_cs_checkConfig(pConfig);
    if(ret!=BLE_SUCCESS){
        CS_HCI_LOG("[CFG][CR8] parma check abnormal:0x%x", ret);
        return ret;
    }




    u8 cfgIdx = blt_ll_getCsConfigById(pConfig->Connection_Handle,pConfig->Config_ID);
    if( cfgIdx == 0xff){
        cfgIdx = blt_ll_getNewCsConfig();
        if(cfgIdx == 0xff){
            CS_HCI_LOG("[CFG][CR8] create cfg fail:0x%x", cfgIdx);
            return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
        }
        CS_HCI_LOG("[CFG][CR8] create cfg id:0x%x", cfgIdx);
    }
    else{
        CS_HCI_LOG("[CFG][CR8] cfg id is exist:0x%x", cfgIdx);
    }

    cs_config_t *pCsCfg = gGlobal_pCsCfg + cfgIdx;
    chn_sound_capbilities_t *csLocalCap = &bltCsLocalSupportCap;

    if(pCsCfg ==NULL){
        CS_HCI_LOG("[CFG][CR8] cfg null");
        return HCI_ERR_LIMIT_REACHED;
    }

    /*Channels n = 0, 1, 23, 24, 25, 77, and 78 shall be reserved for future use and shall be set to zero. At least 15 channels shall be enabled.
      The most significant bit (bit 79) is reserved for future use.*/
    pCsCfg->Chn_en_num = blt_cs_extractEnableChnMap(pConfig->Channel_Map, pCsCfg->filteredChnArray);

    if(pCsCfg->Chn_en_num <15){
        CS_HCI_LOG("[CFG][CR8] chnNum eroro:0x%x", pCsCfg->Chn_en_num);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if((!(csLocalCap->Mode_Types & BIT(0))) || (!(pCsRemoteSupCap->Mode_Types & BIT(0))))//local & remote not support mode3
    {
        if((pConfig->Main_Mode ==0x03)||  (pConfig->Sub_Mode==0x03))
        {
            CS_HCI_LOG("[CFG][CR8] local or remote not support mode3:0x%x,0x%x",pConfig->Main_Mode,pConfig->Sub_Mode);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }

    if(((pConfig->Role==CS_CONFIG_INITIATOR_ROLE) && (!(csLocalCap->Roles_Supported & CS_INITIATOR_ROLE))) ||
            ( (pConfig->Role==CS_CONFIG_REFLECTOR_ROLE) && (!(csLocalCap->Roles_Supported & CS_REFLECTOR_ROLE)) ))
    {
        CS_HCI_LOG("[CFG][CR8] role support err:0x%x,0x%x",pConfig->Role,csLocalCap->Roles_Supported );
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if((pConfig->CS_SYNC_PHY==0x02) &&  (!(csLocalCap->Optional_CS_SYNC_PHYs_Supported & BIT(1)))){
        CS_HCI_LOG("[CFG][CR8] phy support err:0x%x,0x%x",pConfig->CS_SYNC_PHY,csLocalCap->Optional_CS_SYNC_PHYs_Supported);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }



    pCsCfg->Config_ID               = pConfig->Config_ID;
    pCsCfg->Create_Context          = pConfig->Create_Context;
    pCsCfg->Main_Mode               = pConfig->Main_Mode;//
    pCsCfg->Sub_Mode                = pConfig->Sub_Mode;//
    pCsCfg->Main_Mode_Min_Steps     = pConfig->Main_Mode_Min_Steps;
    pCsCfg->Main_Mode_Max_Steps     = pConfig->Main_Mode_Max_Steps;
    pCsCfg->Main_Mode_Repetition    = pConfig->Main_Mode_Repetition;
    pCsCfg->Mode_0_Steps            = pConfig->Mode_0_Steps;
    pCsCfg->Role                    = pConfig->Role; //
    pCsCfg->RTT_Type                = pConfig->RTT_Type;
    pCsCfg->CS_SYNC_PHY             = pConfig->CS_SYNC_PHY;//

    smemcpy(pCsCfg->Channel_Map,pConfig->Channel_Map,10);

    pCsCfg->Channel_Map_Repetition  = pConfig->Channel_Map_Repetition;
    pCsCfg->ChSel                   = pConfig->ChSel;
    pCsCfg->Ch3c_Shape              = pConfig->Ch3c_Shape;
    pCsCfg->Ch3c_Jump               = pConfig->Ch3c_Jump;
    pCsCfg->Companion_Signal_Enable = pConfig->Companion_Signal_Enable;
    pCsCfg->state                   = 1;//create is 1, remove is 0
    pCsCfg->occupy                  = 1;
    pCsCfg->aclHandle               = pConfig->Connection_Handle;

    pCsCfg->idx                     = cfgIdx;



    u16 tp1 = bltCsLocalSupportCap.Optional_T_IP1_Times_Supported & pc->csRemoteSupCap.Optional_T_IP1_Times_Supported;
    u16 tp2 = bltCsLocalSupportCap.Optional_T_IP2_Times_Supported & pc->csRemoteSupCap.Optional_T_IP2_Times_Supported;
    u16 fcs = bltCsLocalSupportCap.Optional_T_FCS_Times_Supported & pc->csRemoteSupCap.Optional_T_FCS_Times_Supported;
    u16 t_pm = bltCsLocalSupportCap.Optional_T_PM_Times_Supported & pc->csRemoteSupCap.Optional_T_PM_Times_Supported;

    pCsCfg->T_IP1                   = blt_csBitMsk2IdxDecending(tp1, 6);
    pCsCfg->T_IP2                   = blt_csBitMsk2IdxDecending(tp2, 6);
    pCsCfg->T_FCS                   = blt_csBitMsk2IdxDecending(fcs, 8);
    pCsCfg->T_PM                    = blt_csBitMsk2IdxDecending(t_pm, 1);

    if(pCsCfg->Create_Context==1){
        pCsParam->cs_config_pend_idx = pCsCfg->idx;  // Only one CsConfig can be in pending state on the same ACL
        pCsParam->cs_config_req = PROC_SEND_REQ | PROC_EVT_PENDING;
    }
    else{
        pCsParam->cs_config_req = PROC_EVT_PENDING;
    }
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
        blt_os_giveSem_cb();
    }
#endif
    return BLE_SUCCESS;
}

/*
 * The HCI_LE_CS_Remove_Config command is used to remove a CS configuration identified by
 * Config_ID from the local Controller for the connection identified by the Connection_
 * Handle parameter. When the Host issues this command, the local Controller shall initiate
 * a Channel Sounding Configuration procedure to remove the CS configuration from both the
 * local and remote device
 */
ble_sts_t blc_hci_le_cs_removeConfig(u16 connHandle, u8 config_ID){
    CS_HCI_LOG("[CMD] cs remove config");
    /*If the Host sends this command with a Connection_Handle that does not exist, or the Connection_Handle
     *is not for an ACL the Controller shall return the error code Unknown Connection Identifier (0x02).*/
    if (blt_ll_isAclhdlInvalid(connHandle)){
        CS_HCI_LOG("[CFG][RMV] handle invalid:0x%x", connHandle);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    /*If the CS configuration corresponding to Config_ID does not exist, then the Controller shall return the
    error code Invalid HCI Command Parameters (0x12).*/
    u8 cfgIdx = blt_ll_getCsConfigById(connHandle, config_ID);
    if(0xff == cfgIdx){
        CS_HCI_LOG("[CFG][RMV] local hasn't this config:0x%x,0x%x",connHandle,config_ID);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    st_ll_conn_t* pc = (st_ll_conn_t*)blt_ll_getAclConnPtr(connHandle);
    cs_param_t *pCsParam = &pc->csParam;
    cs_config_t *config = gGlobal_pCsCfg + cfgIdx;
    /*If the Host issues this command when one or more CS procedures have been enabled using the
    HCI_LE_CS_Procedure_Enable command, then the Controller shall return the error code Command
    Disallowed (0x0C).*/
    if(config->cs_procedure_en || config->cs_procedure_measurement_en){
        CS_HCI_LOG("[CFG][RMV] CS procedures have been enabled:0x%x,0x%x",config->cs_procedure_en,config->cs_procedure_measurement_en);
        return HCI_ERR_CMD_DISALLOWED;
    }

    config->state = 0;
    config->cs_procedure_para_set_en = 0;
    config->cs_procedure_en = 0;
    config->cs_procedure_measurement_en = 0;
    pCsParam->cs_config_pend_idx = config->idx;
    pCsParam->cs_config_req = PROC_SEND_REQ | PROC_EVT_PENDING;
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
        blt_os_giveSem_cb();
    }
#endif
    return BLE_SUCCESS;
}


/*
 * The HCI_LE_CS_Set_Channel_Classification command is used by a Host to update the channel
 * classification based on its local information. This channel classification persists until
 * overwritten with a subsequent HCI_LE_CS_Set_CS_Channel_Classification command or until
 * the Controller is reset. The Controller may combine the channel classification information
 * provided by the Host along with local channel classification information to send an updated
 * CS channel map to the remote Controller.
 */
ble_sts_t blc_hci_le_cs_setChannelClassification(u8 *chnM){
    CS_HCI_LOG("[CMD] cs set channel classification");
    /*The interval between two successive commands sent shall be at least 1 second. Otherwise, the
      Controller shall return the error code Command Disallowed (0x0C).*/
    if(gCsMng.chn_map_upt_tick && (!clock_time_exceed(gCsMng.chn_map_upt_tick,1000*1000))){
        CS_HCI_LOG("[CHNL] two successive cmds sent < 1s");
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*If the Channel_Classification parameter enables channels that are reserved for future use or enables
      fewer than 15 channels, then the Controller shall return the error code Invalid HCI Command Parameters
      (0x12).*/
    /*Channels n = 0, 1, 23, 24, 25, 77, and 78 shall be reserved for future use and shall be set to zero. At least 15 channels shall be enabled.
      The most significant bit (bit 79) is reserved for future use.*/
    u8 chn_en_num = blt_cs_getEnableChmNum(chnM);

    if(chn_en_num < 15) {
        CS_HCI_LOG("[CHNL] chn en num < 15:%s",hex_to_str(chnM,10));
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    memcpy(gCsMng.chn_map, chnM, 10);

    blt_ll_cs_chnMapUpdateProce();

//  for(int conn_idx=ACL_CONN_IDX_CEN0; conn_idx<LL_MAX_ACL_CONN_NUM; conn_idx++){
//      st_ll_conn_t *pc = (st_ll_conn_t*)&blms[conn_idx];
//      cs_param_t *pCsParam = &pc->csParam;
//      if(pc->connState){
//          pCsParam->cs_chn_map_ind = PROC_EVT_PENDING;
//          //pCsParam->cs_chn_map_instance
//      }
//  }
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
        blt_os_giveSem_cb();
    }
#endif
    gCsMng.chn_map_upt_tick = clock_time()|1;
    return BLE_SUCCESS;
}

/*
 * The HCI_LE_CS_Set_Procedure_Parameters command is used by a Host to set the parameters for
 * the scheduling of one or more CS procedures by the local Controller, with the remote device
 * for the CS configuration identified by Config_ID and the connection identified by the
 * Connection_Handle parameter
 */
ble_sts_t blc_hci_le_cs_setProcedureParam(hci_le_cs_setProcedureParame_cmdParam_t *pParam,
                                        hci_le_cs_setProcedureParam_retParam_t *ret)
{
    CS_HCI_LOG("[CMD] set procedure param");
    ret->status = blc_ll_cs_setProcedureParam(pParam);
    ret->connection_handle = pParam->Connection_Handle;

    return  ret->status;
}

/*
 * The HCI_LE_CS_Procedure_Enable command is used by a Host to enable or disable the scheduling
 * of CS procedures by the local Controller, with the remote device for the connection identified
 * by the Connection_Handle parameter.
 */
ble_sts_t blc_hci_le_cs_procedureEnable(hci_le_cs_enableProcedure_cmdParam_t *pCmd)
{
    CS_HCI_LOG("[CMD] set procedure enable:0x%x",pCmd->Enable);
    /*If the Host sends this command with a Connection_Handle that does not exist, or the Connection_Handle
     *is not for an ACL the Controller shall return the error code Unknown Connection Identifier (0x02).*/
    if (blt_ll_isAclhdlInvalid(pCmd->Connection_Handle)){
        CS_HCI_LOG("[PROC_EN] handle invalid:0x%x", pCmd->Connection_Handle);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    /*If the CS configuration corresponding to Config_ID does not exist, then the Controller shall return the
     error code Invalid HCI Command Parameters (0x12).*/
    u8 cfgIdx = blt_ll_getCsConfigById(pCmd->Connection_Handle, pCmd->Config_ID);
    cs_config_t *config = gGlobal_pCsCfg + cfgIdx;
    if(cfgIdx == 0xff ){
        CS_HCI_LOG("[PROC_EN] cfg id not exist:0x%x,0x%x,0x%x", cfgIdx,pCmd->Connection_Handle,pCmd->Config_ID);
        return HCI_ERR_CMD_DISALLOWED;  //HCI_ERR_CMD_DISALLOWED
    }
    /*If the Host issues this command to enable a CS configuration identified by the Config_ID parameter
     before a corresponding HCI_LE_CS_Set_Procedure_Parameters command has been issued for the
     same Config_ID, then the Controller shall return the error code Command Disallowed (0x0C).*/

    /*If the Host issues this command to enable a CS configuration identified by the Config_ID parameter that
     is already enabled using the HCI_LE_CS_Procedure_Enable command or is disabled using the
     HCI_LE_CS_Remove_Config command, then the Controller shall return the error code Command
     Disallowed (0x0C).*/
    if(pCmd->Enable && ((0 == config->cs_procedure_para_set_en) || config->cs_procedure_en )){
        CS_HCI_LOG("[PROC_EN] cs start abnormal:0x%x,0x%x,0x%x",pCmd->Enable,config->cs_procedure_para_set_en,config->cs_procedure_en);
        return HCI_ERR_CMD_DISALLOWED;
    }

    st_ll_conn_t* pc = (st_ll_conn_t*)blt_ll_getAclConnPtr(pCmd->Connection_Handle);
    cs_param_t *pCsParam = &pc->csParam;

//  if(!(pCsParam->cs_cap_exchange && pCsParam->cs_security_exchange && pCsParam->cs_fae_exchange)){
    if(!(pCsParam->cs_cap_exchange && pCsParam->cs_security_exchange )){
        CS_HCI_LOG("[PROC_EN] proc abnormal: 0x%x,0x%x,0x%x", pCsParam->cs_cap_exchange, pCsParam->cs_security_exchange, pCsParam->cs_fae_exchange);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if(config->Chn_en_num < 15) {
        CS_HCI_LOG("[PROC_EN] the number of channels < 15:%s",hex_to_str(&config->Channel_Map[0],10));
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }



    //todo  config->offset_max & config->offset_min;
    if(pCmd->Enable){
        config->cs_procedure_measurement_en = 0;
        config->cs_procedure_en = 1;
        pCsParam->cs_req = (PROC_SEND_REQ | PROC_EVT_PENDING);
    }
    else if(config->cs_procedure_measurement_en){//terminate indicate
        pCsParam->cs_terminate_ind = (PROC_SEND_IND | PROC_EVT_PENDING);
    }
    pCsParam->cs_pend_idx = config->idx;
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
        blt_os_giveSem_cb();
    }
#endif
    return BLE_SUCCESS;
}
/*This command is used to start a CS test where the Device Under Test (DUT) is placed in the role of either
 *the initiator or reflector. The first mode 0 channel in the list is used as the starting channel for the test. At
 *the beginning of any test, the DUT in the reflector role shall listen on Channel[0] until it receives the first
 *transmission from the initiator. Similarly, with the DUT in the initiator role, the tester will start by listening
 *on Channel[0] and the DUT shall transmit on that channel for the first half of the first CS step. Thereafter,
 *the parameters of this command describe the required transmit and receive behavior for the CS test.
 */
ble_sts_t blc_hci_le_cs_test(hci_le_cs_test_cmdParam_t *pCmd)
{
    (void)pCmd;
    CS_HCI_LOG("[CMD] cs test");






    return BLE_SUCCESS;
}

/*The HCI_LE_CS_Test_End command is used to stop any CS test that is in progress.*/
ble_sts_t blc_hci_le_cs_testEnd(void)
{
    return BLE_SUCCESS;
}

ble_sts_t blc_ll_cs_writeCachedRemoteSupportedCap(
            hci_le_cs_writeCachedRemoteSupportedCap_cmdParam_t *pCS_param)
{
    /*If the Host issues this command when the Channel Sounding (Host Support) feature bit
     *(see [Vol 6] Part B, Section 4.6.33.X) is not set, then the Controller shall return
     *the error code Command Disallowed (0x0C).*/

    if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_SOUNDING_HOST)){
        CS_HCI_LOG("[CAP][W_REM] feature not set");
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*If the Host sends this command with a Connection_Handle that does not exist, or the Connection_Handle
     *is not for an ACL the Controller shall return the error code Unknown Connection Identifier (0x02).*/
    if (blt_ll_isAclhdlInvalid(pCS_param->Connection_Handle)){
        CS_HCI_LOG("[CAP][W_REM] handle invalid:0x%x", pCS_param->Connection_Handle);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    st_ll_conn_t* pAcl = (st_ll_conn_t*)blt_ll_getAclConnPtr(pCS_param->Connection_Handle);
    chn_sound_capbilities_t *pCsRemoteSupCap = &pAcl->csRemoteSupCap;
    cs_param_t *pCs_param = &pAcl->csParam;

    /*If the Host issues this command after a LL_CS_CAPABILITIES_REQ or LL_CS_CAPABILITIES_RSP
     *PDU has been received from the remote Controller, then the Controller shall return the error code
     *Command Disallowed (0x0C).*/
    if(pCs_param->cs_cap_req || pCs_param->cs_cap_exchange)
    {
        CS_HCI_LOG("[CAP][W_REM] has received cap cmd:0x%x,0x%x",pCs_param->cs_cap_req, pCs_param->cs_cap_exchange);
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*If the Host issues this command after a CS configuration has been created in the local
     *Controller, then the Controller shall return the error code Command Disallowed (0x0C).*/

    if(blt_ll_getCsConfigByConnHandle(pCS_param->Connection_Handle) != 0xff)
    {
        CS_HCI_LOG("[CAP][W_REM] cfg has been created:0x%x",pCS_param->Connection_Handle);
        return HCI_ERR_CMD_DISALLOWED;
    }


    smemcpy(pCsRemoteSupCap,&pCS_param->Num_Config_Supported,sizeof(chn_sound_capbilities_t));//todo  to check len
    pCs_param->cs_cap_exchange = 1;

    CS_HCI_LOG("[CAP][W_REM] success");
    return BLE_SUCCESS;
}

ble_sts_t blc_ll_cs_setDefaultSetting(u16 handle, cs_role_t role_enable, u8 ant_sel, s8 max_tx_power)
{
    /*If the Host sends this command with a Connection_Handle that does not exist, or the Connection_Handle
     *is not for an ACL the Controller shall return the error code Unknown Connection Identifier (0x02).*/
    if (blt_ll_isAclhdlInvalid(handle)){
        CS_HCI_LOG("[DFT_SET] handle invalid:0x%x", handle);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    st_ll_conn_t* pc = (st_ll_conn_t*)blt_ll_getAclConnPtr(handle);
    cs_param_t *pCsParam = &pc->csParam;
    /*
     *  If the Host issues this command to disable a Role for which a valid CS configuration is present, then the Controller
     *  shall return the error code Invalid HCI Command Parameters (0x12)
     */

    if(( (!(role_enable & CS_INITIATOR_ROLE)) && blt_ll_getCsConfigByRole(pc->acl_conHandle,CS_CONFIG_INITIATOR_ROLE))
            || ( (!(role_enable & CS_REFLECTOR_ROLE)) && blt_ll_getCsConfigByRole(pc->acl_conHandle,CS_CONFIG_REFLECTOR_ROLE)))
    {
        CS_HCI_LOG("[DFT_SET] cfg is present:0x%x,0x%x",role_enable,pc->acl_conHandle);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    /*
     * If the Role_Enable parameter is used to enable an unsupported role or the CS_SYNC_Antenna_Selection parameter indicates
     * an unsupported antenna identifier, then the Controller shall return the error code Unsupported Feature or Parameter
     * Value (0x11)
     */
    if(role_enable)
    {
        if(!(role_enable & bltCsLocalSupportCap.Roles_Supported))
        {
            CS_HCI_LOG("[DFT_SET] role not support:0x%x,0x%x",role_enable,bltCsLocalSupportCap.Roles_Supported);
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }

    if(((ant_sel>4) && (ant_sel < 0xfe)) || (0 == ant_sel) ){                       //todo need to consider 0xfe & 0xff
//  if((ant_sel>bltCsLocalSupportCap.Num_Antennas_Supported) || (0 == ant_sel) ){
        CS_HCI_LOG("[DFT_SET] ant sel param abnormal:0x%x,0x%x",ant_sel,bltCsLocalSupportCap.Num_Antennas_Supported);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if(max_tx_power > 20){// spec no indicate need return value.
        CS_HCI_LOG("[DFT_SET] max tx power param abnormal:0x%x",max_tx_power);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    pCsParam->role_enable = role_enable;

    //pcl.c  blt_ll_pclGetRfActualTxPwr
    if      (max_tx_power >=   ble_rf_max_tx_pwr)   {  pCsParam->Max_TX_Power = ble_rf_max_tx_pwr;  }//ble_rf_max_tx_pwr:9dbm
    else if (max_tx_power >=   0)                   {  pCsParam->Max_TX_Power = max_tx_power; }
    else if (max_tx_power >=  -4)                   {  pCsParam->Max_TX_Power =  -4;  }
    else if (max_tx_power >=  -8)                   {  pCsParam->Max_TX_Power =  -8;  }
    else if (max_tx_power >= -12)                   {  pCsParam->Max_TX_Power = -12;  }
    else if (max_tx_power >= -18)                   {  pCsParam->Max_TX_Power = -18;  }
    else                                            {  pCsParam->Max_TX_Power = ble_rf_min_tx_pwr;  }//ble_rf_min_tx_pwr:-23dbm


    pCsParam->CS_SYNC_AntSel = ant_sel;

    CS_HCI_LOG("[DFT_SET] success");

    return BLE_SUCCESS;
}


ble_sts_t blc_ll_cs_writeCachedRemoteFAE_table(u16 handle,u8* table){
    /*If the Host sends this command with a Connection_Handle that does not exist, or the Connection_Handle
     *is not for an ACL the Controller shall return the error code Unknown Connection Identifier (0x02).*/
    if (blt_ll_isAclhdlInvalid(handle)){
        CS_HCI_LOG("[FAE][W_REM] handle invalid:0x%x", handle);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }
    st_ll_conn_t* pc = (st_ll_conn_t*)blt_ll_getAclConnPtr(handle);
    cs_param_t *pCsParam = &pc->csParam;
    chn_sound_capbilities_t *pCsRemoteSupCap = &pc->csRemoteSupCap;
    /*If the remote Controller supports a Frequency Actuation Error of zero
     *relative to its mode 0 transmissions in the reflector role, then the
     *Controller shall return the error code Unsupported Feature or Parameter Value (0x11).
     */

    if(pCsRemoteSupCap->Optional_Subfeatures_Supported & CS_No_FAE_SUPPORT){
        CS_HCI_LOG("[FAE][W_REM] Remote CS_No_FAE_SUPPORT:0x%x",pCsRemoteSupCap->Optional_Subfeatures_Supported);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    /*If the Host issues this command after a LL_FAE_RSP PDU has been received from the remote
     *Controller, then the Controller shall return the error code Command Disallowed (0x0C).
     */
    if(pCsParam->cs_fae_exchange){
        CS_HCI_LOG("[FAE][W_REM] LL_FAE_RSP PDU has been received");
        return HCI_ERR_CMD_DISALLOWED;
    }

    /*If the Host issues this command after a CS configuration has been created in the local Controller, then
     *the Controller shall return the error code Command Disallowed (0x0C).
     */

    if(blt_ll_getCsConfigByConnHandle(handle) != 0xff){
        CS_HCI_LOG("[FAE][W_REM] cfg has been created:0x%x",handle);
        return HCI_ERR_CMD_DISALLOWED;
    }

    smemcpy(pCsParam->fae_table,table,72);

    pCsParam->cs_fae_exchange = 1;

    CS_HCI_LOG("[FAE][W_REM] success");
    return BLE_SUCCESS;
}

ble_sts_t blc_ll_cs_setProcedureParam(hci_le_cs_setProcedureParame_cmdParam_t *pParam)
{
    /*If the Host sends this command with a Connection_Handle that does not exist, or the Connection_Handle
     *is not for an ACL the Controller shall return the error code Unknown Connection Identifier (0x02).*/
    if (blt_ll_isAclhdlInvalid(pParam->Connection_Handle)){
        CS_HCI_LOG("[PROC_PARAM] handle invalid:0x%x", pParam->Connection_Handle);
        return HCI_ERR_UNKNOWN_CONN_ID;
    }


    /*If the CS configuration corresponding to Config_ID does not exist, is already enabled using the
    HCI_LE_CS_Procedure_Enable command, or is disabled using the HCI_LE_CS_Remove_Config
    command, then the Controller shall return the error code Invalid HCI Command Parameters (0x12).*/
    u8 cfgIdx = blt_ll_getCsConfigById(pParam->Connection_Handle, pParam->Config_ID);
    cs_config_t *config = gGlobal_pCsCfg + cfgIdx;
    if((cfgIdx == 0xff) || config->cs_procedure_en){
        CS_HCI_LOG("[PROC_PARAM] config id not exist:0x%x", cfgIdx);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    /*If the Host issues this command with parameters that exceed the CS capabilities or any coexistence
      constraints, then the Controller shall return the error code Connection Rejected Due to Limited Resources
      (0x0D).*/
    if(blt_cs_checkProcedureParam(pParam,config)){
        CS_HCI_LOG("[PROC_PARAM] pParam err");
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }
    /*If the Host issues this command when a CS procedure measurement is enabled for the specified
    Config_ID in the Controller, then the Controller shall return the error code Command Disallowed (0x0C).*/
    if(config->cs_procedure_measurement_en){
        CS_HCI_LOG("[PROC_PARAM] measurement is enabled");
        return HCI_ERR_CMD_DISALLOWED;
    }

    if(config->Chn_en_num < 15) {
        CS_HCI_LOG("[PROC_PARAM] the number of channels < 15:%s",hex_to_str(&config->Channel_Map[0],10));
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    st_ll_conn_t * pAclConn =  (st_ll_conn_t*)(u32)&blms[pParam->Connection_Handle & CONN_IDX_MASK];
    config->Max_Procedure_Len               = pParam->Max_Procedure_Len;
    config->Max_Procedure_Interval          = pParam->Max_Procedure_Interval;
    config->Min_Procedure_Interval          = pParam->Min_Procedure_Interval;
    config->procMaxCount                = pParam->Max_Procedure_Count;
    config->Tone_Antenna_Config_Selection   = pParam->Tone_Antenna_Config_Selection;
    config->PHY                             = pParam->PHY;
    config->Tx_Pwr_Delta                    = pParam->Tx_Pwr_Delta;
    config->Preferred_Peer_Ant              = pParam->Preferred_Peer_Antenna;

    config->Min_Subevent_Len                = pParam->Min_Subevent_Len[0] | (pParam->Min_Subevent_Len[1]<<8) | (pParam->Min_Subevent_Len[2]<<16);
    config->Max_Subevent_Len                = pParam->Max_Subevent_Len[0] | (pParam->Max_Subevent_Len[1]<<8) | (pParam->Max_Subevent_Len[2]<<16);

    u32 subevent_len_us   = (config->Max_Subevent_Len + config->Min_Subevent_Len)/2;

//  if(subevent_len_us>3800){
//      subevent_len_us = 3800;
//  }
    config->Subevent_Len                    =  min2(subevent_len_us, pAclConn->conn_intvl_n_1m25*1250/3 * 2);



    config->subEvtIntvl_625us               = (config->Subevent_Len + TLK_T_MES )/625 + 5 ;  // 5 bSlot for inserting ACL task
    config->Subevents_Per_Event             = (pAclConn->conn_intvl_n_1m25*1250/3 * 2)/(config->subEvtIntvl_625us*625) -1 ;

    config->Event_Interval                  = (config->subEvtIntvl_625us * config->Subevents_Per_Event )/ pAclConn->bSlot_interval + 1;

    if(config->procMaxCount != 1){
        config->Procedure_Interval              = (config->Max_Procedure_Interval + config->Min_Procedure_Interval)/2;
    }
    else{
        config->Procedure_Interval = 0;
    }

    config->aci                             = pParam->Tone_Antenna_Config_Selection;//todo by biao
    config->Preferred_Peer_Ant              = BIT(0);//todo by biao
    config->cs_procedure_para_set_en = 1;
    CS_HCI_LOG("[PROC_PARAM] success");
    return BLE_SUCCESS;
}


#endif
