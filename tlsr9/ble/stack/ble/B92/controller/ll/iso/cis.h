/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef CIS_H_
#define CIS_H_

#define			CIS_TX_PDU_BUF_EXT_LEN							12	//user can't modify this value !!!



#define			CIS_CONN_PARAM_LENGTH							sizeof(ll_cis_conn_t) //396	//user can't modify this value !!!




/**
 * @brief      for user to initialize CIS Connection module and allocate CIS connection parameters buffer.
 * @param[in]  pCisConnParaBuf - start address of CIS connection parameters buffer
 * @param[in]  cis_cen_num - CIS Central number application layer may use
 * @param[in]  cis_per_num - CIS Peripheral number application layer may use
 * @return     status, 0x00:  succeed
 * 			           other: failed
 */
ble_sts_t 	blc_ll_initCisConnModule_initCisConnParametersBuffer(u8 *pCisConnParaBuf, u32 cis_cen_num, u32 cis_per_num);

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
 * @brief      This function is used to initialize IAL CIS SDU in and out buffer.
 * @param[in]  in_fifo
 * @param[in]  in_fifo_size
 * @param[in]  in_fifo_num
 * @param[in]  out_fifo
 * @param[in]  out_fifo_size
 * @param[in]  out_fifo_num
 */
void 		blc_ll_initCisSduBuffer(u8 *in_fifo, int in_fifo_size, u8 in_fifo_num, u8 *out_fifo, int out_fifo_size, u8 out_fifo_num);



/**
 * @brief      This function is used to disconnect CIS conection.
 * @param[in]  cisHandle - CIS connection handle.
 * @param[in]  reason - Reason for disconnection..
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t	blc_ll_cis_disconnect(u16 cisHandle, u8 reason);




typedef enum{
	CIS_PDU_STRATEGY0=0, /*Link lay sends empty PDUs when the upper layer has no data*/
	CIS_PDU_STRATEGY1,	 /*Link lay sends NULL PDUs when the upper layer has no data*/
	CIS_PDU_STRATEGY2,	 /*Link lay Send Empty when the upper layer data has not arrived after 1 iso_interval,or send NULL PDU*/
}cis_pdu_strategy_t;


/**
 * @brief      This function is used to set sending strategy when no data is sent from the upper layer.
 * @param[in]  stgy - strategy that is set.
 */
void blc_ll_setCisSupplementPDUStrategy(cis_pdu_strategy_t stgy);


/**
 * @brief      This function is used to get sdu buffer num valid.
 * @param[in]  cisHandle.
 * @return      Status - 0x00:  no buffer num valid or cisHandle invalid.
 * 						 other:  valid buffer num.
 */
int blc_ll_getCisSduInBufferFreeNum(u16 cisHandle);

sdu_packet_t* blc_ll_popCisRxSduData(u16 cis_connHandle);

#endif /* CIS_H_ */
