#pragma once

typedef struct __attribute__((packed))
{
    u16 write_index;
    u16 read_index;
    u16 size;
    u8 *buffer;
} ring_buf_t;

/**
 * @brief        ring buffer initial function.
 * @param[in]    ring_buf: ring buffer structure pointer.
 * @param[in]    size: ring buffer size.
 * @param[in]    buffer: ring buffer store data pointer.
 * @return        none.
 */
void ring_buf_init(ring_buf_t *ring_buf, u16 size, u8 *buffer);

/**
 * @brief        free ring buffer space.
 * @param[in]    ring_buf: ring buffer structure pointer.
 * @return        none.
 */
u16 ring_buf_free_space(ring_buf_t *ring_buf);

/**
 * @brief        write data into ring buffer.
 * @param[in]    ring_buf: ring buffer structure pointer.
 * @param[in]    length: write buffer length.
 * @param[in]    buffer: write buffer pointer.
 * @return        number of bytes written.
 */
_attribute_ram_code_
    u16
    ring_buf_write(ring_buf_t *ring_buf, u16 length, u8 *buffer);

/**
 * @brief        read data into ring buffer.
 * @param[in]    ring_buf: ring buffer structure pointer.
 * @param[in]    length: want read buffer length.
 * @param[in]    buffer: read buffer pointer.
 * @return        number of bytes read.
 */
u16 ring_buf_read(ring_buf_t *ring_buf, u16 length, u8 *buffer);
