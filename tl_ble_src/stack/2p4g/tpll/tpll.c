/********************************************************************************************************
 * @file    tpll.c
 *
 * @brief   This is the source file for 2.4G SDK
 *
 * @author  2.4G Group
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
#include "driver.h"
#include "types.h"
#include "tpll.h"

static unsigned char rf_mode_format = 0;

#define RF_TX_PACKET_DMA_LEN(rf_data_len)       (((rf_data_len) + 3) / 4) | (((rf_data_len) % 4) << 22)

#define REG_BASEBAND_ACCESS_CODE_LEN            (0x170005)
#define REG_MODEM_ACCESS_CODE_LEN               (0x17044c)
#define REG_BASEBAND_ACCESS_CODE_SYNC_THRESHOLD (0x17044e)
#define REG_DMA_RX_FIFO_WPTR                    (0x1708f4)
#define REG_DMA_RX_FIFO_RPTR                    (0x1708f5)
#define REG_DMA_TX_FIFO_WPTR_PIPE0              (0x170900)
#define REG_DMA_TX_FIFO_RPTR_PIPE0              (0x170901)

#define REG_LINKLAYER_STATE_MACHINE_STATUS      (read_reg8(0x170224) & 0x07)


#define TPLL_RX_WAIT_TIME_MAX                   (4096)
#define TPLL_TX_WAIT_TIME_MAX                   (4096)

#define TPLL_RX_SETTLE_TIME_MIN                 (85)
#define TPLL_RX_SETTLE_TIME_MAX                 (4095)

#define TPLL_FS_RX_SETTLE_TIME_MIN              (44)
#define TPLL_FS_RX_SETTLE_TIME_MAX              (4095)

#define TPLL_TX_SETTLE_TIME_MIN                 (108)
#define TPLL_TX_SETTLE_TIME_MAX                 (4095)

#define TPLL_FS_TX_SETTLE_TIME_MIN              (50)
#define TPLL_FS_TX_SETTLE_TIME_MAX              (4095)

#define TPLL_RX_TIMEOUT_TIME_MIN                (85)
#define TPLL_RX_TIMEOUT_TIME_MAX                (4095)

/** @brief Macro for verifying statement to be true. It will cause the exterior function to return
 *        err_code if the statement is not true.
 *
 * @param[in]   statement   Statement to test.
 * @param[in]   err_code    Error value to return if test was invalid.
 *
 * @retval      nothing, but will cause the exterior function to return @p err_code if @p statement
 *              is false.
 */
#define TPLL_VERIFY_TRUE(statement, err_code) \
    do {                                      \
        if (!(statement)) {                   \
            return err_code;                  \
        }                                     \
    } while (0)

/** @brief Macro for verifying statement to be false. It will cause the exterior function to return
 *        err_code if the statement is not false.
 *
 * @param[in]   statement   Statement to test.
 * @param[in]   err_code    Error value to return if test was invalid.
 *
 * @retval      nothing, but will cause the exterior function to return @p err_code if @p statement
 *              is true.
 */
#define TPLL_VERIFY_FALSE(statement, err_code) \
    do {                                       \
        if ((statement)) {                     \
            return err_code;                   \
        }                                      \
    } while (0)

/** @brief Macro for verifying that the module is initialized. It will cause the exterior function to
 *        return if not.
 *
 * @param[in] param  The variable to check if is NULL.
 */
#define TPLL_VERIFY_PARAM_NOT_NULL(param) TPLL_VERIFY_FALSE(((param) == NULL), TPLL_ERROR_NULL)

#define TPLL_VERIFY_PAYLOAD_LENGTH(length, max_length) \
    do {                                               \
        if (length == 0 || length > max_length) {      \
            return TPLL_ERROR_INVALID_LENGTH;          \
        }                                              \
    } while (0)

/**
 * @brief Enables the W_TX_PAYLOAD_NOACK command
 * @param enable Whether to 1:enable or 0:disable NoAck option in 9-bit(legacy packet) Packet control field
 * @note when use generic mode you should write h0 no ack bit rather than write this register to make peer no ack or ack.
 */
void TPLL_EnableNoAck(unsigned char enable)
{
    if (enable) {
        write_reg8(0x170004, read_reg8(0x170004) | 0x40); //set reg_0x170004's bit6 to enable tx_noack
    } else {
        write_reg8(0x170004, read_reg8(0x170004) & 0xbf); //clear reg_0x170004's bit6 to disable tx_noack
    }
}

/**
 * @brief   Enables the ACK payload feature
 * @param   enable   Whether to enable or disable ACK payload
 */
void TPLL_Set_Local_Ack_Bit(unsigned char enable)
{
    if (!enable) {
        REG_ADDR8(0x170215) |= BIT(5);
    } else {
        REG_ADDR8(0x170215) &= ~BIT(5);
    }
}

/**
 * @brief       select tpll packet format.
 * @param       mode_format  packet format.
 * @return      none.
 */
void TPLL_SetFormatMode(TPLL_ModeFormatTypeDef mode_format)
{
    if (TPLL_MODE_LEGACY_FORMAT == mode_format) {
        rf_mode_format = TPLL_MODE_LEGACY_FORMAT;
    } else if (TPLL_MODE_GENERIC_FORMAT == mode_format) {
        rf_mode_format = TPLL_MODE_GENERIC_FORMAT;
    }
}

/**
 * @brief       Initiate the the Telink primary link layer module and Set the radio bitrate.
 * @param       bitrate  Radio bitrate.
 * @return      none.
 */
uint32_t TPLL_SetBitrate(TPLL_BitrateTypeDef bitrate)
{
    TPLL_VERIFY_TRUE(TPLL_STATE_MACHINE_STATUS_IDLE == REG_LINKLAYER_STATE_MACHINE_STATUS, TPLL_ERROR_BUSY);

    rf_mode_init();
    if (TPLL_MODE_LEGACY_FORMAT == rf_mode_format) {
        if (TPLL_BITRATE_1MBPS == bitrate) {
            rf_set_pri_1M_mode();
            write_reg8(0x170004, 0x9a); // 1m 9a //enable  ack flag
        } else if (TPLL_BITRATE_2MBPS == bitrate) {
            rf_set_pri_2M_mode();
            write_reg8(0x170004, 0x8a); //enable  ack flag
        } else if (TPLL_BITRATE_500kBPS == bitrate) {
            rf_set_pri_500K_mode();
            write_reg8(0x170005, 0x02);
            write_reg8(0x170004, 0x8a); //enable  ack flag
            write_reg8(0x170003, 0x47); //500K
        } else if (TPLL_BITRATE_250KBPS == bitrate) {
            rf_set_pri_250K_mode();
            write_reg8(0x170004, 0x8a); //enable  ack flag
            write_reg8(0x170003, 0x45); //250K
        }
    } else if (TPLL_MODE_GENERIC_FORMAT == rf_mode_format) {
        if (TPLL_BITRATE_1MBPS == bitrate) {
            rf_set_pri_generic_1M_mode();
            write_reg8(0x170004, 0x94); // 1m 9a //enable  ack flag
        } else if (TPLL_BITRATE_2MBPS == bitrate) {
            rf_set_pri_generic_2M_mode();
            write_reg8(0x170004, 0x84); //enable  ack flag
        } else if (TPLL_BITRATE_500kBPS == bitrate) {
            rf_set_pri_generic_500K_mode();
            write_reg8(0x170004, 0x44); //500K
        } else if (TPLL_BITRATE_250KBPS == bitrate) {
            rf_set_pri_generic_250K_mode();
            write_reg8(0x170004, 0x84); //enable  ack flag
            write_reg8(0x170003, 0x45); //250K
        }
        rf_set_pri_generic_noack_en();
    }

    write_reg8(0x170002, 0x48);                       //set preamble
    write_reg8(0x170215, 0xd0);                       //enable  ack flag

    write_reg8(0x170200, 0x80);                       // stop cmd
    write_reg8(0x170216, 0x29);                       // reg0xf16 pll_en_man and tx_en_dly_en  enable
    write_reg8(0x170028, read_reg8(0x170028) & 0xfe); // rx disable
    write_reg8(0x170202, read_reg8(0x170202) & 0xce); // reg0xf02 disable rx_en_man and tx_en_man

    return TPLL_SUCCESS;
}

