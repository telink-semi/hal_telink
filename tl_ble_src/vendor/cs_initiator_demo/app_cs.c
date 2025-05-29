/********************************************************************************************************
 * @file    app_cs.c
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
#include "algorithm/hadm/gcc10/cs_cal.h"
#include "app_cs.h"
#include "app_buffer.h"
#include "app_parse_char.h"

#ifndef isinf
#define isinf(d) (isnan((d - d)) && !isnan(d))
#endif

#ifndef isnan
#define isnan(d) (d != d)
#endif

typedef enum
{
    INITIATOR_ROLE = 0,
    REFLECTOR_ROLE = 1,
} app_ranging_role_t;

app_cs_config_t  app_cs_config[APP_CS_CONFIG_NUM] = {0};
cs_app_control_t cs_app_ctrl;


#if (CS_DISTANCE_FILTER)
app_filter_ctrl filter_ctrl[MAX_DISTANCE_CNT_SUPPORT] = {0};
#endif

/**
 * @brief      Add CS control block by ACL connection handle
 * 
 * This function searches for an available CS control block  
 * and assigns the provided connection handle to it. If a suitable block is found, 
 * a pointer to this block is returned; otherwise, NULL is returned.
 * 
 * @param[in]  connhandle  The ACL connection handle to be added.
 * @return     Pointer to the CS control block if available, otherwise NULL.
 */
cs_control_t *user_addCsCtrlByHadle(u16 connhandle)
{
    cs_control_t *pCsCtrl = NULL;
    for (int i = 0; i < CS_MAX_NUM; i++) {
        if (cs_app_ctrl.cs_ctrl[i].connhandle == 0) {
            cs_app_ctrl.cs_ctrl[i].connhandle = connhandle;
            pCsCtrl                           = &cs_app_ctrl.cs_ctrl[i];
            break;
        }
    }
    return pCsCtrl;
}

/**
 * @brief      Clear CS procedure control block by acl connect handle
 * @param[in]  connhandle ACL connect handle
 * @return     0x01 success 0x00 failed
 */
int user_clrCsCtrlByHadle(u16 connhandle)
{
    int ret = 0;
    for (int i = 0; i < CS_MAX_NUM; i++) {
        if (cs_app_ctrl.cs_ctrl[i].connhandle == connhandle) {
            memset(&cs_app_ctrl.cs_ctrl[i], 0, sizeof(cs_control_t));
            ret = 1;
            break;
        }
    }
    return ret;
}

/**
 * @brief      Get index of CS procedure control block by acl connect handle
 * @param[in]  connhandle ACL connect handle
 * @return     0x00 failed
 *             else : index
 */
cs_control_t *user_getCsCtrlByHadle(u16 connhandle)
{
    cs_control_t *pCsCtrl = NULL;

    for (int i = 0; i < CS_MAX_NUM; i++) {
        if (cs_app_ctrl.cs_ctrl[i].connhandle == connhandle) {
            pCsCtrl = &cs_app_ctrl.cs_ctrl[i];
            break;
        }
    }
    return pCsCtrl;
}

/**
 * @brief      Set procedure control block start status
 * @param[in]  index: index of procedure control block
 * @param[in]  status: procedure start status
 * @return     None
 */
void user_setCsProcStartStatus(cs_control_t *pCsCtrlBlock, eCsProcStatus status)
{
    pCsCtrlBlock->exch_start_state = status;
}

/**
 * @brief      Set procedure control block complete status
 * @param[in]  index: index of procedure control block
 * @param[in]  status: procedure complete status
 * @return     None
 */
void user_setCsProcCmpltStatus(cs_control_t *pCsCtrlBlock, eCsProcCmpltStatusMask status)
{
    pCsCtrlBlock->exch_cmplt_state |= status;
}

/**
 * @brief      Clear procedure control block complete status
 * @param[in]  index: index of procedure control block
 * @param[in]  status: procedure complete status
 * @return     None
 */
void user_clrCsProcCmpltStatus(cs_control_t *pCsCtrl, eCsProcCmpltStatusMask status)
{
    pCsCtrl->exch_cmplt_state &= ~status;
}

/**
 * @brief      Initialize CS procedure control block
 * @param[in]  None
 * @return     None
 */
void user_initCsCtrl(void)
{
    memset(&cs_app_ctrl.cs_ctrl[0], 0, sizeof(cs_app_control_t));
}

static void handle_cap_exch_completion(cs_control_t *pCsCtrl)
{
    user_setCsProcStartStatus(pCsCtrl, SET_DEFAULT);
    user_clrCsProcCmpltStatus(pCsCtrl, CAP_EXCH_CMPLT);
}

static void handle_set_default_completion(cs_control_t *pCsCtrl)
{
    user_clrCsProcCmpltStatus(pCsCtrl, SET_DFT_CMPLT);
    uint8_t status = blc_ll_CsFaeExchCtrl(pCsCtrl->connhandle);
    switch (status) {
    case 1: // need wait peer dev initiator fae exchange
        break;
    case 2:
        user_setCsProcStartStatus(pCsCtrl, FAE_EXCH);
        break;
    case 0:
        user_setCsProcCmpltStatus(pCsCtrl, FAE_EXCH_CMPLT);
        break;
    }
}

static void handle_fae_completion(cs_control_t *pCsCtrl)
{
    user_clrCsProcCmpltStatus(pCsCtrl, FAE_EXCH_CMPLT);
#if UI_CONTROL_ENABLE
#if CS_PROCEDURE_CMD_TRIG
    app_parse_printf("please send cs config cmd <cs cc [role][mainmode_type][mode0_step][submode_type][mainmode_repetition][rtt_type]>,if peer not initiate exchange firstly!\r\n");
#else
    user_setCsProcStartStatus(pCsCtrl, CFG_EXCH);
#endif
#endif
}

static void handle_cfg_completion(cs_control_t *pCsCtrl)
{
    user_clrCsProcCmpltStatus(pCsCtrl, CFG_EXCH_CMPLT);
    if (pCsCtrl->acl_role == ACL_ROLE_CENTRAL) {
        if (!blc_ll_getCsSecExchStatus(pCsCtrl->connhandle)) {
            user_setCsProcStartStatus(pCsCtrl, SEC_EXCH);
        } else {
            user_setCsProcCmpltStatus(pCsCtrl, SEC_EXCH_CMPLT);
        }
    }
}

