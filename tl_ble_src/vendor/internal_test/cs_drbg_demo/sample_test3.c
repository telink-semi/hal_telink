#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "stack/ble/controller/cs_drbg/drbg_stack.h"
#if (INTER_TEST_MODE == TEST_CS_DRBG)
//test data
u8 st3_chm[10] = {0xFC,0xFF,0x7F,0xFC,0xFF,0xFF,0xFF,0xFF,0xFF,0x1F};
u8 st3_chm_repetition = 1;
u8 st3_Main_Mode = 0x02;
u8 st3_Sub_Mode = 0x01;
u8 st3_Main_Mode_Min_Steps = 2;
u8 st3_Main_Mode_Max_steps = 6;
u8 st3_Main_Mode_Repetition = 1;
u8 st3_Mode_0_Steps = 3;
u8 st3_RTT_Types = 0x01;
u8 st3_Role = 0b01;
u8 st3_ChSel = 0;

u8 Filtered_channel[72];
u8 Filtered_channel_num = 0;

u8 st3_Procedure_Count = 1;
u16 st3_ACI = 7;

u8 st3_h9_cs_iv[]={0x3b ,0x0b ,0xca ,0xe0 ,0x86 ,0x51 ,0x7f ,0x3e ,\
                                0xe9 ,0xdf ,0xfd ,0x0b ,0x8a ,0xc2 ,0x0b ,0xe1 };
u8 st3_h9_cs_in[]={0x0d ,0x84 ,0x73 ,0x86 ,0xc1 ,0x77 ,0xf4 ,0x9f };
u8 st3_h9_cs_pv[]={0x43 ,0xf1 ,0x68 ,0x78 ,0x96 ,0x74 ,0xa6 ,0x64 ,\
                                0x44 ,0xed ,0x82 ,0x98 ,0xdf ,0xde ,0x80 ,0xc9 };


u8 st3_shufflingchm[3][80];
u8 st3_kdrbg[1][16];
u8 st3_vdrbg[1][16];
u8 st3_i_accessaddr[14][4];
u8 st3_r_accessaddr[14][4];
volatile u8 st3_sub_mode_insertion[8];
u8 st3_tpm_ext[41];
volatile u8 st3_antenna_path_perm[41];

