/********************************************************************************************************
 * @file    ll_misc.c
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

/*
test sample:
write_reg8(0x40001, blt_calBit1Number(0x0) );           // 0
write_reg8(0x40002, blt_calBit1Number(0x1) );           // 1
write_reg8(0x40003, blt_calBit1Number(0x55555555) );    // 16
write_reg8(0x40004, blt_calBit1Number(0xfcdffcdf) );    // 26
write_reg8(0x40005, blt_calBit1Number(0x12345678) );    // 13
 */

int blt_calBit1Number(u32 dat)
{
    dat = (dat & 0x55555555) + ((dat >> 1) & 0x55555555);
    dat = (dat & 0x33333333) + ((dat >> 2) & 0x33333333);
    dat = (dat & 0x0f0f0f0f) + ((dat >> 4) & 0x0f0f0f0f);
    dat = dat + (dat >> 8);
    dat = dat + (dat >> 16);
    return (dat & 0x0000003f);
}

int blt_calBit1Number_16bit(u32 dat)
{
    dat = (dat & 0x5555) + ((dat >> 1) & 0x5555);
    dat = (dat & 0x3333) + ((dat >> 2) & 0x3333);
    dat = (dat & 0x0F0F) + ((dat >> 4) & 0x0F0F);
    dat = (dat & 0x00FF) + ((dat >> 8) & 0x00FF);
    return dat;
}

/* this access code algorithm v1 is now only used for BQB, SDK do not use it */
u32 blt_ll_connCalcAccessAddr_v1(void)
{
    u32 aa;
    u16 aa_low;
    u16 aa_high;
    u32 temp;
    u32 mask;
    u32 prev_bit;
    u8  bits_diff;
    u8  consecutive;
    u8  transitions;
    u8  ones;
    int tmp;

    /* Calculate a random access address */
    aa = 0;
    while (1) {
        /* Get two, 16-bit random numbers */
        aa_low  = trng_rand() & 0xFFFF;
        aa_high = trng_rand() & 0xFFFF;

        /* All four bytes cannot be equal */
        if (aa_low == aa_high) {
            continue;
        }

        /* Upper 6 bits must have 2 transitions */
        tmp = (s16)aa_high >> 10;

        if (__builtin_popcount(tmp ^ (tmp >> 1)) < 2) {
            continue;
        }

        /* Cannot be access address or be 1 bit different */
        aa        = aa_high;
        aa        = (aa << 16) | aa_low;
        bits_diff = 0;
        temp      = aa ^ 0x8E89BED6;
        for (mask = 0x00000001; mask != 0; mask <<= 1) {
            if (mask & temp) {
                ++bits_diff;
                if (bits_diff > 1) {
                    break;
                }
            }
        }
        if (bits_diff <= 1) {
            continue;
        }

        /* Cannot have more than 24 transitions */
        transitions = 0;
        consecutive = 1;
        ones        = 0;
        mask        = 0x00000001;
        while (mask < 0x80000000) {
            prev_bit = aa & mask;
            mask <<= 1;
            if (mask & aa) {
                if (prev_bit == 0) {
                    ++transitions;
                    consecutive = 1;
                } else {
                    ++consecutive;
                }
            } else {
                if (prev_bit == 0) {
                    ++consecutive;
                } else {
                    ++transitions;
                    consecutive = 1;
                }
            }

            if (prev_bit) {
                ones++;
            }

            /* 8 lsb should have at least three 1 */
            if (mask == 0x00000100 && ones < 3) {
                break;
            }

            /* 16 lsb should have no more than 11 transitions */
            if (mask == 0x00010000 && transitions > 11) {
                break;
            }

            /* This is invalid! */
            if (consecutive > 6) {
                /* Make sure we always detect invalid sequence below */
                mask = 0;
                break;
            }
        }

        /* Invalid sequence found */
        if (mask != 0x80000000) {
            continue;
        }

        /* Cannot be more than 24 transitions */
        if (transitions > 24) {
            continue;
        }

        /* We have a valid access address */
        break;
    }
    return aa;
}

