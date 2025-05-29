#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"


#define CS_DRBG_LOG(fmt, ...) tlkapi_send_string_u32s(0, "[CS][DRBG]" fmt "\n", ##__VA_ARGS__)

drbg_param_t *drbg = NULL;

_attribute_ble_data_retention_ chn_sel_3c_callback_t chn_sel_3c_cb = NULL;

#if(!HARDWARE_DRBG_ENABLE)

static u8 nShapeIteration = 0;
/**
 * @brief       This function is to add a 32-bit array to other 32-bit array.
 * @param[in]   augend_data:the augend.
 *                  augend_data_len: size of augend_data
 *                  add_data: the addend.
 *                  add_data_len: size of add_data
 * @return      none
 */
_attribute_ram_code_ void multi_u32_add(u32 *augend_data, u32 augend_data_len, u32 *add_data, u32 add_data_len)
{
    u8  carry = 0;
    u32 i;
    for (i = 0; i < augend_data_len; i++) {
        u32 sum;
        if (i < add_data_len) {
            sum = augend_data[i] + add_data[i] + carry;
        } else {
            sum = augend_data[i] + carry;
        }
        if (sum < augend_data[i]) {
            carry = 1;
        } else if (sum != augend_data[i] || !carry) {
            carry = 0;
        }
        augend_data[i] = sum;
    }
}

