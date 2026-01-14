/********************************************************************************************************
 * @file    cs_drbg_stack.h
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
#ifndef HARDWARE_DRBG_ENABLE
    #define HARDWARE_DRBG_ENABLE 0
#endif

//Randomization of hop channel set for non-mode-0 steps
#define CSTransactionID_0 0
//Randomization of hop channel set for mode-0 steps
#define CSTransactionID_1 1
//Randomization of subevent sub-mode (into main-mode cycle).
#define CSTransactionID_2 2
//T_PM CS tone extension slot transmission presence
#define CSTransactionID_3 3
//Antenna path permutation index selection
#define CSTransactionID_4 4
//RTT PN sequence CS Access Address generation
#define CSTransactionID_5 5
//Sounding sequence marker position randomization
#define CSTransactionID_6 6
//Sounding sequence marker signal selection
#define CSTransactionID_7 7
//Random sequence generation
#define CSTransactionID_8 8
//Backtracking resistance
#define CSTransactionID_9 9

#define CSTransactionMaxIDNum (CSTransactionID_9 + 1)

#define CS_DRBG_3C_SALTRATE   2

typedef struct __attribute__((packed))
{
    u8 transactionCnt;
    u8 randomBitsNum;
    u8 randomBits[16];
} drbg_id_param_t;

typedef struct __attribute__((packed))
{
    u16             stepCnt;
    u8              kdrbg[16];
    u8              vdrbg[16];
    drbg_id_param_t idParam[CSTransactionMaxIDNum];
} drbg_param_t;

extern drbg_param_t *pDrbg;

/**
 * @brief Sets the global DRBG (Deterministic Random Bit Generator) parameter pointer
 * @param p Pointer to the DRBG parameters structure to be set as the active instance
 * @note When hardware DRBG is enabled (HARDWARE_DRBG_ENABLE), additional hardware-specific handling may be performed
 */
static inline void cs_set_pDrbg(drbg_param_t *p)
{
    pDrbg = p;
#if(HARDWARE_DRBG_ENABLE)
    cs_kdrbg_setup((unsigned int*)pDrbg->kdrbg);
    cs_vdrbg_setup((unsigned int*)pDrbg->vdrbg);
#endif
}

/**
 * @brief Sets the step counter value for the DRBG (Deterministic Random Bit Generator).
 *
 * @param stepCnt The step count value to be set.
 *
 * @note If hardware DRBG is enabled (HARDWARE_DRBG_ENABLE), it also configures
 *       the hardware with the new step count value via cs_step_cnt_setup().
 */
static inline void cs_set_stepCnt(u16 stepCnt)
{
    pDrbg->stepCnt = stepCnt;
#if(HARDWARE_DRBG_ENABLE)
    cs_step_cnt_setup(pDrbg->stepCnt);
#endif
}

/**
 * @brief Increments the step counter of the DRBG (Deterministic Random Bit Generator).
 *
 * If hardware DRBG is enabled, also updates the hardware step counter register.
 * This is an internal helper function for DRBG operations.
 */
static inline void cs_add_stepCnt(void)
{
    pDrbg->stepCnt++;
#if(HARDWARE_DRBG_ENABLE)
    cs_step_cnt_setup(pDrbg->stepCnt);
#endif
}

/**
 * @brief Get the current step count from the DRBG (Deterministic Random Bit Generator).
 *
 * @return u16 The current step count value.
 */
static inline u16 cs_get_stepCnt(void)
{
    return pDrbg->stepCnt;
}

typedef u32 (*chn_sel_3c_callback_t)(u8 *chm, u8 CSShapeSelection, u8 CSChannelJump, u8 CSNumRepetitions, u8 *NonMode0ShuffledChannelArray);
extern chn_sel_3c_callback_t chn_sel_3c_cb;

/**
 * @brief       This function is DRBG instantiation function h9.
 * @param[in]   cs_iv: 128-bit. the result of the CS Security Start procedure
 *                  cs_in: 64-bit. the result of the CS Security Start procedure
 *                  cs_pv: 128-bit. the result of the CS Security Start procedure
 *                  kdrbg: the result of this function.128-bit temporal key
 *                  vdrbg: the result of this function.128-bit nonce vector
 * @return      none
 */
void drbg_instantiation_func_h9(u8 *cs_iv, u8 *cs_in, u8 *cs_pv);

