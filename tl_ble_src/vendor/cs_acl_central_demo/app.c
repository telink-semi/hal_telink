/********************************************************************************************************
 * @file    app.c
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
#include "app_buffer.h"
#include "app_ui.h"
#include "app_parse_ui.h"
#include "application/usbstd/usb.h"
#include "app_parse_char.h"
#include "algorithm/hadm/hadm.h"

int central_smp_pending = 0;        // SMP: security & encryption;
int central_connected_led_on = 0;


/**
 * @brief      BLE Adv report event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int AA_dbg_adv_rpt = 0;
u32 tick_adv_rpt = 0;
#define CS_BUFF_COUNT           1
u8 cs_buff[CS_PARAM_LENGTH*CS_BUFF_COUNT] = {0};


int app_le_adv_report_event_handle(u8 *p)
{
    event_adv_report_t *pa = (event_adv_report_t *)p;
    s8 rssi = pa->data[pa->len];

    #if 0  //debug, print ADV report number every 5 seconds
        AA_dbg_adv_rpt ++;
        if(clock_time_exceed(tick_adv_rpt, 5000000)){
            tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Adv report", pa->mac, 6);
            tick_adv_rpt = clock_time();
        }
    #endif

    /*********************** Central Create connection demo: Key press or ADV pair packet triggers pair  ********************/
#if (ACL_CENTRAL_SMP_ENABLE)
    if(central_smp_pending){     //if previous connection SMP not finish, can not create a new connection
        return 1;
    }
#endif

#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
    if(central_sdp_pending){     //if previous connection SDP not finish, can not create a new connection
        return 1;
    }
#endif

    if (central_disconnect_connhandle){ //one ACL connection central role is in un_pair disconnection flow, do not create a new one
        return 1;
    }

    int central_auto_connect = 0;
    int user_manual_pairing = 0;

    //manual pairing methods 1: key press triggers
    user_manual_pairing = central_pairing_enable && (rssi > -66);  //button trigger pairing(RSSI threshold, short distance)

    #if (ACL_CENTRAL_SMP_ENABLE)
        central_auto_connect = blc_smp_searchBondingPeripheralDevice_by_PeerMacAddress(pa->adr_type, pa->mac);
    #endif

    #if UI_CONTROL_ENABLE
        if(reconn_en==0) central_auto_connect = 0;
    #endif

    if(central_auto_connect || user_manual_pairing){
        /* send create connection command to Controller, trigger it switch to initiating state. After this command, Controller
         * will scan all the ADV packets it received but not report to host, to find the specified device(mac_adr_type & mac_adr),
         * then send a "CONN_REQ" packet, enter to connection state and send a connection complete event
         * (HCI_SUB_EVT_LE_CONNECTION_COMPLETE) to Host*/
        u8 status = blc_ll_createConnection( SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, INITIATE_FP_ADV_SPECIFY,  \
                                 pa->adr_type, pa->mac, OWN_ADDRESS_PUBLIC, \
                                 CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 0, CONN_TIMEOUT_4S, \
                                 0, 0xFFFF);

        if(status == BLE_SUCCESS){ //create connection success
            tlkapi_send_string_data(APP_PAIR_LOG_EN, "[APP] create connection success", pa->mac, 6);
        }
        else{
            //tlkapi_send_string_data(APP_PAIR_LOG_EN, "[APP] create connection fail", &status, 1);
        }
    }
    /*********************** Central Create connection demo code end  *******************************************************/


    return 0;
}


////////////////////////////channel sounding test function///////////////////////////////////////
cs_app_control_t    cs_app_ctrl;
#if CS_PROCEDURE_EXCHANGE
int user_addCsCtrlByHadle(u16 connhadle)
{
    int ret = 0;
    for(int i=0;i<CS_MAX_NUM;i++){
        if(cs_app_ctrl.cs_ctrl[i].connhandle == 0){
            cs_app_ctrl.cs_ctrl[i].connhandle = connhadle;
            ret = 1;
            break;
        }
    }
    return ret;
}
int user_clrCsCtrlByHadle(u16 connhadle)
{
    int ret = 0;
    for(int i=0;i<CS_MAX_NUM;i++){
        if(cs_app_ctrl.cs_ctrl[i].connhandle == connhadle){
            memset(&cs_app_ctrl.cs_ctrl[i],0,sizeof(cs_control_t));
            ret = 1;
            break;
        }
    }
    return ret;
}

int user_getCsCtrlByHadle(u16 connhadle)
{
    int ret = 0;
    for(int i=0;i<CS_MAX_NUM;i++){
        if(cs_app_ctrl.cs_ctrl[i].connhandle == connhadle){
            ret = i + 1;
            break;
        }
    }
    return ret;
}

void user_initCsCtrl(void)
{
    memset(&cs_app_ctrl.cs_ctrl[0],0,sizeof(cs_app_control_t));
}

void user_setCsProcStartStatus(u8 index,eCsProcStatus status)
{
    cs_app_ctrl.cs_ctrl[index % CS_MAX_NUM].exch_start_state = status;
}

void user_setCsProcCmpltStatus(u8 index,eCsProcCmpltStatusMask status)
{
    cs_app_ctrl.cs_ctrl[index % CS_MAX_NUM].exch_cmplt_state |= status;
}

void user_clrCsProcCmpltStatus(u8 index,eCsProcCmpltStatusMask status)
{
    cs_app_ctrl.cs_ctrl[index % CS_MAX_NUM].exch_cmplt_state &= ~status;
}

