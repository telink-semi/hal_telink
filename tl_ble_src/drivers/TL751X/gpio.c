/********************************************************************************************************
 * @file    gpio.c
 *
 * @brief   This is the source file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "gpio.h"

/**********************************************************************************************************************
 *                                            local constants                                                       *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                              local macro                                                        *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                             local data type                                                     *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                              global variable                                                       *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                              local variable                                                     *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                          local function prototype                                               *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                         global function implementation                                             *
 *********************************************************************************************************************/

/**
 * @brief      This function serves to set the gpio-mux function.
 * @param[in]  pin      - the pin needs to set.
 * @param[in]  function - the function need to set.
 * @return     none.
 */
void gpio_set_mux_function(gpio_func_pin_e pin,gpio_func_e function)
{
    reg_gpio_func_mux(pin) = function;
}
/**
 * @brief      This function enable the input function of a pin.
 * @param[in]  pin - the pin needs to set the input function.
 * @return     none.
 */
void gpio_input_en(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        analog_write_reg8(0x13d,(analog_read_reg8(0x13d))|(bit));
    }
    else{
        BM_SET(reg_gpio_ie(pin), bit);
    }
}

/**
 * @brief      This function disable the input function of a pin.
 * @param[in]  pin - the pin needs to set the input function.
 * @return     none.
 */
void gpio_input_dis(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        analog_write_reg8(0x13d,(analog_read_reg8(0x13d))&(~bit));
    }
    else{
        BM_CLR(reg_gpio_ie(pin), bit);
    }
}

/**
 * @brief      This function set the input function of a pin.
 * @param[in]  pin - the pin needs to set the input function
 * @param[in]  value - enable or disable the pin's input function(1: enable,0: disable )
 * @return     none
 */
void gpio_set_input(gpio_pin_e pin, unsigned char value)
{
    if(value)
    {
        gpio_input_en(pin);
    }
    else
    {
        gpio_input_dis(pin);
    }
}

/**
 * @brief      This function set the pin's driving strength at strong.
 * @param[in]  pin - the pin needs to set the driving strength
 * @return     none
 */
 void gpio_ds_en(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        analog_write_reg8(0x13d,(analog_read_reg8(0x13d))|(bit<<4));
    }
    else{
        BM_SET(reg_gpio_ds(pin), bit);
    }
}

 /**
  * @brief      This function set the pin's driving strength.
  * @param[in]  pin - the pin needs to set the driving strength at poor.
  * @return     none
  */
void gpio_ds_dis(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        analog_write_reg8(0x13d,(analog_read_reg8(0x13d))&(~(bit<<4)));
    }
    else{
        BM_CLR(reg_gpio_ds(pin), bit);
    }
}

/**
 * @brief      This function servers to set the specified GPIO as high resistor.
 * @param[in]  pin  - select the specified GPIO, GPIOI GPIOJ group is not included in GPIO_ALL.
 * @return     none.
 * @note       -# gpio_shutdown(GPIO_ALL) is a debugging method only and is not recommended for use in applications.
 *             -# gpio_shutdown(GPIO_ALL) set all GPIOs to high impedance except SWS and MSPI.
 *             -# If you want to use JTAG/USB in active state, or wake up the MCU with a specific pin,
 *                you can enable the corresponding pin after calling gpio_shutdown(GPIO_ALL).
 */
/**
 * @brief      This function servers to set the specified GPIO as high resistor.
 * @param[in]  pin  - select the specified GPIO, GPIOI GPIOJ group is not included in GPIO_ALL.
 * @return     none.
 * @note       -# gpio_shutdown(GPIO_ALL) is a debugging method only and is not recommended for use in applications.
 *             -# gpio_shutdown(GPIO_ALL) set all GPIOs to high impedance except SWS and MSPI.
 *             -# If you want to use JTAG/USB in active state, or wake up the MCU with a specific pin,
 *                you can enable the corresponding pin after calling gpio_shutdown(GPIO_ALL).
 */
