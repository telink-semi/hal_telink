/**
**********************************************************************************************************************************************************************************************************************************
* @file:    l4_hadm_in_ble_api.h
* @author:
* @date:    2023/07/21  14:41:23 Friday
* @brief:
* Copyright 2023 Lambda:4 Entwicklungen GmbH, Germany.
*
* All rights reserved. Using, copying, publishing or distributing
* is not permitted without prior written agreement.
**********************************************************************************************************************************************************************************************************************************
**/
#ifndef __L4_HADM_IN_BLE_API_H__
#define __L4_HADM_IN_BLE_API_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus

  // Antenna permutation table
  static const uint8_t l4_hadm_ant_perm_n_ap[24][4] = {
      {0, 1, 2, 3},  // A1,A2,A3,A4
      {1, 0, 2, 3},  // A2,A1,A3,A4
      {0, 2, 1, 3},  // A1,A3,A2,A4
      {2, 0, 1, 3},  // A3,A1,A2,A4

      {2, 1, 0, 3},  // A3,A2,A1,A4
      {1, 2, 0, 3},  // A2,A3,A1,A4
      {0, 1, 3, 2},  // A1,A2,A4,A3
      {1, 0, 3, 2},  // A2,A1,A4,A3

      {0, 3, 1, 2},  // A1,A4,A2,A3
      {3, 0, 1, 2},  // A4,A1,A2,A3
      {3, 1, 0, 2},  // A4,A2,A1,A3
      {1, 3, 0, 2},  // A2,A4,A1,A3

      {0, 3, 2, 1},  // A1,A4,A3,A2
      {3, 0, 2, 1},  // A4,A1,A3,A2
      {0, 2, 3, 1},  // A1,A3,A4,A2
      {2, 0, 3, 1},  // A3,A1,A4,A2

      {2, 3, 0, 1},  // A3,A4,A1,A2
      {3, 2, 0, 1},  // A4,A3,A1,A2
      {3, 1, 2, 0},  // A4,A2,A3,A1
      {1, 3, 2, 0},  // A2,A4,A3,A1

      {3, 2, 1, 0},  // A4,A3,A2,A1
      {2, 3, 1, 0},  // A3,A4,A2,A1
      {2, 1, 3, 0},  // A3,A2,A4,A1
      {1, 2, 3, 0}   // A2,A3,A4,A1
  };

  // Antenna permutation inversion index table
  /*static const uint8_t l4_hadm_ant_perm_rev_idx_table[24] = {20, 17, 18, 10, 9, 13, 21, 16, 22, 4, 3, 15,
                                                             23, 5,  19, 11, 7, 1,  2,  14, 0,  6, 8, 12};*/
  static const uint8_t l4_hadm_ant_perm_rev_idx_table[24] = {0,  1,  2, 5,  4,  3,  6,  7,  14, 23, 22, 15,
                                                             12, 19, 8, 11, 16, 21, 18, 13, 20, 17, 10, 9};


  // Antenna permutation permutation count dependen from ct antenna paths table
  static const uint8_t l4_hadm_ant_perm_ct_for_antpath_ct[5] = {0, 1, 2, 6, 24};

