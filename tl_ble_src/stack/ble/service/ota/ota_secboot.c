/********************************************************************************************************
 * @file    ota_secboot.c
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

#include "ota.h"
#include "ota_stack.h"
#include "ota_server.h"

#include "stack/ble/secureboot/secureboot_stack.h"

#if (HARDWARE_SECURE_BOOT_SUPPORT_EN || HARDWARE_FIRMWARE_ENCRYPTION_SUPPORT_EN)

    #ifndef FW_SIMPLE_CHECK_EN
        #define FW_SIMPLE_CHECK_EN 0
    #endif

    //attention: disable these macro after debug OK
    #define DBG_LOG_OTASB_FLOW_EN                   0
    #define DBG_LOG_OTASB_FLOW_ERR_EN               0
    #define DBG_LOG_OTASB_MORE_DETAIL_EN            0
    #define DBG_SIGN_VERIFY_SIMULATE_BY_SAMPLE_DATA 0 //debug, must disable after test OK
    #define DBG_LOG_ADD_SOME_CHECK_WHEN_DEVELOP     0
    #define SIHUI_SIMULATE_DBG_OTASB_FLOW           0 //SiHui debug, must disable after test OK


    //0: calculate signature after all FW received, should read from flash
    //1: calculate signature when every OTA received during OTA
    #define CALCULATE_SIGNATURE_WHEN_RECEIVING_PKT 1

    #define PUBKEY_SIGN_PDU_LEN                    16


_attribute_ble_data_retention_ ota_sb_t bltOtaSb;
_attribute_ble_data_retention_ u8       pubkey_sign_buf[128];


_attribute_data_retention_sec_ flash_handler_t            ota_flash_write      = flash_page_program; //driver: flash_write_page = flash_page_program
_attribute_data_retention_sec_ flash_read_check_handler_t ota_flash_read_check = flash_dread_check;


_attribute_data_retention_sec_ u8 ota_hold_dat_buff[256];


    #if (CALCULATE_SIGNATURE_WHEN_RECEIVING_PKT)

_attribute_data_retention_sec_ unsigned char bin_hash[32];
_attribute_data_retention_sec_ SHA256_Ctx    ctx[1];

    #endif


void blt_ota_security_init(void)
{
    #if (SIHUI_SIMULATE_DBG_OTASB_FLOW) //simulation
    /* when debug, EFUSE not burn, so set these value by software */
    mcuSecur.fw_enc_en  = 0;
    mcuSecur.secboot_en = 1;
    /*   1M Flash: 0x F8000
             2M Flash: 0x1F8000
             4M Flash: 0x3F8000
            16M Flash: 0xFF8000 */
    mcuSecur.sb_desc_adr_k = 0xF8;
    #endif


    /************** load EFUSE information to software variables *******************/
    bltOtaSb.hw_fwEnc_en   = mcuSecur.fw_enc_en;
    bltOtaSb.hw_secBoot_en = mcuSecur.secboot_en;


    if (bltOtaSb.hw_fwEnc_en) {
        ota_flash_write      = flash_page_program_encrypt;
        ota_flash_read_check = flash_dread_decrypt_check;
    } else {
        ota_flash_write      = flash_page_program; // default value of "flash_write_page"
        ota_flash_read_check = flash_dread_check;
    }


    if (bltOtaSb.hw_secBoot_en) {
        u32 secboot_desc_addr = mcuSecur.sb_desc_adr_k << 12; //unit: 4K

        if (ota_program_offset) {
            bltOtaSb.old_fw_desc_addr = secboot_desc_addr;
            bltOtaSb.new_fw_desc_addr = secboot_desc_addr + SECBOOT_DESC_SIZE;
        } else {
            bltOtaSb.old_fw_desc_addr = secboot_desc_addr + SECBOOT_DESC_SIZE;
            bltOtaSb.new_fw_desc_addr = secboot_desc_addr;
        }


        //add some check to be more secure
        /* Min flash size 1M for secure boot, and always put on tail position */
        if (secboot_desc_addr < 0xF000 || secboot_desc_addr > flash_sector_mac_address) {
            bltOtaSb.system_error |= SYSERR_DESC_ADDR_ERR;
        }

    #if 0 //(!BLE_MULTIPLE_CONNECTION_ENABLE)  //take case
            extern u32 flash_sector_mac_address;
            /* single connection SDK, MAC address area follow flash size */
            if(secboot_desc_addr > flash_sector_mac_address ){
                bltOtaSb.system_error |= SYSERR_DESC_ADDR_ERR;
            }
    #endif


    /* SiHui: secure boot descriptor design and details are different for all ICs, so we must write new code for new IC */
    #if (MCU_CORE_TYPE == MCU_CORE_B92 || MCU_CORE_TYPE == MCU_CORE_TL321X || MCU_CORE_TYPE == MCU_CORE_TL721X || \
            MCU_CORE_TYPE == MCU_CORE_TL322X || MCU_CORE_TYPE == MCU_CORE_TL323X)
        /*   1M Flash: 0x F8000
                 2M Flash: 0x1F8000
                 4M Flash: 0x3F8000
                16M Flash: 0xFF8000 */
        if ((mcuSecur.sb_desc_adr_k & 0xFF) != 0xF8) {
            bltOtaSb.system_error |= SYSERR_DESC_ADDR_ERR;
        }
    #else
        #error "descriptor address check for other MCU!!!"
    #endif
    }
}

extern bool mcu_security_read_efuse(void);

ble_sts_t blc_ota_enableFirmwareEncryption(void)
{
    if (!bltOtaSb.secur_infor_read_ok) {
        if (mcu_security_read_efuse()) {
            bltOtaSb.secur_infor_read_ok = 1;
            blt_ota_security_init();
        }
    }


    /* attention: special design
     * for OTA saving data:
     * if customer select both "FW ENC" and "secure boot", will go to "ota_sec boot_cb()", "ota_enc write_cb" will not be used
     * if customer select "FW ENC" but not select "secure boot", then "ota encryption_cb" will be used*/
    ota_encryption_cb = blt_ota_encryption_process;

    return BLE_SUCCESS;
}

