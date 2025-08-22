#ifndef HMACFS_COMMON_H
#define HMACFS_COMMON_H

#define FUSE_USE_VERSION 35
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define _FILE_OFFSET_BITS 64

#include <fuse3/fuse.h>

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

// 간단한 로깅 매크로들 (표준 출력 사용)
static inline void hlog_debug(const char *fmt, ...) {
#ifndef NDEBUG
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[DEBUG] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
#endif
}

static inline void hlog_info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[INFO] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

static inline void hlog_err(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

#endif // HMACFS_COMMON_H