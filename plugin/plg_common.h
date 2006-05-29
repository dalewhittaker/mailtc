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

#ifndef MTC_PLUGIN_COMMON
#define MTC_PLUGIN_COMMON

/*probably not all these actually needed, but i can't bring myself to work out which*/
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

/*The main plugin header and the header containing all the plugin strings*/
#include "plg_strings.h"
#include "plugin.h"

#ifdef MTC_USE_SSL
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif
#ifdef MTC_USE_SASL
#include <gsasl.h>
#endif

#define MAXDATASIZE 200 
#define LINE_LENGTH 80
#define UIDL_FILE "uidlfile"
#define NET_TIMEOUT 25

#define DIGEST_LEN 32
#define UIDL_LEN 70
#define IMAP_ID_LEN 5

/*define the possible plugin errors that can be returned*/
/*these two must be negative as the return value is also the num messages*/
#define MTC_RETURN_FALSE 0
#define MTC_RETURN_TRUE 1
#define MTC_ERR_CONNECT -1
#define MTC_ERR_EXIT -2

/*define the various protocols for the default plugins*/
enum pop_protocol { POP_PROTOCOL= 0, APOP_PROTOCOL, POPCRAM_PROTOCOL, POPSSL_PROTOCOL };
enum imap_protocol { IMAP_PROTOCOL= 0, IMAPCRAM_PROTOCOL, IMAPSSL_PROTOCOL };

/*global variables, currently just the debug flag and the pointer to log file*/
unsigned int net_debug;
FILE *plglog;

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
int check_pop_mail(mail_details *paccount, const char *cfgdir, enum pop_protocol protocol);

/*imapfunc.c functions*/
int check_imap_mail(mail_details *paccount, const char *cfgdir, enum imap_protocol protocol);

/*functions common to all the default plugins*/
int error_and_log_no_exit(char *errmsg, ...);
void *alloc_mem(size_t size, void *pmem);
void *realloc_mem(size_t size, void *pmem);
char *str_cat(char *dest, const char *source);
char *str_cpy(char *dest, const char *source);
char *get_account_file(char *fullpath, const char *cfgdir, char *filename, int account);

/*extra authentication functions*/
#ifdef MTC_USE_SSL
unsigned int encrypt_apop_string(char *decstring, char *encstring);

SSL_CTX *initialise_ssl_ctx(SSL_CTX *ctx);
SSL *initialise_ssl_connection(SSL *ssl, SSL_CTX *ctx, int *sockfd);
int uninitialise_ssl(SSL *ssl, SSL_CTX *ctx);
#endif

#ifdef MTC_USE_SASL
char *create_cram_string(Gsasl *ctx, char *username, char *password, char *serverdigest, char *clientdigest);
#endif

/*function to search for a filter match in the mail header*/
int search_for_filter_match(mail_details **paccount, char *header);

#endif /*MTC_PLUGIN_COMMON*/
