/********************************************************************************************************
 * @file    app_ull_hid.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "../intest_config.h"
#if (INTER_TEST_MODE == TEST_ULL_HID_DEVICE)


#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app_ull_hid.h"
#include "app_parse_char.h"
#include "stack/ble/host/gatt/tlk_timer_stack.h"
#include "app.h"

struct app_ull_all_param{
    u16 aclHandle;
    u16 cisHandle;
    u16 connInterval;
};

static struct app_ull_all_param ullParam;

void app_ull_hid_acl_connect(u16 connHandle, u16 connInterval)
{
    ullParam.aclHandle = connHandle;
    ullParam.connInterval = connInterval;
#if (UI_LED_ENABLE)
    gpio_write(GPIO_LED_RED, 1);
#endif
    app_parse_printf("ACL connect.\r\n");
}

void app_ull_hid_acl_disconnect(u16 connHandle)
{
    ullParam.aclHandle = 0;
#if (UI_LED_ENABLE)
    gpio_write(GPIO_LED_RED, 0);
#endif
    app_parse_printf("ACL disconnect.\r\n");
}

ullhid_sdu_data_t sduData[8];
u8 reportIndex = 0;

void app_ull_hid_cis_connect(u16 connHandle, u16 isoIntvl, u8 NSE, u16 pdu_s2m)
{
    app_parse_printf("CIS connect.\r\n");
    ullParam.cisHandle = connHandle;
    ullhidParam.cisSduInterval = isoIntvl*1250/NSE;
    ullhidParam.maxPduSize = pdu_s2m;
    ullhidParam.sequenceNumber = 0;
    ullhidParam.recvAckSeqNum = 0xFF;
    ullhidParam.recvSequenceNumber = 0;
    ullhidParam.retryCount = ullhidParam.maxPduSize / 5;
    ullhidParam.retryCount = min(ullhidParam.retryCount, ARRAY_SIZE(sduData));

#if (UI_LED_ENABLE)
    gpio_write(GPIO_LED_GREEN, 1);
#endif
    blc_ullhids_setHybridMode();
}

void app_ull_hid_cis_disconnect(u16 connHandle)
{
    if(ullParam.cisHandle == connHandle)
    {
        ullParam.cisHandle = 0;
    #if (UI_LED_ENABLE)
        gpio_write(GPIO_LED_GREEN, 0);
    #endif
        timer_stop(TIMER0);
        app_parse_printf("CIS disconnect.\r\n");
        blc_ullhids_setDefaultMode();
    }
}

u16 app_ull_hid_get_acl_handle(void)
{
    return ullParam.aclHandle;
}

u16 app_ull_hid_get_cis_handle(void)
{
    return ullParam.cisHandle;
}

static int app_ullhid_prfEvtCb(u16 aclHandle, int evtID, u8 *pData, u16 dataLen);

const struct blc_ullhids_regParam ullhidsParam = {
    .properties.deviceModeChange = 1,
//  .properties.suppInterval.interval_1ms = 1,
    .properties.suppInterval.interval_2ms = 1,
    .properties.suppInterval.interval_3ms = 1,
    .properties.suppInterval.interval_4ms = 1,
    .properties.suppInterval.interval_5ms = 1,
//  .properties.suppInterval.interval_1_25ms = 1,
    .properties.suppInterval.interval_2_5ms = 1,
    .properties.suppInterval.interval_3_75ms = 1,
    .properties.deviceToHostMaxSduSize = 20,
    .properties.hostToDeviceMaxSduSize = 10,
//  .properties.hybridModeReport[0].reportType = 0x00,
//  .properties.hybridModeReport[0].reportID = HID_REPORT_ID_MOUSE_INPUT,
//  .properties.hybridModeReport[0].repetition = 0x01,
    .properties.hybridModeReport[0].reportType = 0x00,
    .properties.hybridModeReport[0].reportID = HID_REPORT_ID_KEYBOARD_INPUT,
    .properties.hybridModeReport[0].powerSavingCfm = 0x01,
//  .properties.hybridModeReport[0].reportType = 0x01,
//  .properties.hybridModeReport[0].reportID = HID_REPORT_ID_KEYBOARD_INPUT,
//  .properties.hybridModeReport[0].powerSavingCfm = 0x01,
//  .properties.hybridModeReport[2].reportType = 0x00,
//  .properties.hybridModeReport[2].reportID = 6,
//  .properties.hybridModeReport[2].repetition = 0x01,
//  .properties.hybridModeReport[2].powerSavingCfm = 0x01,
//  .properties.hybridModeReport[3].reportType = 0x00,
//  .properties.hybridModeReport[3].reportID = 7,
//
//  .properties.hybridModeReport[4].reportType = 0x01,
//  .properties.hybridModeReport[4].reportID = 8,
//  .properties.hybridModeReport[4].repetition = 0x01,
//  .properties.hybridModeReport[5].reportType = 0x01,
//  .properties.hybridModeReport[5].reportID = 9,
//  .properties.hybridModeReport[5].powerSavingCfm = 0x01,
//  .properties.hybridModeReport[6].reportType = 0x01,
//  .properties.hybridModeReport[6].reportID = 10,
//  .properties.hybridModeReport[6].repetition = 0x01,
//  .properties.hybridModeReport[6].powerSavingCfm = 0x01,
//  .properties.hybridModeReport[7].reportType = 0x01,
//  .properties.hybridModeReport[7].reportID = 11,
};

/**
 * @brief   initial Ultra Low Latency HID device.
 * @param   none.
 * @return  none.
 */
