/********************************************************************************************************
 * @file    app.c
 *
 * @brief   This is the source file for 2.4G SDK
 *
 * @author  2.4G GROUP
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
#include "tl_common.h"
#include "drivers.h"
#include "app_config.h"
#include "app.h"
#include "mac.h"
#include "ota.h"

#define GPIO_KEY1 GPIO_PB3
#define GPIO_KEY2 GPIO_PB5
#define GPIO_KEY3 GPIO_PB6
#define GPIO_KEY4 GPIO_PB7

//for boot flag
unsigned char Boot_Flag_Buff[OTA_BOOT_FlAG_LEN] = {0};

extern volatile unsigned char OTA_SlaveTrig;
extern volatile unsigned char OTA_MasterTrig;


volatile unsigned char tx_done_cnt, rx_done_cnt, first_timeout_done, rx_timeout_done;

_attribute_ram_code_sec_noinline_ __attribute__((optimize("-Os"))) void rf_irq_handler(void)
{
    if (rf_get_irq_status(FLD_RF_IRQ_TX)) {
        rf_clr_irq_status(FLD_RF_IRQ_TX);
        tx_done_cnt++;
        MAC_TxIrqHandler();
    } else if (rf_get_irq_status(FLD_RF_IRQ_RX)) {
        rf_clr_irq_status(FLD_RF_IRQ_RX);
        rx_done_cnt++;
#if (OTA_MODE == OTA_BATCH_MASTER || OTA_MODE == OTA_BATCH_SLAVE)
        MAC_RxIrqHandler_Batch();
#else
        MAC_RxIrqHandler();
#endif
    } else if (rf_get_irq_status(FLD_RF_IRQ_RX_TIMEOUT)) {
        rf_clr_irq_status(FLD_RF_IRQ_RX_TIMEOUT);
        rx_timeout_done = 1;
        MAC_RxTimeOutHandler();
    } else if (rf_get_irq_status(FLD_RF_IRQ_FIRST_TIMEOUT)) {
        rf_clr_irq_status(FLD_RF_IRQ_FIRST_TIMEOUT);
        first_timeout_done = 1;
        MAC_RxFirstTimeOutHandler();
    } else {
        rf_clr_irq_status(FLD_RF_IRQ_ALL);
    }
}
PLIC_ISR_REGISTER(rf_irq_handler, IRQ_ZB_RT)

_attribute_ram_code_sec_ void gpio_irq7_handler(void)
{
    unsigned char gpio_tick = 1 | clock_time();
    if (0 == gpio_read(GPIO_KEY1)) { //press sw3 sw4
        sleep_us(10);
        if (0 == gpio_read(GPIO_KEY1)) {
            while (0 == gpio_read(GPIO_KEY1) && clock_time_exceed(gpio_tick, 200 * 1000))
                ;
#if (OTA_MODE == OTA_MASTER || OTA_MODE == OTA_BATCH_MASTER)
            OTA_MasterTrig = 1;
#else
            OTA_SlaveTrig = 1;
#endif
        }
    }

    if (0 == gpio_read(GPIO_KEY2)) { //press sw5 sw6
        sleep_us(10);
        if (0 == gpio_read(GPIO_KEY2)) {
            while (0 == gpio_read(GPIO_KEY2) && clock_time_exceed(gpio_tick, 200 * 1000))
                ;
            OTA_MasterTrig = 2;
        }
    }

    gpio_clr_irq_status(GPIO_IRQ_IRQ7);
}

PLIC_ISR_REGISTER(gpio_irq7_handler, IRQ_GPIO_IRQ7);

void gpio_key_init(void)
{
    /*
   * Button matrix table:
   *          KEY3    KEY4
   *  KEY1    SW3     SW4
   *  KEY2    SW5     SW6
   */
    /* gpio_key_function_init */
    gpio_function_en(GPIO_KEY1);
    gpio_function_en(GPIO_KEY2);
    gpio_function_en(GPIO_KEY3);
    gpio_function_en(GPIO_KEY4);

    /* gpio_key_status_init */
    gpio_output_en(GPIO_KEY3);
    gpio_output_en(GPIO_KEY4);
    gpio_output_dis(GPIO_KEY1);
    gpio_output_dis(GPIO_KEY2);
    gpio_input_en(GPIO_KEY1);
    gpio_input_en(GPIO_KEY2);
    gpio_set_up_down_res(GPIO_KEY1, GPIO_PIN_PULLUP_10K);
    gpio_set_irq(GPIO_IRQ7, GPIO_KEY1, INTR_FALLING_EDGE);
    gpio_set_up_down_res(GPIO_KEY2, GPIO_PIN_PULLUP_10K);
    gpio_set_irq(GPIO_IRQ7, GPIO_KEY2, INTR_FALLING_EDGE);
    gpio_set_irq_mask(GPIO_IRQ_IRQ7);
    plic_interrupt_enable(IRQ_GPIO_IRQ7);
    gpio_set_low_level(GPIO_KEY3);
    gpio_set_low_level(GPIO_KEY4);
}

