/********************************************************************************************************
 * @file    gpio_private.h
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
#ifndef STACK_BLE_DEBUG_GPIO_PRIVATE_H_
#define STACK_BLE_DEBUG_GPIO_PRIVATE_H_

/*****************************************************************************************************************

                                            GPIO for different person

*****************************************************************************************************************/
#ifndef DEBUG_SIHUI_GPIO_ENABLE
#define DEBUG_SIHUI_GPIO_ENABLE                             0
#endif


#ifndef DEBUG_QIUWEI_GPIO_ENABLE
#define DEBUG_QIUWEI_GPIO_ENABLE                            0
#endif

#ifndef DEBUG_TIANXIANG_GPIO_ENABLE
#define DEBUG_TIANXIANG_GPIO_ENABLE                         0
#endif

#ifndef DEBUG_YAFEI_GPIO_ENABLE
#define DEBUG_YAFEI_GPIO_ENABLE                             0
#endif

#ifndef DEBUG_FANQH_GPIO_ENABLE
#define DEBUG_FANQH_GPIO_ENABLE                             0
#endif


#ifndef DEBUG_CS_GPIO_ENABLE
#define DEBUG_CS_GPIO_ENABLE                                0
#endif


#if (DEBUG_SIHUI_GPIO_ENABLE)
    #ifdef GPIO_CHN0
        #define DBG_SIHUI_CHN0_LOW      gpio_write(GPIO_CHN0, 0)
        #define DBG_SIHUI_CHN0_HIGH     gpio_write(GPIO_CHN0, 1)
        #define DBG_SIHUI_CHN0_TOGGLE   gpio_toggle(GPIO_CHN0)
    #else
        #define DBG_SIHUI_CHN0_LOW
        #define DBG_SIHUI_CHN0_HIGH
        #define DBG_SIHUI_CHN0_TOGGLE
    #endif

    #ifdef  GPIO_CHN1
        #define DBG_SIHUI_CHN1_LOW      gpio_write(GPIO_CHN1, 0)
        #define DBG_SIHUI_CHN1_HIGH     gpio_write(GPIO_CHN1, 1)
        #define DBG_SIHUI_CHN1_TOGGLE   gpio_toggle(GPIO_CHN1)
    #else
        #define DBG_SIHUI_CHN1_LOW
        #define DBG_SIHUI_CHN1_HIGH
        #define DBG_SIHUI_CHN1_TOGGLE
    #endif

    #ifdef  GPIO_CHN2
        #define DBG_SIHUI_CHN2_LOW      gpio_write(GPIO_CHN2, 0)
        #define DBG_SIHUI_CHN2_HIGH     gpio_write(GPIO_CHN2, 1)
        #define DBG_SIHUI_CHN2_TOGGLE   gpio_toggle(GPIO_CHN2)
    #else
        #define DBG_SIHUI_CHN2_LOW
        #define DBG_SIHUI_CHN2_HIGH
        #define DBG_SIHUI_CHN2_TOGGLE
    #endif

    #ifdef  GPIO_CHN3
        #define DBG_SIHUI_CHN3_LOW      gpio_write(GPIO_CHN3, 0)
        #define DBG_SIHUI_CHN3_HIGH     gpio_write(GPIO_CHN3, 1)
        #define DBG_SIHUI_CHN3_TOGGLE   gpio_toggle(GPIO_CHN3)
    #else
        #define DBG_SIHUI_CHN3_LOW
        #define DBG_SIHUI_CHN3_HIGH
        #define DBG_SIHUI_CHN3_TOGGLE
    #endif

    #ifdef GPIO_CHN4
        #define DBG_SIHUI_CHN4_LOW      gpio_write(GPIO_CHN4, 0)
        #define DBG_SIHUI_CHN4_HIGH     gpio_write(GPIO_CHN4, 1)
        #define DBG_SIHUI_CHN4_TOGGLE   gpio_toggle(GPIO_CHN4)
    #else
        #define DBG_SIHUI_CHN4_LOW
        #define DBG_SIHUI_CHN4_HIGH
        #define DBG_SIHUI_CHN4_TOGGLE
    #endif

    #ifdef  GPIO_CHN5
        #define DBG_SIHUI_CHN5_LOW      gpio_write(GPIO_CHN5, 0)
        #define DBG_SIHUI_CHN5_HIGH     gpio_write(GPIO_CHN5, 1)
        #define DBG_SIHUI_CHN5_TOGGLE   gpio_toggle(GPIO_CHN5)
    #else
        #define DBG_SIHUI_CHN5_LOW
        #define DBG_SIHUI_CHN5_HIGH
        #define DBG_SIHUI_CHN5_TOGGLE
    #endif

    #ifdef  GPIO_CHN6
        #define DBG_SIHUI_CHN6_LOW      gpio_write(GPIO_CHN6, 0)
        #define DBG_SIHUI_CHN6_HIGH     gpio_write(GPIO_CHN6, 1)
        #define DBG_SIHUI_CHN6_TOGGLE   gpio_toggle(GPIO_CHN6)
    #else
        #define DBG_SIHUI_CHN6_LOW
        #define DBG_SIHUI_CHN6_HIGH
        #define DBG_SIHUI_CHN6_TOGGLE
    #endif

    #ifdef  GPIO_CHN7
        #define DBG_SIHUI_CHN7_LOW      gpio_write(GPIO_CHN7, 0)
        #define DBG_SIHUI_CHN7_HIGH     gpio_write(GPIO_CHN7, 1)
        #define DBG_SIHUI_CHN7_TOGGLE   gpio_toggle(GPIO_CHN7)
    #else
        #define DBG_SIHUI_CHN7_LOW
        #define DBG_SIHUI_CHN7_HIGH
        #define DBG_SIHUI_CHN7_TOGGLE
    #endif

    #ifdef GPIO_CHN8
        #define DBG_SIHUI_CHN8_LOW      gpio_write(GPIO_CHN8, 0)
        #define DBG_SIHUI_CHN8_HIGH     gpio_write(GPIO_CHN8, 1)
        #define DBG_SIHUI_CHN8_TOGGLE   gpio_toggle(GPIO_CHN8)
    #else
        #define DBG_SIHUI_CHN8_LOW
        #define DBG_SIHUI_CHN8_HIGH
        #define DBG_SIHUI_CHN8_TOGGLE
    #endif

    #ifdef  GPIO_CHN9
        #define DBG_SIHUI_CHN9_LOW      gpio_write(GPIO_CHN9, 0)
        #define DBG_SIHUI_CHN9_HIGH     gpio_write(GPIO_CHN9, 1)
        #define DBG_SIHUI_CHN9_TOGGLE   gpio_toggle(GPIO_CHN9)
    #else
        #define DBG_SIHUI_CHN9_LOW
        #define DBG_SIHUI_CHN9_HIGH
        #define DBG_SIHUI_CHN9_TOGGLE
    #endif

    #ifdef  GPIO_CHN10
        #define DBG_SIHUI_CHN10_LOW     gpio_write(GPIO_CHN10, 0)
        #define DBG_SIHUI_CHN10_HIGH    gpio_write(GPIO_CHN10, 1)
        #define DBG_SIHUI_CHN10_TOGGLE  gpio_toggle(GPIO_CHN10)
    #else
        #define DBG_SIHUI_CHN10_LOW
        #define DBG_SIHUI_CHN10_HIGH
        #define DBG_SIHUI_CHN10_TOGGLE
    #endif

    #ifdef  GPIO_CHN11
        #define DBG_SIHUI_CHN11_LOW     gpio_write(GPIO_CHN11, 0)
        #define DBG_SIHUI_CHN11_HIGH    gpio_write(GPIO_CHN11, 1)
        #define DBG_SIHUI_CHN11_TOGGLE  gpio_toggle(GPIO_CHN11)
    #else
        #define DBG_SIHUI_CHN11_LOW
        #define DBG_SIHUI_CHN11_HIGH
        #define DBG_SIHUI_CHN11_TOGGLE
    #endif

    #ifdef GPIO_CHN12
        #define DBG_SIHUI_CHN12_LOW     gpio_write(GPIO_CHN12, 0)
        #define DBG_SIHUI_CHN12_HIGH    gpio_write(GPIO_CHN12, 1)
        #define DBG_SIHUI_CHN12_TOGGLE  gpio_toggle(GPIO_CHN12)
    #else
        #define DBG_SIHUI_CHN12_LOW
        #define DBG_SIHUI_CHN12_HIGH
        #define DBG_SIHUI_CHN12_TOGGLE
    #endif

    #ifdef  GPIO_CHN13
        #define DBG_SIHUI_CHN13_LOW     gpio_write(GPIO_CHN13, 0)
        #define DBG_SIHUI_CHN13_HIGH    gpio_write(GPIO_CHN13, 1)
        #define DBG_SIHUI_CHN13_TOGGLE  gpio_toggle(GPIO_CHN13)
    #else
        #define DBG_SIHUI_CHN13_LOW
        #define DBG_SIHUI_CHN13_HIGH
        #define DBG_SIHUI_CHN13_TOGGLE
    #endif

    #ifdef  GPIO_CHN14
        #define DBG_SIHUI_CHN14_LOW     gpio_write(GPIO_CHN14, 0)
        #define DBG_SIHUI_CHN14_HIGH    gpio_write(GPIO_CHN14, 1)
        #define DBG_SIHUI_CHN14_TOGGLE  gpio_toggle(GPIO_CHN14)
    #else
        #define DBG_SIHUI_CHN14_LOW
        #define DBG_SIHUI_CHN14_HIGH
        #define DBG_SIHUI_CHN14_TOGGLE
    #endif

    #ifdef  GPIO_CHN15
        #define DBG_SIHUI_CHN15_LOW     gpio_write(GPIO_CHN15, 0)
        #define DBG_SIHUI_CHN15_HIGH    gpio_write(GPIO_CHN15, 1)
        #define DBG_SIHUI_CHN15_TOGGLE  gpio_toggle(GPIO_CHN15)
    #else
        #define DBG_SIHUI_CHN15_LOW
        #define DBG_SIHUI_CHN15_HIGH
        #define DBG_SIHUI_CHN15_TOGGLE
    #endif
