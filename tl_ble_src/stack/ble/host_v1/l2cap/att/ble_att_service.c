#include "common/types.h"

#include "inc/ble_att_service.h"
#include "inc/ble_att_internal.h"

struct atts_group *ble_host_get_attribute_service_group_by_conn_handle(uint16_t conn_handle)
{
    (void) conn_handle;
    return ble_host_get_attribute_service_group();
}

void ble_host_add_attribute_service_group_by_conn_handle(uint16_t conn_handle, struct atts_group *pGroup)
{
    (void) conn_handle;
    ble_host_add_attribute_service_group(pGroup);
}

void ble_host_remove_attribute_service_group_by_conn_handle(uint16_t conn_handle, uint16_t startHandle)
{
    (void) conn_handle;
    ble_host_remove_attribute_service_group(startHandle);
}

struct atts_group *ble_host_get_attribute_service_group(void)
{
    return *ble_host_att_get_atts_header();
}

void ble_host_add_attribute_service_group(struct atts_group *pGroup)
{
    struct atts_group **pHeader = ble_host_att_get_atts_header();

    struct atts_group *prev_group = NULL;
    struct atts_group *curr_group = *pHeader;
    for (; curr_group != NULL; prev_group = curr_group, curr_group = curr_group->pNext) {

        if (curr_group == pGroup) {
            return;
        }

        if (curr_group->startHandle > pGroup->startHandle) {
            break;
        }
    }

    if (prev_group == NULL) {
        pGroup->pNext = NULL;
        *pHeader = pGroup;
    } else {
        pGroup->pNext = prev_group->pNext;
        prev_group->pNext = pGroup;
    }
}

void ble_host_remove_attribute_service_group(uint16_t startHandle)
{
    struct atts_group **pHeader = ble_host_att_get_atts_header();

    struct atts_group *prev_group = NULL;
    struct atts_group *curr_group = *pHeader;
    for (; curr_group != NULL; prev_group = curr_group, curr_group = curr_group->pNext) {

        if (curr_group->startHandle == startHandle) {
            if (prev_group == NULL) {
                *pHeader = curr_group->pNext;
            } else {
                prev_group->pNext = curr_group->pNext;
            }
            break;
        }
    }
}

const struct atts_attribute *ble_host_get_attribute_info(uint16_t attr_handle)
{
    struct atts_group *pGroup = ble_host_get_attribute_service_group();

    if (pGroup == NULL) {
        return NULL;
    }

    for (; pGroup != NULL; pGroup = pGroup->pNext) {
        if ((attr_handle >= pGroup->startHandle) && (attr_handle <= pGroup->endHandle)) {
            return &pGroup->pAttr[attr_handle - pGroup->startHandle];
        }
    }

    return NULL;
}
