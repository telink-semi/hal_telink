/********************************************************************************************************
 * @file    ecc_ll.c
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
#include "algorithm/algorithm.h"


extern void tlk_mem_cpy(void *pd, void *ps, int len);
extern int  tlk_mem_cmp(void * m1, void * m2, int len);


#define smemcpy     tlk_mem_cpy
#define smemcmp     tlk_mem_cmp


#define     ecc_ll_set_rng              hwECC_set_rng
#define     ecc_ll_make_key             hwECC_make_key
#define     ecc_ll_gen_dhkey            hwECC_shared_secret


/*Refer to <<Core 5.2>> Vol 4, Part E, page 2279
  For P-192:
    Private key:    07915f86 918ddc27 005df1d6 cf0c142b 625ed2ef f4a518ff
    Public key (X): 15207009 984421a6 586f9fc3 fe7e4329 d2809ea5 1125f8ed
    Public key (Y): b09d42b8 1bc5bd00 9f79e4b5 9dbbaa85 7fca856f b9f7ea25
    For P-256:
    Private key:    3f49f6d4 a3c55f38 74c9b3e3 d2103f50 4aff607b eb40b799 5899b8a6 cd3c1abd
    Public key (X): 20b003d2 f297be2c 5e2c83a7 e9f9a5b9 eff49111 acf4fddb cc030148 0e359de6
    Public key (Y): dc809c49 652aeb6d 63329abf 5a52155c 766345c2 8fed3024 741c8ed0 1589d28b
 */

const u8 blt_ecc_dbg_priv_key192[24] = { //SKb :Private key of the response device, big--endian
    //Private key
    0x07, 0x91, 0x5f, 0x86, 0x91, 0x8d, 0xdc, 0x27, 0x00, 0x5d, 0xf1, 0xd6,
    0xcf, 0x0c, 0x14, 0x2b, 0x62, 0x5e, 0xd2, 0xef, 0xf4, 0xa5, 0x18, 0xff,
};

const u8 blt_ecc_dbg_pub_key192[48] = { //PKb :Public key of response device, big--endian
    //Public key (X):
    0x15, 0x20, 0x70, 0x09, 0x98, 0x44, 0x21, 0xa6, 0x58, 0x6f, 0x9f, 0xc3,
    0xfe, 0x7e, 0x43, 0x29, 0xd2, 0x80, 0x9e, 0xa5, 0x11, 0x25, 0xf8, 0xed,
    //Public key (Y):
    0xb0, 0x9d, 0x42, 0xb8, 0x1b, 0xc5, 0xbd, 0x00, 0x9f, 0x79, 0xe4, 0xb5,
    0x9d, 0xbb, 0xaa, 0x85, 0x7f, 0xca, 0x85, 0x6f, 0xb9, 0xf7, 0xea, 0x25,
};

/* Refer to <<Core 4.2>> Vol 3. Part H 2.3.5.6.1
 * SMP test dhkey. Only one side (initiator or responder) needs to set SC debug mode in order for debug
 * equipment to be able to determine the LTK and, therefore, be able to monitor the encrypted connection.
 * */
const u8 blt_ecc_dbg_priv_key256[32] = { //SKb :Private key of the response device, big--endian
    //Private key
    0x3f, 0x49, 0xf6, 0xd4, 0xa3, 0xc5, 0x5f, 0x38, 0x74, 0xc9, 0xb3, 0xe3, 0xd2, 0x10, 0x3f, 0x50,
    0x4a, 0xff, 0x60, 0x7b, 0xeb, 0x40, 0xb7, 0x99, 0x58, 0x99, 0xb8, 0xa6, 0xcd, 0x3c, 0x1a, 0xbd,
};

const u8 blt_ecc_dbg_pub_key256[64] = { //PKb :Public key of response device, big--endian
    //Public key (X):
    0x20, 0xb0, 0x03, 0xd2, 0xf2, 0x97, 0xbe, 0x2c, 0x5e, 0x2c, 0x83, 0xa7, 0xe9, 0xf9, 0xa5, 0xb9,
    0xef, 0xf4, 0x91, 0x11, 0xac, 0xf4, 0xfd, 0xdb, 0xcc, 0x03, 0x01, 0x48, 0x0e, 0x35, 0x9d, 0xe6,
    //Public key (Y):
    0xdc, 0x80, 0x9c, 0x49, 0x65, 0x2a, 0xeb, 0x6d, 0x63, 0x32, 0x9a, 0xbf, 0x5a, 0x52, 0x15, 0x5c,
    0x76, 0x63, 0x45, 0xc2, 0x8f, 0xed, 0x30, 0x24, 0x74, 0x1c, 0x8e, 0xd0, 0x15, 0x89, 0xd2, 0x8b,
};


