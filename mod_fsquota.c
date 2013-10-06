/*
 * ProFTPD - mod_fsquota
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
 *
 * -----DO NOT EDIT BELOW THIS LINE-----
 * $Archive: mod_fsquota.a $
 */

#include "mod_fsquota.h"
#include "fsquota.h"

module fsquota_module;

static int fsquota_authenticated = FALSE;
static int fsquota_engine = FALSE;

static unsigned long fsquota_opts = 0UL;
#define FSQUOTA_SHOW_QUOTA	0x001

static pool *fsquota_pool = NULL;

static const char *trace_channel = "fsquota";

/* Variable handlers
 */

/* XXX Future enhancement: keep per-path table with struct of all values,
 * user and group, to save on system calls for each individual Display
 * variables.
 */

static const char *format_file_str(pool *p, uint64_t file) {
  char buf[512];
  int len;

  memset(buf, '\0', sizeof(buf));
  len = snprintf(buf, sizeof(buf)-1, "%lu", (unsigned long) file);

  return pstrndup(p, buf, len);
}

static const char *format_kb_str(pool *p, uint64_t kb) {
  char *units[] = {"K", "M", "G", "T", "P"};
  char buf[512];
  unsigned int nunits = 5;
  register unsigned int i = 0;
  int len;

  /* Determine the appropriate units label to use. */
  while (kb > 1024 &&
         i < nunits) {
    pr_signals_handle();

    kb /= 1024;
    i++;
  }

  /* Now, prepare the buffer. */
  memset(buf, '\0', sizeof(buf));
  len = snprintf(buf, sizeof(buf)-1, "%.2lu%sB", (unsigned long) kb,
    units[i]);

  return pstrndup(p, buf, len);
}

static const char *fsquota_group_enabled_str(void *data, size_t datasz) {
  const char *status = "unknown";

  if (fsquota_engine == FALSE) {
    return status;
  }

  if (fsquota_authenticated == TRUE) {
    int enabled = -1, res;

    res = fsquota_group_enabled(pr_fs_getcwd(), session.gid, &enabled);
    if (res < 0) {
      status = "unavailable";

    } else {
      status = (enabled ? "true" : "false");
    }

  } else {
    status = "unavailable";
  }

  return status;
}

static const char *fsquota_group_total_files_str(void *data, size_t datasz) {
  const char *total = "unknown";

  if (fsquota_engine == FALSE) {
    return total;
  }

  if (fsquota_authenticated == TRUE) {
    int res;
    uint64_t file_total = 0;

    res = fsquota_group_get(pr_fs_getcwd(), session.gid, NULL, NULL,
      &file_total, NULL);
    if (res < 0) {
      total = "unavailable";

    } else {
      total = format_file_str(fsquota_pool, file_total);
    }

  } else {
    total = "unavailable";
  }

  return total;
}

static const char *fsquota_group_total_kb_str(void *data, size_t datasz) {
  const char *total = "unknown";

  if (fsquota_engine == FALSE) {
    return total;
  }

  if (fsquota_authenticated == TRUE) {
    int res;
    uint64_t kb_total = 0;

    res = fsquota_group_get(pr_fs_getcwd(), session.gid, &kb_total,
      NULL, NULL, NULL);
    if (res < 0) {
      total = "unavailable";

    } else {
      total = format_kb_str(fsquota_pool, kb_total);
    }

  } else {
    total = "unavailable";
  }

  return total;
}

static const char *fsquota_group_used_files_str(void *data, size_t datasz) {
  const char *used = "unknown";

  if (fsquota_engine == FALSE) {
    return used;
  }

  if (fsquota_authenticated == TRUE) {
    int res;
    uint64_t file_used = 0;

    res = fsquota_group_get(pr_fs_getcwd(), session.gid, NULL, NULL,
      NULL, &file_used);
    if (res < 0) {
      used = "unavailable";

    } else {
      used = format_file_str(fsquota_pool, file_used);
    }

  } else {
    used = "unavailable";
  }

  return used;
}

static const char *fsquota_group_used_kb_str(void *data, size_t datasz) {
  const char *used = "unknown";

  if (fsquota_engine == FALSE) {
    return used;
  }

  if (fsquota_authenticated == TRUE) {
    int res;
    uint64_t kb_used = 0;

    res = fsquota_group_get(pr_fs_getcwd(), session.gid, NULL, &kb_used,
      NULL, NULL);
    if (res < 0) {
      used = "unavailable";

    } else {
      used = format_kb_str(fsquota_pool, kb_used);
    }

  } else {
    used = "unavailable";
  }

  return used;
}

