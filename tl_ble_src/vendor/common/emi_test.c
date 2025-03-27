/********************************************************************************************************
 * @file    emi_test.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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


#include "tl_common.h"
#include "drivers.h"
#include "emi_test.h"



#if (APP_EMI_TEST_ENABLE)






/**
 * @brief   Enumeration type of PA operation.
 */
typedef enum{
    PA_SETTING_OPT_OUTPUT = 0xa5,
}pa_setting_OPT_e;

/**
 * @brief   Enumeration type of PA pin level.
 */
typedef enum{
    PA_SETTING_PIN_LEVEL_LOW = 0x00,
    PA_SETTING_PIN_LEVEL_HIGH = 0x01,
}pa_setting_pin_level_e;

/**
 * @brief   Structure used to store an operation instruction.
 */
typedef union{
    unsigned int cmd;
    struct{
        unsigned char state;
        unsigned char opt;
        unsigned char para0;
        unsigned char para1;
    }data;
}pa_setting_cmd_t, *pa_setting_cmd_t_ptr;

/**
 * @brief   Structure used to store PA Setting information.
 */
typedef struct{
    unsigned int addr;
    unsigned int bit1_cnt;
    unsigned int cmd_cnt;
}pa_setting_data_t, *pa_setting_data_t_ptr;

/**
 * @brief   Structure used to store PA operation information.
 */
typedef struct{
    pa_setting_data_t pa_setting_data;
    unsigned char pa_en;
    unsigned char bypass_en;
}pa_operation_t, *pa_operation_t_ptr;

/**
 * @brief   Local variable used to hold data related to the PA operation.
 */
static pa_operation_t s_pa_operation = {{0, 0, 0}, 0, 0};

/**
 * @brief       This function serves to read data saved in binary file.
 * @param[in]   addr   - Position in binary file.
 * @return      One-byte result data.
 */
unsigned char read_data8(unsigned int addr)
{
    unsigned char ret = 0;
    flash_read_page(addr, 1, &ret);
    return ret;
}

/**
 * @brief       This function serves to read data saved in binary file.
 * @param[in]   addr   - Position in binary file.
 * @return      Two-byte result data.
 */
unsigned short read_data16(unsigned int addr)
{
    unsigned short ret = 0;
    flash_read_page(addr, 2, (unsigned char*)(&ret));
    return ret;
}

/**
 * @brief       This function serves to read data saved in binary file.
 * @param[in]   addr   - Position in binary file.
 * @return      Four-byte result data.
 */
unsigned int read_data32(unsigned int addr)
{
    unsigned int ret = 0;
    flash_read_page(addr, 4, (unsigned char*)(&ret));
    return ret;
}

/**
 * @brief       This function serves to initialize PA setting.
 * @param[in]   addr       - Address to save PA related operations.
 * @param[in]   bypass_en  - To set PA bypass mode, 0:disable 1:enable.
 * @return      none
 */
void pa_setting_init(unsigned int addr, unsigned char bypass_en)
{
    s_pa_operation.pa_setting_data.addr = read_data32(addr);

    s_pa_operation.bypass_en = bypass_en;

    //check address
    if(s_pa_operation.pa_setting_data.addr != addr)
    {
        s_pa_operation.pa_en = 0;
        return;
    }

    s_pa_operation.pa_setting_data.bit1_cnt = read_data32(addr+4);
    s_pa_operation.pa_setting_data.cmd_cnt = read_data32(addr+8);

    //check bit1 count
    unsigned int data_check;
    unsigned int bit1_cnt = 0;
    unsigned int data_len = 1 + s_pa_operation.pa_setting_data.cmd_cnt;
    for(unsigned int i = 0; i < data_len; i++)
    {
        data_check = read_data32(s_pa_operation.pa_setting_data.addr + 8 + 4*i);
        for(unsigned char j = 0; j < 32; j++)
        {
            bit1_cnt += (data_check>>j) & 1;
        }
    }

    if(bit1_cnt != s_pa_operation.pa_setting_data.bit1_cnt)
    {
        s_pa_operation.pa_en = 0;
        return;
    }

    s_pa_operation.pa_en = 1;
}

/**
 * @brief       This function serves to execute PA operations according to the cmd.
 * @param[in]   cmd      - PA operation command.
 * @return      none
 */
