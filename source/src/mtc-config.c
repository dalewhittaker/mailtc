/* mtc-config.c
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

#include "mtc-config.h"
#include "mtc-envelope.h"
#include "mtc-file.h"
#include "mtc-util.h"

enum
{
    TREEVIEW_ACCOUNT_COLUMN = 0,
    TREEVIEW_PROTOCOL_COLUMN,

    N_TREEVIEW_COLUMNS
};

enum
{
    COMBO_PROTOCOL_COLUMN = 0,
    COMBO_PLUGIN_COLUMN,
    COMBO_INDEX_COLUMN,

    N_COMBO_COLUMNS
};

static void
mailtc_config_dialog_destroy_cb (GtkWidget* widget)
{
    (void) widget;
    mailtc_info ("Now run " PACKAGE " to check mail.");
    mailtc_quit ();
}

static gboolean
mailtc_config_dialog_delete_event_cb (GtkWidget* widget,
                                      GdkEvent*  event)
{
    (void) widget;
    (void) event;
    return FALSE;
}

static void
mailtc_config_dialog_response_cb (GtkWidget*  dialog,
                                  gint        response_id,
                                  mtc_config* config)
{
    if (response_id == GTK_RESPONSE_OK)
    {
        mtc_prefs* prefs = config->prefs;

        if (prefs)
        {
            const gchar* mail_command;
            GError* error = NULL;

            config->interval = gtk_spin_button_get_value_as_int (
                                GTK_SPIN_BUTTON (prefs->spin_interval));

            mail_command = gtk_entry_get_text (GTK_ENTRY (prefs->entry_command));
            if (mail_command)
            {
                g_free (config->mail_command);
                config->mail_command = g_strdup (mail_command);
            }
            if (config->icon_colour)
                gdk_color_free (config->icon_colour);

            config->icon_colour = mailtc_envelope_get_envelope_colour (MAILTC_ENVELOPE (prefs->envelope_config));

            config->net_error = gtk_combo_box_get_active (GTK_COMBO_BOX (prefs->combo_errordlg));
            if (config->net_error > 1)
                config->net_error = gtk_spin_button_get_value_as_int (
                                    GTK_SPIN_BUTTON (prefs->spin_connections));
            if (!mailtc_save_config (config, &error))
                mailtc_gerror (&error);
        }
    }
    else if (response_id == GTK_RESPONSE_CLOSE)
        gtk_widget_destroy (dialog);
}

static void
mailtc_combo_errordlg_changed_cb (GtkComboBox* combo,
                                  GtkWidget*   widget)
{
    if (gtk_combo_box_get_active (combo) > 1)
        gtk_widget_show (widget);
    else
        gtk_widget_hide (widget);
}

static void
mailtc_button_icon_clicked_cb (GtkWidget*      button,
                               MailtcEnvelope* envelope)
{
    GtkWidget* dialog;
	GtkWidget *coloursel;
    GdkColor* colour;
	gint response;

    (void) button;

    dialog = gtk_color_selection_dialog_new ("Select Icon Colour");
    coloursel = gtk_color_selection_dialog_get_color_selection (GTK_COLOR_SELECTION_DIALOG (dialog));

    colour = mailtc_envelope_get_envelope_colour (envelope);
    gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (coloursel), colour);

    response = gtk_dialog_run (GTK_DIALOG (dialog));

    if (response == GTK_RESPONSE_OK)
    {
        gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (coloursel), colour);
        mailtc_envelope_set_envelope_colour (envelope, colour);
    }
    gtk_widget_destroy (dialog);
    gdk_color_free (colour);
}

static void
mailtc_button_plugin_clicked_cb (GtkWidget*  button,
                                 mtc_config* config)
{
    mtc_prefs* prefs;

    (void) button;

    g_return_if_fail (config && config->prefs);
    prefs = config->prefs;

    if (prefs)
    {
        GtkWidget* dialog_plugin;
        GtkTreeModel* model;
        GtkTreeIter iter;
        mtc_plugin* plugin;
        const gchar* authors[2];
        gint plgindex;
        gint index;
        gboolean exists;

        dialog_plugin = prefs->dialog_plugin;

        if (!dialog_plugin)
        {
            dialog_plugin = gtk_about_dialog_new ();

            g_signal_connect (dialog_plugin, "delete-event",
                    G_CALLBACK (gtk_widget_hide_on_delete), NULL);

            prefs->dialog_plugin = dialog_plugin;
        }

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (prefs->combo_plugin));
        g_assert (GTK_IS_TREE_MODEL (model));
        exists = gtk_combo_box_get_active_iter (GTK_COMBO_BOX  (prefs->combo_plugin), &iter);
        g_assert (exists);

        gtk_tree_model_get (model, &iter, COMBO_PLUGIN_COLUMN, &plgindex,
                            COMBO_INDEX_COLUMN, &index, -1);
        g_assert (index > -1);
        plugin = (mtc_plugin*) g_slist_nth_data (config->plugins, plgindex);
        g_assert (plugin);

        authors[0] = plugin->author;
        authors[1] = NULL;

        gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (dialog_plugin), plugin->name);
        gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (dialog_plugin), plugin->compatibility);
        gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (dialog_plugin), plugin->description);
        gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (dialog_plugin), authors);

        gtk_dialog_run (GTK_DIALOG (dialog_plugin));
        gtk_widget_hide (dialog_plugin);
    }
}

static GtkWidget*
mailtc_config_dialog_page_general (mtc_prefs* prefs)
{
    GtkWidget* table_general;
    GtkWidget* label_interval;
    GtkAdjustment* adj_interval;
    GtkWidget* spin_interval;
    GtkWidget* label_command;
    GtkWidget* entry_command;
    GtkWidget* label_errordlg;
    GtkWidget* combo_errordlg;
    GtkWidget* hbox_errordlg;
    GtkAdjustment* adj_connections;
    GtkWidget* spin_connections;
    GtkWidget* label_connections;
    GtkWidget* hbox_icon;
    GtkWidget* label_icon;
    GtkWidget* envelope_icon;
    GtkWidget* button_icon;
    GtkSizeGroup* label_group;
    GtkSizeGroup* label_input;
    GtkWidget* align;

    label_interval = gtk_label_new ("Interval in minutes for mail check:");
    adj_interval = (GtkAdjustment*)gtk_adjustment_new (1.0, 1.0, 60.0, 1.0, 5.0, 0.0);
    spin_interval = gtk_spin_button_new (GTK_ADJUSTMENT (adj_interval), 1.0, 0);

    label_command = gtk_label_new ("Mail reading command:");
    entry_command = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (entry_command), MAILTC_PATH_LENGTH);

    label_errordlg = gtk_label_new ("Show network error dialog:");
    combo_errordlg = gtk_combo_box_new_text ();
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo_errordlg), "Never");
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo_errordlg), "Always");
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo_errordlg), "Every");

    adj_connections = (GtkAdjustment*)gtk_adjustment_new (2.0, 2.0, 5.0, 1.0, 1.0, 0.0);
    spin_connections = gtk_spin_button_new (adj_connections, 1.0, 0);
    label_connections = gtk_label_new ("failed connections");
    hbox_errordlg = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox_errordlg), combo_errordlg, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox_errordlg), spin_connections, FALSE, FALSE, 10);
    gtk_box_pack_start (GTK_BOX (hbox_errordlg), label_connections, FALSE, FALSE, 0);

    label_icon = gtk_label_new ("Multiple icon colour:");
    envelope_icon = mailtc_envelope_new ();
    button_icon = gtk_button_new_with_label ("Select Colour...");
    hbox_icon = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox_icon), envelope_icon, FALSE, FALSE, 0);
    gtk_box_pack_end (GTK_BOX (hbox_icon), button_icon, FALSE, FALSE, 5);

    label_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    label_input = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

#define SIZE_GROUP_ADD(group, widget) \
    align = gtk_alignment_new (0, 0.5, 0, 0); \
    gtk_container_add (GTK_CONTAINER (align), widget); \
    gtk_size_group_add_widget (group, align);

    table_general = gtk_table_new (5, 2, FALSE);
    SIZE_GROUP_ADD (label_group, label_interval);
    gtk_table_attach (GTK_TABLE (table_general), align, 0, 1, 0, 1,
            GTK_SHRINK, GTK_EXPAND | GTK_FILL, 10, 2);
    SIZE_GROUP_ADD (label_input, spin_interval);
    gtk_table_attach (GTK_TABLE (table_general), align, 1, 2, 0, 1,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
    SIZE_GROUP_ADD (label_group, label_command);
    gtk_table_attach (GTK_TABLE (table_general), align, 0, 1, 1, 2,
            GTK_SHRINK, GTK_EXPAND | GTK_FILL, 10, 2);
    SIZE_GROUP_ADD (label_input, entry_command);
    gtk_table_attach (GTK_TABLE (table_general), align, 1, 2, 1, 2,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
    SIZE_GROUP_ADD (label_group, label_icon);
    gtk_table_attach (GTK_TABLE (table_general), align, 0, 1, 3, 4,
            GTK_SHRINK, GTK_EXPAND | GTK_FILL, 10, 2);
    SIZE_GROUP_ADD (label_input, hbox_icon);
    gtk_table_attach (GTK_TABLE (table_general), align, 1, 2, 3, 4,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
    SIZE_GROUP_ADD (label_group, label_errordlg);
    gtk_table_attach (GTK_TABLE (table_general), align, 0, 1, 4, 5,
            GTK_SHRINK, GTK_EXPAND | GTK_FILL, 10, 2);
    SIZE_GROUP_ADD (label_input, hbox_errordlg);
    gtk_table_attach (GTK_TABLE (table_general), align, 1, 2, 4, 5,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);

    g_object_unref (label_input);
    g_object_unref (label_group);

    gtk_container_set_border_width (GTK_CONTAINER (table_general), 4);

    g_signal_connect (combo_errordlg, "changed",
            G_CALLBACK (mailtc_combo_errordlg_changed_cb), label_connections);
    g_signal_connect (combo_errordlg, "changed",
            G_CALLBACK (mailtc_combo_errordlg_changed_cb), spin_connections);
    g_signal_connect (button_icon, "clicked",
            G_CALLBACK (mailtc_button_icon_clicked_cb), envelope_icon);

    prefs->spin_interval = spin_interval;
    prefs->entry_command = entry_command;
    prefs->envelope_config = envelope_icon;
    prefs->combo_errordlg = combo_errordlg;
    prefs->spin_connections = spin_connections;

    gtk_widget_show_all (table_general);

    return table_general;
}

static void
mailtc_account_update_tree_view (mtc_config* config,
                                 gint        index)
{
    GtkTreeModel* model;
    GtkTreeModel* combo_model;
    GtkTreeIter iter;
    GtkTreeIter combo_iter;
    mtc_prefs* prefs;
    mtc_account* account;
    gboolean exists;
    gint n;

    g_return_if_fail (config && config->prefs);
    g_assert (index > -1);

    prefs = config->prefs;
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (prefs->tree_view));
    n = gtk_tree_model_iter_n_children (model, NULL);
    if (index < n)
    {
        if (!gtk_tree_model_iter_nth_child (model, &iter, NULL, index))
        {
            mailtc_error ("Error getting tree view iter");
            return;
        }
    }
    else
        gtk_list_store_append (GTK_LIST_STORE (model), &iter);

    account = g_slist_nth_data (config->accounts, index);

    combo_model = gtk_combo_box_get_model (GTK_COMBO_BOX (prefs->combo_plugin));
    exists = gtk_combo_box_get_active_iter (GTK_COMBO_BOX  (prefs->combo_plugin), &combo_iter);
    g_assert (exists);

    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                        TREEVIEW_ACCOUNT_COLUMN, account->name,
                        TREEVIEW_PROTOCOL_COLUMN, account->plugin->protocols[account->protocol], -1);
}

static gint
mailtc_account_dialog_save (mtc_config*  config,
                            mtc_account* account,
                            GError**     error)
{
    mtc_prefs* prefs;
    mtc_plugin* plugin;
    GtkTreeModel* model;
    GtkTreeIter iter;
    const gchar* name;
    const gchar* server;
    const gchar* user;
    const gchar* password;
    const gchar* port;
    const gchar*msg;
    gboolean exists;
    gboolean empty;
    gboolean changed;
    gint active;
    guint iport;
    guint protocol;

    g_return_val_if_fail (config && config->prefs, FALSE);
    prefs = config->prefs;

    name = gtk_entry_get_text (GTK_ENTRY (prefs->entry_name));
    server = gtk_entry_get_text (GTK_ENTRY (prefs->entry_server));
    port = gtk_entry_get_text (GTK_ENTRY (prefs->entry_port));
    user = gtk_entry_get_text (GTK_ENTRY (prefs->entry_user));
    password = gtk_entry_get_text (GTK_ENTRY (prefs->entry_password));

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (prefs->combo_plugin));
    g_assert (GTK_IS_TREE_MODEL (model));
    exists = gtk_combo_box_get_active_iter (GTK_COMBO_BOX  (prefs->combo_plugin), &iter);
    g_assert (exists);

    gtk_tree_model_get (model, &iter, COMBO_PLUGIN_COLUMN, &active,
                        COMBO_INDEX_COLUMN, &protocol, -1);
    plugin = (mtc_plugin*) g_slist_nth_data (config->plugins, active);
    g_assert (plugin);

    empty = FALSE;
    changed = FALSE;

    if (g_str_equal (name, ""))
    {
        msg = "name";
        empty = TRUE;
    }
    else if (g_str_equal (server, ""))
    {
        msg = "server";
        empty = TRUE;
    }
    else if (g_str_equal (port, ""))
    {
        msg = "port";
        empty = TRUE;
    }
    else if (g_str_equal (user, ""))
    {
        msg = "user";
        empty = TRUE;
    }
    else if (g_str_equal (password, ""))
    {
        msg = "password";
        empty = TRUE;
    }

    if (empty)
    {
        mailtc_error ("You must enter a %s", msg);
        return -1;
    }

    if (!account)
    {
        account = g_new0 (mtc_account, 1);
        config->accounts = g_slist_append (config->accounts, account);
        changed = TRUE;
    }
    else
    {
        g_free (account->name);
        g_free (account->server);
        g_free (account->user);
        g_free (account->password);
        if (account->icon_colour)
            gdk_color_free (account->icon_colour);
    }

    iport = (guint) g_ascii_strtod (port, NULL);
    changed = (changed ||
               account->server != server ||
               account->user != user ||
               account->password != password ||
               account->port != iport ||
               account->plugin != plugin ||
               account->protocol != protocol);

    account->name = g_strdup (name);
    account->server = g_strdup (server);
    account->user = g_strdup (user);
    account->password = g_strdup (password);
    account->port = iport;
    account->icon_colour = mailtc_envelope_get_envelope_colour (
                                MAILTC_ENVELOPE (prefs->envelope_account));
    account->protocol = protocol;

    if (changed)
    {
        if (account->plugin && account->plugin->remove_account)
        {
            if (!(*account->plugin->remove_account) (account, error))
                return -1;
        }

        account->plugin = plugin;

        if (plugin->add_account)
        {
            if (!(*plugin->add_account) (config, account, error))
                return -1;
        }
    }
    return g_slist_index (config->accounts, account);
}

static void
mailtc_port_entry_insert_text_cb (GtkEditable* editable,
                                  gchar*       new_text,
                                  gint         new_text_length,
                                  gint*        position)
{
    g_signal_handlers_block_by_func (editable,
        G_CALLBACK (mailtc_port_entry_insert_text_cb), NULL);

    /* NOTE this is only valid for the purpose of entering text.
     * it will need rewriting if the text must be set because
     * it assumes that a single char is passed.
     */
    if (*new_text >= '0' && *new_text <= '9')
        gtk_editable_insert_text (editable, new_text,
                                  new_text_length, position);

    g_signal_handlers_unblock_by_func (editable,
        G_CALLBACK (mailtc_port_entry_insert_text_cb), NULL);
    g_signal_stop_emission_by_name (editable, "insert-text");
}

