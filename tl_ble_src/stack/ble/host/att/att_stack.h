/********************************************************************************************************
 * @file    att_stack.h
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
#pragma  once


#include "tl_common.h"
#include "att.h"
#include "atts.h"
#include "attc.h"

#ifndef PREPARE_WRITE_QUEUE_MODE
#define PREPARE_WRITE_QUEUE_MODE            0
#endif

/*Opcode(1B) */
#define ATT_HDR_LEN                   1         /*!< \brief Attribute PDU header length */
#define ATT_AUTH_SIG_LEN              12        /*!< \brief Authentication signature length */
#define ATT_DEFAULT_MTU               23        /*!< \brief Default value of ATT_MTU */
#define ATT_MAX_MTU                   517       /*!< \brief Maximum value of ATT_MTU */
#define ATT_DEFAULT_PAYLOAD_LEN       20        /*!< \brief Default maximum payload length for most PDUs */



#define ATT_FIND_HANDLE_16_UUID       0x01
#define ATT_FIND_HANDLE_128_UUID      0x02


/**@}*/
/** \name ATT PDU Masks
 *
 */
/**@{*/
#define ATT_PDU_MASK_SERVER           0x01      /*!< \brief Server bit mask */
#define ATT_PDU_MASK_COMMAND          0x40      /*!< \brief Command bit mask */
#define ATT_PDU_MASK_SIGNED           0x80      /*!< \brief Auth signature bit mask */
/**@}*/


#define ATT_HANDLE_NONE               0x0000    /*!< \brief Handle none. */
#define ATT_HANDLE_START              0x0001    /*!< \brief Handle start. */
#define ATT_HANDLE_MAX                0xFFFF    /*!< \brief Handle max. */



//ATT Execute Write Request Flags
#define ATT_EXEC_WRITE_CANCEL         0x00
#define ATT_EXEC_WRITE_ALL            0x01

//CCC Notification/Indication Value.
#define ATT_CCC_NOTIFY                0x0001
#define ATT_CCC_INDICATE              0x0002

/** \name ATT PDU Length Fields
 * Length constants of PDU fixed length fields
 */
/**@{*/
#define ATT_ERR_RSP_LEN               5 /*!< \brief Error response length. */
#define ATT_MTU_REQ_LEN               3 /*!< \brief MTU request length. */
#define ATT_MTU_RSP_LEN               3 /*!< \brief MTU response length. */
#define ATT_FIND_INFO_REQ_LEN         5 /*!< \brief Find information request length. */
#define ATT_FIND_INFO_RSP_LEN         2 /*!< \brief Find information response length. */
#define ATT_FIND_TYPE_REQ_LEN         7 /*!< \brief Find type request length. */
#define ATT_FIND_TYPE_RSP_LEN         1 /*!< \brief Find type response length. */
#define ATT_READ_TYPE_REQ_LEN         5 /*!< \brief Read type request length. */
#define ATT_READ_TYPE_RSP_LEN         2 /*!< \brief Read type response length. */
#define ATT_READ_REQ_LEN              3 /*!< \brief Read request length. */
#define ATT_READ_RSP_LEN              1 /*!< \brief Read response length. */
#define ATT_READ_BLOB_REQ_LEN         5 /*!< \brief Read blob request length. */
#define ATT_READ_BLOB_RSP_LEN         1 /*!< \brief Read blob response length. */
#define ATT_READ_MULT_REQ_LEN         1 /*!< \brief Read multiple request length. */
#define ATT_READ_MULT_RSP_LEN         1 /*!< \brief Read multiple response length. */
#define ATT_READ_GROUP_TYPE_REQ_LEN   5 /*!< \brief Read group type request length. */
#define ATT_READ_GROUP_TYPE_RSP_LEN   2 /*!< \brief Read group type response length. */
#define ATT_WRITE_REQ_LEN             3 /*!< \brief Write request length. */
#define ATT_WRITE_RSP_LEN             1 /*!< \brief Write response length. */
#define ATT_WRITE_CMD_LEN             3 /*!< \brief Write command length. */
#define ATT_SIGNED_WRITE_CMD_LEN      (ATT_WRITE_CMD_LEN + ATT_AUTH_SIG_LEN) /*!< \brief Signed write command length. */
#define ATT_PREP_WRITE_REQ_LEN        5 /*!< \brief Prepared write command length. */
#define ATT_PREP_WRITE_RSP_LEN        5 /*!< \brief Prepared write response length. */
#define ATT_EXEC_WRITE_REQ_LEN        2 /*!< \brief Execute write request length. */
#define ATT_EXEC_WRITE_RSP_LEN        1 /*!< \brief Execute write response length. */
#define ATT_VALUE_NTF_LEN             3 /*!< \brief Value notification length. */
#define ATT_VALUE_IND_LEN             3 /*!< \brief Value indication length. */
#define ATT_VALUE_CNF_LEN             1 /*!< \brief Value confirmation length. */
#define ATT_READ_MULT_VAR_REQ_LEN     1 /*!< \brief Base read multiple variable request length. */
#define ATT_READ_MULT_VAR_RSP_LEN     1 /*!< \brief Base read multiple variable response length. */
#define ATT_PDU_MULT_VALUE_NTF_LEN    1 /*!< \brief Base multiple variable notification length. */


