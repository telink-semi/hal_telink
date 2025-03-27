/********************************************************************************************************
 * @file    tlka_ppm_asrc_api.h
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  Driver Group
 * @date    2020
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef TLKA_PPM_ASRC_API_H
#define TLKA_PPM_ASRC_API_H

#define PPM_ASRC_VERSION_INT(major, minor, micro) (((major) << 16) | ((minor) << 8) | (micro))
#define PPM_ASRC_VERSION PPM_ASRC_VERSION_INT(0, 3, 0)

#define PPM_OVERSAMPLE (8)
#define PPM_BASE_FILTER_LEN (20)

#define S24_MAX ((1 << 23) - 1)
#define S24_MIN (-(1 << 23))

#define USB_OUT_SAMPLES (240)
#define USB_SPEAKER_CHANNEL (2)

#define APP_ALIGN4_NUM(x) (((x) + 3) / 4 * 4)
#define APP_STRUCT_BUF_LEN APP_ALIGN4_NUM(4 * 14 + 4 * (4 * (8 * 28 + 8)) + 4 * (4 * (8 * 16 + 8)) + 4 * 2 + 256)
#define APP_PPM_DATA_BUF_LEN(max_len) APP_ALIGN4_NUM((sizeof(float) * (28 + max_len + 16)))
#define APP_ASRC_BUFF_LEN_MIN(max_len, chn) (APP_STRUCT_BUF_LEN + (chn)*APP_PPM_DATA_BUF_LEN((max_len)))

#define _attribute_aligned_(s) __attribute__((aligned(s)))
#define _attribute_retention_code_ __attribute__((section(".retention_code"))) __attribute__((noinline))

typedef unsigned char u8;

#if __riscv
#define REARRANG_SINC_TABLE
#endif

/* channel type */
typedef enum
{
    TLKA_PPM_ASRC_SINGLE = 0,
    TLKA_PPM_ASRC_STEREO

} TLKA_PPM_ASRC_CHANNEL;

typedef struct TLKA_PPM_ASRC_16_bit_PARAM
{
    unsigned int in_rate;
    unsigned int out_rate;
    unsigned int num_rate;
    unsigned int den_rate;
    int initialised;

    int int_advance;
    int frac_advance;
    float cutoff;

    /* These are per-channel */
    int last_sample;
    unsigned int samp_frac_num;

    int last_sample_o;
    unsigned int samp_frac_num_o;
    TLKA_PPM_ASRC_CHANNEL channel_mode;

    int switch_flag;
    int ppm_data_len_max;
#ifdef REARRANG_SINC_TABLE
    short sinc_table[4 * (8 * 28 + 8)];
    short sinc_table2[4 * (PPM_OVERSAMPLE * 16 + 8)];
#else
    short sinc_table[PPM_OVERSAMPLE * PPM_BASE_FILTER_LEN + 8];
#endif
} tlka_ppm_asrc_16_bit_param;

typedef struct TLKA_PPM_ASRC_24_BIT_PARAM
{
    unsigned int in_rate;
    unsigned int out_rate;
    unsigned int num_rate;
    unsigned int den_rate;
    int initialised;

    int int_advance;
    int frac_advance;
    float cutoff;

    /* These are per-channel */
    int last_sample;
    unsigned int samp_frac_num;

    int last_sample_o;
    unsigned int samp_frac_num_o;

    int switch_flag;
    TLKA_PPM_ASRC_CHANNEL channel_mode;

    int ppm_data_len_max;

#ifdef REARRANG_SINC_TABLE
    // short sinc_table[4*(PPM_OVERSAMPLE* PPM_BASE_FILTER_LEN+8)];
    float sinc_table[4 * (8 * 28 + 8)];
    float sinc_table2[4 * (PPM_OVERSAMPLE * 16 + 8)];
#else
    float sinc_table[PPM_OVERSAMPLE * PPM_BASE_FILTER_LEN + 8];
#endif
    float *ppm_data_buf[2]; //[28 + PPM_DATA_LEN_MAX] = { 0 };
} tlka_ppm_asrc_24_bit_param;

int tlka_ppm_asrc_get_version(void);
int tlka_ppm_asrc_16_bit_get_size(void);
int tlka_ppm_asrc_24_bit_get_size(void);

/*---------------------------------------------------------------*
 * channel : defined the input channel type                      *
 *---------------------------------------------------------------*/
void tlka_ppm_asrc_16_bit_init(void *st, TLKA_PPM_ASRC_CHANNEL channel, int ppm);
void tlka_ppm_asrc_24_bit_init(void *st, TLKA_PPM_ASRC_CHANNEL channel, unsigned int buffer_len, int ppm_data_len_max);

/*---------------------------------------------------------------*
 * name : tlka_ppm_asrc_16_bit_frame_process                     *
 * ps : 16 bit input                                             *
 * n : input frame size                                          *
 * pd : 16 bit output                                            *
 *---------------------------------------------------------------*/
int tlka_ppm_asrc_16_bit_frame_process(void *st, short *ps, int n, short *pd);

/*---------------------------------------------------------------*
 * name : tlka_ppm_asrc_24_bit_frame_process                     *
 * ps : 24 bit input                                             *
 * n : input frame size                                          *
 * pd : 24 bit output                                            *
 *---------------------------------------------------------------*/
int tlka_ppm_asrc_24_bit_frame_process(void *st, int *ps, int n, int *pd);

#endif