// IAR C has no binary literals, so we use macros instead
#define L4_HADM_ANT_PERM_BITS_(a, b, c, d) ((a & 3) | (b & 3) << 2 | (c & 3) << 4 | (d & 3) << 6)
  static const uint8_t l4_hadm_ant_perm_bits[24] = {
      L4_HADM_ANT_PERM_BITS_(0, 1, 2, 3),  // A1,A2,A3,A4
      L4_HADM_ANT_PERM_BITS_(1, 0, 2, 3),  // A2,A1,A3,A4
      L4_HADM_ANT_PERM_BITS_(0, 2, 1, 3),  // A1,A3,A2,A4
      L4_HADM_ANT_PERM_BITS_(2, 0, 1, 3),  // A3,A1,A2,A4
      L4_HADM_ANT_PERM_BITS_(2, 1, 0, 3),  // A3,A2,A1,A4
      L4_HADM_ANT_PERM_BITS_(1, 2, 0, 3),  // A2,A3,A1,A4
      L4_HADM_ANT_PERM_BITS_(0, 1, 3, 2),  // A1,A2,A4,A3
      L4_HADM_ANT_PERM_BITS_(1, 0, 3, 2),  // A2,A1,A4,A3
      L4_HADM_ANT_PERM_BITS_(0, 3, 1, 2),  // A1,A4,A2,A3
      L4_HADM_ANT_PERM_BITS_(3, 0, 1, 2),  // A4,A1,A2,A3
      L4_HADM_ANT_PERM_BITS_(3, 1, 0, 2),  // A4,A2,A1,A3
      L4_HADM_ANT_PERM_BITS_(1, 3, 0, 2),  // A2,A4,A1,A3
      L4_HADM_ANT_PERM_BITS_(0, 3, 2, 1),  // A1,A4,A3,A2
      L4_HADM_ANT_PERM_BITS_(3, 0, 2, 1),  // A4,A1,A3,A2
      L4_HADM_ANT_PERM_BITS_(0, 2, 3, 1),  // A1,A3,A4,A2
      L4_HADM_ANT_PERM_BITS_(2, 0, 3, 1),  // A3,A1,A4,A2
      L4_HADM_ANT_PERM_BITS_(2, 3, 0, 1),  // A3,A4,A1,A2
      L4_HADM_ANT_PERM_BITS_(3, 2, 0, 1),  // A4,A3,A1,A2
      L4_HADM_ANT_PERM_BITS_(3, 1, 2, 0),  // A4,A2,A3,A1
      L4_HADM_ANT_PERM_BITS_(1, 3, 2, 0),  // A2,A4,A3,A1
      L4_HADM_ANT_PERM_BITS_(3, 2, 1, 0),  // A4,A3,A2,A1
      L4_HADM_ANT_PERM_BITS_(2, 3, 1, 0),  // A3,A4,A2,A1
      L4_HADM_ANT_PERM_BITS_(2, 1, 3, 0),  // A3,A2,A4,A1
      L4_HADM_ANT_PERM_BITS_(1, 2, 3, 0)   // A2,A3,A4,A1

#if 0
    0b11100100,  // A1,A2,A3,A4
    0b11100001,  // A2,A1,A3,A4
    0b11011000,  // A1,A3,A2,A4
    0b11010010,  // A3,A1,A2,A4
    0b11000110,  // A3,A2,A1,A4
    0b11001001,  // A2,A3,A1,A4
    0b10110100,  // A1,A2,A4,A3
    0b10110001,  // A2,A1,A4,A3
    0b10011100,  // A1,A4,A2,A3
    0b10010011,  // A4,A1,A2,A3
    0b10000111,  // A4,A2,A1,A3
    0b10001101,  // A2,A4,A1,A3
    0b01101100,  // A1,A4,A3,A2
    0b01100011,  // A4,A1,A3,A2
    0b01111000,  // A1,A3,A4,A2
    0b01110010,  // A3,A1,A4,A2
    0b01001110,  // A3,A4,A1,A2
    0b01001011,  // A4,A3,A1,A2
    0b00100111,  // A4,A2,A3,A1
    0b00101101,  // A2,A4,A3,A1
    0b00011011,  // A4,A3,A2,A1
    0b00011110,  // A3,A4,A2,A1
    0b00110110,  // A3,A2,A4,A1
    0b00111001   // A2,A3,A4,A1
#endif
  };

// HELPER MACROS FOR DENSE PACKED HCI MESSAGE
#if defined(__GNUC__)
#define L4PACKED_STRUCT struct __attribute__((__packed__))
#define L4PACKED_END
#define L4PACKED_UNION union __attribute__((__packed__))
#elif defined(__IAR_SYSTEMS_ICC__)
#define L4PACKED_STRUCT __packed struct
#define L4PACKED_UNION __packed union
#define L4PACKED_END
#elif defined(__CC_ARM)
#define L4PACKED_STRUCT struct __attribute__((packed))
#define L4PACKED_UNION union __attribute__((packed))
#define L4PACKED_END
#elif defined(_MSC_VER)
#define L4PACKED_STRUCT __pragma(pack(push, 1)) struct
#define L4PACKED_END __pragma(pack(pop))
#define L4PACKED_UNION __pragma(pack(push, 1)) union
#else
#warning No definition for L4PACKED_STRUCT and L4PACKED_UNION!
#endif

