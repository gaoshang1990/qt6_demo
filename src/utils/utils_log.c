#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#  include <windows.h>
#  include <io.h>
#else
#  include <unistd.h>
#  include <pthread.h>
#  include <semaphore.h>
#endif

#include "utils_log.h"


#ifdef _WIN32
#  define mkdir(path, arg)      _mkdir(path)
#  define access                _access
#  define snprintf              _snprintf
#  define LOCAL_TIME(pSec, pTm) localtime_s(pTm, pSec)
#else
#  define LOCAL_TIME(pSec, pTm) localtime_r(pSec, pTm)
#endif

#define MLOG_FILE_MAX_SIZE   (5 * 1024 * 1024) /* max size of logfile */
#define MLOG_FILE_MAX_ROTATE (5)               /* rotate file max num */
#define MAX_TIME_STR         (20)
#define MAX_FILE_PATH_LEN    (256)
#define LINE_CONTENT_MAX_LEN (10 * 1024)
#define LINE_PREFIX_MAX_LEN  (256)


/**
 * CSI(Control Sequence Introducer/Initiator) sign
 * more information on https://en.wikipedia.org/wiki/ANSI_escape_code
 */
#define CSI_START            "\033["
#define CSI_END              "\033[0m"
/* output log front color */
#define F_BLACK              "30;"
#define F_RED                "31;"
#define F_GREEN              "32;"
#define F_YELLOW             "33;"
#define F_BLUE               "34;"
#define F_MAGENTA            "35;"
#define F_CYAN               "36;"
#define F_WHITE              "37;"
/* output log background color */
#define B_NULL
#define B_BLACK          "40;"
#define B_RED            "41;"
#define B_GREEN          "42;"
#define B_YELLOW         "43;"
#define B_BLUE           "44;"
#define B_MAGENTA        "45;"
#define B_CYAN           "46;"
#define B_WHITE          "47;"
/* output log fonts style */
#define STYLE_BOLD       "1m"
#define STYLE_UNDERLINE  "4m"
#define STYLE_BLINK      "5m"
#define STYLE_NORMAL     "22m"

/* change the some level logs to not default color if you want */
#define MLOG_COLOR_ERROR F_RED B_NULL STYLE_NORMAL
#define MLOG_COLOR_WARN  F_YELLOW B_NULL STYLE_NORMAL
#define MLOG_COLOR_INFO  F_CYAN B_NULL STYLE_NORMAL
#define MLOG_COLOR_DEBUG F_GREEN B_NULL STYLE_NORMAL
#define MLOG_COLOR_TRACE F_BLUE B_NULL STYLE_NORMAL


#define VSNSPRINTF_WRAPPER(buf, buf_size, fmt)  \
    do {                                        \
        va_list args___;                        \
        va_start(args___, fmt);                 \
        vsnprintf(buf, buf_size, fmt, args___); \
        va_end(args___);                        \
    } while (0)


typedef void* MLogMutex;

struct MLogger {
    int       id;
    int       level;
    char      dir[128];
    char      name[64];
    FILE*     fp;
    MLogMutex mtx;
    int       max_size; /* file max size */
    int       max_num;  /* max rotate file count */

    bool en_print; /* enable print output to console fd=stdout */
    bool en_color; /* enalbe print color. support on linux/unix platform */

    char line_prefix[LINE_PREFIX_MAX_LEN];   /* prefix of log */
    char line_content[LINE_CONTENT_MAX_LEN]; /* content of log */
};


struct MLogInst {
    int              num;
    struct MLogger** loggers;

    MLogMutex lock;
};


static struct MLogInst* _mlog_inst()
{
    static struct MLogInst _mlog = {0};

    return &_mlog;
}


static MLogMutex _mlog_lock_init(int init_value)
{
#ifdef _WIN32
    HANDLE self = CreateSemaphore(NULL, init_value, 1, NULL);
#else
    MLogMutex self = malloc(sizeof(sem_t));
    sem_init((sem_t*)self, 0, init_value);
#endif

    return self;
}


static void _mlog_lock(MLogMutex self)
{
#ifdef _WIN32
    WaitForSingleObject((HANDLE)self, INFINITE);
#else
    sem_wait((sem_t*)self);
#endif
}


static void _mlog_unlock(MLogMutex self)
{
#ifdef _WIN32
    ReleaseSemaphore((HANDLE)self, 1, NULL);
#else
    sem_post((sem_t*)self);
#endif
}