ble_sts_t blc_ota_enableSecureBoot(void)
{
    #if (OTA_SB_PUBKEY_SIGN_MSK != CMD_OTA_SB_PUBKEY_SIGN_MIN)
        #error "OTA_SB_PUBKEY_SIGN mask error"
    #endif

    if (!bltOtaSb.secur_infor_read_ok) {
        if (mcu_security_read_efuse()) {
            bltOtaSb.secur_infor_read_ok = 1;
            blt_ota_security_init();
        }
    }


    /* if user not called "blc_ota enableSecureBoot", can save a lot of flash code */
    ota_sec_boot_cb = blt_ota_secure_boot_process;


    return BLE_SUCCESS;
}

void blt_ota_secboot_reset(void)
{
    /* reset some critical status */
    bltOtaSb.secFlow_msk     = 0;
    bltOtaSb.desc_area_write = 0;
    bltOtaSb.last_sign_cmd   = CMD_OTA_SB_PUBKEY_SIGN_MIN - 1;

    bltOtaSb.hold_data_len  = 0;
    bltOtaSb.cur_flash_addr = 0;
}


    #if (!CALCULATE_SIGNATURE_WHEN_RECEIVING_PKT)

/*
 * Attention!!!
 * For firmware encrypted, delete 'static' of driver's API flash_mspi_read_decrypt.
 * We extern use it here, instead of flash_read_page.
 *
 * We will use 'calculate with receive simultaneously' method at last.
 * One can debug 'calculate with receive simultaneously' method under no encrypted mode. And then test Firmware Encryption + Secure Boot.
 */

unsigned int ota_signature_verify(unsigned int data_adr, unsigned int data_size, unsigned char *pub_key, unsigned char *sign)
{
    unsigned char data[256];
    unsigned char bin_hash[32];
    unsigned int  cycle   = (data_size) >> 8;
    unsigned char leftlen = (data_size & 0xff);

    SHA256_Ctx ctx[1];
    SHA256_Init(ctx, bin_hash);

    for (unsigned int i = 0; i < cycle; i++) {
        flash_read_page(data_adr + (i * 256), 256, data);
        SHA256_Process(ctx, data, 256);
    }
    if (leftlen != 0) {
        flash_read_page(data_adr + (cycle * 256), leftlen, data);
        SHA256_Process(ctx, data, leftlen);
    }
    SHA256_Done(ctx);
    eccp_curve_t *curve = secp256r1;
    return ecdsa_verify(curve, bin_hash, 32, pub_key, sign);
}
    #endif


void blt_ota_check_security_infor(void)
{
    if (bltOtaSb.sec_infor_checked) {
        return;
    } else {
        bltOtaSb.sec_infor_checked = 1;
    }

    /* add more method to prevent mass production risk */
    if (!bltOtaSb.secur_infor_read_ok) {
        tlkapi_send_string_data(DBG_LOG_OTASB_MORE_DETAIL_EN, "[OSB][ERR] read efuse error", 0, 0);
        extern bool mcu_security_read_idcode(void);
        if (mcu_security_read_idcode()) { //read IDCODE OK
            tlkapi_send_string_data(DBG_LOG_OTASB_MORE_DETAIL_EN, "[OSB][OK] read idcode ok", 0, 0);
            if (mcuSecur.secboot_en) {    //for secure boot, descriptor address must need; if only FW ENC, do not care
                bltOtaSb.system_error |= SYSERR_EFUSE_READ_FAIL;
                tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] read efuse error, idcode ok, but desc must need", 0, 0);
            } else {
                blt_ota_security_init();
            }
        } else { //read IDCODE Fail
            bltOtaSb.system_error |= SYSERR_IDCODE_READ_FAIL;
            tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] read efuse & idcode all error", 0, 0);
        }
    }
}

