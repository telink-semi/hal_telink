/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/
#ifndef STACK_BLE_CONTROLLER_LL_CHN_SOUND_CHN_SOUND_STACK_H_
#define STACK_BLE_CONTROLLER_LL_CHN_SOUND_CHN_SOUND_STACK_H_


#ifndef CS_LOG_CHN_QUALITY
    #define CS_LOG_CHN_QUALITY 0
#endif

#if CS_LOG_CHN_QUALITY
typedef struct __attribute__((packed))
{
    u8  channel;
    u16 quality[0];
} step_quality_data_t;

typedef struct __attribute__((packed))
{
    u8                  chn_cnt;
    u8                  ant_num;
    step_quality_data_t step_qual[0];
} mode2_quality_log_t;

extern u8 cs_qual_log_data_local[2 + 72 * (1 + 2 * 4)]; // now ignore extend tone quality, max ant_path 4
extern u8 cs_qual_log_data_remote[2 + 72 * (1 + 2 * 4)];
#endif


#ifndef CS_NADM_EN
    #define CS_NADM_EN 0
#endif

#ifndef CHANNEL_SOUNDING_TEST_MODE_ENABLE
    #define CHANNEL_SOUNDING_TEST_MODE_ENABLE 0
#endif

#ifndef CS_SLEEP_CLOCK_ACCURACY
    #define CS_SLEEP_CLOCK_ACCURACY 0
#endif

#define HAILI_REQUIRE_MODE1_PREAMBLE_1B 1

#ifndef CS_PROC_REPEAT_TERMINATE
    #define CS_PROC_REPEAT_TERMINATE 1
#endif

#ifndef LL_CS_SNIFFER_MODE_ENABLE
    #define LL_CS_SNIFFER_MODE_ENABLE 0
#endif
#ifndef NUM_ANT_SUPPORT
    #define NUM_ANT_SUPPORT 0x01
#endif

#ifndef MAX_ANT_PATHS_SUPPORT
    #define MAX_ANT_PATHS_SUPPORT 0X02
#endif

#define CS_IDX_MSK                 0x0F
#define CS_IDX_FLG                 BIT(7)


#define CS_SUBEVENT_LEN_MIN        1250        //1250 us
#define CS_SUBEVENT_LEN_MAX        4000 * 1000 // 4s
#define CS_SUBEVENT_STEP_CNT_MAX   160

#define CS_MODE0_STEP_LEN_MAX      (5 + 3)
#define CS_MODE1_STEP_LEN_MAX      (14 + 3)
#define CS_MODE2_STEP_LEN_MAX      ((MAX_ANT_PATHS_SUPPORT + 1) * 4 + 1 + 3)           //(21+3)
#define CS_MODE3_STEP_LEN_MAX      (CS_MODE1_STEP_LEN_MAX + CS_MODE2_STEP_LEN_MAX - 3) // jaguar not support
#define CS_SUBEVENT_STEP_LEN_MAX   ((CS_MODE2_STEP_LEN_MAX > CS_MODE1_STEP_LEN_MAX) ? CS_MODE2_STEP_LEN_MAX : CS_MODE1_STEP_LEN_MAX)

#define CS_SUBEVENT_BUFF_LEN_MAX   (16 + CS_SUBEVENT_STEP_LEN_MAX * CS_SUBEVENT_STEP_CNT_MAX)
#define STEP_NUM_PER_SUBEVENT      (CS_SUBEVENT_STEP_CNT_MAX)
#define SLIP_WINDOW_STEP_NUM       (CS_SUBEVENT_STEP_CNT_MAX)


#define TEST_HAILI_MODE2_TONE_CALC 1

#define CS_SUBEVENT_LEN_EVALUATE   1
#define SUBEVENT_DATA_OFFSET       2
/*
 * case : only mode type = 2; C-CLK = 96Mhz;max ant path = 1; ant num = 1;72 chnanel;
 * CHNMap repeat = 2:  max duration time is 140ms
 * CHNMap repeat = 1:  max duration time is 105ms
 *
 * case : only mode type = 2; C-CLK = 48Mhz;max ant path = 1; ant num = 1;72 chnanel;
 * CHNMap repeat = 2:  max duration time is 262ms
 * CHNMap repeat = 1:  max duration time is 158ms
 */
#define CS_HADM_DURATION_US                   100000

#define CS_SUBEVNET_RESULT_REPORT_DURATION_US 2000 //rpl + hci event report need max 2ms @96Mhz
/*
 *  2024.08.21  fanqh & jiapeng
 */
#define CS_PHASE_CONTINUE_OPTIMIZE1 1
#define CS_REL_STX_OPTIMIZE         1

#if (SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_80MS)
    #define CS_SCH_FIFONUM 16 //16*5 = 80 mS
#elif (SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_120MS)
    #define CS_SCH_FIFONUM 24 //24*5 = 120 mS
#elif (SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_160MS)
    #define CS_SCH_FIFONUM 32 //32*5 = 160 mS
#elif (SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_240MS)
    #define CS_SCH_FIFONUM 48 //48*5 = 240 mS
#else
    #error "unsupported CS_SCH FIFONUM!!!"
#endif


/*
 * 1M PHY, 1 byte for preamble extra length
 * 2(preamble) + 4(access address) + 1(4 bit trail), PRMBL_LENGTH_1M
 */
#define SYNC_PDU_1M_LEN                 7
#define SYNC_PDU_1M_LEN_MODE1_CALI      6
#define SYNC_PDU_1M_MODE0_LEN           8

#define PCT_SIZE                        ((MAX_ANT_PATHS_SUPPORT + 1) * 2) //+1 indicate ext slot

#define DBG_CS_LOG_SCH_MASK_EN          (0)

#define MODE_0_T_SY_1M_US               (8 + 32 + 4)            //1 preamble + 4 access address + 4bit trailer
#define MODE_0_T_SY_2M_US               ((8 + 8 + 32 + 4) >> 1) //2 preamble + 4 access address + 4bit trailer

#define MODE_1_T_SY_1M_US_WITHOUT_SS_RS MODE_0_T_SY_1M_US
#define MODE_1_T_SY_2M_US_WITHOUT_SS_RS MODE_0_T_SY_2M_US

#define MODE_3_T_SY_1M_US_WITHOUT_SS_RS MODE_0_T_SY_1M_US
#define MODE_3_T_SY_2M_US_WITHOUT_SS_RS MODE_0_T_SY_2M_US

