/********************************************************************************************************
 * @file    ras_server_data.c
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

#define REDEF_LOG_EN                      (0)
#define PROCEDURE_DATA_BUFF_SIZE          (PROCEDURE_DATA_LEN*(PROCEDURE_COUNT+1))
#define PROCEDURE_CTRL_BUFF_SIZE          (sizeof(blc_rass_data_ctrl_t)+sizeof(blc_rass_describle_t)*(PROCEDURE_COUNT+1))

/* Local initiator ranging data. */
__attribute__((aligned(4))) u8 ras_data_buf[PROCEDURE_DATA_BUFF_SIZE] = {0};
u8 procedure_ctrl_buf[PROCEDURE_CTRL_BUFF_SIZE] = {0};
blc_rass_proc_head_t proc_head_data = {0};

/* Remote reflector ranging data. (IOP Test) */
#if PROCEDURE_RECEIVE_UART
__attribute__((aligned(4))) u8 ras_data_buf_uart[PROCEDURE_DATA_BUFF_SIZE] = {0};
u8 procedure_ctrl_buf_uart[PROCEDURE_CTRL_BUFF_SIZE] = {0};
blc_rass_proc_head_t proc_head_data_uart = {0};
#endif

/* Remote reflector ranging data. (Release)  */
__attribute__((aligned(4))) u8 ras_data_buf_ble[PROCEDURE_DATA_BUFF_SIZE]={0};
u8 procedure_ctrl_buf_ble[PROCEDURE_CTRL_BUFF_SIZE] = {0};
blc_rass_proc_head_t proc_head_data_ble = {0};

blc_rass_query_result_t blc_rass_procedureQueryIndexLiving(u16 index)
{
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf;
    blc_rass_query_result_t queryData = {0};
    u8 rtProCounter = 0;

    if(dataCtrl->storedNum < 1)
    {
        return queryData;
    }

    for(int i = 0; i < dataCtrl->storedNum; i++)
    {
        blc_rass_stored_data_t *procCtrl = (blc_rass_stored_data_t *)dataCtrl->procDataDes[i].procStartaddr;

        rtProCounter = procCtrl->rangingData[0].proCountCfgID&0x0F;  /* Counter of the procedure (the lower four bits)*/

        if(index == rtProCounter)
        {
            queryData.status  = 1;
            queryData.index   = index;
            queryData.procPtr = dataCtrl->procDataDes[i].procStartaddr+2;
            queryData.procLen = dataCtrl->procDataDes[i].procDataLen-2;
            break;
        }

        if(dataCtrl->storedNum == i+1)
        {
            //BLC_RAS_DATA_LOG("index not found");
        }
    }

    return queryData;
}

blc_rass_query_result_t blc_rass_procedureQueryIndexStored(u16 index)
{
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf;
    blc_rass_query_result_t queryData = {0};

    if(dataCtrl->storedNum < 1)
    {
        return queryData;
    }

    for(int i = 0; i < dataCtrl->storedNum; i++)
    {
        blc_rass_stored_data_t *procCtrl = (blc_rass_stored_data_t *)dataCtrl->procDataDes[i].procStartaddr;

        if(index == procCtrl->procedureCounter)
        {
            queryData.status  = 1;
            queryData.index   = index;
            queryData.procPtr = dataCtrl->procDataDes[i].procStartaddr;
            queryData.procLen = dataCtrl->procDataDes[i].procDataLen;
            break;
        }

        if(dataCtrl->storedNum == i+1)
        {
            //BLC_RAS_DATA_LOG("index not found");
        }
    }

    return queryData;
}

int blc_rass_procedureDeleteIndex(u16 index)
{
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf;
    if(dataCtrl->storedNum == 1)
    {
        blc_rass_stored_data_t *procCtrl = (blc_rass_stored_data_t *)dataCtrl->procDataDes[0].procStartaddr;

        if(index == procCtrl->procedureCounter)
        {
            /* Local initiator*/
            memset(ras_data_buf, 0, PROCEDURE_DATA_BUFF_SIZE);
            memset(procedure_ctrl_buf, 0, PROCEDURE_CTRL_BUFF_SIZE);
            memset((u8 *)&proc_head_data, 0, sizeof(blc_rass_proc_head_t));

            dataCtrl->storedNum = 0;
            dataCtrl->storedTotalSize = 0;
            dataCtrl->procDataDes[dataCtrl->storedNum].procIndex     = 0;
            dataCtrl->procDataDes[dataCtrl->storedNum].procStartaddr = ras_data_buf;
            dataCtrl->procDataDes[dataCtrl->storedNum].subEvtsNums   = 0;
            dataCtrl->procDataDes[dataCtrl->storedNum].subeventPtr   = ras_data_buf+2+PROCEDURE_HEAD_LEN;
            dataCtrl->procDataDes[dataCtrl->storedNum].procDataLen   = 2+PROCEDURE_HEAD_LEN+SUBEVENT_HEAD_LEN;
            return CS_RAS_SUCCESS;
        }
        return CS_RAS_NOT_FOUND;
    }

    for(int i = 0; i < dataCtrl->storedNum; i++)
    {
        blc_rass_stored_data_t *procCtrl = (blc_rass_stored_data_t *)dataCtrl->procDataDes[i].procStartaddr;

        if(index == procCtrl->procedureCounter)
        {
            u16 dataSetLen = ((dataCtrl->procDataDes[i].procDataLen + 3)/4)*4;
            u8 *dataSetStart = NULL;
            for(int j = i; j < dataCtrl->storedNum; j++)
            {
                blc_rass_describle_t *procPreCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[j]);
                blc_rass_describle_t *procNextCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[j+1]);
                u8 procedureHead = 0;

                if(procNextCtrl->procDataLen != 0)
                {
                    u16 cpyDataLen = ((procNextCtrl->procDataLen + 3)/4)*4;
                    memcpy(procPreCtrl->procStartaddr, procNextCtrl->procStartaddr, cpyDataLen);
                }
                else
                {
                    procedureHead = 2+PROCEDURE_HEAD_LEN;
                }

                procPreCtrl->subeventPtr = procPreCtrl->procStartaddr + procNextCtrl->procDataLen + procedureHead;
                procPreCtrl->procDataLen = procNextCtrl->procDataLen;
                procPreCtrl->subEvtsNums = procNextCtrl->subEvtsNums;

                //procNextCtrl->procStartaddr = procPreCtrl->procStartaddr + ((procPreCtrl->procDataLen + 3)/4)*4;
            }

            dataSetStart = dataCtrl->procDataDes[dataCtrl->storedNum-1].procStartaddr;
            //BLC_RAS_DATA_LOG("delete data start =  %p, len = %x", dataSetStart, dataSetLen);
            memset(dataSetStart, 0, dataSetLen);
            memset(&dataCtrl->procDataDes[dataCtrl->storedNum-1], 0, sizeof(blc_rass_describle_t));
            dataCtrl->storedNum -= 1;
            dataCtrl->storedTotalSize -= dataSetLen;
            //BLC_RAS_DATA_LOG("delete index 0x%x", i);
            break;
        }

        if(dataCtrl->storedNum == i+1)
        {
            //BLC_RAS_DATA_LOG("index delete error");
            return CS_RAS_NOT_FOUND;
        }
    }

    return CS_RAS_SUCCESS;
}