void app_initial_ull_hid_device(void)
{
    blc_prf_initialModule(app_ullhid_prfEvtCb);

    blc_basic_registerBASControlServer(NULL);
    blc_hid_registerULLHIDControlServer(&ullhidsParam);
    blc_basic_registerDISControlServer(NULL);
    blc_basic_registerSCPSControlServer(NULL);
    blc_hid_registerHIDControlServer(NULL);

    blc_app_ull_ui_init();


    const char *temp[] = {"NoSupp", "Supp"};
    const struct blc_ullhid_properties_format *properties = &ullhidsParam.properties;
    app_parse_printf("Hybrid Mode ULL Reports information\n");
    app_parse_printf("Features: \n\t Device Mode Change:%s\n", temp[properties->deviceModeChange]);
    app_parse_printf("Supported Report Interval: \n\t 1ms:%s, 2ms:%s, 3ms:%s, 4ms:%s, \n\t 5ms:%s, 1.25ms:%s, 2.5ms:%s, 3.75ms:%s\n",
        temp[properties->suppInterval.interval_1ms], temp[properties->suppInterval.interval_2ms],
        temp[properties->suppInterval.interval_3ms], temp[properties->suppInterval.interval_4ms],
        temp[properties->suppInterval.interval_5ms], temp[properties->suppInterval.interval_1_25ms],
        temp[properties->suppInterval.interval_2_5ms], temp[properties->suppInterval.interval_3_75ms]
    );

    app_parse_printf("max SDU size , Device to Host:%d, Host to Device:%d\n", properties->deviceToHostMaxSduSize, properties->hostToDeviceMaxSduSize);

    for(int i=0; i<8; i++)
    {
        if(properties->hybridModeReport[i].reportID == 0)
        {
            break;
        }

        app_parse_printf("Report ID:%d, Type:%s, Power Saving Confirmation:%s, Repetition:%s\n", properties->hybridModeReport[i].reportID,
                properties->hybridModeReport[i].reportType == 0x00? "Input": "Output",
                temp[properties->hybridModeReport[i].powerSavingCfm],  temp[properties->hybridModeReport[i].repetition]
        );
    }

}

void app_ull_hid_cis_loop(void);

/**
 * @brief       Ultra Low Latency HID device main loop function.
 * @param[in]   none.
 * @return      none.
 */
void app_ull_hid_device_main_loop(void)
{
    app_parse_loop();
    app_ull_hid_cis_loop();
    soft_timer_process(MAINLOOP_ENTRY);
}

/**
 * @brief       .
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_ullhid_selectHybridMode(u16 connHandle, u8 *pData, u16 dataLen)
{
    struct ullhids_selectHybridModeEvt* pEvt = (struct ullhids_selectHybridModeEvt*)pData;

    ULL_HID_LOG("host select hybrid mode, connHandle:0x%x", connHandle);
    ULL_HID_LOG("report interval is %dus, report count is %d", pEvt->reportInterval, pEvt->reportCount);
    ULL_HID_LOG("CIG ID:0x%x, CIS ID:0x%x", pEvt->CIG_ID, pEvt->CIS_ID);
    for(int i=0; i<pEvt->reportCount; i++)
    {
        ULL_HID_LOG("report Type is %s, report ID is %d",
                pEvt->reportInfo[i].reportType == 0x00? "ULL-Input": "ULL-Output",
                pEvt->reportInfo[i].reportID);
    }

    app_parse_printf("host select hybrid mode, connHandle:0x%x\r\n", connHandle);
    app_parse_printf("report interval is %dus, report count is %d\r\n", pEvt->reportInterval, pEvt->reportCount);
    app_parse_printf("CIG ID:0x%x, CIS ID:0x%x\r\n", pEvt->CIG_ID, pEvt->CIS_ID);
    for(int i=0; i<pEvt->reportCount; i++)
    {
        app_parse_printf("report Type is %s, report ID is %d\r\n",
                pEvt->reportInfo[i].reportType == ULL_HID_REPORT_TYPE_INPUT? "ULL-Input": "ULL-Output",
                pEvt->reportInfo[i].reportID);
        app_parse_printf("Power Saving Confirmation:%s\r\n", pEvt->reportInfo[i].powerSavingCfm? "Supp": "NoSupp");
        app_parse_printf("Repetition:%s\r\n", pEvt->reportInfo[i].repetition? "Supp": "NoSupp");
        ullhidParam.reportID = pEvt->reportInfo[i].reportID;
        ullhidParam.reportType = pEvt->reportInfo[i].reportType;
        ullhidParam.powerSavingCfm = pEvt->reportInfo[i].powerSavingCfm;
        ullhidParam.repetition = pEvt->reportInfo[i].repetition;
    }

    return 0;
}

/**
 * @brief       .
 * @param[in]   connHandle: ACL connect handle.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0.
 */
