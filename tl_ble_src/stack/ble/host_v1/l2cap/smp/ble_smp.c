#include <string.h>

#include "common/types.h"
#include "common/utility.h"

#include "../../inc/ble_host.h"
#include "../../inc/ble_host_internal.h"
#include "../../inc/ble_host_sal.h"

#include "../inc/ble_l2cap.h"
#include "../inc/ble_l2cap_log.h"

#include "inc/ble_smp.h"
#include "inc/ble_smp_store.h"

#include "../../hci/inc/ble_hci_cmd.h"
#include "../../hci/le_cmd/inc/hci_cmd_le_misc.h"
#include "../../hci/cmd/inc/hci_cmd_link_ctrl.h"

int ble_host_smp_send_data_sync(struct ble_host_conn *conn, const uint8_t *p_data, uint16_t data_len)
{
    struct ble_host_l2cap_tx_packet tx_packet = {
        .channel_id = LE_L2CAP_CID_SMP,
        .data_length = data_len,
        .p_data = p_data,
        .tx_complete_cb = NULL,
        .cb_arg = NULL,
    };

    return ble_host_l2cap_send_l2cap_data_sync(conn, &tx_packet);
}

#if 1
#include <sys/queue.h>

//old gap event design
#include "stack/ble/host/gap/gap.h"
#include "stack/ble/host/gap/gap_event.h"
#include "../../hci/inc/ble_hci_evt.h"

//TODO: e and ECDH use hci_cmd_xxx
#include "algorithm/algorithm.h"
#include "algorithm/crypto/crypto_alg.h"

extern u32 gap_eventMask; //TODO:
extern int blc_gap_send_event(u32 h, u8 *para, int n); //TODO:
/**
 * @brief Define the SMP pairing key distribution start and end.
 */
#define BLE_SMP_DISTRIBUTE_KEY_START                BLE_SM_OP_ENC_INFO
#define BLE_SMP_DISTRIBUTE_KEY_END                  0

/**
 * @brief Define the SMP pairing phase.
 */
#define BLE_SMP_PAIRING_PHASE_IDLE                  0x00UL
#define BLE_SMP_PAIRING_PHASE_1_OK                  0x01UL
#define BLE_SMP_PAIRING_PHASE_2_ENC                 0x02UL
#define BLE_SMP_PAIRING_PHASE_2_OK                  0x03UL

/**
 * @brief Define the SMP key mask.
 */
#define BLE_SMP_KEY_MASK_IDLE                       0
#define BLE_SMP_KEY_MASK_ENC                        BIT(0)
#define BLE_SMP_KEY_MASK_IDENTITY                   BIT(1)

#if (0) //Not used now TODO
    #define PASSKEY_TYPE_ENTRY_STARTED   0x00
    #define PASSKEY_TYPE_DIGIT_ENTERED   0x01
    #define PASSKEY_TYPE_DIGIT_ERASED    0x02
    #define PASSKEY_TYPE_CLEARED         0x03
    #define PASSKEY_TYPE_ENTRY_COMPLETED 0x04
#endif

/** 
 * @brief  Define the SMP pairing TK state.
 */
#define BLE_SMP_TK_STATE_IDLE                       0
#define BLE_SMP_TK_STATE_REQUEST                    BIT(0)
#define BLE_SMP_TK_STATE_UPDATE                     BIT(1)
#define BLE_SMP_TK_STATE_CONFIRM_PENDING            BIT(2)
#define BLE_SMP_TK_STATE_NC                         BIT(3)
#define BLE_SMP_TK_STATE_NC_CHECK_YES               BIT(4)
#define BLE_SMP_TK_STATE_NC_CHECK_NO                BIT(5)
#define BLE_SMP_TK_STATE_NC_DHKEY_FAIL_PENDING      BIT(6)
#define BLE_SMP_TK_STATE_NC_DHKEY_SUCC_PENDING      BIT(7)

/**
 * @brief ACL connection role, Connected role, 0x00 is central, 0x01 is peripheral
 * @note    opcodes role check used in ble_sm_cmd_rx_dispatch_find
 */
#define BLE_SMP_CONN_ROLE_PERIPHERAL                BIT(BLE_HCI_CONN_ROLE_PERIPHERAL)
#define BLE_SMP_CONN_ROLE_CENTRAL                   BIT(BLE_HCI_CONN_ROLE_CENTRAL)
#define BLE_SMP_CONN_ROLE_BOTH                      (BLE_SMP_CONN_ROLE_PERIPHERAL | BLE_SMP_CONN_ROLE_CENTRAL)

/* */
struct smp_initiate_cfg g_smp_initiate_cfg = {
    /* Default security request configuration */
    .peripheral.new_conn_cfg = SEC_REQ_IMM_SEND,
    .peripheral.re_conn_cfg = SEC_REQ_PEND_SEND,
    .peripheral.pending_send_ms = 1000,
    /* Default pairing request configuration */
    .central.new_conn_cfg = PAIR_REQ_AUTO_SEND,
    .central.re_conn_cfg = PAIR_REQ_AUTO_SEND,
    .central.manual_smp_start = false,
};

/* TODO: add API to set/get this configuration */

static int ble_sm_pair_req_tx(struct ble_host_conn *conn);
static int ble_sm_pair_rsp_tx(struct ble_host_conn *conn);
static int ble_sm_pair_confirm_tx(struct ble_host_conn *conn);
static int ble_sm_pair_confirm_tx(struct ble_host_conn *conn);
static int ble_sm_pair_random_tx(struct ble_host_conn *conn);
static int ble_sm_pair_failed_tx(struct ble_host_conn *conn, uint8_t reason);
static int ble_sm_enc_info_tx(struct ble_host_conn *conn);
static int ble_sm_master_id_tx(struct ble_host_conn *conn);
static int ble_sm_id_info_tx(struct ble_host_conn *conn);
static int ble_sm_id_addr_info_tx(struct ble_host_conn *conn);
static int ble_sm_sign_info_tx(struct ble_host_conn *conn);
static int ble_sm_sec_request_tx(struct ble_host_conn *conn);
static int ble_sm_public_key_tx(struct ble_host_conn *conn);
static int ble_sm_dhkey_check_tx(struct ble_host_conn *conn);
static int ble_sm_keypress_tx(struct ble_host_conn *conn, enum smp_notification_type ntf_type);

/* SMP command processing functions */
typedef int ble_sm_rx_fn (struct ble_host_conn *, struct smp_cmd_fmt *);

/** Dispatch table for incoming SMP command.  Sorted by opcode. */
struct smp_cmd_rx_proc_t {
    uint8_t role; //refer to MICROS of BLE_SMP_CONN_ROLE_XXX
    uint8_t expect_len;
    ble_sm_rx_fn *rx_cb;
} ;

static ble_sm_rx_fn ble_sm_pair_req_rx;
static ble_sm_rx_fn ble_sm_pair_rsp_rx;
static ble_sm_rx_fn ble_sm_confirm_rx;
static ble_sm_rx_fn ble_sm_random_rx;
static ble_sm_rx_fn ble_sm_fail_rx;
static ble_sm_rx_fn ble_sm_enc_info_rx;
static ble_sm_rx_fn ble_sm_master_id_rx;
static ble_sm_rx_fn ble_sm_id_info_rx;
static ble_sm_rx_fn ble_sm_id_addr_info_rx;
static ble_sm_rx_fn ble_sm_sign_info_rx;
static ble_sm_rx_fn ble_sm_sec_req_rx;
static ble_sm_rx_fn ble_sm_sc_public_key_rx;
static ble_sm_rx_fn ble_sm_sc_dhkey_check_rx;
static ble_sm_rx_fn ble_sm_sc_keypress_rx;

struct smp_cmd_rx_proc_t const ble_sm_cmd_rx_dispatch[] = {
    [BLE_SM_OP_PAIR_REQ]             = { BLE_SMP_CONN_ROLE_PERIPHERAL,      sizeof(struct ble_smp_pairing_cmd),            &ble_sm_pair_req_rx       },
    [BLE_SM_OP_PAIR_RSP]             = { BLE_SMP_CONN_ROLE_CENTRAL,         sizeof(struct ble_smp_pairing_cmd),            &ble_sm_pair_rsp_rx       },
    [BLE_SM_OP_PAIR_CFM]             = { BLE_SMP_CONN_ROLE_BOTH,            sizeof(struct ble_smp_pairing_confirm),        &ble_sm_confirm_rx        },
    [BLE_SM_OP_PAIR_RANDOM]          = { BLE_SMP_CONN_ROLE_BOTH,            sizeof(struct ble_smp_pairing_random),         &ble_sm_random_rx         },
    [BLE_SM_OP_PAIR_FAIL]            = { BLE_SMP_CONN_ROLE_BOTH,            sizeof(struct ble_smp_pairing_failed),         &ble_sm_fail_rx           },
    [BLE_SM_OP_ENC_INFO]             = { BLE_SMP_CONN_ROLE_BOTH,            sizeof(struct ble_smp_encryption_info),        &ble_sm_enc_info_rx       },
    [BLE_SM_OP_MASTER_ID]            = { BLE_SMP_CONN_ROLE_BOTH,            sizeof(struct ble_smp_central_id),             &ble_sm_master_id_rx      },
    [BLE_SM_OP_ID_INFO]              = { BLE_SMP_CONN_ROLE_BOTH,            sizeof(struct ble_smp_id_info),                &ble_sm_id_info_rx        },
    [BLE_SM_OP_ID_ADDR_INFO]         = { BLE_SMP_CONN_ROLE_BOTH,            sizeof(struct ble_smp_id_addr_info),           &ble_sm_id_addr_info_rx   },
    [BLE_SM_OP_SIGN_INFO]            = { BLE_SMP_CONN_ROLE_BOTH,            sizeof(struct ble_smp_signing_info),           &ble_sm_sign_info_rx      },
    [BLE_SM_OP_SEC_REQ]              = { BLE_SMP_CONN_ROLE_CENTRAL,         sizeof(struct ble_smp_security_request),       &ble_sm_sec_req_rx        },
    [BLE_SM_OP_PAIR_PUBLIC_KEY]      = { BLE_SMP_CONN_ROLE_BOTH,            sizeof(struct ble_smp_pairing_public_key),     &ble_sm_sc_public_key_rx  },
    [BLE_SM_OP_PAIR_DHKEY_CHECK]     = { BLE_SMP_CONN_ROLE_BOTH,            sizeof(struct ble_smp_pairing_dhkey_check),    &ble_sm_sc_dhkey_check_rx },
    [BLE_SM_OP_PAIR_KEYPRESS_NTF]    = { BLE_SMP_CONN_ROLE_BOTH,            sizeof(struct ble_smp_keypress_notification),  &ble_sm_sc_keypress_rx    },
};

#define BLE_SMP_OP_DISPATCH_SZ      (sizeof ble_sm_cmd_rx_dispatch / sizeof ble_sm_cmd_rx_dispatch[0])

static const ble_sm_rx_fn *ble_sm_cmd_rx_dispatch_find(uint8_t role_mask, uint8_t op, uint8_t len)
{
    if (op >= BLE_SMP_OP_DISPATCH_SZ) {
        return NULL;
    }

    const struct smp_cmd_rx_proc_t *smp_cmd_rx_proc = &ble_sm_cmd_rx_dispatch[op];

    if(smp_cmd_rx_proc->expect_len != len || (role_mask & smp_cmd_rx_proc->role) == 0) {
        return NULL;
    }

    return (const ble_sm_rx_fn *)smp_cmd_rx_proc->rx_cb;
}

int ble_smp_rx_handler(struct ble_host_conn *p_conn, uint16_t len, uint8_t *p_packet)
{
    if(p_conn == NULL) {
        BLE_HOST_L2CAP_SMP_INFO("conn not exists");
        return BLE_HOST_ERR_PARM;
    }

    int rc;
    struct smp_cmd_fmt *smp_data = (struct smp_cmd_fmt *)p_packet;
    const ble_sm_rx_fn *rx_cb = ble_sm_cmd_rx_dispatch_find(BIT(p_conn->role), smp_data->opcode, len);
    if (rx_cb != NULL) {
        rc = rx_cb(p_conn, smp_data);
    }
    else {
        rc = BLE_SMP_ERR(SMP_FAILED_PAIRING_NOT_SUPPORTED);
        BLE_HOST_L2CAP_SMP_ERROR("SMP RX dispatch failed: 0x%d", rc);
    }

    return rc;
}

struct ble_sm_keys 
{
    uint8_t ltk_valid:1;
    uint8_t ediv_rand_valid:1;
    uint8_t irk_valid:1;
    uint8_t csrk_valid:1;
    uint8_t addr_valid:1;
    uint8_t dev_pairing_status : 3; // Local Device Pairing Status, refer to enum smp_dev_pairing_status
    uint8_t addr_type;
    uint8_t key_size;
    uint16_t ediv;
    uint64_t rand_val;
    uint8_t addr[6];    /* Little endian. */
    uint8_t ltk[16];    /* Little endian. */
    uint8_t irk[16];    /* Little endian. */
    uint8_t csrk[16];   /* Little endian. */
};

struct ble_sm_public_key 
{
    uint8_t x[32];
    uint8_t y[32];
} __attribute__((packed));

//SC OOB local ECDH key
struct ble_smp_sc_oob_key
{
    uint8_t public_key[64];  //big--endian
    uint8_t private_key[32]; //big--endian
} __attribute__((packed));

//SC OOB data
struct ble_smp_sc_oob_data
{
    uint8_t random[16];  //big--endian
    uint8_t confirm[16]; //big--endian
} __attribute__((packed));

enum smp_pairing_method 
{
    JustWorks = 0,
    PK_Init_Display_Resp_Input = 1,
    PK_Resp_Display_Init_Input = 2,
    PK_BOTH_INPUT = 3,
    OOB_Authentication = 4,
    Numeric_Comparison = 5,
    SC_OOB_Authentication = 6,
} __attribute__((packed));

// H: Initiator Capabilities
// V: Responder Capabilities
// See the Core_v5.0(Vol 3/Part H/2.3.5.1) for more information.
static const enum smp_pairing_method gen_method_legacy[5 /*Responder IOCap*/][5 /*Initiator IOCap*/] = {
    {JustWorks,                  JustWorks,                  PK_Resp_Display_Init_Input, JustWorks, PK_Resp_Display_Init_Input},
    {JustWorks,                  JustWorks,                  PK_Resp_Display_Init_Input, JustWorks, PK_Resp_Display_Init_Input},
    {PK_Init_Display_Resp_Input, PK_Init_Display_Resp_Input, PK_BOTH_INPUT,              JustWorks, PK_Init_Display_Resp_Input},
    {JustWorks,                  JustWorks,                  JustWorks,                  JustWorks, JustWorks                 },
    {PK_Init_Display_Resp_Input, PK_Init_Display_Resp_Input, PK_Resp_Display_Init_Input, JustWorks, PK_Init_Display_Resp_Input},
};

static const enum smp_pairing_method gen_method_sc[5 /*Responder IOCap*/][5 /*Initiator IOCap*/] = {
    {JustWorks,                  JustWorks,                  PK_Resp_Display_Init_Input, JustWorks, PK_Resp_Display_Init_Input},
    {JustWorks,                  Numeric_Comparison,         PK_Resp_Display_Init_Input, JustWorks, Numeric_Comparison        },
    {PK_Init_Display_Resp_Input, PK_Init_Display_Resp_Input, PK_BOTH_INPUT,              JustWorks, PK_Init_Display_Resp_Input},
    {JustWorks,                  JustWorks,                  JustWorks,                  JustWorks, JustWorks                 },
    {PK_Init_Display_Resp_Input, Numeric_Comparison,         PK_Resp_Display_Init_Input, JustWorks, Numeric_Comparison        },
};

struct ble_sm_proc 
{
    STAILQ_ENTRY(ble_sm_proc) next;

    uint16_t conn_handle; //ACL connection handle
    uint8_t used_sec_lvl : 4; // security level of SMP pairing, refer to enum smp_security_level
    uint8_t sc_pairing : 1; // When pairing for the 1st time, whether using SC or legacy pairing method
    uint8_t stk_method : 3; // final available stk generate method, refer to enum smp_pairing_method
    uint8_t peerKey_mask : 2; // double check peer key distribute order
    uint8_t reconn_type : 6; // refer to enum smp_reconn_type
    uint16_t smp_method : 10; // final used SMP pairing method, refer to enum smp_method
    uint16_t enc_key_size : 6; // encryption key size, 7-16 bytes

    /* SMP property parameters configured by user */
    struct security_mng_cfg sm_cfg;

    /**
     * @brief SMP status parameters
     */
    uint8_t pairing_busy:    1;
    uint8_t key_distribute:    1;
    uint8_t bonding_enable:    1;
    uint8_t save_key_flag:    1;
    uint8_t support_smp:    1; //central use only
    uint8_t tk_status;
    uint8_t smpDistributeKeyOrder;  //record key distribute order
    union {
        struct ble_smp_key_dist smp_DistributeKeySend; //send key transmit
        uint8_t smp_DistributeKeySendValue;
    };
    union {
        struct ble_smp_key_dist smp_DistributeKeyRecv; //receive key transmit
        uint8_t smp_DistributeKeyRecvValue;
    };
    uint32_t smp_phase_chk;
    uint32_t smp_timeout_start_tick;

    /**
     * @brief smp parameter about own and peer.
     * 
     */
    struct ble_smp_pairing_cmd pairing_req;
    struct ble_smp_pairing_cmd pairing_rsp;
    uint8_t pairing_tk[16]; /* Temorary Key, little endian. */
    uint8_t peer_confirm[16]; /* if SC used: big endian, else Little endian. */
    uint8_t peer_rand[16]; /* if SC used: big endian, else Little endian. */
    uint8_t own_rand[16]; /* if SC used: big endian, else Little endian. */
    uint8_t own_ltk[16]; /* Little endian. */
    struct ble_sm_keys own_keys;
    struct ble_sm_keys peer_keys;

    /**
     * @brief Secure connections parameter
     * 
     */
    uint8_t sc_passkey_cnt;
//    uint8_t sc_dhk_own[32];
    uint8_t sc_mac_key[16];
    uint8_t sc_sk_dhk_own[32]; //  own  private (reused DH)key[32]  /* SC used: big endian */
    uint8_t sc_pk_own[64];     //  own  public  key[64] /* SC used: big endian */
    uint8_t sc_pk_peer[64];    // peer  public  key[64] /* SC used: big endian */

    struct ble_smp_sc_oob_data *scoob_local;
    struct ble_smp_sc_oob_data *scoob_remote;
    struct ble_smp_sc_oob_key *scoob_local_key;

};


