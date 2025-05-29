/********************************************************************************************************
 * @file    smp_storage.c
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
#include "stack/ble/host/ble_host.h"
#include "stack/ble/controller/ble_controller.h"

#if ((!defined(HOST_V2_ENABLE)))
_attribute_ble_data_retention_ static int SMP_PARAM_NV_ADDR_START = 0xFA000;
#else
_attribute_ble_data_retention_ static int SMP_PARAM_NV_ADDR_START = 0x3EC000;
#endif

_attribute_ble_data_retention_ int SMP_PARAM_NV_MAX_LEN = FLASH_SMP_PAIRING_MAX_SIZE; //2*4096;//default 8K

_attribute_ble_data_retention_ static int FLASH_SMP_MARK_OFFSET = 0x1FF0;             //default: 2*4096 - 16 =  1FF0

#define SMP_PARAM_NV_SEC_ADDR_START (SMP_PARAM_NV_ADDR_START + SMP_PARAM_NV_MAX_LEN)

_attribute_ble_data_retention_ static u32 smp_param_current_start_addr = 0; //current use sector

_attribute_ble_data_retention_ static u32 smp_param_next_start_addr = 0;    //next use sector

_attribute_ble_data_retention_ int smp_bond_device_flash_cfg_idx;           //new device info stored flash idx

_attribute_ble_data_retention_ smp_bond_device_t smpMStblBondDevice;


#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
_attribute_ble_data_retention_ unsigned char smp_flash_unlock = 0;
#endif

#if (CUSTOM_SMP_STORAGE)
_attribute_data_retention_     _attribute_aligned_(4) smp_info_custom_save_callback_t smp_info_custom_save_cb = NULL;
_attribute_data_retention_     _attribute_aligned_(4) smp_info_custom_load_callback_t smp_info_custom_load_cb = NULL;
_attribute_data_retention_ u32 smp_custom_enable                                                              = 0;
#endif

//It was originally considered to be placed in smp_stack.h, but smp_param_save_t is defined in
//smp_storage.h, which saves trouble and directly does not declare to the external header file.
int blt_smp_saveBondingInfoToFlash(u8 isCentral, u8 perDevIdx, smp_param_save_t *save_param);


flash_op_sts_t smp_write_flash_page(u32 addr, u32 len, u8 *buf);
bool           smp_erase_flash_area(u32 addr, int size_byte);

bool smp_erase_flash_area(u32 addr, int size_byte)
{
    u32 sector_head, sector_mid, sector_tail;

    while (size_byte > 0) {
        int i;
        for (i = 0; i < 3; i++) {
            flash_erase_sector(addr);

            flash_read_page(addr, 4, (u8 *)&sector_head);
            flash_read_page(addr + 2048, 4, (u8 *)&sector_mid);
            flash_read_page(addr + 4092, 4, (u8 *)&sector_tail);

            if (sector_head == U32_MAX && sector_mid == U32_MAX && sector_tail == U32_MAX) {
                break;
            }
        }

        if (i >= 3) {        //early return
            return 0;        //False
        }

        size_byte -= 0x1000; //4K sector 0x1000 = 4096 Byte
        addr += 0x1000;
    }

    return 1;                //True
}

//NOTE: return 0 represent SUCCESS !!!!
flash_op_sts_t smp_write_flash_page(u32 addr, u32 len, u8 *buf)
{
    u8 check_buf[SMP_PARAM_NV_UNIT];

    int i;
    for (i = 0; i < 3; i++) {
        flash_write_page(addr, len, buf);

        flash_read_page(addr, len, check_buf);

        if (!memcmp(buf, check_buf, len)) {
            break;
        }
    }

    if (i >= 3) {
        return FLASH_OP_FAIL;    //Fail
    } else {
        return FLASH_OP_SUCCESS; //Success
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Operation Type 1. Init
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
    void
    blt_smp_initBondingInfoFromFlash(void)
{
    // 1. init current start addr
    // sector mark, solve power off at clean flash problem
    u8 sector_mark0;
    u8 sector_mark1;

    flash_read_page(SMP_PARAM_NV_ADDR_START + FLASH_SMP_MARK_OFFSET, 1, (u8 *)&sector_mark0);
    flash_read_page(SMP_PARAM_NV_SEC_ADDR_START + FLASH_SMP_MARK_OFFSET, 1, (u8 *)&sector_mark1);
    
    u8 invalid_sector_mark;

    if (sector_mark0 == U8_MAX && sector_mark1 == U8_MAX) {         //first power on, choose area 0, write mark
        smp_param_current_start_addr = SMP_PARAM_NV_ADDR_START;     //kite_vulture-0x78000;eagle-0xFA000: use area 0
        smp_param_next_start_addr    = SMP_PARAM_NV_SEC_ADDR_START; //kite_vulture-0x7a000;eagle-0xFC000: use area 1

#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
        if (flash_prot_op_cb) {
            flash_prot_op_cb(FLASH_OP_EVT_STACK_SMP_SELECT_PAIRING_INFO_AREA_BEGIN,
                             smp_param_current_start_addr + FLASH_SMP_MARK_OFFSET,
                             smp_param_current_start_addr + FLASH_SMP_MARK_OFFSET + 1);
        }
#endif

        u8 smp_flg = FLAG_SMP_SECTOR_USE;
        smp_write_flash_page(smp_param_current_start_addr + FLASH_SMP_MARK_OFFSET, 1, (u8 *)&smp_flg);

#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
        if (flash_prot_op_cb) {
            flash_prot_op_cb(FLASH_OP_EVT_STACK_SMP_SELECT_PAIRING_INFO_AREA_END,
                             smp_param_current_start_addr + FLASH_SMP_MARK_OFFSET,
                             smp_param_current_start_addr + FLASH_SMP_MARK_OFFSET + 1);
        }
#endif
    } else {
        if (sector_mark0 == FLAG_SMP_SECTOR_USE) {
            smp_param_current_start_addr = SMP_PARAM_NV_ADDR_START; //use area 0
            smp_param_next_start_addr    = SMP_PARAM_NV_SEC_ADDR_START;

            invalid_sector_mark = sector_mark1;
        } else {
            smp_param_current_start_addr = SMP_PARAM_NV_SEC_ADDR_START; //use area 1
            smp_param_next_start_addr    = SMP_PARAM_NV_ADDR_START;

            invalid_sector_mark = sector_mark0;
        }

        //erase unused area
        u32 sector_head;
        flash_read_page(smp_param_next_start_addr, 4, (u8 *)&sector_head);

        if (invalid_sector_mark != U8_MAX || sector_head != U32_MAX) {
/* special: use select paring info area event */
#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
            if (flash_prot_op_cb) {
                flash_prot_op_cb(FLASH_OP_EVT_STACK_SMP_SELECT_PAIRING_INFO_AREA_BEGIN,
                                 smp_param_next_start_addr,
                                 smp_param_next_start_addr + SMP_PARAM_NV_MAX_LEN);
            }
