/*
 * Copyright (c) 2015-2020 ACOINFO Co., Ltd.
 * All rights reserved.
 *
 * Detailed license information can be found in the LICENSE file.
 *
 * File: ms_edgefs.h reliance edge file system implement.
 *
 * Author: Jiao.jinxing <jiaojinxing@acoinfo.com>
 *
 */

#define __MS_IO
#include "ms_kern.h"
#include "ms_io_core.h"
#include <string.h>
#include <stdio.h>

#include <redfs.h>
#include <redposix.h>
#include <redpath.h>
#include <redvolume.h>

#include <ms_edgefs.h>

/**
 * @brief edgefs file system.
 */

typedef struct {
    const char     *volume;                                 /* reliance edge file system volume label   */
    uint8_t         vol_num;                                /* reliance edge file system volume number  */
} ms_edgefs_t;

int ms_edgefs_err_to_errno(int err)
{
    switch (err) {
    case 0:
        err = 0;
        break;

    case RED_EPERM:         /** Operation not permitted. */
        err = EPERM;
        break;

    case RED_ENOENT:        /** No such file or directory. */
        err = ENOENT;
        break;

    case RED_EIO:           /** I/O error. */
        err = EIO;
        break;

    case RED_EBADF:         /** Bad file number. */
        err = EBADF;
        break;

    case RED_ENOMEM:        /** Out of memory */
        err = ENOMEM;
        break;

    case RED_EBUSY:         /** Device or resource busy. */
        err = EBUSY;
        break;

    case RED_EEXIST:        /** File exists. */
        err = EEXIST;
        break;

    case RED_EXDEV:         /** Cross-device link. */
        err = EXDEV;
        break;

    case RED_ENOTDIR:       /** Not a directory. */
        err = ENOTDIR;
        break;

    case RED_EISDIR:        /** Is a directory. */
        err = EISDIR;
        break;

    case RED_EINVAL:        /** Invalid argument. */
        err = EINVAL;
        break;

    case RED_ENFILE:        /** File table overflow. */
        err = ENFILE;
        break;

    case RED_EMFILE:        /** Too many open files. */
        err = EMFILE;
        break;

    case RED_EFBIG:         /** File too large. */
        err = EFBIG;
        break;

    case RED_ENOSPC:        /** No space left on device. */
        err = ENOSPC;
        break;

    case RED_EROFS:         /** Read-only file system. */
        err = EROFS;
        break;

    case RED_EMLINK:        /** Too many links. */
        err = EMLINK;
        break;

    case RED_ERANGE:        /** Math result not representable. */
        err = ERANGE;
        break;

    case RED_ENAMETOOLONG:  /** File name too long. */
        err = ENAMETOOLONG;
        break;

    case RED_ENOSYS:        /** Function not implemented. */
        err = ENOSYS;
        break;

    case RED_ENOTEMPTY:     /** Directory not empty. */
        err = ENOTEMPTY;
        break;

    case RED_ENODATA:       /** No data available. */
        err = ENODATA;
        break;

    case RED_EUSERS:        /** Too many users. */
        err = EUSERS;
        break;

    case RED_ENOTSUPP:      /** Operation is not supported. */
        err = ENOTSUP;
        break;

    default:
        err = EFAULT;
        break;
    }

    return err;
}

static int __ms_oflag_to_edgefs_oflag(int oflag)
{
    int ret = 0;

    switch (oflag & O_ACCMODE) {
    case O_RDONLY:
        ret |= RED_O_RDONLY;
        break;

    case O_WRONLY:
        ret |= RED_O_WRONLY;
        break;

    case O_RDWR:
        ret |= RED_O_RDWR;
        break;
    }

    if (oflag & O_APPEND) {
        ret |= RED_O_APPEND;
    }

    if (oflag & O_TRUNC) {
        ret |= RED_O_TRUNC;
    }

    if (oflag & O_EXCL) {
        ret |= RED_O_EXCL;
    }

    if (oflag & O_CREAT) {
        ret |= RED_O_CREAT;
    }

    return ret;
}

static int __ms_whence_to_edgefs_whence(int whence)
{
    switch (whence) {
    case SEEK_SET:
        whence = RED_SEEK_SET;
        break;

    case SEEK_CUR:
        whence = RED_SEEK_CUR;
        break;

    case SEEK_END:
        whence = RED_SEEK_END;
        break;

    default:
        whence = -1;
        break;
    }

    return whence;
}

static ms_mode_t __ms_edgefs_file_mode_to_mode(ms_uint16_t mode)
{
    return RED_S_ISREG(mode) ? S_IFREG : S_IFDIR;
}

static ms_uint8_t __ms_edgefs_file_mode_to_type(ms_uint16_t mode)
{
    return RED_S_ISREG(mode) ? DT_REG : DT_DIR;
}

