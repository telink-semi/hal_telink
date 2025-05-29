/********************************************************************************************************
 * @file    tlk_tone.h
 *
 * @brief   This is the header file for b92_btll_cc_general_sdk
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
#ifndef __TONE_H__
#define __TONE_H__

#include "tl_common.h"

#ifndef TLK_TONE_MONO_MODE
    #define TLK_TONE_MONO_MODE 0
#endif

enum tlk_tone_type
{
    TLK_TONE_TYPE_ADPCM = 1,
    TLK_TONE_TYPE_SBC   = 5,
    TLK_TONE_TYPE_PCM8  = 6,
};

/****** The default tone format is 16000 sampling rate and 1 channel ******/

#define TLK_TONE_DEBUG_ENABLE              (0)

#define TLK_TONE_DEFAULT_ADDR              (0x2017F000)
#define TLK_TONE_DEFAULT_VOLUME            (512)
#define TLK_TONE_DEFAULT_TYPE              (TLK_TONE_TYPE_PCM8)
#define TLK_TONE_DEFAULT_ADPCM_SAMPLE_RATE (16000)

#define TLK_TONE_SBC_ENABLE                (0)

#if TLK_TONE_MONO_MODE
typedef unsigned short tcodec_type;
#else
typedef unsigned int tcodec_type;
#endif

#define SBC_AUDIO_BUFF_SIZE_MAX 1024

typedef struct
{
    uint32_t addr;
    uint32_t len;
    uint32_t offset;
    uint32_t last_tone_tick;
    int16_t  volume;
    uint8_t *buff;
    uint8_t  busy;
    uint8_t  type;
    uint8_t  ready;

    uint8_t hold;
    uint8_t cur_id;
    uint8_t rsvd[3];

    uint32_t adpcm_sample_rate;
    uint32_t sbc_sample_rate;
    uint8_t  sbc_audio_buff[SBC_AUDIO_BUFF_SIZE_MAX];
} tlk_tone_cfg_t;

typedef enum
{
    TLK_TONE_PAIRING = 0,
    TLK_TONE_CONNECTED,
    TLK_TONE_CONNECTED_1,
    TLK_TONE_LOW_POWER,
    TLK_TONE_RING,
    TLK_TONE_POWER_OFF,

    TLK_TONE_MODE_MAX,
} tlk_tone_type_e;

extern tlk_tone_cfg_t g_tone_cfg;

/**
 * @brief  set tone address
 *
 * @param[in]  addr: tone address
 *
 * @returns none
 */
static inline void tlk_tone_set_address(uint32_t addr)
{
    g_tone_cfg.addr = addr;
}

/**
 * @brief  whether tone is playing
 *
 * @param[in]  none
 *
 * @returns g_tone_cfg.busy
 */
static inline uint8_t tlk_tone_is_playing(void)
{
    return g_tone_cfg.busy;
}

/**
 * @brief  whether tone is ready
 *
 * @param[in]  none
 *
 * @returns g_tone_cfg.ready
 */
static inline uint8_t tlk_tone_data_ready(void)
{
    return g_tone_cfg.ready;
}

/**
 * @brief  tone play finished
 *
 * @param[in]  none
 *
 * @returns none
 */
static inline void tlk_tone_data_played(void)
{
    g_tone_cfg.ready = 0;
}

/**
 * @brief  set tone volume
 *
 * @param[in]  v: tone volume
 *
 * @returns tone volume
 */
static inline uint8_t tlk_tone_set_volume(int16_t v)
{
    return g_tone_cfg.volume = v;
}

/**
 * @brief  get tone data according to id
 *
 * @param[in]  id: tone id
 *
 * @returns none
 */
void tlk_tone_play(tlk_tone_type_e type);

/**
 * @brief  get tone sample
 *
 * @param[in]  p_out: address to restore tone data
 * @param[in]  number_of_byte: data length in byte
 * @param[in]  sample_rate: Conversion sampling rate
 *
 * @returns tone data length
 */
uint16_t tlk_tone_get_sample(int16_t *p_out, uint16_t number_of_byte, uint16_t sample_rate);

#endif
