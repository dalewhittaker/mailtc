/* mtc-configdialog.c
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

#include "mtc-configdialog.h"
#include "mtc-account.h"
#include "mtc-envelope.h"
#include "mtc-util.h"

#ifdef MAXPATHLEN
#define MAILTC_PATH_LENGTH MAXPATHLEN
#elif defined (PATH_MAX)
#define MAILTC_PATH_LENGTH PATH_MAX
#else
#define MAILTC_PATH_LENGTH 2048
#endif

#define MAILTC_CONFIG_DIALOG_SET_OBJECT(dialog,property) \
    mailtc_object_set_object (G_OBJECT (dialog), MAILTC_TYPE_CONFIG_DIALOG, \
                              #property, (GObject **) (&dialog->property), G_OBJECT (property))

enum
{
    TREEVIEW_ACCOUNT_COLUMN = 0,
    TREEVIEW_PROTOCOL_COLUMN,

    N_TREEVIEW_COLUMNS
};

enum
{
    COMBO_PROTOCOL_COLUMN = 0,
    COMBO_EXTENSION_COLUMN,
    COMBO_INDEX_COLUMN,

    N_COMBO_COLUMNS
};

struct _MailtcConfigDialogPrivate
{
    GtkWidget* dialog_account;
    GtkWidget* dialog_extension;
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
    GtkWidget* combo_extension;

    GPtrArray* accounts;
    MailtcModuleManager* modules;

    gulong entry_insert_text_id;
    gulong button_edit_columns_changed_id;
    gulong button_edit_cursor_changed_id;
    gulong button_remove_columns_changed_id;
    gulong button_remove_cursor_changed_id;
};

struct _MailtcConfigDialog
{
    GtkDialog parent_instance;

    MailtcConfigDialogPrivate* priv;
    MailtcSettings* settings;
};

struct _MailtcConfigDialogClass
{
    GtkDialogClass parent_class;
};

G_DEFINE_TYPE (MailtcConfigDialog, mailtc_config_dialog, GTK_TYPE_DIALOG)

enum
{
    PROP_0,
    PROP_SETTINGS
};

static void
mailtc_config_dialog_destroy_cb (GtkWidget* widget)
{
    mailtc_gtk_message (widget, GTK_MESSAGE_INFO, "Now run " PACKAGE " to check mail.");
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
mailtc_config_dialog_response_cb (GtkWidget* dialog,
                                  gint       response_id)
{
    if (response_id == GTK_RESPONSE_OK)
    {
        MailtcConfigDialog* dialog_config;
        MailtcConfigDialogPrivate* priv;
        MailtcSettings* settings;
        guint net_error;
        GdkColor colour;
        GError* error = NULL;

        g_assert (MAILTC_IS_CONFIG_DIALOG (dialog));

        dialog_config = MAILTC_CONFIG_DIALOG (dialog);
        priv = dialog_config->priv;

        settings = dialog_config->settings;

        mailtc_settings_set_interval (settings,
                (guint) gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->spin_interval)));

        mailtc_settings_set_command (settings,
                        gtk_entry_get_text (GTK_ENTRY (priv->entry_command)));

        mailtc_envelope_get_colour (MAILTC_ENVELOPE (priv->envelope_config), &colour);
        mailtc_settings_set_iconcolour (settings, &colour);

        net_error = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->combo_errordlg));
        if (net_error > 1)
            net_error = (guint) gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->spin_connections));

        mailtc_settings_set_neterror (settings, net_error);

        if (!mailtc_settings_write (settings, &error))
            mailtc_gerror_gtk (GTK_WIDGET (dialog_config), &error);
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
    GdkColor colour;
    gint response;

    (void) button;

    dialog = gtk_color_selection_dialog_new ("Select Icon Colour");
    coloursel = gtk_color_selection_dialog_get_color_selection (GTK_COLOR_SELECTION_DIALOG (dialog));

    mailtc_envelope_get_colour (envelope, &colour);
    gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (coloursel), &colour);

    response = gtk_dialog_run (GTK_DIALOG (dialog));

    if (response == GTK_RESPONSE_OK)
    {
        gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (coloursel), &colour);
        mailtc_envelope_set_colour (envelope, &colour);
    }
    gtk_widget_destroy (dialog);
}

static void
mailtc_button_extension_clicked_cb (GtkWidget*          button,
                                    MailtcConfigDialog* dialog_config)
{
    MailtcConfigDialogPrivate* priv;
    MailtcExtension* extension = NULL;
    GtkWidget* dialog_extension;
    GtkTreeModel* model;
    GtkTreeIter iter;
    const gchar* authors[2];
    gboolean exists;

    (void) button;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog_config));

    priv = dialog_config->priv;
    dialog_extension = priv->dialog_extension;

    if (!dialog_extension)
    {
        dialog_extension = gtk_about_dialog_new ();

        g_signal_connect (dialog_extension, "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);

        priv->dialog_extension = dialog_extension;
    }

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_extension));
    g_assert (GTK_IS_TREE_MODEL (model));
    exists = gtk_combo_box_get_active_iter (GTK_COMBO_BOX  (priv->combo_extension), &iter);
    g_assert (exists);

    gtk_tree_model_get (model, &iter, COMBO_EXTENSION_COLUMN, &extension, -1);
    g_assert (MAILTC_IS_EXTENSION (extension));

    authors[0] = mailtc_extension_get_author (extension);
    authors[1] = NULL;

    gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (dialog_extension), mailtc_extension_get_name (extension));
    gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (dialog_extension), mailtc_extension_get_compatibility (extension));
    gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG (dialog_extension), mailtc_extension_get_description (extension));
    gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (dialog_extension), authors);
    g_object_unref (extension);

    gtk_dialog_run (GTK_DIALOG (dialog_extension));
    gtk_widget_hide (dialog_extension);
}

static GtkWidget*
mailtc_config_dialog_page_general (MailtcConfigDialog* dialog)
{
    MailtcConfigDialogPrivate* priv;
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

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog));

    label_interval = gtk_label_new ("Interval in minutes for mail check:");
    adj_interval = (GtkAdjustment*)gtk_adjustment_new (1.0, 1.0, 60.0, 1.0, 5.0, 0.0);
    spin_interval = gtk_spin_button_new (GTK_ADJUSTMENT (adj_interval), 1.0, 0);

    label_command = gtk_label_new ("Mail reading command:");
    entry_command = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (entry_command), MAILTC_PATH_LENGTH);

    label_errordlg = gtk_label_new ("Show network error dialog:");
    combo_errordlg = gtk_combo_box_text_new ();
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_errordlg), "Never");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_errordlg), "Always");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo_errordlg), "Every");

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

    priv = dialog->priv;
    priv->spin_interval = spin_interval;
    priv->entry_command = entry_command;
    priv->envelope_config = envelope_icon;
    priv->combo_errordlg = combo_errordlg;
    priv->spin_connections = spin_connections;

    gtk_widget_show_all (table_general);

    return table_general;
}

static void
mailtc_account_update_tree_view (MailtcConfigDialog* dialog,
                                 gint                index)
{
    MailtcConfigDialogPrivate* priv;
    MailtcAccount* account;
    MailtcExtension* extension;
    MailtcProtocol* protocol;
    GArray* protocols;
    GtkTreeModel* model;
    GtkTreeIter iter;
    GtkTreeIter combo_iter;
    gboolean exists;
    gint n;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog));
    g_assert (index > -1);

    priv = dialog->priv;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->tree_view));
    n = gtk_tree_model_iter_n_children (model, NULL);
    if (index < n)
    {
        if (!gtk_tree_model_iter_nth_child (model, &iter, NULL, index))
        {
            mailtc_gtk_message (GTK_WIDGET (dialog), GTK_MESSAGE_ERROR, "Error getting tree view iter");
            return;
        }
    }
    else
        gtk_list_store_append (GTK_LIST_STORE (model), &iter);

    account = g_ptr_array_index (priv->accounts, index);
    g_assert (MAILTC_IS_ACCOUNT (account));

    exists = gtk_combo_box_get_active_iter (GTK_COMBO_BOX  (priv->combo_extension), &combo_iter);
    g_assert (exists);

    extension = mailtc_account_get_extension (account);
    protocols = mailtc_extension_get_protocols (extension);
    protocol = &g_array_index (protocols, MailtcProtocol, mailtc_account_get_protocol (account));
    g_assert (protocol);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                        TREEVIEW_ACCOUNT_COLUMN, mailtc_account_get_name (account),
                        TREEVIEW_PROTOCOL_COLUMN, protocol->name, -1);
    g_array_unref (protocols);
    g_object_unref (extension);
}

static gint
mailtc_account_dialog_save (MailtcConfigDialog* dialog_config,
                            MailtcAccount*      account,
                            GError**            error)
{
    MailtcConfigDialogPrivate* priv;
    MailtcExtension* extension = NULL;
    MailtcExtension* accextension;
    GtkTreeModel* model;
    GtkTreeIter iter;
    GdkColor colour;
    const gchar* name;
    const gchar* server;
    const gchar* user;
    const gchar* password;
    const gchar* port;
    const gchar*msg;
    gint active;
    guint iport;
    guint protocol;
    gboolean exists;
    gboolean empty;
    gboolean changed;
    gboolean success = TRUE;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog_config));

    priv = dialog_config->priv;

    name = gtk_entry_get_text (GTK_ENTRY (priv->entry_name));
    server = gtk_entry_get_text (GTK_ENTRY (priv->entry_server));
    port = gtk_entry_get_text (GTK_ENTRY (priv->entry_port));
    user = gtk_entry_get_text (GTK_ENTRY (priv->entry_user));
    password = gtk_entry_get_text (GTK_ENTRY (priv->entry_password));

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_extension));
    g_assert (GTK_IS_TREE_MODEL (model));
    exists = gtk_combo_box_get_active_iter (GTK_COMBO_BOX  (priv->combo_extension), &iter);
    g_assert (exists);

    gtk_tree_model_get (model, &iter, COMBO_EXTENSION_COLUMN, &extension, COMBO_INDEX_COLUMN, &protocol, -1);
    g_assert (MAILTC_IS_EXTENSION (extension));

    empty = FALSE;
    changed = FALSE;

    if (!g_strcmp0 (name, ""))
    {
        msg = "name";
        empty = TRUE;
    }
    else if (!g_strcmp0 (server, ""))
    {
        msg = "server";
        empty = TRUE;
    }
    else if (!g_strcmp0 (port, ""))
    {
        msg = "port";
        empty = TRUE;
    }
    else if (!g_strcmp0 (user, ""))
    {
        msg = "user";
        empty = TRUE;
    }
    else if (!g_strcmp0 (password, ""))
    {
        msg = "password";
        empty = TRUE;
    }

    if (empty)
    {
        mailtc_gtk_message (GTK_WIDGET (dialog_config), GTK_MESSAGE_ERROR, "You must enter a %s", msg);
        return -1;
    }

    iport = (guint) g_ascii_strtod (port, NULL);

    if (!account)
    {
        account = mailtc_account_new ();
        g_ptr_array_add (priv->accounts, account);
        changed = TRUE;
    }
    else
    {
        accextension = mailtc_account_get_extension (account);
        changed = (mailtc_account_get_server (account) != server ||
                   mailtc_account_get_user (account) != user ||
                   mailtc_account_get_password (account) != password ||
                   mailtc_account_get_port (account) != iport ||
                   mailtc_account_get_protocol (account) != protocol ||
                   accextension != extension);
        g_object_unref (accextension);
    }

    mailtc_account_set_name (account, name);
    mailtc_account_set_server (account, server);
    mailtc_account_set_user (account, user);
    mailtc_account_set_password (account, password);
    mailtc_account_set_port (account, iport);
    mailtc_account_set_protocol (account, protocol);
    mailtc_envelope_get_colour (MAILTC_ENVELOPE (priv->envelope_account), &colour);
    mailtc_account_set_iconcolour (account, &colour);

    if (changed)
        success = mailtc_account_update_extension (account, extension, error);

    g_object_unref (extension);

    if (success)
    {
        for (active = 0; (guint) active < priv->accounts->len; active++)
        {
            if (g_ptr_array_index (priv->accounts, active) == account)
                return active;
        }
    }
    return -1;
}

static void
mailtc_port_entry_insert_text_cb (GtkEditable*               editable,
                                  gchar*                     new_text,
                                  gint                       new_text_length,
                                  gint*                      position,
                                  MailtcConfigDialogPrivate* priv)
{
    g_assert (priv);
    g_signal_handler_block (editable, priv->entry_insert_text_id);

    /* NOTE this is only valid for the purpose of entering text.
     * it will need rewriting if the text must be set because
     * it assumes that a single char is passed.
     */
    if (*new_text >= '0' && *new_text <= '9')
        gtk_editable_insert_text (editable, new_text, new_text_length, position);

    g_signal_handler_unblock (editable, priv->entry_insert_text_id);
    g_signal_stop_emission_by_name (editable, "insert-text");
}