static char* _process_info(void)
{
    static char cur_process_info[10] = {0};

    int pid =
#ifdef _WIN32
        (int)GetCurrentProcessId();
#else
        (int)getpid();
#endif
    snprintf(cur_process_info, 10, "pid: %04d", pid);

    return cur_process_info;
}


static char* _thread_info(void)
{
    static char cur_thread_info[10] = {0};

    int tid =
#ifdef _WIN32
        (int)GetCurrentThreadId();
#else
        (int)pthread_self();
#endif
    snprintf(cur_thread_info, 10, "tid: %04d", tid);

    return cur_thread_info;
}


static void _make_curr_timestr(char* timestr, int size)
{
    struct tm now_tm;
    time_t    now_sec = time(NULL);
    LOCAL_TIME(&now_sec, &now_tm);

    snprintf(timestr,
             size,
             "%04u-%02u-%02u %02u:%02u:%02u",
             now_tm.tm_year + 1900,
             now_tm.tm_mon + 1,
             now_tm.tm_mday,
             now_tm.tm_hour,
             now_tm.tm_min,
             now_tm.tm_sec);
}


/* move xxx_1.log => xxx_n.log, and xxx.log => xxx_0.log */
static void _rotate_file(struct MLogger* logger)
{
    char file_name[120] = {0}; /* file name without suffix */
    char suffix[16]     = {0}; /* file suffix */
    sscanf(logger->name, "%119[^.]%15s", file_name, suffix);

    char old_path[256] = {0};
    char new_path[256] = {0};

    snprintf(old_path, sizeof(old_path) - 1, "%s/%s", logger->dir, file_name);
    size_t base_len = strlen(old_path);
    snprintf(new_path, base_len + 1, "%s", old_path);

    const uint8_t suffix_len = 32;
    for (int n = logger->max_num - 1; n >= 0; --n) {
        if (n == 0)
            snprintf(old_path + base_len, suffix_len, "%s", suffix);
        else
            snprintf(old_path + base_len, suffix_len, "_%d%s", n - 1, suffix);

        snprintf(new_path + base_len, suffix_len, "_%d%s", n, suffix);
        remove(new_path);
        rename(old_path, new_path);
    }
}


/* Create a multi-level directory */
static int _mkdir_m_(const char* dir)
{
    if (dir == NULL) {
        printf("_mkdir_m_: dir is NULL\n");
        return -1;
    }

    int len = strlen(dir) + 1;
    if (len <= 0 || len > MAX_FILE_PATH_LEN) {
        printf("_mkdir_m_: strlen(dir) = %d\n", len);
        return -1;
    }

    char dirTmp[256] = {0};
    strncpy(dirTmp, dir, sizeof(dirTmp) - 1);

    for (int i = 0; i < len; i++) {
        if (dirTmp[i] == '\\' || dirTmp[i] == '/' || dirTmp[i] == '\0') {
            dirTmp[i] = '\0';
            if (strlen(dirTmp) > 0 && access(dirTmp, 0) != 0) {
                if (mkdir(dirTmp, 0755) != 0) {
                    printf("mkdir[%s] failed\n", dirTmp);
                    return -1;
                }
            }
            dirTmp[i] = dir[i];
        }
    }

    return 0;
}


static struct MLogger* _mlog_new(int id)
{
    struct MLogger* logger = (struct MLogger*)malloc(sizeof(struct MLogger));
    memset(logger, 0, sizeof(struct MLogger));

    logger->id       = id;
    logger->level    = M_DEBUG;
    logger->mtx      = _mlog_lock_init(1);
    logger->max_size = MLOG_FILE_MAX_SIZE;
    logger->max_num  = MLOG_FILE_MAX_ROTATE;
    logger->en_print = true;
    logger->en_color = true;

    return logger;
}

/**
 * @brief   自动扩充logger实例, 考虑到日志实例数量不会太多, 每次只增加一个
 */
static void _mlog_grow(void)
{
    struct MLogInst* mlog    = _mlog_inst();
    struct MLogger** loggers = (struct MLogger**)malloc(sizeof(struct MLogger*) * (mlog->num + 1));

    memcpy(loggers, mlog->loggers, sizeof(struct MLogger*) * mlog->num);
    free(mlog->loggers);

    mlog->loggers = loggers;
}