/****************BLE Host stack SMP procedure management APIs. *****************/
STAILQ_HEAD(ble_sm_proc_list, ble_sm_proc) s_ble_smp_procs_head;

/** BLE Host SMP procedure management APIs.
 * These APIs are used to manage the list of active BLE Host SMP procedure.
 */

/**
 * @brief       The funciton is used to allocate a new SMP procedure control block. If allocation 
 *              succeed, then clear the control block's memory content to zero.
 * @param[in]   none
 * 
 * @return      The allocated SMP procedure control block, or NULL if allocation failed.
 */
static struct ble_sm_proc *ble_smp_proc_alloc(void)
{
    struct ble_sm_proc *proc = (struct ble_sm_proc *)ble_host_malloc(sizeof(struct ble_sm_proc), BLE_HOST_MALLOC_SMP_MANAGER);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("SMP Procedure Control Block: malloc failed");
        return NULL;
    }

    /* clear the SMP procedure control block's memory to zero */
    smemset(proc, 0, sizeof(struct ble_sm_proc));

    return proc;
}

/**
 * @brief       The function is used to find a SMP procedure control block by connection handle.
 *
 * @param[in]   conn_handle - The connection handle of the SMP procedure to find.
 * 
 * @return      The SMP procedure control block if found, or NULL if not found.
 */
static struct ble_sm_proc *ble_smp_proc_find(uint16_t conn_handle)
{
    struct ble_sm_proc *proc;

    STAILQ_FOREACH(proc, &s_ble_smp_procs_head, next)
    {
        if (proc->conn_handle == conn_handle) {
            return proc;
        }
    }

    return NULL;
}

/**
 * @brief       The function is used to insert a new SMP procedure control block into the list of active SMP procedures.
 *
 * @param[in]   sm_proc - The SMP procedure control block to insert.
 *
 * @return      none
 */
static void ble_smp_proc_insert_new(struct ble_sm_proc *sm_proc)
{
    if (sm_proc != NULL) {
        STAILQ_INSERT_HEAD(&s_ble_smp_procs_head, sm_proc, next);
    }
}

/**
 * @brief       The function is used to remove a SMP procedure control block from the list of active SMP procedures.
 * 
 * @param[in]   sm_proc - The SMP procedure control block to reomve.
 *
 * @return      none
 */
static void ble_smp_proc_remove_old(struct ble_sm_proc *sm_proc)
{
    if (sm_proc != NULL) {
        STAILQ_REMOVE(&s_ble_smp_procs_head, sm_proc, ble_sm_proc, next);
        ble_host_free((void*)sm_proc);
    }
}
/**************** The end of BLE Host stack SMP procedure management APIs. *****************/


/*
 * Return STK generate method.
 * See the Core_v5.0(Vol 3/Part H/2.3.5.1) for more information.
 * */
static enum smp_pairing_method ble_smp_get_pairing_method(struct ble_sm_proc *sm_proc)
{
    int responder_MITM  = sm_proc->pairing_rsp.authReq.MITM;
    int responder_oob   = sm_proc->pairing_rsp.oobDataFlag;
    int responder_iocap = sm_proc->pairing_rsp.ioCapability;

    int initiator_MITM  = sm_proc->pairing_req.authReq.MITM;
    int initiator_oob   = sm_proc->pairing_req.oobDataFlag;
    int initiator_iocap = sm_proc->pairing_req.ioCapability;

    int sc_en = sm_proc->stk_method;

    if (sc_en + responder_oob + initiator_oob >= 2) { //simplify the judge: 2 of them is true, OOB is available
        if (sc_en) {
            return SC_OOB_Authentication;
        } else {
            return OOB_Authentication;
        }
    }

    if (!responder_MITM && !initiator_MITM) {
        return JustWorks;
    }

    if (responder_iocap > SMP_IO_CAP_KEYBOARD_DISPLAY || initiator_iocap > SMP_IO_CAP_KEYBOARD_DISPLAY) {
        return JustWorks;
    }

    if (sc_en) {
        return gen_method_sc[responder_iocap][initiator_iocap];
    } else {
        return gen_method_legacy[responder_iocap][initiator_iocap];
    }
}

static enum smp_method ble_smp_get_smp_method(struct ble_sm_proc *proc)
{
    BLE_HOST_SAL_ASSERT(proc!= NULL);
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(proc->conn_handle);
    BLE_HOST_SAL_ASSERT(conn!= NULL);

    enum smp_method method;
    enum smp_security_level used_sec_level;
    bool is_sc = proc->sc_pairing;
    /* Map the pairing method to the SMP method */
    switch(proc->stk_method) {
    case JustWorks:
        method = is_sc ? SMP_METHOD_LEGACY_JW : SMP_METHOD_LESC_JW;
        used_sec_level = LE_SECURITY_MODE_1_LEVEL_2;
        break;
    case PK_Init_Display_Resp_Input:
        if (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) {
            method = is_sc ? SMP_METHOD_LESC_PKD : SMP_METHOD_LEGACY_PKD;
        } else {
            method = is_sc ? SMP_METHOD_LESC_PKI : SMP_METHOD_LEGACY_PKI;
        }
        used_sec_level = is_sc ? LE_SECURITY_MODE_1_LEVEL_3 : LE_SECURITY_MODE_1_LEVEL_4;
        break;
    case PK_Resp_Display_Init_Input:
        if (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) {
            method = is_sc ? SMP_METHOD_LESC_PKI : SMP_METHOD_LEGACY_PKI;
        } else {
            method = is_sc ? SMP_METHOD_LESC_PKD : SMP_METHOD_LEGACY_PKD;
        }
        used_sec_level = is_sc ? LE_SECURITY_MODE_1_LEVEL_3 : LE_SECURITY_MODE_1_LEVEL_4;
        break;
    case PK_BOTH_INPUT:
        method = is_sc ? SMP_METHOD_LESC_PKI : SMP_METHOD_LEGACY_PKI;
        used_sec_level = is_sc ? LE_SECURITY_MODE_1_LEVEL_3 : LE_SECURITY_MODE_1_LEVEL_4;
        break;
    case OOB_Authentication:
        method = SMP_METHOD_LEGACY_OOB;
        used_sec_level = LE_SECURITY_MODE_1_LEVEL_3;
        break;
    case Numeric_Comparison:
        method = SMP_METHOD_LESC_NC;
        used_sec_level = LE_SECURITY_MODE_1_LEVEL_4;
        break;
    case SC_OOB_Authentication:
        method = SMP_METHOD_LESC_OOB;
        used_sec_level = LE_SECURITY_MODE_1_LEVEL_4;
        break;
    default:
        /* Should never happen, default to JustWorks */
        method = SMP_METHOD_UNSPECIFIED;
        used_sec_level = LE_SECURITY_MODE_1_LEVEL_1;
        break;
    }

    proc->used_sec_lvl = used_sec_level;
    proc->smp_method = method;

    return method;
}

static void ble_smp_proc_pairing_end(struct ble_host_conn *conn, uint8_t reason) // 0 is OK; other for fail
{
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc) {
        BLE_HOST_L2CAP_SMP_DEBUG("Free the SMP procedure control block");
        ble_smp_proc_remove_old(proc);
    }

    /* clear the encryption flag for this connection */
    conn->encryption_busy = 0;

    if (reason && (gap_eventMask & GAP_EVT_MASK_SMP_PAIRING_FAIL)) {
        gap_smp_pairingFailEvt_t gapEvt;
        gapEvt.connHandle = conn->conn_handle;
        gapEvt.reason = reason;
        blc_gap_send_event(GAP_EVT_SMP_PAIRING_FAIL, (uint8_t *)&gapEvt, sizeof(gap_smp_pairingFailEvt_t));
    }
}


static int  ble_smp_pair_initiate(struct ble_host_conn *conn, bool is_sec_req_rcvd);
static int  ble_smp_encryption_initiate(struct ble_host_conn *conn);
static int  ble_smp_peripheral_initiate(struct ble_host_conn *conn);


static int ble_sm_pair_req_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    uint8_t pairing_failed_reason;

    if (conn->sm_settings.sm_sec_lvl == LE_SECURITY_MODE_1_LEVEL_1) {
        pairing_failed_reason = SMP_FAILED_PAIRING_NOT_SUPPORTED;
        goto fail;
    }

    /* if already allocated, remove the old procedure */
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc) {
        ble_smp_proc_remove_old(proc);
    }

    /* allocate a new procedure for SMP procedure */
    proc = ble_smp_proc_alloc();
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Allocate a new SMP procedure control block failed, not enough memory");
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    /* insert the new SMP procedure */
    ble_smp_proc_insert_new(proc);

    /* alloc to ACL connection handle */
    proc->conn_handle = conn->conn_handle;

    /**
     * Initialize SMP parameters setting,
     * when ACL connected, gap callback the Upper user, user can change SMP settings
     */
    proc->sm_cfg = conn->sm_settings; //TODO: default use host SM settings, but can be changed by user

    /* prepare pairing response parameters */
    struct ble_smp_auth_req *auth = &proc->pairing_rsp.authReq;
    auth->sc = proc->sm_cfg.sm_sc;
    auth->bondingFlags = proc->sm_cfg.sm_bonding;
    auth->keypress = proc->sm_cfg.sm_keypress;
    auth->MITM = proc->sm_cfg.sm_mitm;
    proc->pairing_rsp.code = BLE_SM_OP_PAIR_RSP;
    proc->pairing_rsp.oobDataFlag = proc->sm_cfg.sm_oob;
    proc->pairing_rsp.ioCapability = proc->sm_cfg.sm_io_capability;
    proc->pairing_rsp.maxEncKeySize = proc->sm_cfg.sm_min_key_size;
    proc->pairing_rsp.initKeyValue = proc->sm_cfg.sm_peer_key_dist;
    proc->pairing_rsp.rspKeyValue = proc->sm_cfg.sm_our_key_dist;

    struct ble_smp_pairing_cmd *pairing_cmd = (struct ble_smp_pairing_cmd *)smp_data;

    /* keep the pairing request parameters for later use */
    proc->pairing_req = *pairing_cmd;

    //smp_evt_handler(conn, EVENT_PAIRING_REQ, NULL);

    //blc_SecReq_ctrl.secReq_pending &= ~BIT(conn_idx); //clear security request pending flag

    /***************************************************************
     * Check parameters of both sides,  and determine :
     * 1. which pairing used:  legacy pairing or Secure Connection
     * 2. which stk generate methods used:  just works/OOB/pass_key
     *    entry/numeric comparison
     ***************************************************************/
    proc->sc_pairing = (proc->pairing_req.authReq.sc && proc->pairing_rsp.authReq.sc) ? 1 : 0;
    proc->stk_method = ble_smp_get_pairing_method(proc);
    proc->smp_method = ble_smp_get_smp_method(proc);

    /********************************************************************************
      Pairing begin
     *******************************************************************************/
    if (gap_eventMask & GAP_EVT_MASK_SMP_PAIRING_BEGIN) {
        gap_smp_pairingBeginEvt_t gapEvt;
        gapEvt.connHandle  = proc ->conn_handle;
        gapEvt.secure_conn = proc->sc_pairing;
        gapEvt.tk_method   = proc->stk_method;
        blc_gap_send_event(GAP_EVT_SMP_PAIRING_BEGIN, (uint8_t *)&gapEvt, sizeof(gap_smp_pairingBeginEvt_t));
    }

    if (proc->pairing_req.maxEncKeySize < SMP_ENC_KEY_SIZE_MINIMUM) {
        pairing_failed_reason = SMP_FAILED_ENCRYPTION_KEY_SIZE;
        goto fail;
    } else if (proc->pairing_req.maxEncKeySize > SMP_ENC_KEY_SIZE_MAXIMUM) {
        pairing_failed_reason = SMP_FAILED_INVALID_PARAMETERS;
        goto fail;
    } else {
        uint8_t level4only = ((proc->sm_cfg.sm_sec_lvl & LE_SECURITY_MODE_1) == LE_SECURITY_MODE_1_LEVEL_4) ? 1 : 0;

       /* Refer to Core5.2 Spec | Vol 3, Part C page 1375
        * if need to check SC level4 only (Notice: Here we refer to SC only corresponding to LE mode1 level4.)
        * A device may be in a Secure Connections Only mode. When in Secure Connections Only mode only security
        * mode 1 level 4 shall be used except for services that only require security mode 1 level 1. */
        if (proc->used_sec_lvl < LE_SECURITY_MODE_1_LEVEL_4) {
            /* if the local gap setting only support level4 only,we should response pairing failed */
            if(level4only){
                pairing_failed_reason = SMP_FAILED_AUTH_REQ;
                goto fail;
            }
        }
    }

    proc->enc_key_size = min(proc->pairing_req.maxEncKeySize, proc->pairing_rsp.maxEncKeySize);
    proc->bonding_enable = proc->pairing_rsp.authReq.bondingFlags && proc->pairing_req.authReq.bondingFlags;

    if (proc->sc_pairing) {
        //Fix PTS case:GAP/SEC/SEM/BI-09-C: LE Mode1 level4, encryption key size must be 16
        if((proc->smp_method != SMP_METHOD_LESC_JW) && (proc->enc_key_size != SMP_ENC_KEY_SIZE_MAXIMUM)) {
            pairing_failed_reason = SMP_FAILED_ENCRYPTION_KEY_SIZE;
            goto fail;
        }

        /* clear key disribution bits, if support SC, then set to key_dist_value bit field to zero */
        proc->pairing_rsp.initKey.encKey = 0;
        proc->pairing_rsp.rspKey.encKey = 0;

        if (proc->smp_method == SMP_METHOD_LESC_OOB) {
            if (!(gap_eventMask & GAP_EVT_MASK_SMP_REQUEST_SCOOB_DATA)) {
                pairing_failed_reason = SMP_FAILED_OOB_NOT_AVAILABLE;
                goto fail;
            }
        } else {
            /* Generate ECDH keys, TODO: if Telink BLE Controller support CONTROLLER_GEN_P256KEY_ENABLE, we can use HCI cmd instead below call */
            if (!tlkalg_ecc_gen_key_pair(proc->sc_pk_own, proc->sc_sk_dhk_own, ECC_use_secp256r1, proc->sm_cfg.sm_sc_debug_mode)) {
                pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                goto fail;
            }
        }

        /* should delete the older smp banding info */
        ble_smp_store_keys_delete(conn);
    } else {
        /* if 1st time use SC, then unpaired, and 2nd time(do not re-power) use Legacy, key distribution bit will be Err. 
         * Set key disribution bits, if not support SC, then set to key_dist_value bit field to ONE */
        proc->pairing_rsp.initKey.encKey = 1;
        proc->pairing_rsp.rspKey.encKey = 1;
        proc->pairing_rsp.authReq.sc = 0;
    }

    /* Generate Srand */
    ble_host_hci_le_gen_rand(proc->own_rand, 16); 

    /* Generate owner LTK */
    if (!proc->sc_pairing) { //SC no need LTK here 
        ble_host_hci_le_gen_rand(proc->own_ltk, 16);

        #if (SMP_LEGACY_LTK_VERIFICATION_EN)
            uint16_t crc16_ = crc16(proc->own_ltk, 14);
            proc->own_ltk[14] = crc16_ &0xFF;
            proc->own_ltk[15] = (crc16_>>8) &0xFF;
            BLE_HOST_L2CAP_SMP_DEBUG("[ltk] crc16_", &crc16_, 2);
        #endif
    }

    proc->smp_DistributeKeyRecvValue = proc->pairing_rsp.initKeyValue & proc->pairing_req.initKeyValue; //s-role: key receive
    proc->smp_DistributeKeySendValue = proc->pairing_rsp.rspKeyValue  & proc->pairing_req.rspKeyValue;  //s-role: key send

    /*
     * M->S Pairing Req: phase1 begin
     * S->M Pairing Rsp:
     */
    proc->smp_phase_chk = BLE_SMP_PAIRING_PHASE_1_OK; //SMP Phase stage 1 begin

    /*
     ***************************************************************
     * Send corresponding event to upper layer according to TK generate methods
     ***************************************************************
     */
    if (proc->smp_method & SMP_METHOD_LEGACY_OOB) {
        proc->tk_status = BLE_SMP_TK_STATE_REQUEST;
        if (gap_eventMask & GAP_EVT_MASK_SMP_TK_REQUEST_OOB) {
            gap_smp_TkRequestOOBEvt_t gapEvt;
            gapEvt.connHandle = conn->conn_handle;
            blc_gap_send_event(GAP_EVT_SMP_TK_REQUEST_OOB, (uint8_t*)&gapEvt, sizeof(gap_smp_TkRequestOOBEvt_t));
        }
    } else if (proc->smp_method & SMP_METHOD_PKD) {
        //Responder generate TK value(0~999999) , should notify upper layer to display this number,
        //then initiator input this number to set TK value upon watching the displayed value
        uint32_t tk_set = proc->sm_cfg.sm_pke_dft_pincode;
        memset(proc->pairing_tk, 0, 16);

        if (tk_set > 0 && tk_set <= 999999) {
            smemcpy(proc->pairing_tk, &tk_set, 4);
        }  else {
            /* Ensure that tk_set is between 100000 and 999999, ensure that the highest bit is not 0 */
            ble_host_hci_le_gen_rand((uint8_t *)&tk_set, 4);
            tk_set = tk_set % 900000 + 100000; 
            smemcpy(proc->pairing_tk, &tk_set, 4);
        }

        if(gap_eventMask & GAP_EVT_MASK_SMP_TK_DISPLAY){
            gap_smp_TkDisplayEvt_t gapEvt;
            gapEvt.connHandle = conn->conn_handle;
            gapEvt.tk_pincode = tk_set;
            blc_gap_send_event(GAP_EVT_SMP_TK_DISPLAY, (uint8_t*)&gapEvt, sizeof(gap_smp_TkDisplayEvt_t));
        }
    } else if (proc->smp_method & SMP_METHOD_LEGACY_PKI) {
        // both sides should input TK value, here send TK request event to upper layer,
        // expect upper layer call "blc_smp_setTK_by_PasskeyEntry"  to set TK value
        proc->tk_status = BLE_SMP_TK_STATE_REQUEST;
        if(gap_eventMask & GAP_EVT_MASK_SMP_TK_REQUEST_PASSKEY){
            gap_smp_TkReqPassKeyEvt_t gapEvt;
            gapEvt.connHandle = conn->conn_handle;
            blc_gap_send_event(GAP_EVT_SMP_TK_REQUEST_PASSKEY, (uint8_t*)&gapEvt, sizeof(gap_smp_TkReqPassKeyEvt_t));
        }
    }

    /* paring request/response exchange end, set pairing busy flag */
    proc->pairing_busy = 1;

