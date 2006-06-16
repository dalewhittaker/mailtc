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

#ifdef SSL_PLUGIN
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#endif

#define MAXDATASIZE 200 
#define LINE_LENGTH 80
#define UIDL_FILE "uidlfile"
#define TMP_UIDL_FILE ".tmpuidlfile"
#define NET_TIMEOUT 5

#define DIGEST_LEN 32
#define UIDL_LEN 70
#define IMAP_ID_LEN 5

/*define the network functions*/
#ifdef SSL_PLUGIN
#define NET_DATA_AVAILABLE(sockfd, ssl) ssl_net_data_available(sockfd, ssl)
#define SEND_NET_STRING(sockfd, sendstring, ssl) ssl_send_net_string(sockfd, sendstring, ssl, 0)
#define SEND_NET_PW_STRING(sockfd, sendstring, ssl) ssl_send_net_string(sockfd, sendstring, ssl, 1)
#define RECEIVE_NET_STRING(sockfd, recvstring, ssl) ssl_receive_net_string(sockfd, recvstring, ssl)
#else
#define NET_DATA_AVAILABLE(sockfd, ssl) std_net_data_available(sockfd, ssl)
#define SEND_NET_STRING(sockfd, sendstring, ssl) std_send_net_string(sockfd, sendstring, ssl, 0)
#define SEND_NET_PW_STRING(sockfd, sendstring, ssl) std_send_net_string(sockfd, sendstring, ssl, 1)
#define RECEIVE_NET_STRING(sockfd, recvstring, ssl) std_receive_net_string(sockfd, recvstring, ssl)
#endif /*SSL_PLUGIN*/

/*define the various protocols for the default plugins*/
enum pop_protocol { POP_PROTOCOL= 0, APOP_PROTOCOL, POPCRAM_PROTOCOL, POPSSL_PROTOCOL };
enum imap_protocol { IMAP_PROTOCOL= 0, IMAPCRAM_PROTOCOL, IMAPSSL_PROTOCOL };

/*global variables, currently just the debug flag and the pointer to log file*/
unsigned int net_debug;
FILE *plglog;

/*netfunc.c functions*/
/*these must have different names as otherwise when loaded they will be duplicates*/
int connect_to_server(int *sockfd, mail_details *paccount);
#ifdef SSL_PLUGIN
int ssl_net_data_available(int sockfd, SSL *ssl);
int ssl_send_net_string(int sockfd, char *sendstring, SSL *ssl, unsigned int pw);
int ssl_receive_net_string(int sockfd, char *recvstring, SSL *ssl);
#else
int std_net_data_available(int sockfd, char *ssl);
int std_send_net_string(int sockfd, char *sendstring, char *ssl, unsigned int pw);
int std_receive_net_string(int sockfd, char *recvstring, char *ssl);
#endif /*SSL_PLUGIN*/

/*popfunc.c functions*/
int check_pop_mail(mail_details *paccount, const char *cfgdir);
int check_apop_mail(mail_details *paccount, const char *cfgdir);
int check_crampop_mail(mail_details *paccount, const char *cfgdir);
int check_popssl_mail(mail_details *paccount, const char *cfgdir);
int pop_read_mail(mail_details *paccount, const char *cfgdir);

/*imapfunc.c functions*/
int check_imap_mail(mail_details *paccount, const char *cfgdir);
int check_cramimap_mail(mail_details *paccount, const char *cfgdir);
int check_imapssl_mail(mail_details *paccount, const char *cfgdir);
int imap_read_mail(mail_details *paccount, const char *cfgdir);

/*functions common to all the default plugins*/
int plg_report_error(char *errmsg, ...);
void *alloc_mem(size_t size, void *pmem);
void *realloc_mem(size_t size, void *pmem);
char *str_cat(char *dest, const char *source);
char *str_cpy(char *dest, const char *source);
char *plg_get_account_file(char *fullpath, const char *cfgdir, char *filename, unsigned int account);

/*extra authentication functions*/
#ifdef SSL_PLUGIN
char *create_cram_string(mail_details *paccount, char *serverdigest, char *clientdigest);
unsigned int encrypt_apop_string(char *decstring, char *encstring);
SSL_CTX *initialise_ssl_ctx(SSL_CTX *ctx);
SSL *initialise_ssl_connection(SSL *ssl, SSL_CTX *ctx, int *sockfd);
int uninitialise_ssl(SSL *ssl, SSL_CTX *ctx);
#endif /*SSL_PLUGIN*/

/*function to search for a filter match in the mail header*/
int search_for_filter_match(mail_details **paccount, char *header);

#endif /*MTC_PLUGIN_COMMON*/
