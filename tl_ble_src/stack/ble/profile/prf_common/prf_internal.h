#pragma once

#define PRF_RFU_CONN_HANDLE                 0xFFFF

#define BLT_COMMON_LOG(fmt, ...)            BLC_BASIC_PRF_LOG(DBG_PRF_MASK_COMMON_LOG, "[ ]"fmt, ##__VA_ARGS__)

#define CHECK_PTR_NUL(ptr)                                  (ptr == NULL)

#define BLT_PRF_CHECK_NULL_PTR1(ptr1)                       if(CHECK_PTR_NUL(ptr1))     return PRF_COMMON_ERR_INPUT_NULL
#define BLT_PRF_CHECK_NULL_PTR2(ptr1, ptr2)                 if(CHECK_PTR_NUL(ptr1) || CHECK_PTR_NUL(ptr2))      return PRF_COMMON_ERR_INPUT_NULL
#define BLT_PRF_CHECK_NULL_PTR3(ptr1, ptr2, ptr3)           if(CHECK_PTR_NUL(ptr1) || CHECK_PTR_NUL(ptr2) || CHECK_PTR_NUL(ptr3))       return PRF_COMMON_ERR_INPUT_NULL

#define BLT_PRF_CHECK_NULL_PTR(...)                         VARARG(BLT_PRF_CHECK_NULL_PTR, __VA_ARGS__)

#define BLC_PRF_SDP_DISCOVERY(connHandle, PRF, prf, CLIENT_ID)  \
        if(blc_prf_checkDiscoveryBusy(connHandle)) {\
            return 0; \
        } \
        if(blc_prf_checkReconnectFlag(connHandle)) { \
            blc_##prf##_client_t *client = blt_##prf##c_getClientInst(connHandle);\
            if(client->ntfInput.startHdl) {\
                if(BLC_##PRF##_START_RECONN(connHandle) == BLE_SUCCESS) { \
                    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, CLIENT_ID, client->ntfInput.startHdl, client->ntfInput.endHdl);  \
                    blc_prf_setDiscoveryStatusBusy(connHandle); \
                    BLT_COMMON_LOG("SDP start reconnect connect handle: 0x%x", connHandle);     \
                }\
            }\
            else {\
                BLT_COMMON_LOG("ATT information not found, connect handle is 0x%x", connHandle);    \
                blc_prf_sendServiceDiscoveryFailEvent(connHandle, CLIENT_ID);   \
                blc_prf_setDiscoveryStatusFinish(connHandle);   \
            }\
        }else if(BLC_##PRF##_START_SDP(connHandle) == BLE_SUCCESS) { \
            blc_prf_setDiscoveryStatusBusy(connHandle);\
            BLT_COMMON_LOG("sdp start discovery connect handle is 0x%x", connHandle);   \
        }   \
        return 0;