#endif

            smp_erase_flash_area(smp_param_next_start_addr, SMP_PARAM_NV_MAX_LEN); //Attention:  can not use smp_erase_flash_sector

/* special: use select paring info area event */
#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
            if (flash_prot_op_cb) {
                flash_prot_op_cb(FLASH_OP_EVT_STACK_SMP_SELECT_PAIRING_INFO_AREA_END,
                                 smp_param_next_start_addr,
                                 smp_param_next_start_addr + SMP_PARAM_NV_MAX_LEN);
            }
#endif
        }
    }

    //2. load bonding device info from flash to sram(smpMStblBondDevice)
    u16 mark;
    u8  flag, role_dev_idx;
    u32 current_flash_adr;

    for (smp_bond_device_flash_cfg_idx = 0; smp_bond_device_flash_cfg_idx < FLASH_SMP_MARK_OFFSET;
         smp_bond_device_flash_cfg_idx += SMP_PARAM_NV_UNIT) {
        current_flash_adr = smp_param_current_start_addr + smp_bond_device_flash_cfg_idx;
        flash_read_page(current_flash_adr, 2, (u8 *)&mark);

        flag         = U16_LO(mark);
        role_dev_idx = U16_HI(mark);

        if (flag == U8_MAX) {
            break;
        } else if ((flag & FLAG_SMP_PARAM_MASK) == FLAG_SMP_PARAM_VALID)                                                             //data storage OK
        {
            if (role_dev_idx & FLAG_CONN_ROLE_MASTER) {                                                                              //master
                if (smpMStblBondDevice.master_cur_bondNum < smpMng.master_max_bondNum) {
                    smpMStblBondDevice.master_bond_flash_idx[smpMStblBondDevice.master_cur_bondNum] = smp_bond_device_flash_cfg_idx; //[!important]save flash address offset
                    smpMStblBondDevice.master_cur_bondNum++;
                } else {
#if (DBG_SMP_ERR_EN)                                                                                                                 //Can be removed after debugging
                    SMP_ERR_DEBUG(0x66000033);
#endif
                }
            } else { //slave
                u8 local_dev_index = role_dev_idx & MSK_SLAVE_DEV_IDX;
                if (local_dev_index > LOCAL_DEVICE_NUM_MAX) {
#if (DBG_SMP_ERR_EN) //Can be removed after debugging
                    SMP_ERR_DEBUG(0x66000044);
#endif
                }

                if (smpMStblBondDevice.slave_cur_bondNum[local_dev_index] < smpMng.slave_max_bondNum) {
                    smpMStblBondDevice.slave_bond_flash_idx[local_dev_index][smpMStblBondDevice.slave_cur_bondNum[local_dev_index]] = smp_bond_device_flash_cfg_idx;
                    smpMStblBondDevice.slave_cur_bondNum[local_dev_index]++;
                } else {
#if (DBG_SMP_ERR_EN) //Can be removed after debugging
                    SMP_ERR_DEBUG(0x66000055);
#endif
                }
            }
        }
    }

    smp_bond_device_flash_cfg_idx -= SMP_PARAM_NV_UNIT;

    //3. too many device in sector,   clean
    blt_smp_cleanBondingInfoStorage();
}