static gint
mailtc_combo_get_protocol_index (mtc_config*  config,
                                 mtc_account* account)
{
    GtkTreeModel* model;
    GtkTreeIter iter;
    mtc_prefs* prefs;
    gint plgindex;
    guint combo_index;
    guint combo_plgindex;
    gint i = 0;

    g_return_val_if_fail (config && config->prefs, -1);

    prefs = config->prefs;

    plgindex = g_slist_index (config->plugins, account->plugin);
    g_assert (plgindex > -1);

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (prefs->combo_plugin));
    while (gtk_tree_model_iter_nth_child (model, &iter, NULL, i++))
    {
        gtk_tree_model_get (model, &iter, COMBO_PLUGIN_COLUMN, &combo_plgindex,
                            COMBO_INDEX_COLUMN, &combo_index, -1);

        if ((guint) plgindex == combo_plgindex && account->protocol == combo_index)
            return (i - 1);
    }
    return -1;
}

static void
mailtc_combo_protocol_changed_cb (GtkComboBox* combo,
                                  mtc_config*  config)
{
    GtkTreeModel* model;
    GtkTreeIter iter;
    mtc_prefs* prefs;
    mtc_plugin* plugin;
    gint plgindex;
    gint index;
    gchar* port;
    gboolean exists;

    g_return_if_fail (config && config->prefs);

    port = NULL;
    prefs = config->prefs;

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
    g_assert (GTK_IS_TREE_MODEL (model));
    exists = gtk_combo_box_get_active_iter (GTK_COMBO_BOX  (combo), &iter);
    g_assert (exists);

    gtk_tree_model_get (model, &iter, COMBO_PLUGIN_COLUMN, &plgindex,
                        COMBO_INDEX_COLUMN, &index, -1);
    g_assert (index > -1);
    plugin = (mtc_plugin*) g_slist_nth_data (config->plugins, plgindex);
    g_assert (plugin);

    if (plugin->ports[index])
        port = g_strdup_printf ("%u", plugin->ports[index]);

    gtk_entry_set_text (GTK_ENTRY (prefs->entry_port), port ? port : "");
    g_free (port);
}

