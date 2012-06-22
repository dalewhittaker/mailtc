/* mtc-util.c
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

#include "mtc-util.h"
#include "mtc-application.h"

#include <gtk/gtk.h>
#include <glib/gstdio.h>

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

gchar*
mailtc_directory (void)
{
    gchar* directory;

    directory = g_build_filename (g_get_user_config_dir (), PACKAGE, NULL);
    g_mkdir_with_parents (directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    return directory;
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
mailtc_quit (void)
{
    mailtc_application_set_log_glib (NULL);
    gtk_main_quit ();

    return FALSE;
}

