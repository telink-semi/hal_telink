/********************************************************************************************************
 * @file    app_ui.c
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
#include "app_ui.h"



#if (INTER_TEST_MODE == TEST_BIS_AUDIO_SENDER)

int central_pairing_enable = 0;
u16 central_unpair_enable = 0;

u16 central_disconnect_connhandle;   //mark the central connection which is in un_pair disconnection flow




#if (UI_BUTTON_ENABLE)

/////////////////////////////////////////////////////////////////////
    #define MAX_BTN_SIZE            2
    #define BTN_VALID_LEVEL         0
    #define BTN_PAIR                0x01
    #define BTN_UNPAIR              0x02

    u32 ctrl_btn[] = {SW1_GPIO, SW2_GPIO};
    u8 btn_map[MAX_BTN_SIZE] = {BTN_PAIR, BTN_UNPAIR};
    /**
     * @brief   record the result of key detect
     */
    typedef struct{
        u8  cnt;                //count button num
        u8  btn_press;
        u8  keycode[MAX_BTN_SIZE];          //6 btn
    }vc_data_t;
    vc_data_t vc_event;

    /**
     * @brief   record the status of button process
     */
    typedef struct{
        u8  btn_history[4];     //vc history btn save
        u8  btn_filter_last;
        u8  btn_not_release;
        u8  btn_new;                    //new btn  flag
    }btn_status_t;
    btn_status_t    btn_status;

    /**
     * @brief      Debounce processing during button detection
     * @param[in]  btn_v - vc_event.btn_press
     * @return     1:Detect new button;0:Button isn't changed
     */
    u8 btn_debounce_filter(u8 *btn_v)
    {
        u8 change = 0;

        for(int i=3; i>0; i--){
            btn_status.btn_history[i] = btn_status.btn_history[i-1];
        }
        btn_status.btn_history[0] = *btn_v;

        if(  btn_status.btn_history[0] == btn_status.btn_history[1] && btn_status.btn_history[1] == btn_status.btn_history[2] && \
            btn_status.btn_history[0] != btn_status.btn_filter_last ){
            change = 1;

            btn_status.btn_filter_last = btn_status.btn_history[0];
        }

        return change;
    }

    /**
     * @brief      This function is key detection processing
     * @param[in]  read_key - Decide whether to return the key detection result
     * @return     1:Detect new button;0:Button isn't changed
     */
    u8 vc_detect_button(int read_key)
    {
        u8 btn_changed, i;
        memset(&vc_event,0,sizeof(vc_data_t));          //clear vc_event
        //vc_event.btn_press = 0;

        for(i=0; i<MAX_BTN_SIZE; i++){
            if(BTN_VALID_LEVEL != !gpio_read(ctrl_btn[i])){
                vc_event.btn_press |= BIT(i);
            }
        }

        btn_changed = btn_debounce_filter(&vc_event.btn_press);


        if(btn_changed && read_key){
            for(i=0; i<MAX_BTN_SIZE; i++){
                if(vc_event.btn_press & BIT(i)){
                    vc_event.keycode[vc_event.cnt++] = btn_map[i];
                }
            }

            return 1;
        }

        return 0;
    }

    /**
     * @brief       this function is used to detect if button pressed or released.
     * @param[in]   none
     * @return      none
     */
    void proc_button (void)
    {
        static u32 button_det_tick;
        if(clock_time_exceed(button_det_tick, 10*1000))
        {
            button_det_tick = clock_time();
        }
        else{
            return;
        }

        int det_key = vc_detect_button (1);

        if (det_key)  //key change: press or release
        {
            u8 key0 = vc_event.keycode[0];
            if(vc_event.cnt == 2)  //two key press
            {

            }
            else if(vc_event.cnt == 1) //one key press
            {
                if(key0 == BTN_PAIR)
                {
                    central_pairing_enable = 1;
                    tlkapi_send_string_data(APP_LOG_EN, "[UI][PAIR] Pair begin", 0, 0);
                }
                else if(key0 == BTN_UNPAIR)
                {
                    /*Here is just Telink Demonstration effect. Cause the demo board has limited key to use, only one "un_pair" key is
                     available. When "un_pair" key pressed, we will choose and un_pair one device in connection state */
                    if(acl_conn_central_num){ //at least 1 central connection exist

                        if(!central_disconnect_connhandle){  //if one central un_pair disconnection flow not finish, here new un_pair not accepted

                            // We choose the earliest connected device to un_pair, conn_dev_list[0] is the earliest connected device.
                            // Attention: when acl_conn_central_num none zero, conn_dev_list[0].conn_state is definite 1, no need to judge
                            central_unpair_enable = conn_dev_list[0].conn_handle;  //mark connHandle on central_unpair_enable
                            tlkapi_send_string_data(APP_LOG_EN, "[UI][PAIR] Unpair", &central_unpair_enable, 2);
                        }
                    }
                }
            }
            else{  //release
                if(central_pairing_enable){
                    central_pairing_enable = 0;
                    tlkapi_send_string_data(APP_LOG_EN, "[UI][PAIR] Pair end", 0, 0);
                }

                if(central_unpair_enable){
                    central_unpair_enable = 0;
                }
            }

        }


    }
