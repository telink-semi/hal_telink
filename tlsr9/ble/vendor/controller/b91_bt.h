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

#ifndef B91_BT_H_
#define B91_BT_H_

/**
 *  @brief b91_bt_host_callback
 *  used for vhci call host function to notify what host need to do
 */
typedef struct b91_bt_host_callback {
    void (*host_send_available)(void);                      /* the host can send packet to the controller */
    void (*host_read_packet)(uint8_t *data, uint16_t len);  /* the controller has a packet to send to the host */
} b91_bt_host_callback_t;

/**
 * @brief register the host reference callback
 */
void b91_bt_host_callback_register(const b91_bt_host_callback_t *callback);

/**
 * @brief     Host send HCI packet to controller
 * @param     data the packet point
 * @param     len the packet length
 */
void b91_bt_host_send_packet(uint8_t type, uint8_t *data, uint16_t len);

/**
 * @brief     Telink B91 BLE Controller initialization
 * @return    Status - 0: command succeeded; -1: command failed
 */
int b91_bt_controller_init(void);

#endif /* B91_BT_H_ */