u32 blt_ll_connCalcAccessAddr_v2(void)
{
    /**
     * Refer to BLE Core Specification: Vol 6, Part B, "2.1.2 Access Address" for more information.
     *
     * The Access Address for all other advertising physical channel packets shall be
     * 0b10001110_10001001_10111110_11010110 (0x8E89BED6).
     *
     * Each Access Address shall meet the following requirements:
     *
     * & It shall not be the Access Address for any existing ACL connection or CIS on this device.
     * & It shall not be the Access Address for any enabled periodic advertising train.
     * & It shall not be the Access Address for any existing BIS on this device.
     * & It shall not be the Access Address for any existing BIG Control logical link on this device.
     * & If it is the Access Address for a new CIS, it shall differ by more than one bit from any other Access Address being used on the same device.
     * & It shall not be the advertising physical channel packets' Access Address.
     * & It shall not be a sequence that differs from the advertising physical channel packets' Access Address by only one bit.
     *
     * !!! Important 4 rules below for LE 1M or 2M PHY !!!
     * @ It shall have no more than six consecutive zeros or ones.
     * @ It shall not have all four octets equal.
     * @ It shall have no more than 24 transitions.
     * @ It shall have a minimum of two transitions in the most significant six bits
     *
     * !!! Important 2 rules below for LE Coded PHY !!!
     * On an implementation that also supports the LE Coded PHY (see Section 2.2),
     * On the Access Address shall also meet the following requirements:
     * @ It shall have at least three ones in the least significant 8 bits.
     * @ It shall have no more than eleven transitions in the least significant 16 bits
     */

    u32 accessAddr = trng_rand();

    /*
   * The following code enforces a pattern to make sure the address meets all requirements
   * (including requirements for the LE coded PHY).  The pattern is
   *
   *  0byyyyyy1x 0xxxx1x0 xxxx1x0x xx11x0x1   //least bit is constant 1
   *
   * with 2^5 choices for the upper six bits.  This provides 2^5 * 2^16 = 2097152 variations.
   */

    /* Patterns for upper six bits.  The lower row contains complemented values of the upper row. */
    static const u8 upperSixBits[] =
        {
            /* 000010 000100 000101 000110 001000 001100 001101 001110 010000 010001 010011 010111 010110 011000 011100 011110 */
            0x08,
            0x10,
            0x14,
            0x18,
            0x20,
            0x30,
            0x34,
            0x38,
            0x40,
            0x44,
            0x4C,
            0x5C,
            0x58,
            0x60,
            0x70,
            0x78,
            /* 111101 111011 111010 111001 110111 110011 110010 110001 101111 101110 101100 101000 101001 100111 100011 100001 */
            0xF4,
            0xEC,
            0xE8,
            0xE4,
            0xDC,
            0xCC,
            0xC8,
            0xC4,
            0xBC,
            0xB8,
            0xB0,
            0xA0,
            0xA4,
            0x9C,
            0x8C,
            0x84};

    /* Set the upper six bits. */
    accessAddr = (accessAddr & ~0xFC000000) | (upperSixBits[accessAddr >> 27] << 24);

    /* Set   ones  with the mask 0b00000010 00000100 00001000 00110001 */
    accessAddr |= 0x02040831;

    /* Clear zeros with the mask 0b00000000 10000001 00000010 00000100 */
    accessAddr &= ~0x00810204;

    return accessAddr;
}


#if (!SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY)
u8 blt_ll_getOwnAddrType(u16 connHandle)
{
    #if (LL_ACL_CEN_EN)
    if (connHandle & BLM_CONN_HANDLE) { //Master
        return ((bltInit.own_addr_type & BIT(0)) ? OWN_ADDRESS_RANDOM : OWN_ADDRESS_PUBLIC);
    } else
    #endif
    { //Slave
    #if (MULTIPLE_LOCAL_DEVICE_ENABLE)
        if (mlDevMng.mldev_en) {
            u8        conn_idx = connHandle & CONN_IDX_MASK;
            extern u8 local_dev_index[];
            u8        dev_idx = local_dev_index[conn_idx];
            return mlDevMng.dev_mac[dev_idx].type;
        } else
    #endif
        {
            return ((bltLegAdv.legadv_ownAddr_type & BIT(0)) ? OWN_ADDRESS_RANDOM : OWN_ADDRESS_PUBLIC);
        }
    }
}

u8 *blt_ll_getOwnMacAddr(u16 connHandle, u8 addr_type)
{
    (void)connHandle; //unused, remove warning

    u8 *pConnAddr = addr_type ? bltMac.macAddress_random : bltMac.macAddress_public;

    #if (MULTIPLE_LOCAL_DEVICE_ENABLE)
    u8 is_master = (connHandle & BLM_CONN_HANDLE);
    if (mlDevMng.mldev_en && !is_master) { //only slave support multi_device now
        u8        conn_idx = connHandle & CONN_IDX_MASK;
        extern u8 local_dev_index[];
        u8        dev_idx = local_dev_index[conn_idx];

        pConnAddr = mlDevMng.dev_mac[dev_idx].address;
    }
    #endif

    return pConnAddr;
}
#endif


int blt_debug_hex_2_dec_display(int src_data)
{
    int weight    = 1;
    int dest_data = 0;
    while (src_data) {
        dest_data += (src_data % 10) * weight;
        src_data /= 10;
        weight *= 16;
    }

    return dest_data;
}

_attribute_noinline_ unsigned int zuixiao_gongbeishu(unsigned int x, unsigned int y, int mode)
{
    unsigned int min, max;
    if (x > y) {
        max = x;
        min = y;
    } else {
        max = y;
        min = x;
    }

    unsigned int mul = 0;
    for (unsigned int i = 1; i <= min; i++) {
        mul = max * i;
        if ((mul % min) == 0) {
            break;
        }

        //special for CIS master
        if (mode && i > 100) {
            return 0;
        }
    }

    return mul;
}

