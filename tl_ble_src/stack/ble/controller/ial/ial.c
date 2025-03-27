/********************************************************************************************************
 * @file    ial.c
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



#if (LL_FEATURE_ENABLE_ISO)

#define   BQB_TEST_WALKAROUNT_EN                    1
#define   BQB_PLDNUM_ESTIMATE_OPTIMIZE_EN           1

#ifndef     WALK_AROUND_CEN_BV_28
#define     WALK_AROUND_CEN_BV_28       0
#endif


#define     DEBUG_UNFRAMED_RX       (0)

#ifndef     DEBUG_PAYLOAD_NUM
#define     DEBUG_PAYLOAD_NUM       0
#endif


#define ISO_FRAMED_SEGM_HEADER_LEN                  (2)
#define ISO_FRAMED_TIMEOFFSET_LEN                   (3)



//sdu buff manage
_attribute_aligned_(4)  iso_sdu_mng_t sduCisMng;
_attribute_aligned_(4)  iso_sdu_mng_t sduBisMng;



/*********************************************************************************************************
 * CIS Function start
 *********************************************************************************************************/
#if (LL_FEATURE_ENABLE_CONNECTED_ISO)



iso_evtcnt_t blt_ial_cis_estimateUnframedPldNum(ll_cis_conn_t *pCisConn, u8 pn)
{

    iso_evtcnt_t currPldNum = 0;  //u32 is enough. though 40bit in Spec

    tlkapi_send_string_u8s(DEBUG_PAYLOAD_NUM, "getPldNum", pn, blt_debug_hex_2_dec_display(pCisConn->cisSendPldNum), pCisConn->tx_numSdu2Pdu, pCisConn->txNullPduFlag);

    if(pn==0)
    {
        currPldNum = (pCisConn->cisSendPldNum + pCisConn->tx_numSdu2Pdu - 1)/pCisConn->tx_numSdu2Pdu * pCisConn->tx_numSdu2Pdu;

        if(!pCisConn->txNullPduFlag || pCisConn->tx_first_flag){
            currPldNum += pCisConn->tx_numSdu2Pdu;
        }


        if((!pCisConn->tx_first_flag) && (pCisConn->tx_lastpldNum >= currPldNum)){//NES = 2; BN = 1; FT = 2;  fix jump CIG Event
            currPldNum = pCisConn->tx_lastpldNum/pCisConn->tx_numSdu2Pdu * pCisConn->tx_numSdu2Pdu + pCisConn->tx_numSdu2Pdu;
        }

//          if(pCis->tx_first_flag){// fix for tianxiang, the first iso data should set the nearby payloadNum
//              currPldNum = (pCis->cisEventCnt + 1)*pCis->bn_loca;
//          }

        #if(WALK_AROUND_CEN_BV_28)
                currPldNum += pCis->tx_numSdu2Pdu;
        #endif

    }
    else{
        currPldNum = pCisConn->tx_lastpldNum + 1;
    }

    //tlkapi_send_string_u32s(0, "PDU 1", currPldNum, pCis->tx_lastpldNum, 0, 0);

    pCisConn->tx_lastpldNum = currPldNum;

    if(pCisConn->tx_first_flag){
        pCisConn->tx_first_flag =0;
    }


    return currPldNum;
}





/**
 * @brief      This function is used to segmentation/fragmentation SDU to one or more Framed/Unframed PDUs.
 * @param[in]  cis_connHandle
 * @param[in]  sdu  point to sdu buff
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 *                       LL_ERR_INVALID_PARAMETER: command failed
 */
ble_sts_t blt_cis_splitSdu2UnframedPdu(ll_cis_conn_t *pCisConn, sdu_packet_t *sdu, u8 *pNumOfCmpPkt)
{



    // check if have enough fifo to fill one SDU
    /* SiHui note: cisPduTxFifoRptr may changed in IRQ,
     * */
    if(((pCisConn->cisPduTxFifoWptr - pCisConn->cisPduTxFifoRptr) & bltCisPduTxfifo.fifo_mask) + pCisConn->tx_numSdu2Pdu > bltCisPduTxfifo.fifo_num){
        return LL_ERR_TX_FIFO_NOT_ENOUGH;
    }



    u16 slen = 0, pn =0;
    u16 max_pdu_len = pCisConn->max_pdu_loca;
    u8 numPdu = sdu->iso_sdu_len ? ((sdu->iso_sdu_len + max_pdu_len - 1)/max_pdu_len) : 1;

    rf_packet_ll_data_t *pRfPdu=NULL;
    cis_tx_pdu_t *pdu;

    tlkapi_send_string_data(DBG_CIS_TX_DATA_FLOW_EN, "CIS SDU split", &sdu->iso_sdu_len, 2);

    #if(SL16_cis0_txSdu_len)
        log_b16(SL_STACK_CIS_TX_DATA_EN,(SL16_cis0_txSdu_len + ((pCisConn->cisRole)?
                (pCisConn->cis_index): (pCisConn->cis_index - bltCisMng.maxNum_cisMaster))), (u16)sdu->iso_sdu_len);
    #endif

    for(pn=0; pn<pCisConn->tx_numSdu2Pdu; pn++)
    {
        u8 cur_send_pdu_len = (sdu->iso_sdu_len - slen) > max_pdu_len ?  max_pdu_len : (sdu->iso_sdu_len - slen);

        /*Get pdu fifo*/
        pdu = (cis_tx_pdu_t*)(pCisConn->cis_txPduBuf + (pCisConn->cisPduTxFifoWptr & bltCisPduTxfifo.fifo_mask)*bltCisPduTxfifo.fifo_size);
        pRfPdu = &pdu->isoTxPdu;


        /****************************fit CIS PDU*********************************************/
        // the end pdu or only one pdu, llid should 0b00 end/Complete   0b01 start/continue
        pRfPdu->llPhysChnPdu.llPduHdr.pduHdr.type = 0; //clean
        pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.llid = ((pCisConn->tx_numSdu2Pdu==1) || (pn==numPdu-1))? \
                                                        ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU:ISO_LLID_UNFRAMED_PDU_START_CONTI_FRAGMENT_SDU;
        pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.npi = 0;
        pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len = cur_send_pdu_len;
        smemcpy(pRfPdu->llPhysChnPdu.llPayload, (sdu->data +slen), cur_send_pdu_len);


        slen += cur_send_pdu_len;


//       u32 r = irq_disable();//import in case of irq interrupt modify cisSendPldNum
        pdu->cis_pdu_number = blt_ial_cis_estimateUnframedPldNum(pCisConn, pn);

        #if(SL16_cis0_txSetPldNum)
            log_b16(SL_STACK_CIS_TX_DATA_EN,(SL16_cis0_txSetPldNum + ((pCisConn->cisRole)?
                    (pCisConn->cis_index): (pCisConn->cis_index - bltCisMng.maxNum_cisMaster))), (u16)pdu->cis_pdu_number);
        #endif

        if(pCisConn->pCisTestParam && (pCisConn->pCisTestParam->isoTestMode == ISO_TEST_TRANSMIT_MODE) && (pn==0)){
            u32 packt_cnt =  pdu->cis_pdu_number/pCisConn->tx_numSdu2Pdu;
            smemcpy(pRfPdu->llPhysChnPdu.llPayload, &packt_cnt, 4);
        }

#if(HW_AES_CCM_ALG_EN)

        if(pCisConn->crypt.enable &&  pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len){
            pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len += 4;
        }
#else
        /*
         * qinghua.fan
         * 1. 48MHZ, all ramcode, 251 bytes This API cost 896 us
         * 2. 48MHZ, all ramcode, 16  bytes This API cost 144us
         */
        DBG_FANQH_CHN0_HIGH;
        blt_ll_cis_encryptPdu(pCisConn, pdu);
        DBG_FANQH_CHN0_LOW;
#endif

        /* calculate dma_len after encryption and before PDU buffer RPTR change */
        pRfPdu->dma_len = rf_tx_packet_dma_len(pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len + 2);
        pCisConn->cisPduTxFifoWptr ++;

//      irq_restore(r);



        tlkapi_send_string_u32s(DEBUG_PAYLOAD_NUM, "Split2PDU", blt_debug_hex_2_dec_display(pdu->cis_pdu_number), pRfPdu->llPhysChnPdu.llPayload[0], cur_send_pdu_len,0);
        tlkapi_send_string_u32s(DBG_CIS_TX_DATA, "CIS PDU in", pn,pdu->cis_pdu_number, cur_send_pdu_len,0);
        tlkapi_send_string_u32s(DBG_IAL_EN, "split SDU", pCisConn->cisPduTxFifoWptr,pCisConn->cisPduTxFifoRptr,pdu->cis_pdu_number,0);
    }

    (*pNumOfCmpPkt) += sdu->numHciPkt;

    pCisConn->cisSduIn_rptr++;

    return BLE_SUCCESS;
}



/**
 * @brief      This function is used to get the payloadNum .
 * @return     PayloadNum
 *
 */
static iso_evtcnt_t blt_ial_cis_estimateFramedPldNum(ll_cis_conn_t *pCis)
{

    iso_evtcnt_t currPldNum = 0;

    if(!pCis->txNullPduFlag){
        currPldNum = pCis->cisSendPldNum + 1;
    }
    else{
        currPldNum = pCis->cisSendPldNum;
    }

    if(currPldNum <= pCis->tx_lastpldNum)
        currPldNum = pCis->tx_lastpldNum + 1;

    tlkapi_send_string_u32s(DBG_IAL_EN, "get pldNum", pCis->cisSendPldNum,pCis->txNullPduFlag, currPldNum, pCis->tx_lastpldNum);
    pCis->tx_lastpldNum = currPldNum;

    return  currPldNum;
}

/**
 * @brief      This function is used to segmentation/fragmentation SDU to one or more Framed/Unframed PDUs.
 * @param[in]  cis_connHandle
 * @param[in]  sdu  point to sdu buff
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 *                       LL_ERR_INVALID_PARAMETER: command failed
 */
