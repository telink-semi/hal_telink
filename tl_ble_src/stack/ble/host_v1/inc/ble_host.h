#pragma once


/**
 * @brief Bluetooth Host Error Code
 * @defgroup bt_host_err Bluetooth Host Error Code
 *
 * Defines error codes returned by Bluetooth host. If error comes from specific
 * component (eg ATTC, GAP, GATT, SDP or SMP) it is shifted by base allowing to
 * identify component.
 * @{
 */
#define BLE_ERR_CODE_VALUE(err)  ((err) & 0xFF)
#define BLE_CHECK_HCI_ERR(err)   ((err) & BLE_HOST_ERR_HCI_HEAD)
#define BLE_CHECK_GAP_ERR(err)   ((err) & BLE_HOST_ERR_GAP_HEAD)
#define BLE_CHECK_GATT_ERR(err)  ((err) & BLE_HOST_ERR_GATT_HEAD)
#define BLE_CHECK_L2CAP_ERR(err) ((err) & BLE_HOST_ERR_L2CAP_HEAD)
#define BLE_CHECK_SMP_ERR(err)   ((err) & BLE_HOST_ERR_SMP_HEAD)
#define BLE_CHECK_PRF_ERR(err)   ((err) & BLE_HOST_ERR_PRF_HEAD)
#define BLE_CHECK_GATTS_ERR(err) ((err) & BLE_HOST_ERR_GATTS_HEAD)

#define BLE_HCI_ERR(err)         ((err) | BLE_HOST_ERR_HCI_HEAD)
#define BLE_GAP_ERR(err)         ((err) | BLE_HOST_ERR_GAP_HEAD)
#define BLE_GATT_ERR(err)        ((err) | BLE_HOST_ERR_GATT_HEAD)
#define BLE_L2CAP_ERR(err)       ((err) | BLE_HOST_ERR_L2CAP_HEAD)
#define BLE_SMP_ERR(err)         ((err) | BLE_HOST_ERR_SMP_HEAD)
#define BLE_PRF_ERR(err)         ((err) | BLE_HOST_ERR_PRF_HEAD)
#define BLE_GATTS_ERR(err)       ((err) | BLE_HOST_ERR_GATTS_HEAD)

enum ble_host_err_code {
    BLE_HOST_ERR_SUCC = 0,           /** Success */
    BLE_HOST_ERR_NOMEM,              /** Out of memory */
    BLE_HOST_ERR_NOTSUP,             /** Command not supported */
    BLE_HOST_ERR_UNKNOWN,            /** Unknown error */
    BLE_HOST_ERR_BADDATA,            /** Invalid data */
    BLE_HOST_ERR_RETRY,              /** Command can be retried */
    BLE_HOST_ERR_PARM,               /** Invalid parameter */
    BLE_HOST_ERR_NOADDR,             /** No address available */

    BLE_HOST_ERR_HOST_HEAD = 0x0100, /** Host Error code, refer to enum ble_host_error_code */
    BLE_HOST_ERR_HCI_HEAD = 0x200, /** HCI Error code, refer to enum ble_host_hci_error_code */
    BLE_HOST_ERR_GAP_HEAD = 0x300, /** GAP Error code, refer to enum ble_host_gap_error_code */
    BLE_HOST_ERR_GATT_HEAD = 0x400, /** GATT Error code, refer to enum ble_host_gatt_error_code */
    BLE_HOST_ERR_L2CAP_HEAD = 0x500, /** L2CAP Error code, refer to enum ble_host_l2cap_error_code */
    BLE_HOST_ERR_SMP_HEAD = 0x600, /** SMP Error code, refer to enum ble_host_smp_error_code */
    BLE_HOST_ERR_PRF_HEAD = 0x700, /** Profile Error code, refer to enum ble_host_prf_error_code */
    BLE_HOST_ERR_GATTS_HEAD = 0x800,
};

/**
 *   @brief Define the ACL Role in BLE Host.
 */
#define BLE_HOST_ACL_ROLE_CENTRAL 0x00
#define BLE_HOST_ACL_ROLE_PERIPHERAL 0x01

enum {
    BLE_HOST_MALLOC_TYPE = 0x0100,
    BLE_HOST_MALLOC_ACL_MANAGER,
    BLE_HOST_MALLOC_SMP_MANAGER,
};