ble_sts_t blc_hci_readLocalSupportedCommands(hci_readLocSupCmds_retParam_t *pRetPara)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_Local_Sup_Cmds", 0, 0);


    u8 cmd_tbl[64] = {
        /* 0
        0 HCI_Inquiry
        1 HCI_Inquiry_Cancel
        2 HCI_Periodic_Inquiry_Mode
        3 HCI_Exit_Periodic_Inquiry_Mode
        4 HCI_Create_Connection
        5 HCI_Disconnect
        6 HCI_Add_SCO_Connection (deprecated)
        7 HCI_Create_Connection_Cancel  */
        BIT(5),

        /* 1
        0 HCI_Accept_Connection_Request
        1 HCI_Reject_Connection_Request
        2 HCI_Link_Key_Request_Reply
        3 HCI_Link_Key_Request_Negative_Reply
        4 HCI_PIN_Code_Request_Reply
        5 HCI_PIN_Code_Request_Negative_Reply
        6 HCI_Change_Connection_Packet_Type
        7 HCI_Authentication_Requested  */
        0x00,

        /* 2
        0 HCI_Set_Connection_Encryption
        1 HCI_Change_Connection_Link_Key
        2 HCI_Master_Link_Key
        3 HCI_Remote_Name_Request
        4 HCI_Remote_Name_Request_Cancel
        5 HCI_Read_Remote_Supported_Features
        6 HCI_Read_Remote_Extended_Features
        7 HCI_Read_Remote_Version_Information   */
        BIT(7),

        /* 3
        0 HCI_Read_Clock_Offset
        1 HCI_Read_LMP_Handle
        2 Reserved for future use
        3 Reserved for future use
        4 Reserved for future use
        5 Reserved for future use
        6 Reserved for future use
        7 Reserved for future use */
        0x00,

        /* 4
        0 Reserved for future use
        1 HCI_Hold_Mode
        2 HCI_Sniff_Mode
        3 HCI_Exit_Sniff_Mode
        4 Previously used
        5 Previously used
        6 HCI_QoS_Setup
        7 HCI_Role_Discovery    */
        0x00,

        /* 5
        0 HCI_Switch_Role
        1 HCI_Read_Link_Policy_Settings
        2 HCI_Write_Link_Policy_Settings
        3 HCI_Read_Default_Link_Policy_Settings
        4 HCI_Write_Default_Link_Policy_Settings
        5 HCI_Flow_Specification
        6 HCI_Set_Event_Mask
        7 HCI_Reset */
        BIT(6) | BIT(7),

        /* 6
        0 HCI_Set_Event_Filter
        1 HCI_Flush
        2 HCI_Read_PIN_Type
        3 HCI_Write_PIN_Type
        4 Previously used
        5 HCI_Read_Stored_Link_Key
        6 HCI_Write_Stored_Link_Key
        7 HCI_Delete_Stored_Link_Key    */
        0x00,

        /* 7
        0 HCI_Write_Local_Name
        1 HCI_Read_Local_Name
        2 HCI_Read_Connection_Accept_Timeout
        3 HCI_Write_Connection_Accept_Timeout
        4 HCI_Read_Page_Timeout
        5 HCI_Write_Page_Timeout
        6 HCI_Read_Scan_Enable
        7 HCI_Write_Scan_Enable */
        BIT(0) | BIT(1),

        /* 8
        0 HCI_Read_Page_Scan_Activity
        1 HCI_Write_Page_Scan_Activity
        2 HCI_Read_Inquiry_Scan_Activity
        3 HCI_Write_Inquiry_Scan_Activity
        4 HCI_Read_Authentication_Enable
        5 HCI_Write_Authentication_Enable
        6 HCI_Read_Encryption_Mode (deprecated)
        7 HCI_Write_Encryption_Mode (deprecated)    */
        0x00,

        /* 9
        0 HCI_Read_Class_Of_Device
        1 HCI_Write_Class_Of_Device
        2 HCI_Read_Voice_Setting
        3 HCI_Write_Voice_Setting
        4 HCI_Read_Automatic_Flush_Timeout
        5 HCI_Write_Automatic_Flush_Timeout
        6 HCI_Read_Num_Broadcast_Retransmissions
        7 HCI_Write_Num_Broadcast_Retransmissions   */
        0x00,

        /* 10
        0 HCI_Read_Hold_Mode_Activity
        1 HCI_Write_Hold_Mode_Activity
        2 HCI_Read_Transmit_Power_Level
        3 HCI_Read_Synchronous_Flow_Control_Enable
        4 HCI_Write_Synchronous_Flow_Control_Enable
        5 HCI_Set_Controller_To_Host_Flow_Control
        6 HCI_Host_Buffer_Size
        7 HCI_Host_Number_Of_Completed_Packets  */
        HCI_CONTROLLER_TO_HOST_FLOW_CTRL_EN << 5 |
            HCI_CONTROLLER_TO_HOST_FLOW_CTRL_EN << 6 |
            HCI_CONTROLLER_TO_HOST_FLOW_CTRL_EN << 7,

        /* 11
        0 HCI_Read_Link_Supervision_Timeout
        1 HCI_Write_Link_Supervision_Timeout
        2 HCI_Read_Number_Of_Supported_IAC
        3 HCI_Read_Current_IAC_LAP
        4 HCI_Write_Current_IAC_LAP
        5 HCI_Read_Page_Scan_Mode_Period (deprecated)
        6 HCI_Write_Page_Scan_Mode_Period (deprecated)
        7 HCI_Read_Page_Scan_Mode (deprecated)  */
        0x00,

        /* 12
        0 HCI_Write_Page_Scan_Mode (deprecated)
        1 HCI_Set_AFH_Host_Channel_Classification
        2 Reserved for future use
        3 Reserved for future use
        4 HCI_Read_Inquiry_Scan_Type
        5 HCI_Write_Inquiry_Scan_Type
        6 HCI_Read_Inquiry_Mode
        7 HCI_Write_Inquiry_Mode    */
        0x00,

        /* 13
        0 HCI_Read_Page_Scan_Type
        1 HCI_Write_Page_Scan_Type
        2 HCI_Read_AFH_Channel_Assessment_Mode
        3 HCI_Write_AFH_Channel_Assessment_Mode
        4 Reserved for future use
        5 Reserved for future use
        6 Reserved for future use
        7 Reserved for future use   */
        blmsParam.chncSup_en << 2 |
            blmsParam.chncSup_en << 3,

        /* 14
        0 Reserved for future use
        1 Reserved for future use
        2 Reserved for future use
        3 HCI_Read_Local_Version_Information
        4 Reserved for future use
        5 HCI_Read_Local_Supported_Features
        6 HCI_Read_Local_Extended_Features
        7 HCI_Read_Buffer_Size  */
        0x28,

        /* 15 <1>: Read BD ADDR
        0 HCI_Read_Country_Code (deprecated)
        1 HCI_Read_BD_ADDR
        2 HCI_Read_Failed_Contact_Counter
        3 HCI_Reset_Failed_Contact_Counter
        4 HCI_Read_Link_Quality
        5 HCI_Read_RSSI
        6 HCI_Read_AFH_Channel_Map
        7 HCI_Read_Clock    */
        BIT(1) |
            blmsParam.pwr_ctrl_en << 5,

        /* 16
        0 HCI_Read_Loopback_Mode
        1 HCI_Write_Loopback_Mode
        2 HCI_Enable_Device_Under_Test_Mode
        3 HCI_Setup_Synchronous_Connection_Request
        4 HCI_Accept_Synchronous_Connection_Request
        5 HCI_Reject_Synchronous_Connection_Request
        6 Reserved for future use
        7 Reserved for future use   */
        0x00,

        /* 17
        0 HCI_Read_Extended_Inquiry_Response
        1 HCI_Write_Extended_Inquiry_Response
        2 HCI_Refresh_Encryption_Key
        3 Reserved for future use
        4 HCI_Sniff_Subrating
        5 HCI_Read_Simple_Pairing_Mode
        6 HCI_Write_Simple_Pairing_Mode
        7 HCI_Read_Local_OOB_Data   */
        0x00,

        /* 18
        0 HCI_Read_Inquiry_Response_Transmit_Power_Level
        1 HCI_Write_Inquiry_Transmit_Power_Level
        2 HCI_Read_Default_Erroneous_Data_Reporting
        3 HCI_Write_Default_Erroneous_Data_Reporting
        4 Reserved for future use
        5 Reserved for future use
        6 Reserved for future use
        7 HCI_IO_Capability_Request_Reply   */
        0x00,

        /* 19
        0 HCI_User_Confirmation_Request_Reply
        1 HCI_User_Confirmation_Request_Negative_Reply
        2 HCI_User_Passkey_Request_Reply
        3 HCI_User_Passkey_Request_Negative_Reply
        4 HCI_Remote_OOB_Data_Request_Reply
        5 HCI_Write_Simple_Pairing_Debug_Mode
        6 HCI_Enhanced_Flush
        7 HCI_Remote_OOB_Data_Request_Negative_Reply    */
        0x00,

        /* 20
        0 Reserved for future use
        1 Reserved for future use
        2 HCI_Send_Keypress_Notification
        3 HCI_IO_Capability_Request_Negative_Reply
        4 HCI_Read_Encryption_Key_Size
        5 Reserved for future use
        6 Reserved for future use
        7 Reserved for future use   */
        0x00,

        /* 21
        0 HCI_Create_Physical_Link
        1 HCI_Accept_Physical_Link
        2 HCI_Disconnect_Physical_Link
        3 HCI_Create_Logical_Link
        4 HCI_Accept_Logical_Link
        5 HCI_Disconnect_Logical_Link
        6 HCI_Logical_Link_Cancel
        7 HCI_Flow_Spec_Modify  */
        0x00,

        /* 22
        0 HCI_Read_Logical_Link_Accept_Timeout
        1 HCI_Write_Logical_Link_Accept_Timeout
        2 HCI_Set_Event_Mask_Page_2
        3 HCI_Read_Location_Data
        4 HCI_Write_Location_Data
        5 HCI_Read_Local_AMP_Info
        6 HCI_Read_Local_AMP_ASSOC
        7 HCI_Write_Remote_AMP_ASSOC    */
        BIT(2),

        /* 23
        0 HCI_Read_Flow_Control_Mode
        1 HCI_Write_Flow_Control_Mode
        2 HCI_Read_Data_Block_Size
        3 Reserved for future use
        4 Reserved for future use
        5 HCI_Enable_AMP_Receiver_Reports
        6 HCI_AMP_Test_End
        7 HCI_AMP_Test  */
        0x00,

        /* 24
        0 HCI_Read_Enhanced_Transmit_Power_Level
        1 Reserved for future use
        2 HCI_Read_Best_Effort_Flush_Timeout
        3 HCI_Write_Best_Effort_Flush_Timeout
        4 HCI_Short_Range_Mode
        5 HCI_Read_LE_Host_Support
        6 HCI_Write_LE_Host_Support
        7 Reserved for future use   */
        0x00,

        /* 25
        0 HCI_LE_Set_Event_Mask
        1 HCI_LE_Read_Buffer_Size [v1]
        2 HCI_LE_Read_Local_Supported_Features
        3 Reserved for future use
        4 HCI_LE_Set_Random_Address
        5 HCI_LE_Set_Advertising_Parameters
        6 HCI_LE_Read_Advertising_Physical_Channel_Tx_Power
        7 HCI_LE_Set_Advertising_Data   */
        0xf7,

        /* 26
        0 HCI_LE_Set_Scan_Response_Data
        1 HCI_LE_Set_Advertising_Enable
        2 HCI_LE_Set_Scan_Parameters
        3 HCI_LE_Set_Scan_Enable
        4 HCI_LE_Create_Connection
        5 HCI_LE_Create_Connection_Cancel
        6 HCI_LE_Read_White_List_Size
        7 HCI_LE_Clear_White_List   */
        0xc3 |
        blmsParam.acl_master_en <<2|
        blmsParam.acl_master_en <<3|
        blmsParam.acl_master_en <<4|
        blmsParam.acl_master_en <<5,


        /* 27
        0 HCI_LE_Add_Device_To_White_List
        1 HCI_LE_Remove_Device_From_White_List
        2 HCI_LE_Connection_Update
        3 HCI_LE_Set_Host_Channel_Classification
        4 HCI_LE_Read_Channel_Map
        5 HCI_LE_Read_Remote_Features
        6 HCI_LE_Encrypt
        7 HCI_LE_Rand   */
        0xff,

        /* 28
        0 HCI_LE_Enable_Encryption
        1 HCI_LE_Long_Term_Key_Request_Reply
        2 HCI_LE_Long_Term_Key_Request_Negative_Reply
        3 HCI_LE_Read_Supported_States
        4 HCI_LE_Receiver_Test [v1]
        5 HCI_LE_Transmitter_Test [v1]
        6 HCI_LE_Test_End
        7 Reserved for future use   */
        0x08 | 
        LL_FEATURE_ENABLE_LE_ENCRYPTION <<0|
        LL_FEATURE_ENABLE_LE_ENCRYPTION <<1|
        LL_FEATURE_ENABLE_LE_ENCRYPTION <<2|
        blmsParam.phy_test_en <<4|
        blmsParam.phy_test_en <<5|
        blmsParam.phy_test_en <<6,


        /* 29
        0 Reserved for future use
        1 Reserved for future use
        2 Reserved for future use
        3 HCI_Enhanced_Setup_Synchronous_Connection
        4 HCI_Enhanced_Accept_Synchronous_Connection
        5 HCI_Read_Local_Supported_Codecs
        6 HCI_Set_MWS_Channel_Parameters
        7 HCI_Set_External_Frame_Configuration  */
        0x00,

        /* 30
        0 HCI_Set_MWS_Signaling
        1 HCI_Set_MWS_Transport_Layer
        2 HCI_Set_MWS_Scan_Frequency_Table
        3 HCI_Get_MWS_Transport_Layer_Configuration
        4 HCI_Set_MWS_PATTERN_Configuration
        5 HCI_Set_Triggered_Clock_Capture
        6 HCI_Truncated_Page
        7 HCI_Truncated_Page_Cancel */
        0x00,

        /* 31
        0 HCI_Set_Connectionless_Slave_Broadcast
        1 HCI_Set_Connectionless_Slave_Broadcast_Receive
        2 HCI_Start_Synchronization_Train
        3 HCI_Receive_Synchronization_Train
        4 HCI_Set_Reserved_LT_ADDR
        5 HCI_Delete_Reserved_LT_ADDR
        6 HCI_Set_Connectionless_Slave_Broadcast_Data
        7 HCI_Read_Synchronization_Train_Parameters */
        0x00,

        /* 32
        0 HCI_Write_Synchronization_Train_Parameters
        1 HCI_Remote_OOB_Extended_Data_Request_Reply
        2 HCI_Read_Secure_Connections_Host_Support
        3 HCI_Write_Secure_Connections_Host_Support
        4 HCI_Read_Authenticated_Payload_Timeout
        5 HCI_Write_Authenticated_Payload_Timeout
        6 HCI_Read_Local_OOB_Extended_Data
        7 HCI_Write_Secure_Connections_Test_Mode    */
        (LL_FEATURE_ENABLE_LE_PING && LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN) << 4 |
            (LL_FEATURE_ENABLE_LE_PING && LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN) << 5,

        /* 33
        0 HCI_Read_Extended_Page_Timeout
        1 HCI_Write_Extended_Page_Timeout
        2 HCI_Read_Extended_Inquiry_Length
        3 HCI_Write_Extended_Inquiry_Length
        4 HCI_LE_Remote_Connection_Parameter_Request_Reply
        5 HCI_LE_Remote_Connection_Parameter_Request_Negative_Reply
        6 HCI_LE_Set_Data_Length
        7 HCI_LE_Read_Suggested_Default_Data_Length */
        LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION << 6 |
            LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION << 7,

        /* 34
        0 HCI_LE_Write_Suggested_Default_Data_Length
        1 HCI_LE_Read_Local_P-256_Public_Key
        2 HCI_LE_Generate_DHKey [v1]
        3 HCI_LE_Add_Device_To_Resolving_List
        4 HCI_LE_Remove_Device_From_Resolving_List
        5 HCI_LE_Clear_Resolving_List
        6 HCI_LE_Read_Resolving_List_Size
        7 HCI_LE_Read_Peer_Resolvable_Address   */
        LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION << 0 |
            CONTROLLER_GEN_P256KEY_ENABLE << 1 |
            CONTROLLER_GEN_P256KEY_ENABLE << 2 |
            LL_FEATURE_ENABLE_PRIVACY << 3 |
            LL_FEATURE_ENABLE_PRIVACY << 4 |
            LL_FEATURE_ENABLE_PRIVACY << 5 |
            LL_FEATURE_ENABLE_PRIVACY << 6 |
            LL_FEATURE_ENABLE_PRIVACY << 7,

        /* 35
        0 HCI_LE_Read_Local_Resolvable_Address
        1 HCI_LE_Set_Address_Resolution_Enable
        2 HCI_LE_Set_Resolvable_Private_Address_Timeout
        3 HCI_LE_Read_Maximum_Data_Length
        4 HCI_LE_Read_PHY
        5 HCI_LE_Set_Default_PHY
        6 HCI_LE_Set_PHY
        7 HCI_LE_Receiver_Test [v2] */
        LL_FEATURE_ENABLE_PRIVACY                   <<0 |
        LL_FEATURE_ENABLE_PRIVACY                   <<1 |
        LL_FEATURE_ENABLE_PRIVACY                   <<2 |
        LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION  <<3 |
        blmsParam.phy_2mCoded_en <<4 |
        blmsParam.phy_2mCoded_en <<5 |
        blmsParam.phy_2mCoded_en <<6 |
        blmsParam.phy_test_en    <<7,

        /* 36
        0 HCI_LE_Transmitter_Test [v2]
        1 HCI_LE_Set_Advertising_Set_Random_Address
        2 HCI_LE_Set_Extended_Advertising_Parameters
        3 HCI_LE_Set_Extended_Advertising_Data
        4 HCI_LE_Set_Extended_Scan_Response_Data
        5 HCI_LE_Set_Extended_Advertising_Enable
        6 HCI_LE_Read_Maximum_Advertising_Data_Length
        7 HCI_LE_Read_Number_of_Supported_Advertising_Sets  */
        blmsParam.phy_test_en     <<0|
        blmsParam.extAdvModule_en <<1|
        blmsParam.extAdvModule_en <<2|
        blmsParam.extAdvModule_en <<3|
        blmsParam.extAdvModule_en <<4|
        blmsParam.extAdvModule_en <<5|
        blmsParam.extAdvModule_en <<6|
        blmsParam.extAdvModule_en <<7,

        /* 37
        0 HCI_LE_Remove_Advertising_Set
        1 HCI_LE_Clear_Advertising_Sets
        2 HCI_LE_Set_Periodic_Advertising_Parameters
        3 HCI_LE_Set_Periodic_Advertising_Data
        4 HCI_LE_Set_Periodic_Advertising_Enable
        5 HCI_LE_Set_Extended_Scan_Parameters
        6 HCI_LE_Set_Extended_Scan_Enable
        7 HCI_LE_Extended_Create_Connection */
        blmsParam.extAdvModule_en << 0 |
            blmsParam.extAdvModule_en << 1 |
            blmsParam.prdAdvModule_en << 2 |
            blmsParam.prdAdvModule_en << 3 |
            blmsParam.prdAdvModule_en << 4 |
            blmsParam.extScanModule_en << 5 |
            blmsParam.extScanModule_en << 6 |
            blmsParam.extInitModule_en << 7,

        /* 38
        0 HCI_LE_Periodic_Advertising_Create_Sync
        1 HCI_LE_Periodic_Advertising_Create_Sync_Cancel
        2 HCI_LE_Periodic_Advertising_Terminate_Sync
        3 HCI_LE_Add_Device_To_Periodic_Advertiser_List
        4 HCI_LE_Remove_Device_From_Periodic_Advertiser_List
        5 HCI_LE_Clear_Periodic_Advertiser_List
        6 HCI_LE_Read_Periodic_Advertiser_List_Size
        7 HCI_LE_Read_Transmit_Power */
        blmsParam.pda_sync_en << 0 |
            blmsParam.pda_sync_en << 1 |
            blmsParam.pda_sync_en << 2 |
            blmsParam.pda_sync_en << 3 |
            blmsParam.pda_sync_en << 4 |
            blmsParam.pda_sync_en << 5 |
            blmsParam.pda_sync_en << 6 |
            BIT(7),

        /* 39
        0 HCI_LE_Read_RF_Path_Compensation
        1 HCI_LE_Write_RF_Path_Compensation
        2 HCI_LE_Set_Privacy_Mode
        3 HCI_LE_Receiver_Test [v3]
        4 HCI_LE_Transmitter_Test [v3]
        5 HCI_LE_Set_Connectionless_CTE_Transmit_Parameters
        6 HCI_LE_Set_Connectionless_CTE_Transmit_Enable
        7 HCI_LE_Set_Connectionless_IQ_Sampling_Enable  */
        BIT(0) |
        BIT(1) |
        LL_FEATURE_ENABLE_PRIVACY << 2 |
        blmsParam.phy_test_en     << 3 |
        blmsParam.phy_test_en     << 4 |
        blmsParam.cte_connLess_en << 5 |
        blmsParam.cte_connLess_en << 6 |
        blmsParam.cte_connLess_en << 7,

        /* 40
        0 HCI_LE_Set_Connection_CTE_Receive_Parameters
        1 HCI_LE_Set_Connection_CTE_Transmit_Parameters
        2 HCI_LE_Connection_CTE_Request_Enable
        3 HCI_LE_Connection_CTE_Response_Enable
        4 HCI_LE_Read_Antenna_Information
        5 HCI_LE_Set_Periodic_Advertising_Receive_Enable
        6 HCI_LE_Periodic_Advertising_Sync_Transfer
        7 HCI_LE_Periodic_Advertising_Set_Info_Transfer */
        blmsParam.cte_connLess_en << 4 |
            blmsParam.pda_sync_en << 5 |
            blmsParam.past_en << 6 |
            blmsParam.past_en << 7,

        /* 41
        0 HCI_LE_Set_Periodic_Advertising_Sync_Transfer_Parameters
        1 HCI_LE_Set_Default_Periodic_Advertising_Sync_Transfer_Parameters
        2 HCI_LE_Generate_DHKey [v2]
        3 HCI_Read_Local_Simple_Pairing_Options
        4 HCI_LE_Modify_Sleep_Clock_Accuracy
        5 HCI_LE_Read_Buffer_Size [v2]
        6 HCI_LE_Read_ISO_TX_Sync
        7 HCI_LE_Set_CIG_Parameters */
        blmsParam.past_en <<0 |
        blmsParam.past_en <<1 |
        CONTROLLER_GEN_P256KEY_ENABLE <<2 |
        blmsParam.iso_en <<5 |
        blmsParam.iso_tx_en <<6 |
        blmsParam.cis_cen_en <<7,

        /* 42
        0 HCI_LE_Set_CIG_Parameters_Test
        1 HCI_LE_Create_CIS
        2 HCI_LE_Remove_CIG
        3 HCI_LE_Accept_CIS_Request
        4 HCI_LE_Reject_CIS_Request
        5 HCI_LE_Create_BIG
        6 HCI_LE_Create_BIG_Test
        7 HCI_LE_Terminate_BIG */
        blmsParam.cis_cen_en << 0 |
            blmsParam.cis_cen_en << 1 |
            blmsParam.cis_cen_en << 2 |
            blmsParam.cis_per_en << 3 |
            blmsParam.cis_per_en << 4 |
            blmsParam.big_bcst_en << 5 |
            blmsParam.big_bcst_en << 6 |
            blmsParam.big_bcst_en << 7,


        /* 43
        0 HCI_LE_BIG_Create_Sync
        1 HCI_LE_BIG_Terminate_Sync
        2 HCI_LE_Request_Peer_SCA
        3 HCI_LE_Setup_ISO_Data_Path
        4 HCI_LE_Remove_ISO_Data_Path
        5 HCI_LE_ISO_Transmit_Test
        6 HCI_LE_ISO_Receive_Test
        7 HCI_LE_ISO_Read_Test_Counters */
        0x00 |
            blmsParam.big_sync_en << 0 |
            blmsParam.big_sync_en << 1 |
            blmsParam.iso_en << 3 |
            blmsParam.iso_en << 4 |
            blmsParam.iso_tx_en << 5 |
            blmsParam.iso_rx_en << 6 |
            blmsParam.iso_rx_en << 7,

        /* 44
        0 HCI_LE_ISO_Test_End
        1 HCI_LE_Set_Host_Feature
        2 HCI_LE_Read_ISO_Link_Quality
        3 HCI_LE_Enhanced_Read_Transmit_Power_Level
        4 HCI_LE_Read_Remote_Transmit_Power_Level
        5 HCI_LE_Set_Path_Loss_Reporting_Parameters
        6 HCI_LE_Set_Path_Loss_Reporting_Enable
        7 HCI_LE_Set_Transmit_Power_Reporting_Enable    */
        blmsParam.iso_en << 0 |
            BIT(1) |
            blmsParam.pwr_ctrl_en << 3 |
            blmsParam.pwr_ctrl_en << 4 |
            blmsParam.pwr_ctrl_en << 5 |
            blmsParam.pwr_ctrl_en << 6 |
            blmsParam.pwr_ctrl_en << 7,

        /* 45
        0 HCI_LE_Transmitter_Test [v4]
        1 HCI_Set_Ecosystem_Base_Interval
        2 HCI_Read_Local_Supported_Codecs [v2]
        3 HCI_Read_Local_Supported_Codec_Capabilities
        4 HCI_Read_Local_Supported_Controller_Delay
        5 HCI_Configure_Data_Path
        6 HCI_LE_Set_Data_Related_Address_Changes
        7 HCI_Set_Min_Encryption_Key_Size
        */
        (LL_FEATURE_ENABLE_PRIVACY && LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE) << 6,

        /* 46
         0 HCI_LE_Set_Default_Subrate command
         1 HCI_LE_Subrate_Request command
         2 HCI_LE_Set_Extended_Advertising_Parameters [v2]
         3 Reserved for future use
         4 Reserved for future use
         5 HCI_LE_Set_Periodic_Advertising_Subevent_Data
         6 HCI_LE_Set_Periodic_Advertising_Response_Data
         7 HCI_LE_Set_Periodic_Sync_Subevent
         */
        blmsParam.subrate_en << 0 |
            blmsParam.subrate_en << 1 |
            blmsParam.advCodeingSel_en << 2 |
            blmsParam.prdAdvWr_en << 5 |
            blmsParam.prdSyncWr_en << 6 |
            blmsParam.prdSyncWr_en << 7,

        /* 47
         0 HCI_LE_Extended_Create_Connection [v2]
         1 HCI_LE_Set_Periodic_Advertising_Parameters [v2]
         */
        blmsParam.prdAdvWr_en << 0 |
            blmsParam.prdAdvWr_en << 1,
        /* 48
        0 HCI_LE_Read_Monitored_Advertisers_List_Size
        1 HCI_LE_Frame_Space_Update
        */
        blmsParam.fsu_en << 0 |
            blmsParam.fsu_en << 1,

        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
#if LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT
        /*0x3D
         7 HCI_LE_Read_Maximum_Data_Length [v2]
         */
        0x80,
        /*0x3E
         0 HCI_LE_Set_HDT_Parameters_Test
         1 HCI_LE_Read_HDT_Local_Supported_Capabilities
         4 HCI_LE_Transmitter_Test [v5]
         5 HCI_HDT_Test_End [v2]
         6 HCI_LE_Create_BIG_Test [v2]
         7 HCI_LE_Set_HDT_Parameters
         */
        0xF3,
        /*0x3F
         0 HCI_LE_Set_CIG_Parameters [v3]
         1 HCI_LE_Set_CIG_Parameters_Test [v3]
         5 HCI_LE_Create_BIG [v2]
         */
        0x23,
#else
        0x00,
        0x00,
        0x00,
#endif
    };

