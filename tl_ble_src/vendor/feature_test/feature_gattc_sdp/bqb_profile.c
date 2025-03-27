/********************************************************************************************************
 * @file    bqb_profile.c
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
#include "app_config.h"
#if (FEATURE_TEST_MODE == TEST_GATTC_SDP)
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "bqb_profile.h"


#if (1 || BQB_PROFILE_ENABLE)

#include "app_att.h"

u16 gAppBqbConnHandle = 0;

volatile u16 gAppBqbUpdateFlags = 0;
extern u8 cis_id[];
int app_bqb_debug(unsigned char *p, int len);

void app_bqb_init(void)
{
    //blc_smp_setSecurityLevel_periphr(No_Security);
    //blc_smp_configSecurityRequestSending(SecReq_NOT_SEND, SecReq_NOT_SEND, 0);

    myudb_register_hci_debug_cb(app_bqb_debug);

    BLT_APP_LOG("app_bqb_init");
}

void app_bqb_connect(u16 aclHandle)
{
    gAppBqbConnHandle = aclHandle;

    BLT_APP_LOG("app_bqb_connect:0x%x", gAppBqbConnHandle);
}
void app_bqb_disconn(u16 aclHandle)
{
    gAppBqbConnHandle = 0;
    BLT_APP_LOG("app_bqb_disconnect:0x%x", aclHandle);
}


void app_bqb_handler(void)
{
    
}

u16 gTempReadError = 0x00;

int app_bqb_debug(unsigned char *p, int len)
{
    BLT_APP_STR_LOG("[APP]app_bqb_debug receive", p, len);

    if(p[0] != 0x11) return 1;

    u16 test_case = ((p[1]<<8) | p[2]);

    switch(test_case){
    /******************** ASCP ********************/

    /******************** CCP ********************/

    /******************** CSIP ********************/

    /******************** CSIP ********************/

    /******************** VCP ********************/
        case BQB_VCP_VC_READ:

        break;
        case BQB_VCP_VC_WRITE:

        break;
        case BQB_VCP_VC_CGGIT_SER_BV_01_C:

        break;
        case BQB_VCP_VC_CGGIT_SER_BV_02_C:

        break;
        case BQB_VCP_VC_CGGIT_SER_BV_03_C:

        break;
        case BQB_VCP_VC_CGGIT_CHA_BV_01_C:

        break;
        case BQB_VCP_VC_CGGIT_CHA_BV_02_C:

        break;
        case BQB_VCP_VC_CGGIT_CHA_BV_03_C:

        break;
        case BQB_VCP_VC_CGGIT_CHA_BV_04_C:

        break;
        case BQB_VCP_VC_CGGIT_CHA_BV_05_C:

        break;
        case BQB_VCP_VC_CGGIT_CHA_BV_06_C:

        break;
        case BQB_VCP_VC_CGGIT_CHA_BV_07_C:

        break;
        case BQB_VCP_VC_CGGIT_CHA_BV_08_C:

        break;
        case BQB_VCP_VC_CGGIT_CHA_BV_09_C:

        break;
        case BQB_VCP_VC_CGGIT_CHA_BV_10_C:

        break;
        case BQB_VCP_VC_CGGIT_CHA_BV_11_C:

        break;
        case BQB_VCP_VC_CGGIT_CHA_BV_12_C:

        break;
        case BQB_VCP_VC_CGGIT_CHA_BV_13_C:

        break;
        //SPE
        case BQB_VCP_VC_SPE_BI_01_C:

        break;
        case BQB_VCP_VC_SPE_BI_02_C:

        break;
        case BQB_VCP_VC_SPE_BI_03_C:

        break;
        case BQB_VCP_VC_SPE_BI_04_C:

        break;
        case BQB_VCP_VC_SPE_BI_05_C:

        break;
        case BQB_VCP_VC_SPE_BI_06_C:

        break;
        case BQB_VCP_VC_SPE_BI_07_C:

        break;
        case BQB_VCP_VC_SPE_BI_08_C:

        break;
        case BQB_VCP_VC_SPE_BI_09_C:

        break;
        case BQB_VCP_VC_SPE_BI_10_C:

        break;
        case BQB_VCP_VC_SPE_BI_11_C:

        break;
        case BQB_VCP_VC_SPE_BI_12_C:

        break;
        case BQB_VCP_VC_SPE_BI_13_C:

        break;
        case BQB_VCP_VC_SPE_BI_14_C:

        break;
        case BQB_VCP_VC_SPE_BI_15_C:

        break;
        case BQB_VCP_VC_SPE_BI_16_C:

        break;
        case BQB_VCP_VC_SPE_BI_17_C:

        break;
        case BQB_VCP_VC_SPE_BI_18_C:

        break;
        case BQB_VCP_VC_SPE_BI_19_C:

        break;
        case BQB_VCP_VC_SPE_BI_20_C:

        break;

        default:
            blc_ll_disconnect(gAppBqbConnHandle, HCI_ERR_REMOTE_USER_TERM_CONN);
            break;
    }


    return 0;
}


#endif //BQB_PROFILE_ENABLE
#endif //#if (FEATURE_TEST_MODE == TEST_GATTC_SDP)

