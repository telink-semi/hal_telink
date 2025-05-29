/********************************************************************************************************
 * @file    hci.c
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
#include "stack/ble/controller/ble_controller.h"
#include "hci_vendor.h"
_attribute_ble_data_retention_ hciMng_t bltHciMng;

_attribute_ble_data_retention_ u16 lmp_subversion = 0;
///////////////////////////////////////////////////////////////////////////////////////////////////////
//  controller hci event call back function
///////////////////////////////////////////////////////////////////////////////////////////////////////
_attribute_ble_data_retention_ hci_event_handler_t   blc_hci_event_handler     = NULL;
_attribute_ble_data_retention_ hci_data_handler_t    blc_hci_data_handler      = NULL;
_attribute_ble_data_retention_ hci_iso_data_handle_t blt_hci_iso_data_handler  = NULL;
_attribute_ble_data_retention_ hci_event_handler_t   blt_gap_hci_event_handler = NULL;


hci_fifo_t                                bltHci_rxfifo;
hci_fifo_t                                bltHci_txfifo;
_attribute_ble_data_retention_ hci_fifo_t bltHci_rxAclfifo;  /* H2C */
_attribute_ble_data_retention_ hci_fifo_t bltHci_outIsofifo; /* controller report hci iso data to host */

_attribute_data_retention_ u16 hciRevision = 0x22bb;

_attribute_data_retention_ u16 gHciPortNum = 0xff;

void hci_set_revision(u16 revision)
{
    hciRevision = revision;
}

u16 hci_get_revision(void)
{
    return hciRevision;
}

void hci_enableHciCmdCmplEvtForUnsupportedCmd(u8 en)
{
    bltHciMng.hciCmplEvtEn = en;
}

/**
 * @brief      for user to initialize HCI TX FIFO.
 * @param[in]  pRxbuf - TX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initHciTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number)
{
    /* number must be 2^n */
    if (IS_POWER_OF_2(fifo_number)) {
        bltHci_txfifo.num  = fifo_number;
        bltHci_txfifo.mask = fifo_number - 1;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    /* size must be 4*n */
    if ((fifo_size & 3) == 0) {
        bltHci_txfifo.size = fifo_size;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    bltHci_txfifo.wptr = bltHci_txfifo.rptr = 0;
    bltHci_txfifo.p                         = pTxbuf;

    return BLE_SUCCESS;
}

/**
 * @brief      for user to initialize HCI RX FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initHciRxFifo(u8 *pRxbuf, int fifo_size, int fifo_number)
{
    /* number must be 2^n */
    if (IS_POWER_OF_2(fifo_number)) {
        bltHci_rxfifo.num  = fifo_number;
        bltHci_rxfifo.mask = fifo_number - 1;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    /* size must be 4*n */
    if ((fifo_size & 3) == 0) {
        bltHci_rxfifo.size = fifo_size;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    bltHci_rxfifo.wptr = bltHci_rxfifo.rptr = 0;
    bltHci_rxfifo.p                         = pRxbuf;

    return BLE_SUCCESS;
}

/**
 * @brief      for user to initialize HCI RX ACL Data FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer address
 * @param[in]  fifo_size - RX FIFO size
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initHciAclDataFifo(u8 *pAclbuf, int fifo_size, int fifo_number)
{
    bltempParam.hci_aclRxFifo_set = 1; //for later buffer check

    /* number must be 2^n */
    if (IS_POWER_OF_2(fifo_number)) {
        bltHci_rxAclfifo.num  = fifo_number;
        bltHci_rxAclfifo.mask = fifo_number - 1;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    /* size must be 4*n */
    if ((fifo_size & 3) == 0) {
        bltHci_rxAclfifo.size = fifo_size;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    bltHci_rxAclfifo.wptr = bltHci_rxAclfifo.rptr = 0;
    bltHci_rxAclfifo.p                            = pAclbuf;


    return BLE_SUCCESS;
}


#if (LL_FEATURE_ENABLE_ISO)


/**
 * @brief      for user to initialize HCI ISO out buffer (controller report HCI ISO data to host).
 * @param[in]  pRxbuf - RX FIFO buffer address
 * @param[in]  fifo_size - RX FIFO size
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initHciTxIsoDataFifo(u8 *pIsobuf, int fifo_size, int fifo_number)
{
    /* number must be 2^n */
    if (IS_POWER_OF_2(fifo_number)) {
        bltHci_outIsofifo.num  = fifo_number;
        bltHci_outIsofifo.mask = fifo_number - 1;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    /* size must be 4*n */
    if ((fifo_size & 3) == 0) {
        bltHci_outIsofifo.size = fifo_size;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }

    bltHci_outIsofifo.wptr = bltHci_outIsofifo.rptr = 0;
    bltHci_outIsofifo.p                             = pIsobuf;


    return BLE_SUCCESS;
}

#endif


void blc_hci_registerControllerEventHandler(hci_event_handler_t handler)
{
    blc_hci_event_handler = handler; ///app_controller_event_callback  ;controller---blc_hci_send_data
}

void blt_gap_registerEventHandler(hci_event_handler_t handler)
{
    blt_gap_hci_event_handler = handler; //blt_gap_ble_hci_rever_handler
}
#if HCI_SEND_NUM_OF_CMP_AFT_ACK
_attribute_ram_code_
#endif
    int
    blc_hci_send_event(u32 h, u8 *para, int n)
{
    if (blt_gap_hci_event_handler) {
        blt_gap_hci_event_handler(h, para, n);
    }

    if (blc_hci_event_handler) ///app_controller_event_callback  blc_hci_send_data
    {
        return blc_hci_event_handler(h, para, n);
    }
    return 1;
}

#ifdef MCU_CORE_D25F_ENABLE
int blc_hci_send_event1(u32 h, u8 *para, int n)
{
    if (blt_gap_hci_event_handler) {
        blt_gap_hci_event_handler(h, para, n); //blt_gap_ble_hci_rever_handler
    }

    if (blc_hci_event_handler) {//app_controller_event_callback  blc_hci_send_data
        return blc_hci_event_handler(h, para, n);
    }

    return 1;
}
#endif

void blc_hci_registerControllerDataHandler(hci_data_handler_t handle)
{
    blc_hci_data_handler = handle;
}

_attribute_ble_data_retention_ u32 hci_eventMask        = HCI_EVT_MASK_DEFAULT;
_attribute_ble_data_retention_ u32 hci_eventMask_2      = HCI_EVT_MASK_2_DEFAULT;

_attribute_ble_data_retention_ u32 hci_eventMaskPage2   = HCI_EVT_MASK_PAGE2_DEFAULT;
_attribute_ble_data_retention_ u32 hci_eventMaskPage2_2 = HCI_EVT_MASK_PAGE2_2_DEFAULT;

_attribute_ble_data_retention_ u32 hci_le_eventMask     = HCI_LE_EVT_MASK_DEFAULT; //11 event in core_4.2
_attribute_ble_data_retention_ u32 hci_le_eventMask_2   = HCI_LE_EVT_MASK_NONE;

ble_sts_t blc_hci_setEventMask_cmd(u32 evtMask)
{
    hci_eventMask = evtMask;
    return BLE_SUCCESS;
}

ble_sts_t blc_hci_setEventMask_2_cmd(u32 evtMask_2)
{
    hci_eventMask_2 = evtMask_2;
    return BLE_SUCCESS;
}

ble_sts_t blc_hci_setEventMaskPage2_cmd(u32 evtMask)
{
    hci_eventMaskPage2 = evtMask;
    return BLE_SUCCESS;
}

ble_sts_t blc_hci_setEventMaskPage2_2_cmd(u32 evtMask_2)
{
    hci_eventMaskPage2_2 = evtMask_2;
    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setEventMask_cmd(u32 evtMask)
{
    hci_le_eventMask = evtMask;
    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setEventMask_2_cmd(u32 evtMask_2)
{
    hci_le_eventMask_2 = evtMask_2;
    return BLE_SUCCESS;
}

ble_sts_t blc_setHciInBufferMaxOctets(u16 isoDataInFifo_size, u8 isoDataInFifo_num)
{
    if (!IS_POWER_OF_2(isoDataInFifo_num)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    bltHciMng.isoDataInFifo_num  = isoDataInFifo_num;
    bltHciMng.isoDataInFifo_size = isoDataInFifo_size;

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_readBufferSize_v2_cmd(hci_le_readBufSize_v2_retParam_t *pRetPara)
{
    if (!bltHci_rxAclfifo.num || !blmsParam.acl_packet_length) {
        my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_Buf_Size_V2, fail", 0, 0);
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }

    pRetPara->status = BLE_SUCCESS;


    /*
     *  The ISO_Data_Packet_Length return parameter shall be used to determine the
     *  maximum size of the SDU segments that are contained in isochronous data
     *  packets, and which are transferred from the Host to the Controller. The
     *  Total_Num_ISO_Data_Packets return parameter contains the total number of
     *  isochronous data packets that can be stored in the data buffers of the
     *  Controller
     */

    pRetPara->iso_data_pkt_len = bltHciMng.isoDataInFifo_size + 4; // return iso_data_load_len
    pRetPara->num_le_iso_pkt   = bltHciMng.isoDataInFifo_num;

    //LE_ACL_Data_Packet_Length not bigger than 251(max DLE) in our design
    pRetPara->acl_data_pkt_len = blmsParam.acl_packet_length;
    pRetPara->num_le_data_pkt  = bltHci_rxAclfifo.num - 1; //leave 1 for host error handle


    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_Buf_Size_V2", &pRetPara->acl_data_pkt_len, 6);


    return BLE_SUCCESS;
}

///modify the variable bltData in this function later.---qiuwei
/////////////////////////////////
//  send ACL packet to HCI
/////////////////////////////////
int blc_hci_sendACLData2Host(u16 connHandle, u8 *p)
{
#if (LL_FEATURE_SUPPORT_LE_DATA_LENGTH_EXTENSION)
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    //start    pkt:  llid 2 -> 0x02
    //continue pkt:  llid 1 -> 0x01

    int len = p[1];

    ll_data_extension_t *pExt_data = &blms[conn_idx].ext_data;

    #if (HCI_CONTROLLER_TO_HOST_FLOW_CTRL_EN)
    st_ll_conn_t *pc = (st_ll_conn_t *)&blms[conn_idx];
    if (pc->connState == CONN_STATUS_DISCONNECT) {
        return 0;
    }
    #endif

    int report_len = min(len, pExt_data->connEffectiveMaxRxOctets);
    p[1]           = report_len;
    blc_hci_send_event(HCI_FLAG_ACL_BT_STD | connHandle, p, report_len);

    for (int i = report_len; i < len; i += pExt_data->connEffectiveMaxRxOctets) {
        //debug:  this should never happen
        BLMS_ERR_DEBUG(DBG_HCI_FIFO, 0xCCCC0000);

        report_len = (len - i) > pExt_data->connEffectiveMaxRxOctets ? pExt_data->connEffectiveMaxRxOctets : (len - i);
        p[0]       = L2CAP_CONTINUING_PKT; //llid, fragment pkt
        p[1]       = report_len;
        smemcpy(p + 2, p + 2 + i, report_len);
        blc_hci_send_event(HCI_FLAG_ACL_BT_STD | connHandle, p, report_len);
    }

#else
    //para[0]&3: llid    para[1]+5: rf_len+5,total data len     para[2] data begin
    blc_hci_send_event(HCI_FLAG_ACL_BT_STD | connHandle, p, p[1]);
#endif


    return 0;
}

void blc_hci_registerControllerIsoDataHandler(hci_iso_data_handle_t handle)
{
    blt_hci_iso_data_handler = handle;
}

int blc_hci_sendIsoData2Host(u8 *p, int data_len)
{
    return blc_hci_send_event(HCI_FLAG_ISO_DATE_STD, p, data_len);
}


#if (LL_FEATURE_ENABLE_ISO)

/**
 * @brief      This function is used to pack HCI ISO data packet to SDU packet.
 * @param[in]  cis_connHandle - point to handle of cis.
 * @param[in]  pIsoData - point to hci ISO Data packet buff.
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #endif
    ble_sts_t
    blc_hci_packIsoData(iso_data_packet_t *pIsoDatPkt)
{
    #if (FANQH_OPTIMIZE_BIS_API)
    if ((pIsoDatPkt->connHandle & BLT_CIS_HANDLE) || (pIsoDatPkt->connHandle & BLT_BIS_HANDLE)) {
        return blt_hci_processIsoData(pIsoDatPkt);
    } else {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }
    #else
    if (pIsoDatPkt->connHandle & BLT_CIS_HANDLE) {
        return blt_hci_processIsoData(pIsoDatPkt);
    }


    u8           *pIsoSdu, *pSduWptr = NULL;
    sdu_packet_t *iso_sdu        = NULL;
    u16           max_inFifoSize = 0;

    int isoDataLoad_len = pIsoDatPkt->iso_dat_len & 0x3fff;
    int isoSdu_len      = 0;

    u16 connHandle = pIsoDatPkt->connHandle;
    if (connHandle & BLT_BIS_HANDLE) {
        u8 bis_sel = connHandle & BLT_BIS_IDX_MSK;

        ll_bis_t *pBis = (ll_bis_t *)(global_pBis + bis_sel);
        iso_sdu        = (sdu_packet_t *)(sduBisMng.in_fifo_b + (sduBisMng.in_fifo_num * bis_sel +
                                                          (pBis->bisSduIn_wptr & sduBisMng.in_fifo_mask)) *
                                                             sduBisMng.max_in_fifo_size);
        pSduWptr       = &pBis->bisSduIn_wptr;
        max_inFifoSize = sduBisMng.max_in_fifo_size;
    } else {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }


    //PB_Flag
    u8 pb = pIsoDatPkt->pb;


    /* SiHui note: state machine need improve to handle error data by Host */
    if ((pb == HCI_ISO_SDU_FIRST_FRAG) || (pb == HCI_ISO_SDU_COMPLETE)) {
        iso_sdu->sduOffset = 0;

        if (pIsoDatPkt->ts) {
            iso_data_load_1_t *iso_load = (iso_data_load_1_t *)pIsoDatPkt->p_ISO_data_load;
            iso_sdu->timestamp          = iso_load->timestamp;
            iso_sdu->pkt_seq_num        = iso_load->pkt_seq;
            iso_sdu->iso_sdu_len        = iso_load->iso_sdu_len & 0xfff;
            iso_sdu->pkt_st             = iso_load->ps;

            pIsoSdu    = iso_load->iso_sdu;   //pIsoDatPkt->p_ISO_data_load + OFFSETOF(iso_data_load_1_t, iso_sdu); // + 8
            isoSdu_len = isoDataLoad_len - 8; // 8 = timestamp 4 + packet sequence number 2 + iso_sdu_len 2
        } else {
            iso_data_load_2_t *iso_load = (iso_data_load_2_t *)pIsoDatPkt->p_ISO_data_load;
            iso_sdu->pkt_seq_num        = iso_load->pkt_seq;
            iso_sdu->iso_sdu_len        = iso_load->iso_sdu_len & 0xfff;
            iso_sdu->pkt_st             = iso_load->ps;

            pIsoSdu    = iso_load->iso_sdu;
            isoSdu_len = isoDataLoad_len - 4; // 4 = packet sequence number 2 + iso_sdu_len 2
        }


        if (isoSdu_len > max_inFifoSize) {
            return HCI_ERR_PACKET_TOO_LONG;
        }

        iso_sdu->numHciPkt = 1; //mark Num of hci packet
        //copy iso_data_load
        smemcpy((iso_sdu->data + iso_sdu->sduOffset), pIsoSdu, isoSdu_len);
        iso_sdu->sduOffset += isoSdu_len;

        iso_sdu->ts = pIsoDatPkt->ts;

        //my_dump_str_data(0, "HCI ISO Rec data len", &isoDataLen, 2);

        if (pb == HCI_ISO_SDU_COMPLETE) {
            my_dump_str_u32s(0, "hci iso", iso_sdu->sduOffset, iso_sdu->iso_sdu_len, 0, 0);
            if (iso_sdu->sduOffset == iso_sdu->iso_sdu_len) {
                iso_sdu->sduOffset     = 0;
                iso_sdu->numOfCmplt_en = 1;
                (*pSduWptr)++;
            } else {
            }
        }
    } else if ((pb == HCI_ISO_SDU_CONTINUE_FRAG) || (pb == HCI_ISO_SDU_LAST_FRAG)) {
        /*The fields Time_Stamp, Packet_Sequence_Number, Packet_Status_Flag and
            ISO_SDU_Length are only included in the HCI ISO Data packet when the
            PB_Flag equals 0b00 or 0b10.*/

        pIsoSdu    = pIsoDatPkt->p_ISO_data_load;
        isoSdu_len = isoDataLoad_len;


        if ((isoSdu_len + iso_sdu->sduOffset) > max_inFifoSize) {
            return HCI_ERR_PACKET_TOO_LONG;
        }

        smemcpy((iso_sdu->data + iso_sdu->sduOffset), pIsoSdu, isoSdu_len);
        iso_sdu->sduOffset += isoSdu_len;


        if ((pb == HCI_ISO_SDU_LAST_FRAG)) {
            if (iso_sdu->sduOffset == iso_sdu->iso_sdu_len) {
                iso_sdu->sduOffset     = 0;
                iso_sdu->numOfCmplt_en = 1;
                (*pSduWptr)++;
            } else {
            }
        }
        iso_sdu->numHciPkt++; //mark Num of hci packet
    }

    iso_sdu->pb = pb;

        #if (IUT_HCI_LOG_EN)
    my_dump_str_u32s(IUT_HCI_LOG_EN, "HCI ISO info", connHandle, *pSduWptr, pIsoDatPkt->ts, isoSdu_len);
    int dump_len = isoSdu_len > 256 ? 256 : isoSdu_len;
    my_dump_str_data(IUT_HCI_LOG_EN, "@HCI_ISO_SDU in", pIsoSdu, dump_len);
        #endif


    return BLE_SUCCESS;
    #endif
}

#endif


//////////////////////////////////////////////////////////////////////////////////
//  HCI
//      1. from l2cap function: from/to link layer (packet_handler/push fifo)
//      2. from host: (46 commands)
//              01: command; 02: ACL; 03: synchronous data; 04: event
//               ---- command ---
//               01 cmd_code_16 length_8 parameter
//               01 06 04 03 01 00 13: disconnect (handle 01 00; reason: 0x13)
//               01 1d 04 02 01 00   : read remote version info
//               01 01 0c   ... set event mask
//               01 03 0c   ... reset hci
//               01 14 0c   ... read local name
//                      03(0c) --> xx(10) ->    20 18 1c 1e 24 13 1a
//                      09 02 05 01 03
//               01 01 10   ... read local version info
//               01 02 10   ... read local supported command
//               01 03 10   ... read local supported feature
//               01 05 10   ... read buffer size
//               01 09 10   ... read BD address
//               01 01 20   ... group 08 (0x20)
//                  01 set event mask
//                  02 read buffer size
//                  03 read local supported feature
//                  04 set random address
//                  05 set advertising parameter
//                  06 set advertising channel Tx power
//                  07 set advertising data
//                  08 set scan response data
//                  09 set advertise enable
//                  0a set scan parameters
//                  0b set scan enable
//                  0c create connection
//                  0d create connection cancel
//                  0e read white list size
//                  0f clear white list
//                  10 add device to white list
//                  11 remove device from white list
//                  12 connection update
//                  13 set host channel classification
//                  14 read channel map
//                  15 read remote used feature
//                  16 encrypt
//                  17 random
//                  18 start encryption
//                  19 long term key request reply
//                  1a long term key request negative
//                  1b read supported states
//                  1c receiver test
//                  1d transmitter test
//                  1e test end
//                  1f remote connection parameter request reply        (4.1)
//                  20 remote connection parameter request negative reply
//                  21 set data length                                  (4.2)
//                  22 read suggested default data length
//                  23 write suggested default data length
//                  24 read local P-256 public key
//                  25 generate DHKey
//                  26 add device to resolving list
//                  27 remove device from resolving list
//                  28 clear resolving list
//                  29 read resolving list size
//                  2a read peer resolvable address
//                  2b read local resolvable address
//                  2c set address resolution enable
//                  2d set resolvable private address timeout
//                  2e read maximum data length
//  controller to host event
//               ---- event ---
//               04 3e 01 status8 connHandle16 role8...
//               04 3e 02 ... advertising report
//               04 3e 03 ...  connection update compete
//               04 3e 04 ...  read remote used feature complete
//               04 3e 05 ...  long term key request event
//               04 3e 06 ...  remote connection parameter request (4.1)
//               04 3e 07 ...  data length change   (4.2)
//               04 3e 08 ...  read local P-256 public key complete
//               04 3e 09 ...  generate DHKey complete
//               04 3e 0a ...  enhanced connection complete
//               04 3e 0b ...  direct advertising report
//               04 03 ...    connection complete
//               04 05 ...    disconnection complete
//               04 08 ...    encryption change
//               04 0c 08 00 01 00 06 11 02 08 00: read remote version complete command
//               04 0e ...    command complete
//               04 0f ...    command status
//               04 13 ...    number of complete packet
// host/controller ACL data format
//               ---- ACL ---
//               02 handle_16 length_16 l2cap_payload:
//                      handle_16( handle_12, packet_boundary_flag_2, broadcast_flag_2)
//                      pbf_2: 00 host-to-controller; 2 controller-to-host
///////////////////////////////////////////////////////////////////////////////////
//  vendor command (SPP module)
//  ----  vendor event --------
//  ff 03 01 07  00 command complete
//  ff 03 30 07  04 (stack state change event: 0(reset) 1(standby) 2(prepare advertising)
//                                             3(advertising) 4(connected) 5(terminated)
//                                             6(error) 7(encrypted) 8(bonded)
//  ff n+2 31 07 d1 .. dn   data received
//  ff 03 32 07 00 data sent  0(ok) 1(fail) 2(data too long) 3(wrong data length) 4(no connection)
//                            5(data transmission busy)
//  ff 03 0c 07 n get available buffer number
//////////////////////////////////////////

_attribute_ble_data_retention_ blc_hci_rx_handler_t   blc_hci_rx_handler   = 0;
_attribute_ble_data_retention_ blc_hci_tx_handler_t   blc_hci_tx_handler   = 0;
_attribute_ble_data_retention_ blc_hci_user_handler_t blc_hci_user_handler = 0;

#define BTUSB_TYPE "Telink Controller"
const u8 blc_controller_name[] = BTUSB_TYPE;

void blc_register_hci_handler(void *prx, void *ptx)
{
    blc_hci_rx_handler = prx; ///rx_from_uart_cb   //controller project: HCI_Tr_RxHandlerCback
    blc_hci_tx_handler = ptx; ///tx_to_uart_cb     //controller project: HCI_Tr_TxHandlerCback
}

void blc_hci_register_user_handler(void *usrHandler)
{
    blc_hci_user_handler = usrHandler;
}

void blc_set_customer_lmp_subversion(u16 subversion)
{
    lmp_subversion = subversion;
}

/////////////////////////////////

Hci_localName_t bltHci_localName = {
    BLE_SUCCESS,
    {"Telink_Ble"},
    11 /*11 is "Telink Ble" length*/
};

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int blc_hci_handler(u8 *p, int n)
{
    //tlkapi_send_string_data(1, "hci rx",p, n);

    /* user handler call-back */
    if (blc_hci_user_handler) {
        if (blc_hci_user_handler(p, n)) {
            return 0;
        }
    }

    if (p[0] == HCI_TYPE_ACL_DATA) {
        // must make sure p is align 2 byte or the type cast will error
        blc_hci_receiveHostACLData((hci_acl_data_pkt_t *)(p + 1));

        return 1;
    }
    else if (p[0] == HCI_TYPE_ISO_DATA) {
    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
        // must make sure p is align 2 byte or the type cast will error
        blc_hci_packIsoData((iso_data_packet_t *)(p + 1));
    #endif

        return 1;
    }
    else if (p[0] == HCI_TYPE_CMD) {
        blc_hci_cmd_handler(p);
    }

    return 0;
}

#if CS_EBQ_TEST
u32 hci_reset_flag = 0;
#endif


_attribute_no_inline_ int blc_hci_cmd_handler(u8 *p)
{
    u8   status   = BLE_SUCCESS;
    u32  header   = 0;
    u8   para[72] = {0};
    u32 *para32   = (u32 *)para;

    u8 *cmdPara = p + 4; //cmdPara is 4 byte aligned
    u8  eventCode;
    u8  resultLen = 0;
    u8  opcode    = p[1];


    //link control cmd--OGF(0x01)
    if (p[2] == HCI_CMD_LINK_CTRL_OPCODE_OGF) {
        if (opcode == HCI_CMD_INQUIRY) {
            status    = HCI_ERR_UNKNOWN_HCI_CMD;
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LINK_CTRL_OPCODE_OGF, status, para);
        } else if (opcode == HCI_CMD_DISCONNECT) {
            status    = (u8)blc_hci_disconnect((hci_disconnect_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LINK_CTRL_OPCODE_OGF, status, para);
        } else if (opcode == HCI_CMD_READ_REMOTE_NAME_REQ) {
            //              // send event remote name request complete event
            //              hci_remoteNateReqComplete_evt (p + 4);
            // send command status first, and send remote name request complete event.
            status    = HCI_ERR_UNKNOWN_HCI_CMD; //BLE_SUCCESS;
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LINK_CTRL_OPCODE_OGF, status, para);
        }
        //  1d read remote version
        else if (opcode == HCI_CMD_READ_REMOTE_VER_INFO) {
            //TODO: add code here, refer to "bls_ll_readRemoteVersion" by zhiTao,
            //and final use slave/master shared API "blc_ll_readRemoteVersion"
            status    = blc_ll_readRemoteVersion((p[5] << 8) | p[4]);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LINK_CTRL_OPCODE_OGF, status, para);
        }
    }
    ///controller & Baseband commands--OGF(0x03)
    else if (p[2] == HCI_CMD_CBC_OPCODE_OGF) {
        switch (opcode) {
        //  01 set event mask, classic
        case HCI_CMD_SET_EVENT_MASK:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Set_Evt_Mask", cmdPara, 8);
            blc_hci_setEventMask_cmd(cmdPara[0] | cmdPara[1] << 8 | cmdPara[2] << 16 | cmdPara[3] << 24);
            blc_hci_setEventMask_2_cmd(cmdPara[4] | cmdPara[5] << 8 | cmdPara[6] << 16 | cmdPara[7] << 24);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, 1, &status, para);
        } break;

        //  03 reset HCI
        case HCI_CMD_RESET:
        {
            status = (u8)blc_hci_reset();
#if CS_EBQ_TEST
            hci_reset_flag++;
#endif

            eventCode                 = HCI_EVT_CMD_COMPLETE;
            resultLen                 = hci_cmdComplete_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, 1, &status, para);
            blmsParam.standard_hci_en = 1; //special use: to know controller working mode
        } break;
#if !BQB_TEST_EN
        case HCI_CMD_WRITE_LOCAL_NAME:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Write_Local_Name", cmdPara, bltHci_localName.len);
            bltHci_localName.len = strlen((const char *)cmdPara);
            memcpy(bltHci_localName.buf, cmdPara, bltHci_localName.len);
            status    = BLE_SUCCESS;
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, 1, &status, para);
        } break;
        case HCI_CMD_READ_LOCAL_NAME:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_Local_Name", 0, 0);
            bltHci_localName.status = BLE_SUCCESS;
            eventCode               = HCI_EVT_CMD_COMPLETE;
            resultLen               = hci_cmdComplete_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, bltHci_localName.len + 1, &(bltHci_localName.status), para);
        } break;
