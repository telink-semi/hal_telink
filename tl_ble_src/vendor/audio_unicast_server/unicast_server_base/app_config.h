/********************************************************************************************************
 * @file    app_config.h
 *
 * @brief   This is the header file for BLE SDK
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
#pragma once

#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_BASE)
#define CONN_MAX_NUM_CONFIG                              CONN_MAX_NUM_C2_P2

#define ACL_CENTRAL_MAX_NUM                              0 // ACL central maximum number
#define ACL_PERIPHR_MAX_NUM                              1 // ACL peripheral maximum number



#define SIHUI_DEBUG_AUDIO                           1


#if (SIHUI_DEBUG_AUDIO)
    #define DEBUG_SIHUI_GPIO_ENABLE                     1
    #define APP_FLASH_INIT_LOG_EN                       1
    #define PERIPHR_CONNECT_CENTRAL_MAC_FILTER_EN       1  //remove when release SDK
    #define TLKAPI_DEBUG_FIFO_NUM                       32
#endif



#if (MCU_CORE_TYPE == MCU_CORE_B91)
#define TLSR9517CDK56D            1
#define TLSR9518ADK80D            2

#define HARDWARE_TYPE                TLSR9518ADK80D
#endif

#if (MCU_CORE_TYPE == MCU_CORE_B92)
#define TLSR9528A                 1
#define TLSR9529A                 2
#define C1T266A20                 3
    #if (SIHUI_DEBUG_AUDIO)
        #define HARDWARE_TYPE             C1T266A20
    #else
        #define HARDWARE_TYPE             TLSR9529A
    #endif
#endif


///////////////////////// Audio Configuration////////////////////////////////////////////////
#define APP_SCENE_TWS                                    0
#define APP_SCENE_HEADSET_EP1_MULTIPLEXING               1

#define APP_SCENE                                        APP_SCENE_HEADSET_EP1_MULTIPLEXING

#define AUDIO_CLOCK_CALIB2_ALGORITHM_EN                  0
#define ALG_AUDIO_EN                                     0
#define EQ_AUDIO_EN                                      0
#define AUDIO_DEBUG_DATA_EN                              0

#if (HARDWARE_TYPE == TLSR9529A)
    #define EXT_CODEC_0581                               1
#else
    #define EXT_CODEC_0581                               0
#endif
#if (APP_SCENE == APP_SCENE_TWS)
    #define DEFAULT_DEV_NAME                             "tlk_le_earbud"
    #define DEFAULT_DEV_APPEARE                          GAP_APPEARANCE_EARBUD
    #define APP_AUDIO_MAX_SINK_EP                        1
    #define APP_AUDIO_MAX_SOURCE_EP                      1

    #define LC3_DECODE_CHANNEL_COUNT                     1
    #define LC3_ENCODE_CHANNEL_COUNT                     1

    #define AUDIO_UNICAST_SERVER_MAX_TRANSPORT_LATENCY  0x28  //40ms
    #define HEAP_MEM_SIZE_CFG                           9000
#elif(APP_SCENE == APP_SCENE_HEADSET_EP1_MULTIPLEXING)
    #define DEFAULT_DEV_NAME                             "tlk_le_headset"
    #define DEFAULT_DEV_APPEARE                          GAP_APPEARANCE_HEADSET

    #define APP_AUDIO_MAX_SINK_EP                        1
    #define APP_AUDIO_MAX_SOURCE_EP                      1

    #define LC3_DECODE_CHANNEL_COUNT                     2
    #define LC3_ENCODE_CHANNEL_COUNT                     1

    #define AUDIO_UNICAST_SERVER_MAX_TRANSPORT_LATENCY   0x28  //40ms
    #define HEAP_MEM_SIZE_CFG                            12000
#endif

#define APP_AUDIO_ASCSS_SINK_ASE_CNT                     APP_AUDIO_MAX_SINK_EP
#define APP_AUDIO_ASCSS_SRC_ASE_CNT                      APP_AUDIO_MAX_SOURCE_EP


///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_PERIPHR_SMP_ENABLE                           1   //1 for smp,  0 no security
#define ACL_CENTRAL_SMP_ENABLE                           0   //1 for smp,  0 no security

///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define TLK_LED_ENABLE                                   1
#define TLK_KEY_ENABLE                                   1

#define TLK_TONE_ENABLE                                  0
#if (TLK_TONE_ENABLE)
#define TLK_TONE_MONO_MODE                               1
#endif

///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#define USER_DEBUG_ENABLE                                0

#define TLKAPI_DEBUG_ENABLE                              1
#define TLKAPI_DEBUG_CHANNEL                             TLKAPI_DEBUG_CHANNEL_UDB
#define APP_LOG_EN                                       1
#define APP_LED_LOG_EN                                   0
#define APP_KEY_LOG_EN                                   0


#define BLC_PM_DEEP_RETENTION_MODE_EN                    0
#define JTAG_DEBUG_DISABLE                               1

/**
 *  @brief  GPIO definition for keyboard
 */