/**
 * @brief       This function is DRBG chain function f7.
 * @param[in]   key:128-bit.
 *                  input_string: n*128-bit
 *                  string_len: size of input_string.
 *                  add_data_len: size of add_data
 *                  out: the result of this function. 128-bit
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
void drbg_chain_func_f7(u8 *key, u8 *input_string, int string_len, u8 *out)
{
    //if(string_len<0||string_len%16) return 1;

    u8 hout[16] = {0};
    while (string_len > 0) {
        u8 block_start = string_len - 16;
        for (int i = 0; i < 16; i++) {
            hout[i] = hout[i] ^ input_string[block_start + i];
        }
        aes_encryption_le(key, hout, hout);
        string_len -= 16;
    }
    smemcpy(out, hout, 16);
    //return 0;
}

/**
 * @brief       This function is DRBG derivation function f8.
 * @param[in]   input_string: 320-bit
 *                  sm: the result of this function. 256-bit seed material
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
void drbg_derivation_func_f8(u32 *input_string, u8 *sm)
{
    u32 s[20] = {0, 0, 0, 0x80000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x20, 0x28, 0, 0, 0, 0}; //Most 4 byte is V. V+S is 20 byte
    smemcpy((u8 *)(s + 4), (u8 *)input_string, 40);
    u8 k[16] = {0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00};
    u8 k2[16];
    u8 x[16];
    drbg_chain_func_f7(k, (u8 *)s, 20 * 4, k2);
    s[19] = 1;
    drbg_chain_func_f7(k, (u8 *)s, 20 * 4, x);
    aes_encryption_le(k2, x, (u8 *)(sm + 16));
    aes_encryption_le(k2, (u8 *)(sm + 16), (u8 *)(sm));
    //return 0;
}

/**
 * @brief       This function is DRBG update function f9.
 * @param[in]   sm: 256-bit seed material
 *                  key: 128-bit temporal key
 *                  nonce_vector: 128-bit nonce vector
 *                  kout: the result of this function.updated temporal key
 *                  vout: the result of this function.updated nonce vector
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    drbg_update_func_f9(u32 *sm, u8 *key, u32 *nonce_vector, u8 *kout, u8 *vout)
{
    u32 x[8];
    u32 a[1] = {1};
    multi_u32_add(nonce_vector, 4, a, 1);
    aes_encryption_le(key, (u8 *)nonce_vector, (u8 *)(x + 4));
    multi_u32_add(nonce_vector, 4, a, 1);
    aes_encryption_le(key, (u8 *)nonce_vector, (u8 *)x);
    for (int i = 0; i < 8; i++) {
        x[i] = x[i] ^ sm[i];
    }
    smemcpy(vout, (u8 *)x, 16);
    smemcpy(kout, (u8 *)(x + 4), 16);
}

/**
 * @brief       This function is DRBG instantiation function h9.
 * @param[in]   cs_iv: 128-bit. the result of the CS Security Start procedure
 *                  cs_in: 64-bit. the result of the CS Security Start procedure
 *                  cs_pv: 128-bit. the result of the CS Security Start procedure
 *                  kdrbg: the result of this function.128-bit temporal key
 *                  vdrbg: the result of this function.128-bit nonce vector
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
void drbg_instantiation_func_h9(u8 *cs_iv, u8 *cs_in, u8 *cs_pv, u8 *kdrbg, u8 *vdrbg)
{
    CS_DRBG_LOG("h9");
    drbg->stepCnt = 0;

    u8 sm[32];
    u8 cs_pack[40];
    u8 k[16] = {0};
    u8 v[16] = {0};
    smemcpy(cs_pack, cs_pv, 16);
    smemcpy(cs_pack + 16, cs_in, 8);
    smemcpy(cs_pack + 24, cs_iv, 16);
    drbg_derivation_func_f8((u32 *)cs_pack, sm);
    drbg_update_func_f9((u32 *)sm, k, (u32 *)v, kdrbg, vdrbg);
    //return 0;
}

/**
 * @brief       This function is to generate 128 random bits
 * @param[in]   kdrbg: 128-bit temporal key
 *                  vdrbg: 128-bit nonce vector
 *                  transaction_id: transaction ID
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    drbg_randomBits_func(u8 transaction_id, u8 *kdrbg, u8 *vdrbg)
{
    u8               vdrbg_temp[16];
    drbg_id_param_t *p = (drbg_id_param_t *)(&drbg->idParam[transaction_id]);

    smemcpy(vdrbg_temp, vdrbg, 16);
    vdrbg_temp[0] += p->transactionCnt;
    vdrbg_temp[1] += transaction_id;
    (*((u16 *)(vdrbg_temp + 2))) += drbg->stepCnt;
    aes_encryption_le(kdrbg, vdrbg_temp, &p->randomBits[0]);
    p->randomBitsNum = 128;
    //For each step, only transaction id=0 or 1 may fetch more than 128 bits of data at a time.
    if ((transaction_id == CSTransactionID_0) || (transaction_id == CSTransactionID_1) || (transaction_id == CSTransactionID_8)) {
        p->transactionCnt++;
    }
    //return 0;
}

/**
 * @brief       This function is  DRBG backtracking resistance.it shall be invoked to update the KDRBG and VDRBG
 *              every time the CSProcCount is incremented.
 * @param[in]   kdrbg: 128-bit temporal key
 *                  vdrbg: 128-bit nonce vector
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    drbg_backtracking_resistance(u8 *kdrbg, u8 *vdrbg)
{
    CS_DRBG_LOG("backtracking resistance");
    u32 sm[8] = {0};
    vdrbg[1] += CSTransactionID_9;
    drbg_update_func_f9(sm, kdrbg, (u32 *)vdrbg, kdrbg, vdrbg);
    //return 0;
}

/**
 * @brief       This function is random bit generation function CS_DRBG
 * @param[in]   bit_num: number of required bits
 *                  cs_drbg_num: the return value of CS_DRBG
 *                  transaction_id: transaction ID
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    cs_drbg(u8 transaction_id, u8 bit_num, u8 *cs_drbg_num)
{
    //if(bit_num > 128) return 0;//check
    drbg_id_param_t *p = (drbg_id_param_t *)(&drbg->idParam[transaction_id]);
    CS_DRBG_LOG("drbg_bit: step, num, t_ID, t_cnt:", drbg->stepCnt, bit_num, transaction_id, p->transactionCnt);
    if (bit_num > p->randomBitsNum) {
        drbg_randomBits_func(transaction_id, &drbg->kdrbg[0], &drbg->vdrbg[0]); //27us (32M) 17us (64M)
    }
    for (u32 i = bit_num - 1;; i--) {
        u8 num = (p->randomBitsNum - 1);
        if (BIT_IS_SET(p->randomBits[num >> 3], (num % 8))) {
            BIT_SET(cs_drbg_num[i >> 3], i % 8);
        } else {
            BIT_CLR(cs_drbg_num[i >> 3], i % 8);
        }
        p->randomBitsNum--;
        if (i == 0) {
            break;
        }
    }

    //return 0;
}

/**
 * @brief       This function optimizes CS_DRBG. This function can be used when the number of bits is a multiple of 8.
 * @param[in]   byte_num: number of required bytes
 *                  cs_drbg_num: the return value of CS_DRBG
 *                  transaction_id: transaction ID
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    cs_drbg_byte(u8 transaction_id, u8 byte_num, u8 *cs_drbg_num)
{
    //if(byte_num > 16) return 0;//check
    drbg_id_param_t *p = (drbg_id_param_t *)(&drbg->idParam[transaction_id]);
    CS_DRBG_LOG("step %d, get %d bytes from t_ID %d t_cnt %d", drbg->stepCnt, byte_num, transaction_id, p->transactionCnt);
    if (byte_num * 8 > p->randomBitsNum) {
        drbg_randomBits_func(transaction_id, &drbg->kdrbg[0], &drbg->vdrbg[0]); //27us
    }
    for (u32 i = byte_num - 1;; i--) {
        cs_drbg_num[i] = p->randomBits[(p->randomBitsNum - 1) >> 3];
        p->randomBitsNum -= 8;
        if (i == 0) {
            break;
        }
    }
    //return 0;
}

/**
 * @brief       This function is Channel Sounding random number generation function hr1.
 * @param[in]   r: The input to hr1 is an 8-bit unsigned integer, representing the arbitrary range 0 to r-1 from
                        which a random number is to be generated.
 * @return      result - random number
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ u8
    drbg_randnumgen_func_hr1(u8 transaction_id, u8 r)
{
    if (r == 1) {
        return 0;
    }
    u8 cs_drbg_n;
    u8 rout;
    cs_drbg_byte(transaction_id, 1, &cs_drbg_n);
    u32 Trand = r * cs_drbg_n;
    if ((Trand & 0xFF) < (256 % r)) {
        u32 cs_drbg_n2 = 0;
        cs_drbg_byte(transaction_id, 1, (u8 *)&cs_drbg_n2);
        rout = (((cs_drbg_n2 * 256) * r) + Trand) / 65536;
    } else {
        rout = Trand / 256;
    }
    return rout;
}

/**
 * @brief       This function is channel index shuffling function cr1.
 * @param[in]   chn_num:  the length of all available channel indices.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    chn_index_shuffling_func_cr1(u8 transaction_id, u8 chn_num, u8 *s_chn, u8 *d_chn)
{
    for (u32 i = 0; i < chn_num; i++) {
        u8 j = drbg_randnumgen_func_hr1(transaction_id, i + 1);
        if (i != j) {
            d_chn[i] = d_chn[j];
        }
        d_chn[j] = s_chn[i];
    }
    //return 0;
}

/**
 * @brief       This function is channel selection Algorithm #3a for mode-0 steps.
 * @param[in]   chn_num:  the length of all available channel indices.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    chn_sel_3a(u8 chn_num, u8 *s_chn, u8 *d_chn)
{
    CS_DRBG_LOG("chn sel 3a");
    chn_index_shuffling_func_cr1(CSTransactionID_1, chn_num, s_chn, d_chn);
    drbg->idParam[CSTransactionID_1].transactionCnt = 0;
    //return 0;
}

/**
 * @brief       This function is channel selection Algorithm #3b for non-mode-0 steps.
 * @param[in]   chn_num:  the length of all available channel indices.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    chn_sel_3b(u8 chn_num, u8 *s_chn, u8 *d_chn)
{
    CS_DRBG_LOG("chn sel 3b");
    chn_index_shuffling_func_cr1(CSTransactionID_0, chn_num, s_chn, d_chn);
    drbg->idParam[CSTransactionID_0].transactionCnt = 0;
    //return 0;
}

/**
 * @brief       This function is to calculate the sum of XOR values for the i-th bit and the (i+k)-th bit.
 * @param[in]   num:  input 32-bit number
                    k: all available channel indices
 * @return      result - the sum of XOR values
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ s8
    cs_autocorr_ck(u32 num, u32 k)
{
    s8 c = 0;
    for (u32 i = 0; i < 32 - k; i++) {
        c = c + (((num >> i) & 0x01) ^ ((num >> (i + k)) & 0x01));
    }
    return c;
}

/**
 * @brief       This function is to calculate CS autocorrelation score.
 * @param[in]   si:  sequence.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      score - autocorrelation score
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ s8
    cs_autocorr_score(u32 si)
{
    s8 score;
    s8 c1 = cs_autocorr_ck(si, 1) * 2 - 31;
    s8 c2 = cs_autocorr_ck(si, 2) * 2 - 30;
    s8 c3 = cs_autocorr_ck(si, 3) * 2 - 29;
    score = abs(c1) + abs(c2) + abs(c3);
    return score;
}

/**
 * @brief       This function is to calculate CS Access Address.
 * @param[in]   reflector_accessaddr:  32-bit CS Access Address used in the CS SYNC from the reflector to initiator
                    initiator_accessaddr: 32-bit CS Access Address used in the CS SYNC from the initiator to reflector
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    cs_access_addr(u8 *reflector_accessaddr, u8 *initiator_accessaddr)
{
    CS_DRBG_LOG("access addr");
    //role: 0-Reflector,1-Initiator
    u32 cs_bit_string[4];
    cs_drbg_byte(CSTransactionID_5, 16, (u8 *)cs_bit_string);
    s8 s0_score = cs_autocorr_score(cs_bit_string[0]);
    s8 s1_score = cs_autocorr_score(cs_bit_string[1]);
    if (s0_score > s1_score) {
        smemcpy(reflector_accessaddr, (u8 *)(cs_bit_string + 1), 4);
    } else {
        smemcpy(reflector_accessaddr, (u8 *)(cs_bit_string), 4);
    }
    s8 s2_score = cs_autocorr_score(cs_bit_string[2]);
    s8 s3_score = cs_autocorr_score(cs_bit_string[3]);
    if (s2_score > s3_score) {
        smemcpy(initiator_accessaddr, (u8 *)(cs_bit_string + 3), 4);
    } else {
        smemcpy(initiator_accessaddr, (u8 *)(cs_bit_string + 2), 4);
    }
    //return 0;
}

/**
 * @brief       This function is to calculate the number of Main_Mode steps to execute.
 * @param[in]   main_mode_max:  the maximum number of Main_Mode steps that shall occur before the
 *                  occurrence of a single a Sub_Mode step.
 *                  main_mode_min: the minimum number of Main_Mode steps that shall occur before the
 *                  occurrence of a single Sub_Mode step.
 * @return      the number of Main_Mode steps
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ u8
    cs_sub_mode_insertion(u8 main_mode_max, u8 main_mode_min)
{
    CS_DRBG_LOG("sub mode insertion");
    return drbg_randnumgen_func_hr1(CSTransactionID_2, (main_mode_max - main_mode_min + 1)) + main_mode_min;
}

/**
 * @brief       This function is to calculate the position of the sounding sequence marker.
 * @param[in]   seqbit_len:   length of sounding sequence
 *                  pos_initiator: position of the marker in initiator
 *                  pos_reflector: position of the marker in reflector
 *                  sig_initiator: the marker signal in initiator
 *                  sig_reflector:  the marker signal in reflector
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    cs_ss_marker(u8 seqbit_len, u8 *pos_initiator, u8 *pos_reflector, u8 *sig_initiator, u8 *sig_reflector)
{
    CS_DRBG_LOG("ss marker");
    u8 flag_bit;
    cs_drbg(CSTransactionID_7, 1, (u8 *)&flag_bit);
    *sig_initiator = (flag_bit & 0x01);
    cs_drbg(CSTransactionID_7, 1, (u8 *)&flag_bit);

    if (seqbit_len == 32) {
        *sig_reflector   = (flag_bit & 0x01);
        *(pos_initiator) = drbg_randnumgen_func_hr1(CSTransactionID_6, 29);
        *(pos_reflector) = drbg_randnumgen_func_hr1(CSTransactionID_6, 29);
    } else {
        // LL/CS/CEN/INI/BV-13-C next four hr1 code can't change order -- yuexin
        *(pos_initiator)     = drbg_randnumgen_func_hr1(CSTransactionID_6, 64);
        *(pos_initiator + 1) = drbg_randnumgen_func_hr1(CSTransactionID_6, 75) + 67;
        if (*(pos_initiator + 1) > 92) {
            *(pos_initiator + 1) = 0xFF;
        } else {
            *(sig_initiator + 1) = (flag_bit & 0x01);
            cs_drbg(CSTransactionID_7, 1, (u8 *)&flag_bit);
            *sig_reflector = (flag_bit & 0x01);
        }
        *(pos_reflector)     = drbg_randnumgen_func_hr1(CSTransactionID_6, 64);
        *(pos_reflector + 1) = drbg_randnumgen_func_hr1(CSTransactionID_6, 75) + 67;
        if (*(pos_reflector + 1) > 92) {
            *(pos_reflector + 1) = 0xFF;
        } else {
            cs_drbg(CSTransactionID_7, 1, (u8 *)&flag_bit);
            *(sig_reflector + 1) = (flag_bit & 0x01);
        }
    }
}

/**
 * @brief       This function is to calculate the position of the sounding sequence marker.
 * @param[in]   seqbit_len:   length of sounding sequence
 *                  pos_initiator: position of the marker in initiator
 *                  pos_reflector: position of the marker in reflector
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    cs_ss_marker_position(u8 seqbit_len, u8 *pos_initiator, u8 *pos_reflector)
{
    CS_DRBG_LOG("ss marker position");
    //  if(seqbit_len!=32 && seqbit_len!=96)
    //          return 1;
    if (seqbit_len == 32) {
        *(pos_initiator) = drbg_randnumgen_func_hr1(CSTransactionID_6, 29);
        *(pos_reflector) = drbg_randnumgen_func_hr1(CSTransactionID_6, 29);
    } else {
        // LL/CS/CEN/INI/BV-13-C next four lines code can't change order -- yuexin
        *(pos_initiator)     = drbg_randnumgen_func_hr1(CSTransactionID_6, 64);
        *(pos_initiator + 1) = drbg_randnumgen_func_hr1(CSTransactionID_6, 75) + 67;
        *(pos_reflector)     = drbg_randnumgen_func_hr1(CSTransactionID_6, 64);
        *(pos_reflector + 1) = drbg_randnumgen_func_hr1(CSTransactionID_6, 75) + 67;
        if (*(pos_initiator + 1) > 92) {
            *(pos_initiator + 1) = 0xFF;
        }
        if (*(pos_reflector + 1) > 92) {
            *(pos_reflector + 1) = 0xFF;
        }
    }
    //  return 0;
}

/**
 * @brief       This function is to calculate the sounding sequence marker signal according to marker signal number.
 * @param[in]   sig_initiator: the marker signal in initiator
 *                  sig_reflector:  the marker signal in reflector
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    cs_ss_marker_sig_sel(u8 *sig_initiator, u8 *sig_reflector, u8 *pos_initiator, u8 *pos_reflector)
{
    CS_DRBG_LOG("ss marker sig sel 2");
    u8 flag_bit;
    cs_drbg(CSTransactionID_7, 1, (u8 *)&flag_bit);
    *sig_initiator = (flag_bit & 0x01);
    cs_drbg(CSTransactionID_7, 1, (u8 *)&flag_bit);
    if (*(pos_initiator + 1) != 0xff) {
        *(sig_initiator + 1) = (flag_bit & 0x01);
        cs_drbg(CSTransactionID_7, 1, (u8 *)&flag_bit);
        *sig_reflector = (flag_bit & 0x01);
        if (*(pos_reflector + 1) != 0xff) {
            cs_drbg(CSTransactionID_7, 1, (u8 *)&flag_bit);
            *(sig_reflector + 1) = (flag_bit & 0x01);
        }
    } else {
        *sig_reflector = (flag_bit & 0x01);
        if (*(pos_reflector + 1) != 0xff) {
            cs_drbg(CSTransactionID_7, 1, (u8 *)&flag_bit);
            *(sig_reflector + 1) = (flag_bit & 0x01);
        }
    }
    //return 0;
}

/**
 * @brief       This function is to generate the random sequence.
 * @param[in]   seq: the random sequence
 *                  seqbit_len:  the length of random sequence
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    cs_random_seq(u8 *seqInit, u8 *seqRefl, u8 seqbit_len)
{
    CS_DRBG_LOG("generate random sequence");
    //  if(seqbit_len!=32 && seqbit_len!=64 && seqbit_len!=96 && seqbit_len!=128)
    //      return 1;
    cs_drbg_byte(CSTransactionID_8, seqbit_len >> 3, seqInit);
    cs_drbg_byte(CSTransactionID_8, seqbit_len >> 3, seqRefl);
    drbg->idParam[CSTransactionID_8].transactionCnt = 0;
    //  return 0;
}

/**
 * @brief       This function is to calculate the antenna path permutation index.
 * @param[in]   na_p: The number of antenna path
 * @return       the antenna path permutation index
 */