static void handle_sec_completion(cs_control_t *pCsCtrl)
{
    user_clrCsProcCmpltStatus(pCsCtrl, SEC_EXCH_CMPLT);
#if CS_PROCEDURE_CMD_TRIG
#if UI_CONTROL_ENABLE
    app_parse_printf("please send cs procedure param cmd <cs scp [max_procedure_cnt]>,if peer not initiate exchange firstly\r\n");
#endif
#else
    user_setCsProcStartStatus(pCsCtrl, SET_PROC_PARAM);
#endif
}

static void handle_set_proc_param_completion(cs_control_t *pCsCtr)
{
    user_clrCsProcCmpltStatus(pCsCtr, SET_PROC_PARAM_CMPLT);
#if CS_PROCEDURE_CMD_TRIG
#else
    user_setCsProcStartStatus(pCsCtr, CS_PROC_EN_EXCH);
#endif
}

static void handle_cs_proc_en_completion(cs_control_t *pCsCtr)
{
    user_clrCsProcCmpltStatus(pCsCtr, CS_PROC_EN_CMPLT);
}

static void handle_completion_state(cs_control_t *pCsCtrl)
{
    if (pCsCtrl->exch_cmplt_state == NULL_EXCH_CMPLT) {
        return;
    }

    tlkapi_send_string_u32s(APP_CS_LOG_EN, "cs compete", pCsCtrl->exch_cmplt_state);

    if (pCsCtrl->exch_cmplt_state & CAP_EXCH_CMPLT) {
        handle_cap_exch_completion(pCsCtrl);
    } else if (pCsCtrl->exch_cmplt_state & SET_DFT_CMPLT) {
        handle_set_default_completion(pCsCtrl);
    } else if (pCsCtrl->exch_cmplt_state & FAE_EXCH_CMPLT) {
        handle_fae_completion(pCsCtrl);
    } else if (pCsCtrl->exch_cmplt_state & CFG_EXCH_CMPLT) {
        handle_cfg_completion(pCsCtrl);
    } else if (pCsCtrl->exch_cmplt_state & SEC_EXCH_CMPLT) {
        handle_sec_completion(pCsCtrl);
    } else if (pCsCtrl->exch_cmplt_state & SET_PROC_PARAM_CMPLT) {
        handle_set_proc_param_completion(pCsCtrl);
    } else if (pCsCtrl->exch_cmplt_state & CS_PROC_EN_CMPLT) {
        handle_cs_proc_en_completion(pCsCtrl);
    }

    tlkapi_send_string_u32s(APP_CS_LOG_EN, "cs start", pCsCtrl->exch_start_state);
}

static void handle_cap_exchange_start(cs_control_t *pCsCtrl)
{
    blc_hci_le_cs_readRemoteSupportedCap(pCsCtrl->connhandle);
    user_setCsProcStartStatus(pCsCtrl, NULL_EXCH);
}

static void handle_set_default_start(cs_control_t *pCsCtrl)
{
    hci_le_cs_setDefaultSetting_cmdParam_t pcmd = {0};
    hci_le_cs_setDefaultSetting_retParam_t pret = {0};
    pcmd.Connection_Handle                      = pCsCtrl->connhandle;
    pcmd.Role_Enable                            = CS_INITIATOR_ROLE;
    pcmd.Max_TX_Power                           = 0;
    pcmd.CS_SYNC_Antenna_Selection              = 1;
    blc_hci_le_cs_setDefaultSettings(&pcmd, &pret);
    user_setCsProcStartStatus(pCsCtrl, NULL_EXCH);
    user_setCsProcCmpltStatus(pCsCtrl, SET_DFT_CMPLT);
}

static void handle_fae_exchange_start(cs_control_t *pCsCtrl)
{
    blc_hci_le_cs_readRemoteFAE_table(pCsCtrl->connhandle);
    user_setCsProcStartStatus(pCsCtrl, NULL_EXCH);
}

static void handle_cfg_exchange_start(cs_control_t *pCsCtrl)
{
    hci_le_cs_creatConfig_cmdParam_t pcmd = {0};
    pCsCtrl->config_id                    = 0;
    pcmd.Connection_Handle                = pCsCtrl->connhandle;
    pcmd.Config_ID                        = 0;
    pcmd.Create_Context                   = 1;
    pcmd.Main_Mode                        = 2;
    pcmd.Sub_Mode                         = 0xff;
    if (pcmd.Sub_Mode == 0xff) {
        pcmd.Main_Mode_Min_Steps = 0xFF;
        pcmd.Main_Mode_Max_Steps = 0xFF;
    } else {
        pcmd.Main_Mode_Min_Steps = 2;
        pcmd.Main_Mode_Max_Steps = 4;
    }
    pcmd.Main_Mode_Repetition    = 0;
    pcmd.Mode_0_Steps            = 2;
    pcmd.Role                    = CS_CONFIG_INITIATOR_ROLE;
    pcmd.RTT_Type                = 0x00;
    pcmd.CS_SYNC_PHY             = 0x01;
    pcmd.Channel_Map[0]          = 0xfc;
    pcmd.Channel_Map[1]          = 0xff;
    pcmd.Channel_Map[2]          = 0x7f;
    pcmd.Channel_Map[3]          = 0xfc;
    pcmd.Channel_Map[4]          = 0xff;
    pcmd.Channel_Map[5]          = 0xff;
    pcmd.Channel_Map[6]          = 0xff;
    pcmd.Channel_Map[7]          = 0xff;
    pcmd.Channel_Map[8]          = 0xff;
    pcmd.Channel_Map[9]          = 0x1f;
    pcmd.Channel_Map_Repetition  = 1;
    pcmd.ChSel                   = 0;
    pcmd.Ch3c_Shape              = 0;
    pcmd.Ch3c_Jump               = 0x02;
    pcmd.Companion_Signal_Enable = 0;
    blc_hci_le_cs_createConfig(&pcmd);
    user_setCsProcStartStatus(pCsCtrl, NULL_EXCH);
}