static void
mailtc_account_dialog_run (GtkWidget*   button,
                           mtc_config*  config,
                           mtc_account* account)
{
    GtkWidget* dialog;
    GtkWidget* table_account;
    GtkWidget* label_name;
    GtkWidget* entry_name;
    GtkWidget* label_server;
    GtkWidget* entry_server;
    GtkWidget* label_port;
    GtkWidget* entry_port;
    GtkWidget* label_user;
    GtkWidget* entry_user;
    GtkWidget* label_password;
    GtkWidget* entry_password;
    GtkWidget* label_protocol;
    GtkWidget* combo_protocol;
    GtkWidget* button_plugin;
    GtkWidget* hbox_icon;
    GtkWidget* label_icon;
    GtkWidget* envelope_icon;
    GtkWidget* button_icon;
    GtkListStore* store;
    GtkCellRenderer* renderer;
    GError* error;
    GSList* plglist;
    mtc_prefs* prefs;
    mtc_plugin* plugin;
    gchar* port;
    gint result;
    gint index;
    guint plgindex;

    (void) button;
    g_return_if_fail (config && config->prefs);

    prefs = config->prefs;
    if (!prefs->dialog_account)
    {
        dialog = gtk_dialog_new_with_buttons (
                PACKAGE " Configuration",
                GTK_WINDOW (prefs->dialog_config),
                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_STOCK_OK, GTK_RESPONSE_OK,
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                NULL);

        prefs->dialog_account = dialog;
        gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
        gtk_window_set_default_size (GTK_WINDOW (dialog), 100, 100);
        gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_PREFERENCES);

        label_name = gtk_label_new ("Name:");
        entry_name = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (entry_name), MAILTC_PATH_LENGTH);

        label_server = gtk_label_new ("Server:");
        entry_server = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (entry_server), MAILTC_PATH_LENGTH);

        label_port = gtk_label_new ("Port:");
        entry_port = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (entry_port), G_ASCII_DTOSTR_BUF_SIZE);

        label_user = gtk_label_new ("User:");
        entry_user = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (entry_user), MAILTC_PATH_LENGTH);

        label_password = gtk_label_new ("Password:");
        entry_password = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (entry_password), MAILTC_PATH_LENGTH);
        gtk_entry_set_visibility (GTK_ENTRY (entry_password), FALSE);

        label_protocol = gtk_label_new ("Protocol:");
        store = gtk_list_store_new (N_COMBO_COLUMNS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
        combo_protocol = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
        g_object_unref (store);
        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_protocol), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_protocol), renderer, "text", 0, NULL);

        plglist = config->plugins;
        plgindex = 0;
        while (plglist)
        {
            plugin = (mtc_plugin* ) plglist->data;
            for (index = 0; (guint) index < g_strv_length (plugin->protocols); index++)
            {
                gtk_list_store_insert_with_values (store, NULL, G_MAXINT,
                                                   COMBO_PROTOCOL_COLUMN, plugin->protocols[index],
                                                   COMBO_PLUGIN_COLUMN, plgindex,
                                                   COMBO_INDEX_COLUMN, index, -1);
            }
            plglist = g_slist_next (plglist);
            plgindex++;
        }

        button_plugin = gtk_button_new_with_label ("Plugin Information...");

        label_icon = gtk_label_new ("Icon Colour:");
        envelope_icon = mailtc_envelope_new ();
        button_icon = gtk_button_new_with_label ("Select Colour...");
        hbox_icon = gtk_hbox_new (FALSE, 5);
        gtk_box_pack_start (GTK_BOX (hbox_icon), envelope_icon, FALSE, FALSE, 0);
        gtk_box_pack_end (GTK_BOX (hbox_icon), button_icon, TRUE, TRUE, 0);

        table_account = gtk_table_new (8, 2, FALSE);
        gtk_table_attach (GTK_TABLE (table_account), label_name, 0, 1, 0, 1,
            GTK_SHRINK, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), entry_name, 1, 2, 0, 1,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), label_server, 0, 1, 1, 2,
            GTK_SHRINK, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), entry_server, 1, 2, 1, 2,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), label_port, 0, 1, 2, 3,
            GTK_SHRINK, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), entry_port, 1, 2, 2, 3,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), label_user, 0, 1, 3, 4,
            GTK_SHRINK, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), entry_user, 1, 2, 3, 4,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), label_password, 0, 1, 4, 5,
            GTK_SHRINK, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), entry_password, 1, 2, 4, 5,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), label_protocol, 0, 1, 5, 6,
            GTK_SHRINK, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), combo_protocol, 1, 2, 5, 6,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), button_plugin, 1, 2, 6, 7,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), label_icon, 0, 1, 7, 8,
            GTK_SHRINK, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), hbox_icon, 1, 2, 7, 8,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);

        gtk_container_set_border_width (GTK_CONTAINER (table_account), 4);

        g_signal_connect (entry_port, "insert-text",
                G_CALLBACK (mailtc_port_entry_insert_text_cb), NULL);
        g_signal_connect (dialog, "delete-event",
                G_CALLBACK (gtk_widget_hide_on_delete), NULL);
        g_signal_connect (dialog, "destroy",
                G_CALLBACK (gtk_widget_destroyed), &prefs->dialog_account);
        g_signal_connect (button_icon, "clicked",
                G_CALLBACK (mailtc_button_icon_clicked_cb), envelope_icon);
        g_signal_connect (button_plugin, "clicked",
                G_CALLBACK (mailtc_button_plugin_clicked_cb), config);
        g_signal_connect (combo_protocol, "changed",
            G_CALLBACK (mailtc_combo_protocol_changed_cb), config);

        prefs->entry_name = entry_name;
        prefs->entry_server = entry_server;
        prefs->entry_port = entry_port;
        prefs->entry_user = entry_user;
        prefs->entry_password = entry_password;
        prefs->envelope_account = envelope_icon;
        prefs->combo_plugin = combo_protocol;

        gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                            table_account, FALSE, 0, 0);
        gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
    }
    else
        dialog = prefs->dialog_account;

    port = NULL;
    if (account)
    {
        g_assert (account->name &&
                  account->server &&
                  account->user &&
                  account->password &&
                  account->icon_colour &&
                  account->plugin);

        port = g_strdup_printf ("%u", account->port);
        gtk_entry_set_text (GTK_ENTRY (prefs->entry_name), account->name);
        gtk_entry_set_text (GTK_ENTRY (prefs->entry_server), account->server);
        gtk_entry_set_text (GTK_ENTRY (prefs->entry_user), account->user);
        gtk_entry_set_text (GTK_ENTRY (prefs->entry_password), account->password);
        gtk_entry_set_text (GTK_ENTRY (prefs->entry_port), port);

        index = mailtc_combo_get_protocol_index (config, account);
        g_assert (index > -1);
        gtk_combo_box_set_active (GTK_COMBO_BOX (prefs->combo_plugin), index);
        mailtc_envelope_set_envelope_colour (MAILTC_ENVELOPE (prefs->envelope_account),
                                             account->icon_colour);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (prefs->entry_name), "");
        gtk_entry_set_text (GTK_ENTRY (prefs->entry_server), "");
        gtk_entry_set_text (GTK_ENTRY (prefs->entry_user), "");
        gtk_entry_set_text (GTK_ENTRY (prefs->entry_password), "");
        plglist = config->plugins;
        if (plglist)
        {
            plugin = (mtc_plugin* ) plglist->data;
            if (plugin && plugin->ports)
                port = g_strdup_printf ("%u", plugin->ports[0]);
        }
        gtk_combo_box_set_active (GTK_COMBO_BOX (prefs->combo_plugin), 0);
        mailtc_envelope_set_envelope_colour (MAILTC_ENVELOPE (prefs->envelope_account), NULL);
    }

    gtk_entry_set_text (GTK_ENTRY (prefs->entry_port), port ? port : "");
    g_free (port);

    gtk_widget_show_all (dialog);
    index = -1;
    error = NULL;

    while (index == -1)
    {
        result = gtk_dialog_run (GTK_DIALOG (dialog));
        switch (result)
        {
            case GTK_RESPONSE_OK:
                index = mailtc_account_dialog_save (config, account, &error);
                if (index != -1)
                    mailtc_account_update_tree_view (config, index);
                else
                    mailtc_gerror (&error);
            break;
            case GTK_RESPONSE_CANCEL:
            default:
                index = 1;
        }
    }
    gtk_widget_hide (dialog);
}