#else
    #define DBG_SIHUI_CHN0_LOW
    #define DBG_SIHUI_CHN0_HIGH
    #define DBG_SIHUI_CHN0_TOGGLE
    #define DBG_SIHUI_CHN1_LOW
    #define DBG_SIHUI_CHN1_HIGH
    #define DBG_SIHUI_CHN1_TOGGLE
    #define DBG_SIHUI_CHN2_LOW
    #define DBG_SIHUI_CHN2_HIGH
    #define DBG_SIHUI_CHN2_TOGGLE
    #define DBG_SIHUI_CHN3_LOW
    #define DBG_SIHUI_CHN3_HIGH
    #define DBG_SIHUI_CHN3_TOGGLE
    #define DBG_SIHUI_CHN4_LOW
    #define DBG_SIHUI_CHN4_HIGH
    #define DBG_SIHUI_CHN4_TOGGLE
    #define DBG_SIHUI_CHN5_LOW
    #define DBG_SIHUI_CHN5_HIGH
    #define DBG_SIHUI_CHN5_TOGGLE
    #define DBG_SIHUI_CHN6_LOW
    #define DBG_SIHUI_CHN6_HIGH
    #define DBG_SIHUI_CHN6_TOGGLE
    #define DBG_SIHUI_CHN7_LOW
    #define DBG_SIHUI_CHN7_HIGH
    #define DBG_SIHUI_CHN7_TOGGLE
    #define DBG_SIHUI_CHN8_LOW
    #define DBG_SIHUI_CHN8_HIGH
    #define DBG_SIHUI_CHN8_TOGGLE
    #define DBG_SIHUI_CHN9_LOW
    #define DBG_SIHUI_CHN9_HIGH
    #define DBG_SIHUI_CHN9_TOGGLE
    #define DBG_SIHUI_CHN10_LOW
    #define DBG_SIHUI_CHN10_HIGH
    #define DBG_SIHUI_CHN10_TOGGLE
    #define DBG_SIHUI_CHN11_LOW
    #define DBG_SIHUI_CHN11_HIGH
    #define DBG_SIHUI_CHN11_TOGGLE
    #define DBG_SIHUI_CHN12_LOW
    #define DBG_SIHUI_CHN12_HIGH
    #define DBG_SIHUI_CHN12_TOGGLE
    #define DBG_SIHUI_CHN13_LOW
    #define DBG_SIHUI_CHN13_HIGH
    #define DBG_SIHUI_CHN13_TOGGLE
    #define DBG_SIHUI_CHN14_LOW
    #define DBG_SIHUI_CHN14_HIGH
    #define DBG_SIHUI_CHN14_TOGGLE
    #define DBG_SIHUI_CHN15_LOW
    #define DBG_SIHUI_CHN15_HIGH
    #define DBG_SIHUI_CHN15_TOGGLE
#endif







