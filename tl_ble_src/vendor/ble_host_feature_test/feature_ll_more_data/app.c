/********************************************************************************************************
 * @file    app.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "stack/multiCoreComm/comm.h"
#include "stack/multiCoreComm/service/service_shareMemory.h"

//#include "app_config.h"
#include "app.h"
#include "../default_buffer.h"
#include "../default_att.h"
#include "app_ui.h"


#if (FEATURE_TEST_MODE == TEST_LL_MD)

/* app ble event callback parameter */
typedef struct
{
    int event_id;
    void (*event_callback)(u16 connHandle, const void *pEvent);
}app_ble_event_t;

void main_lopp_hci(void);
extern int blt_gap_mainloop_v1(void);
extern void ble_host_v1_main_loop(void);
extern int ble_host_hci_le_create_connection(const struct ble_hci_le_create_conn_cp *p_create_conn);


_attribute_ble_data_retention_ int central_smp_pending = 0; // SMP: security & encryption;
_attribute_data_retention_ static u8 mac_public[6]={0};
_attribute_data_retention_ static u8 mac_random_static[6]={0};
_attribute_data_retention_ u8 central_sdp_pending = 0;
#if (HCI_INTERFACE == HCI_UART)
volatile u8 uart_dma_send_flag = 1;
#endif


void ble_host_sal_hci_send_packet(const uint8_t *data, uint16_t len)
{
    #if (HCI_INTERFACE == HCI_UART)
        while(!uart_dma_send_flag){};
        uart_send_dma(UART_MODULE_SEL, data, len);
        while(!uart_dma_send_flag){};
    #elif (HCI_INTERFACE == HCI_SHAREMEMORY)
        tlk_d25f_hci_send_message(TLK_SHARE_MEMOTY_MESSAGE_TYPE_BLE, (u8 *)(uintptr_t)(const void*)data, len);
    #endif
}

void ble_host_hci_reset(void)
{
    uint8_t reset_cmd[] = {0x01, 0x03, 0x0c, 0x0};
    ble_host_sal_hci_send_packet(reset_cmd, sizeof(reset_cmd));
}


#if (HCI_INTERFACE == HCI_UART)
_attribute_ble_data_retention_ u8 __attribute__((aligned(4))) spp_rx_fifo_b[SPP_RXFIFO_SIZE * SPP_RXFIFO_NUM] = {0};
_attribute_ble_data_retention_ my_fifo_t                      spp_rx_fifo = {
    SPP_RXFIFO_SIZE,
    SPP_RXFIFO_NUM,
    0,
    0,
    spp_rx_fifo_b,
};

void user_uart_init(void)
{
    unsigned short div;
    unsigned char bwpc;

    uart_hw_fsm_reset(UART_MODULE_SEL);
    uart_set_pin(UART_MODULE_SEL, UART_MODULE_TX_PIN, UART_MODULE_RX_PIN);
    uart_cal_div_and_bwpc(UART_MODULE_BAUDRATE, sys_clk.pclk*1000*1000, &div, &bwpc);
    uart_set_rx_timeout_with_exp(UART_MODULE_SEL, bwpc, 12, UART_BW_MUL2,0);
    uart_init(UART_MODULE_SEL, div, bwpc, UART_PARITY_NONE, UART_STOP_BIT_ONE);

    uart_set_tx_dma_config(UART_MODULE_SEL, UART_DMA_CHANNEL_TX);
    uart_set_rx_dma_config(UART_MODULE_SEL, UART_DMA_CHANNEL_RX);

    uart_clr_irq_status(UART_MODULE_SEL, UART_TXDONE_IRQ_STATUS);
    uart_set_irq_mask(UART_MODULE_SEL, UART_TXDONE_MASK);
    plic_interrupt_enable(UART_MODULE_IRQ);
    plic_set_priority(UART_MODULE_IRQ, 1);

    dma_set_irq_mask(UART_DMA_CHANNEL_RX, TC_MASK);
    plic_interrupt_enable(IRQ_DMA);

    u8 *uart_rx_addr = (spp_rx_fifo_b + (spp_rx_fifo.wptr & (spp_rx_fifo.num - 1)) * spp_rx_fifo.size);
    uart_receive_dma(UART_MODULE_SEL, uart_rx_addr+4, spp_rx_fifo.size-4); // len-4byte
    uart_clr_irq_status(UART_MODULE_SEL, UART_TXDONE_IRQ_STATUS);

    uart_dma_send_flag = 1;
}