#if 0
    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_ //if not comment, here will occur "Unsupported reloc 254" error. later need to solve that. todo by qiuwei/jiakai
    #endif
#endif
_attribute_ram_code_ u8 cs_antenna_path_perm(u8 na_p)
{
    //  if(na_p<1||na_p>4)
    //      return 0;
    CS_DRBG_LOG("antenna path perm");
    u8 hr1_in = 1;
    do {
        hr1_in = hr1_in * na_p;
        na_p--;
    } while (na_p);
    return drbg_randnumgen_func_hr1(CSTransactionID_4, hr1_in);
}

/**
 * @brief       This function is to calculate the presence of an actual transmission in CS tone extension slot.
 * @param[in]   tpm_ext: the presence of  CS tone extension slot.
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    cs_tpm_ext(u8 *tpm_ext)
{
    CS_DRBG_LOG("tpm ext");
    cs_drbg(CSTransactionID_3, 2, tpm_ext);
    //return 0;
}

/**
 * @brief       After the SaltedChSeq array is generated, it is then filtered through the filter channel bit map
 *              CSFilteredChM. The resulting filtered channel set is then shuffled in a block-based fashion.
 * @param[in]   chm: channel map.
 *                  SaltedChSeq
 *                  SaltedChSeqNum: length of SaltedChSeq
 *                  NonMode0ShuffledChannelArray
 * @return      NonMode0ShuffledChannelNum: length of NonMode0ShuffledChannelArray
 */
static inline u32 cs_filter_shuffle(u8 *chm, u8 *SaltedChSeq, u32 SaltedChSeqNum, u8 *NonMode0ShuffledChannelArray)
{
    u8  filteredSaltedChSeq[136];
    u32 filteredSaltedChSeqNum = 0;
    for (u32 i = 0; i < SaltedChSeqNum; i++) {
        if (BIT_IS_SET(chm[SaltedChSeq[i] >> 3], SaltedChSeq[i] % 8)) {
            filteredSaltedChSeq[filteredSaltedChSeqNum++] = SaltedChSeq[i];
        }
    }
    u32 NonMode0ShuffledChannelNum = filteredSaltedChSeqNum;
    u32 nStepsInblock              = max(10, filteredSaltedChSeqNum / 4);
    u32 nBlocksToShuffle           = max(1, filteredSaltedChSeqNum / nStepsInblock);

    for (u32 i = 0; i < nBlocksToShuffle; i++) {
        u8 cr1_chn_num;
        if (i < nBlocksToShuffle - 1) {
            cr1_chn_num = nStepsInblock;
            filteredSaltedChSeqNum -= nStepsInblock;
        } else {
            cr1_chn_num = filteredSaltedChSeqNum;
        }
        chn_index_shuffling_func_cr1(CSTransactionID_0, cr1_chn_num, filteredSaltedChSeq + i * nStepsInblock, NonMode0ShuffledChannelArray + i * nStepsInblock);
    }
    return NonMode0ShuffledChannelNum;
}

