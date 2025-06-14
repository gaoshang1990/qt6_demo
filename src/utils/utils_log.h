#ifndef _UTILS_LOG_H___
#define _UTILS_LOG_H___

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define M_TRACE (0)
#define M_DEBUG (1)
#define M_INFO  (2)
#define M_WARN  (3)
#define M_ERROR (4)

enum {
    MLOG_ID_DEFAULT = 0,

    /* 用户可以在此扩展日志ID, 上述默认值不可更改 */
};


/**
 * @brief   日志初始化
 * @return  0: success, -1: failed
 * @note    可以重复调用来更改日志级别
 */
int mlog_init(int id, int level, const char* file_dir, const char* file_name);

int  mlog_set_level(int id, int level);
void mlog_enable_color(int id, bool enable);
void mlog_enable_console(int id, bool enable);

void mlog_write(int id, int level, bool is_raw, const char* file, int line, const char* fmt, ...);

int print_buf(int level, uint8_t* buf, uint16_t buf_len);
int print_app_info(const char* name, const char* version, const char* date, const char* time);

#define slog_init(level, dir, file)   mlog_init(MLOG_ID_DEFAULT, level, dir, file)
#define slog_set_level(level)         mlog_set_level(MLOG_ID_DEFAULT, level)
#define slog_enable_color(enable)     mlog_enable_color(MLOG_ID_DEFAULT, enable)
#define slog_enable_console(enable)   mlog_enable_console(MLOG_ID_DEFAULT, enable)
#define slog_app_info(name, version)  print_app_info(name, version, __DATE__, __TIME__)


/*!
 * Multiton mode:
 * mlog_init(0, "./log", "mlog1.log", M_TRACE);
 * mlog_init(1, "./log", "mlog2.log", M_DEBUG);
 * mlog_error(0, "mlog ERROR TEST");
 * mlog_warn(1, "mlog WARN TEST");
 */
#define mlog__(id, level, fmt, ...)   mlog_write(id, level, false, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mlog_trace(id, fmt, ...)      mlog__(id, M_TRACE, fmt, ##__VA_ARGS__)
#define mlog_debug(id, fmt, ...)      mlog__(id, M_DEBUG, fmt, ##__VA_ARGS__)
#define mlog_info(id, fmt, ...)       mlog__(id, M_INFO, fmt, ##__VA_ARGS__)
#define mlog_warn(id, fmt, ...)       mlog__(id, M_WARN, fmt, ##__VA_ARGS__)
#define mlog_error(id, fmt, ...)      mlog__(id, M_ERROR, fmt, ##__VA_ARGS__)

#define mlog_raw(id, level, fmt, ...) mlog_write(id, level, true, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mlog_trace_raw(id, fmt, ...)  mlog_raw(id, M_TRACE, fmt, ##__VA_ARGS__)
#define mlog_debug_raw(id, fmt, ...)  mlog_raw(id, M_DEBUG, fmt, ##__VA_ARGS__)
#define mlog_info_raw(id, fmt, ...)   mlog_raw(id, M_INFO, fmt, ##__VA_ARGS__)
#define mlog_warn_raw(id, fmt, ...)   mlog_raw(id, M_WARN, fmt, ##__VA_ARGS__)
#define mlog_error_raw(id, fmt, ...)  mlog_raw(id, M_ERROR, fmt, ##__VA_ARGS__)

/*!
 * singleton mode:
 * slog_init("./log", "mlog.log", M_TRACE);
 * slog_init("slog ERROR TEST");
 */
#define slog(level, fmt, ...)         mlog__(MLOG_ID_DEFAULT, level, fmt, ##__VA_ARGS__)
#define slog_trace(fmt, ...)          slog(M_TRACE, fmt, ##__VA_ARGS__)
#define slog_debug(fmt, ...)          slog(M_DEBUG, fmt, ##__VA_ARGS__)
#define slog_info(fmt, ...)           slog(M_INFO, fmt, ##__VA_ARGS__)
#define slog_warn(fmt, ...)           slog(M_WARN, fmt, ##__VA_ARGS__)
#define slog_error(fmt, ...)          slog(M_ERROR, fmt, ##__VA_ARGS__)

#define slog_raw(level, fmt, ...)     mlog_raw(MLOG_ID_DEFAULT, level, fmt, ##__VA_ARGS__)
#define slog_trace_raw(fmt, ...)      slog_raw(M_TRACE, fmt, ##__VA_ARGS__)
#define slog_debug_raw(fmt, ...)      slog_raw(M_DEBUG, fmt, ##__VA_ARGS__)
#define slog_info_raw(fmt, ...)       slog_raw(M_INFO, fmt, ##__VA_ARGS__)
#define slog_warn_raw(fmt, ...)       slog_raw(M_WARN, fmt, ##__VA_ARGS__)
#define slog_error_raw(fmt, ...)      slog_raw(M_ERROR, fmt, ##__VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif /* _UTILS_LOG_H___ */
