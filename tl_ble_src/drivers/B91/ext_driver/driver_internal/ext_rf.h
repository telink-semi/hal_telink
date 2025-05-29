/********************************************************************************************************
 * @file    ext_rf.h
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
#include "types.h"

#ifndef DRIVERS_B91_EXT_DRIVER_DRIVER_LIB_EXT_RF_H_
#define DRIVERS_B91_EXT_DRIVER_DRIVER_LIB_EXT_RF_H_


typedef struct {
    unsigned char  rfMode_init_flag;
    unsigned char  txPower_index;
    unsigned char  txPower_level;   /*!< added to be compatible with driver api */
}ext_rf_t;

extern ext_rf_t blt_extRF;




void rf_mode_optimize_init(void);



/******************************* ext_aoa start ******************************************************************/
/*
 * @brief  Enable AOA or AOD in different packet formats.
 */
typedef enum{
    RF_RX_ACL_AOA_EN  = BIT(0),
    RF_RX_ADV_AOA_EN  = BIT(1),
    RF_TX_ACL_AOA_EN  = BIT(2),
    RF_TX_ADV_AOA_EN  = BIT(3),
    RF_AOA_OFF        = 0
}rf_aoa_aod_mode_e;

/**
 * @brief       This function serve to set the antenna switch sequence.
 * @param[in]   ant_num - the number of antenna.
 * @return      none.
 */
void set_antenna_num(unsigned char ant_num);

void triangle_all_open(void);

/**
 * @brief       This function enables the sending and receiving functions of AOA/AOD in ordinary format packets or ADV format packets.
 * @param[in]   mode - AOA/AOD broadcast package or normal package trx mode.
 * @return      none.
 */
void rf_set_aoa_aod_trx_mode(rf_aoa_aod_mode_e mode);

/**
 * @brief       This function is used to calibrate AOA, AOD sampling frequency offset.
 * @param[in]   sample_locate - The default value is 0x2d, add 1 means sample point delay 0.125us.
 * @return      none.
 */
void rf_adjust_ant_sample_offset(unsigned char sample_locate);

/**
 * @brief       This function is used to set the AOD/AOA sampling time.
 * @param[in]   sample_time - The default value is 0x2d, add 1 means sample point delay 0.125us.
 * @return      none.
 * @note        Attention:It should be noted that in normal mode, AOD can set 1us or 2us slot time according to CTE information,
 *              while AOA is 2us slot in normal mode; at the same time, AOD can be manually set to 1us slot through this function.
 */
void aoa_set_sample_slot_time(sample_slot_time_e sample_time);
/******************************* ext_aoa end ********************************************************************/



/******************************* ext_rf start ******************************************************************/

#define DMA_RFRX_LEN_HW_INFO                0   // 826x: 8
#define DMA_RFRX_OFFSET_HEADER              4   // 826x: 12
#define DMA_RFRX_OFFSET_RFLEN               5   // 826x: 13
#define DMA_RFRX_OFFSET_DATA                6   // 826x: 14

#define DMA_RFRX_OFFSET_CRC24(p)            (p[DMA_RFRX_OFFSET_RFLEN]+6)  //data len:3
#define DMA_RFRX_OFFSET_TIME_STAMP(p)       (p[DMA_RFRX_OFFSET_RFLEN]+9)  //data len:4
#define DMA_RFRX_OFFSET_FREQ_OFFSET(p)      (p[DMA_RFRX_OFFSET_RFLEN]+13) //data len:2
#define DMA_RFRX_OFFSET_RSSI(p)             (p[DMA_RFRX_OFFSET_RFLEN]+15) //data len:1, signed
#define DMA_RFRX_OFFSET_STATUS(p)           (p[DMA_RFRX_OFFSET_RFLEN]+16)

#define RF_BLE_RF_PAYLOAD_LENGTH_OK(p)                  (p[5] <= reg_rf_rxtmaxlen)
#define RF_BLE_RF_PACKET_CRC_OK(p)                      ((p[p[5]+5+11] & 0x01) == 0x0)
#define RF_BLE_PACKET_VALIDITY_CHECK(p)                 (RF_BLE_RF_PAYLOAD_LENGTH_OK(p) && RF_BLE_RF_PACKET_CRC_OK(p))


