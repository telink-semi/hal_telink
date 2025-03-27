/********************************************************************************************************
 * @file    bap.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#pragma once

/******************************* BAP Common Start **********************************************************************/

// Audio Support Frame Frequency (bitField, for PACS)
enum{
     BLC_AUDIO_SUPP_FREQ_FLAG_8000                          = BIT(0),  // 8000 Hz
     BLC_AUDIO_SUPP_FREQ_FLAG_11025                         = BIT(1),  // 11025 Hz
     BLC_AUDIO_SUPP_FREQ_FLAG_16000                         = BIT(2),  // 16000 Hz
     BLC_AUDIO_SUPP_FREQ_FLAG_22050                         = BIT(3),  // 22050 Hz
     BLC_AUDIO_SUPP_FREQ_FLAG_24000                         = BIT(4),  // 24000 Hz
     BLC_AUDIO_SUPP_FREQ_FLAG_32000                         = BIT(5),  // 32000 Hz
     BLC_AUDIO_SUPP_FREQ_FLAG_44100                         = BIT(6),  // 44100 Hz
     BLC_AUDIO_SUPP_FREQ_FLAG_48000                         = BIT(7),  // 48000 Hz
     BLC_AUDIO_SUPP_FREQ_FLAG_88200                         = BIT(8),  // 88200 Hz
     BLC_AUDIO_SUPP_FREQ_FLAG_96000                         = BIT(9),  // 96000 Hz
     BLC_AUDIO_SUPP_FREQ_FLAG_176400                        = BIT(10), // 176400 Hz
     BLC_AUDIO_SUPP_FREQ_FLAG_192000                        = BIT(11), // 192000 Hz
     BLC_AUDIO_SUPP_FREQ_FLAG_384000                        = BIT(12), // 384000 Hz
};
#define BLC_AUDIO_SUPP_FREQ_FLAG_VALID_BITS(param)          ((param) & (~BITS(13, 14, 15)))
#define BLC_AUDIO_SUPP_FREQ_FLAG_CHECK_RFU(param)           ((param) & BITS(13, 14, 15))

// Audio Support Frame Duration (bitField, for PACS)
enum{
     BLC_AUDIO_SUPP_DURATION_FLAG_7_5                        = BIT(0),
     BLC_AUDIO_SUPP_DURATION_FLAG_10                         = BIT(1),
     BLC_AUDIO_SUPP_DURATION_FLAG_7_5_PREFERRED              = BIT(4),
     BLC_AUDIO_SUPP_DURATION_FLAG_10_PREFERRED               = BIT(5),
};
#define BLC_AUDIO_SUPP_DURATION_FLAG_VALID_BITS(param)              ((param) & BITS(0, 1, 4, 5))
#define BLC_AUDIO_SUPP_DURATION_FLAG_RFU(param)                     ((param) & BITS(2, 3, 6, 7))

// Audio Channel counts (bitField, for PACS)
enum{
     BLC_AUDIO_CHANNEL_COUNTS_1                              = BIT(0),
     BLC_AUDIO_CHANNEL_COUNTS_2                              = BIT(1),
     BLC_AUDIO_CHANNEL_COUNTS_3                              = BIT(2),
     BLC_AUDIO_CHANNEL_COUNTS_4                              = BIT(3),
     BLC_AUDIO_CHANNEL_COUNTS_5                              = BIT(4),
     BLC_AUDIO_CHANNEL_COUNTS_6                              = BIT(5),
     BLC_AUDIO_CHANNEL_COUNTS_7                              = BIT(6),
     BLC_AUDIO_CHANNEL_COUNTS_8                              = BIT(7),
};
#define BLC_AUDIO_CHANNEL_COUNTS_RFU(param)                         ((param) == 0)
#define BLC_AUDIO_CHANNEL_COUNTS_MIN                                1
#define BLC_AUDIO_CHANNEL_COUNTS_MAX                                8

// Audio Frame Frequency (for codec parameter)
enum{
    BLC_AUDIO_FREQ_CFG_8000                                 =   1 , // 8000 Hz
    BLC_AUDIO_FREQ_CFG_11025                                =   2 , // 11025 Hz
    BLC_AUDIO_FREQ_CFG_16000                                =   3 , // 16000 Hz
    BLC_AUDIO_FREQ_CFG_22050                                =   4 , // 22050 Hz
    BLC_AUDIO_FREQ_CFG_24000                                =   5 , // 24000 Hz
    BLC_AUDIO_FREQ_CFG_32000                                =   6 , // 32000 Hz
    BLC_AUDIO_FREQ_CFG_44100                                =   7 , // 44100 Hz
    BLC_AUDIO_FREQ_CFG_48000                                =   8 , // 48000 Hz
    BLC_AUDIO_FREQ_CFG_88200                                =   9 , // 88200 Hz
    BLC_AUDIO_FREQ_CFG_96000                                =   10 ,// 96000 Hz
    BLC_AUDIO_FREQ_CFG_176400                               =   11 ,// 176400 Hz
    BLC_AUDIO_FREQ_CFG_192000                               =   12 , // 192000 Hz
    BLC_AUDIO_FREQ_CFG_384000                               =   13 ,// 384000 Hz
};
#define BLC_AUDIO_FREQ_CFG_MIN                                      0
#define BLC_AUDIO_FREQ_CFG_MAX                                      14
#define BLC_AUDIO_FREQ_CFG_RFU(param)                               ((param)<=BLC_AUDIO_FREQ_CFG_MIN || (param)>=BLC_AUDIO_FREQ_CFG_MAX)

// Audio Frame Duration (for codec parameter)
enum{
    BLC_AUDIO_DURATION_CFG_7_5                    =           0,
    BLC_AUDIO_DURATION_CFG_10                     =           1,
};
#define BLC_AUDIO_DURATION_RFU(param)                               ((param)!=BLC_AUDIO_DURATION_CFG_7_5 && (param)!=BLC_AUDIO_DURATION_CFG_10)

