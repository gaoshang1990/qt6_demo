#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/time.h>
#  include <unistd.h>
#endif

#include "utils_time.h"


/**
 * @brief   获取某年月的天数
 */
int month_days(int year, int month)
{
    static const uint8_t _MONTH_TAB[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month == 2 && IS_LEAP_YEAR(year))
        return 29;

    return _MONTH_TAB[month - 1];
}

/**
 * @brief   基姆拉尔森计算星期公式
 * @retval  0-6：星期日~六
 */
int get_weekday(int year, int month, int day)
{
    if (month == 1 || month == 2) {
        month += 12;
        year--;
    }

    return (day + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400) % 7;
}

/**
 * @brief   判定一个时间的合法性，注意该检测包含非法日期检测
 * @retval  true-正确；false-错误
 */
bool check_time(struct tm* pdate)
{
    if (pdate->tm_year < 100 || pdate->tm_year > 200)
        return false;
    if ((pdate->tm_mon < 0) || (pdate->tm_mon > 11))
        return false;
    if ((pdate->tm_mday < 0) || (pdate->tm_mday > 31))
        return false;
    if ((pdate->tm_hour < 0) || (pdate->tm_hour > 23))
        return false;
    if ((pdate->tm_min < 0) || (pdate->tm_min > 59))
        return false;
    if ((pdate->tm_sec < 0) || (pdate->tm_sec > 60))
        return false;

    int year  = pdate->tm_year + 1900;
    int month = pdate->tm_mon + 1;
    int day   = pdate->tm_mday;

    if ((pdate->tm_wday != 0xffff) &&
        (((pdate->tm_wday > 0) ? (pdate->tm_wday - 1) : 6) != get_weekday(year, month, day)))
    {
        return true;
    }

    int day_of_month = month_days(year, month);
    if (day <= day_of_month)
        return true;

    return false;
}

time_t time_str_to_sec(const char* str)
{
    if (str == NULL)
        return 0;

    if (strlen(str) != TIME_STR_LEN) {
        printf("timestr[%s] error!\n", str); /* "2000-01-01 00:00:00" */
        return 0;
    }
    struct tm cur_tm;
    sscanf(str,
           "%04d-%02d-%02d %02d:%02d:%02d",
           &cur_tm.tm_year,
           &cur_tm.tm_mon,
           &cur_tm.tm_mday,
           &cur_tm.tm_hour,
           &cur_tm.tm_min,
           &cur_tm.tm_sec);
    cur_tm.tm_year -= 1900;
    cur_tm.tm_mon -= 1;
    cur_tm.tm_isdst = 0; /* 不考虑夏令时, 若此项不设置, 在linux下mktime()可能返回-1 */
    if (!check_time(&cur_tm)) {
        printf("time string[%s] is invalid!\n", str);
        return 0;
    }

    return mktime(&cur_tm);
}

int time_str(char* dst, time_t sec)
{
    if (dst == NULL)
        return -1;

    memset(dst, 0, TIME_STR_LEN + 1);

    struct tm now_tm;
    LOCAL_TIME(&sec, &now_tm);

    sprintf(dst,
            "%04d-%02d-%02d %02d:%02d:%02d",
            now_tm.tm_year + 1900,
            now_tm.tm_mon + 1,
            now_tm.tm_mday,
            now_tm.tm_hour,
            now_tm.tm_min,
            now_tm.tm_sec);

    return 0;
}


int time_str_with_ms(char* timestr, uint64_t ms)
{
    if (timestr == NULL)
        return -1;

    struct tm now_tm;
    time_t    now_sec = ms / 1000;
    int       now_ms  = ms % 1000;
    LOCAL_TIME(&now_sec, &now_tm);

    sprintf(timestr,
            "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            now_tm.tm_year + 1900,
            now_tm.tm_mon + 1,
            now_tm.tm_mday,
            now_tm.tm_hour,
            now_tm.tm_min,
            now_tm.tm_sec,
            now_ms);

    return 0;
}

bool is_same_day(time_t t1, time_t t2)
{
    struct tm tm1, tm2;
    LOCAL_TIME(&t1, &tm1);
    LOCAL_TIME(&t2, &tm2);

    return (tm1.tm_year == tm2.tm_year) && (tm1.tm_mon == tm2.tm_mon) && (tm1.tm_mday == tm2.tm_mday);
}


void delay_ms(int ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif

    return;
}

