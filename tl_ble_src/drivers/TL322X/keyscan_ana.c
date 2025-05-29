/********************************************************************************************************
 * @file    keyscan_ana.c
 *
 * @brief   This is the header file for tl322x
 *
 * @author  Driver Group
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

#include "keyscan_ana.h"

#define KEY_VALUE_MAX_THRESHOLD     200 //update according to actual keyboard

#define KEYSCAN_PIN_POWER_NUMBER    8 //for keyboard rows power
#define KEYSCAN_PIN_CHANNEL_NUMBER  4 //for analog switch 16 mux 1
#define KEYSCAN_PIN_NUMBER          (KEYSCAN_PIN_POWER_NUMBER + KEYSCAN_PIN_CHANNEL_NUMBER)

#if (KS_BOARD_TYPE == KS_EVK_KEYBOARD)
#define KS_ANALOG_SWITCH_EN         GPIO_PG6
#elif (KS_BOARD_TYPE == KS_FULL_KEYBOARD)
#define KS_ANALOG_SWITCH_EN         GPIO_PH2
#endif

/*
 * @brief More combination configurations refer to gpio_lookup table
 */
const int keyscan_pin_groups[KS_CHANNEL_POWER_PIN_TOTAL][KEYSCAN_PIN_NUMBER] = {
#if (KS_BOARD_TYPE == KS_EVK_KEYBOARD)
    {
        /* power 0-7 */
        GPIO_PH3, GPIO_PH4, GPIO_PH5, GPIO_PH6, GPIO_PH7, GPIO_PG0, GPIO_PG1, GPIO_PG2,
        /* channel A0-A3 */
        GPIO_PG7, GPIO_PH0, GPIO_PH1, GPIO_PH2},
#elif (KS_BOARD_TYPE == KS_FULL_KEYBOARD)
    {
    /* power 0-7 */
    GPIO_PH3, GPIO_PH4, GPIO_PH5, GPIO_PH6, GPIO_PH7, GPIO_PE0, GPIO_PE1, GPIO_PE2,
    /* channel A0-A3 */
    GPIO_PE3, GPIO_PE4, GPIO_PE5, GPIO_PE6},
#endif
    {
        /* power 0-7 */
        GPIO_PE3, GPIO_PE4, GPIO_PE5, GPIO_PE6, GPIO_PE7, GPIO_PF0, GPIO_PF1, GPIO_PF2,
        /* channel A0-A3 */
        GPIO_PD7, GPIO_PE0, GPIO_PE1, GPIO_PE2},
    {
        /* power 0-7 */
        GPIO_PF3, GPIO_PF4, GPIO_PF5, GPIO_PF6, GPIO_PF7, GPIO_PG0, GPIO_PG1, GPIO_PG2,
        /* channel A0-A3 */
        GPIO_PE7, GPIO_PF0, GPIO_PF1, GPIO_PF2},
    {
        /* power 0-7 */
        GPIO_PE3, GPIO_PE4, GPIO_PE5, GPIO_PE6, GPIO_PE7, GPIO_PF0, GPIO_PF1, GPIO_PF2,
        /* channel A0-A3 */
        GPIO_PD3, GPIO_PD4, GPIO_PD5, GPIO_PD6},
};

/*
 * @brief keyscan mode related registers setting
 */
const ks_config_t keyscan_config[KEYSCAN_MODE_MAX] = {
    {},//KEYSCAN_1XADC_MODE
    {},//KEYSCAN_2XADC_MODE
    {0x30, 0x26, 0x01, 0x00, 0x3b, 0x14, 0x80, 0x00, 0x05, 0x0a, 0x0f, 0x14, 0x19, 0x1e, 0x23, 0x1f, 0x24, 0x29, 0x2e, 0x32, 0x37, 0x3b, 0x3b},//KEYSCAN_1XADC_8K_ONCE_MODE
    {0x30, 0x15, 0x00, 0x00, 0x2c, 0x14, 0x80, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x1a, 0x1d, 0x1f, 0x22, 0x25, 0x27, 0x2a, 0x2b},//KEYSCAN_2XADC_8K_ONCE_MODE
    {0x30, 0x28, 0x01, 0x01, 0x3f, 0x14, 0x2a, 0x00, 0x05, 0x0a, 0x0f, 0x14, 0x19, 0x1e, 0x23, 0x20, 0x25, 0x2a, 0x2f, 0x34, 0x39, 0x3e, 0x3e},//KEYSCAN_2XADC_8K_TWICE_M1_MODE
    {0x30, 0x11, 0x00, 0x00, 0x3b, 0x14, 0x27, 0x00, 0x04, 0x09, 0x0d, 0x12, 0x17, 0x1c, 0x20, 0x1f, 0x24, 0x29, 0x2e, 0x32, 0x37, 0x3a, 0x3a} //KEYSCAN_2XADC_8K_TWICE_M2_MODE
};