u8 * blc_rass_stepDataProc(u8 *writePtr, u8 *srcPtr, u8 stepsNum)
{
    u8 *stepPtr = srcPtr;
    for(int i = 0; i < stepsNum; i++)
    {
        u32 stepData = 0;
        u8 stepDataLen = 0;
        STREAM_TO_U24(stepData ,stepPtr);
        U24_TO_STREAM(writePtr, stepData);
        stepDataLen = U32_BYTE2(stepData);

        STR_TO_STREAM(writePtr, stepPtr, stepDataLen);
        stepPtr += stepDataLen;
    }

    return writePtr;
}

int blc_rass_procedureEnComplete(hci_le_csProcedureEnableCompleteEvt_t *procedureHead)
{
#if 1
    /* Local initiator*/
    memset(ras_data_buf, 0, PROCEDURE_DATA_BUFF_SIZE);
    memset(procedure_ctrl_buf, 0, PROCEDURE_CTRL_BUFF_SIZE);
    memset((u8 *)&proc_head_data, 0, sizeof(blc_rass_proc_head_t));
#endif

    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf;

    proc_head_data.procedureCounter = 0;
    proc_head_data.proCountCfgID   = (procedureHead->Config_ID<<4)&0xF0;
    proc_head_data.selectedTxPower  = procedureHead->Selected_TX_Power;
    proc_head_data.numAntennaPaths  = 1;

    dataCtrl->storedNum = 0;
    dataCtrl->storedTotalSize = 0;
    dataCtrl->procDataDes[dataCtrl->storedNum].procIndex     = 0;
    dataCtrl->procDataDes[dataCtrl->storedNum].procStartaddr = ras_data_buf;
    dataCtrl->procDataDes[dataCtrl->storedNum].subEvtsNums   = 0;
    dataCtrl->procDataDes[dataCtrl->storedNum].subeventPtr   = ras_data_buf+2+PROCEDURE_HEAD_LEN;
    dataCtrl->procDataDes[dataCtrl->storedNum].procDataLen   = 2+PROCEDURE_HEAD_LEN+SUBEVENT_HEAD_LEN;

    /* Remote reflector */
    memset(ras_data_buf_ble, 0, PROCEDURE_DATA_BUFF_SIZE);
    memset(procedure_ctrl_buf_ble, 0, PROCEDURE_CTRL_BUFF_SIZE);
    memset((u8 *)&proc_head_data_ble, 0, sizeof(blc_rass_proc_head_t));

    dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_ble;
    dataCtrl->storedNum = 0;
    dataCtrl->storedTotalSize = 0;
    dataCtrl->procDataDes[dataCtrl->storedNum].procIndex     = 0;
    dataCtrl->procDataDes[dataCtrl->storedNum].procStartaddr = ras_data_buf_ble;
    dataCtrl->procDataDes[dataCtrl->storedNum].subEvtsNums   = 0;
    dataCtrl->procDataDes[dataCtrl->storedNum].subeventPtr   = ras_data_buf_ble+2+PROCEDURE_HEAD_LEN;
    dataCtrl->procDataDes[dataCtrl->storedNum].procDataLen   = 2+PROCEDURE_HEAD_LEN+SUBEVENT_HEAD_LEN;

    return CS_RAS_SUCCESS;
}