#if SMP_REAL_ENCRYPTION_BUSY_ENABLE
    if (!real_encryption_busy_enable)
#endif
    {
        /* Set in advance after Pairing_REQ/RSP interaction, and in Core Spec set after receiving LL_ENC_REQ */
        conn->encryption_busy = 1;
    }

    return ble_sm_pair_rsp_tx(conn);

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_pair_rsp_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    uint8_t pairing_failed_reason;

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    struct ble_smp_pairing_cmd *pairing_cmd = (struct ble_smp_pairing_cmd *)smp_data;

    /* keep the pairing response parameters for later use */
    proc->pairing_rsp = *pairing_cmd;

    /***************************************************************
     * Check parameters of both sides,  and determine :
     * 1. which pairing used:  legacy pairing or Secure Connection
     * 2. which stk generate methods used:  just works/OOB/pass_key
     *    entry/numeric comparison
     ***************************************************************/
    proc->sc_pairing = (proc->pairing_req.authReq.sc && proc->pairing_rsp.authReq.sc) ? 1 : 0;
    proc->stk_method = ble_smp_get_pairing_method(proc);
    proc->smp_method = ble_smp_get_smp_method(proc);

    /********************************************************************************
     Pairing begin
    *******************************************************************************/
    if (gap_eventMask & GAP_EVT_MASK_SMP_PAIRING_BEGIN) {
        gap_smp_pairingBeginEvt_t gapEvt;
        gapEvt.connHandle  = proc ->conn_handle;
        gapEvt.secure_conn = proc->sc_pairing;
        gapEvt.tk_method   = proc->stk_method;
        blc_gap_send_event(GAP_EVT_SMP_PAIRING_BEGIN, (uint8_t*)&gapEvt, sizeof(gap_smp_pairingBeginEvt_t));
    }

    uint8_t level4only = ((proc->sm_cfg.sm_sec_lvl & LE_SECURITY_MODE_1) == LE_SECURITY_MODE_1_LEVEL_4) ? 1 : 0;

    //if need to check SC level4 only (Notice: Here we refer to SC only corresponding to LE mode1 level4.)
    if (proc->used_sec_lvl < LE_SECURITY_MODE_1_LEVEL_4) {
       /* Refer to Core5.2 Spec | Vol 3, Part C page 1375
        * A device may be in a Secure Connections Only mode. When in Secure Connections Only mode only security
        * mode 1 level 4 shall be used except for services that only require security mode 1 level 1.
        */
        if (level4only) {
            pairing_failed_reason = SMP_FAILED_AUTH_REQ;
            goto fail;
        }
    }

    if (proc->pairing_rsp.maxEncKeySize < SMP_ENC_KEY_SIZE_MINIMUM) {
        pairing_failed_reason = SMP_FAILED_ENCRYPTION_KEY_SIZE;
        goto fail;
    } else if (proc->pairing_rsp.maxEncKeySize > SMP_ENC_KEY_SIZE_MAXIMUM) {
        pairing_failed_reason = SMP_FAILED_INVALID_PARAMETERS;
        goto fail;
    }

    proc->enc_key_size = min(proc->pairing_req.maxEncKeySize, proc->pairing_rsp.maxEncKeySize);
    proc->bonding_enable = proc->pairing_rsp.authReq.bondingFlags && proc->pairing_req.authReq.bondingFlags;

    if (proc->sc_pairing) {
        //Fix PTS case:GAP/SEC/SEM/BI-10-C: LE Mode1 level4, encryption key size must be 16
        if((proc->smp_method != SMP_METHOD_LESC_JW) && (proc->enc_key_size != SMP_ENC_KEY_SIZE_MAXIMUM)) {
            pairing_failed_reason = SMP_FAILED_ENCRYPTION_KEY_SIZE;
            goto fail;
        }

        /* clear key disribution bits, if support SC, then set to key_dist_value bit field to zero */
        proc->pairing_rsp.initKeyValue = 0;
        proc->pairing_rsp.rspKeyValue = 0;

        if (proc->smp_method & SMP_METHOD_LESC_OOB) {
            if (!(gap_eventMask & GAP_EVT_MASK_SMP_REQUEST_SCOOB_DATA)) {
                pairing_failed_reason = SMP_FAILED_OOB_NOT_AVAILABLE;
                goto fail;
            }
        } else {
            /* Generate ECDH keys */
            if (!tlkalg_ecc_gen_key_pair(proc->sc_pk_own, proc->sc_sk_dhk_own, ECC_use_secp256r1, proc->sm_cfg.sm_sc_debug_mode)) {
                pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                goto fail;
            }
        }
    }

    /* Generate Mrand */
    ble_host_hci_le_gen_rand(proc->own_rand, 16); 

    /* Generate owner LTK */
    if (!proc->sc_pairing) { //SC no need LTK here 
        ble_host_hci_le_gen_rand(proc->own_ltk, 16);
    }
    
    proc->smp_DistributeKeySendValue = proc->pairing_rsp.initKeyValue & proc->pairing_req.initKeyValue; //s-role: key send
    proc->smp_DistributeKeyRecvValue = proc->pairing_rsp.rspKeyValue  & proc->pairing_req.rspKeyValue;  //s-role: key receive

    proc->pairing_busy = 1;

#if SMP_REAL_ENCRYPTION_BUSY_ENABLE
    if (!real_encryption_busy_enable)
#endif
    {
        /* Set in advance after Pairing_REQ/RSP interaction, and in Core Spec set after receiving LL_ENC_REQ */
        conn->encryption_busy = 1;
    }

   /*
    * M->S Pairing Req: phase1 begin
    * S->M Pairing Rsp:
    */
    proc->smp_phase_chk = BLE_SMP_PAIRING_PHASE_1_OK; //SMP Phase stage 1 begin

    if (proc->smp_method & SMP_METHOD_PKD) {
        /* Initiator generate TK value(0~999999) , should notify upper layer to display this number,
         * then responder input this number to set TK value upon watching the displayed value */
        uint32_t tk_set = proc->sm_cfg.sm_pke_dft_pincode;
        memset(proc->pairing_tk, 0, 16);

        if (tk_set > 0 && tk_set <= 999999) {
            smemcpy(proc->pairing_tk, &tk_set, 4);
        } else {
            /* Ensure that tk_set is between 100000 and 999999, ensure that the highest bit is not 0 */
            ble_host_hci_le_gen_rand((uint8_t *)&tk_set, 4);
            tk_set = tk_set % 900000 + 100000; 
            smemcpy(proc->pairing_tk, &tk_set, 4);
        }

        /* notify upper layer to display the TK value */
        if (gap_eventMask & GAP_EVT_MASK_SMP_TK_DISPLAY) {
            gap_smp_TkDisplayEvt_t gapEvt;
            gapEvt.connHandle = proc->conn_handle;
            gapEvt.tk_pincode = tk_set;
            blc_gap_send_event(GAP_EVT_SMP_TK_DISPLAY, (uint8_t*)&gapEvt, sizeof(gap_smp_TkDisplayEvt_t));
        }
    }

    if (proc->sc_pairing)  {
        if (proc->smp_method & SMP_METHOD_LESC_OOB) { //central SC OOB
            if (gap_eventMask & GAP_EVT_MASK_SMP_REQUEST_SCOOB_DATA) {
                proc->tk_status = BLE_SMP_TK_STATE_REQUEST;
                //oob data check
                bool scOobLocalUsed, scOobRemoteUsed;
                //see peer's flag decide own sc oob data used
                scOobLocalUsed  = (proc->pairing_rsp.oobDataFlag);
                scOobRemoteUsed = (proc->pairing_req.oobDataFlag);

                //clear
                proc->scoob_local = NULL;
                proc->scoob_remote = NULL;
                proc->scoob_local_key = NULL;

                gap_smp_requestScOobDataEvt_t gapEvt;
                gapEvt.connHandle = proc->conn_handle;
                gapEvt.scOobLocalUsed = scOobLocalUsed;
                gapEvt.scOobRemoteUsed = scOobRemoteUsed;
                blc_gap_send_event(GAP_EVT_SMP_REQUEST_SCOOB_DATA, (uint8_t*)&gapEvt, sizeof(gap_smp_requestScOobDataEvt_t));
                
                /* attention: to do pending process, here do noting */
                return 0;
            }
        }
        /* send ecdh public key */
        return ble_sm_public_key_tx(conn);
    } else { //legacy SMP
        if (proc->smp_method & (SMP_METHOD_LEGACY_PKI|SMP_METHOD_LEGACY_OOB)) {
            proc->tk_status = BLE_SMP_TK_STATE_REQUEST;

            /* notify upper layer to input the TK value for LE legacy OOB or LE legacy PKI */
            if (proc->smp_method & SMP_METHOD_LEGACY_OOB) {
                if (gap_eventMask & GAP_EVT_MASK_SMP_TK_REQUEST_OOB) {
                    gap_smp_TkRequestOOBEvt_t gapEvt;
                    gapEvt.connHandle = proc->conn_handle;
                    blc_gap_send_event(GAP_EVT_SMP_TK_REQUEST_OOB, (uint8_t*)&gapEvt, sizeof(gap_smp_TkRequestOOBEvt_t));
                }
            } else {
                if (gap_eventMask & GAP_EVT_MASK_SMP_TK_REQUEST_PASSKEY) {
                    gap_smp_TkReqPassKeyEvt_t gapEvt;
                    gapEvt.connHandle = proc->conn_handle;;
                    blc_gap_send_event(GAP_EVT_SMP_TK_REQUEST_PASSKEY, (uint8_t*)&gapEvt, sizeof(gap_smp_TkReqPassKeyEvt_t));
                }
            }

            /* check if the upper layer has input the TK value, if not yet, set pending flag */
            if ((proc->tk_status & BLE_SMP_TK_STATE_REQUEST) && !(proc->tk_status & BLE_SMP_TK_STATE_UPDATE)) {
                proc->tk_status |= BLE_SMP_TK_STATE_CONFIRM_PENDING; //pending
                /* attention: to do pending process, here do noting */
                return 0;
            }
        }
        
        /* send pairing confirm to the peer device */   
        return ble_sm_pair_confirm_tx(conn);
    }

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_confirm_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    uint8_t pairing_failed_reason;

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    bool smp_central_role = (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) ? 1 : 0;
    struct ble_smp_pairing_confirm *pairing_confirm = (struct ble_smp_pairing_confirm *)smp_data;

    if (proc->sc_pairing) {
        /* because Secure Connections cryptographic functions use big-endian mode, so need swap the data */
        swapX(pairing_confirm->confirm, proc->peer_confirm, 16);
        
        if (smp_central_role) {
            //In Secure connections Passkey entry protocol.
            if (proc->smp_method & SMP_METHOD_LESC_PKE) {
               /*
                * Secure Connection Pairing:
                * M->S Pairing Req: phase1 begin
                * S->M Pairing Rsp:
                * M->S Pairing Public Key: phase2 begin
                * S->M Pairing Public Key:
                * ......
                * M->S Pairing Confirm: (20 times)
                * S->M Pairing Confirm: (20 times) Pairing Confirm marked
                */
                if (proc->sc_passkey_cnt == 19) { //20 times confirm/random, idx from Zero.
                    if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_PUBLIC_KEY))) {
                        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                        goto fail;
                    }

                    proc->smp_phase_chk = BIT(BLE_SM_OP_PAIR_CFM);
                }
            } else { //Numeric Comparison(or Just work) and OOB protocol
                /*
                        * Secure Connection Pairing:
                        * M->S Pairing Req: phase1 begin
                        * S->M Pairing Rsp:
                        * M->S Pairing Public Key: phase2 begin
                        * S->M Pairing Public Key:
                        * S->M Pairing Confirm: Pairing Confirm marked
                        */
                if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_PUBLIC_KEY))) {
                    pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                    goto fail;
                }

                proc->smp_phase_chk = BIT(BLE_SM_OP_PAIR_CFM);
            }
        }
    } else { //Legacy SMP
        /* because Legacy SMP cryptographic toolbox functions use little-endian mode, so no need swap the data */
        smemcpy(proc->peer_confirm, pairing_confirm->confirm, 16);

       /*
        * Legacy Pairing:
        * M->S Pairing Req: phase1 begin
        * S->M Pairing Rsp:
        * M->S Pairing Confirm: (smp peripheral role)Pairing Confirm marked. phase2 begin
        * S->M Pairing Confirm  (central role)Pairing Confirm marked
        */
        if (proc->smp_phase_chk != BLE_SMP_PAIRING_PHASE_1_OK) { //Ensure the integrity of the pairing process
            pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
            goto fail;
        }

        proc->smp_phase_chk = BIT(BLE_SM_OP_PAIR_CFM);
    }

    if (smp_central_role) {
        /* send pairing random to the peer device */
        return ble_sm_pair_random_tx(conn);
    } else { //smp peripheral role
        bool is_input_tk_ok = true;
        /* check if the input TK is valid, if not use PKI or LE legacy OOB, it means tk use zero, tk_ok is true */
        if ((proc->smp_method & (SMP_METHOD_PKI | SMP_METHOD_LEGACY_OOB))) {
            if ((proc->tk_status & BLE_SMP_TK_STATE_REQUEST) && !(proc->tk_status & BLE_SMP_TK_STATE_UPDATE)) {
                proc->tk_status |= BLE_SMP_TK_STATE_CONFIRM_PENDING; //pending
                is_input_tk_ok = false;                    
            }
        }

        if (is_input_tk_ok == true) {
            /* send pairing comfirm to the peer device */
            return ble_sm_pair_confirm_tx(conn);
        } else { 
            /* attention: to do pending process */
            return 0; 
        }
    }

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);    
}