/**
 * @brief       Set the radio output power.
 * @param       power   Output power.
 * @return      none.
 */
void TPLL_SetOutputPower(tpll_radio_power_t power)
{
    rf_set_power_level_index((rf_power_level_index_e)power);
}

/**
 * @brief       Set the channel to use for the radio.
 * @param       channel Channel to use for the radio.
 * @return      TPLL_SUCCESS                     If the operation completed successfully.
 * @return      TPLL_ERROR_BUSY                  If the function failed because the radio is busy.
 * @return      TPLL_ERROR_INVALID_PARAM         If the param invalid.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) uint32_t TPLL_SetRFChannel(signed short channel)
{
    uint8_t cmd = 0;
    write_reg8(REG_BB_LL_BASE_ADDR, 0x80); // stop first

    cmd = read_reg8(REG_BB_LL_BASE_ADDR);  // if not have this step, 0x140a24 sometime will not change to IDLE
    if (cmd != 0x80) {                     // just for compile error, meanwhile double confirm the BB_LL cmd reg
        write_reg8(REG_BB_LL_BASE_ADDR, 0x80);
    }

    TPLL_VERIFY_TRUE(TPLL_STATE_MACHINE_STATUS_IDLE == REG_LINKLAYER_STATE_MACHINE_STATUS, TPLL_ERROR_BUSY);
    TPLL_VERIFY_TRUE(channel <= 80, TPLL_ERROR_INVALID_PARAM);
    rf_set_chn(channel);

    return TPLL_SUCCESS;
}

/**
 * @brief       Set one pipe as a TX pipe.
 * @param       pipe_id Pipe to be set as a TX pipe.
 * @return      none.
 */
void TPLL_SetTXPipe(TPLL_PipeIDTypeDef pipe_id)
{
    if ((pipe_id >= TPLL_PIPE0) && (pipe_id <= TPLL_PIPE5)) {
        //enable PTX in the specified pipe
        unsigned char tmp = read_reg8(0x170215);
        tmp &= 0xf8;

        tmp |= pipe_id;

        tmp |= 0x10;
        write_reg8(0x170215, tmp);
    }
}

/**
 * @brief       Get the current TX pipe.
 * @param       none.
 * @return      The pipe set as a TX pipe.
*/
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) unsigned char TPLL_GetTXPipe(void)
{
    return (read_reg8(0x170215) & 0x07);
}

/**
 * @brief       Update the read pointer of the TX FIFO.
 * @param       pipe_id Pipe id.
 * @return      none.
 */
void TPLL_UpdateTXFifoRptr(TPLL_PipeIDTypeDef pipe_id)
{
    //adjust rptr manually
    reg_rf_dma_tx_rptr(pipe_id) = 0x80;
}

/**
 * @brief       Set the width of the address.
 * @param       address_width   Width of the TPLL address (in bytes).
 * @return      none.
 */
void TPLL_SetAddressWidth(TPLL_AddressWidthTypeDef address_width)
{
    rf_set_access_code_len(address_width);
    write_reg8(REG_BASEBAND_ACCESS_CODE_SYNC_THRESHOLD, address_width * 8); //set the length of address code sync window
}

/**
 * @brief       Get the width of the address.
 * @param       none.
 * @return      Width of the TPLL address width(in bytes).
 */
unsigned char TPLL_GetAddressWidth(void)
{
    return read_reg8(REG_BASEBAND_ACCESS_CODE_LEN) & 0x07;
}

/**
 * @brief       Check status for a selected pipe.
 * @param       pipe_id Pipe number to check status for.
 * @return      Pipe status.
 */
unsigned char TPLL_GetPipeStatus(TPLL_PipeIDTypeDef pipe_id)
{
    if (read_reg8(0x17044d) & BIT(pipe_id)) //MODEM_RX_CTRL_1

    {
        return 0x03;
    } else {
        return 0x00;
    }
}

#define ACCESS_CODE_BASE_PIPE0 (0x170008)

/**
*   @brief      This function serves to swap bit .
*   @param[in]  original    The objective needs be swaped.
*   @return     The result of swaping
*/
static unsigned char bit_swap8(unsigned char original)
{
    unsigned char ret = 0;
    int           i   = 0;
    for (i = 0; i < 8; i++) {
        if (original & 0x01) {
            ret |= 0x01;
        } else {
            ret &= 0xfe;
        }
        if (i == 7) {
            break;
        }
        ret <<= 1;
        original >>= 1;
    }
    return ret;
}

/**
 * @brief       Set the address for pipes.
 *              Beware of the difference for single and multibyte address registers
 * @param       pipe_id Radio pipe to set.
 * @param       addr    Buffer from which the address is stored in
 * @return      none.
 */
void TPLL_SetAddress(TPLL_PipeIDTypeDef pipe_id, const unsigned char *addr)
{
    unsigned char temp[5];
    unsigned char acc_len = read_reg8(0x170005) & 0x07;
    unsigned char i       = 0;
    for (i = 0; i < acc_len; i++) {
        temp[i] = bit_swap8(addr[i]);
    }
    rf_set_pipe_access_code(pipe_id, temp);
}

/**
 * @brief       Get the address for selected pipe.
 * @param       pipe_id Pipe for which to get the address.
 * @param       addr    Pointer t a buffer that address bytes are written to.
 *               <BR><BR>For pipes containing only LSB byte of address, this byte is returned.
 * @return      Numbers of bytes copied to addr.
 */
static void multi_byte_reg_read(unsigned int reg_start, unsigned char *buf, int len)
{
    int i = 0;
    for (i = 0; i < len; i++, reg_start++) {
        buf[i] = read_reg8(reg_start);
    }
}

/**
 * @brief       Get the address for selected pipe.
 * @param       pipe_id Pipe for which to get the address.
 * @param       addr    Pointer t a buffer that address bytes are written to.
 *               <BR><BR>For pipes containing only LSB byte of address, this byte is returned.
 * @return      Numbers of bytes copied to addr.
 */
unsigned char TPLL_GetAddress(unsigned char pipe_id, unsigned char *addr)
{
    unsigned char i = 0;
    unsigned char temp[7];
    unsigned char acc_len = read_reg8(0x170005) & 0x07;

    switch (pipe_id) {
    case 0:
    case 1:
        multi_byte_reg_read(ACCESS_CODE_BASE_PIPE0 + i + (pipe_id * 5), &temp[i], acc_len);
        break;
    case 2:
    case 3:
    case 4:
    case 5:
        multi_byte_reg_read(ACCESS_CODE_BASE_PIPE0 + (pipe_id * 2 + 6), &temp[0], 1);
        multi_byte_reg_read(ACCESS_CODE_BASE_PIPE0 + (pipe_id * 2 + 7), &temp[1], 1);
        for (i = 2; i < acc_len; i++) {
            multi_byte_reg_read(ACCESS_CODE_BASE_PIPE0 + i + 5, &temp[i], 1);
        }
        break;
    default:
        break;
    }

    for (i = 0; i < acc_len; i++) {
        addr[i] = bit_swap8(temp[i]);
    }

    return acc_len;
}

