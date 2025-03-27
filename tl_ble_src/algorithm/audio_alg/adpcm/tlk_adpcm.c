/********************************************************************************************************
 * @file    tlk_adpcm.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BT Audio Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "common/compiler.h"
#include "tl_common.h"
#include "tlk_adpcm.h"

#if ALG_ADPCM_EN

#ifndef ADPCM8_EN
#define ADPCM8_EN 0
#endif

typedef struct adpcm_para_cfg {
    unsigned char *ptr;
    int len;
    int offset;
    signed short predict;
    signed short last;

    float d1;
    float d2;
    float d3;
    float d4;
    float iuk;

    signed char idx;
    unsigned char fra;
    unsigned char id;
    signed char idx_enc;
    signed short predict_enc;
    signed short vol;
    signed short gain;
} adpcm_para_cfg_t;

adpcm_para_cfg_t adpcm_para = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

///////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if !ADPCM8_EN
static const signed char idxtbl[]     = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };
static const unsigned short steptbl[] = {
    7,    8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,   28,
    31,   34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,  118,
    130,  143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,  494,
    544,  598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878, 2066,
    2272, 2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845, 8630,
    9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

//////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////

void pcm_to_adpcm(signed short *ps, int len, signed short *pd)
{
    //  signed char idxtbl[] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
    int i, j;
    unsigned short code   = 0;
    unsigned short code16 = 0;

    for (i = 0; i < len; i++) {
        s16 di   = ps[i];
        int step = steptbl[adpcm_para.idx_enc];
        int diff = di - adpcm_para.predict_enc;

        if (diff >= 0) {
            code = 0;
        } else {
            diff = -diff;
            code = 8;
        }

        int diffq = step >> 3;

        for (j = 4; j > 0; j = j >> 1) {
            if (diff >= step) {
                diff  = diff - step;
                diffq = diffq + step;
                code  = code + j;
            }
            step = step >> 1;
        }

        code16 = (code16 >> 4) | (code << 12);
        if ((i & 3) == 3) {
            *pd++ = code16;
        }

        int pre = adpcm_para.predict_enc;
        if (code >= 8) {
            pre -= diffq;
        } else {
            pre += diffq;
        }

#if 0
        if (pre > 32767) {
            pre = 32767;
        }
        else if (pre < -32768) {
            pre = -32768;
        }
        adpcm_para.predict_enc = pre;
#else
        adpcm_para.predict_enc = __nds__kaddh(0, pre);
#endif

        adpcm_para.idx_enc = adpcm_para.idx_enc + idxtbl[code];
        if (adpcm_para.idx_enc < 0) {
            adpcm_para.idx_enc = 0;
        } else if (adpcm_para.idx_enc > 88) {
            adpcm_para.idx_enc = 88;
        }
    }
}

////////////////////////////////////////////////////////////////////
/*  name ADPCM to pcm
    signed short *ps -> pointer to the adpcm source buffer
    signed short *pd -> pointer to the pcm destination buffer
    int len          -> decorded size
*/
void adpcm_init(unsigned char *ps, int len, int pre, int idx)
{
    adpcm_para.ptr         = ps;
    adpcm_para.len         = len;
    adpcm_para.predict     = pre;
    adpcm_para.predict_enc = pre;
    adpcm_para.idx         = idx;
    adpcm_para.idx_enc     = idx;
    adpcm_para.offset      = 0;
    adpcm_para.last        = 0;
    adpcm_para.fra         = 0;
    adpcm_para.id          = 0;
}

_attribute_ram_code_ int adpcm_to_pcm(unsigned char *ps, signed short *pd, int outlen)
{
    //  signed char idxtbl[] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

    //  unsigned char ps[BNUM/2];
    //  tmemcpy (ps, psrc, BNUM/2);
    unsigned char code = 0;

    for (int i = 0; i < outlen; i++) {
        if (i & 1) {
            code = code >> 4;
        } else {
            code = *ps++;
        }

        int step = steptbl[adpcm_para.idx];

        int diffq = step >> 3;

        if (code & 4) {
            diffq = diffq + step;
        }
        step = step >> 1;
        if (code & 2) {
            diffq = diffq + step;
        }
        step = step >> 1;
        if (code & 1) {
            diffq = diffq + step;
        }

        int pre = adpcm_para.predict;
        if (code & 8) {
            pre -= diffq;
        } else {
            pre += diffq;
        }

#if 0
        if (pre > 32767) {
            pre = 32767;
        }
        else if (pre < -32768) {
            pre = -32768;
        }
        adpcm_para.predict = pre;
#else
        adpcm_para.predict = __nds__kaddh(0, pre);
#endif

        adpcm_para.idx = adpcm_para.idx + idxtbl[code & 15];

        if (adpcm_para.idx < 0) {
            adpcm_para.idx = 0;
        } else if (adpcm_para.idx > 88) {
            adpcm_para.idx = 88;
        }

        *pd++ = adpcm_para.predict;
    }
    return outlen;
}