static int ble_sm_random_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    uint8_t pairing_failed_reason;

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    uint8_t pairing_conf[16] = {0};
    bool smp_central_role = (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) ? 1 : 0;
    struct ble_smp_pairing_random *pairing_random = (struct ble_smp_pairing_random *)smp_data;

    if (proc->sc_pairing) {
        /* because Secure Connections cryptographic functions use big-endian mode, so need swap the data */
        swapX(pairing_random->random, proc->peer_rand, 16);

        /*
         * 1.sc passkey entry method:
         *    M->S Pairing Random: (smp peripheral role: after recv random , sc_passkey_cnt++)
         *    S->M Pairing Random: (central role: after recv random , sc_passkey_cnt++)
         * 2.other methods:
         *    sc_passkey_cnt must be zero
         */
        if (!proc->sc_passkey_cnt) {
            //Private and DHkey use the same buffer, after obtaining dhkey, the private key content will be overwritten by dhkey
            if (!tlkalg_ecc_gen_dhkey(proc->sc_pk_peer, proc->sc_sk_dhk_own, proc->sc_sk_dhk_own, ECC_use_secp256r1)) {
                pairing_failed_reason = SMP_FAILED_INVALID_PARAMETERS;
                goto fail;
            }
        }

        if (proc->smp_method & SMP_METHOD_LESC_OOB) {
            if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_PUBLIC_KEY))) {
                pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                goto fail;
            }

            proc->smp_phase_chk = BIT(BLE_SM_OP_PAIR_RANDOM);
            
            /* send pairing dhkey check  or pairing random according to ACL connection role */
            return smp_central_role ? ble_sm_dhkey_check_tx(conn) : ble_sm_pair_random_tx(conn);
        }

        uint32_t sc_passkey = 0;
        uint8_t  f4_param_z = 0;

        if (smp_central_role) {
            //In Secure connections Passkey entry protocol.
            if (proc->smp_method & SMP_METHOD_PKE) {
                sc_passkey = bstream_to_u24_le(proc->pairing_tk); //proc->pairing_tk[2] << 16 | proc->pairing_tk[1] << 8 | proc->pairing_tk[0]);
                f4_param_z = ((sc_passkey >> proc->sc_passkey_cnt) & 0x01) | 0x80;
                proc->sc_passkey_cnt++;
            } else { //Numeric Comparison(or Just work) and OOB protocol
                f4_param_z = 0;
            }

            //Cbi = f4(PKb,PKa,Nbi,rai)
            blt_crypto_alg_f4(pairing_conf, proc->sc_pk_peer, proc->sc_pk_own, proc->peer_rand, f4_param_z);

            //check if Cbi=f4(Pkb,Pka,Nbi,rai).if check fails,about.
            if (memcmp(pairing_conf, proc->peer_confirm, 16) || proc->sc_passkey_cnt > 20) {
                proc->sc_passkey_cnt = 0;
                pairing_failed_reason = SMP_FAILED_CONFIRM_VALUE_FAILED;
                goto fail;
            }

            //Check Cbi=f4(Pkb,Pka,Nbi,rai) OK
            if (proc->smp_method & SMP_METHOD_LESC_NC) {
                // when numeric comparison mode used , calc 6-digit confirm value here.
                // Vb = g2(PKa, PKb, Na, Nb)
                uint32_t pinCode = blt_crypto_alg_g2(proc->sc_pk_own, proc->sc_pk_peer, proc->own_rand, proc->peer_rand); //numeric comparison
                /* because Secure Connections cryptographic functions use big-endian mode, so need swap the data */
                swapN((uint8_t *)&pinCode, 4);
                /* pincode is 6-digit, valid value is 000000 - 999999 */
                pinCode = pinCode % 1000000;

                //Slave displays 6 bit  numeric comparison value, start a special task
                //waiting for peripheral confirmation: 'YES'(pairing) or 'NO'(cancel).
                proc->tk_status = BLE_SMP_TK_STATE_NC;

                if (gap_eventMask & GAP_EVT_MASK_SMP_TK_NUMERIC_COMPARE) {
                    // send Vb to upper layer, upper layer should display it, and  confirm check if this
                    // value match peer device's displaying result, then confirm it by calling
                    // "" sending YES or NO to smp layer
                    gap_smp_TkDisplayEvt_t gapEvt;
                    gapEvt.connHandle = conn->conn_handle;
                    gapEvt.tk_pincode = pinCode;
                    blc_gap_send_event(GAP_EVT_SMP_TK_NUMERIC_COMPARE, (uint8_t*)&gapEvt, sizeof(gap_smp_TkDisplayEvt_t));
                }
            } else if (proc->smp_method & SMP_METHOD_PKE) {
                if (proc->sc_passkey_cnt >= 20) {
                    proc->sc_passkey_cnt = 0;

                    if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_CFM))) {
                        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                        goto fail;
                    }

                    proc->smp_phase_chk = BIT(BLE_SM_OP_PAIR_RANDOM);

                    return ble_sm_dhkey_check_tx(conn);
                } else {
                    return ble_sm_pair_confirm_tx(conn);
                }
            }

            if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_CFM))) {
                pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                goto fail;
            }

            proc->smp_phase_chk = BIT(BLE_SM_OP_PAIR_RANDOM);

            //if select numeric comparison, to execute here, Master must already confirmed 'YES'(Pairing).
            if (proc->smp_method & SMP_METHOD_LESC_NC) {
                if (proc->tk_status & BLE_SMP_TK_STATE_NC) {
                    if (proc->tk_status & BLE_SMP_TK_STATE_NC_CHECK_YES) { //NC confirmed "YES", set by upper layer completed
                        proc->tk_status = 0;
                    } else if (proc->tk_status & BLE_SMP_TK_STATE_NC_CHECK_NO) { //NC confirmed "NO", set by upper layer completed
                        proc->tk_status = 0;
                        //See the Core_v5.0(Vol 3/Part H/3.5.5/Pairing Failed) for more information.
                        //NOTICE: test by smart phone, central send unspecified reason when press "NO" button!
                        pairing_failed_reason = SMP_FAILED_NUMERIC_COMPARISION_FAILED;
                        goto fail;
                    } else {
                        proc->tk_status |= BLE_SMP_TK_STATE_NC_DHKEY_SUCC_PENDING;
                        return 0; //check it in gap mainLoop, if NC confirmed "YES", send peer_confirm to peer device
                    }
                }
            }

            return ble_sm_dhkey_check_tx(conn);
        } else { //smp peripheral role
            //In Numeric Comparison protocol
            if (proc->smp_method & SMP_METHOD_LESC_NC) {
                // when numeric comparison mode used , calc 6-digit confirm value here.
                // Vb = g2(PKa, PKb, Na, Nb)
                uint32_t pinCode = blt_crypto_alg_g2(proc->sc_pk_peer, proc->sc_pk_own, proc->peer_rand, proc->own_rand);
                /* because Secure Connections cryptographic functions use big-endian mode, so need swap the data */
                swapN((uint8_t *)&pinCode, 4);
                /* pincode is 6-digit, valid value is 000000 - 999999 */
                pinCode = pinCode % 1000000;

                //Slave displays 6 bit  numeric comparison value, start a special task
                //waiting for peripheral confirmation: 'YES'(pairing) or 'NO'(cancel).
                proc->tk_status = BLE_SMP_TK_STATE_NC;
                
                // send Vb to upper layer, upper layer should display it, and  confirm check if this
                // value match peer device's displaying result, then confirm it by calling
                // "" sending YES or NO to smp layer
                if (gap_eventMask & GAP_EVT_MASK_SMP_TK_NUMERIC_COMPARE) {
                    gap_smp_TkDisplayEvt_t gapEvt;
                    gapEvt.connHandle = conn->conn_handle;
                    gapEvt.tk_pincode = pinCode;
                    blc_gap_send_event(GAP_EVT_SMP_TK_NUMERIC_COMPARE, (uint8_t*)&gapEvt, sizeof(gap_smp_TkDisplayEvt_t));
                }
            }
            //In Secure connections Passkey entry protocol.
            else if (proc->smp_method & SMP_METHOD_PKE) {
                sc_passkey = bstream_to_u24_le(proc->pairing_tk); //proc->pairing_tk[2] << 16 | proc->pairing_tk[1] << 8 | proc->pairing_tk[0]);
                /* Core4.2 Vol3,Part H, Page2302
                 * Z is zero (i.e. 8 bits of zeros) for Numeric Comparison and OOB protocol. In the Passkey Entry protocol,
                 * the most significant bit of Z is set equal to one and the least significant bit is made up from one bit
                 * of the passkey e.g. if the passkey bit is 1, then Z = 0x81 and if the passkey bit is 0, then Z = 0x80.*/
                f4_param_z = ((sc_passkey >> proc->sc_passkey_cnt) & 0x01) | 0x80;
                proc->sc_passkey_cnt++;

                //Cai = f4(PKa,PKb,Nai,rbi)
                blt_crypto_alg_f4(pairing_conf, proc->sc_pk_peer, proc->sc_pk_own, proc->peer_rand, f4_param_z);

                //check if Cai=f4(Pka,Pkb,Nai,rbi).if check fails,about.
                if (memcmp(pairing_conf, proc->peer_confirm, 16) || proc->sc_passkey_cnt > 20) {
                    proc->sc_passkey_cnt = 0;
                    pairing_failed_reason = SMP_FAILED_CONFIRM_VALUE_FAILED;
                    goto fail;
                }
            }

            return ble_sm_pair_random_tx(conn);
        }
    } else { //Legacy SMP
        /* because Legacy SMP cryptographic toolbox functions use little-endian mode, so no need swap the data */
        smemcpy(proc->peer_rand, pairing_random->random, 16);

        if (smp_central_role) {
            /*
                    * Legacy Pairing:
                    * M->S Pairing Confirm:
                    * S->M Pairing Confirm: Pairing Confirm marked
                    * M->S Pairing Random:
                    * S->M Pairing Random: Pairing Random marked
                    * M->S LL_ENC_REQ: encryption start marked
                    */
            if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_CFM))) { //Ensure the integrity of the pairing process
                pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                goto fail;
            }

            proc->smp_phase_chk = BLE_SMP_PAIRING_PHASE_2_ENC;

            /* Checkout Sconfirm
            *    Sconfirm = c1(TK, Srand, Pairing Request command, Pairing Response command,
            *                     initiating device address type, initiating device address,
            *                     responding device address type, responding device address)
            **/
            blt_crypto_alg_c1(pairing_conf, proc->pairing_tk, proc->peer_rand, (uint8_t *)&proc->pairing_rsp, (uint8_t *)&proc->pairing_req, conn->own_ota_addr.type, conn->own_ota_addr.val, conn->peer_ota_addr.type, conn->peer_ota_addr.val);

            if (!memcmp(proc->peer_confirm, pairing_conf, 16)) {
                uint8_t stk_temp[16]; //aes_encryption_le in blt_crypto_alg_c1/blt_crypto_alg_s1 need critical data 4B aligned
                /* generate the STK during the LE legacy pairing process */
                blt_crypto_alg_s1(stk_temp, proc->pairing_tk, proc->own_rand, proc->peer_rand);
                /* shortcut for LTK according to the encryption key size */
                smemset(proc->peer_keys.ltk, 0, 16);
                smemcpy(proc->peer_keys.ltk, stk_temp, proc->enc_key_size);
                /* needless, initialize value are all zeros */
                proc->peer_keys.ediv = proc->peer_keys.rand_val = proc->peer_keys.ediv_rand_valid = 0;

                //smp4.0, after exchange smp random, then transport specific keys distribution
                proc->smpDistributeKeyOrder = BLE_SMP_DISTRIBUTE_KEY_START;

                struct ble_hci_le_start_encrypt_cp start_enc_cp;
                smemcpy(start_enc_cp.ltk, proc->peer_keys.ltk, 16);
                start_enc_cp.div = proc->peer_keys.ediv;
                start_enc_cp.rand = proc->peer_keys.rand_val;

                int rc = ble_host_hci_le_start_encryption(&start_enc_cp);
                if (rc != 0) {
                    BLE_HOST_L2CAP_SMP_ERROR("host hci le start encryption failed, rc = %d", rc);
                    pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                    goto fail;
                }
            } else {
                pairing_failed_reason = SMP_FAILED_CONFIRM_VALUE_FAILED;
                goto fail;
            }
        } else {
            /*
                    * Legacy Pairing:
                    * M->S Pairing Confirm: Pairing Confirm marked
                    * S->M Pairing Confirm:
                    * M->S Pairing Random: Pairing Random marked
                    * S->M Pairing Random:
                    */
            if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_CFM))) { //Ensure the integrity of the pairing process
                pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                goto fail;
            }

            proc->smp_phase_chk = BIT(BLE_SM_OP_PAIR_RANDOM);

            /* Checkout Mconfirm
             *    Mconfirm = c1(TK, Mrand, Pairing Request command, Pairing Response command,
             *                     initiating device address type, initiating device address,
             *                     responding device address type, responding device address)
             */
            blt_crypto_alg_c1(pairing_conf, proc->pairing_tk, proc->peer_rand, (uint8_t *)&proc->pairing_rsp, (uint8_t *)&proc->pairing_req, conn->peer_ota_addr.type, conn->peer_ota_addr.val, conn->own_ota_addr.type, conn->own_ota_addr.val);

            if (!memcmp(proc->peer_confirm, pairing_conf, 16)) {
                proc->smpDistributeKeyOrder = BLE_SMP_DISTRIBUTE_KEY_START;
                return ble_sm_pair_random_tx(conn);
            } else {
                pairing_failed_reason = SMP_FAILED_CONFIRM_VALUE_FAILED;
                goto fail;
            }
        }
    }

    return 0;

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_fail_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    BLE_HOST_SAL_ASSERT(smp_data != NULL);

    struct ble_smp_pairing_failed *fail_data = (struct ble_smp_pairing_failed *)smp_data;

    /* clear status and free the procedure control block */
    ble_smp_proc_pairing_end(conn, fail_data->reason);

    return 0;
}

