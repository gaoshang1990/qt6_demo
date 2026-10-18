#ifndef _UTILS_LIST_H___
#define _UTILS_LIST_H___

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UtilListImpl* UtilList;


UtilList list_new(bool thread_safe);
void      list_del(UtilList self);
void      list_push(UtilList self, void* data);
void*     list_pop(UtilList self);
void*     list_nth(UtilList self, int index);
size_t    list_len(UtilList self);
bool      list_contains(UtilList self, void* data);
void      list_remove(UtilList self, void* data);
void*     list_find_if(UtilList self, bool (*predicate)(void* data, void* arg), void* arg);

#ifdef __cplusplus
}
#endif

#endif /* _UTILS_LIST_H___ */
