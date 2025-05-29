#ifndef STACK_MULTICORECOMM_COMM_H_
#define STACK_MULTICORECOMM_COMM_H_

#include "common/config/user_config.h"
#include "tl_common.h"
#ifndef TLK_MESSAGE_N22
    #define TLK_MESSAGE_N22  0
#endif

#ifndef TLK_MESSAGE_D25F
    #define TLK_MESSAGE_D25F 0
#endif


void tlk_multi_core_communication_init(void);


void tlk_multi_core_communication_loop(void);

#endif /* STACK_MULTICORECOMM_COMM_H_ */
