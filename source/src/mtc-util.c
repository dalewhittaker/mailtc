/* mtc-util.c
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

#include "mtc-util.h"
#include "mtc-config.h"
#include "mtc-plugin.h"
#include "mtc-statusicon.h"

gchar*
mailtc_current_time (void)
{
    GTimeVal timeval;

    g_get_current_time (&timeval);
    return g_time_val_to_iso8601 (&timeval);
}

void
mailtc_info (const gchar* format,
             ...)
{
    va_list args;
    va_start (args, format);
    g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format, args);
    va_end (args);
}

void
mailtc_gerror (GError** error)
{
    if (error && *error)
    {
        mailtc_error ("%s", (*error)->message);
        g_clear_error (error);
    }
}

void
mailtc_gerror_warn (GError** error)
{
    if (error && *error)
    {
        mailtc_warning ("%s", (*error)->message);
        g_clear_error (error);
    }
}

void
mailtc_log (GIOChannel*  log,
            const gchar* message)
{
    if (log)
    {
        gchar* stime;
        gchar* s;
        gsize bytes;

        stime = mailtc_current_time ();
        s = g_strdup_printf ("%s : %s\n", stime, message);
        g_free (stime);

        g_io_channel_write_chars (log, s, -1, &bytes, NULL);
        g_io_channel_flush (log, NULL);
        g_free (s);
    }
}

static void
mailtc_gtk_handler (const gchar*   log_domain,
                    GLogLevelFlags log_level,
                    const gchar*   message,
                    mtc_config*    config)
{
    gchar* s;
    GtkMessageType msg_type;
    const gchar* icon = NULL;

    (void) log_domain;

    s = g_strdup (message);
    g_strchomp (s);

    switch (log_level & G_LOG_LEVEL_MASK)
    {
        case G_LOG_LEVEL_ERROR:
            msg_type = GTK_MESSAGE_QUESTION;
            break;
        case G_LOG_LEVEL_CRITICAL:
            msg_type = GTK_MESSAGE_ERROR;
            icon = GTK_STOCK_DIALOG_ERROR;
            break;
        case G_LOG_LEVEL_WARNING:
            msg_type = GTK_MESSAGE_WARNING;
            icon = GTK_STOCK_DIALOG_WARNING;
            break;
        case G_LOG_LEVEL_INFO:
            msg_type = GTK_MESSAGE_INFO;
            icon = GTK_STOCK_DIALOG_INFO;
            break;
        default:
            msg_type = GTK_MESSAGE_OTHER;
    }

    if (msg_type == GTK_MESSAGE_QUESTION)
        g_printerr ("%s\n", s);
    else
    {
        GtkWidget* toplevel;
        GtkWidget* dialog;
        mtc_prefs* prefs = NULL;

        if (config)
            prefs = config->prefs;

        toplevel = (prefs && prefs->dialog_config) ?
                    gtk_widget_get_toplevel (prefs->dialog_config) : NULL;

        dialog = gtk_message_dialog_new (toplevel && gtk_widget_is_toplevel (toplevel) ?
                                         GTK_WINDOW (toplevel) : NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         msg_type,
                                         GTK_BUTTONS_OK,
                                         "%s", s);
        gtk_window_set_title (GTK_WINDOW (dialog), PACKAGE);
        if (icon)
            gtk_window_set_icon_name (GTK_WINDOW (dialog), icon);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
    }
    mailtc_log (config->log, s);
    g_free (s);
}

static void
mailtc_glib_handler (const gchar*   log_domain,
                     GLogLevelFlags log_level,
                     const gchar*   message,
                     GIOChannel*    log)
{
    gchar* s;

    (void) log_domain;

    s = g_strdup (message);
    g_strchomp (s);

    switch (log_level & G_LOG_LEVEL_MASK)
    {
        case G_LOG_LEVEL_MESSAGE:
        case G_LOG_LEVEL_INFO:
        case G_LOG_LEVEL_DEBUG:
            g_print ("%s\n", s);
            break;
        default:
            g_printerr ("%s\n", s);
    }
    mailtc_log (log, s);
    g_free (s);
}

void
mailtc_set_log_glib (mtc_config* config)
{
    g_log_set_fatal_mask (NULL, G_LOG_LEVEL_ERROR);
    g_log_set_handler (NULL,
                       G_LOG_LEVEL_MASK |
                       G_LOG_FLAG_FATAL |
                       G_LOG_FLAG_RECURSION,
                       (GLogFunc) mailtc_glib_handler,
                       config ? config->log : NULL);
}

void
mailtc_set_log_gtk (mtc_config* config)
{
    g_log_set_fatal_mask (NULL, G_LOG_LEVEL_ERROR);
    g_log_set_handler (NULL,
                       G_LOG_LEVEL_MASK |
                       G_LOG_FLAG_FATAL |
                       G_LOG_FLAG_RECURSION,
                       (GLogFunc) mailtc_gtk_handler,
                       config);
}

void
mailtc_run_command (const gchar* command)
{
    GError* error;
    gchar** args;

    error = NULL;

    /* TODO could allow more information to be used
     * (e.g number of new mails, account etc)
     */
    args = g_strsplit (command, " ", 0);

    if (!g_spawn_async (NULL, args, NULL, G_SPAWN_SEARCH_PATH,
                        NULL, NULL, NULL, &error))
        mailtc_gerror (&error);

    g_strfreev (args);
}

void
mailtc_free_account (mtc_account* account,
                     GError**     error)
{
    (void) error;

    if (account)
    {
        mtc_plugin* plugin = account->plugin;

        if (plugin && plugin->remove_account)
            (*plugin->remove_account) (account, error);

        if (account->icon_colour)
            gdk_color_free (account->icon_colour);
        g_free (account->name);
        g_free (account->server);
        g_free (account->user);
        g_free (account->password);
        g_free (account);
    }
}

gboolean
mailtc_free_config (mtc_config* config,
                    GError**    error)
{
    gboolean success = TRUE;

    if (config)
    {
        GSList* list;

        g_free (config->directory);
        g_free (config->prefs);
        g_free (config->mail_command);

        if (MAILTC_IS_STATUS_ICON (config->status_icon))
            g_object_unref (config->status_icon);

        if (config->icon_colour)
            gdk_color_free (config->icon_colour);

        list = config->accounts;

        while (list)
        {
            mailtc_free_account ((mtc_account*) list->data, error);
            list = g_slist_next (list);
        }
        g_slist_free (config->accounts);

        success = mailtc_unload_plugins (config, error);
        g_free (config);
    }
    return success;
}

gboolean
mailtc_quit (void)
{
    mailtc_set_log_glib (NULL);
    gtk_main_quit ();

    return FALSE;
}