_attribute_ram_code_sec_ void hci_uart_irq_handler(void)
{
     if (uart_get_irq_status(UART_MODULE_SEL,UART_TXDONE_IRQ_STATUS)) {
         uart_clr_irq_status(UART_MODULE_SEL,UART_TXDONE_IRQ_STATUS);
        // tlkapi_printf(1, "hci uart send seccess!!!\n");
         uart_dma_send_flag = 1;
     }
}
PLIC_ISR_REGISTER(hci_uart_irq_handler, UART_MODULE_IRQ);

_attribute_ram_code_ void hci_uart_dma_irq_handler(void)
{

    if (dma_get_tc_irq_status( BIT(UART_DMA_CHANNEL_RX))) {

        u8 *pp = spp_rx_fifo.p + spp_rx_fifo.wptr * spp_rx_fifo.size;

        tlkapi_send_string_data(1, "D25F_Rx", pp+4, pp[0]);

        spp_rx_fifo.wptr == (spp_rx_fifo.num - 1) ? spp_rx_fifo.wptr = 0 : spp_rx_fifo.wptr++;
        u8 *p = spp_rx_fifo.p + spp_rx_fifo.wptr * spp_rx_fifo.size;
        uart_receive_dma(UART_MODULE_SEL, p+4, spp_rx_fifo.size-4);

        if ((uart_get_irq_status(UART_MODULE_SEL,UART_RX_ERR))) {
            uart_clr_irq_status(UART_MODULE_SEL,UART_RXBUF_IRQ_STATUS);
        }
        dma_clr_tc_irq_status( BIT(UART_DMA_CHANNEL_RX));

       // tlkapi_printf(1, "hci uart receive seccess!!!\n");
    }
}
PLIC_ISR_REGISTER(hci_uart_dma_irq_handler, IRQ_DMA)

void uart_hci_mainloop(void)
{
    if (spp_rx_fifo.wptr == spp_rx_fifo.rptr) {
        return;
    }
    u8 *pData = (u8 *)(spp_rx_fifo.p + spp_rx_fifo.rptr * spp_rx_fifo.size);
    u32 *pLength = (u32 *)pData;
    spp_rx_fifo.rptr == (spp_rx_fifo.num - 1) ? spp_rx_fifo.rptr = 0 : spp_rx_fifo.rptr++;

    extern void ble_host_hci_rx_packet(uint8_t *data, unsigned int len);
    ble_host_hci_rx_packet(pData+4, *pLength);
}
#endif

/**
 * @brief   BLE Advertising data
 */
const u8 adv_data[] = {
    3,
    DT_COMPLETE_LOCAL_NAME,
    'm',
    'd',
    2,
    DT_FLAGS,
    0x05, // BLE limited discoverable mode and BR/EDR not supported
    3,
    DT_APPEARANCE,
    0x80,
    0x01, // 384, Generic Remote Control, Generic category
    5,
    DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID,
    0x12,
    0x18,
    0x0F,
    0x18, // incomplete list of service class UUIDs (0x1812, 0x180F)
};

/**
 * @brief   BLE Scan Response Packet data
 */
const u8 tbl_scanRsp[] = {
    3,
    DT_COMPLETE_LOCAL_NAME,
    'm',
    'd',
};

void app_ble_adv_start(void) {
    struct ble_hci_le_set_adv_enable_cp leg_adv_enable={0};
    leg_adv_enable.enable = BLC_ADV_ENABLE;
    int err = ble_host_hci_le_set_adv_enable(&leg_adv_enable);
    tlkapi_printf(1, "ble adv start, err=%X\n", err);
}

void app_ble_adv_stop(void) {
    struct ble_hci_le_set_adv_enable_cp leg_adv_enable={0};
    leg_adv_enable.enable = BLC_ADV_DISABLE;
    int err = ble_host_hci_le_set_adv_enable(&leg_adv_enable);
    tlkapi_printf(1, "ble adv stop, err=%X\n", err);
}

