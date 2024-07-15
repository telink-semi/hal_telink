/********************************************************************************************************
 * @file	ext_aes.h
 *
 * @brief	This is the header file for BLE SDK
 *
 * @author	BLE GROUP
 * @date	06,2022
 *
 * @par		Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *			All rights reserved.
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
#ifndef DRIVERS_B92_EXT_DRIVER_DRIVER_LIB_EXT_AES_H_
#define DRIVERS_B92_EXT_DRIVER_DRIVER_LIB_EXT_AES_H_

//#include "common/types.h"


#define HW_AES_CCM_ALG_EN										0

extern unsigned int aes_data_buff[8];


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


void aes_encryption_le(u8* key, u8* plaintext, u8 *encrypted_data);
void aes_encryption_be(u8* key, u8* plaintext, u8 *encrypted_data);

bool aes_resolve_irk_rpa(u8 *key, u8 *addr);

void blt_ll_setAesCcmPara(u8 role, u8 *sk, u8 *iv, u8 aad, u64 enc_pno, u64 dec_pno, u8 lastTxLenFlag);


#endif /* DRIVERS_B92_EXT_DRIVER_DRIVER_LIB_EXT_AES_H_ */