u16 blc_smp_param_getCurrentBondingDeviceNumber(u8 isCentral, u8 perDevIdx)
{
    if (isCentral) { //master
        return smpMStblBondDevice.master_cur_bondNum;
    } else {
/* if multiple local device function not enabled, SDK code will change it to 0 automatically to avoid error. */
#if (MULTIPLE_LOCAL_DEVICE_ENABLE)
        if (!mlDevMng.mldev_en) {
            perDevIdx = 0;
        }
#else
        perDevIdx = 0;
#endif

        if (perDevIdx >= LOCAL_DEVICE_NUM_MAX) {
            return 0; //parameter invalid
        }

        return smpMStblBondDevice.slave_cur_bondNum[perDevIdx];
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Operation Type 2. Search
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

u32 blt_smp_searchBondingDevice_by_PeerMacAddr(u8 isMasterRole, u8 perDevIdx, u8 peer_addr_type, u8 *peer_addr)
{
    u32 flash_addr = 0;

    int  curBondNum   = 0;
    u32 *pFlashOffset = 0;

    if (isMasterRole) {
        pFlashOffset = (u32 *)&smpMStblBondDevice.master_bond_flash_idx[0];
        curBondNum   = smpMStblBondDevice.master_cur_bondNum;
    } else {
        pFlashOffset = (u32 *)&smpMStblBondDevice.slave_bond_flash_idx[perDevIdx][0];
        curBondNum   = smpMStblBondDevice.slave_cur_bondNum[perDevIdx];
    }

    for (int i = 0; i < curBondNum; i++) {
        flash_addr = smp_param_current_start_addr + pFlashOffset[i];

        if (IS_RESOLVABLE_PRIVATE_ADDR(peer_addr_type, peer_addr)) {
            u8 peerIrk[16] = {0};
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_irk), 16, peerIrk);

            if (smp_quickResolvPrivateAddr((u8 *)peerIrk, peer_addr)) {
                return flash_addr; //return flash address, it is not flash address offset
            }
        } else {
            u8 temp_addr_type = 0;
            u8 temp_addr[6]   = {0};
            u8 read_buffer[7];
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_addr_type), 7, read_buffer);
            temp_addr_type = read_buffer[0];
            smemcpy((u8 *)temp_addr, (u8 *)(read_buffer + 1), 6);

            if (temp_addr_type == peer_addr_type && !memcmp(temp_addr, peer_addr, 6)) {
                return flash_addr; //return flash address, it is not flash address offset
            }
        }
    }
    return 0;
}

/***********************************************************************************************
 This API is for master only, called in ADV report event, to search if current slave device is
    already paired with master
 ***********************************************************************************************/
u32 blc_smp_searchBondingPeripheralDevice_by_PeerMacAddress(u8 peer_addr_type, u8 *peer_addr)
{
    u32 flash_addr = 0;

    for (int i = 0; i < smpMStblBondDevice.master_cur_bondNum; i++) {
        flash_addr = smp_param_current_start_addr + smpMStblBondDevice.master_bond_flash_idx[i];

        if (peer_addr_type >= 2) {
            /* in the "for" loop, peer_addr_type should not be clean to 0 in the first time.
             * just use (peer_addr_type & 0x01) for judge.
             */
            //peer_addr_type &= 0x01;
            u8 read_buffer[7];
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_id_adrType), 7, read_buffer);
            if (read_buffer[0] == (peer_addr_type & 0x01) && !memcmp(read_buffer + 1, peer_addr, 6)) {
                return flash_addr; //return flash address, it is not flash address offset
            }
        } else if (IS_RESOLVABLE_PRIVATE_ADDR(peer_addr_type, peer_addr)) {
            u8 peerIrk[16] = {0};
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_irk), 16, peerIrk);

            if (smp_quickResolvPrivateAddr(peerIrk, peer_addr)) {
                return flash_addr; //return flash address, it is not flash address offset
            }
        } else {
            u8 temp_addr_type = 0;
            u8 temp_addr[6]   = {0};
            u8 read_buffer[7];
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_addr_type), 7, read_buffer);
            temp_addr_type = read_buffer[0];
            smemcpy((u8 *)temp_addr, (u8 *)(read_buffer + 1), 6);

            if (temp_addr_type == peer_addr_type && !memcmp(temp_addr, peer_addr, 6)) {
                return flash_addr; //return flash address, it is not flash address offset
            }
        }
    }


    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Operation Type 3. Get
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
u32 blc_smp_getBondingInfoCurStartAddr(void)
{
    return smp_param_current_start_addr;
}

u8 blt_smp_getBondingFlag_by_FlashAddr(u32 flash_addr)
{
    u8 bondFlag;
    flash_read_page(flash_addr, 1, &bondFlag);
    return bondFlag;
}

u16 blt_smp_getBondingIndex_by_FlashAddr(u8 isCentral, u8 perDevIdx, u32 flash_addr)
{
    int  CurBondNum   = 0;
    u32 *pFlashOffset = 0;

    u32 temp_flash_addr = 0;

    if (isCentral) {
        CurBondNum   = smpMStblBondDevice.master_cur_bondNum;
        pFlashOffset = (u32 *)&smpMStblBondDevice.master_bond_flash_idx[0];
    } else {
        CurBondNum   = smpMStblBondDevice.slave_cur_bondNum[perDevIdx];
        pFlashOffset = (u32 *)&smpMStblBondDevice.slave_bond_flash_idx[perDevIdx][0];
    }

    for (int i = 0; i < CurBondNum; i++) {
        temp_flash_addr = smp_param_current_start_addr + pFlashOffset[i];
        if (temp_flash_addr == flash_addr) {
            return i;
        }
    }

    return ADDR_NOT_BONDED;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Operation Type 4. Load
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


u32 bls_smp_loadLTK_by_EdivRand(u8 perDevIdx, u16 ediv, u8 *random, u8 *ltk)
{
    u32  flash_addr   = 0;
    u16   CurBondNum   = 0;
    u32 *pFlashOffset = 0;

    CurBondNum   = smpMStblBondDevice.slave_cur_bondNum[perDevIdx];
    pFlashOffset = (u32 *)&smpMStblBondDevice.slave_bond_flash_idx[perDevIdx][0];

    //generate random value for EDIV,RAND
    for (int i = 0; i < CurBondNum; i++) {
        flash_addr = smp_param_current_start_addr + pFlashOffset[i];

        u16 load_ediv;
        flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, ediv), 2, (u8 *)&load_ediv);
        u8 load_rand[8];
        flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, random), 8, load_rand);

        if ((ediv == load_ediv) && !memcmp(random, load_rand, 8)) {
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, local_peer_ltk), 16, ltk);

            tlkapi_send_string_data((stkLog_mask & STK_LOG_SMP_LTK), "[SMP][LTK] ", ltk, 16);

        #if defined(GOOGLE_CS_INIT_ROLE_EN) && (GOOGLE_CS_INIT_ROLE_EN == 1)
            extern void app_parse_printf(const char *format, ...);
            app_parse_printf("[SMP][LTK] : %s", hex_to_str(ltk, 16));
        #endif

            return flash_addr; //find
        }
    }

    return 0; //not find
}