static u8 newSubevent = 0;
void blc_rass_procedureHeaderFill(blc_rass_describle_t *procCtrl, u8 *startAddr)
{
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf;

    u8 *writePtr = startAddr;
    //BLC_RAS_DATA_LOG("procedure start wptr = %x", writePtr);
    BLC_RAS_DATA_LOG("record number = %x", proc_head_data.procedureCounter);
    U16_TO_STREAM(writePtr, proc_head_data.procedureCounter);
    U8_TO_STREAM(writePtr, proc_head_data.proCountCfgID);
    U8_TO_STREAM(writePtr, proc_head_data.selectedTxPower);
    U8_TO_STREAM(writePtr, proc_head_data.numAntennaPaths);
    //BLC_RAS_DATA_LOG("procedure end wptr = %x", procCtrl->procStartaddr + procCtrl->procDataLen);

    u16 alignLen = ((procCtrl->procDataLen + 3)/4)*4;
    dataCtrl->storedTotalSize += alignLen;
    dataCtrl->procDataDes[dataCtrl->storedNum + 1].procStartaddr = procCtrl->procStartaddr+alignLen;
    dataCtrl->procDataDes[dataCtrl->storedNum + 1].subeventPtr = procCtrl->procStartaddr+alignLen+2+PROCEDURE_HEAD_LEN;
    dataCtrl->procDataDes[dataCtrl->storedNum + 1].procDataLen = 2+PROCEDURE_HEAD_LEN+SUBEVENT_HEAD_LEN;

#if 0
    u32 totalLen = dataCtrl->procDataDes[dataCtrl->storedNum].procDataLen;
    u32 sendLen = 0;
    u8 *pdata = dataCtrl->procDataDes[dataCtrl->storedNum].procStartaddr;
    while(totalLen)
    {
        if(totalLen>128)
        {
            tlkapi_send_string_data(REDEF_LOG_EN, "step mode1 reflector:", (u8 *)pdata+sendLen, 128);
            sendLen += 128;
            totalLen -= 128;
        }
        else
        {
            tlkapi_send_string_data(REDEF_LOG_EN, "step mode1 reflector:", (u8 *)pdata+sendLen, totalLen);
            totalLen = 0;
            sendLen += totalLen;
        }
    }
#endif

    newSubevent = 0;
    dataCtrl->storedNum++;
    //BLC_RAS_DATA_LOG("stored number = %x", dataCtrl->storedNum);
}

int blc_rass_subeventResultData(hci_le_csSubeventResultEvt_t *resultEvt)
{
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf;
    blc_rass_describle_t *procCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[dataCtrl->storedNum]);

    if((dataCtrl->storedTotalSize > (sizeof(ras_data_buf) - SIZELIMIT)) || dataCtrl->storedNum >= PROCEDURE_COUNT)
    {
        procCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[0]);
        blc_rass_stored_data_t *pEvt = (blc_rass_stored_data_t *)(procCtrl->procStartaddr);
        blc_rass_procedureDataOverwritten(resultEvt->Connection_Handle, pEvt->procedureCounter);
        blc_rass_procedureDeleteIndex(pEvt->procedureCounter);

        blc_rass_describle_t *preProcCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[dataCtrl->storedNum-1]);
        blc_rass_describle_t *nexProcCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[dataCtrl->storedNum]);
        nexProcCtrl->procStartaddr = preProcCtrl->procStartaddr + ((preProcCtrl->procDataLen+3)/4)*4;
        nexProcCtrl->subeventPtr = nexProcCtrl->procStartaddr+2+PROCEDURE_HEAD_LEN;
        nexProcCtrl->procDataLen = 2+PROCEDURE_HEAD_LEN+SUBEVENT_HEAD_LEN;
        nexProcCtrl->subEvtsNums = 0;
    }

    proc_head_data.procedureCounter = resultEvt->Procedure_Counter;
    proc_head_data.proCountCfgID   = (resultEvt->Procedure_Counter&0x0F)|((resultEvt->Config_ID<<4)&0xF0);  /* Counter of the procedure (the lower four bits)*/
    proc_head_data.numAntennaPaths  = resultEvt->Num_Antenna_Paths;
    /*tlkapi_printf(REDEF_LOG_EN, "proCountCfgID=%02X, Procedure_Counter=%02X, Config_ID=%02X, numAntennaPaths=%02X",
                                 proc_head_data.proCountCfgID, resultEvt->Procedure_Counter, resultEvt->Config_ID ,proc_head_data.numAntennaPaths);*/

    procCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[dataCtrl->storedNum]);
    u8 *writePtr = (u8 *)(procCtrl->subeventPtr);

    /* -> Subevent Index
     * -> Start ACL Conn Event
     * -> Frequency Compensation
     * -> Procedure Done Status
     * -> Subevent Done Status
     * -> Reference Power Level
     * -> Num Antenna Paths
     * -> Num Steps Reported
     * -> Subevent Ranging Data[]
     * */

#if STEP_INDEX_ENABLE
    /* fix uart data loss.  */
    u8 index = (resultEvt->Connection_Handle&0xFF00) >> 8;
    U8_TO_STREAM(writePtr, index);
#else
    U8_TO_STREAM(writePtr, procCtrl->subEvtsNums); // Subevent Index
#endif
    U16_TO_STREAM(writePtr, resultEvt->Start_ACL_Conn_Event);
    U16_TO_STREAM(writePtr, resultEvt->Frequency_Compensation);
    U8_TO_STREAM(writePtr, resultEvt->Procedure_Done_Status);
    U8_TO_STREAM(writePtr, resultEvt->Subevent_Done_Status);
    U8_TO_STREAM(writePtr, resultEvt->Reference_Power_Level);

    u8 numSteps = *writePtr + resultEvt->Num_Steps_Reported;
    U8_TO_STREAM(writePtr, numSteps);

    writePtr = procCtrl->procStartaddr + procCtrl->procDataLen;

#if SUBEVENT_RESULT_OVERFLOW_CHECK
    // TODO: cxh
    u8 *endPtr   = ras_data_buf+sizeof(ras_data_buf)-SIZELIMIT;
    //BLC_RAS_DATA_LOG("subevent buffer = %x, %x", ras_data_buf, endPtr);
    if(writePtr >= endPtr)
    {
        tlkapi_printf(REDEF_LOG_EN, "CS_SUBEVT_ABORT.");
        return CS_SUBEVT_ABORT;
    }