void ble_host_hci_set_leg_adv(void)
{
    /* set leg adv param */
    struct ble_hci_le_set_adv_params_cp leg_adv_params={0};
    leg_adv_params.min_interval   = ADV_INTERVAL_30MS;
    leg_adv_params.max_interval   = ADV_INTERVAL_30MS;
    leg_adv_params.type           = ADV_TYPE_CONNECTABLE_UNDIRECTED;
    leg_adv_params.own_addr_type  = 0;
    leg_adv_params.peer_addr_type = 0;
    //leg_adv_params.peer_addr    = 0;
    leg_adv_params.chan_map       = BLT_ENABLE_ADV_ALL;
    leg_adv_params.filter_policy  = ADV_FP_ALLOW_SCAN_ANY_ALLOW_CONN_ANY;
    int err = ble_host_hci_le_set_adv_param(&leg_adv_params);
    tlk_printf("call ble_host_hci_le_set_adv_paramret, err=%X\r\n", err);

    /* set leg adv data */
    const char device_name[] = "tl322x_condor";
//    u8 adv_data[] = {2, DT_FLAGS, 0x05, 14,DT_COMPLETE_LOCAL_NAME, 't','l','3','2','2','x','_','c','o','n','d','o','r'};
    blc_svc_setDeviceName(device_name);
    blc_svc_setAppearance(GAP_APPEARANCE_UNKNOWN);
    struct ble_hci_le_set_adv_data_full_cp leg_adv_data={0};
    leg_adv_data.adv_data_len = sizeof(adv_data);
    memcpy(leg_adv_data.adv_data, adv_data, leg_adv_data.adv_data_len);
    err = ble_host_hci_le_set_adv_data(&leg_adv_data);
    tlk_printf("call ble_host_hci_le_set_adv_data ret, err=%X\r\n", err);

    /* set scan rsp data */
    struct ble_hci_le_set_scan_rsp_data_full_cp  leg_scan_rsp_data={0};
    leg_scan_rsp_data.scan_rsp_len = sizeof(adv_data)-3;
    memcpy(leg_scan_rsp_data.scan_rsp, adv_data+3, leg_scan_rsp_data.scan_rsp_len);
    err = ble_host_hci_le_set_scan_rsp_data(&leg_scan_rsp_data);
    tlk_printf("call ble_host_hci_le_set_scan_rsp_data, err=%X\r\n", err);

    /* set leg adv enable */
    app_ble_adv_start();
}

void ble_host_hci_set_leg_scan(void)
{
    /* set leg scan param */
    struct ble_hci_le_set_scan_params_cp leg_scan_params={0};
    leg_scan_params.scan_type     = SCAN_TYPE_PASSIVE;
    leg_scan_params.scan_itvl     = SCAN_INTERVAL_100MS;
    leg_scan_params.scan_window   = SCAN_WINDOW_100MS;
    leg_scan_params.filter_policy = SCAN_FP_ALLOW_ADV_ANY;
    int err = ble_host_hci_le_set_scan_param(&leg_scan_params);
    tlk_printf("call ble_host_hci_le_set_leg_adv_params, err=%X\r\n", err);

    /* set leg scan enable */
    struct ble_hci_le_set_scan_enable_cp leg_scan_enable={0};
    leg_scan_enable.enable            = BLC_SCAN_ENABLE;
    leg_scan_enable.filter_duplicates = DUP_FILTER_DISABLE;
    err = ble_host_hci_le_set_scan_enable(&leg_scan_enable);
    tlk_printf("call ble_host_hci_le_set_leg_scan_enable, err=%X\r\n", err);
}



static void app_acl_connected_event_callback(u16 aclHandle, const void *pEvent)
{
    (void) aclHandle;
    const blc_prf_aclConnEvt_t *pAclConnEvt = pEvent;
    tlkapi_send_string_data(APP_LOG_EN, "ble connect event",
                            (u8 *)(uintptr_t)(const void*)pAclConnEvt, sizeof(blc_prf_aclConnEvt_t));
    hci_le_connectionCompleteEvt_t conn_info;
    conn_info.connHandle = pAclConnEvt->aclHandle;
    conn_info.role = pAclConnEvt->aclHandle & 0x80 ? ACL_ROLE_CENTRAL : ACL_ROLE_PERIPHERAL;
    memcpy(conn_info.peerAddr, pAclConnEvt->PeerAddr, 6);
    conn_info.peerAddrType = pAclConnEvt->PeerAddrType;
    dev_char_info_insert_by_conn_event(&conn_info);

    if (acl_conn_periphr_num < ACL_PERIPHR_MAX_NUM) {
        app_ble_adv_start();
    }
}

