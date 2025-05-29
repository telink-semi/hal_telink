/********************************************************************************************************
 * @file    cs_common.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2025
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

#include "stack/ble/controller/ble_controller.h"

#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
//////////////////////////////// CS_LB_VERSION////////////////////////////
#define CS_LIB_VERSION_NUM(lib_version_num)     "$$$CS_LIB_" #lib_version_num "$$$"
#define CS_LIB_VERSION(lib_version_num)         CS_LIB_VERSION_NUM(lib_version_num)
#define CS_VERSION_SRTING                       STRINGIFY(CS_VERSION_USE)
// modify CS_VERSION_USE
#define CS_PUBLIC_SDK_VERSION                   V0.1.0.0
#define CS_HANDOVER_SDK_VERSION                 V0.4.0.0
#define CS_GOOGLE_SDK_VERSION                   V0.1.0.0
// sdk release check!!!
#if (LL_CS_SNIFFER_MODE_ENABLE)
    #define CS_VERSION_USE                      CS_HANDOVER_SDK_VERSION
#else
    #define CS_VERSION_USE                      CS_PUBLIC_SDK_VERSION
#endif

volatile __attribute__((section(".sdk_version"))) unsigned char cs_lib_version[] = {CS_LIB_VERSION(CS_VERSION_USE)};
char *blc_get_cs_lib_version(void)
{
    static char *version = CS_VERSION_SRTING;
    return version;
}
//////////////////////////////// CS_LB_VERSION////////////////////////////


u8 CS_tx_early_1M_phy[3] = {CS_RF_TX_1M_PACKET_EARLY_US_MODE0, CS_RF_TX_1M_PACKET_EARLY_US_MODE1, CS_RF_TX_1M_TONE_EARLY_US};
u8 CS_tx_early_2M_phy[3] = {CS_RF_TX_2M_PACKET_EARLY_US_MODE0, CS_RF_TX_2M_PACKET_EARLY_US_MODE1, CS_RF_TX_2M_TONE_EARLY_US};


//////////////////////////////// LL_DEBUG_LOG////////////////////////////
void blt_ll_debug_print_capabilities(chn_sound_capabilities_t *cs_cap, char *title)
{
    u8 t_ip1 = 145;
    u8 t_ip2 = 145;
    u8 t_fcs = 150;
    for (u8 i = 0; i < sizeof(T_IP_US) - 1; i++) {
        if (cs_cap->Optional_T_IP1_Times_Supported & (1 << i)) {
            t_ip1 = T_IP_US[i];
            break;
        }
    }
    for (u8 i = 0; i < sizeof(T_IP_US) - 1; i++) {
        if (cs_cap->Optional_T_IP2_Times_Supported & (1 << i)) {
            t_ip2 = T_IP_US[i];
            break;
        }
    }
    for (u8 i = 0; i < sizeof(T_FCS_US) - 1; i++) {
        if (cs_cap->Optional_T_FCS_Times_Supported & (1 << i)) {
            t_fcs = T_FCS_US[i];
            break;
        }
    }

    CS_LL_LOG("---------------%s---------------", title);
    CS_LL_LOG("Mode3 support:[%s]", cs_cap->Mode_Types & BIT(0) ? "Y" : "N");
    CS_LL_LOG("RTT Cap:AA[%s],Sounding[%s],Random[%s]",
              cs_cap->RTT_Capability & BIT(0) ? "Y" : "N",
              cs_cap->RTT_Capability & BIT(1) ? "Y" : "N",
              cs_cap->RTT_Capability & BIT(2) ? "Y" : "N");
    CS_LL_LOG("AA:[0x%x],Sounding:[0x%x],Random:[0x%x]",
              cs_cap->RTT_AA_Only_N,
              cs_cap->RTT_Sounding_N,
              cs_cap->RTT_Random_Payload_N);
    CS_LL_LOG("PHY Cap:2M[%s],2M 2BT[%s]",
              cs_cap->Optional_CS_SYNC_PHYs_Supported & BIT(1) ? "Y" : "N",
              cs_cap->Optional_CS_SYNC_PHYs_Supported & BIT(2) ? "Y" : "N");
    CS_LL_LOG("Num Ant:[0x%x],Max Ant Path:[0x%x]",
              cs_cap->Num_Antennas_Supported,
              cs_cap->Max_Antenna_Paths_Supported);
    CS_LL_LOG("Role:I[%s],R[%s],Subfeatures:[0x%x]",
              cs_cap->Roles_Supported & BIT(0) ? "Y" : "N",
              cs_cap->Roles_Supported & BIT(1) ? "Y" : "N",
              cs_cap->Optional_Subfeatures_Supported);
    CS_LL_LOG("Max cfgs:[%d],Max Procedures:[%d]",
              cs_cap->Num_Config_Supported,
              cs_cap->max_consecutive_procedures_supported);
    CS_LL_LOG("T_SW:%d,T_IP1:%d,T_IP2:%d,T_FCS:%d",
              cs_cap->T_SW_Time_Supported,
              t_ip1,
              t_ip2,
              t_fcs);
    CS_LL_LOG("T_PM:10[%s],20[%s],TX SNR:[0x%x]",
              cs_cap->Optional_T_PM_Times_Supported & BIT(0) ? "Y" : "N",
              cs_cap->Optional_T_PM_Times_Supported & BIT(1) ? "Y" : "N",
              cs_cap->Optional_TX_SNR_Capability);
    CS_LL_LOG("---------------%s---------------", title);
}

void blt_ll_debug_print_security(void *p, char *title)
{
    rf_pkt_ll_cs_sec_rsp_t *sec = p;

    CS_LL_LOG("---------------%s---------------", title);
    CS_LL_LOG("IV:%s", hex_to_str(sec->CS_IV_P, 8));
    CS_LL_LOG("IN:%s", hex_to_str(sec->CS_IN_P, 4));
    CS_LL_LOG("PV:%s", hex_to_str(sec->CS_PV_P, 8));
    CS_LL_LOG("---------------%s---------------", title);
}

void blt_ll_debug_print_config_reqeust(rf_packet_ll_cs_config_req_t *cs_req, char *title)
{
    CS_LL_LOG("---------------%s---------------", title);
    CS_LL_LOG("Cfg id:[%d],Action:[%s]",
              cs_req->Config_ID,
              cs_req->State & BIT(0) ? "Create" : "Delete");
    CS_LL_LOG("chM:%s", hex_to_str(cs_req->ChM, 10));
    CS_LL_LOG("repeat:[%d],mainMode:[%d],subMode:[%d]",
              cs_req->ChM_Repetition,
              cs_req->Main_Mode,
              cs_req->Sub_Mode);
    CS_LL_LOG("mainMode:minStep[%d],maxStep:[%d],repeat:[%d]",
              cs_req->Main_Mode_Min_Steps,
              cs_req->Main_Mode_Max_Steps,
              cs_req->Main_Mode_Repetition);
    CS_LL_LOG("mode0 steps:[%d]", cs_req->Mode_0_Steps);
    CS_LL_LOG("PHY:1M[%s],2M[%s],coded[%s],2M2BT[%s]",
              cs_req->CS_SYNC_PHY & BIT(0) ? "Y" : "N",
              cs_req->CS_SYNC_PHY & BIT(1) ? "Y" : "N",
              cs_req->CS_SYNC_PHY & BIT(2) ? "Y" : "N",
              cs_req->CS_SYNC_PHY & BIT(3) ? "Y" : "N");
    CS_LL_LOG("RTT type:[0x%x],Role:[%s]",
              cs_req->RTT_Type,
              cs_req->Role ? "Reflector" : "Initiator");
    if (cs_req->ChSel) {
        CS_LL_LOG("ChSel:#3c,Shape:%s,Jump:[0x%x]",
                  cs_req->Ch3cShape ? "X" : "Hat",
                  cs_req->Ch3cJump);
    } else {
        CS_LL_LOG("ChSel:#3b");
    }
    CS_LL_LOG("T_IP1:%d,T_IP2:%d,T_FCS:%d,T_PM:%d",
              T_IP_US[cs_req->T_IP1],
              T_IP_US[cs_req->T_IP2],
              T_FCS_US[cs_req->T_FCS],
              T_PM_US[cs_req->T_PM]);
    CS_LL_LOG("---------------%s---------------", title);
}

void blt_ll_debug_print_cs_reqeust(rf_packet_ll_cs_req_t *req, char *title)
{
    u32 offset_min   = 0;
    u32 offset_max   = 0;
    u32 subevent_len = 0;
    offset_min       = req->Offset_Min[2] << 16 | req->Offset_Min[1] << 8 | req->Offset_Min[0];
    offset_max       = req->Offset_Max[2] << 16 | req->Offset_Max[1] << 8 | req->Offset_Max[0];
    subevent_len     = req->Subevent_Len[2] << 16 | req->Subevent_Len[1] << 8 | req->Subevent_Len[0];

    CS_LL_LOG("---------------%s---------------", title);
    CS_LL_LOG("Cfg id:[%d],Conn Event count:[%d]",
              req->Config_ID,
              req->connEventCount);
    CS_LL_LOG("Offset:min[%dus],max[%dus]",
              offset_min,
              offset_max);
    CS_LL_LOG("Procedure len:[%.5gus],Event interval:[%d]",
              req->Max_Procedure_Len * 0.625f,
              req->Event_Interval);
    CS_LL_LOG("Subevent per event[%d]", req->Subevents_Per_Event);
    CS_LL_LOG("Subevent:interval[%.5gms],len[%.5gms]",
              req->Subevent_Interval * 0.625f,
              subevent_len / 1000.0f);
    CS_LL_LOG("Procedure:Interval[%d],Count[%d]",
              req->Procedure_Interval,
              req->Procedure_Count);
    CS_LL_LOG("ACI:[0x%x],Preferred Peer Ant[0x%x]",
              req->ACI,
              req->Preferred_Peer_Ant);
    CS_LL_LOG("PHY:1M[%s],2M[%s],S8[%s],S2[%s]",
              req->PHY & BIT(0) ? "Y" : "N",
              req->PHY & BIT(1) ? "Y" : "N",
              req->PHY & BIT(2) ? "Y" : "N",
              req->PHY & BIT(3) ? "Y" : "N");
    CS_LL_LOG("Pwr_Delta:[%ddB]", req->Pwr_Delta);
    CS_LL_LOG("---------------%s---------------", title);
}

void blt_ll_debug_print_cs_response(rf_packet_ll_cs_rsp_t *rsp, char *title)
{
    u32 offset_min   = 0;
    u32 offset_max   = 0;
    u32 subevent_len = 0;
    offset_min       = rsp->Offset_Min[2] << 16 | rsp->Offset_Min[1] << 8 | rsp->Offset_Min[0];
    offset_max       = rsp->Offset_Max[2] << 16 | rsp->Offset_Max[1] << 8 | rsp->Offset_Max[0];
    subevent_len     = rsp->Subevent_Len[2] << 16 | rsp->Subevent_Len[1] << 8 | rsp->Subevent_Len[0];

    CS_LL_LOG("---------------%s---------------", title);
    CS_LL_LOG("Cfg id:[%d],Conn Event count:[%d]",
              rsp->Config_ID,
              rsp->connEventCount);
    CS_LL_LOG("Offset:min[%dus],max[%dus]",
              offset_min,
              offset_max);
    CS_LL_LOG("Event interval:[%d]",
              rsp->Event_Interval);
    CS_LL_LOG("Subevent per event[%d]", rsp->Subevents_Per_Event);
    CS_LL_LOG("Subevent:interval[%.5gms],len[%.5gms]",
              rsp->Subevent_Interval * 0.625f,
              subevent_len / 1000.0f);
    CS_LL_LOG("ACI:[0x%x]", rsp->ACI);
    CS_LL_LOG("PHY:1M[%s],2M[%s],S8[%s],S2[%s]",
              rsp->PHY & BIT(0) ? "Y" : "N",
              rsp->PHY & BIT(1) ? "Y" : "N",
              rsp->PHY & BIT(2) ? "Y" : "N",
              rsp->PHY & BIT(3) ? "Y" : "N");
    CS_LL_LOG("Pwr_Delta:[%ddB]", rsp->Pwr_Delta);
    CS_LL_LOG("---------------%s---------------", title);
}

void blt_ll_debug_print_cs_ind(rf_packet_ll_cs_ind_t *ind, char *title)
{
    u32 offset       = 0;
    u32 subevent_len = 0;
    offset           = ind->Offset[2] << 16 | ind->Offset[1] << 8 | ind->Offset[0];
    subevent_len     = ind->Subevent_Len[2] << 16 | ind->Subevent_Len[1] << 8 | ind->Subevent_Len[0];

    CS_LL_LOG("---------------%s---------------", title);
    CS_LL_LOG("Cfg id:[%d],Conn Event count:[%d]",
              ind->Config_ID,
              ind->connEventCount);
    CS_LL_LOG("Offset:[%dus],Event interval:[%d]",
              offset,
              ind->Event_Interval);
    CS_LL_LOG("Subevent per event[%d]", ind->Subevents_Per_Event);
    CS_LL_LOG("Subevent:interval[%.5gms],len[%.5gms]",
              ind->Subevent_Interval * 0.625f,
              subevent_len / 1000.0f);
    CS_LL_LOG("ACI:[0x%x]", ind->ACI);
    CS_LL_LOG("PHY:1M[%s],2M[%s],S8[%s],S2[%s]",
              ind->PHY & BIT(0) ? "Y" : "N",
              ind->PHY & BIT(1) ? "Y" : "N",
              ind->PHY & BIT(2) ? "Y" : "N",
              ind->PHY & BIT(3) ? "Y" : "N");
    CS_LL_LOG("Pwr_Delta:[%ddB]", ind->Pwr_Delta);
    CS_LL_LOG("---------------%s---------------", title);
}

//////////////////////////////// LL_DEBUG_LOG////////////////////////////

///////////////////////////// ANTENNA_SWITCHING /////////////////////////
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
void blt_ll_cs_set_api(cs_config_t *csCfg, u8 role, u8 aci, u8 ant_path, u8 sync_antenna_select)
{
    if (ant_path == 0) {
        return;
    } else if (ant_path == 1) {
        csCfg->perm_table[0] = ant_ctrl_cfg.ant_ctl_default;
    } else if (ant_path == 2) {
        csCfg->perm_table[0] = 0x110;
        csCfg->perm_table[1] = 0x01;
    } else if (ant_path == 3) {
        csCfg->perm_table[0] = 0x2210;
        csCfg->perm_table[1] = 0x2201;
        csCfg->perm_table[2] = 0x1120;
        csCfg->perm_table[3] = 0x1102;
        csCfg->perm_table[4] = 0x012;
        csCfg->perm_table[5] = 0x021;
    } else if (ant_path == 4) {
        if (aci == 7) {
            if (role == CHANNEL_SOUNDING_ROLE_INITIATOR) { //initiator
                u32 table[24] = {0x11100, 0x11100, 0x11010, 0x11001, 0x11001, 0x11010, 0x11100, 0x11100, 0x11010, 0x11001, 0x11001, 0x11010, 0x00110, 0x00101, 0x00110, 0x00101, 0x00011, 0x00011, 0x101, 0x110, 0x11, 0x11, 0x101, 0x110};

                for (int i = 0; i < 24; i++) {
                    csCfg->perm_table[i] = table[i];
                }
            } else { //reflector
                u32 table[24] = {0x11010, 0x11001, 0x11100, 0x11100, 0x11010, 0x11001, 0x110, 0x101, 0x110, 0x101, 0x11, 0x11, 0x11010, 0x11001, 0x11100, 0x11100, 0x11010, 0x11001, 0x11, 0x11, 0x101, 0x110, 0x110, 0x101};

                for (int i = 0; i < 24; i++) {
                    csCfg->perm_table[i] = table[i];
                }
            }
        } else {
            u32 table[24] = {0x33210, 0x33201, 0x33120, 0x33102, 0x33012, 0x33021, 0x22310, 0x22301, 0x22130, 0x22103, 0x22013, 0x22031, 0x11230, 0x11203, 0x11320, 0x11302, 0x11032, 0x11023, 0x213, 0x231, 0x123, 0x132, 0x312, 0x321};

            for (int i = 0; i < 24; i++) {
                csCfg->perm_table[i] = table[i];
            }
        }
    }
    //mode0
    if (sync_antenna_select > 0 && sync_antenna_select < 5) {
        u32 ant_seq = 0;
        for (int i = 0; i < 8; i++) {
            ant_seq |= ((sync_antenna_select - 1) & 0x07) << (i * 4);
        }

        csCfg->perm_table[24] = ant_seq;
        csCfg->perm_table[25] = ant_seq;
        csCfg->perm_table[26] = ant_seq;
        csCfg->perm_table[27] = ant_seq;
    } else if (sync_antenna_select == 0xfe) {
        csCfg->perm_table[24] = 0x00000000;
        csCfg->perm_table[25] = 0x11111111;
        csCfg->perm_table[26] = 0x22222222;
        csCfg->perm_table[27] = 0x33333333;
    } else if (sync_antenna_select == 0xff) {
        csCfg->perm_table[24] = ant_ctrl_cfg.ant_ctl_default;
        csCfg->perm_table[25] = ant_ctrl_cfg.ant_ctl_default;
        csCfg->perm_table[26] = ant_ctrl_cfg.ant_ctl_default;
        csCfg->perm_table[27] = ant_ctrl_cfg.ant_ctl_default;
    }
}



#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
int blt_ll_set_ant_switch_params(cs_config_t *csCfg, u8 sync_ant){
    u8 ant_path_num_table[2][8] = {
        {1, 2, 3, 4, 1, 1, 1, 4},
        {1, 1, 1, 1, 2, 3, 4, 4}
    };

    u8 ant_path = ant_path_num_table[csCfg->Role & 0x01][csCfg->aci & 0x07];
    blt_ll_cs_set_api(csCfg, csCfg->Role, csCfg->aci, ant_path, sync_ant);
    rf_aoa_aod_set_ant_num(ant_path + 1); //conside ext step

    /*
     * Note: tx_ant_offset,{0x36[1],0x39[7:0]},tx_on + tx_ant_offset is the tick ant switch from index0 to index1,
     *       next few ant switch tick based on ant_interval.
     *       rx_ant_offset,{0x36[2], 0x3a[7:0]},rx_en + rx_settle is the tick of rx_run, rx_run + rx_ant_offset is
     *       the tick ant switch from index0 to index1, next few ant switch tick based on ant_interval. -- xuqiang,qinghua,biao,yuexin
     *
     *       Now, tx_on tick ignore the first sw and switch ant in the middle of SW, so tx/rx_ant_offset is T_PM + T_SW/2,
     *       ant_interval is T_PM + T_SW.
     */
    rf_cs_ant_clk_mode(RF_CS_ANT_CLK_ALWAYS_ON_MODE); //todo
    rf_cs_set_ant_interval(CS_1US_CONVERT_125NS(csCfg->T_SW_Us + csCfg->T_PM_Us));
    //    rf_cs_set_rx_ant_offset(CS_1US_CONVERT_125NS(csCfg->T_PM_Us + (csCfg->T_SW_Us >> 1)));
    //    rf_cs_set_tx_ant_offset(CS_1US_CONVERT_125NS(csCfg->T_PM_Us + (csCfg->T_SW_Us >> 1)));

    rf_cs_set_rx_ant_offset(CS_1US_CONVERT_125NS(csCfg->T_PM_Us));
    rf_cs_set_tx_ant_offset(CS_1US_CONVERT_125NS(csCfg->T_PM_Us));
    rf_cs_txant_switch_mode(RF_CS_TX_ANT_SWITCH_TXON);   //todo

    return 0;
}

