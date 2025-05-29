/********************************************************************************************************
 * @file    tlk_tone.c
 *
 * @brief   This is the source file for b92_btll_cc_general_sdk
 *
 * @author  BT Audio Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd.
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
 *          file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "tlk_tone.h"
#include "algorithm/audio_alg/adpcm/tlk_adpcm.h"
#include "algorithm/audio_alg/sbc/tlk_sbc_interface_api.h"
#include "ext_driver/ext_audio.h"

tlk_tone_cfg_t g_tone_cfg = {
    .addr              = TLK_TONE_DEFAULT_ADDR,
    .volume            = TLK_TONE_DEFAULT_VOLUME,
    .type              = TLK_TONE_DEFAULT_TYPE,
    .busy              = 0,
    .ready             = 0,
    .buff              = NULL,
    .adpcm_sample_rate = TLK_TONE_DEFAULT_ADPCM_SAMPLE_RATE,
#if (TLK_TONE_SBC_ENABLE)
    .sbc_sample_rate = 16000,
#endif
};

/**
 * @brief  get tone data according to id
 *
 * @param[in]  id: tone id
 *
 * @returns none
 */
void tlk_tone_play(tlk_tone_type_e type)
{
    uint8_t *p     = NULL;
    uint8_t *p_des = NULL;
    uint16_t len;

    tlkapi_printf(TLK_TONE_DEBUG_ENABLE, "tlk_tone_play enter\n");

    if (g_tone_cfg.busy) {
        g_tone_cfg.hold = type | 0x80;
        return;
    }

    g_tone_cfg.hold   = 0;
    g_tone_cfg.cur_id = type;

    p = (uint8_t *)g_tone_cfg.addr + type * 12;

    len = p[8] + (p[9] << 8) + (p[10] << 16);
    my_dump_str_data(TLK_TONE_DEBUG_ENABLE, "tlk_tone_play get_sample1,", p, 12);
    if (p[7] == 0xff || p[11] == 0xff || p[1] > 0x1F) {
        tlkapi_printf(TLK_TONE_DEBUG_ENABLE, "tlk_tone_play fail!\n");
        g_tone_cfg.busy = 0;
        adpcm_init(0, 0, 0, 0);
    } else {
        tlkapi_printf(TLK_TONE_DEBUG_ENABLE, "tlk_tone_play start\n");

        p_des = (uint8_t *)g_tone_cfg.addr + (p[4] + (p[5] << 8) + (p[6] << 16) + (p[7] << 24));

        g_tone_cfg.type = p[1];
        tlkapi_printf(TLK_TONE_DEBUG_ENABLE, "tlk_tone_play type: %d\n", g_tone_cfg.type);

        if (g_tone_cfg.type == TLK_TONE_TYPE_SBC) {
#if (TLK_TONE_SBC_ENABLE)
            g_msbc_dec_buf_ptr = g_tone_cfg.sbc_audio_buff;
            int size           = tlka_sbc_dec_get_size();
            if (size > SBC_AUDIO_BUFF_SIZE_MAX) {
                tlkapi_printf(TLK_TONE_DEBUG_ENABLE, "dec buffer %d\n", size);
                return;
            }
            tlka_msbc_dec_init((sbc_dec_para_t *)g_msbc_dec_buf_ptr);
#endif
        } else {
            adpcm_init(p_des, len, 0, 0);
        }

        g_tone_cfg.len            = len;
        g_tone_cfg.offset         = 0;
        g_tone_cfg.buff           = p_des;
        g_tone_cfg.ready          = 0;
        g_tone_cfg.busy           = 1;
        g_tone_cfg.last_tone_tick = clock_time() + (200 * sys_clk.pclk);
        ble_audio_timer_set_capture(TIMER0, 0, 200 * sys_clk.pclk);
    }
}

/**
 * @brief  get tone sample
 *
 * @param[in]  p_out: address to restore tone data
 * @param[in]  number_of_byte: data length in byte
 * @param[in]  sample_rate: Conversion sampling rate
 *
 * @returns tone data length
 */
uint16_t tlk_tone_get_sample(int16_t *p_out, uint16_t number_of_byte, uint16_t sample_rate)
{
    uint16_t num   = 0;
    int16_t *p_des = p_out;

    if (g_tone_cfg.type != TLK_TONE_TYPE_SBC) {
        num = adpcm_get_sample(p_des, number_of_byte / sizeof(tcodec_type), sample_rate);
        my_dump_str_data(TLK_TONE_DEBUG_ENABLE, "tlk_tone_play get_sample2,", p_des, num);
        s16 *p = p_des;
        for (int i = 0; i < num; i++) {
            *p = (int)(*p) * g_tone_cfg.volume / 1024;
            p++;
#if !TLK_TONE_MONO_MODE
            *p = (int)(*p) * g_tone_cfg.volume / 1024;
            p++;
#endif
        }
        if (num != number_of_byte / sizeof(tcodec_type)) {
            g_tone_cfg.busy = 0;
            tlkapi_printf(TLK_TONE_DEBUG_ENABLE, "tlk_tone_play stop!\n");
            for (unsigned int i = 0; i < number_of_byte / sizeof(tcodec_type); i++) {
                p_des[i] = 0;
            }
            if (g_tone_cfg.hold) {
                tlk_tone_play(g_tone_cfg.hold & 0x7f);
            }
            return 0;
        }

        g_tone_cfg.ready = 1;

        return number_of_byte;
    }

#if (TLK_TONE_SBC_ENABLE)
    num                       = 0;
    uint8_t sbc_buff[64]      = {0};
    uint8_t sbc_dec_buff[240] = {0};
    while (num < number_of_byte) {
        if (g_tone_cfg.offset >= g_tone_cfg.len) {
            g_tone_cfg.busy = 0;
            tlkapi_printf(TLK_TONE_DEBUG_ENABLE, "tlk_tone_play stop!\n");
            for (int i = 0; i < number_of_byte / sizeof(tcodec_type); i++) {
                p_des[i] = 0;
            }
            if (g_tone_cfg.hold) {
                tlk_tone_play(g_tone_cfg.hold & 0x7f);
            }

            return 0;
        }

        flash_read_page(g_tone_cfg.buff, 60, sbc_buff + 4);
        tlkalg_msbc_dec(sbc_buff, 60, (uint8_t *)sbc_dec_buff);

        for (int16_t i = 0; i < 120; i++) {
            *p_des = sbc_dec_buff[2 * i] | sbc_dec_buff[2 * i + 1] << 8;
            p_des++;
    #if !TLK_TONE_MONO_MODE
            *p_des = sbc_dec_buff[2 * i] | sbc_dec_buff[2 * i + 1] << 8;
            p_des++;
    #endif
        }

        my_dump_str_data(TLK_TONE_DEBUG_ENABLE, "tlk_tone_play get_sample msbc input,", g_tone_cfg.buff, 60);
        my_dump_str_data(TLK_TONE_DEBUG_ENABLE, "tlk_tone_play get_sample msbc output,", sbc_dec_buff, 240);
        my_dump_str_data(TLK_TONE_DEBUG_ENABLE, "tlk_tone_play get_sample msbc output,", p_des, 240);
    #if !TLK_TONE_MONO_MODE
        num += 480;
    #else
        num += 240;
    #endif
        g_tone_cfg.buff += 60;
        g_tone_cfg.offset += 60;
    }

    g_tone_cfg.ready = 1;
#endif
    return num;
}
