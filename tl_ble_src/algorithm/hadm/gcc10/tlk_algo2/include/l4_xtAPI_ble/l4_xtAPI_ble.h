/**
 *  @file l4_xtAPI_ble.h
 *
 *  @brief The Lambda4 Channel Sounding BLE API
 *
 * Copyright 2023 Lambda:4 Entwicklungen GmbH, Germany.
 *
 * All rights reserved. Using, copying, publishing or distributing
 * is not permitted without prior written agreement.
 */
#ifndef __L4_XTAPI_BLE_H__
#define __L4_XTAPI_BLE_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../hadm_hi/l4_hadm_errors.h"
#include "l4_hadm_dataclassifier.h"

#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus

#define L4API_CS_MIN_CHANNEL 0
#define L4API_CS_MAX_CHANNEL 79
  /* Maximum number of CS channels usable in Band 2 (2402-2480 MHz) */
#define L4API_MAX_NUM_CHANNELS 72
#define L4API_MIN_CHANNEL 0
#define L4API_MAX_CHANNEL 78

#define L4API_MAX_CONFIG_ID 3
#define L4API_MAX_STATE_PROC 1

#define L4API_MIN_SELECTED_TX_POWER (-127)
#define L4API_MAX_SELECTED_TX_POWER 20
#define L4API_NOT_AVAILABLE_SELECTED_TX_POWER 0x7F
#define L4API_MIN_REFERENCE_POWER_LEVEL (-127)
#define L4API_MAX_REFERENCE_POWER_LEVEL 20
#define L4API_NOT_APPLICABLE_REFERENCE_POWER_LEVEL 0x7F
#define L4API_MIN_FREQUENCY_COMPENSATION (-10000)
#define L4API_MAX_FREQUENCY_COMPENSATION 10000
#define L4API_NOT_AVAILABLE_FREQUENCY_COMPENSATION 0xC000

#define L4API_MASK_PROCEDURE_DONE_STATUS (0x0F)
#define L4API_MASK_SUBEVENT_DONE_STATUS (0x0F)
#define L4API_MASK_ABORT_REASON_LOW (0x0F)
#define L4API_MASK_ABORT_REASON_HIGH (0xF0)

#define L4API_MAX_TONE_ANTENNA_CONFIG_SELECTION 7
#define L4API_MIN_SUBEVENT_LEN 1250
#define L4API_MAX_SUBEVENT_LEN 4000000
#define L4API_MIN_MAIN_MODE_TYPE 1
#define L4API_MAX_MAIN_MODE_TYPE 3
#define L4API_MIN_SUBMODE_TYPE 1
#define L4API_MAX_SUBMODE_TYPE 3
#define L4API_MIN_MAIN_MODE_MIN_STEPS 1
#define L4API_MAX_MAIN_MODE_MIN_STEPS 0xFF
#define L4API_MIN_MAIN_MODE_MAX_STEPS 1
#define L4API_MAX_MAIN_MODE_MAX_STEPS 0xFF
#define L4API_MAX_MAIN_MODE_REPETITION 3
#define L4API_MAX_STEPDATA_BYTES (24)
#define L4API_MIN_MODE0_STEPS 1
#define L4API_MAX_MODE0_STEPS 3
#define L4API_MAX_ROLE_IDX 1
#define L4API_MAX_RTT_TYPES 6
#define L4API_MIN_PHY_IDX 1
#define L4API_MAX_PHY_IDX 2

#define L4API_MAX_TIP1_IDX 7
#define L4API_MAX_TIP2_IDX 7
#define L4API_MAX_TFCS_IDX 9
#define L4API_MAX_TPM_IDX 3

#define L4API_MIN_NUM_ANTPATH 1
#define L4API_MAX_NUM_ANTPATH 4
#define L4API_MIN_NUM_STEPS 1
#define L4API_MAX_NUM_STEPS 160
#define L4API_MIN_NUM_SUBEVENTS 1
#define L4API_MAX_NUM_SUBEVENTS 32

#define L4API_MAX_API_FOR_ANTPATH_1 0
#define L4API_MAX_API_FOR_ANTPATH_2 1
#define L4API_MAX_API_FOR_ANTPATH_3 5
#define L4API_MAX_API_FOR_ANTPATH_4 23

