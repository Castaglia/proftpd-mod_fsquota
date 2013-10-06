/*
 * ProFTPD - mod_fsquota implementations
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
#include "fsquota.h"

static const char *trace_channel = "fsquota";

#if defined(LINUX)
static int linux_user_enabled(const char *path, uid_t uid, int *enabled) {
  int res = -1;

# ifdef Q_QUOTASTAT
  res = quotactl(QCMD(Q_QUOTASTAT, USRQUOTA), path, uid, enabled);
  if (res < 0) {
    int xerrno = errno;

    pr_trace_msg(trace_channel, 9,
      "Linux: error checking user quota status for UID %lu, path '%s': %s",
      (unsigned long) uid, path, strerror(xerrno));

    errno = xerrno;
  }
# else
  errno = ENOSYS; 
# endif /* Q_QUOTASTAT */

  return res;
}

static int linux_user_get(const char *path, uid_t uid, uint64_t *kb_total,
    uint64_t *kb_used, uint64_t *file_total, uint64_t *file_used) {
  int res;
  struct stat st;
  struct dqblk dq;

  /* We need to stat the directory, in order to find out the block size. */
  res = stat(path, &st);
  if (res < 0) {
    int xerrno = errno;

    pr_trace_channel(trace_channel, 7,
      "stat(2) error on '%s': %s", path, strerror(xerrno));

    errno = xerrno;
    return -1;
  }

  res = quotactl(QCMD(Q_GETQUOTA, USRQUOTA), path, uid, &dq);
  if (res == 0) {
    if (dq.dqb_valid & QIF_BLIMITS) {
      if (kb_total != NULL) {
        *kb_total = ((dq.dqb_bsoftlimit * st.st_blksize) / 1024);
      }

      if (kb_used != NULL) {
        *kb_used = ((dq.dqb_curspace * st.st_blksize) / 1024);
      }
    }

    if (dq.dqb_valid & QIF_INODES) {
      if (file_total != NULL) {
        *file_total = (uint64_t) dq.dqb_isoftlimit;
      }

      if (file_used != NULL) {
        *file_used = (uint64_t) dq.dqb_curinodes;
      }
    }

  } else {
    int xerrno = xerrno;

    pr_trace_msg(trace_channel, 9,
      "Linux: error obtaining user quotas for UID %lu, path '%s': %s",
      (unsigned long) uid, path, strerror(xerrno));

    errno = xerrno;
  }

  return res;
}

static int linux_group_enabled(const char *path, gid_t gid, int *enabled) {
  int res = -1;

# ifdef Q_QUOTASTAT
  res = quotactl(QCMD(Q_QUOTASTAT, GRPQUOTA), path, gid, enabled);
  if (res < 0) {
    int xerrno = errno;

    pr_trace_msg(trace_channel, 9,
      "Linux: error checking group quota status for GID %lu, path '%s': %s",
      (unsigned long) gid, path, strerror(xerrno));

    errno = xerrno;
  }
# else
  errno = ENOSYS; 
# endif /* Q_QUOTASTAT */

  return res;
}

static int linux_group_get(const char *path, gid_t gid, uint64_t *kb_total,
    uint64_t *kb_used, uint64_t *file_total, uint64_t *file_used) {
  int res;
  struct stat st;
  struct dqblk dq;

  /* We need to stat the directory, in order to find out the block size. */
  res = stat(path, &st);
  if (res < 0) {
    int xerrno = errno;

    pr_trace_channel(trace_channel, 7,
      "stat(2) error on '%s': %s", path, strerror(xerrno));

    errno = xerrno;
    return -1;
  }

  res = quotactl(QCMD(Q_GETQUOTA, GRPQUOTA), path, gid, &dq);
  if (res == 0) {
    if (dq.dqb_valid & QIF_BLIMITS) {
      if (kb_total != NULL) {
        *kb_total = ((dq.dqb_bsoftlimit * st.st_blksize) / 1024);
      }

      if (kb_used != NULL) {
        *kb_used = ((dq.dqb_curspace * st.st_blksize) / 1024);
      }
    }

    if (dq.dqb_valid & QIF_INODES) {
      if (file_total != NULL) {
        *file_total = (uint64_t) dq.dqb_isoftlimit;
      }

      if (file_used != NULL) {
        *file_used = (uint64_t) dq.dqb_curinodes;
      }
    }

  } else {
    int xerrno = xerrno;

    pr_trace_msg(trace_channel, 9,
      "Linux: error obtaining group quotas for GID %lu, path '%s': %s",
      (unsigned long) gid, path, strerror(xerrno));

    errno = xerrno;
  }

  return res;
}
#endif /* Linux */

