/* mtc-socket.c
 * Copyright (C) 2009 Dale Whittaker <dayul@users.sf.net>
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

#include "mtc-socket.h"

#include <config.h>
#include <string.h>     /* memset ()                    */
#include <unistd.h>     /* close ()                     */
#include <sys/select.h> /* select ()                    */
#include <sys/types.h>  /* various stuff                */
#include <netdb.h>      /* network stuff                */
#include <netinet/in.h> /* net structs                  */
#include <sys/socket.h> /* connect (), send (), recv () */

#if HAVE_GNUTLS
#include <gnutls/gnutls.h>
#define MTC_CA_FILE "ca.pem"
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#define CONNECT_TIMEOUT 5
#define NET_TIMEOUT 15

#define MAILTC_SOCKET_ERROR g_quark_from_string("MAILTC_SOCKET_ERROR")

typedef enum
{
    MAILTC_SOCKET_ERROR_HOST = 0,
    MAILTC_SOCKET_ERROR_CREATE,
    MAILTC_SOCKET_ERROR_CONNECT,
    MAILTC_SOCKET_ERROR_SET_OPTIONS,
    MAILTC_SOCKET_ERROR_GET_OPTIONS,
    MAILTC_SOCKET_ERROR_SELECT,
    MAILTC_SOCKET_ERROR_READ,
    MAILTC_SOCKET_ERROR_WRITE,
#if HAVE_GNUTLS
    MAILTC_SOCKET_ERROR_SSL_CONNECT,
#endif

} MailtcSocketError;

typedef gint SOCKET;

struct _MailtcSocketPrivate
{
    SOCKET sockfd;
    struct timeval t;
#if HAVE_GNUTLS
    gnutls_session_t session;
    gnutls_certificate_credentials_t creds;
#endif
};

struct _MailtcSocket
{
    GObject parent_instance;

    MailtcSocketPrivate* priv;
#if HAVE_GNUTLS
    gboolean ssl;
#endif
};

struct _MailtcSocketClass
{
    GObjectClass parent_class;
};

G_DEFINE_TYPE (MailtcSocket, mailtc_socket, G_TYPE_OBJECT)

enum
{
    PROP_0,
    PROP_SSL
};

static void
mailtc_socket_finalize (GObject* object)
{
    MailtcSocket* sock = MAILTC_SOCKET (object);
    MailtcSocketPrivate* priv;

    priv = sock->priv;

    mailtc_socket_disconnect (sock);

#if HAVE_GNUTLS
    if (priv->creds)
    {
        gnutls_certificate_free_credentials (priv->creds);
        priv->creds = NULL;
    }
#endif

    G_OBJECT_CLASS (mailtc_socket_parent_class)->finalize (object);
}

#if HAVE_GNUTLS
void
mailtc_socket_set_ssl (MailtcSocket* sock,
                       gboolean      ssl)
{
    g_return_if_fail (MAILTC_IS_SOCKET (sock));

    sock->ssl = ssl;
}

