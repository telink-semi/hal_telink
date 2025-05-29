#include <string.h>

#include "common/types.h"
#include "common/utility.h"

#include "../../../inc/ble_host.h"

#include "../inc/ble_att.h"
#include "../inc/ble_att_uuid.h"
#include "../inc/ble_att_service.h"
#include "../inc/uuid16bit.h"

/**
 *  @brief  Compare uuid that attribute handle and input uuid are same.
 *
 *  @param[in]  pAttr  Pointer to attribute service.
 *  @param[in]  uuidLen  Length of uuid.
 *  @param[in]  pUuid  Pointer to uuid.
 *
 *  @return  true if uuid is same, false otherwise.
*/
bool ble_attribute_uuid_cmp_uuid(const struct atts_attribute *pAttr, uint8_t uuidLen, const uint8_t *pUuid)
{
    if (uuidLen == ATT_16_UUID_LEN) {
        return ble_attribute_uuid_cmp_uuid16(pAttr, *(uint16_t *) pUuid);
    } else if (uuidLen == ATT_128_UUID_LEN) {
        if (pAttr->uuidLen == ATT_128_UUID_LEN) {
            return (memcmp(pAttr->uuid, pUuid, ATT_128_UUID_LEN) == 0);
        } else if (pAttr->uuidLen == ATT_16_UUID_LEN) {
            return ble_uuid_cmp_uuid16_uuid128(pAttr->uuid, pUuid);
        }
    }

    return false;
}

bool ble_attribute_uuid_cmp_uuid16(const struct atts_attribute *pAttr, uint16_t uuid16)
{
    if (pAttr->uuidLen == ATT_16_UUID_LEN) {
        return (*(uint16_t *) pAttr->uuid == uuid16);
    } else if (pAttr->uuidLen == ATT_128_UUID_LEN) {
        return ble_uuid_cmp_uuid16_uuid128((uint8_t *) &uuid16, pAttr->uuid);
    } else {
        return false;
    }
}

bool ble_attribute_value_cmp_uuid(const struct atts_attribute *pAttr, uint8_t uuidLen, const uint8_t *pUuid)
{
    if (*pAttr->attrValueLen == uuidLen) {
        return (memcmp(pAttr->attrValue, pUuid, uuidLen) == 0);
    } else if ((*pAttr->attrValueLen == ATT_128_UUID_LEN) && uuidLen == ATT_16_UUID_LEN) {
        return ble_uuid_cmp_uuid16_uuid128(pUuid, pAttr->attrValue);
    } else if ((*pAttr->attrValueLen == ATT_16_UUID_LEN) && uuidLen == ATT_128_UUID_LEN) {
        return ble_uuid_cmp_uuid16_uuid128(pAttr->attrValue, pUuid);
    } else {
        return false;
    }
}

uint16_t ble_atts_find_uuid_in_range(uint16_t connHandle, uint16_t startHandle, uint16_t endHandle,
    uint8_t uuidLen, const uint8_t *pUuid, const struct atts_attribute **pAttr, struct atts_group **pAttrGroup)
{
    struct atts_group *pGroup = ble_host_get_attribute_service_group_by_conn_handle(connHandle);

    if (pGroup == NULL) {
        return ATTR_HANDLE_NONE;
    }

    for (; pGroup != NULL; pGroup = pGroup->pNext) {
        if ((startHandle < pGroup->startHandle) && (endHandle >= pGroup->startHandle)) {
            startHandle = pGroup->startHandle;
        }
        if ((startHandle >= pGroup->startHandle) && (startHandle <= pGroup->endHandle)) {
            *pAttr = &pGroup->pAttr[startHandle - pGroup->startHandle];
            while ((startHandle <= pGroup->endHandle) && (startHandle <= endHandle)) {
                if (ble_attribute_uuid_cmp_uuid(*pAttr, uuidLen, pUuid)) {
                    *pAttrGroup = pGroup;
                    return startHandle;
                }
                if (startHandle == ATTR_HANDLE_END_MAX) {
                    break;
                }
                startHandle++;
                (*pAttr)++;
            }
        }
    }
    return ATTR_HANDLE_NONE;
}

