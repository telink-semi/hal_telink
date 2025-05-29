set(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR}/../../../../.)
















add_library(feature_test_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${feature_test_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(internal_test_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${internal_test_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(audio_unicast_client_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${audio_unicast_client_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(audio_unicast_server_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${audio_unicast_server_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(cis_central_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${cis_central_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(cis_peripheral_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${cis_peripheral_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(audio_broadcast_source_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${audio_broadcast_source_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(audio_broadcast_sink_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${audio_broadcast_sink_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(audio_broadcast_assistant_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${audio_broadcast_assistant_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(product_bis_sink_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${product_bis_sink_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(product_bis_source_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${product_bis_source_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(product_cis_sink_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${product_cis_sink_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(product_cis_source_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${product_cis_source_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(feature_ota_secure_boot_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${feature_ota_secure_boot_algorithm_audio_alg_lc3_gcc7_SOURCES})
add_library(product_unicast_server_algorithm_audio_alg_lc3_gcc7_Module OBJECT ${product_unicast_server_algorithm_audio_alg_lc3_gcc7_SOURCES})

# assemble_opts_define
target_compile_options(feature_test_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PROJECT_FEATURE_TEST__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(internal_test_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PROJECT_INTERNAL_TEST__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(audio_unicast_client_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PROJECT_AUDIO_UNICAST_CLIENT__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X -D__LE_AUDIO_EN__=1>)
target_compile_options(audio_unicast_server_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PROJECT_AUDIO_UNICAST_SERVER__=1 -D__LE_AUDIO_EN__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(cis_central_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PROJECT_CIS_CEN__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(cis_peripheral_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PROJECT_CIS_PER__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(audio_broadcast_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PROJECT_AUDIO_BROADCAST_SOURCE__=1 -D__LE_AUDIO_EN__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(audio_broadcast_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PROJECT_AUDIO_BROADCAST_SINK__=1 -D__LE_AUDIO_EN__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(audio_broadcast_assistant_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PROJECT_AUDIO_BROADCAST_ASSISTANT__=1 -D__LE_AUDIO_EN__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(product_bis_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PRODUCT_BIS_SINK_DEMO__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(product_bis_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PRODUCT_BIS_SOURCE_DEMO__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(product_cis_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PRODUCT_CIS_SINK_DEMO__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(product_cis_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PRODUCT_CIS_SOURCE_DEMO__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(feature_ota_secure_boot_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PROJECT_FEATURE_DEMO__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(product_unicast_server_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: -D__PROJECT_PRODUCT_UNICAST_SERVER__=1 -D__LE_AUDIO_EN__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)

# assemble_opts_include
set(ASM_INCLUDE_OPTS -I${ProjDirPath}/build/TL721X/ -I${ProjDirPath}/3rd-party/freertos-V5/include -I${ProjDirPath}/3rd-party/freertos-V5/portable/GCC/RISC-V -I${ProjDirPath}/build/TL721X/../.. -I${ProjDirPath}/drivers/TL721X -I${ProjDirPath}/common -I${ProjDirPath}/vendor/common)
target_compile_options(feature_test_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(internal_test_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(audio_unicast_client_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(audio_unicast_server_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(cis_central_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(cis_peripheral_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(audio_broadcast_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(audio_broadcast_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(audio_broadcast_assistant_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(product_bis_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(product_bis_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(product_cis_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(product_cis_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(feature_ota_secure_boot_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)
target_compile_options(product_unicast_server_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)

# assemble_opts_contents
set(ASM_OPTS -mcmodel=medium -fmessage-length=0 -ffunction-sections -fdata-sections -flto -O2 -g3 -Wno-address-of-packed-member -Wall -Wextra -Wshadow -Wimplicit-fallthrough -Wpointer-arith -Wredundant-decls -Wcast-qual -Wsign-compare -Wunused-parameter -Wunused-variable -Wswitch -Wstrict-prototypes -Wmissing-field-initializers -Wdeprecated-declarations -Wenum-conversion -Wpacked-not-aligned -Wundef -x assembler-with-cpp -mext-dsp -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fno-jump-tables)
target_compile_options(feature_test_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(internal_test_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(audio_unicast_client_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(audio_unicast_server_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(cis_central_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(cis_peripheral_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(audio_broadcast_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(audio_broadcast_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(audio_broadcast_assistant_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(product_bis_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(product_bis_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(product_cis_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(product_cis_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(feature_ota_secure_boot_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)
target_compile_options(product_unicast_server_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)

# c_opts_define
target_compile_options(feature_test_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_FEATURE_TEST__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(internal_test_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_INTERNAL_TEST__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(audio_unicast_client_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_AUDIO_UNICAST_CLIENT__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(audio_unicast_server_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_AUDIO_UNICAST_SERVER__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(cis_central_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_CIS_CEN__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(cis_peripheral_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_CIS_PER__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(audio_broadcast_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_AUDIO_BROADCAST_SOURCE__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(audio_broadcast_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_AUDIO_BROADCAST_SINK__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(audio_broadcast_assistant_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_AUDIO_BROADCAST_ASSISTANT__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(product_bis_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_FEATURE_DEMO__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(product_bis_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_FEATURE_DEMO__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(product_cis_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_FEATURE_DEMO__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(product_cis_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_FEATURE_DEMO__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(feature_ota_secure_boot_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_FEATURE_DEMO__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)
target_compile_options(product_unicast_server_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: -D__PROJECT_PRODUCT_UNICAST_SERVER__=1 -DCHIP_TYPE=CHIP_TYPE_TL721X>)

# c_opts_include
set(C_INCLUDE_OPTS -I${ProjDirPath}/build/TL721X/ -I${ProjDirPath}/3rd-party/freertos-V5/include -I${ProjDirPath}/3rd-party/freertos-V5/portable/GCC/RISC-V -I${ProjDirPath}/build/TL721X/../.. -I${ProjDirPath}/drivers/TL721X -I${ProjDirPath}/common -I${ProjDirPath}/vendor/common -I${ProjDirPath}/stack/ble/hal/hal_TL721X -I${TOOLCHAIN_PATH}/lib/gcc/riscv32-elf/10.3.0/include -I${TOOLCHAIN_PATH}/riscv32-elf/include)
target_compile_options(feature_test_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(internal_test_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(audio_unicast_client_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(audio_unicast_server_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(cis_central_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(cis_peripheral_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(audio_broadcast_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(audio_broadcast_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(audio_broadcast_assistant_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(product_bis_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(product_bis_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(product_cis_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(product_cis_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(feature_ota_secure_boot_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)
target_compile_options(product_unicast_server_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)

# c_opts_contents
set(C_OPTS -mcmodel=medium -fmessage-length=0 -ffunction-sections -fdata-sections -flto -O2 -g3 -Wno-address-of-packed-member -Wall -Wextra -Wshadow -Wimplicit-fallthrough -Wpointer-arith -Wredundant-decls -Wcast-qual -Wsign-compare -Wunused-parameter -Wunused-variable -Wswitch -Wstrict-prototypes -Wmissing-field-initializers -Wdeprecated-declarations -Wenum-conversion -Wpacked-not-aligned -Wundef -mext-dsp -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fno-jump-tables -fno-fat-lto-objects)
target_compile_options(feature_test_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(internal_test_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(audio_unicast_client_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(audio_unicast_server_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(cis_central_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(cis_peripheral_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(audio_broadcast_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(audio_broadcast_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(audio_broadcast_assistant_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(product_bis_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(product_bis_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(product_cis_sink_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(product_cis_source_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(feature_ota_secure_boot_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)
target_compile_options(product_unicast_server_algorithm_audio_alg_lc3_gcc7_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)