static void handle_sec_exchange_start(cs_control_t *pCsCtrl)
{
    if (pCsCtrl->acl_role == ACL_ROLE_CENTRAL) {
        blc_hci_le_cs_security_enable(pCsCtrl->connhandle);
    }
    user_setCsProcStartStatus(pCsCtrl, NULL_EXCH);
}

static void handle_set_proc_param_start(cs_control_t *pCsCtrl)
{
    hci_le_cs_setProcedureParame_cmdParam_t pcmd = {0};
    hci_le_cs_setProcedureParam_retParam_t  buff = {0};
    pcmd.Connection_Handle                       = pCsCtrl->connhandle;
    pcmd.Config_ID                               = pCsCtrl->config_id;
    pcmd.Max_Procedure_Len                       = 65535;
    pcmd.Max_Procedure_Interval                  = 20;
    pcmd.Min_Procedure_Interval                  = 20;
    pcmd.Max_Procedure_Count                     = 0;
    pcmd.Min_Subevent_Len[0]                     = 0x60;
    pcmd.Min_Subevent_Len[1]                     = 0xEA;
    pcmd.Min_Subevent_Len[2]                     = 0x00;
    pcmd.Max_Subevent_Len[0]                     = 0x60;
    pcmd.Max_Subevent_Len[1]                     = 0xEA;
    pcmd.Max_Subevent_Len[2]                     = 0x00;
    pcmd.Tone_Antenna_Config_Selection           = 1;
    pcmd.PHY                                     = 1;
    pcmd.Tx_Pwr_Delta                            = 0;
    pcmd.Preferred_Peer_Antenna                  = 1;
    blc_hci_le_cs_setProcedureParam(&pcmd, &buff);
    user_setCsProcStartStatus(pCsCtrl, NULL_EXCH);
    user_setCsProcCmpltStatus(pCsCtrl, SET_PROC_PARAM_CMPLT);
}

static void handle_cs_proc_en_start(cs_control_t *pCsCtrl)
{
    hci_le_cs_enableProcedure_cmdParam_t pcmd = {0};
    pcmd.Connection_Handle                    = pCsCtrl->connhandle;
    pcmd.Config_ID                            = pCsCtrl->config_id;
    pcmd.Enable                               = 1;
    blc_hci_le_cs_procedureEnable(&pcmd);
    user_setCsProcStartStatus(pCsCtrl, NULL_EXCH);
    blc_cs_algo3Init();
}

static void handle_start_state(cs_control_t *pCsCtrl)
{
    if (pCsCtrl->exch_start_state == NULL_EXCH) {
        return;
    }

    switch (pCsCtrl->exch_start_state) {
    case CAP_EXCH:
        handle_cap_exchange_start(pCsCtrl);
        break;
    case SET_DEFAULT:
        handle_set_default_start(pCsCtrl);
        break;
    case FAE_EXCH:
        handle_fae_exchange_start(pCsCtrl);
        break;
    case CFG_EXCH:
        handle_cfg_exchange_start(pCsCtrl);
        break;
    case SEC_EXCH:
        handle_sec_exchange_start(pCsCtrl);
        break;
    case SET_PROC_PARAM:
        handle_set_proc_param_start(pCsCtrl);
        break;
    case CS_PROC_EN_EXCH:
        handle_cs_proc_en_start(pCsCtrl);
        break;
    default:
        break;
    }
}

/**
 * @brief Executes the control loop for the CS (Connection State) procedure.
 * This function iterates through all possible CS control structures and checks for any with an active connection handle.
 * For CS control structures with an active connection handle, it processes their completion and start states.
 *
 * @param None
 * @return None
 */
void cs_procedure_ctrl_loop(void)
{
    // Loop through all possible CS control structures
    for (int i = 0; i < CS_MAX_NUM; i++) {
        // Get the pointer to the current CS control structure
        cs_control_t *pCsCtrl = &cs_app_ctrl.cs_ctrl[i];
        // Check if there is an active connection handle
        if (pCsCtrl->connhandle) {
            // Handle the completion state of the current CS control structure
            handle_completion_state(pCsCtrl);
            // Handle the start state of the current CS control structure
            handle_start_state(pCsCtrl);
        }
    }
}

