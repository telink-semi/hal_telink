/********************************************************************************************************
 * @file    smp_sc.c
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
#include "stack/ble/host/ble_host.h"
#include "stack/ble/controller/ble_controller.h"

#if DUAL_CORE_MODE_ENABLED
bool blt_smp_push_tx_fifo(u16 connHandle, smp2llcap_type_t * smp_pkt);
#endif

typedef struct
{
    //Private and DHkey use the same buffer, after obtaining dhkey, the private key content will be overwritten by dhkey
    u8 sc_sk_dhk_own[32]; //  own  private (reused DH)key[32]
    u8 sc_pk_own[64];     //  own  public  key[64]
    u8 sc_pk_peer[64];    // peer  public  key[64]
} smp_ecdh_key_t;

_attribute_aligned_(4) smp_ecdh_key_t smp_ecdh_key;

#define MTU_SIZE 23

u8 blc_smp_pushDataSplit(u16 connHandle, u8 smp_code, u8 *p, u8 len)
{
#if DUAL_CORE_MODE_ENABLED
    u8 pkt_smp_short[80] = {0};
    pkt_smp_short[0]     = 0;              //first data packet
    pkt_smp_short[1]     = 0;              // rf_len

    *(u16 *)(pkt_smp_short + 2) = len + 1; //l2cap
    *(u16 *)(pkt_smp_short + 4) = 0x06;    //chanid
    pkt_smp_short[6]            = smp_code;

    smemcpy(pkt_smp_short + 7, p, len);
//    extern bool blt_smp_push_tx_fifo(u16 connHandle, smp2llcap_type_t * smp_pkt);
    blt_smp_push_tx_fifo(connHandle, (smp2llcap_type_t *)pkt_smp_short);
#else
    int n;
    u8  pkt_smp_short[36] = {0};

    n                = len < (MTU_SIZE - 1) ? len : MTU_SIZE - 1;
    pkt_smp_short[0] = n + 7;
    pkt_smp_short[4] = 2;                  //first data packet
    pkt_smp_short[5] = n + 5;              // rf_len

    *(u16 *)(pkt_smp_short + 6) = len + 1; //l2cap
    *(u16 *)(pkt_smp_short + 8) = 0x06;    //chanid
    pkt_smp_short[10]           = smp_code;

    smemcpy(pkt_smp_short + 11, p, n);
    ll_push_tx_fifo_handler(connHandle, pkt_smp_short + 4);

    for (int i = n; i < len; i += n) {
        n                = (len - i) > (MTU_SIZE + 4) ? (MTU_SIZE + 4) : len - i;
        pkt_smp_short[0] = n + 2;
        pkt_smp_short[4] = 1; //first data packet
        pkt_smp_short[5] = n;
        smemcpy(pkt_smp_short + 6, p + i, n);
        ll_push_tx_fifo_handler(connHandle, pkt_smp_short + 4);
    }
#endif
    return len;
}


#if (SMP_SC_OOB_EN)

bool blc_smp_cancel_auth(u16 connHandle)
{
    //    u8 smp_master_role = (connHandle & BLM_CONN_HANDLE);
    u8               conn_idx       = connHandle & CONN_IDX_MASK;
    u8               smp_status_idx = (connHandle & BLM_CONN_HANDLE) ? 0 : (conn_idx - LL_MAX_ACL_CEN_NUM + 1);
    smp_param_own_t *s_blms_p_own   = (smp_param_own_t *)&smp_param_own[smp_status_idx];

    if (!(smp_sts_param[conn_idx].tk_status & TK_ST_REQUEST)) { //need OOB pending request is TRUE
        return FALSE;
    }

    u8 smp_err_code;
    switch (s_blms_p_own->stk_method) {
    case PK_Init_Display_Resp_Input:
    case PK_Resp_Display_Init_Input:
    case PK_BOTH_INPUT:
        smp_err_code = PAIRING_FAIL_REASON_PASSKEY_ENTRY;
        break;
    case Numeric_Comparison:
        smp_err_code = PAIRING_FAIL_REASON_CONFIRM_FAILED;
        break;
    case SC_OOB_Authentication:
    case OOB_Authentication:
        smp_err_code = PAIRING_FAIL_REASON_OOB_NOT_AVAILABLE;
        break;
    case JustWorks:
        smp_err_code = PAIRING_FAIL_REASON_UNSPECIFIED_REASON;
        break;
    default:
        tlkapi_printf(SMP_DBG_EN, "Unknown pairing method (%d)", s_blms_p_own->stk_method);
        return FALSE;
    }

    blt_smp_procPairingEnd(connHandle, smp_err_code); //pairing end with failure
    u8 *pr = blt_smp_pushPairingFailed(smp_err_code);
#if DUAL_CORE_MODE_ENABLED
    //extern bool blt_smp_push_tx_fifo(u16 connHandle, smp2llcap_type_t * smp_pkt);
    return blt_smp_push_tx_fifo(connHandle, (smp2llcap_type_t *)pr);
#else
    return ll_push_tx_fifo_handler(connHandle | HANDLE_STK_FLAG, pr);
#endif
}

/**
 * With or without initializing the public-private key pair, we power up
 * to initialize a set that is not currently stored on non-volatile memory.
 * It will be lost when powered down.
 */
int blc_smp_generateScOobData(smp_sc_oob_data_t *local_oob_data, smp_sc_oob_key_t *local_ecdh_key)
{
    if (!local_oob_data || !local_ecdh_key) {
        return 0;
    }

    if (!blt_ecc_gen_key_pair(local_ecdh_key->public_key, local_ecdh_key->private_key, ECC_use_secp256r1, non_debug_mode)) {
        tlkapi_printf(SMP_DBG_EN, "generate an ECDH public-private key pairs failed");
        return 0;
    }

    tlkapi_printf(SMP_DBG_EN, "our pubkey= %s", hex_to_str(local_ecdh_key->public_key, 64));
    tlkapi_printf(SMP_DBG_EN, "our privkey=%s", hex_to_str(local_ecdh_key->private_key, 32));

    generateRandomNum(16, local_oob_data->random);

    blt_crypto_alg_f4(local_oob_data->confirm, local_ecdh_key->public_key, local_ecdh_key->public_key, local_oob_data->random, 0);

    #if (SMP_DBG_EN)
    tlkapi_printf(SMP_DBG_EN, "SC OOB data-confirm (be) %s ", hex_to_str(local_oob_data->confirm, 16));
    tlkapi_printf(SMP_DBG_EN, "SC OOB data-random  (be) %s ", hex_to_str(local_oob_data->random, 16));
    //    u8 le_r[16], le_c[16];
    //    swapX (local_oob_data->random, le_r, 16);
    //    swapX (local_oob_data->confirm, le_c, 16);
    //    tlkapi_printf(SMP_DBG_EN, "SC OOB data-random  (le) %s ", hex_to_str(le_r, 16));
    //    tlkapi_printf(SMP_DBG_EN, "SC OOB data-confirm (le) %s ", hex_to_str(le_c, 16));
    #endif

    return 1;
}

