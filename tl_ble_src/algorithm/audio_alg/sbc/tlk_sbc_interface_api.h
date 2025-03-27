/********************************************************************************************************
 * @file    tlk_sbc_interface_api.h
 *
 * @brief   This is the header file for b91_btll_cc_general_sdk
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
#ifndef CODEC_SBC_TLK_SBC_INTERFACE_API_H_
#define CODEC_SBC_TLK_SBC_INTERFACE_API_H_

#include "config.h"
#include "tlka_sbc_api.h"


extern uint8_t *g_sbc_dec_buf_ptr;
extern uint8_t *g_msbc_dec_buf_ptr;
extern uint8_t *g_msbc_enc_buf_ptr;
extern uint8_t *g_msbc_enc_buf_test_mode_ptr;

extern SBC_CFG_Param g_sbc_param;

/**
 * @brief  The SBC decodes the left channel
 *
 * @param[in]  input the undecoded data pointer
 * @param[in]  input the data length
 * @param[out] Outputs the decoded data pointer
 *
 * @returns Decoding state
 */
int tlkalg_sbc_dec_channel0(uint8_t *ps, int len, uint8_t *pd);

/**
 * @brief  The SBC decodes the right channel
 *
 * @param[in]  input the undecoded data pointer
 * @param[in]  input the data length
 * @param[out] Outputs the decoded data pointer
 *
 * @returns Decoding state
 */
int tlkalg_sbc_dec_channel1(uint8_t *ps, int len, uint8_t *pd);

/**
 * @brief  The SBC decodes the stereo
 *
 * @param[in]  input the undecoded data pointer
 * @param[in]  input the data length
 * @param[out] Outputs the decoded data pointer
 *
 * @returns Decoding state
 */
int tlkalg_sbc_dec_stereo(uint8_t *ps, int len, uint8_t *pd);

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
int tlkalg_msbc_enc_ptr(sbc_enc_para_t *msbc_enc_ptr, uint8_t *ps, int len, uint8_t *pd);

/**
 * @brief  mSBC encoded
 *
 * @param[in]  input the unencoded data pointer
 * @param[in]  input the data length
 * @param[out] Outputs the encoded data pointer
 *
 * @returns encoding state
 */
int tlkalg_msbc_enc(uint8_t *ps, int len, uint8_t *pd);

/**
 * @brief  The mSBC decodes
 *
 * @param[in]  input the undecoded data pointer
 * @param[in]  input the data length
 * @param[out] Outputs the decoded data pointer
 *
 * @returns Decoding state
 */
int tlkalg_msbc_dec(uint8_t *ps, int len, uint8_t *pd);

#endif /* CODEC_SBC_TLK_SBC_INTERFACE_API_H_ */