/**
 * @brief      BLE CS config complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_config_complete_event_handle(u8 *p)
{
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s", __FUNCTION__);
    hci_le_csConfigCompleteEvt_t *ptr = (hci_le_csConfigCompleteEvt_t *)p;

#if (CS_TLK_ALGO2_EN)
    blc_Algo2_CopyConfigCompleteData(&ptr->Main_Mode, sizeof(hci_le_csConfigCompleteEvt_t) - 6);
#endif
    app_cs_config_t *config = blc_getCSConfig(ptr->Connection_Handle, ptr->Config_ID);
    if (ptr->Action == 0x01) {
        //Configuration is to be created
        if (config == NULL) {
            //config didn't exist,create new config
            app_cs_config_t *newCfg   = blc_findUnusedCSConfig();
            newCfg->Config_ID         = ptr->Config_ID;
            newCfg->Connection_Handle = ptr->Connection_Handle;
            newCfg->Main_Mode         = ptr->Main_Mode;
            newCfg->Sub_Mode          = ptr->Sub_Mode;
            newCfg->Role              = ptr->Role;
            newCfg->RTT_Type          = ptr->RTT_Type;
            newCfg->valid             = TRUE;
        } else {
            //config exists, need to cover
            config->Config_ID         = ptr->Config_ID;
            config->Connection_Handle = ptr->Connection_Handle;
            config->Main_Mode         = ptr->Main_Mode;
            config->Sub_Mode          = ptr->Sub_Mode;
            config->Role              = ptr->Role;
            config->RTT_Type          = ptr->RTT_Type;
        }
    } else if (ptr->Action == 0x00) {
        //Configuration is to be removed
        if (config) {
            config->valid = FALSE;
        }
    }
    blc_rap_csConfigComplete(ptr);
}

/**
 * @brief      BLE CS procedure enable complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_procedure_enable_complete_event_handle(u8 *p)
{
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s", __FUNCTION__);
    hci_le_csProcedureEnableCompleteEvt_t *ptr = (hci_le_csProcedureEnableCompleteEvt_t *)p;

#if (CS_TLK_ALGO2_EN)
    blc_Algo2_CopyProcedureEnableCompleteData(ptr->Tone_Antenna_Config_Selection, ptr->Selected_TX_Power,
                                              ptr->Subevent_Len[0] | (ptr->Subevent_Len[1] << 8) | (ptr->Subevent_Len[2] << 16), ptr->Subevents_Per_Event, ptr->Event_Interval,
                                              ptr->Procedure_Interval, ptr->Procedure_Count);
#endif

    blc_ras_csProcedureEnComplete(ptr); // Inform ras data
}

int app_le_processReadRemoteSupCapCompleteEvt(u8 *p, int n)
{
    (void)n;
    hci_le_readRemoteSupCapCompleteEvt_t *pEvt    = (hci_le_readRemoteSupCapCompleteEvt_t *)p;
    cs_control_t                         *pCsCtrl = user_getCsCtrlByHadle(pEvt->Connection_Handle);

    if (pCsCtrl == NULL) {
        return -1;
    }
    user_setCsProcCmpltStatus(pCsCtrl, CAP_EXCH_CMPLT);
    return 0;
}

int app_le_processRemoteFAE_tableEvt(u8 *p, int n)
{
    (void)n;
    hci_le_readRemoteFAETableCompleteEvt_t *pEvt    = (hci_le_readRemoteFAETableCompleteEvt_t *)p;
    cs_control_t                           *pCsCtrl = user_getCsCtrlByHadle(pEvt->Connection_Handle);

    user_setCsProcCmpltStatus(pCsCtrl, FAE_EXCH_CMPLT);

    return 0;
}

int app_le_csConfigCompleteEvt(u8 *p, int n)
{
    (void)n;
    hci_le_csConfigCompleteEvt_t *ptr     = (hci_le_csConfigCompleteEvt_t *)p;
    cs_control_t                 *pCsCtrl = user_getCsCtrlByHadle(ptr->Connection_Handle);

    user_setCsProcCmpltStatus(pCsCtrl, CFG_EXCH_CMPLT);
    extern u8 ConfigComplete_data[];

    tlkapi_send_string_u32s(APP_LOG_EN, "cs cfg1", ptr->Connection_Handle, pCsCtrl->exch_cmplt_state, (u32)pCsCtrl, (u32)ConfigComplete_data);

#if (CS_TLK_ALGO2_EN)
    blc_Algo2_CopyConfigCompleteData(&ptr->Main_Mode, sizeof(hci_le_csConfigCompleteEvt_t) - 6);
#endif

    app_cs_config_t *config = blc_getCSConfig(ptr->Connection_Handle, ptr->Config_ID);
    if (ptr->Action == 0x01) {
        //Configuration is to be created
        if (config == NULL) {
            //config didn't exist,create new config``
            app_cs_config_t *newCfg   = blc_findUnusedCSConfig();
            newCfg->Config_ID         = ptr->Config_ID;
            newCfg->Connection_Handle = ptr->Connection_Handle;
            newCfg->Main_Mode         = ptr->Main_Mode;
            newCfg->Sub_Mode          = ptr->Sub_Mode;
            newCfg->Role              = ptr->Role;
            newCfg->RTT_Type          = ptr->RTT_Type;
            newCfg->valid             = TRUE;
        } else {
            //config exists, need to cover
            config->Config_ID         = ptr->Config_ID;
            config->Connection_Handle = ptr->Connection_Handle;
            config->Main_Mode         = ptr->Main_Mode;
            config->Sub_Mode          = ptr->Sub_Mode;
            config->Role              = ptr->Role;
            config->RTT_Type          = ptr->RTT_Type;
        }
    } else if (ptr->Action == 0x00) {
        //Configuration is to be removed
        if (config) {
            config->valid = FALSE;
        }
    }
    blc_rap_csConfigComplete(ptr);
    tlkapi_send_string_u32s(APP_LOG_EN, "cs cfg", ptr->Connection_Handle, pCsCtrl->exch_cmplt_state, (u32)pCsCtrl);

    return 0;
}

int app_le_csSecurityEnableCompleteEvt(u8 *p, int n)
{
    (void)n;
    hci_le_csSecurityEnableCompleteEvt_t *pEvt    = (hci_le_csSecurityEnableCompleteEvt_t *)p;
    cs_control_t                         *pCsCtrl = user_getCsCtrlByHadle(pEvt->Connection_Handle);

    user_setCsProcCmpltStatus(pCsCtrl, SEC_EXCH_CMPLT);

    return 0;
}

int app_le_csProcedureEnableCompleteEvt(u8 *p, int n)
{
    (void)n;
    hci_le_csProcedureEnableCompleteEvt_t *pEvt    = (hci_le_csProcedureEnableCompleteEvt_t *)p;
    cs_control_t                          *pCsCtrl = user_getCsCtrlByHadle(pEvt->Connection_Handle);

    user_setCsProcCmpltStatus(pCsCtrl, CS_PROC_EN_CMPLT);
    app_le_cs_procedure_enable_complete_event_handle(p);

    return 0;
}

int app_le_csSubeventResultEvt(u8 *p, int n)
{
    (void)n;
#if (APP_CS_SUBEVENT_LOG_EN)
    hci_le_csSubeventResultEvt_t *pCsSubevent = (hci_le_csSubeventResultEvt_t *)p;

    /* print subevent data to txt file to calculate distance with other company */
    u8 tempBuff[258];
    tempBuff[0] = 0x04; //type
    tempBuff[1] = 0x3E; //event_code
    tempBuff[2] = n;    // total_len
    smemcpy(tempBuff + 3, &pCsSubevent->Subevent_Code, n);

#if (GOOGLE_CS_INIT_ROLE_EN)
    app_parse_printf("Procedure count:[%d], subevent len:[%d]\r\n", pCsSubevent->Procedure_Counter, n);
    app_parse_printf("cs subevent:\n");
    for (int i = 0; i < n + 3; i++) {
        if (tempBuff[i] < 0x10) {
            app_parse_printf("0%x ", tempBuff[i]);
        } else {
            app_parse_printf("%2x ", tempBuff[i]);
        }
    }
    app_parse_printf("\n");
#else
    tlkapi_printf(APP_CS_SUBEVENT_LOG_EN, "subevent len***:%d\r\n", n);
    tlkapi_send_string_data(APP_CS_SUBEVENT_LOG_EN, "cs subevent", tempBuff, n + 3);