#if TLK_KEY_ENABLE

#if(MCU_CORE_TYPE == MCU_CORE_B91 && HARDWARE_TYPE == TLSR9517CDK56D)
#define KEY1_GPIO_IN     GPIO_PB2
#define KEY1_GPIO_OUT    0

#define KEY2_GPIO_IN     GPIO_PB1
#define KEY2_GPIO_OUT    0

#define KEY3_GPIO_IN     GPIO_PB0
#define KEY3_GPIO_OUT    0
#endif

#if(MCU_CORE_TYPE == MCU_CORE_B91 && HARDWARE_TYPE == TLSR9518ADK80D)
#define KEY1_GPIO_IN     GPIO_PC2
#define KEY1_GPIO_OUT    GPIO_PC3

#define KEY2_GPIO_IN     GPIO_PC2
#define KEY2_GPIO_OUT    GPIO_PC1

#define KEY3_GPIO_IN     GPIO_PC0
#define KEY3_GPIO_OUT    GPIO_PC3

#define KEY4_GPIO_IN     GPIO_PC0
#define KEY4_GPIO_OUT    GPIO_PC1
#endif

#if(MCU_CORE_TYPE == MCU_CORE_B92 && HARDWARE_TYPE == TLSR9528A)
#define KEY1_GPIO_IN     GPIO_PD2
#define KEY1_GPIO_OUT    GPIO_PD6

#define KEY2_GPIO_IN     GPIO_PD2
#define KEY2_GPIO_OUT    GPIO_PF6

#define KEY3_GPIO_IN     GPIO_PD7
#define KEY3_GPIO_OUT    GPIO_PD6

#define KEY4_GPIO_IN     GPIO_PD7
#define KEY4_GPIO_OUT    GPIO_PF6
#endif

#if(MCU_CORE_TYPE == MCU_CORE_B92 && HARDWARE_TYPE == TLSR9529A)

#define KEY1_GPIO_IN     GPIO_PE2
#define KEY1_GPIO_OUT    GPIO_PE4

#define KEY2_GPIO_IN     GPIO_PE2
#define KEY2_GPIO_OUT    GPIO_PF2

#define KEY3_GPIO_IN     GPIO_PE3
#define KEY3_GPIO_OUT    GPIO_PE4

#define KEY4_GPIO_IN     GPIO_PE3
#define KEY4_GPIO_OUT    GPIO_PF2

#elif(MCU_CORE_TYPE == MCU_CORE_B92 && HARDWARE_TYPE == C1T266A20)

#define KEY1_GPIO_IN     GPIO_PD6
#define KEY1_GPIO_OUT    GPIO_PD7

#define KEY2_GPIO_IN     GPIO_PD6
#define KEY2_GPIO_OUT    GPIO_PD2

#define KEY3_GPIO_IN     GPIO_PF6
#define KEY3_GPIO_OUT    GPIO_PD7

