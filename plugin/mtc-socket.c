/* mtc-socket.c
 * Copyright (C) 2009-2015 Dale Whittaker <dayul@users.sf.net>
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

#include <config.h>
#include "mtc-socket.h"

#include <gio/gio.h>

#define TIMEOUT_NET 5
#define TIMEOUT_FREQ 100

#define MAILTC_SOCKET_ERROR g_quark_from_string ("MAILTC_SOCKET_ERROR")

typedef enum
{
    MAILTC_SOCKET_ERROR_READ = 0,
    MAILTC_SOCKET_ERROR_WRITE
} MailtcSocketError;

typedef struct
{
    GIOStream* connection;
    GPollableInputStream* istream;
    GPollableOutputStream* ostream;
} MailtcSocketPrivate;

struct _MailtcSocket
{
    GObject parent_instance;

    MailtcSocketPrivate* priv;
    gboolean tls;
};

struct _MailtcSocketClass
{
    GObjectClass parent_class;
};

G_DEFINE_TYPE_WITH_PRIVATE (MailtcSocket, mailtc_socket, G_TYPE_OBJECT)

enum
{
    PROP_0,
    PROP_TLS
};

static void
mailtc_socket_finalize (GObject* object)
{
    MailtcSocket* sock = MAILTC_SOCKET (object);

    mailtc_socket_disconnect (sock);

    G_OBJECT_CLASS (mailtc_socket_parent_class)->finalize (object);
}

gboolean
mailtc_socket_supports_tls (void)
{
    return g_tls_backend_supports_tls (g_tls_backend_get_default ());
}

void
mailtc_socket_set_tls (MailtcSocket* sock,
                       gboolean      tls)
{
    g_assert (MAILTC_IS_SOCKET (sock));

    if (sock->tls != tls)
    {
        sock->tls = tls;
        g_object_notify (G_OBJECT (sock), "tls");
    }
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
        case PROP_TLS:
            mailtc_socket_set_tls (sock, g_value_get_boolean (value));
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
        case PROP_TLS:
            g_value_set_boolean (value, sock->tls);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_socket_class_init (MailtcSocketClass* klass)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = mailtc_socket_finalize;
    gobject_class->set_property = mailtc_socket_set_property;
    gobject_class->get_property = mailtc_socket_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_TLS,
                                     g_param_spec_boolean (
                                     "tls",
                                     "TLS",
                                     "Whether to enable TLS",
                                     FALSE,
                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));
}

static void
mailtc_socket_init (MailtcSocket* sock)
{
    MailtcSocketPrivate* priv;

    sock->priv = G_TYPE_INSTANCE_GET_PRIVATE (sock,
                 MAILTC_TYPE_SOCKET, MailtcSocketPrivate);
    priv = sock->priv;

    priv->connection = NULL;
    priv->istream = NULL;
    priv->ostream = NULL;
}

gssize
mailtc_socket_read (MailtcSocket* sock,
                    gchar*        buf,
                    gsize         len,
                    GError**      error)
{
    MailtcSocketPrivate* priv;
    gssize bytes;
    guint t;
    guint i = 0;
    GError* error_read = NULL;

    g_assert (MAILTC_IS_SOCKET (sock));

    priv = sock->priv;

    t = TIMEOUT_NET * TIMEOUT_FREQ;

    while (++i < t)
    {
        bytes = g_pollable_input_stream_read_nonblocking (
                        priv->istream, buf, len, NULL, &error_read);

        if (g_error_matches (error_read, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
        {
            g_clear_error (&error_read);
            g_usleep (G_USEC_PER_SEC / TIMEOUT_FREQ);
        }
        else
            break;
    }
    if (i == t)
    {
        if (error)
        {
            *error = g_error_new (MAILTC_SOCKET_ERROR,
                                  MAILTC_SOCKET_ERROR_READ,
                                  "Error reading from socket: timeout");
        }
        bytes = -1;
    }

    if (bytes != -1)
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
    guint t;
    guint i = 0;
    GError* error_write = NULL;

    g_assert (MAILTC_IS_SOCKET (sock));

    priv = sock->priv;

    t = TIMEOUT_NET * TIMEOUT_FREQ;

    while (++i < t)
    {
        bytes = g_pollable_output_stream_write_nonblocking (
                        priv->ostream, buf, len, NULL, &error_write);

        if (g_error_matches (error_write, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
        {
            g_clear_error (&error_write);
            g_usleep (G_USEC_PER_SEC / TIMEOUT_FREQ);
        }
        else
            break;
    }
    if (i == t)
    {
        if (error)
        {
            *error = g_error_new (MAILTC_SOCKET_ERROR,
                                  MAILTC_SOCKET_ERROR_WRITE,
                                  "Error writing to socket: timeout");
        }
        bytes = -1;
    }

    return bytes;
}

void
mailtc_socket_disconnect (MailtcSocket* sock)
{
    MailtcSocketPrivate* priv;

    g_assert (MAILTC_IS_SOCKET (sock));

    priv = sock->priv;

    if (G_IS_SOCKET_CONNECTION (priv->connection))
    {
        g_tcp_connection_set_graceful_disconnect (
                G_TCP_CONNECTION (priv->connection), TRUE);

        g_io_stream_close (priv->connection, NULL, NULL);
        g_object_unref (priv->connection);

        priv->istream = NULL;
        priv->ostream = NULL;
        priv->connection = NULL;
    }
}

gboolean
mailtc_socket_connect (MailtcSocket* sock,
                       const gchar*  server,
                       guint         port,
                       GError**      error)
{
    MailtcSocketPrivate* priv;
    GSocketClient* client;
    GSocketConnection* connection;
    GTlsCertificateFlags flags;

    g_assert (MAILTC_IS_SOCKET (sock));

    priv = sock->priv;
    mailtc_socket_disconnect (sock);

    client = g_socket_client_new ();

    g_socket_client_set_timeout (client, TIMEOUT_NET);
    g_socket_client_set_tls (client, sock->tls);

    /* Following flags are used to allow self-signed certificates */
    flags = g_socket_client_get_tls_validation_flags (client);
    flags &= ~(G_TLS_CERTIFICATE_UNKNOWN_CA |
               G_TLS_CERTIFICATE_NOT_ACTIVATED |
               /*G_TLS_CERTIFICATE_EXPIRED |*/
               G_TLS_CERTIFICATE_REVOKED |
               G_TLS_CERTIFICATE_INSECURE /*|
               G_TLS_CERTIFICATE_BAD_IDENTITY*/);
    flags = 0;
    g_socket_client_set_tls_validation_flags (client, flags);

    connection = g_socket_client_connect_to_host (
                        client, server, port, NULL, error);
    g_object_unref (client);

    if (!connection)
        return FALSE;

    priv->connection = G_IO_STREAM (connection);
    priv->istream = G_POLLABLE_INPUT_STREAM (
                        g_io_stream_get_input_stream (priv->connection));
    priv->ostream = G_POLLABLE_OUTPUT_STREAM (
                        g_io_stream_get_output_stream (priv->connection));

    return TRUE;
}

MailtcSocket*
mailtc_socket_new (void)
{
    return g_object_new (MAILTC_TYPE_SOCKET, NULL);
}

