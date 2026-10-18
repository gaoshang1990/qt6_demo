#ifndef _UTILS_SOCKET_H___
#define _UTILS_SOCKET_H___

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file    utils_socket.h
 * @brief   极简 TCP Socket 库 (单线程 + select 事件循环)
 *
 * 设计目标:
 * 1. 无抽象基类、无虚函数表 —— 只有一种 socket, 不需要多态
 * 2. 回调只有 3 个 —— connected / readable / disconnected, 覆盖全部实际需求
 * 3. 服务器自动管理所有客户端连接, process_events() 一次驱动全部
 * 4. 需要 DNS 时直接用 getaddrinfo(), 本库不封装
 *
 * 线程模型: 所有对象非线程安全, 必须在单线程内使用, 且不要跨线程共享句柄。
 *
 * 典型用法 (客户端):
 *
 *     UtilSocket sock = socket_new();
 *     socket_connect(sock, "127.0.0.1", 8080);
 *     socket_process_events(sock, 100);   // 在主循环里反复调用
 *     ...
 *     socket_del(sock);
 *
 * 典型用法 (服务器):
 *
 *     UtilSocketServer server = socket_server_start("0.0.0.0", 8080, 16);
 *     while (running) {
 *         socket_server_process_events(server, 100);
 *     }
 *     socket_server_del(server);
 */

/* Forward declarations */
typedef struct UtilSocketImpl* UtilSocket;
typedef struct UtilSocketServerImpl* UtilSocketServer;

/* Error codes */
typedef enum {
    UTIL_SOCKET_OK               = 0,
    UTIL_SOCKET_ERR_PARAM        = -1, /* 参数错误 */
    UTIL_SOCKET_ERR_NETWORK      = -2, /* 网络错误 */
    UTIL_SOCKET_ERR_TIMEOUT      = -3, /* 超时 */
    UTIL_SOCKET_ERR_REFUSED      = -4, /* 连接被拒绝 */
    UTIL_SOCKET_ERR_HOST         = -5, /* 主机名解析失败 */
    UTIL_SOCKET_ERR_CLOSED       = -6, /* 对端已关闭 */
    UTIL_SOCKET_ERR_WOULDBLOCK   = -7, /* 非阻塞模式下暂时无数据 */
    UTIL_SOCKET_ERR_RESOURCE     = -8  /* 内存或句柄资源不足 */
} UtilSocketError;

/* Connection state */
typedef enum {
    UTIL_SOCKET_STATE_IDLE       = 0, /* 未连接 */
    UTIL_SOCKET_STATE_CONNECTING = 1, /* 连接中 */
    UTIL_SOCKET_STATE_CONNECTED  = 2, /* 已连接 */
    UTIL_SOCKET_STATE_CLOSED     = 3  /* 已关闭 */
} UtilSocketState;

/* Callback types — 3 个事件 + 1 个错误 */
typedef void (*UtilSocketConnected_cb)(UtilSocket sock, void* context);
typedef void (*UtilSocketReadable_cb)(UtilSocket sock, void* context);
typedef void (*UtilSocketDisconnected_cb)(UtilSocket sock, void* context);
typedef void (*UtilSocketError_cb)(UtilSocket sock, UtilSocketError err, void* context);

/**
 * @brief   连接回调集合
 *
 * 不需要的事件直接置 NULL 即可。
 * on_connected:   连接建立后触发 (客户端)。服务器接受的连接已处于连接态, 不触发
 * on_readable:    有数据可读时触发, 需在回调内调用 socket_read() 取数据
 * on_disconnected: 对端关闭或主动断开后触发, 之后 socket 不再有效
 * on_error:       发生错误时触发, err 参数说明原因
 */
typedef struct {
    UtilSocketConnected_cb    on_connected;
    UtilSocketReadable_cb     on_readable;
    UtilSocketDisconnected_cb on_disconnected;
    UtilSocketError_cb        on_error;
    void*                      context;
} UtilSocketCallbacks;

