/* main.c
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
#include "mtc-account.h"
#include "mtc-application.h"
#include "mtc-configdialog.h"
#include "mtc-settings.h"
#include "mtc-statusicon.h"
#include "mtc-util.h"

static void
mailtc_read_mail (GPtrArray* accounts)
{
    MailtcAccount* account;
    const mtc_plugin* plugin;
    guint i;
    GError* error = NULL;

    for (i = 0; i < accounts->len; i++)
    {
        account = g_ptr_array_index (accounts, i);
        g_assert (MAILTC_IS_ACCOUNT (account));

        plugin = mailtc_account_get_plugin (account);

        if (plugin->read_messages)
        {
            if (!(*plugin->read_messages) (G_OBJECT (account), &error))
                mailtc_gerror (&error);
        }
    }
}

static void
mailtc_read_mail_cb (MailtcStatusIcon* statusicon,
                     MailtcSettings*   settings)
{
    GPtrArray* accounts;
    const gchar* command;

    mailtc_status_icon_clear (statusicon);

    command = mailtc_settings_get_command (settings);

    if (command)
        mailtc_run_command (command);

    accounts = mailtc_settings_get_accounts (settings);
    if (accounts)
    {
        mailtc_read_mail (accounts);
        g_ptr_array_unref (accounts);
    }
}

static void
mailtc_mark_as_read_cb (MailtcStatusIcon* statusicon,
                        GPtrArray*        accounts)
{
    mailtc_status_icon_clear (statusicon);
    mailtc_read_mail (accounts);
}

static gboolean
mailtc_mail_thread (MailtcApplication* app)
{
    if (!GPOINTER_TO_INT (g_object_get_data (G_OBJECT (app), "locked"))) /* FIXME */
    {
        MailtcSettings* settings;
        MailtcStatusIcon* statusicon;
        MailtcAccount* account;
        const mtc_plugin* plugin;
        GPtrArray* accounts;
        GError* error = NULL;
        GString* err_msg = NULL;
        gboolean debug;
        gint64 messages;
        guint error_count;
        guint net_error;
        guint i;
        guint id = 0;

        g_object_set_data (G_OBJECT (app), "locked", GINT_TO_POINTER (TRUE)); /* FIXME */
        settings = mailtc_application_get_settings (app);
        statusicon = mailtc_application_get_status_icon (app);
        debug = mailtc_application_get_debug (app);
        net_error = mailtc_settings_get_neterror (settings);
        accounts = mailtc_settings_get_accounts (settings);
        g_assert (accounts);

        for (i = 0; i < accounts->len; i++)
        {
            account = g_ptr_array_index (accounts, i);
            g_assert (MAILTC_IS_ACCOUNT (account));

            plugin = mailtc_account_get_plugin (account);

            error_count = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (app), "error_count")); /* FIXME */

            if (plugin->get_messages)
            {
                messages = (*plugin->get_messages) (G_OBJECT (account), debug, &error);
                if (messages >= 0 && !error)
                {
                    mailtc_status_icon_update (statusicon, id++, messages);
                    error_count = 0;
                }
                else
                {
                    error_count++;
                    if (net_error == error_count)
                    {
                        if (error)
                        {
                            mailtc_application_set_log_glib (app);
                            mailtc_gerror (&error);
                            mailtc_application_set_log_gtk (app);
                        }
                        if (!err_msg)
                            err_msg = g_string_new (NULL);

                        err_msg = g_string_prepend (err_msg, "\n");
                        err_msg = g_string_prepend (err_msg, mailtc_account_get_server (account));
                        error_count = 0;
                    }
                    if (error)
                        g_clear_error (&error);
                }
            }
            g_object_set_data (G_OBJECT (app), "error_count", GUINT_TO_POINTER (error_count)); /* FIXME */
        }
        g_ptr_array_unref (accounts);
        g_object_unref (statusicon);
        g_object_unref (settings);

        if (err_msg)
        {
            mailtc_warning ("There was an error connecting to the following servers:\n\n%s\n"
                            "Please check the " PACKAGE " log for the error.", err_msg->str);
            g_string_free (err_msg, TRUE);
        }
        g_object_set_data (G_OBJECT (app), "locked", GINT_TO_POINTER (FALSE)); /* FIXME */
    }
    return TRUE;
}

static gboolean
mailtc_mail_thread_once (MailtcApplication* app)
{
    mailtc_mail_thread (app);
    return FALSE;
}

static guint
mailtc_run_main_loop (MailtcApplication* app)
{
    MailtcStatusIcon* statusicon;
    MailtcSettings* settings;
    MailtcAccount* account;
    GPtrArray* accounts;
    GdkColor icon_colour;
    guint i;

    settings = mailtc_application_get_settings (app);
    accounts = mailtc_settings_get_accounts (settings);
    g_assert (accounts);

    statusicon = mailtc_status_icon_new ();
    mailtc_application_set_status_icon (app, statusicon);

    g_object_set_data (G_OBJECT (app), "locked", GINT_TO_POINTER (FALSE)); /* FIXME */

    mailtc_settings_get_iconcolour (settings, &icon_colour);
    mailtc_status_icon_set_default_colour (statusicon, &icon_colour);

    for (i = 0; i < accounts->len; i++)
    {
        account = g_ptr_array_index (accounts, i);
        mailtc_account_get_iconcolour (account, &icon_colour);
        mailtc_status_icon_add_item (statusicon, mailtc_account_get_name (account), &icon_colour);
    }
    g_ptr_array_unref (accounts);
    g_object_unref (statusicon);
    g_object_unref (settings);

    g_signal_connect (statusicon, "read-mail",
                G_CALLBACK (mailtc_read_mail_cb), settings);
    g_signal_connect (statusicon, "mark-as-read",
                G_CALLBACK (mailtc_mark_as_read_cb), accounts);

    g_idle_add ((GSourceFunc) mailtc_mail_thread_once, app);

    return g_timeout_add_seconds (60 * mailtc_settings_get_interval (settings),
                                  (GSourceFunc) mailtc_mail_thread,
                                  app);
}

void
mailtc_terminate_cb (MailtcApplication* app,
                     gpointer           user_data)
{

    guint source_id;

    (void) app;

    source_id = GPOINTER_TO_UINT (user_data);

    if (source_id > 0)
        g_source_remove (source_id);
}

void
mailtc_run_cb (MailtcApplication* app)
{
    guint source_id;

    source_id = mailtc_run_main_loop (app);

    g_signal_connect (app, "terminate",
            G_CALLBACK (mailtc_terminate_cb), GUINT_TO_POINTER (source_id));
}

void
mailtc_configure_cb (MailtcApplication* app)
{
    MailtcSettings* settings;
    GtkWidget* dialog;

    settings = mailtc_application_get_settings (app);
    dialog = mailtc_config_dialog_new (settings);

    g_object_unref (settings);
    gtk_widget_show (GTK_WIDGET (dialog));
}

int
main (int    argc,
      char** argv)
{
    MailtcApplication* app;
    int status;

    app = mailtc_application_new ();

    g_signal_connect (app, "configure", G_CALLBACK (mailtc_configure_cb), NULL);
    g_signal_connect (app, "run", G_CALLBACK (mailtc_run_cb), NULL);

    status = g_application_run (G_APPLICATION (app), argc, argv);

    g_object_unref (app);

    return status;
}

