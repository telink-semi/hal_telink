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

#ifndef B91_BT_BUFFER_H_
#define B91_BT_BUFFER_H_

#include "stack/ble/ble.h"
#include "zephyr/bluetooth/buf.h"


#define ACL_CONN_MAX_RX_OCTETS (BT_BUF_RX_SIZE > 251 ? 251 : BT_BUF_RX_SIZE)
#define ACL_SLAVE_MAX_TX_OCTETS (CONFIG_BT_BUF_ACL_TX_SIZE > 251 ? 251 : CONFIG_BT_BUF_ACL_TX_SIZE)
#define ACL_MASTER_MAX_TX_OCTETS (CONFIG_BT_BUF_ACL_TX_SIZE > 251 ? 251 : CONFIG_BT_BUF_ACL_TX_SIZE)

#define BT_BUF_TX_SIZE ACL_SLAVE_MAX_TX_OCTETS

/* Macros to align values to specific size */
#define ALIGN_2 1
#define ALIGN_4 2
#define ALIGN_8 3
#define ALIGN_16 4
#define ALIGN(x, order)                                                                            \
	(((x) % (1 << (order))) == 0 ? (x) : ((((x) >> (order)) << (order)) + (1 << order)))

/* Macro to find next power of two value */
/* Found Here: https://lists.freebsd.org/pipermail/freebsd-current/2007-February/069093.html */
#define NPOT2(x) ((x) | ((x) >> 1))
#define NPOT4(x) (NPOT2(x) | (NPOT2(x) >> 2))
#define NPOT8(x) (NPOT4(x) | (NPOT4(x) >> 4))
#define NPOT16(x) (NPOT8(x) | (NPOT8(x) >> 8))
#define NPOT32(x) (NPOT16(x) | (NPOT16(x) >> 16))
#define NEXT_POWER_OF_2(x) (NPOT32(x - 1) + 1)

/* Macro to calculate ACL TX master/slave buffer number. */
/* According to Telink implementation, shall be 9, 17 or 33 */
#define CAL_LL_ACL_BUF_NUM(x) (x <= 9 ? 9 : x <= 17 ? 17 : 33)

/*
    TX from host CMD or ACL buffer, RX to Contorller HCI buffer
    According to Telink implementatios, the buffer shall be alligned to 16.
*/
#define HCI_RX_FIFO_SIZE HCI_FIFO_SIZE(BT_BUF_TX_SIZE)

/*
    According to Telink implementation shall number of buffers shall be power of 2
*/
#define HCI_RX_FIFO_NUM NEXT_POWER_OF_2(MAX(CONFIG_BT_BUF_CMD_TX_COUNT, CONFIG_BT_BUF_ACL_TX_COUNT))

/*
    TX from controller, RX to Host
    According to Telink implementatios, the buffer shall be alligned to 4.
*/
#define HCI_TX_FIFO_SIZE HCI_FIFO_SIZE(ACL_CONN_MAX_RX_OCTETS)

/*
    According to Telink implementation shall number of buffers shall be power of 2
*/
#define HCI_TX_FIFO_NUM NEXT_POWER_OF_2(BT_BUF_RX_COUNT)

/*
    Intermediate ACL buffer that takes data from HCI RX and pass it to ACL TX
    According to Telink implementatios, the buffer shall be alligned to 4.
*/
#define HCI_RX_ACL_FIFO_SIZE ALIGN(BT_BUF_ACL_SIZE(ACL_CONN_MAX_RX_OCTETS), ALIGN_4)

/*
    According to Telink implementation shall number of buffers shall be power of 2
*/
#define HCI_RX_ACL_FIFO_NUM NEXT_POWER_OF_2(CONFIG_BT_BUF_ACL_TX_COUNT)

/*
    Data from radio to RX ACL buffer
    According to Telink implementatios, the buffer shall be alligned to 16.
*/
#define ACL_RX_FIFO_SIZE CAL_LL_ACL_RX_FIFO_SIZE(ACL_CONN_MAX_RX_OCTETS)

/*
    Number of ACL RX buffers. Shall be power of 2
*/
#define ACL_RX_FIFO_NUM NEXT_POWER_OF_2(BT_BUF_RX_COUNT)

/*
    Data from ACL TX to radio
    According to Telink implementatios, the buffer shall be alligned to 16.
*/
#define ACL_SLAVE_TX_FIFO_SIZE CAL_LL_ACL_TX_FIFO_SIZE(BT_BUF_TX_SIZE)

/*
    Number of ACL TX buffers. Shall be 9, 17, 33
*/
#define ACL_SLAVE_TX_FIFO_NUM CAL_LL_ACL_BUF_NUM(CONFIG_BT_BUF_ACL_TX_COUNT)

/*
    Data from ACL TX to radio
    According to Telink implementatios, the buffer shall be alligned to 16.
*/
#define ACL_MASTER_TX_FIFO_SIZE CAL_LL_ACL_TX_FIFO_SIZE(BT_BUF_TX_SIZE)

/*
    Number of ACL TX buffers. Shall be 9, 17, 33
*/
#define ACL_MASTER_TX_FIFO_NUM CAL_LL_ACL_BUF_NUM(CONFIG_BT_BUF_ACL_TX_COUNT)

extern u8 app_acl_rxfifo[];
extern u8 app_acl_mstTxfifo[];
extern u8 app_acl_slvTxfifo[];

extern u8 app_hci_rxfifo[];
extern u8 app_hci_txfifo[];
extern u8 app_hci_rxAclfifo[];

#endif /* B91_BT_BUFFER_H_ */
