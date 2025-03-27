/********************************************************************************************************
 * @file    bis.c
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




#if (LL_FEATURE_ENABLE_CONNECTIONLESS_ISO)



ll_bis_t        *global_pBis = NULL;
ll_bis_t        *blt_pBis    = NULL;

ll_bis_mng_t    bltBisMng;
int             blt_bis_sel = 0;


#if 0  //see sizeof(ll_bis_t) in warning information
    char checker(int);
    char checkSizeOfInt[sizeof(ll_bis_t)]={checker(&checkSizeOfInt)};
#endif


/*
 * @brief      for user to allocate bis parameters buffer. both broadcast and Synchronize use the API.
 * @param[in]  pBisPara - start address of BIS parameters buffer.
 * @param[in]  bis_bcst_num - the bis broadcast number application layer may use.
 * @param[in]  bis_sync_num - the bis Synchronized number application layer may use.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_InitBisParametersBuffer(u8 *pBisPara, u8 bis_bcst_num, u8 bis_sync_num)
{
    STATIC_ASSERT_FILE(BIS_PARAM_LENGTH == sizeof(ll_bis_t), bis);

    if((bis_bcst_num > (LL_BIG_BCST_NUM_MAX*LL_BIS_IN_PER_BIG_BCST_NUM_MAX))  ||  (bis_sync_num > LL_BIG_SYNC_NUM_MAX*LL_BIS_IN_PER_BIG_SYNC_NUM_MAX)){
        return LL_ERR_INVALID_PARAMETER;
    }

    global_pBis = (ll_bis_t*)pBisPara;
    bltBisMng.maxNum_bisTotal = (bis_bcst_num + bis_sync_num);
    bltBisMng.maxNum_bisBcst = bis_bcst_num;
    bltBisMng.maxNum_bisSync = bis_sync_num;

    bltBisMng.curNum_bisBcst = bltBisMng.curNum_bisSync = 0;

    for(int i=0; i< bltBisMng.maxNum_bisTotal; i++)
    {
        if(i<bltBisMng.maxNum_bisBcst){
            (global_pBis + i)->bis_role = BIS_ROLE_BCST;
        }
        else{
            (global_pBis + i)->bis_role = BIS_ROLE_SYNC;
        }

        (global_pBis + i)->bis_occupied = 0;
        (global_pBis + i)->bis_handle = i | BLT_BIS_HANDLE;
    }

    ll_bis_cmd_task_cb = blt_bis_cmd_process_task;

    blmsParam.bis_en = 1;

    return BLE_SUCCESS;
}

ll_bis_t *blt_ll_findBisByHandle(u16 bisHandle)
{

    if((bisHandle >=BLT_BIS_HANDLE) && (BLT_BIS_HANDLE< (BLT_BIS_HANDLE +bltBisMng.maxNum_bisTotal))){
        u8 bis_idx = bisHandle & BLT_BIS_IDX_MSK;

        ll_bis_t *pBis = global_pBis + bis_idx;
        if(pBis->bis_occupied){
            return pBis;
        }
    }
    return NULL;
}

u32 blc_ll_getAvailBisNum(u8 role)
{
    if(role == BIS_ROLE_BCST){//broadcast
        return (bltBisMng.maxNum_bisBcst - bltBisMng.curNum_bisBcst);
    }
    else{
        return (bltBisMng.maxNum_bisSync - bltBisMng.curNum_bisSync);
    }
}

u32 blt_ll_bis_getSeedAccessAddr(void)
{
      u32 accessAddr;
//    generateRandomNum(4, (u8*)&accessAddr);
      accessAddr = blt_ll_connCalcAccessAddr_v2();//TODO: It seems better, recheck later

      /*
       * The following code enforces a pattern to make sure the address meets all requirements.
       * The pattern is either the first one or the second one
       *
       *   0bxxxxxx0x 1yyyyyyy yxxxxxxx xxxxxxxx
       *
       * with 2^5 choices for the middle 8 bits(indicated by y).  This provides 2^5 * 2^ (21 - 4(z)) = 2 ^ 22 = 4194304 variations.
       * Z depends LL_MAX_BIS, 6 BISs require 4 bits or 16 combination to assure at least two bits are different.
       */

      /* Patterns for middle 8 bits with 32 combinations. The upper one follows the 1yy0yy10 and lower one follows the 0yy1yy01 */
      static const u8 upperSixBits[] =
      {
        /* 10000010 10000110  10001010  10001110  10100010  10100110  10101010  10101110  11000010  11000110  11001010  11001110  11100010  11100110  11101010  11101110 */
           0x82,    0x86,     0x8A,     0x8E,     0xA2,     0xA6,     0xAA,     0xAE,     0xC2,     0xC6,     0xCA,     0xCE,     0xE2,     0xE6,     0xEA,     0xEE,
        /* 00010001 00010101  00011001  00011101  00110001  00110101  00111001  00111101  01010001  01010101  01011001  01011101  01110001  01110101  01111001  01111101 */
           0x11,    0x15,     0x19,     0x1D,     0x31,     0x35,     0x39,     0x3D,     0x51,     0x55,     0x59,     0x5D,     0x71,     0x75,     0x79,     0x7D,
      };

      /* Set the middle 8 bits. */
      accessAddr  = (accessAddr & ~0x007F8000) | (upperSixBits[accessAddr >> 27] << 15);

      /* Set ones with the mask 0b00000000 10000000 00000000 00000000 */
      accessAddr |=  0x00800000;

      /* Clear zeros with the mask 0b00000010 00000000 00000000 00000000 */
      accessAddr &= ~0x02000000;

      /* TODO add code to make sure two BIG seedAccessAddress in the same device shall differ in at least two bits. */

      return accessAddr;
}

