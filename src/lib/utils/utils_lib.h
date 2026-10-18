#ifndef _UTILS_LIB_H___
#define _UTILS_LIB_H___

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


#define UTIL_LITTLE_ENDIAN (0)
#define UTIL_BIG_ENDIAN    (1)

uint8_t check_sum_u8(uint8_t* data, uint32_t data_len);
bool    is_bcd(uint8_t target);

/* 0x10 -> 10(0x0a) */
uint8_t bcd2hex(uint8_t bcd);

/* 0x10(16) -> 0x16 */
uint8_t hex2bcd(uint8_t hex);

uint32_t hex2bcd32(uint32_t hex);
uint64_t bcd2int(uint8_t* bcd, uint8_t len);

/* memcpy in an opposite direction */
void memcpy_r(uint8_t* dst, uint8_t* src, uint32_t len);

/* memory data inversion of specified length */
int mem_rev(uint8_t* buf, int len);

void mem_swap(void* a, void* b, int size);

/**
 * @brief   Check system endianness
 * @retval  UTIL_LITTLE_ENDIAN: little endian system
 * @retval  UTIL_BIG_ENDIAN: big endian system
 */
int util_endian(void);

/**
 * @brief   Convert a series of bytes into an integer
 * @param   buf:    byte array
 * @param   offset: offset of the byte array, if offset is NULL, it will be ignored,
 *                  otherwise it will increase by len
 * @param   len:    length of the byte array, should <= sizeof(int64_t)
 * @param   mode:   0-little endian, 1-big endian
 */
uint64_t byte2int(uint8_t* buf, int* offset, int len, int mode);

double byte2fp(uint8_t* buf, int* offset, int len, int mode);

void short2byte(uint8_t* dst, short src, int endian);
void int2byte(uint8_t* dst, int src, int endian);

/* string to number, strlen should <= 8 */
uint64_t util_atox(const char* str, uint32_t len);

/**
 * @brief   string turn to uint8 array:
 *          "680100" -> [0x68, 0x01, 0x00]
 * @return  the length of the converted array
 */
uint32_t str2byte(uint8_t* dst, uint32_t dst_size, const char* str);

/**
 * @brief   uint8 array turn to string:
 *          [0x68, 0x01, 0x00] -> "680100"
 */
void byte2str(char* str, uint8_t* buf, int buf_len);

/**
 * @brief   Add a gap to each byte in the data array
 * @details This function adds a specified gap to each byte in the data array.
 *          If the addition results in a value greater than 0xFF, it wraps around.
 * @param   gap:    The gap to add to each byte, can be positive or negative.
 */
void add_gap(uint8_t* data, uint32_t data_len, int gap);

#ifdef __cplusplus
}
#endif


#endif /* _UTILS_LIB_H___ */
