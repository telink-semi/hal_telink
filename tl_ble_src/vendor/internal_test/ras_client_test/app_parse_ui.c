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
#include "app_cs.h"
#include "app_parse_ui.h"
#include "app_parse_char.h"
//for OOB
#include "app.h"

#if (INTER_TEST_MODE == TEST_RAS_CLIENT)

#if UI_CONTROL_ENABLE
adv_info_t advInfoTable[MAX_ADV_INFO_NUM];

u8 advCnt             = 0;
u8 reconn_en          = 0;
u8 scan_filter_enable = 0;
u8 filter_scan_addr[6];

u8 cs_chn_map_all[10]  = {0xfc, 0xff, 0x7f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f};
u8 cs_chn_map_half[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x1f};

void ttf_log_buffer_with_label(const void *buff, u16 len, char* label)
{
    const uint8_t *b_p = buff;
    u16 to_go = len;

    while(to_go!=0)
    {
        u16 chunk_len;

        chunk_len = min(16, to_go);
        tlkapi_printf(1, "%s%s\r\n", label, hex_to_str(b_p, chunk_len)); debugwait();
        b_p += chunk_len;
        to_go -= chunk_len;
    }
}

//stubbed on client side
int segment_skipper_check(u8 segment_to_check)
{
    (void)segment_to_check;
}
//stubbed on client side
void delay_segment(u8 segment)
{
    (void)segment;
}

/**
 * @brief       find advertisement information and store to buffer.
 * @param[in]   addrType: advertisement address type.
 * @param[in]   address: advertisement address.
 * @return      0: advertisement Already exist/ Insufficient buffer.
 *              other: advertisement index.
 */
static u8 app_findAdvInfo(u8 addrType, u8 address[6])
{
    if (advCnt == MAX_ADV_INFO_NUM) {
        return 0;
    }

    for (int i = 0; i < advCnt; i++) {
        adv_info_t *advInfo = &advInfoTable[i];

        if (advInfo->addrType == addrType && !memcmp(advInfo->address, address, 6)) {
            return 0;
        }
    }
    advInfoTable[advCnt].addrType = addrType;
    memcpy(advInfoTable[advCnt].address, address, 6);
    advCnt++;
    return advCnt;
}

/**
 * @brief       initial/clean advertisement information buffer.
 * @param[in]   none.
 * @return      none.
 */
void app_init_AdvInfoBuf(void)
{
    advCnt = 0;
    memset(&advInfoTable[0], 0, sizeof(adv_info_t) * MAX_ADV_INFO_NUM);
}

/**
 * @brief       found advertisement device event.
 * @param[in]   p: Data carried by the event.
 * @return      0.
 */
int app_parse_foundAdv(u8 *p)
{
    event_adv_report_t *pa = (event_adv_report_t *)p;
    if (scan_filter_enable) {
        if (memcpy(pa->mac, filter_scan_addr, 6)) {
            return 0;
        }
    }
    u8 index = app_findAdvInfo(pa->adr_type, pa->mac);
    if (index) {
        u8  adLen;
        u8  completeName[32] = "{No find name}";
        u8  len              = pa->len;
        u8 *pdata            = pa->data;
        while (len) {
            adLen = pdata[0];
            if (pdata[1] == DT_COMPLETE_LOCAL_NAME) {
                if (adLen - 1 > 31) {
                    adLen = 32;
                }
                memcpy(completeName, (pdata) + 2, adLen - 1);
                completeName[adLen - 1] = '\0';
                break;
            }

            if (len > (adLen + 1)) {
                len -= (adLen + 1);
                pdata += (adLen + 1);
            } else {
                len = 0;
            }
        }
        s8 rssi = pa->data[pa->len];
        app_parse_printf("[%d] %s %s RSSI:%d name:%s\r\n", index, pa->adr_type ? "random" : "public", addr_to_str(pa->mac), rssi, completeName);
    }

    return 0;
}

/**
 * @brief       found advertisement device event.
 * @param[in]   p: Data carried by the event.
 * @return      0.
 */
