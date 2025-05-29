/********************************************************************************************************
 * @file    app_parse_ui.h
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
#ifndef APP_PARSE_UI_H_
#define APP_PARSE_UI_H_

#if (INTER_TEST_MODE == TEST_RAS_SERVER)

extern u8 advCnt;

/**
 * @brief       parse UI initial function.
 * @param[in]   none.
 * @return      none.
 */
void app_parse_ui_init(void);

/**
 * @brief       provide UI with ble address used
 * @param[in]   ble address
 * @return      none.
 */
void app_parse_ui_set_addr(u8 *addr);

/**
 * @brief       central UI loop function.
 * @param[in]   none.
 * @return      none.
 */
void app_parse_ui_loop(void);

#endif /* (INTER_TEST_MODE == TEST_RAS_SERVER) */
#endif /* APP_UI_H_ */
