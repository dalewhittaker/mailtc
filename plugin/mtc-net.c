/* mtc-net.c
 * Copyright (C) 2009-2012 Dale Whittaker <dayul@users.sf.net>
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

#include "mtc-net.h"
#include "mtc-socket.h"

#define MAXDATASIZE 256

struct _MailtcNetPrivate
{
    MailtcSocket* sock;
};

MAILTC_DEFINE_EXTENSION (MailtcNet, mailtc_net)

static gboolean
mailtc_net_remove_account (MailtcNet* net,
                           GObject*   account)
{
    MailtcUidTable* uid_table = NULL;

    g_assert (MAILTC_IS_NET (net));
    g_assert (G_IS_OBJECT (account));

    MAILTC_ACCOUNT_GET_PRIVATE (account, uid_table);

    if (MAILTC_IS_UID_TABLE (uid_table))
    {
        g_object_unref (MAILTC_UID_TABLE (uid_table));
        uid_table = NULL;
        MAILTC_ACCOUNT_SET_PRIVATE (account, uid_table);
    }
    return TRUE;
}

static gboolean
mailtc_net_add_account (MailtcNet* net,
                        GObject*   account)
{
    guint port;
    gchar* filename;
    gchar* hash;
    const gchar* directory = NULL;
    const gchar* server = NULL;
    const gchar* user = NULL;
    MailtcUidTable* uid_table = NULL;

    g_assert (MAILTC_IS_NET (net));
    g_assert (G_IS_OBJECT (account));

    MAILTC_EXTENSION_GET_DIRECTORY (net, directory);
    g_assert (directory);

    g_object_get (account,
                  MAILTC_ACCOUNT_PROPERTY_SERVER, &server,
                  MAILTC_ACCOUNT_PROPERTY_PORT, &port,
                  MAILTC_ACCOUNT_PROPERTY_USER, &user,
                  MAILTC_ACCOUNT_PROPERTY_PRIVATE, &uid_table,
                  NULL);
    g_assert (server && user);

    if (MAILTC_IS_UID_TABLE (uid_table))
    {
        if (!mailtc_net_remove_account (net, account)) /* FIXME GError */
            return FALSE;
    }

    filename = g_strdup_printf ("%s%u%s", server, port, user);
    hash = g_strdup_printf ("%x", g_str_hash (filename));
    g_free (filename);

    filename = g_build_filename (directory, hash, NULL);
    g_free (hash);

    uid_table = mailtc_uid_table_new (filename);
    g_free (filename);
    MAILTC_ACCOUNT_SET_PRIVATE (account, uid_table);

    return mailtc_uid_table_load (uid_table, NULL); /* FIXME GError */
}

static gboolean
mailtc_net_read_messages (MailtcNet* net,
                          GObject*   account)
{
    MailtcUidTable* uid_table = NULL;

    g_assert (MAILTC_IS_NET (net));
    g_assert (G_IS_OBJECT (account));

    MAILTC_ACCOUNT_GET_PRIVATE (account, uid_table);

    return mailtc_uid_table_mark_read (uid_table, NULL); /* FIXME GError */
}

gssize
mailtc_net_read (MailtcNet*   net,
                 GString*     msg,
                 gboolean     debug,
                 const gchar* endchars,
                 guint        endlen,
                 GError**     error)
{
    gchar buf[MAXDATASIZE];
    gssize bytes;

    g_assert (MAILTC_IS_NET (net) && net->priv && net->priv->sock);
    g_assert (msg);

    g_string_set_size (msg, 0);

    while ((bytes = mailtc_socket_read (net->priv->sock, buf, (sizeof (buf) - 1), error)) > 0)
    {
        msg = g_string_append (msg, buf);
        if (msg->len >= endlen &&
            !g_ascii_strncasecmp (msg->str + msg->len - endlen,
                                  endchars, endlen))
        {
            break;
        }
    }
    if (debug && msg->str)
        g_print ("%s", msg->str);

    return bytes;
}

gssize
mailtc_net_write (MailtcNet* net,
                  GString*   msg,
                  gboolean   debug,
                  GError**   error)
{
    g_assert (MAILTC_IS_NET (net) && net->priv && net->priv->sock);
    g_assert (msg);

    if (msg->len == 0)
        return msg->len;

    if (debug)
        g_print ("%s", msg->str);

    return mailtc_socket_write (net->priv->sock, msg->str, msg->len, error);
}

void
mailtc_net_disconnect (MailtcNet* net)
{
    g_assert (MAILTC_IS_NET (net) && net->priv && net->priv->sock);

    mailtc_socket_disconnect (net->priv->sock);
}

gboolean
mailtc_net_connect (MailtcNet*   net,
                    const gchar* server,
                    guint        port,
                    gboolean     tls,
                    GError**     error)
{
    g_assert (MAILTC_IS_NET (net) && net->priv && net->priv->sock);

    mailtc_socket_set_tls (net->priv->sock, tls);

    return mailtc_socket_connect (net->priv->sock, server, port, error);
}

gboolean
mailtc_net_supports_tls (void)
{
    return mailtc_socket_supports_tls ();
}

static void
mailtc_net_finalize (GObject* object)
{
    MailtcNet* net = MAILTC_NET (object);

    if (net->priv->sock)
        g_object_unref (net->priv->sock);

    G_OBJECT_CLASS (mailtc_net_parent_class)->finalize (object);
}

static void
mailtc_net_constructed (GObject* object)
{
    MailtcNet* net = MAILTC_NET (object);

    net->priv->sock = mailtc_socket_new ();

    g_signal_connect (net, MAILTC_EXTENSION_SIGNAL_ADD_ACCOUNT, G_CALLBACK (mailtc_net_add_account), NULL);
    g_signal_connect (net, MAILTC_EXTENSION_SIGNAL_REMOVE_ACCOUNT, G_CALLBACK (mailtc_net_remove_account), NULL);
    g_signal_connect (net, MAILTC_EXTENSION_SIGNAL_READ_MESSAGES, G_CALLBACK (mailtc_net_read_messages), NULL);

    G_OBJECT_CLASS (mailtc_net_parent_class)->constructed (object);
}

static void
mailtc_net_class_init (MailtcNetClass* klass)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->constructed = mailtc_net_constructed;
    gobject_class->finalize = mailtc_net_finalize;

    g_type_class_add_private (klass, sizeof (MailtcNetPrivate));
}

static void
mailtc_net_init (MailtcNet* net)
{
    net->priv = G_TYPE_INSTANCE_GET_PRIVATE (net, MAILTC_TYPE_NET, MailtcNetPrivate);
    net->priv->sock = NULL;
}

MailtcNet*
mailtc_net_new (void)
{
    return g_object_new (MAILTC_TYPE_NET, NULL);
}