static void app_acl_disconnected_event_callback(u16 aclHandle, const void *pEvent)
{
    (void) aclHandle;
    const blc_prf_aclDisconnEvt_t *pAclDisconnectEvt = pEvent;
    tlkapi_send_string_data(APP_LOG_EN, "ble disconnect event",
                            (u8 *)(uintptr_t)(const void*)pAclDisconnectEvt, sizeof(blc_prf_aclDisconnEvt_t));

    app_ble_adv_start();

    if (central_disconnect_connhandle == aclHandle) { //un_pair disconnection flow finish, clear flag
        central_disconnect_connhandle = 0;
    }
    dev_char_info_delete_by_connhandle(aclHandle);
}

static void app_acl_connect_security_done_callback(u16 aclHandle, const void *pEvent)
{
    (void)pEvent;
    tlkapi_printf(APP_LOG_EN, "ble connect security done, handle: 0x%x\n", aclHandle);
}

static void app_acl_connect_interval_update_callback(u16 aclHandle, const void *pEvent)
{
    (void) aclHandle;
    const blc_prf_aclConnectUpdateEvt_t *pAclIntervalUpdateEvt = (const blc_prf_aclConnectUpdateEvt_t *)pEvent;
    tlkapi_send_string_data(APP_LOG_EN, "ble connection update",
                            (u8 *)(uintptr_t)(const void*)pAclIntervalUpdateEvt, sizeof(blc_prf_aclConnectUpdateEvt_t));
}

static const char *app_get_sdp_server_name(int server_id)
{
    (void) server_id;

    return "Unknown";
}

static void app_client_sdp_found_callback(u16 aclHandle, const void *pEvent)
{
    const blc_prf_sdpFoundEvt_t *pSdpFound = pEvent;
    tlkapi_printf(APP_LOG_EN, "ACL connect handle: 0x%x, peer supported server: %s\n",
                  aclHandle, app_get_sdp_server_name(pSdpFound->svcId));
    tlkapi_printf(APP_LOG_EN, "server start handle: 0x%x, end handle: 0x%x\n",
                  pSdpFound->startHdl, pSdpFound->endHdl);
}

static void app_client_sdp_not_found_callback(u16 aclHandle, const void *pEvent)
{
    const blc_prf_sdpFailEvt_t *pSdpFail = pEvent;
    tlkapi_printf(APP_LOG_EN, "ACL connect handle: 0x%x, peer not supported server: %s\n",
                  aclHandle, app_get_sdp_server_name(pSdpFail->svcId));
}

static void app_client_all_sdp_over_callback(u16 aclHandle, const void *pEvent)
{
    (void)pEvent;
    tlkapi_printf(APP_LOG_EN, "ACL connect handle: 0x%x, all SDP over\n", aclHandle);
}

int app_client_sdp_end_callback(u16 connHandle, u8 *pData, u16 dataLen)
{
    (void)connHandle;
    (void)pData;
    (void)dataLen;
}

_attribute_ram_code_ void app_le_adv_report_callback(u16 param1, const void *pEvent)
{
    (void) param1;
    event_adv_report_t *pa = (event_adv_report_t *)(uintptr_t)(const void*)pEvent;
    s8 rssi = pa->data[pa->len];
    //tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Adv report", (u8 *)pa, pa->len+12);

    /*********************** Central Create connection demo: Key press or ADV pair packet triggers pair  ********************/
    #if 0//(ACL_CENTRAL_SMP_ENABLE)
        if (central_smp_pending) { //if previous connection SMP not finish, can not create a new connection
            return ;
        }
    #endif

    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
        if (central_sdp_pending) { //if previous connection SDP not finish, can not create a new connection
            return ;
        }
    #endif

    if (central_disconnect_connhandle) { //one ACL connection central role is in un_pair disconnection flow, do not create a new one
        return ;
    }

    int central_auto_connect = 0;
    int user_manual_pairing  = 0;

    //manual pairing methods 1: key press triggers
    user_manual_pairing = central_pairing_enable && (rssi > -40); //button trigger pairing(RSSI threshold, short distance)

    if(user_manual_pairing){
        tlkapi_send_string_data(APP_LOG_EN, "Create connection", pa->mac, 6);
    }

    #if (ACL_CENTRAL_SMP_ENABLE)
        central_auto_connect = blc_smp_searchBondingPeripheralDevice_by_PeerMacAddress(pa->adr_type, pa->mac);
    #endif

    if (central_auto_connect || user_manual_pairing) {
        struct ble_hci_le_create_conn_cp cmdParam;
        cmdParam.scan_itvl      = SCAN_INTERVAL_100MS;
        cmdParam.scan_window    = SCAN_INTERVAL_100MS;
        cmdParam.filter_policy  = INITIATE_FP_ADV_SPECIFY;
        cmdParam.peer_addr_type = pa->adr_type;
        memcpy(cmdParam.peer_addr, pa->mac, 6);
        cmdParam.own_addr_type  = OWN_ADDRESS_PUBLIC;
        cmdParam.min_conn_itvl  = CONN_INTERVAL_31P25MS;
        cmdParam.max_conn_itvl  = CONN_INTERVAL_48P75MS;
        cmdParam.conn_latency   = 0;
        cmdParam.tmo            = 500;
        cmdParam.min_ce         = 0;
        cmdParam.max_ce         = 0xFFFF;
        u8 status  = ble_host_hci_le_create_connection(&cmdParam);
        if (status != BLE_SUCCESS) {
            tlkapi_printf(APP_LOG_EN, "[APP][CMD] Create connection Error,status=%02X", status);
        }
        else {
            tlkapi_send_string_data(APP_LOG_EN, "[APP][CMD] Create connection success", pa->mac, 6);
        }
    }
}

