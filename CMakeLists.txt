#
# Copyright (c) 2025, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_HAS_TELINK_DRIVERS)

# SOC configuration for Telink RISC-V series
string(REGEX REPLACE "^telink_" "" SOC_FAMILY ${CONFIG_SOC_SERIES})



if(CONFIG_SOC_RISCV_TELINK_B91 OR CONFIG_SOC_RISCV_TELINK_B92 OR 
	CONFIG_SOC_RISCV_TELINK_TL321X OR CONFIG_SOC_RISCV_TELINK_TL721X)
	add_subdirectory(hal_v1)
elseif(CONFIG_SOC_RISCV_TELINK_TL322X OR CONFIG_SOC_RISCV_TELINK_TL323X OR CONFIG_SOC_RISCV_TELINK_TL523X)
	add_subdirectory(hal_v2)
else()
	message(FATAL_ERROR "Not supported SOC")
endif()

endif() # CONFIG_HAS_TELINK_DRIVERS