int app_parse_foundExtAdv(u8 *p)
{
    extAdvEvt_info_t *pa = (extAdvEvt_info_t *)p;
    if (scan_filter_enable) {
        if (memcpy(pa->address, filter_scan_addr, 6)) {
            return 0;
        }
    }
    u8 index = app_findAdvInfo(pa->address_type, pa->address);
    if (index) {
        u8  adLen;
        u8  completeName[32] = "{No find name}";
        u8  len              = pa->data_length;
        u8 *pdata            = pa->data;
        while (len) {
            adLen = pdata[0];
            if (pdata[1] == DT_COMPLETE_LOCAL_NAME) {
                if (adLen - 1 > 31) {
                    adLen = 32;
                }
                memcpy(completeName, (pdata) + 2, adLen - 1);
                completeName[adLen - 1] = '\0';
                break;
            }

            if (len > (adLen + 1)) {
                len -= (adLen + 1);
                pdata += (adLen + 1);
            } else {
                len = 0;
            }
        }
        s8 rssi = pa->data[pa->data_length];
        app_parse_printf("[%d] %s %s RSSI:%d name:%s\r\n", index, pa->address_type ? "random" : "public", addr_to_str(pa->address), rssi, completeName);
    }

    return 0;
}

/**
 * @brief       scan advertisement device.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_parse_ui_scan(char *argv[], int argc, void *user_data)
{
    (void)user_data;
    if (argc == 0) {
        app_parse_printf("scan <start|stop|clear|filter>\r\n");
        return;
    }

    if (strcasecmp(argv[0], "start") == 0) {
        app_parse_printf("central start scan\r\n");
        app_parse_printf("If need to connect, use 'conn <dev_idx>'\r\n");
        app_init_AdvInfoBuf();
        //extended advertising
        blc_ll_setExtScanEnable( BLC_SCAN_ENABLE, DUPE_FLTR_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
        // blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);
    } else if (strcasecmp(argv[0], "stop") == 0) {
        app_parse_printf("central stop scan\r\n");
        blc_ll_setScanEnable(BLC_SCAN_DISABLE, DUP_FILTER_DISABLE);
    } else if (strcasecmp(argv[0], "clear") == 0) {
        app_init_AdvInfoBuf();
        app_parse_printf("central clear adv info\r\n");
    } else if (strcasecmp(argv[0], "filter") == 0) {
        if (strcasecmp(argv[1], "disable") == 0) {
            app_parse_printf("central scan filter disable\r\n");
            scan_filter_enable = 0;
        } else if (strcasecmp(argv[1], "enable") == 0) {
            app_parse_printf("central scan filter enable\r\n");
            scan_filter_enable = 1;
        } else if (argc == 7) {
            scan_filter_enable = 1;
            blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);
            app_parse_printf("central scan filter enable\r\n");
            for (unsigned char i = 0; i < 6; i++) {
                filter_scan_addr[5 - i] = app_parse_str2xn(argv[i + 1]);
            }
        } else {
            app_parse_printf("scan filter not support [%s]", argv[0]);
            app_parse_printf("scan filter <enable|disable|mac address>\r\n");
        }
    } else {
        app_parse_printf("scan not support [%s]", argv[0]);
        app_parse_printf("scan <start|stop|clear|filter>\r\n");
    }
}

/**
 * @brief       create ACL connect by index.
 * @param[in]   index: scan advertisement index.
 * @return      0: index error.
 *              1: index true.
 */
int app_createACLConn(int index)
{
    if (index > advCnt) {
        return 0;
    }

    //extended connection
    blc_ll_extended_createConnection( INITIATE_FP_ADV_SPECIFY, OWN_ADDRESS_PUBLIC, advInfoTable[index - 1].addrType, advInfoTable[index - 1].address, INIT_PHY_1M, \
        SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_100MS, CONN_INTERVAL_100MS, CONN_TIMEOUT_4S, \
        0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0);
    // blc_ll_createConnection(SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, INITIATE_FP_ADV_SPECIFY, advInfoTable[index - 1].addrType, advInfoTable[index - 1].address, OWN_ADDRESS_PUBLIC, CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 0, CONN_TIMEOUT_4S, 0, 0xFFFF);
    return 1;
}

