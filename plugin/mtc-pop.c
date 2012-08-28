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

#include "mtc-net.h"

#include <config.h>
#include <gmodule.h>

#define MAILTC_TYPE_POP            (mailtc_pop_get_type  ())
#define MAILTC_POP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILTC_TYPE_POP, MailtcPop))
#define MAILTC_POP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MAILTC_TYPE_POP, MailtcPopClass))
#define MAILTC_IS_POP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILTC_TYPE_POP))
#define MAILTC_IS_POP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAILTC_TYPE_POP))
#define MAILTC_POP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAILTC_TYPE_POP, MailtcPopClass))

#define EXTENSION_NAME        "POP"
#define EXTENSION_AUTHOR      "Dale Whittaker ("PACKAGE_BUGREPORT")"
#define EXTENSION_DESCRIPTION "POP3 network extension."

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

typedef struct
{
    MailtcNet parent_instance;

    GString* msg;
    gboolean debug;
    gint64 total;
} MailtcPop;

typedef struct
{
    MailtcNetClass parent_class;
} MailtcPopClass;

typedef gboolean
(*MailtcPopReadFunc) (MailtcPop* pop,
                      GError**   error);

typedef gboolean
(*MailtcPopWriteFunc) (MailtcPop* priv,
                       GError**   error,
                       gchar*     buf,
                       ...);

G_DEFINE_TYPE (MailtcPop, mailtc_pop, MAILTC_TYPE_NET)

static gboolean
mailtc_pop_read (MailtcPop* pop,
                 GError**   error)
{
    GString* msg;
    gssize bytes;
    gchar* endchars = "\r\n";
    guint endlen = 2;

    g_assert (MAILTC_IS_POP (pop) && pop->msg);

    msg = pop->msg;

    bytes = mailtc_net_read (MAILTC_NET (pop), msg, pop->debug, endchars, endlen, error);

    if (bytes == -1 || !g_ascii_strncasecmp (msg->str, "-ERR", 4) ||
        (msg->len >= endlen && g_ascii_strncasecmp (msg->str + msg->len - endlen, endchars, endlen) != 0))
    {
        return FALSE;
    }
    return TRUE;
}

