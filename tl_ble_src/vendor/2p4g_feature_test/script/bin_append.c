/********************************************************************************************************
 * @file    bin_append.c
 *
 * @brief   This is the source file for 2.4G SDK
 *
 * @author  2.4G Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef __ENV__
    #define __BOARD__ 0
    #define __WIN__   1
    #define __ENV__   __BOARD__
#endif

#if (__ENV__ == __WIN__)
    #include <stdio.h>
    #include <stdlib.h>
    #include <windows.h>
    #include <wchar.h>

    #define VULTURE_BIN_MAX    (512 * 1024)
    #define EAGLE_BIN_MAX      (512 * 1024)
    #define BIN_BUF_SIZE       VULTURE_BIN_MAX
    #define PAYLOAD_LEN        64
    #define FW_APPEND_INFO_LEN 2

unsigned char bin_buf[BIN_BUF_SIZE];

unsigned short crc16_cal(unsigned short crc, unsigned char *pd, int len)
{
    // unsigned short       crc16_poly[2] = { 0, 0xa001 }; //0x8005 <==> 0xa001
    unsigned short crc16_poly[2] = {0, 0x8408}; //0x1021 <==> 0x8408
    //unsigned short        crc16_poly[2] = { 0, 0x0811 }; //0x0811 <==> 0x8810
    //unsigned short        crc = 0xffff;
    int i, j;

    for (j = len; j > 0; j--) {
        unsigned char ds = *pd++;
        for (i = 0; i < 8; i++) {
            crc = (crc >> 1) ^ crc16_poly[(crc ^ ds) & 1];
            ds  = ds >> 1;
        }
    }

    return crc;
}

void crc_test(unsigned int totoal_bin_size)
{
    int            block_idx     = 0;
    int            max_block_num = (totoal_bin_size + PAYLOAD_LEN - 1) / (PAYLOAD_LEN);
    unsigned short crc_val       = 0;
    int            last_len      = 0;
    int            len           = 0;
    while (1) {
        if ((totoal_bin_size - block_idx * PAYLOAD_LEN) > PAYLOAD_LEN) {
            len     = PAYLOAD_LEN;
            crc_val = crc16_cal(crc_val, &bin_buf[block_idx * PAYLOAD_LEN], len);
        } else {
            // prevent fwCRC results from being added to new CRC verification
            len     = totoal_bin_size - (block_idx * PAYLOAD_LEN);
            crc_val = crc16_cal(crc_val, &bin_buf[block_idx * PAYLOAD_LEN], len);
            break;
        }
        block_idx++;
        printf("block idx:%d, len:%d, crc_val:%2x\n", block_idx, len, crc_val);
    }
    printf("block idx:%d, last len:%d, crc_val:%2x\n", block_idx + 1, len, crc_val);
}

int main(int argc, char *argv[])
{
    FILE *fpi, *fpo;
    int   bin_size = 0;
    int   ret      = 0;
    if (argc < 2 || argc > 3) {
        printf("argc = %d, invalid invaild nums invalid\n", argc);
    }

    fpi = fopen(argv[1], "rb");
    if (fpi == NULL) {
        printf("open bin file failed...\n");
        return -1;
    }

    fpo = fopen(argv[2], "wb");
    if (fpo == NULL) {
        printf("open output file failed...\n");
        return -1;
    }

    /* bin buf init*/
    for (int i = 0; i < BIN_BUF_SIZE; i++) {
        bin_buf[i] = 0xff;
    }

    /* get bin size */
    fseek(fpi, 0, SEEK_END);
    bin_size = ftell(fpi);
    fseek(fpi, 0, 0);
    ret = fread(bin_buf, sizeof(char), bin_size, fpi);

    /* vendor code check*/
    if (bin_buf[35] == 'T' && bin_buf[34] == 'L') {
        printf("vendor code:%c %c\n", bin_buf[35], bin_buf[34]);
    } else {
        printf("invalid vendor code:%c %c\n", bin_buf[35], bin_buf[34]);
        return -1;
    }

    /* bin size check */
    unsigned int expect_bin_size = bin_buf[24] | (bin_buf[25] << 8) | (bin_buf[26] << 16) | (bin_buf[27] << 24);
    printf("bin = %s\nbin_size = %d\nexpect_bin_size = %d\n",
           argv[1],
           bin_size,
           expect_bin_size);

    if (expect_bin_size != bin_size) {
        printf("bin size error...\n");
        return -1;
    }

    /* cal crc */
    unsigned short crc_val = crc16_cal(0x0000, bin_buf, bin_size);
    printf("bin crc : %2x \n", crc_val);
    crc_test(bin_size);

    /* crc info append */
    bin_buf[bin_size]     = crc_val;
    bin_buf[bin_size + 1] = (crc_val >> 8);
    bin_size += FW_APPEND_INFO_LEN;

    fwrite(bin_buf, sizeof(char), bin_size, fpo);
    /* crc_test2 */
    fclose(fpi);
    fclose(fpo);
    printf("done!...\n");
    return 0;
}
#endif