static inline void cmd_operation(pa_setting_cmd_t cmd)
{
    unsigned short pin = get_pin(cmd.data.para0);
    switch(cmd.data.opt)
    {
    case PA_SETTING_OPT_OUTPUT:
        gpio_function_en(pin);
        gpio_input_dis(pin);
        gpio_output_en(pin);
        if(cmd.data.para1)
        {
            gpio_set_high_level(pin);
        }
        else
        {
            gpio_set_low_level(pin);
        }
        break;
    default:
        break;
    }
}


/**
 * @note The pin status of the BY PASS mode varies according to the PA chip.
 *       You need to change the pin status according to the actual situation
 */
#define PA_BYPASS   0
/**
 * @brief       This function serves to execute PA operations according to the state.
 * @param[in]   state      - PA working state.
 * @return      none
 */
void pa_operation(pa_setting_state_e state)
{

#if(APP_EMI_TEST_ENABLE)
    if(!s_pa_operation.pa_en) return;
    if(s_pa_operation.bypass_en) state = PA_SETTING_STATE_BYPASS;

    pa_setting_cmd_t cmd;
    for(unsigned int i = 0; i < s_pa_operation.pa_setting_data.cmd_cnt; i++)
    {
        cmd.cmd = read_data32(s_pa_operation.pa_setting_data.addr + 12 + 4*i);

        if(state == cmd.data.state)
        {
            cmd_operation(cmd);
        }
    }

#else


    switch(state)
    {
    case PA_SETTING_STATE_INIT:
            //to do:To use the PA function, enable related PA control pins
        break;
#if(!PA_BYPASS)
    case PA_SETTING_STATE_TX:
            //to do:To use the PA function, enable related PA control pins
        break;
    case PA_SETTING_STATE_RX:
            //to do:To use the PA function, enable related PA control pins
        break;
#else

    case PA_SETTING_STATE_TX:
    case PA_SETTING_STATE_RX:
    case PA_SETTING_STATE_BYPASS:
            //to do:To use the PA function, enable related PA control pins
        break;

#endif
    default:
        break;
    }


#endif

}


















/*
 * @brief   this macro definition serve to open the setting to deal with problem of zigbee mode 2480Mhz
 *          band edge can't pass the spec.only use it at the time of certification.
 * */
#define FIX_ZIGBEE_BANDEDGE_EN  0

#if !APP_EMI_TEST_ENABLE

/*
 * @brief   this macro definition serve to close internal cap.
 * */
#define CLOSE_INTERNAL_CAP_EN   0//0:Internal capacitance,1:External capacitance

/*
 * @brief   this macro definition serve to enable function of swire-through-usb.
 * */
#define SWIRE_THROUGH_USB_EN    0//0:disable swire-through-usb,1:enable swire-through-usb.

#endif

#if MCU_CORE_TYPE == MCU_CORE_TL751X
#define SRAM_BASE_ADDR 0x00020000
#elif MCU_CORE_TYPE == MCU_CORE_TL721X
#define SRAM_BASE_ADDR 0x00040000
#elif MCU_CORE_TYPE == MCU_CORE_TL321X
#define SRAM_BASE_ADDR 0x00068000
#else
#define SRAM_BASE_ADDR 0xc0000000
#endif

#define TX_PACKET_MODE_ADDR              (SRAM_BASE_ADDR+0x05)
#define RUN_STATUE_ADDR                  (SRAM_BASE_ADDR+0x06)
#define TEST_COMMAND_ADDR                 (SRAM_BASE_ADDR+0x07)
#define POWER_ADDR                          (SRAM_BASE_ADDR+0x08)
#define CHANNEL_ADDR                     (SRAM_BASE_ADDR+0x09)
#define RF_MODE_ADDR                     (SRAM_BASE_ADDR+0x0a)
#define CD_MODE_HOPPING_CHN                 (SRAM_BASE_ADDR+0x0b)
#define RSSI_ADDR                        (SRAM_BASE_ADDR+0x04)
#define RX_PACKET_NUM_ADDR               (SRAM_BASE_ADDR+0x0c)

#define MAX_RF_CHANNEL                   40

/**
 * @brief   The emi test command and function
 */
typedef struct {
    unsigned char  cmd_id;                                      /**< emi command id */
    void     (*func)(rf_mode_e, unsigned char , signed char);   /**< emi function */
}test_list_t;