ble_sts_t blt_cis_splitSdu2FramedPdu(ll_cis_conn_t *pCisConn, u8*pNumOfCmpPkt)
{

    u16 remain_len;
    cis_tx_pdu_t *pdu=NULL;
    rf_packet_ll_data_t *pRfPdu=NULL;

    u8 segmHdrLen = ISO_FRAMED_SEGM_HEADER_LEN;

    /***********************Get SDU *************************************/
    sdu_packet_t *sdu = (sdu_packet_t*)(pCisConn->cis_sduInBuf + sduCisMng.max_in_fifo_size * (pCisConn->cisSduIn_rptr & sduCisMng.in_fifo_mask));

    /**********************Get pdu fifo************************************/
    pdu = (cis_tx_pdu_t*)(pCisConn->cis_txPduBuf + (pCisConn->cisPduTxFifoWptr & bltCisPduTxfifo.fifo_mask)*bltCisPduTxfifo.fifo_size);
    pdu->offset = 0;
    pRfPdu = &pdu->isoTxPdu;
    pRfPdu->llPhysChnPdu.llPduHdr.pduHdr.rf_len = 0; //clean
    pRfPdu->llPhysChnPdu.llPduHdr.pduHdr.type = 0; //clean
    pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.llid = ISO_LLID_FRAMED_PDU_SEGMENT_SDU;
    pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.npi = 0;
    iso_framed_segmHdr_t segmHdr= {0};


    //set cisPayloadNum
    //if current PDU not full, next SDU will fill data in it, but next SDU fill it after Current CIS, so time_offset calculate if empty
    pdu->cis_pdu_number = blt_ial_cis_estimateFramedPldNum(pCisConn);

    u8 segmHdrOffset = 0;
    remain_len = pCisConn->max_pdu_loca;

    tlkapi_send_string_u32s(DBG_IAL_EN, "split framed", pdu->cis_pdu_number, sdu->iso_sdu_len, sdu->sduOffset, pdu->offset);


    while(remain_len)
    {

        /******************fit segmentation header*********************/
        if((pdu->offset==0) || (sdu->sduOffset==0))
        {
            //Start or continue
            if(sdu->sduOffset==0)
            {
                segmHdr.sc = 0;

                segmHdr.time_offset = 0x1122;
                segmHdr.length = ISO_FRAMED_TIMEOFFSET_LEN;

                segmHdrLen = ISO_FRAMED_SEGM_HEADER_LEN +ISO_FRAMED_TIMEOFFSET_LEN; //add timeoffset(3bytes)
            }
            else
            {
                segmHdr.sc = 1;
                segmHdr.length = 0;

                segmHdrLen = ISO_FRAMED_SEGM_HEADER_LEN; //exclude time offset
            }

            if(remain_len<=segmHdrLen){
                blt_ll_cis_encryptPdu(pCisConn, pdu);
                /* calculate dma_len after encryption and before PDU buffer RPTR change */
                pRfPdu->dma_len = rf_tx_packet_dma_len(pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len + 2);
                pCisConn->cisPduTxFifoWptr ++;// PDU finalize
                tlkapi_send_string_data(DBG_CIS_TX_DATA, "CIS PDU in 1", 0, 0);
                break;
            }

            smemcpy((pRfPdu->llPhysChnPdu.llPayload + pdu->offset), &segmHdr, segmHdrLen);
            pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len += segmHdrLen;
//          segmHdrLenOffset = pdu->offset + 1;
            segmHdrOffset = pdu->offset;

            pdu->offset += segmHdrLen;

            remain_len -= segmHdrLen;
        }



        // if PDU can't fit at least one byte effective byte, finish this PDU.
        if(remain_len < 1)
        {
            blt_ll_cis_encryptPdu(pCisConn, pdu);
            /* calculate dma_len after encryption and before PDU buffer RPTR change */
            pRfPdu->dma_len = rf_tx_packet_dma_len(pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len + 2);
            pCisConn->cisPduTxFifoWptr ++;// PDU finalize
            tlkapi_send_string_data(DBG_CIS_TX_DATA, "CIS PDU in 1", 0, 0);
            break;
        }


        tlkapi_send_string_u32s(DBG_IAL_EN,"sdu2pdu",remain_len, sdu->iso_sdu_len - sdu->sduOffset, sdu->iso_sdu_len, sdu->sduOffset);
        /*******************fit segmentation of payload***************************************/
        u8 slen=0;
        if(remain_len >= (sdu->iso_sdu_len - sdu->sduOffset))// pdu can hold more than one SDU
        {
            slen = sdu->iso_sdu_len - sdu->sduOffset;
            segmHdr.cmplt = 1;

            // Framed PDU reassemble
            pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len += slen;
            smemcpy((pRfPdu->llPhysChnPdu.llPayload + pdu->offset), (sdu->data + sdu->sduOffset), slen);
            pdu->offset += slen;
            remain_len -= slen; //PDU remain
            segmHdr.length += slen;

//          pRfPdu->llPhysChnPdu.llPayload[segmHdrLenOffset] = segmHdr.length;
            smemcpy((pRfPdu->llPhysChnPdu.llPayload + segmHdrOffset), &segmHdr, segmHdrLen);

            //if(sdu->numHciPkt)
            {
                (*pNumOfCmpPkt) += sdu->numHciPkt;
            }


            pCisConn->cisSduIn_rptr++;  // SDU finalize


            tlkapi_send_string_u32s(0, "Put one SDU", pdu->cis_pdu_number, remain_len, pCisConn->cisSduIn_wptr, pCisConn->cisSduIn_rptr);
            //get next SDU to fit this PDU
            if(pCisConn->cisSduIn_wptr != pCisConn->cisSduIn_rptr)
            {
                //get next sdu
                sdu = (sdu_packet_t*)(pCisConn->cis_sduInBuf + sduCisMng.max_in_fifo_size * (pCisConn->cisSduIn_rptr & sduCisMng.in_fifo_mask));
            }
            else
            {
                blt_ll_cis_encryptPdu(pCisConn, pdu);
                /* calculate dma_len after encryption and before PDU buffer RPTR change */
                pRfPdu->dma_len = rf_tx_packet_dma_len(pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len + 2);
                pCisConn->cisPduTxFifoWptr ++;// PDU finalize
                tlkapi_send_string_data(DBG_CIS_TX_DATA, "CIS PDU in 2", 0, 0);

                tlkapi_send_string_u32s(DBG_IAL_EN, "SDU2PDU framed1", pdu->cis_pdu_number, segmHdr.length, sdu->sduOffset, pCisConn->cisPduTxFifoWptr);

                break;
            }
        }
        else //current PDU full
        {
            slen = remain_len;
            segmHdr.cmplt = 0;

            // Framed PDU reassemble
            pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len += slen;
            smemcpy((pRfPdu->llPhysChnPdu.llPayload + pdu->offset), (sdu->data + sdu->sduOffset), slen);
//                  pdu->offset += slen; //PDU full
            sdu->sduOffset += slen;
            remain_len = 0;// PDU full
            segmHdr.length += slen;
            smemcpy((pRfPdu->llPhysChnPdu.llPayload + segmHdrOffset), &segmHdr, segmHdrLen);


            blt_ll_cis_encryptPdu(pCisConn, pdu);
            /* calculate dma_len after encryption and before PDU buffer RPTR change */
            pRfPdu->dma_len = rf_tx_packet_dma_len(pRfPdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len + 2);
            pCisConn->cisPduTxFifoWptr ++;
            tlkapi_send_string_data(DBG_CIS_TX_DATA, "CIS PDU in 3", 0, 0);

            tlkapi_send_string_u32s(DBG_IAL_EN, "SDU2PDU framed2", pdu->cis_pdu_number, segmHdr.length, sdu->sduOffset, pCisConn->cisPduTxFifoWptr);

        }


    }


    return BLE_SUCCESS;
}








static sdu_packet_t* blt_isoal_getNextCisRxSdu(ll_cis_conn_t *pCisConn)
{

    sdu_packet_t* sdu = (sdu_packet_t*)(pCisConn->cis_sduOutBuf + sduCisMng.max_out_fifo_size * (pCisConn->cisSduOut_wptr&(sduCisMng.out_fifo_mask)));
    sdu->isoHandle = pCisConn->cis_connHandle;

    return sdu;
}





ll_iso_unframe_type_t blt_ial_getCisUnframedType(ll_cis_conn_t *cis_conn,iso_rx_evt_t* pIsoRxEvt, u8 llid){

    ll_iso_unframe_type_t type = UNFRAMED_START;

    iso_evtcnt_t pldNum = pIsoRxEvt->curRcvdPldNum;
    if((pldNum % cis_conn->rx_numSdu2Pdu) == 0) //first PDU packet in a SDU interval
    {
        tlkapi_send_string_u32s(DBG_CIS_RX_DATA, "[CIS RX] PDU data first", pldNum, llid, 0, 0);

        if(!pIsoRxEvt->null_flag)
        {
            if(cis_conn->rx_numSdu2Pdu == 1)
            {
                if(llid != ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU)
                {
                    pIsoRxEvt->null_flag = 1; //purpose to report error SDU, reference  IAL/CIS/UNF/CEN/BI-05-C
                }
                type = UNFRAMED_COMPLETE;
            }
            else if(llid == ISO_LLID_UNFRAMED_PDU_START_CONTI_FRAGMENT_SDU)
            {
                type = UNFRAMED_START;
            }
            else if(llid == ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU)
            {
                type = UNFRAMED_COMPLETE;
            }
            else
            {
                type = UNFRAMED_INVALID;
            }
        }
        else
        {
            if(cis_conn->rx_numSdu2Pdu == 1)
            {
                type = UNFRAMED_COMPLETE;
            }
            else
            {
                type = UNFRAMED_START;
            }
        }

    }
    else
    {
        //condition 1. BN = 4: invalid SDU P1(llid=start), P2(llid=start),P3(llid=start), P4(llid=start)
        //condition 2. fix BN = 2, P1(complete) P2(pading), when P1 lost, then P1 interpret as Start, P2 interpret as continue, then that SDU will not complete.
        if((pldNum + 1) % cis_conn->rx_numSdu2Pdu == 0) //last PDU packet in a SDU interval
        {
            if((!pIsoRxEvt->null_flag) && (llid != ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU ))
            {
                //condition 1. P4 interpret invalid PDU
                pIsoRxEvt->null_flag = 1;
            }
            type = UNFRAMED_END;
        }
        else
        {
            if(!pIsoRxEvt->null_flag)
            {
                if(llid == ISO_LLID_UNFRAMED_PDU_START_CONTI_FRAGMENT_SDU)
                {
                    type = UNFRAMED_CONTINUE;
                }
                else if(llid == ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU)
                {
                    type = UNFRAMED_END;
                }
                else
                {
                    type = UNFRAMED_INVALID;
                }
            }
            else
            {
                type = UNFRAMED_END;
            }
        }
    }
    return type;
}

u32 blt_ial_getCisUnframedTimestamp(ll_cis_conn_t *cis_conn, u32 cisSyncRef, iso_evtcnt_t pld){

    u32 sduSyncRef = 0;
    if(!cis_conn->cisRole)//peripheral
    {
        /*
         *  For SDUs sent from the Central to the Peripheral in CIS using unframed
         *  SDU_Synchronization_Reference = CIS reference anchor point + CIS_Sync_Delay + (FT_C_To_P - 1) × ISO_Interval
         */
    #if(ISO_DATA_TIMESTAMP_UNIT_US_EN)
        sduSyncRef = cisSyncRef + cis_conn->cis_sync_delay + (cis_conn->ft_peer-1) * cis_conn->iso_intvl_us;
    #else
        sduSyncRef = cisSyncRef + cis_conn->cis_sync_delay*SYSTEM_TIMER_TICK_1US + (cis_conn->ft_peer -1) * cis_conn->iso_intvl_tick;
    #endif

    }
    else //central
    {

    /*
     * For SDUs sent from the Peripheral to the Central in CIS using unframed
     * SDU_Synchronization_Reference = CIS reference anchor point + CIS_Sync_Delay - CIG_Sync_Delay - ((ISO_Interval ÷ SDU_Interval)-1) × SDU_Interva
     */

    #if(ISO_DATA_TIMESTAMP_UNIT_US_EN)
        sduSyncRef = cisSyncRef + cis_conn->cis_sync_delay -cis_conn->cig_sync_delay + (cis_conn->iso_intvl_us/cis_conn->sdu_int_peer_us - 1)*cis_conn->sdu_int_peer_us;
    #else
        sduSyncRef = cisSyncRef + cis_conn->cis_sync_delay*SYSTEM_TIMER_TICK_1US - cis_conn->cig_sync_delay*SYSTEM_TIMER_TICK_1US -\
            (cis_conn->iso_intvl_us/cis_conn->sdu_int_peer_us - 1)*cis_conn->sdu_int_peer_us *SYSTEM_TIMER_TICK_1US;
    #endif
    }

    /*
     * All PDUs belonging to a burst as defined by the configuration of BN have the
     * same reference anchor point. When multiple SDUs have the same reference
     * anchor point, the first SDU uses the reference anchor point timing. Each
     * subsequent SDU increases the SDU synchronization reference timing with one
     * SDU interval
     */
    #if(ISO_DATA_TIMESTAMP_UNIT_US_EN)
        sduSyncRef  += ((pld%cis_conn->bn_peer)/cis_conn->rx_numSdu2Pdu)*cis_conn->sdu_int_peer_us;
    #else
        sduSyncRef  += ((pld%cis_conn->bn_peer)/cis_conn->rx_numSdu2Pdu)*cis_conn->sdu_int_peer_us * SYSTEM_TIMER_TICK_1US;
    #endif

    tlkapi_send_string_u32s(DBG_IAL_EN, "reassem", blt_debug_hex_2_dec_display(pld),
                            blt_debug_hex_2_dec_display(sduSyncRef), cisSyncRef, clock_time());

    return sduSyncRef;
}

/**
 * @brief      This function is used to reassemble PDU to HCI ISO data packet.
 * @param[in]  cis_connHandle
 * @param[in]  pIsoRxEvt event from link lay
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 *                       LL_ERR_INVALID_PARAMETER: command failed
 */