// Audio Support Location (for codec parameter)
enum{
    BLC_AUDIO_LOCATION_FLAG_FL                = BIT(0),  // Front Left
    BLC_AUDIO_LOCATION_FLAG_FR                = BIT(1),  // Front Right
    BLC_AUDIO_LOCATION_FLAG_FC                = BIT(2),  // Front Center
    BLC_AUDIO_LOCATION_FLAG_LFE1              = BIT(3),  // Low Frequency Effects 1
    BLC_AUDIO_LOCATION_FLAG_BL                = BIT(4),  // Back Left
    BLC_AUDIO_LOCATION_FLAG_BR                = BIT(5),  // Back Right
    BLC_AUDIO_LOCATION_FLAG_FLc               = BIT(6),  // Front Left of Center
    BLC_AUDIO_LOCATION_FLAG_FRc               = BIT(7),  // Front Right of Center
    BLC_AUDIO_LOCATION_FLAG_BC                = BIT(8),  // Back Center
    BLC_AUDIO_LOCATION_FLAG_LFE2              = BIT(9),  // Low Frequency Effects 2
    BLC_AUDIO_LOCATION_FLAG_SiL               = BIT(10), // Side Left
    BLC_AUDIO_LOCATION_FLAG_SiR               = BIT(11), // Side Right
    BLC_AUDIO_LOCATION_FLAG_TpFL              = BIT(12), // Top Front Left
    BLC_AUDIO_LOCATION_FLAG_TpFR              = BIT(13), // Top Front Right
    BLC_AUDIO_LOCATION_FLAG_TpFC              = BIT(14), // Top Front Center
    BLC_AUDIO_LOCATION_FLAG_TpC               = BIT(15), // Top Center
    BLC_AUDIO_LOCATION_FLAG_TpBL              = BIT(16), // Top Back Left
    BLC_AUDIO_LOCATION_FLAG_TpBR              = BIT(17), // Top Back Right
    BLC_AUDIO_LOCATION_FLAG_TpSiL             = BIT(18), // Top Side Left
    BLC_AUDIO_LOCATION_FLAG_TpSiR             = BIT(19), // Top Side Right
    BLC_AUDIO_LOCATION_FLAG_TpBC              = BIT(20), // Top Back Center
    BLC_AUDIO_LOCATION_FLAG_BtFC              = BIT(21), // Bottom Front Center
    BLC_AUDIO_LOCATION_FLAG_BtFL              = BIT(22), // Bottom Front Left
    BLC_AUDIO_LOCATION_FLAG_BtFR              = BIT(23), // Bottom Front Right
    BLC_AUDIO_LOCATION_FLAG_FLw               = BIT(24), // Front Left Wide
    BLC_AUDIO_LOCATION_FLAG_FRw               = BIT(25), // Front Right Wide
    BLC_AUDIO_LOCATION_FLAG_LS                = BIT(26), // Left Surround
    BLC_AUDIO_LOCATION_FLAG_RS                = BIT(27), // Right Surround
    BLC_AUDIO_LOCATION_FLAG_RFU               = BITS(28,29,30,31) // bit28 ~ bit29
};
#define BLC_AUDIO_CHANNEL_ALLOCATION_RFU(param)    ((param)&BLC_AUDIO_LOCATION_FLAG_RFU)

// Context Type
enum{
    BLC_AUDIO_CONTEXT_TYPE_PROHIBITED          = 0x0000, // Prohibited.
    BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED         = BIT(0), // Unspecified. Matches any audio content.
    BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL      = BIT(1), // Conversation between humans as, for example, in telephony or video calls.
    BLC_AUDIO_CONTEXT_TYPE_MEDIA               = BIT(2), // Media as, for example, in music, public radio, podcast or video soundtrack.
    BLC_AUDIO_CONTEXT_TYPE_GAME                = BIT(3), // Audio associated with video gaming
    BLC_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL       = BIT(4), // Instructional audio as
    BLC_AUDIO_CONTEXT_TYPE_VOICE_ASSISTANTS    = BIT(5), // Man-machine communication
    BLC_AUDIO_CONTEXT_TYPE_LIVE                = BIT(6), // Live audio
    BLC_AUDIO_CONTEXT_TYPE_SOUND_EFFECTS       = BIT(7), // Sound effects including keyboard and touch feedback; menu and user interface sounds; and other system sounds
    BLC_AUDIO_CONTEXT_TYPE_NOTIFICATIONS       = BIT(8), // Notification and reminder sounds; attention-seeking audio
    BLC_AUDIO_CONTEXT_TYPE_RINGTONE            = BIT(9), // Alerts the user to an incoming call.
    BLC_AUDIO_CONTEXT_TYPE_ALERT               = BIT(10), // Alarms and timers; immediate alerts
    BLC_AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM     = BIT(11), // Emergency alarm Emergency sounds
};
#define BLC_AUDIO_CONTEXT_TYPE_VALID_BITS(param)   ((param)&(~BITS(12, 13, 14, 15)))        //bit12 ~ bit15
#define BLC_AUDIO_CONTEXT_TYPE_CHECK_RFU(param)    ((param) == 0 || ((param)&BITS(12, 13, 14, 15)))

// Target latency type
enum{
    BLC_AUDIO_TARGET_LATENCY_LOW               =    0x01,
    BLC_AUDIO_TARGET_BALANCED_RELIABILITY      =    0x02,
    BLC_AUDIO_TARGET_HIGH_RELIABILITY          =    0x03,
};

// Target PHY type
enum{
    BLC_AUDIO_TARGET_PHY_1M                    =    0x01,
    BLC_AUDIO_TARGET_PHY_2M                    =    0x02,
    BLC_AUDIO_TARGET_PHY_CODED                 =    0x03,
};

// Coding Format
enum{
    BLC_AUDIO_CODING_FORMAT_U_LAW_LOG          = 0x00,
    BLC_AUDIO_CODING_FORMAT_A_LAW_LOG          = 0x01,
    BLC_AUDIO_CODING_FORMAT_CSVD               = 0x02,
    BLC_AUDIO_CODING_FORMAT_TRANSPARENT        = 0x03,
    BLC_AUDIO_CODING_FORMAT_LINEAR_PCM         = 0x04,
    BLC_AUDIO_CODING_FORMAT_MSBC               = 0x05,
    BLC_AUDIO_CODING_FORMAT_LC3                = 0x06,
    BLC_AUDIO_CODING_FORMAT_G729A              = 0x07, /* This is a draft allocation associated with draft specifications and is subject to change */
    BLC_AUDIO_CODING_FORMAT_VENDOR_SPECIFIC    = 0xFF,
};

enum{
     BLC_AUDIO_METATYPE_PREFERRED_CONTEXTS        =       0x01 , // Preferred_Audio_Contexts
     BLC_AUDIO_METATYPE_STREAMING_CONTEXTS        =       0x02 , // Streaming_Audio_Contexts
     BLC_AUDIO_METATYPE_PROGRAM_INFO              =       0x03 ,
     BLC_AUDIO_METATYPE_LANGUAGE                  =       0x04 , // Language,defined in ISO 693-3
     BLC_AUDIO_METATYPE_CCID_LIST                 =       0x05 , // CCID List,It can only changed when accepting a BAP Enable operation or a BAP Update Metadata operation by an Initiator
     BLC_AUDIO_METATYPE_PARENTAL_RATING           =       0x06 , // Parental rating
     BLC_AUDIO_METATYPE_PROGRAM_INFO_URI          =       0x07 , // UTF-8 formatted URL link used to present more information about Program Info
     BLC_AUDIO_METATYPE_AUDIO_ACTIVE_STATE        =       0x08 ,
     BLC_AUDIO_METATYPE_IMMEDIATE_RENDERING       =       0x09 , //Broadcast Audio Immediate Rendering Flag
     BLC_AUDIO_METATYPE_EXTENDED_METADATA         =       0xFE , // Extended Metadata
     BLC_AUDIO_METATYPE_VENDOR_SPECIFIC           =       0xFF , // Vendor_Specific
};

// Audio PHY (bitField, for ASCS)
enum{
    BLC_AUDIO_PHY_FLAG_1M                          = BIT(0), //LE 1M PHY preferred
    BLC_AUDIO_PHY_FLAG_2M                          = BIT(1), //LE 2M PHY preferred
    BLC_AUDIO_PHY_FLAG_CODED                       = BIT(2), //LE Coded PHY preferred
};

// Audio Framing (bitField, for ASCS)
enum{
    BLC_AUDIO_FRAMING_UNFRAMED                      = 0x00, //Unframed ISOAL PDUs preferred
    BLC_AUDIO_FRAMING_FRAMED                        = 0x01, //framed ISOAL PDUs preferred
};

