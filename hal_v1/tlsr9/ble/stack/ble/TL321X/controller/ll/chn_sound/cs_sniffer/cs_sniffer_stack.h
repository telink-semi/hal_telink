/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef CS_SNIFFER_STACK_H_
#define CS_SNIFFER_STACK_H_

#if (LL_CS_SNIFFER_MODE_ENABLE)


_attribute_aligned_(4) typedef struct __attribute__((packed))
{
    u8  event_code;
    u8  status;
    u16 snifHandle;
    u8  event_data[1];      //non-fixed length
} cs_sniffer_event_param_t; //cs sniffer trigger parameters

_attribute_aligned_(4) typedef struct __attribute__((packed))
{
    u8 totalNodeNum;  //main node number and all sub node number, range from 1 to 7
    u8 curNodeIdx;    //MainNode curNodeIdx must be fixed to 0, SubNode curNodeIdx must be from 1 to 6
    u8 curProcedureCountIdx;
    u8 rsvd8;
} cs_sniffer_param_t; //cs sniffer common parameters

extern _attribute_aligned_(4) volatile cs_sniffer_param_t csSniffer_param;

#endif /* end of LL_CS_SNIFFER_MODE_ENABLE */

#endif /* CS_SNIFFER_STACK_H_ */
