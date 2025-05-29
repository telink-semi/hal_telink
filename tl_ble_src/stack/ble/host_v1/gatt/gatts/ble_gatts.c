#include <string.h>

#include "common/types.h"
#include "common/utility.h"

#include "algorithm/crypto/crypto_alg.h"

#include "../../l2cap/att/inc/ble_att_service.h"
#include "../../l2cap/att/inc/ble_att_uuid.h"
#include "../../l2cap/att/inc/uuid16bit.h"

#define DATABASE_HASH_MAX_SIZE 100

static void ble_gatts_calc_database_hash(const struct atts_group *p_group, uint8_t database_hash[16])
{
    if (p_group == NULL || database_hash == NULL) {
        return;
    }

    uint8_t htvDataBuff[DATABASE_HASH_MAX_SIZE];
    int htvDataLen = 0;
    uint8_t *htvData = htvDataBuff;

    blc_aes_cmac_context_t aesCmac;

    memset(&aesCmac, 0, sizeof(blc_aes_cmac_context_t));
    uint8_t key[16] = { 0 };

    blc_crypto_alg_aes_cmac_init_key(&aesCmac, key);

    for (; p_group != NULL; p_group = p_group->pNext) {
        const struct atts_attribute *p_attr = p_group->pAttr;
        if (p_attr == NULL) {
            continue;
        }

        for (int i = 0; i < p_group->endHandle - p_group->startHandle + 1; i++) {
            //Attribute Handle, Attribute Type, Attribute value
            if (ble_attribute_uuid_cmp_uuid16(p_attr, DECLARATIONS_UUID_PRIMARY_SERVICE) ||
                ble_attribute_uuid_cmp_uuid16(p_attr, DECLARATIONS_UUID_SECONDARY_SERVICE) ||
                ble_attribute_uuid_cmp_uuid16(p_attr, DECLARATIONS_UUID_INCLUDE) ||
                ble_attribute_uuid_cmp_uuid16(p_attr, DESCRIPTOR_UUID_CHARACTERISTIC_EXTENDED_PROPERTIES)) {
                U16_TO_STREAM(htvData, p_group->startHandle + i);
                STR_TO_STREAM(htvData, p_attr->uuid, p_attr->uuidLen);
                STR_TO_STREAM(htvData, p_attr->attrValue, *p_attr->attrValueLen);
                htvDataLen += 2 + p_attr->uuidLen + (*p_attr->attrValueLen);
            }
            //Attribute Handle, Attribute Type,.
            else if (ble_attribute_uuid_cmp_uuid16(p_attr, DESCRIPTOR_UUID_CHARACTERISTIC_USER_DESCRIPTION) ||
                ble_attribute_uuid_cmp_uuid16(p_attr, DESCRIPTOR_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION) ||
                ble_attribute_uuid_cmp_uuid16(p_attr, DESCRIPTOR_UUID_SERVER_CHARACTERISTIC_CONFIGURATION) ||
                ble_attribute_uuid_cmp_uuid16(p_attr, DESCRIPTOR_UUID_CHARACTERISTIC_PRESENTATION_FORMAT) ||
                ble_attribute_uuid_cmp_uuid16(p_attr, DESCRIPTOR_UUID_CHARACTERISTIC_AGGREGATE_FORMAT)) {
                U16_TO_STREAM(htvData, p_group->startHandle + i);
                STR_TO_STREAM(htvData, p_attr->uuid, p_attr->uuidLen);
                htvDataLen += 2 + p_attr->uuidLen;
            }
            //Attribute Handle, Attribute Type, Attribute value
            else if (ble_attribute_uuid_cmp_uuid16(p_attr, DECLARATIONS_UUID_CHARACTERISTIC)) {
                if (p_attr->settings & ATTS_SET_ATTR_VALUE_PROPERTIES) {
                    U16_TO_STREAM(htvData, p_group->startHandle + i);
                    STR_TO_STREAM(htvData, p_attr->uuid, p_attr->uuidLen);
                    U8_TO_STREAM(htvData, *p_attr->attrValue);                    //Characteristic Properties
                    U16_TO_STREAM(htvData, p_group->startHandle + i + 1);         //Characteristic Value Handle
                    const struct atts_attribute *p_next_attr = p_attr + 1;
                    STR_TO_STREAM(htvData, p_next_attr->uuid, p_next_attr->uuidLen); //Characteristic UUID
                    htvDataLen += 2 + p_attr->uuidLen + 1 + 2 + p_next_attr->uuidLen;
                } else {
                    U16_TO_STREAM(htvData, p_group->startHandle + i);
                    STR_TO_STREAM(htvData, p_attr->uuid, p_attr->uuidLen);
                    STR_TO_STREAM(htvData, p_attr->attrValue, *p_attr->attrValueLen);
                    htvDataLen += 2 + p_attr->uuidLen + (*p_attr->attrValueLen);
                }
            }
            p_attr++;

            if (htvDataLen > 16) {
                int j = 0;
                for (; j < htvDataLen - 16; j += 16) //
                {
                    blc_crypto_alg_aes_cmac_block(&aesCmac, htvDataBuff + j);
                }
                memcpy(htvDataBuff, htvDataBuff + j, htvDataLen - j);
                htvDataLen -= j;
                htvData = htvDataBuff + htvDataLen;
            }
        }
    }

    blc_crypto_alg_aes_cmac_finish(&aesCmac, htvDataBuff, htvDataLen);
    for (int i = 0; i < 16; i++) {
        database_hash[15 - i] = aesCmac.mac[i];
    }
}

void ble_gatts_calculate_database_hash(uint8_t database_hash[16])
{
    const struct atts_group *p_group = ble_host_get_attribute_service_group();
    ble_gatts_calc_database_hash(p_group, database_hash);
}

void ble_gatts_calculate_database_hash_by_conn_handle(uint16_t conn_handle, uint8_t database_hash[16])
{
    const struct atts_group *p_group = ble_host_get_attribute_service_group_by_conn_handle(conn_handle);
    ble_gatts_calc_database_hash(p_group, database_hash);
}