#define KEY4_GPIO_IN     GPIO_PF6
#define KEY4_GPIO_OUT    GPIO_PD2

#endif


#define TLK_KEY_NUM_MAX           4
#define TLK_KEY_VALID_LEVEL       0 /** 1 is high level */



#if (TLK_KEY_NUM_MAX >= 1 && defined KEY1_GPIO_IN)
#define KEY1_ID tlk_key_get_id(KEY1_GPIO_IN, KEY1_GPIO_OUT)
#endif

#if (TLK_KEY_NUM_MAX >= 2 && defined KEY2_GPIO_IN)
#define KEY2_ID tlk_key_get_id(KEY2_GPIO_IN, KEY2_GPIO_OUT)
#endif

#if (TLK_KEY_NUM_MAX >= 3 && defined KEY3_GPIO_IN)
#define KEY3_ID tlk_key_get_id(KEY3_GPIO_IN, KEY3_GPIO_OUT)
#endif

#if (TLK_KEY_NUM_MAX >= 4 && defined KEY4_GPIO_IN)
#define KEY4_ID tlk_key_get_id(KEY4_GPIO_IN, KEY4_GPIO_OUT)
#endif

#endif

/**
 *  @brief  GPIO definition for LED
 */
#if TLK_LED_ENABLE

#if(MCU_CORE_TYPE == MCU_CORE_B91 && HARDWARE_TYPE == TLSR9517CDK56D) /* PWM0 not used */

//#define LED1_PIN       GPIO_PD0   //PWM0  R
#define LED2_PIN         GPIO_PD1   //PWM1  W
#define LED3_PIN         GPIO_PD2   //PWM2  G
#define LED4_PIN         GPIO_PD3   //PWM3  B

#endif

#if(MCU_CORE_TYPE == MCU_CORE_B91 && HARDWARE_TYPE == TLSR9518ADK80D) /* PWM0 not used */

//#define LED1_PIN       GPIO_PB4   //PWM0  B
#define LED2_PIN         GPIO_PB5   //PWM1  G
//#define LED3_PIN       GPIO_PB6   //not pwm pin
#define LED4_PIN         GPIO_PB7   //PWM2  R

#endif

#if(MCU_CORE_TYPE == MCU_CORE_B92 && HARDWARE_TYPE == TLSR9528A)

#define LED1_PIN         GPIO_PD0
#define LED2_PIN         GPIO_PD1
#define LED3_PIN         GPIO_PE6
#define LED4_PIN         GPIO_PE7

#endif

#if(MCU_CORE_TYPE == MCU_CORE_B92 && HARDWARE_TYPE == TLSR9529A)

#define LED1_PIN         GPIO_PD3
#define LED2_PIN         GPIO_PD4
#define LED3_PIN         GPIO_PD5
#define LED4_PIN         GPIO_PD6

#elif(MCU_CORE_TYPE == MCU_CORE_B92 && HARDWARE_TYPE == C1T266A20)

#define LED1_PIN         GPIO_PD0
#define LED2_PIN         GPIO_PD1
#define LED3_PIN         GPIO_PE6
#define LED4_PIN         GPIO_PE7

#endif

#define TLK_LED_NUM_MAX           4        /** led numbers */
#define TLK_LED_VALID_LEVEL       1        /** 1 is high level */

#ifdef LED1_PIN
#if (TLK_LED_NUM_MAX >= 1)
#define LED1_ID tlk_get_led_id_by_gpio(LED1_PIN)
#endif
#endif

#ifdef LED2_PIN
#if (TLK_LED_NUM_MAX >= 2)
#define LED2_ID tlk_get_led_id_by_gpio(LED2_PIN)
#endif
#endif

#ifdef LED3_PIN
#if (TLK_LED_NUM_MAX >= 3)
#define LED3_ID tlk_get_led_id_by_gpio(LED3_PIN)
#endif
#endif