#define MODE_0_T_FM_US                  (80)
#define T_GD_US                         (10)  //independent of the LE PHY
#define T_RD_US                         (5)

#define TLK_T_MES                       (150) //us

#define CS_MODE0_TX_EARLY_US_1M         13    //8us(1 byte preamble) + 5us(1M PHY TXLLDLY + TXPATHDLY). now mode0 use preamble.
#define CS_MODE2_TX_EARLY_US_1M         5     //5us(1M PHY TXLLDLY + TXPATHDLY). mode2 send Tone, not use preamble and access code.

#define CS_MODE0_RX_EARLY_US_1M         20
#define CS_MODE2_RX_EARLY_US_1M         5
#define CS_MODE1_RX_EARLY_US_1M         5


#define CS_ACCESSCODE_THRESHOLD         31


#define T_PW_40US                       (40)
#define T_PW_20US                       (20)
#define T_PW_10US                       (10)

#define T_SW_10US                       (10)
#define T_SW_4US                        (4)
#define T_SW_2US                        (2)
#define T_SW_1US                        (1)

enum
{
    ATTACK_EXTREMELY_UNLIKELY = 0x00,
    ATTACK_VERY_UNLIKELY      = 0x01,
    ATTACK_UNLIKELY           = 0x02,
    ATTACK_IS_POSSIBLE        = 0x03,
    ATTACK_IS_LIKELY          = 0x04,
    ATTACK_IS_VERY_LIKELY     = 0x05,
    ATTACK_EXTREMELY_LIKELY   = 0x06,
    ATTACK_UNKNOWN            = 0xFF,
};

enum
{
    No_CS_Terminate_Proc          = 0,
    Receive_Send_CS_Terminate_RSP = 1,
    Remove_CS_Task_with_Terminate = 2,
    Wait_Next_CS_Proc_Start       = 3,
};

enum
{
    CHANNEL_SOUNDING_ROLE_INITIATOR = 0,
    CHANNEL_SOUNDING_ROLE_REFLECTOR = 1,
};

enum
{
    BLE_1M_PHY     = 1,
    BLE_2M_PHY     = 2,
    BLE_2M_2BT_PHY = 3,
};

enum
{
    CHANNEL_MAP_ALL_USED_REFRESH = BIT(7),
};

typedef struct __attribute__((packed))
{
    u8  cali_chn_idx;
    u8  cali_tenThousand_flag;
    u16 cali_step_cnt;
} calibration_mode1_ctrl_t;

typedef struct __attribute__((packed))
{
    u8 step_modeType;
    u8 step_chnIdx;

    union
    {
        struct __attribute__((packed))
        {
            u8 step_extSlotRefl : 1; //bit0 indicate reflector. confirm with sunwei.
            u8 step_extSlotInit : 1; //bit1 indicate initiator. confirm with sunwei.
            u8 step_rfu         : 6;
        };

        u8 step_extSlotFlag;
    } extSlot;

    u8 step_antPathPermIdx;  //Antenna Path Permutation Index;//when subevent start, for(jumpStep){calculate}


    u8 step_initRttSeq[16];  //max 128bit = 16B; either sounding sequence or random sequence.//when subevent start, for(jumpStep){calculate}
    u8 step_reflRttSeq[16];  //max 128bit = 16B; either sounding sequence or random sequence.//when subevent start, for(jumpStep){calculate}

    u8 step_initRttSSPos[2]; //only sounding sequence use it. for 96 bit, max 2 mark position. only two type:32 bit and 96 bit.
    u8 step_reflRttSSPos[2]; //only sounding sequence use it. for 96 bit, max 2 mark position. only two type:32 bit and 96 bit.

    u32 step_initAA;         //when subevent start, to calculate.
    u32 step_reflAA;         //when subevent start, to calculate.

    u8 subeventEndFlag;
    u8 proceStopFlag;        //Channel_Map_Repetition
    u8 seqLen;
    u8 seqMode;
} slip_window_step_t;

enum
{
    CS_T_IP_10US  = BIT(0),
    CS_T_IP_20US  = BIT(1),
    CS_T_IP_30US  = BIT(2),
    CS_T_IP_40US  = BIT(3),
    CS_T_IP_50US  = BIT(4),
    CS_T_IP_60US  = BIT(5),
    CS_T_IP_80US  = BIT(6),
    CS_T_IP_145US = BIT(7),
};

enum
{
    CS_T_FCS_15US  = BIT(0),
    CS_T_FCS_20US  = BIT(1),
    CS_T_FCS_30US  = BIT(2),
    CS_T_FCS_40US  = BIT(3),
    CS_T_FCS_50US  = BIT(4),
    CS_T_FCS_60US  = BIT(5),
    CS_T_FCS_80US  = BIT(6),
    CS_T_FCS_100US = BIT(7),
    CS_T_FCS_120US = BIT(8),
    CS_T_FCS_150US = BIT(9),
};

enum
{
    PROC_CS_CAP_SEND_REQ    = BIT(0),
    PROC_CS_CAP_SEND_RSP    = BIT(1),
    PROC_CS_CAP_WAIT_RSP    = BIT(2),
    PROC_CS_CAP_EVT_PENDING = BIT(7),
};

enum
{
    PROC_CS_SEC_SEND_REQ    = BIT(0),
    PROC_CS_SEC_SEND_RSP    = BIT(1),
    PROC_CS_SEC_WAIT_RSP    = BIT(2),
    PROC_CS_SEC_EVT_PENDING = BIT(7),
};

enum
{
    PROC_CS_FAE_SEND_REQ    = BIT(0),
    PROC_CS_FAE_SEND_RSP    = BIT(1),
    PROC_CS_FAE_WAIT_RSP    = BIT(2),
    PROC_CS_FAE_EVT_PENDING = BIT(7),
};

enum
{
    PROC_CS_CONFIG_SEND_REQ    = BIT(0),
    PROC_CS_CONFIG_SEND_RSP    = BIT(1),
    PROC_CS_CONFIG_WAIT_RSP    = BIT(2),
    PROC_CS_CONFIG_EVT_PENDING = BIT(7),
};