#endif

    if(newSubevent)     //Here comes a new subevent, offset the head of the subevent.
    {
        writePtr += SUBEVENT_HEAD_LEN;
        newSubevent = 0;
    }
    writePtr = blc_rass_stepDataProc(writePtr, (u8*)resultEvt->Step_Mode, resultEvt->Num_Steps_Reported);

    procCtrl->procDataLen = writePtr - procCtrl->procStartaddr;
    //BLC_RAS_DATA_LOG("buffer:%s", hex_to_str(procCtrl->subeventPtr, 80));
    //BLC_RAS_DATA_LOG("subevent result end wptr = %x, subPtr = %x, proclen = %d, datalen = %d", writePtr, procCtrl->subeventPtr, procCtrl->procDataLen, resultEvt->Step_Mode->len+3);

    if((resultEvt->Subevent_Done_Status == CS_SUBEVT_DONE) || (resultEvt->Subevent_Done_Status == CS_SUBEVT_ABORT))
    {
        procCtrl->subeventPtr = writePtr;
        procCtrl->subEvtsNums++;
        newSubevent = 1;
    }

    if((resultEvt->Procedure_Done_Status == CS_PROC_DONE) || (resultEvt->Procedure_Done_Status == CS_PROC_ABORT))
    {
        //BLC_RAS_DATA_LOG("procedure done----------------------------------");
        blc_rass_procedureHeaderFill(procCtrl, (u8 *)(procCtrl->procStartaddr));
        blc_rass_procedureDataReady(resultEvt->Connection_Handle, proc_head_data.procedureCounter);
    }

    return CS_RAS_SUCCESS;
}

int blc_rass_subeventResultContinueData(hci_le_csSubeventResultContinueEvt_t *continueEvt)
{
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf;
    blc_rass_describle_t *procCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[dataCtrl->storedNum]);

    u8 *writePtr = (u8 *)(procCtrl->subeventPtr + 5);   //subevent header
    U8_TO_STREAM(writePtr, continueEvt->Procedure_Done_Status);
    U8_TO_STREAM(writePtr, continueEvt->Subevent_Done_Status);

    writePtr += 1;                                      //skip "Reference Power Level [SubeventIndex]"
    u8 numSteps = *writePtr + continueEvt->Num_Steps_Reported;
    U8_TO_STREAM(writePtr, numSteps);
    writePtr = (u8 *)(procCtrl->procStartaddr + procCtrl->procDataLen);

#if SUBEVENT_RESULT_OVERFLOW_CHECK
    // TODO: cxh
    u8 *endPtr   = ras_data_buf+sizeof(ras_data_buf)-SIZELIMIT;
    //BLC_RAS_DATA_LOG("subevent buffer = %x, %x", ras_data_buf, endPtr);
    if(writePtr >= endPtr)
    {
        return CS_SUBEVT_ABORT;
    }
#endif

    writePtr = blc_rass_stepDataProc(writePtr, (u8*)continueEvt->Step_Mode, continueEvt->Num_Steps_Reported);
    //BLC_RAS_DATA_LOG("continue result end wptr = %x, len = %d, status = %d, datalen = %d", writePtr, continueEvt->Step_Mode->len+3, continueEvt->Subevent_Done_Status, continueEvt->Step_Mode->len+3);

    procCtrl->procDataLen = writePtr - procCtrl->procStartaddr;

    if((continueEvt->Subevent_Done_Status == CS_SUBEVT_DONE) || (continueEvt->Subevent_Done_Status == CS_SUBEVT_ABORT))
    {
        procCtrl->subeventPtr = writePtr;
        procCtrl->subEvtsNums++;
        newSubevent = 1;
    }

    if((continueEvt->Procedure_Done_Status == CS_PROC_DONE) || (continueEvt->Procedure_Done_Status == CS_PROC_ABORT))
    {
        //BLC_RAS_DATA_LOG("procedure done----------------------------------");
        blc_rass_procedureHeaderFill(procCtrl, (u8 *)(procCtrl->procStartaddr));
        blc_rass_procedureDataReady(continueEvt->Connection_Handle, proc_head_data.procedureCounter);
    }

    return CS_RAS_SUCCESS;
}

#if PROCEDURE_RECEIVE_UART
blc_rass_query_result_t blc_rass_procedureQueryIndex_uart(u16 index)
{
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_uart;
    blc_rass_query_result_t queryData = {0};

    if(dataCtrl->storedNum < 1)
    {
        return queryData;
    }

    for(int i = 0; i < dataCtrl->storedNum; i++)
    {
        blc_rass_stored_data_t *procCtrl = (blc_rass_stored_data_t *)dataCtrl->procDataDes[i].procStartaddr;

        if(index == procCtrl->rangingData[0].procedureCounter)
        {
            queryData.status = true;
            queryData.index = index;
            queryData.procPtr = dataCtrl->procDataDes[i].procStartaddr;
            queryData.procLen = dataCtrl->procDataDes[i].procDataLen;

            //BLC_RAS_DATA_LOG("status = %x, index = %x, procedurePtr = %x, procedureLen = %x",
                    queryData.status, queryData.index, queryData.procPtr, queryData.procLen);
            break;
        }

        if(dataCtrl->storedNum == i+1)
        {
            //BLC_RAS_DATA_LOG("index not found");
        }
    }

    return queryData;
}

