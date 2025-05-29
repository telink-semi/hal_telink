/********************************************************************************************************
 * @file    store_internal.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#pragma once

#if (defined(HOST_V2_ENABLE))
typedef struct __attribute__((packed)) 
#else
typedef struct
#endif
{
    u8  val;
    u16 version;
} nv_sector_mark_t;

#if (defined(HOST_V2_ENABLE))
typedef struct __attribute__((packed)) 
#else
typedef struct
#endif
{
    u8 val;
} nv_block_mark_t;

#define AUD_NV_VERSION              0x0200

#define AUD_NV_PARAM_UNIT           384
#define AUD_NV_VALUE_SIZE           (AUD_NV_PARAM_UNIT - sizeof(nv_store_head_t))

#define BLT_STORE_LOG(fmt, ...)     BLC_PROFILE_DEBUG(DBG_PRF_MASK_STORE_LOG, "[STORE]" fmt, ##__VA_ARGS__)

#define AUD_MAX_PAIRED_NUM          4

#define AUD_PARAM_ATT_STORE(a, s)   (a ? (a - s) : 0)
#define AUD_PARAM_ATT_RESTORE(a, s) (a ? (a + s) : 0)


//aud storage area reaches alarm line 0 threshold
#define AUD_PARAM_CLEAN_INDEX_ALARM_LOW (u32)((nvEachBlockSize * 3) >> 2) //e.g.: 8k*3/4 -> 38*160; 4k*3/4 -> 19*160;
//aud storage area reaches alarm line 1 threshold
#define AUD_PARAM_CLEAN_INDEX_ALARM_HIGH    (u32)(nvEachBlockSize - (AUD_NV_PARAM_UNIT << 1)) //e.g.: 8k -160*2 -> 50*160; 4k -160*2 -> 23*160;


#define AUD_PARAM_NV_SEC_ADDR_START         (nvStartAddr + nvEachBlockSize)

#define FLAG_AUD_PARAM_SAVE_BASE            0x8A // 1000 1010

#define FLAG_AUD_PARAM_SAVE_PEER_SUP_AR     0x0A // 0000 1010  new storage If Peer device supports address resolution
#define FLAG_AUD_PARAM_SAVE_PEER_NSUP_AR    0x8A // 1000 1010  new storage If Peer device not supports address resolution

#define FLAG_AUD_PARAM_SAVE_PENDING         0xBF // 1011 1111
#define FLAG_AUD_PARAM_SAVE_ERASE           0x00 //

#define FLAG_AUD_PARAM_MASK                 0x0F // 0000 1111
#define FLAG_AUD_PARAM_VALID                0x0A // 0000 1010

#define FLAG_AUD_SECTOR_USE                 0x3C
#define FLAG_AUD_SECTOR_CLEAR               0x00

#define FLAG_AUD_ROLE_MASTER                BIT(0)

#define AUD_PARAM_NV_SEC_ADDR_START         (nvStartAddr + nvEachBlockSize)
#define AUD_MARK_MAX_SIZE                   16

#define FLASH_AUD_MARK_OFFSET               nvEachBlockSize - AUD_MARK_MAX_SIZE

#define CHECK_NV_SECTOR_MARK(sectorMark)    (sectorMark.val == FLAG_AUD_SECTOR_USE && sectorMark.version == AUD_NV_VERSION)
#define CHECK_NV_BLOCK_MARK(blockMark)      (blockMark.val == U8_MAX)
#define CHECK_NV_BLOCK_MARK_SUCC(blockMark) (blockMark.val == FLAG_AUD_PARAM_SAVE_BASE)
#define NV_SET_BLOCK_MARK_PENDING(mark)     mark.val = FLAG_AUD_PARAM_SAVE_PENDING
#define NV_SET_BLOCK_MARK_SUCCESS(mark)     mark.val = FLAG_AUD_PARAM_SAVE_BASE

#if (defined(HOST_V2_ENABLE))
typedef struct __attribute__((packed)) 
#else
typedef struct
#endif
{
    u16 curBondNum;
    u16 maxBondNum;
} aud_bond_device_t;

#if (defined(HOST_V2_ENABLE))
typedef struct __attribute__((packed)) 
#else
typedef struct
#endif
{
    nv_block_mark_t mark;
    u8              peer_addr_type; // peer_addr_type & peer_addr must be together cause using flash read packed "type&address" in code
    u8              peer_addr[6];   //address used in link layer connection
    u16             valueLen;
} nv_store_head_t;

#if (defined(HOST_V2_ENABLE))
typedef struct __attribute__((packed)) 
#else
typedef struct
#endif
{
    u8 length;
    u8 id;
    u8 value[0];
} nv_store_value_t;

#if (defined(HOST_V2_ENABLE))
typedef struct __attribute__((packed)) 
#else
typedef struct
#endif
{
    nv_store_head_t  head;
    nv_store_value_t val[0];
} nv_store_param_t;

typedef union
{ //Total 154

} blc_aud_hdl_info_t;

extern int prf_store_used;
extern u32 nvEachBlockSize;

extern bool smp_erase_flash_area(u32 addr, int size_byte);

u8 *blt_aud_getStoreBuf(u16 connHandle);
u8 *blt_aud_getStoreBufByIdx(u8 instIdx);

void blt_prf_cleanBondingInfoStorage(void);
void blt_prf_procBondingInfoIndexAlarm(void);
bool blt_aud_isBondingInfoStorageLowAlarmed(void);
bool blt_aud_isBondingInfoStorageHighAlarmed(void);

void blt_prf_storeClientHdl(void *dst, void *src, u16 *endPtr);
void blt_prf_loadClientHdl(void *dst, void *src, u16 *endPtr);
u16  blt_prf_loadPairingInfoByAclHandle(u16 connHandle, u8 nvData[AUD_NV_VALUE_SIZE]);
int  blt_prf_saveBondingInformationToFlash(u16 connHandle, u8 nvData[AUD_NV_VALUE_SIZE], u16 totalLen);

/**
 * @brief      This function is used to delete binding information according to the peer device address and device address type.
 * @param[in]  peer_addr_type - Address type.
 * @param[in]  peer_addr - Address.
 * @return     0: Failed to delete the binding information; others: FLASH address of the delete bonding information area.
 */
u32 blc_prf_deletePairingInfoByPeerAddr(u8 isMaster, u8 peer_addr_type, u8 *peer_addr);

/**
 * @brief      This function is used to delete binding information according to the connection handle.
 * @param[in]  connHandle
 * @return     0: Failed to delete the binding information; others: FLASH address of the delete bonding information area.
 */
u32 blc_prf_deletePairingInfoByConnHandle(u16 connHandle);

/**
 * @brief      This function is used to clear all binding information stored in the local FLASH.
 * @param[in]  none.
 * @return     none.
 */
void blc_prf_eraseAllPairingInfo(void);

/**
 * @brief      This function is used to get the bonding information numbers.
 * @param[in]  isMaster - Is it a Master role: 0: slave role, others: master role.
 * @return     0: The number of bound devices is 0; others: Number of bound devices.
 */
u8 blc_prf_getCurrentPairingDeviceNumber(u8 isMasterRole);
