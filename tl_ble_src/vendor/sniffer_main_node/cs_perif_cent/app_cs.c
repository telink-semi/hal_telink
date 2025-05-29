/********************************************************************************************************
 * @file    app_cs.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#include "app.h"
#include "app_cs.h"
#include "app_buffer.h"
#include "app_main_node.h"
#include "app_parse_ui.h"
#include "app_parse_char.h"
#include "math.h"

#include "algorithm/hadm/gcc10/cs_cal.h"


#if (MAIN_NODE_ROLE_SELECT == MAIN_NODE_CS_PERIPHERAL_CENTRAL)

#ifndef isnan
    #define isnan(d) (d != d)
#endif

#ifndef isinf
    #define isinf(d) (isnan((d - d)) && !isnan(d))
#endif

typedef enum
{
    INITIATOR_ROLE = 0,
    REFLECTOR_ROLE = 1,
} app_ranging_role_t;

app_cs_config_t app_cs_config[APP_CS_CONFIG_NUM] = {0};

    #if CS_PROCEDURE_EXCHANGE

cs_app_control_t cs_app_ctrl;

const char *hex_buffer_to_hex_string(const void *buf, u8 len)
{
    static const char hex[] = "0123456789abcdef";
    static char str[301];
    const uint8_t *b = buf;
    u8 i;

    len = min(len, (sizeof(str) - 1) / 2);

    for (i = 0; i < len; i++) {
        str[i * 2]     = hex[b[i] >> 4];
        str[i * 2 + 1] = hex[b[i] & 0xf];
    }

    str[i * 2] = '\0';

    return str;
}

/**
 * @brief      Add CS procedure control block by acl connect handle
 * @param[in]  connhandle ACL connect handle
 * @return     0x01 success 0x00 failed
 */