enum
{
    PROC_CS_SEND_REQ    = BIT(0),
    PROC_CS_SEND_RSP    = BIT(1),
    PROC_CS_WAIT_RSP    = BIT(2),
    PROC_CS_SEND_IND    = BIT(3),
    PROC_CS_WAIT_IND    = BIT(4),
    PROC_CS_PWL_PENDING = BIT(5),
    PROC_CS_EVT_PENDING = BIT(7),
};

enum
{
    PROC_CS_TERMINATE_SEND_REQ    = BIT(0),
    PROC_CS_TERMINATE_SEND_RSP    = BIT(1),
    PROC_CS_TERMINATE_WAIT_RSP    = BIT(2),
    PROC_CS_TERMINATE_EVT_PENDING = BIT(7),
};

typedef struct __attribute__((packed))
{
    u8 csRspCheckErr;
    u8 csConfigExchErr;
    u8 csStartErr;
    u8 csTermiFlag;

    u8 csPowerCtrl;
    u8 configCollision;
    u8 csConnUptErr;
    u8 rsvd;
} chn_sound_ll_flow_ctrl_t;

extern chn_sound_ll_flow_ctrl_t csFlowCtrl;

typedef struct __attribute__((packed))
{
    u8 role_enable;    //BIT(0): initiator, BIT(1): reflector
    u8 CS_SYNC_AntSel; // 1--4,  0xFE,in repetitive from 1--4, 0xff: not have recommendation
    s8 Max_TX_Power;   // -127dbm --- -20dBm
    u8 cs_config_pend_idx;

    u8 cs_config_req;
    u8 cs_cap_req;
    u8 cs_cap_exchange; //init set 0
    u8 cs_security_exchange;

    u8 cs_security_enable;
    u8 cs_pend_idx;
    u8 cs_req;
    u8 cs_fae_exchange;

    u8  cs_terminate_ind;
    u8  cs_terminate_error_code;
    u16 rsvd2;

    u8  cs_fae_req;
    u8  cs_chn_map_ind;
    u16 cs_chn_map_instance;

    u8 CS_IV_C[8];
    u8 CS_IN_C[4];
    u8 CS_PV_C[8];
    u8 CS_IV_P[8];
    u8 CS_IN_P[4];
    u8 CS_PV_P[8];
    u8 CS_IV[16];
    u8 CS_IN[8];
    u8 CS_PV[16];
    u8 fae_table[72];

    u8 drbg_data[216]; //sizeof(drbg_param_t) == 214
} cs_param_t;

enum
{
    CS_CONFIG_STA_DISABLE = 0,
    CS_CONFIG_STA_ENABLE  = 1,
};

#define CHN_REPEAT_BUFF_LEN 4

typedef struct __attribute__((packed))
{
    u8  idx;
    u8  occupy;
    u16 aclHandle;


    u8 state;
    u8 cs_procedure_para_set_en;
    u8 cs_procedure_en;
    u8 cs_procedure_measurement_en;

    u8 Config_ID;
    u8 Create_Context;
    u8 Main_Mode;
    u8 Sub_Mode;

    u8 Main_Mode_Min_Steps;    // range 0x01-0xff
    u8 Main_Mode_Max_Steps;
    u8 Main_Mode_Repetition;   //0--3
    u8 Mode_0_Steps;           //1---3

    u8 Role;                   //init = 0, reflector = 1
    u8 RTT_Type;               //0--6
    u8 CS_SYNC_PHY;            //1:1M, 2: 2M
    u8 Channel_Map[10];
    u8 Channel_Map_Repetition; //0~3
    u8 ChSel;                  //0,1
    u8 Ch3c_Shape;             //0,1

    u8 Chm_Ind_Map[10];
    u8 Origin_Chn_Map[10];

    u8 Chn_en_num;        //channel number that mask valid with ChM
    u8 flag_endEvtInProc; //the end CS event in CS procedure
    u8 Selected_TX_Power;
    s8 cs_sub_event_oft;

    u8  Ch3c_Jump; //2--8
    u8  Companion_Signal_Enable;
    u16 connEventCount;

    u32 offset_min;
    u32 offset_max;
    u32 csOft_us;
    u32 Subevent_Len; //in unit of microsecond and >=1250us & <4s
    u32 Min_Subevent_Len;
    u32 Max_Subevent_Len;

    u16 Max_Procedure_Len; //unit:625us/bSlot
    u16 Procedure_Interval;
    u16 Min_Procedure_Interval;
    u16 Max_Procedure_Interval;

    u16 procMaxCount; //Procedure_Count
    u16 Event_Interval;


    u16 subEvtIntvl_625us;
    u8  Subevents_Per_Event;
    u8  aci; //Antenna Configuration Index

    u8 Tone_Antenna_Config_Selection;
    s8 Tx_Pwr_Delta;
    u8 Preferred_Peer_Ant;
    u8 PHY;

    u16 mode0Step_durUs;
    u16 mode1Step_durUs;
    u16 mode2Step_durUs;
    u16 mode3Step_durUs;

    u16 mode0TxIntvalUs;
    u16 mode1TxIntvalUs;
    u16 mode2TxIntvalUs;
    u16 mode3TxIntvalUs;

    u16 mode2ToneUs;
    u16 mode2ToneUs_noExtslot;

    u16 mode2IQ_StartIdx; // 4 + early_us *4*5; unit: byte index
    u16 mode2IQ_RxIntval; // (T_IP2 + T_SW) * 4 * 5 unit: byte index
    u16 mode2IQ_ValidPMLen;
    u16 procMaxCountInstant;

    u32 mode2IQ_OffsetTick;

    u8 mode0_sync_us;
    u8 none_mode_sync_us;

    u16 csProcCount;

    u16 startCsProcCount;
    u16 endCsProcCount;

    u8 T_IP1;
    u8 T_IP2;
    u8 T_FCS;
    u8 T_PM;

    u8 T_IP1_Us;
    u8 T_IP2_Us;
    u8 T_FCS_Us;
    u8 T_PM_Us;

    u8  T_SW_Us;
    u8  antennaPathNum;
    u16 sch_early_us;

    u32 sSlot_csSubIntvl;
    u32 bSlotEndCsEvent; // It is updated when the initial value is set and the procedure dure_len is configured after LL_cs_ind is sent or received


    u16 inst_start_proc;
    u16 cs_inst_acl; //start acl conn event

    u32 bSlot_start_proc;
    u32 tick_proc_start;
    u32 tick_expect_csSubevent;
    u32 tick_mark_csSubevent;


    s32 sSlot_mark_csSubevent;
    u32 bSlot_mark_csSubevent;
    u32 step_expect_tick;

    u8  csTsk_wptr;
    u8  csTsk_rptr;
    u16 sSlotCsDuration;

    u8 chn_update_pend;
    u8 seqNum_mark_csSubEvent;
    u8 chnMRepeCnt;
    u8 proc_end_flag; //can delete

    u8 stopSch;
    u8 submode_insertion;
    u8 mainmode_repeat_wptr;
    u8 mainmode_repeat_rptr;

    u8 mainmode_repeat_chn[CHN_REPEAT_BUFF_LEN];


    u8 mode0ShuffledChnArray[80];     //max 72 channel, 72=0x48 align 4 .AUTO_CALIB_CHIP_INTERNAL_DELAY_EN need cali 80 chn,so buffer size need set to 80 Byte.
    u8 nonmode0ShuffledChnArray[404]; //for #3b max 72 channel, 72=0x48 align 4, for #3c max 134*3 = 402

    u8 filteredChnArray[72];

    u32 noneMode0ShuffledChannelNum; //noneMode0ShuffledChannelNum;

    u8 csReportTermiEvt;
    u8 procStopRsn;

    u16 mode0_chnReadIdx;
    u16 nonMode0_chnReadIdx;

    u16 csChnAvailNum; //include repetition map. mode-0 and non-mode-0 use the same variable.
    u16 mainNum_noSubMode;

    u8 slip_stepReadIdx;
    u8 slip_stepWriteIdx;
    u8 cs_procdure_1st_flag;
    u8 step_rx_flag;


    u8  mode0_rx_flag;
    u8  phaseContinue_cal_flag;
    u16 chn_update_inst;

    u16 max_subEvtCnt;
    u16 subEvtCnt;

    u8  subEvtContinue;
    u8  calcSubEvtMargin;
    u16 subEvtMargin;

    u32 acl_ac_threshold;

    u8  csRspProcRole; // this parameter is used to check if we init cs procedure or rsp. This will influence chn map update.
    u8  mode0SyncMark;
    u32 t_sy_center_delta;

    u16 winWideUs;                                             // SLEEP_CLOCK_ACCURACY
    u16 sync_pky_cnt;

    slip_window_step_t slip_window_step[SLIP_WINDOW_STEP_NUM]; //according to actual situation to change value. now temporary set to 64.
    cs_sch_task_t      csTskFifo[CS_SCH_FIFONUM];

} cs_config_t;

