#!/bin/bash 
echo "*****************************************************"
riscv32-elf-objcopy -v -O binary $1.elf  $1.bin
../../../vendor/2p4g_feature_test/script/bin_append.exe $1.bin $1_NEW.bin 
if [ $? == 0 ];then echo "$1.bin tail add crc finish !";fi;echo "*****************************************************"