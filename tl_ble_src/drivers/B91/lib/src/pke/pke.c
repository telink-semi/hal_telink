/********************************************************************************************************
 * @file    pke.c
 *
 * @brief   This is the source file for B91
 *
 * @author  Driver Group
 * @date    2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
/********* pke version:1.0 *********/
#include "lib/include/pke/pke.h"
#include "lib/include/trng.h"


extern edward_curve_t ed25519[1];

static unsigned int step = 0x24;

extern unsigned int g_rnd_m_w;
/**
 * @brief       get rand.
 * @param[in]   rand    - the buffer rand.
 * @param[in]   byteLen - the length of rand.
 * @return      0:TRNG_SUCCESS,   1:TRNG_ERROR.
 */
unsigned char rand_get(unsigned char *rand, unsigned int byteLen)
{
    unsigned int word_len;
    unsigned char left_len;

    if(0 == rand)
    {
        return TRNG_ERROR;
    }

    word_len = byteLen >> 2;
    left_len = byteLen & 0x3;

    // obtain the data by word
    while (word_len--)
    {
        trng_init();
        *((unsigned int *)rand) = g_rnd_m_w;
        rand += 4;
    }

    // if the byteLen is not aligned by word
    if (left_len)
    {
        trng_init();
        memcpy(rand, &g_rnd_m_w, left_len);
    }

    return TRNG_SUCCESS;
}

/**
 * @brief       get real bit length of big number a of wordLen words.
 * @param[in]   a           - the buffer a.
 * @param[in]   wordLen     - the length of a.
 * @return      the real bit length of big number a of wordLen words.
 */
unsigned int valid_bits_get(const unsigned int *a, unsigned int wordLen)
{
    unsigned int i = 0;
    unsigned int j = 0;

    if(0 == wordLen)
    {
        return 0;
    }

    for (i = wordLen; i > 0; i--)
    {
        if (a[i - 1])
        {
            break;
        }
    }

    if(0 == i)
    {
        return 0;
    }

    for (j = 32; j > 0; j--)
    {
        if (a[i - 1] & (0x1 << (j - 1)))
        {
            break;
        }
    }

    return ((i - 1) << 5) + j;
}

/**
 * @brief       get real word length of big number a of max_words words.
 * @param[in]   a           - the buffer a.
 * @param[in]   max_words   - the length of a.
 * @return      get real word length of big number a.
 */
unsigned int valid_words_get(unsigned int *a, unsigned int max_words)
{
    unsigned int i = 0;

    for (i = max_words; i > 0; i--)
    {
        if (a[i - 1])
        {
            return i;
        }
    }

    return 0;
}

/**
 * @brief       reverse byte array.
 * @param[in]   in          - need to reverse the array.
 * @param[in]   out         - inverted array.
 * @param[in]   byteLen     - the length of array.
 * @return      none.
 */
void reverse_byte_array(const unsigned char *in, unsigned char *out, unsigned int byteLen)
{
    unsigned int idx, round = byteLen >> 1;
    unsigned char tmp;

    for (idx = 0; idx < round; idx++)
    {
        tmp = *(in + idx);
        *(out + idx) = *(in + byteLen - 1 - idx);
        *(out + byteLen - 1 - idx) = tmp;
    }

    if ((byteLen & 0x1) && (in != out))
    {
        *(out + round) = *(in + round);
    }
}

/**
 * @brief       load operand to specified addr.
 * @param[in]   data        - the buffer data.
 * @param[in]   wordLen     - the length of data.
 * @param[out]  baseaddr    - the address.
 * @return      none.
 */
void pke_load_operand(unsigned int *baseaddr, unsigned int *data, unsigned int wordLen)
{
    unsigned int i;

    if(baseaddr != data)
    {
        for (i = 0; i < wordLen; i++)
        {
            baseaddr[i] = data[i];//*((volatile unsigned int *)baseaddr+i) = data[i];
        }
    }
}

/**
 * @brief       get result operand from specified addr.
 * @param[in]   baseaddr    - the address.
 * @param[in]   data        - the buffer data.
 * @param[in]   wordLen     - the length of data.
 * @return      none.
 */
void pke_read_operand(unsigned int *baseaddr, unsigned int *data, unsigned int wordLen)
{
    unsigned int i;

    if(baseaddr != data)
    {
        for (i = 0; i < wordLen; i++)
        {
            data[i] = *((volatile unsigned int *)baseaddr+i);
        }
    }
}

/**
 * @brief       compare big integer a and b.
 * @param[in]   a           - value.
 * @param[in]   aWordLen    - the length of a.
 * @param[in]   b           - value.
 * @param[in]   bWordLen    - the length of b.
 * @return      0:a=b,   1:a>b,   -1: a<b.
 */
int big_integer_compare(unsigned int *a, unsigned int aWordLen, unsigned int *b, unsigned int bWordLen)
{
    int i;

    aWordLen = valid_words_get(a, aWordLen);
    bWordLen = valid_words_get(b, bWordLen);

    if(aWordLen > bWordLen)
        return 1;
    if(aWordLen < bWordLen)
        return -1;

    for(i=(aWordLen-1); i>=0; i--)
    {
        if(a[i] > b[i])
            return 1;
        if(a[i] < b[i])
            return -1;
    }

    return 0;
}


/**
 * @brief       This function is used to determine whether the array is all 0s.
 * @param[in]   data    - the buffer data.
 * @param[in]   len     - the length of data.
 * @return      1: all 0, 0: not all 0.
 */
int ismemzero4(unsigned int a[], unsigned int wordLen)
{
    unsigned int i;

    if(!wordLen)
    {
        return 1;
    }

    for(i=0; i<wordLen; i++)
    {
        if(a[i])
            return 0;
    }

    return 1;
}


/**
 * @brief       This function serves to clear pke status.
 * @param[in]   status  - the interrupt status that needs to be cleared.
 * @return      none.
 */
