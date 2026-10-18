#ifndef _UTILS_SERIAL_H___
#define _UTILS_SERIAL_H___

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @file    utils_serial.h
 * @brief   串口 (UART) 收发
 *
 * 线程模型: 所有接口非线程安全, 单线程内使用, 不要跨线程共享实例。
 *
 * 典型用法:
 *
 *     UtilSerialConfig cfg;
 *     serial_config_init(&cfg);
 *     cfg.baud_rate = 115200;
 *
 *     UtilSerial com = serial_new("COM3", &cfg);
 *     if (!serial_open(com)) {
 *         printf("open failed: %s\n", serial_strerror(serial_last_error(com)));
 *         serial_del(com);
 *         return -1;
 *     }
 *
 *     serial_write(com, (const uint8_t*)"AT\r", 3);
 *     ...
 *     serial_close(com);
 *     serial_del(com);
 */

typedef struct UtilSerialImpl* UtilSerial;


/* Error codes */
typedef enum {
    UTIL_SERIAL_OK          = 0,  /* 成功 */
    UTIL_SERIAL_ERR_PARAM   = -1, /* 参数错误 */
    UTIL_SERIAL_ERR_OPEN    = -2, /* 打开设备失败 */
    UTIL_SERIAL_ERR_IO      = -3, /* 收发失败 */
    UTIL_SERIAL_ERR_BAUD    = -4, /* 波特率不被支持 */
    UTIL_SERIAL_ERR_CLOSED  = -5, /* 设备未打开 */
    UTIL_SERIAL_ERR_NOMEM   = -6  /* 内存不足 */
} UtilSerialError;


/**
 * @brief   串口参数
 *
 * serial_config_init() 会填入一组常用默认值 (9600 / 8 / 无校验 / 1 位停止位)。
 */
typedef struct {
    int    baud_rate; /* 波特率, 如 9600 / 115200 */
    uint8_t data_bits; /* 数据位 5-8 */
    char   parity;     /* 校验方式: 'N' 无 / 'E' 偶 / 'O' 奇, 大小写不敏感 */
    uint8_t stop_bits; /* 停止位 1 或 2 */
} UtilSerialConfig;


/**
 * @brief   填入默认参数: 9600 bps, 8 数据位, 无校验, 1 停止位
 */
void serial_config_init(UtilSerialConfig* cfg);


/**
 * @brief   创建串口实例 (此时并未打开设备)
 * @param   device 设备名, Windows 形如 "COM3", Linux 形如 "/dev/ttyUSB0"
 * @param   cfg    参数; 为 NULL 时使用默认值
 * @return  新实例; 参数非法或内存不足返回 NULL
 * @note    线程不安全: 返回后不要跨线程使用
 */
UtilSerial serial_new(const char* device, const UtilSerialConfig* cfg);


/**
 * @brief   销毁实例 (若仍打开会先关闭)
 */
void serial_del(UtilSerial self);


/**
 * @brief   打开设备并应用参数
 * @return  true 表示已打开
 * @note    打开失败时 serial_last_error() 给出原因
 */
bool serial_open(UtilSerial self);


/**
 * @brief   关闭设备 (未打开时为空操作, 可重复调用)
 */
void serial_close(UtilSerial self);


/**
 * @brief   设备是否处于打开状态
 */
bool serial_is_open(UtilSerial self);


/**
 * @brief   更换参数并重新打开
 * @return  true 表示重新打开成功
 * @note    实例需已创建; 内部会先关闭再按新参数打开
 */
bool serial_configure(UtilSerial self, const UtilSerialConfig* cfg);


/**
 * @brief   读取当前参数
 * @return  参数副本的地址, 始终非空
 * @note    仅在 serial_configure() / serial_new() 之后有效
 */
const UtilSerialConfig* serial_config(UtilSerial self);


/**
 * @brief   读取数据
 * @param   buf        接收缓冲区
 * @param   max_bytes  缓冲区大小
 * @return  实际读到的字节数; 0 表示当前无数据; 负值为错误码
 */
int serial_read(UtilSerial self, uint8_t* buf, int max_bytes);


/**
 * @brief   发送数据
 * @param   buf   待发送数据
 * @param   size  字节数
 * @return  实际写入的字节数; 负值为错误码
 * @note    只写入不等待发送完成 —— 返回时数据可能仍在硬件缓冲区里。
 *          需要确保已发完再切换方向 (如半双工收发) 时, 显式调用 serial_drain()
 */
int serial_write(UtilSerial self, const uint8_t* buf, int size);


/**
 * @brief   等待发送完成
 * @return  true 表示已全部发完
 */
bool serial_drain(UtilSerial self);


/**
 * @brief   读取一个字节, 最多等待 serial_set_timeout() 设定的时长
 * @return  0-255 为读到的字节; -1 表示超时、出错或未打开
 */
int serial_read_byte(UtilSerial self);


/**
 * @brief   丢弃收发缓冲区里的残留数据
 */
bool serial_purge(UtilSerial self);


/**
 * @brief   设置 serial_read_byte() 的等待上限
 * @param   ms 超时毫秒数
 * @note    需要在打开状态下调用才会立即生效
 */
void serial_set_timeout(UtilSerial self, int ms);


/**
 * @brief   启动软定时器 (用于判断"多久没收到数据")
 * @param   ms 超时毫秒数
 * @note    与 serial_read_byte() 的等待上限是两回事
 */
void serial_set_deadline(UtilSerial self, int ms);


/**
 * @brief   查询软定时器是否到期
 * @return  true 表示距 serial_set_deadline() 已超过设定时长
 */
bool serial_deadline_expired(UtilSerial self);


/**
 * @brief   最近一次操作的错误码
 */
UtilSerialError serial_last_error(UtilSerial self);


/**
 * @brief   错误码描述字符串
 */
const char* serial_strerror(UtilSerialError err);


#ifdef __cplusplus
}
#endif

#endif /* _UTILS_SERIAL_H___ */
