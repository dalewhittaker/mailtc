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

#include "mailtc.h"
#include "mtc-net.h"
#include "mtc-socket.h"
#include "mtc-uid.h"

#include <config.h>

#define PLUGIN_NAME "POP"
#define PLUGIN_AUTHOR "Dale Whittaker ("PACKAGE_BUGREPORT")"
#define PLUGIN_DESCRIPTION "POP3 network plugin."

#define ACCOUNT_PRIVATE "account_private" /* FIXME this is temporary, use a property. */

#define MAXDATASIZE 256

typedef gboolean
(*MailtcPopReadFunc) (MailtcNetPrivate* priv,
                      GError**          error);

typedef gboolean
(*MailtcPopWriteFunc) (MailtcNetPrivate* priv,
                       GError**     error,
                       gchar*       buf,
                       ...);

typedef enum
{
    POP_CMD_NULL = 0,
    POP_CMD_USER,
    POP_CMD_PASS,
    POP_CMD_STAT,
    POP_CMD_QUIT
} MailtcPopCommand;

typedef enum
{
    POP_PROTOCOL = 0,
    POP_PROTOCOL_SSL,
    POP_N_PROTOCOLS
} MailtcPopProtocol;

struct _MailtcNetPrivate
{
    MailtcSocket* sock;
    GString* msg;
    gint64 total;
    gboolean debug;
};

struct _MailtcNet
{
    struct _MailtcExtension parent_instance;

    MailtcNetPrivate* priv;
};

struct _MailtcNetClass
{
    struct _MailtcExtensionClass parent_class;
};

MAILTC_DEFINE_EXTENSION (MailtcNet, mailtc_net)

static gboolean
mailtc_net_remove_account (MailtcNet* net,
                           GObject*   account)
{
    MailtcUidTable* uid_table;

    g_assert (MAILTC_IS_NET (net));
    g_assert (G_IS_OBJECT (account));

    uid_table = g_object_get_data (account, ACCOUNT_PRIVATE);

    if (MAILTC_IS_UID_TABLE (uid_table))
    {
        g_object_unref (MAILTC_UID_TABLE (uid_table));
        g_object_set_data (account, ACCOUNT_PRIVATE, NULL);
    }
    return TRUE;
}

static gboolean
mailtc_net_add_account (MailtcNet* net,
                        GObject*   account)
{
    MailtcUidTable* uid_table;
    guint port;
    gchar* filename;
    gchar* hash;
    const gchar* directory = NULL;
    const gchar* server = NULL;
    const gchar* user = NULL;

    g_assert (MAILTC_IS_NET (net));
    g_assert (G_IS_OBJECT (account));

    g_object_get (net, MAILTC_EXTENSION_PROPERTY_DIRECTORY, &directory, NULL);
    g_assert (directory);

    g_object_get (account, MAILTC_ACCOUNT_PROPERTY_SERVER, &server,
            MAILTC_ACCOUNT_PROPERTY_PORT, &port, MAILTC_ACCOUNT_PROPERTY_USER, &user, NULL);
    g_assert (server && user);

    uid_table = g_object_get_data (account, ACCOUNT_PRIVATE);
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
    g_object_set_data (account, ACCOUNT_PRIVATE, uid_table);

    return mailtc_uid_table_load (uid_table, NULL); /* FIXME GError */
}

static gboolean
mailtc_net_read_messages  (MailtcNet* net,
                           GObject*   account)
{
    MailtcUidTable* uid_table;

    g_assert (MAILTC_IS_NET (net));
    g_assert (G_IS_OBJECT (account));

    uid_table = g_object_get_data (account, ACCOUNT_PRIVATE);
    g_assert (MAILTC_IS_UID_TABLE (uid_table));

    return mailtc_uid_table_mark_read (uid_table, NULL); /* FIXME GError */
}

static gboolean
mailtc_net_pop_read (MailtcNetPrivate* priv,
                     GError**          error)
{
    GString* msg;
    gchar buf[MAXDATASIZE];
    gint bytes;
    gchar* endchars = "\r\n";
    guint endlen = 2;
    gboolean success = TRUE;

    g_assert (priv && priv->msg);

    msg = priv->msg;
    g_string_set_size (msg, 0);

    while ((bytes = mailtc_socket_read (priv->sock, buf, (sizeof (buf) - 1), error)) > 0)
    {
       msg = g_string_append (msg, buf);
       if (msg->len >= endlen &&
           !g_ascii_strncasecmp (msg->str + msg->len - endlen,
                                 endchars, endlen))
       {
           break;
       }
    }

    if (bytes == -1)
        success = FALSE;
    if (msg->str)
    {
        if (priv->debug)
            g_print ("%s", msg->str);

        if (success &&
            (!g_ascii_strncasecmp (msg->str, "-ERR", 4) ||
             (msg->len >= endlen && g_ascii_strncasecmp (msg->str + msg->len - endlen,
                                                         endchars, endlen) != 0)))
        {
            success = FALSE;
        }
    }

    return success;
}