static void
mailtc_socket_set_property (GObject*      object,
                            guint         prop_id,
                            const GValue* value,
                            GParamSpec*   pspec)
{
    MailtcSocket* sock = MAILTC_SOCKET (object);

    switch (prop_id)
    {
        case PROP_SSL:
            mailtc_socket_set_ssl (sock, g_value_get_boolean (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_socket_get_property (GObject*    object,
                            guint       prop_id,
                            GValue*     value,
                            GParamSpec* pspec)
{
    MailtcSocket* sock = MAILTC_SOCKET (object);

    switch (prop_id)
    {
        case PROP_SSL:
            g_value_set_boolean (value, sock->ssl);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}
#endif

static void
mailtc_socket_class_init (MailtcSocketClass* class)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = mailtc_socket_finalize;
#if HAVE_GNUTLS
    gobject_class->set_property = mailtc_socket_set_property;
    gobject_class->get_property = mailtc_socket_get_property;
#endif
    g_type_class_add_private (class, sizeof (MailtcSocketPrivate));
}

static void
mailtc_socket_init (MailtcSocket* sock)
{
    MailtcSocketPrivate* priv;

    sock->priv = G_TYPE_INSTANCE_GET_PRIVATE (sock,
                 MAILTC_TYPE_SOCKET, MailtcSocketPrivate);
    priv = sock->priv;

    priv->sockfd = INVALID_SOCKET;
    priv->t.tv_sec = -1;
    priv->t.tv_usec = -1;
#if HAVE_GNUTLS
    priv->creds = NULL;
    priv->session = NULL;
#endif
}

static void
mailtc_socket_close (MailtcSocket* sock)
{
    MailtcSocketPrivate* priv;

    g_return_if_fail (MAILTC_IS_SOCKET (sock));

    priv = sock->priv;
    if (priv->sockfd != INVALID_SOCKET)
    {
        shutdown (priv->sockfd, SHUT_RDWR);
        close (priv->sockfd);
        priv->sockfd = INVALID_SOCKET;
    }
}

static gboolean
mailtc_socket_set_timeout (MailtcSocket*   sock,
                           struct timeval* tnew,
                           socklen_t       tnewlen,
                           GError**        error)
{
    MailtcSocketPrivate* priv;
    struct timeval t;
    socklen_t tlen;

    g_return_val_if_fail (MAILTC_IS_SOCKET (sock), FALSE);

    tlen = sizeof (t);
    priv = sock->priv;
    if (getsockopt (priv->sockfd, SOL_SOCKET, SO_RCVTIMEO,
                    (gchar*) &t, &tlen) == SOCKET_ERROR)
    {
        if (error)
        {
            *error = g_error_new (MAILTC_SOCKET_ERROR,
                                  MAILTC_SOCKET_ERROR_GET_OPTIONS,
                                  "Error getting socket options.");
        }
        return FALSE;
    }

    if (setsockopt (priv->sockfd, SOL_SOCKET, SO_RCVTIMEO,
                    (gchar*) tnew, tnewlen) == SOCKET_ERROR)
    {
        if (error)
        {
            *error = g_error_new (MAILTC_SOCKET_ERROR,
                                  MAILTC_SOCKET_ERROR_SET_OPTIONS,
                                  "Error setting socket options.");
        }
        return FALSE;
    }
    priv->t = t;
    return TRUE;
}

#if HAVE_GNUTLS
static void
mailtc_socket_ssl_disconnect (MailtcSocket* sock)
{
    MailtcSocketPrivate* priv;

    g_return_if_fail (MAILTC_IS_SOCKET (sock));

    priv = sock->priv;

    if (priv->session)
    {
        gnutls_bye (priv->session, GNUTLS_SHUT_RDWR);
        gnutls_deinit (priv->session);
        priv->session = NULL;
    }
}

static gboolean
mailtc_socket_ssl_connect (MailtcSocket* sock,
                           GError**      error)
{
    MailtcSocketPrivate* priv;
    gint err = GNUTLS_E_SUCCESS;

    g_return_val_if_fail (MAILTC_IS_SOCKET (sock), FALSE);

    priv = sock->priv;

    if (!priv->creds)
        err = gnutls_certificate_allocate_credentials (&priv->creds);
    if (err == GNUTLS_E_SUCCESS)
        /* TODO sort out certificates */
        /*gnutls_certificate_set_x509_trust_file (priv->creds, MTC_CA_FILE, GNUTLS_X509_FMT_PEM);*/
        err = gnutls_init (&priv->session, GNUTLS_CLIENT);
    if (err == GNUTLS_E_SUCCESS)
        err = gnutls_priority_set_direct (priv->session, "PERFORMANCE", NULL);
    if (err == GNUTLS_E_SUCCESS)
        err = gnutls_credentials_set (priv->session, GNUTLS_CRD_CERTIFICATE, priv->creds);
    if (err == GNUTLS_E_SUCCESS)
    {
        gnutls_transport_set_ptr (priv->session, (gnutls_transport_ptr_t) priv->sockfd);
        err = gnutls_handshake (priv->session);
    }
    if (err != GNUTLS_E_SUCCESS)
    {
        if (error)
        {
            *error = g_error_new (MAILTC_SOCKET_ERROR,
                                  MAILTC_SOCKET_ERROR_SSL_CONNECT,
                                  "Error initialising SSL connection on socket: %s",
                                  gnutls_strerror (err));
        }
        return FALSE;
    }
    return TRUE;
}
#endif

gboolean
mailtc_socket_data_ready (MailtcSocket* sock,
                          GError**      error)
{
    MailtcSocketPrivate* priv;
    fd_set fds;
    struct timeval tv;
    gint n;

    g_return_val_if_fail (MAILTC_IS_SOCKET (sock), -1);

    priv = sock->priv;

#if HAVE_GNUTLS
    if (priv->session && gnutls_record_check_pending (priv->session))
        return TRUE;
#endif

	FD_ZERO (&fds);
	FD_SET (priv->sockfd, &fds);
	tv.tv_sec = NET_TIMEOUT;
	tv.tv_usec = 0;

    n = select ((gint)(priv->sockfd + 1), &fds, NULL, NULL, &tv);

    if (n > 0)
        return TRUE;
    else
    {
        /* n == 0 means timeout */
        if (n == SOCKET_ERROR && error)
        {
            *error = g_error_new (MAILTC_SOCKET_ERROR,
                                  MAILTC_SOCKET_ERROR_SELECT,
                                  "Select error on socket.");
        }
        return FALSE;
    }
}

gint
mailtc_socket_read (MailtcSocket* sock,
                    gchar*        buf,
                    guint         len,
                    GError**      error)
{
    MailtcSocketPrivate* priv;
    gint bytes;

    g_return_val_if_fail (MAILTC_IS_SOCKET (sock), -1);

    priv = sock->priv;

#if HAVE_GNUTLS
    if (priv->session)
    {
        if ((bytes = gnutls_record_recv (priv->session, buf, len)) < 0)
            bytes = SOCKET_ERROR;
    }
    else
#endif
    {
        bytes = recv (priv->sockfd, buf, len, 0);
    }

    if (bytes == SOCKET_ERROR)
    {
        if (error)
        {
            *error = g_error_new (MAILTC_SOCKET_ERROR,
                                  MAILTC_SOCKET_ERROR_READ,
                                  "Error reading from socket.");
        }
        return -1;
    }
    *(buf + bytes) = '\0';
    return bytes;
}

gint
mailtc_socket_write (MailtcSocket* sock,
                     const gchar*  buf,
                     guint         len,
                     GError**      error)
{
    MailtcSocketPrivate* priv;
    gint bytes;

    g_return_val_if_fail (MAILTC_IS_SOCKET (sock), -1);

    priv = sock->priv;

#if HAVE_GNUTLS
    if (priv->session)
    {
        if ((bytes = gnutls_record_send (priv->session, buf, len)) < 0)
            bytes = SOCKET_ERROR;
    }
    else
#endif
    {
        bytes = send (priv->sockfd, buf, len, 0);
    }

    if (bytes == SOCKET_ERROR)
    {
        if (error)
        {
            *error = g_error_new (MAILTC_SOCKET_ERROR,
                                  MAILTC_SOCKET_ERROR_WRITE,
                                  "Error writing to socket.");
        }
        return -1;
    }
    return bytes;
}

void
mailtc_socket_disconnect (MailtcSocket* sock)
{
    g_return_if_fail (MAILTC_IS_SOCKET (sock));

#if HAVE_GNUTLS
    if (sock->priv->session)
        mailtc_socket_ssl_disconnect (sock);
#endif

    mailtc_socket_close (sock);
}

gboolean
mailtc_socket_connect (MailtcSocket* sock,
                       gchar*        server,
                       guint         port,
                       GError**      error)
{
    MailtcSocketPrivate* priv;
    SOCKET sockfd;
    struct hostent* he;
    struct sockaddr_in their_addr;
    struct timeval tnew;

    g_return_val_if_fail (MAILTC_IS_SOCKET (sock), FALSE);

    priv = sock->priv;
    mailtc_socket_disconnect (sock);

    if (!(he = gethostbyname (server)))
    {
        if (error)
        {
            *error = g_error_new (MAILTC_SOCKET_ERROR,
                                  MAILTC_SOCKET_ERROR_HOST,
                                  "Error getting host structure.");
        }
        return FALSE;
    }

    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        if (error)
        {
            *error = g_error_new (MAILTC_SOCKET_ERROR,
                                  MAILTC_SOCKET_ERROR_CREATE,
                                  "Error creating socket.");
        }
        return FALSE;
    }

    priv->sockfd = sockfd;
    their_addr.sin_family = AF_INET;
    their_addr.sin_addr = *((struct in_addr*) he->h_addr_list[0]);
    their_addr.sin_port = g_htons (port);
    memset (&their_addr.sin_zero, 0, 8);

    tnew.tv_sec = CONNECT_TIMEOUT;
    tnew.tv_usec = 0;

    if (!mailtc_socket_set_timeout (sock, &tnew,
                                    sizeof (tnew), error))
        return FALSE;

    if (connect (sockfd, (struct sockaddr*) &their_addr,
                 sizeof (struct sockaddr)) == SOCKET_ERROR)
    {
        if (error)
        {
            *error = g_error_new (MAILTC_SOCKET_ERROR,
                                  MAILTC_SOCKET_ERROR_CONNECT,
                                  "Error initialising connection on socket.");
        }
        mailtc_socket_close (sock);
        return FALSE;
    }

    /* reset the timeout */
    if (!mailtc_socket_set_timeout (sock, &priv->t,
                                    sizeof (priv->t), error))
    {
        mailtc_socket_close (sock);
        return FALSE;
    }

#if HAVE_GNUTLS
    if (sock->ssl)
        return mailtc_socket_ssl_connect (sock, error);
#endif
    return TRUE;
}

MailtcSocket*
mailtc_socket_new (void)
{
    return g_object_new (MAILTC_TYPE_SOCKET, NULL);
}