static gint
mailtc_combo_get_protocol_index (MailtcConfigDialog* dialog_config,
                                 MailtcAccount*      account)
{
    MailtcConfigDialogPrivate* priv;
    MailtcExtension* extension;
    MailtcExtension* accextension = NULL;
    GtkTreeModel* model;
    GtkTreeIter iter;
    guint index;
    guint protocol;
    gint i = 0;
    gint j = -1;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog_config));

    priv = dialog_config->priv;

    extension = mailtc_account_get_extension (account);
    g_assert (extension);

    protocol = mailtc_account_get_protocol (account);

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_extension));
    while (gtk_tree_model_iter_nth_child (model, &iter, NULL, i++))
    {
        gtk_tree_model_get (model, &iter, COMBO_EXTENSION_COLUMN, &accextension, COMBO_INDEX_COLUMN, &index, -1);

        if (extension == accextension && protocol == index)
        {
            j = i - 1;
            g_object_unref (accextension);
            break;
        }
        g_object_unref (accextension);
    }

    g_object_unref (extension);
    return j;
}

static void
mailtc_combo_protocol_changed_cb (GtkComboBox*        combo,
                                  MailtcConfigDialog* dialog)
{
    MailtcConfigDialogPrivate* priv;
    MailtcExtension* extension = NULL;
    GtkTreeModel* model;
    GtkTreeIter iter;
    GArray* protocols;
    gint index;
    gboolean exists;
    gchar* port = NULL;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog));

    priv = dialog->priv;

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
    g_assert (GTK_IS_TREE_MODEL (model));
    exists = gtk_combo_box_get_active_iter (GTK_COMBO_BOX  (combo), &iter);
    g_assert (exists);

    gtk_tree_model_get (model, &iter, COMBO_EXTENSION_COLUMN, &extension, COMBO_INDEX_COLUMN, &index, -1);
    g_assert (MAILTC_IS_EXTENSION (extension));

    protocols = mailtc_extension_get_protocols (extension);
    if ((guint) index < protocols->len)
    {
        MailtcProtocol* protocol;

        protocol = &g_array_index (protocols, MailtcProtocol, index);
        port = g_strdup_printf ("%u", protocol->port);
    }
    g_array_unref (protocols);
    g_object_unref (extension);

    gtk_entry_set_text (GTK_ENTRY (priv->entry_port), port ? port : "");
    g_free (port);
}

