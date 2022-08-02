/* mtc-util.c
 * Copyright (C) 2009-2022 Dale Whittaker
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

#include "mtc-util.h"

#include <glib/gstdio.h>

gchar*
mailtc_current_time (void)
{
    GDateTime* dt;
    gint64 realtime;
    gchar* str;

    realtime = g_get_real_time () / G_USEC_PER_SEC;

    dt = g_date_time_new_from_unix_utc (realtime);
    if (!dt)
        return NULL;

    str = g_date_time_format_iso8601 (dt);

    g_date_time_unref (dt);

    return str;
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

    /* TODO could allow more information to be used (e.g number of new mails, account etc) */
    args = g_strsplit (command, " ", 0);

    if (!g_spawn_async (NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error))
        mailtc_gerror (&error);

    g_strfreev (args);
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
                        guint        newvalue)
{
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (newvalue != *value)
    {
        *value = newvalue;

        g_object_notify (obj, name);
    }
}

void
mailtc_object_set_boolean (GObject*     obj,
                           GType        objtype,
                           const gchar* name,
                           gboolean*    value,
                           gboolean     newvalue)
{
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (newvalue != *value)
    {
        *value = newvalue;

        g_object_notify (obj, name);
    }
}

void
mailtc_object_set_object (GObject*     obj,
                          GType        objtype,
                          const gchar* name,
                          GObject**    value,
                          GObject*     newvalue)
{
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (newvalue != *value)
    {
        if (*value)
            g_object_unref (*value);

        *value = newvalue ? g_object_ref (newvalue) : NULL;

        g_object_notify (obj, name);
    }
}

void
mailtc_object_set_colour (GObject*            obj,
                          GType               objtype,
                          const gchar*        name,
                          MailtcColour*       colour,
                          const MailtcColour* newcolour)
{
    MailtcColour defaultcolour;

    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (!newcolour)
    {
        defaultcolour.red = defaultcolour.green = defaultcolour.blue = 1.0;
        newcolour = &defaultcolour;
    }
    if (!mailtc_colour_equal (newcolour, colour))
    {
        *colour = *newcolour;

        g_object_notify (obj, name);
    }
}

void
mailtc_object_set_array (GObject*     obj,
                         GType        objtype,
                         const gchar* name,
                         GArray**     value,
                         GArray*      newvalue)
{
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (newvalue != *value)
    {
        if (*value)
            g_array_unref (*value);

        *value = newvalue ? g_array_ref (newvalue) : NULL;

        g_object_notify (obj, name);
    }
}

void
mailtc_object_set_ptr_array (GObject*     obj,
                             GType        objtype,
                             const gchar* name,
                             GPtrArray**  value,
                             GPtrArray*   newvalue)
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

void
mailtc_object_set_pointer (GObject*     obj,
                           GType        objtype,
                           const gchar* name,
                           gpointer*    value,
                           gpointer     newvalue)
{
    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (newvalue != *value)
    {
        *value = newvalue;

        g_object_notify (obj, name);
    }
}
