#ifndef _UTILS_TIME_H___
#define _UTILS_TIME_H___

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIME_STR_LEN (19) /* "YYYY-MM-DD hh:mm:ss" 不含结束符 */

/**
 * @brief   struct tm.tm_wday 的"未指定"标记
 *
 * tm_wday 的有效范围是 0-6 (0 = 周日)。is_time_valid() 遇到超出该范围的
 * 值会跳过星期校验, 因此只解析出年月日时分秒的调用方应把 tm_wday 置为
 * 本宏, 否则栈上残留的垃圾值可能恰好落在 0-6 内而导致误判。
 *
 * @note    mktime() 会重算 tm_wday / tm_yday, 传 0xffff 进去不影响结果。
 */
#define UTIL_TM_WDAY_UNSET (0xffff)


#ifdef _WIN32
#  define LOCAL_TIME(pSec, pTm) localtime_s(pTm, pSec)
#else
#  define LOCAL_TIME(pSec, pTm) localtime_r(pSec, pTm)
#endif


bool is_leap_year(int year);


/**
 * @brief   获取某月的天数
 * @param   year    年份
 * @param   month   月份 (1-12)
 * @return  该月的天数，参数无效返回 0
 */
int days_of_month(int year, int month);


/**
 * @brief   基姆拉尔森计算星期公式
 * @return  0-6：星期日~六
 */
int weekday_by_kimlarsen(int year, int month, int day);


/**
 * @brief   获取指定日期是一年中的第几天
 * @param   year    年份
 * @param   month   月份 (1-12)
 * @param   day     日期 (1-31)
 * @return  一年中的第几天 (1-366)，参数无效返回 0
 */
int day_of_year(int year, int month, int day);


/**
 * @brief   计算两个日期之间的天数差
 * @param   t1  时间1
 * @param   t2  时间2
 * @return  天数差 (t2 - t1)，可以为负数
 */
int days_between(time_t t1, time_t t2);


/**
 * @brief   判定一个时间是否合法
 *
 * 校验范围:
 *   - tm_year  70-200   (即 1970-2100)
 *   - tm_mon   0-11     tm_mday 1-当月天数
 *   - tm_hour  0-23     tm_min   0-59    tm_sec 0-60 (允许闰秒)
 *   - tm_wday  见下
 *
 * 星期校验: tm_wday 落在 0-6 时会与实际星期比对, 不符则判为非法;
 * tm_wday 超出 0-6(见 UTIL_TM_WDAY_UNSET)时视为"未指定"并跳过该校验。
 * struct tm 的 tm_wday 与 weekday_by_kimlarsen() 约定一致(0 = 周日), 两者可直接比较。
 */
bool is_time_valid(struct tm* target);


/**
 * @brief   将时间字符串转换为时间戳
 * @param   str     时间字符串
 * @param   format  时间字符串格式
 *                  1. "yyyy-MM-dd HH:mm:ss"
 *                  2. "yyyy-MM-ddTHH:mm:ssZ"
 *                  3. "yyyyMMddHHmmss"
 */
time_t timestr_to_second(const char* str, const char* format);


/**
 * @brief   localtime 的线程安全版本
 */
int util_localtime(time_t* psec, struct tm* ptm);


/**
 * @brief   将时间戳转换为字符串格式
 * @param   dst     目标字符串缓冲区，必须至少 20 字节
 * @param   sec     时间戳
 * @param   format  时间字符串格式:
 *                  1. "yyyy-MM-dd HH:mm:ss"
 *                  2. "yyyy-MM-ddTHH:mm:ssZ"
 *                  3. "yyyyMMddHHmmss"
 */
int util_timestr(char* dst, time_t sec, const char* format);


void delay_ms(int ms);


/**
 * @brief  since UTC1970-01-01 00:00:00
 */
uint64_t time_ms(void);


/**
 * @brief  since system start
 */
uint64_t cpu_ms(void);


/* timer demo:
    UtilTimer timer = timer_new(true);

    while (1) {
        timer_refresh(timer);

        if (past_second(timer))
            printf("past second\n");

        if (past_interval(timer, 500))
            printf("past 500ms\n");

        delay_ms(10);
    }
 */

typedef struct UtilTimerImpl* UtilTimer;

/**
 * @param set_all_flag 初次运行时是否设置所有标识为真
 */
UtilTimer timer_new(bool set_all_flag);


void timer_del(UtilTimer self);


/* call this function in the start of loop */
void timer_refresh(UtilTimer self);


/**
 * @brief   判断用户自定义的计时器周期是否到达
 * @param   ms  用户自定义的计时器周期, 单位ms
 * @note    1.按传进的参数 ms 去 user_define 链表中查找, 若不存在则添加
 *          2.新添加的节点 last_ms 为当前时间, 即初次调用时会返回 false
 *          3.last_ms 的更新在 timer_running() 中, 因为同一周期可以在循环中多次调用
 */
bool past_interval(UtilTimer self, uint64_t ms);


bool past_second(UtilTimer self);
bool past_minute(UtilTimer self);
bool past_hour(UtilTimer self);
bool past_day(UtilTimer self);
bool past_week(UtilTimer self);
bool past_month(UtilTimer self);

/**
 * @brief   判断是否刚进入一个新季度
 * @return  true 表示本次刷新跨过了季度边界
 * @note    季度起始月为 1 / 4 / 7 / 10, 所以触发时刻是 1/4/7/10 月的 1 日 00:00。
 *          每个季度最多返回一次 true。
 * @note    若需要"季度结束"(3/6/9/12 月的边界), 请自行配合 past_month() 判断
 */
bool past_quarter(UtilTimer self);

bool past_year(UtilTimer self);


time_t now_timestamp(UtilTimer self);


int now_year(UtilTimer self);

/**
 * @brief   获取当前月份, 1-12
 */
int now_month(UtilTimer self);


/**
 * @brief   获取当前日期, 1-31
 */
int now_day(UtilTimer self);


/**
 * @brief   获取当前小时, 0-23
 */
int now_hour(UtilTimer self);


int now_minute(UtilTimer self);
int now_second(UtilTimer self);
int now_weekday(UtilTimer self);


/**
 * @brief   判断当前是否到达某时某分
 * @note    过分钟时匹配, 所以每分钟最多返回一次true
 */
bool reach_HH_mm(UtilTimer self, int hour, int minute);


#ifdef __cplusplus
}
#endif

#endif /* _UTILS_TIME_H___ */