static void
mailtc_combo_protocol_add_items (MailtcExtension* extension, GtkListStore* store)
{
    MailtcProtocol* protocol;
    GArray* protocols;
    gint index;

    g_assert (MAILTC_IS_EXTENSION (extension));
    g_assert (GTK_IS_LIST_STORE (store));

    protocols = mailtc_extension_get_protocols (extension);
    g_assert (protocols);

    for (index = 0; (guint) index < protocols->len; index++)
    {
        protocol = &g_array_index (protocols, MailtcProtocol, index);
        gtk_list_store_insert_with_values (store, NULL, G_MAXINT,
                                           COMBO_PROTOCOL_COLUMN, protocol->name,
                                           COMBO_EXTENSION_COLUMN, extension,
                                           COMBO_INDEX_COLUMN, index, -1);
    }
    g_array_unref (protocols);
}

static void
mailtc_account_dialog_run (GtkWidget*          button,
                           MailtcConfigDialog* dialog_config,
                           MailtcAccount*      account)
{
    MailtcConfigDialogPrivate* priv;
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
    GtkWidget* button_extension;
    GtkWidget* hbox_icon;
    GtkWidget* label_icon;
    GtkWidget* envelope_icon;
    GtkWidget* button_icon;
    GtkListStore* store;
    GtkCellRenderer* renderer;
    GError* error;
    gint result;
    gint index;
    gulong id;

    (void) button;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog_config));
    priv = dialog_config->priv;

    if (!priv->dialog_account)
    {
        dialog = gtk_dialog_new_with_buttons (
                PACKAGE " Configuration",
                GTK_WINDOW (dialog_config),
                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_STOCK_OK, GTK_RESPONSE_OK,
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                NULL);

        priv->dialog_account = dialog;
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
        store = gtk_list_store_new (N_COMBO_COLUMNS, G_TYPE_STRING, MAILTC_TYPE_EXTENSION, G_TYPE_INT);
        combo_protocol = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
        g_object_unref (store);
        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_protocol), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_protocol), renderer, "text", 0, NULL);

        mailtc_module_manager_foreach_extension (priv->modules, (GFunc) mailtc_combo_protocol_add_items, store);
        button_extension = gtk_button_new_with_label ("Plugin Information...");

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
        gtk_table_attach (GTK_TABLE (table_account), button_extension, 1, 2, 6, 7,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), label_icon, 0, 1, 7, 8,
            GTK_SHRINK, GTK_EXPAND | GTK_FILL, 10, 2);
        gtk_table_attach (GTK_TABLE (table_account), hbox_icon, 1, 2, 7, 8,
            GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);

        gtk_container_set_border_width (GTK_CONTAINER (table_account), 4);

        g_signal_connect (dialog, "delete-event",
                G_CALLBACK (gtk_widget_hide_on_delete), NULL);
        g_signal_connect (dialog, "destroy",
                G_CALLBACK (gtk_widget_destroyed), &priv->dialog_account);
        g_signal_connect (button_icon, "clicked",
                G_CALLBACK (mailtc_button_icon_clicked_cb), envelope_icon);
        g_signal_connect (button_extension, "clicked",
                G_CALLBACK (mailtc_button_extension_clicked_cb), dialog_config);
        g_signal_connect (combo_protocol, "changed",
            G_CALLBACK (mailtc_combo_protocol_changed_cb), dialog_config);

        id = g_signal_connect (entry_port, "insert-text",
                G_CALLBACK (mailtc_port_entry_insert_text_cb), priv);

        priv->entry_insert_text_id = id;
        priv->entry_name = entry_name;
        priv->entry_server = entry_server;
        priv->entry_port = entry_port;
        priv->entry_user = entry_user;
        priv->entry_password = entry_password;
        priv->envelope_account = envelope_icon;
        priv->combo_extension = combo_protocol;

        gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                            table_account, FALSE, 0, 0);
        gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
    }
    else
        dialog = priv->dialog_account;

    if (account)
    {
        const gchar* name;
        const gchar* server;
        const gchar* user;
        const gchar* password;
        gchar* port;
        GdkColor colour;

        name = mailtc_account_get_name (account);
        server = mailtc_account_get_server (account);
        user = mailtc_account_get_user (account);
        password = mailtc_account_get_password (account);
        g_assert (name && server && user && password);

        mailtc_account_get_iconcolour (account, &colour);

        port = g_strdup_printf ("%u", mailtc_account_get_port (account));
        gtk_entry_set_text (GTK_ENTRY (priv->entry_name), name);
        gtk_entry_set_text (GTK_ENTRY (priv->entry_server), server);
        gtk_entry_set_text (GTK_ENTRY (priv->entry_user), user);
        gtk_entry_set_text (GTK_ENTRY (priv->entry_password), password);
        gtk_entry_set_text (GTK_ENTRY (priv->entry_port), port);

        index = mailtc_combo_get_protocol_index (dialog_config, account);
        g_assert (index > -1);
        gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_extension), index);
        mailtc_envelope_set_colour (MAILTC_ENVELOPE (priv->envelope_account), &colour);
        gtk_entry_set_text (GTK_ENTRY (priv->entry_port), port ? port : "");
        g_free (port);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->entry_name), "");
        gtk_entry_set_text (GTK_ENTRY (priv->entry_server), "");
        gtk_entry_set_text (GTK_ENTRY (priv->entry_user), "");
        gtk_entry_set_text (GTK_ENTRY (priv->entry_password), "");

        gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_extension), 0);
        mailtc_envelope_set_colour (MAILTC_ENVELOPE (priv->envelope_account), NULL);
    }


    gtk_widget_show_all (dialog);
    index = -1;
    error = NULL;

    while (index == -1)
    {
        result = gtk_dialog_run (GTK_DIALOG (dialog));
        switch (result)
        {
            case GTK_RESPONSE_OK:
                index = mailtc_account_dialog_save (dialog_config, account, &error);
                if (index != -1)
                    mailtc_account_update_tree_view (dialog_config, index);
                else
                    mailtc_gerror_gtk (GTK_WIDGET (dialog_config), &error);
            break;
            case GTK_RESPONSE_CANCEL:
            default:
                index = 1;
        }
    }
    gtk_widget_hide (dialog);
}

