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
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "common/types.h"


#include "../../inc/ble_host.h"
#include "../../inc/ble_host_sal.h"
#include "../inc/ble_l2cap.h"
#include "../inc/ble_l2cap_log.h"
#include "inc/ble_smp.h"
#include <stddef.h>
#include "../../store/easyflash/inc/easyflash.h"
#include "../../store/inc/ble_store.h"
#include "drivers.h"

//TODO: comments change cn to en,

#include "inc/ble_smp_store.h"


////////////////////////////////////////////////////////////////////////////////
/// 0. init EasyFlash
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// 1. Defining data structures and global variables
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// 2. Implement key value management functions
////////////////////////////////////////////////////////////////////////////////
#if (SMP_BONDING_MNG_MODE_SELECT == GLOBALE_VAR_MODE)
// Global variables: record the binding status of the master device and the slave device
static bool master_bonding_slots[4] = {false}; // The master device supports up to 4 bonds
static bool slave_bonding_slots[8] = {false};  // The slave device supports up to 8 bonds

// Allocate a unused key value
int allocate_bonding_slot(bool is_master, bool delete_oldest) {
    uint8_t max_slots = is_master ? 4 : 8;
    bool *slots = is_master ? master_bonding_slots : slave_bonding_slots;

    for (int i = 0; i < max_slots; i++) {
        if (!slots[i]) {
            slots[i] = true; // mark it as occupied
            return i; // return the allocated key value
        }
    }

    // All key values are occupied, delete the old binding information
    if (delete_oldest) {
        return delete_oldest_bonding(is_master); // Delete the oldest bonding information
    } else {
        return delete_most_recent_bonding(is_master); // Delete the latest bonding information
    }
}

// release a key value
void release_bonding_slot(int index, bool is_master) {
    if (is_master) {
        if (index >= 0 && index < 4) {
            master_bonding_slots[index] = false; // mark it as empty
        }
    } else {
        if (index >= 0 && index < 8) {
            slave_bonding_slots[index] = false; // mark it as empty
        }
    }
}
#elif (SMP_BONDING_MNG_MODE_SELECT == EFLASH_KEY_VAL_MODE)
// Store the key value state
void save_bonding_slot_status(bool is_master, uint8_t slots) {
    char key[16];
    snprintf(key, sizeof(key), is_master ? "master_slots" : "slave_slots");
    ef_set_env_blob(key, &slots, 1); // Use 1 byte to store the bitmap
}

// read key value status
uint8_t read_bonding_slot_status(bool is_master) {
    char key[16];
    snprintf(key, sizeof(key), is_master ? "master_slots" : "slave_slots");
    uint8_t slots = 0;
    size_t len = 1;
    ef_get_env_blob(key, &slots, len, NULL);
    return slots;
}

// Allocate an unused key value
int allocate_bonding_slot(bool is_master, bool delete_oldest) {
    uint8_t slots = read_bonding_slot_status(is_master);
    uint8_t max_slots = is_master ? 4 : 8;

    for (int i = 0; i < max_slots; i++) {
        if (!(slots & (1 << i))) {
            slots |= (1 << i); // mark it as occupied
            save_bonding_slot_status(is_master, slots);
            return i; // return allocated key value
        }
    }

    // All keys are occupied, delete old bonding information based on the policy
    int slot_to_delete = -1;
    if (delete_oldest) {
        slot_to_delete = delete_oldest_bonding(is_master); // Delete the oldest bonding information
    } else {
        slot_to_delete = delete_most_recent_bonding(is_master); // Delete the latest bonding information
    }

    if (slot_to_delete != -1) {
        slots &= ~(1 << slot_to_delete); // mark it as empty
        save_bonding_slot_status(is_master, slots);
        return slot_to_delete; // return allocated key value
    }

    return slot_to_delete;
}

//release a key value
void release_bonding_slot(int index, bool is_master) {
    uint8_t slots = read_bonding_slot_status(is_master);
    slots &= ~(1 << index); //mark it as empty
    save_bonding_slot_status(is_master, slots);
}
#elif (SMP_BONDING_MNG_MODE_SELECT == DYNAMIC_LOOPUP_MODE)
//  Allocate an unused key value
int allocate_bonding_slot(bool is_master, bool delete_oldest) {
    struct smp_bonding_keys keys;
    uint8_t max_slots = is_master ? 4 : 8;

    for (int i = 0; i < max_slots; i++) {
        char key[16];
        snprintf(key, sizeof(key), is_master ? "master_smp_%d" : "slave_smp_%d", i);
        size_t len = sizeof(struct smp_bonding_keys);
        if (ef_get_env_blob(key, &keys, &len, NULL) != EF_NO_ERR) {
            // the key does not exist, indicating it's empty
            return i; // return empty key
        }
    }

    // All keys are occupied, delete old bonding information based on the policy
    int slot_to_delete = -1;
    if (delete_oldest) {
        slot_to_delete = delete_oldest_bonding(is_master); // Delete the oldest binding information
    } else {
        slot_to_delete = delete_most_recent_bonding(is_master); // Delete the latest binding information
    }

    return slot_to_delete; // return the released key value
}
#else
#error "error SMP bonding mode"
#endif
////////////////////////////////////////////////////////////////////////////////
/// 3. Implement storage and retrieval functions
////////////////////////////////////////////////////////////////////////////////
// Save bonding information
void save_smp_bonding_keys(int index, struct smp_bonding_keys *keys, bool is_master) {
    char key[16];
    if (is_master) {
        snprintf(key, sizeof(key), "master_smp_%d", index); // generate unique key name
    } else {
        snprintf(key, sizeof(key), "slave_smp_%d", index); // generate unique key name
    }

    keys->index = index; // keep Index

    // Store data
    if (ef_set_env_blob(key, keys, sizeof(struct smp_bonding_keys)) == EF_NO_ERR) {
        ef_print("SMP bonding keys saved successfully.\n");
    } else {
        ef_print("Failed to save SMP bonding keys.\n");
    }
}

