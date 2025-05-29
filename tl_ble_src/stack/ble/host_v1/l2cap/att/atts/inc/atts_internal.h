
uint16_t ble_atts_find_uuid_in_range(uint16_t connHandle, uint16_t startHandle, uint16_t endHandle,
    uint8_t uuidLen, const uint8_t *pUuid, const struct atts_attribute **pAttr, struct atts_group **pAttrGroup);

uint8_t ble_atts_check_permissions(uint16_t connHandle, uint8_t permit, uint8_t handle, uint8_t permissions);

uint16_t ble_atts_find_service_group_end_handle(uint16_t connHandle, uint16_t startHandle);

uint16_t ble_atts_get_attribute_range_of_handle(uint16_t connHandle, uint16_t startHandle, uint16_t endHandle,
    const struct atts_attribute **pAttr);

const struct atts_attribute *ble_atts_get_service_group_by_handle(uint16_t connHandle, uint16_t handle,
    struct atts_group **pAttrGroup);
