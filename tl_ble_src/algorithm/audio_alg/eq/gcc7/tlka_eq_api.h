
#ifndef EQ_LIB_H_
#define EQ_LIB_H_

#include <math.h>
#include <nds_filtering_math.h>

#define EQ_RAM_CODE         1

#if (EQ_RAM_CODE)
#define _EQ_RAM_CODE_       __attribute__((section(".ram_code"))) __attribute__((noinline))
#else
#define _EQ_RAM_CODE_
#endif

#define EQ_VERSION_INT(major, minor, micro) (((major) << 16) | ((minor) << 8) | (micro))
#define EQ_VERSION EQ_VERSION_INT(0, 1, 0)

typedef enum {
    EQ_FLT_PEAKING   = 0,
    EQ_FLT_LOWPASS   = 1,
    EQ_FLT_HIGHPASS  = 2,
    EQ_FLT_BANDPASS  = 3,
    EQ_FLT_NORTCH    = 4,
    EQ_FLT_LOWSHELF  = 5,
    EQ_FLT_HIGHSHELF = 6,
} e_eq_filter_type_e;


int eq_get_version(void);

/**
 * @brief  Calculate EQ coefficient according to EQ parameter in one stage filter.
 *
 * @param[in]  sample_rate  Sample rate.
 * @param[in]  type Type of filter.
 * @param[in]  freq Center frequence.
 * @param[in]  q    Q value.
 * @param[in]  db   Gain value. 
 * @param[out] coef_out Pointer to coefficient. 
 *
 * @returns 0-success   else-filter type error
 */
signed char eq_calculate_coefficient_per_stage (unsigned int sample_rate, e_eq_filter_type_e type, unsigned int freq, float q, float db, float *coef_out);

/**
 * @brief  Process data.
 *
 * @param[in]  para Pointer to filter parameter.
 * @param[in]  ps   Pointer to rawdata-int16.
 * @param[in]  pd   Pointer to data after processing. 
 * @param[in]  nsample  Number of data need to be processed(<512 sample). 
 *
 * @returns 0-success else-in parameter error
 */
_EQ_RAM_CODE_  signed char eq_process_int16(nds_bq_df1_f32_t *para, signed short *ps, signed short *pd, unsigned short nsample);

/**
 * @brief  Process data.
 *
 * @param[in]  para Pointer to filter parameter.
 * @param[in]  ps   Pointer to rawdata-int32.
 * @param[in]  pd   Pointer to data after processing. 
 * @param[in]  nsample  Number of data need to be processed(<512 sample). 
 *
 * @returns 0-success else-in parameter error
 */
_EQ_RAM_CODE_  signed char eq_process_int32(nds_bq_df1_f32_t *para, signed int *ps, signed int *pd, unsigned short nsample);


#endif