#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
ble_sts_t blt_ial_reassembleCisPdu2Sdu(ll_cis_conn_t *cis_conn, iso_rx_evt_t* pIsoRxEvt)
{
    sdu_packet_t* sdu = blt_isoal_getNextCisRxSdu(cis_conn);

    u8 llid = pIsoRxEvt->llid;
    u8 rf_len = 0;
    rf_packet_ll_data_t *rx_pdu = NULL;

    if(pIsoRxEvt->pCurrIsoRxPdu!=NULL)
    {
        rx_pdu = pIsoRxEvt->pCurrIsoRxPdu;
        rf_len =rx_pdu->llPhysChnPdu.llPduHdr.cisPduHdr.rf_len;
    }

#if(SL16_cis0_rxProPdu)
    log_b16(SL_STACK_CIS_RX_DATA_EN, (SL16_cis0_rxProPdu + ((cis_conn->cisRole)?
            (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))), (u16)pIsoRxEvt->curRcvdPldNum);//+cis_conn->cis_index
#endif

    tlkapi_send_string_u32s(DBG_CIS_RX_DATA, "[CIS RX] PDU re_assemble", 0, 0, 0, 0);

    if(cis_conn->cis_frame == CIS_UNFRAMED)
    {
        ll_iso_unframe_type_t type = blt_ial_getCisUnframedType(cis_conn, pIsoRxEvt, llid);
        tlkapi_send_string_u32s(DBG_IAL_EN, "reassemble SDU", pIsoRxEvt->curRcvdPldNum, rx_pdu, type, cis_conn->cis_rxSduStatus);

        #if(SL16_cis_rxPdu2Sdu_st)
            log_b16_byte(SL_STACK_CIS_RX_DATA_EN, (SL16_cis_rxPdu2Sdu_st + ((cis_conn->cisRole)?
                    (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))), cis_conn->cis_rxSduStatus, type);
        #endif

        switch(cis_conn->cis_rxSduStatus)
        {
            case    SDU_STATE_NEW:
            {
                sdu->iso_sdu_len =  0;
                if((type == UNFRAMED_COMPLETE) || (type == UNFRAMED_START))//UNFRAMED_CONTINUE/UNFRAMED_END
                {
                    sdu->timestamp = blt_ial_getCisUnframedTimestamp(cis_conn, pIsoRxEvt->cisRefAP,pIsoRxEvt->curRcvdPldNum);
                }

                if(type == UNFRAMED_START)
                {
                    if(!pIsoRxEvt->null_flag)
                    {
                        smemcpy((sdu->data+sdu->iso_sdu_len), rx_pdu->llPhysChnPdu.llPayload, rf_len);
                        sdu->iso_sdu_len += rf_len;
                        sdu->pkt_st = HCI_ISO_VALID_DATA;
                    }
                    else
                    {
                        sdu->pkt_st = HCI_ISO_LOST_DATA;
                    }
                    cis_conn->cis_rxSduStatus = SDU_STATE_CONTINUE; //Switch state
                }
                else if(type == UNFRAMED_COMPLETE)
                {
                    if(!pIsoRxEvt->null_flag)
                    {
                        smemcpy(sdu->data, rx_pdu->llPhysChnPdu.llPayload, rf_len);     // copy iso_sdu
                        sdu->iso_sdu_len += rf_len;
                        sdu->pkt_st = HCI_ISO_VALID_DATA;
                    }
                    else
                    {
                        sdu->pkt_st = HCI_ISO_LOST_DATA;
                    }
                    cis_conn->cisSduOut_wptr ++;    //next SDU buff, un_framed
                    cis_conn->cis_rxSduStatus = SDU_STATE_NEW;

                    tlkapi_send_string_u32s(DBG_CIS_RX_DATA, "[CIS RX] push sduOut", pIsoRxEvt->curRcvdPldNum, cis_conn->cisSduOut_wptr, cis_conn->cisSduOut_rptr, cis_conn->cis_rxSduStatus);
                    tlkapi_send_string_u32s(DBG_IAL_EN,"sdu0",pIsoRxEvt->curRcvdPldNum, cis_conn->rx_lastPktSeqNum,sdu->iso_sdu_len, sdu->pkt_seq_num);

                #if(SL16_cis0_rxSdu_len)
                    log_b16(SL_STACK_CIS_RX_DATA_EN, (SL16_cis0_rxSdu_len + ((cis_conn->cisRole)?
                            (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))), (u16)sdu->iso_sdu_len);
                #endif
                #if (SLEV_cis0_sdu_cmplt)
                    log_event_irq(SL_STACK_CIS_RX_DATA_EN, (SLEV_cis0_sdu_cmplt + ((cis_conn->cisRole)?
                            (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))));
                #endif
                }

                sdu->pkt_seq_num = pIsoRxEvt->curRcvdPldNum/cis_conn->rx_numSdu2Pdu;
                break;
            }


            case SDU_STATE_CONTINUE:
            {
                if(type == UNFRAMED_CONTINUE)
                {
                    if((!pIsoRxEvt->null_flag) && (sdu->pkt_st == HCI_ISO_VALID_DATA))
                    {
                        smemcpy((sdu->data + sdu->iso_sdu_len), rx_pdu->llPhysChnPdu.llPayload, rf_len); //copy iso_sdu
                        sdu->iso_sdu_len += rf_len;
                    }
                    else
                    {
                        //discard current pdu
                        sdu->pkt_st = HCI_ISO_LOST_DATA;
                    }
                }
                else if(type == UNFRAMED_END)
                {
                    if((!pIsoRxEvt->null_flag) && (sdu->pkt_st == HCI_ISO_VALID_DATA))
                    {
                        smemcpy((sdu->data + sdu->iso_sdu_len), rx_pdu->llPhysChnPdu.llPayload, rf_len);
                        sdu->iso_sdu_len += rf_len;

                        tlkapi_send_string_u32s(DBG_IAL_EN,"sdu1",pIsoRxEvt->curRcvdPldNum, sdu->pkt_st,sdu->iso_sdu_len,0);
                    }
                    else //discard this PDU
                    {
                        sdu->pkt_st = (sdu->iso_sdu_len)?HCI_ISO_POSSIBLE_INVALID_DATA:HCI_ISO_LOST_DATA;
                        tlkapi_send_string_u32s(DBG_IAL_EN,"sdu2",pIsoRxEvt->curRcvdPldNum, sdu->pkt_st,sdu->iso_sdu_len,0);
                    }
                    cis_conn->cis_rxSduStatus = SDU_STATE_NEW; //switch stated
                    cis_conn->cisSduOut_wptr ++;    //next SDU buff, un_framed

                    #if(SL16_cis0_rxSdu_len)
                        log_b16(SL_STACK_CIS_RX_DATA_EN, (SL16_cis0_rxSdu_len + ((cis_conn->cisRole)?
                                (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))), (u16)sdu->iso_sdu_len);
                    #endif
                    #if (SLEV_cis0_sdu_cmplt)
                        log_event_irq(SL_STACK_CIS_RX_DATA_EN, (SLEV_cis0_sdu_cmplt + ((cis_conn->cisRole)?
                                (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))));
                    #endif
                }
                else//start, complete
                {
                    cis_conn->cis_rxSduStatus = SDU_STATE_NEW; //switch stated
                }
                break;

            }
            default:
                break;
        }
    }
    else //framed packet
    {
        if(pIsoRxEvt != NULL)
        {
            u8 *segData;
            u8 segDataLen = 0, offset = 0;
            int remainLen = rx_pdu->llPhysChnPdu.llPduHdr.pduHdr.rf_len;

            if(pIsoRxEvt->pCurrIsoRxPdu==NULL){
                return (ble_sts_t)LL_CIS_RX_EVT_BUF_PARAM_INVALID;
            }

            while(remainLen > 0)
            {
                u8 skip_flag =0;
                iso_framed_segmHdr_t *segHdr = (iso_framed_segmHdr_t*)(rx_pdu->llPhysChnPdu.llPayload + offset);
                segDataLen = segHdr->length;
                tlkapi_send_string_u32s(DBG_IAL_EN, "IAL Framed PDU", pIsoRxEvt->curRcvdPldNum, segHdr->sc, segHdr->cmplt, cis_conn->cis_rxSduStatus);

                #if(SL16_cis_rxPdu2Sdu_st)
                    log_b16_byte(SL_STACK_CIS_RX_DATA_EN, SL16_cis_rxPdu2Sdu_st, cis_conn->cis_rxSduStatus, (segHdr->sc | (segHdr->cmplt<<4)));
                #endif

                if( (remainLen < segHdr->length+2)){
                    pIsoRxEvt = NULL;

                    tlkapi_send_string_u32s(DBG_IAL_EN, "discard this PUD", pIsoRxEvt->curRcvdPldNum, rf_len, segHdr->length+2,0);
                    goto fun_pdu_error;
                }

                switch(cis_conn->cis_rxSduStatus)
                {
                    case    SDU_STATE_NEW:
                    {
                        if((segHdr->sc ==0))//start/complete
                        {
                            if(gIsoTsEn)
                            {
                            #if(ISO_DATA_TIMESTAMP_UNIT_US_EN)
                                sdu->timestamp = pIsoRxEvt->cisRefAP + cis_conn->cis_sync_delay + cis_conn->sdu_int_peer_us
                                                    +cis_conn->ft_peer * cis_conn->iso_intvl_us -segHdr->time_offset;
                            #else
                                sdu->timestamp = pIsoRxEvt->cisRefAP + cis_conn->cis_sync_delay*SYSTEM_TIMER_TICK_1US +\
                                        cis_conn->sdu_int_peer_us*SYSTEM_TIMER_TICK_1US  + cis_conn->ft_peer *         \
                                        cis_conn->iso_intvl_tick - segHdr->time_offset*SYSTEM_TIMER_TICK_1US;
                            #endif
                            }
                            sdu->iso_sdu_len = 0;
                            //copy data from pdu to SDU
                            segDataLen -= ISO_FRAMED_TIMEOFFSET_LEN; // segmentation payload data len
                            segData = rx_pdu->llPhysChnPdu.llPayload + offset + ISO_FRAMED_SEGM_HEADER_LEN + ISO_FRAMED_TIMEOFFSET_LEN;
                            smemcpy((sdu->data), segData, segDataLen);
                            sdu->iso_sdu_len += segDataLen;
                            offset += (segHdr->length + ISO_FRAMED_SEGM_HEADER_LEN);
                            remainLen -= (segHdr->length + ISO_FRAMED_SEGM_HEADER_LEN);
                            sdu->pkt_st = HCI_ISO_VALID_DATA;
                            cis_conn->lossFlag = 0;
                            skip_flag = 0;

                            tlkapi_send_string_u32s(DBG_IAL_EN, "IAL Framed New1", pIsoRxEvt->curRcvdPldNum, segDataLen, cis_conn->lossFlag, skip_flag);

                        }
                        else // ERROR! continue/End
                        {
                            //DIscard this segmentation
                            offset += segHdr->length + ISO_FRAMED_SEGM_HEADER_LEN;
                            remainLen -= segDataLen + ISO_FRAMED_SEGM_HEADER_LEN;

                            if(!cis_conn->lossFlag)// PDU(Start X, End X, Start X),,,,,,  PDU(Continue)
                            {
                                cis_conn->lossFlag = 1;
                                if(gIsoTsEn)
                                {
                                #if(ISO_DATA_TIMESTAMP_UNIT_US_EN)
                                    sdu->timestamp = pIsoRxEvt->cisRefAP + cis_conn->cis_sync_delay + cis_conn->sdu_int_peer_us
                                                        +cis_conn->ft_peer * cis_conn->iso_intvl_us;
                                #else
                                    sdu->timestamp = pIsoRxEvt->cisRefAP + cis_conn->cis_sync_delay*SYSTEM_TIMER_TICK_1US +\
                                            cis_conn->sdu_int_peer_us*SYSTEM_TIMER_TICK_1US  + cis_conn->ft_peer * cis_conn->iso_intvl_tick;
                                #endif
                                }
                                sdu->iso_sdu_len = 0;
                                //packet sequence number
                                cis_conn->rx_lastPktSeqNum ++;
                                sdu->pkt_seq_num = cis_conn->rx_lastPktSeqNum;
                                sdu->pkt_st = HCI_ISO_LOST_DATA;
                                cis_conn->cisSduOut_wptr ++; //end SDU
                                #if(SL16_cis0_rxSdu_len)
                                    log_b16(SL_STACK_CIS_RX_DATA_EN, (SL16_cis0_rxSdu_len + ((cis_conn->cisRole)?
                                            (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))), (u16)sdu->iso_sdu_len);
                                #endif
                                #if (SLEV_cis0_sdu_cmplt)
                                    log_event_irq(SL_STACK_CIS_RX_DATA_EN, (SLEV_cis0_sdu_cmplt + ((cis_conn->cisRole)?
                                            (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))));
                                #endif
                                sdu = blt_isoal_getNextCisRxSdu(cis_conn);
                                sdu->iso_sdu_len =0;
                            }
                            //todo Loss SDU
                            skip_flag = 1;
                            tlkapi_send_string_u32s(DBG_IAL_EN, "IAL Framed New2", pIsoRxEvt->curRcvdPldNum, segDataLen, cis_conn->lossFlag, skip_flag);
                        }

                        break;
                    }

                    case    SDU_STATE_CONTINUE:
                    {
                        if(segHdr->sc ==0)// ERROR!!! start/complete
                        {
                            //finish last sdu, and continue current sdu
                            sdu->pkt_st = (sdu->iso_sdu_len)?HCI_ISO_POSSIBLE_INVALID_DATA : HCI_ISO_LOST_DATA;
                            //set packet_sequence_num
                            cis_conn->rx_lastPktSeqNum ++;
                            sdu->pkt_seq_num = cis_conn->rx_lastPktSeqNum;
                            cis_conn->cisSduOut_wptr ++; //finish last SDU
                            #if(SL16_cis0_rxSdu_len)
                                log_b16(SL_STACK_CIS_RX_DATA_EN, (SL16_cis0_rxSdu_len + ((cis_conn->cisRole)?
                                        (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))), (u16)sdu->iso_sdu_len);
                            #endif
                            #if (SLEV_cis0_sdu_cmplt)
                                log_event_irq(SL_STACK_CIS_RX_DATA_EN, (SLEV_cis0_sdu_cmplt + ((cis_conn->cisRole)?
                                        (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))));
                            #endif
                            /*************Get New SDU*********************/
                            sdu = blt_isoal_getNextCisRxSdu(cis_conn);
                            if(gIsoTsEn)
                            {
                                #if(ISO_DATA_TIMESTAMP_UNIT_US_EN)
                                    sdu->timestamp = pIsoRxEvt->cisRefAP + cis_conn->cis_sync_delay + cis_conn->sdu_int_peer_us
                                                  +cis_conn->ft_peer * cis_conn->iso_intvl_us - segHdr->time_offset;
                                #else
                                    sdu->timestamp = pIsoRxEvt->cisRefAP + cis_conn->cis_sync_delay*SYSTEM_TIMER_TICK_1US +\
                                            cis_conn->sdu_int_peer_us*SYSTEM_TIMER_TICK_1US  + cis_conn->ft_peer *         \
                                            cis_conn->iso_intvl_tick - segHdr->time_offset*SYSTEM_TIMER_TICK_1US;
                                #endif
                            }

                            sdu->iso_sdu_len = 0;
                            segDataLen -= ISO_FRAMED_TIMEOFFSET_LEN; // segmentation payload data len
                            segData = rx_pdu->llPhysChnPdu.llPayload + offset + ISO_FRAMED_SEGM_HEADER_LEN + ISO_FRAMED_TIMEOFFSET_LEN;
                            smemcpy((sdu->data), segData, segDataLen);

                            sdu->iso_sdu_len += segDataLen;
                            offset += segHdr->length + ISO_FRAMED_SEGM_HEADER_LEN;
                            remainLen -= segDataLen + ISO_FRAMED_SEGM_HEADER_LEN + ISO_FRAMED_TIMEOFFSET_LEN;
                            sdu->pkt_st = HCI_ISO_VALID_DATA;
                            cis_conn->cis_rxSduStatus = SDU_STATE_NEW;

                            cis_conn->lossFlag = 0;
                            skip_flag = 0;

                            tlkapi_send_string_u32s(DBG_IAL_EN, "IAL Framed Continue1", pIsoRxEvt->curRcvdPldNum, segDataLen, cis_conn->lossFlag, skip_flag);
                        }
                        else //end/continue
                        {
                            segData = rx_pdu->llPhysChnPdu.llPayload + offset + ISO_FRAMED_SEGM_HEADER_LEN;
                            smemcpy((sdu->data + sdu->iso_sdu_len), segData, segDataLen);

                            sdu->iso_sdu_len += segDataLen;
                            offset += segHdr->length + ISO_FRAMED_SEGM_HEADER_LEN;
                            remainLen -= segDataLen + ISO_FRAMED_SEGM_HEADER_LEN;

                            sdu->pkt_st = HCI_ISO_VALID_DATA;

                            skip_flag = 0;
                            cis_conn->lossFlag = 0;

                            tlkapi_send_string_u32s(DBG_IAL_EN, "IAL Framed Continue2", pIsoRxEvt->curRcvdPldNum, segDataLen, cis_conn->lossFlag, skip_flag);
                        }
                        break;
                    }

                    default:
                    {
                        skip_flag = 1;
                        break;
                    }
                }

                if(skip_flag==0)
                {
                    if(segHdr->cmplt==1)
                    {

                        //set packet_sequence_num
                        cis_conn->rx_lastPktSeqNum ++;
                        sdu->pkt_seq_num = cis_conn->rx_lastPktSeqNum;

                        cis_conn->cisSduOut_wptr ++; //end SDU

                        #if(SL16_cis0_rxSdu_len)
                            log_b16(SL_STACK_CIS_RX_DATA_EN, (SL16_cis0_rxSdu_len + ((cis_conn->cisRole)?
                                    (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))), (u16)sdu->iso_sdu_len);
                        #endif
                        #if (SLEV_cis0_sdu_cmplt)
                            log_event_irq(SL_STACK_CIS_RX_DATA_EN, (SLEV_cis0_sdu_cmplt + ((cis_conn->cisRole)?
                                    (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))));
                        #endif
                        tlkapi_send_string_u32s(DBG_IAL_EN, "framed PDU2SDU", pIsoRxEvt->curRcvdPldNum, cis_conn->cisSduOut_wptr, rx_pdu->llPhysChnPdu.llPduHdr.pduHdr.rf_len, cis_conn->cis_rx_stream_start);

                        /*********************Get next sdu buff*****************************/
                        sdu = blt_isoal_getNextCisRxSdu(cis_conn);
                        sdu->iso_sdu_len =0;
                        cis_conn->cis_rxSduStatus = SDU_STATE_NEW;
                    }
                    else
                    {
                        cis_conn->cis_rxSduStatus = SDU_STATE_CONTINUE;
                    }
                }
            }
            return BLE_SUCCESS;
        }


        fun_pdu_error:
        {
            if(cis_conn->cis_rxSduStatus == SDU_STATE_NEW)//start/complete
            {
                if(!cis_conn->lossFlag)
                {
                    cis_conn->lossFlag = 1;
                    /*invalid sdu(Part(s) of the ISO_SDU were not received correctly. This is reported as"lost data".)*/
                    if(gIsoTsEn)
                    {
                        #if(ISO_DATA_TIMESTAMP_UNIT_US_EN)
                            sdu->timestamp = pIsoRxEvt->cisRefAP + cis_conn->cis_sync_delay + cis_conn->sdu_int_peer_us
                                            +cis_conn->ft_peer * cis_conn->iso_intvl_us;
                        #else
                            sdu->timestamp = pIsoRxEvt->cisRefAP + cis_conn->cis_sync_delay*SYSTEM_TIMER_TICK_1US +\
                                    cis_conn->sdu_int_peer_us*SYSTEM_TIMER_TICK_1US  + cis_conn->ft_peer *cis_conn->iso_intvl_tick;
                        #endif
                    }
                    sdu->iso_sdu_len = 0;
                    //packet sequence number
                    cis_conn->rx_lastPktSeqNum ++;
                    sdu->pkt_seq_num = cis_conn->rx_lastPktSeqNum;
                    sdu->pkt_st = HCI_ISO_LOST_DATA;
                    cis_conn->cisSduOut_wptr++;// next SDU buff

                    #if(SL16_cis0_rxSdu_len)
                        log_b16(SL_STACK_CIS_RX_DATA_EN, (SL16_cis0_rxSdu_len + ((cis_conn->cisRole)?
                                (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))), (u16)sdu->iso_sdu_len);
                    #endif
                    #if (SLEV_cis0_sdu_cmplt)
                        log_event_irq(SL_STACK_CIS_RX_DATA_EN, (SLEV_cis0_sdu_cmplt + ((cis_conn->cisRole)?
                                (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))));
                    #endif
                    //get next sdu buff
                    sdu = blt_isoal_getNextCisRxSdu(cis_conn);
                    sdu->iso_sdu_len = 0;
                }
            }
            else //SDU_STATE_CONTINUE
            {
                cis_conn->lossFlag = 1;
                //finish last sdu, and continue current sdu
                sdu->pkt_st = (sdu->iso_sdu_len)?HCI_ISO_POSSIBLE_INVALID_DATA : HCI_ISO_LOST_DATA;
                //set packet_sequence_num
                cis_conn->rx_lastPktSeqNum ++;
                sdu->pkt_seq_num = cis_conn->rx_lastPktSeqNum;
                cis_conn->cisSduOut_wptr ++; //finish last SDU
                #if(SL16_cis0_rxSdu_len)
                    log_b16(SL_STACK_CIS_RX_DATA_EN, (SL16_cis0_rxSdu_len + ((cis_conn->cisRole)?
                            (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))), (u16)sdu->iso_sdu_len);
                #endif
                #if (SLEV_cis0_sdu_cmplt)
                    log_event_irq(SL_STACK_CIS_RX_DATA_EN, (SLEV_cis0_sdu_cmplt + ((cis_conn->cisRole)?
                            (cis_conn->cis_index): (cis_conn->cis_index - bltCisMng.maxNum_cisMaster))));
                #endif
                //get next sdu buff
                sdu = blt_isoal_getNextCisRxSdu(cis_conn);
                sdu->iso_sdu_len = 0;
                cis_conn->cis_rxSduStatus = SDU_STATE_NEW;
            }
        }
    }
    return BLE_SUCCESS;
}







