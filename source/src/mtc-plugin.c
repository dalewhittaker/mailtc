/* mtc-plugin.c
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

#include "mtc-plugin.h"
#include "mtc-file.h"
#include <glib/gstdio.h>

#define MAILTC_PLUGIN_ERROR g_quark_from_string("MAILTC_PLUGIN_ERROR")

typedef enum
{
    MAILTC_PLUGIN_ERROR_SUPPORTED = 0,
    MAILTC_PLUGIN_ERROR_OPEN,
    MAILTC_PLUGIN_ERROR_SYMBOL,
    MAILTC_PLUGIN_ERROR_COMPATIBILITY,
    MAILTC_PLUGIN_ERROR_EMPTY,
    MAILTC_PLUGIN_ERROR_CLOSE,

} MailtcPluginError;

static gint
mailtc_plugin_compare (mtc_plugin* a,
                       mtc_plugin* b)
{
    g_return_val_if_fail (a && b && a->name && b->name, 0);

    return g_ascii_strcasecmp (a->name, b->name);
}

static gboolean
mailtc_load_plugin (mtc_config*  config,
                    const gchar* dirname,
                    const gchar* filename,
                    GError**     error)
{
    GModule* module;
    mtc_plugin* plugin;
    mtc_plugin* (*plugin_init) (void);
    gchar* fullname;
    gboolean errorset;

    fullname = g_build_filename (dirname, filename, NULL);
    module = g_module_open (fullname, G_MODULE_BIND_LOCAL);
    g_free (fullname);

    errorset = *error ? TRUE : FALSE;

    if (!module)
    {
        if (!errorset)
        {
            *error = g_error_new (MAILTC_PLUGIN_ERROR,
                                  MAILTC_PLUGIN_ERROR_OPEN,
                                  "Error loading plugin: %s: %s",
                                  filename,
                                  g_module_error ());
        }
        return FALSE;
    }

    if (!g_module_symbol (module, "plugin_init", (gpointer) &plugin_init))
    {
        if (!errorset)
        {
            *error = g_error_new (MAILTC_PLUGIN_ERROR,
                                  MAILTC_PLUGIN_ERROR_SYMBOL,
                                  "Error getting pointer to symbol in %s: %s",
                                  filename,
                                  g_module_error ());
        }
        g_module_close (module);
        return FALSE;;
    }

    plugin = plugin_init ();
    if (plugin)
    {
        if (g_str_equal (VERSION, plugin->compatibility))
        {
            plugin->module = module;
            config->plugins = g_slist_insert_sorted (config->plugins,
                                                     (gpointer) plugin,
                                                     (GCompareFunc) mailtc_plugin_compare);
        }
        else
        {
            if (!errorset)
            {
                *error = g_error_new (MAILTC_PLUGIN_ERROR,
                                      MAILTC_PLUGIN_ERROR_COMPATIBILITY,
                                      "Error: plugin %s has incompatible version",
                                      filename);
            }
            g_free (plugin);
            g_module_close (module);
            return FALSE;
        }

        plugin->directory = mailtc_file (config, filename);
        g_assert (plugin->directory);
        g_mkdir_with_parents (plugin->directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    return TRUE;
}

gboolean
mailtc_load_plugins (mtc_config* config,
                     GError**    error)
{
    GDir* dir;
    gboolean retval = TRUE;
    gchar* dirname = LIBDIR;

    if (!g_module_supported ())
    {
        *error = g_error_new (MAILTC_PLUGIN_ERROR,
                              MAILTC_PLUGIN_ERROR_SUPPORTED,
                              "Error loading plugins: %s",
                              g_module_error ());
        return FALSE;
    }

    if ((dir = g_dir_open (dirname, 0, error)))
    {
        const gchar* filename;

        while ((filename = g_dir_read_name (dir)))
        {
            if (!g_str_has_suffix (filename, G_MODULE_SUFFIX))
                continue;

            if (!mailtc_load_plugin (config, dirname, filename, error))
                retval = FALSE;
        }
        g_dir_close (dir);
    }
    else
    {
        *error = g_error_new (MAILTC_PLUGIN_ERROR,
                              MAILTC_PLUGIN_ERROR_OPEN,
                              "Error opening plugin directory %s",
                              dirname);
        return FALSE;
    }

    if (!config->plugins && retval)
    {
        *error = g_error_new (MAILTC_PLUGIN_ERROR,
                              MAILTC_PLUGIN_ERROR_EMPTY,
                              "Error: no plugins found!");
        retval = FALSE;
    }
    return retval;
}

static void
mailtc_unload_plugin (mtc_plugin* plugin,
                      GError**    error)
{
    GModule* module;

    g_assert (plugin && plugin->module);

    if (plugin->terminate)
        (*plugin->terminate) (plugin);

    module = (GModule*) plugin->module;

    if (!g_module_close (module) && error && !(*error))
    {
        *error = g_error_new (MAILTC_PLUGIN_ERROR,
                              MAILTC_PLUGIN_ERROR_CLOSE,
                              "Error unloading plugin: %s: %s",
                              g_module_name (module),
                              g_module_error ());
    }
    g_strfreev (plugin->protocols);
    g_free (plugin->ports);
    g_free (plugin->directory);
    g_free (plugin);
}

gboolean
mailtc_unload_plugins (mtc_config* config,
                       GError**    error)
{
    g_assert (config);

    g_slist_foreach (config->plugins,
                     (GFunc)mailtc_unload_plugin,
                      error);
    g_slist_free (config->plugins);
    config->plugins = NULL;

    return ((error && *error) ? FALSE : TRUE);
}

