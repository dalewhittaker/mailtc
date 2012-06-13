/* mtc-mail.c
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

#include "mtc-mail.h"
#include "mtc-statusicon.h"
#include "mtc-util.h"

static void
mailtc_read_mail (GPtrArray* accounts)
{
    mtc_plugin* plugin;
    mtc_account* account;
    guint i;
    GError* error = NULL;

    for (i = 0; i < accounts->len; i++)
    {
        account = g_ptr_array_index (accounts, i);
        g_assert (account);

        plugin = (mtc_plugin*) account->plugin;

        if (plugin->read_messages)
        {
            if (!(*plugin->read_messages) (account, &error))
                mailtc_gerror (&error);
        }
    }
}

static void
mailtc_read_mail_cb (MailtcStatusIcon* status_icon,
                     mtc_run_params*   params)
{
    mtc_config* config = params->config;

    (void) status_icon;

    mailtc_status_icon_clear (status_icon);

    if (config->mail_command && *(config->mail_command))
        mailtc_run_command (config->mail_command);

    mailtc_read_mail (params->accounts);
}

static void
mailtc_mark_as_read_cb (MailtcStatusIcon* status_icon,
                        GPtrArray*        accounts)
{
    (void) status_icon;

    mailtc_status_icon_clear (status_icon);
    mailtc_read_mail (accounts);
}

static gboolean
mailtc_mail_thread (mtc_run_params* params)
{
    mtc_config* config = params->config;

    if (!params->locked)
    {
        MailtcStatusIcon* icon;
        mtc_account* account;
        mtc_plugin* plugin;
        GPtrArray* accounts;
        GError* error = NULL;
        GString* err_msg = NULL;
        gint64 messages;
        guint i;
        guint id = 0;

        params->locked = TRUE;
        accounts = params->accounts;
        icon = MAILTC_STATUS_ICON (config->status_icon);

        for (i = 0; i < accounts->len; i++)
        {
            account = g_ptr_array_index (accounts, i);
            g_assert (account);

            plugin = (mtc_plugin*) account->plugin;

            if (plugin->get_messages)
            {
                messages = (*plugin->get_messages) (account, params->debug, &error);
                if (messages >= 0 && !error)
                {
                    mailtc_status_icon_update (icon, id++, messages);
                    params->error_count = 0;
                }
                else
                {
                    params->error_count++;
                    if (config->net_error > 0 && config->net_error == params->error_count)
                    {
                        if (error)
                        {
                            mailtc_set_log_glib (config);
                            mailtc_gerror (&error);
                            mailtc_set_log_gtk (config);
                        }
                        if (!err_msg)
                            err_msg = g_string_new (NULL);

                        err_msg = g_string_prepend (err_msg, "\n");
                        err_msg = g_string_prepend (err_msg, account->server);
                        params->error_count = 0;
                    }
                    if (error)
                        g_clear_error (&error);
                }
            }
        }

        if (err_msg)
        {
            mailtc_warning ("There was an error connecting to the following servers:\n\n%s\n"
                            "Please check the " PACKAGE " log for the error.", err_msg->str);
            g_string_free (err_msg, TRUE);
        }
        params->locked = FALSE;
    }
    return TRUE;
}

static gboolean
mailtc_mail_thread_once (mtc_run_params* params)
{
    mailtc_mail_thread (params);
    return FALSE;
}

guint
mailtc_run_main_loop (mtc_run_params* params)
{
    MailtcStatusIcon* icon;
    GPtrArray* accounts;
    mtc_config* config;
    mtc_account* account;
    guint i;

    config = params->config;
    accounts = params->accounts;

    icon = mailtc_status_icon_new ();
    config->status_icon = G_OBJECT (icon);
    params->locked = FALSE;

    mailtc_status_icon_set_default_colour (icon, config->icon_colour);

    for (i = 0; i < accounts->len; i++)
    {
        account = g_ptr_array_index (accounts, i);
        mailtc_status_icon_add_item (icon, account->name, account->icon_colour);
    }

    g_signal_connect (icon, "read-mail",
                G_CALLBACK (mailtc_read_mail_cb), params);
    g_signal_connect (icon, "mark-as-read",
                G_CALLBACK (mailtc_mark_as_read_cb), accounts);

    g_idle_add ((GSourceFunc) mailtc_mail_thread_once, params);

    return g_timeout_add_seconds (60 * config->interval,
                                  (GSourceFunc) mailtc_mail_thread,
                                  params);
}