static void
mailtc_add_button_clicked_cb (GtkWidget*    button,
                              mtc_config*   config)
{
    mailtc_account_dialog_run (button, config, NULL);
}

static gint
mailtc_tree_view_get_selected_iter (GtkTreeView*  tree_view,
                                    GtkTreeModel* model,
                                    GtkTreeIter*  iter)
{
    GtkTreeSelection* selection;
    guint i = 0;

    selection = gtk_tree_view_get_selection (tree_view);

    if (!gtk_tree_selection_get_selected (selection, &model, NULL) ||
        !gtk_tree_model_get_iter_first (model, iter))
        return -1;

    do
    {
        if (gtk_tree_selection_iter_is_selected (selection, iter))
            return i;
        i++;
    } while (gtk_tree_model_iter_next (model, iter));

    return -1;
}

static void
mailtc_edit_button_clicked_cb (GtkWidget*    button,
                               mtc_config*   config)
{
    GtkTreeModel* model;
    GtkTreeIter iter;
    mtc_prefs* prefs;
    gint index;

    g_return_if_fail (config && config->prefs);

    prefs = config->prefs;
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (prefs->tree_view));
    index = mailtc_tree_view_get_selected_iter (GTK_TREE_VIEW (prefs->tree_view),
                                                model, &iter);
    if (index != -1)
    {
        mtc_account* account;

        account = (mtc_account*) g_slist_nth_data (config->accounts, index);
        g_assert (account);

        mailtc_account_dialog_run (button, config, account);
    }
}