#define L4API_MAX_PROC_STEPS 255

#define L4API_CH_MAP_LEN 10

  typedef uint8_t l4_ble_deviceId_t;  // local definition for BLE device id.

  /**
   * @brief Format types
   */
  typedef enum l4_format_type_tag
  {
    l4_ssmt_6bit = 0,
    l4_json_input,
    l4_json,
  } l4_format_type_t;

  /**
   * @brief Channel sounding role type
   */
  typedef enum l4_role_tag
  {
    l4_csRoleInitiator = 0,
    l4_csRoleReflector,
  } l4_role_t;

  /**
   * @brief Channel sounding location role type
   */
  typedef enum l4_LocRole_tag
  {
    l4_csRoleLocal = 0,
    l4_csRoleRemote,
  } l4_LocRole_t;

  /**
   * @brief Parameter needed to calculate memory consumption of l4_xtAPI_BleCsHci_convert
   */
  typedef struct l4_csMemCalc_tag
  {
    uint8_t          Number_Of_Antenna_Paths; /**< Number of antenna paths used by CS [1 to 4] */
    uint16_t         Number_Of_Mode0_Steps;   /**< Number of mode0 steps in CS procedure */
    uint16_t         Number_Of_Mode1_Steps;   /**< Number of mode1 steps in CS procedure */
    uint16_t         Number_Of_Mode2_Steps;   /**< Number of mode2 steps in CS procedure */
    uint16_t         Number_Of_Mode3_Steps;   /**< Number of mode3 steps in CS procedure */
    uint8_t          Number_Of_Subevents;     /**< Number of Xsubevents in CS procedure */
    l4_format_type_t format;                  /**< Format type */
  } l4_csMemCalc_t;

  /**
   * @brief Time stamp
   */
  typedef struct l4_csTimeStamp_tag
  {
    uint16_t year;  /**< Year of date. Should be larger than 1970 */
    uint8_t  month; /**< Month of date. [1 to 12] */
    uint8_t  day;   /**< Day of date. [1 to 31] */
    uint8_t  hour;  /**< Hour [0 to 23] */
    uint8_t  min;   /**< Minutes [0 to 59] */
    uint8_t  sec;   /**< Seconds [0 to 59] */
    uint32_t usec;  /**< Microsecends [0 to 999999] */
  } l4_csTimeStamp_t;

  /**
   * @brief Data structure according to HCI_LE_CS_Config_Complete event
   */
  typedef struct l4_csConfigCompleteEvent_tag
  {
    uint8_t Main_Mode_Type;       /**< Mode1 [0x01], Mode2 [0x02], Mode3 [0x03] */
    uint8_t Sub_Mode_Type;        /**< Mode1 [0x01], Mode2 [0x02], Mode3 [0x03], unused [0xFF]*/
    uint8_t Min_Main_Mode_Steps;  /**< Minimum number of CS main mode steps to be executed before a sub mode step
                                     [0x01 to 0xFF] */
    uint8_t Max_Main_Mode_Steps;  /**< Maximum number of CS main mode steps to be executed before a sub mode step
                                     [0x01 to 0xFF] */
    uint8_t Main_Mode_Repetition; /**< Number of main mode steps taken from the end of the last CS subevent to be
                                   repeated at the beginning of the current CS subevent directly after the last mode 0
                                   step of that event [0 to 3] */
    uint8_t Mode_0_Steps; /**< Number of CS mode 0 steps to be included at the beginning of each CS subevent [1 to 3] */
    uint8_t Role;         /**< initiator [0], reflector [1]*/
    uint8_t RTT_Type;     /**< RTT AA Only [0], RTT with 32-bit sounding sequence [1], RTT with 96-bit sounding sequence
                             [2],     RTT with 32-bit random sequence [3], RTT with 64-bit random sequence [4], RTT with
                             96-bit     random     sequence [5], RTT with 128-bit random sequence [6] */
    uint8_t CS_SYNC_PHY;  /**< LE 1M PHY [1], LE 2M PHY [2]*/
    uint8_t Channel_Map[L4API_CH_MAP_LEN]; /**< This parameter contains 80 1-bit fields.The nth such field (in the range
                                0 to 78)
                                contains the value for the CS channel index n. Channel n is enabled for CS procedure
                                [1]. Channel n is disabled for CS procedure [0]. Channels n = 0, 1, 23, 24, 25, 77,
                                and 78. shall be ignored and shall be set to zero. At least 15 channels shall be
                                enabled. The most significant bit (bit 79) is reserved. */
    uint8_t Channel_Map_Repetition; /**< The number of times the Channel_Map field will be cycled through for non-mode 0
                                       steps within a CS procedure [0x01 to 0xFF] */
    uint8_t Channel_Selection_Type; /**< Use Channel Selection Algorithm #3b for non-mode 0 CS steps [0x00], Use
                                       Channel Selection Algorithm #3c for non-mode 0 CS steps [0x01]*/
    uint8_t Ch3c_Shape;             /**< Use Hat shape for user-specified channel sequence [0x00], Use X shape for
                                       user-specified channel sequence [0x01]*/
    uint8_t Ch3c_Jump; /**< Number of channels skipped in each rising and falling sequence [0x02 to 0x08] */
    uint8_t Companion_Signal_Enable; /**< Companion Signal disabled [0x00], Companion Signal enabled [0x01]*/
    uint8_t T_IP1_Time; /**< Interlude time in microseconds between the RTT packets [10, 20, 30, 40, 50, 60, 80, 145] */
    uint8_t T_IP2_Time; /**< Interlude time in microseconds between the CS tones [10, 20, 30, 40, 50, 60, 80, 145] */
    uint8_t T_FCS_Time; /**< Time in microseconds for frequency changes [10, 20, 30, 40, 50, 60, 80, 145, 120, 150] */
    uint8_t T_PM_Time;  /**< Time in microseconds for the phase measurement period of the CS tones [10, 20, 40 ] */
  } l4_csConfigCompleteEvent_t;

  /**
   * @brief Data structure according to HCI_LE_CS_Procedure_Enable_Complete event
   */
  typedef struct l4_csProcedureEnableCompleteEvent_tag
  {
    uint8_t Tone_Antenna_Config_Selection; /**< Antenna Configuration Index [0 to 7] */
    int8_t  Selected_Tx_Power; /**< Transmit power level used for CS procedure [-127 to 20]dBm, Transmit power level
                                 is unavailable [0x7F]*/
    uint32_t Subevent_Len;     /**< Duration for each CS subevent in microseconds [1250μs to 4s] */
    uint8_t
        Subevents_Per_Event; /**< Number of CS Xsubevents anchored off the same ACL connection event [0x01 to 0x20] */
    uint16_t Subevent_Interval;  /**< Time between consecutive CS Xsubevents anchored off the same ACL connection event.
                                  Units: 0.625 ms */
    uint16_t Event_Interval;     /**< Number of ACL connection events between consecutive CS event anchor points */
    uint16_t Procedure_Interval; /**< Number of ACL connection events between consecutive CS procedure anchor points */
    uint16_t Procedure_Count;    /**< CS procedures to continue until disabled [0x00], [0x1 to 0xFFFF]=Number of CS
                                   procedures to be scheduled */
  } l4_csProcedureEnableCompleteEvent_t;

  /**
   * @brief Data structure according to HCI_LE_CS_Subevent_Result event
   */
  typedef struct l4_csSubeventResultEvent_tag
  {
    int16_t Frequency_Compensation; /**< Frequency compensation value in units of 0.01 ppm (15-bit signed integer).
                                      Range: -100 ppm (-0xD8F0) to +100 ppm (+0x2710). 0xC000=Frequency compensation
                                      value is not available, or the role is not initiator */
    int8_t Reference_Power_Level;   /**< Reference power level [-127 to 20]dBm, Reference power level is not
                                       applicable [0x7F] */
    uint8_t
        Num_Antenna_Paths;      /**< Ignored because phase measurement does not occur during the CS step [0],
                                   Number of antenna paths used during the phase measurement stage of the CS step [1 to 4] */
    uint8_t Num_Steps_Reported; /**< Number of steps in the CS subevent for which results are reported [1 to 160] */
  } l4_csSubeventResultEvent_t;

  /**
   * @brief Procedure miscellanoeus data object
   */
  typedef struct l4_csMisc_tag
  {
    l4_csTimeStamp_t         Timestamp;                 /**< Time of the measurement */
    uint32_t                 procedure_counter;         /**< HADM usually from l4_hciLeHadmEventResultEvent_t */
    uint32_t                 initiator_id;              /**< unique id of local partner */
    uint32_t                 reflector_id;              /**< unique id of remote partner */
    uint32_t                 timestamp_216;             /**< timestamp of measurement from from 2^16 Hz timer */
    l4_hadm_dataclassifier_t local_data_classifier;     /**< Data classifier for local device */
    l4_hadm_dataclassifier_t remote_data_classifier;    /**< Data classifier for remote device */
    int16_t                  local_board_temp_celsius;  /**< unit 0.1 degrees */
    int16_t                  remote_board_temp_celsius; /**< unit 0.1 degrees */
    bool                     pcfs_remotedata_2wr_valid; /**< iq2 contains valid 2 way ranging data  */
    bool                     pcfs_local2_for_ppb_valid; /**< iq2 contains valid sniffed ranging data */
    uint8_t                  locRole;                   /**< Location role 0 = local device, 1 = remote device */
    uint8_t                  t_sw;                      /**< Switching time period in microseconds [0, 1, 2, 4, 10] */
    uint8_t pct_reduction_config_id; /**< PCT reduction configuration identifier [1 to 8], 0: no pct reduction */
    uint8_t pct_reduction_iq_size;   /**< PCT reduction IQ value size [1 to 16] bit. */
    int8_t  local_FAE[L4API_MAX_NUM_CHANNELS];  /**< Frequency actuation errors, in units of 1/32 ppm.*/
    int8_t  remote_FAE[L4API_MAX_NUM_CHANNELS]; /**< Frequency actuation errors, in units of 1/32 ppm.*/

  } l4_csMisc_t;

  /**
   * @brief L4 channel sounding procedure object
   */
  typedef struct l4_csProcedure_tag
  {
    l4_csMisc_t*                         Misc;                    /**< Pointer to miscellaneous data object. */
    l4_csConfigCompleteEvent_t*          ConfigComplete;          /**< Pointer to ConfigCompleteEvent. */
    l4_csProcedureEnableCompleteEvent_t* ProcedureEnableComplete; /**< Pointer to ProcedureEnableCompleteEvent. */
    l4_csSubeventResultEvent_t* SubeventResultLocal[L4API_MAX_NUM_SUBEVENTS];  /**< Pointer to subevent meta data for
                                                                               each
                                                                                subevent. If not used set to NULL */
    l4_csSubeventResultEvent_t* SubeventResultRemote[L4API_MAX_NUM_SUBEVENTS]; /**< Pointer to subevent meta data for
                                                                               each
                                                                               subevent. If not used set to NULL */
    uint8_t* StepDataLocal[L4API_MAX_NUM_SUBEVENTS];  /**< Pointer to encoded step data for each subevent, can't be
                                    structured. If not used
                                     set to NULL */
    uint8_t* StepDataRemote[L4API_MAX_NUM_SUBEVENTS]; /**< Pointer to encoded step data for each subevent, can't be
                                    structured. If not used
                                    set to NULL */
  } l4_csProcedure_t;

  /**
   * @brief Callback function type to dump a line of text.
   *
   */
  typedef void (*xtapi_ble_dump_func)(const char* p, size_t len);

  /**
   * @brief Convert BLE Channel Sounding data into another format
   * @param ble         Pointer to BLE CS data
   * @param mem         Pointer to RAM used for temporary storage
   * @param mem_size    Size of used RAM
   * @param dump_func   Pointer to callback function
   * @param format_type Format into which the input data is converted
   * @return            Error code. If success the error code is 1. If fails the error code < 0
   */
  l4_hadm_error_t l4_xtAPI_ble_convert(l4_csProcedure_t* ble, uint32_t* mem, size_t mem_size,
                                       xtapi_ble_dump_func dump_func, l4_format_type_t format);
  /**
   * @brief             Calculates the memory space needed by l4_xtAPI_ble_convert
   * @param ParMemCalc  Pointer to parameters needed to calculate memory consumption
   * @return            Memory consumption in bytes, < 0 if error
   */
  int32_t l4_xtAPI_ble_calc_memory_consumption(l4_csMemCalc_t* param);

  /**
   * @brief             Checks the BLE CS data according to BLE standard compliance
   * @param ble         Pointer to BLE CS data
   * @return            Error code. If success the error code is 1. If fails the error code < 0
   */
