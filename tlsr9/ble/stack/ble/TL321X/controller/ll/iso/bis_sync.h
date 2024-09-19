/********************************************************************************************************
 * @file    bis_sync.h
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
#ifndef BIS_SYNC_H_
#define BIS_SYNC_H_


#define     BIG_SYNC_PARAM_LENGTH                           928  //user can't modify this value !!!



/**
 * @brief      for user to initialize BIG Synchronize module and allocate BIG Synchronize parameters buffer.
 * @param[in]  pBigSyncPara - start address of BIG Synchronize parameters buffer
 * @param[in]  bigSyncNum - BIG Synchronize number application layer may use
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_initBigSyncModule_initBigSyncParametersBuffer(u8 *pBigSyncPara, u8 bigSyncNum);


/*
 * @brief      This function is used to initialize Synchronize sdu out fifo buffer.
 *             sdu out indicate the data from controller to host.
 * @param[in]  out_fifo      - the start address of Synchronize sdu out fifo buffer.
 * @param[in]  out_fifo_size - the fifo size.
 * @param[in]  out_fifo_num  - the fifo number.
 * @return     status, 0x00:  succeed
 *             other: failed
 */
ble_sts_t blc_ll_initBisSyncSduOutBuffer(u8 *out_fifo, u16 out_fifo_size, u8 out_fifo_num);

/**
 * @brief      for user to initialize BIS ISO RX FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer start address.
 * @param[in]  fifo_size - RX FIFO size
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32, must be 2^n
 * @param[in]  fifo_number - bis num
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initBisRxFifo(u8 *pRxbuf, int full_size, int fifo_number, u8 bis_sync_num);


/**
 * @brief      be used to stop synchronizing or cancel the process of synchronizing to the BIG identified by the bigHandle.
 *             also terminate the reception of BISes in the BIG specified in the blc_hci_le_bigCreateSync,
 *             destroys the associated connection handles of the BISes in the BIG and removes the data paths for all BISes in the BIG.
 * @param[in]  bigHandle - Identifier of the BIG to terminate.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_bigTerminateSync(u8 bigHandle);


/**
 * @brief      For user to pop rx SDU FIFO that hold Rx data.
 * @param[in]  bis_connHandle - Identifier of the bis handle.
 * @return     sdu_packet_t, PDU packet to SDU data structure
 */
sdu_packet_t* blc_ll_popBisSyncRxSduData(u16 bis_connHandle);


#endif /* BIS_SYNC_H_ */