int blc_smp_setScOobData(u16 connHandle, const smp_sc_oob_data_t *oob_local, const smp_sc_oob_key_t *local_ecdh_key, const smp_sc_oob_data_t *oob_remote)
{
    u8               smp_master_role = (connHandle & BLM_CONN_HANDLE);
    u8               conn_idx        = connHandle & CONN_IDX_MASK;
    u8               smp_status_idx  = (connHandle & BLM_CONN_HANDLE) ? 0 : (conn_idx - LL_MAX_ACL_CEN_NUM + 1);
    smp_param_own_t *s_blms_p_own    = (smp_param_own_t *)&smp_param_own[smp_status_idx];

    if (!(smp_sts_param[conn_idx].tk_status & TK_ST_REQUEST)) { //need SCOOB pending request is TRUE
        return 0;
    }

    //oob data check
    bool scOobLocalUsed, scOobRemoteUsed;
    if (!smp_master_role) {
        //For slave role: if the master's pairing_req oob flag is set, the slave's local SC OOB data will be used!!!
        scOobLocalUsed  = (s_blms_p_own->pairing_req.oobDataFlag);
        scOobRemoteUsed = (s_blms_p_own->pairing_rsp.oobDataFlag);
    } else {
        //For master role: if the slave's pairing_rsp oob flag is set, the master's local SC OOB data will be used!!!
        scOobLocalUsed  = (s_blms_p_own->pairing_rsp.oobDataFlag);
        scOobRemoteUsed = (s_blms_p_own->pairing_req.oobDataFlag);
    }

    if ((scOobLocalUsed && oob_local == NULL) || (scOobRemoteUsed && oob_remote == NULL)) {
        tlkapi_printf(SMP_DBG_EN, "sc oob data check fail");
        blc_smp_cancel_auth(connHandle);
        return 0;
    }

    s_blms_p_own->scoob_remote = scOobRemoteUsed ? (smp_sc_oob_data_t *)(size_t)oob_remote : NULL;

    if (scOobLocalUsed) {
        s_blms_p_own->scoob_local     = (smp_sc_oob_data_t *)(size_t)oob_local;
        s_blms_p_own->scoob_local_key = (smp_sc_oob_key_t *)(size_t)local_ecdh_key;
    } else {
        s_blms_p_own->scoob_local     = NULL;
        s_blms_p_own->scoob_local_key = NULL;
    }

    if (local_ecdh_key) {
        memcpy(smp_ecdh_key.sc_pk_own, local_ecdh_key->public_key, 64);
        memcpy(smp_ecdh_key.sc_sk_dhk_own, local_ecdh_key->private_key, 32);
    }

    blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_PUBLIC_KEY);

    //for smp slave
    if (!smp_master_role) {
        //sc_oob confirm check
        if (s_blms_p_own->scoob_remote) {
            u8 confirm[16];
            //Confirm = f4(PK,PK,random,0)
            tlkapi_printf(SMP_DBG_EN, "sc_pk_peer(be):%s", hex_to_str(smp_ecdh_key.sc_pk_peer, 64));
            tlkapi_printf(SMP_DBG_EN, "scoob_remote-r(be):%s", hex_to_str(s_blms_p_own->scoob_remote->random, 16));
            blt_crypto_alg_f4(confirm, smp_ecdh_key.sc_pk_peer, smp_ecdh_key.sc_pk_peer, s_blms_p_own->scoob_remote->random, 0);

            tlkapi_printf(SMP_DBG_EN, "blt_crypto_alg_f4(be):%s", hex_to_str(confirm, 16));

            bool match = (memcmp(confirm, s_blms_p_own->scoob_remote->confirm, sizeof(confirm)) == 0);

            if (!match) {
                //pairing fail event to tell upper layer
                blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_CONFIRM_FAILED); //pairing end with failure

                u8 *pr = blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_CONFIRM_FAILED);
            #if DUAL_CORE_MODE_ENABLED
                extern bool blt_smp_push_tx_fifo(u16 connHandle, smp2llcap_type_t * smp_pkt);
                return blt_smp_push_tx_fifo(connHandle, (smp2llcap_type_t *)pr);
            #else
                return ll_push_tx_fifo_handler(connHandle | HANDLE_STK_FLAG, pr);
            #endif
            }
        }
    }

    return 1;
}

#endif


bool blc_smp_sendKeypressNotify(u16 connHandle, keypress_notify_t ntfType)
{
    u8 smp_master_role = (connHandle & BLM_CONN_HANDLE);
    if (smp_master_role) {
        return FALSE;
    }

    u8               conn_idx       = connHandle & CONN_IDX_MASK;
    u8               smp_status_idx = (connHandle & BLM_CONN_HANDLE) ? 0 : (conn_idx - LL_MAX_ACL_CEN_NUM + 1);
    smp_param_own_t *s_blms_p_own   = (smp_param_own_t *)&smp_param_own[smp_status_idx];
    u8               keyPress       = s_blms_p_own->pairing_req.authReq.keyPress && s_blms_p_own->pairing_rsp.authReq.keyPress;

    if (s_blms_p_own->sc_pairing && keyPress && (smp_sts_param[conn_idx].tk_status & TK_ST_REQUEST)) {
        smpResSignalPkt.l2capLen = 0x02;    //l2cap_len
        smpResSignalPkt.data[0]  = ntfType; //notify value
        smpResSignalPkt.opcode   = SMP_OP_KEYPRESS_NOTIFICATION;
        smpResSignalPkt.rf_len   = smpResSignalPkt.l2capLen + 4;
    #if DUAL_CORE_MODE_ENABLED
        extern bool blt_smp_push_tx_fifo(u16 connHandle, smp2llcap_type_t * smp_pkt);
        return blt_smp_push_tx_fifo(connHandle, &smpResSignalPkt);
    #else
        return ll_push_tx_fifo_handler(connHandle | HANDLE_STK_FLAG, (u8 *)&smpResSignalPkt);
    #endif
    }

    return FALSE;
}

