#ifndef _UTILS_TIME_H___
#define _UTILS_TIME_H___

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIME_STR_LEN       19 /* "YYYY-MM-DD hh:mm:ss" 不含结束符 */
#define IS_LEAP_YEAR(year) ((year % 4 == 0 && year % 100) || year % 400 == 0)


#ifdef _WIN32
#  define LOCAL_TIME(pSec, pTm) localtime_s(pTm, pSec)
#else
#  define LOCAL_TIME(pSec, pTm) localtime_r(pSec, pTm)
#endif


time_t time_str_to_sec(const char* str);
int    time_str(char* dst, time_t sec);
int    time_str_with_ms(char* timestr, uint64_t ms);

bool is_same_day(time_t t1, time_t t2);

int get_weekday(int year, int month, int day);
int month_days(int year, int month);

bool check_time(struct tm* pdate);

void     delay_ms(int ms);
uint64_t time_ms(void);
uint64_t cpu_ms(void);

/* timer demo:
    UtilTimer timer = timer_new(true);

    while (1) {
        timer_running(timer);

        if (past_second(timer))
            printf("past second\n");

        if (past_few_ms(timer, 500))
            printf("past 500ms\n");

        delay_ms(10);
    }
 */

typedef struct _UtilTimer_t_* UtilTimer;

/**
 * @param set_all_flag 初次运行时是否设置所有标识为真
 */
UtilTimer timer_new(bool set_all_flag);
void      timer_del(UtilTimer self);

/* call this function in the start of loop */
void timer_running(UtilTimer self);

bool past_few_ms(UtilTimer self, uint64_t ms);
bool past_second(UtilTimer self);
bool past_minute(UtilTimer self);
bool past_hour(UtilTimer self);
bool past_day(UtilTimer self);
bool past_week(UtilTimer self);
bool past_month(UtilTimer self);
bool past_quarter(UtilTimer self);
bool past_year(UtilTimer self);

int now_year(UtilTimer self);
int now_month(UtilTimer self);
int now_day(UtilTimer self);
int now_hour(UtilTimer self);
int now_minute(UtilTimer self);
int now_second(UtilTimer self);
int now_weekday(UtilTimer self);

bool does_time_match(UtilTimer self, int hour, int minute);

#ifdef __cplusplus
}
#endif

#endif /* _UTILS_TIME_H___ */
