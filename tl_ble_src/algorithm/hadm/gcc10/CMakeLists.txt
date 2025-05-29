set(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR}/../../../.)

recursive_glob(${CMAKE_CURRENT_SOURCE_DIR} sniffer_sub_node_algorithm_hadm_gcc10_SOURCES)

add_library(sniffer_sub_node_algorithm_hadm_gcc10_Module OBJECT ${sniffer_sub_node_algorithm_hadm_gcc10_SOURCES})

# assemble_opts_define
set(ASM_DEFINE_OPTS -DCHIP_TYPE=CHIP_TYPE_B92 -D__CHANNEL_SOUNDING_EN__=1 -D__PROJECT_SNIF_SUB_NODE__=1)
target_compile_options(sniffer_sub_node_algorithm_hadm_gcc10_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_DEFINE_OPTS}>)

# assemble_opts_include
set(ASM_INCLUDE_OPTS -I${ProjDirPath}/build/B92/ -I${ProjDirPath}/3rd-party/freertos-V5/include -I${ProjDirPath}/3rd-party/freertos-V5/portable/GCC/RISC-V -I${ProjDirPath}/build/B92/../.. -I${ProjDirPath}/drivers/B92 -I${ProjDirPath}/common -I${ProjDirPath}/vendor/common -I${ProjDirPath}/stack/ble/hal/hal_B92)
target_compile_options(sniffer_sub_node_algorithm_hadm_gcc10_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_INCLUDE_OPTS}>)

# assemble_opts_contents
set(ASM_OPTS -mcmodel=medium -fmessage-length=0 -ffunction-sections -fdata-sections -flto -O2 -g3 -Wno-address-of-packed-member -x assembler-with-cpp -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fno-jump-tables)
target_compile_options(sniffer_sub_node_algorithm_hadm_gcc10_Module PRIVATE $<$<COMPILE_LANGUAGE:ASM>: ${ASM_OPTS}>)

# c_opts_define
set(C_DEFINE_OPTS -DCHIP_TYPE=CHIP_TYPE_B92 -D__PROJECT_SNIF_SUB_NODE__=1)
target_compile_options(sniffer_sub_node_algorithm_hadm_gcc10_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_DEFINE_OPTS}>)

# c_opts_include
set(C_INCLUDE_OPTS -I${ProjDirPath}/build/B92/ -I${ProjDirPath}/3rd-party/freertos-V5/include -I${ProjDirPath}/3rd-party/freertos-V5/portable/GCC/RISC-V -I${ProjDirPath}/build/B92/../.. -I${ProjDirPath}/drivers/B92 -I${ProjDirPath}/common -I${ProjDirPath}/vendor/common -I${TOOLCHAIN_PATH}/lib/gcc/riscv32-elf/10.3.0/include -I${TOOLCHAIN_PATH}/riscv32-elf/include -I${ProjDirPath}/stack/ble/hal/hal_B92)
target_compile_options(sniffer_sub_node_algorithm_hadm_gcc10_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_INCLUDE_OPTS}>)

# c_opts_contents
set(C_OPTS -mcmodel=medium -fmessage-length=0 -ffunction-sections -fdata-sections -flto -O2 -g3 -Wno-address-of-packed-member -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fshort-wchar -fuse-ld=bfd -fno-jump-tables -fno-fat-lto-objects)
target_compile_options(sniffer_sub_node_algorithm_hadm_gcc10_Module PRIVATE $<$<COMPILE_LANGUAGE:C>: ${C_OPTS}>)