#endif
#endif

    hci_le_csSubeventResultEvt_t *ptr = (hci_le_csSubeventResultEvt_t *)p;
    blc_ras_csSubeventResultData(ptr);

    return 0;
}

int app_le_csSubeventResultContinueEvt(u8 *p, int n)
{
    (void)n;
#if (APP_CS_SUBEVENT_LOG_EN)
    hci_le_csSubeventResultContinueEvt_t *pCsSubevent = (hci_le_csSubeventResultContinueEvt_t *)p;
    /* print subevent continue data to txt file to calculate distance with other company */
    u8 tempBuff[258];
    tempBuff[0] = 0x04;
    tempBuff[1] = 0x3E; //event_code
    tempBuff[2] = n;    // total_len
    smemcpy(tempBuff + 3, &pCsSubevent->Subevent_Code, n);

#if (GOOGLE_CS_INIT_ROLE_EN)
    app_parse_printf("continue subevent len:%d\r\n", n);
    app_parse_printf("cs continue subevent:\r\n");
    for (int i = 0; i < n + 3; i++) {
        if (tempBuff[i] < 0x10) {
            app_parse_printf("0%x ", tempBuff[i]);
        } else {
            app_parse_printf("%2x ", tempBuff[i]);
        }
    }
    app_parse_printf("\n");
#else
    tlkapi_printf(APP_CS_SUBEVENT_LOG_EN, "continue subevent len***:%d\r\n", n);
    tlkapi_send_string_data(APP_CS_SUBEVENT_LOG_EN, "cs continue subevent", tempBuff, n + 3);
#endif
#endif

    hci_le_csSubeventResultContinueEvt_t *ptr = (hci_le_csSubeventResultContinueEvt_t *)p;
    blc_ras_csSubeventResultContinueData(ptr);

    return 0;
}

static const cs_event_handler_t event_handlers[] = {
    {HCI_SUB_EVT_LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES_COMPLETE, app_le_processReadRemoteSupCapCompleteEvt, "cs capability exchange success\r\n"},
    {HCI_SUB_EVT_LE_CS_READ_REMOTE_FAE_TABLE_COMPLETE, app_le_processRemoteFAE_tableEvt, "cs fae exchange success\r\n"},
    {HCI_SUB_EVT_LE_CS_CONFIG_COMPLETE, app_le_csConfigCompleteEvt, "cs cfg exchange success\r\n"},
    {HCI_SUB_EVT_LE_CS_SECURITY_ENABLE_COMPLETE, app_le_csSecurityEnableCompleteEvt, "cs sec exchange success\r\n"},
    {HCI_SUB_EVT_LE_CS_PROCEDURE_ENABLE_COMPLETE, app_le_csProcedureEnableCompleteEvt, "cs measurement start\r\n"},

    {HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT, app_le_csSubeventResultEvt, NULL},
    {HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT_CONTINUE, app_le_csSubeventResultContinueEvt, NULL},

};

void app_handle_cs_event(u8 *p, int n)
{
    u8 subEvt_code = p[0];

    for (size_t i = 0; i < sizeof(event_handlers) / sizeof(event_handlers[0]); i++) {
        if (event_handlers[i].subEvt_code == subEvt_code) {
            event_handlers[i].event_handler(p, n);

            if (event_handlers[i].log_message != NULL) {
#if UI_CONTROL_ENABLE
                app_parse_printf(event_handlers[i].log_message);
#endif
                tlkapi_send_string_u8s(APP_LOG_EN, event_handlers[i].log_message);
            }


            return;
        }
    }
}

/**
 * @brief      Find unsused cs config buffer
 * @param[in]  None
 * @return     Pointer to unused CS config buffer
 */
app_cs_config_t *blc_findUnusedCSConfig(void)
{
    int idx = 0;
    for (idx = 0; idx < APP_CS_CONFIG_NUM; idx++) {
        if (app_cs_config[idx].valid == FALSE) {
            return app_cs_config + idx;
        }
    }
    return NULL;
}

/**
 * @brief      Get cs config buffer by acl connect handle and config ID
 * @param[in]  connhandle ACL connect handle
 * @param[in]  Config_ID  config ID
 * @return     Pointer to unused CS config buffer
 */
app_cs_config_t *blc_getCSConfig(u16 connHandle, u8 Config_ID)
{
    int idx = 0;
    for (idx = 0; idx < APP_CS_CONFIG_NUM; idx++) {
        if (app_cs_config[idx].Connection_Handle == connHandle && app_cs_config[idx].Config_ID == Config_ID && app_cs_config[idx].valid == TRUE) {
            return app_cs_config + idx;
        }
    }
    return NULL;
}

#if (CS_DISTANCE_FILTER)
/**
 * @brief      Initialize app ranging filter
 * @param[in]  None
 * @return     None
 */
void app_rangingFilter_init(void)
{
    for (int i = 0; i < MAX_DISTANCE_CNT_SUPPORT; i++) {
        // update kalman filter
        filter_ctrl[i].kf.state          = 0.0;
        filter_ctrl[i].kf.err_cov        = 1.0;
        filter_ctrl[i].kf.proc_noise_cov = 0.001;
        filter_ctrl[i].kf.msr_noise_cov  = 0.01;
        filter_ctrl[i].kf.kal_gain       = 0.0;
        // update amplitude filter
        filter_ctrl[i].lastValidAmplitudeDist = 0.0;
    }
}

/**
 * @brief      update Kalman filter
 * @param[in]  None
 * @return     None
 */
static float kalmanFilter_update(kalmanFilter_t *pkf, float measurement)
{
    pkf->err_cov += pkf->proc_noise_cov;
    pkf->kal_gain = pkf->err_cov / (pkf->err_cov + pkf->msr_noise_cov);
    pkf->state += pkf->kal_gain * (measurement - pkf->state);
    pkf->err_cov = (1 - pkf->kal_gain) * pkf->err_cov;

    return pkf->state;
}

/** filtFirst
 * @brief  Primary filter, limit amplitude.
 * @param[in]  distance : the origin distance.
 * @param[in]  *lastValidDistance : pointer to last distance.
 * @@return
 */