#endif


/*********************************************************************************************************
 * CIS Function end
 *********************************************************************************************************/

































/*********************************************************************************************************
 * BIS Function start
 *********************************************************************************************************/
#if (LL_FEATURE_ENABLE_CONNECTIONLESS_ISO)





static int blt_ial_bis_setPldNum(ll_bis_t *pBis, bis_tx_pdu_t *pdu)
{
    u8 ret = 0;
    ll_big_bcst_t *pBig =(ll_big_bcst_t*) (global_pBigBcst + pBis->big_idx);
    (void)pBig; //remove compiler warning
    tlkapi_send_string_u32s(DEBUG_PAYLOAD_NUM,"IAL setPLD", pBis->txBnIdx,pBis->txSduIdx, 0,0);
    if(((pBis->tx_first_pdu==1) )|| ((pBis->txBnIdx==1))){

        pBis->tx_first_pdu = 0;
        if(pBis->curBisPldNum >= pBis->lastPayloadNum)
        {
            pdu->payloadNumber =  (pBis->curBisPldNum/pBis->numSdu2Pdu + 1) * pBis->numSdu2Pdu;
        }
        else
        {
            pdu->payloadNumber =  (pBis->lastPayloadNum/pBis->numSdu2Pdu + 1) * pBis->numSdu2Pdu;
        }
        tlkapi_send_string_u32s(DEBUG_PAYLOAD_NUM ,"IAL SetPldNumStart", pdu->payloadNumber,pBig->bigEventCnt, pBis->curBisPldNum, pBis->lastPayloadNum);
    }
    else{
        pdu->payloadNumber = pBis->lastPayloadNum + 1;

        tlkapi_send_string_u32s(DEBUG_PAYLOAD_NUM ,"IAL SetPldNumConti",pdu->payloadNumber,pBig->bigEventCnt,pBis->curBisPldNum,pBis->lastPayloadNum);
    }

    if(pdu->payloadNumber < pBis->curBisPldNum  || (pdu->payloadNumber<= pBis->lastPayloadNum))
    {

        tlkapi_send_string_u32s(DEBUG_PAYLOAD_NUM, "IAL SetPldNum Error !!!", pdu->payloadNumber,pBig->bigEventCnt,pBis->curBisPldNum,pBis->lastPayloadNum);
        //if the PDU not the first in SDU, so discard this PDU.
//      if(pBis->txBnIdx==1)
//      {
//          pdu->eventCnt = (pdu->eventCnt+1);
//          pdu->payloadNumber = pdu->eventCnt *pBig->bn;
//          pBis->txSduIdx = 1;
//      }
//      else
//      {
//          ret = 1;
//
//          tlkapi_send_string_data(DBG_IAL_EN ,"IAL Discard this PDU", 0, 0);
//      }
    }
    pBis->lastPayloadNum = pdu->payloadNumber;

    return ret;
}