static int ble_sm_enc_info_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    BLE_HOST_SAL_ASSERT(smp_data != NULL);

    struct ble_smp_encryption_info *enc_info = (struct ble_smp_encryption_info *)smp_data;

    uint8_t pairing_failed_reason;

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    proc->peerKey_mask |= BLE_SMP_KEY_MASK_ENC;

    if (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) {
        proc->peer_keys.ltk_valid = 1;
        smemcpy(proc->peer_keys.ltk, enc_info->ltk, 16);
    } else {
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    return 0;

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_master_id_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    BLE_HOST_SAL_ASSERT(smp_data != NULL);


    uint8_t pairing_failed_reason;

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    if (proc->peerKey_mask & BLE_SMP_KEY_MASK_ENC) {
        proc->peerKey_mask &= ~BLE_SMP_KEY_MASK_ENC;
    } else { //check err: 1.LTK (lost) + 2.EDIV+RANDOM
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    struct ble_smp_central_id *central_id = (struct ble_smp_central_id *)smp_data;

    proc->smp_DistributeKeyRecv.encKey = 0;

    if (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) {
        //The central roles share a set of parameters, and it is important to note the reuse situation here.
        proc->peer_keys.ediv = central_id->ediv;
        proc->peer_keys.rand_val = central_id->rand_val;
        proc->peer_keys.ediv_rand_valid = 1;

        //Slave send key completely. Master start key sending
        if (proc->smp_DistributeKeyRecvValue == 0) {
            proc->key_distribute = 1;
        }
    } else {
        //[Note]: peripheral is not save EDIV and Random of central
        //if sending key and receiving key all completed, process pairing end
        if (!proc->smp_DistributeKeyRecvValue && !proc->smp_DistributeKeySendValue) {
            //blt_smp_saveBondingKey(connHandle);
            struct smp_bonding_keys keys; //TODO:
            ble_smp_store_keys_write(conn, &keys);
        }
    }

    return 0;

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_id_info_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    BLE_HOST_SAL_ASSERT(smp_data != NULL);

    uint8_t pairing_failed_reason;

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    struct ble_smp_id_info *id_info = (struct ble_smp_id_info *)smp_data;

    proc->peerKey_mask |= BLE_SMP_KEY_MASK_IDENTITY;

    smemcpy(proc->peer_keys.irk, id_info->irk, 16);

    return 0;

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_id_addr_info_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    BLE_HOST_SAL_ASSERT(smp_data != NULL);

    uint8_t pairing_failed_reason;

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    if (proc->peerKey_mask & BLE_SMP_KEY_MASK_IDENTITY) {
        proc->peerKey_mask &= ~BLE_SMP_KEY_MASK_IDENTITY;
    } else { //check err: 1.IRK (lost) + 2.IADR
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    struct ble_smp_id_addr_info *id_addr_info = (struct ble_smp_id_addr_info *)smp_data;

    proc->smp_DistributeKeyRecv.idKey = 0;

    proc->peer_keys.addr_valid = 1;
    proc->peer_keys.addr_type = id_addr_info->addrType;
    smemcpy(proc->peer_keys.addr, id_addr_info->bd_addr, 6);

    if (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) {
        //if sending key and receiving key all completed, process pairing end
        if (proc->smp_DistributeKeyRecvValue == 0) {
            proc->key_distribute = 1;
            BLE_HOST_L2CAP_SMP_DEBUG("key distribute begin");
        }
    } else {
        //It is found that the resolvable random address when the Redmi Note3 is paired, the public address given in
        //the key distribution, and the public address direct adv is not connected. The connectAddr and the identityAddr
        //may not be the same, so it needs to be distinguished. The resolving list needs to use this address.
        //Real central address from central

        //if sending key and receiving key all completed, process pairing end
        if (!proc->smp_DistributeKeyRecvValue && !proc->smp_DistributeKeySendValue) {
            //blt_smp_saveBondingKey(connHandle);
            struct smp_bonding_keys keys; //TODO:
            ble_smp_store_keys_write(conn, &keys);
        }
    }
    return 0;

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_sign_info_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    BLE_HOST_SAL_ASSERT(smp_data != NULL);

    uint8_t pairing_failed_reason;

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    proc->smp_DistributeKeyRecv.signKey = 0;

    /* check if the peer key mask is idle, if not, means the peer send the key distribute mismatch key distribution sequence */
    if (proc->peerKey_mask != BLE_SMP_KEY_MASK_IDLE) {
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    #if (0) //TODO: Currently not support CSRK
        proc->peer_keys.csrk_valid = 1;
        smemcpy(proc->peer_keys.csrk, req->data , 16);
    #endif

    if (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) {
        //if sending key and receiving key all completed, process pairing end
        if (proc->smp_DistributeKeyRecvValue == 0) {
            proc->key_distribute = 1;
        }
    } else {
        #if 0 // @@@ do later: process signCounter

        #endif

        //if sending key and receiving key all completed, process pairing end
        if (!proc->smp_DistributeKeyRecvValue && !proc->smp_DistributeKeySendValue) {
            //blt_smp_saveBondingKey(connHandle);
            struct smp_bonding_keys keys; //TODO:
            ble_smp_store_keys_write(conn, &keys);
        }
    }
    return 0;

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_sec_req_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    BLE_HOST_SAL_ASSERT(smp_data != NULL);

    uint8_t pairing_failed_reason;

#if BROADCOM_WORKAROUND
    if (1) {
        BLE_HOST_L2CAP_SMP_DEBUG("re-connection, encryption start", );

        struct ble_hci_le_start_encrypt_cp start_enc_cp;
        smemset(start_enc_cp.ltk, 0, 16);
        start_enc_cp.div = 0;
        start_enc_cp.rand = 0;

        int rc = ble_host_hci_le_start_encryption(&start_enc_cp);
        if (rc != 0) {
            BLE_HOST_L2CAP_SMP_ERROR("host hci le start encryption failed, rc = %d", rc);
            pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
            goto fail;
        }

        return 0;
    }
#endif

    struct ble_smp_security_request *sec_req = (struct ble_smp_security_request *)smp_data;
    bool initiate_pairing = true;

    /* If the peer is requesting a bonded connection, query database for an
     * LTK corresponding to the sender.
     */
    if (sec_req->authReq.bondingFlags & SMP_BONDING_FLAG_BONDING) {
        struct ble_sm_keys peer_keys;
        /* Check if we have a key corresponding to the peer's address.  */
        if (!ble_smp_store_keys_read(conn, (struct smp_bonding_keys *)&peer_keys)) { //Success
            BLE_HOST_L2CAP_SMP_DEBUG("Found Bonding keys corresponding to the peer's address: %s", hex_to_str(conn->peer_ota_addr.val, 6));
            /* Found a key corresponding to this peer.  Make sure it meets the requested minimum authreq:
                * - A device sets the MITM flag to one to request an Authenticated security property for 
                *   the STK when using LE legacy pairing and the LTK when using LE Secure Connections.
                */
            uint8_t dev_pairing_status = peer_keys.dev_pairing_status;
            uint8_t auth_req_mitm = sec_req->authReq.MITM;
            if (auth_req_mitm &&  dev_pairing_status < AUTH_LTK_OR_STK) {
                BLE_HOST_L2CAP_SMP_DEBUG("MITM flag set but device does not have an Authenticated LTK or STK");
            } else {
                /* Initiate the encryption procedure for the specified connection later. */
                initiate_pairing = false;
            }
        } else { //Fail
            /* No key found corresponding to this peer. */
            BLE_HOST_L2CAP_SMP_DEBUG("No Bonding keys found corresponding to the peer's address: %s", hex_to_str(conn->peer_ota_addr.val, 6));
        }
    }

    if (initiate_pairing == true) {
        BLE_HOST_L2CAP_SMP_DEBUG("Initiate the pairing procedure for the specified connection.");

        /**
         * Initialize SMP parameters setting,
         * when ACL connected, gap callback the Upper user, user can change SMP settings
         */
        
        //TODO: default use host SM settings, but can be changed by user

        /* Refer to Core5.2 Spec | Vol 3, Part C page 1375
         * A device may be in a Secure Connections Only mode. When in Secure Connections Only mode only security
         * mode 1 level 4 shall be used except for services that only require security mode 1 level 1.
         */
        uint8_t level4only = ((conn->sm_settings.sm_sec_lvl & LE_SECURITY_MODE_1) == LE_SECURITY_MODE_1_LEVEL_4) ? 1 : 0;

        if (!sec_req->authReq.sc && level4only) {
            pairing_failed_reason = SMP_FAILED_AUTH_REQ;
            goto fail;
        }

        return ble_smp_pair_initiate(conn, TRUE);
    } else {
        BLE_HOST_L2CAP_SMP_DEBUG("Initiate the encryption procedure for the specified connection.");

        return ble_smp_encryption_initiate(conn);
    }

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_sc_public_key_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    uint8_t pairing_failed_reason = 0;

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    struct ble_smp_pairing_public_key *public_key = (struct ble_smp_pairing_public_key *)smp_data;
    bool smp_central_role = (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) ? 1 : 0;

    if (proc->sc_pairing) {
        /* because Secure Connections cryptographic functions use big-endian mode, over the air use little-endian mode, so need swap the data */
        swapX(public_key->keyX, proc->sc_pk_peer, 32);
        swapX(public_key->keyY, proc->sc_pk_peer + 32, 32);

        /*
         * Secure Connection Pairing:
         * M->S Pairing Req: phase1 begin
         * S->M Pairing Rsp:
         * M->S Pairing Public Key: phase2 begin
         * S->M Pairing Public Key:
         */
        if (proc->smp_phase_chk != BLE_SMP_PAIRING_PHASE_1_OK) {
            pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
            goto fail;
        }

        proc->smp_phase_chk = BIT(BLE_SM_OP_PAIR_PUBLIC_KEY);

        if (smp_central_role) {
            /*
            * Fix << CVE-2020-26558 >>
            * Issue:
            * . The researchers identified that it was possible for an attacker acting as a MITM in the Passkey
            *   authentication procedure to use a crafted series of responses to determine each bit of the randomly
            *   generated Passkey selected by the pairing initiator in each round of the pairing procedure, and
            *   once identified, to use these Passkey bits during the same pairing session to successfully complete
            *   the authenticated pairing procedure with the responder.
            *
            * The Bluetooth Special Interest Group (SIG) is making the following recommendations for circumventing
            * this attack:
            *  For the attack to succeed it is necessary for the pairing device to accept the same public key that it
            *  provided to the remote peer as the remote peer's public key. Devices should not accept their own public
            *  key from a peer during a pairing session. The pairing procedure should be terminated with a failure
            *  status if this occurs.
            */
            if (memcmp(proc->sc_pk_own, proc->sc_pk_peer, 64) == 0) {
                pairing_failed_reason = SMP_FAILED_INVALID_PARAMETERS;
                goto fail;
            }

            if (proc->smp_method & SMP_METHOD_LESC_PKI) {
                /* request Upper layer to input Pincode value, expect upper layer call "blc_smp_setTK_by_PasskeyEntry" to set Pincode */
                proc->tk_status = BLE_SMP_TK_STATE_REQUEST;
                
                /* notify upper layer to input Pincode value */
                if (gap_eventMask & GAP_EVT_MASK_SMP_TK_REQUEST_PASSKEY) {
                    gap_smp_TkReqPassKeyEvt_t gapEvt;
                    gapEvt.connHandle = conn->conn_handle;
                    blc_gap_send_event(GAP_EVT_SMP_TK_REQUEST_PASSKEY, (uint8_t*)&gapEvt, sizeof(gap_smp_TkReqPassKeyEvt_t));
                }

                /* check if need to send pairing confirm to peer or use pending flag to do it later */
                if ((proc->tk_status & BLE_SMP_TK_STATE_REQUEST) && !(proc->tk_status & BLE_SMP_TK_STATE_UPDATE)) {
                    proc->tk_status |= BLE_SMP_TK_STATE_CONFIRM_PENDING; //pending
                    /* attention: to do pending process */
                    return 0;
                } else {
                    /* already get Pincode value, send pairing confirm to peer */
                    return ble_sm_pair_confirm_tx(conn);
                }
            } else if (proc->smp_method & SMP_METHOD_LESC_PKD) {
                /* send pairing confirm to peer */
                return ble_sm_pair_confirm_tx(conn);
            } else if (proc->smp_method & SMP_METHOD_LESC_OOB) { 
                /* sc_oob confirm check */
                if (proc->scoob_remote) {
                    BLE_HOST_L2CAP_SMP_DEBUG("sc_pk_peer(be):%s", hex_to_str(proc->sc_pk_peer, 64));
                    BLE_HOST_L2CAP_SMP_DEBUG("scoob_remote-r(be):%s", hex_to_str(proc->scoob_remote->random, 16));
                    
                    uint8_t confirm[16];
                    /* Confirm = f4(PK,PK,random,0) */
                    blt_crypto_alg_f4(confirm, proc->sc_pk_peer, proc->sc_pk_peer, proc->scoob_remote->random, 0);

                    BLE_HOST_L2CAP_SMP_DEBUG("blt_crypto_alg_f4(be):%s", hex_to_str(confirm, 16));

                    /* Confirm unmatch, pairing failed */
                    if (memcmp(confirm, proc->scoob_remote->confirm, sizeof(confirm))) {
                        pairing_failed_reason = SMP_FAILED_CONFIRM_VALUE_FAILED;
                        goto fail;
                    }
                }

                /* send pairing random to peer */
                return ble_sm_pair_random_tx(conn);
            }
        } else { //peripheral SC
            if (proc->smp_method & SMP_METHOD_LESC_OOB) {
                /* Request SC OOB data event to upper layer, expect upper layer call "blc_smp_setSC_OOB_data" to set OOB data */
                if (gap_eventMask & GAP_EVT_MASK_SMP_REQUEST_SCOOB_DATA) {
                    proc->tk_status = BLE_SMP_TK_STATE_REQUEST;
                    //oob data check
                    bool scOobLocalUsed, scOobRemoteUsed;
                    //see peer's flag decide own sc oob data used
                    scOobLocalUsed  = (proc->pairing_req.oobDataFlag);
                    scOobRemoteUsed = (proc->pairing_rsp.oobDataFlag);

                    //clear
                    proc->scoob_local = NULL;
                    proc->scoob_remote = NULL;
                    proc->scoob_local_key = NULL;

                    gap_smp_requestScOobDataEvt_t gapEvt;
                    gapEvt.connHandle = conn->conn_handle;
                    gapEvt.scOobLocalUsed = scOobLocalUsed;
                    gapEvt.scOobRemoteUsed = scOobRemoteUsed;
                    blc_gap_send_event(GAP_EVT_SMP_REQUEST_SCOOB_DATA,  (uint8_t*)&gapEvt, sizeof(gap_smp_requestScOobDataEvt_t));
                    
                    /* attention: to do pending process, here do noting */
                    return 0; 
                }
            }

            /* send public key to peer */
            int rc = ble_sm_public_key_tx(conn);
            if (rc != 0) {
                BLE_HOST_L2CAP_SMP_ERROR("host hci send public key to peer failed, rc = %d", rc);
                pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                goto fail;
            }

            if (proc->smp_method & (SMP_METHOD_LESC_NC | SMP_METHOD_LESC_JW)) {
                /* send DHKey check to peer */
                return ble_sm_pair_confirm_tx(conn);
            } else if (proc->smp_method & SMP_METHOD_LESC_PKI) {
                /* request Upper layer to input Pincode value, expect upper layer call "blc_smp_setTK_by_PasskeyEntry" to set Pincode */
                proc->tk_status = BLE_SMP_TK_STATE_REQUEST;

                /* notify upper layer to input Pincode value */
                if (gap_eventMask & GAP_EVT_MASK_SMP_TK_REQUEST_PASSKEY) {
                    gap_smp_TkReqPassKeyEvt_t gapEvt;
                    gapEvt.connHandle = conn->conn_handle;
                    blc_gap_send_event(GAP_EVT_SMP_TK_REQUEST_PASSKEY, (uint8_t*)&gapEvt, sizeof(gap_smp_TkReqPassKeyEvt_t));
                }
            }

            /* need do nothing, after pairing confirm rx, will check if already get Pincode value */
            return 0; 
        }
    } else { //Legacy SMP
        pairing_failed_reason = SMP_FAILED_PAIRING_NOT_SUPPORTED;
        goto fail;
    }

    return 0;

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_sc_dhkey_check_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    uint8_t pairing_failed_reason;

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    struct ble_smp_pairing_dhkey_check *dhkey_check = (struct ble_smp_pairing_dhkey_check *)smp_data;

    bool smp_central_role = (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) ? 1 : 0;

    if (proc->sc_pairing) {
        uint8_t confirm_Ex[16] = {0};
        /* because Secure Connections cryptographic functions use big-endian mode, over the air use little-endian mode, so need swap the data */
        swapX(dhkey_check->dhkeyCheck, confirm_Ex, 16);

        uint8_t bd_addr_init[7] = {0};
        uint8_t bd_addr_rsp[7]  = {0};
        uint8_t ioCapA[3]       = {0};
        uint8_t ioCapB[3]       = {0};

        if (smp_central_role) {
            /*
            * Secure Connection Pairing:
            * M->S Pairing Req: phase1 begin
            * S->M Pairing Rsp:
            * M->S Pairing Public Key: phase2 begin
            * S->M Pairing Public Key:
            * ......
            * M->S Pairing Random:
            * S->M Pairing Random: Pairing Random marked
            * M->S Pairing DHKey Check:
            * S->M Pairing DHKey Check: Pairing DHKey Check marked
            */
            if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_RANDOM))) {
                pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                goto fail;
            }

            proc->smp_phase_chk = BIT(BLE_SM_OP_PAIR_DHKEY_CHECK);

            uint8_t confirm_eb[16] = {0};
            //A = BD_ADDR of A used during pairing
            bd_addr_init[0] = conn->own_ota_addr.type;
            /* because Secure Connections cryptographic functions use big-endian mode, over the air use little-endian mode, so need swap the data */
            swapX(conn->own_ota_addr.val, bd_addr_init + 1, 6);

            //B = BD_ADDR of B used during pairing
            bd_addr_rsp[0] = conn->peer_ota_addr.type;
            /* because Secure Connections cryptographic functions use big-endian mode, over the air use little-endian mode, so need swap the data */
            swapX(conn->peer_ota_addr.val, bd_addr_rsp + 1, 6);

            //check confirm Eb.
            //pairing_tk: in security connection to keep own random. rb
            //pairing_confirm: in security connection oob mode to keep peer random. ra
            ioCapA[0] = (uint8_t)proc->pairing_req.authReqValue;
            ioCapA[1] = proc->pairing_req.oobDataFlag;
            ioCapA[2] = proc->pairing_req.ioCapability;

            ioCapB[0] = (uint8_t)proc->pairing_rsp.authReqValue;
            ioCapB[1] = proc->pairing_rsp.oobDataFlag;
            ioCapB[2] = proc->pairing_rsp.ioCapability;

            uint8_t *ra = proc->pairing_tk;

            if (proc->smp_method & SMP_METHOD_LESC_OOB) {
                //if Device B's IO data flag does not indicate OOB authentication data present, set ra = 0.
                //if remote SC OOB flag exist, local central SC OOB data used, else ra = 0.
                if (proc->scoob_local) {
                    ra = proc->scoob_local->random;
                }
                BLE_HOST_L2CAP_SMP_DEBUG("M Eb check: ra (be) %s", hex_to_str(ra, 16));
            }

            //Eb = f6(MacKey,Nb,Na,ra,IOcapB,B,A)
            blt_crypto_alg_f6(confirm_eb, proc->sc_mac_key, proc->peer_rand, proc->own_rand, ra, ioCapB, bd_addr_rsp, bd_addr_init);

            // check DHkey parameter Eb fails
            if (memcmp(confirm_eb, confirm_Ex, 16)) {
                pairing_failed_reason = SMP_FAILED_DHKEY_CHECK_FAILED;
                goto fail;
            } else {
                if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_DHKEY_CHECK))) {
                    pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                    goto fail;
                }

                proc->smp_phase_chk = BLE_SMP_PAIRING_PHASE_2_ENC;

                /*
                 * The SC does not need to store the counterpart's LTK. Considering that the legacy pairing stores
                 * the LTK of the peer, in the case of SC, the local LTK is copied to the peer LTK for storage. (
                 * In the case of SC, the central and peripheral LTK are the same).
                 */
                smemcpy(proc->peer_keys.ltk, proc->own_ltk, 16);
                proc->peer_keys.ltk_valid = 1;

                BLE_HOST_L2CAP_SMP_DEBUG("Initiate the encryption procedure for the specified connection.");

                struct ble_hci_le_start_encrypt_cp start_enc_cp;
                smemcpy(start_enc_cp.ltk, proc->peer_keys.ltk, 16);
                /* for secure connections, EDIV and RAND are all zero */
                start_enc_cp.div = start_enc_cp.rand = 0;

                int rc = ble_host_hci_le_start_encryption(&start_enc_cp);
                if (rc != 0) {
                    BLE_HOST_L2CAP_SMP_ERROR("host hci le start encryption failed, rc = %d", rc);
                    pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                    goto fail;
                }
                
                /* after exchange smp pairing DHkey check, then transport specific keys distribution */
                proc->smpDistributeKeyOrder = BLE_SMP_DISTRIBUTE_KEY_START;
            }
        } else { //smp peripheral role
            uint8_t confirm_ea[16] = {0};
            //A = BD_ADDR of A used during pairing
            bd_addr_init[0] = conn->peer_ota_addr.type;
            /* because Secure Connections cryptographic functions use big-endian mode, so need swap the data */
            swapX(conn->peer_ota_addr.val, bd_addr_init + 1, 6);

            //B = BD_ADDR of B used during pairing
            bd_addr_rsp[0] = conn->own_ota_addr.type;
            /* because Secure Connections cryptographic functions use big-endian mode, so need swap the data */
            swapX(conn->own_ota_addr.val, bd_addr_rsp + 1, 6);

            /* MacKey || LTK = f5(DHKey, Na, Nb,A,B) */
            blt_crypto_alg_f5(proc->sc_mac_key, proc->own_ltk, proc->sc_sk_dhk_own, proc->peer_rand, proc->own_rand, bd_addr_init, bd_addr_rsp);
            /* because Secure Connections cryptographic functions use big-endian mode, LTK use little-endian mode in code, so need swap the data */
            swapN(proc->own_ltk, 16);

            /* Compute confirm Ea.
             * pairing_tk: in security connection to keep own random. rb
             * peer_confirm: in security connection oob mode to keep peer random. 
             */
            ioCapA[0] = (uint8_t)proc->pairing_req.authReqValue;
            ioCapA[1] = proc->pairing_req.oobDataFlag;
            ioCapA[2] = proc->pairing_req.ioCapability;

            ioCapB[0] = (uint8_t)proc->pairing_rsp.authReqValue;
            ioCapB[1] = proc->pairing_rsp.oobDataFlag;
            ioCapB[2] = proc->pairing_rsp.ioCapability;

            if (proc->smp_method & SMP_METHOD_LESC_PKE) {
                /* because Secure Connections cryptographic functions use big-endian mode, so need swap the data */
                swapN(proc->pairing_tk, 16);
            } else {
                /* except LESC PKE, other method pairing tk is all zero */
                memset(proc->pairing_tk, 0, 16);
            }

            uint8_t *rb = proc->pairing_tk;

            if (proc->smp_method & SMP_METHOD_LESC_OOB) {
                //if Device A's IO data flag does not indicate OOB authentication data present, set rb = 0.
                //if remote central SC OOB flag exist, local SC OOB data used, else set rb = 0.
                if (proc->scoob_local) {
                    rb = proc->scoob_local->random;
                }
                BLE_HOST_L2CAP_SMP_DEBUG("S Ea check: rb (be) %s", hex_to_str(rb, 16));
            }

            //Ea = f6(MacKey,Na,Nb,rb,IOcapA,A,B)
            blt_crypto_alg_f6(confirm_ea, proc->sc_mac_key, proc->peer_rand, proc->own_rand, rb, ioCapA, bd_addr_init, bd_addr_rsp);

            //BLE_HOST_L2CAP_SMP_DEBUG("confirm_ea:(be) %s", hex_to_str(confirm_ea, 16));

            bool dhkey_check_pass = !memcmp(confirm_ea, confirm_Ex, 16); //Ea == f6(MacKey,Na,Nb,rb,IOcapA,A,B). check pass

            if (dhkey_check_pass) {
                uint8_t *ra = proc->pairing_tk;

                if (proc->smp_method & SMP_METHOD_LESC_OOB) {
                    //if Device B's IO data flag does not indicate OOB authentication data present, set ra = 0.
                    //if local SC OOB flag exist, remote central SC OOB data used, else ra = 0.
                    if (proc->scoob_remote) {
                        ra = proc->scoob_remote->random;
                    }
                    BLE_HOST_L2CAP_SMP_DEBUG("S Eb check: ra (be) %s", hex_to_str(ra, 16));
                }

                // check pass
                // 10b. compute Eb , keep in confirm temp
                //Eb = f6(MacKey,Nb,Na,ra,IOcapB,B,A)
                blt_crypto_alg_f6(proc->peer_confirm, proc->sc_mac_key, proc->own_rand, proc->peer_rand, ra, ioCapB, bd_addr_rsp, bd_addr_init);
            }

            //if select numeric comparison, to execute here, Central must already confirmed 'YES'(Pairing).
            if (proc->smp_method & SMP_METHOD_LESC_NC) {
                if (proc->tk_status & BLE_SMP_TK_STATE_NC) {
                    if (proc->tk_status & BLE_SMP_TK_STATE_NC_CHECK_YES) { //NC confirmed "YES", set by upper layer completed
                        proc->tk_status = 0;
                    } else if (proc->tk_status & BLE_SMP_TK_STATE_NC_CHECK_NO) { //NC confirmed "NO", set by upper layer completed
                        proc->tk_status = 0;

                        /* See the Core_v5.0(Vol 3/Part H/3.5.5/Pairing Failed) for more information.
                         * NOTICE: test by smart phone, central send unspecified reason when press "NO" button! 
                         */
                        pairing_failed_reason = SMP_FAILED_NUMERIC_COMPARISION_FAILED;
                        goto fail;
                    } else { // check it in gap mainLoop, if NC confirmed "YES", send peer_confirm to peer device
                        proc->tk_status |= dhkey_check_pass ? BLE_SMP_TK_STATE_NC_DHKEY_SUCC_PENDING : BLE_SMP_TK_STATE_NC_DHKEY_FAIL_PENDING;
                        /* attention: to do pending process, here do noting */
                        return 0;
                    }
                }
            }

            if (dhkey_check_pass) {
                /* after exchange smp pairing DH key check, then transport specific keys distribution */
                proc->smpDistributeKeyOrder = BLE_SMP_DISTRIBUTE_KEY_START;
                /* send peer_confirm to peer device */
                return ble_sm_dhkey_check_tx(conn);
            } else {
                pairing_failed_reason = SMP_FAILED_DHKEY_CHECK_FAILED;
                goto fail;
            }
        }
    } else { //Legacy SMP
        pairing_failed_reason = SMP_FAILED_PAIRING_NOT_SUPPORTED;
        goto fail;
    }

    return 0;

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_sc_keypress_rx (struct ble_host_conn *conn, struct smp_cmd_fmt *smp_data)
{
    uint8_t pairing_failed_reason;

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    bool smp_central_role = (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) ? 1 : 0;

    if(!smp_central_role || !proc->sc_pairing) {
        pairing_failed_reason = SMP_FAILED_COMMAND_NOT_SUPPORTED;
        goto fail;
    } else {
        if (gap_eventMask & GAP_EVT_MASK_SMP_KEYPRESS_NOTIFY) {
            gap_smp_keypressNotifyEvt_t gapEvt;
            gapEvt.connHandle = conn->conn_handle;
            gapEvt.ntfType = ((struct ble_smp_keypress_notification *)smp_data)->notificationType;
            blc_gap_send_event(GAP_EVT_SMP_KEYPRESS_NOTIFY, (uint8_t *)&gapEvt, sizeof(gap_smp_keypressNotifyEvt_t));
        }
    }

    /* success, do nothing */
    return 0;

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}
