static float amplitudeFilter(float distance, float *lastValidDistance)
{
    float filt_dis;

    if (distance < 0.01f || distance > 150.0f) {
        filt_dis = *lastValidDistance;
        return filt_dis;
    }

    if (*lastValidDistance) {
        float trend = (abs(distance - *lastValidDistance));
        if (trend > 2.0f) {
            if (distance > *lastValidDistance) {
                filt_dis = *lastValidDistance + 1.0f;
            } else {
                filt_dis = *lastValidDistance - 1.0f;
            }
        } else {
            filt_dis = distance;
        }
    } else {
        filt_dis = distance;
    }
    *lastValidDistance = filt_dis;
    return filt_dis;
}

/**
 * @brief      CS distance filter, first amplitude filter, second kalman filter.
 * @param[in]  *ctrl   Pointer of filter control block.
 * @param[in]  in      in data
 * @param[out] *out    Pointer of out data.
 * @return
 */
void app_rangingFilter(app_filter_ctrl *ctrl, float in, float *out)
{
    if (isnan(in) || isinf(in)) {
        in = ctrl->lastValidDist;
    } else {
        ctrl->lastValidDist = in;
    }

    float af_dist = amplitudeFilter(in, &ctrl->lastValidAmplitudeDist);

    float kalman_dist = kalmanFilter_update(&ctrl->kf, af_dist);

    if (isnan(kalman_dist) || isinf(kalman_dist)) {
        kalman_dist = ctrl->lastValidFiltDist;
    } else {
        ctrl->lastValidFiltDist = kalman_dist;
    }
    *out = kalman_dist;
}
#endif

/**
 * @brief       for calculate the ranging data of one procedure.
 * @param[in]   connHandle: ACL handle..
 * @param[in]   *pData: ranging data
 * @param[out] *distance: if it is mode1, only one distance.
 *                        if it is mode2, could be 1~3 distances return.
 * @return      0     - the result of distance is valid.
 *              other - the result of distance is invalid.
 */
s32 blc_calcRangData(u16 connHandle, u8 *pRangingData, float *distance)
{
    blc_rasc_ranging_data_evt_t *rangingData    = (blc_rasc_ranging_data_evt_t *)pRangingData;
    u16                          rangingCounter = rangingData->rangingCounter;
    tlkapi_printf(APP_LOG_EN, "[APP][CS] rangingCounter=%d", rangingCounter);

    s32 retVal = 0;

    // 1.check procedure data. delete abort subevent data. restore remote step data(protocol to procedure)
    blt_ras_proc_ctrl_t remoteProcCtrl;
    blt_ras_proc_ctrl_t localProcCtrl;
    smemset(&remoteProcCtrl, 0, sizeof(blt_ras_proc_ctrl_t));
    smemset(&localProcCtrl, 0, sizeof(blt_ras_proc_ctrl_t));
    retVal = blc_restoreProcedureData(connHandle, &remoteProcCtrl, &localProcCtrl, pRangingData);
    if (retVal) {
        return retVal;
    }

    // 2.calculate distance
    blt_ras_proc_ctrl_t *localProcedure = blc_getLocalProcedureData(connHandle, rangingCounter);
    app_cs_config_t     *cfg            = blc_getCSConfig(connHandle, localProcedure->procedureHead.data.proCountCfgID);
    if (cfg->Role == INITIATOR_ROLE) {
        retVal = csCalculateDistance(connHandle, &localProcCtrl, &remoteProcCtrl, cfg->Main_Mode, distance);
    } else {
        retVal = csCalculateDistance(connHandle, &remoteProcCtrl, &localProcCtrl, cfg->Main_Mode, distance);
    }

    return retVal;
}

/**
 * @brief      SDP end handler.
 * @param[in]  connHandle       ACL connect handle
 * @param[in]  *pData           Pointer to sdp end data buffer
 * @param[in]  dataLen          length of data
 * @return     0x00
 */
static int app_prf_sdp_end(u16 connHandle, u8 *pData, u16 dataLen)
{
    (void)connHandle;
    (void)dataLen;
    blc_prf_sdpEndEvt_t *evt = (blc_prf_sdpEndEvt_t *)pData;
    if (evt->svcId == CS_RAS_CLIENT) {
        tlkapi_printf(APP_LOG_EN, "[APP][CS] RAS service has been found.");
        svc_ras_feature_t *feature = NULL;
        ble_sts_t          ret     = blc_rasc_getRasFeature(connHandle, &feature);
        if (ret == BLE_SUCCESS) {
            if (feature->realTimeProcedureDataSupport) {
                blc_rapc_writeRealtimeCcc(connHandle, 0x01, NULL);
            } else {
                blc_rapc_writeOnDemandCcc(connHandle, 0x01, NULL);
            }
        }

#if CS_PROCEDURE_EXCHANGE
        cs_control_t *pCsCtrlBlock = user_addCsCtrlByHadle(connHandle);
        if (pCsCtrlBlock == NULL) {
            //error,buffer not enough
            tlkapi_printf(APP_LOG_EN, "[APP][CS] add cs ctrl error.\r\n");
            return -1;
        }
        user_setCsProcStartStatus(pCsCtrlBlock, CAP_EXCH);

        pCsCtrlBlock->acl_role = dev_char_get_conn_role_by_connhandle(connHandle);


#if UI_CONTROL_ENABLE
        app_parse_printf("wait cs capability exchange\r\n");
#endif

        tlkapi_send_string_u32s(APP_LOG_EN, "SDP end", (u32)pCsCtrlBlock, pCsCtrlBlock->connhandle, pCsCtrlBlock->exch_cmplt_state);
#endif
    }
    return 0;
}

////////////////// algo1 and algo2 switch function ///////////////////////
// note: default use algo2 when reset mcu, if continue 4 times distance calculate with algo2, system will enable algo1
//       use algo1 distance and must reset distance filter machine, please remind that algo2 should still keep calculating.
//       After algo1 and algo2 both calculate, check if algo2 continue 4times get distance then disable algo1 and default use
//       algo2 distance and update distance filter machine.

#define CS_ALGO_AUTO_SWITCH 0

#if (CS_ALGO_AUTO_SWITCH)
u8 algo2_dist_mark_cnt = 0;
u8 algo1_enable        = 0;

