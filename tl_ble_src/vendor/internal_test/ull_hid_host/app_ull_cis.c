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
#if (INTER_TEST_MODE == TEST_ULL_HID_HOST)


    #include "tl_common.h"
    #include "drivers.h"
    #include "stack/ble/ble.h"

    #include "app_parse_char.h"
    #include "app_buffer.h"
    #include "app_ull_cis.h"
    #include "app.h"
    #include "app_ull_hid.h"


extern u16 app_aclConnHandle[];
extern u16 app_cisConnHandle[];
extern int app_cis_conn_num;


    #define REPORT_INTERVAL_US 1000 //5ms, unit: us
    #define NSE                5
    #define MAX_PDU_M2S        10
    #define MAX_PDU_S2M        10
    #define PHY_M2S            PHY_PREFER_2M //refer to 'le_phy_prefer_type_t'
    #define PHY_S2M            PHY_PREFER_2M


    #define PHY_M2S            PHY_PREFER_2M //refer to 'le_phy_prefer_type_t'
    #define PHY_S2M            PHY_PREFER_2M

    #define MAX_TEST_SDU_M2S   20
    #define MAX_TEST_SDU_S2M   20

void app_ullhid_initCigParam(void)
{
    int sduInterval = ullhidParam.reportInterval;
    int nse         = ullhidParam.nse;

    int iso_interval = sduInterval * nse / 1250;

    u8 cigParam[100];
    u8 retParam[12];

    u8 *pCigParam = cigParam;

    U8_TO_STREAM(pCigParam, CIG_ID_0);        //cig_id
    U24_TO_STREAM(pCigParam, sduInterval);    //sdu_int_m2s
    U24_TO_STREAM(pCigParam, sduInterval);    //sdu_int_s2m
    U8_TO_STREAM(pCigParam, 1);               //ft_m2s
    U8_TO_STREAM(pCigParam, 1);               //ft_s2m
    U16_TO_STREAM(pCigParam, iso_interval);   //iso_intvl
    U8_TO_STREAM(pCigParam, PPM_251_500);     //sca
    U8_TO_STREAM(pCigParam, PACK_SEQUENTIAL); //packing
    U8_TO_STREAM(pCigParam, CIS_UNFRAMED);    //framing
    U8_TO_STREAM(pCigParam, 1);               //cis_count
    U8_TO_STREAM(pCigParam, CIS_ID_0);        //cis_id
    U8_TO_STREAM(pCigParam, nse);             //nse

    u16 m2sPdu = 0;
    u16 s2mPdu = 0;

    if (ullhidParam.reportType == ULL_HID_REPORT_TYPE_INPUT) {
        if (ullhidParam.powerSavingCfm == 0) {
            m2sPdu = 0;
        } else {
            m2sPdu = 3;
        }
        s2mPdu = ullhidParam.cisSduS2M;
    }

    else if (ullhidParam.reportType == ULL_HID_REPORT_TYPE_OUTPUT) {
        if (ullhidParam.powerSavingCfm == 0) {
            s2mPdu = 0;
        } else {
            s2mPdu = 3; //TODO: 3 will create CIG failed.
        }
        m2sPdu = ullhidParam.cisSduM2S;
    }

    s2mPdu = ullhidParam.cisSduS2M;
    m2sPdu = ullhidParam.cisSduM2S;

    U16_TO_STREAM(pCigParam, m2sPdu);               //max_sdu_m2s
    U16_TO_STREAM(pCigParam, s2mPdu);               //max_sdu_s2m
    U16_TO_STREAM(pCigParam, m2sPdu);               //max_pdu_m2s
    U16_TO_STREAM(pCigParam, s2mPdu);               //max_pdu_s2m

    U8_TO_STREAM(pCigParam, PHY_M2S);               //phy_m2s
    U8_TO_STREAM(pCigParam, PHY_S2M);               //phy_s2m
    U8_TO_STREAM(pCigParam, m2sPdu == 0 ? 0 : nse); //bn_m2s
    U8_TO_STREAM(pCigParam, s2mPdu == 0 ? 0 : nse); //bn_s2m

    int state = 0;

    if (sduInterval < 2000) {
        state = blc_hci_le_setCigParamsTest((hci_le_setCigParamTest_cmdParam_t *)cigParam, (hci_le_setCigParam_retParam_t *)retParam);
    } else {
        state = blc_hci_le_setCigParamsULL((hci_le_setCigParamTest_cmdParam_t *)cigParam, (hci_le_setCigParam_retParam_t *)retParam);
    }


    app_parse_printf("set CIG parameter test state is 0x%x, res is %s\r\n", state, hex_to_str(retParam, 5));

    u16 cisConnHandle = ((hci_le_setCigParam_retParam_t *)retParam)->cis_connHandle[0];

    u8                        cis_create_buffer[12];
    hci_le_CreateCisParams_t *pCisParam = (hci_le_CreateCisParams_t *)cis_create_buffer;

    pCisParam->cis_count             = 1;
    pCisParam->cisConn[0].cis_handle = cisConnHandle;
    pCisParam->cisConn[0].acl_handle = app_ull_hid_get_acl_handle();

    ble_sts_t status = blc_hci_le_createCis(pCisParam);

    app_parse_printf("create CIS connect %s\r\n", status == BLE_SUCCESS ? "successful" : "failed");
}


#endif //INTER_TEST_MODE == TEST_ULL_HID_HOST