// todo: move it to the SMP Layer. by yafei.
/* Security Manager (sm_ members) is configurable at runtime */
struct security_mng_cfg {
    uint8_t sm_sec_lvl; //configured security level, refer to enum smp_security_level
    uint8_t sm_our_key_dist;  //smp our Key Distribution Mask */
    uint8_t sm_peer_key_dist;  //smp peer Key Distribution Mask */
    uint8_t sm_io_capability : 3; //IO capability, 0:display only, 1:display yes/no, 2:keyboard only, 3:no input no output, 4:keyboard display
    uint8_t sm_min_key_size : 5; //minimum key size, 7-16 bytes

    uint8_t sm_sc : 1; //1:secure connection, 0:legacy pairing
    uint8_t sm_bonding : 1; //1:bonding, 0:no bonding
    uint8_t sm_mitm : 1; //1:man in the middle protection, 0:no protection
    uint8_t sm_oob : 1; //1:out of band data, 0:no oob data
    uint8_t sm_keypress : 1; //1:keypress notification, 0:no keypress notification
    uint8_t sm_sc_debug_mode : 1; //1:debug_mode,0:ecdh public/private key pairs distribute
    uint8_t sm_re_cfged : 1;

    uint32_t sm_pke_dft_pincode; //when using the PKE, set the default pincode displayed by our side.
};

/* HCI information, shared between host and controller, and used by all Host layers. */
struct ble_host_info {
    uint8_t hs_hci_version;
    uint8_t hs_hci_max_pkts;

    /* controller acl data buffer size, in bytes */
    uint16_t hs_hci_acl_avail_pkts;
    /* controller acl data buffer size, in bytes */
    uint16_t hs_hci_acl_buf_sz;
    /* controller iso data buffer size, in bytes */
    uint16_t hs_hci_iso_avail_pkts;
    /* controller iso data buffer size, in bytes */
    uint16_t hs_hci_iso_buf_sz;

    // check controller support LE
    bool le_supported_controller;
    // Controller supported Commands. defined in Information parameters.
    // uint8_t supported_commands[64];

    // le supported features. 
    uint64_t le_supported_feature;

    uint8_t *hs_mpool;

    /* identity address */
    uint8_t hs_id_pub[6];
    uint8_t hs_id_rnd[6];

    // remove it ?
    /* Security Manager (sm_ members) is configurable at runtime */
    struct security_mng_cfg sm_dft_settings; //all ACL connection used security manager configuration
};

/** Structure representing a BLE address. */
struct ble_addr {
    /** The type of the address. */
    uint8_t type;

    /** The value of the address as an array of 6 bytes. */
    uint8_t val[6];
};

#define BLE_ADDR_EMPTY (&(struct ble_addr) { 0, {0, 0, 0, 0, 0, 0}})

// can not modify this value, it is used in the stack.
enum ble_host_user_data_id {
    BLE_HOST_L2CAP_USER_ID,
    BLE_HOST_ACL_DATA_TX_USER_ID,
    BLE_HOST_GATT_USER_ID,
    BLE_HOST_READER_USER_ID,
    BLE_HOST_PROFILE_USER_ID,
    BLE_HOST_APP_DATA1_USER_ID,
    BLE_HOST_APP_DATA2_USER_ID,
    BLE_HOST_USER_DATA_ID_MAX,
};

struct ble_host_conn {
    uint16_t conn_handle; //Connection handle
    uint8_t  role; //Connected role, 0x00 is central, 0x01 is peripheral
    uint8_t  master_clock_acc; //MCA: 0x00: 500ppm, 0x01: 250ppm, 0x02: 150ppm, 0x03: 100ppm 0x04: 75ppm, 0x05: 50ppm, 0x06: 30ppm, 0x07: 20ppm
    uint16_t conn_interval; //connection interval , range 0x0006 to 0x0C80, unit 1.25ms
    uint16_t conn_latency; //connection latency , range 0x0000 to 0x01F3
    uint16_t supervision_timeout; //supervision timeout, range 0x000A to 0x0C80, unit 10ms
    uint16_t cfc_outstanding_pkts; //Controller flow control used
    uint16_t hfc_completed_pkts; //Host flow control used
    /*GAP using start*/
    uint32_t supported_feat; //supported features
    uint32_t remote_feat; //remote device features
    uint8_t version; //bluetooth version
    uint8_t remote_version; //remote device bluetooth version
    uint8_t reader_status; //reader status
    /*GAP using end*/
    /* */
    void *user_data[BLE_HOST_USER_DATA_ID_MAX];


    /* address type: 0:public, 1:random, 2:public identity, 3:random identity, this value is set by the host.
     * for Central role, this value is set by Create Connection or Create extended connection,
     * for Peripheral role, this value is set by the Set Legacy Advertising or Set Extended Advertising */
    uint8_t own_addr_type;
    /* Only used for Peripheral role, if use extended advertising, each connection can have different random address, in
     * this case, we need to store the random address of each connection. */
    uint8_t own_rnd_addr[6]; //TODO: code need to manage this value. maybe in GAP layer

