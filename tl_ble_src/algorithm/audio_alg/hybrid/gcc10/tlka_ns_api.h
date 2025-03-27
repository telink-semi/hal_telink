#ifndef W_NS_API_H
#define W_NS_API_H

//#include <stddef.h>

#define NS_MICNUM 1
#define NS_INOUT_FREQ 1

#define WEBRTC_OK 0

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



/*
typedef struct
{
    float pre_prior_snr[kFftSizeBy2Plus1MAXALINE];
    float signal_spectrum[kFftSizeBy2Plus1MAXALINE];
    float parametric_noise_spectrum[kFftSizeBy2Plus1MAXALINE];

} Scratch_Param;
*/



//int tlka_w_ns_get_scratch_size();

//int tlka_w_ns_get_version(void);

int tlka_w_ns_get_size();

void tlka_w_ns_init(void *sc, W_NS_CFG_PARAM *pst_para, void *ScratchBuffer);

#if NS_INOUT_FREQ

/*------------------------------------------------------*
* name: tlka_w_ns_process_frame                          *
* st   : input struct pointer                           *
* ref_in : input reference spec buffer                *
* x_in : input mic spec buffer                        *
* x_out : output spec buffer                          *
*-------------------------------------------------------*/
int tlka_w_ns_process_frame(void *sc, float *in_spec, float *out_spec);

#else


#endif


#endif
