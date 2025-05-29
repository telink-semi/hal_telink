
#define GATTS_DISCOVER_CHAR_END         {NULL, NULL}

#define GATTS_DISCOVER_INCLUDED_MAX_COUNT   4

/** < @brief Structure to the parameters of the discovered characteristic. */
struct gatts_discover_char_param {
    uint8_t   service_index;                /** < The index of the service uuid found */
    uint8_t   char_index;         /** < The index of the characteristic uuid found */
    uint16_t  handle;                       /** < The Attribute handle of the characteristic */
    uint16_t *data_length;                  /** < The pointer to store the characteristic data length */
    uint8_t *data;                          /** < The pointer to store the characteristic data */
    uint8_t *CCC_value;                     /** < The pointer to store the CCC value */
    uint16_t  ccc_handle;                   /** < The Attribute handle of the CCC */
};

/** < @brief Callback function type for the discovered characteristic. */
typedef void (*gatts_found_char_callback)(struct gatts_discover_char_param *char_param, void *user_data);

/** < @brief Structure to want discover characteristic information list. */
struct gatts_discover_char_info {
    const struct att_uuid *char_uuid;
    gatts_found_char_callback callback;
};

struct gatts_discover_included_uuid {
    // const
    uint8_t included_size;
    const struct att_uuid *service_uuid;
    const struct gatts_discover_char_info *service_char_list;
    struct {
        const struct att_uuid *incl_uuid;
        void (*found_incl_callback)(uint8_t index, uint16_t start_handle, uint16_t end_handle, void *user_data);
        const struct gatts_discover_char_info *incl_char_list;
    }incl_list[GATTS_DISCOVER_INCLUDED_MAX_COUNT];
};

/**
 *   @brief Discover all characteristics information by service uuid on local device.
 *
 *   @param[in] service_uuid The service uuid to be discovered.
 *   @param[in] char_list The list of characteristic uuids to be discovered.
 *   @param[in] user_data The user data to be passed to the callback function.
 *
 *   @return  BLE_HOST_ERR_SUCC if the operation is successful.
 *              --- BLE_GATT_ERR_NOT_FOUND_SERVICE if the service uuid is not found.
 */
int ble_gatts_discover_by_service_uuid(const struct att_uuid *service_uuid,
    const struct gatts_discover_char_info *char_list, void *user_data);

/**
 *   @brief Discover included service uuid count.
 *
 *   @param[in] service_uuid The service uuid to be discovered.
 *   @param[in] included_uuid The included service uuid to be discovered.
 *
 *   @return  The count of included service uuid.
 */
int ble_gatts_discover_included_uuid(const struct att_uuid *service_uuid, const struct att_uuid *included_uuid);

int ble_gatts_discover_by_service_uuid_with_included_uuid(const struct gatts_discover_included_uuid *included_uuid, void *user_data);
