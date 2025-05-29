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
#include "app_config.h"
#include "app.h"
#include "application/usbstd/usb.h"
#include "stack/ble/ble.h"
#include "app_audio.h"

#if (INTER_TEST_MODE == TEST_BIS_AUDIO_SENDER)


    #if (APP_AUDIO_INPUT_TYPE == APP_AUDIO_INPUT_USB_MIC)

unsigned int audio_buffer_write_ptr = 0;

extern void blc_ll_register_user_irq_handler_cb(user_irq_handler_cb_t cb);
void        usb_endpoint_irq_handler(void);
        #if (MCU_CORE_TYPE == MCU_CORE_B92)
extern audio_codec_stream0_input_t audio_codec_input;
extern audio_codec_output_t        audio_codec_output;
        #endif
void app_audio_usb_speak_init(void)
{
    blc_audio_usb_init(BLC_CODEC_SUBDEV_SPK);
        #if (MCU_CORE_TYPE == MCU_CORE_B91)
    blc_audio_setBuffer((u8 *)gAppAudioBuffer, sizeof(gAppAudioBuffer));
    audio_init(BUF_TO_LINE_OUT, AUDIO_48K, STEREO_BIT_16);
    audio_tx_dma_chain_init(DMA3, (unsigned short *)&gAppAudioBuffer, sizeof(gAppAudioBuffer));
        #elif (MCU_CORE_TYPE == MCU_CORE_B92)
    blc_audio_setBuffer((u8 *)gAppAudioBuffer, sizeof(gAppAudioBuffer));
    ble_codec_setSpkBuffer((u8 *)gAppAudioBuffer, sizeof(gAppAudioBuffer));
    ble_audio_codec_init(BLC_CODEC_SUBDEV_SPK, &audio_codec_input, &audio_codec_output);
    ble_codec_spkOpen();
        #endif
    blc_ll_register_user_irq_handler_cb(usb_endpoint_irq_handler);
}

/**
 * @brief     This function servers to set USB Input.
 * @param[in] none
 * @return    none.
 */
_attribute_ram_code_ void audio_rx_data_from_usb()
{
        #if (UI_LED_ENABLE)
    gpio_write(GPIO_LED_BLUE, 1);
        #endif
    u8            usbData[256];
    unsigned char len = reg_usb_ep6_ptr;
    audio_buffer_write_ptr++;
    usbhw_reset_ep_ptr(USB_EDP_SPEAKER);
    for (unsigned int i = 0; i < len; i++) {
        usbData[i] = reg_usb_ep6_dat;
    }
    blc_audio_writeBuffer(usbData, len);
    usbhw_data_ep_ack(USB_EDP_SPEAKER);
}

_attribute_ram_code_ void usb_endpoint_irq_handler(void)
{
    /////////////////////////////////////
    // ISO OUT
    /////////////////////////////////////
    if (usbhw_get_eps_irq() & FLD_USB_EDP6_IRQ) {
        usbhw_clr_eps_irq(FLD_USB_EDP6_IRQ);
        ///////////// output to audio fifo out ////////////////
        audio_rx_data_from_usb();
    }
}
PLIC_ISR_REGISTER(usb_endpoint_irq_handler, IRQ_USB_ENDPOINT)

static void app_audio_usbSpeakerLed(void)
{
        #if (UI_LED_ENABLE)
    static u32 usbSpeakerTimer = 0;
    static u32 temp            = 0;
    if (clock_time_exceed(usbSpeakerTimer, 6000)) {
        if (temp == audio_buffer_write_ptr) {
            audio_buffer_write_ptr = 0;
            memset(gAppAudioBuffer, 0, sizeof(gAppAudioBuffer));
            gpio_write(GPIO_LED_BLUE, 0);
        }
        temp            = audio_buffer_write_ptr;
        usbSpeakerTimer = clock_time();
    }
        #endif
}

static void app_audio_usbSpeakerSendBis(void)
{
    u16 frameDataLen = 480;
    u16 lenPcm       = blc_audio_getDataLen();

    if (lenPcm < 4 * frameDataLen) //STEREO_BIT_16 control
    {
        return;
    }

    u16 pcmData[APP_AUDIO_FRAME_BYTES];

    blc_audio_readBuffer((u8 *)pcmData, 4 * frameDataLen);

    if (!app_audio_check_send_flag()) {
        return;
    }

    u16 audioBuff[APP_AUDIO_FRAME_SAMPLE];
    u8  coded_raw[155]; //Max LC3 encode data
    for (int i = 0; i < frameDataLen; i++) {
        audioBuff[i] = pcmData[2 * i];
    }
    lc3enc_encode_pkt(0, (u8 *)audioBuff, coded_raw);
    blc_iso_sendData(app_bisBcstHandle[0], coded_raw, gbisOctetPer);
        #if SUBGROUP_NUM_BIS > 1
    for (int i = 0; i < frameDataLen; i++) {
        audioBuff[i] = pcmData[2 * i + 1];
    }
    lc3enc_encode_pkt(1, (u8 *)audioBuff, coded_raw);
    blc_iso_sendData(app_bisBcstHandle[1], coded_raw, gbisOctetPer);
        #endif
}

void app_audio_usb_speaker_handle(void)
{
    usb_handle_irq();
    app_audio_usbSpeakerLed();
    app_audio_usbSpeakerSendBis();
}


    #endif


#endif /* INTER_TEST_MODE */
