/********************************************************************************************************
 * @file    att_stack_v0.h
 *
 * @brief   This is the header file for BLE SDK
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
#ifndef STACK_BLE_ATT_STACK_H_
#define STACK_BLE_ATT_STACK_H_


#ifndef ATT_HANDLE_VALUE_CUSTOM_EN
    #define ATT_HANDLE_VALUE_CUSTOM_EN 0
#endif


#ifndef HANDLE_VALUE_CUSTOM_DBG_EN
    #define HANDLE_VALUE_CUSTOM_DBG_EN 0
#endif


#define ATT_MTU_SIZE 23 //!< Minimum ATT MTU size

typedef struct __attribute__((packed))
{
    u8 *buff;
    u16 offset;
    u16 handle;
    u16 buffMaxLen;
} att_pre_write_buff_t;

typedef struct __attribute__((packed))
{
    u8 attHl_custom_en;
    u8 attHl_cusNum; //max 255 now
    u8 smpPairingHoldATT;
    u8 rsvd;
} att_mng_t;

extern att_mng_t bltAtt;


u8  *bls_att_l2capAttCmdHandler(u16 connHandle, u8 *p);
void blt_att_procMtuExgPending(u16 connHandle);


u16 blt_att_change_sdkAttHandle_to_customAttHandle(u16 sdk_attHl);
u16 blt_att_change_customAttHandle_to_sdkAttHandle(u16 custom_attHl);


void blt_att_processAttHandle_in_attCmd(rf_packet_l2cap_req_t *);


/**
 * @brief       Register Custom ATT table.
 *              hidden API, inform FAE or customers only when they have very strong requirement, to reduce our risk.
 * @param[in]   pAttCus - custom att table.
 * @param[in]   attHlConvert_num - table size.
 * @return[in]  0: success
 *              other: fail
 */
ble_sts_t blc_att_setAttributeHandleCustomTable(attHl_convert_t *pAttCus, int attHlConvert_num);


#endif /* ATT_STACK_H_ */
