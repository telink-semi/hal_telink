#include <string.h>

#include "common/types.h"

#include "../../inc/ble_host.h"

#include "../../l2cap/att/inc/ble_att_uuid.h"
#include "../../l2cap/att/inc/ble_att_service.h"
#include "../../l2cap/att/inc/uuid16bit.h"

#include "../inc/gatt.h"
#include "../inc/gatt_internal.h"

#include "inc/gatts_sdp.h"

static uint16_t ble_gatts_found_service_uuid(const struct att_uuid *service_uuid, struct atts_group **p_atts_group)
{
    struct atts_group *p_group = *p_atts_group;

    if (CHECK_ATT_UUID(service_uuid) || p_group == NULL) {
        return GATT_ATTR_HANDLE_NONE;
    }

    for (; p_group != NULL; p_group = p_group->pNext) {
        for (int i = 0; i <= p_group->endHandle - p_group->startHandle; i++) {
            const struct atts_attribute *p_attr = &p_group->pAttr[i];

            if (ble_attribute_uuid_cmp_uuid16(p_attr, DECLARATIONS_UUID_PRIMARY_SERVICE) ||
                ble_attribute_uuid_cmp_uuid16(p_attr, DECLARATIONS_UUID_SECONDARY_SERVICE)) {
                if (ble_attribute_value_cmp_uuid(p_attr, service_uuid->uuidLength, service_uuid->uuid)) {
                    *p_atts_group = p_group;
                    return p_group->startHandle + i;
                }
            }
        }
    }

    return GATT_ATTR_HANDLE_NONE;
}

static void ble_gatts_discover_char_info(const struct atts_attribute *p_attr, uint16_t attr_handle, int attr_count,
    const struct gatts_discover_char_info *char_list, void *user_data)
{
#define FIND_CHAR_MAX_NUM           50

    struct gatts_discover_char_param char_param = {
        .service_index = 0,
    };

    uint8_t char_number[FIND_CHAR_MAX_NUM] = { 0 };

    do {
        if (attr_count > 1 && ble_attribute_uuid_cmp_uuid16(p_attr, DECLARATIONS_UUID_CHARACTERISTIC)) {
            // skip characteristic attribute.
            p_attr++;
            attr_handle++;
            attr_count--;
            for (int i = 0;i < FIND_CHAR_MAX_NUM;i++) {
                if (char_list[i].char_uuid == NULL) {
                    break;    // if char uuid is NULL, it means the end of char list
                }

                if (ble_attribute_uuid_cmp_uuid(p_attr, char_list[i].char_uuid->uuidLength, char_list[i].char_uuid->uuid)
                    && char_list[i].callback != NULL) {
                    char_param.handle = attr_handle;
                    char_param.char_index = char_number[i];
                    char_param.data = p_attr->attrValue;
                    char_param.data_length = p_attr->attrValueLen;
                    char_param.CCC_value = NULL;
                    char_param.ccc_handle = GATT_ATTR_HANDLE_NONE;

                    // skip character value attribute.
                    p_attr++;
                    attr_handle++;
                    attr_count--;

                    if (attr_count > 0 &&
                        ble_attribute_uuid_cmp_uuid16(p_attr, DESCRIPTOR_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION)) {
                        char_param.CCC_value = p_attr->attrValue;
                        char_param.ccc_handle = attr_handle;
                        // skip CCC attribute.
                        p_attr++;
                        attr_handle++;
                        attr_count--;
                    } else if (attr_count > 1 &&
                        ble_attribute_uuid_cmp_uuid16(p_attr + 1, DESCRIPTOR_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION)) {
                        char_param.CCC_value = (p_attr + 1)->attrValue;
                        char_param.ccc_handle = attr_handle + 1;
                        // skip CCC attribute.
                        p_attr += 2;
                        attr_handle += 2;
                        attr_count -= 2;
                    }

                    char_list[i].callback(&char_param, user_data);

                    char_number[i]++;
                }
            }
        } else {
            p_attr++;
            attr_handle++;
            attr_count--;
        }
    } while (attr_count > 0);
}

int ble_gatts_discover_by_service_uuid(const struct att_uuid *service_uuid,
    const struct gatts_discover_char_info *char_list, void *user_data)
{
    struct atts_group *p_atts_group = ble_host_get_attribute_service_group();

    uint16_t start_handle = ble_gatts_found_service_uuid(service_uuid, &p_atts_group);

    if (start_handle == GATT_ATTR_HANDLE_NONE || char_list == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_NOT_FOUND_SERVICE);
    }

    ble_gatts_discover_char_info(&p_atts_group->pAttr[start_handle - p_atts_group->startHandle],
        start_handle, p_atts_group->endHandle - start_handle + 1, char_list, user_data);

    return BLE_HOST_ERR_SUCC;
}