#define BNUM 128
signed short adpcm_d[BNUM];
_attribute_ram_code_ int adpcm_get_sample(signed short *pd, int n, int sample_rate)
{
    if (adpcm_para.len - adpcm_para.offset < BNUM / 2) {
        return 0;
    }

    int nt = 0;

    int step = 256 * 16000 / sample_rate;

    if (!adpcm_para.offset) {
        adpcm_to_pcm(adpcm_para.ptr + adpcm_para.offset, adpcm_d, BNUM);
        adpcm_para.offset += BNUM / 2;
        adpcm_para.last = adpcm_d[0];
        adpcm_para.id   = 0;
        adpcm_para.fra  = 0;
    }
    while (n--) {
        int pos = adpcm_para.fra + step;
        if (pos >= 256) {
            adpcm_para.last = adpcm_d[adpcm_para.id++];
            if (adpcm_para.id >= BNUM) {
                int left = adpcm_para.len - adpcm_para.offset;
                if (left < BNUM / 2) {
                    if (left) {
                        adpcm_para.offset = adpcm_para.len;
                        tlk_mem_set(adpcm_d, 0, BNUM * 2);
                    }
                } else {
                    adpcm_to_pcm(adpcm_para.ptr + adpcm_para.offset, adpcm_d, BNUM);
                    adpcm_para.offset += BNUM / 2;
                }
                adpcm_para.id = 0;
            }
        }
        adpcm_para.fra = pos;
        int v = (adpcm_para.last * (int)(256 - adpcm_para.fra) + adpcm_d[adpcm_para.id] * (int)adpcm_para.fra) >> 8;
        //*pd++ = v;
        *pd = (*pd >> 1) + (v >> 1);
        pd++;
#if !CODEC_DAC_MONO_MODE
        *pd = (*pd >> 1) + (v >> 1);
        pd++;
#endif
        nt++;
    }
    return nt;
}

#else

/**
 * @brief ADPCM data upsampled
 *
 * @param[in] PCM data error accuracy
 * @param[in] Enter the PCM data
 * @param[out] Output the converted data buffer
 *
 * @returns Interpolation state
 */
_attribute_ram_code_ static int tlka_adpcm_upresample_fra(float iTErr, signed short di, signed short *pd)
{
    float iOneTERRBW = 1.0f;

    float iPPAlpha = (0.2f);

    float iUKs2 = (adpcm_para.iuk * adpcm_para.iuk);

    float iC2  = iPPAlpha * (iUKs2 - adpcm_para.iuk);
    float iC1  = iPPAlpha * (-iUKs2 + adpcm_para.iuk) + adpcm_para.iuk;
    float iC0  = iPPAlpha * (-iUKs2 + adpcm_para.iuk) - adpcm_para.iuk + iOneTERRBW;
    float iC_1 = iPPAlpha * (iUKs2 - adpcm_para.iuk);

    float iOut32 = iC_1 * adpcm_para.d4 + iC0 * adpcm_para.d3 + iC1 * adpcm_para.d2 + iC2 * adpcm_para.d1;

    if (iOut32 > 32767) {
        *pd = 32767;
    } else if (iOut32 < -32768) {
        *pd = -32768;
    } else {
        *pd = (int16_t)iOut32;
    }

    adpcm_para.iuk += iTErr;

    if (adpcm_para.iuk >= iOneTERRBW) {
        adpcm_para.iuk -= iOneTERRBW;
        adpcm_para.d4 = adpcm_para.d3;
        adpcm_para.d3 = adpcm_para.d2;
        adpcm_para.d2 = adpcm_para.d1;
        adpcm_para.d1 = di;

        return 1;
    }

    return 0;
}

/**
 * @brief ADPCM upsample conversion
 *
 * @param[in] ADPCM up-sampling weight step
 * @param[in] Enter the PCM data
 * @param[out] Output the converted data buffer
 *
 * @returns Interpolation state
 */
static inline int tlka_adpcm_upresample_int(int step, signed short di, signed short *pd)
{
    *pd = (adpcm_para.last * (int)(256 - adpcm_para.fra) + di * (int)adpcm_para.fra) >> 8;

    int fra = adpcm_para.fra + step;

    adpcm_para.fra = fra;

    if (fra >= 256) {
        adpcm_para.last = di;
        return 1;
    }

    return 0;
}

/**
 * @brief Initialization of the adpcm module
 *
 * @param[in] System pointer of the adpcm module
 * @param[in] decorded size
 * @param[in] Preprocessing encoding is enabled
 * @param[in] Preprocessing code index
 *
 * @returns none
 */
void adpcm_init(unsigned char *ps, int len, int pre, int idx)
{
    adpcm_para.ptr         = ps;
    adpcm_para.len         = len;
    adpcm_para.predict     = pre;
    adpcm_para.predict_enc = pre;
    adpcm_para.idx         = idx;
    adpcm_para.idx_enc     = idx;
    adpcm_para.offset      = 0;
    adpcm_para.last        = 0;
    adpcm_para.fra         = 0;
    adpcm_para.id          = 0;
    adpcm_para.d1          = 0;
    adpcm_para.d2          = 0;
    adpcm_para.d3          = 0;
    adpcm_para.d4          = 0;
    adpcm_para.iuk         = 0;
    if (!adpcm_para.gain) {
        adpcm_para.gain = 1024;
    }
}

