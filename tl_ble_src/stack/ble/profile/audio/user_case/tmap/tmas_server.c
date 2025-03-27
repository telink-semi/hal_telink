/********************************************************************************************************
 * @file    tmas_server.c
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

static void blt_tmass_serviceInit(const blc_tmass_regParam_t *param);

_attribute_ble_data_retention_
blc_tmas_server_ctrl_t tmas_server_ctrl = {
    .process = {
        .pNext = NULL,
        .id = AUDIO_TMAS_SERVER,
        .usedAclRole = 0,
        .init = blt_tmass_init,
        .connect = NULL,
        .discov = NULL,
        .loop = NULL,
    },
};

void blc_audio_registerTMASControlServer(const blc_tmass_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t*)&tmas_server_ctrl, param);
}

blc_tmas_server_t* blt_tmass_getCtrl(u16 connHandle)
{
#if (0)
    int ret = blt_prf_getAclRole(connHandle);
    if(ret < 0 || (0 && ret == ACL_ROLE_CENTRAL)) {
        BLT_TMAS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if(ret >= 0){
            /* TMAP Client GAP Central and Peripheral ? */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_TMAS_SERVER, ret);
        }

        return NULL;
    }
#else
    (void)connHandle;
#endif

    return &tmas_server_ctrl.tmasServer;
}

#define TMASS_TMAP_ROLE_HANDLE(connHandle)      (blt_tmass_getCtrl(connHandle)->tmapRoleHandle)

int blt_tmass_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_tmas_server_t)), blc_tmas_server_t);
#endif

    if(initType == PRF_PROC_INIT) {
        BLT_TMAS_LOG("server init");
        blc_svc_addTmasGroup();
        blt_tmass_serviceInit(param);
    }
//  else if (initType == PRF_PROC_DEINIT) {
//      blc_svc_removeTmasGroup();
//      BLT_TMAS_LOG("server deinit");
//  }
    return 0;
}

static u16* blc_tmass_getTmapRole(u16 connHandle)
{
    return (u16*)blc_gatts_getAttributeValueByHandle(connHandle, TMASS_TMAP_ROLE_HANDLE(connHandle));
}

void blc_tmass_setTmapRole(u16 role)
{
    u16 *pRole = blc_tmass_getTmapRole(0xFFFF);
    if(!pRole)      return ;

    *pRole = role & (~BLC_TMAP_ROLE_RFU);
}

static void blt_tmass_initTmapRoleChar(atts_foundCharParam_t * p, void *input)
{
    blc_tmas_server_t *tmass = (blc_tmas_server_t*)input;
    if(p->num > 0)
    {
        BLT_TMAS_LOG("ERR: TMAP Role characteristic too many");
        return ;
    }
    tmass->tmapRoleHandle = p->charHandle;
}

static const atts_findCharList_t tmassChar[] = {
    {
        .charUuid = characteristicTmapRoleUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_tmass_initTmapRoleChar,
    },
};

static void blt_tmass_serviceInit(const blc_tmass_regParam_t *param)
{
    blc_tmas_server_t *server = blt_tmass_getCtrl(0xFFFF);
    blc_atts_findCharacteristicByServiceUuid(serviceTelephonyAndMediaAudioUuid, ATT_16_UUID_LEN, tmassChar, ARRAY_SIZE(tmassChar), server);
    BLT_TMAS_LOG("Handle information, TMAP Role:0x%x", server->tmapRoleHandle);
    const blc_tmass_regParam_t* tmasParam = param;

    if(!tmasParam)  return ;    //NULL return

    blc_tmass_setTmapRole(tmasParam->role);
}