#if (DEBUG_QIUWEI_GPIO_ENABLE)
    #ifdef GPIO_CHN0
        #define DBG_QIUWEI_CHN0_LOW     gpio_write(GPIO_CHN0, 0)
        #define DBG_QIUWEI_CHN0_HIGH    gpio_write(GPIO_CHN0, 1)
        #define DBG_QIUWEI_CHN0_TOGGLE  gpio_toggle(GPIO_CHN0)
    #else
        #define DBG_QIUWEI_CHN0_LOW
        #define DBG_QIUWEI_CHN0_HIGH
        #define DBG_QIUWEI_CHN0_TOGGLE
    #endif

    #ifdef  GPIO_CHN1
        #define DBG_QIUWEI_CHN1_LOW     gpio_write(GPIO_CHN1, 0)
        #define DBG_QIUWEI_CHN1_HIGH    gpio_write(GPIO_CHN1, 1)
        #define DBG_QIUWEI_CHN1_TOGGLE  gpio_toggle(GPIO_CHN1)
    #else
        #define DBG_QIUWEI_CHN1_LOW
        #define DBG_QIUWEI_CHN1_HIGH
        #define DBG_QIUWEI_CHN1_TOGGLE
    #endif

    #ifdef  GPIO_CHN2
        #define DBG_QIUWEI_CHN2_LOW     gpio_write(GPIO_CHN2, 0)
        #define DBG_QIUWEI_CHN2_HIGH    gpio_write(GPIO_CHN2, 1)
        #define DBG_QIUWEI_CHN2_TOGGLE  gpio_toggle(GPIO_CHN2)
    #else
        #define DBG_QIUWEI_CHN2_LOW
        #define DBG_QIUWEI_CHN2_HIGH
        #define DBG_QIUWEI_CHN2_TOGGLE
    #endif

    #ifdef  GPIO_CHN3
        #define DBG_QIUWEI_CHN3_LOW     gpio_write(GPIO_CHN3, 0)
        #define DBG_QIUWEI_CHN3_HIGH    gpio_write(GPIO_CHN3, 1)
        #define DBG_QIUWEI_CHN3_TOGGLE  gpio_toggle(GPIO_CHN3)
    #else
        #define DBG_QIUWEI_CHN3_LOW
        #define DBG_QIUWEI_CHN3_HIGH
        #define DBG_QIUWEI_CHN3_TOGGLE
    #endif

    #ifdef GPIO_CHN4
        #define DBG_QIUWEI_CHN4_LOW     gpio_write(GPIO_CHN4, 0)
        #define DBG_QIUWEI_CHN4_HIGH    gpio_write(GPIO_CHN4, 1)
        #define DBG_QIUWEI_CHN4_TOGGLE  gpio_toggle(GPIO_CHN4)
    #else
        #define DBG_QIUWEI_CHN4_LOW
        #define DBG_QIUWEI_CHN4_HIGH
        #define DBG_QIUWEI_CHN4_TOGGLE
    #endif

    #ifdef  GPIO_CHN5
        #define DBG_QIUWEI_CHN5_LOW     gpio_write(GPIO_CHN5, 0)
        #define DBG_QIUWEI_CHN5_HIGH    gpio_write(GPIO_CHN5, 1)
        #define DBG_QIUWEI_CHN5_TOGGLE  gpio_toggle(GPIO_CHN5)
    #else
        #define DBG_QIUWEI_CHN5_LOW
        #define DBG_QIUWEI_CHN5_HIGH
        #define DBG_QIUWEI_CHN5_TOGGLE
    #endif

    #ifdef  GPIO_CHN6
        #define DBG_QIUWEI_CHN6_LOW     gpio_write(GPIO_CHN6, 0)
        #define DBG_QIUWEI_CHN6_HIGH    gpio_write(GPIO_CHN6, 1)
        #define DBG_QIUWEI_CHN6_TOGGLE  gpio_toggle(GPIO_CHN6)
    #else
        #define DBG_QIUWEI_CHN6_LOW
        #define DBG_QIUWEI_CHN6_HIGH
        #define DBG_QIUWEI_CHN6_TOGGLE
    #endif

    #ifdef  GPIO_CHN7
        #define DBG_QIUWEI_CHN7_LOW     gpio_write(GPIO_CHN7, 0)
        #define DBG_QIUWEI_CHN7_HIGH    gpio_write(GPIO_CHN7, 1)
        #define DBG_QIUWEI_CHN7_TOGGLE  gpio_toggle(GPIO_CHN7)
    #else
        #define DBG_QIUWEI_CHN7_LOW
        #define DBG_QIUWEI_CHN7_HIGH
        #define DBG_QIUWEI_CHN7_TOGGLE
    #endif

    #ifdef GPIO_CHN8
        #define DBG_QIUWEI_CHN8_LOW     gpio_write(GPIO_CHN8, 0)
        #define DBG_QIUWEI_CHN8_HIGH    gpio_write(GPIO_CHN8, 1)
        #define DBG_QIUWEI_CHN8_TOGGLE  gpio_toggle(GPIO_CHN8)
    #else
        #define DBG_QIUWEI_CHN8_LOW
        #define DBG_QIUWEI_CHN8_HIGH
        #define DBG_QIUWEI_CHN8_TOGGLE
    #endif

    #ifdef  GPIO_CHN9
        #define DBG_QIUWEI_CHN9_LOW     gpio_write(GPIO_CHN9, 0)
        #define DBG_QIUWEI_CHN9_HIGH    gpio_write(GPIO_CHN9, 1)
        #define DBG_QIUWEI_CHN9_TOGGLE  gpio_toggle(GPIO_CHN9)
    #else
        #define DBG_QIUWEI_CHN9_LOW
        #define DBG_QIUWEI_CHN9_HIGH
        #define DBG_QIUWEI_CHN9_TOGGLE
    #endif

    #ifdef  GPIO_CHN10
        #define DBG_QIUWEI_CHN10_LOW    gpio_write(GPIO_CHN10, 0)
        #define DBG_QIUWEI_CHN10_HIGH   gpio_write(GPIO_CHN10, 1)
        #define DBG_QIUWEI_CHN10_TOGGLE gpio_toggle(GPIO_CHN10)
    #else
        #define DBG_QIUWEI_CHN10_LOW
        #define DBG_QIUWEI_CHN10_HIGH
        #define DBG_QIUWEI_CHN10_TOGGLE
    #endif

    #ifdef  GPIO_CHN11
        #define DBG_QIUWEI_CHN11_LOW    gpio_write(GPIO_CHN11, 0)
        #define DBG_QIUWEI_CHN11_HIGH   gpio_write(GPIO_CHN11, 1)
        #define DBG_QIUWEI_CHN11_TOGGLE gpio_toggle(GPIO_CHN11)
    #else
        #define DBG_QIUWEI_CHN11_LOW
        #define DBG_QIUWEI_CHN11_HIGH
        #define DBG_QIUWEI_CHN11_TOGGLE
    #endif

    #ifdef GPIO_CHN12
        #define DBG_QIUWEI_CHN12_LOW    gpio_write(GPIO_CHN12, 0)
        #define DBG_QIUWEI_CHN12_HIGH   gpio_write(GPIO_CHN12, 1)
        #define DBG_QIUWEI_CHN12_TOGGLE gpio_toggle(GPIO_CHN12)
    #else
        #define DBG_QIUWEI_CHN12_LOW
        #define DBG_QIUWEI_CHN12_HIGH
        #define DBG_QIUWEI_CHN12_TOGGLE
    #endif

    #ifdef  GPIO_CHN13
        #define DBG_QIUWEI_CHN13_LOW    gpio_write(GPIO_CHN13, 0)
        #define DBG_QIUWEI_CHN13_HIGH   gpio_write(GPIO_CHN13, 1)
        #define DBG_QIUWEI_CHN13_TOGGLE gpio_toggle(GPIO_CHN13)
    #else
        #define DBG_QIUWEI_CHN13_LOW
        #define DBG_QIUWEI_CHN13_HIGH
        #define DBG_QIUWEI_CHN13_TOGGLE
    #endif

    #ifdef  GPIO_CHN14
        #define DBG_QIUWEI_CHN14_LOW    gpio_write(GPIO_CHN14, 0)
        #define DBG_QIUWEI_CHN14_HIGH   gpio_write(GPIO_CHN14, 1)
        #define DBG_QIUWEI_CHN14_TOGGLE gpio_toggle(GPIO_CHN14)
    #else
        #define DBG_QIUWEI_CHN14_LOW
        #define DBG_QIUWEI_CHN14_HIGH
        #define DBG_QIUWEI_CHN14_TOGGLE
    #endif

    #ifdef  GPIO_CHN15
        #define DBG_QIUWEI_CHN15_LOW    gpio_write(GPIO_CHN15, 0)
        #define DBG_QIUWEI_CHN15_HIGH   gpio_write(GPIO_CHN15, 1)
        #define DBG_QIUWEI_CHN15_TOGGLE gpio_toggle(GPIO_CHN15)
    #else
        #define DBG_QIUWEI_CHN15_LOW
        #define DBG_QIUWEI_CHN15_HIGH
        #define DBG_QIUWEI_CHN15_TOGGLE
    #endif
