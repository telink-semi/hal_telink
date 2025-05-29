/********************************************************************************************************
 * @file    libcs_tlk3.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
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

#ifndef LIBCS_TLK3_H_
#define LIBCS_TLK3_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#if 0//__riscv
#include <nds_intrinsic.h>
#include "riscv_dsp_statistics_math.h"
#include "riscv_dsp_complex_math.h"
#include "riscv_dsp_utils_math.h"
#endif



#define CS_VERSION_INT(major, minor, micro) (((major) << 16) | ((minor) << 8) | (micro))
#define CS_ALGO3_VERSION CS_VERSION_INT(0, 3, 4)

typedef unsigned char      u8;
typedef unsigned int      u32;

#define CS_PI 3.1415926535897932354626

#define CS_MAX_CH_NUM 80
#define CS_MAX_DIST 150

typedef struct
{
    float I;
    float Q;
}compx;

#define CS_ANT_NUM 4

typedef struct
{
    float cs_alpha_s;
    float cs_distbeta_s;
    float cs_thd_s;
    u8 cs_normest_en_s;

    float cs_alpha_m;
    float cs_distbeta_m;
    float cs_thd_m;
    u8 cs_normest_en_m;

    float dist_fine_range;
    float cs_distest;
    u8 cs_nd_thd;
    u8 cs_qi_thd;
    short cs_mant_ceta;

    float cs_alpha_nlos;
    float cs_distest_nlos;
    u8 cs_los_mant[CS_ANT_NUM];
    u8 cs_los_st;
    u8 cs_agc_st;
    u8 cs_agc_cnt;
    u8 cs_agc_init;
    u8 cs_agc_refl;

    compx cs_h[CS_MAX_CH_NUM * CS_ANT_NUM];
    compx cs_phi_norm[CS_MAX_CH_NUM * CS_ANT_NUM];
    compx cs_phi_trak[CS_MAX_CH_NUM * CS_ANT_NUM];
    u8 cs_nd[CS_MAX_CH_NUM];
    u8 cs_qi[CS_MAX_CH_NUM * CS_ANT_NUM];

    float rt_win[10];
} tlka_cs_t;

#define QI_THD 3
#define ND_THD 10

/*
 * @brief           This function is mainly used for getting the version of CS distance calculation algorithm3.
 * @param[in]       none.
 * @return          none.
 */
int tlka_cs_get_version(void);

/*
 * @brief           This function is mainly used for initializing the distance calculation algorithm3
 * @param[in]       cs_st - relevant CS algorithm3 running parameter.
 * @return          none.
 */
void tlka_cs_init(tlka_cs_t* cs_st);

/*
 * @brief           This function is mainly used for calculate the CS distance by algorithm3
 * @param[in]       cs_st - relevant CS algorithm3 running parameter.
 * @param[in]       pct_pdu_init - pct of initiator
 * @param[in]       pct_pdu_refl - pct of reflector
 * @param[in]       ch_num       - channel number of a single procedure
 * @param[in]       ch_valid     - valid channel number of a single procedure
 * @param[in]       ant_num      - antenna path number
 * @return          the result of distance.
 */
float tlka_cs_proc(tlka_cs_t* cs_st, u8* pct_pdu_init, u8* pct_pdu_refl, u8 ch_num, u8* ch_valid, u8 ant_num);


#endif /* LIBCS_TLK3_H_ */
