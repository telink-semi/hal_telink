/********************************************************************************************************
 * @file    hadm.c
 *
 * @brief   This is the header file for TL323X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "lib/include/rf/rf_common.h"
#include "lib/include/pm/pm.h"
#include "compiler.h"

/*********************************************************************************************************************
 *                                         global function implementation                                            *
 *********************************************************************************************************************/

void rf_hadm_frond_end(unsigned char *data_src, int *data_has_amplitude, unsigned int len)
{
    int temp[2048] = {0};
    for (unsigned int i = 0; i < len; i++) {
        temp[i * 2]     = ((data_src[i * 5 + 2] & 0x0f) << 16) + (data_src[i * 5 + 1] << 8) + data_src[i * 5];
        temp[i * 2 + 1] = (data_src[i * 5 + 4] << 12) + (data_src[i * 5 + 3] << 4) + ((data_src[i * 5 + 2] >> 4) & 0x0f);
    }
    for (unsigned int i = 0; i < 2 * len; i++) {
        if (temp[i] > 524288) {
            data_has_amplitude[i] = temp[i] - 1048576;
        } else {
            data_has_amplitude[i] = temp[i];
        }
    }
}

void rf_hadm_restore_reflector_data(unsigned char *data_src, int *data_has_amplitude, unsigned int len)
{
    int temp[2048] = {0};

    for (unsigned int i = 0; i < len; i++) {
        temp[i] = (data_src[i * 2] << 8) + data_src[i * 2 + 1];
    }
    for (unsigned int i = 0; i < 2 * len; i++) {
        if (temp[i] > 524288) {
            data_has_amplitude[i] = temp[i] - 1048576;
        } else {
            data_has_amplitude[i] = temp[i];
        }
    }
}
