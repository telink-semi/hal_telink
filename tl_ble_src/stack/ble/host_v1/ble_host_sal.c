
#include "drivers.h"

#include "common/types.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

//#include "tlklib/dbg/tlkdbg.h"

#include "inc/ble_host_sal.h"

#include "stack/ble/host/gatt/tlk_malloc_stack.h"
#include "stack/ble/host/gatt/tlk_timer_stack.h"

/** ble host sal  */

/*************** list, queue, stack, or ohter data structure ***********/
// ble all data structure use sys/queue.h defined in kernel.
// #include <sys/queue.h>

/*************** platform time interface ***********/
/**
 *   @brief BLE host platform time initialization.
 *
 *   @return none.
 */
__attribute__((weak))
void ble_host_sal_platform_time_init(void)
{
}

/**
 *   @brief BLE host get current time.
 *
 *   @return current time.
 */
__attribute__((weak))
uint32_t ble_host_sal_get_current_time(void)
{
    return clock_time();
}

/**
 *   @brief BLE host time exceed.
 *
 *   @param[in]  last_time   last time.
 *   @param[in]  time_ms     time in millisecond.
 *
 *   @return true if time exceed, false if not.
 */
__attribute__((weak))
bool ble_host_sal_is_time_exceed(uint32_t last_time, uint32_t time_ms)
{
    return clock_time_exceed(last_time, time_ms * 1000);
}


/*************** timer sal interface ***********/

#define BLE_HOST_MAX_TIMER_NUM   16

struct ble_host_sal_timer_env {
    bool start;
    ble_host_sal_timer_cb_t cb;
    void *arg;
    uint32_t timeout_ms;
    struct soft_timer timer;
};

static struct ble_host_sal_timer_env s_timer_env[BLE_HOST_MAX_TIMER_NUM];

static int ble_host_sal_soft_timer_callback(void *arg)
{
    struct ble_host_sal_timer_env *timer_env = (struct ble_host_sal_timer_env *) arg;
    timer_env->start = false;
    if (timer_env->cb) {
        timer_env->cb(timer_env->arg);
    }
    return 0;
}

/**
 *   @brief BLE host timer initialization.
 *
 *   @return none.
 */
__attribute__((weak))
void ble_host_sal_timer_init(void)
{
    soft_timer_initial();
}

/**
 *   @brief BLE host timer create a timer.
 *
 *   @param[in]  cb          timer callback function.
 *   @param[in]  arg         timer callback function argument.
 *   @param[in]  timeout_ms  timer timeout in millisecond.
 *
 *   @return timer handle.
 */
__attribute__((weak))
void *ble_host_sal_timer_create(ble_host_sal_timer_cb_t cb, void *arg, uint32_t timeout_ms)
{
    for (int i = 0; i < BLE_HOST_MAX_TIMER_NUM; i++) {
        if (s_timer_env[i].cb == cb) {
            return &s_timer_env[i];
        }
    }

    for (int i = 0; i < BLE_HOST_MAX_TIMER_NUM; i++) {
        if (s_timer_env[i].cb == NULL) {
            s_timer_env[i].start = false;
            s_timer_env[i].cb = cb;
            s_timer_env[i].arg = arg;
            s_timer_env[i].timeout_ms = timeout_ms;
            return &s_timer_env[i];
        }
    }
    return NULL;
}

/**
 *   @brief ble host timer update timeout value.
 *
 *   @param[in]  timer_hdl   timer handle.
 *   @param[in]  timeout_ms  timer timeout in millisecond.
 *
 *   @return true if success, false if failed.
 */
__attribute__((weak))
bool ble_host_sal_timer_update_timeout(void *timer_hdl, uint32_t timeout_ms)
{
    struct ble_host_sal_timer_env *timer_env = (struct ble_host_sal_timer_env *) timer_hdl;
    if (timer_env == NULL) {
        return false;
    }

    if (timer_env->start == true) {
        return false;
    }

    timer_env->timeout_ms = timeout_ms;
    return true;
}

/**
 *   @brief ble host timer start a timer.
 *
 *   @param[in]  timer_hdl   timer handle.
 *
 *   @return true if success, false if failed.
 */
__attribute__((weak))
bool ble_host_sal_timer_start(void *timer_hdl)
{
    struct ble_host_sal_timer_env *timer_env = (struct ble_host_sal_timer_env *) timer_hdl;
    if (timer_env == NULL) {
        return false;
    }

    if (timer_env->start == true) {
        return false;
    }

    timer_env->timer.timer = timer_env->timeout_ms;
    timer_env->timer.arg = timer_env;
    timer_env->timer.cb = ble_host_sal_soft_timer_callback;
    soft_timer_add(&timer_env->timer);
    timer_env->start = true;
    return true;
}