#define BLC_PRF_NV_STORE_BASE(connHandle, prf, attrHandle, CLIENT_ID)       \
        if(client->ntfInput.startHdl) {\
            blt_##prf##_nv_info_t nvInfo;   \
            blt_prf_storeClientHdl(&nvInfo.att, client, &client->attrHandle);\
            U8_TO_STREAM(param->dataPtr, sizeof(blt_##prf##_nv_info_t));    \
            U8_TO_STREAM(param->dataPtr, CLIENT_ID);    \
            pNvInfo = (blt_##prf##_nv_info_t*) param->dataPtr;  \
            STR_TO_STREAM(param->dataPtr, &nvInfo, sizeof(blt_##prf##_nv_info_t));  \
            param->currentTotalLen += 2 + sizeof(blt_##prf##_nv_info_t);    \
        }\

#define BLC_PRF_NV_LOAD_BASE(connHandle, prf, attrHandle)\
        pNvInfo = (blt_##prf##_nv_info_t*) param->dataPtr;  \
        blt_##prf##_nv_info_t *nvInfo = (blt_##prf##_nv_info_t*)param->dataPtr; \
        blt_prf_loadClientHdl(client, &nvInfo->att, &client->attrHandle);   \
        client->ntfInput.ntfOrIndFunc = blt_##prf##c_dataInput; \
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);

#define BLC_PRF_NV_STORE(connHandle, CLIENT_ID, prf, attrHandle)        \
        blt_##prf##_nv_info_t *pNvInfo = NULL;  \
        blc_##prf##_client_t* client = blt_##prf##c_getClientInst(connHandle); \
        if(client == NULL)      return 0;   \
        if(nvState == PRF_NV_STATE_STORE)   {   \
            BLC_PRF_NV_STORE_BASE(connHandle, prf, attrHandle, CLIENT_ID)   \
        }\
        else if(nvState == PRF_NV_STATE_LOAD) { \
            BLC_PRF_NV_LOAD_BASE(connHandle, prf, attrHandle)   \
        }\
        (void)pNvInfo;

/////////will delete///////
#define BLC_PRF_SDP_DISCOVERY1(connHandle, PRF, prf, CLIENT_ID) \
        if(blc_prf_checkDiscoveryBusy(connHandle)) {\
            return 0; \
        } \
        if(blc_prf_checkReconnectFlag(connHandle)) { \
            struct blc_##prf##_client *client = blt_##prf##c_getClientInst(connHandle);\
            if(client->ntfInput.startHdl) {\
                if(BLC_##PRF##_START_RECONN(connHandle) == BLE_SUCCESS) { \
                    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, CLIENT_ID, client->ntfInput.startHdl, client->ntfInput.endHdl);  \
                    blc_prf_setDiscoveryStatusBusy(connHandle); \
                    BLT_COMMON_LOG("SDP start reconnect connect handle: 0x%x", connHandle);     \
                }\
            }\
            else {\
                BLT_COMMON_LOG("ATT information not found, connect handle is 0x%x", connHandle);    \
                blc_prf_sendServiceDiscoveryFailEvent(connHandle, CLIENT_ID);   \
                blc_prf_setDiscoveryStatusFinish(connHandle);   \
            }\
        }else if(BLC_##PRF##_START_SDP(connHandle) == BLE_SUCCESS) { \
            blc_prf_setDiscoveryStatusBusy(connHandle);\
            BLT_COMMON_LOG("sdp start discovery connect handle is 0x%x", connHandle);   \
        }   \
        return 0

#define BLC_PRF_NV_STORE_BASE1(connHandle, prf, attrHandle, CLIENT_ID)      \
        if(client->ntfInput.startHdl) {\
            struct blt_##prf##_nv_info nvInfo;  \
            blt_prf_storeClientHdl(&nvInfo.att, client, &client->attrHandle);\
            U8_TO_STREAM(param->dataPtr, sizeof(struct blt_##prf##_nv_info));   \
            U8_TO_STREAM(param->dataPtr, CLIENT_ID);    \
            pNvInfo = (struct blt_##prf##_nv_info*) param->dataPtr; \
            STR_TO_STREAM(param->dataPtr, &nvInfo, sizeof(struct blt_##prf##_nv_info)); \
            param->currentTotalLen += 2 + sizeof(struct blt_##prf##_nv_info);   \
        }\

#define BLC_PRF_NV_LOAD_BASE1(connHandle, prf, attrHandle)\
        pNvInfo = (struct blt_##prf##_nv_info*) param->dataPtr; \
        struct blt_##prf##_nv_info *nvInfo = (struct blt_##prf##_nv_info*)param->dataPtr;   \
        blt_prf_loadClientHdl(client, &nvInfo->att, &client->attrHandle);   \
        client->ntfInput.ntfOrIndFunc = blt_##prf##c_dataInput; \
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);

#define BLC_PRF_NV_STORE1(connHandle, CLIENT_ID, prf, attrHandle)       \
        struct blt_##prf##_nv_info *pNvInfo = NULL; \
        struct blc_##prf##_client *client = blt_##prf##c_getClientInst(connHandle); \
        if(client == NULL)      return 0;   \
        if(nvState == PRF_NV_STATE_STORE)   {   \
            BLC_PRF_NV_STORE_BASE1(connHandle, prf, attrHandle, CLIENT_ID)  \
        }\
        else if(nvState == PRF_NV_STATE_LOAD) { \
            BLC_PRF_NV_LOAD_BASE1(connHandle, prf, attrHandle)  \
        }\
        (void)pNvInfo
//////////////

#define BLC_BASIC_NV_STORE(connHandle, PRF, prf, attrHandle)    BLC_PRF_NV_STORE1(connHandle, PRF##_CLIENT, prf, attrHandle)
#define BLC_BASIC_SDP_DISCOVERY(connHandle, PRF, prf)           BLC_PRF_SDP_DISCOVERY1(connHandle, PRF, prf, PRF##_CLIENT)

#define BLT_PRF_WRITE_ATTR_START(prf, PRF, HDL)                 \
        if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {    \
            BLT_##PRF##_LOG("ERR: ACL handle invalid");                 \
            return HCI_ERR_UNKNOWN_CONN_ID;                         \
        }   \
        \
        struct blc_##prf##_client* client = blt_##prf##c_getClientInst(connHandle); \
        \
        if(client == NULL || client->HDL == 0)  \
        {   \
            BLT_##PRF##_LOG("ERR: handle not set"); \
            return PRF_COMMON_ERR_ATTR_HANDLE_NOT_FOUND;    \
        }   \
        \
        gapc_write_cfg_t pGapWrCfg; \
        pGapWrCfg.func = blc_prf_writeAttributeValueDefaultCallback;    \
        pGapWrCfg.handle = client->HDL; \
        pGapWrCfg.cbData = NULL

#define BLT_PRF_WRITE_ATTR_VALUE(prf, PRF, HDL, value, len)     \
        BLT_PRF_WRITE_ATTR_START(prf, PRF, HDL);    \
        pGapWrCfg.data = (u8*)&value;   \
        pGapWrCfg.length = len; \
        pGapWrCfg.withoutRsp = false;   \
        \
        return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, writeCb)

#define BLT_PRF_WRITE_ATTR_VALUE_WITHOUT_RSP(prf, PRF, HDL, value, len)     \
        BLT_PRF_WRITE_ATTR_START(prf, PRF, HDL);    \
        pGapWrCfg.data = (u8*)&value;   \
        pGapWrCfg.length = len; \
        pGapWrCfg.withoutRsp = true;    \
        \
        return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, NULL)

#define BLT_PRF_WRITE_ATTR_VALUE_WITH_LEN(prf, PRF, HDL, value, len)                \
        BLT_PRF_WRITE_ATTR_START(prf, PRF, HDL);    \
        pGapWrCfg.data = (u8*)value;    \
        pGapWrCfg.length = len; \
        pGapWrCfg.withoutRsp = false;   \
        \
        return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, writeCb)

#define BLT_PRF_WRITE_ATTR_VALUE_WITHOUT_RSP_WITH_LEN(prf, PRF, HDL, value, len)                \
        BLT_PRF_WRITE_ATTR_START(prf, PRF, HDL);    \
        pGapWrCfg.data = (u8*)value;    \
        pGapWrCfg.length = len; \
        pGapWrCfg.withoutRsp = true;    \
        \
        return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, NULL)

#define BLT_PRF_WRITE_ATTR_VALUE_FIX_LEN(prf, PRF, HDL, value)              \
        BLT_PRF_WRITE_ATTR_START(prf, PRF, HDL);    \
        pGapWrCfg.data = (u8*)&value;   \
        pGapWrCfg.length = sizeof(value);   \
        pGapWrCfg.withoutRsp = false;   \
        \
        return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, writeCb)

#define BLT_PRF_WRITE_ATTR_VALUE_WITHOUT_RSP_FIX_LEN(prf, PRF, HDL, value)              \
        BLT_PRF_WRITE_ATTR_START(prf, PRF, HDL);    \
        pGapWrCfg.data = (u8*)&value;   \
        pGapWrCfg.length = sizeof(value);   \
        pGapWrCfg.withoutRsp = true;    \
        \
        return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, NULL)


#define BLT_PRF_READ_ATTR_START(prf, PRF, HDL)                  \
        if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {    \
            BLT_##PRF##_LOG("ERR: ACL handle invalid");                 \
            return HCI_ERR_UNKNOWN_CONN_ID;                         \
        }   \
        \
        struct blc_##prf##_client* client = blt_##prf##c_getClientInst(connHandle); \
        \
        if(client == NULL || client->HDL == 0)  \
        {   \
            BLT_##PRF##_LOG("ERR: handle not set"); \
            return PRF_ERR_INVALID_ATTR_HANDLE; \
        }   \
        \
        gapc_read_cfg_t pGapReCfg;  \
        pGapReCfg.func = blc_prf_readAttributeValueDefaultCallback; \
        pGapReCfg.handle = client->HDL

#define BLT_PRF_READ_ATTR_VALUE(prf, PRF, HDL, value, len)      \
        BLT_PRF_READ_ATTR_START(prf, PRF, HDL); \
        pGapReCfg.wBuff = (u8*)client->value;   \
        pGapReCfg.wBuffLen = (u16*)&client->len;    \
        pGapReCfg.maxLen = sizeof(client->value);   \
        \
        return blc_prf_readAttributeValue(connHandle, &pGapReCfg, readCb)

#define BLT_PRF_READ_ATTR_VALUE_WITH_LEN(prf, PRF, HDL, value, len)     \
        BLT_PRF_READ_ATTR_START(prf, PRF, HDL); \
        pGapReCfg.wBuff = (u8*)&client->value;  \
        pGapReCfg.wBuffLen = (u16*)&client->len;    \
        pGapReCfg.maxLen = sizeof(client->value);   \
        \
        return blc_prf_readAttributeValue(connHandle, &pGapReCfg, readCb)


#define BLT_PRF_READ_ATTR_VALUE_FIX_LEN(prf, PRF, HDL, value)               \
        BLT_PRF_READ_ATTR_START(prf, PRF, HDL); \
        pGapReCfg.wBuff = (u8*)&client->value;  \
        pGapReCfg.wBuffLen = NULL;  \
        pGapReCfg.maxLen = sizeof(client->value);   \
        \
        return blc_prf_readAttributeValue(connHandle, &pGapReCfg, readCb)

#define BLT_PRF_GET_ATTR_VALUE_COMMON(prf, characteristic)  \
    if(blt_ll_isAclHandleOutOfRange(connHandle))    \
        return PRF_HCI_ERROR_FLAG + HCI_ERR_UNKNOWN_CONN_ID;    \
    \
    struct blc_##prf##_client* client = blt_##prf##c_getClientInst(connHandle); \
    \
    if(client == NULL)  \
        return PRF_COMMON_ERR_CTRL_MODULE_NOT_FOUND;    \
    \
    if(client->characteristic##Hdl == 0)    \
        return PRF_COMMON_ERR_ATTR_HANDLE_NOT_FOUND

#define BLT_PRF_GET_ATTR_VALUE(prf, characteristic) \
    BLT_PRF_CHECK_NULL_PTR(characteristic, characteristic##Len);    \
    BLT_PRF_GET_ATTR_VALUE_COMMON(prf, characteristic); \
    *characteristic##Len = client->characteristic##Len; \
    memcpy(characteristic, client->characteristic, client->characteristic##Len);    \
    \
    return PRF_COMMON_SUCC

#define BLT_PRF_GET_ATTR_VALUE_WITH_LEN(prf, characteristic)    \
    BLT_PRF_CHECK_NULL_PTR(characteristic, characteristic##Len);    \
    BLT_PRF_GET_ATTR_VALUE_COMMON(prf, characteristic); \
    *characteristic##Len = client->characteristic##Len; \
    memcpy(characteristic, &client->characteristic, client->characteristic##Len);   \
    \
    return PRF_COMMON_SUCC

#define BLT_PRF_GET_ATTR_VALUE_FIX_LEN(prf, characteristic) \
    BLT_PRF_CHECK_NULL_PTR(characteristic); \
    BLT_PRF_GET_ATTR_VALUE_COMMON(prf, characteristic); \
    memcpy(characteristic, &client->characteristic, sizeof(client->characteristic));    \
    \
    return PRF_COMMON_SUCC


#define BLT_PRF_SDP_DISCOVERY_SERVICE(prf, PRF, CLIENT_ID)      \
static void blt_##prf##c_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle) \
{   \
    struct blc_##prf##_client* client = blt_##prf##c_getClientInst(connHandle); \
    if(count == 0xFF)   \
    {   \
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, CLIENT_ID);   \
        blc_prf_setDiscoveryStatusFinish(connHandle);   \
        BLT_##PRF##_LOG("ERR:not found "#PRF);  \
        return ;    \
    }   \
    \
    if(count == 0)  \
    {   \
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, CLIENT_ID);   \
        if(client) {    \
            blt_##prf##c_displayInfo(connHandle, client);   \
            blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);   \
        }   \
        blc_prf_setDiscoveryStatusFinish(connHandle);   \
        return ;    \
    }   \
    \
    if(client) {    \
        client->ntfInput.startHdl = startHandle;    \
        client->ntfInput.endHdl = endHandle;    \
        client->ntfInput.ntfOrIndFunc = blt_##prf##c_dataInput;\
    }   \
    \
    BLT_##PRF##_LOG("   INFO: "#PRF" connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);  \
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, CLIENT_ID, startHandle, endHandle);  \
}

#define BLT_BASIC_SDP_DISCOVERY_SERVICE(prf, PRF)           BLT_PRF_SDP_DISCOVERY_SERVICE(prf, PRF, PRF##_CLIENT)

#define BLT_PRF_DISCOVERY_WRITE_CHAR(prf, characteristicUuid, characteristic)   \
{\
    .uuid = UUID16_INIT(characteristicUuid),    \
    .cfun = blt_##prf##c_##characteristic##FoundChar,           \
}

#define BLT_PRF_DISCOVERY_IND_CHAR(prf, characteristicUuid, characteristic) \
{\
    .subscribeInd = true,   \
    .uuid = UUID16_INIT(characteristicUuid),    \
    .cfun = blt_##prf##c_##characteristic##FoundChar,           \
}

#define BLT_PRF_DISCOVERY_NOTIFY_CHAR(prf, characteristicUuid, characteristic)  \
{\
    .subscribeNtf = true,   \
    .uuid = UUID16_INIT(characteristicUuid),    \
    .cfun = blt_##prf##c_##characteristic##FoundChar,           \
}

#define BLT_PRF_DISCOVERY_READ_CHAR(prf, characteristicUuid, characteristic)    \
{\
    .readValue = true,  \
    .uuid = UUID16_INIT(characteristicUuid),    \
    .cfun = blt_##prf##c_##characteristic##FoundChar,           \
    .rfun = blt_##prf##c_##characteristic##StartRead,           \
}

#define BLT_PRF_DISCOVERY_READ_NOTIFY_CHAR(prf, characteristicUuid, characteristic) \
{\
    .subscribeNtf = true,   \
    .readValue = true,  \
    .uuid = UUID16_INIT(characteristicUuid),    \
    .cfun = blt_##prf##c_##characteristic##FoundChar,           \
    .rfun = blt_##prf##c_##characteristic##StartRead,           \
}

#define BLT_PRF_DISCOVERY_FOUND_CHAR(prf, PRF, characteristic)  \
    struct blc_##prf##_client* client = blt_##prf##c_getClientInst(connHandle); \
    \
    if(client == NULL)  \
    {   \
        BLT_##PRF##_LOG("ERR: "#PRF" client control module is NULL. connHandle[0x%x]", connHandle); \
        return ;    \
    }\
    \
    client->characteristic##Hdl = valueHandle;  \
    \
    BLT_##PRF##_LOG(#characteristic" connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle)

#define BLT_DEFINE_PRF_DISCOVERY_FOUND_CHAR(prf, PRF, characteristic)   \
static void blt_##prf##c_##characteristic##FoundChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)   \
{\
    (void)serviceCount; \
    BLT_PRF_DISCOVERY_FOUND_CHAR(prf, PRF, characteristic); \
}

#define BLT_DEFINE_PRF_DISCOVERY_FOUND_CHAR_PROP(prf, PRF, characteristic)  \
static void blt_##prf##c_##characteristic##FoundChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)   \
{\
    (void)serviceCount; \
    BLT_PRF_DISCOVERY_FOUND_CHAR(prf, PRF, characteristic); \
    client->characteristic##Prop = properties;  \
}

#define BLT_PRF_DISCOVERY_START_READ_COMMON(prf, PRF, characteristic)   \
    struct blc_##prf##_client* client = blt_##prf##c_getClientInst(connHandle); \
    \
    if(client == NULL)  \
    {   \
        return ;    \
    }   \
    \
    *readMaxSize = sizeof(client->characteristic);  \
    BLT_##PRF##_LOG(#characteristic" read info")

#define BLT_PRF_DISCOVERY_START_READ(prf, PRF, characteristic)      \
    BLT_PRF_DISCOVERY_START_READ_COMMON(prf, PRF, characteristic);\
    *read = (u8*)client->characteristic;    \
    *readLen = &client->characteristic##Len \

#define BLT_PRF_DISCOVERY_START_READ_WITH_LEN(prf, PRF, characteristic)     \
    BLT_PRF_DISCOVERY_START_READ_COMMON(prf, PRF, characteristic);\
    *read = (u8*)&client->characteristic;   \
    *readLen = &client->characteristic##Len \

#define BLT_PRF_DISCOVERY_START_READ_FIX_LEN(prf, PRF, characteristic)      \
    BLT_PRF_DISCOVERY_START_READ_COMMON(prf, PRF, characteristic);\
    *read = (u8*)&client->characteristic    \

#define BLT_DEFINE_PRF_DISCOVERY_START_READ_COMMON(prf, characteristic)     \
static void blt_##prf##c_##characteristic##StartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)


#define BLT_DEFINE_PRF_DISCOVERY_START_READ(prf, PRF, characteristic)   \
BLT_DEFINE_PRF_DISCOVERY_START_READ_COMMON(prf, characteristic) \
{\
    (void)attrHandle;   \
    (void)rdCbFunc;     \
    BLT_PRF_DISCOVERY_START_READ(prf, PRF, characteristic); \
}

#define BLT_DEFINE_PRF_DISCOVERY_START_READ_WITH_LEN(prf, PRF, characteristic)  \
BLT_DEFINE_PRF_DISCOVERY_START_READ_COMMON(prf, characteristic) \
{\
    (void)attrHandle;   \
    (void)rdCbFunc;     \
    (void)readLen;      \
    BLT_PRF_DISCOVERY_START_READ_WITH_LEN(prf, PRF, characteristic);    \
}

#define BLT_DEFINE_PRF_DISCOVERY_START_READ_FIX_LEN(prf, PRF, characteristic)   \
BLT_DEFINE_PRF_DISCOVERY_START_READ_COMMON(prf, characteristic) \
{\
    (void)attrHandle;   \
    (void)rdCbFunc;     \
    (void)readLen;      \
    BLT_PRF_DISCOVERY_START_READ_FIX_LEN(prf, PRF, characteristic); \
}

#define BLT_PRF_RECONNECT_SERVICE(prf, PRF, CLIENT_ID)      \
static bool blt_##prf##c_recService(u16 connHandle, int count)  \
{   \
    if(count == 0)  \
    {   \
        struct blc_##prf##_client *client = blt_##prf##c_getClientInst(connHandle); \
        if(client) {    \
            blt_##prf##c_displayInfo(connHandle, client);   \
            BLT_##PRF##_LOG("   INFO: "#PRF" connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);\
        }   \
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, CLIENT_ID);   \
        blc_prf_setDiscoveryStatusFinish(connHandle);   \
        return true;    \
    }   \
    \
    if(count > 1)   \
        return false;   \
    \
    return true;    \
}

#define BLT_BASIC_RECONNECT_SERVICE(prf, PRF)       BLT_PRF_RECONNECT_SERVICE(prf, PRF, PRF##_CLIENT)



#define BLT_PRF_RECONNECT_GET_INFO(prf, Properties, characteristic) \
    struct blc_##prf##_client* client = blt_##prf##c_getClientInst(connHandle); \
    \
    if(client == NULL)      return 0;   \
    \
    charInfo->properties = Properties;  \
    charInfo->valueHandle = client->characteristic##Hdl;    \
    charInfo->cccHandle = 0;    \
    \
    return 1

#define BLT_DEFINE_PRF_RECONNECT_GET_INFO(prf, Properties, characteristic)  \
    static int blt_##prf##c_##characteristic##GetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo) \
{\
    BLT_PRF_RECONNECT_GET_INFO(prf, Properties, characteristic);    \
}

#define BLT_PRF_RECONNECT_NOTIFY_CHAR(prf, characteristic)      \
{\
    .ifun = blt_##prf##c_##characteristic##GetInfo, \
}

#define BLT_PRF_RECONNECT_READ_CHAR(prf, characteristic)        \
{\
    .ifun = blt_##prf##c_##characteristic##GetInfo, \
    .rfun = blt_##prf##c_##characteristic##StartRead,   \
}

#define BLT_PRF_SERVER_INIT_HANDLE(prf, PRF, characteristic)    \
static void blt_##prf##s_##characteristic##InitChar(atts_foundCharParam_t * p, void *input) \
{   \
    struct blc_##prf##_server *server = (struct blc_##prf##_server*)input;  \
    if(p->num > 0)  \
    {\
        BLT_##PRF##_LOG("ERR: "#characteristic" char too many");    \
        return ;    \
    }   \
    server->characteristic##Hdl = p->charHandle;    \
}

#define BLT_PRF_SERVER_FIND_CHAR(prf, characteristic, uuid) \
{\
    .charUuid = uuid,   \
    .charUuidLen = ATT_16_UUID_LEN, \
    .foundCback = blt_##prf##s_##characteristic##InitChar,  \
}

typedef struct {
    u8 clientRdySdp;
    u8 reconnFlag;
    blc_prf_proc_t* currSvcNodeId;
} blt_prf_clientSdpCtrl_t;

typedef struct {
    /* Profile event callback */
    prf_evt_cb_t evtCb;
    /* 0: ACL Central SDP; 1: ACL Peripheral SDP */
    blt_prf_clientSdpCtrl_t sdpCtrl[STACK_PRF_ACL_CONN_MAX_NUM];
    prf_read_cb_t readCb[STACK_PRF_ACL_CONN_MAX_NUM];
    prf_write_cb_t writeCb[STACK_PRF_ACL_CONN_MAX_NUM];
}blt_prf_control_t;

typedef struct{
    u8 subEvt_code;
    void (*evtCb)(u8* p, int len);
} prf_hciLeMetaEvtCb_t;

#define PRF_HCI_EVT_LE_META(META_EVT_CALLBACK)  \
            if((h & HCI_FLAG_EVENT_BT_STD) && ((h&0xff) == HCI_EVT_LE_META)) { \
                u8 subEvt_code = p[0];  \
                for(size_t i=0; i<ARRAY_SIZE(META_EVT_CALLBACK); i++) { \
                    if(META_EVT_CALLBACK[i].subEvt_code == subEvt_code) { \
                        META_EVT_CALLBACK[i].evtCb(p, len); \
                        break;  \
                    }\
                }\
            }

#define PRF_HCI_EVT_CALLBACK(META_EVT_CALLBACK) \
static void blt_prf_hciEventCb(u32 h, u8 *p, int len)   \
{\
    PRF_HCI_EVT_LE_META(META_EVT_CALLBACK)  \
}


int blt_prf_sendEvent(u16 connHandle, int evtID, void *pData, u16 dataLen);

int blt_prf_getAclRole(u16 connHandle);

void blt_prf_sendSvrGapRoleErrEvt(u16 connHandle, int svcId, acl_connection_role_t currAclRole);

#if STACK_PRF_ACL_CONN_MAX_NUM < LL_MAX_ACL_CONN_NUM || STACK_PRF_ACL_CENTRAL_MAX_NUM < LL_MAX_ACL_CEN_NUM || STACK_PRF_ACL_PERIPHERAL_MAX_NUM < LL_MAX_ACL_PER_NUM
#error "profile stack supported ACL connect error."
#endif