static void app_parse_ui_conn(char *argv[], int argc, void *user_data)
{
    (void)user_data;
    if (argc != 1) {
        app_parse_printf("conn <dev_idx|unpair>\r\n");
        return;
    }

    static u16 connHandle = (u16) 0x80;

    if (strcasecmp(argv[0], "unpair") == 0) {
#if (CS_PROCEDURE_EXCHANGE)
        if (cs_app_ctrl.cs_ctrl[0].connhandle) {
            app_parse_printf("disconnection\r\n");
            blc_ll_disconnect(cs_app_ctrl.cs_ctrl[0].connhandle, HCI_ERR_REMOTE_USER_TERM_CONN);
        }
#endif
        blc_smp_eraseAllBondingInfo();
        app_parse_printf("device unpair success\r\n");
    } else if (strcasecmp(argv[0], "disc") == 0) {
        app_parse_printf("disconnection %x\r\n", connHandle);
        blc_ll_disconnect(0x80, HCI_ERR_REMOTE_USER_TERM_CONN);
    } else if (app_createACLConn(app_parse_str2n(argv[0]))) {
        app_parse_printf("central start connect peripheral\r\n");
    } else {
        app_parse_printf("connect index error\r\n");
    }
}

static void app_parse_ui_reconn(char *argv[], int argc, void *user_data)
{
    (void)user_data;
    if (argc != 1) {
        app_parse_printf("reconn <enable|disable>\r\n");
        return;
    }

    if (strcasecmp(argv[0], "enable") == 0) {
        app_parse_printf("reconnection enable\r\n");
        blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);
        reconn_en = 1;
    } else if (strcasecmp(argv[0], "disable") == 0) {
        app_parse_printf("reconnection disable\r\n");
        reconn_en = 0;
    } else {
        app_parse_printf("reconnect command error\r\n");
        app_parse_printf("reconn <enable|disable>\r\n");
    }
}


