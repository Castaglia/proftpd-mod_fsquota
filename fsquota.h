/*
 * ProFTPD - mod_fsquota API
 * Copyright (c) 2013 TJ Saunders
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 * As a special exemption, TJ Saunders and other respective copyright holders
 * give permission to link this program with OpenSSL, and distribute the
 * resulting executable, without including the source code for OpenSSL in the
 * source distribution.
 */

#include "mod_fsquota.h"

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_QUOTA_H
# include <sys/quota.h>
#endif

#ifdef HAVE_SYS_FS_UFS_QUOTA_H
# include <sys/fs/ufs_quota.h>
#endif

#ifdef HAVE_UFS_UFS_QUOTA_H
# include <ufs/ufs/quota.h>
#endif

#ifdef HAVE_XFS_XQM_H
# include <xfs/xqm.h>
#endif

#ifndef MOD_FSQUOTA_FSQUOTA_H
#define MOD_FSQUOTA_FSQUOTA_H

int fsquota_group_enabled(const char *path, gid_t gid, int *enabled);

int fsquota_group_get(const char *path, gid_t gid, uint64_t *kb_total,
  uint64_t *kb_used, uint64_t *file_total, uint64_t *file_used);

int fsquota_user_enabled(const char *path, uid_t uid, int *enabled);

int fsquota_user_get(const char *path, uid_t uid, uint64_t *kb_total,
  uint64_t *kb_used, uint64_t *file_total, uint64_t *file_used);

#endif /* MOD_FSQUOTA_FSQUOTA_H */
