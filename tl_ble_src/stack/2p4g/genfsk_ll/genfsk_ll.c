/********************************************************************************************************
 * @file    genfsk_ll.c
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
#include "genfsk_ll.h"

_attribute_data_retention_sec_ static volatile signed short    gen_fsk_current_channel = 0;
_attribute_data_retention_sec_ static volatile unsigned char   gen_fsk_mode            = 0;
_attribute_data_retention_sec_ static volatile unsigned char   gen_fsk_fix_payload_len = 0;
_attribute_data_retention_sec_ static volatile gen_fsk_state_t gen_fsk_current_state   = GEN_FSK_STATE_TX;


#define REG_GENFSK_LL_CRC_EN          (0x80170000) // crc enable/disable
#define REG_GENFSK_LL_CRC_BYTES       (0x80170111) // crc bytes.
#define REG_GENFSK_LL_PREAMBLE_LENGTH (0x80170002) // preamble length


/** @brief Macro for verifying statement to be true. It will cause the exterior function to return
 *        err_code if the statement is not true.
 *
 * @param[in]   statement   Statement to test.
 * @param[in]   err_code    Error value to return if test was invalid.
 *
 * @retval      nothing, but will cause the exterior function to return @p err_code if @p statement
 *              is false.
 */
#define GEN_FSK_VERIFY_TRUE(statement, err_code) \
    do {                                         \
        if (!(statement)) {                      \
            return err_code;                     \
        }                                        \
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
#define GEN_FSK_VERIFY_FALSE(statement, err_code) \
    do {                                          \
        if ((statement)) {                        \
            return err_code;                      \
        }                                         \
    } while (0)

/** @brief Macro for verifying that the module is initialized. It will cause the exterior function to
 *        return if not.
 *
 * @param[in] param  The variable to check if is NULL.
 */
#define GEN_FSK_VERIFY_PARAM_NOT_NULL(param) GEN_FSK_VERIFY_FALSE(((param) == NULL), GEN_FSK_ERROR_NULL)

#if 1
_attribute_ram_code_sec_ static unsigned char bit_swap8(unsigned char original)
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
#endif

_attribute_ram_code_sec_noinline_ void gen_fsk_radio_power_set(gen_fsk_radio_power_t level)
{
    rf_set_power_level_index((rf_power_level_index_e)level);
}

_attribute_ram_code_sec_noinline_ void gen_fsk_set_crc_config(const rf_crc_config_t *config)
{
    rf_set_crc_config(config);
    if (GEN_FSK_MODE_GENERIC_VARIABLE_FORMAT == gen_fsk_mode) {
        rf_set_pri_generic_length_adj(config->len);
    }
}

_attribute_ram_code_sec_noinline_ void gen_fsk_datarate_set(gen_fsk_datarate_t datarate)
{
    rf_mode_init();

    if (GEN_FSK_MODE_GENERIC_VARIABLE_FORMAT == gen_fsk_mode) {
        switch (datarate) {
        case GEN_FSK_DATARATE_1MBPS:
            rf_set_pri_generic_1M_mode();
            break;
        case GEN_FSK_DATARATE_250KBPS:
            rf_set_pri_generic_250K_mode();
            break;
        case GEN_FSK_DATARATE_500KBPS:
            rf_set_pri_generic_500K_mode();
            break;

        case GEN_FSK_DATARATE_2MBPS:
            rf_set_pri_generic_2M_mode();
            break;
        default:
            break;
        }
        write_reg8(0x170004, ((read_reg8(0x170004) & 0xfc) | GEN_FSK_MODE_GENERIC_VARIABLE_FORMAT)); //select generic header mode
        rf_set_pri_generic_header_size(GEN_FSK_GenericHeader.h0_size, GEN_FSK_GenericHeader.length_size, GEN_FSK_GenericHeader.h1_size);
    } else if (GEN_FSK_MODE_FIXED_FORMAT == gen_fsk_mode || GEN_FSK_MODE_LEGACY_VARIABLE_FORMAT == gen_fsk_mode) {
        switch (datarate) {
        case GEN_FSK_DATARATE_250KBPS:
            rf_set_pri_250K_mode();
            break;
        case GEN_FSK_DATARATE_500KBPS:
            rf_set_pri_500K_mode();
            break;
        case GEN_FSK_DATARATE_1MBPS:
            rf_set_pri_1M_mode();
            break;
        case GEN_FSK_DATARATE_2MBPS:
            rf_set_pri_2M_mode();
            break;
        default:
            break;
        }
        if (GEN_FSK_MODE_FIXED_FORMAT == gen_fsk_mode) {
            rf_private_sb_en();
            rf_set_private_sb_len(gen_fsk_fix_payload_len);
        } else {
            write_reg8(0x170004, ((read_reg8(0x170004) & 0xfa) | GEN_FSK_MODE_LEGACY_VARIABLE_FORMAT)); //select tpll header mode
        }
    }
}

_attribute_ram_code_sec_noinline_ void gen_fsk_channel_set(signed short channel_num)
{
    gen_fsk_current_channel = channel_num;
    if (GEN_FSK_STATE_OFF != gen_fsk_current_state) {
        rf_set_trx_state((rf_status_e)gen_fsk_current_state, gen_fsk_current_channel);
    }
}

_attribute_ram_code_sec_noinline_ void gen_fsk_radio_state_set(gen_fsk_state_t state)
{
    gen_fsk_current_state = state;
    if (GEN_FSK_STATE_OFF == gen_fsk_current_state) {
        rf_set_tx_rx_off();
        return;
    }
    rf_set_trx_state((rf_status_e)gen_fsk_current_state, gen_fsk_current_channel);
}

_attribute_ram_code_sec_noinline_ void gen_fsk_preamble_len_set(unsigned char preamble_len)
{
    rf_set_preamble_len(preamble_len);
}

_attribute_ram_code_sec_noinline_ void gen_fsk_sync_word_len_set(gen_fsk_sync_word_len_t length)
{
    rf_set_access_code_len(length);
    write_reg8(0x17044e, length * 8); //set the length of address code sync window
}

_attribute_ram_code_sec_noinline_ void gen_fsk_sync_word_set(gen_fsk_pipe_id_t pipe_id, unsigned char *sync_word)
{
    unsigned char temp[7];
    unsigned char acc_len = read_reg8(0x170005) & 0x07;
    unsigned char i       = 0;
    for (i = 0; i < acc_len; i++) {
        // TODO change bit_swap8
        temp[i] = bit_swap8(sync_word[i]);
    }
    rf_set_pipe_access_code(pipe_id, temp);
}

_attribute_ram_code_sec_noinline_ void gen_fsk_pipe_open(gen_fsk_pipe_id_t pipe)
{
    unsigned char tmp = read_reg8(0x17044d);
    switch (pipe) {
    case GEN_FSK_PIPE0:
    case GEN_FSK_PIPE1:
    case GEN_FSK_PIPE2:
    case GEN_FSK_PIPE3:
    case GEN_FSK_PIPE4:
    case GEN_FSK_PIPE5:
    case GEN_FSK_PIPE6:
    case GEN_FSK_PIPE7:
        tmp |= BIT(pipe);
        break;

    case GEN_FSK_PIPE_ALL:
        tmp |= 0xff;
        break;
    default:
        break;
    }

    write_reg8(0x17044d, tmp);
}

_attribute_ram_code_sec_noinline_ void gen_fsk_pipe_close(gen_fsk_pipe_id_t pipe)
{
    unsigned char tmp = read_reg8(0x17044d);

    switch (pipe) {
    case GEN_FSK_PIPE0:
    case GEN_FSK_PIPE1:
    case GEN_FSK_PIPE2:
    case GEN_FSK_PIPE3:
    case GEN_FSK_PIPE4:
    case GEN_FSK_PIPE5:
    case GEN_FSK_PIPE6:
    case GEN_FSK_PIPE7:
        tmp &= (~BIT(pipe));
        break;

    case GEN_FSK_PIPE_ALL:
        tmp &= 0x00;
        break;
    default:
        break;
    }

    write_reg8(0x17044d, tmp);
}

_attribute_ram_code_sec_noinline_ void gen_fsk_tx_pipe_set(gen_fsk_pipe_id_t pipe)
{
    if ((pipe >= GEN_FSK_PIPE0) && (pipe <= GEN_FSK_PIPE7)) {
        //enable PTX in the specified pipe
        unsigned char tmp = read_reg8(0x170215);
        tmp &= 0xf8;

        tmp |= pipe;

        tmp |= 0x10;
        write_reg8(0x170215, tmp);
    }
}

_attribute_ram_code_sec_noinline_ void gen_fsk_packet_format_set(gen_fsk_packet_format_t mode, unsigned short legacy_fixed_plength)
{
    gen_fsk_mode            = mode;
    gen_fsk_fix_payload_len = legacy_fixed_plength;
}

_attribute_ram_code_sec_noinline_ void gen_fsk_set_pri_generic_length_adj(signed char length_adj)
{
    rf_set_pri_generic_length_adj(length_adj);
}

_attribute_ram_code_sec_noinline_ void gen_fsk_set_pkt_filter(rf_pkt_flt_t rf_pkt_flt)
{
    rf_set_pkt_filter(rf_pkt_flt);
}

_attribute_ram_code_sec_noinline_ void gen_fsk_crc_len_set(gen_fsk_crc_len_t crc_len)
{
    if (crc_len == CRC_DISABLE) {
        write_reg8(REG_GENFSK_LL_CRC_EN, (read_reg8(REG_GENFSK_LL_CRC_EN) & 0xfd)); // disable the crc
    } else {
        write_reg8(REG_GENFSK_LL_CRC_EN, (read_reg8(REG_GENFSK_LL_CRC_EN) | 0x02)); // enable the crc
        rf_set_crc_len(crc_len);
    }
}

_attribute_ram_code_sec_noinline_ void gen_fsk_rx_buffer_set(unsigned char *rx_buffer, unsigned char rx_buffer_num, unsigned short rx_buffer_len)
{
    rf_set_rx_dma(rx_buffer, rx_buffer_num - 1, rx_buffer_len);
}

_attribute_ram_code_sec_noinline_ void gen_fsk_tx_buffer_set(unsigned char tx_buffer_num, unsigned short tx_buffer_len)
{
    rf_set_tx_dma(tx_buffer_num - 1, tx_buffer_len);
}

_attribute_ram_code_sec_noinline_ unsigned int gen_fsk_write_payload(unsigned char *tx_buffer, const unsigned char *payload, unsigned int payload_length)
{
    GEN_FSK_VERIFY_PARAM_NOT_NULL(payload);
    GEN_FSK_VERIFY_PARAM_NOT_NULL(tx_buffer);

    if (GEN_FSK_MODE_FIXED_FORMAT == gen_fsk_mode) {
        if (payload_length == 0 || payload_length > GEN_FSK_MAX_VARIABLE_LENGTH) {
            return GEN_FSK_ERROR_INVALID_LENGTH;
        }
        unsigned int rf_tx_dma_len = rf_tx_packet_dma_len(payload_length);
        tx_buffer[3]               = (rf_tx_dma_len >> 24) & 0xff;
        tx_buffer[2]               = (rf_tx_dma_len >> 16) & 0xff;
        tx_buffer[1]               = (rf_tx_dma_len >> 8) & 0xff;
        tx_buffer[0]               = rf_tx_dma_len & 0xff;

        for (unsigned int i = 0; i < payload_length; i++) {
            tx_buffer[4 + i] = payload[i];
        }

    } else if (GEN_FSK_MODE_LEGACY_VARIABLE_FORMAT == gen_fsk_mode) {
        if (payload_length == 0 || payload_length > GEN_FSK_MAX_FIXED_LENGTH) {
            return GEN_FSK_ERROR_INVALID_LENGTH;
        }
        unsigned char rf_data_len   = payload_length + 1;
        unsigned int  rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
        tx_buffer[4]                = payload_length;
        tx_buffer[3]                = (rf_tx_dma_len >> 24) & 0xff;
        tx_buffer[2]                = (rf_tx_dma_len >> 16) & 0xff;
        tx_buffer[1]                = (rf_tx_dma_len >> 8) & 0xff;
        tx_buffer[0]                = rf_tx_dma_len & 0xff;

        for (unsigned int i = 0; i < payload_length; i++) {
            tx_buffer[5 + i] = payload[i];
        }
    } else if (GEN_FSK_MODE_GENERIC_VARIABLE_FORMAT == gen_fsk_mode) {
        if (payload_length == 0 || payload_length > GEN_FSK_MAX_GNC_VARIABLE_LENGTH) {
            return GEN_FSK_ERROR_INVALID_LENGTH;
        }
        unsigned char      header_len  = (GEN_FSK_GenericHeader.h0_size + GEN_FSK_GenericHeader.h1_size + GEN_FSK_GenericHeader.length_size) >> 3;
        unsigned int       rf_data_len = payload_length + header_len;
        unsigned long long header      = GEN_FSK_GenericHeader.h0_val | (GEN_FSK_GenericHeader.length_val << GEN_FSK_GenericHeader.h0_size) |
                                    (GEN_FSK_GenericHeader.h1_val << (GEN_FSK_GenericHeader.h0_size + GEN_FSK_GenericHeader.length_size));
        for (unsigned char i = 0; i < header_len; i++) {
            tx_buffer[4 + i] = (header >> (8 * i)) & 0xff;
        }

        unsigned int rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
        tx_buffer[3]               = (rf_tx_dma_len >> 24) & 0xff;
        tx_buffer[2]               = (rf_tx_dma_len >> 16) & 0xff;
        tx_buffer[1]               = (rf_tx_dma_len >> 8) & 0xff;
        tx_buffer[0]               = rf_tx_dma_len & 0xff;

        for (unsigned int i = 0; i < payload_length; i++) {
            tx_buffer[header_len + 4 + i] = payload[i];
        }
    }

    return GEN_FSK_SUCCESS;
}

_attribute_ram_code_sec_noinline_ unsigned char gen_fsk_is_rx_crc_ok(unsigned char *rx_buffer)
{
    if (GEN_FSK_MODE_LEGACY_VARIABLE_FORMAT == gen_fsk_mode) {
        return ((rx_buffer[(rx_buffer[4] & 0x3f) + 14] & 0x01) == 0x00);

    } else if (GEN_FSK_MODE_GENERIC_VARIABLE_FORMAT == gen_fsk_mode || GEN_FSK_MODE_FIXED_FORMAT == gen_fsk_mode) {
        return ((rx_buffer[rx_buffer[0] + 3] & 0x01) == 0x00);
    } else {
        return 0;
    }
}

_attribute_ram_code_sec_noinline_ unsigned char *gen_fsk_rx_payload_get(unsigned char *rx_buffer, unsigned char *payload_len)
{
    if (GEN_FSK_MODE_FIXED_FORMAT == gen_fsk_mode) {
        (*payload_len) = rx_buffer[0] - 10;
        return (rx_buffer + 4);
    } else if (GEN_FSK_MODE_LEGACY_VARIABLE_FORMAT == gen_fsk_mode) {
        (*payload_len) = rx_buffer[4] & 0x3f;
        return (rx_buffer + 5);
    } else if (GEN_FSK_MODE_GENERIC_VARIABLE_FORMAT == gen_fsk_mode) {
        unsigned char crc_len    = reg_rf_crc_config2 & (FLD_RF_CRC_LENGTH);
        unsigned char header_len = (GEN_FSK_GenericHeader.h0_size + GEN_FSK_GenericHeader.h1_size + GEN_FSK_GenericHeader.length_size) / 8;
        (*payload_len)           = rx_buffer[0] - 8 - header_len - crc_len; //get length of rx payload
        return (rx_buffer + 4 + header_len);
    } else {
        return 0;
    }
}

_attribute_ram_code_sec_noinline_ signed char gen_fsk_rx_packet_rssi_get(unsigned char *rx_buffer)
{
    return (signed char)((rx_buffer[rx_buffer[0] + 2]) - 110);
}

_attribute_ram_code_sec_noinline_ signed char gen_fsk_rx_instantaneous_rssi_get(void)
{
    return ((signed char)(read_reg8(0x8017045d)) - 110); //this function can not tested on fpga
}

_attribute_ram_code_sec_noinline_ unsigned int gen_fsk_rx_timestamp_get(unsigned char *rx_buffer)
{
    unsigned char *p = &rx_buffer[rx_buffer[0] - 4];
    return (unsigned int)((*p) | ((*(p + 1)) << 8) | ((*(p + 2)) << 16) | ((*(p + 3)) << 24));
}

_attribute_ram_code_sec_noinline_ void gen_fsk_tx_start(unsigned char *tx_buffer)
{
    rf_tx_pkt(tx_buffer);
}

_attribute_ram_code_sec_noinline_ unsigned char gen_fsk_is_tx_done(void)
{
    return ((read_reg8(0x80170220) & BIT(1)) == 0x02);
}

_attribute_ram_code_sec_noinline_ void gen_fsk_tx_done_status_clear(void)
{
    write_reg8(0x80170220, 0x02);
}

_attribute_ram_code_sec_noinline_ void gen_fsk_stx_start(unsigned char *tx_buffer, unsigned int start_point)
{
    rf_start_stx(tx_buffer, start_point);
}

_attribute_ram_code_sec_noinline_ void gen_fsk_srx_start(unsigned int start_point, unsigned int timeout_us)
{
    if (timeout_us) {
        write_reg8(0x170203, read_reg8(0x170203) | 0x02); //enable rx first timeout
        write_reg32(0x170228, timeout_us - 1);
    } else {
        //      write_reg8(0x170203, read_reg8(0x170203) & 0xfd); //disable rx first timeout
        reg_rf_ll_rx_fst_timeout = 0x0fffffff;   // first timeout.
    }
    reg_rf_ll_cmd_schedule = start_point;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x86;
}

_attribute_ram_code_sec_noinline_ void gen_fsk_stx2rx_start(unsigned char *tx_buffer, unsigned int start_point, unsigned int timeout_us)
{
    rf_dma_set_src_address(RF_TX_DMA, (unsigned int)(tx_buffer));
    write_reg8(0x170203, read_reg8(0x170203) | 0x04); //enable rx timeout
    write_reg16(0x17020a, timeout_us - 1);
    reg_rf_ll_cmd_schedule = start_point;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;      // Enable cmd_schedule mode.
                                                      //  reg_rf_ll_cmd = 0x87;   // single tx2rx.
    write_reg8(0x80170200, 0x87);                     // single tx2rx.
}

_attribute_ram_code_sec_noinline_ void gen_fsk_srx2tx_start(unsigned char *tx_buffer, unsigned int start_point, unsigned int timeout_us)
{
    if (timeout_us) {
        write_reg8(0x170203, read_reg8(0x170203) | 0x02); //enable rx first timeout
        write_reg32(0x170228, timeout_us - 1);
    } else {
        write_reg8(0x170203, read_reg8(0x170203) & 0xfd); //disable rx first timeout
    }
    reg_rf_ll_cmd_schedule = start_point;
    rf_dma_set_src_address(RF_TX_DMA, (unsigned int)(tx_buffer));
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    write_reg8(0x80170200, 0x88);                // single rx2tx
}

/**
 * @brief      This function sets the tx settle period of transceiver for automatic Single-TX , Single-TX-to-RX and Single-RX-to-TX.
 *             In those three automatic modes, the actual transmission starts a short while later after the transceiver enters TX
 *             state. That short while is so-called tx settle period which is used to wait for the RF PLL settling down before
 *             transmission. The TX settle period must be larger than 130uS. The default value is 150uS.
 * @param[in]  period_us  specifying the TX settle period in microsecond.
 * @param[out] none
 * @return     none
 */
_attribute_ram_code_sec_noinline_ void gen_fsk_tx_settle_set(unsigned short period_us)
{
    unsigned short tmp;

    period_us &= 0x0fff;
    tmp = read_reg16(0x170204) & 0xf000;
    tmp |= period_us;
    write_reg16(0x170204, tmp);
};

/**
 * @brief       This function set the packet filter.
 * @param[in]   rf_pkt_flt_t - RF packet filtering parameters
 * @return      none.
 */
void gen_fsk_pkt_filter(rf_pkt_flt_t PktFlt)
{
    rf_set_pkt_filter(PktFlt);
}

/**
 * @brief      This function sets the rx settle period of transceiver for automatic Single-RX, Single-RX-to-TX and Single-TX-to-RX.
 *             In those three automatic modes, the actual reception starts a short while later after the transceiver enters RX
 *             state. That short while is so-called rx settle period which is used to wait for the RF PLL settling down before
 *             reception. The RX settle period must be larger than 85uS. The default value is 90uS.
 * @param[in]  period_us  specifying the RX settle period in microsecond.
 * @param[out] none
 * @return     none
 */
_attribute_ram_code_sec_noinline_ void gen_fsk_rx_settle_set(unsigned short period_us)
{
    unsigned short tmp;

    period_us &= 0x0fff;
    tmp = read_reg16(0x17020c) & 0xf000;
    tmp |= period_us;
    write_reg16(0x17020c, tmp);
};

_attribute_ram_code_sec_noinline_ void gen_fsk_auto_pid_disable(void)
{
    write_reg8(0x170215, read_reg8(0x170215) & 0x7f);
}

_attribute_ram_code_sec_noinline_ void gen_fsk_set_pid(unsigned char *tx_buffer, unsigned char pid)
{
    tx_buffer[4] = (tx_buffer[4] & 0x3f) | ((pid & 0x03) << 6);
}
#if 1
/**
 * @brief       Set the frequency deviation of the transmitter, which follows the equation below.
 *              frequency deviation = bitrate/(modulation index)^2
 *              Note:configure this function before state_machine acting.
 * @param       mi_value    Modulation index.
 * @return      none.
 */
void gen_fsk_tx_set_mi(GEN_MIVauleTypeDef mi_value)
{
    rf_set_tx_modulation_index((rf_mi_value_e)mi_value);
}

/**
 * @brief       Set the frequency deviation of the transmitter, which follows the equation below.
 *              frequency deviation = bitrate/(modulation index)^2
 *              Note:configure this function before state_machine acting.
 * @param       mi_value    Modulation index.
 * @return      none.
 */
void gen_fsk_rx_set_mi(GEN_MIVauleTypeDef mi_value)
{
    rf_set_rx_modulation_index((rf_mi_value_e)mi_value);
}
#endif
/**
 *  @brief      This function serve to adjust tx/rx settle timing sequence.
 *  @param[in]  tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *              rx_settle_us    After adjusting the timing sequence, the time required for rx to settle.
 *  @return      0                   -  Correct configuration.
 *              -1                   -  Incorrect configuration.
*/
signed char gen_fsk_fast_settle_config(GEN_FSK_TxSettleTimeTypeDef tx_settle_us, GEN_FSK_RxSettleTimeTypeDef rx_settle_us)
{
    return rf_fast_settle_config((rf_tx_fast_settle_time_e)tx_settle_us, (rf_rx_fast_settle_time_e)rx_settle_us);
}

/**
 *  @brief      this function serve to enable the fast tx timing sequence adjusted.
 *  @param[in]  none.
 *  @return     none.
*/
void gen_fsk_fast_txsettleEnable(void)
{
    rf_tx_fast_settle_en();
}

/**
 *  @brief      this function serve to disable the fast tx timing sequence adjusted.
 *  @param[in]  none.
 *  @return     none.
*/
void gen_fsk_fast_txsettleDisable(void)
{
    rf_tx_fast_settle_dis();
}

/**
 *  @brief      this function serve to enable the fast rx timing sequence adjusted.
 *  @param[in]  none.
 *  @return     none.
*/
void gen_fsk_fast_rxsettleEnable(void)
{
    rf_rx_fast_settle_en();
}

/**
 *  @brief      this function serve to disable the fast rx timing sequence adjusted.
 *  @param[in]  none.
 *  @return     none.
*/
void gen_fsk_fast_rxsettleDisable(void)
{
    rf_rx_fast_settle_dis();
}
