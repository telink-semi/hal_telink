/********************************************************************************************************
 * @file    att_cmd.c
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
#include "att_cmd.h"

/**
 * Format of the ATT_ERROR_RSP PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Request Opcode In Error            | 1                 |
 * | Attribute Handle In Error          | 2                 |
 * | Error Code                         | 1                 |
 */
u16 blt_att_packageErrorRsp(u8 errOpcode, u16 errHandle, u8 errReason, attr_pkt_t *txBuf)
{
    if (!txBuf) {
        return 0;
    }
    u8 *buffer = &txBuf->opcode; //skip dataLen field

    U8_TO_STREAM(buffer, ATT_OP_ERROR_RSP);
    U8_TO_STREAM(buffer, errOpcode);
    U16_TO_STREAM(buffer, errHandle);
    U8_TO_STREAM(buffer, errReason);

    return buffer - &txBuf->opcode;
}

/**
 * Format of ATT_EXCHANGE_MTU_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Server Rx MTU                      | 2                 |
 */
int blt_att_packageExchangeMtuReq(u16 mtuSize, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_EXCHANGE_MTU_REQ;
    buffer[buffLen++] = mtuSize & 0xFF;
    buffer[buffLen++] = (mtuSize & 0xFF00) >> 8;


    return buffLen;
    ;
}

/**
 * Format of ATT_EXCHANGE_MTU_RSP PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Server Rx MTU                      | 2                 |
 */
u16 blt_att_packageExchangeMtuRsp(u16 mtu, attr_pkt_t *txBuf)
{
    u8 *buffer = &txBuf->opcode; //skip dataLen field

    U8_TO_STREAM(buffer, ATT_OP_EXCHANGE_MTU_RSP);
    U16_TO_STREAM(buffer, mtu)

    return buffer - &txBuf->opcode;
}

/**
 * Format of ATT_READ_BY_TYPE_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Starting Handle                    | 2                 |
 * | Ending Handle                      | 2                 |
 * | Attribute Type                     | 2 or 16           |
 */
int blt_att_packageReadByTypeReq(u16 startAttHandle, u16 endAttHandle, u8 *uuid, u8 uuidLen, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    if (uuidLen != 2 || uuidLen != 16) {
        return -1;
    }

    buffer[buffLen++] = ATT_OP_READ_BY_TYPE_REQ;
    buffer[buffLen++] = startAttHandle & 0xFF;
    buffer[buffLen++] = (startAttHandle & 0xFF00) >> 8;
    buffer[buffLen++] = endAttHandle & 0xFF;
    buffer[buffLen++] = (endAttHandle & 0xFF00) >> 8;
    memcpy(&buffer[buffLen], uuid, uuidLen);
    buffLen += uuidLen;


    return buffLen;
    ;
}

/**
 * Format of ATT_READ_BY_TYPE_RSP PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Length                             | 1                 |
 * | Attribute Data List                | 2 to (ATT_MTU-2)  |
 *
 *      Attribute Data List:
 *      +--------------------------+-------------------+
 *      |     Attribute Handle     |   Attribute Value |
 *      |             2            |    (Length - 2)   |
 */
int blt_att_packageReadByTypeRsp(u8 typeLen, u8 dataLen, u8 *pData, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_READ_BY_TYPE_RSP;
    buffer[buffLen++] = typeLen;  //The size of each attribute handle value pair
    memcpy(&buffer[buffLen], pData, dataLen);
    buffLen += dataLen;


    return buffLen;
    ;
}

/**
 * Format of ATT_READ_MULTIPLE_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Set Of Handles                     | 4 to (ATT_MTU-1)  |
 */
int blt_att_packageReadMultipleReq(u8 numHandles, u16 *pHandle, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    if (numHandles > 7) {
        return -1;
    }

    buffer[buffLen++] = ATT_OP_READ_MULTIPLE_REQ;
    while (numHandles--) {
        buffer[buffLen++] = ((*pHandle) & 0x00FF);
        buffer[buffLen++] = ((*pHandle) & 0xFF00) >> 8;
        pHandle++;
    }


    return buffLen;
    ;
}

/**
 * Format of ATT_READ_MULTIPLE_VARIABLE_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Set Of Handles                     | 4 to (ATT_MTU-1)  |
 */