static void
mailtc_add_button_clicked_cb (GtkWidget*          button,
                              MailtcConfigDialog* dialog)
{
    mailtc_account_dialog_run (button, dialog, NULL);
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
mailtc_edit_button_clicked_cb (GtkWidget*          button,
                               MailtcConfigDialog* dialog)
{
    MailtcConfigDialogPrivate* priv;
    GtkTreeModel* model;
    GtkTreeIter iter;
    gint index;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog));

    priv = dialog->priv;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->tree_view));
    index = mailtc_tree_view_get_selected_iter (GTK_TREE_VIEW (priv->tree_view), model, &iter);
    if (index != -1)
    {
        MailtcAccount* account;

        account = g_ptr_array_index (priv->accounts, index);
        g_assert (MAILTC_IS_ACCOUNT (account));

        mailtc_account_dialog_run (button, dialog, account);
    }
}

static void
mailtc_remove_button_clicked_cb (GtkWidget* button,
                                 MailtcConfigDialog* dialog)
{
    MailtcConfigDialogPrivate* priv;
    GtkTreeModel* model;
    GtkTreeIter iter;
    gint index;

    (void) button;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog));

    priv = dialog->priv;

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->tree_view));
    index = mailtc_tree_view_get_selected_iter (GTK_TREE_VIEW (priv->tree_view),
                                                model, &iter);
    if (index != -1)
    {
        MailtcAccount* account;

        account = g_ptr_array_remove_index (priv->accounts, index);
        g_object_unref (account);
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

    g_assert (GTK_IS_TREE_VIEW (tree_view));

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
mailtc_tree_view_destroy_cb (GtkWidget*                 widget,
                             MailtcConfigDialogPrivate* priv)
{
    g_assert (priv);
    g_signal_handler_disconnect (widget, priv->button_edit_columns_changed_id);
    g_signal_handler_disconnect (widget, priv->button_edit_cursor_changed_id);
    g_signal_handler_disconnect (widget, priv->button_remove_columns_changed_id);
    g_signal_handler_disconnect (widget, priv->button_remove_cursor_changed_id);
}

static GtkWidget*
mailtc_config_dialog_page_accounts (MailtcConfigDialog* dialog)
{
    MailtcConfigDialogPrivate* priv;
    MailtcAccount* account;
    MailtcExtension* extension;
    MailtcProtocol* protocol;
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
    GArray* protocols;
    guint i;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog));

    priv = dialog->priv;

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

    for (i = 0; i < priv->accounts->len; i++)
    {
        account = g_ptr_array_index (priv->accounts, i);
        g_assert (MAILTC_IS_ACCOUNT (account));

        extension = mailtc_account_get_extension (account);
        g_assert (extension);
        protocols = mailtc_extension_get_protocols (extension);
        g_assert (protocols);
        protocol = &g_array_index (protocols, MailtcProtocol, mailtc_account_get_protocol (account));
        g_assert (protocol);
        gtk_list_store_insert_with_values (store, NULL, G_MAXINT,
                                           TREEVIEW_ACCOUNT_COLUMN, mailtc_account_get_name (account),
                                           TREEVIEW_PROTOCOL_COLUMN, protocol->name, -1);
        g_array_unref (protocols);
        g_object_unref (extension);
    }

    button_edit = gtk_button_new_from_stock (GTK_STOCK_PROPERTIES);
    button_add = gtk_button_new_from_stock (GTK_STOCK_ADD);
    button_remove = gtk_button_new_from_stock (GTK_STOCK_REMOVE);

    hbox = gtk_hbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), button_add, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), button_edit, TRUE, TRUE, 5);
    gtk_box_pack_start (GTK_BOX (hbox), button_remove, TRUE, TRUE, 0);

    g_signal_connect (button_add, "clicked",
            G_CALLBACK (mailtc_add_button_clicked_cb), dialog);
    g_signal_connect (button_edit, "clicked",
            G_CALLBACK (mailtc_edit_button_clicked_cb), dialog);
    g_signal_connect (button_remove, "clicked",
            G_CALLBACK (mailtc_remove_button_clicked_cb), dialog);

    priv->button_edit_cursor_changed_id = g_signal_connect (
            tree_view, "cursor-changed", G_CALLBACK (mailtc_cursor_or_cols_changed_cb), button_edit);
    priv->button_edit_columns_changed_id = g_signal_connect (
            tree_view, "columns-changed", G_CALLBACK (mailtc_cursor_or_cols_changed_cb), button_edit);
    priv->button_remove_cursor_changed_id = g_signal_connect (
            tree_view, "cursor-changed", G_CALLBACK (mailtc_cursor_or_cols_changed_cb), button_remove);
    priv->button_remove_columns_changed_id = g_signal_connect (
            tree_view, "columns-changed", G_CALLBACK (mailtc_cursor_or_cols_changed_cb), button_remove);

    g_signal_connect (tree_view, "destroy",
            G_CALLBACK (mailtc_tree_view_destroy_cb), priv);

    mailtc_cursor_or_cols_changed_cb (GTK_TREE_VIEW (tree_view), button_edit);
    mailtc_cursor_or_cols_changed_cb (GTK_TREE_VIEW (tree_view), button_remove);

    priv->tree_view = tree_view;

    table_accounts = gtk_table_new (2, 1, FALSE);
    gtk_table_attach (GTK_TABLE (table_accounts), tree_scroll, 0, 1, 0, 1,
                      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
    gtk_table_attach (GTK_TABLE (table_accounts), hbox, 0, 1, 1, 2,
                      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 10, 2);
    gtk_container_set_border_width (GTK_CONTAINER (table_accounts), 4);
    gtk_widget_show_all (table_accounts);

    return table_accounts;
}

