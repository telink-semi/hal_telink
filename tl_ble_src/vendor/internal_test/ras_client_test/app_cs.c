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

#if (INTER_TEST_MODE == TEST_RAS_CLIENT)

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

app_cs_config_t app_cs_config[APP_CS_CONFIG_NUM]      = {0};


#if (CS_DISTANCE_FILTER)
app_filter_ctrl filter_ctrl[MAX_DISTANCE_CNT_SUPPORT] = {0};
#endif

//for iop purposes, so that consecutive executions have different ranging counters
static u16 iopRangingCounter = 0; //0x205;
static u16 iopConnHandle = 0;

#if (CS_SW_FIXED_OFFSET)
#define FLASH_ADDRESS_FIXED_OFFSET                 0xB0000
typedef struct __attribute__((packed))
{
    s16 read_offset_algo1;
    s16 read_offset_algo2;
    float offset_algo1;
    float offset_algo2;
} app_cs_fixed_offset_t;
app_cs_fixed_offset_t app_cs_fixed_offset = {
    .read_offset_algo1 = 0,
    .read_offset_algo2 = 0,
    .offset_algo1 = 0.5,
    .offset_algo2 = 0.5,
};

static void app_cs_load_fixed_offset_from_flash(void)
{
    app_cs_fixed_offset_t read_offset = {0};
    flash_read_page(FLASH_ADDRESS_FIXED_OFFSET, sizeof(s16)*2, (u8 *)&read_offset);
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] Read offset from flash, algo1 offset:%dcm, algo2 offset:%dcm",
                  read_offset.read_offset_algo1, read_offset.read_offset_algo2);
    if(abs(read_offset.read_offset_algo1) <= 500 && read_offset.read_offset_algo1 != -1){
        app_cs_fixed_offset.offset_algo1 = read_offset.read_offset_algo1/100.0f;
    }
    if(abs(read_offset.read_offset_algo2) <= 500 && read_offset.read_offset_algo2 != -1){
        app_cs_fixed_offset.offset_algo2 = read_offset.read_offset_algo2/100.0f;
    }
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] Use fixed offset algo1 offset:%f, algo2 offset:%f",
                  app_cs_fixed_offset.offset_algo1, app_cs_fixed_offset.offset_algo2);
}
#endif

#if CS_PROCEDURE_EXCHANGE

