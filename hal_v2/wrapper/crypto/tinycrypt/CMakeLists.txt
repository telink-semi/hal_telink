#
# Copyright (c) 2026, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BOOT_USE_TINYCRYPT
   AND CONFIG_TELINK_TLX_TINYCRYPT_HW_ACCELERATION
   AND CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256)
    
    zephyr_library()

    zephyr_library_sources(
        mcuboot_ecc_signature_verify.c
    )

    zephyr_link_libraries(
        -Wl,--wrap,uECC_verify
    )

endif()
