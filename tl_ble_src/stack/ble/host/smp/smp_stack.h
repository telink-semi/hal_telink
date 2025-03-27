/********************************************************************************************************
 * @file    smp_stack.h
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
#ifndef STACK_BLE_SMP_STACK_H_
#define STACK_BLE_SMP_STACK_H_

#include "stack/ble/ble_config.h"
#include "stack/ble/controller/ll/ll_stack.h"
#include "stack/ble/device/device_stack.h"
#include "algorithm/crypto/crypto_alg.h"

/******************************* smp start ***********************************************************************/
#ifndef SMP_SC_OOB_EN
#define SMP_SC_OOB_EN                           1
#endif

//Legacy pairing used: If the LTK value in FLASH is tampered with, it will be checked when connecting back.
#ifndef SMP_LEGACY_LTK_VERIFICATION_EN
#define SMP_LEGACY_LTK_VERIFICATION_EN          1
#endif

//peripheral smp parameters configure separately
#ifndef SMP_PERIPHERAL_LEVEL_CFG_SEPARATE_EN
#define SMP_PERIPHERAL_LEVEL_CFG_SEPARATE_EN    1
#endif

#ifndef EXPAND_MASTER_BONDING_DEVICE_MAX_NUM_FOR_PAWR_AP
#define EXPAND_MASTER_BONDING_DEVICE_MAX_NUM_FOR_PAWR_AP 0
#endif

#define DBG_SMP_ERR_EN                      0 //TODO: all debug MACRO should remove at last
#if (DBG_SMP_ERR_EN)
#define SMP_ERR_DEBUG(x)                    do{irq_disable();write_reg32(0x40000, (x));while(1);}while(0)
#else
#define SMP_ERR_DEBUG(x)
#endif

// MACRO
#define ADDR_NOT_BONDED                     0xFF
#define ADDR_NEW_BONDED                     0xFE
#define ADDR_DELETE_BOND                    0xFD

#define KEY_FLAG_IDLE                       0xFF
#define KEY_FLAG_NEW                        0xFE
#define KEY_FLAG_FAIL                       0xFD

#define FlAG_BOND                           BIT(0)
#define FLAG_FASTSMP                        BIT(4)

#define SLAVE_TRIGGER_SMP_FIRST_PAIRING 0       //first pair, slave send security_request to trigger master's pairing&encryption
#define MASTER_TRIGGER_SMP_FIRST_PAIRING    BIT(0)

#define SLAVE_TRIGGER_SMP_AUTO_CONNECT      0       //auto connect, slave send security_request to trigger master's encryption
#define MASTER_TRIGGER_SMP_AUTO_CONNECT BIT(1)

#define SMP_TRANSPORT_SPECIFIC_KEY_START    0xEF
#define SMP_TRANSPORT_SPECIFIC_KEY_END      0

#define PAIRING_PHASE_IDLE                  0x00UL
#define PAIRING_PHASE_1_OK                  0x01UL
#define PAIRING_PHASE_2_ENC                 0x02UL
#define PAIRING_PHASE_2_OK                  0x03UL

#if (0) //Not used now
#define PASSKEY_TYPE_ENTRY_STARTED          0x00
#define PASSKEY_TYPE_DIGIT_ENTERED          0x01
#define PASSKEY_TYPE_DIGIT_ERASED           0x02
#define PASSKEY_TYPE_CLEARED                0x03
#define PASSKEY_TYPE_ENTRY_COMPLETED        0x04

#define KEY_MASK_ENC                        BIT(0)
#define KEY_MASK_IDENTITY                   BIT(1)
#define KEY_MASK_SIGN                       BIT(2)
#endif

/** @addtogroup SMP TK status definition
 * @{
 */
#define TK_ST_IDLE                          0x00UL
#define TK_ST_REQUEST                       BIT(0)
#define TK_ST_UPDATE                        BIT(1)
#define TK_ST_CONFIRM_PENDING               BIT(2)
#define TK_ST_NUMERIC_COMPARE               BIT(3)
#define TK_ST_NUMERIC_CHECK_YES             BIT(4)
#define TK_ST_NUMERIC_CHECK_NO              BIT(5)
#define TK_ST_NUMERIC_DHKEY_FAIL_PENDING    BIT(6)
#define TK_ST_NUMERIC_DHKEY_SUCC_PENDING    BIT(7)
/** @} end of group SMP TK status */


