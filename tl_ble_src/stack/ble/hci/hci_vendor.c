/********************************************************************************************************
 * @file    hci_vendor.c
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
#include "stack/ble/controller/ble_controller.h"
#include "drivers.h"
#include "hci_stack.h"
#include "hci_vendor.h"

typedef unsigned char (*blt_vendor_callback_t)(u8 pCmdparaLen, hci_vendor_CmdParams_t* pCmd, hci_vendor_EndStatusParam_t *pRetParam);

typedef struct __attribute__((packed)) {
    u8 ogf; //2bit  private ogf
    u8 ocf; //8bit
    u16 en;
    blt_vendor_callback_t cb;
}hci_vendor_cmdBuf_t;

typedef struct __attribute__((packed)) {
    u8 ogf; //2bit  private ogf
    hci_vendor_cmdBuf_t cb;
}hci_vendor_MainCmdBuf_t;

static unsigned char Vendor_eventCode = HCI_EVT_CMD_COMPLETE;
static blt_vendor_FuCallback_t blt_vendor_Fu_cb = 0;

void blt_hci_vendor_setEventCode(u8 result)
{
    Vendor_eventCode = result;
}

unsigned char hci_vendor_getCurrentEventCode(void)
{
    return  Vendor_eventCode;
}

////////////////////////////////////////////////HCI_VENDOR_CMD_CBC_OPCODE_OGF function Begin/////////////////////////////////////////////////////////////
static unsigned char blt_hci_vendor_telink_read_reg(u8 pCmdparaLen, hci_vendor_CmdParams_t* pCmd, hci_vendor_EndStatusParam_t *pRetParam)
{
    (void)pCmdparaLen;
    my_dump_str_data(IUT_HCI_VENDOR_LOG_EN, "[HCI][CMD] blt_hci_vendor_telink_read_reg", pCmd, 3);
    u8 result;
    u8 type = pCmd[0];      //0 for digital 1 for analog
    if(type==0)
    {
       u16 addr = (pCmd[2]<<8) + pCmd[1] + 0x800000;
       result = read_reg8(addr);
    }
    else{
        result = analog_read(pCmd[1]);
    }
    Vendor_eventCode = HCI_EVT_CMD_COMPLETE;
    return hci_cmdComplete_evt(1, HCI_TELINK_READ_REG, HCI_CMD_VENDOR_OPCODE_OGF|HCI_VENDOR_CMD_CBC_OPCODE_OGF, 1, &result, pRetParam);
}


static unsigned char blt_hci_vendor_telink_write_reg(u8 pCmdparaLen, hci_vendor_CmdParams_t* pCmd, hci_vendor_EndStatusParam_t *pRetParam)
{
    (void)pCmdparaLen;
    my_dump_str_data(IUT_HCI_VENDOR_LOG_EN, "[HCI][CMD] blt_hci_vendor_telink_write_reg", pCmd, 4);
    u8 type = pCmd[0];      //0 for digital 1 for analog
    u8 value = pCmd[3];
    u8 result = BLE_SUCCESS;
    if(type==0)
    {
        u16 addr = (pCmd[2]<<8) + pCmd[1] + 0x800000;
        write_reg8(addr,value);
    }
    else
        analog_write(pCmd[1],value);
    Vendor_eventCode = HCI_EVT_CMD_COMPLETE;
    return hci_cmdComplete_evt(1, HCI_TELINK_WRITE_REG, HCI_CMD_VENDOR_OPCODE_OGF|HCI_VENDOR_CMD_CBC_OPCODE_OGF, 1, &result, pRetParam);
}

static unsigned char blt_hci_vendor_telink_set_txPower(u8 pCmdparaLen, hci_vendor_CmdParams_t* pCmd, hci_vendor_EndStatusParam_t *pRetParam)
{
    (void)pCmdparaLen;
    my_dump_str_data(IUT_HCI_VENDOR_LOG_EN, "[HCI][CMD] blt_hci_vendor_telink_set_txPower", pCmd, 1);
    u8 power = pCmd[0];
    u8 result = BLE_SUCCESS;
    rf_set_power_level_index(power);
    Vendor_eventCode = HCI_EVT_CMD_COMPLETE;
    return hci_cmdComplete_evt(1, HCI_TELINK_SET_TX_PWR, HCI_CMD_VENDOR_OPCODE_OGF|HCI_VENDOR_CMD_CBC_OPCODE_OGF, 1, &result, pRetParam);
}

static unsigned char blt_hci_vendor_telink_reboot(u8 pCmdparaLen, hci_vendor_CmdParams_t* pCmd, hci_vendor_EndStatusParam_t *pRetParam)
{
    (void)pCmd;(void)pCmdparaLen;
    start_reboot(); //No hci cmd complete return parameters.
    u8 result = BLE_SUCCESS;
    Vendor_eventCode = HCI_EVT_CMD_COMPLETE;
    return hci_cmdComplete_evt(1, HCI_TELINK_REBOOT_MCU, HCI_CMD_VENDOR_OPCODE_OGF|HCI_VENDOR_CMD_CBC_OPCODE_OGF, 1, &result, pRetParam);
}

static unsigned char blt_hci_vendor_telink_set_rxTxDataLen(u8 pCmdparaLen, hci_vendor_CmdParams_t* pCmd, hci_vendor_EndStatusParam_t *pRetParam)
{
    (void)pRetParam;(void)pCmd;(void)pCmdparaLen;
    //u8 power = pCmd[0];
    //todo
    return 0;
}

static unsigned char blt_hci_vendor_telink_writeBDAddr(u8 pCmdparaLen, hci_vendor_CmdParams_t* pCmd, hci_vendor_EndStatusParam_t *pRetParam)
{
    u8 result;
    extern ble_sts_t blc_ll_writeBDAddr(u8 *addr);
    if(pCmdparaLen == 6)
    {
        result = blc_ll_writeBDAddr(&pCmd[0]);
    }
    else
    {
        result =  HCI_ERR_UNKNOWN_HCI_CMD;
    }
    Vendor_eventCode = HCI_EVT_CMD_COMPLETE;
    return hci_cmdComplete_evt(1, HCI_TELINK_REBOOT_MCU, HCI_CMD_VENDOR_OPCODE_OGF|HCI_VENDOR_CMD_CBC_OPCODE_OGF, 1, &result, pRetParam);
}

static unsigned char blt_hci_vendor_telink_ebq_testCaseLog(u8 pCmdparaLen, hci_vendor_CmdParams_t* pCmd, hci_vendor_EndStatusParam_t *pRetParam)
{
    (void)pCmdparaLen;
    static u16 caseNum = 0;
//                  my_dump_str_data(0, "EBQ", p+4, p[3]);

    u32 timestamp = blt_debug_hex_2_dec_display(pCmd[0] | (pCmd[1] <<8) | (pCmd[2]<<16) | (pCmd[3]<<24));
    (void)timestamp; //remove compiler warning

    my_dump_str_data(IUT_HCI_VENDOR_LOG_EN,"########Case Infor########",&timestamp, 4);
    pCmd[pCmd[8]+13] =0;//end str
    my_dump_str_data(0, (char*)(&pCmd[9]), 0, 0); //case name

    caseNum++;

#if(SL16_eqb_testcase_seqNum)
    log_b16(1, SL16_eqb_testcase_seqNum, caseNum);
#endif
//  pRetParam->eventCode = HCI_EVT_CMD_COMPLETE;
//  pRetParam->length=1;
//  pRetParam->param[0]=BLE_SUCCESS;
    u8 result = BLE_SUCCESS;
    Vendor_eventCode = HCI_EVT_CMD_COMPLETE;
    return hci_cmdComplete_evt(1, HCI_EBQ_TEST_CASE_LOG, HCI_CMD_VENDOR_OPCODE_OGF|HCI_VENDOR_CMD_CBC_OPCODE_OGF, 1, &result, pRetParam);

}

hci_vendor_cmdBuf_t hci_vendor_cbc_cmdBuf[] =
{
    {HCI_VENDOR_CMD_CBC_OPCODE_OGF, HCI_TELINK_READ_REG,            true, blt_hci_vendor_telink_read_reg},
    {HCI_VENDOR_CMD_CBC_OPCODE_OGF, HCI_TELINK_WRITE_REG,           true, blt_hci_vendor_telink_write_reg},
    {HCI_VENDOR_CMD_CBC_OPCODE_OGF, HCI_TELINK_SET_TX_PWR,          true, blt_hci_vendor_telink_set_txPower},
    {HCI_VENDOR_CMD_CBC_OPCODE_OGF, HCI_TELINK_REBOOT_MCU,          true, blt_hci_vendor_telink_reboot},
    {HCI_VENDOR_CMD_CBC_OPCODE_OGF, HCI_TELINK_SET_RXTX_DATA_LEN,   true, blt_hci_vendor_telink_set_rxTxDataLen},
    {HCI_VENDOR_CMD_CBC_OPCODE_OGF, HCI_TELINK_SET_BD_ADDR,         true, blt_hci_vendor_telink_writeBDAddr},
    {HCI_VENDOR_CMD_CBC_OPCODE_OGF, HCI_TELINK_READ_TX_PWR,         true, NULL},
    {HCI_VENDOR_CMD_CBC_OPCODE_OGF, HCI_TELINK_SET_FREQ_OFFSET,     true, NULL},
    {HCI_VENDOR_CMD_CBC_OPCODE_OGF, HCI_TELINK_READ_FREQ_OFFSET,    true, NULL},
    {HCI_VENDOR_CMD_CBC_OPCODE_OGF, HCI_TELINK_SET_SCAN_FILTER,     true, NULL},//MESH
    {HCI_VENDOR_CMD_CBC_OPCODE_OGF, HCI_EBQ_TEST_CASE_LOG,          true, blt_hci_vendor_telink_ebq_testCaseLog},
//  {0,                             HCI_TELINK_VENDOR_MAX_CBC,      false, NULL}
};
////////////////////////////////////////////////HCI_VENDOR_CMD_LEA_OPCODE_OGF function End/////////////////////////////////////////////////////////////



////////////////////////////////////////////////HCI_VENDOR_CMD_LEA_OPCODE_OGF function Begin/////////////////////////////////////////////////////////////
//demo
static unsigned char blt_hci_vendor_telink_startLaAudio(u8 pCmdparaLen, hci_vendor_CmdParams_t* pCmd, hci_vendor_EndStatusParam_t *pRetParam)
{
    (void)pCmdparaLen;
    (void)pCmd;
    my_dump_str_data(IUT_HCI_VENDOR_LOG_EN, "[HCI][CMD] blt_hci_vendor_telink_startLaAudio", pCmd, 1);
    u8 result = BLE_SUCCESS;
    Vendor_eventCode = HCI_EVT_CMD_COMPLETE;//must use
    return hci_cmdComplete_evt(1, HCI_OCF_VS_START_LEA, HCI_CMD_VENDOR_OPCODE_OGF|HCI_VENDOR_CMD_LEA_OPCODE_OGF, 1, &result, pRetParam);
}

hci_vendor_cmdBuf_t hci_vendor_lea_cmdBuf[] =
{
    {HCI_VENDOR_CMD_LEA_OPCODE_OGF, HCI_OCF_VS_START_LEA,           true, blt_hci_vendor_telink_startLaAudio},
    {0,                             HCI_OCF_VS_START_LEA_MAX_LEA,   false, NULL}//
};

////////////////////////////////////////////////HCI_VENDOR_CMD_LEA_OPCODE_OGF function End/////////////////////////////////////////////////////////////




ble_sts_t blt_hci_vendor_setFuVendorCallback(blt_vendor_FuCallback_t handler)
{
    blt_vendor_Fu_cb = handler;
    return BLE_SUCCESS;
}


unsigned char hci_vendor_Process(u8 pCmdparaLen, u8 opCode_ogf,u8 opCode_ocf,hci_vendor_CmdParams_t* pCmd, hci_vendor_EndStatusParam_t *pRetParam)
{
    static u8 opcode_ogf_value;
    static u8 ret_length = 0;
    static u32 VendorBufSize;
    opcode_ogf_value = (opCode_ogf & 0x03);
    my_dump_str_u8s(IUT_HCI_VENDOR_LOG_EN,"[HCI][CMD]hci_vendor_Process ogf:ocf:0:0",opcode_ogf_value,opCode_ocf,pCmdparaLen,0);
    Vendor_eventCode = HCI_EVT_CMD_COMPLETE;
    switch(opcode_ogf_value){
    //00
    case HCI_VENDOR_CMD_CBC_OPCODE_OGF:
        VendorBufSize = sizeof(hci_vendor_cbc_cmdBuf)>>3;//8
        for(u32 i= 0; i<VendorBufSize; i++)
        {
            if(hci_vendor_cbc_cmdBuf[i].ocf == opCode_ocf)
            {
                if((hci_vendor_cbc_cmdBuf[i].cb != NULL)&&(hci_vendor_cbc_cmdBuf[i].en == true))
                {
                    ret_length = hci_vendor_cbc_cmdBuf[i].cb(pCmdparaLen, pCmd, pRetParam);
                }
                break;
            }
            if(HCI_TELINK_VENDOR_MAX_CBC == hci_vendor_cbc_cmdBuf[i].ocf)
            {
                break;
            }
        }
        break;
    //01
    case HCI_VENDOR_CMD_FU_OPCODE_OGF:
        if(blt_vendor_Fu_cb)
        {
            static u32 TempCheckRunTick;
            TempCheckRunTick = clock_time();
            //
            ret_length = blt_vendor_Fu_cb(pCmdparaLen, opCode_ocf, pCmd, pRetParam);
            //
            if( clock_time_exceed(TempCheckRunTick, 100000)){ //100ms
                TempCheckRunTick -= clock_time();
                my_dump_str_data(IUT_HCI_VENDOR_WARN_EN,"[HCI][CMD] hci_vendor_Process run time(warn) ",&TempCheckRunTick,4);
            }
        }
        break;

    //02
    case HCI_VENDOR_CMD_LEA_OPCODE_OGF:
        VendorBufSize = sizeof(hci_vendor_lea_cmdBuf)>>3;//8
        for(u32 i= 0; i<VendorBufSize; i++)
        {
            if(hci_vendor_lea_cmdBuf[i].ocf == opCode_ocf)
            {
                if((hci_vendor_lea_cmdBuf[i].cb != NULL)&&(hci_vendor_lea_cmdBuf[i].en == true))
                {
                    ret_length = hci_vendor_lea_cmdBuf[i].cb(pCmdparaLen, pCmd, pRetParam);
                }
                break;
            }
            if(HCI_OCF_VS_START_LEA_MAX_LEA == hci_vendor_cbc_cmdBuf[i].ocf)
            {
                break;
            }
        }
        break;
    //03
    case HCI_VENDOR_CMD_DFU_OPCODE_OGF:
        //This code is currently in app
        break;
    default:

        break;
    }
     return ret_length;
}







