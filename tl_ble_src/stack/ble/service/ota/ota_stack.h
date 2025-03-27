/********************************************************************************************************
 * @file    ota_stack.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2020
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
#ifndef STACK_BLE_SERVICE_OTA_STACK_H_
#define STACK_BLE_SERVICE_OTA_STACK_H_

#include "stack/ble/ble_common.h"
#include "ota_server.h"

#define         OTA_RESULT_ORDER_CHANGE                             1

#define         FW_MIN_SIZE                                         0x04000  //16K



#ifndef         DBG_OTA_FLOW
#define         DBG_OTA_FLOW                                        0
#endif

#ifndef         OTA_FIRMWARE_RESUME_ENABLE
#define         OTA_FIRMWARE_RESUME_ENABLE                          0
#endif


#ifndef         BLE_OTA_FW_CHECK_EN
#define         BLE_OTA_FW_CHECK_EN                                 1
#endif


#define         OTA_FLOW_VERSION                                    BIT(0)
#define         OTA_FLOW_START                                      BIT(1)
#define         OTA_FLOW_DATA_COME                                  BIT(2)
#define         OTA_FLOW_VALID_DATA                                 BIT(3)
#define         OTA_FLOW_GET_SIZE                                   BIT(5)
#define         OTA_FLOW_END                                        BIT(6)


#define         OTA_STEP_IDLE                                       0
#define         OTA_STEP_START                                      BIT(0)
#define         OTA_STEP_DATA                                       BIT(1)
#define         OTA_STEP_FEEDBACK                                   BIT(2)  //feedback OTA result to peer device
#define         OTA_STEP_FINISH                                     BIT(3)



#define         DATA_PENDING_VERSION_RSP                            1
#define         DATA_PENDING_OTA_RESULT                             2
#define         DATA_PENDING_TERMINATE_CMD                          3


#define         OTA_ADD_MORE_CHECK_BEFORE_ERASE_FM_BAKUP_AREA       1

/**
 *  @brief
 */
typedef struct {
    u16     ota_cmd;
    u8      data[1];
} ota_cmd_t;





typedef struct {
    u8  ota_step;
    u8  otaResult;
    u8  version_accept;
    u8  resume_mode;   //1: resume_mode enable; 0: resume_mode disable

    u8  ota_busy;
    u8  fw_check_en;
    u8  fw_check_match;
    u8  flow_mask;

    u8  pdu_len;        //OTA valid data length
    u8  last_pdu_crc_offset;
    u8  last_actual_pdu_len;
    u8  last_valid_pdu_len;  //maximum value 240

    u8  data_pending_type;  //mark, and also data length
    u8  otaInit;
    u8  newFwArea_clear;
    s8  handle_offset;

    u8  process_timeout_100S_num;
    u8  process_timeout_100S_cnt;
    u8  write_16B_each_time;
    u8  align16_makeup_len; //0~15

    u16 local_version_num;  //default value:0; use API to set version
    u16 ota_write_attHandle;        //OTA write

    u16 ota_notify_attHandle;
    u16 ota_connHandle;

    u16 last_adr_index;
    u16 ota_cmd_adr;

    u16 schedule_pdu_num;
    u16 schdl_pduNum_mark;

    u16 schdl_pduNum_rpt;
    u8  fw_area_unlock; //unlock flash
    u8  ota_timeout_enable;

    u32 fw_crc_default;
    u32 fw_crc_init;
    //u32 fw_crc_addr;

    u32 firmware_size_byte;
    int flash_addr_mark;  //must be "s32", have special usage with "< 0"
    int cur_adr_index; //must be "s32"

    u32 feedback_begin_tick;  //add a OTA feedback timeout control, prevent some extreme case which lead to OTA flow blocked
    u32 ota_start_tick;
    u32 data_packet_tick;
    u32 process_timeout_us;
    u32 packet_timeout_us;
    u32 schedule_fw_size;

    #if (0)
    /* differ code mark attention: different code from single priority IRQ IC(e.g. B85m) for some special Flash
     * SONOS_ARCH_FLASH_ON_SINGLE_PRIORITY_IRQ_IC_WORKAROUND_EN */
        u32 cur_flash_addr;
        u16 hold_data_len;
        u16   rsdv;
    #endif
}ota_server_t;

extern ota_server_t blotaSvr;


extern      ota_resIndicateCb_t otaResIndicateCb;


typedef int (*ota_write_fw_callback_t)(u32, int, u8 *);
void        blt_ota_registerOtaWriteFwCallback (ota_write_fw_callback_t cb);