#define SMP_INFO_STORAGE_IN_FLASH           1
#define SMP_INFO_STORAGE_IN_EEPROM          2
#define SMP_INFO_STORAGE_OTHER_MCU          3

//define smp database(keys/peer device informations ...)
#ifndef SMP_DATABASE_INFO_SOURCE
#define SMP_DATABASE_INFO_SOURCE            SMP_INFO_STORAGE_IN_FLASH
#endif


/** @addtogroup SMP pairing opcode definition
 * @{
 */
#define SMP_OP_PAIRING_REQ                  1
#define SMP_OP_PAIRING_RSP                  2
#define SMP_OP_PAIRING_CONFIRM              3
#define SMP_OP_PAIRING_RANDOM               4
#define SMP_OP_PAIRING_FAIL                 5
#define SMP_OP_ENC_INFO                     6
#define SMP_OP_ENC_IDX                      7
#define SMP_OP_ENC_IINFO                    8
#define SMP_OP_ENC_IADR                     9
#define SMP_OP_ENC_SIGN                     0x0a
#define SMP_OP_SEC_REQ                      0x0b
#define SMP_OP_PAIRING_PUBLIC_KEY           0x0c
#define SMP_OP_PAIRING_DHKEY                0x0d
#define SMP_OP_KEYPRESS_NOTIFICATION        0x0e
#define SMP_OP_WAIT                         0x0f
#define SMP_OP_ENC_END                      0xFF //TLK defined
/** @} end of group SMP pairing opcode */


/** @addtogroup SMP encryption key size definition
 * @{
 */
#define ENCRYPTION_KEY_SIZE_MINIMUM         7
#define ENCRYPTION_KEY_SIZE_MAXIMUM         16
/** @} end of group SMP encryption key size */

#if (SMP_REAL_ENCRYPTION_BUSY_ENABLE)
extern u8 real_encryption_busy_enable;
#endif

typedef int (*smp_init_handler_t)(u16 conn, u8 *p);

typedef int (*smp_delete_handler_t)(u8 isMaster, u32 flash_addr);

#if (0)
typedef enum {
    Smp_None         = 0,
    Smp_Peripheral   = BIT(0),
    Smp_Central      = BIT(1),
}smp_role_t;
#endif


typedef struct{
    u8      secReq_conn;    //BIT<7:4> reConn;  BIT<3:0> newConn;
    u8      secReq_pending;
    u16     pending_ms;
}secReq_ctl_t;


typedef struct{
    u8      trigger_mask;
    u8      smp_begin_flg;
    u8      manual_smp_start;
    u8      smp_start_pending;
}smp_m_trig_t;  //master SMP trigger data type


typedef enum {
    JustWorks = 0,
    PK_Init_Display_Resp_Input = 1,
    PK_Resp_Display_Init_Input = 2,
    PK_BOTH_INPUT = 3,
    OOB_Authentication = 4,  //if (!SMP_SC_OOB_EN) legacy OOB or SC OOB
    Numeric_Comparison = 5,

#if(SMP_SC_OOB_EN)
    SC_OOB_Authentication = 6, //SC OOB
#endif

} stk_generationMethod_t;


//Local Device Pairing Status
//See the Core_v5.0(Vol 3/Part C/10.3.1/Table 10.2) for more information.

typedef enum {
    No_Bonded_No_LTK = 0x00,    //No_LTK
    Unauthenticated_LTK = 0x01,
    Authenticated_LTK_Legacy_Pairing = 0x02,
    Authenticated_LTK_Secure_Connection = 0x03,
} pairing_sts_t;

/* refer to BLE SPEC: Vol 3, Part H, 3.6 "Security in Bluetooth Low Energy" for more information. */
typedef union {
    struct{
        u8 bondingFlag : 2;
        u8 MITM : 1;
        u8 SC   : 1;
        u8 keyPress: 1;
        u8 rsvd: 3;
    };
    u8 authType;
}smp_authReq_t;