u32 blc_smp_loadBondingInfoByAddr(u8 isCentral, u8 perDevIdx, u8 addr_type, u8 *addr, smp_param_save_t *smp_param_load)
{
    u32  flash_addr   = 0;
    u16  CurBondNum   = 0;
    u32 *pFlashOffset = 0;

    if (isCentral) {
        CurBondNum   = smpMStblBondDevice.master_cur_bondNum;
        pFlashOffset = (u32 *)&smpMStblBondDevice.master_bond_flash_idx[0];
    } else {
/* if multiple local device function not enabled, SDK code will change it to 0 automatically to avoid error. */
#if (MULTIPLE_LOCAL_DEVICE_ENABLE)
        if (!mlDevMng.mldev_en) {
            perDevIdx = 0;
        }
#else
        perDevIdx = 0;
#endif

        if (perDevIdx >= LOCAL_DEVICE_NUM_MAX) {
            return 0; //parameter invalid
        }

        CurBondNum   = smpMStblBondDevice.slave_cur_bondNum[perDevIdx];
        pFlashOffset = (u32 *)&smpMStblBondDevice.slave_bond_flash_idx[perDevIdx][0];
    }

    int device_match = 0;

    for (int i = 0; i < CurBondNum; i++) {
        flash_addr = smp_param_current_start_addr + pFlashOffset[i];

        if (addr_type >= 2) {
            /* in the "for" loop, peer_addr_type should not be clean to 0 in the first time.
             * just use (peer_addr_type & 0x01) for judge.
             */
            //addr_type &= 0x01;
            u8 read_buffer[7];
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_id_adrType), 7, read_buffer);
            if (read_buffer[0] == (addr_type & 0x01) && !memcmp(read_buffer + 1, addr, 6)) {
                device_match = 1;
            }
        } else if (IS_RESOLVABLE_PRIVATE_ADDR(addr_type, addr)) {
            u8 peerIrk[16] = {0};
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_irk), 16, peerIrk);

            if (smp_quickResolvPrivateAddr(peerIrk, addr)) {
                device_match = 1;
            }
        } else {
            u8 temp_addr_type = 0;
            u8 temp_addr[6]   = {0};
            u8 read_buffer[7];
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_addr_type), 7, read_buffer);
            temp_addr_type = read_buffer[0];
            smemcpy((u8 *)temp_addr, (u8 *)(read_buffer + 1), 6);

            if (temp_addr_type == addr_type && !memcmp(temp_addr, addr, 6)) {
                device_match = 1;
            }
        }

        if (device_match && smp_param_load) {
            flash_read_page(flash_addr, sizeof(smp_param_save_t), (u8 *)smp_param_load);
            return flash_addr;
        }
    }

    return 0; //not find
}

/*
 * Used for load smp parameter base on index
 */
u32 blc_smp_loadBondingInfoFromFlashByIndex(u8 isCentral, u8 perDevIdx, u16 index, smp_param_save_t *smp_param_load)
{
    u32  flash_addr   = 0;
    u16  CurBondNum   = 0;
    u32 *pFlashOffset = 0;

    if (isCentral) {
        CurBondNum   = smpMStblBondDevice.master_cur_bondNum;
        pFlashOffset = (u32 *)&smpMStblBondDevice.master_bond_flash_idx[0];
    } else {
/* if multiple local device function not enabled, SDK code will change it to 0 automatically to avoid error. */
#if (MULTIPLE_LOCAL_DEVICE_ENABLE)
        if (!mlDevMng.mldev_en) {
            perDevIdx = 0;
        }
#else
        perDevIdx = 0;
#endif

        CurBondNum   = smpMStblBondDevice.slave_cur_bondNum[perDevIdx];
        pFlashOffset = (u32 *)&smpMStblBondDevice.slave_bond_flash_idx[perDevIdx][0];
    }

    if (index < CurBondNum) {
        flash_addr = smp_param_current_start_addr + pFlashOffset[index];
        flash_read_page(flash_addr, sizeof(smp_param_save_t), (u8 *)smp_param_load);

        return flash_addr;
    } else {
        return 0; //load bonding information failed
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Operation Type 5. Delete
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//GaoQiu New
void blc_smp_eraseAllBondingInfo(void)
{
#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb) {
        flash_prot_op_cb(FLASH_OP_EVT_STACK_SMP_DELETE_DEVICE_BEGIN,
                         SMP_PARAM_NV_ADDR_START,
                         SMP_PARAM_NV_ADDR_START + 2 * SMP_PARAM_NV_MAX_LEN);
    }
#endif

    smp_erase_flash_area(SMP_PARAM_NV_ADDR_START, SMP_PARAM_NV_MAX_LEN);
    smp_erase_flash_area(SMP_PARAM_NV_SEC_ADDR_START, SMP_PARAM_NV_MAX_LEN);

    smp_param_current_start_addr = SMP_PARAM_NV_ADDR_START; //use sector 0
    smp_param_next_start_addr    = SMP_PARAM_NV_SEC_ADDR_START;

    u8 flag = FLAG_SMP_SECTOR_USE;
    smp_write_flash_page(smp_param_current_start_addr + FLASH_SMP_MARK_OFFSET, 1, (u8 *)&flag);

    smp_bond_device_flash_cfg_idx = -SMP_PARAM_NV_UNIT;

    memset(&smpMStblBondDevice, 0, sizeof(smp_bond_device_t));

#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb) {
        flash_prot_op_cb(FLASH_OP_EVT_STACK_SMP_DELETE_DEVICE_END,
                         SMP_PARAM_NV_ADDR_START,
                         SMP_PARAM_NV_ADDR_START + 2 * SMP_PARAM_NV_MAX_LEN);
    }
#endif
}