void cs_procedure_ctrl(void){


    for(int i=0;i<CS_MAX_NUM;i++){
        if(cs_app_ctrl.cs_ctrl[i].connhandle){
            u8 index = i;
            if(cs_app_ctrl.cs_ctrl[index].exch_cmplt_state != NULL_EXCH_CMPLT){
                if(cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & CAP_EXCH_CMPLT){
                    user_setCsProcStartStatus(index,SET_DEFAULT);
                    user_clrCsProcCmpltStatus(index,CAP_EXCH_CMPLT);
                }
                else if(cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & SET_DFT_CMPLT){
                    user_clrCsProcCmpltStatus(index,SET_DFT_CMPLT);
                    u8 status = blc_ll_test_CsFaeExchCtrl(cs_app_ctrl.cs_ctrl[index].connhandle);

                    if(status == 1){//need wait peer dev initiator fae exchange

                    }
                    else if(status == 2){
                        cs_app_ctrl.cs_ctrl[index].exchange_tick = clock_time()|1;
                        user_setCsProcStartStatus(index,FAE_EXCH);
                    }
                    else if(status == 0){
                        user_setCsProcCmpltStatus(index,FAE_EXCH_CMPLT);
                    }

                }
                else if(cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & FAE_EXCH_CMPLT){
                    user_clrCsProcCmpltStatus(index,FAE_EXCH_CMPLT);
                    #if CS_PROCEDURE_CMD_TRIG
                        #if UI_CONTROL_ENABLE
                            app_parse_printf("please send cs config cmd <cs cc [role][mainmode_type][mode0_step]>,if peer not initiate exchange firstly!\r\n");
                        #endif
                    #else
                        user_setCsProcStartStatus(index,CFG_EXCH);
                        cs_app_ctrl.cs_ctrl[index].exchange_tick = clock_time()|1;
                    #endif
                }
                else if(cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & CFG_EXCH_CMPLT){
                    user_clrCsProcCmpltStatus(index,CFG_EXCH_CMPLT);
                    if(cs_app_ctrl.cs_ctrl[index].acl_role == ACL_ROLE_CENTRAL){
                        if(!(blc_ll_getCsSecExchStatus(cs_app_ctrl.cs_ctrl[index].connhandle)))
                        {
                            user_setCsProcStartStatus(index,SEC_EXCH);
                        }
                        else{
                            user_setCsProcCmpltStatus(index,SEC_EXCH_CMPLT);
                        }
                    }
                }
                else if(cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & SEC_EXCH_CMPLT){
                    user_clrCsProcCmpltStatus(index,SEC_EXCH_CMPLT);
                    #if CS_PROCEDURE_CMD_TRIG
                        #if UI_CONTROL_ENABLE
                            app_parse_printf("please send cs procedure param cmd <cs scp [max_procedure_cnt]>,if peer not initiate exchange firstly\r\n");
                        #endif
                    #else
                        user_setCsProcStartStatus(index,SET_PROC_PARAM);
                    #endif
                }
                else if(cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & SET_PROC_PARAM_CMPLT){
                    user_clrCsProcCmpltStatus(index,SET_PROC_PARAM_CMPLT);
                    #if CS_PROCEDURE_CMD_TRIG

                    #else
                        user_setCsProcStartStatus(index,CS_PROC_EN_EXCH);
                    #endif
                }
                else if(cs_app_ctrl.cs_ctrl[index].exch_cmplt_state & CS_PROC_EN_CMPLT){
                    user_clrCsProcCmpltStatus(index,CS_PROC_EN_CMPLT);
                }
            }


            if(cs_app_ctrl.cs_ctrl[index].exch_start_state== NULL_EXCH){
                return;
            }
            switch (cs_app_ctrl.cs_ctrl[index].exch_start_state){
                case CAP_EXCH:
                {
                    if(cs_app_ctrl.cs_ctrl[index].exchange_tick && clock_time_exceed(cs_app_ctrl.cs_ctrl[index].exchange_tick,2000*1000)){
                        blc_hci_le_cs_readRemoteSupportedCap(cs_app_ctrl.cs_ctrl[index].connhandle);
                        user_setCsProcStartStatus(index,NULL_EXCH);
                        cs_app_ctrl.cs_ctrl[index].exchange_tick = 0;
                    }
                }
                    break;
                case SET_DEFAULT:
                {
                    u8 pcmd[sizeof(hci_le_cs_setDefaultSetting_cmdParam_t)]= {0};
                    u8 pret[sizeof(hci_le_cs_setDefaultSetting_retParam_t)]= {0};
                    hci_le_cs_setDefaultSetting_cmdParam_t  *p = (hci_le_cs_setDefaultSetting_cmdParam_t*) pcmd;

                    p->Connection_Handle            = cs_app_ctrl.cs_ctrl[index].connhandle;
                    p->Role_Enable                  = 3;//all role en
                    p->Max_TX_Power                 = 0;//0dbm
                    p->CS_SYNC_Antenna_Selection    = 1;

                    blc_hci_le_cs_setDefaultSettings(p,(hci_le_cs_setDefaultSetting_retParam_t *)pret);

                    if(p->Role_Enable & CS_INITIATOR_ROLE){
                        blc_ll_initCsInitiatorModule();
                    }
                    if(p->Role_Enable & CS_REFLECTOR_ROLE){
                        blc_ll_initCsReflectorModule();
                    }
                    user_setCsProcStartStatus(index,NULL_EXCH);
                    user_setCsProcCmpltStatus(index,SET_DFT_CMPLT);

                }
                    break;
                case FAE_EXCH:
                {
                    if(cs_app_ctrl.cs_ctrl[index].exchange_tick && clock_time_exceed(cs_app_ctrl.cs_ctrl[index].exchange_tick,100*1000)){
                        blc_hci_le_cs_readRemoteFAE_table(cs_app_ctrl.cs_ctrl[index].connhandle);
                        user_setCsProcStartStatus(index,NULL_EXCH);
                        cs_app_ctrl.cs_ctrl[index].exchange_tick = 0;
                    }
                }
                    break;
                case CFG_EXCH:
                {
                    if(cs_app_ctrl.cs_ctrl[index].exchange_tick && clock_time_exceed(cs_app_ctrl.cs_ctrl[index].exchange_tick,100*1000)){
                        u8 pcmd[sizeof(hci_le_cs_creatConfig_cmdParam_t)] = {0};
                        hci_le_cs_creatConfig_cmdParam_t  *p = (hci_le_cs_creatConfig_cmdParam_t*) pcmd;
                        cs_app_ctrl.cs_ctrl[index].config_id        =  0;
                        p->Connection_Handle        = cs_app_ctrl.cs_ctrl[index].connhandle;
                        p->Config_ID                = 0;
                        p->Create_Context           = 1;
                        p->Main_Mode                = 2;//mode2
                        p->Sub_Mode                 = 0xff;//unused

                        p->Main_Mode_Min_Steps      = 1;
                        p->Main_Mode_Max_Steps      = 3;

                        p->Main_Mode                    = 2;//Main_Mode !=0, <= 3
                        p->Main_Mode_Repetition         = 0;
                        p->Mode_0_Steps                 = 2;//2
                        p->Role                         = CS_CONFIG_INITIATOR_ROLE;
                        p->RTT_Type                     = 0x00;//RTT CS Access Address only timing
                        p->CS_SYNC_PHY                  = 0x01;//LE 1M PHY
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
                        p->Channel_Map_Repetition   = 3;
                        p->ChSel                    = 0;//Use Channel Selection Algorithm #3b for non-mode 0 CS steps
                        p->Ch3c_Shape               = 0;//Use Hat shape for user-specified channel sequence
                        p->Ch3c_Jump                = 0;
                        p->Companion_Signal_Enable  = 0;//Companion Signal disabled

                        blc_hci_le_cs_createConfig(p);


                        user_setCsProcStartStatus(index,NULL_EXCH);
                        cs_app_ctrl.cs_ctrl[index].exchange_tick = 0;
                    }
                }
                    break;
                case SEC_EXCH:
                {
                    if(cs_app_ctrl.cs_ctrl[index].acl_role == ACL_ROLE_CENTRAL){
                        blc_hci_le_cs_security_enable(cs_app_ctrl.cs_ctrl[index].connhandle);
                    }
                    user_setCsProcStartStatus(index,NULL_EXCH);
                }
                    break;
                case SET_PROC_PARAM:
                {
                    u8 cmdPara[sizeof(hci_le_cs_setProcedureParame_cmdParam_t)] = {0};//return length is 3
                    u8 buff[sizeof(hci_le_cs_setProcedureParam_retParam_t)] = {0};//return length is 3
                    hci_le_cs_setProcedureParame_cmdParam_t *param  = (hci_le_cs_setProcedureParame_cmdParam_t*)cmdPara;

                    param->Connection_Handle                = cs_app_ctrl.cs_ctrl[index].connhandle;
                    param->Config_ID                        = cs_app_ctrl.cs_ctrl[index].config_id;
                    param->Max_Procedure_Len                = 65535;
                    param->Max_Procedure_Interval           = 20;
                    param->Min_Procedure_Interval           = 20;
                    param->Max_Procedure_Count              = 2;//need >1
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


                    user_setCsProcStartStatus(index,NULL_EXCH);
                    user_setCsProcCmpltStatus(index,SET_PROC_PARAM_CMPLT);
                }
                    break;
                case CS_PROC_EN_EXCH:
                {
                    u8 pcmd[16]= {0};
                    hci_le_cs_enableProcedure_cmdParam_t  *p = (hci_le_cs_enableProcedure_cmdParam_t*) pcmd;

                    p->Connection_Handle = cs_app_ctrl.cs_ctrl[index].connhandle;
                    p->Config_ID = cs_app_ctrl.cs_ctrl[index].config_id;
                    p->Enable   = 1;

                    blc_hci_le_cs_procedureEnable(p);
                    user_setCsProcStartStatus(index,NULL_EXCH);
                }
                    break;
                default:
                    break;
            }
        }
    }
}
#endif
/////////////////////////////////////////////////////////////////////////////