//push smp cmd data to send peer device
void blt_smp_sc_pushPkt_handler(u16 connHandle, u8 type)
{
    if (blms_p_own->sc_pairing) //smp4.2 or above
    {
        u8 smp_master_role = (connHandle & BLM_CONN_HANDLE);

        switch (type) {
        case SMP_OP_PAIRING_CONFIRM:
        {
            u32 sc_passkey = (blms_p_own->pairing_tk[2] << 16 | blms_p_own->pairing_tk[1] << 8 | blms_p_own->pairing_tk[0]);
            /*core4.2 Vol3,Part H, Page2302
                    Z is zero (i.e. 8 bits of zeros) for Numeric Comparison and OOB protocol. In the
                    Passkey Entry protocol, the most significant bit of Z is set equal to one and the
                    least significant bit is made up from one bit of the passkey e.g. if the passkey
                    bit is 1, then Z = 0x81 and if the passkey bit is 0, then Z = 0x80.*/
            u8 f4_param_z;

            //In Secure connections Passkey entry protocol.
            if (blms_p_own->stk_method >= PK_Init_Display_Resp_Input && blms_p_own->stk_method <= PK_BOTH_INPUT) {
                f4_param_z = ((sc_passkey >> blms_p_own->sc_passkey_cnt) & 0x01) | 0x80;
                generateRandomNum(16, blms_p_own->own_rand);

                if (!smp_master_role) {
                    /*
                         * Secure Connection Pairing:
                         * M->S Pairing Req: phase1 begin
                         * S->M Pairing Rsp:
                         * M->S Pairing Public Key: phase2 begin
                         * S->M Pairing Public Key:
                         * ......
                         * M->S Pairing Confirm: (20 times)
                         * S->M Pairing Confirm: (20 times) Pairing Confirm marked
                         */
                    if (blms_p_own->sc_passkey_cnt == 19) { //20 times confirm/random, idx from Zero.
                        if (!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_PUBLIC_KEY))) {
                            goto smpPushSCcmdProcErr;
                        }

                        blms_p_sts->smp_phase_chk = BIT(SMP_OP_PAIRING_CONFIRM);
                    }
                }
            } else { //Numeric Comparison(or Just work) and OOB protocol
                f4_param_z = 0;

                if (!smp_master_role) {
                    /*
                         * Secure Connection Pairing:
                         * M->S Pairing Req: phase1 begin
                         * S->M Pairing Rsp:
                         * M->S Pairing Public Key: phase2 begin
                         * S->M Pairing Public Key:
                         * S->M Pairing Confirm: Pairing Confirm marked
                         */
                    if (!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_PUBLIC_KEY))) {
                        goto smpPushSCcmdProcErr;
                    }

                    blms_p_sts->smp_phase_chk = BIT(SMP_OP_PAIRING_CONFIRM);
                }
            }
            //Cb = f4(PKb,PKa,Nb,0)
            blt_crypto_alg_f4(smpOwnPairingConfirm, smp_ecdh_key.sc_pk_own, smp_ecdh_key.sc_pk_peer, blms_p_own->own_rand, f4_param_z);
            swapN(smpOwnPairingConfirm, 16);
        } break;

        case SMP_OP_PAIRING_RANDOM:
        {
            swapX(blms_p_own->own_rand, smpResSignalPkt.data, 16);

            if (blms_p_own->stk_method >= PK_Init_Display_Resp_Input && blms_p_own->stk_method <= PK_BOTH_INPUT) {
                if (!smp_master_role) {
                    /*
                         * Secure Connection Pairing:
                         * M->S Pairing Req: phase1 begin
                         * S->M Pairing Rsp:
                         * M->S Pairing Public Key: phase2 begin
                         * S->M Pairing Public Key:
                         * ......
                         * M->S Pairing Confirm: (20 times) Pairing Confirm marked
                         * S->M Pairing Confirm: (20 times)
                         * M->S Pairing Random: (20 times) Pairing Random marked
                         * S->M Pairing Random: (20 times)
                         */
                    if (blms_p_own->sc_passkey_cnt == 20) { //20 times confirm/random
                        if (!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_CONFIRM))) {
                            goto smpPushSCcmdProcErr;
                        }

                        blms_p_own->sc_passkey_cnt = 0;
                        blms_p_sts->smp_phase_chk  = BIT(SMP_OP_PAIRING_RANDOM);
                    }
                }
            } else {
#if (SMP_SC_OOB_EN)
                if (blms_p_own->stk_method == SC_OOB_Authentication) {
                    return;
                }
#endif

                if (!smp_master_role) {
                    /* Note: OOB is not take into account
                         * Secure Connection Pairing:
                         * M->S Pairing Req: phase1 begin
                         * S->M Pairing Rsp:
                         * M->S Pairing Public Key: phase2 begin
                         * S->M Pairing Public Key:
                         * S->M Pairing Confirm: Pairing Confirm marked
                         * M->S Pairing Random:
                         * S->M Pairing Random: Pairing Random marked
                         */
                    if (!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_CONFIRM))) {
                        goto smpPushSCcmdProcErr;
                    }

                    blms_p_sts->smp_phase_chk = BIT(SMP_OP_PAIRING_RANDOM);
                }
            }
        } break;

        case SMP_OP_PAIRING_PUBLIC_KEY:
        {
            u8 temp_data[64] = {0};
            swapX(smp_ecdh_key.sc_pk_own, temp_data, 32);
            swapX(smp_ecdh_key.sc_pk_own + 32, temp_data + 32, 32);
        #ifdef MCU_CORE_D25F_ENABLE
            blc_smp_pushDataSplit(connHandle, SMP_OP_PAIRING_PUBLIC_KEY, temp_data, 64); // need packet split
        #else
            blc_smp_pushDataSplit(connHandle | HANDLE_STK_FLAG, SMP_OP_PAIRING_PUBLIC_KEY, temp_data, 64); // need packet split
        #endif
        } break;

        case SMP_OP_PAIRING_DHKEY:
        {
            if (smp_master_role) {
                u8 bd_addr_init[7] = {0};
                u8 bd_addr_rsp[7]  = {0};
                u8 ioCapA[3]       = {0};
                u8 confirm_ea[16]  = {0};

                //A = BD_ADDR of A used during pairing
                bd_addr_init[0] = blms_p_own->own_addr_type;
                swapX(blms_p_own->own_conn_addr, bd_addr_init + 1, 6);

                //B = BD_ADDR of B used during pairing
                bd_addr_rsp[0] = blms_p_peer->peer_addr_type;
                swapX(blms_p_peer->peer_conn_addr, bd_addr_rsp + 1, 6);

                //MacKey || LTK = f5(DHKey, Na, Nb,A,B)
                blt_crypto_alg_f5(blms_p_own->macKey, blms_p_own->own_ltk, smp_ecdh_key.sc_sk_dhk_own, blms_p_own->own_rand, blms_p_peer->peer_pairing_rand, bd_addr_init, bd_addr_rsp);
                swapN(blms_p_own->own_ltk, 16);

                ///////////////////////////// compute confirm Ea ////////////////////////////////////
                //pairing_tk: in security connection to keep own random. rb
                //pairing_confirm: in security connection oob mode to keep peer random. ra

                //IOcapA is from Pairing Request
                ioCapA[0] = blms_p_own->pairing_req.authReq.authType;
                ioCapA[1] = blms_p_own->pairing_req.oobDataFlag;
                ioCapA[2] = blms_p_own->pairing_req.ioCapability;

                if (blms_p_own->stk_method >= PK_Init_Display_Resp_Input && blms_p_own->stk_method <= PK_BOTH_INPUT) {
                    swapN(blms_p_own->pairing_tk, 16);     //swap pairing_tk
                } else {
                    memset(blms_p_own->pairing_tk, 0, 16); //clear pairing_tk
                }

                u8 *rb = blms_p_own->pairing_tk;

#if (SMP_SC_OOB_EN)
                if (blms_p_own->stk_method == SC_OOB_Authentication) {
                    //if Device A's IO data flag does not indicate OOB authentication data present, set rb = 0.
                    //if local master SC OOB flag exist, remote SC OOB data used, else set rb = 0.
                    if (blms_p_own->scoob_remote) {
                        rb = blms_p_own->scoob_remote->random;
                    }
                    tlkapi_printf(SMP_DBG_EN, "M Ea check: rb (be) %s", hex_to_str(rb, 16));
                }
#endif

                //Ea = f6(MacKey,Na,Nb,rb,IOcapA,A,B)
                blt_crypto_alg_f6(confirm_ea, blms_p_own->macKey, blms_p_own->own_rand, blms_p_peer->peer_pairing_rand, rb, ioCapA, bd_addr_init, bd_addr_rsp);

                swapX(confirm_ea, smpResSignalPkt.data, 16);
            } else {
                /*
                     * Secure Connection Pairing:
                     * M->S Pairing Req: phase1 begin
                     * S->M Pairing Rsp:
                     * M->S Pairing Public Key: phase2 begin
                     * S->M Pairing Public Key:
                     * ......
                     * M->S Pairing Random:
                     * S->M Pairing Random: Pairing Random marked
                     * M->S Pairing DHKey Check: Pairing DHKey Check marked
                     */
                if (!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_RANDOM))) {
                    goto smpPushSCcmdProcErr;
                }

                blms_p_sts->smp_phase_chk = BIT(SMP_OP_PAIRING_DHKEY);

                swapX(blms_p_peer->peer_confirm, smpResSignalPkt.data, 16); //Eb Reuse Parameter: peer_confirm
            }

            smpResSignalPkt.l2capLen = 0x11;                                //l2cap len
        } break;

        default:
            goto smpPushSCcmdProcErr;
            break;
        }

        return;
    }

