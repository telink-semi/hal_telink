/********************************************************************************************************
 * @file    multi_device.c
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
#include "stack/ble/controller/ble_controller.h"
#include "stack/ble/host/ble_host.h"

#include "multi_device.h"
#include "device_stack.h"

#if (MULTIPLE_LOCAL_DEVICE_ENABLE)

// MLD: multiple local device


_attribute_data_retention_ _attribute_aligned_(4) loc_dev_mng_t mlDevMng; //multiple local device manage

void blc_ll_setMultipleLocalDeviceEnable(multi_dev_en_t enable)
{
    mlDevMng.mldev_en = enable;
}

ble_sts_t blc_ll_setLocalDeviceIndexAndIdentityAddress(loc_dev_idx_t dev_idx, u8 id_adrType, u8 *id_addr)
{
    if (blmsParam.max_slave_num > LOCAL_DEVICE_NUM_MAX) {
        return LL_ERR_INVALID_PARAMETER;
    }

    if (dev_idx >= LOCAL_DEVICE_NUM_MAX) {
        return LL_ERR_INVALID_PARAMETER;
    }

    if (id_adrType != BLE_ADDR_PUBLIC && id_adrType != BLE_ADDR_RANDOM) {
        return LL_ERR_INVALID_PARAMETER;
    }


    mlDevMng.dev_mac[dev_idx].set  = 1;
    mlDevMng.dev_mac[dev_idx].type = id_adrType;
    smemcpy(mlDevMng.dev_mac[dev_idx].address, id_addr, BLE_ADDR_LEN);

    return BLE_SUCCESS;
}

ble_sts_t blc_ll_setCurrentLocalDevice_by_index(loc_dev_idx_t dev_idx)
{
    if (dev_idx >= LOCAL_DEVICE_NUM_MAX) {
        return LL_ERR_INVALID_PARAMETER;
    }


    if (!mlDevMng.dev_mac[dev_idx].set) {
        return LL_ERR_INVALID_PARAMETER;
    }

    bltMac.macAddress_type = mlDevMng.dev_mac[dev_idx].type;
    smemcpy(bltMac.macAddress_use, mlDevMng.dev_mac[dev_idx].address, BLE_ADDR_LEN);

    mlDevMng.cur_dev_idx = dev_idx;

    return BLE_SUCCESS;
}


#endif
