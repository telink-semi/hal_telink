/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/
#ifndef EXT_AES_H_
#define EXT_AES_H_


#include "types.h"

#define	CV_LLBT_BASE				(0x160000)

#define reg_rwbtcntl				REG_ADDR32(CV_LLBT_BASE)

enum{

	FLD_NWINSIZE					= BIT_RNG(0,5),
	FLD_RWBT_RSVD6_7				= BIT_RNG(6,7),

	FLD_RWBTEN              =   BIT(8),
	FLD_CX_DNABORT      	=   BIT(9),
	FLD_CX_RXBSYENA     	=   BIT(10),
	FLD_CX_TXBSYENA			=   BIT(11),
	FLD_SEQNDSB				=   BIT(12),
	FLD_ARQNDSB     		=   BIT(13),
	FLD_FLOWDSB				=   BIT(14),
	FLD_HOPDSB				=   BIT(15),

	FLD_WHITDSB             =   BIT(16),
	FLD_CRCDSB      		=   BIT(17),
	FLD_CRYPTDSB     		=   BIT(18),
	FLD_LMPFLOWDSB			=   BIT(19),
	FLD_SNIFF_ABORT			=   BIT(20),
	FLD_PAGEINQ_ABORT    	=   BIT(21),
	FLD_RFTEST_ABORT		=   BIT(22),
	FLD_SCAN_ABORT			=   BIT(23),


	FLD_RWBT_RSVD24_25			=   BIT_RNG(24,25),
	FLD_CRYPT_SOFT_RST			=	BIT(26),   /**HW AES_CMM module reset*/
	FLD_SWINT_REQ               =   BIT(27),
	FLD_RADIOCNTL_SOFT_RST      =   BIT(28),
	FLD_REG_SOFT_RST     		=   BIT(29),
	FLD_MASTER_TGSOFT_RST		=   BIT(30),
	FLD_MASTER_SOFT_RST			=   BIT(31),

};

/**
 * @brief       this function is used to encrypt the plaintext
 * @param[in]   *key - aes key: 128 bit key for the encryption of the data, little--endian.
 * @param[in]   *plaintext - 128 bit data block that is requested to be encrypted, little--endian.
 * @param[out]  *result - 128 bit encrypted data block, little--endian.
 * @return      none.
 */
void aes_encryption_le(u8* key, u8* plaintext, u8 *encrypted_data);

/**
 * @brief       this function is used to encrypt the plaintext by hw aes
 * @param[in]   *key - aes key: 128 bit key for the encryption of the data, big--endian.
 * @param[in]   *plaintext - 128 bit data block that is requested to be encrypted, big--endian.
 * @param[out]  *encrypted_data - 128 bit encrypted data block, big--endian.
 * @return      none.
 */
void aes_encryption_be(u8* key, u8* plaintext, u8 *encrypted_data);

/**
 * @brief       this function is used to resolve address by irk
 * @param[in]   *key - irk key: 128 bit key for the encryption of the data, little--endian.
 * @param[in]   *addr - 48-bit the bluetooth address, little--endian.
 * @return      1: TRUE: Bluetooth address resolution succeeded
 *              0: FALSE: bluetooth address resolution failed.
 */
bool aes_resolve_irk_rpa(u8 *key, u8 *addr);

void blt_ll_setAesCcmPara(u8 role, u8 *sk, u8 *iv, u8 aad, u64 enc_pno, u64 dec_pno, u8 lastTxLenFlag);
#endif /* EXT_AES_H_ */
