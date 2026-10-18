#ifndef _UTILS_LOCK_H___
#define _UTILS_LOCK_H___

#ifdef __cplusplus
extern "C" {
#endif

typedef void* UtilSem;
typedef void* UtilMutex;

/* 信号量接口 */
/**
 * @brief   创建信号量
 * @param   init_count  初始计数值
 * @param   max_count   上限。**仅 Windows 生效**: POSIX 无名信号量只有计数值,
 *                      没有上限概念, 该参数在 Linux/MinGW 下被忽略
 * @return  句柄; 失败返回 NULL
 * @note    因此不要依赖计数被"卡在上限"这个行为, 两个平台上语义并不一致
 */
UtilSem util_sem_new(int init_count, int max_count);
void     util_sem_take(UtilSem sem);
void     util_sem_give(UtilSem sem);
void     util_sem_del(UtilSem sem);

/* 互斥量接口 */
UtilMutex util_mutex_new();
void       util_mutex_lock(UtilMutex mutex);
void       util_mutex_unlock(UtilMutex mutex);
void       util_mutex_del(UtilMutex mutex);

#ifdef __cplusplus
}
#endif

#endif /* _UTILS_LOCK_H___ */
