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

#include "app.h"
#include "app_ui.h"
#include "app_cs.h"

_attribute_data_retention_ static u8 mac_public[6]={0};
_attribute_data_retention_ static u8 mac_random_static[6]={0};

void main_lopp_hci(void);
extern int blt_gap_mainloop_v1(void);
extern void ble_host_v1_main_loop(void);
extern int ble_host_hci_le_create_connection(const struct ble_hci_le_create_conn_cp *p_create_conn);

#if (HCI_INTERFACE == HCI_UART)
volatile u8 uart_dma_send_flag = 1;
#endif

/* app ble event callback parameter */
typedef struct
{
    int event_id;
    void (*event_callback)(u16 connHandle, const void *pEvent);
}app_ble_event_t;


void ble_host_sal_hci_send_packet(const uint8_t *data, uint16_t len)
{
    tlkapi_send_string_data(APP_HCI_LOG_EN, "hci tx",data, len);

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
    u8 adv_data[] = {2, DT_FLAGS, 0x05, 10,DT_COMPLETE_LOCAL_NAME, 't','l','3','2','2','x','_','c','s'};
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
}

static void app_acl_disconnected_event_callback(u16 aclHandle, const void *pEvent)
{
    (void) aclHandle;
    const blc_prf_aclDisconnEvt_t *pAclDisconnectEvt = pEvent;
    tlkapi_send_string_data(APP_LOG_EN, "ble disconnect event",
                            (u8 *)(uintptr_t)(const void*)pAclDisconnectEvt, sizeof(blc_prf_aclDisconnEvt_t));

    app_ble_adv_start();
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

/**
 * @brief   Unicast Server register profile event callback.
 */
_attribute_ble_data_retention_ app_ble_event_t profile_event[] = {
    /* Event for controller or Host */
    {PRF_EVTID_ACL_CONNECT,           app_acl_connected_event_callback        },
    {PRF_EVTID_ACL_DISCONNECT,        app_acl_disconnected_event_callback     },
    {PRF_EVTID_SMP_SECURITY_DONE,     app_acl_connect_security_done_callback  },
    {PRF_EVTID_ACL_CONNECT_UPDATE,    app_acl_connect_interval_update_callback},
    {PRF_EVTID_LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES_COMPLETE, app_le_cs_read_remote_support_capabilities_complete_event_handle},
    {PRF_EVTID_LE_CS_CONFIG_COMPLETE, app_le_cs_config_complete_event_handle},
    {PRF_EVTID_LE_CS_PROCEDURE_ENABLE_COMPLETE, app_le_cs_procedure_enable_complete_event_handle},
    {PRF_EVTID_LE_CS_SUBEVENT_RESULT, app_le_cs_subevent_result_event_handle},
    {PRF_EVTID_LE_CS_SUBEVENT_RESULT_CONTINUE, app_le_cs_subevent_result_continue_event_handle},
    /* Event for Client SDP */
    {PRF_EVTID_CLIENT_SDP_FOUND,      app_client_sdp_found_callback           },
    {PRF_EVTID_CLIENT_SDP_FAIL,       app_client_sdp_not_found_callback       },
    {PRF_EVTID_CLIENT_ALL_SDP_OVER,   app_client_all_sdp_over_callback        },
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
static uint8_t hostMallocBuffer[HOST_MALLOC_BUFF_SIZE_1];

void app_ble_profile_init(void)
{
    blc_prf_initialModule(app_ble_profile_event_callback, hostMallocBuffer, HOST_MALLOC_BUFF_SIZE_1);
//    blc_prf_initialModule(app_ble_profile_event_callback);

    blc_svc_addCoreGroup();
    blc_svc_addDisGroup();
    blc_svc_addOtaGroup();
    blc_svc_addBasGroup();

    blc_basic_registerDISControlServer(NULL);

    blc_basic_registerBASControlServer(NULL);

    //RAS server initial
    const blc_rass_regParam_t rasParam = {
#if (GOOGLE_CS_REFL_ROLE_EN)
        .ras_feature.realTimeProcedureDataSupport = 1,
#else
        .ras_feature.realTimeProcedureDataSupport = 0,
#endif
        .ras_feature.getLostProcedureDataSegmentsSupport = 1,
        .ras_feature.abortOperationSupport               = 1,
        .ras_feature.filterProcedureDataSupport          = 0,
    };
    blc_rap_registerRasProfileControlServer(&rasParam);
    //Initialize RAS client

//    blc_rap_registerRasProfileControlClient(NULL);
}

#if 0
#define MKEY_VOL_UP 0x00E9
#define MKEY_VOL_DN 0x00EA

//void app_ble_hid_initial(void)
//{
//    blc_basic_registerDISControlServer(NULL);
//    blc_basic_registerSCPSControlServer(NULL);
//    blc_hid_registerHIDControlServer(NULL);
////    tlkdrv_key_registerVendorConfig1Callback(app_ble_hid_report_volume_increment);
////    tlkdrv_key_registerVendorConfig2Callback(app_ble_hid_report_volume_decrement);
//}

static uint16_t acl_hid_connHandle;

static void app_ble_hid_report_consume_control(uint16_t consumer_key)
{
    if (acl_hid_connHandle) {
        blc_hids_notifyInputReport(acl_hid_connHandle,
                                   HID_REPORT_ID_CONSUME_CONTROL_INPUT,
                                   (u8 *)&consumer_key,
                                   2);
        consumer_key = 0;
        blc_hids_notifyInputReport(acl_hid_connHandle,
                                   HID_REPORT_ID_CONSUME_CONTROL_INPUT,
                                   (u8 *)&consumer_key,
                                   2);
    }
}

static void app_ble_hid_report_volume_increment(void)
{
    app_ble_hid_report_consume_control(MKEY_VOL_UP);
}

static void app_ble_hid_report_volume_decrement(void)
{
    app_ble_hid_report_consume_control(MKEY_VOL_DN);
}

void app_ble_hid_initial(void)
{
    blc_basic_registerDISControlServer(NULL);
    blc_basic_registerSCPSControlServer(NULL);
    blc_hid_registerHIDControlServer(NULL);
//    tlkdrv_key_registerVendorConfig1Callback(app_ble_hid_report_volume_increment);
//    tlkdrv_key_registerVendorConfig2Callback(app_ble_hid_report_volume_decrement);
}
#endif

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

//    ble_host_hci_set_leg_scan();
}

void app_ble_host_cs_raw_pct_process(uint8_t *data, unsigned int len)
{
    tlkapi_send_string_data(1, "app_ble_host_cs_raw_pct_process",data,len);
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
        gpio_pin_e debug_gpio[] = {GPIO_PG4, GPIO_PG5, GPIO_PG6, GPIO_PG7, GPIO_PH0, GPIO_PH1, GPIO_PH2, GPIO_PH3, GPIO_PH4, GPIO_PH5, GPIO_PH6, GPIO_PH7 };
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
    gpio_write(GPIO_PG4, 1);
    /* Condor-A0: delay cannot be less than 136ms */
    delay_ms(300);

#if (HCI_INTERFACE == HCI_UART)
    user_uart_init();
#elif (HCI_INTERFACE == HCI_SHAREMEMORY)
    tlk_multi_core_communication_init();
    tlk_d25f_register_hci_receive_cb(TLK_SHARE_MEMOTY_MESSAGE_TYPE_BLE, ble_host_hci_rx_packet);
        delay_ms(1);
#endif
}

void main_lopp_hci(void)
{
    tlk_multi_core_communication_loop();
}

void user_ble_main_loop(void)
{
    blt_gap_mainloop_v1();
    ble_host_v1_main_loop();
    blc_prf_main_loop();
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

#if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_handler();
#endif

    static volatile u32 nowTick = 0;
    if(clock_time_exceed(nowTick, 500*1000)){
        nowTick = clock_time();
        gpio_toggle(GPIO_LED_GREEN);
    }
}