dma_chain_config_t adc0_rx_dma_list_config;
dma_chain_config_t adc1_rx_dma_list_config;

extern dma_config_t adc_rx_dma_config;
extern dma_config_t adc1_rx_dma_config;

/**
 * @brief     This function serves to configure adc_dma_chn channel.
 * @param[in] chn          - the DMA channel
 * @param[in] src_addr     - the DMA source address
 * @param[in] data_buffer  - the DMA data_buffer address
 * @param[in] config       - the DMA config
 * @param[in] head_of_list - the DMA linked list head
 * @param[in] size_byte    - the DMA buffer size
 * @return    none
 */
static void keyscan_sar_adc_dma_config(dma_chn_e chn, unsigned int src_addr, unsigned short *data_buffer, dma_config_t *config, unsigned int size_byte)
{
    dma_set_address(chn, src_addr, (unsigned int)data_buffer);
    dma_set_size(chn, size_byte, DMA_WORD_WIDTH);
    dma_config(chn, config);
    dma_chn_en(chn);
}

/**
 * @brief     This function serves to configure adc_dma_chn channel llp.
 * @param[in] chn          - the DMA channel
 * @param[in] src_addr     - the DMA source address
 * @param[in] data_buffer  - the DMA data_buffer address
 * @param[in] config       - the DMA config
 * @param[in] head_of_list - the DMA linked list head
 * @param[in] size_byte    - the DMA buffer size
 * @return    none
 */
static void keyscan_sar_adc_dma_config_llp(dma_chn_e chn, unsigned int src_addr, unsigned short *data_buffer, dma_config_t *config, dma_chain_config_t *head_of_list, unsigned int size_byte)
{
    dma_set_address(chn, src_addr, (unsigned int)data_buffer);
    dma_set_size(chn, size_byte, DMA_WORD_WIDTH);
    dma_config(chn, config);
    reg_dma_llp(chn) = (unsigned int)(head_of_list);
}

/**
 * @brief      This function serves to configure adc_dma_chn channel list element.
 * @param[in]  chn          - the DMA channel
 * @param[in]  src_addr     - the DMA source address
 * @param[in]  data_buffer  - the DMA data_buffer address
 * @param[in]  node         - the list node
 * @param[in]  llpointer    - the linked list pointer
 * @param[in]  size_byte    - the DMA buffer size
 * @return     none
 */
static void keyscan_rx_dma_add_list_element(dma_chn_e chn, unsigned int src_addr, unsigned short *data_buffer, dma_chain_config_t *node, dma_chain_config_t *llpointer, unsigned int size_byte)
{
    node->dma_chain_ctl      = reg_dma_ctrl(chn) | BIT(0);
    node->dma_chain_src_addr = src_addr;
    node->dma_chain_dst_addr = (unsigned int)(data_buffer);
    node->dma_chain_data_len = dma_cal_size(size_byte, 4);
    node->dma_chain_llp_ptr  = (unsigned int)(llpointer);
}

/**
 * @brief      This function serves to get sar adc data with DMA.
 * @param[in]  chn              - the DMA channel
 * @param[in]  buffer_addr      - the DMA data_buffer address
 * @param[in]  size_byte        - the DMA buffer size 
 * @return     none
 */
void keyscan_get_sar_adc_data_dma(dma_chn_e chn, unsigned short* buffer_addr, unsigned int size_byte)
{
    keyscan_sar_adc_dma_config(chn, SAR_ADC_FIFO(ADC0), buffer_addr, &adc_rx_dma_config, size_byte);
}

/**
 * @brief      This function serves to get sar adc data with DMA LLP.
 * @param[in]  chn              - the DMA channel
 * @param[in]  buffer_addr      - the DMA data_buffer address
 * @param[in]  size_byte        - the DMA buffer size 
 * @return     none
 */
