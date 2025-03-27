/********************************************************************************************************
 * @file    tlk_sbc_interface_api.c
 *
 * @brief   This is the source file for b91_btll_cc_general_sdk
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
#include "tlk_sbc_interface_api.h"
#include "types.h"
#include "tl_common.h"

#define SBC_LOG_EN      0

uint8_t *g_sbc_dec_buf_ptr            = NULL;
uint8_t *g_msbc_dec_buf_ptr           = NULL;
uint8_t *g_msbc_enc_buf_ptr           = NULL;
uint8_t *g_msbc_enc_buf_test_mode_ptr = NULL;

SBC_CFG_Param g_sbc_param = {
    .sbc_blocks     = 10,//10:5ms frame,80 sample;15:7.5ms frame,120 sample;
    .sbc_bitpool    = 26,
    .sbc_allocation = 0,
    .sbc_samplerate = 16000,
    .sbc_channel    = 1,
    .msbc           = 1,

};

/**
 * @brief  The SBC decodes the left channel
 *
 * @param[in]  input the undecoded data pointer
 * @param[in]  input the data length
 * @param[out] Outputs the decoded data pointer
 *
 * @returns Decoding state
 */
int tlkalg_sbc_dec_channel0(uint8_t *ps, int len, uint8_t *pd)
{
    uint32_t dlen = 0;
    uint32_t ret  = 0;

    if ((ps == NULL || pd == NULL)) {
        tlkapi_printf(SBC_LOG_EN,"SBC dec ch0 input buff point null");
        return ret;
    }

    if (g_sbc_dec_buf_ptr == NULL) {
        tlkapi_printf(SBC_LOG_EN,"SBC dec struct point null");
        return ret;
    }

    ret = tlka_sbc_dec_process((sbc_dec_para_t *)g_sbc_dec_buf_ptr,
                               (const uint8_t *)ps,
                               (uint32_t)len,
                               (uint8_t *)pd,
                               &dlen,
                               0x00,
                               0x01);
    if (ret && dlen) {
        ret = 1;
    } else {
        tlkapi_printf(SBC_LOG_EN,"SBC dec ch0 err");
    }
    return ret;
}

/**
 * @brief  The SBC decodes the right channel
 *
 * @param[in]  input the undecoded data pointer
 * @param[in]  input the data length
 * @param[out] Outputs the decoded data pointer
 *
 * @returns Decoding state
 */
int tlkalg_sbc_dec_channel1(uint8_t *ps, int len, uint8_t *pd)
{
    uint32_t dlen = 0;
    uint32_t ret  = 0;

    if ((ps == NULL || pd == NULL)) {
        tlkapi_printf(SBC_LOG_EN, "SBC dec ch1 input buff point null");
        return ret;
    }

    if (g_sbc_dec_buf_ptr == NULL) {
        tlkapi_printf(SBC_LOG_EN, "SBC dec struct point null");
        return ret;
    }

    ret = tlka_sbc_dec_process((sbc_dec_para_t *)g_sbc_dec_buf_ptr,
                               (const uint8_t *)ps,
                               (uint32_t)len,
                               (uint8_t *)pd,
                               &dlen,
                               0x00,
                               0x02);
    if (ret && dlen) {
        ret = 1;
    } else {
        tlkapi_printf(SBC_LOG_EN, "SBC dec ch1 err");
    }
    return ret;
}

/**
 * @brief  The SBC decodes the stereo
 *
 * @param[in]  input the undecoded data pointer
 * @param[in]  input the data length
 * @param[out] Outputs the decoded data pointer
 *
 * @returns Decoding state
 */
int tlkalg_sbc_dec_stereo(uint8_t *ps, int len, uint8_t *pd)
{
    uint32_t dlen = 0;
    uint32_t ret  = 0;

    if ((ps == NULL || pd == NULL)) {
        tlkapi_printf(SBC_LOG_EN,"SBC dec stereo input buff point null");
        return ret;
    }

    if (g_sbc_dec_buf_ptr == NULL) {
        tlkapi_printf(SBC_LOG_EN, "SBC dec struct point null");
        return ret;
    }


    ret = tlka_sbc_dec_process((sbc_dec_para_t *)g_sbc_dec_buf_ptr,
                               (const uint8_t *)ps,
                               (uint32_t)len,
                               (uint8_t *)pd,
                               &dlen,
                               0x00,
                               0x03);
    if (ret && dlen) {
        ret = 1;
    } else {
        tlkapi_printf(SBC_LOG_EN,"SBC dec struct err");
    }
    return ret;
}

