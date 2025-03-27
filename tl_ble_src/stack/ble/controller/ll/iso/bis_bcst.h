/********************************************************************************************************
 * @file    bis_bcst.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#ifndef BIS_BCST_H_
#define BIS_BCST_H_



#define     BIG_BCST_PARAM_LENGTH                           844  //user can't modify this value !!!



/**
 * @brief      for user to initialize BIG broadcast module and allocate BIG broadcast parameters buffer.
 * @param[in]  pBigBcstPara - start address of BIG broadcast parameters buffer
 * @param[in]  bigBcstNum - BIG broadcast number application layer may use
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_initBigBcstModule_initBigBcstParametersBuffer(u8 *pBigBcstPara, u8 bigBcstNum);


/**
 * @brief      for user to initialize BIS ISO TX FIFO.
 * @param[in]  pRxbuf - TX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size, must use BIS_PDU_ALIGN4_TXBUFF to calculate.
 * @param[in]  fifo_number - RX FIFO number, must be: 2^n, (power of 2),recommended value: 2, 4, 8, 16, 32, 64
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_initBisTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number);

/*
 * @brief      This function is used to initialize broadcast sdu in fifo buffer.
 *             sdu in indicate the data if from host to controller.
 * @param[in]  in_fifo      - the start address of broadcast sdu in fifo buffer.
 * @param[in]  in_fifo_size - the fifo size.
 * @param[in]  in_fifo_num  - the fifo number.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initBisBcstSduInBuffer(u8 *in_fifo,u16 in_fifo_size, u8 in_fifo_num);


/**
 * @brief      This function is used to get SDU buffer number valid.
 * @param[in]  bisHandle.
 * @return      Status - 0x00:  no buffer number valid or bisHandle invalid.
 *                       other:  valid buffer number.
 */
int blc_ll_getBisSduInBufferFreeNum(u16 bisHandle);





#endif /* BIS_BCST_H_ */