bool blc_smp_isBondingInfoStorageLowAlarmed(void)
{
    return (smp_bond_device_flash_cfg_idx >= SMP_PARAM_CLEAN_INDEX_ALARM_LOW);
}

void blt_delete_one_device(u32 flash_addr)
{
#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb && !smp_flash_unlock) {
        flash_prot_op_cb(FLASH_OP_EVT_STACK_SMP_DELETE_DEVICE_BEGIN, flash_addr, flash_addr + 1);
    }
#endif

    u8 flag = FLAG_SMP_PARAM_SAVE_ERASE;
    smp_write_flash_page(flash_addr, 1, &flag);

#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb && !smp_flash_unlock) {
        flash_prot_op_cb(FLASH_OP_EVT_STACK_SMP_DELETE_DEVICE_END, flash_addr, flash_addr + 1);
    }
#endif
}

int blt_smp_deleteBondingInfo_by_Index(u8 isCentral, u8 perDevIdx, u16 index, bool smp_del_cb)
{
    u32  flash_addr   = 0;
    u16 *pCurBondNum  = 0;
    u32 *pFlashOffset = 0;

    if (isCentral) {
        pCurBondNum  = (u16 *)&smpMStblBondDevice.master_cur_bondNum;
        pFlashOffset = (u32 *)&smpMStblBondDevice.master_bond_flash_idx[0];
    } else {
        pCurBondNum  = (u16 *)&smpMStblBondDevice.slave_cur_bondNum[perDevIdx];
        pFlashOffset = (u32 *)&smpMStblBondDevice.slave_bond_flash_idx[perDevIdx][0];
    }

    //clean device
    if (index < *pCurBondNum) {
        flash_addr = smp_param_current_start_addr + pFlashOffset[index];
        //Audio store concerned
        if (smp_delete_cb && (smp_del_cb == true)) {
            smp_delete_cb(isCentral, flash_addr);
        }

        blt_delete_one_device(flash_addr);

        for (int i = index; i < ((*pCurBondNum) - 1); i++) {
            pFlashOffset[i] = pFlashOffset[i + 1];
        }
        (*pCurBondNum)--;
    } else {
#if (DBG_SMP_ERR_EN)  //Can be removed after debugging
        SMP_ERR_DEBUG(0x66000066);
#endif
        return FALSE; //parameter invalid
    }


    return true;
}

int blt_smp_deleteBondingInfo_by_PeerMacAddress(u8 isCentral, u8 perDevIdx, u8 peer_addr_type, u8 *peer_addr)
{
    u32 flash_addr = 0;

    u16 *pCurBondNum  = 0;
    u32 *pFlashOffset = 0;

    if (isCentral) {
        pCurBondNum  = (u16 *)&smpMStblBondDevice.master_cur_bondNum;
        pFlashOffset = (u32 *)&smpMStblBondDevice.master_bond_flash_idx[0];
    } else {
        if (perDevIdx >= LOCAL_DEVICE_NUM_MAX) {
            return 0; //parameter invalid
        }
        pCurBondNum  = (u16 *)&smpMStblBondDevice.slave_cur_bondNum[perDevIdx];
        pFlashOffset = (u32 *)&smpMStblBondDevice.slave_bond_flash_idx[perDevIdx][0];
    }

    for (int i = 0; i < (*pCurBondNum); i++) {
        flash_addr = smp_param_current_start_addr + pFlashOffset[i];

        int device_match = 0;

        if (peer_addr_type >= 2) {
            peer_addr_type &= 0x01;

            u8 addr_type = 0;
            u8 addr[6]   = {0};
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_id_adrType), 1, &addr_type);
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_id_addr), 6, addr);

            if (addr_type == peer_addr_type && !memcmp(addr, peer_addr, 6)) {
                device_match = 1;
            }
        } else if (IS_RESOLVABLE_PRIVATE_ADDR(peer_addr_type, peer_addr)) {
            u8 peerIrk[16] = {0};
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_irk), 16, peerIrk);

            if (smp_quickResolvPrivateAddr((u8 *)peerIrk, peer_addr)) {
                device_match = 1;
            }
        } else {
            u8 addr_type = 0;
            u8 addr[6]   = {0};
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_addr_type), 1, &addr_type);
            flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_addr), 6, addr);

            if (addr_type == peer_addr_type && !memcmp(addr, peer_addr, 6)) {
                device_match = 1;
            }
        }

        if (device_match) {
            //Audio store concerned
            if (smp_delete_cb) {
                smp_delete_cb(isCentral, flash_addr);
            }
            //clean device
            blt_delete_one_device(flash_addr);

            for (int j = i; j < ((*pCurBondNum) - 1); j++) {
                pFlashOffset[j] = pFlashOffset[j + 1];
            }
            (*pCurBondNum)--;

            return flash_addr; //find and delete OK
        }
    }
    return 0;                  //not find
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
    void
    blt_smp_procBondingInfoIndexAlarm(void)
{
    /* if SMP flash space is full, clean SMP flash space */
    //if(smp_bond_device_flash_cfg_idx >= SMP_PARAM_CLEAN_INDEX_ALARM_HIGH && !blm_btxbrx_state)
    {
        if (gap_eventMask & GAP_EVT_MASK_SMP_BONDING_INFO_FULL) {
            blc_gap_send_event(GAP_EVT_SMP_BONDING_INFO_FULL, NULL, 0);
        }

        my_dump_str_data(SMP_DBG_EN, "SMP flash space is full, clean SMP flash space", 0, 0);

        smpMStblBondDevice.master_cur_bondNum = 0;                                                     //clr master smp bonding num
        memset(smpMStblBondDevice.slave_cur_bondNum, 0, sizeof(smpMStblBondDevice.slave_cur_bondNum)); //clr slave smp bonding num

        /////////////////////// multi-role SMP storage init /////////////////////////////////
#if (SMP_DATABASE_INFO_SOURCE == SMP_INFO_STORAGE_IN_FLASH)
        blt_smp_initBondingInfoFromFlash(); //to get smpMStblBondDevice data from flash
#else
        // smpMStblBondDevice data initialization from where your database stored
#endif
    }
}

