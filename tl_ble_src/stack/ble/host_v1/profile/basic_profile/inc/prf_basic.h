/*
 * It is used to implement profiles commonly used before Bluetooth LE Core v5.2,
 * usually these are the Basic services of LE devices, so they are also called basic profiles.
 * all profile: Generic Attribute Service(GATT), Generic Access Profile Service(GAP), Device Information Service(DIS),
 * Battery Service(BAS), Scan Parameters Service(ScPS).
 */

enum {
    BASIC_SERVICE_ID_START = PRF_BASIC_SERVICE_ID_START - 1,
    SERVICE_ID_GATT,
    SERVICE_ID_GAP,
    SERVICE_ID_BAS,
    SERVICE_ID_DIS,
    SERVICE_ID_SCPS,
};
