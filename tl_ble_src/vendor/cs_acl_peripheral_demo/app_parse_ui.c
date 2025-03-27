/********************************************************************************************************
 * @file    app_parse_ui.c
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
#include <strings.h>
#include "app.h"
#include "app_parse_ui.h"
#include "app_parse_char.h"
#if UI_CONTROL_ENABLE
#if CS_PROCEDURE_EXCHANGE
static void app_parse_ui_cs(char *argv[], int argc, void *user_data)
{
    (void)user_data;
    if(!cs_app_ctrl.cs_ctrl[0].connhandle){
        app_parse_printf("acl connection establishment fail\r\n");
        return ;
    }
    if(argc < 1)
    {
        app_parse_printf("cs <rst|cc|scp|log>\r\n");
        app_parse_printf("cmd-> [rst]:reset\r\n");
        app_parse_printf("cmd-> [cc]:create config <role><mainmode_rept><mode0_step>\r\n");
        app_parse_printf("param-> [role]:{00:initiator role,01:reflector role}\r\n");
        app_parse_printf("param-> [mainmode_rept]:{00~03:The number of main mode steps to be repeated at the next subevt}\r\n");
        app_parse_printf("param-> [mode0_step]:{01~03: Number of CS mode 0 steps}\r\n");
        app_parse_printf("cmd-> [scp]:set cs procedure param <max_procedure_cnt>\r\n");
        app_parse_printf("param-> [max_procedure_cnt]:{00:CS procedures to continue until disabled,01~ff:Maximum number of CS procedures to be scheduled}\r\n");

        return ;
    }

    if(strcasecmp(argv[0], "rst") == 0)
    {
        blc_ll_disconnect(cs_app_ctrl.cs_ctrl[0].connhandle, HCI_ERR_CONN_TERM_BY_LOCAL_HOST);
        blc_cs_resetByHandle(cs_app_ctrl.cs_ctrl[0].connhandle);
        user_clrCsCtrlByHadle(cs_app_ctrl.cs_ctrl[0].connhandle);
        app_parse_printf("cs reset ok\r\n");
    }
    else if(strcasecmp(argv[0], "cc") == 0)
    {
        if(argc < 4)
        {
            app_parse_printf("need input full params,length is 3 Bytes <role><mainmode_rept><mode0_step>\r\n");
            app_parse_printf("param-> [role]:{00:initiator role,01:reflector role}\r\n");
            app_parse_printf("param-> [mainmode_rept]:{00~03:The number of main mode steps to be repeated at the next subevt}\r\n");
            app_parse_printf("param-> [mode0_step]:{01~03: Number of CS mode 0 steps}\r\n");
            return;
        }
        u8 pcmd[sizeof(hci_le_cs_creatConfig_cmdParam_t)] = {0};
        hci_le_cs_creatConfig_cmdParam_t  *p = (hci_le_cs_creatConfig_cmdParam_t*) pcmd;
        cs_app_ctrl.cs_ctrl[0].config_id =  0;
        p->Connection_Handle        = cs_app_ctrl.cs_ctrl[0].connhandle;
        p->Config_ID                = 0;
        p->Create_Context           =  1;
        p->Main_Mode                = 2;//mode2
        p->Sub_Mode                 = 0xff;//mode1
        if(p->Sub_Mode == 0xff){
            p->Main_Mode_Min_Steps      = 0;
            p->Main_Mode_Max_Steps      = 0;
        }
        else{
            p->Main_Mode_Min_Steps      = 1;
            p->Main_Mode_Max_Steps      = 3;
        }

        u8 main_mode = app_parse_str2n(argv[2]) & 0x03;//0
        if((main_mode ==0) || (main_mode > 3)){
            app_parse_printf("main mode type invalid\r\n");
            return ;
        }
        p->Main_Mode = app_parse_str2n(argv[2]) & 0x03;//0

        p->Main_Mode_Repetition = 0;
//      p->Main_Mode_Repetition     =  app_parse_str2n(argv[2]) & 0x03;//0

//      p->Main_Mode_Repetition     = app_parse_str2n(argv[2]) & 0x03;//0
        p->Mode_0_Steps             = app_parse_str2n(argv[3]) & 0x03;//2
        p->Role                     = app_parse_str2n(argv[1]) ? CS_CONFIG_REFLECTOR_ROLE : CS_CONFIG_INITIATOR_ROLE;
        p->RTT_Type                 = 0x00;//RTT CS Access Address only timing
        p->CS_SYNC_PHY              = 0x01;//LE 1M PHY
        /*
         * Channels n = 0, 1, 23, 24, 25, 77, and 78 shall be ignored and shall
         *  be set to zero. At least 15 channels shall be enabled.
         */
        p->Channel_Map[0]           = 0xfc;// 0~7
        p->Channel_Map[1]           = 0xff;// 8~15
        p->Channel_Map[2]           = 0x7f;// 16~23
        p->Channel_Map[3]           = 0xfc;// 24~31
        p->Channel_Map[4]           = 0xff;// 32~39
        p->Channel_Map[5]           = 0xff;// 40~47
        p->Channel_Map[6]           = 0xff;// 48~55
        p->Channel_Map[7]           = 0xff;// 56~63
        p->Channel_Map[8]           = 0xff;// 64~71
        p->Channel_Map[9]           = 0x1f;// 72~79
        p->Channel_Map_Repetition   = 1;
        p->ChSel                    = 0;//Use Channel Selection Algorithm #3b for non-mode 0 CS steps
        p->Ch3c_Shape               = 0;//Use Hat shape for user-specified channel sequence
        p->Ch3c_Jump                = 0;
        p->Companion_Signal_Enable  = 0;//Companion Signal disabled

        blc_hci_le_cs_createConfig(p);

        app_parse_printf("cs create config\r\n");
    }
    else if(strcasecmp(argv[0], "scp") == 0)
    {
        if(argc < 2)
        {
            app_parse_printf("need input full params,length is 1 Bytes <max_procedure_cnt>\r\n");
            app_parse_printf("param-> [max_procedure_cnt]:{00:CS procedures to continue until disabled,01~ff:Maximum number of CS procedures to be scheduled}\r\n");
            return;
        }
        u8 cmdPara[sizeof(hci_le_cs_setProcedureParame_cmdParam_t)] = {0};//return length is 3
        u8 buff[sizeof(hci_le_cs_setProcedureParam_retParam_t)] = {0};//return length is 3
        hci_le_cs_setProcedureParame_cmdParam_t *param  = (hci_le_cs_setProcedureParame_cmdParam_t*)cmdPara;
        param->Connection_Handle                = cs_app_ctrl.cs_ctrl[0].connhandle;
        param->Config_ID                        = cs_app_ctrl.cs_ctrl[0].config_id;
        param->Max_Procedure_Len                = 65535;
        param->Max_Procedure_Interval           = 20;
        param->Min_Procedure_Interval           = 20;
        param->Max_Procedure_Count              = app_parse_str2n(argv[1]);//2
        param->Min_Subevent_Len[0]              = 0x60;//0x00;
        param->Min_Subevent_Len[1]              = 0x09;//0x10;
        param->Min_Subevent_Len[2]              = 0x00;

        param->Max_Subevent_Len[0]              = 0x60;//0x00;
        param->Max_Subevent_Len[1]              = 0x09;//0x30;
        param->Max_Subevent_Len[2]              = 0x00;

        param->Tone_Antenna_Config_Selection    = 0;
        param->PHY                              = 1;
        param->Tx_Pwr_Delta                     = 0;
        param->Preferred_Peer_Antenna           = 1;


        blc_hci_le_cs_setProcedureParam ((hci_le_cs_setProcedureParame_cmdParam_t *)cmdPara,
                                    (hci_le_cs_setProcedureParam_retParam_t *)buff);

        app_parse_printf("cs set cs procedure param\r\n");
        //////////////////////////////////////////////////
        u8 pcmd[16]= {0};
        hci_le_cs_enableProcedure_cmdParam_t  *p = (hci_le_cs_enableProcedure_cmdParam_t*) pcmd;

        p->Connection_Handle = cs_app_ctrl.cs_ctrl[0].connhandle;
        p->Config_ID = cs_app_ctrl.cs_ctrl[0].config_id;
        p->Enable   = 1;

        blc_hci_le_cs_procedureEnable(p);
        app_parse_printf("cs start cs procedure\r\n");
    }
    else if(strcasecmp(argv[0], "log") == 0)
    {
        if(argc < 2)
        {
            app_parse_printf("need input full params,length is 4 Bytes <mask>\r\n");
            app_parse_printf("param-> [mask]:{bit0~31 map}\r\n");
            app_parse_printf("param-> [mask]:{bit10:STK_LOG_CS_HCI_CMD}\r\n");
            app_parse_printf("param-> [mask]:{bit11:STK_LOG_CS_HCI_EVT}\r\n");
            app_parse_printf("param-> [mask]:{bit12:STK_LOG_CS_APP}\r\n");
            app_parse_printf("param-> [mask]:{bit13:STK_LOG_CS_TIMING}\r\n");
            app_parse_printf("param-> [mask]:{bit14:STK_LOG_CS_SCHEDULE}\r\n");
            app_parse_printf("param-> [mask]:{bit15:STK_LOG_CS_HCI_LOG}\r\n");
            app_parse_printf("param-> [mask]:{bit16:STK_LOG_INITIATOR_TIMING}\r\n");
            app_parse_printf("param-> [mask]:{bit17:STK_LOG_CS_PROFILE}\r\n");
            app_parse_printf("param-> [mask]:{bit18:STK_LOG_CS_DRBG_ALGORITHM}\r\n");
            return;
        }
        u32 log_mask = STK_LOG_ALL;
        log_mask = app_parse_str2n(argv[1]);
        blc_debug_enableStackLog(log_mask);
        app_parse_printf("cs print log mask:0x%x",log_mask);
    }
    else
    {
        app_parse_printf("cs cmd not support [%s]", argv[0]);
        app_parse_printf("cs <rst|cc|scp|log>\r\n");
    }

}
#endif
static void app_parse_ui_conn(char *argv[], int argc, void *user_data)
{
    (void)user_data;
    if(argc != 1)
    {
        app_parse_printf("conn <unpair>\r\n");
        return ;
    }

    if(strcasecmp(argv[0], "unpair") == 0)
    {
        if(cs_app_ctrl.cs_ctrl[0].connhandle){
            app_parse_printf("disconnection\r\n");
            blc_ll_disconnect(cs_app_ctrl.cs_ctrl[0].connhandle, HCI_ERR_REMOTE_USER_TERM_CONN);
        }
        blc_smp_eraseAllBondingInfo();
        app_parse_printf("device unpair success\r\n");
    }
    else
    {
        app_parse_printf("connect index error\r\n");
    }
}

static const parse_fun_list_t assistantParse[] = {
    {"conn", app_parse_ui_conn, NULL},
#if (CS_PROCEDURE_EXCHANGE && CS_PROCEDURE_CMD_TRIG)
    {"cs", app_parse_ui_cs, NULL},
#endif
};

/**
 * @brief       parse UI initial function.
 * @param[in]   none.
 * @return      none.
 */
void app_parse_ui_init(void)
{
    app_parse_init(assistantParse, ARRAY_SIZE(assistantParse));
    app_parse_printf("Parse init OK\r\n");
}


/**
 * @brief       central UI loop function.
 * @param[in]   none.
 * @return      none.
 */
void app_parse_ui_loop(void)
{
    app_parse_loop();
}
#endif
