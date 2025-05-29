/********************************************************************************************************
 * @file    ble_smp.h
 *
 * @brief   This is the header file for TLSR/TL
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
#pragma once


#define BLE_SM_MTU                          65

/** Procedure timeout; 30 seconds. */
#define BLE_SM_TIMEOUT_MS                   (30000)

/** If not defined, define BIT() */
#ifndef BIT
#define BIT(i)                              (1<<(i))
#endif

/**
 * @brief Definition SMP command codes
 * core_v5.4 Vol 3, Part H, 3.2 Table 3.3: SMP command codes
 */
enum smp_opcode {
    BLE_SM_OP_PAIR_REQ                     = 0x01,
    BLE_SM_OP_PAIR_RSP                     = 0x02,
    BLE_SM_OP_PAIR_CFM                     = 0x03,
    BLE_SM_OP_PAIR_RANDOM                  = 0x04,
    BLE_SM_OP_PAIR_FAIL                    = 0x05,
    BLE_SM_OP_ENC_INFO                     = 0x06,
    BLE_SM_OP_MASTER_ID                    = 0x07,
    BLE_SM_OP_ID_INFO                      = 0x08,
    BLE_SM_OP_ID_ADDR_INFO                 = 0x09,
    BLE_SM_OP_SIGN_INFO                    = 0x0a,
    BLE_SM_OP_SEC_REQ                      = 0x0b,
    BLE_SM_OP_PAIR_PUBLIC_KEY              = 0x0c,
    BLE_SM_OP_PAIR_DHKEY_CHECK             = 0x0d,
    BLE_SM_OP_PAIR_KEYPRESS_NTF            = 0x0e,

    BLE_SMP_OP_ENC_END                     = 0xFF, //TLK defined
};

/**
 * @brief Definition SMP security level
 */
enum smp_security_level {
    LE_SECURITY_MODE_1_LEVEL_1 = BIT(0), /** LE_Security_Mode_1_Level_1: No authentication, no encryption */
    LE_SECURITY_MODE_1_LEVEL_2 = BIT(1), /** LE_Security_Mode_1_Level_2: Unauthenticated pairing with encryption */
    LE_SECURITY_MODE_1_LEVEL_3 = BIT(2), /** LE_Security_Mode_1_Level_3: Authenticated pairing with encryption */
    LE_SECURITY_MODE_1_LEVEL_4 = BIT(3), /** LE_Security_Mode_1_Level_4: Secure Connections pairing with encryption */

    LE_SECURITY_MODE_1 = (LE_SECURITY_MODE_1_LEVEL_1 | LE_SECURITY_MODE_1_LEVEL_2 | LE_SECURITY_MODE_1_LEVEL_3 | LE_SECURITY_MODE_1_LEVEL_4),
};

/** 
 * @brief Definition SMP first pairing or connecting back
 */
enum smp_reconn_type {
    RECOON_TYPE_STD_PAIR = 0,   /**< Standard pairing mode */
    RECOON_TYPE_CONN_BACK = 1,  /**< Connecting back mode */
};

/** 
 * @brief Definition SMP pairing request configuration
 */
enum smp_sec_req_cfg {
    SEC_REQ_NOT_SEND = 0,        /**< do not send "security request" after link layer connection established */
    SEC_REQ_IMM_SEND = BIT(0),   /**< "IMM" refer to immediate, send "security request" immediately after link layer connection established */
    SEC_REQ_PEND_SEND = BIT(1),  /**< "PEND" refer to pending,  pending "security request" for some time after link layer connection established, when pending time arrived. send it */
}  __attribute__((packed));;

 /**
  * @brief Definition SMP pairing request configuration
  */
 enum smp_pair_req_cfg {
    PAIR_REQ_SEND_UPON_SEC_REQ = BIT(0), /**< central send "pairing request" when received peripheral's "security request" */
    PAIR_REQ_AUTO_SEND = BIT(1),         /**< central send "pairing request" automatically, regardless of "security request" */
}  __attribute__((packed));

struct smp_initiate_cfg {
    /* central pair request configuration */
    struct {
        enum smp_pair_req_cfg new_conn_cfg;      
        enum smp_pair_req_cfg re_conn_cfg;
        bool manual_smp_start;
    } central;

    /* peripheral security request configuration */
    struct {
        enum smp_sec_req_cfg new_conn_cfg;
        enum smp_sec_req_cfg re_conn_cfg;
        uint16_t pending_send_ms;
    } peripheral;
};

/**
 * @brief Definition SMP local device pairing status
 */
