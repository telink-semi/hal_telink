/********************************************************************************************************
 * @file    chn_sound_stack.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#ifndef STACK_BLE_CONTROLLER_LL_CHN_SOUND_CHN_SOUND_STACK_H_
#define STACK_BLE_CONTROLLER_LL_CHN_SOUND_CHN_SOUND_STACK_H_

#define MULTIPLE_ANTENNA_EN             1


#define MAX_NUM_CS_CONFIG               1

#define     CS_IDX_MSK                  0x0F
#define     CS_IDX_FLG                  BIT(7)

#if(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_80MS)
    #define             CS_SCH_FIFONUM              16  //16*5 = 80 mS
#elif(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_120MS)
    #define             CS_SCH_FIFONUM              24  //24*5 = 120 mS
#elif(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_160MS)
    #define             CS_SCH_FIFONUM              32  //32*5 = 160 mS
#elif(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_240MS)
    #define             CS_SCH_FIFONUM              48  //48*5 = 240 mS
#else
    #error "unsupported CS_SCH FIFONUM!!!"
#endif

#ifndef     ADD_CS_MODE1_FUN
#define     ADD_CS_MODE1_FUN                    1
#endif


#define DBG_CS_LOG_SCH_MASK_EN              (0)

#define     MODE_0_T_SY_1M_US                   (8+32+4)//1 preamble + 4 access address + 4bit trailer
#define     MODE_0_T_SY_2M_US                   (MODE_0_T_SY_1M_US>>1)

#define     MODE_1_T_SY_1M_US_WITHOUT_SS_RS     MODE_0_T_SY_1M_US
#define     MODE_1_T_SY_2M_US_WITHOUT_SS_RS     (MODE_1_T_SY_1M_US_WITHOUT_SS_RS>>1)

#define     MODE_3_T_SY_1M_US_WITHOUT_SS_RS     MODE_0_T_SY_1M_US
#define     MODE_3_T_SY_2M_US_WITHOUT_SS_RS     (MODE_1_T_SY_1M_US_WITHOUT_SS_RS>>1)

#define     MODE_0_T_FM_US                      (80)
#define     T_GD_US                             (10) //independent of the LE PHY
#define     T_RD_US                             (5)


#define     STEP_NUM_PER_SUBEVENT               (16) //2^n
#define     STEP_NUM_PER_SUBEVENT_MSK           (STEP_NUM_PER_SUBEVENT-1)
#define     REMAIN_STEP_SAVE_NUM                (6)

#define     TLK_T_MES                           (150)//us


#define     SLIP_WINDOW_STEP_NUM                (64)
#define     SLIP_WINDOW_STEP_MSK                (SLIP_WINDOW_STEP_NUM-1)

#define     CHANNEL_MAP_STORE_NUM               (128)
#define     CHANNEL_MAP_STORE_MSK               (CHANNEL_MAP_STORE_NUM-1)

#define     CS_MODE0_TX_EARLY_US_1M             13 //8us(1 byte preamble) + 5us(1M PHY TXLLDLY + TXPATHDLY). now mode0 use preamble.
#define     CS_MODE2_TX_EARLY_US_1M             5  //5us(1M PHY TXLLDLY + TXPATHDLY). mode2 send Tone, not use preamble and access code.

#define     CS_MODE0_RX_EARLY_US_1M             20
#define     CS_MODE2_RX_EARLY_US_1M             5
#define     CS_MODE1_RX_EARLY_US_1M             5

enum{
    CHANNEL_SOUNDING_ROLE_INITIATOR = 0,
    CHANNEL_SOUNDING_ROLE_REFLECTOR = 1,
};

enum{
    BLE_1M_PHY = 1,
    BLE_2M_PHY = 2,
};

enum{
    NON_MODE0_CSA_3B = 0,
    NON_MODE0_CSA_3C = 1,
};

enum{
    STEP_MODE_0 = 0,
    STEP_MODE_1 = 1,    SUBMODE_TYPE_MODE_1 = 1,    MAINMODE_TYPE_MODE_1 = 1,
    STEP_MODE_2 = 2,    SUBMODE_TYPE_MODE_2 = 2,    MAINMODE_TYPE_MODE_2 = 2,
    STEP_MODE_3 = 3,    SUBMODE_TYPE_MODE_3 = 3,    MAINMODE_TYPE_MODE_3 = 3,

    SUBMODE_TYPE_MODE_UNUSED = 0xFF,
};
enum{
    RTT_Type_coarse = 0,
    RTT_Type_32bit_ss = 1,
    RTT_Type_96bit_ss = 2,
    RTT_Type_32bit_rs = 3,
    RTT_Type_64bit_rs = 4,
    RTT_Type_96bit_rs = 5,
    RTT_Type_128bit_rs = 6,
};

enum{
    CHANNEL_MAP_ALL_USED_REFRESH = BIT(7),
};

enum{
    CUR_SUBEVENT_NOT_LEAVE = 1,
    CUR_SUBEVENT_LEAVE_MAIN_SUB_MODE = 2,
    CUR_SUBEVENT_ONLY_LEAVE_SUB_MODE = 3,
};



typedef struct{
    u8      step_modeType;
    u8      step_chnIdx;
    union{
        struct{
            u8   step_extSlotRefl: 1; //bit0 indicate reflector. confirm with sunwei.
            u8   step_extSlotInit: 1; //bit1 indicate initiator. confirm with sunwei.
            u8   step_rfu:         6;
        };
        u8   step_extSlotFlag;
    }extSlot;
    u8      step_antPath;   //maybe 4 bytes size array, cause max 4 antenna path.//when subevent start, for(jumpStep){calculate}


    u8      step_initRttSeq[16];//max 128bit = 16B; either sounding sequence or random sequence.//when subevent start, for(jumpStep){calculate}
    u8      step_reflRttSeq[16];//max 128bit = 16B; either sounding sequence or random sequence.//when subevent start, for(jumpStep){calculate}

    u8      step_initRttSSPos[2]; //only sounding sequence use it. for 96 bit, max 2 mark position. only two type:32 bit and 96 bit.
    u8      step_reflRttSSPos[2]; //only sounding sequence use it. for 96 bit, max 2 mark position. only two type:32 bit and 96 bit.

    u32     step_initAA;    //when subevent start, to calculate.
    u32     step_reflAA;    //when subevent start, to calculate.

    u8      subeventEndFlag;
    u8      repetMapEndFlag;//Channel_Map_Repetition
    u8      step_rsvd[2];
}slip_window_step_t;




static inline u8 switchBitMsk2Idx(u8 bitMsk, u8 changeLen){
    for(int i=0; i<changeLen;i++){
        if( (bitMsk>>i) & BIT(0)){
            return i;
        }
    }
    return 0xFF;
}



enum{
    CS_COMPANION_SIGNAL_SUPPORT         = BIT(0),
    CS_No_FAE_SUPPORT                   = BIT(1),
    CS_CSA_3C_SUPPORT                   = BIT(2),
    CS_SOUNDING_PCT_ESTIMATE_SUPPORT    = BIT(3),
};

enum{
    RTT_AC_ONLY = 0,
    RTT_32B_SOUND_SEQUENCE = 1,
    RTT_96B_SOUND_SEQUENCE = 2,
    RTT_32B_RANDOM_SEQUENCE = 3,
//  RTT_64B_RANDOM_SEQUENCE =
};

enum{
    CS_T_TP_10US = BIT(0),
    CS_T_TP_20US = BIT(1),
    CS_T_TP_30US = BIT(2),
    CS_T_TP_40US = BIT(3),
    CS_T_TP_50US = BIT(4),
    CS_T_TP_60US = BIT(5),
    CS_T_TP_80US = BIT(6),
    CS_T_TP_145US =BIT(7),
};

enum{
    CS_T_FCS_15US = BIT(0),
    CS_T_FCS_20US = BIT(1),
    CS_T_FCS_30US = BIT(2),
    CS_T_FCS_40US = BIT(3),
    CS_T_FCS_50US = BIT(4),
    CS_T_FCS_60US = BIT(5),
    CS_T_FCS_80US = BIT(6),
    CS_T_FCS_100US = BIT(7),
    CS_T_FCS_120US = BIT(8),
    CS_T_FCS_150US = BIT(9),
};

enum{
    CS_T_PM_10US = BIT(0),
    CS_T_PM_20US = BIT(1),
    CS_T_PM_40US = BIT(2),
};

typedef struct{

    u8      Num_Config_Supported;//range 1-4
    u16     max_consecutive_procedures_supported; // range 0- 0xffff
    u8      Num_Antennas_Supported; // range 1--4

    u8      Max_Antenna_Paths_Supported; //range 1--4
    u8      Roles_Supported;// bit map,  BIT(0) initiator, BIT(1) reflector
    u8      Mode_Types; // bit map, BIT(0) mode3
    u8      RTT_Capability;

    u8      RTT_AA_Only_N;
    u8      RTT_Sounding_N;
    u8      RTT_Random_Payload_N;
    u16     Optional_NADM_Sounding_Capability;
    u16     Optional_NADM_Random_Capability;
    u8      Optional_CS_SYNC_PHYs_Supported;

    u16     Optional_Subfeatures_Supported;
    u16     Optional_T_IP1_Times_Supported;

    u16     Optional_T_IP2_Times_Supported;
    u16     Optional_T_FCS_Times_Supported;//

    u16     Optional_T_PM_Times_Supported; //bit map, BIT(0): 10us, BIT(1):20us
    u8      T_SW_Time_Supported; //0x01, 0x02, 0x04, or 0x0A
    u8      rsvd;
}chn_sound_capbilities_t;



enum{
    PROC_IDLE           = 0,
    PROC_SEND_REQ       = BIT(0),
    PROC_SEND_RSP       = BIT(1),
    PROC_WAIT_RSP       = BIT(2),
    PROC_SEND_IND       = BIT(3),
    PROC_WAIT_IND       = BIT(4),
    PROC_EVT_PENDING    = BIT(7),
};
typedef struct{

    u8 role_enable; //BIT(0): initiator, BIT(1): reflector
    u8 CS_SYNC_AntSel;// 1--4,  0xFE,in repetitive from 1--4, 0xff: not have recommendation
    s8 Max_TX_Power; // -127dbm --- -20dBm
    u8 cs_config_pend_idx;

    u8 cs_config_req;
    u8 cs_cap_req;
    u8 cs_cap_exchange;    //init set 0
    u8 cs_security_exchange;

    u8 cs_security_enable;
    u8 cs_pend_idx;
    u8 cs_req;
    u8 cs_fae_exchange;

    u8 cs_terminate_ind;
    u8 rsvd1;
    u16 rsvd2;

    u8 cs_fae_req;
    u8 cs_chn_map_ind;
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
}cs_param_t;


enum{
    CS_CONFIG_STA_DISABLE = 0,
    CS_CONFIG_STA_ENABLE = 1,
};



typedef struct{
    u8 idx;
    u16 aclHandle;
    u8 occupy;

    u8 state;
    u8 cs_procedure_para_set_en;
    u8 cs_procedure_en;
    u8 cs_procedure_measurement_en;

    u8 Config_ID;
    u8 Create_Context;
    u8 Main_Mode;
    u8 Sub_Mode;

    u8 Main_Mode_Min_Steps;// range 0x01-0xff
    u8 Main_Mode_Max_Steps;
    u8 Main_Mode_Repetition;//0--3
    u8 Mode_0_Steps; //1---3

    u8 Role;//0,1
    u8 RTT_Type;//0--6
    u8 CS_SYNC_PHY;//1:1M, 2: 2M
    u8 Channel_Map[10];
    u8 Channel_Map_Repetition;//0~3
    u8 ChSel;//0,1
    u8 Ch3c_Shape;//0,1


    u8 Chn_en_num; //channel number that mask valid with ChM
    u8 flag_endEvtInProc;//the end CS event in CS procedure
    u16 subModeStep_durUs;


    u8 Ch3c_Jump;//2--8
    u8 Companion_Signal_Enable;
    u16 connEventCount;

    u32 offset_min;
    u32 offset_max;
    u32 csOft_us;
    u32  Subevent_Len;    //in unit of microsecond and >=1250us & <4s
    u32  Min_Subevent_Len;
    u32  Max_Subevent_Len;

    u16 Max_Procedure_Len;  //unit:625us/bSlot
    u16 Procedure_Interval;
    u16 Min_Procedure_Interval;
    u16 Max_Procedure_Interval;

    u16 procMaxCount;//Procedure_Count
    u16 Event_Interval;

    u16 subEvtIntvl_625us;
    u8  Subevents_Per_Event;
    u8  aci; //Antenna Configuration Index

    u8 Tone_Antenna_Config_Selection;
    s8 Tx_Pwr_Delta;
    u8 Preferred_Peer_Ant;
    u8 PHY;

    u8 Selected_TX_Power;
    s8 cs_sub_event_oft;
    u16 allMode0Step_durUs;

    u16 mainModeStep_durUs; //used when there is no sub_mode
    u16 mainMode_num;

    u16 stepNum_curSubevent;
    u16 mode0Step_durUs;

    u16 mode1Step_durUs;
    u16 mode2Step_durUs;

    u16 mode3Step_durUs;
    u16 t_synu_us;

    u8 T_IP1;
    u8 T_IP2;
    u8 T_FCS;
    u8 T_PM;


    u32     sSlot_csSubIntvl;
    u32     bSlotEndCsEvent;  // It is updated when the initial value is set and the procedure dure_len is configured after LL_cs_ind is sent or received

    u16     csProcCount;
    /*
     * CS procedure, csStepCount shall be set to the CS step number, which starts at zero and is incremented
     * by one at each subsequent step within that procedure
     */
    u16     csStepCount; //

    u16    inst_start_proc;
    u16    cs_inst_acl;  //start acl conn event
    u32    bSlot_start_proc;


    u32     tick_proc_start;
    u32     tick_expect_csSubevent;
    u32     tick_mark_csSubevent;


    u32     seqNum_mark_csSubEvent;
    s32     sSlot_mark_csSubevent;
    u32     bSlot_mark_csSubevent;

    u32     step_expect_tick;



    u8      csTsk_wptr;
    u8      csTsk_rptr;
    u16     sSlotCsDuration;


    u8      chn_update_pend;
    u8      chn_update_inst;
    u8      mm_cnt; //unused
    u8      proc_end_flag; //submode insert



    u8      mode0ShuffledChnArray[72];    //max 72 channel, 72=0x48 align 4
    u8      nonmode0ShuffledChnArray[72]; //max 72 channel, 72=0x48 align 4

    u8      filteredChnArray[72];

    u16     mode0_chnReadIdx;
    u16     nonMode0_chnReadIdx;

    u16     csChnAvailNum; //include repetition map. mode-0 and non-mode-0 use the same variable.
    u16     mainNum_noSubMode;

    u8      slip_stepReadIdx;
    u8      slip_stepWriteIdx;
    u8      cs_procdure_1st_flag;
    u8      step_rx_flag;

    u32     mode0_tick_rx;

    u8      mode0_rx_flag;
    u8      phaseContin_config_flag;
    u8      u8_rsvd1[2];

    slip_window_step_t   slip_window_step[SLIP_WINDOW_STEP_NUM]; //according to actual situation to change value. now temporary set to 64.

    sch_task_t  csTskFifo[CS_SCH_FIFONUM];

}cs_config_t;
extern cs_config_t  *blt_pCsCfg;