#else
    #define DBG_QIUWEI_CHN0_LOW
    #define DBG_QIUWEI_CHN0_HIGH
    #define DBG_QIUWEI_CHN0_TOGGLE
    #define DBG_QIUWEI_CHN1_LOW
    #define DBG_QIUWEI_CHN1_HIGH
    #define DBG_QIUWEI_CHN1_TOGGLE
    #define DBG_QIUWEI_CHN2_LOW
    #define DBG_QIUWEI_CHN2_HIGH
    #define DBG_QIUWEI_CHN2_TOGGLE
    #define DBG_QIUWEI_CHN3_LOW
    #define DBG_QIUWEI_CHN3_HIGH
    #define DBG_QIUWEI_CHN3_TOGGLE
    #define DBG_QIUWEI_CHN4_LOW
    #define DBG_QIUWEI_CHN4_HIGH
    #define DBG_QIUWEI_CHN4_TOGGLE
    #define DBG_QIUWEI_CHN5_LOW
    #define DBG_QIUWEI_CHN5_HIGH
    #define DBG_QIUWEI_CHN5_TOGGLE
    #define DBG_QIUWEI_CHN6_LOW
    #define DBG_QIUWEI_CHN6_HIGH
    #define DBG_QIUWEI_CHN6_TOGGLE
    #define DBG_QIUWEI_CHN7_LOW
    #define DBG_QIUWEI_CHN7_HIGH
    #define DBG_QIUWEI_CHN7_TOGGLE
    #define DBG_QIUWEI_CHN8_LOW
    #define DBG_QIUWEI_CHN8_HIGH
    #define DBG_QIUWEI_CHN8_TOGGLE
    #define DBG_QIUWEI_CHN9_LOW
    #define DBG_QIUWEI_CHN9_HIGH
    #define DBG_QIUWEI_CHN9_TOGGLE
    #define DBG_QIUWEI_CHN10_LOW
    #define DBG_QIUWEI_CHN10_HIGH
    #define DBG_QIUWEI_CHN10_TOGGLE
    #define DBG_QIUWEI_CHN11_LOW
    #define DBG_QIUWEI_CHN11_HIGH
    #define DBG_QIUWEI_CHN11_TOGGLE
    #define DBG_QIUWEI_CHN12_LOW
    #define DBG_QIUWEI_CHN12_HIGH
    #define DBG_QIUWEI_CHN12_TOGGLE
    #define DBG_QIUWEI_CHN13_LOW
    #define DBG_QIUWEI_CHN13_HIGH
    #define DBG_QIUWEI_CHN13_TOGGLE
    #define DBG_QIUWEI_CHN14_LOW
    #define DBG_QIUWEI_CHN14_HIGH
    #define DBG_QIUWEI_CHN14_TOGGLE
    #define DBG_QIUWEI_CHN15_LOW
    #define DBG_QIUWEI_CHN15_HIGH
    #define DBG_QIUWEI_CHN15_TOGGLE
#endif


#if (DEBUG_FANQH_GPIO_ENABLE)
    #ifdef GPIO_CHN0
        #define DBG_FANQH_CHN0_LOW      gpio_write(GPIO_CHN0, 0)
        #define DBG_FANQH_CHN0_HIGH     gpio_write(GPIO_CHN0, 1)
        #define DBG_FANQH_CHN0_TOGGLE   gpio_toggle(GPIO_CHN0)
    #else
        #define DBG_FANQH_CHN0_LOW
        #define DBG_FANQH_CHN0_HIGH
        #define DBG_FANQH_CHN0_TOGGLE
    #endif

    #ifdef  GPIO_CHN1
        #define DBG_FANQH_CHN1_LOW      gpio_write(GPIO_CHN1, 0)
        #define DBG_FANQH_CHN1_HIGH     gpio_write(GPIO_CHN1, 1)
        #define DBG_FANQH_CHN1_TOGGLE   gpio_toggle(GPIO_CHN1)
    #else
        #define DBG_FANQH_CHN1_LOW
        #define DBG_FANQH_CHN1_HIGH
        #define DBG_FANQH_CHN1_TOGGLE
    #endif

    #ifdef  GPIO_CHN2
        #define DBG_FANQH_CHN2_LOW      gpio_write(GPIO_CHN2, 0)
        #define DBG_FANQH_CHN2_HIGH     gpio_write(GPIO_CHN2, 1)
        #define DBG_FANQH_CHN2_TOGGLE   gpio_toggle(GPIO_CHN2)
    #else
        #define DBG_FANQH_CHN2_LOW
        #define DBG_FANQH_CHN2_HIGH
        #define DBG_FANQH_CHN2_TOGGLE
    #endif

    #ifdef  GPIO_CHN3
        #define DBG_FANQH_CHN3_LOW      gpio_write(GPIO_CHN3, 0)
        #define DBG_FANQH_CHN3_HIGH     gpio_write(GPIO_CHN3, 1)
        #define DBG_FANQH_CHN3_TOGGLE   gpio_toggle(GPIO_CHN3)
    #else
        #define DBG_FANQH_CHN3_LOW
        #define DBG_FANQH_CHN3_HIGH
        #define DBG_FANQH_CHN3_TOGGLE
    #endif

    #ifdef GPIO_CHN4
        #define DBG_FANQH_CHN4_LOW      gpio_write(GPIO_CHN4, 0)
        #define DBG_FANQH_CHN4_HIGH     gpio_write(GPIO_CHN4, 1)
        #define DBG_FANQH_CHN4_TOGGLE   gpio_toggle(GPIO_CHN4)
    #else
        #define DBG_FANQH_CHN4_LOW
        #define DBG_FANQH_CHN4_HIGH
        #define DBG_FANQH_CHN4_TOGGLE
    #endif

    #ifdef  GPIO_CHN5
        #define DBG_FANQH_CHN5_LOW      gpio_write(GPIO_CHN5, 0)
        #define DBG_FANQH_CHN5_HIGH     gpio_write(GPIO_CHN5, 1)
        #define DBG_FANQH_CHN5_TOGGLE   gpio_toggle(GPIO_CHN5)
    #else
        #define DBG_FANQH_CHN5_LOW
        #define DBG_FANQH_CHN5_HIGH
        #define DBG_FANQH_CHN5_TOGGLE
    #endif

    #ifdef  GPIO_CHN6
        #define DBG_FANQH_CHN6_LOW      gpio_write(GPIO_CHN6, 0)
        #define DBG_FANQH_CHN6_HIGH     gpio_write(GPIO_CHN6, 1)
        #define DBG_FANQH_CHN6_TOGGLE   gpio_toggle(GPIO_CHN6)
    #else
        #define DBG_FANQH_CHN6_LOW
        #define DBG_FANQH_CHN6_HIGH
        #define DBG_FANQH_CHN6_TOGGLE
    #endif

    #ifdef  GPIO_CHN7
        #define DBG_FANQH_CHN7_LOW      gpio_write(GPIO_CHN7, 0)
        #define DBG_FANQH_CHN7_HIGH     gpio_write(GPIO_CHN7, 1)
        #define DBG_FANQH_CHN7_TOGGLE   gpio_toggle(GPIO_CHN7)
    #else
        #define DBG_FANQH_CHN7_LOW
        #define DBG_FANQH_CHN7_HIGH
        #define DBG_FANQH_CHN7_TOGGLE
    #endif

    #ifdef GPIO_CHN8
        #define DBG_FANQH_CHN8_LOW      gpio_write(GPIO_CHN8, 0)
        #define DBG_FANQH_CHN8_HIGH     gpio_write(GPIO_CHN8, 1)
        #define DBG_FANQH_CHN8_TOGGLE   gpio_toggle(GPIO_CHN8)
    #else
        #define DBG_FANQH_CHN8_LOW
        #define DBG_FANQH_CHN8_HIGH
        #define DBG_FANQH_CHN8_TOGGLE
    #endif

    #ifdef  GPIO_CHN9
        #define DBG_FANQH_CHN9_LOW      gpio_write(GPIO_CHN9, 0)
        #define DBG_FANQH_CHN9_HIGH     gpio_write(GPIO_CHN9, 1)
        #define DBG_FANQH_CHN9_TOGGLE   gpio_toggle(GPIO_CHN9)
    #else
        #define DBG_FANQH_CHN9_LOW
        #define DBG_FANQH_CHN9_HIGH
        #define DBG_FANQH_CHN9_TOGGLE
    #endif

    #ifdef  GPIO_CHN10
        #define DBG_FANQH_CHN10_LOW     gpio_write(GPIO_CHN10, 0)
        #define DBG_FANQH_CHN10_HIGH    gpio_write(GPIO_CHN10, 1)
        #define DBG_FANQH_CHN10_TOGGLE  gpio_toggle(GPIO_CHN10)
    #else
        #define DBG_FANQH_CHN10_LOW
        #define DBG_FANQH_CHN10_HIGH
        #define DBG_FANQH_CHN10_TOGGLE
    #endif

    #ifdef  GPIO_CHN11
        #define DBG_FANQH_CHN11_LOW     gpio_write(GPIO_CHN11, 0)
        #define DBG_FANQH_CHN11_HIGH    gpio_write(GPIO_CHN11, 1)
        #define DBG_FANQH_CHN11_TOGGLE  gpio_toggle(GPIO_CHN11)
    #else
        #define DBG_FANQH_CHN11_LOW
        #define DBG_FANQH_CHN11_HIGH
        #define DBG_FANQH_CHN11_TOGGLE
    #endif

    #ifdef GPIO_CHN12
        #define DBG_FANQH_CHN12_LOW     gpio_write(GPIO_CHN12, 0)
        #define DBG_FANQH_CHN12_HIGH    gpio_write(GPIO_CHN12, 1)
        #define DBG_FANQH_CHN12_TOGGLE  gpio_toggle(GPIO_CHN12)
    #else
        #define DBG_FANQH_CHN12_LOW
        #define DBG_FANQH_CHN12_HIGH
        #define DBG_FANQH_CHN12_TOGGLE
    #endif

    #ifdef  GPIO_CHN13
        #define DBG_FANQH_CHN13_LOW     gpio_write(GPIO_CHN13, 0)
        #define DBG_FANQH_CHN13_HIGH    gpio_write(GPIO_CHN13, 1)
        #define DBG_FANQH_CHN13_TOGGLE  gpio_toggle(GPIO_CHN13)
    #else
        #define DBG_FANQH_CHN13_LOW
        #define DBG_FANQH_CHN13_HIGH
        #define DBG_FANQH_CHN13_TOGGLE
    #endif

    #ifdef  GPIO_CHN14
        #define DBG_FANQH_CHN14_LOW     gpio_write(GPIO_CHN14, 0)
        #define DBG_FANQH_CHN14_HIGH    gpio_write(GPIO_CHN14, 1)
        #define DBG_FANQH_CHN14_TOGGLE  gpio_toggle(GPIO_CHN14)
    #else
        #define DBG_FANQH_CHN14_LOW
        #define DBG_FANQH_CHN14_HIGH
        #define DBG_FANQH_CHN14_TOGGLE
    #endif

    #ifdef  GPIO_CHN15
        #define DBG_FANQH_CHN15_LOW     gpio_write(GPIO_CHN15, 0)
        #define DBG_FANQH_CHN15_HIGH    gpio_write(GPIO_CHN15, 1)
        #define DBG_FANQH_CHN15_TOGGLE  gpio_toggle(GPIO_CHN15)
    #else
        #define DBG_FANQH_CHN15_LOW
        #define DBG_FANQH_CHN15_HIGH
        #define DBG_FANQH_CHN15_TOGGLE
    #endif
