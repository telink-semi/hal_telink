#ifndef TAGSDK_TAGDEVICEINFO_H_
#define TAGSDK_TAGDEVICEINFO_H_

#include "TagConfig.h"

#if defined(TAG_CONFIG_USE_DEVICE_INFO_HEADER)
/* Private key */
const char *conf_device_seckey_curve25519 = "ML0C3FpDXLa1xILmslqkdFJlGPo9VBAy+8IHbAIYrkk=";

/* Public key */
const char *conf_device_pubkey_ed25519 = "FEX+lBLyKB22AWgy+JyTirtA2WcsiLtk/Rw8H64DJdM=";

/* Serial number */
const char *conf_device_serial_number = "STDK0yOnbygexFck";
#endif /* TAG_CONFIG_USE_DEVICE_INFO_HEADER */

#endif /* TAGSDK_TAGDEVICEINFO_H_ */
