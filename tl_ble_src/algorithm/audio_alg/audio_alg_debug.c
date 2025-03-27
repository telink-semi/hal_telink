/*
 * audio_alg_dubug.c
 *
 *  Created on: Jun 19, 2023
 *      Author: Admin
 */

#include "types.h"
#include "config.h"
#include "audio_alg_debug.h"
#include "sbc/tlk_sbc_interface_api.h"

unsigned char alg_audio_mask        = 0xff;

#if AUDIO_DEBUG_DATA_EN

uint8_t* msbc_enc_channel0_buff     = NULL;
uint8_t* msbc_enc_channel1_buff     = NULL;
uint8_t add_buff_rptr               = 0;//add mean:audio data debug
uint8_t add_buff_wptr               = 0;
uint16_t add_data_index             = 0;

char add_buff[128*ADD_BUFF_BLOCK_NUM] = {0};

/**
 *  @brief      Audio debug data initiate.
 *
 */
int add_init(void* p_buf)
{
    int offset = 0;
    msbc_enc_channel0_buff = p_buf;
    offset += tlka_sbc_enc_get_size();
    msbc_enc_channel1_buff = p_buf + offset;
    offset += tlka_sbc_enc_get_size();

    tlka_sbc_enc_init((sbc_enc_para_t *)msbc_enc_channel0_buff, &g_sbc_param);//frame size 80 or 120, need setting ?
    tlka_sbc_enc_init((sbc_enc_para_t *)msbc_enc_channel1_buff, &g_sbc_param);
    return offset;
}

/**
 *  @brief      Audio debug data stereo encode(MSBC).
 *
 */

int add_enc_stereo(unsigned char *ptr_chn0,unsigned char *ptr_chn1,int len)
{
    int data_len = 160;
    int ret = 0;
    if(g_sbc_param.sbc_blocks == 10){
        data_len = 160;
    }
    else if(g_sbc_param.sbc_blocks == 15){
        data_len = 240;
    }
    else{
        return ret;
    }

    if(len<data_len){
        return ret;
    }


    ///fill frame header
    add_buff[add_buff_wptr * 128]     = 0x5a;
    add_buff[add_buff_wptr * 128 + 1] = 0x5a;
    add_buff[add_buff_wptr * 128 + 2] = 0x5a;
    add_buff[add_buff_wptr * 128 + 3] = add_data_index & 0xff;
    add_buff[add_buff_wptr * 128 + 4] = (add_data_index >> 8) & 0xff;
    add_buff[add_buff_wptr * 128 + 5] = 0x01;

    ///encode chn0
    tlkalg_msbc_enc_ptr((sbc_enc_para_t*)msbc_enc_channel0_buff,(u8*)ptr_chn0, data_len, (u8*)(add_buff+add_buff_wptr*128+4));

    ///fill frame header
    add_buff[add_buff_wptr * 128 + 64] = 0x5a;
    add_buff[add_buff_wptr * 128 + 65] = 0x5a;
    add_buff[add_buff_wptr * 128 + 66] = 0x5a;
    add_buff[add_buff_wptr * 128 + 67] = add_data_index & 0xff;
    add_buff[add_buff_wptr * 128 + 68] = (add_data_index >> 8) & 0xff;
    add_buff[add_buff_wptr * 128 + 69] = 0x02;

    ///encode chn1
    tlkalg_msbc_enc_ptr((sbc_enc_para_t*)msbc_enc_channel1_buff,(u8*)ptr_chn1, data_len, (u8*)(add_buff+add_buff_wptr*128+68));
    add_data_index ++;
    add_buff_wptr = (add_buff_wptr + 1) % ADD_BUFF_BLOCK_NUM;
    ret = data_len;
    return ret;
}
#endif
