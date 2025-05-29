/********************************************************************************************************
 * @file    mac.c
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
#include "mac.h"
#include "ota.h"
#include "tl_common.h"
#include "drivers.h"
#include "../stack/2p4g/genfsk_ll/genfsk_ll.h"

#define MAC_TX_BUF_LEN 64
#define MAC_RX_BUF_LEN 128
#define MAC_RX_BUF_NUM 4
#define MAC_STX_WAIT   30 //in us
#define MAC_SRX_WAIT   5  //in us
//#define MAC_RX_WAIT                   200000 //in us
#define MAC_RX_WAIT (60 * 1000) //in us

//#define    GPIO_DEBUG_PD0        GPIO_PD0
//#define    GPIO_DEBUG_PD1        GPIO_PD1

typedef struct
{
    unsigned short Channel;
    MAC_Cb         RxCb;
    MAC_Cb         TxCb;
    MAC_Cb         RxTimeoutCb;
    MAC_Cb         RxFirstTimeoutCb;
} MAC_InfoTypeDef;

MAC_InfoTypeDef mac_Info = {0, 0, 0, 0, 0};

static unsigned char mac_TxBuf[MAC_TX_BUF_LEN] __attribute__((aligned(4)))                  = {};
static unsigned char mac_RxBuf[MAC_RX_BUF_LEN * MAC_RX_BUF_NUM] __attribute__((aligned(4))) = {};

void tx_en_on_init(void)
{
    write_reg8(0x5a8, 0xff);
    //    write_reg8(0x586, 0x00);
    write_reg8(0x586, read_reg8(0x586) & 0xe0);
    write_reg8(0x5b6, read_reg8(0x5b6) | (1 << 1));
}

/**
 *  @note  The TPLL_GenericHeader_t function to set generic packet header,
 *         You need to configure the size in bit and the value of h0,h1 and payload len.
 *         You also can configure the start location of pid and no ack bit if you need.
 *         only generic mode need to configure this!!!
*/
gen_fsk_generic_header_t GEN_FSK_GenericHeader;

gen_fsk_generic_header_t OTA_GenericHeader = {
    .h0_size     = 0,
    .length_size = 8,
    .h1_size     = 0,
    .h0_val      = 0,
    .length_val  = 8,
    .h1_val      = 0,
};

void MAC_Init(const unsigned short Channel,
              const MAC_Cb         RxCb,
              const MAC_Cb         TxCb,
              const MAC_Cb         RxTimeoutCb,
              const MAC_Cb         RxFirstTimeoutCb,
              unsigned int         acc)
{
    mac_Info.Channel          = Channel;
    mac_Info.RxCb             = RxCb;
    mac_Info.TxCb             = TxCb;
    mac_Info.RxTimeoutCb      = RxTimeoutCb;
    mac_Info.RxFirstTimeoutCb = RxFirstTimeoutCb;

    //reset rf
    rf_dma_reset();
    rf_reset_register_value();

    memcpy(&GEN_FSK_GenericHeader, &OTA_GenericHeader, sizeof(GEN_FSK_GenericHeader));
    gen_fsk_packet_format_set(GEN_FSK_MODE_GENERIC_VARIABLE_FORMAT, 0);
    gen_fsk_datarate_set(GEN_FSK_DATARATE_2MBPS); //Note that this API must be invoked first before all other APIs
    gen_fsk_preamble_len_set(4);
    gen_fsk_sync_word_len_set(SYNC_WORD_LEN_4BYTE);
    rf_access_code_comm(acc);                     //set pipe0's sync word
    gen_fsk_pipe_open(GEN_FSK_PIPE0);             //enable pipe0's reception
    gen_fsk_tx_pipe_set(GEN_FSK_PIPE0);           //set pipe0 as the TX pipe
    gen_fsk_radio_power_set(GEN_FSK_POWER_INDEX_P0p0dBm);
    gen_fsk_rx_buffer_set(mac_RxBuf, MAC_RX_BUF_NUM, MAC_RX_BUF_LEN);
    gen_fsk_tx_buffer_set(2, MAC_TX_BUF_LEN);
    gen_fsk_channel_set(Channel);
    gen_fsk_radio_state_set(GEN_FSK_STATE_AUTO); //set transceiver to basic RX state
    gen_fsk_rx_settle_set(89);
    delay_us(90);                                //wait for rx settle

    //irq configuration
    plic_set_priority(IRQ_ZB_RT, 3);
    plic_interrupt_enable(IRQ_ZB_RT);
    rf_set_irq_mask(FLD_RF_IRQ_RX | FLD_RF_IRQ_TX | FLD_RF_IRQ_FIRST_TIMEOUT | FLD_RF_IRQ_RX_TIMEOUT);
    rf_clr_irq_status(FLD_RF_IRQ_ALL);
    core_interrupt_enable();
}