#if BQB_HCI_LOCAL_SUP_CMD
    cmd_tbl[7] &= ~(BIT(0) | BIT(1)); //HCI_Write_Local_Name|HCI_Read_Local_Name
    cmd_tbl[7] |= BIT(2) | BIT(3);    //HCI_Read/Write_Connection_Accept_Timeout
    cmd_tbl[10] |= BIT(2);            //HCI_Read_Transmit_Power_Level
    cmd_tbl[39] |= BIT(3) | BIT(4);   //HCI_LE_Receiver/Transmitter_Test [V3]
    cmd_tbl[45] |= BIT(0);            //HCI_LE_Transmitter_Test [V4]
#endif


    pRetPara->status = BLE_SUCCESS;
    smemcpy(pRetPara->Supported_Commands, cmd_tbl, 64);

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_readLocalSupportedFeatures(hci_readLocSupFeatures_retParam_t *pRetPara)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_Local_Sup_Features", pRetPara->LMP_features, 8);

    u8 feat_tbl[8] = {0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00};

    pRetPara->status = BLE_SUCCESS;
    smemcpy(pRetPara->LMP_features, feat_tbl, 8);

    return BLE_SUCCESS;
}

/* for BQB test, manual add feature bit */
void blc_ll_addFeature_0(u32 feat_mask)
{
    LL_FEATURE_MASK_0 |= feat_mask;

    if (LL_FEATURE_MASK_0 & LL_FEATURE_MASK_CONNECTED_ISOCHRONOUS_STREAM_MASTER) {
        blmsParam.cis_cen_en = 1;
        blmsParam.cis_en     = 1;
    }

    if (LL_FEATURE_MASK_0 & LL_FEATURE_MASK_CONNECTED_ISOCHRONOUS_STREAM_SLAVE) {
        blmsParam.cis_per_en = 1;
        blmsParam.cis_en     = 1;
    }

    if (LL_FEATURE_MASK_0 & LL_FEATURE_MASK_ISOCHRONOUS_BROADCASTER) {
        blmsParam.big_bcst_en = 1;
        blmsParam.bis_en      = 1;
    }

    if (LL_FEATURE_MASK_0 & LL_FEATURE_MASK_SYNCHRONIZED_RECEIVER) {
        blmsParam.big_sync_en = 1;
        blmsParam.bis_en      = 1;
    }

    blmsParam.iso_en    = blmsParam.cis_en || blmsParam.bis_en;
    blmsParam.iso_tx_en = blmsParam.cis_en || blmsParam.big_bcst_en;
    blmsParam.iso_rx_en = blmsParam.cis_en || blmsParam.big_sync_en;

    if (LL_FEATURE_MASK_0 & (LL_FEATURE_MASK_PERIODIC_ADVERTISING_SYNC_TRANSFER_SENDER |
                             LL_FEATURE_MASK_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECIPIENT)) {
        blmsParam.past_en     = 1;
        blmsParam.pda_sync_en = 1;
    }

    if (LL_FEATURE_MASK_0 & LL_FEATURE_MASK_LE_EXTENDED_ADVERTISING) {
        blmsParam.extAdvModule_en = 1;
    }

    if (LL_FEATURE_MASK_0 & LL_FEATURE_MASK_LE_PERIODIC_ADVERTISING) {
        blmsParam.prdAdvModule_en = 1;
    }

    if (LL_FEATURE_MASK_0 &
        (LL_FEATURE_MASK_CONNECTIONLESS_CTE_TRANSMITTER |
         LL_FEATURE_MASK_CONNECTIONLESS_CTE_RECEIVER |
         LL_FEATURE_MASK_ANTENNA_SWITCHING_DURING_CTE_TRANSMISSION |
         LL_FEATURE_MASK_ANTENNA_SWITCHING_DURING_CTE_RECEPTION |
         LL_FEATURE_MASK_RECEIVING_CONSTANT_TONE_EXTENSIONS)) {
        blmsParam.cte_connLess_en = 1;
    }
}

