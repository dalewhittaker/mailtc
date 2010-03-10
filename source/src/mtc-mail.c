/* mtc-mail.c
 * Copyright (C) 2009-2010 Dale Whittaker <dayul@users.sf.net>
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
mailtc_read_mail (mtc_config* config)
{
    GSList* list = config->accounts;
    GError* error = NULL;
    mtc_plugin* plugin;
    mtc_account* account;

    while (list)
    {
        account = (mtc_account*) list->data;
        plugin = (mtc_plugin*) account->plugin;

        if (plugin->read_messages)
        {
            if (!(*plugin->read_messages) (config, account, &error))
                mailtc_gerror (&error);
        }
        list = g_slist_next (list);
    }
}

static void
mailtc_read_mail_cb (MailtcStatusIcon* status_icon,
                     mtc_config*       config)
{
    (void) status_icon;

    mailtc_status_icon_clear (status_icon);

    if (config->mail_command && *(config->mail_command))
        mailtc_run_command (config->mail_command);

    mailtc_read_mail (config);
}

static void
mailtc_mark_as_read_cb (MailtcStatusIcon* status_icon,
                        mtc_config*       config)
{
    (void) status_icon;

    mailtc_status_icon_clear (status_icon);
    mailtc_read_mail (config);
}

gboolean
mailtc_mail_thread (mtc_config* config)
{

   if (!config->locked)
   {
        MailtcStatusIcon* icon;
        mtc_account* account;
        mtc_plugin* plugin;
        GSList* list;
        GError* error = NULL;
        GString* err_msg = NULL;
        gint64 messages;
        guint id = 0;

        config->locked = TRUE;
        list = config->accounts;
        icon = MAILTC_STATUS_ICON (config->status_icon);

        while (list)
        {
            account = (mtc_account*) list->data;
            plugin = (mtc_plugin*) account->plugin;

            if (plugin->get_messages)
            {
                messages = (*plugin->get_messages) (config, account, &error);
                if (messages >= 0 && !error)
                {
                    mailtc_status_icon_update (icon, id++, messages);
                    config->error_count = 0;
                }
                else
                {
                    config->error_count++;
                    if (config->net_error > 0 && config->net_error == config->error_count)
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
                        config->error_count = 0;
                    }
                    if (error)
                        g_clear_error (&error);
                }
            }
            list = g_slist_next (list);
        }

        if (err_msg)
        {
            mailtc_warning ("There was an error connecting to the following servers:\n\n%s\n"
                            "Please check the " PACKAGE " log for the error.", err_msg->str);
            g_string_free (err_msg, TRUE);
        }
        config->locked = FALSE;
   }
   return TRUE;
}

gboolean
mailtc_mail_thread_once (mtc_config* config)
{
    mailtc_mail_thread (config);
    return FALSE;
}

void
mailtc_run_main_loop (mtc_config* config)
{
    MailtcStatusIcon* icon;
    GSList* list;
    mtc_account* account;

    list = config->accounts;
    icon = mailtc_status_icon_new ();
    config->status_icon = G_OBJECT (icon);
    config->locked = FALSE;

    mailtc_status_icon_set_default_colour (icon, config->icon_colour);

    while (list)
    {
        account = (mtc_account*) list->data;

        mailtc_status_icon_add_item (icon, account->name, account->icon_colour);
        list = g_slist_next (list);
    }

    g_signal_connect (icon, "read-mail",
                G_CALLBACK (mailtc_read_mail_cb), config);
    g_signal_connect (icon, "mark-as-read",
                G_CALLBACK (mailtc_mark_as_read_cb), config);

    g_idle_add ((GSourceFunc) mailtc_mail_thread_once, config);

    config->source_id = g_timeout_add_seconds (60 * config->interval,
                                               (GSourceFunc) mailtc_mail_thread,
                                               config);
}