enum
{
    PROC_DONE_MAX_STEP_CNT    = BIT(0),
    PROC_DONE_ALL_3B_CHN_USED = BIT(1),
    PROC_DONE_ALL_3C_CHN_USED = BIT(2),
    PROC_DONE_LAST_EVENT      = BIT(3),
    PROC_DONE_MAX_PROC_LEN    = BIT(4),

};

enum
{
    CSA_3B = 0,
    CSA_3C = 1,
};

enum
{
    PROC_NO_ABORT                 = 0x0,
    PROC_ABORT_HOST_REMOTE_REQ    = 0x1,
    PROC_ABORT_CHN_LESS_15        = 0x2,
    PROC_ABORT_CHN_INST_PASS      = 0x3,
    PROC_ABORT_UNSPECIFIED_REASON = 0xF, // proc abort bit0~3

    SUBEVT_NO_ABORT                               = 0x0,
    SUBEVT_ABORT_HOST_REMOTE_REQ                  = 0x1,
    SUBEVT_ABORT_NO_MODE0_RECEIVED                = 0x2,
    SUBEVT_ABORT_SCHE_CONFLICT_AND_LIMIT_RESOURCE = 0x3,
    SUBEVT_ABORT_UNSPECIFIED_REASON               = 0xF, // subevt abort bit4~7

};

typedef struct __attribute__((packed))
{
    u8  main_mode_repetition;
    u8  mode_step;
    u16 drbg_nonce;

    u8 Mode_0_Steps;
    u8 Main_Mode;
    u8 Sub_Mode;
    u8 Transmit_Power_Level;

    u8 test_mode_en;
    u8 submode_insertion;
    u8 Main_Mode_Max_Steps;
    u8 Main_Mode_Min_Steps;

    u8 role;
    u8 rtt_type;
    u8 cs_cync_phy;
    u8 cs_sync_ant_selection;

    u32 subevent_len_us;

    u16 subevent_interval_625;
    u8  slip_stepReadIdx;
    u8  slip_stepWriteIdx;

    u8 max_num_subevent;
    u8 power_level;
    u8 ip1_us;
    u8 ip2_us;

    u8 fcs_us;
    u8 pm_us;
    u8 sw_us;
    u8 aci;

    u8 companion_signal_en;
    u8 snr_init;
    u8 snr_refl;
    u8 cs_procdure_1st_flag;

    u8 loopMarkerSigCnt;
    u8 subEvtCnt;
    u8 mainmode_repeat_wptr;
    u8 mainmode_repeat_rptr;

    u16 mode0_chnReadIdx;
    u16 nonMode0_chnReadIdx;

    u8 Ch3c_Jump;
    u8 Chn_en_num;
    u8 ChSel;
    u8 Ch3c_Shape;

    u8 filteredChnArray[72];
    u8 mode0ShuffledChnArray[72];
    u8 nonmode0ShuffledChnArray[72];

    u8 Channel_Map[10];
    u8 Channel_Map_Repetition;
    u8 rsdv;

    u32 noneMode0ShuffledChannelNum;

    u8 mainmode_repeat_chn[CHN_REPEAT_BUFF_LEN];

    u16 mode0Step_durUs;
    u16 mode1Step_durUs;
    u16 mode2Step_durUs;
    u16 mode3Step_durUs;

    u16 mode0TxIntvalUs;
    u16 mode1TxIntvalUs;
    u16 mode2TxIntvalUs;
    u16 mode3TxIntvalUs;

    u8  mode0_sync_us;
    u8  none_mode_sync_us;
    u16 sch_early_us;

    u16 mode2ToneUs;
    u16 mode2ToneUs_noExtslot;

    u32 step_expect_tick;

    u8  step_rx_flag;
    u8  antennaPathNum;
    u16 csProcCount;

    u8 CS_SYNC_PHY;
    u8 phaseContinue_cal_flag;
    u8 firstReflRx;
    u8 mode0Sche;

    u8  mode0_rx_flag;
    u8  rsvd1;
    u16 rsvd2;

    u32 tick_proc_start;

    slip_window_step_t slip_window_step[SLIP_WINDOW_STEP_NUM];


} cs_testModeCfg_t;