#if defined(FREEBSD7) || defined(FREEBSD8) || defined(FREEBSD9) || \
    defined(FREEBSD10)
static int freebsd_user_enabled(const char *path, uid_t uid, int *enabled) {
  errno = ENOSYS;
  return -1;
}

static int freebsd_user_get(const char *path, uid_t uid, uint64_t *kb_total,
    uint64_t *kb_used, uint64_t *file_total, uint64_t *file_used) {
  int res;
  struct stat st;
  struct dqblk dq;

  /* We need to stat the directory, in order to find out the block size. */
  res = stat(path, &st);
  if (res < 0) {
    int xerrno = errno;

    pr_trace_channel(trace_channel, 7,
      "stat(2) error on '%s': %s", path, strerror(xerrno));

    errno = xerrno;
    return -1;
  }

  res = quotactl(path, QCMD(Q_GETQUOTA, USRQUOTA), uid, &dq);
  if (res == 0) {
    if (kb_total != NULL) {
      *kb_total = ((dq.dqb_bsoftlimit * st.st_blksize) / 1024);
    }

    if (kb_used != NULL) {
      *kb_used = ((dq.dqb_curblocks * st.st_blksize) / 1024);
    }

    if (file_total != NULL) {
      *file_total = (uint64_t) dq.dqb_isoftlimit;
    }

    if (file_used != NULL) {
      *file_used = (uint64_t) dq.dqb_curinodes;
    }

  } else {
    int xerrno = xerrno;

    pr_trace_msg(trace_channel, 9,
      "FreeBSD: error obtaining user quotas for UID %lu, path '%s': %s",
      (unsigned long) uid, path, strerror(xerrno));

    errno = xerrno;
  }

  return res;
}

static int freebsd_group_enabled(const char *path, gid_t gid, int *enabled) {
  errno = ENOSYS;
  return -1;
}

static int freebsd_group_get(const char *path, gid_t gid, uint64_t *kb_total,
    uint64_t *kb_used, uint64_t *file_total, uint64_t *file_used) {
  int res;
  struct stat st;
  struct dqblk dq;

  /* We need to stat the directory, in order to find out the block size. */
  res = stat(path, &st);
  if (res < 0) {
    int xerrno = errno;

    pr_trace_channel(trace_channel, 7,
      "stat(2) error on '%s': %s", path, strerror(xerrno));

    errno = xerrno;
    return -1;
  }

  res = quotactl(path, QCMD(Q_GETQUOTA, GRPQUOTA), gid, &dq);
  if (res == 0) {
    if (kb_total != NULL) {
      *kb_total = ((dq.dqb_bsoftlimit * st.st_blksize) / 1024);
    }

    if (kb_used != NULL) {
      *kb_used = ((dq.dqb_curblocks * st.st_blksize) / 1024);
    }

    if (file_total != NULL) {
      *file_total = (uint64_t) dq.dqb_isoftlimit;
    }

    if (file_used != NULL) {
      *file_used = (uint64_t) dq.dqb_curinodes;
    }

  } else {
    int xerrno = xerrno;

    pr_trace_msg(trace_channel, 9,
      "FreeBSD: error obtaining group quotas for GID %lu, path '%s': %s",
      (unsigned long) gid, path, strerror(xerrno));

    errno = xerrno;
  }

  return res;
}
#endif /* FreeBSD */

#if defined(DARWIN9) || defined(DARWIN10) || defined(DARWIN11)
/* MacOSX */
static int darwin_user_enabled(const char *path, uid_t uid, int *enabled) {
  int res = -1;
  char status = 0;

# ifdef Q_QUOTASTAT
  res = quotactl(path, QCMD(Q_QUOTASTAT, USRQUOTA), uid, &status);
  if (res == 0) {
    *enabled = status;

  } else {
    int xerrno = errno;

    pr_trace_msg(trace_channel, 9,
      "MacOSX: error checking user quota status for UID %lu, path '%s': %s",
      (unsigned long) uid, path, strerror(xerrno));

    errno = xerrno;
  }
# else
  errno = ENOSYS;
# endif /* Q_QUOTASTAT */

  return res;
}

