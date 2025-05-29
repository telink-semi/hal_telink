#ifndef TAGSDK_TAGACCESSORYOPTION_H_
#define TAGSDK_TAGACCESSORYOPTION_H_

/**
 * The SDK support to play Tag's ring
 *
 * \note You can enable or disable the feature by this config
 */
#define TAG_ACCESSORY_OPTION_RING_THE_TAG (1)

/**
 * The SDK support to update ringtone of tag
 *
 * \note You can enable or disable the feature by this config
 */
#define TAG_ACCESSORY_OPTION_UPDATE_RINGTONE (1)

/**
 * The SDK support to use button action (Ring my mobile, Automation) by pushing tag button.
 *
 * \note You can enable or disable the feature by this config
 *
 */
#define TAG_ACCESSORY_OPTION_BUTTON_ACTION (1)

/**
 * The SDK support to update and download firmware by using SmartThinng server.
 *
 * \note You can enable or disable the feature by this config
 *
 */
#define TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE (1)

/**
 * The SDK support to use LBA(Left Behind Alert) feature of tag
 *
 * \note If using nRF5 SDK, you can disable this option to save code space.
 *
 */
#define TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT (1)

/**
 * The SDK support to set battery type of device
 *
 * \note You can set the "replaceable" or "rechargeable" battery type by this config
 *
 */
#define TAG_ACCESSORY_OPTION_BATTERY_TYPE "replaceable"

/**
 * The SDK support to blink Tag's LED
 *
 * \note You can enable or disable the feature by this config
 */
#define TAG_ACCESSORY_OPTION_LED_BLINKING (0)

/**
 * The SDK support to set lost message URL to NFC
 *
 * \note You can enable or disable the feature by this config
 *
 */
#define TAG_ACCESSORY_OPTION_LOST_MESSAGE (0)

/**
 * The SDK support to enter Power Saving Mode
 *
 * \note You can enable or disable the feature by this config
 *
 */
#define TAG_ACCESSORY_OPTION_POWER_SAVING_MODE (1)

#endif /* TAGSDK_TAGACCESSORYOPTION_H_ */
