/********************************************************************************************************
 * @file    whitelist_stack.h
 *
 * @brief   This is the header file for BLE SDK
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
#ifndef STACK_BLE_WHITELIST_STACK_H_
#define STACK_BLE_WHITELIST_STACK_H_




/******************************* whitelist start ******************************************************************/

#define         MAX_WHITE_LIST_SIZE                 4


typedef u8 irk_key_t[16];

typedef struct {
    u8 type;
    u8 address[BLE_ADDR_LEN];
    u8 reserved;
} wl_addr_t;

typedef struct {
    wl_addr_t  wl_addr_tbl[MAX_WHITE_LIST_SIZE];
    u8  wl_addr_tbl_index;
    u8  wl_irk_tbl_index;
} ll_whiteListTbl_t;



extern  ll_whiteListTbl_t       ll_whiteList_tbl;


ble_sts_t   blc_hci_le_addDeviceToAcceptList(hci_le_addDeviceAcceptlist_cmdParam_t *pCmdParam);
ble_sts_t   blc_ll_readWhiteListSize(hci_le_readWhiteListSizeCmd_retParam_t *retPara);


wl_addr_t*  blt_ll_searchAddrInWhiteListTbl(u8 type, u8 *addr);




/******************************* whitelist end  ******************************************************************/






/******************************* resolvelist start ******************************************************************/

#define         MAX_RESOLVING_LIST_SIZE             4 //New RL NUM


typedef struct {
    u8 type;
    u8 address[BLE_ADDR_LEN];
    u8 reserved;
    u8 irk[16];
} rl_addr_t;



typedef struct {
    u8  rl_idx;
    u8  rpaUsed;     //at least one RPA(local RPA or peer RPA) used
    u8  rlPrivMode;             //Privacy network(0) or privacy device(1)
    u8  rpa_timeout_100S_cnt;


    u8  dev_iden_valid;         //device identity valid
    u8  rlIdAddrType;           //Public address (0) or random address (1)
    u8  peerIrk_valid;
    u8  peerRpa_save_flag;

    u8  rlIdAddr[BLE_ADDR_LEN];       //The Identity Address is stored in little endian format
    u8  genrt_peerRpa[BLE_ADDR_LEN];  //generate peer RPA, for targetA(initA) of ADV_DIRECT_IND & AUX_ADV_IND
    u8  store_peerRpa[BLE_ADDR_LEN];  //store Peer RPA in the RF packet
    u8  u8_rsvd1[2];

    u8  peerIRK[16];             //The IRKs are stored in little endian format


#if 1 //(LL_FEATURE_ENABLE_LOCAL_RPA)
    u32 rpa_start_tick;

    u8  rlLocalRpa[BLE_ADDR_LEN];   //The local rpa is stored in little endian format
    u8  localIrk_valid;
    u8  u8_rsvd2[1];

    u8  localIRK[16];               //The IRKs are stored in little endian format
#endif

}ll_resolv_list_t;


typedef struct {
    u8  rlSize;
    u8  rlCnt;
    u8  rpa_use_matrix;
    u8  addrRlEn;

    u8  u8_rsvd[3];
    u8  rl_timeout_100S_num;

    u32 rl_timeout_us;

    ll_resolv_list_t rlList[MAX_RESOLVING_LIST_SIZE];

}ll_ResolvingListTbl_t;


extern  ll_ResolvingListTbl_t   blRslvLst;


/**
 * @brief      This function is used to initialize resolving list && RPA concerned parameters
 * @param[in]  none
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
void        blt_ll_initResolvingList(void);
void        blt_ll_clearResolvingList(void);

ble_sts_t   blc_hci_le_addDeviceToResolvingList(hci_le_addDeviceResolvinglist_cmdParam_t *cmdParam);
ble_sts_t   blc_hci_le_removeDeviceFromResolvingList(le_identityAddress_t *cmdParam);
ble_sts_t   blc_hci_le_readResolvingListSize(hci_le_readResolvingListSizeCmd_retParam_t *retPara);

ble_sts_t   blc_hci_le_setPrivacyMode(hci_le_setPrivacyMode_cmdParam_t *cmdParam);


void        blt_ll_resolvRefreshRpa(ll_resolv_list_t *);
int         blt_ll_resolvRpaTimeoutLoop (void);
void        blt_ll_resolvGenerateRpa(u8 *irk, u8 *addr);

void        blt_ll_resolvSetRpaInUse(ll_resolv_list_t *pRL);
void        blt_ll_storePeerDeviceRpa(ll_resolv_list_t *pRL, u8 *peer_rpa);
bool        blt_ll_resolvIsLocalRpaUsed(void);

ll_resolv_list_t* blt_ll_searchResolvingListEntry(u8 addrType, u8* addr);
ll_resolv_list_t* blt_ll_resolve_rpa(int local, u8 *rpa, ll_resolv_list_t *pRL_in);



ble_sts_t blc_hci_le_readPeerResolvableAddress(le_identityAddress_t *, hci_le_readPeerResolvableAddress_retParam_t *);
ble_sts_t blc_hci_le_readLocalResolvableAddress(le_identityAddress_t *, hci_le_readLocalResolvableAddress_retParam_t *);
/******************************* resolvelist end  ******************************************************************/


#endif /* WHITELIST_STACK_H_ */
