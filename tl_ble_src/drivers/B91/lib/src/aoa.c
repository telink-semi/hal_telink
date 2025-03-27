/********************************************************************************************************
 * @file    aoa.c
 *
 * @brief   This is the source file for B91
 *
 * @author  Driver Group
 * @date    2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "lib/include/aoa.h"
/**
 * @brief       This function servers to find the average of a set of numbers
 * @param[in]   data - the  data.
 * @param[in]   bytenum   - the type of data.
 * @return      abs.
 */
unsigned int abs_value(unsigned int data, TypeDefByteNum bytenum)
{
    if(BYTE_NUM_2P5 == bytenum){
        if(data>=0x80000)
            return (0xfffff-data+1);
        else if(data < 0x80000)
            return(data);
    }
    else if(BYTE_NUM_4 == bytenum){
        if(data>=0x80000000)
            return(0xffffffff-data+1);
        else if(data < 0x80000000)
            return(data);
    }
    return 0;
}


/**
 * @brief       This function is used to convert 20bit to 8bit data
 * @param[in]   *data_src               - the ddr of data.
 * @param[in]   *data_has_amplitude     - the data with amplitude.
 * @param[in]   *data_no_amplitude      - the data without amplitude.
 * @return      none.
 */
void frond_end(unsigned char *data_src, unsigned char *data_has_amplitude, unsigned char *data_no_amplitude)
{
    int temp[90] = {0};
    unsigned int max = 0;
    unsigned char ii = 0;
    for(unsigned char i = 0; i < 45; i++)
    {
        temp[i*2] = ((data_src[i*5+2]&0x0f)<<16) + (data_src[i*5+1]<<8) + data_src[i*5];
        temp[i*2+1] = (data_src[i*5+4]<<12) + (data_src[i*5+3]<<4) + ((data_src[i*5+2]>>4)&0x0f);

        if(abs_value(temp[i*2],BYTE_NUM_2P5) > abs_value(temp[i*2+1],BYTE_NUM_2P5))
            max = abs_value(temp[i*2],BYTE_NUM_2P5);
        else
            max = abs_value(temp[i*2+1],BYTE_NUM_2P5);

        if(max>128)
        {
            for (ii=0; ii < 12; ++ii)
            {
                max = max >> 1;
                if(max<=128)
                    break;
            }
            data_no_amplitude[i*2] = (abs_value(temp[i*2], BYTE_NUM_2P5) >> (ii+1));
            data_no_amplitude[i*2+1] = (abs_value(temp[i*2+1], BYTE_NUM_2P5) >> (ii+1));
        }else{
            data_no_amplitude[i*2] = (abs_value(temp[i*2], BYTE_NUM_2P5));
            data_no_amplitude[i*2+1] = (abs_value(temp[i*2+1], BYTE_NUM_2P5));
        }

        if(temp[i*2] > 0x7ffff)
            data_no_amplitude[i*2] = 0x100 - data_no_amplitude[i*2];
        if(temp[i*2+1] > 0x7ffff)
            data_no_amplitude[i*2+1] = 0x100 - data_no_amplitude[i*2+1];
    }

    max = 0;
    for(unsigned char i = 0; i < 90; i++)
    {
        if(abs_value(temp[i],BYTE_NUM_2P5) > max){
            max = abs_value(temp[i],BYTE_NUM_2P5);
        }
    }

    if(max>128)
    {
        for (ii=0; ii < 12; ++ii)
        {
            max = max >> 1;
            if(max<=128)
                break;
        }
        ii = ii + 1;
    }else{
        ii = 0;
    }

    for (int i = 0; i < 90; ++i)
    {
        data_has_amplitude[i] = (abs_value(temp[i],BYTE_NUM_2P5) >> (ii));
        if(temp[i] > 0x7ffff)
            data_has_amplitude[i] = 0x100 - data_has_amplitude[i];

    }
}