/**
 * @brief   The emi test mode.
 */
typedef enum{
    ble2m = 0,
    ble1m = 1,
    zigbee250k = 2,
    ble125K = 3,
    ble500K = 4,
    pri2m = 5,
    pri1m = 6,
}emi_rf_mode_e;

/**
 * @brief   Init the emi frequency hopping channel
 */
const unsigned char rf_chn_hipping[MAX_RF_CHANNEL] = {
    48,  4, 66, 62, 44,
    12, 38, 16 ,26, 20,
    22, 68, 18, 28, 42,
    32, 34, 36, 14, 40,
    30, 54, 46, 2,  50,
    52, 80, 56, 78, 74,
    8,  64, 6,  24, 70,
    72, 60, 76, 58, 10,
};

/**
 * @brief   Init the emi reference parameter
 */
unsigned char  g_power_level = 0;  // 0
unsigned char  g_chn = 2;          // 2
unsigned char  g_cmd_now = 1;      // 1
unsigned char  g_run = 1;          // 1
unsigned char  g_hop = 0;          // 0
unsigned char  g_tx_cnt = 0;       // 0
#if(MCU_CORE_TYPE == MCU_CORE_B92)
unsigned char  g_pkt_tone = 0;      //0
#endif
emi_rf_mode_e  g_mode = ble1m;     // 1



/**
 * @brief Structure to save general setting.
 */
typedef struct{
    unsigned char ptr[0];
    unsigned char calib_pos:2;
    unsigned char pa_bypass_en:1;
    unsigned char :1;
    unsigned char swire_through_usb_en:1;
    unsigned char power_mode:2;
    unsigned char cap:1;
}general_setting_t, *general_setting_t_ptr;

/**
 * @brief Structure to save emi configuration.
 */
typedef struct{
    unsigned int pa_setting_pos;
    general_setting_t general_setting;
}emi_setting_t, *emi_setting_t_ptr;



/**
* @brief global variable to save EMI initial setting
*/
volatile emi_setting_t g_emi_setting;