static void
mailtc_config_dialog_set_settings (MailtcConfigDialog* dialog,
                                   MailtcSettings*     settings)
{
    MAILTC_CONFIG_DIALOG_SET_OBJECT (dialog, settings);
}

static void
mailtc_config_dialog_set_property (GObject*      object,
                                   guint         prop_id,
                                   const GValue* value,
                                   GParamSpec*   pspec)
{
    MailtcConfigDialog* dialog;

    dialog = MAILTC_CONFIG_DIALOG (object);

    switch (prop_id)
    {
        case PROP_SETTINGS:
            mailtc_config_dialog_set_settings (dialog, g_value_get_object (value));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_config_dialog_get_property (GObject*    object,
                                   guint       prop_id,
                                   GValue*     value,
                                   GParamSpec* pspec)
{
    MailtcConfigDialog* dialog = MAILTC_CONFIG_DIALOG (object);

    switch (prop_id)
    {
        case PROP_SETTINGS:
            g_value_set_object (value, dialog->settings);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_config_dialog_finalize (GObject* object)
{
    MailtcConfigDialog* dialog;
    MailtcConfigDialogPrivate* priv;

    dialog = MAILTC_CONFIG_DIALOG (object);
    priv = dialog->priv;

    if (priv->accounts)
        g_ptr_array_unref (priv->accounts);
    if (priv->modules)
        g_object_unref (priv->modules);
    if (dialog->settings)
        g_object_unref (dialog->settings);

    G_OBJECT_CLASS (mailtc_config_dialog_parent_class)->finalize (object);
}

static void
mailtc_config_dialog_constructed (GObject* object)
{
    MailtcConfigDialog* dialog;
    MailtcConfigDialogPrivate* priv;
    MailtcSettings* settings;
    GtkWidget* notebook;
    GtkWidget* page_general;
    GtkWidget* label_general;
    GtkWidget* page_accounts;
    GtkWidget* label_accounts;
    GdkColor colour;
    const gchar* str;
    guint u;

    dialog = MAILTC_CONFIG_DIALOG (object);
    priv = dialog->priv;

    settings = dialog->settings;
    g_assert (MAILTC_IS_SETTINGS (settings));

    priv->accounts = mailtc_settings_get_accounts (settings);
    g_assert (priv->accounts);

    priv->modules = mailtc_settings_get_modules (settings);
    g_assert (priv->modules);

    gtk_window_set_title (GTK_WINDOW (dialog), PACKAGE " Configuration");
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
    gtk_window_set_default_size (GTK_WINDOW (dialog), 100, 100);
    gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_PREFERENCES);
    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                            GTK_STOCK_SAVE, GTK_RESPONSE_OK,
                            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                            NULL);

    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_container_set_border_width (GTK_CONTAINER (notebook), 6);

    label_general = gtk_label_new ("General");
    page_general = mailtc_config_dialog_page_general (dialog);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_general, label_general);

    label_accounts = gtk_label_new ("Mail Accounts");
    page_accounts = mailtc_config_dialog_page_accounts (dialog);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_accounts, label_accounts);

    g_signal_connect_after (dialog, "destroy",
            G_CALLBACK (mailtc_config_dialog_destroy_cb), NULL);
    g_signal_connect (dialog, "delete-event",
            G_CALLBACK (mailtc_config_dialog_delete_event_cb), NULL);
    g_signal_connect (dialog, "response",
            G_CALLBACK (mailtc_config_dialog_response_cb), NULL);

    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), notebook, FALSE, 0, 0);

    gtk_widget_show (label_general);
    gtk_widget_show (label_accounts);
    gtk_widget_show (notebook);

    str = mailtc_settings_get_command (settings);
    gtk_entry_set_text (GTK_ENTRY (priv->entry_command), str ? str : "");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->spin_interval), (gdouble) mailtc_settings_get_interval (settings));
    u = mailtc_settings_get_neterror (settings);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->spin_connections), (gdouble) u);
    gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_errordlg), (u > 2) ? 2 : u);

    mailtc_settings_get_iconcolour (settings, &colour);
    mailtc_envelope_set_colour (MAILTC_ENVELOPE (priv->envelope_config), &colour);

    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

    G_OBJECT_CLASS (mailtc_config_dialog_parent_class)->constructed (object);
}