#define L4_MAX_NUM_ANTPATH 4
#define L4_MAX_NUM_STEPS 120
#define L4_MAX_NUM_SUBEVENTS 7
#define L4_MAX_NUM_MODE0_STORE_PER_SUBEVENT 3

#define L4_HADM_EVTIDX_MASK (0xFCU)
#define L4_HADM_EVTIDX_SHIFT (2U)
#define L4_NUM_HADM_BLE_CHANNELS 72  // 72 BLE channels usable in Band 2 (2402-2480 MHz)
#define L4_NUM_BL_STRUCTS 5          // Number of BLE structures in 6-bit dumps (BL0-BL4)

// const int L4_MAX_STEP_BYTES = (16+4*L4_MAX_NUM_ANTPATH); // 32 Worst case Mode3
#define L4_MAX_STEP_BYTES (3 + 4 * L4_MAX_NUM_ANTPATH)  // 19  Worst case Mode2

#define L4HadmEventResultEventSize_c (11U)

#define L4HadmDataSizeMax_c (4 /* HCI data header */ + (6 + 1 + 4 * (1 + L4_MAX_NUM_ANTPATH)) /* Mode 3 HCI length */)
#define L4HadmRawBufferSz_c (L4HadmEventResultEventSize_c + (L4HadmDataSizeMax_c * L4_MAX_NUM_STEPS))

  typedef uint8_t l4_ble_nxp_deviceId_t;  // local definition for BLE device id.

  /**
   * @brief mapping of a HCI 'subeventResult' message.not exactly the same as the hciMessage.
   */