/**
 * @brief      BLE Connection complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_connection_complete_event_handle(u8 *p)
{
    hci_le_connectionCompleteEvt_t *pConnEvt = (hci_le_connectionCompleteEvt_t *)p;

    if(pConnEvt->status == BLE_SUCCESS){

        tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Connection complete event", &pConnEvt->connHandle, sizeof(hci_le_connectionCompleteEvt_t) - 2);
    #if (UI_LED_ENABLE)
        //led show connection state
        central_connected_led_on = 1;
        gpio_write(GPIO_LED_RED, LED_ON_LEVEL);     //red on
        gpio_write(GPIO_LED_WHITE, !LED_ON_LEVEL);  //white off
    #endif

        dev_char_info_insert_by_conn_event(pConnEvt);

        if(pConnEvt->role == ACL_ROLE_CENTRAL) // central role, process SMP and SDP if necessary
        {
            #if (ACL_CENTRAL_SMP_ENABLE)
                central_smp_pending = pConnEvt->connHandle; // this connection need SMP
            #endif

            #if CS_PROCEDURE_EXCHANGE
                u8 index= 0;
                if(user_addCsCtrlByHadle(pConnEvt->connHandle)){
                    index = user_getCsCtrlByHadle(pConnEvt->connHandle);
                    if(index == 0){
                        //err
                    }
                    else{
                        index = index -1;
                    }
                }
                else{
                    //error,buffer not enough
                }
                cs_app_ctrl.cs_ctrl[index].exchange_tick = clock_time()|1;
                user_setCsProcStartStatus(index,CAP_EXCH);
                cs_app_ctrl.cs_ctrl[index].acl_role = pConnEvt->role;
                #if UI_CONTROL_ENABLE
                    app_parse_printf("wait cs capability exchange\r\n");
                #endif
            #endif
            #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                memset(&cur_sdp_device, 0, sizeof(dev_char_info_t));
                cur_sdp_device.conn_handle = pConnEvt->connHandle;
                cur_sdp_device.peer_adrType = pConnEvt->peerAddrType;
                memcpy(cur_sdp_device.peer_addr, pConnEvt->peerAddr, 6);

                u8  temp_buff[sizeof(dev_att_t)];
                dev_att_t *pdev_att = (dev_att_t *)temp_buff;

                /* att_handle search in flash, if success, load char_handle directly from flash, no need SDP again */
                if( dev_char_info_search_peer_att_handle_by_peer_mac(pConnEvt->peerAddrType, pConnEvt->peerAddr, pdev_att) ){
                    //cur_sdp_device.char_handle[1] =                                   //Speaker
                    cur_sdp_device.char_handle[2] = pdev_att->char_handle[2];           //OTA
                    cur_sdp_device.char_handle[3] = pdev_att->char_handle[3];           //consume report
                    cur_sdp_device.char_handle[4] = pdev_att->char_handle[4];           //normal key report
                    //cur_sdp_device.char_handle[6] =                                   //BLE Module, SPP Server to Client
                    //cur_sdp_device.char_handle[7] =                                   //BLE Module, SPP Client to Server

                    /* add the peer device att_handle value to conn_dev_list */
                    dev_char_info_add_peer_att_handle(&cur_sdp_device);
                }
                else
                {
                    central_sdp_pending = pConnEvt->connHandle;  // mark this connection need SDP

                    #if (ACL_CENTRAL_SMP_ENABLE)
                         //service discovery initiated after SMP done, trigger it in "GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE" event callBack.
                    #else
                         app_register_service(&app_service_discovery);  //No SMP, service discovery can initiated now
                    #endif
                }
            #endif
        }
    }

    return 0;
}



