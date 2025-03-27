#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "stack/ble/controller/cs_drbg/drbg_stack.h"
#if (INTER_TEST_MODE == TEST_CS_DRBG)
//h9 test data
u8 st1_h9_cs_iv[]={0x3b ,0x0b ,0xca ,0xe0 ,0x86 ,0x51 ,0x7f ,0x3e ,\
                                0xe9 ,0xdf ,0xfd ,0x0b ,0x8a ,0xc2 ,0x0b ,0xe1 };
u8 st1_h9_cs_in[]={0x0d ,0x84 ,0x73 ,0x86 ,0xc1 ,0x77 ,0xf4 ,0x9f };
u8 st1_h9_cs_pv[]={0x43 ,0xf1 ,0x68 ,0x78 ,0x96 ,0x74 ,0xa6 ,0x64 ,\
                                0x44 ,0xed ,0x82 ,0x98 ,0xdf ,0xde ,0x80 ,0xc9 };
u8 st1_kdrbg[4][16];
u8 st1_vdrbg[4][16];
u8 st1_randomBits[13][16];

volatile u32 A_randombit_tick;
volatile u32 A_backtrack_tick;

/**
 * @brief       This is main function
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ int sample_test1 (void)   //must on ramcode
{
    //h9() instantiation
    drbg_instantiation_func_h9(st1_h9_cs_iv, st1_h9_cs_in, st1_h9_cs_pv, kdrbg_global, vdrbg_global);
    //FIRST PROCEDURE
    smemcpy(st1_kdrbg[0], kdrbg_global, 16);
    smemcpy(st1_vdrbg[0], vdrbg_global, 16);
    //Proc. cnt. = 0; Step cnt. = 0; Transaction ID = 0; Transaction cnt. = 0
    u32 start_tick = clock_time();
    drbg_randomBits_func(kdrbg_global, vdrbg_global, 0, 0, 0);
    A_randombit_tick = clock_time() - start_tick;
    smemcpy(st1_randomBits[0], randomBits[0], 16);
    //Proc. cnt. = 0; Step cnt. = 0; Transaction ID = 0; Transaction cnt. = 1
    drbg_randomBits_func(kdrbg_global, vdrbg_global, 0, 0, 1);
    smemcpy(st1_randomBits[1], randomBits[0], 16);
    //Proc. cnt. = 0; Step cnt. = 0; Transaction ID = 0; Transaction cnt. = 2
    drbg_randomBits_func(kdrbg_global, vdrbg_global, 0, 0, 2);
    smemcpy(st1_randomBits[2], randomBits[0], 16);
    //Proc. cnt. = 0; Step cnt. = 10; Transaction ID = 4; Transaction cnt. = 0
    drbg_randomBits_func(kdrbg_global, vdrbg_global, 10, 4, 0);
    smemcpy(st1_randomBits[3], randomBits[4], 16);

    //NEW PROCEDURE
    start_tick = clock_time();
    drbg_backtracking_resistance(kdrbg_global, vdrbg_global);
    A_backtrack_tick = clock_time() - start_tick;
    smemcpy(st1_kdrbg[1], kdrbg_global, 16);
    smemcpy(st1_vdrbg[1], vdrbg_global, 16);
    //Proc. cnt. = 1; Step cnt. = 0; Transaction ID = 0; Transaction cnt. = 0
    drbg_randomBits_func(kdrbg_global, vdrbg_global, 0, 0, 0);
    smemcpy(st1_randomBits[4], randomBits[0], 16);
    //Proc. cnt. = 1; Step cnt. = 0; Transaction ID = 0; Transaction cnt. = 1
    drbg_randomBits_func(kdrbg_global, vdrbg_global, 0, 0, 1);
    smemcpy(st1_randomBits[5], randomBits[0], 16);
    //Proc. cnt. = 1; Step cnt. = 0; Transaction ID = 0; Transaction cnt. = 2
    drbg_randomBits_func(kdrbg_global, vdrbg_global, 0, 0, 2);
    smemcpy(st1_randomBits[6], randomBits[0], 16);
    //Proc. cnt. = 1; Step cnt. = 14; Transaction ID = 4; Transaction cnt. = 0
    drbg_randomBits_func(kdrbg_global, vdrbg_global, 14, 4, 0);
    smemcpy(st1_randomBits[7], randomBits[4], 16);

    //NEW PROCEDURE
    drbg_backtracking_resistance(kdrbg_global, vdrbg_global);
    smemcpy(st1_kdrbg[2], kdrbg_global, 16);
    smemcpy(st1_vdrbg[2], vdrbg_global, 16);
    //Proc. cnt. = 2; Step cnt. = 0; Transaction ID = 0; Transaction cnt. = 0
    drbg_randomBits_func(kdrbg_global, vdrbg_global, 0, 0, 0);
    smemcpy(st1_randomBits[8], randomBits[0], 16);
    //Proc. cnt. = 2; Step cnt. = 1; Transaction ID = 0; Transaction cnt. = 0
    drbg_randomBits_func(kdrbg_global, vdrbg_global, 1, 0, 0);
    smemcpy(st1_randomBits[9], randomBits[0], 16);
    //Proc. cnt. = 2; Step cnt. = 2; Transaction ID = 0; Transaction cnt. = 0
    drbg_randomBits_func(kdrbg_global, vdrbg_global, 2, 0, 0);
    smemcpy(st1_randomBits[10], randomBits[0], 16);
    //Proc. cnt. = 2; Step cnt. = 3; Transaction ID = 0; Transaction cnt. = 0
    drbg_randomBits_func(kdrbg_global, vdrbg_global, 3, 0, 0);
    smemcpy(st1_randomBits[11], randomBits[0], 16);

    //NEW PROCEDURE
    drbg_backtracking_resistance(kdrbg_global, vdrbg_global);
    smemcpy(st1_kdrbg[3], kdrbg_global, 16);
    smemcpy(st1_vdrbg[3], vdrbg_global, 16);
    //Proc. cnt. = 3; Step cnt. = 0; Transaction ID = 0; Transaction cnt. = 0
    drbg_randomBits_func(kdrbg_global, vdrbg_global, 0, 0, 0);
    smemcpy(st1_randomBits[12], randomBits[0], 16);
    return 0;
}
#endif
