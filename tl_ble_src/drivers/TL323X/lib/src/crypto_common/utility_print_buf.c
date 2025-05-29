/*! @file utility_print_buf.c */
#include "lib/include/crypto_common/utility.h"

#ifdef UTILITY_PRINT_BUF

/**
 * @brief           Prints a buffer of unsigned 8-bit integers.
 * @param[in]       buf                  - Pointer to the buffer of unsigned 8-bit integers.
 * @param[in]       byteLen              - Length of the buffer in bytes.
 * @param[in]       name                 - Name or description of the buffer for printing purposes.
 * @return          None
 */
void print_buf_u8(const unsigned char *buf, unsigned int byteLen, char *name)
{
    unsigned int i;

    if (NULL != buf) {
        (void)printf("\r\n %s: %08x\r\n  ", name,
                     (unsigned int)buf); // fflush(stdout);
        for (i = 0U; i < byteLen; i++) {
            (void)printf("%02x", buf[i]);
        }

        (void)printf("\r\n");
    }
}

/**
 * @brief           Prints a buffer of unsigned 32-bit integers.
 * @param[in]       buf                  - Pointer to the buffer of unsigned 32-bit integers.
 * @param[in]       wlen                 - Length of the buffer in terms of number of unsigned 32-bit integers.
 * @param[in]       name                 - Name or description of the buffer for printing purposes.
 * @return          None
  */
void print_buf_u32(const unsigned int *buf, unsigned int wlen, char *name)
{
    unsigned int i;

    if (NULL != buf) {
        (void)printf("\r\n %s: %08x\r\n", name,
                     (unsigned int)buf); // fflush(stdout);
        for (i = 0U; i < wlen; i++) {
            (void)printf("%08x", buf[i]); // fflush(stdout);
        }

        (void)printf("\r\n"); // fflush(stdout);
    }
}

/**
 * @brief           Prints a buffer of unsigned 32-bit integers representing a big number (BN).
 * @param[in]       buf                  - Pointer to the buffer of unsigned 32-bit integers.
 * @param[in]       wlen                 - Length of the buffer in terms of number of unsigned 32-bit integers.
 * @param[in]       name                 - Name or description of the buffer for printing purposes.
 * @return          None
  */
void print_bn_buf_u32(const unsigned int *buf, unsigned int wlen, char *name)
{
    unsigned int i;

    if (NULL != buf) {
        (void)printf("\r\n %08x %s: ", (unsigned int)buf, name); // fflush(stdout);
        for (i = 0U; i < wlen; i++) {
            (void)printf("%08x", buf[wlen - 1U - i]);
        }
        (void)printf("\r\n"); // fflush(stdout);
    }
}
#endif