typedef struct __attribute__((packed)) {
    u8  code; //0x04
    u8 randomValue[16];
}smp_pairing_random_t;


typedef struct __attribute__((packed)) {
    u8  code; //0x03
    u8 confirmValue[16];
}smp_pairing_confirm_t;


typedef struct __attribute__((packed)) {
    u8 code; // 0x05
    u8 reason;
}smp_pairing_failed_t;


typedef struct __attribute__((packed)) {
    u8 code;//0x0b

    smp_authReq_t authReq;
}smp_secure_req_t;


typedef struct __attribute__((packed)) {
    u8  code;//0xa
    u8  signalKey[16];
}smp_signal_info_t;


typedef struct __attribute__((packed)) {
    u8 code;//0x9
    u8 addrType;
    u8 bdAddr[6];
}smp_id_addr_info_t;


typedef struct __attribute__((packed)) {
    u8 code;//0x8
    u8 idResolveKey[16];
}smp_id_info_t;


typedef struct __attribute__((packed)) {
    u8  code;//0x7
    u16 edivPtr[2];
    u8  masterRand[8];
}smp_master_id_t;


typedef struct __attribute__((packed)) {
    u8  code;//0x6
    u8  LTK[16];
}smp_enc_info_t;

// -------add core 4.2 ------
typedef struct __attribute__((packed)) {
    u8  code;//0xc
    u8  publicKeyX[32];
    u8  publicKeyY[32];
}smp_pairing_public_key_t;


typedef struct __attribute__((packed)) {
    u8  code;//0xd
    u8  DHKeyCheck[16];
}smp_DHkey_check_t;


typedef struct __attribute__((packed)) {
    u8  code;//0xe
    u8  notifyType;
}smp_key_notify_t;


typedef struct __attribute__((packed)) {
    /* data */
    u8 csrk[16];
    u32 signCounter;
} smp_secSigInfo_t;


typedef struct __attribute__((packed)) {
    u8  type;               //RFU(3)_MD(1)_SN(1)_NESN(1)-LLID(2)
    u8  rf_len;             //LEN(5)_RFU(3)
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u8 data[21];
}smp2llcap_type_t;


typedef union{
    struct {
        u8 encKey  : 1;
        u8 idKey   : 1;
        u8 sign    : 1;
        u8 linkKey : 4;
    };
    u8 keyIni;
}smp_keyDistribution_t;


typedef struct __attribute__((packed)) {
    u8 code;                //req = 0x01; rsp = 0x02;
    u8 ioCapability;
    u8 oobDataFlag;
    smp_authReq_t authReq;
    u8 maxEncrySize;

    smp_keyDistribution_t initKeyDistribution;
    smp_keyDistribution_t rspKeyDistribution;
}smp_pairing_req_rsp_t;


/*
 * smp parameter about peer device.
 * Attention:The use of pointers in an array of structures requires attention to prevent the
 * reference to the corresponding structure member variable addresses from being misaligned.
 */
_attribute_aligned_(4)
typedef struct{
    u8      rsvd1;   // use smp own param's, removed here. bond and key_size
    u8      peer_addr_type;  //address used in link layer connection
    u8      peer_conn_addr[6];

    u8      rsvd2;
    u8      peer_id_address_type;  //peer identity address information in key distribution, used to identify
    u8      peer_id_address[6];

    u8      peer_pairing_rand[16];

    u8      peer_confirm[16];

//  u8      peer_csrk[16];
    u8      peer_irk[16];
    u8      peer_ltk[16];   //peer device LTK

    u8      peer_random[8]; //peer device RANDOM

    u16     peer_ediv;      //peer device EDIV
    u16     rsvd3;          //must be aligned
}smp_param_peer_t;


/*
 * smp parameter about own device.
 * Attention:The use of pointers in an array of structures requires attention to prevent the
 * reference to the corresponding structure member variable addresses from being misaligned.
 */
