/********************************************************************************************************
 * @file    ota.h
 *
 * @brief   This is the header file for 2.4G SDK
 *
 * @author  2.4G Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#ifndef _OTA_H_
#define _OTA_H_

#define OTA_SLAVE_BIN_ADDR_0x40000        0x40000
#define OTA_SLAVE_BIN_ADDR_0x20000        0x20000
#define OTA_SLAVE_BIN_ADDR_0x00           0x00

#define OTA_SLAVE_BIN_ADDR_1              OTA_SLAVE_BIN_ADDR_0x00
#define OTA_SLAVE_BIN_ADDR_2              OTA_SLAVE_BIN_ADDR_0x20000
#define OTA_MASTER_BIN_ADDR_1             0x20000
#define OTA_MASTER_BIN_ADDR_2             0x40000

#define OTA_FW_VERSION_ADDR_OFFSET        0x02
#define OTA_BIN_SIZE_OFFSET               0x18
#define OTA_REBOOT_WAIT                   (1000 * 1000) //in us
#define OTA_BOOT_FlAG_OFFSET              0x20
#define OTA_BOOT_FlAG_LEN                 1

#define OTA_FRAME_TYPE_CMD                0x01
#define OTA_FRAME_TYPE_DATA               0x02
#define OTA_FRAME_TYPE_ACK                0x03

#define OTA_CMD_ID_START_REQ              0x01
#define OTA_CMD_ID_START_RSP              0x02
#define OTA_CMD_ID_END_REQ                0x03
#define OTA_CMD_ID_END_RSP                0x04
#define OTA_CMD_ID_VERSION_REQ            0x05
#define OTA_CMD_ID_VERSION_RSP            0x06
#define OTA_CMD_ID_DATA_LOSS_REQ          0x07
#define OTA_CMD_ID_DATA_LOSS_RSP          0x08

#define OTA_CMD_ID_BATCH_START_REQ        0x10


#define DATA_BROADCAST_CNT                (10)
#define DATA_LOSS_CHECK_CNT               (21)

#define OTA_START_DURATION                (20 * 1000 * 1000)
#define OTA_LOSS_CHECK_DURATION           (2 * 1000 * 1000)
#define OTA_MASTER_LISTENING_DURATION     (5 * 1000 * 1000)  //in us
#define OTA_PROCESS_DURATION              (40 * 1000 * 1000) //zewen

#define OTA_WAIT_BATCH_OTA_START_DURATION (30 * 1000 * 1000) //zewen


#define MAC_FLASH_ADDR                    (0X76000)


#define OTA_DATA_CHANNEL                  (88)
#define OTA_ACCESS_CODE                   0xd6be898e
#define OTA_CHANNEL                       (88) //2488, request by customer CN

enum
{
    OTA_UPDATE_HIGHER,
    OTA_UPDATE_MAND,
    OTA_UPDATE_NONE,
};

#define OTA_FRAME_PAYLOAD_MAX (48 + 2)
#define OTA_RETRY_MAX         10
#define OTA_APPEND_INFO_LEN   2 // FW_CRC 2 BYTE

typedef struct
{
    unsigned int   FlashAddr;
    unsigned int   TotalBinSize;
    unsigned short MaxBlockNum;
    unsigned short BlockNum;
    unsigned short PeerAddr;
    unsigned int   FwVersion;
    unsigned short FwCRC;
    unsigned short PktCRC;
    unsigned short TargetFwCRC;
    unsigned char  State;
    unsigned char  RetryTimes;
    unsigned char  FinishFlag;
    unsigned char  Bcnt;
    unsigned short RecvBlkNum;
} OTA_CtrlTypeDef;

typedef struct
{
    unsigned int  OtaBinAddr;
    unsigned char loss_blk_num;

} OTA_LossCtrlTypeDef;

typedef struct
{
    unsigned char Type;
    unsigned char Payload[OTA_FRAME_PAYLOAD_MAX];
} OTA_FrameTypeDef;

enum
{
    OTA_MASTER_STATE_IDLE = 0,
    OTA_MASTER_STATE_FW_VER_WAIT,
    OTA_MASTER_STATE_START_RSP_WAIT,
    OTA_MASTER_STATE_DATA_SEND,
    OTA_MASTER_STATE_DATA_ACK_WAIT,
    OTA_MASTER_STATE_END_RSP_WAIT,
    OTA_MASTER_STATE_END,
    OTA_MASTER_STATE_DATA_LOSS_CHECK,
    OTA_MASTER_STATE_DATA_LOSS_SUPP,
    OTA_MASTER_STATE_ERROR,
};

enum
{
    OTA_SLAVE_STATE_IDLE = 0,
    OTA_SLAVE_STATE_FW_VERSION_READY,
    OTA_SLAVE_STATE_START_READY,
    OTA_SLAVE_STATE_DATA_READY,
    OTA_SLAVE_STATE_END_READY,
    OTA_SLAVE_STATE_END,
    OTA_SLAVE_STATE_DATA_LOSS_CHECK,
    OTA_SLAVE_STATE_ERROR
};

enum
{
    OTA_MSG_TYPE_INVALID_DATA = 0,
    OTA_MSG_TYPE_DATA,
    OTA_MSG_TYPE_TIMEOUT,
    OTA_MSG_TYPE_TX_DONE,
};

extern void OTA_MasterInit(unsigned int OTABinAddr);
extern int  OTA_MasterStart(void);
extern int  OTA_Batch_MasterStart(void);

extern void OTA_SlaveInit(unsigned int OTABinAddr);
extern int  OTA_SlaveStart(void);
extern int  OTA_Batch_SlaveStart(void);

extern void OTA_RxIrq(unsigned char *Data);
extern void OTA_TxIrq(unsigned char *Data);
extern void OTA_RxTimeoutIrq(unsigned char *Data);


#endif /*_OTA_H_*/
