/********************************************************************************************************
 * @file    ots_client.c
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
#if (0)

    #include "tl_common.h"
    #include "drivers.h"
    #include "stack/ble/ble.h"


//TODO:
blc_ots_client_t otsTest;

static blc_ots_client_t *blt_otsc_getClientInst(u16 connHandle)
{
    return &otsTest;
}

void blt_otsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
}

static bool blt_otsc_foundService(u16 connHandle, u16 startHandle, u16 endHandle)
{
    blc_ots_client_t *client = blt_otsc_getClientInst(connHandle);

    client->ntfInput.startHdl     = startHandle;
    client->ntfInput.endHdl       = endHandle;
    client->ntfInput.ntfOrIndFunc = blt_otsc_dataInput;
    BLT_OTS_LOG("connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);
    return true;
}

static void blt_otsc_foundFeatureChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_ots_client_t *client = blt_otsc_getClientInst(connHandle);
    client->featureHdl       = valueHandle;
    BLT_OTS_LOG("feature ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_otsc_featureStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    //  blc_ots_client_t* client = blt_otsc_getClientInst(connHandle);
    //  *read = (u8*)&client->;
    *readLen     = NULL;
    *readMaxSize = 0;
}

static void blt_otsc_foundObjectNameChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_ots_client_t *client = blt_otsc_getClientInst(connHandle);
    client->objectNameHdl    = valueHandle;
    BLT_OTS_LOG("object name ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_otsc_objectNameStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    //  blc_ots_client_t* client = blt_otsc_getClientInst(connHandle);
    //  *read = (u8*)&client->;
    *readLen     = NULL;
    *readMaxSize = 0;
    *rdCbFunc    = NULL;
}

static void blt_otsc_foundObjectTypeChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_ots_client_t *client = blt_otsc_getClientInst(connHandle);
    client->objectTypeHdl    = valueHandle;
    BLT_OTS_LOG("object type ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_otsc_objectTypeStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    //  blc_ots_client_t* client = blt_otsc_getClientInst(connHandle);
    //  *read = (u8*)&client->;
    *readLen     = NULL;
    *readMaxSize = 0;
    *rdCbFunc    = NULL;
}

static void blt_otsc_foundObjectSizeChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_ots_client_t *client = blt_otsc_getClientInst(connHandle);
    client->objectSizeHdl    = valueHandle;
    BLT_OTS_LOG("object size ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_otsc_objectSizeStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    //  blc_ots_client_t* client = blt_otsc_getClientInst(connHandle);
    //  *read = (u8*)&client->;
    *readLen     = NULL;
    *readMaxSize = 0;
    *rdCbFunc    = NULL;
}

static void blt_otsc_foundObjectFirstCreatedChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_ots_client_t *client      = blt_otsc_getClientInst(connHandle);
    client->objectFirstCreatedHdl = valueHandle;
    BLT_OTS_LOG("object first created ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_otsc_objectFirstCreatedStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    //  blc_ots_client_t* client = blt_otsc_getClientInst(connHandle);
    //  *read = (u8*)&client->;
    *readLen     = NULL;
    *readMaxSize = 0;
    *rdCbFunc    = NULL;
}

static void blt_otsc_foundObjectLastModifiedChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_ots_client_t *client      = blt_otsc_getClientInst(connHandle);
    client->objectLastModifiedHdl = valueHandle;
    BLT_OTS_LOG("object last modified ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_otsc_objectLastModifiedStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    //  blc_ots_client_t* client = blt_otsc_getClientInst(connHandle);
    //  *read = (u8*)&client->;
    *readLen     = NULL;
    *readMaxSize = 0;
    *rdCbFunc    = NULL;
}

static void blt_otsc_foundObjectIdChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_ots_client_t *client = blt_otsc_getClientInst(connHandle);
    client->objectIdHdl      = valueHandle;
    BLT_OTS_LOG("object id ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_otsc_objectIdStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    //  blc_ots_client_t* client = blt_otsc_getClientInst(connHandle);
    //  *read = (u8*)&client->;
    *readLen     = NULL;
    *readMaxSize = 0;
    *rdCbFunc    = NULL;
}

static void blt_otsc_foundObjectPropertiesChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_ots_client_t *client    = blt_otsc_getClientInst(connHandle);
    client->objectPropertiesHdl = valueHandle;
    BLT_OTS_LOG("object properties ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_otsc_objectPropertiesStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    //  blc_ots_client_t* client = blt_otsc_getClientInst(connHandle);
    //  *read = (u8*)&client->;
    *readLen     = NULL;
    *readMaxSize = 0;
    *rdCbFunc    = NULL;
}

static void blt_otsc_foundObjectActionControlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_ots_client_t *client            = blt_otsc_getClientInst(connHandle);
    client->objectActionControlPointHdl = valueHandle;
    BLT_OTS_LOG("object action control point ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_otsc_foundObjectListControlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_ots_client_t *client          = blt_otsc_getClientInst(connHandle);
    client->objectListControlPointHdl = valueHandle;
    BLT_OTS_LOG("object list control point ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_otsc_foundObjectListFilterChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_ots_client_t *client    = blt_otsc_getClientInst(connHandle);
    client->objectListFilterHdl = valueHandle;
    BLT_OTS_LOG("object list filter ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_otsc_objectListFilterStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    //  blc_ots_client_t* client = blt_otsc_getClientInst(connHandle);
    //  *read = (u8*)&client->;
    *readLen     = NULL;
    *readMaxSize = 0;
    *rdCbFunc    = NULL;
}

static void blt_otsc_foundObjectChangedChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_ots_client_t *client = blt_otsc_getClientInst(connHandle);
    client->objectChangedHdl = valueHandle;
    BLT_OTS_LOG("object changed ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static const blc_gapc_discChar_t otsChar[] = {
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OTS_FEATURE),
     .cfun      = blt_otsc_foundFeatureChar,
     .rfun      = blt_otsc_featureStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_NAME),
     .cfun      = blt_otsc_foundObjectNameChar,
     .rfun      = blt_otsc_objectNameStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_TYPE),
     .cfun      = blt_otsc_foundObjectTypeChar,
     .rfun      = blt_otsc_objectTypeStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_SIZE),
     .cfun      = blt_otsc_foundObjectSizeChar,
     .rfun      = blt_otsc_objectSizeStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_FIRST_CREATED),
     .cfun      = blt_otsc_foundObjectFirstCreatedChar,
     .rfun      = blt_otsc_objectFirstCreatedStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_LAST_MODIFIED),
     .cfun      = blt_otsc_foundObjectLastModifiedChar,
     .rfun      = blt_otsc_objectLastModifiedStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_ID),
     .cfun      = blt_otsc_foundObjectIdChar,
     .rfun      = blt_otsc_objectIdStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_PROPERTIES),
     .cfun      = blt_otsc_foundObjectPropertiesChar,
     .rfun      = blt_otsc_objectPropertiesStartRead,
     },
    {
     .subscribeInd = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_ACTION_CONTROL_POINT),
     .cfun         = blt_otsc_foundObjectActionControlPointChar,
     },
    {
     .subscribeInd = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_LIST_CONTROL_POINT),
     .cfun         = blt_otsc_foundObjectListControlPointChar,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_LIST_FILTER),
     .cfun      = blt_otsc_foundObjectListFilterChar,
     .rfun      = blt_otsc_objectListFilterStartRead,
     },
    {
     .subscribeInd = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_OBJECT_CHANGED),
     .cfun         = blt_otsc_foundObjectChangedChar,
     },
};

const blc_gapc_discInclude_t discOts = {
    .uuid           = UUID16_INIT(SERVICE_UUID_OBJECT_TRANSFER),
    .characteristic = {
                       .size           = ARRAY_SIZE(otsChar),
                       .characteristic = otsChar,
                       },
    .ifun = blt_otsc_foundService,
};


#endif