/**
 * @brief Set the ADPCM conversion gain
 *
 * @param[in] conversion gain
 *
 * @returns none
 */
void adpcm_set_gain(int gain)
{
    // tlkapi_sendData (1, "set tone gain of 1024", &gain, 4);
    adpcm_para.gain = gain;
}

/**
 * @brief ADPCM fixed 8 sampling point conversions
 *
 * @param[in] The last sampling point of the previous frame
 * @param[in] Input data pointer
 * @param[out] Output data pointer
 * @param[in] Converted data length
 *
 * @returns The last sampling point of the current frame
 */
_attribute_ram_code_ int adpcm8to16_8samples(int last, unsigned char *ps, signed short *pd, int n)
{
    int m   = *ps++;
    int max = 1 << (m >> 4);
    max += (max * (m & 15)) >> 4;

    for (int i = 0; i < n; i++) {
        signed char pcm8 = *ps++;

        int pcm16 = ((int)pcm8 * max) >> 7;

        last += pcm16;

        *pd++ = last;
    }

    return last;
}

/**
 * @brief ADPCM data 8bits Convert 16bits
 *
 * @param[in] Input data pointer
 * @param[out] Output data pointer
 * @param[in] Converted data length
 *
 * @returns The last sampling point of the current frame
 */
_attribute_ram_code_ int adpcm8to16(unsigned char *ps, signed short *pd, int n)
{
    int nb = 0;
    for (int i = 0; i < n; i += 8) {
        adpcm_para.predict_enc = adpcm8to16_8samples(adpcm_para.predict_enc, ps + nb, pd, 8);
        nb += 9;
        pd += 8;
    }
    return nb;
}

#define BNUM 128

signed short adpcm_d[BNUM];


/**
 * @brief ADPCM to pcm
 *
 * @param[in] pointer to the adpcm source buffer
 * @param[in] decorded size
 * @param[in] Conversion sampling rate
 *
 * @returns Encoding result size
 */
_attribute_ram_code_ int adpcm_get_sample(signed short *pd, int n, int sample_rate)
{
    int ni = n;

    if (adpcm_para.len - adpcm_para.offset < BNUM / 2) {
        if (adpcm_para.vol < 1024) {
            while (n--) {
                if (adpcm_para.vol < 1024) {
                    adpcm_para.vol++;
                }
                *pd = (*pd * adpcm_para.vol + 512) >> 10;
                pd++;

#if !CODEC_DAC_MONO_MODE
                *pd = ((int)(*pd) * adpcm_para.vol + 1) >> 10;
                pd++;
#endif
            }
            return ni;
        }
        return 0;
    }

    if (!adpcm_para.offset) {
        if (adpcm_para.vol > 512) {
            while (n--) {
                if (adpcm_para.vol > 512) {
                    adpcm_para.vol--;
                }
                *pd = ((int)(*pd) * adpcm_para.vol + 512) >> 10;
                pd++;

#if !CODEC_DAC_MONO_MODE
                *pd = ((int)(*pd) * adpcm_para.vol + 512) >> 10;
                pd++;
#endif
            }
            return ni;
        }

        adpcm_para.offset += adpcm8to16(adpcm_para.ptr + adpcm_para.offset, adpcm_d, BNUM);
        adpcm_para.last = adpcm_d[0];
        adpcm_para.id   = 0;
        adpcm_para.fra  = 0;
    }

    int nt = 0;

    //  int step = 256 * 16000 / sample_rate;

    float iErr = 16000.0f / sample_rate;

    while (n--) {
        signed short v;

        int next;
        if (sample_rate == 16000) {
            v    = adpcm_d[adpcm_para.id];
            next = 1;
        } else {
            next = tlka_adpcm_upresample_fra(iErr, adpcm_d[adpcm_para.id], &v);
        }
        if (next) {
            adpcm_para.id++;
            if (adpcm_para.id >= BNUM) {
                int left = adpcm_para.len - adpcm_para.offset;
                if (left < BNUM) {
                    if (1 || left) {
                        adpcm_para.offset = adpcm_para.len;
                        memset(adpcm_d, 0, BNUM * 2);
                    }
                } else {
                    adpcm_para.offset += adpcm8to16(adpcm_para.ptr + adpcm_para.offset, adpcm_d, BNUM);
                }
                adpcm_para.id = 0;
            }
        }

        *pd = (((int)(*pd) * (int)adpcm_para.vol) + (v * adpcm_para.gain) + 512) >> 10;
        pd++;

#if !CODEC_DAC_MONO_MODE
        *pd = (((int)(*pd) * (int)adpcm_para.vol) + (v * adpcm_para.gain) + 512) >> 10;
        pd++;
#endif
        nt++;
    }
    // tlkapi_sendU32s (1, "tone_play", nt, adpcm_para.last, adpcm_para.offset, adpcm_para.len);
    return nt;
}
#endif
#endif
