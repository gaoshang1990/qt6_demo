#ifndef _UTILS_MATH_H___
#define _UTILS_MATH_H___

#include <stdint.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif


/* NOTE: 实参会被求值两次, 勿传带副作用的表达式(如 i++)。
   (x)-(y) 可能溢出: INT_MIN 与正数相减的结果无法用同类型表示,
   这种情况请改用 uint64_t 计算后再取差。 */
#define UTIL_ABS_DIFF(x, y) ((x) >= (y) ? ((x) - (y)) : ((y) - (x)))
#define UTIL_MAX(x, y)      ((x) >= (y) ? (x) : (y))
#define UTIL_MIN(x, y)      ((x) <= (y) ? (x) : (y))
#define UTIL_ARR_LEN(arr)   (sizeof(arr) / sizeof((arr)[0]))


/* bit array */
#define BIT_MASK(b)         (1 << ((b) % 8))
#define BIT_SET(a, b)       ((a)[(b) / 8] |= BIT_MASK(b))    /* Set bit b of bit array a to 1 */
#define BIT_CLR(a, b)       ((a)[(b) / 8] &= ~BIT_MASK(b))   /* Set bit b of bit array a to 0 */
#define BIT_GET(a, b)       (!!((a)[(b) / 8] & BIT_MASK(b))) /* Read the value of bit b in bit array a */
#define BYTE_NUM(nb)        (((nb) + 8 - 1) / 8)             /* to declare an array of nb bits */


/**
 * @brief   return a random nuber: [min, max)
 * @note    If min >= max, returns min directly to avoid crash
 */
int util_rand(int min, int max);

void util_rand_n(int* dst, int min, int max, int n);
void util_rand_split(int* dst, int range_min, int range_max, int n, int min_value);

int util_ceil(double num);
int util_floor(double num);
int util_round(double num);

int shift_decimal_point(char* szNum, int scaler);
int util_shell_sort(int arr[], int len);


/**
 * @brief   Wrap a value into the range [min, max)
 * @param   target Input value to be wrapped
 * @param   min Minimum allowed value (inclusive)
 * @param   max Maximum allowed value (exclusive)
 * @return  Wrapped value inside [min, max)
 */
int wrap_range(int target, int min, int max);

/**
 * @brief   Clamp a value into the range [min, max]
 * @param   target Input value to be clamped
 * @param   min Minimum allowed value
 * @param   max Maximum allowed value
 * @return  Clamped value inside [min, max]
 */
int clamp_range(int target, int min, int max);


/* ------------------------------------------------------------------ */
/* ------------------------- 以上为 工具函数 ------------------------- */
/* ------------------------------------------------------------------ */
/* ------------------------- 以下为 随机游走 ------------------------- */
/* ------------------------------------------------------------------ */


typedef struct UtilWalkerImpl* UtilWalker;


/**
 * @brief   创建随机游走
 * @param   start 初始值
 * @param   max_step 单次调用的增量上限(不含), <= 1 表示不再增长
 * @return  随机游走句柄, 分配失败返回 NULL
 * @note    max_step 上限被截断到 RAND_MAX
 */
UtilWalker walker_new(uint32_t start, uint32_t max_step);
void       walker_del(UtilWalker self);


/**
 * @brief   在当前值上叠加一个 [0, max_step) 的随机增量, 返回叠加后的值
 * @note    值单调不减; 命中 0 时本次不变
 * @note    max_step <= 1 时区间为空, 恒为 0, 即不再增长
 * @note    叠加到 UINT32_MAX 后饱和, 不再增长, 也不回绕,
 *          调用方因此可以安全地用 cur - prev 反推增量
 */
uint32_t walker_next(UtilWalker self);


/**
 * @brief   返回当前累计值, 不推进
 */
uint32_t walker_cur(UtilWalker self);


/**
 * @brief   重置累计值, 增量上限保持不变
 * @return  0 成功, -1 失败
 */
int walker_reset(UtilWalker self, uint32_t start);


/* ------------------------------------------------------------------ */
/* ------------------------- 以上为 随机游走 ------------------------- */
/* ------------------------------------------------------------------ */
/* ------------------------- 以下为 统计单元 ------------------------- */
/* ------------------------------------------------------------------ */


typedef struct UtilStatImpl* UtilStat;

/**
 * @brief   创建统计单元
 * @param   size 统计窗口大小, 0-一直统计, 直至上限(INT32_MAX); 负值按 0 处理
 * @return  统计单元句柄, 分配失败返回 NULL, 调用方需检查
 */
UtilStat stat_new(int size);


/**
 * @brief   销毁统计单元, 传入 NULL 不做处理
 */
void stat_del(UtilStat self);


/**
 * @brief   重置统计单元, 清空窗口内的全部数据
 * @note    会一并复位窗口状态并清零历史缓冲
 * @note    重置后 cnt 归 0, 下一个样本的 avg 即其自身, 不会被稀释
 * @note    不清 stat_cur(): 它是最后一个送入的样本, 不是统计量
 * @return  0 成功, -1 失败(NULL)
 */
int stat_reset(UtilStat self);


/**
 * @brief   送入一个样本
 * @return  0 成功, -1 失败(NULL)
 */
int stat_push(UtilStat self, double item);


/**
 * @brief   取统计量, 传入 NULL 一律返回 0
 */
double stat_min(UtilStat self);
double stat_max(UtilStat self);
double stat_avg(UtilStat self);
double stat_sum(UtilStat self);
double stat_cur(UtilStat self);


/**
 * @brief   计算窗口内方差
 * @note    非滑动窗口(size <= 0)不支持, 返回 0 并向 stdout 输出提示
 * @note    窗口为空(尚未 push 或刚 reset 过)返回 0, 不会返回 NaN
 */
double stat_variance(UtilStat self);


/* ------------------------------------------------------------------ */
/* ------------------------- 以上为 统计单元 ------------------------- */
/* ------------------------------------------------------------------ */
/* ------------------------- 以下为 三角函数 ------------------------- */
/* ------------------------------------------------------------------ */


double degree2radian(double degree);
double radian2degree(double radian);


/**
 * @brief   查表法快速计算正弦值
 * @param   angle 角度值
 */
float fast_sin(int angle);


/**
 * @brief   查表法快速计算余弦值
 * @param   angle 角度值
 */
float fast_cos(int angle);


/* ------------------------------------------------------------------ */
/* ------------------------- 以上为 三角函数 ------------------------- */
/* ------------------------------------------------------------------ */


#ifdef __cplusplus
}
#endif

#endif /* _UTILS_MATH_H___ */