static void blc_cs_algoAutoSwitch(s32 retval)
{
#define ALGO_SWTICH_MAX_ABORT_COUNT 4 // This macro decide how many times with algo2 distance error/correct to switch algo
#define ALGO2_CALC_SUCCESS          0 // Calculate distance with distance2 success
    extern void app_parse_printf(const char *format, ...);
    // todo: consider indoor distance test, never use algo1 in indoor env
    if (algo1_enable == 0) {
        if (retval != ALGO2_CALC_SUCCESS) {
            app_parse_printf("[algo1 disable,algo2 fail,mark add 1]\r\n");
            algo2_dist_mark_cnt++;
        } else {
            app_parse_printf("[algo1 disable,algo2 fine,mark clean]\r\n");
            algo2_dist_mark_cnt = 0;
        }
    }
    if (algo1_enable == 1) {
        if (retval == ALGO2_CALC_SUCCESS) {
            app_parse_printf("[algo1 enable,algo2 fine,mark add 1]\r\n");
            algo2_dist_mark_cnt++;
        } else {
            app_parse_printf("[algo1 enable,algo2 fail,mark clean]\r\n");
            algo2_dist_mark_cnt = 0;
        }
    }

    if (algo2_dist_mark_cnt >= ALGO_SWTICH_MAX_ABORT_COUNT) {
        if (algo1_enable) { // algo1 -> algo2, only use algo2 now.
            algo1_enable = 0;
            blc_cs_removeAlgoMask(BLC_RANGING_ALGORITHM_1);
            app_parse_printf("[marker cnt over max(%d),switch algo,only using algo2 now]\r\n", algo2_dist_mark_cnt); // lambda
        } else {                                                                                                     // algo2->algo1, use algo1 distance, but algo2 keep running
            algo1_enable = 1;
            blc_cs_addAlgoMask(BLC_RANGING_ALGORITHM_1);
            app_parse_printf("[marker cnt over max(%d),switch algo,only using algo1 now]\r\n", algo2_dist_mark_cnt); // phase
        }
        algo2_dist_mark_cnt = 0;
        app_rangingFilter_init();
    }

    if (algo1_enable) {
        gpio_write(GPIO_LED_WHITE, 0);
        gpio_write(GPIO_LED_GREEN, 1);
    } else {
        gpio_write(GPIO_LED_WHITE, 1);
        gpio_write(GPIO_LED_GREEN, 0);
    }
}
#endif
////////////////// algo1 and algo2 switch function done //////////////////

/**
 * @brief      CS procedure data ready to calculate distance.
 * @param[in]  connHandle       ACL connect handle
 * @param[in]  *pRangingData    ranging data
 * @param[in]  dataLen          length of data
 * @return     0x00
 */
static int app_cs_procedure_data(u16 connHandle, u8 *pRangingData, u16 dataLen)
{
    (void)dataLen;

    DBG_CS_CHN10_HIGH;
    u32   algo_mask                          = blc_cs_getAlgoMask();
    float distance[MAX_DISTANCE_CNT_SUPPORT] = {0.0f};
    s32   retval                             = blc_calcRangData(connHandle, pRangingData, distance);
#if (CS_ALGO_AUTO_SWITCH)
    blc_cs_algoAutoSwitch(retval); // cs algo1/2 auto switch
    float filter_dist[MAX_DISTANCE_CNT_SUPPORT] = {0.0f};
    for (int i = 0; i < MAX_DISTANCE_CNT_SUPPORT; i++) {
        app_rangingFilter(&filter_ctrl[i], distance[i], &filter_dist[i]);
        tlkapi_printf(APP_LOG_EN, "dist[%d]: %f, kalman: %f", i, distance[i], filter_dist[i]);
    }
    if (algo1_enable) {
        app_parse_printf("{\"title\":\"cs_dist\",\"unfiltered-dist\":{\"dist\":[%.1f]},\"filtered-dist\":{\"dist\":[%.1f]}}\r\n", distance[idx],
                         filter_dist[idx++]); // in this situation, we need use algo1 distance
    } else {
        if (retval == ALGO2_CALC_SUCCESS) {
            app_parse_printf("{\"title\":\"cs_dist\",\"unfiltered-dist\":{\"dist\":[%.1f]},\"filtered-dist\":{\"dist\":[%.1f]}}\r\n", distance[idx],
                             filter_dist[idx++]); // in this situation, we need use algo1 distance
        } else {                                  // algo1 not enable and algo2 calculate fail
            app_parse_printf("ALGO2 Calculate Failed,Error Code: %d\r\n", retval);
        }
    }

#else
    if (retval == CS_DIST_SUCCESS) {
        // If algo2 enable,default print algo2 distance, then algo3 distance, finally algo1 distance
        // user can change final parse distance.
        u8 parse_idx;
        if (algo_mask & BLC_RANGING_ALGORITHM_2) {
            parse_idx = 1;
        } else {
            if (algo_mask & BLC_RANGING_ALGORITHM_3) {
                parse_idx = 2;
            } else {
                parse_idx = 0;
            }
        }
#if (CS_DISTANCE_FILTER)
        float filter_dist[MAX_DISTANCE_CNT_SUPPORT] = {0.0f};
        for (int i = 0; i < MAX_DISTANCE_CNT_SUPPORT; i++) {
            if (distance[i] < 0.0f) {
                continue;
            }
            app_rangingFilter(&filter_ctrl[i], distance[i], &filter_dist[i]);
            tlkapi_printf(APP_LOG_EN, "dist[%d]: %f, kalman: %f", i, distance[i], filter_dist[i]);
        }
        app_parse_printf("{\"title\":\"cs_dist\",\"unfiltered-dist\":{\"dist\":[%.1f]},\"filtered-dist\":{\"dist\":[%.1f]}}\r\n", distance[parse_idx], filter_dist[parse_idx]);
#else
        app_parse_printf("{\"title\":\"cs_dist\",\"dist\":[%.1f]}\r\n", distance[parse_idx]);
#endif
    } else {
        tlkapi_printf(APP_LOG_EN, "distance error, error code: %d", retval);
        app_parse_printf("distance error, error code: %d\r\n", retval);
    }
    DBG_CS_CHN10_LOW;
#endif
    return 0;
}

