#ifndef _UTILS_STRING_H___
#define _UTILS_STRING_H___

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int str2lower(char* str, size_t len);
int str2upper(char* str, size_t len);
int util_str_split(void* dst, uint32_t dst_size, uint32_t item_size, const char* src, char delim);
int util_str_replace(char* dst, const char* before, const char* after);


#ifdef __cplusplus
}
#endif

#endif /* _UTILS_STRING_H___ */
