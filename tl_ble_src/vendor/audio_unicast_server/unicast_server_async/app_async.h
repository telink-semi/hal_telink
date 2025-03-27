/*
 * app_aysnc.h
 *
 *  Created on: 2023骞�9鏈�20鏃�
 *      Author: ADmin
 */

#ifndef VENDOR_AUDIO_UNICAST_SERVER_UNICAST_SERVER_ASYNC_APP_ASYNC_H_
#define VENDOR_AUDIO_UNICAST_SERVER_UNICAST_SERVER_ASYNC_APP_ASYNC_H_

#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_ASYNC)
#include"vendor/common/tlk_api/tlk_tone.h"
typedef enum{
    APP_ASYNC_STATE_IDLE,
    APP_ASYNC_STATE_CONNECT,
}app_async_state_e;

typedef struct{
    u32 syncTick;
    blc_async_message_t message;
}app_async_node_t;

/*
 * @brief   adv struture, length-type-value
 */
typedef struct __attribute__((packed)) {
    u8 length;
    u8 type;
    u8 data[0];
    u16 resved;
}app_advdata_LTV;

void app_async_init(void);


void app_async_task(void);

/**
 * @brief      Timer irq process,used to playback audio data at a specific tick.
 * @param[in]  none.
 * @return     none.
 */
void app_timer1_irq_proc(void);

/**
 * @brief      BLE Adv report event handler
 * @param[in]  p - Pointer point to event parameter buffer.
 * @return
 */
int app_le_ext_adv_report_event_handle(u8 *p);

void app_tone_play(tlk_tone_type_e type);
#endif

#endif /* VENDOR_AUDIO_UNICAST_SERVER_UNICAST_SERVER_ASYNC_APP_ASYNC_H_ */