u32 blt_ll_bis_getAccessCode(u32 seedAccessCode, u8 bisSeq)
{
      u32 accAddr = seedAccessCode;
      u16 div;
      u32 dw = 0;
      u8 d0, d1, d2, d3, d4, d5, d6;

      d0 = d1 = d2 = d3 = d4 = d5 = d6 = 0;

      /* Diversifier = ((35 * n) + 42) MOD 128 */
      u16 term = (35 * bisSeq) + 42;
      div = term - ((term >> 7) * 128);

      d0 = div & 1;
      d1 = (div >> 1) & 1;
      d2 = (div >> 2) & 1;
      d3 = (div >> 3) & 1;
      d4 = (div >> 4) & 1;
      d5 = (div >> 5) & 1;
      d6 = (div >> 6) & 1;

      /* DW = D0D0D0D0D0D0D1D6_D10D5D40D3D20_00000000_00000000b  Note the digit after the letter is the index of the D. */
      dw = ((d0 << 31) | (d0 << 30) | (d0 << 29) | (d0 << 28) | (d0 << 27)  | (d0 << 26) | (d1 << 25) | (d6 << 24) | \
            (d1 << 23) | (d5 << 21) | (d4 << 20) | (d3 << 18) | (d2 << 17));

      /* SAA bit-wise XORed with DW */
      accAddr ^= dw;

      return accAddr;
}


ble_sts_t   blc_ll_removeBisDataPath(u16 handle, u8 dp_dir_mask)
{

    ll_bis_t *pBis = blt_ll_findBisByHandle(handle);

    if(pBis==NULL){//|| (pCis->conState !=CONN_STATUS_ESTABLISH)
        return HCI_ERR_UNKNOWN_CONN_ID;
    }else if(!(pBis->bis_dapth_setup & dp_dir_mask)){
        return HCI_ERR_CMD_DISALLOWED;
    }

    pBis->bis_dapth_setup &= ~dp_dir_mask;
    pBis->dpID = 0xff;


    return BLE_SUCCESS;
}

ble_sts_t   blc_hci_le_removeBisDataPath(hci_le_rmvIsoDataPath_cmdParam_t *pCmdPara, hci_le_rmvIsoDataPath_retParam_t *pRetParam)
{

    pRetParam->conn_handle = pCmdPara->conn_handle;
    pRetParam->status = blc_ll_removeBisDataPath(pCmdPara->conn_handle, pCmdPara->dp_dir_mask);

    return pRetParam->status;
}

