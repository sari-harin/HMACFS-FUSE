#ifndef HMACFS_CONFIG_H
#define HMACFS_CONFIG_H

#include "common.h"

// 런타임 설정 (FUSE private_data로 전달)
typedef struct hmacfs_config {
    char root[PATH_MAX]; // 백엔드 실제 디렉토리
    int debug;           // 디버그 로깅
} hmacfs_config_t;

#endif // HMACFS_CONFIG_H