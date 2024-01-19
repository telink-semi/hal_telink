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
#pragma once


#define	CHIP_TYPE_B91		1
#define	CHIP_TYPE_B92		2


#ifndef CHIP_TYPE
#define	CHIP_TYPE 			CHIP_TYPE_B91
#endif






#define	MCU_CORE_B91 		1
#define	MCU_CORE_B92 		2
#define MCU_CORE_B93		3

#if(CHIP_TYPE == CHIP_TYPE_B91)
	#define MCU_CORE_TYPE	MCU_CORE_B91
#elif(CHIP_TYPE == CHIP_TYPE_B92)
	#define MCU_CORE_TYPE	MCU_CORE_B92
#elif(CHIP_TYPE == CHIP_TYPE_B92)
	#define MCU_CORE_TYPE	MCU_CORE_B93
#endif