cs_app_control_t cs_app_ctrl;

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
            #if (AUTO_CALIB_CHIP_INTERNAL_DELAY_EN)
                    app_parse_printf("if need to calibrate chip internal delay,please set <cs calib [mode_type]>, mode_type include 2/1");
            #endif

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
        #if (AUTO_CALIB_CHIP_INTERNAL_DELAY_EN)
                    if (cs_app_ctrl.cs_ctrl[index].chipDly_calib_en) {
                        chipDly_cs_procedure_param(cs_app_ctrl.cs_ctrl[i].chipDly_max_proc_cnt);

                        app_parse_printf("calibration: cs set cs procedure param\r\n");
                        //////////////////////////////////////////////////
                        u8                                    pcmd[16] = {0};
                        hci_le_cs_enableProcedure_cmdParam_t *p        = (hci_le_cs_enableProcedure_cmdParam_t *)pcmd;

                        p->Connection_Handle = cs_app_ctrl.cs_ctrl[0].connhandle;
                        p->Config_ID         = cs_app_ctrl.cs_ctrl[0].config_id;
                        p->Enable            = 1;

                        blc_hci_le_cs_procedureEnable(p);
                        app_parse_printf("calibration: cs start cs procedure\r\n");
                    } else
        #endif
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
            } break;
            case SET_DEFAULT:
            {
                u8                                      pcmd[sizeof(hci_le_cs_setDefaultSetting_cmdParam_t)] = {0};
                u8                                      pret[sizeof(hci_le_cs_setDefaultSetting_retParam_t)] = {0};
                hci_le_cs_setDefaultSetting_cmdParam_t *p                                                    = (hci_le_cs_setDefaultSetting_cmdParam_t *)pcmd;

                p->Connection_Handle         = cs_app_ctrl.cs_ctrl[index].connhandle;
                p->Role_Enable               = CS_INITIATOR_ROLE; //initiator role en
                p->Max_TX_Power              = 0;                 //0dbm
                p->CS_SYNC_Antenna_Selection = 1;

                blc_hci_le_cs_setDefaultSettings(p, (hci_le_cs_setDefaultSetting_retParam_t *)pret);

                user_setCsProcStartStatus(index, NULL_EXCH);
                user_setCsProcCmpltStatus(index, SET_DFT_CMPLT);

            } break;
            case FAE_EXCH:
            {
                if (cs_app_ctrl.cs_ctrl[index].exchange_tick && clock_time_exceed(cs_app_ctrl.cs_ctrl[index].exchange_tick, 100 * 1000)) {
                    blc_hci_le_cs_readRemoteFAE_table(cs_app_ctrl.cs_ctrl[index].connhandle);
                    user_setCsProcStartStatus(index, NULL_EXCH);
                    cs_app_ctrl.cs_ctrl[index].exchange_tick = 0;
                }
            } break;
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
                    p->Mode_0_Steps         = 2;    //2
                    p->Role                 = CS_CONFIG_INITIATOR_ROLE;
                    p->RTT_Type             = 0x00; //RTT CS Access Address only timing
                    p->CS_SYNC_PHY          = 0x01; //LE 1M PHY
                    /*
                         * Channels n = 0, 1, 23, 24, 25, 77, and 78 shall be ignored and shall
                         *  be set to zero. At least 15 channels shall be enabled.
                         */
                    p->Channel_Map[0]          = 0xfc; // 0~7
                    p->Channel_Map[1]          = 0xff; // 8~15
                    p->Channel_Map[2]          = 0x7f; // 16~23
                    p->Channel_Map[3]          = 0xfc; // 24~31
                    p->Channel_Map[4]          = 0xff; // 32~39
                    p->Channel_Map[5]          = 0xff; // 40~47
                    p->Channel_Map[6]          = 0xff; // 48~55
                    p->Channel_Map[7]          = 0xff; // 56~63
                    p->Channel_Map[8]          = 0xff; // 64~71
                    p->Channel_Map[9]          = 0x1f; // 72~79
                    p->Channel_Map_Repetition  = 1;
                    p->ChSel                   = 0;    //Use Channel Selection Algorithm #3b for non-mode 0 CS steps
                    p->Ch3c_Shape              = 0;    //Use Hat shape for user-specified channel sequence
                    p->Ch3c_Jump               = 0x02; //Number of channels skipped in each rising and falling sequence
                    p->Companion_Signal_Enable = 0;    //Companion Signal disabled

                    blc_hci_le_cs_createConfig(p);


                    user_setCsProcStartStatus(index, NULL_EXCH);
                    cs_app_ctrl.cs_ctrl[index].exchange_tick = 0;
                }
            } break;
            case SEC_EXCH:
            {
                if (cs_app_ctrl.cs_ctrl[index].acl_role == ACL_ROLE_CENTRAL) {
                    blc_hci_le_cs_security_enable(cs_app_ctrl.cs_ctrl[index].connhandle);
                }
                user_setCsProcStartStatus(index, NULL_EXCH);
            } break;
            case SET_PROC_PARAM:
            {
                u8                                       cmdPara[sizeof(hci_le_cs_setProcedureParame_cmdParam_t)] = {0}; //return length is 3
                u8                                       buff[sizeof(hci_le_cs_setProcedureParam_retParam_t)]     = {0}; //return length is 3
                hci_le_cs_setProcedureParame_cmdParam_t *param                                                    = (hci_le_cs_setProcedureParame_cmdParam_t *)cmdPara;

                param->Connection_Handle      = cs_app_ctrl.cs_ctrl[index].connhandle;
                param->Config_ID              = cs_app_ctrl.cs_ctrl[index].config_id;
                param->Max_Procedure_Len      = 65535;
                param->Max_Procedure_Interval = 20;
                param->Min_Procedure_Interval = 20;
                param->Max_Procedure_Count    = 0;    //need >1
                param->Min_Subevent_Len[0]    = 0x60; //0x00;
                param->Min_Subevent_Len[1]    = 0xEA; //0x10;
                param->Min_Subevent_Len[2]    = 0x00;

                param->Max_Subevent_Len[0] = 0x60;    //0x00;
                param->Max_Subevent_Len[1] = 0xEA;    //0x30;
                param->Max_Subevent_Len[2] = 0x00;

                param->Tone_Antenna_Config_Selection = 1;
                param->PHY                           = 1;
                param->Tx_Pwr_Delta                  = 0;
                param->Preferred_Peer_Antenna        = 1;


                blc_hci_le_cs_setProcedureParam((hci_le_cs_setProcedureParame_cmdParam_t *)cmdPara,
                                                (hci_le_cs_setProcedureParam_retParam_t *)buff);


                user_setCsProcStartStatus(index, NULL_EXCH);
                user_setCsProcCmpltStatus(index, SET_PROC_PARAM_CMPLT);
            } break;
            case CS_PROC_EN_EXCH:
            {
                u8                                    pcmd[16] = {0};
                hci_le_cs_enableProcedure_cmdParam_t *p        = (hci_le_cs_enableProcedure_cmdParam_t *)pcmd;

                p->Connection_Handle = cs_app_ctrl.cs_ctrl[index].connhandle;
                p->Config_ID         = cs_app_ctrl.cs_ctrl[index].config_id;
                p->Enable            = 1;

                blc_hci_le_cs_procedureEnable(p);
                user_setCsProcStartStatus(index, NULL_EXCH);
            } break;
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
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s", __FUNCTION__);
    hci_le_csConfigCompleteEvt_t *ptr = (hci_le_csConfigCompleteEvt_t *)p;

    #if (CS_TLK_ALGO2_EN)
    blc_Algo2_CopyConfigCompleteData(&ptr->Main_Mode, sizeof(hci_le_csConfigCompleteEvt_t));
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
    blc_Algo2_CopyProcedureEnableCompleteData(ptr->Tone_Antenna_Config_Selection, ptr->Selected_TX_Power, ptr->Subevent_Len[0] | (ptr->Subevent_Len[1] << 8) | (ptr->Subevent_Len[2] << 16), ptr->Subevents_Per_Event, ptr->Event_Interval, ptr->Procedure_Interval, ptr->Procedure_Count);
    #endif

    if((iopConnHandle) != 0) {
        ptr->Connection_Handle = iopConnHandle;
    }

    blc_ras_csProcedureEnComplete(ptr); // Inform ras data
}

