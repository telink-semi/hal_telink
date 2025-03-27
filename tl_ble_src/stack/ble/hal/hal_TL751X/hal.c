/*
 * hal_dma.c
 *
 *  Created on: 2024
 *      Author: ADmin
 */

#include"stack/ble/hal/hal_internal.h"


_attribute_ram_code_
void blt_hal_reset_baseband(void)
{
    /*
     * At the end of each task, reset baseband is used to ensure that RF status is the default when the next task starts.
     * The main effect is to remove the previous redundant stop RF machine operation.
     */
    HAL_CSEM_IP_RESET_BASEBAND;
}