static int darwin_user_get(const char *path, uid_t uid, uint64_t *kb_total,
    uint64_t *kb_used, uint64_t *file_total, uint64_t *file_used) {
  int res;
  struct dqblk dq;

  res = quotactl(path, QCMD(Q_GETQUOTA, USRQUOTA), uid, (void *) &dq);
  if (res == 0) {
    if (kb_total != NULL) {
      *kb_total = (dq.dqb_bsoftlimit / 1024);
    }

    if (kb_used != NULL) {
      *kb_used = (dq.dqb_curbytes / 1024);
    }

    if (file_total != NULL) {
      *file_total = (uint64_t) dq.dqb_isoftlimit;
    }

    if (file_used != NULL) {
      *file_used = (uint64_t) dq.dqb_curinodes;
    }

  } else {
    int xerrno = xerrno;

    pr_trace_msg(trace_channel, 9,
      "Linux: error obtaining user quotas for UID %lu, path '%s': %s",
      (unsigned long) uid, path, strerror(xerrno));

    errno = xerrno;
  }

  return res;
}

static int darwin_group_enabled(const char *path, gid_t gid, int *enabled) {
  int res = -1;
  char status = 0;

# ifdef Q_QUOTASTAT
  res = quotactl(path, QCMD(Q_QUOTASTAT, GRPQUOTA), gid, &status);
  if (res == 0) {
    *enabled = status;

  } else {
    int xerrno = errno;

    pr_trace_msg(trace_channel, 9,
      "MacOSX: error checking group quota status for GID %lu, path '%s': %s",
      (unsigned long) gid, path, strerror(xerrno));

    errno = xerrno;
  }
# else
  errno = ENOSYS;
# endif /* Q_QUOTASTAT */

  return res;
}

static int darwin_group_get(const char *path, gid_t gid, uint64_t *kb_total,
    uint64_t *kb_used, uint64_t *file_total, uint64_t *file_used) {
  int res;
  struct dqblk dq;

  res = quotactl(path, QCMD(Q_GETQUOTA, GRPQUOTA), gid, (void *) &dq);
  if (res == 0) {
    if (kb_total != NULL) {
      *kb_total = (dq.dqb_bsoftlimit / 1024);
    }

    if (kb_used != NULL) {
      *kb_used = (dq.dqb_curbytes / 1024);
    }

    if (file_total != NULL) {
      *file_total = (uint64_t) dq.dqb_isoftlimit;
    }

    if (file_used != NULL) {
      *file_used = (uint64_t) dq.dqb_curinodes;
    }

  } else {
    int xerrno = xerrno;

    pr_trace_msg(trace_channel, 9,
      "Linux: error obtaining group quotas for GID %lu, path '%s': %s",
      (unsigned long) gid, path, strerror(xerrno));

    errno = xerrno;
  }

  return res;
}
#endif /* MacOSX */

#if defined(SOLARIS2)
static int solaris_user_enabled(const char *path, uid_t uid, int *enabled) {
  errno = ENOSYS;
  return -1;
}

static int solaris_user_get(const char *path, uid_t uid, uint64_t *kb_total,
    uint64_t *kb_used, uint64_t *file_total, uint64_t *file_used) {
  int res, fd;
  struct stat st;
  struct dqblk dq;
  struct quotctl qctl;

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    int xerrno = errno;

    pr_trace_msg(trace_channel, 3,
      "unable to open '%s': %s", path, strerror(xerrno));

    errno = xerrno;
    return -1;
  }

  res = fstat(fd, &st);
  if (res < 0) {
    int xerrno = errno;

    pr_trace_msg(trace_channel, 5,
      "fstat(2) error for '%s' (fd %d): %s", path, fd, strerror(xerrno));

    (void) close(fd);
    errno = xerrno;
    return -1;
  }

  qctl.op = Q_GETQUOTA;
  qctl.uid = uid;
  qctl.addr = (void *) &dq;

  res = ioctl(fd, Q_QUOTACTL, &qctl);
  if (res == 0) {
    if (kb_total != NULL) {
      *kb_total = ((dq.dqb_bsoftlimit * st.st_blksize) / 1024);
    }

    if (kb_used != NULL) {
      *kb_used = ((dq.dqb_curblocks * st.st_blksize) / 1024);
    }

    if (file_total != NULL) {
      *file_total = (uint64_t) dq.dqb_fsoftlimit;
    }

    if (file_used != NULL) {
      *file_used = (uint64_t) dq.dqb_curfiles;
    }

  } else {
    int xerrno = xerrno;

    pr_trace_msg(trace_channel, 9,
      "Solaris: error obtaining user quotas for UID %lu, path '%s': %s",
      (unsigned long) uid, path, strerror(xerrno));

    errno = xerrno;
  }

  (void) close(fd);
  return res;
}