static void
mailtc_remove_button_clicked_cb (GtkWidget*  button,
                                 mtc_config* config)
{
    GtkTreeModel* model;
    GtkTreeIter iter;
    mtc_prefs* prefs;
    gint index;

    (void) button;
    g_return_if_fail (config && config->prefs);

    prefs = config->prefs;
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (prefs->tree_view));
    index = mailtc_tree_view_get_selected_iter (GTK_TREE_VIEW (prefs->tree_view),
                                                model, &iter);
    if (index != -1)
    {
        mtc_account* account;
        mtc_plugin* plugin;
        GError* error = NULL;

        account = (mtc_account*) g_slist_nth_data (config->accounts, index);
        g_assert (account);
        plugin = account->plugin;
        g_assert (plugin);

        config->accounts = g_slist_remove (config->accounts, account);
        mailtc_free_account (account, &error);
        if (error)
            mailtc_gerror (&error);
        gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
    }
}

static void
mailtc_cursor_or_cols_changed_cb (GtkTreeView* tree_view,
                                  GtkWidget*   button)
{
    GtkTreeModel* model;
    GtkTreeSelection* selection;
    GtkTreeIter iter;
    gboolean selected;

    g_return_if_fail (GTK_IS_TREE_VIEW (tree_view));

    if (GTK_IS_BUTTON (button))
    {
        selected = FALSE;
        selection = gtk_tree_view_get_selection (tree_view);
        model = gtk_tree_view_get_model (tree_view);
        if (selection)
            if (gtk_tree_selection_get_selected (selection, &model, &iter))
                selected = TRUE;
        gtk_widget_set_sensitive (button, selected);
    }
}

