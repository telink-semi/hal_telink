/**
**********************************************************************************************************************************************************************************************************************************
* @file: l4_pct_opts.h
* @author:
* @date:    2024/04/10
* @brief:
* Copyright 2023 Lambda:4 Entwicklungen GmbH, Germany.
*
* All rights reserved. Using, copying, publishing or distributing
* is not permitted without prior written agreement.
**********************************************************************************************************************************************************************************************************************************
**/

#ifndef __L4_PCT_OPTS_H__
#define __L4_PCT_OPTS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus

  /// @brief Set configuration for modifying the remote input data
  typedef struct _tag_pct_options
  {
    uint8_t bitsize : 4;       ///< 0=12bits, 3=3bits, 4=5bits, ..., 15=15bits
    // in future use 1 = inline phase return

    uint8_t way_1 : 1;         ///< 0==no way-1 , 1== way-1 active
    uint8_t no_amplitude : 1;  ///< 0==amplitude is present, 1==no amplitude
    uint8_t no_tqi_bits : 1;   ///< 0==tqi bits are present, 1==no tqi bits
  } l4hadm_pct_options_t;

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // __L4_PCT_OPTS_H__