static int app_ullhid_selectDefaultMode(u16 connHandle, u8 *pData, u16 dataLen)
{
    ULL_HID_LOG("host set exit hybrid mode, connHandle:0x%x", connHandle);
    app_parse_printf("receive select default mode.\r\n");
    return 0;
}

static const app_prf_evtCb_t ullhidPrfCb[] = {
    {ULLHIDS_EVT_SELECT_HYBRID_MODE, app_ullhid_selectHybridMode},
    {ULLHIDS_EVT_SELECT_DEFAULT_MODE, app_ullhid_selectDefaultMode},
};

/**
 * @brief       ULL-HID profile event callback function.
 * @param[in]   aclHandle: ACL connect handle.
 * @param[in]   evtID: audio event ID, refer audio_event_enum.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0/1.
 */
int app_ullhid_prfEvtCb(u16 aclHandle, int evtID, u8 *pData, u16 dataLen)
{
    for(int i=0; i < ARRAY_SIZE(ullhidPrfCb); i++)
    {
        if(ullhidPrfCb[i].evtId == evtID)
            return ullhidPrfCb[i].evtCb(aclHandle, pData, dataLen);
    }
    return 0;
}

struct ullhid_packet{
    u8 length;
    u8 sequenceNumber;
    u8 reportId;
    u8 report[0];
};

static void app_ull_receiveOutputHidIso(struct ullhid_packet *pkt)
{
    app_parse_printf("Output, Seq:%d, Value:%s\r\n", pkt->sequenceNumber, hex_to_str(pkt->report, pkt->length));
}

void app_ull_hid_cis_loop(void)
{
    u16 cisHandle = app_ull_hid_get_cis_handle();
    if(cisHandle == 0)  return ;

    sdu_packet_t* pPkt = blc_ll_popCisRxSduData(cisHandle);

    if(pPkt)
    {
        if(pPkt->iso_sdu_len == 0)
        {
            static u8 emptyCnt = 0;
            emptyCnt++;
            if(emptyCnt == 100)
            {
                emptyCnt = 0;
//              app_parse_printf("ISO receive empty.\r\n");
            }
            return ;
        }

        u16 sduLen = pPkt->iso_sdu_len;
        u8 *sdu = pPkt->data;
//      app_parse_printf("ISO receive %s", hex_to_str(sdu, sduLen));
        do{
            struct ullhid_packet *pkt = (struct ullhid_packet*)sdu;

            u16 pktLen = pkt->length + sizeof(struct ullhid_packet);

            if(sduLen < pktLen)
            {
                break;
            }

            sduLen -= pktLen;
            sdu += pktLen;

            if(ullhidParam.reportID != pkt->reportId)
            {
                app_parse_printf("error Report ID. except ID:%d, actual ID:%d", ullhidParam.reportID ,pkt->reportId);
                continue;
            }

            if(ullhidParam.reportType == ULL_HID_REPORT_TYPE_OUTPUT)
            {
                if(ullhidParam.powerSavingCfm == 1)
                {
                    struct ullhid_packet outputAck = {
                        .length = 0,
                        .reportId = pkt->reportId,
                        .sequenceNumber = pkt->sequenceNumber
                    };
                    blc_iso_sendData(cisHandle, (u8*)&outputAck, sizeof(outputAck));
                }
            }
            if(ullhidParam.reportType == ULL_HID_REPORT_TYPE_OUTPUT)
            {
                if(ullhidParam.recvSequenceNumber == pkt->sequenceNumber)
                {
                    ullhidParam.recvSequenceNumber++;
                    app_ull_receiveOutputHidIso(pkt);
                }
                else if((u8)(ullhidParam.recvSequenceNumber - pkt->sequenceNumber) < 3)
                {

                }
                else
                {
                    ullhidParam.recvSequenceNumber = pkt->sequenceNumber + 1;
                    app_ull_receiveOutputHidIso(pkt);
                }
            }
            if(ullhidParam.reportType == ULL_HID_REPORT_TYPE_INPUT)
            {
                if(pkt->length == 0)
                {
                    ullhidParam.recvAckSeqNum = pkt->sequenceNumber;
//                  app_parse_printf("receive ack sequence number is %d.\r\n", ullhidParam.recvAckSeqNum);
                }
            }
        }while(sduLen);
    }
}

#endif  //INTER_TEST_MODE == TEST_ULL_HID_DEVICE