//  l4_hadm_error_t l4_xtAPI_ble_check(l4_csProcedure_t* ble);
  
  /**
   * @brief             Creates a header file with ble memory content
   * @param ble         Pointer to BLE CS data
   * @param filename    File name
   * @return            0 if successful
   */
  int l4_xtAPI_ble_fprintf(l4_csProcedure_t* ble, const char* filename);

  /**
   * @brief             Prints header file with ble memory content
   * @param ble         Pointer to BLE CS data
   * @param filename    File name
   * @return            0 if successful
   */
  int l4_xtAPI_ble_printf(l4_csProcedure_t* ble, const char* filename);

  /**
   * @brief             Creates a header file with ble memory content
   * @param ble         Pointer to BLE CS data
   * @param filename    File name
   * @param prefix      Prefix for header file members
   * @return            0 if successful
   */
//  int l4_xtAPI_ble_fprintf_prefix(l4_csProcedure_t* ble, const char* filename, const char* prefix);

  /**
   * @brief             Prints header file with ble memory content
   * @param ble         Pointer to BLE CS data
   * @param filename    File name
   * @param prefix      Prefix for header file members
   * @return            0 if successful
   */
//  int l4_xtAPI_ble_printf_prefix(l4_csProcedure_t* ble, const char* filename, const char* prefix);

  /**
   * @brief             Provide the number of antenna paths in given procedure
   * @param ble         Pointer to BLE CS data
   * @return            Number of antenna paths
   */
  uint8_t l4_xtAPI_ble_get_num_antenna_path(l4_csProcedure_t* ble);

  /**
   * @brief             Provide the number of subevents in given procedure
   * @param ble         Pointer to BLE CS data
   * @return            Number of subevents
   */
  uint8_t l4_xtAPI_ble_get_num_subevents(l4_csProcedure_t* ble);

  /**
   * @brief             Provide the number of steps in given procedure
   * @param ble         Pointer to BLE CS data
   * @return            Number of steps
   */
  uint16_t l4_xtAPI_ble_get_num_steps(l4_csProcedure_t* ble);

  /**
   * @brief             Provide the number of MODE0 steps in given procedure
   * @param ble         Pointer to BLE CS data
   * @return            Number of MODE0 steps
   */
  uint16_t l4_xtAPI_ble_get_num_mode0_steps(l4_csProcedure_t* ble);

  /**
   * @brief             Provide the number of MODE1 steps in given procedure
   * @param ble         Pointer to BLE CS data
   * @return            Number of MODE1 steps
   */
  uint16_t l4_xtAPI_ble_get_num_mode1_steps(l4_csProcedure_t* ble);

  /**
   * @brief             Provide the number of MODE2 steps in given procedure
   * @param ble         Pointer to BLE CS data
   * @return            Number of MODE2 steps
   */
  uint16_t l4_xtAPI_ble_get_num_mode2_steps(l4_csProcedure_t* ble);

  /**
   * @brief               Allocation of memory and assigning to BLE pointers
   * @param ble           Pointer to BLE CS data
   * @param num_subevents Number of subevents to be assigned to memory
   * @return              Error code. If success the error code is 1. If fails the error code < 0
   */
//  l4_hadm_error_t l4_xtAPI_ble_malloc(l4_csProcedure_t* ble, uint8_t num_subevents);

  /**
   * @brief               Releasing of memory assigned to BLE pointers
   * @param ble           Pointer to BLE CS data
   * @return              Error code. If success the error code is 1. If fails the error code < 0
   */
//  l4_hadm_error_t l4_xtAPI_ble_free(l4_csProcedure_t* ble);

#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // *__L4_XTAPI_BLE_CONVERTER_H__