int blt_ota_secure_boot_process(int type, u32 flash_addr, int len, void *p)
{
    /* when code run to here, means "ota_sec_boot_cb" none zero, user want use secure_boot.
     * and consider two special case which lead to EFUSE secure boot bit not enable(bltOtaSb.hw secBoot_en is 0):
     * 1. debug at development stage, EFUSE not burned
     * 2. some flow error on production, EFUSE not burn or burn error  */


    int err_flag = OTA_SUCCESS;

    if (type == OSB_TYPE_CHECK_SEC_INFO) {
        blt_ota_check_security_infor();
    } else if (type == OSB_TYPE_CLEAR) {
        //tlkapi_send_string_u8s(DBG_LOG_OTASB_FLOW_EN, "[OSB][FLW] CLEAR", bltOtaSb.hw_secBoot_en, bltOtaSb.system_error, 0, 0);

        /* though customer set SB function, EFUSE SB bit may not correctly burned(debugging stage or TestBench flow error)
         * we can neglect flash area erase, decrease flash writing risk, and save some time */
        if (bltOtaSb.system_error) {
            tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] system error, jump clearing new area", &bltOtaSb.system_error, 1);
            return OTA_SECBOOT_SYSTEM_ERR;
        }

        if (bltOtaSb.hw_secBoot_en) {
            tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_EN, "[OSB][FLW] clear new desc area", &bltOtaSb.new_fw_desc_addr, 4);
    /* attention that erasing flash is dangerous considering that our history flash destroy customer complaint
               so we check more carefully if erasing is a must doing action */
    /* SiHui: secure boot descriptor design and details are different for all ICs, so we must write new code for new IC */
    #if (MCU_CORE_TYPE == MCU_CORE_B92)
        #if (DBG_LOG_ADD_SOME_CHECK_WHEN_DEVELOP)                                   //this should not happen on mass production, here just for debug
            u32 vendorMark_check;
            flash_read_page(bltOtaSb.old_fw_desc_addr, 4, (u8 *)&vendorMark_check); //always plain text
            if (vendorMark_check != 0x544C4E4B) {
                tlkapi_send_string_u32s(DBG_LOG_OTASB_FLOW_EN, "[OSB][ERR] no vendor mark on old descriptor", bltOtaSb.old_fw_desc_addr, vendorMark_check, 0, 0);
            }
        #endif

            u8 temp_buffer[DESC_2ND_SECTOR_DATA_LEN];
            flash_read_page(bltOtaSb.new_fw_desc_addr, DESC_1ST_SECTOR_DATA_LEN, temp_buffer);
            for (int i = 0; i < DESC_1ST_SECTOR_DATA_LEN; i++) {
                if (temp_buffer[i] != 0xFF) {
                    flash_erase_sector(bltOtaSb.new_fw_desc_addr);
                    tlkapi_send_string_data(DBG_LOG_OTASB_MORE_DETAIL_EN, "[OSB][DTL] erase desc 1st sector", 0, 0);
                    break;
                }
            }

            flash_read_page(bltOtaSb.new_fw_desc_addr + 0x1000, DESC_2ND_SECTOR_DATA_LEN, temp_buffer);
            for (int i = 0; i < DESC_2ND_SECTOR_DATA_LEN; i++) {
                if (temp_buffer[i] != 0xFF) {
                    flash_erase_sector(bltOtaSb.new_fw_desc_addr + 0x1000);
                    tlkapi_send_string_data(DBG_LOG_OTASB_MORE_DETAIL_EN, "[OSB][DTL] erase desc 2nd sector", 0, 0);
                    break;
                }
            }
    #elif (MCU_CORE_TYPE == MCU_CORE_TL321X || MCU_CORE_TYPE == MCU_CORE_TL721X || MCU_CORE_TYPE == MCU_CORE_TL322X || \
        MCU_CORE_TYPE == MCU_CORE_TL323X)
        #if (DBG_LOG_ADD_SOME_CHECK_WHEN_DEVELOP) //this should not happen on mass production, here just for debug
            u32 vendorMark_check;
            flash_read_page(bltOtaSb.old_fw_desc_addr, 4, (u8 *)&vendorMark_check); //always plain text
            if (vendorMark_check != 0x544C4E4B) {
                tlkapi_send_string_u32s(DBG_LOG_OTASB_FLOW_EN, "[OSB][ERR] no vendor mark on old descriptor", bltOtaSb.old_fw_desc_addr, vendorMark_check, 0, 0);
            }
        #endif

            u8 temp_buffer[DESC_SECTOR_DATA_LEN];
            flash_read_page(bltOtaSb.new_fw_desc_addr, DESC_SECTOR_DATA_LEN, temp_buffer);
            for (int i = 0; i < DESC_SECTOR_DATA_LEN; i++) {
                if (temp_buffer[i] != 0xFF) {
                    flash_erase_sector(bltOtaSb.new_fw_desc_addr);
                    tlkapi_send_string_data(DBG_LOG_OTASB_MORE_DETAIL_EN, "[OSB][DTL] erase desc 1st sector", 0, 0);
                    break;
                }
            }
    #else
        #error "add descriptor clear for other MCU"
    #endif
        }
    } else if (type == OSB_TYPE_START) {
    #if 1 //debug, remove later
        if (bltOtaSb.hw_secBoot_en) {
        /* SiHui: secure boot descriptor design and details are different for all ICs, so we must write new code for new IC */
        #if (MCU_CORE_TYPE == MCU_CORE_B92 || MCU_CORE_TYPE == MCU_CORE_TL321X || MCU_CORE_TYPE == MCU_CORE_TL721X || \
                MCU_CORE_TYPE == MCU_CORE_TL322X || MCU_CORE_TYPE == MCU_CORE_TL323X)
            u8 temp_buffer[4];
            flash_read_page(bltOtaSb.old_fw_desc_addr, 4, temp_buffer);

            u8 vendor_fw_mark[4] = {0x4B, 0x4E, 0x4C, 0x54};
            if (memcmp(temp_buffer, vendor_fw_mark, 4)) { //do not equal
                return OTA_FIRMWARE_MARK_ERR;
            }
        #else
            #error "other MCU process check"
        #endif
        }
    #endif


        blt_ota_secboot_reset();
    } else if (type == OSB_TYPE_PUBKEY_SIGN) {
        rf_packet_att_data_t *pAttDat = (rf_packet_att_data_t *)p;
        tlkapi_send_string_data((stkLog_mask & STK_LOG_OTA_FLOW), "[OTA][FLW] ota pubkey signature", pAttDat->dat, 18);

        if (bltOtaSb.system_error) {
            err_flag = OTA_SECBOOT_SYSTEM_ERR;
            tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] system error", &bltOtaSb.system_error, 1);
        }
        /* 1. no OTA start before OTA PUBKEY_SIGN */
        else if (!(blotaSvr.flow_mask & OTA_FLOW_START)) {
            err_flag = OTA_FLOW_ERR;
            tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] no OTA start before OTA PUBKEY_SIGN", 0, 0);
        }
        /* 2. last command of OTA PUBKEY_SIGN has already send */
        else if (bltOtaSb.secFlow_msk & OSB_FLOW_SIGN_RX_ALL) {
            err_flag = OTA_SECBOOT_PUBKEY_SIGN_SEQ_ERR;
            tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] PUBKEY_SIGN last data already send", 0, 0);
        }
        /* 3. l2cap length error */
        else if (pAttDat->l2cap != 23) { //3: opcode(1) + attHandle(2);  20: adr_index(2) + data(16) + CRC(2);
            err_flag = OTA_SECBOOT_PUBKEY_SIGN_LEN_ERR;
            tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] l2cap_len not 23", 0, 0);
        }
        /* repeated OTA PDU or lost some OTA PDU */
        else if (blotaSvr.ota_cmd_adr != bltOtaSb.last_sign_cmd + 1) {
            err_flag = OTA_SECBOOT_PUBKEY_SIGN_SEQ_ERR;
            tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] PUBKEY_SIGN data sequence error", 0, 0);
        } else {
            u16 crc16_cal = blt_Crc16ComputeInternal(pAttDat->dat, 18);
            u16 crc16_rcv = pAttDat->dat[19] << 8 | pAttDat->dat[18];

            /* CRC16 error */
            if (crc16_cal != crc16_rcv) {
                err_flag = OTA_DATA_CRC_ERR;
                tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] CRC 16 error", 0, 0);
            } else {
                u8 data_idx = blotaSvr.ota_cmd_adr & 0x7;
                memcpy(pubkey_sign_buf + data_idx * PUBKEY_SIGN_PDU_LEN, pAttDat->dat + 2, PUBKEY_SIGN_PDU_LEN);

                if (blotaSvr.ota_cmd_adr == CMD_OTA_SB_PUBKEY_SIGN_MAX) {
                    bltOtaSb.secFlow_msk |= OSB_FLOW_SIGN_RX_ALL;

                    if (bltOtaSb.hw_secBoot_en) //for EFUSE & descriptor burning missed IC on TestBench,  skip public key check
                    {
                        /* two method: 1. calculate hash on EFUSE; 2.compare with public_key on previous Flash descriptor
                         * method 2 is simple, we use it now */
                        int public_key_match = 0;
    #if 0 //method 1
        #if (DBG_SIGN_VERIFY_SIMULATE_BY_SAMPLE_DATA)
                                int read_hash_success = 1;
                                u8 hw_pubkey_hash[32]= { 0x02,0xBF,0xE3,0xF7,0x27,0xF3,0xFF,0x17,0xCE,0x58,0x7E,0x3B,0x17,0xFB,0x3B,0x2C,
                                                         0x86,0x93,0xB5,0x7E,0xFF,0xA4,0x0B,0xA6,0x70,0x46,0xA4,0xEF,0x9D,0x6F,0x83,0x9C };
        #else
                                u8 hw_pubkey_hash[32]; //EFUSE_BYTE_LEN_PUB_KEY_HASH
                                int read_hash_success = efuse_get_pubkey_hash(pubkey_hash);
        #endif

                            if(!read_hash_success){ //read fail
                                err_flag = OTA_SECBOOT_HW_ERR;
                                tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] read efuse hash error", 0, 0);
                            }
                            else{
                                unsigned char pub_key_hash[32];
                                SHA256_Hash(pubkey_sign_buf, 64, pub_key_hash);//pub_key hash
                                if(!memcmp(pub_key_hash, hw_pubkey_hash, 32)){ //public key error
                                    public_key_match = 1;
                                }
                            }
    #else //method 2
                        u8  old_pubkey_buf[64];
                        u32 old_pubkey_addr = bltOtaSb.old_fw_desc_addr + DESCRIPTOR_PUBKEY_OFFSET;
                        flash_read_page(old_pubkey_addr, 64, old_pubkey_buf);
                        if (!memcmp(old_pubkey_buf, pubkey_sign_buf, 64)) { //do not equal
                            public_key_match = 1;
                        }
    #endif

                        if (public_key_match) {
                            bltOtaSb.secFlow_msk |= OSB_FLOW_PUBKEY_MATCH;
                            tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_EN, "[OSB][FLW] public key match hash", 0, 0);
                        } else {
                            err_flag = OTA_SECBOOT_PUBLIC_KEY_ERR;
                            tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] public key not match hash", 0, 0);
                            //tlkapi_send_string_u32s(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] addr", old_pubkey_addr, bltOtaSb.secur_read_ok,bltOtaSb.old_fw_desc_addr,0);
                            //tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] old_pubkey_buf", old_pubkey_buf, 32);
                            //tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] pubkey_sign_buf", pubkey_sign_buf, 32);
                        }
                    }
                }


                if (!err_flag) {
                    bltOtaSb.last_sign_cmd = blotaSvr.ota_cmd_adr; //update

                    /* to re_set OTA PDU timeout counter in case timeout triggers too early */
                    blotaSvr.ota_start_tick           = clock_time() | 1; //mark time
                    blotaSvr.process_timeout_100S_cnt = blotaSvr.process_timeout_100S_num;
                }
            }
        }

    } else if (type == OSB_TYPE_SAVE_DATA) {
        if (!(bltOtaSb.secFlow_msk & OSB_FLOW_SIGN_RX_ALL)) {
            /* should send signature before OTA PDU
             * once customer select "secure boot" by calling "blc_ota enableSecureBoot", no not care about if efuse burned OK,
             * even efuse bit not write, we think that OTA client must use "secure boot" mode, must send signature in advance.
             */
            err_flag = OTA_SECBOOT_PUBKEY_SIGN_SEQ_ERR;
        } else {
            u8 *pFwdata = (u8 *)p;

    #if (CALCULATE_SIGNATURE_WHEN_RECEIVING_PKT)
            if (bltOtaSb.hw_secBoot_en || !blotaSvr.write_16B_each_time) {
                err_flag = ota_security_pack_256B_save_data(flash_addr, len, pFwdata);
            } else
    #endif
            {
                err_flag = ota_security_save_data(flash_addr, len, pFwdata);
            }


            if (blotaSvr.ota_cmd_adr == blotaSvr.last_adr_index) { //last adr_index
                //for EFUSE & descriptor burning missed IC on TestBench,  skip verifying signature
                if (!err_flag && bltOtaSb.hw_secBoot_en && (bltOtaSb.secFlow_msk & OSB_FLOW_PUBKEY_MATCH)) {
                    u32 sign_verify_fail;

    #if (CALCULATE_SIGNATURE_WHEN_RECEIVING_PKT)
                    SHA256_Done(ctx);
                    eccp_curve_t *curve = secp256r1;
                    sign_verify_fail    = ecdsa_verify(curve, bin_hash, 32, pubkey_sign_buf, pubkey_sign_buf + 64);
    #else
        #if (SIHUI_SIMULATE_DBG_OTASB_FLOW)
                    sign_verify_fail = 0;
        #else
                    sign_verify_fail = ota_signature_verify(ota_program_offset, blotaSvr.firmware_size_byte, pubkey_sign_buf, pubkey_sign_buf + 64);
        #endif
    #endif

                    if (sign_verify_fail) { //fail
                        err_flag = OTA_SECBOOT_SIGN_VERIFY_FAIL;
                        tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] sign verify fail", 0, 0);
                    } else {                //pass
                        bltOtaSb.secFlow_msk |= OSB_FLOW_SIGN_PASS;
                        tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_EN, "[OSB][FLW] sign verify pass", 0, 0);
                    }
                }

                if (!err_flag && (bltOtaSb.secFlow_msk & OSB_FLOW_SIGN_PASS)) {
                    err_flag = blt_osb_save_descriptor();
                    if (!err_flag) {
                        bltOtaSb.secFlow_msk |= OSB_FLOW_SAVE_DESC_OK;
                    }
                }
            }
        }
    } else if (type == OSB_TYPE_END) {
        if (bltOtaSb.hw_secBoot_en) {
            if (!(bltOtaSb.secFlow_msk & OSB_FLOW_PUBKEY_MATCH)) {
                err_flag = OTA_SECBOOT_PUBLIC_KEY_ERR;
                tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] public key error", 0, 0);
            } else if (!(bltOtaSb.secFlow_msk & OSB_FLOW_SIGN_PASS)) {
                err_flag = OTA_SECBOOT_SIGN_VERIFY_FAIL;
                tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] signature verify fail", 0, 0);
            } else if (!(bltOtaSb.secFlow_msk & OSB_FLOW_SAVE_DESC_OK)) {
                err_flag = OTA_SECBOOT_WRITE_DESC_FAIL;
                tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_ERR_EN, "[OSB][ERR] write descriptor fail", 0, 0);
            }
        }
    } else if (type == OSB_TYPE_FINISH) {
        /* 1. do not concern MCU that not support "MCU_SUPPORT_MULTI_PRIORITY_IRQ" such as Kite/Vulture,
         *    no need reboot when erase flash for OTA fail
         *
         * */
        int reboot = 0;
        if (blotaSvr.otaResult) { //OTA Fail
            if (blotaSvr.flash_addr_mark >= 0) {
                /* erase from end to head */
                for (int adr = blotaSvr.flash_addr_mark; adr >= 0; adr -= 4096) {
                    flash_erase_sector(ota_program_offset + adr);
                }
            }

            if (bltOtaSb.desc_area_write) {
                for (int i = 0; i < SECBOOT_DESC_SECTOR_NUM; i++) {
                    flash_erase_sector(bltOtaSb.new_fw_desc_addr + 0x1000 * i);
                }
            }
        } else { //OTA Success, must reboot
            /* attention: can not erase any data before new firmware start to work !!! */

            if (bltOtaSb.hw_secBoot_en) { //boot flag is on descriptor is valid, boot flag on firmware is ignored
    #if (SIHUI_SIMULATE_DBG_OTASB_FLOW)   //only for debug
                blt_ota_writeBootMark();
    #endif

                ota_write_desc_boot_mark_secure_boot_mode();
            } else { //boot flag on firmware is valid, boot flag on descriptor is ignored
                /* some MCU(like Jaguar), if encryption enabled, should operate 4 byte of "0x544C4E4B", maybe write flash error happens */
                err_flag = ota_write_fw_boot_mark_no_secure_boot_mode();
            }


            if (err_flag) {
                blotaSvr.otaResult = err_flag; //only OTA_WRITE_FLASH_ERR
            } else {
                reboot = 1;
            }
        }


        if (otaResIndicateCb) {
            otaResIndicateCb(blotaSvr.otaResult); //OTA result(Success/Fail) indicate callback
        }

    #if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
        /* do it before reboot*/
        if (flash_prot_op_cb && blotaSvr.fw_area_unlock) {
            flash_prot_op_cb(FLASH_OP_EVT_STACK_OTA_WRITE_NEW_FW_END, 0, 0);
        }
    #endif

        if (reboot) {
    #if (SIHUI_SIMULATE_DBG_OTASB_FLOW) //SiHui debug, must remove later !!!
            tlkapi_send_string_data(SIHUI_SIMULATE_DBG_OTASB_FLOW, "simulate reboot", 0, 0);
            while (1) {
        #if (TLKAPI_DEBUG_ENABLE)
                tlkapi_debug_handler();
        #endif
            }
    #endif

            start_reboot();
        } else {
            blt_ota_reset(); //must reset OTA flow status for next OTA
            blt_ota_secboot_reset();
        }
    }


    return err_flag;
}