#if CS_PROCEDURE_EXCHANGE
static void app_parse_ui_cs(char *argv[], int argc, void *user_data)
{
    (void)user_data;
    if (!cs_app_ctrl.cs_ctrl[0].connhandle) {
        app_parse_printf("acl connection establishment fail\r\n");
        return;
    }
    if (argc < 1) {
        app_parse_printf("cs <rst|cc|scp|log>\r\n");
        app_parse_printf("cmd-> [rst]:reset\r\n");
        app_parse_printf("cmd-> [cc]:create config <role><mainmode_rept><mode0_step>\r\n");
        app_parse_printf("param-> [role]:{00:initiator role,01:reflector role}\r\n");
        app_parse_printf("param-> [mainmode_type]:{01~03:mian mode type}\r\n");
        app_parse_printf("param-> [mode0_step]:{01~03: Number of CS mode 0 steps}\r\n");
        app_parse_printf("param-> [submode_type]:{01~03: submode type,0xff:unused}\r\n");
        app_parse_printf("param-> [mainmode_rept]:{00~03:The number of main mode steps to be repeated at the next subevt}\r\n");
        app_parse_printf("param-> [rtt type]:{00~06:rtt type}\r\n");
        app_parse_printf("cmd-> [scp]:set cs procedure param <max_procedure_cnt>\r\n");
        app_parse_printf("param-> [max_procedure_cnt]:{00:CS procedures to continue until disabled,01~ff:Maximum number of CS procedures to be scheduled}\r\n");

        return;
    }

    if (strcasecmp(argv[0], "rst") == 0) {
        blc_ll_disconnect(cs_app_ctrl.cs_ctrl[0].connhandle, HCI_ERR_CONN_TERM_BY_LOCAL_HOST);
        blc_cs_resetByHandle(cs_app_ctrl.cs_ctrl[0].connhandle);
        user_clrCsCtrlByHadle(cs_app_ctrl.cs_ctrl[0].connhandle);
        app_parse_printf("cs reset ok\r\n");
    } else if (strcasecmp(argv[0], "cc") == 0) {
        if (argc < 4) {
            app_parse_printf("need input full params,length is 6 Bytes <role><mainmode type><mode0_step><submode><mainmode rept><rtt type>\r\n");
            app_parse_printf("param-> [role]:{00:initiator role,01:reflector role}\r\n");
            app_parse_printf("param-> [mainmode_type]:{01~03:mode1,mode2,mode3}\r\n");
            app_parse_printf("param-> [mode0_step]:{01~03: Number of CS mode 0 steps}\r\n");
            app_parse_printf("param-> [submode_type]:{01~03: submode type,0xff:unused}\r\n");
            app_parse_printf("param-> [mainmode_rept]:{00~03:The number of main mode steps to be repeated at the next subevt}\r\n");
            app_parse_printf("param-> [rtt type]:{00~06:rtt type}\r\n");
            return;
        }
        u8                                pcmd[sizeof(hci_le_cs_creatConfig_cmdParam_t)] = {0};
        hci_le_cs_creatConfig_cmdParam_t *p                                              = (hci_le_cs_creatConfig_cmdParam_t *)pcmd;
        cs_app_ctrl.cs_ctrl[0].config_id                                                 = 0;
        p->Connection_Handle                                                             = cs_app_ctrl.cs_ctrl[0].connhandle;
        p->Config_ID                                                                     = 0;
        p->Create_Context                                                                = 1;
        p->Main_Mode                                                                     = 2;                        //mode2
        p->Sub_Mode                                                                      = app_parse_str2n(argv[4]); //mode1
        if (p->Sub_Mode == 0xff) {
            p->Main_Mode_Min_Steps = 0xff;
            p->Main_Mode_Max_Steps = 0xff;
        } else {
            p->Main_Mode_Min_Steps = 2;
            p->Main_Mode_Max_Steps = 4;
        }

        u8 main_mode = app_parse_str2n(argv[2]) & 0x03; //0
        if ((main_mode == 0) || (main_mode > 3)) {
            app_parse_printf("main mode type invalid\r\n");
            return;
        }
        p->Main_Mode = app_parse_str2n(argv[2]) & 0x03;            //0

        p->Main_Mode_Repetition = app_parse_str2n(argv[5]) & 0x03;
        p->Mode_0_Steps         = app_parse_str2n(argv[3]) & 0x03; //2
        p->Role                 = app_parse_str2n(argv[1]) ? CS_CONFIG_REFLECTOR_ROLE : CS_CONFIG_INITIATOR_ROLE;
        p->RTT_Type             = app_parse_str2n(argv[6]);        //RTT CS Access Address only timing
        p->CS_SYNC_PHY          = 0x01;                            //0x01: LE CS SYNC 1M PHY; 0x02: LE CS SYNC 2M PHY


        /*
         * Channels n = 0, 1, 23, 24, 25, 77, and 78 shall be ignored and shall
         *  be set to zero. At least 15 channels shall be enabled.
         */
        memcpy(&p->Channel_Map[0], &cs_chn_map_all[0], 10);
        p->Channel_Map_Repetition  = 1;
        p->ChSel                   = 0; //Use Channel Selection Algorithm #3b for non-mode 0 CS steps
        p->Ch3c_Shape              = 0; //Use Hat shape for user-specified channel sequence
        p->Ch3c_Jump               = 2;
        p->Companion_Signal_Enable = 0; //Companion Signal disabled

        blc_hci_le_cs_createConfig(p);

        app_parse_printf("cs create config\r\n");
    } else if (strcasecmp(argv[0], "cm") == 0) {
        u8 chnM[10];

        chnM[0]       = app_parse_str2n(argv[1]);  // 0~7
        chnM[1]       = app_parse_str2n(argv[2]);  // 8~15
        chnM[2]       = app_parse_str2n(argv[3]);  // 16~23
        chnM[3]       = app_parse_str2n(argv[4]);  // 24~31
        chnM[4]       = app_parse_str2n(argv[5]);  // 32~39
        chnM[5]       = app_parse_str2n(argv[6]);  // 40~47
        chnM[6]       = app_parse_str2n(argv[7]);  // 48~55
        chnM[7]       = app_parse_str2n(argv[8]);  // 56~63
        chnM[8]       = app_parse_str2n(argv[9]);  // 64~71
        chnM[9]       = app_parse_str2n(argv[10]); // 72~79
        ble_sts_t ret = blc_hci_le_cs_setChannelClassification(chnM);
        if (ret == HCI_ERR_INVALID_HCI_CMD_PARAMS) {
            app_parse_printf("valid channel number < 15, channel map update procedure quit, use old channel map!!! \r\n");
        } else {
            app_parse_printf("cs set cs channel map update received %x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n", chnM[0], chnM[1], chnM[2], chnM[3], chnM[4], chnM[5], chnM[6], chnM[7], chnM[8], chnM[9]);
        }
    } else if (strcasecmp(argv[0], "scp") == 0) {
        app_rangingFilter_init(); // update ranging filter after start distance calculation
        if (argc < 2) {
            app_parse_printf("need input full params,length is 1 Bytes <max_procedure_cnt>\r\n");
            app_parse_printf("param-> [max_procedure_cnt]:{00:CS procedures to continue until disabled,01~ff:Maximum number of CS procedures to be scheduled}\r\n");
            return;
        }
        u8                                       cmdPara[sizeof(hci_le_cs_setProcedureParame_cmdParam_t)] = {0}; //return length is 3
        u8                                       buff[sizeof(hci_le_cs_setProcedureParam_retParam_t)]     = {0}; //return length is 3
        hci_le_cs_setProcedureParame_cmdParam_t *param                                                    = (hci_le_cs_setProcedureParame_cmdParam_t *)cmdPara;

        u16 max_proc_cnt = app_parse_str2n(argv[1]);                                                             //2

        param->Connection_Handle = cs_app_ctrl.cs_ctrl[0].connhandle;
        param->Config_ID         = cs_app_ctrl.cs_ctrl[0].config_id;
        param->Max_Procedure_Len = 65535;
        #if (PARSE_TLK_ALGO2_JSON_LOG)
        param->Max_Procedure_Interval = 30;
        #else
        param->Max_Procedure_Interval = 20;
        #endif
        param->Min_Procedure_Interval = param->Max_Procedure_Interval;
        param->Max_Procedure_Count    = max_proc_cnt;

        param->Min_Subevent_Len[0] = 0x60;        //0x60;
        param->Min_Subevent_Len[1] = 0xEA;        //0x09;
        param->Min_Subevent_Len[2] = 0x00;

        param->Max_Subevent_Len[0] = 0x60;        //0x60;
        param->Max_Subevent_Len[1] = 0xEA;        //0x09;
        param->Max_Subevent_Len[2] = 0x00;

        param->Tone_Antenna_Config_Selection = 1;
        param->PHY                           = 1;
        param->Tx_Pwr_Delta                  = 0;
        param->Preferred_Peer_Antenna        = 1;


        u8 cs_para_ret = blc_hci_le_cs_setProcedureParam((hci_le_cs_setProcedureParame_cmdParam_t *)cmdPara,
                                                         (hci_le_cs_setProcedureParam_retParam_t *)buff);

        app_parse_printf("cs set cs procedure param\r\n");
        //////////////////////////////////////////////////
        u8                                    pcmd[16] = {0};
        hci_le_cs_enableProcedure_cmdParam_t *p        = (hci_le_cs_enableProcedure_cmdParam_t *)pcmd;

        p->Connection_Handle = cs_app_ctrl.cs_ctrl[0].connhandle;
        p->Config_ID         = cs_app_ctrl.cs_ctrl[0].config_id;
        p->Enable            = 1;
        if (cs_para_ret == BLE_SUCCESS) {
            blc_hci_le_cs_procedureEnable(p);
            app_parse_printf("cs start cs procedure\r\n");
        } else {
            app_parse_printf("cs param error: 0x%x\r\n", cs_para_ret);
        }
    } else if (strcasecmp(argv[0], "terminate") == 0) {
        u8                                    pcmd[16] = {0};
        hci_le_cs_enableProcedure_cmdParam_t *p        = (hci_le_cs_enableProcedure_cmdParam_t *)pcmd;
        p->Connection_Handle                           = cs_app_ctrl.cs_ctrl[0].connhandle;
        p->Config_ID                                   = cs_app_ctrl.cs_ctrl[0].config_id;
        p->Enable                                      = 0; // proc disable

        blc_hci_le_cs_procedureEnable(p);
        app_parse_printf("cs terminate cs procedure\r\n");

    } else if (strcasecmp(argv[0], "log") == 0) {
        if (argc < 2) {
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
        log_mask     = app_parse_str2n(argv[1]);
        blc_debug_enableStackLog(log_mask);
        app_parse_printf("cs print log mask:0x%x", log_mask);
    } else {
        app_parse_printf("cs cmd not support [%s]", argv[0]);
        app_parse_printf("cs <rst|cc|scp|log>\r\n");
    }
}
#endif /* #if CS_PROCEDURE_EXCHANGE */

static void app_rass_read_cb(u16 connHandle, att_err_t err)
{
    app_parse_printf("rass_read_cb conn_handle:%d status:0x%02X\r\n", connHandle, err);
}

static void app_parse_ui_rass(char *argv[], int argc, void *user_data)
{
    (void)user_data;
    (void)argc;

    static u16 connHandle = (u16) 0x80;

#if (RAS_OOB)
    if(strcasecmp(argv[0], "oob") == 0)
    {
        extern smp_sc_oob_data_t remoteOob;
        for(u8 i = 0; i<16; i++) {
            remoteOob.random[i] = app_parse_str2xn(argv[1+i]);
            remoteOob.confirm[i] = app_parse_str2xn(argv[1+16+i]);
            // appScOobCtrl.scoob_remote.random[i] = app_parse_str2xn(argv[1+i]);
            // appScOobCtrl.scoob_remote.confirm[i] = app_parse_str2xn(argv[1+16+i]);
        }
        tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP]Get Remote SC OOB data-r(be)(by UART)", remoteOob.random, 16);
        tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP]    Remote SC OOB data-c(be)(by UART)", remoteOob.confirm, 16);
        tlkapi_printf(APP_SMP_LOG_EN, "[APP][SMP]    ConnHandle %x", pinCodeReq_handle);

        // blc_smp_setScOobData(pinCodeReq_handle, &appScOobCtrl.scoob_local, &appScOobCtrl.scoob_local_key, &appScOobCtrl.scoob_remote);
        // pinCodeReq_handle = 0;
        return;
    }
    if(strcasecmp(argv[0], "oobpts") == 0)
    {
        extern smp_sc_oob_data_t remoteOob;
        u8 buff[48];
        int retVal;

        retVal = hex_string_to_bytes(buff, argv[1], 96);
        if (retVal != 48) { //number of bytes
            app_parse_printf("ERROR hex_string_to_bytes wrong len\r\n");
            return;
        }
        // 081B // 0-1
        // 65EBF3E807C0 00 //2-7 (6) //8
        // 021C 02 //9-10 //11
        // 1122 //12-13
        // 1473C3537566CBC60E0803DD0521A045 - confirm //14-29
        // 1123 //30-31
        // F0B8AA4A01F3DAF70000000000000000 - random //32-47
        app_parse_printf("%s\r\n", hex_to_str(buff, 48));

        #define RAS_OOB_CONFIRM_OFFSET 14
        #define RAS_OOB_RANDOM_OFFSET 32

        for(u8 i = 0; i<16; i++) {
            //reverse
            remoteOob.random[i] = buff[RAS_OOB_RANDOM_OFFSET + 15 - i];
            remoteOob.confirm[i] = buff[RAS_OOB_CONFIRM_OFFSET + 15 - i];
        }
        app_parse_printf("OOB remote confirm: %s\r\n", hex_to_str(remoteOob.confirm, 16));
        app_parse_printf("OOB remote random: %s\r\n", hex_to_str(remoteOob.random, 16));

        // tlkapi_printf(APP_SMP_LOG_EN, "[APP][SMP]    ConnHandle %x", pinCodeReq_handle);

        // blc_smp_setScOobData(pinCodeReq_handle, &appScOobCtrl.scoob_local, &appScOobCtrl.scoob_local_key, &appScOobCtrl.scoob_remote);
        // pinCodeReq_handle = 0;
        return;
    }
