/**
 * Project:     Lambda:4 HADM Library
 *
 * @file        l4_hadm_xtAPI.h
 * @brief       HADM Library API data types and functions.
 *
 * Copyright 2022 Lambda:4 Entwicklungen GmbH, Germany.
 *
 * All rights reserved. Using, copying, publishing or distributing
 * is not permitted without prior written agreement.
 */

#ifndef _L4_HADM_xtAPI_H
#define _L4_HADM_xtAPI_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "l4_hadm_basics.h"
#include "l4_hadm_pctopts.h"

#include "l4_hadm_errors.h"

#include "l4_hadm_in_ble_api.h"
#include "../l4_xtAPI_ble/l4_hadm_dataclassifier.h"
#include "l4_hadm_xtAPI_result.h"

#include "../l4_xtAPI_ble/l4_xtAPI_ble.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Use versioning scheme like https://semver.org/
// Semantic Versioning 2.0.0
//  MAJOR version when you make incompatible API changes
//  MINOR version when you add functionality in a backwards compatible manner
//  PATCH version when you make backwards compatible bug fixes
// Define the Version of this API.
// Check against L4HADM_LIB_V_XXX in your user code for compatibility
// Use l4hadm_lib_get_version_string to get detailed information (git-hash/build-date).

/** helper macro to make string from version defines */
#define _V_Stringize_(a, b, c) #a "." #b "." #c
/** helper macro to make string from version defines */
#define _V_StringizeHelper_(a, b, c) _V_Stringize_(a, b, c)

/** Mayor library version number */
#define L4HADM_LIB_V_MAYOR 2
/** Minor library version number */
#define L4HADM_LIB_V_MINOR 2
/** Library patch version number */
#define L4HADM_LIB_V_PATCH 7
/** library version number string */
#define L4HADM_LIB_V_STRING _V_StringizeHelper_(L4HADM_LIB_V_MAYOR, L4HADM_LIB_V_MINOR, L4HADM_LIB_V_PATCH)

#define BUILD_UINT32_VERSION(may, min, patch) ((uint32_t)(may << 16) | (min << 8) | patch)
#define L4HADM_LIB_V_U32 BUILD_UINT32_VERSION(L4HADM_LIB_V_MAYOR, L4HADM_LIB_V_MINOR, L4HADM_LIB_V_PATCH)

#define BUILD_UINT16_VERSION(may, min) ((uint16_t)(may << 8) | min)
#define MAYOR_FROM_UINT16_VERSION(u) ((int)((u >> 8) & 0xFFU))
#define MINOR_FROM_UINT16_VERSION(u) ((int)(u & 0xFFU))

#ifndef L4_HADM_MAXANTENNAPATHS
/** maximum number of supported antenna paths by this library */
#define L4_HADM_MAXANTENNAPATHS 4
#endif

#define XTAPI_MAXSIZE_TEXTBUF_TODUMP (4000)