///////////////////////////////////////////

/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */
_attribute_no_inline_ void user_init_normal(void)
{
#if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_init();
#endif

    gpio_key_init();

#if (OTA_MODE == OTA_BATCH_SLAVE)
    MAC_Init(OTA_CHANNEL, 0, 0, 0, 0, OTA_ACCESS_CODE);
    sleep_ms(1000);
    MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
#endif
}

/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void)
{
}

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////

/**
 * @brief      main loop
 * @param[in]  none.
 * @return     none.
 */
int main_idle_loop(void)
{
    ////////////////////////////////////// Debug entry /////////////////////////////////
#if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_handler();
#endif

#if (OTA_MODE == OTA_MASTER || OTA_MODE == OTA_BATCH_MASTER)
    if (OTA_MasterTrig) {
        unsigned int ota_bin_addr = 0;
        if (OTA_MasterTrig == 1) {
            ota_bin_addr = OTA_MASTER_BIN_ADDR_1;

        } else if (OTA_MasterTrig == 2) {
            ota_bin_addr = OTA_MASTER_BIN_ADDR_2;
        }
        OTA_MasterTrig = 0;
        tlk_printf("ota bin:%4x\r\n", ota_bin_addr);
#else
    if (OTA_SlaveTrig) {
        OTA_SlaveTrig = 0;
#endif

        flash_unlock();

#if (BATT_CHECK_ENABLE)
        if (!app_battery_power_check(2000)) {
            while (1) {
                gpio_toggle(GPIO_LED_RED);
                sleep_ms(50);
            }
        }
#endif

        tlk_printf("OTA start!\r\n");
        MAC_Init(OTA_CHANNEL,
                 OTA_RxIrq,
                 OTA_TxIrq,
                 OTA_RxTimeoutIrq,
                 OTA_RxTimeoutIrq,
                 OTA_ACCESS_CODE);

#if (OTA_MODE == OTA_MASTER || OTA_MODE == OTA_BATCH_MASTER)
        OTA_MasterInit(ota_bin_addr);
#else
        flash_read_page(OTA_SLAVE_BIN_ADDR_1 + OTA_BOOT_FlAG_OFFSET, OTA_BOOT_FlAG_LEN, (unsigned char *)Boot_Flag_Buff);
        if (Boot_Flag_Buff[0] == 0x4b) {
            OTA_SlaveInit(OTA_SLAVE_BIN_ADDR_2);
        } else {
            OTA_SlaveInit(OTA_SLAVE_BIN_ADDR_1);
        }
#endif


        gpio_write(GPIO_LED_BLUE, 1);
        sleep_ms(80);
        gpio_write(GPIO_LED_BLUE, 0);
        sleep_ms(80);
        gpio_write(GPIO_LED_BLUE, 1);
        sleep_ms(80);
        gpio_write(GPIO_LED_BLUE, 0);

        while (1) {
#if (TLKAPI_DEBUG_ENABLE)
            tlkapi_debug_handler();
#endif

#if (OTA_MODE == OTA_MASTER)
            if (!OTA_MasterStart()) {
                OTA_MasterTrig = 0;
                break;
            }
#elif (OTA_MODE == OTA_SLAVE)
            if (!OTA_SlaveStart()) {
                OTA_SlaveTrig = 0;
                break;
            }
#elif (OTA_MODE == OTA_BATCH_MASTER)
        if (!OTA_Batch_MasterStart()) {
            OTA_MasterTrig = 0;
            break;
        }
#else
        if (!OTA_Batch_SlaveStart()) {
            OTA_SlaveTrig      = 0;
            first_timeout_done = 1;
            break;
        }
#endif
        }
    }


#if (OTA_MODE == OTA_MASTER || OTA_MODE == OTA_BATCH_MASTER)
    gpio_toggle(GPIO_LED_GREEN);
    tlk_printf("ota master\r\n");
#elif (OTA_MODE == OTA_BATCH_SLAVE)
    if (first_timeout_done) {
        first_timeout_done = 0;
        MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
    }
    gpio_toggle(GPIO_LED_GREEN);
    tlk_printf("ota slave\r\n");
#else
gpio_toggle(GPIO_LED_GREEN);
tlk_printf("ota slave\r\n");
#endif
    sleep_ms(1000);

    return 0; //must return 0 due to SDP flow
}

_attribute_no_inline_ void main_loop(void)
{
    main_idle_loop();
}