#define rf_set_tx_packet_address(addr)      (dma_set_src_address(DMA0, convert_ram_addr_cpu2bus(addr)))



#ifndef RF_ACCESS_CODE_DEFAULT_THRESHOLD
#define RF_ACCESS_CODE_DEFAULT_THRESHOLD    (31)    //0x1e  . BQB may use 32. Coded PHY may use 0xF0
#endif


//RF BLE Minimum TX Power LVL (unit: 1dBm)
extern const char  ble_rf_min_tx_pwr;
//RF BLE Maximum TX Power LVL (unit: 1dBm)
extern const char  ble_rf_max_tx_pwr;
//RF BLE Current TX Path Compensation
extern  signed short ble_rf_tx_path_comp;
//RF BLE Current RX Path Compensation
extern  signed short ble_rf_rx_path_comp;
//Current RF RX DMA buffer point for BLE
extern  unsigned char *ble_curr_rx_dma_buff;


typedef enum {
     RF_ACC_CODE_TRIGGER_AUTO   =    BIT(0),    /**< auto trigger */
     RF_ACC_CODE_TRIGGER_MANU   =    BIT(1),    /**< manual trigger */
} rf_acc_trigger_mode;


#if (SW_DCOC_EN)
/**
 *  @brief  DCOC calibration value
 */
typedef struct
{
    unsigned char DCOC_IDAC;
    unsigned char DCOC_QDAC;
}rf_dcoc_iq_dac_t;
#endif


extern signed char ble_txPowerLevel;

_attribute_ram_code_ void ble_rf_set_rx_dma(unsigned char *buff, unsigned char fifo_byte_size);

_attribute_ram_code_ void ble_rf_set_tx_dma(unsigned char fifo_dep, unsigned char fifo_byte_size);

void ble_rx_dma_config(void);



/**
 * @brief   This function serves to settle adjust for RF Tx.This function for adjust the differ time
 *          when rx_dly enable.
 * @param   txstl_us - adjust TX settle time.
 * @return  none
 */
__INLINE void  rf_ble_set_tx_settle(unsigned short txstl_us)
{
    REG_ADDR16(0x80140a04) = txstl_us;
}

__INLINE unsigned short rf_read_tx_settle(void)
{
    return REG_ADDR16(0x80140a04);
}


/* attention that not bigger than 4095 */
__INLINE void rf_ble_set_rx_settle( unsigned short rx_stl_us )
{
     write_reg16(0x140a0c, rx_stl_us);
}


__INLINE void  rf_ble_set_rx_wait(unsigned short rx_wait_us)
{
    write_reg8(0x80140a06, rx_wait_us);
}

__INLINE void  rf_ble_set_tx_wait(unsigned short tx_wait_us)
{
    tx_wait_us = (tx_wait_us>0x0fff ? 0x0fff : tx_wait_us);
    write_reg16(0x80140a0e, tx_wait_us);
}

/**
*   @brief     This function serves to reset RF BaseBand
*   @param[in] none.
*   @return    none.
*/
__INLINE void rf_reset_baseband(void)
{
    REG_ADDR8(0x801404e3) = 0;      //rf_reset_baseband,rf reg need re-setting
    REG_ADDR8(0x801404e3) = BIT(0);         //release reset signal
}



/**
 * @brief     This function performs to set RF Access Code Threshold.// use for BQB
 * @param[in] threshold   cans be 0-32bits
 * @return    none.
 */
__INLINE void rf_ble_set_access_code_threshold(u8 threshold)
{
    write_reg8(0x140c4e, threshold);
}



/**
 * @brief   This function serves to set RF access code value.
 * @param[in]   ac - the address value.
 * @return  none
 */
__INLINE void rf_set_ble_access_code_value (unsigned int ac)
{
    write_reg32 (0x80140808, ac);
}

/**
 * @brief   This function serves to set RF access code.
 * @param[in]   p - the address to access.
 * @return  none
 */
