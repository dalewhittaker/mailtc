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

gboolean
mailtc_quit (void)
{
    mailtc_application_set_log_glib (NULL);
    gtk_main_quit ();

    return FALSE;
}

void
mailtc_object_set_string (GObject*     obj,
                          GType        objtype,
                          const gchar* name,
                          gchar**      value,
                          const gchar* newvalue)
{
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (g_strcmp0 (newvalue, *value) != 0)
    {
        g_free (*value);
        *value = g_strdup (newvalue);

        g_object_notify (obj, name);
    }
}

void
mailtc_object_set_uint (GObject*     obj,
                        GType        objtype,
                        const gchar* name,
                        guint*       value,
                        const guint  newvalue)
{
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (newvalue != *value)
    {
        *value = newvalue;

        g_object_notify (obj, name);
    }
}

void
mailtc_object_set_colour (GObject*        obj,
                          GType           objtype,
                          const gchar*    name,
                          GdkColor*       colour,
                          const GdkColor* newcolour)
{
    GdkColor defaultcolour;

    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (!newcolour)
    {
        defaultcolour.red = defaultcolour.green = defaultcolour.blue = 0xFFFF;
        newcolour = &defaultcolour;
    }
    if (!gdk_color_equal (newcolour, colour))
    {
        colour->red = newcolour->red;
        colour->green = newcolour->green;
        colour->blue = newcolour->blue;

        g_object_notify (obj, name);
    }
}

void
mailtc_object_set_ptr_array (GObject*         obj,
                             GType            objtype,
                             const gchar*     name,
                             GPtrArray**      value,
                             GPtrArray*       newvalue)
{
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (newvalue != *value)
    {
        if (*value)
            g_ptr_array_unref (*value);

        *value = newvalue ? g_ptr_array_ref (newvalue) : NULL;

        g_object_notify (obj, name);
    }
}