/**
 * @brief   Unicast Server register profile event callback.
 */
_attribute_ble_data_retention_ app_ble_event_t profile_event[] = {
    {PRF_EVTID_LE_ADVERTISING_REPORT, app_le_adv_report_callback              },
    /* Event for controller or Host */
    {PRF_EVTID_ACL_CONNECT,           app_acl_connected_event_callback        },
    {PRF_EVTID_ACL_DISCONNECT,        app_acl_disconnected_event_callback     },
    {PRF_EVTID_SMP_SECURITY_DONE,     app_acl_connect_security_done_callback  },
    {PRF_EVTID_ACL_CONNECT_UPDATE,    app_acl_connect_interval_update_callback},
    /* Event for Client SDP */
    {PRF_EVTID_CLIENT_SDP_FOUND,      app_client_sdp_found_callback           },
    {PRF_EVTID_CLIENT_SDP_FAIL,       app_client_sdp_not_found_callback       },
    {PRF_EVTID_CLIENT_ALL_SDP_OVER,   app_client_all_sdp_over_callback        },
    {PRF_EVTID_CLIENT_SDP_END,        app_client_sdp_end_callback},
};

static int app_ble_profile_event_callback(u16 aclHandle, int evtID, u8 *pData, u16 dataLen)
{
    (void)dataLen;

    for (unsigned int i = 0; i < ARRAY_SIZE(profile_event); i++) {
        if (profile_event[i].event_id == evtID) {
            profile_event[i].event_callback(aclHandle, pData);
            return 0;
        }
    }
    return 0;
}

#define HOST_MALLOC_BUFF_SIZE_1 (4096 + 2048)
static uint8_t hostMallocBuffer1[HOST_MALLOC_BUFF_SIZE_1];

void app_ble_profile_init(void)
{
    blc_prf_initialModule(app_ble_profile_event_callback, hostMallocBuffer1, HOST_MALLOC_BUFF_SIZE_1);
//    blc_prf_initialModule(app_ble_profile_event_callback);

    blc_svc_addCoreGroup();
    blc_svc_addDisGroup();
    blc_svc_addOtaGroup();
    blc_svc_addBasGroup();

    blc_basic_registerDISControlServer(NULL);

    blc_basic_registerBASControlServer(NULL);
}




void user_ble_init(void)
{
    extern void blc_gap_initial_hci_event(void);
    blc_gap_initial_hci_event();
    ble_host_hci_cmd_transport_layer_receive_process(main_lopp_hci);

    ble_host_v1_init(mac_public,mac_random_static);

    ble_host_hci_reset();

    extern void blc_gap_init_v1(void);
    blc_gap_init_v1();
    blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

    /* SMP Initialization */
    #if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
        blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);

        blc_smp_setSecurityLevel_central(Unauthenticated_Pairing_with_Encryption);
        blc_smp_setSecurityLevel(LE_Security_Mode_1_Level_2);
        blc_smp_setSecurityParameters(Bondable_Mode, 0, LE_Legacy_Pairing, 0, 0, IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
        //blc_smp_setEcdhDebugMode(debug_mode);

        blc_smp_smpParamInit();

        blc_smp_configSecurityRequestSending(SecReq_IMM_SEND, SecReq_PEND_SEND, 1000); //if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection)
    #endif

    extern void ble_host_hci_ext_test_init(void);
    ble_host_hci_read_controller_basic_info();
   // ble_host_hci_register_rx_iso_data_callback(host_recv_iso_data);

    app_ble_profile_init();
    //app_ble_hid_initial();
    //blc_svc_calculateDatabaseHash();

    /* get controller information */
    ble_host_hci_read_controller_basic_info();

    ble_host_hci_set_leg_adv();

    ble_host_hci_set_leg_scan();
}

