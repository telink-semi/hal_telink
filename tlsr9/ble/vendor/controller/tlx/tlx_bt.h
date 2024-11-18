/******************************************************************************
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef TLX_BT_H_
#define TLX_BT_H_

/**
 *  @brief tl_bt_contriller_state
 *  TLX Bluetooth controller state
 */
enum tl_bt_controller_state {
	TL_BT_CONTROLLER_STATE_STOPPED = 0,
	TL_BT_CONTROLLER_STATE_ACTIVE,
	TL_BT_CONTROLLER_STATE_STOPPING
};

/**
 *  @brief tlx_bt_host_callback
 *  used for vhci call host function to notify what host need to do
 */
typedef struct tlx_bt_host_callback {
    void (*host_send_available)(void);                      /* the host can send packet to the controller */
    void (*host_read_packet)(uint8_t *data, uint16_t len);  /* the controller has a packet to send to the host */
} tlx_bt_host_callback_t;

/**
 * @brief register the host reference callback
 */
void tlx_bt_host_callback_register(const tlx_bt_host_callback_t *callback);

/**
 * @brief     Host send HCI packet to controller
 * @param     data the packet point
 * @param     len the packet length
 */
void tlx_bt_host_send_packet(uint8_t type, const uint8_t *data, uint16_t len);

/**
 * @brief     Telink TLX BLE Controller initialization
 * @return    Status - 0: command succeeded; -1: command failed
 */
int tlx_bt_controller_init(void);

/**
 * @brief     Telink TLX BLE Controller deinitialization
 */
void tlx_bt_controller_deinit(void);

/**
 * @brief     Get state of Telink TLX BLE Controller
 */
enum tl_bt_controller_state tl_bt_controller_state(void);


#endif /* TLX_BT_H_ */