int user_addCsCtrlByHadle(u16 connhandle)
{
    int ret = 0;
    for (int i = 0; i < CS_MAX_NUM; i++) {
        if (cs_app_ctrl.cs_ctrl[i].connhandle == 0) {
            cs_app_ctrl.cs_ctrl[i].connhandle = connhandle;
            ret                               = 1;
            break;
        }
    }
    return ret;
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
int user_getCsCtrlByHadle(u16 connhandle)
{
    int ret = 0;
    for (int i = 0; i < CS_MAX_NUM; i++) {
        if (cs_app_ctrl.cs_ctrl[i].connhandle == connhandle) {
            ret = i + 1;
            break;
        }
    }
    return ret;
}

/**
 * @brief      Set procedure control block start status
 * @param[in]  index: index of procedure control block
 * @param[in]  status: procedure start status
 * @return     None
 */
void user_setCsProcStartStatus(u8 index, eCsProcStatus status)
{
    cs_app_ctrl.cs_ctrl[index % CS_MAX_NUM].exch_start_state = status;
}

/**
 * @brief      Set procedure control block complete status
 * @param[in]  index: index of procedure control block
 * @param[in]  status: procedure complete status
 * @return     None
 */
void user_setCsProcCmpltStatus(u8 index, eCsProcCmpltStatusMask status)
{
    cs_app_ctrl.cs_ctrl[index % CS_MAX_NUM].exch_cmplt_state |= status;
}

/**
 * @brief      Clear procedure control block complete status
 * @param[in]  index: index of procedure control block
 * @param[in]  status: procedure complete status
 * @return     None
 */
void user_clrCsProcCmpltStatus(u8 index, eCsProcCmpltStatusMask status)
{
    cs_app_ctrl.cs_ctrl[index % CS_MAX_NUM].exch_cmplt_state &= ~status;
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

/**
 * @brief      BLE channel sounding procedure control loop
 * @param      None
 * @return     None
 */
void cs_procedure_ctrl(void)
{
    for (int i = 0; i < CS_MAX_NUM; i++) {
        if (cs_app_ctrl.cs_ctrl[i].connhandle) {
            u8 index = i;
            if (cs_app_ctrl.cs_ctrl[index].exch_cmplt_state != NULL_EXCH_CMPLT) {
                if (cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & CAP_EXCH_CMPLT) {
                    user_setCsProcStartStatus(index, SET_DEFAULT);
                    user_clrCsProcCmpltStatus(index, CAP_EXCH_CMPLT);
                } else if (cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & SET_DFT_CMPLT) {
        #if UI_CONTROL_ENABLE
                    app_parse_printf("cs set default success\r\n");
        #endif
                    user_clrCsProcCmpltStatus(index, SET_DFT_CMPLT);
                    u8 status = blc_ll_CsFaeExchCtrl(cs_app_ctrl.cs_ctrl[index].connhandle);

                    if (status == 1) { //need wait peer dev initiator fae exchange

                    } else if (status == 2) {
                        cs_app_ctrl.cs_ctrl[index].exchange_tick = clock_time() | 1;
                        user_setCsProcStartStatus(index, FAE_EXCH);
                    } else if (status == 0) {
                        user_setCsProcCmpltStatus(index, FAE_EXCH_CMPLT);
                    }

                } else if (cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & FAE_EXCH_CMPLT) {
                    user_clrCsProcCmpltStatus(index, FAE_EXCH_CMPLT);
        #if CS_PROCEDURE_CMD_TRIG
            #if UI_CONTROL_ENABLE
                    app_parse_printf("please send cs config cmd <cs cc [role][mainmode_type][mode0_step][submode_type][mainmode_repetition][rtt_type]>,if peer not initiate exchange firstly!\r\n");
            #endif
        #else
                    user_setCsProcStartStatus(index, CFG_EXCH);
                    cs_app_ctrl.cs_ctrl[index].exchange_tick = clock_time() | 1;
        #endif
                } else if (cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & CFG_EXCH_CMPLT) {
                    user_clrCsProcCmpltStatus(index, CFG_EXCH_CMPLT);
                    if (cs_app_ctrl.cs_ctrl[index].acl_role == ACL_ROLE_CENTRAL) {
                        if (!(blc_ll_getCsSecExchStatus(cs_app_ctrl.cs_ctrl[index].connhandle))) {
                            user_setCsProcStartStatus(index, SEC_EXCH);
                        } else {
                            user_setCsProcCmpltStatus(index, SEC_EXCH_CMPLT);
                        }
                    }
                } else if (cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & SEC_EXCH_CMPLT) {
                    user_clrCsProcCmpltStatus(index, SEC_EXCH_CMPLT);
        #if CS_PROCEDURE_CMD_TRIG
                    {
            #if UI_CONTROL_ENABLE
                        app_parse_printf("please send cs procedure param cmd <cs scp [max_procedure_cnt]>,if peer not initiate exchange firstly\r\n");
            #endif
                    }
        #else
                    user_setCsProcStartStatus(index, SET_PROC_PARAM);
        #endif
                } else if (cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & SET_PROC_PARAM_CMPLT) {
                    user_clrCsProcCmpltStatus(index, SET_PROC_PARAM_CMPLT);
        #if CS_PROCEDURE_CMD_TRIG

        #else
                    user_setCsProcStartStatus(index, CS_PROC_EN_EXCH);
        #endif
                } else if (cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & CS_PROC_EN_CMPLT) {
                    user_clrCsProcCmpltStatus(index, CS_PROC_EN_CMPLT);
                }
            }


            if (cs_app_ctrl.cs_ctrl[index].exch_start_state == NULL_EXCH) {
                return;
            }

            switch (cs_app_ctrl.cs_ctrl[index].exch_start_state) {
            case CAP_EXCH:
            {
                if (cs_app_ctrl.cs_ctrl[index].exchange_tick && clock_time_exceed(cs_app_ctrl.cs_ctrl[index].exchange_tick, 500 * 1000)) {
                    blc_hci_le_cs_readRemoteSupportedCap(cs_app_ctrl.cs_ctrl[index].connhandle);
                    user_setCsProcStartStatus(index, NULL_EXCH);
                    cs_app_ctrl.cs_ctrl[index].exchange_tick = 0;
                }
                break;
            }

            case SET_DEFAULT:
            {
                u8                                      pcmd[sizeof(hci_le_cs_setDefaultSetting_cmdParam_t)] = {0};
                u8                                      pret[sizeof(hci_le_cs_setDefaultSetting_retParam_t)] = {0};
                hci_le_cs_setDefaultSetting_cmdParam_t *p                                                    = (hci_le_cs_setDefaultSetting_cmdParam_t *)pcmd;

                p->Connection_Handle         = cs_app_ctrl.cs_ctrl[index].connhandle;
                p->Role_Enable               = CS_INITIATOR_ROLE; //initiator role en   //CS_INIT_REFL_ROLE; //all role en
                p->Max_TX_Power              = 0; //0dbm
                p->CS_SYNC_Antenna_Selection = 1;

                blc_hci_le_cs_setDefaultSettings(p, (hci_le_cs_setDefaultSetting_retParam_t *)pret);

                user_setCsProcStartStatus(index, NULL_EXCH);
                user_setCsProcCmpltStatus(index, SET_DFT_CMPLT);
                break;
            }

            case FAE_EXCH:
            {
                if (cs_app_ctrl.cs_ctrl[index].exchange_tick && clock_time_exceed(cs_app_ctrl.cs_ctrl[index].exchange_tick, 100 * 1000)) {
                    blc_hci_le_cs_readRemoteFAE_table(cs_app_ctrl.cs_ctrl[index].connhandle);
                    user_setCsProcStartStatus(index, NULL_EXCH);
                    cs_app_ctrl.cs_ctrl[index].exchange_tick = 0;
                }
                break;
            }

            case CFG_EXCH:
            {
                if (cs_app_ctrl.cs_ctrl[index].exchange_tick && clock_time_exceed(cs_app_ctrl.cs_ctrl[index].exchange_tick, 100 * 1000)) {
                    u8                                pcmd[sizeof(hci_le_cs_creatConfig_cmdParam_t)] = {0};
                    hci_le_cs_creatConfig_cmdParam_t *p                                              = (hci_le_cs_creatConfig_cmdParam_t *)pcmd;
                    cs_app_ctrl.cs_ctrl[index].config_id                                             = 0;
                    p->Connection_Handle                                                             = cs_app_ctrl.cs_ctrl[index].connhandle;
                    p->Config_ID                                                                     = 0;
                    p->Create_Context                                                                = 1;
                    p->Main_Mode                                                                     = 2;    //mode2
                    p->Sub_Mode                                                                      = 0xff; //unused

                    if (p->Sub_Mode == 0xff) {
                        p->Main_Mode_Min_Steps = 0x02;
                        p->Main_Mode_Max_Steps = 0xFF;
                    } else {
                        p->Main_Mode_Min_Steps = 2;
                        p->Main_Mode_Max_Steps = 4;
                    }

                    p->Main_Mode            = 2;    //Main_Mode !=0, <= 3
                    p->Main_Mode_Repetition = 0;
                    p->Mode_0_Steps         = 3;    //3
                    p->Role                 = CS_CONFIG_INITIATOR_ROLE;
                    p->RTT_Type             = 0x00; //RTT CS Access Address only timing
                    p->CS_SYNC_PHY          = 0x01; //LE 1M PHY
                    /*
                     * Channels n = 0, 1, 23, 24, 25, 77, and 78 shall be ignored and shall
                     *  be set to zero. At least 15 channels shall be enabled.
                     */
        #if (1)
                    p->Channel_Map[0]         = appCsParamSetting.channelMap[0];
                    p->Channel_Map[1]         = appCsParamSetting.channelMap[1];
                    p->Channel_Map[2]         = appCsParamSetting.channelMap[2];
                    p->Channel_Map[3]         = appCsParamSetting.channelMap[3];
                    p->Channel_Map[4]         = appCsParamSetting.channelMap[4];
                    p->Channel_Map[5]         = appCsParamSetting.channelMap[5];
                    p->Channel_Map[6]         = appCsParamSetting.channelMap[6];
                    p->Channel_Map[7]         = appCsParamSetting.channelMap[7];
                    p->Channel_Map[8]         = appCsParamSetting.channelMap[8];
                    p->Channel_Map[9]         = appCsParamSetting.channelMap[9];
        #else
                    p->Channel_Map[0]           = 0x00;//0xfc;// 0~7
                    p->Channel_Map[1]           = 0x00;//0xff;// 8~15
                    p->Channel_Map[2]           = 0x00;//0x7f;// 16~23
                    p->Channel_Map[3]           = 0x00;//0xf0;// 24~31
                    p->Channel_Map[4]           = 0x00;//0xff;// 32~39
                    p->Channel_Map[5]           = 0xAA;//0xff;// 40~47
                    p->Channel_Map[6]           = 0xff,//0xff;// 48~55
                    p->Channel_Map[7]           = 0xAA;//0xff;// 56~63
                    p->Channel_Map[8]           = 0xff;//0xff;// 64~71
                    p->Channel_Map[9]           = 0x0A;//0x1f;// 72~79
        #endif
                    p->Channel_Map_Repetition  = 1;
                    p->ChSel                   = 0;    //Use Channel Selection Algorithm #3b for non-mode 0 CS steps
                    p->Ch3c_Shape              = 0;    //Use Hat shape for user-specified channel sequence
                    p->Ch3c_Jump               = 0x02; //Number of channels skipped in each rising and falling sequence
                    p->Companion_Signal_Enable = 0;    //Companion Signal disabled

                    blc_hci_le_cs_createConfig(p);

        #if UI_CONTROL_ENABLE
                    app_parse_printf("Auto cs create config\r\n");
        #endif
                    user_setCsProcStartStatus(index, NULL_EXCH);
                    cs_app_ctrl.cs_ctrl[index].exchange_tick = 0;
                }
                break;
            }

            case SEC_EXCH:
            {
                if (cs_app_ctrl.cs_ctrl[index].acl_role == ACL_ROLE_CENTRAL) {
                    blc_hci_le_cs_security_enable(cs_app_ctrl.cs_ctrl[index].connhandle);
                }
                user_setCsProcStartStatus(index, NULL_EXCH);
                break;
            }

            case SET_PROC_PARAM:
            {
                u8                                       cmdPara[sizeof(hci_le_cs_setProcedureParame_cmdParam_t)] = {0}; //return length is 3
                u8                                       buff[sizeof(hci_le_cs_setProcedureParam_retParam_t)]     = {0}; //return length is 3
                hci_le_cs_setProcedureParame_cmdParam_t *param                                                    = (hci_le_cs_setProcedureParame_cmdParam_t *)cmdPara;

                param->Connection_Handle = cs_app_ctrl.cs_ctrl[index].connhandle;
                param->Config_ID         = cs_app_ctrl.cs_ctrl[index].config_id;
                param->Max_Procedure_Len = 65535;
                param->Max_Procedure_Count = 0; //CS procedures to continue until disabled
        #if (1)
                param->Max_Procedure_Interval = appCsParamSetting.procedureInterval;
                param->Min_Procedure_Interval = appCsParamSetting.procedureInterval;

                param->Min_Subevent_Len[0] = U32_BYTE0(appCsParamSetting.subeventLen);
                param->Min_Subevent_Len[1] = U32_BYTE1(appCsParamSetting.subeventLen);
                param->Min_Subevent_Len[2] = U32_BYTE2(appCsParamSetting.subeventLen);

                param->Max_Subevent_Len[0] = U32_BYTE0(appCsParamSetting.subeventLen);
                param->Max_Subevent_Len[1] = U32_BYTE1(appCsParamSetting.subeventLen);
                param->Max_Subevent_Len[2] = U32_BYTE2(appCsParamSetting.subeventLen);
        #else
                param->Max_Procedure_Interval = 10;
                param->Min_Procedure_Interval = 10;

                param->Min_Subevent_Len[0]              = 0x20;//0x60;
                param->Min_Subevent_Len[1]              = 0x4e;//0x09;
                param->Min_Subevent_Len[2]              = 0x00;

                param->Max_Subevent_Len[0]              = 0x20;//0x60;
                param->Max_Subevent_Len[1]              = 0x4e;//0x09;
                param->Max_Subevent_Len[2]              = 0x00;
        #endif
                param->Tone_Antenna_Config_Selection = 1;
                param->PHY                           = 1;
                param->Tx_Pwr_Delta                  = 0;
                param->Preferred_Peer_Antenna        = 1;


                blc_hci_le_cs_setProcedureParam((hci_le_cs_setProcedureParame_cmdParam_t *)cmdPara,
                                                (hci_le_cs_setProcedureParam_retParam_t *)buff);
        #if UI_CONTROL_ENABLE
                app_parse_printf("Auto cs set cs procedure param\r\n");
        #endif
                user_setCsProcStartStatus(index, NULL_EXCH);
                user_setCsProcCmpltStatus(index, SET_PROC_PARAM_CMPLT);
                break;
            }

            case CS_PROC_EN_EXCH:
            {
                u8                                    pcmd[16] = {0};
                hci_le_cs_enableProcedure_cmdParam_t *p        = (hci_le_cs_enableProcedure_cmdParam_t *)pcmd;

                p->Connection_Handle = cs_app_ctrl.cs_ctrl[index].connhandle;
                p->Config_ID         = cs_app_ctrl.cs_ctrl[index].config_id;
                p->Enable            = 1;

                blc_hci_le_cs_procedureEnable(p);
        #if UI_CONTROL_ENABLE
                app_parse_printf("Auto cs start cs procedure\r\n");
        #endif
                user_setCsProcStartStatus(index, NULL_EXCH);
                break;
            }

            default:
                break;
            }
        }
    }
}
    #endif

/**
 * @brief      BLE CS config complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_config_complete_event_handle(u8 *p)
{
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s\r\n", __FUNCTION__);
    hci_le_csConfigCompleteEvt_t *ptr = (hci_le_csConfigCompleteEvt_t *)p;

    #if (CS_TLK_ALGO2_EN)
    blc_Algo2_CopyConfigCompleteData(&ptr->Main_Mode, sizeof(hci_le_csConfigCompleteEvt_t)-6);
    #endif
    app_cs_config_t *config = blc_getCSConfig(ptr->Connection_Handle, ptr->Config_ID);
    if (ptr->Action == 0x01) {
        //Configuration is to be created
        if (config == NULL) {
            //config didn't exist,create new config
            app_cs_config_t *newCfg   = blc_findUnusedCSConfig();
            if(newCfg != NULL){
                newCfg->Config_ID         = ptr->Config_ID;
                newCfg->Connection_Handle = ptr->Connection_Handle;
                newCfg->Main_Mode         = ptr->Main_Mode;
                newCfg->Sub_Mode          = ptr->Sub_Mode;
                newCfg->Role              = ptr->Role;
                newCfg->RTT_Type          = ptr->RTT_Type;
                newCfg->valid             = TRUE;
            }
            else{
                tlkapi_printf(APP_CS_LOG_EN, "blc_findUnusedCSConfig error!!!\n");
            }
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
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s\r\n", __FUNCTION__);
    hci_le_csProcedureEnableCompleteEvt_t *ptr = (hci_le_csProcedureEnableCompleteEvt_t *)p;

    #if (CS_TLK_ALGO2_EN)
    blc_Algo2_CopyProcedureEnableCompleteData(ptr->Tone_Antenna_Config_Selection, ptr->Selected_TX_Power, ptr->Subevent_Len[0] | (ptr->Subevent_Len[1] << 8) | (ptr->Subevent_Len[2] << 16), ptr->Subevents_Per_Event, ptr->Event_Interval, ptr->Procedure_Interval, ptr->Procedure_Count);
    #endif

    blc_ras_csProcedureEnComplete(ptr); // Inform ras data
}

/**
 * @brief      BLE CS subevent result event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_event_handle(u8 *p)
{
    //tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s\r\n", __FUNCTION__);
    hci_le_csSubeventResultEvt_t *ptr = (hci_le_csSubeventResultEvt_t *)p;

    blc_ras_csSubeventResultData(ptr);
}

/**
 * @brief      BLE CS subevent result continue event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_continue_event_handle(u8 *p)
{
    //tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s\r\n", __FUNCTION__);
    hci_le_csSubeventResultContinueEvt_t *ptr = (hci_le_csSubeventResultContinueEvt_t *)p;

    blc_ras_csSubeventResultContinueData(ptr);
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

void app_print_cs_hci_info_summary_local_remote(u16 rangingCounter, blt_ras_proc_ctrl_t *localProcCtrl, blt_ras_proc_ctrl_t *remoteProcCtrl)
{
    if (appCsParamSetting.printCsHciInforMask) {
        u8 print_hci_info_flag = 0;
        u8 curSubNodeIndex = blc_sniffer_getSubNodeIndexByCsCounter(rangingCounter);
        if (curSubNodeIndex == CS_COUNTER_CONVERT_SUB_NODE_INDEX_INVALID) {
            /* current is main node index */
            if (appCsParamSetting.printCsHciInforMask & BIT(0)) {
                print_hci_info_flag = 1;
            }
        }
        else if (appCsParamSetting.printCsHciInforMask & 0x1E) {
            /* mask is sub node 0~3 */
            if (appCsParamSetting.printCsHciInforMask & BIT(1)) {
                if (curSubNodeIndex == 0) {
                    /* current is sub node 0 */
                    print_hci_info_flag = 1;

                }
            }
            if (appCsParamSetting.printCsHciInforMask & BIT(2)) {
                if (curSubNodeIndex == 1) {
                    /* current is sub node 1 */
                    print_hci_info_flag = 1;

                }
            }
            if (appCsParamSetting.printCsHciInforMask & BIT(3)) {
                if (curSubNodeIndex == 2) {
                    /* current is sub node 2 */
                    print_hci_info_flag = 1;

                }
            }
            if (appCsParamSetting.printCsHciInforMask & BIT(4)) {
                if (curSubNodeIndex == 3) {
                    /* current is sub node 3 */
                    print_hci_info_flag = 1;

                }
            }
        }
        else if (appCsParamSetting.printCsHciInforMask & 0xE0) {
                /* mask is sub node 4~6 */
            if (appCsParamSetting.printCsHciInforMask & BIT(5)) {
                if (curSubNodeIndex == 4) {
                    /* current is sub node 4 */
                    print_hci_info_flag = 1;

                }
            }
            if (appCsParamSetting.printCsHciInforMask & BIT(6)) {
                if (curSubNodeIndex == 5) {
                    /* current is sub node 5 */
                    print_hci_info_flag = 1;

                }
            }
            if (appCsParamSetting.printCsHciInforMask & BIT(7)) {
                if (curSubNodeIndex == 6) {
                    /* current is sub node 6 */
                    print_hci_info_flag = 1;

                }
            }
        }

        if (print_hci_info_flag) {
            int len;
            u8 print_len;
            app_parse_printf("\n{\"title\":\"hci_data\",\"data_init\":\"");
            len = 0;
            while (len < localProcCtrl->proc.dataLen) {
                print_len = 0;
                if ((len + 96) < localProcCtrl->proc.dataLen)
                {
                    print_len = 96;
                }
                else
                {
                    print_len = localProcCtrl->proc.dataLen - len;
                }
                app_parse_printf("%s",hex_buffer_to_hex_string(localProcCtrl->proc.pData + len, print_len));

                len = print_len + len;
            }
            app_parse_printf("\",\"data_reflt\":\"");
            len = 0;
            while (len < remoteProcCtrl->proc.dataLen) {
                print_len = 0;
                if ((len + 96) < remoteProcCtrl->proc.dataLen)
                {
                    print_len = 96;
                }
                else
                {
                    print_len = remoteProcCtrl->proc.dataLen - len;
                }
                app_parse_printf("%s",hex_buffer_to_hex_string(remoteProcCtrl->proc.pData + len, print_len));
                len = print_len + len;
            }
            app_parse_printf("\"}\n");
        }
    }
}

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
    //tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] rangingCounter=%d\r\n", rangingCounter);

    s32 retVal = 0;

    /* 1.check procedure data. delete abort subevent data. restore remote step data(protocol to procedure) */
    blt_ras_proc_ctrl_t remoteProcCtrl;
    blt_ras_proc_ctrl_t localProcCtrl;
    memset(&remoteProcCtrl, 0, sizeof(blt_ras_proc_ctrl_t));
    memset(&localProcCtrl, 0, sizeof(blt_ras_proc_ctrl_t));
    retVal = blc_restoreProcedureData(connHandle, &remoteProcCtrl, &localProcCtrl, pRangingData);
    if (retVal) {
        return retVal;
    }

    /* 2.transmit RAS data to sub node for calculate distance */
    #if (CS_DISTANCE_CALC_SUB_NODE_EN)
        if (nodeSetting.subNodeNumber) {
            u8 curSubNodeIndex = blc_sniffer_getSubNodeIndexByCsCounter(rangingCounter);
            if (curSubNodeIndex != CS_COUNTER_CONVERT_SUB_NODE_INDEX_INVALID) {
                /* current is sub node index */
                //tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] calcRanging, rngCnt=0x%X, localProcDataLen=%d, remoteProcDataLen=%d\r\n", rangingCounter, localProcCtrl.proc.dataLen, remoteProcCtrl.proc.dataLen);

                snif_main_node_cs_ras_client_data_event(rangingCounter, curSubNodeIndex, connHandle, localProcCtrl.proc.dataLen, localProcCtrl.proc.pData);
                snif_main_node_cs_ras_server_data_event(rangingCounter, curSubNodeIndex, connHandle, remoteProcCtrl.proc.dataLen, remoteProcCtrl.proc.pData);

                //return retVal;
                goto print_cs_hci_info;
            }
        }
    #endif

    /* 3.calculate main node distance */
    blt_ras_proc_ctrl_t *localProcedure = blc_getLocalProcedureData(connHandle, rangingCounter);
    app_cs_config_t     *cfg            = blc_getCSConfig(connHandle, localProcedure->procedureHead.data.proCountCfgID);
    if((cfg == NULL) || (localProcedure == NULL)){
        return CS_DIST_ERR_RAS_RANGING_DATA_WRONG;
    }

    if (cfg->Role == INITIATOR_ROLE) {
        retVal = csCalculateDistance(connHandle, &localProcCtrl, &remoteProcCtrl, cfg->Main_Mode, distance);
    } else {
        retVal = csCalculateDistance(connHandle, &remoteProcCtrl, &localProcCtrl, cfg->Main_Mode, distance);
    }

