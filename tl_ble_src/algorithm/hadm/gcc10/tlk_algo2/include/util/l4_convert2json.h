/**
 * Project:     Lambda:4 CS library.
 *
 * @file        l4_convert2json.h
 * @brief       Formats xtAPI results as JSON.
 *
 * Copyright 2023 Lambda:4 Entwicklungen GmbH, Germany.
 *
 * All rights reserved. Using, copying, publishing or distributing
 * is not permitted without prior written agreement.
 */
#ifndef L4_CONVERT2JSON_H_
#define L4_CONVERT2JSON_H_

#include <stdint.h>
#include "../hadm_hi/l4_hadm_xtAPI.h"

#ifdef __cplusplus

extern "C"
{
#endif

  /**
   * Converts a xtAPI result into a JSON struct and dumps it to an output device by use of a linedump_func.
   *
   * @param res_p         pointer at the xtAPI result.
   * @param timestamp216  Associated time stamp in 1/2^16 ms.
   * @param func          Function to dump the output, <code>void (*linedump_func)(const char*, size_t);</code>
   */
  typedef void (*linedump_func)(const char* p, size_t len);
  void l4_convert2json(const l4hadm_xtAPI_result_t* const res_p, uint32_t timestamp216, linedump_func func);
  void l4_convert2shortjson(const l4hadm_xtAPI_result_t* const res_p, uint32_t timestamp216, linedump_func func);

#ifdef __cplusplus
}
#endif

#endif /* L4_CONVERT2JSON_H_ */