void gpio_shutdown(gpio_pin_e pin)
{
    unsigned short group = pin & 0xf00;
    unsigned char bit = pin & 0xff;
    if(0x00 == g_chip_version)
    {
        switch(group)
        {
            case GPIO_GROUPA:
                reg_gpio_pa_pd |= (0x7f & bit);//SWS
                reg_gpio_pa_oen |= bit;// disable output
                reg_gpio_pa_gpio |= (bit&0x7f);
                reg_gpio_pa_ie |= (bit|0x80);//disable input
                break;
            case GPIO_GROUPB:
                reg_gpio_pb_pd |= bit;
                reg_gpio_pb_oen |= bit;
                reg_gpio_pb_gpio |= bit;
                reg_gpio_pb_ie |= bit;
                break;
            case GPIO_GROUPC:
                reg_gpio_pc_pd |= bit;
                reg_gpio_pc_oen |= bit;
                reg_gpio_pc_gpio |= bit;
                reg_gpio_pc_ie |= bit;
                break;
            case GPIO_GROUPD:
                reg_gpio_pd_pd |= bit;
                reg_gpio_pd_oen |= bit;
                reg_gpio_pd_gpio |= bit;
                reg_gpio_pd_ie  |= bit;
                break;

            case GPIO_GROUPE:
                reg_gpio_pe_pd |= bit;
                reg_gpio_pe_oen |= bit;
                reg_gpio_pe_gpio |= bit;
                reg_gpio_pe_ie  |= bit;
                break;
            case GPIO_GROUPF:
                reg_gpio_pf_pd |= bit;
                reg_gpio_pf_oen |= bit;
                reg_gpio_pf_gpio |= bit;
                reg_gpio_pf_ie  |= bit;
                break;

            case GPIO_GROUPG:
                reg_gpio_pg_pd |= bit;
                reg_gpio_pg_oen |= bit;
                reg_gpio_pg_gpio |= bit;
                reg_gpio_pg_ie  |= bit;
                break;
            case GPIO_GROUPH:
                reg_gpio_ph_pd |= bit;
                reg_gpio_ph_oen |= bit;
                reg_gpio_ph_gpio |= bit;
                reg_gpio_ph_ie  |= bit;
                break;
            case GPIO_GROUPI:
                reg_gpio_pi_pd |= bit;
                reg_gpio_pi_oen |= bit;
                reg_gpio_pi_gpio |= bit;
                reg_gpio_pi_ie  |= bit;
                break;
            case GPIO_GROUPJ:
                reg_gpio_pj_pd |= bit;
                reg_gpio_pj_oen |= bit;
                reg_gpio_pj_gpio |= bit;
                reg_gpio_pj_ie  |= bit;
                break;
            case GPIO_GPOUPANA:
                analog_write_reg8(0x13d,(analog_read_reg8(0x13d)&0x30)|0xcf);
                break;
            case GPIO_ALL:
            {
                //as gpio
                reg_gpio_pa_gpio = 0x7f;//SWS
                reg_gpio_pb_gpio = 0xff;
                reg_gpio_pc_gpio = 0xff;
                reg_gpio_pd_gpio = 0xff;
                reg_gpio_pe_gpio = 0xff;
                reg_gpio_pf_gpio = 0xff;
                reg_gpio_pg_gpio = 0xff;
                reg_gpio_ph_gpio = 0xff;

                //output disable
                reg_gpio_pa_oen = 0xff;
                reg_gpio_pb_oen = 0xff;
                reg_gpio_pc_oen = 0xff;
                reg_gpio_pd_oen = 0xff;
                reg_gpio_pe_oen = 0xff;
                reg_gpio_pf_oen = 0xff;
                reg_gpio_pg_oen = 0xff;
                reg_gpio_ph_oen = 0xff;

                //enable input
                reg_gpio_pa_ie = 0xff;//SWS
                reg_gpio_pb_ie = 0xff;
                reg_gpio_pc_ie = 0xff;
                reg_gpio_pd_ie = 0xff;
                reg_gpio_pe_ie = 0xff;
                reg_gpio_pf_ie = 0xff;
                reg_gpio_pg_ie = 0xff;
                reg_gpio_ph_ie = 0xff;

                reg_gpio_pa_pd = 0x7f;//SWS
                reg_gpio_pb_pd = 0xff;
                reg_gpio_pc_pd = 0xff;
                reg_gpio_pd_pd = 0xff;
                reg_gpio_pe_pd = 0xff;
                reg_gpio_pf_pd = 0xff;
                reg_gpio_pg_pd = 0xff;
                reg_gpio_ph_pd = 0xff;
                analog_write_reg8(0x13d,(analog_read_reg8(0x13d)&0x30)|0xcf);
            }
       }
    }
    else
    {
        switch(group)
        {
            case GPIO_GROUPA:
                reg_gpio_pa_out_clear |= bit;
                reg_gpio_pa_oen |= bit;// disable output
                reg_gpio_pa_gpio |= (bit&0x7f);
                reg_gpio_pa_ie &= ((~bit)|0x80);//disable input
                break;
            case GPIO_GROUPB:
                reg_gpio_pb_out_clear |= bit;
                reg_gpio_pb_oen |= bit;
                reg_gpio_pb_gpio |= bit;
                reg_gpio_pb_ie &= (~bit);
                break;
            case GPIO_GROUPC:
                reg_gpio_pc_out_clear |= bit;
                reg_gpio_pc_oen |= bit;
                reg_gpio_pc_gpio |= bit;
                reg_gpio_pc_ie &= (~bit);
                break;
            case GPIO_GROUPD:
                reg_gpio_pd_out_clear |= bit;
                reg_gpio_pd_oen |= bit;
                reg_gpio_pd_gpio |= bit;
                reg_gpio_pd_ie &= (~bit);
                break;

            case GPIO_GROUPE:
                reg_gpio_pe_out_clear |= bit;
                reg_gpio_pe_oen |= bit;
                reg_gpio_pe_gpio |= bit;
                reg_gpio_pe_ie &= (~bit);
                break;
            case GPIO_GROUPF:
                reg_gpio_pf_out_clear |= bit;
                reg_gpio_pf_oen |= bit;
                reg_gpio_pf_gpio |= bit;
                reg_gpio_pf_ie &= (~bit);
                break;

            case GPIO_GROUPG:
                reg_gpio_pg_out_clear |= bit;
                reg_gpio_pg_oen |= bit;
                reg_gpio_pg_gpio |= bit;
                reg_gpio_pg_ie &= (~bit);
                break;
            case GPIO_GROUPH:
                reg_gpio_ph_out_clear |= bit;
                reg_gpio_ph_oen |= bit;
                reg_gpio_ph_gpio |= bit;
                reg_gpio_ph_ie &= (~bit);
                break;
            case GPIO_GROUPI:
                reg_gpio_pi_out_clear |= bit;
                reg_gpio_pi_oen |= bit;
                reg_gpio_pi_gpio |= bit;
                reg_gpio_pi_ie &= (~bit);
                break;
            case GPIO_GROUPJ:
                reg_gpio_pj_out_clear |= bit;
                reg_gpio_pj_oen |= bit;
                reg_gpio_pj_gpio |= bit;
                reg_gpio_pj_ie &= (~bit);
                break;
            case GPIO_GPOUPANA:
                analog_write_reg8(0x13d,(analog_read_reg8(0x13d))|(bit<<2));
                analog_write_reg8(0x13d,(analog_read_reg8(0x13d))&(~bit));
                break;
            case GPIO_ALL:
            {
                //as gpio
                reg_gpio_pa_gpio = 0x7f;//SWS
                reg_gpio_pb_gpio = 0xff;
                reg_gpio_pc_gpio = 0xff;
                reg_gpio_pd_gpio = 0xff;
                reg_gpio_pe_gpio = 0xff;
                reg_gpio_pf_gpio = 0xff;
                reg_gpio_pg_gpio = 0xff;
                reg_gpio_ph_gpio = 0xff;

                //set low level
                reg_gpio_pa_out_clear = 0xff;//SWS
                reg_gpio_pb_out_clear = 0xff;
                reg_gpio_pc_out_clear = 0xff;
                reg_gpio_pd_out_clear = 0xff;
                reg_gpio_pe_out_clear = 0xff;
                reg_gpio_pf_out_clear = 0xff;
                reg_gpio_pg_out_clear = 0xff;
                reg_gpio_ph_out_clear = 0xff;

                //output disable
                reg_gpio_pa_oen = 0xff;
                reg_gpio_pb_oen = 0xff;
                reg_gpio_pc_oen = 0xff;
                reg_gpio_pd_oen = 0xff;
                reg_gpio_pe_oen = 0xff;
                reg_gpio_pf_oen = 0xff;
                reg_gpio_pg_oen = 0xff;
                reg_gpio_ph_oen = 0xff;

                //disable input
                reg_gpio_pa_ie = 0x80;//SWS
                reg_gpio_pb_ie = 0x00;
                reg_gpio_pc_ie = 0x00;
                reg_gpio_pd_ie = 0x00;
                reg_gpio_pe_ie = 0x00;
                reg_gpio_pf_ie = 0x00;
                reg_gpio_pg_ie = 0x00;
                reg_gpio_ph_ie = 0x00;

                analog_write_reg8(0x13d,(analog_read_reg8(0x13d)&0xf0)|0x0c);
                analog_write_reg8(0x140,(analog_read_reg8(0x140)|0x03));
            }
        }
    }
}

