/********************************************************************************************************
 * @file    app_usb_desc.c
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

#include "../bis_source_config.h"

#if (PRODUCT_BIS_SOURCE_SELECT == PRODUCT_SIG_AURACAST_TRANSMITTER)

    #include <application/usbstd/usbdesc.h>
    #include "tl_common.h"
    #include "drivers.h"

    #if (USB_SPEAKER_ENABLE || USB_MIC_ENABLE)
        #include <application/app/usbaud_i.h>
    #endif

    #include "app_usb_desc.h"

USB_Descriptor_Device_t vendor_device_desc = {
    {sizeof(USB_Descriptor_Device_t), DTYPE_Device}, // Header
    #if (MS_OS_DESCRIPTOR_ENABLE)
    0x0200, // USBSpecification, USB 2.0
    #else
    0x0110, // USBSpecification, USB 1.1
    #endif
    USB_CSCP_NoDeviceClass,
    USB_CSCP_NoDeviceSubclass, // SubClass
    USB_CSCP_NoDeviceProtocol, // Protocol
    8, // Endpoint0Size, Maximum Packet Size for Zero Endpoint. Valid Sizes are 8, 16, 32, 64
    ID_VENDOR, // VendorID
    ID_PRODUCT_BASE,
    0x0100, // .ReleaseNumber
    USB_STRING_VENDOR, // .ManufacturerStrIndex
    USB_STRING_PRODUCT, // .ProductStrIndex
    3, // .SerialNumStrIndex, iSerialNumber
    1
};


USB_Descriptor_Configuration_t
    vendor_configuration_desc = {
        {
         {sizeof(USB_Descriptor_Configuration_Hdr_t),
             DTYPE_Configuration},                                   // Length, type
            sizeof(USB_Descriptor_Configuration_t),                  // TotalLength: variable
            USB_INTF_MAX,                                            // NumInterfaces
            1,                                                       // Configuration index
            NO_DESCRIPTOR,                                           // Configuration String
    #if (USB_RESUME_HOST)
            USB_CONFIG_ATTR_RESERVED | USB_CONFIG_ATTR_REMOTEWAKEUP, // Attributes
    #else
            USB_CONFIG_ATTR_RESERVED, //don't support remote wakeup
    #endif

            USB_CONFIG_POWER_MA(250) // MaxPower = 2*250mA
        },

    #if (USB_MIC_ENABLE || USB_SPEAKER_ENABLE)
        // audio_control_interface
        {
         {sizeof(USB_Descriptor_Interface_t), DTYPE_Interface},
         USB_INTF_AUDIO_CONTROL,
         0,                          // AlternateSetting
            0,                          // bNumEndpoints
            AUDIO_CSCP_AudioClass,      // bInterfaceclass ->Printer
            AUDIO_CSCP_ControlSubclass, // bInterfaceSubClass -> Control
            AUDIO_CSCP_ControlProtocol, // bInterfaceProtocol
            NO_DESCRIPTOR               // iInterface
        },
        // audio_control_interface_ac;
        {
        #if (USB_MIC_ENABLE && USB_SPEAKER_ENABLE)
         {sizeof(USB_Audio_Descriptor_Interface_AC_TL_t), DTYPE_CSInterface},
         AUDIO_DSUBTYPE_CSInterface_Header,                 // Subtype
            {0x00, 0x01},                                      // ACSpecification, version == 1.0
                                                               // debug note: TotalLength must less than  256
            {(sizeof(USB_Audio_Descriptor_Interface_AC_TL_t) + /*10*/
              sizeof(USB_Audio_Descriptor_InputTerminal_t) +   /*12*/
              sizeof(USB_Audio_Descriptor_OutputTerminal_t) +  /*9*/
              sizeof(USB_Audio_Descriptor_FeatureUnit_Mic_t) + /*9*/
              sizeof(USB_Audio_Descriptor_InputTerminal_t) +   /*12*/
              sizeof(USB_Audio_Descriptor_OutputTerminal_t) +  /*9*/
              sizeof(USB_Audio_StdDescriptor_FeatureUnit_t) /*10*/),
             0},
         2,                                                 // InCollection
            USB_INTF_SPEAKER,
         USB_INTF_MIC
        #else
            {sizeof(USB_Audio_Descriptor_Interface_AC_t), DTYPE_CSInterface},
            AUDIO_DSUBTYPE_CSInterface_Header,                // Subtype
            {0x00, 0x01},                                     // ACSpecification, version == 1.0
            #if (USB_MIC_ENABLE)
            {(sizeof(USB_Audio_Descriptor_Interface_AC_t) +   /*9*/
              sizeof(USB_Audio_Descriptor_InputTerminal_t) +  /*12*/
              sizeof(USB_Audio_Descriptor_OutputTerminal_t) + /*9*/
              sizeof(USB_Audio_Descriptor_FeatureUnit_Mic_t) /*9*/),
             0},
            1,
            USB_INTF_MIC
            #else
            {(sizeof(USB_Audio_Descriptor_Interface_AC_t) +   /*9*/
              sizeof(USB_Audio_Descriptor_InputTerminal_t) +  /*12*/
              sizeof(USB_Audio_Descriptor_OutputTerminal_t) + /*9*/
              sizeof(USB_Audio_StdDescriptor_FeatureUnit_t) /*10*/),
             0},

            1,
            USB_INTF_SPEAKER
            #endif
        #endif
        },

    #endif
    #if (USB_SPEAKER_ENABLE)
        // speaker_input_terminal
        {{sizeof(USB_Audio_Descriptor_InputTerminal_t), DTYPE_CSInterface},
         AUDIO_DSUBTYPE_CSInterface_InputTerminal,
         USB_SPEAKER_INPUT_TERMINAL_ID,
         AUDIO_TERMINAL_STREAMING,
         0,      // AssociatedOutputTerminal
         2,      // TotalChannels
         0x0003, // ChannelConfig
         0,      // ChannelStrIndex
         NO_DESCRIPTOR},
        // speaker_feature_unit
        {sizeof(USB_Audio_StdDescriptor_FeatureUnit_t), DTYPE_CSInterface, AUDIO_DSUBTYPE_CSInterface_Feature, USB_SPEAKER_FEATURE_UNIT_ID, USB_SPEAKER_FEATURE_UNIT_SOURCE_ID, 1, // bControlSize
         {0x03, 0x00, 0x00},                                                                                                                                                       // bmaControls
         NO_DESCRIPTOR},
        // speaker_output_terminal
        {{sizeof(USB_Audio_Descriptor_OutputTerminal_t), DTYPE_CSInterface},
         AUDIO_DSUBTYPE_CSInterface_OutputTerminal,
         USB_SPEAKER_OUTPUT_TERMINAL_ID,
         AUDIO_TERMINAL_OUT_SPEAKER,
         0, // AssociatedOutputTerminal
         USB_SPEAKER_OUTPUT_TERMINAL_SOURCE_ID,
         NO_DESCRIPTOR},
    #endif
    #if (USB_MIC_ENABLE)
        // mic_input_terminal
        {{sizeof(USB_Audio_Descriptor_InputTerminal_t), DTYPE_CSInterface},
         AUDIO_DSUBTYPE_CSInterface_InputTerminal,
         USB_MIC_INPUT_TERMINAL_ID,
         AUDIO_TERMINAL_IN_MIC,
         0,      // AssociatedOutputTerminal
         2,      // TotalChannels
         0x0003, // ChannelConfig
         0,      // ChannelStrIndex
         NO_DESCRIPTOR},
        // mic_feature_unit
        {
         {sizeof(USB_Audio_Descriptor_FeatureUnit_Mic_t),
             DTYPE_CSInterface},
         AUDIO_DSUBTYPE_CSInterface_Feature,
         USB_MIC_FEATURE_UNIT_ID,
         USB_MIC_FEATURE_UNIT_SOURCE_ID,
         1,            // bControlSize
            {0x03, 0x00}, // bmaControls
            NO_DESCRIPTOR},
        // mic_output_terminal
        {{sizeof(USB_Audio_Descriptor_OutputTerminal_t), DTYPE_CSInterface},
         AUDIO_DSUBTYPE_CSInterface_OutputTerminal,
         USB_MIC_OUTPUT_TERMINAL_ID,
         AUDIO_TERMINAL_STREAMING,
         0, // AssociatedOutputTerminal
         USB_MIC_OUTPUT_TERMINAL_SOURCE_ID,
         NO_DESCRIPTOR},
    #endif
    #if (USB_SPEAKER_ENABLE)
        // speaker_setting0
        {{sizeof(USB_Descriptor_Interface_t), DTYPE_Interface},
         USB_INTF_SPEAKER,
         0, // AlternateSetting
         0, // bNumEndpoints
         AUDIO_CSCP_AudioClass,
         AUDIO_CSCP_AudioStreamingSubclass,
         AUDIO_CSCP_StreamingProtocol,
         NO_DESCRIPTOR},
        // speaker_setting1
        {{sizeof(USB_Descriptor_Interface_t), DTYPE_Interface},
         USB_INTF_SPEAKER,
         1, // AlternateSetting
         1, // bNumEndpoints
         AUDIO_CSCP_AudioClass,
         AUDIO_CSCP_AudioStreamingSubclass,
         AUDIO_CSCP_StreamingProtocol,
         NO_DESCRIPTOR},
        // speaker_audio_stream
        {{sizeof(USB_Audio_Descriptor_Interface_AS_t), DTYPE_CSInterface},
         AUDIO_DSUBTYPE_CSInterface_General,
         1, // TerminalLink #1 USB Streaming IT
         1, // FrameDelay
         {USB_AUDIO_FORMAT_PCM & 0xff, (USB_AUDIO_FORMAT_PCM >> 8) & 0xff}},
        // speaker_audio_format
        {
         {sizeof(USB_Audio_Descriptor_Format_t) + sizeof(USB_Audio_SampleFreq_t), DTYPE_CSInterface},
         AUDIO_DSUBTYPE_CSInterface_FormatType,
         USB_AUDIO_FORMAT_PCM,
         SPK_CHANNEL_COUNT,  // Channels
            2,                  // SubFrameSize
            SPK_RESOLUTION_BIT, // BitsResolution
            1                   // TotalDiscreteSampleRates
        },
        // speaker_sample_rate AUDIO_SAMPLE_FREQ
        {(SPK_SAMPLE_RATE & 0xff), (SPK_SAMPLE_RATE >> 8), 0x00},
        // speaker_stream_endpoint
        {
         {
                {sizeof(USB_Audio_Descriptor_StreamEndpoint_Std_t), DTYPE_Endpoint},
                USB_EDP_SPEAKER,
                EP_TYPE_ISOCHRONOUS | (EP_SYNC_TYPE_ADAPTIVE << 2) | (EP_USAGE_TYPE_DATA << 4), // Attributes ENDPOINT_ATTR_ASYNC
                USB_SPK_CHANNELS_LEN,                                                           // EndpointSize USB_MIC_CHANNELS_LEN
                1                                                                               // PollingIntervalMS
            },
         0,                                                                                  // Refresh
            0                                                                                   // SyncEndpointNumber
        },
        // speaker_stream_endpoint_spc
        {
         {sizeof(USB_Audio_Descriptor_StreamEndpoint_Spc_t),
             DTYPE_CSEndpoint},
         AUDIO_DSUBTYPE_CSInterface_General,
         AUDIO_EP_FULL_PACKETS_ONLY | AUDIO_EP_SAMPLE_FREQ_CONTROL,
         0,     // LockDelayUnits
            {0, 0} // LockDelay
        },
    #endif
};

void app_usb_changeDesc(vendor_usbDesc_t *newDesc)
{
    vendor_device_desc.VendorID                                             = newDesc->vendorId;
    vendor_device_desc.ProductID                                            = newDesc->productId;
    vendor_configuration_desc.speaker_sample_rate.Byte1                     = newDesc->speakSampleRate & 0xff;
    vendor_configuration_desc.speaker_sample_rate.Byte2                     = (newDesc->speakSampleRate >> 8) & 0xff;
    vendor_configuration_desc.speaker_sample_rate.Byte3                     = (newDesc->speakSampleRate >> 16) & 0xff;
    vendor_configuration_desc.speaker_audio_format.Channels                 = newDesc->speakNum;
    vendor_configuration_desc.speaker_stream_endpoint.Endpoint.EndpointSize = newDesc->speakNum * (newDesc->speakSampleRate * SPK_RESOLUTION_BIT / 1000 / 8);
}

#endif //PRODUCT_BIS_SOURCE_SELECT == PRODUCT_SIG_AURACAST_TRANSMITTER