typedef struct{

    u8      Num_Config_Supported;//range 1-4
    u16     max_consecutive_procedures_supported;

    u8      Num_Antennas_Supported; // range 1--4
    u8      Max_Antenna_Paths_Supported; //range 1--4
    u8      Roles_Supported; //bit map
    u8      Mode_Types; //bit map
    u8      RTT_Capability; //bit map
    u8      RTT_AA_Only_N;
    u8      RTT_Sounding_N;
    u8      RTT_Random_Payload_N;

    u16     Optional_NADM_Sounding_Capability;
    u16     Optional_NADM_Random_Capability;

    u8      Optional_CS_SYNC_PHYs_Supported;
    u16     Optional_Subfeatures_Supported;
    u16     Optional_T_IP1_Times_Supported;
    u16     Optional_T_IP2_Times_Supported;
    u16     Optional_T_FCS_Times_Supported;
    u16     Optional_T_PM_Times_Supported;
    u8      T_SW_Time_Supported;

}cs_local_support_t;


typedef struct{
    u8 max_num_cofig;
    u8 rsvd;
    u8  chn_map[10];
    u32 chn_map_upt_tick;

}cs_mng_t;


typedef struct{
    u32 dma_len;
    u8  preamble[2];
    u32 accessAddress;
    u8  trailer         :4;
    u8  shift_sequence  :4;
    u8  sequence[16];   //max 128 bits
}rf_packet_cs_t;
extern rf_packet_cs_t   pkt_CS;