static inline void pke_clr_irq_status(void)
{
    if(PKE_STAT & 1)
    {
        PKE_STAT &= ~0x00000001;/*Note: Unlike the commonly used write 1 to clear 0, this flag bit writes 0 to clear 0.add by weihua.zhang, confirmed by jianzhi.chen*/
    }
}

/**
 * @brief       This function serves to enable pke interrupt function.
 * @param[in]   mask - the irq mask.
 * @return      none.
 */
static inline void pke_set_irq_mask(pke_conf_e mask)
{
    BM_SET(PKE_CONF, mask);
}

/**
 * @brief       This function serves to disable PKE interrupt function.
 * @param[in]   mask - the irq mask.
 * @return      none.
 */
static inline void pke_clr_irq_mask(pke_conf_e mask)
{
    BM_CLR(PKE_CONF, mask);
}

/**
 * @brief       set operand width please make sure 0 < bitLen <= 256.
 * @param[in]   bitLen  - operand width.
 * @return      none.
 */
void pke_set_operand_width(unsigned int bitLen)
{
    unsigned int len;

    step = 0x24;

    len = (2<<24)|(GET_WORD_LEN(bitLen)<<16);

    PKE_CONF = ((PKE_CONF & ~(0x07FF<<16)) | len);
}

/**
 * @brief       get current operand byte length.
 * @param[in]   none.
 * @return      length of current operand byte.
 */
unsigned int pke_get_operand_bytes(void)
{
    return step;
}

/**
 * @brief       set operation micro code.
 * @param[in]   addr    - pke micro code.
 * @return      none.
 */
static inline void pke_set_microcode(unsigned int addr)
{
    PKE_MC_PTR = addr;
}

/**
 * @brief       set exe config.
 * @param[in]   cfg - pke exe conf.
 * @return      none.
 */
void pke_set_exe_cfg(unsigned int cfg)
{
    PKE_EXE_CONF = cfg;
}

/**
 * @brief       start pke calculate.
 * @return      none.
 */
static inline void pke_opr_start(void)
{
    PKE_CTRL |= PKE_START_CALC;
}

/**
 * @brief       This is used to indicate the reason when the pke stopped.
 * @return      0 - normal stop.
 *              1 - received a termination request(CTRL.STOP is high).
 *              2 - no valid modulo inverse.
 *              3 - point is not on the curve(CTRL.CMD:PVER).
 *              4 - invalid microcode.
 */
static inline unsigned char pke_check_rt_code(void)
{
    return (unsigned char)(PKE_RT_CODE & 0x0F);
}


/**
 * @brief       This function serves to get pke status.
 * @param[in]   status  - the interrupt status to be obtained.
 * @retval    non-zero   -  the interrupt occurred.
 * @retval    zero  -  the interrupt did not occur.
 */
static inline unsigned int pke_get_irq_status(void)
{
    return PKE_STAT & 0x1;
}

/**
 * @brief       out = (a+b) mod modulus.
 * @param[in]   modulus - modulus.
 * @param[in]   a       - integer a.
 * @param[in]   b       - integer b.
 * @param[in]   wordLen - word length of modulus, a, b.
 * @param[out]  out     - out = a+b mod modulus.
 * @return      PKE_SUCCESS(success), other(error).
 */
unsigned char pke_mod_add(const unsigned int *modulus, const unsigned int *a, const unsigned int *b,
                   unsigned int *out, unsigned int wordLen)
{
    unsigned char ret;

    pke_set_operand_width(wordLen<<5);

    pke_load_operand((unsigned int *)(PKE_B(3,step)), (unsigned int *)modulus, wordLen);     //B3 modulus
    pke_load_operand((unsigned int *)(PKE_A(0,step)), (unsigned int *)a, wordLen);           //A0 a
    pke_load_operand((unsigned int *)(PKE_B(0,step)), (unsigned int *)b, wordLen);           //B0 b

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)(PKE_B(3,step))+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)(PKE_A(0,step))+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)(PKE_B(0,step))+wordLen, (step/4)-wordLen);
    }

    pke_set_microcode(MICROCODE_MODADD);

    //no need to config PKE_EXE_CONF

    pke_clr_irq_status();

    pke_opr_start();

    while(!pke_get_irq_status())  //0(in progress), 1(over)
    {;}

    ret =  pke_check_rt_code();
    if(ret)
    {
        return ret;
    }

    pke_read_operand((unsigned int *)(PKE_A(0, step)), out, wordLen);                     //A0 result

    return PKE_SUCCESS;
}

/**
 * @brief       out = (a-b) mod modulus.
 * @param[in]   modulus - input, modulus.
 * @param[in]   a       - input, integer a.
 * @param[in]   b       - input, integer b.
 * @param[in]   wordLen - input, word length of modulus, a, b.
 * @param[out]  out     - output, out = a-b mod modulus.
 * @return      PKE_SUCCESS(success), other(error).
 */
 unsigned char pke_mod_sub(const unsigned int *modulus, const unsigned int *a, const unsigned int *b,
                   unsigned int *out, unsigned int wordLen)
{
    unsigned char ret;

    pke_set_operand_width(wordLen<<5);

    pke_load_operand((unsigned int *)(PKE_B(3,step)), (unsigned int *)modulus, wordLen);      //B3 modulus
    pke_load_operand((unsigned int *)(PKE_A(0,step)), (unsigned int *)a, wordLen);            //A0 a
    pke_load_operand((unsigned int *)(PKE_B(0,step)), (unsigned int *)b, wordLen);            //B0 b

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)(PKE_B(3,step))+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)(PKE_A(0,step))+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)(PKE_B(0,step))+wordLen, (step/4)-wordLen);
    }

    pke_set_microcode(MICROCODE_MODSUB);

    //no need to config PKE_EXE_CONF

    pke_clr_irq_status();

    pke_opr_start();

    while(!pke_get_irq_status())  //0(in progress), 1(over)
    {;}

    ret =  pke_check_rt_code();
    if(ret)
    {
        return ret;
    }

    pke_read_operand((unsigned int *)(PKE_A(0, step)), out, wordLen);                     //A0 result

    return PKE_SUCCESS;
}

 /**
  * @brief      calc h(R^2 mod modulus) and n1( - modulus ^(-1) mod 2^w ) for modmul, pointMul. etc.
  *                 here w is bit width of word, i,e. 32.
  * @param[in]   modulus - input, modulus.
  * @param[in]   wordLen - input, word length of modulus or H.
  * @param[out]  h       - output, R^2 mod modulus.
  * @param[out]  n1      - output, - modulus ^(-1) mod 2^w, here w is 32 actually.
  * @return      PKE_SUCCESS(success), other(error).
  */