void MAC_BroadcastData(const unsigned char *Payload, const unsigned char PayloadLen)
{
#if 0
    tlk_printf("MAC_BroadcastData\n");
    for(int i = 0; i < PayloadLen +5; i++)
    {
        tlk_printf("%x ", mac_TxBuf[i]);
    }
    tlk_printf("\n");
#endif
    GEN_FSK_GenericHeader.length_val = PayloadLen;
    gen_fsk_write_payload(mac_TxBuf, (const unsigned char *)Payload, PayloadLen);
    gen_fsk_stx_start(mac_TxBuf, clock_time() + MAC_STX_WAIT * 16);
}

void MAC_SendData(const unsigned char *Payload, const int PayloadLen)
{
    if (PayloadLen > MAC_TX_BUF_LEN) {
        return;
    }
    GEN_FSK_GenericHeader.length_val = PayloadLen;
    gen_fsk_write_payload(mac_TxBuf, (const unsigned char *)Payload, PayloadLen);
    gen_fsk_stx2rx_start(mac_TxBuf, clock_time() + MAC_STX_WAIT * 16, MAC_RX_WAIT);
}

void MAC_RecvData(unsigned int TimeUs)
{
    gen_fsk_srx_start(clock_time() + MAC_SRX_WAIT * 16, TimeUs);
}

_attribute_ram_code_sec_noinline_ void MAC_RxIrqHandler(void)
{
    //get rf packet
    unsigned char *RxPacket = rf_get_rx_packet_addr(MAC_RX_BUF_NUM, MAC_RX_BUF_LEN, mac_RxBuf);
    /* clear the interrupt flag */
    reg_rf_irq_status = FLD_RF_IRQ_RX;

    if (rf_get_crc_err()) {
        if (mac_Info.RxCb) {
            mac_Info.RxCb(NULL);
        }
        return;
    }
    if (mac_Info.RxCb) {
        mac_Info.RxCb(&RxPacket[4]);
    }
}

extern volatile unsigned char OTA_SlaveTrig;

_attribute_ram_code_sec_noinline_ void MAC_RxIrqHandler_Batch(void)
{
    unsigned char *RxPacket = rf_get_rx_packet_addr(MAC_RX_BUF_NUM, MAC_RX_BUF_LEN, mac_RxBuf);

    /* clear the interrupt flag */
    reg_rf_irq_status = FLD_RF_IRQ_RX;
    if (rf_get_crc_err()) {
        unsigned short bln = RxPacket[7] | (RxPacket[8] << 8);
        tlk_printf("rx packet crc error blknum:%d\r\n", bln);
#if 0
         for(int i = 0; i < RxPacket[0] + 11; i++)
         {
             tlk_printf("%x ", RxPacket[i+4] );
         }
         tlk_printf("\n");
#endif
        if (mac_Info.RxCb) {
            mac_Info.RxCb(NULL);
        }
        //        gpio_toggle(GPIO_DEBUG_PD1);
        return;
    }
    //    tlk_printf("RxPacket[4]:%d %d %d %d\r\n", RxPacket[4],RxPacket[5],RxPacket[6],RxPacket[7]);
    if (RxPacket[5] == OTA_FRAME_TYPE_CMD && RxPacket[6] == OTA_CMD_ID_BATCH_START_REQ) {
        OTA_SlaveTrig = 1;
    }
    if (mac_Info.RxCb) {
#if 0
       if(loss_check)
        {
           tlk_printf("loss_check rx\r\n");
        for(int i = 0; i < RxPacket[0] + 16; i++)
        {
            tlk_printf("%x ", RxPacket[i] );
        }
        tlk_printf("\n");
        }
#endif


        mac_Info.RxCb(&RxPacket[4]);
    }
}

void MAC_TxIrqHandler(void)
{
    if (mac_Info.TxCb) {
        mac_Info.TxCb(NULL);
    }
}

void MAC_RxTimeOutHandler(void)
{
    /* clear the interrupt flag */
    reg_rf_irq_status = FLD_RF_IRQ_RX_TIMEOUT;

    if (mac_Info.RxTimeoutCb) {
        mac_Info.RxTimeoutCb(NULL);
    }
}

void MAC_RxFirstTimeOutHandler(void)
{
    /* clear the interrupt flag */
    reg_rf_irq_status = FLD_RF_IRQ_FIRST_TIMEOUT;

    if (mac_Info.RxFirstTimeoutCb) {
        mac_Info.RxFirstTimeoutCb(NULL);
    }
}