typedef struct{
    u16 lenIQ;
    u16 lenSample;
    u16 startIQIdx;
    u8 tone_ext;//tone_extension_flag
    u8 d_T_SW;
    u8 d_N_AP;
    u8 d_ACI;
    u8 rsvd[2];
}cs_step_IQ_param_t;
extern cs_step_IQ_param_t   csStepIQ_param;


extern _attribute_data_retention_ u8 g_T_IP1_us; // if there are more than one config ID, g_T_IP1_us is not right. later process. todo
extern _attribute_data_retention_ u8 g_T_IP2_us; // if there are more than one config ID, g_T_IP2_us is not right. later process. todo
extern _attribute_data_retention_ u8 g_T_FCS_us;
extern _attribute_data_retention_ u8 g_t_pm_us;
extern _attribute_data_retention_ u8 g_T_SW_us;
extern _attribute_data_retention_ u8 g_antennaPathNum;
extern _attribute_data_retention_ u8 *pCsRxAddr;

extern u32 cs_tick_tx_on;
extern u8 cs_rx_agc_gain;

extern cs_config_t *gGlobal_pCsCfg;


u8 blt_ll_getCsConfigByConnHandle(u16 connHandle);
u8 blt_ll_getCsConfigByRole(u16 connHandle, cs_config_role_t role);
u8 blt_ll_getCsConfigById(u16 connHandle, u8 config_id);
u8 blt_ll_getNewCsConfig(void);
ble_sts_t blt_ll_cs_chnMapUpdateProce(void);

