/* mtc-socket.c
 * Copyright (C) 2009-2011 Dale Whittaker <dayul@users.sf.net>
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
#include <string.h> /* memset () */

#if HAVE_GNUTLS
#include <gnutls/gnutls.h>
#define MTC_CA_FILE "ca.pem"

/* socket is really non-blocking, as gio only emulates blocking in it's code
 * so we need to use non-blocking with gnutls calls
 */
#define TLS_WHILE_ACTIVE(tlsproc, err) \
        do { (err) = (tlsproc); } while ((err) != GNUTLS_E_SUCCESS && !gnutls_error_is_fatal (err))

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
    MAILTC_SOCKET_ERROR_CONNECT,
    MAILTC_SOCKET_ERROR_POLL,
#if HAVE_GNUTLS
    MAILTC_SOCKET_ERROR_SSL_CONNECT,
    MAILTC_SOCKET_ERROR_SSL_READ,
    MAILTC_SOCKET_ERROR_SSL_WRITE
#endif

} MailtcSocketError;

typedef gint SOCKET;

struct _MailtcSocketPrivate
{
    SOCKET sockfd;
    GSocket* gsocket;
    GSocketConnection* connection;
    GCancellable* cancellable;
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
    priv->connection = NULL;
    priv->cancellable = NULL;
    priv->gsocket = NULL;
#if HAVE_GNUTLS
    priv->creds = NULL;
    priv->session = NULL;
#endif
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
        gint err = GNUTLS_E_SUCCESS;