/**
 * @brief   服务器回调集合
 *
 * on_new_connection: 新连接到达时触发。sock 参数由服务器管理,
 *                    服务器会自动把它加入事件循环, 无需用户保存。
 *                    该连接的回调可用 socket_server_set_client_callbacks() 预设。
 * on_closed:         服务器关闭后触发
 */
typedef struct {
    void (*on_new_connection)(UtilSocketServer server, UtilSocket sock, void* context);
    void (*on_closed)(UtilSocketServer server, void* context);
    void* context;
} UtilSocketServerCallbacks;


/* ------------------------------------------------------------------ */
/* ----------------------------- 客户端 ----------------------------- */
/* ------------------------------------------------------------------ */

/**
 * @brief   创建客户端 socket
 * @return  新实例, 失败返回 NULL
 */
UtilSocket socket_new(void);

/**
 * @brief   设置回调
 * @note    必须在 socket_connect() 之前设置
 */
UtilSocketError socket_set_callbacks(UtilSocket sock, const UtilSocketCallbacks* callbacks);

/**
 * @brief   发起非阻塞连接
 * @param   host 主机名或 IP 地址
 * @param   port 端口号
 * @return  UTIL_SOCKET_OK 表示连接已发起 (不代表已建立),
 *          连接结果通过 on_connected 或 on_error 回调通知
 * @note    内部使用 getaddrinfo() 解析域名, 该调用是阻塞的
 * @note    同步失败(域名解析失败、所有候选地址都连不上)同样会触发 on_error,
 *          同时通过返回值报告 —— 两种方式都会通知, 按需选用
 */
UtilSocketError socket_connect(UtilSocket sock, const char* host, uint16_t port);

/**
 * @brief   读取数据 (非阻塞)
 * @param   buf  接收缓冲区
 * @param   size 缓冲区大小
 * @return  读取字节数; 无数据返回 UTIL_SOCKET_ERR_WOULDBLOCK(-7);
 *          对端关闭返回 UTIL_SOCKET_ERR_CLOSED(-6); 其它负值为错误码
 */
int socket_read(UtilSocket sock, void* buf, int size);

/**
 * @brief   读取一行 (以 \n 结尾, 不含 \n)
 * @param   buf  接收缓冲区
 * @param   size 缓冲区大小
 * @return  行长度; 无完整行返回 UTIL_SOCKET_ERR_WOULDBLOCK(-7); 对端关闭返回负值
 */
int socket_read_line(UtilSocket sock, char* buf, int size);

/**
 * @brief   发送数据 (非阻塞)
 * @param   data 要发送的数据
 * @param   size 数据长度
 * @return  实际发送字节数 (可能小于 size, 需自行重发剩余部分);
 *          负值为错误码
 * @note    TCP 不保证一次发完。缓冲区满时返回已发送字节数, 不返回错误。
 */
int socket_write(UtilSocket sock, const void* data, int size);

/**
 * @brief   发送字符串 (自动计算长度, 不含结尾 0)
 */
int socket_write_string(UtilSocket sock, const char* str);

/**
 * @brief   等待连接建立
 * @param   timeout_ms 超时毫秒数, -1 表示一直等
 * @return  连接成功返回 true, 超时或失败返回 false
 */
bool socket_wait_connected(UtilSocket sock, int timeout_ms);

/**
 * @brief   等待数据可读
 * @param   timeout_ms 超时毫秒数, -1 表示一直等
 * @return  有数据返回 true, 超时返回 false
 */
bool socket_wait_readable(UtilSocket sock, int timeout_ms);

/**
 * @brief   驱动一次事件 (在主循环中反复调用)
 * @param   timeout_ms 本次最多阻塞的毫秒数
 * @return  UTIL_SOCKET_OK
 * @note    这是单线程事件循环的驱动函数, 触发回调
 */
UtilSocketError socket_process_events(UtilSocket sock, int timeout_ms);

/**
 * @brief   主动关闭连接 (会触发 on_disconnected)
 * @note    关闭后实例仍然存在, 需再调 socket_del() 释放
 */
void socket_close(UtilSocket sock);

/**
 * @brief   销毁实例
 */
void socket_del(UtilSocket sock);