static void
mailtc_config_dialog_class_init (MailtcConfigDialogClass* klass)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->constructed = mailtc_config_dialog_constructed;
    gobject_class->finalize = mailtc_config_dialog_finalize;
    gobject_class->set_property = mailtc_config_dialog_set_property;
    gobject_class->get_property = mailtc_config_dialog_get_property;

    g_object_class_install_property (gobject_class,
                                     PROP_SETTINGS,
                                     g_param_spec_object (
                                     "settings",
                                     "Settings",
                                     "The settings",
                                     MAILTC_TYPE_SETTINGS,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_type_class_add_private (klass, sizeof (MailtcConfigDialogPrivate));
}

static void
mailtc_config_dialog_init (MailtcConfigDialog* dialog)
{
    MailtcConfigDialogPrivate* priv;

    priv = dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog, MAILTC_TYPE_CONFIG_DIALOG, MailtcConfigDialogPrivate);
    dialog->settings = NULL;

    priv->dialog_account = NULL;
    priv->dialog_extension = NULL;
    priv->spin_interval = NULL;
    priv->entry_command = NULL;
    priv->envelope_config = NULL;
    priv->combo_errordlg = NULL;
    priv->spin_connections = NULL;
    priv->tree_view = NULL;
    priv->entry_name = NULL;
    priv->entry_server = NULL;
    priv->entry_port = NULL;
    priv->entry_user = NULL;
    priv->entry_password = NULL;
    priv->envelope_account = NULL;
    priv->combo_extension = NULL;

    priv->entry_insert_text_id = 0;
    priv->button_edit_columns_changed_id = 0;
    priv->button_edit_cursor_changed_id = 0;
    priv->button_remove_columns_changed_id = 0;
    priv->button_remove_cursor_changed_id = 0;

}

GtkWidget*
mailtc_config_dialog_new (MailtcSettings* settings)
{
    return g_object_new (MAILTC_TYPE_CONFIG_DIALOG, "settings", settings, NULL);
}