static int __ms_edgefs_mount(ms_io_mnt_t *mnt, ms_io_device_t *dev, const char *dev_name, ms_const_ptr_t param)
{
    int ret;

    if ((dev != MS_NULL) && (param != MS_NULL)) {
        ms_edgefs_t *edgefs = ms_kzalloc(sizeof(ms_edgefs_t));
        if (edgefs != MS_NULL) {
            uint8_t vol_num;

            /*
             * Get os block device spec info
             */
            const ms_edgefs_mount_param_t *mparam = (const ms_edgefs_mount_param_t *)param;

            /*
             * Get volume number
             */
            (void)RedPathVolumeLookup(mparam->path_prefix, &vol_num);

            /*
             * Record edgefs volume label
             */
            edgefs->volume  = mparam->path_prefix;
            edgefs->vol_num = vol_num;

            /*
             * Init global volume configuration
             */
            gaRedVolConf[vol_num].ulSectorSize       = mparam->sector_size;
            gaRedVolConf[vol_num].ullSectorCount     = mparam->sector_count;
            gaRedVolConf[vol_num].ullSectorOffset    = mparam->sector_offset;
            gaRedVolConf[vol_num].fAtomicSectorWrite = mparam->atomic_sector_write;
            gaRedVolConf[vol_num].ulInodeCount       = mparam->inode_count;
            gaRedVolConf[vol_num].bBlockIoRetries    = mparam->block_io_retries;
            gaRedVolConf[vol_num].pszPathPrefix      = mparam->path_prefix;
            gaRedVolConf[vol_num].dev                = dev;

            /*
             * Try to mount file system
             */
            ret = red_mount(mparam->path_prefix);
            if (ret < 0) {
                gaRedVolConf[vol_num].dev = dev;

                ret = red_format(mparam->path_prefix);
                if (ret == 0) {
                    gaRedVolConf[vol_num].dev = dev;

                    ret = red_mount(mparam->path_prefix);
                }
            }

            if (ret == 0) {
                mnt->ctx = edgefs;
            } else {
                (void)ms_kfree(edgefs);
            }

        } else {
            ms_thread_set_errno(ENOMEM);
            ret = -1;
        }

    } else {
        ms_thread_set_errno(EFAULT);
        ret = -1;
    }

    return ret;
}

static int __ms_edgefs_mkfs(ms_io_mnt_t *mnt, ms_const_ptr_t param)
{
    ms_io_device_t *dev = mnt->dev;
    ms_edgefs_t *edgefs = mnt->ctx;
    ms_uint8_t vol_num = edgefs->vol_num;
    int ret;

    RedOsMutexAcquire();

    ret = red_umount(edgefs->volume);
    if (ret == 0) {
        gaRedVolConf[vol_num].dev = dev;
        ret = red_format(edgefs->volume);

        gaRedVolConf[vol_num].dev = dev;
        (void)red_mount(edgefs->volume);
    } else {
        gaRedVolConf[vol_num].dev = dev;
    }

    RedOsMutexRelease();

    return ret;
}

static int __ms_edgefs_unmount(ms_io_mnt_t *mnt, ms_const_ptr_t param)
{
    ms_edgefs_t *edgefs = mnt->ctx;
    int ret;

    ret = red_umount(edgefs->volume);
    if ((ret == 0) || mnt->umount_req) {
        mnt->ctx = MS_NULL;
        (void)ms_kfree(edgefs);
    }

    return ret;
}

static int __ms_edgefs_open(ms_io_mnt_t *mnt, ms_io_file_t *file, const char *path, int oflag, ms_mode_t mode)
{
    int ret;

    (void)mode;

    oflag = __ms_oflag_to_edgefs_oflag(oflag);

    path = ms_io_path_add_mnt(path);

    ret = red_open(path, oflag);
    if (ret >= 0) {
        file->ctx = (ms_ptr_t)ret;
        ret = 0;
    }

    return ret;
}

static int __ms_edgefs_close(ms_io_mnt_t *mnt, ms_io_file_t *file)
{
    int ret;

    ret = red_close((int)file->ctx);
    if ((ret == 0) || mnt->umount_req) {
        file->ctx = MS_NULL;
    }

    return ret;
}

static ms_ssize_t __ms_edgefs_read(ms_io_mnt_t *mnt, ms_io_file_t *file, ms_ptr_t buf, ms_size_t len)
{
    return (ms_ssize_t)red_read((int)file->ctx, buf, len);
}

static ms_ssize_t __ms_edgefs_write(ms_io_mnt_t *mnt, ms_io_file_t *file, ms_const_ptr_t buf, ms_size_t len)
{
    return (ms_ssize_t)red_write((int)file->ctx, buf, len);
}

