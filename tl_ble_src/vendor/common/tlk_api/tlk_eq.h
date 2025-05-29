/********************************************************************************************************
 * @file    tlk_eq.h
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2023
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
#ifndef VENDOR_COMMON_TLK_EQ_H_
#define VENDOR_COMMON_TLK_EQ_H_
#include <nds_filtering_math.h>

#include "../../drivers.h"
#define TLK_KEY_EQ_EN 1

#ifndef my_eq_ram_code
    #define my_eq_ram_code _attribute_ram_code_
#endif

#define EQ_DEBUG 0

#if EQ_DEBUG
    #define eqprintf printf
#else
    #define eqprintf(...)
#endif

typedef enum _e_eq_sample_rate
{
    EQ_SAMPLE_RATE_48K   = 0,
    EQ_SAMPLE_RATE_44P1K = 1,
    EQ_SAMPLE_RATE_16K   = 2,
    EQ_SAMPLE_RATE_32K   = 3,

} e_eq_sample_rate_t;

typedef enum _e_eq_channel
{
    EQ_CHANNEL_LEFT   = 0,
    EQ_CHANNEL_RIGHT  = 1,
    EQ_CHANNEL_STEREO = 2,
} e_eq_channel_t;

typedef enum _e_eq_type
{
    EQ_TYPE_MUSIC     = 0,
    EQ_TYPE_VOICE_MIC = 1,
    EQ_TYPE_VOICE_SPK = 2,

} e_eq_type_t;

typedef struct _eq_para
{
    uint8_t eq_nstage;      /// EQ number of stage.
    uint8_t eq_sample_rate; /// 0:48K ;1:44.1K ;2: 16K
    uint8_t eq_channel;     /// 0:left   1:right
    uint8_t eq_type;        /// 0:EQ_TYPE_MUSIC  1:EQ_TYPE_VOICE_MIC  2:EQ_TYPE_VOICE_SPK
} __attribute__((__packed__)) eq_para_t;

#define EQ_FLASH_BANK_SIZE      0xd0
#define EQ_FLASH_BANK_NUM_MAX   0x07

#define NSTAGE_EQ_VOICE_MIC_MAX 4
#define NSTAGE_EQ_VOICE_SPK_MAX 4
#define NSTAGE_EQ_MISIC_SPK_MAX 9

#define NSTAGE_EQ_VOICE_MIC     4
#define NSTAGE_EQ_VOICE_SPK     4

#if SOUNDBAR_EQ_EN
    #define NSTAGE_EQ_MISIC_SPK 5
#else
    #define NSTAGE_EQ_MISIC_SPK 9
#endif

#if (NSTAGE_EQ_VOICE_MIC_MAX < NSTAGE_EQ_VOICE_MIC) || (NSTAGE_EQ_VOICE_SPK_MAX < NSTAGE_EQ_VOICE_SPK) || (NSTAGE_EQ_MISIC_SPK_MAX < NSTAGE_EQ_MISIC_SPK)
    #error Wrong NSTAGE_EQ MAX
#endif

extern eq_para_t        g_eq_para;
extern nds_bq_df1_f32_t instance_eq_music_right;
extern nds_bq_df1_f32_t instance_eq_music_left;
extern nds_bq_df1_f32_t instance_eq_voice_mic_right;
extern nds_bq_df1_f32_t instance_eq_voice_mic_left;
extern nds_bq_df1_f32_t instance_eq_voice_speaker_right;
extern nds_bq_df1_f32_t instance_eq_voice_speaker_left;

extern float32_t state_eq_voice_mic_left[4 * NSTAGE_EQ_VOICE_MIC_MAX];
extern float32_t state_eq_voice_mic_right[4 * NSTAGE_EQ_VOICE_MIC_MAX];
extern float32_t state_eq_voice_speaker_left[4 * NSTAGE_EQ_VOICE_SPK_MAX];
extern float32_t state_eq_voice_speaker_right[4 * NSTAGE_EQ_VOICE_SPK_MAX];

extern float32_t state_eq_music_left[4 * NSTAGE_EQ_MISIC_SPK_MAX];
extern float32_t state_eq_music_right[4 * NSTAGE_EQ_MISIC_SPK_MAX];

void eq_state_reset_all(void);

void              eq_inf_load(uint32_t adr);
int               eq_coef_load_ota(eq_para_t eq_tp, u8 *p);
int               eq_func_enable(eq_para_t eq_tp);
nds_bq_df1_f32_t *eq_instance_get(eq_para_t para);
int               eq_instance_load_flash(eq_para_t para);

void eq_proc(eq_para_t para, signed short *ps, signed short *pd, unsigned int nsample);

int my_debug_eq(unsigned char *p, int len);

#endif /* VENDOR_COMMON_TLK_EQ_H_ */
