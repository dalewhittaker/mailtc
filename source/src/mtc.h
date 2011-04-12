/* mtc.h
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

#ifndef __MAILTC_H__
#define __MAILTC_H__

#include <config.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef gboolean
(*add_account_func) (gconstpointer config,
                     gconstpointer account,
                     GError**      error);

typedef gboolean
(*remove_account_func) (gconstpointer account,
                        GError**      error);

typedef gint64
(*get_message_func) (gconstpointer config,
                     gconstpointer account,
                     GError**      error);

typedef gboolean
(*read_message_func) (gconstpointer config,
                      gconstpointer account,
                      GError**      error);

typedef void
(*terminate_func) (gpointer);

typedef struct
{
    gpointer            module;
    gpointer            priv;
    gchar*              directory;

    const gchar*        compatibility;
    gchar*              name;
    gchar*              author;
    gchar*              description;
    gchar**             protocols;
    guint*              ports;

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

typedef struct
{
    GtkWidget* dialog_config;
    GtkWidget* dialog_account;
    GtkWidget* dialog_plugin;
    GtkWidget* spin_interval;
    GtkWidget* entry_command;
    GtkWidget* envelope_config;
    GtkWidget* combo_errordlg;
    GtkWidget* spin_connections;
    GtkWidget* tree_view;
    GtkWidget* entry_name;
    GtkWidget* entry_server;
    GtkWidget* entry_port;
    GtkWidget* entry_user;
    GtkWidget* entry_password;
    GtkWidget* envelope_account;
    GtkWidget* combo_plugin;

    gulong     entry_insert_text_id;
    gulong     button_edit_columns_changed_id;
    gulong     button_edit_cursor_changed_id;
    gulong     button_remove_columns_changed_id;
    gulong     button_remove_cursor_changed_id;

} mtc_prefs;

typedef struct
{
    gchar*      directory;
    GIOChannel* log;
    guint       source_id;
    mtc_prefs*  prefs;
    GObject*    status_icon;

    gboolean    locked;
    gint        interval;
    gchar*      mail_command;
    GdkColor*   icon_colour;
    gint        net_error;
    gint        error_count;
    gboolean    debug;

    GSList*     accounts;
    GSList*     plugins;

} mtc_config;

G_END_DECLS

#endif /* __MAILTC_H__ */