/**
 * @brief     This function set a pin's IRQ.
 * @param[in] pin           - the pin needs to enable its IRQ.
 * @param[in] trigger_type  - gpio interrupt type.
 *                            0: rising edge.
 *                            1: falling edge.
 *                            2: high level.
 *                            3: low level.
 * @return    none.
 */
void gpio_set_irq(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type)
{
    unsigned char bit = pin & 0xff;
     /*
        When selecting pull-up resistance and rising edge to trigger gpio interrupt, gpio_irq_en should be placed before setting gpio_set_irq,
        otherwise an interrupt will be triggered by mistake.
     */
    gpio_irq_en(pin);
    switch(trigger_type)
    {
    case INTR_RISING_EDGE:
        BM_CLR(reg_gpio_pol(pin), bit);
        BM_CLR(reg_gpio_irq_ctrl, FLD_GPIO_IRQ_LVL_GPIO);
    break;
    case INTR_FALLING_EDGE:
        BM_SET(reg_gpio_pol(pin), bit);
        BM_CLR(reg_gpio_irq_ctrl, FLD_GPIO_IRQ_LVL_GPIO);
    break;
    case INTR_HIGH_LEVEL:
        BM_CLR(reg_gpio_pol(pin), bit);
        BM_SET(reg_gpio_irq_ctrl, FLD_GPIO_IRQ_LVL_GPIO);
    break;
    case INTR_LOW_LEVEL:
        BM_SET(reg_gpio_pol(pin), bit);
        BM_SET(reg_gpio_irq_ctrl, FLD_GPIO_IRQ_LVL_GPIO);
     break;
    }
    reg_gpio_irq_ctrl |= FLD_GPIO_CORE_INTERRUPT_EN;
    reg_gpio_irq_clr = FLD_GPIO_IRQ_CLR;//must clear cause to unexpected interrupt.
    gpio_set_irq_mask(GPIO_IRQ_MASK_GPIO);
}