int blc_rass_procedureDeleteIndex_uart(u16 index)
{
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_uart;

    for(int i = 0; i < dataCtrl->storedNum; i++)
    {
        blc_rass_stored_data_t *procCtrl = (blc_rass_stored_data_t *)dataCtrl->procDataDes[i].procStartaddr;

        if(index == procCtrl->rangingData[0].procedureCounter)
        {
            u16 dataSetLen = ((dataCtrl->procDataDes[i].procDataLen + 3)/4)*4;
            u8 *dataSetStart = NULL;
            for(int j = i; j < dataCtrl->storedNum; j++)
            {
                blc_rass_describle_t *procPreCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[j]);
                blc_rass_describle_t *procNextCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[j+1]);
                u8 procedureHead = 0;

                if(procNextCtrl->procDataLen != 0)
                {
                    u16 cpyDataLen = ((procNextCtrl->procDataLen + 3)/4)*4;
                    memcpy(procPreCtrl->procStartaddr, procNextCtrl->procStartaddr, cpyDataLen);
                }
                else
                {
                    procedureHead = 2+PROCEDURE_HEAD_LEN;
                }

                procPreCtrl->subeventPtr = procPreCtrl->procStartaddr + procNextCtrl->procDataLen + procedureHead;
                procPreCtrl->procDataLen = procNextCtrl->procDataLen;
                procPreCtrl->subEvtsNums = procNextCtrl->subEvtsNums;

                //procNextCtrl->procStartaddr = procPreCtrl->procStartaddr + ((procPreCtrl->procDataLen + 3)/4)*4;
            }
            dataSetStart = dataCtrl->procDataDes[dataCtrl->storedNum-1].procStartaddr;
            //BLC_RAS_DATA_LOG("delete data start =  %p, len = %x", dataSetStart, dataSetLen);
            memset(dataSetStart, 0, dataSetLen);
            //memset(&dataCtrl->procDataDes[dataCtrl->storedNum], 0, sizeof(blc_rass_describle_t));

            dataCtrl->storedNum -= 1;
            dataCtrl->storedTotalSize -= dataSetLen;
            //BLC_RAS_DATA_LOG("delete index 0x%x", i);
            break;
        }

        if(dataCtrl->storedNum == i+1)
        {
            //BLC_RAS_DATA_LOG("index delete error");
            return CS_RAS_NOT_FOUND;
        }
    }

    return CS_RAS_SUCCESS;
}

u8 * blc_rass_stepDataProc_uart(u8 *writePtr, u8 *srcPtr, u8 stepsNum)
{

    u8 *stepPtr = srcPtr;
    for(int i = 0; i < stepsNum; i++)
    {
        u32 stepData = 0;
        u8 stepDataLen = 0;
        STREAM_TO_U24(stepData ,stepPtr);
        U24_TO_STREAM(writePtr, stepData);
        stepDataLen = U32_BYTE2(stepData);

        STR_TO_STREAM(writePtr, stepPtr, stepDataLen);
        stepPtr += stepDataLen;
    }

    return writePtr;
}

int blc_rass_procedureEnComplete_uart(hci_le_csProcedureEnableCompleteEvt_t *procedureHead)
{
#if 1
    memset(ras_data_buf_uart, 0, PROCEDURE_DATA_BUFF_SIZE);
    memset(procedure_ctrl_buf_uart, 0, PROCEDURE_CTRL_BUFF_SIZE);
    memset((u8 *)&proc_head_data_uart, 0, sizeof(blc_rass_proc_head_t));
#endif
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_uart;

    proc_head_data_uart.recordNumber = 0;
    proc_head_data_uart.procedureCounter = 0;
    proc_head_data_uart.configurationId = procedureHead->Config_ID;
    proc_head_data_uart.selectedTxPower = procedureHead->Selected_TX_Power;
    proc_head_data_uart.numAntennaPaths = 1;

    dataCtrl->storedNum = 0;
    dataCtrl->storedTotalSize = 0;
    dataCtrl->procDataDes[dataCtrl->storedNum].procIndex = 0;
    dataCtrl->procDataDes[dataCtrl->storedNum].procStartaddr = ras_data_buf_uart;
    dataCtrl->procDataDes[dataCtrl->storedNum].subEvtsNums = 0;
    dataCtrl->procDataDes[dataCtrl->storedNum].subeventPtr = ras_data_buf_uart+2+PROCEDURE_HEAD_LEN;
    dataCtrl->procDataDes[dataCtrl->storedNum].procDataLen = 2+PROCEDURE_HEAD_LEN+SUBEVENT_HEAD_LEN;

    return CS_RAS_SUCCESS;
}