static int ble_sm_pair_req_tx(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);
        
    proc->smp_timeout_start_tick = ble_host_sal_get_current_time() | 1; //reset timeout timer

    return ble_host_smp_send_data_sync(conn, (uint8_t *)&proc->pairing_req.code, sizeof(proc->pairing_req));
}

static int ble_sm_pair_rsp_tx(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);
    
    proc->smp_timeout_start_tick = ble_host_sal_get_current_time() | 1; //reset timeout timer

    return ble_host_smp_send_data_sync(conn, (uint8_t *)&proc->pairing_rsp.code, sizeof(proc->pairing_rsp));
}

static int ble_sm_pair_confirm_tx(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);

    uint8_t pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
    bool smp_central_role = (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) ? 1 : 0;

    struct ble_smp_pairing_confirm pairing_confirm;
    pairing_confirm.code = BLE_SM_OP_PAIR_CFM;
    uint8_t *pConfirm = &pairing_confirm.confirm[0];

    if(proc->sc_pairing) {
        uint32_t sc_passkey = bstream_to_u24_le(proc->pairing_tk); //proc->pairing_tk[2] << 16 | proc->pairing_tk[1] << 8 | proc->pairing_tk[0]);
        /*core4.2 Vol3,Part H, Page2302
                Z is zero (i.e. 8 bits of zeros) for Numeric Comparison and OOB protocol. In the
                Passkey Entry protocol, the most significant bit of Z is set equal to one and the
                least significant bit is made up from one bit of the passkey e.g. if the passkey
                bit is 1, then Z = 0x81 and if the passkey bit is 0, then Z = 0x80.*/
        uint8_t f4_param_z;

        /* In Secure connections Passkey entry protocol. */
        if (proc->smp_method & SMP_METHOD_PKE) {
            f4_param_z = ((sc_passkey >> proc->sc_passkey_cnt) & 0x01) | 0x80;
            ble_host_hci_le_gen_rand(proc->own_rand, 16);

            if (!smp_central_role) {
                /*
                        * Secure Connection Pairing:
                        * M->S Pairing Req: phase1 begin
                        * S->M Pairing Rsp:
                        * M->S Pairing Public Key: phase2 begin
                        * S->M Pairing Public Key:
                        * ......
                        * M->S Pairing Confirm: (20 times)
                        * S->M Pairing Confirm: (20 times) Pairing Confirm marked
                        */
                if (proc->sc_passkey_cnt == 19) { //20 times confirm/random, idx from Zero.
                    if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_PUBLIC_KEY))) {
                        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                        goto fail;
                    }

                    proc->smp_phase_chk = BIT(BLE_SM_OP_PAIR_CFM);
                }
            }
        } else { //Numeric Comparison(or Just work) and OOB protocol
            f4_param_z = 0;

            if (!smp_central_role) {
                /*
                        * Secure Connection Pairing:
                        * M->S Pairing Req: phase1 begin
                        * S->M Pairing Rsp:
                        * M->S Pairing Public Key: phase2 begin
                        * S->M Pairing Public Key:
                        * S->M Pairing Confirm: Pairing Confirm marked
                        */
                if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_PUBLIC_KEY))) {
                    pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                    goto fail;
                }

                proc->smp_phase_chk = BIT(BLE_SM_OP_PAIR_CFM);
            }
        }
        /* Cb = f4(PKb,PKa,Nb,0) */
        blt_crypto_alg_f4(pConfirm, proc->sc_pk_own, proc->sc_pk_peer, proc->own_rand, f4_param_z);
        /* because Secure Connections cryptographic functions use big-endian mode, over the air use little-endian mode, so need swap the data */
        swapN(pConfirm, 16);
    } else {
        uint8_t *ia  = conn->peer_ota_addr.val;
        uint8_t  iat = conn->peer_ota_addr.type;
        uint8_t *ra  = conn->own_ota_addr.val;
        uint8_t  rat = conn->own_ota_addr.type;

        if (smp_central_role) {
            ia  = conn->own_ota_addr.val;
            iat = conn->own_ota_addr.type;
            ra  = conn->peer_ota_addr.val;
            rat = conn->peer_ota_addr.type;
        }

       /* Sconfirm = c1(TK, Srand,Pairing Request command, Pairing Response command,
        *                initiating device address type, initiating device address,
        *                responding device address type, responding device address)
        **/
        blt_crypto_alg_c1(pConfirm, proc->pairing_tk, proc->own_rand, (uint8_t *)&proc->pairing_rsp, (uint8_t *)&proc->pairing_req, iat, ia, rat, ra);
    }

    proc->smp_timeout_start_tick = ble_host_sal_get_current_time() | 1; //reset timeout timer

    return ble_host_smp_send_data_sync(conn, (uint8_t *)&pairing_confirm.code, sizeof(struct ble_smp_pairing_confirm));

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_pair_random_tx(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);

    uint8_t pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
    bool smp_central_role = (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) ? 1 : 0;

    struct ble_smp_pairing_random smp_tx_pkt;
    smp_tx_pkt.code = BLE_SM_OP_PAIR_RANDOM;

    if(proc->sc_pairing) {
        if (proc->smp_method & SMP_METHOD_PKE) {
            if (!smp_central_role) {
                /*
                        * Secure Connection Pairing:
                        * M->S Pairing Req: phase1 begin
                        * S->M Pairing Rsp:
                        * M->S Pairing Public Key: phase2 begin
                        * S->M Pairing Public Key:
                        * ......
                        * M->S Pairing Confirm: (20 times) Pairing Confirm marked
                        * S->M Pairing Confirm: (20 times)
                        * M->S Pairing Random: (20 times) Pairing Random marked
                        * S->M Pairing Random: (20 times)
                        */
                if (proc->sc_passkey_cnt == 20) { //20 times confirm/random
                    if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_CFM))) {
                        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                        goto fail;
                    }

                    proc->sc_passkey_cnt = 0;
                    proc->smp_phase_chk  = BIT(BLE_SM_OP_PAIR_RANDOM);
                }
            }
        } else {
            if (proc->smp_method & SMP_METHOD_LESC_OOB) {
                return 0;
            }

            if (!smp_central_role) {
                /* Note: OOB is not take into account
                        * Secure Connection Pairing:
                        * M->S Pairing Req: phase1 begin
                        * S->M Pairing Rsp:
                        * M->S Pairing Public Key: phase2 begin
                        * S->M Pairing Public Key:
                        * S->M Pairing Confirm: Pairing Confirm marked
                        * M->S Pairing Random:
                        * S->M Pairing Random: Pairing Random marked
                        */
                if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_CFM))) {
                    pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                    goto fail;
                }

                proc->smp_phase_chk = BIT(BLE_SM_OP_PAIR_RANDOM);
            }
        }

        /* because Secure Connections cryptographic functions use big-endian mode, over the air use little-endian mode, so need swap the data */
        swapX(proc->own_rand, smp_tx_pkt.random, 16);
    } else { //Legacy SMP
        /* because Legacy SMP cryptographic toolbox functions use little-endian mode, so no need swap the data */
        smemcpy(smp_tx_pkt.random, proc->own_rand, 16);
    }

    proc->smp_timeout_start_tick = ble_host_sal_get_current_time() | 1; //reset timeout timer

    return ble_host_smp_send_data_sync(conn, (uint8_t *)&smp_tx_pkt, sizeof(struct ble_smp_pairing_failed));

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_pair_failed_tx(struct ble_host_conn *conn, uint8_t reason)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);

    /* clear status and free the procedure control block */
    ble_smp_proc_pairing_end(conn, reason);

    struct ble_smp_pairing_failed smp_tx_pkt;
    smp_tx_pkt.code = BLE_SM_OP_PAIR_FAIL;
    smp_tx_pkt.reason = reason;
    return ble_host_smp_send_data_sync(conn, (uint8_t *)&smp_tx_pkt, sizeof(struct ble_smp_pairing_failed));
}

static int ble_sm_enc_info_tx(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);

    proc->smp_timeout_start_tick = ble_host_sal_get_current_time() | 1; //reset timeout timer

    struct ble_smp_encryption_info smp_tx_pkt;
    smp_tx_pkt.code = BLE_SM_OP_ENC_INFO;
    smemcpy(smp_tx_pkt.ltk, proc->peer_keys.ltk, 16);

    return ble_host_smp_send_data_sync(conn, (uint8_t *)&smp_tx_pkt, sizeof(struct ble_smp_encryption_info));
}

static int ble_sm_master_id_tx(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);

    proc->smp_timeout_start_tick = ble_host_sal_get_current_time() | 1; //reset timeout timer

    /* generate random value for EDIV,RAND */
    uint8_t ediv_rand[10] = {0};

    /*
     * Standard method of generating EDIV: (currently the SDK uses random generation method)
     * dm(k, r) = e(k, r) mod 2^16,  Y = dm(DHK, Rand),  EDIV = Y xor DIV
     * EDIV = ( e(DHK, Rand || padding) mod 2^16)  xor 0xFFFF
     */
    ble_host_hci_le_gen_rand(ediv_rand, 10); //EDIV(2Byte) + Rand(8Byte)

    struct ble_smp_central_id smp_tx_pkt;
    smp_tx_pkt.code = BLE_SM_OP_MASTER_ID;
    smp_tx_pkt.ediv = *((uint16_t*)&ediv_rand[0]); //EDIV
    smp_tx_pkt.rand_val = *((uint64*)&ediv_rand[2]); //RAND

    //Notice: if it is a smp peripheral role, the value of ediv_rand is saved
    //using its own parameters, otherwise, the value of the peer device is used.
    if (conn->role == BLE_HCI_CONN_ROLE_PERIPHERAL) {
        proc->own_keys.ediv = smp_tx_pkt.ediv; //EDIV
        proc->own_keys.rand_val = smp_tx_pkt.rand_val; //RAND
        proc->own_keys.ediv_rand_valid = 1;
    } else {
        //If the current connection is the central role, do not store its own EDIV
        //and RAND, only store the EDIV and RAND distributed by the peripheral device.
    }

    return ble_host_smp_send_data_sync(conn, (uint8_t *)&smp_tx_pkt, sizeof(struct ble_smp_central_id));
}

static int ble_sm_id_info_tx(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);

    proc->smp_timeout_start_tick = ble_host_sal_get_current_time() | 1; //reset timeout timer

    struct ble_smp_id_info id_info;
    id_info.code = BLE_SM_OP_ID_INFO;
    smemcpy(id_info.irk, proc->own_keys.irk, 16);

    return ble_host_smp_send_data_sync(conn, (uint8_t *)&id_info, sizeof(struct ble_smp_id_info));
}

static int ble_sm_id_addr_info_tx(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);

    proc->smp_timeout_start_tick = ble_host_sal_get_current_time() | 1; //reset timeout timer

    /* get the Identify address of the current connection */
    proc->own_keys.addr_valid = 1;
    proc->own_keys.addr_type = conn->own_id_addr.type;
    smemcpy(proc->own_keys.addr, conn->own_id_addr.val, 6);

    struct ble_smp_id_addr_info id_addr_info;
    id_addr_info.code = BLE_SM_OP_ID_ADDR_INFO;
    smemcpy(id_addr_info.bd_addr, proc->own_keys.addr, 6);
    id_addr_info.addrType = proc->own_keys.addr_type;

    return ble_host_smp_send_data_sync(conn, (uint8_t *)&id_addr_info, sizeof(struct ble_smp_id_addr_info));
}

static int ble_sm_sign_info_tx(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);

    proc->smp_timeout_start_tick = ble_host_sal_get_current_time() | 1; //reset timeout timer

    /* generate own CSRK */
    proc->own_keys.csrk_valid = 1;
    ble_host_hci_le_gen_rand(proc->own_keys.csrk, 16); //CSRK(16Byte)

    struct ble_smp_signing_info sign_info;
    sign_info.code = BLE_SM_OP_SIGN_INFO;
    smemcpy(sign_info.csrk, proc->own_keys.csrk, 16);

    return ble_host_smp_send_data_sync(conn, (uint8_t *)&sign_info, sizeof(struct ble_smp_signing_info));
}

static int ble_sm_sec_request_tx(struct ble_host_conn *conn)
{
    if (conn->sm_settings.sm_sec_lvl == LE_SECURITY_MODE_1_LEVEL_1 || \
        conn->role == BLE_HCI_CONN_ROLE_CENTRAL) {
        BLE_HOST_L2CAP_SMP_ERROR("ACL CONN role [%d] or SEC LVL [%d] not allowed tx SEC REQ packet", conn->role, conn->sm_settings.sm_sec_lvl);
        return SMP_FAILED_COMMAND_NOT_SUPPORTED;
    }

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc != NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Already allocate the SMP procedure control block");
        return SMP_FAILED_REPEATED_ATTEMPTS;
    }

    /* allocate a new procedure for SMP procedure, already clear the memory content to zero in ble_smp_proc_alloc() */
    proc = ble_smp_proc_alloc();
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Allocate a new SMP procedure control block failed, not enough memory");
        return SMP_FAILED_UNSPECIFIED_REASON;
    }

    /* insert the new SMP procedure */
    ble_smp_proc_insert_new(proc);

    /* alloc to ACL connection handle */
    proc->conn_handle = conn->conn_handle;

    /**
     * Initialize SMP parameters setting,
     * when ACL connected, gap callback the Upper user, user can change SMP settings
     */
    proc->sm_cfg = conn->sm_settings; //TODO: default use host SM settings, but can be changed by user

    struct ble_smp_security_request smp_tx_pkt;
    memset((void*)&smp_tx_pkt, 0, sizeof(smp_tx_pkt)); //careful

    /* prepare security request packet */
    smp_tx_pkt.code = BLE_SM_OP_SEC_REQ;
    struct ble_smp_auth_req *auth = &smp_tx_pkt.authReq;
    auth->sc = proc->sm_cfg.sm_sc;
    auth->bondingFlags = proc->sm_cfg.sm_bonding;
    auth->keypress = proc->sm_cfg.sm_keypress;
    auth->MITM = proc->sm_cfg.sm_mitm;

    proc->smp_timeout_start_tick = ble_host_sal_get_current_time() | 1; //reset timeout timer

    return ble_host_smp_send_data_sync(conn, (uint8_t *)&smp_tx_pkt, sizeof(struct ble_smp_security_request));
}

static int ble_sm_public_key_tx(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);

    proc->smp_timeout_start_tick = ble_host_sal_get_current_time() | 1; //reset timeout timer

    struct ble_smp_pairing_public_key public_key;
    public_key.code = BLE_SM_OP_PAIR_PUBLIC_KEY;

    /* because Secure Connections cryptographic functions use big-endian mode, over the air use little-endian mode, so need swap the data */
    swapX(proc->sc_pk_own, public_key.keyX, 32);
    swapX(proc->sc_pk_own, public_key.keyY, 32);

    //Must MTU >= 65, TODO:
    return ble_host_smp_send_data_sync(conn,  (uint8_t *)&proc->pairing_rsp.code, sizeof(proc->pairing_rsp));
}

static int ble_sm_dhkey_check_tx(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);

    uint8_t pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
    bool smp_central_role = (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) ? 1 : 0;

    if(proc->sc_pairing) {
        struct ble_smp_pairing_dhkey_check dhkey_check;
        dhkey_check.code = BLE_SM_OP_PAIR_DHKEY_CHECK;

        if (smp_central_role) {
            uint8_t bd_addr_init[7] = {0};
            uint8_t bd_addr_rsp[7]  = {0};
            uint8_t ioCapA[3]       = {0};
            uint8_t confirm_ea[16]  = {0};

            //A = BD_ADDR of A used during pairing
            bd_addr_init[0] = conn->own_ota_addr.type;
            /* because Secure Connections cryptographic functions use big-endian mode, so need swap the data */
            swapX(conn->own_ota_addr.val, bd_addr_init + 1, 6);

            //B = BD_ADDR of B used during pairing
            bd_addr_rsp[0] = conn->peer_ota_addr.type;
            /* because Secure Connections cryptographic functions use big-endian mode, so need swap the data */
            swapX(conn->peer_ota_addr.val, bd_addr_rsp + 1, 6);

            //MacKey || LTK = f5(DHKey, Na, Nb,A,B)
            blt_crypto_alg_f5(proc->sc_mac_key, proc->own_ltk, proc->sc_sk_dhk_own, proc->own_rand, proc->peer_rand, bd_addr_init, bd_addr_rsp);
            /* because Secure Connections cryptographic functions use big-endian mode, LTK use little-endian mode in code, so need swap the data */
            swapN(proc->own_ltk, 16);

            ///////////////////////////// compute confirm Ea ////////////////////////////////////
            //pairing_tk: in security connection to keep own random. rb
            //pairing_confirm: in security connection oob mode to keep peer random. ra

            //IOcapA is from Pairing Request
            ioCapA[0] = (uint8_t)proc->pairing_req.authReqValue;
            ioCapA[1] = proc->pairing_req.oobDataFlag;
            ioCapA[2] = proc->pairing_req.ioCapability;

            if (proc->smp_method & SMP_METHOD_LESC_PKE) {
                /* because Secure Connections cryptographic functions use big-endian mode, so need swap the data */
                swapN(proc->pairing_tk, 16);
            } else {
                /* except LESC PKE, other method pairing tk is all zero */
                memset(proc->pairing_tk, 0, 16);
            }

            uint8_t *rb = proc->pairing_tk;

            if (proc->smp_method & SMP_METHOD_LESC_OOB) {
                //if Device A's IO data flag does not indicate OOB authentication data present, set rb = 0.
                //if local central SC OOB flag exist, remote SC OOB data used, else set rb = 0.
                if (proc->scoob_remote) {
                    rb = proc->scoob_remote->random;
                }
                BLE_HOST_L2CAP_SMP_DEBUG("M Ea check: rb (be) %s", hex_to_str(rb, 16));
            }

            //Ea = f6(MacKey,Na,Nb,rb,IOcapA,A,B)
            blt_crypto_alg_f6(confirm_ea, proc->sc_mac_key, proc->own_rand, proc->peer_rand, rb, ioCapA, bd_addr_init, bd_addr_rsp);

            /* because Secure Connections cryptographic functions use big-endian mode, over the air use little-endian mode, so need swap the data */
            swapX(confirm_ea, dhkey_check.dhkeyCheck, 16);
        } else { //smp peripheral role
            /*
            * Secure Connection Pairing:
            * M->S Pairing Req: phase1 begin
            * S->M Pairing Rsp:
            * M->S Pairing Public Key: phase2 begin
            * S->M Pairing Public Key:
            * ......
            * M->S Pairing Random:
            * S->M Pairing Random: Pairing Random marked
            * M->S Pairing DHKey Check: Pairing DHKey Check marked
            */
            if (!(proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_RANDOM))) {
                pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
                goto fail;
            }

            proc->smp_phase_chk = BIT(BLE_SM_OP_PAIR_DHKEY_CHECK);

            /* because Secure Connections cryptographic functions use big-endian mode, over the air use little-endian mode, so need swap the data */
            swapX(proc->peer_confirm, dhkey_check.dhkeyCheck, 16); //Eb Reuse Parameter: peer_confirm
        }
        /* send Pairing DHKey Check */
        return ble_host_smp_send_data_sync(conn,  (uint8_t *)&dhkey_check.code, sizeof(struct ble_smp_pairing_dhkey_check));
    } else { //Legacy SMP
        pairing_failed_reason = SMP_FAILED_PAIRING_NOT_SUPPORTED;
        goto fail;
    }

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_sm_keypress_tx(struct ble_host_conn *conn, enum smp_notification_type ntf_type)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);

    struct ble_smp_keypress_notification smp_tx_pkt;
    smp_tx_pkt.code = BLE_SM_OP_PAIR_KEYPRESS_NTF;
    smp_tx_pkt.notificationType = ntf_type;

    proc->smp_timeout_start_tick = ble_host_sal_get_current_time() | 1; //reset timeout timer

    return ble_host_smp_send_data_sync(conn, (uint8_t *)&smp_tx_pkt, sizeof(struct ble_smp_keypress_notification));
}

