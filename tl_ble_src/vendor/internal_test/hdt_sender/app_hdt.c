/********************************************************************************************************
 * @file    app_hdt.c
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
#include "app_hdt.h"
#include "app_buffer.h"
#include "app_parse_char.h"
#include "stack/ble/ble.h"

#if (INTER_TEST_MODE == TEST_HDT_SENDER)

#define HOST_MALLOC_BUFF_SIZE (4 * 1024)

static u8 hostMallocBuffer[HOST_MALLOC_BUFF_SIZE];

hdt_app_control_t hdt_app_ctrl;
/**
 * @brief      SDP end handler.
 * @param[in]  connHandle       ACL connect handle
 * @param[in]  *pData           Pointer to sdp end data buffer
 * @param[in]  dataLen          length of data
 * @return     0x00
 */
static int app_prf_sdp_end(u16 connHandle, u8 *pData, u16 dataLen)
{
    (void)connHandle;
    (void)dataLen;
    blc_prf_sdpEndEvt_t *evt = (blc_prf_sdpEndEvt_t *)pData;
    tlkapi_printf(APP_LOG_EN, "[APP][HDT] profile sdp end %x.\r\n",connHandle);
    return 0;
}

static const app_prf_evtCb_t hdtCentralEvt[] = {
    {PRF_EVTID_CLIENT_SDP_END, app_prf_sdp_end},
};

PRF_EVT_CB(hdtCentralEvt)

/**
 * @brief       BLE higher data throughput init.
 * @param      None
 * @return     None
 */
void app_higher_data_throughput_init(void)
{
    blc_prf_initialModule(app_prf_eventCb, hostMallocBuffer, HOST_MALLOC_BUFF_SIZE);
}
#endif