/* 查询接口 */
UtilSocketState socket_state(UtilSocket sock);
UtilSocketError  socket_last_error(UtilSocket sock);
bool             socket_is_connected(UtilSocket sock);

/**
 * @brief   获取对端地址
 * @param   buf  接收缓冲区, 至少 32 字节
 * @param   size 缓冲区大小
 * @param   port 存储端口, 可为 NULL
 * @return  UTIL_SOCKET_OK 表示成功
 */
UtilSocketError socket_peer_address(UtilSocket sock, char* buf, int size, uint16_t* port);

/**
 * @brief   获取本地地址
 * @param   buf  接收缓冲区, 至少 32 字节
 * @param   size 缓冲区大小
 * @param   port 存储端口, 可为 NULL
 * @return  UTIL_SOCKET_OK 表示成功
 */
UtilSocketError socket_local_address(UtilSocket sock, char* buf, int size, uint16_t* port);

/**
 * @brief   获取错误码描述字符串
 */
const char* socket_strerror(UtilSocketError err);


/* ------------------------------------------------------------------ */
/* ------------------------------ 服务器 ----------------------------- */
/* ------------------------------------------------------------------ */

/**
 * @brief   创建服务器实例 (尚未监听)
 * @param   ip   监听地址, NULL 或 "0.0.0.0" 表示所有接口
 * @param   port 监听端口, 0 表示由系统自动分配
 * @return  新实例, 失败返回 NULL
 */
UtilSocketServer socket_server_new(const char* ip, uint16_t port);

/**
 * @brief   创建并开始监听 (便捷函数)
 * @param   backlog 最大待处理连接数
 * @return  已处于监听状态的实例, 失败返回 NULL
 */
UtilSocketServer socket_server_start(const char* ip, uint16_t port, int backlog);

/**
 * @brief   设置服务器回调
 * @note    必须在 socket_server_listen() 之前设置
 */
UtilSocketError socket_server_set_callbacks(UtilSocketServer server, const UtilSocketServerCallbacks* callbacks);

/**
 * @brief   为所有新接受的连接预设回调
 * @param   client_callbacks 回调集合, NULL 表示不预设
 * @note    这样 on_new_connection 里就不用再逐个设置回调了
 */
UtilSocketError socket_server_set_client_callbacks(UtilSocketServer server, const UtilSocketCallbacks* client_callbacks);

/**
 * @brief   开始监听
 * @param   backlog 最大待处理连接数
 */
UtilSocketError socket_server_listen(UtilSocketServer server, int backlog);

/**
 * @brief   驱动一次事件 (在主循环中反复调用)
 * @param   timeout_ms 本次最多阻塞的毫秒数
 * @return  UTIL_SOCKET_OK
 * @note    一次调用处理所有事件: 新连接、读数据、断开连接
 */
UtilSocketError socket_server_process_events(UtilSocketServer server, int timeout_ms);

/**
 * @brief   主动断开并销毁某个客户端连接
 * @param   sock 要断开的连接, 必须是本服务器管理的连接
 * @return  UTIL_SOCKET_OK 表示成功
 */
UtilSocketError socket_server_disconnect(UtilSocketServer server, UtilSocket sock);

/**
 * @brief   关闭服务器 (会断开所有客户端连接, 触发 on_closed)
 */
void socket_server_close(UtilSocketServer server);

/**
 * @brief   销毁服务器实例 (自动断开并销毁所有客户端连接)
 */
void socket_server_del(UtilSocketServer server);

/* 查询接口 */
int      socket_server_client_count(UtilSocketServer server);
bool     socket_server_listening(UtilSocketServer server);

/**
 * @brief   获取实际监听地址
 * @param   buf  接收缓冲区, 至少 32 字节
 * @param   size 缓冲区大小
 * @param   port 存储端口, 可为 NULL
 * @note    端口传 0 时用这个取回系统分配的端口
 */
UtilSocketError socket_server_address(UtilSocketServer server, char* buf, int size, uint16_t* port);


#ifdef __cplusplus
}
#endif

#endif /* _UTILS_SOCKET_H___ */