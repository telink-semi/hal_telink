rm -f liblt_B92.a
find ./algorithm -name "*.o" -type f -print -exec riscv32-elf-ar -crs liblt_B92.a  {} \;
find ./drivers/B92/lib/src -name "*.o" -type f -print -exec riscv32-elf-ar -crs liblt_B92.a  {} \;
# under ext_driver: gen lib only with driver_lib/
find ./drivers/B92/ext_driver/driver_internal -name "*.o"  -type f -print -exec riscv32-elf-ar -crs liblt_B92.a  {} \;
# under stack: no archive audio services/ , audio *_buf.o
find ./stack -path ./stack/ble/profile/services -prune -o ! -name "*_buf.o" -name "*.o" -type f -print -exec riscv32-elf-ar -crs liblt_B92.a  {} \;