__INLINE void rf_set_ble_access_code (unsigned char *p)
{
    write_reg32 (0x80140808, p[3] | (p[2]<<8) | (p[1]<<16) | (p[0]<<24));
}


/**
 * @brief   This function serves to set RF access code advantage.
 * @param   none.
 * @return  none.
 */
__INLINE void rf_set_ble_access_code_adv (void)
{
    write_reg32 (0x0140808, 0xd6be898e);
}

/**
 * @brief   This function serves to reset function for RF.
 * @param   none
 * @return  none
 *******************need driver change
 */
__INLINE void reset_sn_nesn(void)
{
    REG_ADDR8(0x80140a01) =  0x01;
}




/**
 * @brief   This function serves to trigger accesscode in coded Phy mode.
 * @param   none.
 * @return  none.
 */
__INLINE void rf_trigger_codedPhy_accesscode(void)
{
    write_reg8(0x140c25,read_reg8(0x140c25)|0x01);
}

/**
 * @brief     This function performs to enable RF Tx.
 * @param[in] none.
 * @return    none.
 */
__INLINE void rf_ble_tx_on ()
{
    write_reg8  (0x80140a02, 0x45 | BIT(4));    // TX enable
}

/**
 * @brief     This function performs to done RF Tx.
 * @param[in] none.
 * @return    none.
 */
__INLINE void rf_ble_tx_done ()
{
    write_reg8  (0x80140a02, 0x45);
}


/**
 * @brief   This function serves to set RX first timeout value.
 * @param[in]   tm - timeout, unit: uS.
 * @return  none.
 */
__INLINE void rf_set_1st_rx_timeout(unsigned int tm)
{
    reg_rf_ll_rx_fst_timeout = tm;
}


/**
 * @brief   This function serves to set RX timeout value.
 * @param[in]   tm - timeout, unit: uS, range: 0 ~ 0xfff
 * @return  none.
 */
__INLINE void rf_ble_set_rx_timeout(u16 tm)
{
    reg_rf_rx_timeout = tm;
}

/**
 * @brief   This function serve to set the length of preamble for BLE packet.
 * @param[in]   len     -The value of preamble length.Set the register bit<0>~bit<4>.
 * @return      none
 */
__INLINE void rf_ble_set_preamble_len(u8 len)
{
    write_reg8(0x140802,(read_reg8(0x140802)&0xe0)|(len&0x1f));
}

__INLINE int rf_ble_get_preamble_len(void)
{
    return (read_reg8(0x140802)&0x1f); //[4:0]: BLE preamble length
}

typedef enum{
    FSM_BTX     = 0x81,
    FSM_BRX     = 0x82,
    FSM_STX     = 0x85,
    FSM_SRX     = 0x86,
    FSM_TX2RX   = 0x87,
    FSM_RX2TX   = 0x88,
}fsm_mode_e;


/**
 * @brief       This function serves to RF trigger RF state machine.
 * @param[in]   mode  - FSM mode.
 * @param[in]   tx_addr  - DMA TX buffer, if not TX, must be "NULL"
 * @param[in]   tick  - FAM Trigger tick.
 * @return      none.
 */
void rf_start_fsm(fsm_mode_e mode, void* tx_addr, unsigned int tick);

/**
 * @brief      This function serves to reset baseband.this function is same as driver rf_emi_reset_baseband.
 * but rf_emi_reset_baseband is not ram code.
 * @return     none
 */
void ble_rf_reset_baseband(void);

/**
 * @brief       This function serves to set RF baseband channel.This function is suitable for ble open PN mode.
 * @param[in]   chn_num  - Bluetooth channel set according to Bluetooth protocol standard.
 * @return      none.
 */
void rf_set_ble_channel (signed char chn_num);


/**
 * @brief     This function performs to switch PHY test mode.
 * @param[in] mode - PHY mode
 * @return    none.
 */
void rf_switchPhyTestMode(rf_mode_e mode);



