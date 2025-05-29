/********************************************************************************************************
 * @file    iso.c
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

iso_para_t iso_param;


u8 gIsoTsEn = 1;


//#define LL_CMD_SETUP_ISO_DATA_PATH                                 0x106E //7.8.109 LE Setup ISO Data Path command
//#define LL_CMD_REMOVE_ISO_DATA_PATH                                0x106F //7.8.110 LE Remove ISO Data Path command


/* only ISO module use, so define here */
_attribute_ble_data_retention_ hci_cmd_callback_t ll_bis_cmd_task_cb = NULL;

/**
 * @brief  This function is used to enable/disable timestamp in SDU reported from controller
 * to host
 * @param[in]      Status - 0x00:  disable, time stamp is invalid in SDU;
 *                        - 1:  enable, time stamp is valid in SDU
 */
void blc_iso_enableSduToHostTimestamp(u8 en)
{
    gIsoTsEn = en;
}

ble_sts_t blt_iso_proSduPacket(sdu_packet_t *sdu)
{
    ble_sts_t ret = IAL_HCI_BUFFER_INVALID;
    /*
    +--------------+--------------+------------+------------+------------+---------+----+----+--------------+
    | 2            | 2            | 4          | 2          | 1          | 1       | 1  | 1  | iso_sdu_len  |
    +--------------+--------------+------------+------------+------------+---------+----+----+--------------+
    | pkt_seq_num  | iso_sdu_len  | timestamp  | sduOffset  | numHciPkt  | pkt_st  | PB | TS | SDU_Data     |
    +--------------+--------------+------------+------------+------------+---------+----+----+--------------+

    HCI ISO out DATA format in telink
    +------+-------+---------+-----------------------+------------+----------------------+-----------------+----------+
    | 2    | 1     | 2       | 2                     | 4          | 2                    | 2               | n        |
    +------+-------+---------+-----------------------+------------+----------------------+-----------------+----------+
    | len  | type  | handle  | ISO_data_load_length  | timestamp  | packet_sequence_num  | iso_sdu_length  | sd_data  |
    +------+-------+---------+-----------------------+------------+----------------------+-----------------+----------+
     */
    iso_data_packet_t *pIsoPkt;
    iso_data_load_1_t *pIsoLd1 = NULL; //give NULL to avoid compile warning
    iso_data_load_2_t *pIsoLd2 = NULL; //give NULL to avoid compile warning
    u16                iso_data_load_len;
    u32                timeStamp = 0;

    /* store in advance, in case modified by pointer "pIsoPkt" */
    u16 iso_sdu_len = sdu->iso_sdu_len;
    u16 pkt_seq     = sdu->pkt_seq_num;
    u8  psf         = sdu->pkt_st; //packet status flag

    if (gIsoTsEn) {
        iso_data_load_len = sdu->iso_sdu_len + 8;
        pIsoPkt           = (iso_data_packet_t *)(sdu->data - 12);
        pIsoLd1           = (iso_data_load_1_t *)pIsoPkt->p_ISO_data_load;
        timeStamp         = sdu->timestamp; //store in advance, in case modified by pointer "pIsoPkt"
    } else {
        iso_data_load_len = sdu->iso_sdu_len + 4;
        pIsoPkt           = (iso_data_packet_t *)(sdu->data - 8);
        pIsoLd2           = (iso_data_load_2_t *)pIsoPkt->p_ISO_data_load;
    }


    pIsoPkt->connHandle = sdu->isoHandle;
    pIsoPkt->pb         = HCI_ISO_SDU_COMPLETE;
    pIsoPkt->ts         = gIsoTsEn;
    pIsoPkt->rfu1       = 0;  //must

    pIsoPkt->iso_dat_len = iso_data_load_len;
    pIsoPkt->rfu2        = 0; //must


    if (gIsoTsEn) {
        pIsoLd1->timestamp   = timeStamp;
        pIsoLd1->pkt_seq     = pkt_seq;
        pIsoLd1->iso_sdu_len = iso_sdu_len;
        pIsoLd1->rfu         = 0;
        pIsoLd1->ps          = psf;
    } else {
        pIsoLd2->pkt_seq     = pkt_seq;
        pIsoLd2->iso_sdu_len = iso_sdu_len;
        pIsoLd2->rfu         = 0;
        pIsoLd2->ps          = psf;
    }


    #if (IUT_HCI_LOG_EN)
    if (iso_sdu_len) {
        cisConn_param.cis_sduDataNum++;
        u8 *data_begin = ((u8 *)pIsoPkt) - 1;
        data_begin[0]  = cisConn_param.cis_sduDataNum;
        //log_event(SL_STACK_ISO_DATA_EN, SLEV_iso_out_dat);
        tlkapi_send_string_data(IUT_HCI_LOG_EN, "HCI_ISO_Data out", data_begin, pIsoPkt->iso_dat_len + 5);
        //tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][DAT] HCI_ISO_Data out", data_begin ,pIsoPkt->iso_dat_len + 5);
    } else {
        //tlkapi_send_string_data(IUT_HCI_LOG_EN, "@HCI_ISO_Data out zero", pIsoPkt ,pIsoPkt->iso_dat_len + 4);
    }
    #endif

    //consider: do not care about data buffer overflow now
    if (blt_hci_iso_data_handler) {                                           //blc_hci_sendIsoData2Host
        ret = blt_hci_iso_data_handler((u8 *)pIsoPkt, iso_data_load_len + 4); // 4: handle(2), iso_data_load_length(2)
    }

    return ret;
}

