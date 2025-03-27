#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "stack/ble/controller/cs_drbg/drbg_stack.h"
#if (INTER_TEST_MODE == TEST_CS_DRBG)
//test data
u8 st6_chm[10] = {0xFC,0xFF,0x7F,0xFC,0xFF,0xFF,0xFF,0xFF,0xFF,0x1F};
u8 st6_chm_repetition = 1;
u8 st6_Main_Mode = 0x02;
u8 st6_Sub_Mode = 0x01;
u8 st6_Main_Mode_Min_Steps = 2;
u8 st6_Main_Mode_Max_steps = 4;
u8 st6_Main_Mode_Repetition = 3;
u8 st6_Mode_0_Steps = 3;
u8 st6_RTT_Types = 0x01;
u8 st6_Role = 0b01;
u8 st6_ChSel = 0;

u8 st6_Filtered_channel[72];
u8 st6_Filtered_channel_num = 0;

u8 st6_Procedure_Count = 1;
u16 st6_ACI = 7;

u8 st6_h9_cs_iv[]={0x3b ,0x0b ,0xca ,0xe0 ,0x86 ,0x51 ,0x7f ,0x3e ,\
                                0xe9 ,0xdf ,0xfd ,0x0b ,0x8a ,0xc2 ,0x0b ,0xe1 };
u8 st6_h9_cs_in[]={0x0d ,0x84 ,0x73 ,0x86 ,0xc1 ,0x77 ,0xf4 ,0x9f };
u8 st6_h9_cs_pv[]={0x43 ,0xf1 ,0x68 ,0x78 ,0x96 ,0x74 ,0xa6 ,0x64 ,\
                                0x44 ,0xed ,0x82 ,0x98 ,0xdf ,0xde ,0x80 ,0xc9 };

u8 st6_shufflingchm[3][80];
u8 st6_kdrbg[1][16];
u8 st6_vdrbg[1][16];
u8 st6_i_accessaddr[14][4];
u8 st6_r_accessaddr[14][4];
volatile u8 st6_sub_mode_insertion[29];
u8 st6_tpm_ext[38];
volatile u8 st6_antenna_path_perm[38];

u8 st6_pos_initiator[8][2];
u8 st6_pos_reflector[8][2];
u8 st6_sig_initiator[8][1];
u8 st6_sig_reflector[8][1];



u8 st6_NonMode0ShuffledChannelArray[160];
u8 st6_NonMode0ShuffledChannelNum;

/**
 * @brief       This is main function
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ int sample_test6 (void)   //must on ramcode
{
    smemset(randomBits_num,0,10);
    //h9() instantiation
    drbg_instantiation_func_h9(st6_h9_cs_iv, st6_h9_cs_in, st6_h9_cs_pv, kdrbg_global, vdrbg_global);

    smemcpy(st6_kdrbg[0], kdrbg_global, 16);
    smemcpy(st6_vdrbg[0], vdrbg_global, 16);

    //CS SEQUENCE
    //Channel map

    blt_cs_extractEnableChnMap(st6_chm, st6_Filtered_channel);

    //Shuffled channels 0
    chn_sel_3a(st6_Filtered_channel_num,st6_Filtered_channel,st6_shufflingchm[0]);


    chn_sel_3c(st6_chm, 0, 2, 1, st6_NonMode0ShuffledChannelArray, &st6_NonMode0ShuffledChannelNum);

    cs_step_add();
    cs_step_add();
    cs_step_add();

    //submode insertion 0
    st6_sub_mode_insertion[0] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 1
    st6_sub_mode_insertion[1] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 2
    st6_sub_mode_insertion[2] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 3
    st6_sub_mode_insertion[3] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 4
    st6_sub_mode_insertion[4] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 5
    st6_sub_mode_insertion[5] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 6
    st6_sub_mode_insertion[6] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 7
    st6_sub_mode_insertion[7] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 8
    st6_sub_mode_insertion[8] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 9
    st6_sub_mode_insertion[9] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 10
    st6_sub_mode_insertion[10] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 11
    st6_sub_mode_insertion[11] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 12
    st6_sub_mode_insertion[12] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 13
    st6_sub_mode_insertion[13] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 14
    st6_sub_mode_insertion[14] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 15
    st6_sub_mode_insertion[15] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    step_cnt_global = 68;
    smemset(transaction_cnt_global, 0, 10);

    //submode insertion 16
    st6_sub_mode_insertion[16] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 17
    st6_sub_mode_insertion[17] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 18
    st6_sub_mode_insertion[18] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 19
    st6_sub_mode_insertion[19] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 20
    st6_sub_mode_insertion[20] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 21
    st6_sub_mode_insertion[21] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 22
    st6_sub_mode_insertion[22] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 23
    st6_sub_mode_insertion[23] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 24
    st6_sub_mode_insertion[24] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 25
    st6_sub_mode_insertion[25] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 26
    st6_sub_mode_insertion[26] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 27
    st6_sub_mode_insertion[27] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);

    //submode insertion 28
    st6_sub_mode_insertion[28] = cs_sub_mode_insertion(st6_Main_Mode_Max_steps, st6_Main_Mode_Min_Steps);
    return 0;
}
#endif