/**
 * @brief   根据id获取logger, 如果不存在则新建一个
 */
static struct MLogger* _get_logger(int id)
{
    struct MLogInst* mlog = _mlog_inst();

    for (int i = 0; i < mlog->num; i++) {
        if (mlog->loggers[i]->id == id)
            return mlog->loggers[i];
    }

    struct MLogger* logger = NULL;

    if (mlog->lock == NULL)
        mlog->lock = _mlog_lock_init(1);

    _mlog_lock(mlog->lock);
    {
        /* not found, create a new logger */
        _mlog_grow();
        logger                     = _mlog_new(id);
        mlog->loggers[mlog->num++] = logger;
    }
    _mlog_unlock(mlog->lock);

    return logger;
}


/**
 * @brief   日志初始化
 * @return  0: success, -1: failed
 * @note    可以重复调用来更改日志级别
 */
int mlog_init(int id, int level, const char* file_dir, const char* file_name)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001); /* 控制台按utf-8输出中文编码 */
#endif
    struct MLogger* logger = _get_logger(id);

    logger->level = level;

    if (file_dir == NULL || file_name == NULL) {
        printf("mlog_init: log_dir or file_name is NULL\n");
        return -1;
    }

    if (_mkdir_m_(file_dir) < 0) {
        printf("mlog_init: mkdir[%s] failed\n", file_dir);
        return -1;
    }

    snprintf(logger->dir, sizeof(logger->dir) - 1, "%s", file_dir);
    snprintf(logger->name, sizeof(logger->name) - 1, "%s", file_name);

    return 0;
}


int mlog_set_level(int id, int level)
{
    struct MLogger* logger = _get_logger(id);

    logger->level = level;

    return 0;
}


void mlog_enable_console(int id, bool enable)
{
    struct MLogger* logger = _get_logger(id);
    logger->en_print       = enable;
}


void mlog_enable_color(int id, bool enable)
{
    struct MLogger* logger = _get_logger(id);
    logger->en_color       = enable;
}


static const char* _level_str(int level)
{
    switch (level) {
    default:
    case M_TRACE:
        return "[TRACE]";
    case M_DEBUG:
        return "[DEBUG]";
    case M_INFO:
        return "[INFO ]";
    case M_WARN:
        return "[WARN ]";
    case M_ERROR:
        return "[ERROR]";
    }
}


static int _make_line_prefix(char* prefix, int level, const char* file, int line)
{
    char timestr[MAX_TIME_STR] = {0};
    _make_curr_timestr(timestr, sizeof(timestr)); /* time */

    const char* szlevel = _level_str(level);

    /* log format for different level please modify below */
    switch (level) {
    case M_WARN:
    case M_ERROR:
        snprintf(prefix, LINE_PREFIX_MAX_LEN - 1, "%s %s- %s: %d | ", szlevel, timestr, file, line);
        break;
    case M_TRACE:
        snprintf(prefix,
                 LINE_PREFIX_MAX_LEN - 1,
                 "%s %s [%s %s] - %s: %d | ",
                 szlevel,
                 timestr,
                 _process_info(),
                 _thread_info(),
                 file,
                 line);
        break;
    case M_DEBUG:
    case M_INFO:
    default:
        snprintf(prefix, LINE_PREFIX_MAX_LEN - 1, "%s %s | ", szlevel, timestr);
        break;
    }

    return 0;
}


static void _print_in_console(struct MLogger* logger, int level)
{
    if (logger->en_print == false)
        return;

    if (logger->en_color) {
        switch (level) {
        default:
        case M_TRACE:
            printf(CSI_START MLOG_COLOR_TRACE "%s%s" CSI_END, logger->line_prefix, logger->line_content);
            break;
        case M_DEBUG:
            printf(CSI_START MLOG_COLOR_DEBUG "%s%s" CSI_END, logger->line_prefix, logger->line_content);
            break;
        case M_INFO:
            printf(CSI_START MLOG_COLOR_INFO "%s%s" CSI_END, logger->line_prefix, logger->line_content);
            break;
        case M_WARN:
            printf(CSI_START MLOG_COLOR_WARN "%s%s" CSI_END, logger->line_prefix, logger->line_content);
            break;
        case M_ERROR:
            printf(CSI_START MLOG_COLOR_ERROR "%s%s" CSI_END, logger->line_prefix, logger->line_content);
            break;
        }
    }
    else {
        printf("%s%s", logger->line_prefix, logger->line_content);
    }

    return;
}