static gboolean
mailtc_net_pop_statread (MailtcNetPrivate* priv,
                         GError**          error)
{
    gboolean success = FALSE;

    g_assert (priv);

    if (mailtc_net_pop_read (priv, error))
    {
        GString* msg = priv->msg;

        g_assert (msg);

        if (msg->str)
        {
            gchar** total;

            total = g_strsplit (msg->str, " ", 0);
            if (total)
            {
                if (total[0] && total[1] && total[2])
                {
                    gchar* stotal;

                    stotal = g_strdup (total[1]);
                    priv->total = g_ascii_strtoll (stotal, NULL, 10);
                    g_free (stotal);
                    success = TRUE;
                }
                g_strfreev (total);
            }
        }
    }
    return success;
}

static gboolean
mailtc_net_pop_passwrite (MailtcNetPrivate* priv,
                          GError**          error,
                          gchar*            buf,
                          ...)
{
    gssize len;
    va_list list;
    GString* msg;

    g_assert (priv && priv->msg);
    msg = priv->msg;

    va_start (list, buf);
    g_string_vprintf (msg, buf, list);
    va_end (list);

    if (priv->debug)
    {
        gchar* pass;

        pass = g_strdup (msg->str);
        g_strcanon (pass + 5, "*\r\n", '*');
        g_print ("%s", pass);
        g_free (pass);
    }

    len = mailtc_socket_write (priv->sock, msg->str, msg->len, error);

    return (len == -1) ? FALSE : TRUE;
}

static gboolean
mailtc_net_pop_write (MailtcNetPrivate* priv,
                      GError**          error,
                      gchar*            buf,
                      ...)
{
    GString* msg;
    gssize len;
    va_list list;

    g_assert (priv && priv->msg);
    msg = priv->msg;

    va_start (list, buf);
    g_string_vprintf (msg, buf, list);
    va_end (list);

    if (priv->debug)
        g_print ("%s", msg->str);

    len = mailtc_socket_write (priv->sock, msg->str, msg->len, error);

    return (len == -1) ? FALSE : TRUE;
}

