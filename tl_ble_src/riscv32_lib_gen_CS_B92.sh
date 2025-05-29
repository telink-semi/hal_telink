rm -f liblt_B92_cs.a
find ./algorithm -name "*.o" -type f -print -exec riscv32-elf-ar -crs liblt_B92_cs.a  {} \;
find ./drivers/B92/lib/src -name "*.o" -type f -print -exec riscv32-elf-ar -crs liblt_B92_cs.a  {} \;
# under ext_driver: gen lib only with driver_lib/
find ./drivers/B92/ext_driver/driver_internal -name "*.o"  -type f -print -exec riscv32-elf-ar -crs liblt_B92_cs.a  {} \;
# under stack: no archive audio services/ , audio *_buf.o
find ./stack ! -name "*_buf.o" -name "*.o" -type f -print -exec riscv32-elf-ar -crs liblt_B92_cs.a  {} \;