//__attribute__((unused))
int ble_sm_keypress_notify(uint16_t conn_handle, enum smp_notification_type ntf_type)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);
    if(conn == NULL) {
        return BLE_HOST_ERR_PARM;
    }

    return ble_sm_keypress_tx(conn, ntf_type);
}



bool ble_smp_cancel_auth(uint16_t conn_handle)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);
    if(conn == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Unknown Connection ID");
        return FALSE;
    }

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        return FALSE;   
    }

    if (!(proc->tk_status & BLE_SMP_TK_STATE_REQUEST)) {
        return FALSE;
    }

    uint8_t smp_err_code;
    switch (proc->stk_method) {
    case PK_Init_Display_Resp_Input:
    case PK_Resp_Display_Init_Input:
    case PK_BOTH_INPUT:
        smp_err_code = SMP_FAILED_PASSKEY_ENTRY_FAILED;
        break;
    case Numeric_Comparison:
        smp_err_code = SMP_FAILED_CONFIRM_VALUE_FAILED;
        break;
    case SC_OOB_Authentication:
    case OOB_Authentication:
        smp_err_code = SMP_FAILED_OOB_NOT_AVAILABLE;
        break;
    case JustWorks:
        smp_err_code = SMP_FAILED_UNSPECIFIED_REASON;
        break;
    default:
        BLE_HOST_L2CAP_SMP_DEBUG("Unknown pairing method (%d)", proc->stk_method);
        return FALSE;
    }

    (void)ble_sm_pair_failed_tx(conn, smp_err_code);
    return TRUE;
}

/**
 * With or without initializing the public-private key pair, we power up
 * to initialize a set that is not currently stored on non-volatile memory.
 * It will be lost when powered down.
 */
bool ble_smp_gen_sc_oob_data(struct ble_smp_sc_oob_data *local_oob_data, struct ble_smp_sc_oob_key *local_ecdh_key)
{
    if (!local_oob_data || !local_ecdh_key) {
        return FALSE;
    }

    if (!tlkalg_ecc_gen_key_pair(local_ecdh_key->public_key, local_ecdh_key->private_key, ECC_use_secp256r1, FALSE)) {
        BLE_HOST_L2CAP_SMP_DEBUG("generate an ECDH public-private key pairs failed");
        return FALSE;
    }

    BLE_HOST_L2CAP_SMP_DEBUG("our pubkey= %s", hex_to_str(local_ecdh_key->public_key, 64));
    BLE_HOST_L2CAP_SMP_DEBUG("our privkey=%s", hex_to_str(local_ecdh_key->private_key, 32));

    ble_host_hci_le_gen_rand(local_oob_data->random, 16);

    blt_crypto_alg_f4(local_oob_data->confirm, local_ecdh_key->public_key, local_ecdh_key->public_key, local_oob_data->random, 0);

    BLE_HOST_L2CAP_SMP_DEBUG("SC OOB data-confirm (be) %s ", hex_to_str(local_oob_data->confirm, 16));
    BLE_HOST_L2CAP_SMP_DEBUG("SC OOB data-random  (be) %s ", hex_to_str(local_oob_data->random, 16));
    // uint8_t le_r[16], le_c[16];
    // swapX (local_oob_data->random, le_r, 16);
    // swapX (local_oob_data->confirm, le_c, 16);
    // BLE_HOST_L2CAP_SMP_DEBUG("SC OOB data-random  (le) %s ", hex_to_str(le_r, 16));
    // BLE_HOST_L2CAP_SMP_DEBUG("SC OOB data-confirm (le) %s ", hex_to_str(le_c, 16));

    return TRUE;
}

bool ble_smp_set_sc_oob_data(struct ble_host_conn *conn, const struct ble_smp_sc_oob_data *oob_local, const struct ble_smp_sc_oob_key *local_ecdh_key, const struct ble_smp_sc_oob_data *oob_remote)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        return FALSE;   
    }

    /* need to check if the SCOOB pending request is TRUE */
    if (!(proc->tk_status & BLE_SMP_TK_STATE_REQUEST)) {
        return FALSE;
    }

    bool smp_peripheral_role = (conn->role == BLE_HCI_CONN_ROLE_PERIPHERAL) ? 1 : 0;

    //oob data check
    bool scOobLocalUsed, scOobRemoteUsed;
    if (smp_peripheral_role) {
        //For smp peripheral role: if the central's pairing_req oob flag is set, the peripheral's local SC OOB data will be used!!!
        scOobLocalUsed  = (proc->pairing_req.oobDataFlag);
        scOobRemoteUsed = (proc->pairing_rsp.oobDataFlag);
    } else {
        //For central role: if the peripheral's pairing_rsp oob flag is set, the central's local SC OOB data will be used!!!
        scOobLocalUsed  = (proc->pairing_rsp.oobDataFlag);
        scOobRemoteUsed = (proc->pairing_req.oobDataFlag);
    }

    if ((scOobLocalUsed && oob_local == NULL) || (scOobRemoteUsed && oob_remote == NULL)) {
        BLE_HOST_L2CAP_SMP_DEBUG("sc oob data check fail");
        ble_smp_cancel_auth(conn->conn_handle);
        return FALSE;
    }

    proc->scoob_remote = scOobRemoteUsed ? (struct ble_smp_sc_oob_data *)(uint32_t)oob_remote : NULL;

    if (scOobLocalUsed) {
        proc->scoob_local     = (struct ble_smp_sc_oob_data *)(uint32_t)oob_local;
        proc->scoob_local_key = (struct ble_smp_sc_oob_key *)(uint32_t)local_ecdh_key;
    } else {
        proc->scoob_local     = NULL;
        proc->scoob_local_key = NULL;
    }

    if (local_ecdh_key) {
        smemcpy(proc->sc_pk_own, local_ecdh_key->public_key, 64);
        smemcpy(proc->sc_sk_dhk_own, local_ecdh_key->private_key, 32);
    }

    int rc = ble_sm_public_key_tx(conn);
    if (rc != 0) {
        BLE_HOST_L2CAP_SMP_ERROR("host hci send public key to peer failed, rc = %d", rc);
        return FALSE;
    }

    if (smp_peripheral_role) {
        if (proc->scoob_remote) { //sc_oob confirm check
            uint8_t confirm[16];
            //Confirm = f4(PK,PK,random,0)
            BLE_HOST_L2CAP_SMP_DEBUG("sc_pk_peer(be):%s", hex_to_str(proc->sc_pk_peer, 64));
            BLE_HOST_L2CAP_SMP_DEBUG("scoob_remote-r(be):%s", hex_to_str(proc->scoob_remote->random, 16));
            blt_crypto_alg_f4(confirm, proc->sc_pk_peer, proc->sc_pk_peer, proc->scoob_remote->random, 0);

            BLE_HOST_L2CAP_SMP_DEBUG("blt_crypto_alg_f4(be):%s", hex_to_str(confirm, 16));

            bool match = (memcmp(confirm, proc->scoob_remote->confirm, sizeof(confirm)) == 0);

            if (!match) {
                BLE_HOST_L2CAP_SMP_DEBUG("sc_oob confirm check fail");
                (void) ble_sm_pair_failed_tx(conn, SMP_FAILED_CONFIRM_VALUE_FAILED);
            }
        }
    }

    return TRUE;
}

static int ble_smp_central_proc_alloc(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);

    uint8_t pairing_failed_reason;

    if (conn->sm_settings.sm_sec_lvl == LE_SECURITY_MODE_1_LEVEL_1 || \
        conn->role == BLE_HCI_CONN_ROLE_PERIPHERAL) {
        BLE_HOST_L2CAP_SMP_ERROR("ACL CONN role [%d] or SEC LVL [%d] not allowed SEC REQ rx packet", conn->role, conn->sm_settings.sm_sec_lvl);
        pairing_failed_reason = SMP_FAILED_COMMAND_NOT_SUPPORTED;
        goto fail;
    }

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc != NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Already allocate the SMP procedure control block");
        pairing_failed_reason = SMP_FAILED_REPEATED_ATTEMPTS;
        goto fail;
    }

    /* allocate a new procedure for SMP procedure, already clear the memory content to zero in ble_smp_proc_alloc() */
    proc = ble_smp_proc_alloc();
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Allocate a new SMP procedure control block failed, not enough memory");
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }

    /* insert the new SMP procedure */
    ble_smp_proc_insert_new(proc);

    /* alloc to ACL connection handle */
    proc->conn_handle = conn->conn_handle;

    /**
     * Initialize SMP parameters setting,
     * when ACL connected, gap callback the Upper user, user can change SMP settings
     */
    proc->sm_cfg = conn->sm_settings; //TODO: default use host SM settings, but can be changed by user

    return 0; //success

fail:
    return pairing_failed_reason;   
}

static int ble_smp_pair_initiate(struct ble_host_conn *conn, bool is_sec_req_rcvd)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);

    uint8_t pairing_failed_reason;

    pairing_failed_reason = ble_smp_central_proc_alloc(conn);
    if (pairing_failed_reason != 0) { //fail
        goto fail;
    } 

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);

    /* prepare pairing response parameters */
    struct ble_smp_auth_req *auth = &proc->pairing_req.authReq;
    auth->sc = proc->sm_cfg.sm_sc;
    auth->bondingFlags = proc->sm_cfg.sm_bonding;
    auth->keypress = proc->sm_cfg.sm_keypress;
    auth->MITM = proc->sm_cfg.sm_mitm;
    proc->pairing_req.code = BLE_SM_OP_PAIR_RSP;
    proc->pairing_req.oobDataFlag = proc->sm_cfg.sm_oob;
    proc->pairing_req.ioCapability = proc->sm_cfg.sm_io_capability;
    proc->pairing_req.maxEncKeySize = proc->sm_cfg.sm_min_key_size;
    proc->pairing_req.initKeyValue = proc->sm_cfg.sm_peer_key_dist;
    proc->pairing_req.rspKeyValue = proc->sm_cfg.sm_our_key_dist;

    return ble_sm_pair_req_tx(conn);

fail:
    if(is_sec_req_rcvd) {
        return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
    } else {
        return 0;
    }
}

static int ble_smp_encryption_initiate(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);

    uint8_t pairing_failed_reason;

    pairing_failed_reason = ble_smp_central_proc_alloc(conn);
    if (pairing_failed_reason != 0) { //fail
        goto fail;
    } 

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    BLE_HOST_SAL_ASSERT(proc != NULL);

    BLE_HOST_L2CAP_SMP_DEBUG("Initiate the encryption procedure for the specified connection(0x%x)", conn->conn_handle);

    /* check if the peer device is bonded */
    if (!ble_smp_store_keys_read(conn, (struct smp_bonding_keys *)&proc->peer_keys)) { //Success to load the stored keys from flash
        BLE_HOST_L2CAP_SMP_DEBUG("load stored keys success");
    } else {
        BLE_HOST_SAL_ASSERT(0);
    }

    struct ble_hci_le_start_encrypt_cp start_enc_cp;
    smemcpy(start_enc_cp.ltk, proc->peer_keys.ltk, 16);
    start_enc_cp.div = proc->peer_keys.ediv;
    start_enc_cp.rand = proc->peer_keys.rand_val;

    int rc = ble_host_hci_le_start_encryption(&start_enc_cp);
    if (rc != 0) {
        BLE_HOST_L2CAP_SMP_ERROR("host hci le start encryption failed, rc = %d", rc);
        pairing_failed_reason = SMP_FAILED_UNSPECIFIED_REASON;
        goto fail;
    }
    
    return 0;

fail:
    return ble_sm_pair_failed_tx(conn, pairing_failed_reason);
}

static int ble_smp_peripheral_initiate(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    return ble_sm_sec_request_tx(conn);
}

void ble_smp_cfg_initiate(struct smp_initiate_cfg initiate_cfg)
{
    (void)initiate_cfg;
}

/**
 * @XXX: use pointer to function to avoid the function call overhead, TODO
 */

void ble_smp_connected_evt(struct ble_host_conn *conn)  
{
    BLE_HOST_SAL_ASSERT(conn != NULL);

    /* before ble_smp_connected_evt, we should tell the upper layer that the connection is established, and user 
     * can change the connection sm_settings parameters if needed. */
    if (conn->sm_settings.sm_sec_lvl == LE_SECURITY_MODE_1_LEVEL_1) {
        return;
    }

    bool peer_dev_is_bonded = FALSE;
    struct ble_sm_keys peer_keys;

    /* check if the peer device is bonded */
    if (!ble_smp_store_keys_read(conn,  (struct smp_bonding_keys *)&peer_keys)) { //Success to load the stored keys from flash
        BLE_HOST_L2CAP_SMP_DEBUG("load stored keys success");
        peer_dev_is_bonded = TRUE;
    } else {
        BLE_HOST_L2CAP_SMP_DEBUG("load stored keys fail");
    }

    if (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) {
        if (peer_dev_is_bonded) {
            /* start encryption with the bonded device */
            if (g_smp_initiate_cfg.central.manual_smp_start || \
                g_smp_initiate_cfg.central.re_conn_cfg == PAIR_REQ_AUTO_SEND) {
                ble_smp_encryption_initiate(conn);
            }
        } else {
            /* send pairing request to the peer device */
            if (g_smp_initiate_cfg.central.manual_smp_start || \
                g_smp_initiate_cfg.central.new_conn_cfg == PAIR_REQ_AUTO_SEND) {
                ble_smp_pair_initiate(conn, FALSE);
            }
        }
    } else { 
        if (peer_dev_is_bonded) {
            if (g_smp_initiate_cfg.peripheral.re_conn_cfg == SEC_REQ_IMM_SEND) {
                ble_smp_peripheral_initiate(conn);
            } else if (g_smp_initiate_cfg.peripheral.re_conn_cfg == SEC_REQ_PEND_SEND) {
                /* TODO: set the pending flag , process it later , maybe use soft timer to process it , but currently not support */
            }
        } else {
            if (g_smp_initiate_cfg.peripheral.new_conn_cfg == SEC_REQ_IMM_SEND) {
                ble_smp_peripheral_initiate(conn);
            } else if (g_smp_initiate_cfg.peripheral.new_conn_cfg == SEC_REQ_PEND_SEND) {
                /* TODO: set the pending flag , process it later , maybe use soft timer to process it , but currently not support */
            } 
        }
    }
}

void ble_smp_disconnected_evt(struct ble_host_conn *conn, uint8_t reason)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);

    if (proc && proc->pairing_busy) {
        /* clear status and free the SM procedure control block, tell the upper layer that the pairing process has been terminated */
        ble_smp_proc_pairing_end(conn, SMP_FAILED_UNSPECIFIED_REASON);
    }

    (void)reason;
}