/**
 * @brief      BLE Disconnection event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int     app_disconnect_event_handle(u8 *p)
{
    hci_disconnectionCompleteEvt_t  *pDisConn = (hci_disconnectionCompleteEvt_t *)p;
    tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] disconnect event", &pDisConn->connHandle, 3);

    //terminate reason
    if(pDisConn->reason == HCI_ERR_CONN_TIMEOUT){   //connection timeout

    }
    else if(pDisConn->reason == HCI_ERR_REMOTE_USER_TERM_CONN){     //peer device send terminate command on link layer

    }
    //central host disconnect( blm_ll_disconnect(current_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN) )
    else if(pDisConn->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST){

    }
    else{

    }

#if (UI_LED_ENABLE)
    //led show none connection state
    if(central_connected_led_on){
        central_connected_led_on = 0;
        gpio_write(GPIO_LED_WHITE, LED_ON_LEVEL);   //white on
        gpio_write(GPIO_LED_RED, !LED_ON_LEVEL);    //red off
    }
#endif

    //if previous connection SMP & SDP not finished, clear flag
#if (ACL_CENTRAL_SMP_ENABLE)
    if(central_smp_pending == pDisConn->connHandle){
        central_smp_pending = 0;
    }
#endif
#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
    if(central_sdp_pending == pDisConn->connHandle){
        central_sdp_pending = 0;
    }
#endif

    if(central_disconnect_connhandle == pDisConn->connHandle){  //un_pair disconnection flow finish, clear flag
        central_disconnect_connhandle = 0;
    }
#if CS_PROCEDURE_EXCHANGE
    blc_cs_resetByHandle(pDisConn->connHandle);
    user_clrCsCtrlByHadle(pDisConn->connHandle);
    blc_ll_setScanEnable (BLC_SCAN_DISABLE, DUP_FILTER_DISABLE);
    reconn_en = 0;
#endif
    dev_char_info_delete_by_connhandle(pDisConn->connHandle);


    return 0;
}


/**
 * @brief      BLE Connection update complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_connection_update_complete_event_handle(u8 *p)
{
    hci_le_connectionUpdateCompleteEvt_t *pUpt = (hci_le_connectionUpdateCompleteEvt_t *)p;
    tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Connection Update Event", &pUpt->connHandle, 8);

    if(pUpt->status == BLE_SUCCESS){



    }

    return 0;
}

//////////////////////////////////////////////////////////
// event call back
//////////////////////////////////////////////////////////
/**
 * @brief      BLE controller event handler call-back.
 * @param[in]  h       event type
 * @param[in]  p       Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_controller_event_callback (u32 h, u8 *p, int n)
{
    (void)n;
    if (h &HCI_FLAG_EVENT_BT_STD)       //Controller HCI event
    {
        u8 evtCode = h & 0xff;

        //------------ disconnect -------------------------------------
        if(evtCode == HCI_EVT_DISCONNECTION_COMPLETE)  //connection terminate
        {
            app_disconnect_event_handle(p);
        }
        else if(evtCode == HCI_EVT_LE_META)  //LE Event
        {
            u8 subEvt_code = p[0];
            u16 handle = p[1]|(p[2]<<8);
            #if CS_PROCEDURE_EXCHANGE
                u8 index = user_getCsCtrlByHadle(handle);
                if(index == 0){
                    //err
                }
                else{
                    index = index - 1;
                }
            #endif
            //------hci le event: le connection complete event---------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE)  // connection complete
            {
                app_le_connection_complete_event_handle(p);
            }
            //--------hci le event: le adv report event ----------------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)  // ADV packet
            {
                //after controller is set to scan state, it will report all the adv packet it received by this event
                #if UI_CONTROL_ENABLE
                    app_parse_foundAdv(p);
                #endif
                app_le_adv_report_event_handle(p);
            }
            //------hci le event: le connection update complete event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE)  // connection update
            {
                app_le_connection_update_complete_event_handle(p);
            }
            else if (subEvt_code == HCI_SUB_EVT_LE_DATA_LENGTH_CHANGE){
                hci_le_dataLengthChangeEvt_t    *pDle = (hci_le_dataLengthChangeEvt_t *)p;
                tlkapi_send_string_data(APP_LOG_EN, "dle change", pDle, sizeof(hci_le_dataLengthChangeEvt_t));
            }
            #if CS_PROCEDURE_EXCHANGE
                else if (subEvt_code == HCI_SUB_EVT_LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES_COMPLETE){
                    user_setCsProcCmpltStatus(index,CAP_EXCH_CMPLT);
                    #if UI_CONTROL_ENABLE
                        app_parse_printf("cs capability exchange success\r\n");
                    #endif
                }
                else if (subEvt_code == HCI_SUB_EVT_LE_CS_READ_REMOTE_FAE_TABLE_COMPLETE){
                    user_setCsProcCmpltStatus(index,FAE_EXCH_CMPLT);
                    #if UI_CONTROL_ENABLE
                        app_parse_printf("cs fae exchange success\r\n");
                    #endif
                }
                else if (subEvt_code == HCI_SUB_EVT_LE_CS_CONFIG_COMPLETE){
                    user_setCsProcCmpltStatus(index,CFG_EXCH_CMPLT);
                    #if UI_CONTROL_ENABLE
                        app_parse_printf("cs cfg exchange success\r\n");
                    #endif
                }
                else if (subEvt_code == HCI_SUB_EVT_LE_CS_SECURITY_ENABLE_COMPLETE){
                    user_setCsProcCmpltStatus(index,SEC_EXCH_CMPLT);
                    #if UI_CONTROL_ENABLE
                        app_parse_printf("cs sec exchange success\r\n");
                    #endif
                }
                else if (subEvt_code == HCI_SUB_EVT_LE_CS_PROCEDURE_ENABLE_COMPLETE){
                    user_setCsProcCmpltStatus(index,CS_PROC_EN_CMPLT);

                    #if UI_CONTROL_ENABLE
                    app_parse_printf("cs measurement start\r\n");
                    hci_le_csProcedureEnableCompleteEvt_t *pCsComplete = (hci_le_csProcedureEnableCompleteEvt_t *)p;

                    blc_rass_procedureEnComplete(pCsComplete);
                    #endif
                }

                //------HCI LE event: LE CS Subevent Result event-------------------------------
                else if (subEvt_code == HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT)
                {
                    hci_le_csSubeventResultEvt_t *pCsSubevent = (hci_le_csSubeventResultEvt_t *)p;

                    u16 data_len = sizeof(hci_le_csSubeventResultEvt_t) + 3 + pCsSubevent->Step_Mode->len;(void)data_len;
                    //tlkapi_printf(APP_CS_DIST_EN, "[APP][CS]SubeventResult,mode=%d,len=%d,%s", pCsSubevent->Step_Mode->mode, data_len, hex_to_str(pCsSubevent, data_len));
                    blc_rass_subeventResultData(pCsSubevent);
                }
                //------HCI LE event: LE CS Subevent Result Continue event-------------------------------
                else if (subEvt_code == HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT_CONTINUE)
                {
                    hci_le_csSubeventResultContinueEvt_t *pCsSubevent = (hci_le_csSubeventResultContinueEvt_t *)p;

                    u16 data_len = sizeof(hci_le_csSubeventResultContinueEvt_t) + 3 + pCsSubevent->Step_Mode->len;(void)data_len;
                    //tlkapi_printf(APP_CS_DIST_EN, "[APP][CS]SubeventResultContinue,mode=%d,len=%d,%s", pCsSubevent->Step_Mode->mode, data_len, hex_to_str(pCsSubevent, data_len));
                    blc_rass_subeventResultContinueData(pCsSubevent);
                }
            #endif
        }
    }


    return 0;

}




/**
 * @brief      BLE host event handler call-back.
 * @param[in]  h       event type
 * @param[in]  para    Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_host_event_callback (u32 h, u8 *para, int n)
{
    (void)n;
    u8 event = h & 0xFF;

    switch(event)
    {
        case GAP_EVT_SMP_PAIRING_BEGIN:
        {

        }
        break;

        case GAP_EVT_SMP_PAIRING_SUCCESS:
        {

        }
        break;

        case GAP_EVT_SMP_PAIRING_FAIL:
        {
        #if (ACL_CENTRAL_SMP_ENABLE)
            gap_smp_pairingFailEvt_t *p = (gap_smp_pairingFailEvt_t *)para;

            if( dev_char_get_conn_role_by_connhandle(p->connHandle) == ACL_ROLE_CENTRAL){
                if(central_smp_pending == p->connHandle){
                    central_smp_pending = 0;
                }
            }
        #endif
        }
        break;

        case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
        {

        }
        break;

        case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
        {
            gap_smp_connEncDoneEvt_t* p = (gap_smp_connEncDoneEvt_t*)para;

            #if (ACL_CENTRAL_SMP_ENABLE)
                if( dev_char_get_conn_role_by_connhandle(p->connHandle) == ACL_ROLE_CENTRAL){

                    if(central_smp_pending == p->connHandle){
                        central_smp_pending = 0;
                    }
                }
            #endif

            #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)  //SMP finish
                if(central_sdp_pending == p->connHandle){  //SDP is pending
                    app_register_service(&app_service_discovery);  //start SDP now
                }
            #endif
        }
        break;

        case GAP_EVT_SMP_TK_DISPLAY:
        {

        }
        break;

        case GAP_EVT_SMP_TK_REQUEST_PASSKEY:
        {

        }
        break;

        case GAP_EVT_SMP_TK_REQUEST_OOB:
        {

        }
        break;

        case GAP_EVT_SMP_TK_NUMERIC_COMPARE:
        {

        }
        break;

        case GAP_EVT_ATT_EXCHANGE_MTU:
        {

        }
        break;

        case GAP_EVT_GATT_HANDLE_VALUE_CONFIRM:
        {

        }
        break;

        default:
        break;
    }

    return 0;
}



#define         HID_HANDLE_CONSUME_REPORT           25
#define         HID_HANDLE_KEYBOARD_REPORT          29
#define         AUDIO_HANDLE_MIC                    52
#define         OTA_HANDLE_DATA                     48

/**
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler (u16 connHandle, u8 *pkt)
{
    if( dev_char_get_conn_role_by_connhandle(connHandle) == ACL_ROLE_CENTRAL)   //GATT data for Central
    {
        #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
            if(central_sdp_pending == connHandle ){  //ATT service discovery is ongoing on this conn_handle
                //when service discovery function is running, all the ATT data from peripheral
                //will be processed by it,  user can only send your own att cmd after  service discovery is over
                host_att_client_handler (connHandle, pkt); //handle this ATT data by service discovery process
            }
        #endif

        rf_packet_att_t *pAtt = (rf_packet_att_t*)pkt;

        //so any ATT data before service discovery will be dropped
        dev_char_info_t* dev_info = dev_char_info_search_by_connhandle (connHandle);
        if(dev_info)
        {
            //-------   user process ------------------------------------------------
            u16 attHandle = pAtt->handle;

            if(pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI)
            {
                    //---------------   consumer key --------------------------
                #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                    if(attHandle == dev_info->char_handle[3])  // Consume Report In (Media Key)
                #else
                    if(attHandle == HID_HANDLE_CONSUME_REPORT)   //Demo device(825x_ble_sample) Consume Report AttHandle value is 25
                #endif
                    {
                        att_keyboard_media (connHandle, pAtt->dat);
                    }
                    //---------------   keyboard key --------------------------
                #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                    else if(attHandle == dev_info->char_handle[4])     // Key Report In
                #else
                    else if(attHandle == HID_HANDLE_KEYBOARD_REPORT)   // Demo device(825x_ble_sample) Key Report AttHandle value is 29
                #endif
                    {
                        att_keyboard (connHandle, pAtt->dat);
                    }
                #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                    else if(attHandle == dev_info->char_handle[0])     // AUDIO Notify
                #else
                    else if(attHandle == AUDIO_HANDLE_MIC)   // Demo device(825x_ble_remote) Key Report AttHandle value is 52
                #endif
                    {

                    }
                    else
                    {

                    }
            }
            else if (pAtt->opcode == ATT_OP_HANDLE_VALUE_IND)
            {

            }
        }

        /* The Central does not support GATT Server by default */
        if(!(pAtt->opcode & 0x01)){
            switch(pAtt->opcode){
                case ATT_OP_FIND_INFO_REQ:
                case ATT_OP_FIND_BY_TYPE_VALUE_REQ:
                case ATT_OP_READ_BY_TYPE_REQ:
                case ATT_OP_READ_BY_GROUP_TYPE_REQ:
                    blc_gatt_pushErrResponse(connHandle, pAtt->opcode, pAtt->handle, ATT_ERR_ATTR_NOT_FOUND);
                    break;
                case ATT_OP_READ_REQ:
                case ATT_OP_READ_BLOB_REQ:
                case ATT_OP_READ_MULTI_REQ:
                case ATT_OP_WRITE_REQ:
                case ATT_OP_PREPARE_WRITE_REQ:
                    blc_gatt_pushErrResponse(connHandle, pAtt->opcode, pAtt->handle, ATT_ERR_INVALID_HANDLE);
                    break;
                case ATT_OP_EXECUTE_WRITE_REQ:
                case ATT_OP_HANDLE_VALUE_CFM:
                case ATT_OP_WRITE_CMD:
                case ATT_OP_SIGNED_WRITE_CMD:
                    //ignore
                    break;
                default://no action
                    break;
            }
        }
    }
    else{   //GATT data for Peripheral


    }


    return 0;
}



