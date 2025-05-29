#include "tl_common.h"
#include "stack/ble/ble.h"

#include "stack/ble/host_v1/hci/inc/ble_hci_cmd.h"
#include "stack/ble/host_v1/hci/inc/ble_hci_log.h"
#include "stack/ble/host_v1/hci/le_cmd/inc/hci_cmd_le_cis.h"

#include "hci_simu_ll_iso.h"

void ble_hci_iso_enableSduToHostTimestamp(u8 en);

ble_sts_t ble_hci_ll_setupIsoDataPath(u16 conn_handle, dat_path_dir_t dir, dat_path_id_t id, u8 cid_assignNum, u16 cidcompId, u16 cid_vendorDef, u32 control_dly, u8 codec_cfg_len, u8 codec_cfg1, u8 codec_cfg2, u8 codec_cfg3, u8 codec_cfg4)
{
    (void)cid_assignNum; //unused, remove warning
    (void)cidcompId;     //unused, remove warning
    (void)cid_vendorDef; //unused, remove warning
    (void)control_dly;   //unused, remove warning
    (void)codec_cfg1;    //unused, remove warning
    (void)codec_cfg2;    //unused, remove warning
    (void)codec_cfg3;    //unused, remove warning
    (void)codec_cfg4;    //unused, remove warning

    struct ble_hci_le_setup_iso_data_path_full_cp setup_iso_data_path    = {0};
    struct ble_hci_le_setup_iso_data_path_rp      setup_iso_data_path_rp = {0};

    setup_iso_data_path.conn_handle         = conn_handle;
    setup_iso_data_path.data_path_dir       = dir;
    setup_iso_data_path.data_path_id        = id;
    setup_iso_data_path.codec_id[0]         = 0;
    setup_iso_data_path.codec_id[1]         = 0;
    setup_iso_data_path.codec_id[2]         = 0;
    setup_iso_data_path.controller_delay[0] = 0;
    setup_iso_data_path.codec_config_len    = codec_cfg_len;
    if (codec_cfg_len > 0) {
        setup_iso_data_path.codec_config[0] = 0;
    }
    if (codec_cfg_len > 1) {
        setup_iso_data_path.codec_config[1] = 0;
    }
    if (codec_cfg_len > 2) {
        setup_iso_data_path.codec_config[2] = 0;
    }
    if (codec_cfg_len > 3) {
        setup_iso_data_path.codec_config[3] = 0;
    }

    return ble_host_hci_le_setup_iso_data_path(&setup_iso_data_path, &setup_iso_data_path_rp);
}

ble_sts_t ble_hci_ll_removeIsoDataPath(u16 conn_handle, dp_dir_msk_t dir_mask)
{
    (void)dir_mask; //unused, remove warning

    struct ble_hci_le_remove_iso_data_path_cp remove_iso_data_path    = {0};
    struct ble_hci_le_remove_iso_data_path_rp remove_iso_data_path_rp = {0};

    remove_iso_data_path.conn_handle   = conn_handle;
    remove_iso_data_path.data_path_dir = 0;

    return ble_host_hci_le_remove_iso_data_path(&remove_iso_data_path, &remove_iso_data_path_rp);
}

ble_sts_t ble_hci_iso_sendData(u16 handle, u8 *pData, u16 len);


void ble_hci_ial_register_sdu_pop_callback(ial_sdu_pop_callback_t callback);