void app_ble_host_cs_raw_pct_process(uint8_t *data, unsigned int len)
{
   // tlkapi_send_string_data(1, "app_ble_host_cs_raw_pct_process",data,len);
}

#define TEST_DATA_LEN (20)
// 2byte  handle
// 1byte  count
// data
u8 app_test_data[DEVICE_CHAR_INFO_MAX_NUM][TEST_DATA_LEN];
u8 currRcvdSeqNo[DEVICE_CHAR_INFO_MAX_NUM] = {0};

int        AA_dbg_write_cmd_err = 0;
int        AA_dbg_notify_err    = 0;
static u32 device_connection_tick;
static u32 multii_ll_md_start_flag = 0; //0000 0000

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
void feature_md_test_init(void)
{
    device_connection_tick  = clock_time();
    multii_ll_md_start_flag = 0;
}

void feature_md_test_start(void)
{
    for (int i = 0; i < DEVICE_CHAR_INFO_MAX_NUM; i++) {
        if (conn_dev_list[i].conn_state) {
            multii_ll_md_start_flag |= (1 << i);
        }
        currRcvdSeqNo[i] = 0;
    }
    tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][GATT] feature_md_test_start", &multii_ll_md_start_flag, 1);
}

void feature_md_test_mainloop(void)
{
    if (!clock_time_exceed(device_connection_tick, 10000)) {
        return;
    }
    device_connection_tick = clock_time();

    if (multii_ll_md_start_flag) {
        for (int i = 0; i < DEVICE_CHAR_INFO_MAX_NUM; i++) {
            if (conn_dev_list[i].conn_state && (multii_ll_md_start_flag & (1 << i))) { //connection state
                u16 connHandle      = conn_dev_list[i].conn_handle;
                app_test_data[i][0] = connHandle & 0XFF;
                app_test_data[i][1] = connHandle >> 8;
                if (dev_char_get_conn_role_by_connhandle(connHandle) == ACL_ROLE_CENTRAL) { //Central
                    ble_sts_t ret_val = ble_gatts_notify(connHandle, SPP_SER2CLI_DP_HDL, app_test_data[i], 20);
                    if (ret_val == BLE_SUCCESS) {
                        tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][GATT] blc_gatt_pushWriteCommand", &app_test_data[i][0], TEST_DATA_LEN);
                        gpio_toggle(GPIO_LED_GREEN);
                        app_test_data[i][2] += 1;
                    } else {
                        tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][GATT] blc_gatt_pushWriteCommand error code", &ret_val, 1);
                    }
                } else { //Peripheral
                    ble_sts_t ret_val = ble_gatts_notify(connHandle, SPP_SER2CLI_DP_HDL, app_test_data[i], 20);
                    if (BLE_SUCCESS == ret_val) {
                        tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][GATT] blc_gatt_pushHandleValueNotify", &app_test_data[i][0], 20);
                        gpio_toggle(GPIO_LED_GREEN);
                        app_test_data[i][2] += 1;
                    } else {
                        tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][GATT] blc_gatt_pushHandleValueNotify error code", &ret_val, 1);
                    }
                }
            }
            if (app_test_data[i][2] >= 255) {
                app_test_data[i][2] = 0;
                multii_ll_md_start_flag &= ~(1 << i);
                gpio_write(GPIO_LED_GREEN, 0);
            }
        } // for(int i=0; i< MASTER_SLAVE_MAX_NUM; i++)

    } // if(multii_ll_md_start){
}

/******************************************************************************
 * Function: tlkapp_init
 * Descript: user initialization when MCU power on or wake_up from deepSleep mode.
 * Params: None.
 * Return: None.
 * Others: None.
 *******************************************************************************/
