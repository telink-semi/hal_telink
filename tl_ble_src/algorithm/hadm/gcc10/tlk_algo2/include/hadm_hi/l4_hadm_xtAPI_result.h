// File: l4_hadm_xtAPI_result.h

#ifndef L4_HADM_XTAPI_RESULT_H

#define L4_HADM_XTAPI_RESULT_H

#include <stdint.h>
#include <stdbool.h>
#include "l4_hadm_basics.h"
#include "l4_hadm_errors.h"

typedef struct _result_quality_indicators
{
  float peak_match_value;  ///< "rqi-pmv", how good does peak match into signal space. 1.0F is perfect, e.g. 0.7F is
                           ///< very poor
                           // #ifdef SLOTVIEW  commented out, because we need consistent sizes for the struct
  float fullpeak_match_value;
  // #endif

  uint8_t amplitude_noise;  ///< "rqi-an", 0 is perfect; 50 would be bad, deviation in amplitude between adjacent
                            ///< frequencies, this is an indicator for disturbances AND motion

  uint8_t
      disturbed_percent;  ///< "rqi-dp", 0F is perfect; else percent how many percent of the frequencies are disturbed

  uint8_t matrix_size;  ///< "rqi-ms", count of element for each row/colum in matrix

  uint8_t decorrelation_percent;  ///<  "rqi-deco", 0F is perfect; else percent how many percent of the frequencies are
                                  ///< decorrelated

  uint8_t accuracy_mhz;  ///< "rqi-am",bandwidth in the processed matrix minimum is 7MHz (?) maximum around 40MHz,
                         ///< total available bandwith in BLE CS is 74MHz

  uint8_t decorrelation_mhz;  ///< "rqi-dm",bandwidth in the whole frequency hopping, typically 74MHz, decorrelation
                              ///< may suffer if lower

  uint8_t peak_quality_percent;  ///< "rqi-qp",TO BE DEFINED, 100 is perfect

  uint8_t spectrum_consistency_percent;  ///< "rqi-cp", TO BE DEFINED, 100 is perfect

  float __peak_match_value2;  ///< "rqi-pmv", how good does peak match into signal space. 1.0F is perfect, e.g. 0.7F is
} result_quality_indicators_t;

typedef struct _l4hadm_xtAPI_rtt_protection_result
{
  float distance_corrected_deviation;  ///< "rtt": difference between measured distances and
                                       ///< RTT result.
  uint8_t valid_measurements_per_ap[L4_HADM_MAXANTENNAPATHS];
  float   deviation_meter_per_ap[L4_HADM_MAXANTENNAPATHS];
  float   noise_per_ap[L4_HADM_MAXANTENNAPATHS];
} l4hadm_xtAPI_rtt_protection_result_t;

/**
 * @brief xtAPI result containing the items used to output a JSON HADM result.
 */
struct _l4hadm_xtAPI1_result
{
  uint32_t        libversion;  ///< version of lib
  l4_hadm_error_t error;       ///< "er": error code

  uint32_t timestamp;                  ///< Time in seconds since boot [ms].
  uint32_t hadm_num;                   ///< "sq": Sequence number of measurements since boot.
  float    distance_premeter_highres;  ///< "ud": unfiltered distance  in meters.
  float    distance_mpros_highres;     ///< "uv": unfiltered velocity in meter/seconds.

  float ud_av_deviation_tooshort;
  float ud_av_deviation_toolong;
  float est_distance_meter;
  float est_deviation_tooshort;
  float est_deviation_toolong;
  float est_B_distance_meter;

