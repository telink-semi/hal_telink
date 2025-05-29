/********************************************************************************************************
 * @file    ots_server_data.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    04,2024
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

struct ots_server_data_object
{
    bool   active : 1;
    bool   locked : 1;
    u16    lockConnHandle;
    u8     name[OTS_SERVER_MAX_OBJECT_NAME_LENGTH];
    u16    nameLen;
    u32    properties;
    u32    allocatedSize;
    u32    currentSize;
    uuid_t type;
};

typedef struct
{
    struct ots_server_data_object objects[OTS_SERVER_MAX_OBJECTS_NUM];
} ots_server_data_t;

_attribute_data_retention_ static ots_server_data_t server_data;

void ots_server_data_init(void)
{
    memset(&server_data, 0, sizeof(server_data));
}

static bool ots_server_data_is_valid_object(struct ots_server_data_object *object)
{
    u32 offset;

    if (object < &server_data.objects[0] || object > &server_data.objects[ARRAY_SIZE(server_data.objects) - 1]) {
        return false;
    }

    offset = (u8 *)object - (u8 *)server_data.objects;
    if (offset % sizeof(server_data.objects[0])) {
        return false;
    }

    return object->active;
}

u64 ots_server_data_get_object_id(struct ots_server_data_object *object)
{
    u32 offset;

    if (object < &server_data.objects[0] || object > &server_data.objects[ARRAY_SIZE(server_data.objects) - 1]) {
        return 0;
    }

    offset = (u8 *)object - (u8 *)server_data.objects;
    if (offset % sizeof(server_data.objects[0])) {
        return 0;
    }

    return (offset / sizeof(server_data.objects[0])) + OTS_SERVER_OBJECT_ID_START;
}

struct ots_server_data_object *ots_server_data_get_object_by_object_id(u64 id)
{
    if (id < OTS_SERVER_OBJECT_ID_START || id >= (ARRAY_SIZE(server_data.objects) + OTS_SERVER_OBJECT_ID_START)) {
        return NULL;
    }

    return server_data.objects[id - OTS_SERVER_OBJECT_ID_START].active ? &server_data.objects[id - OTS_SERVER_OBJECT_ID_START] : NULL;
}

struct ots_server_data_object *ots_server_data_get_first_object(void)
{
    foreach_arr(i, server_data.objects)
    {
        if (server_data.objects[i].active) {
            return &server_data.objects[i];
        }
    }

    return NULL;
}

struct ots_server_data_object *ots_server_data_get_last_object(void)
{
    for (int i = ARRAY_SIZE(server_data.objects) - 1; i >= 0; i--) {
        if (server_data.objects[i].active) {
            return &server_data.objects[i];
        }
    }

    return NULL;
}

struct ots_server_data_object *ots_server_data_get_next_object(struct ots_server_data_object *object)
{
    bool object_found = false;

    foreach_arr(i, server_data.objects)
    {
        if (!server_data.objects[i].active) {
            continue;
        }

        if (object_found) {
            return &server_data.objects[i];
        } else if (object == &server_data.objects[i]) {
            object_found = true;
        }
    }

    return NULL;
}

struct ots_server_data_object *ots_server_data_get_prev_object(struct ots_server_data_object *object)
{
    bool object_found = false;

    for (int i = ARRAY_SIZE(server_data.objects) - 1; i >= 0; i--) {
        if (!server_data.objects[i].active) {
            continue;
        }

        if (object_found) {
            return &server_data.objects[i];
        } else if (object == &server_data.objects[i]) {
            object_found = true;
        }
    }

    return NULL;
}

struct ots_server_data_object *ots_server_data_new_object(u32 allocatedSize, u32 currentSize, uuid_t *type, u32 properties)
{
    foreach_arr(i, server_data.objects)
    {
        if (!server_data.objects[i].active) {
            server_data.objects[i].active        = true;
            server_data.objects[i].currentSize   = currentSize;
            server_data.objects[i].allocatedSize = allocatedSize;
            server_data.objects[i].type          = *type;
            server_data.objects[i].properties    = properties;
            return &server_data.objects[i];
        }
    }

    return NULL;
}

bool ots_server_data_delete_object(struct ots_server_data_object *object)
{
    if (ots_server_data_is_valid_object(object)) {
        memset(object, 0, sizeof(*object));
        return true;
    }

    return false;
}

bool ots_server_data_set_name(struct ots_server_data_object *object, u8 *name, u16 nameLen)
{
    if (!ots_server_data_is_valid_object(object) || nameLen > sizeof(object->name)) {
        return false;
    }

    memcpy(object->name, name, nameLen);
    object->nameLen = nameLen;

    return true;
}

bool ots_server_data_get_name(struct ots_server_data_object *object, u8 **name, u16 *nameLen)
{
    if (!ots_server_data_is_valid_object(object)) {
        return false;
    }

    *name    = object->name;
    *nameLen = object->nameLen;

    return true;
}

bool ots_server_data_get_properties(struct ots_server_data_object *object, u32 *properties)
{
    if (!ots_server_data_is_valid_object(object)) {
        return false;
    }

    *properties = object->properties;

    return true;
}

bool ots_server_data_get_type(struct ots_server_data_object *object, uuid_t *type)
{
    if (!ots_server_data_is_valid_object(object)) {
        return false;
    }

    *type = object->type;

    return true;
}

bool ots_server_data_set_size(struct ots_server_data_object *object, u32 allocatedSize, u32 currentSize)
{
    if (!ots_server_data_is_valid_object(object)) {
        return false;
    }

    object->allocatedSize = allocatedSize;
    object->currentSize   = currentSize;

    return true;
}

bool ots_server_data_get_size(struct ots_server_data_object *object, u32 *allocatedSize, u32 *currentSize)
{
    if (!ots_server_data_is_valid_object(object)) {
        return false;
    }

    if (allocatedSize) {
        *allocatedSize = object->allocatedSize;
    }

    if (currentSize) {
        *currentSize = object->currentSize;
    }

    return true;
}

bool ots_server_data_get_length(struct ots_server_data_object *object, u32 *allocatedSize, u32 *currentSize)
{
    if (!ots_server_data_is_valid_object(object)) {
        return false;
    }

    *allocatedSize = object->allocatedSize;
    *currentSize   = object->currentSize;

    return true;
}

bool ots_server_data_set_locked(struct ots_server_data_object *object, u16 connHandle, bool locked)
{
    if (!ots_server_data_is_valid_object(object)) {
        return false;
    }

    if ((locked && object->locked) || (!locked && object->locked && (connHandle != object->lockConnHandle))) {
        return false;
    }

    object->locked         = locked;
    object->lockConnHandle = locked ? connHandle : 0;

    return true;
}