#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_
int blt_ll_set_ant_switch_permu(u32 permu){
    ble_rf_cs_ant_lut(permu + ant_ctrl_cfg.ant_ctrl_seq_base);
    return 0;
}


void blc_cs_antenna_switch_config_init(cs_ant_switch_config_t *params)
{
    // set ANT number
    rf_aoa_aod_set_ant_num(1);

    //antenna index switch sequence 01230123
    rf_aoa_aod_ant_pattern(SWITCH_SEQ_MODE0);
    ble_rf_cs_ant_lut(params->ant_default_seq_value + params->ant_ctrl_seq_base_value);

    rf_cs_ant_clk_mode(RF_CS_ANT_CLK_ALWAYS_ON_MODE);

    rf_cs_txant_switch_mode(RF_CS_TX_ANT_SWITCH_TXON);//RF_CS_TX_ANT_SWITCH_TXON //RF_CS_TX_ANT_SWITCH_PAPUP
    rf_cs_rxant_switch_on();
    rf_cs_ant_switch_auto();

    ant_ctrl_cfg.set_ant_param_func = blt_ll_set_ant_switch_params;
    ant_ctrl_cfg.set_ant_permu_func = blt_ll_set_ant_switch_permu;
    ant_ctrl_cfg.ant_ctl_default = params->ant_default_seq_value;
    ant_ctrl_cfg.ant_ctrl_seq_base = params->ant_ctrl_seq_base_value;
}

