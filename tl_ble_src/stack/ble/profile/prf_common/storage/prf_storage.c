/********************************************************************************************************
 * @file    aud_storage.c
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


_attribute_ble_data_retention_ static u32 nvStartAddr                = 0; //2M default
_attribute_ble_data_retention_ u32        nvEachBlockSize            = 0; //2*4096;//default 8K
_attribute_ble_data_retention_ static u32 nvCurrentUseFlashAddress   = 0; //current use sector
_attribute_ble_data_retention_ static u32 nvCurrentWriteFlashAddress = 0; //new device info stored flash idx

_attribute_ble_data_retention_ static aud_bond_device_t prf_bond_mng;

_attribute_ble_data_retention_ int prf_store_used = 0;

//NOTE: return 0 represent SUCCESS !!!!
static flash_op_sts_t prf_write_flash_page(u32 addr, u32 len, u8 *buf)
{
    u8 check_buf[AUD_NV_PARAM_UNIT];

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

static u32 blt_prf_getNextStartAddr(void)
{
    return nvCurrentUseFlashAddress == nvStartAddr ? nvStartAddr + nvEachBlockSize : nvStartAddr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Operation Type 1. Init
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void blt_prf_initBondingInfoFromFlash(void)
{
    // 1. init current start addr
    // sector mark, solve power off at clean flash problem
    nv_sector_mark_t sector_mark0;
    nv_sector_mark_t sector_mark1;
    flash_read_page(nvStartAddr + FLASH_AUD_MARK_OFFSET, sizeof(nv_sector_mark_t), (u8 *)&sector_mark0);
    flash_read_page(nvStartAddr + nvEachBlockSize + FLASH_AUD_MARK_OFFSET, sizeof(nv_sector_mark_t), (u8 *)&sector_mark1);

    if (CHECK_NV_SECTOR_MARK(sector_mark0)) {
        nvCurrentUseFlashAddress = nvStartAddr; //use area 0

        BLT_STORE_LOG("sector0 used:0x%x", nvCurrentUseFlashAddress);
    } else if (CHECK_NV_SECTOR_MARK(sector_mark1)) {
        nvCurrentUseFlashAddress = nvStartAddr + nvEachBlockSize; //use area 1

        BLT_STORE_LOG("sector1 used:0x%x", nvCurrentUseFlashAddress);
    } else {
        nvCurrentUseFlashAddress = nvStartAddr;

        nv_sector_mark_t smp_flg = {
            .val     = FLAG_AUD_SECTOR_USE,
            .version = AUD_NV_VERSION,
        };
        prf_write_flash_page(nvCurrentUseFlashAddress + FLASH_AUD_MARK_OFFSET, sizeof(nv_sector_mark_t), (u8 *)&smp_flg);
        BLT_STORE_LOG("initial sector0 used:0x%x", nvCurrentUseFlashAddress);
    }

    //erase unused area
    u32 sector_head;
    flash_read_page(blt_prf_getNextStartAddr(), 4, (u8 *)&sector_head);

    if (sector_head != U32_MAX) {
        smp_erase_flash_area(blt_prf_getNextStartAddr(), nvEachBlockSize); //Attention:  can not use smp_erase_flash_sector
    }

    prf_bond_mng.curBondNum = 0;
    prf_bond_mng.maxBondNum = AUD_MAX_PAIRED_NUM;

    //2. load bonding device info from flash to SRAM(prf_bond_mng)
    u32 current_flash_adr;
    for (nvCurrentWriteFlashAddress = 0; nvCurrentWriteFlashAddress < FLASH_AUD_MARK_OFFSET;
         nvCurrentWriteFlashAddress += AUD_NV_PARAM_UNIT) {
        current_flash_adr = nvCurrentUseFlashAddress + nvCurrentWriteFlashAddress;

        nv_block_mark_t blockMark;

        flash_read_page(current_flash_adr, sizeof(nv_block_mark_t), (u8 *)&blockMark);

        if (CHECK_NV_BLOCK_MARK(blockMark)) {
            break;
        }
        prf_bond_mng.curBondNum++;
    }

    nvCurrentWriteFlashAddress += nvCurrentUseFlashAddress;
    BLT_STORE_LOG("current write flash address offset is %x", nvCurrentWriteFlashAddress);

    blt_prf_cleanBondingInfoStorage();
}

static int blt_prf_deleteSmpStoreEvtTriggered(u8 isMaster, u32 flash_addr)
{
    BLT_STORE_LOG("%s: SMP delete bonding information:0x%x", isMaster ? "M" : "S", flash_addr);

    u8 peer_addr[6];
    u8 peer_addr_type;
    u8 peer_irk[16] = {0};

    flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_addr_type), 1, &peer_addr_type);
    flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_addr), 6, peer_addr);
    flash_read_page(flash_addr + OFFSETOF(smp_param_save_t, peer_irk), 16, peer_irk);

    BLT_STORE_LOG(" peer_addr_type:0x%x", peer_addr_type);
    BLT_STORE_LOG(" peer_addr:%s", addr_to_str(peer_addr));

    flash_addr = blc_prf_deletePairingInfoByPeerAddr(isMaster, peer_addr_type, peer_addr);

    /*
     * Here, SMP has deleted the SMP binding information for certain reasons. We will also delete
     * the audio binding SDP and other information
     */

    return flash_addr;
}

