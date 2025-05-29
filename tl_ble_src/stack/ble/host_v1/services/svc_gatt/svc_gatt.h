
//sdk fix service uuid handle
#define SERVICE_GENERIC_ACCESS_HDL                      SERVICE_GATT_START_HANDLE
#define GAP_MAX_HDL_NUM                                 0x0F
#define SERVICE_GENERIC_ATTRIBUTE_HDL                   SERVICE_GENERIC_ACCESS_HDL + GAP_MAX_HDL_NUM
#define GATT_MAX_HDL_NUM                                0x10
#define SERVICE_DEVICE_INFORMATION_HDL                  SERVICE_GENERIC_ATTRIBUTE_HDL + GATT_MAX_HDL_NUM
#define DIS_MAX_HDL_NUM                                 0x20
#define SERVICE_BATTERY_HDL                             SERVICE_DEVICE_INFORMATION_HDL + DIS_MAX_HDL_NUM
#define BAS_MAX_HDL_NUM                                 0x30
#define SERVICE_SCAN_PARAMETERS_HDL                     SERVICE_BATTERY_HDL + BAS_MAX_HDL_NUM
#define ScPS_MAX_HDL_NUM                                0x10