void keyscan_get_sar_adc_data_dma_llp(dma_chn_e chn, unsigned short* buffer_addr, unsigned int size_byte)
{
    keyscan_sar_adc_dma_config_llp(chn, SAR_ADC_FIFO(ADC0), buffer_addr, &adc_rx_dma_config, &adc0_rx_dma_list_config, size_byte);
    keyscan_rx_dma_add_list_element(chn, SAR_ADC_FIFO(ADC0), buffer_addr, &adc0_rx_dma_list_config, &adc0_rx_dma_list_config, size_byte);
    dma_chn_en(chn);
}

/**
 * @brief      This function serves to get sar adc0 adc1 data with DMA.
 * @param[in]  ks_mode          - the Keyscan mode
 * @param[in]  chn0             - the DMA channel
 * @param[in]  chn1             - the DMA channel
 * @param[in]  data_buffer0     - the DMA data_buffer0 address
 * @param[in]  data_buffer1     - the DMA data_buffer1 address
 * @param[in]  size_byte        - the DMA buffer size 
 * @return     none
 */
void keyscan_get_sar_adc0_adc1_data_dma(ks_mode_e ks_mode, dma_chn_e chn0, dma_chn_e chn1, unsigned short *data_buffer0, unsigned short *data_buffer1, unsigned int size_byte)
{
    if (ks_mode == KEYSCAN_2XADC_8K_ONCE_MODE) {
        size_byte = size_byte / 2;
    }

    keyscan_sar_adc_dma_config(chn0, SAR_ADC_FIFO(ADC0), data_buffer0, &adc_rx_dma_config, size_byte);
    keyscan_sar_adc_dma_config(chn1, SAR_ADC_FIFO(ADC1), data_buffer1, &adc1_rx_dma_config, size_byte);
}

/**
 * @brief      This function serves to get sar adc0 adc1 data with DMA LLP.
 * @param[in]  ks_mode          - the Keyscan mode
 * @param[in]  chn0             - the DMA channel
 * @param[in]  chn1             - the DMA channel
 * @param[in]  data_buffer0     - the DMA data_buffer0 address
 * @param[in]  data_buffer1     - the DMA data_buffer1 address
 * @param[in]  size_byte        - the DMA buffer size 
 * @return     none
 */
void keyscan_get_sar_adc0_adc1_data_dma_llp(ks_mode_e ks_mode, dma_chn_e chn0, dma_chn_e chn1, unsigned short *data_buffer0, unsigned short *data_buffer1, unsigned int size_byte)
{
    if (ks_mode == KEYSCAN_2XADC_8K_ONCE_MODE) {
        size_byte = size_byte / 2;
    }

    keyscan_sar_adc_dma_config_llp(chn0, SAR_ADC_FIFO(ADC0), data_buffer0, &adc_rx_dma_config, &adc0_rx_dma_list_config, size_byte);
    keyscan_rx_dma_add_list_element(chn0, SAR_ADC_FIFO(ADC0), data_buffer0, &adc0_rx_dma_list_config, &adc0_rx_dma_list_config, size_byte);
    dma_chn_en(chn0);

    keyscan_sar_adc_dma_config_llp(chn1, SAR_ADC_FIFO(ADC1), data_buffer1, &adc1_rx_dma_config, &adc1_rx_dma_list_config, size_byte);
    keyscan_rx_dma_add_list_element(chn1, SAR_ADC_FIFO(ADC1), data_buffer1, &adc1_rx_dma_list_config, &adc1_rx_dma_list_config, size_byte);
    dma_chn_en(chn1);
}

/**
 * @brief      This function serves to init sar_adc for keyscan.
 * @param[in]  ks_mode          - the Keyscan mode
 * @return     none
 */
static void keyscan_sar_adc_init(ks_mode_e ks_mode)
{
    if (ks_mode == KEYSCAN_1XADC_MODE || ks_mode == KEYSCAN_1XADC_8K_ONCE_MODE) {//sar_adc0
        adc_init(ADC0, DMA_M_CHN);
        adc_keyscan_sample_init(ADC0);
        
        adc_power_on(ADC0);
    } else {//sar_adc0 and sar_adc1
        adc_init(ADC0, DMA_M_CHN);
        adc_init(ADC1, DMA_M_CHN);

        adc_keyscan_sample_init(ADC0);
        adc_keyscan_sample_init(ADC1);

        adc_power_on(ADC0);
        adc_power_on(ADC1);
    }
}

/**
 * @brief      This function serves to configure keyscan registers for 8K data rate.
 * @param[in]  ks_config - the keyscan register configuration
 * @return     none
 */