#define REG_BASEBAND_RX_PIPE_EN (0x17044d)

/**
 * @brief       Open one or all pipes.
 * @param       pipe_id Radio pipes to open.
 * @return      none.
 */
void TPLL_OpenPipe(TPLL_PipeIDTypeDef pipe_id)
{
    unsigned char tmp = read_reg8(REG_BASEBAND_RX_PIPE_EN) & 0x3f;
    switch (pipe_id) {
    case TPLL_PIPE0:
    case TPLL_PIPE1:
    case TPLL_PIPE2:
    case TPLL_PIPE3:
    case TPLL_PIPE4:
    case TPLL_PIPE5:
        tmp |= BIT(pipe_id);
        break;

    case TPLL_PIPE_ALL:
        tmp |= 0x3f;
        break;
    default:
        break;
    }

    write_reg8(REG_BASEBAND_RX_PIPE_EN, tmp);
}

/**
 * @brief       Close one or all pipes.
 * @param       pipe_id Radio pipes to close.
 * @return      none.
 */
void TPLL_ClosePipe(TPLL_PipeIDTypeDef pipe_id)
{
    unsigned char tmp = read_reg8(REG_BASEBAND_RX_PIPE_EN);

    switch (pipe_id) {
    case TPLL_PIPE0:
    case TPLL_PIPE1:
    case TPLL_PIPE2:
    case TPLL_PIPE3:
    case TPLL_PIPE4:
    case TPLL_PIPE5:
        tmp &= (~BIT(pipe_id));
        break;

    case TPLL_PIPE_ALL:
        tmp &= 0x00;
        break;
    default:
        break;
    }

    write_reg8(REG_BASEBAND_RX_PIPE_EN, tmp);
}

#define REG_LINKLAYER_AUTO_RETRY_TIMES (0x170214)
#define REG_LINKLAYER_AUTO_RETRY_DELAY (0x170210)

/**
 * @brief       Set the the number of retransmission attempts and the packet retransmit delay.
 * @param       retr_times  Number of retransmissions. Setting the parmater to 0 disables retransmission.
 * @param       retry_delay Delay between retransmissions.
 * @return      none.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) void TPLL_SetAutoRetry(unsigned char retry_times, unsigned short retry_delay)
{
    retry_times &= 0x0f; //accommodate with another chip
    write_reg8(REG_LINKLAYER_AUTO_RETRY_TIMES, retry_times);

    retry_delay &= 0x0fff;
    unsigned short tmp = read_reg16(REG_LINKLAYER_AUTO_RETRY_DELAY);
    tmp &= 0xf000;
    tmp |= retry_delay;
    write_reg16(REG_LINKLAYER_AUTO_RETRY_DELAY, tmp);
}

/**
 * @brief     This function serves to init tx/rx fifos.
 * @return    none.
 */
static void TPLL_InitFifos(void)
{
    // rx fifo init
    write_reg8(REG_BB_LL_BASE_ADDR, 0x80);  // stop first
    write_reg8(REG_DMA_RX_FIFO_RPTR, 0x80); // reset rptr

    // reset the wptr and rptr of all pipes' tx-fifos
    int i = 0;
    for (i = 0; i < TPLL_PIPE_NUM; i++) {
        write_reg8((REG_DMA_TX_FIFO_WPTR_PIPE0 + (i << 1)), 0);
        write_reg8((REG_DMA_TX_FIFO_RPTR_PIPE0 + (i << 1)), 0x80); // bit5, rptr clear
    }
}

/**
 * @brief     This function serves to init tx dma default buffer.
 * @return    none.
 */
void TPLL_InitDmaDefaultBuff(unsigned char *ptx_buffer)
{
#define TX_PKT_PAYLOAD 0
    if (TPLL_MODE_LEGACY_FORMAT == rf_mode_format) {
        u8 rf_data_len    = TX_PKT_PAYLOAD + 1;
        ptx_buffer[4]     = TX_PKT_PAYLOAD;
        u32 rf_tx_dma_len = RF_TX_PACKET_DMA_LEN(rf_data_len);
        ptx_buffer[3]     = (rf_tx_dma_len >> 24) & 0xff;
        ptx_buffer[2]     = (rf_tx_dma_len >> 16) & 0xff;
        ptx_buffer[1]     = (rf_tx_dma_len >> 8) & 0xff;
        ptx_buffer[0]     = rf_tx_dma_len & 0xff;
    } else if (TPLL_MODE_GENERIC_FORMAT == rf_mode_format) {
        unsigned char header_len  = (TPLL_GenericHeader.h0_size + TPLL_GenericHeader.h1_size + TPLL_GenericHeader.length_size) >> 3;
        unsigned int  rf_data_len = TX_PKT_PAYLOAD + header_len;

        unsigned long long header = TPLL_GenericHeader.h0_val | (TX_PKT_PAYLOAD << TPLL_GenericHeader.h0_size) |
                                    (TPLL_GenericHeader.h1_val << (TPLL_GenericHeader.h0_size + TPLL_GenericHeader.length_size));
        for (unsigned char i = 0; i < header_len; i++) {
            ptx_buffer[4 + i] = (header >> (8 * i)) & 0xff;
        }

        unsigned int rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
        ptx_buffer[3]              = (rf_tx_dma_len >> 24) & 0xff;
        ptx_buffer[2]              = (rf_tx_dma_len >> 16) & 0xff;
        ptx_buffer[1]              = (rf_tx_dma_len >> 8) & 0xff;
        ptx_buffer[0]              = rf_tx_dma_len & 0xff;
    }

    rf_dma_set_src_address(RF_TX_DMA, (unsigned int)(ptx_buffer));
}

/**
 * @brief     This function serves to inti rx/tx dma setting.
 * @param       rx_buffer  rx buffer address.
 * @param       tx_buffer  tx buffer address.
 * @return    none.
 */
void TPLL_DmaInit(unsigned char *rx_buffer, unsigned char *tx_buffer)
{
    TPLL_InitFifos();
    TPLL_InitDmaDefaultBuff(tx_buffer);
    rf_set_tx_dma(TPLL_TX_FIFO_DEP, TPLL_TX_FIFO_SIZE);
    rf_set_rx_dma(rx_buffer, TPLL_PIPE_RX_FIFO_NUM - 1, TPLL_PIPE_RX_FIFO_SIZE);
}

/**
 * @brief       Check if the TX FIFO is empty.
 * @param       pipe_id pipe id for which to check.
 * @return      1: the TX FIFO is empty; 0: the packet is not empty.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) unsigned char TPLL_TxFifoEmpty(TPLL_PipeIDTypeDef pipe_id)
{
    return (read_reg8((REG_DMA_TX_FIFO_WPTR_PIPE0 + (pipe_id << 1)))) ==
           (read_reg8((REG_DMA_TX_FIFO_RPTR_PIPE0 + (pipe_id << 1))));
}

/**
 * @brief       Check if TX FIFO is full.
 * @param       pipe_id pipe id for which to check.
 * @return      TRUE TX FIFO full or not.
 */
unsigned char TPLL_TxFifoFull(TPLL_PipeIDTypeDef pipe_id)
{
    return (read_reg8((REG_DMA_TX_FIFO_WPTR_PIPE0 + (pipe_id << 1)))) !=
           (read_reg8((REG_DMA_TX_FIFO_RPTR_PIPE0 + (pipe_id << 1))));
}

#define REG_LINKLAYER_RETRY_CNT (0x170225)

/**
 * @brief       Get the number of retransmission attempts when it goes max retransmission attempts it will be reset to 0.
 * @param       none.
 * @return      Number of retransmissions.
 */
