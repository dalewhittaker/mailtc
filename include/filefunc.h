/* filefunc.h
 * Copyright (C) 2006 Dale Whittaker <dayul@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef DW_MAILTC_FILEFUNC
#define DW_MAILTC_FILEFUNC

#include "common.h"

/*As i don't know the maximum length a font name can be
 *it is currently set to maximum filename length*/
/*TODO should go in filefunc.c*/
/*TODO may actually be removed when xml implemented*/
/*various files that are used*/
#define DETAILS_FILE "details"
#define CONFIG_FILE "config"
#define FILTER_FILE "filter"
#define UIDL_FILE "uidlfile"
#define LOG_FILE "log"
#define PID_FILE ".pidfile"
#define PASSWORD_FILE "encpwd"

#define MAX_FONTNAME_LEN NAME_MAX

#define IS_FILE(file) g_file_test(file, G_FILE_TEST_IS_REGULAR)
#define IS_DIR(file) g_file_test(file, G_FILE_TEST_IS_DIR)
#define FILE_EXISTS(file) g_file_test(file, G_FILE_TEST_EXISTS)
#define PATH_DELIM (G_DIR_SEPARATOR == '/') ? '\\' : '/'

/*filefunc.c functions*/
gboolean mtc_dir(void);
mtc_account *get_account(guint item);
void remove_account(guint item);
void free_accounts(void);
GSList *create_account(void);
gboolean read_accounts(void);
gboolean cfg_read(void);
gboolean acc_write(mtc_account *pcurrent);
gboolean cfg_write(void);
gchar *mtc_file(gchar *fullpath, gchar *filename, gint account);
gint rm_mtc_file(gchar *shortname, gint count, gint fullcount);

#endif /*DW_MAILTC_FILEFUNC*/