static int __ms_edgefs_fcntl(ms_io_mnt_t *mnt, ms_io_file_t *file, int cmd, int arg)
{
    int ret;

    switch (cmd) {
    case F_GETFL:
        ret = file->flags;
        break;

    case F_SETFL:
        if ((!(file->flags & FWRITE)) && (arg & FWRITE)) {
            ms_thread_set_errno(EACCES);
            ret = -1;
        } else {
            file->flags = arg;
            ret = 0;
        }
        break;

    default:
        ms_thread_set_errno(EINVAL);
        ret = -1;
        break;
    }

    return ret;
}

static int __ms_edgefs_fstat(ms_io_mnt_t *mnt, ms_io_file_t *file, ms_stat_t *buf)
{
    int ret;
    REDSTAT red_stat;

    bzero(buf, sizeof(ms_stat_t));

    ret = red_fstat((int)file->ctx, &red_stat);
    if (ret == 0) {
        buf->st_mode = S_IRWXU | S_IRWXG | S_IRWXO | S_IFREG;
        buf->st_size = red_stat.st_size;
    }

    return ret;
}

static int __ms_edgefs_isatty(ms_io_mnt_t *mnt, ms_io_file_t *file)
{
    return 0;
}

static int __ms_edgefs_fsync(ms_io_mnt_t *mnt, ms_io_file_t *file)
{
    return red_fsync((int)file->ctx);
}

static int __ms_edgefs_ftruncate(ms_io_mnt_t *mnt, ms_io_file_t *file, ms_off_t len)
{
    return red_ftruncate((int)file->ctx, len);
}

static ms_off_t __ms_edgefs_lseek(ms_io_mnt_t *mnt, ms_io_file_t *file, ms_off_t offset, int whence)
{
    whence = __ms_whence_to_edgefs_whence(whence);

    return red_lseek((int)file->ctx, offset, whence);
}

static int __ms_edgefs_stat(ms_io_mnt_t *mnt, const char *path, ms_stat_t *buf)
{
    ms_edgefs_t *edgefs = mnt->ctx;
    int ret;
    int red_fd;
    REDSTAT red_stat;

    bzero(buf, sizeof(ms_stat_t));

    if (MS_IO_PATH_IS_ROOT(path)) {
        path = edgefs->volume;
    } else {
        path = ms_io_path_add_mnt(path);
    }

    red_fd = red_open(path, RED_O_RDONLY);
    if (red_fd >= 0) {
        ret = red_fstat(red_fd, &red_stat);
        (void)red_close(red_fd);

        if (ret == 0) {
            buf->st_mode  = S_IRWXU | S_IRWXG | S_IRWXO;
            buf->st_mode |= __ms_edgefs_file_mode_to_mode(red_stat.st_mode);
            buf->st_size  = RED_S_ISREG(red_stat.st_mode) ? red_stat.st_size : 0;
        }
    } else {
        ret = -1;
    }

    return ret;
}

static int __ms_edgefs_statvfs(ms_io_mnt_t *mnt, ms_statvfs_t *buf)
{
    ms_edgefs_t *edgefs = mnt->ctx;
    REDSTATFS red_statfs;
    int ret;

    ret = red_statvfs(edgefs->volume, &red_statfs);
    if (ret == 0) {
        buf->f_bsize  = red_statfs.f_bsize;
        buf->f_frsize = red_statfs.f_frsize;
        buf->f_blocks = red_statfs.f_blocks;
        buf->f_bfree  = red_statfs.f_bfree;
        buf->f_files  = red_statfs.f_files;
        buf->f_ffree  = red_statfs.f_ffree;
        buf->f_dev    = mnt->dev->nnode.name;
        buf->f_mnt    = mnt->nnode.name;
        buf->f_fsname = MS_EDGEFS_NAME;

    } else {
        bzero(buf, sizeof(ms_statvfs_t));
    }

    return ret;
}

static int __ms_edgefs_unlink(ms_io_mnt_t *mnt, const char *path)
{
    path = ms_io_path_add_mnt(path);
    return red_unlink(path);
}

static int __ms_edgefs_mkdir(ms_io_mnt_t *mnt, const char *path, ms_mode_t mode)
{
    (void)mode;

    path = ms_io_path_add_mnt(path);
    return red_mkdir(path);
}

static int __ms_edgefs_rmdir(ms_io_mnt_t *mnt, const char *path)
{
    path = ms_io_path_add_mnt(path);
    return red_rmdir(path);
}

static int __ms_edgefs_rename(ms_io_mnt_t *mnt, const char *old, const char *_new)
{
    old = ms_io_path_add_mnt(old);
    _new = ms_io_path_add_mnt(_new);
    return red_rename(old, _new);
}