/* refer to "flash dread_decrypt_check"
 * only process max 256 byte */
unsigned char flash_dread_check(unsigned long addr, unsigned long len, unsigned char *buf)
{
    if (len <= 256) {
        u8 flash_check[256];                  //biggest value 240
        flash_read_page(addr, len, flash_check);

        if (!memcmp(flash_check, buf, len)) { //equal
            return 0;                         //check pass
        }
    }

    return 1; //check fail
}

_attribute_no_inline_ int ota_security_write_flash(int skip_mode, u8 skip_offset, u32 flash_addr, int len, u8 *data)
{
    /* skip_byte_num = 4
     *
     * 1. skip_data_oft = 0, then write one buffer
     *          address: flash_addr + 4;  length: len - 4;   buffer: data[4]...data[len-1]
     * 2. skip_data_oft = len - 4, then write one buffer
     *          address: flash_addr;  length: len - 4;   buffer: data[0]...data[len-4]
     * 3. other case:  write two buffer
     *     3.A. address: flash_addr;             length: offset;   buffer: data[0]...data[offset - 1]
     *     3.B. address: flash_addr + offset+4;  length: len - (offset + 4);   buffer: data[offset+4]...data[len-1]
     */

    int split_write = 0;
    if (skip_mode) {
        if (skip_offset == 0) {
            flash_addr += 4;
            data += 4;
            len -= 4; //len = 16.32... in OTA
        } else if (skip_offset == len - 4) {
            //flash_addr not change
            //data not change
            len -= 4;
        } else {
            split_write = 1;

            u32 real_flash_addr = ota_program_offset + flash_addr;

            ota_flash_write(real_flash_addr, skip_offset, data);
            if (ota_flash_read_check(real_flash_addr, skip_offset, data)) { //check fail
                return OTA_WRITE_FLASH_ERR;
            }

            u8 new_begin = skip_offset + 4;
            ota_flash_write(real_flash_addr + new_begin, len - new_begin, data + new_begin);
            if (ota_flash_read_check(real_flash_addr + new_begin, len - new_begin, data + new_begin)) { //check fail
                return OTA_WRITE_FLASH_ERR;
            }
        }
    }


    if (!split_write) {
        u32 real_flash_addr = ota_program_offset + flash_addr;
        ota_flash_write(real_flash_addr, len, data);
        if (ota_flash_read_check(real_flash_addr, len, data)) { //check fail
            return OTA_WRITE_FLASH_ERR;
        }
    }


    return 0;
}