void ble_smp_long_term_key_req_evt(struct ble_host_conn *conn, uint64_t rand, uint16_t div)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
        
    uint16_t conn_handle = conn->conn_handle;

    struct ble_sm_proc *proc = ble_smp_proc_find(conn_handle);
    if (proc == NULL) { //proc not exist, re-connect encrytion
        BLE_HOST_L2CAP_SMP_ERROR("[LTK_REQ_EVT] Not allocate the SMP procedure control block");
        /* allocate a new procedure for SMP procedure, already clear the memory content to zero in ble_smp_proc_alloc() */
        proc = ble_smp_proc_alloc();
        if (proc == NULL) {
            BLE_HOST_L2CAP_SMP_ERROR("[LTK_REQ_EVT] Allocate a new SMP procedure control block failed, not enough memory");
            return;
        }

        /* insert the new SMP procedure */
        ble_smp_proc_insert_new(proc);

        /* alloc to ACL connection handle */
        proc->conn_handle = conn_handle;

        /**
         * Initialize SMP parameters setting,
         * when ACL connected, gap callback the Upper user, user can change SMP settings
         */
        proc->sm_cfg = conn->sm_settings; //TODO: default use host SM settings, but can be changed by user

        /* set the reconnection type */
        proc->reconn_type = RECOON_TYPE_CONN_BACK;

        /* load the stored keys from flash */
        if(!ble_smp_store_keys_read(conn,  (struct smp_bonding_keys *)&proc->peer_keys)){ //load the stored keys from flash success
            proc->enc_key_size = proc->peer_keys.key_size; 
        } else { //load the stored keys from flash fail 
            proc->enc_key_size = proc->sm_cfg.sm_min_key_size; 
        }   
    } else { //proc exist, first pairing
        /* set the reconnection type */
        proc->reconn_type = RECOON_TYPE_STD_PAIR;
    }

    //not send security request after LL_ENC_REQ received
    //blc_SecReq_ctrl.secReq_pending &= ~BIT(conn_idx); //clear security request pending flag

    if (rand == 0 && div == 0) { // EDIV+ RAND are all zero
        uint8_t smp_stk[16] = {0};
        uint8_t max_key_size = proc->enc_key_size;

        /*
         * This parameter is set only if the 1st pairing(set to 1 if both M and S support SC).
         * It should be 0 when it's connected back.
         */
        if (proc->sc_pairing) {
            /*
             * Secure Connection Pairing:
             * ......
             * (omit)
             * M->S Pairing DHKey Check: none
             * S->M Pairing DHKey Check: Pairing DHKey Check marked
             * M->S LL_ENC_REQ: Encryption begin
             */
            if (proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_DHKEY_CHECK)) {
                proc->smp_phase_chk = BLE_SMP_PAIRING_PHASE_2_ENC;

                //////////////////// secure connections 1st //////////////////
                BLE_HOST_L2CAP_SMP_DEBUG("[LTK_REQ_EVT] secure connections 1st");

                /* shorcut for the long term key */
                smemcpy(smp_stk, proc->own_ltk, max_key_size);
                
                struct ble_hci_le_lt_key_req_reply_cp le_ltk_req_reply;
                le_ltk_req_reply.conn_handle = conn_handle;
                smemcpy(le_ltk_req_reply.ltk, smp_stk, 16);

                ble_host_hci_le_ltk_request_reply(&le_ltk_req_reply);

            } else { //LTK get error
                BLE_HOST_L2CAP_SMP_ERROR("[LTK_REQ_EVT] LTK get error1 (hdl:0x%x):0x%x", conn_handle, proc->smp_phase_chk);

                proc->smp_phase_chk = BLE_SMP_PAIRING_PHASE_IDLE;

                /* LE Long term key negative request replay  */
                struct ble_hci_le_lt_key_req_neg_reply_cp le_lt_key_req_neg_reply = {
                    .conn_handle = conn_handle
                };
                ble_host_hci_le_ltk_request_negative_reply(&le_lt_key_req_neg_reply);
            }
        } else {
            /*
             * there are two cases:
             * case1: legacy pairing 1st ( smp_phase_record & BLE_SM_OP_PAIR_RANDOM must be True)
             * case2: secure connections back
             */
            if (proc->smp_phase_chk & BIT(BLE_SM_OP_PAIR_RANDOM)) { //legacy pairing 1st
            
                proc->smp_phase_chk = BLE_SMP_PAIRING_PHASE_2_ENC;

                BLE_HOST_L2CAP_SMP_DEBUG("[LTK_REQ_EVT] legacy pairing 1st (hdl:0x%x)", conn_handle);

                uint8_t smp_Stk_temp[16] = {0};
                /* STK = s1(TK, Srand, Mrand) */
                blt_crypto_alg_s1(smp_Stk_temp, proc->pairing_tk, proc->peer_rand, proc->own_rand); //generate session key
                
                /* shorcut for the long term key */
                smemcpy(smp_stk, smp_Stk_temp, max_key_size);

                /* LE Long term key request replay */
                struct ble_hci_le_lt_key_req_reply_cp le_ltk_req_reply;
                le_ltk_req_reply.conn_handle = conn_handle;
                smemcpy(le_ltk_req_reply.ltk, smp_stk, 16);
                ble_host_hci_le_ltk_request_reply(&le_ltk_req_reply);

                /* 
                 * The addr can match, but the master uses a new key. It is possible that: 1. The previous key is stored incorrectly; 
                 * 2. The master side unpair, and the slave side does not know it, so the old bonding information should be deleted here 
                 */
                ble_smp_store_keys_delete(conn); // if already save the keys, delete it, and save the new keys latter
            } else if (proc->smp_phase_chk == BLE_SMP_PAIRING_PHASE_IDLE) {
                BLE_HOST_L2CAP_SMP_DEBUG("[LTK_REQ_EVT] secure connections back\n");

                #if (CUSTOM_DARWIN_FMN_ENABLE) //FMN get its own LTK
                    if (custom_darwin_fmn.sec_info_req_cb) {
                        custom_darwin_fmn.sec_info_req_cb(conn_handle);
                    } else
                #endif
                {
                #if (BROADCOM_WORKAROUND)
                    /* LE Long term key request replay */
                    struct ble_hci_le_lt_key_req_reply_cp le_ltk_req_reply;
                    le_ltk_req_reply.conn_handle = conn_handle;
                    smemset(le_ltk_req_reply.ltk, 0, 16);
                    ble_host_hci_le_ltk_request_reply(&le_ltk_req_reply);
                #else
                    if (!ble_smp_store_keys_read(conn,  (struct smp_bonding_keys *)&proc->peer_keys)) { //Success to load the stored keys from flash
                        BLE_HOST_L2CAP_SMP_DEBUG("[LTK_REQ_EVT] load LTK form flash: %s", hex_to_str(proc->peer_keys.ltk, 16));

                        /* LE Long term key request replay */
                        struct ble_hci_le_lt_key_req_reply_cp le_ltk_req_reply;
                        le_ltk_req_reply.conn_handle = conn_handle;
                        smemcpy(le_ltk_req_reply.ltk, proc->peer_keys.ltk, 16);
                        ble_host_hci_le_ltk_request_reply(&le_ltk_req_reply);
                    } else { //load the stored keys from flash fail 
                        /* LE Long term key negative request replay  */
                        struct ble_hci_le_lt_key_req_neg_reply_cp le_lt_key_req_neg_reply = {
                            .conn_handle = conn_handle
                        };
                        ble_host_hci_le_ltk_request_negative_reply(&le_lt_key_req_neg_reply);
                    }
                #endif
                }
            } else { //LTK get error
                BLE_HOST_L2CAP_SMP_ERROR("[LTK_REQ_EVT] LTK get error2(hdl:0x%x):0x%x", conn_handle, proc->smp_phase_chk);
                
                proc->smp_phase_chk = BLE_SMP_PAIRING_PHASE_IDLE;

                /* LE Long term key negative request replay  */
                struct ble_hci_le_lt_key_req_neg_reply_cp le_lt_key_req_neg_reply = {
                    .conn_handle = conn_handle
                };
                ble_host_hci_le_ltk_request_negative_reply(&le_lt_key_req_neg_reply);
            }
        }
    } else { // EDIV+ RAND are not all zero
        BLE_HOST_L2CAP_SMP_DEBUG("[LTK_REQ_EVT] legacy pairing back");
        
        uint8_t load_ltk[16] = {0};
        bool load_ltk_success = FALSE;
        if(!ble_smp_store_keys_read(conn,  (struct smp_bonding_keys *)&proc->peer_keys)) { //load the stored keys from flash success
            if (proc->peer_keys.ediv == div && proc->peer_keys.rand_val == rand) {
                load_ltk_success = TRUE;
            }
        }
        
        if (load_ltk_success == TRUE) {
            BLE_HOST_L2CAP_SMP_DEBUG("[LTK_REQ_EVT] load LTK form flash: %s", hex_to_str(proc->peer_keys.ltk, 16));
            smemcmp(load_ltk, proc->peer_keys.ltk, 16);

            #if (SMP_LEGACY_LTK_VERIFICATION_EN) 
                uint16_t crc16_  = crc16(load_ltk, 14);
                uint16_t ltkverf = load_ltk[14] | (load_ltk[15] << 8);
                if (crc16_ != ltkverf) {
                    load_ltk_success = FALSE;
                    BLE_HOST_L2CAP_SMP_DEBUG("[ltk] crc16_%d, ltkverf_%d", crc16_, ltkverf);
                }
            #endif
        }

        if (load_ltk_success == TRUE) {
            /* LE Long term key request replay */
            struct ble_hci_le_lt_key_req_reply_cp le_ltk_req_reply;
            le_ltk_req_reply.conn_handle = conn_handle;
            smemcpy(le_ltk_req_reply.ltk, load_ltk, 16);
            ble_host_hci_le_ltk_request_reply(&le_ltk_req_reply);
        } else { // LTK get error

        #if (LL_PAUSE_ENC_FIX_EN)
            bool needSpecialProc = FALSE;
            if (proc->smp_phase_chk == BLE_SMP_PAIRING_PHASE_2_OK) {
                needSpecialProc = TRUE;
            }

            //Notice: if it is a slave role, the value of ediv_rand is saved
            //using its own parameters, otherwise, the value of the peer device is used.
            if (needSpecialProc && (proc->peer_keys.ediv == div && proc->peer_keys.rand_val == rand)) {
                /* LE Long term key request replay */
                struct ble_hci_le_lt_key_req_reply_cp le_ltk_req_reply;
                le_ltk_req_reply.conn_handle = conn_handle;
                smemcpy(le_ltk_req_reply.ltk, proc->own_ltk, 16);
                ble_host_hci_le_ltk_request_reply(&le_ltk_req_reply);
            } else
        #endif
            {
            #if SAMSUNG_WORKAROUND
                /* LE Long term key request replay */
                struct ble_hci_le_lt_key_req_reply_cp le_ltk_req_reply;
                le_ltk_req_reply.conn_handle = conn_handle;
                smemset(le_ltk_req_reply.ltk, 0, 16);
                ble_host_hci_le_ltk_request_reply(&le_ltk_req_reply);
            #else
                
                BLE_HOST_L2CAP_SMP_ERROR("[LTK_REQ_EVT] LTK get error3(hdl:0x%x):0x%x", conn_handle, proc->smp_phase_chk);

                /* LE Long term key negative request replay  */
                struct ble_hci_le_lt_key_req_neg_reply_cp le_lt_key_req_neg_reply = {
                    .conn_handle = conn_handle
                };
                ble_host_hci_le_ltk_request_negative_reply(&le_lt_key_req_neg_reply);
            #endif
            }
        }
    }
}

void ble_smp_encrypt_change_evt(struct ble_host_conn *conn, uint8_t status, uint8_t enc_enable) //register encryption done event in GAP event callBack
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL) {
        BLE_HOST_L2CAP_SMP_ERROR("Not allocate the SMP procedure control block");
        return; 
    }

    bool smp_central_role = (conn->role == BLE_HCI_CONN_ROLE_CENTRAL) ? 1 : 0;

    if (status == BLE_SUCCESS && enc_enable) { //encryption done success
        if (proc->reconn_type == RECOON_TYPE_STD_PAIR) { //First Pair   
            if (proc->smp_phase_chk == BLE_SMP_PAIRING_PHASE_2_ENC) {
                proc->smp_phase_chk = BLE_SMP_PAIRING_PHASE_2_OK;
            } 

            /* key distribute trigger condition */
            if (smp_central_role) {  
                if (!proc->smp_DistributeKeyRecvValue) {
                    proc->key_distribute = 1;
                }
            } else { 
                proc->key_distribute = 1;
            }
        } else {
            proc->smp_timeout_start_tick = 0;
        }

        if (gap_eventMask & GAP_EVT_MASK_SMP_CONN_ENCRYPTION_DONE) {
            gap_smp_connEncDoneEvt_t gapEvt;
            gapEvt.connHandle = conn->conn_handle;
            gapEvt.re_connect = proc->reconn_type;
            blc_gap_send_event(GAP_EVT_SMP_CONN_ENCRYPTION_DONE, (uint8_t *)&gapEvt, sizeof(gap_smp_pairingBeginEvt_t));
        }

        if ((gap_eventMask & GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE) && (proc->reconn_type == RECOON_TYPE_CONN_BACK)) {
            gap_smp_securityProcessDoneEvt_t gapEvt;
            gapEvt.connHandle = conn->conn_handle;
            gapEvt.re_connect = RECOON_TYPE_CONN_BACK;
            blc_gap_send_event(GAP_EVT_SMP_SECURITY_PROCESS_DONE, (uint8_t *)&gapEvt, sizeof(gap_smp_securityProcessDoneEvt_t));
        }

        /* set ACL connection encryption status */
        conn->sec_state.encrypted = 1;
        //TOTO:
    } else {
        if (smp_central_role && (status == HCI_ERR_PIN_KEY_MISSING || status == HCI_ERR_UNSUPPORTED_REMOTE_FEATURE)) {
            /* peripheral device key missing, central shall clean smp information in flash, then terminate connection with peer */
            //extern int ble_host_hci_disconnect(struct ble_hci_lc_disconnect_cp * p_lc_disconnect);
            struct ble_hci_lc_disconnect_cp disconnect = { 
                conn->conn_handle, 
                HCI_ERR_REMOTE_USER_TERM_CONN
            };
            /* disconnect with peer */
            ble_host_hci_disconnect(&disconnect);
            /* should delete the older smp banding info */
            ble_smp_store_keys_delete(conn);
        }
    }
}

void ble_sm_pending_task_loop(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);

    if (conn->sm_settings.sm_sec_lvl == LE_SECURITY_MODE_1_LEVEL_1) {
        return;
    }

    struct ble_sm_proc *proc = ble_smp_proc_find(conn->conn_handle);
    if (proc == NULL || !proc->pairing_busy) {
        BLE_HOST_L2CAP_SMP_DEBUG("[sm pending task] %s", proc == NULL ? "Not allocate the SMP procedure control block" : "SMP procedure is idle");
        return; 
    }

    /* ------------------------------ 1. SMP pending procedure ------------------------------ */
    if (proc->tk_status) {
        if (proc->tk_status & BLE_SMP_TK_STATE_CONFIRM_PENDING) {
            if (proc->tk_status & BLE_SMP_TK_STATE_UPDATE) {
                /* send pairing confirm to peer successfully, clear tk_status flag */
                if (!ble_sm_pair_confirm_tx(conn)) {
                    proc->tk_status = 0;
                }
            }
        } else if (proc->tk_status & BLE_SMP_TK_STATE_NC) {
            if (proc->tk_status & BLE_SMP_TK_STATE_NC_CHECK_YES) {
                if (proc->tk_status & BLE_SMP_TK_STATE_NC_DHKEY_FAIL_PENDING) {
                    /* send pairing failed to peer successfully, clear tk_status flag */
                    if (!ble_sm_pair_failed_tx(conn, SMP_FAILED_DHKEY_CHECK_FAILED)) {
                        proc->tk_status = 0;
                    }
                } else if (proc->tk_status & BLE_SMP_TK_STATE_NC_DHKEY_SUCC_PENDING) {
                    /* send pairing confirm to peer successfully, clear tk_status flag */
                    if (!ble_sm_pair_confirm_tx(conn)) {
                        proc->tk_status = 0;
                        if (conn->role == BLE_HCI_CONN_ROLE_PERIPHERAL) {
                            //smp4.2 or above, after exchange smp pairing DH key check, then transport specific keys distribution
                            proc->smpDistributeKeyOrder = BLE_SMP_DISTRIBUTE_KEY_START;
                        }
                    }
                }
            } else if (proc->tk_status & BLE_SMP_TK_STATE_NC_CHECK_NO) {
                /* send pairing failed to peer successfully, clear tk_status flag */
                if (!ble_sm_pair_failed_tx(conn, SMP_FAILED_NUMERIC_COMPARISION_FAILED)) {
                    proc->tk_status = 0;
                }
            }
        }
    }

    /* ------------------------------ 2. SMP key distribute ------------------------------ */
    if (proc->key_distribute) {
        switch (proc->smpDistributeKeyOrder) {
        case BLE_SM_OP_ENC_INFO:  //LTK
            if (proc->smp_DistributeKeySend.encKey) {
                /* send encryption information to peer successfully, switch key_distribute flag to next step */
                if(!ble_sm_enc_info_tx(conn)){
                    proc->smpDistributeKeyOrder = BLE_SM_OP_MASTER_ID;
                }
            } else {
                proc->smpDistributeKeyOrder = BLE_SM_OP_ID_INFO;
            }
            __attribute__((fallthrough)); //no break;
        case BLE_SM_OP_MASTER_ID: //EDIV and Random
            /* send Central Identification  to peer successfully, switch key_distribute flag to next step */
            if(!ble_sm_master_id_tx(conn)) {
                proc->smp_DistributeKeySend.encKey = 0;
                proc->smpDistributeKeyOrder = BLE_SM_OP_ID_INFO;
            }
            __attribute__((fallthrough)); //no break;
        case BLE_SM_OP_ID_INFO: //IRK
            if (proc->smp_DistributeKeySend.idKey) {
                /* send Identity Information to peer successfully, switch key_distribute flag to next step */
                if(!ble_sm_id_info_tx(conn)){
                    proc->smpDistributeKeyOrder = BLE_SM_OP_ID_ADDR_INFO;
                }
            } else {
                proc->smpDistributeKeyOrder = BLE_SM_OP_SIGN_INFO;
            }
            __attribute__((fallthrough)); //no break;
        case BLE_SM_OP_ID_ADDR_INFO: //Id address
            /* send Identity Address Information to peer successfully, switch key_distribute flag to next step */
            if (!ble_sm_id_addr_info_tx(conn)) {
                proc->smp_DistributeKeySend.idKey = 0;
                proc->smpDistributeKeyOrder = BLE_SM_OP_SIGN_INFO;
            }
            __attribute__((fallthrough)); //no break;
        case BLE_SM_OP_SIGN_INFO: //CSRK
            if (proc->smp_DistributeKeySend.signKey) {
                /* send Signing Information to peer successfully, switch key_distribute flag to next step */
                if(!ble_sm_sign_info_tx(conn)){
                    proc->smp_DistributeKeySend.signKey = 0;
                    proc->smpDistributeKeyOrder = BLE_SMP_OP_ENC_END;
                }
            } else {
                proc->smpDistributeKeyOrder = BLE_SMP_OP_ENC_END;
            }
            __attribute__((fallthrough)); //no break;
        case BLE_SMP_OP_ENC_END:
            /* if sending key and receiving key all completed, process pairing end */
            if (!proc->smp_DistributeKeyRecvValue && !proc->smp_DistributeKeySendValue) {
                //blt_smp_saveBondingKey(connHandle);
                struct smp_bonding_keys keys; //TODO:
                ble_smp_store_keys_write(conn, &keys);
            }
            /* clear key_distribute flag */
            proc->smpDistributeKeyOrder = BLE_SMP_DISTRIBUTE_KEY_END;
            proc->key_distribute = 0;
            break;
        default:
            BLE_HOST_L2CAP_SMP_ERROR("Invalid SMP distribute key order: %d", proc->smpDistributeKeyOrder);
            break;
        }
    }

    /* ------------------------------ 3. SMP Timeout check ------------------------------ 
     * If the Security Manager Timer reaches 30 seconds, the procedure shall be considered 
     * to have failed, and the local higher layer shall be notified. TODO: use soft timer instead of system timer check */
    if (proc->smp_timeout_start_tick && ble_host_sal_is_time_exceed(proc->smp_timeout_start_tick, 30000)) {
        /*
         * In the case of SMP protocol command interaction timeout, the stack will not disconnect the ACL connection when the
         * timeout occurs. The processing of disconnecting the ACL connection will be handed by the user in the GAP event callback.
         */
        #if (0)
            struct ble_hci_lc_disconnect_cp disconnect = {
                connHandle,
                HCI_ERR_REMOTE_USER_TERM_CONN
            };
            ble_host_hci_disconnect(&disconnect);
        #endif

        /* clear status and free the SM procedure control block, not need to send pairing failed pkt 
         * to peer device, just tell the upper layer that the pairing process has failed. Here not 
         * call ble_sm_pair_failed_tx(conn, SMP_FAILED_UNSPECIFIED_REASON) */
        ble_smp_proc_pairing_end(conn, SMP_FAILED_UNSPECIFIED_REASON);
    }
}


#endif