#endif
    if(strcasecmp(argv[0], "config") == 0)
    {
        u8 role = app_parse_str2n(argv[1]);
        u8 rttType = app_parse_str2n(argv[2]);

        // if (index == 9)
        // {
        //     extern int blc_rasc_subscribeOnDemandNtf(u16 connHandle, bool enable);
        //     blc_rasc_subscribeOnDemandNtf(0x80, true);
        //     app_parse_printf("ondemandNtf! \r\n");
        //     return;
        // }

        blc_ras_iop_initConfig(connHandle, role, rttType);
        app_parse_printf("OK\r\n");
        return;
    }
    if (strcasecmp(argv[0], "setconnh") == 0)
    {
        connHandle = app_parse_str2n(argv[1]);
        app_le_cs_iop_setConnectionHandle(connHandle);
        app_parse_printf("OK\r\n");
        return;
    }
    if(strcasecmp(argv[0], "manual") == 0)
    {
        // extern void blc_rasc_iop_enableRasClientManualTesting();
        blc_rasc_iop_enableRasClientManualTesting(connHandle);
        app_parse_printf("OK\r\n");
        return;
    }
    if(strcasecmp(argv[0], "get") == 0)
    {
        u16 rangingCounter = app_parse_str2n(argv[1]);

        ble_sts_t result = blc_rapc_iop_writeGetSpecificRecord(connHandle, rangingCounter, NULL);
        app_parse_printf("result = %d\r\n", result);

        return;
    }
    if(strcasecmp(argv[0], "lost") == 0)
    {
        u8 rangingCounter = app_parse_str2n(argv[1]);
        u8 startSegment = app_parse_str2n(argv[2]);
        u8 endSegment = app_parse_str2n(argv[3]);

        blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);
        client->rang_data.lost_data_ctl.lostSegmentsFlag = 1; //hack to force it to go to the correct spot in the blt_rasc_recvOnDemandProcedureData, otherwise a likely crash
        int result = blc_rasc_iop_writeGetRecordSegments(connHandle, rangingCounter, startSegment, endSegment, NULL);
        app_parse_printf("result = %d\r\n", result);

        return;
    }
    if(strcasecmp(argv[0], "ack") == 0)
    {
        u16 rangingCounter = app_parse_str2n(argv[1]);
        int result = blc_rapc_iop_writeAckSpecificRecord(connHandle, rangingCounter, NULL);
        app_parse_printf("result = %d\r\n", result);
        return;
    }
    if(strcasecmp(argv[0], "odccc") == 0) {
        u16 value = app_parse_str2n(argv[1]);
        blc_rapc_writeOnDemandCcc(connHandle, value, NULL);
        app_parse_printf("OK\r\n");
        return;
    }
    if(strcasecmp(argv[0], "rtccc") == 0) {
        u16 value = app_parse_str2n(argv[1]);
        blc_rapc_writeRealtimeCcc(connHandle, value, NULL);
        app_parse_printf("OK\r\n");
        return;
    }
    if(strcasecmp(argv[0], "newpr") == 0) { //for timeout. Normally it would come from internal CS
        u8 result = blt_rasc_newProcedure(connHandle);
        app_parse_printf("result = %x\r\n", result);
    }
    if(strcasecmp(argv[0], "abort") == 0) { //for abort
        u8 result = blc_rapc_writeAbortOperation(connHandle, NULL);
        app_parse_printf("result = %x\r\n", result);
    }
    if(strcasecmp(argv[0], "ccc") == 0)
    {
        u16 cccHandle = app_parse_str2n(argv[1]);
        u16 value = app_parse_str2n(argv[2]);

        ble_sts_t result = blc_rapc_iop_setCccValue(connHandle, cccHandle, value, NULL);
        app_parse_printf("result = %d\r\n", result);

        return;
    }
    if(strcasecmp(argv[0], "cmd") == 0)
    {
        #define MAX_PARAMS    5 //I dont expect too many parameters

        u8 opcode = app_parse_str2n(argv[1]);
        u8 parameterNumber = app_parse_str2n(argv[2]); //each parameter is expect to be 1 byte
        if(parameterNumber > MAX_PARAMS) {
            parameterNumber = MAX_PARAMS;
        }
        u8 value[MAX_PARAMS] = {0};

        for(u8 i=0; i<parameterNumber; i++) {
            value[i] = app_parse_str2n(argv[i+3]);
        }

        int result = blc_rasc_iop_issueAnyCommandControlPoint(connHandle, opcode, value, parameterNumber, NULL);
        app_parse_printf("result = %d\r\n", result);

        return;
    }
    if(strcasecmp(argv[0], "filter") == 0)
    {
        u16 filterSetting = app_parse_str2n(argv[1]);

        u8 result = blc_rapc_setFilterSetting(connHandle, filterSetting, NULL);
        if (result == 0) {
            app_parse_printf("OK\r\n");
        } else {
            app_parse_printf("ERROR %d\r\n", result);
        }
        return;
    }