/* 1. Hardware Secure Boot enable: boot flag is on descriptor is valid, boot flag on firmware is ignored
 *          do not change boot flag "0x544C4E4B", write directly
 * 2. Hardware Secure Boot disable: boot flag on firmware is valid, boot flag on firmware is ignored
 *    2.1 Hardware FW Encryption enable
 *          B92(Jaguar): must skip boot flag 4 byte area, can not write anything, because encrypted value is random, maybe 0x00 or 0xFF
 *    2.2 Hardware FW Encryption disable
 *          process same as "ota save_data" in ota_server.c
 */
_attribute_no_inline_ int ota_security_save_data(u32 flash_addr, int len, u8 *data)
{
    int err_flag = OTA_SUCCESS;

    if (bltOtaSb.hw_secBoot_en || bltOtaSb.hw_fwEnc_en) {
        int skip_mode     = 0;
        u8  fwMark_offset = 0;
        if (flash_addr <= BOOT_MARK_ADDR && (flash_addr + len) > BOOT_MARK_ADDR) {
            fwMark_offset        = BOOT_MARK_ADDR - flash_addr;
            u8 vendor_fw_mark[4] = {0x4B, 0x4E, 0x4C, 0x54};

            if (memcmp(data + fwMark_offset, vendor_fw_mark, 4)) { //do not equal
                return OTA_FIRMWARE_MARK_ERR;
            }

            if (!bltOtaSb.hw_secBoot_en) { //FW ENC enable & SEC BOOT disable
                skip_mode = 1;             //skip boot mark write, leave 0xFFFFFFFF on Flash
            }

    #if (SIHUI_SIMULATE_DBG_OTASB_FLOW)    //only for debug
            data[fwMark_offset] = 0xFF;
    #endif
        }


        if (blotaSvr.write_16B_each_time) { //GD flash, 16B, 16M clock:162 uS;  32M clock: 133uS
            for (int i = 0; i < len; i += 16) {
                err_flag = ota_security_write_flash(skip_mode, fwMark_offset, flash_addr + i, 16, data + i);
                if (err_flag) {
                    break;
                }
            }
        } else {
            err_flag = ota_security_write_flash(skip_mode, fwMark_offset, flash_addr, len, data);
        }
    } else {
        err_flag = ota_save_data(flash_addr, len, data);
    }

    return err_flag;
}