ble_sts_t   blc_ll_setupBisDataPath(u16 handle, dat_path_dir_t dir, dat_path_id_t id, u8 cid_assignNum, u16 cidcompId, u16 cid_vendorDef,
                                     u32 control_dly, u8 codec_cfg_len,      u8 codec_cfg1,    u8 codec_cfg2,     u8 codec_cfg3, u8 codec_cfg4 )
{
    (void)cid_assignNum; //unused, remove warning
    (void)cidcompId; //unused, remove warning
    (void)cid_vendorDef; //unused, remove warning
    (void)control_dly; //unused, remove warning
    (void)codec_cfg1; //unused, remove warning
    (void)codec_cfg2; //unused, remove warning
    (void)codec_cfg3; //unused, remove warning
    (void)codec_cfg4; //unused, remove warning

    if(codec_cfg_len > 4){
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }

    ll_bis_t *pBis = blt_ll_findBisByHandle(handle);



    if(pBis==NULL){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    /* para */
    if(dir == Data_Dir_Input){ /* input data path */

        if(pBis->bis_role == BIS_ROLE_SYNC){
            return HCI_ERR_CMD_DISALLOWED;
        }
        if(pBis->bis_dapth_setup & DATA_PATH_INPUT_FLAG){
            return HCI_ERR_CMD_DISALLOWED;
        }
        pBis->bis_dapth_setup |= DATA_PATH_INPUT_FLAG;
    }
    else if(dir == Data_Dir_Output){ /* output data path */

        if(pBis->bis_role == BIS_ROLE_BCST){
            return HCI_ERR_CMD_DISALLOWED;
        }
        if(pBis->bis_dapth_setup & DATA_PATH_OUTPUT_FLAG){
            return HCI_ERR_CMD_DISALLOWED;
        }
        pBis->bis_dapth_setup |= DATA_PATH_OUTPUT_FLAG;
    }
    pBis->dpID = id;


    return BLE_SUCCESS;
}


ble_sts_t   blc_hci_le_setupBisDataPath(hci_le_setupIsoDataPath_cmdParam_t *pCmdPara, hci_le_setupIsoDataPath_retParam_t *pRetParam)
{

    u32 controller_delay = pCmdPara->control_delay[0] | pCmdPara->control_delay[1]<<8 | pCmdPara->control_delay[2]<<16;
    pRetParam->status = blc_ll_setupBisDataPath(pCmdPara->conn_handle, pCmdPara->data_path_dir, pCmdPara->data_path_id,
                                                pCmdPara->codec_id_assignNum,pCmdPara->codec_id_compId, pCmdPara->codec_id_vendorDef,
                                                controller_delay, pCmdPara->codec_config_len, pCmdPara->codec_config[0],
                                                pCmdPara->codec_config[1],pCmdPara->codec_config[2],pCmdPara->codec_config[3]);

    pRetParam->conn_handle = pCmdPara->conn_handle;



    return pRetParam->status;
}



ble_sts_t blc_ll_bis_iso_test_end_cmd(u16 connHandle, hci_le_isoTestEndStatusParam_t *pRetParam)
{

    iso_test_param_t *pIsoTest=NULL;

    pRetParam->status = BLE_SUCCESS;
    pRetParam->conn_handle = connHandle;
    pRetParam->failed_packet_count = 0;
    pRetParam->miss_packet_count = 0;
    pRetParam->received_packet_count = 0;


    ll_bis_t *pBis = blt_ll_findBisByHandle(connHandle);
    /*
     * If the Host issues this command with a connection handle that does not exist,
     * or the Connection_Handle command parameter is not associated with a CIS or
     * a BIS, the Controller shall return the error code Unknown Connection Identifier
     * (0x02)
     */
    if(pBis==NULL)
    {
        pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
    }
    else
    {
        if(pBis->bis_role==BIS_ROLE_SYNC)
        {
            ll_big_sync_t *pBig =(ll_big_sync_t *)(global_pBigSync + pBis->big_idx);
            if(pBig->big_state != BIG_SYNCHRONIZED)
            {
                pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
            }

            pBis->bisSduOut_rptr = pBis->bisSduOut_wptr;
            pBis->bisPduTxFifoRptr = pBis->bisPduTxFifoWptr;
        }
        else
        {
            ll_big_bcst_t *pBig = (ll_big_bcst_t*)(global_pBigBcst  + pBis->big_idx);
            if(pBig->cmd_status != BIG_CREATE_COMPLETE)
            {
                pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
            }
        }

        if(pRetParam->status==BLE_SUCCESS)
        {
            pIsoTest = pBis->pBisTestParam;
            if((pIsoTest == NULL) || (pIsoTest->isoTestMode == ISO_TEST_DISABLE))
            {
                pRetParam->status = HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
            }
        }

        pBis->bisSduIn_rptr = pBis->bisSduIn_wptr;
        pBis->bisPduTxFifoWptr = pBis->bisPduTxFifoRptr;
        blt_ll_ResetBisRxFifo(pBis->bis_handle);
    }

    if((pRetParam->status==BLE_SUCCESS) && (pIsoTest->isoTestMode==ISO_RECEIVE_MODE))
    {
        pRetParam->failed_packet_count = pIsoTest->recMode.failedCnt;
        pRetParam->miss_packet_count = pIsoTest->recMode.missedCnt;
        pRetParam->received_packet_count = pIsoTest->recMode.successCnt;
    }

    if((pRetParam->status==BLE_SUCCESS) && (pIsoTest!=NULL))
    {
        pIsoTest->isoTestMode = ISO_TEST_DISABLE;
    }

    return pRetParam->status;
}

ble_sts_t blc_hci_bis_iso_test_end_cmd(hci_le_isoTestEndCmdParams_t *pCmd, hci_le_isoTestEndStatusParam_t *pRet)
{
    return blc_ll_bis_iso_test_end_cmd(pCmd->conn_handle, pRet);
}

_attribute_noinline_
int blt_bis_cmd_process_task (int opcode, void *pCmd, void *pRet)
{
    if(opcode == HCI_CMD_LE_READ_ISO_TX_SYNC){

    }
    else if(opcode == HCI_CMD_LE_SETUP_ISO_DATA_PATH){
        return blc_hci_le_setupBisDataPath((hci_le_setupIsoDataPath_cmdParam_t *)pCmd, ( hci_le_setupIsoDataPath_retParam_t *)pRet);
    }
    else if(opcode == HCI_CMD_LE_REMOVE_ISO_DATA_PATH){
        return blc_hci_le_removeBisDataPath((hci_le_rmvIsoDataPath_cmdParam_t *)pCmd, ( hci_le_rmvIsoDataPath_retParam_t *)pRet);
    }
    else if(opcode == HCI_CMD_LE_ISO_TRANSMIT_TEST){
        return blc_hci_bisBcst_iso_transmit_test_cmd((hci_le_isoTestCmdParams_t *)pCmd, (hci_le_isoTestRetParams_t *)pRet);
    }
    else if(opcode==HCI_CMD_LE_ISO_RECEIVE_TEST){
        return blc_hci_bisSync_iso_receive_test((hci_le_isoTestCmdParams_t *)pCmd, ( hci_le_isoTestRetParams_t *)pRet);
    }
    else if(opcode==HCI_CMD_LE_ISO_READ_TEST_COUNTERS){
        return blc_hci_bisSync_iso_read_test_count_cmd((hci_le_isoReadTestCountsCmdParams_t *)pCmd, ( hci_le_isoRxTestStatusParam_t *)pRet);
    }
    else if(opcode==HCI_CMD_LE_ISO_TEST_END){
        return blc_hci_bis_iso_test_end_cmd((hci_le_isoTestEndCmdParams_t *)pCmd, ( hci_le_isoTestEndStatusParam_t *)pRet);
    }
#if(FANQH_OPTIMIZE_BIS_API)
    else if(opcode==HCI_CMD_LE_ISO_DATA){
        return blc_hci_le_pushBisData((iso_data_packet_t *)pCmd);
    }
#endif
    return 0;
}

#endif