/* since UTC1970-01-01 00:00:00 */
uint64_t time_ms(void)
{
    uint64_t now_ms = 0;

#ifdef _WIN32
    static const uint64_t _DIFF_TO_UNIXTIME = 11644473600000LL;

    FILETIME ft;
    uint64_t now;

    GetSystemTimeAsFileTime(&ft);
    now    = (uint64_t)ft.dwLowDateTime + ((uint64_t)(ft.dwHighDateTime) << 32LL);
    now_ms = (now / 10000LL) - _DIFF_TO_UNIXTIME; /* now的单位为100ns */
#else
    struct timeval now;
    gettimeofday(&now, NULL);
    now_ms = ((uint64_t)now.tv_sec * 1000LL) + (now.tv_usec / 1000);
#endif

    return now_ms;
}


/* since system start */
uint64_t cpu_ms(void)
{
    uint64_t now_ms = 0;

#ifdef _WIN32
    LARGE_INTEGER frequency; /* 计时器频率 */
    LARGE_INTEGER now;       /* 当前时间 */

    if (QueryPerformanceFrequency(&frequency)) { /* 获取计时器的频率(每秒的计数) */
        if (QueryPerformanceCounter(&now)) {     /* 获取当前计数 */
            now_ms = (now.QuadPart * 1000) / frequency.QuadPart;
        }
    }

#else
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    now_ms = ((uint64_t)tp.tv_sec) * 1000LL + (tp.tv_nsec / 1000000);
#endif

    return now_ms;
}


#define TMR_FLAG_SEC  0x01
#define TMR_FLAG_MIN  0x02
#define TMR_FLAG_HOUR 0x04
#define TMR_FLAG_DAY  0x08
#define TMR_FLAG_WEEK 0x10
#define TMR_FLAG_MON  0x20
#define TMR_FLAG_YEAR 0x40


struct UserDefinedPeriod {
    uint64_t period; /* ms */
    uint64_t last_ms;

    struct UserDefinedPeriod* next;
};

struct _UtilTimer_t_ {
    uint8_t   flag;
    uint64_t  now_ms;
    struct tm last_tm;
    struct tm curr_tm;

    struct UserDefinedPeriod* user_define;
};

/**
 * @param set_all_flag 初次运行时是否设置所有标识为真
 */
UtilTimer timer_new(bool set_all_flag)
{
    UtilTimer tmr = (UtilTimer)malloc(sizeof(struct _UtilTimer_t_));
    if (tmr == NULL) {
        printf("timer_new() failed\n");
        return NULL;
    }

    if (set_all_flag)
        memset(&tmr->last_tm, 0xff, sizeof(struct tm)); /* 首次所有标识将置1 */
    else {
        time_t now_sec = time(NULL);
        LOCAL_TIME(&now_sec, &tmr->last_tm);
    }

    tmr->user_define = NULL;

    return tmr;
}

void timer_del(UtilTimer self)
{
    if (self != NULL) {
        free(self);
        self = NULL;
    }
}

/**
 * @brief   用户自定义的定时器周期处理, 在 timer_running() 中清除 flag 之前调用
 */
static void _user_defined_running(UtilTimer self)
{
    struct UserDefinedPeriod* p = self->user_define;
    while (p != NULL) {
        if (self->now_ms - p->last_ms >= p->period)
            p->last_ms = self->now_ms;

        p = p->next;
    }
}

/**
 * @brief   call this function in the start of loop
 */
void timer_running(UtilTimer self)
{
    /* 先处理自定义周期 */
    _user_defined_running(self);

    self->flag = 0; /* clear timer flag */

    time_t now_sec = time(NULL);
    LOCAL_TIME(&now_sec, &self->curr_tm);

    self->now_ms = time_ms();

    if (self->curr_tm.tm_sec != self->last_tm.tm_sec) {
        self->last_tm.tm_sec = self->curr_tm.tm_sec;
        self->flag |= TMR_FLAG_SEC;
    }
    if (self->curr_tm.tm_min != self->last_tm.tm_min) {
        self->last_tm.tm_min = self->curr_tm.tm_min;
        self->flag |= TMR_FLAG_MIN;
    }
    if (self->curr_tm.tm_hour != self->last_tm.tm_hour) {
        self->last_tm.tm_hour = self->curr_tm.tm_hour;
        self->flag |= TMR_FLAG_HOUR;
    }
    if (self->curr_tm.tm_mday != self->last_tm.tm_mday) {
        self->last_tm.tm_mday = self->curr_tm.tm_mday;
        self->flag |= TMR_FLAG_DAY;
    }
    if (self->curr_tm.tm_wday != self->last_tm.tm_wday) {
        self->last_tm.tm_wday = self->curr_tm.tm_wday;
        self->flag |= TMR_FLAG_WEEK;
    }
    if (self->curr_tm.tm_mon != self->last_tm.tm_mon) {
        self->last_tm.tm_mon = self->curr_tm.tm_mon;
        self->flag |= TMR_FLAG_MON;
    }
    if (self->curr_tm.tm_year != self->last_tm.tm_year) {
        self->last_tm.tm_year = self->curr_tm.tm_year;
        self->flag |= TMR_FLAG_YEAR;
    }

    return;
}