int blt_cs_subevent_post(unsigned char phase_en);
s32 blt_ll_cs_getStepRxFreqOffset(u8 phy, u8* raw_data);
void blt_ll_cs_getStepIQParam(u8 role, u8 step_mode, u8* raw_data, cs_step_IQ_param_t *step_param);
u8 blt_ll_cs_getPktMatchSyncQuality(u8* raw_data);
void blt_ll_cs_Convert20BitIQ2int(u8 *data_src, s32 *data_dest, u16 len_sample);
u8 blt_ll_cs_getToneQualityIndicator(float toneQualityRaw);
void blt_cs_subevent_rf_init(void);
void blt_cs_subevent_rf_deinit(unsigned char phase_en);


typedef struct {
    u16     size;
    u8      size_div_16;
    u8      rsvd;

    u8      num;
    u8      mask;
    u8      wptr;
    u8      rptr;

    u8*     p_base;
} cs_rx_fifo_t;


extern _attribute_ble_data_retention_ cs_rx_fifo_t  cs_rx_fifo;

extern chn_sound_capbilities_t bltCsLocalSupportCap;
extern cs_mng_t gCsMng;
extern u8 cs_fae_cmplt_reason;


#ifndef     DBG_CS_DATA_PRINT_EN
#define     DBG_CS_DATA_PRINT_EN                0
#endif