/*
 * brief:If already know the DMA length value,this API can calculate the real RF length value that is easier for humans to understand.
 * param: dma_len -- the value calculated by this macro "rf_tx_packet_dma_len"
 * return: 0xFFFFFFFE --- error;
 *         other value--- success;
 */
u32 rf_cal_rfLenFromDmaLen(u32 dma_len);


//TODO: merge into driver
enum{
    FLD_RF_SN                     = BIT(0),
};







/**
 * @brief    This function serves to enable zb_rt interrupt source.
 * @return  none
 */
__INLINE void zb_rt_irq_enable(void)
{
    plic_interrupt_enable(IRQ_ZB_RT);
}




#define RF_RX_WAIT_MIN_VALUE            (0)
#define RF_TX_WAIT_MIN_VALUE            (0)


#ifndef FAST_SETTLE
#define FAST_SETTLE         1
#endif

#define TX_FAST_SETTLE_LEVEL  TX_SETTLE_TIME_50US
#define RX_FAST_SETTLE_LEVEL  RX_SETTLE_TIME_45US

#define TX_FAST_SETTLE_TIME  50
#define RX_FAST_SETTLE_TIME  45

#ifndef BLE_S2_S8_NEW_PATH
#define BLE_S2_S8_NEW_PATH      0
#endif



/*
 * SiHui & QingHua & SunWei sync with Xuqiang.Zhang & Zhiwei.Wang & Kaixin.Chen & Shujuan.chu
 * B91/B92
 * TX settle recommend value by digital team: 108.5uS without fast settle;   108.5-58.5=50 with fast settle
 * we BLE use 110 without fast settle; 110-57=53 with fast settle, here 53 = real settle 45uS + extra 1 preamble 8uS(1M for example)
 *
 * RX settle recommend value by digital team: 108.5uS without fast settle;   85-40=45 with fast settle
 */

#if (FAST_SETTLE)
    #define RX_SETTLE_US                    RX_FAST_SETTLE_TIME
#else
    #define RX_SETTLE_US                    85
#endif


#define RXSET_OPTM_ANTI_INTRF               100  //RX settle value for optimize anti-interference


#if (FAST_SETTLE)
    #define PRMBL_LENGTH_1M                 2   //preamble length for 1M PHY
    #define PRMBL_LENGTH_2M                 3   //preamble length for 2M PHY
#else
    #define PRMBL_LENGTH_1M                 6   //preamble length for 1M PHY
    #define PRMBL_LENGTH_2M                 6   //preamble length for 2M PHY
#endif
#define PRMBL_LENGTH_Coded                  10  //preamble length for Coded PHY, never change this value !!!

#define PRMBL_EXTRA_1M                      (PRMBL_LENGTH_1M - 1)   // 1 byte for 1M PHY
#define PRMBL_EXTRA_2M                      (PRMBL_LENGTH_2M - 2)   // 2 byte for 2M
#define PRMBL_EXTRA_Coded                   0                       // 10byte for Coded, 80uS, no extra byte

