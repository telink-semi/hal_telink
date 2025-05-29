/********************************************************************************************************
 * @file    ota.c
 *
 * @brief   This is the source file for 2.4G SDK
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

#include "ota.h"
#include "tl_common.h"
#include "drivers.h"
#include "mac.h"
#include "../stack/2p4g/genfsk_ll/genfsk_ll.h"
#include "app_config.h"

#define MSG_QUEUE_LEN 4

typedef struct
{
    unsigned int   Type;
    unsigned char *Data; //content of msg
} OTA_MsgTypeDef;

typedef struct
{
    OTA_MsgTypeDef Msg[MSG_QUEUE_LEN]; //buffer storing msg
    unsigned char  Cnt;                //current num of msg
    unsigned char  ReadPtr;            //ptr of first msg should be read next
} OTA_MsgQueueTypeDef;

OTA_MsgQueueTypeDef MsgQueue;

OTA_FrameTypeDef TxFrame;
OTA_FrameTypeDef RxFrame;

unsigned char ota_flag[3] = {'o', 't', 'a'};
unsigned char ota_tag[11] = {'o', 't', 'a'};

unsigned char dev_mac[6] = {0};

volatile unsigned char OTA_MasterTrig = 0;
volatile unsigned char OTA_SlaveTrig  = 0;

uint32_t state_tick      = 0;
uint32_t loss_check_tick = 0;

//zewen debug
//#define    GPIO_DEBUG_PD0        GPIO_PD0
//#define    GPIO_DEBUG_PD1        GPIO_PD1
//int loss_state = 0;
//int loss_check = 0;
//volatile int binSize = 0;


static int OTA_MsgQueuePush(const unsigned char *Data, const unsigned int Type, OTA_MsgQueueTypeDef *Queue)
{
    if (Queue->Cnt < MSG_QUEUE_LEN) {
        Queue->Msg[(Queue->ReadPtr + Queue->Cnt) % MSG_QUEUE_LEN].Data = (unsigned char *)Data;
        Queue->Msg[(Queue->ReadPtr + Queue->Cnt) % MSG_QUEUE_LEN].Type = Type;
        Queue->Cnt++;
        return 1;
    }

    return 0;
}

static OTA_MsgTypeDef *OTA_MsgQueuePop(OTA_MsgQueueTypeDef *Queue)
{
    OTA_MsgTypeDef *ret = NULL;

    if (Queue->Cnt > 0) {
        ret            = &(Queue->Msg[Queue->ReadPtr]);
        Queue->ReadPtr = (Queue->ReadPtr + 1) % MSG_QUEUE_LEN;
        Queue->Cnt--;
    }

    return ret;
}

static int OTA_BuildCmdFrame(OTA_FrameTypeDef *Frame, const unsigned char CmdId, const unsigned char *Value, unsigned short Len)
{
    Frame->Type       = OTA_FRAME_TYPE_CMD;
    Frame->Payload[0] = CmdId;
    if (Value) {
        if (Len > OTA_FRAME_PAYLOAD_MAX - 1) {
            return 0;
        }
        memcpy(&Frame->Payload[1], Value, Len);
    }
    unsigned int fram_length = Len + 2;
    return fram_length;
}

void OTA_RxIrq(unsigned char *Data)
{
    if (NULL == Data) {
        OTA_MsgQueuePush(NULL, OTA_MSG_TYPE_INVALID_DATA, &MsgQueue);
    } else {
        if (Data[0]) {
            OTA_MsgQueuePush(Data, OTA_MSG_TYPE_DATA, &MsgQueue);
        } else {
            OTA_MsgQueuePush(NULL, OTA_MSG_TYPE_INVALID_DATA, &MsgQueue);
        }
    }
}

void OTA_TxIrq(unsigned char *Data)
{
    OTA_MsgQueuePush(Data, OTA_MSG_TYPE_TX_DONE, &MsgQueue);
}

void OTA_RxTimeoutIrq(unsigned char *Data)
{
    OTA_MsgQueuePush(Data, OTA_MSG_TYPE_TIMEOUT, &MsgQueue);
}

#if (OTA_MODE == OTA_MASTER || OTA_MODE == OTA_BATCH_MASTER)


unsigned char  loss_dev_mac[6]               = {0};
unsigned short loss_pkt_blknum[(48 - 6) / 2] = {0};

static OTA_CtrlTypeDef     MasterCtrl = {0};
static OTA_LossCtrlTypeDef LossCtrl   = {0};

static int OTA_IsBlockNumMatch(unsigned char *Payload)
{
    unsigned short BlockNum = Payload[1];
    BlockNum <<= 8;
    BlockNum += Payload[0];

    if (BlockNum == MasterCtrl.BlockNum) {
        return 1;
    } else {
        return 0;
    }
}

static int OTA_BuildDataFrame(OTA_FrameTypeDef *Frame)
{
    unsigned int fram_length;
    Frame->Type = OTA_FRAME_TYPE_DATA;
    if ((MasterCtrl.TotalBinSize - MasterCtrl.BlockNum * (OTA_FRAME_PAYLOAD_MAX - 2)) > (OTA_FRAME_PAYLOAD_MAX - 2)) {
        fram_length = OTA_FRAME_PAYLOAD_MAX;
    } else {
        fram_length           = MasterCtrl.TotalBinSize - MasterCtrl.BlockNum * (OTA_FRAME_PAYLOAD_MAX - 2) + 2;
        MasterCtrl.FinishFlag = 1;
    }
    //    tlk_printf("OTA_BuildDataFrame: SEND DATA: fram_length:%d \n", fram_length);
    MasterCtrl.BlockNum++;
    memcpy(Frame->Payload, &MasterCtrl.BlockNum, 2);
    flash_read_page(MasterCtrl.FlashAddr, fram_length - 2, &Frame->Payload[2]);
    MasterCtrl.FlashAddr += fram_length - 2;
    return (1 + fram_length);
}

static int OTA_BuildLossDataFrame(OTA_FrameTypeDef *Frame)
{
    unsigned int fram_length;
    //    MasterCtrl.BlockNum++;
    if (!loss_pkt_blknum[LossCtrl.loss_blk_num]) {
        LossCtrl.loss_blk_num = 0;
        memset(loss_pkt_blknum, 0, sizeof(loss_pkt_blknum));
        return -1;
    }
    unsigned short blkn = loss_pkt_blknum[LossCtrl.loss_blk_num];
    Frame->Type         = OTA_FRAME_TYPE_DATA;
    if ((MasterCtrl.TotalBinSize - (blkn - 1) * (OTA_FRAME_PAYLOAD_MAX - 2)) > (OTA_FRAME_PAYLOAD_MAX - 2)) {
        fram_length = OTA_FRAME_PAYLOAD_MAX + 6;
    } else {
        fram_length = MasterCtrl.TotalBinSize - (blkn - 1) * (OTA_FRAME_PAYLOAD_MAX - 2) + 2 + 6;
        //      MasterCtrl.FinishFlag = 1;
    }
    tlk_printf("OTA_BuildLossDataFrame: SEND DATA: fram_length:%d \n", fram_length);

    memcpy(Frame->Payload, &blkn, 2);
    memcpy(&Frame->Payload[2], loss_dev_mac, 6);
    flash_read_page(
        LossCtrl.OtaBinAddr + ((blkn - 1) * (OTA_FRAME_PAYLOAD_MAX - 2)),
        fram_length - 2 - 6,
        &Frame->Payload[8]);
    //    MasterCtrl.FlashAddr += fram_length-2;
    LossCtrl.loss_blk_num++;
    return (1 + fram_length);
}

void OTA_MasterInit(unsigned int OTABinAddr)
{
    MasterCtrl.FlashAddr = OTABinAddr;
    //read the size of OTA_bin file
    flash_read_page((unsigned long)MasterCtrl.FlashAddr + OTA_BIN_SIZE_OFFSET, 4, (unsigned char *)&MasterCtrl.TotalBinSize);
    MasterCtrl.TotalBinSize += OTA_APPEND_INFO_LEN; // APPEND CRC INFO IN BIN TAIL
    MasterCtrl.MaxBlockNum = (MasterCtrl.TotalBinSize + (OTA_FRAME_PAYLOAD_MAX - 2) - 1) / (OTA_FRAME_PAYLOAD_MAX - 2);
    MasterCtrl.BlockNum    = 0;
    MasterCtrl.State       = OTA_MASTER_STATE_IDLE;
    MasterCtrl.RetryTimes  = 0;
    MasterCtrl.FinishFlag  = 0;
    MasterCtrl.Bcnt        = 0;
    state_tick             = 0;

    #if FW_VERSION
    unsigned char version[5] = {0};
    flash_read_page(OTABinAddr + MasterCtrl.TotalBinSize - 11, 5, version);
    MasterCtrl.FwVersion = (version[0] - 0x30) << 16 | (version[2] - 0x30) << 8 | (version[4] - 0x30);
    #endif

    memcpy(ota_tag + 3, &MasterCtrl.FwVersion, 4);
    ota_tag[7]  = MasterCtrl.MaxBlockNum & 0xff;
    ota_tag[8]  = (MasterCtrl.MaxBlockNum >> 8) & 0xff;
    ota_tag[9]  = OTA_DATA_CHANNEL;
    ota_tag[10] = OTA_UPDATE_HIGHER;
    tlk_printf("bin_size: %x MaxBlockNum:%x ota_bin_version:%4x\n", MasterCtrl.TotalBinSize, MasterCtrl.MaxBlockNum, MasterCtrl.FwVersion);
}

    #if (OTA_MODE == OTA_BATCH_MASTER)
int OTA_Batch_MasterStart(void)
{
    OTA_MsgTypeDef *Msg  = OTA_MsgQueuePop(&MsgQueue);
    static int      Len  = 0;
    static short    fLen = 0;

    if (OTA_MASTER_STATE_IDLE == MasterCtrl.State) {
        if (!state_tick) {
            state_tick = clock_time() | 1;
            Len        = OTA_BuildCmdFrame(&TxFrame, OTA_CMD_ID_BATCH_START_REQ, ota_tag, sizeof(ota_tag));
            tlk_printf("build_batch_start_req state:%d Len:%2x\n", MasterCtrl.State, Len);
            tlk_printf("ota tag: %s\n", ota_tag);
            MAC_BroadcastData((unsigned char *)&TxFrame, Len);
        } else if (Msg) {
            //            tlk_printf ("debug %d\r\n", __LINE__);
            //            tlkapi_debug_handler ();
            //            tlk_printf ("RxPacket: %d %d %d", Msg->Data[0], Msg->Data[1],Msg->Data[2]);
            //            tlk_printf ("type: %d\r\d", Msg->Type);
            //            tlkapi_debug_handler ();
            if (Msg->Type == OTA_MSG_TYPE_TX_DONE) {
                if (state_tick && clock_time_exceed(state_tick, OTA_START_DURATION)) {
                    MasterCtrl.State = OTA_MASTER_STATE_DATA_ACK_WAIT;
                    state_tick       = clock_time() | 1;
                    tlk_printf("stop send BATCH_OTA_START\n");
                    tlkapi_debug_handler();
                    gen_fsk_channel_set(OTA_DATA_CHANNEL);
                }
                MAC_BroadcastData((unsigned char *)&TxFrame, Len);
            }
        }
    } else if (OTA_MASTER_STATE_DATA_ACK_WAIT == MasterCtrl.State) {
        if (state_tick && clock_time_exceed(state_tick, OTA_MASTER_LISTENING_DURATION)) {
            MasterCtrl.State = OTA_MASTER_STATE_END;
            tlk_printf("OTA_MASTER_STATE_DATA_ACK_WAIT timed out-----\n");
            return 1;
        }
        if (Msg) {
            // if receive a valid rf packet
            if (Msg->Type == OTA_MSG_TYPE_TX_DONE) {
                MasterCtrl.RetryTimes = 0;
                if (MasterCtrl.FinishFlag) {
                    tlk_printf("OTA_MASTER_STATE_DATA_ACK_WAIT: DATA FinishFlag %d\n", MasterCtrl.Bcnt);
                    if (DATA_BROADCAST_CNT == MasterCtrl.Bcnt) {
                        MasterCtrl.Bcnt  = 0;
                        MasterCtrl.State = OTA_MASTER_STATE_DATA_LOSS_CHECK;
                        state_tick       = clock_time() | 1;
                        fLen             = OTA_BuildCmdFrame(&TxFrame, OTA_CMD_ID_DATA_LOSS_REQ, 0, 0);
                        MAC_SendData((unsigned char *)&TxFrame, fLen);
                        return 1;
                    }
                    MasterCtrl.Bcnt++;
                    MAC_BroadcastData((unsigned char *)&TxFrame, fLen);
                } else {
                    // read OTA_bin from flash and packet it in OTA data frame
                    if (0 == MasterCtrl.Bcnt) {
                        fLen = OTA_BuildDataFrame(&TxFrame);
                    }
                    MasterCtrl.Bcnt++;
                    if (DATA_BROADCAST_CNT == MasterCtrl.Bcnt) {
                        MasterCtrl.Bcnt = 0;
                    }
                    MAC_BroadcastData((unsigned char *)&TxFrame, fLen);
                }
                return 1;
            }
        }
    } else if (OTA_MASTER_STATE_DATA_LOSS_CHECK == MasterCtrl.State) {
        if (state_tick && clock_time_exceed(state_tick, OTA_LOSS_CHECK_DURATION)) {
            MasterCtrl.State = OTA_MASTER_STATE_END;
            tlk_printf("OTA_MASTER_STATE_DATA_LOSS_CHECK timed out-----\n");
            return 1;
        }
        if (Msg) {
            if (Msg->Type == OTA_MSG_TYPE_DATA) {
                state_tick   = clock_time() | 1;
                Len          = (int)Msg->Data[0];
                RxFrame.Type = Msg->Data[1];
                memcpy(RxFrame.Payload, Msg->Data + 2, Len - 1);
                // if receive the valid FW version response
                if ((OTA_FRAME_TYPE_CMD == RxFrame.Type) && (OTA_CMD_ID_DATA_LOSS_RSP == RxFrame.Payload[0])) {
                    memcpy(loss_dev_mac, &RxFrame.Payload[1], sizeof(loss_dev_mac));
                    memcpy(loss_pkt_blknum, &RxFrame.Payload[7], Len - 1 - 6);
                    fLen = OTA_BuildLossDataFrame(&TxFrame);
                    MAC_BroadcastData((unsigned char *)&TxFrame, fLen);
                    //          gpio_write(GPIO_DEBUG_PD0, 1);
                    MasterCtrl.State = OTA_MASTER_STATE_DATA_LOSS_SUPP;
                    loss_check_tick  = clock_time() | 1;
                    MasterCtrl.Bcnt  = 1;
                    return 1;
                }
            }
            if (Msg->Type != OTA_MSG_TYPE_TX_DONE) {
                fLen = OTA_BuildCmdFrame(&TxFrame, OTA_CMD_ID_DATA_LOSS_REQ, 0, 0);
                MAC_SendData((unsigned char *)&TxFrame, fLen);
            }
        }
    } else if (OTA_MASTER_STATE_DATA_LOSS_SUPP == MasterCtrl.State) {
        if (Msg) {
            if (Msg->Type == OTA_MSG_TYPE_TX_DONE) {
                tlk_printf("OTA_MASTER_STATE_DATA_LOSS_SUPP-----:%d\n", MasterCtrl.Bcnt);
                if (0 == MasterCtrl.Bcnt) {
                    fLen = OTA_BuildLossDataFrame(&TxFrame);
                    if (fLen < 0) {
                        fLen             = OTA_BuildCmdFrame(&TxFrame, OTA_CMD_ID_DATA_LOSS_REQ, 0, 0);
                        MasterCtrl.State = OTA_MASTER_STATE_DATA_LOSS_CHECK;
                        tlk_printf("set state OTA_MASTER_STATE_DATA_LOSS_CHECK-----:%d\n", MasterCtrl.Bcnt);
                        return 1;
                    }
                }
                MasterCtrl.Bcnt++;
                if (DATA_BROADCAST_CNT == MasterCtrl.Bcnt) {
                    MasterCtrl.Bcnt = 0;
                }
                MAC_BroadcastData((unsigned char *)&TxFrame, fLen);
            }
        }
    } else if (OTA_MASTER_STATE_END == MasterCtrl.State) {
        gpio_function_en(GPIO_LED_WHITE);
        gpio_output_en(GPIO_LED_WHITE);
        gpio_write(GPIO_LED_WHITE, 1);
        int wcnt = 0;
        while (1) {
            if (wcnt == 10) {
                MasterCtrl.State = OTA_MASTER_STATE_IDLE;
                state_tick       = 0;
                gpio_write(GPIO_LED_WHITE, 0);
                return 0;
            }
            wcnt++;
            sleep_ms(200);
            gpio_toggle(GPIO_LED_WHITE);
        }
    } else if (OTA_MASTER_STATE_ERROR == MasterCtrl.State) {
        gpio_function_en(GPIO_LED_RED);
        gpio_output_en(GPIO_LED_RED);
        gpio_write(GPIO_LED_RED, 1);
        sleep_ms(60);
        gpio_write(GPIO_LED_RED, 0);
        sleep_ms(120);
        gpio_write(GPIO_LED_RED, 1);
        sleep_ms(60);
        gpio_write(GPIO_LED_RED, 0);
        sleep_ms(120);
        return 0;
    }
    return 1;
}
    #endif


    #if (OTA_MODE == OTA_MASTER)
int OTA_MasterStart(void)
{
    wd_clear(); //feed dog
    OTA_MsgTypeDef *Msg = OTA_MsgQueuePop(&MsgQueue);
    static int      Len = 0;
    if (OTA_MASTER_STATE_IDLE == MasterCtrl.State) {
        if (!state_tick) {
            state_tick = clock_time() | 1;
            Len        = OTA_BuildCmdFrame(&TxFrame, OTA_CMD_ID_VERSION_REQ, 0, 0);
            MAC_SendData((unsigned char *)&TxFrame, Len);
            MasterCtrl.State = OTA_MASTER_STATE_FW_VER_WAIT;
        }
    } else if (OTA_MASTER_STATE_FW_VER_WAIT == MasterCtrl.State) {
        if (Msg) {
            //            tlk_printf("RxPacket: %d %d %d",Msg->Data[0],Msg->Data[1],Msg->Data[2]);
            //            tlk_printf("type: %d",Msg->Type);
            //            tlk_printf("debug %d\r\n",__LINE__);
            //if receive a valid rf packet
            if (Msg->Type == OTA_MSG_TYPE_DATA) {
                Len          = (int)Msg->Data[0];
                RxFrame.Type = Msg->Data[1];
                if (Len > OTA_FRAME_PAYLOAD_MAX + 1) {
                    MasterCtrl.State = OTA_MASTER_STATE_ERROR;
                    tlk_printf("len bigger than paylaod max!-----\r\n");
                    return 1;
                }
                memcpy(RxFrame.Payload, Msg->Data + 2, Len - 1);
                //if receive the valid FW version response
                if ((OTA_FRAME_TYPE_CMD == RxFrame.Type) &&
                    (OTA_CMD_ID_VERSION_RSP == RxFrame.Payload[0])) {
                    MasterCtrl.RetryTimes = 0;
                    //compare the received version with that of OTA_bin
                    unsigned int Version = 0;
                    Version              = RxFrame.Payload[4] << 24 | RxFrame.Payload[3] << 16 | RxFrame.Payload[2] << 8 | RxFrame.Payload[1];
        #if FW_VERSION
                    if (Version < MasterCtrl.FwVersion) {
        #else
                    if (1) {
        #endif
                        MasterCtrl.State = OTA_MASTER_STATE_START_RSP_WAIT;
                        Len              = OTA_BuildCmdFrame(&TxFrame, OTA_CMD_ID_START_REQ, (unsigned char *)&MasterCtrl.MaxBlockNum, sizeof(MasterCtrl.MaxBlockNum));
                        MAC_SendData((unsigned char *)&TxFrame, Len);
                        tlk_printf("ota bin version:%2x,slave bin version %2x\r\n", Version, MasterCtrl.FwVersion);
                    } else {
                        MasterCtrl.State = OTA_MASTER_STATE_ERROR;
                        tlk_printf("ota bin version not higher than slave!-----\r\n");
                        tlk_printf("ota bin version:%2x,slave bin version %2x\r\n", Version, MasterCtrl.FwVersion);
                    }
                    return 1;
                }
            }

            if (MasterCtrl.RetryTimes == OTA_RETRY_MAX) {
                MasterCtrl.State = OTA_MASTER_STATE_ERROR;
                tlk_printf("OTA_MASTER_STATE_FW_VER_WAIT retry timed out-----\n");
                return 1;
            }
            if (Msg->Type != OTA_MSG_TYPE_TX_DONE) {
                MasterCtrl.RetryTimes++;
                MAC_SendData((unsigned char *)&TxFrame, Len);
            }
        }
    } else if (OTA_MASTER_STATE_START_RSP_WAIT == MasterCtrl.State) {
        if (Msg) {
            //if receive a valid rf packet
            if (Msg->Type == OTA_MSG_TYPE_DATA) {
                Len          = (int)Msg->Data[0];
                RxFrame.Type = Msg->Data[1];
                if (Len > OTA_FRAME_PAYLOAD_MAX + 1) {
                    MasterCtrl.State = OTA_MASTER_STATE_ERROR;
                    tlk_printf("len bigger than max payload-----\n");
                    return 1;
                }
                memcpy(RxFrame.Payload, Msg->Data + 2, Len - 1);
                //if receive the valid FW version response
                if ((OTA_FRAME_TYPE_CMD == RxFrame.Type) &&
                    (OTA_CMD_ID_START_RSP == RxFrame.Payload[0])) {
                    MasterCtrl.RetryTimes = 0;
                    //read OTA_bin from flash and packet it in OTA data frame
                    MasterCtrl.State = OTA_MASTER_STATE_DATA_ACK_WAIT;
                    Len              = OTA_BuildDataFrame(&TxFrame);
                    MAC_SendData((unsigned char *)&TxFrame, Len);
                    return 1;
                }
            }

            if (MasterCtrl.RetryTimes == OTA_RETRY_MAX) {
                MasterCtrl.State = OTA_MASTER_STATE_ERROR;
                tlk_printf("OTA_MASTER_STATE_START_RSP_WAIT retry timed out-----\n");
                return 1;
            }
            if (Msg->Type != OTA_MSG_TYPE_TX_DONE) {
                MasterCtrl.RetryTimes++;
                MAC_SendData((unsigned char *)&TxFrame, Len);
            }
        }
    }

    else if (OTA_MASTER_STATE_DATA_ACK_WAIT == MasterCtrl.State) {
        if (Msg) {
            //if receive a valid rf packet
            if (Msg->Type == OTA_MSG_TYPE_DATA) {
                Len          = (int)Msg->Data[0];
                RxFrame.Type = Msg->Data[1];
                if (Len > OTA_FRAME_PAYLOAD_MAX + 1) {
                    MasterCtrl.State = OTA_MASTER_STATE_ERROR;
                    tlk_printf("len bigger than max payload-----\n");
                    return 1;
                }
                memcpy(RxFrame.Payload, Msg->Data + 2, Len - 1);
                //if receive the valid OTA data ack
                if ((OTA_FRAME_TYPE_ACK == RxFrame.Type) && OTA_IsBlockNumMatch(RxFrame.Payload)) {
                    MasterCtrl.RetryTimes = 0;
                    if (MasterCtrl.FinishFlag) {
                        MasterCtrl.State = OTA_MASTER_STATE_END_RSP_WAIT;
                        Len              = OTA_BuildCmdFrame(&TxFrame, OTA_CMD_ID_END_REQ, (unsigned char *)&MasterCtrl.TotalBinSize, sizeof(MasterCtrl.TotalBinSize));
                        MAC_SendData((unsigned char *)&TxFrame, Len);
                    } else {
                        //read OTA_bin from flash and packet it in OTA data frame
                        Len = OTA_BuildDataFrame(&TxFrame);
                        MAC_SendData((unsigned char *)&TxFrame, Len);
                    }
                    return 1;
                }
            }

            if (MasterCtrl.RetryTimes == OTA_RETRY_MAX) {
                MasterCtrl.State = OTA_MASTER_STATE_ERROR;
                tlk_printf("OTA_MASTER_STATE_DATA_ACK_WAIT retry timed out-----\n");
                return 1;
            }
            if (Msg->Type != OTA_MSG_TYPE_TX_DONE) {
                MasterCtrl.RetryTimes++;
                MAC_SendData((unsigned char *)&TxFrame, Len);
            }
        }
    }

    else if (OTA_MASTER_STATE_END_RSP_WAIT == MasterCtrl.State) {
        if (Msg) {
            //if receive a valid rf packet
            if (Msg->Type == OTA_MSG_TYPE_DATA) {
                Len          = (int)Msg->Data[0];
                RxFrame.Type = Msg->Data[1];
                if (Len > OTA_FRAME_PAYLOAD_MAX + 1) {
                    MasterCtrl.State = OTA_MASTER_STATE_ERROR;
                    tlk_printf("len bigger than max payload-----\n");
                    return 1;
                }
                memcpy(RxFrame.Payload, Msg->Data + 2, Len - 1);
                //if receive the valid FW version response
                if ((OTA_FRAME_TYPE_CMD == RxFrame.Type) &&
                    (OTA_CMD_ID_END_RSP == RxFrame.Payload[0])) {
                    MasterCtrl.RetryTimes = 0;
                    MasterCtrl.State      = OTA_MASTER_STATE_END;
                    return 1;
                }
            }
            if (MasterCtrl.RetryTimes == OTA_RETRY_MAX) {
                MasterCtrl.State = OTA_MASTER_STATE_ERROR;
                tlk_printf("OTA_MASTER_STATE_END_RSP_WAIT retry timed out-----\n");
                return 1;
            }
            if (Msg->Type != OTA_MSG_TYPE_TX_DONE) {
                MasterCtrl.RetryTimes++;
                MAC_SendData((unsigned char *)&TxFrame, Len);
            }
        }
    } else if (OTA_MASTER_STATE_END == MasterCtrl.State) {
        tlk_printf("master ota state success\n");
        gpio_function_en(GPIO_LED_WHITE);
        gpio_output_en(GPIO_LED_WHITE);
        gpio_write(GPIO_LED_WHITE, 1);
        sleep_ms(200);
        gpio_write(GPIO_LED_WHITE, 0);
        sleep_ms(200);
        gpio_write(GPIO_LED_WHITE, 1);
        sleep_ms(200);
        gpio_write(GPIO_LED_WHITE, 0);
        sleep_ms(200);

        return 0;
    } else if (OTA_MASTER_STATE_ERROR == MasterCtrl.State) {
        tlk_printf("master ota state err\r\n");

        gpio_function_en(GPIO_LED_RED);
        gpio_output_en(GPIO_LED_RED);
        gpio_write(GPIO_LED_RED, 1);
        sleep_ms(200);
        gpio_write(GPIO_LED_RED, 0);
        sleep_ms(200);
        gpio_write(GPIO_LED_RED, 1);
        sleep_ms(200);
        gpio_write(GPIO_LED_RED, 0);
        sleep_ms(200);

        return 0;
    }
}
    #endif

#else
    #define OTA_LOSS_PKT_MAX_NUM (48 - 6) //in us

unsigned short loss_packet_block[OTA_LOSS_PKT_MAX_NUM] = {0};
unsigned short loss_packet_cnt                         = 0;

static OTA_CtrlTypeDef SlaveCtrl = {0};

static unsigned short OTA_CRC16_Cal(unsigned short crc, unsigned char *pd, int len)
{
    // unsigned short       crc16_poly[2] = { 0, 0xa001 }; //0x8005 <==> 0xa001
    unsigned short crc16_poly[2] = {0, 0x8408}; //0x1021 <==> 0x8408
    //unsigned short        crc16_poly[2] = { 0, 0x0811 }; //0x0811 <==> 0x8810
    //unsigned short        crc = 0xffff;
    int i, j;

    for (j = len; j > 0; j--) {
        unsigned char ds = *pd++;
        for (i = 0; i < 8; i++) {
            crc = (crc >> 1) ^ crc16_poly[(crc ^ ds) & 1];
            ds  = ds >> 1;
        }
    }

    return crc;
}

static void OTA_FlashErase(void)
{
    int SectorAddr = SlaveCtrl.FlashAddr;
    int i          = 0;
    for (i = 0; i < 15; i++) {
        flash_erase_sector(SectorAddr);
        SectorAddr += 0x1000;
    }
}

unsigned int mac_Length = 0;

void OTA_SlaveInit(unsigned int OTABinAddr)
{
    SlaveCtrl.FlashAddr  = OTABinAddr;
    SlaveCtrl.BlockNum   = 0;
    SlaveCtrl.State      = OTA_SLAVE_STATE_IDLE;
    SlaveCtrl.RetryTimes = 0;
    SlaveCtrl.FinishFlag = 0;
    SlaveCtrl.FwCRC = SlaveCtrl.PktCRC = 0;
    SlaveCtrl.RecvBlkNum               = 0;
    SlaveCtrl.TotalBinSize             = 0;
    //erase the OTA write area
    OTA_FlashErase();

    flash_read_page(MAC_FLASH_ADDR, 6, dev_mac);

    #if FW_VERSION

    unsigned char version[5]    = {0};
    unsigned int  local_binsize = 0;
    if (SlaveCtrl.FlashAddr) {
        flash_read_page((unsigned long)OTA_SLAVE_BIN_ADDR_1 + OTA_BIN_SIZE_OFFSET, 4, (unsigned char *)&local_binsize);
        local_binsize += OTA_APPEND_INFO_LEN;
        flash_read_page(OTA_SLAVE_BIN_ADDR_1 + local_binsize - 11, 5, version);
        SlaveCtrl.FwVersion = (version[0] - 0x30) << 16 | (version[2] - 0x30) << 8 | (version[4] - 0x30);
    } else {
        flash_read_page((unsigned long)OTA_SLAVE_BIN_ADDR_2 + OTA_BIN_SIZE_OFFSET, 4, (unsigned char *)&local_binsize);
        local_binsize += OTA_APPEND_INFO_LEN;
        flash_read_page(OTA_SLAVE_BIN_ADDR_2 + local_binsize - 11, 5, version);
        SlaveCtrl.FwVersion = (version[0] - 0x30) << 16 | (version[2] - 0x30) << 8 | (version[4] - 0x30);
    }
    tlk_printf("local_bin_ver: %4x ", SlaveCtrl.FwVersion);
    #endif
}
    #if (OTA_MODE == OTA_SLAVE)

static int OTA_BuildAckFrame(OTA_FrameTypeDef *Frame, unsigned short BlockNum)
{
    unsigned int fram_length;
    Frame->Type       = OTA_FRAME_TYPE_ACK;
    fram_length       = 2;
    Frame->Payload[0] = BlockNum & 0xff;
    Frame->Payload[1] = BlockNum >> 8;


    return (1 + fram_length);
}

int OTA_SlaveStart(void)
{
    wd_clear(); //feed dog

    OTA_MsgTypeDef *Msg = OTA_MsgQueuePop(&MsgQueue);
    static int      Len = 0;
    if (OTA_SLAVE_STATE_IDLE == SlaveCtrl.State) {
        SlaveCtrl.State = OTA_SLAVE_STATE_FW_VERSION_READY;
        MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
    } else if (OTA_SLAVE_STATE_FW_VERSION_READY == SlaveCtrl.State) {
        if (Msg) {
            //            tlk_printf("debug %d\r\n",__LINE__);
            //            tlk_printf("RxPacket: %d %d %d",Msg->Data[0],Msg->Data[1],Msg->Data[2]);
            //            tlk_printf("type: %d",Msg->Type);
            //            tlk_printf("\r\n");
            //if receive a valid rf packet
            if (Msg->Type == OTA_MSG_TYPE_DATA) {
                Len          = (int)Msg->Data[0];
                RxFrame.Type = Msg->Data[1];
                if (Len > OTA_FRAME_PAYLOAD_MAX + 1) {
                    SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
                    tlk_printf("len bigger than max payload-----\n");
                    return 1;
                }
                memcpy(RxFrame.Payload, Msg->Data + 2, Len - 1);
                //if receive the valid FW version request
                if ((OTA_FRAME_TYPE_CMD == RxFrame.Type) && (OTA_CMD_ID_VERSION_REQ == RxFrame.Payload[0])) {
                    SlaveCtrl.RetryTimes = 0;
                    //send the FW version response to master
                    SlaveCtrl.State = OTA_SLAVE_STATE_START_READY;
                    Len             = OTA_BuildCmdFrame(&TxFrame, OTA_CMD_ID_VERSION_RSP, (unsigned char *)&SlaveCtrl.FwVersion, sizeof(SlaveCtrl.FwVersion));
                    MAC_SendData((unsigned char *)&TxFrame, Len);
                    return 1;
                }
            }

            if (SlaveCtrl.RetryTimes == OTA_RETRY_MAX) {
                SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
                tlk_printf("OTA_SLAVE_STATE_FW_VERSION_READY retry timed out-----\n");
                return 1;
            }
            if (Msg->Type != OTA_MSG_TYPE_TX_DONE) {
                SlaveCtrl.RetryTimes++;
                MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
            }
        }
    } else if (OTA_SLAVE_STATE_START_READY == SlaveCtrl.State) {
        if (Msg) {
            //if receive a valid rf packet
            if (Msg->Type == OTA_MSG_TYPE_DATA) {
                Len          = (int)Msg->Data[0];
                RxFrame.Type = Msg->Data[1];
                if (Len > OTA_FRAME_PAYLOAD_MAX + 1) {
                    SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
                    tlk_printf("len bigger than max payload-----\n");
                    return 1;
                }
                memcpy(RxFrame.Payload, Msg->Data + 2, Len - 1);
                if (OTA_FRAME_TYPE_CMD == RxFrame.Type) {
                    //if receive the FW version request again
                    if (OTA_CMD_ID_VERSION_REQ == RxFrame.Payload[0]) {
                        SlaveCtrl.RetryTimes = 0;
                        //send the FW version response again to master
                        MAC_SendData((unsigned char *)&TxFrame, Len);
                        return 1;
                    }
                    //if receive the OTA start request
                    if (OTA_CMD_ID_START_REQ == RxFrame.Payload[0]) {
                        SlaveCtrl.RetryTimes = 0;
                        memcpy(&SlaveCtrl.MaxBlockNum, &RxFrame.Payload[1], sizeof(SlaveCtrl.MaxBlockNum));
                        //send the OTA start response to master
                        SlaveCtrl.State = OTA_SLAVE_STATE_DATA_READY;
                        Len             = OTA_BuildCmdFrame(&TxFrame, OTA_CMD_ID_START_RSP, 0, 0);
                        MAC_SendData((unsigned char *)&TxFrame, Len);
                        return 1;
                    }
                }
            }

            if (SlaveCtrl.RetryTimes == OTA_RETRY_MAX) {
                SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
                tlk_printf("OTA_SLAVE_STATE_START_READY retry timed out-----\n");
                return 1;
            }
            if (Msg->Type != OTA_MSG_TYPE_TX_DONE) {
                SlaveCtrl.RetryTimes++;
                MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
            }
        }
    } else if (OTA_SLAVE_STATE_DATA_READY == SlaveCtrl.State) {
        if (Msg) {
            //if receive a valid rf packet
            if (Msg->Type == OTA_MSG_TYPE_DATA) {
                Len          = (int)Msg->Data[0];
                RxFrame.Type = Msg->Data[1];
                if (Len > OTA_FRAME_PAYLOAD_MAX + 1) {
                    SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
                    tlk_printf("len bigger than max payload-----\n");
                    return 1;
                }
                memcpy(RxFrame.Payload, Msg->Data + 2, Len - 1);
                //if receive the OTA start request again
                if (OTA_FRAME_TYPE_CMD == RxFrame.Type) {
                    if (OTA_CMD_ID_START_REQ == RxFrame.Payload[0]) {
                        SlaveCtrl.RetryTimes = 0;
                        //send the OTA start response again to master
                        MAC_SendData((unsigned char *)&TxFrame, Len); //need change
                        return 1;
                    }
                }
                //if receive the OTA data frame
                if (OTA_FRAME_TYPE_DATA == RxFrame.Type) {
                    //check the block number included in the incoming frame
                    unsigned short BlockNum = RxFrame.Payload[1];
                    BlockNum <<= 8;
                    BlockNum += RxFrame.Payload[0];
                    //if receive the same OTA data frame again, just respond with the same ACK
                    if (BlockNum == SlaveCtrl.BlockNum) {
                        SlaveCtrl.RetryTimes = 0;
                        //send the OTA data ack again to master
                        MAC_SendData((unsigned char *)&TxFrame, Len);
                        return 1;
                    }
                    //if receive the next OTA data frame, just respond with an ACK
                    if (BlockNum == SlaveCtrl.BlockNum + 1) {
                        SlaveCtrl.RetryTimes = 0;
                        /*
                        * write received data to flash,
                        * and avoid first block data writing boot flag as head of time.
                        */
                        if (1 == BlockNum) {
                            // unfill boot flag in ota procedure
                            flash_write_page(SlaveCtrl.FlashAddr, OTA_BOOT_FlAG_OFFSET, &RxFrame.Payload[2]);
                            flash_write_page(SlaveCtrl.FlashAddr + OTA_BOOT_FlAG_OFFSET + 4, Len - 3 - (OTA_BOOT_FlAG_OFFSET + 4), &RxFrame.Payload[2 + OTA_BOOT_FlAG_OFFSET + 4]);
                        } else {
                            //write received data to flash
                            flash_write_page(SlaveCtrl.FlashAddr + SlaveCtrl.TotalBinSize, Len - 3, &RxFrame.Payload[2]);
                        }
                        SlaveCtrl.BlockNum = BlockNum;
                        SlaveCtrl.TotalBinSize += (Len - 3);
                        if (SlaveCtrl.MaxBlockNum == BlockNum) {
                            SlaveCtrl.PktCRC = OTA_CRC16_Cal(SlaveCtrl.PktCRC, &RxFrame.Payload[2], Len - 3 - OTA_APPEND_INFO_LEN);
                            SlaveCtrl.State  = OTA_SLAVE_STATE_END_READY;
                        } else {
                            SlaveCtrl.PktCRC = OTA_CRC16_Cal(SlaveCtrl.PktCRC, &RxFrame.Payload[2], Len - 3);
                        }
                        //send the OTA data ack to master
                        unsigned int Length = OTA_BuildAckFrame(&TxFrame, BlockNum);
                        mac_Length          = Length;
                        MAC_SendData((unsigned char *)&TxFrame, Length);
                        return 1;
                    }
                }
            }
            if (SlaveCtrl.RetryTimes == OTA_RETRY_MAX) {
                SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
                tlk_printf("OTA_SLAVE_STATE_DATA_READY retry timed out-----\n");
                return 1;
            }
            if (Msg->Type != OTA_MSG_TYPE_TX_DONE) {
                SlaveCtrl.RetryTimes++;
                MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
            }
        }
    } else if (OTA_SLAVE_STATE_END_READY == SlaveCtrl.State) {
        if (Msg) {
            //if receive a valid rf packet
            if (Msg->Type == OTA_MSG_TYPE_DATA) {
                Len          = (int)Msg->Data[0];
                RxFrame.Type = Msg->Data[1];
                if (Len > OTA_FRAME_PAYLOAD_MAX + 1) {
                    SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
                    tlk_printf("len bigger than max payload-----\n");
                    return 1;
                }
                memcpy(RxFrame.Payload, Msg->Data + 2, Len - 1);
                //if receive the last OTA data frame again
                if (OTA_FRAME_TYPE_DATA == RxFrame.Type) {
                    //check the block number included in the incoming frame
                    unsigned short BlockNum = RxFrame.Payload[1];
                    BlockNum <<= 8;
                    BlockNum += RxFrame.Payload[0];
                    //if receive the same OTA data frame again, just respond with the same ACK
                    if (BlockNum == SlaveCtrl.BlockNum) {
                        SlaveCtrl.RetryTimes = 0;
                        //send the OTA data ack again to master
                        MAC_SendData((unsigned char *)&TxFrame, Len);
                        return 1;
                    }
                }
                //if receive the OTA end request
                if (OTA_FRAME_TYPE_CMD == RxFrame.Type) {
                    if (OTA_CMD_ID_END_REQ == RxFrame.Payload[0]) {
                        SlaveCtrl.RetryTimes = 0;
                        unsigned int BinSize = 0;
                        memcpy(&BinSize, &RxFrame.Payload[1], sizeof(BinSize));
                        if (SlaveCtrl.TotalBinSize != BinSize) {
                            SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
                            tlk_printf("BinSize ERR!-----\n");
                            tlk_printf("BinSize %4x,TotalBinSize %4x\n", BinSize, SlaveCtrl.TotalBinSize);
                            return 1;
                        }
                        SlaveCtrl.State = OTA_SLAVE_STATE_END;
                        //send the OTA end response to master
                        Len = OTA_BuildCmdFrame(&TxFrame, OTA_CMD_ID_END_RSP, 0, 0);
                        MAC_SendData((unsigned char *)&TxFrame, Len);
                        sleep_ms(5); //wait for transmission finished
                        return 1;
                    }
                }
            }
            if (SlaveCtrl.RetryTimes == OTA_RETRY_MAX) {
                SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
                tlk_printf("OTA_SLAVE_STATE_END_READY retry timed out-----\n");
                return 1;
            }
            if (Msg->Type != OTA_MSG_TYPE_TX_DONE) {
                SlaveCtrl.RetryTimes++;
                MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
            }
        }
    } else if (OTA_SLAVE_STATE_END == SlaveCtrl.State) {
        #if 1
        // 1. todo FW crc check
        // int max_block_num = (SlaveCtrl.TotalBinSize + OTA_FRAME_PAYLOAD_MAX -2 - 1) / (OTA_FRAME_PAYLOAD_MAX - 2);
        unsigned char bin_buf[48] = {0};
        int           block_idx   = 0;
        int           len         = 0;
        flash_read_page((unsigned long)SlaveCtrl.FlashAddr + SlaveCtrl.TotalBinSize - OTA_APPEND_INFO_LEN, 2, (unsigned char *)&SlaveCtrl.TargetFwCRC);
        while (1) {
            if (SlaveCtrl.TotalBinSize - block_idx * (OTA_FRAME_PAYLOAD_MAX - 2) > (OTA_FRAME_PAYLOAD_MAX - 2)) {
                len = OTA_FRAME_PAYLOAD_MAX - 2;
                flash_read_page((unsigned long)SlaveCtrl.FlashAddr + block_idx * (OTA_FRAME_PAYLOAD_MAX - 2), len, &bin_buf[0]);
                if (0 == block_idx) {
                    // fill the boot flag mannually
                    bin_buf[OTA_BOOT_FlAG_OFFSET]     = 0x4b;
                    bin_buf[OTA_BOOT_FlAG_OFFSET + 1] = 0x4e;
                    bin_buf[OTA_BOOT_FlAG_OFFSET + 2] = 0x4c;
                    bin_buf[OTA_BOOT_FlAG_OFFSET + 3] = 0x54;
                }
                SlaveCtrl.FwCRC = OTA_CRC16_Cal(SlaveCtrl.FwCRC, &bin_buf[0], len);
            } else {
                tlk_printf("len:%d total bin size:%2x block_idx:%d\r\n", len, SlaveCtrl.TotalBinSize, block_idx);
                len = SlaveCtrl.TotalBinSize - (block_idx * (OTA_FRAME_PAYLOAD_MAX - 2)) - OTA_APPEND_INFO_LEN;
                flash_read_page((unsigned long)SlaveCtrl.FlashAddr + block_idx * (OTA_FRAME_PAYLOAD_MAX - 2), len, &bin_buf[0]);
                SlaveCtrl.FwCRC = OTA_CRC16_Cal(SlaveCtrl.FwCRC, &bin_buf[0], len);
                break;
            }

            block_idx++;
            //            tlk_printf("read addr:%d\r\n", SlaveCtrl.FlashAddr + block_idx * (OTA_FRAME_PAYLOAD_MAX - 2));
            //            tlk_printf("fw block idx:%d, len:%d, FwCRC:%2x\r\n", block_idx, len, SlaveCtrl.FwCRC);
        }

        //        tlk_printf("fw block idx:%d, len:%d, FwCRC:%2x  \r\n", block_idx + 1, len, SlaveCtrl.FwCRC);
        //        tlk_printf("pkt_crc:%2x, fw_crc:%2x, target_fw_crc:%2x\r\n", SlaveCtrl.PktCRC, SlaveCtrl.FwCRC, SlaveCtrl.TargetFwCRC);
        if (SlaveCtrl.FwCRC != SlaveCtrl.PktCRC || SlaveCtrl.TargetFwCRC != SlaveCtrl.FwCRC) {
            tlk_printf("Crc Check Error\r\n");
            SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
            return 1;
        }
        #endif

        // 2. set next boot flag
        unsigned int utmp = 0x544C4E4B;
        flash_write_page(SlaveCtrl.FlashAddr + OTA_BOOT_FlAG_OFFSET, 4, (unsigned char *)&utmp);

        //clear current boot flag
        unsigned char tmp = 0x00;
        #if (OTA_SLAVE_BIN_ADDR_2 == OTA_SLAVE_BIN_ADDR_0x20000)
        flash_write_page(SlaveCtrl.FlashAddr ? OTA_BOOT_FlAG_OFFSET : (OTA_SLAVE_BIN_ADDR_0x20000 + OTA_BOOT_FlAG_OFFSET), 1, &tmp);
        tlk_printf("SlaveCtrl.FlashAddr:%4x\r\n", SlaveCtrl.FlashAddr);
        #else
        flash_write_page((OTA_SLAVE_BIN_ADDR_0x20000 + OTA_BOOT_FlAG_OFFSET), 1, &tmp);
        flash_write_page(SlaveCtrl.FlashAddr ? OTA_BOOT_FlAG_OFFSET : (OTA_SLAVE_BIN_ADDR_0x40000 + OTA_BOOT_FlAG_OFFSET), 1, &tmp);
        tlk_printf("SlaveCtrl.FlashAddr:%4x\r\n", SlaveCtrl.FlashAddr);
        #endif
        //reboot
        irq_disable();
        while (1) {
            gpio_function_en(GPIO_LED_WHITE);
            gpio_output_en(GPIO_LED_WHITE);
            gpio_write(GPIO_LED_WHITE, 1);
            sleep_ms(200);
            gpio_write(GPIO_LED_WHITE, 0);
            sleep_ms(200);
            gpio_write(GPIO_LED_WHITE, 1);
            sleep_ms(200);
            gpio_write(GPIO_LED_WHITE, 0);
            sleep_ms(200);
            start_reboot();
            while (1)
                ;
        }
    } else if (OTA_SLAVE_STATE_ERROR == SlaveCtrl.State) {
        //erase the OTA write area
        OTA_FlashErase();
        //cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_TIMER, ClockTime() + OTA_REBOOT_WAIT * 16);
        gpio_function_en(GPIO_LED_RED);
        gpio_output_en(GPIO_LED_RED);
        gpio_write(GPIO_LED_RED, 1);
        sleep_ms(200);
        gpio_write(GPIO_LED_RED, 0);
        sleep_ms(200);
        gpio_write(GPIO_LED_RED, 1);
        sleep_ms(200);
        gpio_write(GPIO_LED_RED, 0);
        sleep_ms(200);
        return 0;
    }
    return 1;
}
    #endif


    #if (OTA_MODE == OTA_BATCH_SLAVE)