/**
 * @brief     This function set a pin's IRQ_RISC0.
 * @param[in] pin           - the pin needs to enable its IRQ.
 * @param[in] trigger_type  - gpio interrupt type 0  rising edge 1 falling edge 2 high level 3 low level.
 * @return    none.
 */
void gpio_set_gpio2risc0_irq(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type)
{
    unsigned char bit = pin & 0xff;
    /*
       When selecting pull-up resistance and rising edge to trigger gpio interrupt, gpio_gpio2risc0_irq_en should be placed before setting gpio_set_gpio2risc0_irq,
       otherwise an interrupt will be triggered by mistake.
    */
    gpio_gpio2risc0_irq_en(pin);
    switch(trigger_type)
    {
    case INTR_RISING_EDGE:
        BM_CLR(reg_gpio_pol(pin), bit);
        BM_CLR(reg_gpio_irq_ctrl, FLD_GPIO_IRQ_LVL_GPIO2RISC0);
    break;
    case INTR_FALLING_EDGE:
        BM_SET(reg_gpio_pol(pin), bit);
        BM_CLR(reg_gpio_irq_ctrl, FLD_GPIO_IRQ_LVL_GPIO2RISC0);
    break;
    case INTR_HIGH_LEVEL:
        BM_CLR(reg_gpio_pol(pin), bit);
        BM_SET(reg_gpio_irq_ctrl, FLD_GPIO_IRQ_LVL_GPIO2RISC0);
        break;
    case INTR_LOW_LEVEL:
        BM_SET(reg_gpio_pol(pin), bit);
        BM_SET(reg_gpio_irq_ctrl, FLD_GPIO_IRQ_LVL_GPIO2RISC0);
       break;
    }
    reg_gpio_irq_clr = FLD_GPIO_IRQ_GPIO2RISC0_CLR;//must clear cause to unexpected interrupt.
    gpio_set_irq_mask(GPIO_IRQ_MASK_GPIO2RISC0);
}

