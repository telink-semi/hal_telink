/*
 * audio_alg_dubug.h
 *
 *  Created on: Jun 19, 2023
 *      Author: Admin
 */

#ifndef ALGORITHM_AUDIO_ALG_AUDIO_ALG_DEBUG_H_
#define ALGORITHM_AUDIO_ALG_AUDIO_ALG_DEBUG_H_

#pragma once
#include "alg_audio_cfg.h"
#include "sbc/tlka_sbc_api.h"

#ifndef AUDIO_DEBUG_DATA_EN
#define AUDIO_DEBUG_DATA_EN             0
#endif

#ifndef ADD_BUFF_BLOCK_NUM
#define ADD_BUFF_BLOCK_NUM              4
#endif

extern unsigned char alg_audio_mask;
extern char add_buff[128*ADD_BUFF_BLOCK_NUM];
extern unsigned char add_buff_rptr;
extern unsigned char add_buff_wptr;

int add_init(void* p_buf);
int add_enc_stereo(unsigned char *ptr_chn0,unsigned char *ptr_chn1,int len);

#endif /* ALGORITHM_AUDIO_ALG_AUDIO_ALG_DEBUG_H_ */
