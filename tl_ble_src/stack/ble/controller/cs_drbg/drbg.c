#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"


#define CS_DRBG_LOG(fmt, ...)               tlkapi_send_string_u32s(0, "[CS][DRBG]"fmt "\n", ##__VA_ARGS__)


_attribute_data_retention_ u8 randomBits[10][16];
_attribute_data_retention_ u8 randomBits_num[10] = {0};
_attribute_data_retention_ u8 kdrbg_global[16];
_attribute_data_retention_ u8 vdrbg_global[16];
_attribute_data_retention_ u16 step_cnt_global = 0;
_attribute_data_retention_ u8 transaction_id_global = 0;
_attribute_data_retention_ u8 transaction_cnt_global[10] = {0};


// These parameters are used for #3c.
//Todo:need check by SUNWEI
_attribute_data_retention_ u8 startJitter;
_attribute_data_retention_ u8 nShapeIteration = 0;
_attribute_data_retention_ u8 procedure_1st_chm = 1;
_attribute_data_retention_ s8 seq1StartCh;
_attribute_data_retention_ s8 seq2StartCh;
_attribute_data_retention_ u8 maxRepsAllowed;
_attribute_data_retention_ u8 saltRate = 2;

/**
 * @brief       This function should be used when step ascends.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void cs_step_add(void)
{
    CS_DRBG_LOG("step++");
    //step_cnt_global ascend
    step_cnt_global++;
    //CSTransactionCounter shall always begin with a value of 0 the first time a new set of 128 random bits is
    //generated at any CS step for a specific transaction ID.
    smemset(transaction_cnt_global,0,10);
}

/**
 * @brief       This function should be used when step need to set up.
 * @param[in]   step: current step
 * @return      none
 */
_attribute_ram_code_ void cs_step_set(u16 step)
{
    CS_DRBG_LOG("step set to ", step);
    //step_cnt_global set to current step
    step_cnt_global = step;
    //CSTransactionCounter shall always begin with a value of 0 the first time a new set of 128 random bits is
    //generated at any CS step for a specific transaction ID.
    smemset(transaction_cnt_global,0,10);
}

/**
 * @brief       This function is to add a 32-bit array to other 32-bit array.
 * @param[in]   augend_data:the augend.
 *                  augend_data_len: size of augend_data
 *                  add_data: the addend.
 *                  add_data_len: size of add_data
 * @return      none
 */
