/********************************************************************************************************
 * @file    pke.c
 *
 * @brief   This is the source file for TL721X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
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


#include <stdio.h>
#include "lib/include/crypto_common/utility.h"
#include "lib/include/pke/pke.h"
#include "lib/include/trng/trng.h"


#ifdef SUPPORT_SM2
extern eccp_curve_t sm2_curve[1];
#endif

#ifdef SUPPORT_C25519
extern edward_curve_t ed25519[1];
#endif

static unsigned int step;


/**
 * @brief       load input operand to baseaddr
 * @param[out]   baseaddr            - destination data.
 * @param[in]   data                 - source data.
 * @param[in]   wordLen              - word length of data.
 * @return      none
 */
static void pke_load_operand(volatile unsigned int *baseaddr, volatile unsigned int *data, unsigned int wordLen)
{
    if(baseaddr != data)
    {
        uint32_copy(baseaddr, data, wordLen);
    }
    else
    {;}
}

/**
 * @brief       set operand with an unsigned int value
 * @param[out]  a                    - operand a.
 * @param[in]   wordLen              - iword length of operand a.
 * @param[in]   b                    - unsigned int value b.
 * @return     none
 * @note
  @verbatim
      -# 1. aWordLen can not be 0
  @endverbatim
 */
static void pke_set_operand_uint32_value(unsigned int *a, unsigned int aWordLen, unsigned int b)
{
    unsigned int i = aWordLen;

    while(i>1)
    {
        a[--i] = 0;
    }

    a[0] = b;
}

/**
 * @brief      get result operand from baseaddr
 * @param[in]  baseaddr          - source data.
 * @param[out] data              - destination data.
 * @param[in]  wordLen           - word length of data.
 * @return     none
 */
void pke_read_operand(volatile unsigned int *baseaddr, volatile unsigned int *data, unsigned int wordLen)
{
    unsigned int i;

    if(baseaddr != data)
    {
        for (i = 0; i < wordLen; i++)
        {
            data[i] = *((volatile unsigned int *)baseaddr);
            baseaddr++;
        }
    }
    else
    {;}
}

/**
 * @brief     clear finished and interrupt tag
 * @return     none
 */
void pke_clear_interrupt(void)
{
    if(PKE_RISR & 1)
    {
        PKE_RISR &= ~0x01;//PKE_RISR |= 0x01;
    }
    else
    {;}
}

/**
 * @brief     enable pke interrupt
 * @return    none
 */
void pke_enable_interrupt(void)
{
    PKE_CFG |= (1 << PKE_INT_ENABLE_OFFSET);
}

/**
 * @brief     disable pke interrupt
 * @return    none
 */
void pke_disable_interrupt(void)
{
    PKE_CFG &= ~(1 << PKE_INT_ENABLE_OFFSET);
}

/**
 * @brief      set operand width
 * @param[in]  bitLen            - bit length of operand.
 * @return     none
 * @note
  @verbatim
      -# 1. please make sure 0 < bitLen <= OPERAND_MAX_BIT_LEN
  @endverbatim
 */
void pke_set_operand_width(unsigned int bitLen)
{
    unsigned int cfg =0, len;

    len = (bitLen+255)/256;

    if(1 == len)
    {
        cfg = 2;
        step = 0x24;
    }
    else if(2 == len)
    {
        cfg = 3;
        step = 0x44;
    }
    else if(len <= 4)
    {
        cfg = 4;
        step = 0x84;
    }
    else if(len <= 8)
    {
        cfg = 5;
        step = 0x104;
    }
    else if(len <= 16)
    {
        cfg = 6;
        step = 0x204;
    }
    else
    {;}

    cfg = (cfg<<16)|(bitLen);//cfg = (cfg<<16)|(len<<8);

    PKE_CFG &= ~(0x07FFFF);
    PKE_CFG |= cfg;  //printf("PKE_CFG=%08x", PKE_CFG);
}

/**
 * @brief     get current operand byte length
 * @return     none
 * @note
  @verbatim
      -# 1. current operand byte length
  @endverbatim
 */
unsigned int pke_get_operand_bytes(void)
{
    return step;
}

/**
 * @brief      set operation micro code
 * @param[in]  addr            - specific micro code.
 * @return     none
 */
void pke_set_microcode(unsigned int addr)
{
    PKE_MC_PTR = addr;
}

/**
 * @brief      set exe config
 * @param[in]  cfg            - specific config value.
 * @return     none
 */
void pke_set_exe_cfg(unsigned int cfg)
{
    PKE_EXE_CONF = cfg;
}

/**
 * @brief      start pke calc
 * @return     none
 */
void pke_start(void)
{
    PKE_CTRL |= PKE_START_CALC;
}

/**
 * @brief      return calc return code
 * @return     0(success), other(error)
 */
unsigned int pke_check_rt_code(void)
{
    return PKE_RT_CODE & 0x07;
}


/**
 * @brief       wait till done
 * @return      none
 */
void pke_wait_till_done(void)
{
    volatile unsigned int flag = 1;

    while(!(PKE_RISR & flag))
    {;}
}

/**
 * @brief      ainv = a^(-1) mod modulus
 * @param[in]  modulus          - modulus.
 * @param[in]  a                - integer a.
 * @param[in]  ainv             - ainv = a^(-1) mod modulus.
 * @param[in]  modWordLen       - word length of modulus and ainv.
 * @param[in]  aWordLen         - word length of a.
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1. please make sure aWordLen <= modWordLen <= OPERAND_MAX_WORD_LEN and a < modulus
  @endverbatim
 */
unsigned int pke_modinv( unsigned int *modulus,  unsigned int *a, unsigned int *ainv, unsigned int modWordLen,//----------------------
        unsigned int aWordLen)
{
    unsigned int ret;

    pke_set_operand_width(modWordLen<<5);

    pke_load_operand((volatile unsigned int *)(PKE_B(3,step)), (unsigned int *)modulus, modWordLen); //B3 modulus
    if((step/4) > modWordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(3,step))+modWordLen, (step/4)-modWordLen);
    }
    else
    {;}

    pke_load_operand((volatile unsigned int *)(PKE_B(0,step)), (unsigned int *)a, aWordLen);         //B0 a
    if((step/4) > aWordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(0,step))+aWordLen, (step/4)-aWordLen);
    }
    else
    {;}

    pke_set_microcode(MICROCODE_MODINV);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    ret = pke_check_rt_code();
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        pke_read_operand((volatile unsigned int *)(PKE_A(0,step)), ainv, modWordLen);            //A0 ainv

        return PKE_SUCCESS;
    }
}

/**
 * @brief      out = (a+b) mod modulus
 * @param[in]  modulus          - modulus.
 * @param[in]  a                - integer a.
 * @param[in]  b                - integer b.
 * @param[out] out              - out = a+b mod modulus.
 * @param[in]  wordLen          - word length of modulus, a, b.
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1. a,b must be less than modulus
      -# 2. wordLen must not be bigger than OPERAND_MAX_WORD_LEN
  @endverbatim
 */
