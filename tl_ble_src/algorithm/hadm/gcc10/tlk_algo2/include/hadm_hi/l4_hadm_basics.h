/**
 * Project:     Lambda:4 HADM Library
 *
 * @file        l4_hadm_basics.h
 * @brief:      Basic defines.
 *
 * Copyright 2023 Lambda:4 Entwicklungen GmbH, Germany.
 *
 * All rights reserved. Using, copying, publishing or distributing
 * is not permitted without prior written agreement.
 */
#ifndef _L4_HADM_BASICS_H
#define _L4_HADM_BASICS_H

//#define L4_HADM_CFG_MAXLITE_ACC_LEVEL (5)
#define L4_HADM_CFG_NORMAL_ACC_LEVEL (9)
#define L4_HADM_CFG_HIGH_ACC_LEVEL (10)

// determine the maximum accuracy level in compilation units
#ifndef L4_HADM_CFG_MAX_ACC_LEVEL

//#pragma message("L4_HADM_CFG_MAX_ACC_LEVEL not defined, using L4_HADM_CFG_HIGH_ACC_LEVEL")
#define L4_HADM_CFG_MAX_ACC_LEVEL (L4_HADM_CFG_HIGH_ACC_LEVEL)

//#else
//
//#if L4_HADM_CFG_MAX_ACC_LEVEL < L4_HADM_CFG_HIGH_ACC_LEVEL
//#pragma message("L4_HADM_CFG_MAX_ACC_LEVEL defined lower than L4_HADM_CFG_HIGH_ACC_LEVEL")
//#else
//#pragma message("L4_HADM_CFG_MAX_ACC_LEVEL defined above L4_HADM_CFG_NORMAL_ACC_LEVEL")
//#endif

#endif  // L4_HADM_CFG_MAX_ACC_LEVEL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef L4HADM_XTAPI_SUPPORTS_PCFS
 #define L4HADM_XTAPI_SUPPORTS_L4RTT
#endif

#undef L4HADM_ENABLE_SECOND_IQDATASET

#ifndef L4_HADM_MAXANTENNAPATHS

/** maximum number of antenna paths supported. */
#define L4_HADM_MAXANTENNAPATHS (4)
#endif 

/** maximum number of Xsubevents per procedure supported. */
#define _L4_HADM_MAX_SUBEVENTS (32)
#define L4_HADM_MAX_SUBEVENTS_SMALL (4)
#define L4_HADM_MAX_SUBEVENTS_MEDIUM (32)
#define L4_HADM_MAX_SUBEVENTS_LARGE (32)
#define L4_HADM_MAX_NUM_MODE0_STORES_SMALL (L4_MAX_NUM_MODE0_STORE_PER_SUBEVENT * L4_HADM_MAX_SUBEVENTS_SMALL)
#define L4_HADM_MAX_NUM_MODE0_STORES_MEDIUM (L4_MAX_NUM_MODE0_STORE_PER_SUBEVENT * L4_HADM_MAX_SUBEVENTS_MEDIUM)
#define L4_HADM_MAX_NUM_MODE0_STORES_LARGE (L4_MAX_NUM_MODE0_STORE_PER_SUBEVENT * L4_HADM_MAX_SUBEVENTS_LARGE)

/** maximum number of frequencies/CS steps supported. */
#define _L4_HADM_MAXFREQUENCIES (106)
#define L4_HADM_MAXFREQUENCIES_SMALL (96)
#define L4_HADM_MAXFREQUENCIES_MEDIUM (96)
#define L4_HADM_MAXFREQUENCIES_LARGE (106)

/** maximum number of frequencies aft linear sort supported. */
#define _L4_HADM_MAX_LINEAR_SORT_FREQUENCIES (79)
#define L4_HADM_MAX_LINEAR_SORT_FREQUENCIES_SMALL (75)
#define L4_HADM_MAX_LINEAR_SORT_FREQUENCIES_MEDIUM (75)
#define L4_HADM_MAX_LINEAR_SORT_FREQUENCIES_LARGE (79)

/** maximum number of BLE RTT steps supported. */
#define L4_MAX_RTT_BLE_STEPS (79)

/** maximum matrix sizes. */
#define _HADMSOLV4_HADM_MAXMATRIX_SIZE (24)
#define HADMSOLV4_HADM_MAXMATRIX_SIZE_SMALL (20)
#define HADMSOLV4_HADM_MAXMATRIX_SIZE_MEDIUM (24)
#define HADMSOLV4_HADM_MAXMATRIX_SIZE_LARGE (26)

/** field size for FFT calculation. */
#define _L4HADM_FFT_FIELDSIZE (1024)
#define L4HADM_FFT_FIELDSIZE_SMALL (512)
#define L4HADM_FFT_FIELDSIZE_MEDIUM (512)
#define L4HADM_FFT_FIELDSIZE_LARGE (1024)

#define L4_HADM_IQ_MAXVAL (2048)

#define _ACM180_MAX_MATRIXSIZE (12)
#define ACM180_MAX_MATRIXSIZE_SMALL (8)
#define ACM180_MAX_MATRIXSIZE_MEDIUM (12)
#define ACM180_MAX_MATRIXSIZE_LARGE (12)

#define _L4HADM_BUGLIST_MAXBUGS ((_L4_HADM_MAX_LINEAR_SORT_FREQUENCIES * L4_HADM_MAXANTENNAPATHS) >> 1)
#define L4HADM_BUGLIST_MAXBUGS_SMALL ((L4_HADM_MAX_LINEAR_SORT_FREQUENCIES_SMALL * L4_HADM_MAXANTENNAPATHS) >> 1)
#define L4HADM_BUGLIST_MAXBUGS_MEDIUM ((L4_HADM_MAX_LINEAR_SORT_FREQUENCIES_MEDIUM * L4_HADM_MAXANTENNAPATHS) >> 1)
#define L4HADM_BUGLIST_MAXBUGS_LARGE ((L4_HADM_MAX_LINEAR_SORT_FREQUENCIES_LARGE * L4_HADM_MAXANTENNAPATHS) >> 1)

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _L4_HADM_BASICS_H */