enum smp_dev_pairing_status {
    NO_LTK_OR_STK               = 0,    /**< No LTK or STK found */
    UNAUTH_LTK_OR_STK           = 1,    /**< Unauthenticated LTK(with or without Secure Connections) or Unauthenticated STK found */
    AUTH_LTK_OR_STK             = 2,    /**< Authenticated LTK without Secure Connections or Authenticated STK found */
    AUTH_LTK_WITH_SC            = 3,    /**< Authenticated LTK with Secure Connections found */
};

/**
 * @brief Definition SMP pairing method
 */
enum smp_method {
    SMP_METHOD_UNSPECIFIED  = 0,            /** < LE_Security_Mode_1_Level_1: No authentication, no encryption    */
    SMP_METHOD_LEGACY_JW     = BIT(1),        /** < LE_Security_Mode_1_Level_2: Legacy JustWorks                    */
    SMP_METHOD_LESC_JW         = BIT(2),        /** < LE_Security_Mode_1_Level_2: LESC JustWorks                      */
    SMP_METHOD_LEGACY_PKI     = BIT(3),        /** < LE_Security_Mode_1_Level_3: Legacy Passkey Entry input          */
    SMP_METHOD_LEGACY_PKD     = BIT(4),        /** < LE_Security_Mode_1_Level_3: Legacy Passkey Entry display        */
    SMP_METHOD_LEGACY_OOB     = BIT(5),        /** < LE_Security_Mode_1_Level_3: Legacy Out of Band                  */
    SMP_METHOD_LESC_PKI     = BIT(6),        /** < LE_Security_Mode_1_Level_4: LESC Passkey Entry input            */
    SMP_METHOD_LESC_PKD     = BIT(7),        /** < LE_Security_Mode_1_Level_4: LESC Passkey Entry display          */
    SMP_METHOD_LESC_NC         = BIT(8),          /** < LE_Security_Mode_1_Level_4: LESC Numeric Comparison             */
    SMP_METHOD_LESC_OOB     = BIT(9),        /** < LE_Security_Mode_1_Level_4: LESC Out of Band                    */
    /* combination methods */
    SMP_METHOD_JW           = SMP_METHOD_LEGACY_JW  | SMP_METHOD_LESC_JW,  /** < JustWorks method, Legacy or LESC */
    SMP_METHOD_PKD          = SMP_METHOD_LEGACY_PKD | SMP_METHOD_LESC_PKD, /** < Passkey Entry Display method, Legacy or LESC */
    SMP_METHOD_PKI          = SMP_METHOD_LEGACY_PKI | SMP_METHOD_LESC_PKI, /** < Passkey Entry Input method, Legacy or LESC */
    SMP_METHOD_PKE          = SMP_METHOD_PKD | SMP_METHOD_PKI,             /** < Passkey Entry method, PKD or PKI */
    SMP_METHOD_LESC_PKE     = SMP_METHOD_LESC_PKD | SMP_METHOD_LESC_PKI,   /** < LESC Passkey Entry method, LESC_PKD or LESC_PKI */
    SMP_METHOD_OOB          = SMP_METHOD_LEGACY_OOB | SMP_METHOD_LESC_OOB, /** < Out of Band method, Legacy or LESC */
};

//See the Core_v5.0(Vol 3/Part C/10.2, Page 2067) for more information.
//enum le_security_mode_level {
//    LE_Security_Mode_1_Level_1 = BIT(0), No_Authentication_No_Encryption = BIT(0), No_Security = BIT(0),
//    LE_Security_Mode_1_Level_2 = BIT(1), Unauthenticated_Pairing_with_Encryption = BIT(1),
//    LE_Security_Mode_1_Level_3 = BIT(2), Authenticated_Pairing_with_Encryption = BIT(2),
//    LE_Security_Mode_1_Level_4 = BIT(3), Authenticated_LE_Secure_Connection_Pairing_with_Encryption = BIT(3),
//
//    LE_Security_Mode_2_Level_1 = BIT(4), Unauthenticated_Pairing_with_Data_Signing = BIT(4),
//    LE_Security_Mode_2_Level_2 = BIT(5), Authenticated_Pairing_with_Data_Signing = BIT(5),
//
//    LE_Security_Mode_1 = (LE_Security_Mode_1_Level_1 | LE_Security_Mode_1_Level_2 | LE_Security_Mode_1_Level_3 | LE_Security_Mode_1_Level_4)
//};

#define SMP_ENC_KEY_SIZE_MINIMUM    7
#define SMP_ENC_KEY_SIZE_MAXIMUM    16

