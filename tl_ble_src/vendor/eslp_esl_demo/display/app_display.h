/********************************************************************************************************
 * @file    app_display.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    05,2024
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
#include "stack/ble/ble.h"

#ifndef APP_DISPLAY
    #define APP_DISPLAY

    #define APP_DISPLAY_MAX_DISPLAYS 4

typedef bool (*app_display_image_cb_t)(void);
typedef bool (*app_display_get_buffer_cb_t)(u8 **buffer_ptr);
typedef bool (*app_display_clean_cb_t)(void);
typedef bool (*app_display_is_busy_cb_t)(void);
typedef void (*app_display_loop_cb_t)(void);
typedef void (*app_display_init_cb_t)(void);

typedef struct
{
    u16                         width;
    u16                         heigth;
    u16                         image_size;
    u8                          type;
    app_display_init_cb_t       init;
    app_display_image_cb_t      image;
    app_display_is_busy_cb_t    is_busy;
    app_display_loop_cb_t       loop;
    app_display_clean_cb_t      clean;
    app_display_get_buffer_cb_t get_buffer;
} app_display_iface_t;

/**
 * @brief      Get the number of displays currently available.
 * @param[in]  none - No input parameters.
 * @return     u8 - The number of displays.
 */
u8 app_display_get_num_displays(void);

/**
 * @brief      Register a display with the specified configuration.
 * @param[in]  config - Pointer to the configuration of the display interface.
 * @param[out] display_id - Pointer to the variable where the display ID will be stored upon successful registration.
 * @return     bool - true: registration successful, false: registration failed.
 */
bool app_display_register(const app_display_iface_t *config, u8 *display_id);

/**
 * @brief      Main loop for the display management system. Should be called periodically to process display tasks.
 * @param[in]  none - No input parameters.
 * @return     none.
 */
void app_display_loop(void);

/**
 * @brief      Display an image on the specified display.
 * @param[in]  display_id - The ID of the display on which the image should be shown.
 * @return     bool - true: image displayed successfully, false: failed to display the image.
 */
bool app_display_image(u8 display_id);

/**
 * @brief      Clear the display on the specified display.
 * @param[in]  display_id - The ID of the display to be cleared.
 * @return     bool - true: display cleared successfully, false: failed to clear the display.
 */
bool app_display_clean(u8 display_id);

/**
 * @brief      Check if the specified display is currently busy.
 * @param[in]  display_id - The ID of the display to check.
 * @return     bool - true: display is busy, false: display is not busy.
 */
bool app_display_is_busy(u8 display_id);

/**
 * @brief      Get information about the specified display.
 * @param[in]  display_id - The ID of the display to query.
 * @param[out] width - Pointer to the variable where the display width will be stored.
 * @param[out] height - Pointer to the variable where the display height will be stored.
 * @param[out] displayType - Pointer to the variable where the display type will be stored.
 * @param[out] image_size - Pointer to the variable where the image size will be stored.
 * @return     bool - true: information retrieved successfully, false: failed to retrieve information.
 */
bool app_display_get_info(u8 display_id, u16 *width, u16 *height, u8 *displayType, u16 *image_size);

/**
 * @brief      Get the buffer pointer for the specified display's image data.
 * @param[in]  display_id - The ID of the display to query.
 * @param[out] buffer_ptr - Pointer to the buffer where the display image data will be stored.
 * @return     bool - true: buffer retrieved successfully, false: failed to retrieve buffer.
 */
bool app_display_get_buffer(u8 display_id, u8 **buffer_ptr);

#endif /* APP_DISPLAY */