#if (BATT_CHECK_ENABLE)  //battery check must do before OTA relative operation

_attribute_data_retention_  u32 lowBattDet_tick   = 0;

/**
 * @brief       this function is used to process battery power.
 *              The low voltage protection threshold 2.0V is an example and reference value. Customers should
 *              evaluate and modify these thresholds according to the actual situation. If users have unreasonable designs
 *              in the hardware circuit, which leads to a decrease in the stability of the power supply network, the
 *              safety thresholds must be increased as appropriate.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void user_battery_power_check(u16 alarm_vol_mv)
{
    /*For battery-powered products, as the battery power will gradually drop, when the voltage is low to a certain
      value, it will cause many problems.
        a) When the voltage is lower than operating voltage range of chip, chip can no longer guarantee stable operation.
        b) When the battery voltage is low, due to the unstable power supply, the write and erase operations
            of Flash may have the risk of error, causing the program firmware and user data to be modified abnormally,
            and eventually causing the product to fail. */
    u8 battery_check_returnValue=0;
    if(analog_read(USED_DEEP_ANA_REG) & LOW_BATT_FLG){
        battery_check_returnValue=app_battery_power_check(alarm_vol_mv+200);
    }
    else{
        battery_check_returnValue=app_battery_power_check(alarm_vol_mv);
    }
    if(battery_check_returnValue)
    {
        analog_write_reg8(USED_DEEP_ANA_REG,  analog_read_reg8(USED_DEEP_ANA_REG) &(~LOW_BATT_FLG));  //clr
    }
    else
    {
        #if (UI_LED_ENABLE)  //led indicate
            for(int k=0;k<3;k++){
                gpio_write(GPIO_LED_BLUE, LED_ON_LEVEL);
                sleep_us(200000);
                gpio_write(GPIO_LED_BLUE, !LED_ON_LEVEL);
                sleep_us(200000);
            }
        #endif
        analog_write_reg8(USED_DEEP_ANA_REG,  analog_read_reg8(USED_DEEP_ANA_REG) | LOW_BATT_FLG);  //mark

        #if (UI_KEYBOARD_ENABLE)
        u32 pin[] = KB_DRIVE_PINS;
        for (unsigned int i=0; i<(sizeof (pin)/sizeof(*pin)); i++)
        {
            cpu_set_gpio_wakeup (pin[i], 1, 1);  //drive pin pad high wakeup deepsleep
        }

        cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  //deepsleep
        #endif
    }
}