// Audio Clock Accuracy
enum{
    BLC_AUDIO_CLOCK_ACCURACY_251_500PPM             = 0x00,
    BLC_AUDIO_CLOCK_ACCURACY_151_250PPM             = 0x01,
    BLC_AUDIO_CLOCK_ACCURACY_101_150PPM             = 0x02,
    BLC_AUDIO_CLOCK_ACCURACY_76_100PPM              = 0x03,
    BLC_AUDIO_CLOCK_ACCURACY_51_75PPM               = 0x04,
    BLC_AUDIO_CLOCK_ACCURACY_31_50PPM               = 0x05,
    BLC_AUDIO_CLOCK_ACCURACY_21_30PPM               = 0x06,
    BLC_AUDIO_CLOCK_ACCURACY_0_20PPM                = 0x07,
};
#define BLC_AUDIO_CLOCK_ACCURACY_DEFAULT            BLC_AUDIO_CLOCK_ACCURACY_76_100PPM

// Audio Packing Type
enum{
    BLC_AUDIO_PACKING_SEQUENTIAL                    = 0x00,
    BLC_AUDIO_PACKING_INTERLEAVED                   = 0x01,
};
#define BLC_AUDIO_PACKING_DEFAULT                   BLC_AUDIO_PACKING_SEQUENTIAL

// Announcement Type
enum{
    BLC_AUDIO_GENERAL_ANNOUNCEMENT                  = 0x00, //means the Unicast Server is connectable but is not requesting a connection.
    BLC_AUDIO_TARGETED_ANNOUNCEMENT                 = 0x01, //means the Unicast Server is connectable and is requesting a connection.
};



typedef struct{ //Parameters initialize for metadata
    u16 preferredContexts;  //Context Type is a preferred use case for this codec configuration, BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED
    u16 StreamingContexts;  //Context Type is a intended use case for this Audio Stream, BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED
    char* programInfo;      //Title and/or summary of Audio Stream content: UTF-8 format
    char* language;         //3-byte, lower case language code as defined in ISO 639-3
    u8 ccidListLen;         //
    u8* ccidList;           //
    u8 parentalRating;      //https://www.etsi.org
    char* programInfoURI;   //A UTF-8 formatted URL link used to present more information about Program_Info.
    u16 audioState;         //Audio Active State, 0x0000: no had this structures, 0xFFFF:mean is 0x00
    u8 immediateRendering;  //Broadcast Audio Immediate Rendering Flag

    u8 otherMetadataLen;    //
    u8* otherMetadata;      //use extended metadata
} blc_audio_metadataParam_t;

typedef struct{ //Parameters parsed from metadata
    u16 fieldExistFlg;
    u16 metadataLen;
    u16 prefCtx;        //metadata type  0x01
    u16 streamingCtx;   //metadata type  0x02
    u8 *pProgramInfo;   //metadata type  0x03
    u32  language;      //metadata type  0x04
    u8  programInfoLen;
    u8  ccidListLen;
    u8  programInfoURILen;
    u8  extMetadataLen;
    u8 *pCcidList;      //metadata type  0x05
    u8  parentalRating;//metadata type  0x06
    u8 *pProgramInfoURI;//metadata type  0x07
    u8 *pExtMetadata;   //metadata type  0xfe

    u8  rspCode;
    u8  rsnMark;
    u8  ignoreUnsuppMetadataFlag;
    u8  vsMetadataLen;
    u8 *pVendorSpecMetadata; //metadata type  0xff
} blc_audio_metadata_parsed_t;

typedef struct{
    u8  id;
    u16 companyID;
    u16 vendorID;
} blc_audio_codec_id_t;

typedef struct{
    u16 fieldExistFlg;

    u8  frequency;              // Sampling_Frequency
    u8  duration;               // Frame_Duration
    u32 allocation;             // Audio_Channel_Allocation
    u16 frameOcts;              // Octets_Per_Codec_Frame
    u8  codecFrameBlksPerSDU;   // Codec_Frame_Blocks_Per_SDU
} blc_audio_codecSpecCfgParsed_t;

typedef struct{
    u16 samplingFreq;           //[Must]Supported Sampling Frequencies, 0:RFU, BLC_AUDIO_SUPP_FREQ_FLAG_8000
    u8 frameDurations;          //[Must]Supported Frame Durations, 0:RFU, BLC_AUDIO_SUPP_DURATION_FLAG_7_5
    u8 channelCounts;           //[Must]Supported Audio Channel Counts, 0:RFU, BLC_AUDIO_CHANNEL_COUNTS_1

    struct{                     //[Must]Supported Octets Per Codec Frame
        u16 minPerCodecFrame;   //      Minimum number of octets supported per codec frame
        u16 maxPerCodecFrame;   //      Maximum number of octets supported per codec frame
    };
    u8 maxPerSdu;               //[Must]Supported Max Codec Frames Per SDU.
} blc_audio_codecSpecCapParam_t;

typedef struct{
    u8 samplingFreq;            //[Must]Sampling Frequency.         BLC_AUDIO_FREQ_CFG_8000
    u8 frameDuration;           //[Must]Frame Duration.             BLC_AUDIO_DURATION_CFG_7_5
    u32 channelAllocation;      //[Must]Audio Channel Allocation.   BLC_AUDIO_LOCATION_FLAG_FL
    u16 perCodecFrame;          //[Must]Per Codec Frame Octets.
    u8 perSduFrameBlocks;       //[Must]Codec Frame Blocks Per SDU.
} blc_audio_codecSpecCfgParam_t;

////////////////////////////Generic Audio////////////////////////
/**
 * @brief       This function use calculate metadata length.
 * @param[in]   metadata - initial metadata value pointer.
 * @return      metadata LTV structures length.
 */
u16 blc_bap_calculateMetadataLength(const blc_audio_metadataParam_t* metadata);

/**
 * @brief       This function use set metadata to destination address.
 * @param[in]   metadata    - initial metadata value pointer.
 * @param[in]   dst         - the pointer of write destination address.
 * @return      write finish pointer.
 */
u8* blc_bap_setMetadataToAddress(const blc_audio_metadataParam_t* metadata, u8* dst);

/**
 * @brief       This function use calculate Codec Capabilities length.
 * @param[in]   codecSpec - initial Codec Capabilities value pointer.
 * @return      Codec Capabilities LTV structures length.
 */
u8 blc_bap_calculateCodecSpecificCapabilitiesLength(const blc_audio_codecSpecCapParam_t* codecSpec);

/**
 * @brief       This function use set Codec Capabilities to destination address.
 * @param[in]   codecSpec   - initial Codec Capabilities value pointer.
 * @param[in]   dst         - the pointer of write destination address.
 * @return      write finish pointer.
 */
u8* blc_bap_setCodecSpecificCapabilitiesToAddress(const blc_audio_codecSpecCapParam_t* codecSpec, u8* dst);

/**
 * @brief       This function use calculate Codec Configuration length.
 * @param[in]   codecCfg - initial Codec Configuration value pointer.
 * @return      Codec Capabilities LTV structures length.
 */
u8 blc_bap_calculateCodecSpecificConfigurationLength(blc_audio_codecSpecCfgParam_t* codecCfg);

/**
 * @brief       This function use set Codec Configuration to destination address.
 * @param[in]   codecCfg    - initial Codec Configuration value pointer.
 * @param[in]   dst         - the pointer of write destination address.
 * @return      write finish pointer.
 */
u8* blc_bap_setCodecSpecificConfigurationToAddress(blc_audio_codecSpecCfgParam_t* codecCfg, u8* dst);

typedef enum  {
    AUDIO_DIR_SINK = 0x01,
    AUDIO_DIR_SOURCE = 0x02,
} audio_dir_enum;

