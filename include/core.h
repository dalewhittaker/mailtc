/* core.h
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

#ifndef DW_MAILTC_HEADER_FILE
#define DW_MAILTC_HEADER_FILE

#include <string.h> /*memset, strlen*/
#include <signal.h> /*signal stuff*/
#include <errno.h> /*not sure if this is actually required*/
#include <stdlib.h> /*exit, atoi*/
#include <time.h> /*asctime etc in common.c*/

/*The pid stuff is UNIX only*/
#ifdef _POSIX_SOURCE
#include <unistd.h> /*for getpid*/
#include <sys/types.h> /*for kill*/
#define MTC_USE_PIDFUNC
#endif /*_POSIX_SOURCE*/

#include <gmodule.h> /*would be nice to know the exact headers to include here*/
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktable.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkcellrenderer.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkcolorsel.h>
#include <gtk/gtkcolorseldialog.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkfontbutton.h>
#include <gtk/gtktextview.h>

#ifdef MTC_USE_SSL
#include <openssl/evp.h>
#include <openssl/ssl.h>
#endif /*MTC_USE_SSL*/

#include "plugin.h"
#include "strings.h"
#include "envelope_large.h"

#ifdef MTC_NOTMINIMAL
#include "envelope_small.h"
#endif /*MTC_NOTMINIMAL*/

#define DETAILS_FILE "details"
#define CONFIG_FILE "config"
#define FILTER_FILE "filter"
#define UIDL_FILE "uidlfile"
#define LOG_FILE "log"
#define PID_FILE ".pidfile"
#define ENCRYPTION_KEY "mailtc password encryption key"
#define PASSWORD_FILE "encpwd"

#define FILTERSTRING_LEN 100
#define MAX_FILTER_EXP 5 

/*values used for the PID routine*/
#define PID_APPLOAD 1
#define PID_APPEXIT 2
#define PID_APPKILL 4

/*values used for the icon status*/
#define ACTIVE_ICON_NONE -1
#define ACTIVE_ICON_MULTI -2 

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#ifndef LOGIN_NAME_MAX
#define LOGIN_NAME_MAX 256
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

/*this is mainly for testing purposes*/
/*#undef LIBDIR*/
#ifndef LIBDIR
#define LIBDIR "../plugin/.libs"
#endif

#define IS_FILE(file) g_file_test(file, G_FILE_TEST_IS_REGULAR)
#define IS_DIR(file) g_file_test(file, G_FILE_TEST_IS_DIR)
#define FILE_EXISTS(file) g_file_test(file, G_FILE_TEST_EXISTS)
#define PATH_DELIM (G_DIR_SEPARATOR == '/') ? '\\' : '/'

/*As i don't know the maximum length a font name can be
 *it is currently set to maximum filename length*/
#define MAX_FONTNAME_LEN NAME_MAX

/*used to define when to report connection errors*/
#define CONNECT_ERR_NEVER -1
#define CONNECT_ERR_ALWAYS 0

/*global variables*/
enum { ACCOUNT_COLUMN= 0, PROTOCOL_COLUMN, N_COLUMNS };
mtc_cfg config;
GSList *acclist;
GSList *plglist;

/*configdlg.c functions*/
GtkWidget *cfgdlg_run(GtkWidget *dialog);
gboolean accdlg_run(gint profile, gint newaccount);

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
gboolean pw_read(mtc_account *paccount);
gboolean pw_write(mtc_account *paccount);
gchar *mtc_file(gchar *fullpath, gchar *filename, gint account);
gint rm_mtc_file(gchar *shortname, gint count, gint fullcount);

/*docklet.c functions*/
gboolean mail_thread(gpointer data);
#ifdef MTC_EGGTRAYICON
void docklet_clicked(GtkWidget *button, GdkEventButton *event);
void docklet_destroyed(GtkWidget *widget, gpointer data);
#else
void docklet_rclicked(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data);
void docklet_lclicked(GtkStatusIcon *status_icon, gpointer user_data);
#endif /*MTC_EGGTRAYICON*/

/*common.c functions*/
gchar *str_time(void);
gboolean err_dlg(gchar *errmsg, ...);
gboolean err_exit(gchar *errmsg, ...);
gboolean err_noexit(gchar *errmsg, ...);
mtc_icon *pixbuf_create(mtc_icon *picon);

/*encrypter.c functions*/
#ifdef MTC_USE_SSL
gulong pw_encrypt(gchar *decstring, gchar *encstring);
gboolean pw_decrypt(gchar *encstring, gint enclen, gchar *decstring);
#endif /*MTC_USE_SSL*/

/*plugin.c functions*/
gboolean plg_load_all(void);
gboolean plg_unload_all(void);
mtc_plugin *plg_find(const gchar *plugin_name);

/*filterdlg.c functions*/
gboolean filterdlg_run(mtc_account *paccount);
gboolean filter_read(mtc_account *paccount);

/*summarydlg.c functions*/
#ifdef MTC_EXPERIMENTAL
gint sumcfgdlg_run(gchar *default_font);
gboolean sumdlg_create(void);
gboolean sumdlg_show(void);
gboolean sumdlg_hide(void);
gboolean sumdlg_destroy(void);
#endif /*MTC_EXPERIMENTAL*/

#endif /*DW_MAILTC_HEADER_FILE*/

