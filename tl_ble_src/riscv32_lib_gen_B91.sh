rm -f liblt_B91.a
find ./algorithm -name "*.o" -type f -print -exec riscv32-elf-ar -crs liblt_B91.a  {} \;
find ./drivers/B91/lib/src -name "*.o" -type f -print -exec riscv32-elf-ar -crs liblt_B91.a  {} \;
# under ext_driver: gen lib only with driver_lib/
find ./drivers/B91/ext_driver/driver_internal -name "*.o"  -type f -print -exec riscv32-elf-ar -crs liblt_B91.a  {} \;
# under stack: no archive audio services/ , audio *_buf.o
find ./stack -path ./stack/ble/profile/services -prune -o ! -name "*_buf.o" -name "*.o" -type f -print -exec riscv32-elf-ar -crs liblt_B91.a  {} \;