#endif
#if HCI_CONTROLLER_TO_HOST_FLOW_CTRL_EN
        case HCI_CMD_SET_CONTROLLER_TO_HOST_FLOW_CTRL:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Set_Controller_To_Host_Flow_Ctrl", 0, 0);
            status    = hci_setControllerToHostFlowCtrl(cmdPara[0]);
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, 1, &status, para);
            eventCode = HCI_EVT_CMD_COMPLETE;
        } break;
        case HCI_CMD_HOST_BUF_SIZE:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Host_Buf_Size", 0, 0);
            hci_hostBufferSize_cmdParam_t *BufSize = (hci_hostBufferSize_cmdParam_t *)cmdPara;
            status                                 = hci_hostBufferSize(BufSize);
            eventCode                              = HCI_EVT_CMD_COMPLETE;
            resultLen                              = hci_cmdComplete_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, 1, &status, para);
        } break;
        case HCI_CMD_HOST_NUM_OF_COMPLETE_PACKETS:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Host_Num_Of_Complete_Packets", 0, 0);
            hci_hostNumOfCompletedPkt_cmdParam_t *CompPackCom = (hci_hostNumOfCompletedPkt_cmdParam_t *)cmdPara;
            status                                            = hci_hostNumCompletedPackets(CompPackCom);
            if (status == BLE_SUCCESS) {
                return 0;
            }
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, 1, &status, para);
        } break;
#endif

        case HCI_CMD_SET_EVT_MASK_PAGE_2:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Set_Evt_Mask_Page_2", cmdPara, 4);
            status    = blc_hci_setEventMaskPage2_cmd(cmdPara[0] | cmdPara[1] << 8 | cmdPara[2] << 16 | cmdPara[3] << 24);
            blc_hci_setEventMaskPage2_2_cmd(cmdPara[4] | cmdPara[5] << 8 | cmdPara[6] << 16 | cmdPara[7] << 24);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, 1, &status, para);
            break;
        }

#if (LL_FEATURE_ENABLE_LE_PING && LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN)
        case HCI_CMD_READ_AUTH_PAYLOAD_TIMEOUT:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_Auth_Payload_Timeout", 0, 0);
            u8 returnPara[sizeof(hci_readAuthPduTimeout_retParam_t)] = {0};
            blc_hci_readAuthPayloadTimeout(cmdPara[1] << 8 | cmdPara[0], (hci_readAuthPduTimeout_retParam_t *)returnPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, sizeof(returnPara), &returnPara[0], para);
            break;
        }
        case HCI_CMD_WRITE_AUTH_PAYLOAD_TIMEOUT:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Write_Auth_Payload_Timeout", cmdPara, 4);
            u8 res[sizeof(hci_writeAuthPayloadTimeout_retParam_t)] = {0};
            blc_hci_writeAuthPayloadTimeout((hci_writeAuthPayloadTimeout_cmdParam_t *)cmdPara, (hci_writeAuthPayloadTimeout_retParam_t *)&res[0]);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, sizeof(res), &res[0], para);
            break;
        }
#endif

#if (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION)
        case HCI_CMD_READ_AFH_CHN_ASSESSMENT_MODE:
        {
            if (blmsParam.chncSup_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_Afh_Chn_Assessment_Mode", 0, 0);
                u8 returnPara[2] = {0};
                returnPara[0]    = blc_ll_chnclassRdAfhChnAssessmentMode(&returnPara[1]);
                eventCode        = HCI_EVT_CMD_COMPLETE;
                resultLen        = hci_cmdComplete_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, 2, (u8 *)&returnPara[0], para);
            }
        } break;

        case HCI_CMD_WRITE_AFH_CHN_ASSESSMENT_MODE:
        {
            if (blmsParam.chncSup_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Write_Afh_Chn_Assessment_Mode", cmdPara, 1);
                status    = blc_ll_chnclassWrAfhChnAssessmentMode(cmdPara[0]);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, 1, &status, para);
            }
        } break;