/**
 * @brief      BLE CS subevent result event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_event_handle(u8 *p)
{
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s", __FUNCTION__);
    hci_le_csSubeventResultEvt_t *ptr = (hci_le_csSubeventResultEvt_t *)p;

    // ptr->Connection_Handle = 0x40;
    //TODO: if you use controller input, only use controller. Controller subevent input will get overwritten if any rass <index> get input first.
    if((iopConnHandle) != 0) {
        ptr->Connection_Handle = iopConnHandle;
    }
    if((iopRangingCounter) != 0) {
        ptr->Procedure_Counter = iopRangingCounter;
    }

    blc_ras_csSubeventResultData(ptr);
}

/**
 * @brief      BLE CS subevent result continue event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_continue_event_handle(u8 *p)
{
    tlkapi_printf(APP_CS_LOG_EN, "[APP][CS] %s", __FUNCTION__);
    hci_le_csSubeventResultContinueEvt_t *ptr = (hci_le_csSubeventResultContinueEvt_t *)p;

    // ptr->Connection_Handle = 0x40;
    //TODO: if you use controller input, only use controller. Controller subevent input will get overwritten if any rass <index> get input first.
    if((iopConnHandle) != 0) {
        ptr->Connection_Handle = iopConnHandle;
    }
    blc_ras_csSubeventResultContinueData(ptr);
}

#if (AUTO_CALIB_CHIP_INTERNAL_DELAY_EN)
void chipDly_cs_procedure_param(u8 max_proc_cnt)
{
    u8                                       cmdPara[sizeof(hci_le_cs_setProcedureParame_cmdParam_t)] = {0}; //return length is 3
    u8                                       buff[sizeof(hci_le_cs_setProcedureParam_retParam_t)]     = {0}; //return length is 3
    hci_le_cs_setProcedureParame_cmdParam_t *param                                                    = (hci_le_cs_setProcedureParame_cmdParam_t *)cmdPara;

    param->Connection_Handle      = cs_app_ctrl.cs_ctrl[0].connhandle;
    param->Config_ID              = cs_app_ctrl.cs_ctrl[0].config_id;
    param->Max_Procedure_Len      = 65535;
    param->Max_Procedure_Interval = 20;
    param->Min_Procedure_Interval = 20;

    param->Max_Procedure_Count = max_proc_cnt;
    param->Min_Subevent_Len[0] = 0x10;        //0x60;
    param->Min_Subevent_Len[1] = 0x0e;        //0x09;
    param->Min_Subevent_Len[2] = 0x00;

    param->Max_Subevent_Len[0] = 0x10;        //0x60;
    param->Max_Subevent_Len[1] = 0x0e;        //0x09;
    param->Max_Subevent_Len[2] = 0x00;

    param->Tone_Antenna_Config_Selection = 1;
    param->PHY                           = 1;
    param->Tx_Pwr_Delta                  = 0;
    param->Preferred_Peer_Antenna        = 1;


    blc_hci_le_cs_setProcedureParam((hci_le_cs_setProcedureParame_cmdParam_t *)cmdPara,
                                    (hci_le_cs_setProcedureParam_retParam_t *)buff);
}

void chipDly_initiator_calc_cfo(u16 connHandle, float cs_cfo)
{
    for (int i = 0; i < CS_MAX_NUM; i++) {
        if (!cs_app_ctrl.cs_ctrl[i].connhandle || !cs_app_ctrl.cs_ctrl[i].chipDly_calib_en) {
            continue;
        }

        if (connHandle != cs_app_ctrl.cs_ctrl[i].connhandle) {
            continue;
        }

        if (cs_app_ctrl.cs_ctrl[i].chipDly_calib_CFO_cmplt) {
            continue;
        }

        if (!cs_app_ctrl.cs_ctrl[i].chipDly_cfo_1stStep_flag) { //0 indicate the first mode0 step

            cs_app_ctrl.cs_ctrl[i].chipDly_cfo_1stStep_flag = 1;
            cs_app_ctrl.cs_ctrl[i].chipDly_initCalcCfoVal   = cs_cfo;
        } else {
            cs_app_ctrl.cs_ctrl[i].chipDly_initCalcCfoVal += cs_cfo;
            cs_app_ctrl.cs_ctrl[i].chipDly_initCalcCfoVal /= 2;
        }
    }
}

_attribute_ram_code_ void chipDly_set_chnIdx(u8 *chnIdx)
{
    if (cs_app_ctrl.cs_ctrl[0].chipDly_calib_en) {
        for (int i = 0; i < 79; i++) {
            chnIdx[i] = i; //0~78 indicate [2402,2480] 79 channels.
        }
    }
}
#endif

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
        filter_ctrl[i].kf.state          = 0.0;
        filter_ctrl[i].kf.err_cov        = 1.0;
        filter_ctrl[i].kf.proc_noise_cov = 0.001;
        filter_ctrl[i].kf.msr_noise_cov  = 0.01;
        filter_ctrl[i].kf.kal_gain       = 0.0;
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
        // if (ret == BLE_SUCCESS) {
        //     if (feature->realTimeProcedureDataSupport) {
        //         blc_rapc_writeRealtimeCcc(connHandle, 0x01, NULL);
        //     } else {
        //         blc_rapc_writeOnDemandCcc(connHandle, 0x01, NULL);
        //     }
        // }

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
            cs_app_ctrl.cs_ctrl[index].exchange_tick = clock_time() | 1;
            user_setCsProcStartStatus(index, CAP_EXCH);
            cs_app_ctrl.cs_ctrl[index].acl_role = dev_char_get_conn_role_by_connhandle(connHandle);
                #if UI_CONTROL_ENABLE
            app_parse_printf("wait cs capability exchange\r\n");
                #endif
        #endif
    }
    return 0;
}

////////////////// algo1 and algo2 switch function ///////////////////////
// note: default use algo2 when reset mcu, if continue 4 times distance calculate with algo2, system will enable algo1
//       use algo1 distance and must reset distance filter machine, please remind that algo2 should still keep calculating.
//       After algo1 and algo2 both calculate, check if algo2 continue 4times get distance then disable algo1 and default use
//       algo2 distance and update distance filter machine.

#define    CS_ALGO_AUTO_SWITCH    0

#if(CS_ALGO_AUTO_SWITCH)
u8 algo2_dist_mark_cnt = 0;
u8 algo1_enable = 0;

static void blc_cs_algoAutoSwitch(s32 retval)
{
    #define    ALGO_SWTICH_MAX_ABORT_COUNT    4 // This macro decide how many times with algo2 distance error/correct to switch algo
    #define    ALGO2_CALC_SUCCESS             0 // Calculate distance with distance2 success
    extern void app_parse_printf(const char *format, ...);
    // todo: consider indoor distance test, never use algo1 in indoor env
    if(algo1_enable == 0) {
        if(retval != ALGO2_CALC_SUCCESS) {
            app_parse_printf("[algo1 disable,algo2 fail,mark add 1]\r\n");
            algo2_dist_mark_cnt++;
        } else {
            app_parse_printf("[algo1 disable,algo2 fine,mark clean]\r\n");
            algo2_dist_mark_cnt = 0;
        }
    }
    if(algo1_enable == 1) {
        if(retval == ALGO2_CALC_SUCCESS) {
            app_parse_printf("[algo1 enable,algo2 fine,mark add 1]\r\n");
            algo2_dist_mark_cnt++;
        } else {
            app_parse_printf("[algo1 enable,algo2 fail,mark clean]\r\n");
            algo2_dist_mark_cnt = 0;
        }
    }

    if(algo2_dist_mark_cnt >= ALGO_SWTICH_MAX_ABORT_COUNT) {
        if(algo1_enable) { // algo1 -> algo2, only use algo2 now.
            algo1_enable = 0;
            blc_cs_removeAlgoMask(BLC_RANGING_ALGORITHM_1);
            app_parse_printf("[marker cnt over max(%d),switch algo,only using algo2 now]\r\n",algo2_dist_mark_cnt); // lambda
        }
        else{ // algo2->algo1, use algo1 distance, but algo2 keep running
            algo1_enable = 1;
            blc_cs_addAlgoMask(BLC_RANGING_ALGORITHM_1);
            app_parse_printf("[marker cnt over max(%d),switch algo,only using algo1 now]\r\n",algo2_dist_mark_cnt); // phase
        }
        algo2_dist_mark_cnt = 0;
        app_rangingFilter_init();
    }

    if(algo1_enable) {
        gpio_write(GPIO_LED_WHITE,0);
        gpio_write(GPIO_LED_GREEN,1);
    } else {
        gpio_write(GPIO_LED_WHITE,1);
        gpio_write(GPIO_LED_GREEN,0);
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

    float distance[MAX_DISTANCE_CNT_SUPPORT] = {0.0f};
    u8    idx                                = 0;
    s32   retval                             = blc_calcRangData(connHandle, pRangingData, distance);
#if(CS_ALGO_AUTO_SWITCH)
    blc_cs_algoAutoSwitch(retval); // cs algo1/2 auto switch
    float filter_dist[MAX_DISTANCE_CNT_SUPPORT] = {0.0f};
    for (int i = 0; i < MAX_DISTANCE_CNT_SUPPORT; i++) {
        app_rangingFilter(&filter_ctrl[i], distance[i], &filter_dist[i]);
        tlkapi_printf(1, "dist[%d]: %f, kalman: %f", i, distance[i], filter_dist[i]);
    }
    if(algo1_enable){
        app_parse_printf("{\"title\":\"cs_dist\",\"unfiltered-dist\":{\"dist\":[%.1f]},\"filtered-dist\":{\"dist\":[%.1f]}}\r\n",
                         distance[idx],
                         filter_dist[idx++]); // in this situation, we need use algo1 distance
    }
    else{
        if(retval == ALGO2_CALC_SUCCESS) {
            app_parse_printf("{\"title\":\"cs_dist\",\"unfiltered-dist\":{\"dist\":[%.1f]},\"filtered-dist\":{\"dist\":[%.1f]}}\r\n",
                             distance[idx],
                             filter_dist[idx++]); // in this situation, we need use algo1 distance
        } else { // algo1 not enable and algo2 calculate fail
            app_parse_printf("ALGO2 Calculate Failed,Error Code: %d\r\n",retval);
        }
    }

#else
    if (retval == CS_DIST_SUCCESS) {
#if (CS_DISTANCE_FILTER)
        float filter_dist[MAX_DISTANCE_CNT_SUPPORT] = {0.0f};
        for (int i = 0; i < MAX_DISTANCE_CNT_SUPPORT; i++) {
            if(distance[i] == 0.0f) continue;
            #if (CS_SW_FIXED_OFFSET)
            float offset = app_cs_fixed_offset.offset_algo2;
            float random_dist = (trng_rand() % 10 + 10) / 100.0f;
            distance[i] = distance[i] - offset > 0.0f ? distance[i] - offset : random_dist;
            #endif
            app_rangingFilter(&filter_ctrl[i], distance[i], &filter_dist[i]);
            tlkapi_printf(1, "dist[%d]: %f, kalman: %f", i, distance[i], filter_dist[i]);
        }
        app_parse_printf("{\"title\":\"cs_dist\",\"unfiltered-dist\":{\"dist\":[%.1f]},\"filtered-dist\":{\"dist\":[%.1f]}}\r\n",
                         distance[idx],
                         filter_dist[idx++]);
#else
        app_parse_printf("{\"title\":\"cs_dist\",\"dist\":[%.1f]}\r\n", distance[idx++]);
#endif
    } else {
        if (retval == CS_DIST_ERR_STEPS_NUMS_ZEROS) {
            tlkapi_send_string_u8s(1, "distance error, steps number zero", retval);
            app_parse_printf("distance error, steps number zero\r\n");
        } else if (retval == CS_DIST_ERR_STEPS_NUM_NOT_ENOUGH) {
            tlkapi_send_string_u8s(1, "distance error, steps number not enough", retval);
            app_parse_printf("distance error, steps number not enough\r\n");
        } else if (retval == CS_DIST_ERR_RAS_RANGING_DATA_WRONG) {
            tlkapi_send_string_u8s(1, "distance error, ranging data error", retval);
            app_parse_printf("distance error, ranging data error\r\n");
        } else if (retval == CS_DIST_ERR_ALGO_MASK_NOT_SET) {
            tlkapi_send_string_u8s(1, "distance error, ranging data error", retval);
            app_parse_printf("distance error, algorithm mask not set\r\n");
        } else {
            tlkapi_send_string_u8s(1, "distance error, unknown reason", retval);
            app_parse_printf("distance error, unknown reason\r\n");
        }
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

    tlkapi_printf(1, "[APP][CS] RAS timeout for conn:%02X", connHandle);
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

#if (RAS_LOGIC_MANUAL)
/**
 * @brief      Remote procedure data ready handler.
 * @param[in]  connHandle ACL connect handle
 * @param[in]  *pData     Pointer to data ready event data buffer
 * @param[in]  dataLen    length of data
 * @return     0x00
 */