#if (RAS_PTS_LOST_SEGMENT_WORKAROUND)
    if(strcasecmp(argv[0], "lostskipcnt") == 0)
    {
        u8 value = app_parse_str2n(argv[1]);
        blc_rapc_iop_setSegmentsToSkipCount(connHandle, value);
        return;
    }
#endif
    if(strcasecmp(argv[0], "read") == 0)
    {
        ble_sts_t status;

        if (!strcasecmp(argv[1], "feat")) {
            status = blc_rasc_readRasFeature(connHandle, app_rass_read_cb);
        } else if (!strcasecmp(argv[1], "ready")) {
            status = blc_rasc_readRasDataReady(connHandle, app_rass_read_cb);
        } else if (!strcasecmp(argv[1], "overw")) {
            status = blc_rasc_readRasDataOverwritten(connHandle, app_rass_read_cb);
        } else {
            app_parse_printf("rass read <conn_handle> <feat|ready|overw>\r\n");
        }

        app_parse_printf("rass read %s conn_handle:%d status:0x%02X\r\n", argv[1], connHandle, status);
        return;
    }
    app_parse_printf("ERROR\r\n");
}

static u8   ble_addr[6];

static void app_parse_ui_test(char *argv[], int argc, void *user_data)
{
    if(strcasecmp(argv[0], "ident") == 0)
    {
        app_parse_printf("cs_central\r\n");
        BLC_RAS_DATA_LOG("cs_central");
        return;
    }
    if(strcasecmp(argv[0], "read_bdaddr") == 0)
    {
        app_parse_printf("read_bdaddr: %02x:%02x:%02x:%02x:%02x:%02x\r\n", ble_addr[5], ble_addr[4],ble_addr[3],
                                                                           ble_addr[2], ble_addr[1], ble_addr[0]);
        return;
    }
    if(strcasecmp(argv[0], "reboot") == 0)
    {
        start_reboot();
        return;
    }
}