static u8 newSubevent_uart = 0;
void blc_rass_procedureHeaderFill_uart(blc_rass_describle_t *procCtrl, u8 *startAddr)
{
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_uart;

    u8 *writePtr = startAddr;
    //BLC_RAS_DATA_LOG("procedure start wptr = %x", writePtr);
    BLC_RAS_DATA_LOG("record number = %x", proc_head_data_uart.recordNumber);
    U32_TO_STREAM(writePtr, proc_head_data_uart.recordNumber);
    U8_TO_STREAM(writePtr, proc_head_data_uart.procedureCounter);
    U8_TO_STREAM(writePtr, proc_head_data_uart.configurationId);
    U8_TO_STREAM(writePtr, proc_head_data_uart.selectedTxPower);
    U8_TO_STREAM(writePtr, proc_head_data_uart.numAntennaPaths);
    //BLC_RAS_DATA_LOG("procedure end wptr = %x", procCtrl->procStartaddr + procCtrl->procDataLen);

    u16 alignLen = ((procCtrl->procDataLen + 3)/4)*4;
    dataCtrl->storedTotalSize += alignLen;
    dataCtrl->procDataDes[dataCtrl->storedNum + 1].procStartaddr = procCtrl->procStartaddr+alignLen;
    dataCtrl->procDataDes[dataCtrl->storedNum + 1].subeventPtr = procCtrl->procStartaddr+alignLen+2+PROCEDURE_HEAD_LEN;
    dataCtrl->procDataDes[dataCtrl->storedNum + 1].procDataLen = 2+PROCEDURE_HEAD_LEN+SUBEVENT_HEAD_LEN;
    newSubevent_uart = 0;

    //tlkapi_printf(REDEF_LOG_EN, "procedure done uart.\n");

#if 0
    blc_rass_data_ctrl_t *local_dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf;
    blc_rass_describle_t *procCtrl_t = (blc_rass_describle_t *)&(local_dataCtrl->procDataDes[local_dataCtrl->storedNum-1]);
    blc_rass_stored_data_t *pEvt = (blc_rass_stored_data_t *)(procCtrl_t->procStartaddr);
    tlkapi_printf(REDEF_LOG_EN, "Local Proocedure, procedureCounter = %d, storedNum = %d", pEvt->rangingData[0].procedureCounter, local_dataCtrl->storedNum);
    tlkapi_printf(REDEF_LOG_EN, "       ->procIndex     = %d",   procCtrl_t->procIndex);
    tlkapi_printf(REDEF_LOG_EN, "       ->procStartaddr = %08X", (u32)procCtrl_t->procStartaddr);
    tlkapi_printf(REDEF_LOG_EN, "       ->subEvtsNums   = %d",   procCtrl_t->subEvtsNums);
    tlkapi_printf(REDEF_LOG_EN, "       ->subeventPtr   = %08X", procCtrl_t->subeventPtr);
    tlkapi_printf(REDEF_LOG_EN, "       ->procDataLen   = %d",  procCtrl_t->procDataLen);
    //tlkapi_send_string_data(REDEF_LOG_EN, "Local procedure Data:", (u8 *)pEvt, 100);

    procCtrl_t = (blc_rass_describle_t *)&(dataCtrl->procDataDes[dataCtrl->storedNum]);
    pEvt = (blc_rass_stored_data_t *)(procCtrl_t->procStartaddr);
    tlkapi_printf(REDEF_LOG_EN, "Remote Proocedure, procedureCounter = %d, storedNum = %d", pEvt->rangingData[0].procedureCounter, dataCtrl->storedNum);
    tlkapi_printf(REDEF_LOG_EN, "       ->procIndex     = %d",   procCtrl_t->procIndex);
    tlkapi_printf(REDEF_LOG_EN, "       ->procStartaddr = %08X", (u32)procCtrl_t->procStartaddr);
    tlkapi_printf(REDEF_LOG_EN, "       ->subEvtsNums   = %d",   procCtrl_t->subEvtsNums);
    tlkapi_printf(REDEF_LOG_EN, "       ->subeventPtr   = %08X", procCtrl_t->subeventPtr);
    tlkapi_printf(REDEF_LOG_EN, "       ->procDataLen   = %d",  procCtrl_t->procDataLen);
    //tlkapi_send_string_data(REDEF_LOG_EN, "Remote procedure Data:", (u8 *)pEvt, 100);
#endif

    extern void csCalculateDistance(u16 procIndex);
    csCalculateDistance(dataCtrl->storedNum);

    dataCtrl->storedNum++;
    //BLC_RAS_DATA_LOG("stored number = %x", dataCtrl->storedNum);
}

int blc_rass_subeventResultData_uart(hci_le_csSubeventResultEvt_t *resultEvt)
{

    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_uart;
    blc_rass_describle_t *procCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[dataCtrl->storedNum]);

    if((dataCtrl->storedTotalSize > (sizeof(ras_data_buf_uart) - SIZELIMIT)) || dataCtrl->storedNum >= PROCEDURE_COUNT)
    {
        procCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[0]);
        //tlkapi_printf(REDEF_LOG_EN, "procedure overWrite uart. storeTotalSize=%d, storedNum=%d",dataCtrl->storedTotalSize, dataCtrl->storedNum);
        blc_rass_stored_data_t *pEvt = (blc_rass_stored_data_t *)(procCtrl->procStartaddr);
        blc_rass_procedureDataOverwritten(resultEvt->Connection_Handle, pEvt->rangingData[0].procedureCounter);

        pEvt = (blc_rass_stored_data_t *)(procCtrl->procStartaddr);
        blc_rass_procedureDeleteIndex_uart(pEvt->rangingData[0].procedureCounter);

        blc_rass_describle_t *preProcCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[dataCtrl->storedNum-1]);
        blc_rass_describle_t *nexProcCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[dataCtrl->storedNum]);
        nexProcCtrl->procStartaddr = preProcCtrl->procStartaddr + ((preProcCtrl->procDataLen+3)/4)*4;
        nexProcCtrl->subeventPtr = nexProcCtrl->procStartaddr+2+PROCEDURE_HEAD_LEN;
        nexProcCtrl->procDataLen = 2+PROCEDURE_HEAD_LEN+SUBEVENT_HEAD_LEN;
        nexProcCtrl->subEvtsNums = 0;
    }

    procCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[dataCtrl->storedNum]);
    u8 *writePtr = (u8 *)(procCtrl->subeventPtr);
#if STEP_INDEX_ENABLE
    /* fix uart data loss.  */
    u8 index = ((resultEvt->Connection_Handle&0xFF00) >> 8);
    U8_TO_STREAM(writePtr, index);