unsigned char pke_calc_pre_mont_output(const unsigned int *modulus, unsigned int wordLen, unsigned int *h, unsigned int *n1)
{
    unsigned char ret;

    pke_set_operand_width(wordLen<<5);

    pke_load_operand((unsigned int *)(PKE_B(3,step)), (unsigned int *)modulus, wordLen);            //B3 modulus
    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)(PKE_B(3,step))+wordLen, (step/4)-wordLen);
    }

    pke_set_microcode(MICROCODE_CAL_PRE_MON);

    //no need to config PKE_EXE_CONF

    pke_clr_irq_status();

    pke_opr_start();

    while(!pke_get_irq_status())  //0(in progress), 1(over)
    {;}

    ret =  pke_check_rt_code();
    if(ret)
    {
        return ret;
    }

    pke_read_operand((unsigned int *)(PKE_A(3,step)), (unsigned int *)h, wordLen);
    pke_read_operand((unsigned int *)(PKE_B(4,step)), (unsigned int *)n1, 1);

    return PKE_SUCCESS;
}

/**
 * @brief       like function pke_calc_pre_mont_output(), but this one is without output here.
 * @param[in]   modulus - input, modulus.
 * @param[in]   wordLen - input, word length of modulus.
 * @return      PKE_SUCCESS(success), other(error).
 */
unsigned char pke_calc_pre_mont_without_output(const unsigned int *modulus, unsigned int wordLen)
{
    return pke_calc_pre_mont_output(modulus, wordLen,
                                    (unsigned int *)(PKE_A(3,step)), (unsigned int *)(PKE_B(4,step)));
}

/**
 * @brief       load the pre-calculated mont parameters H(R^2 mod modulus) and
 *              n1( - modulus ^(-1) mod 2^w ).
 * @param[in]   H       - R^2 mod modulus.
 * @param[in]   n1      - modulus ^(-1) mod 2^w, here w is 32 actually.
 * @param[in]   wordLen - word length of modulus or H.
 * @return:     none.
 */
void pke_load_pre_calc_mont(unsigned int *H, unsigned int *n1, unsigned int wordLen)
{
    pke_set_operand_width(wordLen<<5);

    pke_load_operand((unsigned int *)(PKE_A(3,step)), H, wordLen);
    pke_load_operand((unsigned int *)(PKE_B(4,step)), n1, 1);

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)(PKE_A(3,step)+wordLen), (step/4)-wordLen);
    }
}

/**
 * @brief       out = a*b mod modulus.
 * @param[in]   modulus - input, modulus.
 * @param[in]   a       - input, integer a.
 * @param[in]   b       - input, integer b.
 * @param[in]   wordLen - input, word length of modulus, a, b.
 * @param[out]  out     - output, out = a*b mod modulus.
 * @return      PKE_SUCCESS(success), other(error).
 */
unsigned char pke_modmul_internal(const unsigned int *modulus, const unsigned int *a, const unsigned int *b,
                            unsigned int *out, unsigned int wordLen)
{
    unsigned char ret;

    pke_set_operand_width(wordLen<<5);

    pke_load_operand((unsigned int *)(PKE_B(3,step)), (unsigned int *)modulus, wordLen);      //B3 modulus
    pke_load_operand((unsigned int *)(PKE_A(0,step)), (unsigned int *)a, wordLen);            //A0 a
    pke_load_operand((unsigned int *)(PKE_B(0,step)), (unsigned int *)b, wordLen);            //B0 b

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)(PKE_B(3,step))+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)(PKE_A(0,step))+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)(PKE_B(0,step))+wordLen, (step/4)-wordLen);
    }

    pke_set_microcode(MICROCODE_MODMUL);

//  pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    pke_clr_irq_status();

    pke_opr_start();

    while(!pke_get_irq_status())   //0(in progress), 1(over)
    {;}

    ret = pke_check_rt_code();
    if(ret)
    {
        return ret;
    }

    pke_read_operand((unsigned int *)(PKE_A(0,step)), out, wordLen);                      //A0 result

    return PKE_SUCCESS;
}

/**
 * @brief       out = a*b mod modulus.
 * @param[in]   modulus - modulus.
 * @param[in]   a       - integer a.
 * @param[in]   b       - integer b.
 * @param[in]   wordLen - word length of modulus, a, b.
 * @param[out]  out     - out = a*b mod modulus.
 * @return      PKE_SUCCESS(success), other(error).
 */
unsigned char pke_mod_mul(const unsigned int *modulus, const unsigned int *a, const unsigned int *b,
                   unsigned int *out, unsigned int wordLen)
{
    unsigned char ret;

    ret = pke_calc_pre_mont_without_output(modulus, wordLen);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    return pke_modmul_internal(modulus, a, b, out, wordLen);
}

/**
 * @brief       ainv = a^(-1) mod modulus.
 * @param[in]   modulus     - modulus.
 * @param[in]   a           - integer a.
 * @param[in]   modWordLen  - word length of modulus, ainv.
 * @param[in]   aWordLen    - word length of integer a.
 * @param[out]  ainv        - ainv = a^(-1) mod modulus.
 * @return:     PKE_SUCCESS(success), other(inverse not exists or error).
 */
