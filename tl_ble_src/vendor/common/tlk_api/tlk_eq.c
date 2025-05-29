/********************************************************************************************************
 * @file    tlk_eq.c
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
#include "tl_common.h"
#include "tlk_eq.h"
#include "algorithm/audio_alg/eq/gcc7/tlka_eq_api.h"

#if EQ_AUDIO_EN

    //#define DUMP_APP_MSG                            1
    #define DUMP_ACL_MSG 1

eq_para_t g_eq_para = {0, 0, 0, 0};

float32_t state_eq_voice_mic_left[4 * NSTAGE_EQ_VOICE_MIC_MAX]      = {0.0};
float32_t state_eq_voice_mic_right[4 * NSTAGE_EQ_VOICE_MIC_MAX]     = {0.0};
float32_t state_eq_voice_speaker_left[4 * NSTAGE_EQ_VOICE_SPK_MAX]  = {0.0};
float32_t state_eq_voice_speaker_right[4 * NSTAGE_EQ_VOICE_SPK_MAX] = {0.0};

float32_t state_eq_music_left[4 * NSTAGE_EQ_MISIC_SPK_MAX]  = {0.0};
float32_t state_eq_music_right[4 * NSTAGE_EQ_MISIC_SPK_MAX] = {0.0};

float32_t coeff_eq_voice_mic_left[5 * NSTAGE_EQ_VOICE_MIC_MAX] = {
    1,
    -1.98984742164612,
    0.990231096744537,
    1.98984742164612,
    -0.990231096744537,
    1,
    -1.62248027324677,
    0.803260147571564,
    1.62248027324677,
    -0.803260147571564,
    1,
    -1.0633008480072,
    0.676091015338898,
    1.0633008480072,
    -0.676091015338898,
    1,
    -0.406621515750885,
    0.610435903072357,
    0.406621515750885,
    -0.610435903072357,
};

float32_t coeff_eq_voice_mic_right[5 * NSTAGE_EQ_VOICE_MIC_MAX] = {
    1,
    -1.98984742164612,
    0.990231096744537,
    1.98984742164612,
    -0.990231096744537,
    1,
    -1.62248027324677,
    0.803260147571564,
    1.62248027324677,
    -0.803260147571564,
    1,
    -1.0633008480072,
    0.676091015338898,
    1.0633008480072,
    -0.676091015338898,
    1,
    -0.406621515750885,
    0.610435903072357,
    0.406621515750885,
    -0.610435903072357,
};

float32_t coeff_eq_voice_speaker[5 * NSTAGE_EQ_VOICE_SPK_MAX] = {
    1,
    -1.98984742164612,
    0.990231096744537,
    1.98984742164612,
    -0.990231096744537,
    1,
    -1.91552603244781,
    0.933762669563293,
    1.91552603244781,
    -0.933762669563293,
    1,
    -1.82029581069946,
    0.881250023841858,
    1.82029581069946,
    -0.881250023841858,
    1,
    -1.70690107345581,
    0.832982957363129,
    1.70690107345581,
    -0.832982957363129,
};

    #if 1
// default all 0db Peaking
float32_t coeff_eq_music[5 * NSTAGE_EQ_MISIC_SPK_MAX] = {
    1,
    -1.99669015407562,
    0.996732890605927,
    1.99669015407562,
    -0.996732890605927,
    1,
    -1.90592575073242,
    0.927726268768311,
    1.90592575073242,
    -0.927726268768311,
    1,
    -1.78438568115234,
    0.864678144454956,
    1.78438568115234,
    -0.864678144454956,
    1,
    -1.63700175285339,
    0.808075308799744,
    1.63700175285339,
    -0.808075308799744,
    1,
    -1.46824896335602,
    0.758196771144867,
    1.46824896335602,
    -0.758196771144867,
    1,
    -1.28210616111755,
    0.715173006057739,
    1.28210616111755,
    -0.715173006057739,
    1,
    -1.082066655159,
    0.679035484790802,
    1.082066655159,
    -0.679035484790802,
    1,
    -0.871183037757874,
    0.649755895137787,
    0.871183037757874,
    -0.649755895137787,
    1,
    -0.652129411697388,
    0.627277076244354,
    0.652129411697388,
    -0.627277076244354,
};

    #else
//test all 0db 50hz LowPass
float32_t coeff_eq_music[5 * NSTAGE_EQ_MISIC_SPK_MAX] = {
    5.17661328558461E-6,
    1.03532265711692E-5,
    5.17661328558461E-6,
    1.99669015407562,
    -0.996732890605927,
    1,
    -1.90592575073242,
    0.927726268768311,
    1.90592575073242,
    -0.927726268768311,
    1,
    -1.78438568115234,
    0.864678144454956,
    1.78438568115234,
    -0.864678144454956,
    1,
    -1.63700175285339,
    0.808075308799744,
    1.63700175285339,
    -0.808075308799744,
    1,
    -1.46824896335602,
    0.758196771144867,
    1.46824896335602,
    -0.758196771144867,
    1,
    -1.28210616111755,
    0.715173006057739,
    1.28210616111755,
    -0.715173006057739,
    1,
    -1.082066655159,
    0.679035484790802,
    1.082066655159,
    -0.679035484790802,
    1,
    -0.871183037757874,
    0.649755895137787,
    0.871183037757874,
    -0.649755895137787,
    1,
    -0.652129411697388,
    0.627277076244354,
    0.652129411697388,
    -0.627277076244354,
};
    #endif
// default all 0db Peaking
float32_t coeff_eq_music_44p1k[5 * NSTAGE_EQ_MISIC_SPK_MAX] = {
    1,
    -1.99669015407562,
    0.996732890605927,
    1.99669015407562,
    -0.996732890605927,
    1,
    -1.90592575073242,
    0.927726268768311,
    1.90592575073242,
    -0.927726268768311,
    1,
    -1.78438568115234,
    0.864678144454956,
    1.78438568115234,
    -0.864678144454956,
    1,
    -1.63700175285339,
    0.808075308799744,
    1.63700175285339,
    -0.808075308799744,
    1,
    -1.46824896335602,
    0.758196771144867,
    1.46824896335602,
    -0.758196771144867,
    1,
    -1.28210616111755,
    0.715173006057739,
    1.28210616111755,
    -0.715173006057739,
    1,
    -1.082066655159,
    0.679035484790802,
    1.082066655159,
    -0.679035484790802,
    1,
    -0.871183037757874,
    0.649755895137787,
    0.871183037757874,
    -0.649755895137787,
    1,
    -0.652129411697388,
    0.627277076244354,
    0.652129411697388,
    -0.627277076244354,
};

nds_bq_df1_f32_t instance_eq_music_left  = {NSTAGE_EQ_MISIC_SPK, state_eq_music_left, coeff_eq_music};
nds_bq_df1_f32_t instance_eq_music_right = {NSTAGE_EQ_MISIC_SPK, state_eq_music_right, coeff_eq_music};

nds_bq_df1_f32_t instance_eq_voice_mic_left  = {NSTAGE_EQ_VOICE_MIC, state_eq_voice_mic_left, coeff_eq_voice_mic_left};
nds_bq_df1_f32_t instance_eq_voice_mic_right = {NSTAGE_EQ_VOICE_MIC, state_eq_voice_mic_right, coeff_eq_voice_mic_right};

nds_bq_df1_f32_t instance_eq_voice_speaker_left  = {NSTAGE_EQ_VOICE_SPK, state_eq_voice_speaker_left, coeff_eq_voice_speaker};
nds_bq_df1_f32_t instance_eq_voice_speaker_right = {NSTAGE_EQ_VOICE_SPK, state_eq_voice_speaker_right, coeff_eq_voice_speaker};

void eq_coeff_dump(nds_bq_df1_f32_t *instance_eq)
{
    for (int i = 0; i < 5 * instance_eq->nstage; i++) {
        if ((i % 5) == 0) {
            tlkapi_printf(TLK_KEY_EQ_EN, "\n\r");
        }
        tlkapi_printf(TLK_KEY_EQ_EN, "%f,", instance_eq->coeff[i]);
    }
}

typedef struct eq_para_tdump_
{
    uint8_t eq_para;  /// EQ number of stage.
    char   *stringeq; /// EQ number of stage.
} eq_para_tdump;

const eq_para_tdump dumpeq_sample_rate[] = {
    {0, "48k"  },
    {1, "44.1k"},
    {2, "16k"  },
};
const eq_para_tdump dumpeq_channel[] = {
    {0, "left" },
    {1, "right"},
};
const eq_para_tdump dumpeq_type[] = {
    {0, "music"      },
    {1, "voice mic"  },
    {2, "voice speak"},
};

void eq_para_dump(eq_para_t *eq_para_tt)
{
    tlkapi_printf(TLK_KEY_EQ_EN, "\n\req_type:%s\n\r", dumpeq_type[eq_para_tt->eq_type].stringeq);
    tlkapi_printf(TLK_KEY_EQ_EN, "eq_nstage:%d\n\r", eq_para_tt->eq_nstage);
    tlkapi_printf(TLK_KEY_EQ_EN, "eq_sample_rate:%s\n\r", dumpeq_sample_rate[eq_para_tt->eq_sample_rate].stringeq);
    tlkapi_printf(TLK_KEY_EQ_EN, "eq_channel:%s\n\r", dumpeq_channel[eq_para_tt->eq_channel].stringeq);
}

_attribute_ram_code_ void eq_state_reset_all(void)
{
    tlkapi_printf(TLK_KEY_EQ_EN, "eq_state_reset_all");

    memset(&state_eq_voice_mic_left, 0, sizeof(state_eq_voice_mic_left));
    memset(&state_eq_voice_mic_right, 0, sizeof(state_eq_voice_mic_right));
    memset(&state_eq_voice_speaker_left, 0, sizeof(state_eq_voice_speaker_left));
    memset(&state_eq_voice_speaker_right, 0, sizeof(state_eq_voice_speaker_right));
    memset(&state_eq_music_left, 0, sizeof(state_eq_music_left));
    memset(&state_eq_music_right, 0, sizeof(state_eq_music_right));
}

void eq_inf_load(uint32_t adr)
{
    eq_state_reset_all();
    if (adr) {
        uint32_t flag;
        flash_read_page(adr, 4, (u8 *)&flag);
        if (0xFFFFFFFF == flag || !flag) {
    #if 1
            //          audio_codec_flag_set(CODEC_FLAG_EQ_MUSIC_EN
            //                              |CODEC_FLAG_EQ_VOICE_MIC_EN
            //                              |CODEC_FLAG_EQ_VOICE_SPK_EN, 0);
            tlkapi_printf(TLK_KEY_EQ_EN, "EQ disable");
            return;
    #else
            audio_codec_flag_set(CODEC_FLAG_EQ_MUSIC_EN | CODEC_FLAG_EQ_VOICE_MIC_EN | CODEC_FLAG_EQ_VOICE_SPK_EN, 1);
            tlkapi_printf(TLK_KEY_EQ_EN, "EQ enable", 0, 0);
            return;
    #endif
        }

        int eq_bank = EQ_FLASH_BANK_NUM_MAX;

        while (eq_bank--) {
            flag = *(uint32_t *)(adr);
            if (flag && (flag != 0xFFFFFFFF)) {
                eq_para_t eq_tp;
                memcpy(&eq_tp, (uint32_t *)adr, sizeof(eq_para_t));
                eq_para_dump(&eq_tp);
                if ((EQ_TYPE_MUSIC == eq_tp.eq_type && eq_tp.eq_nstage > NSTAGE_EQ_MISIC_SPK) || (EQ_TYPE_VOICE_MIC == eq_tp.eq_type && eq_tp.eq_nstage > NSTAGE_EQ_VOICE_MIC) || (EQ_TYPE_VOICE_SPK == eq_tp.eq_type && eq_tp.eq_nstage > NSTAGE_EQ_VOICE_SPK) || eq_tp.eq_type > EQ_TYPE_VOICE_SPK || eq_tp.eq_channel > EQ_CHANNEL_RIGHT || eq_tp.eq_sample_rate > EQ_SAMPLE_RATE_16K) {
                    tlkapi_printf(TLK_KEY_EQ_EN, "#eq_inf_load, error head");
                    return;
                }

                tlkapi_printf(TLK_KEY_EQ_EN, "EQ state2");

                nds_bq_df1_f32_t *instance_eq = eq_instance_get(eq_tp);
                if (!(instance_eq)) {
                    tlkapi_printf(TLK_KEY_EQ_EN, "#eq_instance_get, error");
                } else {
                    instance_eq->nstage = eq_tp.eq_nstage;
                    memcpy((uint32_t *)instance_eq->coeff, (uint32_t *)(adr + 4), eq_tp.eq_nstage * 20);
                    eq_coeff_dump(instance_eq);
                    eq_func_enable(eq_tp);
                }
            }
            adr = adr + EQ_FLASH_BANK_SIZE;
        }
    } else {
        tlkapi_printf(TLK_KEY_EQ_EN, "EQ state : EQ addr are empty");
    }
}

int eq_instance_load_flash(eq_para_t para)
{
    uint32_t  eq_adr = 0x20000000 + 0xEC000;
    eq_para_t eq_tp[EQ_FLASH_BANK_NUM_MAX];
    uint32_t  flag;
    uint8_t   i;
    uint8_t   findok = 0xff;
    tlkapi_printf(TLK_KEY_EQ_EN, "eq_instance_load_flash\n\r");
    for (i = 0; i < EQ_FLASH_BANK_NUM_MAX; i++) {
        flag = *(uint32_t *)(eq_adr);
        if (flag && (flag != 0xFFFFFFFF)) {
            memcpy(&(eq_tp[i]), (uint32_t *)eq_adr, sizeof(eq_para_t));
            eq_para_dump(&(eq_tp[i]));
            if ((para.eq_type == eq_tp[i].eq_type) &&
                (para.eq_channel == eq_tp[i].eq_channel) &&
                (para.eq_sample_rate == eq_tp[i].eq_sample_rate) &&
                (para.eq_nstage == eq_tp[i].eq_nstage)) {
                tlkapi_printf(TLK_KEY_EQ_EN, "#find eq_bank %d %d %d %d", eq_tp[i].eq_type, eq_tp[i].eq_channel, eq_tp[i].eq_sample_rate, eq_tp[i].eq_nstage);
                nds_bq_df1_f32_t *instance_eq = eq_instance_get(eq_tp[i]);
                if (!(instance_eq)) {
                    tlkapi_printf(TLK_KEY_EQ_EN, "#eq_bank, error load");
                    return -1;
                }
                eq_state_reset_all();
                instance_eq->nstage = eq_tp[i].eq_nstage;
                memcpy((uint32_t *)instance_eq->coeff, (uint32_t *)(eq_adr + 4), eq_tp[i].eq_nstage * 20);
                findok = i;
                eq_coeff_dump(instance_eq);
                return 0;
            }
        }
        eq_adr += EQ_FLASH_BANK_SIZE;
    }
    if (findok == 0xff) {
        tlkapi_printf(TLK_KEY_EQ_EN, "#not find eq_bank %d %d %d %d", para.eq_type, para.eq_channel, para.eq_sample_rate, para.eq_type);
    }
    return 0;
}

//my_ram_code
nds_bq_df1_f32_t *eq_instance_get(eq_para_t para)
{
    nds_bq_df1_f32_t *eq_tp = NULL;

    switch (para.eq_type) {
    case EQ_TYPE_MUSIC:
        if (EQ_CHANNEL_RIGHT == para.eq_channel) {
            instance_eq_music_right.nstage = para.eq_nstage;
            instance_eq_music_right.state  = state_eq_music_right;
            instance_eq_music_right.coeff  = ((para.eq_sample_rate == EQ_SAMPLE_RATE_44P1K) ? coeff_eq_music_44p1k : coeff_eq_music);
            eq_tp                          = &instance_eq_music_right;
        } else {
            instance_eq_music_left.nstage = para.eq_nstage;
            instance_eq_music_left.state  = state_eq_music_left;
            instance_eq_music_left.coeff  = ((para.eq_sample_rate == EQ_SAMPLE_RATE_44P1K) ? coeff_eq_music_44p1k : coeff_eq_music);
            eq_tp                         = &instance_eq_music_left;
        }
        break;

    case EQ_TYPE_VOICE_MIC:
    #if (PROJ_DUALMODE_HEADSET || PROJ_CC_HEADSET)
        if (EQ_CHANNEL_RIGHT == para.eq_channel) {
            instance_eq_voice_mic_right.nstage = para.eq_nstage;
            instance_eq_voice_mic_right.state  = state_eq_voice_mic_right;
            instance_eq_voice_mic_right.coeff  = coeff_eq_voice_mic_right;
            eq_tp                              = &instance_eq_voice_mic_right;
        } else
    #endif
        {
            instance_eq_voice_mic_left.nstage = para.eq_nstage;
            instance_eq_voice_mic_left.state  = state_eq_voice_mic_left;
            instance_eq_voice_mic_left.coeff  = coeff_eq_voice_mic_left;
            eq_tp                             = &instance_eq_voice_mic_left;
        }
        break;

    case EQ_TYPE_VOICE_SPK:
        if (EQ_CHANNEL_RIGHT == para.eq_channel) {
            instance_eq_voice_speaker_right.nstage = para.eq_nstage;
            instance_eq_voice_speaker_right.state  = state_eq_voice_speaker_right;
            instance_eq_voice_speaker_right.coeff  = coeff_eq_voice_speaker;
            eq_tp                                  = &instance_eq_voice_speaker_right;
        } else {
            instance_eq_voice_speaker_left.nstage = para.eq_nstage;
            instance_eq_voice_speaker_left.state  = state_eq_voice_speaker_left;
            instance_eq_voice_speaker_left.coeff  = coeff_eq_voice_speaker;
            eq_tp                                 = &instance_eq_voice_speaker_left;
        }
        break;
    default:
        eq_tp = NULL;
        break;
    }

    return eq_tp;
}

int eq_func_enable(eq_para_t eq_tp)
{
    nds_bq_df1_f32_t *instance_eq = eq_instance_get(eq_tp);
    if (!(instance_eq)) {
        tlkapi_printf(TLK_KEY_EQ_EN, "#eq_func_enable, error load");
        return -1;
    }
    switch (eq_tp.eq_type) {
    case EQ_TYPE_MUSIC:
        //          audio_codec_flag_set(CODEC_FLAG_EQ_MUSIC_EN, 1);
        memset(instance_eq->state, 0, 4 * NSTAGE_EQ_MISIC_SPK);
        tlkapi_printf(TLK_KEY_EQ_EN, "EQ state : EQ is loaded for MUSIC ");
        break;

    case EQ_TYPE_VOICE_MIC:
        //          audio_codec_flag_set(CODEC_FLAG_EQ_VOICE_MIC_EN, 1);
        memset(instance_eq->state, 0, 4 * NSTAGE_EQ_VOICE_MIC);
        tlkapi_printf(TLK_KEY_EQ_EN, "EQ state : EQ is loaded for VOICE_MIC ");
        break;

    case EQ_TYPE_VOICE_SPK:
        //          audio_codec_flag_set(CODEC_FLAG_EQ_VOICE_SPK_EN, 1);
        memset(instance_eq->state, 0, 4 * NSTAGE_EQ_VOICE_SPK);
        tlkapi_printf(TLK_KEY_EQ_EN, "EQ state : EQ is loaded for VOICE_SPK ");
        break;

    default:
        tlkapi_printf(TLK_KEY_EQ_EN, "eq_func_enable : EQ parameters are not initialized");
        break;
    }
    return 0;
}

int eq_func_disable(eq_para_t eq_tp)
{
    nds_bq_df1_f32_t *instance_eq = eq_instance_get(eq_tp);
    if (!(instance_eq)) {
        tlkapi_printf(TLK_KEY_EQ_EN, "#eq_func_disable, error load");
        return -1;
    }
    switch (eq_tp.eq_type) {
    case EQ_TYPE_MUSIC:
        //          audio_codec_flag_set(CODEC_FLAG_EQ_MUSIC_EN, 0);
        tlkapi_printf(TLK_KEY_EQ_EN, "EQ state : EQ is disable for MUSIC ");
        break;

    case EQ_TYPE_VOICE_MIC:
        //          audio_codec_flag_set(CODEC_FLAG_EQ_VOICE_MIC_EN, 0);
        tlkapi_printf(TLK_KEY_EQ_EN, "EQ state : EQ is disable for VOICE_MIC ");
        break;

    case EQ_TYPE_VOICE_SPK:
        //          audio_codec_flag_set(CODEC_FLAG_EQ_VOICE_SPK_EN, 0);
        tlkapi_printf(TLK_KEY_EQ_EN, "EQ state : EQ is disable for VOICE_SPK ");
        break;

    default:
        tlkapi_printf(TLK_KEY_EQ_EN, "eq_func_disable : EQ parameters are not initialized");
        break;
    }
    return 0;
}

    #define MAX_1_IN_3(a, b, c)  (a > b ? (a > c ? a : c) : (b > c ? b : c))
    #define MY_EQ_OTA_NSTAGE_MAX MAX_1_IN_3(NSTAGE_EQ_VOICE_MIC_MAX, NSTAGE_EQ_VOICE_SPK_MAX, NSTAGE_EQ_MISIC_SPK_MAX)

//eq parameter for ota
int   my_eq_nstage_ota = 0;
float my_eq_coef_ota[MY_EQ_OTA_NSTAGE_MAX * 5];

//eq function for ota
int eq_coef_load_ota(eq_para_t eq_tp, u8 *p)
{
    nds_bq_df1_f32_t *instance_eq = eq_instance_get(eq_tp);

    if (!(instance_eq && p)) {
        tlkapi_printf(TLK_KEY_EQ_EN, "#eq_coef_load_ota, error p");
        return 1;
    }

    instance_eq->nstage = eq_tp.eq_nstage;
    memcpy((uint32_t *)instance_eq->coeff, (uint32_t *)p, eq_tp.eq_nstage * 20);
    eq_para_dump(&eq_tp);
    eq_coeff_dump(instance_eq);
    tlkapi_printf(TLK_KEY_EQ_EN, "\n\r# my_debug_eq, data translate ok");
    eq_func_enable(eq_tp);


    return 0;
}

int eq_get_max_stage(uint8_t eq_type)
{
    switch (eq_type) {
    case EQ_TYPE_MUSIC:
        return NSTAGE_EQ_MISIC_SPK_MAX;
        break;
    case EQ_TYPE_VOICE_MIC:
        return NSTAGE_EQ_VOICE_MIC_MAX;
        break;
    case EQ_TYPE_VOICE_SPK:
        return NSTAGE_EQ_VOICE_SPK_MAX;
        break;
    }
    return 0;
}

my_eq_ram_code void eq_proc(eq_para_t para, signed short *ps, signed short *pd, unsigned int nsample)
{
    nds_bq_df1_f32_t *nds_bq_p = eq_instance_get(para);
    if (!(ps && pd && nds_bq_p) || nsample > 1024) {
        tlkapi_printf(TLK_KEY_EQ_EN, "# eq_proc, error");
        return;
    }

    eq_process_int16(nds_bq_p, ps, pd, nsample);
}

//eq function for ota by usb
int my_debug_eq(unsigned char *p, int len)
{
    static eq_para_t eq_tp_debug;
    static short     my_debug_eq_len, my_debug_eq_n;
    tlkapi_printf(TLK_KEY_EQ_EN, "my_debug_eq", p, len);
    tlkapi_printf(TLK_KEY_EQ_EN, "\n\rmy_debug_eq:0x%x,len:%d\n\r", p[0], len);
    float32_t        *data_tp     = (float32_t *)(p + 4);
    uint8_t          *data_ttp    = 0;
    nds_bq_df1_f32_t *instance_eq = eq_instance_get(eq_tp_debug);
    if ((p[0] != 0x12) && (!(instance_eq))) //cmd 0x12 means begin
    {
        tlkapi_printf(TLK_KEY_EQ_EN, "# my_debug_eq, eq_instance_get error");
        return -1;
    }

    int i = 0;
    switch (p[0]) {
    case 0x12: //cmd 0x12 means begin(4bytes) and eq parameter(52bytes)

        data_tp++;
        data_ttp = (uint8_t *)data_tp;
        memcpy(&eq_tp_debug, p + 4, sizeof(eq_para_t));
        instance_eq = eq_instance_get(eq_tp_debug);

        if (!(instance_eq)) {
            tlkapi_printf(TLK_KEY_EQ_EN, "# my_debug_eq, eq_instance_get error");
            return -1;
        }

        if (eq_get_max_stage(eq_tp_debug.eq_type) < eq_tp_debug.eq_nstage) {
            tlkapi_printf(TLK_KEY_EQ_EN, "# my_debug_eq, data coeff is too little");
            my_debug_eq_len = 0;
            my_debug_eq_n   = 0;
        } else {
            instance_eq->nstage = eq_tp_debug.eq_nstage;
            my_debug_eq_len     = eq_tp_debug.eq_nstage * 20;
            //eq parameter(52bytes)
            for (my_debug_eq_n = 0; (my_debug_eq_n < my_debug_eq_len) && (my_debug_eq_n < (20 * eq_get_max_stage(eq_tp_debug.eq_type))) && (i < 52); my_debug_eq_n++, i++) {
                ((uint8_t *)my_eq_coef_ota)[my_debug_eq_n] = data_ttp[i];
            }
        }
        break;
    case 0x13: //cmd 0x13,0x14,0x15,0x16,0x17,0x18,0x19 means eq parameter(56bytes)
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
        data_ttp = (uint8_t *)data_tp;
        //eq parameter(56bytes)
        for (; (my_debug_eq_n < my_debug_eq_len) && (my_debug_eq_n < (20 * eq_get_max_stage(eq_tp_debug.eq_type))) && (i < 56); my_debug_eq_n++, i++) {
            ((uint8_t *)my_eq_coef_ota)[my_debug_eq_n] = data_ttp[i];
        }

        if ((my_debug_eq_len == my_debug_eq_n) && (my_debug_eq_n != 0)) {
            //Generally speaking, music and voice speak nead 2 speakers, we set them as same eq parameter.
            if ((eq_tp_debug.eq_type == EQ_TYPE_MUSIC) || (eq_tp_debug.eq_type == EQ_TYPE_VOICE_SPK)) {
                eq_tp_debug.eq_channel = EQ_CHANNEL_LEFT;
                eq_coef_load_ota(eq_tp_debug, (u8 *)my_eq_coef_ota);

                eq_tp_debug.eq_channel = EQ_CHANNEL_RIGHT;
                eq_coef_load_ota(eq_tp_debug, (u8 *)my_eq_coef_ota);
            } else if (eq_tp_debug.eq_type == EQ_TYPE_VOICE_MIC) //Generally speaking, voice mic always need 1 mic.
            {
                eq_coef_load_ota(eq_tp_debug, (u8 *)my_eq_coef_ota);
            }

            my_debug_eq_len = 0;
            my_debug_eq_n   = 0;
        }

        break;
    default:
    {
        tlkapi_printf(TLK_KEY_EQ_EN, "my_debug_eq,error");
    } break;
    }

    return 0;
}
#endif