/**
 * @brief       After salt channels are generated, shape channels are mixed with salt channels.
 * @param[in]   CSShapeSelection
 *                  CSNumRepetitions
 *                  ShapeChSeq
 *                  ShapeChSeqnum: length of ShapeChSeqnum
 *                  FirstAndEndSaltChSeq
 *                  MiddleSaltChSeq
 *                  SaltedChSeq
 *                  SaltedChSeqNum: length of SaltedChSeq
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
static inline u32 cs_salt_chn_insert(u8 CSShapeSelection, u8 CSNumRepetitions, u8 *ShapeChSeq, u8 ShapeChSeqnum, u8 *FirstAndEndSaltChSeq, u8 *MiddleSaltChSeq, u8 *SaltedChSeq)
{
    u8  FirstAndEndSaltChSeqUseNum = 0;
    u8  MiddleSaltChSeqUseNum      = 0;
    u8  ShapeChSeqUseNum           = 0;
    u32 SaltedChSeqNum             = 0;
    if (nShapeIteration == 0) {
        u8 nInitialSalt = drbg_randnumgen_func_hr1(CSTransactionID_0, 10);
        for (u32 i = 0; i < nInitialSalt; i++) {
            if (i % 2) {
                SaltedChSeq[SaltedChSeqNum++] = MiddleSaltChSeq[MiddleSaltChSeqUseNum++];
            } else {
                SaltedChSeq[SaltedChSeqNum++] = FirstAndEndSaltChSeq[FirstAndEndSaltChSeqUseNum++];
            }
        }
    }
    for (u32 i = 0; i < ShapeChSeqnum; i++) {
        if (i % CS_DRBG_3C_SALTRATE == 0) {
            //u8 j = ShapeChSeq[i]/20+1;//Delete this to optimize code, By SunWei 240611
            if (CSShapeSelection) {
                //if(j==1||j==4) SaltedChSeq[(*SaltedChSeqNum)++] = FirstAndEndSaltChSeq[FirstAndEndSaltChSeqUseNum++];
                //else SaltedChSeq[(*SaltedChSeqNum)++] = MiddleSaltChSeq[MiddleSaltChSeqUseNum++];
                if (ShapeChSeq[i] < 20 || ShapeChSeq[i] > 59) {
                    SaltedChSeq[SaltedChSeqNum++] = FirstAndEndSaltChSeq[FirstAndEndSaltChSeqUseNum++];
                } else {
                    SaltedChSeq[SaltedChSeqNum++] = MiddleSaltChSeq[MiddleSaltChSeqUseNum++];
                }
            } else {
                //if(j==1||j==2) SaltedChSeq[(*SaltedChSeqNum)++] = FirstAndEndSaltChSeq[FirstAndEndSaltChSeqUseNum++];
                //else SaltedChSeq[(*SaltedChSeqNum)++] = MiddleSaltChSeq[MiddleSaltChSeqUseNum++];
                if (ShapeChSeq[i] < 40) {
                    SaltedChSeq[SaltedChSeqNum++] = FirstAndEndSaltChSeq[FirstAndEndSaltChSeqUseNum++];
                } else {
                    SaltedChSeq[SaltedChSeqNum++] = MiddleSaltChSeq[MiddleSaltChSeqUseNum++];
                }
            }
        }
        SaltedChSeq[SaltedChSeqNum++] = ShapeChSeq[ShapeChSeqUseNum++];
    }
    if (nShapeIteration == CSNumRepetitions - 1) {
        u8 nFinalSalt = drbg_randnumgen_func_hr1(CSTransactionID_0, 5);
        for (u32 i = 0; i < nFinalSalt; i++) {
            if (i % 2) {
                SaltedChSeq[SaltedChSeqNum++] = MiddleSaltChSeq[MiddleSaltChSeqUseNum++];
            } else {
                SaltedChSeq[SaltedChSeqNum++] = FirstAndEndSaltChSeq[FirstAndEndSaltChSeqUseNum++];
            }
        }
    }
    while (MiddleSaltChSeqUseNum < FirstAndEndSaltChSeqUseNum) {
        SaltedChSeq[SaltedChSeqNum++] = MiddleSaltChSeq[MiddleSaltChSeqUseNum++];
    }
    while (MiddleSaltChSeqUseNum > FirstAndEndSaltChSeqUseNum) {
        SaltedChSeq[SaltedChSeqNum++] = FirstAndEndSaltChSeq[FirstAndEndSaltChSeqUseNum++];
    }
    return SaltedChSeqNum;
}

/**
 * @brief       Salt channels are used to generate a uniform channel distribution in the valid set of CS physical channels
 * @param[in]   CSShapeSelection
 *                  ShapeChSeq
 *                  ShapeChSeqnum: length of ShapeChSeqnum
 *                  FirstAndEndSaltChSeq
 *                  FirstAndEndSaltChSeqNum: length of FirstAndEndSaltChSeq
 *                  MiddleSaltChSeq
 *                  MiddleSaltChSeqNum: length of MiddleSaltChSeq
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
static inline void cs_gen_salt(u8 CSShapeSelection, u8 *ShapeChSeq, u32 ShapeChSeqnum, u8 *FirstAndEndSaltChSeq, u8 *MiddleSaltChSeq)
{
    u8 usedChSeq[80]; //The maximum of the used channels is 79, so 80 is enough. Confirmed by SunWei 20240528.
    smemset(usedChSeq, 0, 80);
    for (u32 i = 0; i < ShapeChSeqnum; i++) {
        usedChSeq[ShapeChSeq[i]] = 1;
    }
    u8 firstAndEndAllChSeq[40];    //The maximum of middleUnusedChSeqNum is 40, so 40 is enough.  Confirmed by SunWei 20240611.
    u8 middleAllChSeq[40];         //The maximum of middleUnusedChSeqNum is 40, so 40 is enough.  Confirmed by SunWei 20240611.
    u8 firstAndEndUnusedChSeq[40]; //The maximum of firstAndEndUnusedChSeqNum is 40, so 40 is enough.  Confirmed by SunWei 20240611.
    u8 middleUnusedChSeq[40];      //The maximum of middleUnusedChSeqNum is 40, so 40 is enough.  Confirmed by SunWei 20240611.
    u8 firstAndEndUnusedChSeqNum = 0;
    u8 middleUnusedChSeqNum      = 0;

    //Move initial to for for optimizing SRAM, by SunWei 240611
    //if CSShapeSelection is 1
    //u8 firstAndEndAllChSeq[40] = {20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59};
    //u8 middleAllChSeq[39] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78};
    //if CSShapeSelection is 0
    //u8 firstAndEndAllChSeq[40] = {20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59};
    //u8 middleAllChSeq[39] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78};
    for (int i = 0; i < 40; i++) {
        firstAndEndAllChSeq[i] = i + 40 - 20 * CSShapeSelection;
        middleAllChSeq[i]      = i + (i / 20) * 40 * CSShapeSelection;
        //The number of firstAndEndAllChSeq in X shape is 40 and in Hat shape is 39.
        //So the number in judgment is 39+CSShapeSelection.By SunWei 240612
        if (i < (39 + CSShapeSelection) && usedChSeq[firstAndEndAllChSeq[i]] == 0) {
            firstAndEndUnusedChSeq[firstAndEndUnusedChSeqNum++] = firstAndEndAllChSeq[i];
        }
        //The number of middleAllChSeq in X shape is 39 and in Hat shape is 40.
        //So the number in judgment is 40-CSShapeSelection.By SunWei 240612
        if (i < (40 - CSShapeSelection) && usedChSeq[middleAllChSeq[i]] == 0) //The number of middleAllChSeq is only 39, so when i is equal to 39, need skip.
        {
            middleUnusedChSeq[middleUnusedChSeqNum++] = middleAllChSeq[i];
        }
    }
    //The number of firstAndEndAllChSeq and middleAllChSeq in X shape is 40 and 39.
    //The number of firstAndEndAllChSeq and middleAllChSeq in Hat shape is 39 and 40.
    //So the number of firstAndEndAllChSeq and middleAllChSeq is 39 + CSShapeSelection and 40 - CSShapeSelection.
    //Besides to numbers of firstAndEndAllChSeq and middleAllChSeq, inserting firstAndEndAllChSeq and middleAllChSeq to FirstAndEndSaltChSeq
    //and MiddleSaltChSeq are same in Hat shape and X shape, so move to outside if-else. By SunWei 240612
    chn_index_shuffling_func_cr1(CSTransactionID_0, 39 + CSShapeSelection, firstAndEndAllChSeq, FirstAndEndSaltChSeq + firstAndEndUnusedChSeqNum);
    chn_index_shuffling_func_cr1(CSTransactionID_0, 40 - CSShapeSelection, middleAllChSeq, MiddleSaltChSeq + middleUnusedChSeqNum);
    //Inserting firstAndEndUnusedChSeq and middleUnusedChSeq to FirstAndEndSaltChSeq and MiddleSaltChSeq are same in Hat shape and X shape, so move to outside if-else. By SunWei 240612
    chn_index_shuffling_func_cr1(CSTransactionID_0, firstAndEndUnusedChSeqNum, firstAndEndUnusedChSeq, FirstAndEndSaltChSeq);
    chn_index_shuffling_func_cr1(CSTransactionID_0, middleUnusedChSeqNum, middleUnusedChSeq, MiddleSaltChSeq);
    //return 0;
}

/**
 * @brief       Each invocation of Channel Selection Algorithm #3c begins with the identification of the channels used to
 *              hold the shape selected by the CSShapeSelection parameter.
 * @param[in]   CSShapeSelection
 *                  CSChannelJump
 *                  ShapeChSeq
 * @return      ShapeChSeqnum: length of ShapeChSeqnum
 */