ble_sts_t blc_hci_le_read_iso_tx_sync(u16 iso_connHandle, hci_le_readIsoTxSync_retParam_t *pRetParam)
{
    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_ISO_TX_Sync", pRetParam, sizeof(hci_le_readIsoTxSync_retParam_t));

    u16 iso_handle = iso_connHandle;
    if (0) {
    }
    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    else if (iso_connHandle & BLT_CIS_HANDLE) {
        if (ll_cis_cmd_task_cb) {
            return ll_cis_cmd_task_cb(HCI_CMD_LE_READ_ISO_TX_SYNC, (u8 *)&iso_handle, pRetParam); // blt_cis_cmd_process_task  blc_ll_read_cis_tx_sync
        }
    }
    #endif
    #if (LL_FEATURE_ENABLE_CONNECTIONLESS_ISO)
    else if (iso_connHandle & BLT_BIS_HANDLE) {

        u16 pkt_sn = 0x1234;
        u32 pkt_ts = 0x0000;
        u32 pkt_to = 0x0000;

        ll_bis_t *pBis = NULL;
        //      ll_big_sync_t *pBigSync;
        //      ll_big_bcst_t *pBigBcst;
        if ((pBis = blt_ll_findBisByHandle(iso_connHandle)) != NULL) {
            if (pBis->bis_role == BIS_ROLE_SYNC) {
                pRetParam->status = HCI_ERR_CMD_DISALLOWED;
                return HCI_ERR_CMD_DISALLOWED;
            }
            /*
             * If the Host issues this command before an SDU has been transmitted by the
                Controller, the Controller shall return the error code Command Disallowed
                (0x0C).
             */
            tlkapi_send_string_u32s(0, "read_iso_tx_sync", pBis->tx_first_pdu, 0, 0, 0) if (pBis->tx_first_pdu == 1)
            {
                pRetParam->status = HCI_ERR_CMD_DISALLOWED;
                return HCI_ERR_CMD_DISALLOWED;
            }
            if (blt_ll_findExistingBigSyncByBigHdl(pBis->link_big_handle) != BIG_HANDLE_INVALID) {
                pkt_sn = latest_pBigSync->bigEventCnt;
            } else if (blt_ll_searchExistingBigBcstHdl(pBis->link_big_handle) != BIG_HANDLE_INVALID) {
                pkt_sn = latest_pBigBcst->bigEventCnt;
            } else {
                pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
                return HCI_ERR_UNKNOWN_CONN_ID;
            }
        }


        pRetParam->status     = BLE_SUCCESS;
        pRetParam->connHandle = iso_connHandle;
        pRetParam->pkt_seqno  = pkt_sn;
        pRetParam->tx_ts      = pkt_ts;
        smemcpy(&pRetParam->time_offset, &pkt_to, 3);
        return BLE_SUCCESS;
    }
    #endif


    pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blc_hci_le_setupIsoDataPath(hci_le_setupIsoDataPath_cmdParam_t *pCmdPara, hci_le_setupIsoDataPath_retParam_t *pRetParam)
{
    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Setup_ISO_Data_Path", pCmdPara, 13); // to codec_config_len


    pRetParam->status      = HCI_ERR_UNKNOWN_CONN_ID;
    pRetParam->conn_handle = pCmdPara->conn_handle;
    if (0) {
    }
    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    else if (pCmdPara->conn_handle & BLT_CIS_HANDLE) {
        if (ll_cis_cmd_task_cb) {
            return ll_cis_cmd_task_cb(HCI_CMD_LE_SETUP_ISO_DATA_PATH, pCmdPara, pRetParam); // blt_cis_cmd_process_task  blc_hci_le_setupCisDataPath
        }
    }
    #endif
    #if (LL_FEATURE_ENABLE_CONNECTIONLESS_ISO)
    else if (pCmdPara->conn_handle & BLT_BIS_HANDLE) {
        if (ll_bis_cmd_task_cb) {
            return ll_bis_cmd_task_cb(HCI_CMD_LE_SETUP_ISO_DATA_PATH, pCmdPara, pRetParam); // blt_bis_cmd_process_task  blc_hci_le_setupBisDataPath
        }
    }
    #endif


    return pRetParam->status;
}

ble_sts_t blc_hci_le_removeIsoDataPath(hci_le_rmvIsoDataPath_cmdParam_t *pCmdParam, hci_le_rmvIsoDataPath_retParam_t *pRetParam)
{
    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Remove_ISO_Data_Path", pCmdParam, sizeof(hci_le_rmvIsoDataPath_cmdParam_t));

    pRetParam->status      = HCI_ERR_UNKNOWN_CONN_ID;
    pRetParam->conn_handle = pCmdParam->conn_handle;
    if (0) {
    }
    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    else if (pCmdParam->conn_handle & BLT_CIS_HANDLE) {
        if (ll_cis_cmd_task_cb) {
            return ll_cis_cmd_task_cb(HCI_CMD_LE_REMOVE_ISO_DATA_PATH, pCmdParam, pRetParam); // blt_cis_cmd_process_task  blc_hci_le_removeCisDataPath
        }
    }
    #endif

    #if (LL_FEATURE_ENABLE_CONNECTIONLESS_ISO)
    else if (pCmdParam->conn_handle & BLT_BIS_HANDLE) {
        if (ll_bis_cmd_task_cb) {
            return ll_bis_cmd_task_cb(HCI_CMD_LE_REMOVE_ISO_DATA_PATH, pCmdParam, pRetParam); //blt_bis_cmd_process_task   blc_hci_le_removeBisDataPath
        }
    }
    #endif


    return pRetParam->status;
}

ble_sts_t blc_ll_setupIsoDataPath(u16 conn_handle, dat_path_dir_t dir, dat_path_id_t id, u8 cid_assignNum, u16 cidcompId, u16 cid_vendorDef, u32 control_dly, u8 codec_cfg_len, u8 codec_cfg1, u8 codec_cfg2, u8 codec_cfg3, u8 codec_cfg4)
{
    tlkapi_send_string_u8s(BLC_LL_LOG_EN, "[LL][CMD] Setup_ISO_Data_Path", conn_handle, dir, id, cid_assignNum);


    u8                                  param_buffer[sizeof(hci_le_setupIsoDataPath_cmdParam_t)];
    hci_le_setupIsoDataPath_cmdParam_t *pCmdPara = (hci_le_setupIsoDataPath_cmdParam_t *)param_buffer;
    pCmdPara->conn_handle                        = conn_handle;
    pCmdPara->data_path_dir                      = dir;
    pCmdPara->data_path_id                       = id;
    pCmdPara->codec_id_assignNum                 = cid_assignNum;
    pCmdPara->codec_id_compId                    = cidcompId;
    pCmdPara->codec_id_vendorDef                 = cid_vendorDef;
    pCmdPara->control_delay[0]                   = U32_BYTE0(control_dly);
    pCmdPara->control_delay[1]                   = U32_BYTE1(control_dly);
    pCmdPara->control_delay[2]                   = U32_BYTE2(control_dly);
    pCmdPara->codec_config_len                   = codec_cfg_len;
    pCmdPara->codec_config[0]                    = codec_cfg1;
    pCmdPara->codec_config[1]                    = codec_cfg2;
    pCmdPara->codec_config[2]                    = codec_cfg3;
    pCmdPara->codec_config[3]                    = codec_cfg4;

    u8                                  ret_buffer[sizeof(hci_le_setupIsoDataPath_retParam_t)];
    hci_le_setupIsoDataPath_retParam_t *pRetParam = (hci_le_setupIsoDataPath_retParam_t *)ret_buffer;


    if (0) {
    }
    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    else if (conn_handle & BLT_CIS_HANDLE) {
        if (ll_cis_cmd_task_cb) {
            return ll_cis_cmd_task_cb(HCI_CMD_LE_SETUP_ISO_DATA_PATH, pCmdPara, pRetParam); // blt_cis_cmd_process_task  blc_hci_le_setupCisDataPath
        }
    }
    #endif
    #if (LL_FEATURE_ENABLE_CONNECTIONLESS_ISO)
    else if (conn_handle & BLT_BIS_HANDLE) {
        if (ll_bis_cmd_task_cb) {
            return ll_bis_cmd_task_cb(HCI_CMD_LE_SETUP_ISO_DATA_PATH, pCmdPara, pRetParam); // blt_bis_cmd_process_task  blc_hci_le_setupBisDataPath
        }
    }
    #endif


    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blc_ll_removeIsoDataPath(u16 conn_handle, dp_dir_msk_t dir_mask)
{
    u8                                param_buffer[sizeof(hci_le_rmvIsoDataPath_cmdParam_t)];
    hci_le_rmvIsoDataPath_cmdParam_t *pCmdPara = (hci_le_rmvIsoDataPath_cmdParam_t *)param_buffer;
    pCmdPara->conn_handle                      = conn_handle;
    pCmdPara->dp_dir_mask                      = dir_mask;

    tlkapi_send_string_data(BLC_LL_LOG_EN, "[LL][CMD] Remove_ISO_Data_Path", pCmdPara, sizeof(hci_le_rmvIsoDataPath_cmdParam_t));

    u8                                ret_buffer[sizeof(hci_le_rmvIsoDataPath_retParam_t)];
    hci_le_rmvIsoDataPath_retParam_t *pRetParam = (hci_le_rmvIsoDataPath_retParam_t *)ret_buffer;


    if (0) {
    }
    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    else if (conn_handle & BLT_CIS_HANDLE) {
        if (ll_cis_cmd_task_cb) {
            return ll_cis_cmd_task_cb(HCI_CMD_LE_REMOVE_ISO_DATA_PATH, pCmdPara, pRetParam); // blt_cis_cmd_process_task  blc_hci_le_removeCisDataPath
        }
    }
    #endif
    #if (LL_FEATURE_ENABLE_CONNECTIONLESS_ISO)
    else if (conn_handle & BLT_BIS_HANDLE) {
        if (ll_bis_cmd_task_cb) {
            return ll_bis_cmd_task_cb(HCI_CMD_LE_REMOVE_ISO_DATA_PATH, pCmdPara, pRetParam); // blt_bis_cmd_process_task  blc_hci_le_removeBisDataPath
        }
    }
    #endif


    return HCI_ERR_UNKNOWN_CONN_ID;
}


    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #endif
    ble_sts_t
    blt_hci_processIsoData(iso_data_packet_t *pIsoDatPkt)
{
    DBG_SIHUI_CHN11_TOGGLE;

    if (0) {
    }
    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    else if (pIsoDatPkt->connHandle & BLT_CIS_HANDLE) {
        if (ll_cis_cmd_task_cb) {
            return ll_cis_cmd_task_cb(HCI_CMD_LE_ISO_DATA, pIsoDatPkt, NULL); // blt_cis_cmd_process_task  blc_hci_le_pushCisData
        }
    }
    #endif

    #if (LL_FEATURE_ENABLE_CONNECTIONLESS_ISO)
    else if (pIsoDatPkt->connHandle & BLT_BIS_HANDLE) {
        if (ll_bis_cmd_task_cb) {
            return ll_bis_cmd_task_cb(HCI_CMD_LE_ISO_DATA, pIsoDatPkt, NULL); //blt_bis_cmd_process_task
        }
    }
    #endif

    return HCI_ERR_UNKNOWN_CONN_ID;
}


    #define DBG_ISO_PACK_DATA_FLOW 1


    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #endif
    ble_sts_t
    blt_iso_process_sdu_in_data(sdu_packet_t *iso_sdu, iso_pb_flag_t PB_Flag, u8 TS_Flag, u32 time_stamp, u16 seqnum, u16 total_len, u16 cur_len, u8 *pData)
{
    int error  = 0;
    int finish = 0;

    do {
        if ((PB_Flag == HCI_ISO_SDU_FIRST_FRAG) || (PB_Flag == HCI_ISO_SDU_COMPLETE)) {
            /* for more secure: clear "sduOffset" in CIS "blt_cis_establish common" & BIS ""  */
            if (iso_sdu->sduOffset) {
                tlkapi_send_string_u32s(HOST_HCI_ERR_LOG_EN, "previous SDU in data not finish", iso_sdu->sduOffset, 0, 0, 0);
                BLMS_ERR_DEBUG(HOST_HCI_ERR_LOG_EN, 0x99C80000);
                error = 1;
                break;
            }

            iso_sdu->pkt_seq_num = seqnum;
            iso_sdu->iso_sdu_len = total_len;
            if (TS_Flag) {
                iso_sdu->timestamp = time_stamp;
            }

            iso_sdu->sduOffset = 0; //for more secure
            iso_sdu->numHciPkt = 0;

            iso_sdu->ts = TS_Flag;

            if (PB_Flag == HCI_ISO_SDU_COMPLETE) {
                finish = 1;
                break;
            }
        } else if ((PB_Flag == HCI_ISO_SDU_CONTINUE_FRAG) || (PB_Flag == HCI_ISO_SDU_LAST_FRAG)) {
            /*The fields Time_Stamp, Packet_Sequence_Number, Packet_Status_Flag and
                ISO_SDU_Length are only included in the HCI ISO Data packet when the
                PB_Flag equals 0b00 or 0b10.*/
            if (!iso_sdu->sduOffset) {
                tlkapi_send_string_u32s(HOST_HCI_ERR_LOG_EN, "no first SDU in data", iso_sdu->sduOffset, 0, 0, 0);
                BLMS_ERR_DEBUG(HOST_HCI_ERR_LOG_EN, 0x99C80000);
                error = 1;
                break;
            }

            u16 cur_accumulate_len = iso_sdu->sduOffset + cur_len;

            if ((PB_Flag == HCI_ISO_SDU_LAST_FRAG)) {
                if (cur_accumulate_len == iso_sdu->iso_sdu_len) {
                    finish = 1;
                    break;
                } else {
                    tlkapi_send_string_u32s(HOST_HCI_ERR_LOG_EN, "HCI ERROR, last data, length not match", cur_accumulate_len, iso_sdu->iso_sdu_len, 0, 0);
                    BLMS_ERR_DEBUG(HOST_HCI_ERR_LOG_EN, 0x99C80000);
                    error = 1;
                    break;
                }
            } else { //HCI_ISO_SDU_CONTINUE_FRAG
                if (cur_accumulate_len >= iso_sdu->iso_sdu_len) {
                    tlkapi_send_string_u32s(HOST_HCI_ERR_LOG_EN, "HCI ERROR, continue data, length exceed", cur_accumulate_len, iso_sdu->iso_sdu_len, 0, 0);
                    BLMS_ERR_DEBUG(HOST_HCI_ERR_LOG_EN, 0x99C80000);
                    error = 1;
                    break;
                }
            }
        }
    } while (0);


    if (error) {
        iso_sdu->iso_sdu_len = 0;
        iso_sdu->sduOffset   = 0;
        iso_sdu->numHciPkt   = 0;

        return FALSE;
    } else {
        smemcpy((iso_sdu->data + iso_sdu->sduOffset), pData, cur_len);
        iso_sdu->sduOffset += cur_len;
        iso_sdu->numHciPkt++;       //mark Num of hci packet

        if (finish) {
            iso_sdu->sduOffset = 0; //clear
            return TRUE;
        } else {
            return FALSE;
        }
    }
}


    #if (FANQH_OPTIMIZE_BIS_API)
/**
 * @brief      This function is used to send ISO data.
 * @param[in]  cisHandle or bisHandle
 * @param[in]  pData  point to data to send
 * @param[in]  len  the length to send
 * @return      Status - 0x00:  succeeded;
 *                       other:  failed
 */
ble_sts_t blc_iso_sendData(u16 handle, u8 *pData, u16 len)
{
    if (handle & BLT_CIS_HANDLE) {
        return blt_ll_pushCisDataFun(handle, HCI_ISO_SDU_COMPLETE, 0, 0, 0, len, len, pData); //blt_ll_pushCisData
    } else if (handle & BLT_BIS_HANDLE) {
        return blt_ll_pushBisDataFun(handle, HCI_ISO_SDU_COMPLETE, 0, 0, 0, len, len, pData); //blt_ll_pushBisData
    } else {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }
}
    #endif

#endif //end of LL_FEATURE_ENABLE_ISO
