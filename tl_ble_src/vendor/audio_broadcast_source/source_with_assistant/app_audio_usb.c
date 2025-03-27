/********************************************************************************************************
 * @file    app_audio_usb.c
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
#include "../source_config.h"

#if (SOURCE_VERSION == SOURCE_WITH_ASSISTANT)

#include "app_config.h"
#include "app_audio.h"
#include "application/usbstd/usb.h"
#include "stack/ble/ble.h"

#if APP_AUDIO_INPUT_MODE == APP_AUDIO_INPUT_USB_MIC

#define USB_RX_BUFF_SIZE                1024
unsigned int usbRxDataBuffer[USB_RX_BUFF_SIZE];     //supported stereo 48kHz 16bit speaker input
int usbRxWptr = 0;
int usbRxRptr = 0;

static u32 usbIsoOutTimer = 0;

/**
 * @brief       usb audio irq handler function.
 * @param[in]   none
 * @return      none
 */
void  usb_endpoint_irq_handler (void);

/**
 * @brief       audio initial usb audio speak role.
 * @param[in]   none
 * @return      none
 */
void app_audio_initUsbMic(void)
{
    blc_audio_usb_init();
    extern void blc_ll_register_user_irq_handler_cb(user_irq_handler_cb_t cb);
    blc_ll_register_user_irq_handler_cb(usb_endpoint_irq_handler);
}

/**
 * @brief       usb audio irq handler function.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void  usb_endpoint_irq_handler (void)
{
    /////////////////////////////////////
                // ISO OUT
    /////////////////////////////////////
    if (usbhw_get_eps_irq()&FLD_USB_EDP6_IRQ)
    {
        usbIsoOutTimer = clock_time();
#if UI_LED_ENABLE
        gpio_write(GPIO_LED_RED, 1);
#endif
        usbhw_clr_eps_irq(FLD_USB_EDP6_IRQ);
        ///////////// output to audio fifo out ////////////////
        unsigned char len = reg_usb_ep6_ptr;
        usbhw_reset_ep_ptr(USB_EDP_SPEAKER);
        for (unsigned int i=0; i<len; i+=4)
        {
            u32 d = reg_usb_ep6_dat;
            d |= reg_usb_ep6_dat << 8;
            d |= reg_usb_ep6_dat << 16;
            d |= reg_usb_ep6_dat << 24;
            usbRxDataBuffer[usbRxWptr] = d;
            usbRxWptr = (usbRxWptr+1)&(USB_RX_BUFF_SIZE - 1);
        }
        usbhw_data_ep_ack(USB_EDP_SPEAKER);
    }
}
PLIC_ISR_REGISTER(usb_endpoint_irq_handler, IRQ_USB_ENDPOINT)
/**
 * @brief       usb audio clean rx Buffer.
 * @param[in]   none
 * @return      none
 */
void usb_audio_cleanUsbRxBuffer(void)
{
    usbRxRptr = usbRxWptr;
}

/**
 * @brief       usb audio get pcm data.
 * @param[in]   none
 * @return      none
 */
void app_audio_getUsbMicData(u16* pcm)
{
    if(((usbRxWptr-usbRxRptr) & (USB_RX_BUFF_SIZE - 1)) < codecFrameDataLen)
    {
        memset(pcm, 0, 4*codecFrameDataLen);
    }
    else
    {
        int len = (USB_RX_BUFF_SIZE-usbRxRptr)<<2;
        len = min(4*codecFrameDataLen, len);
        memcpy(pcm, usbRxDataBuffer+usbRxRptr, len);
        if(len != 4*codecFrameDataLen)
        {
            memcpy(((void*)pcm)+len, usbRxDataBuffer, 4*codecFrameDataLen-len);
        }
        usbRxRptr += codecFrameDataLen;
        usbRxRptr = usbRxRptr&(USB_RX_BUFF_SIZE - 1);
        blc_usb_adjust_volume(pcm, 2*codecFrameDataLen);
    }
}

/**
 * @brief       usb audio handler.
 * @param[in]   none
 * @return      none
 */
void app_audio_usbMicHandler(void)
{
    usb_handle_irq();
    app_cdc_loop();

    if(usbIsoOutTimer && clock_time_exceed(usbIsoOutTimer, 3*1000))
    {
        usbIsoOutTimer = 0;
        memset(usbRxDataBuffer, 0, sizeof(usbRxDataBuffer));
        usbRxWptr = 0;
        usbRxRptr = 0;
#if UI_LED_ENABLE
        gpio_write(GPIO_LED_RED, 0);
#endif
    }
}

#endif

#endif      //SOURCE_VERSION == SOURCE_WITH_ASSISTANT