#endif

        case HCI_CMD_DELETE_STORED_LINK_KEY:
        case HCI_CMD_SET_EVENT_FILTER:
        case HCI_CMD_WRITE_PIN_TYPE:
        case HCI_CMD_CREATE_NEW_UINT_KEY:
        case HCI_CMD_WRITE_CONNECTION_ACCEPT_TIMEOUT:
        case HCI_CMD_WRITE_PAGE_TIMEOUT:
        case HCI_CMD_WRITE_SCAN_ENABLE:
        case HCI_CMD_WRITE_PAGE_SCAN_ACTIVITY:
        case HCI_CMD_WRITE_INQUIRY_SCAN_ACTIVITY:
        case HCI_CMD_WRITE_AUTHENTICATION_ENABLE:
        case HCI_CMD_WRITE_CLASS_OF_DEVICE:
        case HCI_CMD_WRITE_VOICE_SETTING:
        case HCI_CMD_WRITE_NUM_BROADCAST_RETRANSMISSIONS:
        case HCI_CMD_WRITE_HOLD_MODE_ACTIVITY:
        case HCI_CMD_SYNCHRONOUS_FLOW_CONTROL_ENABLE:
        case HCI_CMD_WRITE_CURRENT_IAC_LAP:
        case HCI_CMD_WRITE_INQUIRY_SCAN_TYPE:
        case HCI_CMD_WRITE_INQUIRY_MODE:
        case HCI_CMD_WRITE_PAGE_SCAN_TYPE:
        default:
            if (bltHciMng.hciCmplEvtEn) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Unprocessed Command", &opcode, 1);
                status    = BLE_SUCCESS;
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, 1, &status, para);
            } else { //Run #449 - /HCI/GEV/BV-01-C  [Unsupported Commands on each supported controller]
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Unsupported Command", &opcode, 1);
                status    = HCI_ERR_UNKNOWN_HCI_CMD;
                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                hci_cmdStatus_evt(1, opcode, HCI_CMD_CBC_OPCODE_OGF, status, para);
            }
            break;
        }
    }
    //Informational Parameters--OGF(0x04)
    else if (p[2] == HCI_CMD_IP_OPCODE_OGF) {
        switch (opcode) {
        case HCI_CMD_READ_LOCAL_VER_INFO:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_Local_Ver_Info", 0, 0);
            const u8 tbl[9] = {
                0x00,                      //status
                BLUETOOTH_VER,             //HCI Version,Bluetooth Core Specification 5.0
                hciRevision & 0xff,
                (hciRevision >> 8) & 0xff, //HCI Revision
                BLUETOOTH_VER,             //LMP/PAL Version, Bluetooth Core Specification 5.0
                //NOTE: some special case vendor_id should be changed,e.g. some customer buy our IC but declared their own IC, we need change vendor_id for them
                VENDOR_ID_LO_B,
                VENDOR_ID_HI_B,               //Manufacturer_Name,
                lmp_subversion & 0xff,
                (lmp_subversion >> 8) & 0xff, //LMP/PAL_Subversion implementation dependent
            };
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_IP_OPCODE_OGF, 9, (u8 *)(u32)tbl, para);
            eventCode = HCI_EVT_CMD_COMPLETE;
        } break;

        /* 6.27 SUPPORTED COMMANDS */
        case HCI_CMD_READ_LOCAL_SUPPORTED_CMDS:
        {
            u8 returnPara[sizeof(hci_readLocSupCmds_retParam_t)];
            blc_hci_readLocalSupportedCommands((hci_readLocSupCmds_retParam_t *)returnPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_IP_OPCODE_OGF, sizeof(hci_readLocSupCmds_retParam_t), returnPara, para);
        } break;

        case HCI_CMD_READ_LOCAL_SUPPORTED_FEATURES:
        {
            u8 returnPara[sizeof(hci_readLocSupFeatures_retParam_t)];
            blc_hci_readLocalSupportedFeatures((hci_readLocSupFeatures_retParam_t *)returnPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_IP_OPCODE_OGF, sizeof(hci_readLocSupFeatures_retParam_t), returnPara, para);
        } break;

#if !BQB_TEST_EN //HCI/GEV/BV-01-C   [Unsupported Commands on each supported controller]
        case HCI_CMD_READ_EXTENDED_LOCAL_SUPPORTED_FEATURES:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_Extended_Local_Supported_Features", 0, 0);
            u8 page[11] = {0x00, 0x00, 0x01, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            if (cmdPara[0] == 0) {
                page[1] = 0;
            } else if (cmdPara[0] == 1) {
                page[1] = 1;
            }
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_IP_OPCODE_OGF, 11, page, para);
            eventCode = HCI_EVT_CMD_COMPLETE;
        } break;

        case HCI_CMD_READ_BUFFER_SIZE_COMMAND: //not support in LE only
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_Buf_Size", 0, 0);
            //const u8 tbl[7] = {0x1b,0x00, 0x40, 0x04,0x00, 0x01,0x00};
            const u8 tbl[8] = {0x00, 0x36, 0x01, 0x40, 0x0a, 0x00, 0x08, 0x00};
            resultLen       = hci_cmdComplete_evt(1, opcode, HCI_CMD_IP_OPCODE_OGF, 8, (u8 *)(u32)tbl, para);
            eventCode       = HCI_EVT_CMD_COMPLETE;
        } break;
