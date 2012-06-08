/* mtc-pop.c
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

#include <config.h>
#include "mtc.h"
#include "mtc-socket.h"
#include "mtc-uid.h"
#include <gmodule.h>

#define PLUGIN_NAME "POP"
#define PLUGIN_AUTHOR "Dale Whittaker ("PACKAGE_BUGREPORT")"
#define PLUGIN_DESCRIPTION "POP3 network plugin."

#define MAXDATASIZE 256

typedef enum
{
    POP_CMD_NULL = 0,
    POP_CMD_USER,
    POP_CMD_PASS,
    POP_CMD_STAT,
    POP_CMD_QUIT
} pop_command;

typedef enum
{
    POP_PROTOCOL = 0,
    POP_PROTOCOL_SSL,
    POP_N_PROTOCOLS
} pop_protocol;

typedef struct _pop_private pop_private;

typedef gboolean
(*pop_read_func) (pop_private* priv,
                  GError**     error);

typedef gboolean
(*pop_write_func) (pop_private* priv,
                   GError**     error,
                   gchar*       buf,
                   ...);

struct _pop_private
{
    MailtcSocket* sock;
    mtc_account* account;
    GString* msg;
    gboolean debug;
    gint64 total;
};

static gboolean
pop_read (pop_private* priv,
          GError**     error)
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
pop_statread (pop_private* priv,
              GError**     error)
{
    gboolean success = FALSE;

    g_assert (priv);

    if (pop_read (priv, error))
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
pop_write (pop_private*  priv,
           GError**      error,
           gchar*        buf,
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
pop_passwrite (pop_private*  priv,
               GError**      error,
               gchar*        buf,
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
pop_run (pop_private* priv,
         pop_command  index,
         GError**     error)
{
    mtc_account* account;

    g_assert (priv);

    account = priv->account;

    if (account)
    {
        gboolean success;
        gchar* command;
        gpointer arg;
        pop_write_func pwrite;
        pop_read_func pread;

        switch (index)
        {
            case POP_CMD_NULL:
                command = NULL;
                arg = NULL;
                pread = pop_read;
                pwrite = pop_write;
                break;
            case POP_CMD_USER:
                command = "USER %s\r\n";
                arg = account->user;
                pread = pop_read;
                pwrite = pop_write;
                break;
            case POP_CMD_PASS:
                command = "PASS %s\r\n";
                arg = account->password;
                pread = pop_read;
                pwrite = pop_passwrite;
                break;
            case POP_CMD_STAT:
                command = "STAT\r\n";
                arg = NULL;
                pread = pop_statread;
                pwrite = pop_write;
                break;
            case POP_CMD_QUIT:
                command = "QUIT\r\n";
                arg = NULL;
                pread = pop_read;
                pwrite = pop_write;
                break;
            default:
                return FALSE;
        }

        success = TRUE;

        if (command)
        {
            if (arg)
                success = (*pwrite) (priv, error, command, arg);
            else
                success = (*pwrite) (priv, error, command);
        }
        if (success)
            success = (*pread) (priv, error);

        if (!success || *error)
        {
            mailtc_socket_disconnect (priv->sock);
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

static gint64
pop_calculate_new (pop_private*    priv,
                   MailtcUidTable* uid_table,
                   GError**        error)
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
        if (!pop_write (priv, error, "UIDL %" G_GINT64_FORMAT "\r\n", i))
            break;

        if (pop_read (priv, error))
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
pop_get_messages (mtc_config*  config,
                  mtc_account* account,
                  GError**     error)
{
    MailtcSocket* sock;
    mtc_plugin* plugin;
    pop_private* priv;
    gint64 nmails;

    g_assert (account && account->plugin);
    g_assert (MAILTC_IS_UID_TABLE (account->priv));
    plugin = account->plugin;
    g_assert (plugin->priv);

    priv = (pop_private*) plugin->priv;
    priv->debug = config->debug;
    priv->account = account;
    sock = priv->sock;

    mailtc_socket_set_tls (sock, account->protocol == POP_PROTOCOL_SSL ? TRUE : FALSE);

    if (!mailtc_socket_connect (sock, account->server, account->port, error))
        return -1;
    if (!pop_run (priv, POP_CMD_NULL, error))
        return -1;
    if (!pop_run (priv, POP_CMD_USER, error))
        return -1;
    if (!pop_run (priv, POP_CMD_PASS, error))
        return -1;
    if (!pop_run (priv, POP_CMD_STAT, error))
        return -1;
    if ((nmails = pop_calculate_new (priv, MAILTC_UID_TABLE (account->priv), error)) == -1)
        return -1;
    if (!pop_run (priv, POP_CMD_QUIT, error))
        return -1;

    mailtc_socket_disconnect (sock);
    return nmails;
}

static gboolean
pop_read_messages (mtc_config*  config,
                   mtc_account* account,
                   GError**     error)
{
    (void) config;
    g_assert (account);
    g_assert (account && MAILTC_IS_UID_TABLE (account->priv));

    return mailtc_uid_table_mark_read (MAILTC_UID_TABLE (account->priv), error);
}

static gboolean
pop_remove_account (mtc_account* account,
                    GError**     error)
{
    (void) error;
    g_assert (account && account->plugin);

    if (account->priv && MAILTC_IS_UID_TABLE (account->priv))
    {
        g_object_unref (MAILTC_UID_TABLE (account->priv));
        account->priv = NULL;
    }
    return TRUE;
}

static gboolean
pop_add_account (mtc_config*  config,
                 mtc_account* account,
                 GError**     error)
{
    mtc_plugin* plugin;
    MailtcUidTable* uid_table;
    gchar* hash;
    gchar* filename;

    (void) config;
    g_assert (account && account->plugin);

    if (account->priv)
    {
        if (!pop_remove_account (account, error))
            return FALSE;
    }

    plugin = account->plugin;
    filename = g_strdup_printf ("%s%u%s", account->server, account->port, account->user);
    hash = g_strdup_printf ("%x", g_str_hash (filename));
    g_free (filename);
    filename = g_build_filename (plugin->directory, hash, NULL);
    g_free (hash);

    uid_table = mailtc_uid_table_new (filename);
    g_free (filename);
    account->priv = (gpointer) uid_table;

    return mailtc_uid_table_load (uid_table, error);
}


static void
pop_terminate (mtc_plugin* plugin)
{
    pop_private* priv;

    g_assert (plugin && plugin->priv);

    priv = (pop_private*) plugin->priv;
    g_string_free (priv->msg, TRUE);
    g_object_unref (priv->sock);
    g_free (priv);
    plugin->priv = NULL;
}

G_MODULE_EXPORT mtc_plugin*
plugin_init (void)
{
    mtc_plugin* plugin;
    pop_private* priv;
    mtc_protocol protocols[] =
    {
        { "POP",  110 },
        { "POPS", 995 }
    };

    plugin = g_new0 (mtc_plugin, 1);
    plugin->compatibility = VERSION;
    plugin->name = PLUGIN_NAME;
    plugin->author = PLUGIN_AUTHOR;
    plugin->description = PLUGIN_DESCRIPTION;
    plugin->add_account = (add_account_func) pop_add_account;
    plugin->remove_account = (remove_account_func) pop_remove_account;
    plugin->get_messages = (get_message_func) pop_get_messages;
    plugin->read_messages = (read_message_func) pop_read_messages;
    plugin->terminate = (terminate_func) pop_terminate;

    plugin->protocols = g_array_new (TRUE, FALSE, sizeof (mtc_protocol));
    g_array_append_val (plugin->protocols, protocols[0]);
    if (mailtc_socket_supports_tls ())
        g_array_append_val (plugin->protocols, protocols[1]);

    priv = (pop_private*) g_new0 (pop_private, 1);
    priv->sock = mailtc_socket_new ();
    priv->msg = g_string_new (NULL);
    plugin->priv = priv;

    return plugin;
}

