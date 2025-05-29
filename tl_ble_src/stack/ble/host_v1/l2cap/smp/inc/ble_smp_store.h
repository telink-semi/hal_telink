/********************************************************************************************************
 * @file    ble_smp_store.h
 *
 * @brief   This is the header file for TLSR/TL
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
#pragma once


// SMP encryption binding keys structure, TODO: comments change cn to en,
struct smp_bonding_keys
{
    uint8_t peer_addr[6];  // peer device address
    uint8_t irk[16];       // Identity Resolving Key (IRK)
    uint8_t csrk[16];      // Connection Signature Resolving Key (CSRK)
    uint8_t ltk[16];       // Long Term Key (LTK)
    uint8_t rand[8];       // Random number
    uint16_t ediv;         // Encrypted Diversifier
    uint32_t index;        // index ID number
};


#define GLOBALE_VAR_MODE        0 //Managing keys with global variables
#define EFLASH_KEY_VAL_MODE     1 //Using EasyFlash to store key-value state
#define DYNAMIC_LOOPUP_MODE     2 //dynamic lookup for free keys

#define SMP_BONDING_MNG_MODE_SELECT     EFLASH_KEY_VAL_MODE


// init EasyFlash
void flash_init(void);

// Allocate an unused key
int allocate_bonding_slot(bool is_master, bool delete_oldest);

// Store bonding information
void save_smp_bonding_keys(int index, struct smp_bonding_keys *keys, bool is_master);

// Read bonding information
void read_smp_bonding_keys(int index, struct smp_bonding_keys *keys, bool is_master);

// Delete bonding information
void delete_smp_bonding_keys(int index, bool is_master);

// Delete the most recent pairing information
int delete_most_recent_bonding(bool is_master);

// Delete the oldest pairing information
int delete_oldest_bonding(bool is_master);

int ble_smp_store_keys_read(struct ble_host_conn *conn, struct smp_bonding_keys *keys);
int ble_smp_store_keys_write(struct ble_host_conn *conn, struct smp_bonding_keys *keys);
int ble_smp_store_keys_delete(struct ble_host_conn *conn);



