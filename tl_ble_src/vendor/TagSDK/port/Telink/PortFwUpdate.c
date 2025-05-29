/* ***************************************************************************
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of Samsung
 * Electronics Co., Ltd. ("Confidential Information").
 * You shall not disclose such Confidential Information and shall use it
 * only in accordance with the terms of the license agreement you entered
 * into with Samsung Electronics Co., Ltd. ("SAMSUNG")
 * SAMSUNG MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SAMSUNG SHALL NOT BE
 * LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 *
 ****************************************************************************/
#include <stdint.h>
#include "TagConfig.h"

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
//#include "ota_server.h"



#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)


//#include "TagCore.h"
#include "TagDebug.h"

#include "PortFwUpdate.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG
#define  UNUSEDARG(x)  ((void )x);
#define MAX_ALLOWED_WRITE_WITHOUT_RESPONSE (40)

//extern unsigned int ota_program_bootAddr;
//extern unsigned int ota_firmware_max_size;
//extern unsigned int ota_program_offset;

//int ota_save_data(u32 flash_addr, int len, u8 *data);
//void blt_ota_writeBootMark(void);
_attribute_ble_data_retention_ bool portble_fw_status = false;
_attribute_ble_data_retention_ unsigned int        portble_ota_program_offset = 0;

uint8_t PortFwUpdateGetMaxWriteWithoutResponse(void)
{
    return MAX_ALLOWED_WRITE_WITHOUT_RESPONSE;
}

uint32_t PortFwUpdateGetStartAddress()
{
    TAG_LOG_I("PortFwUpdateGetStartAddress 0x%4x",blc_ota_getNextFirmwareStartAddress());
    if(blc_ota_getCurrentFirmwareStartAddress())
    {
        portble_ota_program_offset = 0;
    }
    else
    {
        portble_ota_program_offset = MULTI_BOOT_ADDR_0x80000;
    }
    return portble_ota_program_offset;
}

static u8 temp_buf[243];
TagError_t PortFwUpdateWriteFlash(uint32_t length, uint32_t Addr, uint8_t *pData)
{
    TAG_LOG_I("PortFwUpdateWriteFlash 0x%4x %d",Addr,length);
    static u8 fw_flag = 0;
    Addr = Addr - portble_ota_program_offset;
    if(fw_flag == 0)
    {
        tlkapi_send_string_data(APP_LOG_EN, "fw updata ", pData, 16);
    }
    if (Addr <= BOOT_MARK_ADDR && (Addr + length) > BOOT_MARK_ADDR)
    {
        smemcpy(temp_buf,pData,length);
        ota_save_data(Addr,length,temp_buf);
    }
    else
    {
        ota_save_data(Addr,length,pData);
    }
    return 0;
}

void PortFwUpdateReadFlash(uint16_t length, uint32_t Addr, uint8_t *inbuf)
{
    TAG_LOG_I("PortFwUpdateReadFlash");
    flash_read_page(Addr, length, inbuf);
    /* empty */
}

TagError_t PortFwUpdateEraseFlash(uint32_t length, uint32_t Addr)
{
    TAG_LOG_I("PortFwUpdateEraseFlash 0x%04X %d",Addr,length);
    int sec_num = (length + ((1L<<12) -1)) >> 12;
#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb) {
        flash_prot_op_cb(FLASH_OP_EVT_STACK_OTA_CLEAR_OLD_FW_BEGIN, 0, blc_ota_getNextFirmwareStartAddress() + ota_firmware_max_size);
    }
#endif
    for(int i = 0; i < sec_num ;i++)
    {
        flash_erase_sector(Addr + i * 0x1000);
    }
#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb && blotaSvr.fw_area_unlock) {
        flash_prot_op_cb(FLASH_OP_EVT_STACK_OTA_CLEAR_OLD_FW_END, 0, 0);
    }
    #endif
    return TAG_ERROR_NONE;
}

void PortFwUpdateInitCb(void)
{
    TAG_LOG_I("PortFwUpdateInitCb");
    /* empty */
}

void PortFwUpdateStartCb(void)
{
    TAG_LOG_I("PortFwUpdateStartCb");
    portble_fw_status = true;
#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb) {
        flash_prot_op_cb(FLASH_OP_EVT_STACK_OTA_WRITE_NEW_FW_BEGIN, 0, blc_ota_getNextFirmwareStartAddress() + ota_firmware_max_size);
    }
#endif
    /* empty */
}

static void systemResetCb(void)
{
    TagUtilSystemReset(4, NULL);
}

void blt_ota_writeBootMark(void)
{
    u8 flag = BOOT_MARK_VALUE_1_BYTE;
    flash_write_page(portble_ota_program_offset + BOOT_MARK_ADDR, 1, (u8 *)&flag);                              //Set FW flag
    flag = 0;
    flash_write_page((portble_ota_program_offset ? 0 : ota_program_bootAddr) + BOOT_MARK_ADDR, 1, (u8 *)&flag); //Invalid flag
}

void PortFwUpdateSuccessCb(void *param)
{
    TAG_LOG_I("PortFwUpdateSuccessCb");
    portble_fw_status = false;
    blt_ota_writeBootMark();
    /* empty */
    TagPutPostWork(systemResetCb, NULL);
}

void PortFwUpdateFailedCb(void)
{
    portble_fw_status = false;
    TAG_LOG_I("PortFwUpdateFailedCb");
    /* empty */
}

bool PortFwUpdateStatus(void)
{
    TAG_LOG_I("PortFwUpdateStatus");
    return portble_fw_status;
    /* empty */
}

void PortFwUpdateEndCb(void)
{
    portble_fw_status = false;
#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
/* do it before reboot*/
if (flash_prot_op_cb && blotaSvr.fw_area_unlock) {
    flash_prot_op_cb(FLASH_OP_EVT_STACK_OTA_WRITE_NEW_FW_END, 0, 0);
}
#endif
    TAG_LOG_I("PortFwUpdateEndCb");
    /* empty */
}

#endif // TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE
