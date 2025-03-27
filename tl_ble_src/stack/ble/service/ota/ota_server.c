/********************************************************************************************************
 * @file    ota_server.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd.
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

#if OS_SUP_EN
#include "stack/ble/os_sup/os_sup.h"
#include "stack/ble/os_sup/os_sup_stack.h"
#endif

#include "ota.h"
#include "ota_stack.h"
#include "ota_server.h"




_attribute_ble_data_retention_  ota_server_t        blotaSvr;

_attribute_ble_data_retention_  ota_startCb_t       otaStartCb = NULL;
_attribute_ble_data_retention_  ota_versionCb_t     otaVersionCb = NULL;
_attribute_ble_data_retention_  ota_resIndicateCb_t otaResIndicateCb = NULL;


_attribute_ble_data_retention_  ota_write_fw_callback_t ota_write_fw_cb = NULL;


#if (HARDWARE_SECURE_BOOT_SUPPORT_EN)
    _attribute_ble_data_retention_  ota_security_callback_t ota_sec_boot_cb = NULL;
#endif

#if (HARDWARE_FIRMWARE_ENCRYPTION_SUPPORT_EN)
    _attribute_ble_data_retention_  ota_security_callback_t ota_encryption_cb = NULL; //read & write with firmware encryption
#endif


/*
 * old data tested on Kite by SiHui:
 * for 16 Bytes data input, calculate in half byte
 * crc32_half_tbl on SRAM,  cost 80uS timing and 64 Bytes SRAM
 * crc32_half_tbl on FLASH, cost 136uS
 * so choose SRAM
 */
//static const unsigned long crc32_half_tbl[16] = {
_attribute_ble_data_retention_  static unsigned long crc32_half_tbl[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};


_attribute_no_inline_
void blt_ota_reset(void)
{
    blotaSvr.ota_step = OTA_STEP_IDLE;
    blotaSvr.ota_busy = 0;
    blotaSvr.pdu_len = 16;
    blotaSvr.version_accept = 0;
    blotaSvr.last_adr_index = ota_firmware_max_size/16; //default: max_size div 16
    blotaSvr.flow_mask = 0;
    //blotaSvr.resume_mode = 0;
    blotaSvr.data_pending_type = 0;
    blotaSvr.cur_adr_index = -1;
    blotaSvr.flash_addr_mark = -1;
    blotaSvr.fw_check_match = 0;
    blotaSvr.process_timeout_100S_cnt = 0;
    blotaSvr.ota_start_tick = 0;
    blotaSvr.data_packet_tick = 0;
    blotaSvr.ota_connHandle = 0;
    blotaSvr.ota_write_attHandle = 0;
    blotaSvr.otaResult = OTA_SUCCESS;

    /* reset scheduler indication parameters, so if OTA result is coming, scheduler indication packet is abandoned */
    blotaSvr.schdl_pduNum_mark = 0;
    blotaSvr.schdl_pduNum_rpt = 0;

#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    blotaSvr.fw_area_unlock = 0;
#endif
}



/**
 * @brief      this function is used for user to initialize OTA server module.
 * @param      none
 * @return     none
 */
_attribute_no_inline_
void blc_ota_initOtaServer_module(void)
{
    blotaSvr.otaInit = 1;

    #if (MCU_SUPPORT_MULTI_PRIORITY_IRQ)  //MCU support multiple priority interrupt, like B91, B92...
        #if (GD_FLASH_SUPPORTED_ON_MULTI_PRIORITY_IRQ_IC)  //add GD flash support
            if(blc_flash_vendor == FLASH_ETOX_GD){
                blotaSvr.write_16B_each_time = 1;
            }
        #endif
    #else  //MCU do not support multiple priority, like B85m(Kite/Vulture), B80(Eaglet)
        blotaSvr.write_16B_each_time = 1;
    #endif


    host_ota_main_loop_cb = blt_ota_server_main_loop;
    host_ota_terminate_cb = blt_ota_server_terminate;


    /* differ code mark attention: different code from single priority IRQ IC(e.g. B85m) for some special Flash
     * SONOS_ARCH_FLASH_ON_SINGLE_PRIORITY_IRQ_IC_WORKAROUND_EN */


    //global status, only reset in user initialization stage
    blotaSvr.fw_check_en = 1; //default firmware check is must
    blotaSvr.ota_timeout_enable = 1;
    blotaSvr.fw_crc_default = 0xFFFFFFFF;
    blotaSvr.process_timeout_100S_num = 0;
    blotaSvr.process_timeout_us = 30 * 1000000;   //default 30 S
    blotaSvr.packet_timeout_us = 5 * 1000000;     //default 5 S


    //OTA flow status, should reset for each new OTA flow
    blt_ota_reset();


    /* special design for potential EFUSE read error when initialization
     * must before "bls_ota clearNewFwDataArea" */
        if(0){
        }
    #if (HARDWARE_SECURE_BOOT_SUPPORT_EN)
        else if(ota_sec_boot_cb){
            ota_sec_boot_cb(OSB_TYPE_CHECK_SEC_INFO, 0, 0, NULL);  //blt_ota_secure_boot_process
        }
    #endif
    #if (HARDWARE_FIRMWARE_ENCRYPTION_SUPPORT_EN)
        else if(ota_encryption_cb){
            ota_encryption_cb(OERW_TYPE_CHECK_SEC_INFO, 0, 0, NULL);  //blt_ota_encryption_process
        }
    #endif


    if(!blotaSvr.newFwArea_clear){
        bls_ota_clearNewFwDataArea(); //must
    }

    tlkapi_send_string_data(DBG_OTA_FLOW, "OTA server init", 0, 0);
}



/* hidden API, if user do not use firmware check, call  this API to disable it. */
void blc_ota_setOtaFirmwareCheckEnable(int en)
{
    blotaSvr.fw_check_en = en;
}

/* hidden API, if user do not want share same firmware check algorithm with all other customers,
 * he can call this API change crc32 init_value, pay attention that TestBench need also corresponding process. */
void blc_ota_setFirmwareCheckCrcInitValue(u32 crc_init_value){
    blotaSvr.fw_crc_default = crc_init_value; //attention: not set fw_crc_init !!!
}




/**
 * @brief      This function is used to set OTA timeout enable
 *             attention 1: hidden API, do not show on SDK, but can introduced in Handbook.
 *             attention 2: OTA timeout control is very important, so it's enable by default.
 *                          User can disable it by setting "timeout_en" to 0
 *             attention 3: If this API is used, must be called after "blc_ota_initOtaServer_module" when initialization !!!
 * @param[in]  timeout_en - OTA time out control enable or disable
 * @return     none
 */
void blc_ota_setOtaTimeoutEnable(int timeout_en)
{
    blotaSvr.ota_timeout_enable = timeout_en;
}



/* this function must be called before "sys_init" or "cpu_wakeup_init".*/
ble_sts_t blc_ota_setFirmwareSizeAndBootAddress(int firmware_size_k, multi_boot_addr_e boot_addr)
{
    int param_valid = 0;

    if( (firmware_size_k & 3) == 0)
    {
        if( boot_addr == MULTI_BOOT_ADDR_0x20000 && firmware_size_k <= 124){
            param_valid = 1;
        }
        else if(boot_addr == MULTI_BOOT_ADDR_0x40000 && firmware_size_k <= 252){
            param_valid = 1;
        }
        /*not use MULTI_BOOT_ADDR_0x80000 here, in case some MCU do not support 0x80000 boot */
        else if(boot_addr == 0x80000 && firmware_size_k <= 508){
            param_valid = 1;
        }
    }


    if(param_valid){
        ota_firmware_max_size = firmware_size_k<<10; //*1024
        ota_program_bootAddr  = boot_addr;
        return BLE_SUCCESS;
    }
    else{
        return SERVICE_ERR_INVALID_PARAMETER;
    }
}


u32  blc_ota_getCurrentUsedMultipleBootAddress(void)
{
    return ota_program_bootAddr;
}

u32 blc_ota_getCurrentFirmwareStartAddress(void)
{
    if(ota_program_offset){
        return 0;
    }
    else{
        return ota_program_bootAddr;
    }
}


u32 blc_ota_getNextFirmwareStartAddress(void)
{
    return ota_program_offset;
}


void blc_ota_setFirmwareVersionNumber(u16 version_number)
{
    blotaSvr.local_version_num = version_number;
}



void blc_ota_registerOtaStartCmdCb(ota_startCb_t cb)
{
    otaStartCb = cb;
}

void blc_ota_registerOtaFirmwareVersionReqCb(ota_versionCb_t cb)
{
    otaVersionCb = cb;
}

void blc_ota_registerOtaResultIndicationCb(ota_resIndicateCb_t cb)
{
    otaResIndicateCb = cb;
}

_attribute_no_inline_
ble_sts_t blc_ota_setOtaProcessTimeout(int timeout_second)
{
    if(timeout_second > 4 && timeout_second < 1001){
        blotaSvr.process_timeout_100S_num = timeout_second/100;
        blotaSvr.process_timeout_us  = (timeout_second%100)*1000000;

        return BLE_SUCCESS;
    }
    else{
        return SERVICE_ERR_INVALID_PARAMETER;
    }
}

_attribute_no_inline_
ble_sts_t blc_ota_setOtaDataPacketTimeout(int timeout_second)
{
    if(timeout_second > 0 && timeout_second < 21){
        blotaSvr.packet_timeout_us = timeout_second * 1000000;
        return BLE_SUCCESS;
    }
    else{
        return SERVICE_ERR_INVALID_PARAMETER;
    }
}


/**
 * @brief      This function is used to set resolution of OTA schedule indication by PDU number
 * @param[in]  pdu_num -
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
_attribute_no_inline_
ble_sts_t blc_ota_setOtaScheduleIndication_by_pduNum(int pdu_num)
{
    /* can only select one from PDU number & FW size */
    if(blotaSvr.schedule_fw_size){
        return SERVICE_ERR_INVALID_PARAMETER;
    }
    else{
        blotaSvr.schedule_pdu_num = pdu_num;
        return BLE_SUCCESS;
    }
}



//Hidden API, release later
/**
 * @brief      This function is used to set resolution of OTA schedule indication by firmware size
 * @param[in]  fw_size -
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
_attribute_no_inline_
ble_sts_t blc_ota_setOtaScheduleIndication_by_FirmwareSize(int fw_size)
{
    /* can only select one from PDU number & FW size */
    if(blotaSvr.schedule_pdu_num){
        return SERVICE_ERR_INVALID_PARAMETER;
    }
    else{
        blotaSvr.schedule_fw_size = fw_size;
        return BLE_SUCCESS;
    }
}



void      blc_ota_setAttHandleOffset(s8 attHandle_offset)
{
    blotaSvr.handle_offset = attHandle_offset;
}



__attribute__((weak))  // The customer application layer overwrites this function
void blt_ota_writeBootMark(void)
{
    u8 flag = BOOT_MARK_VALUE_1_BYTE;
    flash_write_page(ota_program_offset + BOOT_MARK_ADDR, 1, (u8 *)&flag);      //Set FW flag
    flag = 0;
    flash_write_page((ota_program_offset ? 0 : ota_program_bootAddr) + BOOT_MARK_ADDR, 1, (u8 *)&flag); //Invalid flag
}




_attribute_no_inline_
int ota_save_data(u32 flash_addr, int len, u8 * data)
{
    if(flash_addr <= BOOT_MARK_ADDR && (flash_addr + len) > BOOT_MARK_ADDR){
        u8 offset = BOOT_MARK_ADDR - flash_addr;
        u8 vendor_fw_mark[4] = {0x4B, 0x4E, 0x4C, 0x54};

        if(memcmp(data + offset, vendor_fw_mark, 4)){  //do not equal
            return OTA_FIRMWARE_MARK_ERR;
        }

        data[offset] = 0xFF;
    }

    u32 real_flash_addr = ota_program_offset + flash_addr;

    if(blotaSvr.write_16B_each_time){
        for(int i=0; i<len; i+=16){
            flash_write_page(real_flash_addr + i, 16, data + i);  //GD flash, 16B, 16M clock:162 uS;  32M clock: 133uS

            u8 flash_check[16];
            flash_read_page(real_flash_addr + i, 16, flash_check);

            if(memcmp(flash_check, data + i, 16)){  //do not equal
                return OTA_WRITE_FLASH_ERR;
            }
        }
    }
    else{
        flash_write_page(real_flash_addr, len, data);

        u8 flash_check[240];  //biggest value 240
        flash_read_page(real_flash_addr, len, flash_check);

        if(memcmp(flash_check, data, len)){  //do not equal
            return OTA_WRITE_FLASH_ERR;
        }
    }

    return OTA_SUCCESS;
}








_attribute_no_inline_ int otaWrite(u16 connHandle, void * p)
{
    if(!blotaSvr.otaInit){
        return 1;
    }

    blotaSvr.ota_busy = 1;  //any OTA command coming triggers OTA busy, only clear it in OTA_STEP_FINISH

    /* previous OTA result process not finished, can not process another OTA flow */    //TODO


    /* OTA error has happened, do not process anymore */
    if(blotaSvr.otaResult || blotaSvr.ota_step == OTA_STEP_FINISH){
        return 1;
    }

    
    #if (BLE_MULTIPLE_CONNECTION_ENABLE)
        if(blotaSvr.ota_connHandle && connHandle != blotaSvr.ota_connHandle){
            return 1;
        }
        else if(!blotaSvr.ota_connHandle){
            blotaSvr.ota_connHandle = connHandle;
        }
    #else
        blotaSvr.ota_connHandle = BIT(6); //BLS_CONN_HANDLE;
    #endif


    rf_packet_att_data_t *pAttDat = (rf_packet_att_data_t*)p;
    blotaSvr.ota_cmd_adr =  pAttDat->dat[0] | (pAttDat->dat[1]<<8);


    if(!blotaSvr.ota_write_attHandle){
        blotaSvr.ota_write_attHandle = pAttDat->handle;  //record att_handle
        blotaSvr.ota_notify_attHandle = blotaSvr.ota_write_attHandle + blotaSvr.handle_offset;
    }

    int err_flag = OTA_SUCCESS;
    /* 1. OTA command */
    if(blotaSvr.ota_cmd_adr >= CMD_OTA_VERSION)
    {
        /* 1.1 old version command, no not change it, maybe some customers used this */
        if(blotaSvr.ota_cmd_adr == CMD_OTA_VERSION)
        {
            if(otaVersionCb){
                otaVersionCb();
            }
        }
        /* 1.2 version_req_ext.  if version compare enable, record compare result*/
        else if(blotaSvr.ota_cmd_adr == CMD_OTA_FW_VERSION_REQ)
        {
            /* If version compare fail, but peer device do not send OTA start command, OTA flow do not give any OTA result, no need disconnect */
            if(blotaSvr.flow_mask){ //previous OTA flow not finish
                err_flag = OTA_FLOW_ERR;
            }
            else{
                ota_versionReq_t *pVerReq = (ota_versionReq_t *)pAttDat->dat;

                blotaSvr.version_accept = 1;
                if(pVerReq->version_compare && pVerReq->version_num <= blotaSvr.local_version_num){
                    blotaSvr.version_accept = 0;
                }

                if(blt_ota_pushVersionRsp() != BLE_SUCCESS){
                    blotaSvr.data_pending_type = DATA_PENDING_VERSION_RSP;
                }

                tlkapi_send_string_data((stkLog_mask & STK_LOG_OTA_FLOW), "[OTA][FLW] ota FW version REQ", pVerReq, sizeof(ota_versionReq_t));
            }
        }
        /* 1.3 old OTA start command, trigger short PDU(16Byte) only */
        else if(blotaSvr.ota_cmd_adr == CMD_OTA_START || blotaSvr.ota_cmd_adr == CMD_OTA_START_EXT)
        {

            tlkapi_send_string_u32s(DBG_OTA_FLOW, "OTA start", pAttDat->handle);



            /* previous OTA flow not finish */
            if(blotaSvr.flow_mask){
                err_flag = OTA_FLOW_ERR;
            }
            else if(blotaSvr.ota_cmd_adr == CMD_OTA_START_EXT){
                ota_startExt_t *pStartExt = (ota_startExt_t *)pAttDat->dat;
                if( pStartExt->version_compare && !blotaSvr.version_accept ){
                    err_flag = OTA_VERSION_COMPARE_ERR;
                }
                else if( (pStartExt->pdu_length & 0x0F) != 0 || pStartExt->pdu_length == 0){//not 16*n, or 0
                    err_flag = OTA_PDU_LEN_ERR;
                }
                /* differ code mark attention: different code from B85m SDK, some MCU have special limitation about PDU_length */
                else{
                    blotaSvr.pdu_len = pStartExt->pdu_length;
                }

                tlkapi_send_string_data(DBG_OTA_FLOW, "start ext", &blotaSvr.pdu_len, 1);
                tlkapi_send_string_data((stkLog_mask & STK_LOG_OTA_FLOW), "[OTA][FLW] ota start extended", pStartExt, 4);
            }
            else{
                tlkapi_printf((stkLog_mask & STK_LOG_OTA_FLOW), "[OTA][FLW] ota start legacy\n");
            }

            #if (HARDWARE_SECURE_BOOT_SUPPORT_EN)
                if(ota_sec_boot_cb && !err_flag){
                    err_flag = ota_sec_boot_cb(OSB_TYPE_START, 0, 0, NULL);  //blt_ota_secure_boot_process
                }
            #endif

            if(!err_flag){
                /* OTA status clear and re_set */
                blotaSvr.flow_mask |= OTA_FLOW_START;
                blotaSvr.ota_start_tick = clock_time() | 1;  //mark time
                blotaSvr.process_timeout_100S_cnt = blotaSvr.process_timeout_100S_num;
                blotaSvr.cur_adr_index = -1;
                blotaSvr.fw_check_match = 0;

                if(blotaSvr.schedule_pdu_num){
                    blotaSvr.schdl_pduNum_mark = blotaSvr.schedule_pdu_num; //set first mark value
                }

                /* differ code mark attention: different code from single priority IRQ IC(e.g. B85m) for some special Flash
                 * ZBIT_FLASH_ON_SINGLE_PRIORITY_IRQ_IC_WORKAROUND_EN */

                /* differ code mark attention: different code from single priority IRQ IC(e.g. B85m) for some special Flash
                 * SONOS_ARCH_FLASH_ON_SINGLE_PRIORITY_IRQ_IC_WORKAROUND_EN */

                /* OTA start callback triggers only no err_flag*/
                if(otaStartCb){
                    otaStartCb();
                }
            }

        }
        /* 1.4 OTA END*/
        else if(blotaSvr.ota_cmd_adr == CMD_OTA_END)
        {
            /* 1.4.1 no OTA start & OTA valid data before OTA end */
            if( (blotaSvr.flow_mask & (OTA_FLOW_START | OTA_FLOW_VALID_DATA)) == 0 )
            {
                err_flag = OTA_FLOW_ERR;
            }
            else{
                ota_end_t *pEnd = (ota_end_t *)pAttDat->dat;

                /* if no index_max check, set OTA success directly, otherwise we check if any index_max match */
                if( (pEnd->adr_index_max ^ pEnd->adr_index_max_xor) == 0xFFFF){  //index_max valid, we can check
                    if(pEnd->adr_index_max != blotaSvr.cur_adr_index){  //last one or more packets missed
                        err_flag = OTA_DATA_INCOMPLETE;
                    }
                }

                #if (HARDWARE_SECURE_BOOT_SUPPORT_EN)
                    if(ota_sec_boot_cb){
                        err_flag = ota_sec_boot_cb(OSB_TYPE_END, 0, 0, NULL);  //blt_ota_secure_boot_process
                    }
                #endif

                #if (BLE_OTA_FW_CHECK_EN)
                    if(blotaSvr.fw_check_en && !blotaSvr.fw_check_match){
                        err_flag = OTA_FW_CHECK_ERR;
                    }
                #endif

                    tlkapi_printf((stkLog_mask & STK_LOG_OTA_FLOW), "[OTA][FLW] ota end\n");
            }

            blt_ota_setResult(OTA_STEP_FEEDBACK, err_flag); //set result no matter OTA success or fail

            return 0; //must return here, or "blt_ota_setResult" will execute again at the end of this function
        }
     #if (HARDWARE_SECURE_BOOT_SUPPORT_EN)
        else if(blotaSvr.ota_cmd_adr >= CMD_OTA_SB_PUBKEY_SIGN_MIN && blotaSvr.ota_cmd_adr <= CMD_OTA_SB_PUBKEY_SIGN_MAX)
        {
            if(ota_sec_boot_cb){
                err_flag = ota_sec_boot_cb(OSB_TYPE_PUBKEY_SIGN, 0, 0, pAttDat);  //blt_ota_secure_boot_process
            }
            else{
                err_flag = OTA_SECBOOT_FUNC_NOT_ENABLE;
            }
        }
     #endif
        /* invalid OTA command */
        else
        {
            err_flag = OTA_PACKET_INVALID;
        }
    }
    /* 2. OTA valid data */
    else if(blotaSvr.ota_cmd_adr <= blotaSvr.last_adr_index)
    {
        if(blotaSvr.ota_cmd_adr == 0){
            tlkapi_send_string_data((stkLog_mask & STK_LOG_OTA_FLOW), "[OTA][DAT] first ota data", pAttDat->dat, 18);
        }


        //blotaSvr.flow_mask |= OTA_FLOW_DATA_COME;
        int ota_actual_pdu_len;
        if(blotaSvr.ota_cmd_adr < blotaSvr.last_adr_index){
            ota_actual_pdu_len = blotaSvr.pdu_len;
        }
        else{
            ota_actual_pdu_len = blotaSvr.last_actual_pdu_len;
        }

        /* 2.1 no OTA start */
        if( !(blotaSvr.flow_mask & OTA_FLOW_START) )
        {
            err_flag = OTA_FLOW_ERR;
        }
        /* 2.2 no FW size, FW size on 0x00018, so choose adr_index = 2 to check is OK */
        else if(blotaSvr.ota_cmd_adr == 2 && !(blotaSvr.flow_mask & OTA_FLOW_GET_SIZE))
        {
            err_flag = OTA_FW_SIZE_ERR;
        }
        /* 2.3 OTA PDU not match with define in OTA_START_EXT or last PDU not correct */
        else if(pAttDat->l2cap != (ota_actual_pdu_len + 4 + 3)) //4: adr_index(2) + CRC(2); 3: opcode(1) + attHandle(2)
        {
            err_flag = OTA_PDU_LEN_ERR;
        }
        /* 2.4 adr_index err, repeated OTA PDU or lost some OTA PDU */
        else if(blotaSvr.ota_cmd_adr != blotaSvr.cur_adr_index + 1)
        {
            err_flag = OTA_DATA_PACKET_SEQ_ERR;
        }
        else
        {
            //DBG_C HN9_HIGH;
            /* 16M clock, function in RamCode, OTA PDU max 240 Byte, 1400 uS
             * TODO: use lookup table method to save time, just like crc32 */
            u16 crc16_cal = blt_Crc16ComputeInternal(pAttDat->dat, ota_actual_pdu_len + 2);
            //DBG_C HN9_LOW;
            u16 crc16_rcv = (pAttDat->dat[ota_actual_pdu_len + 3]<<8) | pAttDat->dat[ota_actual_pdu_len + 2];

            #if 0  //debug
                if(blotaSvr.ota_cmd_adr == blotaSvr.last_adr_index){
                    tlkapi_send_string_u32s(DBG_OTA_FLOW, "CRC16 last", crc16_cal, crc16_rcv, 0, 0);
                }
            #endif

            /* 2.5 CRC16 error */
            if(crc16_cal != crc16_rcv)
            {
                err_flag = OTA_DATA_CRC_ERR;
            }
            else
            {
                    /***************************************** OTA Data Process  *************************************************/
                    int flash_addr = blotaSvr.ota_cmd_adr*blotaSvr.pdu_len;
                    u8  *pFwDat = pAttDat->dat + 2;

                    if(flash_addr <= FW_SIZE_ADDR && (flash_addr + blotaSvr.pdu_len) > FW_SIZE_ADDR){  //firmware_size packet
                        u8 offset = FW_SIZE_ADDR - flash_addr;
                        u32 fw_size = pFwDat[offset] | pFwDat[offset+1] <<8 | pFwDat[offset+2]<<16 | pFwDat[offset+3]<<24;

                        if(fw_size < FW_MIN_SIZE || fw_size > ota_firmware_max_size){
                            err_flag = OTA_FW_SIZE_ERR;
                        }
                        else if(blotaSvr.fw_check_en && (fw_size & 0x0f) != 4 ){  //firmware check: size must be 16*n + 4
                            err_flag = OTA_FW_SIZE_ERR;
                        }
                        else{
                            blotaSvr.firmware_size_byte = fw_size;
                            blotaSvr.last_adr_index = (fw_size - 1)/blotaSvr.pdu_len;  //-1 is important
                            blotaSvr.last_valid_pdu_len = fw_size % blotaSvr.pdu_len;
                            blotaSvr.align16_makeup_len = 16 - (blotaSvr.last_valid_pdu_len & 15); //only make up to make sure 16B aligned(compatible with old protocol)
                            if(blotaSvr.align16_makeup_len == 16){
                                blotaSvr.align16_makeup_len = 0;
                            }
                            blotaSvr.last_actual_pdu_len = blotaSvr.last_valid_pdu_len + blotaSvr.align16_makeup_len;
                            blotaSvr.last_pdu_crc_offset = (blotaSvr.last_valid_pdu_len & 0xF0);
                            blotaSvr.flow_mask |= OTA_FLOW_GET_SIZE;

                            tlkapi_send_string_u32s(DBG_OTA_FLOW, "FW size", fw_size, blotaSvr.last_valid_pdu_len, blotaSvr.last_actual_pdu_len, blotaSvr.last_pdu_crc_offset);

                            #if (HARDWARE_FIRMWARE_ENCRYPTION_SUPPORT_EN)
                                u8 fwEnc_value = pFwDat[FIRMWARE_ENCRYPTION_FLAG_ADDR - flash_addr];
                                int fwEnc_enable = fwEnc_value == 0x5A;  //attention: cstartup.S
                                if((ota_encryption_cb && !fwEnc_enable) || (!ota_encryption_cb && fwEnc_enable)){
                                    err_flag = OTA_FWENC_NEW_FW_NOT_MATCH_OLD_FW;
                                }
                            #endif

                            #if (HARDWARE_SECURE_BOOT_SUPPORT_EN)
                                u8 secBoot_value = pFwDat[SECURE_BOOT_FLAG_ADDR - flash_addr];
                                int secBoot_enable = secBoot_value == 0x5A;  //attention: cstartup.S
                                if((ota_sec_boot_cb && !secBoot_enable) || (!ota_sec_boot_cb && secBoot_enable)){
                                    err_flag = OTA_SECBOOT_NEW_FW_NOT_MATCH_OLD_FW;
                                }
                            #endif

                            #if (APP_FLASH_PROTECTION_ENABLE)
                                u8 flashProtec_value = pFwDat[FIRMWARE_FLASH_PROTECTION_FLAG_ADDR - flash_addr];
                                int flashProtec_enable = flashProtec_value == 0x5A;  //attention: cstartup.S
                                if((flash_prot_op_cb && !flashProtec_enable)){
                                    err_flag = OTA_FW_FLASH_PROT_NEW_FW_NOT_MATCH_OLD_FW;
                                }
                            #endif
                        }
                    }


                    if(err_flag == OTA_SUCCESS)
                    {

                        #if (BLE_OTA_FW_CHECK_EN)
                            if(blotaSvr.fw_check_en)
                            {
                                if(blotaSvr.ota_cmd_adr == 0){  //first PDU
                                    blotaSvr.fw_crc_init = blotaSvr.fw_crc_default;  //CRC_init recover
                                }

                                u32 fw_check_value = 0;
                                int ota_fw_crc_len = blotaSvr.pdu_len;
                                if(blotaSvr.ota_cmd_adr == blotaSvr.last_adr_index){ //last adr_index
                                    ota_fw_crc_len = blotaSvr.last_pdu_crc_offset;  //maybe 0x00/0x10/0x20 ..
                                    fw_check_value = pFwDat[blotaSvr.last_pdu_crc_offset] | pFwDat[blotaSvr.last_pdu_crc_offset+1] <<8 | pFwDat[blotaSvr.last_pdu_crc_offset+2]<<16 | pFwDat[blotaSvr.last_pdu_crc_offset+3]<<24;

                                    tlkapi_send_string_u32s(DBG_OTA_FLOW, "FW last", ota_actual_pdu_len, ota_fw_crc_len, fw_check_value);
                                }

                                if(ota_fw_crc_len){
                                    #if 0  //do not need now
                                        blotaSvr.fw_crc_init = crc32_cal(blotaSvr.fw_crc_init, ota_dat, crc32_tbl, ota_fw_crc_len);
                                    #else
                                        u8 ota_dat[240*2];  //maximum 240 Bytes
                                        for(int i=0; i<ota_fw_crc_len; i++){
                                            ota_dat[i*2]   = pFwDat[i] & 0x0f;
                                            ota_dat[i*2+1] = pFwDat[i]>>4;
                                        }
                                        //tlkapi_send_string_data(DBG_OTA_FLOW, "FW CRC", &blotaSvr.fw_crc_init, 4);
                                        //DBG_C HN10_HIGH;
                                        /* 16M clock, OTA PDU maximum length 240 Byte, 390 uS*/
                                        blotaSvr.fw_crc_init = crc32_half_cal(blotaSvr.fw_crc_init, ota_dat, (unsigned long* )crc32_half_tbl, ota_fw_crc_len*2);
                                        //DBG_C HN10_LOW;
                                    #endif
                                }


                                if(blotaSvr.ota_cmd_adr == blotaSvr.last_adr_index){
                                    if(fw_check_value == blotaSvr.fw_crc_init){  //CRC32 match
                                        blotaSvr.fw_check_match = 1;
                                    }
                                    else{
                                        err_flag = OTA_FW_CHECK_ERR;
                                    }

                                    tlkapi_send_string_u32s(DBG_OTA_FLOW, "FW check", blotaSvr.ota_cmd_adr, fw_check_value, blotaSvr.fw_crc_init, blotaSvr.fw_check_match);
                                }
                            }
                        #endif


                        if(err_flag == OTA_SUCCESS){
                          //blotaSvr.flash_addr_mark = flash_addr;
                            blotaSvr.flash_addr_mark = flash_addr + ota_actual_pdu_len;  //important for "+ pdu_len"


                                if(blotaSvr.ota_cmd_adr == 0)
                                {
                                    #if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
                                        if(flash_prot_op_cb){
                                            /* ota write data, destroy old boot mark, so 0 ~ ota_program_bootAddr + ota_firmware_max_size */
                                            flash_prot_op_cb(FLASH_OP_EVT_STACK_OTA_WRITE_NEW_FW_BEGIN, 0, ota_program_bootAddr + ota_firmware_max_size);
                                            blotaSvr.fw_area_unlock = 1;
                                        }
                                    #endif

                                    /* differ code mark attention: different code from single priority IRQ IC(e.g. B85m) for some special Flash
                                     * ZBIT_FLASH_ON_SINGLE_PRIORITY_IRQ_IC_WORKAROUND_EN */
                                }




                            tlkapi_send_string_data((stkLog_mask & STK_LOG_OTA_DATA), "[OTA][DAT] ota data", pAttDat->dat, 18);

                            /* Very special logic here, can not change without SiHui's evaluation
                             * 1. for MCU not support secure boot & FW encryption, go "else" branch
                             * 2. for MCU support secure boot & FW encryption:
                             *    2.1. if customer select "secure boot" function, go "ota_sec_boot_cb",  this branch will process
                             *         signature verification and FW encryption(depend on if customer select "FW enc") together
                             *    2.2 if customer select "FW enc" only, not select "secure boot", go "ota encryption_cb", this branch
                             *         will process FW encryption.
                             *         Signature verification will cost big size of flash code, so we use a special branch "ota encryption_cb"
                             *         to process FW encryption, to save a lot of code under this situation
                             *    2.3 never consider "ota_write_fw_cb", this is a design
                             *        for PUYA flash(write a byte cost 2 mS) on MCU don not support MULTI_PRIORITY_IRQ(Kite/Vulture/Eaglet)
                             */
                                if(0)
                                {

                                }
                            #if (HARDWARE_SECURE_BOOT_SUPPORT_EN)
                                else if(ota_sec_boot_cb)
                                {
                                    err_flag = ota_sec_boot_cb(OSB_TYPE_SAVE_DATA, flash_addr, ota_actual_pdu_len, pAttDat->dat + 2);  //blt_ota_secure_boot_process
                                }
                            #endif
                            #if (HARDWARE_FIRMWARE_ENCRYPTION_SUPPORT_EN)
                                else if(ota_encryption_cb) //attention: FW_ENC enable & SEC_BOOT disable, goes here
                                {
                                    err_flag = ota_encryption_cb(OERW_TYPE_SAVE_DATA, flash_addr, ota_actual_pdu_len, pAttDat->dat + 2); //blt_ota_encryption_process
                                }
                            #endif
                                else
                                {
                                    if(ota_write_fw_cb){ //for PUYA flash(write a byte cost 2 mS) on MCU don not support MULTI_PRIORITY_IRQ
                                        err_flag = ota_write_fw_cb (flash_addr, ota_actual_pdu_len, pAttDat->dat + 2);  //ota_sonos_flash_write_fw
                                    }
                                    else{
                                        err_flag = ota_save_data (flash_addr, ota_actual_pdu_len, pAttDat->dat + 2);
                                    }
                                }
                        }
                    }
                    /*********************************************************************************************************************/
            }



            if(err_flag == OTA_SUCCESS){ //update current adr_index only when no error happen
                blotaSvr.cur_adr_index = blotaSvr.ota_cmd_adr;
                blotaSvr.flow_mask |= OTA_FLOW_VALID_DATA;
                blotaSvr.data_packet_tick = clock_time() | 1;

                /* OTA schedule indication */
                if(blotaSvr.schedule_pdu_num){
                    u16 cur_pdu_num = blotaSvr.ota_cmd_adr + 1;
                    if(blotaSvr.schdl_pduNum_mark == cur_pdu_num){
                        /* report PDU number with handleValueNotify, if old indication packet is not send OK, replace it
                         * with new indication, so for master, it may not see continuous indication due to RF block */
                        blotaSvr.schdl_pduNum_rpt = cur_pdu_num;
                        blotaSvr.schdl_pduNum_mark += blotaSvr.schedule_pdu_num; //update new mark
                    }
                }
            }
        }
    }
    /* 3. invalid packet */
    else
    {
        err_flag = OTA_PACKET_INVALID;
    }



    if(err_flag){
        blt_ota_setResult(OTA_STEP_FEEDBACK, err_flag);
    }


    return 0;  //attention: can not return 1
}





_attribute_no_inline_
void blt_ota_setResult(int next_step, int result)
{
    blotaSvr.otaResult = result;
    blotaSvr.ota_step = next_step;
    blotaSvr.ota_start_tick = 0; //disable OTA timeout trigger
    blotaSvr.process_timeout_100S_cnt = 0;
    blotaSvr.data_packet_tick = 0;

    if(next_step == OTA_STEP_FEEDBACK){
        blotaSvr.feedback_begin_tick = clock_time();
        blotaSvr.data_pending_type = DATA_PENDING_OTA_RESULT;  //if version_rsp pending, overwrite it
    }
    else if(next_step == OTA_STEP_FINISH){ //just feedback step
        blotaSvr.feedback_begin_tick = 0;
        blotaSvr.data_pending_type = 0;
    }

    tlkapi_send_string_u8s(DBG_OTA_FLOW, "OTA result", result, next_step);

    tlkapi_printf((stkLog_mask & STK_LOG_OTA_FLOW), "[OTA][FLW] ota result: 0x%x\n", blotaSvr.otaResult);
}




_attribute_no_inline_
void blt_ota_procTimeout(void)
{
    if(blotaSvr.process_timeout_100S_cnt && clock_time_exceed(blotaSvr.ota_start_tick , 100*1000*1000)){
        blotaSvr.process_timeout_100S_cnt--;
        blotaSvr.ota_start_tick = clock_time() | 1;
    }
    else if(!blotaSvr.process_timeout_100S_cnt){
        if(blotaSvr.ota_start_tick && clock_time_exceed(blotaSvr.ota_start_tick , blotaSvr.process_timeout_us)){  //OTA timeout
            blt_ota_setResult(OTA_STEP_FEEDBACK, OTA_TIMEOUT);
        }
    }

    /* data packet interval timeout */
    if(blotaSvr.data_packet_tick && clock_time_exceed(blotaSvr.data_packet_tick, blotaSvr.packet_timeout_us)){
        blt_ota_setResult(OTA_STEP_FEEDBACK, OTA_DATA_PACKET_TIMEOUT);
    }

    /* OTA schedule indication */
    //TODO
}


_attribute_no_inline_
ble_sts_t blt_ota_pushVersionRsp(void)
{
    u8 temp_buffer[sizeof(ota_versionRsp_t)];
    ota_versionRsp_t *pVerRsp = (ota_versionRsp_t *)temp_buffer;
    pVerRsp->ota_cmd = CMD_OTA_FW_VERSION_RSP;
    pVerRsp->version_num = blotaSvr.local_version_num;
    pVerRsp->version_accept = blotaSvr.version_accept;


    u8 status = blc_gatt_pushHandleValueNotify (blotaSvr.ota_connHandle, blotaSvr.ota_notify_attHandle, temp_buffer, sizeof(ota_versionRsp_t));
    if(status == BLE_SUCCESS){
        blotaSvr.data_pending_type = 0;
    }

    return status;
}



/**
 *  @brief OTA service flow connection terminate callback, should be registered in GAP layer
 */
_attribute_no_inline_
int blt_ota_server_terminate(u16 connHandle)
{
    /* can auto handle zero value */
    if(connHandle == blotaSvr.ota_connHandle){
        if(blotaSvr.ota_step == OTA_STEP_FEEDBACK){
            blotaSvr.ota_step = OTA_STEP_FINISH;
            blotaSvr.data_pending_type = 0;
        }
        else if(blotaSvr.ota_step != OTA_STEP_IDLE){
            /* jump feedback step, cause can not send any notify data when connection terminate */
            blt_ota_setResult(OTA_STEP_FINISH, OTA_FAIL_DUE_TO_CONNECTION_TERMINATE);
        }
    }

    return 0;
}


_attribute_no_inline_
void blt_ota_procOtaResultFeedback(void)
{
    if(clock_time_exceed(blotaSvr.feedback_begin_tick, 4000000)){ //4S
        //time cost too long, do not consider sending feedback to peer device, enter OTA indicate and OTA result immediately
        blotaSvr.ota_step = OTA_STEP_FINISH;
        blotaSvr.data_pending_type = 0;
    }
    else{

        if(blotaSvr.data_pending_type == DATA_PENDING_OTA_RESULT){

            /* send OTA result on notify data
             * if connection disconnect, can not notify any data to peer device */
            u8 temp_buffer[sizeof(ota_result_t)];
            ota_result_t *pResult = (ota_result_t *)temp_buffer;
            pResult->ota_cmd = CMD_OTA_RESULT;
            pResult->result = blotaSvr.otaResult;
            pResult->rsvd = 0;

            u8 status = blc_gatt_pushHandleValueNotify (blotaSvr.ota_connHandle, blotaSvr.ota_notify_attHandle, temp_buffer, sizeof(ota_result_t));
            if(status == BLE_SUCCESS){
                blotaSvr.data_pending_type = DATA_PENDING_TERMINATE_CMD;
            }
        }

        if(blotaSvr.data_pending_type == DATA_PENDING_TERMINATE_CMD){
        
            #if (BLE_MULTIPLE_CONNECTION_ENABLE)
                u8 status = blc_ll_disconnect(blotaSvr.ota_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN);
            #else
                u8 status = bls_ll_terminateConnection(HCI_ERR_REMOTE_USER_TERM_CONN);
            #endif
            
            if(status == BLE_SUCCESS || status == HCI_ERR_UNKNOWN_CONN_ID){
                blotaSvr.data_pending_type = 0;
            }
        }

    }

}