/**********************************************************************************/


//This API is for master only
int blc_smp_deleteBondingPeripheralInfo_by_PeerMacAddress(u8 peer_addr_type, u8 *peer_addr)
{
    return blt_smp_deleteBondingInfo_by_PeerMacAddress(1, 0, peer_addr_type, peer_addr);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Operation Type 6. Update
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
u32 blt_smp_updateBondingInfoToNearestByIndex(u8 isCentral, u8 perDevIdx, u16 index)
{
    smp_param_save_t smp_param_temp;

    blc_smp_loadBondingInfoFromFlashByIndex(isCentral, perDevIdx, index, &smp_param_temp);
    blt_smp_deleteBondingInfo_by_Index(isCentral, perDevIdx, index, false);

    return blt_smp_saveBondingInfoToFlash(isCentral, perDevIdx, &smp_param_temp);
}

#define DBG_FLASH_CLEAN 0

void blt_smp_switchBondingInfoArea(void)
{
    flash_op_sts_t flash_op_result             = FLASH_OP_SUCCESS;
    u8             temp_buf[SMP_PARAM_NV_UNIT] = {0};
    u32            dest_addr                   = smp_param_next_start_addr;
    int            flash_offset                = 0;

    //for master
    for (int i = 0; i < smpMStblBondDevice.master_cur_bondNum; i++) {
        u32 flash_addr = smp_param_current_start_addr + smpMStblBondDevice.master_bond_flash_idx[i];
        flash_read_page(flash_addr, sizeof(smp_param_save_t), temp_buf);

        int temp_dest_addr = dest_addr;
        for (size_t k = 0; k < (sizeof(smp_param_save_t) >> 4); k++) { //When designing smp_param_save_t, its size must be an integer multiple of 16
            flash_op_result = smp_write_flash_page(temp_dest_addr, 16, (u8 *)&temp_buf[k << 4]);
            temp_dest_addr += 16;
            if (flash_op_result == FLASH_OP_FAIL) {
                return;
            }
        }

        smpMStblBondDevice.master_bond_flash_idx[i] = flash_offset;
        dest_addr += SMP_PARAM_NV_UNIT;
        flash_offset += SMP_PARAM_NV_UNIT;
    }

    //for slave
    for (int i = 0; i < LOCAL_DEVICE_NUM_MAX; i++) {
        int slave_cur_bond_num = smpMStblBondDevice.slave_cur_bondNum[i];
        for (int j = 0; j < slave_cur_bond_num; j++) {
            u32 flash_addr = smp_param_current_start_addr + smpMStblBondDevice.slave_bond_flash_idx[i][j];
            flash_read_page(flash_addr, sizeof(smp_param_save_t), temp_buf);

            int temp_dest_addr = dest_addr;
            for (size_t k = 0; k < (sizeof(smp_param_save_t) >> 4); k++) {
                flash_op_result = smp_write_flash_page(temp_dest_addr, 16, (u8 *)&temp_buf[k << 4]);
                temp_dest_addr += 16;
                if (flash_op_result == FLASH_OP_FAIL) {
                    return;
                }
            }

            smpMStblBondDevice.slave_bond_flash_idx[i][j] = flash_offset;
            dest_addr += SMP_PARAM_NV_UNIT;
            flash_offset += SMP_PARAM_NV_UNIT;
        }
    }

    if (flash_op_result == FLASH_OP_SUCCESS) {
        u8 smp_flg      = FLAG_SMP_SECTOR_USE;
        flash_op_result = smp_write_flash_page(smp_param_next_start_addr + FLASH_SMP_MARK_OFFSET, 1, (u8 *)&smp_flg);
        if (flash_op_result == FLASH_OP_FAIL) {
            return;
        }

        smp_flg         = FLAG_SMP_SECTOR_CLEAR;
        flash_op_result = smp_write_flash_page(smp_param_current_start_addr + FLASH_SMP_MARK_OFFSET, 1, (u8 *)&smp_flg);
        if (flash_op_result == FLASH_OP_FAIL) {
            return;
        }

        //must use "smp_erase_flash_area"
        smp_erase_flash_area(smp_param_current_start_addr, SMP_PARAM_NV_MAX_LEN);
    }

    u32 temp                     = smp_param_current_start_addr;
    smp_param_current_start_addr = smp_param_next_start_addr;
    smp_param_next_start_addr    = temp;

    smp_bond_device_flash_cfg_idx = flash_offset - SMP_PARAM_NV_UNIT;
}

void blt_smp_cleanBondingInfoStorage(void)
{
#if DBG_FLASH_CLEAN
    if (smp_bond_device_flash_cfg_idx < 256) {
        return;
    }
#else
    if (smp_bond_device_flash_cfg_idx < SMP_PARAM_CLEAN_INDEX_ALARM_LOW) {
        return;
    }
#endif

#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb) {
        flash_prot_op_cb(FLASH_OP_EVT_STACK_SMP_SWITCH_PAIRING_INFO_BEGIN, SMP_PARAM_NV_ADDR_START, SMP_PARAM_NV_ADDR_START + 2 * SMP_PARAM_NV_MAX_LEN);
    }
#endif

    blt_smp_switchBondingInfoArea();

#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb) {
        flash_prot_op_cb(FLASH_OP_EVT_STACK_SMP_SWITCH_PAIRING_INFO_END, SMP_PARAM_NV_ADDR_START, SMP_PARAM_NV_ADDR_START + 2 * SMP_PARAM_NV_MAX_LEN);
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Operation Type 7. Save
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int blt_smp_saveBondingInfoToFlash(u8 isCentral, u8 perDevIdx, smp_param_save_t *save_param)
{
    //The storage area is full, no storage. In fact, it can be guaranteed that this condition
    //will never be established by initializing clean and automatically detecting clean in the
    //unconnected state in the mainloop.
    if (smp_bond_device_flash_cfg_idx >= FLASH_SMP_MARK_OFFSET) {
        return 0; //flash full
    }

    u32  flash_addr   = 0;
    u16  maxBondNum   = 0;
    u16 *pCurBondNum  = 0;
    u32 *pFlashOffset = 0;

    if (isCentral) {
        maxBondNum   = smpMng.master_max_bondNum;
        pCurBondNum  = (u16 *)&smpMStblBondDevice.master_cur_bondNum;
        pFlashOffset = (u32 *)&smpMStblBondDevice.master_bond_flash_idx[0];
    } else {
        if (perDevIdx >= LOCAL_DEVICE_NUM_MAX) {
            return 0; //parameter invalid
        }

        maxBondNum   = smpMng.slave_max_bondNum;
        pCurBondNum  = (u16 *)&smpMStblBondDevice.slave_cur_bondNum[perDevIdx];
        pFlashOffset = (u32 *)&smpMStblBondDevice.slave_bond_flash_idx[perDevIdx][0];
    }


#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb) {
        flash_prot_op_cb(FLASH_OP_EVT_STACK_SMP_SAVE_PAIRING_INFO_BEGIN, 0, 0);
        smp_flash_unlock = 1;
    }
#endif


    if (*pCurBondNum >= maxBondNum) {
        blt_smp_deleteBondingInfo_by_Index(isCentral, perDevIdx, 0, true); //delete index 0 SMP info
    }

#if (CUSTOM_SMP_STORAGE)
    #if (MULTIPLE_LOCAL_DEVICE_ENABLE)
        #error "If enable MULTIPLE_LOCAL_DEVICE_ENABLE, must disable CUSTOM_SMP_STORAGE! Sync with SunWei!"
    #endif
    if (smp_info_custom_save_cb) {
        u32 current_addr = smp_param_current_start_addr + smp_bond_device_flash_cfg_idx + SMP_PARAM_NV_UNIT;
        if (!smp_info_custom_save_cb(isCentral ? BLM_CONN_HANDLE : BLS_CONN_HANDLE, current_addr, (smp_param_save_t *)save_param)) {
            return current_addr;
        }
    }
#endif

    smp_bond_device_flash_cfg_idx += SMP_PARAM_NV_UNIT;
    flash_addr = smp_param_current_start_addr + smp_bond_device_flash_cfg_idx;


    my_dump_str_data(SMP_DBG_EN, "key save", 0, 0);


    u8 res = FLASH_OP_SUCCESS;


#if 1 //for PUYA FLASH, packet all data together, then write


    u8 temp_buffer[sizeof(smp_param_save_t)];
    for (size_t i = 0; i < sizeof(smp_param_save_t); i++) {
        temp_buffer[i] = 0xff;
    }


    temp_buffer[0] = FLAG_SMP_PARAM_SAVE_PENDING;
    smemcpy(temp_buffer + OFFSETOF(smp_param_save_t, role_dev_idx), (u8 *)&save_param->role_dev_idx, 1);
    smemcpy(temp_buffer + OFFSETOF(smp_param_save_t, peer_addr_type), (u8 *)&save_param->peer_addr_type, 7);
    smemcpy(temp_buffer + OFFSETOF(smp_param_save_t, peer_id_adrType), (u8 *)&save_param->peer_id_adrType, 7);
    smemcpy(temp_buffer + OFFSETOF(smp_param_save_t, encrypt_key_size), (u8 *)&save_param->encrypt_key_size, 8);
    smemcpy(temp_buffer + OFFSETOF(smp_param_save_t, local_id_adrType), (u8 *)&save_param->local_id_adrType, 7);
    smemcpy(temp_buffer + OFFSETOF(smp_param_save_t, local_peer_ltk), save_param->local_peer_ltk, 16);
    smemcpy(temp_buffer + OFFSETOF(smp_param_save_t, random), save_param->random, 8);
    smemcpy(temp_buffer + OFFSETOF(smp_param_save_t, ediv), (u8 *)&save_param->ediv, 2);
    smemcpy(temp_buffer + OFFSETOF(smp_param_save_t, peer_irk), save_param->peer_irk, 16);
    smemcpy(temp_buffer + OFFSETOF(smp_param_save_t, local_irk), save_param->local_irk, 16);

    res = smp_write_flash_page(flash_addr, sizeof(smp_param_save_t), temp_buffer);

    if (res == FLASH_OP_SUCCESS) {
        u8 flag = save_param->flag;
        res     = smp_write_flash_page(flash_addr, 1, (u8 *)&flag);

        if (res == FLASH_OP_SUCCESS) {
            pFlashOffset[*pCurBondNum] = smp_bond_device_flash_cfg_idx;
            (*pCurBondNum)++;

    #if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
            if (flash_prot_op_cb) {
                flash_prot_op_cb(FLASH_OP_EVT_STACK_SMP_SAVE_PAIRING_INFO_END, 0, 0);
                smp_flash_unlock = 0;
            }
    #endif

            return flash_addr;
        }
    }
#else
    u8 flag_pending = FLAG_SMP_PARAM_SAVE_PENDING;

    res += smp_write_flash_page(flash_addr, 1, &flag_pending);
    res += smp_write_flash_page(flash_addr + OFFSETOF(smp_param_save_t, role_dev_idx), 1, (u8 *)&save_param->role_dev_idx);
    res += smp_write_flash_page(flash_addr + OFFSETOF(smp_param_save_t, peer_addr_type), 7, (u8 *)&save_param->peer_addr_type);
    res += smp_write_flash_page(flash_addr + OFFSETOF(smp_param_save_t, peer_id_adrType), 7, (u8 *)&save_param->peer_id_adrType);
    res += smp_write_flash_page(flash_addr + OFFSETOF(smp_param_save_t, encrypt_key_size), 8, (u8 *)&save_param->encrypt_key_size);
    res += smp_write_flash_page(flash_addr + OFFSETOF(smp_param_save_t, local_id_adrType), 7, (u8 *)&save_param->local_id_adrType);
    res += smp_write_flash_page(flash_addr + OFFSETOF(smp_param_save_t, local_peer_ltk), 16, save_param->local_peer_ltk);
    res += smp_write_flash_page(flash_addr + OFFSETOF(smp_param_save_t, random), 8, save_param->random);
    res += smp_write_flash_page(flash_addr + OFFSETOF(smp_param_save_t, ediv), 2, (u8 *)&save_param->ediv);
    res += smp_write_flash_page(flash_addr + OFFSETOF(smp_param_save_t, peer_irk), 16, save_param->peer_irk);
    res += smp_write_flash_page(flash_addr + OFFSETOF(smp_param_save_t, local_irk), 16, save_param->local_irk);

    if (res == FLASH_OP_SUCCESS) {
        u8 flag = save_param->flag;
        res     = smp_write_flash_page(flash_addr, 1, (u8 *)&flag);

        if (res == FLASH_OP_SUCCESS) {
            pFlashOffset[*pCurBondNum] = smp_bond_device_flash_cfg_idx;
            (*pCurBondNum)++;

            return flash_addr;
        }
    }
#endif


#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb) {
        flash_prot_op_cb(FLASH_OP_EVT_STACK_SMP_SAVE_PAIRING_INFO_END, 0, 0);
        smp_flash_unlock = 0;
    }
