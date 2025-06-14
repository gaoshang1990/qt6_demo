#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils_lib.h"


/**
 * @brief 进行8位算数和运算
 */
uint8_t check_sum_u8(uint8_t* data, uint32_t data_len)
{
    uint8_t s = 0;
    for (uint32_t i = 0; i < data_len; i++)
        s += data[i];

    return s;
}

/**
 * @brief   判断是否为 BCD 码
 */
bool is_bcd(uint8_t target)
{
    uint8_t lo = target & 0x0f;
    uint8_t hi = (target >> 4) & 0x0f;

    if ((lo <= 9) && (hi <= 9))
        return true;
    else
        return false;
}

/* bcd to hex: 0x10 -> 10(0x0a) */
uint8_t bcd2hex(uint8_t bcd)
{
    uint8_t lo = bcd & 0x0f;
    uint8_t hi = (bcd >> 4) & 0x0f;

    if (lo > 9 || hi > 9)
        return 0;

    return hi * 10 + lo;
}

/* hex to bcd: 0x10(16) -> 0x16 */
uint8_t hex2bcd(uint8_t hex)
{
    if (hex >= 99)
        return 0x99;

    uint8_t lo = hex % 10;
    uint8_t hi = hex / 10;

    return (hi << 4) | lo;
}

uint32_t hex2bcd32(uint32_t hex)
{
    uint8_t  tmp[4];
    uint32_t divisor = 1;
    for (int i = 0; i < 4; i++) {
        tmp[i] = (hex / divisor) % 100;
        tmp[i] = hex2bcd(tmp[i]);
        divisor *= 100;
    }

    return (uint32_t)byte2int(tmp, NULL, 4, UTIL_LITTLE_ENDIAN);
}

uint64_t bcd2int(uint8_t* bcd, uint8_t len)
{
    uint64_t ret    = 0;
    uint64_t weight = 1;

    for (uint8_t i = 0; i < len; i++) {
        ret += bcd2hex(bcd[i]) * weight;
        weight *= 100;
    }

    return ret;
}

/* memcpy in an opposite direction */
void memcpy_r(uint8_t* dst, uint8_t* src, int len)
{
    for (int i = 0; i < len; i++)
        dst[i] = src[len - i - 1];
}


void mem_swap(void* a, void* b, int size)
{
    uint8_t tmp;

    for (int i = 0; i < size; i++) {
        tmp                = *((uint8_t*)a + i);
        *((uint8_t*)a + i) = *((uint8_t*)b + i);
        *((uint8_t*)b + i) = tmp;
    }
}


/* memory data inversion of specified length */
int mem_rev(uint8_t* buf, int len)
{
    for (int i = 0; i < len / 2; i++) {
        uint8_t tmp      = buf[i];
        buf[i]           = buf[len - i - 1];
        buf[len - i - 1] = tmp;
    }

    return 0;
}


/**
 * @brief   Convert a series of bytes into an integer
 * @param   buf:    byte array
 * @param   offset: offset of the byte array, if offset is NULL, it will be ignored,
 *                  otherwise it will increase by len
 * @param   len:    length of the byte array, should <= sizeof(int64_t)
 * @param   mode:   0-little endian, 1-big endian
 */
uint64_t byte2int(uint8_t* buf, int* offset, int len, int mode)
{
    if (offset)
        buf += *offset;

    if (len > sizeof(uint64_t))
        len = sizeof(uint64_t);

    uint64_t ret = 0;
    for (int i = 0; i < len; i++) {
        int pos = (mode == UTIL_BIG_ENDIAN) ? i : len - 1 - i;

        ret = ret << 8;
        ret += buf[pos];
    }

    if (offset)
        *offset += len;

    return ret;
}

/**
 * @brief   Convert a short into a series of bytes
 */
void short2byte(uint8_t* dst, short src, int mode)
{
    for (size_t i = 0; i < sizeof(short); i++) {
        int pos = (mode == UTIL_BIG_ENDIAN) ? i : sizeof(short) - 1 - i;

        dst[pos] = src & 0xff;
        src >>= 8;
    }
}

/**
 * @brief   Convert an int into a series of bytes
 */
void int2byte(uint8_t* dst, int src, int mode)
{
    short2byte(&dst[0], src, mode);
    short2byte(&dst[2], src >> 16, mode);
}

double byte2fp(uint8_t* buf, int* offset, int len, int mode)
{
    int64_t ret = byte2int(buf, offset, len, mode);
    double* p   = (double*)&ret;

    return *p;
}


/* string to number, strlen should <= 8 */
int atox_(const char* str, int len)
{
    if (len > 8)
        len = 8;

    int hex = -1;

    for (uint8_t i = 0; i < len; i++) {
        if (str[i] >= '0' && str[i] <= '9')
            hex = hex * 16 + str[i] - '0';
        else if (str[i] >= 'a' && str[i] <= 'f')
            hex = hex * 16 + str[i] - 'a' + 10;
        else if (str[i] >= 'A' && str[i] <= 'F')
            hex = hex * 16 + str[i] - 'A' + 10;
    }

    return hex;
}


/**
 * \brief   string turn to uint8 array:
 *          "680100" -> [0x68, 0x01, 0x00]
 */
int str2byte(uint8_t* dst, int bufSize, const char* str)
{
    if (dst == NULL || str == NULL) {
        printf("buf or str is NULL!\n");
        return -1;
    }

    uint16_t j      = 0;
    uint16_t bufLen = 0;
    char     tmp[2] = {0};

    for (uint16_t i = 0; i < strlen(str); i++) {
        if ((str[i] >= '0' && str[i] <= '9') || (str[i] >= 'a' && str[i] <= 'f') || (str[i] >= 'A' && str[i] <= 'F')) {
            tmp[j++] = str[i];
            if (j >= 2) {
                dst[bufLen++] = atox_(tmp, 2);
                if (bufLen >= bufSize) {
                    break;
                }
                memset(tmp, 0, 2);
                j = 0;
            }
        }
    }

    return bufLen;
}


/**
 * \brief   uint8 array turn to string:
 *          [0x68, 0x01, 0x00] -> "680100"
 */
int byte2str(char* str, uint8_t* buf, int bufLen)
{
    if (buf == NULL || str == NULL || bufLen <= 0) {
        printf("buf/str is NULL, or bufLen <= 0\n");
        return -1;
    }

    for (int i = 0; i < bufLen; i++)
        sprintf(str + i * 2, "%02x", buf[i]);

    return 0;
}

int add_gap(uint8_t* data, int len, int gap)
{
    if (len <= 0 || data == NULL) {
        printf("len[%d] <= 0 or data is null", len);
        return -1;
    }

    for (int i = 0; i < len; i++)
        data[i] += gap;

    return 0;
}

/**
 * @brief   Get the first number in a string
 * @retval  -1: no number found
 */
int num_in_str(const char* str)
{
    int num = 0;

    if (1 == sscanf(str, "%*[^-0-9]%d", &num))
        return num;

    return -1;
}