#else
    #define DBG_FANQH_CHN0_LOW
    #define DBG_FANQH_CHN0_HIGH
    #define DBG_FANQH_CHN0_TOGGLE
    #define DBG_FANQH_CHN1_LOW
    #define DBG_FANQH_CHN1_HIGH
    #define DBG_FANQH_CHN1_TOGGLE
    #define DBG_FANQH_CHN2_LOW
    #define DBG_FANQH_CHN2_HIGH
    #define DBG_FANQH_CHN2_TOGGLE
    #define DBG_FANQH_CHN3_LOW
    #define DBG_FANQH_CHN3_HIGH
    #define DBG_FANQH_CHN3_TOGGLE
    #define DBG_FANQH_CHN4_LOW
    #define DBG_FANQH_CHN4_HIGH
    #define DBG_FANQH_CHN4_TOGGLE
    #define DBG_FANQH_CHN5_LOW
    #define DBG_FANQH_CHN5_HIGH
    #define DBG_FANQH_CHN5_TOGGLE
    #define DBG_FANQH_CHN6_LOW
    #define DBG_FANQH_CHN6_HIGH
    #define DBG_FANQH_CHN6_TOGGLE
    #define DBG_FANQH_CHN7_LOW
    #define DBG_FANQH_CHN7_HIGH
    #define DBG_FANQH_CHN7_TOGGLE
    #define DBG_FANQH_CHN8_LOW
    #define DBG_FANQH_CHN8_HIGH
    #define DBG_FANQH_CHN8_TOGGLE
    #define DBG_FANQH_CHN9_LOW
    #define DBG_FANQH_CHN9_HIGH
    #define DBG_FANQH_CHN9_TOGGLE
    #define DBG_FANQH_CHN10_LOW
    #define DBG_FANQH_CHN10_HIGH
    #define DBG_FANQH_CHN10_TOGGLE
    #define DBG_FANQH_CHN11_LOW
    #define DBG_FANQH_CHN11_HIGH
    #define DBG_FANQH_CHN11_TOGGLE
    #define DBG_FANQH_CHN12_LOW
    #define DBG_FANQH_CHN12_HIGH
    #define DBG_FANQH_CHN12_TOGGLE
    #define DBG_FANQH_CHN13_LOW
    #define DBG_FANQH_CHN13_HIGH
    #define DBG_FANQH_CHN13_TOGGLE
    #define DBG_FANQH_CHN14_LOW
    #define DBG_FANQH_CHN14_HIGH
    #define DBG_FANQH_CHN14_TOGGLE
    #define DBG_FANQH_CHN15_LOW
    #define DBG_FANQH_CHN15_HIGH
    #define DBG_FANQH_CHN15_TOGGLE
#endif