static inline u32 cs_gen_shape_seq(s8 seq1StartCh, s8 seq2StartCh, u8 CSShapeSelection, u8 CSChannelJump, u8 *ShapeChSeq)
{
    static u8 startJitter = 0;
    //Confirmed by SunWei 240604.
    //If this is the first instance of the non-mode-0 channel map generation procedure with a CS procedure,
    //which shall be the case when nShapeIteration is equal to 0, then the startJitter value is produced as follows:
    if (nShapeIteration == 0) {
        startJitter = drbg_randnumgen_func_hr1(CSTransactionID_0, CSChannelJump);
    }
    u8  offset        = (nShapeIteration + startJitter) % CSChannelJump;
    s8  s1ch          = seq1StartCh + offset;
    s8  s2ch          = seq2StartCh + offset;
    u32 ShapeChSeqnum = 0;
    if (CSShapeSelection) {
        s8 inc;
        if (seq1StartCh < seq2StartCh) {
            inc = CSChannelJump;
        } else {
            inc = -CSChannelJump;
        }

        while ((s1ch >= 0 && s1ch <= 78) || (s2ch >= 0 && s2ch <= 78)) {
            if (s1ch >= 0 && s1ch <= 78) {
                ShapeChSeq[ShapeChSeqnum++] = s1ch;
            }
            if (s2ch >= 0 && s2ch <= 78) {
                ShapeChSeq[ShapeChSeqnum++] = s2ch;
            }
            s1ch += inc;
            s2ch -= inc;
        }
    } else {
        s8 risingCh, fallingCh;
        if (s1ch < s2ch) {
            risingCh  = s1ch;
            fallingCh = s2ch;
        } else {
            risingCh  = s2ch;
            fallingCh = s1ch;
        }
        while (risingCh <= 78) {
            ShapeChSeq[ShapeChSeqnum++] = risingCh;
            risingCh += CSChannelJump;
        }
        while (fallingCh >= 0) {
            //When CSChannelJump is exceed to 3, maybe shape channel will exceed to 79, but the size of used channel array only is 79,
            //unavailable channel need to delete. Confirmed by SunWei 20240604.
            if (fallingCh <= 78) {
                ShapeChSeq[ShapeChSeqnum++] = fallingCh;
            }
            fallingCh -= CSChannelJump;
        }
    }
    return ShapeChSeqnum;
}

/**
 * @brief       Channel Selection Algorithm #3c integrates rising and falling ramps into the resulting channel map for
                non-mode-0 CS steps.
 * @param[in]   chm: channel map.
 *                  CSShapeSelection
 *                  CSChannelJump
 *                  CSNumRepetitions
 *                  NonMode0ShuffledChannelArray
 * @return      NonMode0ShuffledChannelArrayNum: length of NonMode0ShuffledChannelArray
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ u32
    chn_sel_3c(u8 *chm, u8 CSShapeSelection, u8 CSChannelJump, u8 CSNumRepetitions, u8 *NonMode0ShuffledChannelArray)
{
    CS_DRBG_LOG("chn sel 3c");
    s8 seq1StartCh;
    s8 seq2StartCh;
    u8 maxRepsAllowed;
    u8 paramBlock[7][3] = {
        {1,  76, 1},
        {77, 0,  1},
        {78, 0,  2},
        {78, 0,  2},
        {76, 1,  3},
        {74, 1,  3},
        {76, 0,  3}
    };
    seq1StartCh    = (s8)paramBlock[CSChannelJump - 2][0];
    seq2StartCh    = (s8)paramBlock[CSChannelJump - 2][1];
    maxRepsAllowed = paramBlock[CSChannelJump - 2][2];

    if ((CSNumRepetitions > maxRepsAllowed) || (CSNumRepetitions == 0)) {
        CS_DRBG_LOG("CSNumRepetitions abnormal:%d,%d", CSNumRepetitions, maxRepsAllowed);
    }
    u8 ShapeChSeq[80];                                                                                                                                              //The number of the shape channel is related of CSChannelJump, and the minimum of CSChannelJump is 2.
                                                                                                                                                                    //When CSChannelJump is 2, the maximum of channel is 78, so 80 is enough. Confirmed by SunWei 20240611.
    u8 FirstAndEndSaltChSeq[80];                                                                                                                                    //The maximum of FirstAndEndSaltChSeq is 40 + 39, so 80 is enough. Confirmed by SunWei 20240528.
    u8 MiddleSaltChSeq[80];                                                                                                                                         //The maximum of FirstAndEndSaltChSeq is 40 + 39, so 80 is enough. Confirmed by SunWei 20240528.
    u8 SaltedChSeq[136];                                                                                                                                            //SaltedChSeqNum =  nInitialSalt(max:9) + ShapeChSeqnum(max:78) + SaltedChSeqNum/2(max:40) + nFinalSalt(max:4) + deficit amount of FirstAndEndSaltChSeq/MiddleSaltChSeq(max<3)
                                                                                                                                                                    //So the maximum of SaltedChSeq is less than 136, so 136 is enough. Confirmed by SunWei 20240611.
    u32 NonMode0ShuffledChannelNum = 0;
    while (nShapeIteration < CSNumRepetitions) {
        u32 ShapeChSeqnum = cs_gen_shape_seq(seq1StartCh, seq2StartCh, CSShapeSelection, CSChannelJump, ShapeChSeq);                                                //pass

        cs_gen_salt(CSShapeSelection, ShapeChSeq, ShapeChSeqnum, FirstAndEndSaltChSeq, MiddleSaltChSeq);                                                            //pass

        u32 SaltedChSeqNum = cs_salt_chn_insert(CSShapeSelection, CSNumRepetitions, ShapeChSeq, ShapeChSeqnum, FirstAndEndSaltChSeq, MiddleSaltChSeq, SaltedChSeq); //pass
        //When all SaltedChSeq is available, the SaltedChSeqNum is appended to NonMode0ShuffledChannelNum, so the maximum of NonMode0ShuffledChannelArray is 134 * CSNumRepetitions. Confirmed by SunWei 20240528.
        NonMode0ShuffledChannelNum += cs_filter_shuffle(chm, SaltedChSeq, SaltedChSeqNum, NonMode0ShuffledChannelArray + NonMode0ShuffledChannelNum); //pass

        nShapeIteration++;
    }
    nShapeIteration                                 = 0;
    drbg->idParam[CSTransactionID_0].transactionCnt = 0;
    return NonMode0ShuffledChannelNum;
}

/**
 * @brief       This function should be used when new CS starts.
 *              This function now does not initiate parameters about channel selection algorithm #3c.
 * @param[in]   none
 * @return      none
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    cs_drbg_init(void)
{
    CS_DRBG_LOG("init");
    for (int i = 0; i < CSTransactionMaxIDNum; i++) {
        drbg->idParam[i].randomBitsNum  = 0;
        drbg->idParam[i].transactionCnt = 0;
    }
    drbg->stepCnt = 0;
}
#else
/**
 * @brief        This function is DRBG instantiation function h9.
 * @param[in]    cs_iv: 128-bit. the result of the CS Security Start procedure
 *                     cs_in: 64-bit. the result of the CS Security Start procedure
 *                     cs_pv: 128-bit. the result of the CS Security Start procedure
 *                     kdrbg: the result of this function.128-bit temporal key
 *                     vdrbg: the result of this function.128-bit nonce vector
 * @return      None. Function in stack only, and not judge return value when
 *                 using this function, so modify it to void. By SunWei 20240612
 */