int ota_security_pack_256B_save_data(u32 flash_addr, int ota_len, u8 *ota_data)
{
    #if (FW_SIMPLE_CHECK_EN)
    if (flash_addr == 0) {
        check_in     = 0;
        check_out    = 0;
        check_finish = 1;
    }
    for (int i = 0; i < ota_len; i++) {
        check_in ^= ota_data[i];
    }
    #endif

    #if (DBG_OTA_WRITE_FW) //debug, remove when code test OK
    //debug, remove when code test OK
    if (bltOtaSb.hold_data_len > 255) {
        BLMS_ERR_DEBUG(DBG_OTA_WRITE_FW, 0xEE010000 | (bltOtaSb.hold_data_len & 0xFFFF));
    }

    if (flash_addr == 0 && bltOtaSb.cur_flash_addr != 0) {
        write_reg32(0x40004, flash_addr);
        write_reg32(0x40008, bltOtaSb.cur_flash_addr);
        BLMS_ERR_DEBUG(DBG_OTA_WRITE_FW, 0xEE020000);
    }
    #else
    if (flash_addr == 0 && bltOtaSb.cur_flash_addr != 0) {
        return OTA_LOGIC_ERR;
    }
    #endif


    int new_data_len = 0;
    int new_hold_len = 0;

    int total_data_len = bltOtaSb.hold_data_len + ota_len;

    if (total_data_len > 255) { //>=256
        new_hold_len = total_data_len - 256;
        new_data_len = ota_len - new_hold_len;

    #if (DBG_OTA_WRITE_FW) //debug, remove when code test OK
        if ((new_hold_len + new_data_len) != ota_len) {
            write_reg32(0x40004, new_hold_len);
            write_reg32(0x40008, new_data_len);
            write_reg32(0x4000C, ota_len);
            BLMS_ERR_DEBUG(DBG_OTA_WRITE_FW, 0xEE030000);
        }
    #endif


        u8 OTA_buff[256];

        /* packet 256 Byte together, ready to write a new page */
        if (bltOtaSb.hold_data_len) {
            memcpy(OTA_buff, ota_hold_dat_buff, bltOtaSb.hold_data_len);   //old hold OTA data into buffer first
        }
        memcpy(OTA_buff + bltOtaSb.hold_data_len, ota_data, new_data_len); //new OTA data into buffer


        /* if there is some data left, put them in hold buffer for later use */
        if (new_hold_len) {                    //this value is 0 when total_data_len is 256, so add judge
            memcpy(ota_hold_dat_buff, ota_data + new_data_len, new_hold_len);
        }
        bltOtaSb.hold_data_len = new_hold_len; //update


    #if (CALCULATE_SIGNATURE_WHEN_RECEIVING_PKT)
        if (bltOtaSb.hw_secBoot_en && (bltOtaSb.secFlow_msk & OSB_FLOW_PUBKEY_MATCH)) {
            if ((flash_addr & 0xFFFFFF00) == 0) { //first OTA data
                SHA256_Init(ctx, bin_hash);
            }

            int ota_data_size = 256;
            if (blotaSvr.ota_cmd_adr == blotaSvr.last_adr_index && !bltOtaSb.hold_data_len) {
                /* firmware data is 256*N, no left data, last 256 data may have align16 makeup data
                     * for CRC32 check firmware, align16 makeup_len is always 12, last firmware size is 244 */
                ota_data_size = 256 - blotaSvr.align16_makeup_len;
            }
            SHA256_Process(ctx, OTA_buff, ota_data_size);
        }
    #endif


        int err_flag = ota_security_save_data(bltOtaSb.cur_flash_addr, 256, OTA_buff); //can not return here
        bltOtaSb.cur_flash_addr += 256;                                                //maintain flash address
        if (err_flag) {
            return err_flag;
        }
    } else {
        memcpy(ota_hold_dat_buff + bltOtaSb.hold_data_len, ota_data, ota_len);
        bltOtaSb.hold_data_len = total_data_len;
    }


    if (blotaSvr.ota_cmd_adr == blotaSvr.last_adr_index) { //last OTA PDU, may not 256B
        if (bltOtaSb.hold_data_len) {
    #if (CALCULATE_SIGNATURE_WHEN_RECEIVING_PKT)
            if (bltOtaSb.hw_secBoot_en && (bltOtaSb.secFlow_msk & OSB_FLOW_PUBKEY_MATCH)) {
                /* for CRC32 check firmware, align16 makeup_len is always 12, hold data_len is 16*N */
                SHA256_Process(ctx, ota_hold_dat_buff, bltOtaSb.hold_data_len - blotaSvr.align16_makeup_len);
            }
    #endif

            return ota_security_save_data(bltOtaSb.cur_flash_addr, bltOtaSb.hold_data_len, ota_hold_dat_buff);
        }
    }

    return OTA_SUCCESS;
}