_attribute_ram_code_ void multi_u32_add(u32* augend_data, u32 augend_data_len, u32* add_data, u32 add_data_len)
{
    u8 carry = 0;
    u32 i;
    for(i=0;i<augend_data_len;i++)
    {
        u32 sum;
        if(i<add_data_len)
            sum = augend_data[i] + add_data[i] + carry;
        else
            sum = augend_data[i] + carry;
        if(sum < augend_data[i])
            carry = 1;
        else if(sum != augend_data[i]||!carry)
            carry = 0;
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
 * @return      result - 0:success 1:fail
 */
u8 drbg_chain_func_f7(u8* key, u8* input_string, int string_len, u8* out)
{
    if(string_len<0||string_len%16) return 1;

    u8 hout[16] ={0};
    while(string_len>0)
    {
        u8 block_start = string_len-16;
        for(int i=0; i<16; i++)
        {
            hout[i] = hout[i]^input_string[block_start+i];
        }
        aes_encryption_le(key, hout, hout);
        string_len-=16;
    }
    smemcpy(out, hout, 16);
    return 0;
}

/**
 * @brief       This function is DRBG derivation function f8.
 * @param[in]   input_string: 320-bit
 *                  sm: the result of this function. 256-bit seed material
 * @return      result - 0:success 1:fail
 */
u8 drbg_derivation_func_f8(u32* input_string, u8* sm)
{
    u32 s[20] = {0,0,0,0x80000000,0,0,0,0,0,0,0,0,0,0,0x20,0x28,0,0,0,0};//Most 4 byte is V. V+S is 20 byte
    smemcpy((u8*)(s+4), (u8*)input_string, 40);
    u8 k[16] = {0x0F,0x0E,0x0D,0x0C,0x0B,0x0A,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00};
    u8 k2[16];
    u8 x[16];
    drbg_chain_func_f7(k, (u8*)s, 20*4, k2);
    s[19] = 1;
    drbg_chain_func_f7(k, (u8*)s, 20*4, x);
    aes_encryption_le(k2, x, (u8*)(sm+16));
    aes_encryption_le(k2, (u8*)(sm+16), (u8*)(sm));
    return 0;
}

/**
 * @brief       This function is DRBG update function f9.
 * @param[in]   sm: 256-bit seed material
 *                  key: 128-bit temporal key
 *                  nonce_vector: 128-bit nonce vector
 *                  kout: the result of this function.updated temporal key
 *                  vout: the result of this function.updated nonce vector
 * @return      result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ u8 drbg_update_func_f9(u32* sm, u8* key, u32* nonce_vector, u8* kout, u8* vout)
{
    u32 x[8];
    u32 a[1]={1};
    multi_u32_add(nonce_vector,4,a,1);
    aes_encryption_le(key, (u8*)nonce_vector, (u8*)(x+4));
    multi_u32_add(nonce_vector,4,a,1);
    aes_encryption_le(key, (u8*)nonce_vector, (u8*)x);
    for(int i=0; i<8; i++)
    {
        x[i] = x[i]^sm[i];
    }
    smemcpy(vout, (u8*)x, 16);
    smemcpy(kout, (u8*)(x+4), 16);
    return 0;
}

/**
 * @brief       This function is DRBG instantiation function h9.
 * @param[in]   cs_iv: 128-bit. the result of the CS Security Start procedure
 *                  cs_in: 64-bit. the result of the CS Security Start procedure
 *                  cs_pv: 128-bit. the result of the CS Security Start procedure
 *                  kdrbg: the result of this function.128-bit temporal key
 *                  vdrbg: the result of this function.128-bit nonce vector
 * @return      result - 0:success 1:fail
 */
u8 drbg_instantiation_func_h9(u8* cs_iv, u8* cs_in, u8* cs_pv, u8* kdrbg, u8* vdrbg)
{
    CS_DRBG_LOG("h9");
    step_cnt_global = 0;
    procedure_1st_chm = 1;
    smemset(transaction_cnt_global,0,10);


    u8 sm[32];
    u8 cs_pack[40];
    u8 k[16] = {0};
    u8 v[16] = {0};
    smemcpy(cs_pack, cs_pv, 16);
    smemcpy(cs_pack+16, cs_in, 8);
    smemcpy(cs_pack+24, cs_iv, 16);
    drbg_derivation_func_f8((u32*)cs_pack, sm);
    drbg_update_func_f9((u32*)sm, k, (u32*)v, kdrbg, vdrbg);
    return 0;
}

/**
 * @brief       This function is to generate 128 random bits
 * @param[in]   kdrbg: 128-bit temporal key
 *                  vdrbg: 128-bit nonce vector
 *                  step_cnt: CS step counter
 *                  transaction_id: transaction ID
 *                  transaction_cnt: transaction counter
 * @return      result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ u8 drbg_randomBits_func(u8* kdrbg, u8* vdrbg, u16 step_cnt, u8 transaction_id, u8 transaction_cnt)
{
    //u32 a[1]={(u32)transaction_cnt + ((u32)transaction_id<<8) + ((u32)step_cnt<<16)};
    u8 vdrbg_temp[16];
    smemcpy(vdrbg_temp,vdrbg,16);
    vdrbg_temp[0]+=transaction_cnt;
    vdrbg_temp[1]+=transaction_id;
    (*((u16*)(vdrbg_temp+2)))+=step_cnt;
    //multi_u32_add((u32*)vdrbg_temp,4,a,1);
    aes_encryption_le(kdrbg, vdrbg_temp, randomBits[transaction_id]);
    randomBits_num[transaction_id] = 128;
    return 0;
}

/**
 * @brief       This function is  DRBG backtracking resistance.it shall be invoked to update the KDRBG and VDRBG
 *              every time the CSProcCount is incremented.
 * @param[in]   kdrbg: 128-bit temporal key
 *                  vdrbg: 128-bit nonce vector
 * @return      result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ u8 drbg_backtracking_resistance(u8* kdrbg, u8* vdrbg)
{
    CS_DRBG_LOG("backtracking resistance");
    transaction_id_global = 9;
    u32 sm[8] = {0};
    vdrbg[1]+=transaction_id_global;
    drbg_update_func_f9(sm, kdrbg, (u32*)vdrbg, kdrbg, vdrbg);
    return 0;
}

/**
 * @brief       This function is random bit generation function CS_DRBG
 * @param[in]   bit_num: number of required bits
 *                  cs_drbg_num: the return value of CS_DRBG
 * @return      result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_  u8 cs_drbg(u8 bit_num, u8* cs_drbg_num)
{
    if(bit_num > 128) return 0;//check
    CS_DRBG_LOG("drbg_bit: step, num, t_ID, t_cnt:", step_cnt_global, bit_num, transaction_id_global, transaction_cnt_global[transaction_id_global]);
    if(bit_num > randomBits_num[transaction_id_global])
    {
        drbg_randomBits_func(kdrbg_global, vdrbg_global, step_cnt_global, transaction_id_global, transaction_cnt_global[transaction_id_global]);//27us (32M) 17us (64M)
        transaction_cnt_global[transaction_id_global]++;
    }
    for(u8 i = bit_num-1;;i--)
    {
        if(BIT_IS_SET(randomBits[transaction_id_global][(randomBits_num[transaction_id_global]-1)>>3], (randomBits_num[transaction_id_global]-1)%8))
        {
            BIT_SET(cs_drbg_num[i>>3], i%8);
        }
        else
        {
            BIT_CLR(cs_drbg_num[i>>3], i%8);
        }
        randomBits_num[transaction_id_global]--;
        if(i==0) break;
    }

    return 0;
}

/**
 * @brief       This function optimizes CS_DRBG. This function can be used when the number of bits is a multiple of 8.
 * @param[in]   byte_num: number of required bytes
 *                  cs_drbg_num: the return value of CS_DRBG
 * @return      result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_  u8 cs_drbg_byte(u8 byte_num, u8* cs_drbg_num)
{
    if(byte_num > 16) return 0;//check
    CS_DRBG_LOG("step %d, get %d bytes from t_ID %d t_cnt %d", step_cnt_global, byte_num, transaction_id_global, transaction_cnt_global[transaction_id_global]);
    if(byte_num*8 > randomBits_num[transaction_id_global])
    {
        drbg_randomBits_func(kdrbg_global, vdrbg_global, step_cnt_global, transaction_id_global, transaction_cnt_global[transaction_id_global]);//27us
        transaction_cnt_global[transaction_id_global]++;
    }
    for(u8 i = byte_num-1;;i--)
    {
        cs_drbg_num[i] = randomBits[transaction_id_global][(randomBits_num[transaction_id_global]-1)>>3];
        randomBits_num[transaction_id_global]-=8;
        if(i==0) break;
    }
    return 0;
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
_attribute_ram_code_ u8  drbg_randnumgen_func_hr1(u8 r)
{
    if(r==1) return 0;
    u8 cs_drbg_n;
    u8 rout;
    cs_drbg_byte(1, &cs_drbg_n);
    u32 Trand = r*cs_drbg_n;
    if((Trand&0xFF)<(256%r))
    {
        u32 cs_drbg_n2 = 0;
        cs_drbg_byte(1, (u8*)&cs_drbg_n2);
        rout = (((cs_drbg_n2*256)*r)+Trand)/65536;
    }
    else
        rout = Trand/256;
    return rout;
}

/**
 * @brief       This function is channel index shuffling function cr1.
 * @param[in]   chn_num:  the length of all available channel indices.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ u8 chn_index_shuffling_func_cr1(u8 chn_num,u8* s_chn,u8* d_chn)
{
    for(u8 i =0; i<chn_num; i++)
    {
        u8 j = drbg_randnumgen_func_hr1(i+1);
        if(i!=j)
            d_chn[i] = d_chn[j];
        d_chn[j] = s_chn[i];
    }
    return 0;
}

/**
 * @brief       This function is channel selection Algorithm #3a for mode-0 steps.
 * @param[in]   chn_num:  the length of all available channel indices.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ u8 chn_sel_3a(u8 chn_num,u8* s_chn,u8* d_chn)
{
    CS_DRBG_LOG("chn sel 3a");
    transaction_id_global = 1;
    return chn_index_shuffling_func_cr1(chn_num,s_chn,d_chn);
}

/**
 * @brief       This function is channel selection Algorithm #3b for non-mode-0 steps.
 * @param[in]   chn_num:  the length of all available channel indices.
                    s_chn: all available channel indices
                    d_chn: shuffled channel
 * @return      result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ u8 chn_sel_3b(u8 chn_num,u8* s_chn,u8* d_chn)
{
    CS_DRBG_LOG("chn sel 3b");
    transaction_id_global = 0;
    return chn_index_shuffling_func_cr1(chn_num,s_chn,d_chn);
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
_attribute_ram_code_ s8 cs_autocorr_ck(u32 num,u8 k)
{
    s8 c=0;
    for(u8 i=0;i<32-k;i++)
    {
        c = c+(((num>>i)&0x01)^((num>>(i+k))&0x01));
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
_attribute_ram_code_ s8 cs_autocorr_score(u32 si)
{
    s8 score;
    s8 c1 = cs_autocorr_ck(si,1)*2-31;
    s8 c2 = cs_autocorr_ck(si,2)*2-30;
    s8 c3 = cs_autocorr_ck(si,3)*2-29;
    score = abs(c1) + abs(c2) + abs(c3);
    return score;
}

/**
 * @brief       This function is to calculate CS Access Address.
 * @param[in]   reflector_accessaddr:  32-bit CS Access Address used in the CS SYNC from the reflector to initiator
                    initiator_accessaddr: 32-bit CS Access Address used in the CS SYNC from the initiator to reflector
 * @return      result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ u8 cs_access_addr(u8* reflector_accessaddr, u8* initiator_accessaddr)
{
    CS_DRBG_LOG("access addr");
    transaction_id_global = 5;
    //role: 0-Reflector,1-Initiator
    u32 cs_bit_string[4];
    cs_drbg_byte(16, (u8*)cs_bit_string);
    s8 s0_score = cs_autocorr_score(cs_bit_string[0]);
    s8 s1_score = cs_autocorr_score(cs_bit_string[1]);
    if(s0_score > s1_score)
        smemcpy(reflector_accessaddr,(u8*)(cs_bit_string+1),4);
    else
        smemcpy(reflector_accessaddr,(u8*)(cs_bit_string),4);
    s8 s2_score = cs_autocorr_score(cs_bit_string[2]);
    s8 s3_score = cs_autocorr_score(cs_bit_string[3]);
    if(s2_score > s3_score)
        smemcpy(initiator_accessaddr,(u8*)(cs_bit_string+3),4);
    else
        smemcpy(initiator_accessaddr,(u8*)(cs_bit_string+2),4);
    return 0;
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
_attribute_ram_code_ u8 cs_sub_mode_insertion(u8 main_mode_max, u8 main_mode_min)
{
    CS_DRBG_LOG("sub mode insertion");
    transaction_id_global = 2;
    return drbg_randnumgen_func_hr1(main_mode_max-main_mode_min+1) + main_mode_min;
}

/**
 * @brief       This function is to calculate the position of the sounding sequence marker.
 * @param[in]   seqbit_len:   length of sounding sequence
 *                  pos_initiator: position of the marker in initiator
 *                  pos_reflector: position of the marker in reflector
 * @return      result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ u8 cs_ss_marker_position(u8 seqbit_len, u8* pos_initiator, u8* pos_reflector)
{
    CS_DRBG_LOG("ss marker position");
    transaction_id_global = 6;
    if(seqbit_len!=32 && seqbit_len!=96)
            return 1;
    if(seqbit_len==32)
    {
        *(pos_initiator) = drbg_randnumgen_func_hr1(29);
        *(pos_reflector) = drbg_randnumgen_func_hr1(29);
    }
    else
    {
        *(pos_initiator) = drbg_randnumgen_func_hr1(64);
        *(pos_reflector) = drbg_randnumgen_func_hr1(64);
        *(pos_initiator+1) = drbg_randnumgen_func_hr1(75) + 67;
        *(pos_reflector+1) = drbg_randnumgen_func_hr1(75) + 67;
        if(*(pos_initiator+1)>92) *(pos_initiator+1)=0xFF;
        if(*(pos_reflector+1)>92) *(pos_reflector+1)=0xFF;
    }
    return 0;
}

/**
 * @brief       This function is to calculate the sounding sequence marker signal.
 * @param[in]   sig_initiator: the marker signal in initiator
 *                  sig_reflector:  the marker signal in reflector
 * @return      result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ u8 cs_ss_marker_sig_sel(u8* sig_initiator, u8* sig_reflector)
{
    CS_DRBG_LOG("ss marker sig sel");
    transaction_id_global = 7;
    u8 flag_bit;
    cs_drbg(2, (u8*)&flag_bit);
    *sig_reflector = flag_bit&0x01;
    *sig_initiator = flag_bit&0x02;
    return 0;
}

/**
 * @brief       This function is to generate the random sequence.
 * @param[in]   seq: the random sequence
 *                  seqbit_len:  the length of random sequence
 * @return      result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ u8 cs_random_seq(u8* seq, u8 seqbit_len)
{
    CS_DRBG_LOG("generate random sequence");
    transaction_id_global = 8;
    if(seqbit_len!=32 || seqbit_len!=64 || seqbit_len!=96 || seqbit_len!=128)
        return 1;
    cs_drbg_byte(seqbit_len>>3,seq);
    return 0;
}

/**
 * @brief       This function is to calculate the antenna path permutation index.
 * @param[in]   na_p: The number of antenna path
 * @return       the antenna path permutation index
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ u8 cs_antenna_path_perm(u8 na_p)
{
//  if(na_p<1||na_p>4)
//      return 0;
    CS_DRBG_LOG("antenna path perm");
    transaction_id_global = 4;
    u8 hr1_in = 1;
    do{
        hr1_in = hr1_in * na_p;
        na_p--;
    }while(na_p);
    return drbg_randnumgen_func_hr1(hr1_in);
}

/**
 * @brief       This function is to calculate the presence of an actual transmission in CS tone extension slot.
 * @param[in]   tpm_ext: the presence of  CS tone extension slot.
 * @return       result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ u8 cs_tpm_ext(u8* tpm_ext)
{
    CS_DRBG_LOG("tpm ext");
    transaction_id_global = 3;
    cs_drbg(2, tpm_ext);
    return 0;
}







/**
 * @brief       After the SaltedChSeq array is generated, it is then filtered through the filter channel bit map
 *              CSFilteredChM. The resulting filtered channel set is then shuffled in a block-based fashion.
 * @param[in]   chm: channel map.
 *                  SaltedChSeq
 *                  SaltedChSeqNum: length of SaltedChSeq
 *                  NonMode0ShuffledChannelArray
 *                  NonMode0ShuffledChannelNum: length of NonMode0ShuffledChannelArray
 * @return       result - 0:success 1:fail
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ u8 cs_filter_shuffle(u8* chm,u8* SaltedChSeq,u8 SaltedChSeqNum, u8* NonMode0ShuffledChannelArray,u8* NonMode0ShuffledChannelNum)
{
    u8 filteredSaltedChSeq[SaltedChSeqNum];
    u8 filteredSaltedChSeqNum = 0;
    for(u8 i=0;i<SaltedChSeqNum;i++)
    {
        if(BIT_IS_SET(chm[SaltedChSeq[i]>>3],SaltedChSeq[i]%8))
        {
            filteredSaltedChSeq[filteredSaltedChSeqNum++] = SaltedChSeq[i];
        }
    }
    *NonMode0ShuffledChannelNum = filteredSaltedChSeqNum;
    u8 nStepsInblock = max(10, filteredSaltedChSeqNum/4);
    u8 nBlocksToShuffle = max(1, filteredSaltedChSeqNum/nStepsInblock);

    for(u8 i=0;i<nBlocksToShuffle;i++)
    {
        u8 cr1_chn_num;
        if(i < nBlocksToShuffle-1)
        {
            cr1_chn_num = nStepsInblock;
            filteredSaltedChSeqNum-=nStepsInblock;
        }
        else
        {
            cr1_chn_num = filteredSaltedChSeqNum;
        }
        chn_index_shuffling_func_cr1(cr1_chn_num,filteredSaltedChSeq+i*nStepsInblock,NonMode0ShuffledChannelArray+i*nStepsInblock);
    }
    return 0;
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
 * @return       result - 0:success 1:fail
 */
_attribute_ram_code_ u8 cs_salt_chn_insert(u8 CSShapeSelection, u8 CSNumRepetitions, u8* ShapeChSeq, u8 ShapeChSeqnum, u8* FirstAndEndSaltChSeq,u8* MiddleSaltChSeq,u8* SaltedChSeq,u8* SaltedChSeqNum )
{
    u8 FirstAndEndSaltChSeqUseNum = 0;
    u8 MiddleSaltChSeqUseNum = 0;
    u8 ShapeChSeqUseNum = 0;
    *SaltedChSeqNum = 0;
    if(nShapeIteration == 0)
    {
        u8 nInitialSalt = drbg_randnumgen_func_hr1(10);
        for(u8 i=0;i<nInitialSalt;i++)
        {
            if(i%2) SaltedChSeq[(*SaltedChSeqNum)++] = MiddleSaltChSeq[MiddleSaltChSeqUseNum++];
            else SaltedChSeq[(*SaltedChSeqNum)++] = FirstAndEndSaltChSeq[FirstAndEndSaltChSeqUseNum++];
        }
    }
    for(u8 i=0;i<ShapeChSeqnum;i++)
    {
        if(i%saltRate==0)
        {
            u8 j = ShapeChSeq[i]/20+1;
            if(CSShapeSelection)
            {
                if(j==1||j==4) SaltedChSeq[(*SaltedChSeqNum)++] = FirstAndEndSaltChSeq[FirstAndEndSaltChSeqUseNum++];
                else SaltedChSeq[(*SaltedChSeqNum)++] = MiddleSaltChSeq[MiddleSaltChSeqUseNum++];
            }
            else
            {
                if(j==1||j==2) SaltedChSeq[(*SaltedChSeqNum)++] = FirstAndEndSaltChSeq[FirstAndEndSaltChSeqUseNum++];
                else SaltedChSeq[(*SaltedChSeqNum)++] = MiddleSaltChSeq[MiddleSaltChSeqUseNum++];
            }

        }
        SaltedChSeq[(*SaltedChSeqNum)++] = ShapeChSeq[ShapeChSeqUseNum++];
    }
    if(nShapeIteration == CSNumRepetitions - 1)
    {
        u8 nFinalSalt  = drbg_randnumgen_func_hr1(5);
        for(u8 i=0;i<nFinalSalt;i++)
        {
            if(i%2) SaltedChSeq[(*SaltedChSeqNum)++] = MiddleSaltChSeq[MiddleSaltChSeqUseNum++];
            else SaltedChSeq[(*SaltedChSeqNum)++] = FirstAndEndSaltChSeq[FirstAndEndSaltChSeqUseNum++];
        }
    }
    if(MiddleSaltChSeqUseNum<FirstAndEndSaltChSeqUseNum)
    {
        for(;MiddleSaltChSeqUseNum<FirstAndEndSaltChSeqUseNum;MiddleSaltChSeqUseNum++)
        {
            SaltedChSeq[(*SaltedChSeqNum)++] = MiddleSaltChSeq[MiddleSaltChSeqUseNum];
        }
    }
    else if(MiddleSaltChSeqUseNum>FirstAndEndSaltChSeqUseNum)
    {
        for(;FirstAndEndSaltChSeqUseNum<MiddleSaltChSeqUseNum;FirstAndEndSaltChSeqUseNum++)
        {
            SaltedChSeq[(*SaltedChSeqNum)++] = FirstAndEndSaltChSeq[FirstAndEndSaltChSeqUseNum];
        }
    }
    return 0;
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
 * @return       result - 0:success 1:fail
 */
_attribute_ram_code_ u8 cs_gen_salt(u8 CSShapeSelection, u8* ShapeChSeq, u8 ShapeChSeqnum, u8* FirstAndEndSaltChSeq, u8* FirstAndEndSaltChSeqNum,u8* MiddleSaltChSeq,u8* MiddleSaltChSeqNum)
{
    u8 usedChSeq[80] = {0};
    for(unsigned char i=0;i<ShapeChSeqnum; i++)
    {
        usedChSeq[ShapeChSeq[i]] = 1;
    }
    if(CSShapeSelection)
    {
        u8 firstAndEndAllChSeq[40] = {20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59};
        u8 middleAllChSeq[39] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78};
        u8 firstAndEndUnusedChSeq[40];
        u8 firstAndEndUnusedChSeqNum=0;
        u8 middleUnusedChSeq[39];
        u8 middleUnusedChSeqNum=0;
        for(unsigned char i=0; i<40; i++)
        {
            if(usedChSeq[firstAndEndAllChSeq[i]]==0)
            {
                firstAndEndUnusedChSeq[firstAndEndUnusedChSeqNum++] = firstAndEndAllChSeq[i];
            }
            if(i<39&&usedChSeq[middleAllChSeq[i]]==0)
            {
                middleUnusedChSeq[middleUnusedChSeqNum++] = middleAllChSeq[i];
            }
        }
        chn_index_shuffling_func_cr1(40,firstAndEndAllChSeq,FirstAndEndSaltChSeq+firstAndEndUnusedChSeqNum);
        chn_index_shuffling_func_cr1(39,middleAllChSeq,MiddleSaltChSeq+middleUnusedChSeqNum);
        chn_index_shuffling_func_cr1(firstAndEndUnusedChSeqNum,firstAndEndUnusedChSeq,FirstAndEndSaltChSeq);
        chn_index_shuffling_func_cr1(middleUnusedChSeqNum,middleUnusedChSeq,MiddleSaltChSeq);
        (*FirstAndEndSaltChSeqNum) = firstAndEndUnusedChSeqNum + 40;
        (*MiddleSaltChSeqNum) = middleUnusedChSeqNum + 39;
    }
    else
    {
        u8 firstAndEndAllChSeq[39] = {40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78};
        u8 middleAllChSeq[40] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39};
        u8 firstAndEndUnusedChSeq[39];
        u8 firstAndEndUnusedChSeqNum=0;
        u8 middleUnusedChSeq[40];
        u8 middleUnusedChSeqNum=0;
        for(unsigned char i=0; i<40; i++)
        {
            if(i<39&&usedChSeq[firstAndEndAllChSeq[i]]==0)
            {
                firstAndEndUnusedChSeq[firstAndEndUnusedChSeqNum++] = firstAndEndAllChSeq[i];
            }
            if(usedChSeq[middleAllChSeq[i]]==0)
            {
                middleUnusedChSeq[middleUnusedChSeqNum++] = middleAllChSeq[i];
            }
        }
        chn_index_shuffling_func_cr1(39,firstAndEndAllChSeq,FirstAndEndSaltChSeq+firstAndEndUnusedChSeqNum);
        chn_index_shuffling_func_cr1(40,middleAllChSeq,MiddleSaltChSeq+middleUnusedChSeqNum);
        chn_index_shuffling_func_cr1(firstAndEndUnusedChSeqNum,firstAndEndUnusedChSeq,FirstAndEndSaltChSeq);
        chn_index_shuffling_func_cr1(middleUnusedChSeqNum,middleUnusedChSeq,MiddleSaltChSeq);
        (*FirstAndEndSaltChSeqNum) = firstAndEndUnusedChSeqNum + 39;
        (*MiddleSaltChSeqNum) = middleUnusedChSeqNum + 40;
    }
    return 0;
}

/**
 * @brief       Each invocation of Channel Selection Algorithm #3c begins with the identification of the channels used to
 *              hold the shape selected by the CSShapeSelection parameter.
 * @param[in]   CSShapeSelection
 *                  CSChannelJump
 *                  ShapeChSeq
 *                  ShapeChSeqnum: length of ShapeChSeqnum
 * @return       result - 0:success 1:fail
 */
_attribute_ram_code_ u8 cs_gen_shape_seq(u8 CSShapeSelection, u8 CSChannelJump, u8* ShapeChSeq, u8* ShapeChSeqnum)
{
    if(procedure_1st_chm)
    {
        nShapeIteration = 0;
        procedure_1st_chm = 0;
        startJitter = drbg_randnumgen_func_hr1(CSChannelJump);
    }
    u8 offset = (nShapeIteration+startJitter)%CSChannelJump;
    s8 s1ch = seq1StartCh + offset;
    s8 s2ch = seq2StartCh + offset;
    (*ShapeChSeqnum)=0;
    if(CSShapeSelection)
    {
        s8 inc;
        if(seq1StartCh<seq2StartCh)
        {
            inc = CSChannelJump;
            while(s2ch>78){
                ShapeChSeq[(*ShapeChSeqnum)++] = s1ch;
                s1ch+=inc;
                s2ch-=inc;
            }
        }
        else
        {
            inc = -CSChannelJump;
            while(s1ch>78){
                ShapeChSeq[(*ShapeChSeqnum)++] = s2ch;
                s1ch+=inc;
                s2ch-=inc;
            }
        }

        while((s1ch>=0&&s1ch<=78)||(s2ch>=0&&s2ch<=78))
        {
            if(s1ch>=0&&s1ch<=78)
            {
                ShapeChSeq[(*ShapeChSeqnum)++] = s1ch;
                s1ch+=inc;
            }
            if(s2ch>=0&&s2ch<=78)
            {
                ShapeChSeq[(*ShapeChSeqnum)++] = s2ch;
                s2ch-=inc;
            }
        }
    }
    else
    {
        s8 risingCh,fallingCh;
        if(s1ch<s2ch)
        {
            risingCh = s1ch;
            fallingCh = s2ch;
        }
        else
        {
            risingCh = s2ch;
            fallingCh = s1ch;
        }
        while(risingCh<=78)
        {
            ShapeChSeq[(*ShapeChSeqnum)++] = risingCh;
            risingCh+=CSChannelJump;
        }
        while(fallingCh>=0)
        {
            ShapeChSeq[(*ShapeChSeqnum)++] = fallingCh;
            fallingCh-=CSChannelJump;
        }
    }
    return 0;
}

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
_attribute_ram_code_ u8 chn_sel_3c(u8* chm, u8 CSShapeSelection, u8 CSChannelJump, u8 CSNumRepetitions, u8* NonMode0ShuffledChannelArray, u8* NonMode0ShuffledChannelNum)
{
    CS_DRBG_LOG("chn sel 3c");
    transaction_id_global = 0;
    if(CSChannelJump == 2)
    {
        seq1StartCh = 1;
        seq2StartCh = 76;
        maxRepsAllowed = 1;
    }
    else if(CSChannelJump == 3)
    {
        seq1StartCh = 77;
        seq2StartCh = 0;
        maxRepsAllowed = 1;
    }
    else if(CSChannelJump == 4||CSChannelJump == 5)
    {
        seq1StartCh = 78;
        seq2StartCh = 0;
        maxRepsAllowed = 2;
    }
    else if(CSChannelJump == 6)
    {
        seq1StartCh = 76;
        seq2StartCh = 1;
        maxRepsAllowed = 3;
    }
    else if(CSChannelJump == 7)
    {
        seq1StartCh = 74;
        seq2StartCh = 1;
        maxRepsAllowed = 3;
    }
    else if(CSChannelJump == 8)
    {
        seq1StartCh = 76;
        seq2StartCh = 0;
        maxRepsAllowed = 3;
    }
    u8 ShapeChSeq[80];
    u8 ShapeChSeqnum;
    u8 FirstAndEndSaltChSeq[80];
    u8 FirstAndEndSaltChSeqNum;
    u8 MiddleSaltChSeq[80];
    u8 MiddleSaltChSeqNum;
    u8 SaltedChSeq[160];
    u8 SaltedChSeqNum;

    cs_gen_shape_seq(CSShapeSelection, CSChannelJump, ShapeChSeq, &ShapeChSeqnum);//pass

    cs_gen_salt(CSShapeSelection, ShapeChSeq, ShapeChSeqnum, FirstAndEndSaltChSeq, &FirstAndEndSaltChSeqNum,MiddleSaltChSeq,&MiddleSaltChSeqNum);//pass

    cs_salt_chn_insert(CSShapeSelection, CSNumRepetitions, ShapeChSeq, ShapeChSeqnum, FirstAndEndSaltChSeq,MiddleSaltChSeq,SaltedChSeq,&SaltedChSeqNum );//pass

    cs_filter_shuffle(chm,SaltedChSeq,SaltedChSeqNum, NonMode0ShuffledChannelArray,NonMode0ShuffledChannelNum);//pass
    nShapeIteration++;
    return 0;
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
_attribute_ram_code_ void cs_drbg_init(void)
{
    CS_DRBG_LOG("init");
    smemset(randomBits_num, 0, 10);
    smemset(transaction_cnt_global, 0, 10);
    step_cnt_global = 0;
}