static const char *fsquota_user_enabled_str(void *data, size_t datasz) {
  const char *status = "unknown";

  if (fsquota_engine == FALSE) {
    return status;
  }

  if (fsquota_authenticated == TRUE) {
    int enabled = -1, res;

    res = fsquota_user_enabled(pr_fs_getcwd(), session.uid, &enabled);
    if (res < 0) {
      status = "unavailable";

    } else {
      status = (enabled ? "true" : "false");
    }

  } else {
    status = "unavailable";
  }

  return status;
}

static const char *fsquota_user_total_files_str(void *data, size_t datasz) {
  const char *total = "unknown";

  if (fsquota_engine == FALSE) {
    return total;
  }

  if (fsquota_authenticated == TRUE) {
    int res;
    uint64_t file_total = 0;

    res = fsquota_user_get(pr_fs_getcwd(), session.uid, NULL, NULL,
      &file_total, NULL);
    if (res < 0) {
      total = "unavailable";

    } else {
      total = format_file_str(fsquota_pool, file_total);
    }

  } else {
    total = "unavailable";
  }

  return total;
}

static const char *fsquota_user_total_kb_str(void *data, size_t datasz) {
  const char *total = "unknown";

  if (fsquota_engine == FALSE) {
    return total;
  }

  if (fsquota_authenticated == TRUE) {
    int res;
    uint64_t kb_total = 0;

    res = fsquota_user_get(pr_fs_getcwd(), session.uid, &kb_total,
      NULL, NULL, NULL);
    if (res < 0) {
      total = "unavailable";

    } else {
      total = format_kb_str(fsquota_pool, kb_total);
    }

  } else {
    total = "unavailable";
  }

  return total;
}

static const char *fsquota_user_used_files_str(void *data, size_t datasz) {
  const char *used = "unknown";

  if (fsquota_engine == FALSE) {
    return used;
  }

  if (fsquota_authenticated == TRUE) {
    int res;
    uint64_t file_used = 0;

    res = fsquota_user_get(pr_fs_getcwd(), session.gid, NULL, NULL,
      NULL, &file_used);
    if (res < 0) {
      used = "unavailable";

    } else {
      used = format_file_str(fsquota_pool, file_used);
    }

  } else {
    used = "unavailable";
  }

  return used;
}

static const char *fsquota_user_used_kb_str(void *data, size_t datasz) {
  const char *used = "unknown";

  if (fsquota_engine == FALSE) {
    return used;
  }

  if (fsquota_authenticated == TRUE) {
    int res;
    uint64_t kb_used = 0;

    res = fsquota_user_get(pr_fs_getcwd(), session.uid, NULL, &kb_used,
      NULL, NULL);
    if (res < 0) {
      used = "unavailable";

    } else {
      used = format_kb_str(fsquota_pool, kb_used);
    }

  } else {
    used = "unavailable";
  }

  return used;
}

/* Configuration handlers
 */

/* usage: FSQuotaEngine on|off */
MODRET set_fsquotaengine(cmd_rec *cmd) {
  int bool = 1;
  config_rec *c;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  bool = get_boolean(cmd, 1);
  if (bool == -1)
    CONF_ERROR(cmd, "expected Boolean parameter");

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(int));
  *((int *) c->argv[0]) = bool;

  return PR_HANDLED(cmd);
}

/* usage: FSQuotaOptions opt1 ... optN */
MODRET set_fsquotaoptions(cmd_rec *cmd) {
  register unsigned int i;
  unsigned long opts = 0UL;
  config_rec *c;

  if (cmd->argc < 2) {
    CONF_ERROR(cmd, "wrong number of parameters");
  }

  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  for (i = 1; i < cmd->argc; i++) {
    if (strcmp(cmd->argv[i], "ShowQuota") == 0) {
      opts |= FSQUOTA_SHOW_QUOTA;

    } else {
      CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "unsupported FSQuotaOption: ",
        cmd->argv[i], NULL));
    }
  }

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = palloc(c->pool, sizeof(unsigned long));
  *((unsigned long *) c->argv[0]) = opts;

  return PR_HANDLED(cmd);
}

/* Command handlers
 */

MODRET fsquota_post_pass(cmd_rec *cmd) {
  if (fsquota_engine == FALSE) {
    return PR_DECLINED(cmd);
  }

  fsquota_authenticated = TRUE;
  return PR_DECLINED(cmd);
}