typedef enum {
    ATT_UNKNOWN = 0x00,
    ATT_NED_ACK = 0x40,
    ATT_REQUEST,
    ATT_INDICATION,

    ATT_NO_ACK = 0x80,
    ATT_COMMAND,
    ATT_RESPONSE,
    ATT_NOTIFICATION,
    ATT_CONFIRMATION,

} att_type_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x01 ATT_ERROR_RSP
    u8 reqOpcode;
    u16 attrHandle;
    u8 errorCode;
} blt_attr_errorRsp_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x02 ATT_EXCHANGE_MTU_REQ
    u16 clientRxMtu;
} blt_attr_exchangeMtuReq_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x03 ATT_EXCHANGE_MTU_RSP
    u16 serverRxMtu;
} blt_attr_exchangeMtuRsp_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x04 ATT_FIND_INFORMATION_REQ
    u16 startHandle;
    u16 endHandle;
} blt_attr_findInfoReq_t;


/* The format of the information data for ATT_FIND_INFORMATION_RSP */
#define ATT_INFO_FORMAT_16              0x01
#define ATT_INFO_FORMAT_128             0x02

struct att_info16 {
    u16 handle;
    u16 uuid;
};
struct att_info128 {
    u16 handle;
    u8  uuid[16];
};
typedef struct __attribute__((packed)) {
    u8 opcode;          //0x05 ATT_FIND_INFORMATION_RSP
    u8 format;
    u8 infoData[0];
} blt_attr_findInfoRsp_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x06 ATT_FIND_BY_TYPE_VALUE_REQ
    u16 startHandle;
    u16 endHandle;
    u16 attrType;
    u8 attrValue[0];
} blt_attr_findByTypeValueReq_t;

struct __attribute__((packed)) attr_handle_group {
    u16 startHandle;
    u16 endHandle;
};
typedef struct __attribute__((packed)) {
    u8 opcode;          //0x07 ATT_FIND_BY_TYPE_VALUE_RSP
    struct attr_handle_group list[0];   //least one group
} blt_attr_findByTypeValueRsp_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x08 ATT_READ_BY_TYPE_REQ
    u16 startHandle;
    u16 endHandle;
    u8 attrType[2];     //2 or 16 octet UUID
} blt_attr_readByTypeReq_t;

struct __attribute__((packed)) attr_data_list {
    u16 handle;
    u8 value[0];        //length - 2
};

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x09 ATT_READ_BY_TYPE_RSP
    u8 length;
    struct attr_data_list list[0];  //least one group,
} blt_attr_readByTypeRsp_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x0A ATT_READ_REQ
    u16 handle;
} blt_attr_readReq_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x0B ATT_READ_RSP
    u8 value[0];
} blt_attr_readRsp_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x0C ATT_READ_BLOB_REQ
    u16 handle;
    u16 valueOffset;
} blt_attr_readBlobReq_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x0D ATT_READ_BLOB_RSP
    u8 partAttrValue[0];
} blt_attr_readBlobRsp_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x0E ATT_READ_MULTIPLE_REQ
    u16 handles[2];     //two or more attribute handles
} blt_attr_readMultiReq_t;

typedef struct {
    u8 opcode;          //0x0F ATT_READ_MULTIPLE_RSP
    u8 values;
} blt_attr_readMultiRsp_t;


typedef struct __attribute__((packed)) {
    u8 opcode;          //0x10 ATT_READ_BY_GROUP_TYPE_REQ
    u16 startHandle;
    u16 endHandle;
    u8 attrGroupType[2];    //2 or 16 octet UUID
} blt_attr_readByGroupTypeReq_t;

struct __attribute__((packed)) attr_group_data_list {
    u16 startHandle;
    u16 endHandle;
    u8 attrValue[0];    //length - 4
};

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x11 ATT_READ_BY_GROUP_TYPE_RSP
    u8 length;
    struct attr_group_data_list data[0];    //least one group,
} blt_attr_readByGroupTypeRsp_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x12 ATT_WRITE_REQ
    u16 handle;
    u8 value[0];
} blt_attr_writeReq_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x13 ATT_WRITE_RSP
} blt_attr_writeRsp_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x16 ATT_PREPARE_WRITE_REQ
    u16 handle;
    u16 valueOffset;
    u8 partAttrValue[0];
} blt_attr_prepareWriteReq_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x17 ATT_PREPARE_WRITE_RSP
    u16 handle;
    u16 valueOffset;
    u8 partAttrValue[0];
} blt_attr_prepareWriteRsp_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x18 ATT_EXECUTE_WRITE_REQ
    u8 flag;
} blt_attr_executeWriteReq_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x19 ATT_EXECUTE_WRITE_RSP
} blt_attr_executeWriteRsp_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x1B ATT_HANDLE_VALUE_NTF
    u16 handle;
    u8 value[0];
} blt_attr_handleValueNtf_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x1D ATT_HANDLE_VALUE_IND
    u16 handle;
    u8 value[0];
} blt_attr_handleValueInd_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x1E ATT_HANDLE_VALUE_CFM
} blt_attr_handleValueCfm_t;


