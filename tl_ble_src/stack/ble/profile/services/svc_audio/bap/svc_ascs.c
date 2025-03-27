/********************************************************************************************************
 * @file    svc_ascs.c
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
#include "stack/ble/ble.h"

#define ASCS_START_HDL                  SERVICE_AUDIO_STREAM_CONTROL_HDL

#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 0
static const u8 ascsSinkASEValue[APP_AUDIO_ASCSS_SINK_ASE_CNT][2] = {
    {APP_AUDIO_ASCSS_SINK_ASE_ID + 0, 0x00},        //idle
#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 1
    {APP_AUDIO_ASCSS_SINK_ASE_ID + 1, 0x00},        //idle
#endif
#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 2
    {APP_AUDIO_ASCSS_SINK_ASE_ID + 2, 0x00},        //idle
#endif
#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 3
    {APP_AUDIO_ASCSS_SINK_ASE_ID + 3, 0x00},        //idle
#endif
#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 4
    {APP_AUDIO_ASCSS_SINK_ASE_ID + 4, 0x00},        //idle
#endif
#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 5
    {APP_AUDIO_ASCSS_SINK_ASE_ID + 5, 0x00},        //idle
#endif
#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 6
    {APP_AUDIO_ASCSS_SINK_ASE_ID + 6, 0x00},        //idle
#endif
#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 7
    {APP_AUDIO_ASCSS_SINK_ASE_ID + 7, 0x00},        //idle
#endif
};
#endif

#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 0
static const u8 ascsSourceASEValue[APP_AUDIO_ASCSS_SRC_ASE_CNT][2] = {
    {APP_AUDIO_ASCSS_SRC_ASE_ID + 0, 0x00},     //idle
#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 1
    {APP_AUDIO_ASCSS_SRC_ASE_ID + 1, 0x00},     //idle
#endif
#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 2
    {APP_AUDIO_ASCSS_SRC_ASE_ID + 2, 0x00},     //idle
#endif
#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 3
    {APP_AUDIO_ASCSS_SRC_ASE_ID + 3, 0x00},     //idle
#endif
#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 4
    {APP_AUDIO_ASCSS_SRC_ASE_ID + 4, 0x00},     //idle
#endif
#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 5
    {APP_AUDIO_ASCSS_SRC_ASE_ID + 5, 0x00},     //idle
#endif
#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 6
    {APP_AUDIO_ASCSS_SRC_ASE_ID + 6, 0x00},     //idle
#endif
#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 7
    {APP_AUDIO_ASCSS_SRC_ASE_ID + 7, 0x00},     //idle
#endif
};
#endif

static const u16 ascsASEValueLen = 2;

#define ATTS_CHAR_ASCS_ASE_UUID(uuid, value)                    \
            ATTS_CHARACTERISTIC_DECLARATIONS(charPropReadNotfiy),\
            ATTS_CHAR_UUID_DEFINE(ATT_PERMISSIONS_ENCRYPT_READ, uuid, ascsASEValueLen, 2, value, ATTS_SET_READ_CBACK)

#define ATTS_CHAR_ASCS_SINK_ASE(value)                          ATTS_CHAR_ASCS_ASE_UUID(characteristicSinkAseUuid, value)
#define ATTS_CHAR_ASCS_SRC_ASE(value)                           ATTS_CHAR_ASCS_ASE_UUID(characteristicSourceAseUuid, value)

/*
 * @brief the structure for default ASCS service List.
 */
static const atts_attribute_t ascsList[] =
{
    ATTS_PRIMARY_SERVICE(serviceAudioStreamControlUuid),

    //ASE Control Point
    ATTS_CHAR_UUID_ENCR_WRITE_NULL(charPropWriteWriteWithoutNotify, characteristicAseControlPointUuid),
    ATTS_COMMON_CCC_DEFINE,

    //Sink ASE
#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 0
    ATTS_CHAR_ASCS_SINK_ASE(&ascsSinkASEValue[0][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 1
    ATTS_CHAR_ASCS_SINK_ASE(&ascsSinkASEValue[1][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 2
    ATTS_CHAR_ASCS_SINK_ASE(&ascsSinkASEValue[2][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 3
    ATTS_CHAR_ASCS_SINK_ASE(&ascsSinkASEValue[3][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 4
    ATTS_CHAR_ASCS_SINK_ASE(&ascsSinkASEValue[4][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 5
    ATTS_CHAR_ASCS_SINK_ASE(&ascsSinkASEValue[5][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 6
    ATTS_CHAR_ASCS_SINK_ASE(&ascsSinkASEValue[6][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SINK_ASE_CNT > 7
    ATTS_CHAR_ASCS_SINK_ASE(&ascsSinkASEValue[7][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

    //Source ASE
#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 0
    ATTS_CHAR_ASCS_SRC_ASE(&ascsSourceASEValue[0][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 1
    ATTS_CHAR_ASCS_SRC_ASE(&ascsSourceASEValue[1][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 2
    ATTS_CHAR_ASCS_SRC_ASE(&ascsSourceASEValue[2][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 3
    ATTS_CHAR_ASCS_SRC_ASE(&ascsSourceASEValue[3][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 4
    ATTS_CHAR_ASCS_SRC_ASE(&ascsSourceASEValue[4][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 5
    ATTS_CHAR_ASCS_SRC_ASE(&ascsSourceASEValue[5][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 6
    ATTS_CHAR_ASCS_SRC_ASE(&ascsSourceASEValue[6][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if APP_AUDIO_ASCSS_SRC_ASE_CNT > 7
    ATTS_CHAR_ASCS_SRC_ASE(&ascsSourceASEValue[7][0]),
    ATTS_COMMON_CCC_DEFINE,
#endif

};

/*
 * @brief the structure for default ASCS service group.
 */
_attribute_ble_data_retention_
static atts_group_t svcAscsGroup =
{
    NULL,
    ascsList,
    NULL,
    NULL,
    ASCS_START_HDL,
    0,
};

/**
 * @brief      for user add default ASCS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addAscsGroup(void)
{
    svcAscsGroup.endHandle = svcAscsGroup.startHandle+ARRAY_SIZE(ascsList)-1;
    blc_gatts_addAttributeServiceGroup(&svcAscsGroup);
}

/**
 * @brief      for user remove default ASCS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeAscsGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(ASCS_START_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in ASCS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_ascsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
    svcAscsGroup.readCback = readCback;
    svcAscsGroup.writeCback = writeCback;
}