static void _mlog_file_rotate(struct MLogger* logger)
{
    if (logger->fp == NULL)
        return;

    int         fd = fileno(logger->fp);
    struct stat statbuf;
    fstat(fd, &statbuf);

    int file_size = statbuf.st_size;
    if (file_size >= logger->max_size) {
        fclose(logger->fp);
        logger->fp = NULL;

        _rotate_file(logger);
    }

    return;
}


static void _mlog_file_open(struct MLogger* logger)
{
    if (logger->fp == NULL) {
        char file_path[256] = {0};
        snprintf(file_path, sizeof(file_path) - 1, "%s/%s", logger->dir, logger->name);
        logger->fp = fopen(file_path, "a+");
    }

    return;
}


static void _mlog_file_write(struct MLogger* logger)
{
    _mlog_file_open(logger);

    const char* prefix  = logger->line_prefix;
    const char* content = logger->line_content;

    if (logger->fp) {
        fwrite(prefix, sizeof(char), strlen(prefix), logger->fp);
        fwrite(content, sizeof(char), strlen(content), logger->fp);
        fflush(logger->fp);
    }

    return;
}


/*!
 * warn: The maximum length of the output content at one time is 10KB,
 * and exceeding the length will result in truncation.
 * Through MAX_LOG_LINE macro can modify maximum length
 */
void mlog_write(int id, int level, bool is_raw, const char* file, int line, const char* fmt, ...)
{
    struct MLogger* logger = _get_logger(id);
    if (logger->level > level)
        return;

    if (level > M_ERROR)
        level = M_ERROR;

    _mlog_lock(logger->mtx);
    {
        char* line_content = logger->line_content;
        char* line_prefix  = logger->line_prefix;
        line_content[0]    = '\0';
        line_prefix[0]     = '\0';

        VSNSPRINTF_WRAPPER(line_content, LINE_CONTENT_MAX_LEN - 1, fmt);

        if (is_raw == false) {
            _make_line_prefix(line_prefix, level, file, line);
            snprintf(line_content + strlen(line_content), LINE_CONTENT_MAX_LEN - strlen(line_content) - 1, "\n");
        }

        _print_in_console(logger, level);
        _mlog_file_rotate(logger);
        _mlog_file_write(logger);
    }
    _mlog_unlock(logger->mtx);

    return;
}


int print_buf(int level, uint8_t* buf, uint16_t buf_len)
{
    if (buf == NULL && buf_len == 0)
        return -1;

    for (int i = 0; i < buf_len; i++)
        slog_raw(level, "%02x ", buf[i]);

    slog_raw(level, "\n");

    return 0;
}

/**
 * @brief   make star string with '*'
 */
static void _make_star_str(char* str, size_t nb)
{
    for (size_t i = 0; i < nb; i++)
        str[i] = '*';

    return;
}

/**
 * @brief   make info string with ' *' at the end
 */
static void _make_info_str(char* str, size_t nb)
{
    for (size_t i = strlen(str); i < nb - 1; i++)
        str[i] = ' ';

    str[nb - 1] = '*';

    return;
}


int print_app_info(const char* name, const char* version, const char* date, const char* time)
{
    char stars[128]    = {0};
    char app_info[128] = {0};
    char app_ver[128]  = {0};
    char app_date[128] = {0};

    snprintf(app_info, sizeof(app_info), "* This is \"%s\" App", name);
    snprintf(app_ver, sizeof(app_ver), "* Version: %s", version);
    snprintf(app_date, sizeof(app_date), "* Build time: %s, %s", date, time);

    /* get the max length of the three strings */
    int max_len = strlen(app_info);
    if (strlen(app_ver) > max_len)
        max_len = strlen(app_ver);
    if (strlen(app_date) > max_len)
        max_len = strlen(app_date);

    max_len += 2; /* add 2 for ' ' and '*' */
    if (max_len > 128)
        max_len = 128;

    _make_star_str(stars, max_len);
    _make_info_str(app_info, max_len);
    _make_info_str(app_ver, max_len);
    _make_info_str(app_date, max_len);

    slog_info_raw("\n");
    slog_info("%s", stars);
    slog_info("%s", app_info);
    slog_info("%s", app_ver);
    slog_info("%s", app_date);
    slog_info("%s", stars);
    slog_info_raw("\n");

    return 0;
}