/**
 *  @brief Definition SMP IO capability values,
 * core_v5.4 Vol 3, Part H, 3.5.1(table 3.4).
*/
enum smp_io_capability {
    SMP_IO_CAP_DISPLAY_ONLY         = 0x00, /** < Display Only */
    SMP_IO_CAP_DISPLAY_YES_NO       = 0x01, /** < Display Yes No */
    SMP_IO_CAP_KEYBOARD_ONLY        = 0x02, /** < Keyboard Only */
    SMP_IO_CAP_NO_INPUT_NO_OUTPUT   = 0x03, /** < No Input No Output */
    SMP_IO_CAP_KEYBOARD_DISPLAY     = 0x04, /** < Keyboard Display */
    SMP_IO_CAP_RFU,                         /** < Reserved for future use */
};

/**
 *  @brief Definition SMP OOB data present values,
 * core_v5.4 Vol 3, Part H, 3.5.1(table 3.5).
*/
enum smp_oob_data_flag {
    SMP_OOB_DATA_FLAG_NOT_PRESENT    = 0x00,    /** < OOB Authentication data not present */
    SMP_OOB_DATA_FLAG_REMOTE_PRESENT = 0x01,    /** < OOB Authentication data from remote device present */
    SMP_OOB_DATA_FLAG_RFU,                      /** < Reserved for future use */
};

/**
 *  @brief Definition bonding flags in authentication requirements flags,
 * core_v5.4 Vol 3, Part H, 3.5.1(table 3.6).
*/
enum smp_bonding_flag {
    SMP_BONDING_FLAG_NO_BONDING = 0x00, /** < No bonding */
    SMP_BONDING_FLAG_BONDING    = 0x01, /** < bonding */
    SMP_BONDING_FLAG_RFU1       = 0x02, /** < Reserved for future use */
    SMP_BONDING_FLAG_RFU2       = 0x03, /** < Reserved for future use */
};

/**
 *  @brief Definition pairing failed reason codes,
 * core_v5.4 Vol 3, Part H, 3.5.5(table 3.7).
*/
enum smp_pairing_failed_reason {
    //  The user input of passkey failed, for example, the user cancelled the operation 
    SMP_FAILED_PASSKEY_ENTRY_FAILED = 0x01,
    // The OOB data is not available
    SMP_FAILED_OOB_NOT_AVAILABLE = 0x02,
    // The pairing procedure cannot be performed as authentication requirements
    // cannot be met due to IO capabilities of one or both devices
    SMP_FAILED_AUTH_REQ = 0x03,
    // The confirm value does not match the calculated compare value
    SMP_FAILED_CONFIRM_VALUE_FAILED = 0x04,
    // Pairing is not supported by the device
    SMP_FAILED_PAIRING_NOT_SUPPORTED = 0x05,
    // The resultant encryption key size is not long enough for the security requirements of this device
    SMP_FAILED_ENCRYPTION_KEY_SIZE = 0x06,
    // The SMP command received is not supported on this device
    SMP_FAILED_COMMAND_NOT_SUPPORTED = 0x07,
    // Pairing failed due to an unspecified reason
    SMP_FAILED_UNSPECIFIED_REASON = 0x08,
    // Pairing or authentication procedure is disallowed because too little time has elapsed since last pairing request or security request
    SMP_FAILED_REPEATED_ATTEMPTS = 0x09,
    // The Invalid Parameters error code indicates that the command length is invalid or that a parameter is outside of the specified range.
    SMP_FAILED_INVALID_PARAMETERS = 0x0A,
    // Indicates to the remote device that the DHKey Check value received doesn't match the one calculated by the local device.
    SMP_FAILED_DHKEY_CHECK_FAILED = 0x0B,
    // Indicates that the confirm values in the numeric comparison protocol do not match
    SMP_FAILED_NUMERIC_COMPARISION_FAILED = 0x0C,
    // Indicates that the pairing over the LE transport failed due to a Pairing Request sent over the BR/EDR transport in progress.
    SMP_FAILED_BR_EDR_PAIRING_IN_PROGRESS = 0x0D,
    // Indicates that the BR/EDR Link Key generated on the BR/EDR transport cannot be used to derive and distribute keys for the LE transport 
    // or the LE LTK generated on the LE transport cannot be used to derive a key for the BR/EDR transport.
    SMP_FAILED_CTKD_NOT_ALLOWED = 0x0E,
    // Indicates that the device chose not to accept a distributed key.
    SMP_FAILED_KEY_REJECTED = 0x0F,
};

