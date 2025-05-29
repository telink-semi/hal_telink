#include <string.h>
#include <stdio.h>

#include "common/types.h"

#include "inc/ble_att_uuid.h"

#define UUID_16_BASE_OFFSET                 12

/* Base UUID : 0000[0000]-0000-1000-8000-00805F9B34FB
 * 0x2800    : 0000[2800]-0000-1000-8000-00805F9B34FB
 *  little endian 0x2800 : [00 28] -> no swapping required
 *  big endian    0x2800 : [28 00] -> swapping required
 */
static const struct att_uuid uuid128_base = {
    .uuidLength = ATT_128_UUID_LEN,
    .uuid128 = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

/**
 *  @brief Convert an attribute UUID to a 128-bit UUID.
 *
 *  @param[in] src Source UUID to convert.
 *  @param[out] dst Destination UUID to store the result.
 *
 *  @return None.
*/
static void ble_att_uuid_to_uuid128(const struct att_uuid *src, struct att_uuid *dst)
{
    dst->uuidLength = ATT_128_UUID_LEN;

    switch (src->uuidLength) {
    case ATT_16_UUID_LEN:
        *dst = uuid128_base;
        memcpy(&dst->uuid128[UUID_16_BASE_OFFSET], &src->uuid16, ATT_16_UUID_LEN);
        break;
    case ATT_128_UUID_LEN:
        memcpy(dst->uuid128, src->uuid128, ATT_128_UUID_LEN);
        break;
    }
}

/**
 *  @brief Compare two attribute UUIDs.
 *
 *  @param[in] uuid1 First UUID to compare.
 *  @param[in] uuid2 Second UUID to compare.
 *
 *  @return 0 if the UUIDs are equal, otherwise if the UUIDs are not equal or if one of the UUIDs is invalid.
*/
int ble_att_uuid_cmp(const struct att_uuid *uuid1, const struct att_uuid *uuid2)
{
    if (CHECK_ATT_UUID(uuid1) || CHECK_ATT_UUID(uuid2)) {
        return 1;
    }

    struct att_uuid uuid128_1, uuid128_2;
    ble_att_uuid_to_uuid128(uuid1, &uuid128_1);
    ble_att_uuid_to_uuid128(uuid2, &uuid128_2);

    return memcmp(uuid128_1.uuid128, uuid128_2.uuid128, ATT_128_UUID_LEN);
}

/**
 *  @brief Format an attribute UUID as a string.
 *
 *  @param[in] uuid UUID to format.
 *
 *  @return Pointer to a string containing the formatted UUID.
*/
const char *ble_att_uuid_format(const struct att_uuid *uuid)
{
    static char uuidStr[37] = { '\0' };
    if (uuid->uuidLength == ATT_16_UUID_LEN) {
        sprintf(uuidStr, "0x%04x", uuid->uuid16);
    } else if (uuid->uuidLength == ATT_128_UUID_LEN) {
        sprintf(uuidStr, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            uuid->uuid128[15], uuid->uuid128[14], uuid->uuid128[13], uuid->uuid128[12],
            uuid->uuid128[11], uuid->uuid128[10], uuid->uuid128[9], uuid->uuid128[8],
            uuid->uuid128[7], uuid->uuid128[6], uuid->uuid128[5], uuid->uuid128[4],
            uuid->uuid128[3], uuid->uuid128[2], uuid->uuid128[1], uuid->uuid128[0]
        );
    }
    return uuidStr;
}

/**
 *  @brief Compare a 16-bit UUID with a 128-bit UUID.
 *
 *  @param[in] pUuid16 Pointer to a 16-bit UUID.
 *  @param[in] pUuid128 Pointer to a 128-bit UUID.
 *
 *  @return true if the 16-bit UUID matches the 128-bit UUID, false otherwise.
*/
bool ble_uuid_cmp_uuid16_uuid128(const uint8_t *pUuid16, const uint8_t *pUuid128)
{
    uint8_t attBaseUuid[16] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
            0x00, 0x10, 0x00, 0x00, pUuid16[0], pUuid16[1], 0x00, 0x00 };
    return (memcmp(attBaseUuid, pUuid128, ATT_128_UUID_LEN) == 0);
}

/**
 *  @brief Compare a 16-bit UUID with a UUID of any length.
 *
 *  @param[in] pUuid16 Pointer to a 16-bit UUID.
 *  @param[in] uuidLen Length of the UUID to compare.
 *  @param[in] pUuid Pointer to the UUID to compare.
 *
 *  @return true if the 16-bit UUID matches the UUID of any length, false otherwise.
*/
bool ble_uuid_cmp_uuid16_uuid(const uint8_t *pUuid16, uint8_t uuidLen, const uint8_t *pUuid)
{
    if (uuidLen == ATT_16_UUID_LEN) {
        return ((pUuid16[0] == pUuid[0]) && (pUuid16[1] == pUuid[1]));
    } else {
        return ble_uuid_cmp_uuid16_uuid128(pUuid16, pUuid);
    }
}