void emicarrieronly(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emi_con_prbs9(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emirx(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emitxprbs9(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emitx55(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emitx0f(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emi_con_tx55(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emi_con_tx0f(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emitxaa(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emitxf0(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
#if(MCU_CORE_TYPE == MCU_CORE_B92)
void rf_emi_tx_current_test(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void rf_emi_rx_current_test(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
#endif
/**
 * @brief   Init the structure of the emi test command and function
 */
test_list_t  ate_list[] = {
        {0x01,emicarrieronly},
        {0x02,emi_con_prbs9},
        {0x03,emirx},
        {0x04,emitxprbs9},
        {0x05,emitx55},
        {0x06,emitx0f},
        {0x07,emi_con_tx55},
        {0x08,emi_con_tx0f},
        {0x0d,emitxaa},
        {0x0f,emitxf0},
#if(MCU_CORE_TYPE == MCU_CORE_B92)
        {0x10,rf_emi_tx_current_test},
        {0x11,rf_emi_rx_current_test}
#endif
};

#if(MCU_CORE_TYPE == MCU_CORE_B92)
/**
 * @brief      This function serves to test RF tx current
 * @param[in]  rf_mode     - mode of RF.
 * @param[in]  pwr         - power level of RF.
 * @param[in]  rf_chn      - channel of RF.
 * @return     none
 */
void rf_emi_tx_current_test(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
    (void)(rf_mode);
    extern void rf_set_power_level_index_singletone (rf_power_level_e level);
    extern unsigned char g_single_tong_freqoffset;
    rf_mode_init();
    rf_power_level_e  power = rf_power_Level_list[pwr];
    g_single_tong_freqoffset = 1;
    rf_set_chn(rf_chn);
    g_single_tong_freqoffset = 0;
    rf_set_power_level_index_singletone(power);
    rf_set_txmode();
    gpio_shutdown(GPIO_ALL);
    rf_current_test_cfg();
    while(1);
}

/**
 * @brief      This function serves to test RF rx current
 * @param[in]  rf_mode     - mode of RF.
 * @param[in]  pwr         - power level of RF.
 * @param[in]  rf_chn      - channel of RF.
 * @return     none
 */
void rf_emi_rx_current_test(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
    (void)rf_mode;
    (void)pwr;
    rf_emi_rx_setup(RF_MODE_BLE_1M_NO_PN, rf_chn);
    gpio_shutdown(GPIO_ALL);
    rf_current_test_cfg();
    while(1);
}
#endif










/**
 * @brief       This function serves to EMI Init
 * @return      none
 */

void emi_init(void)
{

    if(g_emi_setting.general_setting.cap)
    {
        rf_turn_off_internal_cap();
    }


    rf_access_code_comm(EMI_ACCESS_CODE);             // access code
#if(MCU_CORE_TYPE == MCU_CORE_B91 || MCU_CORE_TYPE == MCU_CORE_TL721X || MCU_CORE_TYPE == MCU_CORE_TL321X)
    write_sram8(TX_PACKET_MODE_ADDR,g_tx_cnt);       // tx_cnt,pkt+tone
#elif(MCU_CORE_TYPE == MCU_CORE_B92)
    write_sram8(TX_PACKET_MODE_ADDR,g_tx_cnt|(g_pkt_tone<<7)); // tx_cnt,pkt+tone
#endif
    write_sram8(RUN_STATUE_ADDR,g_run);               // run
    write_sram8(TEST_COMMAND_ADDR,g_cmd_now);         // cmd
    write_sram8(POWER_ADDR,g_power_level);            // power
    write_sram8(CHANNEL_ADDR,g_chn);                  // chn
    write_sram8(RF_MODE_ADDR,g_mode);                 // mode
    write_sram8(CD_MODE_HOPPING_CHN,g_hop);           // hop
    write_sram8(RSSI_ADDR,0);                         // rssi
    write_sram32(RX_PACKET_NUM_ADDR,0);               // rx_packet_num

    /*diasble fast settle*/
    rf_tx_fast_settle_dis();
    rf_rx_fast_settle_dis();
}

/**
 * @brief       This function serves to EMI CarryOnly
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return      none
 */

void emicarrieronly(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{

    pa_operation(PA_SETTING_STATE_TX);
    (void)(rf_mode);
#if(MCU_CORE_TYPE == MCU_CORE_B92 || MCU_CORE_TYPE == MCU_CORE_TL721X)
    unsigned int t0 = reg_system_tick,chnidx = 1;
#endif
    rf_power_level_e  power = rf_power_Level_list[pwr];
    g_hop = read_sram8(CD_MODE_HOPPING_CHN);
    rf_emi_tx_single_tone(power,rf_chn);
    while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
            && ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
            && ((read_sram8(RF_MODE_ADDR)) == g_mode))
    {
#if(MCU_CORE_TYPE == MCU_CORE_B92 || MCU_CORE_TYPE == MCU_CORE_TL721X)
        if(g_hop)
        {
            trng_init();
            while(!clock_time_exceed(t0,10000))    // run 10ms
                rf_emi_tx_single_tone(power,chnidx);

            while(!clock_time_exceed(t0,20000));   // stop 20ms
            t0 = reg_system_tick;
            rf_emi_stop();
            chnidx = (trng_rand()&0x7f);
            if(chnidx>80)
                chnidx -= 80;
        }
#endif
    }
    rf_emi_stop();
    rf_set_tx_rx_off();
#if(MCU_CORE_TYPE == MCU_CORE_TL721X || MCU_CORE_TYPE == MCU_CORE_TL321X)
    rf_dma_reset();
    rf_clr_dig_logic_state();
#else
    dma_reset();
    rf_emi_reset_baseband();
#endif
}

/**
 * @brief       This function serves to EMI ConPrbs9
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return      none
 */
void emi_con_prbs9(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
    pa_operation(PA_SETTING_STATE_TX);
    unsigned int t0 = reg_system_tick,chnidx = 1;
    rf_power_level_e power = rf_power_Level_list[pwr];
    g_hop = read_sram8(CD_MODE_HOPPING_CHN);
    rf_emi_tx_continue_update_data(rf_mode,power,rf_chn,0);
    while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
            && ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
            && ((read_sram8(RF_MODE_ADDR)) == g_mode))
    {
#if FIX_ZIGBEE_BANDEDGE_EN
        if(rf_mode == RF_MODE_ZIGBEE_250K)
        {
            if (rf_chn == 80)
            {
                write_reg8(0x140c23,0x00);
            }
            else
            {
                write_reg8(0x140c23,0x80);
            }
        }
#endif
        rf_continue_mode_run();
        if(g_hop)
        {
            while(!clock_time_exceed(t0,10000))    // run 10ms
                rf_continue_mode_run();

            while(!clock_time_exceed(t0,20000));   // stop 20ms
            t0 = reg_system_tick;
            rf_emi_tx_continue_update_data(rf_mode,power,rf_chn_hipping[chnidx - 1],0);
            (chnidx >= MAX_RF_CHANNEL)?(chnidx = 1):(chnidx++);
        }
    }
    rf_emi_stop();

    rf_set_tx_rx_off();
#if(MCU_CORE_TYPE == MCU_CORE_TL721X || MCU_CORE_TYPE == MCU_CORE_TL321X)
    rf_dma_reset();
    rf_clr_dig_logic_state();
#else
    dma_reset();
    rf_emi_reset_baseband();
#endif
}

/**
 * @brief       This function serves to EMI Rx
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return      none
 */
void emirx(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
    pa_operation(PA_SETTING_STATE_RX);
    (void)(pwr);
    rf_emi_rx_setup(rf_mode,rf_chn);
    write_sram8(RSSI_ADDR,0);
    write_sram32(RX_PACKET_NUM_ADDR,0);
    while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
            && ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
            && ((read_sram8(RF_MODE_ADDR)) == g_mode))
    {
        rf_emi_rx_loop();
        if(rf_emi_get_rxpkt_cnt() != read_sram32(RX_PACKET_NUM_ADDR))
        {
            write_sram8(RSSI_ADDR,rf_emi_get_rssi_avg());
            write_sram32(RX_PACKET_NUM_ADDR,rf_emi_get_rxpkt_cnt());
        }
    }
    rf_emi_stop();
}

/**
 * @brief       This function serves to EMI TxPrbs
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return      none
 */

void emitxprbs9(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
    pa_operation(PA_SETTING_STATE_TX);
    unsigned int tx_num = 0;
    rf_power_level_e  power = rf_power_Level_list[pwr];
    rf_emi_tx_burst_setup(rf_mode,power,rf_chn,1);
    while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
            && ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
            && ((read_sram8(RF_MODE_ADDR)) == g_mode))
    {
        rf_emi_tx_burst_loop(rf_mode,0);
        if(g_tx_cnt)
        {
            tx_num++;
            if(tx_num >= 1000)
                break;
        }
#if(MCU_CORE_TYPE == MCU_CORE_B92)
        if(g_pkt_tone)
        {
            rf_set_tx_modulation_index(RF_MI_P0p00);
            rf_set_power_level_singletone(power);
            delay_us(40);
            rf_set_power_off_singletone();
            rf_set_tx_modulation_index(RF_MI_P0p50);
        }
#endif
    }
    rf_emi_stop();
}

/**
 * @brief       This function serves to EMI Tx55
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return      none
 */

void emitx55(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
    pa_operation(PA_SETTING_STATE_TX);
    unsigned int tx_num = 0;
    rf_power_level_e  power = rf_power_Level_list[pwr];
    rf_emi_tx_burst_setup(rf_mode,power,rf_chn,2);
    while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
            && ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
            && ((read_sram8(RF_MODE_ADDR)) == g_mode))
    {
        rf_emi_tx_burst_loop(rf_mode,2);
        if(g_tx_cnt)
        {
            tx_num++;
            if(tx_num >= 1000)
                break;
        }
#if(MCU_CORE_TYPE == MCU_CORE_B92)
        if(g_pkt_tone)
        {
            rf_set_tx_modulation_index(RF_MI_P0p00);
            rf_set_power_level_singletone(power);
            delay_us(40);
            rf_set_power_off_singletone();
            rf_set_tx_modulation_index(RF_MI_P0p50);
        }
#endif
    }
    rf_emi_stop();
}

/**
 * @brief       This function serves to EMI Tx0f
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return      none
 */
void emitx0f(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
    pa_operation(PA_SETTING_STATE_TX);
    unsigned int tx_num = 0;
    rf_power_level_e  power = rf_power_Level_list[pwr];
    rf_emi_tx_burst_setup(rf_mode,power,rf_chn,1);
    while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
            && ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
            && ((read_sram8(RF_MODE_ADDR)) == g_mode))
    {
        rf_emi_tx_burst_loop(rf_mode,1);
        if(g_tx_cnt)
        {
            tx_num++;
            if(tx_num >= 1000)
                break;
        }
#if(MCU_CORE_TYPE == MCU_CORE_B92)
        if(g_pkt_tone)
        {
            rf_set_tx_modulation_index(RF_MI_P0p00);
            rf_set_power_level_singletone(power);
            delay_us(40);
            rf_set_power_off_singletone();
            rf_set_tx_modulation_index(RF_MI_P0p50);
        }
#endif
    }
    rf_emi_stop();
}
/**
 * @brief       This function serves to EMI Txaa
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return      none
 */
void emitxaa(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
    pa_operation(PA_SETTING_STATE_TX);
    unsigned int tx_num = 0;
    rf_power_level_e  power = rf_power_Level_list[pwr];
    rf_emi_tx_burst_setup(rf_mode,power,rf_chn,3);
    while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
            && ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
            && ((read_sram8(RF_MODE_ADDR)) == g_mode))
    {
        rf_emi_tx_burst_loop(rf_mode,3);
        if(g_tx_cnt)
        {
            tx_num++;
            if(tx_num >= 1000)
                break;
        }
#if(MCU_CORE_TYPE == MCU_CORE_B92)
        if(g_pkt_tone)
        {
            rf_set_tx_modulation_index(RF_MI_P0p00);
            rf_set_power_level_singletone(power);
            delay_us(40);
            rf_set_power_off_singletone();
            rf_set_tx_modulation_index(RF_MI_P0p50);
        }
#endif
    }
    rf_emi_stop();
}

/**
 * @brief       This function serves to EMI Txf0
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return      none
 */
void emitxf0(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
    pa_operation(PA_SETTING_STATE_TX);
    unsigned int tx_num = 0;
    rf_power_level_e  power = rf_power_Level_list[pwr];
    rf_emi_tx_burst_setup(rf_mode,power,rf_chn,4);
    while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
            && ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
            && ((read_sram8(RF_MODE_ADDR)) == g_mode))
    {
        rf_emi_tx_burst_loop(rf_mode,4);
        if(g_tx_cnt)
        {
            tx_num++;
            if(tx_num >= 1000)
                break;
        }
#if(MCU_CORE_TYPE == MCU_CORE_B92)
        if(g_pkt_tone)
        {
            rf_set_tx_modulation_index(RF_MI_P0p00);
            rf_set_power_level_singletone(power);
            delay_us(40);
            rf_set_power_off_singletone();
            rf_set_tx_modulation_index(RF_MI_P0p50);
        }
#endif
    }
    rf_emi_stop();
}



/**
 * @brief       This function serves to EMI_CON_TX55
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return      none
 */
void emi_con_tx55(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
    pa_operation(PA_SETTING_STATE_TX);
    unsigned int t0 = reg_system_tick,chnidx = 1;
    rf_power_level_e power = rf_power_Level_list[pwr];
    rf_emi_tx_continue_update_data(rf_mode,power,rf_chn,2);
    while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
            && ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
            && ((read_sram8(RF_MODE_ADDR)) == g_mode))
    {
        rf_continue_mode_run();
        if(g_hop)
        {
            while(!clock_time_exceed(t0,10000))    // run 10ms
                rf_continue_mode_run();

            while(!clock_time_exceed(t0,20000));   // stop 20ms
            t0 = reg_system_tick;
            rf_emi_tx_continue_update_data(rf_mode,power,rf_chn_hipping[chnidx - 1],2);
            (chnidx >= MAX_RF_CHANNEL)?(chnidx = 1):(chnidx++);
        }
    }
    rf_emi_stop();
#if(MCU_CORE_TYPE == MCU_CORE_TL721X || MCU_CORE_TYPE == MCU_CORE_TL321X)
    rf_dma_reset();
    rf_clr_dig_logic_state();
#else
    dma_reset();
    rf_emi_reset_baseband();
#endif
}

/**
 * @brief       This function serves to EMI_CON_TX0f
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return      none
 */
void emi_con_tx0f(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
    pa_operation(PA_SETTING_STATE_TX);
    unsigned int t0 = reg_system_tick,chnidx = 1;
    rf_power_level_e power = rf_power_Level_list[pwr];
    rf_emi_tx_continue_update_data(rf_mode,power,rf_chn,1);
    while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
            && ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
            && ((read_sram8(RF_MODE_ADDR)) == g_mode))
    {
        rf_continue_mode_run();
        if(g_hop)
        {
            while(!clock_time_exceed(t0,10000))    // run 10ms
                rf_continue_mode_run();

            while(!clock_time_exceed(t0,20000));   // stop 20ms
            t0 = reg_system_tick;
            rf_emi_tx_continue_update_data(rf_mode,power,rf_chn_hipping[chnidx - 1],1);
            (chnidx >= MAX_RF_CHANNEL)?(chnidx = 1):(chnidx++);
        }
    }
    rf_emi_stop();
#if(MCU_CORE_TYPE == MCU_CORE_TL721X || MCU_CORE_TYPE == MCU_CORE_TL321X)
    rf_dma_reset();
    rf_clr_dig_logic_state();
#else
    dma_reset();
    rf_emi_reset_baseband();
#endif
}

/**
 * @brief     This function serves to EMI ServiceLoop
 * @return    none
 */
void emi_serviceloop(void)
{
   unsigned char i = 0;

   while(1)
   {
       g_run = read_sram8(RUN_STATUE_ADDR);  // get the run state!
       if(g_run != 0)
       {
           g_power_level = read_sram8(POWER_ADDR);
           g_chn = read_sram8(CHANNEL_ADDR);
           g_mode = read_sram8(RF_MODE_ADDR);
           g_cmd_now = read_sram8(TEST_COMMAND_ADDR);  // get the command!
           g_tx_cnt = (read_sram8(TX_PACKET_MODE_ADDR)&0x7f);
           g_hop = read_sram8(CD_MODE_HOPPING_CHN);
#if(MCU_CORE_TYPE == MCU_CORE_B92)
           g_pkt_tone = (read_sram8(TX_PACKET_MODE_ADDR)&BIT(7));
#endif
           pa_operation(PA_SETTING_STATE_INIT);

           for (i = 0;i < sizeof(ate_list)/sizeof(test_list_t);i++)
           {
                if(g_cmd_now == ate_list[i].cmd_id)
                {
                    switch(g_mode)
                    {
                      case ble2m:
                          ate_list[i].func(RF_MODE_BLE_2M,g_power_level,g_chn);
                          break;

                      case ble1m:
                          ate_list[i].func(RF_MODE_BLE_1M_NO_PN,g_power_level,g_chn);
                          break;

                      case zigbee250k:
                          ate_list[i].func(RF_MODE_ZIGBEE_250K,g_power_level,g_chn);
                          break;

                      case ble125K:
                          ate_list[i].func(RF_MODE_LR_S8_125K,g_power_level,g_chn);
                          break;

                      case ble500K:
                          ate_list[i].func(RF_MODE_LR_S2_500K,g_power_level,g_chn);
                          break;

                      case pri2m:
                          ate_list[i].func(RF_MODE_PRIVATE_2M,g_power_level,g_chn);
                          break;

                      case pri1m:
                          ate_list[i].func(RF_MODE_PRIVATE_1M,g_power_level,g_chn);
                          break;

                      default:break;
                    }
                }
           }
           g_run = 0;
           write_sram8(RUN_STATUE_ADDR,g_run);
       }
   }
}



/**
 * @brief       This function serves to read EMI initial setting.
 * @return      none
 */

void emi_setting_init(void)
{
    g_emi_setting.general_setting.ptr[0] = read_data8(GENERAL_SETTING_ADDR);
    g_emi_setting.pa_setting_pos = read_data32(PA_SETTING_ADDR);
}




/**
 * @brief       This function serves to User Init emi
 * @return      none
 */

void app_user_init_emi(void)
{

    pa_setting_init(g_emi_setting.pa_setting_pos, g_emi_setting.general_setting.pa_bypass_en);
    if(g_emi_setting.general_setting.swire_through_usb_en)
    {
        usb_set_pin_en();
        gpio_set_up_down_res(GPIO_DM, GPIO_PIN_PULLUP_10K);
    }

    emi_init();
}

#endif //end of "APP_EMI_TEST_ENABLE"