#define XTAPI_STATIC_ASSERT(COND, MSG) typedef char static_assertion_##MSG[(!!(COND)) * 2 - 1]
#define XTAPI_COMPILE_TIME_ASSERT3(X, L) XTAPI_STATIC_ASSERT(X, static_assertion_at_line_##L)
#define XTAPI_COMPILE_TIME_ASSERT2(X, L) XTAPI_COMPILE_TIME_ASSERT3(X, L)
#define XTAPI_COMPILE_TIME_ASSERT(X) XTAPI_COMPILE_TIME_ASSERT2(X, __LINE__)
  XTAPI_COMPILE_TIME_ASSERT(sizeof(bool) == 1);
  // Note: a doxygen configuration file can be found in [.../l4/doc/libapi]
  /**
   * @page instance_handling L4 HADM Library Instance handling
   *
   *  Example Usage  pseudo-code:
   *
   *  Create memory pointers and variables to store the memory requirements:
   *
   *  <pre>
   *    size_t _statusmemorysize,_runtimememorysize;
   *    l4hadm_xtAPI_instance_cfg_t cfg;
   *    uint8_t* _statusmemoryp;
   *    uint8_t*_runtimememoryp;
   * </pre>
   *
   * Fill the instance with default values:
   *
   *  <pre>
   *    l4_hadm_xtAPI_init_instance_cfg( &cfg); // get default config
   * </pre>
   *
   * Request the memory requirements. These depend on the instance configuration.
   * Allocate the memory required to use the library:
   *
   *  <pre>
   *    // calc need sizes
   *    l4_hadm_xtAPI_req_requiredsize_forinstance( &cfg, &_statusmemorysize, &_runtimememorysize);
   *
   *    _statusmemoryp = malloc(_statusmemorysize);
   *    _runtimememoryp = malloc(_runtimememorysize);
   * </pre>
   *
   * Open the instance, provide the memory pointers:
   *  <pre>
   *    l4hadm_xtAPI_instance_handle_t user_handle;
   *
   *    err = l4_hadm_xtAPI_open_instance( &cfg,
   * &user_handle,_statusmemoryp,_statusmemory_size,_runtimememoryp,_runtimememory_size);
   * </pre>
   */

  /**
   * @brief Data type defining the instance handle. Here as an opaque pointer.
   *
   * Early declaration of the handle type to be used in the API and also in other headers.
   */
  typedef void* l4hadm_xtAPI_instance_handle_t;

  /**
   * @brief Callback function type to dump a line of text.
   *
   */
  typedef void (*xtapi_dump_func)(const char* p, size_t len);

  /**
   * @brief Values that represent instance topologies
   */
  typedef enum _instance_topology
  {
    l4hadm_instance_topology_none             = 0,  ///< topology not set
    l4hadm_instance_topology_p2p              = 1,  ///< only one-to-one ranging
    l4hadm_instance_topology_ancho0rnet_2022C = 2,  ///< one-to-many ranging with PCFS
  } l4_instance_topology_t;

  /**
   * @brief Values that represent instance memory model
   */
  typedef enum _instance_memory_model
  {
    l4hadm_instance_memory_model_none   = 0,
    l4hadm_instance_memory_model_small  = 1,
    l4hadm_instance_memory_model_medium = 2,
    l4hadm_instance_memory_model_large  = 3,
  } l4_instance_memory_model_t;

#define L4HADM_MEMORY_MODEL_LAST l4hadm_instance_memory_model_large

  /**
   * @brief Configuration of the features of a single Lambda:4 HADM library instance.
   * The flags define the use cases, define the data field sizes
   * and thus the memory requirements.
   *
   * Use l4_hadm_xtAPI_init_instance_cfg(l4hadm_xtAPI_instance_cfg_t *cfg_p) to get a default setting.
   */
  typedef struct _l4hadm_xtAPI_instance_cfg
  {
    uint8_t instance_topology;           ///< defines the system topology.Used with l4_instance_topology_t enums.
    float   distfilter_typical_delay_s;  ///< typical delay caused by the distance filter.
    uint8_t pairs_tobehandled_count;     ///< max number of ranging partners
    uint8_t max_antennapaths_tobeused;   ///< max. number of antenna paths (not physical antennas) to be used.
    uint8_t max_frequencies_tobeused;    ///< max. number of frequencies allowed to be used.

    bool enable_level2memory;  ///< 0 = L2 memory not enabled; else enabled. L2 memory is used for max accuracy mode(s)

    bool enable_pcfs;                           ///< 0 = PCFS disabled; else enabled.
    bool enable_rtt;                            ///< 0 = RTT disabled; else enabled.
    bool enable_rtt_offsethandling;             ///< 0 = RTT offset calibration disabled; else enabled.
    bool enable_distfilter;                     ///< 0 = distance filter disabled; else enabled.
    bool reset_runtimememory_before_calculate;  ///< flag indicating to reset the memory before each HADM calculation.
    xtapi_dump_func            dump_func;       ///< callback function to dump a line of text, can be NULL.
    l4_instance_memory_model_t memory_model;    ///< to select different runtime memory requirements
  } l4hadm_xtAPI_instance_cfg_t;

  typedef enum _accuracy_level
  {
    l4hadm_algorithmlevel_none       = 0,
    l4hadm_algorithmlevel_rttonly    = 1,
    l4hadm_algorithmlevel_l4basic    = 2,
    l4hadm_algorithmlevel_cfft       = 3,
    l4hadm_algorithmlevel_l4cfft     = 4,
    l4hadm_algorithmlevel_l4cfftplus = 5,

    l4hadm_algorithmlevel_hires_superfast = 6,
    l4hadm_algorithmlevel_hires_fast      = 7,
    l4hadm_algorithmlevel_cap_normal      = 8,
    l4hadm_algorithmlevel_hires_normal    = 9,
    l4hadm_algorithmlevel_hires_highacc   = 10,  // only available if (L4_HADM_CFG_MAX_ACC_LEVEL > L4_HADM_CFG_NORMAL_ACC_LEVEL)
    l4hadm_algorithmlevel_level_last = 10  // last valid level
  } l4hadm_algorithmlevel_t;