/**
 *  @brief Definition notification type in Pairing Keypress Notification PDU,
 * core_v5.4 Vol 3, Part H, 3.5.8(table 3.8).
*/
enum smp_notification_type {
    SMP_PASSKEY_ENTRY_STARTED   = 0x00, /** < Passkey entry started */
    SMP_PASSKEY_DIGIT_ENTERED   = 0x01, /** < Passkey digit entered */
    SMP_PASSKEY_DIGIT_ERASED    = 0x02, /** < Passkey digit erased */
    SMP_PASSKEY_CLEARED         = 0x03, /** < Passkey cleared */
    SMP_PASSKEY_ENTRY_COMPLETED = 0x04, /** < Passkey entry completed */
};

/**
 *  @brief enum RFU field check macro for SMP IO capability, OOB data present, and bonding flags,
 * core_v5.4 Vol 3, Part H, 3.5.1(table 3.4/table 3.5/table 3.6).
*/
#define SMP_IO_CAP_CHECK_RFU(ioCap)             ((ioCap) >= SMP_IO_CAP_RFU)
#define SMP_OOB_DATA_CHECK_RFU(oobData)         ((oobData) >= SMP_OOB_DATA_FLAG_RFU)
#define SMP_BONDING_FLAG_CHECK_RFU(bondingFlag) (((bondingFlag) == SMP_BONDING_FLAG_RFU1) || ((bondingFlag) == SMP_BONDING_FLAG_RFU2))

/**
 *  @brief Definition SMP authentication requirements flags,
 * core_v5.4 Vol 3, Part H, 3.5.1(figure 3.3).
*/
struct ble_smp_auth_req {
    uint8_t bondingFlags : 2;               /** < refer to enum smp_bonding_flag */
    uint8_t MITM : 1;                       /** < man-in-the-middle */
    uint8_t sc : 1;                         /** < LE secure connections */
    uint8_t keypress : 1;                   /** < the Passkey Entry protocol */
    uint8_t ct2 : 1;                        /** < 1 upon transmission to indicate support for the h7 function.*/
    uint8_t rfu : 2;                        /** < Reserved for future use */
} __attribute__((packed));

/**
 *  @brief Definition SMP key distribution,
 * core_v5.4 Vol 3, Part H, 3.6.1(figure 3.11).
*/
struct ble_smp_key_dist {
    uint8_t encKey : 1;                     /** < EDIV and rand */
    uint8_t idKey : 1;                      /** < the device shall distribute IRK and public device or static random address using Identity Address Information */
    uint8_t signKey : 1;                    /** < the device shall distribute CSRK using the Signing Information command */
    uint8_t linkKey : 1;                    /** < the device would like to derive the Link Key from the LTK */
    uint8_t rtu : 4;                        /** < Reserved for future use */
} __attribute__((packed));

/**
 * @brief Deifnition key distribution value
 */
typedef union {
    struct ble_smp_key_dist key_dist;
    uint8_t key_dist_value;
} ble_smp_key_dist_t;

/**
 * @brief Definition SMP command format
 * core_v5.4 Vol 3, Part H, 3.3
 */
struct smp_cmd_fmt {
    uint8_t opcode;
    uint8_t data[0];
} __attribute__((packed));

/**
 *  @brief Definition format of Pairing Request / Response PDU,
 * core_v5.4 Vol 3, Part H, 3.5.1/3.5.2(figure 3.2/figure 3.4).
*/
struct ble_smp_pairing_cmd {
    uint8_t code;                           /** < 0x01/0x02 SMP_PAIRING_REQ / SMP_PAIRING_RSP */
    uint8_t ioCapability;                   /** < refer to enum smp_io_capability */
    uint8_t oobDataFlag;                    /** < refer to enum smp_oob_data_flag */
    union {
        struct ble_smp_auth_req authReq;    /** < authentication requirements flags */
        uint8_t authReqValue;
    };
    uint8_t maxEncKeySize;                  /** < Maximum Encryption Key Size */
    union {
        struct ble_smp_key_dist initKey;    /** <  Initiator Key Distribution / Generation */
        uint8_t initKeyValue;
    };
    union {
        struct ble_smp_key_dist rspKey;     /** <  Responder Key Distribution / Generation */
        uint8_t rspKeyValue;
    };
} __attribute__((packed));

/**
 *  @brief Definition format of Pairing Confirm PDU,
 * core_v5.4 Vol 3, Part H, 3.5.3(figure 3.5).
*/
struct ble_smp_pairing_confirm {
    uint8_t code;                           /** < 0x03 SMP_PAIRING_CONFIRM */
    uint8_t confirm[16];                    /** < Confirm Value */
} __attribute__((packed));

/**
 *  @brief Definition format of Pairing Random PDU,
 * core_v5.4 Vol 3, Part H, 3.5.4(figure 3.6).
*/
struct ble_smp_pairing_random {
    uint8_t code;                           /** < 0x04 SMP_PAIRING_RANDOM */
    uint8_t random[16];                     /** < random Value */
} __attribute__((packed));