static int __ms_edgefs_sync(ms_io_mnt_t *mnt)
{
    return red_sync();
}

static int __ms_edgefs_truncate(ms_io_mnt_t *mnt, const char *path, ms_off_t len)
{
    int oflag = __ms_oflag_to_edgefs_oflag(O_WRONLY);
    int red_fd;
    int ret;

    path = ms_io_path_add_mnt(path);
    red_fd = red_open(path, oflag);
    if (red_fd >= 0) {
        ret = red_ftruncate(red_fd, len);
        (void)red_close(red_fd);
    } else {
        ret = -1;
    }

    return ret;
}

static int __ms_edgefs_opendir(ms_io_mnt_t *mnt, ms_io_file_t *file, const char *path)
{
    ms_edgefs_t *edgefs = mnt->ctx;
    int ret;
    REDDIR *red_dir;

    if (MS_IO_PATH_IS_ROOT(path)) {
        path = edgefs->volume;
    } else {
        path = ms_io_path_add_mnt(path);
    }

    red_dir = red_opendir(path);
    if (red_dir != MS_NULL) {
        file->ctx = red_dir;
        ret = 0;
    } else {
        ret = -1;
    }

    return ret;
}

static int __ms_edgefs_closedir(ms_io_mnt_t *mnt, ms_io_file_t *file)
{
    int ret;

    ret = red_closedir(file->ctx);
    if ((ret == 0) || mnt->umount_req) {
        file->ctx = MS_NULL;
    }

    return ret;
}

static int __ms_edgefs_readdir_r(ms_io_mnt_t *mnt, ms_io_file_t *file, ms_dirent_t *entry, ms_dirent_t **result)
{
    int errno_bak = ms_thread_get_errno();
    REDDIRENT *red_ent;
    int ret;

    red_ent = red_readdir(file->ctx);
    if (red_ent != MS_NULL) {
        strlcpy(entry->d_name, red_ent->d_name, sizeof(entry->d_name));
        entry->d_type = __ms_edgefs_file_mode_to_type(red_ent->d_stat.st_mode);

        if (result != MS_NULL) {
            *result = entry;
        }

        ret = 1;

    } else {
        if (result != MS_NULL) {
            *result = MS_NULL;
        }

        if (errno_bak == ms_thread_get_errno()) {
            ret = 0;
        } else {
            ret = -1;
        }
    }

    return ret;
}

static int __ms_edgefs_rewinddir(ms_io_mnt_t *mnt, ms_io_file_t *file)
{
    red_rewinddir(file->ctx);

    return 0;
}

static ms_io_fs_ops_t ms_io_edgefs_ops = {
        .type       = MS_IO_FS_TYPE_DISKFS,
        .mount      = __ms_edgefs_mount,
        .unmount    = __ms_edgefs_unmount,
        .mkfs       = __ms_edgefs_mkfs,

        .link       = MS_NULL,
        .unlink     = __ms_edgefs_unlink,
        .mkdir      = __ms_edgefs_mkdir,
        .rmdir      = __ms_edgefs_rmdir,
        .rename     = __ms_edgefs_rename,
        .sync       = __ms_edgefs_sync,
        .truncate   = __ms_edgefs_truncate,
        .stat       = __ms_edgefs_stat,
        .lstat      = __ms_edgefs_stat,
        .statvfs    = __ms_edgefs_statvfs,

        .open       = __ms_edgefs_open,
        .close      = __ms_edgefs_close,
        .read       = __ms_edgefs_read,
        .write      = __ms_edgefs_write,
        .ioctl      = MS_NULL,
        .fcntl      = __ms_edgefs_fcntl,
        .fstat      = __ms_edgefs_fstat,
        .isatty     = __ms_edgefs_isatty,
        .fsync      = __ms_edgefs_fsync,
        .fdatasync  = __ms_edgefs_fsync,
        .ftruncate  = __ms_edgefs_ftruncate,
        .lseek      = __ms_edgefs_lseek,
        .poll       = MS_NULL,

        .opendir    = __ms_edgefs_opendir,
        .closedir   = __ms_edgefs_closedir,
        .readdir_r  = __ms_edgefs_readdir_r,
        .rewinddir  = __ms_edgefs_rewinddir,
        .seekdir    = MS_NULL,
        .telldir    = MS_NULL,
};

static ms_io_fs_t ms_io_edgefs = {
        .nnode = {
            .name = MS_EDGEFS_NAME,
        },
        .ops = &ms_io_edgefs_ops,
};

ms_err_t ms_edgefs_register(void)
{
    ms_err_t err;

    if (red_init() == 0) {
        err = ms_io_fs_register(&ms_io_edgefs);

    } else {
        err = MS_ERR;
    }

    return err;
}
