/********************************************************************************************************
 * @file    drbg_stack.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
extern u8 randomBits[10][16];
extern u8 randomBits_num[10];
extern u8 kdrbg_global[16];
extern u8 vdrbg_global[16];
extern u16 step_cnt_global;
extern u8 transaction_id_global;
extern u8 transaction_cnt_global[10];

/**
 * @brief       This function is DRBG instantiation function h9.
 * @param[in]   cs_iv: 128-bit. the result of the CS Security Start procedure
 *                  cs_in: 64-bit. the result of the CS Security Start procedure
 *                  cs_pv: 128-bit. the result of the CS Security Start procedure
 *                  kdrbg: the result of this function.128-bit temporal key
 *                  vdrbg: the result of this function.128-bit nonce vector
 * @return      result - 0:success 1:fail
 */
unsigned char drbg_instantiation_func_h9(u8* cs_iv, u8* cs_in, u8* cs_pv, u8* kdrbg, u8* vdrbg);

/**
 * @brief       This function is to generate 128 random bits
 * @param[in]   kdrbg: 128-bit temporal key
 *                  vdrbg: 128-bit nonce vector
 *                  step_cnt: CS step counter
 *                  transaction_id: transaction ID
 *                  transaction_cnt: transaction counter
 * @return      result - 0:success 1:fail
 */
_attribute_ram_code_
unsigned char drbg_randomBits_func(u8* kdrbg, u8* vdrbg, u16 step_cnt, u8 transaction_id, u8 transaction_cnt);

/**
 * @brief       This function is  DRBG backtracking resistance.it shall be invoked to update the KDRBG and VDRBG
 *              every time the CSProcCount is incremented.
 * @param[in]   kdrbg: 128-bit temporal key
 *                  vdrbg: 128-bit nonce vector
 * @return      result - 0:success 1:fail
 */
_attribute_ram_code_
unsigned char drbg_backtracking_resistance(u8* kdrbg, u8* vdrbg);

/**
 * @brief       This function is to calculate CS Access Address.
 * @param[in]   reflector_accessaddr:  32-bit CS Access Address used in the CS SYNC from the reflector to initiator
                    initiator_accessaddr: 32-bit CS Access Address used in the CS SYNC from the initiator to reflector
 * @return      result - 0:success 1:fail
 */
_attribute_ram_code_
u8 cs_access_addr(u8* reflector_accessaddr, u8* initiator_accessaddr);

/**
 * @brief       This function is to calculate the number of Main_Mode steps to execute.
 * @param[in]   main_mode_max:  the maximum number of Main_Mode steps that shall occur before the
 *                  occurrence of a single a Sub_Mode step.
 *                  main_mode_min: the minimum number of Main_Mode steps that shall occur before the
 *                  occurrence of a single Sub_Mode step.
 * @return      the number of Main_Mode steps
 */
u8 cs_sub_mode_insertion(u8 main_mode_max, u8 main_mode_min);

/**
 * @brief       This function is to generate the random sequence.
 * @param[in]   seq: the random sequence
 *                  seqbit_len:  the length of random sequence
 * @return      result - 0:success 1:fail
 */
u8 cs_random_seq(u8* seq, u8 seqbit_len);
/**
 * @brief       This function is to calculate the antenna path permutation index.
 * @param[in]   na_p: The number of antenna path
 * @return       the antenna path permutation index
 */
u8 cs_antenna_path_perm(u8 na_p);

/**
 * @brief       This function is to calculate the position of the sounding sequence marker.
 * @param[in]   seqbit_len:   length of sounding sequence
 *                  pos_initiator: position of the marker in initiator
 *                  pos_reflector: position of the marker in reflector
 * @return      result - 0:success 1:fail
 */
u8 cs_ss_marker_position(u8 seqbit_len, u8* pos_initiator, u8* pos_reflector);

/**
 * @brief       This function is to calculate the sounding sequence marker signal.
 * @param[in]   sig_initiator: the marker signal in initiator
 *                  sig_reflector:  the marker signal in reflector
 * @return      result - 0:success 1:fail
 */
u8 cs_ss_marker_sig_sel(u8* sig_initiator, u8* sig_reflector);

/**
 * @brief       This function is channel selection Algorithm #3a for mode-0 steps.
 * @param[in]   chn_num:  the length of all available channel indices.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      result - 0:success 1:fail
 */
_attribute_ram_code_
u8 chn_sel_3a(u8 chn_num,u8* s_chn,u8* d_chn);

/**
 * @brief       This function is channel selection Algorithm #3b for non-mode-0 steps.
 * @param[in]   chn_num:  the length of all available channel indices.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      result - 0:success 1:fail
 */
_attribute_ram_code_
u8 chn_sel_3b(u8 chn_num,u8* s_chn,u8* d_chn);

/**
 * @brief       This function should be used when step ascends.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_
void cs_step_add(void);

/**
 * @brief       This function should be used when step need to set up.
 * @param[in]   step: current step
 * @return      none
 */
_attribute_ram_code_
void cs_step_set(u16 step);

/**
 * @brief       This function is to calculate the presence of an actual transmission in CS tone extension slot.
 * @param[in]   tpm_ext: the presence of  CS tone extension slot.
 * @return       result - 0:success 1:fail
 */
_attribute_ram_code_
u8 cs_tpm_ext(u8* tpm_ext);

/**
 * @brief       This function is to convert channel map to channel array.
 * @param[in]   chm: channel map.
 *                  Filtered_channel: channel array
 *                  Filtered_channel_num: length of channel array
 * @return       result - 0:success 1:fail
 */
_attribute_ram_code_
u8 blt_cs_extractEnableChnMap(u8* chm, u8* Filtered_channel);

/**
 * @brief       Channel Selection Algorithm #3c integrates rising and falling ramps into the resulting channel map for
                non-mode-0 CS steps.
 * @param[in]   chm: channel map.
 *                  CSShapeSelection
 *                  CSChannelJump
 *                  CSNumRepetitions
 *                  NonMode0ShuffledChannelArray
 *                  NonMode0ShuffledChannelArrayNum: length of NonMode0ShuffledChannelArray
 * @return       result - 0:success 1:fail
 */
_attribute_ram_code_
u8 chn_sel_3c(u8* chm, u8 CSShapeSelection, u8 CSChannelJump, u8 CSNumRepetitions, u8* NonMode0ShuffledChannelArray, u8* NonMode0ShuffledChannelNum);


/**
 * @brief       This function should be used when new CS starts.
 * @param[in]   none
 * @return      none
 */
void cs_drbg_init(void);
