/********************************************************************************************************
 * @file    darwin_fmn.c
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
#include "stack/ble/ble.h"

#include "darwin_fmn.h"
#include "darwin_stack.h"

//Darwin means Apple.

#if (CUSTOM_DARWIN_FMN_ENABLE)
_attribute_ble_data_retention_ custom_darwin_fmn_t custom_darwin_fmn = {0};

void blc_ll_setCustomFMNEnable(u8 en, blc_smp_paringreq_cb_t pr_cb, blc_smp_sec_info_cb_t sir_cb)
{
    custom_darwin_fmn.darwin_fmn_enable = en;
    if (en) {
        custom_darwin_fmn.pair_req_cb     = pr_cb;
        custom_darwin_fmn.sec_info_req_cb = sir_cb;
    } else {
        custom_darwin_fmn.pair_req_cb     = NULL;
        custom_darwin_fmn.sec_info_req_cb = NULL;
    }
}
#endif