static int ble_gatts_discover_included_uuid_count(const struct atts_attribute *p_attr, int attr_count,
    const struct att_uuid *included_uuid)
{
    int count = 0;
    do {
        if (attr_count > 1 && ble_attribute_uuid_cmp_uuid16(p_attr, DECLARATIONS_UUID_INCLUDE)) {
            struct att_included_uuid_attribute_value {
                uint16_t start_handle;
                uint16_t end_handle;
                uint8_t uuid[0];        // if uuid is 128bit, uuid is NULL.
            };

            struct att_included_uuid_attribute_value *p_included_info =
                (struct att_included_uuid_attribute_value *) p_attr->attrValue;

            const struct atts_attribute *p_included_attr_info = ble_host_get_attribute_info(p_included_info->start_handle);

            if (p_included_attr_info == NULL) {
                break;
            }

            // check first attribute uuid is primary or secondary service uuid 
            if (ble_attribute_uuid_cmp_uuid16(p_included_attr_info, DECLARATIONS_UUID_PRIMARY_SERVICE) ||
                ble_attribute_uuid_cmp_uuid16(p_included_attr_info, DECLARATIONS_UUID_SECONDARY_SERVICE)) {
                    // check included uuid is match or not.
                if (ble_attribute_value_cmp_uuid(p_included_attr_info, included_uuid->uuidLength, included_uuid->uuid)) {
                    count++;
                }
            }

        }
        p_attr++;
        attr_count--;

    } while (attr_count > 0);

    return count;
}

int ble_gatts_discover_included_uuid(const struct att_uuid *service_uuid, const struct att_uuid *included_uuid)
{
    struct atts_group *p_atts_group = ble_host_get_attribute_service_group();

    uint16_t start_handle = ble_gatts_found_service_uuid(service_uuid, &p_atts_group);

    if (start_handle == GATT_ATTR_HANDLE_NONE) {
        return 0;
    }

    return ble_gatts_discover_included_uuid_count(&p_atts_group->pAttr[start_handle - p_atts_group->startHandle],
        p_atts_group->endHandle - start_handle + 1, included_uuid);
}

static void ble_gatts_discover_include_info(const struct atts_attribute *p_attr, uint16_t attr_handle, int attr_count,
    const struct gatts_discover_included_uuid *included_uuid, void *user_data)
{
    const struct atts_attribute *p_attr_temp = p_attr;
    int attr_count_temp = attr_count;
    uint8_t incl_index[included_uuid->included_size];
    memset(incl_index, 0, included_uuid->included_size);
    do {
        if (attr_count > 1 && ble_attribute_uuid_cmp_uuid16(p_attr, DECLARATIONS_UUID_INCLUDE)) {
            struct att_included_uuid_attribute_value {
                uint16_t start_handle;
                uint16_t end_handle;
                uint8_t uuid[0];        // if uuid is 128bit, uuid is NULL.
            };

            struct att_included_uuid_attribute_value *p_included_info =
                (struct att_included_uuid_attribute_value *) p_attr->attrValue;

            const struct atts_attribute *p_included_attr_info = ble_host_get_attribute_info(p_included_info->start_handle);

            if (p_included_attr_info == NULL) {
                break;
            }

            // check first attribute uuid is primary or secondary service uuid 
            if (ble_attribute_uuid_cmp_uuid16(p_included_attr_info, DECLARATIONS_UUID_PRIMARY_SERVICE) ||
                ble_attribute_uuid_cmp_uuid16(p_included_attr_info, DECLARATIONS_UUID_SECONDARY_SERVICE)) {

                for (int i = 0; i < included_uuid->included_size; i++) {
                    if (ble_attribute_value_cmp_uuid(p_included_attr_info, included_uuid->incl_list[i].incl_uuid->uuidLength,
                        included_uuid->incl_list[i].incl_uuid->uuid)) {

                        if (included_uuid->incl_list[i].found_incl_callback != NULL) {
                            included_uuid->incl_list[i].found_incl_callback(incl_index[i], p_included_info->start_handle,
                                p_included_info->end_handle, user_data);
                        }
                        ble_gatts_discover_char_info(p_included_attr_info, p_included_info->start_handle,
                            p_included_info->end_handle - p_included_info->start_handle + 1, included_uuid->incl_list[i].incl_char_list,
                            user_data);
                        incl_index[i]++;
                        break;
                    }
                }

            }
        }
        p_attr++;
        attr_count--;

    } while (attr_count > 0);

    ble_gatts_discover_char_info(p_attr_temp, attr_handle, attr_count_temp, included_uuid->service_char_list, user_data);
}

int ble_gatts_discover_by_service_uuid_with_included_uuid(const struct gatts_discover_included_uuid *included_uuid, void *user_data)
{
    if (included_uuid == NULL || included_uuid->service_char_list == NULL ||
        included_uuid->service_uuid == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_NOT_FOUND_SERVICE);
    }

    for (int i = 0; i < included_uuid->included_size; i++) {
        if (included_uuid->incl_list[i].incl_uuid == NULL || included_uuid->incl_list[i].incl_char_list == NULL) {
            return BLE_GATT_ERR(BLE_GATT_ERR_NOT_FOUND_SERVICE);
        }
    }

    struct atts_group *p_atts_group = ble_host_get_attribute_service_group();

    uint16_t start_handle = ble_gatts_found_service_uuid(included_uuid->service_uuid, &p_atts_group);

    if (start_handle == GATT_ATTR_HANDLE_NONE) {
        return BLE_GATT_ERR(BLE_GATT_ERR_NOT_FOUND_SERVICE);
    }

    ble_gatts_discover_include_info(&p_atts_group->pAttr[start_handle - p_atts_group->startHandle],
        start_handle, p_atts_group->endHandle - start_handle + 1, included_uuid, user_data);
    return BLE_HOST_ERR_SUCC;
}