  float    filtered_dist;             ///< "fd": filtered distance [m]
  uint16_t used_distances;            ///< "fn": number of used entries in distance filter
  float    distance_varianz_highres;  ///< "qm": variance of  distance.
  uint16_t fullpeak;                  ///< "qt": Quality indicator: result type A = 1, B = 0.
  uint16_t dmesolv_ct_ev;             ///< "qc": Quality indicator, complexity of scene:  [1..16]
  int16_t  pak_rssi_db;               ///<  "rssi":RSSI in dB, not actually dBm, but relative to an arbitrary 0dB point
  int16_t  tone_mid_rssi_db;  ///<  "tra": tone-RSSI-average in dB, averaged over all antenna pathes and frequencies
  int16_t  tone_max_rssi_db;  ///<  "trm": tone-RSSI-max in dB, maximum over all antenna pathes and frequencies
  float    distance_corrected_deviation;  ///< "rtt": difference between measured distances and RTT result.

  uint16_t rtt_valid;       ///< "rtv": RTT is valid.
  uint16_t rtt_calibrated;  ///< "rtc": 0 = no valid RTT calibration; 1 = calibration done with the device

  uint32_t slave_mac_h;  ///< "macrefl": mac adress of reflector
  uint32_t slave_mac_l;  ///< "macrefl": mac adress of reflector

  float delta_rssi_midamp_per_antennapath[L4_HADM_MAXANTENNAPATHS];
  float delta_rssi_maxamp_per_antennapath[L4_HADM_MAXANTENNAPATHS];

  uint16_t iqdata_is_pcfs;          ///< "pcf": 0 = conventional pll; 1 = use coherent cPLL
  uint32_t iqdata_timesync_age_ms;  ///< "tsy": = 0 no previous time sync or function not present
  // float    distance_var_might_be_as_short_as;  ///< "sd": -  Unsafe shortest path, the shortest detected path,
  ///< which might be not that reliable. In case of more available
  ///< information to confirm this USP to be true it could be used.
  // float distance_meter_might_be_as_short_as;   ///< sv:  variance of "sd" value

  // float distance_varianz_upper_limit;   ///< "ld":  Safe Shortest Path, the shortest calculated path which is high
  ///< reliable. We can be quite sure that the true distance is not above.
  // float distance_premeter_upper_limit;  ///< "lv": variance of "ld" value
  float consistency_factor;    ///< "cv": consistancy value: 2.0 is very good, below 1.6 might be critical, below 1.1 is
                               ///< definitely unsafe, lowest value is 1.0
  bool velocity_invalid;       ///< if true the hopping is insuficient to calculate velocity - only static use cases are
                               ///< possible ->generate WARNING
  bool hires180_invalid;       ///< if true the input data was to high correlated - so one stage of ambiguity solving
                               ///< failed ->generate WARNING
                               ///<  this will happen if we enter theoretic data into the algorithm with e.g. only one
                               ///<  signal path
  bool qualification_invalid;  ///< if true the input setting is cleared, but not fully qualified ->generate WARNING

  // new profile time
  uint32_t profile_time_us;  ///< "pt": time in us for the whole profile
  // new quality indicators
  //
  result_quality_indicators_t rqi;
  char*                       textbuf_todump_p;
  float                       true_distance_m;
  float                       true_velocity_ms;
  bool                        true_is_dynamic;
  bool                        true_distance_is_valid;
};
struct _l4hadm_xtAPI2_result
{
  uint32_t        libversion;  ///< version of lib
  l4_hadm_error_t error;       ///< "er": error code

  uint32_t timestamp;                  ///< Time in seconds since boot [ms].
  uint32_t hadm_num;                   ///< "sq": Sequence number of measurements since boot.
  float    distance_premeter_highres;  ///< "ud": unfiltered distance  in meters.
  float    distance_mpros_highres;     ///< "uv": unfiltered velocity in meter/seconds.

  float ud_av_deviation_tooshort;
  float ud_av_deviation_toolong;
  float est_distance_meter;
  float est_deviation_tooshort;
  float est_deviation_toolong;
  float est_B_distance_meter;