unsigned char pke_mod_inv(const unsigned int *modulus, const unsigned int *a, unsigned int *ainv, unsigned int modWordLen,
                   unsigned int aWordLen)
{
    unsigned char ret;

    pke_set_operand_width(modWordLen<<5);

    //B3 modulus
    pke_load_operand((unsigned int *)(PKE_B(3,step)), (unsigned int *)modulus, modWordLen);
    if((step/4) > modWordLen)
    {
        uint32_clear((unsigned int *)(PKE_B(3,step))+modWordLen, (step/4)-modWordLen);
    }

    //B0 a
    pke_load_operand((unsigned int *)(PKE_B(0,step)), (unsigned int *)a, aWordLen);
    if((step/4) > aWordLen)
    {
        uint32_clear((unsigned int *)(PKE_B(0,step))+aWordLen, (step/4)-aWordLen);
    }

    pke_set_microcode(MICROCODE_MODINV);

    //no need to config PKE_EXE_CONF

    pke_clr_irq_status();

    pke_opr_start();

    while(!pke_get_irq_status())   //0(in progress), 1(over)
    {;}

    ret = pke_check_rt_code();
    if(ret)
    {
        return ret;
    }

    pke_read_operand((unsigned int *)(PKE_A(0,step)), (unsigned int *)ainv, modWordLen);    //A0 ainv

    return PKE_SUCCESS;
}

/**
 * @brief       c = a - b.
 * @param[in]   a       - integer a.
 * @param[in]   b       - integer b.
 * @param[in]   wordLen - the length of a and b.
 * @param[out]  c       - integer c = a - b.
 * @return      none.
 */
void sub_u32(unsigned int *a, unsigned int *b, unsigned int *c, unsigned int wordLen)
{
    unsigned int i, carry, temp;

    carry = 0;
    for(i=0; i<wordLen; i++)
    {
        temp = a[i]-b[i];
        c[i] = temp-carry;
        if(temp > a[i] || c[i] > temp)
        {
            carry = 1;
        }
        else
        {
            carry = 0;   //not return; for security
        }
    }
}

/**
 * @brief       c = a mod b.
 * @param[in]   a           - integer a.
 * @param[in]   b           - integer b.
 * @param[in]   aWordLen    - word length of a.
 * @param[in]   bWordLen    - word length of b.
 * @param[in]   b_h         - parameter b_h.
 * @param[in]   b_n1        - parameter b_n1.
 * @param[out]  c           - c = a mod b.
 * @return      PKE_SUCCESS(success), other(error).
 */
unsigned char pke_mod(unsigned int *a, unsigned int aWordLen, unsigned int *b, unsigned int *b_h, unsigned int *b_n1,
                unsigned int bWordLen, unsigned int *c)
{
    int ret;
    unsigned int bitLen, tmpLen;
    unsigned int *a_high, *a_low, *p;

    ret = big_integer_compare(a, aWordLen, b, bWordLen);
    if(ret < 0)
    {
        aWordLen = valid_words_get(a, aWordLen);
        uint32_copy(c, a, aWordLen);
        uint32_clear(c+aWordLen, bWordLen-aWordLen);

        return PKE_SUCCESS;
    }
    else if(0 == ret)
    {
        uint32_clear(c, bWordLen);

        return PKE_SUCCESS;
    }

    bitLen = valid_bits_get(b, bWordLen) & 0x1F;
    pke_set_operand_width(bWordLen<<5);
    p = (unsigned int *)(PKE_A(1, step));

    //get a_high mod b
    a_high = c;
    if(bitLen)
    {
        tmpLen = aWordLen-bWordLen+1;
        uint32_copy(p, a+bWordLen-1, tmpLen);
        div2n_u32(p, tmpLen, bitLen);
        if(tmpLen < bWordLen)
        {
            uint32_clear(p+tmpLen, bWordLen-tmpLen);
        }

        if(big_integer_compare(p, bWordLen, b, bWordLen) >= 0)
        {
            sub_u32(p, b, a_high, bWordLen);
        }
        else
        {
            uint32_copy(a_high, p, bWordLen);
        }
    }
    else
    {
        tmpLen = aWordLen - bWordLen;
        if(big_integer_compare(a+bWordLen, tmpLen, b, bWordLen) > 0)
        {
            sub_u32(a+bWordLen, b, a_high, bWordLen);
        }
        else
        {
            uint32_copy(a_high, a+bWordLen, tmpLen);
            uint32_clear(a_high+tmpLen, bWordLen-tmpLen);
        }
    }

    if(NULL == b_h && NULL == b_n1)
    {
        ret = pke_calc_pre_mont_without_output(b, bWordLen);
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
    }
    else
    {
        pke_load_pre_calc_mont(b_h, b_n1, bWordLen);
    }

    //get 1000...000 mod b
    uint32_clear(p, bWordLen);
    if(bitLen)
    {
        p[bWordLen-1] = 1<<(bitLen);
    }
    sub_u32(p, b, (unsigned int *)(PKE_B(1, step)), bWordLen);


    //get a_high * 1000..000 mod b
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_modmul_internal(b, (unsigned int *)(PKE_B(1, step)), a_high, (unsigned int *)(PKE_B(1, step)), bWordLen);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }

    //get a_low mod b
    if(bitLen)
    {
        a_low = c;
        uint32_copy(p, a, bWordLen);
        p[bWordLen-1] &= ((1<<(bitLen))-1);
        if(big_integer_compare(p, bWordLen, b, bWordLen) >= 0)
        {
            sub_u32(p, b, a_low, bWordLen);
        }
        else
        {
            uint32_copy(a_low, p, bWordLen);
        }
    }
    else
    {
        if(big_integer_compare(a, bWordLen, b, bWordLen) >= 0)
        {
            a_low = c;
            sub_u32(a, b, a_low, bWordLen);
        }
        else
        {
            a_low = a;
        }
    }

    return pke_mod_add(b, a_low, (unsigned int *)(PKE_B(1, step)), c, bWordLen);
}