_attribute_ram_code_sec_noinline_
void drbg_instantiation_func_h9(u8* cs_iv, u8* cs_in, u8* cs_pv, u8* kdrbg, u8* vdrbg)
{
    drbg->stepCnt = 0;
    cs_step_cnt_setup(drbg->stepCnt);
    cs_iv_setup((unsigned int*)(cs_iv));
    cs_pv_setup((unsigned int*)(cs_pv));
    cs_in_setup((unsigned int*)(cs_in));

    //drbg_init_start
    cs_h9_instantiation_trigger();
    cs_working_status_clear();

    cs_kdrbg_load((unsigned int*)(kdrbg));
    cs_vdrbg_load((unsigned int*)(vdrbg));
}

/**
 * @brief        This function is to generate 128 random bits
 * @param[in]    kdrbg: 128-bit temporal key
 *                     vdrbg: 128-bit nonce vector
 *                     transaction_id: transaction ID
 * @return      None. Function in stack only, and not judge return value when
 *                 using this function, so modify it to void. By SunWei 20240612
 */
_attribute_ram_code_sec_noinline_
void drbg_randomBits_func(u8 transaction_id, u8* kdrbg, u8* vdrbg)
{
    drbg_id_param_t *p = (drbg_id_param_t *)(&drbg->idParam[transaction_id]);

    cs_kdrbg_setup((unsigned int*)kdrbg);
    cs_vdrbg_setup((unsigned int*)vdrbg);
    cs_step_cnt_setup(drbg->stepCnt);
    cs_transaction_id_setup(transaction_id);
    cs_transaction_cnt_setup(p->transactionCnt);

    cs_cs_drbg_start();
    cs_working_status_clear();

    cs_randombits_load((unsigned int* )p->randomBits);

    p->randomBitsNum = 128;
    //For each step, only transaction id=0 or 1 may fetch more than 128 bits of data at a time.
    if((transaction_id == CSTransactionID_8))
    {
        p->transactionCnt++;
    }
}


/**
 * @brief        This function is  DRBG backtracking resistance.it shall be invoked to update the KDRBG and VDRBG
 *                 every time the CSProcCount is incremented.
 * @param[in]    kdrbg: 128-bit temporal key
 *                     vdrbg: 128-bit nonce vector
 * @return      None. Function in stack only, and not judge return value when
 *                 using this function, so modify it to void. By SunWei 20240612
 */
_attribute_ram_code_sec_noinline_
void drbg_backtracking_resistance(u8* kdrbg, u8* vdrbg)
{
    cs_kdrbg_setup((unsigned int*)(kdrbg));
    cs_vdrbg_setup((unsigned int*)(vdrbg));
    cs_transaction_id_setup(CSTransactionID_9);
    cs_transaction_cnt_setup(0);


    cs_drbg_core_trigger();
    cs_working_status_clear();

    cs_kdrbg_load((unsigned int*)(kdrbg));
    cs_vdrbg_load((unsigned int*)(vdrbg));
}

/**
 * @brief        This function is random bit generation function CS_DRBG
 * @param[in]    bit_num: number of required bits
 *                     cs_drbg_num: the return value of CS_DRBG
 *                     transaction_id: transaction ID
 * @return      None. Function in stack only, and not judge return value when
 *                 using this function, so modify it to void. By SunWei 20240612
 */
_attribute_ram_code_sec_noinline_
 void cs_drbg(u8 transaction_id,u8 bit_num, u8* cs_drbg_num)
{
    drbg_id_param_t *p = (drbg_id_param_t *)(&drbg->idParam[transaction_id]);
    if(bit_num > p->randomBitsNum)
    {
        drbg_randomBits_func(transaction_id, &drbg->kdrbg[0], &drbg->vdrbg[0]);//27us (32M) 17us (64M)
    }
    for(unsigned int i = bit_num-1;;i--)
    {
        unsigned char num = (p->randomBitsNum-1);
        if(BIT_IS_SET(p->randomBits[num>>3], (num%8)))
        {
            BIT_SET(cs_drbg_num[i>>3], i%8);
        }
        else
        {
            BIT_CLR(cs_drbg_num[i>>3], i%8);
        }
        p->randomBitsNum--;
        if(i==0) break;
    }

}

/**
 * @brief       This function optimizes CS_DRBG. This function can be used when the number of bits is a multiple of 8.
 * @param[in]   byte_num: number of required bytes
 *                  cs_drbg_num: the return value of CS_DRBG
 *                  transaction_id: transaction ID
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
_attribute_ram_code_sec_noinline_
void cs_drbg_byte(u8 transaction_id,u8 byte_num, u8* cs_drbg_num)
{
    drbg_id_param_t *p = (drbg_id_param_t *)(&drbg->idParam[transaction_id]);
    if(byte_num*8 > p->randomBitsNum)
    {
        drbg_randomBits_func(transaction_id, &drbg->kdrbg[0], &drbg->vdrbg[0]);//27us
    }
    for(u32 i = byte_num-1;;i--)
    {
        cs_drbg_num[i] = p->randomBits[(p->randomBitsNum-1)>>3];
        p->randomBitsNum -= 8;
        if(i==0) break;
    }
}

/**
 * @brief       This function is Channel Sounding random number generation function hr1.
 * @param[in]   r: The input to hr1 is an 8-bit unsigned integer, representing the arbitrary range 0 to r-1 from
                        which a random number is to be generated.
 * @return      result - random number
 */