/**
 * @brief      Remote procedure data timeout handler.
 * @param[in]  connHandle ACL connect handle
 * @param[in]  *pData     Refer to blc_ras_timeout_evt_t
 * @param[in]  dataLen    length of blc_ras_timeout_evt_t
 * @return     0x00
 */
static int app_cs_procedure_timeout(u16 connHandle, u8 *pData, u16 dataLen)
{
    (void)pData;
    (void)dataLen;

    tlkapi_printf(APP_LOG_EN, "[APP][CS] RAS timeout for conn:%02X", connHandle);
    app_parse_printf("RAS timeout for conn:%02X\r\n", connHandle);
    return 0;
}

/**
 * @brief      Remote procedure data overwritten handler.
 * @param[in]  connHandle ACL connect handle
 * @param[in]  *pData     Pointer to overwritten event data buffer
 * @param[in]  dataLen    length of data
 * @return     0x00
 */
static int app_cs_procedure_overwritten(u16 connHandle, u8 *pData, u16 dataLen)
{
    blc_rasc_overwritten_evt_t *overwrittenEvent = (blc_rasc_overwritten_evt_t *)pData;
    (void)dataLen;

    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] RAS Remote overwritten for conn:%02X, rangCtr %d", connHandle, overwrittenEvent->rangingCounter);

    return 0;
}

static const app_prf_evtCb_t csCentralEvt[] = {
    {PRF_EVTID_CLIENT_SDP_END, app_prf_sdp_end},
    {CS_EVT_RANGING_DATA, app_cs_procedure_data},
    {CS_EVT_OVERWRITTEN, app_cs_procedure_overwritten},
    {CS_EVT_TIMEOUT, app_cs_procedure_timeout},
};
PRF_EVT_CB(csCentralEvt)


#define HOST_MALLOC_BUFF_SIZE ((RAS_PROCEDURE_COUNT * 2 + 1) * PROCEDURE_DATA_LEN + 4 * 1024)

static u8 hostMallocBuffer[HOST_MALLOC_BUFF_SIZE];

/**
 * @brief       This function initializes the local capabilities for channel sounding, sets up the necessary buffers,
 *              loads calibration tables, configures antenna switching (if enabled), sets the transmission power level,
 *              initializes the RAS client, and enables the distance calculation algorithm..
 * @param      None
 * @return     None
 */
void app_channel_sounding_init(void)
{
    //Initialize local capabilities
    chn_sound_capabilities_t appCsLocalSupportCap = {
        .Num_Config_Supported                 = 1, //range 1-4
        .max_consecutive_procedures_supported = 0,
        .Num_Antennas_Supported               = NUM_ANT_SUPPORT,
        .Max_Antenna_Paths_Supported          = MAX_ANT_PATHS_SUPPORT,

        .Roles_Supported                   = CS_INITIATOR_ROLE,
        .Mode_Types                        = 0, //mandatory mode1 and mode 2
        .RTT_Capability                    = 0, //150ns
        .RTT_AA_Only_N                     = 240,
        .RTT_Sounding_N                    = 240,
        .RTT_Random_Payload_N              = 240,
        .Optional_NADM_Sounding_Capability = 0,
        .Optional_NADM_Random_Capability   = 0,
        .Optional_CS_SYNC_PHYs_Supported   = 0 | BIT(1), //just mandatory 1M PHY
        .Optional_Subfeatures_Supported    = 0,
        .Optional_T_IP1_Times_Supported    = 0, //only support 145us
        .Optional_T_IP2_Times_Supported    = 0, //only support 145us
        .Optional_T_FCS_Times_Supported    = 0, //only support 150us
        .Optional_T_PM_Times_Supported     = CS_T_PM_20US,
        .T_SW_Time_Supported               = 10, //10us
        .Optional_TX_SNR_Capability        = 0xff,
    };
    blc_ll_initCsInitiatorModule(&appCsLocalSupportCap);

    //Initialize CS buffer
    blc_ll_initCsConfigParam(app_CsConfigParam, APP_CS_CONFIG_NUM);
    blc_ll_initCsRxFifo(app_cs_rx_buf, CS_RX_FIFO_SIZE, CS_RX_FIFO_NUM);

    //Load calibration table for RTT.
    blc_loadCsCali_table(flash_sector_calibration + CALIB_OFFSET_CALI_TABLE_HEADER_INFO);

#if (ANTENNA_SWITCHING_AUTO_EN)
    //Initialize multi-antenas
    cs_ant_switch_config_t ant_cfg = {
        .ant_default_seq_value   = 0,
        .ant_ctrl_seq_base_value = ANTENNA_SWITCHING_CTRL_BASE,
    };

    rf_cs_ant_switch_ctrl ant_switch_ctrl[] = {
            ANTENNA_SWITCHING_SEL_0_PIN, ATSEL_0,
            ANTENNA_SWITCHING_SEL_1_PIN, ATSEL_1,
            #if(BOARD_SELECT != BOARD_721X_EVK_CIT314A102)
            ANTENNA_SWITCHING_SEL_2_PIN, ATSEL_2,
            #endif
    };
    blc_cs_antenna_switch_config_init(&ant_cfg);
    rf_cs_ant_switch_pin_init(ant_switch_ctrl, sizeof(ant_switch_ctrl)/sizeof(rf_cs_ant_switch_ctrl));
#endif

#if (MCU_CORE_TYPE == MCU_CORE_TL721X)
    blc_cs_disableGpioPinsFromD4ToD7();
#endif

    //Set cs use tx power level
    blc_cs_set_tx_power_level(CS_USE_TX_POWER_LEVEL);

    //Initialize RAS client
    blc_prf_initialModule(app_prf_eventCb, hostMallocBuffer, HOST_MALLOC_BUFF_SIZE);
    blc_rap_registerRasProfileControlClient(NULL);

#if (CS_DISTANCE_FILTER)
    //Initialize distance filter
    app_rangingFilter_init();
#endif

    //Enable distance calculate algorithm, see blc_ranging_algorithm_enum.
    blc_cs_enableAlgoMask(BLC_RANGING_ALGORITHM_3);

    //Initialize internal delay
    blc_cs_initInternalDelay();

    //Init cs ranging log
    blc_cs_initRangingLog();
}