#if RF_RX_SHORT_MODE_EN//open rx dly
//TX settle time

    #if (FAST_SETTLE)
        #define         TX_STL_LEGADV_SCANRSP_REAL                  TX_FAST_SETTLE_TIME  //can change, consider TX packet quality
    #else
        #define         TX_STL_LEGADV_SCANRSP_REAL                  110 //can change, consider TX packet quality
    #endif

    #define         TX_STL_LEGADV_SCANRSP_SET                       (TX_STL_LEGADV_SCANRSP_REAL - PRMBL_EXTRA_1M * 8)  //can not change !!!




    #if (FAST_SETTLE)
        #define         TX_STL_TIFS_REAL_COMMON                     TX_FAST_SETTLE_TIME  //can change, consider TX packet quality
    #else
        #define         TX_STL_TIFS_REAL_COMMON                     110 //can change, consider TX packet quality
    #endif

    #define         TX_STL_TIFS_REAL_1M                             TX_STL_TIFS_REAL_COMMON  //can not change !!!
    #define         TX_STL_TIFS_SET_1M                              (TX_STL_TIFS_REAL_1M - PRMBL_EXTRA_1M * 8)  //can not change !!!

    #define         TX_STL_TIFS_REAL_2M                             TX_STL_TIFS_REAL_COMMON  //can not change !!!
    #define         TX_STL_TIFS_SET_2M                              (TX_STL_TIFS_REAL_2M - PRMBL_EXTRA_2M * 4)  //can not change !!!

    #define         TX_STL_TIFS_REAL_CODED                          TX_STL_TIFS_REAL_COMMON  //can not change !!!
    #define         TX_STL_TIFS_SET_CODED                           TX_STL_TIFS_REAL_CODED   //can not change !!!





    #if (FAST_SETTLE)
        #define         TX_STL_ADV_REAL_COMMON                      TX_FAST_SETTLE_TIME  //can change, consider TX packet quality
    #else
        #define         TX_STL_ADV_REAL_COMMON                      110 //can change, consider TX packet quality
    #endif

    #define         TX_STL_ADV_REAL_1M                              TX_STL_ADV_REAL_COMMON  //can not change !!!
    #define         TX_STL_ADV_SET_1M                               (TX_STL_ADV_REAL_1M - PRMBL_EXTRA_1M * 8)  //can not change !!!

    #define         TX_STL_ADV_REAL_2M                              TX_STL_ADV_REAL_COMMON  //can not change !!!
    #define         TX_STL_ADV_SET_2M                               (TX_STL_ADV_REAL_2M - PRMBL_EXTRA_2M * 4)  //can not change !!!

    #define         TX_STL_ADV_REAL_CODED                           TX_STL_ADV_REAL_COMMON  //can not change !!!
    #define         TX_STL_ADV_SET_CODED                            TX_STL_ADV_REAL_CODED  //can not change !!!




    /* TX settle in auto mode, include TX in BRX & TX except first in BTX, main purpose is to make up TIFS 150uS
     * 1. no need concern TX packet quality guaranteed by TX settle time, actually TX settle time very enough
     * 2. no need consider fast settle
     * */
    #if (SW_DCOC_EN)
        //after enable SW_DCOC, TODO change to reasonable value
        #define         TX_STL_AUTO_MODE_1M                         (127 - PRMBL_EXTRA_1M * 8)
        #define         TX_STL_AUTO_MODE_2M                         (133 - PRMBL_EXTRA_2M * 4)
        #define         TX_STL_AUTO_MODE_CODED                      123 //after enable DCOC, ACL T_IFS is about 152.5us, minus 2 us delay by lihaojie at 2024-05-11
    #else
        #define         TX_STL_AUTO_MODE_1M                         (127 - PRMBL_EXTRA_1M * 8)
        #define         TX_STL_AUTO_MODE_2M                         (133 - PRMBL_EXTRA_2M * 4)
        #define         TX_STL_AUTO_MODE_CODED                      125
    #endif





    #if (FAST_SETTLE)
        #define     TX_STL_BTX_1ST_PKT_REAL                         TX_FAST_SETTLE_TIME
    #else
        /* normal mode(no fast settle): for ACL Central, tx settle real 110uS or 107uS or even 105uS, not big difference,
         * but for CIS Central, timing is very urgent considering T_MSS between two sub_event, so SiHui use 107, we keep this set
         * fast settle mode:  */
        #define     TX_STL_BTX_1ST_PKT_REAL                         (110 - 3) //3 is total switch delay time
    #endif

    #define         TX_STL_BTX_1ST_PKT_SET_1M                       (TX_STL_BTX_1ST_PKT_REAL - PRMBL_EXTRA_1M * 8)
    #define         TX_STL_BTX_1ST_PKT_SET_2M                       (TX_STL_BTX_1ST_PKT_REAL - PRMBL_EXTRA_2M * 4)
    #define         TX_STL_BTX_1ST_PKT_SET_CODED                    TX_STL_BTX_1ST_PKT_REAL
#else// close rx dly
    #error "add code here"
#endif