#if (DEBUG_TIANXIANG_GPIO_ENABLE)
    #ifdef GPIO_CHN0
        #define DBG_TIANXIANG_CHN0_LOW      gpio_write(GPIO_CHN0, 0)
        #define DBG_TIANXIANG_CHN0_HIGH     gpio_write(GPIO_CHN0, 1)
        #define DBG_TIANXIANG_CHN0_TOGGLE   gpio_toggle(GPIO_CHN0)
    #else
        #define DBG_TIANXIANG_CHN0_LOW
        #define DBG_TIANXIANG_CHN0_HIGH
        #define DBG_TIANXIANG_CHN0_TOGGLE
    #endif

    #ifdef  GPIO_CHN1
        #define DBG_TIANXIANG_CHN1_LOW      gpio_write(GPIO_CHN1, 0)
        #define DBG_TIANXIANG_CHN1_HIGH     gpio_write(GPIO_CHN1, 1)
        #define DBG_TIANXIANG_CHN1_TOGGLE   gpio_toggle(GPIO_CHN1)
    #else
        #define DBG_TIANXIANG_CHN1_LOW
        #define DBG_TIANXIANG_CHN1_HIGH
        #define DBG_TIANXIANG_CHN1_TOGGLE
    #endif

    #ifdef  GPIO_CHN2
        #define DBG_TIANXIANG_CHN2_LOW      gpio_write(GPIO_CHN2, 0)
        #define DBG_TIANXIANG_CHN2_HIGH     gpio_write(GPIO_CHN2, 1)
        #define DBG_TIANXIANG_CHN2_TOGGLE   gpio_toggle(GPIO_CHN2)
    #else
        #define DBG_TIANXIANG_CHN2_LOW
        #define DBG_TIANXIANG_CHN2_HIGH
        #define DBG_TIANXIANG_CHN2_TOGGLE
    #endif

    #ifdef  GPIO_CHN3
        #define DBG_TIANXIANG_CHN3_LOW      gpio_write(GPIO_CHN3, 0)
        #define DBG_TIANXIANG_CHN3_HIGH     gpio_write(GPIO_CHN3, 1)
        #define DBG_TIANXIANG_CHN3_TOGGLE   gpio_toggle(GPIO_CHN3)
    #else
        #define DBG_TIANXIANG_CHN3_LOW
        #define DBG_TIANXIANG_CHN3_HIGH
        #define DBG_TIANXIANG_CHN3_TOGGLE
    #endif

    #ifdef GPIO_CHN4
        #define DBG_TIANXIANG_CHN4_LOW      gpio_write(GPIO_CHN4, 0)
        #define DBG_TIANXIANG_CHN4_HIGH     gpio_write(GPIO_CHN4, 1)
        #define DBG_TIANXIANG_CHN4_TOGGLE   gpio_toggle(GPIO_CHN4)
    #else
        #define DBG_TIANXIANG_CHN4_LOW
        #define DBG_TIANXIANG_CHN4_HIGH
        #define DBG_TIANXIANG_CHN4_TOGGLE
    #endif

    #ifdef  GPIO_CHN5
        #define DBG_TIANXIANG_CHN5_LOW      gpio_write(GPIO_CHN5, 0)
        #define DBG_TIANXIANG_CHN5_HIGH     gpio_write(GPIO_CHN5, 1)
        #define DBG_TIANXIANG_CHN5_TOGGLE   gpio_toggle(GPIO_CHN5)
    #else
        #define DBG_TIANXIANG_CHN5_LOW
        #define DBG_TIANXIANG_CHN5_HIGH
        #define DBG_TIANXIANG_CHN5_TOGGLE
    #endif

    #ifdef  GPIO_CHN6
        #define DBG_TIANXIANG_CHN6_LOW      gpio_write(GPIO_CHN6, 0)
        #define DBG_TIANXIANG_CHN6_HIGH     gpio_write(GPIO_CHN6, 1)
        #define DBG_TIANXIANG_CHN6_TOGGLE   gpio_toggle(GPIO_CHN6)
    #else
        #define DBG_TIANXIANG_CHN6_LOW
        #define DBG_TIANXIANG_CHN6_HIGH
        #define DBG_TIANXIANG_CHN6_TOGGLE
    #endif

    #ifdef  GPIO_CHN7
        #define DBG_TIANXIANG_CHN7_LOW      gpio_write(GPIO_CHN7, 0)
        #define DBG_TIANXIANG_CHN7_HIGH     gpio_write(GPIO_CHN7, 1)
        #define DBG_TIANXIANG_CHN7_TOGGLE   gpio_toggle(GPIO_CHN7)
    #else
        #define DBG_TIANXIANG_CHN7_LOW
        #define DBG_TIANXIANG_CHN7_HIGH
        #define DBG_TIANXIANG_CHN7_TOGGLE
    #endif

    #ifdef GPIO_CHN8
        #define DBG_TIANXIANG_CHN8_LOW      gpio_write(GPIO_CHN8, 0)
        #define DBG_TIANXIANG_CHN8_HIGH     gpio_write(GPIO_CHN8, 1)
        #define DBG_TIANXIANG_CHN8_TOGGLE   gpio_toggle(GPIO_CHN8)
    #else
        #define DBG_TIANXIANG_CHN8_LOW
        #define DBG_TIANXIANG_CHN8_HIGH
        #define DBG_TIANXIANG_CHN8_TOGGLE
    #endif

    #ifdef  GPIO_CHN9
        #define DBG_TIANXIANG_CHN9_LOW      gpio_write(GPIO_CHN9, 0)
        #define DBG_TIANXIANG_CHN9_HIGH     gpio_write(GPIO_CHN9, 1)
        #define DBG_TIANXIANG_CHN9_TOGGLE   gpio_toggle(GPIO_CHN9)
    #else
        #define DBG_TIANXIANG_CHN9_LOW
        #define DBG_TIANXIANG_CHN9_HIGH
        #define DBG_TIANXIANG_CHN9_TOGGLE
    #endif

    #ifdef  GPIO_CHN10
        #define DBG_TIANXIANG_CHN10_LOW     gpio_write(GPIO_CHN10, 0)
        #define DBG_TIANXIANG_CHN10_HIGH    gpio_write(GPIO_CHN10, 1)
        #define DBG_TIANXIANG_CHN10_TOGGLE  gpio_toggle(GPIO_CHN10)
    #else
        #define DBG_TIANXIANG_CHN10_LOW
        #define DBG_TIANXIANG_CHN10_HIGH
        #define DBG_TIANXIANG_CHN10_TOGGLE
    #endif

    #ifdef  GPIO_CHN11
        #define DBG_TIANXIANG_CHN11_LOW     gpio_write(GPIO_CHN11, 0)
        #define DBG_TIANXIANG_CHN11_HIGH    gpio_write(GPIO_CHN11, 1)
        #define DBG_TIANXIANG_CHN11_TOGGLE  gpio_toggle(GPIO_CHN11)
    #else
        #define DBG_TIANXIANG_CHN11_LOW
        #define DBG_TIANXIANG_CHN11_HIGH
        #define DBG_TIANXIANG_CHN11_TOGGLE
    #endif

    #ifdef GPIO_CHN12
        #define DBG_TIANXIANG_CHN12_LOW     gpio_write(GPIO_CHN12, 0)
        #define DBG_TIANXIANG_CHN12_HIGH    gpio_write(GPIO_CHN12, 1)
        #define DBG_TIANXIANG_CHN12_TOGGLE  gpio_toggle(GPIO_CHN12)
    #else
        #define DBG_TIANXIANG_CHN12_LOW
        #define DBG_TIANXIANG_CHN12_HIGH
        #define DBG_TIANXIANG_CHN12_TOGGLE
    #endif

    #ifdef  GPIO_CHN13
        #define DBG_TIANXIANG_CHN13_LOW     gpio_write(GPIO_CHN13, 0)
        #define DBG_TIANXIANG_CHN13_HIGH    gpio_write(GPIO_CHN13, 1)
        #define DBG_TIANXIANG_CHN13_TOGGLE  gpio_toggle(GPIO_CHN13)
    #else
        #define DBG_TIANXIANG_CHN13_LOW
        #define DBG_TIANXIANG_CHN13_HIGH
        #define DBG_TIANXIANG_CHN13_TOGGLE
    #endif

    #ifdef  GPIO_CHN14
        #define DBG_TIANXIANG_CHN14_LOW     gpio_write(GPIO_CHN14, 0)
        #define DBG_TIANXIANG_CHN14_HIGH    gpio_write(GPIO_CHN14, 1)
        #define DBG_TIANXIANG_CHN14_TOGGLE  gpio_toggle(GPIO_CHN14)
    #else
        #define DBG_TIANXIANG_CHN14_LOW
        #define DBG_TIANXIANG_CHN14_HIGH
        #define DBG_TIANXIANG_CHN14_TOGGLE
    #endif

    #ifdef  GPIO_CHN15
        #define DBG_TIANXIANG_CHN15_LOW     gpio_write(GPIO_CHN15, 0)
        #define DBG_TIANXIANG_CHN15_HIGH    gpio_write(GPIO_CHN15, 1)
        #define DBG_TIANXIANG_CHN15_TOGGLE  gpio_toggle(GPIO_CHN15)
    #else
        #define DBG_TIANXIANG_CHN15_LOW
        #define DBG_TIANXIANG_CHN15_HIGH
        #define DBG_TIANXIANG_CHN15_TOGGLE
    #endif