static struct UserDefinedPeriod* _user_last_node(UtilTimer self)
{
    struct UserDefinedPeriod* p = self->user_define;
    while (p != NULL) {
        if (p->next == NULL)
            return p;
        p = p->next;
    }

    return NULL;
}

/**
 * @brief   添加一个用户自定义的定时器周期, 并返回新节点指针
 */
static struct UserDefinedPeriod* _user_add_node(UtilTimer self, uint64_t ms)
{
    struct UserDefinedPeriod* new_node = (struct UserDefinedPeriod*)malloc(sizeof(struct UserDefinedPeriod));
    if (new_node == NULL)
        return NULL;

    new_node->period  = ms;
    new_node->last_ms = self->now_ms;
    new_node->next    = NULL;

    struct UserDefinedPeriod* last_node = _user_last_node(self);
    if (last_node == NULL)
        self->user_define = new_node;
    else
        last_node->next = new_node;

    return new_node;
}

/**
 * @brief   查找用户自定义的定时器周期, 若不存在则添加并返回新节点指针
 */
struct UserDefinedPeriod* _user_find_node(UtilTimer self, uint64_t ms)
{
    struct UserDefinedPeriod* p = self->user_define;
    while (p != NULL) {
        if (p->period == ms)
            return p;
        p = p->next;
    }

    return _user_add_node(self, ms);
}

/**
 * @brief   判断用户自定义的定时器周期是否到达
 * @param   ms  用户自定义的定时器周期, 单位ms
 * @note    1.按传进的参数 ms 去 user_define 链表中查找, 若不存在则添加
 *          2.新添加的节点 last_ms 为当前时间, 即初次调用时会返回 false
 *          3.last_ms 的更新在 timer_running() 中, 因为同一周期可以在循环中多次调用
 */
bool past_few_ms(UtilTimer self, uint64_t ms)
{
    struct UserDefinedPeriod* user_node = _user_find_node(self, ms);
    if (user_node == NULL)
        return false;

    if (self->now_ms - user_node->last_ms < ms)
        return false;

    return true;
}

bool past_second(UtilTimer self)
{
    return self->flag & TMR_FLAG_SEC;
}

bool past_minute(UtilTimer self)
{
    return self->flag & TMR_FLAG_MIN;
}

bool past_hour(UtilTimer self)
{
    return self->flag & TMR_FLAG_HOUR;
}

bool past_day(UtilTimer self)
{
    return self->flag & TMR_FLAG_DAY;
}

bool past_week(UtilTimer self)
{
    return self->flag & TMR_FLAG_WEEK;
}

bool past_month(UtilTimer self)
{
    return self->flag & TMR_FLAG_MON;
}

bool past_quarter(UtilTimer self)
{
    return (past_month(self) && (now_month(self) + 2) % 3 == 0);
}

bool past_year(UtilTimer self)
{
    return self->flag & TMR_FLAG_YEAR;
}

/**
 * @brief   获取当前年份
 */
int now_year(UtilTimer self)
{
    return self->curr_tm.tm_year + 1900;
}

/**
 * @brief   获取当前月份
 * @retval  1-12
 */
int now_month(UtilTimer self)
{
    return self->curr_tm.tm_mon + 1;
}

/**
 * @brief   获取当前日期
 * @retval  1-31
 */
int now_day(UtilTimer self)
{
    return self->curr_tm.tm_mday;
}

/**
 * @brief   获取当前小时
 * @retval  0-23
 */
int now_hour(UtilTimer self)
{
    return self->curr_tm.tm_hour;
}

/**
 * @brief   获取当前分钟
 * @retval  0-59
 */
int now_minute(UtilTimer self)
{
    return self->curr_tm.tm_min;
}

/**
 * @brief   获取当前秒
 * @retval  0-59
 */
int now_second(UtilTimer self)
{
    return self->curr_tm.tm_sec;
}

/**
 * @brief   获取当前星期
 * @retval  1-7
 */
int now_weekday(UtilTimer self)
{
    return self->curr_tm.tm_wday + 1;
}

/**
 * @brief   判断当前是否是某时某分
 * @note    过分钟时匹配, 所以每分钟最多返回一次true
 */
bool does_time_match(UtilTimer self, int hour, int minute)
{
    if (!past_minute(self))
        return false;

    if (hour != now_hour(self))
        return false;

    if (minute != now_minute(self))
        return false;

    return true;
}
