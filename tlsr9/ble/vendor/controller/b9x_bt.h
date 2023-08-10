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

#ifndef B9X_BT_H_
#define B9X_BT_H_

/**
 *  @brief b9x_bt_contriller_state
 *  B9X Bluetooth controller state
 */
enum b9x_bt_controller_state {
	B9X_BT_CONTROLLER_STATE_STOPPED = 0,
	B9X_BT_CONTROLLER_STATE_ACTIVE,
	B9X_BT_CONTROLLER_STATE_STOPPING
};

/**
 *  @brief b9x_bt_host_callback
 *  used for vhci call host function to notify what host need to do
 */
typedef struct b9x_bt_host_callback {
    void (*host_send_available)(void);                      /* the host can send packet to the controller */
    void (*host_read_packet)(uint8_t *data, uint16_t len);  /* the controller has a packet to send to the host */
} b9x_bt_host_callback_t;

/**
 * @brief register the host reference callback
 */
void b9x_bt_host_callback_register(const b9x_bt_host_callback_t *callback);

/**
 * @brief     Host send HCI packet to controller
 * @param     data the packet point
 * @param     len the packet length
 */
void b9x_bt_host_send_packet(uint8_t type, const uint8_t *data, uint16_t len);

/**
 * @brief     Telink B9X BLE Controller initialization
 * @return    Status - 0: command succeeded; -1: command failed
 */
int b9x_bt_controller_init(void);

/**
 * @brief     Telink B9X BLE Controller deinitialization
 */
void b9x_bt_controller_deinit(void);

/**
 * @brief     Get state of Telink B9X BLE Controller
 */
enum b9x_bt_controller_state b9x_bt_controller_state(void);


#endif /* B9X_BT_H_ */