#include "ascp/ascs_client_buf.h"
#include "ascp/ascs_server_buf.h"
#include "basp/bass_client_buf.h"
#include "basp/bass_server_buf.h"
#include "pacp/pacs_client_buf.h"
#include "pacp/pacs_server_buf.h"

/**
 * @brief   Standard codec settings recommended by BAP spec,
 *          view BAP_v1.0.1 page 25 for unicast server.
 *          view BAP_v1.0.1 page 33 for unicast client.
 *          view BAP_v1.0.1 page 34 for broadcast source.
 *          view BAP_v1.0.1 page 42 for broadcast sink.
 *          Used in unicast and broadcast scene.
 */
typedef enum{
    BLC_AUDIO_STD_FREQ_8K_DURATION_7_5MS_FRAME_26BYTES,
    BLC_AUDIO_STD_FREQ_8K_DURATION_10MS_FRAME_30BYTES,
    BLC_AUDIO_STD_FREQ_16K_DURATION_7_5MS_FRAME_30BYTES,
    BLC_AUDIO_STD_FREQ_16K_DURATION_10MS_FRAME_40BYTES,//support
    BLC_AUDIO_STD_FREQ_24K_DURATION_7_5MS_FRAME_45BYTES,
    BLC_AUDIO_STD_FREQ_24K_DURATION_10MS_FRAME_60BYTES,
    BLC_AUDIO_STD_FREQ_32K_DURATION_7_5MS_FRAME_60BYTES,
    BLC_AUDIO_STD_FREQ_32K_DURATION_10MS_FRAME_80BYTES,
    BLC_AUDIO_STD_FREQ_44_1K_DURATION_7_5MS_FRAME_97BYTES,
    BLC_AUDIO_STD_FREQ_44_1K_DURATION_10MS_FRAME_130BYTES,
    BLC_AUDIO_STD_FREQUENCY_48K_DURATION_7_5MS_FRAME_75BYTES,
    BLC_AUDIO_STD_FREQ_48K_DURATION_10MS_FRAME_100BYTES,//support
    BLC_AUDIO_STD_FREQUENCY_48K_DURATION_7_5MS_FRAME_90BYTES,
    BLC_AUDIO_STD_FREQ_48K_DURATION_10MS_FRAME_120BYTES,
    BLC_AUDIO_STD_FREQ_48K_DURATION_7_5MS_FRAME_117BYTES,
    BLC_AUDIO_STD_FREQ_48K_DURATION_10MS_FRAME_155BYTES,
    BLC_AUDIO_STD_CODEC_SETTINGS_E_MAX,
} blc_audio_std_codec_settings_enum;

typedef struct{
    u32   frequencyBitField;  //bit fields
    u8    frequencyValue;
    u8    durationBitField;   //bit fields
    u8    durationValue;
    u16   frameOctets;
} std_codec_settings_t;

/**
 * @brief   Standard qos settings recommended by BAP spec,
 *          view BAP_v1.0.1 page 87 for unicast audio.
 *          view BAP_v1.0.1 page 112 for broadcast audio.
 *          Qos settings need to be combined with Codec settings,such as 'audioConfig[blc_audio_qos_settings_e][blc_audio_codec_settings_e]'
 */
typedef enum{
    BLC_AUDIO_STD_QOS_LOW_LATENCY,
    BLC_AUDIO_STD_QOS_HIGH_RELIABILITY,
    BLC_AUDIO_STD_QOS_SETTINGS_E_MAX,
    //TODO,add a medium choice
}blc_audio_std_qos_settings_enum;

typedef struct{
    u32   sduInterval;         //us
    u8    framing;             //framed 0x01 or unframed 0x00
    u8    maxSduSize;          //sdu size
    u8    retransmitNum;       //retransmit number
    u16   maxTransportLatency; //max transport latency
} std_qos_settings_t;

/**
 * @brief   Standard Audio Configurations recommended by BAP spec,
 *          view BAP_v1.0.1 page 52
 */
typedef enum{
    BLC_AUDIO_1_SVR_1_SINK_1_CHN_1_SRC_N_CHN_N_CISES_1_STREAMS_1,   //1     C --------> S1
    BLC_AUDIO_2_SVR_1_SINK_N_CHN_N_SRC_1_CHN_1_CISES_1_STREAMS_1,   //2     C <-------- S1
    BLC_AUDIO_3_SVR_1_SINK_1_CHN_1_SRC_1_CHN_1_CISES_1_STREAMS_2,   //3     C <-------> S1
    BLC_AUDIO_4_SVR_1_SINK_1_CHN_2_SRC_N_CHN_N_CISES_1_STREAMS_1,   //4     C ------->> S1  Min Sink Audio Locations Per Server: 2
    BLC_AUDIO_5_SVR_1_SINK_1_CHN_2_SRC_1_CHN_1_CISES_1_STREAMS_2,   //5     C <------>> S1  Min Sink Audio Locations Per Server: 2

    BLC_AUDIO_6I_SVR_1_SINK_2_CHN_1_SRC_N_CHN_N_CISES_2_STREAMS_2,  //6(i)  C --------> - S1 Min Sink Audio Locations Per Server: 2
                                                                    //         ------->   S1

    BLC_AUDIO_6II_SVR_2_SINK_2_CHN_1_SRC_N_CHN_N_CISES_2_STREAMS_2, //6(ii) C --------> - S1 Min Sink Audio Locations Per Server: 1
                                                                    //         ------->   S2

    BLC_AUDIO_7I_SVR_1_SINK_1_CHN_1_SRC_1_CHN_1_CISES_2_STREAMS_2,  //7(i)  C --------> S1
                                                                    //        <-------- S1

    BLC_AUDIO_7II_SVR_2_SINK_1_CHN_1_SRC_1_CHN_1_CISES_2_STREAMS_2, //7(ii) C --------> S1
                                                                    //        <-------- S2

    BLC_AUDIO_8I_SVR_1_SINK_2_CHN_1_SRC_1_CHN_1_CISES_2_STREAMS_3,  //8(i)  C --------> S1  Min Sink Audio Locations Per Server: 2
                                                                    //        <-------> S1

    BLC_AUDIO_8II_SVR_2_SINK_2_CHN_1_SRC_1_CHN_1_CISES_2_STREAMS_3, //8(ii) C --------> S1  Min Sink Audio Locations Per Server: 1
                                                                    //        <-------> S2  Min Source Audio Locations Per Server: 1

    BLC_AUDIO_9I_SVR_1_SINK_N_CHN_N_SRC_2_CHN_1_CISES_2_STREAMS_2,  //9(i)  C <-------- S1  Min Source Audio Locations Per Server: 2
                                                                    //        <-------- S1

    BLC_AUDIO_9II_SVR_2_SINK_N_CHN_N_SRC_2_CHN_1_CISES_2_STREAMS_2, //9(ii) C <-------- S1  Min Source Audio Locations Per Server: 1
                                                                    //        <-------- S2

    BLC_AUDIO_10_SVR_1_SINK_N_CHN_N_SRC_1_CHN_2_CISES_1_STREAMS_1,  //10    C <<------- S1  Min Source Audio Locations Per Server: 2

    BLC_AUDIO_11I_SVR_1_SINK_2_CHN_1_SRC_2_CHN_1_CISES_2_STREAMS_4, //11(i) C <-------> S1  Min Sink Audio Locations Per Server: 2
                                                                    //        <-------> S1  Min Source Audio Locations Per Server: 2

    BLC_AUDIO_11II_SVR_2_SINK_2_CHN_1_SRC_2_CHN_1_CISES_2_STREAMS_4,//11(ii)C <-------> S1  Min Sink Audio Locations Per Server: 1
                                                                    //        <-------> S2  Min Source Audio Locations Per Server: 1

    BLC_AUDIO_STD_AUDIO_CONFIGURATIONS_E_MAX,
} std_unicast_aud_cfg_enum;