static void keyscan_8k_config(ks_config_t ks_config)
{
    reg_ks_ctrl0 = ks_config.adc_simple;        //adc simple

    reg_ks_a_setl0 = ks_config.setl_time;       //Settle time the scan period
    reg_ks_a_setl1 = ks_config.setl_capture;    //Settle time when Capture start for ADC0
    reg_ks_a_setl2 = ks_config.setl_capture1;   //Settle time when Capture start for ADC1
    reg_ks_a_setl3 = ks_config.setl_time_pwr;   //Settle time the power manager
    reg_ks_a_setl4 = ks_config.setl_time_scan;  //Settle time the start time of scan after the Power
    reg_ks_a_setl5 = ks_config.setl_time_scan2; //Settle time the second start time of scan after the Power

    reg_ks_pwr_on_setl0 = ks_config.pwr_on0;    //Power on gpio 0 - 7
    reg_ks_pwr_on_setl1 = ks_config.pwr_on1;
    reg_ks_pwr_on_setl2 = ks_config.pwr_on2;
    reg_ks_pwr_on_setl3 = ks_config.pwr_on3;
    reg_ks_pwr_on_setl4 = ks_config.pwr_on4;
    reg_ks_pwr_on_setl5 = ks_config.pwr_on5;
    reg_ks_pwr_on_setl6 = ks_config.pwr_on6;
    reg_ks_pwr_on_setl7 = ks_config.pwr_on7;

    reg_ks_pwr_dn_setl0 = ks_config.pwr_dn0;    //Power down gpio 0 - 7
    reg_ks_pwr_dn_setl1 = ks_config.pwr_dn1;
    reg_ks_pwr_dn_setl2 = ks_config.pwr_dn2;
    reg_ks_pwr_dn_setl3 = ks_config.pwr_dn3;
    reg_ks_pwr_dn_setl4 = ks_config.pwr_dn4;
    reg_ks_pwr_dn_setl5 = ks_config.pwr_dn5;
    reg_ks_pwr_dn_setl6 = ks_config.pwr_dn6;
    reg_ks_pwr_dn_setl7 = ks_config.pwr_dn7;
}

/**
 * @brief      This function serves to init keyscan work mode.
 * @param[in]  ks_mode          - the Keyscan mode
 * @param[in]  ks_clock_mode    - the Keyscan clock mode
 * @return     none
 */
static void keyscan_mode_init(ks_mode_e ks_mode, ks_clock_mode_e ks_clock_mode)
{
    reg_clkks_mode = ks_clock_mode;

    if (ks_mode == KEYSCAN_1XADC_MODE || ks_mode == KEYSCAN_2XADC_MODE) {
        reg_ks_a_setl0 = 0x7f;    //settle time the scan period
    } else {
        keyscan_8k_config(keyscan_config[ks_mode]);
    }

    if (ks_mode == KEYSCAN_1XADC_MODE || ks_mode == KEYSCAN_1XADC_8K_ONCE_MODE) {
        reg_ks_a_en0 = FLD_KS_CAPTURE_IO | FLD_KS_A_IE0;                //release IO after capture stop, keyscan interrupt enable for ADC0
        reg_ks_a_en1 = FLD_KS_ADC_EN | FLD_KS_MOD_A | FLD_KS_CLK_A_EN;
    } else if (ks_mode == KEYSCAN_2XADC_MODE || ks_mode >= KEYSCAN_2XADC_8K_ONCE_MODE) {
        reg_ks_a_en0 = FLD_KS_CAPTURE_IO | FLD_KS_A_IE0 | FLD_KS_A_IE1; //release IO after capture stop, keyscan interrupt enable for ADC0 and ADC1
        reg_ks_a_en1 = FLD_KS_ADC_EN | FLD_KS_ADC1_EN | FLD_KS_MOD_A | FLD_KS_CLK_A_EN;
    }

    if (ks_mode == KEYSCAN_2XADC_8K_TWICE_M1_MODE) {
        reg_ks_a_en2 = FLD_KS_SCAN2_EN | FLD_KS_IRQ_DLY_EN;
    } else if (ks_mode == KEYSCAN_2XADC_8K_TWICE_M2_MODE) {
        reg_ks_a_en2 = FLD_KS_SCAN2_EN_IO | FLD_KS_SETL_TIME_SCAN2_EN | FLD_KS_IRQ_DLY_EN;
    }

    reg_ks_a_en1 |= FLD_KS_A_EN;//keyscan enable
}

