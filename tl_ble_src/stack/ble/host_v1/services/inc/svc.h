
#define SERVICE_GATT_START_HANDLE                       0x0001

#define SERVICE_LE_AUDIO_START_HDL                      0x0200

#define SERVICE_CHANNEL_SOUNDING_START_HDL              0x0800

#define SERVICE_HID_START_HDL                           0x0880

//Telink private Service all 128 uuid
#define SERVICE_TELINK_PRIVATE_START_HDL                0x8000

// TODO: remove later.
#include "vendor/common/user_config.h"

#if (DUAL_CORE_MODE_ENABLED) && (ENABLE_LE_NEW_ATT_PROCESS)
#define blc_gatts_addAttributeServiceGroup          ble_host_add_attribute_service_group
#define blc_gatts_removeAttributeServiceGroup       ble_host_remove_attribute_service_group
#define blc_gatts_calculateDatabaseHash(conn, databaseHash) ble_gatts_calculate_database_hash(databaseHash)
#else
#include "stack/ble/ble_common.h"
#include "stack/ble/host/att/atts.h"
extern void blc_gatts_addAttributeServiceGroup_old(atts_group_t *pGroup);
extern void blc_gatts_removeAttributeServiceGroup_old(u16 startHandle);
extern bool blc_gatts_calculateDatabaseHash(u16 connHandle, u8 *databaseHash);
#define blc_gatts_addAttributeServiceGroup(pGroup)  blc_gatts_addAttributeServiceGroup_old((atts_group_t *)pGroup)
#define blc_gatts_removeAttributeServiceGroup       blc_gatts_removeAttributeServiceGroup_old
#endif