#endif

#if (APP_FLASH_PROTECTION_ENABLE)

/**
 * @brief      flash protection operation, including all locking & unlocking for application
 *             handle all flash write & erase action for this demo code. use should add more more if they have more flash operation.
 * @param[in]  flash_op_evt - flash operation event, including application layer action and stack layer action event(OTA write & erase)
 *             attention 1: if you have more flash write or erase action, you should should add more type and process them
 *             attention 2: for "end" event, no need to pay attention on op_addr_begin & op_addr_end, we set them to 0 for
 *                          stack event, such as stack OTA write new firmware end event
 * @param[in]  op_addr_begin - operating flash address range begin value
 * @param[in]  op_addr_end - operating flash address range end value
 *             attention that, we use: [op_addr_begin, op_addr_end)
 *             e.g. if we write flash sector from 0x10000 to 0x20000, actual operating flash address is 0x10000 ~ 0x1FFFF
 *                  but we use [0x10000, 0x20000):  op_addr_begin = 0x10000, op_addr_end = 0x20000
 * @return     none
 */
_attribute_data_retention_ u16  flash_lockBlock_cmd;
void app_flash_protection_operation(u8 flash_op_evt, u32 op_addr_begin, u32 op_addr_end)
{
    if(flash_op_evt == FLASH_OP_EVT_APP_INITIALIZATION)
    {
        /* ignore "op addr_begin" and "op addr_end" for initialization event
         * must call "flash protection_init" first, will choose correct flash protection relative API according to current internal flash type in MCU */
        flash_protection_init();

        /* just sample code here, protect all flash area for old firmware and OTA new firmware.
         * user can change this design if have other consideration */
        u32 app_lockBlock = FLASH_LOCK_FW_LOW_256K; //just demo value, user can change this value according to application

        flash_lockBlock_cmd = flash_change_app_lock_block_to_flash_lock_block(app_lockBlock);

        tlkapi_send_string_data(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] initialization, lock flash", 0, 0);
        flash_lock(flash_lockBlock_cmd);
    }
    /* add more flash protection operation for your application if needed */
}