static gboolean
mailtc_pop_statread (MailtcPop* pop,
                     GError**   error)
{
    gboolean success = FALSE;

    g_assert (MAILTC_IS_POP (pop) && pop->msg);

    if (mailtc_pop_read (pop, error))
    {
        GString* msg = pop->msg;

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
                    pop->total = g_ascii_strtoll (stotal, NULL, 10);
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
mailtc_pop_passwrite (MailtcPop* pop,
                      GError**   error,
                      gchar*     buf,
                      ...)
{
    gssize len;
    va_list list;

    g_assert (MAILTC_IS_POP (pop) && pop->msg);

    va_start (list, buf);
    g_string_vprintf (pop->msg, buf, list);
    va_end (list);

    if (pop->debug)
    {
        gchar* pass;

        pass = g_strdup (pop->msg->str);
        g_strcanon (pass + 5, "*\r\n", '*');
        g_print ("%s", pass);
        g_free (pass);
    }

    len = mailtc_net_write (MAILTC_NET (pop), pop->msg, FALSE, error);

    return (len == -1) ? FALSE : TRUE;
}

static gboolean
mailtc_pop_write (MailtcPop* pop,
                  GError**   error,
                  gchar*     buf,
                  ...)
{
    gssize len;
    va_list list;

    g_assert (MAILTC_IS_POP (pop) && pop->msg);

    va_start (list, buf);
    g_string_vprintf (pop->msg, buf, list);
    va_end (list);

    len = mailtc_net_write (MAILTC_NET (pop), pop->msg, pop->debug, error);

    return (len == -1) ? FALSE : TRUE;
}

static gboolean
mailtc_pop_run (MailtcPop*        pop,
                GObject*          account,
                MailtcPopCommand  index,
                GError**          error)
{
    MailtcPopWriteFunc pwrite;
    MailtcPopReadFunc pread;
    gchar* command;
    gpointer arg;
    gboolean success;

    g_assert (MAILTC_IS_POP (pop));

    if (!G_IS_OBJECT (account))
        return FALSE;

    switch (index)
    {
        case POP_CMD_NULL:
            command = NULL;
            arg = NULL;
            pread = mailtc_pop_read;
            pwrite = mailtc_pop_write;
            break;

        case POP_CMD_USER:
            command = "USER %s\r\n";
            MAILTC_ACCOUNT_GET_USER (account, arg);
            pread = mailtc_pop_read;
            pwrite = mailtc_pop_write;
            break;

        case POP_CMD_PASS:
            command = "PASS %s\r\n";
            MAILTC_ACCOUNT_GET_PASSWORD (account, arg);
            pread = mailtc_pop_read;
            pwrite = mailtc_pop_passwrite;
            break;

        case POP_CMD_STAT:
            command = "STAT\r\n";
            arg = NULL;
            pread = mailtc_pop_statread;
            pwrite = mailtc_pop_write;
            break;

        case POP_CMD_QUIT:
            command = "QUIT\r\n";
            arg = NULL;
            pread = mailtc_pop_read;
            pwrite = mailtc_pop_write;
            break;

        default:
            return FALSE;
    }

    success = TRUE;

    if (command)
    {
        if (arg)
        {
            success = (*pwrite) (pop, error, command, arg);
            g_free (arg);
        }
        else
            success = (*pwrite) (pop, error, command);
    }
    if (success)
        success = (*pread) (pop, error);

    if (!success || (error && *error))
    {
        mailtc_net_disconnect (MAILTC_NET (pop));
        return FALSE;
    }
    return TRUE;
}

static gint64
mailtc_pop_calculate_new (MailtcPop*      pop,
                          MailtcUidTable* uid_table,
                          GError**        error)
{
    gchar* pstart;
    gint64 i;
    gint64 messages;
    gboolean success = TRUE;

    g_assert (MAILTC_IS_POP (pop));

    mailtc_uid_table_age (uid_table);

    if (!pop->total)
        return 0;

    for (i = 1; i <= pop->total; ++i)
    {
        success = FALSE;
        if (!mailtc_pop_write (pop, error, "UIDL %" G_GINT64_FORMAT "\r\n", i))
            break;

        if (mailtc_pop_read (pop, error))
        {
            GString* msg = pop->msg;

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
        mailtc_net_disconnect (MAILTC_NET (pop));
    }
    pop->total = 0;

    return messages;
}

static gint64
mailtc_pop_get_messages (MailtcPop* pop,
                         GObject*   account,
                         gboolean   debug,
                         GError**   error)
{
    guint port;
    guint protocol;
    gint64 nmails;
    gboolean tls;
    gboolean success;
    gchar* server = NULL;
    MailtcUidTable* uid_table = NULL;

    g_assert (MAILTC_IS_POP (pop));
    g_assert (G_IS_OBJECT (account));

    g_object_get (account,
                  MAILTC_ACCOUNT_PROPERTY_SERVER, &server,
                  MAILTC_ACCOUNT_PROPERTY_PORT, &port,
                  MAILTC_ACCOUNT_PROPERTY_PROTOCOL, &protocol,
                  MAILTC_ACCOUNT_PROPERTY_PRIVATE, &uid_table,
                  NULL);

    g_assert (MAILTC_IS_UID_TABLE (uid_table));

    pop->debug = debug;

    tls = protocol == POP_PROTOCOL_SSL ? TRUE : FALSE;

    success = mailtc_net_connect (MAILTC_NET (pop), server, port, tls, error);
    g_free (server);

    if (!success)
        return -1;

    if (!mailtc_pop_run (pop, account, POP_CMD_NULL, error))
        return -1;
    if (!mailtc_pop_run (pop, account, POP_CMD_USER, error))
        return -1;
    if (!mailtc_pop_run (pop, account, POP_CMD_PASS, error))
        return -1;
    if (!mailtc_pop_run (pop, account, POP_CMD_STAT, error))
        return -1;
    if ((nmails = mailtc_pop_calculate_new (pop, uid_table, error)) == -1)
        return -1;
    if (!mailtc_pop_run (pop, account, POP_CMD_QUIT, error))
        return -1;

    mailtc_net_disconnect (MAILTC_NET (pop));

    return nmails;
}

static void
mailtc_pop_finalize (GObject* object)
{
    MailtcPop* pop = MAILTC_POP (object);

    if (pop->msg)
        g_string_free (pop->msg, TRUE);

    G_OBJECT_CLASS (mailtc_pop_parent_class)->finalize (object);
}

static void
mailtc_pop_constructed (GObject* object)
{
    MailtcPop* pop = MAILTC_POP (object);

    pop->msg = g_string_new (NULL);

    G_OBJECT_CLASS (mailtc_pop_parent_class)->constructed (object);
}

static void
mailtc_pop_class_init (MailtcPopClass* klass)
{
    GObjectClass* gobject_class;
    MailtcExtensionClass* extension_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->constructed = mailtc_pop_constructed;
    gobject_class->finalize = mailtc_pop_finalize;

    extension_class = MAILTC_BASE_EXTENSION_CLASS (klass);
    extension_class->get_messages = (MailtcExtensionGetMessagesFunc) mailtc_pop_get_messages;
}

static void
mailtc_pop_init (MailtcPop* pop)
{
    pop->msg = NULL;
    pop->debug = FALSE;
}

static MailtcPop*
mailtc_pop_new (const gchar* directory)
{
    MailtcPop* pop;
    GArray* protocol_array;

    MailtcProtocol protocols[] =
    {
        { "POP",  110 },
        { "POPS", 995 }
    };

    protocol_array = g_array_new (TRUE, FALSE, sizeof (MailtcProtocol));
    g_array_append_val (protocol_array, protocols[0]);
    if (mailtc_net_supports_tls ())
        g_array_append_val (protocol_array, protocols[1]);

    pop = g_object_new (MAILTC_TYPE_POP,
                        MAILTC_EXTENSION_PROPERTY_DIRECTORY,     directory,
                        MAILTC_EXTENSION_PROPERTY_COMPATIBILITY, VERSION,
                        MAILTC_EXTENSION_PROPERTY_NAME,          EXTENSION_NAME,
                        MAILTC_EXTENSION_PROPERTY_AUTHOR,        EXTENSION_AUTHOR,
                        MAILTC_EXTENSION_PROPERTY_DESCRIPTION,   EXTENSION_DESCRIPTION,
                        MAILTC_EXTENSION_PROPERTY_PROTOCOLS,     protocol_array,
                        NULL);

    g_array_unref (protocol_array);

    return pop;
}

G_MODULE_EXPORT GSList*
extension_init (const gchar* directory)
{
    return g_slist_append (NULL, mailtc_pop_new (directory));
}

