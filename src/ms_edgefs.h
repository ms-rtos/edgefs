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

#ifndef MS_EDGEFS_H
#define MS_EDGEFS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ms_uint32_t    sector_size;
    ms_uint64_t    sector_count;
    ms_uint64_t    sector_offset;
    ms_bool_t      atomic_sector_write;
    ms_uint32_t    inode_count;
    ms_uint8_t     block_io_retries;
    const char    *path_prefix;
} ms_edgefs_mount_param_t;

#define MS_EDGEFS_NAME      "edgefs"

ms_err_t ms_edgefs_register(void);

#ifdef __cplusplus
}
#endif

#endif /* MS_EDGEFS_H */
