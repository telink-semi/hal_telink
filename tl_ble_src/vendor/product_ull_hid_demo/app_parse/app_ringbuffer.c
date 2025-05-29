
#include "tl_common.h"
#include "drivers.h"
#include "app_parse_cfg.h"
#include "app_ringbuffer.h"

/**
 * @brief        ring buffer initial function.
 * @param[in]    ring_buf: ring buffer structure pointer.
 * @param[in]    size: ring buffer size.
 * @param[in]    buffer: ring buffer store data pointer.
 * @return        none.
 */
void ring_buf_init(ring_buf_t *ring_buf, u16 size, u8 *buffer)
{
    ring_buf->size        = size;
    ring_buf->buffer      = buffer;
    ring_buf->write_index = 0;
    ring_buf->read_index  = 0;
}

/**
 * @brief        free ring buffer space.
 * @param[in]    ring_buf: ring buffer structure pointer.
 * @return        none.
 */
u16 ring_buf_free_space(ring_buf_t *ring_buf)
{
    if (ring_buf->read_index > ring_buf->write_index) {
        return ring_buf->read_index - ring_buf->write_index - 1;
    } else if (ring_buf->write_index > ring_buf->read_index) {
        return (ring_buf->read_index + ring_buf->size - ring_buf->write_index - 1);
    } else {
        // (ring_buf->write_index == ring_buf->read_index) means that buffer is empty
        return ring_buf->size - 1;
    }
}

/**
 * @brief        write data into ring buffer.
 * @param[in]    ring_buf: ring buffer structure pointer.
 * @param[in]    length: write buffer length.
 * @param[in]    buffer: write buffer pointer.
 * @return        number of bytes written.
 */
_attribute_ram_code_
    u16
    ring_buf_write(ring_buf_t *ring_buf, u16 length, u8 *buffer)
{
    u16 free_space = ring_buf_free_space(ring_buf);
    u16 remaining  = length;
    u16 len2;

    if (ring_buf->write_index >= ring_buf->read_index) {
        u16 len1 = free_space < (ring_buf->size - ring_buf->write_index) ? free_space : (ring_buf->size - ring_buf->write_index);
        if (len1 > remaining) {
            len1 = remaining;
        }

        //        memcpy(&ring_buf->buffer[ring_buf->write_index], buffer, len1);
        //Interrupt code, must be placed in ramcode
        for (int i = 0; i < len1; i++) {
            ring_buf->buffer[ring_buf->write_index + i] = buffer[i];
        }

        ring_buf->write_index = (ring_buf->write_index + len1) % ring_buf->size;
        remaining -= len1;
        buffer += len1;
        free_space -= len1;
    }

    len2 = remaining < free_space ? remaining : free_space;
    //    memcpy(&ring_buf->buffer[ring_buf->write_index], buffer, len2);
    //Interrupt code, must be placed in ramcode
    for (int i = 0; i < len2; i++) {
        ring_buf->buffer[ring_buf->write_index + i] = buffer[i];
    }
    remaining -= len2;
    ring_buf->write_index = (ring_buf->write_index + len2) % ring_buf->size;

    return length - remaining;
}

/**
 * @brief        read data into ring buffer.
 * @param[in]    ring_buf: ring buffer structure pointer.
 * @param[in]    length: want read buffer length.
 * @param[in]    buffer: read buffer pointer.
 * @return        number of bytes read.
 */
u16 ring_buf_read(ring_buf_t *ring_buf, u16 length, u8 *buffer)
{
    u16 remaining = length;
    u16 available;

    if (ring_buf->read_index > ring_buf->write_index) {
        available = ring_buf->size - ring_buf->read_index;
        if (available > remaining) {
            available = remaining;
        }

        memcpy(buffer, &ring_buf->buffer[ring_buf->read_index], available);
        ring_buf->read_index = (ring_buf->read_index + available) % ring_buf->size;
        remaining -= available;
        buffer += available;
    }

    if (ring_buf->read_index < ring_buf->write_index) {
        available = ring_buf->write_index - ring_buf->read_index;
        if (available > remaining) {
            available = remaining;
        }

        memcpy(buffer, &ring_buf->buffer[ring_buf->read_index], available);
        ring_buf->read_index = (ring_buf->read_index + available) % ring_buf->size;
        remaining -= available;
        buffer += available;
    }

    return length - remaining;
}
