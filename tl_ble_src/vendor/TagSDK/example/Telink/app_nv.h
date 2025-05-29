/*
 * app_nv.h
 *
 *  Created on: 2025.5
 *      Author: Admin
 */

#ifndef VENDOR_TAGSDK_EXAMPLE_TELINK_APP_NV_H_
#define VENDOR_TAGSDK_EXAMPLE_TELINK_APP_NV_H_

int app_tag_stoage_set_data(int idx, u8* data, size_t len);

int app_tag_stoage_get_data(int idx, u8* data, size_t len);

int app_tag_stoage_del_data(int idx);

void app_tag_stoage_init(void);

#endif /* VENDOR_TAGSDK_EXAMPLE_TELINK_APP_NV_H_ */
