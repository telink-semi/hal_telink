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

/********************************************************************************************************
 * @file	compiler.h
 *
 * @brief	This is the header file for B91
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
#ifndef COMPILER_H_
#define COMPILER_H_

#define _attribute_noinline_                    __attribute__((noinline))
#define _attribute_ram_code_sec_                __attribute__((section(".ram_code")))
#define _attribute_ram_code_sec_noinline_       __attribute__((section(".ram_code"))) __attribute__((noinline))
#define _attribute_ram_code_                    _attribute_ram_code_sec_noinline_
#define _attribute_text_sec_                    __attribute__((section(".text")))
#define _attribute_aes_data_sec_                __attribute__((section(".aes_data")))
#define _attribute_data_retention_sec_          __attribute__((section(".retention_data")))
#define _attribute_data_retention_              __attribute__((section(".retention_data")))
#define _attribute_ble_data_retention_          __attribute__((section(".retention_data")))
#define _attribute_aligned_(s)                  __attribute__((aligned(s)))
#define _attribute_no_inline_                   __attribute__((noinline))
#define _attribute_data_dlm_                    __attribute__((section(".dlm_data")))
#define _attribute_session_(s)                  __attribute__((section(s)))

#endif