#endif   //end of UI_BUTTON_ENABLE




#if (UI_KEYBOARD_ENABLE)

_attribute_ble_data_retention_  int     key_not_released;


#define CONSUMER_KEY                1
#define KEYBOARD_KEY                2
#define PAIR_UNPAIR_KEY             3

_attribute_ble_data_retention_  u8      key_type;

/**
 * @brief   Check changed key value.
 * @param   none.
 * @return  none.
 */
void key_change_proc(void)
{
    u8 key0 = kb_event.keycode[0];
    key_not_released = 1;
    if (kb_event.cnt == 2)   //two key press
    {

    }
    else if(kb_event.cnt == 1)
    {
        if(key0 == SW2_K1){
            BLT_APP_LOG("SW2 K1");
        }
        else if(key0 == SW3_K2 )
        {
            blc_ll_setExtAdvEnable( BLC_ADV_DISABLE, ADV_HANDLE0, 0 , 0);
            blc_ll_setPeriodicAdvEnable( BLC_ADV_DISABLE, ADV_HANDLE0);

            gpio_write(GPIO_LED_GREEN, 1);
            BLT_APP_LOG("stop extended adv and periodic adv");
        }
        else if(key0 == SW4_K3)
        {
            BLT_APP_LOG("SW4 K3");
        }
        else if(key0 == SW5_K4)
        {
            extern ble_sts_t    blc_ll_terminateBig(u8 bigHandle, u8 reason);
            u8 status0 = blc_ll_terminateBig(BIG_HANDLE_0, HCI_ERR_REMOTE_USER_TERM_CONN);

            #if (SLEV_terminate_big)
                log_event_irq(SL_STACK_BIS_SOURCE_TIMING_EN, SLEV_terminate_big);
            #endif
            sleep_ms(500); //wait terminate control pdu complete. terminate control pdu need 18 big event.


            gpio_write(GPIO_LED_GREEN, 0);

            //must after 18 big event. about 18*big_interval = 180ms
            blt_ll_reset_big_bcst();

            blc_ll_setExtAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0, 0 , 0);
            blc_ll_setPeriodicAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0);
            BLT_APP_LOG("start extended adv and periodic adv");


            extern hci_le_createBigParamsTest_t gBigCreateTstParam;
            ble_sts_t status1 = blc_hci_le_createBigParamsTest(&gBigCreateTstParam);
            BLT_APP_LOG("create big statue:0x%x", status1);
        }
    }
    else   //kb_event.cnt == 0,  key release
    {
        key_not_released = 0;
        if(key_type == CONSUMER_KEY)
        {

        }
        else if(key_type == KEYBOARD_KEY)
        {

        }
        else if(key_type == PAIR_UNPAIR_KEY){

        }
    }


}



_attribute_ble_data_retention_      static u32 keyScanTick = 0;

/**
 * @brief      keyboard task handler
 * @param[in]  e    - event type
 * @param[in]  p    - Pointer point to event parameter.
 * @param[in]  n    - the length of event parameter.
 * @return     none.
 */
void proc_keyboard (u8 e, u8 *p, int n)
{
    if(clock_time_exceed(keyScanTick, 10 * 1000)){  //keyScan interval: 10mS
        keyScanTick = clock_time();
    }
    else{
        return;
    }

    kb_event.keycode[0] = 0;
    int det_key = kb_scan_key (0, 1);

    if (det_key){
        key_change_proc();
    }
}




#endif   //end of UI_KEYBOARD_ENABLE







/**
 * @brief       This function is used to send consumer HID report by USB.
 * @param[in]   conn     - connection handle
 * @param[in]   p        - Pointer point to data buffer.
 * @return
 */
void    att_keyboard_media (u16 conn, u8 *p)
{
    u16 consumer_key = p[0] | p[1]<<8;

#if (APPLICATION_DONGLE)
    usbkb_report_consumer_key(consumer_key);
#else
    #if (UI_LED_ENABLE) //Demo effect: when peripheral send Vol+/Vol- to central, LED GPIO toggle to show the result
        if(consumer_key == MKEY_VOL_UP){
            gpio_toggle(GPIO_LED_GREEN);
        }
        else if(consumer_key == MKEY_VOL_DN){
            gpio_toggle(GPIO_LED_BLUE);
        }
    #endif
#endif
}


//////////////// keyboard ///////////////////////////////////////////////////
kb_data_t       kb_dat_report = {1, 0, {0,0,0,0,0,0} };
int keyboard_not_release = 0;
/**
 * @brief       This function is used to send HID report by USB.
 * @param[in]   conn     - connection handle
 * @param[in]   p        - Pointer point to data buffer.
 * @return
 */
void    att_keyboard (u16 conn, u8 *p)
{
    memcpy(&kb_dat_report, p, sizeof(kb_data_t));

#if (APPLICATION_DONGLE)
    if (kb_dat_report.keycode[0])           //keycode[0]
    {
        kb_dat_report.cnt = 1;  //1 key value
        keyboard_not_release = 1;
    }
    else{
        kb_dat_report.cnt = 0;  //key release
        keyboard_not_release = 0;
    }
    usbkb_hid_report((kb_data_t *) &kb_dat_report);
#endif
}


#endif