MODRET fsquota_site(cmd_rec *cmd) {

  /* Make sure it's a valid SITE FSQUOTA command */
  if (cmd->argc < 2) {
    return PR_DECLINED(cmd);
  }

  if (strncasecmp(cmd->argv[1], "FSQUOTA", 8) == 0) {
    char *cmd_name;
    int res, enabled = FALSE;

    if (fsquota_authenticated == FALSE) {
      pr_response_send(R_530, _("Please login with USER and PASS"));
      return PR_ERROR(cmd);
    }

    if (fsquota_engine == FALSE) {
      pr_response_add_err(R_500, _("'SITE FSQUOTA' not understood."));
      return PR_ERROR(cmd);
    }

    /* Is showing of the user's fsquota barred by configuration? */
    if (!(fsquota_opts & FSQUOTA_SHOW_QUOTA)) {
      pr_response_add_err(R_500, _("'SITE FSQUOTA' not understood."));
      return PR_ERROR(cmd);
    }

    /* Check for <Limit> restrictions. */
    cmd_name = cmd->argv[0];
    pr_cmd_set_name(cmd, "SITE_FSQUOTA");
    if (!dir_check(cmd->tmp_pool, cmd, "NONE", session.cwd, NULL)) {
      int xerrno = EPERM;

      pr_cmd_set_name(cmd, cmd_name);
      pr_response_add_err(R_550, "%s: %s", cmd->arg, strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }
    pr_cmd_set_name(cmd, cmd_name);

    /* Log that the user requested their quota. */
    pr_log_debug(DEBUG10, MOD_FSQUOTA_VERSION
      ": SITE FSQUOTA requested by user %s", session.user);

    /* If fsquota are not in use, no need to do anything. */
    res = fsquota_user_enabled(pr_fs_getcwd(), session.uid, &enabled);
    if (res < 0 ||
        enabled != TRUE) {
      pr_response_add(R_202, _("No filesystem quotas in effect"));
      return PR_HANDLED(cmd);
    }

    /* XXX Implement */

    /* Add one final line to preserve the spacing. */
    pr_response_add(R_DUP,
      _("Please contact %s if these entries are inaccurate"),
      cmd->server->ServerAdmin ? cmd->server->ServerAdmin : _("ftp-admin"));

    return PR_HANDLED(cmd);

  } else if (strncasecmp(cmd->argv[1], "HELP", 5) == 0) {
    if (fsquota_engine == TRUE) {
      /* Add a description of SITE FSQUOTA to the output. */
      pr_response_add(R_214, "FSQUOTA");
    }
  }

  return PR_DECLINED(cmd);
}

/* Event handlers
 */

/* Initialization routines
 */

static int fsquota_sess_init(void) {
  config_rec *c;
  int res;

  fsquota_pool = make_sub_pool(session.pool);
  pr_pool_tag(fsquota_pool, MOD_FSQUOTA_VERSION);

  res = pr_var_set(fsquota_pool, "%{fsquota.user.enabled}",
    "User quotas enabled", PR_VAR_TYPE_FUNC,
    (void *) fsquota_user_enabled_str, NULL, 0);
  if (res < 0) {
    pr_trace_msg(trace_channel, 8,
      "error registering %%{fsquota.user.enabled} variable: %s",
      strerror(errno));
  }

  res = pr_var_set(fsquota_pool, "%{fsquota.user.kb.total}",
    "Maximum number of KB on disk for user", PR_VAR_TYPE_FUNC,
    (void *) fsquota_user_total_kb_str, NULL, 0);
  if (res < 0) {
    pr_trace_msg(trace_channel, 8,
      "error registering %%{fsquota.user.kb.total} variable: %s",
      strerror(errno));
  }

  res = pr_var_set(fsquota_pool, "%{fsquota.user.kb.used}",
    "Current number of KB on disk for user", PR_VAR_TYPE_FUNC,
    (void *) fsquota_user_used_kb_str, NULL, 0);
  if (res < 0) {
    pr_trace_msg(trace_channel, 8,
      "error registering %%{fsquota.user.kb.used} variable: %s",
      strerror(errno));
  }

  res = pr_var_set(fsquota_pool, "%{fsquota.user.files.total}",
    "Maximum number of files on disk for user", PR_VAR_TYPE_FUNC,
    (void *) fsquota_user_total_files_str, NULL, 0);
  if (res < 0) {
    pr_trace_msg(trace_channel, 8,
      "error registering %%{fsquota.user.files.total} variable: %s",
      strerror(errno));
  }

  res = pr_var_set(fsquota_pool, "%{fsquota.user.files.used}",
    "Current number of files on disk for user", PR_VAR_TYPE_FUNC,
    (void *) fsquota_user_used_files_str, NULL, 0);
  if (res < 0) {
    pr_trace_msg(trace_channel, 8,
      "error registering %%{fsquota.user.files.used} variable: %s",
      strerror(errno));
  }

  res = pr_var_set(fsquota_pool, "%{fsquota.group.enabled}",
    "Group quotas enabled", PR_VAR_TYPE_FUNC,
    (void *) fsquota_group_enabled_str, NULL, 0);
  if (res < 0) {
    pr_trace_msg(trace_channel, 8,
      "error registering %%{fsquota.group.enabled} variable: %s",
      strerror(errno));
  }

  res = pr_var_set(fsquota_pool, "%{fsquota.group.kb.total}",
    "Maximum number of KB on disk for group", PR_VAR_TYPE_FUNC,
    (void *) fsquota_group_total_kb_str, NULL, 0);
  if (res < 0) {
    pr_trace_msg(trace_channel, 8,
      "error registering %%{fsquota.group.kb.total} variable: %s",
      strerror(errno));
  }

  res = pr_var_set(fsquota_pool, "%{fsquota.group.kb.used}",
    "Current number of KB on disk for group", PR_VAR_TYPE_FUNC,
    (void *) fsquota_group_used_kb_str, NULL, 0);
  if (res < 0) {
    pr_trace_msg(trace_channel, 8,
      "error registering %%{fsquota.group.kb.used} variable: %s",
      strerror(errno));
  }

  res = pr_var_set(fsquota_pool, "%{fsquota.group.files.total}",
    "Maximum number of files on disk for group", PR_VAR_TYPE_FUNC,
    (void *) fsquota_group_total_files_str, NULL, 0);
  if (res < 0) {
    pr_trace_msg(trace_channel, 8,
      "error registering %%{fsquota.group.files.total} variable: %s",
      strerror(errno));
  }

  res = pr_var_set(fsquota_pool, "%{fsquota.group.files.used}",
    "Current number of files on disk for group", PR_VAR_TYPE_FUNC,
    (void *) fsquota_group_used_files_str, NULL, 0);
  if (res < 0) {
    pr_trace_msg(trace_channel, 8,
      "error registering %%{fsquota.group.files.used} variable: %s",
      strerror(errno));
  }

  c = find_config(main_server->conf, CONF_PARAM, "FSQuotaEngine", FALSE);
  if (c) {
    fsquota_engine = *((int *) c->argv[0]);
  }

  if (fsquota_engine == FALSE) {
    destroy_pool(fsquota_pool);
    fsquota_pool = NULL;

    return 0;
  }

  c = find_config(main_server->conf, CONF_PARAM, "FSQuotaOptions", FALSE);
  while (c != NULL) {
    unsigned long opts;

    pr_signals_handle();

    opts = *((unsigned long *) c->argv[0]);
    fsquota_opts |= opts;

    c = find_config_next(c, c->next, CONF_PARAM, "FSQuotaOptions", FALSE);
  }

  return 0;
}

/* Module API tables
 */

static conftable fsquota_conftab[] = {
  { "FSQuotaEngine",	set_fsquotaengine,	NULL },
  { "FSQuotaOptions",	set_fsquotaoptions,	NULL },
  { NULL }
};

static cmdtable fsquota_cmdtab[] = {
  { POST_CMD,	C_PASS, G_NONE,	fsquota_post_pass,	FALSE,	FALSE },
  { CMD,	C_SITE,	G_NONE,	fsquota_site,		FALSE,	FALSE,	CL_MISC },

  { 0, NULL }
};

module fsquota_module = {
  /* Always NULL */
  NULL, NULL,

  /* Module API version */
  0x20,

  /* Module name */
  "fsquota",

  /* Module configuration handler table */
  fsquota_conftab,

  /* Module command handler table */
  fsquota_cmdtab,

  /* Module authentication handler table */
  NULL,

  /* Module initialization */
  NULL,

  /* Session initialization */
  fsquota_sess_init,

  /* Module version */
  MOD_FSQUOTA_VERSION
};

