#ifndef HMACFS_UTILS_H
#define HMACFS_UTILS_H

#include "common.h"
#include "config.h"

// FUSE 컨텍스트에서 설정 가져오기
static inline hmacfs_config_t* hmacfs_get_config(void) {
    struct fuse_context *ctx = fuse_get_context();
    return (hmacfs_config_t*)ctx->private_data;
}

// FUSE 경로를 백엔드 실제 경로로 변환
// out_size는 PATH_MAX 이상 권장
static inline int hmacfs_fullpath(char *out, size_t out_size, const char *path) {
    hmacfs_config_t *cfg = hmacfs_get_config();
    if (!cfg || cfg->root[0] == '\0') return -EINVAL;

    // path는 항상 "/"로 시작 (FUSE 관례)
    if (snprintf(out, out_size, "%s%s", cfg->root, path) >= (int)out_size) {
        return -ENAMETOOLONG;
    }
    return 0;
}

#endif // HMACFS_UTILS_H