#endif

    return 0; //Fail
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Configuration
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void blc_smp_setDevExceedMaxStrategy(dev_exceed_max_strategy_t strategy)
{
    smpMng.dev_exceed_max_strategy = strategy;
}

void blc_smp_configPairingSecurityInfoStorageAddressAndSize(int address, int size_byte)
{
    SMP_PARAM_NV_ADDR_START = address;
    SMP_PARAM_NV_MAX_LEN    = size_byte;

    FLASH_SMP_MARK_OFFSET = size_byte - 16;
}

ble_sts_t blc_smp_setBondingDeviceMaxNumber(int cen_max_bondNum, int per_max_bondNum)
{
    if (cen_max_bondNum > SMP_MASTER_BONDING_DEVICE_MAX_NUM || per_max_bondNum > SMP_SLAVE_BONDING_DEVICE_MAX_NUM) {
        return SMP_ERR_INVALID_PARAMETER;
    }

    smpMng.master_max_bondNum = cen_max_bondNum;
    smpMng.slave_max_bondNum  = per_max_bondNum;
    return BLE_SUCCESS;
}

#if (CUSTOM_SMP_STORAGE)
ble_sts_t blc_smp_initCustomBondingInfoEnable(u8 enable, smp_info_custom_save_callback_t custom_save_cb, smp_info_custom_load_callback_t custom_load_cb)
{
    if (smp_custom_enable && (!custom_save_cb || !custom_load_cb)) {
        return SMP_ERR_INVALID_PARAMETER;
    }
    smp_custom_enable = enable;
    if (enable) {
        smp_info_custom_save_cb = custom_save_cb;
        smp_info_custom_load_cb = custom_load_cb;
    } else {
        smp_info_custom_save_cb = NULL;
        smp_info_custom_load_cb = NULL;
    }
    return BLE_SUCCESS;
}

