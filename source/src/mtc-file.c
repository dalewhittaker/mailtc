/* mtc-file.c
 * Copyright (C) 2009 Dale Whittaker <dayul@users.sf.net>
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

#include "mtc-file.h"
#include "mtc-util.h"
#include <glib/gstdio.h>
#include <string.h>

#define CONFIG_NAME "config"

static gchar*
mailtc_directory (void)
{
    gchar* directory;

    directory = g_build_filename (g_get_home_dir (), ".config", PACKAGE, NULL);
    g_mkdir_with_parents (directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    return directory;
}

gchar*
mailtc_file (mtc_config*  config,
             const gchar* filename)
{
    gchar* directory;
    gchar* absfilename;

    if (!config || !config->directory)
    {
        directory = mailtc_directory ();
        if (!directory)
        {
            mailtc_error ("Failed to create " PACKAGE  " directory");
            return NULL;
        }
        if (config)
            config->directory = directory;
    }
    else
        directory = config->directory;

    absfilename = g_build_filename (directory, filename, NULL);

    if (!config)
        g_free (directory);

    return absfilename;
}

gboolean
mailtc_save_config (mtc_config* config,
                    GError**    error)
{
    GKeyFile* key_file;
    GSList* list;
    mtc_account* account;
    gchar* colour;
    gchar* filename;
    gchar* key_group;
    gchar* password;
    guint i;

    *error = NULL;
    list = config->accounts;
    i = 0;

    filename = mailtc_file (config, CONFIG_NAME);

    if (g_file_test (filename, G_FILE_TEST_EXISTS))
        g_chmod (filename, S_IRUSR | S_IWUSR);

    key_file = g_key_file_new ();
    g_key_file_set_integer (key_file, "settings", "interval", config->interval);
    g_key_file_set_integer (key_file, "settings", "neterror", config->net_error);
    g_key_file_set_string (key_file, "settings", "command", config->mail_command);
    colour = gdk_color_to_string (config->icon_colour);
    g_key_file_set_string (key_file, "settings", "iconcolour", colour + 1);
    g_free (colour);

    while (list)
    {
        account = (mtc_account*) list->data;
        key_group = g_strdup_printf ("account%u", i++);
        g_key_file_set_string (key_file, key_group, "name", account->name);
        g_key_file_set_string (key_file, key_group, "server", account->server);
        g_key_file_set_integer (key_file, key_group, "port", account->port);
        g_key_file_set_string (key_file, key_group, "user", account->user);
        g_key_file_set_string (key_file, key_group, "plugin",
                               g_module_name (account->plugin->module));
        g_key_file_set_integer (key_file, key_group, "protocol", account->protocol);

        colour = gdk_color_to_string (account->icon_colour);
        g_key_file_set_string (key_file, key_group, "iconcolour", colour + 1);
        g_free (colour);

        password = g_base64_encode ((const guchar*) account->password, strlen (account->password));
        g_key_file_set_string (key_file, key_group, "password", password);
        g_free (password);
        g_free (key_group);

        if (*error)
            break;

        list = g_slist_next (list);
    }
    g_key_file_set_integer (key_file, "settings", "naccounts", i);

    if (!*error)
    {
        gchar* data;

        data = g_key_file_to_data (key_file, NULL, error);
        if (data)
        {
            g_file_set_contents (filename, data, -1, error);
            g_free (data);
        }
    }
    g_key_file_free (key_file);

    if (g_file_test (filename, G_FILE_TEST_EXISTS))
        g_chmod (filename, S_IRUSR);

    g_free (filename);
    return (*error) ? FALSE : TRUE;
}

static GdkColor*
mailtc_string_to_colour (const gchar* colourstring)
{
    GdkColor colour;

    if (colourstring)
    {
        guint64 colourval;

        colourval = g_ascii_strtoull (colourstring, NULL, 16);
        colour.red = (guint16)((colourval >> 32) & 0xFFFF);
        colour.green = (guint16)((colourval >> 16) & 0xFFFF);
        colour.blue = (guint16)(colourval & 0xFFFF);
    }
    else
        colour.red = colour.green = colour.blue = 0xFFFF;

    return gdk_color_copy (&colour);
}

gboolean
mailtc_load_config (mtc_config* config,
                    GError**    error)
{
    GKeyFile* key_file;
    gchar* filename;
    gchar* colourstring;

    filename = mailtc_file (config, CONFIG_NAME);

    if (g_file_test (filename, G_FILE_TEST_EXISTS))
        g_chmod (filename, S_IRUSR | S_IWUSR);

    key_file = g_key_file_new ();
    if (g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, error))
    {
        guint n = 0;

        config->interval = g_key_file_get_integer (key_file, "settings", "interval", error);
        if (!*error)
            config->net_error = g_key_file_get_integer (key_file, "settings", "neterror", error);
        if (!*error)
            config->mail_command = g_key_file_get_string (key_file, "settings", "command", error);
        if (!*error)
        {
            colourstring = g_key_file_get_string (key_file, "settings", "iconcolour", error);
            config->icon_colour = mailtc_string_to_colour (colourstring);
            g_free (colourstring);
        }
        if (!*error)
            n = g_key_file_get_integer (key_file, "settings", "naccounts", error);
        if (!*error)
        {
            GSList* list;
            mtc_account* account;
            mtc_plugin* plugin;
            guint i;
            gchar* key_group;
            gboolean success;

            for (i = 0; i < n; i++)
            {
                success = FALSE;
                key_group = g_strdup_printf ("account%u", i);
                if (!g_key_file_has_group (key_file, key_group))
                {
                    g_free (key_group);
                    break;
                }

                account = g_new0 (mtc_account, 1);

                account->name = g_key_file_get_string (key_file, key_group, "name", error);
                if (!*error)
                    account->server = g_key_file_get_string (key_file, key_group, "server", error);
                if (!*error)
                    account->port = g_key_file_get_integer (key_file, key_group, "port", error);
                if (!*error)
                    account->user = g_key_file_get_string (key_file, key_group, "user", error);
                if (!*error)
                    account->protocol = g_key_file_get_integer (key_file, key_group, "protocol", error);
                if (!*error)
                {
                    gchar* plugin_name;

                    plugin_name = g_key_file_get_string (key_file, key_group, "plugin", error);

                    list = config->plugins;
                    while (list)
                    {
                        plugin = (mtc_plugin*) list->data;

                        if (g_str_equal (plugin_name, g_module_name (plugin->module)))
                        {
                            account->plugin = plugin;
                            break;
                        }
                        list = g_slist_next (list);
                    }
                    g_free (plugin_name);
                }
                if (!*error)
                {
                    colourstring = g_key_file_get_string (key_file, key_group, "iconcolour", error);
                    account->icon_colour = mailtc_string_to_colour (colourstring);
                    g_free (colourstring);
                }

                if (!*error)
                {
                    gchar* password;
                    gsize len;

                    password = g_key_file_get_string (key_file, key_group, "password", error);
                    account->password = (gchar*) g_base64_decode (password, &len);
                    g_free (password);

                    if (*error)
                    {
                        g_free (account->password);
                        account->password = NULL;
                    }
                }
                if (account->name &&
                    account->server &&
                    account->port &&
                    account->user &&
                    account->password &&
                    account->plugin)
                {
                    if (plugin->add_account)
                        success = (*plugin->add_account) (config, account, error);
                    else
                        success = TRUE;
                }
                if (success)
                    config->accounts = g_slist_append (config->accounts, account);
                else
                    mailtc_free_account (account, error);

                g_free (key_group);
            }
        }
    }
    g_key_file_free (key_file);

    if (g_file_test (filename, G_FILE_TEST_EXISTS))
        g_chmod (filename, S_IRUSR);

    g_free (filename);
    return (*error) ? FALSE : TRUE;
}

