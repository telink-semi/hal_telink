#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

#include <stdint.h> //including definitions of int32_t, unsigned int, etc.

#include <string.h> //including definition of NULL

/************************************************************************************
 ******************************    common config    *********************************
 ************************************************************************************/

// print buffer functions
#define UTILITY_PRINT_BUF

#ifdef UTILITY_PRINT_BUF
#include <stdio.h>
#endif

#define UTILITY_SEC

// only one of the following two macro could be enabled
// #define MEM_VOLATILE
#define MEM_VOLATILE volatile

// #define FLAG_STATIC
#define FLAG_STATIC static // default

#define SUPPORT_STATIC_ANALYSIS
#endif
