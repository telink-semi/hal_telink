/*! @file hash_basic.c */
#include "lib/include/hash/hash_basic.h"
#include "lib/include/crypto_common/utility.h"
#include "reg_include/hash_reg.h"
#include "driver.h"

/**
 * @brief           get HFE IP version
 * @return          HFE IP version
 */
unsigned int hash_get_version(void)
{
    return rHASH_VERSION;
}

/**
 * @brief           get hash driver version
 * @return          hash driver version(software version)
 */
unsigned int hash_get_driver_version(void)
{
    // the meaning of the version(for example, if the return value is 0x23080301)
    // the first 3 bytes:  23.08.03 ---- date
    // the last byte:      01       ---- first version on the day
    unsigned int year = 24U;
    unsigned int month = 3U;
    unsigned int day = 4U;
    unsigned int version = 1U;

    return (year << 24U) | (month << 16U) | (day << 8U) | version;
}

/**
 * @brief           set hash to be CPU mode
 * @return          none
 */
void hash_set_cpu_mode(void)
{
    MEM_VOLATILE unsigned int mask = ~(((unsigned int)1) << HASH_DMA_OFFSET);

    rHASH_CFG &= mask;
}

/**
 * @brief           set hash to be DMA mode
 * @return          none
 */
void hash_set_dma_mode(void)
{
    MEM_VOLATILE unsigned int flag = (((unsigned int)1) << HASH_DMA_OFFSET);

    rHASH_CFG |= flag;
}

/**
 * @brief           set the specific hash algorithm
 * @param[in]       alg                  - specific hash algorithm
 * @return          none
 * @note
 *        1. please make sure alg is valid
 */
void hash_set_alg(hash_alg_e alg)
{
    MEM_VOLATILE unsigned int mask = (~0x0000000FU);

    rHASH_CFG &= mask;
    rHASH_CFG |= (unsigned int)alg;
}

/**
 * @brief           enable hash interruption in CPU mode or DMA mode
 * @return          none
 * @note
 */
void hash_enable_interruption(void)
{
    MEM_VOLATILE unsigned int flag = (((unsigned int)1) << HASH_INTERRUPTION_OFFSET);

    rHASH_CFG |= flag;
}

/**
 * @brief           disable hash interruption in CPU mode or DMA mode
 * @return          none
 * @note
 */
void hash_disable_interruption(void)
{
    MEM_VOLATILE unsigned int mask = ~(((unsigned int)1) << HASH_INTERRUPTION_OFFSET);

    rHASH_CFG &= mask;
}

/**
 * @brief           set the tag whether current block is the last message block or not
 * @param[in]       tag                  - 0(no), other(yes)
 * @return          none
 * @note
 *        1. if it is the last block, please config rHASH_MSG_LEN,
 *           then the hardware will do the padding and post-processing
 */
void hash_set_last_block(unsigned int tag)
{
    MEM_VOLATILE unsigned int mask = (~(((unsigned int)1) << HASH_LAST_OFFSET));
    MEM_VOLATILE unsigned int flag = (((unsigned int)1) << HASH_LAST_OFFSET);

    if (0U != tag) // current block is the last one of the message
    {
        rHASH_CFG |= flag;
    }
    else // current block is not the last one of the message
    {
        rHASH_CFG &= mask;
    }
}

/**
 * @brief           get current HASH iterator value
 * @param[out]      iterator             - current hash iterator
 * @param[in]       hash_iterator_words  - iterator word length
 * @return          none
 */
void hash_get_iterator(unsigned char *iterator, unsigned int hash_iterator_words)
{
    unsigned int temp;
    unsigned int i;

    if (0U != (((unsigned int)iterator) & 3U)) // for the case that iterator is not aligned by word
    {
        for (i = 0U; i < hash_iterator_words; i++)
        {
            temp = rHASH_OUT(i);
            memcpy_((unsigned char *)(&(iterator[(i << 2)])), (unsigned char *)(&temp), 4);
        }
    }
    else
    {
        for (i = 0U; i < hash_iterator_words; i++)
        {
            ((unsigned int *)iterator)[i] = rHASH_OUT(i);
        }
    }
}

/**
 * @brief           input current iterator value
 * @param[in]       iterator             - hash iterator value
 * @param[in]       hash_iterator_words  - iterator word length
 * @return          none
 * @note
 *        1. iterator must be word aligned
 */
void hash_set_iterator(const unsigned int *iterator, unsigned int hash_iterator_words)
{
    unsigned int i;

    for (i = 0U; i < hash_iterator_words; i++)
    {
        rHASH_IN(i) = iterator[i];
    }
}

/**
 * @brief           clear rHASH_PCR_LEN
 * @return          none
 * @note
 */