/**
 *   @brief ble host timer stop a timer.
 *
 *   @param[in]  timer_hdl   timer handle.
 *
 *   @return true if success, false if failed.
 */
__attribute__((weak))
bool ble_host_sal_timer_stop(void *timer_hdl)
{
    struct ble_host_sal_timer_env *timer_env = (struct ble_host_sal_timer_env *) timer_hdl;
    if (timer_env == NULL) {
        return false;
    }

    if (timer_env->start == false) {
        return false;
    }

    soft_timer_delete(&timer_env->timer);
    timer_env->start = false;
    return true;
}

/**
 *   @brief ble host timer delete a timer.
 *
 *   @param[in]  timer_hdl   timer handle.
 *
 *   @return true if success, false if failed.
 */
__attribute__((weak))
bool ble_host_sal_timer_delete(void *timer_hdl)
{
    struct ble_host_sal_timer_env *timer_env = (struct ble_host_sal_timer_env *) timer_hdl;
    if (timer_env == NULL) {
        return false;
    }

    if (timer_env->start == true) {
        ble_host_sal_timer_stop(timer_hdl);
    }
    timer_env->cb = NULL;

    return true;
}

/**************** memory pool(malloc/free) sal interface ***********/
/**
 *   @brief BLE host memory pool initialization.
 *
 *   @param[in]  memory_addr  memory start address.
 *   @param[in]  size         memory size.
 *
 *   @return none.
 */
__attribute__((weak))
void ble_host_sal_memory_pool_init(void *memory_addr, uint32_t size)
{
    tlk_malloc_init(memory_addr, size);
}

/**
 *   @brief BLE host memory pool deinitialization.
 *
 *   @param[in]  memory_addr  memory start address.
 *
 *   @return none.
 */
__attribute__((weak))
void ble_host_sal_memory_pool_deinit(void *memory_addr)
{
    tlk_malloc_deinit(memory_addr);
}

/**
 *   @brief BLE host memory malloc.
 *
 *   @param[in]  memory_addr  memory start address.
 *   @param[in]  size        malloc size.
 *   @param[in]  type_id     high layer type id(range 256-65535).
 *
 *   @return malloc pointer.
 */
__attribute__((weak))
void *ble_host_sal_memory_malloc(void *memory_addr, uint32_t size, uint16_t type_id)
{
    return tlk_malloc_buffer(memory_addr, size, type_id);
}

/**
 *   @brief BLE host memory free.
 *
 *   @param[in]  memory_addr  memory start address.
 *   @param[in]  ptr          malloc pointer.
 *
 *   @return none.
 */
__attribute__((weak))
void ble_host_sal_memory_free(void *memory_addr, void *ptr)
{
    tlk_free_buffer(memory_addr, ptr);
}

/****************** log sal interface ****************/
/**
 *   @brief BLE host log initialization.
 *
 *   @return none.
 */
__attribute__((weak))
void ble_host_sal_log_init(enum ble_host_log_level level)
{
    (void) level;
}

/**
 *   @brief BLE host log deinitialization.
 *
 *   @return none.
 */
__attribute__((weak))
void ble_host_sal_log_deinit(void)
{
}

static uint32_t s_log_level = BLE_HOST_LOG_LEVEL_WARN;

/**
 *   @brief BLE host log output.
 *
 *   @param[in]  level   log level.
 *   @param[in]  fmt     log format string.
 *
 *   @return none.
 */
__attribute__((weak))
void ble_host_sal_log_output(enum ble_host_log_level level, const char *fmt, ...)
{
    if (level <= s_log_level) {
        char    log_out_buffer[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(log_out_buffer, sizeof(log_out_buffer) - 1, fmt, args);
        tlk_printf(log_out_buffer);
        va_end(args);
    }
}

/**
 *   @brief BLE host log set output level.
 *
 *   @param[in]  level   log level.
 *
 *   @return none.
 */
__attribute__((weak))
void ble_host_sal_log_set_output_level(enum ble_host_log_level level)
{
    (void) level;
}

/****************** hci sal interface ****************/
/**
 *   @brief BLE host hci initialization.
 *
 *   @return none.
 */
__attribute__((weak))
void ble_host_sal_hci_init(void)
{
}

/**
 *   @brief BLE host hci send value to controller.
 *
 *   @param[in]  data    hci data pointer.
 *   @param[in]  len     hci data length.
 *
 *   @return none.
 */
__attribute__((weak))
void ble_host_sal_hci_send_packet(const uint8_t *data, uint16_t len)
{
    (void) data;
    (void) len;
}

/********************* RTOS or mainloop register interface ****************/
