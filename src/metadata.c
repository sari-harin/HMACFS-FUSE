// metadata.c
// 예시 코드
#include "metadata.h"
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>

#define META_ROOT "meta"

// data/foo/bar.txt -> meta/foo/bar.txt.hmac
void get_meta_path(const char *data_path, char *meta_path, size_t size) {
    snprintf(meta_path, size, "%s/%s.hmac", META_ROOT, data_path + strlen("data/"));
}

// HMAC 저장
int save_hmac(const char *data_path, const unsigned char *hmac, size_t hmac_len) {
    char meta_path[PATH_MAX];
    get_meta_path(data_path, meta_path, sizeof(meta_path));

    // 디렉토리 구조 보장
    char tmp[PATH_MAX];
    strcpy(tmp, meta_path);
    char *dir = dirname(tmp);
    mkdir(dir, 0755); // 중첩 디렉토리 재귀 생성 로직 필요

    FILE *f = fopen(meta_path, "wb");
    if (!f) return -1;
    fwrite(hmac, 1, hmac_len, f);
    fclose(f);
    return 0;
}

// HMAC 불러오기
int load_hmac(const char *data_path, unsigned char *hmac, size_t hmac_len) {
    char meta_path[PATH_MAX];
    get_meta_path(data_path, meta_path, sizeof(meta_path));

    FILE *f = fopen(meta_path, "rb");
    if (!f) return -1;
    fread(hmac, 1, hmac_len, f);
    fclose(f);
    return 0;
}