/********************************************************************************************************
 * @file    hci_stack.h
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
#ifndef STACK_BLE_HCI_HCI_STACK_H_
#define STACK_BLE_HCI_HCI_STACK_H_

#include "hci.h"


typedef struct{
    u16  connHandle      :12;
    u16  PB_Flag         :2;
    u16  BC_Flag         :2;

    u16  data_total_len;
    u8   data[1];
}hci_acl_data_pkt_t;



typedef struct{
    u8  hciCmplEvtEn;     /* Reply with HCI Command Complete Event for unsupported HCI CMD */

    /* Host to Controller flow control */
    u8  flowCtrlEnable;     //00 01 02 03
    u8  align;              //RFU
    u16 curCmplPktNum;      //currently the number of host available buffer. Host_Total_Num_ACL_Data_Packets

    u16 aclDataPktTotalNum; //the total number of host available ACL buffer.
    u16 aclDataPktLen;      //the length of host available ACL buffer.

    u8 scoDataPktTotalNum;  //the total number of host available SCO buffer.
    u8 scoDataPktLen;       //the length of host available SCO buffer.


    u8 isoDataInFifo_num;
    u16 isoDataInFifo_size;


}hciMng_t;

extern hciMng_t bltHciMng;


extern  hci_fifo_t  bltHci_rxAclfifo;



extern hci_event_handler_t      blt_gap_hci_event_handler;


typedef int (*hci_handler_t) (void);



int blc_hci_cmd_handler (u8 *p);

/**
 * @brief      This function is used to pack HCI ISO data packet to SDU packet.
 * @param[in]  pIsoData - point to hci ISO Data packet buff.
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_hci_packIsoData(iso_data_packet_t *);



/**
 * @brief      This function is used to register the controller and host event callback for audio.
 * @param[in]  handler - hci_event_handler_t
 * @return     none.
 */
void        blt_gap_registerEventHandler (hci_event_handler_t handler);


/******************************* hci_event start ******************************************************************/



int     hci_le_periodcAdvSubeventDataRequest_evt(u8 adv_handle, u8 subevt_start, u8 subevt_data_cnt);



/******************************* hci_event end ******************************************************************/






void hci_initHciMng(void);   //Reset

void hci_resetCurHostAvailBufNum(void);

ble_sts_t hci_setControllerToHostFlowCtrl(u8 ctrl);

u8 hci_getControllerToHostFlowCtrl(void);

u16 hci_getHostAvailBufNum(void);

void hci_reduceOneHostAvailBuf(void);

ble_sts_t hci_hostBufferSize(hci_hostBufferSize_cmdParam_t * cmdPara);

ble_sts_t hci_hostNumCompletedPackets(hci_hostNumOfCompletedPkt_cmdParam_t* CompPackCom);



#endif /* STACK_BLE_HCI_HCI_STACK_H_ */