    /* peer or rpa address received (or convert) from the connection complete event */
    struct ble_addr peer_addr; //peer address received from the connection complete event
    struct ble_addr own_rpa_addr; //local rpa address received from the connection complete event
    struct ble_addr peer_rpa_addr; //peer rpa address received from the connection complete event

    /* own and peer address, id, ota address */
    struct ble_addr own_id_addr;  //our identity address, updated after connection complete event
    struct ble_addr peer_id_addr; //peer identity address, updated after connection complete event
    struct ble_addr own_ota_addr; //own address over the air, updated after connection complete event
    struct ble_addr peer_ota_addr; //peer address over the air, updated after connection complete event

    /* security state of the connection */
    struct security_state {
        uint8_t encrypted : 1;  // If connection is encrypted
        uint8_t authenticated : 1; //If connection is authenticated 
        uint8_t bonded : 1; //If connection is bonded (security information is stored) 
        uint8_t key_size : 5; //Size of a key used for encryption
        uint8_t sec_level; //security level, 0:no security, 1:unauthenticated, 2:authenticated, 3:secure connection
    } sec_state;

    /* Security Manager (sm_ members) is configurable at runtime */
    struct security_mng_cfg sm_settings;

    /** XXX: Add more connection-specific state here. */
    // must remove this later.
    uint32_t conn_created_ticks;
    uint8_t  encryption_busy;
};

struct ble_host_acl_conn_callbacks {
    void (*connected)(struct ble_host_conn *conn);
    void (*disconnected)(struct ble_host_conn *conn, uint8_t reason);
};

/**
 *   @brief this function is used to initialize the BLE Host stack.
 *
 *   @param[in] p_host_memory: pointer to the Host memory pool.
 *   @param[in] size: size of the Host memory pool.
 *
 *   @return none.
 */
void ble_host_init(uint8_t *p_host_memory, uint32_t size);

/***************  */

/**
 *   @brief this function is used to find a connection by connection handle.
 *
 *   @param[in] conn_handle: connection handle.
 *
 *   @return pointer to the connection structure, or NULL if not found.
 */
struct ble_host_conn *ble_host_conn_find_by_conn_handle(uint16_t conn_handle);

/**
 *   @brief this function is used to check connection handle is connected or not.
 *
 *   @param[in] conn_handle: connection handle.
 *
 *   @return true if connected, false otherwise.
 */
bool ble_host_conn_is_connected(uint16_t conn_handle);

/**
 *   @brief this function is used to check connection handle is peripheral or not.
 *
 *   @param[in] conn_handle: connection handle.
 *
 *   @return true if peripheral, false otherwise.
 */
bool ble_host_conn_is_peripheral(uint16_t conn_handle);

/**
 *   @brief this function is used to check connection handle is central or not.
 *
 *   @param[in] conn_handle: connection handle.
 *
 *   @return true if central, false otherwise.
 */
bool ble_host_conn_is_central(uint16_t conn_handle);

/**
 *   @brief this function is used to register the callbacks for ACL connection events(BLE Host).
 *
 *   @param[in] id: Host ACL connection user data ID.
 *   @param[in] cb: pointer to the callback structure.
 *
 *   @return none.
 */
void ble_host_acl_conn_register_user_data(enum ble_host_user_data_id id, const struct ble_host_acl_conn_callbacks *cb);

/**
 *   @brief this function is used to register the callbacks for ACL connection events(BLE Host) by index.
 *
 *   @param[in] id: Host ACL connection user data ID.
 *
 *   @return none.
 */
void ble_host_acl_conn_unregister_user_data(enum ble_host_user_data_id id);


// delete later.
void ble_host_v1_init(const uint8_t *mac, const uint8_t *mac_random);

uint8_t  ble_host_hci_get_hci_version(void);
uint16_t ble_host_hci_max_acl_payload_sz(void);
uint16_t ble_host_hci_max_iso_payload_sz(void);
uint32_t ble_host_hci_get_hci_supported_cmd(void);
bool ble_host_hci_get_le_supported_controller(void);

void ble_host_hci_add_acl_avail_pkts(int16_t delta);
void ble_host_hci_add_iso_avail_pkts(int16_t delta);
void ble_host_hci_get_bd_addr(uint8_t *addr);
void ble_host_hci_set_hci_supported_cmd(uint32_t sup_cmd);

void ble_host_hci_read_controller_basic_info(void);
void ble_host_hci_set_le_addr(const uint8_t *mac);