_attribute_ram_code_sec_noinline_
u8  drbg_randnumgen_func_hr1(u8 transaction_id, u8 r)
{
    cs_randombits_pointer_setup((drbg->idParam[transaction_id].randomBits));
    cs_hr1_in(r);
    cs_restore_drbg_randombyte_index(drbg->idParam[transaction_id].randomBitsNum>>3);
    cs_transaction_id_setup(transaction_id);
    cs_transaction_cnt_setup(drbg->idParam[transaction_id].transactionCnt);

    cs_drbg_core_trigger();
    cs_working_status_clear();
    cs_randombits_load((unsigned int* )(drbg->idParam[transaction_id].randomBits));
    drbg->idParam[transaction_id].randomBitsNum = cs_load_drbg_randombyte_index()<<3;

    u8 hr1_out = cs_hr1_out();
    return hr1_out;
}
_attribute_aligned_(4) u8 ShuffledChannelArrayBuf[160];
_attribute_aligned_(4) u8 Src_channel[80];
/**
 * @brief        This function is channel selection Algorithm #3a or #3b for mode-0 steps.
 * @param[in]    chn_num:  the length of all available channel indices.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      None. Function in stack only, and not judge return value when
 *                 using this function, so modify it to void. By SunWei 20240612
 */
_attribute_ram_code_sec_noinline_
void chn_sel_3a_or_3b(u8 transaction_id,u8 chn_num,u8* s_chn,u8* d_chn)
{
    tlk_mem_cpy(Src_channel,s_chn,chn_num);
    cs_channel_num(chn_num);
    cs_channel_array_pointer_setup((unsigned int)Src_channel);
    cs_mapped_channel_array_pointer_setup((unsigned int)ShuffledChannelArrayBuf);
    cs_randombits_pointer_setup(drbg->idParam[transaction_id].randomBits);
    cs_restore_drbg_randombyte_index(drbg->idParam[transaction_id].randomBitsNum>>3);
    cs_transaction_id_setup(transaction_id);
    cs_transaction_cnt_setup(drbg->idParam[transaction_id].transactionCnt);

    cs_drbg_core_trigger();
    cs_working_status_clear();
    tlk_mem_cpy(d_chn,ShuffledChannelArrayBuf,chn_num);
    drbg->idParam[transaction_id].randomBitsNum = cs_load_drbg_randombyte_index()<<3;
    drbg->idParam[transaction_id].transactionCnt = 0;
    cs_randombits_load((unsigned int* )(drbg->idParam[transaction_id].randomBits));
}

/**
 * @brief        This function is channel selection Algorithm #3a for mode-0 steps.
 * @param[in]    chn_num:  the length of all available channel indices.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      None. Function in stack only, and not judge return value when
 *                 using this function, so modify it to void. By SunWei 20240612
 */
_attribute_ram_code_sec_noinline_
void chn_sel_3a(u8 chn_num,u8* s_chn,u8* d_chn)
{
    chn_sel_3a_or_3b(CSTransactionID_1, chn_num, s_chn, d_chn);
}

/**
 * @brief        This function is channel selection Algorithm #3b for non-mode-0 steps.
 * @param[in]    chn_num:  the length of all available channel indices.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      None. Function in stack only, and not judge return value when
 *                 using this function, so modify it to void. By SunWei 20240612
 */
_attribute_ram_code_sec_noinline_
void chn_sel_3b(u8 chn_num,u8* s_chn,u8* d_chn)
{
    chn_sel_3a_or_3b(CSTransactionID_0, chn_num, s_chn, d_chn);
}

/**
 * @brief        This function is to calculate CS Access Address.
 * @param[in]    reflector_accessaddr:  32-bit CS Access Address used in the CS SYNC from the reflector to initiator
                    initiator_accessaddr: 32-bit CS Access Address used in the CS SYNC from the initiator to reflector
 * @return      None. Function in stack only, and not judge return value when
 *                 using this function, so modify it to void. By SunWei 20240612
 */
_attribute_ram_code_sec_noinline_
void cs_access_addr(u8* reflector_accessaddr, u8* initiator_accessaddr)
{
    cs_transaction_id_setup(CSTransactionID_5);
    cs_restore_drbg_randombyte_index(drbg->idParam[CSTransactionID_5].randomBitsNum>>3);
    cs_transaction_cnt_setup(drbg->idParam[CSTransactionID_5].transactionCnt);
    cs_randombits_pointer_setup(drbg->idParam[CSTransactionID_5].randomBits);
    cs_restore_drbg_randombyte_index(0);
    cs_drbg_core_trigger();
    cs_working_status_clear();

    drbg->idParam[CSTransactionID_5].randomBitsNum = 0;
    drbg->idParam[CSTransactionID_5].transactionCnt = 0;

    cs_accesscode_load((u32*)reflector_accessaddr, (u32*)initiator_accessaddr);
}

/**
 * @brief        This function is to calculate the number of Main_Mode steps to execute.
 * @param[in]    main_mode_max:  the maximum number of Main_Mode steps that shall occur before the
 *                     occurrence of a single a Sub_Mode step.
 *                     main_mode_min: the minimum number of Main_Mode steps that shall occur before the
 *                     occurrence of a single Sub_Mode step.
 * @return      the number of Main_Mode steps
 */
_attribute_ram_code_sec_noinline_
unsigned char cs_sub_mode_insertion(u8 main_mode_max, u8 main_mode_min)
{
    return drbg_randnumgen_func_hr1(CSTransactionID_2, main_mode_max - main_mode_min + 1) + main_mode_min;
}

/**
 * @brief       This function is to calculate the position of the sounding sequence marker.
 * @param[in]   seqbit_len:   length of sounding sequence
 *                  pos_initiator: position of the marker in initiator
 *                  pos_reflector: position of the marker in reflector
 *                  sig_initiator: the marker signal in initiator
 *                  sig_reflector:  the marker signal in reflector
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ void
    cs_ss_marker(u8 seqbit_len, u8 *pos_initiator, u8 *pos_reflector, u8 *sig_initiator, u8 *sig_reflector)
{
    u8 flag_bit;
    cs_drbg(CSTransactionID_7, 1, (u8 *)&flag_bit);
    *sig_initiator = (flag_bit & 0x01);
    cs_drbg(CSTransactionID_7, 1, (u8 *)&flag_bit);

    if (seqbit_len == 32) {
        *sig_reflector   = (flag_bit & 0x01);
        *(pos_initiator) = drbg_randnumgen_func_hr1(CSTransactionID_6, 29);
        *(pos_reflector) = drbg_randnumgen_func_hr1(CSTransactionID_6, 29);
    } else {
        // LL/CS/CEN/INI/BV-13-C next four hr1 code can't change order -- yuexin
        *(pos_initiator)     = drbg_randnumgen_func_hr1(CSTransactionID_6, 64);
        *(pos_initiator + 1) = drbg_randnumgen_func_hr1(CSTransactionID_6, 75) + 67;
        if (*(pos_initiator + 1) > 92) {
            *(pos_initiator + 1) = 0xFF;
        } else {
            *(sig_initiator + 1) = (flag_bit & 0x01);
            cs_drbg(CSTransactionID_7, 1, (u8 *)&flag_bit);
            *sig_reflector = (flag_bit & 0x01);
        }
        *(pos_reflector)     = drbg_randnumgen_func_hr1(CSTransactionID_6, 64);
        *(pos_reflector + 1) = drbg_randnumgen_func_hr1(CSTransactionID_6, 75) + 67;
        if (*(pos_reflector + 1) > 92) {
            *(pos_reflector + 1) = 0xFF;
        } else {
            cs_drbg(CSTransactionID_7, 1, (u8 *)&flag_bit);
            *(sig_reflector + 1) = (flag_bit & 0x01);
        }
    }
}

/**
 * @brief        This function is to calculate the position of the sounding sequence marker.
 * @param[in]    seqbit_len:   length of sounding sequence
 *                     pos_initiator: position of the marker in initiator
 *                     pos_reflector: position of the marker in reflector
 * @return      None. Function in stack only, and not judge return value when
 *                 using this function, so modify it to void. By SunWei 20240612
 */