unsigned char TPLL_GetTransmitAttempts(void)
{
    return (read_reg8(REG_LINKLAYER_RETRY_CNT));
}

#define TPLL_CARRIER_DETECT_THRESHOLD 0x30

/**
 * @brief       Get the carrier detect status.
 * @param       rx_buf  rx buffer address.
 * @return      Carrier detect status.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) unsigned char TPLL_GetCarrierDetect(unsigned char *rx_buf)
{
    unsigned char *rx_packet = NULL;
    unsigned char  rptr      = read_reg8(REG_DMA_RX_FIFO_RPTR);
    unsigned char  wptr      = read_reg8(REG_DMA_RX_FIFO_WPTR);
    unsigned char  ret       = 0;

    if ((rptr % TPLL_PIPE_RX_FIFO_NUM) != (wptr % TPLL_PIPE_RX_FIFO_NUM)) {
        rx_packet = rx_buf + (rptr % TPLL_PIPE_RX_FIFO_NUM) * TPLL_PIPE_RX_FIFO_SIZE;
        //read rssi in rx_packet[4]
        if (rx_packet[4] >= TPLL_CARRIER_DETECT_THRESHOLD) {
            ret = 1;
        }
    }

    return ret;
}

/**
 * @brief       Check if the RX FIFO is empty.
 * @param       pipe_id pipe id for which to check.
 * @return      1: the RX FIFO is empty; 0: the packet is not empty.
 */
unsigned char TPLL_RxFifoEmpty(void)
{
    return ((read_reg8(REG_DMA_RX_FIFO_RPTR) % TPLL_PIPE_RX_FIFO_NUM) == (read_reg8(REG_DMA_RX_FIFO_WPTR) % TPLL_PIPE_RX_FIFO_NUM));
}

/**
 * @brief       Check if RX FIFO is full.
 * @return      TRUE RX FIFO full or not.
 */
unsigned char TPLL_RxFifoFull(void)
{
    return (((read_reg8(REG_DMA_RX_FIFO_RPTR) + 31) % TPLL_PIPE_RX_FIFO_NUM) == (read_reg8(REG_DMA_RX_FIFO_WPTR) % TPLL_PIPE_RX_FIFO_NUM));
}

/**
 * @brief Function for reading an RX payload.
 * @param  rx_pload   Pointer to buffer where the RX payload is stored.
 * @param  rx_buf     rx buffer address.
 * @return TPLL_ReadRxPayload_t        param in TPLL_ReadRxPayload_t.
 *
 */
__attribute__((section(".ram_code"))) TPLL_ReadRxPayload_t TPLL_ReadRxPayload(unsigned char *rx_pload, unsigned char *rx_buf)
{
    uint8_t rptr = read_reg8(REG_DMA_RX_FIFO_RPTR);
    uint8_t wptr = read_reg8(REG_DMA_RX_FIFO_WPTR);

    unsigned char       *rx_packet = NULL;
    TPLL_ReadRxPayload_t TPLL_ReadRxPayload;
    unsigned int         i = 0;

    if ((rptr % TPLL_PIPE_RX_FIFO_NUM) != (wptr % TPLL_PIPE_RX_FIFO_NUM)) {
        rx_packet = rx_buf + (rptr % TPLL_PIPE_RX_FIFO_NUM) * TPLL_PIPE_RX_FIFO_SIZE;
        if (rf_mode_format == TPLL_MODE_LEGACY_FORMAT) {
            TPLL_ReadRxPayload.crc_len        = reg_rf_crc_config2 & (FLD_RF_CRC_LENGTH);
            TPLL_ReadRxPayload.header_len     = 1;
            TPLL_ReadRxPayload.rx_payload_len = rx_packet[4] & 0x3f; //get length of rx payload

            for (i = 0; i < TPLL_ReadRxPayload.rx_payload_len; i++) {
                rx_pload[i] = rx_packet[5 + i];
            }
            //paylaod + 2byte crc + 4byte timestamp
            TPLL_ReadRxPayload.rx_timestamp = rx_packet[rx_packet[0] - 4] | (rx_packet[rx_packet[0] - 3] << 8) | (rx_packet[rx_packet[0] - 2] << 16) | (rx_packet[rx_packet[0] - 1] << 24);

            TPLL_ReadRxPayload.rx_rssi = rx_packet[rx_packet[0] + 2];
            TPLL_ReadRxPayload.rx_pipe = (rx_packet[rx_packet[0] + 1] & 0x70) >> 4; //get rx pipe and payload length
        } else if (rf_mode_format == TPLL_MODE_GENERIC_FORMAT) {
            TPLL_ReadRxPayload.crc_len        = reg_rf_crc_config2 & (FLD_RF_CRC_LENGTH);
            TPLL_ReadRxPayload.header_len     = (TPLL_GenericHeader.h0_size + TPLL_GenericHeader.h1_size + TPLL_GenericHeader.length_size) / 8;
            TPLL_ReadRxPayload.rx_payload_len = rx_packet[0] - 8 - TPLL_ReadRxPayload.header_len - TPLL_ReadRxPayload.crc_len; //get length of rx payload

            for (i = 0; i < TPLL_ReadRxPayload.rx_payload_len; i++) {
                rx_pload[i] = rx_packet[4 + i + TPLL_ReadRxPayload.header_len];
            }
            //paylaod + 2byte crc + 4byte timestamp
            TPLL_ReadRxPayload.rx_timestamp = rx_packet[rx_packet[0] - 4] | (rx_packet[rx_packet[0] - 3] << 8) | (rx_packet[rx_packet[0] - 2] << 16) | (rx_packet[rx_packet[0] - 1] << 24);
            TPLL_ReadRxPayload.rx_rssi      = rx_packet[rx_packet[0] + 2];
            TPLL_ReadRxPayload.rx_pipe      = (rx_packet[rx_packet[0] + 1] & 0x70) >> 4; //get rx pipe and payload length
        }
        /*
         * it is a new way to adjust rptr of rx fifo
         * it make read same pid different crc packet possible
         */
        write_reg8(REG_DMA_RX_FIFO_RPTR, 0x40); // set bit6 to increase by one
    }

    return TPLL_ReadRxPayload;
}

/**
 * @brief       Get the RX timestamp.
 * @note        It is required to call TPLL_ReadRxPayload() before this function is called.
 * @param       none.
 * @return      RX timestamp.
 */
unsigned int TPLL_GetTimestamp(volatile unsigned char *rx_packet)
{
    unsigned int timestamp = 0;
    timestamp              = rx_packet[rx_packet[0] - 4] | (rx_packet[rx_packet[0] - 3] << 8) | (rx_packet[rx_packet[0] - 2] << 16) | (rx_packet[rx_packet[0] - 1] << 24);
    return timestamp;
}

/**
 * @brief       Get the RX RSSI.
 * @note        It is required to call TPLL_ReadRxPayload() before this function is called.
 * @param       none.
 * @return      RX RSSI.
 */
signed char TPLL_GetRxRssiValue(volatile unsigned char *rx_packet)
{
    signed char rssi = 0;
    rssi             = rx_packet[rx_packet[0] + 2];
    return rssi;
}