#if(1) //(LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)
    #define         RX_PATH_DLY_EXTRA_PREAMBLE_1M                   (150 - TX_STL_AUTO_MODE_1M)
    #define         RX_PATH_DLY_EXTRA_PREAMBLE_2M                   (150 - TX_STL_AUTO_MODE_2M)

    #if(BLE_S2_S8_NEW_PATH)
        #define         RX_PATH_DLY_EXTRA_PREAMBLE_S2               (150-TX_STL_AUTO_MODE_CODED_S2)
        #define         RX_PATH_DLY_EXTRA_PREAMBLE_S8               (150-TX_STL_AUTO_MODE_CODED_S8)
    #else
        #define         RX_PATH_DLY_EXTRA_PREAMBLE_CODED            (150- TX_STL_AUTO_MODE_CODED)
    #endif
#endif

/* AD convert delay : timing cost on RF analog to digital convert signal process:
 *                  Eagle   1M: 20uS       2M: 10uS;      500K(S2): 14uS    125K(S8):  14uS
 *                  Jaguar  1M: 20uS       2M: 10uS;      500K(S2): 14uS    125K(S8):  14uS
 */
#define AD_CONVERT_DLY_1M                                           19//before:20
#define AD_CONVERT_DLY_2M                                           12//before:10
#define AD_CONVERT_DLY_CODED                                        17//before:14

#if (SW_DCOC_EN)
    //after enable SW_DCOC, TODO change to reasonable value
    #define OTHER_SWITCH_DELAY_1M                                   0
    #define OTHER_SWITCH_DELAY_2M                                   0
    #define OTHER_SWITCH_DELAY_CODED                                2 //after enable DCOC, ADV T_IFS is about 152.5us, add 2 us delay by lihaojie at 2024-05-11
#else
    #define OTHER_SWITCH_DELAY_1M                                   0
    #define OTHER_SWITCH_DELAY_2M                                   0
    #define OTHER_SWITCH_DELAY_CODED                                0
#endif


#define HW_DELAY_1M                                                 (AD_CONVERT_DLY_1M + OTHER_SWITCH_DELAY_1M)
#define HW_DELAY_2M                                                 (AD_CONVERT_DLY_2M + OTHER_SWITCH_DELAY_2M)
#define HW_DELAY_CODED                                              (AD_CONVERT_DLY_CODED + OTHER_SWITCH_DELAY_CODED)



__INLINE void rf_ble_set_1m_phy(void)
{
    write_reg8(0x140e3d,0x61);
    write_reg32(0x140e20,0x23200a16);
    write_reg8(0x140c20,0x8c);// script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                              //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                              //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22,0x00);
    write_reg8(0x140c4d,0x01);
    write_reg8(0x140c4e,RF_ACCESS_CODE_DEFAULT_THRESHOLD);
    write_reg16(0x140c36,0x0eb7);
    write_reg16(0x140c38,0x71c4);
    write_reg8(0x140c73,0x01);
    write_reg8(0x140c9a,0x00);          //default:0x08;tx_tp_align

    write_reg16(0x140cc2,0x4b39);
    write_reg32(0x140cc4,0x796e6256);

    #if 1
        write_reg32(0x140800,0x4440081f | PRMBL_LENGTH_1M<<16);
    #else
        write_reg32(0x140800,0x4446081f);
    #endif

    write_reg16(0x140804,0x04f5);

    #if (SW_DCOC_EN)
        //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
        //Restore the secondary filter to initial state (turn off)
        write_reg8(0x140e7a,read_reg8(0x140e7a)|0x20);//bit<5>:BYPASS_RRC_BLE   default 1, Turn off the secondary filter
    #endif
}


