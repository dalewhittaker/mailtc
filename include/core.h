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

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <limits.h>
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
#endif
#ifdef MTC_USE_SASL
#include <gsasl.h>
#endif

#include "eggtrayicon.h"
#include "strings.h"
#include "envelope_white.h"
#include "envelope_small.h"

#define MAXDATASIZE 200 
#define LINE_LENGTH 80

#define DETAILS_FILE "details"
#define CONFIG_FILE "config"
#define UIDL_FILE "uidlfile"
#define FILTER_FILE "filter"
#define LOG_FILE "log"
#define PID_FILE ".pidfile"
#define ENCRYPTION_KEY "mailtc password encryption key"
#define PASSWORD_FILE "encpwd"

#define PROTOCOL_POP "POP"
#define PROTOCOL_IMAP "IMAP"
#define PROTOCOL_APOP "POP (APOP)"
#define PROTOCOL_POP_CRAM_MD5 "POP (CRAM-MD5)"
#define PROTOCOL_IMAP_CRAM_MD5 "IMAP (CRAM-MD5)"
#define PROTOCOL_POP_SSL "POP (SSL/TLS)"
#define PROTOCOL_IMAP_SSL "IMAP (SSL/TLS)"

#define NET_TIMEOUT 25

#define PORT_LEN 20
#define PASSWORD_LEN 32
#define DELAY_STRLEN 10
#define ICON_LEN 10
#define PROTOCOL_LEN 20
#define DIGEST_LEN 32
#define UIDL_LEN 70
#define IMAP_ID_LEN 5
#define FILTERSTRING_LEN 100
#define MAX_FILTER_EXP 5 

#define CONFIG_MODE 0
#define DETAILS_MODE 1

/*used for various arrays to store account value*/
#define MAXINTLEN 10

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

/*structure to hold the file names*/
typedef struct _mailtc_files
{
	char base_name[NAME_MAX+ 1];
	FILE *logfile;
	
} mailtc_files;

/*structure used to hold the filter details*/
typedef struct _filter_details
{
	int matchall;
	int contains[MAX_FILTER_EXP];
	int subject[MAX_FILTER_EXP];
	char search_string[MAX_FILTER_EXP][FILTERSTRING_LEN+ 1];
	
} filter_details;

/*structure to hold mail account details*/
typedef struct _mail_details
{
	char accname[NAME_MAX+ 1];
	char hostname[LOGIN_NAME_MAX+ HOST_NAME_MAX+ 1];
	char port[PORT_LEN];
	char username[LOGIN_NAME_MAX+ HOST_NAME_MAX+ 1];
	char password[PASSWORD_LEN];
	char protocol[PROTOCOL_LEN];
	char icon[ICON_LEN];
	unsigned int runfilter;
	filter_details *pfilters;
	
	/*the linked list id and pointer to next struct*/
	unsigned int id;
	struct _mail_details *next;

	/*added for use with docklet and tooltip*/
	int num_messages;
	int active;
	
} mail_details;

/*structure to hold mailtc configuration details*/
typedef struct _config_details
{
	char check_delay[DELAY_STRLEN];
	char mail_program[NAME_MAX+1];
	unsigned int icon_size;
	unsigned int multiple;
	unsigned int net_debug;
	char icon[ICON_LEN];
	
} config_details;

/*global variables*/
enum { ACCOUNT_COLUMN= 0, PROTOCOL_COLUMN, N_COLUMNS };
mailtc_files files;
config_details config;
mail_details *paccounts;

/*configdlg.c functions*/
GtkWidget * run_config_dialog(GtkWidget *dialog);
int run_details_dialog(int profile, int newaccount);

/*netfunc.c functions*/
#ifdef MTC_USE_SSL
int net_data_available(int sockfd, SSL *ssl);
#else
int net_data_available(int sockfd, char *ssl);
#endif /*MTC_USE_SSL*/
int connect_to_server(int *sockfd, mail_details *paccount);
#ifdef MTC_USE_SSL
int send_net_string(int sockfd, char *sendstring, SSL *ssl);
#else
int send_net_string(int sockfd, char *sendstring, char *ssl);
#endif /*MTC_USE_SSL*/
#ifdef MTC_USE_SSL
int receive_net_string(int sockfd, char *buf, SSL *ssl);
#else
int receive_net_string(int sockfd, char *buf, char *ssl);
#endif /*MTC_USE_SSL*/

/*popfunc.c functions*/
#ifdef MTC_USE_SSL
int output_uidls_to_file(int sockfd, mail_details *paccount, int num_messages, SSL *ssl);
#else
int output_uidls_to_file(int sockfd, mail_details *paccount, int num_messages, char *ssl);
#endif /*MTC_USE_SSL*/
int check_pop_mail(mail_details *paccount);

/*imapfunc.c functions*/
int check_imap_mail(mail_details *paccount);

/*filefunc.c functions*/
int get_program_dir(void);
mail_details **get_account(unsigned int item);
void remove_account(unsigned int item);
void free_accounts(void);
mail_details *create_account(mail_details **pfirst);
int read_accounts(void);
int read_config_file(void);
int write_user_details(mail_details **pcurrent);
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
void *alloc_mem(size_t size, void *pmem);
void *realloc_mem(size_t size, void *pmem);
char *str_cat(char *dest, const char *source);
char *str_cpy(char *dest, const char *source);
char *str_ins(char *dest, const char *source, int pos);
int str_case_search(char *haystack, char *needle);

/*encrypter.c functions*/
#ifdef MTC_USE_SSL
int encrypt_password(char *decstring, char *encstring);
int decrypt_password(char *encstring, int enclen, char *decstring);
unsigned int encrypt_apop_string(char *decstring, char *encstring);

/*tls.c functions*/
SSL_CTX *initialise_ssl_ctx(SSL_CTX *ctx);
SSL *initialise_ssl_connection(SSL *ssl, SSL_CTX *ctx, int *sockfd);
int uninitialise_ssl(SSL *ssl, SSL_CTX *ctx);
#endif

/*sasl.c functions*/
#ifdef MTC_USE_SASL
char *create_cram_string(Gsasl *ctx, char *username, char *password, char *serverdigest, char *clientdigest);
#endif

/*filterdlg.c functions*/
int run_filter_dialog(mail_details **paccount);
int read_filter_info(mail_details *paccount);
int search_for_filter_match(mail_details **paccount, char *header);

#endif /*DW_MAILTC_HEADER_FILE*/