static gboolean
mailtc_net_run_pop (MailtcNetPrivate* priv,
                    GObject*          account,
                    MailtcPopCommand  index,
                    GError**          error)
{
    g_assert (priv);

    if (G_IS_OBJECT (account))
    {
        MailtcPopWriteFunc pwrite;
        MailtcPopReadFunc pread;
        gchar* command;
        gpointer arg;
        gboolean success;

        switch (index)
        {
            case POP_CMD_NULL:
                command = NULL;
                arg = NULL;
                pread = mailtc_net_pop_read;
                pwrite = mailtc_net_pop_write;
                break;

            case POP_CMD_USER:
                command = "USER %s\r\n";
                g_object_get (account, MAILTC_ACCOUNT_PROPERTY_USER, &arg, NULL);
                pread = mailtc_net_pop_read;
                pwrite = mailtc_net_pop_write;
                break;

            case POP_CMD_PASS:
                command = "PASS %s\r\n";
                g_object_get (account, MAILTC_ACCOUNT_PROPERTY_PASSWORD, &arg, NULL);
                pread = mailtc_net_pop_read;
                pwrite = mailtc_net_pop_passwrite;
                break;

            case POP_CMD_STAT:
                command = "STAT\r\n";
                arg = NULL;
                pread = mailtc_net_pop_statread;
                pwrite = mailtc_net_pop_write;
                break;

            case POP_CMD_QUIT:
                command = "QUIT\r\n";
                arg = NULL;
                pread = mailtc_net_pop_read;
                pwrite = mailtc_net_pop_write;
                break;
            default:
                return FALSE;
        }

        success = TRUE;

        if (command)
        {
            if (arg)
            {
                success = (*pwrite) (priv, error, command, arg);
                g_free (arg);
            }
            else
                success = (*pwrite) (priv, error, command);
        }
        if (success)
            success = (*pread) (priv, error);
        if (!success || (error && *error))
        {
            mailtc_socket_disconnect (priv->sock);
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

static gint64
pop_calculate_new (MailtcNetPrivate* priv,
                   MailtcUidTable*   uid_table,
                   GError**          error)
{
    gchar* pstart;
    gint64 i;
    gint64 messages;
    gboolean success = TRUE;

    g_assert (priv);

    mailtc_uid_table_age (uid_table);

    if (!priv->total)
        return 0;

    for (i = 1; i <= priv->total; ++i)
    {
        success = FALSE;
        if (!mailtc_net_pop_write (priv, error, "UIDL %" G_GINT64_FORMAT "\r\n", i))
            break;

        if (mailtc_net_pop_read (priv, error))
        {
            GString* msg = priv->msg;

            g_assert (msg);

            if (msg->str)
            {
                if ((pstart = g_strrstr (msg->str, " ")) &&
                    (pstart < (msg->str + msg->len) - 1))
                {
                    mailtc_uid_table_add (uid_table, pstart + 1);
                    success = TRUE;
                }
            }
        }
        if (!success)
            break;
    }
    if (success)
        messages = mailtc_uid_table_remove_old (uid_table);
    else
    {
        messages = -1;
        mailtc_socket_disconnect (priv->sock);
    }
    priv->total = 0;

    return messages;
}

static gint64
mailtc_net_get_messages (MailtcNet* net,
                         GObject*   account,
                         gboolean   debug)
{
    MailtcNetPrivate* priv;
    MailtcSocket* sock;
    MailtcUidTable* uid_table;
    gchar* server;
    guint port;
    guint protocol;
    gint64 nmails;

    g_assert (MAILTC_IS_NET (net));
    g_assert (G_IS_OBJECT (account));

    g_object_get (account, MAILTC_ACCOUNT_PROPERTY_SERVER, &server,
            MAILTC_ACCOUNT_PROPERTY_PORT, &port, MAILTC_ACCOUNT_PROPERTY_PROTOCOL, &protocol, NULL);

    uid_table = g_object_get_data (account, ACCOUNT_PRIVATE);
    g_assert (MAILTC_IS_UID_TABLE (uid_table));

    priv = net->priv;
    priv->debug = debug;
    sock = priv->sock;

    mailtc_socket_set_tls (sock, protocol == POP_PROTOCOL_SSL ? TRUE : FALSE);

    /* FIXME GError for all of these functions */
    if (!mailtc_socket_connect (sock, server, port, NULL))
        return -1;
    if (!mailtc_net_run_pop (priv, account, POP_CMD_NULL, NULL))
        return -1;
    if (!mailtc_net_run_pop (priv, account, POP_CMD_USER, NULL))
        return -1;
    if (!mailtc_net_run_pop (priv, account, POP_CMD_PASS, NULL))
        return -1;
    if (!mailtc_net_run_pop (priv, account, POP_CMD_STAT, NULL))
        return -1;
    if ((nmails = pop_calculate_new (priv, uid_table, NULL)) == -1)
        return -1;
    if (!mailtc_net_run_pop (priv, account, POP_CMD_QUIT, NULL))
        return -1;

    mailtc_socket_disconnect (sock);

    return nmails;
}

static void
mailtc_net_finalize (GObject* object)
{
    MailtcNet* net = MAILTC_NET (object);

    g_string_free (net->priv->msg, TRUE);
    g_object_unref (net->priv->sock);

    G_OBJECT_CLASS (mailtc_net_parent_class)->finalize (object);
}

static void
mailtc_net_constructed (GObject* object)
{
    MailtcNet* net;
    MailtcNetPrivate* priv;

    net = MAILTC_NET (object);
    priv = net->priv;

    priv->sock = mailtc_socket_new ();
    priv->msg = g_string_new (NULL);

    g_signal_connect (net, MAILTC_EXTENSION_SIGNAL_ADD_ACCOUNT, G_CALLBACK (mailtc_net_add_account), NULL);
    g_signal_connect (net, MAILTC_EXTENSION_SIGNAL_REMOVE_ACCOUNT, G_CALLBACK (mailtc_net_remove_account), NULL);
    g_signal_connect (net, MAILTC_EXTENSION_SIGNAL_GET_MESSAGES, G_CALLBACK (mailtc_net_get_messages), NULL);
    g_signal_connect (net, MAILTC_EXTENSION_SIGNAL_READ_MESSAGES, G_CALLBACK (mailtc_net_read_messages), NULL);

    G_OBJECT_CLASS (mailtc_net_parent_class)->constructed (object);
}

static void
mailtc_net_class_init (MailtcNetClass* class)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->constructed = mailtc_net_constructed;
    gobject_class->finalize = mailtc_net_finalize;

    g_type_class_add_private (class, sizeof (MailtcNetPrivate));
}

static void
mailtc_net_init (MailtcNet* net)
{
    MailtcNetPrivate* priv;

    priv = net->priv = G_TYPE_INSTANCE_GET_PRIVATE (net, MAILTC_TYPE_NET, MailtcNetPrivate);
}

MailtcNet*
mailtc_net_new (void)
{
    MailtcNet* net;
    GArray* protocol_array;

    MailtcProtocol protocols[] =
    {
        { "POP",  110 },
        { "POPS", 995 }
    };

    protocol_array = g_array_new (TRUE, FALSE, sizeof (MailtcProtocol));
    g_array_append_val (protocol_array, protocols[0]);
    if (mailtc_socket_supports_tls ())
        g_array_append_val (protocol_array, protocols[1]);

    net = g_object_new (MAILTC_TYPE_NET,
                        MAILTC_EXTENSION_PROPERTY_COMPATIBILITY, VERSION,
                        MAILTC_EXTENSION_PROPERTY_NAME,          PLUGIN_NAME,
                        MAILTC_EXTENSION_PROPERTY_AUTHOR,        PLUGIN_AUTHOR,
                        MAILTC_EXTENSION_PROPERTY_DESCRIPTION,   PLUGIN_DESCRIPTION,
                        MAILTC_EXTENSION_PROPERTY_PROTOCOLS,     protocol_array,
                        NULL);

    g_array_unref (protocol_array);

    return net;
}

