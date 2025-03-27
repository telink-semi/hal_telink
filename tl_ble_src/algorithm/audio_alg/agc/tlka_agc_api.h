/********************************************************************************************************
 * @file    tlka_agc_api.h
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  Driver Group
 * @date    2020
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef TLKA_AGC_API_H
#define TLKA_AGC_API_H

/*! Construct version number from major/minor/micro values. */
#define AGC_VERSION_INT(major, minor, micro) (((major) << 16) | ((minor) << 8) | (micro))

/*! Version number to ensure header and binary are matching. */
#define AGC_VERSION AGC_VERSION_INT(0, 2, 1)

typedef struct _AGC_CFG_Param
{
    short targetLevelDbfs;   /// AGC TARGET,This setting value N means the actual output -N (dB) signal
                             /// To correctly set this parameter, the user must take into account a margin to avoid signal saturation.
                             /// range(dB):0~10            default(dB) :3
    short compressionGaindB; /// the AGC gain,This setting value M means that the actual AGC gain is M (dB)
                             /// range(dB):0~40            default(dB) :9
    unsigned int fs;

} AGC_CFG_Param;

typedef enum
{
    TLKA_AGC_OK = 0,
    TLKA_AGC_INIT_ERR,
    TLKA_AGC_CONFIG_ERR,
    TLKA_AGC_PROCESS_ERR,
    TLKA_AGC_PARA_END
} TLKA_AGC_Error;

/*
 * Return AGC lib version
 */
int tlka_agc_get_version(void);

/*
 * Return persistent memory size required by AGC
 */
int tlka_agc_get_size(void);

/** Init a new agc state
 * @param agcInst AGC state
 * @param agc_param User defined parameters, see AGC_CFG_Param for detail
 * @return error code defined by TLKA_AGC_Error
 */
TLKA_AGC_Error tlka_agc_init(void *agcInst, AGC_CFG_Param agc_param);

/** Preprocess a frame
 * @param agcInst AGC state
 * @param in_near Pointer to input buffer
 * @param samples Number of samples to process
 * @param out Output buffer
 * @return error code defined by TLKA_AGC_Error
 */
TLKA_AGC_Error tlka_agc_process(void *agcInst,
                                const short *const *in_near,
                                size_t samples, /// 80:5ms,120:7.5ms,160:10ms
                                short *const *out);

#endif
