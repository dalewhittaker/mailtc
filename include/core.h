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

#include <string.h> /*memset, strlen, strcmp*/
#include <signal.h> /*signal stuff*/
#include <errno.h> /*not sure if this is actually required*/
#include <stdlib.h> /*exit, atoi*/
#include <time.h> /*asctime etc in common.c*/
#include <stdio.h> /*rename remove and all kinds of other file related stuff*/
#include <unistd.h> /*getpid*/
#include <sys/types.h> /*mkdir chmod kill*/
#include <sys/stat.h>
#include <limits.h>

#include <gmodule.h> /*would be nice to know the exact headers to include here*/
#include <glib.h>
#include <gtk/gtkmain.h>
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
#include <gtk/gtktooltips.h>
#include <gtk/gtkradiobutton.h>

#ifdef MTC_USE_SSL
#include <openssl/evp.h>
#include <openssl/ssl.h>
#endif /*MTC_USE_SSL*/

#include "eggtrayicon.h"
#include "plugin.h"
#include "strings.h"
#include "envelope_white.h"
#include "envelope_small.h"

#define DETAILS_FILE "details"
#define CONFIG_FILE "config"
#define FILTER_FILE "filter"
#define UIDL_FILE "uidlfile"
#define LOG_FILE "log"
#define PID_FILE ".pidfile"
#define ENCRYPTION_KEY "mailtc password encryption key"
#define PASSWORD_FILE "encpwd"

#define DELAY_STRLEN 10
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

/*TODO this is mainly for testing purposes*/
/*#undef LIBDIR*/
#ifndef LIBDIR
#define LIBDIR "../plugin/.libs"
#endif

#define IS_FILE(file) g_file_test(file, G_FILE_TEST_IS_REGULAR)
#define IS_DIR(file) g_file_test(file, G_FILE_TEST_IS_DIR)
#define FILE_EXISTS(file) g_file_test(file, G_FILE_TEST_EXISTS)

/*structure to hold mailtc configuration details*/
typedef struct _config_details
{
	char check_delay[DELAY_STRLEN];
	char mail_program[NAME_MAX+1];
	unsigned int icon_size;
	unsigned int multiple;
	unsigned int net_debug;
	char icon[ICON_LEN];
	
	/*some additional stuff used*/
	char base_name[NAME_MAX+ 1];
	FILE *logfile;
	
} config_details;

/*global variables*/
enum { ACCOUNT_COLUMN= 0, PROTOCOL_COLUMN, N_COLUMNS };
config_details config;
GSList *acclist;
GSList *plglist;

/*configdlg.c functions*/
GtkWidget * run_config_dialog(GtkWidget *dialog);
int run_details_dialog(int profile, int newaccount);

/*filefunc.c functions*/
int get_program_dir(void);
mail_details *get_account(unsigned int item);
void remove_account(unsigned int item);
void free_accounts(void);
GSList *create_account(void);
int read_accounts(void);
int read_config_file(void);
int write_user_details(mail_details *pcurrent);
int write_config_file(void);
int read_password_from_file(mail_details *paccount);
int write_password_to_file(mail_details *paccount);
char *get_account_file(char *fullpath, char *filename, int account);
int remove_file(char *shortname, int count, int fullcount);

/*docklet.c functions*/
gboolean mail_thread(gpointer data);
void set_icon_colour(GdkPixbuf *pixbuf, char *colourstring);

/*common.c functions*/
char *get_current_time(void);
int run_error_dialog(char *errmsg, ...);
int error_and_log(char *errmsg, ...);
int error_and_log_no_exit(char *errmsg, ...);

/*encrypter.c functions*/
#ifdef MTC_USE_SSL
int encrypt_password(char *decstring, char *encstring);
int decrypt_password(char *encstring, int enclen, char *decstring);
#endif /*MTC_USE_SSL*/

/*plugin.c functions*/
gboolean load_plugins(void);
gboolean unload_plugins(void);
mtc_plugin_info *find_plugin(const gchar *plugin_name);

/*filterdlg.c functions*/
int run_filter_dialog(mail_details *paccount);
int read_filter_info(mail_details *paccount);

#endif /*DW_MAILTC_HEADER_FILE*/

