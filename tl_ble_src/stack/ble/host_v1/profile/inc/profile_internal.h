
/** < profile support max acl connection */
#define PRF_CENTRAL_MAX_COUNT               4
#define PRF_PERIPHERAL_MAX_COUNT            4

#define PRF_CONNECT_MAX_COUNT               (PRF_CENTRAL_MAX_COUNT + PRF_PERIPHERAL_MAX_COUNT)

/** < profile malloc type && client/server malloc type */
enum {
    BLE_PRF_MALLOC_TYPE = 0x100,
    BLE_PRF_CONN_INFO_TYPE,
    BLE_PRF_MALLOC_SPEC_PRF_SERVER_START = 0x1000,
    BLE_PRF_MALLOC_SPEC_PRF_CLIENT_START = 0x2000,
};

#define BLE_PRF_MALLOC_SPEC_PRF_SERVER(size, service_id) \
            ble_prf_malloc(size, BLE_PRF_MALLOC_SPEC_PRF_SERVER_START + service_id)

#define BLE_PRF_MALLOC_SPEC_PRF_CLIENT(size, service_id) \
            ble_prf_malloc(size, BLE_PRF_MALLOC_SPEC_PRF_CLIENT_START + service_id)

/**
 *   @brief  Function to allocate memory for a profile.
 *
 *   @param[in]  size    Size of memory to allocate.
 *   @param[in]  type_id Type ID of the profile.
 *
 *   @return Pointer to the allocated memory.
 */
void *ble_prf_malloc(uint32_t size, uint16_t type_id);

/**
 *   @brief  Function to free memory allocated for a profile.
 *
 *   @param[in]  ptr Pointer to the memory to free.
 *
 *   @return None.
 */
void ble_prf_free(void *ptr);

#define CHECK_PTR_NUL(ptr)       (ptr == NULL)

#define BLE_PRF_CHECK_NULL_PTR1(ptr1) \
    if (CHECK_PTR_NUL(ptr1))   {       \
        return BLE_PRF_ERR(BLE_PRF_ERR_INPUT_NULL);   \
    }

#define BLE_PRF_CHECK_NULL_PTR2(ptr1, ptr2)         \
    if (CHECK_PTR_NUL(ptr1) || CHECK_PTR_NUL(ptr2)) {\
        return BLE_PRF_ERR(BLE_PRF_ERR_INPUT_NULL);   \
    }

#define BLE_PRF_CHECK_NULL_PTR3(ptr1, ptr2, ptr3)                          \
    if (CHECK_PTR_NUL(ptr1) || CHECK_PTR_NUL(ptr2) || CHECK_PTR_NUL(ptr3)) {\
        return BLE_PRF_ERR(BLE_PRF_ERR_INPUT_NULL);   \
    }

#define BLE_PRF_CHECK_NULL_PTR(...) VARARG(BLE_PRF_CHECK_NULL_PTR, __VA_ARGS__)

/** < server common functions marco */
#define BLE_FUNC_NAME_INIT_CHAR(prf, characteristic) ble_##prf##s_##characteristic##_init

#define BLE_PRF_SERVER_INIT_HANDLE(prf, PRF, characteristic)                                   \
    static void BLE_FUNC_NAME_INIT_CHAR(prf, characteristic)(struct gatts_discover_char_param *char_param, void *user_data) \
    {                                                                                          \
        struct ble_##prf##_server *server = (struct ble_##prf##_server *)user_data;                \
        if (char_param->char_index > 0) {                                                                      \
            BLE_##PRF##_WARN(""#characteristic " char too many");                         \
            return;                                                                            \
        }                                                                          \
        server->characteristic##_handle = char_param->handle;                                           \
    }