#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
void blt_ll_bis_encryptPld(ble_crypt_para_t *pLeCryptCtrl , rf_packet_ll_data_t *pBisPdu, u64 txPayloadCnt)
{
    /*
     * The value of packetCounter used for a BIG Control PDU shall be equal to the
     * value of packetCounter used in the first subevent of the BIG event that the BIG
     * Control PDU is part of.
     */
    rf_bis_data_hdr_t* pBisPduHdr = &pBisPdu->llPhysChnPdu.llPduHdr.bisPduHdr;
    u8* pRfLen = &pBisPduHdr->rf_len;

    //DBG_C HN11_TOGGLE;
    if( 1 && (pBisPduHdr->rf_len > 0) && pLeCryptCtrl->enable ){
        /* AES_CCM_Encryption in IRQ, AES_CCM_Decryption in main_loop maybe overlap!!! (IRQ protect)
        It's best to add protection, safety : save AES_CCM settings */
        ble_crypt_para_t bisCryptCtrlBackUp = *pLeCryptCtrl;

        pLeCryptCtrl->enc_pno = txPayloadCnt;
        //tlkapi_send_string_data(0,"raw ISO pkt", (u8*)pBisPdu, pLeCryptCtrl->pllPhysChnPdu->llPduHdr.pduHdr.rf_len+2);

        /* The directionBit shall be set to 1 for Broadcast Isochronous PDUs sent by the Isochronous Broadcaster. */
        
        
        //TODO:  TO DO IT LATTER: There is a risk here, and the encryption can be handled in the loop, so that the aes reentrancy problem can be fixed more thoroughly
        aes_ll_ccm_encryption(&pBisPdu->llPhysChnPdu, 1, CRYPT_NONCE_TYPE_BIS, pLeCryptCtrl);
        
        //tlkapi_send_string_data(0,"enc ISO pkt", (u8*)pBisPdu, pLeCryptCtrl->pllPhysChnPdu->llPduHdr.pduHdr.rf_len+2+4);
        /* AES_CCM_Encryption in IRQ, AES_CCM_Decryption in main_loop maybe overlap!!! (IRQ protect)
        It's best to add protection, safety : restore AES_CCM settings */
        *pLeCryptCtrl = bisCryptCtrlBackUp;
    }

    pBisPdu->dma_len = rf_tx_packet_dma_len(*pRfLen + 2);

    //DBG_C HN11_TOGGLE;
}

/**
 * @brief      This function is used to segmentation/fragmentation SDU to one or more Framed/Unframed PDUs.
 * @param[in]  cis_connHandle
 * @param[in]  sdu  point to sdu buff
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 *                       LL_ERR_INVALID_PARAMETER: command failed
 */
#if (SUB_INTERVAL_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
ble_sts_t blt_bis_splitSdu2UnframedPdu(u16 bisHandle, sdu_packet_t *sdu, u8 *pNumOfCmpPkt)
{

    bis_tx_pdu_t *pdu;
    rf_packet_ll_data_t *pRfPdu=NULL;



    u8 bis_sel = bisHandle & BLT_BIS_IDX_MSK;
    ll_bis_t *pBis = (ll_bis_t *)(global_pBis + bis_sel);
    ll_big_bcst_t *pBig =(ll_big_bcst_t*) (global_pBigBcst + pBis->big_idx);


    if((pBig->max_sdu==0) || (pBig->max_pdu==0)){

        if(sdu->numOfCmplt_en){
            (*pNumOfCmpPkt) += sdu->numHciPkt;
        }
        return LL_ERR_INVALID_PARAMETER;
    }

    if(sdu->iso_sdu_len > pBig->max_sdu){

        if(sdu->numOfCmplt_en){
            (*pNumOfCmpPkt) += sdu->numHciPkt;
        }
        return IAL_ERR_SDU_LEN_EXCEED_SDU_MAX;
    }



    if(pBig->framing == 0)//unframed
    {
//      tlkapi_send_string_data(0, "@IAL_SDU_DATA in", sdu->data , sdu->iso_sdu_len );

        tlkapi_send_string_data(DEBUG_PAYLOAD_NUM, "@IAL_SDU_DATA in", &sdu->iso_sdu_len , 2);

        u16 offset = 0;
        u16 remain = sdu->iso_sdu_len ;
        u16 cur_send_pdu_len = 0;

        // check if have enough fifo to fill one SDU
        if(((pBis->bisPduTxFifoWptr - pBis->bisPduTxFifoRptr)&(bltBisPduTxfifo.mask)) + pBis->numSdu2Pdu >= bltBisPduTxfifo.fifo_num)
            return LL_ERR_TX_FIFO_NOT_ENOUGH;


        pBis->txBnIdx = 0;
        pBis->txSduIdx ++;
        if(pBis->txSduIdx > pBis->numSduEachEvent)
        {
            pBis->txSduIdx = 1;
        }

        while(offset < sdu->iso_sdu_len )
        {
            remain = remain - cur_send_pdu_len;
            cur_send_pdu_len = min(remain, pBig->max_pdu);


            pdu = (bis_tx_pdu_t*)(((u32)bltBisPduTxfifo.bis_tx_pdu) + (bis_sel*bltBisPduTxfifo.fifo_num + \
                            (pBis->bisPduTxFifoWptr&(bltBisPduTxfifo.mask)))*bltBisPduTxfifo.full_size);
            pRfPdu = &pdu->isoTxPdu;
            //pRfPdu->llPhysChnPdu.llPduHdr.pduHdr.rf_len = 0; //clean
            pRfPdu->llPhysChnPdu.llPduHdr.pduHdr.type = 0; //clean
            pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.llid = ((offset+ cur_send_pdu_len)>= sdu->iso_sdu_len )? \
                            ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU:ISO_LLID_UNFRAMED_PDU_START_CONTI_FRAGMENT_SDU;
            pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.cssn = 0;
            pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.cstf = 0;
            pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len = cur_send_pdu_len;
            pBis->txBnIdx ++;

            smemcpy(pRfPdu->llPhysChnPdu.llPayload, (sdu->data +offset), cur_send_pdu_len);
            pRfPdu->dma_len = rf_tx_packet_dma_len(cur_send_pdu_len + 2);

            if(blt_ial_bis_setPldNum(pBis, pdu)){

                if(sdu->numOfCmplt_en){
                    (*pNumOfCmpPkt) += sdu->numHciPkt;
                }
                return IAL_ERR_EVENT_PASSED;
            }

            if((pBis->pBisTestParam!=NULL)&&(pBis->pBisTestParam->isoTestMode==ISO_TEST_TRANSMIT_MODE) && (offset==0)){

                u32 packt_cnt =  pdu->payloadNumber/pBis->numSdu2Pdu;
                pRfPdu->llPhysChnPdu.llPayload[0] = packt_cnt & 0xff;
                pRfPdu->llPhysChnPdu.llPayload[1] = (packt_cnt>>8)  & 0xff;
                pRfPdu->llPhysChnPdu.llPayload[2] = (packt_cnt>>16) & 0xff;
                pRfPdu->llPhysChnPdu.llPayload[3] = (packt_cnt>>24) & 0xff;
            }

            offset += cur_send_pdu_len;

            #if(0)
                static u16 debug_cnt = 0;
                 debug_cnt++;
                 pRfPdu->llPhysChnPdu.llPayload[0] = debug_cnt & 0xff;
                 pRfPdu->llPhysChnPdu.llPayload[1] = (debug_cnt>>8)&0xff;
                 pRfPdu->llPhysChnPdu.llPayload[2] = pdu->bisPayloadNumber & 0xff;
                 pRfPdu->llPhysChnPdu.llPayload[3] = (pdu->bisPayloadNumber>>8) & 0xff;
                 pRfPdu->llPhysChnPdu.llPayload[4] = (pdu->bisPayloadNumber>>16) & 0xff;
                 pRfPdu->llPhysChnPdu.llPayload[5] = (pdu->bisPayloadNumber>>24) & 0xff;

                 pRfPdu->llPhysChnPdu.llPayload[6] = cis_conn->cis SendPldNum & 0xff;
                 pRfPdu->llPhysChnPdu.llPayload[7] = (cis_conn->cis SendPldNum>>8) & 0xff;
                 pRfPdu->llPhysChnPdu.llPayload[8] = (cis_conn->cis SendPldNum>>16) & 0xff;
                 pRfPdu->llPhysChnPdu.llPayload[9] = (cis_conn->cis SendPldNum>>24) & 0xff;
            #endif


            tlkapi_send_string_data(0, "       @ISO_PDU_Data tx*", pRfPdu->llPhysChnPdu.llPayload, 2);//cur_send_pdu_len);

            #if(HW_AES_CCM_ALG_EN)
                if(pBis->bisCryptCtrl.enable && pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len){
                    pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len +=4;
                }
            #else
                blt_ll_bis_encryptPld(&pBis->bisCryptCtrl, &pdu->isoTxPdu, pdu->payloadNumber);
            #endif

            pBis->bisPduTxFifoWptr++;
            ll_big_bcst_t *pBigTmp =(ll_big_bcst_t*) (global_pBigBcst + pBis->big_idx);
            (void)pBigTmp; //remove compiler warning
            tlkapi_send_string_u32s(DEBUG_PAYLOAD_NUM, "PB", blt_debug_hex_2_dec_display(pdu->payloadNumber), pBigTmp->bigEventCnt, cur_send_pdu_len, offset);

        }

        /*
         * Each SDU shall generate BN(SDU_IntervalISO_Interval) fragments. All
         *these fragments shall be sent to the Link Layer before any fragments of the
         *next SDU. If an SDU generates less than this number of fragments, empty
         *payloads shall be used to make up the number. Empty payloads shall be PDUs
         *with LLID 0b01 with zero length payload
         */
        for(u8 sn= (pBis->txBnIdx+1); sn<=pBis->numSdu2Pdu; sn++)
        {
            pBis->txBnIdx ++;
            /*Get pdu fifo*/
            pdu =(bis_tx_pdu_t*)(((u8*)bltBisPduTxfifo.bis_tx_pdu) + (bis_sel*bltBisPduTxfifo.fifo_num + (pBis->bisPduTxFifoWptr&(bltBisPduTxfifo.mask)))*bltBisPduTxfifo.full_size);
            pRfPdu = &pdu->isoTxPdu;

            tlkapi_send_string_u32s(0, "ial split", sn, pBis->txBnIdx, sdu->iso_sdu_len , 0);
            /****************************fit BIS PDU*********************************************/
            // if sdu len=0, first packet llid= 0b00,others padding llid = 0b01
            //pRfPdu->llPhysChnPdu.llPduHdr.pduHdr.rf_len = 0; //clean
            pRfPdu->llPhysChnPdu.llPduHdr.pduHdr.type = 0; //clean
            pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.llid = (sn<=1)? ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU \
                                                                    :ISO_LLID_UNFRAMED_PDU_START_CONTI_FRAGMENT_SDU;
            pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.cssn = 0;
            pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.cstf = 0;
            pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len = 0;
            pdu->isoTxPdu.dma_len = rf_tx_packet_dma_len(2);

            blt_ial_bis_setPldNum(pBis, pdu);
            tlkapi_send_string_data(0, "       @ISO_PDU_Data tx**", pRfPdu->llPhysChnPdu.llPayload, cur_send_pdu_len);
            pBis->bisPduTxFifoWptr ++;

        }
    }

    pBis->lastPktSeqNum = sdu->pkt_seq_num;

    if(sdu->numOfCmplt_en){
        (*pNumOfCmpPkt) += sdu->numHciPkt;
    }

    return BLE_SUCCESS;
}