///////////////////////////// ANTENNA_SWITCHING /////////////////////////

//////////////////////////// POWER LEVEL CONTROL ////////////////////////
void blc_cs_set_tx_power_level(u8 power_level){
    gCsMng.cs_tx_power = power_level;
}

_attribute_ram_code_
void blt_ll_cs_tx_power_init(void){
    gCsMng.acl_tx_power = blt_extRF.txPower_level;
    if (gCsMng.cs_tx_power) {
        rf_set_power_level(gCsMng.cs_tx_power);
    }
}

_attribute_ram_code_
void blt_ll_cs_tx_power_deinit(void){
    if (gCsMng.acl_tx_power) {
        rf_set_power_level(gCsMng.acl_tx_power);
    }
}
//////////////////////////// POWER LEVEL CONTROL ////////////////////////

//////////////////////// TERCEL PD4-PD7 ANA PULLDOWN ////////////////////

/**
 * @brief      disable input and output, set pulldown of PD4 - pd7, only for TL721X
 * @param[in]  None
 * @return     None
 */
void blc_cs_disableGpioPinsFromD4ToD7(void){
    gpio_output_dis(GPIO_PD4);
    gpio_input_dis(GPIO_PD4);
    gpio_set_up_down_res(GPIO_PD4, GPIO_PIN_PULLDOWN_100K);

    gpio_output_dis(GPIO_PD5);
    gpio_input_dis(GPIO_PD5);
    gpio_set_up_down_res(GPIO_PD5, GPIO_PIN_PULLDOWN_100K);

    gpio_output_dis(GPIO_PD6);
    gpio_input_dis(GPIO_PD6);
    gpio_set_up_down_res(GPIO_PD6, GPIO_PIN_PULLDOWN_100K);

    gpio_output_dis(GPIO_PD7);
    gpio_input_dis(GPIO_PD7);
    gpio_set_up_down_res(GPIO_PD7, GPIO_PIN_PULLDOWN_100K);
}
//////////////////////// TERCEL PD4-PD7 ANA PULLDOWN ////////////////////
#endif