/**
 * @brief       a = a/(2^n).
 * @param[in]   a           - big integer a.
 * @param[in]   aWordLen    - word length of a.
 * @param[in]   n           - exponent of 2^n.
 * @return      word length of a = a/(2^n).
 * @attention:  1. make sure aWordLen is real word length of a.
 *              2. a may be 0, then aWordLen is 0, to make sure aWordLen-1 is available, so data
 *                 type of aWordLen is int, not unsigned int.
 */
unsigned int div2n_u32(unsigned int a[], int aWordLen, unsigned int n)
{
    int i;
    unsigned int j;

    //aWordLen = valid_words_get(a, aWordLen);

    if(!aWordLen)
        return 0;

    if(n<=32)
    {
        for(i=0; i<aWordLen-1; i++)
        {
            a[i] >>= n;
            a[i] |= (a[i+1]<<(32-n));
        }
        a[i] >>= n;

        if(!a[i])
            return i;
        return aWordLen;
    }
    else        //general method
    {
        j=n>>5; //j=n/32;
        n&=31;  //n=n%32;
        for(i=0; i<aWordLen-(int)j-1; i++)
        {
            a[i] = a[i+j]>>n;
            a[i] |= (a[i+j+1]<<(32-n));
        }
        a[i] = a[i+j]>>n;
        uint32_clear(a+aWordLen-j, j);

        if(!a[i])
            return i;
        return aWordLen-j;
    }
}

/********************************** ECCp functions *************************************/

/**
 * @brief       ECCP curve point mul(random point), Q=[k]P.
 * @param[in]   curve   - ECCP_CURVE struct pointer.
 * @param[in]   k       - scalar.
 * @param[in]   Px      - x coordinate of point P.
 * @param[in]   Py      - y coordinate of point P.
 * @param[out]  Qx      - x coordinate of point Q=[k]P.
 * @param[out]  Qy      - y coordinate of point Q=[k]P.
 * @return      PKE_SUCCESS(success), other(error).
 */
unsigned char pke_eccp_point_mul(eccp_curve_t *curve, unsigned int *k, unsigned int *Px, unsigned int *Py,
                      unsigned int *Qx, unsigned int *Qy)
{
    unsigned char ret;
    unsigned int wordLen = (curve->eccp_p_bitLen + 31)>>5;

    pke_set_operand_width(curve->eccp_p_bitLen);

    pke_load_operand((unsigned int *)PKE_B(3,step), curve->eccp_p, wordLen);              //B3 p
    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_B(3,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(3,step)+wordLen, (step/4)-wordLen);
    }

    if((NULL != curve->eccp_p_h) && (NULL != curve->eccp_p_n1))
    {
        pke_load_operand((unsigned int *)PKE_A(3,step), curve->eccp_p_h, wordLen);        //A3 p_h
        pke_load_operand((unsigned int *)PKE_B(4,step), curve->eccp_p_n1, 1);             //B4 p_n1
    }
    else
    {
        pke_calc_pre_mont_without_output((unsigned int *)PKE_B(3,step), wordLen);
    }

    pke_load_operand((unsigned int *)PKE_B(0,step), Px, wordLen);                         //B0 Px
    pke_load_operand((unsigned int *)PKE_B(1,step), Py, wordLen);                         //B1 Py
    pke_load_operand((unsigned int *)PKE_A(5,step), curve->eccp_a, wordLen);              //A5 a
    pke_load_operand((unsigned int *)PKE_A(4,step), k, wordLen);                          //A4 k

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_B(0,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_B(1,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(5,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(4,step)+wordLen, (step/4)-wordLen);
    }

    pke_set_microcode(MICROCODE_PMUL);

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    pke_clr_irq_status();

    pke_opr_start();

    while(!pke_get_irq_status())   //0(in progress)1(done))
    {;}

    ret = pke_check_rt_code();
    if(ret)
    {
        return ret;
    }

    pke_read_operand((unsigned int *)PKE_A(0,step), Qx, wordLen);
    if(Qy != NULL)
    {
        pke_read_operand((unsigned int *)PKE_A(1,step), Qy, wordLen);
    }

    return ret;
}

/**
 * @brief       ECCP curve point del point, Q=2P.
 * @param[in]   curve   - ECCP_CURVE struct pointer.
 * @param[in]   Px      - x coordinate of point P.
 * @param[in]   Py      - y coordinate of point P.
 * @param[out]  Qx      - x coordinate of point Q=2P.
 * @param[out]  Qy      - y coordinate of point Q=2P.
 * @return      PKE_SUCCESS(success), other(error).
 */
unsigned char pke_eccp_point_del(eccp_curve_t *curve, unsigned int *Px, unsigned int *Py,
                      unsigned int *Qx, unsigned int *Qy)
{
    unsigned char ret;
    unsigned int wordLen = (curve->eccp_p_bitLen + 31)>>5;

    pke_set_operand_width(curve->eccp_p_bitLen);

    pke_load_operand((unsigned int *)PKE_A(0,step), Px, wordLen);                         //A0 Px
    pke_load_operand((unsigned int *)PKE_A(1,step), Py, wordLen);                         //A1 Py
    pke_load_operand((unsigned int *)PKE_A(5,step), curve->eccp_a, wordLen);              //A5 a
    pke_load_operand((unsigned int *)PKE_B(3,step), curve->eccp_p, wordLen);              //B3 p

    if((0 != curve->eccp_p_h) && (0 != curve->eccp_p_n1))
    {
        pke_load_operand((unsigned int *)PKE_A(3,step), curve->eccp_p_h, wordLen);        //A3 p_h
        pke_load_operand((unsigned int *)PKE_B(4,step), curve->eccp_p_n1, 1);             //B4 p_n1
    }
    else
    {
        pke_set_microcode(MICROCODE_CAL_PRE_MON);

        pke_clr_irq_status();

        pke_opr_start();

        while(!pke_get_irq_status()){}   //0(in progress) 1(done))
    }

    pke_set_microcode(MICROCODE_PDBL);

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    pke_clr_irq_status();

    pke_opr_start();

    while(!pke_get_irq_status()){}   //0(in progress) 1(done))

    ret = pke_check_rt_code();

    if(ret)
    {
        return ret;
    }

    pke_read_operand((unsigned int *)PKE_A(0,step), Qx, wordLen);
    if(Qy != 0)
    {
        pke_read_operand((unsigned int *)PKE_A(1,step), Qy, wordLen);
    }

    return ret;
}