/**
 * @brief     This function set a pin's IRQ_RISC1.
 * @param[in] pin           - the pin needs to enable its IRQ.
 * @param[in] trigger_type  - gpio interrupt type 0  rising edge 1 falling edge 2 high level 3 low level
 * @return    none.
 */
void gpio_set_gpio2risc1_irq(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type)
{
    unsigned char bit = pin & 0xff;
    /*
       When selecting pull-up resistance and rising edge to trigger gpio interrupt, gpio_gpio2risc1_irq_en should be placed before setting gpio_set_gpio2risc1_irq,
       otherwise an interrupt will be triggered by mistake.
    */
    gpio_gpio2risc1_irq_en(pin);
    switch(trigger_type)
    {
    case INTR_RISING_EDGE:
        BM_CLR(reg_gpio_pol(pin), bit);
        BM_CLR(reg_gpio_irq_ctrl, FLD_GPIO_IRQ_LVL_GPIO2RISC1);
    break;
    case INTR_FALLING_EDGE:
        BM_SET(reg_gpio_pol(pin), bit);
        BM_CLR(reg_gpio_irq_ctrl, FLD_GPIO_IRQ_LVL_GPIO2RISC1);
    break;
    case INTR_HIGH_LEVEL:
        BM_CLR(reg_gpio_pol(pin), bit);
        BM_SET(reg_gpio_irq_ctrl, FLD_GPIO_IRQ_LVL_GPIO2RISC1);
        break;
    case INTR_LOW_LEVEL:
        BM_SET(reg_gpio_pol(pin), bit);
        BM_SET(reg_gpio_irq_ctrl, FLD_GPIO_IRQ_LVL_GPIO2RISC1);
       break;
    }
    reg_gpio_irq_clr =FLD_GPIO_IRQ_GPIO2RISC1_CLR;//must clear cause to unexpected interrupt.
    gpio_set_irq_mask(GPIO_IRQ_MASK_GPIO2RISC1);
}

/**
 * @brief     This function set a pin's IRQ.
 * @param[in] pin           - the pin needs to enable its IRQ.
 * @param[in] trigger_type  - gpio interrupt type.
 *                            0: rising edge.
 *                            1: falling edge.
 *                            2: high level.
 *                            3: low level
 * @note      if you want to use this irq,you should select irq_group first,which correspond to the function "gpio_set_src_irq_group()".
 * @return    none.
 */
