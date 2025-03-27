/********************************************************************************************************
 * @file    phy_2.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "stack/ble/controller/ble_controller.h"






_attribute_ram_code_ void blt_ll_phy_param_reset(void)
{
    bltPHYs.cur_llPhy = BLE_PHY_1M;

    bltPHYs.tx_stl_adv = TX_STL_ADV_SET_1M;
    bltPHYs.tx_stl_tifs = TX_STL_TIFS_SET_1M;
    bltPHYs.own_oneByte_us = 8;
    bltPHYs.peer_oneByte_us = 8;
    bltPHYs.TIFS_offset_us = 190 - TX_STL_TIFS_REAL_1M - HW_DELAY_1M;
    bltPHYs.prmb_ac_us = 40 + AD_CONVERT_DLY_1M;  //timing: 5(preamble 1B + access_code 4B) * 8 = 40
}