u8 st3_pos_initiator[8][2];
u8 st3_pos_reflector[8][2];
u8 st3_sig_initiator[8][1];
u8 st3_sig_reflector[8][1];
/**
 * @brief       This is main function
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ int sample_test3 (void)   //must on ramcode
{
    smemset(randomBits_num,0,10);
    //h9() instantiation
    drbg_instantiation_func_h9(st3_h9_cs_iv, st3_h9_cs_in, st3_h9_cs_pv, kdrbg_global, vdrbg_global);

    smemcpy(st3_kdrbg[0], kdrbg_global, 16);
    smemcpy(st3_vdrbg[0], vdrbg_global, 16);

    //CS SEQUENCE
    //Channel map

    blt_cs_extractEnableChnMap(st3_chm, Filtered_channel);

    //Shuffled channels 0
    chn_sel_3a(Filtered_channel_num,Filtered_channel,st3_shufflingchm[0]);

    //Access code 0
    cs_access_addr(st3_r_accessaddr[0], st3_i_accessaddr[0]);

    //Access code 1
    cs_step_add();
    cs_access_addr(st3_r_accessaddr[1], st3_i_accessaddr[1]);

    //Access code 2
    cs_step_add();
    cs_access_addr(st3_r_accessaddr[2], st3_i_accessaddr[2]);

    //Shuffled channels 0
    cs_step_add();
    chn_sel_3b(Filtered_channel_num,Filtered_channel,st3_shufflingchm[1]);

    //submode insertion 0
    st3_sub_mode_insertion[0] = cs_sub_mode_insertion(st3_Main_Mode_Max_steps, st3_Main_Mode_Min_Steps);

    //TPM extension 0
    cs_tpm_ext(st3_tpm_ext);

    //Antenna path perm 0
    st3_antenna_path_perm[0] = cs_antenna_path_perm(4);

    //TPM extension 1
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+1);

    //Antenna path perm 1
    st3_antenna_path_perm[1] = cs_antenna_path_perm(4);

    //TPM extension 2
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+2);

    //Antenna path perm 2
    st3_antenna_path_perm[2] = cs_antenna_path_perm(4);

    //TPM extension 3
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+3);

    //Antenna path perm 3
    st3_antenna_path_perm[3] = cs_antenna_path_perm(4);

    //TPM extension 4
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+4);

    //Antenna path perm 4
    st3_antenna_path_perm[4] = cs_antenna_path_perm(4);

    //TPM extension 5
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+5);

    //Antenna path perm 5
    st3_antenna_path_perm[5] = cs_antenna_path_perm(4);

    //TPM extension 6
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+6);

    //Antenna path perm 6
    st3_antenna_path_perm[6] = cs_antenna_path_perm(4);

    //Access code 3
    cs_access_addr(st3_r_accessaddr[3], st3_i_accessaddr[3]);

    //SS marker position 0
    cs_ss_marker_position(32, st3_pos_initiator[0], st3_pos_reflector[0]);

    //SS marker sig sel 0
    cs_ss_marker_sig_sel(st3_sig_initiator[0], st3_sig_reflector[0]);

    //submode insertion 1
    cs_step_add();
    st3_sub_mode_insertion[1] = cs_sub_mode_insertion(st3_Main_Mode_Max_steps, st3_Main_Mode_Min_Steps);

    //TPM extension 7
    cs_tpm_ext(st3_tpm_ext+7);

    //Antenna path perm 7
    st3_antenna_path_perm[7] = cs_antenna_path_perm(4);

    //TPM extension 8
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+8);

    //Antenna path perm 8
    st3_antenna_path_perm[8] = cs_antenna_path_perm(4);

    //TPM extension 9
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+9);

    //Antenna path perm 9
    st3_antenna_path_perm[9] = cs_antenna_path_perm(4);

    //TPM extension 10
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+10);

    //Antenna path perm 10
    st3_antenna_path_perm[10] = cs_antenna_path_perm(4);

    //TPM extension 11
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+11);

    //Antenna path perm 11
    st3_antenna_path_perm[11] = cs_antenna_path_perm(4);

    //Access code 4
    cs_access_addr(st3_r_accessaddr[4], st3_i_accessaddr[4]);

    //SS marker position 1
    cs_ss_marker_position(32, st3_pos_initiator[1], st3_pos_reflector[1]);

    //SS marker sig sel 1
    cs_ss_marker_sig_sel(st3_sig_initiator[1], st3_sig_reflector[1]);

    //submode insertion 2
    cs_step_add();
    st3_sub_mode_insertion[2] = cs_sub_mode_insertion(st3_Main_Mode_Max_steps, st3_Main_Mode_Min_Steps);

    //TPM extension 12
    cs_tpm_ext(st3_tpm_ext+12);

    //Antenna path perm 12
    st3_antenna_path_perm[12] = cs_antenna_path_perm(4);

    //TPM extension 13
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+13);

    //Antenna path perm 13
    st3_antenna_path_perm[13] = cs_antenna_path_perm(4);

    //TPM extension 14
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+14);

    //Antenna path perm 14
    st3_antenna_path_perm[14] = cs_antenna_path_perm(4);

    //Access code 5
    cs_access_addr(st3_r_accessaddr[5], st3_i_accessaddr[5]);

    //SS marker position 2
    cs_ss_marker_position(32, st3_pos_initiator[2], st3_pos_reflector[2]);

    //SS marker sig sel 2
    cs_ss_marker_sig_sel(st3_sig_initiator[2], st3_sig_reflector[2]);

    //submode insertion 3
    cs_step_add();
    st3_sub_mode_insertion[3] = cs_sub_mode_insertion(st3_Main_Mode_Max_steps, st3_Main_Mode_Min_Steps);

    //TPM extension 15
    cs_tpm_ext(st3_tpm_ext+15);

    //Antenna path perm 15
    st3_antenna_path_perm[15] = cs_antenna_path_perm(4);

    //TPM extension 16
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+16);

    //Antenna path perm 16
    st3_antenna_path_perm[16] = cs_antenna_path_perm(4);

    //Access code 6
    cs_step_add();
    cs_access_addr(st3_r_accessaddr[6], st3_i_accessaddr[6]);

    //Access code 7
    cs_step_add();
    cs_access_addr(st3_r_accessaddr[7], st3_i_accessaddr[7]);

    //Access code 8
    cs_step_add();
    cs_access_addr(st3_r_accessaddr[8], st3_i_accessaddr[8]);

    //TPM extension 17
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+17);

    //Antenna path perm 17
    st3_antenna_path_perm[17] = cs_antenna_path_perm(4);

    //TPM extension 18
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+18);

    //Antenna path perm 18
    st3_antenna_path_perm[18] = cs_antenna_path_perm(4);

    //Access code 9
    cs_access_addr(st3_r_accessaddr[9], st3_i_accessaddr[9]);

    //SS marker position 3
    cs_ss_marker_position(32, st3_pos_initiator[3], st3_pos_reflector[3]);

    //SS marker sig sel 3
    cs_ss_marker_sig_sel(st3_sig_initiator[3], st3_sig_reflector[3]);

    //Shuffled channels 2
    cs_step_add();
    chn_sel_3b(Filtered_channel_num,Filtered_channel,st3_shufflingchm[2]);

    //submode insertion 4
    st3_sub_mode_insertion[4] = cs_sub_mode_insertion(st3_Main_Mode_Max_steps, st3_Main_Mode_Min_Steps);

    //TPM extension 19
    cs_tpm_ext(st3_tpm_ext+19);

    //Antenna path perm 19
    st3_antenna_path_perm[19] = cs_antenna_path_perm(4);

    //TPM extension 20
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+20);

    //Antenna path perm 20
    st3_antenna_path_perm[20] = cs_antenna_path_perm(4);

    //TPM extension 21
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+21);

    //Antenna path perm 21
    st3_antenna_path_perm[21] = cs_antenna_path_perm(4);

    //TPM extension 22
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+22);

    //Antenna path perm 22
    st3_antenna_path_perm[22] = cs_antenna_path_perm(4);

    //TPM extension 23
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+23);

    //Antenna path perm 23
    st3_antenna_path_perm[23] = cs_antenna_path_perm(4);

    //TPM extension 24
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+24);

    //Antenna path perm 24
    st3_antenna_path_perm[24] = cs_antenna_path_perm(4);

    //TPM extension 25
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+25);

    //Antenna path perm 25
    st3_antenna_path_perm[25] = cs_antenna_path_perm(4);

    //Access code 10
    cs_access_addr(st3_r_accessaddr[10], st3_i_accessaddr[10]);

    //SS marker position 4
    cs_ss_marker_position(32, st3_pos_initiator[4], st3_pos_reflector[4]);

    //SS marker sig sel 4
    cs_ss_marker_sig_sel(st3_sig_initiator[4], st3_sig_reflector[4]);

    //submode insertion 5
    cs_step_add();
    st3_sub_mode_insertion[5] = cs_sub_mode_insertion(st3_Main_Mode_Max_steps, st3_Main_Mode_Min_Steps);

    //TPM extension 26
    cs_tpm_ext(st3_tpm_ext+26);

    //Antenna path perm 26
    st3_antenna_path_perm[26] = cs_antenna_path_perm(4);

    //TPM extension 27
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+27);

    //Antenna path perm 27
    st3_antenna_path_perm[27] = cs_antenna_path_perm(4);

    //TPM extension 28
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+28);

    //Antenna path perm 28
    st3_antenna_path_perm[28] = cs_antenna_path_perm(4);

    //TPM extension 29
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+29);

    //Antenna path perm 29
    st3_antenna_path_perm[29] = cs_antenna_path_perm(4);

    //TPM extension 30
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+30);

    //Antenna path perm 30
    st3_antenna_path_perm[30] = cs_antenna_path_perm(4);

    //Access code 11
    cs_access_addr(st3_r_accessaddr[11], st3_i_accessaddr[11]);

    //SS marker position 5
    cs_ss_marker_position(32, st3_pos_initiator[5], st3_pos_reflector[5]);

    //SS marker sig sel 5
    cs_ss_marker_sig_sel(st3_sig_initiator[5], st3_sig_reflector[5]);

    //submode insertion 6
    cs_step_add();
    st3_sub_mode_insertion[6] = cs_sub_mode_insertion(st3_Main_Mode_Max_steps, st3_Main_Mode_Min_Steps);

    //TPM extension 31
    cs_tpm_ext(st3_tpm_ext+31);

    //Antenna path perm 31
    st3_antenna_path_perm[31] = cs_antenna_path_perm(4);

    //TPM extension 32
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+32);

    //Antenna path perm 32
    st3_antenna_path_perm[32] = cs_antenna_path_perm(4);

    //TPM extension 33
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+33);

    //Antenna path perm 33
    st3_antenna_path_perm[33] = cs_antenna_path_perm(4);

    //TPM extension 34
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+34);

    //Antenna path perm 34
    st3_antenna_path_perm[34] = cs_antenna_path_perm(4);

    //Access code 12
    cs_access_addr(st3_r_accessaddr[12], st3_i_accessaddr[12]);

    //SS marker position 6
    cs_ss_marker_position(32, st3_pos_initiator[6], st3_pos_reflector[6]);

    //SS marker sig sel 6
    cs_ss_marker_sig_sel(st3_sig_initiator[6], st3_sig_reflector[6]);

    //submode insertion 7
    cs_step_add();
    st3_sub_mode_insertion[7] = cs_sub_mode_insertion(st3_Main_Mode_Max_steps, st3_Main_Mode_Min_Steps);

    //TPM extension 35
    cs_tpm_ext(st3_tpm_ext+35);

    //Antenna path perm 35
    st3_antenna_path_perm[35] = cs_antenna_path_perm(4);

    //TPM extension 36
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+36);

    //Antenna path perm 36
    st3_antenna_path_perm[36] = cs_antenna_path_perm(4);

    //TPM extension 37
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+37);

    //Antenna path perm 37
    st3_antenna_path_perm[37] = cs_antenna_path_perm(4);

    //TPM extension 38
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+38);

    //Antenna path perm 38
    st3_antenna_path_perm[38] = cs_antenna_path_perm(4);

    //TPM extension 39
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+39);

    //Antenna path perm 39
    st3_antenna_path_perm[39] = cs_antenna_path_perm(4);

    //TPM extension 40
    cs_step_add();
    cs_tpm_ext(st3_tpm_ext+40);

    //Antenna path perm 40
    st3_antenna_path_perm[40] = cs_antenna_path_perm(4);

    //Access code 13
    cs_access_addr(st3_r_accessaddr[13], st3_i_accessaddr[13]);

    //SS marker position 7
    cs_ss_marker_position(32, st3_pos_initiator[7], st3_pos_reflector[7]);

    //SS marker sig sel 7
    cs_ss_marker_sig_sel(st3_sig_initiator[7], st3_sig_reflector[7]);
    return 0;
}
#endif