ble_sts_t blc_smp_setCustomBondingInfoAddress(u16 connHandle, u16 cur_bondNum, u32 bond_flash_start_addr, u32 *bond_flash_idx)
{
    if (!smp_custom_enable) {
        return SMP_ERR_INVALID_PARAMETER;
    }

    u32 *smp_flash_idx_p;
    if (connHandle & BLM_CONN_HANDLE) { //master
        if (cur_bondNum > smpMng.master_max_bondNum) {
            return SMP_ERR_INVALID_PARAMETER;
        }
        smp_flash_idx_p                       = smpMStblBondDevice.master_bond_flash_idx;
        smpMStblBondDevice.master_cur_bondNum = cur_bondNum;
    } else { //slave
        u8 slave_dev_idx = 0;
        if (cur_bondNum > smpMng.slave_max_bondNum) {
            return SMP_ERR_INVALID_PARAMETER;
        }
        smp_flash_idx_p                                     = smpMStblBondDevice.slave_bond_flash_idx[slave_dev_idx]; //Todo: process multi-device
        smpMStblBondDevice.slave_cur_bondNum[slave_dev_idx] = cur_bondNum;
    }
    smp_param_current_start_addr = bond_flash_start_addr;
    smemcpy(smp_flash_idx_p, bond_flash_idx, cur_bondNum);

    return BLE_SUCCESS;
}
#endif
