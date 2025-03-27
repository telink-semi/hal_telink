#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "stack/ble/controller/cs_drbg/drbg_stack.h"
#if (INTER_TEST_MODE == TEST_CS_DRBG)
//test data
u8 st2_chm[10] = {0xFC,0xFF,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
u8 st2_chm_repetition = 2;
u8 st2_Main_Mode = 0x02;
u8 st2_Sub_Mode = 0x03;
u8 st2_Main_Mode_Min_Steps = 2;
u8 st2_Main_Mode_Max_steps = 6;
u8 st2_Main_Mode_Repetition = 0;
u8 st2_Mode_0_Steps = 3;
u8 st2_RTT_Types = 0x00;
u8 st2_Role = 0b01;
u8 st2_ChSel = 0;

u8 st2_Filtered_channel[72];
u8 st2_Filtered_channel_num = 0;

u8 st2_Procedure_Count = 1;
u16 st2_ACI = 7;

u8 st2_h9_cs_iv[]={0x3b ,0x0b ,0xca ,0xe0 ,0x86 ,0x51 ,0x7f ,0x3e ,\
                                0xe9 ,0xdf ,0xfd ,0x0b ,0x8a ,0xc2 ,0x0b ,0xe1 };
u8 st2_h9_cs_in[]={0x0d ,0x84 ,0x73 ,0x86 ,0xc1 ,0x77 ,0xf4 ,0x9f };
u8 st2_h9_cs_pv[]={0x43 ,0xf1 ,0x68 ,0x78 ,0x96 ,0x74 ,0xa6 ,0x64 ,\
                                0x44 ,0xed ,0x82 ,0x98 ,0xdf ,0xde ,0x80 ,0xc9 };


u8 st2_shufflingchm[3][80];
u8 st2_kdrbg[1][16];
u8 st2_vdrbg[1][16];
u8 st2_i_accessaddr[14][4];
u8 st2_r_accessaddr[14][4];
volatile u8 st2_sub_mode_insertion[8];
u8 st2_tpm_ext[38];
volatile u8 st2_antenna_path_perm[38];

u8 st2_pos_initiator[8][2];
u8 st2_pos_reflector[8][2];
u8 st2_sig_initiator[8][1];
u8 st2_sig_reflector[8][1];


volatile u32 A_0_tick;
volatile u32 A_1_tick;
volatile u32 A_2_tick;
volatile u32 A_3_tick;
volatile u32 A_4_tick;
volatile u32 A_5_tick;
volatile u32 A_6_tick;
volatile u32 A_7_tick;
volatile u32 A_8_tick;
/**
 * @brief       This is main function
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ int sample_test2 (void)   //must on ramcode
{
    smemset(randomBits_num,0,10);
    //h9() instantiation
    drbg_instantiation_func_h9(st2_h9_cs_iv, st2_h9_cs_in, st2_h9_cs_pv, kdrbg_global, vdrbg_global);

    smemcpy(st2_kdrbg[0], kdrbg_global, 16);
    smemcpy(st2_vdrbg[0], vdrbg_global, 16);

    //CS SEQUENCE
    //Channel map

    blt_cs_extractEnableChnMap(st2_chm, st2_Filtered_channel);

    //Shuffled channels 0
    u32 start_tick = clock_time();
    chn_sel_3a(st2_Filtered_channel_num,st2_Filtered_channel,st2_shufflingchm[0]);//238.5us 20chn //40us 2chn
    A_1_tick = clock_time() - start_tick;

    //Access code 0
    start_tick = clock_time();
    cs_access_addr(st2_r_accessaddr[0], st2_i_accessaddr[0]);//293us
    A_5_tick = clock_time() - start_tick;

    //Access code 1
    cs_step_add();
    cs_access_addr(st2_r_accessaddr[1], st2_i_accessaddr[1]);

    //Access code 2
    cs_step_add();
    cs_access_addr(st2_r_accessaddr[2], st2_i_accessaddr[2]);

    //Shuffled channels 0
    cs_step_add();
    start_tick = clock_time();
    chn_sel_3b(st2_Filtered_channel_num,st2_Filtered_channel,st2_shufflingchm[1]);
    A_0_tick = clock_time() - start_tick;
    //submode insertion 0
    start_tick = clock_time();
    st2_sub_mode_insertion[0] = cs_sub_mode_insertion(st2_Main_Mode_Max_steps, st2_Main_Mode_Min_Steps);//38us
    A_2_tick = clock_time() - start_tick;

    //TPM extension 0
    start_tick = clock_time();
    cs_tpm_ext(st2_tpm_ext);//31us
    A_3_tick = clock_time() - start_tick;

    //Antenna path perm 0
    start_tick = clock_time();
    st2_antenna_path_perm[0] = cs_antenna_path_perm(4);
    A_4_tick = clock_time() - start_tick;

    //TPM extension 1
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+1);

    //Antenna path perm 1
    st2_antenna_path_perm[1] = cs_antenna_path_perm(4);

    //TPM extension 2
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+2);

    //Antenna path perm 2
    st2_antenna_path_perm[2] = cs_antenna_path_perm(4);

    //TPM extension 3
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+3);

    //Antenna path perm 3
    st2_antenna_path_perm[3] = cs_antenna_path_perm(4);

    //TPM extension 4
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+4);

    //Antenna path perm 4
    st2_antenna_path_perm[4] = cs_antenna_path_perm(4);

    //TPM extension 5
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+5);

    //Antenna path perm 5
    st2_antenna_path_perm[5] = cs_antenna_path_perm(4);

    //TPM extension 6
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+6);

    //Antenna path perm 6
    st2_antenna_path_perm[6] = cs_antenna_path_perm(4);

    //Access code 3
    cs_access_addr(st2_r_accessaddr[3], st2_i_accessaddr[3]);

    //SS marker position 0
    start_tick = clock_time();
    cs_ss_marker_position(32, st2_pos_initiator[0], st2_pos_reflector[0]);
    A_6_tick = clock_time() - start_tick;

    //SS marker sig sel 0
    start_tick = clock_time();
    cs_ss_marker_sig_sel(st2_sig_initiator[0], st2_sig_reflector[0]);
    A_7_tick = clock_time() - start_tick;

    //submode insertion 1
    cs_step_add();
    st2_sub_mode_insertion[1] = cs_sub_mode_insertion(st2_Main_Mode_Max_steps, st2_Main_Mode_Min_Steps);

    //TPM extension 7
    cs_tpm_ext(st2_tpm_ext+7);

    //Antenna path perm 7
    st2_antenna_path_perm[7] = cs_antenna_path_perm(4);

    //TPM extension 8
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+8);

    //Antenna path perm 8
    st2_antenna_path_perm[8] = cs_antenna_path_perm(4);

    //TPM extension 9
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+9);

    //Antenna path perm 9
    st2_antenna_path_perm[9] = cs_antenna_path_perm(4);

    //TPM extension 10
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+10);

    //Antenna path perm 10
    st2_antenna_path_perm[10] = cs_antenna_path_perm(4);

    //TPM extension 11
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+11);

    //Antenna path perm 11
    st2_antenna_path_perm[11] = cs_antenna_path_perm(4);

    //Access code 4
    cs_access_addr(st2_r_accessaddr[4], st2_i_accessaddr[4]);

    //SS marker position 1
    cs_ss_marker_position(32, st2_pos_initiator[1], st2_pos_reflector[1]);

    //SS marker sig sel 1
    cs_ss_marker_sig_sel(st2_sig_initiator[1], st2_sig_reflector[1]);

    //submode insertion 2
    cs_step_add();
    st2_sub_mode_insertion[2] = cs_sub_mode_insertion(st2_Main_Mode_Max_steps, st2_Main_Mode_Min_Steps);

    //TPM extension 12
    cs_tpm_ext(st2_tpm_ext+12);

    //Antenna path perm 12
    st2_antenna_path_perm[12] = cs_antenna_path_perm(4);

    //TPM extension 13
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+13);

    //Antenna path perm 13
    st2_antenna_path_perm[13] = cs_antenna_path_perm(4);

    //TPM extension 14
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+14);

    //Antenna path perm 14
    st2_antenna_path_perm[14] = cs_antenna_path_perm(4);

    //Access code 5
    cs_access_addr(st2_r_accessaddr[5], st2_i_accessaddr[5]);

    //SS marker position 2
    cs_ss_marker_position(32, st2_pos_initiator[2], st2_pos_reflector[2]);

    //SS marker sig sel 2
    cs_ss_marker_sig_sel(st2_sig_initiator[2], st2_sig_reflector[2]);

    //submode insertion 3
    cs_step_add();
    st2_sub_mode_insertion[3] = cs_sub_mode_insertion(st2_Main_Mode_Max_steps, st2_Main_Mode_Min_Steps);

    //TPM extension 15
    cs_tpm_ext(st2_tpm_ext+15);

    //Antenna path perm 15
    st2_antenna_path_perm[15] = cs_antenna_path_perm(4);

    //TPM extension 16
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+16);

    //Antenna path perm 16
    st2_antenna_path_perm[16] = cs_antenna_path_perm(4);

    //Access code 6
    cs_step_add();
    cs_access_addr(st2_r_accessaddr[6], st2_i_accessaddr[6]);

    //Access code 7
    cs_step_add();
    cs_access_addr(st2_r_accessaddr[7], st2_i_accessaddr[7]);

    //Access code 8
    cs_step_add();
    cs_access_addr(st2_r_accessaddr[8], st2_i_accessaddr[8]);

    //TPM extension 17
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+17);

    //Antenna path perm 17
    st2_antenna_path_perm[17] = cs_antenna_path_perm(4);

    //TPM extension 18
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+18);

    //Antenna path perm 18
    st2_antenna_path_perm[18] = cs_antenna_path_perm(4);

    //Access code 9
    cs_access_addr(st2_r_accessaddr[9], st2_i_accessaddr[9]);

    //SS marker position 3
    cs_ss_marker_position(32, st2_pos_initiator[3], st2_pos_reflector[3]);

    //SS marker sig sel 3
    cs_ss_marker_sig_sel(st2_sig_initiator[3], st2_sig_reflector[3]);

    //Shuffled channels 2
    cs_step_add();
    chn_sel_3b(st2_Filtered_channel_num,st2_Filtered_channel,st2_shufflingchm[2]);

    //submode insertion 4
    st2_sub_mode_insertion[4] = cs_sub_mode_insertion(st2_Main_Mode_Max_steps, st2_Main_Mode_Min_Steps);

    //TPM extension 19
    cs_tpm_ext(st2_tpm_ext+19);

    //Antenna path perm 19
    st2_antenna_path_perm[19] = cs_antenna_path_perm(4);

    //TPM extension 20
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+20);

    //Antenna path perm 20
    st2_antenna_path_perm[20] = cs_antenna_path_perm(4);

    //TPM extension 21
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+21);

    //Antenna path perm 21
    st2_antenna_path_perm[21] = cs_antenna_path_perm(4);

    //TPM extension 22
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+22);

    //Antenna path perm 22
    st2_antenna_path_perm[22] = cs_antenna_path_perm(4);

    //TPM extension 23
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+23);

    //Antenna path perm 23
    st2_antenna_path_perm[23] = cs_antenna_path_perm(4);

    //TPM extension 24
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+24);

    //Antenna path perm 24
    st2_antenna_path_perm[24] = cs_antenna_path_perm(4);

    //TPM extension 25
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+25);

    //Antenna path perm 25
    st2_antenna_path_perm[25] = cs_antenna_path_perm(4);

    //Access code 10
    cs_access_addr(st2_r_accessaddr[10], st2_i_accessaddr[10]);

    //SS marker position 4
    cs_ss_marker_position(32, st2_pos_initiator[4], st2_pos_reflector[4]);

    //SS marker sig sel 4
    cs_ss_marker_sig_sel(st2_sig_initiator[4], st2_sig_reflector[4]);

    //submode insertion 5
    cs_step_add();
    st2_sub_mode_insertion[5] = cs_sub_mode_insertion(st2_Main_Mode_Max_steps, st2_Main_Mode_Min_Steps);

    //TPM extension 26
    cs_tpm_ext(st2_tpm_ext+26);

    //Antenna path perm 26
    st2_antenna_path_perm[26] = cs_antenna_path_perm(4);

    //TPM extension 27
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+27);

    //Antenna path perm 27
    st2_antenna_path_perm[27] = cs_antenna_path_perm(4);

    //TPM extension 28
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+28);

    //Antenna path perm 28
    st2_antenna_path_perm[28] = cs_antenna_path_perm(4);

    //TPM extension 29
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+29);

    //Antenna path perm 29
    st2_antenna_path_perm[29] = cs_antenna_path_perm(4);

    //TPM extension 30
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+30);

    //Antenna path perm 30
    st2_antenna_path_perm[30] = cs_antenna_path_perm(4);

    //Access code 11
    cs_access_addr(st2_r_accessaddr[11], st2_i_accessaddr[11]);

    //SS marker position 5
    cs_ss_marker_position(32, st2_pos_initiator[5], st2_pos_reflector[5]);

    //SS marker sig sel 5
    cs_ss_marker_sig_sel(st2_sig_initiator[5], st2_sig_reflector[5]);

    //submode insertion 6
    cs_step_add();
    st2_sub_mode_insertion[6] = cs_sub_mode_insertion(st2_Main_Mode_Max_steps, st2_Main_Mode_Min_Steps);

    //TPM extension 31
    cs_tpm_ext(st2_tpm_ext+31);

    //Antenna path perm 31
    st2_antenna_path_perm[31] = cs_antenna_path_perm(4);

    //TPM extension 32
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+32);

    //Antenna path perm 32
    st2_antenna_path_perm[32] = cs_antenna_path_perm(4);

    //TPM extension 33
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+33);

    //Antenna path perm 33
    st2_antenna_path_perm[33] = cs_antenna_path_perm(4);

    //TPM extension 34
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+34);

    //Antenna path perm 34
    st2_antenna_path_perm[34] = cs_antenna_path_perm(4);

    //Access code 12
    cs_access_addr(st2_r_accessaddr[12], st2_i_accessaddr[12]);

    //SS marker position 6
    cs_ss_marker_position(32, st2_pos_initiator[6], st2_pos_reflector[6]);

    //SS marker sig sel 6
    cs_ss_marker_sig_sel(st2_sig_initiator[6], st2_sig_reflector[6]);

    //submode insertion 7
    cs_step_add();
    st2_sub_mode_insertion[7] = cs_sub_mode_insertion(st2_Main_Mode_Max_steps, st2_Main_Mode_Min_Steps);

    //TPM extension 35
    cs_tpm_ext(st2_tpm_ext+35);

    //Antenna path perm 35
    st2_antenna_path_perm[35] = cs_antenna_path_perm(4);

    //TPM extension 36
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+36);

    //Antenna path perm 36
    st2_antenna_path_perm[36] = cs_antenna_path_perm(4);

    //TPM extension 37
    cs_step_add();
    cs_tpm_ext(st2_tpm_ext+37);

    //Antenna path perm 37
    st2_antenna_path_perm[37] = cs_antenna_path_perm(4);

    //Access code 13
    cs_access_addr(st2_r_accessaddr[13], st2_i_accessaddr[13]);

    //SS marker position 7
    cs_ss_marker_position(32, st2_pos_initiator[7], st2_pos_reflector[7]);

    //SS marker sig sel 7
    cs_ss_marker_sig_sel(st2_sig_initiator[7], st2_sig_reflector[7]);
    return 0;
}
#endif