//Unicast LC3 Audio Configurations
typedef struct{
    u8  numOfSvr;
    u8  sinkASEs;
    u8  srcASEs;
    u8  audChnsPerSinkASE;
    u8  minSinkAudLocPerSvr;
    u8  audChnsPerSrcASE;
    u8  minSrcAudLocPerSvr;
    u8  CISes;
    u8  audStreams;
} std_unicast_aud_cfg_t;

/**
 * @brief   Standard Audio Configurations recommended by BAP spec,
 *          view BAP_v1.0.1 page 77
 */
typedef enum{
    BLC_AUDIO_12_CHN_1_PER_BIS_BISES_1_STREAMS_1,   //12         ---)))-->

    BLC_AUDIO_13_CHN_1_PER_BIS_BISES_2_STREAMS_2,   //13         ---)))-->
                                                    //           ---)))-->

    BLC_AUDIO_14_CHN_2_PER_BIS_BISES_1_STREAMS_1,   //14         ---)))->>

} std_bcst_aud_cfg_enum;

//Broadcast LC3 Audio Configurations
typedef struct{
    u8  audChnsPerBIS;
    u8  BISes;
    u8  audStreams;
} std_bcst_aud_cfg_t;

//codecSettings[blc_audio_std_codec_settings_enum]
extern const std_codec_settings_t  codecSettings[16];

//unicastQosSettings[blc_audio_qos_settings_e][blc_audio_std_codec_settings_enum]
extern const std_qos_settings_t    unicastQosSettings[2][16];

//broadcastQosSettings[blc_audio_qos_settings_e][blc_audio_std_codec_settings_enum]
extern const std_qos_settings_t    broadcastQosSettings[2][16];

//unicastAudioConfigurations[std_unicast_aud_cfg_enum]
extern const std_unicast_aud_cfg_t unicastAudioConfigurations[16];

//bcstAudioConfigurations[std_bcst_aud_cfg_enum]
extern const std_bcst_aud_cfg_t    bcstAudioConfigurations[3];

/******************************* BAP Common End **********************************************************************/

/******************************* BAP Unicast Client Start **********************************************************************/

typedef struct{
    //PAC record audio location
    u32 srcAudLoc;
    u32 sinkAudLoc;
    //Codec Configuration allocated location
    u32 sinkAudLocAlloc[2]; //max 2 channels
    u32 srcAudLocAlloc[2];  //max 2 channels

    u8 sinkCodecFrameBlksPerSDU;
    u8 srcCodecFrameBlksPerSDU;
    u8 sinkASEsPerSvr;
    u8 sinkASEId[2];//max 2 ASE
    u8 srcASEsPerSvr;
    u8 srcASEId[2];//max 2 ASE
} blc_audio_ase_cfg_info_t;

typedef struct {
    const blc_ascsc_regParam_t *pAscsParam;
    const blc_pacsc_regParam_t *pPacsParam;
} blc_bapuc_regParam_t;

//BAP Unicast Client Event ID
typedef enum{
    AUDIO_EVT_BAPUC_START = AUDIO_EVT_TYPE_BAPUC,
    AUDIO_EVT_BAPUC_SET_CIG_PARAMS,     //NULL event data, only Event ID
    AUDIO_EVT_BAPUC_CODEC_CONFIGURED,  //refer to 'blc_bapuc_codecConfiguredEvt_t'
    AUDIO_EVT_BAPUC_QOS_CONFIGURED,    //refer to 'blc_bapuc_qosConfiguredEvt_t'
    AUDIO_EVT_BAPUC_ENABLING,           //refer to 'blc_bapuc_enablingEvt_t
    AUDIO_EVT_BAPUC_DISABLING,          //refer to 'blc_bapuc_disablingEvt_t'
    AUDIO_EVT_BAPUC_UPDATE_METADATA,    //refer to 'blc_bapuc_updateMetadataEvt_t'
    AUDIO_EVT_BAPUC_RELEASING,          //refer to 'blc_bapuc_releasingEvt_t'
    /* Client (as Source) already enabling, after CIS OK, Server (as Sink) can autonomously initiates the Receiver Start Ready and enter Streaming. */
    AUDIO_EVT_BAPUC_SEND_STREAMING,     //refer to 'blc_bapuc_sendStreamingEvt_t'
    /* Client (as Sink) already enabling, write receive start ready cmd, Server (as Source) can enter Streaming. */
    AUDIO_EVT_BAPUC_RECEIVE_STREAMING,  //refer to 'blc_bapuc_receiveStreamingEvt_t'
} audio_bapuc_evt_enum;

typedef struct{ //Event ID: AUDIO_EVT_BAPUC_CODEC_CONFIGURED
    u16 aclHandle;
    audio_dir_enum  aseDir;
    u8  aseID;
    u8  framing;
    u8  PreferredRetransmitNum;
    u16 maxTransportLatency;
}blc_bapuc_codecConfiguredEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_QOS_CONFIGURED
    u16 aclHandle;
    audio_dir_enum  aseDir;
    u8  aseID;
    u8  framing;
    u8  PHY;
    u8  retransNum;
    u16 maxSdu;
    u16 maxTransLatency;
    u32 sduInterval;
    u32 presentationDelay;
}blc_bapuc_qosConfiguredEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_BAPUC_ENABLING
    u16 aclHandle;
    audio_dir_enum  aseDir;
    u8  aseID;
    blc_audio_metadata_parsed_t metaParam;
} blc_bapuc_enablingEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_BAPUC_DISABLING or AUDIO_EVT_BAPUC_RELEASING
    u16 aclHandle;
    audio_dir_enum  aseDir;
    u8  aseID;
} blc_bapuc_disablingEvt_t, \
  blc_bapuc_releasingEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_BAPUC_UPDATE_METADATA
    u16 aclHandle;
    audio_dir_enum  aseDir;
    u8  aseID;
    blc_audio_metadata_parsed_t metaParam;
} blc_bapuc_updateMetadataEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_BAPUC_SEND_STREAMING
                //          AUDIO_EVT_BAPUC_RECEIVE_STREAMING
    u16 aclHandle;
    u8  aseID;
} blc_bapuc_sendStreamingEvt_t, blc_bapuc_receiveStreamingEvt_t,\
  blc_audio_streamingEvt_t;


/**
 * @brief       This function serves to register BAP Unicast client
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerBapUnicastClient(const blc_bapuc_regParam_t *param);

/**
 * @brief       This function serves to check if the audio configuration match the audio capability.
 * @param[in]   aclHandle  - acl connect handle.
 * @param[in]   audCfgIdx  - audio configure index,search 'std_unicast_aud_cfg_enum' for detail.
 * @param[out]  outChnInfo - audio output information,if the audio configuration match the audio capability,user can use the information to configure the unciast stream
 *              search 'blc_audio_ase_cfg_info_t' for detail.
 * @return      none.
 */
int blc_bapuc_checkAudioConfigures(u16 aclHandle, std_unicast_aud_cfg_enum audCfgIdx, blc_audio_ase_cfg_info_t *outChnInfo);

