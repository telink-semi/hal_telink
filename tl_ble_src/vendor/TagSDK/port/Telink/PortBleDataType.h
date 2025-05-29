#ifndef TAGSDK_PORT_DUMMY_PORTBLEDATATYPE_H_
#define TAGSDK_PORT_DUMMY_PORTBLEDATATYPE_H_

#include <stdint.h>

/**
 * @brief Contain connection information needed by each solution
 */
typedef struct {
    uint16_t conn_idx;
    int    auth_result;
} PortBleConnInfo;

/**
 * @brief Contain attribute information needed by each solution
 */
typedef struct {
    uint16_t handle;
    uint8_t tagCharIndex;
} PortBleAttrInfo;

/**
 * @def Max MTU size
 */
#define TAG_MAX_MTU     (243)

#define SUPPORT_GATT_UNREGISTER 1

typedef unsigned  long int   uint32_t;

#endif /* TAGSDK_PORT_DUMMY_PORTBLEDATATYPE_H_ */