#define BLE_PRF_SERVER_INIT_HANDLE_WITH_CCC(prf, PRF, characteristic)                                   \
    static void BLE_FUNC_NAME_INIT_CHAR(prf, characteristic)(struct gatts_discover_char_param *char_param, void *user_data) \
    {                                                                                          \
        struct ble_##prf##_server *server = (struct ble_##prf##_server *)user_data;                \
        if (char_param->char_index > 0) {                                                                      \
            BLE_##PRF##_WARN(""#characteristic " char too many");                         \
            return;                                                                            \
        }                                                                          \
        server->characteristic##_handle = char_param->handle;                                           \
        server->characteristic##_ccc_handle = char_param->ccc_handle;                                           \
}

#define BLE_PRF_SERVER_FIND_CHAR(prf, characteristic, uuid)     \
    {                                                           \
        .char_uuid = &uuid,                                      \
        .callback  = BLE_FUNC_NAME_INIT_CHAR(prf, characteristic), \
    }

/** < client common functions marco */
#define BLE_FUNC_DEFINE_INIT(prf)           static void ble_##prf##c_init(enum prf_process_type type, const void *param)
#define BLE_FUNC_DEFINE_CONNECT(prf)        static void ble_##prf##c_connect(uint16_t conn_handle, enum prf_acl_state_change state)
#define BLE_FUNC_DEFINE_DISCOVERY(prf)      static void ble_##prf##c_discovery(uint16_t conn_handle, enum prf_disc_type type)

#define BLE_FUNC_PRF_CLIENT_INIT(prf, PRF)     BLE_FUNC_DEFINE_INIT(prf) \
    { \
        if (type == PRF_PROCESS_INIT) { \
            BLE_##PRF##_DEBUG("Client initialization"); \
            const struct ble_##prf##c_register_param *p_reg_param = param;  \
            if (p_reg_param != NULL && p_reg_param->event_callback != NULL) {   \
                s_##prf##_client_ctrl.prf_process.event_cb = p_reg_param->event_callback;   \
            }   \
        } else if (type == PRF_PROCESS_DEINIT) {    \
            BLE_##PRF##_DEBUG("Client deinitialization");   \
        }   \
        \
        for (int i = 0; i < BLE_##PRF##_CLIENT_SUPPORT_MAX_COUNT; i++) {\
            if (s_##prf##_client_ctrl.p_##prf##_client[i] != NULL) {   \
                BLE_##PRF##_CLIENT_FREE(s_##prf##_client_ctrl.p_##prf##_client[i]);   \
            }   \
            s_##prf##_client_ctrl.p_##prf##_client[i] = NULL;   \
        }   \
    }

#define BLE_FUNC_PRF_CLIENT_CONNECT(prf, PRF)      BLE_FUNC_DEFINE_CONNECT(prf)    \
    {   \
        BLE_##PRF##_DEBUG("acl:0x%03x, state:%s", conn_handle, state == PRF_ACL_CONNECT ? "connected" : "disconnected");    \
        if (state == PRF_ACL_DISCONNECT) { \
            for (int i = 0; i < BLE_##PRF##_CLIENT_SUPPORT_MAX_COUNT; i++) { \
                if (s_##prf##_client_ctrl.p_##prf##_client[i] != NULL && \
                    s_##prf##_client_ctrl.p_##prf##_client[i]->conn_handle == conn_handle) { \
                    BLE_##PRF##_CLIENT_FREE(s_##prf##_client_ctrl.p_##prf##_client[i]); \
                    s_##prf##_client_ctrl.p_##prf##_client[i] = NULL; \
                    break; \
                }   \
            }   \
        }   \
}

#define BLE_FUNC_PRF_CLIENT_GET_CONTEXT(prf, PRF)        \
    static struct ble_##prf##_client *ble_##prf##c_get_client_context(uint16_t conn_handle) \
    {                                               \
        for (int i = 0; i < BLE_##PRF##_CLIENT_SUPPORT_MAX_COUNT; i++) { \
            if (s_##prf##_client_ctrl.p_##prf##_client[i] != NULL && \
                s_##prf##_client_ctrl.p_##prf##_client[i]->conn_handle == conn_handle) { \
                return s_##prf##_client_ctrl.p_##prf##_client[i]; \
            } \
        } \
        return NULL; \
    }