/**
 * @brief       This function serves to configure the codec of the server.
 * @param[in]   aclHandle   - acl connect handle.
 * @param[in]   aseID       - audio stream index
 * @param[in]   codecCfgIdx - codec configure index,search 'blc_audio_std_codec_settings_enum' for detail.
 * @param[in]   pAseCfgInfo - audio information from the API 'blc_bapuc_checkAudioConfigures'.
 * @return      none.
 */
int blc_bapuc_setAseConfigCodec(u16 aclHandle, u8 aseID, blc_audio_std_codec_settings_enum codecCfgIdx, blc_audio_ase_cfg_info_t *pAseCfgInfo);

/**
 * @brief       This function serves to configure the qos of the server.
 * @param[in]   aclHandle   - acl connect handle.
 * @param[in]   aseID       - audio stream index
 * @param[in]   qosCfgIdx   - qos configure index,search 'blc_audio_std_qos_settings_enum' for detail.
 * @return      none.
 */
int blc_bapuc_setAseConfigQos(u16 aclHandle, u8 aseID, blc_audio_std_qos_settings_enum qosCfgIdx);

/**
 * @brief       This function serves to release the ASE.
 * @param[in]   connHandle - The ACL connection handle.
 * @param[in]   aseID      - The ASE need to operation.
 * @return      0          - The ASE operation successfully.
 *              Others     - The ASE operation failed.
 */
int blc_bapuc_setAseReceiverStartReady(u16 aclHandle, u8 aseID);

/**
 * @brief       This function serves to config the ASE to disable state.
 * @param[in]   connHandle - The ACL connection handle.
 * @param[in]   aseID      - The ASE need to disable.
 * @return      0          - The ASE disable successfully.
 *              Others     - The ASE disable failed.
 */
int blc_bapuc_setAseDisable(u16 aclHandle, u8 aseID);

/**
 * @brief       This function serves to release the ASE.
 * @param[in]   connHandle - The ACL connection handle.
 * @param[in]   aseID      - The ASE need to operation.
 * @return      0          - The ASE operation successfully.
 *              Others     - The ASE operation failed.
 */
int blc_bapuc_setAseReceiverStopReady(u16 aclHandle, u8 aseID);


/**
 * @brief       This function serves to set the metadata.
 * @param[in]   connHandle - The ACL connection handle.
 * @param[in]   aseID      - The ASE need to set the metadata.
 * @param[in]   metaParam  - The metadata configuration.
 * @return      0          - Set metadata operation successfully.
 *              Others     - Set metadata operation failed,search for BLC_AUDIO_ERROR_ENUM.
 */
int blc_bapuc_setAseMetadata(u16 aclHandle, u8 aseID, u8 *pMetadata, u8 metadataLen);

/**
 * @brief       This function serves to set the update the metadata.
 * @param[in]   connHandle - The ACL connection handle.
 * @param[in]   aseID      - The ASE need to set the update the metadata.
 * @param[in]   metaParam  - The metadata configuration.
 * @return      0          - Update metadata operation successfully.
 *              Others     - Update metadata operation failed,search for BLC_AUDIO_ERROR_ENUM.
 */
int blc_bapuc_setAseUpdateMetadata(u16 aclHandle, u8 aseID, u8 *pMetadata, u8 metadataLen);

/**
 * @brief       This function serves to release the ASE.
 * @param[in]   connHandle - The ACL connection handle.
 * @param[in]   aseID      - The ASE need to release.
 * @return      0          - The ASE release successfully.
 *              Others     - The ASE release failed.
 */
int blc_bapuc_setAseRelease(u16 aclHandle, u8 aseID);


/**
 * @brief       This function is used to send isochronous packet.
 * @param[in]   aclHandle  - The ACL connection handle.
 * @param[in]   idx        - index.
 * @param[in]   pPkt       - Raw packet need to be sent.
 * @param[in]   pktLen     - Raw packet length.
 * @return      0          - Isochronous packet send success.
 *              Others     - Isochronous packet send fail.
 */
int blc_bapuc_sduPacketPush(u16 aclHandle,u8 idx, u8 *pPkt, u16 pktLen);

/**
 * @brief       This function is used to pop received isochronous packet .
 * @param[in]   aclHandle  - The ACL connection handle.
 * @param[in]   aseID      - The ID of the ASE that received data .
 * @return[out] !NULL      - Isochronous packet pop success.
 *              NULL       - Isochronous packet pop failed.
 */
sdu_packet_t* blc_bapuc_sduPacketPop(u16 aclHandle,u8 idx);

/******************************* BAP Unicast Client End **********************************************************************/




/******************************* BAP Unicast Server Start **********************************************************************/

typedef struct{
    u8 epId;
    blc_audio_codec_id_t  codecid;
} blc_audio_config_codec_t;

typedef struct {
    const blc_ascss_regParam_t *pAscsParam;
    const blc_pacss_regParam_t *pPacsParam;
} blc_bapus_regParam_t;


//BAP Unicast Server Event ID
typedef enum{
    AUDIO_EVT_BAPUS_START = AUDIO_EVT_TYPE_BAPUS,
    AUDIO_EVT_BAPUS_CODEC_CONFIGURED,   //refer to 'blc_bapus_codecConfiguredEvt_t'
    AUDIO_EVT_BAPUS_QOS_CONFIGURED,     //refer to 'blc_bapus_qosConfiguredEvt_t'
    AUDIO_EVT_BAPUS_ENABLING,           //refer to 'blc_bapus_enablingEvt_t'
    AUDIO_EVT_BAPUS_UPDATE_METADATA,    //refer to 'blc_bapus_updateMetadataEvt_t'
    AUDIO_EVT_BAPUS_RELEASING,          //refer to 'blc_bapus_releasingEvt_t'
    AUDIO_EVT_BAPUS_DISABLING,          //refer to 'blc_bapus_disablingEvt_t'
    AUDIO_EVT_BAPUS_RECEIVE_STREAMING,  //refer to 'blc_bapus_receiveStreamingEvt_t'
    AUDIO_EVT_BAPUS_SEND_STREAMING,     //refer to 'blc_bapus_sendStreamingEvt_t'
} audio_bapus_evt_enum;

typedef struct{ //Event ID: AUDIO_EVT_BAPUS_CODEC_CONFIGURED
    audio_dir_enum dir;
    u8  audioEpId;
    blc_audio_codec_id_t  codecid;
    u8  frequency;
    u8  duration;
    u16 frameOcts;
    u32 location;
    u8  codecFrmBlksPerSDU;
} blc_bapus_codecConfiguredEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_BAPUS_QOS_CONFIGURED
    audio_dir_enum dir;
    u8  audioEpId;
    u8  cigID;
    u8  cisID;
    u8  framing;
    u8  PHY;
    u8  retransNum;
    u16 maxSdu;
    u16 maxTransLatency;
    u32 sduInterval;
    u32 presentationDelay;
} blc_bapus_qosConfiguredEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_BAPUS_ENABLING or AUDIO_EVT_BAPUS_UPDATE_METADATA
    audio_dir_enum dir;
    u8  audioEpId;
    u8  metaLen;
    u8  meta[255];
} blc_bapus_enablingEvt_t, blc_bapus_updateMetadataEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_BAPUS_RELEASING
                //          AUDIO_EVT_BAPUS_DISABLING
                //          AUDIO_EVT_BAPUS_RECEIVE_STREAMING
                //          AUDIO_EVT_BAPUS_SEND_STREAMING
    audio_dir_enum dir;
    u8  audioEpId;
} blc_bapus_disablingEvt_t, \
  blc_bapus_releasingEvt_t, \
  blc_bapus_sendStreamingEvt_t, \
  blc_bapus_receiveStreamingEvt_t;