#endif

        case HCI_CMD_READ_BD_ADDR:
        {
            u8 returnPara[8];
            returnPara[0] = blc_ll_readBDAddr(&(returnPara[1]));
            resultLen     = hci_cmdComplete_evt(1, opcode, HCI_CMD_IP_OPCODE_OGF, 7, returnPara, para);
            eventCode     = HCI_EVT_CMD_COMPLETE;
        } break;

        default:
            break;
        }

    }
    // Status parameter --OGF(0x05)
    else if (p[2] == HCI_CMD_STATUS_PARAM_OPCODE_OGF) //OGF = 0x05 = 0b110000, 0x05 <<2 = 0x14
    {
#if (LL_FEATURE_ENABLE_POWER_CONTROL)
        switch (opcode) {
        case HCI_CMD_READ_RSSI: //TODO: needless ?
        {
            if (blmsParam.pwr_ctrl_en) {
                u8 returnPara[sizeof(hci_readRssi_retParam_t)];
                blc_hci_readRSSI((hci_readRssi_cmdParam_t *)cmdPara, (hci_readRssi_retParam_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_STATUS_PARAM_OPCODE_OGF, sizeof(hci_readRssi_retParam_t), returnPara, para);
            }
        } break;

        default:
            break;
        }
#endif

    }
    // LE command --OGF(0x08)
    else if (p[2] == HCI_CMD_LE_OPCODE_OGF) //OGF = 0x08 = 0b001000,  8 <<2 = 0x20
    {
        switch (opcode) {
            //core_4.0 begin

        //  01 set event mask
        case HCI_CMD_LE_SET_EVENT_MASK:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Evt_Mask", cmdPara, 8);
            blc_hci_le_setEventMask_cmd(cmdPara[0] | cmdPara[1] << 8 | cmdPara[2] << 16 | cmdPara[3] << 24);
            blc_hci_le_setEventMask_2_cmd(cmdPara[4] | cmdPara[5] << 8 | cmdPara[6] << 16 | cmdPara[7] << 24);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  02 read buffer size
        case HCI_CMD_LE_READ_BUF_SIZE: //ACL DATA
        {
            u8 returnPara[sizeof(hci_le_readBufSize_v1_retParam_t)];
            blc_hci_le_readBufferSize_cmd((hci_le_readBufSize_v1_retParam_t *)returnPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_readBufSize_v1_retParam_t), returnPara, para);
        } break;

        //  03 read local supported feature
        case HCI_CMD_LE_READ_LOCAL_SUPPORTED_FEATURES:
        {
            u8 returnPara[sizeof(hci_le_readLocSupFeature_retParam_t)];
            blc_hci_le_getLocalSupportedFeatures((hci_le_readLocSupFeature_retParam_t *)returnPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_readLocSupFeature_retParam_t), returnPara, para);
        } break;

        //  05 set random address
        case HCI_CMD_LE_SET_RANDOM_ADDR:
        {
            status    = (u8)blc_ll_setRandomAddr(cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  06 set advertising parameter
        case HCI_CMD_LE_SET_ADVERTISE_PARAMETERS:
        {
            status    = (u8)blc_hci_le_setAdvParam((hci_le_setAdvParam_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        }

        break;

        //  07 set advertising channel Tx power
        case HCI_CMD_LE_READ_ADVERTISING_CHANNEL_TX_POWER:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Advertising_Channel_Tx_Power", 0, 0);
            u8 returnPara[2];
            status = BLE_SUCCESS;

#if (ONLY_FOR_EBQ_TEST_LATER_REMOVE) //TODO EBQ_TEST_EN
            //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
            if (IS_EXTENDED_ADV_VALID) {
                status = HCI_ERR_CMD_DISALLOWED;
            }
            SET_LEGACY_ADV_VALID;
#endif

            returnPara[0] = status;
            returnPara[1] = 0; //TODO: rf_get_tx_power_level();
            eventCode     = HCI_EVT_CMD_COMPLETE;
            resultLen     = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 2, returnPara, para);
        } break;

        //  08 set advertising data
        case HCI_CMD_LE_SET_ADVERTISE_DATA:
        {
            status    = (u8)blc_hci_le_setAdvData(cmdPara + 1, cmdPara[0]);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  09 set scan response data
        case HCI_CMD_LE_SET_SCAN_RSP_DATA:
        {
            status    = (u8)blc_hci_le_setScanRspData(cmdPara + 1, cmdPara[0]);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  0a set advertise enable
        case HCI_CMD_LE_SET_ADVERTISE_ENABLE:
        {
            status    = (u8)blc_hci_le_setAdvEnable(cmdPara[0]);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

#if (LL_FEATURE_SUPPORT_LE_LEGACY_SCANNING)
        //  0b set scan parameters
        case HCI_CMD_LE_SET_SCAN_PARAMETERS:
        {
            status    = blc_hci_le_setScanParameter((hci_le_setScanParam_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  0c set scan enable
        case HCI_CMD_LE_SET_SCAN_ENABLE:
        {
            status    = blc_hci_le_setScanEnable((hci_le_setScanEnable_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;
#if (LL_FEATURE_ENABLE_LE_EXTENDED_INITIATE && LL_ACL_CEN_EN)
        //  0d create connection
        case HCI_CMD_LE_CREATE_CONNECTION:
        {
            status    = (u8)blc_hci_le_createConnection((hci_le_createConn_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);

        } break;

        //  0e create connection cancel
        case HCI_CMD_LE_CREATE_CONNECTION_CANCEL:
        {
            status    = blc_ll_createConnectionCancel();
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = 4;
            hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;
#endif
#endif /*!< LL_FEATURE_SUPPORT_LE_LEGACY_SCANNING */

        //  0f read white list size
        case HCI_CMD_LE_READ_WHITE_LIST_SIZE:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_White_List_Size", 0, 0);
            u8 returnPara[sizeof(hci_le_readWhiteListSizeCmd_retParam_t)];
            blc_ll_readWhiteListSize((hci_le_readWhiteListSizeCmd_retParam_t *)returnPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_readWhiteListSizeCmd_retParam_t), returnPara, para);
        } break;

        //  10 clear white list
        case HCI_CMD_LE_CLEAR_WHITE_LIST:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Clear_White_List", 0, 0);
            status    = (u8)blc_ll_clearWhiteList();
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  11 add device to white list
        case HCI_CMD_LE_ADD_DEVICE_TO_WHITE_LIST:
        {
            status    = blc_hci_le_addDeviceToAcceptList((hci_le_addDeviceAcceptlist_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  12 remove device from white list
        case HCI_CMD_LE_REMOVE_DEVICE_FROM_WL:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Remove_Device_From_WL", cmdPara, 7);
            status    = blc_ll_removeDeviceFromWhiteList(cmdPara[0], cmdPara + 1);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  13 connection update
        //  TODO: add "blm_ll_updateConnection" here
        case HCI_CMD_LE_CONNECTION_UPDATE:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Connection_Update", cmdPara, 14);
            u16 connHandle = (cmdPara[0] | cmdPara[1] << 8);
// Run #3467 - /Pre-Release/LL/CS/CEN/INI/BV-25-C   [LE Connection Update Errors During CS Procedure, Central, Initiator]
// Run #3468 - /Pre-Release/LL/CS/CEN/REF/BV-25-C   [LE Connection Update Errors During CS Procedure, Central, Reflector]
// connection update should not process when cs procedure enable(central role)
// don't send connection update indication and report hci complete evt with error code command disallowed
// this is temp solution, if merge to master, need sync with sihui/qinghua -- note by yuexin 2024/07/30
#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
            u8            conn_idx = connHandle & CONN_IDX_MASK;
            st_ll_conn_t *pc       = (st_ll_conn_t *)&blms[conn_idx];
            cs_config_t  *pCsCfg   = gCsMng.gGlobal_pCsCfg + pc->csParam.cs_config_pend_idx;
            CS_HCI_LOG("CHECK CONNECTION UPDATE----------------,procedure state,file,line: %d, %d, %s, %d", pCsCfg->cs_procedure_en, pCsCfg->cs_procedure_measurement_en, __FILENAME__, __LINE__);
            if (pCsCfg->cs_procedure_en || pCsCfg->cs_procedure_measurement_en) {
                csFlowCtrl.csConnUptErr = 1;
                status                  = BLE_SUCCESS;
            }
            if (!csFlowCtrl.csConnUptErr) {
                status = (u8)blc_ll_updateConnection(connHandle,
                                                     cmdPara[2] + cmdPara[3] * 256,   //conn min
                                                     cmdPara[4] + cmdPara[5] * 256,   //conn max
                                                     cmdPara[6] + cmdPara[7] * 256,   //conn latency
                                                     cmdPara[8] + cmdPara[9] * 256,   //timeout
                                                     cmdPara[10] + cmdPara[11] * 256, //ce min
                                                     cmdPara[12] + cmdPara[13] * 256  //ce max
                );
            } else {
                if (hci_le_eventMask & HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE) {
                    u8                                    buff[sizeof(hci_le_connectionUpdateCompleteEvt_t)];
                    hci_le_connectionUpdateCompleteEvt_t *pUpt = (hci_le_connectionUpdateCompleteEvt_t *)buff;
                    pUpt->subEventCode                         = HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE; // sub code
                    pUpt->status                               = HCI_ERR_CMD_DISALLOWED;
                    pUpt->connHandle                           = connHandle;                                // handle
                    pUpt->connInterval                         = pc->conn_intvl_n_1m25;                     //pkt_init.interval;             // interval
                    pUpt->connLatency                          = pc->conn_latency;                          //pkt_init.latency;                    // latency
                    pUpt->supervisionTimeout                   = pc->conn_timeout / (10000 * 16);           //pkt_init.timeout;
                    csFlowCtrl.csConnUptErr                    = 0;
                    blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, buff, 10);
                    CS_HCI_LOG("ERROR: Conn Upt during CS!!!");
                }
            }
#else
            status = (u8)blc_ll_updateConnection(connHandle,
                                                 cmdPara[2] + cmdPara[3] * 256,   //conn min
                                                 cmdPara[4] + cmdPara[5] * 256,   //conn max
                                                 cmdPara[6] + cmdPara[7] * 256,   //conn latency
                                                 cmdPara[8] + cmdPara[9] * 256,   //timeout
                                                 cmdPara[10] + cmdPara[11] * 256, //ce min
                                                 cmdPara[12] + cmdPara[13] * 256  //ce max
            );
#endif
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            *para32   = HCI_EVT_CMDSTATUS(1, opcode, HCI_CMD_LE_OPCODE_OGF, status);
        } break;

        //  14 set host channel classification
        case HCI_CMD_LE_SET_HOST_CHANNEL_CLASSIFICATION:
        {
            status    = (u8)blc_ll_setHostChannel(cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  15 read channel map
        case HCI_CMD_LE_READ_CHANNEL_MAP:
        {
            //u16 connHandle = cmdPara[0] | cmdPara[1]<<8;
            u8 returnPara[8];
            returnPara[1] = cmdPara[0];
            returnPara[2] = cmdPara[1];
            returnPara[0] = (u8)blc_hci_le_readChannelMap(cmdPara[0] | cmdPara[1] << 8, returnPara + 3);
            eventCode     = HCI_EVT_CMD_COMPLETE;
            resultLen     = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 8, returnPara, para);
        } break;

        //  16 read remote used feature
        case HCI_CMD_LE_READ_REMOTE_USED_FEATURES:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Remote_Used_Features_Page_0", 0, 0);
            status    = (u8)blc_hci_le_getRemoteSupportedFeatures(cmdPara[1] << 8 | cmdPara[0]);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
        } break;


        //  17 encrypt
        case HCI_CMD_LE_ENCRYPT:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Encrypt", 0, 0);
            ///// return param: param_total_len : 17 byte
            /////               param0: status           len : 1 byte
            /////               param1: encryptedTextData len : 16 byte
            ///// event code : HCI_EVT_CMD_COMPLETE
            u8 *hciCmdParam = (u8 *)cmdPara;

            u8 *key           = (u8 *)hciCmdParam;
            u8 *plaintextData = (u8 *)(hciCmdParam + 16);

            u8 returnPara[17] = {0};

            //pointer encryptedTextData must be 4 byte aligned, or there will be ERR
            u8 encryptedTextData[16];
            status = blc_ll_encryptedData(key, plaintextData, (u8 *)encryptedTextData);

            returnPara[0] = status;
            smemcpy(returnPara + 1, encryptedTextData, 16);

            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 17, returnPara, para);
        } break;

        //  18 random
        case HCI_CMD_LE_RANDOM:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Random", 0, 0);
            ///// return param: param_total_len : 9 byte
            /////               param0: status           len : 1 byte
            /////               param1: random_number    len : 8 byte
            ///// event code : HCI_EVT_CMD_COMPLETE
            u8  returnPara[9] = {0};
            u8 *randomNumber  = returnPara + 1;

            status        = blc_ll_genRandomNumber(randomNumber, 8);
            returnPara[0] = status;

            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 9, returnPara, para);
        } break;

        //  19 start encryption
        case HCI_CMD_LE_START_ENCRYPTION:
        {
            status    = blc_hci_le_enableEncryption((hci_le_enableEncryption_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
        } break;

        //  1a long term key request reply
        case HCI_CMD_LE_LONG_TERM_KEY_REQUESTED_REPLY:
        {
            ///// return param: param_total_len : 3 byte
            /////               param0: status            len : 1 byte
            /////               param1: connection handle len : 2 byte
            ///// event code : HCI_EVT_CMD_COMPLETE
            u16 connectHandle = cmdPara[0] | cmdPara[1] << 8;
            u8 *specifiesLtk  = (cmdPara + 2);

            u8 returnPara[3] = {0};

            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_LTK_Request_Reply", specifiesLtk, 16);
            status        = blt_hci_ltkRequestReply(connectHandle, specifiesLtk);
            returnPara[0] = status;
            returnPara[1] = connectHandle;
            returnPara[2] = connectHandle >> 8;

            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 3, returnPara, para);
        } break;

        //  1b long term key request negative
        case HCI_CMD_LE_LONG_TERM_KEY_REQUESTED_NEGATIVE_REPLY:
        {
            ///// return param: param_total_len : 3 byte
            /////               param0: status            len : 1 byte
            /////               param1: connection handle len : 2 byte
            ///// event code : HCI_EVT_CMD_COMPLETE
            u16 connectHandle = cmdPara[0] | cmdPara[1] << 8;

            u8 returnPara[3] = {0};

            //                  status = blt_getLtkVsConnHandleFail(connectHandle);
            status        = blt_hci_ltkRequestNegativeReply(connectHandle);
            returnPara[0] = status;
            returnPara[1] = connectHandle;
            returnPara[2] = connectHandle >> 8;

            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 3, returnPara, para);

        } break;
        //  1c read supported states
        case HCI_CMD_LE_READ_SUPPORTED_STATES:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Supported_States", 0, 0);
            const u8 le_states[12] = {0, 0xff, 0x66, 0x2A, 0xFF, 0x6D, 0x03, 0x00, 0x00}; //slave  TODO: merge slave/master state together
            resultLen              = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 9, (u8 *)(u32)le_states, para);
            eventCode              = HCI_EVT_CMD_COMPLETE;
        } break;

    #if (LL_FEATURE_SUPPORT_PHY_TEST_MODE)
        //  1d receiver test
        case HCI_CMD_LE_RECEIVER_TEST:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Receiver_Test", 0, 0);
            status    = blt_phyTest_hci_setReceiverTest_V1((hci_le_receiverTestV1_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;
        //  1e transmitter test
        case HCI_CMD_LE_TRANSMITTER_TEST:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Transmitter_Test", 0, 0);
            status    = blt_phyTest_hci_setTransmitterTest_V1((hci_le_transmitterTestV1_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  1f test end
        case HCI_CMD_LE_TEST_END:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Test_End", 0, 0);
            u8 returnPara[3] = {0};
            returnPara[0]    = blt_phyTest_setTestEnd(returnPara + 1);
            eventCode        = HCI_EVT_CMD_COMPLETE;
            resultLen        = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 3, returnPara, para);
        } break;
    #endif /*!< LL_FEATURE_SUPPORT_PHY_TEST_MODE */

            //core_4.0 end

            //core_4.1 begin

        //  20 remote connection parameter request reply        (4.1)
        case HCI_CMD_LE_REMOTE_CONNECTION_PARAM_REQ_REPLY:
        {
        } break;
        //  21 remote connection parameter request negative reply
        case HCI_CMD_LE_REMOTE_CONNECTION_PARAM_REQ_NEGATIVE_REPLY:
        {
        } break;

        //core_4.1 end

        //core_4.2 begin
#if LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION
        //  21 set data length                                  (4.2)
        case HCI_CMD_LE_SET_DATA_LENGTH:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Data_Length", cmdPara, 6);
            u8 returnPara[12] = {0};
            returnPara[0]     = blc_hci_setTxDataLength(cmdPara[0] + cmdPara[1] * 256, cmdPara[2] + cmdPara[3] * 256, cmdPara[4] + cmdPara[5] * 256);
            returnPara[1]     = cmdPara[0];
            returnPara[2]     = cmdPara[1];
            eventCode         = HCI_EVT_CMD_COMPLETE;
            resultLen         = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 3, returnPara, para);
        } break;
        //  23 read suggested default data length
        case HCI_CMD_LE_READ_SUGGESTED_DEFAULT_DATA_LENGTH:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Suggested_Default_Data_Length", 0, 0);
            u8 returnPara[8] = {0};
            //                  returnPara[0] = 0;
            //                  returnPara[1] = blt_txfifo.size - 13;
            //                  returnPara[2] = 0;
            //                  u16 t = LL_PACKET_OCTET_TIME (returnPara[1]);
            //                  returnPara[3] = t;
            //                  returnPara[4] = t >> 8;

            returnPara[0] = blc_hci_readSuggestedDefaultTxDataLength(returnPara + 1, returnPara + 3);

            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 5, returnPara, para);
        } break;
        //  24 write suggested default data length
        case HCI_CMD_LE_WRITE_SUGGESTED_DEFAULT_DATA_LENGTH:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Write_Suggested_Default_Data_Length", cmdPara, 4);
            status = blc_hci_writeSuggestedDefaultTxDataLength(cmdPara[0] + cmdPara[1] * 256, cmdPara[2] + cmdPara[3] * 256);

            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;
#endif
#if (CONTROLLER_GEN_P256KEY_ENABLE)
        //  25 read local P-256 public key
        case HCI_CMD_LE_READ_LOCAL_P256_PUBLIC_KEY:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Local_P256_Public_Key", 0, 0);
            status = blt_ll_getP256pubKey();

            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
        } break;

            //  26 generate DHKey

        case HCI_CMD_LE_GENERATE_DHKEY:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Generate_Dhkey", 0, 0);
            ///// return param: structure (hci_le_generateDHKeyCompleteEvt_t)
            ///// event code : HCI_EVT_LE_META
            u8 *hciCmdParam   = (u8 *)cmdPara;
            u8 *remoteP256Key = (u8 *)hciCmdParam;

            //API_version1: False, API_version2: TRUE
            status = blt_ll_generateDHkey(remoteP256Key, FALSE);

            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
        } break;
#endif

#if (LL_FEATURE_ENABLE_PRIVACY)
        //  27 add device to resolving list
        case HCI_CMD_LE_ADD_DEVICE_TO_RESOLVING_LIST:
        {
            status    = blc_hci_le_addDeviceToResolvingList((hci_le_addDeviceResolvinglist_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  28 remove device from resolving list
        case HCI_CMD_LE_REMOVE_DEVICE_FROM_RESOLVING_LIST:
        {
            status    = blc_hci_le_removeDeviceFromResolvingList((le_identityAddress_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  29 clear resolving list
        case HCI_CMD_LE_CLEAR_RESOLVING_LIST:
        {
            status    = blc_ll_clearResolvingList();
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  2a read resolving list size
        case HCI_CMD_LE_READ_RESOLVING_LIST_SIZE:
        {
            u8 returnPara[sizeof(hci_le_readResolvingListSizeCmd_retParam_t)] = {0};
            blc_hci_le_readResolvingListSize((hci_le_readResolvingListSizeCmd_retParam_t *)returnPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_readResolvingListSizeCmd_retParam_t), returnPara, para);
        } break;

        //  2b read peer resolvable address
        case HCI_CMD_LE_READ_PEER_RESOLVABLE_ADDRESS:
        {
            u8 returnPara[sizeof(hci_le_readPeerResolvableAddress_retParam_t)] = {0};
            blc_hci_le_readPeerResolvableAddress((le_identityAddress_t *)cmdPara, (hci_le_readPeerResolvableAddress_retParam_t *)returnPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_readPeerResolvableAddress_retParam_t), returnPara, para);
        } break;

        //  2c read local resolvable address
        case HCI_CMD_LE_READ_LOCAL_RESOLVABLE_ADDRESS:
        {
            u8 returnPara[sizeof(hci_le_readLocalResolvableAddress_retParam_t)] = {0};
            blc_hci_le_readLocalResolvableAddress((le_identityAddress_t *)cmdPara, (hci_le_readLocalResolvableAddress_retParam_t *)returnPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_readLocalResolvableAddress_retParam_t), returnPara, para);
        } break;

        //  2d set address resolution enable
        case HCI_CMD_LE_SET_ADDRESS_RESOLUTION_ENABLE:
        {
            status    = blc_ll_setAddressResolutionEnable(cmdPara[0]);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //  2e set resolvable private address timeout
        case HCI_CMD_LE_SET_RESOLVABLE_PRIVATE_ADDRESS_TIMEOUT:
        {
            status    = blc_ll_setResolvablePrivateAddressTimeout(cmdPara[0] | cmdPara[1] << 8);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;
#endif
#if LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION
        //  2f read maximum data length
        case HCI_CMD_LE_READ_MAX_DATA_LENGTH:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Max_Data_Lengtht", 0, 0);
            u8 returnPara[12] = {0};
            blc_hci_readMaximumDataLength((hci_le_readMaxDataLengthCmd_retParam_t *)returnPara);

            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 9, returnPara, para);
        } break;
#endif
        //core_4.2 end

        //core_5.0 begin

#if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY | LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT)
        case HCI_CMD_LE_READ_PHY:
        {
            if (blmsParam.phy_2mCoded_en || blmsParam.phy_hdt_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_PHY", 0, 0);
                u8  returnPara[6];
                u16 connHandle = (cmdPara[0] | cmdPara[1] << 8);
                blc_ll_readPhy(connHandle, (hci_le_readPhyCmd_retParam_t *)returnPara); //note that status is included in "returnPara"

                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 5, returnPara, para);
            }
        } break;

        case HCI_CMD_LE_SET_DEFAULT_PHY:
        {
            if (blmsParam.phy_2mCoded_en || blmsParam.phy_hdt_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Default_PHY", 0, 0);
                status    = (u8)blc_ll_setDefaultPhy(cmdPara[0], cmdPara[1], cmdPara[2]);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        case HCI_CMD_LE_SET_PHY:
        {
            if (blmsParam.phy_2mCoded_en || blmsParam.phy_hdt_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_PHY", 0, 0);
                status    = (u8)blc_hci_le_setPhy((hci_le_setPhyCmd_param_t *)cmdPara);
                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
            }
        } break;
#endif

    #if (LL_FEATURE_SUPPORT_PHY_TEST_MODE)
        case HCI_CMD_LE_ENHANCED_RECEIVER_TEST:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Enhanced_Receiver_Test", 0, 0);
            status    = blt_phyTest_hci_setReceiverTest_V2((hci_le_receiverTestV2_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;


        case HCI_CMD_LE_ENHANCED_TRANSMITTER_TEST:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Enhanced_Transmitter_Test", 0, 0);
            status    = blt_phyTest_hci_setTransmitterTest_V2((hci_le_transmitterTestV2_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;
    #endif /*!< LL_FEATURE_SUPPORT_PHY_TEST_MODE */

#if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)
        case HCI_CMD_LE_SET_ADVERTISING_SET_RANDOM_ADDRESS:
        {
            if (blmsParam.extAdvModule_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Advertising_Set_Random_Address", cmdPara, 7);
                status    = (u8)blc_ll_setAdvRandomAddr(cmdPara[0], cmdPara + 1);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;


        case HCI_CMD_LE_SET_EXTENDED_ADVERTISING_PARAMETERS:
        {
            if (blmsParam.extAdvModule_en) {
                u8 returnPara[4];
                returnPara[0] = (u8)blc_hci_le_setExtAdvParam((hci_le_setExtAdvParam_cmdParam_t *)cmdPara, returnPara + 1);
                eventCode     = HCI_EVT_CMD_COMPLETE;
                resultLen     = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 2, returnPara, para);
            }
        } break;


        case HCI_CMD_LE_SET_EXTENDED_ADVERTISING_DATA:
        {
            if (blmsParam.extAdvModule_en) {
                status = (u8)blc_hci_le_setExtendedAdvData((hci_le_setExtAdvData_cmdParam_t *)cmdPara);

                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        case HCI_CMD_LE_SET_EXTENDED_SCAN_RESPONSE_DATA:
        {
            if (blmsParam.extAdvModule_en) {
                status = (u8)blc_hci_le_setExtendedScanResponseData((hci_le_setExtScanRspData_cmdParam_t *)cmdPara);

                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        case HCI_CMD_LE_SET_EXTENDED_ADVERTISING_ENABLE:
        {
            if (blmsParam.extAdvModule_en) {
                status    = (u8)blc_hci_le_setExtAdvEnable((hci_le_setExtAdvEn_cmdParam_t *)cmdPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;


        case HCI_CMD_LE_READ_MAXIMUM_ADVERTISING_DATA_LENGTH:
        {
            if (blmsParam.extAdvModule_en) {
                u8 returnPara[4];

                returnPara[0] = blc_hci_le_readMaxAdvDataLength(returnPara + 1);

                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 3, returnPara, para);
            }
        } break;

        case HCI_CMD_LE_READ_NUMBER_OF_SUPPORTED_ADVERTISING_SETS:
        {
            if (blmsParam.extAdvModule_en) {
                u8 returnPara[4];
                u8 num_adv_set;
                num_adv_set   = 0;
                status        = blc_hci_le_readNumberOfSupportedAdvSets(&num_adv_set);
                returnPara[0] = status;
                returnPara[1] = num_adv_set;
                eventCode     = HCI_EVT_CMD_COMPLETE;
                resultLen     = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 2, returnPara, para);
            }
        } break;


        case HCI_CMD_LE_REMOVE_ADVERTISING_SET:
        {
            if (blmsParam.extAdvModule_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Remove_Advertising_Set", 0, 0);
                status    = blc_ll_removeAdvSet(cmdPara[0]);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        case HCI_CMD_LE_CLEAR_ADVERTISING_SETS:
        {
            if (blmsParam.extAdvModule_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Clear_Advertising_Sets", 0, 0);
                status    = blc_ll_clearAdvSets();
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;
#endif


#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING)
        ///////////////// Periodic Advertising ///////////////////////////////////////////
        case HCI_CMD_LE_SET_PERIODIC_ADVERTISING_PARAMETERS:
        {
            if (blmsParam.prdAdvModule_en) {
                status    = (u8)blc_hci_le_setPeriodicAdvParam((hci_le_setPeriodicAdvParam_cmdParam_t *)cmdPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        case HCI_CMD_LE_SET_PERIODIC_ADVERTISING_DATA:
        {
            if (blmsParam.prdAdvModule_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Periodic_Advertising_Data", cmdPara, 4);
                status    = (u8)blc_hci_le_setPeriodicAdvData(cmdPara[0], cmdPara[1], cmdPara[2], cmdPara + 3);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        case HCI_CMD_LE_SET_PERIODIC_ADVERTISING_ENABLE:
        {
            if (blmsParam.prdAdvModule_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Periodic_Advertising_Enable", cmdPara, 2);
                status    = (u8)blc_ll_setPeriodicAdvEnable(cmdPara[0], cmdPara[1]);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;
        /////////////////////////////////////////////////////////////////////////////////////
#endif


#if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
        ///////////////// Periodic Advertising with Response [Advertiser] ///////////////////
        //7.8.61 LE Set Periodic Advertising Parameters Command [v2]
        case HCI_CMD_LE_SET_PERIODIC_ADVERTISING_PARAMETERS_V2:
        {
            if (blmsParam.prdAdvWr_en) {
                hci_le_setPeriodicAdvParamV2_retParam_t retParamV2 = blc_hci_le_setPeriodicAdvParam_v2((hci_le_setPeriodicAdvParamV2_cmdParam_t *)cmdPara);
                eventCode                                          = HCI_EVT_CMD_COMPLETE;
                resultLen                                          = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_setPeriodicAdvParamV2_retParam_t), &retParamV2, para);
            }
        } break;
        //7.8.125 LE Set Periodic Advertising Subevent Data command
        case HCI_CMD_LE_SET_PERIODIC_ADVERTISING_SUBEVENT_DATA:
        {
            if (blmsParam.prdAdvWr_en) {
                u8 returnPara[sizeof(hci_le_setPeridAdvSubeventDataRetParams_t)];

                status    = (u8)blc_hci_le_setPeriodicAdvSubeventData((hci_le_setPeridAdvSubeventData_cmdParam_t *)cmdPara, (hci_le_setPeridAdvSubeventDataRetParams_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_setPeridAdvSubeventDataRetParams_t), returnPara, para);
            }
        } break;
        /////////////////////////////////////////////////////////////////////////////////////
#endif


#if (LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)
        ///////////////// Extended Scanning ///////////////////////////////////////////
        //  41 set ext scan parameters
        case HCI_CMD_LE_SET_EXTENDED_SCAN_PARAMETERS:
        {
            if (blmsParam.extScanModule_en) {
                status    = (u8)blc_hci_le_setExtScanParam((hci_le_setExtScanParam_cmdParam_t *)cmdPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        //  42 set ext scan enable
        case HCI_CMD_LE_SET_EXTENDED_SCAN_ENABLE:
        {
            if (blmsParam.extScanModule_en) {
                status    = (u8)blc_hci_le_setExtScanEnable((hci_le_setExtScanEnable_cmdParam_t *)cmdPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;
        /////////////////////////////////////////////////////////////////////////////////////
#endif
#if LL_FEATURE_ENABLE_LE_EXTENDED_INITIATE  & LL_ACL_CEN_EN
        // 7.8.66 LE Extended Create Connection Command
        case HCI_CMD_LE_EXTENDED_CREATE_CONNECTION:
        {
            if (blmsParam.extInitModule_en) {
                hci_le_ext_createConn_cmdParam_t *pCmdParm = (hci_le_ext_createConn_cmdParam_t *)cmdPara;
                status                                     = (u8)blc_hci_le_extended_createConnection(pCmdParm);
                eventCode                                  = HCI_EVT_CMD_STATUS;
                resultLen                                  = 4;
                hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
            }
        } break;
    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
        ///////////////// Periodic Advertising with Response [Advertiser] ///////////////////
        // 7.8.66 LE Extended Create Connection Command [v2]
        case HCI_CMD_LE_EXTENDED_CREATE_CONNECTION_V2:
        {
            if (blmsParam.prdAdvWr_en) {
                hci_le_ext_createConnV2_cmdParam_t *pCmdParm = (hci_le_ext_createConnV2_cmdParam_t *)cmdPara;
                status                                       = (u8)blc_hci_le_extended_createConnection_v2(pCmdParm);
                eventCode                                    = HCI_EVT_CMD_STATUS;
                resultLen                                    = 4;
                hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
            }
        } break;
            /////////////////////////////////////////////////////////////////////////////////////
    #endif
#endif

#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
        // 7.8.67 LE Periodic Advertising Create Sync command
        case HCI_CMD_LE_PERIODIC_ADVERTISING_CREATE_SYNC:
        {
            if (blmsParam.pda_sync_en) {
                hci_le_periodicAdvCreateSync_cmdParam_t *pCmdParm = (hci_le_periodicAdvCreateSync_cmdParam_t *)cmdPara;

                status    = blc_hci_le_periodic_advertising_create_sync(pCmdParm);
                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
            }
        } break;


        //7.8.68 LE Periodic Advertising Create Sync Cancel command
        case HCI_CMD_LE_PERIODIC_ADVERTISING_CREATE_SYNC_CANCEL:
        {
            if (blmsParam.pda_sync_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Periodic_Adv_Create_Sync_Cancel", 0, 0);
                status    = (u8)blc_ll_periodicAdvertisingCreateSyncCancel();
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        //7.8.69 LE Periodic Advertising Terminate Sync command
        case HCI_CMD_LE_PERIODIC_ADVERTISING_TERMINATE_SYNC:
        {
            if (blmsParam.pda_sync_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Periodic_Adv_Terminate_Sync", 0, 0);
                status    = (u8)blc_ll_periodicAdvertisingTerminateSync(cmdPara[0] | cmdPara[1] << 8);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        //7.8.70 LE Add Device To Periodic Advertiser List command
        case HCI_CMD_LE_ADD_DEVICE_TO_PERIODIC_ADVERTISER_LIST:
        {
            if (blmsParam.pda_sync_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Add_Device_To_Periodic_Adv_List", 0, 0);
                status    = (u8)blc_ll_addDeviceToPeriodicAdvertiserList(cmdPara[0], cmdPara + 1, cmdPara[7]);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        //7.8.71 LE Remove Device From Periodic Advertiser List command
        case HCI_CMD_LE_REMOVE_DEVICE_FROM_PERIODIC_ADVERTISER_LIST:
        {
            if (blmsParam.pda_sync_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Remove_Device_From_Periodic_Adv_List", 0, 0);
                status    = (u8)blc_ll_removeDeviceFromPeriodicAdvertiserList(cmdPara[0], cmdPara + 1, cmdPara[7]);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        //7.8.72 LE Clear Periodic Advertiser List command
        case HCI_CMD_LE_CLEAR_PERIODIC_ADVERTISER_LIST:
        {
            if (blmsParam.pda_sync_en) {
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Clear_Periodic_Adve_List", 0, 0);
                status    = (u8)blc_ll_clearPeriodicAdvertiserList();
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        //7.8.73 LE Read Periodic Advertiser List Size command
        case HCI_CMD_LE_READ_PERIODIC_ADVERTISER_LIST_SIZE:
        {
            if (blmsParam.pda_sync_en) {
                u8 returnPara[2];
                my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Periodic_Adv_List_Size", 0, 0);
                returnPara[0] = (u8)blc_ll_readPeriodicAdvertiserListSize(&returnPara[1]);
                eventCode     = HCI_EVT_CMD_COMPLETE;
                resultLen     = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 2, returnPara, para);
            }
        } break;
#endif


#if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
        ///////////////// Periodic Advertising with Response [Scanner] //////////////////////
        case HCI_CMD_LE_SET_PERIODIC_ADVERTISING_RESPONSE_DATA:
        {
            if (blmsParam.prdSyncWr_en) {
                u8                                    returnPara[4];
                hci_le_setPeridAdvRspData_cmdParam_t *pCmdPara = (hci_le_setPeridAdvRspData_cmdParam_t *)cmdPara;

                status = (u8)blc_hci_le_setPAwRsync_rspData(pCmdPara->sync_handle, pCmdPara->req_event_count, pCmdPara->req_subevt_idx, pCmdPara->rsp_subevt_idx, pCmdPara->rsp_slot_idx, pCmdPara->rsp_data_len, pCmdPara->rsp_data);

                returnPara[0] = status;
                returnPara[1] = pCmdPara->sync_handle;
                returnPara[2] = pCmdPara->sync_handle >> 8;

                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 3, returnPara, para);
            }
        } break;

        case HCI_CMD_LE_SET_PERIODIC_SYNC_SUBEVENT:
        {
            if (blmsParam.prdSyncWr_en) {
                u8 returnPara[4];

                hci_le_setPeriodicSyncSubevent_cmdParam_t *pCmdPara = (hci_le_setPeriodicSyncSubevent_cmdParam_t *)cmdPara;
                status                                              = (u8)blc_hci_le_setPeriodicSyncSubevent(pCmdPara->sync_handle, pCmdPara->pda_prop, pCmdPara->num_subevent, pCmdPara->subeventIdx);

                returnPara[0] = status;
                returnPara[1] = pCmdPara->sync_handle;
                returnPara[2] = pCmdPara->sync_handle >> 8;

                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 3, returnPara, para);
            }
        } break;
        /////////////////////////////////////////////////////////////////////////////////////
#endif


        //7.8.74 LE Read Transmit Power Command
        case HCI_CMD_LE_READ_TRANSMIT_POWER:
        {
            u8 returnPara[sizeof(hci_le_rdSuppTxPwrRetParams_t)];
            blc_hci_le_readSuppTxPower((hci_le_rdSuppTxPwrRetParams_t *)returnPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_rdSuppTxPwrRetParams_t), returnPara, para);
        } break;

        //7.8.75 LE Read RF Path Compensation Command
        case HCI_CMD_LE_READ_RF_PATH_COMPENSATION:
        {
            u8 returnPara[sizeof(hci_le_rdRfPathCompRetParams_t)];
            blc_hci_le_readRfPathComp((hci_le_rdRfPathCompRetParams_t *)returnPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_rdRfPathCompRetParams_t), returnPara, para);
        } break;

        //7.8.76 LE Write RF Path Compensation Command
        case HCI_CMD_LE_WRITE_RF_PATH_COMPENSATION:
        {
            status    = blc_hci_le_writeRfPathComp((hci_le_writeRfPathCompCmdParams_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

#if (LL_FEATURE_ENABLE_PRIVACY)
        //7.8.77 LE Set Privacy Mode Command
        case HCI_CMD_LE_SET_PRIVACY_MODE:
        {
            status    = blc_hci_le_setPrivacyMode((hci_le_setPrivacyMode_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;
#endif

            //core_5.0 end

            //core_5.1 begin
    #if (LL_FEATURE_SUPPORT_PHY_TEST_MODE)
            //7.8.78 LE Receiver Test command [v3]
        case HCI_CMD_LE_RECEIVER_TEST_V3:
        {
#if LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Receiver_Test_V3", 0, 0);
            status    = blt_phyTest_hci_setReceiverTest_V3((hci_le_receiverTestV3_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
#endif
        } break;

        //7.8.79 LE Transmitter Test command [v3]
        case HCI_CMD_LE_TRANSMITTER_TEST_V3:
        {
        } break;
#if (LL_FEATURE_ENABLE_LE_AOA_AOD)
        //7.8.80 LE Set Connectionless CTE Transmit Parameters command
        case HCI_CMD_LE_SET_CONNECTIONLESS_CTE_TRANSMIT_PARAMETERS:
        {
            if (blmsParam.cte_connLess_en) {
                hci_le_setConnectionless_CTETransmitParam_t *param = (hci_le_setConnectionless_CTETransmitParam_t *)cmdPara;
                status                                             = blc_hci_le_setConnectionless_CTETransmitParams(param);

                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        //7.8.81 LE Set Connectionless CTE Transmit Enable command
        case HCI_CMD_LE_SET_CONNECTIONLESS_CTE_TRANSMIT_ENABLE:
        {
            if (blmsParam.cte_connLess_en) {
                hci_le_CTE_enable_type *param = (hci_le_CTE_enable_type *)cmdPara;
                status                        = blc_hci_le_setConnectionless_CTETransmit_Enable(param);

                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        //7.8.82 LE Set Connectionless IQ Sampling Enable command
        case HCI_CMD_LE_SET_CONNECTIONLESS_IQ_SAMPLING_ENABLE:
        {
            if (blmsParam.cte_connLess_en) {
                u8                                     returnPara[3];
                hci_le_setConnectionless_IQsampleEn_t *param = (hci_le_setConnectionless_IQsampleEn_t *)cmdPara;
                returnPara[0]                                = blc_hci_le_setConnectionless_IQsample_Enable(param);
                returnPara[1]                                = param->Sync_Handle;
                returnPara[2]                                = param->Sync_Handle >> 8;

                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 3, returnPara, para);
            }
        } break;

        //7.8.83 LE Set Connection CTE Receive Parameters command
        case HCI_CMD_LE_SET_CONNECTION_CTE_RECEIVE_PARAMETERS:
        {
    #if (LL_UNREQUESTED_CONSTANT_TONE_EXTENSION_RECEIVING_ENABLE)
            u8                                   returnPara[3];
            hci_le_setConnection_CTERevParams_t *param = (hci_le_setConnection_CTERevParams_t *)cmdPara;
            returnPara[0]                              = blc_hci_le_setConnection_CTEReceiveParams(param);
            returnPara[1]                              = param->conn_handle;
            returnPara[2]                              = param->conn_handle >> 8;

            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 2, returnPara, para);
    #endif
        } break;

        //7.8.84 LE Set Connection CTE Transmit Parameters command
        case HCI_CMD_LE_SET_CONNECTION_CTE_TRANSMIT_PARAMETERS:
        {
    #if (LL_FEATURE_ENABLE_CONNECTION_CTE_RESPONSE)
            u8                                        returnPara[3];
            hci_le_setConnection_CTETransmitParams_t *param = (hci_le_setConnection_CTETransmitParams_t *)cmdPara;
            returnPara[0]                                   = blc_hci_le_setConnection_CTETransmitParams(param);
            returnPara[1]                                   = param->conn_handle;
            returnPara[2]                                   = param->conn_handle >> 8;

            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 2, returnPara, para);
    #endif
        } break;

        //7.8.85 LE Connection CTE Request Enable command
        case HCI_CMD_LE_CONNECTION_REQUEST_ENABLE:
        {
    #if (LL_FEATURE_ENABLE_CONNECTION_CTE_REQUEST)
            u8                 returnPara[3];
            hci_le_cteReqEn_t *param = (hci_le_cteReqEn_t *)cmdPara;
            returnPara[0]            = blc_hci_le_connection_CTEReq_Enable(param);
            returnPara[1]            = param->conn_handle;
            returnPara[2]            = param->conn_handle >> 8;

            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 2, returnPara, para);
    #endif
        } break;

        //7.8.86 LE Connection CTE Response Enable command
        case HCI_CMD_LE_CONNECTION_RESPONSE_ENABLE:
        {
    #if (LL_FEATURE_ENABLE_CONNECTION_CTE_RESPONSE)
            u8                 returnPara[3];
            hci_le_cteRspEn_t *param = (hci_le_cteRspEn_t *)cmdPara;
            returnPara[0]            = blc_hci_le_connection_CTERsp_Enable(param);
            returnPara[1]            = param->conn_handle;
            returnPara[2]            = param->conn_handle >> 8;

            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 2, returnPara, para);
    #endif
        } break;

        //7.8.87 LE Read Antenna Information command
        case HCI_CMD_LE_READ_ANTENNA_INFORMATION:
        {
            if (blmsParam.cte_connLess_en) {
                u8 returnPara[5];
                returnPara[0] = blc_hci_le_ReadAntennaInfor(&returnPara[1]);
                eventCode     = HCI_EVT_CMD_COMPLETE;
                resultLen     = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 5, returnPara, para);
            }
        } break;
    #endif
#endif

#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
        //7.8.88 LE Set Periodic Advertising Receive Enable command
        case HCI_CMD_LE_SET_PERIODIC_ADVERTISING_RECEIVE_ENABLE:
        {
            if (blmsParam.pda_sync_en) {
                status    = blc_hci_le_periodicAdvertisingReceiveEn((hci_le_setPeriodicAdvReceiveEnCmdParams_t *)cmdPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;
#endif

#if (LL_FEATURE_ENABLE_PAST)
        //7.8.89 LE Periodic Advertising Sync Transfer command
        case HCI_CMD_LE_PERIODIC_ADVERTISING_SYNC_TRANSFER:
        {
            if (blmsParam.past_en) {
                u8 returnPara[sizeof(hci_le_pastRetParams_t)];
                blc_hci_le_periodicAdvSyncTransfer((hci_le_pastCmdParams_t *)cmdPara, (hci_le_pastRetParams_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_pastRetParams_t), returnPara, para);
            }
        } break;

        //7.8.90 LE Periodic Advertising Set Info Transfer command
        case HCI_CMD_LE_PERIODIC_ADVERTISING_SET_INFO_TRANSFER:
        {
            if (blmsParam.past_en) {
                u8 returnPara[sizeof(hci_le_paSetInfoTransferRetParams_t)];
                blc_hci_le_periodicAdvSetInfoTransfer((hci_le_paSetInfoTransferCmdParams_t *)cmdPara, (hci_le_paSetInfoTransferRetParams_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_paSetInfoTransferRetParams_t), returnPara, para);
            }
        } break;

        //7.8.91 LE Set Periodic Advertising Sync Transfer Parameters command
        case HCI_CMD_LE_SET_PERIODIC_ADV_SYNC_TRANSFER_PARAMETERS:
        {
            if (blmsParam.past_en) {
                u8 returnPara[sizeof(hci_le_pastParamsRetParams_t)];
                blc_hci_le_setPeriodicAdvSyncTransferParams((hci_le_pastParamsCmdParams_t *)cmdPara, (hci_le_pastParamsRetParams_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_pastParamsRetParams_t), returnPara, para);
            }
        } break;

        //7.8.92 LE Set Default Periodic Advertising Sync Transfer Parameters command
        case HCI_CMD_LE_SET_DEFAULT_PERIODIC_ADV_SYNC_TRANSFER_PARAMS:
        {
            if (blmsParam.past_en) {
                status    = blc_hci_le_setDftPeriodicAdvSyncTransferParams((hci_le_dftPastParamsCmdParams_t *)cmdPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;
#endif

    #if (CONTROLLER_GEN_P256KEY_ENABLE)
        //7.8.93 LE Generate DHKey command [v2]
        case HCI_CMD_LE_GENERATE_DHKEY_V2:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_enerate_Dhkey_V2", 0, 0);
            ///// return param: structure (hci_le_generateDHKeyCompleteEvt_t)
            ///// event code : HCI_EVT_LE_META
            u8* hciCmdParam = (u8*)cmdPara;
            u8* remoteP256Key = (u8*) hciCmdParam;
            bool key_type = hciCmdParam[32] ? TRUE : FALSE;

            status = blt_ll_generateDHkey (remoteP256Key, key_type);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
        } break;
    #endif /*!< CONTROLLER_GEN_P256KEY_ENABLE */

        //7.8.94 LE Modify Sleep Clock Accuracy command
        case HCI_CMD_LE_MODIFY_SLEEP_CLOCK_ACCURACY:
        {
        } break;

//core_5.1 end

//core_5.2 begin
#if (LL_FEATURE_ENABLE_ISO)
        //7.8.2 LE Read Buffer Size command
        case HCI_CMD_LE_READ_BUFFER_SIZE_V2:
        {
            if (blmsParam.iso_en) {
                u8 returnPara[sizeof(hci_le_readBufSize_v2_retParam_t)];
                blc_hci_le_readBufferSize_v2_cmd((hci_le_readBufSize_v2_retParam_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_readBufSize_v2_retParam_t), returnPara, para);
            }
        } break;


        case HCI_CMD_LE_READ_ISO_TX_SYNC:
        {
            if (blmsParam.iso_en) {
                u8 returnPara[sizeof(hci_le_readIsoTxSync_retParam_t)];
                blc_hci_le_read_iso_tx_sync(cmdPara[0] | cmdPara[1] << 8, (hci_le_readIsoTxSync_retParam_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_readIsoTxSync_retParam_t), returnPara, para);
            }
        } break;
#endif


#if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER)
        case HCI_CMD_LE_SET_CIG_PARAMETERS:
        {
            if (blmsParam.cis_cen_en) {
                u8 returnPara[3 + (CIS_IN_CIGM_NUM_MAX * 2)]; //3 + CIS_CounT_max*2
                blc_hci_le_setCigParams((hci_le_setCigParam_cmdParam_t *)cmdPara, (hci_le_setCigParam_retParam_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 3 + returnPara[2] * 2, returnPara, para);
            }
        } break;

        case HCI_CMD_LE_SET_CIG_PARAMETERS_TEST:
        {
            if (blmsParam.cis_cen_en) {
                u8 returnPara[3 + (CIS_IN_CIGM_NUM_MAX << 1)]; //3 + CIS_CounT_max*2
                returnPara[2] = 1;                             //CIS count set 1 here, in case a random value due to HCI error
                blc_hci_le_setCigParamsTest((hci_le_setCigParamTest_cmdParam_t *)cmdPara, (hci_le_setCigParam_retParam_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 3 + returnPara[2] * 2, returnPara, para);
            }
        } break;

        case HCI_CMD_LE_CREATE_CIS:
        {
            if (blmsParam.cis_cen_en) {
                status    = blc_hci_le_createCis((hci_le_CreateCisParams_t *)cmdPara);
                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                *para32   = HCI_EVT_CMDSTATUS(1, opcode, HCI_CMD_LE_OPCODE_OGF, status);
            }
        } break;

        case HCI_CMD_LE_REMOVE_CIG:
        {
            if (blmsParam.cis_cen_en) {
                u8 returnPara[2];
                blc_hci_le_removeCig(cmdPara[0], (hci_le_removeCig_retParam_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 2, returnPara, para);
            }
        } break;
#endif


#if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE)
        case HCI_CMD_LE_ACCEPT_CIS_REQUEST:
        {
            if (blmsParam.cis_per_en) {
                status    = blc_ll_acceptCisRequest(cmdPara[0] | cmdPara[1] << 8);
                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                *para32   = HCI_EVT_CMDSTATUS(1, opcode, HCI_CMD_LE_OPCODE_OGF, status);
            }
        } break;

        case HCI_CMD_LE_REJECT_CIS_REQUEST:
        {
            if (blmsParam.cis_per_en) {
                u8 returnPara[sizeof(hci_le_rejectCisReq_retParams_t)];
                blc_hci_le_rejectCisReq((hci_le_rejectCisReq_cmdParams_t *)cmdPara, (hci_le_rejectCisReq_retParams_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_rejectCisReq_retParams_t), returnPara, para);
            }
        } break;
#endif

#if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
        case HCI_CMD_LE_CREATE_BIG:
        {
            if (blmsParam.big_bcst_en) {
                status    = blc_hci_le_createBigParams((hci_le_createBigParams_t *)cmdPara);
                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                *para32   = HCI_EVT_CMDSTATUS(1, opcode, HCI_CMD_LE_OPCODE_OGF, status);
            }
        } break;

        case HCI_CMD_LE_CREATE_BIG_TEST:
        {
            if (blmsParam.big_bcst_en) {
                status    = blc_hci_le_createBigParamsTest((hci_le_createBigParamsTest_t *)cmdPara);
                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                *para32   = HCI_EVT_CMDSTATUS(1, opcode, HCI_CMD_LE_OPCODE_OGF, status);
            }
        } break;

        case HCI_CMD_LE_TERMINATE_BIG:
        {
            if (blmsParam.big_bcst_en) {
                status    = blc_hci_le_terminateBig((hci_le_terminateBigParams_t *)cmdPara);
                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                *para32   = HCI_EVT_CMDSTATUS(1, opcode, HCI_CMD_LE_OPCODE_OGF, status);
            }
        } break;
#endif

#if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)
        case HCI_CMD_LE_BIG_CREATE_SYNC:
        {
            if (blmsParam.big_sync_en) {
                status    = blc_hci_le_bigCreateSync((hci_le_bigCreateSyncParams_t *)cmdPara);
                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                *para32   = HCI_EVT_CMDSTATUS(1, opcode, HCI_CMD_LE_OPCODE_OGF, status);
            }
        } break;

        case HCI_CMD_LE_BIG_TERMINATE_SYNC:
        {
            if (blmsParam.big_sync_en) {
                u8 returnPara[2];
                returnPara[0] = blc_hci_le_bigTerminateSync(cmdPara[0], returnPara + 1);
                eventCode     = HCI_EVT_CMD_COMPLETE;
                resultLen     = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 2, returnPara, para);
            }
        } break;
#endif
        case HCI_CMD_LE_REQUEST_PEER_SCA:
        {
        } break;
#if (LL_FEATURE_ENABLE_ISO)
        case HCI_CMD_LE_SETUP_ISO_DATA_PATH:
        {
            if (blmsParam.iso_en) {
                u8 returnPara[sizeof(hci_le_setupIsoDataPath_retParam_t)];
                blc_hci_le_setupIsoDataPath((hci_le_setupIsoDataPath_cmdParam_t *)cmdPara, (hci_le_setupIsoDataPath_retParam_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_setupIsoDataPath_retParam_t), (u8 *)returnPara, (u8 *)para);
            }
        } break;

        case HCI_CMD_LE_REMOVE_ISO_DATA_PATH:
        {
            if (blmsParam.iso_en) {
                u8 returnPara[sizeof(hci_le_rmvIsoDataPath_retParam_t)];
                blc_hci_le_removeIsoDataPath((hci_le_rmvIsoDataPath_cmdParam_t *)cmdPara, (hci_le_rmvIsoDataPath_retParam_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_rmvIsoDataPath_retParam_t), (u8 *)returnPara, (u8 *)para);
            }
        } break;
#endif

#if (LL_FEATURE_ENABLE_ISOCHRONOUS_TEST_MODE)
        case HCI_CMD_LE_ISO_TRANSMIT_TEST:
        {
            if (blmsParam.iso_en) {
                u8 returnPara[sizeof(hci_le_isoTestRetParams_t)];
                returnPara[0] = blc_hci_le_iso_transmit_test((hci_le_isoTestCmdParams_t *)cmdPara, (hci_le_isoTestRetParams_t *)returnPara);
                eventCode     = HCI_EVT_CMD_COMPLETE;
                resultLen     = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_isoTestRetParams_t), returnPara, para);
            }
        } break;

        case HCI_CMD_LE_ISO_RECEIVE_TEST:
        {
            if (blmsParam.iso_en) {
                u8 returnPara[sizeof(hci_le_isoTestRetParams_t)];
                blc_hci_le_iso_receive_test((hci_le_isoTestCmdParams_t *)cmdPara, (hci_le_isoTestRetParams_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_isoTestRetParams_t), returnPara, para);
            }
        } break;

        case HCI_CMD_LE_ISO_READ_TEST_COUNTERS:
        {
            if (blmsParam.iso_en) {
                u8 returnPara[sizeof(hci_le_isoRxTestStatusParam_t)];
                blc_hci_le_iso_read_test_count_cmd((hci_le_isoReadTestCountsCmdParams_t *)cmdPara, (hci_le_isoRxTestStatusParam_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_isoRxTestStatusParam_t), returnPara, para);
            }
        } break;

        case HCI_CMD_LE_ISO_TEST_END:
        {
            if (blmsParam.iso_en) {
                u8 returnPara[sizeof(hci_le_isoRxTestStatusParam_t)];
                blc_hci_le_iso_test_end_cmd((hci_le_isoTestEndCmdParams_t *)cmdPara, (hci_le_isoTestEndStatusParam_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_isoRxTestStatusParam_t), returnPara, para);
            }

        } break;
#endif

        case HCI_CMD_LE_SET_HOST_FEATURE:
        {
            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Set_Host_Feature", cmdPara, 2);
            status    = blc_ll_setHostFeature(cmdPara[0], cmdPara[1]);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        //7.8.116 LE Read ISO Link Quality command
        case HCI_CMD_LE_READ_ISO_LINK_QUALITY:
        {
        } break;

#if (LL_FEATURE_ENABLE_POWER_CONTROL)
        //7.8.117 LE Enhanced Read Transmit Power Level command
        case HCI_CMD_LE_ENHANCED_READ_TRANSMIT_POWER_LEVEL:
        {
            if (blmsParam.pwr_ctrl_en) {
                u8 returnPara[sizeof(hci_le_enRdTxPwrLvlRetParams_t)];
                blc_hci_le_readEnhancedTxPower((hci_le_rdTxPwrLvlCmdParams_t *)cmdPara, (hci_le_enRdTxPwrLvlRetParams_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_enRdTxPwrLvlRetParams_t), returnPara, para);
            }
        } break;

        //7.8.118 LE Read Remote Transmit Power Level command
        case HCI_CMD_LE_READ_REMOTE_TRANSMIT_POWER_LEVEL:
        {
            if (blmsParam.pwr_ctrl_en) {
    #if (CS_EBQ_TEST) // if cs init procedure in process, pending pcl after cs_req/cs_rsp/cs_ind/ done.
                u16 connHandle = (cmdPara[0] | cmdPara[1] << 8);
                tlkapi_send_string_data(1, "connHandle", (u8 *)&connHandle, 2);
                st_ll_conn_t *pAclConn = (st_ll_conn_t *)(u32)&blms[connHandle & CONN_IDX_MASK];
                cs_param_t   *pCsParam = &pAclConn->csParam;

                if (pCsParam->cs_req & (PROC_CS_WAIT_RSP | PROC_CS_WAIT_IND)) {
                    status = BLE_SUCCESS;
                    pCsParam->cs_req |= PROC_CS_PWL_PENDING;
                    tlkapi_send_string_data(1, "check cs req state", (u8 *)&pCsParam->cs_req, 1);
                } else {
                    status = blc_hci_le_readRemoteTxPwrLvl((hci_le_rdTxPwrLvlCmdParams_t *)cmdPara);
                }

    #else
                status = blc_hci_le_readRemoteTxPwrLvl((hci_le_rdTxPwrLvlCmdParams_t *)cmdPara);
    #endif
                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                *para32   = HCI_EVT_CMDSTATUS(1, opcode, HCI_CMD_LE_OPCODE_OGF, status);
            }
        } break;

        //7.8.119 LE Set Path Loss Reporting Parameters command
        case HCI_CMD_LE_SET_PATH_LOSS_REPORTING_PARAMETERS:
        {
            if (blmsParam.pwr_ctrl_en) {
                u8 returnPara[sizeof(hci_le_setPathLossRptingRetParams_t)];
                blc_hci_le_setPathLossRptingParams((hci_le_setPathLossRptingCmdParams_t *)cmdPara, (hci_le_setPathLossRptingRetParams_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_setPathLossRptingRetParams_t), returnPara, para);
            }
        } break;

        //7.8.120 LE Set Path Loss Reporting Enable command
        case HCI_CMD_LE_SET_PATH_LOSS_REPORTING_ENABLE:
        {
            if (blmsParam.pwr_ctrl_en) {
                u8 returnPara[sizeof(hci_le_setPathLossRptingEnRetParams_t)];
                blc_hci_le_setPathLossRptingEnable((hci_le_setPathLossRptingEnCmdParams_t *)cmdPara, (hci_le_setPathLossRptingEnRetParams_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_setPathLossRptingEnRetParams_t), returnPara, para);
            }
        } break;

        //7.8.121 LE Set Transmit Power Reporting Enable command
        case HCI_CMD_LE_SET_TRANSMIT_POWER_REPORTING_ENABLE:
        {
            if (blmsParam.pwr_ctrl_en) {
                u8 returnPara[sizeof(hci_le_setTxPwrRptingEnRetParams_t)];
                blc_hci_le_setTxPwrRptingEnable((hci_le_setTxPwrRptingEnCmdParams_t *)cmdPara, (hci_le_setTxPwrRptingEnRetParams_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_setTxPwrRptingEnRetParams_t), returnPara, para);
            }
        } break;


#endif
        //core_5.2 end

        //core_5.3 begin

#if (LL_FEATURE_ENABLE_PRIVACY && LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE)
        case HCI_CMD_LE_SET_DATA_RELATE_ADDRESS_CHANGES:
        {
            status    = blc_hci_le_setDataRelatedAddressChange((hci_le_setDataAddrChange_cmdParams_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;
#endif

#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
        case HCI_CMD_LE_SET_DEFAULT_SUBRATE:
        {
            if (blmsParam.subrate_en) {
                status    = blc_hci_le_set_default_subrate((hci_le_setDefaultSubrateCmdParams_t *)cmdPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
            }
        } break;

        case HCI_CMD_LE_SUBRATE_REQUEST:
        {
            if (blmsParam.subrate_en) {
                status    = blc_hci_le_subrate_request((hci_le_subrateRequestCmdParams_t *)cmdPara);
                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                *para32   = HCI_EVT_CMDSTATUS(1, opcode, HCI_CMD_LE_OPCODE_OGF, status);
            }
        } break;
#endif
        //core_5.3 end

        //core_6.0 begin
#if (LL_FEATURE_ENABLE_DECISION_BASED_ADVERTISING_FILTER)
        case HCI_CMD_LE_SET_DECISION_DATA:
        {
            status = blc_ll_setDecisionData(cmdPara[0], cmdPara[1], cmdPara[2], &cmdPara[3]);//hci_le_setDecisionData_t
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        }
        break;

        case HCI_CMD_LE_SET_DECISION_INSTRUCTIONS:
        {
            status = blc_ll_setDecisionInstructCmd(cmdPara[0], (dec_ins_t*)&cmdPara[1]);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        }
        break;
#endif

#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
        case HCI_CMD_LE_CS_READ_LOCAL_SUPPORTED_CAPABILITIES:
        {
            u8 buff[sizeof(hci_le_cs_readLocalSupportedCap_retParam_t)] = {0}; //return length is 28
            status                                                      = (u8)blc_hci_le_cs_readLocalSupportedCap((hci_le_cs_readLocalSupportedCap_retParam_t *)buff);
            eventCode                                                   = HCI_EVT_CMD_COMPLETE;
            resultLen                                                   = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_cs_readLocalSupportedCap_retParam_t), buff, para);
        } break;
        case HCI_CMD_LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES:
        {
            CS_HCI_LOG("Read_Remote_Cap:%s", hex_to_str(cmdPara, 2));
            status    = (u8)blc_hci_le_cs_readRemoteSupportedCap(cmdPara[0] | cmdPara[1] << 8);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
        } break;
        case HCI_CMD_LE_CS_WRITE_REMOTE_SUPPORTED_CAPABILITIES:
        {
            CS_HCI_LOG("Write_Cache_Remote_Cap:%s", hex_to_str(cmdPara, sizeof(hci_le_cs_writeCachedRemoteSupportedCap_cmdParam_t)));
            u8 buff[sizeof(hci_le_cs_writeCachedRemoteSupportedCap_retParam_t)] = {0}; //return length is 3
            status                                                              = (u8)blc_hci_le_cs_writeCachedRemoteSupportedCap(
                (hci_le_cs_writeCachedRemoteSupportedCap_cmdParam_t *)cmdPara,
                (hci_le_cs_writeCachedRemoteSupportedCap_retParam_t *)buff);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_cs_writeCachedRemoteSupportedCap_retParam_t), buff, para);
        } break;
        case HCI_CMD_LE_CS_SECURITY_ENABLE:
        {
            CS_HCI_LOG("CS_Sec_Enable:%s", hex_to_str(cmdPara, 2));
            status    = (u8)blc_hci_le_cs_security_enable(cmdPara[0] | cmdPara[1] << 8);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
        } break;
        case HCI_CMD_LE_CS_SET_DEFAULT_SETTINGS:
        {
            CS_HCI_LOG("CS_Set_Default_Setting:%s", hex_to_str(cmdPara, sizeof(hci_le_cs_setDefaultSetting_cmdParam_t)));
            u8 buff[sizeof(hci_le_cs_setDefaultSetting_retParam_t)] = {0}; //return length is 3
            status                                                  = (u8)blc_hci_le_cs_setDefaultSettings((hci_le_cs_setDefaultSetting_cmdParam_t *)cmdPara,
                                                          (hci_le_cs_setDefaultSetting_retParam_t *)buff);
            eventCode                                               = HCI_EVT_CMD_COMPLETE;
            resultLen                                               = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_cs_setDefaultSetting_retParam_t), buff, para);
        } break;
        case HCI_CMD_LE_CS_READ_REMOTE_FAE_TABLE:
        {
            CS_HCI_LOG("Read_Remote_FAE_Table:%s", hex_to_str(cmdPara, 2));
            status    = (u8)blc_hci_le_cs_readRemoteFAE_table(cmdPara[0] | cmdPara[1] << 8);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
        } break;
        case HCI_CMD_LE_CS_WRITE_CACHE_REMOTE_FAE_TABLE:
        {
            CS_HCI_LOG("Write_Cache_Remote_FAE_Table:%s", hex_to_str(cmdPara, 74));
            u8 buff[sizeof(hci_le_cs_writeChchedRemoteFAE_retParam_t)] = {0}; //return length is 3
            status                                                     = (u8)blc_hci_le_cs_writeCachedRemoteFAE_table((cmdPara[0] | cmdPara[1] << 8), &cmdPara[2], (hci_le_cs_writeChchedRemoteFAE_retParam_t *)buff);
            eventCode                                                  = HCI_EVT_CMD_COMPLETE;
            resultLen                                                  = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_cs_writeChchedRemoteFAE_retParam_t), buff, para);
        } break;
        case HCI_CMD_LE_CS_CREATE_CONFIG:
        {
            CS_HCI_LOG("CS_Create_Config:%s", hex_to_str(cmdPara, sizeof(hci_le_cs_creatConfig_cmdParam_t)));
            status    = (u8)blc_hci_le_cs_createConfig((hci_le_cs_creatConfig_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
        } break;
        case HCI_CMD_LE_CS_REMOVE_CONFIG:
        {
            CS_HCI_LOG("CS_Remove_Config:%s", hex_to_str(cmdPara, 3));
            status    = (u8)blc_hci_le_cs_removeConfig((cmdPara[0] | cmdPara[1] << 8), cmdPara[2]);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
        } break;
        case HCI_CMD_LE_CS_SET_Channel_CLASSIFICATION:
        {
            CS_HCI_LOG("CS_Set_Chn_Classification:%s", hex_to_str(cmdPara, 10));
            status    = (u8)blc_hci_le_cs_setChannelClassification(cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;
        case HCI_CMD_LE_CS_SET_PROCEDURE_PARAMETERS:
        {
            CS_HCI_LOG("Set_Procedure_Param:%s", hex_to_str(cmdPara, sizeof(hci_le_cs_setProcedureParame_cmdParam_t)));
            u8 buff[sizeof(hci_le_cs_setProcedureParam_retParam_t)] = {0}; //return length is 3
            status                                                  = (u8)blc_hci_le_cs_setProcedureParam((hci_le_cs_setProcedureParame_cmdParam_t *)cmdPara,
                                                         (hci_le_cs_setProcedureParam_retParam_t *)buff);
            eventCode                                               = HCI_EVT_CMD_COMPLETE;
            resultLen                                               = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_cs_setProcedureParam_retParam_t), buff, para);
        } break;
        case HCI_CMD_LE_CS_PROCEDURE_ENABLE:
        {
            CS_HCI_LOG("CS_Procedure_Enable:%s", hex_to_str(cmdPara, 4));
            status    = (u8)blc_hci_le_cs_procedureEnable((hci_le_cs_enableProcedure_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
        } break;
        case HCI_CMD_LE_CS_TEST:
        {
    #if (CHANNEL_SOUNDING_TEST_MODE_ENABLE)
            CS_TEST_SEND_STRING(1, "get test cmd param", cmdPara, 32);
            status     = (u8)blc_hci_le_cs_startCsTest((hci_le_cs_test_cmdParam_t *)cmdPara);
            eventCode  = HCI_EVT_CMD_COMPLETE;
            u8 buff[1] = {0};
            buff[0]    = status;
            resultLen  = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, buff, para);
    #endif
        } break;
        case HCI_CMD_LE_CS_TEST_END:
        {
    #if (CHANNEL_SOUNDING_TEST_MODE_ENABLE)
            CS_TEST_LOG("Test mode end!!!");
            status    = (u8)blc_hci_le_cs_endCsTest();
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
            hci_le_csTestEndComplete_evt(status);
    #endif
        } break;
#endif

#if (LL_FEATURE_ENABLE_MONITORING_ADVERTISERS)
        //7.8.149 LE Enable Monitoring Advertisers command
        case HCI_CMD_LE_ENABLE_MONITORING_ADVERTISERS:
        {
            status = blc_ll_monitoringAdvertisersEnable(cmdPara[0]);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        case HCI_CMD_LE_READ_MONITORED_ADVERTISERS_LIST_SIZE:
        {
            u8 returnParam[sizeof(hci_le_readMonitoredAdvertisersListSizeStatusParam_t)];
            returnParam[0] = blc_ll_readMonitoredAdvertisersListSize((hci_le_readMonitoredAdvertisersListSizeStatusParam_t*)returnParam);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, sizeof(hci_le_readMonitoredAdvertisersListSizeStatusParam_t), returnParam, para);
        } break;

        case HCI_CMD_LE_REMOVE_DEVICE_FROM_MONITORED_ADVERTISERS_LIST:
        {
            status    = blc_ll_removeDeviceFromMonitoredAdvertisersList(cmdPara[0], cmdPara + 1);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        case HCI_CMD_LE_ADD_DEVICE_TO_MONITORED_ADVERTISERS_LIST:
        {
            status    = blc_hci_le_addDeviceToMonitoredAdvertisersList((hci_le_addDeviceToMonitoredAdvertisersListcmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;

        case HCI_CMD_LE_CLEAR_MONITORED_ADVERTISERS_LIST:
        {
            status    = (u8)blc_ll_clearMonitoredAdvertisersList();
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, 1, &status, para);
        } break;
#endif

#if (LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)

        case HCI_CMD_LE_FRAME_SPACE_UPDATE:
        {
            if(blmsParam.fsu_en){
                u16 connHandle  = cmdPara[1] << 8 | cmdPara[0];
                u16 fs_min      = cmdPara[3] << 8 | cmdPara[2];
                u16 fs_max      = cmdPara[5] << 8 | cmdPara[4];
                u8 phys         = cmdPara[6];
                u16 spacingType = cmdPara[8] << 8 | cmdPara[7];

                status    = (u8)blc_ll_frameSpaceUpdate(connHandle, fs_min, fs_max, phys, spacingType);

                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                hci_cmdStatus_evt(1, opcode, HCI_CMD_LE_OPCODE_OGF, status, para);
            }
        }
        break;
#endif
        //core_6.0 end

        default:
            break;
        } //end of switch
    }
    ///Test commands--OGF(0x06);;; Link Policy commands--OGF(0x02)
    else if (p[2] == HCI_CMD_TEST_OPCODE_OGF || p[2] == HCI_CMD_LINK_POLICY_OPCODE_OGF || p[2] == (0x05 << 2)) //vendor cmds group
    {
        if (bltHciMng.hciCmplEvtEn) {
            status    = HCI_ERR_UNKNOWN_HCI_CMD;
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, p[2], 1, &status, para);
        } else {
            status    = HCI_ERR_UNKNOWN_HCI_CMD;
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, p[2], status, para);
        }
    }
    else if ((p[2] & HCI_CMD_VENDOR_OPCODE_OGF) == HCI_CMD_VENDOR_OPCODE_OGF) //vendor cmds group
    {
        my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] HCI_CMD_VENDOR_OPCODE_OGF", 0, 0);
        extern unsigned char hci_vendor_Process(u8 pCmdparaLen, u8 opCode_ogf, u8 opCode_ocf, hci_vendor_CmdParams_t * pCmd, hci_vendor_EndStatusParam_t * pRetParam);
        resultLen = hci_vendor_Process(p[3], p[2], opcode, cmdPara, para);
        extern unsigned char hci_vendor_getCurrentEventCode(void);
        eventCode = hci_vendor_getCurrentEventCode();
#if (LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT)
    }else if ((p[2] & HCI_CMD_HDT_OPCODE_OGF) == HCI_CMD_HDT_OPCODE_OGF) //higher data throughput test
    {
        switch (opcode) {
#if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER)
        case HCI_CMD_LE_SET_CIG_PARAMETERS_V3:
        {
            if (blmsParam.cis_cen_en) {
                u8 returnPara[3 + (CIS_IN_CIGM_NUM_MAX * 2)]; //3 + CIS_CounT_max*2
                blc_hci_le_setCigParams_V3((hci_le_setCigParamV3_cmdParam_t *)cmdPara, (hci_le_setCigParam_retParam_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_HDT_OPCODE_OGF, 3 + returnPara[2] * 2, returnPara, para);
            }
        } break;
        case HCI_CMD_LE_SET_CIG_PARAMETERS_TEST_V3:
        {
            if (blmsParam.cis_cen_en) {
                u8 returnPara[3 + (CIS_IN_CIGM_NUM_MAX << 1)]; //3 + CIS_CounT_max*2
                returnPara[2] = 1;                             //CIS count set 1 here, in case a random value due to HCI error
                blc_hci_le_setCigParamsTest_V3((hci_le_setCigParamTestV3_cmdParam_t *)cmdPara, (hci_le_setCigParam_retParam_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_HDT_OPCODE_OGF, 3 + returnPara[2] * 2, returnPara, para);
            }
        } break;
#endif

#if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
        case HCI_CMD_LE_CREATE_BIG_V2:
        {
            if (blmsParam.big_bcst_en) {
                status    = blc_hci_le_createBigParams_V2((hci_le_createBigParamsV2_t *)cmdPara);
                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                *para32   = HCI_EVT_CMDSTATUS(1, opcode, HCI_CMD_HDT_OPCODE_OGF, status);
            }
        } break;
        case HCI_CMD_LE_CREATE_BIG_TEST_V2:
        {
            if (blmsParam.big_bcst_en) {
                status    = blc_hci_le_createBigParamsTest_V2((hci_le_createBigParamsTestV2_t *)cmdPara);
                eventCode = HCI_EVT_CMD_STATUS;
                resultLen = 4;
                *para32   = HCI_EVT_CMDSTATUS(1, opcode, HCI_CMD_HDT_OPCODE_OGF, status);
            }
        } break;
#endif
        case HCI_CMD_LE_SET_HDT_DEFAULT_PARAMETERS:
        {
            if(blmsParam.phy_hdt_en){
                u8 returnPara = blc_hci_le_setHdtDftParams((hci_le_setHdtDftParam_cmdParam_t *)cmdPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_HDT_OPCODE_OGF, 1, &returnPara, para);
            }
        } break;
        case HCI_CMD_LE_SET_HDT_PARAMETERS_TEST:
        {
            if(blmsParam.phy_hdt_en){
                u8 returnPara[3];
                blc_hci_le_setHdtParamsTest((hci_le_setHdtParamTest_cmdParam_t *)cmdPara, (hci_le_setHdtParamTest_retParam_t *)returnPara);
                eventCode = HCI_EVT_CMD_COMPLETE;
                resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_HDT_OPCODE_OGF, 3, returnPara, para);
            }
        } break;
        case HCI_CMD_LE_READ_HDT_LOCAL_SUPPORTED_CAPABILITIES:
        {
        } break;
        case HCI_CMD_LE_TRANSMITTER_TEST_V5:
        {
            status    = blt_phyTest_hci_setTransmitterTest_V5((hci_le_transmitterTestV5_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_HDT_OPCODE_OGF, 1, &status, para);
        } break;
        case HCI_CMD_HDT_TEST_END_V2:
        {
        } break;
        case HCI_CMD_LE_READ_MAXIMUM_DATA_LENGTH_V2:
        {
            u8 returnPara[13];
            blc_hci_le_readMaxDataLength_V2((hci_le_readMaxDataLengthV2_cmdParam_t *)cmdPara, (hci_le_readMaxDataLengthV2_retParam_t *)returnPara);
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, HCI_CMD_HDT_OPCODE_OGF, 13, returnPara, para);
        } break;
        case HCI_CMD_REFRESH_ENCRYPTION_KEY_V2:
        {
            status    = blc_hci_refreshEncyptKey_V2((hci_refreshEncryptKeyV2_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            *para32   = HCI_EVT_CMDSTATUS(1, opcode, HCI_CMD_HDT_OPCODE_OGF, status);
        }break;
        case HCI_CMD_LE_START_ENCRYPTION_V2:
        {
            status    = blc_hci_le_enableEncryption_V2((hci_le_enableEncryptionV2_cmdParam_t *)cmdPara);
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, HCI_CMD_HDT_OPCODE_OGF, status, para);
        } break;
        default:
            break;
        } //end of switch
#endif
    } else {
        //  01 06 04 03 01 00 13: disconnect (handle 01 00; reason: 0x13)
        //  01 0d 04 02 01 00   : read remote version info
        //  01 01 0c   ... set event mask
        //  01 03 0c   ... reset hci
        //  01 01 10   ... read local version info
        //  01 03 10   ... read local supported feature
        //  01 05 10   ... read buffer size
        //  01 09 10   ... read BD address

        status  = 0; //OK
        u32 ret = HCI_EVT_CMD_COMPLETE_STATUS(1, opcode, p[2], status);
        blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_CMD_COMPLETE, (u8 *)&ret, 4);
        return 0;
    }

    if (resultLen) {
        header = HCI_FLAG_EVENT_BT_STD | eventCode;
        blc_hci_send_event(header, para, resultLen);
    } else { // IUT which supports LE only, does not respond to BR/EDR HCI commands.return an HCI Command
        // Complete Event or HCI Command Status Event with Status = Unknown HCI Command.

        //              if(opcode != HCI_CMD_LE_OPCODE_OGF)
        if (bltHciMng.hciCmplEvtEn) {
            status    = HCI_ERR_UNKNOWN_HCI_CMD; //HCI/GEV/BV-01-C
            eventCode = HCI_EVT_CMD_COMPLETE;
            resultLen = hci_cmdComplete_evt(1, opcode, p[2], 1, &status, para);

            header = HCI_FLAG_EVENT_BT_STD | eventCode;
            blc_hci_send_event(header, para, resultLen);
        } else {
            status    = HCI_ERR_UNKNOWN_HCI_CMD; //HCI/GEV/BV-01-C
            eventCode = HCI_EVT_CMD_STATUS;
            resultLen = 4;
            hci_cmdStatus_evt(1, opcode, p[2], status, para);

            header = HCI_FLAG_EVENT_BT_STD | eventCode;
            blc_hci_send_event(header, para, resultLen);
        }
    }


    return 0;
}

int blc_hci_getFreeTxFIFONum(void)
{
    u8 fifo_usedNum = (bltHci_txfifo.wptr - bltHci_txfifo.rptr) & 255;

    if (bltHci_txfifo.num) {
        return bltHci_txfifo.num - fifo_usedNum;
    }

    return 0;
}

int blc_hci_isHciTxFIFOfull(void)
{
    if (((bltHci_txfifo.wptr - bltHci_txfifo.rptr) & 255) < bltHci_txfifo.num) {
        return 0; //there still are buffer to use.
    }

    return -1;
}

///////////////////////////////////////////
// TX
///////////////////////////////////////////
#if HCI_SEND_NUM_OF_CMP_AFT_ACK
_attribute_ram_code_
#endif
    int
    blc_hci_send_data(u32 h, u8 *para, int n)
{
    u8 *p = NULL;
    if (((bltHci_txfifo.wptr - bltHci_txfifo.rptr) & 255) < bltHci_txfifo.num) {
        p = bltHci_txfifo.p + (bltHci_txfifo.wptr & bltHci_txfifo.mask) * bltHci_txfifo.size;
    }

    if (!p || n >= (int)bltHci_txfifo.size) {
#if (UPPER_TESTER_HCI_LOG_EN)
        if (h & HCI_FLAG_EVENT_BT_STD) {
            my_dump_str_data(UPPER_TESTER_HCI_LOG_EN, "controller TX FIFO overflow", 0, 0);
        }
#endif

        return -1;
    }

    int nl = n + 4;
    if (h & HCI_FLAG_EVENT_TLK_MODULE) {
        *p++ = nl;
        *p++ = nl >> 8;
        *p++ = 0xff;
        *p++ = n + 2;
        *p++ = h;
        *p++ = h >> 8;
        smemcpy(p, para, n);
        p += n;
    } else if (h & HCI_FLAG_EVENT_BT_STD) {
        //skip le adv active evt
        if ((h & 0xff) == HCI_EVT_LE_META &&
            (para[0] == HCI_SUB_EVT_LE_ADVERTISING_REPORT || para[0] == HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT)) {
            p = NULL;

            if (((bltHci_txfifo.wptr - bltHci_txfifo.rptr) & 255) < bltHci_txfifo.num - HCI_ADV_REPORT_EVT_RSVD_FIFO) { //keep 3 fifo left for others evt
                p = bltHci_txfifo.p + (bltHci_txfifo.wptr & bltHci_txfifo.mask) * bltHci_txfifo.size;
            }
            if (!p) {
                return -1;
            }
        }
#if (0)
        // debug-yuexin
        if (h & 0xff == HCI_EVT_LE_META) {
            u8  subEvt_code = p[0];
            u16 handle      = p[1] | (p[2] << 8);

            if (subEvt_code == HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT) {
                hci_le_csSubeventResultEvt_t *pCsSubevent = (hci_le_csSubeventResultEvt_t *)p;
                /* print subevent data to txt file to calculate distance with other company */
                u8 tempBuff[258];
                tempBuff[0] = 0x04; //type
                tempBuff[1] = 0x3E; //event_code
                tempBuff[2] = n;    // total_len
                smemcpy(tempBuff + 3, &pCsSubevent->Subevent_Code, n);
                tlkapi_printf(1, "subevent len***:%d\r\n", n);
                tlkapi_send_string_data(1, "cs subevent data", tempBuff, n + 3);
                debugwait();
            }
            //------HCI LE event: LE CS Subevent Result Continue event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT_CONTINUE) {
                hci_le_csSubeventResultContinueEvt_t *pCsSubevent = (hci_le_csSubeventResultContinueEvt_t *)p;
                /* print subevent continue data to txt file to calculate distance with other company */
                u8 tempBuff[258];
                tempBuff[0] = 0x04;
                tempBuff[1] = 0x3E; //event_code
                tempBuff[2] = n;    // total_len
                smemcpy(tempBuff + 3, &pCsSubevent->Subevent_Code, n);
                tlkapi_printf(1, "continue subevent len***:%d\r\n", n);
                tlkapi_send_string_data(1, "cs continue subevent data", tempBuff, n + 3);
                debugwait();
            }
        }
#endif
#if HCI_TX_FIFO_OPTIMIZE_EN
        if (!(para[0] == HCI_SUB_EVT_LE_ADVERTISING_REPORT || para[0] == HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT)) {
            u8 w = bltHci_txfifo.wptr - 1;

            if ((u8)(bltHci_txfifo.wptr - bltHci_txfifo.rptr) >= 3) {
                while (w != bltHci_txfifo.rptr) //H5 slide win = 1
                {
                    u8 *pBuf = bltHci_txfifo.p + (w & bltHci_txfifo.mask) * bltHci_txfifo.size;
                    if (pBuf[2] == HCI_TYPE_ACL_DATA ||
                        (pBuf[2] == HCI_TYPE_EVENT && (pBuf[3] != 0x3E || (pBuf[3] == 0x3E && pBuf[5] != 0x02 && pBuf[5] != 0x0D)))) {
                        break;
                    }
                    w--;
                }

                if (w != (u8)(bltHci_txfifo.wptr - 1)) {
                    w++;
                    p = bltHci_txfifo.p + (w & bltHci_txfifo.mask) * bltHci_txfifo.size;

                    *p++ = n + 3;          //3: 1(HCI Type) + 1(Event Code) + 1(Parameter Total Length)
                    *p++ = 0x0;
                    *p++ = HCI_TYPE_EVENT; //HCI Type
                    *p++ = h;              //Event Code
                    *p++ = n;              //Parameter Total Length
                    smemcpy(p, para, n);
                    p += n;

                    return 0; //[!!!important]
                }
            }
        }
#endif


        *p++ = n + 3;                   //3: 1(HCI Type) + 1(Event Code) + 1(Parameter Total Length)
        *p++ = (n + 3) >> 8;

        *p++ = HCI_TYPE_EVENT;          //HCI Type
        *p++ = h;                       //Event Code
        *p++ = n;                       //Parameter Total Length
        smemcpy(p, para, n);
        p += n;
    } else if (h & HCI_FLAG_ACL_BT_STD) //ACL data
    {
#if HCI_TX_FIFO_OPTIMIZE_EN
        p = NULL;
        if (((bltHci_txfifo.wptr - bltHci_txfifo.rptr) & 255) < bltHci_txfifo.num - 2) { //keep 2 fifo left for others evt
            p = bltHci_txfifo.p + (bltHci_txfifo.wptr & bltHci_txfifo.mask) * bltHci_txfifo.size;
        }
        if (!p) {
            return -1;
        }

        u8 w = bltHci_txfifo.wptr - 1;

        if ((u8)(bltHci_txfifo.wptr - bltHci_txfifo.rptr) >= bltHci_txfifo.num - 8) {
            while (w != bltHci_txfifo.rptr) //H5 slide win = 1
            {
                u8 *pBuf = bltHci_txfifo.p + (w & bltHci_txfifo.mask) * bltHci_txfifo.size;
                if (pBuf[2] == HCI_TYPE_ACL_DATA ||
                    (pBuf[2] == HCI_TYPE_EVENT && (pBuf[3] != 0x3E || (pBuf[3] == 0x3E && pBuf[5] != 0x02 && pBuf[5] != 0x0D)))) {
                    break;
                }
                w--;
            }

            if (w != (u8)(bltHci_txfifo.wptr - 1)) {
                w++;
                p = bltHci_txfifo.p + (w & bltHci_txfifo.mask) * bltHci_txfifo.size;

                n    = para[1];                                                                   // para[1]: rf_len
                *p++ = n + 5;                                                                     // handle| PB | BC :2byte    data total length: 2byte  : ACL date Type(0x02) 1byte
                *p++ = (n + 5) >> 8;
                *p++ = 0x02;
                *p++ = h;                                                                         // para[1]: llid       //Handle:12|PB flag:2|BC flag:2 (2B)
                *p++ = ((h >> 8) & 0x0F) | ((para[0] & 3) == L2CAP_CONTINUING_PKT ? 0x10 : 0x20); //start llid 2 ->0x20 ;  continue llid 1 ->0x10
                *p++ = n;                                                                         //HCI ACL Data Total Length (2B)
                *p++ = n >> 8;
                smemcpy(p, para + 2, n);
                p += n;
    #if HCI_CONTROLLER_TO_HOST_FLOW_CTRL_EN
                hci_reduceOneHostAvailBuf();
    #endif
                return 0; //[!!!important]
            }
        }
#endif

        /*
        +------+-------+---------+-----------+-----------------+
        | 2    | 1     | 2       | 2         | n               |
        +------+-------+---------+-----------+-----------------+
        | len  | type  | handle  | data_len  | data_total_len  |
        +------+-------+---------+-----------+-----------------+
         */
        n    = para[1]; // para[1]: rf_len
        *p++ = n + 5;   // handle| PB | BC :2byte    data total length: 2byte  : ACL date Type(0x02) 1byte
        *p++ = (n + 5) >> 8;

        *p++ = HCI_TYPE_ACL_DATA;
        *p++ = h;                                                                         // para[1]: llid       //Handle:12|PB flag:2|BC flag:2 (2B)
        *p++ = ((h >> 8) & 0x0F) | ((para[0] & 3) == L2CAP_CONTINUING_PKT ? 0x10 : 0x20); //start llid 2 ->0x20 ;  continue llid 1 ->0x10
        *p++ = n;                                                                         //HCI ACL Data Total Length (2B)
        *p++ = n >> 8;
        smemcpy(p, para + 2, n);
        p += n;

#if HCI_CONTROLLER_TO_HOST_FLOW_CTRL_EN
        hci_reduceOneHostAvailBuf();
#endif
    } else if (h & HCI_FLAG_EVENT_PHYTEST_2_WIRE_UART) {
        *p++ = n; //length
        *p++ = n >> 8;
        smemcpy(p, para, n);
    }
#if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    else if (h & HCI_FLAG_ISO_DATE_STD) {
        int len = 1 + n; //type(1)
        *p++    = U16_LO(len);
        *p++    = U16_HI(len);
        *p++    = HCI_TYPE_ISO_DATA;
        smemcpy(p, para, n);
    }
#endif
#if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER & 0)
    else if (h & HCI_FLAG_ISO_DATE_STD) {

        /* HCI ISO out DATA format in telink
        +------+-------+---------+-----------------------+------------+----------------------+-----------------+----------+
        | 2    | 1     | 2       | 2                     | 4          | 2                    | 2               | n        |
        +------+-------+---------+-----------------------+------------+----------------------+-----------------+----------+
        | len  | type  | handle  | ISO_data_load_length  | timestamp  | packet_sequence_num  | iso_sdu_length  | sd_data  |
        +------+-------+---------+-----------------------+------------+----------------------+-----------------+----------+
         */

        u16           iso_data_load_len, len;
        sdu_packet_t *sdu = (sdu_packet_t *)para;

        if (gIsoTsEn) {
            iso_data_load_len = sdu->iso_sdu_len + 8;
        } else {
            iso_data_load_len = sdu->iso_sdu_len + 4;
        }
        len = iso_data_load_len + 5; // type(1), handle(2), iso_data_load_length(2)


        *p++ = U16_LO(len);
        *p++ = U16_HI(len);
        *p++ = HCI_TYPE_ISO_DATA;

        u16 hBit = ((h & 0xFFF) | (HCI_ISO_SDU_COMPLETE << 12) | (((gIsoTsEn) ? 1 : 0) << 14));
        //handle 2byte
        *p++ = U16_LO(hBit);
        *p++ = U16_HI(hBit);

        //iso_data_load_len
        iso_data_load_len &= 0x3FFF;
        *p++ = U16_LO(iso_data_load_len);
        *p++ = U16_HI(iso_data_load_len);

        //timestamp 4byte if ts is set
        if (gIsoTsEn) {
            *p++ = U32_BYTE0(sdu->timestamp);
            *p++ = U32_BYTE1(sdu->timestamp);
            *p++ = U32_BYTE2(sdu->timestamp);
            *p++ = U32_BYTE3(sdu->timestamp);
        }

        //packet sequence number
        *p++ = U16_LO(sdu->pkt_seq_num);
        *p++ = U16_HI(sdu->pkt_seq_num);

        //iso_sdu len
        u16 iso_sdu_len = (sdu->iso_sdu_len & 0x0fff) | (sdu->pkt_st << 14);
        *p++            = U16_LO(iso_sdu_len);
        *p++            = U16_HI(iso_sdu_len);

        smemcpy(p, sdu->data, sdu->iso_sdu_len);
    }
#endif


    bltHci_txfifo.wptr++;

    return 0;
}


#if (LL_FEATURE_ENABLE_ISO)
int blc_hci_iso_send_data(u32 h, u8 *iso_load, int data_load_len)
{
    (void)data_load_len;
    (void)h;
    (void)iso_load;
    #if 0
    u8 *p = NULL;
    if (((bltHci_txfifo.wptr - bltHci_txfifo.rptr) & 255) < bltHci_txfifo.num)
    {
        p = bltHci_txfifo.p + (bltHci_txfifo.wptr & bltHci_txfifo.mask) * bltHci_txfifo.size;
    }

    if (!p || n >= bltHci_txfifo.size)
    {
        #if (UPPER_TESTER_HCI_LOG_EN)
            if(h & HCI_FLAG_EVENT_BT_STD){
                my_dump_str_data(UPPER_TESTER_HCI_LOG_EN, "controller TX FIFO overflow", 0, 0);
            }
        #endif

        return -1;
    }


    if(h | HCI_FLAG_ISO_DATE_STD)
    {
        u16 dma_len = data_load_len + 1; //HCI ISO Data packet 1byte

        *p++ = dma_len & 0xff;
        *p++ = dma_len >> 8;

        *p++ = HCI_TYPE_ISO_DATA; //HCI ISO Data packet
        smemcpy (p, iso_load, data_load_len);
    }

    hci_tx_iso_fifo.wptr++;
    #endif

    return 0;
}
#endif


/////////////////////////////////////
//  checking incoming packet
//  send out pending data in buffer
////////////////////////////////////
int blc_hci_proc(void)
{
    ///////// RX //////////////
    if (blc_hci_rx_handler) //rx_from_uart_cb
    {
        blc_hci_rx_handler();
    }

    ///////// TX //////////////
    if (blc_hci_tx_handler) //tx_to_uart_cb
    {
        blc_hci_tx_handler();
    }
    return 0;
}
