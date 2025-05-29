#include "common/types.h"

#include "../gatt/inc/gatt.h"

#include "inc/svc_format.h"

const uint16_t attr16UuidLen = 2;
const uint16_t attr128UuidLen = 16;
const uint16_t gattIncludeValueLen = 6;

const uint16_t characteristicPropertiesLen = CHARACTERISTIC_PROPERTIES_LENGTH;

const uint8_t charPropRead = CHAR_PROP_READ;
const uint8_t charPropReadWrite = CHAR_PROP_READ | CHAR_PROP_WRITE;
const uint8_t charPropReadWriteWithout = CHAR_PROP_READ | CHAR_PROP_WRITE_WITHOUT_RSP;
const uint8_t charPropReadWriteWriteWithout = CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RSP;
const uint8_t charPropReadWriteWriteWithoutNotify = CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RSP | CHAR_PROP_NOTIFY;
const uint8_t charPropReadWriteNotify = CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_NOTIFY;
const uint8_t charPropReadWriteWithoutNotify = CHAR_PROP_READ | CHAR_PROP_WRITE_WITHOUT_RSP | CHAR_PROP_NOTIFY;
const uint8_t charPropReadNotify = CHAR_PROP_READ | CHAR_PROP_NOTIFY;


const uint8_t charPropWrite = CHAR_PROP_WRITE;
const uint8_t charPropWriteWithout = CHAR_PROP_WRITE_WITHOUT_RSP;
const uint8_t charPropWriteWriteWithout = CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RSP;
const uint8_t charPropWriteIndicate = CHAR_PROP_WRITE | CHAR_PROP_INDICATE;
const uint8_t charPropWriteNotifyIndicate = CHAR_PROP_WRITE | CHAR_PROP_NOTIFY | CHAR_PROP_INDICATE;
const uint8_t charPropWriteWriteWithoutNotify = CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RSP | CHAR_PROP_NOTIFY;
const uint8_t charPropWriteWithoutIndicate = CHAR_PROP_WRITE_WITHOUT_RSP | CHAR_PROP_INDICATE;

const uint8_t charPropNotify = CHAR_PROP_NOTIFY;
const uint8_t charPropIndicate = CHAR_PROP_INDICATE;
const uint8_t charPropNotifyIndicate = CHAR_PROP_NOTIFY | CHAR_PROP_INDICATE;

const uint8_t clientCharacteristicConfiguration[2] = { 0x00, 0x00 };
const uint16_t clientCharacteristicConfigurationLen = sizeof(clientCharacteristicConfiguration);