#ifdef LED4_PIN
#if (TLK_LED_NUM_MAX >= 4)
#define LED4_ID tlk_get_led_id_by_gpio(LED4_PIN)
#endif
#endif

#endif


#if (JTAG_DEBUG_DISABLE)
    //JTAG will cost some power
    #if (MCU_CORE_TYPE == MCU_CORE_B91)
        #define PE4_FUNC            AS_GPIO
        #define PE5_FUNC            AS_GPIO
        #define PE6_FUNC            AS_GPIO
        #define PE7_FUNC            AS_GPIO

        #define PE4_INPUT_ENABLE    0
        #define PE5_INPUT_ENABLE    0
        #define PE6_INPUT_ENABLE    0
        #define PE7_INPUT_ENABLE    0

        #define PULL_WAKEUP_SRC_PE4 0
        #define PULL_WAKEUP_SRC_PE5 0
        #define PULL_WAKEUP_SRC_PE6 0
        #define PULL_WAKEUP_SRC_PE7 0
    #elif (MCU_CORE_TYPE == MCU_CORE_B92)
        #define PC4_FUNC            AS_GPIO
        #define PC5_FUNC            AS_GPIO
        #define PC6_FUNC            AS_GPIO
        #define PC7_FUNC            AS_GPIO

        #define PC4_INPUT_ENABLE    0
        #define PC5_INPUT_ENABLE    0
        #define PC6_INPUT_ENABLE    0
        #define PC7_INPUT_ENABLE    0

        #define PULL_WAKEUP_SRC_PC4 0
        #define PULL_WAKEUP_SRC_PC5 0
        #define PULL_WAKEUP_SRC_PC6 0
        #define PULL_WAKEUP_SRC_PC7 0
    #endif
#endif




#if (DEBUG_GPIO_ENABLE || DEBUG_SIHUI_GPIO_ENABLE)
    #if (MCU_CORE_TYPE == MCU_CORE_B91)
        #define GPIO_CHN0                           GPIO_PE1
        #define GPIO_CHN1                           GPIO_PE2
        #define GPIO_CHN2                           GPIO_PA0
        #define GPIO_CHN3                           GPIO_PA4
        #define GPIO_CHN4                           GPIO_PA3
        #define GPIO_CHN5                           GPIO_PB0
        #define GPIO_CHN6                           GPIO_PB2
        #define GPIO_CHN7                           GPIO_PE0

        #define GPIO_CHN8                           GPIO_PA2
        #define GPIO_CHN9                           GPIO_PA1
        #define GPIO_CHN10                          GPIO_PB1
        #define GPIO_CHN11                          GPIO_PB3
        #define GPIO_CHN12                          GPIO_PC7
        #define GPIO_CHN13                          GPIO_PC6
        #define GPIO_CHN14                          GPIO_PC5
        #define GPIO_CHN15                          GPIO_PC4


        #define PE1_OUTPUT_ENABLE                   1
        #define PE2_OUTPUT_ENABLE                   1
        #define PA0_OUTPUT_ENABLE                   1
        #define PA4_OUTPUT_ENABLE                   1
        #define PA3_OUTPUT_ENABLE                   1
        #define PB0_OUTPUT_ENABLE                   1
        #define PB2_OUTPUT_ENABLE                   1
        #define PE0_OUTPUT_ENABLE                   1

        #define PA2_OUTPUT_ENABLE                   1
        #define PA1_OUTPUT_ENABLE                   1
        #define PB1_OUTPUT_ENABLE                   1
        #define PB3_OUTPUT_ENABLE                   1
        #define PC7_OUTPUT_ENABLE                   1
        #define PC6_OUTPUT_ENABLE                   1
        #define PC5_OUTPUT_ENABLE                   1
        #define PC4_OUTPUT_ENABLE                   1
    #elif (MCU_CORE_TYPE == MCU_CORE_B92)
        #define GPIO_CHN0                           GPIO_PA1
        #define GPIO_CHN1                           GPIO_PA2
        #define GPIO_CHN2                           GPIO_PA3
        #define GPIO_CHN3                           GPIO_PA4
        #define GPIO_CHN4                           GPIO_PB1
        #define GPIO_CHN5                           GPIO_PB2
        #define GPIO_CHN6                           GPIO_PB3
        #define GPIO_CHN7                           GPIO_PB4

        #define GPIO_CHN8                           GPIO_PB5
        #define GPIO_CHN9                           GPIO_PB6
        #define GPIO_CHN10                          GPIO_PB7
        #define GPIO_CHN11                          GPIO_PC0
        #define GPIO_CHN12                          GPIO_PE0
        #define GPIO_CHN13                          GPIO_PE1
        #define GPIO_CHN14                          GPIO_PE2
        #define GPIO_CHN15                          GPIO_PE3


        #define PA1_OUTPUT_ENABLE                   1
        #define PA2_OUTPUT_ENABLE                   1
        #define PA3_OUTPUT_ENABLE                   1
        #define PA4_OUTPUT_ENABLE                   1
        #define PB1_OUTPUT_ENABLE                   1
        #define PB2_OUTPUT_ENABLE                   1
        #define PB3_OUTPUT_ENABLE                   1
        #define PB4_OUTPUT_ENABLE                   1

        #define PB5_OUTPUT_ENABLE                   1
        #define PB6_OUTPUT_ENABLE                   1
        #define PB7_OUTPUT_ENABLE                   1
        #define PC0_OUTPUT_ENABLE                   1
        #define PE0_OUTPUT_ENABLE                   1
        #define PE1_OUTPUT_ENABLE                   1
        #define PE2_OUTPUT_ENABLE                   1
        #define PE3_OUTPUT_ENABLE                   1
    #endif