/**
 * @brief   This function is used to segmentation process splits an SDU into one or more segments which
 *          are carried by one or more framed PDUs in a Broadcast Isochronous Stream.
 * @param[in]  BIS handle
 * @param[in]  sdu  point to sdu buff
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 *                       LL_ERR_INVALID_PARAMETER: command failed
 */
#if (SUB_INTERVAL_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
ble_sts_t blt_bis_splitSdu2FramedPdu(u16 bisHandle, u8*pNumOfCmpPkt)
{

    u16 remain_len;
    bis_tx_pdu_t *pdu=NULL;
    rf_packet_ll_data_t *pRfPdu=NULL;
    sdu_packet_t* sdu = NULL;

    u8 segmHdrLen = ISO_FRAMED_SEGM_HEADER_LEN;

    u8 bis_sel = bisHandle & BLT_BIS_IDX_MSK;
    ll_bis_t *pBis = (ll_bis_t *)(global_pBis + bis_sel);
    ll_big_bcst_t *pBig =(ll_big_bcst_t*) (global_pBigBcst + pBis->big_idx);


    /***********************Get SDU *************************************/
    sdu = (sdu_packet_t*)(pBis->bis_sduInBuf + sduBisMng.max_in_fifo_size * \
                                ((pBis->bisSduIn_rptr & sduBisMng.in_fifo_mask)));

    // cis channel parameter not allow transmit data
    if((pBig->max_sdu==0) || (pBig->max_pdu==0))
    {
        if(sdu->numOfCmplt_en){
            (*pNumOfCmpPkt) += sdu->numHciPkt;
        }
        return LL_ERR_INVALID_PARAMETER;
    }



    /**********************Get pdu fifo************************************/
    pdu =(bis_tx_pdu_t*)(((u8*)bltBisPduTxfifo.bis_tx_pdu) + (bis_sel*bltBisPduTxfifo.fifo_num + (pBis->bisPduTxFifoWptr&(bltBisPduTxfifo.mask)))*bltBisPduTxfifo.full_size);
    pRfPdu = &pdu->isoTxPdu;

    pdu->offset = 0;
    pRfPdu->llPhysChnPdu.llPduHdr.pduHdr.rf_len = 0; //clean
    pRfPdu->llPhysChnPdu.llPduHdr.pduHdr.type = 0; //clean
    pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.llid = ISO_LLID_FRAMED_PDU_SEGMENT_SDU;
    pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.cssn = 0;
    pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.cstf = 0;
    iso_framed_segmHdr_t segmHdr= {0};


    tlkapi_send_string_u32s(DEBUG_PAYLOAD_NUM, "@IAL_SDU_DATA in'", sdu->iso_sdu_len, sdu->sduOffset,pBig->max_sdu,0);

    if(sdu->iso_sdu_len > pBig->max_sdu)
    {
        pBis->bisSduIn_rptr++; //deserted this SDU
        pBis->bisSduInFreeNum++;

        if(sdu->numOfCmplt_en){
            (*pNumOfCmpPkt) += sdu->numHciPkt;
        }
        return IAL_ERR_SDU_LEN_EXCEED_SDU_MAX;
    }

    pdu->payloadNumber = (pBis->lastPayloadNum >= pBis->curBisPldNum)?(pBis->lastPayloadNum+1):(pBis->curBisPldNum+1);
    tlkapi_send_string_u32s(DEBUG_PAYLOAD_NUM, "framed pld", pBis->lastPayloadNum, pBis->curBisPldNum, pdu->payloadNumber, 0);
    pBis->lastPayloadNum = pdu->payloadNumber;


    //todo if current PDU not full, next SDU will fill data in it, but next SDU fill it after Current CIS, so time_offset calculate if empty
    u8 segmHdrOffset = 0;
    remain_len = pBig->max_pdu;

    while(remain_len)
    {

        /******************fit segmentation header*********************/
        if((pdu->offset==0) || (sdu->sduOffset==0))
        {
            //Start or continue
            if(sdu->sduOffset==0)
            {
                segmHdr.sc = 0;

                segmHdr.time_offset = 0x1122;
                segmHdr.length = ISO_FRAMED_TIMEOFFSET_LEN;

                segmHdrLen = ISO_FRAMED_SEGM_HEADER_LEN +ISO_FRAMED_TIMEOFFSET_LEN; //add timeoffset(3bytes)

                if(remain_len < segmHdrLen)
                {
                    tlkapi_send_string_data(DBG_IAL_EN, "      @ISO_PDU_Data tx'", &pdu->isoTxPdu, pdu->isoTxPdu.llPhysChnPdu.llPduHdr.bisPduHdr.rf_len);
                    blt_ll_bis_encryptPld(&pBis->bisCryptCtrl, &pdu->isoTxPdu, pdu->payloadNumber);
                    pBis->bisPduTxFifoWptr ++;// PDU finalize

                    tlkapi_send_string_u32s(DBG_IAL_EN,"PDU finalize",pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len,pdu->payloadNumber,0,0);
                    break;
                }

                tlkapi_send_string_data(DBG_IAL_EN,"********start**********",0,0);
            }
            else
            {
                segmHdr.sc = 1;
                segmHdr.length = 0;

                tlkapi_send_string_data(DBG_IAL_EN,"********Continue**********",0,0);

                segmHdrLen = ISO_FRAMED_SEGM_HEADER_LEN; //exclude time offset
            }


            smemcpy((pRfPdu->llPhysChnPdu.llPayload + pdu->offset), &segmHdr, segmHdrLen);
            pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len += segmHdrLen;
            segmHdrOffset = pdu->offset;//the position of the segmentation header in PDU

            pdu->offset += segmHdrLen;
            remain_len -= segmHdrLen;
        }

        // if PDU can't fit at least one byte effective byte, finish this PDU.
        if(remain_len < 1)
        {

            tlkapi_send_string_data(DBG_IAL_EN, "      @ISO_PDU_Data tx''", &pdu->isoTxPdu, pdu->isoTxPdu.llPhysChnPdu.llPduHdr.bisPduHdr.rf_len);
            blt_ll_bis_encryptPld(&pBis->bisCryptCtrl, &pdu->isoTxPdu, pdu->payloadNumber);
            pBis->bisPduTxFifoWptr ++;// PDU finalize

            tlkapi_send_string_u32s(DBG_IAL_EN,"PDU finalize",pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len,pdu->payloadNumber,0,0);
            break;
        }

        /*******************fit segmentation of payload***************************************/
        u8 slen=0;

        tlkapi_send_string_u32s(DBG_IAL_EN,"remain_len",remain_len,sdu->iso_sdu_len,sdu->sduOffset,sdu->iso_sdu_len - sdu->sduOffset);

        if(remain_len >= (sdu->iso_sdu_len - sdu->sduOffset))
        {
            slen = sdu->iso_sdu_len - sdu->sduOffset;
            segmHdr.cmplt = 1;

            // Framed PDU reassemble
            pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len += slen;
            smemcpy((pRfPdu->llPhysChnPdu.llPayload + pdu->offset), (sdu->data + sdu->sduOffset), slen);
            pRfPdu->dma_len = rf_tx_packet_dma_len(pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len +2);
            pdu->offset += slen;
            remain_len -= slen; //PDU remain
            segmHdr.length += slen;
            smemcpy((pRfPdu->llPhysChnPdu.llPayload + segmHdrOffset), &segmHdr, segmHdrLen);
            pBis->bisSduIn_rptr++;
            pBis->bisSduInFreeNum++;

            //get next SDU to fit this PDU
            if((pBis->bisSduIn_rptr != pBis->bisSduIn_wptr)&&(remain_len > 5))
            {
                //get next sdu
                sdu = (sdu_packet_t*)(pBis->bis_sduInBuf + sduBisMng.max_in_fifo_size * \
                                            ((pBis->bisSduIn_rptr & sduBisMng.in_fifo_mask)));

                if(sdu == NULL)
                {
                    pBis->bisSduIn_rptr++; //deserted this SDU
                    return IAL_ERR_SDU_BUFF_INVALID;
                }

                tlkapi_send_string_u32s(DBG_IAL_EN, "ial split wptr != rptr", pBis->bisSduIn_rptr, pBis->bisSduIn_wptr, 0,0);
                 //get iso_data_load point

                if(sdu->iso_sdu_len > pBig->max_sdu)
                {
                    if(sdu->numOfCmplt_en){
                        (*pNumOfCmpPkt) += sdu->numHciPkt;
                    }
                    pBis->bisSduIn_rptr++; //deserted this SDU
                    pBis->bisSduInFreeNum++;
                    return IAL_ERR_SDU_LEN_EXCEED_SDU_MAX;
                }

                tlkapi_send_string_data(DBG_IAL_EN, "@IAL_SDU_DATA in''", sdu->data , sdu->iso_sdu_len);
            }
            else
            {

                tlkapi_send_string_data(DBG_IAL_EN, "      @ISO_PDU_Data tx'''", &pdu->isoTxPdu, pdu->isoTxPdu.llPhysChnPdu.llPduHdr.bisPduHdr.rf_len);
                blt_ll_bis_encryptPld(&pBis->bisCryptCtrl, &pdu->isoTxPdu, pdu->payloadNumber);
                pBis->bisPduTxFifoWptr ++;// PDU finalize

                tlkapi_send_string_u32s(DBG_IAL_EN,"PDU complete/End",pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len,pdu->payloadNumber,remain_len,pBis->bisPduTxFifoWptr);

                tlkapi_send_string_data(DBG_IAL_EN,"********End inSduFifoRptr",&pBis->bisSduIn_rptr,1);
                break;

            }

            if(sdu->numOfCmplt_en){
                (*pNumOfCmpPkt) += sdu->numHciPkt;
            }
        }
        else
        {
            slen = remain_len;
            segmHdr.cmplt = 0;


            // Framed PDU reassemble
            pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len += slen;
            smemcpy((pRfPdu->llPhysChnPdu.llPayload + pdu->offset), (sdu->data + sdu->sduOffset), slen);
            pRfPdu->dma_len = rf_tx_packet_dma_len(pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len +2);
            sdu->sduOffset += slen;
            remain_len = 0;// PDU full
            segmHdr.length += slen;
            smemcpy((pRfPdu->llPhysChnPdu.llPayload + segmHdrOffset), &segmHdr, segmHdrLen);
            tlkapi_send_string_data(DBG_IAL_EN, "      @ISO_PDU_Data tx''''", &pdu->isoTxPdu, pdu->isoTxPdu.llPhysChnPdu.llPduHdr.bisPduHdr.rf_len);
            blt_ll_bis_encryptPld(&pBis->bisCryptCtrl, &pdu->isoTxPdu, pdu->payloadNumber);
            pBis->bisPduTxFifoWptr++;

            tlkapi_send_string_u32s(DBG_IAL_EN,"PDU Start/Cont",pRfPdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len,pdu->payloadNumber,remain_len,pBis->bisPduTxFifoWptr);

        }


    }
    return BLE_SUCCESS;
}