#endif

static int app_prf_sdp_end(u16 connHandle, u8 *pData, u16 dataLen)
{
    (void)connHandle;(void)dataLen;
    blc_prf_sdpEndEvt_t* evt = (blc_prf_sdpEndEvt_t*)pData;
    if (evt->svcId == CS_RAS_CLIENT) {
        tlkapi_printf(APP_LOG_EN, "[APP][CS] RAS service has been found.");
    }

    return 0;
}

static int app_cs_procedure_data(u16 connHandle, u8 *pData, u16 dataLen)
{
    (void)dataLen;
    tlkapi_send_string_data(APP_LOG_EN, "[APP][CS] On-demand procedure data", pData, 128);

    ras_rangingData_t *rangingData = (ras_rangingData_t *)pData;
    u8 *data = NULL;
    u32 len = 0;
    if(rangingData->onDemandDataFlag==1)
    {
        data = rangingData->proc_data->rangingData;
        len = rangingData->proc_data->rangingDataLen;
    }

    float distance1 = 0;
    float distance2 = 0;
    u32 retval = blc_rass_calcRangData(connHandle, data, len, &distance1, &distance2);
    if(retval==0)
    {
        tlkapi_printf(1, "channel sounding distance: %f", distance1);
        app_parse_printf("channel sounding distance: %f\r\n", distance1);
    }
    else
    {
        tlkapi_printf(1, "channel sounding distance, error code = %08X", retval);
        app_parse_printf("channel sounding distance, error code = %08X\r\n", retval);
    }

    return 0;
}

static const app_prf_evtCb_t csPeripheralEvt [] = {
    {PRF_EVTID_CLIENT_SDP_END, app_prf_sdp_end},
    {CS_EVT_PROCEDURE_DATA, app_cs_procedure_data},
};

PRF_EVT_CB(csPeripheralEvt)


///////////////////////////////////////////

/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */
_attribute_no_inline_ void user_init_normal(void)
{
//////////////////////////// basic hardware Initialization  Begin //////////////////////////////////
    /* random number generator must be initiated here( in the beginning of user_init_normal).
     * When deepSleep retention wakeUp, no need initialize again */
    random_generator_init();

    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_init();
        blc_debug_addStackLog(STK_LOG_SMP_LTK|STK_LOG_SMP_LTK);
    #endif

    #if (BATT_CHECK_ENABLE)
    /*The SDK must do a quick low battery detect during user initialization instead of waiting
      until the main_loop. The reason for this process is to avoid application errors that the device
      has already working at low power.
      Considering the working voltage of MCU and the working voltage of flash, if the Demo is set below 2.0V,
      the chip will alarm and deep sleep (Due to PM does not work in the current version of B92, it does not go
      into deepsleep), and once the chip is detected to be lower than 2.0V, it needs to wait until the voltage rises to 2.2V,
      the chip will resume normal operation. Consider the following points in this design:
        At 2.0V, when other modules are operated, the voltage may be pulled down and the flash will not
        work normally. Therefore, it is necessary to enter deepsleep below 2.0V to ensure that the chip no
        longer runs related modules;
        When there is a low voltage situation, need to restore to 2.2V in order to make other functions normal,
        this is to ensure that the power supply voltage is confirmed in the charge and has a certain amount of
        power, then start to restore the function can be safer.*/

        user_battery_power_check(2000);
    #endif

    blc_readFlashSize_autoConfigCustomFlashSector();

    /* attention that this function must be called after "blc readFlashSize_autoConfigCustomFlashSector" !!!*/
    blc_app_loadCustomizedParameters_normal();

    #if (APP_FLASH_PROTECTION_ENABLE)
        app_flash_protection_operation(FLASH_OP_EVT_APP_INITIALIZATION, 0, 0);
        blc_appRegisterStackFlashOperationCallback(app_flash_protection_operation); //register flash operation callback for stack
    #endif

//////////////////////////// basic hardware Initialization  End //////////////////////////////////


//////////////////////////// BLE stack Initialization  Begin //////////////////////////////////

    u8  mac_public[6];
    u8  mac_random_static[6];
    
    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);

    //////////// LinkLayer Initialization  Begin /////////////////////////

    blc_ll_initBasicMCU();

    blc_ll_initStandby_module(mac_public);

    blc_ll_initLegacyScanning_module();

    blc_ll_initLegacyInitiating_module();

    blc_ll_initAclConnection_module();

    blc_ll_initAclCentralRole_module();

    blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

    blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_CENTRAL_MAX_TX_OCTETS, ACL_PERIPHR_MAX_TX_OCTETS);

    /* all ACL connection share same RX FIFO */
    blc_ll_initAclConnRxFifo(app_acl_rx_fifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
    /* ACL Central TX FIFO */
    blc_ll_initAclCentralTxFifo(app_acl_cen_tx_fifo, ACL_CENTRAL_TX_FIFO_SIZE, ACL_CENTRAL_TX_FIFO_NUM, ACL_CENTRAL_MAX_NUM);

    blc_ll_setAclCentralBaseConnectionInterval(CONN_INTERVAL_31P25MS);

    rf_set_power_level_index(RF_POWER_P3dBm);

    blc_ll_configScanEnableStrategy (SCAN_STRATEGY_0);

    //channel sounding
    blc_ll_initCsModule_initConfigParametersBuffer(cs_buff, CS_BUFF_COUNT);
    blc_ll_initCsRxFifo(app_cs_rx_buf, CS_RX_FIFO_SIZE, CS_RX_FIFO_NUM);

    //////////// LinkLayer Initialization  End /////////////////////////



    //////////// HCI Initialization  Begin /////////////////////////
    blc_hci_registerControllerDataHandler(blc_l2cap_pktHandler_5_3);

    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

    //bluetooth event
    blc_hci_setEventMask_cmd (HCI_EVT_MASK_DISCONNECTION_COMPLETE);

    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(        HCI_LE_EVT_MASK_CONNECTION_COMPLETE  \
                                    |   HCI_LE_EVT_MASK_ADVERTISING_REPORT \
                                    |   HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE\
                                    |   HCI_LE_EVT_MASK_DATA_LENGTH_CHANGE\
                                    |   HCI_LE_EVT_MASK_2_CS_READ_REMOTE_SUPPORTED_CAPABILITIES_COMPLETE\
                                    |   HCI_LE_EVT_MASK_2_CS_READ_REMOTE_FAE_TABLE_COMPLETE\
                                    |   HCI_LE_EVT_MASK_2_CS_SECURITY_ENABLE_COMPLETE\
                                    |   HCI_LE_EVT_MASK_2_CS_CONFIG_COMPLETE\
                                    |   HCI_LE_EVT_MASK_2_CS_PROCEDURE_ENABLE_COMPLETE);
    blc_hci_le_setEventMask_2_cmd(      HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT\
                                    |   HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT_CONTINUE);

    //////////// HCI Initialization  End /////////////////////////


    //////////// Host Initialization  Begin /////////////////////////
    /* Host Initialization */
    /* GAP initialization must be done before any other host feature initialization !!! */
    blc_gap_init();

    /* L2CAP data buffer Initialization */
    blc_l2cap_initAclCentralBuffer(app_cen_l2cap_rx_buf, CENTRAL_L2CAP_BUFF_SIZE, app_cen_l2cap_tx_buf, CENTRAL_L2CAP_BUFF_SIZE);

    blc_att_setCentralRxMtuSize(CENTRAL_ATT_RX_MTU); ///must be placed after "blc_gap_init"

    /* GATT Initialization */
    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
        host_att_register_idle_func (main_idle_loop);
    #endif
    blc_gatt_register_data_handler(app_gatt_data_handler);

    /* SMP Initialization */
    #if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
        
        blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);
    #endif

    #if (0)
        blc_smp_setSecurityLevel_periphr(Unauthenticated_Pairing_with_Encryption);  //LE_Security_Mode_1_Level_2
    #else
        blc_smp_setSecurityLevel_periphr(No_Security);
    #endif

    #if (ACL_CENTRAL_SMP_ENABLE)
        blc_smp_setSecurityLevel_central(Unauthenticated_Pairing_with_Encryption);  //LE_Security_Mode_1_Level_2
    #else
        blc_smp_setSecurityLevel_central(No_Security);
    #endif

    blc_smp_smpParamInit();


    //host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
    blc_gap_registerHostEventHandler( app_host_event_callback );
    blc_gap_setEventMask( GAP_EVT_MASK_SMP_PAIRING_BEGIN            |  \
                          GAP_EVT_MASK_SMP_PAIRING_SUCCESS          |  \
                          GAP_EVT_MASK_SMP_PAIRING_FAIL             |  \
                          GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE);

    //RAS client initial
    blc_prf_initialModule(app_prf_eventCb);
    blc_cs_registerRasProfileControlClient(NULL);
    //////////// Host Initialization  End /////////////////////////



    /* Check if any Stack(Controller & Host) Initialization error after all BLE initialization done.
     * attention: user can not delete !!! */
    u32 error_code1 = blc_contr_checkControllerInitialization();
    u32 error_code2 = blc_host_checkHostInitialization();
    if(error_code1 != INIT_SUCCESS || error_code2 != INIT_SUCCESS){
        /* It's recommended that user set some UI alarm to know the exact error, e.g. LED shine, print log */
        #if (UI_LED_ENABLE)
            gpio_write(GPIO_LED_RED, LED_ON_LEVEL);
        #endif

        #if (TLKAPI_DEBUG_ENABLE)
            tlkapi_printf(APP_LOG_EN, "[APP][INI] Stack INIT ERROR 0x%04x, 0x%04x", error_code1, error_code2);
            while(1){
                tlkapi_debug_handler();
            }
        #else
            while(1);
        #endif
    }