int blt_osb_save_descriptor(void)
{
    tlkapi_send_string_data(DBG_LOG_OTASB_FLOW_EN, "[OSB][FLW] save descriptor", 0, 0);

    //make descriptor and write on flash
    /* SiHui: secure boot descriptor design and details are different for all ICs, so we must write new code for new IC */
    #if (MCU_CORE_TYPE == MCU_CORE_B92)
    u8                    desc_buffer[sizeof(sb_desc_2nd_sector_t)];
    sb_desc_2nd_sector_t *pSbDesc = (sb_desc_2nd_sector_t *)desc_buffer;

    //0x23FFFF20 mspi_set_l: mutiboot address offset option, 0:0k;  1:128k;  2:256k;  4:512k
    //0x23FFFF21 mspi_set_h: program space size = (mspi_set_h+1)*4k
    if (ota_program_offset) {
        if (ota_program_bootAddr == MULTI_BOOT_ADDR_0x20000) {
            pSbDesc->multi_boot = 0x2020;
        } else if (ota_program_bootAddr == MULTI_BOOT_ADDR_0x40000) {
            pSbDesc->multi_boot = 0x4040;
        } else if (ota_program_bootAddr == MULTI_BOOT_ADDR_0x80000) {
            pSbDesc->multi_boot = 0x8080;
        }
    } else {
        pSbDesc->multi_boot = 0x0000;
    }

    memcpy(pSbDesc->public_key, pubkey_sign_buf, 128); // public_key & signature
    pSbDesc->run_code_adr  = ota_program_offset;
    pSbDesc->run_code_size = blotaSvr.firmware_size_byte;

    //TODO @@@@@@@@@@ : maybe this two value can be CONST after driver team confirm
    /* attention that: descriptor use plain text*/
    flash_read_page(bltOtaSb.old_fw_desc_addr + DESCRIPTOR_WATCHDOG_OFFSET, 8, pSbDesc->watdog_v); // watdog_v & smpi_lane


    bltOtaSb.desc_area_write = 1;                                                                  //mark
    u32 current_flash_addr   = bltOtaSb.new_fw_desc_addr;                                          //first sector
    u8  vendor_fw_mark[4]    = {0x4B, 0x4E, 0x4C, 0x54};
    vendor_fw_mark[0]        = 0xFF;                                                               //very important !!!
    /* here write 4 byte, first is 0xFF, but not only write back 3 byte, because we need
         * check if the first flash address is 0xFF for later 4B */
    flash_write_page(current_flash_addr, 4, vendor_fw_mark);
    if (flash_dread_check(current_flash_addr, 4, vendor_fw_mark)) { //check fail
        return OTA_SECBOOT_WRITE_DESC_FAIL;
    }

    current_flash_addr = bltOtaSb.new_fw_desc_addr + 0x1000; //second sector
    if (blotaSvr.write_16B_each_time) {
        // 146 = 144 + 2 = 16*9 + 2
        for (int i = 0; i < 144; i += 16) {                                       //16*8 = 144
            flash_write_page(current_flash_addr + i, 16, desc_buffer + i);
            if (flash_dread_check(current_flash_addr + i, 16, desc_buffer + i)) { //check fail
                return OTA_SECBOOT_WRITE_DESC_FAIL;
            }
        }

        flash_write_page(current_flash_addr + 144, 2, desc_buffer + 144);
        if (flash_dread_check(current_flash_addr + 144, 2, desc_buffer + 144)) { //check fail
            return OTA_SECBOOT_WRITE_DESC_FAIL;
        }
    } else {
        flash_write_page(current_flash_addr, DESC_2ND_SECTOR_DATA_LEN, desc_buffer);
        tlkapi_send_string_data(SIHUI_SIMULATE_DBG_OTASB_FLOW, "simulate reboot", 0, 0);
        if (flash_dread_check(current_flash_addr, DESC_2ND_SECTOR_DATA_LEN, desc_buffer)) { //check fail
            return OTA_SECBOOT_WRITE_DESC_FAIL;
        }
    }
    #elif (MCU_CORE_TYPE == MCU_CORE_TL321X || MCU_CORE_TYPE == MCU_CORE_TL721X || MCU_CORE_TYPE == MCU_CORE_TL322X || \
            MCU_CORE_TYPE == MCU_CORE_TL323X)
    u8                desc_buffer[sizeof(sb_desc_sector_t)];
    sb_desc_sector_t *pSbDesc = (sb_desc_sector_t *)desc_buffer;

    memcpy(pSbDesc->public_key, pubkey_sign_buf, 128); // public_key & signature
    pSbDesc->run_code_adr  = ota_program_offset;
    pSbDesc->run_code_size = blotaSvr.firmware_size_byte;

    //TODO @@@@@@@@@@ : maybe this two value can be CONST after driver team confirm
    /* attention that: descriptor use plain text*/
    flash_read_page(bltOtaSb.old_fw_desc_addr + DESCRIPTOR_WATCHDOG_OFFSET, 8, pSbDesc->watdog_v); // watdog_v & smpi_lane


    bltOtaSb.desc_area_write = 1;                                                                  //mark
    u32 current_flash_addr   = bltOtaSb.new_fw_desc_addr;                                          //first sector
    u8  vendor_fw_mark[4]    = {0x4B, 0x4E, 0x4C, 0x54};
    vendor_fw_mark[0]        = 0xFF;                                                               //very important !!!

    memcpy(pSbDesc->tlnk_mark, vendor_fw_mark, 4);
    current_flash_addr = bltOtaSb.new_fw_desc_addr;                                                //second sector
    if (blotaSvr.write_16B_each_time) {
        for (int i = 0; i < DESC_SECTOR_DATA_LEN; i += 16) {                                       //16*8 = 144
            flash_write_page(current_flash_addr + i, 16, desc_buffer + i);
            if (flash_dread_check(current_flash_addr + i, 16, desc_buffer + i)) {                  //check fail
                return OTA_SECBOOT_WRITE_DESC_FAIL;
            }
        }
    } else {
        flash_write_page(current_flash_addr, DESC_SECTOR_DATA_LEN, desc_buffer);
        tlkapi_send_string_data(SIHUI_SIMULATE_DBG_OTASB_FLOW, "simulate reboot", 0, 0);
        if (flash_dread_check(current_flash_addr, DESC_SECTOR_DATA_LEN, desc_buffer)) { //check fail
            return OTA_SECBOOT_WRITE_DESC_FAIL;
        }
    }
    #else
        #error "add descriptor write for other MCU"
    #endif

    return OTA_SUCCESS;
}