static int app_cs_procedure_data_ready(u16 connHandle, u8 *pData, u16 dataLen)
{
    blc_ras_data_ready_evt_t *dataReadyEvent = (blc_ras_data_ready_evt_t *)pData;
    (void)dataLen;

    return blt_rapc_writeGetSpecificRecord(dataReadyEvent->connHandle, dataReadyEvent->rangingCounter, NULL);
}

/**
 * @brief      Remote procedure lost segments handler.
 * @param[in]  connHandle ACL connect handle
 * @param[in]  *pData     Pointer to lost segments event data buffer
 * @param[in]  dataLen    length of data
 * @return     0x00
 */
static int app_cs_procedure_lost_segments(u16 connHandle, u8 *pData, u16 dataLen)
{
    blc_ras_lost_segments_evt_t *lostSegmentsEvent = (blc_ras_lost_segments_evt_t *)pData;
    (void)dataLen;
    //should also check ras feature before sending
    return blt_rapc_writeGetRecordSegments(lostSegmentsEvent->connHandle, lostSegmentsEvent->rangingCounter, lostSegmentsEvent->segmentStart, lostSegmentsEvent->segmentEnd, NULL);
}

/**
 * @brief      Remote procedure ack handler.
 * @param[in]  connHandle ACL connect handle
 * @param[in]  *pData     Pointer to procedure ack event data buffer
 * @param[in]  dataLen    length of data
 * @return     0x00
 */