smpPushSCcmdProcErr:
    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_UNSPECIFIED_REASON); //pairing end with failure
    u8 *p = blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_UNSPECIFIED_REASON);
#if DUAL_CORE_MODE_ENABLED
    extern bool blt_smp_push_tx_fifo(u16 connHandle, smp2llcap_type_t * smp_pkt);
    blt_smp_push_tx_fifo(connHandle, (smp2llcap_type_t *)p);
#else
    ll_push_tx_fifo_handler(connHandle | HANDLE_STK_FLAG, p);
#endif
}

//smp cmd data receive from peer device
u8 *blt_smp_sc_handler(u16 connHandle, u8 *p) //l2cap layer process
{
    u8 smp_master_role = (connHandle & BLM_CONN_HANDLE);
    u8 conn_idx        = connHandle & CONN_IDX_MASK;

    u8 slave_dev_idx = blt_gap_getSlaveDeviceIndex_by_connIdx(conn_idx);

    if (blms_p_own->sc_pairing)                                  //smp4.2 or above
    {
        rf_packet_l2cap_req_t *req = (rf_packet_l2cap_req_t *)p; //important not include dma 4byte!!!
        u8                     param_evt[8];

        switch (req->opcode)                                     //smp4.2 or above
        {
        case SMP_OP_PAIRING_REQ:
        {
            if (!smp_master_role) {
#if SMP_SC_OOB_EN
                if (blms_p_own->stk_method == SC_OOB_Authentication) {
                    //Request SC OOB data event
                    if (!(gap_eventMask & GAP_EVT_MASK_SMP_REQUEST_SCOOB_DATA)) {
                        //pairing fail event to tell upper layer
                        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_OOB_NOT_AVAILABLE); //pairing end with failure
                        return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_OOB_NOT_AVAILABLE);
                    }
                } else
#endif
                {
                    if (!blt_ecc_gen_key_pair(smp_ecdh_key.sc_pk_own, smp_ecdh_key.sc_sk_dhk_own, ECC_use_secp256r1, blms_p_prop->ecdh_debug_mode)) {
                        //pairing fail event to tell upper layer
                        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_UNSPECIFIED_REASON); //pairing end with failure
                        return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_UNSPECIFIED_REASON);   //
                    }
                }
//this is added by YaFei
#if (SMP_DATABASE_INFO_SOURCE == SMP_INFO_STORAGE_IN_FLASH)

                u32 flash_addr = blt_smp_searchBondingDevice_by_PeerMacAddr(smp_master_role, slave_dev_idx, blms_p_peer->peer_addr_type, blms_p_peer->peer_conn_addr);
                if (flash_addr) {
                    //should delete the older smp banding info
                    u16 index = blt_smp_getBondingIndex_by_FlashAddr(smp_master_role, slave_dev_idx, flash_addr);
                    if (index != ADDR_NOT_BONDED) {
                        blt_smp_deleteBondingInfo_by_Index(smp_master_role, slave_dev_idx, index, true);
                    }
                }

                //blms_p_own->auth_req.authType = blms_p_own->pairing_req.authReq.authType & blms_p_own->pairing_rsp.authReq.authType;
#else

#endif
            } else {
                goto smpSCcmdProcErr;
            }
        } break;

        case SMP_OP_PAIRING_RSP:
        {
            if (smp_master_role) {
#if SMP_SC_OOB_EN
                if (blms_p_own->stk_method == SC_OOB_Authentication) {
                    //Request SC OOB data event
                    if (!(gap_eventMask & GAP_EVT_MASK_SMP_REQUEST_SCOOB_DATA)) {
                        //pairing fail event to tell upper layer
                        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_OOB_NOT_AVAILABLE); //pairing end with failure
                        return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_OOB_NOT_AVAILABLE);
                    }
                } else
#endif
                {
                    if (!blt_ecc_gen_key_pair(smp_ecdh_key.sc_pk_own, smp_ecdh_key.sc_sk_dhk_own, ECC_use_secp256r1, blms_p_prop->ecdh_debug_mode)) {
                        //pairing fail event to tell upper layer
                        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_UNSPECIFIED_REASON); //pairing end with failure
                        return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_UNSPECIFIED_REASON);   //
                    }
                }
            } else {
                goto smpSCcmdProcErr;
            }
        } break;

        case SMP_OP_PAIRING_CONFIRM:
        {
            swapX(req->data, blms_p_peer->peer_confirm, 16);

            if (smp_master_role) {
                //In Secure connections Passkey entry protocol.
                if (blms_p_own->stk_method >= PK_Init_Display_Resp_Input && blms_p_own->stk_method <= PK_BOTH_INPUT) {
                    /*
                         * Secure Connection Pairing:
                         * M->S Pairing Req: phase1 begin
                         * S->M Pairing Rsp:
                         * M->S Pairing Public Key: phase2 begin
                         * S->M Pairing Public Key:
                         * ......
                         * M->S Pairing Confirm: (20 times)
                         * S->M Pairing Confirm: (20 times) Pairing Confirm marked
                         */
                    if (blms_p_own->sc_passkey_cnt == 19) { //20 times confirm/random, idx from Zero.
                        if (!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_PUBLIC_KEY))) {
                            goto smpSCcmdProcErr;
                        }

                        blms_p_sts->smp_phase_chk = BIT(SMP_OP_PAIRING_CONFIRM);
                    }
                } else { //Numeric Comparison(or Just work) and OOB protocol
                    /*
                         * Secure Connection Pairing:
                         * M->S Pairing Req: phase1 begin
                         * S->M Pairing Rsp:
                         * M->S Pairing Public Key: phase2 begin
                         * S->M Pairing Public Key:
                         * S->M Pairing Confirm: Pairing Confirm marked
                         */
                    if (!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_PUBLIC_KEY))) {
                        goto smpSCcmdProcErr;
                    }

                    blms_p_sts->smp_phase_chk = BIT(SMP_OP_PAIRING_CONFIRM);
                }
            }
        } break;

        case SMP_OP_PAIRING_RANDOM:
        {
            swapX(req->data, blms_p_peer->peer_pairing_rand, 16);

            u8  pairing_conf[16] = {0};
            u32 sc_passkey       = 0;
            u8  f4_param_z       = 0;


            /*
                 * 1.sc passkey entry method:
                 *    M->S Pairing Random: (slave role: after recv random , sc_passkey_cnt++)
                 *    S->M Pairing Random: (master role: after recv random , sc_passkey_cnt++)
                 * 2.other methods:
                 *    sc_passkey_cnt must be zero
                 */
            if (!blms_p_own->sc_passkey_cnt) {
                //Private and DHkey use the same buffer, after obtaining dhkey, the private key content will be overwritten by dhkey
                if (!blt_ecc_gen_dhkey(smp_ecdh_key.sc_pk_peer, smp_ecdh_key.sc_sk_dhk_own, smp_ecdh_key.sc_sk_dhk_own, ECC_use_secp256r1)) {
                    //pairing fail event to tell upper layer
                    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_INVALID_PARAMETER); //pairing end with failure
                    return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_INVALID_PARAMETER);
                }
            }