  float    filtered_dist;             ///< "fd": filtered distance [m]
  uint16_t used_distances;            ///< "fn": number of used entries in distance filter
  float    distance_varianz_highres;  ///< "qm": variance of  distance.
  uint16_t fullpeak;                  ///< "qt": Quality indicator: result type A = 1, B = 0.
  uint16_t dmesolv_ct_ev;             ///< "qc": Quality indicator, complexity of scene:  [1..16]
  int16_t  pak_rssi_db;               ///<  "rssi":RSSI in dB, not actually dBm, but relative to an arbitrary 0dB point
  int16_t  tone_mid_rssi_db;  ///<  "tra": tone-RSSI-average in dB, averaged over all antenna pathes and frequencies
  int16_t  tone_max_rssi_db;  ///<  "trm": tone-RSSI-max in dB, maximum over all antenna pathes and frequencies
  float    obsolete_distance_corrected_deviation;  ///< "rtt": replaced by rtt_protection_result in 2.2.7

  uint16_t rtt_valid;       ///< "rtv": RTT is valid.
  uint16_t rtt_calibrated;  ///< "rtc": 0 = no valid RTT calibration; 1 = calibration done with the device

  uint32_t slave_mac_h;  ///< "macrefl": mac adress of reflector
  uint32_t slave_mac_l;  ///< "macrefl": mac adress of reflector

  float delta_rssi_midamp_per_antennapath[L4_HADM_MAXANTENNAPATHS];
  float delta_rssi_maxamp_per_antennapath[L4_HADM_MAXANTENNAPATHS];

  uint16_t iqdata_is_pcfs;          ///< "pcf": 0 = conventional pll; 1 = use coherent cPLL
  uint32_t iqdata_timesync_age_ms;  ///< "tsy": = 0 no previous time sync or function not present
  // float    distance_var_might_be_as_short_as;  ///< "sd": -  Unsafe shortest path, the shortest detected path,
  ///< which might be not that reliable. In case of more available
  ///< information to confirm this USP to be true it could be used.
  // float distance_meter_might_be_as_short_as;   ///< sv:  variance of "sd" value

  // float distance_varianz_upper_limit;   ///< "ld":  Safe Shortest Path, the shortest calculated path which is high
  ///< reliable. We can be quite sure that the true distance is not above.
  // float distance_premeter_upper_limit;  ///< "lv": variance of "ld" value
  float consistency_factor;    ///< "cv": consistancy value: 2.0 is very good, below 1.6 might be critical, below 1.1 is
                               ///< definitely unsafe, lowest value is 1.0
  bool velocity_invalid;       ///< if true the hopping is insuficient to calculate velocity - only static use cases are
                               ///< possible ->generate WARNING
  bool hires180_invalid;       ///< if true the input data was to high correlated - so one stage of ambiguity solving
                               ///< failed ->generate WARNING
                               ///<  this will happen if we enter theoretic data into the algorithm with e.g. only one
                               ///<  signal path
  bool qualification_invalid;  ///< if true the input setting is cleared, but not fully qualified ->generate WARNING

  // new profile time
  uint32_t profile_time_us;  ///< "pt": time in us for the whole profile
  // new quality indicators
  //
  result_quality_indicators_t rqi;
  char*                       textbuf_todump_p;
  float                       true_distance_m;
  float                       true_velocity_ms;
  bool                        true_is_dynamic;
  bool                        true_distance_is_valid;
  // new members for version 2.2.7, old members renamed to 'obsolete_...'.
  l4hadm_xtAPI_rtt_protection_result_t rtt_protection_result;
};

typedef struct _l4hadm_xtAPI1_result
                                     l4hadm_xtAPI_result_2_2_5_t;  // Version 2.2.5 , 2.2.6  and 2.2.7 have the same struct
typedef struct _l4hadm_xtAPI1_result l4hadm_xtAPI_result_2_2_6_t;
typedef struct _l4hadm_xtAPI2_result l4hadm_xtAPI_result_2_2_7_t;

typedef struct _l4hadm_xtAPI2_result l4hadm_xtAPI_result_t;

#endif  // L4_HADM_XTAPI_RESULT_H