print_cs_hci_info:
    /* 4. output HCI information for the current node summarizing local and remote */
    app_print_cs_hci_info_summary_local_remote(rangingCounter, &localProcCtrl, &remoteProcCtrl);

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
        // Auto choose real-time or on-demand ranging data.
        svc_ras_feature_t *feature = NULL;
        ble_sts_t          ret     = blc_rasc_getRasFeature(connHandle, &feature);

        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] RAS service has been found:0x%x, realTimeSupport=%d\r\n", connHandle, feature->realTimeProcedureDataSupport);

        if (ret == BLE_SUCCESS) {
            if (feature->realTimeProcedureDataSupport) {
                blc_rapc_writeRealtimeCcc(connHandle, 0x01, NULL);
            } else {
                blc_rapc_writeOnDemandCcc(connHandle, 0x01, NULL);
            }

            #if CS_PROCEDURE_EXCHANGE
                u8 index = 0;
                if (user_addCsCtrlByHadle(connHandle)) {
                    index = user_getCsCtrlByHadle(connHandle);
                    if (index == 0) {
                        //err
                        tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] get cs ctrl idx error.\r\n");
                    } else {
                        index = index - 1;
                    }
                } else {
                    //error,buffer not enough
                    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] add cs ctrl error.\r\n");
                }

                extern u16 app_cs_distance_curConnHandle;
                if (!app_cs_distance_curConnHandle) {
                    cs_app_ctrl.cs_ctrl[index].exchange_tick = clock_time() | 1;
                    user_setCsProcStartStatus(index, CAP_EXCH);
                    cs_app_ctrl.cs_ctrl[index].acl_role = dev_char_get_conn_role_by_connhandle(connHandle);

                    #if UI_CONTROL_ENABLE
                        app_parse_printf("wait cs capability exchange:0x%x\r\n", connHandle);
                    #endif
                    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] trigger cs_procedure_ctrl, wait cs capability exchange:0x%x\r\n", connHandle);
                }
                else {
                    #if UI_CONTROL_ENABLE
                        app_parse_printf("other connection 0x%x are being ranging, current connection 0x%x does not trigger ranging\r\n", app_cs_distance_curConnHandle, connHandle);
                    #endif
                    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] other connection 0x%x are being ranging, current connection 0x%x does not trigger ranging\r\n", app_cs_distance_curConnHandle, connHandle);
                }
            #endif
        }
    }

    return 0;
}

