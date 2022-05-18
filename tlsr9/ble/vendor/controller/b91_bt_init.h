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

#ifndef B91_BT_INIIT_H_
#define B91_BT_INIIT_H_

#define INIT_OK           (0)
#define INIT_FAILED       (-1)

/**
 * @brief       Telink B91 BLE Controller initialization
 * @param[in]   prx - HCI RX callback
 * @param[in]   ptx -HCI TX callback
 * @return      Status - 0: command succeeded; -1: command failed
 */
int b91_bt_blc_init(void *prx, void *ptx);

#endif /* B91_BT_INIIT_H_ */
