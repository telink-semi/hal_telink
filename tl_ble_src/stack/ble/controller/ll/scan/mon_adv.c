/********************************************************************************************************
 * @file    mon_adv.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "stack/ble/ble_stack.h"
#include "stack/ble/controller/ll/ll_stack.h"
#include "stack/ble/hci/hci_cmd.h"
#include "stack/ble/debug/debug_cfg.h"
#include "common/types.h"

#if (LL_FEATURE_ENABLE_MONITORING_ADVERTISERS)

#define MONITORING_ADVERTISERS_LIST_MAX_ENTRIES     2       //TODO: optimize: using bit field, In spec it's a dynamic buffer. by @kai.jia 20250403

#define MON_ADV_STATUS_NOT_AWAITING_RSSI_THRESHOLD_HIGH 0
#define MON_ADV_STATUS_AWAITING_RSSI_GREATER_THAN_RSSI_THRESHOLD_HIGH   1

typedef struct{
    int timer;                      //0 means disable; Enable set timer as clock_time()|1; Though the timer is used by all device now. JK think it may be used by different device someday. So define the timer per device.
    int fsm;                        //MON_ADV_STATUS_xxx
    u8 address[BLE_ADDR_LEN];
    u8 address_type;
    s8 RSSI_Threshold_Low;
    s8 RSSI_Threshold_High;
    u8 timeout;                     //unit: s;              0:RFU in BLE Spec, used as key for device in list
}monitored_advertisers_list_t;

_attribute_ble_data_retention_ monitored_advertisers_list_t mon_adv_list[MONITORING_ADVERTISERS_LIST_MAX_ENTRIES];
_attribute_ble_data_retention_ int mon_adv_en = 0;
_attribute_ble_data_retention_ int mon_adv_list_index = 0;


ble_sts_t blc_hci_le_addDeviceToMonitoredAdvertisersList(hci_le_addDeviceToMonitoredAdvertisersListcmdParam_t *pCmdParam)
{
    tlkapi_send_string_data(DBG_SCAN_MON_ADV_EN, "[HCI][CMD] Add Device to Monitored Advertisers List", pCmdParam, sizeof(hci_le_addDeviceToMonitoredAdvertisersListcmdParam_t));


    if (pCmdParam->adr_type > 0x01) {
        tlkapi_printf(DBG_SCAN_MON_ADV_EN || 1, "[MON_ADV] Error!!! Address Type = 0x%X\n", pCmdParam->adr_type);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if (pCmdParam->RSSI_Threshold_High < pCmdParam->RSSI_Threshold_Low) {
        tlkapi_printf(DBG_SCAN_MON_ADV_EN || 1, "[MON_ADV] Error!!! RSSI Threshold High[%d] < Low[%d]\n", pCmdParam->RSSI_Threshold_High, pCmdParam->RSSI_Threshold_Low);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if (pCmdParam->RSSI_Threshold_High < -127 || pCmdParam->RSSI_Threshold_High > 20) {
        tlkapi_printf(DBG_SCAN_MON_ADV_EN || 1, "[MON_ADV] Error!!! RSSI Threshold High[%d] invalid\n", pCmdParam->RSSI_Threshold_High);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if (pCmdParam->RSSI_Threshold_Low < -127 || pCmdParam->RSSI_Threshold_Low > 20) {
        tlkapi_printf(DBG_SCAN_MON_ADV_EN || 1, "[MON_ADV] Error!!! RSSI Threshold Low[%d] invalid\n", pCmdParam->RSSI_Threshold_Low);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if (pCmdParam->timeout == 0) {
        tlkapi_printf(DBG_SCAN_MON_ADV_EN || 1, "[MON_ADV] Error!!! Timeout[%d] invalid\n", pCmdParam->timeout);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    monitored_advertisers_list_t *pMAL = NULL;
    u8 ADDR_ALL_ZERO[6] = {0};
    int i = 0;
    for (; i < MONITORING_ADVERTISERS_LIST_MAX_ENTRIES; i++) {
        pMAL = &mon_adv_list[i];
        if (!pMAL->timeout) {
            pMAL->address_type = pCmdParam->adr_type;
            smemcpy(pMAL->address, pCmdParam->addr, BLE_ADDR_LEN);
            pMAL->RSSI_Threshold_Low = pCmdParam->RSSI_Threshold_Low;
            pMAL->RSSI_Threshold_High = pCmdParam->RSSI_Threshold_High;
            pMAL->timeout = pCmdParam->timeout;
            tlkapi_printf(DBG_SCAN_MON_ADV_EN, "[MON_ADV] Added Mon_adv_list[%d], with RSSI Threshold [%d, %d]\n", i, pMAL->RSSI_Threshold_Low, pMAL->RSSI_Threshold_High);
            tlkapi_send_string_data(DBG_SCAN_MON_ADV_EN, "[MON_ADV] Added Mon_adv", pMAL->address, BLE_ADDR_LEN);
            break;
        } else if (!smemcmp(pMAL->address, pCmdParam->addr, BLE_ADDR_LEN)) {
            //If the device is already in the Monitored Advertisers List, then the Controller shall not
            //add the device to the Monitored Advertisers List again and shall return success.
            //@kai.jia: But I think the RSSI Limit should be updated.
            pMAL->RSSI_Threshold_High = pCmdParam->RSSI_Threshold_High;
            pMAL->RSSI_Threshold_Low = pCmdParam->RSSI_Threshold_Low;
            tlkapi_printf(DBG_SCAN_MON_ADV_EN, "[MON_ADV] Repeated Mon_adv_list[%d]\n", i);
            tlkapi_send_string_data(DBG_SCAN_MON_ADV_EN, "[MON_ADV] Repeated Mon_adv", pMAL->address, BLE_ADDR_LEN);
            break;
        }
    }

    if (i == MONITORING_ADVERTISERS_LIST_MAX_ENTRIES) {     //Not added successfully
        tlkapi_printf(DBG_SCAN_MON_ADV_EN || 1, "[MON_ADV] Error!!! CAP Exceeded\n");
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }

    return BLE_SUCCESS;
}

//enable: 0 : Disable; 1: Enable
ble_sts_t blc_ll_monitoringAdvertisersEnable(u8 enable)
{
    tlkapi_printf(DBG_SCAN_MON_ADV_EN, "[HCI][CMD] LE %s Monitoring Advertisers\n", enable?"Enable":"Disable");

    if (enable > 0x01) {
        tlkapi_printf(DBG_SCAN_MON_ADV_EN || 1, "[MON_ADV] Error!!! Enable = 0x%X\n", enable);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    mon_adv_en = enable;
    for (int i = 0; i < MONITORING_ADVERTISERS_LIST_MAX_ENTRIES; i++) {
        monitored_advertisers_list_t *pMAL = &mon_adv_list[i];
        if (pMAL->timeout) {        //Already added
            if (enable) {
                tlkapi_printf(DBG_SCAN_MON_ADV_EN, "[MON_ADV] Init as Status [NOT AWAITING]\n");
                mon_adv_list[i].fsm = MON_ADV_STATUS_NOT_AWAITING_RSSI_THRESHOLD_HIGH;
                mon_adv_list[i].timer = clock_time()|1;
            } else {
                mon_adv_list[i].timer = 0;
            }
        }
    }
    return BLE_SUCCESS;
}

ble_sts_t blc_ll_readMonitoredAdvertisersListSize(hci_le_readMonitoredAdvertisersListSizeStatusParam_t *pRetParam)
{
    pRetParam->number =  MONITORING_ADVERTISERS_LIST_MAX_ENTRIES;
    return BLE_SUCCESS;
}

ble_sts_t blc_ll_removeDeviceFromMonitoredAdvertisersList(u8 adr_type, u8 *addr)
{
    monitored_advertisers_list_t *pMAL = NULL;

    if (adr_type > 0x01) {
        tlkapi_printf(DBG_SCAN_MON_ADV_EN || 1, "[MON_ADV] Error!!! Address Type = 0x%X\n", adr_type);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    int i = 0;
    for (; i < MONITORING_ADVERTISERS_LIST_MAX_ENTRIES; i++) {
        pMAL = &mon_adv_list[i];
        if (pMAL->timeout && !smemcmp(pMAL->address, addr, BLE_ADDR_LEN) && pMAL->address_type == adr_type) {
            tlkapi_printf(DBG_SCAN_MON_ADV_EN, "[MON_ADV] Remove Mon_adv_list[%d]\n", i);
            tlkapi_send_string_data(DBG_SCAN_MON_ADV_EN, "[MON_ADV] Remove mon_adv", pMAL->address, BLE_ADDR_LEN);
            pMAL-> timeout = 0;   //Clear key
            return BLE_SUCCESS;
        }
    }
    return HCI_ERR_INVALID_HCI_CMD_PARAMS;
}

ble_sts_t blc_ll_clearMonitoredAdvertisersList(void)
{
    for (int i = 0; i < MONITORING_ADVERTISERS_LIST_MAX_ENTRIES; i++) {
        mon_adv_list[i].timeout = 0;
    }

    return BLE_SUCCESS;
}

//FSM
void blt_ll_mon_adv_state_machine(u8 addr_type, u8* addr, s8 rssi)
{
    //Check mon_adv_en outside: No need to jump into this API.
    //Check mon_adv_en inside: Good for module de-couple, safe for calling several places.
    if (!mon_adv_en) {
        return;
    }

    monitored_advertisers_list_t *pMAL = NULL;
    u8 ADDR_ALL_ZERO[6] = {0};
    int i = 0;
    for (; i < MONITORING_ADVERTISERS_LIST_MAX_ENTRIES; i++) {
        pMAL = &mon_adv_list[i];

        if (pMAL->timeout) {
            if (pMAL->timer && pMAL->fsm == MON_ADV_STATUS_NOT_AWAITING_RSSI_THRESHOLD_HIGH) {
                if (clock_time_exceed(pMAL->timer, pMAL->timeout*1000*1000)) {
                    tlkapi_printf(DBG_SCAN_MON_ADV_EN, "[MON_ADV] Timer expires, Loss of Signal event generated -> Awaiting Status.\n");
                    hci_le_monitoredAdvertisersReport_evt(pMAL->address_type, pMAL->address, RSSI_BELOW_LOW_RSSI_THRESHOLD_TIMEOUT);
                    pMAL->timer = 0;
                    pMAL->fsm = MON_ADV_STATUS_AWAITING_RSSI_GREATER_THAN_RSSI_THRESHOLD_HIGH;
                    continue;
                }
            }

            if (pMAL->address[0] == addr[0] && !smemcmp(pMAL->address, addr, BLE_ADDR_LEN) && ((addr_type & PEERATYPE_RANDOM_MASK) == pMAL->address_type)) {  //        //pMAL->address[0] == addr[0] is for optimize running time
                tlkapi_printf(DBG_SCAN_MON_ADV_EN, "[MON_ADV] Dev[%d], rssi = %d\n", i, rssi);
                if (pMAL->fsm == MON_ADV_STATUS_NOT_AWAITING_RSSI_THRESHOLD_HIGH) {
                    if (rssi > pMAL->RSSI_Threshold_Low) {
                        tlkapi_printf(DBG_SCAN_MON_ADV_EN, "[MON_ADV] NOT Awaiting Status: Reset Timer(rssi = %d)\n", rssi);
                        pMAL->timer = clock_time()|1;
                    }
                } else if (pMAL->fsm == MON_ADV_STATUS_AWAITING_RSSI_GREATER_THAN_RSSI_THRESHOLD_HIGH) {
                    if (rssi > pMAL->RSSI_Threshold_High) {
                        tlkapi_printf(DBG_SCAN_MON_ADV_EN, "[MON_ADV] Awaiting Status->NOT Awaiting Status(rssi=%d), Reset Timer\n", rssi);
                        pMAL->timer = clock_time()|1;
                        hci_le_monitoredAdvertisersReport_evt(addr_type, pMAL->address, SRRI_GREATER_HIGH_THRESHOLD);
                        pMAL->fsm = MON_ADV_STATUS_NOT_AWAITING_RSSI_THRESHOLD_HIGH;
                    }
                }
                break;
            }
        }
        if (i == MONITORING_ADVERTISERS_LIST_MAX_ENTRIES) {
            tlkapi_send_string_data(DBG_SCAN_MON_ADV_EN, "[MON_ADV] Not found", addr, BLE_ADDR_LEN);
            tlkapi_printf(DBG_SCAN_MON_ADV_EN, "[MON_ADV] RSSI = %d\n", rssi);
        }
    }
}

int blt_mon_adv_mainloop_task(int flag, u8* p)
{
    if (flag == FLAG_MODULE_RESET) {
        blc_ll_clearMonitoredAdvertisersList();
        blc_ll_monitoringAdvertisersEnable(0);
    } else if (flag == FLAG_MON_ADV_DATA_REPORT_EXTADV) {
        extAdvEvt_info_t *pExtAdvInfo = (extAdvEvt_info_t *)p;
        blt_ll_mon_adv_state_machine(pExtAdvInfo->address_type, pExtAdvInfo->address, pExtAdvInfo->rssi);
    } else if (flag == FLAG_MON_ADV_DATA_REPORT_LEGADV) {
        //Leg Adv Info{adv_type, rssi, advA}
        blt_ll_mon_adv_state_machine((u8)*p, p+2, (s8)*(p+1));
    }

    return BLE_SUCCESS;
}

ble_sts_t blc_ll_initMonitoringAdvertisers(int mon_adv_max_num)
{
    //TODO: use mon_adv_max_num to config the buffer mon_adv_list instead of MONITORING_ADVERTISERS_LIST_MAX_ENTRIES.

    ll_mon_adv_mlp_task_cb  = blt_mon_adv_mainloop_task;

    return BLE_SUCCESS;
}

#endif
