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
#include "../../ull_hid_config.h"
#if (ULL_HID_DEMO_SLECT == ULL_HID_HOST)


#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app_ull_hid.h"
#include "../../app_parse/app_parse_char.h"
#include "../app.h"
#include "stack/ble/host/gatt/tlk_timer_stack.h"
enum{
    NONE_MODE,
    KEYBOARD_MODE,
    MOUSE_MODE,
};
struct app_ull_all_param{
    u16 aclHandle;
    u16 cisHandle;
};

struct ullhid_packet{
    u8 length;
    u8 sequenceNumber;
    u8 reportId;
    u8 report[0];
};
ullhid_sdu_data_t sduData[8];
u8 reportIndex;
static struct app_ull_all_param ullParam;

void blc_app_ull_ui_init(void);
void app_ull_cis_loop(void);

void app_ull_hid_acl_connect(u16 connHandle)
{
    ullParam.aclHandle = connHandle;
#if (UI_LED_ENABLE)
    gpio_write(GPIO_LED_RED, 1);
#endif
    app_parse_printf("ACL connect.\r\n");

//    blc_ll_setPhy (connHandle, PHY_TRX_PREFER, PHY_PREFER_2M, PHY_PREFER_2M, CODED_PHY_PREFER_NONE);
}

void app_ull_hid_acl_disconnect(u16 connHandle)
{
    ullParam.aclHandle = 0;
#if (UI_LED_ENABLE)
    gpio_write(GPIO_LED_RED, 0);
#endif
    app_parse_printf("ACL disconnect.\r\n");
}



void app_ull_hid_cis_connect(u16 connHandle, u16 isoIntvl, u8 NSE, u16 pdu_m2s)
{
    app_parse_printf("CIS connect.\r\n");
    ullParam.cisHandle = connHandle;
    ullhidParam.cisSduInterval = isoIntvl*1250/NSE;
    ullhidParam.maxPduSize = pdu_m2s;
    ullhidParam.sequenceNumber = 0;
    ullhidParam.recvAckSeqNum = 0xFF;
    ullhidParam.recvSequenceNumber = 0;
    ullhidParam.retryCount = ullhidParam.maxPduSize / 5;
    ullhidParam.retryCount = min(ullhidParam.retryCount, ARRAY_SIZE(sduData));

#if (UI_LED_ENABLE)
    gpio_write(GPIO_LED_GREEN, 1);
#endif
}

void app_ull_hid_cis_disconnect(u16 connHandle)
{
    if(ullParam.cisHandle == connHandle)
    {
        ullParam.cisHandle = 0;
    #if (UI_LED_ENABLE)
        gpio_write(GPIO_LED_GREEN, 0);
    #endif
        hci_le_removeCig_retParam_t removeCigRetParam;
        blc_hci_le_removeCig(CIG_ID_0, &removeCigRetParam);
        app_parse_printf("CIS disconnect.\r\n");
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


/**
 * @brief   initial Ultra Low Latency HID Host.
 * @param   none.
 * @return  none.
 */
void app_initial_ull_hid_host(void)
{

//    blc_prf_initPairingInfoStoreModule();

//    blc_basic_registerBASControlClient(NULL);
    blc_hid_registerULLHIDControlClient(NULL);
//    blc_basic_registerDISControlClient(NULL);
//    blc_basic_registerSCPSControlClient(NULL);
    blc_hid_registerHIDControlClient(NULL);

    blc_app_ull_ui_init();
    soft_timer_initial();

}



static void app_ull_receiveOutputHidIso(struct ullhid_packet *pkt)
{
    app_parse_printf("Input, Seq:%d, Value:%s\r\n", pkt->sequenceNumber, hex_to_str(pkt->report, pkt->length));
}
/**
 * @brief       Ultra Low Latency HID device main loop function.
 * @param[in]   none.
 * @return      none.
 */
void app_ull_hid_host_main_loop(void)
{
    app_parse_loop();
    app_ull_cis_loop();
    soft_timer_process(MAINLOOP_ENTRY);
}

void app_ull_cis_loop(void)
{
    u16 cisHandle = app_ull_hid_get_cis_handle();
    if(cisHandle == 0)    return ;

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
//                app_parse_printf("ISO receive empty.\r\n");
            }
            return ;
        }
//        tlkapi_printf(1, "sdu is %s", hex_to_str(pPkt, sizeof(sdu_packet_t) + pPkt->iso_sdu_len));
        u16 sduLen = pPkt->iso_sdu_len;
        u8 *sdu = pPkt->data;
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

            if(pkt->length == 0)
            {
                //ack packet.
                if(ullhidParam.powerSavingCfm == 1 && ullhidParam.reportType == ULL_HID_REPORT_TYPE_OUTPUT)
                {
                    ullhidParam.recvAckSeqNum = pkt->sequenceNumber;
                }
            }

            if(ullhidParam.reportType == ULL_HID_REPORT_TYPE_INPUT)
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
            if(ullhidParam.reportType == ULL_HID_REPORT_TYPE_INPUT)
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
        }while(sduLen);
    }
}

#endif    //INTER_TEST_MODE == TEST_ULL_HID_HOST_CUSTOMER
