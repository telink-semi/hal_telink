/******************************************************************************
 * Copyright (c) 2022 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/
#ifndef ISO_H_
#define ISO_H_

#define  	ISO_RX_EVENT_LENGTH			(24)	//Note: user can't modify this value,and this value must 4 byte aligned
#define		CIS_TX_PDU_BUFFER_LENGTH	(28)	//Note: user can't modify this value,and this value must 4 byte aligned
#define		BIS_TX_PDU_BUFFER_LENGTH	(20)	//Note: user can't modify this value,and this value must 4 byte aligned
/**
 * @brief      for user to initialize CIS ISO TX FIFO.
 * @param[in]  pRxbuf - TX FIFO buffer address(Tx buffer must concern all CISes).
 * @param[in]  fifo_size - TX FIFO size, size must be 4*n
 * @param[in]  fifo_number - TX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t	blc_ll_initCisTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number);


/**
 * @brief      for user to initialize CIS ISO RX FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size, size must be 4*n
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t	blc_ll_initCisRxFifo(u8 *pRxbuf, int fifo_size, int fifo_number);


/**
 * @brief      for user to initialize CIS RX EVT FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size, size must be 4*n
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t	blc_ll_initCisRxEvtFifo(u8 *pRxbuf, int fifo_size, int fifo_number);


/**
 * @brief      for user to initialize BIS ISO TX FIFO.
 * @param[in]  pRxbuf - TX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t	blc_ll_initBisTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number);


/**
 * @brief      for user to initialize BIS ISO RX FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
//ble_sts_t	blc_ll_initBisRxFifo(u8 *pRxbuf, int fifo_size, int fifo_number);
ble_sts_t blc_ll_initBisRxFifo(u8 *pRxbuf, int full_size, int fifo_number, u8 bis_sync_num);

/**
 * @brief      this function is used by the Host to enable LL feature of Isochronous channels,
 * @param[in]  en - 1:enable 0: disable.
 * @return     status, 0x00:  succeed
 * 			           other: failed
 */
ble_sts_t	blc_setHostFeatureISOChannel_en(u8 en);


#endif


