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

#include "app_config.h"
#include "app.h"
#include "app_codec.h"


#if (INTER_TEST_MODE == TEST_USB_PPM_ASRC)


extern u8* dmic_in_buffer_start_addr;
extern void usb_handle_irq(void);

volatile u8 micDataRdyFlg = 0;

void usb_audio_init(){
#if (MIC_CHANNEL_COUNT == 2)
    reg_usb_ep6_buf_addr = 0x60;
#elif (MIC_CHANNEL_COUNT == 1)
    reg_usb_ep6_buf_addr = 0x40;
#endif
    reg_usb_ep7_buf_addr = 0x20;
    reg_usb_ep8_buf_addr = 0x00;
    reg_usb_ep_max_size = 64;//(192 >> 3);
    usbhw_data_ep_ack(USB_EDP_SPEAKER);//buffer len 16byte
    usb_set_pin_en();
    usb_init();
    core_interrupt_enable();
    plic_interrupt_enable(IRQ_USB_ENDPOINT);        // enable usb endpoint interrupt
    plic_set_priority(IRQ_USB_ENDPOINT, 2);     //
    reg_usb_ep_irq_mask = 0;  //default value is 0xff, so clear first
    usbhw_set_eps_irq_mask(FLD_USB_EDP6_IRQ);
    usbhw_set_eps_irq_mask(FLD_USB_EDP7_IRQ);
    usbhw_set_irq_mask(USB_IRQ_RESET_MASK|USB_IRQ_SUSPEND_MASK);
}
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
        blc_debug_enableStackLog(STK_LOG_NONE);
    #endif

    blc_readFlashSize_autoConfigCustomFlashSector();

    /* attention that this function must be called after "blc readFlashSize_autoConfigCustomFlashSector" !!!*/
    blc_app_loadCustomizedParameters_normal();
//////////////////////////// basic hardware Initialization  End /////////////////////////////////



    usb_audio_init();

    app_codec_init();

    tlkapi_send_string_data(APP_LOG_EN, "[APP] kmlea dongle init", NULL, 0);
}

/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void)
{

}



/**
 * @brief     BLE main idle loop
 * @param[in]  none.
 * @return     none.
 */
void app_dmicInBuff_2_usbInBuff(void){

    static u16 buffer_mic_rptr = 0;
    u16 mic_wptr = audio_get_rx_dma_wptr(DMA2) - (u32)dmic_in_buffer_start_addr; //app_dmic_in_pingpong

    u16 mic_size = (mic_wptr >= buffer_mic_rptr) ? ((mic_wptr) - buffer_mic_rptr) : 0xffff;

    if(mic_size > DMIC_IN_PINGPONG_SIZE){

        //u8 status = tlk_buffer_write( (s8*)(app_dmic_in_pingpong+buffer_mic_rptr), DMIC_IN_PINGPONG_SIZE, MIC_BUFF_IDX);

        buffer_mic_rptr = buffer_mic_rptr ? 0 : (DMIC_IN_PINGPONG_SIZE);//either 0 or the half of the buffer.

        if(buffer_mic_rptr && iso_in_irq_stick){
            APP_DBG_CHN_6_TOGGLE;
            micDataRdyFlg = 0x01;

        }else{
            APP_DBG_CHN_9_TOGGLE;
            micDataRdyFlg = 0x00;
        }
    }

    if(clock_time_exceed(iso_in_irq_stick, 100*1000)){
        iso_in_irq_stick = 0;
        iso_in_permiteFlag = 0;
    }
}


u32 ledToggleTick = 0;
/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void main_loop (void)
{
    APP_DBG_CHN_0_HIGH;

    #if 1
        if(clock_time_exceed(ledToggleTick, 1000 * 1000))
        {  //led toggle interval: 1000mS
            static u32 loop_cnt = 0;
            ledToggleTick = clock_time();
            loop_cnt ++;
            gpio_toggle(GPIO_LED_RED);
            tlkapi_send_string_data(APP_LOG_EN, "[APP] mainloop", &loop_cnt, 4);
        }
    #endif

    ////////////////////////////////////// BLE entry /////////////////////////////////

    //app_audio_handler();

    usb_handle_irq();

    app_dmicInBuff_2_usbInBuff();
    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif

    ////////////////////////////////////// UI entry /////////////////////////////////
    #if (UI_KEYBOARD_ENABLE)
        proc_keyboard (0, 0, 0);
    #endif


    APP_DBG_CHN_0_LOW;
}




#endif //end of (PRODUCT_CIS_SOURCE_SELECT == ...)