//////////////////////////////////////////////////////////////////////////

/**
 * @brief  mSBC encoded
 *
 * @param[in]  input the unencoded data pointer
 * @param[in]  input the data length
 * @param[out] Outputs the encoded data pointer
 *
 * @returns encoding state
 */
int tlkalg_msbc_enc(uint8_t *ps, int len, uint8_t *pd)
{
    static uint8_t sn                   = 0;
    unsigned char h2_header_sn_table[4] = { 0x08, 0x38, 0xc8, 0xf8 };
    uint32_t dlen                       = 0;
    uint32_t ret                        = 0;

    if ((ps == NULL || pd == NULL)) {
        tlkapi_printf(SBC_LOG_EN,"mSBC enc input buff point null");
        return ret;
    }

    if (g_msbc_enc_buf_ptr == NULL) {
        tlkapi_printf(SBC_LOG_EN, "mSBC enc struct point null");
        return ret;
    }

    pd[0] = 0x01;
    pd[1] = h2_header_sn_table[sn++ & 3];
    tlka_sbc_enc_process((sbc_enc_para_t *)g_msbc_enc_buf_ptr, (int16_t *)ps, len, pd + 2, &dlen, 0x01);

    return dlen == 57;
}

/**
 * @brief  mSBC encoding with pointer
 *
 * @param[in]  mSBC code structure pointer
 * @param[in]  input the unencoded data pointer
 * @param[in]  input the data length
 * @param[out] Outputs the encoded data pointer
 *
 * @returns encoding state
 */
int tlkalg_msbc_enc_ptr(sbc_enc_para_t *msbc_enc_ptr, uint8_t *ps, int len, uint8_t *pd)
{
    static uint8_t sn             = 0;
    uint8_t h2_header_sn_table[4] = { 0x08, 0x38, 0xc8, 0xf8 };

    uint32_t dlen = 0;
    uint32_t ret  = 0;

    if ((ps == NULL || pd == NULL)) {
        tlkapi_printf(SBC_LOG_EN,"mSBC enc ptr input buff point null");
        return ret;
    }

    if (msbc_enc_ptr == NULL) {
        tlkapi_printf(SBC_LOG_EN,"mSBC enc ptr struct point null");
        return ret;
    }

//    pd[0] = 0x01;
//    pd[1] = h2_header_sn_table[sn++ & 3];
    tlka_sbc_enc_process(msbc_enc_ptr, (int16_t *)ps, len, pd + 2, &dlen, 0x01);

    return dlen;
}

/**
 * @brief  The mSBC decodes
 *
 * @param[in]  input the undecoded data pointer
 * @param[in]  input the data length
 * @param[out] Outputs the decoded data pointer
 *
 * @returns Decoding state
 */
int tlkalg_msbc_dec(uint8_t *ps, int len, uint8_t *pd)
{
    uint32_t dlen = 0;
    uint32_t ret  = 0;

    if ((ps == NULL || pd == NULL)) {
        tlkapi_printf(SBC_LOG_EN, "mSBC dec input buff point null");
        return ret;
    }

    if (g_msbc_dec_buf_ptr == NULL) {
        tlkapi_printf(SBC_LOG_EN, "mSBC dec struct point null");
        return ret;
    }

    if (MSBC_SYNCWORD == ps[4]) {
        ret = tlka_sbc_dec_process((sbc_dec_para_t *)g_msbc_dec_buf_ptr,
                                   (const uint8_t *)ps + 4,
                                   (uint32_t)len,
                                   (uint8_t *)pd,
                                   &dlen,
                                   0x01,
                                   0x01);
        if (ret && dlen) {
            ret = 1;
        }
    } else {
        memset(pd, 0, 240);
        tlkapi_printf(SBC_LOG_EN, "mSBC dec syncword err");
    }
    return ret;
}
