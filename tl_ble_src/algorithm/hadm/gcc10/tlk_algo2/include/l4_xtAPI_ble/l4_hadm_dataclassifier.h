/**
 * Project:     Lambda:4 HADM Library
 *
 * @file        l4_hadm_dataclassifier.h
 * @brief       HADM Library Data Classifier data types and functions.
 *
 * Copyright 2024 Lambda:4 Entwicklungen GmbH, Germany.
 *
 * All rights reserved. Using, copying, publishing or distributing
 * is not permitted without prior written agreement.
 */

#ifndef _L4_HADM_DATACLASSIFIER_H
#define _L4_HADM_DATACLASSIFIER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief Lambda:4 Data classifier.
   *
   * In addition to the standardized Bluetooth Channel Sounding Interface,
   * the Lambda:4 cs:Lib can be optimized for MCU-specific features.<br>
   * Customization can be applied after data evaluation by Lambda:4.
   *
   * Full documentation can be found in @see l4_hadm_dataclassifier_doxygen.h.
   */
  typedef struct _tag_l4_hadm_in_dataclassifier
  {
    unsigned char symmetry_type : 2;
    unsigned char linearity_type : 4;
    unsigned char subevent_phase_constant : 1;
    unsigned char subevent_magnitude_constant : 1;
    unsigned char tqi_type : 3;
    unsigned char pllproblem : 2;
    unsigned char symmetry_a_type : 2;
    unsigned char reserved_4 : 1;
    int16_t       hw_offset_mm;
  } l4_hadm_dataclassifier_t;

#ifdef __cplusplus
}
#endif

#endif  // _L4_HADM_DATACLASSIFIER_H