#endif  //end of DEBUG_GPIO_ENABLE


#if (USER_DEBUG_ENABLE)

#if(MCU_CORE_TYPE == MCU_CORE_B91 && HARDWARE_TYPE == TLSR9518ADK80D)
#define DBG_GPIO_CHN_0                          GPIO_PE1
#define DBG_GPIO_CHN_1                          GPIO_PE2
#define DBG_GPIO_CHN_2                          GPIO_PA0
#define DBG_GPIO_CHN_3                          GPIO_PA4
#define DBG_GPIO_CHN_4                          GPIO_PA3
#define DBG_GPIO_CHN_5                          GPIO_PB0
#define DBG_GPIO_CHN_6                          GPIO_PB2
#define DBG_GPIO_CHN_7                          GPIO_PE0

#define DBG_GPIO_CHN_8                          GPIO_PA2
#define DBG_GPIO_CHN_9                          GPIO_PA1
#define DBG_GPIO_CHN_10                         GPIO_PB1
#define DBG_GPIO_CHN_11                         GPIO_PB3
#define DBG_GPIO_CHN_12                         GPIO_PC7
#define DBG_GPIO_CHN_13                         GPIO_PC6
#define DBG_GPIO_CHN_14                         GPIO_PC5
#define DBG_GPIO_CHN_15                         GPIO_PC4

#define PE1_OUTPUT_ENABLE                   1
#define PE2_OUTPUT_ENABLE                   1
#define PA0_OUTPUT_ENABLE                   1
#define PA4_OUTPUT_ENABLE                   1
#define PA3_OUTPUT_ENABLE                   1
#define PB0_OUTPUT_ENABLE                   1
#define PB2_OUTPUT_ENABLE                   1
#define PE0_OUTPUT_ENABLE                   1

#define PA2_OUTPUT_ENABLE                   1
#define PA1_OUTPUT_ENABLE                   1
#define PB1_OUTPUT_ENABLE                   1
#define PB3_OUTPUT_ENABLE                   1
#define PC7_OUTPUT_ENABLE                   1
#define PC6_OUTPUT_ENABLE                   1
#define PC5_OUTPUT_ENABLE                   1
#define PC4_OUTPUT_ENABLE                   1
#endif