/**
 *  @brief Definition format of Pairing Failed PDU,
 * core_v5.4 Vol 3, Part H, 3.5.5(figure 3.7).
*/
struct ble_smp_pairing_failed {
    uint8_t code;                           /** < 0x05 SMP_PAIRING_FAILED */
    uint8_t reason;                         /** < refer to enum smp_pairing_failed_reason */
} __attribute__((packed));

/**
 *  @brief Definition format of Encryption Information PDU,
 * core_v5.4 Vol 3, Part H, 3.6.2(figure 3.12).
*/
struct ble_smp_encryption_info {
    uint8_t code;                           /** < 0x06 SMP_ENCRYPTION_INFORMATION */
    uint8_t ltk[16];                        /** < The generated LTK value being distributed */
} __attribute__((packed));

/**
 *  @brief Definition format of Central Identification PDU,
 * core_v5.4 Vol 3, Part H, 3.6.3(figure 3.13).
*/
struct ble_smp_central_id {
    uint8_t code;                           /** < 0x07 SMP_CENTRAL_IDENTIFICATION */
    uint16_t ediv;                          /** < Diversifier(EDIV) */
    uint64_t rand_val;                      /** < Random number(RAND) */
} __attribute__((packed));

/**
 *  @brief Definition format of Identity Information  PDU,
 * core_v5.4 Vol 3, Part H, 3.6.4(figure 3.14).
*/
struct ble_smp_id_info {
    uint8_t code;                           /** < 0x08 SMP_IDENTITY_INFORMATION */
    uint8_t irk[16];                        /** < Identity Resolving Key */
} __attribute__((packed));

/**
 *  @brief Definition format of Identity Address Information PDU,
 * core_v5.4 Vol 3, Part H, 3.6.5(figure 3.15).
*/
struct ble_smp_id_addr_info {
    uint8_t code;                           /** < 0x09 SMP_IDENTITY_ADDRESS_INFORMATION */
    uint8_t addrType;                       /** < 0x00 public; 0x01 static random */
    uint8_t bd_addr[6];                     /** < device address */
} __attribute__((packed));

/**
 *  @brief Definition format of Signing Information PDU,
 * core_v5.4 Vol 3, Part H, 3.6.6(figure 3.16).
*/
struct ble_smp_signing_info {
    uint8_t code;                           /** < 0x0A SMP_SIGNING_INFORMATION */
    uint8_t csrk[16];                       /** < Signature Key(CSRK) */
} __attribute__((packed));

/**
 *  @brief Definition format of Security Request PDU,
 * core_v5.4 Vol 3, Part H, 3.6.7(figure 3.17).
*/
struct ble_smp_security_request {
    uint8_t code;                           /** < 0x0B SMP_SECURITY_REQ */
    union {
        struct ble_smp_auth_req authReq;   /** < authentication requirements flags */
        uint8_t authReqValue;
    };
} __attribute__((packed));

/**
 *  @brief Definition format of Pairing Public Key PDU,
 * core_v5.4 Vol 3, Part H, 3.5.6(figure 3.8).
*/
struct ble_smp_pairing_public_key {
    uint8_t code;                           /** < 0x0C SMP_PAIRING_PUBLIC_KEY */
    uint8_t keyX[32];                       /** < Public Key X */
    uint8_t keyY[32];                       /** < Public Key Y */
} __attribute__((packed));

/**
 *  @brief Definition format of Pairing DHKey Check PDU,
 * core_v5.4 Vol 3, Part H, 3.5.7(figure 3.9).
*/
struct ble_smp_pairing_dhkey_check {
    uint8_t code;                           /** < 0x0D SMP_PAIRING_DH_KEY_CHECK */
    uint8_t dhkeyCheck[16];                 /** < DH Key check(E) */
} __attribute__((packed));

/**
 *  @brief Definition format of Pairing Keypress Notification PDU,
 * core_v5.4 Vol 3, Part H, 3.5.7(figure 3.10).
*/
struct ble_smp_keypress_notification {
    uint8_t code;                           /** < 0x0E SMP_KEYPRESS_NOTIFICATION */
    uint8_t notificationType;               /** < refer to enum smp_notification_type */
} __attribute__((packed));


bool ble_smp_cancel_auth(uint16_t conn_handle);

int ble_sm_keypress_notify(uint16_t conn_handle, enum smp_notification_type ntf_type);

int ble_host_smp_send_data_sync(struct ble_host_conn *conn, const uint8_t *p_data, uint16_t data_len);