uint8_t ble_atts_check_permissions(uint16_t connHandle, uint8_t permit, uint8_t handle, uint8_t permissions)
{
    (void) connHandle;
    (void) permit;
    (void) handle;
    (void) permissions;
    if (!(permit & permissions)) {
        return (permit & ATT_PERMISSIONS_READ) ? ATT_ERR_READ_NOT_PERMITTED : ATT_ERR_WRITE_NOT_PERMITTED;
    }

//     if (permissions & ATT_PERMISSIONS_SECURITY) {
// //        return blt_gatt_requestServiceAccess(connHandle, permissions);
//     }

    return ATT_SUCCESS;
}

uint16_t ble_atts_find_service_group_end_handle(uint16_t connHandle, uint16_t startHandle)
{
    const struct atts_attribute *pAttr;
    uint16_t prevHandle;

    if (startHandle == ATTR_HANDLE_END_MAX) {
        return ATTR_HANDLE_END_MAX;
    }

    prevHandle = startHandle;
    startHandle++;

    struct atts_group *pGroup = ble_host_get_attribute_service_group_by_conn_handle(connHandle);

    if (pGroup == NULL) {
        return ATTR_HANDLE_END_MAX;
    }

    for (; pGroup != NULL; pGroup = pGroup->pNext) {

        if (startHandle < pGroup->startHandle) {
            startHandle = pGroup->startHandle;
        }

        if (startHandle <= pGroup->endHandle) {
            pAttr = &pGroup->pAttr[startHandle - pGroup->startHandle];
            while (startHandle <= pGroup->endHandle) {

                if (ble_attribute_uuid_cmp_uuid16(pAttr, DECLARATIONS_UUID_PRIMARY_SERVICE) ||
                    ble_attribute_uuid_cmp_uuid16(pAttr, DECLARATIONS_UUID_SECONDARY_SERVICE)) {
                    return prevHandle;
                }

                if (startHandle == ATTR_HANDLE_END_MAX) {
                    return ATTR_HANDLE_END_MAX;
                }

                prevHandle = startHandle;
                startHandle++;
                pAttr++;
            }
            if (startHandle == pGroup->endHandle + 1) {
                return prevHandle;
            }
        }
    }

    return ATTR_HANDLE_END_MAX;
}

uint16_t ble_atts_get_attribute_range_of_handle(uint16_t connHandle, uint16_t startHandle, uint16_t endHandle,
    const struct atts_attribute **pAttr)
{
    for (struct atts_group *pGroup = ble_host_get_attribute_service_group_by_conn_handle(connHandle); pGroup != NULL; pGroup = pGroup->pNext) {
        if ((startHandle < pGroup->startHandle) && (endHandle >= pGroup->startHandle)) {
            startHandle = pGroup->startHandle;
        }

        if ((startHandle >= pGroup->startHandle) && (startHandle <= pGroup->endHandle)) {
            *pAttr = &pGroup->pAttr[startHandle - pGroup->startHandle];
            return startHandle;
        }
    }

    return ATTR_HANDLE_NONE;
}

const struct atts_attribute *ble_atts_get_service_group_by_handle(uint16_t connHandle, uint16_t handle,
    struct atts_group **pAttrGroup)
{

    struct atts_group *pGroup = ble_host_get_attribute_service_group_by_conn_handle(connHandle);

    if (pGroup == NULL) {
        return NULL;
    }

    for (; pGroup != NULL; pGroup = pGroup->pNext) {
        if ((handle >= pGroup->startHandle) && (handle <= pGroup->endHandle)) {
            if (pAttrGroup)
                *pAttrGroup = pGroup;
            return &pGroup->pAttr[handle - pGroup->startHandle];
        }
    }

    return NULL;
}

