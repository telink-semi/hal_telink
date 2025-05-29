/********************************************************************************************************
 * @file    ecdh.h
 *
 * @brief   This is the header file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef ECDH_H
#define ECDH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pke.h"

//ECDH return code
enum ECDH_RET_CODE
{
    ECDH_SUCCESS = PKE_SUCCESS,
    ECDH_POINTOR_NULL = PKE_SUCCESS+0x60,
    ECDH_INVALID_INPUT,
};




//APIs
/**
 * @brief       ECDH compute key
 * @param[in]   curve           - curve structured data types.
 * @param[in]   local_prikey    - local private key, big-endian.
 * @param[in]   peer_pubkey     - peer public key, big-endian.
 * @param[out]  key             - output key.
 * @param[in]   keyByteLen      - byte length of output key.
 * @param[in]   kdf             - kdf function to get key.
 * @return      0:success     other:error
 */
unsigned int ecdh_compute_key(eccp_curve_t *curve, unsigned char *local_prikey, unsigned char *peer_pubkey, unsigned char *key,
        unsigned int keyByteLen, KDF_FUNC kdf);



#ifdef __cplusplus
}
#endif

#endif