_attribute_ram_code_sec_noinline_
void cs_ss_marker_position(u8 seqbit_len, u8* pos_initiator, u8* pos_reflector)
{
    if(seqbit_len==32)
    {
        *(pos_initiator) = drbg_randnumgen_func_hr1(CSTransactionID_6, 29);
        *(pos_reflector) = drbg_randnumgen_func_hr1(CSTransactionID_6, 29);
    }
    else
    {
        *(pos_initiator) = drbg_randnumgen_func_hr1(CSTransactionID_6, 64);
        *(pos_initiator+1) = drbg_randnumgen_func_hr1(CSTransactionID_6, 75) + 67;

        *(pos_reflector) = drbg_randnumgen_func_hr1(CSTransactionID_6, 64);
        *(pos_reflector+1) = drbg_randnumgen_func_hr1(CSTransactionID_6, 75) + 67;
        /*if the starting bit position for the second marker exceeds 92, then the second
        marker shall be omitted.*/
        if(*(pos_initiator+1)>92) *(pos_initiator+1)=0xFF;
        if(*(pos_reflector+1)>92) *(pos_reflector+1)=0xFF;
    }
}

/**
 * @brief        This function is to calculate the sounding sequence marker signal.
 * @param[in]    sig_initiator: the marker signal in initiator
 *                     sig_reflector:  the marker signal in reflector
 * @return      None. Function in stack only, and not judge return value when
 *                 using this function, so modify it to void. By SunWei 20240612
 */
_attribute_ram_code_sec_noinline_
void cs_ss_marker_sig_sel(u8* sig_initiator, u8* sig_reflector, u8* pos_initiator, u8* pos_reflector)
{
    u8 flag_bit;
    cs_drbg(CSTransactionID_7, 1, (u8*)&flag_bit);
    *sig_initiator = (flag_bit&0x01);
    cs_drbg(CSTransactionID_7, 1, (u8*)&flag_bit);
    if(*(pos_initiator+1)!=0xff)
    {
        *(sig_initiator+1) = (flag_bit&0x01);
        cs_drbg(CSTransactionID_7, 1, (u8*)&flag_bit);
        *sig_reflector = (flag_bit&0x01);
        if(*(pos_reflector+1)!=0xff){
            cs_drbg(CSTransactionID_7, 1, (u8*)&flag_bit);
            *(sig_reflector+1) = (flag_bit&0x01);
        }
    }
    else
    {
        *sig_reflector = (flag_bit&0x01);
        if(*(pos_reflector+1)!=0xff){
            cs_drbg(CSTransactionID_7, 1, (u8*)&flag_bit);
            *(sig_reflector+1) = (flag_bit&0x01);
        }
    }
}

/**
 * @brief       This function is to generate the random sequence.
 * @param[in]   seq: the random sequence
 *                  seqbit_len:  the length of random sequence
 * @return      None. Function in stack only, and not judge return value when
 *              using this function, so modify it to void. By SunWei 20240612
 */
_attribute_ram_code_sec_noinline_
void cs_random_seq(u8* seqInit,u8* seqRefl, u8 seqbit_len)
{
    cs_drbg_byte(CSTransactionID_8, seqbit_len>>3,seqInit);
    cs_drbg_byte(CSTransactionID_8, seqbit_len>>3,seqRefl);
    drbg->idParam[CSTransactionID_8].transactionCnt = 0;
}

/**
 * @brief        This function is to calculate the antenna path permutation index.
 * @param[in]    na_p: The number of antenna path
 * @return       the antenna path permutation index
 */
_attribute_ram_code_sec_noinline_
unsigned char cs_antenna_path_perm(u8 na_p)
{
    unsigned char hr1_in = 1;
    do{
        hr1_in = hr1_in * na_p;
        na_p--;
    }while(na_p);

    return drbg_randnumgen_func_hr1(CSTransactionID_4, hr1_in);
}

/**
 * @brief        This function is to calculate the presence of an actual transmission in CS tone extension slot.
 * @param[in]    tpm_ext: the presence of  CS tone extension slot.
 * @return      None. Function in stack only, and not judge return value when
 *                 using this function, so modify it to void. By SunWei 20240612
 */
_attribute_ram_code_sec_noinline_
void cs_tpm_ext(u8* tpm_ext)
{
    cs_drbg(CSTransactionID_3, 2, tpm_ext);
}



/**
 * @brief        Channel Selection Algorithm #3c integrates rising and falling ramps into the resulting channel map for
                non-mode-0 CS steps.
 * @param[in]    chm: channel map.
 *                     CSShapeSelection
 *                     CSChannelJump
 *                     CSNumRepetitions
 *                     NonMode0ShuffledChannelArray
 * @return      NonMode0ShuffledChannelArrayNum: length of NonMode0ShuffledChannelArray
 */
_attribute_ram_code_sec_noinline_
u32 chn_sel_3c(u8* chm, u8 CSShapeSelection, u8 CSChannelJump, u8 CSNumRepetitions, u8* NonMode0ShuffledChannelArray)
{
    u8 chmNum;
    u32 NonMode0ShuffledChannelArrayNum = 0;
    blt_cs_extractEnableChnMap(chm, Src_channel, &chmNum);

    cs_channel_num(chmNum);
    cs_channel_array_pointer_setup((unsigned int)Src_channel);
    cs_restore_drbg_randombyte_index(drbg->idParam[CSTransactionID_0].randomBitsNum>>3);
    cs_transaction_id_setup(CSTransactionID_0);
    cs_transaction_cnt_setup(drbg->idParam[CSTransactionID_0].transactionCnt);
    chn_cas_3c_ctrl(chm, CSShapeSelection, CSChannelJump, CSNumRepetitions);
    u8 nShapeIteration = 0;
    while(nShapeIteration<CSNumRepetitions)
    {
        cs_mapped_channel_array_pointer_setup((unsigned int)ShuffledChannelArrayBuf);//NonMode0ShuffledChannelArray + NonMode0ShuffledChannelArrayNum);
        cs_randombits_pointer_setup(drbg->idParam[CSTransactionID_0].randomBits);
        cs_nShapeIteration_setup(nShapeIteration);
        cs_drbg_3c_trigger();
        cs_drbg_irq_clear();
        cs_working_status_clear();
        cs_randombits_load((unsigned int* )(drbg->idParam[CSTransactionID_0].randomBits));
        nShapeIteration++;
        chmNum = cs_get_NonMode0ShuffledChannelArrayNum();
        tlk_mem_cpy(NonMode0ShuffledChannelArray + NonMode0ShuffledChannelArrayNum,ShuffledChannelArrayBuf,chmNum);
        NonMode0ShuffledChannelArrayNum += chmNum;
    }

    drbg->idParam[CSTransactionID_0].randomBitsNum = cs_load_drbg_randombyte_index()<<3;
    drbg->idParam[CSTransactionID_0].transactionCnt = 0;
    chn_cas_3c_disable();
    return NonMode0ShuffledChannelArrayNum;
}

/**
 * @brief        This function should be used when new CS starts.
 *                 This function now does not initiate parameters about channel selection algorithm #3c.
 * @param[in]    none
 * @return      none
 */
void cs_drbg_init(void)
{
    for(int i = 0; i < CSTransactionMaxIDNum;i++)
    {
        drbg->idParam[i].randomBitsNum = 0;
        drbg->idParam[i].transactionCnt = 0;
    }
    drbg->stepCnt = 0;
    cs_step_cnt_setup(drbg->stepCnt);
}

#endif

/**
 * @brief      this function is used to initialize channel selection algorithm #3c feature in channel sounding module.
 * @param      none
 * @return     none
 */
void blc_ll_initChannelSelectionAlgorithm_3c_feature(void)
{
    chn_sel_3c_cb = chn_sel_3c;
    bltCsLocalSupportCap.Optional_Subfeatures_Supported |= CS_CSA_3C_SUPPORT;
}
