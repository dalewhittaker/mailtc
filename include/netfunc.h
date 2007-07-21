/* netfunc.h
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
#ifndef DW_MAILTC_NETFUNC
#define DW_MAILTC_NETFUNC

#include "plg_common.h"

/*OpenSSL libs for SSL/TLS*/
#ifdef SSL_PLUGIN
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

/*for *nix there are a number of network headers*/
#ifdef _POSIX_SOURCE
#include <unistd.h> /*close(), windows appears to require closesocket()*/
#include <sys/select.h> /*select()*/
#include <sys/types.h> /*various stuff*/
#include <netdb.h> /*network stuff*/
#include <netinet/in.h> /*net structs*/
#include <sys/socket.h> /*connect(), send(), recv() etc*/
typedef gint SOCKET;
#endif /*_POSIX_SOURCE*/

/*for windows most of the stuff seems to be done with winsock2.h*/
#ifdef _WIN32
#include <winsock2.h>
typedef gint socklen_t;
#endif /*_WIN32*/

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

/*used for connection to mail server*/
typedef struct _mtc_net
{
    SOCKET sockfd;
    auth_protocol authtype;
    GString *pdata;

    /*message id is required for IMAP*/
    guint msgid;

#ifdef SSL_PLUGIN
    SSL *pssl;
    SSL_CTX *pctx;
#endif

} mtc_net;

/*netfunc.c functions*/
mtc_error net_connect(mtc_net *pnetinfo, mtc_account *paccount);
mtc_error net_available(mtc_net *pnetinfo);
gint net_recv(mtc_net *pnetinfo, gchar *recvstring, guint recvslen);
mtc_error net_send(mtc_net *pnetinfo, gchar *sendstring, gboolean pw);
mtc_error net_disconnect(mtc_net *pnetinfo);

#endif /*DW_MAILTC_NETFUNC*/
