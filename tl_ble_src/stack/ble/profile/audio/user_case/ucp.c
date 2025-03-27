/********************************************************************************************************
 * @file    ucp.c
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

#include "ucp.h"
#include "ucp_internal.h"



/***********************************************************************
 *  Refer to <<TMAP_v1.0>>, Page14: Mapping of TMAP roles to CAP roles
 * *------------*-------------------------------------------*
 * | TMAP Roles |               CAP Roles                   |
 * |            |-----------*---------------*---------------|
 * |            | Initiator |   Acceptor    |   Commander   |
 * |------------|-----------|---------------|---------------|
 * |        CG  |       M   |           X   |           M   |
 * *------------|-----------|---------------|---------------|
 * |        CT  |       X   |           M   |           O   |
 * *------------|-----------|---------------|---------------|
 * |        UMS |       M   |           X   |           M   |
 * *------------|-----------|---------------|---------------|
 * |        UMR |       X   |           M   |           O   |
 * *------------|-----------|---------------|---------------|
 * |        BMS |       M   |           X   |           O   |
 * *------------|-----------|---------------|---------------|
 * |        BMR |       X   |           M   |           O   |
 * *------------*-----------*---------------*---------------*
 ***********************************************************************/