/**
 * @brief       Check if the crc of the received packet is valid.
 * @param       rx_packet.
 * @return      1: the packet crc is valid; 0: the packet crc is invalid.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) uint8_t TPLL_IsCrcValid(volatile uint8_t *rx_packet)
{
    if (rf_mode_format == TPLL_MODE_LEGACY_FORMAT) {
        return ((rx_packet[(rx_packet[4] & 0x3f) + 14] & 0x01) == 0x00);

    } else if (rf_mode_format == TPLL_MODE_GENERIC_FORMAT) {
        return ((rx_packet[(rx_packet[0]) + 3] & 0x01) == 0x00);
    } else {
        return 0;
    }
}

/**
 * @brief       Check whether the received legacy packet len is valid.
 * @param       none.
 * @return      1: the packet len is valid; 0: the packet len is invalid.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) uint8_t TPLL_IsPacketLenValid(uint8_t *rx_packet)
{
    if (rf_mode_format == TPLL_MODE_LEGACY_FORMAT) {
        return ((rx_packet[4] & 0x3f) <= TPLL_MAX_LEGACY_VARIABLE_PAYLOAD_LENGTH); // for compile error

    } else if (rf_mode_format == TPLL_MODE_GENERIC_FORMAT) {
        unsigned char header_len = ((TPLL_GenericHeader.h0_size + TPLL_GenericHeader.h1_size + TPLL_GenericHeader.length_size) / 8);
        unsigned char crc_len    = reg_rf_crc_config2 & (FLD_RF_CRC_LENGTH);
        return ((rx_packet[0] - 8 - header_len - crc_len) <= TPLL_MAX_GENERIC_VARIABLE_PAYLOAD_LENGTH); //get length of rx payload
    } else {
        return 0;
    }
}

/**
 * @brief       Check whether the received packet is valid.
 * @param       rx_buf  rx buffer address.
 * @return      1: the packet is valid; 0: the packet is invalid.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) unsigned char TPLL_IsRxPacketValid(unsigned char *rx_buf)
{
    unsigned char  rptr      = read_reg8(REG_DMA_RX_FIFO_RPTR);
    unsigned char  wptr      = read_reg8(REG_DMA_RX_FIFO_WPTR);
    unsigned char *rx_packet = NULL;

    if ((rptr % TPLL_PIPE_RX_FIFO_NUM) != (wptr % TPLL_PIPE_RX_FIFO_NUM)) {
        rx_packet = rx_buf + (rptr % TPLL_PIPE_RX_FIFO_NUM) * TPLL_PIPE_RX_FIFO_SIZE;
        if (TPLL_IsCrcValid(rx_packet) && TPLL_IsPacketLenValid(rx_packet)) {
            return 1;
        } else {
            //adjust rptr of rx fifo
            write_reg8(REG_DMA_RX_FIFO_RPTR, 0x40); //set bit5 to increase by one to discard the invalid packet
            return 0;
        }
    }
    return 0;
}

/**
 * @brief       Disable / enable sending an ACK packet when a crc error occurs.
 * @param       1:enable,0:disable.
 * @return      none.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) void TPLL_EnableCrcfilter(uint8_t enable)
{
    if (enable) {
        BM_SET(read_reg8(0x170030), BIT(0));
    } else {
        BM_CLR(read_reg8(0x170030), BIT(0));
    }
}

/**
 * @brief       Get the packet received.
 * @param       rx_buf  rx buffer address.
 * @return      rx_packet.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) volatile unsigned char *TPLL_GetRxPacket(unsigned char *rx_buf)
{
    unsigned char *rx_packet = rx_buf + (read_reg8(REG_DMA_RX_FIFO_RPTR) % TPLL_PIPE_RX_FIFO_NUM) * TPLL_PIPE_RX_FIFO_SIZE;
    return rx_packet;
}

/**
 * @brief       Get the pid of the received packet.
 * @param       rx_packet.
 * @return      packet id.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) unsigned char TPLL_GetRxPacketId(volatile unsigned char *rx_packet)
{
    if (rf_mode_format == TPLL_MODE_LEGACY_FORMAT) {
        return (rx_packet[4] >> 6);

    } else if (rf_mode_format == TPLL_MODE_GENERIC_FORMAT) {
        return rx_packet[4 + TPLL_GenericHeader.pid_start_bit / 8] & BIT_RNG(TPLL_GenericHeader.pid_start_bit % 8, TPLL_GenericHeader.pid_start_bit % 8 + 1);
    } else {
        return 0;
    }
}

/**
 * @brief       Get the crc of the received packet.
 * @param       rx_packet.
 * @return      packet crc.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) void TPLL_GetRxPacketCrc(volatile unsigned char *rx_packet, volatile unsigned char *crc)
{
    unsigned char crc_len = reg_rf_crc_config2 & (FLD_RF_CRC_LENGTH);
    if (rf_mode_format == TPLL_MODE_LEGACY_FORMAT) {
        for (int i = 0; i < crc_len; i++) {
            crc[i] = rx_packet[rx_packet[0] - 6 + i];
        }

    } else if (rf_mode_format == TPLL_MODE_GENERIC_FORMAT) {
        for (int i = 0; i < crc_len; i++) {
            crc[i] = rx_packet[rx_packet[0] - 6 + i];
        }
    }
}

/**
 * @brief       Check if the payload length is zero.
 * @param       rx_packet.
 * @return      1:payload length is zero,0:payload length is not zero.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) unsigned char TPLL_IsPacketEmpty(unsigned char *rx_packet)
{
    if (rf_mode_format == TPLL_MODE_LEGACY_FORMAT) {
        return ((rx_packet[0] == 0x0b) && ((rx_packet[4] & 0x3f) == 0x00));

    } else if (rf_mode_format == TPLL_MODE_GENERIC_FORMAT) {
        unsigned char header_len = (TPLL_GenericHeader.h0_size + TPLL_GenericHeader.h1_size + TPLL_GenericHeader.length_size) % 8;
        unsigned char crc_len    = reg_rf_crc_config2 & (FLD_RF_CRC_LENGTH);

        return ((rx_packet[0] == header_len + crc_len + 8));
    } else {
        return 0;
    }
}

/**
 * @brief       Get the local pid.
 * @return      local pid.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) unsigned char TPLL_GetLocalPid(TPLL_PipeIDTypeDef pipe_id)
{
    return (unsigned char)((read_reg16(0x170222) >> (pipe_id << 1)) & 0x03);
}

/**
 * @brief       Set the pid reset bit to 0.
 * @return      None.
 */
void TPLL_Pid_Reset_Disable(void)
{
    write_reg8(0x170201, 0xc0);
}

/** @brief Function for writing a payload for transmission or acknowledgement.
 *
 * @details This function writes a payload that is added to the tx fifo.
 *
 * @param[in]   pipe_id     tx pipe id.
 * @param[in]   tx_pload    Pointer to the array that contains the payload.
 * @param[in]   length      tx payload length.
 * @param[in]   ptx_buffer  tx buffer address.
 *
 * @retval  TPLL_SUCCESS                     If the payload was successfully queued for writing.
 * @retval  TPLL_ERROR_NULL                  If the required parameter was NULL.
 * @retval  TPLL_ERROR_INVALID_PARAM         If the param invalid.
 * @retval  TPLL_ERROR_NO_MEM                If the TX FIFO is full.
 * @retval  TPLL_ERROR_INVALID_LENGTH        If the payload length was invalid (zero or larger than the allowed
 * maximum).
 */
