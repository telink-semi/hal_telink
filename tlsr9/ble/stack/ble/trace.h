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
#ifndef STACK_BLE_TRACE_H_
#define STACK_BLE_TRACE_H_





//log_event, ID: 0~31
#define			SLEV_reservd				0

#define			SLEV_rebuild				10
#define			SLEV_rst_sSlot				11




//log_tick, ID: 0~31

#define			SLET_00_systimer			0
#define 		SLET_01_rx					1
#define 		SLET_02_rx_scn				2
#define			SLET_03_rx_slv     			3
#define			SLET_04_rx_mas				4
#define			SLET_timestamp				31

//log_task, ID: 0~31
#define			SL01_01_scan				1
#define			SL01_02_adv     			2
#define			SL01_03_btx1     			3
#define			SL01_04_btx2     			4
#define			SL01_05_btx3     			5
#define			SL01_06_btx4     			6
#define			SL01_07_brx1     			7
#define			SL01_08_brx2     			8
#define			SL01_09_brx3     			9
#define			SL01_10_brx4     			10

#define			SL01_29_sysTimer			29
#define			SL01_30_rf					30
#define			SL01_31_IRQ					31





// 8-bit data: cid0 - cid63
#define			SL08_reserved				0



// 16-bit data: sid0 - sid63
#define			SL16_reserved				0




#endif /* STACK_BLE_TRACE_H_ */
