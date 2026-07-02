#ifndef __DEBUG_GPIO_H__
#define __DEBUG_GPIO_H__

#ifndef DEBUG_OT_BLE_GPIO_ENABLE
#define DEBUG_OT_BLE_GPIO_ENABLE 1
#endif

#if (DEBUG_OT_BLE_GPIO_ENABLE)
    #define GPIO_CHN0         GPIO_PE0
    #define GPIO_CHN1         GPIO_PE1
    #define GPIO_CHN2         GPIO_PE2
    #define GPIO_CHN3         GPIO_PE3
    #define GPIO_CHN4         GPIO_PE4
    #define GPIO_CHN5         GPIO_PE5
    #define GPIO_CHN6         GPIO_PE6
    #define GPIO_CHN7         GPIO_PF3

    #define GPIO_CHN8         GPIO_PF4
    #define GPIO_CHN9         GPIO_PF6
    #define GPIO_CHN10        GPIO_PF7
    #define GPIO_CHN11        GPIO_PA0
    #define GPIO_CHN12        GPIO_PA1
    #define GPIO_CHN13        GPIO_PE7
    #define GPIO_CHN14        GPIO_PF0
    #define GPIO_CHN15        GPIO_PF1


    #define PE0_OUTPUT_ENABLE 1
    #define PE1_OUTPUT_ENABLE 1
    #define PE2_OUTPUT_ENABLE 1
    #define PE3_OUTPUT_ENABLE 1
    #define PE4_OUTPUT_ENABLE 1
    #define PE5_OUTPUT_ENABLE 1
    #define PE6_OUTPUT_ENABLE 1
    #define PF3_OUTPUT_ENABLE 1

    #define PF4_OUTPUT_ENABLE 1
    #define PF6_OUTPUT_ENABLE 1
    #define PF7_OUTPUT_ENABLE 1
    #define PA0_OUTPUT_ENABLE 1
    #define PA1_OUTPUT_ENABLE 1
    #define PE7_OUTPUT_ENABLE 1
    #define PF0_OUTPUT_ENABLE 1
    #define PF1_OUTPUT_ENABLE 1
#endif

