/********************************************************************************************************
 * @file    efuse.c
 *
 * @brief   This is the source file for TL321X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
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
//Due to the fact that efuse is not open to the public, the relevant header files and registers are defined in C file
#include "reg_include/soc.h"
#include "lib/include/stimer.h"
#include "lib/include/clock.h"
#include "adc.h"
#include "lib/include/efuse.h"
#include "lib/include/pm/pm_internal.h"

#define EFUSE_BASE_ADDR 0x140300
/*******************************       efuse registers: 0x80140300      ******************************/
#define reg_efuse_addr REG_ADDR8(EFUSE_BASE_ADDR)
#define reg_efuse_rdat REG_ADDR8(EFUSE_BASE_ADDR + 0x01)
#define reg_efuse_wdat REG_ADDR8(EFUSE_BASE_ADDR + 0x02)
#define reg_efuse_ctrl REG_ADDR8(EFUSE_BASE_ADDR + 0x03)

enum
{
    FLD_EFUSE_WREN = BIT(0),
    FLD_EFUSE_RDEN = BIT(1),
    //RSVD
    FLD_EFUSE_WR_TRIG = BIT(3),
    //RSVD
    //RSVD
    //RSVD
    FLD_EFUSE_BUSY = BIT(7),
};

#define reg_efuse_b0           REG_ADDR32(EFUSE_BASE_ADDR + 0x04)

#define reg_efuse_timimng_cfg  REG_ADDR16(EFUSE_BASE_ADDR + 0x08)
#define reg_efuse_timimng_cfg0 REG_ADDR8(EFUSE_BASE_ADDR + 0x08) //Adjust the T_PGM timing
#define reg_efuse_timimng_cfg1 REG_ADDR8(EFUSE_BASE_ADDR + 0x09) //Adjust except T_PGM timing

/**
 * @brief      This function serves to judge whether efuse is busy.
 * @return     0: not busy  1:busy
 */
_attribute_ram_code_sec_noinline_ bool efuse_busy(void)
{
    return FLD_EFUSE_BUSY == (reg_efuse_ctrl & FLD_EFUSE_BUSY);
}

/**
 * @brief      This function serves to judge whether efuse write/read is busy .
 * @return     none.
 */
#define EFUSE_WAIT() wait_condition_fails_or_timeout(efuse_busy, g_drv_api_error_timeout_us, drv_timeout_handler, (unsigned int)DRV_API_ERROR_TIMEOUT_EFUSE_WAIT)

/**
 * @brief   This function serve to  configure the efuse timing function.
 * @return  none.
 */
void efuse_timing_cfg(void)
{
    switch (sys_clk.pclk) {
    case 6:
        reg_efuse_timimng_cfg = 0x0f;
        break;
    case 12:
        reg_efuse_timimng_cfg = 0x1e;
        break;
    case 16:
        reg_efuse_timimng_cfg = 0x28;
        break;
    case 24:
        reg_efuse_timimng_cfg = 0x3c;
        break;
    case 32:
        reg_efuse_timimng_cfg = 0x150;
        break;
    case 48:
        reg_efuse_timimng_cfg = 0x178;
        break;
    default:
        break;
    }
}

/**
 * @brief   This function serve to get efuse_b0_b3 value.
 * @return  The value of efuse_b0_b3.
 */
unsigned long efuse_b0(void)
{
    return reg_efuse_b0;
}

/**
 * @brief       This function servers to get data from EFUSE. EFUSE default value is 0. The EFUSE address range is 0~255 bytes.
 * @param[in]   addr    - the start address of the EFUSE location.
 * @param[in]   buf     - the start address of the buffer.
 * @param[in]   len     - the length(in byte) of content needs to read out from EFUSE.The range is between 0 and 256.
 * @return      DRV_API_SUCCESS: operation successful.
 *              DRV_API_TIMEOUT: operation timeout.
 * @note        When EFUSE is read, the efuse power cannot be powered. Efuse cannot be powered at any time except when burning.
 */
drv_api_status_e efuse_read(unsigned char addr, unsigned char *buff, unsigned short len)
{
    reg_efuse_ctrl |= FLD_EFUSE_RDEN;
    reg_efuse_addr = addr;
    while (len--) {
        reg_efuse_ctrl |= FLD_EFUSE_WR_TRIG;
        if (EFUSE_WAIT()) {
            return DRV_API_TIMEOUT;
        }

        *buff++ = reg_efuse_rdat;
    }
    reg_efuse_ctrl &= ~(FLD_EFUSE_RDEN);

    return DRV_API_SUCCESS;
}

/**
 * @brief       this function servers to set data to EFUSE. The efuse address range is 0~255 bytes. VDD25EF is need to give 2.5V power when write.
 * @param[in]   addr    - the start address of the efuse location.
 * @param[in]   buf     - the start address of the buffer.
 * @param[in]   len     - the length(in byte) of content needs to be written to efuse.The range is between 0 and 256.
 * @return      DRV_API_SUCCESS: operation successful. It does not mean that the written data is correct and requires read back verification.
 *              DRV_API_TIMEOUT: operation timeout.
 */
