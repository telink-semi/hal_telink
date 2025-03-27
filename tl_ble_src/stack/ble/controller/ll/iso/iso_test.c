/********************************************************************************************************
 * @file    iso_test.c
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
#include "stack/ble/controller/ble_controller.h"



#if(LL_FEATURE_ENABLE_ISOCHRONOUS_TEST_MODE)


#define ISO_TEST_CTRL_BLOCK_NUM     2

iso_test_param_t    gIsoTestPara[ISO_TEST_CTRL_BLOCK_NUM];  //EBQ test use 2 ISO test now, 20220312  //


iso_test_param_t* blt_iso_test_allocateCtrlBlock(void)
{
    iso_test_param_t *pCtrl = NULL;
    for(int i = 0; i<ISO_TEST_CTRL_BLOCK_NUM; i++){

        pCtrl = &gIsoTestPara[i];
        if(pCtrl->occupy ==0){

            pCtrl->occupy = 1;
            return &gIsoTestPara[i];
        }
    }
    return pCtrl;
}


int blt_iso_test_transmit_mainloop(iso_test_param_t *isoTest, u32 interval,u16 max_sdu, sdu_packet_t *sdu, u8 frame)
{
    int ret = 1;

    if(isoTest->tranMode.isoTestSendTick && clock_time_exceed(isoTest->tranMode.isoTestSendTick, interval))
    {

        u32 payloadLen = 0;
        isoTest->tranMode.isoTestSendTick = clock_time()|1;

        if(isoTest->isoTest_payload_type == ITEST_ZERO_LENGTH)//zero size SDU
        {
            payloadLen = 0;
        }
        else if(isoTest->isoTest_payload_type == ITEST_VARIABLE_LENGTH)//variable size SDU
        {
            generateRandomNum(4, (u8*)&payloadLen);
            payloadLen = rand();
            payloadLen = max(payloadLen%max_sdu, 4);
        }
        else if(isoTest->isoTest_payload_type == ITEST_MAX_LENGTH)
        {
            payloadLen = max_sdu;
        }

        sdu->iso_sdu_len = payloadLen;

//      generateRandomNum(payloadLen, (u8*)sdu->data);
        tlkapi_send_string_data(DBG_ISO_TEST_EN,"iso transmit loop", &payloadLen, 4);

        if(frame == CIS_FRAMED)
        {//framed packet

            sdu->data[0] =    isoTest->tranMode.send_pkt_cnt&0xff;
            sdu->data[1] = (isoTest->tranMode.send_pkt_cnt>>8)&0xff;
            sdu->data[2] = (isoTest->tranMode.send_pkt_cnt>>16)&0xff;
            sdu->data[3] = (isoTest->tranMode.send_pkt_cnt>>24)&0xff;
        }
        else
        {//unframed packet
            if(payloadLen>=8)// just debug
            {
                sdu->data[4] =  isoTest->tranMode.send_pkt_cnt&0xff;
                sdu->data[5] = (isoTest->tranMode.send_pkt_cnt>>8)&0xff;
                sdu->data[6] = (isoTest->tranMode.send_pkt_cnt>>16)&0xff;
                sdu->data[7] = (isoTest->tranMode.send_pkt_cnt>>24)&0xff;
            }
        }

        isoTest->tranMode.send_pkt_cnt ++;

        ret = 0;
    }
    return ret;
}


int blt_iso_test_receive_mainloop(sdu_packet_t *sdu, iso_test_param_t *pBisTestParam, u16 max_sdu, u8 frame)
{

    iso_test_receive_infor_t *isoRecInfo = &pBisTestParam->recMode;


//  tlkapi_send_string_u32s(DBG_ISO_TEST_EN,"isoTest00",0, sdu->pkt_st,frame,0);

    if(sdu->pkt_st!=0){

        isoRecInfo->missedCnt++;
        isoRecInfo->expectCnt++;//just for frame packet
        tlkapi_send_string_u32s(DBG_ISO_TEST_EN,"ISOTest missedCnt",isoRecInfo->missedCnt,isoRecInfo->expectCnt, sdu->pkt_st,0);
    }
    else
    {

        if(pBisTestParam->isoTest_payload_type==ITEST_ZERO_LENGTH)
        {
            if(sdu->iso_sdu_len == 0){
                isoRecInfo->successCnt++;
                tlkapi_send_string_u32s(DBG_ISO_TEST_EN,"ISOTest Success",isoRecInfo->successCnt,isoRecInfo->failedCnt,isoRecInfo->missedCnt, 0);
            }
            else{
                isoRecInfo->failedCnt++;
                tlkapi_send_string_u32s(DBG_ISO_TEST_EN,"ISOTest Fail",isoRecInfo->successCnt,isoRecInfo->failedCnt,isoRecInfo->missedCnt,0);
            }

        }
        else{//ITEST_VARIABLE_LENGTH

            u32 pkt_cnt = sdu->data[0] | (sdu->data[1]<<8) | (sdu->data[1]<<16) | (sdu->data[1]<<24);

            if((sdu->iso_sdu_len<4) || (sdu->iso_sdu_len>max_sdu)){ // length error
                isoRecInfo->failedCnt++;
                tlkapi_send_string_u32s(DBG_ISO_TEST_EN,"ISOTest SduLen Error", isoRecInfo->successCnt,isoRecInfo->failedCnt,isoRecInfo->missedCnt, sdu->iso_sdu_len);

                return 0;
            }

            u32 expectPktCnt;
            if(frame)
            {
                if(!isoRecInfo->expectCnt){//first sdu
                    isoRecInfo->expectCnt = pkt_cnt;
                    tlkapi_send_string_u32s(DBG_ISO_TEST_EN,"ISOTest First SDU", isoRecInfo->successCnt,isoRecInfo->failedCnt,isoRecInfo->missedCnt, isoRecInfo->expectCnt);
                }

                expectPktCnt = isoRecInfo->expectCnt;
                isoRecInfo->expectCnt++;
            }
            else{
                expectPktCnt = sdu->pkt_seq_num; // payloadNum of the first PDU in the SDU
            }


            if(pkt_cnt != expectPktCnt){ //packet counter error

                isoRecInfo->failedCnt++;
                isoRecInfo->expectCnt = (++pkt_cnt);
                tlkapi_send_string_u32s(DBG_ISO_TEST_EN,"ISOTest PktCnt Error",pkt_cnt, isoRecInfo->expectCnt,isoRecInfo->failedCnt,isoRecInfo->missedCnt);
            }
            else{
                isoRecInfo->successCnt++;
                tlkapi_send_string_u32s(DBG_ISO_TEST_EN,"ISOTest PktCnt Success",pkt_cnt, isoRecInfo->successCnt,isoRecInfo->failedCnt,isoRecInfo->missedCnt);
            }
        }



    }


    return 1;
}





ble_sts_t blc_hci_le_iso_read_test_count_cmd(hci_le_isoReadTestCountsCmdParams_t *pcmd, hci_le_isoRxTestStatusParam_t *pRetParam)
{
    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] ISO_Read_Test_Cnt", &pRetParam->conn_handle, 2);

    pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
    pRetParam->conn_handle = pcmd->conn_handle;
    pRetParam->failed_packet_count = 0;
    pRetParam->miss_packet_count = 0;
    pRetParam->received_packet_count = 0;

    if(0)
    {
    }
#if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)
    else if(pcmd->conn_handle & BLT_BIS_HANDLE){
         return ll_bis_cmd_task_cb(HCI_CMD_LE_ISO_READ_TEST_COUNTERS, pcmd, pRetParam);//blt_bis_cmd_process_task   blc_hci_bisSync_iso_read_test_count_cmd
    }
#endif

#if(LL_FEATURE_ENABLE_CONNECTED_ISO)
    else if(pcmd->conn_handle & BLT_CIS_HANDLE){
        return ll_cis_cmd_task_cb(HCI_CMD_LE_ISO_READ_TEST_COUNTERS, pcmd, pRetParam);//blt_cis_cmd_process_task    blc_hci_cis_read_test_count_cmd
    }
#endif

    return pRetParam->status;
}


ble_sts_t blc_hci_le_iso_test_end_cmd(hci_le_isoTestEndCmdParams_t* pCmd, hci_le_isoTestEndStatusParam_t *pRetParam)
{
    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] ISO_Test_End", &pRetParam->conn_handle, 2);

    pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
    pRetParam->conn_handle = pCmd->conn_handle;

    pRetParam->failed_packet_count = 0;
    pRetParam->miss_packet_count = 0;
    pRetParam->received_packet_count = 0;


    if(0){

    }
#if(LL_FEATURE_ENABLE_CONNECTIONLESS_ISO)
    else if(pCmd->conn_handle & BLT_BIS_HANDLE){
        return ll_bis_cmd_task_cb(HCI_CMD_LE_ISO_TEST_END, pCmd, pRetParam);//blt_bis_cmd_process_task   blc_hci_bis_iso_test_end_cmd
    }
#endif

#if(LL_FEATURE_ENABLE_CONNECTED_ISO)
    else if(pCmd->conn_handle & BLT_CIS_HANDLE){
        return ll_cis_cmd_task_cb(HCI_CMD_LE_ISO_TEST_END, pCmd, pRetParam);//blt_cis_cmd_process_task   blc_ll_cis_iso_test_end_cmd
    }
#endif

    return pRetParam->status;
}




ble_sts_t blc_hci_le_iso_transmit_test(hci_le_isoTestCmdParams_t *pCmdParam, hci_le_isoTestRetParams_t *pRetParam)
{
    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] ISO_Transmit_Test", pCmdParam, sizeof(hci_le_isoTestCmdParams_t));


    pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
    pRetParam->conn_handle = pCmdParam->conn_handle;

    if(0){

    }
#if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
    else if(pCmdParam->conn_handle & BLT_BIS_HANDLE){
        return ll_bis_cmd_task_cb(HCI_CMD_LE_ISO_TRANSMIT_TEST, pCmdParam, pRetParam); // blt_bis_cmd_process_task  blc_hci_bisBcst_iso_transmit_test_cmd
    }
#endif

#if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    else if(pCmdParam->conn_handle & BLT_CIS_HANDLE){
            return ll_cis_cmd_task_cb(HCI_CMD_LE_ISO_TRANSMIT_TEST, pCmdParam, pRetParam); // blt_cis_cmd_process_task  blc_hci_cis_iso_transmit_test_cmd
    }
#endif


    return pRetParam->status;
}


ble_sts_t blc_hci_le_iso_receive_test(hci_le_isoTestCmdParams_t *pCmdParam, hci_le_isoTestRetParams_t *pRetParam)
{

    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] ISO_Receive_Test", pCmdParam, sizeof(hci_le_isoTestCmdParams_t));


    pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
    pRetParam->conn_handle = pCmdParam->conn_handle;
    if(0){

    }
#if(LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)
    if(pCmdParam->conn_handle & BLT_BIS_HANDLE){
         return ll_bis_cmd_task_cb(HCI_CMD_LE_ISO_RECEIVE_TEST, pCmdParam, pRetParam);//blt_bis_cmd_process_task   blc_hci_bisSync_iso_receive_test
    }
#endif

#if(LL_FEATURE_ENABLE_CONNECTED_ISO)
    else if(pCmdParam->conn_handle & BLT_CIS_HANDLE){
        return ll_cis_cmd_task_cb(HCI_CMD_LE_ISO_RECEIVE_TEST, pCmdParam, pRetParam);//blt_cis_cmd_process_task   blc_hci_cis_iso_receive_test
    }
#endif



    return pRetParam->status;
}

#endif