#if(MCU_CORE_TYPE == MCU_CORE_B92 && HARDWARE_TYPE == TLSR9528A)
#define DBG_GPIO_CHN_0                          GPIO_PA1
#define DBG_GPIO_CHN_1                          GPIO_PA2
#define DBG_GPIO_CHN_2                          GPIO_PA3
#define DBG_GPIO_CHN_3                          GPIO_PA4
#define DBG_GPIO_CHN_4                          GPIO_PB1
#define DBG_GPIO_CHN_5                          GPIO_PB2
#define DBG_GPIO_CHN_6                          GPIO_PB3
#define DBG_GPIO_CHN_7                          GPIO_PB4

#define DBG_GPIO_CHN_8                          GPIO_PB5
#define DBG_GPIO_CHN_9                          GPIO_PB6
#define DBG_GPIO_CHN_10                         GPIO_PB7
#define DBG_GPIO_CHN_11                         GPIO_PC0
#define DBG_GPIO_CHN_12                         GPIO_PE0
#define DBG_GPIO_CHN_13                         GPIO_PE1
#define DBG_GPIO_CHN_14                         GPIO_PE2
#define DBG_GPIO_CHN_15                         GPIO_PE3


#define PA1_OUTPUT_ENABLE                   1
#define PA2_OUTPUT_ENABLE                   1
#define PA3_OUTPUT_ENABLE                   1
#define PA4_OUTPUT_ENABLE                   1
#define PB1_OUTPUT_ENABLE                   1
#define PB2_OUTPUT_ENABLE                   1
#define PB3_OUTPUT_ENABLE                   1
#define PB4_OUTPUT_ENABLE                   1

#define PB5_OUTPUT_ENABLE                   1
#define PB6_OUTPUT_ENABLE                   1
#define PB7_OUTPUT_ENABLE                   1
#define PC0_OUTPUT_ENABLE                   1
#define PE0_OUTPUT_ENABLE                   1
#define PE1_OUTPUT_ENABLE                   1
#define PE2_OUTPUT_ENABLE                   1
#define PE3_OUTPUT_ENABLE                   1
#endif


#if(MCU_CORE_TYPE == MCU_CORE_B92 && HARDWARE_TYPE == TLSR9529A)
#define DBG_GPIO_CHN_0                          GPIO_PA0
#define DBG_GPIO_CHN_1                          GPIO_PA1
#define DBG_GPIO_CHN_2                          GPIO_PA2
#define DBG_GPIO_CHN_3                          GPIO_PA3
#define DBG_GPIO_CHN_4                          GPIO_PA4
#define DBG_GPIO_CHN_5                          GPIO_PB5
#define DBG_GPIO_CHN_6                          GPIO_PB6
#define DBG_GPIO_CHN_7                          GPIO_PD0

#define DBG_GPIO_CHN_8                          GPIO_PD7
#define DBG_GPIO_CHN_9                          GPIO_PD2
#define DBG_GPIO_CHN_10                         GPIO_PD1
#define DBG_GPIO_CHN_11                         GPIO_PF3
#define DBG_GPIO_CHN_12                         GPIO_PF4
#define DBG_GPIO_CHN_13                         GPIO_PF5
#define DBG_GPIO_CHN_14                         GPIO_PF6
#define DBG_GPIO_CHN_15                         GPIO_PF7

#define PA0_OUTPUT_ENABLE                   1
#define PA1_OUTPUT_ENABLE                   1
#define PA2_OUTPUT_ENABLE                   1
#define PA3_OUTPUT_ENABLE                   1
#define PA4_OUTPUT_ENABLE                   1
#define PB5_OUTPUT_ENABLE                   1
#define PB6_OUTPUT_ENABLE                   1
#define PD0_OUTPUT_ENABLE                   1