void blc_ll_removeFeature_0(u32 feat_mask)
{
    LL_FEATURE_MASK_0 &= ~feat_mask;
}

void blc_ll_addFeature_1(u32 feat_mask)
{
    LL_FEATURE_MASK_1 |= feat_mask;

    if (LL_FEATURE_MASK_1 &
        (LL_FEATURE_MASK_LE_POWER_CTRL_REQUEST |
         LL_FEATURE_MASK_LE_POWER_CHANGE_INDICATION |
         LL_FEATURE_MASK_LE_PATH_LOSS_MONITORING)) {
        blmsParam.pwr_ctrl_en = 1;
    }

    if (LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_CLASSIFICATION) {
        blmsParam.chncSup_en = 1;
    }

    if (LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CONNECTION_SUBRATING) {
        blmsParam.subrate_en = 1;
    }

    if (LL_FEATURE_MASK_1 & LL_FEATURE_MASK_ADVERTISING_CODING_SELECTION) {
        blmsParam.advCodeingSel_en = 1;
    }

    if (LL_FEATURE_MASK_1 & LL_FEATURE_MASK_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER) {
        blmsParam.prdAdvWr_en = 1;
    }

    if (LL_FEATURE_MASK_1 & LL_FEATURE_MASK_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER) {
        blmsParam.prdSyncWr_en = 1;
    }
}

void blc_ll_removeFeature_1(u32 feat_mask)
{
    LL_FEATURE_MASK_1 &= ~feat_mask;
}
