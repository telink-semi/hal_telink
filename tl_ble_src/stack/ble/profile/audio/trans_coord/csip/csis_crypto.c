/********************************************************************************************************
 * @file    csis_crypto.c
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"



static void blt_csis_cryptoSIH(const u8 sirk[16], u8 prand[3], u8 hash[3])
{
    u8 r[16] = {0};
    u8 out[16] = {0};

    r[0] = *prand;
    r[1] = *(prand+1);
    r[2] = *(prand+2);

    aes_encryption_le((u8*)(size_t)sirk, r, out);

    //Little-endian, take the least significant 24 bits.
    *hash     = out[0];
    *(hash+1) = out[1];
    *(hash+2) = out[2];
}

void blc_csis_cryptoGenerateRSI(const u8 SIRK[16], u8 outRSI[6])
{
    // Resolvable private address:
    // LSB                                                     MSB
    // +--------------------------+----------------------+---+---+
    // |                          | Random part of prand | 1 | 0 |
    // +--------------------------+----------------------+---+---+
    // <--------+ hash +---------> <-----------+ prand +--------->
    //          (24 bits)                     (24 bits)
    u8* prand = outRSI + 3;
    generateRandomNum(3, prand);
    prand[2] = (prand[2] & 0x3F) | 0x40;

    blt_csis_cryptoSIH(SIRK, prand, outRSI);
}

static void blt_csis_cryptoSefSdf(u8* in, u8* ltk, u8* out)
{
    //s1(M) = AES-CMACzero(M), M="SIRKenc", salt = ZERO
    unsigned char s1[16] = {0};
    unsigned char SIRKenc[7] = {'S','I','R','K','e','n','c'};
    blt_crypto_alg_csip_s1(SIRKenc, 7, s1);

    //k1(N,SALT,P) = AES-CMACT(P), P="csis", salt = T
    //T = AES-CMACsalt(N), N=ltk, salt = s1("SIRKenc")
    unsigned char k1[16] = {0};
    unsigned char csis_str[4] = {'c','s','i','s'};
    blt_crypto_alg_h8(k1, ltk, s1, csis_str);

    for(u8 i=0; i<16; i++)
    {
        out[i] = k1[i] ^ in[i];
    }
}
/*
 * The output of the SIRK encryption function sef is as follows:
 *  sef(K, SIRK) = k1(K, s1("SIRKenc"), "csis") ^ SIRK
 * The output of the SIRK decryption function sdf is as follows:
 *  sdf(K, EncSIRK) = k1(K, s1("SIRKenc"), "csis") ^ EncSIRK
 */
void blt_csis_cryptoSIRKEncDec(u16 connHandle, u8* in, u8* out)
{
    //Get LTK
    u8 ltk[16] = {0};
    u8 sirk_in[16] = {0};       //in in big_endian
    u8 sirk_out[16] = {0};      //out in big_endian
    extern u8 local_dev_index[];
    int is_master = (connHandle & BLM_CONN_HANDLE);
    u8 conn_idx  = connHandle & CONN_IDX_MASK;
    u8 smp_status_idx = is_master ? 0: (conn_idx - LL_MAX_ACL_CEN_NUM + 1);

    u8 slave_dev_idx = local_dev_index[conn_idx];
    smp_param_peer_t *pBlms_p_peer = (smp_param_peer_t *)&smp_param_peer[smp_status_idx];
    smp_param_save_t smp_param_temp;
    blc_smp_loadBondingInfoByAddr(is_master, slave_dev_idx, pBlms_p_peer->peer_addr_type, pBlms_p_peer->peer_conn_addr, &smp_param_temp);

    swapX(smp_param_temp.local_peer_ltk, ltk, 16);
    swapX(in, sirk_in, 16);

    blt_csis_cryptoSefSdf(sirk_in, ltk, sirk_out);
    swapX(sirk_out, out, 16);
}

bool blc_csis_resolveRSI(const u8 sirk[16], u8 rsi[6])
{
    u8 prand[3] = {0};
    u8 local_hash[3] = {0};

    prand[0] = *(rsi+3);
    prand[1] = *(rsi+4);
    prand[2] = *(rsi+5);

    blt_csis_cryptoSIH(sirk, prand, local_hash);

    if((local_hash[0] == *rsi) && (local_hash[1] == *(rsi+1)) && (local_hash[2] == *(rsi+2))){
        return true;
    }

    return false;
}



