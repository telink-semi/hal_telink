/********************************************************************************************************
 * @file    tlka_w_ns_api.h
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
#ifndef W_NS_API_H
#define W_NS_API_H

#include <stddef.h>

/*! Version number to ensure header and binary are matching. */
#define W_NS_VERSION_INT(major, minor, micro) (((major) << 16) | ((minor) << 8) | (micro))
#define W_NS_VERSION W_NS_VERSION_INT(0, 5, 0)

/* Whether to enable the GSC module */
#define OUTPUTFREQ 0
#define WEBRTC_OK (0)

typedef struct
{
    /* support 5ms/7.5ms/10ms
     * 5ms  : 50
     * 7.5ms: 75
     * 10ms : 100 */
    unsigned int frame_size;

    /* only support 16K */
    unsigned int sampleRate;

    /* support k6dB, k12dB, k18dB, k21dB */
    unsigned int target_level;

    /* lowShelf_filter enable
     * 0: disable
     * 1: enable */
    unsigned int lowShelf_En;

    float preGain;
    float postGain;
} W_NS_CFG_PARAM;

enum target_level
{
    k6dB,
    k12dB,
    k14dB,
    k16dB,
    k18dB,
    k21dB,
    K_END
};

#define kFftSize_MAX 256
#define kFftSizeBy2Plus1MAX 129
#define kFftSizeBy2Plus1MAXALINE 160
#define kOverlapSizeMAX 136
#define FRAME_SIZE_80 80

#define kHistogramSize 500
#define kSimult 3

typedef struct
{
    float pre_prior_snr[kFftSizeBy2Plus1MAXALINE];
    float signal_spectrum[kFftSizeBy2Plus1MAXALINE];
    float parametric_noise_spectrum[kFftSizeBy2Plus1MAXALINE];
    float real[kFftSize_MAX];
    float imag[kFftSize_MAX];

} Scratch_Param;

typedef struct st_mcra_t_tag{
    float P[kFftSizeBy2Plus1MAX];
    float Pmin[kFftSizeBy2Plus1MAX];
    float Ptmp[kFftSizeBy2Plus1MAX];
    float noise_ps[kFftSizeBy2Plus1MAX];
    float pk[kFftSizeBy2Plus1MAX];
    float as;
    float ad;
    float ap;
    float delta;
    int L;
    int n;
    int len;

} st_mcra_t;

typedef struct
{
    st_mcra_t st_mcra;
    unsigned int lowShelf_En;
    float preGain;
    float postGain;

    float energies_before_filtering;

    float prev_analysis_signal_spectrum[kFftSizeBy2Plus1MAX];
    float analyze_analysis_memory[kFftSize_MAX - FRAME_SIZE_80];
    float process_synthesis_memory[kOverlapSizeMAX];

    int num_analyzed_frames; // int32_t

    size_t bit_reversal_state[kFftSize_MAX / 2];
    float tables[kFftSize_MAX / 2];

    /* NoiseSuppressor */
    size_t num_bands_;
    size_t num_framesize_;
    size_t num_fftsize_;
    size_t num_fftsizeby2plus1_;
    size_t num_subbands_;

    /* SpeechProbabilityEstimator */
    float prior_speech_prob_;

    /* SignalModelEstimator */
    float diff_normalization_;
    float signal_energy_sum_;
    int histogram_analysis_counter_;

    /* Histograms */
    //float Histograms_lrt[kHistogramSize];
    //float Histograms_spectral_flatness[kHistogramSize];
    //float Histograms_spectral_diff[kHistogramSize];

    /* SignalModel */
    float SignalModel_lrt;
    float SignalModel_spectral_diff;
    float SignalModel_spectral_flatness;
    float SignalModel_avg_log_lrt[kFftSizeBy2Plus1MAX];

    /* PriorSignalModel */
    float PriorSignalModel_lrt;
    float PriorSignalModel_flatness_threshold;
    float PriorSignalModel_template_diff_threshold;
    float PriorSignalModel_lrt_weighting;
    float PriorSignalModel_flatness_weighting;
    float PriorSignalModel_difference_weighting;

    /* QuantileNoiseEstimator */
    //float density_[kSimult * kFftSizeBy2Plus1MAX];
    //float log_quantile_[kSimult * kFftSizeBy2Plus1MAX];
    //float quantile_[kFftSizeBy2Plus1MAX];
    //float counter_[kSimult];
    //int num_updates_;

    /* SuppressionParams */
    float over_subtraction_factor;
    float minimum_attenuating_gain;
    int use_attenuation_adjustment; // bool

    /* NoiseEstimator */
    float white_noise_level_;
    float pink_noise_numerator_;
    float pink_noise_exp_;

    float prev_noise_spectrum_[kFftSizeBy2Plus1MAX];
    //float conservative_noise_spectrum_[kFftSizeBy2Plus1MAX];
    float noise_spectrum_[kFftSizeBy2Plus1MAX];

    /* WienerFilter */
    float initial_spectral_estimate_[kFftSizeBy2Plus1MAX];
    float filter_[kFftSizeBy2Plus1MAX];

    /* pointer to scratch buffer */
    int   *pScratch;
} W_NsState_C;

int tlka_w_ns_get_scratch_size(void);

int tlka_w_ns_get_version(void);

int tlka_w_ns_get_size(void);

void tlka_w_ns_init(void *sc, W_NS_CFG_PARAM w_ns_cfg_para, void *ScratchBuffer);

int tlka_w_ns_process_frame(void *sc, short *buffer);


#endif