#else
    U8_TO_STREAM(writePtr, procCtrl->subEvtsNums); // Subevent Index
#endif
    U16_TO_STREAM(writePtr, resultEvt->Start_ACL_Conn_Event);
    U16_TO_STREAM(writePtr, resultEvt->Frequency_Compensation);
    U8_TO_STREAM(writePtr, resultEvt->Procedure_Done_Status);
    U8_TO_STREAM(writePtr, resultEvt->Subevent_Done_Status);
    U8_TO_STREAM(writePtr, resultEvt->Reference_Power_Level);
    U8_TO_STREAM(writePtr, resultEvt->Num_Antenna_Paths);

    u8 numSteps = *writePtr + resultEvt->Num_Steps_Reported;
    U8_TO_STREAM(writePtr, numSteps);

    writePtr = procCtrl->procStartaddr + procCtrl->procDataLen;

#if SUBEVENT_RESULT_OVERFLOW_CHECK
    // TODO: cxh
    u8 *endPtr   = ras_data_buf_uart+sizeof(ras_data_buf_uart)-SIZELIMIT;
    //BLC_RAS_DATA_LOG("subevent buffer = %x, %x", ras_data_buf_uart, endPtr);
    if(writePtr >= endPtr)
    {
        tlkapi_printf(REDEF_LOG_EN, "CS_SUBEVT_ABORT UART.");
        return CS_SUBEVT_ABORT;
    }
#endif

    if(newSubevent_uart)        //Here comes a new subevent, offset the head of the subevent.
    {
        writePtr += SUBEVENT_HEAD_LEN;
        newSubevent_uart = 0;
    }
    writePtr = blc_rass_stepDataProc_uart(writePtr, (u8*)resultEvt->Step_Mode, resultEvt->Num_Steps_Reported);

    procCtrl->procDataLen = writePtr - procCtrl->procStartaddr;
    //BLC_RAS_DATA_LOG("buffer:%s", hex_to_str(procCtrl->subeventPtr, 80));
    //BLC_RAS_DATA_LOG("subevent result end wptr = %x, subPtr = %x, proclen = %d, datalen = %d", writePtr, procCtrl->subeventPtr, procCtrl->procDataLen, resultEvt->Step_Mode->len+3);

    if((resultEvt->Subevent_Done_Status == CS_SUBEVT_DONE) || (resultEvt->Subevent_Done_Status == CS_SUBEVT_ABORT))
    {
        procCtrl->subeventPtr = writePtr;
        procCtrl->subEvtsNums++;
        newSubevent_uart = 1;
    }

    proc_head_data_uart.recordNumber = resultEvt->Procedure_Counter;
    if((resultEvt->Procedure_Done_Status == CS_PROC_DONE) || (resultEvt->Procedure_Done_Status == CS_PROC_ABORT))
    {
        //BLC_RAS_DATA_LOG("procedure done----------------------------------");
        blc_rass_procedureHeaderFill_uart(procCtrl, (u8 *)(procCtrl->procStartaddr));
        //blc_rass_procedureDataReady(resultEvt->Connection_Handle, proc_head_data_uart.procedureCounter);
        proc_head_data_uart.procedureCounter++;
    }

    return CS_RAS_SUCCESS;
}

int blc_rass_subeventResultContinueData_uart(hci_le_csSubeventResultContinueEvt_t *continueEvt)
{
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_uart;
    blc_rass_describle_t *procCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[dataCtrl->storedNum]);

    u8 *writePtr = (u8 *)(procCtrl->subeventPtr + 5);   //subevent header
    U8_TO_STREAM(writePtr, continueEvt->Procedure_Done_Status);
    U8_TO_STREAM(writePtr, continueEvt->Subevent_Done_Status);

    writePtr += 1;
    u8 numSteps = *writePtr + continueEvt->Num_Steps_Reported;
    U8_TO_STREAM(writePtr, numSteps);

    writePtr = (u8 *)(procCtrl->procStartaddr + procCtrl->procDataLen);

#if SUBEVENT_RESULT_OVERFLOW_CHECK
    // TODO: cxh
    u8 *endPtr   = ras_data_buf_uart+sizeof(ras_data_buf_uart)-SIZELIMIT;
    //BLC_RAS_DATA_LOG("subevent buffer = %x, %x", ras_data_buf_uart, endPtr);
    if(writePtr >= endPtr)
    {
        return CS_SUBEVT_ABORT;
    }
#endif

    //BLC_RAS_DATA_LOG("continue result start wptr = %x", writePtr);
    writePtr = blc_rass_stepDataProc_uart(writePtr, (u8*)continueEvt->Step_Mode, continueEvt->Num_Steps_Reported);
    //BLC_RAS_DATA_LOG("continue result end wptr = %x, len = %d, status = %d, datalen = %d", writePtr, continueEvt->Step_Mode->len+3, continueEvt->Subevent_Done_Status, continueEvt->Step_Mode->len+3);

    procCtrl->procDataLen = writePtr - procCtrl->procStartaddr;

    if((continueEvt->Subevent_Done_Status == CS_SUBEVT_DONE) || (continueEvt->Subevent_Done_Status == CS_SUBEVT_ABORT))
    {
        procCtrl->subeventPtr = writePtr;
        procCtrl->subEvtsNums++;
        newSubevent_uart = 1;
    }

    if((continueEvt->Procedure_Done_Status == CS_PROC_DONE) || (continueEvt->Procedure_Done_Status == CS_PROC_ABORT))
    {
        //BLC_RAS_DATA_LOG("procedure done----------------------------------");
        blc_rass_procedureHeaderFill_uart(procCtrl, (u8 *)(procCtrl->procStartaddr));
        //blc_rass_procedureDataReady(continueEvt->Connection_Handle, proc_head_data_uart.procedureCounter);
        proc_head_data_uart.procedureCounter++;
    }

    return CS_RAS_SUCCESS;
}