static void
mailtc_tree_view_destroy_cb (GtkWidget* widget,
                             GtkButton* button)
{
    g_signal_handlers_disconnect_by_func (widget,
            mailtc_cursor_or_cols_changed_cb, button);
}

static GtkWidget*
mailtc_config_dialog_page_accounts (mtc_config* config)
{
    GtkWidget* table_accounts;
    GtkWidget* tree_view;
    GtkWidget* tree_scroll;
    GtkListStore* store;
    GtkTreeSelection* selection;
    GtkTreeViewColumn* column;
    GtkCellRenderer* renderer;
    GtkWidget* hbox;
    GtkWidget* button_add;
    GtkWidget* button_edit;
    GtkWidget* button_remove;
    GSList* list;
    mtc_account* account;
    mtc_prefs* prefs;

    g_return_val_if_fail (config && config->prefs, NULL);
    prefs = config->prefs;
    list = config->accounts;

    store = gtk_list_store_new (N_TREEVIEW_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
    tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    g_object_unref (store);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Account",
                                    renderer, "text", TREEVIEW_ACCOUNT_COLUMN, NULL);
    gtk_tree_view_column_set_min_width (column, 230);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
    column = gtk_tree_view_column_new_with_attributes ("Protocol",
                                    renderer, "text", TREEVIEW_PROTOCOL_COLUMN, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    tree_scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (tree_scroll), tree_view);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tree_scroll),
                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (tree_scroll),
                                GTK_SHADOW_IN);

    while (list)
    {
        account = (mtc_account*) list->data;
        gtk_list_store_insert_with_values (store, NULL, G_MAXINT,
                                           TREEVIEW_ACCOUNT_COLUMN, account->name,
                                           TREEVIEW_PROTOCOL_COLUMN, account->plugin->protocols[account->protocol], -1);
        list = g_slist_next (list);
    }

    button_edit = gtk_button_new_from_stock (GTK_STOCK_PROPERTIES);
    button_add = gtk_button_new_from_stock (GTK_STOCK_ADD);
    button_remove = gtk_button_new_from_stock (GTK_STOCK_REMOVE);

    hbox = gtk_hbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), button_add, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), button_edit, TRUE, TRUE, 5);
    gtk_box_pack_start (GTK_BOX (hbox), button_remove, TRUE, TRUE, 0);

	g_signal_connect (button_add, "clicked",
            G_CALLBACK (mailtc_add_button_clicked_cb), config);
    g_signal_connect (button_edit, "clicked",
            G_CALLBACK (mailtc_edit_button_clicked_cb), config);
	g_signal_connect (button_remove, "clicked",
	        G_CALLBACK (mailtc_remove_button_clicked_cb), config);

    g_object_connect (tree_view,
                      "signal::cursor-changed",
                      mailtc_cursor_or_cols_changed_cb, button_edit,
                      "signal::columns-changed",
                      mailtc_cursor_or_cols_changed_cb, button_edit,
                      "signal::cursor-changed",
                      mailtc_cursor_or_cols_changed_cb, button_remove,
                      "signal::columns-changed",
                      mailtc_cursor_or_cols_changed_cb, button_remove,
                      "signal::destroy",
                      mailtc_tree_view_destroy_cb, button_remove,
                      "signal::destroy",
                      mailtc_tree_view_destroy_cb, button_edit,
                      NULL);
    mailtc_cursor_or_cols_changed_cb (GTK_TREE_VIEW (tree_view), button_edit);
    mailtc_cursor_or_cols_changed_cb (GTK_TREE_VIEW (tree_view), button_remove);

    prefs->tree_view = tree_view;

    table_accounts = gtk_table_new (2, 1, FALSE);
    gtk_table_attach (GTK_TABLE (table_accounts), tree_scroll, 0, 1, 0, 1,
                      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
    gtk_table_attach (GTK_TABLE (table_accounts), hbox, 0, 1, 1, 2,
                      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
    gtk_container_set_border_width (GTK_CONTAINER (table_accounts), 4);
    gtk_widget_show_all (table_accounts);

    return table_accounts;
}

GtkWidget*
mailtc_config_dialog (mtc_config* config)
{
    GtkWidget* dialog;
    GtkWidget* notebook;
    GtkWidget* page_general;
    GtkWidget* label_general;
    GtkWidget* page_accounts;
    GtkWidget* label_accounts;
    mtc_prefs* prefs;

    dialog = gtk_dialog_new_with_buttons (PACKAGE " Configuration",
                                          NULL,
                                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_STOCK_SAVE, GTK_RESPONSE_OK,
                                          GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                          NULL);
    gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
    gtk_window_set_default_size (GTK_WINDOW (dialog), 100, 100);
    gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_PREFERENCES);

    prefs = g_new0 (mtc_prefs, 1);
    prefs->dialog_config = dialog;
    config->prefs = prefs;

    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_container_set_border_width (GTK_CONTAINER (notebook), 6);

    label_general = gtk_label_new ("General");
    page_general = mailtc_config_dialog_page_general (prefs);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_general, label_general);

    if (config->mail_command)
        gtk_entry_set_text (GTK_ENTRY (prefs->entry_command), config->mail_command);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (prefs->spin_interval),
                               (gdouble)config->interval);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (prefs->spin_connections),
                               (gdouble)config->net_error);
    gtk_combo_box_set_active (GTK_COMBO_BOX (prefs->combo_errordlg),
                              (config->net_error > 2) ? 2 : config->net_error);
    mailtc_envelope_set_envelope_colour (MAILTC_ENVELOPE (prefs->envelope_config),
                                         config->icon_colour);

    label_accounts = gtk_label_new ("Mail Accounts");
    page_accounts = mailtc_config_dialog_page_accounts (config);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_accounts, label_accounts);

    g_signal_connect (dialog, "destroy",
            G_CALLBACK (gtk_widget_destroyed), &prefs->dialog_config);
    g_signal_connect_after (dialog, "destroy",
            G_CALLBACK (mailtc_config_dialog_destroy_cb), NULL);
    g_signal_connect (dialog, "delete-event",
            G_CALLBACK (mailtc_config_dialog_delete_event_cb), NULL);
    g_signal_connect (dialog, "response",
            G_CALLBACK (mailtc_config_dialog_response_cb), config);

    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), notebook, FALSE, 0, 0);

    gtk_widget_show (label_general);
    gtk_widget_show (label_accounts);
    gtk_widget_show (notebook);
    gtk_widget_show (dialog);

    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

    return dialog;
}

