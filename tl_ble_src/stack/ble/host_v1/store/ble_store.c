/********************************************************************************************************
 * @file    ble_store.c
 *
 * @brief   This is the source file for TLSR/TL
 *
 * @author  Bluetooth Group
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
#include <stddef.h>
#include "easyflash/inc/easyflash.h"

#include "inc/ble_store.h"

void ble_store_init(void)
{
    easyflash_init();
}

void ble_store_deinit(void)
{

}

void ble_store_read(const char *key, const void *value_buf, size_t buf_len)
{
    (void)key;
    (void)value_buf;
    (void)buf_len;

}

void ble_store_write(const char *key, const void *value_buf, size_t buf_len)
{
    (void)key;
    (void)value_buf;
    (void)buf_len;

}

void ble_store_delete(const char *key)
{
    (void)key;
}