typedef struct __attribute__((packed))
{
    u8 max_num_cofig;
    u8 capCalibValue;
    u8 capCalibValuePre;
    u8 cs_subeventResultReportType;

    u8 rsvd[1];
    u8 cs_get_rf_cali_flag;
    u8 chn_map[10];

    u8   *hciRxFifo_b;
    u8    hciRxFifoNum;
    u8    hciRxFifoMask;
    u16   hciRxFifoSize;
    u8    hciFifoWptr;
    u8    hciFifoRptr;
    float rpl_factor;

    u32          chn_map_upt_tick;
    cs_config_t *gGlobal_pCsCfg;
    cs_config_t *blt_pCsCfg;

    cs_testModeCfg_t *cs_tdCfg;
} cs_mng_t;

typedef struct __attribute__((packed))
{
    u32 dma_len;
    u8  preamble[2];
    u32 accessAddress;
    u8  trailer        : 4;
    u8  shift_sequence : 4;
    u8  sequence[16]; //max 128 bits
} rf_packet_cs_t;

extern rf_packet_cs_t pkt_CS;


#if (HAILI_REQUIRE_MODE1_PREAMBLE_1B)
typedef struct __attribute__((packed))
{
    u32 dma_len;
    u8  preamble[1];
    u32 accessAddress;
    u8  trailer        : 4;
    u8  shift_sequence : 4;
    u8  sequence[16]; //max 128 bits
} rf_packet_cs_mode1_t;

extern rf_packet_cs_mode1_t pkt_CS_m1;
#endif


typedef struct __attribute__((packed))
{
    u32 dma_len;
    u8  preamble[2];
    u32 accessAddress;
    u16 trailer; // change 4bits trailer to 16bits to fix 'rssi = -110' issue
} rf_packet_cs_mode0_t;

extern rf_packet_cs_mode0_t pkt_CS_m0;

typedef struct __attribute__((packed))
{
    u16 lenIQ;     //unit :byte
    u16 lenSample; // sample number include I and Q 5bytes

    u16 startIQIdx;
    u8  tone_ext;  //tone_extension_flag
    u8  d_T_SW;

    u8 d_N_AP;
    u8 d_ACI;
    u8 d_API; //Antenna Path Permutation Index
    u8 rsvd;
} cs_step_IQ_param_t;

extern cs_step_IQ_param_t csStepIQ_param;

typedef struct __attribute__((packed))
{
    u16 check_sum;
    u16 len            : 13;
    u16 cali_2M_flag   : 1;
    u16 cali_1M_flag   : 1;
    u16 cali_tone_flag : 1;
    u8  cali_table[0];
} cs_flash_cali_table_t;

extern u32 cs_tick_tx_on;
extern u8  cs_rx_agc_gain;


u8                  blt_ll_getCsConfigByConnHandle(u16 connHandle);
u8                  blt_ll_getCsConfigByRole(u16 connHandle, cs_config_role_t role);
u8                  blt_ll_getCsConfigById(u16 connHandle, u8 config_id);
u8                  blt_ll_getNewCsConfig(void);
ble_sts_t           blt_ll_cs_chnMapUpdateProce(void);
slip_window_step_t *blt_cs_getSlipWindow(void);
slip_window_step_t *blt_cs_getLastSlipWindow(void);
slip_window_step_t *blt_cs_getTestModeSlipWindow(void);
void                blt_cs_packetSyncPDU(rf_packet_cs_t *pPkt, u32 access_code, slip_window_step_t *pStep, u8 cs_role);

void blt_cs_packetSyncPDU_mode1_cali(rf_packet_cs_mode1_t *pPkt, u32 access_code, slip_window_step_t *pStep, u8 cs_role);


int  blt_cs_subevent_post(unsigned char phase_en);
s32  blt_ll_cs_getStepRxFreqOffset(u8 phy, u8 *raw_data);
void blt_ll_cs_getStepIQParam(u8 role, u8 *raw_data, cs_step_IQ_param_t *step_param);
u8   blt_ll_cs_getPktMatchSyncQuality(u8 *raw_data);
void blt_ll_cs_Convert20BitIQ2int(u8 *data_src, s32 *data_dest, u16 len_sample);
void blt_ll_cs_Convert20BitIQ2Float(u8 *data_src, float *data_dest, u16 len_sample);
u8   blt_ll_cs_getToneQualityIndicator(float toneQualityRaw);
void blt_cs_subevent_rf_init(void);
void blt_cs_subevent_rf_deinit(unsigned char phase_en);

typedef struct __attribute__((packed))
{
    u16 size;
    u8  size_div_16;
    u8  rsvd;

    u8 num;
    u8 mask;
    u8 wptr;
    u8 rptr;

    u8 *p_base;
    u8 *pCsRxAddr;
} cs_rx_fifo_t;

extern _attribute_ble_data_retention_ cs_rx_fifo_t cs_rx_fifo;

extern chn_sound_capabilities_t bltCsLocalSupportCap;
extern cs_mng_t                 gCsMng;
extern u8                       cs_fae_cmplt_reason;


#ifndef DBG_CS_DATA_PRINT_EN
    #define DBG_CS_DATA_PRINT_EN 0
#endif


#ifndef DBG_CS_DATA_USB_PRINT_EN
    #define DBG_CS_DATA_USB_PRINT_EN 0
#endif


#ifndef DBG_CS_RX_FIFO_ENABLE
    #define DBG_CS_RX_FIFO_ENABLE 0
#endif


#ifndef DBG_CS_SUBEVENT_ENABLE
    #define DBG_CS_SUBEVENT_ENABLE 0
#endif


#ifndef DBG_CS_DISTANCE_STRING_PRINT_EN
    #define DBG_CS_DISTANCE_STRING_PRINT_EN 0
#endif


#ifndef CS_ANTENNA_SWITCHING_DATA_EN
    #define CS_ANTENNA_SWITCHING_DATA_EN 1
#endif