#if (SMP_SC_OOB_EN)
            if (blms_p_own->stk_method == SC_OOB_Authentication) {
                if (!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_PUBLIC_KEY))) {
                    goto smpSCcmdProcErr;
                }

                blms_p_sts->smp_phase_chk = BIT(SMP_OP_PAIRING_RANDOM);

                if (smp_master_role) { //master SC OOB
                    return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_DHKEY);
                } else {               //slave  SC OOB
                    return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_RANDOM);
                }
            }
#endif

            if (smp_master_role) {
                //In Secure connections Passkey entry protocol.
                if (blms_p_own->stk_method >= PK_Init_Display_Resp_Input && blms_p_own->stk_method <= PK_BOTH_INPUT) {
                    sc_passkey = (blms_p_own->pairing_tk[2] << 16 | blms_p_own->pairing_tk[1] << 8 | blms_p_own->pairing_tk[0]);
                    f4_param_z = ((sc_passkey >> blms_p_own->sc_passkey_cnt) & 0x01) | 0x80;
                    blms_p_own->sc_passkey_cnt++;
                } else { //Numeric Comparison(or Just work) and OOB protocol
                    f4_param_z = 0;
                }

                //Cbi = f4(PKb,PKa,Nbi,rai)
                blt_crypto_alg_f4(pairing_conf, smp_ecdh_key.sc_pk_peer, smp_ecdh_key.sc_pk_own, blms_p_peer->peer_pairing_rand, f4_param_z);

                //check if Cbi=f4(Pkb,Pka,Nbi,rai).if check fails,about.
                if (memcmp(pairing_conf, blms_p_peer->peer_confirm, 16) || blms_p_own->sc_passkey_cnt > 20) {
                    blms_p_own->sc_passkey_cnt = 0;

                    //pairing fail event to tell upper layer
                    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_CONFIRM_FAILED); //pairing end with failure
                    return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_CONFIRM_FAILED);
                }

                //Check Cbi=f4(Pkb,Pka,Nbi,rai) OK
                if (blms_p_own->stk_method == Numeric_Comparison) {
                    // when numeric comparison mode used , calc 6-digit confirm value here.
                    // Vb = g2(PKa, PKb, Na, Nb)
                    u32 pinCode = blt_crypto_alg_g2(smp_ecdh_key.sc_pk_own, smp_ecdh_key.sc_pk_peer, blms_p_own->own_rand, blms_p_peer->peer_pairing_rand); //numeric comparison
                    swapN((u8 *)&pinCode, 4);
                    pinCode = pinCode % 1000000;

                    //Slave displays 6 bit  numeric comparison value, start a special task
                    //waiting for slave confirmation: 'YES'(pairing) or 'NO'(cancel).
                    blms_p_sts->tk_status = TK_ST_NUMERIC_COMPARE;
                    if (gap_eventMask & GAP_EVT_MASK_SMP_TK_NUMERIC_COMPARE) {
                        // send Vb to upper layer, upper layer should display it, and  confirm check if this
                        // value match peer device's displaying result, then confirm it by calling
                        // "" sending YES or NO to smp layer

                        gap_smp_TkDisplayEvt_t *pEvt = (gap_smp_TkDisplayEvt_t *)param_evt;
                        pEvt->connHandle             = connHandle;
                        pEvt->tk_pincode             = pinCode;
                        blc_gap_send_event(GAP_EVT_SMP_TK_NUMERIC_COMPARE, param_evt, sizeof(gap_smp_TkDisplayEvt_t));
                    }
                } else if (blms_p_own->stk_method >= PK_Init_Display_Resp_Input && blms_p_own->stk_method <= PK_BOTH_INPUT) {
                    if (blms_p_own->sc_passkey_cnt >= 20) {
                        blms_p_own->sc_passkey_cnt = 0;

                        if (!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_CONFIRM))) {
                            goto smpSCcmdProcErr;
                        }

                        blms_p_sts->smp_phase_chk = BIT(SMP_OP_PAIRING_RANDOM);

                        return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_DHKEY);
                    } else {
                        return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_CONFIRM);
                    }
                }

                if (!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_CONFIRM))) {
                    goto smpSCcmdProcErr;
                }

                blms_p_sts->smp_phase_chk = BIT(SMP_OP_PAIRING_RANDOM);

                //if select numeric comparison, to execute here, Master must already confirmed 'YES'(Pairing).
                if (blms_p_own->stk_method == Numeric_Comparison) {
                    if (blms_p_sts->tk_status & TK_ST_NUMERIC_COMPARE) {
                        //NC confirmed "YES", set by upper layer completed
                        if (blms_p_sts->tk_status & TK_ST_NUMERIC_CHECK_YES) {
                            blms_p_sts->tk_status = 0;
                        }
                        //NC confirmed "NO", set by upper layer completed
                        else if (blms_p_sts->tk_status & TK_ST_NUMERIC_CHECK_NO) {
                            blms_p_sts->tk_status = 0;

                            //pairing fail event to tell upper layer
                            blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_NUMERIC_FAILED); //pairing end with failure

                            //refer to BLE Core Specification: Vol 3, Part H, "3.5.5 Pairing Failed" for more information.
                            //NOTICE: test by smart phone, master send unspecified reason when press "NO" button!
                            return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_NUMERIC_FAILED); // unsure??
                        }
                        // check it in gap mainLoop, if NC confirmed "YES", send peer_confirm to peer device
                        else {
                            blms_p_sts->tk_status |= TK_ST_NUMERIC_DHKEY_SUCC_PENDING;
                            return NULL;
                        }
                    }
                }

                return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_DHKEY);
            } else {
                //In Numeric Comparison protocol
                if (blms_p_own->stk_method == Numeric_Comparison) {
                    // when numeric comparison mode used , calc 6-digit confirm value here.
                    // Vb = g2(PKa, PKb, Na, Nb)
                    u32 pinCode = blt_crypto_alg_g2(smp_ecdh_key.sc_pk_peer, smp_ecdh_key.sc_pk_own, blms_p_peer->peer_pairing_rand, blms_p_own->own_rand); //numeric comparison
                    swapN((u8 *)&pinCode, 4);
                    pinCode = pinCode % 1000000;

                    //Slave displays 6 bit  numeric comparison value, start a special task
                    //waiting for slave confirmation: 'YES'(pairing) or 'NO'(cancel).
                    blms_p_sts->tk_status = TK_ST_NUMERIC_COMPARE;

                    if (gap_eventMask & GAP_EVT_MASK_SMP_TK_NUMERIC_COMPARE) {
                        // send Vb to upper layer, upper layer should display it, and  confirm check if this
                        // value match peer device's displaying result, then confirm it by calling
                        // "" sending YES or NO to smp layer

                        gap_smp_TkDisplayEvt_t *pEvt = (gap_smp_TkDisplayEvt_t *)param_evt;
                        pEvt->connHandle             = connHandle;
                        pEvt->tk_pincode             = pinCode;
                        blc_gap_send_event(GAP_EVT_SMP_TK_NUMERIC_COMPARE, param_evt, sizeof(gap_smp_TkDisplayEvt_t));
                    }
                }
                //In Secure connections Passkey entry protocol.
                else if (blms_p_own->stk_method >= PK_Init_Display_Resp_Input && blms_p_own->stk_method <= PK_BOTH_INPUT) {
                    sc_passkey = (blms_p_own->pairing_tk[2] << 16 | blms_p_own->pairing_tk[1] << 8 | blms_p_own->pairing_tk[0]);
                    /*core4.2 Vol3,Part H, Page2302
                          Z is zero (i.e. 8 bits of zeros) for Numeric Comparison and OOB protocol. In the Passkey Entry protocol,
                          the most significant bit of Z is set equal to one and the least significant bit is made up from one bit
                          of the passkey e.g. if the passkey bit is 1, then Z = 0x81 and if the passkey bit is 0, then Z = 0x80.*/
                    f4_param_z = ((sc_passkey >> blms_p_own->sc_passkey_cnt) & 0x01) | 0x80;

                    blms_p_own->sc_passkey_cnt++;

                    //Cai = f4(PKa,PKb,Nai,rbi)
                    blt_crypto_alg_f4(pairing_conf, smp_ecdh_key.sc_pk_peer, smp_ecdh_key.sc_pk_own, blms_p_peer->peer_pairing_rand, f4_param_z);

                    //check if Cai=f4(Pka,Pkb,Nai,rbi).if check fails,about.
                    if (memcmp(pairing_conf, blms_p_peer->peer_confirm, 16) || blms_p_own->sc_passkey_cnt > 20) {
                        blms_p_own->sc_passkey_cnt = 0;

                        //pairing fail event to tell upper layer
                        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_CONFIRM_FAILED); //pairing end with failure
                        return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_CONFIRM_FAILED);
                    }
                }

                return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_RANDOM);
            }
        } break;

        case SMP_OP_PAIRING_PUBLIC_KEY:
        {
            swapX(req->data, smp_ecdh_key.sc_pk_peer, 32);
            swapX(req->data + 32, smp_ecdh_key.sc_pk_peer + 32, 32);

            /*
                 * Secure Connection Pairing:
                 * M->S Pairing Req: phase1 begin
                 * S->M Pairing Rsp:
                 * M->S Pairing Public Key: phase2 begin
                 * S->M Pairing Public Key:
                 */
            if (blms_p_sts->smp_phase_chk != PAIRING_PHASE_1_OK) {
                goto smpSCcmdProcErr;
            }

            blms_p_sts->smp_phase_chk = BIT(SMP_OP_PAIRING_PUBLIC_KEY);

            if (smp_master_role) {
                /*
                     * Fix << CVE-2020-26558 >>
                     * Issue:
                     * . The researchers identified that it was possible for an attacker acting as a MITM in the Passkey
                     *   authentication procedure to use a crafted series of responses to determine each bit of the randomly
                     *   generated Passkey selected by the pairing initiator in each round of the pairing procedure, and
                     *   once identified, to use these Passkey bits during the same pairing session to successfully complete
                     *   the authenticated pairing procedure with the responder.
                     *
                     * The Bluetooth Special Interest Group (SIG) is making the following recommendations for circumventing
                     * this attack:
                     *  For the attack to succeed it is necessary for the pairing device to accept the same public key that it
                     *  provided to the remote peer as the remote peer's public key. Devices should not accept their own public
                     *  key from a peer during a pairing session. The pairing procedure should be terminated with a failure
                     *  status if this occurs.
                     */
                if (memcmp(smp_ecdh_key.sc_pk_own, smp_ecdh_key.sc_pk_peer, 64) == 0) {
                    //pairing fail event to tell upper layer
                    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_INVALID_PARAMETER); //pairing end with failure
                    return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_INVALID_PARAMETER);
                }

                if (blms_p_own->stk_method != Numeric_Comparison && blms_p_own->stk_method != JustWorks) {
                    if (blms_p_own->stk_method == PK_Resp_Display_Init_Input || blms_p_own->stk_method == PK_BOTH_INPUT) {
                        blms_p_sts->tk_status = TK_ST_REQUEST;
                        if (gap_eventMask & GAP_EVT_MASK_SMP_TK_REQUEST_PASSKEY) {
                            gap_smp_TkReqPassKeyEvt_t *pEvt = (gap_smp_TkReqPassKeyEvt_t *)param_evt;
                            pEvt->connHandle                = connHandle;
                            blc_gap_send_event(GAP_EVT_SMP_TK_REQUEST_PASSKEY, param_evt, sizeof(gap_smp_TkReqPassKeyEvt_t));
                        }

                        if ((blms_p_sts->tk_status & TK_ST_REQUEST) && !(blms_p_sts->tk_status & TK_ST_UPDATE)) {
                            blms_p_sts->tk_status |= TK_ST_CONFIRM_PENDING; //pending
                            return NULL;
                        }
                    }
#if (SMP_SC_OOB_EN)
                    else if (blms_p_own->stk_method == SC_OOB_Authentication) { //master SC OOB
                        //sc_oob confirm check
                        if (blms_p_own->scoob_remote) {
                            u8 confirm[16];
                            //Confirm = f4(PK,PK,random,0)
                            tlkapi_printf(SMP_DBG_EN, "sc_pk_peer(be):%s", hex_to_str(smp_ecdh_key.sc_pk_peer, 64));
                            tlkapi_printf(SMP_DBG_EN, "scoob_remote-r(be):%s", hex_to_str(blms_p_own->scoob_remote->random, 16));
                            blt_crypto_alg_f4(confirm, smp_ecdh_key.sc_pk_peer, smp_ecdh_key.sc_pk_peer, blms_p_own->scoob_remote->random, 0);

                            tlkapi_printf(SMP_DBG_EN, "blt_crypto_alg_f4(be):%s", hex_to_str(confirm, 16));

                            bool match = (memcmp(confirm, blms_p_own->scoob_remote->confirm, sizeof(confirm)) == 0);

                            if (!match) {
                                //pairing fail event to tell upper layer
                                blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_CONFIRM_FAILED); //pairing end with failure
                                return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_CONFIRM_FAILED);
                            }
                        }

                        return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_RANDOM);
                    }
