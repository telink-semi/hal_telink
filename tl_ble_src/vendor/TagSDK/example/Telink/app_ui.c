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
#include "app_att.h"
#include "app_ui.h"

#if (FREERTOS_ENABLE)
    #include "tlk_riscv.h"
    #include <FreeRTOS.h>
    #include <task.h>
    #include <timers.h>
    #include <queue.h>
    #include <event_groups.h>
    #include "app_freertos.h"
#endif
void Portble_btn_press(void);
void Portble_btn_d_press(void);
void Portble_btn_l_press(void);
int app_button_press_proc(void);
int app_button_long_press(void);

#if UI_LED_ENABLE
int app_led_flash(void);
void app_button_indicate_stop(void);
#endif

#if FREERTOS_ENABLE
#define CONV_MS_TO_TICKS(x)    (x/2)

#define BTN_PRESS_TIME_MS 1000
#define BTN_PRESS_LONG_TIME_MS 3000

_attribute_data_retention_ TimerHandle_t   btnPressTimer;
_attribute_data_retention_ TimerHandle_t   btnLongPressTimer ;
_attribute_data_retention_ TimerHandle_t   btnflashTimer ;
_attribute_data_retention_ TimerHandle_t   btnindTimer ;

static void app_ui_btn_press_timer_handler(TimerHandle_t timer)
{
    (void)timer;
    TagPutPostWork(app_button_press_proc, NULL);
}

static void app_ui_btn_lpress_timer_handler(TimerHandle_t timer)
{
    (void)timer;
    TagPutPostWork(app_button_long_press, NULL);
}

static  void app_ui_stop_btn_timer(void)
{
    xTimerStop(btnPressTimer,0);
}


static  void app_ui_start_btn_timer(void)
{
    xTimerStart(btnPressTimer,0);
}


static  void app_ui_stop_l_btn_timer(void)
{
    xTimerStop(btnLongPressTimer,0);
}


static void app_ui_start_l_btn_timer(void)
{
    xTimerStart(btnLongPressTimer,0);
}
#if UI_LED_ENABLE

static  void app_ui_stop_flash_timer(void)
{
    xTimerStop(btnflashTimer,0);
}


static  void app_ui_start_flash_timer(void)
{
    xTimerStart(btnflashTimer,0);
}


static  void app_ui_start_ind_timer(void)
{
    xTimerStart(btnindTimer,0);
}
#endif

void app_ui_os_timer_init(void)
{

       btnPressTimer = xTimerCreate("btnPressTimer",                                                 /* Text name. */
                                                             CONV_MS_TO_TICKS(BTN_PRESS_TIME_MS), /* Timer period. */
                                                             false,                                                                           /* Disable auto reload. */
                                                             1,                                                                   /* ID as tagContext */
                                                             app_ui_btn_press_timer_handler);

       btnLongPressTimer = xTimerCreate("btnLPressTimer",                                                 /* Text name. */
                                                             CONV_MS_TO_TICKS(BTN_PRESS_LONG_TIME_MS), /* Timer period. */
                                                             true,                                                                           /* Disable auto reload. */
                                                             2,                                                                   /* ID as tagContext */
                                                             app_ui_btn_lpress_timer_handler);
#if UI_LED_ENABLE

       btnflashTimer = xTimerCreate("btnflashTimer",                                                 /* Text name. */
                                                             CONV_MS_TO_TICKS(250), /* Timer period. */
                                                             true,                                                                           /* Disable auto reload. */
                                                             3,                                                                   /* ID as tagContext */
                                                             app_led_flash);

       btnindTimer = xTimerCreate("btnindTimer",                                                 /* Text name. */
                                                             CONV_MS_TO_TICKS(2000), /* Timer period. */
                                                             false,                                                                           /* Disable auto reload. */
                                                             4,                                                                   /* ID as tagContext */
                                                             app_button_indicate_stop);
#endif

}


#endif

#if (UI_KEYBOARD_ENABLE)

_attribute_data_retention_    int     key_not_released;
_attribute_ble_data_retention_      u8 key_type;
#define GPIO_WAKEUP_KEYPROC_CNT 3
_attribute_ble_data_retention_ static u32 keyScanTick            = 0;
_attribute_ble_data_retention_ static int gpioWakeup_keyProc_cnt = 0;

_attribute_data_retention_    static u8 key_press_flag = 0;

#define USER_BTN_1                0x01
#define USER_BTN_2                0x02

extern void app_button_proc(bool press);


/**
 * @brief        this function is used to process keyboard matrix status change.
 * @param[in]    none
 * @return      none
 */
