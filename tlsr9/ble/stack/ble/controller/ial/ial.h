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
#ifndef IAL_H_
#define IAL_H_




#include "stack/ble/controller/ll/iso/iso.h"




/*
 * First fragment of sdu, data field
 */
#define HCI_ISO_PKT_HDR_HANDLE_LEN					(2)
#define HCI_ISO_PKT_HDR_DATA_LEN					(2)
#define	HCI_ISO_LOAD_HDR_TIMESTAMP_LEN				(4)
#define	HCI_ISO_LOAD_HDR_PACKET_SN_LEN				(2)
#define	HCI_ISO_LOAD_SDU_LEN						(2)

#define HCI_ISO_PKT_HDR_LEN							(HCI_ISO_PKT_HDR_HANDLE_LEN + HCI_ISO_PKT_HDR_DATA_LEN)

#define HCI_ISO_LOAD_HDR_LEN_MAX					(HCI_ISO_LOAD_HDR_TIMESTAMP_LEN + HCI_ISO_LOAD_HDR_PACKET_SN_LEN + HCI_ISO_LOAD_SDU_LEN)
#define HCI_ISO_LOAD_HDR_LEN_MIN					(HCI_ISO_LOAD_HDR_PACKET_SN_LEN + HCI_ISO_LOAD_SDU_LEN)


#define ISO_FRAMED_SEGM_HEADER_LEN					(2)
#define	ISO_FRAMED_TIMEOFFSET_LEN					(3)


/*
 * HCI ISO data packet
 */
typedef struct{

//0
	u32 timestamp;
	u16 offset;
	u16 sn_offset;


//8
	u16 connHandle		 :12;
	u16 pb         	 	 :2;
	u16 ts				 :1;
	u16 RFU2			 :1;
//10
	u16 iso_dl_len		 :14;  //iso_data_load_length
	u16 RFU3			 :2;


//11
	u8 data[1];

}iso_data_packet_t;






/******************************* Macro & Enumeration & Structure Definition for Stack End ******************************/






/******************************* Macro & Enumeration variables for User Begin ******************************************/



/******************************* Macro & Enumeration variables for User End ********************************************/














/******************************* User Interface  Begin *****************************************************************/

/**
 * @brief      This function is used to initialize the ISOAL module.
 */
void 		blc_ial_initSdu_module(void);

/**
 * @brief      This function is used to initialize sdu buff.
 * @param[in]  rx_fifo
 * @param[in]  rx_fifo_size
 * @param[in]  rx_fifo_num
 * @param[in]  tx_fifo
 * @param[in]  tx_fifo_size
 * @param[in]  tx_fifo_num
 */
void 		blc_ial_initCisSduBuff(u8 *rx_fifo,u16 rx_fifo_size, u8 rx_fifo_num, u8 *tx_fifo,u16 tx_fifo_size, u8 tx_fifo_num);

/**
 * @brief      This function is used to initialize cis sdu buff.
 * @param[in]  rx_fifo
 * @param[in]  rx_fifo_size
 * @param[in]  rx_fifo_num
 * @param[in]  tx_fifo
 * @param[in]  tx_fifo_size
 * @param[in]  tx_fifo_num
 */
void		blc_ial_initBisSduBuff(u8 *rx_fifo,u16 rx_fifo_size, u8 rx_fifo_num, u8 *tx_fifo,u16 tx_fifo_size, u8 tx_fifo_num);

/**
 * @brief      This function is used to pack HCI ISO data packet to SDU packet.
 * @param[in]  cis_connHandle - point to handle of cis.
 * @param[in]  pIsoData - point to hci ISO Data packet buff.
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_hci_packIsoData(u16 cis_connHandle, u8 *pIsoData);

/**
 * @brief      This function is used to setup ISO Data Path.
 * @param[in]  refer to the structure 'hci_le_setupIsoDataPathCmdParams_t'
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t	blc_isoal_le_setupISODataPath_cmd(hci_le_setupIsoDataPathCmdParams_t *para);

/**
 * @brief      This function is used to segmentation SDU to one Framed PDUs.
 * @param[in]  cis_connHandle
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 * 						 LL_ERR_INVALID_PARAMETER: command failed
 */
ble_sts_t	blc_ial_splitCisSdu2FramedPdu(u16 cis_connHandle);


/**
 * @brief      This function is used to fragmentation SDU to one or more Unframed PDUs.
 * @param[in]  cis_connHandle
 * @param[in]  sdu  point to sdu buff
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 * 						 LL_ERR_INVALID_PARAMETER: command failed
 */
ble_sts_t 	blc_ial_cis_splitSdu2UnframedPdu(u16 cisHandle, iso_data_packet_t *sdu);


/**
 * @brief      This function is used to fragmentation SDU to one or more Unframed PDUs.
 * @param[in]  bis_connHandle
 * @param[in]  sdu  point to sdu buff
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 * 						 LL_ERR_INVALID_PARAMETER: command failed
 */
ble_sts_t 	blc_ial_bis_splitSdu2UnframedPdu(u16 bis_connHandle, iso_data_packet_t *sdu);


/******************************* User Interface  End  ******************************************************************/



#endif


