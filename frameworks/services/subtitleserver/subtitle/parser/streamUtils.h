#ifndef __SUBTITLE_STREAM_UTILS_H__
#define __SUBTITLE_STREAM_UTILS_H__



//TODO: move to utils directory

/**
 *  return the literal value of ascii printed char
 */
static inline int subAscii2Value(char ascii) {
    return ascii - '0';
}

/**
 *  Peek the buffer data, consider it to int 32 value.
 *
 *  Peek, do not affect the buffer contents and pointer
 */
static inline int subPeekAsInt32(const char *buffer) {
    return (buffer[0] << 24) | (buffer[1] << 16)
          | (buffer[2] << 8) | buffer[3];
}

/**
 *  Peek the buffer data, consider it to int 64 value.
 *
 *  Peek, do not affect the buffer contents and pointer
 */

static inline uint64_t subPeekAsInt64(const char *buffer) {
    uint64_t value = 0;
    for (uint64_t i = 0; i < 8; i++) {
        value <<= 8;
        value |= buffer[i];
    }
    return value;
}

#endif