__INLINE void rf_ble_set_2m_phy(void)
{
    write_reg8(0x140e3d,0x41);
    write_reg32(0x140e20,0x26432a06);
    write_reg8(0x140c20,0x8c);// script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                              //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                              //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22,0x01);
    write_reg8(0x140c4d,0x01);
    write_reg8(0x140c4e,RF_ACCESS_CODE_DEFAULT_THRESHOLD);
    write_reg16(0x140c36,0x0eb7);
    write_reg16(0x140c38,0x71c4);
    write_reg8(0x140c73,0x01);
    write_reg8(0x140c9a,0x00);          //default:0x08;tx_tp_align

    write_reg16(0x140cc2,0x4c3b);
    write_reg32(0x140cc4,0x7a706458);

    #if 1
        write_reg32(0x140800,0x4440081f | PRMBL_LENGTH_2M<<16);
    #else
        write_reg32(0x140800,0x4446081f);
    #endif

    write_reg16(0x140804,0x04e5);

    #if (SW_DCOC_EN)
        //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
        //Restore the secondary filter to initial state (turn off)
        write_reg8(0x140e7a,read_reg8(0x140e7a)|0x20);//bit<5>:BYPASS_RRC_BLE   default 1, Turn off the secondary filter
    #endif
}




__INLINE void rf_ble_set_coded_phy_common(void)
{
    write_reg8(0x140e3d,0x61);
    write_reg32(0x140e20,0x23200a16);
    write_reg8(0x140c20,0x8d);// script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                              //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                              //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22,0x00);
    write_reg8(0x140c4d,0x01);
    write_reg8(0x140c4e,0xf0);
    write_reg16(0x140c38,0x7dc8);
    write_reg8(0x140c73,0x21);
    write_reg8(0x140c9a,0x00);          //default:0x08;tx_tp_align

    write_reg16(0x140cc2,0x4836);
    write_reg32(0x140cc4,0x796e6254);

    #if 1
        write_reg32(0x140800,0x4440081f | PRMBL_LENGTH_Coded<<16);
    #else
        write_reg32(0x140800,0x444a081f);
    #endif

    #if (SW_DCOC_EN)
        //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
        //But this will lead to a narrowing of the RX packet receiving bandwidth and a decrease in frequency offset performance.(Modified by chenxi,confirmed by yuya at 20240407)
        write_reg8(0x140e7a,read_reg8(0x140e7a)&0xdf);//bit<5>:BYPASS_RRC_BLE   default 1,->0 Turn on the secondary filter
    #endif
}


__INLINE void rf_ble_set_coded_phy_s2(void)
{
    write_reg16(0x140c36,0x0cee);
    write_reg16(0x140804,0xa4f5);
}


__INLINE void rf_ble_set_coded_phy_s8(void)
{
    write_reg16(0x140c36,0x0cf6);
    write_reg16(0x140804,0xb4f5);
}

__INLINE u8 rf_ble_get_tx_pwr_idx(s8 rfTxPower)
{
    rf_power_level_index_e rfPwrLvlIdx;

    /*VBAT*/
    if      (rfTxPower >=   9)  {  rfPwrLvlIdx = RF_POWER_INDEX_P9p11dBm;  }
    else if (rfTxPower >=   8)  {  rfPwrLvlIdx = RF_POWER_INDEX_P8p05dBm;  }
    else if (rfTxPower >=   7)  {  rfPwrLvlIdx = RF_POWER_INDEX_P6p98dBm;  }
    else if (rfTxPower >=   6)  {  rfPwrLvlIdx = RF_POWER_INDEX_P5p68dBm;  }
    /*VANT*/
    else if (rfTxPower >=   5)  {  rfPwrLvlIdx = RF_POWER_INDEX_P4p35dBm;  }
    else if (rfTxPower >=   4)  {  rfPwrLvlIdx = RF_POWER_INDEX_P3p83dBm;  }
    else if (rfTxPower >=   3)  {  rfPwrLvlIdx = RF_POWER_INDEX_P3p25dBm;  }
    else if (rfTxPower >=   2)  {  rfPwrLvlIdx = RF_POWER_INDEX_P2p32dBm;  }
    else if (rfTxPower >=   1)  {  rfPwrLvlIdx = RF_POWER_INDEX_P0p80dBm;  }
    else if (rfTxPower >=   0)  {  rfPwrLvlIdx = RF_POWER_INDEX_P0p01dBm;  }
    else if (rfTxPower >=  -4)  {  rfPwrLvlIdx = RF_POWER_INDEX_N3p37dBm;  }
    else if (rfTxPower >=  -8)  {  rfPwrLvlIdx = RF_POWER_INDEX_N8p78dBm;  }
    else if (rfTxPower >= -12)  {  rfPwrLvlIdx = RF_POWER_INDEX_N12p06dBm; }
    else if (rfTxPower >= -18)  {  rfPwrLvlIdx = RF_POWER_INDEX_N17p83dBm; }
    else                        {  rfPwrLvlIdx = RF_POWER_INDEX_N23p54dBm; }

    return rfPwrLvlIdx;
}