static int solaris_group_enabled(const char *path, gid_t gid, int *enabled) {
  /* Solaris doesn't support group quotas. */
  errno = ENOSYS;
  return -1;
}

static int solaris_group_get(const char *path, gid_t gid, uint64_t *kb_total,
    uint64_t *kb_used, uint64_t *file_total, uint64_t *file_used) {
  /* Solaris doesn't support group quotas. */
  errno = ENOSYS;
  return -1;
}
#endif /* Solaris */

/* XXX NFS */

int fsquota_user_enabled(const char *path, uid_t uid, int *enabled) {
  int res = -1;

#if defined(LINUX)
  res = linux_user_enabled(path, uid, enabled);

#elif defined(FREEBSD7) || defined(FREEBSD8) || defined(FREEBSD9) || \
      defined(FREEBSD10)
  res = freebsd_user_enabled(path, uid, enabled);

#elif defined(DARWIN9) || defined(DARWIN10) || defined(DARWIN11)
  res = darwin_user_enabled(path, uid, enabled);

#elif defined(SOLARIS2)
  res = solaris_user_enabled(path, uid, enabled);

#else
  pr_trace_msg(trace_channel, 3,
    "checking user quota status for platform '%s' not implemented",
    PR_PLATFORM);

  errno = ENOSYS;
  res = -1;
#endif

  return res;
}

int fsquota_group_enabled(const char *path, gid_t gid, int *enabled) {
  int res = -1;

#if defined(LINUX)
  res = linux_group_enabled(path, gid, enabled);

#elif defined(FREEBSD7) || defined(FREEBSD8) || defined(FREEBSD9) || \
      defined(FREEBSD10)
  res = freebsd_group_enabled(path, gid, enabled);

#elif defined(DARWIN9) || defined(DARWIN10) || defined(DARWIN11)
  res = darwin_group_enabled(path, gid, enabled);

#elif defined(SOLARIS2)
  res = solaris_group_enabled(path, gid, enabled);

#else
  pr_trace_msg(trace_channel, 3,
    "checking group quota status for platform '%s' not implemented",
    PR_PLATFORM);

  errno = ENOSYS;
  res = -1;
#endif

  return res;
}

int fsquota_user_get(const char *path, uid_t uid, uint64_t *kb_total,
    uint64_t *kb_used, uint64_t *file_total, uint64_t *file_used) {
  int res = -1;

#if defined(LINUX)
  res = linux_user_get(path, uid, kb_total, kb_used, file_total, file_used);

#elif defined(FREEBSD7) || defined(FREEBSD8) || defined(FREEBSD9) || \
      defined(FREEBSD10)
  res = freebsd_user_get(path, uid, kb_total, kb_used, file_total, file_used);

#elif defined(DARWIN9) || defined(DARWIN10) || defined(DARWIN11)
  res = darwin_user_get(path, uid, kb_total, kb_used, file_total, file_used);

#elif defined(SOLARIS2)
  res = solaris_user_get(path, uid, kb_total, kb_used, file_total, file_used);

#else
  pr_trace_msg(trace_channel, 3,
    "getting user quota for platform '%s' not implemented",
    PR_PLATFORM);

  errno = ENOSYS;
  res = -1;
#endif

  return res;
}

int fsquota_group_get(const char *path, gid_t gid, uint64_t *kb_total,
    uint64_t *kb_used, uint64_t *file_total, uint64_t *file_used) {
  int res = -1;

#if defined(LINUX)
  res = linux_group_get(path, gid, kb_total, kb_used, file_total, file_used);

#elif defined(FREEBSD7) || defined(FREEBSD8) || defined(FREEBSD9) || \
      defined(FREEBSD10)
  res = freebsd_group_get(path, gid, kb_total, kb_used, file_total, file_used);

#elif defined(DARWIN9) || defined(DARWIN10) || defined(DARWIN11)
  res = darwin_group_get(path, gid, kb_total, kb_used, file_total, file_used);

#elif defined(SOLARIS2)
  res = solaris_group_get(path, gid, kb_total, kb_used, file_total, file_used);

#else
  pr_trace_msg(trace_channel, 3,
    "getting group quota for platform '%s' not implemented",
    PR_PLATFORM);

  errno = ENOSYS;
  res = -1;
#endif

  return res;
}