static int app_cs_procedure_ack(u16 connHandle, u8 *pData, u16 dataLen)
{
    blc_ras_procedure_ack_evt_t *procedureAckEvent = (blc_ras_procedure_ack_evt_t *)pData;
    (void)dataLen;

    return blt_rapc_writeAckSpecificRecord(procedureAckEvent->connHandle, procedureAckEvent->rangingCounter, NULL);
}
#endif

static const app_prf_evtCb_t csCentralEvt[] = {
    {PRF_EVTID_CLIENT_SDP_END, app_prf_sdp_end               },
    {CS_EVT_RANGING_DATA,      app_cs_procedure_data         },
    {CS_EVT_OVERWRITTEN,       app_cs_procedure_overwritten  },
    {CS_EVT_TIMEOUT,           app_cs_procedure_timeout      },
#if (RAS_LOGIC_MANUAL)
    {CS_EVT_DATA_READY,        app_cs_procedure_data_ready   },
    {CS_EVT_LOST_SEGMENTS,     app_cs_procedure_lost_segments},
    {CS_EVT_PROCEDURE_ACK,     app_cs_procedure_ack          },
#endif
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

        .Roles_Supported                   = CS_INITIATOR_ROLE,
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
    rf_ant_switch_init_t ant_cfg = {
        .antsel0_pin = ANTENNA_SWITCHING_SEL_0_PIN,
        .antsel1_pin = ANTENNA_SWITCHING_SEL_1_PIN,
        .antsel2_pin = ANTENNA_SWITCHING_SEL_2_PIN,
        .ant_default_seq_value = 0,
        .ant_ctrl_seq_base_value = ANTENNA_SWITCHING_CTRL_BASE,
    };
    blc_antenna_switch_init(&ant_cfg);
#endif

    //Initialize RAS client
    blc_prf_initialModule(app_prf_eventCb, hostMallocBuffer, HOST_MALLOC_BUFF_SIZE);
    blc_rap_registerRasProfileControlClient(NULL);

#if (CS_DISTANCE_FILTER)
    //Initialize distance filter
    app_rangingFilter_init();
#endif

    //Enable distance calculate algorithm, see blc_ranging_algorithm_enum.
    blc_cs_enableAlgoMask(BLC_RANGING_ALGORITHM_2);

#if (CS_SW_FIXED_OFFSET)
    //Load distance fixed offset.
    app_cs_load_fixed_offset_from_flash();
#endif
}

void app_le_cs_iop_rangingCounterIncrease(void)
{
    iopRangingCounter++;
}

void app_le_cs_iop_setConnectionHandle(u16 connHandle)
{
    iopConnHandle = connHandle;
}

u16 app_le_cs_iop_getConnectionHandle(void)
{
    return iopConnHandle;
}
#endif //#if (INTER_TEST_MODE == TEST_RAS_CLIENT)