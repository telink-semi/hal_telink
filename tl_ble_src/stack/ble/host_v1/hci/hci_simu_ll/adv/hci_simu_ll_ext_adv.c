#include "tl_common.h"
#include "stack/ble/ble.h"

#include "stack/ble/host_v1/hci/inc/ble_hci_cmd.h"
#include "stack/ble/host_v1/hci/inc/ble_hci_log.h"
#include "stack/ble/host_v1/hci/le_cmd/inc/hci_cmd_le_ext_adv.h"

#include "hci_simu_ll_ext_adv.h"

ble_sts_t ble_hci_ll_setExtAdvParam(u8 adv_handle, advEvtProp_type_t adv_evt_prop, u32 pri_advInter_min, u32 pri_advInter_max, adv_chn_map_t pri_advChnMap, own_addr_type_t ownAddrType, u8 peerAddrType, u8 *peerAddr, adv_fp_type_t advFilterPolicy, tx_power_t adv_tx_pow, le_phy_type_t pri_adv_phy, u8 sec_adv_max_skip, le_phy_type_t sec_adv_phy, u8 adv_sid, u8 scan_req_notify_en)
{
    BLE_HOST_HCI_ASSERT(adv_handle < BLE_HCI_LE_MAX_SUPPORTED_EXT_ADV_SET_COUNT);
    BLE_HOST_HCI_ASSERT(peerAddr);

    struct ble_hci_le_set_ext_adv_params_cp ext_adv_params    = {0};
    struct ble_hci_le_set_ext_adv_params_rp ext_adv_params_rp = {0};

    ext_adv_params.adv_handle = adv_handle;
    ext_adv_params.props      = adv_evt_prop;

    ext_adv_params.pri_itvl_min[0] = pri_advInter_min & 0xff;
    ext_adv_params.pri_itvl_min[1] = (pri_advInter_min >> 8) & 0xff;
    ext_adv_params.pri_itvl_min[2] = (pri_advInter_min >> 16) & 0xff;

    ext_adv_params.pri_itvl_max[0] = pri_advInter_max & 0xff;
    ext_adv_params.pri_itvl_max[1] = (pri_advInter_max >> 8) & 0xff;
    ext_adv_params.pri_itvl_max[2] = (pri_advInter_max >> 16) & 0xff;

    ext_adv_params.pri_chan_map = pri_advChnMap;

    ext_adv_params.own_addr_type  = ownAddrType;
    ext_adv_params.peer_addr_type = peerAddrType;

    memcpy(ext_adv_params.peer_addr, peerAddr, 6);

    ext_adv_params.filter_policy  = advFilterPolicy;
    ext_adv_params.tx_power       = adv_tx_pow;
    ext_adv_params.pri_phy        = pri_adv_phy;
    ext_adv_params.sec_max_skip   = sec_adv_max_skip;
    ext_adv_params.sec_phy        = sec_adv_phy;
    ext_adv_params.sid            = adv_sid;
    ext_adv_params.scan_req_notif = scan_req_notify_en;

    return ble_host_hci_le_set_ext_adv_params(&ext_adv_params, &ext_adv_params_rp);
}

ble_sts_t ble_hci_ll_setExtAdvData(u8 adv_handle, int advData_len, const u8 *advData)
{
    BLE_HOST_HCI_ASSERT(adv_handle < BLE_HCI_LE_MAX_SUPPORTED_EXT_ADV_SET_COUNT);
    BLE_HOST_HCI_ASSERT(advData);
    BLE_HOST_HCI_ASSERT(advData_len <= BLE_HCI_LE_MAX_EXT_ADV_DATA_LEN);

    struct ble_hci_le_set_ext_adv_data_full_cp ext_adv_data = {0};
    ext_adv_data.adv_handle                                 = adv_handle;
    /**
     * 0x00 Intermediate fragment of fragmented extended advertising data
     * 0x01 First fragment of fragmented extended advertising data
     * 0x02 Last fragment of fragmented extended advertising data
     * 0x03 Complete extended advertising data
     * 0x04 Unchanged data (just update the Advertising DID)
     * All other values Reserved for future use
     */
    ext_adv_data.operation = 3;
    /**
     * 0x00 The Controller may fragment all Host advertising data
     * 0x01 The Controller should not fragment or should minimize fragmentation of Host advertising data
     */
    ext_adv_data.fragment_pref = 1;

    ext_adv_data.adv_data_len = advData_len;
    memcpy(ext_adv_data.adv_data, advData, advData_len);

    return ble_host_hci_le_set_ext_adv_data(&ext_adv_data);
}

ble_sts_t ble_hci_ll_setExtScanRspData(u8 adv_handle, int scanRspData_len, const u8 *scanRspData)
{
    BLE_HOST_HCI_ASSERT(adv_handle < BLE_HCI_LE_MAX_SUPPORTED_EXT_ADV_SET_COUNT);
    BLE_HOST_HCI_ASSERT(scanRspData);
    BLE_HOST_HCI_ASSERT(scanRspData_len <= BLE_HCI_LE_MAX_EXT_ADV_DATA_LEN);

    struct ble_hci_le_set_ext_scan_rsp_data_full_cp ext_scan_rsp_data = {0};
    ext_scan_rsp_data.adv_handle                                      = adv_handle;
    /**
     * 0x00 Intermediate fragment of fragmented scan response data
     * 0x01 First fragment of fragmented scan response data
     * 0x02 Last fragment of fragmented scan response data
     * 0x03 Complete scan response data
     * All other values Reserved for future use
    */
    ext_scan_rsp_data.operation = 3;
    /**
     * 0x00 The Controller may fragment all scan response data
     * 0x01 The Controller should not fragment or should minimize fragmentation of scan response data
    */
    ext_scan_rsp_data.fragment_pref = 1;

    ext_scan_rsp_data.scan_rsp_len = scanRspData_len;
    memcpy(ext_scan_rsp_data.scan_rsp, scanRspData, scanRspData_len);

    return ble_host_hci_le_set_ext_scan_rsp_data(&ext_scan_rsp_data);
}

ble_sts_t ble_hci_ll_setExtAdvEnable(adv_en_t enable, u8 adv_handle, u16 duration, u8 max_extAdvEvt)
{
    BLE_HOST_HCI_ASSERT(adv_handle < BLE_HCI_LE_MAX_SUPPORTED_EXT_ADV_SET_COUNT);

    struct ble_hci_le_set_ext_adv_enable_full_cp ext_adv_enable = {0};
    ext_adv_enable.enable                                       = enable;
    ext_adv_enable.num_sets                                     = 1;
    ext_adv_enable.sets->adv_handle                             = adv_handle;
    ext_adv_enable.sets->duration                               = duration;
    ext_adv_enable.sets->max_events                             = max_extAdvEvt;

    return ble_host_hci_le_set_ext_adv_enable(&ext_adv_enable);
}

ble_sts_t ble_hci_ll_setAdvRandomAddr(u8 adv_handle, u8 *rand_addr);

ble_sts_t ble_hci_ll_removeAdvSet(u8 adv_handle)
{
    BLE_HOST_HCI_ASSERT(adv_handle < BLE_HCI_LE_MAX_SUPPORTED_EXT_ADV_SET_COUNT);

    struct ble_hci_le_remove_adv_set_cp remove_adv_set = {0};
    remove_adv_set.adv_handle                          = adv_handle;

    return ble_host_hci_le_remove_adv_set(&remove_adv_set);
}

ble_sts_t ble_hci_ll_clearAdvSets(void)
{
    return ble_host_hci_le_clear_adv_sets();
}
