/********************************************************************************************************
 * @file    bf_aec_ns.c
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
#include "bf_aec_ns.h"
#include "common/compiler.h"
#include "tl_common.h"


#include "../audio_alg_debug.h"

///to GSC
#if ALG_GSC_EN
gscState        *g_gsc_st_p   = NULL;


//_attribute_ram_code_
int tlk_gsc_BeamFormer(short *x_in, short *ref_in, short *x_out, gscState *st, int was_speech)
{
    int ret;
//  log_task (SL_APP_COMMON_EN, SL01_mic_bf_aec_ns, 1);
    if (alg_audio_mask & FLG_ALG_AUDIO_GSC_MASK)
    {
        ret = gsc_BeamFormer(x_in, ref_in, x_out, st, was_speech);
    }
    else
    {
        ret = 0;
    }
//  log_task (SL_APP_COMMON_EN, SL01_mic_bf_aec_ns, 0);
    return ret;
}


#endif

///TO AEC and S_NS
#if (ALG_AEC_EN|ALG_S_NS)

void *g_aec_st      = NULL;
void *g_s_ns_den    = NULL;
void *scratch_buff  = NULL;


AEC_CFG_PARAS aecParas =
    {
        .sampling_rate = 16000,
        .frame_size = 80,
        .use_pre_emp = 1,      /* 1: enable pre-emphasis filter, 0: disable pre-emphasis filter */
        .use_dc_notch = 0,     /* 1: enable DC removal filter, 0: disable DC removal filter */
        .sampling_rate = 16000,    /* sample rate */
    };

NS_CFG_PARAS nsParas =
{
    .sampling_rate = 16000,
    .frame_size = 80,
    .low_shelf_enable = 1,                              //low_shelf_enable
    .post_gain_enable = 1,                              //post_gain_enable
    .hf_cutting_enable = 0,                             //hf_cutting_enable
    .noise_suppress_default = -25,                      //noise_suppress_default
    .echo_suppress_default = -55,                       //
    .echo_suppress_active_default = -45,                //
    .ns_smoothness = 27853,                             //ns_smoothness
    .ns_threshold_low = 0.0f,                           //ns_threshold_low
};

/**
 * @brief the AEC module is initialized
 *
 * @param[in] sample rate
 * @param[in] frame size
 *
 * @returns none
 */
//__attribute__((noinline))
void tlk_aec_init(unsigned int sample_rate, unsigned char frame_size)
{
//    g_voice_frame_samples = frame_size;
//    audio_rx_dma_dis();
//    audio_tx_dma_dis();
//    dma_set_address(DMA3, (u32)convert_ram_addr_cpu2bus(buff_playback), REG_AUDIO_AHB_BASE);
//    dma_set_address(DMA2, REG_AUDIO_AHB_BASE, (u32)convert_ram_addr_cpu2bus(buff_mic)); // SRC_addr  //dst addr ch0 adc
//
//    // dma_set_address( DMA2,(u32)convert_ram_addr_cpu2bus(buff_playback),REG_AUDIO_AHB_BASE);
//    // dma_set_size(DMA3,240*8,DMA_WORD_WIDTH);
//    // dma_set_size(DMA2,240*8,DMA_WORD_WIDTH);
//
//    dma_set_size(DMA3, (TWS_PLAY_FIFO_SIZE * sizeof(codec_int)), DMA_WORD_WIDTH); // Speaker DMA
//    dma_set_size(DMA2, (TWS_MIC_FIFO_SIZE * sizeof(adc_int)), DMA_WORD_WIDTH);    // Mic  DMA
//
//    audio_tx_dma_en();
//    audio_rx_dma_en();
//
//    cur_mic_widx          = ((audio_get_rx_dma_wptr(DMA2) - (u32)buff_mic) / sizeof(adc_int));
//    cur_mic_ridx          = cur_mic_widx;
//    cur_ec_to_encoder_idx = cur_mic_ridx;
//
//
//    cur_spk_ec_ridx = cur_mic_ridx - ec_delay_time; // set the spk ec point
//    if (cur_spk_ec_ridx < 0) {
//        cur_spk_ec_ridx = cur_spk_ec_ridx + EC_BUFF_SPK_SIZE;
//    }

}
#endif