/**
 * @brief      This function is used to initialize default audio store address.
 * @param[in]  none
 * @return     none
 */
static void blt_prf_initDefaultPairingInfoStoreAddress(void)
{
    if (nvStartAddr) {
        return;
    }

    nvEachBlockSize = FLASH_SDP_ATT_MAX_SIZE;
    if (0) {
    }
#if (FLASH_P25Q80U_SUPPORT_EN) //1M
    else if (blc_flash_capacity == FLASH_SIZE_1M) {
        nvStartAddr = FLASH_AUD_ATT_ADDRESS_1M_FLASH;
    }
#endif
#if (FLASH_P25Q16SU_SUPPORT_EN || FLASH_GD25LQ16E_SUPPORT_EN) //2M
    else if (blc_flash_capacity == FLASH_SIZE_2M) {
        nvStartAddr = FLASH_AUD_ATT_ADDRESS_2M_FLASH;
    }
#endif
#if (FLASH_P25Q32SU_SUPPORT_EN) //4M
    else if (blc_flash_capacity == FLASH_SIZE_4M) {
        nvStartAddr = FLASH_AUD_ATT_ADDRESS_4M_FLASH;
    }
#endif
#if (FLASH_P25Q128L_SUPPORT_EN || FLASH_P25Q128H_SUPPORT_EN) //16M
    else if (blc_flash_capacity == FLASH_SIZE_16M) {
        nvStartAddr = FLASH_AUD_ATT_ADDRESS_16M_FLASH;
    }
#endif
    else {
        nvStartAddr = FLASH_AUD_ATT_ADDRESS_2M_FLASH;
    }
}

