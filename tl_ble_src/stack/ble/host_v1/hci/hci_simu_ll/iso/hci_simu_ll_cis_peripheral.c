#include "tl_common.h"
#include "stack/ble/ble.h"

#include "stack/ble/host_v1/hci/inc/ble_hci_cmd.h"
#include "stack/ble/host_v1/hci/inc/ble_hci_log.h"
#include "stack/ble/host_v1/hci/le_cmd/inc/hci_cmd_le_cis.h"

#include "hci_simu_ll_cis_peripheral.h"

ble_sts_t ble_hci_ll_acceptCisRequest(u16 cisHandle)
{
    struct ble_hci_le_accept_cis_request_cp accept_cis_request = {0};
    accept_cis_request.conn_handle                             = cisHandle;

    return ble_host_hci_le_accept_cis_request(&accept_cis_request);
}

ble_sts_t ble_hci_ll_rejectCisReq(u16 cisHandle, u8 reason)
{
    struct ble_hci_le_reject_cis_request_cp reject_cis_request    = {0};
    struct ble_hci_le_reject_cis_request_rp reject_cis_request_rp = {0};

    reject_cis_request.conn_handle = cisHandle;
    reject_cis_request.reason      = reason;

    return ble_host_hci_le_reject_cis_request(&reject_cis_request, &reject_cis_request_rp);
}