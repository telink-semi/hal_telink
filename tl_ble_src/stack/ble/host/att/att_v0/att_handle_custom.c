/********************************************************************************************************
 * @file    att_handle_custom.c
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





_attribute_ble_data_retention_  attHl_convert_t* gAttCusTbl = NULL;




ble_sts_t blc_att_setAttributeHandleCustomTable (attHl_convert_t *pAttCus, int attHlConvert_num)
{
    gAttCusTbl = pAttCus;
    bltAtt.attHl_cusNum = attHlConvert_num;
    bltAtt.attHl_custom_en = 1;

    return BLE_SUCCESS;
}




#if 0 //demo code

attHl_convert_t attHandCustom_tbl[4] = {
    {36, 0x3000},
    {37, 0x3001},
    {38, 0x3002},
    {39, 0x3003},
};


blc_att_setAttributeHandleCustomTable(attHandCustom_tbl, 4);

#endif






u16 blt_att_change_sdkAttHandle_to_customAttHandle(u16 sdk_attHl)
{
    u16 custom_attHl = sdk_attHl;

    for(int i=0; i<bltAtt.attHl_cusNum; i++){
        if(sdk_attHl == gAttCusTbl[i].attHl_sdk){
            custom_attHl = gAttCusTbl[i].attHl_cus;
            break;
        }
    }

    return custom_attHl;
}


u16 blt_att_change_customAttHandle_to_sdkAttHandle(u16 custom_attHl)
{
    u16 sdk_attHl = custom_attHl;

    //>=: sdk att handle do not change.
    if(custom_attHl >= gAttCusTbl[0].attHl_cus){
        for(int i=0; i<bltAtt.attHl_cusNum; i++){
            //<=: start handle in Read by Group Req may be +1 value, not in custom handle table.
            if(custom_attHl <= gAttCusTbl[i].attHl_cus){
                sdk_attHl = gAttCusTbl[i].attHl_sdk;
                break;
            }
        }
    }
    //eg. find range: 0x000F~0x3002, when gAttCusTbl[0] = {0x0008, 0x1000}, 0x000F should be change to 0x0008.
    else if(custom_attHl > gAttCusTbl[0].attHl_sdk){
        sdk_attHl = gAttCusTbl[0].attHl_sdk;
    }


    return sdk_attHl;
}

//                                              att handle
// ATT_EXCHANGE_MTU_REQ                0x02     no
// ATT_OP_FIND_INFO_REQ                0x04     [7]s[9]e
// ATT_OP_FIND_BY_TYPE_VALUE_REQ       0x06     [7]s[9]e
// ATT_OP_READ_BY_TYPE_REQ             0x08     [7]s[9]e
// ATT_OP_READ_REQ                     0x0a     [7]h
// ATT_OP_READ_BLOB_REQ                0x0c     [7]h
// ATT_OP_READ_MULTI_REQ               0x0e     set
// ATT_OP_READ_BY_GROUP_TYPE_REQ       0x10     [7]s[9]e
// ATT_OP_WRITE_REQ                    0x12     [7]h
// ATT_OP_PREPARE_WRITE_REQ            0x16     [7]h
// ATT_OP_EXECUTE_WRITE_REQ            0x18     no
// ATT_OP_HANDLE_VALUE_CFM             0x1e     no
// ATT_OP_WRITE_CMD                    0x52     [7]h
// ATT_OP_SIGNED_WRITE_CMD             0xd2     [7]h

typedef struct __attribute__((packed)) {
    u8  type;
    u8  rf_len;

    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u16 handle1;
    u16 handle2;
} rf_packet_att_get_handle_t;

void blt_att_processAttHandle_in_attCmd(rf_packet_l2cap_req_t * pL2capReq)
{
    rf_packet_att_get_handle_t * pkt = (rf_packet_att_get_handle_t *)pL2capReq;
    switch(pL2capReq->opcode)
    {
        //startingHandle+endingHandle
        case ATT_OP_FIND_INFO_REQ:
        case ATT_OP_FIND_BY_TYPE_VALUE_REQ:
        case ATT_OP_READ_BY_TYPE_REQ:
        case ATT_OP_READ_BY_GROUP_TYPE_REQ:
        {
            pkt->handle1 = blt_att_change_customAttHandle_to_sdkAttHandle(pkt->handle1);
            pkt->handle2 = blt_att_change_customAttHandle_to_sdkAttHandle(pkt->handle2);
        }
        break;

        //one handle
        case ATT_OP_READ_REQ:
        case ATT_OP_READ_BLOB_REQ:
        case ATT_OP_WRITE_REQ:
        case ATT_OP_PREPARE_WRITE_REQ:
        case ATT_OP_WRITE_CMD:
        case ATT_OP_SIGNED_WRITE_CMD:
        {
            pkt->handle1 = blt_att_change_customAttHandle_to_sdkAttHandle(pkt->handle1);
        }
        break;

        default:
            break;
    }

}