_attribute_aligned_(4)
typedef struct{
    u8                      sc_pairing; // When pairing for the 1st time, whether using SC or legacy pairing method
    u8                      stk_method;  // final available stk generate method
    u8                      sc_passkey_cnt;//20 timer passkey entry for security connection
    u8                      peerKey_mask;// double check peer key distribute order


    u8                      pairing_tk[16];   // in security connection to keep own random
    u8                      own_ltk[16];    //used for generate ediv and random

    u8                      own_rand[16];
    u8                      own_csrk[16];  //todo: can be optimized
    u8                      own_irk[16];

    smp_authReq_t           auth_req;    // 1B
    u8                      own_addr_type;  // 1B current connection peer own type
    u8                      own_conn_addr[6];

    u8                      encrypt_key_size;//min(max key size of master, max key size of slave).
    smp_pairing_req_rsp_t   pairing_req; // 7B

    smp_pairing_req_rsp_t   pairing_rsp; // 7B
    u8                      sign_flag_en; //
    u8                      macKey[16];

#if (SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY)
    u8                      idenAdr_type;   //identity address type
    u8                      idenAdr_addr[6];//identity address
    u8                      cur_rpa;
#endif

#if SMP_SC_OOB_EN
    smp_sc_oob_data_t       *scoob_local;
    smp_sc_oob_data_t       *scoob_remote;
    smp_sc_oob_key_t        *scoob_local_key;
#endif

}smp_param_own_t;


_attribute_aligned_(4)
typedef struct{
#if (SMP_SEC_LEVEL_CHECK_EN)
    u32                     pairing_busy:   1;
    u32                     key_distribute: 1; //flag key distribute
    u32                     bonding_enable: 1;
    u32                     save_key_flag:  1;
    u32                     support_smp:    1; //smp support flg, master use only
    u32                     rsvd:           27;

    u8                      tk_status;
#else
    u8                      tk_status;
    u8                      pairing_busy;
    u8                      key_distribute;  //flag key distribute
    u8                      bonding_enable;

    u8                      save_key_flag;
#endif
    u8                      smpDistributeKeyOrder;//record key distribute order
    smp_keyDistribution_t   smp_DistributeKeySend; //send key transmit
    smp_keyDistribution_t   smp_DistributeKeyRecv; //receive key transmit

    u32                     smp_phase_chk;

    u32                     smp_timeout_start_tick;
}smp_st_t;  //SMP status data type


_attribute_aligned_(4)
typedef struct {
    u8      security_level;
    u8      bonding_mode;
    u8      IO_capability;
    u8      ecdh_debug_mode;//1:debug_mode,0:ecdh public/private key pairs distribute

    u8      MITM_protection;
    u8      oob_enable;
    u8      pairing_method;  //pairing_method
    u8      keyPress_en;

    u32     passKeyEntryDftTK; //when using the PasskeyEntry, set the default pincode displayed by our side and input by the other side.

} smp_prop_t; //SMP property parameters configured by user (slave can separate to multi_mac device )




typedef u8 * (*smp_sc_cmd_handler_t)(u16 conn, u8*p);
typedef void (*smp_sc_pushPkt_handler_t)(u16 conn, u8 type);
extern smp_sc_cmd_handler_t     func_smp_sc_proc;
extern smp_sc_pushPkt_handler_t func_smp_sc_pushPkt_proc;




extern  smp2llcap_type_t    smpResSignalPkt;
extern _attribute_aligned_(4)   secReq_ctl_t        blc_SecReq_ctrl;
extern _attribute_aligned_(4)   smp_m_trig_t        blt_smpTrig;
extern _attribute_aligned_(4)   smp_prop_t          *blms_p_prop;
extern _attribute_aligned_(4)   smp_param_own_t     *blms_p_own;
extern _attribute_aligned_(4)   smp_param_peer_t    *blms_p_peer;
extern _attribute_aligned_(4)   smp_st_t            *blms_p_sts;
extern _attribute_aligned_(4)   smp_prop_t          blt_smpProp[];
extern _attribute_aligned_(4)   smp_param_own_t     smp_param_own[];
extern _attribute_aligned_(4)   smp_param_peer_t    smp_param_peer[];
extern _attribute_aligned_(4)   smp_st_t            smp_sts_param[];

extern u8   smpPairingOwnIRK[16];
extern u8   smpPairingOwnCSRK[16];
extern u8   smpOwnPairingConfirm[16];

extern smp_init_handler_t   func_smp_init;