/**
 * @brief       ECCP curve point add, Q=P1+P2.
 * @param[in]   curve   - eccp curve struct pointer.
 * @param[in]   P1x     - x coordinate of point P1.
 * @param[in]   P1y     - y coordinate of point P1.
 * @param[in]   P2x     - x coordinate of point P2.
 * @param[in]   P2y     - y coordinate of point P2.
 * @param[out]  Qx      - x coordinate of point Q=P1+P2.
 * @param[out]  Qy      - y coordinate of point Q=P1+P2.
 * @return      PKE_SUCCESS(success), other(error).
 */
unsigned char pke_eccp_point_add(eccp_curve_t *curve, unsigned int *P1x, unsigned int *P1y, unsigned int *P2x, unsigned int *P2y,
                      unsigned int *Qx, unsigned int *Qy)
{
    unsigned char ret;
    unsigned int wordLen = (curve->eccp_p_bitLen + 31)>>5;

    pke_set_operand_width(curve->eccp_p_bitLen);

    pke_load_operand((unsigned int *)PKE_B(3,step), curve->eccp_p, wordLen);              //B3 p
    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_B(3,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(3,step)+wordLen, (step/4)-wordLen);
    }

    if((NULL != curve->eccp_p_h) && (NULL != curve->eccp_p_n1))
    {
        pke_load_operand((unsigned int *)PKE_A(3,step), curve->eccp_p_h, wordLen);        //A3 p_h
        pke_load_operand((unsigned int *)PKE_B(4,step), curve->eccp_p_n1, 1);             //B4 p_n1
    }
    else
    {
        pke_calc_pre_mont_without_output((unsigned int *)PKE_B(3,step), wordLen);
    }

    //pke_calc_pre_mont_without_output() may cover B0, so load B0(P1x) and other paras here
    pke_load_operand((unsigned int *)PKE_B(0,step), P1x, wordLen);                        //P1x
    pke_load_operand((unsigned int *)PKE_B(1,step), P1y, wordLen);                        //P1y
    pke_load_operand((unsigned int *)PKE_A(0,step), P2x, wordLen);                        //P2x
    pke_load_operand((unsigned int *)PKE_A(1,step), P2y, wordLen);                        //P2y

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_B(0,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_B(1,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(0,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(1,step)+wordLen, (step/4)-wordLen);
    }

    pke_set_microcode(MICROCODE_PADD);

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    pke_clr_irq_status();

    pke_opr_start();

    while(!pke_get_irq_status())    //0(in progress)1(done))
    {;}

    ret = pke_check_rt_code();
    if(ret)
    {
        return ret;
    }

    pke_read_operand((unsigned int *)PKE_A(0,step), Qx, wordLen);
    pke_read_operand((unsigned int *)PKE_A(1,step), Qy, wordLen);

    return ret;
}

#ifdef ECCP_POINT_DOUBLE
/**
 * @brief       ECCP curve point double, Q=[2]P.
 * @param[in]   curve   - ECCP_CURVE struct pointer.
 * @param[in]   Px      - x coordinate of point P.
 * @param[in]   Py      - y coordinate of point P.
 * @param[out]  Qx      - x coordinate of point Q=[2]P.
 * @param[out]  Qy      - y coordinate of point Q=[2]P.
 * @return      PKE_SUCCESS(success, on the curve), other(error or not on the curve).
 */
unsigned char eccp_pointDouble(eccp_curve_t *curve, unsigned int *Px, unsigned int *Py, unsigned int *Qx, unsigned int *Qy)
{
    unsigned char ret;
    unsigned int wordLen = (curve->eccp_p_bitLen + 31)>>5;

    pke_set_operand_width(curve->eccp_p_bitLen);

    pke_load_operand((unsigned int *)PKE_B(3,step), curve->eccp_p, wordLen);              //B3 p
    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_B(3,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(3,step)+wordLen, (step/4)-wordLen);
    }

    if((NULL != curve->eccp_p_h) && (NULL != curve->eccp_p_n1))
    {
        pke_load_operand((unsigned int *)PKE_A(3,step), curve->eccp_p_h, wordLen);        //A3 p_h
        pke_load_operand((unsigned int *)PKE_B(4,step), curve->eccp_p_n1, 1);             //B4 p_n1
    }
    else
    {
        pke_calc_pre_mont_without_output((unsigned int *)PKE_B(3,step), wordLen);
    }

    pke_load_operand((unsigned int *)PKE_A(0,step), Px, wordLen);                        //Px
    pke_load_operand((unsigned int *)PKE_A(1,step), Py, wordLen);                        //Py
    pke_load_operand((unsigned int *)PKE_A(5,step), curve->eccp_a, wordLen);             //a

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_A(0,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(1,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(5,step)+wordLen, (step/4)-wordLen);
    }

    pke_set_microcode(MICROCODE_PDBL);

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    pke_clear_int();

    pke_opr_start();

    while(!pke_opr_done())   //0(in progress)1(done))
    {;}

    ret = pke_check_rt_code();
    if(ret)
    {
        return ret;
    }

    pke_read_operand((unsigned int *)PKE_A(0,step), Qx, wordLen);
    pke_read_operand((unsigned int *)PKE_A(1,step), Qy, wordLen);

    return ret;
}
#endif

/**
 * @brief       check whether the input point P is on ECCP curve or not.
 * @param[in]   curve   - ECCP_CURVE struct pointer.
 * @param[in]   Px      - x coordinate of point P.
 * @param[in]   Py      - y coordinate of point P.
 * @return      PKE_SUCCESS(success, on the curve), other(error or not on the curve).
 */
