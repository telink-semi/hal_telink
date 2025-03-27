/********************************************************************************************************
 * @file    gap_event.c
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"



_attribute_ble_data_retention_  u32     gap_eventMask = GAP_EVT_MASK_DEFAULT;  //11 event in core_4.2

_attribute_ble_data_retention_  gap_event_handler_t     blc_gap_event_handler = NULL;

void    blc_gap_setEventMask(u32 evtMask)
{
    gap_eventMask = evtMask;
}


void blc_gap_registerHostEventHandler (gap_event_handler_t  handler)
{
    blc_gap_event_handler = handler;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
int blc_gap_send_event (u32 h, u8 *para, int n)
{
    if(blt_gap_hci_event_handler)
    {
        blt_gap_hci_event_handler(h, para, n);
    }

    if (blc_gap_event_handler)
    {
        return blc_gap_event_handler (h, para, n);      //app_host_event_callback
    }
    return 1;
}