#endif

                    return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_CONFIRM);
                }
            } else {
#if (SMP_SC_OOB_EN)
                if (blms_p_own->stk_method == SC_OOB_Authentication) {
                    //Request SC OOB data event
                    if (gap_eventMask & GAP_EVT_MASK_SMP_REQUEST_SCOOB_DATA) {
                        blms_p_sts->tk_status = TK_ST_REQUEST;
                        //oob data check
                        bool scOobLocalUsed, scOobRemoteUsed;
                        //see peer's flag decide own sc oob data used
                        scOobLocalUsed  = (blms_p_own->pairing_req.oobDataFlag);
                        scOobRemoteUsed = (blms_p_own->pairing_rsp.oobDataFlag);

                        //clear
                        blms_p_own->scoob_local     = NULL;
                        blms_p_own->scoob_remote    = NULL;
                        blms_p_own->scoob_local_key = NULL;

                        gap_smp_requestScOobDataEvt_t *pEvt = (gap_smp_requestScOobDataEvt_t *)param_evt;
                        pEvt->connHandle                    = connHandle;
                        pEvt->scOobLocalUsed                = scOobLocalUsed;
                        pEvt->scOobRemoteUsed               = scOobRemoteUsed;
                        blc_gap_send_event(GAP_EVT_SMP_REQUEST_SCOOB_DATA, param_evt, sizeof(gap_smp_requestScOobDataEvt_t));

                        return NULL; //Attention: to do pending process, here do noting
                    }
                }
#endif

                blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_PUBLIC_KEY);
                if (blms_p_own->stk_method == Numeric_Comparison || blms_p_own->stk_method == JustWorks) {
                    return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_CONFIRM);
                } else if (blms_p_own->stk_method == PK_BOTH_INPUT || blms_p_own->stk_method == PK_Init_Display_Resp_Input) {
                    // both sides should input TK value, here send TK request event to upper layer,
                    // expect upper layer call "blc_smp_setTK_by_PasskeyEntry"  to set TK value
                    blms_p_sts->tk_status = TK_ST_REQUEST;
                    if (gap_eventMask & GAP_EVT_MASK_SMP_TK_REQUEST_PASSKEY) {
                        gap_smp_TkReqPassKeyEvt_t *pEvt = (gap_smp_TkReqPassKeyEvt_t *)param_evt;
                        pEvt->connHandle                = connHandle;
                        blc_gap_send_event(GAP_EVT_SMP_TK_REQUEST_PASSKEY, param_evt, sizeof(gap_smp_TkReqPassKeyEvt_t));
                    }
                }
            }
        } break;

        case SMP_OP_PAIRING_DHKEY:
        {
            u8 confirm_Ex[16] = {0};
            swapX(req->data, confirm_Ex, 16);

            u8 bd_addr_init[7] = {0};
            u8 bd_addr_rsp[7]  = {0};
            u8 ioCapA[3]       = {0};
            u8 ioCapB[3]       = {0};

            if (smp_master_role) {
                /*
                     * Secure Connection Pairing:
                     * M->S Pairing Req: phase1 begin
                     * S->M Pairing Rsp:
                     * M->S Pairing Public Key: phase2 begin
                     * S->M Pairing Public Key:
                     * ......
                     * M->S Pairing Random:
                     * S->M Pairing Random: Pairing Random marked
                     * M->S Pairing DHKey Check:
                     * S->M Pairing DHKey Check: Pairing DHKey Check marked
                     */
                if (!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_RANDOM))) {
                    goto smpSCcmdProcErr;
                }

                blms_p_sts->smp_phase_chk = BIT(SMP_OP_PAIRING_DHKEY);

                u8 confirm_eb[16] = {0};
                //A = BD_ADDR of A used during pairing
                bd_addr_init[0] = blms_p_own->own_addr_type;
                swapX(blms_p_own->own_conn_addr, bd_addr_init + 1, 6);

                //B = BD_ADDR of B used during pairing
                bd_addr_rsp[0] = blms_p_peer->peer_addr_type;
                swapX(blms_p_peer->peer_conn_addr, bd_addr_rsp + 1, 6);

                //check confirm Eb.
                //pairing_tk: in security connection to keep own random. rb
                //pairing_confirm: in security connection oob mode to keep peer random. ra
                ioCapA[0] = blms_p_own->pairing_req.authReq.authType;
                ioCapA[1] = blms_p_own->pairing_req.oobDataFlag;
                ioCapA[2] = blms_p_own->pairing_req.ioCapability;

                ioCapB[0] = blms_p_own->pairing_rsp.authReq.authType;
                ioCapB[1] = blms_p_own->pairing_rsp.oobDataFlag;
                ioCapB[2] = blms_p_own->pairing_rsp.ioCapability;

                u8 *ra = blms_p_own->pairing_tk;