_attribute_no_inline_
void blt_ota_procOtaFinish(void)
{

#if (OTA_RESULT_ORDER_CHANGE)
    int reboot = 0;
    if(blotaSvr.otaResult){  //OTA Fail
        /* need reboot for those MCU which can not process erasing Flash while ACL connection still works,
         * For those MCU support , no need reboot, just inform peer device OTA fail and disconnect */
        if(blotaSvr.flash_addr_mark >= 0)
        {
            #if (MCU_SUPPORT_MULTI_PRIORITY_IRQ)
                /* erase from end to head */
                for(int adr = blotaSvr.flash_addr_mark; adr >= 0; adr -= 4096) {
                    flash_erase_sector(ota_program_offset + adr);
                }
            #else
                irq_disable();
                /* erase from end to head */
                for(int adr = blotaSvr.flash_addr_mark; adr >= 0; adr -= 4096) {
                    flash_erase_sector(ota_program_offset + adr);
                }
                reboot = 1;
            #endif
        }
    }
    else{ //OTA Success, must reboot
        /* attention: can not erase any data before new firmware start to work !!! */
        blt_ota_writeBootMark();
        reboot = 1;
    }


    if(otaResIndicateCb){
        otaResIndicateCb(blotaSvr.otaResult);   //OTA result(Success/Fail) indicate callback
    }


    #if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
        /* do it before reboot*/
        if(flash_prot_op_cb && blotaSvr.fw_area_unlock){
            flash_prot_op_cb(FLASH_OP_EVT_STACK_OTA_WRITE_NEW_FW_END, 0, 0);
        }
    #endif

    /* differ code mark attention: different code from single priority IRQ IC(e.g. B85m) for some special Flash
     * ZBIT_FLASH_ON_SINGLE_PRIORITY_IRQ_IC_WORKAROUND_EN */

    if(reboot){
        start_reboot();
    }
    else{
        blt_ota_reset();  //must reset OTA flow status for next OTA
    }
#else
    if(otaResIndicateCb){
        otaResIndicateCb(blotaSvr.otaResult);   //OTA result(Success/Fail) indicate callback
    }


    if(blotaSvr.otaResult){  //OTA Fail
        /* need reboot for those SDK which can not process erasing Flash while BLE still works,
         * For Eagle PUYA flash,  no need reboot, just tell peer device OTA fail and disconnect */
        if(blotaSvr.flash_addr_mark >= 0)
        {
            #if (MCU_SUPPORT_MULTI_PRIORITY_IRQ)
                /* erase from end to head */
                for(int adr = blotaSvr.flash_addr_mark; adr >= 0; adr -= 4096) {
                    flash_erase_sector(ota_program_offset + adr);
                }
            #else
                irq_disable();
                /* erase from end to head */
                for(int adr = blotaSvr.flash_addr_mark; adr >= 0; adr -= 4096) {
                    flash_erase_sector(ota_program_offset + adr);
                }
                start_reboot();
            #endif
        }

        blt_ota_reset();  //must reset OTA flow status for next OTA
    }
    else{ //OTA Success, must reboot
        /* attention: can not erase any data before new firmware start to work !!! */
        blt_ota_writeBootMark();
        start_reboot();
    }
#endif
}


_attribute_no_inline_
ble_sts_t blt_ota_pushSchedulerIndication(void)
{
    u8 temp_buffer[sizeof(ota_sche_pdu_num_t)];
    ota_sche_pdu_num_t *pScheIndicate = (ota_sche_pdu_num_t *)temp_buffer;
    pScheIndicate->ota_cmd = CMD_OTA_SCHEDULE_PDU_NUM;
    pScheIndicate->success_pdu_cnt = blotaSvr.schdl_pduNum_rpt;

    u8 status = blc_gatt_pushHandleValueNotify (blotaSvr.ota_connHandle, blotaSvr.ota_notify_attHandle, temp_buffer, sizeof(ota_sche_pdu_num_t));
    if(status == BLE_SUCCESS){
        blotaSvr.schdl_pduNum_rpt = 0; //clear when pushTxFifo OK
    }

    return status;
}



_attribute_no_inline_
int blt_proc_ota_server(void)
{
    if(blotaSvr.data_pending_type == DATA_PENDING_VERSION_RSP){
        blt_ota_pushVersionRsp();
    }

    if(blotaSvr.ota_step == OTA_STEP_FEEDBACK){
        blt_ota_procOtaResultFeedback();
    }

    if(blotaSvr.ota_step == OTA_STEP_FINISH){
            if(0){
            }
        #if (HARDWARE_SECURE_BOOT_SUPPORT_EN)
            else if(ota_sec_boot_cb){
                ota_sec_boot_cb(OSB_TYPE_FINISH, 0, 0, NULL);  //blt_ota_secure_boot_process
            }
        #endif
        #if (HARDWARE_FIRMWARE_ENCRYPTION_SUPPORT_EN)
            else if(ota_encryption_cb){
                ota_encryption_cb(OERW_TYPE_FINISH, 0, 0, NULL);  //blt_ota_encryption_process
            }
        #endif
            else{
                blt_ota_procOtaFinish();
            }
    }

    if(blotaSvr.ota_timeout_enable && (blotaSvr.ota_start_tick || blotaSvr.data_packet_tick)){
        blt_ota_procTimeout();
    }


    /* scheduler indication notify process
     * when OTA feedback or OTA result is coming, scheduler indication packet is abandoned */
    if(blotaSvr.schdl_pduNum_rpt && blotaSvr.ota_step != OTA_STEP_FEEDBACK && blotaSvr.ota_step != OTA_STEP_FINISH){
        blt_ota_pushSchedulerIndication();
    }

    return 0;
}