drv_api_status_e efuse_write(unsigned char addr, unsigned char *buff, unsigned short len)
{
    //Turn on efuse power supply
    analog_write_reg8(0x10, analog_read_reg8(0x10) & (~BIT(3)));
    delay_us(20); //Waiting for ldo to stabilize/

    reg_efuse_ctrl |= (FLD_EFUSE_WREN);
    reg_efuse_addr = addr;

    while (len--) {
        reg_efuse_wdat = *buff++;
        reg_efuse_ctrl |= FLD_EFUSE_WR_TRIG;
        if (EFUSE_WAIT()) {
            return DRV_API_TIMEOUT;
        }
    }
    reg_efuse_ctrl &= ~(FLD_EFUSE_WREN);
    //Turn off efuse power supply
    analog_write_reg8(0x10, analog_read_reg8(0x10) | BIT(3));

    return DRV_API_SUCCESS;
}

/**
* @brief      This function servers to get chip id from EFUSE.
* @param[in]  chip_id_buff - store chip id. Chip ID is 16 bytes.
* @return     DRV_API_SUCCESS: operation successful.
*             DRV_API_TIMEOUT: operation timeout.
*/
drv_api_status_e efuse_get_chip_id(unsigned char *chip_id_buff)
{
    return efuse_read(57, chip_id_buff, 16);
}

/**
 * @brief      This function serves to read IEEE address from EFUSE.
 * @param[out] buf  - Pointer to IEEE address buffer(IEEE address is 8bytes)
 * @return     none
 */
void efuse_get_ieee_addr(unsigned char *buf)
{
    efuse_timing_cfg();
    efuse_read(113, buf, 8);
}

/**
 * @brief        This function serves to check protection code according SDK version.
 * @param[in]    sdk_version, 0:driver sdk  0xff:sdk_version_ignore
 * @return       none.
 */
void efuse_check_protection_code(unsigned char sdk_version)
{
    unsigned int pCode = 0;

    efuse_timing_cfg();
    efuse_read(41, (unsigned char *)&pCode, 4);
    pCode = pCode & 0x1f; //Bit0-4 is market protection code.

    switch (sdk_version) {
    case 0:
        //Different SDKs have different restrictions. Please modify the code according to your own situation.
        //The driver here is only for example reference.
        if (0xE0 > pCode) {
            sys_reset_all();
            while (1);
        }
        break;
    case 7: //BLE SDK used
        if (7 < pCode) {
            sys_reset_all();
            while (1);
        }
        break;
    case 0xff:
        break;
    default:
        if (1) // Prevent macro setting exceptions from invalidating the ProtectionCode function
        {
            sys_reset_all();
            while (1);
        }
        break;
    }
}


#if ADC_CALIB_EN
/**
 * @brief       This function is used to Tighten the judgment of illegal values for gpio calibration and vbat calibration in the efuse.
 *              The ADC vref gain calibtation should range from 1100mV to 1300mV, the ADC vref offset calibration should range from -100mV to 100mV.
 * @param[in]   gain - the value of gpio_calib_vref_gain or vbat_calib_vref_gain
 *              offset - the value of gpio_calib_vref_offset or vbat_calib_vref_offset
 *              calib_func - Function pointer to gpio_calibration or vbat_calibration.
 * @return      DRV_API_FAILURE:the calibration function is invalid; DRV_API_SUCCESS:the calibration function is valid.
 */
drv_api_status_e efuse_set_adc_calib_value(unsigned char gain, signed char offset, void (*calib_func)(unsigned short, signed char))
{
    if ((gain <= 200) && (offset >= -100) && (offset <= 100)) {
        (*calib_func)(gain + 1100, offset);
        return DRV_API_SUCCESS;
    } else {
        return DRV_API_FAILURE;
    }
}

/**
 * @brief      This function servers to get calibration value from EFUSE.
 * @param[in]  none
 * @return     DRV_API_SUCCESS - the calibration value update, DRV_API_FAILURE - the calibration value is not update.
 */
drv_api_status_e efuse_calib_adc_vref(void)
{
    adc_ft_cp_calib_t calib_value;
    /********************************************************************************************
        The ADC calibration value priority of TL321X is FT > CP.
        The GPIO calibration value and the VBAT calibration value do not necessarily exist at the same time.
    ********************************************************************************************/
    efuse_read(0x31, (unsigned char *)&calib_value, 8);
    if (efuse_set_adc_calib_value(calib_value.ft_vbat_gain, (signed char)calib_value.ft_vbat_offset, adc_set_vbat_calib_vref) || efuse_set_adc_calib_value(calib_value.ft_gpio_gain, (signed char)calib_value.ft_gpio_offset, adc_set_gpio_calib_vref))     //vbat_ft and gpio_ft
    {
        if (efuse_set_adc_calib_value(calib_value.cp_vbat_gain, (signed char)calib_value.cp_vbat_offset, adc_set_vbat_calib_vref) || efuse_set_adc_calib_value(calib_value.cp_gpio_gain, (signed char)calib_value.cp_gpio_offset, adc_set_gpio_calib_vref)) //vbat_cp and gpio_cp
        {
            return DRV_API_FAILURE;
        }
    }
    return DRV_API_SUCCESS;
}
#endif
