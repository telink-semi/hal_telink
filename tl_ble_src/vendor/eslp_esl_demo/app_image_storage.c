/********************************************************************************************************
 * @file    app_image_storage.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    01,2024
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
#include "app_image_storage.h"

#define APP_IMAGE_STORAGE_SECTOR_SIZE    0x1000
#define APP_IMAGE_STORAGE_MAGIC_SIZE     8
#define APP_IMAGE_STORAGE_PARTITION_ADDR 0x80000
#define APP_IMAGE_STORAGE_LOCATION_RAM   (1)
#define APP_IMAGE_STORAGE_LOCATION_FLASH (2)

#if APP_IMAGE_STORAGE_FLASH
    #define APP_IMAGE_STORAGE_LOCATION APP_IMAGE_STORAGE_LOCATION_FLASH
#else
    #define APP_IMAGE_STORAGE_LOCATION APP_IMAGE_STORAGE_LOCATION_RAM
#endif
#ifndef APP_IMAGE_STORAGE_MAX_IMAGES
    #define APP_IMAGE_STORAGE_MAX_IMAGES 8
#endif
#ifndef APP_IMAGE_STORAGE_MAX_IMAGE_SIZE
    #define APP_IMAGE_STORAGE_MAX_IMAGE_SIZE 0xffff
#endif

#define IMAGE_ENTRY_SIZE ((APP_IMAGE_STORAGE_MAX_IMAGE_SIZE % APP_IMAGE_STORAGE_SECTOR_SIZE) ?                                           \
                              ((APP_IMAGE_STORAGE_MAX_IMAGE_SIZE / APP_IMAGE_STORAGE_SECTOR_SIZE) + 1) * APP_IMAGE_STORAGE_SECTOR_SIZE : \
                              (APP_IMAGE_STORAGE_MAX_IMAGE_SIZE))

#if APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH
static const u8 app_image_storage_magic_marker[] = {0xa5, 0xb5, 0xc4, 0xd3, 0xe2, 0xf1, 0x00, 0x1f};
#endif

typedef struct __attribute__((packed))
{
#if APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH
    u8 length[4];
#elif (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_RAM)
    u32 length;
#endif

} app_image_storage_img_entry_t;

typedef struct __attribute__((packed))
{
#if APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH
    u8 magic[sizeof(app_image_storage_magic_marker)];
#endif
    u8 num_images;
#if APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH
    u8 image_max_size[4];
#elif (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_RAM)
    u32 image_max_size;
#endif
    app_image_storage_img_entry_t entries[];
} app_image_storage_hdr_t;

_attribute_data_retention_ static bool app_image_storage_initialized;
_attribute_data_retention_ static u8   app_image_storage_hdr_cache[sizeof(app_image_storage_hdr_t) + (APP_IMAGE_STORAGE_MAX_IMAGES * sizeof(app_image_storage_img_entry_t))];
#if (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH)
_attribute_iram_noinit_data_ u8 read_back_buf[APP_IMAGE_STORAGE_SECTOR_SIZE];
#elif (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_RAM)
_attribute_data_retention_ static u8 image_data[APP_IMAGE_STORAGE_MAX_IMAGES][APP_IMAGE_STORAGE_MAX_IMAGE_SIZE];
#endif

void app_image_storage_init(void)
{
    app_image_storage_hdr_t *hdr = (app_image_storage_hdr_t *)app_image_storage_hdr_cache;
#if (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH)
    u32 hdr_image_max_size;
    u8 *entries        = (u8 *)hdr->entries;
    u8  num_images     = APP_IMAGE_STORAGE_MAX_IMAGES;
    u32 image_max_size = APP_IMAGE_STORAGE_MAX_IMAGE_SIZE;
#endif

    if (app_image_storage_initialized) {
        return;
    }

#if (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH)
    flash_read_page(APP_IMAGE_STORAGE_PARTITION_ADDR, sizeof(*hdr), app_image_storage_hdr_cache);
    BYTE_TO_UINT32(hdr_image_max_size, (u8 *)hdr->image_max_size);
    if (memcmp(hdr->magic, app_image_storage_magic_marker, sizeof(app_image_storage_magic_marker)) ||
        (hdr->num_images != num_images) || image_max_size != hdr_image_max_size) {
        //u32 next_image_address = APP_IMAGE_STORAGE_PARTITION_ADDR + APP_IMAGE_STORAGE_SECTOR_SIZE;

        memcpy(hdr->magic, app_image_storage_magic_marker, sizeof(hdr->magic));
        hdr->image_max_size[0] = U32_BYTE0(image_max_size);
        hdr->image_max_size[1] = U32_BYTE1(image_max_size);
        hdr->image_max_size[2] = U32_BYTE2(image_max_size);
        hdr->image_max_size[3] = U32_BYTE3(image_max_size);
        hdr->num_images        = num_images;
        for (u8 i = 0; i < num_images; i++) {
            U32_TO_STREAM(entries, 0)

            tlkapi_printf(APP_LOG_EN, "Writing image storage hdr, image[%d] length 0x%08X", i, 0);
        }

        flash_erase_sector(APP_IMAGE_STORAGE_PARTITION_ADDR);
        flash_write_page(APP_IMAGE_STORAGE_PARTITION_ADDR, entries - app_image_storage_hdr_cache, app_image_storage_hdr_cache);
    } else {
        for (u8 i = 0; i < hdr->num_images; i++) {
            u32 length;

            flash_read_page(APP_IMAGE_STORAGE_PARTITION_ADDR + sizeof(*hdr) + (i * sizeof(app_image_storage_img_entry_t)), sizeof(app_image_storage_img_entry_t), entries);
            BYTE_TO_UINT32(length, entries);

            tlkapi_printf(APP_LOG_EN, "Reading image storage hdr, image[%d] length 0x%08X", i, length);
            entries += sizeof(app_image_storage_img_entry_t);
        }
    }
#elif (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_RAM)
    hdr->image_max_size = APP_IMAGE_STORAGE_MAX_IMAGE_SIZE;
    hdr->num_images     = APP_IMAGE_STORAGE_MAX_IMAGES;

    for (u8 i = 0; i < hdr->num_images; i++) {
        hdr->entries[i].length = 0;
    }
#endif

    app_image_storage_initialized = true;
}

u8 app_image_storage_get_max_image_number(void)
{
    app_image_storage_hdr_t *hdr = (app_image_storage_hdr_t *)app_image_storage_hdr_cache;

    if (!app_image_storage_initialized) {
        return 0;
    }

    return hdr->num_images;
}

u32 app_image_storage_get_max_image_length(void)
{
    app_image_storage_hdr_t *hdr        = (app_image_storage_hdr_t *)app_image_storage_hdr_cache;
    u32                      max_length = 0;

    if (!app_image_storage_initialized) {
        return 0;
    }

#if (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH)
    BYTE_TO_UINT32(max_length, hdr->image_max_size);
#elif (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_RAM)
    max_length = hdr->image_max_size;
#endif

    return max_length;
}

bool app_image_storage_get_image_length(u8 image_idx, u32 *length)
{
    app_image_storage_hdr_t *hdr = (app_image_storage_hdr_t *)app_image_storage_hdr_cache;

    if (!app_image_storage_initialized || hdr->num_images <= image_idx) {
        return false;
    }

#if (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH)
    BYTE_TO_UINT32(*length, hdr->entries[image_idx].length);
#elif (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_RAM)
    *length = hdr->entries[image_idx].length;
#else
    *length = 0;
#endif

    return true;
}

u32 app_image_storage_get_image_data(u8 image_idx, u32 offset, u32 length, u8 *buffer)
{
    u32 image_length;
#if (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH)
    u32 image_addr;
#endif

    if (!app_image_storage_get_image_length(image_idx, &image_length)) {
        return 0;
    }

    if (offset >= image_length) {
        return 0;
    }

#if (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH)
    image_addr = APP_IMAGE_STORAGE_PARTITION_ADDR + APP_IMAGE_STORAGE_SECTOR_SIZE + (image_idx * IMAGE_ENTRY_SIZE);
    length     = (offset + length) > image_length ? image_length - offset : length;
    flash_read_page(image_addr + offset, length, buffer);
#elif (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_RAM)
    length = (offset + length) > image_length ? image_length - offset : length;
    memcpy(buffer, &image_data[image_idx][offset], length);
#endif

    return length;
}

bool app_image_storage_update_image_info(u8 image_idx, u32 length)
{
    app_image_storage_hdr_t *hdr = (app_image_storage_hdr_t *)app_image_storage_hdr_cache;

    if (!app_image_storage_initialized || hdr->num_images <= image_idx) {
        return false;
    }

    if (length > app_image_storage_get_max_image_length()) {
        return false;
    }

#if (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH)
    hdr->entries[image_idx].length[0] = U32_BYTE0(length);
    hdr->entries[image_idx].length[1] = U32_BYTE1(length);
    hdr->entries[image_idx].length[2] = U32_BYTE2(length);
    hdr->entries[image_idx].length[3] = U32_BYTE3(length);

    // Write new header
    flash_erase_sector(APP_IMAGE_STORAGE_PARTITION_ADDR);
    flash_write_page(APP_IMAGE_STORAGE_PARTITION_ADDR, sizeof(app_image_storage_hdr_t) + (hdr->num_images * sizeof(app_image_storage_img_entry_t)), app_image_storage_hdr_cache);
#elif (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_RAM)
    hdr->entries[image_idx].length = length;
#endif

    return true;
}

u32 app_image_storage_image_write(u8 image_idx, u32 length, u32 offset, u8 *data, bool truncate)
{
    app_image_storage_hdr_t *hdr = (app_image_storage_hdr_t *)app_image_storage_hdr_cache;
#if (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH)
    u32 image_addr, remaining;
#elif (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_RAM)
    (void)truncate;
#endif

    if (!app_image_storage_initialized || hdr->num_images <= image_idx) {
        return 0;
    }

    if (offset + length > app_image_storage_get_max_image_length()) {
        return 0;
    }
#if (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_FLASH)
    image_addr = APP_IMAGE_STORAGE_PARTITION_ADDR + APP_IMAGE_STORAGE_SECTOR_SIZE + (image_idx * IMAGE_ENTRY_SIZE);
    remaining  = length;

    do {
        u16  sector_offset = offset % APP_IMAGE_STORAGE_SECTOR_SIZE;
        u32  chunk_len     = remaining < (u32)(APP_IMAGE_STORAGE_SECTOR_SIZE - sector_offset) ? remaining : (APP_IMAGE_STORAGE_SECTOR_SIZE - (offset % APP_IMAGE_STORAGE_SECTOR_SIZE));
        u32  read_len      = truncate ? sector_offset + chunk_len : APP_IMAGE_STORAGE_SECTOR_SIZE;
        u32  sector_start  = image_addr + ((offset / APP_IMAGE_STORAGE_SECTOR_SIZE) * APP_IMAGE_STORAGE_SECTOR_SIZE);
        bool erase         = !truncate;

        // First, read back the sector content
        flash_read_page(sector_start, read_len, read_back_buf);
        if (truncate) {
            for (u32 i = 0; i < chunk_len; i++) {
                if (read_back_buf[i + sector_offset] != 0xff) {
                    // Need to erase sector
                    erase = true;
                    break;
                }
            }
        }

        if (erase) {
            // Apply new data
            memcpy(read_back_buf + sector_offset, data, chunk_len);
            // Erase sector
            flash_erase_sector(sector_start);
            // Write
            flash_write_page(sector_start, read_len, read_back_buf);
        } else {
            // Write only data - no need to erase
            flash_write_page(sector_start + sector_offset, chunk_len, data);
        }

        remaining -= chunk_len;
        data += chunk_len;
        offset += chunk_len;
    } while (remaining);
#elif (APP_IMAGE_STORAGE_LOCATION == APP_IMAGE_STORAGE_LOCATION_RAM)
    memcpy(&image_data[image_idx][offset], data, length);
#else
    length = 0;
#endif

    return length;
}
