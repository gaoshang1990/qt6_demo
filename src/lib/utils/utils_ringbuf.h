#ifndef _UTILS_RING_BUFFER_H___
#define _UTILS_RING_BUFFER_H___

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @file    utils_ringbuf.h
 * @brief   定长槽位环形缓冲区 (byte-oriented ring buffer)
 *
 * 每个槽位固定存放 item_size 字节, push 时按值拷贝进内部缓冲, 因此本模块
 * 不做任何内存管理: 队列里的元素不会被 free, 也不会因为丢弃而泄漏。
 *
 * 满时行为: push 覆盖最旧的元素(丢队首), len 不再增长, 返回 RING_BUF_OK。
 * 即本模块不会返回 RING_BUF_FULL, 也不会阻塞或拒绝写入。
 *
 * 线程模型: 传入 thread_safe = true 时所有接口内部加锁; 否则必须由调用方保证串行。
 */

typedef struct UtilRingbufImpl* UtilRingbuf;


enum UtilRingErr {
    RING_BUF_OK,
    RING_BUF_NULL,
    RING_BUF_PARAM,
    RING_BUF_FULL, /* 保留: 当前实现不会返回该值 */
    RING_BUF_EMPTY,
};


/**
 * @brief   创建环形缓冲区, 内部自行分配 ring_size * item_size 字节
 * @param   ring_size    槽位数 (元素个数), 不是字节数
 * @param   item_size    单个槽位的字节数
 * @param   thread_safe  是否启用内部互斥锁
 * @return  新实例; 参数非法或分配失败返回 NULL
 * @note    ringbuf_del() 会连同内部缓冲一起释放
 */
UtilRingbuf ringbuf_new(size_t ring_size, size_t item_size, bool thread_safe);


/**
 * @brief   用调用者已有的缓冲创建环形缓冲区
 * @param   buf          外部缓冲, 至少容纳 ring_size * item_size 字节
 * @param   ring_size    槽位数 (元素个数)
 * @param   item_size    单个槽位的字节数
 * @param   thread_safe  是否启用内部互斥锁
 * @return  新实例; 参数非法返回 NULL
 * @warning buf 的生命周期必须长于本实例, 且不得由调用方释放
 * @note    ringbuf_del() 只释放实例本身, 不会碰 buf
 */
UtilRingbuf ringbuf_new_from_buf(void* buf, size_t ring_size, size_t item_size, bool thread_safe);


/**
 * @brief   当前元素个数
 * @return  元素个数; self 为 NULL 时返回 0
 */
size_t ringbuf_len(UtilRingbuf self);


/**
 * @brief   是否已满
 * @return  true 表示元素个数已达槽位数上限; self 为 NULL 时返回 false
 */
bool ringbuf_full(UtilRingbuf self);


/**
 * @brief   是否为空
 * @return  true 表示无元素; self 为 NULL 时返回 true
 */
bool ringbuf_empty(UtilRingbuf self);


/**
 * @brief   写入一个元素 (按值拷贝 item_size 字节)
 * @param   src  源数据指针, 需至少提供 item_size 字节
 * @return  RING_BUF_OK 表示写入成功
 *         -RING_BUF_NULL 表示 self 或 src 为 NULL
 * @note    缓冲区已满时覆盖最旧的元素 (丢队首), 返回值仍为 RING_BUF_OK
 */
int ringbuf_push(UtilRingbuf self, void* src);


/**
 * @brief   读取并移除队首元素
 * @return  指向内部存储中该元素的指针; 无元素或 self 为 NULL 时返回 NULL
 * @warning 返回值指向 ringbuf 内部缓冲, 下一次 push/pop 之后即失效。
 *          需要保留数据请立即 memcpy 出去。
 */
void* ringbuf_pop(UtilRingbuf self);


/**
 * @brief   读取元素但不移动 tail 指针
 * @param   index  相对队首的偏移, 0 即队首
 * @return  指向内部存储中该元素的指针; index 越界、index 为负或 self 为 NULL 时返回 NULL
 * @warning 同 ringbuf_pop(), 返回值在下一次 push/pop 之后即失效
 */
void* ringbuf_peek(UtilRingbuf self, int index);


/**
 * @brief   弹出队首元素并拷贝到 dst, 全程持锁
 * @param   dst       接收缓冲区
 * @param   dst_size  dst 的字节数, 必须 >= item_size
 * @return  RING_BUF_OK  表示已弹出并拷贝
 *         -RING_BUF_EMPTY 表示队列为空
 *         -RING_BUF_NULL  表示 self 或 dst 为 NULL
 *         -RING_BUF_PARAM 表示 dst_size 小于 item_size
 * @note    只有成功时才消费元素; 失败时 dst 不会被写入
 *
 * 多线程场景下请优先用本接口而不是 ringbuf_pop: ringbuf_pop 在放锁之后
 * 生产者随时可能写回同一槽位(缓冲区处于「满」状态时, 下一次 push 就会
 * 命中刚 pop 的那个槽位), 此时继续读返回的指针就是数据竞争。
 */
int ringbuf_pop_copy(UtilRingbuf self, void* dst, size_t dst_size);


/**
 * @brief   丢弃全部元素, 缓冲区回到可写入状态
 * @note    只移动索引, 不擦除数据
 */
void ringbuf_clear(UtilRingbuf self);


/**
 * @brief   销毁实例
 * @note    由 ringbuf_new() 创建的实例会连同内部缓冲一起释放;
 *          由 ringbuf_new_from_buf() 创建的实例只释放自身, 不碰调用者的 buf
 */
void ringbuf_del(UtilRingbuf self);


#ifdef __cplusplus
}
#endif

#endif /* _UTILS_RING_BUFFER_H___ */