__INLINE s8 rf_ble_get_tx_pwr_level(rf_power_level_index_e rfPwrLvlIdx)
{
    s8 rfTxPower;

    /*VBAT*/
    if      (rfPwrLvlIdx <= RF_POWER_INDEX_P9p11dBm)  {  rfTxPower =   9;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P8p05dBm)  {  rfTxPower =   8;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P6p98dBm)  {  rfTxPower =   7;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P5p68dBm)  {  rfTxPower =   6;  }
    /*VANT*/
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P4p35dBm)  {  rfTxPower =   5;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P3p83dBm)  {  rfTxPower =   4;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P3p25dBm)  {  rfTxPower =   3;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P2p32dBm)  {  rfTxPower =   2;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P0p80dBm)  {  rfTxPower =   1;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P0p01dBm)  {  rfTxPower =   0;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_N3p37dBm)  {  rfTxPower =  -4;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_N8p78dBm)  {  rfTxPower =  -8;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_N12p06dBm) {  rfTxPower = -12;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_N17p83dBm) {  rfTxPower = -18;  }
    else                                              {  rfTxPower = -23;  }

    return rfTxPower;
}


#if (SW_DCOC_EN)
/**
 * @brief   This function serves to get agc gain.
 * @return  agc gain(3 bit)
 */
unsigned char rf_get_agc_gain(void);

/**
 *  @brief      This function is mainly used to get dcoc dac values.
 *  @param[in]  dcoc_cal   - dcoc calibration value address pointer
 *  @return     none
*/
void rf_get_dcoc_dac_val(rf_dcoc_iq_dac_t *dcoc_cal);

/**
 *  @brief      This function is used to get ADC IQ values.
 *  @return     ADC IQ values.
*/
#endif









#if FAST_SETTLE

    typedef struct
    {
        unsigned short cal_tbl[81];
        rf_ldo_trim_t  ldo_trim;
        unsigned char tx_fast_en;
        unsigned char rx_fast_en;
    }Fast_Settle;

    extern Fast_Settle fast_settle_1M;
    extern Fast_Settle fast_settle_2M;
    extern Fast_Settle fast_settle_S2;
    extern Fast_Settle fast_settle_S8;

    void ble_rf_tx_fast_settle(void);
    void ble_rf_rx_fast_settle(void);
    unsigned short get_rf_hpmc_cal_val(void);
    void set_rf_hpmc_cal_val(unsigned short value);
    unsigned char ble_is_rf_tx_fast_settle_en(void);
    unsigned char ble_is_rf_rx_fast_settle_en(void);
    void get_ldo_trim_val(u8* p);
    void set_ldo_trim_val(u8* p);
    void set_ldo_trim_on(void);

    /**
     *  @brief      this function serve to enable the tx timing sequence adjusted.
     *  @param[in]  none
     *  @return     none
    */
    void ble_rf_tx_fast_settle_en(void);

    /**
     *  @brief      this function serve to disable the tx timing sequence adjusted.
     *  @param[in]  none
     *  @return     none
    */
    void ble_rf_tx_fast_settle_dis(void);


    /**
     *  @brief      this function serve to enable the rx timing sequence adjusted.
     *  @param[in]  none
     *  @return     none
    */
    void ble_rf_rx_fast_settle_en(void);


    /**
     *  @brief      this function serve to disable the rx timing sequence adjusted.
     *  @param[in]  none
     *  @return     none
    */
    void ble_rf_rx_fast_settle_dis(void);



#endif



/******************************* ext_rf end ********************************************************************/

#endif /* DRIVERS_B91_EXT_DRIVER_DRIVER_LIB_EXT_RF_H_ */