#ifdef _MSC_VER
#pragma warning(disable : 4200)
#endif

  typedef L4PACKED_STRUCT l4_hciLeHadmEventResultEvent_tag
  {
    l4_ble_nxp_deviceId_t deviceId;
    uint8_t           configId;            /*!< HADM configuration id */
    uint16_t          startACLConnEvent;   /*!< Starting ACL con event count for this result */
    uint16_t          procedureCounter;    /*!< Procedure count since completion of the Security Start procedure */
    int8_t            referencePowerLevel; /*!< Reference Power Level for HADM proc (signed dBm) */
    int16_t frequencyCompensation; /*!< Frequency compensation value in units of 0.01 ppm (15-bit signed integer) */
    uint8_t procedureDoneStatus;   /*!< 0=not complete, 1=completed success, 0xFE/0xFF=Error */
    uint8_t subeventDoneStatus;    /*!< 0x00 = complete success, 0x01 = not complete, 0xFE/0xFF=Error */
    uint8_t abortReason; /*!< Abort reason for the CS procedure: b0-3 procedure done status = 0xF; b4-7 subevent done
                            status = 0x0F */
    uint8_t numAntennaPaths;  /*!< Number of antenna paths for RTP steps */
    uint8_t numStepsReported; /*!< Number of steps reported in this event [0x01-0xA0] */
    uint8_t data[0];          /*!< Encoded data, can't be structured */
  }
  l4_hciLeHadmEventResultEvent_t L4PACKED_END;

  /**
   * @brief mapping of a HCI 'configComplete' message.not exactly the same as the hciMessage.
   */
  typedef struct l4_config_complete_event_tag
  {
    l4_ble_nxp_deviceId_t deviceId;
    uint8_t           status;
    uint8_t           configId;
    uint8_t           mainModeType;
    uint8_t           subModeType;
    uint8_t           mainModeMinSteps;
    uint8_t           mainModeMaxSteps;
    uint8_t           mainModeRepetition;
    uint8_t           mode0Steps;
    uint8_t           role;
    uint8_t           RTTTypes;
    uint8_t           RTTPhy;
    uint8_t           t_ip1; /*!< HADM T_IP1 */
    uint8_t           t_ip2; /*!< HADM T_IP2 */
    uint8_t           t_fcs; /*!< HADM T_FCS */
    uint8_t           t_pm;  /*!< HADM T_PM */

  } l4_config_complete_event_t;

  /**
   * @brief mapping of a HCI 'procedureEnableComplete' message.not exactly the same as the hciMessage.
   */
  typedef struct l4_procedure_enable_complete_event_tag
  {
    l4_ble_nxp_deviceId_t deviceId;
    uint8_t           configId;
    uint8_t           status;
    uint8_t           state;
    uint8_t           toneAntennaConfigSelection;
    uint8_t           selectedTxPower;
    uint8_t           subeventLen[L4_MAX_NUM_SUBEVENTS];
    uint8_t           subeventsPerInterval;
    uint8_t           subeventInterval;
    uint16_t          eventInterval;
    uint16_t          procedureInterval;
    uint16_t          procedureCount;
  } l4_procedure_enable_complete_event_t;

  /**
   * @brief Additional information needed for timestamp deduction of single CS steps.
   */
  typedef struct l4_isp_cfg_optional_tag
  {
    int8_t  local_FAE[L4_NUM_HADM_BLE_CHANNELS];  /*!< Frequency actuation errors, in units of 1/32 ppm.*/
    int8_t  remote_FAE[L4_NUM_HADM_BLE_CHANNELS]; /*!< Frequency actuation errors, in units of 1/32 ppm.*/
    bool    input_is_reduced_to_one_path;         /*!<  only best path is used in accumulation of PCT data */
    bool    pcfs_remotedata_2wr_valid;            /*!< iq2 contains valid 2 way ranging data  */
    bool    pcfs_iq2_ppb_valid;                   /*!< iq2 contains valid sniffed ranging data */
    int16_t local_board_temp_celsius;             /*!< unit 0.1 degrees */
    int16_t remote_board_temp_celsius;            /*!< unit 0.1 degrees */
  } l4_isp_cfg_optional_t;

  typedef struct l4_ble_steptiming_tag
  {
    uint32_t step_timestamp[L4_MAX_NUM_STEPS]; /*!< microsecs timestamp of step measurement releative to start */
  } l4_ble_step_timing_t;

  typedef struct l4_isp_cfg_tag
  {
    uint16_t l4_ble_api_version; /*!< Version info ,will be set to L4HADM_LIB_V_MAYOR/MINOR*/
    uint32_t procedure_counter;  /*!< HADM usually from l4_hciLeHadmEventResultEvent_t */
    uint32_t initiator_id;       /*!< unique id of local partner */
    uint32_t reflector_id;       /*!< unique id of remote partner */
    uint32_t timestamp_216;      /*!< timestamp of measurement from from 2^16 Hz timer */
    uint8_t  t_sw;               /*!< HADM T_SW */
    uint8_t  t_pm_tone_ext;      /*!< HADM T_PM tone extension */
    bool     ant_perm_unknown;   /*!< Antenna permutation unknown */

    l4_isp_cfg_optional_t optional;
  } l4_isp_cfg_t;
  /**
   * @brief Accuumulated subeventResults. Header of first procedure subevent gets updated by later Xsubevents.
   *
   */
  typedef struct l4_hadmEventResultBuffer_tag
  {
    uint16_t rawBufferLen;
    union
    {
      l4_hciLeHadmEventResultEvent_t result;
      uint8_t                        raw[L4HadmRawBufferSz_c];
    } buffer;
  } l4_hadmEventResultBuffer_t;

  /**
   * @brief Input data for distance calculation based on BLE CS procedure information.
   */
  typedef struct l4_procedureinput_data_tag
  {
    l4_isp_cfg_t*                         l4_isp_cfg_p;
    l4_procedure_enable_complete_event_t* l4_procedure_enable_complete_event_p;
    l4_config_complete_event_t*           l4_config_complete_event_p;
    l4_hadmEventResultBuffer_t*           l4_initiator_events_p;
    l4_hadmEventResultBuffer_t*           l4_reflector_events_p;
    l4_ble_step_timing_t*                 l4_ble_step_timing_p;
  } l4_procedureinput_data_t;

  typedef struct l4_bl9_verifier_data_tag
  {
    uint32_t bl_fletchers[L4_NUM_BL_STRUCTS];
  } l4_bl9_verifier_data_t;

#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // !__L4_HADM_IN_BLE_API_H__