__attribute__((section(".ram_code"))) uint32_t TPLL_WriteTxPayload(unsigned char pipe_id, unsigned char *ptx_buffer, const unsigned char *tx_pload, unsigned int length)
{
    uint8_t        wptr, rptr;
    unsigned char *p_tx_fifo;
    TPLL_VERIFY_PARAM_NOT_NULL(tx_pload);

    TPLL_VERIFY_TRUE(pipe_id < TPLL_PIPE_NUM, TPLL_ERROR_INVALID_PARAM);


    wptr = read_reg8(REG_DMA_TX_FIFO_WPTR_PIPE0 + (pipe_id << 1));
    rptr = read_reg8(REG_DMA_TX_FIFO_RPTR_PIPE0 + (pipe_id << 1));

    wptr = (wptr + 1) % (32 + 1);
    if (wptr == 0) { //skip default packet
        wptr = 1;
    }

    if (((wptr + 1) % TPLL_TX_FIFO_NUM) == (rptr % TPLL_TX_FIFO_NUM)) { // if the tx-fifo is full, return  immediately
        return TPLL_ERROR_NO_MEM;
    }

    p_tx_fifo = ptx_buffer + pipe_id * TPLL_TX_FIFO_SIZE * TPLL_TX_FIFO_NUM + (wptr % (TPLL_TX_FIFO_NUM + 1)) * TPLL_TX_FIFO_SIZE;
    if (TPLL_MODE_LEGACY_FORMAT == rf_mode_format) {
        if (length == 0 || length > TPLL_MAX_LEGACY_VARIABLE_PAYLOAD_LENGTH) {
            return TPLL_ERROR_INVALID_LENGTH;
        }
        unsigned char rf_data_len   = length + 1;
        unsigned int  rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
        p_tx_fifo[4]                = length;
        p_tx_fifo[3]                = (rf_tx_dma_len >> 24) & 0xff;
        p_tx_fifo[2]                = (rf_tx_dma_len >> 16) & 0xff;
        p_tx_fifo[1]                = (rf_tx_dma_len >> 8) & 0xff;
        p_tx_fifo[0]                = rf_tx_dma_len & 0xff;

        for (unsigned int i = 0; i < length; i++) {
            p_tx_fifo[5 + i] = tx_pload[i];
        }
    } else if (TPLL_MODE_GENERIC_FORMAT == rf_mode_format) {
        if (length == 0 || length > TPLL_MAX_GENERIC_VARIABLE_PAYLOAD_LENGTH) {
            return TPLL_ERROR_INVALID_LENGTH;
        }
        unsigned char      header_len  = (TPLL_GenericHeader.h0_size + TPLL_GenericHeader.h1_size + TPLL_GenericHeader.length_size) >> 3;
        unsigned int       rf_data_len = length + header_len;
        unsigned long long header      = TPLL_GenericHeader.h0_val | (TPLL_GenericHeader.length_val << TPLL_GenericHeader.h0_size) |
                                    (TPLL_GenericHeader.h1_val << (TPLL_GenericHeader.h0_size + TPLL_GenericHeader.length_size));
        for (unsigned char i = 0; i < header_len; i++) {
            p_tx_fifo[4 + i] = (header >> (8 * i)) & 0xff;
        }

        unsigned int rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
        p_tx_fifo[3]               = (rf_tx_dma_len >> 24) & 0xff;
        p_tx_fifo[2]               = (rf_tx_dma_len >> 16) & 0xff;
        p_tx_fifo[1]               = (rf_tx_dma_len >> 8) & 0xff;
        p_tx_fifo[0]               = rf_tx_dma_len & 0xff;

        for (unsigned int i = 0; i < length; i++) {
            p_tx_fifo[header_len + 4 + i] = tx_pload[i];
        }
    }

    TPLL_SetTXPipe(pipe_id);
    write_reg8(REG_DMA_TX_FIFO_WPTR_PIPE0 + (pipe_id << 1), wptr);

    return TPLL_SUCCESS;
}

/**
 * @brief       Write payload bytes of the ACK packet.
 *              Writes the payload that will be transmitted with the ack on the given pipe.
 * @param       pipe_id     Pipe that transmits the payload.
 * @param       payload     Pointer to the payload data.
 * @param       length      Size of the data to transmit.
 * @param       ptx_buffer  tx buffer address.
 * @return
 */
void TPLL_WriteAckPayload(unsigned char pipe_id, unsigned char *ptx_buffer, const unsigned char *tx_pload, unsigned char length)
{
    TPLL_WriteTxPayload(pipe_id, ptx_buffer, tx_pload, length);
}

/**
 * @brief       Trigger the transmission in the specified pipe
 * @param       none.
 * @return      TPLL_SUCCESS                     If the operation completed successfully.
 * @return      TPLL_ERROR_BUSY                  If the function failed because the radio is busy.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) int TPLL_PTXTrig(void)
{
    TPLL_VERIFY_TRUE(TPLL_STATE_MACHINE_STATUS_IDLE == REG_LINKLAYER_STATE_MACHINE_STATUS, TPLL_ERROR_BUSY);
    write_reg8(REG_BB_LL_BASE_ADDR, 0x80);     // stop first

    if (TPLL_STATE_MACHINE_STATUS_IDLE == REG_LINKLAYER_STATE_MACHINE_STATUS) {
        write_reg8(REG_BB_LL_BASE_ADDR, 0x83); //trig
                                               //        while(TPLL_STATE_MACHINE_STATUS_IDLE == REG_LINKLAYER_STATE_MACHINE_STATUS);

        return TPLL_SUCCESS;
    }

    return TPLL_ERROR_BUSY;
}

/**
 * @brief       Trigger the receiver activity in the specified pipe
 * @param       none.
 * @return      TPLL_SUCCESS                     If the operation completed successfully.
 * @return      TPLL_ERROR_BUSY                  If the function failed because the radio is busy.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) int TPLL_PRXTrig(void)
{
    TPLL_VERIFY_TRUE(TPLL_STATE_MACHINE_STATUS_IDLE == REG_LINKLAYER_STATE_MACHINE_STATUS, TPLL_ERROR_BUSY);
    write_reg8(REG_BB_LL_BASE_ADDR, 0x80);     // stop first

    if (TPLL_STATE_MACHINE_STATUS_IDLE == REG_LINKLAYER_STATE_MACHINE_STATUS) {
        write_reg8(REG_BB_LL_BASE_ADDR, 0x84); //trig

                                               //  rf_set_rxpara();

        //        while(TPLL_STATE_MACHINE_STATUS_IDLE == REG_LINKLAYER_STATE_MACHINE_STATUS);

        return TPLL_SUCCESS;
    }

    return TPLL_ERROR_BUSY;
}

#define REG_LINKLAYER_PTX_RX_WAIT (0x170206)

/**
 * @brief       Set the wait time between the end of an Ack-required packet's transmission
 *              and the start of Ack receiving to accommodate with another chip.
 * @param       wait_us Wait time between the end of an Ack-required packet's transmission
 *              and the start of Ack receiving.
 * @return      TPLL_SUCCESS                         If the operation completed successfully.
 * @return      TPLL_ERROR_INVALID_LENGTH            If the param was invalid.
 */
int TPLL_RxWaitSet(unsigned short wait_us)
{
    unsigned short tmp;

    if ((wait_us == 0) || (wait_us > TPLL_RX_WAIT_TIME_MAX)) {
        return TPLL_ERROR_INVALID_PARAM;
    }

    tmp = read_reg16(REG_LINKLAYER_PTX_RX_WAIT) & 0xf000;
    tmp |= (wait_us - 1);
    write_reg16(REG_LINKLAYER_PTX_RX_WAIT, tmp);

    return TPLL_SUCCESS;
}

#define REG_LINKLAYER_PRX_TX_WAIT (0x17020e)

/**
 * @brief       Set the TX wait time.
 * @param       wait_us   Period in microseconds.
 * @return      TPLL_SUCCESS                         If the operation completed successfully.
 * @return      TPLL_ERROR_INVALID_LENGTH            If the param was invalid.
 */