#else
    #define DBG_TIANXIANG_CHN0_LOW
    #define DBG_TIANXIANG_CHN0_HIGH
    #define DBG_TIANXIANG_CHN0_TOGGLE
    #define DBG_TIANXIANG_CHN1_LOW
    #define DBG_TIANXIANG_CHN1_HIGH
    #define DBG_TIANXIANG_CHN1_TOGGLE
    #define DBG_TIANXIANG_CHN2_LOW
    #define DBG_TIANXIANG_CHN2_HIGH
    #define DBG_TIANXIANG_CHN2_TOGGLE
    #define DBG_TIANXIANG_CHN3_LOW
    #define DBG_TIANXIANG_CHN3_HIGH
    #define DBG_TIANXIANG_CHN3_TOGGLE
    #define DBG_TIANXIANG_CHN4_LOW
    #define DBG_TIANXIANG_CHN4_HIGH
    #define DBG_TIANXIANG_CHN4_TOGGLE
    #define DBG_TIANXIANG_CHN5_LOW
    #define DBG_TIANXIANG_CHN5_HIGH
    #define DBG_TIANXIANG_CHN5_TOGGLE
    #define DBG_TIANXIANG_CHN6_LOW
    #define DBG_TIANXIANG_CHN6_HIGH
    #define DBG_TIANXIANG_CHN6_TOGGLE
    #define DBG_TIANXIANG_CHN7_LOW
    #define DBG_TIANXIANG_CHN7_HIGH
    #define DBG_TIANXIANG_CHN7_TOGGLE
    #define DBG_TIANXIANG_CHN8_LOW
    #define DBG_TIANXIANG_CHN8_HIGH
    #define DBG_TIANXIANG_CHN8_TOGGLE
    #define DBG_TIANXIANG_CHN9_LOW
    #define DBG_TIANXIANG_CHN9_HIGH
    #define DBG_TIANXIANG_CHN9_TOGGLE
    #define DBG_TIANXIANG_CHN10_LOW
    #define DBG_TIANXIANG_CHN10_HIGH
    #define DBG_TIANXIANG_CHN10_TOGGLE
    #define DBG_TIANXIANG_CHN11_LOW
    #define DBG_TIANXIANG_CHN11_HIGH
    #define DBG_TIANXIANG_CHN11_TOGGLE
    #define DBG_TIANXIANG_CHN12_LOW
    #define DBG_TIANXIANG_CHN12_HIGH
    #define DBG_TIANXIANG_CHN12_TOGGLE
    #define DBG_TIANXIANG_CHN13_LOW
    #define DBG_TIANXIANG_CHN13_HIGH
    #define DBG_TIANXIANG_CHN13_TOGGLE
    #define DBG_TIANXIANG_CHN14_LOW
    #define DBG_TIANXIANG_CHN14_HIGH
    #define DBG_TIANXIANG_CHN14_TOGGLE
    #define DBG_TIANXIANG_CHN15_LOW
    #define DBG_TIANXIANG_CHN15_HIGH
    #define DBG_TIANXIANG_CHN15_TOGGLE
#endif