u8 readCBackFun(u16 connHandle, u8 err, gatt_read_data_t *rdData, struct __attribute__((packed)) gattc_read_cfg *params)
{
    (void)connHandle;

    if (err) {
        app_parse_printf("read attr value callback, err is %d\r\n", err);
        return GATT_PROC_END;
    }
    app_parse_printf("read value state is %d, data(%d) is %s", rdData->rdState, rdData->dataLen, hex_to_str(rdData->dataVal, rdData->dataLen));
    app_parse_printf("read sing data(%d) is %s", *params->single.wBuffLen, hex_to_str(params->single.wBuff, *params->single.wBuffLen));

    return GATT_PROC_CONT;
}

static const parse_fun_list_t centralParse[] = {
    {"scan",   app_parse_ui_scan,   NULL},
    {"conn",   app_parse_ui_conn,   NULL},
    {"reconn", app_parse_ui_reconn, NULL},
    {"test", app_parse_ui_test, NULL},
    #if (CS_PROCEDURE_EXCHANGE)
    {"cs",     app_parse_ui_cs,     NULL},
    #endif
    {"rass", app_parse_ui_rass, NULL},
};

/**
 * @brief       parse UI initial function.
 * @param[in]   none.
 * @return      none.
 */
void app_parse_ui_init(void)
{
    app_parse_init(centralParse, ARRAY_SIZE(centralParse));
    app_parse_printf("Parse init OK\r\n");
}

/**
 * @brief       provide UI with ble address used
 * @param[in]   ble address
 * @return      none.
 */
void app_parse_ui_set_addr(u8 *addr)
{
    memcpy(ble_addr, addr, 6);
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
#endif //#if (INTER_TEST_MODE == TEST_RAS_CLIENT)