//////////////////////////// BLE stack Initialization  End //////////////////////////////////




//////////////////////////// User Configuration for BLE application ////////////////////////////
    blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
    #if UI_CONTROL_ENABLE
        app_parse_ui_init();
    #else
        blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);
    #endif

    u8 lib_ver[5];
    blc_get_sdk_version(lib_ver, 5);
    tlkapi_send_string_data(1, "[APP][INI] acl central demo Lib version", lib_ver, 5);
////////////////////////////////////////////////////////////////////////////////////////////////

}



/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void)
{

}

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
u32 ledToggleTick = 0;
/**
 * @brief     BLE main idle loop
 * @param[in]  none.
 * @return     none.
 */
int main_idle_loop (void)
{


    ////////////////////////////////////// BLE entry /////////////////////////////////
    blc_sdk_main_loop();

    blc_prf_main_loop();
    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif

    ////////////////////////////////////// UI entry /////////////////////////////////
    #if (BATT_CHECK_ENABLE)
        /*The frequency of low battery detect is controlled by the variable lowBattDet_tick, which is executed every
         500ms in the demo. Users can modify this time according to their needs.*/
        if(battery_get_detect_enable() && clock_time_exceed(lowBattDet_tick, 500000) ){
            lowBattDet_tick = clock_time();
            user_battery_power_check(BAT_DEEP_THRESHOLD_MV);
        }
    #endif

    #if (UI_BUTTON_ENABLE)
        static u8 button_detect_en = 0;
        if(!button_detect_en && clock_time_exceed(0, 1000000)){// process button 1 second later after power on
            button_detect_en = 1;
        }
        if(button_detect_en){
            proc_button();  //button triggers pair & unpair  and OTA
        }
    #elif (UI_KEYBOARD_ENABLE)
        proc_keyboard (0, 0, 0);
    #endif


    proc_central_role_unpair();
    #if UI_CONTROL_ENABLE
        app_parse_ui_loop();
    #endif
    #if CS_PROCEDURE_EXCHANGE
        cs_procedure_ctrl();
    #endif

    return 0; //must return 0 due to SDP flow
}



/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void main_loop (void)
{
    if(clock_time_exceed(ledToggleTick, 1000 * 1000))
    {  //led toggle interval: 1000mS
        ledToggleTick = clock_time();
        gpio_toggle(GPIO_LED_BLUE);
    }
    main_idle_loop ();



    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
        simple_sdp_loop ();
    #endif
}