static int is_blkn_in_q(unsigned short blkn)
{
    unsigned char i    = 0;
    int           iret = 0;

    for (; i < loss_packet_cnt; i++) {
        if (loss_packet_block[i] == blkn) {
            iret = 1;
            break;
        }
    }
    return iret;
}

static void delete_blkn_in_q(unsigned short blkn)
{
    unsigned char i = 0;
    for (; i < loss_packet_cnt; i++) {
        if (loss_packet_block[i] == blkn) {
            for (int j = i; j < loss_packet_cnt; j++) {
                loss_packet_block[j] = loss_packet_block[j + 1];
            }
            loss_packet_cnt--;
            break;
        }
    }
}

int OTA_Batch_SlaveStart(void)
{
    wd_clear(); //feed dog

    static int      Len = 0;
    OTA_MsgTypeDef *Msg = OTA_MsgQueuePop(&MsgQueue);
    if (OTA_SLAVE_STATE_IDLE == SlaveCtrl.State) {
        SlaveCtrl.State = OTA_SLAVE_STATE_START_READY;
        state_tick      = clock_time() | 1;
        MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
    } else if (OTA_SLAVE_STATE_START_READY == SlaveCtrl.State) {
        if (Msg) {
            //      tlk_printf ("debug %d\r\n", __LINE__);
            //      tlk_printf ("RxPacket: %d %d %d %d %d %d %d %d %d", Msg->Data[2], Msg->Data[3],Msg->Data[4],Msg->Data[5],Msg->Data[6],Msg->Data[7],Msg->Data[8],Msg->Data[9]);
            //      tlk_printf ("type: %d", Msg->Type);
            //      tlk_printf ("\r\n");
            //      tlkapi_debug_handler ();
            if (state_tick && clock_time_exceed(state_tick, OTA_WAIT_BATCH_OTA_START_DURATION)) {
                tlk_printf("ota wait for batch OTA start timed out\n");
                SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
                return 1;
            }
            //if receive a valid rf packet
            if (Msg->Type == OTA_MSG_TYPE_DATA) {
                Len          = (int)Msg->Data[0];
                RxFrame.Type = Msg->Data[1];
                memcpy(RxFrame.Payload, Msg->Data + 2, Len - 1);
                if (OTA_FRAME_TYPE_CMD == RxFrame.Type) {
                    //if receive the OTA start request
                    if (OTA_CMD_ID_BATCH_START_REQ == RxFrame.Payload[0]) {
                        unsigned char ota_flag_r[3] = {0};
                        ota_flag_r[0]               = RxFrame.Payload[1];
                        ota_flag_r[1]               = RxFrame.Payload[2];
                        ota_flag_r[2]               = RxFrame.Payload[3];
                        if (memcmp(ota_flag, ota_flag_r, sizeof(ota_flag_r))) {
                            tlk_printf("recv ota flag error: %x %x %x\n", ota_flag_r[0], ota_flag_r[1], ota_flag_r[2]);
                            SlaveCtrl.State = OTA_SLAVE_STATE_IDLE;
                            return 1;
                        }
                        unsigned int  fwv    = RxFrame.Payload[4] | (RxFrame.Payload[5] << 8) | (RxFrame.Payload[6] << 16) | (RxFrame.Payload[7] << 24);
                        unsigned char policy = RxFrame.Payload[11];
                        tlk_printf("local_bin_ver:%4x ota_bin_ver: %4x policy: %x\n", SlaveCtrl.FwVersion, fwv, policy);
                        if (policy == OTA_UPDATE_NONE) {
                            tlk_printf("master Specify OTA_UPDATE_NONE\n");
                            SlaveCtrl.State = OTA_SLAVE_STATE_END;
                            return 1;
                        } else if (policy == OTA_UPDATE_HIGHER) {
        #if FW_VERSION
                            if (SlaveCtrl.FwVersion >= fwv) {
                                tlk_printf("ota fw ver not higher than local ver ota_ver:%x local_ver:%x\n", fwv, SlaveCtrl.FwVersion);
                                SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
                                return 1;
                            }
        #endif
                        }
                        SlaveCtrl.RetryTimes = 0;
                        memcpy(&SlaveCtrl.MaxBlockNum, &RxFrame.Payload[8], sizeof(SlaveCtrl.MaxBlockNum));
                        tlk_printf("MaxBlockNum: %x ", SlaveCtrl.MaxBlockNum);
                        //send the OTA start response to master
                        SlaveCtrl.State            = OTA_SLAVE_STATE_DATA_READY;
                        state_tick                 = clock_time() | 1;
                        unsigned char ota_data_chn = RxFrame.Payload[10];
                        tlk_printf("chn:%d\n", ota_data_chn);
                        gen_fsk_channel_set((signed short)ota_data_chn);
                    }
                }
            }
            MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
        }
    } else if (OTA_SLAVE_STATE_DATA_READY == SlaveCtrl.State) {
        if (Msg) {
            //      tlk_printf ("debug %d\r\n", __LINE__);
            //      tlk_printf ("RxPacket: %d %d %d %d", Msg->Data[0], Msg->Data[1],Msg->Data[2],Msg->Data[3]);
            //      tlk_printf ("type: %d", Msg->Type);
            //      tlk_printf ("\r\n");
            //      tlkapi_debug_handler ();
            if (state_tick && clock_time_exceed(state_tick, OTA_PROCESS_DURATION)) {
                SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
                tlk_printf("OTA_PROCESS_DURATION timed out-----\n");
                return 1;
            }
            // if receive a valid rf packet
            if (Msg->Type == OTA_MSG_TYPE_DATA) {
                Len          = (int)Msg->Data[0];
                RxFrame.Type = Msg->Data[1];
                memcpy(RxFrame.Payload, Msg->Data + 2, Len - 1);
                // if receive the OTA start request again
                if (OTA_FRAME_TYPE_CMD == RxFrame.Type) {
                    if (OTA_CMD_ID_BATCH_START_REQ == RxFrame.Payload[0]) {
                        SlaveCtrl.RetryTimes = 0;
                        MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
                        return 1;
                    } else if (OTA_CMD_ID_DATA_LOSS_REQ == RxFrame.Payload[0]) {
                        //                        gpio_write(GPIO_DEBUG_PD1, 1);
                        if (SlaveCtrl.MaxBlockNum == SlaveCtrl.RecvBlkNum) {
                            unsigned int bin_ver;
        #if FW_VERSION
                            unsigned char version[5] = {0};
                            flash_read_page(SlaveCtrl.FlashAddr + SlaveCtrl.TotalBinSize - 11, 5, version);
                            bin_ver = (version[0] - 0x30) << 16 | (version[2] - 0x30) << 8 | (version[4] - 0x30);
        #endif
                            tlk_printf("good job, no packet loss:%d, ota bin VERSION:%x\n", SlaveCtrl.RecvBlkNum, bin_ver);
                            SlaveCtrl.State = OTA_SLAVE_STATE_END;
                        } else if ((SlaveCtrl.MaxBlockNum - SlaveCtrl.RecvBlkNum) > DATA_LOSS_CHECK_CNT) {
                            tlk_printf("too bad job, too many packet losses:%d\n", SlaveCtrl.MaxBlockNum - SlaveCtrl.RecvBlkNum);
                            SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
                            return 1;
                        } else {
                            tlk_printf("bad job, packet loss:%d\n", SlaveCtrl.MaxBlockNum - SlaveCtrl.RecvBlkNum);
                            if (SlaveCtrl.BlockNum != SlaveCtrl.MaxBlockNum) {
                                for (int i = 0; i < (SlaveCtrl.MaxBlockNum - SlaveCtrl.BlockNum); i++) {
                                    loss_packet_block[loss_packet_cnt++] = SlaveCtrl.BlockNum + 1 + i;
                                    // loss_packet_cnt++;
                                }
                            }
                            unsigned char loss_buf[OTA_FRAME_PAYLOAD_MAX - 2] = {0};
                            memcpy(loss_buf, dev_mac, sizeof(dev_mac));
                            memcpy(&loss_buf[6], loss_packet_block, loss_packet_cnt);
                            Len = OTA_BuildCmdFrame(&TxFrame, OTA_CMD_ID_DATA_LOSS_RSP, loss_buf, sizeof(dev_mac) + loss_packet_cnt);
                            MAC_BroadcastData((unsigned char *)&TxFrame, Len);
                            //                            gpio_write(GPIO_DEBUG_PD0, 1);
                            SlaveCtrl.State = OTA_SLAVE_STATE_DATA_LOSS_CHECK;
                            loss_check_tick = clock_time() | 1;
                            //                          loss_state = 1;
                            return 1;
                        }
                    }
                }
                // if receive the OTA data frame
                if (OTA_FRAME_TYPE_DATA == RxFrame.Type) {
                    //  tlk_printf("OTA_SLAVE_STATE_DATA_READY state recv OTA_FRAME_TYPE_DATA\n");
                    // check the block number included in the incoming frame
                    unsigned short BlockNum = RxFrame.Payload[1];
                    BlockNum <<= 8;
                    BlockNum += RxFrame.Payload[0];
                    // tlk_printf("recv bn:%d\n", BlockNum);
                    // if receive the same OTA data frame again, just respond with the same ACK
                    if (BlockNum == SlaveCtrl.BlockNum) {
                        SlaveCtrl.RetryTimes = 0;
                        // send the OTA data ack again to master
                        // MAC_SendData((unsigned char *)&TxFrame, Len);
                        MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
                        return 1;
                    }
                    // if receive the next OTA data frame, just respond with an ACK
                    if (BlockNum >= (SlaveCtrl.BlockNum + 1)) {
                        if (BlockNum > (SlaveCtrl.BlockNum + 1)) {
                            for (int i = 0; i < (BlockNum - SlaveCtrl.BlockNum - 1); i++) {
                                loss_packet_block[loss_packet_cnt++] = SlaveCtrl.BlockNum + 1 + i;
                                // loss_packet_cnt++;
                                tlk_printf("packet loss bln: %x ", SlaveCtrl.BlockNum + 1 + i);
                            }
                        }
                        SlaveCtrl.RetryTimes = 0;
                        flash_write_page(SlaveCtrl.FlashAddr + ((OTA_FRAME_PAYLOAD_MAX - 2) * (BlockNum - 1)), Len - 3, &RxFrame.Payload[2]);
                        SlaveCtrl.RecvBlkNum++;
                        SlaveCtrl.BlockNum = BlockNum;
                        SlaveCtrl.TotalBinSize += (Len - 3);

                        if ((SlaveCtrl.MaxBlockNum == BlockNum) && (!loss_packet_cnt)) {
                            SlaveCtrl.PktCRC = OTA_CRC16_Cal(SlaveCtrl.PktCRC, &RxFrame.Payload[2], Len - 3 - OTA_APPEND_INFO_LEN);
                        } else {
                            SlaveCtrl.PktCRC = OTA_CRC16_Cal(SlaveCtrl.PktCRC, &RxFrame.Payload[2], Len - 3);
                        }

                        MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
                        return 1;
                    }
                }
            }
            MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
        }
    } else if (OTA_SLAVE_STATE_DATA_LOSS_CHECK == SlaveCtrl.State) {
        //    loss_check = 1;
        tlk_printf("OTA_SLAVE_STATE_DATA_LOSS_CHECK  -----\n");
        if (Msg) {
            if (loss_check_tick && clock_time_exceed(loss_check_tick, OTA_LOSS_CHECK_DURATION)) {
                SlaveCtrl.State = OTA_SLAVE_STATE_DATA_READY;
                tlk_printf("OTA_SLAVE_STATE_DATA_LOSS_CHECK timed out-----\n");
                return 1;
            }
            // if receive a valid rf packet
            if (Msg->Type == OTA_MSG_TYPE_DATA) {
                tlk_printf("OTA_SLAVE_STATE_DATA_LOSS_CHECK recv supplement package---\n");
                Len          = (int)Msg->Data[0];
                RxFrame.Type = Msg->Data[1];
                memcpy(RxFrame.Payload, Msg->Data + 2, Len - 1);
                unsigned char mac_temp[6] = {0};
                memcpy(mac_temp, &RxFrame.Payload[2], sizeof(mac_temp));
                if (!memcmp(dev_mac, mac_temp, sizeof(mac_temp))) {
                    tlk_printf("recv supplement package---\n");
                    unsigned short blkn = RxFrame.Payload[0] | (RxFrame.Payload[1] << 8);
                    if (is_blkn_in_q(blkn)) {
                        tlk_printf("recv supplement package---Len: %d  %x %x %x %x\n", Len - 3 - 6, RxFrame.Payload[8], RxFrame.Payload[9], RxFrame.Payload[10], RxFrame.Payload[11]);
                        flash_write_page(SlaveCtrl.FlashAddr + ((OTA_FRAME_PAYLOAD_MAX - 2) * (blkn - 1)), Len - 3 - 6, &RxFrame.Payload[8]);
                        SlaveCtrl.TotalBinSize += (Len - 3 - 6);
                        delete_blkn_in_q(blkn);
                        if (!loss_packet_cnt) {
                            tlk_printf("recv all supplement packages---\n");
                            SlaveCtrl.State = OTA_SLAVE_STATE_END;
                            return 1;
                        }
                    }
                }
            }
            MAC_RecvData(OTA_MASTER_LISTENING_DURATION);
        }
    } else if (OTA_SLAVE_STATE_END == SlaveCtrl.State) {
        #if 1
        // 1. todo FW crc check
        unsigned char bin_buf[48] = {0};
        int           block_idx   = 0;
        int           len         = 0;
        flash_read_page((unsigned long)SlaveCtrl.FlashAddr + SlaveCtrl.TotalBinSize - OTA_APPEND_INFO_LEN, 2, (unsigned char *)&SlaveCtrl.TargetFwCRC);
        while (1) {
            if (SlaveCtrl.TotalBinSize - block_idx * (OTA_FRAME_PAYLOAD_MAX - 2) > (OTA_FRAME_PAYLOAD_MAX - 2)) {
                len = OTA_FRAME_PAYLOAD_MAX - 2;
                flash_read_page((unsigned long)SlaveCtrl.FlashAddr + block_idx * (OTA_FRAME_PAYLOAD_MAX - 2), len, &bin_buf[0]);
                if (0 == block_idx) {
                    // fill the boot flag mannually
                    bin_buf[32] = 0x4b;
                    bin_buf[33] = 0x4e;
                    bin_buf[34] = 0x4c;
                    bin_buf[35] = 0x54;
                }
                SlaveCtrl.FwCRC = OTA_CRC16_Cal(SlaveCtrl.FwCRC, &bin_buf[0], len);
            } else {
                len = SlaveCtrl.TotalBinSize - (block_idx * (OTA_FRAME_PAYLOAD_MAX - 2)) - OTA_APPEND_INFO_LEN;
                flash_read_page((unsigned long)SlaveCtrl.FlashAddr + block_idx * (OTA_FRAME_PAYLOAD_MAX - 2), len, &bin_buf[0]);
                SlaveCtrl.FwCRC = OTA_CRC16_Cal(SlaveCtrl.FwCRC, &bin_buf[0], len);
                break;
            }
            block_idx++;
            //          tlk_printf("fw block idx:%d, len:%d, FwCRC:%2x\r\n", block_idx, len, SlaveCtrl.FwCRC);
        }
        tlk_printf("fw block idx:%d, len:%d, FwCRC:%2x  \r\n", block_idx + 1, len, SlaveCtrl.FwCRC);
        tlk_printf("pkt_crc:%2x, fw_crc:%2x, target_fw_crc:%2x\r\n", SlaveCtrl.PktCRC, SlaveCtrl.FwCRC, SlaveCtrl.TargetFwCRC);
        // if (SlaveCtrl.FwCRC != SlaveCtrl.PktCRC || SlaveCtrl.TargetFwCRC != SlaveCtrl.FwCRC)
        if (SlaveCtrl.TargetFwCRC != SlaveCtrl.FwCRC) {
            tlk_printf("Crc Check Error\r\n");
            SlaveCtrl.State = OTA_SLAVE_STATE_ERROR;
            return 1;
        }
        #endif

        // 2. set next boot flag
        unsigned int utmp = 0x544C4E4B;
        flash_write_page(SlaveCtrl.FlashAddr + 0x20, 4, (unsigned char *)&utmp);

        //clear current boot flag
        unsigned char tmp = 0x00;
        #if (OTA_SLAVE_BIN_ADDR_2 == OTA_SLAVE_BIN_ADDR_0x20000)
        flash_write_page(SlaveCtrl.FlashAddr ? 0x20 : (OTA_SLAVE_BIN_ADDR_0x20000 + 0x20), 1, &tmp);
        tlk_printf("SlaveCtrl.FlashAddr:%4x\r\n", SlaveCtrl.FlashAddr);
        #else
        flash_write_page((OTA_SLAVE_BIN_ADDR_0x20000 + 0x20), 1, &tmp);
        flash_write_page(SlaveCtrl.FlashAddr ? 0x20 : (OTA_SLAVE_BIN_ADDR_0x40000 + 0x20), 1, &tmp);
        tlk_printf("SlaveCtrl.FlashAddr:%4x\r\n", SlaveCtrl.FlashAddr);
        #endif

        gpio_function_en(GPIO_LED_WHITE);
        gpio_output_en(GPIO_LED_WHITE);
        gpio_write(GPIO_LED_WHITE, 1);
        sleep_ms(200);
        gpio_write(GPIO_LED_WHITE, 0);
        sleep_ms(200);
        gpio_write(GPIO_LED_WHITE, 1);
        sleep_ms(200);
        gpio_write(GPIO_LED_WHITE, 0);
        sleep_ms(200);

        //reboot
        irq_disable();
        sleep_ms(1000);
        start_reboot();
        while (1)
            ;
    } else if (OTA_SLAVE_STATE_ERROR == SlaveCtrl.State) {
        //erase the OTA write area
        OTA_FlashErase();
        //      irq_disable ();
        //        cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_TIMER, ClockTime() + OTA_REBOOT_WAIT * 16);
        gpio_function_en(GPIO_LED_RED);
        gpio_output_en(GPIO_LED_RED);
        gpio_write(GPIO_LED_RED, 1);
        sleep_ms(60);
        gpio_write(GPIO_LED_RED, 0);
        sleep_ms(120);
        gpio_write(GPIO_LED_RED, 1);
        sleep_ms(60);
        gpio_write(GPIO_LED_RED, 0);
        sleep_ms(120);
        return 0;
    }
    // tlk_printf("ota state error :%d\n", SlaveCtrl.State);
    return 1;
}
    #endif
#endif /*OTA_MASTER_EN*/
