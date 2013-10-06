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

static int fsquota_engine = FALSE;

static const char *trace_channel = "fsquota";

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

/* Event handlers
 */

/* Initialization routines
 */

static int fsquota_sess_init(void) {
  config_rec *c;

  c = find_config(main_server->conf, CONF_PARAM, "FSQuotaEngine", FALSE);
  if (c) {
    fsquota_engine = *((int *) c->argv[0]);
  }

  return 0;
}

/* Module API tables
 */

static conftable fsquota_conftab[] = {
  { "FSQuotaEngine",	set_fsquotaengine,	NULL },
  { NULL }
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
  NULL,

  /* Module authentication handler table */
  NULL,

  /* Module initialization */
  NULL,

  /* Session initialization */
  fsquota_sess_init,

  /* Module version */
  MOD_FSQUOTA_VERSION
};

