#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#if (INTER_TEST_MODE == TEST_CS_DRBG)
//test data
u8 st4_chm[10]              = {0xFC, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
u8 st4_chm_repetition       = 2;
u8 st4_Main_Mode            = 0x02;
u8 st4_Sub_Mode             = 0x03;
u8 st4_Main_Mode_Min_Steps  = 2;
u8 st4_Main_Mode_Max_steps  = 6;
u8 st4_Main_Mode_Repetition = 0;
u8 st4_Mode_0_Steps         = 3;
u8 st4_RTT_Types            = 0x01;
u8 st4_Role                 = 0b01;
u8 st4_ChSel                = 0;

u8 st4_Filtered_channel[72];
u8 st4_Filtered_channel_num = 0;

u8  st4_Procedure_Count = 2;
u16 st4_ACI             = 7;

u8 st4_h9_cs_iv[] = {0x3b, 0x0b, 0xca, 0xe0, 0x86, 0x51, 0x7f, 0x3e, 0xe9, 0xdf, 0xfd, 0x0b, 0x8a, 0xc2, 0x0b, 0xe1};
u8 st4_h9_cs_in[] = {0x0d, 0x84, 0x73, 0x86, 0xc1, 0x77, 0xf4, 0x9f};
u8 st4_h9_cs_pv[] = {0x43, 0xf1, 0x68, 0x78, 0x96, 0x74, 0xa6, 0x64, 0x44, 0xed, 0x82, 0x98, 0xdf, 0xde, 0x80, 0xc9};


u8          st4_shufflingchm[3][80];
u8          st4_kdrbg[1][16];
u8          st4_vdrbg[1][16];
u8          st4_i_accessaddr[14][4];
u8          st4_r_accessaddr[14][4];
volatile u8 st4_sub_mode_insertion[8];
u8          st4_tpm_ext[38];
volatile u8 st4_antenna_path_perm[38];

u8 st4_pos_initiator[8][2];
u8 st4_pos_reflector[8][2];
u8 st4_sig_initiator[8][1];
u8 st4_sig_reflector[8][1];

/**
 * @brief       This is main function
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ int sample_test4(void) //must on ramcode
{
    smemset(randomBits_num, 0, 10);
    //h9() instantiation
    drbg_instantiation_func_h9(st4_h9_cs_iv, st4_h9_cs_in, st4_h9_cs_pv, kdrbg_global, vdrbg_global);

    smemcpy(st4_kdrbg[0], kdrbg_global, 16);
    smemcpy(st4_vdrbg[0], vdrbg_global, 16);

    //CS SEQUENCE
    //Channel map

    blt_cs_extractEnableChnMap(st4_chm, st4_Filtered_channel);

    //Shuffled channels 0
    chn_sel_3a(st4_Filtered_channel_num, st4_Filtered_channel, st4_shufflingchm[0]);

    //Access code 0
    cs_access_addr(st4_r_accessaddr[0], st4_i_accessaddr[0]);

    //Access code 1
    cs_step_add(); //1
    cs_access_addr(st4_r_accessaddr[1], st4_i_accessaddr[1]);

    //Access code 2
    cs_step_add(); //2
    cs_access_addr(st4_r_accessaddr[2], st4_i_accessaddr[2]);

    //Shuffled channels 0
    cs_step_add(); //3
    chn_sel_3b(st4_Filtered_channel_num, st4_Filtered_channel, st4_shufflingchm[1]);

    //submode insertion 0

    st4_sub_mode_insertion[0] = cs_sub_mode_insertion(st4_Main_Mode_Max_steps, st4_Main_Mode_Min_Steps); //38us

    //TPM extension 0
    cs_tpm_ext(st4_tpm_ext); //31us

    //Antenna path perm 0
    st4_antenna_path_perm[0] = cs_antenna_path_perm(4);

    cs_step_add(); //4
    //TPM extension 1
    cs_tpm_ext(st4_tpm_ext + 1);

    //Antenna path perm 1
    st4_antenna_path_perm[1] = cs_antenna_path_perm(4);

    cs_step_add(); //5
    //TPM extension 2
    cs_tpm_ext(st4_tpm_ext + 2);

    //Antenna path perm 2
    st4_antenna_path_perm[2] = cs_antenna_path_perm(4);

    cs_step_add(); //6
    //TPM extension 3
    cs_tpm_ext(st4_tpm_ext + 3);

    //Antenna path perm 3
    st4_antenna_path_perm[3] = cs_antenna_path_perm(4);

    cs_step_add(); //7
    //TPM extension 4
    cs_tpm_ext(st4_tpm_ext + 4);

    //Antenna path perm 4
    st4_antenna_path_perm[4] = cs_antenna_path_perm(4);

    cs_step_add(); //8
    //TPM extension 5
    cs_tpm_ext(st4_tpm_ext + 5);

    //Antenna path perm 5
    st4_antenna_path_perm[5] = cs_antenna_path_perm(4);

    cs_step_add(); //9
    //TPM extension 6
    cs_tpm_ext(st4_tpm_ext + 6);

    //Antenna path perm 6
    st4_antenna_path_perm[6] = cs_antenna_path_perm(4);

    //Access code 3
    cs_access_addr(st4_r_accessaddr[3], st4_i_accessaddr[3]);

    //SS marker position 0
    cs_ss_marker_position(32, st4_pos_initiator[0], st4_pos_reflector[0]);

    //SS marker sig sel 0
    cs_ss_marker_sig_sel(st4_sig_initiator[0], st4_sig_reflector[0]);

    cs_step_add(); //10
    //submode insertion 1
    st4_sub_mode_insertion[1] = cs_sub_mode_insertion(st4_Main_Mode_Max_steps, st4_Main_Mode_Min_Steps);

    //TPM extension 7
    cs_tpm_ext(st4_tpm_ext + 7);

    //Antenna path perm 7
    st4_antenna_path_perm[7] = cs_antenna_path_perm(4);

    cs_step_add(); //11
    //TPM extension 8
    cs_tpm_ext(st4_tpm_ext + 8);

    //Antenna path perm 8
    st4_antenna_path_perm[8] = cs_antenna_path_perm(4);

    cs_step_add(); //12
    //TPM extension 9
    cs_tpm_ext(st4_tpm_ext + 9);

    //Antenna path perm 9
    st4_antenna_path_perm[9] = cs_antenna_path_perm(4);

    cs_step_add(); //13
    //TPM extension 10
    cs_tpm_ext(st4_tpm_ext + 10);

    //Antenna path perm 10
    st4_antenna_path_perm[10] = cs_antenna_path_perm(4);

    cs_step_add(); //14
    //TPM extension 11
    cs_tpm_ext(st4_tpm_ext + 11);

    //Antenna path perm 11
    st4_antenna_path_perm[11] = cs_antenna_path_perm(4);

    //Access code 4
    cs_access_addr(st4_r_accessaddr[4], st4_i_accessaddr[4]);

    //SS marker position 1
    cs_ss_marker_position(32, st4_pos_initiator[1], st4_pos_reflector[1]);

    //SS marker sig sel 1
    cs_ss_marker_sig_sel(st4_sig_initiator[1], st4_sig_reflector[1]);

    cs_step_add(); //15
    //submode insertion 2
    st4_sub_mode_insertion[2] = cs_sub_mode_insertion(st4_Main_Mode_Max_steps, st4_Main_Mode_Min_Steps);

    //TPM extension 12
    cs_tpm_ext(st4_tpm_ext + 12);

    //Antenna path perm 12
    st4_antenna_path_perm[12] = cs_antenna_path_perm(4);

    cs_step_add(); //16
    //TPM extension 13
    cs_tpm_ext(st4_tpm_ext + 13);

    //Antenna path perm 13
    st4_antenna_path_perm[13] = cs_antenna_path_perm(4);

    cs_step_add(); //17
    //TPM extension 14
    cs_tpm_ext(st4_tpm_ext + 14);

    //Antenna path perm 14
    st4_antenna_path_perm[14] = cs_antenna_path_perm(4);

    //Access code 5
    cs_access_addr(st4_r_accessaddr[5], st4_i_accessaddr[5]);

    //SS marker position 2
    cs_ss_marker_position(32, st4_pos_initiator[2], st4_pos_reflector[2]);

    //SS marker sig sel 2
    cs_ss_marker_sig_sel(st4_sig_initiator[2], st4_sig_reflector[2]);

    cs_step_add(); //18
    //submode insertion 3
    st4_sub_mode_insertion[3] = cs_sub_mode_insertion(st4_Main_Mode_Max_steps, st4_Main_Mode_Min_Steps);

    //TPM extension 15
    cs_tpm_ext(st4_tpm_ext + 15);

    //Antenna path perm 15
    st4_antenna_path_perm[15] = cs_antenna_path_perm(4);

    cs_step_add(); //19
    //TPM extension 16
    cs_tpm_ext(st4_tpm_ext + 16);

    //Antenna path perm 16
    st4_antenna_path_perm[16] = cs_antenna_path_perm(4);

    cs_step_add(); //20
    //Access code 6
    cs_access_addr(st4_r_accessaddr[6], st4_i_accessaddr[6]);

    cs_step_add(); //21
    //Access code 7
    cs_access_addr(st4_r_accessaddr[7], st4_i_accessaddr[7]);

    cs_step_add(); //22
    //Access code 8
    cs_access_addr(st4_r_accessaddr[8], st4_i_accessaddr[8]);

    cs_step_add(); //23
    //TPM extension 17
    cs_tpm_ext(st4_tpm_ext + 17);

    //Antenna path perm 17
    st4_antenna_path_perm[17] = cs_antenna_path_perm(4);

    cs_step_add(); //24
    //TPM extension 18
    cs_tpm_ext(st4_tpm_ext + 18);

    //Antenna path perm 18
    st4_antenna_path_perm[18] = cs_antenna_path_perm(4);

    //Access code 9
    cs_access_addr(st4_r_accessaddr[9], st4_i_accessaddr[9]);

    //SS marker position 3
    cs_ss_marker_position(32, st4_pos_initiator[3], st4_pos_reflector[3]);

    //SS marker sig sel 3
    cs_ss_marker_sig_sel(st4_sig_initiator[3], st4_sig_reflector[3]);

    cs_step_add(); //25
    chn_sel_3b(st4_Filtered_channel_num, st4_Filtered_channel, st4_shufflingchm[2]);

    //submode insertion 4
    st4_sub_mode_insertion[4] = cs_sub_mode_insertion(st4_Main_Mode_Max_steps, st4_Main_Mode_Min_Steps);

    //TPM extension 19
    cs_tpm_ext(st4_tpm_ext + 19);

    //Antenna path perm 19
    st4_antenna_path_perm[19] = cs_antenna_path_perm(4);
}

#endif