#define L4HADM_ACCURACY_LEVEL_LAST l4hadm_algorithmlevel_level_last

  /**
   * @brief Params to control one measurement calculation.
   * Defines the configuration of a single calculation,
   * in contrast to _l4hadm_xtAPI_instance_cfg, which defines the features of a HADM instance.
   *
   * Some parameter depend on the instance configuration. If the related feature is disabled in the configuration,
   * the flags defined here are not used.
   */
  typedef struct _l4hadm_xtAPI_calculation_params
  {
    uint32_t config_options;  ///< bitfield to control the calculation, see l4hadm_lib_get_cfgnew_string
    /** level of algorithm requested, resulting level could be lower */
    l4hadm_algorithmlevel_t algorithmlevel_requested;

    /** 0 = RTT protection enabled, otherwise skipped. */
    bool skip_rtt_protection;

    /** 0 = if data is coherent and LIB supports this - use iut */
    /** 1 = to simulate non coherency or library without coherency support */
    bool switch_off_coherency_processing;

    uint8_t decrease_sensitivity;  ///< decrease_sensitivity of the calculation, 0 = no decrease, 1 = decrease, later we
                                   ///< might implement more levels
    uint8_t enable_fallback;       ///< 0 = no fallback, 1 = fallback to lower accuracy level

    uint8_t enable_data_preanalysis;
    uint8_t custom_setting1;
    uint8_t custom_setting2;
    uint8_t custom_setting3;
    uint8_t custom_setting4;

  } l4hadm_xtAPI_calculation_params_t;

  /**
   * @brief An information passed from one anchor to all others.
   * The information is coded in a string with max 63 byte of content + zerobyte
   */
  typedef char xinfo_string_t[64];

  /// @brief Return a Version-String identifiying the build and source.
  /// See also L4HADM_LIB_V_MAYOR,L4HADM_LIB_V_MINOR and L4HADM_LIB_V_PATCH
  /// @param
  /// @return a constant string with Version information
  extern const char* l4hadm_lib_get_version_string(void);

  /// @brief Return the  Version-Mayor int from the compiled lib
  /// See also L4HADM_LIB_V_MAYOR,L4HADM_LIB_V_MINOR and L4HADM_LIB_V_PATCH
  /// @return a constant int with Version information mayor number
  extern unsigned int l4hadm_lib_get_version_mayor(void);

  /// @brief Return the  Version-Minor int from the compiled lib
  /// See also L4HADM_LIB_V_MAYOR,L4HADM_LIB_V_MINOR and L4HADM_LIB_V_PATCH
  /// @return a constant int with Version information minor number
  extern unsigned int l4hadm_lib_get_version_minor(void);

  /// @brief Return the  Version-Patch int from the compiled lib
  /// See also L4HADM_LIB_V_MAYOR,L4HADM_LIB_V_MINOR and L4HADM_LIB_V_PATCH
  /// @return a constant int with Version information patch number
  extern unsigned int l4hadm_lib_get_version_patch(void);

  /// @brief Return a Config-String identifiying the config option at build time.
  /// @param
  /// @return a constant string with config information
  extern const char* l4hadm_lib_get_config_string(void);

  /// @brief Returns a hash of the relevant git commit of the library as a string
  /// @param
  /// @return relevant git commit hash string of the library.
  extern const char* l4hadm_lib_get_git_commit(void);

  /// @brief Returns the build date of the library as a string.
  /// @param
  /// @return library build date as a string
  extern const char* l4hadm_lib_get_build_date(void);

  /// @brief Returns a hash of the currently checked-out commit of the git-repository "l4hadm_root" as a string
  /// @param
  /// @return git commit hash string of the l4hadm_root-repository.
  extern const char* l4hadm_root_get_git_commit(void);

  /// @brief Returns the date of the currently checked-out commit of the git-repository "l4hadm_root" as a string.
  /// @param
  /// @return root-repository commit-date as a string
  extern const char* l4hadm_root_get_commit_date(void);

  //-------------------------------------------------------------------------------
  // Library initialization.
  //-------------------------------------------------------------------------------

  /**
   * @fn extern void l4_hadm_xtAPI_init_instance_cfg(l4hadm_xtAPI_instance_cfg_t *cfg_p);
   *
   * @brief 4 hadm xt api initialize cfg struct with defaults:
   * <pre>
   * xtAPI_cfg.instance_topology                    = l4hadm_instance_topology_p2p;

   * xtAPI_cfg.pairs_tobehandled_count              = 6;
   * xtAPI_cfg.enable_rtt                           = 1;
   * xtAPI_cfg.enable_rtt_offsethandling            = 1;
   * xtAPI_cfg.enable_distfilter                    = 1;
   * xtAPI_cfg.distfilter_typical_delay_s           = 0.3F;
   * xtAPI_cfg.max_antennapaths_tobeused            = L4_HADM_MAXANTENNAPATHS;
   * xtAPI_cfg.max_frequencies_tobeused             = _L4_HADM_MAXFREQUENCIES;
   * xtAPI_cfg.reset_runtimememory_before_calculate = 0;
   * </pre>
   *
   * @param [in,out]    cfg_p If non-null, the configuration p.
   * @return            L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_init_instance_cfg(l4hadm_xtAPI_instance_cfg_t* cfg_p);

  /**
   * @fn extern void l4_hadm_xtAPI_init_calculation_params(l4hadm_xtAPI_calculation_params_t* param_p);
   *
   * @brief 4 hadm xt a pi initialize calculation parameters
   *
   * @param param_p         A variable-length parameters list containing parameter p.
   * @return                L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_init_calculation_params(l4hadm_xtAPI_calculation_params_t* param_p);

  /**
   * @brief Calculate for a specific HADM instance the required memory size.
   * The feature setup is passed by struct _l4hadm_xtAPI_instance_cfg*,
   * so the libray is able to calculate memory required for this instance.
   *
   * @param _cfgp               HADM feature configuration.
   * @param _statusmemorysizep  result with the required number of bytes for the status memory.
   * @param _runtimememorysizep result with the required number of bytes for the runtime memory.
   * @return                    L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_req_requiredsize_forinstance(const l4hadm_xtAPI_instance_cfg_t* _cfgp,
                                                                    size_t* _statusmemorysizep,
                                                                    size_t* _runtimememorysizep);

  /**
   * @brief Opens a HADM instance and prepare it for usage by the caller.
   * - checks struct _l4hadm_xtAPI_instance_cfg with provided memory
   * - fills struct _l4hadm_xtAPI_instance_handle with all information
   * - does a reset.
   *
   * @param _cfgp               HADM feature configuration.
   * @param _instance_handlep   the HADM library instance handle to be used.
   * @param _statusmemoryp      Pointer at the status memory.
   * @param _statusmemory_size  Size of the runtime memory (number of bytes).
   * @param _runtimememoryp     Pointer at the runtime memory.
   * @param _runtimememory_size Size of the runtime memory (number of bytes).
   * @return                    L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   *
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_open_instance(const struct _l4hadm_xtAPI_instance_cfg* _cfgp,
                                                     l4hadm_xtAPI_instance_handle_t*          _instance_handlep,
                                                     void* _statusmemoryp, size_t _statusmemory_size,
                                                     void* _runtimememoryp, size_t _runtimememory_size);

  /**
   * @brief Close a HADM instance and release all internal resources.
   *
   * Memory allocated by the caller and passed by _l4hadm_xtAPI_instance_handle* must be freed by the caller.
   * To use the instance again, l4_hadm_xtAPI_open_instance() must be called.
   *
   *
   * @param _instance_handle   the HADM library instance handle to be used.
   * @return                    L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_close_instance(l4hadm_xtAPI_instance_handle_t _instance_handle);

  /**
   * @brief Reset a HADM instance.
   *
   * Reset all internal data to defaults. This function is called by l4_hadm_xtAPI_open_instance().
   *
   * @param _instance_handle    the HADM library instance handle to be used.
   * @return                    L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_reset_instance(l4hadm_xtAPI_instance_handle_t _instance_handle);

  //---------------------------------------------
  //  Bluetooth CS support
  //---------------------------------------------
  /**
   * @brief BLE compatible input for distance measurment calculation
   * see include\hadm_hi\l4_hadm_in_ble_api.h for details of required BLE CS input.
   */
