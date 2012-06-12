/* mtc-config.h
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

#ifndef __MAILTC_CONFIG_H__
#define __MAILTC_CONFIG_H__

#include "mtc.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _mtc_prefs
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

    GPtrArray* plugins;

    gulong     entry_insert_text_id;
    gulong     button_edit_columns_changed_id;
    gulong     button_edit_cursor_changed_id;
    gulong     button_remove_columns_changed_id;
    gulong     button_remove_cursor_changed_id;

} mtc_prefs;

GtkWidget*
mailtc_config_dialog (mtc_config* config,
                      GPtrArray*  plugins);

G_END_DECLS

#endif /* __MAILTC_CONFIG_H__ */