/**
 * @brief      This function serves to init keyscan pin.
 * @param[in]  pin_group    - the Keyscan pin group
 * @return     none
 */
static void keyscan_pin_init(ks_channel_power_pin_group_e pin_group) 
{
    //set keyscan pins
    for (int i = 0; i < KEYSCAN_PIN_NUMBER; i++) {
        gpio_function_dis(keyscan_pin_groups[pin_group][i]);
        gpio_set_mux_function(keyscan_pin_groups[pin_group][i], KEYS0_IO);
        /*
        * In A0, only power pin pulldown 100K by internal resister, channel pin need pulldown 10k by external resister.
        * A1 version will fix this issue so that all pin can pulldown by internal resister.(confirmed by xuqiang at 20250423)
        */
        if (i < KEYSCAN_PIN_POWER_NUMBER) {
            gpio_set_up_down_res(keyscan_pin_groups[pin_group][i], GPIO_PIN_PULLDOWN_100K);
        }
        gpio_set_low_level(keyscan_pin_groups[pin_group][i]);
    }

    gpio_function_en(KS_ANALOG_SWITCH_EN);
    gpio_output_en(KS_ANALOG_SWITCH_EN);
    gpio_set_low_level(KS_ANALOG_SWITCH_EN);//low level enable

    reg_ks_a_en1 |= (pin_group << 6);//iomux[7:6]
}

/**
 * @brief      This function serves to init keyscan analog module.
 * @param[in]  ks_mode          - the keyscan mode
 * @param[in]  ks_clock_mode    - the keyscan clock mode
 * @param[in]  ks_pin_group     - the keyscan pin group
 * @return     none
 */
void keyscan_ana_init(ks_mode_e ks_mode, ks_clock_mode_e ks_clock_mode, ks_channel_power_pin_group_e ks_pin_group)
{
    reg_rst5 |= FLD_RST5_KEY_SCAN;
    reg_clk_en5 |= FLD_CLK5_KEYSCAN_EN;

    keyscan_pin_init(ks_pin_group);//init pin

    keyscan_sar_adc_init(ks_mode);//init sar_adc

    keyscan_mode_init(ks_mode, ks_clock_mode);//init mode
}

/**********************************************************************************************************************
 *                                          local function implementation                                             *
 *********************************************************************************************************************/

/**
 * @brief      This function serves to get the keynumber from meta data.
 * @param[in]  ks_mode          - the keyscan mode
 * @param[in]  data             - the meta data
 * @param[in]  index            - the meta data index
 * @return     the keynumber    - the index of the pressed key ADC value in the data buffer
 * @note       This function used to do data validation, only support 2 * 16 keys, each adc corresponding to 16 keys, 
 *             so the data format (the data length occupied by each key and the data order of keys) is different with the practical application.
 *             In practical application, user should analysis the meta data according to the keyboard design mode 
 *             and make validation by the data buffer read out from SRAM.
 */
int keyscan_get_keynumber_from_meta_data(ks_mode_e ks_mode, short *data, unsigned char index)
{
    unsigned char size = 0, len = 0;

    if (ks_mode == KEYSCAN_1XADC_MODE || ks_mode == KEYSCAN_1XADC_8K_ONCE_MODE) {
        size = KS_ADC0_SCAN_ONCE_DATA_SIZE;
        len = KS_ADC0_SCAN_ONCE_DATA_LEN;
    } else if (ks_mode == KEYSCAN_2XADC_MODE || ks_mode == KEYSCAN_2XADC_8K_ONCE_MODE) {
        size = KS_ADC0_1_SCAN_ONCE_DATA_SIZE;
        len = KS_ADC0_1_SCAN_ONCE_DATA_LEN;
    } else {
        size = KS_ADC0_1_SCAN_TWICE_DATA_SIZE;
        len = KS_ADC0_1_SCAN_TWICE_DATA_LEN;
    }

    if (ks_mode == KEYSCAN_2XADC_8K_TWICE_M1_MODE || ks_mode == KEYSCAN_2XADC_8K_TWICE_M2_MODE) {
        if ((data[index] > KEY_VALUE_MAX_THRESHOLD) || (data[index + 4] > KEY_VALUE_MAX_THRESHOLD)) {
            return -1;
        }
    } else {
        if ((data[index] > KEY_VALUE_MAX_THRESHOLD)) {
            return -1;
        }
    }


    return index % size / len;
}