typedef struct __attribute__((packed)) {
    u8 opcode;          //0x20 ATT_READ_MULTIPLE_VARIABLE_REQ
    u16 handles[2];     //two or more attribute handles
} blt_attr_readMultiVarReq_t;

struct __attribute__((packed)) attr_value_tuple_list {
    u16 length;
    u8 attrValue[0];    //length
};
typedef struct __attribute__((packed)) {
    u8 opcode;          //0x21 ATT_READ_MULTIPLE_VARIABLE_RSP
    struct attr_value_tuple_list list[2];
} blt_attr_readMultiVarRsp_t;

struct __attribute__((packed)) attr_ntf_value_tuple_list {
    u16 handle;
    u16 length;
    u8 value[0];
};
typedef struct __attribute__((packed)) {
    u8 opcode;          //0x23 ATT_MULTIPLE_HANDLE_VALUE_NTF
    struct attr_ntf_value_tuple_list list[2];
} blt_attr_multiHandleValueNtf_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0x52 ATT_WRITE_CMD
    u16 handle;
    u8 value[0];
} blt_attr_writeCmd_t;

typedef struct __attribute__((packed)) {
    u8 opcode;          //0xD2 ATT_SIGNED_WRITE_CMD
    u16 handle;
    u8 value[0];
    u8 authSignature[12];
} blt_attr_signedWriteCmd_t;


typedef struct __attribute__((packed)) {
    //st_ll_conn_t *pConnCb;
    u16 scid;
    u16 connHandle;

    attr_pkt_t *attTxBuff;

}attConCb_t;



/////////////////////////////////////////////
//ATT Server process att_rx_packet
/////////////////////////////////////////////
u16 blt_atts_proc_exchangeMtuReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_findInfoReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_findByTypeValueReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_readByTypeReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_readReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_readBlobReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_readMultiReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_readByGroupTypeReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_readMultiVarReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_writeReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_prepareWriteReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_executeWriteReq(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_writeCmd(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_signedWriteCmd(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_atts_proc_handleValueCfm(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);

/// UUID compare ////
bool blt_atts_uuidCmp(const atts_attribute_t *pAttr, u8 uuidLen, const u8 *pUuid);
bool blt_atts_attrCmp(const atts_attribute_t *pAttr, u8 uuidLen, const u8 *pUuid);

u8  blt_atts_permissions(u16 connHandle, u8 permit, u8 handle, u8 permissions);
u16 blt_atts_findUuidInRange(u16 connHandle, u16 startHandle, u16 endHandle, u8 uuidLen, u8 *pUuid, const atts_attribute_t **pAttr, atts_group_t **pAttrGroup);
u16 blt_atts_findServiceGroupEnd(u16 connHandle, u16 startHandle);
u16 blt_atts_findInRange(u16 connHandle, u16 startHandle, u16 endHandle, const atts_attribute_t **pAttr);
u8 *blt_atts_l2capAttCmdHandler(u16 connHandle, u8 * p, u8 *ptx_buff);
bool    blt_atts_initServiceDiscoverTick(u16 connHandle);

/// ///
u16 blt_att_procAttrRxPkt(attConCb_t *pAttrConCb, attr_pkt_t* attr, u16 attrLen);
int blt_att_sendData_2_controller(u16 aclHandle, u16 scid, u8 *pHead, u8 headLen, u8 *pData, u16 dataLen);

/////////////////////////////////////////////
//ATT Client process att_rx_packet
/////////////////////////////////////////////
u16 blt_attc_proc_errorRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_exchangeMtuRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_findInfoRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_findByTypeValueRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_readByTypeRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_readRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_readBlobRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_readMultiRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_readMultiVarRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_readByGroupTypeRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_writeRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_prepareWriteRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_executeWriteRsp(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_handleValueNtf(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_handleValueInd(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);
u16 blt_attc_proc_multiHandleValueNtf(attConCb_t *pAttrConCb, attr_pkt_t *attr, u16 attrLen);

/////////////////////////////////////////////
//ATT process att_rx_packet
/////////////////////////////////////////////
//void blt_att_l2capAttRxHandler (u16 connHandle, l2cap_pkt_t *ptrAttr);
const atts_attribute_t* blt_atts_findByHandle(u16 connHandle, u16 handle, atts_group_t **pAttrGroup);