void gpio_set_src_irq(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type)
{
    unsigned char bit = pin & 0xff;
    switch(trigger_type)
    {
    case INTR_RISING_EDGE:
        BM_CLR(reg_gpio_pol(pin), bit);
        BM_CLR(reg_gpio_irq_level, FLD_GPIO_IRQ_LVL_GPIO);
    break;
    case INTR_FALLING_EDGE:
        BM_SET(reg_gpio_pol(pin), bit);
        BM_CLR(reg_gpio_irq_level, FLD_GPIO_IRQ_LVL_GPIO);
    break;
    case INTR_HIGH_LEVEL:
        BM_CLR(reg_gpio_pol(pin), bit);
        BM_SET(reg_gpio_irq_level, FLD_GPIO_IRQ_LVL_GPIO);
    break;
    case INTR_LOW_LEVEL:
        BM_SET(reg_gpio_pol(pin), bit);
        BM_SET(reg_gpio_irq_level, FLD_GPIO_IRQ_LVL_GPIO);
    break;
    }
    gpio_clr_group_irq_status(bit);//must clear, or it will cause to unexpected interrupt.
    gpio_set_group_irq_mask(bit);//set mask
}

/**
 * @brief     This function set a pin's pull-up/down resistor.
 * @param[in] pin - the pin needs to set its pull-up/down resistor.
 * @param[in] up_down_res - the type of the pull-up/down resistor.
 * @return    none.
 */
void gpio_set_up_down_res(gpio_pin_e pin, gpio_pull_type_e up_down_res)
{
    // PA[3:0]               PA[7:4]         PB[3:0]          PB[7:4]         PC[3:0]         PC[7:4]
    // sel: ana_0x80<7:0>    ana_0x81<7:0>   ana_0x82<7:0>    ana_0x83<7:0>   ana_0x84<7:0>   ana_0x85<7:0>
    // PD[3:0]               PD[7:4]         PE[3:0]          PE[7:4]         PF[3:0]         PF[7:4]
    // sel: ana_0x86<7:0>    ana_0x87<7:0>   ana_0x88<7:0>    ana_0x89<7:0>   ana_0x8a<7:0>   ana_0x8b<7:0>
    // PG[3:0]               PG[7:4]         PH[3:0]          PH[7:4]    
    // sel: ana_0x8c<7:0>    ana_0x8d<7:0>   ana_0x8e<7:0>    ana_0x8f<7:0>
    unsigned char r_val = up_down_res & 0x03;

    unsigned char base_ana_reg = 0;
    if((pin>>8)<8)//A-H
    {
         base_ana_reg = 0x80 + ((pin >> 8) << 1) + ((pin & 0xf0) ? 1 : 0 );
    }
    else{
        return;
    }
    unsigned char shift_num, mask_not;

    if(pin & 0x11){
        shift_num = 0;
        mask_not = 0xfc;
    }
    else if(pin & 0x22){
        shift_num = 2;
        mask_not = 0xf3;
    }
    else if(pin & 0x44){
        shift_num = 4;
        mask_not = 0xcf;
    }
    else if(pin & 0x88){
        shift_num = 6;
        mask_not = 0x3f;
    }
    else{
        return;
    }
    analog_write_reg8(base_ana_reg, (analog_read_reg8(base_ana_reg) & mask_not) | (r_val << shift_num));
}

/**
 * @brief     This function set pin's  pull-down register.
 * @param[in] pin - the pin needs to set its pull-down register.
 * @return    none.
 * @attention  This function sets the digital pull-down, it will not work after entering low power consumption.
 */
void gpio_set_digital_pulldown(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        analog_write_reg8(0x13d,(analog_read_reg8(0x13d))|(bit<<6));
    }
    else{
        BM_SET(reg_gpio_pd(pin), bit);
    }
}

/**
 * @brief     This function set pin's  pull-up register.
 * @param[in] pin - the pin needs to set its pull-up register.
 * @return    none.
 * @attention  This function sets the digital pull-up, it will not work after entering low power consumption.
 */
void gpio_set_digital_pullup(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        analog_write_reg8(0x13e,(analog_read_reg8(0x13e))| bit);
    }
    else{
        BM_SET(reg_gpio_pu(pin), bit);
    }
}

/**
 * @brief     This function disable pin's  pull-down register.
 * @param[in] pin - the pin needs to disable its pull-down register.
 * @return    none.
 */
void gpio_digital_pulldown_dis(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        analog_write_reg8(0x13d,(analog_read_reg8(0x13d))&(~(bit<<6)));
    }
    else{
        BM_CLR(reg_gpio_pd(pin), bit);
    }
}