float lastValidDist     = 0;
float lastValidFiltDist = 0;

/**
 * @brief      CS procedure data ready to calculate distance.
 * @param[in]  connHandle       ACL connect handle
 * @param[in]  *pData           ranging data
 * @param[in]  dataLen          length of data
 * @return     0x00
 */
static int app_cs_procedure_data(u16 connHandle, u8 *pData, u16 dataLen)
{
    (void)dataLen;

    DBG_SNIF_CHN9_HIGH;

    blc_rasc_ranging_data_evt_t *rangingData    = (blc_rasc_ranging_data_evt_t *)pData;
    u16                          rangingCounter = rangingData->rangingCounter;

    float distance1       = 0;
    float distance2       = 0;
    float dis1_filt_first = 0;
    float dis2_filt_first = 0;
    float dis1_filt       = 0;
    float dis2_filt       = 0;

    if (dev_char_info_is_connection_state_by_conn_handle(connHandle) == FALSE) {
        /* currently connHandle not in the connected state */
        return 0;
    }

    float distance[CS_DISTANCE_TYPE_SUPPORT_MAX] = {0.0f};
    extern s32 blc_calcRangData(u16 connHandle, u8 * pRangingData, float *distance);
    s32        retval = blc_calcRangData(connHandle, pData, distance);

    if (retval == CS_DIST_SUCCESS) {
        #if (1)
            #if (CS_DISTANCE_CALC_SUB_NODE_EN)
                u8 curSubNodeIndex = blc_sniffer_getSubNodeIndexByCsCounter(rangingCounter);
                if (curSubNodeIndex == CS_COUNTER_CONVERT_SUB_NODE_INDEX_INVALID) {
                    /* current is main node index */
                    snif_main_node_cs_distacne_process(connHandle, rangingCounter, distance[0], distance[1], distance[2]);
                }
            #else
                snif_main_node_cs_distacne_process(connHandle, rangingCounter, distance[0], distance[1], distance[2]);
            #endif
        #else
            #if (MEDIAN_FILTER_ENABLE) // MEDIAN_FILTER_ENABLE
        dis1_filt = medianFilterRealTime(distance1, medianWin, &median_count, MEDIAN_WIN_SIZE);
        dis2_filt = medianFilterRealTime(distance2, medianWin, &median_count, MEDIAN_WIN_SIZE);
            #endif


            #if (KALMAN_FILTER_ENABLE)     // KALMAN_FILTER_ENABLE

        if (isnan(distance1)) {
            distance1 = lastValidDist;
        } else {
            lastValidDist = distance1;
        }

        dis1_filt_first = filtFirst(distance1);
        dis2_filt_first = filtFirst(distance2);

        dis1_filt = kalmanFilter_update(kf1, dis1_filt_first);
        dis2_filt = kalmanFilter_update(kf2, dis2_filt_first);

        if (isnan(dis1_filt)) {
            dis1_filt = lastValidFiltDist;
        } else {
            lastValidFiltDist = dis1_filt;
        }
            #endif
        //        tlkapi_printf(1, "dis: %f, %f; dis filter1: %f, %f; kalman: %f, %f",
        //                distance1,distance2,dis1_filt_first,dis2_filt_first,dis1_filt, dis2_filt);

        tlkapi_printf(APP_CS_LOG_EN, "Phase: %f, dis filter1: %f; kalman: %f\r\n", distance1, dis1_filt_first, dis1_filt);

        tlkapi_printf(APP_CS_LOG_EN, "MUSIC: %f, dis filter2: %f; kalman: %f\r\n", distance2, dis2_filt_first, dis2_filt);

        #endif
    } else {
        if (retval == CS_DIST_ERR_STEPS_NUMS_ZEROS) {
            tlkapi_send_string_u32s(APP_CS_LOG_EN, "[APP][CS] distance error, steps number zero", retval);
        #if UI_CONTROL_ENABLE
            app_parse_printf("distance error, steps number zero\r\n");
        #endif
        } else if (retval == CS_DIST_ERR_STEPS_NUM_NOT_ENOUGH) {
            tlkapi_send_string_u32s(APP_CS_LOG_EN, "[APP][CS] distance error, steps number not enough", retval);
        #if UI_CONTROL_ENABLE
            app_parse_printf("distance error, steps number not enough\r\n");
        #endif
        } else if (retval == CS_DIST_ERR_RAS_RANGING_DATA_WRONG) {
            tlkapi_send_string_u32s(APP_CS_LOG_EN, "[APP][CS] distance error, ranging data error", retval);
        #if UI_CONTROL_ENABLE
            app_parse_printf("distance error, ranging data error\r\n");
        #endif
        } else if (retval == CS_DIST_ERR_ALGO_MASK_NOT_SET) {
            tlkapi_send_string_u32s(APP_CS_LOG_EN, "distance error, ranging data error", retval);
        #if UI_CONTROL_ENABLE
            app_parse_printf("distance error, algorithm mask not set\r\n");
        #endif
        } else {
            tlkapi_send_string_u32s(APP_CS_LOG_EN, "[APP][CS] distance error, unknown reason", retval);
        #if UI_CONTROL_ENABLE
            app_parse_printf("distance error, unknown reason\r\n");
        #endif
        }
    }
    DBG_SNIF_CHN9_LOW;
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

    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] RAS timeout for conn:%02X", connHandle);
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
    {PRF_EVTID_CLIENT_SDP_END, app_prf_sdp_end             },
    {CS_EVT_RANGING_DATA,      app_cs_procedure_data       },
    {CS_EVT_OVERWRITTEN,       app_cs_procedure_overwritten},
    {CS_EVT_TIMEOUT,           app_cs_procedure_timeout    },
};
PRF_EVT_CB(csCentralEvt)
#define HOST_MALLOC_BUFF_SIZE      ((RAS_PROCEDURE_COUNT * 2 + 1) * PROCEDURE_DATA_LEN + 4 * 1024)