unsigned int pke_modadd( unsigned int *modulus,  unsigned int *a,  unsigned int *b,//----------------------
                   unsigned int *out, unsigned int wordLen)
{
    unsigned int ret;

    pke_set_operand_width(wordLen<<5);

    pke_load_operand((volatile unsigned int *)(PKE_B(3,step)), (unsigned int *)modulus, wordLen);    //B3 modulus
    pke_load_operand((volatile unsigned int *)(PKE_A(0,step)), (unsigned int *)a, wordLen);          //A0 a
    pke_load_operand((volatile unsigned int *)(PKE_B(0,step)), (unsigned int *)b, wordLen);          //B0 b

    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(3,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_A(0,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_B(0,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_set_microcode(MICROCODE_MODADD);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    ret = pke_check_rt_code();
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        pke_read_operand((volatile unsigned int *)(PKE_A(0, step)), out, wordLen);               //A0 result

        return PKE_SUCCESS;
    }
}

/**
 * @brief      out = (a+b) mod modulus
 * @param[in]  modulus          - modulus.
 * @param[in]  a                - integer a.
 * @param[in]  b                - integer b.
 * @param[out] out              - out = out = a-b mod modulus
 * @param[in]  wordLen          - word length of modulus, a, b
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1. a,b must be less than modulus
      -# 2. wordLen must not be bigger than OPERAND_MAX_WORD_LEN
  @endverbatim
 */
unsigned int pke_modsub( unsigned int *modulus,  unsigned int *a,  unsigned int *b,//----------------------
                   unsigned int *out, unsigned int wordLen)
{
    unsigned int ret;

    pke_set_operand_width(wordLen<<5);

    pke_load_operand((volatile unsigned int *)(PKE_B(3,step)), (unsigned int *)modulus, wordLen);    //B3 modulus
    pke_load_operand((volatile unsigned int *)(PKE_A(0,step)), (unsigned int *)a, wordLen);          //A0 a
    pke_load_operand((volatile unsigned int *)(PKE_B(0,step)), (unsigned int *)b, wordLen);          //B0 b

    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(3,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_A(0,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_B(0,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_set_microcode(MICROCODE_MODSUB);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    ret = pke_check_rt_code();
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        pke_read_operand((volatile unsigned int *)(PKE_A(0,step)), out, wordLen);                //A0 result

        return PKE_SUCCESS;
    }
}

#if 0
/* function: out = a+b
 * parameters:
 *     a -------------------------- input, integer a
 *     b -------------------------- input, integer b
 *     out ------------------------ output, out = a+b
 *     wordLen -------------------- input, word length of a, b, out
 * return: PKE_SUCCESS(success), other(error)
 * caution:
 *     1. a+b may overflow
 *     2. wordLen must not be bigger than OPERAND_MAX_WORD_LEN
 */

unsigned int pke_add( unsigned int *a,  unsigned int *b, unsigned int *out, unsigned int wordLen)//---------------------------
{
    unsigned int ret;

    pke_set_operand_width(wordLen<<5);

    pke_load_operand((unsigned int *)(PKE_A(0,step)), (unsigned int *)a, wordLen);          //A1 a
    pke_load_operand((unsigned int *)(PKE_B(0,step)), (unsigned int *)b, wordLen);          //B1 b

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)(PKE_A(0,step))+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)(PKE_B(0,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_set_microcode(MICROCODE_INTADD);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    ret = pke_check_rt_code();
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        pke_read_operand((unsigned int *)(PKE_A(1,step)), out, wordLen);                //A1 result

        return PKE_SUCCESS;
    }
}
#else

/**
 * @brief      out = (a+b) mod modulus
 * @param[in]  modulus          - modulus.
 * @param[in]  a                - integer a.
 * @param[in]  b                - integer b.
 * @param[in]  out              - out = a+b
 * @param[in]  wordLen          - word length of a, b, out
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1. a+b may overflow
      -# 2. wordLen must not be bigger than OPERAND_MAX_WORD_LEN
  @endverbatim
 */
unsigned int pke_add(volatile unsigned int *a, volatile unsigned int *b, unsigned int *out, unsigned int wordLen)//---------------------------
{
    unsigned int i, carry, temp, temp2;

    carry = 0;
    for(i=0; i<wordLen; i++)
    {
        temp2 = a[i];
        temp = a[i]+b[i];
        out[i] = temp+carry;
        if(temp < temp2 || out[i] < carry)
        {
            carry = 1;
        }
        else
        {
            carry = 0;
        }
    }

    return PKE_SUCCESS;
}
#endif
#if 0
/* function: out = a-b
 * parameters:
 *     a -------------------------- input, integer a
 *     b -------------------------- input, integer b
 *     out ------------------------ output, out = a-b
 *     wordLen -------------------- input, word length of a, b, out
 * return: PKE_SUCCESS(success), other(error)
 * caution:
 *     1. please make sure a > b
 *     2. wordLen must not be bigger than OPERAND_MAX_WORD_LEN
 */
unsigned int pke_sub( unsigned int *a,  unsigned int *b, unsigned int *out, unsigned int wordLen)//-------------------------------
{
    unsigned int ret;

    pke_set_operand_width(wordLen<<5);

    pke_load_operand((unsigned int *)(PKE_A(1,step)), (unsigned int *)a, wordLen);          //A1 a
    pke_load_operand((unsigned int *)(PKE_B(1,step)), (unsigned int *)b, wordLen);          //B1 b

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)(PKE_A(1,step))+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)(PKE_B(1,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_set_microcode(MICROCODE_INTSUB);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    ret = pke_check_rt_code();
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        pke_read_operand((unsigned int *)(PKE_A(1,step)), out, wordLen);                    //A1 result

        return PKE_SUCCESS;
    }
}
#else

/**
 * @brief      out = a-b
 * @param[in]  a          - integer a
 * @param[in]  b          - integer b
 * @param[out] out        - out = a-b
 * @param[in]  wordLen    - word length of a, b, out
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1. please make sure a > b
      -# 2. wordLen must not be bigger than OPERAND_MAX_WORD_LEN
  @endverbatim
 */
unsigned int pke_sub( unsigned int *a,  unsigned int *b, unsigned int *out, unsigned int wordLen)//-------------------------------
{
    unsigned int i, carry, tmp, tmp2;

    carry = 0;
    for(i=0; i<wordLen; i++)
    {
        tmp = a[i]-b[i];
        tmp2 = tmp-carry;
        if(tmp > a[i] || tmp2 > tmp)
        {
            carry = 1;
        }
        else
        {
            carry = 0;
        }
        out[i] = tmp2;
    }

    return PKE_SUCCESS;
}
#endif

#if 1
/**
 * @brief      out = a*b
a* @param[in]  a          - integer a
 * @param[in]  b          - integer b
 * @param[out] out        - out = a-b
 * @param[in]  wordLen    - word length of a, b, out
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1. please make sure out buffer word length is bigger than (2*max_bit_len(a,b)+0x1F)>>5
      -# 2. please make sure ab_wordLen is not bigger than OPERAND_MAX_WORD_LEN/2
  @endverbatim
 */
unsigned int pke_mul( unsigned int *a,  unsigned int *b, volatile unsigned int *out, unsigned int ab_wordLen)//-----------------------------
{
    unsigned int bitLen, tempLen;
    unsigned int ret;

    bitLen = get_valid_bits((void*)a, ab_wordLen);
    tempLen = get_valid_bits((void*)b, ab_wordLen);

    bitLen = GET_MAX_LEN(bitLen,tempLen);
    tempLen = GET_WORD_LEN(bitLen<<1);
    if(tempLen < (ab_wordLen<<1))
    {
        tempLen = (ab_wordLen<<1)-1;
    }
    else
    {
        tempLen = (ab_wordLen<<1);
    }

    pke_set_operand_width(tempLen<<5);  //for pke lp
    //pke_set_operand_width(GET_MAX_LEN(tempLen<<5,512));  //for pke hp

    pke_load_operand((volatile unsigned int *)(PKE_A(0,step)), (unsigned int *)a, ab_wordLen);       //A0 a
    pke_load_operand((volatile unsigned int *)(PKE_B(0,step)), (unsigned int *)b, ab_wordLen);       //B0 b

    uint32_clear((volatile unsigned int *)(PKE_A(0,step))+ab_wordLen, (step/4)-ab_wordLen);
    uint32_clear((volatile unsigned int *)(PKE_B(0,step))+ab_wordLen, (step/4)-ab_wordLen);

    pke_set_microcode(MICROCODE_INTMUL);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    ret = pke_check_rt_code();
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        pke_read_operand((volatile unsigned int *)(PKE_A(1,step)), out, tempLen);                //A1 result

        return PKE_SUCCESS;
    }
}
#else
/**
 * @brief      out = a*b
 * @param[in]  a          - integer a
 * @param[in]  b          - integer b
 * @param[out] out        - out = a-b
 * @param[in]  wordLen    - word length of a, b, out
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1. please make sure out buffer word length is bigger than (2*max_bit_len(a,b)+0x1F)>>5
      -# 2. please make sure ab_wordLen is not bigger than OPERAND_MAX_WORD_LEN/2
  @endverbatim
 */
unsigned int pke_mul( unsigned int *a,  unsigned int *b, unsigned int *out, unsigned int ab_wordLen)//-----------------------------
{
    uint64_t UV;
    unsigned int i,j,*U,*V;
    unsigned int bitLen, tempLen;

    bitLen = get_valid_bits(a, ab_wordLen);
    tempLen = get_valid_bits(b, ab_wordLen);

    bitLen = GET_MAX_LEN(bitLen,tempLen);
    tempLen = GET_WORD_LEN(bitLen<<1);
    if(tempLen < (ab_wordLen<<1))
    {
        tempLen = (ab_wordLen<<1)-1;
    }
    else
    {
        tempLen = (ab_wordLen<<1);
    }

    uint32_clear(out, tempLen);

    V = (unsigned int *)(&UV);
    U = V+1;
    for(i=0; i<ab_wordLen; i++)
    {
        *U = 0;
        for(j=0; j<ab_wordLen; j++)
        {
            UV = ((uint64_t)a[i])*b[j]+out[i+j]+(*U);
            out[i+j] = (*V);
        }
        out[i+j] = (*U);
    }

    return PKE_SUCCESS;
}
#endif
#if 0

/* function: calc n0(- modulus ^(-1) mod 2^w) for modMul, and pointMul. etc.
 * parameters: none
 * return: PKE_SUCCESS(success), other(error)
 * caution:
 *     1. before calling, please make sure the modulus is set in PKE_A(a, 0)
 *     2. please make sure the modulus is odd, and word length of the modulus
 *        is not bigger than OPERAND_MAX_WORD_LEN
 *     3. the result is set in the internal register, no need to output.//---------------------------

unsigned int pke_pre_calc_mont_N0()
{
    pke_set_microcode(MICROCODE_MGMR_PRE_N0);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    return pke_check_rt_code();
} */

#endif

/**
 * @brief      calc H(R^2 mod modulus) and n1( - modulus ^(-1) mod 2^w ) for modMul,modExp, and pointMul. etc.
 *            here w is bit width of word, i,e. 32.
 * @param[in]  modulus          - modulus
 * @param[in]  bitLen           - bit length of modulus
 * @param[out] H                - R^2 mod modulus
 * @param[in]  n1               - modulus ^(-1) mod 2^w, here w is 32 actually
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1. modulus must be odd
      -# 2. please make sure word length of buffer H is equal to wordLen(word length of modulus),
 *        and n1 only need one word.
 *    -# 3. wordLen must not be bigger than OPERAND_MAX_WORD_LEN
  @endverbatim
 */
unsigned int pke_pre_calc_mont( volatile unsigned int *modulus, unsigned int bitLen, unsigned int *H, unsigned int *n1)
{
    unsigned int wordLen = GET_WORD_LEN(bitLen);
    unsigned int ret;

    pke_set_operand_width(bitLen);//pke_set_operand_width(wordLen<<5);

    pke_load_operand((volatile unsigned int *)(PKE_B(3,step)), (volatile unsigned int *)modulus, wordLen);    //B3 modulus

    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(3,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_A(3,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_set_microcode(MICROCODE_MGMR_PRE);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    ret = pke_check_rt_code();
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }

    if(NULL != H)
    {
        pke_read_operand((volatile unsigned int *)(PKE_A(3,step)), H, wordLen);                  //A3 H
    }
    else
    {;}

    if(NULL != n1)
    {
        pke_read_operand((volatile unsigned int *)(PKE_B(4,step)), n1, 1);                       //B4 n1
    }
    else
    {;}

    return PKE_SUCCESS;
}

/**
 * @brief      like function pke_pre_calc_mont(), but this one is without output here
 * @param[in]  modulus          - modulus
 * @param[in]  wordLen          - word length of modulus
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1. modulus must be odd
      -# 2. wordLen must not be bigger than OPERAND_MAX_WORD_LEN
  @endverbatim
 */
unsigned int pke_pre_calc_mont_no_output( unsigned int *modulus, unsigned int wordLen)
{
    return pke_pre_calc_mont(modulus, get_valid_bits((void*)modulus, wordLen), NULL, NULL);
}

/**
 * @brief      load the pre-calculated mont parameters H(R^2 mod modulus)
 * @param[in]  H           - modulus
 * @param[in]  n1          - word length of modulus
 * @param[in]  wordLen     - word length of modulus or H
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1.please make sure the 2 input parameters are both valid
      -# 2.wordLen must not be bigger than OPERAND_MAX_WORD_LEN
  @endverbatim
 */
void pke_load_pre_calc_mont(volatile unsigned int *H, volatile unsigned int *n1, unsigned int wordLen)
{
    pke_set_operand_width(wordLen<<5);

    pke_load_operand((volatile unsigned int *)(PKE_A(3,step)), H, wordLen);
    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_A(3,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_load_operand((volatile unsigned int *)(PKE_B(4,step)), n1, 1);
}

/**
 * @brief      load modulus and pre-calculated mont parameters H(R^2 mod modulus) and n0'(- modulus ^(-1) mod 2^w) for hardware operation
 * @param[in]  modulus           - modulus
 * @param[in]  modulus_h         - R^2 mod modulus
 * @param[in]  modulus_n0        - modulus ^(-1) mod 2^w, here w is 32 actually
 * @param[in]  bitLen            - bit length of modulus
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1.modulus must be odd
      -# 2.bitLen must not be bigger than OPERAND_MAX_BIT_LEN
  @endverbatim
 */
unsigned int pke_load_modulus_and_pre_monts(unsigned int *modulus, unsigned int *modulus_h, unsigned int *modulus_n0, unsigned int bitLen)
{
    unsigned int wordLen = GET_WORD_LEN(bitLen);

    pke_set_operand_width(bitLen);

    pke_load_operand((volatile unsigned int *)(PKE_B(3,step)), (unsigned int *)modulus, wordLen);    //B3 modulus
    pke_load_operand((volatile unsigned int *)(PKE_A(3,step)), modulus_h, wordLen);              //A3 h
    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(3,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_A(3,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_load_operand((volatile unsigned int *)(PKE_B(4,step)), modulus_n0, 1);

    return PKE_SUCCESS;
}

/**
 * @brief      set modulus and pre-calculated mont parameters H(R^2 mod modulus) and n0'(- modulus ^(-1) mod 2^w) for hardware operation
 * @param[in]  modulus           - modulus
 * @param[in]  modulus_h         - R^2 mod modulus
 * @param[in]  modulus_n0        - modulus ^(-1) mod 2^w, here w is 32 actually
 * @param[in]  bitLen            - bit length of modulus
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1.modulus must be odd
      -# 2.bitLen must not be bigger than OPERAND_MAX_BIT_LEN
  @endverbatim
 */
unsigned int pke_set_modulus_and_pre_monts(unsigned int *modulus, unsigned int *modulus_h, unsigned int *modulus_n0, unsigned int bitLen)
{
    if((NULL == modulus_h) || (NULL == modulus_n0))
    {
        return pke_pre_calc_mont(modulus, bitLen, NULL, NULL);
    }
    else
    {
        return pke_load_modulus_and_pre_monts(modulus, modulus_h, modulus_n0, bitLen);
    }
}

/**
 * @brief      set modulus and pre-calculated mont parameters H(R^2 mod modulus) and n0'(- modulus ^(-1) mod 2^w) for hardware operation
 * @param[in]  modulus           - modulus
 * @param[in]  a                 - integer a
 * @param[in]  b                 - integer b
 * @param[out] out              - bit length of modulus
 * @param[in]  wordLen          - word length of modulus, a, b
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1.modulus must be odd
      -# 2.a, b must less than modulus
      -# 3. wordLen must not be bigger than OPERAND_MAX_WORD_LEN
      -# 4. before calling this function, please make sure the pre-calculated mont arguments
 *        of modulus are located in the right address.
  @endverbatim
 */
unsigned int pke_modmul_internal( volatile unsigned int *modulus,  volatile unsigned int *a,  volatile unsigned int *b, volatile unsigned int *out,
        unsigned int wordLen)
{
    unsigned int ret;

    pke_set_operand_width(wordLen<<5);

    pke_load_operand((volatile unsigned int *)(PKE_B(3,step)), (volatile unsigned int *)modulus, wordLen);    //B3 modulus
    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(3,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_load_operand((volatile unsigned int *)(PKE_A(0,step)), (volatile unsigned int *)a, wordLen);          //A0 a
    pke_load_operand((volatile unsigned int *)(PKE_B(0,step)), (volatile unsigned int *)b, wordLen);          //B0 b
    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_A(0,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_B(0,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_set_microcode(MICROCODE_MODMUL);

    //pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    ret = pke_check_rt_code();
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        pke_read_operand((volatile unsigned int *)(PKE_A(0,step)), (volatile unsigned int *)out, wordLen);                //A0 out

        return PKE_SUCCESS;
    }
}

/**
 * @brief      out = a*b mod modulus
 * @param[in]  modulus           - modulus
 * @param[in]  a                 - integer a
 * @param[in]  b                 - integer b
 * @param[out] out               - out = a*b mod modulus
 * @param[in]  wordLen           - word length of modulus, a, b
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1.modulus must be odd
      -# 2.a, b must less than modulus
      -# 3. wordLen must not be bigger than OPERAND_MAX_WORD_LEN
  @endverbatim
 */
unsigned int pke_modmul( unsigned int *modulus,  unsigned int *a,  unsigned int *b, unsigned int *out, unsigned int wordLen)
{
    unsigned int ret;

    ret = pke_pre_calc_mont(modulus, get_valid_bits((void*)modulus,wordLen), NULL, NULL);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

        return pke_modmul_internal(modulus, a, b, out, wordLen);
    }
}

/**
 * @brief      mod exponent, this could be used for rsa encrypting,decrypting,signing,verifying.
 * @param[in]  modulus           - modulus
 * @param[in]  a                 - integer a
 * @param[in]  b                 - integer b
 * @param[out] out               - out = a^(b) mod modulus
 * @param[in]  mod_wordLen       - word length of modulus and base number
 * @param[in]  exp_wordLen       - word length of exponent
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1.before calling this function, please make sure R^2 mod modulus, the pre-calculated
           mont arguments of modulus is located in the right address
      -# 2.modulus must be odd
      -# 3.please make sure exp_wordLen <= mod_wordLen <= OPERAND_MAX_WORD_LEN
  @endverbatim
 */
unsigned int pke_modexp( volatile unsigned int *modulus,  volatile unsigned int *exponent,  volatile unsigned int *base,//------------------------------------------
        volatile unsigned int *out, unsigned int mod_wordLen, unsigned int exp_wordLen)
{
    unsigned int ret;

    pke_set_operand_width(mod_wordLen<<5);

    pke_load_operand((volatile unsigned int *)(PKE_B(1,step)), (volatile unsigned int *)exponent, exp_wordLen);   //B1 exponent
    if((step/4) > exp_wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(1,step))+exp_wordLen, (step/4)-exp_wordLen);
    }
    else
    {;}

    pke_load_operand((volatile unsigned int *)(PKE_B(3,step)), (volatile unsigned int *)modulus, mod_wordLen);    //B3 modulus
    pke_load_operand((volatile unsigned int *)(PKE_B(0,step)), (volatile unsigned int *)base, mod_wordLen);       //B0 base

    if((step/4) > mod_wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(3,step))+mod_wordLen, (step/4)-mod_wordLen);
        uint32_clear((volatile unsigned int *)(PKE_B(0,step))+mod_wordLen, (step/4)-mod_wordLen);
    }
    else
    {;}

    pke_set_microcode(MICROCODE_MODEXP);

    pke_set_exe_cfg(PKE_EXE_CFG_MOD_EXP);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    ret = pke_check_rt_code();
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        pke_read_operand((volatile unsigned int *)(PKE_A(0,step)), out, mod_wordLen);                //A0 result

        return PKE_SUCCESS;
    }
}

#if 0
/* function: c = a mod b
 * parameters:
 *     a -------------------------- input, integer a
 *     aWordLen ------------------- input, word length of integer
 *     b -------------------------- input, integer b, modulus
 *     b_h ------------------------ input, H parameter of b
 *     b_n1 ----------------------- input, - modulus ^(-1) mod 2^w, here w is 32 actually
 *     bWordLen ------------------- input, word length of integer b and b_h
 *     c -------------------------- output, c = a mod b
 * return: PKE_SUCCESS(success), other(error)
 * caution:
 *     1. b must be odd, and please make sure bWordLen is real word length of b
 *     2. please make sure aWordLen <= 2*OPERAND_MAX_WORD_LEN, bWordLen <= OPERAND_MAX_WORD_LEN,
 *     3. real bit length of a can not be bigger than 2*(real bit length of b)
 */

unsigned int pke_mod(unsigned int *a, unsigned int aWordLen, unsigned int *b, unsigned int *b_h, unsigned int *b_n1, unsigned int bWordLen,
        unsigned int *c)  //----------------------
{
    int32_t flag;
    unsigned int bBitLen, bitLen, tmpLen;
    unsigned int *A1;//, *B2;
    unsigned int B2[MAX_RSA_WORD_LEN/2];
    unsigned int ret;

    flag = uint32_BigNumCmp(a, aWordLen, b, bWordLen);
    if(flag < 0)
    {
        aWordLen = get_valid_words(a, aWordLen);
        uint32_copy(c, a, aWordLen);
        uint32_clear(c+aWordLen, bWordLen-aWordLen);

        return PKE_SUCCESS;
    }
    else if(0 == flag)
    {
        uint32_clear(c, bWordLen);

        return PKE_SUCCESS;
    }
    else
    {;}

    pke_set_operand_width(bWordLen<<5);
    A1 = (unsigned int *)(PKE_A(1, step));
//  B2 = (unsigned int *)(PKE_B(2, step));

    bBitLen = get_valid_bits(b, bWordLen);
    bitLen = bBitLen & 0x1F;

    //get B2 = a high part mod b
    if(bitLen)
    {
        tmpLen = aWordLen-bWordLen+1;
        uint32_copy(B2, a+bWordLen-1, tmpLen);
        Big_Div2n(B2, tmpLen, bitLen);
        
        if(tmpLen < bWordLen)
        {
            uint32_clear(B2+tmpLen, bWordLen-tmpLen);
        }
        else if(uint32_BigNumCmp(B2, bWordLen, b, bWordLen) >= 0)
        {
            ret = pke_sub(B2, b, B2, bWordLen);
            if(PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {;}
        }
        else
        {;}
    }
    else
    {
        tmpLen = aWordLen - bWordLen;
        if(uint32_BigNumCmp(a+bWordLen, tmpLen, b, bWordLen) >= 0)
        {
            ret = pke_sub(a+bWordLen, b, B2, bWordLen);
            if(PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {;}
        }
        else
        {
            uint32_copy(B2, a+bWordLen, tmpLen);
            uint32_clear(B2+tmpLen, bWordLen-tmpLen);
        }
    }
    
    print_BN_buf_U32((unsigned int *)B2, bWordLen, "B2 = a high part mod b");

    //set the pre-calculated mont parameter
    if(NULL == b_h || NULL == b_n1)
    {
        ret = pke_pre_calc_mont(b, bBitLen, NULL, NULL);
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }
    else
    {
        pke_load_pre_calc_mont(b_h, b_n1, bWordLen);
    }
    
    print_BN_buf_U32(b_h, bWordLen, "b_h");
    print_BN_buf_U32(b_n1, 1, "b_n1");
    print_BN_buf_U32((unsigned int *)(PKE_A(3,0x24)), 0x24/4, "A3  H");
    print_BN_buf_U32((unsigned int *)(PKE_B(4,0x24)), 1, "B4 n0 PKE RAMB not support read");

    //get A1 = 1000...000 mod b
    uint32_clear(A1, bWordLen);
    if(bitLen)
    {
        A1[bWordLen-1] = 1<<(bitLen);
    }
    else
    {;}

    print_BN_buf_U32((unsigned int *)A1, bWordLen, "A1");
    print_BN_buf_U32((unsigned int *)b, bWordLen, "b");
    ret = pke_sub(A1, b, A1, bWordLen);
    print_BN_buf_U32((unsigned int *)A1, Ed25519_WORD_LEN+1, "A1=A1-b");
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //get B2 = a_high * 1000..000 mod b
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    print_BN_buf_U32((unsigned int *)b, bWordLen, "b");
    print_BN_buf_U32((unsigned int *)A1, bWordLen, "A1");
    print_BN_buf_U32((unsigned int *)B2, bWordLen, "B2");
    ret = pke_modmul_internal(b, A1, B2, B2, bWordLen);
    print_BN_buf_U32((unsigned int *)B2, Ed25519_WORD_LEN+1, "B2=A1*B2 mod b");
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //get A1 = a low part mod b
    if(bitLen)
    {
        uint32_copy(A1, a, bWordLen);
        A1[bWordLen-1] &= ((1<<(bitLen))-1);
        if(uint32_BigNumCmp(A1, bWordLen, b, bWordLen) >= 0)
        {
            ret = pke_sub(A1, b, A1, bWordLen);
            if(PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {;}
        }
        else
        {;}
    }
    else
    {
        if(uint32_BigNumCmp(a, bWordLen, b, bWordLen) >= 0)
        {
            ret = pke_sub(a, b, A1, bWordLen);
            if(PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {;}
        }
        else
        {
            A1 = a;
        }
    }
    
    print_BN_buf_U32((unsigned int *)A1, bWordLen, "A1 = a low part mod b");

    print_BN_buf_U32((unsigned int *)b, bWordLen, "b");
    print_BN_buf_U32((unsigned int *)A1, bWordLen, "A1");
    print_BN_buf_U32((unsigned int *)B2, bWordLen, "B2");
    ret = pke_modadd(b, A1, B2, c, bWordLen);
    print_BN_buf_U32((unsigned int *)c, bWordLen, "c=A1+B2 mod b");
    
    return ret;
}
#else

/**
 * @brief      c = a mod b
 * @param[in]  a           - modulus
 * @param[in]  aWordLen         - integer a
 * @param[in]  b                - integer b
 * @param[in]  b_h              - out = a^(b) mod modulus
 * @param[in]  b_n1             - word length of modulus and base number
 * @param[in]  bWordLen         - word length of exponent
 * @param[out] bWordLen         - word length of exponent
 * @return     PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1.b must be odd, and please make sure bWordLen is real word length of b
      -# 2.please make sure aWordLen <= 2*OPERAND_MAX_WORD_LEN, bWordLen <= OPERAND_MAX_WORD_LEN,
      -# 3.real bit length of a can not be bigger than 2*(real bit length of b)
  @endverbatim
 */
unsigned int pke_mod(unsigned int *a, unsigned int aWordLen, unsigned int *b, volatile unsigned int *b_h, volatile unsigned int *b_n1, unsigned int bWordLen,
        unsigned int *c)  //----------------------
{
    int32_t flag;
    unsigned int bBitLen, bitLen, tmpLen;
    //unsigned int *A1;//, *B2;
    unsigned int A1[MAX_RSA_WORD_LEN];
    unsigned int B2[MAX_RSA_WORD_LEN/2];
    unsigned int ret;
    unsigned int *tmp_p = A1;

    flag = uint32_BigNumCmp(a, aWordLen, b, bWordLen);
    if(flag < 0)
    {
        aWordLen = get_valid_words(a, aWordLen);
        uint32_copy(c, a, aWordLen);
        uint32_clear(c+aWordLen, bWordLen-aWordLen);

        return PKE_SUCCESS;
    }
    else if(0 == flag)
    {
        uint32_clear(c, bWordLen);

        return PKE_SUCCESS;
    }
    else
    {;}

    pke_set_operand_width(bWordLen<<5);
    //A1 = (unsigned int *)(PKE_A(1, step));
//  B2 = (unsigned int *)(PKE_B(2, step));

    bBitLen = get_valid_bits(b, bWordLen);
    bitLen = bBitLen & 0x1F;

    //get B2 = a high part mod b
    if(bitLen)
    {
        tmpLen = aWordLen-bWordLen+1;
        uint32_copy(B2, a+bWordLen-1, tmpLen);
        Big_Div2n(B2, tmpLen, bitLen);
        
        if(tmpLen < bWordLen)
        {
            uint32_clear(B2+tmpLen, bWordLen-tmpLen);
        }
        else if(uint32_BigNumCmp(B2, bWordLen, b, bWordLen) >= 0)
        {
            ret = pke_sub(B2, b, B2, bWordLen);
            if(PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {;}
        }
        else
        {;}
    }
    else
    {
        tmpLen = aWordLen - bWordLen;
        if(uint32_BigNumCmp(a+bWordLen, tmpLen, b, bWordLen) >= 0)
        {
            ret = pke_sub(a+bWordLen, b, B2, bWordLen);
            if(PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {;}
        }
        else
        {
            uint32_copy(B2, a+bWordLen, tmpLen);
            uint32_clear(B2+tmpLen, bWordLen-tmpLen);
        }
    }
    
//    print_BN_buf_U32((unsigned int *)B2, bWordLen, "B2 = a high part mod b");

    //set the pre-calculated mont parameter
    if(NULL == b_h || NULL == b_n1)
    {
        ret = pke_pre_calc_mont(b, bBitLen, NULL, NULL);
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }
    else
    {
        pke_load_pre_calc_mont((volatile unsigned int *)b_h, (volatile unsigned int *)b_n1, bWordLen);
    }
    //get A1 = 1000...000 mod b
    uint32_clear(A1, bWordLen);
    if(bitLen)
    {
        A1[bWordLen-1] = 1<<(bitLen);
    }
    else
    {;}
    ret = pke_sub(A1, b, A1, bWordLen);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //get B2 = a_high * 1000..000 mod b
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_modmul_internal(b, A1, B2, B2, bWordLen);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //get A1 = a low part mod b
    if(bitLen)
    {
        uint32_copy(A1, a, bWordLen);
        A1[bWordLen-1] &= ((1<<(bitLen))-1);
        if(uint32_BigNumCmp(A1, bWordLen, b, bWordLen) >= 0)
        {
            ret = pke_sub(A1, b, A1, bWordLen);
            if(PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {;}
        }
        else
        {;}
    }
    else
    {
        if(uint32_BigNumCmp(a, bWordLen, b, bWordLen) >= 0)
        {
            ret = pke_sub(a, b, A1, bWordLen);
            if(PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {;}
        }
        else
        {
            //A1 = a;
            tmp_p = a;
        }
    }
    
    ret = pke_modadd(b, tmp_p, B2, c, bWordLen);
    return ret;
}
#endif


/********************************** ECCp functions *************************************/

/**
 * @brief      ECCP curve point mul(random point), Q=[k]P
 * @param[in]  curve           - eccp_curve_t curve struct pointer
 * @param[in]  k               - scalar
 * @param[in]  Px              - x coordinate of point P
 * @param[in]  Py              - y coordinate of point P
 * @param[in]  Qx              - x coordinate of point Q
 * @param[out] Qy              - y coordinate of point Q
 * @return     PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1.please make sure k in [1,n-1], n is order of ECCP curve
      -# 2.please make sure input point P is on the curve
      -# 3.please make sure bit length of the curve is not bigger than ECCP_MAX_BIT_LEN
  @endverbatim
 */
unsigned int eccp_pointMul(eccp_curve_t *curve, unsigned int *k, unsigned int *Px, unsigned int *Py,
                      unsigned int *Qx, unsigned int *Qy)
{
    unsigned int wordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    unsigned int ret;

    pke_set_operand_width(curve->eccp_p_bitLen);

    pke_load_operand((volatile unsigned int *)(PKE_B(3,step)), curve->eccp_p, wordLen);          //B3 p
    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(3,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    //set ecc_p_h & ecc_p_n1
    if((NULL == curve->eccp_p_h) || (NULL == curve->eccp_p_n1))
    {
        pke_pre_calc_mont((volatile unsigned int *)(PKE_B(3,step)), curve->eccp_p_bitLen, NULL, NULL);
    }
    else
    {
        pke_load_pre_calc_mont(curve->eccp_p_h, curve->eccp_p_n1, wordLen);
    }

    pke_load_operand((volatile unsigned int *)(PKE_B(0,step)), Px, wordLen);                     //B0 Px
    pke_load_operand((volatile unsigned int *)(PKE_B(1,step)), Py, wordLen);                     //B1 Py
    pke_load_operand((volatile unsigned int *)(PKE_A(5,step)), curve->eccp_a, wordLen);          //A5 a
    pke_load_operand((volatile unsigned int *)(PKE_B(2,step)), k, wordLen);                      //B2 k
    pke_load_operand((volatile unsigned int *)(PKE_B(5,step)), curve->eccp_n, wordLen);          //B5 n

    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(0,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_B(1,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_A(5,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_B(2,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_B(5,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_set_microcode(MICROCODE_PMUL);

    pke_set_exe_cfg(PKE_EXE_ECCP_POINT_MUL);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    ret = pke_check_rt_code();
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    pke_read_operand((volatile unsigned int *)(PKE_A(0,step)), Qx, wordLen);                     //A0 Qx
    if(NULL != Qy)
    {
        pke_read_operand((volatile unsigned int *)(PKE_A(1,step)), Qy, wordLen);                 //A1 Qy
    }
    else
    {;}

    return PKE_SUCCESS;
}

/**
 * @brief      ECCP curve point add, Q=P1+P2
 * @param[in]  curve            - eccp_curve_t curve struct pointer
 * @param[in]  P1x              - x coordinate of point P1
 * @param[in]  P1y              - y coordinate of point P1
 * @param[in]  P2x              - x coordinate of point P2
 * @param[in]  P2y              - y coordinate of point P2
 * @param[out] Qx               - x coordinate of point Q=P1+P2
 * @param[out] Qy               - y coordinate of point Q=P1+P2
 * @return     PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1.please make sure input point P1 and P2 are both on the curve
      -# 2.please make sure bit length of the curve is not bigger than ECCP_MAX_BIT_LEN
  @endverbatim
 */
unsigned int eccp_pointAdd(eccp_curve_t *curve, unsigned int *P1x, unsigned int *P1y, unsigned int *P2x, unsigned int *P2y,
                      unsigned int *Qx, unsigned int *Qy)
{
    unsigned int wordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    unsigned int ret;

    pke_set_operand_width(curve->eccp_p_bitLen);

    pke_load_operand((volatile unsigned int *)(PKE_B(3,step)), curve->eccp_p, wordLen);          //B3 p
    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(3,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    //set ecc_p_h & ecc_p_n1
    if((NULL == curve->eccp_p_h) || (NULL == curve->eccp_p_n1))
    {
        pke_pre_calc_mont((volatile unsigned int *)(PKE_B(3,step)), curve->eccp_p_bitLen, NULL, NULL);
    }
    else
    {
        pke_load_pre_calc_mont(curve->eccp_p_h, curve->eccp_p_n1, wordLen);
    }

    //pke_pre_calc_mont() may cover A1, so load A1(P1x) here
    pke_load_operand((volatile unsigned int *)(PKE_A(0,step)), P1x, wordLen);                    //A0 P1x
    pke_load_operand((volatile unsigned int *)(PKE_A(1,step)), P1y, wordLen);                    //A1 P1y
    pke_load_operand((volatile unsigned int *)(PKE_B(0,step)), P2x, wordLen);                    //B0 P2x
    pke_load_operand((volatile unsigned int *)(PKE_B(1,step)), P2y, wordLen);                    //B1 P2y
    pke_load_operand((volatile unsigned int *)(PKE_A(5,step)), curve->eccp_a, wordLen);          //A5 a

    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_A(0,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_A(1,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_B(0,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_B(1,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_A(5,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_set_microcode(MICROCODE_PADD);

    pke_set_exe_cfg(PKE_EXE_ECCP_POINT_ADD);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    ret = pke_check_rt_code();
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    pke_read_operand((volatile unsigned int *)(PKE_A(0,step)), Qx, wordLen);                     //A0 Qx
    if(Qy != NULL)
    {
        pke_read_operand((volatile unsigned int *)(PKE_A(1,step)), Qy, wordLen);                 //A1 Qy
    }
    else
    {;}

    return PKE_SUCCESS;
}

#ifdef ECCP_POINT_DOUBLE

/**
 * @brief      ECCP curve point double, Q=[2]P
 * @param[in]  curve            - eccp_curve_t curve struct pointer
 * @param[in]  Px               - x coordinate of point P
 * @param[in]  Py               - y coordinate of point P
 * @param[out] Qx               - x coordinate of point Q=[2]P
 * @param[out] Qy               - y coordinate of point Q=[2]P
 * @return     PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1.please make sure input point P is on the curve
      -# 2.please make sure bit length of the curve is not bigger than ECCP_MAX_BIT_LEN
  @endverbatim
 */
unsigned int eccp_pointDouble(eccp_curve_t *curve, unsigned int *Px, unsigned int *Py, unsigned int *Qx, unsigned int *Qy)
{
    unsigned int wordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    unsigned int ret;

    pke_set_operand_width(curve->eccp_p_bitLen);

    pke_load_operand((unsigned int *)(PKE_B(3,step)), curve->eccp_p, wordLen);          //B3 p
    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)(PKE_B(3,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    //set ecc_p_h & ecc_p_n1
    if((NULL == curve->eccp_p_h) || (NULL == curve->eccp_p_n1))
    {
        pke_pre_calc_mont((unsigned int *)(PKE_B(3,step)), curve->eccp_p_bitLen, NULL, NULL);
    }
    else
    {
        pke_load_pre_calc_mont(curve->eccp_p_h, curve->eccp_p_n1, wordLen);
    }

    //pke_pre_calc_mont() may cover A1, so load A1(Px) and other paras here
    pke_load_operand((unsigned int *)(PKE_A(0,step)), Px, wordLen);                     //A0 Px
    pke_load_operand((unsigned int *)(PKE_A(1,step)), Py, wordLen);                     //A1 Py
    pke_load_operand((unsigned int *)(PKE_A(5,step)), curve->eccp_a, wordLen);          //A5 a

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)(PKE_A(0,step))+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)(PKE_A(1,step))+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)(PKE_A(5,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_set_microcode(MICROCODE_PDBL);

    pke_set_exe_cfg(PKE_EXE_ECCP_POINT_DBL);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    ret = pke_check_rt_code();
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        pke_read_operand((unsigned int *)(PKE_A(0,step)), Qx, wordLen);                     //A0 Qx
        pke_read_operand((unsigned int *)(PKE_A(1,step)), Qy, wordLen);                     //A1 Qy

        return PKE_SUCCESS;
    }
}
#endif

/**
 * @brief      check whether the input point P is on ECCP curve or not
 * @param[in]  curve            - eccp_curve_t curve struct pointer
 * @param[in]  Px               - x coordinate of point P
 * @param[in]  Py               - y coordinate of point P
 * @return     PKE_SUCCESS(success, on the curve), other(error or not on the curve)
 * @note
  @verbatim
      -# 1.please make sure bit length of the curve is not bigger than ECCP_MAX_BIT_LEN
      -# 2.after calculation, A1 and A2 will be changed!
  @endverbatim
 */
unsigned int eccp_pointVerify(eccp_curve_t *curve, unsigned int *Px, unsigned int *Py)
{
    unsigned int wordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    unsigned int ret;

    pke_set_operand_width(curve->eccp_p_bitLen);

    pke_load_operand((volatile unsigned int *)(PKE_B(3,step)), curve->eccp_p, wordLen);          //B3 p
    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(3,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    //set ecc_p_h & ecc_p_n1
    if((NULL == curve->eccp_p_h) || (NULL == curve->eccp_p_n1))
    {
        pke_pre_calc_mont((volatile unsigned int *)(PKE_B(3,step)), curve->eccp_p_bitLen, NULL, NULL);
    }
    else
    {
        pke_load_pre_calc_mont(curve->eccp_p_h, curve->eccp_p_n1, wordLen);
    }

    //pke_pre_calc_mont() may cover A1, so load A1(Px) and other paras here
    pke_load_operand((volatile unsigned int *)(PKE_B(0,step)), Px, wordLen);                     //B0 Px
    pke_load_operand((volatile unsigned int *)(PKE_B(1,step)), Py, wordLen);                     //B1 Py
    pke_load_operand((volatile unsigned int *)(PKE_A(5,step)), curve->eccp_a, wordLen);          //A5 a
    pke_load_operand((volatile unsigned int *)(PKE_A(4,step)), curve->eccp_b, wordLen);          //A4 b

    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)(PKE_B(0,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_B(1,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_A(5,step))+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)(PKE_A(4,step))+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_set_microcode(MICROCODE_PVER);

    pke_set_exe_cfg(PKE_EXE_ECCP_POINT_VER);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    ret = pke_check_rt_code();
    if(0 != ret)
    {
        return ret;
    }
    else
    {
        return PKE_SUCCESS;
    }
}

/**
 * @brief      get ECCP public key from private key(the key pair could be used in SM2/ECDSA/ECDH)
 * @param[in]  curve            - eccp_curve_t curve struct pointer
 * @param[in]  priKey           - x private key, big-endian
 * @param[out] pubKey           - y public key, big-endian
 * @return     PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1.please make sure bit length of the curve is not bigger than ECCP_MAX_BIT_LEN
  @endverbatim
 */
unsigned int eccp_get_pubkey_from_prikey(eccp_curve_t *curve, unsigned char *priKey, unsigned char *pubKey)
{
    unsigned int nByteLen = GET_BYTE_LEN(curve->eccp_n_bitLen);
    unsigned int nWordLen = GET_WORD_LEN(curve->eccp_n_bitLen);
    unsigned int pByteLen = GET_BYTE_LEN(curve->eccp_p_bitLen);
    unsigned int pWordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    unsigned int k[ECCP_MAX_WORD_LEN]={0};
    unsigned int Px[ECCP_MAX_WORD_LEN], Py[ECCP_MAX_WORD_LEN];
    unsigned int ret;

    Px[pWordLen - 1U] = 0u;
    Py[pWordLen - 1U] = 0u;
    
    pke_set_operand_width(curve->eccp_p_bitLen);

    reverse_byte_array(priKey, (unsigned char *)k, nByteLen);

    //make sure k in [1, n-1]
    if(uint32_BigNum_Check_Zero(k, nWordLen))
    {
        return PKE_ZERO_ALL;
    }
    else if(uint32_BigNumCmp(k, nWordLen, curve->eccp_n, nWordLen) >= 0)
    {
        return PKE_INTEGER_TOO_BIG;
    }
    else
    {;}

#ifdef SUPPORT_SM2
    if(curve == sm2_curve)
    {
        if((k[0] == sm2_curve->eccp_n[0]-1) && (0 == uint32_BigNumCmp(k+1, nWordLen-1, (curve->eccp_n)+1, nWordLen-1)))
        {
            return PKE_INTEGER_TOO_BIG;
        }
        else
        {;}
    }
    else
    {;}
#endif

    //get pubKey
    ret = eccp_pointMul(curve, k, curve->eccp_Gx, curve->eccp_Gy, Px, Py);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        reverse_byte_array((unsigned char *)Px, pubKey, pByteLen);
        reverse_byte_array((unsigned char *)Py, pubKey+pByteLen, pByteLen);

        return PKE_SUCCESS;
    }
}

/**
 * @brief      get ECCP key pair(the key pair could be used in SM2/ECDSA/ECDH)
 * @param[in]  curve            - eccp_curve_t curve struct pointer
 * @param[out] priKey           - x private key, big-endian
 * @param[out] pubKey           - y public key, big-endian
 * @return     PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1.please make sure bit length of the curve is not bigger than ECCP_MAX_BIT_LEN
  @endverbatim
 */
unsigned int eccp_getkey(eccp_curve_t *curve, unsigned char *priKey, unsigned char *pubKey)
{
    unsigned int tmpLen;
    unsigned int nByteLen = GET_BYTE_LEN(curve->eccp_n_bitLen);
    unsigned int ret;

ECCP_GETKEY_LOOP:

    ret = get_rand(priKey, nByteLen);
    if(TRNG_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //make sure k has the same bit length as n
    tmpLen = (curve->eccp_n_bitLen)&7;
    if(tmpLen)
    {
        priKey[0] &= (1<<(tmpLen))-1;
    }
    else
    {;}

    ret = eccp_get_pubkey_from_prikey(curve, priKey, pubKey);
    if(PKE_ZERO_ALL == ret || PKE_INTEGER_TOO_BIG == ret)
    {
        goto ECCP_GETKEY_LOOP;
    }
    else
    {
        return ret;
    }
}

/****************************** ECCp functions finished ********************************/





unsigned int Big_Mul2n(unsigned int a[], int32_t aWordLen, unsigned char n)
{
    unsigned char flag=0;
    int32_t i;

    //aWordLen = Get_WordLen(a, aWordLen);

    if(!aWordLen)
        return 0;

    if(a[aWordLen-1]&(0xFFFFFFFF<<(32-n)))    // ����n�����أ���Ҫ����λ������ǰ����������n�����أ������λ
    {
        a[aWordLen]=a[aWordLen-1]>>(32-n);
        flag=1;
    }

    for(i=aWordLen-1; i>0; i--)
    {
        a[i] <<= n;
        a[i] |= (a[i-1]>>(32-n));
    }
    a[i] <<= n;

    if(flag)
    {
        return aWordLen+1;
    }

    return aWordLen;
}

/**
 * @brief      get aimed bit value of big integer a
 * @param[in]  a                - big integer a
 * @param[in]  bitLen           - x private key, big-endian
 * @return      bit value of aimed bit
 * @note
  @verbatim
      -# 1.make sure bitLen > 0
  @endverbatim
 */
unsigned char Get_BitValue(unsigned int a[], unsigned int bitLen)
{
    bitLen--;
    if(a[(bitLen)>>5]&(1<<(bitLen&31)))
    {
        return 1;
    }
    return 0;
}

unsigned int get_J0(unsigned int n0)
{
    int32_t i;
    unsigned int t = n0;

    for(i=30;i>0;i--)
    {
        t = t*t;
        t = t*n0;
    }

    return (0-t);
}

//out = mont(a, b) = a*b*R^(-1) mod n
unsigned int get_H(unsigned int *n, unsigned int *H, unsigned int wordLen)
{
    unsigned int bitLen, tmpLen, i;
    (void)bitLen;
    unsigned int t[MAX_RSA_WORD_LEN],t1[MAX_RSA_WORD_LEN];

    wordLen = get_valid_words(n, wordLen);
    bitLen = get_valid_bits(n, wordLen);

    //t = R   //(w=32)
    uint32_clear(t, wordLen);
    t[wordLen] = 1;
    tmpLen = wordLen+1;

    //get t = R mod n
    uint32_copy(t1, n, wordLen);
    t1[wordLen] = 0;
    while(uint32_BigNumCmp(t, tmpLen, t1, wordLen) >= 0)
    {
        pke_sub(t, t1, t, tmpLen);
    }

    i = get_J0(n[0]);
    pke_load_pre_calc_mont(t, &i, wordLen);//print_BN_buf_U32(t, wordLen, "R");print_BN_buf_U32(&i, 1, "J0");

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_MONT);

    //get 2R
    tmpLen = Big_Mul2n(t, wordLen, 1);
    if(uint32_BigNumCmp(t, tmpLen, n, wordLen) >= 0)
    {
        uint32_copy(t1, n, wordLen);
        t1[wordLen] = 0;

        pke_sub(t, t1, t, tmpLen);
    }
    uint32_copy(t1, t, wordLen);//print_BN_buf_U32(t, wordLen, "2R");

    uint32_copy((volatile unsigned int *)(PKE_A(0,step)), t, wordLen);//print_BN_buf_U32((unsigned int *)(PKE_A(0,step)), wordLen, "A0");print_BN_buf_U32(n, wordLen, "n");
    uint32_copy((volatile unsigned int *)(PKE_B(3,step)), n, wordLen);//print_BN_buf_U32((unsigned int *)(PKE_B(3,step)), wordLen, "n");

    tmpLen = wordLen*32;  //tmpLen = RbitLen-1
    for(i=get_valid_bits(&tmpLen, 1)-1; i>0; i--)
    {
        //pke_modmul_internal(n, t, t, t, wordLen);
        pke_modmul_internal((volatile unsigned int *)(PKE_B(3,step)), (volatile unsigned int *)(PKE_A(0,step)), (volatile unsigned int *)(PKE_A(0,step)), (volatile unsigned int *)(PKE_A(0,step)), wordLen);

        if(Get_BitValue(&tmpLen, i))
        {
            //pke_modmul_internal(n, t, t1, t, wordLen);
            pke_modmul_internal((volatile unsigned int *)(PKE_B(3,step)), (volatile unsigned int *)(PKE_A(0,step)), t1, (volatile unsigned int *)(PKE_A(0,step)), wordLen);
        }
    }

    //uint32_copy((unsigned int *)(PKE_A(3,step)), t, wordLen);
    uint32_copy((volatile unsigned int *)(PKE_A(3,step)), (volatile unsigned int *)(PKE_A(0,step)), wordLen);
    if(H)
        //uint32_copy(H, t, wordLen);
        uint32_copy(H, (volatile unsigned int *)(PKE_A(0,step)), wordLen);

    return 0;
}


#ifdef SUPPORT_C25519
#if 0
/**************************** X25519 & Ed25519 functions *******************************/

/**
 * @brief      c25519 point mul(random point), Q=[k]P
 * @param[in]  curve            - c25519 curve struct pointer
 * @param[in]  k                - scalar
 * @param[in]  Pu               - u coordinate of point P
 * @param[in]  Qu               - u coordinate of point Q
 * @return     PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1.please make sure input point P is on the curve
      -# 2.even if the input point P is valid, the output may be infinite point, in this case return error.
      -# 3.please make sure the curve is c25519
  @endverbatim
 */
unsigned int x25519_pointMul(mont_curve_t *curve, unsigned int *k, unsigned int *Pu, unsigned int *Qu)
{
    unsigned int wordLen = GET_WORD_LEN(curve->p_bitLen);
    unsigned int ret;

    //set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->p, curve->p_h, curve->p_n0, curve->p_bitLen);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    pke_load_operand((unsigned int *)PKE_A(0,step), Pu, wordLen);                     //A0 Pu
    pke_load_operand((unsigned int *)PKE_B(0,step), curve->a24, wordLen);             //B0 a24
    pke_load_operand((unsigned int *)PKE_A(4,step), k, wordLen);                      //A4 k

    if((step/4) > wordLen)
    {
        uint32_clear((unsigned int *)PKE_A(0,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_B(0,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_A(4,step)+wordLen, (step/4)-wordLen);
        uint32_clear((unsigned int *)PKE_B(3,step)+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_C25519_PMUL);
    if(PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(PKE_A(0,step)), wordLen<<2);
        get_rand_fast((unsigned char *)(PKE_A(4,step)), wordLen<<2);
        get_rand_fast((unsigned char *)(PKE_B(0,step)), wordLen<<2);
        get_rand_fast((unsigned char *)(PKE_B(3,step)), wordLen<<2);
#endif
        return ret;
    }
    else
    {;}

    pke_read_operand((unsigned int *)PKE_A(1,step), Qu, wordLen);                     //A1 Qu

    return PKE_SUCCESS;
}
#endif


#if 0

/**
 * @brief      out = a^b mod n
 * @param[in]  a            - base number, 8 words
 * @param[in]  b            - exponent number, 8 words
 * @param[in]  n            - modulus number, 8 words
 * @param[out] out          - out = a^b mod n
 * @return     PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1.please make sure n is odd, b is not zero
      -# 2.this function is used in Ed25519 to decode point
  @endverbatim
 */
unsigned int mod_exp(unsigned int a[8], unsigned int b[8], unsigned int n[8], unsigned int out[8])
{
    unsigned int t[8];
    int32_t cfg_bak, bitLen;
    unsigned int ret;

    pke_pre_calc_mont(n, 256, NULL, NULL);

    cfg_bak = PKE_EXE_CONF;
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_MONT);

    //t = A0 = aR mod n
    ret = pke_modmul_internal(a, (unsigned int *)(PKE_A(3,step)), t, Ed25519_WORD_LEN);     //A3: R^2 mod n
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    bitLen = get_valid_bits(b, Ed25519_WORD_LEN);
    bitLen -= 2;
    for(; bitLen>=0; bitLen--)
    {
        ret = pke_modmul_internal((unsigned int *)(PKE_A(0,step)), (unsigned int *)(PKE_A(0,step)), (unsigned int *)(PKE_A(0,step)), Ed25519_WORD_LEN);
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else if(b[bitLen/32] & (1<<(bitLen&31)))
        {
            ret = pke_modmul_internal((unsigned int *)(PKE_A(0,step)), t, (unsigned int *)(PKE_A(0,step)), Ed25519_WORD_LEN);
            if(PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {;}
        }
        else
        {;}
    }

    //t = 1
    pke_set_operand_uint32_value(t, 8, 1);

    //get result
    ret = pke_modmul_internal((unsigned int *)(PKE_A(0,step)), t, out, Ed25519_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    pke_set_exe_cfg(cfg_bak);

    return PKE_SUCCESS;
}
#endif

/**
 * @brief      Ed25519 decode point
 * @param[in]  in_y            - encoded Ed25519 point
 * @param[out] out_x           - x coordinate of input point
 * @param[out] out_y           - y coordinate of input point
 * @return     PKE_SUCCESS(success), other(error)
 */
unsigned int ed25519_decode_point(unsigned char in_y[32], unsigned char out_x[32], unsigned char out_y[32])
{
    unsigned int u[Ed25519_WORD_LEN];
    unsigned int v[Ed25519_WORD_LEN];
    unsigned int t[Ed25519_WORD_LEN]={0};
    unsigned int t2[Ed25519_WORD_LEN];
    unsigned int t3[Ed25519_WORD_LEN];
    unsigned int ret;

    //get y
    memcpy_(u, in_y, Ed25519_BYTE_LEN);
    u[Ed25519_WORD_LEN-1] &= 0x7FFFFFFF;

    //make sure y < prime p
    if(uint32_BigNumCmp(u, Ed25519_WORD_LEN, ed25519->p, Ed25519_WORD_LEN) >= 0)
    {
        return PKE_INVALID_INPUT;
    }
    else
    {;}

    //set type
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    //set pre-calculated paras
    ret = pke_set_modulus_and_pre_monts(ed25519->p, ed25519->p_h, ed25519->p_n0, ed25519->p_bitLen);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = pke_modmul_internal(ed25519->p, u, u, v, Ed25519_WORD_LEN);                        //v = y^2
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    t[0]=1;
    ret = pke_modsub(ed25519->p, v, t, u, Ed25519_WORD_LEN);                     //u = y^2 - 1
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = pke_modmul_internal(ed25519->p, ed25519->d, v, v, Ed25519_WORD_LEN);               //v = d*y^2
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = pke_modadd(ed25519->p, v, t, v, Ed25519_WORD_LEN);                     //v = d*y^2 + 1
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = pke_modmul_internal(ed25519->p, v, v, t2, Ed25519_WORD_LEN);                       //t2 = v^2
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = pke_modmul_internal(ed25519->p, v, t2, t3, Ed25519_WORD_LEN);                      //t3 = v^3
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = pke_modmul_internal(ed25519->p, t3, u, t, Ed25519_WORD_LEN);                       //t = u*v^3
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = pke_modmul_internal(ed25519->p, t2, t2, t2, Ed25519_WORD_LEN);                     //t2 = v^4
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = pke_modmul_internal(ed25519->p, t2, t3, t2, Ed25519_WORD_LEN);                     //t2 = v^7
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = pke_modmul_internal(ed25519->p, t2, u, t2, Ed25519_WORD_LEN);                      //t2 = u*v^7
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //t3 = (p-5)/8
    uint32_copy(t3, ed25519->p, Ed25519_WORD_LEN);
    t3[0] -= 5;
    Big_Div2n(t3, Ed25519_WORD_LEN, 3);

    //t2 = (u*v^7 )^((p-5)/8)
#if 0
    ret = mod_exp(t2, t3, ed25519->p, t2);
#else
    ret = pke_modexp(ed25519->p, t3, t2, t2, Ed25519_WORD_LEN, Ed25519_WORD_LEN);
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
#endif
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = pke_modmul_internal(ed25519->p, t2, t, t, Ed25519_WORD_LEN);                       //t = x = (u*v^3)*(u*v^7 )^((p-5)/8)
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = pke_modmul_internal(ed25519->p, t, t, t2, Ed25519_WORD_LEN);                       //t2 = x^2
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = pke_modmul_internal(ed25519->p, t2, v, t2, Ed25519_WORD_LEN);                      //t2 = v*x^2
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    if(0 == uint32_BigNumCmp(t2, Ed25519_WORD_LEN, u, Ed25519_WORD_LEN))         //if v x^2 = u (mod p), x is a square root.
    {
        goto result;
    }
    else
    {;}

    ret = pke_sub(ed25519->p, u, t3, Ed25519_WORD_LEN);                          //t3 = -u mod p
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else if(0 == uint32_BigNumCmp(t2, Ed25519_WORD_LEN, t3, Ed25519_WORD_LEN))
    {
        //v = (p-1)/4
        uint32_copy(v, ed25519->p, Ed25519_WORD_LEN);
        v[0] -= 1;
        Big_Div2n(v, Ed25519_WORD_LEN, 2);

        //t2 = 2
        pke_set_operand_uint32_value(t2, Ed25519_WORD_LEN, 2);

        //u = 2^((p-1)/4)
#if 0
        ret = mod_exp(t2, v, ed25519->p, u);
#else
        ret = pke_modexp(ed25519->p, v, t2, u, Ed25519_WORD_LEN, Ed25519_WORD_LEN);
        pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
#endif
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ret = pke_modmul_internal(ed25519->p, t, u, t, Ed25519_WORD_LEN);                    //t = x*(2^((p-1)/4))
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        goto result;
    }
    else
    {;}

    return PKE_INVALID_INPUT;   //root not exist

result:

    //if x=0 and x is odd, decode fail
    if(uint32_BigNum_Check_Zero(t, Ed25519_WORD_LEN) && (in_y[Ed25519_BYTE_LEN-1] & 0x80))
    {
        return PKE_INVALID_INPUT;
    }
    else
    {;}

    //get out_x
    if((unsigned char)((t[0]&1)<<7) == (in_y[Ed25519_BYTE_LEN-1] & 0x80))
    {
        memcpy_(out_x, t, Ed25519_BYTE_LEN);
    }
    else
    {
        ret = pke_sub(ed25519->p, t, v, Ed25519_WORD_LEN);                             //v = -x mod p
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            memcpy_(out_x, v, Ed25519_BYTE_LEN);
        }
    }

    //get out_y
    memcpy_(out_y, in_y, Ed25519_BYTE_LEN);
    out_y[Ed25519_BYTE_LEN-1] &= 0x7F;

    return PKE_SUCCESS;
}

/**
 * @brief      edwards25519 curve point mul(random point), Q=[k]P
 * @param[in]  curve        - edwards25519 curve struct pointer
 * @param[in]  k            - scalar
 * @param[in]  Px           - x coordinate of point P
 * @param[in]  Py           - y coordinate of point P
 * @param[out] Qx           - x coordinate of point Q
 * @param[out] Py           - y coordinate of point Q
 * @return     PKE_SUCCESS(success), other(error)
 */
unsigned int ed25519_pointMul(edward_curve_t *curve, unsigned int *k, unsigned int *Px, unsigned int *Py,
        unsigned int *Qx, unsigned int *Qy)
{
    unsigned int wordLen = GET_WORD_LEN(curve->p_bitLen);
    unsigned int ret;

    //set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->p, curve->p_h, curve->p_n0, curve->p_bitLen);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    pke_load_operand((volatile unsigned int *)PKE_A(1,step), Px, wordLen);                    //A1 Px
    pke_load_operand((volatile unsigned int *)PKE_A(2,step), Py, wordLen);                    //A2 Py
    pke_load_operand((volatile unsigned int *)PKE_B(0,step), curve->d, wordLen);              //B0 d
    pke_load_operand((volatile unsigned int *)PKE_A(0,step), k, wordLen);                     //A0 k

    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)PKE_A(1,step)+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)PKE_A(2,step)+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)PKE_B(0,step)+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)PKE_A(0,step)+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    pke_set_microcode(MICROCODE_Ed25519_PMUL);
    pke_clear_interrupt();
    pke_start();
    pke_wait_till_done();
    ret = pke_check_rt_code();
    // ret = pke_set_micro_code_start_wait_return_code(MICROCODE_Ed25519_PMUL);
    if(PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(PKE_A(0,step)), wordLen<<2);
        get_rand_fast((unsigned char *)(PKE_A(1,step)), wordLen<<2);
        get_rand_fast((unsigned char *)(PKE_A(2,step)), wordLen<<2);
        get_rand_fast((unsigned char *)(PKE_B(0,step)), wordLen<<2);
#endif
        return ret;
    }
    else
    {;}

    pke_read_operand((volatile unsigned int *)PKE_A(1,step), Qx, wordLen);                    //A1 Qx
    if(NULL != Qy)
    {
        pke_read_operand((volatile unsigned int *)PKE_A(2,step), Qy, wordLen);                //A2 Qx
    }
    else
    {;}

    return PKE_SUCCESS;
}

/**
 * @brief      edwards25519 point add, Q=P1+P2
 * @param[in]  curve        - edwards25519 curve struct pointer
 * @param[in]  P1x           - x coordinate of point P1
 * @param[in]  P1y           - y coordinate of point P1
 * @param[in]  P2x           - x coordinate of point P2
 * @param[in]  P2y           - y coordinate of point P2
 * @param[out] Qx            - x coordinate of point Q=P1+P2
 * @param[out] Py            - y coordinate of point Q=P1+P2
 * @return     PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1. please make sure input point P1 and P2 are both on the curve
      -# 2.the output point may be neutral point (0, 1), it is valid
      -# 3.please make sure the curve is edwards25519
  @endverbatim
 */
unsigned int ed25519_pointAdd(edward_curve_t *curve, unsigned int *P1x, unsigned int *P1y, unsigned int *P2x, unsigned int *P2y,
        unsigned int *Qx, unsigned int *Qy)
{
    unsigned int wordLen = GET_WORD_LEN(curve->p_bitLen);
    unsigned int ret;

    //set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->p, curve->p_h, curve->p_n0, curve->p_bitLen);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //pke_pre_calc_mont() may cover some addresses, so load parameters here
    pke_load_operand((volatile unsigned int *)PKE_A(1,step), P1x, wordLen);                      //A1 P1x
    pke_load_operand((volatile unsigned int *)PKE_A(2,step), P1y, wordLen);                      //A2 P1y
    pke_load_operand((volatile unsigned int *)PKE_B(1,step), P2x, wordLen);                      //B1 P2x
    pke_load_operand((volatile unsigned int *)PKE_B(2,step), P2y, wordLen);                      //B2 P2y
    pke_load_operand((volatile unsigned int *)PKE_B(0,step), curve->d, wordLen);                 //B0 d

    if((step/4) > wordLen)
    {
        uint32_clear((volatile unsigned int *)PKE_A(1,step)+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)PKE_A(2,step)+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)PKE_B(1,step)+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)PKE_B(2,step)+wordLen, (step/4)-wordLen);
        uint32_clear((volatile unsigned int *)PKE_B(0,step)+wordLen, (step/4)-wordLen);
    }
    else
    {;}

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    pke_set_microcode(MICROCODE_Ed25519_PADD);
    pke_clear_interrupt();
    pke_start();
    pke_wait_till_done();
    ret = pke_check_rt_code();
    // ret = pke_set_micro_code_start_wait_return_code(MICROCODE_Ed25519_PADD);
    if(PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(PKE_A(1,step)), wordLen<<2);
        get_rand_fast((unsigned char *)(PKE_A(2,step)), wordLen<<2);
        get_rand_fast((unsigned char *)(PKE_B(0,step)), wordLen<<2);
        get_rand_fast((unsigned char *)(PKE_B(1,step)), wordLen<<2);
        get_rand_fast((unsigned char *)(PKE_B(2,step)), wordLen<<2);
#endif
        return ret;
    }
    else
    {;}

    pke_read_operand((volatile unsigned int *)PKE_A(1,step), Qx, wordLen);                       //A1 Qx
    pke_read_operand((volatile unsigned int *)PKE_A(2,step), Qy, wordLen);                       //A2 Qy

    return PKE_SUCCESS;
}

/**************************** X25519 & Ed25519 finished ********************************/
#endif