int blt_att_packageReadMultVarReq(u8 numHandles, u16 *pHandle, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    if (numHandles > 10 || numHandles == 0) {
        return -1;
    }

    buffer[buffLen++] = ATT_OP_READ_MULTIPLE_VARIABLE_REQ;
    while (numHandles--) {
        buffer[buffLen++] = ((*pHandle) & 0x00FF);
        buffer[buffLen++] = ((*pHandle) & 0xFF00) >> 8;
        pHandle++;
    }


    return buffLen;
    ;
}

/**
 * Format of ATT_READ_MULTIPLE_VARIABLE_RSP PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Length Value Tuple List            | 4 to (ATT_MTU-1)  |
 *
 *      Length Value Tuple:
 *      +--------------------------+-------------------+
 *      |     Value Length         |   Attribute Value |
 *      |          2               |    Value Length   |
 */
int blt_att_packageReadMultVarRsp(u8 numVars, u16 *pVarLen, attr_pkt_t *txBuf) //TODO: package error, refer to <<Core_5.3 | Vol 3, Part F page 1442>>
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode;                                              //skip dataLen field

    if (numVars > 10 || numVars == 0) {
        return -1;
    }

    buffer[buffLen++] = ATT_OP_READ_MULTIPLE_VARIABLE_RSP;
    while (numVars--) {
        buffer[buffLen++] = ((*pVarLen) & 0x00FF);
        buffer[buffLen++] = ((*pVarLen) & 0xFF00) >> 8;
        pVarLen++;
    }


    return buffLen;
    ;
}

/**
 * Format of ATT_READ_BY_GROUP_TYPE_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Starting Handle                    | 2                 |
 * | Ending Handle                      | 2                 |
 * | Attribute Group Type               | 2 or 16           |
 */
int blt_att_packageReadByGroupTypeReq(u16 startAttHandle, u16 endAttHandle, u8 *uuid, u8 uuidLen, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    if (uuidLen != 2 || uuidLen != 16) {
        return -1;
    }

    buffer[buffLen++] = ATT_OP_READ_BY_GROUP_TYPE_REQ;
    buffer[buffLen++] = startAttHandle & 0xFF;
    buffer[buffLen++] = (startAttHandle & 0xFF00) >> 8;
    buffer[buffLen++] = endAttHandle & 0xFF;
    buffer[buffLen++] = (endAttHandle & 0xFF00) >> 8;
    memcpy(&buffer[buffLen], uuid, uuidLen);
    buffLen += uuidLen;


    return buffLen;
    ;
}

/**
 * Format of ATT_READ_BY_GROUP_TYPE_RSP PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Length                             | 1                 |
 * | Attribute Data List                | 2 to (ATT_MTU-2)  |
 *
 *      Attribute Data List:
 *      +--------------------+--------------------+------------------+
 *      |  Attribute Handle  |   End Group Handle |  Attribute Value |
 *      |          2         |          2         |   (Length - 4)   |
 */
int blt_att_packageReadByGroupTypeRsp(u8 typeLen, u8 *pData, u16 datalen, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_READ_BY_GROUP_TYPE_RSP;
    buffer[buffLen++] = typeLen;
    memcpy(&buffer[buffLen], pData, datalen);
    buffLen += datalen;


    return buffLen;
    ;
}

/**
 * Format of ATT_FIND_INFORMATION_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Starting Handle                    | 2                 |
 * | Ending Handle                      | 2                 |
 */
int blt_att_packageFindInfoReq(u16 startAttHandle, u16 endAttHandle, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_FIND_INFO_REQ;
    buffer[buffLen++] = startAttHandle & 0xFF;
    buffer[buffLen++] = (startAttHandle & 0xFF00) >> 8;
    buffer[buffLen++] = endAttHandle & 0xFF;
    buffer[buffLen++] = (endAttHandle & 0xFF00) >> 8;


    return buffLen;
    ;
}

/**
 * Format of ATT_FIND_INFORMATION_RSP PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Format                             | 1                 |
 * | Information Data                   | 4 to (ATT_MTU-2)  |
 */
int blt_att_packageFindInfoRsp(u8 format, u8 dataLen, u8 *pData, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_FIND_INFO_RSP;
    buffer[buffLen++] = format;
    memcpy(&buffer[buffLen], pData, dataLen);
    buffLen += dataLen;


    return buffLen;
    ;
}

