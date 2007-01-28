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
/*i think all this needs to be here*/
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h> /*anything here?*/
#include <errno.h>
#include <time.h>

#ifdef _POSIX_SOURCE
#include <unistd.h> /*close(), windows appears to require closesocket()*/
#include <sys/select.h> /*select()*/
#include <sys/types.h> /*various stuff*/
#include <netdb.h> /*network stuff*/
#include <netinet/in.h> /*net structs*/
#include <sys/socket.h> /*connect(), send(), recv() etc*/
#include <limits.h> /*NAME_MAX and others*/
#endif /*_POSIX_SOURCE*/

/*for windows most of the stuff seems to be done with winsock2.h*/
#ifdef _WIN32
#include <winsock2.h>
#endif /*_WIN32*/

#include <glib/gstdio.h>
#include <gmodule.h> /*interface to dynamic linking loader*/

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
#define CONNECT_TIMEOUT 30
#define NET_TIMEOUT 5

/*win32 defines INVALID_SOCKET for socket()*/
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif /*INVALID_SOCKET*/

/*win32 defines SOCKET_ERROR for setsockopt()/getsockopt()*/
#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif /*SOCKET_ERROR*/

#define DIGEST_LEN 32
#define IMAP_ID_LEN 5

#define IS_SSL_AUTH(auth) ((auth)== POPSSL_PROTOCOL|| (auth)== IMAPSSL_PROTOCOL)
#define IS_FILE(file) g_file_test((file), G_FILE_TEST_IS_REGULAR)

/*define the various protocols for the default plugins*/
typedef enum _auth_protocol
{ 
    POP_PROTOCOL= 0,
    APOP_PROTOCOL,
    POPCRAM_PROTOCOL,
    POPSSL_PROTOCOL,
    IMAP_PROTOCOL,
    IMAPCRAM_PROTOCOL,
    IMAPSSL_PROTOCOL,
    N_PROTOCOLS
} auth_protocol;

/*define the various imap responses, note they can either be tagged or untagged
 *both are required.  Rather than define an id for each, use a macro to use higher word for tagged*/
#define MAX_RESPONSE_STRING 20
#define TAGGED_RESPONSE(response) ((response)<< 8)
typedef enum _imap_response
{ 
    IMAP_OK= 1,
    IMAP_BAD= 2,
    IMAP_NO= 4,
    IMAP_BYE= 8,
    IMAP_PREAUTH= 16,
    IMAP_CAPABILITY= 32,
    IMAP_MAXRESPONSE
} imap_response;

/*used to contain response strings*/
typedef struct _imap_response_list
{
    guint id;
    gchar *str;
} imap_response_list;

/*used for connection to mail server*/
typedef struct _mtc_net
{
#ifdef _WIN32
    SOCKET sockfd;
#else
    gint sockfd;
#endif /*_WIN32*/

    /*maybe should change to a protocol enum*/
    auth_protocol authtype;
    GString *pdata;

    /*message id is required for IMAP*/
    guint msgid;

#ifdef SSL_PLUGIN
    SSL *pssl;
    SSL_CTX *pctx;
#endif

} mtc_net;

/*global variables, currently just the debug flag and the pointer to log file*/

/*netfunc.c functions*/
/*these must have different names as otherwise when loaded they will be duplicates*/
mtc_error net_connect(mtc_net *pnetinfo, mtc_account *paccount);
mtc_error net_available(mtc_net *pnetinfo);
gint net_recv(mtc_net *pnetinfo, gchar *recvstring, guint recvslen);
mtc_error net_send(mtc_net *pnetinfo, gchar *sendstring, gboolean pw);
mtc_error net_disconnect(mtc_net *pnetinfo);

/*popfunc.c functions*/
mtc_error check_pop_mail(mtc_account *paccount, const mtc_cfg *pconfig);
mtc_error pop_read_mail(mtc_account *paccount, const mtc_cfg *pconfig);
#ifdef SSL_PLUGIN
mtc_error check_apop_mail(mtc_account *paccount, const mtc_cfg *pconfig);
mtc_error check_crampop_mail(mtc_account *paccount, const mtc_cfg *pconfig);
mtc_error check_popssl_mail(mtc_account *paccount, const mtc_cfg *pconfig);
#endif /*SSL_PLUGIN*/

/*imapfunc.c functions*/
mtc_error check_imap_mail(mtc_account *paccount, const mtc_cfg *pconfig);
mtc_error imap_read_mail(mtc_account *paccount, const mtc_cfg *pconfig);
#ifdef SSL_PLUGIN
mtc_error check_cramimap_mail(mtc_account *paccount, const mtc_cfg *pconfig);
mtc_error check_imapssl_mail(mtc_account *paccount, const mtc_cfg *pconfig);
#endif /*SSL_PLUGIN*/

/*functions common to all the default plugins*/
mtc_cfg *cfg_get(void);
void cfg_load(mtc_cfg *pconfig);
void cfg_unload(void);
gint plg_err(gchar *errmsg, ...);
gchar *mtc_file(gchar *fullpath, const gchar *cfgdir, gchar *filename, guint account);
gint strstr_cins(gchar *haystack, gchar *needle);

/*extra authentication functions*/
#ifdef SSL_PLUGIN
gchar *mk_cramstr(mtc_account *paccount, gchar *serverdigest, gchar *clientdigest);
guint apop_encrypt(gchar *decstring, gchar *encstring);
#endif /*SSL_PLUGIN*/

/*msg.c functions*/
/*gboolean print_msg_info(mtc_account *paccount);*/ /*just for debugging*/
GSList *msglist_add(mtc_account *paccount, gchar *uid, GString *header);
GSList *msglist_cleanup(mtc_account *paccount);
void msglist_reset(mtc_account *paccount);
gboolean msglist_verify(mtc_account *paccount);

#endif /*MTC_PLUGIN_COMMON*/