#if (DEBUG_CS_GPIO_ENABLE)
    #ifdef GPIO_CHN0
        #define DBG_CS_CHN0_LOW     gpio_write(GPIO_CHN0, 0)
        #define DBG_CS_CHN0_HIGH        gpio_write(GPIO_CHN0, 1)
        #define DBG_CS_CHN0_TOGGLE  gpio_toggle(GPIO_CHN0)
    #else
        #define DBG_CS_CHN0_LOW
        #define DBG_CS_CHN0_HIGH
        #define DBG_CS_CHN0_TOGGLE
    #endif

    #ifdef  GPIO_CHN1
        #define DBG_CS_CHN1_LOW     gpio_write(GPIO_CHN1, 0)
        #define DBG_CS_CHN1_HIGH        gpio_write(GPIO_CHN1, 1)
        #define DBG_CS_CHN1_TOGGLE  gpio_toggle(GPIO_CHN1)
    #else
        #define DBG_CS_CHN1_LOW
        #define DBG_CS_CHN1_HIGH
        #define DBG_CS_CHN1_TOGGLE
    #endif

    #ifdef  GPIO_CHN2
        #define DBG_CS_CHN2_LOW     gpio_write(GPIO_CHN2, 0)
        #define DBG_CS_CHN2_HIGH        gpio_write(GPIO_CHN2, 1)
        #define DBG_CS_CHN2_TOGGLE  gpio_toggle(GPIO_CHN2)
    #else
        #define DBG_CS_CHN2_LOW
        #define DBG_CS_CHN2_HIGH
        #define DBG_CS_CHN2_TOGGLE
    #endif

    #ifdef  GPIO_CHN3
        #define DBG_CS_CHN3_LOW     gpio_write(GPIO_CHN3, 0)
        #define DBG_CS_CHN3_HIGH        gpio_write(GPIO_CHN3, 1)
        #define DBG_CS_CHN3_TOGGLE  gpio_toggle(GPIO_CHN3)
    #else
        #define DBG_CS_CHN3_LOW
        #define DBG_CS_CHN3_HIGH
        #define DBG_CS_CHN3_TOGGLE
    #endif

    #ifdef GPIO_CHN4
        #define DBG_CS_CHN4_LOW     gpio_write(GPIO_CHN4, 0)
        #define DBG_CS_CHN4_HIGH        gpio_write(GPIO_CHN4, 1)
        #define DBG_CS_CHN4_TOGGLE  gpio_toggle(GPIO_CHN4)
    #else
        #define DBG_CS_CHN4_LOW
        #define DBG_CS_CHN4_HIGH
        #define DBG_CS_CHN4_TOGGLE
    #endif

    #ifdef  GPIO_CHN5
        #define DBG_CS_CHN5_LOW     gpio_write(GPIO_CHN5, 0)
        #define DBG_CS_CHN5_HIGH        gpio_write(GPIO_CHN5, 1)
        #define DBG_CS_CHN5_TOGGLE  gpio_toggle(GPIO_CHN5)
    #else
        #define DBG_CS_CHN5_LOW
        #define DBG_CS_CHN5_HIGH
        #define DBG_CS_CHN5_TOGGLE
    #endif

    #ifdef  GPIO_CHN6
        #define DBG_CS_CHN6_LOW     gpio_write(GPIO_CHN6, 0)
        #define DBG_CS_CHN6_HIGH        gpio_write(GPIO_CHN6, 1)
        #define DBG_CS_CHN6_TOGGLE  gpio_toggle(GPIO_CHN6)
    #else
        #define DBG_CS_CHN6_LOW
        #define DBG_CS_CHN6_HIGH
        #define DBG_CS_CHN6_TOGGLE
    #endif

    #ifdef  GPIO_CHN7
        #define DBG_CS_CHN7_LOW     gpio_write(GPIO_CHN7, 0)
        #define DBG_CS_CHN7_HIGH        gpio_write(GPIO_CHN7, 1)
        #define DBG_CS_CHN7_TOGGLE  gpio_toggle(GPIO_CHN7)
    #else
        #define DBG_CS_CHN7_LOW
        #define DBG_CS_CHN7_HIGH
        #define DBG_CS_CHN7_TOGGLE
    #endif

    #ifdef GPIO_CHN8
        #define DBG_CS_CHN8_LOW     gpio_write(GPIO_CHN8, 0)
        #define DBG_CS_CHN8_HIGH        gpio_write(GPIO_CHN8, 1)
        #define DBG_CS_CHN8_TOGGLE  gpio_toggle(GPIO_CHN8)
    #else
        #define DBG_CS_CHN8_LOW
        #define DBG_CS_CHN8_HIGH
        #define DBG_CS_CHN8_TOGGLE
    #endif

    #ifdef  GPIO_CHN9
        #define DBG_CS_CHN9_LOW     gpio_write(GPIO_CHN9, 0)
        #define DBG_CS_CHN9_HIGH        gpio_write(GPIO_CHN9, 1)
        #define DBG_CS_CHN9_TOGGLE  gpio_toggle(GPIO_CHN9)
    #else
        #define DBG_CS_CHN9_LOW
        #define DBG_CS_CHN9_HIGH
        #define DBG_CS_CHN9_TOGGLE
    #endif

    #ifdef  GPIO_CHN10
        #define DBG_CS_CHN10_LOW        gpio_write(GPIO_CHN10, 0)
        #define DBG_CS_CHN10_HIGH   gpio_write(GPIO_CHN10, 1)
        #define DBG_CS_CHN10_TOGGLE gpio_toggle(GPIO_CHN10)
    #else
        #define DBG_CS_CHN10_LOW
        #define DBG_CS_CHN10_HIGH
        #define DBG_CS_CHN10_TOGGLE
    #endif

    #ifdef  GPIO_CHN11
        #define DBG_CS_CHN11_LOW        gpio_write(GPIO_CHN11, 0)
        #define DBG_CS_CHN11_HIGH   gpio_write(GPIO_CHN11, 1)
        #define DBG_CS_CHN11_TOGGLE gpio_toggle(GPIO_CHN11)
    #else
        #define DBG_CS_CHN11_LOW
        #define DBG_CS_CHN11_HIGH
        #define DBG_CS_CHN11_TOGGLE
    #endif

    #ifdef GPIO_CHN12
        #define DBG_CS_CHN12_LOW        gpio_write(GPIO_CHN12, 0)
        #define DBG_CS_CHN12_HIGH   gpio_write(GPIO_CHN12, 1)
        #define DBG_CS_CHN12_TOGGLE gpio_toggle(GPIO_CHN12)
    #else
        #define DBG_CS_CHN12_LOW
        #define DBG_CS_CHN12_HIGH
        #define DBG_CS_CHN12_TOGGLE
    #endif

    #ifdef  GPIO_CHN13
        #define DBG_CS_CHN13_LOW        gpio_write(GPIO_CHN13, 0)
        #define DBG_CS_CHN13_HIGH   gpio_write(GPIO_CHN13, 1)
        #define DBG_CS_CHN13_TOGGLE gpio_toggle(GPIO_CHN13)
    #else
        #define DBG_CS_CHN13_LOW
        #define DBG_CS_CHN13_HIGH
        #define DBG_CS_CHN13_TOGGLE
    #endif

    #ifdef  GPIO_CHN14
        #define DBG_CS_CHN14_LOW        gpio_write(GPIO_CHN14, 0)
        #define DBG_CS_CHN14_HIGH   gpio_write(GPIO_CHN14, 1)
        #define DBG_CS_CHN14_TOGGLE gpio_toggle(GPIO_CHN14)
    #else
        #define DBG_CS_CHN14_LOW
        #define DBG_CS_CHN14_HIGH
        #define DBG_CS_CHN14_TOGGLE
    #endif

    #ifdef  GPIO_CHN15
        #define DBG_CS_CHN15_LOW        gpio_write(GPIO_CHN15, 0)
        #define DBG_CS_CHN15_HIGH   gpio_write(GPIO_CHN15, 1)
        #define DBG_CS_CHN15_TOGGLE gpio_toggle(GPIO_CHN15)
    #else
        #define DBG_CS_CHN15_LOW
        #define DBG_CS_CHN15_HIGH
        #define DBG_CS_CHN15_TOGGLE
    #endif
#else
    #define DBG_CS_CHN0_LOW
    #define DBG_CS_CHN0_HIGH
    #define DBG_CS_CHN0_TOGGLE
    #define DBG_CS_CHN1_LOW
    #define DBG_CS_CHN1_HIGH
    #define DBG_CS_CHN1_TOGGLE
    #define DBG_CS_CHN2_LOW
    #define DBG_CS_CHN2_HIGH
    #define DBG_CS_CHN2_TOGGLE
    #define DBG_CS_CHN3_LOW
    #define DBG_CS_CHN3_HIGH
    #define DBG_CS_CHN3_TOGGLE
    #define DBG_CS_CHN4_LOW
    #define DBG_CS_CHN4_HIGH
    #define DBG_CS_CHN4_TOGGLE
    #define DBG_CS_CHN5_LOW
    #define DBG_CS_CHN5_HIGH
    #define DBG_CS_CHN5_TOGGLE
    #define DBG_CS_CHN6_LOW
    #define DBG_CS_CHN6_HIGH
    #define DBG_CS_CHN6_TOGGLE
    #define DBG_CS_CHN7_LOW
    #define DBG_CS_CHN7_HIGH
    #define DBG_CS_CHN7_TOGGLE
    #define DBG_CS_CHN8_LOW
    #define DBG_CS_CHN8_HIGH
    #define DBG_CS_CHN8_TOGGLE
    #define DBG_CS_CHN9_LOW
    #define DBG_CS_CHN9_HIGH
    #define DBG_CS_CHN9_TOGGLE
    #define DBG_CS_CHN10_LOW
    #define DBG_CS_CHN10_HIGH
    #define DBG_CS_CHN10_TOGGLE
    #define DBG_CS_CHN11_LOW
    #define DBG_CS_CHN11_HIGH
    #define DBG_CS_CHN11_TOGGLE
    #define DBG_CS_CHN12_LOW
    #define DBG_CS_CHN12_HIGH
    #define DBG_CS_CHN12_TOGGLE
    #define DBG_CS_CHN13_LOW
    #define DBG_CS_CHN13_HIGH
    #define DBG_CS_CHN13_TOGGLE
    #define DBG_CS_CHN14_LOW
    #define DBG_CS_CHN14_HIGH
    #define DBG_CS_CHN14_TOGGLE
    #define DBG_CS_CHN15_LOW
    #define DBG_CS_CHN15_HIGH
    #define DBG_CS_CHN15_TOGGLE
#endif



#endif /* STACK_BLE_DEBUG_GPIO_PRIVATE_H_ */
