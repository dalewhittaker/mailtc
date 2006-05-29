/* header.h
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

#ifndef DW_MTC_PLUGIN_HEADER_FILE
#define DW_MTC_PLUGIN_HEADER_FILE


/*TODO all this needs to be reviewed (pretty much whole file)*/
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
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
#include <dlfcn.h> /*interface to dynamic linking loader*/

/*TODO remove this from final one*/
#include "../include/strings.h"
#include "plugin.h"

#ifdef MTC_USE_SSL
#include <openssl/evp.h>
#include <openssl/ssl.h>
#endif
#ifdef MTC_USE_SASL
#include <gsasl.h>
#endif

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

/*structure to hold mailtc configuration details*/
/*TODO this needs to be removed*/
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
mailtc_files files;
config_details config;

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
char *get_account_file(char *fullpath, char *filename, int account);

/*common.c functions TODO some need to be sorted*/
int error_and_log(char *errmsg, ...);
int error_and_log_no_exit(char *errmsg, ...);
void *alloc_mem(size_t size, void *pmem);
void *realloc_mem(size_t size, void *pmem);
char *str_cat(char *dest, const char *source);

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

/*filterdlg.c functions TODO needs to be sorted*/
int search_for_filter_match(mail_details **paccount, char *header);

#endif /*DW_MTC_PLUGIN_HEADER_FILE*/