/**
 * @brief       This function serves to register BAP unicast server
 * @param[in]   refer to 'blc_bapus_regParam_t'
 * @return      none.
 */
void blc_audio_registerBapUnicastServer(const blc_bapus_regParam_t *param);

/**
 * @brief       This function serves to configure the codec of the ASE by server.
 * @param[in]   aclHandle - acl connect handle
 * @param[in]   epId      - endpoint index
 * @return      none.
 */
int blc_bapus_aseConfigCodec(u16 aclHandle, u8 epId);

/**
 * @brief       This function serves to disable the ASE by server.
 * @param[in]   aclHandle - acl connect handle
 * @param[in]   epId      - endpoint index
 * @return      none.
 */
int blc_bapus_aseDisable(u16 aclHandle, u8 epId);

/**
 * @brief       This function serves to update the meta of the ASE by server.
 * @param[in]   aclHandle - acl connect handle
 * @param[in]   epId      - endpoint index
 * @param[in]   meta      - meta
 * @param[in]   metaLen   - meta data length
 * @return      none.
 */
int blc_bapus_aseUpdateMetadata(u16 aclHandle, u8 epId, u8 meta[], u8 metaLen);

/**
 * @brief       This function serves to execute the receive start ready operation by server.
 * @param[in]   aclHandle - acl connect handle
 * @param[in]   epId      - endpoint index
 * @return      none.
 */
int blc_bapus_aseReceiverStartReady(u16 aclHandle, u8 epId);


/**
 * @brief       This function serves to execute the relesased operation by server.
 * @param[in]   aclHandle - acl connect handle
 * @param[in]   epId      - endpoint index
 * @param[in]   cache     - 1:cache
 *                          0:no cache
 * @return      none.
 */
int blc_bapus_aseReleasedByCache(u16 aclHandle, u8 epId, bool cache);

/**
 * @brief       This function serves to execute the release operation by server.
 * @param[in]   aclHandle - acl connect handle
 * @param[in]   epId      - endpoint index
 * @return      none.
 */
int blc_bapus_aseRelease(u16 aclHandle,u8 epId);

/**
 * @brief       This function serves to push audio data.
 * @param[in]   aclHandle - acl connect handle
 * @param[in]   epId      - endpoint index
 * @param[in]   pPkt      - packet
 * @param[in]   pktL      - packet data length
 * @return      none.
 */
int blc_bapus_sduPacketPush(u16 aclHandle,u8 aseID, u8* pPkt, u16 pktLen);

/**
 * @brief       This function serves to pop audio data.
 * @param[in]   aclHandle - acl connect handle
 * @param[in]   epId      - endpoint index
 * @return      search for 'sdu_packet_t'.
 */
sdu_packet_t* blc_bapus_sduPacketPop(u16 aclHandle,u8 aseID);

/******************************* BAP Unicast Server End **********************************************************************/



/******************************* BAP Broadcast Sink Start **********************************************************************/

typedef struct {
    const blc_basss_regParam_t *pBassParam;
    const blc_pacss_regParam_t *pPacsParam;
} blc_bapbs_regParam_t;

typedef enum{
    BIG_SYNCED_FAILED,
    BIG_LOST,
    BIG_SYNCED,
} blc_audio_bigSyncState_enum;

typedef enum{
    PDA_SYNCED,
    PDA_SYNCED_FAILED,
    PDA_LOST,
} blc_audio_pdaSyncState_enum;

//BAP Broadcast Sink Event ID
typedef enum{
    AUDIO_EVT_BAPBS_START = AUDIO_EVT_TYPE_BAPBS,
    AUDIO_EVT_BAPBS_REMOTE_SCAN_STOPPED,            //NULL event data, only Event ID
    AUDIO_EVT_BAPBS_REMOTE_SCAN_STARTED,            //NULL event data, only Event ID
    AUDIO_EVT_BAPBS_BIS_SINK_INIT_CODEC,            //refer to 'blc_bapbs_bisSinkInitCodecEvt_t'
    AUDIO_EVT_BAPBS_BIS_SINK_SYNC_BIG,              //refer to 'blc_bapbs_BisSinkSyncBigEvt_t'
    AUDIO_EVT_BAPBS_PDA_SYNC_STATE,                 //refer to 'blc_bapbs_pdaSyncStateEvt_t'
} audio_bapbs_evt_enum;

typedef struct{
    blc_audio_codec_id_t    CodecId;
    blc_audio_codecSpecCfgParsed_t codecCfg;
    u8* metadata; //TODO: metadata info
} bisSyncInfo_t;
typedef struct{ //Event ID: AUDIO_EVT_BAPBS_BIS_SINK_INIT_CODEC
    u32 presentationDelay;
    u8 bisNum;
    bisSyncInfo_t bisInfo[0];
} blc_bapbs_bisSinkInitCodecEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_BAPBS_BIS_SINK_SYNC_BIG
    blc_audio_bigSyncState_enum state;
    u8 bigHandle;
    union{
        struct{
        u8 numBis;
        u16 isoInterval;
        u16 bisHandles[0];
        };
        u8 lostReason;
    };
} blc_bapbs_BisSinkSyncBigEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_BAPBS_PDA_SYNC_STATE
    blc_audio_pdaSyncState_enum  state;
    u16 syncHandle;
    u8  advSID;
    u8  advAddrType;
    u8  advAddr[6];
    u8  advPHY;
    u16 perdAdvItvl;
    u8  advClkAccuracy;
} blc_bapbs_pdaSyncStateEvt_t;

/**
 * @brief       This function serves to register BAP broadcast sink
 * @param[in]   refer to 'blc_bapbs_regParam_t'
 * @return      none.
 */
void blc_audio_registerBapBroadcastSink(const blc_bapbs_regParam_t *param);

/******************************* BAP Broadcast Sink End **********************************************************************/




/******************************* BAP Broadcast Assistant Start **********************************************************************/

typedef struct{
    u8 addrType;
    u8 addr[6];
    u8 sid;
    u8 broadcastId[3];
} blc_audio_source_head_t;

typedef struct{
    u32 bisSync;
    u8 metadataLen;
    u8* metadata;
} blc_audio_add_source_subgroup_t;

typedef struct {
    const blc_bassc_regParam_t *pBassParam;
    const blc_pacsc_regParam_t *pPacsParam;
} blc_bapba_regParam_t;

//BAP Broadcast Assistant Event ID
typedef enum{
    AUDIO_EVT_BAPBA_START = AUDIO_EVT_TYPE_BAPBA,
    AUDIO_EVT_BAPBA_FOUND_SINK,         //refer to 'blc_bapba_foundSinkEvt_t'
    AUDIO_EVT_BAPBA_START_SYNC_PA,      //refer to 'blc_bapba_startSyncPaEvt_t'
    AUDIO_EVT_BAPBA_FOUND_SOURCE_INFO,  //refer to 'blc_bapba_foundSourceInfoEvt_t'
    AUDIO_EVT_BAPBA_SOURCE_ENC_STATE,   //refer to 'blc_bapba_sourceEncStateEvt_t'
    AUDIO_EVT_BAPBA_PAST_STARTED_READY, //NULL event data, only Event ID
} audio_bapba_evt_enum;

