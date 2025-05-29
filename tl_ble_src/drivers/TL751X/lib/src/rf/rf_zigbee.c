/********************************************************************************************************
 * @file    rf_zigbee.c
 *
 * @brief   This is the source file for TL751X
 *
 * @author  Driver Group
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
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include "lib/include/rf/rf_common.h"
#include "lib/include/rf/rf_zigbee.h"

/*********************************************************************************************************************
 *                                         global function implementation                                            *
 *********************************************************************************************************************/

/**
 * @brief     This function serves to  set zigbee_250K  mode of RF.
 * @return    none.
 * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_set_zigbee_250K_mode(void)
{
    //This function interface is not available at this time.
    g_rfmode = RF_MODE_ZIGBEE_250K;
};