#define PD7_OUTPUT_ENABLE                   1
#define PD2_OUTPUT_ENABLE                   1
#define PD1_OUTPUT_ENABLE                   1
#define PF3_OUTPUT_ENABLE                   1
#define PF4_OUTPUT_ENABLE                   1
#define PF5_OUTPUT_ENABLE                   1
#define PF6_OUTPUT_ENABLE                   1
#define PF7_OUTPUT_ENABLE                   1
#endif
//main_loop
#define APP_DBG_CHN_0_LOW       gpio_write(DBG_GPIO_CHN_0, 0)
#define APP_DBG_CHN_0_HIGH      gpio_write(DBG_GPIO_CHN_0, 1)

//tx encode and send
#define APP_DBG_CHN_1_LOW       gpio_write(DBG_GPIO_CHN_1, 0)
#define APP_DBG_CHN_1_HIGH      gpio_write(DBG_GPIO_CHN_1, 1)

//rx receice
#define APP_DBG_CHN_2_LOW       gpio_write(DBG_GPIO_CHN_2, 0)
#define APP_DBG_CHN_2_HIGH      gpio_write(DBG_GPIO_CHN_2, 1)

//render common
#define APP_DBG_CHN_3_LOW       gpio_write(DBG_GPIO_CHN_3, 0)
#define APP_DBG_CHN_3_HIGH      gpio_write(DBG_GPIO_CHN_3, 1)

//render offset exceed
#define APP_DBG_CHN_4_LOW       gpio_write(DBG_GPIO_CHN_4, 0)
#define APP_DBG_CHN_4_HIGH      gpio_write(DBG_GPIO_CHN_4, 1)

//ST,return
#define APP_DBG_CHN_5_LOW       gpio_write(DBG_GPIO_CHN_5, 0)
#define APP_DBG_CHN_5_HIGH      gpio_write(DBG_GPIO_CHN_5, 1)

//lc3 plc
#define APP_DBG_CHN_6_LOW       gpio_write(DBG_GPIO_CHN_6, 0)
#define APP_DBG_CHN_6_HIGH      gpio_write(DBG_GPIO_CHN_6, 1)

//rx common
#define APP_DBG_CHN_7_LOW       gpio_write(DBG_GPIO_CHN_7, 0)
#define APP_DBG_CHN_7_HIGH      gpio_write(DBG_GPIO_CHN_7, 1)

//delete fail
#define APP_DBG_CHN_8_LOW       gpio_write(DBG_GPIO_CHN_8, 0)
#define APP_DBG_CHN_8_HIGH      gpio_write(DBG_GPIO_CHN_8, 1)

//delete find
#define APP_DBG_CHN_9_LOW       gpio_write(DBG_GPIO_CHN_9, 0)
#define APP_DBG_CHN_9_HIGH      gpio_write(DBG_GPIO_CHN_9, 1)

//add find list end
#define APP_DBG_CHN_10_LOW      gpio_write(DBG_GPIO_CHN_10, 0)
#define APP_DBG_CHN_10_HIGH     gpio_write(DBG_GPIO_CHN_10, 1)

//time exceed,return
#define APP_DBG_CHN_11_LOW      gpio_write(DBG_GPIO_CHN_11, 0)
#define APP_DBG_CHN_11_HIGH     gpio_write(DBG_GPIO_CHN_11, 1)

//irq disable time
#define APP_DBG_CHN_12_LOW      gpio_write(DBG_GPIO_CHN_12, 0)
#define APP_DBG_CHN_12_HIGH     gpio_write(DBG_GPIO_CHN_12, 1)

//malloc fail
#define APP_DBG_CHN_13_LOW      gpio_write(DBG_GPIO_CHN_13, 0)
#define APP_DBG_CHN_13_HIGH     gpio_write(DBG_GPIO_CHN_13, 1)

#define APP_DBG_CHN_14_LOW      gpio_write(DBG_GPIO_CHN_14, 0)
#define APP_DBG_CHN_14_HIGH     gpio_write(DBG_GPIO_CHN_14, 1)