#if (SMP_SC_OOB_EN)
                if (blms_p_own->stk_method == SC_OOB_Authentication) {
                    //if Device B's IO data flag does not indicate OOB authentication data present, set ra = 0.
                    //if remote SC OOB flag exist, local master SC OOB data used, else ra = 0.
                    if (blms_p_own->scoob_local) {
                        ra = blms_p_own->scoob_local->random;
                    }
                    tlkapi_printf(SMP_DBG_EN, "M Eb check: ra (be) %s", hex_to_str(ra, 16));
                }
#endif

                //Eb = f6(MacKey,Nb,Na,ra,IOcapB,B,A)
                blt_crypto_alg_f6(confirm_eb, blms_p_own->macKey, blms_p_peer->peer_pairing_rand, blms_p_own->own_rand, ra, ioCapB, bd_addr_rsp, bd_addr_init);

                // check DHkey parameter Eb fails
                if (memcmp(confirm_eb, confirm_Ex, 16)) {
                    //pairing fail event to tell upper layer
                    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_DHKEY_CHECK_FAIL); //pairing end with failure
                    //check if Ea = f6(MacKey,Na,Nb,rb,IOcapA,A,B).If check fails,abort
                    return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_DHKEY_CHECK_FAIL);
                } else {
                    if (!(blms_p_sts->smp_phase_chk & BIT(SMP_OP_PAIRING_DHKEY))) {
                        goto smpSCcmdProcErr;
                    }

                    blms_p_sts->smp_phase_chk = PAIRING_PHASE_2_ENC;

                    /*
                         * The SC does not need to store the counterpart's LTK. Considering that the legacy pairing stores
                         * the LTK  of the peer, in the case of SC, the local LTK is copied to the peer LTK for storage. (
                         * In the case of SC, the master and slave LTK are the same).
                         */
                    smemcpy(blms_p_peer->peer_ltk, blms_p_own->own_ltk, 16);

                    /*
                         * SC does not need to store EDIV and RAND, it is cleared here.
                         */
                    blms_p_peer->peer_ediv = 0;
                    memset(blms_p_peer->peer_random, 0, 8);
#if DUAL_CORE_MODE_ENABLED
                    struct ble_hci_le_start_encrypt_cp start_enc_cp;
                    smemcpy(start_enc_cp.ltk, blms_p_own->own_ltk, 16);
                    start_enc_cp.div = blms_p_peer->peer_ediv;
                    // start_enc_cp.rand = *(u64 *)blms_p_peer->peer_random;
                    memcpy(&(start_enc_cp.rand), blms_p_peer->peer_random, 8);
                    start_enc_cp.conn_handle = connHandle;
                    ble_host_hci_le_start_encryption(&start_enc_cp);
#else
                    blt_ll_startEncryption(connHandle, blms_p_peer->peer_ediv, blms_p_peer->peer_random, blms_p_own->own_ltk);
#endif
                    //smp4.2 or above, after exchange smp pairing DH key check, then transport specific keys distribution
                    blms_p_sts->smpDistributeKeyOrder = SMP_TRANSPORT_SPECIFIC_KEY_START;
                }
            } else {
                u8 confirm_ea[16] = {0};
                //A = BD_ADDR of A used during pairing
                bd_addr_init[0] = blms_p_peer->peer_addr_type;
                swapX(blms_p_peer->peer_conn_addr, bd_addr_init + 1, 6);

                //B = BD_ADDR of B used during pairing
                bd_addr_rsp[0] = blms_p_own->own_addr_type;
                swapX(blms_p_own->own_conn_addr, bd_addr_rsp + 1, 6);

                //MacKey || LTK = f5(DHKey, Na, Nb,A,B)
                blt_crypto_alg_f5(blms_p_own->macKey, blms_p_own->own_ltk, smp_ecdh_key.sc_sk_dhk_own, blms_p_peer->peer_pairing_rand, blms_p_own->own_rand, bd_addr_init, bd_addr_rsp);
                swapN(blms_p_own->own_ltk, 16);

                //Compute confirm Ea.
                //pairing_tk: in security connection to keep own random. rb
                //peer_confirm: in security connection oob mode to keep peer random. ra
                ioCapA[0] = blms_p_own->pairing_req.authReq.authType;
                ioCapA[1] = blms_p_own->pairing_req.oobDataFlag;
                ioCapA[2] = blms_p_own->pairing_req.ioCapability;

                ioCapB[0] = blms_p_own->pairing_rsp.authReq.authType;
                ioCapB[1] = blms_p_own->pairing_rsp.oobDataFlag;
                ioCapB[2] = blms_p_own->pairing_rsp.ioCapability;

                if (blms_p_own->stk_method >= PK_Init_Display_Resp_Input && blms_p_own->stk_method <= PK_BOTH_INPUT) {
                    swapN(blms_p_own->pairing_tk, 16);     //swap pairing_tk
                } else {
                    memset(blms_p_own->pairing_tk, 0, 16); //clear pairing_tk
                }

                u8 *rb = blms_p_own->pairing_tk;

#if (SMP_SC_OOB_EN)
                if (blms_p_own->stk_method == SC_OOB_Authentication) {
                    //if Device A's IO data flag does not indicate OOB authentication data present, set rb = 0.
                    //if remote master SC OOB flag exist, local SC OOB data used, else set rb = 0.
                    if (blms_p_own->scoob_local) {
                        rb = blms_p_own->scoob_local->random;
                    }
                    tlkapi_printf(SMP_DBG_EN, "S Ea check: rb (be) %s", hex_to_str(rb, 16));
                }
#endif

                //Ea = f6(MacKey,Na,Nb,rb,IOcapA,A,B)
                blt_crypto_alg_f6(confirm_ea, blms_p_own->macKey, blms_p_peer->peer_pairing_rand, blms_p_own->own_rand, rb, ioCapA, bd_addr_init, bd_addr_rsp);

                //tlkapi_printf(SMP_DBG_EN, "confirm_ea:(be) %s", hex_to_str(confirm_ea, 16));

                // check DHkey parameter Ea.
                if (memcmp(confirm_ea, confirm_Ex, 16)) {
                    //if select numeric comparison, to execute here, Master must already confirmed 'YES'(Pairing).
                    if (blms_p_own->stk_method == Numeric_Comparison) {
                        if (blms_p_sts->tk_status & TK_ST_NUMERIC_COMPARE) {
                            //NC confirmed "YES", set by upper layer completed
                            if (blms_p_sts->tk_status & TK_ST_NUMERIC_CHECK_YES) {
                                blms_p_sts->tk_status = 0;
                            }
                            //NC confirmed "NO", set by upper layer completed
                            else if (blms_p_sts->tk_status & TK_ST_NUMERIC_CHECK_NO) {
                                blms_p_sts->tk_status = 0;

                                //pairing fail event to tell upper layer
                                blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_NUMERIC_FAILED); //pairing end with failure

                                //refer to BLE Core Specification: Vol 3, Part H, "3.5.5 Pairing Failed" for more information.
                                //NOTICE: test by smart phone, master send unspecified reason when press "NO" button!
                                return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_NUMERIC_FAILED); // unsure??
                            }
                            // check it in gap mainLoop, if NC confirmed "YES", send peer_confirm to peer device
                            else {
                                blms_p_sts->tk_status |= TK_ST_NUMERIC_DHKEY_FAIL_PENDING;
                                return NULL;
                            }
                        }
                    }

                    //pairing fail event to tell upper layer
                    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_DHKEY_CHECK_FAIL); //pairing end with failure

                    // check fail
                    //check if Ea = f6(MacKey,Na,Nb,rb,IOcapA,A,B).If check fails,abort
                    return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_DHKEY_CHECK_FAIL);
                } else {
                    u8 *ra = blms_p_own->pairing_tk;

#if (SMP_SC_OOB_EN)
                    if (blms_p_own->stk_method == SC_OOB_Authentication) {
                        //if Device B's IO data flag does not indicate OOB authentication data present, set ra = 0.
                        //if local SC OOB flag exist, remote master SC OOB data used, else ra = 0.
                        if (blms_p_own->scoob_remote) {
                            ra = blms_p_own->scoob_remote->random;
                        }
                        tlkapi_printf(SMP_DBG_EN, "S Eb check: ra (be) %s", hex_to_str(ra, 16));
                    }
#endif

                    // check pass
                    // 10b. compute Eb , keep in confirm temp
                    //Eb = f6(MacKey,Nb,Na,ra,IOcapB,B,A)
                    blt_crypto_alg_f6(blms_p_peer->peer_confirm, blms_p_own->macKey, blms_p_own->own_rand, blms_p_peer->peer_pairing_rand, ra, ioCapB, bd_addr_rsp, bd_addr_init);

                    //if select numeric comparison, to execute here, Master must already confirmed 'YES'(Pairing).
                    if (blms_p_own->stk_method == Numeric_Comparison) {
                        if (blms_p_sts->tk_status & TK_ST_NUMERIC_COMPARE) {
                            //NC confirmed "YES", set by upper layer completed
                            if (blms_p_sts->tk_status & TK_ST_NUMERIC_CHECK_YES) {
                                blms_p_sts->tk_status = 0;
                            }
                            //NC confirmed "NO", set by upper layer completed
                            else if (blms_p_sts->tk_status & TK_ST_NUMERIC_CHECK_NO) {
                                blms_p_sts->tk_status = 0;

                                //pairing fail event to tell upper layer
                                blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_NUMERIC_FAILED); //pairing end with failure

                                //refer to BLE Core Specification: Vol 3, Part H, "3.5.5 Pairing Failed" for more information.
                                //NOTICE: test by smart phone, master send unspecified reason when press "NO" button!
                                return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_NUMERIC_FAILED); // unsure??
                            }
                            // check it in gap mainLoop, if NC confirmed "YES", send peer_confirm to peer device
                            else {
                                blms_p_sts->tk_status |= TK_ST_NUMERIC_DHKEY_SUCC_PENDING;
                                return NULL;
                            }
                        }
                    }

                    //smp4.2 or above, after exchange smp pairing DH key check, then transport specific keys distribution
                    blms_p_sts->smpDistributeKeyOrder = SMP_TRANSPORT_SPECIFIC_KEY_START;

                    return blt_smp_pushSmpCmdPkt(connHandle, SMP_OP_PAIRING_DHKEY);
                }
            }
        } break;

        case SMP_OP_KEYPRESS_NOTIFICATION:
        {
            if (smp_master_role) {
                if (gap_eventMask & GAP_EVT_MASK_SMP_KEYPRESS_NOTIFY) {
                    gap_smp_keypressNotifyEvt_t *pEvt = (gap_smp_keypressNotifyEvt_t *)param_evt;
                    pEvt->connHandle                  = connHandle;
                    pEvt->ntfType                     = req->data[0];
                    blc_gap_send_event(GAP_EVT_SMP_KEYPRESS_NOTIFY, param_evt, sizeof(gap_smp_keypressNotifyEvt_t));
                }
            }
        } break;

        default:
            goto smpSCcmdProcErr;
            break;
        }
        return NULL;
    }

smpSCcmdProcErr:
    blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_UNSPECIFIED_REASON); //pairing end with failure
    return blt_smp_pushPairingFailed(PAIRING_FAIL_REASON_UNSPECIFIED_REASON);
}
