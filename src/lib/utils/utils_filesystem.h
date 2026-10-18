#ifndef _UTILS_FILESYSTEM_H__
#define _UTILS_FILESYSTEM_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 兼容 hal_file.h 的类型定义 */
typedef FILE*                    UtilFile;
typedef struct UtilDirImpl* UtilDir;

/* 文件操作接口 */
UtilFile file_open(const char* pathName, bool readWrite);
size_t    file_read(UtilFile handle, uint8_t* buffer, size_t maxSize);
size_t    file_write(UtilFile handle, uint8_t* buffer, size_t size);
int       file_flush(UtilFile handle);
void      file_close(UtilFile handle);
bool      file_info(const char* filename, uint32_t* fileSize, uint64_t* lastModificationTimestamp);
bool      file_delete(const char* filename);
bool      file_rename(const char* oldFilename, const char* newFilename);

/* 目录操作接口 - 兼容 hal_file.h */
UtilDir file_open_dir(const char* directoryName);

/**
 * @brief Read directory entry.
 * @param directory The directory handle
 * @param isDirectory Pointer to store whether the entry is a directory. May be NULL.
 * @return Entry name, or NULL when enumeration finishes. "." and ".." are skipped.
 * @note  The returned pointer refers to the handle's internal buffer and is
 *        overwritten by the next call. Copy the string out if you need to keep it.
 */
char* file_read_dir(UtilDir directory, bool* isDirectory);

void file_close_dir(UtilDir directory);

/* 扩展的文件操作接口 */
const char* file_content(const char* path);
bool        path_exist(const char* path);
bool        file_exist(const char* path);
int         file_size(const char* path);
int         file_size_fd(int fd);
int         file_size_fp(FILE* fp);

/**
 * @brief Create a multi-level directory (like "mkdir -p").
 * @param dir The directory path
 * @retval 0  Success, including the case where the directory already exists
 * @retval -1 Parameter is NULL, out of memory, or mkdir failed
 * @note  Idempotent: existing intermediate levels are not treated as errors.
 */
int mkdir_m(const char* dir);

/* 高级文件操作接口 */
bool util_file_copy(const char* src, const char* dst);
bool util_file_move(const char* src, const char* dst);

/**
 * @brief Join multiple path components (variadic).
 * @param first The first path component. Followed by additional `const char*`
 * arguments and terminated by a NULL pointer.
 * @return Joined path (caller should free) or NULL on error.
 */
char* util_path_join(const char* first, ...);

char* util_path_dirname(const char* path);   /* caller should free the returned pointer */
char* util_path_basename(const char* path);  /* caller should free the returned pointer */
char* util_path_extension(const char* path); /* caller should free the returned pointer */
char* util_path_normalize(const char* path); /* caller should free the returned pointer */
bool  util_path_is_absolute(const char* path);

#ifdef __cplusplus
}
#endif

#endif /* _UTILS_FILESYSTEM_H__ */
