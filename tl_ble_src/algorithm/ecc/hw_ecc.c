/********************************************************************************************************
 * @file    hw_ecc.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
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
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include <algorithm/ecc/hw_ecc.h>
#include "stack/ble/ble_config.h"
#include "drivers.h"


eccp_curve_t* blt_ecc_get_eccp_curve(ecc_curve_t curve)
{
    eccp_curve_t* pEccCurve = NULL;
    switch (curve){
//      case ECC_use_secp160r1:
//          pEccCurve = (eccp_curve_t*)secp160r1;
//      break;

#ifdef SUPPORT_SECP192R1
        case ECC_use_secp192r1:
            pEccCurve = (eccp_curve_t*)secp192r1;
        break;
#endif

#ifdef SUPPORT_SECP224R1
        case ECC_use_secp224r1:
            pEccCurve = (eccp_curve_t*)secp224r1;
        break;
#endif

#ifdef SUPPORT_SECP256R1
        case ECC_use_secp256r1:
            pEccCurve = (eccp_curve_t*)secp256r1;
        break;
#endif

//      case ECC_use_secp256k1:
//          pEccCurve = (eccp_curve_t*)secp256k1;
//      break;

        default:
            pEccCurve = (eccp_curve_t*)secp256r1;
        break;
    }

    return pEccCurve;
}


_attribute_data_retention_
static hECC_rng_func g_rng_function = NULL;

void hwECC_set_rng(hECC_rng_func rng_func) {
    g_rng_function = rng_func;

#if ((MCU_CORE_TYPE == MCU_CORE_TL751X) || (MCU_CORE_TYPE == MCU_CORE_TL322X) || (MCU_CORE_TYPE == MCU_CORE_TL323X) )
    pke_dig_en();
#endif

}

/**
 * @brief       get ECCP key pair(the key pair could be used in ECDH).
 * @param[out]  public_key  - public key, big--endian.
 * @param[out]  private_key - private key, big--endian.
 * @return      1(success), 0(error).
 */
unsigned char hwECC_make_key(unsigned char *public_key, unsigned char *private_key, ecc_curve_t curve_sel)
{
    eccp_curve_t *curve = blt_ecc_get_eccp_curve(curve_sel);

    unsigned char ret;
    unsigned int tmpLen;
    unsigned int k[PKE_OPERAND_MAX_WORD_LEN] = {0};
    unsigned int x[PKE_OPERAND_MAX_WORD_LEN] = {0};
    unsigned int y[PKE_OPERAND_MAX_WORD_LEN] = {0};
    unsigned int nByteLen = GET_BYTE_LEN(curve->eccp_n_bitLen);
    unsigned int nWordLen = GET_WORD_LEN(curve->eccp_n_bitLen);
    unsigned int pByteLen = GET_BYTE_LEN(curve->eccp_p_bitLen);

    ECCP_GETKEY_LOOP:

    if(g_rng_function == NULL)
    {
        return 0;
    }

    if(!g_rng_function((unsigned char *)k, nByteLen))
    {
        return 0;
    }

    //make sure k has the same bit length as n
    tmpLen = (curve->eccp_n_bitLen)&0x1F;
    if(tmpLen)
    {
        k[nWordLen-1] &= (1<<(tmpLen))-1;
    }

    //make sure k in [1, n-1]
    if(ismemzero4(k, nWordLen<<2))
    {
        goto ECCP_GETKEY_LOOP;
    }
    if(big_integer_compare(k, nWordLen, curve->eccp_n, nWordLen) >= 0)
    {
        goto ECCP_GETKEY_LOOP;
    }
    //get public_key
    ret = pke_eccp_point_mul(curve, k, curve->eccp_Gx, curve->eccp_Gy, x, y);
    if(PKE_SUCCESS != ret)
    {
        return 0; //Q=[k]P Failed
    }

    //to big-end
    swapX((unsigned char *)k, private_key, nByteLen);
    swapX((unsigned char *)x, public_key, pByteLen);
    swapX((unsigned char *)y, public_key + pByteLen, pByteLen);

    return 1;
}


/**
 * @brief       ECDH compute key.
 * @param[in]   local_prikey    - local private key, big--endian.
 * @param[in]   public_key      - peer public key, big--endian.
 * @param[out]  dhkey           - output dhkey, big--endian..
 * @Return      1(success); 0(error).
 */
unsigned char hwECC_shared_secret(const unsigned char *public_key, const unsigned char *private_key, \
                                  unsigned char *secret, ecc_curve_t curve_sel)
{
    eccp_curve_t *curve = blt_ecc_get_eccp_curve(curve_sel);

    unsigned char ret;
    unsigned int k[ECC_MAX_WORD_LEN] = {0};
    unsigned int Px[ECC_MAX_WORD_LEN] = {0};
    unsigned int Py[ECC_MAX_WORD_LEN] = {0};
    unsigned int byteLen = GET_BYTE_LEN(curve->eccp_n_bitLen);
    unsigned int wordLen = GET_WORD_LEN(curve->eccp_n_bitLen);


    if(0 == private_key || 0 == public_key || 0 == secret)
    {
        return 0;
    }



    //make sure private key is in [1, n-1]
    swapX(private_key, (unsigned char *)k, byteLen);

    if(ismemzero4(k, wordLen<<2))
    {
        return 0;
    }
    if(big_integer_compare(k, wordLen, curve->eccp_n, wordLen) >= 0)
    {
        return 0;
    }

    //check public key
    swapX(public_key, (unsigned char *)Px, byteLen);
    swapX(public_key+byteLen, (unsigned char *)Py, byteLen);
    ret = pke_eccp_point_verify(curve, Px, Py);
    if(PKE_SUCCESS != ret)
    {
        return 0;
    }

    ret = pke_eccp_point_mul(curve, k, Px, Py, Px, Py);
    if(PKE_SUCCESS != ret)
    {
        return 0; //Q=[k]P Failed
    }

    swapX((unsigned char *)Px, secret, byteLen);

    return 1;
}