#ifndef     DBG_CS_DATA_USB_PRINT_EN
#define     DBG_CS_DATA_USB_PRINT_EN            0
#endif


#ifndef     DBG_CS_RX_FIFO_ENABLE
#define     DBG_CS_RX_FIFO_ENABLE               0
#endif


#ifndef     DBG_CS_SUBEVENT_ENABLE
#define     DBG_CS_SUBEVENT_ENABLE              0
#endif


#ifndef     DBG_CS_DISTANCE_STRING_PRINT_EN
#define     DBG_CS_DISTANCE_STRING_PRINT_EN     0
#endif


#ifndef     CS_ANTENNA_SWITCHING_DATA_EN
#define     CS_ANTENNA_SWITCHING_DATA_EN        1
#endif


//B92
//DAM_len(4 Bytes), no CRC
//IQ_20_BIT_data(iq_sample_number*5) + hd_normal(8 Bytes) + hd_extension(36 Bytes)
#define DMA_CS_RFRX_OFFSET_DMA_LEN              0
#define DMA_CS_RFRX_OFFSET_IQ_DATA              4
#define DMA_CS_RFRX_DMA_LEN(p)                  (p[DMA_CS_RFRX_OFFSET_DMA_LEN] + (p[DMA_CS_RFRX_OFFSET_DMA_LEN + 1] << 8))
#define DMA_CS_RFRX_IQ_DATA_LEN(p)              (DMA_CS_RFRX_DMA_LEN(p) - 44)
#define DMA_CS_RFRX_OFFSET_TIME_STAMP(p)        (DMA_CS_RFRX_OFFSET_IQ_DATA + DMA_CS_RFRX_IQ_DATA_LEN(p))
#define DMA_CS_RFRX_OFFSET_FREQ_OFFSET(p)       (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 4)
#define DMA_CS_RFRX_OFFSET_SYNC_FLAG(p)         (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 5)
#define DMA_CS_RFRX_OFFSET_RSSI(p)              (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 6)
#define DMA_CS_RFRX_OFFSET_STATUS(p)            (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 7)
#define DMA_CS_RFRX_OFFSET_IQ_START_TSTAMP(p)   (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 8)
#define DMA_CS_RFRX_OFFSET_PKT_MATCH_SYNC(p)    (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 12)
#define DMA_CS_RFRX_OFFSET_PKT_TX_POS_TSTAMP(p) (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 24)
#define DMA_CS_RFRX_OFFSET_PKT_TX_NEG_TSTAMP(p) (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 28)
//CUSTOM_DATA
#define DMA_CS_RFRX_OFFSET_CONN_HANDLE(p)                   (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 44)
#define DMA_CS_RFRX_OFFSET_CONFIG_ID_LOW4BIT(p)             (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 45)
#define DMA_CS_RFRX_OFFSET_NUM_ANTENNA_PATHS_HIGH4BIT(p)    (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 45)
#define DMA_CS_RFRX_OFFSET_START_ACL_CONN_EVENT_2BYTE(p)    (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 46)
#define DMA_CS_RFRX_OFFSET_PROCEDURE_COUNTER_2BYTE(p)       (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 48)
#define DMA_CS_RFRX_OFFSET_PROCEDURE_DONE_STATUS(p)         (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 50)
#define DMA_CS_RFRX_OFFSET_SUBEVENT_DONE_STATUS(p)          (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 51)
#define DMA_CS_RFRX_OFFSET_LAST_TX_POS_TSTAMP_4BYTE(p)      (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 52)
#define DMA_CS_RFRX_OFFSET_TX_ON_TSTAMP_4BYTE(p)            (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 56)
#define DMA_CS_RFRX_OFFSET_RX_ACCESS_ADDRESS_4BYTE(p)       (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 60)
#define DMA_CS_RFRX_OFFSET_T_SY_CENTER_DELTA_2BYTE(p)       (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 64)
#define DMA_CS_RFRX_OFFSET_RX_AGC_GAIN(p)                   (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 66)
#define DMA_CS_RFRX_OFFSET_TICK_CS_PROC_START_4BYTE(p)      (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 67)
#define DMA_CS_RFRX_OFFSET_T_SW_LOW4BIT(p)                  (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 71)
#define DMA_CS_RFRX_OFFSET_ACI_HIGH4BIT(p)                  (DMA_CS_RFRX_OFFSET_TIME_STAMP(p) + 71)