void hash_clear_msg_len(void)
{
    MEM_VOLATILE unsigned int flag = 0U;

    rHASH_PCR_LEN(0U) = flag;
    rHASH_PCR_LEN(1U) = flag;
    rHASH_PCR_LEN(2U) = flag;
    rHASH_PCR_LEN(3U) = flag;
}

/**
 * @brief           set the total byte length of the whole message
 * @param[in]       msg_total_bytes      - total byte length of the whole message
 * @param[in]       words                - word length of array msg_total_bytes
 * @return          none
 */
void hash_set_msg_total_byte_len(unsigned int *msg_total_bytes, unsigned int words)
{
    while (0U != (words--))
    {
        rHASH_PCR_LEN(words) = msg_total_bytes[words];
    }
}

/**
 * @brief           set dma output bytes length
 * @param[in]       bytes                - byte length of the written data for hash hardware
 * @return          none
 */
void hash_set_dma_output_len(unsigned int bytes)
{
    rHASH_DMA_WLEN = bytes;
}

/**
 * @brief           start HASH iteration calc
 * @return          none
 */
void hash_start(void)
{
    MEM_VOLATILE unsigned int start_flag = 1U;
    MEM_VOLATILE unsigned int clean_flag = 0U;

    // while((rHASH_SR1 & flag) == 1)
    //{;}

    rHASH_SR2 |= clean_flag;
    rHASH_CTRL |= start_flag;
}

/**
 * @brief           wait till done
 * @return          none
 */
void hash_wait_till_done(void)
{
    MEM_VOLATILE unsigned int finish_flag = 1U;
    MEM_VOLATILE unsigned int clean_flag = 0U;

    while (0U == (rHASH_SR2 & finish_flag))
    {
    }

    rHASH_SR2 = clean_flag;
}

/**
 * @brief           DMA wait till done
 * @param[in]       callback             - callback function pointer
 * @return          none
 */
void hash_dma_wait_till_done(hash_callback callback)
{
    MEM_VOLATILE unsigned int finish_flag = 1U;
    MEM_VOLATILE unsigned int clean_flag = 0U;

    while (0U == (rHASH_SR2 & finish_flag))
    {
        if (NULL != callback)
        {
            callback();
        }
        else
        {
        }
    }

    rHASH_SR2 = clean_flag;
}

/**
 * @brief           input message(at most a block)
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of msg, can not be greater than block bytes
 * @return          none
 * @note
 *        1. msg_len can not be greater than block byte
 */
void hash_input_msg_u8(const unsigned char *msg, unsigned int msg_len)
{
    unsigned int msg_words = msg_len / 4U;
    unsigned int remainder_bytes = msg_len & (3U);
    unsigned int tmp = 0U;
    unsigned int i;

    if (0U != (((unsigned int)msg) & 3U))
    {
        for (i = 0U; i < msg_words; i++)
        {
            memcpy_((unsigned char *)&tmp, msg, 4);
            rHASH_M_DIN(i) = tmp;
            msg = &(msg[4]);
        }
    }
    else
    {
        for (i = 0U; i < msg_words; i++)
        {
            rHASH_M_DIN(i) = *((const unsigned int *)msg);
            msg = &(msg[4]);
        }
    }

    if (0 != remainder_bytes)
    {
        tmp = 0U;
        memcpy_((unsigned char *)&tmp, msg, remainder_bytes);
        rHASH_M_DIN(i) = tmp;
    }
    else
    {
    }
}

#ifdef HASH_DMA_FUNCTION
/**
 * @brief           basic HASH DMA operation
 * @param[in]       in                   - message of some blocks, or message including the last byte(last block)
 * @param[out]      out                  - hash digest or hmac.
 * @param[in]       inByteLen            - actual byte length of input msg
 * @param[in]       callback             - callback function pointer
 * @return          none
 * @note
 *        1. for DMA operation, the unit of input and output is 4 words, so, please make sure the buffer
 *           out is sufficient.
 *        2. if just to input message, not to get digest or hmac, please set para out to be NULL and WLEN to be 0.
 *           if to get the digest or hmac, para out can not be NULL, and please set WLEN to be digest length
 */
void hash_dma_operate(const unsigned int *in, const unsigned int *out, unsigned int inByteLen, hash_callback callback)
{
    // src addr
    rHASH_DMA_SA_L = ((unsigned int)in) & 0xFFFFFFFFU;
    hash_tx_dma(hash_get_tx_dma_channel(), (unsigned int)in, inByteLen);
    // dst addr
    if (NULL != out)
    {
        rHASH_DMA_DA_L = ((unsigned int)out) & 0xFFFFFFFFU;
        hash_rx_dma(hash_get_rx_dma_channel(), (unsigned int)out, rHASH_DMA_WLEN);
    }
    else
    {
        ;
    }

    // data byte length
    rHASH_DMA_RLEN = inByteLen;

    hash_start();

    hash_dma_wait_till_done(callback);
}

#endif
