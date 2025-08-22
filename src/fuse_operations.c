#include "common.h"
#include "utils.h"
#include "config.h"
#include "fs_operations.h"

// 파일 핸들에 fd 저장/해제 편의
static inline void set_fh(struct fuse_file_info *fi, int fd) {
    fi->fh = (uint64_t)fd;
}
static inline int get_fh(const struct fuse_file_info *fi) {
    return (int)fi->fh;
}

static void* hmacfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    (void)conn;
    // kernel caching 정책: 여기선 기본값을 사용
    cfg->kernel_cache = 0;
    cfg->auto_cache = 1;
    cfg->attr_timeout = 0.0;
    cfg->entry_timeout = 0.0;

    hmacfs_config_t *c = hmacfs_get_config();
    hlog_info("init: backend root = %s", c ? c->root : "(null)");
    return c;
}

static void hmacfs_destroy(void *private_data) {
    (void)private_data;
    hlog_info("destroy");
}

static int hmacfs_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    (void)fi;
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;
    hlog_debug("getattr %s -> %s", path, fpath);
    if (lstat(fpath, st) == -1) return -errno;
    return 0;
}

static int hmacfs_access(const char *path, int mask) {
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;
    hlog_debug("access %s (mask=%d)", fpath, mask);
    if (access(fpath, mask) == -1) return -errno;
    return 0;
}

static int hmacfs_readlink(const char *path, char *buf, size_t size) {
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;
    ssize_t len = readlink(fpath, buf, size - 1);
    if (len == -1) return -errno;
    buf[len] = '\0';
    return 0;
}

static int hmacfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
    (void)offset; (void)fi; (void)flags;
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;

    hlog_debug("readdir %s", fpath);
    DIR *dp = opendir(fpath);
    if (!dp) return -errno;

    // "." ".."
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        struct stat st = {0};
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0, 0))
            break;
    }
    closedir(dp);
    return 0;
}

static int hmacfs_mknod(const char *path, mode_t mode, dev_t rdev) {
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;

    int res;
    if (S_ISREG(mode)) {
        res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (res >= 0) {
            close(res);
            return 0;
        }
    } else if (S_ISFIFO(mode)) {
        res = mkfifo(fpath, mode);
        if (res == -1) return -errno;
        return 0;
    } else {
        res = mknod(fpath, mode, rdev);
        if (res == -1) return -errno;
        return 0;
    }
    return -errno;
}

static int hmacfs_mkdir(const char *path, mode_t mode) {
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;
    if (mkdir(fpath, mode) == -1) return -errno;
    return 0;
}

static int hmacfs_unlink(const char *path) {
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;
    if (unlink(fpath) == -1) return -errno;
    return 0;
}

static int hmacfs_rmdir(const char *path) {
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;
    if (rmdir(fpath) == -1) return -errno;
    return 0;
}

static int hmacfs_symlink(const char *to, const char *from) {
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), from);
    if (r) return r;
    if (symlink(to, fpath) == -1) return -errno;
    return 0;
}

static int hmacfs_rename(const char *from, const char *to, unsigned int flags) {
#ifdef RENAME_NOREPLACE
    (void)flags; // 단순화: flags 무시(심플 구현)
#endif
    char ffrom[PATH_MAX], fto[PATH_MAX];
    int r1 = hmacfs_fullpath(ffrom, sizeof(ffrom), from);
    if (r1) return r1;
    int r2 = hmacfs_fullpath(fto, sizeof(fto), to);
    if (r2) return r2;
    if (rename(ffrom, fto) == -1) return -errno;
    return 0;
}

static int hmacfs_link(const char *from, const char *to) {
    char ffrom[PATH_MAX], fto[PATH_MAX];
    int r1 = hmacfs_fullpath(ffrom, sizeof(ffrom), from);
    if (r1) return r1;
    int r2 = hmacfs_fullpath(fto, sizeof(fto), to);
    if (r2) return r2;
    if (link(ffrom, fto) == -1) return -errno;
    return 0;
}

static int hmacfs_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void)fi;
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;
    if (chmod(fpath, mode) == -1) return -errno;
    return 0;
}

static int hmacfs_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {
    (void)fi;
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;
    if (lchown(fpath, uid, gid) == -1) return -errno;
    return 0;
}

static int hmacfs_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
    int res;
    if (fi) {
        res = ftruncate(get_fh(fi), size);
    } else {
        char fpath[PATH_MAX];
        int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
        if (r) return r;
        res = truncate(fpath, size);
    }
    if (res == -1) return -errno;
    return 0;
}

static int hmacfs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
    (void)fi;
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;
#ifdef HAVE_UTIMENSAT
    if (utimensat(AT_FDCWD, fpath, tv, AT_SYMLINK_NOFOLLOW) == -1) return -errno;
#else
    struct timeval times[2];
    times[0].tv_sec = tv[0].tv_sec;
    times[0].tv_usec = tv[0].tv_nsec / 1000;
    times[1].tv_sec = tv[1].tv_sec;
    times[1].tv_usec = tv[1].tv_nsec / 1000;
    if (lutimes(fpath, times) == -1) return -errno;
#endif
    return 0;
}

static int hmacfs_open(const char *path, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;
    int fd = open(fpath, fi->flags);
    if (fd == -1) return -errno;
    set_fh(fi, fd);
    return 0;
}

static int hmacfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;
    int fd = open(fpath, fi->flags | O_CREAT, mode);
    if (fd == -1) return -errno;
    set_fh(fi, fd);
    return 0;
}

static int hmacfs_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    (void)path;
    int fd = get_fh(fi);
    ssize_t res = pread(fd, buf, size, offset);
    if (res == -1) return -errno;
    return (int)res;
}

static int hmacfs_write(const char *path, const char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    (void)path;
    int fd = get_fh(fi);
    ssize_t res = pwrite(fd, buf, size, offset);
    if (res == -1) return -errno;
    return (int)res;
}

static int hmacfs_statfs(const char *path, struct statvfs *st) {
    char fpath[PATH_MAX];
    int r = hmacfs_fullpath(fpath, sizeof(fpath), path);
    if (r) return r;
    if (statvfs(fpath, st) == -1) return -errno;
    return 0;
}

static int hmacfs_release(const char *path, struct fuse_file_info *fi) {
    (void)path;
    int fd = get_fh(fi);
    if (close(fd) == -1) return -errno;
    return 0;
}

int hmacfs_fill_operations(struct fuse_operations *ops) {
    memset(ops, 0, sizeof(*ops));
    ops->init       = hmacfs_init;
    ops->destroy    = hmacfs_destroy;
    ops->getattr    = hmacfs_getattr;
    ops->access     = hmacfs_access;
    ops->readlink   = hmacfs_readlink;
    ops->readdir    = hmacfs_readdir;
    ops->mknod      = hmacfs_mknod;
    ops->mkdir      = hmacfs_mkdir;
    ops->symlink    = hmacfs_symlink;
    ops->unlink     = hmacfs_unlink;
    ops->rmdir      = hmacfs_rmdir;
    ops->rename     = hmacfs_rename;
    ops->link       = hmacfs_link;
    ops->chmod      = hmacfs_chmod;
    ops->chown      = hmacfs_chown;
    ops->truncate   = hmacfs_truncate;
    ops->open       = hmacfs_open;
    ops->create     = hmacfs_create;
    ops->read       = hmacfs_read;
    ops->write      = hmacfs_write;
    ops->statfs     = hmacfs_statfs;
    ops->release    = hmacfs_release;
    ops->utimens    = hmacfs_utimens;
    return 0;
}