//cs_rx_buff[2]
#define BLT_CS_INITIATOR_FLAG                   BIT(7)
#define BLT_CS_REFLECTOR_FLAG                   BIT(6)
#define BLT_CS_MODE_RX_FLAG                     BIT(5)
#define BLT_CS_STEP_TONE_EXTENSION_SLOT_FLAG    BIT(4)
#define BLT_CS_MODE_3_FLAG                      BIT(3)
#define BLT_CS_MODE_2_FLAG                      BIT(2)
#define BLT_CS_MODE_1_FLAG                      BIT(1)
#define BLT_CS_MODE_0_FLAG                      BIT(0)

//cs_rx_buff[3]
#define BLT_CS_STEP_CHANNEL_MASK                0x7F//BIT(6)~BIT(0)

#define CS_RFRXEN_MODE_EARLY_US                 5   //5us(1M PHY RXPATHDLY) RXEN MODE
#define CS_RFRXEN_MODE_1M_EARLY_US              5   //5us(1M PHY RXPATHDLY) RXEN MODE
#define CS_RFRXEN_MODE_2M_EARLY_US              3   //3us(2M PHY RXPATHDLY) RXEN MODE
#define CS_RF_TX_1M_PACKET_EARLY_US             13  //8us(1 byte extra preamble) + 5us(1M PHY TXLLDLY + TXPATHDLY)
#define CS_RF_TX_2M_PACKET_EARLY_US             4   //4us(2M PHY TXLLDLY + TXPATHDLY)
#define CS_RF_TX_1M_TONE_EARLY_US               5   //5us(1M PHY TXLLDLY + TXPATHDLY)
#define CS_RF_TX_2M_TONE_EARLY_US               4   //4us(2M PHY TXLLDLY + TXPATHDLY)