typedef struct{ //Event ID: AUDIO_EVT_BAPBA_FOUND_SINK
    u8 addrType;
    u8 address[6];
    u8 completeNameLen;
    u8* completeName;
} blc_bapba_foundSinkEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_BAPBA_START_SYNC_PA
    u8 sid;
    u8 addrType;
    u8 address[6];
    u8 broadcastId[3];
    u8 completeNameLen;
    u8* completeName;
    u8 broadcastNameLen;
    u8* broadcastName;
} blc_bapba_startSyncPaEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_BAPBA_FOUND_SOURCE_INFO
    u8 sid;
    u8 addrType;
    u8 address[6];
    u8 bisIndex;
    u32 presentationDelay;
    bisSyncInfo_t bisInfo[1];
} blc_bapba_foundSourceInfoEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_BAPBA_SOURCE_ENC_STATE
    u8 enc;
} blc_bapba_sourceEncStateEvt_t;


/**
 * @brief       This function serves to register BAP broadcast assistant
 * @param[in]   currently not used, input NULL.
 * @return      none.
 */
void blc_audio_registerBroadcastAssistant(const blc_bapba_regParam_t *param);

/**
 * @brief       BAP broadcast assistant start scan.
 * @param[in]   connHandle: ACL connection handle.
 * @return      none.
 */
void blc_bapba_writeRemoteScanStarted(u16 connHandle);

/**
 * @brief       BAP broadcast assistant stop scan.
 * @param[in]   connHandle: ACL connection handle.
 * @return      none.
 */
void blc_bapba_writeRemoteScanStopped(u16 connHandle);

/**
 * @brief       BAP broadcast assistant add source by BASS client.
 * @param[in]   connHandle: ACL connection handle.
 * @param[in]   head: source information head pointer.
 * @param[in]   bissync: BIS sync state.
 * @return      none.
 */
void blc_bapba_writeAddSourceNotSyncPA(u16 connHandle, blc_audio_source_head_t *head, u32 bisSync);
void blc_bapba_writeAddSourcePast(u16 connHandle, blc_audio_source_head_t *head, u32 bisSync);
void blc_bapba_writeAddSourceNoPast(u16 connHandle, blc_audio_source_head_t *head, u32 bisSync);


/**
 * @brief       BAP broadcast assistant modify source by BASS client.
 * @param[in]   connHandle: ACL connection handle.
 * @param[in]   sourceID: source ID.
 * @param[in]   bissync: BIS sync state.
 * @return      none.
 */
void blc_bapba_writeModifySourceNotSyncPA(u16 connHandle, u8 sourceID, u32 bisSync);
void blc_bapba_writeModifySourcePast(u16 connHandle, u8 sourceID, u32 bisSync);
void blc_bapba_writeModifySourceNoPast(u16 connHandle, u8 sourceID, u32 bisSync);

/**
 * @brief       BAP broadcast assistant write set broadcast code command.
 * @param[in]   connHandle: ACL connection handle.
 * @param[in]   sourceID: source ID.
 * @param[in]   bcstCode: broadcast code, unsigned char[16].
 * @return      none.
 */
void blc_bapba_writeSetBroadcastCode(u16 connHandle, u8 sourceID, u8 bcstCode[16]);

/**
 * @brief       BAP broadcast assistant write remove source command.
 * @param[in]   connHandle: ACL connection handle.
 * @param[in]   sourceID: source ID.
 * @return      none.
 */
void blc_bapba_writeRemoveSource(u16 connHandle, u8 sourceID);

bool blc_bapba_startPAST(u16 connHandle, blc_audio_source_head_t *head);
void blc_bapba_stopPAST(u16 connHandle);
void blc_bapba_setLocalSourceInfo(u16 advHandle, blc_audio_source_head_t *head);

/******************************* BAP Broadcast Assistant  End **********************************************************************/


/******************************* BAP Broadcast Source Start **********************************************************************/

#define CODEC_SPEC_CFG(sampFreq, duration, octets)          .codecId = INIT_CODEC_ID_LC3,   \
                                                            .codecCfg = {.samplingFreq = sampFreq, .frameDuration = duration, .perCodecFrame = octets,}

#define LC3_CFG_8_1                     CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_8000, BLC_AUDIO_DURATION_CFG_7_5, 26)
#define LC3_CFG_8_2                     CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_8000, BLC_AUDIO_DURATION_CFG_10, 30)
#define LC3_CFG_16_1                    CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_16000, BLC_AUDIO_DURATION_CFG_7_5, 30)
#define LC3_CFG_16_2                    CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_16000, BLC_AUDIO_DURATION_CFG_10, 40)
#define LC3_CFG_24_1                    CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_24000, BLC_AUDIO_DURATION_CFG_7_5, 45)
#define LC3_CFG_24_2                    CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_24000, BLC_AUDIO_DURATION_CFG_10, 60)
#define LC3_CFG_32_1                    CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_32000, BLC_AUDIO_DURATION_CFG_7_5, 60)
#define LC3_CFG_32_2                    CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_32000, BLC_AUDIO_DURATION_CFG_10, 80)
#define LC3_CFG_441_1                   CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_44100, BLC_AUDIO_DURATION_CFG_7_5, 97)
#define LC3_CFG_441_2                   CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_44100, BLC_AUDIO_DURATION_CFG_10, 130)
#define LC3_CFG_48_1                    CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_DURATION_CFG_7_5, 75)
#define LC3_CFG_48_2                    CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_DURATION_CFG_10, 100)
#define LC3_CFG_48_3                    CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_DURATION_CFG_7_5, 90)
#define LC3_CFG_48_4                    CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_DURATION_CFG_10, 120)
#define LC3_CFG_48_5                    CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_DURATION_CFG_7_5, 117)
#define LC3_CFG_48_6                    CODEC_SPEC_CFG(BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_DURATION_CFG_10, 155)

#ifndef BIS_NUM_IN_PER_BIG_BCST
#define BIS_NUM_IN_PER_BIG_BCST                 0
#endif

#ifndef BIG_BCST_NUMBER
#define BIG_BCST_NUMBER                         0
#endif


//Broadcast Source
typedef struct{
    u8 BIS_index;
    blc_audio_codecSpecCfgParam_t codecCfg;
}blc_BASE_BIS_param_t;

typedef struct{
    u8 BIS_num;
    blc_audio_codec_id_t  codecId; //Codec ID, 06 0000 0000 mean LC3 codec
    blc_audio_codecSpecCfgParam_t codecCfg;
    blc_audio_metadataParam_t metadata;
    blc_BASE_BIS_param_t BIS_param[BIS_NUM_IN_PER_BIG_BCST];
} blc_BASE_BIG_param_t;

typedef struct{
    u32 presentation_delay; //Range:0x000000-0xFFFFFF  Units: us
    u8 subGroupNum;
    blc_BASE_BIG_param_t BIG_param[BIG_BCST_NUMBER];
} blc_bcstAudioAnnouncements_param_t;

/**
 * @brief       BAP broadcast source calculate Broadcast Audio Source Endpoint (BASE) structure length.
 * @param[in]   base: structure blc_bcstAudioAnnouncements_param_t.
 * @return      none.
 */
int blc_bap_calculateBASELength(void* base);

/**
 * @brief       BAP broadcast source set Broadcast Audio Source Endpoint (BASE) value to destination address.
 * @param[in]   base: structure blc_bcstAudioAnnouncements_param_t.
 * @param[in]   dst: destination pointer.
 * @return      write BASE ending pointer.
 */
u8* blc_bap_setBASEToAddress(void* base, u8* dst);

/******************************* BAP Broadcast Source End **********************************************************************/