int tlkapp_init(void)
{
    user_ble_init();

    tlkapi_printf(APP_LOG_EN, "[APP][INI] sys_clk: pll_clk=%d, cclk=%d, hclk_n22=%d, pclk=%d, mspi_clk=%d\n",
                  sys_clk.pll_clk,sys_clk.cclk,sys_clk.hclk_n22,sys_clk.pclk,sys_clk.mspi_clk);
    tlkapi_printf(APP_LOG_EN, "[APP][INI] d25f ble host init\n");

    return 0;
}

void user_init(void)
{
    #if 1
        /* Debug GPIO */
        gpio_pin_e debug_gpio[] = {GPIO_PB0, GPIO_PB1, GPIO_PB2, GPIO_PB3, GPIO_PB4, GPIO_PB5, GPIO_PB6, GPIO_PB7,
                                   GPIO_PD0, GPIO_PD1, GPIO_PD2, GPIO_PD3,
                                   GPIO_PF0, GPIO_PF1, GPIO_PF2, GPIO_PF3, GPIO_PF4, GPIO_PF5, GPIO_PF6, GPIO_PF7};

        for(u32 i=0; i<(sizeof(debug_gpio)/sizeof(debug_gpio[0])); i++) {
            gpio_function_en(debug_gpio[i]);
            gpio_input_dis(debug_gpio[i]);
            gpio_output_en(debug_gpio[i]);
            gpio_set_level(debug_gpio[i], 0);
        }
    #endif

    #if (UI_KEYBOARD_ENABLE)
        gpio_function_en(GPIO_PG1);
        gpio_input_dis(GPIO_PG1);
        gpio_output_en(GPIO_PG1);
        gpio_set_up_down_res(GPIO_PG1, (gpio_pull_type_e)PM_PIN_PULLDOWN_100K);

        gpio_function_en(GPIO_PG0);
        gpio_input_dis(GPIO_PG0);
        gpio_output_en(GPIO_PG0);
        gpio_set_up_down_res(GPIO_PG0, (gpio_pull_type_e)PM_PIN_PULLDOWN_100K);

        gpio_function_en(GPIO_PG3);
        gpio_input_en(GPIO_PG3);
        gpio_output_dis(GPIO_PG3);
        gpio_set_up_down_res(GPIO_PG3, (gpio_pull_type_e)PM_PIN_PULLUP_10K);

        gpio_function_en(GPIO_PG2);
        gpio_input_en(GPIO_PG2);
        gpio_output_dis(GPIO_PG2);
        gpio_set_up_down_res(GPIO_PG2, (gpio_pull_type_e)PM_PIN_PULLUP_10K);
    #endif

    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_init();
        blc_debug_enableStackLog(STK_LOG_NONE);
    #endif

    blc_readFlashSize_autoConfigCustomFlashSector();

    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);

    sys_n22_init(N22_FW_DOWNLOAD_FLASH_ADDR);
    sys_n22_start();
    gpio_write(GPIO_PB0, 1);
    /* Condor-A0: delay cannot be less than 136ms */
    delay_ms(300);

    #if (HCI_INTERFACE == HCI_UART)
        user_uart_init();
    #elif (HCI_INTERFACE == HCI_SHAREMEMORY)
        tlk_multi_core_communication_init();
        tlk_d25f_register_hci_receive_cb(TLK_SHARE_MEMOTY_MESSAGE_TYPE_BLE, ble_host_hci_rx_packet);
        delay_ms(1);
    #endif
    feature_md_test_init();
}

void main_lopp_hci(void)
{
    tlk_multi_core_communication_loop();
}

void user_ble_main_loop(void)
{
    blt_gap_mainloop_v1();
    ble_host_v1_main_loop();
}

/**
 * @brief      BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void main_loop(void)
{
    #if TLK_STK_BLE_ENABLE
        user_ble_main_loop();
    #endif

    #if (HCI_INTERFACE == HCI_UART)
        uart_hci_mainloop();
    #elif (HCI_INTERFACE == HCI_SHAREMEMORY)
        tlk_multi_core_communication_loop();
    #endif

    #if (UI_KEYBOARD_ENABLE)
        proc_keyboard(0, 0, 0);
    #endif

    feature_md_test_mainloop();

    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif

    proc_central_role_unpair();

    static volatile u32 nowTick = 0;
    if(clock_time_exceed(nowTick, 500*1000)){
        nowTick = clock_time();
        gpio_toggle(GPIO_LED_GREEN);
    }
}


#endif //end of (FEATURE_TEST_MODE == ...)
