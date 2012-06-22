/* mtc.h
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

#ifndef __MAILTC_H__
#define __MAILTC_H__

#include <config.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gdk/gdk.h> /* GdkColor */

G_BEGIN_DECLS

typedef gboolean
(*add_account_func)    (gconstpointer account,
                        GError**      error);

typedef gboolean
(*remove_account_func) (gconstpointer account,
                        GError**      error);

typedef gint64
(*get_message_func)    (gconstpointer account,
                        gboolean      debug,
                        GError**      error);

typedef gboolean
(*read_message_func)   (gconstpointer account,
                        GError**      error);

typedef void
(*terminate_func)      (gpointer plugin);

typedef struct
{
    gchar* name;
    guint  port;

} mtc_protocol;

typedef struct
{
    gpointer            module;
    gpointer            priv;
    gchar*              directory;

    const gchar*        compatibility;
    gchar*              name;
    gchar*              author;
    gchar*              description;

    GArray*             protocols;

    add_account_func    add_account;
    remove_account_func remove_account;
    get_message_func    get_messages;
    read_message_func   read_messages;
    terminate_func      terminate;

} mtc_plugin;

typedef struct
{
    gpointer    priv;
    gchar*      name;
    gchar*      server;
    guint       port;
    gchar*      user;
    gchar*      password;
    GdkColor*   icon_colour;
    mtc_plugin* plugin;
    guint       protocol;

} mtc_account;

G_END_DECLS

#endif /* __MAILTC_H__ */