void key_change_proc(void)
{

    u8 key0 = kb_event.keycode[0];

    key_not_released = 1;
    if (kb_event.cnt == 2)   //two key press, do  not process
    {
         tlkapi_send_string_data(APP_KEY_LOG_EN, "[UI][KEY2] ", &key0, 1);
    }
    else if(kb_event.cnt == 1)
    {
        tlkapi_send_string_data(APP_KEY_LOG_EN, "[UI][KEY1] ", &key0, 1);
        if(key0 == USER_BTN_1)
        {
            app_button_proc(true);
            key_press_flag = 1;
        }
        else if(key0 == USER_BTN_2)
        {
            app_button_proc(true);
            key_press_flag = 1;
        }
    }
    else   //kb_event.cnt == 0,  key release
    {
        tlkapi_send_string_data(APP_KEY_LOG_EN, "[UI][KEY REL] ", NULL, 0);
        key_not_released = 0;

        if(key_press_flag){
            key_press_flag = 0;
            app_button_proc(false);
        }

    }


}


/**
 * @brief      this function is used to detect if key pressed or released.
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
void proc_keyboard(u8 e, u8 *p, int n)
{
    (void)n;
    (void)p;
//    DBG_CHN7_HIGH;
//    DBG_CHN7_LOW;
    //when key press GPIO wake_up sleep, process key_scan at least GPIO_WAKEUP_KEYPROC_CNT times
    if (e == BLT_EV_FLAG_GPIO_EARLY_WAKEUP) {
        gpioWakeup_keyProc_cnt = GPIO_WAKEUP_KEYPROC_CNT;
    } else if (gpioWakeup_keyProc_cnt) {
        gpioWakeup_keyProc_cnt--;
    }

    if (gpioWakeup_keyProc_cnt || clock_time_exceed(keyScanTick, 10 * 1000)) { //keyScan interval: 10mS
        keyScanTick = clock_time();
    } else {
        return;
    }
    kb_event.keycode[0] = 0;
    int det_key = kb_scan_key (0, 1);

    if (det_key){
        key_change_proc();
    }
}

_attribute_ram_code_ void app_set_kb_wakeup(u8 e, u8 *p, int n)
{
    (void)e;
    (void)p;
    (void)n;
//    DBG_CHN10_HIGH;
//    DBG_CHN10_LOW;
    #if (BLE_APP_PM_ENABLE)
    /* suspend time > 50ms.add GPIO wake_up */
    if (((u32)(blc_pm_getWakeupSystemTick() - clock_time())) > 100 * SYSTEM_TIMER_TICK_1MS) {
        blc_pm_setWakeupSource(PM_WAKEUP_PAD); //GPIO PAD wake_up
    }
    #endif
}

void  test_button_irq(void)
{
   u32 IRQ_PIN = GPIO_PC6;
   gpio_function_en(IRQ_PIN);
   gpio_output_dis(IRQ_PIN);
   gpio_input_en(IRQ_PIN);
   gpio_set_up_down_res(IRQ_PIN, GPIO_PIN_PULLDOWN_100K);
   gpio_set_irq(GPIO_IRQ0, IRQ_PIN, INTR_HIGH_LEVEL);
   gpio_set_irq_mask(GPIO_IRQ_IRQ0);
   plic_interrupt_enable(IRQ_GPIO_IRQ1);
}
/**
 * @brief      keyboard initialization
 * @param[in]  none
 * @return     none.
 */
void keyboard_init(void)
{
//    test_button_irq();
    u32 pin[] = KB_DRIVE_PINS;
    #if (BLE_APP_PM_ENABLE)
    /////////// keyboard GPIO wakeup init ////////
    for (unsigned int i = 0; i < (sizeof(pin) / sizeof(*pin)); i++) {
        pm_set_gpio_wakeup(pin[i], WAKEUP_LEVEL_HIGH, 1); //drive pin pad high level wakeup deepsleep
    }
    blc_ll_registerTelinkControllerEventCallback(BLT_EV_FLAG_SLEEP_ENTER, &app_set_kb_wakeup);
    blc_ll_registerTelinkControllerEventCallback(BLT_EV_FLAG_GPIO_EARLY_WAKEUP, &proc_keyboard);
    #endif

    #if (FREERTOS_ENABLE &&  BLE_APP_PM_ENABLE)
    for (unsigned int i=0; i<(sizeof (pin)/sizeof(*pin)); i++){
        gpio_set_irq(GPIO_IRQ1, pin[i], INTR_HIGH_LEVEL);
        gpio_set_irq_mask(GPIO_IRQ_IRQ1);
        plic_interrupt_enable(IRQ_GPIO_IRQ1);

    }
    extern void proc_keyboardSupend (u8 e, u8 *p, int n);
    blc_ll_registerTelinkControllerEventCallback (BLT_EV_FLAG_GPIO_EARLY_WAKEUP, &proc_keyboardSupend);
    #endif
}