#define CS_1M_PACKET_AA_ONLY_US                 44  //44us for 1M = 1 byte preamble + 4 byte access address + 4 bit trailer
#define CS_2M_PACKET_AA_ONLY_US                 26  //26us for 2M = 2 byte preamble + 4 byte access address + 4 bit trailer

#define CS_RF_RX_1M_EXTRA_PREAMBLE_US           8   //1M PHY EXTRA PREAMBLE
#define CS_RF_RX_1M_WINDOW_EXTEND_US            15  //1M PHY WINDOW EXTEND, need at least 6us

#define CS_RX_1M_TONE_HALF_EXCLUSION_PERIOD_US  5   //5us

#ifndef HADM_PHASE_CONTINUITY
    #define HADM_PHASE_CONTINUITY               1
#endif

#if(HADM_PHASE_CONTINUITY)
    #define RF_FCAL_MANUAL_START2DONE_TIME_US   18 // driver recommend 22, theoretical value 12.5

    #define CS_RF_TX_SETTLE_US                  53 // driver recommend 78
    #define CS_RF_RX_SETTLE_US                  50 // driver recommend
//  #define CS_RF_TX_SETTLE_US                  108 // only for debug
//  #define CS_RF_RX_SETTLE_US                  85 // only for debug
#else
    #define CS_RF_TX_SETTLE_US                  78
    #define CS_RF_RX_SETTLE_US                  50
#endif

/**
 * @brief   CS RX Data buffer length
 *          actual value to 2948 = 4(DMA_len) + 2900(IQ len: 145[CS_RFRXEN_MODE_EARLY_US + T_PM_ANT_40us + T_SW_10us + T_PM_40us + T_SW_10us + extension_slot] * 4[sample rate is 4Mhz] * 5[IQ_20_BIT]) + 44(ExtraInfo)
 *          RX buffer size must be be 16*n, due to MCU design
 *          finally value to 2976, remaining 28 Bytes for CUSTOM_DATA
 */
#define     DMA_CS_RFRX_MAX_DMA_LEN             2976

#if (DBG_CS_RX_FIFO_ENABLE)
    typedef struct {
        u16     size;
        u8      num;
        u8      wptr;
        u8      rptr;
        u8*     p_base;
    } cs_fifo_t;
    extern _attribute_ble_data_retention_ cs_fifo_t cs_rx_fifo_test;//for test
    extern _attribute_ble_data_retention_ u8* cs_rx_buff;//for test
#else
    extern _attribute_ble_data_retention_ u8 cs_rx_buff[];//for test
#endif

#define     CAL_LL_CS_TONE_TX_SIZE(n)           (((n) + 7) / 8)

// 640 = (T_PM_40us + extension_slot) * 4 * 2
// 640 = (T_FM_80us) * 4 * 2
#define     LL_CS_STEP_IQ_NUM_MAX               640

// IQ_NUM_MAX * 5 / 2 for IQ_20_BIT
#define     LL_CS_STEP_IQ_LEN_MAX               1600

#define     LL_CS_CHANNEL_NUM_MAX               79

