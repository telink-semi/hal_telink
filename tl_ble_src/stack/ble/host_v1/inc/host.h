
#include "ble_host.h"

#include "ble_host.h"

// include all hci/inc header files.
#include "../hci/inc/ble_acl_data.h"
#include "../hci/inc/ble_hci_cmd.h"
#include "../hci/inc/ble_hci_evt.h"
#include "../hci/inc/ble_hci.h"
#include "../hci/inc/ble_iso_data.h"

// include all hci/cmd header files.
#include "../hci/cmd/inc/hci_cmd_bb.h"
#include "../hci/cmd/inc/hci_cmd_info_param.h"
#include "../hci/cmd/inc/hci_cmd_link_ctrl.h"
#include "../hci/cmd/inc/hci_cmd_link_policy.h"
#include "../hci/cmd/inc/hci_cmd_status_param.h"
#include "../hci/cmd/inc/hci_cmd_test.h"

// include all hci/le_cmd header files.
#include "../hci/le_cmd/inc/hci_cmd_le_adv.h"
#include "../hci/le_cmd/inc/hci_cmd_le_bis.h"
#include "../hci/le_cmd/inc/hci_cmd_le_cis.h"
#include "../hci/le_cmd/inc/hci_cmd_le_ext_adv.h"
#include "../hci/le_cmd/inc/hci_cmd_le_ext_init.h"
#include "../hci/le_cmd/inc/hci_cmd_le_ext_scn.h"
#include "../hci/le_cmd/inc/hci_cmd_le_init.h"
#include "../hci/le_cmd/inc/hci_cmd_le_iso.h"
#include "../hci/le_cmd/inc/hci_cmd_le_misc.h"
#include "../hci/le_cmd/inc/hci_cmd_le_pcl.h"
#include "../hci/le_cmd/inc/hci_cmd_le_prd.h"
#include "../hci/le_cmd/inc/hci_cmd_le_scn.h"
#include "../hci/le_cmd/inc/hci_cmd_le_sync.h"
#include "../hci/le_cmd/inc/hci_cmd_le_test.h"


//qihang todo start
// include all hci/le_event header files.
// include all l2cap header files.
#include "../l2cap/inc/ble_l2cap.h"
#include "../l2cap/inc/ble_l2cap_interface.h"


// include all l2cap/smp header files.
#include "../l2cap/smp/inc/ble_smp.h"

// include all l2cap/att header files.
#include "../l2cap/att/inc/ble_att.h"
#include "../l2cap/att/inc/ble_att_pdu_format.h"
#include "../l2cap/att/inc/uuid16bit.h"
//qihang todo end