//B92
//DAM_len(4 Bytes), no CRC
//IQ_20_BIT_data(iq_sample_number*5) + hd_normal(8 Bytes) + hd_extension(36 Bytes)
#define DMA_CS_RFRX_OFFSET_DMA_LEN 0
#define DMA_CS_RFRX_OFFSET_IQ_DATA 4
#define DMA_CS_RFRX_DMA_LEN(p)     (p[DMA_CS_RFRX_OFFSET_DMA_LEN] + (p[DMA_CS_RFRX_OFFSET_DMA_LEN + 1] << 8))

#if (CHIP_TYPE == CHIP_TYPE_TL721X)
    #define DMA_CS_RFRX_IQ_DATA_LEN(p) (DMA_CS_RFRX_DMA_LEN(p) - 52)
#else
    #define DMA_CS_RFRX_IQ_DATA_LEN(p) (DMA_CS_RFRX_DMA_LEN(p) - 44)
#endif

#define DMA_CS_RFRX_OFFSET_TIME_STAMP(p)        (DMA_CS_RFRX_OFFSET_IQ_DATA + DMA_CS_RFRX_IQ_DATA_LEN(p))
#define DMA_CS_RFRX_OFFSET_FREQ_OFFSET(p)       (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 4)
#define DMA_CS_RFRX_OFFSET_SYNC_FLAG(p)         (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 5)
#define DMA_CS_RFRX_OFFSET_RSSI(p)              (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 6)
#define DMA_CS_RFRX_OFFSET_STATUS(p)            (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 7)
#define DMA_CS_RFRX_OFFSET_IQ_START_TSTAMP(p)   (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 8)
#define DMA_CS_RFRX_OFFSET_PKT_MATCH_SYNC(p)    (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 12)
#define DMA_CS_RFRX_OFFSET_PKT_TX_POS_TSTAMP(p) (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 24)
#define DMA_CS_RFRX_OFFSET_PKT_TX_NEG_TSTAMP(p) (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 28)

//This structure is related to DMA_CS_RFRX_OFFSET above it, so do not modify it. By SunWei, 20240620.
// note: The first 44bytes in this structure are assigned by hardware,
// however, in order to save ram, the software overwrites some word fields.
typedef struct __attribute__((packed))
{
    u32 timestamp;             //0 HW

    u8 freq_offset;            //4 HW
    u8 sync_flag;              //5 HW
    u8 rssi;                   //6 HW
    u8 status;                 //7 HW

    u32 iq_start_tstamp;       //8 HW

    u8 pkt_match_sync;         //12 HW
                               //    u8 rsdv_0[11];

    u16 start_acl_conn_event;  // 13 SW
    u16 procedure_counter;     // 15  SW
    u8  procedure_done_status; // 17 SW
    u8  subevent_done_status;  // 18SW
    u8  rx_agc_gain;           //   19 SW
    u32 last_tx_pos_tstamp;    //   20 SW

    u32 pkt_tx_pos_tstamp;     //24 HW
    u32 pkt_tx_neg_tstamp;     //28 HW


                               //    u8 rsdv_1[12];
    u32 tx_on_tstamp;                          // 32 SW
    u32 rx_access_address;                     // 36 SW
    u32 tick_cs_proc_start;                    // 40 SW

#if (CHIP_TYPE == CHIP_TYPE_TL721X)
    u32 tx_frac_time_pos;                      // 44
    u32 tx_frac_time_neg;                      // 48
#endif

    cs_config_t      *config_struct_addr;      //44/52
    cs_testModeCfg_t *config_struct_addr_test; //48/56
    u8                ant_path_perm_idx;       //   52 SW //60
    u8                rsvd[1];                 //53 SW //61
} cs_rx_para_t;

//cs_rx_buff[2]
#define BLT_CS_INITIATOR_FLAG                BIT(7)
#define BLT_CS_REFLECTOR_FLAG                BIT(6)
#define BLT_CS_MODE_RX_FLAG                  BIT(5)
#define BLT_CS_STEP_TONE_EXTENSION_SLOT_FLAG BIT(4)
#define BLT_CS_MODE_3_FLAG                   BIT(3)
#define BLT_CS_MODE_2_FLAG                   BIT(2)
#define BLT_CS_MODE_1_FLAG                   BIT(1)
#define BLT_CS_MODE_0_FLAG                   BIT(0)

//cs_rx_buff[3]
#define BLT_CS_STEP_CHANNEL_MASK          0x7F //BIT(6)~BIT(0)

#define CS_RFRXEN_MODE_EARLY_US           5    //5us(1M PHY RXPATHDLY) RXEN MODE
#define CS_RFRXEN_MODE_1M_EARLY_US        5    //5us(1M PHY RXPATHDLY) RXEN MODE
#define CS_RFRXEN_MODE_2M_EARLY_US        3    //3us(2M PHY RXPATHDLY) RXEN MODE
#define CS_RF_TX_1M_PACKET_EARLY_US       13   //8us(1 byte extra preamble) + 5us(1M PHY TXLLDLY + TXPATHDLY)
#define CS_RF_TX_1M_PACKET_EARLY_US_MODE1 13   //5us(1M PHY TXLLDLY + TXPATHDLY) //mode 1 use 1 byte preamble.
#define CS_RF_TX_2M_PACKET_EARLY_US       4    //4us(2M PHY TXLLDLY + TXPATHDLY)
#define CS_RF_TX_2M_TONE_EARLY_US         4    //4us(2M PHY TXLLDLY + TXPATHDLY)

// if enable software DCOC, secondary filter will be turned on, causing RX 2us later, so add 2us to TX early time
#if SW_DCOC_EN
    #define CS_RF_TX_1M_PACKET_EARLY_US_MODE0      15        //8us(1 byte extra preamble) + 5us(1M PHY TXLLDLY + TXPATHDLY)
    #define CS_RF_TX_1M_PACKET_EARLY_US_MODE1_CALI 7         //mode1 no need extra 1byte preamble
    #define CS_RF_TX_1M_TONE_EARLY_US              7         //5us(1M PHY TXLLDLY + TXPATHDLY)
#else
    #define CS_RF_TX_1M_PACKET_EARLY_US_MODE0      13        //8us(1 byte extra preamble) + 5us(1M PHY TXLLDLY + TXPATHDLY)
    #define CS_RF_TX_1M_PACKET_EARLY_US_MODE1_CALI 5         //mode1 no need extra 1byte preamble
    #define CS_RF_TX_1M_TONE_EARLY_US              5         //5us(1M PHY TXLLDLY + TXPATHDLY)