/**
 * Format of ATT_FIND_BY_TYPE_VALUE_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Starting Handle                    | 2                 |
 * | Ending Handle                      | 2                 |
 * | Attribute Type                     | 2                 |
 * | Attribute Value                    | 0 to (ATT_MTU-7)  |
 */
int blt_att_packageFindByTypeReq(u16 startAttHdl, u16 endAttHdl, u8 *pUuid, u8 *pAttrValue, int valueLen, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_FIND_BY_TYPE_VALUE_REQ;
    buffer[buffLen++] = startAttHdl & 0xFF;
    buffer[buffLen++] = (startAttHdl & 0xFF00) >> 8;
    buffer[buffLen++] = endAttHdl & 0xFF;
    buffer[buffLen++] = (endAttHdl & 0xFF00) >> 8;
    memcpy(&buffer[buffLen], pUuid, 2);
    buffLen += 2;
    if (valueLen != 0) {
        memcpy(&buffer[buffLen], pAttrValue, valueLen);
        buffLen += valueLen;
    }


    return buffLen;
    ;
}

/**
 * Format of ATT_FIND_BY_TYPE_VALUE_RSP PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Information Data                   | 4 to (ATT_MTU-1)  |
 *
 *      Information Data (Handles Information List):
 *      +------------------------------------+-------------------+
 *      |    Found Attribute Handle          |   Group End Handle|
 *      |         2 octets                   |        2 octets   |
 */
int blt_att_packageFindByTypeRsp(u8 dataLen, u8 *pData, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    if (dataLen < 4 || dataLen % 4 != 0) {
        return -1;
    }

    buffer[buffLen++] = ATT_OP_FIND_BY_TYPE_VALUE_RSP;
    memcpy(&buffer[buffLen], pData, dataLen);
    buffLen += dataLen;


    return buffLen;
    ;
}

/**
 * Format of ATT_READ_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Attribute Handle                   | 2                 |
 */
int blt_att_packageReadReq(u16 attHandle, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_READ_REQ;
    buffer[buffLen++] = attHandle & 0xFF;
    buffer[buffLen++] = (attHandle & 0xFF00) >> 8;


    return buffLen;
    ;
}

/**
 * Format of ATT_READ_RSP PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Attribute Value                    | 0 to (ATT_MTU-1)  |
 */
int blt_att_packageReadRsp(u8 dataLen, u8 *pData, attr_pkt_t *txBuf)
{
    (void)pData;                  //unused, remove warning
    (void)dataLen;                //unused, remove warning
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_READ_RSP;


    return buffLen;
    ;
}

/**
 * Format of ATT_READ_BLOB_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Attribute Handle                   | 2                 |
 * | Value Offset                       | 2                 |
 */
int blt_att_packageReadBlobReq(u16 attHandle, u16 offset, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_READ_BLOB_REQ;
    buffer[buffLen++] = attHandle & 0xFF;
    buffer[buffLen++] = (attHandle & 0xFF00) >> 8;
    buffer[buffLen++] = offset & 0xFF;
    buffer[buffLen++] = (offset & 0xFF00) >> 8;


    return buffLen;
    ;
}

/**
 * Format of ATT_READ_BLOB_RSP PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Attribute Value                    | 0 to (ATT_MTU-1)  |
 */
int blt_att_packageReadBlobRsp(u8 dataLen, u8 *pData, attr_pkt_t *txBuf)
{
    (void)dataLen;                //unused, remove warning
    (void)pData;                  //unused, remove warning
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_READ_BLOB_RSP;


    return buffLen;
    ;
}

/**
 * Format of ATT_HANDLE_VALUE_NTF PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Attribute Handle                   | 2                 |
 * | Attribute Value                    | 0 to (ATT_MTU-3)  |
 */
int blc_att_prepareNotify(u16 attHandle, u16 dataLen, u8 *pData, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field //skip dataLen field

    buffer[buffLen++] = ATT_OP_HANDLE_VALUE_NTF;
    buffer[buffLen++] = attHandle & 0xFF;
    buffer[buffLen++] = (attHandle & 0xFF00) >> 8;
    memcpy(&buffer[buffLen], pData, dataLen);
    buffLen += dataLen;


    return buffLen;
    ;
}