#define APP_DBG_CHN_15_LOW      gpio_write(DBG_GPIO_CHN_15, 0)
#define APP_DBG_CHN_15_HIGH     gpio_write(DBG_GPIO_CHN_15, 1)


#endif




#ifndef APP_DBG_CHN_0_LOW
#define APP_DBG_CHN_0_LOW
#endif
#ifndef APP_DBG_CHN_0_HIGH
#define APP_DBG_CHN_0_HIGH
#endif

#ifndef APP_DBG_CHN_1_LOW
#define APP_DBG_CHN_1_LOW
#endif
#ifndef APP_DBG_CHN_1_HIGH
#define APP_DBG_CHN_1_HIGH
#endif


#ifndef APP_DBG_CHN_2_LOW
#define APP_DBG_CHN_2_LOW
#endif
#ifndef APP_DBG_CHN_2_HIGH
#define APP_DBG_CHN_2_HIGH
#endif

#ifndef APP_DBG_CHN_3_LOW
#define APP_DBG_CHN_3_LOW
#endif
#ifndef APP_DBG_CHN_3_HIGH
#define APP_DBG_CHN_3_HIGH
#endif

#ifndef APP_DBG_CHN_4_LOW
#define APP_DBG_CHN_4_LOW
#endif
#ifndef APP_DBG_CHN_4_HIGH
#define APP_DBG_CHN_4_HIGH
#endif

#ifndef APP_DBG_CHN_5_LOW
#define APP_DBG_CHN_5_LOW
#endif
#ifndef APP_DBG_CHN_5_HIGH
#define APP_DBG_CHN_5_HIGH
#endif

#ifndef APP_DBG_CHN_6_LOW
#define APP_DBG_CHN_6_LOW
#endif
#ifndef APP_DBG_CHN_6_HIGH
#define APP_DBG_CHN_6_HIGH
#endif

#ifndef APP_DBG_CHN_7_LOW
#define APP_DBG_CHN_7_LOW
#endif
#ifndef APP_DBG_CHN_7_HIGH
#define APP_DBG_CHN_7_HIGH
#endif

#ifndef APP_DBG_CHN_8_LOW
#define APP_DBG_CHN_8_LOW
#endif
#ifndef APP_DBG_CHN_8_HIGH
#define APP_DBG_CHN_8_HIGH
#endif

#ifndef APP_DBG_CHN_9_LOW
#define APP_DBG_CHN_9_LOW
#endif
#ifndef APP_DBG_CHN_9_HIGH
#define APP_DBG_CHN_9_HIGH
#endif

#ifndef APP_DBG_CHN_10_LOW
#define APP_DBG_CHN_10_LOW
#endif
#ifndef APP_DBG_CHN_10_HIGH
#define APP_DBG_CHN_10_HIGH
#endif

#ifndef APP_DBG_CHN_11_LOW
#define APP_DBG_CHN_11_LOW
#endif
#ifndef APP_DBG_CHN_11_HIGH
#define APP_DBG_CHN_11_HIGH
#endif

#ifndef APP_DBG_CHN_12_LOW
#define APP_DBG_CHN_12_LOW
#endif
#ifndef APP_DBG_CHN_12_HIGH
#define APP_DBG_CHN_12_HIGH
#endif

#ifndef APP_DBG_CHN_13_LOW
#define APP_DBG_CHN_13_LOW
#endif
#ifndef APP_DBG_CHN_13_HIGH
#define APP_DBG_CHN_13_HIGH
#endif

#ifndef APP_DBG_CHN_14_LOW
#define APP_DBG_CHN_14_LOW
#endif
#ifndef APP_DBG_CHN_14_HIGH
#define APP_DBG_CHN_14_HIGH
#endif

#ifndef APP_DBG_CHN_15_LOW
#define APP_DBG_CHN_15_LOW
#endif
#ifndef APP_DBG_CHN_15_HIGH
#define APP_DBG_CHN_15_HIGH
#endif







#include "../common/default_config.h"

#endif