int TPLL_TxWaitSet(unsigned short wait_us)
{
    unsigned short tmp;

    if ((wait_us == 0) || (wait_us > TPLL_TX_WAIT_TIME_MAX)) {
        return TPLL_ERROR_INVALID_PARAM;
    }

    tmp = read_reg16(REG_LINKLAYER_PRX_TX_WAIT) & 0xf000;
    tmp |= (wait_us - 1);
    write_reg16(REG_LINKLAYER_PRX_TX_WAIT, tmp);

    return TPLL_SUCCESS;
}

#define REG_LINKLAYER_PTX_RX_TIME (0x17020a)

/**
 * @brief       Set the rx duration when an Ack-required packet has been
 *              transmitted and an Ack is expected.
 * @param       period_us   specifies the time of rx duration.
 * @return      TPLL_SUCCESS                         If the operation completed successfully.
 * @return      TPLL_ERROR_INVALID_LENGTH            If the param was invalid.
 */
int TPLL_RxTimeoutSet(unsigned short period_us)
{
    unsigned short tmp;

    if ((period_us < TPLL_RX_SETTLE_TIME_MIN) || (period_us > TPLL_RX_TIMEOUT_TIME_MAX)) {
        return TPLL_ERROR_INVALID_PARAM;
    }

    period_us &= 0x0fff;
    tmp = read_reg16(REG_LINKLAYER_PTX_RX_TIME) & 0xf000;
    tmp |= period_us;
    write_reg16(REG_LINKLAYER_PTX_RX_TIME, tmp);

    return TPLL_SUCCESS;
}

#define REG_LINKLAYER_PRX_TX_SETTLE (0x170204)

/**
 * @brief       Set the TX Settle phase.
 * @param       period_us   specifies the time.
 * @return      TPLL_SUCCESS                         If the operation completed successfully.
 * @return      TPLL_ERROR_INVALID_LENGTH            If the param was invalid.
 */
int TPLL_TxSettleSet(unsigned short period_us)
{
    unsigned short tmp;

    if ((period_us < TPLL_TX_SETTLE_TIME_MIN) || (period_us > TPLL_TX_SETTLE_TIME_MAX)) {
        return TPLL_ERROR_INVALID_PARAM;
    }
    period_us &= 0x0fff;
    tmp = read_reg16(REG_LINKLAYER_PRX_TX_SETTLE) & 0xf000;
    tmp |= period_us;
    write_reg16(REG_LINKLAYER_PRX_TX_SETTLE, tmp);

    return TPLL_SUCCESS;
}

#define REG_LINKLAYER_PRX_RX_SETTLE (0x17020c)

/**
 * @brief       Set the RX Settle phase.
 * @param       period_us   specifies the time.
 * @return      TPLL_SUCCESS                         If the operation completed successfully.
 * @return      TPLL_ERROR_INVALID_LENGTH            If the param was invalid.
 */
int TPLL_RxSettleSet(unsigned short period_us)
{
    unsigned short tmp;

    if ((period_us < TPLL_RX_SETTLE_TIME_MIN) || (period_us > TPLL_RX_SETTLE_TIME_MAX)) {
        return TPLL_ERROR_INVALID_PARAM;
    }

    period_us &= 0x0fff;
    tmp = read_reg16(REG_LINKLAYER_PRX_RX_SETTLE) & 0xf000;
    tmp |= period_us;
    write_reg16(REG_LINKLAYER_PRX_RX_SETTLE, tmp);

    return TPLL_SUCCESS;
}

/**
 * @brief   This function is used to set the initial PID value of PRX.
 * @param[in]   rx_pid  -Initial value of pid for PTX. (PID range:0~3)
 * @return  none
 */
static inline void rf_set_prx_init_pid(unsigned char rx_pid)
{
    reg_rf_ll_ctrl_1 |= ((rx_pid & 0x03) << 4);
}

/**
 * @brief   This function is used to set the initial PID value of PTX.
 * @param[in]   tx_pid  -Initial value of pid for PTX. (PID range:0~3)
 * @return  none
 */
static inline void rf_set_ptx_init_pid(unsigned char tx_pid)
{
    reg_rf_ll_ctrl_1 |= ((tx_pid & 0x03) << 6);
}

/**
 * @brief       Set the mode of TPLL radio.
 * @param       mode    TPLL_MODE_PTX or TPLL_MODE_PRX.
 * @return      none.
 */
void TPLL_ModeSet(TPLL_ModeTypeDef mode)
{
    if (TPLL_MODE_PTX == mode) {
        write_reg8(0x170202, read_reg8(0x170202) & 0xfe);      // md_dis
        write_reg8(0x170203, read_reg8(0x170203) & (~BIT(3))); //This bit is required to be turned off in private mode, this bit is only for BLE mode to enable crc related interrupts in BLE mode.
        reg_rf_ll_ctrl2 &= ~FLD_RF_R_NOACK_RETRY_CNT_EN;       //noack_retry_count_dis

        //PID_EN
        if (TPLL_MODE_LEGACY_FORMAT == rf_mode_format) {
            reg_rf_ll_ctrl2 |= FLD_RF_R_REP_SN_PID_EN;
        } else if (TPLL_MODE_GENERIC_FORMAT == rf_mode_format) {
            TPLL_PlayloadBitOrder(1);
            rf_set_pri_generic_noack_en();
            rf_set_pri_generic_pid_en();
            rf_set_pri_generic_header_size(TPLL_GenericHeader.h0_size, TPLL_GenericHeader.length_size, TPLL_GenericHeader.h1_size);
            rf_set_pri_generic_length_adj(reg_rf_crc_config2 & (FLD_RF_CRC_LENGTH));
            rf_set_pri_generic_pid_start_bit(TPLL_GenericHeader.pid_start_bit);
            rf_set_pri_generic_noack_start_bit(TPLL_GenericHeader.noack_start_bit);
        }

        rf_set_ptx_init_pid(0);
    } else if (TPLL_MODE_PRX == mode) {
        write_reg8(0x170202, read_reg8(0x170202) & 0xfe);      // md_dis
        write_reg8(0x170203, read_reg8(0x170203) & (~BIT(3))); //This bit is required to be turned off in private mode, this bit is only for BLE mode to enable crc related interrupts in BLE mode.
        write_reg8(0x170203, read_reg8(0x170203) & 0xf0);      //rx timeout disable
        reg_rf_ll_ctrl2 &= ~FLD_RF_R_NOACK_RETRY_CNT_EN;       //noack_retry_count_dis

        //PID_EN
        if (TPLL_MODE_LEGACY_FORMAT == rf_mode_format) {
            reg_rf_ll_ctrl2 |= FLD_RF_R_REP_SN_PID_EN;
        } else if (TPLL_MODE_GENERIC_FORMAT == rf_mode_format) {
            TPLL_PlayloadBitOrder(1);
            rf_set_pri_generic_noack_en();
            rf_set_pri_generic_pid_en();
            rf_set_pri_generic_header_size(TPLL_GenericHeader.h0_size, TPLL_GenericHeader.length_size, TPLL_GenericHeader.h1_size);
            rf_set_pri_generic_length_adj(reg_rf_crc_config2 & (FLD_RF_CRC_LENGTH));
            rf_set_pri_generic_pid_start_bit(TPLL_GenericHeader.pid_start_bit);
            rf_set_pri_generic_noack_start_bit(TPLL_GenericHeader.noack_start_bit);
        }

        write_reg8(0x170201, (read_reg8(0x170201) & 0xc0) | 0x3f); //reset pid1~5
        write_reg8(0x170215, 0xc0);                                //chn tx_manual off
        TPLL_EnableCrcfilter(1);
        rf_set_prx_init_pid(3);
    }
}

/**
 * @brief       Stop the TPLL state machine.
 * @param       none.
 * @return      none.
 */
