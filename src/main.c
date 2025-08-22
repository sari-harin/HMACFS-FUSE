#include "common.h"
#include "config.h"
#include "fs_operations.h"
#include "utils.h"

static struct {
    hmacfs_config_t cfg;
} g_state;

// FUSE 옵션 파싱: -o root=/path, --root=/path, -d(debug)
enum {
    KEY_HELP,
    KEY_VERSION,
    KEY_ROOT,
};

static struct fuse_opt hmacfs_opts[] = {
    // -o root=/backend/path
    { "root=%s", offsetof(hmacfs_config_t, root), 0 },
    FUSE_OPT_END
};

static int hmacfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs) {
    (void)outargs;
    hmacfs_config_t *cfg = (hmacfs_config_t*)data;

    switch (key) {
        case FUSE_OPT_KEY_NONOPT:
            // 비옵션 인자는 무시 (마운트 포인트는 FUSE가 처리)
            return 1;

        case FUSE_OPT_KEY_OPT:
            // --root=/path 형태를 수동 파싱
            if (strncmp(arg, "--root=", 7) == 0) {
                snprintf(cfg->root, sizeof(cfg->root), "%s", arg + 7);
                return 0;
            }
            return 1;

        default:
            return 1;
    }
}

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s <mountpoint> -o root=<backend_dir> [FUSE options]\n"
        "   or: %s <mountpoint> --root=<backend_dir> [FUSE options]\n"
        "Examples:\n"
        "  %s mnt -o root=/realdata -f -d\n",
        prog, prog, prog);
}

int main(int argc, char *argv[]) {
    memset(&g_state, 0, sizeof(g_state));

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_operations ops;
    hmacfs_fill_operations(&ops);

    // 기본값: 현재 디렉터리를 root로
    if (getcwd(g_state.cfg.root, sizeof(g_state.cfg.root)) == NULL) {
        perror("getcwd");
        return 1;
    }

    if (fuse_opt_parse(&args, &g_state.cfg, hmacfs_opts, hmacfs_opt_proc) == -1) {
        print_usage(argv[0]);
        return 1;
    }

    if (g_state.cfg.root[0] == '\0') {
        print_usage(argv[0]);
        return 1;
    }

    // 백엔드 유효성 체크
    struct stat st;
    if (stat(g_state.cfg.root, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Invalid backend root: %s\n", g_state.cfg.root);
        return 1;
    }

    hlog_info("Starting hmacfs with backend root: %s", g_state.cfg.root);

    int ret = fuse_main(args.argc, args.argv, &ops, &g_state.cfg);
    fuse_opt_free_args(&args);
    return ret;
}