static u8 hostMallocBuffer[HOST_MALLOC_BUFF_SIZE];

/**
 * @brief      BLE channel sounding initialize.
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

        .Roles_Supported                   = CS_ROLE_DISABLE,
        .Mode_Types                        = 0,   //mandatory mode1 and mode 2
        .RTT_Capability                    = 0,   //150ns
        .RTT_AA_Only_N                     = 240,
        .RTT_Sounding_N                    = 240,
        .RTT_Random_Payload_N              = 240,
        .Optional_NADM_Sounding_Capability = 0,
        .Optional_NADM_Random_Capability   = 0,
        .Optional_CS_SYNC_PHYs_Supported   = 0 | BIT(1),  //just mandatory 1M PHY
        .Optional_Subfeatures_Supported    = 0,
        .Optional_T_IP1_Times_Supported    = 0,  //only support 145us
        .Optional_T_IP2_Times_Supported    = 0,  //only support 145us
        .Optional_T_FCS_Times_Supported    = 0,  //only support 150us
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
    //Initialize multi-antenas
    cs_ant_switch_config_t ant_cfg = {
        .ant_default_seq_value = 0,
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

    //Set cs use tx power level
    blc_cs_set_tx_power_level(CS_USE_TX_POWER_LEVEL);

    //Initialize RAS client
    blc_prf_initialModule(app_prf_eventCb, hostMallocBuffer, HOST_MALLOC_BUFF_SIZE);
    blc_rap_registerRasProfileControlClient(NULL);

    //Init cs ranging log
    blc_cs_initRangingLog();
}

#endif
