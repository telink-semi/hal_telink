/* ***************************************************************************
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of Samsung
 * Electronics Co., Ltd. ("Confidential Information").
 * You shall not disclose such Confidential Information and shall use it
 * only in accordance with the terms of the license agreement you entered
 * into with Samsung Electronics Co., Ltd. ("SAMSUNG")
 * SAMSUNG MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SAMSUNG SHALL NOT BE
 * LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 *
 ****************************************************************************/

#ifndef TAGSDK_TAGCONFIG_H_
#define TAGSDK_TAGCONFIG_H_

#include "TagAccessoryOption.h"
#include "ProjectConfig.h"

/* Checking human error in option */
#if defined(TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT) && (TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT == 1)
#if defined(PEER_MANAGER_ENABLED) && (PEER_MANAGER_ENABLED == 0)
#error "Please enable PEER_MANAGER_ENABLED option to use LBA feature in sdk_config.h file"
#endif
#else
#if defined(PEER_MANAGER_ENABLED) && (PEER_MANAGER_ENABLED == 1)
#error "Please disable PEER_MANAGER_ENABLED option in sdk_config.h file"
#endif
#endif

/* Checking human error in option */
#if defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 0)
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 0)
#error "Please enable either TAG_ACCESSORY_OPTION_RING_THE_TAG or TAG_ACCESSORY_OPTION_LED_BLINKING. The Tag must support one of two options."
#endif
#if defined(TAG_ACCESSORY_OPTION_UPDATE_RINGTONE) && (TAG_ACCESSORY_OPTION_UPDATE_RINGTONE == 1)
#error "Please enable TAG_ACCESSORY_OPTION_RING_THE_TAG option to use Ringtone feature in TagAccessoryOption.h file"
#endif
#elif defined(TAG_ACCESSORY_OPTION_RING_THE_TAG) && (TAG_ACCESSORY_OPTION_RING_THE_TAG == 1)
#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
#error "Please disable either TAG_ACCESSORY_OPTION_RING_THE_TAG or TAG_ACCESSORY_OPTION_LED_BLINKING. The Tag must support one of two options."
#endif
#endif


/* Checking human error in option */
#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
#if defined(NRFX_NFCT_ENABLED) && (NRFX_NFCT_ENABLED == 0)
#error "Please enable NRFX_NFCT_ENABLED option to use NFC feature in sdk_config.h file"
#endif /* NRFX_NFCT_ENABLED */
#if defined(TIMER4_ENABLED) && (TIMER4_ENABLED == 0)
#error "Please enable TIMER4_ENABLED option to use NFC feature in sdk_config.h file"
#endif /* TIMER4_ENABLED */
#if defined(NFC_NDEF_MSG_ENABLED) && (NFC_NDEF_MSG_ENABLED == 0)
#error "Please enable NFC_NDEF_MSG_ENABLED option to use NFC feature in sdk_config.h file"
#endif /* NFC_NDEF_MSG_ENABLED */
#if defined(NFC_NDEF_RECORD_ENABLED) && (NFC_NDEF_RECORD_ENABLED == 0)
#error "Please enable NFC_NDEF_RECORD_ENABLED option to use NFC feature in sdk_config.h file"
#endif /* NFC_NDEF_RECORD_ENABLED */
#if defined(NFC_NDEF_URI_MSG_ENABLED) && (NFC_NDEF_URI_MSG_ENABLED == 0)
#error "Please enable NFC_NDEF_URI_MSG_ENABLED option to use NFC feature in sdk_config.h file"
#endif /* NFC_NDEF_URI_MSG_ENABLED */
#if defined(NFC_NDEF_URI_REC_ENABLED) && (NFC_NDEF_URI_REC_ENABLED == 0)
#error "Please enable NFC_NDEF_URI_REC_ENABLED option to use NFC feature in sdk_config.h file"
#endif /* NFC_NDEF_URI_REC_ENABLED */
#if defined(NFC_PLATFORM_ENABLED) && (NFC_PLATFORM_ENABLED == 0)
#error "Please enable NFC_PLATFORM_ENABLED option to use NFC feature in sdk_config.h file"
#endif /* NFC_PLATFORM_ENABLED */
#if defined(NFC_T2T_PARSER_ENABLED) && (NFC_T2T_PARSER_ENABLED == 0)
#error "Please enable NFC_T2T_PARSER_ENABLED option to use NFC feature in sdk_config.h file"
#endif /* NFC_T2T_PARSER_ENABLED */
#else
#if defined(NRFX_NFCT_ENABLED) && (NRFX_NFCT_ENABLED == 1)
#error "Please disable NRFX_NFCT_ENABLED option in sdk_config.h file"
#endif /* NRFX_NFCT_ENABLED */
#if defined(TIMER4_ENABLED) && (TIMER4_ENABLED == 1)
#error "Please disable TIMER4_ENABLED option in sdk_config.h file"
#endif /* TIMER4_ENABLED */
#if defined(NFC_NDEF_MSG_ENABLED) && (NFC_NDEF_MSG_ENABLED == 1)
#error "Please disable NFC_NDEF_MSG_ENABLED option in sdk_config.h file"
#endif /* NFC_NDEF_MSG_ENABLED */
#if defined(NFC_NDEF_RECORD_ENABLED) && (NFC_NDEF_RECORD_ENABLED == 1)
#error "Please disable NFC_NDEF_RECORD_ENABLED option in sdk_config.h file"
#endif /* NFC_NDEF_RECORD_ENABLED */
#if defined(NFC_NDEF_URI_MSG_ENABLED) && (NFC_NDEF_URI_MSG_ENABLED == 1)
#error "Please disable NFC_NDEF_URI_MSG_ENABLED option in sdk_config.h file"
#endif /* NFC_NDEF_URI_MSG_ENABLED */
#if defined(NFC_NDEF_URI_REC_ENABLED) && (NFC_NDEF_URI_REC_ENABLED == 1)
#error "Please disable NFC_NDEF_URI_REC_ENABLED option in sdk_config.h file"
#endif /* NFC_NDEF_URI_REC_ENABLED */
#if defined(NFC_PLATFORM_ENABLED) && (NFC_PLATFORM_ENABLED == 1)
#error "Please disable NFC_PLATFORM_ENABLED option in sdk_config.h file"
#endif /* NFC_PLATFORM_ENABLED */
#if defined(NFC_T2T_PARSER_ENABLED) && (NFC_T2T_PARSER_ENABLED == 1)
#error "Please disable NFC_T2T_PARSER_ENABLED option in sdk_config.h file"
#endif /* NFC_T2T_PARSER_ENABLED */
#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */

#endif /* TAGSDK_TAGCONFIG_H_ */
