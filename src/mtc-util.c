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
#include <config.h>

#include "mtc-util.h"

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
mailtc_gtk_message (GtkWidget*     parent,
                    GtkMessageType msg_type,
                    const gchar*   format,
                    ...)
{
    const gchar* icon;
    gchar* s;
    va_list args;

    va_start (args, format);
    s = g_strdup_vprintf (format, args);
    va_end (args);

    switch (msg_type)
    {
        case GTK_MESSAGE_WARNING:
            icon = "dialog-warning";
            mailtc_warning (s);
            break;
        case GTK_MESSAGE_INFO:
            icon = "dialog-information";
            mailtc_info (s);
            break;
        case GTK_MESSAGE_ERROR:
            mailtc_error (s);
            icon = "dialog-error";
            break;
        default:
            mailtc_error (s);
            icon = NULL;
    }

    if (icon)
    {
        GtkWidget* dialog;
        GtkWidget* button;
        GtkWindow* toplevel;

        toplevel = parent ? GTK_WINDOW (gtk_widget_get_toplevel (parent)) : NULL;

        dialog = gtk_message_dialog_new (toplevel,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         msg_type,
                                         GTK_BUTTONS_NONE,
                                         /*GTK_BUTTONS_OK,*/
                                         "%s", s);
        button = gtk_button_new ();
        gtk_button_set_use_underline (GTK_BUTTON (button), TRUE);
        gtk_button_set_label (GTK_BUTTON (button), "_OK");
        gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_OK);
        gtk_widget_show (button);
        gtk_window_set_title (GTK_WINDOW (dialog), PACKAGE);
        gtk_window_set_icon_name (GTK_WINDOW (dialog), icon);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        g_free (s);
    }
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
mailtc_gerror_gtk (GtkWidget* parent,
                   GError**   error)
{
    if (error && *error)
    {
        mailtc_gtk_message (parent, GTK_MESSAGE_ERROR, "%s", (*error)->message);
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
        mailtc_gerror_gtk (NULL, &error);

    g_strfreev (args);
}

gboolean
mailtc_quit (void)
{
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
mailtc_object_set_colour (GObject*        obj,
                          GType           objtype,
                          const gchar*    name,
                          GdkRGBA*        colour,
                          const GdkRGBA*  newcolour)
{
    GdkRGBA defaultcolour;

    g_assert (G_TYPE_CHECK_INSTANCE_TYPE (obj, objtype));

    if (!newcolour)
    {
        defaultcolour.red = defaultcolour.green = defaultcolour.blue = 1.0;
        newcolour = &defaultcolour;
    }
    if (!gdk_rgba_equal (newcolour, colour))
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