#if(!FANQH_OPTIMIZE_BIS_API)
ble_sts_t blc_iso_sendData(u16 bisHandle, u8 *pData, u16 len){

    u8 bis_idx = bisHandle & BLT_BIS_IDX_MSK;

    ll_bis_t *pBis = (ll_bis_t *) (global_pBis + bis_idx);
    ll_big_bcst_t* pBig = global_pBigBcst + pBis->big_idx;

    if((!pBis->bis_occupied) || (pBig->cmd_status!=BIG_CREATE_COMPLETE)){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if((pBig->max_sdu==0) || (pBig->max_pdu==0)|| (pBis->bn==0)||(len > pBig->max_sdu)){
        return LL_ERR_INVALID_PARAMETER;
    }

    sdu_packet_t *sdu =(sdu_packet_t*) (sduBisMng.in_fifo_b + sduBisMng.max_in_fifo_size * \
                                            (bis_idx*sduBisMng.in_fifo_num + (pBis->bisSduIn_wptr & sduBisMng.in_fifo_mask)));

    if(sdu==NULL){
        return IAL_ERR_ISO_TX_FIFO_NOT_ENOUGH;
    }

    sdu->iso_sdu_len = len;
    sdu->timestamp = clock_time();
    sdu->numHciPkt = 1;
    sdu->pkt_st = HCI_ISO_SDU_COMPLETE;
    smemcpy(sdu->data, pData, len);

    pBis->bisSduIn_wptr++;

    return BLE_SUCCESS;

}
#endif

#if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER & 1)
static sdu_packet_t* blt_isoal_getNextBisSyncSdu(u16 bis_connHandle)
{
    u8 bis_sel = bis_connHandle & BLT_BIS_IDX_MSK;
    ll_bis_t *pBis = (ll_bis_t *)(global_pBis + bis_sel);

    sdu_packet_t* sdu = (sdu_packet_t*)(pBis->bis_sduOutBuf + sduBisMng.max_out_fifo_size * ((pBis->bisSduOut_wptr&(sduBisMng.out_fifo_mask))));
    sdu->isoHandle = bis_connHandle;

    return sdu;
}


ll_iso_unframe_type_t blt_ial_getBisUnframedType(ll_bis_t *pBis, bis_rx_pdu_t *pBisPdu, u8 llid){


    ll_iso_unframe_type_t type = UNFRAMED_START;

    if((pBisPdu->payloadNum) % pBis->numSdu2Pdu ==0)
    {
        if(pBisPdu->rawData[0] != 0)
        { //mark PDU valid
            if(pBis->numSdu2Pdu==1)
            {
                if(llid != ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU)
                {
                    pBisPdu->rawData[0] = 0;//invalid this PDU, purpose to report error SDU, reference  IAL/CIS/UNF/CEN/BI-05-C
                }
                type = UNFRAMED_COMPLETE;
            }
            else if(llid==ISO_LLID_UNFRAMED_PDU_START_CONTI_FRAGMENT_SDU)
            {
                type = UNFRAMED_START;
            }
            else if(llid == ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU)
            {
                type = UNFRAMED_COMPLETE;
            }
            else
            {
                return UNFRAMED_INVALID;
            }
        }
        else if(pBis->numSdu2Pdu==1){//empty
            type = UNFRAMED_COMPLETE;
        }
        else{//empty
            type = UNFRAMED_START;
        }
    }
    else{

        //condition 1. BN = 4: invalid SDU P1(llid=start), P2(llid=start),P3(llid=start), P4(llid=start)
        //condition 2. fix BN = 2, P1(complete) P2(pading), when P1 lost, then P1 interpret as Start, P2 interpret as continue, then that SDU will not complete.
        if((u32)(pBisPdu->payloadNum+1)%pBis->numSdu2Pdu ==0){

            if(pBisPdu->rawData[0] != 0 ){//set in rx_irq to mark PDU valid
                if(llid != ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU){
                    //condition 1. P4 interpret invalid PDU
                    pBisPdu->rawData[0] = 0;// invalid this PDU
                }

            }
            type = UNFRAMED_END;
        }
        else if(pBisPdu->rawData[0] != 0){
            if(llid==ISO_LLID_UNFRAMED_PDU_START_CONTI_FRAGMENT_SDU){
                type = UNFRAMED_CONTINUE;
            }
            else if(llid == ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU){
                type = UNFRAMED_END;
            }
            else{
                type= UNFRAMED_INVALID;
            }
        }
        else{
            type = UNFRAMED_END;
        }
    }

    return type;
}

ble_sts_t blt_ial_bisSync_reassemblePdu2Sdu(ll_bis_t *pBis, bis_rx_pdu_t *pBisPdu)
{
    u8 llid = 0, rfLen = 0;;
    u16 bis_handle = pBis->bis_handle;
    ll_big_sync_t *pBigSync =(ll_big_sync_t *) (global_pBigSync + pBis->big_idx);

    sdu_packet_t* sdu = blt_isoal_getNextBisSyncSdu(bis_handle);
    rf_packet_ll_data_t *rx_pdu = (rf_packet_ll_data_t*)pBisPdu->rawData;

    if(pBisPdu->rawData[0] != 0){
        rfLen = rx_pdu->llPhysChnPdu.llPduHdr.bisPduHdr.rf_len;
        llid = rx_pdu->llPhysChnPdu.llPduHdr.bisPduHdr.llid;
    }

    #if (SL16_bis0_rx_pro)
        log_b16_irq(SL_STACK_BIS_RX_DATA_EN, (SL16_bis0_rx_pro + (pBis->bis_handle&BLT_BIS_HANDLE) -bltBisMng.maxNum_bisBcst), (u16)pBisPdu->payloadNum);
    #endif

    if(pBigSync->framing ==0){
        ll_iso_unframe_type_t type = blt_ial_getBisUnframedType(pBis, pBisPdu, llid);
        tlkapi_send_string_u32s(DBG_IAL_EN, "IAL type", pBisPdu->payloadNum, pBisPdu->rawData[0], pBis->rxSduStatus, type);

        #if(SL16_bis0_rxPdu2Sdu_st)
            log_b16_byte(SL_STACK_BIS_RX_DATA_EN, (SL16_bis0_rxPdu2Sdu_st + (pBis->bis_handle&BLT_BIS_HANDLE) -bltBisMng.maxNum_bisBcst), pBis->rxSduStatus, type);
        #endif

        switch(pBis->rxSduStatus)
        {
            case    SDU_STATE_NEW:
            {
                if((type==UNFRAMED_COMPLETE) || (type == UNFRAMED_START))
                {
                    sdu->iso_sdu_len =  0;
                    if(gIsoTsEn){
                        sdu->timestamp = pBisPdu->bigRefAnchorPoint + SYSTEM_TIMER_TICK_1US * (pBigSync->big_sync_delay_us + (pBisPdu->payloadNum%pBis->bn/pBis->numSdu2Pdu) * pBigSync->sdu_intvl);
                    }
                }

                if(type==UNFRAMED_START)
                {
                    if(pBisPdu->rawData[0] != 0)
                    {
                        smemcpy(sdu->data, rx_pdu->llPhysChnPdu.llPayload, rfLen);
                        sdu->iso_sdu_len += rfLen;
                        sdu->pkt_st = HCI_ISO_VALID_DATA;
                    }
                    else
                    {
                        sdu->pkt_st = HCI_ISO_LOST_DATA;
                    }
                    pBis->rxSduStatus = SDU_STATE_CONTINUE; //Switch state
                }
                else if(type==UNFRAMED_COMPLETE)
                {
                    if(pBisPdu->rawData[0] != 0){
                        // copy iso_sdu
                        smemcpy(sdu->data, rx_pdu->llPhysChnPdu.llPayload, rfLen);
                        sdu->iso_sdu_len += rfLen;
                        sdu->pkt_st = HCI_ISO_VALID_DATA;
                    }
                    else{
                        sdu->pkt_st = HCI_ISO_LOST_DATA;
                    }
                    pBis->bisSduOut_wptr++;// next SDU buff
                    if(sdu->iso_sdu_len){
                        tlkapi_send_string_data(DBG_IAL_EN, " cmlt", sdu->data, 16);//sdu->iso_sdu_len
                    }
                    #if (SLEV_bis0_rx_sdu_cmplt)
                        log_event_irq(SL_STACK_BIS_RX_DATA_EN, (SLEV_bis0_rx_sdu_cmplt + (pBis->bis_handle&BLT_BIS_HANDLE) - bltBisMng.maxNum_bisBcst));
                    #endif

                    #if(SL16_bis0_rx_sdu_len)
                        log_b16(SL_STACK_BIS_RX_DATA_EN, (SL16_bis0_rx_sdu_len + (pBis->bis_handle&BLT_BIS_HANDLE) -bltBisMng.maxNum_bisBcst), sdu->iso_sdu_len);
                    #endif

                    pBis->rxSduStatus = SDU_STATE_NEW;
                }

                else{ // UNFRAMED_CONTINUE/UNFRAMED_END
                    pBis->rxSduStatus = SDU_STATE_NEW;
                }

                sdu->pkt_seq_num = pBisPdu->payloadNum/pBis->numSdu2Pdu;
                break;

            }
            case SDU_STATE_CONTINUE:
            {

                if(type==UNFRAMED_CONTINUE)
                {
                    if((pBisPdu->rawData[0] != 0) && (sdu->pkt_st == HCI_ISO_VALID_DATA))
                    {
                        //copy iso_sdu
                        smemcpy((sdu->data + sdu->iso_sdu_len), rx_pdu->llPhysChnPdu.llPayload, rfLen);
                        sdu->iso_sdu_len += rfLen;
                    }
                    else
                    {
                        sdu->pkt_st = HCI_ISO_LOST_DATA;
                    }
                }
                else if(type==UNFRAMED_END)
                {
                    if((pBisPdu->rawData[0] != 0) && (sdu->pkt_st == HCI_ISO_VALID_DATA)){
                        smemcpy((sdu->data + sdu->iso_sdu_len), rx_pdu->llPhysChnPdu.llPayload, rfLen);
                        sdu->iso_sdu_len += rfLen;

                        tlkapi_send_string_u32s(DBG_IAL_EN, "ial reassemble", pBisPdu->payloadNum, pBis->bisSduOut_wptr, pBisPdu->rawData[7], pBis->bis_handle);
                    }
                    else{
                        // modify when merge
                        sdu->pkt_st = (sdu->iso_sdu_len)?HCI_ISO_POSSIBLE_INVALID_DATA:HCI_ISO_LOST_DATA;
                        //discard this PDU
                    }
                    pBis->rxSduStatus = SDU_STATE_NEW; //switch stated
                    pBis->bisSduOut_wptr++;// next SDU buff

                    if(sdu->iso_sdu_len){
                        tlkapi_send_string_data(DBG_IAL_EN, " end", sdu->data, 16);//sdu->iso_sdu_len
                    }
                    #if (SLEV_bis0_rx_sdu_cmplt)
                        log_event_irq(SL_STACK_BIS_RX_DATA_EN, (SLEV_bis0_rx_sdu_cmplt + (pBis->bis_handle&BLT_BIS_HANDLE) - bltBisMng.maxNum_bisBcst));
                    #endif

                    #if(SL16_bis0_rx_sdu_len)
                        log_b16(SL_STACK_BIS_RX_DATA_EN, (SL16_bis0_rx_sdu_len + (pBis->bis_handle&BLT_BIS_HANDLE) -bltBisMng.maxNum_bisBcst), sdu->iso_sdu_len);
                    #endif
                }
                else{// error state: start, complete
                    pBis->rxSduStatus = SDU_STATE_NEW;
                }
                break;
            }
            default:
                break;
        }

        #if(DEBUG_UNFRAMED_RX & DBG_IAL_EN)
                char str[4][32] = {
                        "start",
                        "continue",
                        "end",
                        "complete",
                };
                tlkapi_send_string_u32s(DEBUG_UNFRAMED_RX&DBG_IAL_EN, str[type], pBisPdu->payloadNum, sdu->pkt_st,pBis->rxSduStatus, pBis->bisSduOut_wptr);
        #endif
    }
    else{ //framed packet

        if(pBisPdu->rawData[0] != 0){

            u8 *segData;
            u8 segDataLen = 0, offset = 0;
            int remainLen = rx_pdu->llPhysChnPdu.llPduHdr.pduHdr.rf_len;

            tlkapi_send_string_data(DBG_IAL_EN, "IAL Framed PDU1", pBisPdu->rawData, 16);

            while(remainLen > 0)
            {
                u8 skip_flag =0;
                iso_framed_segmHdr_t *segHdr = (iso_framed_segmHdr_t*)(rx_pdu->llPhysChnPdu.llPayload + offset);
                segDataLen = segHdr->length;

                tlkapi_send_string_u32s(DBG_IAL_EN, "IAL Framed PDU", pBisPdu->payloadNum, segHdr->sc, segHdr->cmplt, pBis->rxSduStatus);

                #if(SL16_bis0_rxPdu2Sdu_st)
                    log_b16_byte(SL_STACK_BIS_RX_DATA_EN, (SL16_bis0_rxPdu2Sdu_st + (pBis->bis_handle&BLT_BIS_HANDLE) -bltBisMng.maxNum_bisBcst), pBis->rxSduStatus, (segHdr->sc|segHdr->cmplt<<1));
                #endif

                if( ((remainLen-2) < segHdr->length))
                {
                    pBisPdu->rawData[0] = 0;
                    tlkapi_send_string_u32s(DBG_IAL_EN, "discard this PUD", pBisPdu->payloadNum,rx_pdu->llPhysChnPdu.llPduHdr.pduHdr.rf_len,segHdr->length+2,0);
                    goto fun_pdu_error;
                }


                switch(pBis->rxSduStatus)
                {
                    case    SDU_STATE_NEW:
                    {
                        if(segHdr->sc ==0)//start/complete
                        {
                            if(gIsoTsEn)
                            {
                                sdu->timestamp = pBisPdu->bigRefAnchorPoint + SYSTEM_TIMER_TICK_1US * (pBigSync->big_sync_delay_us + pBigSync->sdu_intvl + pBigSync->iso_itvl*1250 - segHdr->time_offset);
                                tlkapi_send_string_u32s(DBG_IAL_EN&0, "Ref: SDU", blt_debug_hex_2_dec_display(pBisPdu->payloadNum),sdu->timestamp, pBisPdu->bigRefAnchorPoint, segHdr->time_offset);
                            }
                            sdu->iso_sdu_len = 0;
                            //copy data from pdu to SDU
                            segDataLen -= ISO_FRAMED_TIMEOFFSET_LEN; // segmentation payload data len
                            segData = rx_pdu->llPhysChnPdu.llPayload + offset + ISO_FRAMED_SEGM_HEADER_LEN + ISO_FRAMED_TIMEOFFSET_LEN;
                            smemcpy(sdu->data, segData, segDataLen);
                            sdu->iso_sdu_len += segDataLen;
                            offset += segHdr->length + ISO_FRAMED_SEGM_HEADER_LEN;
                            remainLen -= segHdr->length + ISO_FRAMED_SEGM_HEADER_LEN;
                            sdu->pkt_st = HCI_ISO_VALID_DATA;
                            pBis->lossFlag = 0;
                            skip_flag = 0;

                        }
                        else // ERROR! continue/End
                        {
                            //DIscard this segmentation
                            offset += segHdr->length + ISO_FRAMED_SEGM_HEADER_LEN;
                            remainLen -= segDataLen + ISO_FRAMED_SEGM_HEADER_LEN;

                            if(!pBis->lossFlag)// PDU(Start X, End X, Start X),,,,,,  PDU(Continue)
                            {
                                pBis->lossFlag = 1;
                                /*invalid sdu(Part(s) of the ISO_SDU were not received correctly. This is reported as"lost data".)*/
                                if(gIsoTsEn)
                                {
                                    sdu->timestamp = pBisPdu->bigRefAnchorPoint + SYSTEM_TIMER_TICK_1US * (pBigSync->big_sync_delay_us + pBigSync->sdu_intvl + pBigSync->iso_itvl*1250);
                                }
                                sdu->iso_sdu_len = 0;


                                //packet sequence number
                                pBis->lastPktSeqNum ++;
                                sdu->pkt_seq_num = pBis->lastPktSeqNum;
                                //ISO SDU length
                                sdu->pkt_st = HCI_ISO_LOST_DATA;
                                pBis->bisSduOut_wptr++;// next SDU buff
                                #if (SLEV_bis0_rx_sdu_cmplt)
                                    log_event_irq(SL_STACK_BIS_RX_DATA_EN, (SLEV_bis0_rx_sdu_cmplt + (pBis->bis_handle&BLT_BIS_HANDLE) - bltBisMng.maxNum_bisBcst));
                                #endif

                                #if(SL16_bis0_rx_sdu_len)
                                    log_b16(SL_STACK_BIS_RX_DATA_EN, (SL16_bis0_rx_sdu_len + (pBis->bis_handle&BLT_BIS_HANDLE) -bltBisMng.maxNum_bisBcst), sdu->iso_sdu_len);
                                #endif

                                sdu = blt_isoal_getNextBisSyncSdu(bis_handle);
                                sdu->iso_sdu_len =0;
                            }
                            else{

                            }


                            //todo Loss SDU
                            skip_flag = 1;
                        }

                        break;
                    }

                    case    SDU_STATE_CONTINUE:
                    {
                        if(segHdr->sc ==0)// ERROR!!! start/complete
                        {

                            //finish last sdu, and continue current sdu
                            sdu->pkt_st = (sdu->iso_sdu_len)?HCI_ISO_POSSIBLE_INVALID_DATA : HCI_ISO_LOST_DATA;
                            //set packet_sequence_num
                            pBis->lastPktSeqNum ++;
                            sdu->pkt_seq_num = pBis->lastPktSeqNum;

                            pBis->bisSduOut_wptr++;// next SDU buff
                            #if (SLEV_bis0_rx_sdu_cmplt)
                                log_event_irq(SL_STACK_BIS_RX_DATA_EN, (SLEV_bis0_rx_sdu_cmplt + (pBis->bis_handle&BLT_BIS_HANDLE) - bltBisMng.maxNum_bisBcst));
                            #endif

                            #if(SL16_bis0_rx_sdu_len)
                                log_b16(SL_STACK_BIS_RX_DATA_EN, (SL16_bis0_rx_sdu_len + (pBis->bis_handle&BLT_BIS_HANDLE) -bltBisMng.maxNum_bisBcst), sdu->iso_sdu_len);
                            #endif

                            /*************Get New SDU*********************/
                            sdu = blt_isoal_getNextBisSyncSdu(bis_handle);
                            if(gIsoTsEn)
                            {
                                sdu->timestamp = pBisPdu->bigRefAnchorPoint + SYSTEM_TIMER_TICK_1US * (pBigSync->big_sync_delay_us + pBigSync->sdu_intvl + pBigSync->iso_itvl*1250 - segHdr->time_offset);
                            }
                            sdu->iso_sdu_len = 0;

                            segDataLen -= ISO_FRAMED_TIMEOFFSET_LEN; // segmentation payload data len
                            segData = rx_pdu->llPhysChnPdu.llPayload + offset + ISO_FRAMED_SEGM_HEADER_LEN + ISO_FRAMED_TIMEOFFSET_LEN;
                            smemcpy(sdu->data , segData, segDataLen);

                            sdu->iso_sdu_len += segDataLen;
                            offset += segHdr->length + ISO_FRAMED_SEGM_HEADER_LEN;
                            remainLen -= segDataLen + ISO_FRAMED_SEGM_HEADER_LEN + ISO_FRAMED_TIMEOFFSET_LEN;

                            //set ISO_SDU_Length = len
                            sdu->pkt_st = HCI_ISO_VALID_DATA;
                            pBis->rxSduStatus = SDU_STATE_NEW;
                            pBis->lossFlag = 0;
                            skip_flag = 0;
                        }
                        else //end/continue
                        {
                            segData = rx_pdu->llPhysChnPdu.llPayload + offset + ISO_FRAMED_SEGM_HEADER_LEN;
                            smemcpy((sdu->data + sdu->iso_sdu_len), segData, segDataLen);

                            sdu->iso_sdu_len += segDataLen;
                            offset += segHdr->length + ISO_FRAMED_SEGM_HEADER_LEN;
                            remainLen -= segDataLen + ISO_FRAMED_SEGM_HEADER_LEN;

                            sdu->pkt_st = HCI_ISO_VALID_DATA;

                            skip_flag = 0;
                            pBis->lossFlag = 0;
                        }
                        break;
                    }

                    default:
                    {
                        skip_flag = 1;
                        break;
                    }
                }

                if(skip_flag==0)
                {
                    if(segHdr->cmplt==1)
                    {
                        //set packet_sequence_num
                        pBis->lastPktSeqNum ++;
                        sdu->pkt_seq_num = pBis->lastPktSeqNum;
                        pBis->bisSduOut_wptr++;// next SDU buff
                        #if (SLEV_bis0_rx_sdu_cmplt)
                            log_event_irq(SL_STACK_BIS_RX_DATA_EN, (SLEV_bis0_rx_sdu_cmplt + (pBis->bis_handle&BLT_BIS_HANDLE) - bltBisMng.maxNum_bisBcst));
                        #endif

                        #if(SL16_bis0_rx_sdu_len)
                            log_b16(SL_STACK_BIS_RX_DATA_EN, (SL16_bis0_rx_sdu_len + (pBis->bis_handle&BLT_BIS_HANDLE) -bltBisMng.maxNum_bisBcst), sdu->iso_sdu_len);
                        #endif

                        tlkapi_send_string_u32s(DBG_IAL_EN, "Framed SDU", sdu->iso_sdu_len, pBis->bisSduOut_wptr, pBis->bisSduOut_rptr, sdu->data[0]);

                        /*********************Get next sdu buff*****************************/
                        sdu = blt_isoal_getNextBisSyncSdu(bis_handle);
                        sdu->iso_sdu_len =0;
                        pBis->rxSduStatus = SDU_STATE_NEW;
                    }
                    else
                    {
                        pBis->rxSduStatus = SDU_STATE_CONTINUE;
                    }
                }
            }

            return BLE_SUCCESS;
        }

        fun_pdu_error:
        {
            if(pBis->rxSduStatus == SDU_STATE_NEW)//start/complete
            {
                if(!pBis->lossFlag)
                {
                    pBis->lossFlag = 1;


                    /*invalid sdu(Part(s) of the ISO_SDU were not received correctly. This is reported as"lost data".)*/
                    if(gIsoTsEn)
                    {
                        sdu->timestamp = pBisPdu->bigRefAnchorPoint + SYSTEM_TIMER_TICK_1US * (pBigSync->big_sync_delay_us + pBigSync->sdu_intvl + pBigSync->iso_itvl*1250);
                    }

                    sdu->iso_sdu_len = 0;

                    //packet sequence number
                    pBis->lastPktSeqNum ++;
                    sdu->pkt_seq_num = pBis->lastPktSeqNum;

                    sdu->pkt_st = HCI_ISO_LOST_DATA;
                    pBis->bisSduOut_wptr++;// next SDU buff
                    #if (SLEV_bis0_rx_sdu_cmplt)
                        log_event_irq(SL_STACK_BIS_RX_DATA_EN, (SLEV_bis0_rx_sdu_cmplt + (pBis->bis_handle&BLT_BIS_HANDLE) - bltBisMng.maxNum_bisBcst));
                    #endif

                    #if(SL16_bis0_rx_sdu_len)
                        log_b16(SL_STACK_BIS_RX_DATA_EN, (SL16_bis0_rx_sdu_len + (pBis->bis_handle&BLT_BIS_HANDLE) -bltBisMng.maxNum_bisBcst), sdu->iso_sdu_len);
                    #endif

                    //get next sdu buff
                    sdu = blt_isoal_getNextBisSyncSdu(bis_handle);
                    sdu->iso_sdu_len = 0;
                }
            }
            else //SDU_STATE_CONTINUE
            {
                pBis->lossFlag = 1;

                //finish last sdu, and continue current sdu
                sdu->pkt_st = (sdu->iso_sdu_len)?HCI_ISO_POSSIBLE_INVALID_DATA : HCI_ISO_LOST_DATA;
                //set packet_sequence_num
                pBis->lastPktSeqNum ++;
                sdu->pkt_seq_num = pBis->lastPktSeqNum;

                pBis->bisSduOut_wptr++;// next SDU buff
                #if (SLEV_bis0_rx_sdu_cmplt)
                    log_event_irq(SL_STACK_BIS_RX_DATA_EN, (SLEV_bis0_rx_sdu_cmplt + (pBis->bis_handle&BLT_BIS_HANDLE) - bltBisMng.maxNum_bisBcst));
                #endif

                #if(SL16_bis0_rx_sdu_len)
                    log_b16(SL_STACK_BIS_RX_DATA_EN, (SL16_bis0_rx_sdu_len + (pBis->bis_handle&BLT_BIS_HANDLE) -bltBisMng.maxNum_bisBcst), sdu->iso_sdu_len);
                #endif


                //get next sdu buff
                sdu = blt_isoal_getNextBisSyncSdu(bis_handle);
                sdu->iso_sdu_len = 0;

                pBis->rxSduStatus = SDU_STATE_NEW;
            }

        }
    }

    return BLE_SUCCESS;
}
#else

#endif



#endif




/*********************************************************************************************************
 * BIS Function End
 *********************************************************************************************************/

#endif //end of "LL_FEATURE_ENABLE_ISO"


