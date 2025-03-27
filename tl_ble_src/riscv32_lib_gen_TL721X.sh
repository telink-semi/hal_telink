rm -f liblt_TL721X.a
find ./algorithm -name "*.o" -type f -print -exec riscv32-elf-ar -crs liblt_TL721X.a  {} \;
find ./drivers/TL721X/lib/src -name "*.o" -type f -print -exec riscv32-elf-ar -crs liblt_TL721X.a  {} \;
# under ext_driver: gen lib only with driver_lib/
find ./drivers/TL721X/ext_driver/driver_internal -name "*.o"  -type f -print -exec riscv32-elf-ar -crs liblt_TL721X.a  {} \;
# under stack: no archive audio services/ , audio *_buf.o
find ./stack -path ./stack/ble/profile/services -prune -o ! -name "*_buf.o" -name "*.o" -type f -print -exec riscv32-elf-ar -crs liblt_TL721X.a  {} \;





