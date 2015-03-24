/* mtc-configdialog.c
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
    GtkWidget* about_dialog;
    GtkWidget* account_button_extension;
    GtkWidget* account_button_icon;
    GtkWidget* account_combo_protocol;
    GtkWidget* account_dialog;
    GtkWidget* account_entry_name;
    GtkWidget* account_entry_server;
    GtkWidget* account_entry_port;
    GtkWidget* account_entry_user;
    GtkWidget* account_entry_password;
    GtkWidget* account_icon;
    GtkWidget* accounts_tree_view;
    GtkWidget* accounts_button_add;
    GtkWidget* accounts_button_edit;
    GtkWidget* accounts_button_remove;
    GtkWidget* general_button_icon;
    GtkWidget* general_combo_errordlg;
    GtkWidget* general_entry_command;
    GtkWidget* general_icon;
    GtkWidget* general_label_connections;
    GtkWidget* general_spin_connections;
    GtkWidget* general_spin_interval;
    GtkWidget* config_button_close;

    GPtrArray* accounts;
    MailtcModuleManager* modules;

    gulong entry_insert_text_id;
    gulong columns_changed_id;
    gulong cursor_changed_id;
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

G_DEFINE_TYPE_WITH_CODE (MailtcConfigDialog, mailtc_config_dialog, GTK_TYPE_WINDOW, G_ADD_PRIVATE (MailtcConfigDialog))

enum
{
    PROP_0,
    PROP_SETTINGS
};

static void
mailtc_gerror_gtk (GtkWidget* parent,
                   GError**   error)
{
    if (error && *error)
    {
        mailtc_gtk_message (parent, GTK_MESSAGE_ERROR, "%s", (*error)->message);
        g_clear_error (error);
    }
}

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
mailtc_close_button_clicked_cb (GtkWidget*          button,
                                MailtcConfigDialog* dialog)
{
    (void) button;

    gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
mailtc_save_button_clicked_cb (GtkWidget*          button,
                               MailtcConfigDialog* dialog)
{
    MailtcConfigDialogPrivate* priv;
    MailtcSettings* settings;
    guint net_error;
    const gchar* colour;
    GdkRGBA rgb;
    GError* error = NULL;

    (void) button;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog));

    priv = dialog->priv;
    settings = dialog->settings;

    mailtc_settings_set_interval (settings,
            (guint) gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->general_spin_interval)));

    mailtc_settings_set_command (settings, gtk_entry_get_text (GTK_ENTRY (priv->general_entry_command)));

    mailtc_envelope_get_colour (MAILTC_ENVELOPE (priv->general_icon), &rgb);
    colour = gdk_rgba_to_string (&rgb);
    mailtc_settings_set_iconcolour (settings, colour);

    net_error = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->general_combo_errordlg));
    if (net_error > 1)
        net_error = (guint) gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->general_spin_connections));

    mailtc_settings_set_neterror (settings, net_error);

    if (!mailtc_settings_write (settings, &error))
        mailtc_gerror_gtk (GTK_WIDGET (dialog), &error);

}

static void
mailtc_combo_errordlg_changed_cb (GtkComboBox*        combo,
                                  MailtcConfigDialog* dialog)
{
    MailtcConfigDialogPrivate* priv;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog));

    priv = dialog->priv;

    if (gtk_combo_box_get_active (combo) > 1)
    {
        gtk_widget_show (priv->general_label_connections);
        gtk_widget_show (priv->general_spin_connections);
    }
    else
    {
        gtk_widget_hide (priv->general_label_connections);
        gtk_widget_hide (priv->general_spin_connections);
    }
}

static void
mailtc_button_icon_clicked_cb (GtkWidget*      button,
                               MailtcEnvelope* envelope)
{
    GtkWidget* dialog;
    GdkRGBA colour;
    gint response;

    dialog = gtk_color_chooser_dialog_new ("Select Icon Colour", GTK_WINDOW (gtk_widget_get_toplevel (button)));

    mailtc_envelope_get_colour (envelope, &colour);
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (dialog), &colour);

    response = gtk_dialog_run (GTK_DIALOG (dialog));
    if (response == GTK_RESPONSE_OK)
    {
        gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (dialog), &colour);
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
    dialog_extension = priv->about_dialog;

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->account_combo_protocol));
    g_assert (GTK_IS_TREE_MODEL (model));
    exists = gtk_combo_box_get_active_iter (GTK_COMBO_BOX  (priv->account_combo_protocol), &iter);
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

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->accounts_tree_view));
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

    exists = gtk_combo_box_get_active_iter (GTK_COMBO_BOX  (priv->account_combo_protocol), &combo_iter);
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
    GdkRGBA rgb;
    const gchar* colour;
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

    name = gtk_entry_get_text (GTK_ENTRY (priv->account_entry_name));
    server = gtk_entry_get_text (GTK_ENTRY (priv->account_entry_server));
    port = gtk_entry_get_text (GTK_ENTRY (priv->account_entry_port));
    user = gtk_entry_get_text (GTK_ENTRY (priv->account_entry_user));
    password = gtk_entry_get_text (GTK_ENTRY (priv->account_entry_password));

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->account_combo_protocol));
    g_assert (GTK_IS_TREE_MODEL (model));
    exists = gtk_combo_box_get_active_iter (GTK_COMBO_BOX  (priv->account_combo_protocol), &iter);
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
    mailtc_envelope_get_colour (MAILTC_ENVELOPE (priv->account_icon), &rgb);
    colour = gdk_rgba_to_string (&rgb);
    mailtc_account_set_iconcolour (account, colour);

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

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->account_combo_protocol));
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

    gtk_entry_set_text (GTK_ENTRY (priv->account_entry_port), port ? port : "");
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
    GError* error;
    gint result;
    gint index;

    (void) button;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog_config));
    priv = dialog_config->priv;

    dialog = priv->account_dialog;

    if (account)
    {
        const gchar* name;
        const gchar* server;
        const gchar* user;
        const gchar* password;
        const gchar* colour;
        gchar* port;
        GdkRGBA rgb;

        name = mailtc_account_get_name (account);
        server = mailtc_account_get_server (account);
        user = mailtc_account_get_user (account);
        password = mailtc_account_get_password (account);
        g_assert (name && server && user && password);

        colour = mailtc_account_get_iconcolour (account);

        port = g_strdup_printf ("%u", mailtc_account_get_port (account));
        gtk_entry_set_text (GTK_ENTRY (priv->account_entry_name), name);
        gtk_entry_set_text (GTK_ENTRY (priv->account_entry_server), server);
        gtk_entry_set_text (GTK_ENTRY (priv->account_entry_user), user);
        gtk_entry_set_text (GTK_ENTRY (priv->account_entry_password), password);
        gtk_entry_set_text (GTK_ENTRY (priv->account_entry_port), port);

        index = mailtc_combo_get_protocol_index (dialog_config, account);
        g_assert (index > -1);
        gtk_combo_box_set_active (GTK_COMBO_BOX (priv->account_combo_protocol), index);
        gdk_rgba_parse (&rgb, colour);
        mailtc_envelope_set_colour (MAILTC_ENVELOPE (priv->account_icon), &rgb);
        gtk_entry_set_text (GTK_ENTRY (priv->account_entry_port), port ? port : "");
        g_free (port);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (priv->account_entry_name), "");
        gtk_entry_set_text (GTK_ENTRY (priv->account_entry_server), "");
        gtk_entry_set_text (GTK_ENTRY (priv->account_entry_user), "");
        gtk_entry_set_text (GTK_ENTRY (priv->account_entry_password), "");

        gtk_combo_box_set_active (GTK_COMBO_BOX (priv->account_combo_protocol), 0);
        mailtc_envelope_set_colour (MAILTC_ENVELOPE (priv->account_icon), NULL);
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

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->accounts_tree_view));
    index = mailtc_tree_view_get_selected_iter (GTK_TREE_VIEW (priv->accounts_tree_view), model, &iter);
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

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->accounts_tree_view));
    index = mailtc_tree_view_get_selected_iter (GTK_TREE_VIEW (priv->accounts_tree_view),
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
mailtc_cursor_or_cols_changed (GtkTreeView* tree_view,
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
mailtc_cursor_or_cols_changed_cb (GtkTreeView*        tree_view,
                                  MailtcConfigDialog* dialog)
{

    MailtcConfigDialogPrivate* priv;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog));

    priv = dialog->priv;

    mailtc_cursor_or_cols_changed (tree_view, priv->accounts_button_edit);
    mailtc_cursor_or_cols_changed (tree_view, priv->accounts_button_remove);
}

static void
mailtc_tree_view_destroy_cb (GtkWidget*          widget,
                             MailtcConfigDialog* dialog)
{
    MailtcConfigDialogPrivate* priv;

    g_assert (MAILTC_IS_CONFIG_DIALOG (dialog));

    priv = dialog->priv;

    g_signal_handler_disconnect (widget, priv->columns_changed_id);
    g_signal_handler_disconnect (widget, priv->cursor_changed_id);
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
    MailtcAccount* account;
    MailtcExtension* extension;
    MailtcProtocol* protocol;
    GtkListStore* store;
    GdkRGBA colour;
    GArray* protocols;
    const gchar* str;
    guint i;
    guint u;

    dialog = MAILTC_CONFIG_DIALOG (object);
    priv = dialog->priv;

    settings = dialog->settings;
    g_assert (MAILTC_IS_SETTINGS (settings));

    priv->accounts = mailtc_settings_get_accounts (settings);
    g_assert (priv->accounts);

    priv->modules = mailtc_settings_get_modules (settings);
    g_assert (priv->modules);

    g_type_ensure (MAILTC_TYPE_ENVELOPE);
    g_type_ensure (MAILTC_TYPE_EXTENSION);

    gtk_window_set_title (GTK_WINDOW (dialog), PACKAGE " Configuration");
    gtk_window_set_title (GTK_WINDOW (priv->account_dialog), PACKAGE " Configuration");
    gtk_window_set_transient_for (GTK_WINDOW (priv->account_dialog), GTK_WINDOW (dialog));
    gtk_entry_set_max_length (GTK_ENTRY (priv->general_entry_command), MAILTC_PATH_LENGTH);
    gtk_entry_set_max_length (GTK_ENTRY (priv->account_entry_name), MAILTC_PATH_LENGTH);
    gtk_entry_set_max_length (GTK_ENTRY (priv->account_entry_server), MAILTC_PATH_LENGTH);
    gtk_entry_set_max_length (GTK_ENTRY (priv->account_entry_port), G_ASCII_DTOSTR_BUF_SIZE);
    gtk_entry_set_max_length (GTK_ENTRY (priv->account_entry_user), MAILTC_PATH_LENGTH);
    gtk_entry_set_max_length (GTK_ENTRY (priv->account_entry_password), MAILTC_PATH_LENGTH);

    store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (priv->accounts_tree_view)));

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

    priv->cursor_changed_id = g_signal_connect (
            priv->accounts_tree_view, "cursor-changed", G_CALLBACK (mailtc_cursor_or_cols_changed_cb), dialog);
    priv->columns_changed_id = g_signal_connect (
            priv->accounts_tree_view, "columns-changed", G_CALLBACK (mailtc_cursor_or_cols_changed_cb), dialog);
    priv->entry_insert_text_id = g_signal_connect (
            priv->account_entry_port, "insert-text", G_CALLBACK (mailtc_port_entry_insert_text_cb), priv);

    mailtc_cursor_or_cols_changed (GTK_TREE_VIEW (priv->accounts_tree_view), priv->accounts_button_edit);
    mailtc_cursor_or_cols_changed (GTK_TREE_VIEW (priv->accounts_tree_view), priv->accounts_button_remove);

    str = mailtc_settings_get_command (settings);
    gtk_entry_set_text (GTK_ENTRY (priv->general_entry_command), str ? str : "");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->general_spin_interval), (gdouble) mailtc_settings_get_interval (settings));
    u = mailtc_settings_get_neterror (settings);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->general_spin_connections), (gdouble) u);
    gtk_combo_box_set_active (GTK_COMBO_BOX (priv->general_combo_errordlg), (u > 2) ? 2 : u);

    str = mailtc_settings_get_iconcolour (settings);
    gdk_rgba_parse (&colour, str);
    mailtc_envelope_set_colour (MAILTC_ENVELOPE (priv->general_icon), &colour);

    store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->account_combo_protocol)));
    mailtc_module_manager_foreach_extension (priv->modules, (GFunc) mailtc_combo_protocol_add_items, store);

    g_signal_connect (priv->account_dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &priv->account_dialog);
    g_signal_connect (priv->account_button_icon, "clicked", G_CALLBACK (mailtc_button_icon_clicked_cb), priv->account_icon);
    g_signal_connect (priv->general_button_icon, "clicked", G_CALLBACK (mailtc_button_icon_clicked_cb), priv->general_icon);
    gtk_widget_show_all (GTK_WIDGET (dialog));

    G_OBJECT_CLASS (mailtc_config_dialog_parent_class)->constructed (object);
}

static void
mailtc_config_dialog_class_init (MailtcConfigDialogClass* klass)
{
    GObjectClass* gobject_class;
    GtkWidgetClass* widget_class;

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

    widget_class = GTK_WIDGET_CLASS (klass);
    gtk_widget_class_set_template_from_resource (widget_class, "/org/mailtc/ui/configdialog.xml");

    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, about_dialog);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, account_button_extension);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, account_button_icon);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, account_dialog);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, account_icon);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, account_entry_name);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, account_entry_server);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, account_entry_port);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, account_entry_user);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, account_entry_password);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, account_combo_protocol);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, accounts_button_add);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, accounts_button_edit);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, accounts_button_remove);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, accounts_tree_view);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, general_button_icon);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, general_combo_errordlg);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, general_entry_command);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, general_icon);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, general_label_connections);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, general_spin_connections);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, general_spin_interval);
    gtk_widget_class_bind_template_child_private (widget_class, MailtcConfigDialog, config_button_close);

    gtk_widget_class_bind_template_callback (widget_class, gtk_widget_hide_on_delete);
    gtk_widget_class_bind_template_callback (widget_class, mailtc_button_extension_clicked_cb);
    gtk_widget_class_bind_template_callback (widget_class, mailtc_combo_errordlg_changed_cb);
    gtk_widget_class_bind_template_callback (widget_class, mailtc_combo_protocol_changed_cb);
    gtk_widget_class_bind_template_callback (widget_class, mailtc_config_dialog_delete_event_cb);
    gtk_widget_class_bind_template_callback (widget_class, mailtc_config_dialog_destroy_cb);
    gtk_widget_class_bind_template_callback (widget_class, mailtc_save_button_clicked_cb);
    gtk_widget_class_bind_template_callback (widget_class, mailtc_close_button_clicked_cb);
    gtk_widget_class_bind_template_callback (widget_class, mailtc_add_button_clicked_cb);
    gtk_widget_class_bind_template_callback (widget_class, mailtc_edit_button_clicked_cb);
    gtk_widget_class_bind_template_callback (widget_class, mailtc_remove_button_clicked_cb);
    gtk_widget_class_bind_template_callback (widget_class, mailtc_tree_view_destroy_cb);
}

static void
mailtc_config_dialog_init (MailtcConfigDialog* dialog)
{
    MailtcConfigDialogPrivate* priv;

    gtk_widget_init_template (GTK_WIDGET (dialog));

    priv = dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog, MAILTC_TYPE_CONFIG_DIALOG, MailtcConfigDialogPrivate);
    dialog->settings = NULL;

    priv->entry_insert_text_id = 0;
    priv->columns_changed_id = 0;
    priv->cursor_changed_id = 0;
}

GtkWidget*
mailtc_config_dialog_new (MailtcSettings* settings)
{
    return g_object_new (MAILTC_TYPE_CONFIG_DIALOG, "settings", settings, NULL);
}