#endif

#define CS_1M_PACKET_AA_ONLY_US                44            //44us for 1M = 1 byte preamble + 4 byte access address + 4 bit trailer
#define CS_2M_PACKET_AA_ONLY_US                26            //26us for 2M = 2 byte preamble + 4 byte access address + 4 bit trailer

#define CS_RF_RX_1M_EXTRA_PREAMBLE_US          8             //1M PHY EXTRA PREAMBLE
#define CS_RF_RX_1M_WINDOW_EXTEND_US           15            //1M PHY WINDOW EXTEND, need at least 6us

#define CS_RX_1M_TONE_HALF_EXCLUSION_PERIOD_US 10            //5us


#define CS_1US_CONVERT_125NS(n)                ((n) * 8 - 1) //(time + 1)*0.125us
#define CS_TX_ANT_SWITCH_EARLY_TX_ON_US        46            //46us early than tx_on


#if (HADM_PHASE_CONTINUITY | 1)
    #define RF_FCAL_MANUAL_START2DONE_TIME_US 18 // driver recommend 22, theoretical value 12.5

    #define CS_COMMON_TX_SETTLE_US            50
    #define CS_COMMON_RX_SETTLE_US            RX_SETTLE_US
    #define CS_PHASE_CON_RX_SETTLE_US         50
//  #define CS_RF_TX_SETTLE_US                  53 // driver recommend 78
//  #define CS_RF_RX_SETTLE_US                  50 // driver recommend
//  #define CS_RF_TX_SETTLE_US                  108 // only for debug
//  #define CS_RF_RX_SETTLE_US                  85 // only for debug
#else
    #define CS_RF_TX_SETTLE_US 78
    #define CS_RF_RX_SETTLE_US 50
#endif

/**
 * @brief   CS RX Data buffer length
 *          actual value to 2948 = 4(DMA_len) + 2900(IQ len: 145[CS_RFRXEN_MODE_EARLY_US + T_PM_ANT_40us + T_SW_10us + T_PM_40us + T_SW_10us + extension_slot] * 4[sample rate is 4Mhz] * 5[IQ_20_BIT]) + 44(ExtraInfo)
 *          RX buffer size must be be 16*n, due to MCU design
 *          finally value to 2976, remaining 28 Bytes for CUSTOM_DATA
 */
#if DBG_CS_SUBEVENT_ENABLE
    #define DMA_CS_RFRX_MAX_DMA_LEN 2976 //only for test
#endif

#if (DBG_CS_RX_FIFO_ENABLE)
typedef struct __attribute__((packed))
{
    u16 size;
    u8  num;
    u8  wptr;
    u8  rptr;
    u8 *p_base;
} cs_fifo_t;

extern _attribute_ble_data_retention_ cs_fifo_t cs_rx_fifo_test; //for test
extern _attribute_ble_data_retention_ u8       *cs_rx_buff;      //for test
#else
extern _attribute_ble_data_retention_ u8 cs_rx_buff[]; //for test
#endif

#define CAL_LL_CS_TONE_TX_SIZE(n)        (((n) + 7) / 8)
#define CS_US_TO_IQ_LEN(n)               (((n) << 2) * 5)                  //1us ->4 sample -> 5bytes
#define CS_US_TO_IQ_SAMPLE_NUM(n)        ((n) << 2)                        //1us -> 4sample


#define LL_CS_MODE0_STEP_IQ_NUM_MAX      (80 * 2) * 4                      //T_FM:80us  // I and Q number
#define LL_CS_NONE_MODE0_STEP_IQ_NUM_MAX ((40) * 2) * 4                    //T_PM:40us
#define LL_CS_STEP_IQ_NUM_MAX            LL_CS_MODE0_STEP_IQ_NUM_MAX       // MAX(LL_CS_MODE0_STEP_IQ_NUM_MAX,LL_CS_NONE_MODE0_STEP_IQ_NUM_MAX)

#define LL_CS_STEP_IQ_LEN_MAX            ((LL_CS_STEP_IQ_NUM_MAX * 5) / 2) // IQ_NUM_MAX * 5 / 2 for IQ_20_BIT

#define LL_CS_CHANNEL_NUM_MAX            79

//CS_Step_Data_Length
#define CS_STEP_DATA_LENGTH_MODE0_INITIATOR           5
#define CS_STEP_DATA_LENGTH_MODE0_REFLECTOR           3
#define CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY         6
#define CS_STEP_DATA_LENGTH_MODE1_RTT_SOUNDING        (sizeof(cs_step_mode1_t))
#define CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1 ((sizeof(cs_step_mode2_t) + sizeof(cs_step_tone_t)) + 1 * sizeof(cs_step_tone_t)) // 5 + 1 * 4 = 9
#define CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_2 ((sizeof(cs_step_mode2_t) + sizeof(cs_step_tone_t)) + 2 * sizeof(cs_step_tone_t)) // 5 + 2 * 4 = 13
#define CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_3 ((sizeof(cs_step_mode2_t) + sizeof(cs_step_tone_t)) + 3 * sizeof(cs_step_tone_t)) // 5 + 3 * 4 = 17
#define CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_4 ((sizeof(cs_step_mode2_t) + sizeof(cs_step_tone_t)) + 4 * sizeof(cs_step_tone_t)) // 5 + 4 * 4 = 21
//enum{
//  CS_Step_Data_Length_Mode0_Initiator = 5,
//  CS_Step_Data_Length_Mode0_Reflector = 3,
//  CS_Step_Data_Length_Mode1_RTT_AA_Only = 6,
//  CS_Step_Data_Length_Mode1_RTT_Sounding = 12,
//  CS_Step_Data_Length_Mode2_Num_Antenna_Paths_1 = 9,
//  CS_Step_Data_Length_Mode2_Num_Antenna_Paths_2 = 13,
//  CS_Step_Data_Length_Mode2_Num_Antenna_Paths_3 = 17,
//  CS_Step_Data_Length_Mode2_Num_Antenna_Paths_4 = 21,
//};

//CS Step Receive Packet Quality
#define CS_STEP_RECEIVE_PACKET_QUALITY_HIGH   0 //CS Access Address check is successful, and all bits match the expected sequence
#define CS_STEP_RECEIVE_PACKET_QUALITY_MIDDLE 1 //CS Access Address check contains one or more bit errors
#define CS_STEP_RECEIVE_PACKET_QUALITY_LOW    2 //CS Access Address not found