extern smp_delete_handler_t smp_delete_cb;

void    blt_smp_param_pre_init(void);
int     blt_smp_param_init (void);
u8*     blt_smp_pushPairingFailed(u8 failReason);
int     blt_smp_llEncryptionDone(u16 connHandle, u8 status, u8 enc_enable);
int     blt_smp_llEncryptionPause(u16 connHandle);
void    blt_smp_setPairingBusy(u16 connHandle, u8 busy);
int     blt_smp_sendSecurityRequest (u16 connHandle);
void    blt_smp_procSlaveSecurityRequest(u16 connHandle);
void    blt_smp_procCentralPairingRequest(u16 connHandle);
void    blt_smp_proc_pairing_loop(u16 connHandle);

int     bls_smp_llGetLtkReq(u16 connHandle, u8* random, u16 ediv);
u8 *    blt_smp_pushSmpCmdPkt(u16 connHandle, u8 type);
void    blt_smp_saveBondingKey(u16 connHandle);
u8 *    blt_smp_sendInfo(u16 connHandle);
void    blt_smp_procPairingEnd(u16 connHandle, u8 err_reason);
u8 *    blt_smp_l2capSmpCmdHandler(u16 connHandle, u8* p);
void    blt_smp_certTimeoutLoopEvt(u16 connHandle);
pairing_sts_t blt_smp_get_pairing_status(u16 connHandle);  //connHandle for future multiConnection, now use direct BLS_CONN_HANDLE is OK
void    blt_smp_encChangeEvt(u8 status, u16 connHandle, u8 enc_enable);
smp_keyDistribution_t blt_smp_setInitiatorKey (smp_param_own_t *pSmpOwn, u8 LTK_En, u8 IRK_En, u8 CSRK_En);
smp_keyDistribution_t blt_smp_setResponderKey (smp_param_own_t *pSmpOwn, u8 LTK_En, u8 IRK_En, u8 CSRK_En);
int     blt_smp_getStkGenMethod (smp_param_own_t *blms_p_own, int SC_en);
void    blt_smp_setCertTimeoutTick (u16 conn_handle, u32 t);

u8*     blt_smp_sc_handler(u16 connHandle, u8*p);
void    blt_smp_sc_pushPkt_handler(u16 connHandle, u8 type);

#if (MULTIPLE_LOCAL_DEVICE_ENABLE)
void    blc_smp_setSecurityLevel_periphr_by_device_index(int slave_dev_ind, le_security_mode_level_t  mode_level);
void    blc_smp_setPairingMethods_periphr_by_device_index(int slave_dev_ind, pairing_methods_t  method);
void    blc_smp_setIoCapability_periphr_by_device_index(int slave_dev_ind, io_capability_t ioCapability);
void    blc_smp_enableOobAuthentication_periphr_by_device_index(int slave_dev_ind, int OOB_en);
void    blc_smp_setBondingMode_periphr_by_device_index(int slave_dev_ind, bonding_mode_t mode);
void    blc_smp_enableAuthMITM_periphr_by_device_index(int slave_dev_ind, int MITM_en);
void    blc_smp_enableKeypress_periphr_by_device_index(int slave_dev_ind, int keyPress_en);
void    blc_smp_setEcdhDebugMode_periphr_by_device_index(int slave_dev_ind, ecdh_keys_mode_t mode);
void    blc_smp_setSecurityParameters_periphr_by_device_index(int slave_dev_ind, bonding_mode_t  bond_mode, int MITM_en, pairing_methods_t method,
                                                           int OOB_en, int keyPress_en,io_capability_t ioCapability);
#endif



#define smp_quickResolvPrivateAddr  aes_resolve_irk_rpa

/******************************* smp end *************************************************************************/






/******************************* smp_storage start ***********************************************************************/

#if (EXPAND_MASTER_BONDING_DEVICE_MAX_NUM_FOR_PAWR_AP)
    #define SMP_MASTER_BONDING_DEVICE_MAX_NUM       200
#else
    #define SMP_MASTER_BONDING_DEVICE_MAX_NUM       8
#endif

#if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
    #define SMP_SLAVE_BONDING_DEVICE_MAX_NUM        8
