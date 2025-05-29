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


#define FAST_DISTANCE_ESTIMATE 0

#ifndef FAST_DISTANCE_ESTIMATE
#define FAST_DISTANCE_ESTIMATE 0
#endif

#define MAX_ADV_INFO_NUM 20

typedef struct __attribute__((packed))
{
    u8 addrType;
    u8 address[6];
} adv_info_t;

extern adv_info_t advInfoTable[MAX_ADV_INFO_NUM];

extern u8 advCnt;
extern u8 reconn_en;
/**
 * @brief       parse UI initial function.
 * @param[in]   none.
 * @return      none.
 */
void app_parse_ui_init(void);

/**
 * @brief       central UI loop function.
 * @param[in]   none.
 * @return      none.
 */
void app_parse_ui_loop(void);

/**
 * @brief       found advertisement device event.
 * @param[in]   p: Data carried by the event.
 * @return      0.
 */
int app_parse_foundAdv(u8 *p);

#endif /* APP_UI_H_ */