// Read bonding information
void read_smp_bonding_keys(int index, struct smp_bonding_keys *keys, bool is_master) {
    char key[16];
    if (is_master) {
        snprintf(key, sizeof(key), "master_smp_%d", index); // generate unique key name
    } else {
        snprintf(key, sizeof(key), "slave_smp_%d", index); // generate unique key name
    }

    size_t len = sizeof(struct smp_bonding_keys);
    if (ef_get_env_blob(key, keys, len, NULL) == len) {
        ef_print("SMP bonding keys read successfully.\n");
    } else {
        ef_print("Failed to read SMP bonding keys.\n");
    }
}

// Delete bonding information
void delete_smp_bonding_keys(int index, bool is_master) {
    char key[16];
    if (is_master) {
        snprintf(key, sizeof(key), "master_smp_%d", index); //generate unique key name
    } else {
        snprintf(key, sizeof(key), "slave_smp_%d", index); // generate unique key name
    }

    // Delete data
    if (ef_del_env(key) == EF_NO_ERR) {
        ef_print("SMP bonding keys deleted successfully.\n");
    } else {
        ef_print("Failed to delete SMP bonding keys.\n");
    }
    release_bonding_slot(index, is_master); // release key value
}

////////////////////////////////////////////////////////////////////////////////
/// 4. Handle duplicate pairing bonding
////////////////////////////////////////////////////////////////////////////////
bool is_device_bonded(uint8_t *peer_addr, bool is_master) {
    struct smp_bonding_keys keys;
    uint8_t max_bondings = is_master ? 4 : 8;

    for (uint8_t i = 0; i < max_bondings; i++) {
        read_smp_bonding_keys(i, &keys, is_master);
        if (memcmp(keys.peer_addr, peer_addr, 6) == 0) {
            return true; // device is bonded
        }
    }
    return false; // device is not bonded
}

void handle_duplicate_bonding(uint8_t *peer_addr, bool is_master) {
    struct smp_bonding_keys keys;
    uint8_t max_bondings = is_master ? 4 : 8;

    for (uint8_t i = 0; i < max_bondings; i++) {
        read_smp_bonding_keys(i, &keys, is_master);
        if (memcmp(keys.peer_addr, peer_addr, 6) == 0) {
            delete_smp_bonding_keys(i, is_master); // delete old bonding information
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/// 6. Implement a deletion policy
////////////////////////////////////////////////////////////////////////////////
int delete_most_recent_bonding(bool is_master) { //Delete the most recent pairing information
    struct smp_bonding_keys keys;
    uint8_t max_slots = is_master ? 4 : 8;
    uint32_t max_timestamp = 0;
    int slot_to_delete = -1;

    for (int i = 0; i < max_slots; i++) {
        read_smp_bonding_keys(i, &keys, is_master);
        if (keys.index > max_timestamp) {
            max_timestamp = keys.index;
            slot_to_delete = i;
        }
    }

    if (slot_to_delete != -1) {
        delete_smp_bonding_keys(slot_to_delete, is_master);
    }
    return slot_to_delete;
}

int delete_oldest_bonding(bool is_master) { //Delete the oldest pairing information
    struct smp_bonding_keys keys;
    uint8_t max_slots = is_master ? 4 : 8;
    uint32_t min_index = UINT32_MAX;
    int slot_to_delete = -1;

    for (int i = 0; i < max_slots; i++) {
        read_smp_bonding_keys(i, &keys, is_master);
        if (keys.index < min_index) {
            min_index = keys.index;
            slot_to_delete = i;
        }
    }

    if (slot_to_delete != -1) {
        delete_smp_bonding_keys(slot_to_delete, is_master);
    }
    return slot_to_delete;
}

////////////////////////////////////////////////////////////////////////////////
/// 5. Example: Store and read data
////////////////////////////////////////////////////////////////////////////////
int smp_bond_mode_test_demo(void)
{
    flash_init();

    // Example: Main device bonding
    uint8_t peer_addr[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    struct smp_bonding_keys smp_keys = {
        .peer_addr = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
        .irk = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10},
        .csrk = {0},
        .ltk = {0},
        .rand = {0},
        .ediv = 0x1234,
        .index = 0, // Index
    };

    // Check if already bonded
    if (is_device_bonded(peer_addr, true)) {
        ef_print("Device already bonded. Deleting old bonding info...\n");
        handle_duplicate_bonding(peer_addr, true); // Delete the old pairing information
    }

    // Allocate an unused key (delete the oldest bonding information)
    int index = allocate_bonding_slot(true, true); // Master device, delete the oldest bonding information
    if (index == -1) {
        ef_print("No available bonding slot for master.\n");
        return -1;
    }

    // Store new bonding information
    save_smp_bonding_keys(index, &smp_keys, true);

    // Read bonding information
    struct smp_bonding_keys read_keys;
    read_smp_bonding_keys(index, &read_keys, true);

    // Delete bonding information
    delete_smp_bonding_keys(index, true);

    return 0;
}



int ble_smp_store_keys_read(struct ble_host_conn *conn, struct smp_bonding_keys *keys)
{
    (void)conn;
    (void)keys;

    return 0;
}

int ble_smp_store_keys_write(struct ble_host_conn *conn, struct smp_bonding_keys *keys)
{
    (void)conn;
    (void)keys;

    return 0;
}

int ble_smp_store_keys_delete(struct ble_host_conn *conn)
{
    (void)conn;

    return 0;
}