#if (DEBUG_OT_BLE_GPIO_ENABLE)
    #ifdef GPIO_CHN0
        #define DBG_OT_BLE_CHN0_LOW    gpio_write(GPIO_CHN0, 0)
        #define DBG_OT_BLE_CHN0_HIGH   gpio_write(GPIO_CHN0, 1)
        #define DBG_OT_BLE_CHN0_TOGGLE gpio_toggle(GPIO_CHN0)
    #else
        #define DBG_OT_BLE_CHN0_LOW
        #define DBG_OT_BLE_CHN0_HIGH
        #define DBG_OT_BLE_CHN0_TOGGLE
    #endif

    #ifdef GPIO_CHN1
        #define DBG_OT_BLE_CHN1_LOW    gpio_write(GPIO_CHN1, 0)
        #define DBG_OT_BLE_CHN1_HIGH   gpio_write(GPIO_CHN1, 1)
        #define DBG_OT_BLE_CHN1_TOGGLE gpio_toggle(GPIO_CHN1)
    #else
        #define DBG_OT_BLE_CHN1_LOW
        #define DBG_OT_BLE_CHN1_HIGH
        #define DBG_OT_BLE_CHN1_TOGGLE
    #endif

    #ifdef GPIO_CHN2
        #define DBG_OT_BLE_CHN2_LOW    gpio_write(GPIO_CHN2, 0)
        #define DBG_OT_BLE_CHN2_HIGH   gpio_write(GPIO_CHN2, 1)
        #define DBG_OT_BLE_CHN2_TOGGLE gpio_toggle(GPIO_CHN2)
    #else
        #define DBG_OT_BLE_CHN2_LOW
        #define DBG_OT_BLE_CHN2_HIGH
        #define DBG_OT_BLE_CHN2_TOGGLE
    #endif

    #ifdef GPIO_CHN3
        #define DBG_OT_BLE_CHN3_LOW    gpio_write(GPIO_CHN3, 0)
        #define DBG_OT_BLE_CHN3_HIGH   gpio_write(GPIO_CHN3, 1)
        #define DBG_OT_BLE_CHN3_TOGGLE gpio_toggle(GPIO_CHN3)
    #else
        #define DBG_OT_BLE_CHN3_LOW
        #define DBG_OT_BLE_CHN3_HIGH
        #define DBG_OT_BLE_CHN3_TOGGLE
    #endif

    #ifdef GPIO_CHN4
        #define DBG_OT_BLE_CHN4_LOW    gpio_write(GPIO_CHN4, 0)
        #define DBG_OT_BLE_CHN4_HIGH   gpio_write(GPIO_CHN4, 1)
        #define DBG_OT_BLE_CHN4_TOGGLE gpio_toggle(GPIO_CHN4)
    #else
        #define DBG_OT_BLE_CHN4_LOW
        #define DBG_OT_BLE_CHN4_HIGH
        #define DBG_OT_BLE_CHN4_TOGGLE
    #endif

    #ifdef GPIO_CHN5
        #define DBG_OT_BLE_CHN5_LOW    gpio_write(GPIO_CHN5, 0)
        #define DBG_OT_BLE_CHN5_HIGH   gpio_write(GPIO_CHN5, 1)
        #define DBG_OT_BLE_CHN5_TOGGLE gpio_toggle(GPIO_CHN5)
    #else
        #define DBG_OT_BLE_CHN5_LOW
        #define DBG_OT_BLE_CHN5_HIGH
        #define DBG_OT_BLE_CHN5_TOGGLE
    #endif

    #ifdef GPIO_CHN6
        #define DBG_OT_BLE_CHN6_LOW    gpio_write(GPIO_CHN6, 0)
        #define DBG_OT_BLE_CHN6_HIGH   gpio_write(GPIO_CHN6, 1)
        #define DBG_OT_BLE_CHN6_TOGGLE gpio_toggle(GPIO_CHN6)
    #else
        #define DBG_OT_BLE_CHN6_LOW
        #define DBG_OT_BLE_CHN6_HIGH
        #define DBG_OT_BLE_CHN6_TOGGLE
    #endif

    #ifdef GPIO_CHN7
        #define DBG_OT_BLE_CHN7_LOW    gpio_write(GPIO_CHN7, 0)
        #define DBG_OT_BLE_CHN7_HIGH   gpio_write(GPIO_CHN7, 1)
        #define DBG_OT_BLE_CHN7_TOGGLE gpio_toggle(GPIO_CHN7)
    #else
        #define DBG_OT_BLE_CHN7_LOW
        #define DBG_OT_BLE_CHN7_HIGH
        #define DBG_OT_BLE_CHN7_TOGGLE
    #endif

    #ifdef GPIO_CHN8
        #define DBG_OT_BLE_CHN8_LOW    gpio_write(GPIO_CHN8, 0)
        #define DBG_OT_BLE_CHN8_HIGH   gpio_write(GPIO_CHN8, 1)
        #define DBG_OT_BLE_CHN8_TOGGLE gpio_toggle(GPIO_CHN8)
    #else
        #define DBG_OT_BLE_CHN8_LOW
        #define DBG_OT_BLE_CHN8_HIGH
        #define DBG_OT_BLE_CHN8_TOGGLE
    #endif

    #ifdef GPIO_CHN9
        #define DBG_OT_BLE_CHN9_LOW    gpio_write(GPIO_CHN9, 0)
        #define DBG_OT_BLE_CHN9_HIGH   gpio_write(GPIO_CHN9, 1)
        #define DBG_OT_BLE_CHN9_TOGGLE gpio_toggle(GPIO_CHN9)
    #else
        #define DBG_OT_BLE_CHN9_LOW
        #define DBG_OT_BLE_CHN9_HIGH
        #define DBG_OT_BLE_CHN9_TOGGLE
    #endif

    #ifdef GPIO_CHN10
        #define DBG_OT_BLE_CHN10_LOW    gpio_write(GPIO_CHN10, 0)
        #define DBG_OT_BLE_CHN10_HIGH   gpio_write(GPIO_CHN10, 1)
        #define DBG_OT_BLE_CHN10_TOGGLE gpio_toggle(GPIO_CHN10)
    #else
        #define DBG_OT_BLE_CHN10_LOW
        #define DBG_OT_BLE_CHN10_HIGH
        #define DBG_OT_BLE_CHN10_TOGGLE
    #endif

    #ifdef GPIO_CHN11
        #define DBG_OT_BLE_CHN11_LOW    gpio_write(GPIO_CHN11, 0)
        #define DBG_OT_BLE_CHN11_HIGH   gpio_write(GPIO_CHN11, 1)
        #define DBG_OT_BLE_CHN11_TOGGLE gpio_toggle(GPIO_CHN11)
    #else
        #define DBG_OT_BLE_CHN11_LOW
        #define DBG_OT_BLE_CHN11_HIGH
        #define DBG_OT_BLE_CHN11_TOGGLE
    #endif

    #ifdef GPIO_CHN12
        #define DBG_OT_BLE_CHN12_LOW    gpio_write(GPIO_CHN12, 0)
        #define DBG_OT_BLE_CHN12_HIGH   gpio_write(GPIO_CHN12, 1)
        #define DBG_OT_BLE_CHN12_TOGGLE gpio_toggle(GPIO_CHN12)
    #else
        #define DBG_OT_BLE_CHN12_LOW
        #define DBG_OT_BLE_CHN12_HIGH
        #define DBG_OT_BLE_CHN12_TOGGLE
    #endif

    #ifdef GPIO_CHN13
        #define DBG_OT_BLE_CHN13_LOW    gpio_write(GPIO_CHN13, 0)
        #define DBG_OT_BLE_CHN13_HIGH   gpio_write(GPIO_CHN13, 1)
        #define DBG_OT_BLE_CHN13_TOGGLE gpio_toggle(GPIO_CHN13)
    #else
        #define DBG_OT_BLE_CHN13_LOW
        #define DBG_OT_BLE_CHN13_HIGH
        #define DBG_OT_BLE_CHN13_TOGGLE
    #endif

    #ifdef GPIO_CHN14
        #define DBG_OT_BLE_CHN14_LOW    gpio_write(GPIO_CHN14, 0)
        #define DBG_OT_BLE_CHN14_HIGH   gpio_write(GPIO_CHN14, 1)
        #define DBG_OT_BLE_CHN14_TOGGLE gpio_toggle(GPIO_CHN14)
    #else
        #define DBG_OT_BLE_CHN14_LOW
        #define DBG_OT_BLE_CHN14_HIGH
        #define DBG_OT_BLE_CHN14_TOGGLE
    #endif

    #ifdef GPIO_CHN15
        #define DBG_OT_BLE_CHN15_LOW    gpio_write(GPIO_CHN15, 0)
        #define DBG_OT_BLE_CHN15_HIGH   gpio_write(GPIO_CHN15, 1)
        #define DBG_OT_BLE_CHN15_TOGGLE gpio_toggle(GPIO_CHN15)
    #else
        #define DBG_OT_BLE_CHN15_LOW
        #define DBG_OT_BLE_CHN15_HIGH
        #define DBG_OT_BLE_CHN15_TOGGLE
    #endif