#else
    #define SMP_SLAVE_BONDING_DEVICE_MAX_NUM        4
#endif

#define     SMP_PARAM_NV_UNIT                       96
#define     SMP_MAX_PAIRED_NUM                      (SMP_MASTER_BONDING_DEVICE_MAX_NUM + \
                                                    LOCAL_DEVICE_NUM_MAX*SMP_SLAVE_BONDING_DEVICE_MAX_NUM)
#if (EXPAND_MASTER_BONDING_DEVICE_MAX_NUM_FOR_PAWR_AP)
    //smp storage area reaches alarm line 0 threshold
    #define     SMP_PARAM_CLEAN_INDEX_ALARM_LOW         (SMP_PARAM_NV_MAX_LEN - 2048 ) // max size - 2K
    //smp storage area reaches alarm line 1 threshold
    #define     SMP_PARAM_CLEAN_INDEX_ALARM_HIGH        (SMP_PARAM_NV_MAX_LEN - (SMP_PARAM_NV_UNIT<<1))
#else
    //smp storage area reaches alarm line 0 threshold
    #define     SMP_PARAM_CLEAN_INDEX_ALARM_LOW         ((SMP_PARAM_NV_MAX_LEN*3) >>2 ) //e.g.: 8k*3/4 -> 64*96
    //smp storage area reaches alarm line 1 threshold
    #define     SMP_PARAM_CLEAN_INDEX_ALARM_HIGH        (SMP_PARAM_NV_MAX_LEN - (SMP_PARAM_NV_UNIT<<1)) //e.g.: 8k -96*2 -> 84*96
#endif

#define     SMP_PARAM_NV_SEC_ADDR_START             (SMP_PARAM_NV_ADDR_START + SMP_PARAM_NV_MAX_LEN)
#define     SMP_PARAM_NV_SEC_ADDR_END               (SMP_PARAM_NV_SEC_ADDR_START + SMP_PARAM_NV_MAX_LEN - 1)

#define     FLAG_SMP_PARAM_SAVE_OLD                 0x5A  // 0101 1010  old storage, not used now!!!
                                                          // 10xx 1010  new storage,  xx: see "pairing_sts_t" definition
#define     FLAG_SMP_PARAM_SAVE_BASE                0x8A  // 1000 1010
#define     FLAG_SMP_PARAM_SAVE_UNAUTHEN            0x9A  // 1001 1010  new storage Unauthenticated_LTK
#define     FLAG_SMP_PARAM_SAVE_AUTHEN              0xAA  // 1010 1010  new storage Authenticated_LTK_Legacy_Pairing
#define     FLAG_SMP_PARAM_SAVE_AUTHEN_SC           0xBA  // 1011 1010  new storage Authenticated_LTK_Secure_Connection

#define     FLAG_SMP_PARAM_SAVE_PEER_SUP_AR         0x0A  // 0000 1010  new storage If Peer device supports address resolution
#define     FLAG_SMP_PARAM_SAVE_PEER_NSUP_AR        0x8A  // 1000 1010  new storage If Peer device not supports address resolution

#define     FLAG_SMP_PARAM_SAVE_PENDING             0xBF  // 1011 1111
#define     FLAG_SMP_PARAM_SAVE_ERASE               0x00  //

#define     FLAG_SMP_PARAM_MASK                     0x0F  // 0000 1111
#define     FLAG_SMP_PARAM_VALID                    0x0A  // 0000 1010
#define     FLAG_SMP_PAIRING_STATUS_MASK            0x30  // 0011 0000

#define     FLAG_SMP_PEER_SUP_AR_MASK               0x80  // 1000 0000

#define     FLAG_SMP_SECTOR_USE                     0x3C
#define     FLAG_SMP_SECTOR_CLEAR                   0x00

#define     FLAG_CONN_ROLE_MASTER                   BIT(7)
#define     MSK_SLAVE_DEV_IDX                       0x07  // [2:0] slave device index



typedef enum {
    FLASH_OP_SUCCESS = 0,
    FLASH_OP_FAIL    = 1,
} flash_op_sts_t;