unsigned char pke_eccp_point_verify(eccp_curve_t *curve, unsigned int *Px, unsigned int *Py)
{
    int ret;
    unsigned int wordLen = (curve->eccp_p_bitLen + 31)>>5;

    pke_set_operand_width(curve->eccp_p_bitLen);

    pke_load_operand((unsigned int *)PKE_B(3,step), curve->eccp_p, wordLen);              //B3 p
    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_B(3,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(3,step)+wordLen, (step/4)-wordLen);
    }

    if((NULL != curve->eccp_p_h) && (NULL != curve->eccp_p_n1))
    {
        pke_load_operand((unsigned int *)PKE_A(3,step), curve->eccp_p_h, wordLen);        //A3 p_h
        pke_load_operand((unsigned int *)PKE_B(4,step), curve->eccp_p_n1, 1);             //B4 p_n1
    }
    else
    {
        pke_calc_pre_mont_without_output((unsigned int *)PKE_B(3,step), wordLen);
    }

    pke_load_operand((unsigned int *)PKE_B(0,step), Px, wordLen);                         //Px
    pke_load_operand((unsigned int *)PKE_B(1,step), Py, wordLen);                         //Py
    pke_load_operand((unsigned int *)PKE_A(5,step), curve->eccp_a, wordLen);              //a
    pke_load_operand((unsigned int *)PKE_A(4,step), curve->eccp_b, wordLen);              //b

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_B(0,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_B(1,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(5,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(4,step)+wordLen, (step/4)-wordLen);
    }

    pke_set_microcode(MICROCODE_PVER);

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    pke_clr_irq_status();

    pke_opr_start();

    while(!pke_get_irq_status())   //0(in progress)1(done))
    {;}

    ret = pke_check_rt_code();
    if(ret)
    {
        return ret;
    }

    return PKE_SUCCESS;
}

/**
 * @brief       get ECCP key pair(the key pair could be used in ECDSA/ECDH).
 * @param[in]   curve   - eccp curve struct pointer.
 * @param[out]  priKey  - private key, big-endian.
 * @param[out]  pubKey  - public key, big-endian.
 * @return      PKE_SUCCESS(success), other(error).
 */
unsigned char eccp_getkey(eccp_curve_t *curve, unsigned char *priKey, unsigned char *pubKey)
{
    unsigned int tmpLen;
    unsigned int nByteLen = GET_BYTE_LEN(curve->eccp_n_bitLen);
    unsigned int nWordLen = GET_WORD_LEN(curve->eccp_n_bitLen);
    unsigned int pByteLen = GET_BYTE_LEN(curve->eccp_p_bitLen);
    unsigned int k[PKE_OPERAND_MAX_WORD_LEN] = {0};
    unsigned int x[PKE_OPERAND_MAX_WORD_LEN];
    unsigned int y[PKE_OPERAND_MAX_WORD_LEN];
    unsigned char ret;

ECCP_GETKEY_LOOP:

    ret = rand_get((unsigned char *)k, nByteLen);
    if(TRNG_SUCCESS != ret)
    {
        return ret;
    }

    //make sure k has the same bit length as n
    tmpLen = (curve->eccp_n_bitLen)&0x1F;
    if(tmpLen)
    {
        k[nWordLen-1] &= (1<<(tmpLen))-1;
    }

    //make sure k in [1, n-1]
    if(ismemzero4(k, nWordLen))
    {
        goto ECCP_GETKEY_LOOP;
    }
    if(big_integer_compare(k, nWordLen, curve->eccp_n, nWordLen) >= 0)
    {
        goto ECCP_GETKEY_LOOP;
    }

    //get pubKey
    ret = pke_eccp_point_mul(curve, k, curve->eccp_Gx, curve->eccp_Gy, x, y);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }

    reverse_byte_array((unsigned char *)k, priKey, nByteLen);
    reverse_byte_array((unsigned char *)x, pubKey, pByteLen);
    reverse_byte_array((unsigned char *)y, pubKey+pByteLen, pByteLen);

    return PKE_SUCCESS;
}

/****************************** ECCp functions finished ********************************/

/**************************** X25519 & Ed25519 functions *******************************/

/**
 * @brief       c25519 point mul(random point), Q=[k]P.
 * @param[in]   curve   - c25519 curve struct pointer.
 * @param[in]   k       - scalar.
 * @param[in]   Pu      - u coordinate of point P.
 * @param[out]  Qu      - u coordinate of point Q=[k]P.
 * @return      PKE_SUCCESS(success), other(error).
 */
unsigned char pke_x25519_point_mul(mont_curve_t *curve, unsigned int *k, unsigned int *Pu, unsigned int *Qu)
{
    unsigned char ret;
    unsigned int wordLen = (curve->p_bitLen + 31)>>5;

    pke_set_operand_width(curve->p_bitLen);

    pke_load_operand((unsigned int *)PKE_B(3,step), curve->p, wordLen);              //B3 p
    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_B(3,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(3,step)+wordLen, (step/4)-wordLen);
    }

    if((NULL != curve->p_h) && (NULL != curve->p_n1))
    {
        pke_load_operand((unsigned int *)PKE_A(3,step), curve->p_h, wordLen);        //A3 p_h
        pke_load_operand((unsigned int *)PKE_B(4,step), curve->p_n1, 1);             //B4 p_n1
    }
    else
    {
        pke_calc_pre_mont_without_output((unsigned int *)PKE_B(3,step), wordLen);
    }

    pke_load_operand((unsigned int *)PKE_A(0,step), Pu, wordLen);                     //A0 Pu
    pke_load_operand((unsigned int *)PKE_B(0,step), curve->a24, wordLen);             //B0 a24
    pke_load_operand((unsigned int *)PKE_A(4,step), k, wordLen);                      //A4 k

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_A(0,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_B(0,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(4,step)+wordLen, (step/4)-wordLen);
    }

    pke_set_microcode(MICROCODE_C25519_PMUL);

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    pke_clr_irq_status();

    pke_opr_start();

    while(!pke_get_irq_status())   //0(in progress)1(done))
    {;}

    ret =  pke_check_rt_code();
    if(ret)
    {
        return ret;
    }

    pke_read_operand((unsigned int *)PKE_A(1,step), Qu, wordLen);

    return PKE_SUCCESS;
}