//  extern l4_hadm_error_t l4_hadm_xtAPI_hadm_ble_calculate(const l4hadm_xtAPI_instance_handle_t _instance_handle,
//                                                          l4hadm_xtAPI_calculation_params_t*   _paramsp,
//                                                          const l4_procedureinput_data_t*      _cs_proc_data_p,
//                                                          l4hadm_xtAPI_result_t*               _hadm_xtResultp);
  /**
   * @brief BLE compatible input for distance measurement calculation
   *
   * @param _instance_handle            the HADM library instance handle to be used.
   * @param _paramsp                    calculation parameters.
   * @param _cs_proc_data_p             channel sounding procedure data.
   * @param _hadm_xtResultp             distance result.
   * @return                            L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_ble_calculate(const l4hadm_xtAPI_instance_handle_t _instance_handle,
                                                     l4hadm_xtAPI_calculation_params_t*   _paramsp,
                                                     const l4_csProcedure_t*              _cs_proc_data_p,
                                                     l4hadm_xtAPI_result_t*               _hadm_xtResultp);
  //
  //-------------------------------------------------------------------------------
  // PCFS support.
  //-------------------------------------------------------------------------------

  /**
   * @brief resets only the PCFS system (if this is enabled in struct _l4hadm_xtAPI_instance_cfg)
   * mainly needed by simulations / slotview
   *
   * @param _instance_handle    the HADM library instance handle to be used.
   * @return                    L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_PCFS_reset(const l4hadm_xtAPI_instance_handle_t _instance_handle);

  /**
   * @brief Reads the calibration data. Intended to be used for simulations.
   *
   * @param _instance_handle            the HADM library instance handle to be used.
   * @param _local_mac_h                MAC of the local host, used to identify the initiator/reflector pair.
   * @param _local_mac_l                MAC of the local host, used to identify the initiator/reflector pair.
   * @param _remote_mac_h               MAC of the remote, used to identify the initiator/reflector pair.
   * @param _remote_mac_l               MAC of the remote, used to identify the initiator/reflector pair.
   * @param cal_memory_pp               Pointer at the calibration data memory.
   * @param cal_memory_size_p           Size of the calibration data memory.
   * @param cal_version_p               Version for compatibility checks.
   * @return                            L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_PCFS_read_calibration(
      const l4hadm_xtAPI_instance_handle_t _instance_handle, uint32_t _local_mac_h, uint32_t _local_mac_l,
      uint32_t _remote_mac_h, uint32_t _remote_mac_l, void** cal_memory_pp,
      uint16_t* cal_memory_size_p,  // CBD TODO: pointer typedef for cal_memory?
      uint16_t* cal_version_p);

  /**
   * @brief Presets the calibration data with previously stored data . Intended to be used for simulations.
   *
   * @param _instance_handle            the HADM library instance handle to be used.
   * @param _local_mac_h                MAC of the local host, used to identify the initiator/reflector pair.
   * @param _local_mac_l                MAC of the local host, used to identify the initiator/reflector pair.
   * @param _remote_mac_h               MAC of the remote, used to identify the initiator/reflector pair.
   * @param _remote_mac_l               MAC of the remote, used to identify the initiator/reflector pair.
   * @param cal_memory_p                Memory with calibration data.
   * @param cal_memory_size             Size of the calibration data memory.
   * @param cal_version                 Version for compatibility checks.
   * @return                            L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_PCFS_write_calibration(const l4hadm_xtAPI_instance_handle_t _instance_handle,
                                                              uint32_t _local_mac_h, uint32_t _local_mac_l,
                                                              uint32_t _remote_mac_h, uint32_t _remote_mac_l,
                                                              void* cal_memory_p, uint16_t cal_memory_size,
                                                              uint16_t cal_version);

  //-------------------------------------------------------------------------------
  // RTT support.
  //-------------------------------------------------------------------------------

  /**
   * @brief Reads a RTT offset correction for a initiator/reflector pair from the flash file system.
   *
   * @param _instance_handle            the HADM library instance handle to be used.
   * @param _local_mac_h                MAC of the local host, used to identify the initiator/reflector pair.
   * @param _local_mac_l                MAC of the local host, used to identify the initiator/reflector pair.
   * @param _remote_mac_h               MAC of the remote, used to identify the initiator/reflector pair.
   * @param _remote_mac_l               MAC of the remote, used to identify the initiator/reflector pair.
   * @param _dist_deviation_p           RTT distance offset [m] to be added to the RTT result.
   * @return                            L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_read_rtt_distdeviation(const l4hadm_xtAPI_instance_handle_t _instance_handle,
                                                              uint32_t _local_mac_h, uint32_t _local_mac_l,
                                                              uint32_t _remote_mac_h, uint32_t _remote_mac_l,
                                                              float* _dist_deviation_p);

  /**
   * @brief Writes a new RTT offset correction for a initiator/reflector pair into the flash file system.
   *
   * @param _instance_handle            the HADM library instance handle to be used.
   * @param _local_mac_h                MAC of the local host, used to identify the initiator/reflector pair.
   * @param _local_mac_l                MAC of the local host, used to identify the initiator/reflector pair.
   * @param _remote_mac_h               MAC of the remote, used to identify the initiator/reflector pair.
   * @param _remote_mac_l               MAC of the remote, used to identify the initiator/reflector pair.
   * @param _dist_deviation             RTT distance offset [m] to be added to the RTT result.
   * @return                            L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_write_rtt_distdeviation(const l4hadm_xtAPI_instance_handle_t _instance_handle,
                                                               uint32_t _local_mac_h, uint32_t _local_mac_l,
                                                               uint32_t _remote_mac_h, uint32_t _remote_mac_l,
                                                               float _dist_deviation);

  /**
   * @brief Erases the RTT offset correction for a initiator/reflector pair.
   *
   * @param _instance_handle            the HADM library instance handle to be used.
   * @param _local_mac_h                MAC of the local host, used to identify the initiator/reflector pair.
   * @param _local_mac_l                MAC of the local host, used to identify the initiator/reflector pair.
   * @param _remote_mac_h               MAC of the remote, used to identify the initiator/reflector pair.
   * @param _remote_mac_l               MAC of the remote, used to identify the initiator/reflector pair.
   * @return                            L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_reset_rtt_distdeviation(const l4hadm_xtAPI_instance_handle_t _instance_handle,
                                                               uint32_t _local_mac_h, uint32_t _local_mac_l,
                                                               uint32_t _remote_mac_h, uint32_t _remote_mac_l);

  //-------------------------------------------------------------------------------
  // Distfilter support.
  //-------------------------------------------------------------------------------

  /**
   * @brief Resets the distance filter, if it was enabled in _l4hadm_xtAPI_instance_cfg.
   *
   * @param _instance_handle            the HADM library instance handle to be used.
   * @return                            L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_distfilter_reset(const l4hadm_xtAPI_instance_handle_t _instance_handle);

  /**
   * @brief After a hadm calculation is finished, this function feed the last result (or no result) into the distance
   * filter.
   *
   * @param _instance_handle            the HADM library instance handle to be used.
   * @param _hadm_xtResultp             filtered distance result.
   * @return                            L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_xtAPI_distfilter_calculate(const l4hadm_xtAPI_instance_handle_t _instance_handle,
                                                            l4hadm_xtAPI_result_t*               _hadm_xtResultp);

  /**
   *  @brief Just updates the current time to the filter and produce updated estimation for distance.
   *
   * @param _instance_handle            the HADM library instance handle to be used.
   * @param newcurrent_time_us          new time [us].
   * @param _hadm_xtResultp             filtered distance result.
   * @return                            L4_HADM_HIRES180_OK_1=1 in case of success, otherwise an error code.
   */
  extern l4_hadm_error_t l4_hadm_API_distfilter_updatetime(const l4hadm_xtAPI_instance_handle_t _instance_handle,
                                                           unsigned long                        newcurrent_time_us,
                                                           l4hadm_xtAPI_result_t*               _hadm_xtResultp);

  //-------------------------------------------------------------------------------
  // Result and Information support.
  //-------------------------------------------------------------------------------

  /**
   * @brief Dumps the result of a HADM calculation into a 6-bit "HDR" string(if dump_func is on xtAPI_open was set).
   *
   * @param src                        the source.
   */
  extern l4_hadm_error_t l4hadm_xtAPI_result_dump(l4hadm_xtAPI_result_t* src, xtapi_dump_func dump_func);

  /// @brief Return a String for the algorithm_level requested
  /// @param
  /// @return a constant string with config information
  extern const char* l4hadm_lib_get_algorithmlevel_string(l4hadm_algorithmlevel_t algorithm_level);

  /// @brief Return a String for the MEMORY MODEL requested
  /// @param
  /// @return a constant string with config information
  extern const char* l4hadm_lib_get_memorymodel_string(l4_instance_memory_model_t memory_model);

  /// @brief Return a String identifiying the current use of cfg_newX flags
  /// @param
  /// @return a constant string with config information
  extern const char* l4hadm_lib_get_cfgnew_string(int cfgnew_num);

  extern void l4hadm_lib_dump_config(const l4hadm_xtAPI_instance_handle_t _instance_handle);

//  extern void dump_proc_input(const l4_procedureinput_data_t* _cs_proc_data_p, xtapi_dump_func dump_func);
//
//  /// @brief Convert input BLE data into ssmt output data
//  /// @param _ble_in    Pointer to BLE data structure (l4_procedureinput_data_t)
//  /// @param mem_p      Pointer to RAM used for temporary storage of SSMT data
//  /// @param mem_size   Size of used RAM
//  /// @param dump_func  Pointer on callback function from type xtapi_dump_func
//  /// @return
//  extern l4_hadm_error_t l4hadm_lib_ble_to_ssmt_dump(l4_procedureinput_data_t* _ble_in, uint32_t* mem_p,
//                                                     size_t mem_size, xtapi_dump_func dump_func);
//
//  /// @brief Calculate the size of required memory for ssmt output
//  /// @param _ble_in    Pointer to BLE data structure (l4_procedureinput_data_t)
//  /// @return           memory size in bytes
//  extern uint32_t l4hadm_lib_calc_req_mem_size(l4_procedureinput_data_t* _ble_in);

#ifdef __cplusplus
}
#endif

#endif  // _L4_HADM_xtAPI_H
