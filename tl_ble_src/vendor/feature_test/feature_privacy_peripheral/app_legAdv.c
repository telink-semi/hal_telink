/********************************************************************************************************
 * @file    app_legAdv.c
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app_config.h"
#include "app.h"


#if (FEATURE_TEST_MODE == TEST_PRIVACY_PERIPHERAL)

/**
 * @brief     Configuration of the advertising parameter
 * @param[in]  none.
 * @return     none.
 */
void app_configLegacyAdvParam(void)
{
    blc_ll_setAdvEnable(BLC_ADV_DISABLE);

    //ACL Peripheral RPA Configuration - 0: ACL Peripheral role.
    //If bond number is not 0, add the latest bonding device to resolving list and white list and set ADV Filter Policy to ADV_FP_ALLOW_SCAN_WL_ALLOW_CONN_WL.
    //Only the latest bonding device can connect.
    u8 bond_number = blc_smp_param_getCurrentBondingDeviceNumber(0, 0);
    tlkapi_send_string_data(APP_LOG_EN,"[APP][RPA] bond_number", &bond_number, 1);
    if (bond_number) {
        smp_param_save_t  bondInfo;
        blc_smp_loadBondingInfoFromFlashByIndex(0, 0, bond_number-1, &bondInfo);  //get the latest bonding device (index: bond_number-1 )
        tlkapi_send_string_data(APP_LOG_EN,"[APP][SMP] central bondInfo.flag", &bondInfo.flag,1);

        u8 own_use_rpa = 1;
        tlkapi_send_string_data(APP_LOG_EN,"[APP][SMP] bondInfo.peer_irk", bondInfo.peer_irk, 16);
        tlkapi_send_string_data(APP_LOG_EN,"[APP][SMP] bondInfo.local_irk", bondInfo.local_irk, 16);
        if(!blc_app_isIrkValid(bondInfo.peer_irk))
        {
            memset(bondInfo.peer_irk, 0, 16);
        }
        if(!blc_app_isIrkValid(bondInfo.local_irk))
        {
            own_use_rpa = 0;
            memset(bondInfo.local_irk, 0, 16);
        }
        ble_sts_t status = blc_ll_addDeviceToResolvingList(bondInfo.peer_id_adrType, bondInfo.peer_id_addr, bondInfo.peer_irk, bondInfo.local_irk);
        tlkapi_send_string_data(APP_LOG_EN,"[APP][RPA] LL resolving list add status", &status,1);

        status = blc_ll_setAddressResolutionEnable(1);
        tlkapi_send_string_data(APP_LOG_EN,"[APP][RPA] LL add resolution enable status", &status,1);

        u8 app_own_address_type = own_use_rpa ? OWN_ADDRESS_RESOLVE_PRIVATE_PUBLIC : OWN_ADDRESS_PUBLIC;

        tlkapi_send_string_data(APP_LOG_EN,"[APP][SMP] app_own_address_type", &app_own_address_type, 1);

        u8* peerAddr;
        u8  peerAddrType;
        if(app_own_address_type < OWN_ADDRESS_RESOLVE_PRIVATE_PUBLIC){
            peerAddr = bondInfo.peer_addr;
            peerAddrType = bondInfo.peer_addr_type;
            tlkapi_send_string_data(APP_LOG_EN,"[APP][RPA] AdvA: pub",bondInfo.peer_addr,6);
        }
        else{
            peerAddr = bondInfo.peer_id_addr;
            peerAddrType = bondInfo.peer_id_adrType;
            tlkapi_send_string_data(APP_LOG_EN,"[APP][RPA] AdvA: rpa id",bondInfo.peer_id_addr,6);
        }

        status = blc_ll_addDeviceToWhiteList(bondInfo.peer_id_adrType, bondInfo.peer_id_addr);
        tlkapi_send_string_data(APP_LOG_EN,"[APP][RPA] LL white list add status", &status,1);
        status = blc_ll_setAdvParam(ADV_INTERVAL_50MS, ADV_INTERVAL_50MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, app_own_address_type,
                peerAddrType, peerAddr, BLT_ENABLE_ADV_ALL, ADV_FP_ALLOW_SCAN_WL_ALLOW_CONN_WL);


        if(status != BLE_SUCCESS) {
            while(1);
        }  //debug: adv setting err
    }
    else {
        u8 status = blc_ll_setAdvParam(ADV_INTERVAL_50MS, ADV_INTERVAL_50MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
                0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
        if(status != BLE_SUCCESS) {
            while(1);
        }  //debug: adv setting err
        status = blc_ll_setAddressResolutionEnable(0);
        tlkapi_send_string_data(APP_LOG_EN,"[APP][RPA] LL add resolution disable status",&status,1);
    }

    blc_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable
    tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][RPA] adv config done", 0, 0);
}

#endif //end of (FEATURE_TEST_MODE == ...)