typedef struct {
#if (SMP_DATABASE_INFO_SOURCE == SMP_INFO_STORAGE_IN_FLASH)
    u16 master_bond_flash_idx[SMP_MASTER_BONDING_DEVICE_MAX_NUM];                 //Attention: flash address offset here, 0xFFFF 64K max
    u16 slave_bond_flash_idx[LOCAL_DEVICE_NUM_MAX][SMP_SLAVE_BONDING_DEVICE_MAX_NUM];    //Attention: flash address offset here, 0xFFFF 64K max
#else
    //may be SRAM address if use other MCU store SMP information
#endif

//0x18
    u8 pairing_status[LL_MAX_ACL_CONN_NUM];
    u8 addrIndex[LL_MAX_ACL_CONN_NUM];   //different slave can not share this variable, master can share but not optimize
    u8 keyIndex[LL_MAX_ACL_CONN_NUM];    //different slave can not share this variable, master can share but not optimize

//0x24
    u8 master_cur_bondNum;
    u8 master_curIndex;
    u8 isBond_fastSmp;
    u8 slave_cur_bondNum[LOCAL_DEVICE_NUM_MAX];
} smp_bond_device_t;


typedef struct __attribute__((packed))  {
    u8 master_max_bondNum;
    u8 slave_max_bondNum;
    u8 index_update_method;
    u8 dev_exceed_max_strategy;
    u8 loc_irk_str;
    u8 dynamic_cfg_smp;
} smp_mng_t;
extern  smp_mng_t smpMng;


/**
 * @brief   local IRK generating strategy when controller use identity address in RF packet
 *          default use "LOCIRK_BINDING_WITH_DEVICE" if user not set
 */
typedef enum {
    LOCIRK_BINDING_WITH_DEVICE = 0,  //every device have unique local IRK, never change
    LOCIRK_RANDOM_GENERATE     = 1,  //random generate
}loc_irk_gen_str_t;

/**
 * @brief      This function is used to set local IRK generating strategy.
 * @param[in]  str - local IRK generating strategy.
 * @return     none.
 */
void        blc_smp_setLocalIrkGenerateStrategy (loc_irk_gen_str_t  str);


extern  int SMP_PARAM_NV_MAX_LEN;
extern  int smp_bond_device_flash_cfg_idx;
extern  smp_bond_device_t smpMStblBondDevice;


void            blt_smp_initBondingInfoFromFlash(void);
void            blt_smp_cleanBondingInfoStorage(void);

//Search
u32             blt_smp_searchBondingDevice_by_PeerMacAddr(u8 isMasterRole,u8 slaveDevIdx, u8 peer_addr_type, u8* peer_addr);

//Get
u8              blt_smp_getBondingFlag_by_FlashAddr(u32 flash_addr);
u8              blt_smp_getBondingIndex_by_FlashAddr(u8 isMaster, u8 slaveDevIdx, u32 flash_addr);

//Load
u32             bls_smp_loadLTK_by_EdivRand(u8 slaveDevIdx, u16 ediv, u8* random, u8 *ltk); //for slave only

//Delete
int             blt_smp_deleteBondingInfo_by_Index(u8 isMaster, u8 slaveDevIdx, u8 index, bool smp_del_cb);
int             blt_smp_deleteBondingInfo_by_PeerMacAddress(u8 isMaster, u8 slaveDevIdx, u8 peer_addr_type, u8* peer_addr);

    void            blt_smp_procBondingInfoIndexAlarm(void);

#ifndef CUSTOM_SMP_STORAGE
#define CUSTOM_SMP_STORAGE          1
#endif

//Update
u32             blt_smp_updateBondingInfoToNearestByIndex(u8 isMaster, u8 slaveDevIdx, u8 index);

/******************************* smp_storage end *************************************************************************/

#if CUSTOM_SMP_STORAGE
typedef u32 (*smp_info_custom_save_callback_t)(u16 connHandle, u32 current_addr, smp_param_save_t* smp_param);
typedef void (*smp_info_custom_load_callback_t)(u16 connHandle);
extern smp_info_custom_save_callback_t smp_info_custom_save_cb;
extern smp_info_custom_load_callback_t smp_info_custom_load_cb;
#endif


#endif /* STACK_BLE_SMP_STACK_H_ */