/**
 * @brief       This function is to generate 128 random bits
 * @param[in]   kdrbg: 128-bit temporal key
 *                  vdrbg: 128-bit nonce vector
 *                  step_cnt: CS step counter
 *                  transaction_id: transaction ID
 *                  transaction_cnt: transaction counter
 * @return      none
 */
_attribute_ram_code_ void drbg_randomBits_func(u8 transaction_id);

/**
 * @brief       This function is  DRBG backtracking resistance.it shall be invoked to update the KDRBG and VDRBG
 *              every time the CSProcCount is incremented.
 * @param[in]   kdrbg: 128-bit temporal key
 *                  vdrbg: 128-bit nonce vector
 * @return      none
 */
_attribute_ram_code_ void drbg_backtracking_resistance(void);

/**
 * @brief       This function is to calculate CS Access Address.
 * @param[in]   reflector_accessaddr:  32-bit CS Access Address used in the CS SYNC from the reflector to initiator
                    initiator_accessaddr: 32-bit CS Access Address used in the CS SYNC from the initiator to reflector
 * @return      none
 */
_attribute_ram_code_ void cs_access_addr(u8 *reflector_accessaddr, u8 *initiator_accessaddr);

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
 * @param[in]   seqInit: the initiator random sequence
 *                  seqRefl:the reflector random sequence
 *                  seqbit_len:  the length of random sequence
 * @return      none
 */
void cs_random_seq(u8 *seqInit, u8 *seqRefl, u8 seqbit_len);

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
 *                  sig_initiator: the marker signal in initiator
 *                  sig_reflector:  the marker signal in reflector
 * @return      None.
 */
void cs_ss_marker(u8 seqbit_len, u8 *pos_initiator, u8 *pos_reflector, u8 *sig_initiator, u8 *sig_reflector);

/**
 * @brief       This function is to calculate the position of the sounding sequence marker.
 * @param[in]   seqbit_len:   length of sounding sequence
 *                  pos_initiator: position of the marker in initiator
 *                  pos_reflector: position of the marker in reflector
 * @return      none
 */
void cs_ss_marker_position(u8 seqbit_len, u8 *pos_initiator, u8 *pos_reflector);

/**
 * @brief       This function is to calculate the sounding sequence marker signal according to marker signal number.
 * @param[in]   sig_initiator: the marker signal in initiator
 *                  sig_reflector:  the marker signal in reflector
 * @return      none
 */
void cs_ss_marker_sig_sel(u8 *sig_initiator, u8 *sig_reflector, u8 *pos_initiator, u8 *pos_reflector);

/**
 * @brief       This function is channel selection Algorithm #3a for mode-0 steps.
 * @param[in]   chn_num:  the length of all available channel indices.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      none
 */
_attribute_ram_code_ void chn_sel_3a(u8 chn_num, u8 *s_chn, u8 *d_chn);

/**
 * @brief       This function is channel selection Algorithm #3b for non-mode-0 steps.
 * @param[in]   chn_num:  the length of all available channel indices.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      none
 */
_attribute_ram_code_ void chn_sel_3b(u8 chn_num, u8 *s_chn, u8 *d_chn);


/**
 * @brief       This function is to calculate the presence of an actual transmission in CS tone extension slot.
 * @param[in]   tpm_ext: the presence of  CS tone extension slot.
 * @return       none
 */
_attribute_ram_code_ void cs_tpm_ext(u8 *tpm_ext);

/**
 * @brief       This function is to convert channel map to channel array.
 * @param[in]   chm: channel map.
 *                  Filtered_channel: channel array
 *                  Filtered_channel_num: length of channel array
 * @return       ble_sts_t - 0:success 
 */
_attribute_ram_code_
    ble_sts_t
    blt_cs_extractEnableChnMap(u8 *chm, u8 *Filtered_channel, u8 *Filtered_channel_num);

/**
 * @brief       Channel Selection Algorithm #3c integrates rising and falling ramps into the resulting channel map for
                non-mode-0 CS steps.
 * @param[in]   chm: channel map.
 *                  CSShapeSelection
 *                  CSChannelJump
 *                  CSNumRepetitions
 *                  NonMode0ShuffledChannelArray
 * @return       NonMode0ShuffledChannelArrayNum: length of NonMode0ShuffledChannelArray
 */
_attribute_ram_code_
    u32
    chn_sel_3c(u8 *chm, u8 CSShapeSelection, u8 CSChannelJump, u8 CSNumRepetitions, u8 *NonMode0ShuffledChannelArray);


/**
 * @brief       This function should be used when new CS starts.
 * @param[in]   none
 * @return      none
 */
void cs_drbg_init(void);