#else
    #define DBG_OT_BLE_CHN0_LOW
    #define DBG_OT_BLE_CHN0_HIGH
    #define DBG_OT_BLE_CHN0_TOGGLE
    #define DBG_OT_BLE_CHN1_LOW
    #define DBG_OT_BLE_CHN1_HIGH
    #define DBG_OT_BLE_CHN1_TOGGLE
    #define DBG_OT_BLE_CHN2_LOW
    #define DBG_OT_BLE_CHN2_HIGH
    #define DBG_OT_BLE_CHN2_TOGGLE
    #define DBG_OT_BLE_CHN3_LOW
    #define DBG_OT_BLE_CHN3_HIGH
    #define DBG_OT_BLE_CHN3_TOGGLE
    #define DBG_OT_BLE_CHN4_LOW
    #define DBG_OT_BLE_CHN4_HIGH
    #define DBG_OT_BLE_CHN4_TOGGLE
    #define DBG_OT_BLE_CHN5_LOW
    #define DBG_OT_BLE_CHN5_HIGH
    #define DBG_OT_BLE_CHN5_TOGGLE
    #define DBG_OT_BLE_CHN6_LOW
    #define DBG_OT_BLE_CHN6_HIGH
    #define DBG_OT_BLE_CHN6_TOGGLE
    #define DBG_OT_BLE_CHN7_LOW
    #define DBG_OT_BLE_CHN7_HIGH
    #define DBG_OT_BLE_CHN7_TOGGLE
    #define DBG_OT_BLE_CHN8_LOW
    #define DBG_OT_BLE_CHN8_HIGH
    #define DBG_OT_BLE_CHN8_TOGGLE
    #define DBG_OT_BLE_CHN9_LOW
    #define DBG_OT_BLE_CHN9_HIGH
    #define DBG_OT_BLE_CHN9_TOGGLE
    #define DBG_OT_BLE_CHN10_LOW
    #define DBG_OT_BLE_CHN10_HIGH
    #define DBG_OT_BLE_CHN10_TOGGLE
    #define DBG_OT_BLE_CHN11_LOW
    #define DBG_OT_BLE_CHN11_HIGH
    #define DBG_OT_BLE_CHN11_TOGGLE
    #define DBG_OT_BLE_CHN12_LOW
    #define DBG_OT_BLE_CHN12_HIGH
    #define DBG_OT_BLE_CHN12_TOGGLE
    #define DBG_OT_BLE_CHN13_LOW
    #define DBG_OT_BLE_CHN13_HIGH
    #define DBG_OT_BLE_CHN13_TOGGLE
    #define DBG_OT_BLE_CHN14_LOW
    #define DBG_OT_BLE_CHN14_HIGH
    #define DBG_OT_BLE_CHN14_TOGGLE
    #define DBG_OT_BLE_CHN15_LOW
    #define DBG_OT_BLE_CHN15_HIGH
    #define DBG_OT_BLE_CHN15_TOGGLE
#endif

#endif