#define BLE_FUNC_PRF_CLIENT_CREATE_CONTEXT(prf, PRF)        \
    static struct ble_##prf##_client *ble_##prf##c_create_client_context(uint16_t conn_handle) \
    {                                               \
        for (int i = 0; i < BLE_##PRF##_CLIENT_SUPPORT_MAX_COUNT; i++) { \
            if (s_##prf##_client_ctrl.p_##prf##_client[i] == NULL) { \
                struct ble_##prf##_client *p_##prf##_client = BLE_##PRF##_CLIENT_MALLOC(sizeof(struct ble_##prf##_client)); \
                if (p_##prf##_client == NULL) { \
                    return NULL; \
                } \
                memset(p_##prf##_client, 0, sizeof(struct ble_##prf##_client)); \
                p_##prf##_client->conn_handle = conn_handle; \
                s_##prf##_client_ctrl.p_##prf##_client[i] = p_##prf##_client; \
                return p_##prf##_client; \
            } \
        } \
        return NULL; \
    }

#define BLE_FUNC_PRF_DISC_TYPE_SVC_ONLY(prf)        \
    BLE_FUNC_DEFINE_DISCOVERY(prf){     \
        struct ble_prf_disc_svc_param disc_svc_param;   \
        memset(&disc_svc_param, 0, sizeof(struct ble_prf_disc_svc_param));  \
        disc_svc_param.included = false;  \
        disc_svc_param.disc_list = &s_disc_##prf;  \
        ble_prf_discovery_common(conn_handle, type, &disc_svc_param);  \
    }

#define BLE_FUNC_DEFINE_INIT_CONNECT_DISC(prf)  \
    BLE_FUNC_DEFINE_INIT(prf);  \
    BLE_FUNC_DEFINE_CONNECT(prf);   \
    BLE_FUNC_DEFINE_DISCOVERY(prf);    \
    BLE_SSDP_LIST_NAME(prf);


#define BLE_FUNC_PRF_CLIENT_INIT_CONNECT_DISC(prf, PRF)     \
    BLE_FUNC_PRF_CLIENT_INIT(prf, PRF)      \
    BLE_FUNC_PRF_CLIENT_CONNECT(prf, PRF)   \
    BLE_FUNC_PRF_DISC_TYPE_SVC_ONLY(prf)    \
    BLE_FUNC_PRF_CLIENT_GET_CONTEXT(prf, PRF)   \
    BLE_FUNC_PRF_CLIENT_CREATE_CONTEXT(prf, PRF)

#define BLE_FUNC_NAME_DISC_FOUND_CHAR(prf, characteristic) ble_##prf##c_##characteristic##_found_char
#define BLE_FUNC_NAME_DISC_START_READ(prf, characteristic) ble_##prf##c_##characteristic##_start_read
#define BLE_FUNC_NAME_START_READ(prf, characteristic)      ble_##prf##c_##characteristic##_start_read

#define BLE_FUNC_NAME_FOUND_SERVICE(prf)    ble_##prf##c_found_service

#define BLE_SSDP_CHARACTERISTIC_NAME(prf) static const struct gatt_ssdp_characteristic s_disc_##prf##_char[]

#define BLE_SSDP_LIST_NAME(prf)     static const struct ble_gatt_ssdp_no_include_list s_disc_##prf

#define BLE_PRF_DEFINE_SSDP_NO_INCLUDE_LIST(prf, ser_uuid)      \
        BLE_SSDP_LIST_NAME(prf) = {    \
            .max_service_count = 1,   \
            .service = {              \
                .service_uuid = &ser_uuid, \
                .service_callback = ble_##prf##c_found_service, \
             }, \
            .characteristic_table = { \
                .characteristic_size = ARRAY_SIZE(s_disc_##prf##_char), \
                .characteristic = s_disc_##prf##_char, \
             } \
        }

#define BLE_FUNC_PRF_SDP_DISCOVERY_SERVICE(prf, PRF) \
    static void BLE_FUNC_NAME_FOUND_SERVICE(prf)(uint16_t conn_handle, uint8_t count, uint16_t start_handle, uint16_t end_handle) \
    { \
        if (count == GATT_SSDP_SERVICE_NOT_FOUND) { \
            BLE_##PRF##_WARN("Service not found"); \
            ble_prf_report_sdp_not_found_event(conn_handle, SERVICE_ID_##PRF, #PRF); \
        } else if (count == GATT_SSDP_SERVICE_FOUND_END) { \
            BLE_##PRF##_INFO("Service found end"); \
            ble_prf_report_sdp_found_end_event(conn_handle, SERVICE_ID_##PRF, #PRF); \
            BLE_PRF_GET_CLIENT_INST(conn_handle, prf); \
            if (p_client != NULL) { \
                ble_##prf##c_display_information(p_client); \
                ble_host_gattc_add_subscribe_ccc_message(conn_handle, &p_client->ccc_msg); \
            } \
        } else if (count == 1) { \
            BLE_##PRF##_INFO("Service found, acl:0x%03x, start_handle:0x%04x, end_handle:0x%04x", conn_handle, start_handle, end_handle); \
            ble_prf_report_sdp_found_event(conn_handle, SERVICE_ID_##PRF, #PRF, start_handle, end_handle); \
            struct ble_##prf##_client *p_client = ble_##prf##c_create_client_context(conn_handle); \
            if (p_client == NULL) { \
                BLE_##PRF##_ERROR("Failed to create client context");   \
            } else {    \
                p_client->ccc_msg.start_handle = start_handle;  \
                p_client->ccc_msg.end_handle = end_handle;  \
                p_client->ccc_msg.report_callback = ble_##prf##c_data_input;    \
            }\
        } \
    }

// client characteristic discovery
#define BLE_PRF_DISC_READ_NOTIFY_CHAR(prf, characteristicUuid, characteristic) \
    {           \
        .subscribe_notify = true,                 \
        .read_value = true,                     \
        .characteristic_uuid = &characteristicUuid,      \
        .characteristic_callback = BLE_FUNC_NAME_DISC_FOUND_CHAR(prf, characteristic),  \
        .read_value_callback = BLE_FUNC_NAME_DISC_START_READ(prf, characteristic),     \
    }

#define BLE_PRF_DISC_READ_CHAR(prf, characteristicUuid, characteristic) \
    {           \
        .read_value = true,                     \
        .characteristic_uuid = &characteristicUuid,      \
        .characteristic_callback = BLE_FUNC_NAME_DISC_FOUND_CHAR(prf, characteristic),  \
        .read_value_callback = BLE_FUNC_NAME_DISC_START_READ(prf, characteristic),     \
    }

#define BLE_PRF_DISC_NOTIFY_CHAR(prf, characteristicUuid, characteristic) \
    {           \
        .subscribe_notify = true,                 \
        .characteristic_uuid = &characteristicUuid,      \
        .characteristic_callback = BLE_FUNC_NAME_DISC_FOUND_CHAR(prf, characteristic),  \
    }

#define BLE_FUNC_NAME_GET_CLIENT_INST(prf)          ble_##prf##c_get_client_context
#define BLE_PRF_GET_CLIENT_INST(conn_handle, prf)   struct ble_##prf##_client *p_client = BLE_FUNC_NAME_GET_CLIENT_INST(prf)(conn_handle)

#define BLE_PRF_DISC_FOUND_CHAR(prf, PRF, characteristic)         \
    BLE_PRF_GET_CLIENT_INST(conn_handle, prf); \
    if (p_client != NULL) {                                           \
        p_client->characteristic##_handle = value_handle;              \
    }                                                     \
    BLE_##PRF##_INFO(#characteristic " found acl:0x%03x properties:0x%03x hdl:0x%04x ", conn_handle, properties, value_handle)

#define BLE_FUNC_PRF_DISC_FOUND_CHAR(prf, PRF, characteristic)                                                     \
    static void BLE_FUNC_NAME_DISC_FOUND_CHAR(prf, characteristic)(uint16_t conn_handle, uint8_t service_count, uint8_t properties, uint16_t value_handle) \
    {                                                                                                                     \
        (void)service_count;                                                                                               \
        BLE_PRF_DISC_FOUND_CHAR(prf, PRF, characteristic);                                                           \
    }

#define BLE_FUNC_START_READ_COMMON(prf, characteristic) \
    static void BLE_FUNC_NAME_START_READ(prf, characteristic)(uint16_t conn_handle, uint16_t value_handle, uint8_t **read, uint16_t **read_len, uint16_t *read_max_size)

#define BLE_FUNC_PRF_DISC_START_READ_FIX_LEN(prf, PRF, characteristic) \
    BLE_FUNC_START_READ_COMMON(prf, characteristic)           \
    {                                                                         \
        (void)value_handle;                                                     \
        (void)read_len;                                                       \
        BLE_PRF_GET_CLIENT_INST(conn_handle, prf);                              \
        if(p_client != NULL){   \
            *read = (uint8_t *) &p_client->characteristic;                      \
            *read_max_size = sizeof(p_client->characteristic);                    \
        }   \
    }

#define BLE_FUNC_PRF_DISC_START_READ(prf, PRF, characteristic) \
    BLE_FUNC_START_READ_COMMON(prf, characteristic)           \
    {                                                                         \
        (void)value_handle;                                                     \
        BLE_PRF_GET_CLIENT_INST(conn_handle, prf);                              \
        if(p_client != NULL){   \
            *read = (uint8_t *) &p_client->characteristic;                      \
            *read_len = &p_client->characteristic##_len;                         \
            *read_max_size = sizeof(p_client->characteristic);                    \
        }   \
    }


#define BLE_PRF_CHECK_CONN_HANDLE(prf)                                  \
    BLE_PRF_GET_CLIENT_INST(conn_handle, prf);           \
    if (p_client == NULL) {     \
        return BLE_PRF_ERR(BLE_PRF_ERR_INVALID_CONN_HANDLE);\
    }

#define BLE_PRF_READ_ATTR_VALUE_FIX_LEN(prf, charName) \
    BLE_PRF_CHECK_CONN_HANDLE(prf)                   \
    struct ble_prf_read_value prf_read_param = {        \
        .data = (uint8_t *) &p_client->charName,   \
        .length = NULL, \
        .max_data_len = sizeof(p_client->charName),    \
        .callback = callback,   \
    };  \
    return ble_prf_read_attribute_value(conn_handle, p_client->charName##_handle, &prf_read_param)

#define BLE_PRF_GET_ATTR_VALUE_COMMON(characteristic)                      \
    if (p_client->characteristic##_handle == 0){                     \
        return BLE_PRF_ERR(BLE_PRF_ERR_GET_ATTR_VALUE_NOT_FOUND);   \
    }

#define BLE_PRF_GET_ATTR_VALUE_FIX_LEN(prf, characteristic)                          \
    BLE_PRF_CHECK_NULL_PTR(characteristic)                                          \
    BLE_PRF_CHECK_CONN_HANDLE(prf)                       \
    BLE_PRF_GET_ATTR_VALUE_COMMON(characteristic)                              \
    memcpy(characteristic, &p_client->characteristic, sizeof(p_client->characteristic)); \
                                                                                     \
    return BLE_HOST_ERR_SUCC


struct ble_prf_disc_svc_param {
    bool included;
    const void *disc_list;
};

void ble_prf_discovery_common(uint16_t conn_handle, enum prf_disc_type type, struct ble_prf_disc_svc_param *param);
