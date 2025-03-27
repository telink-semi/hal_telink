#ifndef SBC_API_H
#define SBC_API_H

//#define F_FLOAT

#include <stdint.h>

/*! Construct version number from major/minor/micro values. */
#define SBC_VERSION_INT(major, minor, micro) (((major) << 16) | ((minor) << 8) | (micro))

/*! Version number to ensure header and binary are matching. */
#define SBC_VERSION SBC_VERSION_INT(0, 4, 8)

#define SBC_MAX_CHANNELS    2
#define SBC_X_BUFFER_SIZE   328
#define SBC_MAX_SUBBANDS    8
#define SBC_MAX_BLOCKS      16

typedef unsigned char u8;

/*! Return library version number. It should match SBC_VERSION. */
int tlka_sbc_get_version(void);

typedef struct sbc_enc_para
{
    int16_t X[SBC_MAX_CHANNELS][SBC_X_BUFFER_SIZE];
    uint32_t scale_factor[SBC_MAX_CHANNELS][SBC_MAX_SUBBANDS];
    int32_t sb_sample_f[SBC_MAX_BLOCKS][SBC_MAX_CHANNELS][SBC_MAX_SUBBANDS];
    int32_t position;
    uint32_t sbc_analyze_even;

    u8 sbc_blocks;
    u8 sbc_bitpool;
    u8 sbc_allocation;
    int16_t sbc_samplerate;
    u8 sbc_channel;

}sbc_enc_para_t;

typedef struct sbc_dec_para
{

#ifdef F_FLOAT
    float V[2][160];  
#else
    int16_t V[2][160];
#endif

    u8 sbc_blocks;
    u8 sbc_bitpool;
    u8 sbc_allocation;

}sbc_dec_para_t;

/*------------------------------------------------------*
* name:  _SBC_CFG_Param                                 *
* sbc param struct                                      *
*-------------------------------------------------------*/
typedef struct __attribute__((packed)) _SBC_CFG_Param
{
    u8 sbc_blocks;
    u8 sbc_bitpool;
    u8 sbc_allocation;
    int16_t sbc_samplerate;
    u8 sbc_channel;
    u8 msbc;

}SBC_CFG_Param;

#ifndef SBC_SYNCWORD
#define SBC_SYNCWORD    0x9C
#endif

#ifndef MSBC_SYNCWORD
#define MSBC_SYNCWORD   0xAD
#endif


int tlka_sbc_enc_get_size(void);
int tlka_sbc_dec_get_size(void);

/*init sbc enc/dec*/
int tlka_sbc_enc_init(sbc_enc_para_t* encoder_p, SBC_CFG_Param* sbc_param);
int tlka_sbc_dec_init(sbc_dec_para_t* decoder_p, SBC_CFG_Param* sbc_param);

/*init msbc enc/dec*/
int tlka_msbc_enc_init(sbc_enc_para_t* encoder_p);
int tlka_msbc_dec_init(sbc_dec_para_t* decoder_p);

/*set bitpool*/
void tlka_sbc_set_enc_blocks_bitpool(sbc_enc_para_t* encoder_p,u8 blocks,u8 bitpool);
void tlka_sbc_set_dec_blocks_bitpool(sbc_dec_para_t* decoder_p,u8 blocks,u8 bitpool);

/*----------------------------------------------------------*
* tlka_sbc_dec_process                                      *
*                                                           *
* decoder   : decoder struct                                *
* buf       : input buffer                                  *
* len       : codesize = subbands * blocks * channels * 2   *
* outbuf    : output buffer                                 *
* out_len   : output buffer len                             *
* msbc      : 1->msbc   0->sbc                              *
* sbc_out_chn_mask : 1                                      *
*                                                           *
* return    :  encoder frame size                                   *
*           0: frame header error                           *
*-----------------------------------------------------------*/
uint32_t tlka_sbc_dec_process(sbc_dec_para_t* decoder, const uint8_t* buf, uint32_t len, uint8_t* outbuf, uint32_t* out_len, int msbc, uint8_t sbc_out_chn_mask);


/*----------------------------------------------------------*
* tlka_sbc_enc_process                                      *
*                                                           *
* encoder   : encoder struct                                *
* buf       : input buffer                                  *
* len       : codesize = subbands * blocks * channels * 2   *
* outbuf    : output buffer                                 *
* out_len   : encoder output frmae len                      *
* msbc      : 1->msbc   0->sbc                              *
*                                                           *
* return    : codesize                                      *
*             0 -> encoder error                            *
*-----------------------------------------------------------*/
uint32_t tlka_sbc_enc_process(sbc_enc_para_t* encoder, int16_t* buf, uint16_t len,
        uint8_t* outbuf, uint32_t* out_len, int msbc);


#endif