//CS Step Receive Packet NADM
#define CS_STEP_RECEIVE_PACKET_NADM_EXTREMELY_UNLIKELY 0    //0x00 Attack is extremely unlikely
#define CS_STEP_RECEIVE_PACKET_NADM_VERY_UNLIKELY      1    //0x01 Attack is very unlikely
#define CS_STEP_RECEIVE_PACKET_NADM_UNLIKELY           2    //0x02 Attack is unlikely
#define CS_STEP_RECEIVE_PACKET_NADM_POSSIBLE           3    //0x03 Attack is possible
#define CS_STEP_RECEIVE_PACKET_NADM_LIKELY             4    //0x04 Attack is likely
#define CS_STEP_RECEIVE_PACKET_NADM_VERY_LIKELY        5    //0x05 Attack is very likely
#define CS_STEP_RECEIVE_PACKET_NADM_EXTREMELY_LIKELY   6    //0x06 Attack is extremely likely
#define CS_STEP_RECEIVE_PACKET_NADM_UNKNOWN            0xFF //0xFF Unknown NADM. Default value for RTT types that do not have a random or sounding sequence.

//CS Step Receive Tone Quality
#define CS_STEP_RECEIVE_TONE_QUALITY_GOOD        0 //Tone quality is good
#define CS_STEP_RECEIVE_TONE_QUALITY_MEDIUM      1 //Tone quality is medium
#define CS_STEP_RECEIVE_TONE_QUALITY_LOW         2 //Tone quality is low
#define CS_STEP_RECEIVE_TONE_QUALITY_UNAVAILABLE 3 //Tone quality is unavailable

//CS Subevent Result event Type
#define CS_SUBEVENT_RESULT_EVENT_FIRST    1
#define CS_SUBEVENT_RESULT_EVENT_CONTINUE 2

//CS Procedure Done Status
#define CS_PROCEDURE_DONE_STATUS_COMPLETE 0
#define CS_PROCEDURE_DONE_STATUS_PARTIAL  1
#define CS_PROCEDURE_DONE_STATUS_ABORTED  0xF

//CS Subevent Done Status
#define CS_SUBEVENT_DONE_STATUS_COMPLETE 0
#define CS_SUBEVENT_DONE_STATUS_PARTIAL  1
#define CS_SUBEVENT_DONE_STATUS_ABORTED  0xF

#define CS_ACCESS_ADDRESS_BIT_SIZE       32

#define CS_EVENT_OFFSET_MIN              500
#define CS_EVENT_OFFSET_MAX              4000 * 1000
#define CS_STEPS_PER_PROCEDURE_MAX       256

#define CS_SUBEVENT_LEN_THRESHOLD_LOW    12 * 1000 //12 ms
#define CS_SUBEVENT_LEN_THRESHOLD_HIGH   36 * 1000 //36 ms

#if (CS_EBQ_TEST)                                  // EBQ test max subevent len may reach 30ms, here set 40ms.  Notice that modify sch_early_us with large subevent len
    #define TLK_CS_SUBEVENT_MAX_LEN 45000          // 40ms
#else
    #if (DBG_CS_ONE_SUBEVENT_72CHN)
        #define TLK_CS_SUBEVENT_MAX_LEN 50000      // 50ms
    #else
        #define TLK_CS_SUBEVENT_MAX_LEN 24000      // 24ms
    #endif
#endif

#define TLK_CS_SUBINTVL_MARGIN 10000 // 10ms

extern u8 CS_tx_early_array[3];

extern const u8 ACI_to_N_AP[8];
extern const u8 RTT_Type_SeqNum[7];
extern const u8 T_IP_US[8];
extern const u8 T_FCS_US[10];
extern const u8 T_PM_US[3];

#if (DBG_CS_SUBEVENT_ENABLE)
extern _attribute_ble_data_retention_ u32 cs_procedure_start_tick;
extern _attribute_ble_data_retention_ u8  cs_mode0_rx_flag;

ble_sts_t blc_ll_initCsRxFifo_test(void);
void      blt_ll_csRxFifoUpdate(void);
void      blt_ll_cs_main_loop_test(void);

u32 ble_cs_initiator_mode0_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_IP1_us);
u32 ble_cs_reflector_mode0_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_IP1_us);
u32 ble_cs_initiator_mode1_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_IP1_us);
u32 ble_cs_reflector_mode1_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_IP1_us);
u32 ble_cs_initiator_mode2_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_PM_us, u8 T_IP2_us, u8 CS_DRBG);
u32 ble_cs_reflector_mode2_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_PM_us, u8 T_IP2_us, u8 CS_DRBG);
#endif


u16 blt_cs_calcMaxProcLenSubevtCount(cs_config_t *pCsCfg);

void blt_cs_chnMapAndOperate(u8 *chn_out, u8 *chnM, u8 *originChnMap);

void blt_le_cs_reset(void);

int blt_ll_cs_data_loop(void);


#ifndef ANTENNA_SWITCHING_AUTO_EN
    #define ANTENNA_SWITCHING_AUTO_EN 0
#endif


#if (ANTENNA_SWITCHING_AUTO_EN)

extern u32 antnenna_path_permutation_index[28];
void       blt_ll_cs_set_api(u8 role, u8 aci, u8 ant_path, u8 sync_antenna_select);

#endif


extern void blt_cs_mode0_packetSyncPDU(rf_packet_cs_mode0_t *pPkt, u32 access_code);

extern int blt_ll_cs_loop_hci_subevent(void);
///////////////////////////////// CS DEBUG LOG MACRO ///////////////////////////////
#define CS_DATA_DEBUG_LOG_EN   0 // control cs data process debug log, cs_data_proc.c
#define CS_TIMING_DEBUG_LOG_EN 0 // control cs timing debuf log, cs_initiator.c / cs_reflector.c
#define CS_SCHE_DEBUG_LOG_EN   0 // control cs scheduler debug log, cs_sche.c
#define CS_ALGO_DEBUG_LOG_EN   0 // control cs algorithm debug log

#define __FILENAME__           (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)

#endif /* STACK_BLE_CONTROLLER_LL_CHN_SOUND_CHN_SOUND_STACK_H_ */