#elif (UI_BUTTON_ENABLE)



/////////////////////////////////////////////////////////////////////
    #define MAX_BTN_SIZE            2
    #define BTN_VALID_LEVEL            0

    #define USER_BTN_1                0x01
    #define USER_BTN_2                0x02

    _attribute_data_retention_    u32 ctrl_btn[] = {SW1_GPIO, SW2_GPIO};
    _attribute_data_retention_    u8 btn_map[MAX_BTN_SIZE] = {USER_BTN_1, USER_BTN_2};



    _attribute_data_retention_    int key_not_released = 0;

    _attribute_data_retention_    static u8 button_press_flag = 0;

    /**
     * @brief     record the result of key detect
     */
    typedef    struct{
        u8     cnt;                //count button num
        u8     btn_press;
        u8     keycode[MAX_BTN_SIZE];            //6 btn
    }vc_data_t;
    _attribute_data_retention_    vc_data_t vc_event;

    /**
     * @brief     record the status of button process
     */
    typedef struct{
        u8  btn_history[4];        //vc history btn save
        u8  btn_filter_last;
        u8    btn_not_release;
        u8     btn_new;                    //new btn  flag
    }btn_status_t;
    _attribute_data_retention_    btn_status_t     btn_status;

    extern void app_button_proc(bool press);

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
        memset(&vc_event,0,sizeof(vc_data_t));            //clear vc_event
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
     * @brief        this function is used to detect if button pressed or released.
     * @param[in]    e - event type when this function is triggered by LinkLayer event
     * @param[in]    p - event callback data pointer for when this function is triggered by LinkLayer event
     * @param[in]    n - event callback data length when this function is triggered by LinkLayer event
     * @return      none
     */
    void proc_button(u8 e, u8 *p, int n)
    {

        int det_key = vc_detect_button(1);

        if (det_key)  //key change: press or release
        {

            u8 key0 = vc_event.keycode[0];
            u8 key1 = vc_event.keycode[1];

            key_not_released = 1;

            if(vc_event.cnt == 2)  //two key press
            {
                printf("%s, %d: key0 %d, key1 %d press\r\n", __FUNCTION__, __LINE__, key0, key1);
            }
            else if(vc_event.cnt == 1) //one key press
            {
                printf("%s, %d: key0 %d, key1 %d press\r\n", __FUNCTION__, __LINE__, key0, key1);
                if(key0 == USER_BTN_1)
                {
                    app_button_proc(true);
                    button_press_flag = 1;
                }
                else if(key0 == USER_BTN_2)
                {
                    #if (UI_ACC_SIMU_ENABLE)
                    extern void app_set_simulate_motion_detected_flag(void);
                    app_set_simulate_motion_detected_flag();
                    #endif
                }
            }
            else{  //release
                printf("%s, %d: key0 %d, key1 %d release\r\n", __FUNCTION__, __LINE__, key0, key1);
                key_not_released = 0;

                if(button_press_flag){
                    button_press_flag = 0;
                    app_button_proc(false);
                }
            }

        }


    }
#endif   //end of UI_BUTTON_ENABLE


#if (UI_KEYBOARD_ENABLE || UI_BUTTON_ENABLE)

_attribute_data_retention_ unsigned char app_button_press_times;
_attribute_data_retention_ unsigned char app_button_release_times;
_attribute_data_retention_ unsigned char app_button_hold_cnts;


#if UI_LED_ENABLE
int app_led_flash(void)
{
    gpio_toggle(GPIO_LED_WHITE);
    return 250 *1000;

}
#endif

void app_button_indicate_stop(void)
{
    #if UI_LED_ENABLE
    app_ui_stop_flash_timer();
    gpio_write(GPIO_LED_WHITE, 0);
    #endif
}

void app_button_indicate_start(void)
{
    app_ui_start_ind_timer();

    #if UI_LED_ENABLE
    app_led_flash();
    app_ui_start_flash_timer();
    #endif
}

void app_button_short_press(void)
{
//    extern void fmna_sn_access_handler(void);
//    app_sched_event_put(NULL,NULL,fmna_sn_access_handler);
    Portble_btn_press();
}


