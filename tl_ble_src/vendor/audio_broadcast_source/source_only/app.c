/********************************************************************************************************
 * @file    app.c
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
#include "../source_config.h"

#if (SOURCE_VERSION == SOURCE_ONLY_VERSION)
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app.h"
#include "app_audio.h"
#include "app_config.h"



///////////////////////////////////////////
/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */
_attribute_no_inline_ void user_init_normal(void)
{

//////////////////////////// basic hardware Initialization  Begin //////////////////////////////////
    /* random number generator must be initiated here( in the beginning of user_init_normal).
     * When deepSleep retention wakeUp, no need initialize again */
    random_generator_init();

    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_init();
        blc_debug_enableStackLog(STK_LOG_NONE);
    #endif

    blc_readFlashSize_autoConfigCustomFlashSector();

    /* attention that this function must be called after "blc readFlashSize_autoConfigCustomFlashSector" !!!*/
    blc_app_loadCustomizedParameters_normal();
//////////////////////////// basic hardware Initialization  End /////////////////////////////////


    //////////// BLE stack Initialization  Begin /////////////////////////

    u8  mac_public[6];
    u8  mac_random_static[6];
    
    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);


    //////////// LinkLayer Initialization  Begin /////////////////////////
    blc_ll_initBasicMCU();

    blc_ll_initStandby_module(mac_public);                         //mandatory

    rf_set_power_level_index(RF_POWER_P9dBm);

    //////////////////////////// User Configuration for BLE application ////////////////////////////

    bool audioInit = app_audio_init();

    u8 error_code = blc_contr_checkControllerInitialization();
    if(error_code != INIT_SUCCESS || !audioInit){
        /* It's recommended that user set some UI alarm to know the exact error, e.g. LED shine, print log */
        write_log32(0x88880000 | error_code);
        #if(UI_LED_ENABLE)
            gpio_write(GPIO_LED_RED, LED_ON_LEVEL);
        #endif
        #if (TLKAPI_DEBUG_ENABLE)
            tlkapi_printf(APP_LOG_EN, "[APP][INI] Controller INIT ERROR, %d\r\n", error_code);
            while(1){
                tlkapi_debug_handler();
            }
        #else
            while(1);
        #endif
    }

    tlkapi_printf(APP_LOG_EN, "[APP][INT]audio broadcast source initial\n");
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
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void main_loop (void)
{

    ////////////////////////////////////// BLE entry /////////////////////////////////
    blc_sdk_main_loop();

#if (UI_LED_ENABLE)
    static u32 tick=0;
    if(clock_time_exceed(tick, 500*1000))
    {
        tick = clock_time();
        gpio_toggle(GPIO_LED_GREEN);
    }
#endif

    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif

    ////////////////////////////////////// Broadcast Source Audio entry /////////////////////////////////
    app_audio_handler();
}

#endif      //SOURCE_VERSION == SOURCE_ONLY_VERSION