/**
 * @brief       This function is used to provide random number generator for ECC calculation
 * @param[out]  dest: The address where the random number is stored
 * @param[in]   size: Output random number size, unit byte
 * @return      1:  success
 */
static int blt_ecc_gen_rand(unsigned char *dest, unsigned int size)
{
    unsigned int randNums = 0;
    /* if len is odd */
    for (unsigned int i=0; i<size; i++ ) {
        if( (i & 3) == 0 ){
            randNums = rand();
        }

        dest[i] = randNums & 0xff;
        randNums >>=8;
    }

    return 1;
}


/**
 * @brief       This function is used to register the random number function needed for ECC calculation
 * @param       none
 * @return      none
 */
void blt_ecc_init(void)
{
    ecc_ll_set_rng(blt_ecc_gen_rand);
}


/**
 * @brief       This function is used to generate an ECDH public-private key pairs
 * @param[out]  pub[64]:  output ecdh public key, big--endian
 * @param[out]  priv[64]: output ecdh private key, big--endian
 * @param[in]   use_dbg_key: 0: Non-debug key , others: debug key
 * @return      1: success
 *              0: failure
 */
int blt_ecc_gen_key_pair(unsigned char *pub, unsigned char *priv, ecc_curve_t curve, bool use_dbg_key)
{
    //distribute private/public key pairs
    if(use_dbg_key){
        if(curve == ECC_use_secp192r1){ //BT maybe used
            smemcpy(priv, (u8*)(u32)blt_ecc_dbg_priv_key192, 24);
            smemcpy(pub , (u8*)(u32)blt_ecc_dbg_pub_key192, 48);
        }
        else if(curve == ECC_use_secp256r1){ //BLE maybe used
            smemcpy(priv, (u8*)(u32)blt_ecc_dbg_priv_key256, 32);
            smemcpy(pub , (u8*)(u32)blt_ecc_dbg_pub_key256, 64);
        }
        else{
            return 0;
        }
    }
    else{
        u8* pDbg_priv_key = NULL;
        u8  priv_key_size = 0;
        if(curve == ECC_use_secp192r1 || curve == ECC_use_secp256r1){ // BT/BLE maybe used
            priv_key_size = (curve == ECC_use_secp192r1) ? 24 : 32;
            pDbg_priv_key = (curve == ECC_use_secp192r1) ? (u8*)(u32)blt_ecc_dbg_priv_key192 : (u8*)(u32)blt_ecc_dbg_priv_key256;

            do{
                //DBG_C HN4_TOGGLE;  //32M: hw take 45ms
                if(!ecc_ll_make_key(pub, priv, curve)){
                    return 0; //check swECC_make_key() failed
                }
                //DBG_C HN4_TOGGLE;
            /* Make sure generated key isn't debug key. */
            }while (smemcmp(priv, pDbg_priv_key, priv_key_size) == 0);

            //my_dump_str_data(0, "Ecc pub", pub, priv_key_size*2);
            //my_dump_str_data(0, "Ecc priv", priv, priv_key_size);
        }
        else{
            if(!ecc_ll_make_key(pub, priv, curve)){
                return 0; //check swECC_make_key() failed
            }
        }
    }

    return 1;
}


/**
 * @brief       This function is used to calculate DHKEY based on the peer public key and own private key
 * @param[in]   peer_pub_key[64]: peer public key, big--endian
 * @param[in]   own_priv_key[32]: own private key, big--endian
 * @param[out]  out_dhkey[32]: dhkey key, big--endian
 * @return      1:  success
 *              0: failure
 */
int blt_ecc_gen_dhkey(const unsigned char *peer_pub, const unsigned char *own_priv, unsigned char *out_dhkey, ecc_curve_t curve)
{
    //DBG_C HN4_TOGGLE;  //32M: hw take 45ms
    if (!ecc_ll_gen_dhkey(peer_pub, own_priv, out_dhkey, curve)) {
        //my_dump_str_data(0, "ecc_gen_dhkey failure ", 0, 0);
        return 0;
    }
    //DBG_C HN4_TOGGLE;

    return 1;
}