bool app_button_press_state(void)
{
    bool press_state = false;

#if (UI_KEYBOARD_ENABLE)
       unsigned char gpio[8];
        scan_pin_need = kb_key_pressed(gpio);
        if (scan_pin_need) {
            return true;
        }
#elif (UI_BUTTON_ENABLE)

    if(gpio_read(SW1_GPIO) == 0) press_state = true;

#endif

    return press_state;
}

int app_button_long_press(void)
{
    if(!app_button_press_state())
    {
        app_button_press_times = 0;
        app_button_release_times = 0;
        app_button_hold_cnts = 0;
        app_ui_stop_l_btn_timer();
    }
    else
    {
        app_button_hold_cnts++;

        if(app_button_press_times == 1 && app_button_hold_cnts == 6)    //6 * 500ms = 3s
        {
            //power off
            app_ui_stop_l_btn_timer();
            app_button_indicate_start();
            Portble_btn_l_press();

        }
        else if(app_button_press_times >= 2)
        {
            app_ui_stop_l_btn_timer();

        }
        else
           return BTN_PRESS_LONG_TIME_MS;
    }

    tlk_printf("%s, %d: press_times = %d, release_times = %d, hold_cnts = %d\r\n", __FUNCTION__, __LINE__, app_button_press_times, app_button_release_times, app_button_hold_cnts);
    return 0;
}

int app_button_press_proc(void)
{
    if(app_button_press_times == 1)    //single button event
    {
        if(app_button_release_times == 1 && !app_button_press_state())    //short press
        {
            app_button_press_times = 0;
            app_button_release_times = 0;
            app_button_hold_cnts = 0;
            app_button_short_press();
        }
        else if(app_button_release_times == 0 && app_button_press_state())    //enter long press detect flow.
        {
            app_button_hold_cnts = 1;
            app_ui_start_l_btn_timer();
        }
        else
        {
            app_button_press_times = app_button_release_times = app_button_hold_cnts = 0;
        }
    }
    else if(app_button_press_times == 2)    //uncertian state.
    {

    }
    else
    {
        app_button_press_times = 0;
        app_button_release_times = 0;
        app_button_hold_cnts = 0;
    }

    tlk_printf("%s, %d: press_times = %d, release_times = %d, hold_cnts = %d\r\n", __FUNCTION__, __LINE__, app_button_press_times, app_button_release_times, app_button_hold_cnts);
    return 0;
}

void app_button_release_proc(void)
{

    if(app_button_press_times == 2)
    {
        app_ui_stop_btn_timer();
        app_ui_stop_l_btn_timer();
        app_button_press_times = 0;
        app_button_release_times = 0;
        Portble_btn_d_press();
    }
    else if(app_button_press_times == 1)
    {

    }
    else if(app_button_hold_cnts >= 1 )
    {
        app_button_press_times = 0;
        app_button_release_times = 0;
        app_button_hold_cnts = 0;
        app_ui_stop_l_btn_timer();
    }

    tlk_printf("%s, %d: press_times = %d, release_times = %d, hold_cnts = %d\r\n", __FUNCTION__, __LINE__, app_button_press_times, app_button_release_times, app_button_hold_cnts);
}

void app_button_proc(bool press)
{
    if(press)
    {
        #if UI_LED_ENABLE
        gpio_toggle(GPIO_LED_WHITE);
        #endif

        if(app_button_press_times == 0)
        {
            app_ui_start_btn_timer();
        }
        app_button_press_times++;
    }
    else
    {
        #if UI_LED_ENABLE
        gpio_write(GPIO_LED_WHITE, 0);
        #endif

        app_button_release_times++;
        app_button_release_proc();
    }
}


bool app_ui_need_deal(void)
{
    if( key_not_released || scan_pin_need)
        return true;
    return false;
}


void app_ui_Entry_suspend(void)
{
    u32 pin[] = KB_DRIVE_PINS;
    for (unsigned int i=0; i<(sizeof (pin)/sizeof(*pin)); i++){
        gpio_set_irq(GPIO_IRQ1, pin[i], INTR_HIGH_LEVEL);

    }
    extern void proc_keyboardSupend (u8 e, u8 *p, int n);
    blc_ll_registerTelinkControllerEventCallback (BLT_EV_FLAG_GPIO_EARLY_WAKEUP, &proc_keyboardSupend);
    gpio_set_irq_mask(GPIO_IRQ_IRQ1);
    plic_interrupt_enable(IRQ_GPIO_IRQ0);
}

#endif