        TLS_WHILE_ACTIVE (gnutls_bye (priv->session, GNUTLS_SHUT_RDWR), err);
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
        TLS_WHILE_ACTIVE (gnutls_certificate_allocate_credentials (&priv->creds), err);
    if (err == GNUTLS_E_SUCCESS)
        /* TODO sort out certificates */
        /*gnutls_certificate_set_x509_trust_file (priv->creds, MTC_CA_FILE, GNUTLS_X509_FMT_PEM);*/
        TLS_WHILE_ACTIVE (gnutls_init (&priv->session, GNUTLS_CLIENT), err);
    if (err == GNUTLS_E_SUCCESS)
        TLS_WHILE_ACTIVE (gnutls_priority_set_direct (priv->session, "PERFORMANCE", NULL), err);
    if (err == GNUTLS_E_SUCCESS)
        TLS_WHILE_ACTIVE (gnutls_credentials_set (priv->session, GNUTLS_CRD_CERTIFICATE, priv->creds), err);
    if (err == GNUTLS_E_SUCCESS)
    {
        gnutls_transport_set_ptr (priv->session, (gnutls_transport_ptr_t) (long) priv->sockfd);
        TLS_WHILE_ACTIVE (gnutls_handshake (priv->session), err);
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
    GPollFD fds;
    gint n;

    g_return_val_if_fail (MAILTC_IS_SOCKET (sock), -1);

    priv = sock->priv;

#if HAVE_GNUTLS
    if (priv->session && gnutls_record_check_pending (priv->session))
        return TRUE;
#endif

    memset (&fds, 0, sizeof (fds));
    fds.fd = priv->sockfd;
    fds.events = G_IO_IN;

    n = g_poll (&fds, 1, NET_TIMEOUT * 1000);

    if (n > 0)
    {
        if (fds.revents == G_IO_IN)
            return TRUE;

        n = SOCKET_ERROR;
    }

    /* n == 0 means timeout */
    if (n == SOCKET_ERROR && error)
    {
        *error = g_error_new (MAILTC_SOCKET_ERROR,
                              MAILTC_SOCKET_ERROR_POLL,
                              "poll error on socket.");
    }
    return FALSE;
}

gssize
mailtc_socket_read (MailtcSocket* sock,
                    gchar*        buf,
                    gsize         len,
                    GError**      error)
{
    MailtcSocketPrivate* priv;
    gssize bytes;

    g_return_val_if_fail (MAILTC_IS_SOCKET (sock), -1);

    priv = sock->priv;

#if HAVE_GNUTLS
    if (priv->session)
    {
        if ((bytes = gnutls_record_recv (priv->session, buf, len)) < 0)
        {
            bytes = SOCKET_ERROR;
            if (error)
            {
                *error = g_error_new (MAILTC_SOCKET_ERROR,
                                      MAILTC_SOCKET_ERROR_SSL_READ,
                                      "Error reading from socket: %s",
                                      gnutls_strerror ((int) bytes));
            }
        }
    }
    else
#endif
    {
        bytes = g_socket_receive (priv->gsocket, buf, len, NULL, error);
    }

    if (bytes == SOCKET_ERROR)
        return -1;

    *(buf + bytes) = '\0';
    return bytes;
}

gssize
mailtc_socket_write (MailtcSocket* sock,
                     const gchar*  buf,
                     gsize         len,
                     GError**      error)
{
    MailtcSocketPrivate* priv;
    gssize bytes;

    g_return_val_if_fail (MAILTC_IS_SOCKET (sock), -1);

    priv = sock->priv;

#if HAVE_GNUTLS
    if (priv->session)
    {
        if ((bytes = gnutls_record_send (priv->session, buf, len)) < 0)
        {
            bytes = SOCKET_ERROR;
            if (error)
            {
                *error = g_error_new (MAILTC_SOCKET_ERROR,
                                      MAILTC_SOCKET_ERROR_SSL_WRITE,
                                      "Error writing to socket: %s",
                                      gnutls_strerror ((int) bytes));
            }
        }
    }
    else
#endif
    {
        bytes = g_socket_send (priv->gsocket, buf, len, NULL, error);
    }

    return (bytes != SOCKET_ERROR) ? bytes : -1;
}

void
mailtc_socket_disconnect (MailtcSocket* sock)
{
    MailtcSocketPrivate* priv;

    g_return_if_fail (MAILTC_IS_SOCKET (sock));

    priv = sock->priv;

#if HAVE_GNUTLS
    mailtc_socket_ssl_disconnect (sock);
#endif

    if (G_IS_SOCKET_CONNECTION (priv->connection))
    {
        g_tcp_connection_set_graceful_disconnect (G_TCP_CONNECTION (priv->connection), TRUE);
        g_io_stream_close (G_IO_STREAM (priv->connection), NULL, NULL);

        if (G_IS_CANCELLABLE (priv->cancellable))
            g_object_unref (priv->cancellable);

        priv->cancellable = NULL;

        g_object_unref (priv->connection);

        priv->connection = NULL;
        priv->gsocket = NULL;
        priv->sockfd = INVALID_SOCKET;
    }
}

static gpointer
mailtc_socket_cancel_thread (gpointer data)
{
    GCancellable* cancellable;

    g_return_val_if_fail (G_IS_CANCELLABLE (data), NULL);

    cancellable = (GCancellable*) data;

    g_usleep (CONNECT_TIMEOUT * 1000 * 1000);

    g_cancellable_cancel (cancellable);
    g_object_unref (cancellable);

    return NULL;
}

gboolean
mailtc_socket_connect (MailtcSocket* sock,
                       gchar*        server,
                       guint         port,
                       GError**      error)
{
    MailtcSocketPrivate* priv;
    GSocket* gsocket;
    GSocketClient* client;
    GSocketConnection* connection;
    SOCKET sockfd;

    g_return_val_if_fail (MAILTC_IS_SOCKET (sock), FALSE);

    priv = sock->priv;
    mailtc_socket_disconnect (sock);

    client = g_socket_client_new ();

    if (g_thread_get_initialized ())
    {
        priv->cancellable = g_cancellable_new ();
        g_object_ref_sink (priv->cancellable);

        if (!g_thread_create (mailtc_socket_cancel_thread, priv->cancellable, FALSE, error))
        {
            g_object_unref (priv->cancellable);
            return FALSE;
        }
    }

    connection = g_socket_client_connect_to_host (client, server, port, priv->cancellable, error);
    g_object_unref (client);

    if (!connection)
        return FALSE;

    gsocket = g_socket_connection_get_socket (connection);

    /* we set blocking for now, but this might change */
    g_socket_set_blocking (gsocket, TRUE);

    /* sockfd is primarily for gnutls, until it is possible to use GIOStreams with it */
    sockfd = g_socket_get_fd (gsocket);

    priv->connection = connection;
    priv->gsocket = gsocket;
    priv->sockfd = sockfd;

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