/**
 * @brief     This function disable pin's  pull-up register.
 * @param[in] pin - the pin needs to disable its pull-up register.
 * @return    none.
 */
void gpio_digital_pullup_dis(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        analog_write_reg8(0x13e,(analog_read_reg8(0x13e))&(~bit));
    }
    else{
        BM_CLR(reg_gpio_pu(pin), bit);
    }
}

/**
 * @brief     This function set the pin as JTAG function for DSP.
 * @param[in] pin - the pin needs to set to JTAG function.
 * @return    none.
 */
void dsp_jtag_set_pin(gpio_pin_e pin)
{
    gpio_input_en(pin);
    reg_gpio_func_mux(pin) = 15; // DSP_TDI_I DSP_TDO_IO DSP_TMS_I DSP_TCK_I
    gpio_function_dis(pin);
}

/**
 * @brief     This function is used to enable the JTAG function of the DSP.
 * @param[in] dsp_jtag_pin - DSP signal line structure.
 * @return    none.
 */
void dsp_jtag_set_pin_en(dsp_jtag_pin_st *dsp_jtag_pin)
{
    dsp_jtag_set_pin((gpio_pin_e)dsp_jtag_pin->tck);//TCK
    dsp_jtag_set_pin((gpio_pin_e)dsp_jtag_pin->tms);//TMS
    dsp_jtag_set_pin((gpio_pin_e)dsp_jtag_pin->tdo);//TDO
    dsp_jtag_set_pin((gpio_pin_e)dsp_jtag_pin->tdi);//TDI
}

/**
 * @brief     This function set JTAG or SDP function for d25f and n22 core.
 * @param[in] pin
 * @return    none.
 */
void jtag_sdp_set_pin(gpio_pin_e pin)
{
    gpio_input_en(pin);
    reg_gpio_func_mux(pin) = 0;
    gpio_function_dis(pin);
}

/**
 * @brief     This function serves to set JTAG(4 wires) pin for d25f and n22 core. Where, PB[4]; PB[3]; PB[2]; PB[1] correspond to TDI; TDO; TMS; TCK functions mux respectively.
 * @param[in] none
 * @return    none.
 * @note      Power-on or hardware reset will detect the level of PB6 (reboot will not detect it), detecting a low level is configured as JTAG,
               detecting a high level is configured as SDP.  the level of PB6 can not be configured internally by the software, and can only be input externally.
 */
void jtag_set_pin_en(void)
{
    jtag_sdp_set_pin(GPIO_PB4);//TDI
    jtag_sdp_set_pin(GPIO_PB3);//TDO
    jtag_sdp_set_pin(GPIO_PB2);//TMS
    jtag_sdp_set_pin(GPIO_PB1);//TCK
}

/**
 * @brief     This function serves to set SDP(2 wires) pin for d25f and n22 core. where, PB[2]; PB[1] correspond to TMS and TCK functions mux respectively.
 * @param[in] none
 * @return    none.
 * @note      Power-on or hardware reset will detect the level of PB6 (reboot will not detect it), detecting a low level is configured as JTAG,
               detecting a high level is configured as SDP.  the level of PB6 can not be configured internally by the software, and can only be input externally.
 */
void sdp_set_pin_en(void)
{
    jtag_sdp_set_pin(GPIO_PB2);//TMS
    jtag_sdp_set_pin(GPIO_PB1);//TCK
}

/**
 * @brief     This function set probe clk output.
 * @param[in] pin
 * @param[in] sel_clk
 * @return    none.
 */
void gpio_set_probe_clk_function(gpio_func_pin_e pin,probe_clk_sel_e sel_clk)
{
    reg_probe_clk_sel= (reg_probe_clk_sel&0xe0)|sel_clk;    //probe_clk_sel_e
    gpio_set_mux_function(pin,DBG_PROBE_CLK);               //sel probe_clk function
    gpio_function_dis((gpio_pin_e)pin);
}

/**********************************************************************************************************************
  *                                         local function implementation                                             *
  *********************************************************************************************************************/