#endif


int blc_rass_procedureDeleteIndex_ble(u16 index)
{
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_ble;

    for(int i = 0; i < dataCtrl->storedNum; i++)
    {
        blc_rass_stored_data_t *procCtrl = (blc_rass_stored_data_t *)dataCtrl->procDataDes[i].procStartaddr;

        if(index == procCtrl->procedureCounter)
        {
            u16 dataSetLen = ((dataCtrl->procDataDes[i].procDataLen + 3)/4)*4;
            u8 *dataSetStart = NULL;
            for(int j = i; j < dataCtrl->storedNum; j++)
            {
                blc_rass_describle_t *procPreCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[j]);
                blc_rass_describle_t *procNextCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[j+1]);
                u8 procedureHead = 0;

                if(procNextCtrl->procDataLen != 0)
                {
                    u16 cpyDataLen = ((procNextCtrl->procDataLen + 3)/4)*4;
                    memcpy(procPreCtrl->procStartaddr, procNextCtrl->procStartaddr, cpyDataLen);
                }
                else
                {
                    procedureHead = 2+PROCEDURE_HEAD_LEN;
                }

                procPreCtrl->subeventPtr = procPreCtrl->procStartaddr + procNextCtrl->procDataLen + procedureHead;
                procPreCtrl->procDataLen = procNextCtrl->procDataLen;
                procPreCtrl->subEvtsNums = procNextCtrl->subEvtsNums;

                //procNextCtrl->procStartaddr = procPreCtrl->procStartaddr + ((procPreCtrl->procDataLen + 3)/4)*4;
            }
            dataSetStart = dataCtrl->procDataDes[dataCtrl->storedNum-1].procStartaddr;
            //BLC_RAS_DATA_LOG("delete data start =  %p, len = %x", dataSetStart, dataSetLen);
            memset(dataSetStart, 0, dataSetLen);
            //tmemset(&dataCtrl->procDataDes[dataCtrl->storedNum-1], 0, sizeof(blc_rass_describle_t));

            dataCtrl->storedNum -= 1;
            dataCtrl->storedTotalSize -= dataSetLen;
            //BLC_RAS_DATA_LOG("delete index 0x%x", i);
            break;
        }

        if(dataCtrl->storedNum == i+1)
        {
            //BLC_RAS_DATA_LOG("index delete error");
            return CS_RAS_NOT_FOUND;
        }
    }

    return CS_RAS_SUCCESS;
}
/**
 * @brief
 * @param[in]
 * @param[in]
 * @@return
 */
u32 blc_rass_calcRangData(u16 connHandle, u8 *rangData, u32 length, float *distance1, float *distance2)
{
    (void)connHandle;
    if((rangData==NULL)||(length==0))
    {
        tlkapi_printf(REDEF_LOG_EN, "rangData err.");
        return 3;
    }

#if 0
    u32 totalLen = length;
    u32 sendLen = 0;
    u8 *pdata = rangData;
    while(totalLen)
    {
        if(totalLen>128)
        {
            tlkapi_send_string_data(REDEF_LOG_EN, "ranging data:", (u8 *)pdata+sendLen, 128);
            sendLen += 128;
            totalLen -= 128;
        }
        else
        {
            tlkapi_send_string_data(REDEF_LOG_EN, "ranging data:", (u8 *)pdata+sendLen, totalLen);
            totalLen = 0;
            sendLen += totalLen;
        }
    }
#endif

#if 1
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_ble;
    blc_rass_describle_t *procCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[dataCtrl->storedNum]);

    if((dataCtrl->storedTotalSize > (sizeof(ras_data_buf_ble) - SIZELIMIT)) || dataCtrl->storedNum >= PROCEDURE_COUNT)
    {
        procCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[0]);
        blc_rass_stored_data_t *pEvt = (blc_rass_stored_data_t *)(procCtrl->procStartaddr);
        blc_rass_procedureDeleteIndex_ble(pEvt->procedureCounter);
    }

    memcpy(dataCtrl->procDataDes[dataCtrl->storedNum].procStartaddr, rangData, length);
    dataCtrl->procDataDes[dataCtrl->storedNum].procDataLen = length;

    extern  u32 csCalculateDistance(u16 procIndex, float *distance1, float *distance2);
    u32 retVal = csCalculateDistance(dataCtrl->storedNum, distance1, distance2);

    dataCtrl->storedNum++;
    dataCtrl->storedTotalSize += length;
    dataCtrl->procDataDes[dataCtrl->storedNum].procStartaddr = ras_data_buf_ble+dataCtrl->storedTotalSize;
    dataCtrl->procDataDes[dataCtrl->storedNum].procDataLen   = 0;
#else
    blc_rass_data_ctrl_t *dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf_ble;
    blc_rass_describle_t *procCtrl = (blc_rass_describle_t *)&(dataCtrl->procDataDes[0]);
    memcpy(ras_data_buf_ble, rangData, length);
    dataCtrl->procDataDes[0].procDataLen = length;
    dataCtrl->procDataDes[0].procStartaddr = ras_data_buf_ble;
    dataCtrl->storedTotalSize = length;

    //dataCtrl = (blc_rass_data_ctrl_t *)procedure_ctrl_buf;
    extern void csCalculateDistance(u16 procIndex);
    csCalculateDistance(dataCtrl->storedNum);
    dataCtrl->storedNum++;
#endif
    return retVal;
}