/* hidden API, if user do not use firmware check, call  this API to disable it. */
void        blc_ota_setOtaFirmwareCheckEnable(int en);

/* hidden API, if user do not want share same firmware check algorithm with all other customers,
 * he can call this API change crc32 init_value, pay attention that TestBench need also corresponding process. */
void        blc_ota_setFirmwareCheckCrcInitValue(u32 crc_init_value);

/**
 * @brief      This function is used to set OTA timeout enable
 *             attention 1: hidden API, do not show on SDK, but can introduced in Handbook.
 *             attention 2: OTA timeout control is very important, so it's enable by default.
 *                          User can disable it by setting "timeout_en" to 0
 *             attention 3: If this API is used, must be called after "blc_ota_initOtaServer_module" when initialization !!!
 * @param[in]  timeout_en - OTA time out control enable or disable
 * @return     none
 */
void        blc_ota_setOtaTimeoutEnable(int timeout_en);


int         blt_ota_server_main_loop(void);
int         blt_ota_server_terminate(u16 connHandle);

void        blt_ota_setResult(int next_step, int result);
void        blt_ota_reset(void);

void        blt_ota_writeBootMark(void);
int         ota_save_data(u32 flash_addr, int len, u8 * data);
/**
 * @brief      This function is used to response ota version request.
 * @param      none
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t   blt_ota_pushVersionRsp(void);


void        bls_ota_clearNewFwDataArea(void);






/************************************* OTA secure boot begin ********************************************/
#define OTA_SB_PUBKEY_SIGN_MSK              0xFF10


typedef struct {
    u8  hw_secBoot_en;  // security boot bit on efuse enable && user manual set enable by calling "blc_ota enableSecureBoot"
    u8  hw_fwEnc_en;   // FW encryption bit on efuse enable && user manual set enable by calling "blc_ota enableFirmwareEncryption"
    u8  secFlow_msk;
    u8  desc_area_write;

    u8  secur_infor_read_ok;
    u8  system_error;
    u8  sec_infor_checked;
    u8      rsdv[1];

    u16 last_sign_cmd;
    u16 hold_data_len;

    u32 cur_flash_addr;
    u32 old_fw_desc_addr;
    u32 new_fw_desc_addr;
}ota_sb_t;

extern ota_sb_t bltOtaSb;  //OTA secure boot


//OTA secure boot type
#define OSB_TYPE_CHECK_SEC_INFO     1
#define OSB_TYPE_CLEAR              2
#define OSB_TYPE_START              3
#define OSB_TYPE_PUBKEY_SIGN        4
#define OSB_TYPE_SAVE_DATA          5
#define OSB_TYPE_END                6
#define OSB_TYPE_FINISH             7

//OTA encryption read write type
#define OERW_TYPE_CHECK_SEC_INFO    1
#define OERW_TYPE_SAVE_DATA         2
#define OERW_TYPE_FINISH            3


#define SYSERR_EFUSE_READ_FAIL      BIT(0)
#define SYSERR_IDCODE_READ_FAIL     BIT(1)
#define SYSERR_DESC_ADDR_ERR        BIT(2)


#define OSB_FLOW_SIGN_RX_ALL        BIT(1) //signature (and public_key) received all
#define OSB_FLOW_PUBKEY_MATCH       BIT(2)
#define OSB_FLOW_SIGN_PASS          BIT(3)
#define OSB_FLOW_SAVE_DESC_OK       BIT(4)

typedef int (*ota_security_callback_t)(int, u32, int, void *);


extern ota_security_callback_t  ota_encryption_cb;
extern ota_security_callback_t  ota_sec_boot_cb;

void blt_ota_check_security_infor(void);

void blt_ota_secboot_reset(void);

int blt_ota_encryption_process(int type, u32, int, void *p);
int blt_ota_secure_boot_process(int type, u32, int, void *p);

int ota_enc_write_fw_boot_mark(void);

void ota_write_desc_boot_mark_secure_boot_mode(void);  //for secure boot mode, write boot flag on descriptor
int ota_write_fw_boot_mark_no_secure_boot_mode(void); //for no secure boot mode, write boot flag on firmware


int ota_security_save_data(u32 flash_addr, int len, u8 *data);
int ota_security_pack_256B_save_data(u32 flash_addr, int ota_len, u8 * ota_data);

int blt_osb_save_descriptor(void);

unsigned char  flash_dread_check(unsigned long addr, unsigned long len, unsigned char *buf);
/************************************* OTA secure boot end ********************************************/


#endif /* STACK_BLE_SERVICE_OTA_STACK_H_ */
