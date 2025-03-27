/********************************************************************************************************
 * @file    app_parse_char.h
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
#pragma once

typedef struct __attribute__((packed)) {
    char *fun_name;
    void (*fun)(char *argv[], int argc, void *user_data);
    void *user_data;
} parse_fun_list_t;


/**
 * @brief        parse initial function.
 * @param[in]    parseList: parse command list.
 * @param[in]    size: list size.
 * @return        none.
 */
void app_parse_init(const parse_fun_list_t *parseList, int size);

/**
 * @brief        parse string loop.
 * @param[in]    none.
 * @return        none.
 */
void app_parse_loop(void);

/**
 * @brief        parse string to immediate value.
 * @param[in]    ps: value string, '\0' ending, supported -1, -0xAB, 1.
 * @return        immediate value.
 */
int app_parse_str2n (char * ps);

/**
 * @brief        parse print log function.
 * @param[in]    Refer to printf parameter description.
 * @return        none.
 */
void app_parse_printf(const char *format, ...);

