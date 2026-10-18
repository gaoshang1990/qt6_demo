#ifndef _UTILS_THREAD_H___
#define _UTILS_THREAD_H___

#ifdef __cplusplus
extern "C" {
#endif


typedef struct UtilThreadImpl* UtilThread;
typedef void* (*UtilThreadFunc)(void* arg);


/**
 * @brief   创建线程对象, 线程函数 func 的参数为 arg, 成功返回线程对象指针, 失败返回 NULL
 *          注意: 创建线程对象后, 线程函数 func 会立即执行, 因此在 create 后不能马上销毁线程对象,
 *                因为此时在 thread_entry() 中还在执行 thread->func(thread->arg)
 */
UtilThread util_thread_create(UtilThreadFunc func, void* arg);


void util_thread_join(UtilThread self);
void util_thread_detach(UtilThread self);


/**
 * @brief   销毁线程对象, 销毁前应确认线程已运行, 即在 create 后不能马上销毁,
 *          因为此时在 thread_entry() 中还在执行 thread->func(thread->arg)
 */
void util_thread_destroy(UtilThread self);


void util_thread_exit(void);
void util_thread_sleep(int ms);
int  util_thread_tid(void);
int  util_thread_pid(void);


#ifdef __cplusplus
}
#endif

#endif /* _UTILS_THREAD_H___ */