/**
 * @brief       edwards25519 curve point mul(random point), Q=[k]P.
 * @param[in]   curve   - edwards25519 curve struct pointer.
 * @param[in]   k       - scalar.
 * @param[in]   Px      - x coordinate of point P.
 * @param[in]   Py      - y coordinate of point P.
 * @param[out]  Qx      - x coordinate of point Q=[k]P.
 * @param[out]  Qy      - y coordinate of point Q=[k]P.
 * @return      PKE_SUCCESS(success), other(error).
 */
unsigned char pke_ed25519_point_mul(edward_curve_t *curve, unsigned int *k, unsigned int *Px, unsigned int *Py,
                      unsigned int *Qx, unsigned int *Qy)
{
    unsigned char ret;
    unsigned int wordLen = (curve->p_bitLen + 31)>>5;

    pke_set_operand_width(curve->p_bitLen);

    pke_load_operand((unsigned int *)PKE_B(3,step), curve->p, wordLen);              //B3 p
    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_B(3,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(3,step)+wordLen, (step/4)-wordLen);
    }

    if((NULL != curve->p_h) && (NULL != curve->p_n1))
    {
        pke_load_operand((unsigned int *)PKE_A(3,step), curve->p_h, wordLen);        //A3 p_h
        pke_load_operand((unsigned int *)PKE_B(4,step), curve->p_n1, 1);             //B4 p_n1
    }
    else
    {
        pke_calc_pre_mont_without_output((unsigned int *)PKE_B(3,step), wordLen);
    }

    pke_load_operand((unsigned int *)PKE_A(1,step), Px, wordLen);                    //A1 Px
    pke_load_operand((unsigned int *)PKE_A(2,step), Py, wordLen);                    //A2 Py
    pke_load_operand((unsigned int *)PKE_B(0,step), curve->d, wordLen);              //B0 d
    pke_load_operand((unsigned int *)PKE_A(0,step), k, wordLen);                     //A0 k

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_A(1,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(2,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(5,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(4,step)+wordLen, (step/4)-wordLen);
    }

    pke_set_microcode(MICROCODE_Ed25519_PMUL);

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    pke_clr_irq_status();

    pke_opr_start();

    while(!pke_get_irq_status())   //0(in progress)1(done))
    {;}

    ret =  pke_check_rt_code();
    if(ret)
    {
        return ret;
    }

    pke_read_operand((unsigned int *)PKE_A(1,step), Qx, wordLen);
    if(Qy != NULL)
    {
        pke_read_operand((unsigned int *)PKE_A(2,step), Qy, wordLen);
    }

    return PKE_SUCCESS;
}

/**
 * @brief       edwards25519 point add, Q=P1+P2.
 * @param[in]   curve   - edwards25519 curve struct pointer.
 * @param[in]   P1x     - x coordinate of point P1.
 * @param[in]   P1y     - y coordinate of point P1.
 * @param[in]   P2x     - x coordinate of point P2.
 * @param[in]   P2y     - y coordinate of point P2.
 * @param[out]  Qx      - x coordinate of point Qx=P1x+P2x.
 * @param[out]  Qy      - y coordinate of point Qy=P1y+P2y.
 * @return      PKE_SUCCESS(success), other(error).
 */
unsigned char pke_ed25519_point_add(edward_curve_t *curve, unsigned int *P1x, unsigned int *P1y, unsigned int *P2x, unsigned int *P2y,
                      unsigned int *Qx, unsigned int *Qy)
{
    unsigned char ret;
    unsigned int wordLen = (curve->p_bitLen + 31)>>5;

    pke_set_operand_width(curve->p_bitLen);

    pke_load_operand((unsigned int *)PKE_B(3,step), curve->p, wordLen);              //B3 p
    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_B(3,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(3,step)+wordLen, (step/4)-wordLen);
    }

    if((NULL != curve->p_h) && (NULL != curve->p_n1))
    {
        pke_load_operand((unsigned int *)PKE_A(3,step), curve->p_h, wordLen);        //A3 p_h
        pke_load_operand((unsigned int *)PKE_B(4,step), curve->p_n1, 1);             //B4 p_n1
    }
    else
    {
        pke_calc_pre_mont_without_output((unsigned int *)PKE_B(3,step), wordLen);
    }

    //pke_calc_pre_mont_without_output() may cover B0, so load B0(P1x) and other paras here
    pke_load_operand((unsigned int *)PKE_A(1,step), P1x, wordLen);                   //P1x
    pke_load_operand((unsigned int *)PKE_A(2,step), P1y, wordLen);                   //P1y
    pke_load_operand((unsigned int *)PKE_B(1,step), P2x, wordLen);                   //P2x
    pke_load_operand((unsigned int *)PKE_B(2,step), P2y, wordLen);                   //P2y
    pke_load_operand((unsigned int *)PKE_B(0,step), curve->d, wordLen);              //B0 d

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_A(1,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(2,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_B(1,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_B(2,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_B(0,step)+wordLen, (step/4)-wordLen);
    }

    pke_set_microcode(MICROCODE_Ed25519_PADD);

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    pke_clr_irq_status();

    pke_opr_start();

    while(!pke_get_irq_status())    //0(in progress)1(done))
    {;}

    ret =  pke_check_rt_code();
    if(ret)
    {
        return ret;
    }

    pke_read_operand((unsigned int *)PKE_A(1,step), Qx, wordLen);
    pke_read_operand((unsigned int *)PKE_A(2,step), Qy, wordLen);

    return PKE_SUCCESS;
}

/**************************** X25519 & Ed25519 finished ********************************/
