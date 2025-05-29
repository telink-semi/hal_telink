/********************************************************************************************************
 * @file    ble_host_sal.h
 *
 * @brief   This is the header file for TLSR/TL
 *
 * @author  Bluetooth Group
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
#pragma once

// #ifdef __cplusplus
// extern "C" {
// #endif

/*************** list, queue, stack, or ohter data structure ***********/
// ble all data structure use sys/queue.h defined in kernel.
// #include <sys/queue.h>

/*************** platform time interface ***********/
/**
 *   @brief BLE host platform time initialization.
 *
 *   @return none.
 */
void ble_host_sal_platform_time_init(void);

/**
 *   @brief BLE host get current time.
 *
 *   @return current time.
 */
uint32_t ble_host_sal_get_current_time(void);

/**
 *   @brief BLE host time exceed.
 *
 *   @param[in]  last_time   last time.
 *   @param[in]  time_ms     time in millisecond.
 *
 *   @return true if time exceed, false if not.
 */
bool ble_host_sal_is_time_exceed(uint32_t last_time, uint32_t time_ms);

/*************** timer sal interface ***********/
/**
 *   @brief BLE host timer callback function type_id.
 *
 *   @param[in]  arg         timer callback function argument.
 *
 *   @return none.
 */
typedef void (*ble_host_sal_timer_cb_t)(void *arg);

/**
 *   @brief BLE host timer initialization.
 *
 *   @return none.
 */
void ble_host_sal_timer_init(void);

/**
 *   @brief BLE host timer create a timer.
 *
 *   @param[in]  cb          timer callback function.
 *   @param[in]  arg         timer callback function argument.
 *   @param[in]  timeout_ms  timer timeout in millisecond.
 *
 *   @return timer handle.
 */
void *ble_host_sal_timer_create(ble_host_sal_timer_cb_t cb, void *arg, uint32_t timeout_ms);

/**
 *   @brief ble host timer update timeout value.
 *
 *   @param[in]  timer_hdl   timer handle.
 *   @param[in]  timeout_ms  timer timeout in millisecond.
 *
 *   @return true if success, false if failed.
 */
bool ble_host_sal_timer_update_timeout(void *timer_hdl, uint32_t timeout_ms);

/**
 *   @brief ble host timer start a timer.
 *
 *   @param[in]  timer_hdl   timer handle.
 *
 *   @return true if success, false if failed.
 */
bool ble_host_sal_timer_start(void *timer_hdl);

/**
 *   @brief ble host timer stop a timer.
 *
 *   @param[in]  timer_hdl   timer handle.
 *
 *   @return true if success, false if failed.
 */
bool ble_host_sal_timer_stop(void *timer_hdl);

/**
 *   @brief ble host timer delete a timer.
 *
 *   @param[in]  timer_hdl   timer handle.
 *
 *   @return true if success, false if failed.
 */
bool ble_host_sal_timer_delete(void *timer_hdl);

/**************** memory pool(malloc/free) sal interface ***********/
/**
 *   @brief BLE host memory pool initialization.
 *
 *   @param[in]  memory_addr  memory start address.
 *   @param[in]  size         memory size.
 *
 *   @return none.
 */
void ble_host_sal_memory_pool_init(void *memory_addr, uint32_t size);

/**
 *   @brief BLE host memory pool deinitialization.
 *
 *   @param[in]  memory_addr  memory start address.
 *
 *   @return none.
 */
void ble_host_sal_memory_pool_deinit(void *memory_addr);

/**
 *   @brief BLE host memory malloc.
 *
 *   @param[in]  memory_addr  memory start address.
 *   @param[in]  size        malloc size.
 *   @param[in]  type_id     high layer type id(range 256-65535).
 *
 *   @return malloc pointer.
 */
void *ble_host_sal_memory_malloc(void *memory_addr, uint32_t size, uint16_t type_id);

/**
 *   @brief BLE host memory free.
 *
 *   @param[in]  memory_addr  memory start address.
 *   @param[in]  ptr          malloc pointer.
 *
 *   @return none.
 */
void ble_host_sal_memory_free(void *memory_addr, void *ptr);

/****************** log sal interface ****************/
enum ble_host_log_level {
    BLE_HOST_LOG_LEVEL_NONE = 0,
    BLE_HOST_LOG_LEVEL_ERROR,
    BLE_HOST_LOG_LEVEL_WARN,
    BLE_HOST_LOG_LEVEL_INFO,
    BLE_HOST_LOG_LEVEL_DEBUG,
};

/**
 *   @brief BLE host log initialization.
 *
 *   @return none.
 */
void ble_host_sal_log_init(enum ble_host_log_level level);

/**
 *   @brief BLE host log deinitialization.
 *
 *   @return none.
 */
void ble_host_sal_log_deinit(void);

/**
 *   @brief BLE host log output.
 *
 *   @param[in]  level   log level.
 *   @param[in]  fmt     log format string.
 *
 *   @return none.
 */
void ble_host_sal_log_output(enum ble_host_log_level level, const char *fmt, ...);

/**
 *   @brief BLE host log set output level.
 *
 *   @param[in]  level   log level.
 *
 *   @return none.
 */
void ble_host_sal_log_set_output_level(enum ble_host_log_level level);

#define BLE_HOST_SAL_LOG_ERROR(fmt, ...) ble_host_sal_log_output(BLE_HOST_LOG_LEVEL_ERROR, "[E]" fmt, ##__VA_ARGS__)
#define BLE_HOST_SAL_LOG_WARN(fmt, ...)  ble_host_sal_log_output(BLE_HOST_LOG_LEVEL_WARN, "[W]" fmt, ##__VA_ARGS__)
#define BLE_HOST_SAL_LOG_INFO(fmt, ...)  ble_host_sal_log_output(BLE_HOST_LOG_LEVEL_INFO, "[I]" fmt, ##__VA_ARGS__)
#define BLE_HOST_SAL_LOG_DEBUG(fmt, ...) ble_host_sal_log_output(BLE_HOST_LOG_LEVEL_DEBUG, "[D]" fmt, ##__VA_ARGS__)

/****************** hci sal interface ****************/
typedef void (*ble_host_recv_hci_data_cb_t)(const uint8_t *data, uint16_t len);

/**
 *   @brief BLE host hci initialization.
 *
 *   @return none.
 */
void ble_host_sal_hci_init(void);

/**
 *   @brief BLE host hci send value to controller.
 *
 *   @param[in]  data    hci data pointer.
 *   @param[in]  len     hci data length.
 *
 *   @return none.
 */
void ble_host_sal_hci_send_packet(const uint8_t *data, uint16_t len);

/**
 *   @brief BLE host hci receive value from controller.
 *
 *   @note It called by Application Layer Controller Driver.
 */
// void ble_host_hci_rx_packet(uint8_t *data, unsigned int len);

/********************* assert interface ****************/

#define BLE_HOST_SAL_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            BLE_HOST_SAL_LOG_ERROR("assert failed: %s:%d: %s", __FILE__, __LINE__, #expr); \
            while (1) { \
                ; \
            } \
        } \
    } while (0)

/********************* RTOS or mainloop register interface ****************/

// #ifdef __cplusplus
// }
// #endif