void TPLL_ModeStop(void)
{
    write_reg8(REG_BB_LL_BASE_ADDR, read_reg8(REG_BB_LL_BASE_ADDR) & 0xf0);
}

/**
 * @brief       Remove remaining items from the RX buffer.
 * @param       none.
 * @return      none.
 */
void TPLL_FlushRx(void)
{
    write_reg8(REG_DMA_RX_FIFO_RPTR, 0x80);
}

/**
 * @brief       Reuse the last transmitted payload for the next packet.
 * @param       pipe_id pipe id.
 * @return      none.
 */
void TPLL_ReuseTx(TPLL_PipeIDTypeDef pipe_id)
{
    unsigned char wptr = read_reg8(REG_DMA_TX_FIFO_WPTR_PIPE0 + (pipe_id << 1));
    unsigned char rptr = (wptr + 31) % 32; //move rptr one step backward from wptr
    rptr |= 0x20;                          //rx_rptr_set
    write_reg8((REG_DMA_TX_FIFO_RPTR_PIPE0 + (pipe_id << 1)), rptr);
}

/**
 * @brief       Remove remaining items from the TX buffer.
 * @param       pipe_id Pipe id.
 * @return      none.
 */
__attribute__((section(".ram_code"))) __attribute__((optimize("-Os"))) void TPLL_FlushTx(TPLL_PipeIDTypeDef pipe_id)
{
    write_reg8((REG_DMA_TX_FIFO_RPTR_PIPE0 + (pipe_id << 1)), 0x80);
}

/**
 * @brief      Set the length of the preamble field of the on-air data packet.
 * @note       The valid range of this parameter is 1-16.
 * @param      preamble_len Preamble length.
 * @return     none.
 */
void TPLL_Preamble_Set(unsigned char preamble_len)
{
    if ((1 <= preamble_len) && (preamble_len <= 16)) {
        write_reg8(REG_BASEBAND_BASE_ADDR+0x02, (read_reg8(REG_BASEBAND_BASE_ADDR+0x02) & 0xe0) | preamble_len);
    }
}

/**
 * @brief      Read the length of the preamble field of the on-air data packet.
 * @param      none.
 * @return     Preamble length.
 */
unsigned char TPLL_Preamble_Read(void)
{
    unsigned char preamble_len;

    preamble_len = read_reg8(REG_BASEBAND_BASE_ADDR+0x02) & 0x1f;

    return preamble_len;
}

/**
 * @brief      disable the receiver preamble detection banking duiring the first byte of pdu.
 * @param      none.
 * @return     none.
 */
void TPLL_Preamble_Detect_Disable(void)
{
    write_reg8(0x170479, read_reg8(0x170479) | 0x08);
}

/**
 *  @brief      this function serve to reset buffer.
 *  @param[in]  none.
 *  @return     none.
*/
void TPLL_ResetBuf(void)
{
    write_reg8(REG_BB_LL_BASE_ADDR, 0x80);
    write_reg8(REG_DMA_RX_FIFO_RPTR, 0x80);                        //reset rptr
    for (u8 i = 0; i < TPLL_PIPE_NUM; i++) {
        write_reg8((REG_DMA_TX_FIFO_WPTR_PIPE0 + (i << 1)), 0);
        write_reg8((REG_DMA_TX_FIFO_RPTR_PIPE0 + (i << 1)), 0x80); //bit4, rptr clear
    }
}

/**
 * @brief       This function set generic packet header.
 * @param[in]   TPLL_GenericHeader_t - generic packet header parameters
 * @return      none.
 */
void TPLL_GenericHeaderSet(TPLL_GenericHeader_t GenericHeader)
{
    rf_set_pri_generic_header_size(GenericHeader.h0_size, GenericHeader.length_size, GenericHeader.h1_size);
    rf_set_pri_generic_pid_start_bit(GenericHeader.pid_start_bit);
    rf_set_pri_generic_noack_start_bit(GenericHeader.noack_start_bit);
}

/**
 * @brief       This function set generic packet filter.
 * @param[in]   TPLL_CrcConfig_t - generic packet filtering parameters
 * @return      none.
 */
void TPLL_CrcSet(TPLL_CrcConfig_t crc_cfg)
{
    rf_set_crc_config((rf_crc_config_t *)&crc_cfg);
    if (TPLL_MODE_GENERIC_FORMAT == rf_mode_format) {
        rf_set_pri_generic_length_adj(crc_cfg.len);
    }
}

/**
 * @brief       This function set the packet filter.
 * @param[in]   rf_pkt_flt_t - RF packet filtering parameters
 * @return      none.
 */
void TPLL_PktFilter(rf_pkt_flt_t PktFlt)
{
    rf_set_pkt_filter(PktFlt);
}

/**
 * @brief   This function serve to enable/disable generic format mode ack
 * @return  none.
 */
void TPLL_GenericNoAckEnable(void)
{
    rf_set_pri_generic_noack_en();
}

/**
 * @brief   This function serve to enable/disable generic format mode pid
 * @return  none.
 */
void TPLL_GenericPidEnable(void)
{
    rf_set_pri_generic_pid_en();
}

/**
 * @brief  This function serve to enable new/old crc value check for PRX new/old pkt
 * @return none.
 */
static void rf_prx_crc_check_en(void)
{
    write_reg8(0x170201, read_reg8(0x170201) | BIT(6));
}

/**
 * @brief  This function serve to disable new/old crc value check for PRX new/old pkt
 * @return none.
 */
static void rf_prx_crc_check_dis(void)
{
    write_reg8(0x170201, read_reg8(0x170201) & (~BIT(6)));
}

/**
 * @brief   This function serve to enable/disable new/old crc value check for PRX new/old pkt
 * @param[in]   1:enable 0:disable
 * @return  none.
 */
void TPLL_PrxCrcCheckEnable(unsigned char enable)
{
    if (enable) {
        rf_prx_crc_check_en();
    } else {
        rf_prx_crc_check_dis();
    }
}

/**
 * @brief This function serve to set the private ack enable,mainly used in prx/ptx.
 * @param[in] none
 * @return        none
 */
static void rf_set_ptx_prx_ack_en(void)
{
    write_reg8(0x170004, read_reg8(0x170004) & (~BIT(6)));
    write_reg8(0x170215, read_reg8(0x170215) & (~BIT(5)));
}

/**
 * @brief  This function serve to disable the private ack ,mainly used in prx/ptx.
 * @param[in]  none
 * @return     none
 */
static void rf_set_ptx_prx_ack_dis(void)
{
    write_reg8(0x170004, read_reg8(0x170004) | BIT(6));    //bit6 1
    write_reg8(0x170215, read_reg8(0x170215) & (~BIT(5))); //bit5 0
}

/**
 * @brief   This function serve to enable/disable PTX/PRX ACK.
 * @return  none.
 */
void TPLL_PtxPrxAckEnable(unsigned char Enable)
{
    if (Enable) {
        rf_set_ptx_prx_ack_en();
    } else {
        rf_set_ptx_prx_ack_dis();
    }
}

/**
 * @brief   This function serve to change payload bit order.
 * @param[in]   1: MSBit (Most Significant Bit) of the LSByte (Least Significant Byte) first
 *              0: LSBit (Least Significant Bit) of the LSByte (Least Significant Byte) first
 * @return  none.
 */
void TPLL_PlayloadBitOrder(unsigned char bitorder)
{
    if (bitorder) {
        write_reg8(0x170003, (read_reg8(0x80170003) | 0x08));
    } else {
        write_reg8(0x170003, (read_reg8(0x80170003) & 0xf7));
    }
}