/*
 * when executing this function, we know that:
 * 1. application layer enable FW Encryption, but not enable Secure Boot
 * 2. Current MCU hardware fw encryption function using now, corresponding EFUSE bit enabled
 */
int ota_enc_write_fw_boot_mark(void)
{
    /* SiHui: secure boot descriptor design and details are different for all ICs, so we must write new code for new IC */
    #if (MCU_CORE_TYPE == MCU_CORE_B92 || MCU_CORE_TYPE == MCU_CORE_TL321X || MCU_CORE_TYPE == MCU_CORE_TL721X || \
        MCU_CORE_TYPE == MCU_CORE_TL322X || MCU_CORE_TYPE == MCU_CORE_TL323X)
    /* if write one byte, error risk probability is big: maybe encrypted value of "0x4B" is 0xFF
         * write 4 byte, error risk probability is very small: encrypted value of "0x544C4E4B" is 0xFFFFFFFF
         * here "ota_flash write" can also be  "flash page_program_encrypt"
         *      "ota flash_read_check" can be "flash dread_decrypt_check" */
    u32 flag = 0x544C4E4B;
    ota_flash_write(ota_program_offset + BOOT_MARK_ADDR, 4, (u8 *)&flag);            //Set FW flag
    if (ota_flash_read_check(ota_program_offset + BOOT_MARK_ADDR, 4, (u8 *)&flag)) { //check fail
        return OTA_WRITE_FLASH_ERR;
    }

    /* to destroy old boot mark
         * if write one byte, error risk probability is big: maybe encrypted value of "0x4B" is 0xFF
         * write 4 byte, error risk probability is very small: encrypted value of "0x544C4E4B" is 0xFFFFFFFF
         * normal method: read real value on Flash(use flash read_page), then change at one bit of total 32 bit
         * consider that flash can only write from 1 to 0, we write all 0 value(use flash write_page) is a simple way to change,
         * if encrypted value of "0x544C4E4B" is 0x00000000, we have no method to solve */
    flag = 0;
    flash_write_page((ota_program_offset ? 0 : ota_program_bootAddr) + BOOT_MARK_ADDR, 1, (u8 *)&flag); //Invalid flag
    #else
        #error "enc write boot mark for other MCU !!!"
    #endif


    return OTA_SUCCESS;
}

void ota_write_desc_boot_mark_secure_boot_mode(void)
{
    /* attention: descriptor boot mark is always plain text, so use "flash write_page" is OK
     * no need consider encryption value of "0x544C4E4B",
     * so operate operate one byte  to decrease write flash error probability*/
    u8 flag = BOOT_MARK_VALUE_1_BYTE;
    flash_write_page(bltOtaSb.new_fw_desc_addr, 1, (u8 *)&flag); //Set FW flag
    flag = 0;
    flash_write_page(bltOtaSb.old_fw_desc_addr, 1, (u8 *)&flag); //Invalid flag
}

int ota_write_fw_boot_mark_no_secure_boot_mode(void)
{
    if (bltOtaSb.hw_fwEnc_en) { //special process, consider encryption value of "0x544C4E4B" maybe random and different
        return ota_enc_write_fw_boot_mark();
    } else {                    //use write mark function in ota_server.c, operate one byte 0x4B to decrease write flash error probability
        blt_ota_writeBootMark();
    }

    return OTA_SUCCESS;
}

/*
 * only when application layer secure boot function not enable and FW encryption function enable, this function be executed
 */
int blt_ota_encryption_process(int type, u32 flash_addr, int len, void *p)
{
    int err_flag = OTA_SUCCESS;

    if (type == OERW_TYPE_CHECK_SEC_INFO) {
        blt_ota_check_security_infor();
    } else if (type == OERW_TYPE_SAVE_DATA) {
        u8 *pFwdata = (u8 *)p;
        err_flag    = ota_security_save_data(flash_addr, len, pFwdata);
    } else if (type == OERW_TYPE_FINISH) {
        /* 1. do not concern MCU that not support "MCU_SUPPORT_MULTI_PRIORITY_IRQ" such as Kite/Vulture,
         *    no need reboot when erase flash for OTA fail
         * */
        int reboot = 0;
        if (blotaSvr.otaResult) { //OTA Fail
            if (blotaSvr.flash_addr_mark >= 0) {
                /* erase from end to head */
                for (int adr = blotaSvr.flash_addr_mark; adr >= 0; adr -= 4096) {
                    flash_erase_sector(ota_program_offset + adr);
                }
            }
        } else { //OTA Success
            /* boot flag on firmware is valid, boot flag on firmware is ignored */
            err_flag = ota_write_fw_boot_mark_no_secure_boot_mode();

            if (err_flag) {
                blotaSvr.otaResult = OTA_WRITE_FLASH_ERR;
            } else {
                reboot = 1;
            }
        }

        if (otaResIndicateCb) {
            otaResIndicateCb(blotaSvr.otaResult); //OTA result(Success/Fail) indicate callback
        }


    #if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
        /* do it before reboot*/
        if (flash_prot_op_cb && blotaSvr.fw_area_unlock) {
            flash_prot_op_cb(FLASH_OP_EVT_STACK_OTA_WRITE_NEW_FW_END, 0, 0);
        }
    #endif

        if (reboot) {
            start_reboot();
        } else {
            blt_ota_reset(); //must reset OTA flow status for next OTA
        }
    }


    return err_flag;
}


#endif //end of HARDWARE_SECURE_BOOT_SUPPORT_EN