/**
 * Format of ATT_HANDLE_VALUE_IND PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Attribute Handle                   | 2                 |
 * | Attribute Value                    | 0 to (ATT_MTU-3)  |
 */
int blc_att_prepareIndicate(u16 attHandle, u16 dataLen, u8 *pData, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_HANDLE_VALUE_IND;
    buffer[buffLen++] = attHandle & 0xFF;
    buffer[buffLen++] = (attHandle & 0xFF00) >> 8;
    memcpy(&buffer[buffLen], pData, dataLen);
    buffLen += dataLen;


    return buffLen;
}

/**
 * Format of ATT_HANDLE_VALUE_CFM PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 */
int blc_att_prepareConfirm(attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_HANDLE_VALUE_CFM;


    return buffLen;
}

/**
 * Format of ATT_WRITE_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Attribute Handle                   | 2                 |
 * | Attribute Value                    | 0 to (ATT_MTU-3)  |
 */
int blt_att_packageWriteReq(u16 attHandle, u16 dataLen, u8 *pData, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_WRITE_REQ;
    buffer[buffLen++] = attHandle & 0xFF;
    buffer[buffLen++] = (attHandle & 0xFF00) >> 8;
    memcpy(&buffer[buffLen], pData, dataLen);
    buffLen += dataLen;


    return buffLen;
    ;
}

/**
 * Format of ATT_WRITE_RSP
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 */
int blt_att_packageWriteRsp(attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_WRITE_RSP;


    return buffLen;
    ;
}

/**
 * Format of ATT_WRITE_CMD PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Attribute Handle                   | 2                 |
 * | Attribute Value                    | 0 to (ATT_MTU-3)  |
 */
int blt_att_packageWriteCmd(u16 attHandle, u16 dataLen, u8 *pData, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_WRITE_CMD;
    buffer[buffLen++] = attHandle & 0xFF;
    buffer[buffLen++] = (attHandle & 0xFF00) >> 8;
    memcpy(&buffer[buffLen], pData, dataLen);
    buffLen += dataLen;


    return buffLen;
    ;
}

/**
 * Format of ATT_PREPARE_WRITE_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Attribute Handle                   | 2                 |
 * | Value Offset                       | 2                 |
 * | Part Attribute Value               | 0 to (ATT_MTU-5)  |
 */
int blt_att_packagePrepareWriteReq(u16 attHandle, u16 offset, u16 dataLen, u8 *pData, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_PREPARE_WRITE_REQ;
    buffer[buffLen++] = attHandle & 0xFF;
    buffer[buffLen++] = (attHandle & 0xFF00) >> 8;
    buffer[buffLen++] = offset & 0xFF;
    buffer[buffLen++] = (offset & 0xFF00) >> 8;
    memcpy(&buffer[buffLen], pData, dataLen);
    buffLen += dataLen;


    return buffLen;
    ;
}

/**
 * Format of ATT_PREPARE_WRITE_RSP PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Attribute Handle                   | 2                 |
 * | Value Offset                       | 2                 |
 * | Part Attribute Value               | 0 to (ATT_MTU-5)  |
 */
int blt_att_packagePrepareWriteRsp(u16 attHandle, u16 offset, u16 dataLen, u8 *pData, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_PREPARE_WRITE_RSP;
    buffer[buffLen++] = attHandle & 0xFF;
    buffer[buffLen++] = (attHandle & 0xFF00) >> 8;
    buffer[buffLen++] = offset & 0xFF;
    buffer[buffLen++] = (offset & 0xFF00) >> 8;
    memcpy(&buffer[buffLen], pData, dataLen);
    buffLen += dataLen;


    return buffLen;
    ;
}

/**
 * Format of ATT_EXECUTE_WRITE_REQ PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Flags                              | 1                 |
 */
int blt_att_packageExecuteWriteReq(u8 flag, attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_EXECUTE_WRITE_REQ;
    buffer[buffLen++] = flag & 0xFF;


    return buffLen;
    ;
}

/**
 * Format of ATT_EXECUTE_WRITE_RSP PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 */
int blt_att_packageExecuteWriteRsp(attr_pkt_t *txBuf)
{
    u8  buffLen = 0;
    u8 *buffer  = &txBuf->opcode; //skip dataLen field

    buffer[buffLen++] = ATT_OP_EXECUTE_WRITE_RSP;


    return buffLen;
    ;
}