/**
 *  @brief OTA flow main_loop callback, should be registered in GAP layer
 */
//_attribute_ram_code_  //use flash function, save some SRAM. Actually here no running timing requirement
int blt_ota_server_main_loop(void)
{
    if(blotaSvr.ota_busy){
        blt_proc_ota_server();

    #if OS_SUP_EN
        if(blt_os_giveSem_cb)
        {
            blt_os_giveSem_cb();
        }
    #endif
    }

    return 0;
}


bool blt_ota_isOtaBusy(void)
{
    return blotaSvr.ota_busy;
}













/**
 * @brief     check if MCU load flash error when power on, will reboot if this error happens
 *            attention: this is a kind of error handling, to detect some potential risk.
 * @param      none
 * @return     1: flash load error happens
 *             0: flash load no error
 */
bool blt_ota_software_check_flash_load_error(void)
{

    #if (HARDWARE_SECURE_BOOT_SUPPORT_EN || HARDWARE_FIRMWARE_ENCRYPTION_SUPPORT_EN)
        /* if hardware secure boot and encryption used, no need check this
         * if encryption used, secure boot must be used */
        if(ota_sec_boot_cb || ota_encryption_cb){
            return FALSE;
        }
    #endif



    u32 cur_run_fw_addr;
    if(ota_program_offset){
        cur_run_fw_addr = 0;
    }
    else{
        cur_run_fw_addr = ota_program_bootAddr;
    }

    u32 tlnk_flag;
    flash_read_page(cur_run_fw_addr + BOOT_MARK_ADDR, 4, (unsigned char *)&tlnk_flag);
    if(tlnk_flag != BOOT_MARK_VALUE_4_BYTE){
        start_reboot();
        return TRUE;
    }

    /* in case of e.g. 0x00000 & 0x20000 both valid boot flag, should use firmware boot address 0 here
     * so firmware boot address 0x20000 is error */
    if(cur_run_fw_addr){ //none zero
        flash_read_page(0 + BOOT_MARK_ADDR, 4, (unsigned char *)&tlnk_flag);
        if(tlnk_flag == BOOT_MARK_VALUE_4_BYTE){
            start_reboot();
            return TRUE;
        }
    }

    return FALSE;
}


void bls_ota_clearNewFwDataArea(void)
{
    blotaSvr.newFwArea_clear = 1;

     //in case customer not call in old version
    if(!blotaSvr.otaInit){
        blc_ota_initOtaServer_module();
    }



    #if (OTA_ADD_MORE_CHECK_BEFORE_ERASE_FM_BAKUP_AREA)
        if(blt_ota_software_check_flash_load_error()){
            return;
        }
    #endif



    #if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
        u8  flash_clear_trigger = 0;
    #endif


    #if 1 //new, safer method
        //When the OTA is successfully restarted to erase the old firmware,
        //the 0 and 2k addresses of each sector in the old firmware area are polled, a
        //nd the sector is erased if the 4-byte data read out is not 0xFFFFFFFF.
        //A special situation, the address of the last sector 0 in the old firmware area is just 0xFFFFFFFF,
        //and the end of the firmware is less than 2k,
        //so the last sector is not erased after OTA succeeds,
        //which may cause the next OTA to read and write flash data in this area inconsistent and fail.
        u32 tmp1 = 0;
        u32 tmp2 = 0;
        u32 tmp3 = 0;
        u32 tmp4 = 0;
        u32 erase_consecutive_cnt = 0;
        int cur_flash_setor;
        for(unsigned int i = 0; i < (ota_firmware_max_size>>12); ++i)  // "size>>12" equal to "size/4K"
        {
            cur_flash_setor = ota_program_offset + i*0x1000;

            int cur_erase_trigger = 0;
            if(erase_consecutive_cnt){
                cur_erase_trigger = 1;
                erase_consecutive_cnt --;
            }

            flash_read_page(cur_flash_setor,        4, (u8 *)&tmp1);
            flash_read_page(cur_flash_setor + 1024, 4, (u8 *)&tmp2);
            flash_read_page(cur_flash_setor + 2048, 4, (u8 *)&tmp3);
            flash_read_page(cur_flash_setor + 4000, 4, (u8 *)&tmp4);

            if(tmp1 != ONES_32 || tmp2 != ONES_32 || tmp3 != ONES_32 || tmp4 != ONES_32)
            {
                cur_erase_trigger = 1;
                erase_consecutive_cnt = 2;
            }

            if(cur_erase_trigger){
                #if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
                    if(!flash_clear_trigger && flash_prot_op_cb){
                        flash_clear_trigger = 1;
                        flash_prot_op_cb(FLASH_OP_EVT_STACK_OTA_CLEAR_OLD_FW_BEGIN, ota_program_offset, ota_program_offset + ota_firmware_max_size);
                    }
                #endif
                flash_erase_sector(cur_flash_setor);
            }
        }
    #else
        u32 tmp1 = 0;
        u32 tmp2 = 0;
        int cur_flash_sector;
        for(int i = 0; i < (ota_firmware_max_size>>12); ++i)  // "size>>12" equal to "size/4K"
        {
            cur_flash_sector = ota_program_offset + i*0x1000;
            flash_read_page(cur_flash_sector,       4, (u8 *)&tmp1);
            flash_read_page(cur_flash_sector + 2048, 4, (u8 *)&tmp2);

            if(tmp1 != ONES_32 || tmp2 != ONES_32)
            {
                #if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
                    if(!flash_clear_trigger && flash_prot_op_cb){
                        flash_clear_trigger = 1;
                        flash_prot_op_cb(FLASH_OP_EVT_STACK_OTA_CLEAR_OLD_FW_BEGIN, ota_program_offset, ota_program_offset + ota_firmware_max_size);
                    }
                #endif

                flash_erase_sector(cur_flash_sector);
            }
        }
    #endif

    #if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
        if(flash_clear_trigger){
            flash_prot_op_cb(FLASH_OP_EVT_STACK_OTA_CLEAR_OLD_FW_END, 0, 0);
        }
    #endif


    /* consider disabling flash protect, clearing secure boot descriptor area must be here */
    #if (HARDWARE_SECURE_BOOT_SUPPORT_EN)
        /* erase secure boot descriptor area */
        if(ota_sec_boot_cb){
            ota_sec_boot_cb(OSB_TYPE_CLEAR, 0, 0, NULL);  //blt_ota_secure_boot_process
        }
    #endif
}


void blt_ota_registerOtaWriteFwCallback (ota_write_fw_callback_t cb)
{
    ota_write_fw_cb = cb;
}

