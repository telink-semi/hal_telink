#ifndef TRNG_CONFIG_H
#define TRNG_CONFIG_H


#include "lib/include/crypto_common/common_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************************
 *******************************    TRNG config    **********************************
 ************************************************************************************/

//trng IP base address
//#define TRNG_BASE_ADDR (0x80103000U) //TRNG register base address

#define CONFIG_TRNG_GENERATE_BY_HARDWARE

#define TRNG_RO_ENTROPY //enable Ring Oscillator entropy
//#define TRNG_TERO_ENTROPY        //enable Transient Effect Ring Oscillator entropy

#define TRNG_DELAY_COUNTER             (10U)
#define TRNG_TIMEOUT_COUNTER_THRESHOLD (100000U)
#define TRNG_ERROR_COUNTER_THRESHOLD   (20U)


#ifdef __cplusplus
}
#endif


#endif
