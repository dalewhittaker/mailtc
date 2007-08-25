/* plg_common.h
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

#ifndef DW_MAILTC_PLG_COMMON
#define DW_MAILTC_PLG_COMMON

/*probably not all these actually needed, but i can't bring myself to work out which*/
/*i think all this needs to be here*/
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

/*The main plugin header and the header containing all the plugin strings*/
#include "plg_strings.h"
#include "plugin.h"

#define MAXDATASIZE 200 
#define LINE_LENGTH 80
#define UIDL_FILE "uidlfile"
#define TMP_UIDL_FILE ".tmpuidlfile"
#define IS_FILE(file) g_file_test((file), G_FILE_TEST_IS_REGULAR)

/*functions common to all the default plugins*/
mtc_cfg *cfg_get(void);
void cfg_load(mtc_cfg *pconfig);
void cfg_unload(void);
gint plg_err(gchar *errmsg, ...);
gchar *mtc_file(gchar *fullpath, const gchar *cfgdir, gchar *filename, guint account);
gint strstr_cins(gchar *haystack, gchar *needle);
mtc_error rm_uidfile(mtc_account *paccount, gint fullcount);

/*extra authentication functions*/
#ifdef SSL_PLUGIN
gchar *mk_cramstr(mtc_account *paccount, gchar *serverdigest, gchar *clientdigest);
guint apop_encrypt(gchar *decstring, gchar *encstring);
#endif /*SSL_PLUGIN*/

#endif /*DW_MAILTC_PLG_COMMON*/
