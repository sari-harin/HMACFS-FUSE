#ifndef HMACFS_COMMON_H
#define HMACFS_COMMON_H

#define FUSE_USE_VERSION 35
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define _FILE_OFFSET_BITS 64

#include <fuse3/fuse.h>
#include <fuse3/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <limits.h>
#include <libgen.h>
#include <stdbool.h>
#include <time.h>

#define HMACFS_OK 0

// 간단한 에러 체크 헬퍼
#define CHECK_RET(expr) do { if ((expr) == -1) return -errno; } while(0)

// 로깅 헬퍼
static inline void hlog_debug(const char *fmt, ...) {
#ifdef NDEBUG
    (void)fmt;
#else
    va_list ap;
    va_start(ap, fmt);
    fuse_log(FUSE_LOG_DEBUG, "[hmacfs] ");
    fuse_logv(FUSE_LOG_DEBUG, fmt, ap);
    fuse_log(FUSE_LOG_DEBUG, "\n");
    va_end(ap);
#endif
}

static inline void hlog_info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fuse_log(FUSE_LOG_INFO, "[hmacfs] ");
    fuse_logv(FUSE_LOG_INFO, fmt, ap);
    fuse_log(FUSE_LOG_INFO, "\n");
    va_end(ap);
}

static inline void hlog_err(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fuse_log(FUSE_LOG_ERR, "[hmacfs] ");
    fuse_logv(FUSE_LOG_ERR, fmt, ap);
    fuse_log(FUSE_LOG_ERR, "\n");
    va_end(ap);
}

#endif // HMACFS_COMMON_H