void blc_prf_initPairingInfoStoreModule(void)
{
    BLT_STORE_LOG("Audio bonding information Init");
    blt_prf_initDefaultPairingInfoStoreAddress();

    smp_delete_cb = blt_prf_deleteSmpStoreEvtTriggered;

    blt_prf_initBondingInfoFromFlash(); //to get prf_bond_mng data from flash

    prf_store_used = 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Operation Type 2. Search
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static u32 blt_prf_searchBondingDeviceByPeerAddr(u8 isMaster, u8 peerAddrType, u8 peerAddr[6], u16 *valueLen)
{
    (void)isMaster;
    u32 current_flash_adr;
    for (u32 startAddress = 0; startAddress < FLASH_AUD_MARK_OFFSET; startAddress += AUD_NV_PARAM_UNIT) {
        current_flash_adr = nvCurrentUseFlashAddress + startAddress;

        nv_store_head_t blockHead;

        flash_read_page(current_flash_adr, sizeof(nv_store_head_t), (u8 *)&blockHead);

        if (CHECK_NV_BLOCK_MARK(blockHead.mark)) {
            return 0;
        }

        if (CHECK_NV_BLOCK_MARK_SUCC(blockHead.mark)) {
            if (IS_RESOLVABLE_PRIVATE_ADDR(peerAddrType, peerAddr)) {
            } else {
                if (blockHead.peer_addr_type == peerAddrType && !memcmp(blockHead.peer_addr, peerAddr, 6)) {
                    if (valueLen) {
                        *valueLen = blockHead.valueLen;
                    }
                    return current_flash_adr; //return flash address, it is not flash address offset
                }
            }
        }
    }

    return 0;
}

//static
u32 blt_prf_searchBondingDeviceByAclHandle(u16 connHandle, u16* valueLen)
{
    u8  peer_addr_type = blt_ll_getAclConnPeerAddrType(connHandle);
    u8 *peer_addr      = blt_ll_getAclConnPeerAddr(connHandle);

    return blt_prf_searchBondingDeviceByPeerAddr(connHandle & BLM_CONN_HANDLE, peer_addr_type, peer_addr, valueLen);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Operation Type 4. Load
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
u16 blt_prf_loadPairingInfoByAclHandle(u16 connHandle, u8 nvData[AUD_NV_VALUE_SIZE])
{
    if (nvData == NULL) {
        return 0;
    }

    u16 valueLen  = 0;
    u32 flashAddr = blt_prf_searchBondingDeviceByAclHandle(connHandle, &valueLen);

    if (flashAddr && valueLen <= AUD_NV_VALUE_SIZE && valueLen) {
        flash_read_page(flashAddr + OFFSETOF(nv_store_param_t, val), valueLen, nvData);
        BLT_STORE_LOG("Load Bonding Info To Flash SUCC, flash address is 0x%08x", flashAddr);
    } else {
        BLT_STORE_LOG("Load Bonding Info To Flash NONE");
    }


    return valueLen;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Operation Type 5. Delete
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void blc_prf_eraseAllPairingInfo(void)
{
    BLT_STORE_LOG("Audio Erase all Bonding Info");

    smp_erase_flash_area(nvStartAddr, nvEachBlockSize);
    smp_erase_flash_area(AUD_PARAM_NV_SEC_ADDR_START, nvEachBlockSize);

    nvCurrentUseFlashAddress = nvStartAddr; //use sector 0

    nv_sector_mark_t smp_flg = {
        .val     = FLAG_AUD_SECTOR_USE,
        .version = AUD_NV_VERSION,
    };
    prf_write_flash_page(nvCurrentUseFlashAddress + FLASH_AUD_MARK_OFFSET, sizeof(nv_sector_mark_t), (u8 *)&smp_flg);

    nvCurrentWriteFlashAddress = nvCurrentUseFlashAddress;
    memset(&prf_bond_mng, 0, sizeof(aud_bond_device_t));
}

//static
void blt_prf_deleteBondingInfoByFlashAddress(u32 flashAddr)
{
    nv_block_mark_t mark = {.val = FLAG_AUD_PARAM_SAVE_ERASE};
    prf_write_flash_page(flashAddr, sizeof(nv_block_mark_t), (u8 *)&mark);

    prf_bond_mng.curBondNum--;
}

u32 blc_prf_deletePairingInfoByPeerAddr(u8 isMaster, u8 peer_addr_type, u8 *peer_addr)
{
    u32 flash_addr = blt_prf_searchBondingDeviceByPeerAddr(isMaster, peer_addr_type, peer_addr, NULL);
    if (flash_addr) {
        //should delete the older aud banding info
        blt_prf_deleteBondingInfoByFlashAddress(flash_addr);
    }

    BLT_STORE_LOG("Audio delete Bonding Info By PeerAddr FAIL");
    return 0; //Fail
}

u32 blc_prf_deletePairingInfoByConnHandle(u16 connHandle)
{
    u32 flash_addr = blt_prf_searchBondingDeviceByAclHandle(connHandle, NULL);
    if (flash_addr) {
        //should delete the older aud banding info
        blt_prf_deleteBondingInfoByFlashAddress(flash_addr);
    }

    BLT_STORE_LOG("Audio delete Bonding Info By PeerAddr FAIL");
    return 0; //Fail
}

void blt_prf_procBondingInfoIndexAlarm(void)
{
    /* if AUD flash space is full, clean AUD flash space */
    if (nvCurrentWriteFlashAddress - nvCurrentUseFlashAddress >= AUD_PARAM_CLEAN_INDEX_ALARM_HIGH && !blm_btxbrx_state) {
        BLT_STORE_LOG("[Bonding Index Alarm] AUD flash space is full, clean flash space:%+.d, %+.d", nvCurrentWriteFlashAddress, AUD_PARAM_CLEAN_INDEX_ALARM_HIGH);

        blt_prf_cleanBondingInfoStorage(); //to get prf_bond_mng data from flash
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Operation Type 6. Update
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void blt_prf_cleanBondingInfoStorage(void)
{
    if (nvCurrentWriteFlashAddress - nvCurrentUseFlashAddress < AUD_PARAM_CLEAN_INDEX_ALARM_LOW) {
        BLT_STORE_LOG("nvCurrentWriteFlashAddress < AUD_PARAM_CLEAN_INDEX_ALARM_LOW(%x)", nvCurrentWriteFlashAddress);
        return;
    }

    flash_op_sts_t    flash_op_result             = FLASH_OP_SUCCESS;
    u8                temp_buf[AUD_NV_PARAM_UNIT] = {0};
    u32               dest_addr                   = blt_prf_getNextStartAddr();
    nv_store_param_t *nvStore                     = (nv_store_param_t *)temp_buf;

    for (nvCurrentWriteFlashAddress = 0; nvCurrentWriteFlashAddress < FLASH_AUD_MARK_OFFSET;
         nvCurrentWriteFlashAddress += AUD_NV_PARAM_UNIT) {
        u32 current_flash_adr = nvCurrentUseFlashAddress + nvCurrentWriteFlashAddress;

        flash_read_page(current_flash_adr, AUD_NV_PARAM_UNIT, (u8 *)temp_buf);

        if (CHECK_NV_BLOCK_MARK_SUCC(nvStore->head.mark)) {
            if (prf_write_flash_page(dest_addr, AUD_NV_PARAM_UNIT, temp_buf) == FLASH_OP_FAIL) {
                return;
            }

            dest_addr += AUD_NV_PARAM_UNIT;
        }

        if (CHECK_NV_BLOCK_MARK(nvStore->head.mark)) {
            break;
        }
    }

    nv_sector_mark_t sectorMark;

    sectorMark.val     = FLAG_AUD_SECTOR_USE;
    sectorMark.version = AUD_NV_VERSION;
    flash_op_result    = prf_write_flash_page(blt_prf_getNextStartAddr() + FLASH_AUD_MARK_OFFSET, sizeof(nv_sector_mark_t), (u8 *)&sectorMark);
    if (flash_op_result == FLASH_OP_FAIL) {
        return;
    }

    sectorMark.val     = FLAG_AUD_SECTOR_CLEAR;
    sectorMark.version = 0x0000;
    flash_op_result    = prf_write_flash_page(nvCurrentUseFlashAddress + FLASH_AUD_MARK_OFFSET, sizeof(nv_sector_mark_t), (u8 *)&sectorMark);
    if (flash_op_result == FLASH_OP_FAIL) {
        return;
    }

    //must use "smp_erase_flash_area"
    smp_erase_flash_area(nvCurrentUseFlashAddress, nvEachBlockSize);

    nvCurrentUseFlashAddress = blt_prf_getNextStartAddr();

    nvCurrentWriteFlashAddress += nvCurrentUseFlashAddress;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Operation Type 7. Save
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int blt_prf_saveBondingInformationToFlash(u16 connHandle, u8 nvData[AUD_NV_VALUE_SIZE], u16 totalLen)
{
    //The storage area is full, no storage. In fact, it can be guaranteed that this condition
    //will never be established by initializing clean and automatically detecting clean in the
    //unconnected state in the mainloop.
    if (nvCurrentWriteFlashAddress - nvCurrentUseFlashAddress >= FLASH_AUD_MARK_OFFSET) {
        BLT_STORE_LOG("Audio save bonding info Full");
        return 0; //flash full
    }

    if (prf_bond_mng.curBondNum >= prf_bond_mng.maxBondNum) {
        nv_block_mark_t blockMark;
        u32             flashOffsetAddr = nvCurrentUseFlashAddress;

        for (; flashOffsetAddr < nvCurrentUseFlashAddress + FLASH_AUD_MARK_OFFSET;
             flashOffsetAddr += AUD_NV_PARAM_UNIT) {
            flash_read_page(flashOffsetAddr, sizeof(nv_block_mark_t), (u8 *)&blockMark);

            if (CHECK_NV_BLOCK_MARK_SUCC(blockMark)) {
                break;
            }

            if (CHECK_NV_BLOCK_MARK(blockMark)) {
                break;
            }
        }

        blt_prf_deleteBondingInfoByFlashAddress(flashOffsetAddr);
    }

    u8 temp_buffer[AUD_NV_PARAM_UNIT];
    memset(temp_buffer, U8_MAX, AUD_NV_PARAM_UNIT);

    nv_store_param_t *nvValue = (nv_store_param_t *)temp_buffer;

    NV_SET_BLOCK_MARK_PENDING(nvValue->head.mark);

    nvValue->head.valueLen = totalLen;

    nvValue->head.peer_addr_type = blt_ll_getAclConnPeerAddrType(connHandle);
    memcpy(nvValue->head.peer_addr, blt_ll_getAclConnPeerAddr(connHandle), 6);

    memcpy(nvValue->val, nvData, totalLen);

    u8 res = prf_write_flash_page(nvCurrentWriteFlashAddress, AUD_NV_PARAM_UNIT, temp_buffer);

    if (res == FLASH_OP_SUCCESS) {
        NV_SET_BLOCK_MARK_SUCCESS(nvValue->head.mark);
        res = prf_write_flash_page(nvCurrentWriteFlashAddress, sizeof(nv_block_mark_t), (u8 *)&nvValue->head.mark);
        if (res == FLASH_OP_SUCCESS) {
            BLT_STORE_LOG("Save Bonding Info To Flash SUCC:0x%x", nvCurrentWriteFlashAddress);
            nvCurrentWriteFlashAddress += AUD_NV_PARAM_UNIT;
            prf_bond_mng.curBondNum++;
            return nvCurrentWriteFlashAddress;
        }
    }

    BLT_STORE_LOG("Save Bonding Info To Flash FAIL:0x%x", nvCurrentWriteFlashAddress);
    nvCurrentWriteFlashAddress += AUD_NV_PARAM_UNIT;
    return 0; //Fail
}

typedef struct
{
    u16 baseHandle; //start handle
    u8  endHdl;     //ending handle
    u8  attrHdl[0]; //attribute handle
} blt_prf_hdl_info_t;

typedef struct
{
    gattc_sub_ccc_msg_t ntfInput;
    u16                 attrHdl[0];
} blt_prf_client_t;

void blt_prf_storeClientHdl(void *dst, void *src, u16 *endPtr)
{
    blt_prf_client_t   *client = (blt_prf_client_t *)src;
    blt_prf_hdl_info_t *info   = (blt_prf_hdl_info_t *)dst;
    info->baseHandle           = client->ntfInput.startHdl;
    info->endHdl               = AUD_PARAM_ATT_STORE(client->ntfInput.endHdl, info->baseHandle);
    u16 *clientAttrHdl         = client->attrHdl;
    u8  *infoAttrHdl           = info->attrHdl;
    while (clientAttrHdl <= endPtr) {
        *infoAttrHdl = AUD_PARAM_ATT_STORE(*clientAttrHdl, info->baseHandle);
        infoAttrHdl++;
        clientAttrHdl++;
    }
}

void blt_prf_loadClientHdl(void *dst, void *src, u16 *endPtr)
{
    blt_prf_client_t   *client = (blt_prf_client_t *)dst;
    blt_prf_hdl_info_t *info   = (blt_prf_hdl_info_t *)src;
    client->ntfInput.startHdl  = info->baseHandle;
    client->ntfInput.endHdl    = AUD_PARAM_ATT_RESTORE(info->endHdl, info->baseHandle);
    u16 *clientAttrHdl         = client->attrHdl;
    u8  *infoAttrHdl           = info->attrHdl;
    while (clientAttrHdl <= endPtr) {
        *clientAttrHdl = AUD_PARAM_ATT_RESTORE(*infoAttrHdl, info->baseHandle);
        infoAttrHdl++;
        clientAttrHdl++;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Configuration 8
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void blc_prf_configPairingInfoStorageAddressAndSize(int address, int size_byte)
{
    nvStartAddr     = address;
    nvEachBlockSize = size_byte;
}

int blc_prf_setPairingDeviceMaxNumber(int maxBondNum)
{
    if (maxBondNum >= (int)(nvEachBlockSize / AUD_NV_PARAM_UNIT)) {
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    prf_bond_mng.maxBondNum = maxBondNum;

    return BLE_SUCCESS;
}

u8 blc_prf_getCurrentPairingDeviceNumber(u8 isMaster)
{
    (void)isMaster;
    return prf_bond_mng.curBondNum;
}