//CS_Step_Data_Length
#define CS_STEP_DATA_LENGTH_MODE0_INITIATOR             5
#define CS_STEP_DATA_LENGTH_MODE0_REFLECTOR             3
#define CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY           6
#define CS_STEP_DATA_LENGTH_MODE1_RTT_SOUNDING          12
#define CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_1   9
#define CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_2   13
#define CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_3   17
#define CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_4   21
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
#define CS_STEP_RECEIVE_PACKET_QUALITY_HIGH             0   //CS Access Address check is successful, and all bits match the expected sequence
#define CS_STEP_RECEIVE_PACKET_QUALITY_MIDDLE           1   //CS Access Address check contains one or more bit errors
#define CS_STEP_RECEIVE_PACKET_QUALITY_LOW              2   //CS Access Address not found

//CS Step Receive Packet NADM
#define CS_STEP_RECEIVE_PACKET_NADM_EXTREMELY_UNLIKELY  0   //0x00 Attack is extremely unlikely
#define CS_STEP_RECEIVE_PACKET_NADM_VERY_UNLIKELY       1   //0x01 Attack is very unlikely
#define CS_STEP_RECEIVE_PACKET_NADM_UNLIKELY            2   //0x02 Attack is unlikely
#define CS_STEP_RECEIVE_PACKET_NADM_POSSIBLE            3   //0x03 Attack is possible
#define CS_STEP_RECEIVE_PACKET_NADM_LIKELY              4   //0x04 Attack is likely
#define CS_STEP_RECEIVE_PACKET_NADM_VERY_LIKELY         5   //0x05 Attack is very likely
#define CS_STEP_RECEIVE_PACKET_NADM_EXTREMELY_LIKELY    6   //0x06 Attack is extremely likely
#define CS_STEP_RECEIVE_PACKET_NADM_UNKNOWN             0xFF//0xFF Unknown NADM. Default value for RTT types that do not have a random or sounding sequence.

//CS Step Receive Tone Quality
#define CS_STEP_RECEIVE_TONE_QUALITY_GOOD               0   //Tone quality is good
#define CS_STEP_RECEIVE_TONE_QUALITY_MEDIUM             1   //Tone quality is medium
#define CS_STEP_RECEIVE_TONE_QUALITY_LOW                2   //Tone quality is low
#define CS_STEP_RECEIVE_TONE_QUALITY_UNAVAILABLE        3   //Tone quality is unavailable

//CS Subevent Result event Type
#define CS_SUBEVENT_RESULT_EVENT_FIRST                  1
#define CS_SUBEVENT_RESULT_EVENT_CONTINUE               2

//CS Procedure Done Status
#define CS_PROCEDURE_DONE_STATUS_COMPLETE               0
#define CS_PROCEDURE_DONE_STATUS_PARTIAL                1
#define CS_PROCEDURE_DONE_STATUS_ABORTED                0xF

//CS Subevent Done Status
#define CS_SUBEVENT_DONE_STATUS_COMPLETE                0
#define CS_SUBEVENT_DONE_STATUS_PARTIAL                 1
#define CS_SUBEVENT_DONE_STATUS_ABORTED                 0xF

#define CS_ACCESS_ADDRESS_BIT_SIZE                      32

#if (DBG_CS_SUBEVENT_ENABLE)
extern _attribute_ble_data_retention_ u32 cs_procedure_start_tick;
extern _attribute_ble_data_retention_ u8 cs_mode0_rx_flag;

ble_sts_t   blc_ll_initCsRxFifo_test(void);
void blt_ll_csRxFifoUpdate(void);
void blt_ll_cs_main_loop_test(void);

u32 ble_cs_initiator_mode0_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_IP1_us);
u32 ble_cs_reflector_mode0_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_IP1_us);
u32 ble_cs_initiator_mode1_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_IP1_us);
u32 ble_cs_reflector_mode1_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_IP1_us);
u32 ble_cs_initiator_mode2_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_PM_us, u8 T_IP2_us, u8 CS_DRBG);
u32 ble_cs_reflector_mode2_test(u32 tick_step_start, u32 csAccessAddr, u8 csChannel, u8 csPHY, u8 T_FCS_us, u8 T_PM_us, u8 T_IP2_us, u8 CS_DRBG);
#endif


#endif /* STACK_BLE_CONTROLLER_LL_CHN_SOUND_CHN_SOUND_STACK_H_ */
