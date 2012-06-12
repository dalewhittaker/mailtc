/* mtc-application.c
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
#include "mtc-application.h"
#include "mtc-config.h"
#include "mtc-util.h"
#include "mtc-file.h"
#include "mtc-module.h"
#include "mtc-mail.h"

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <signal.h>

/*
 * See MAILTC_MODE_UNIQUE below.
 */
typedef enum
{
    MAILTC_MODE_HELP = 0,
    MAILTC_MODE_ERROR,
    MAILTC_MODE_NORMAL = 4,
    MAILTC_MODE_DEBUG,
    MAILTC_MODE_CONFIG,
    MAILTC_MODE_KILL
} mtc_mode;

/* Flag to mask modes that require GApplication */
#define MAILTC_MODE_UNIQUE(mode) (((mode) & 0xFC) == 0x04)

#define MAILTC_APPLICATION_ID       "org." PACKAGE
#define MAILTC_APPLICATION_LOG_NAME "log"

#define MAILTC_APPLICATION_ERROR g_quark_from_string ("MAILTC_APPLICATION_ERROR")

typedef enum
{
    MAILTC_APPLICATION_ERROR_MODULE_DIRECTORY = 0,
    MAILTC_APPLICATION_ERROR_MODULE_COMPATIBILITY,
    MAILTC_APPLICATION_ERROR_MODULE_EMPTY
} MailtcApplicationError;

struct _MailtcApplicationPrivate
{
    gboolean is_running;
    guint source_id;
    GPtrArray* modules;
};

struct _MailtcApplication
{
    GApplication parent_instance;

    MailtcApplicationPrivate* priv;
};

struct _MailtcApplicationClass
{
    GApplicationClass parent_class;
};

G_DEFINE_TYPE (MailtcApplication, mailtc_application, G_TYPE_APPLICATION)

static void
mailtc_application_unload_module (mtc_plugin* plugin,
                                  GError**    error)
{
    g_assert (plugin);

    if (plugin->terminate)
        (*plugin->terminate) (plugin);

    mailtc_module_unload (MAILTC_MODULE (plugin->module), error);

    g_array_free (plugin->protocols, TRUE);
    g_free (plugin->directory);
    g_free (plugin);
}

static gboolean
mailtc_application_unload_modules (MailtcApplication* app,
                                   GError**           error)
{
    MailtcApplicationPrivate* priv;

    g_assert (MAILTC_IS_APPLICATION (app));

    priv = app->priv;

    if (priv->modules)
    {
        g_ptr_array_foreach (priv->modules,
                (GFunc) mailtc_application_unload_module,
                error);
        g_ptr_array_unref (priv->modules);
        priv->modules = NULL;
    }
    return ((error && *error) ? FALSE : TRUE);
}

static gboolean
mailtc_application_load_modules (MailtcApplication* app,
                                 GError**           error)
{
    MailtcApplicationPrivate* priv;
    GDir* dir;
    gchar* dirname = LIBDIR;
    gboolean retval = TRUE;

    g_assert (MAILTC_IS_APPLICATION (app));

    if (!mailtc_module_supported (error))
        return FALSE;

    priv = app->priv;

    if ((dir = g_dir_open (dirname, 0, error)))
    {
        MailtcModule* module;
        GPtrArray* modules;
        mtc_plugin* plugin;
        mtc_plugin* (*plugin_init) (void);
        const gchar* filename;
        gchar* fullname;

        modules = g_ptr_array_new ();

        while ((filename = g_dir_read_name (dir)))
        {
            if (!mailtc_module_filename (filename))
                continue;

            fullname = g_build_filename (dirname, filename, NULL);

            module = mailtc_module_new ();
            if (!mailtc_module_load (module, fullname, error) ||
                !mailtc_module_symbol (module, "plugin_init", (gpointer*) &plugin_init, error))
            {
                g_object_unref (module);
                retval = FALSE;
            }
            else
            {
                plugin = plugin_init ();
                if (plugin)
                {
                    if (g_str_equal (VERSION, plugin->compatibility))
                    {
                        plugin->module = module;
                        g_ptr_array_add (modules, plugin);
                    }
                    else
                    {
                        if (error && !*error)
                        {
                            *error = g_error_new (MAILTC_APPLICATION_ERROR,
                                                  MAILTC_APPLICATION_ERROR_MODULE_COMPATIBILITY,
                                                  "Error: plugin %s has incompatible version",
                                                  fullname);
                        }
                        g_free (plugin);
                        g_object_unref (module);
                        retval = FALSE;
                    }

                    plugin->directory = mailtc_file (NULL, filename);
                    g_assert (plugin->directory);
                    g_mkdir_with_parents (plugin->directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                }

            }
            g_free (fullname);
        }

        priv->modules = modules;
        g_dir_close (dir);
    }
    else
    {
        *error = g_error_new (MAILTC_APPLICATION_ERROR,
                              MAILTC_APPLICATION_ERROR_MODULE_DIRECTORY,
                              "Error opening module directory %s",
                              dirname);
        return FALSE;
    }

    if (!priv->modules && retval)
    {
        *error = g_error_new (MAILTC_APPLICATION_ERROR,
                              MAILTC_APPLICATION_ERROR_MODULE_EMPTY,
                              "Error: no modules found!");
        retval = FALSE;
    }
    return retval;
}


static void
mailtc_application_set_option_entry (GOptionEntry* entry,
                                     const gchar*  long_name,
                                     gchar         short_name,
                                     const gchar*  description,
                                     gpointer      arg_data)
{
    entry->long_name = long_name;
    entry->short_name = short_name;
    entry->arg = G_OPTION_ARG_NONE;
    entry->arg_data = arg_data;
    entry->description = description;
}

static mtc_mode
mailtc_application_parse_config (int*     argc,
                                 char***  argv,
                                 GError** error)
{
    GOptionContext* context;
    GOptionEntry *entries;
    gboolean configure;
    gboolean debug;
    gboolean kill;
    gboolean parsed;
    gboolean help;
    GString* hstr;

    configure = debug = kill = help = FALSE;
    hstr = NULL;

    entries = g_new0 (GOptionEntry, 5);
    mailtc_application_set_option_entry (&entries[0], "configure", 'c', "Configure mail details.", &configure);
    mailtc_application_set_option_entry (&entries[1], "debug", 'd', "Run in network debug mode.", &debug);
    mailtc_application_set_option_entry (&entries[2], "kill", 'k', "Kill all running " PACKAGE " processes.", &kill);
    mailtc_application_set_option_entry (&entries[3], "help", 'h', "Show help options.", &help);

    context = g_option_context_new (NULL);
    g_option_context_set_help_enabled (context, FALSE);
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_set_ignore_unknown_options (context, FALSE);
    parsed = g_option_context_parse (context, argc, argv, error);

    if (help)
    {
        guint i;

        hstr = g_string_sized_new (300);
        g_string_append_printf (hstr, "\n%s %s - Copyright (c) 2009-2012 Dale Whittaker.\n\n"
                                      "Usage:\n  %s <option>\n\nOptions:\n",
                                      PACKAGE, VERSION, g_get_prgname ());

        for (i = 0; i < 4; i++)
        {
            g_string_append_printf (hstr, "  -%c, --%-14s%s\n",
                    entries[i].short_name, entries[i].long_name, entries[i].description);
        }
        g_string_append (hstr, "\n");
    }
    g_option_context_free (context);
    g_free (entries);

    if (!parsed)
        return MAILTC_MODE_ERROR;

    if (hstr)
    {
        g_print ("%s", hstr->str);
        g_string_free (hstr, TRUE);
        return MAILTC_MODE_HELP;
    }

    if (kill)
        return MAILTC_MODE_KILL;
    if (configure)
        return MAILTC_MODE_CONFIG;
    if (debug)
        return MAILTC_MODE_DEBUG;

    return MAILTC_MODE_NORMAL;
}

static void
mailtc_application_term_handler (gint signal)
{
    mailtc_quit ();
    mailtc_message ("\n%s: %s.", PACKAGE, g_strsignal (signal));
}

static gboolean
mailtc_application_server_init (MailtcApplication* app,
                                mtc_mode           mode,
                                mtc_config*        config,
                                GError**           error)
{
    MailtcApplicationPrivate* priv;

    g_assert (MAILTC_IS_APPLICATION (app));

    mailtc_set_log_gtk (config);

    /* Initialise thread system */
    if (!g_thread_supported ())
        g_thread_init (NULL);

    if (!mailtc_application_load_modules (app, error))
        return FALSE;

    priv = app->priv;

    if (!mailtc_load_config (config, priv->modules, error))
    {
        if (g_error_matches (*error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_NOT_FOUND) ||
            g_error_matches (*error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND) ||
            g_error_matches (*error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND) ||
            g_error_matches (*error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
        {
            if (mode != MAILTC_MODE_CONFIG)
                mailtc_warning ("No accounts found.\nPlease enter a mail account");
            g_clear_error (error);
        }
        else
            return FALSE;

        mode = MAILTC_MODE_CONFIG;
    }


    switch (mode)
    {
        case MAILTC_MODE_DEBUG:
            config->debug = TRUE;
        case MAILTC_MODE_NORMAL:
            priv->source_id = mailtc_run_main_loop (config);
            break;

        case MAILTC_MODE_CONFIG:
            mailtc_config_dialog (config, priv->modules);
            break;

        default:
            /* FIXME some kind of error? */
            return FALSE;
    }

    signal (SIGABRT, mailtc_application_term_handler);
    signal (SIGTERM, mailtc_application_term_handler);
    signal (SIGINT, mailtc_application_term_handler);
    signal (SIGSEGV, mailtc_application_term_handler);
    signal (SIGHUP, mailtc_application_term_handler);
    signal (SIGQUIT, mailtc_application_term_handler);

    gtk_main ();

    return TRUE;
}

static gboolean
mailtc_application_initialise (mtc_config* config,
                               GError**    error)
{
    if (config)
    {
        gchar* filename;
        gchar* str_time;
        gchar* str_init;
        gsize bytes;

        filename = mailtc_file (config, MAILTC_APPLICATION_LOG_NAME);
        config->log = g_io_channel_new_file (filename, "w", error);
        mailtc_set_log_glib (config);
        g_free (filename);

        if (*error)
            return FALSE;

        str_time = mailtc_current_time ();
        str_init = g_strdup_printf ("\n*******************************************\n"
                                    PACKAGE " started %s\n\n"
                                    "*******************************************\n",
                                    str_time);

        g_free (str_time);
        g_io_channel_write_chars (config->log, str_init, -1, &bytes, NULL);
        g_io_channel_flush (config->log, NULL);
        g_free (str_init);
    }
    return TRUE;
}

static gboolean
mailtc_application_terminate (MailtcApplication* app,
                              mtc_config*        config,
                              GError**           error)
{
    mtc_config* newconfig = NULL;
    MailtcApplicationPrivate* priv;

    g_assert (MAILTC_IS_APPLICATION (app));

    priv = app->priv;

    if (priv->source_id > 0)
    {
        g_source_remove (priv->source_id);
        priv->source_id = 0;
    }

    if (config)
    {
        if (config->log)
        {
            gboolean status_error;

            status_error = (g_io_channel_shutdown (config->log, TRUE,
                                    *error ? NULL : error) == G_IO_STATUS_ERROR);
            if (!status_error)
            {
                g_io_channel_unref (config->log);
                config->log = NULL;
                newconfig = config;
            }
        }
    }
    mailtc_set_log_glib (newconfig);

    return newconfig ? TRUE : FALSE;
}

static gboolean
mailtc_application_cleanup (MailtcApplication* app,
                            mtc_config*        config,
                            GError**           error)
{
    gboolean success = TRUE;

    if (!mailtc_free_config (config, *error ? NULL : error))
        success = FALSE;
    if (!mailtc_application_unload_modules (app, *error ? NULL : error))
        success = FALSE;

    return success;
}

static int
mailtc_application_command_line_cb (GApplication*            app,
                                    GApplicationCommandLine* cmdline)
{
    MailtcApplication* mtcapp;
    MailtcApplicationPrivate* priv;
    mtc_mode mode;
    gchar** args;
    gchar** argv;
    gint argc;
    gint i;
    gboolean is_running;
    gboolean report_error = FALSE;
    mtc_config* config = NULL;
    GError* error = NULL;

    g_assert (g_application_get_is_registered (app));

    g_application_hold (app);

    mtcapp = MAILTC_APPLICATION (app);
    priv = mtcapp->priv;

    is_running = priv->is_running;
    priv->is_running = TRUE;

    args = g_application_command_line_get_arguments (cmdline, &argc);

    argv = g_new (gchar*, argc + 1);
    for (i = 0; i <= argc; i++)
        argv[i] = args[i];

    mode = mailtc_application_parse_config (&argc, &argv, &error);
    if (mode == MAILTC_MODE_ERROR)
    {
        report_error = TRUE;
        mailtc_gerror (&error);
    }
    else
    {
        if (mode == MAILTC_MODE_KILL)
        {
            if (is_running)
            {
                g_assert (gtk_main_level () > 0);
                mailtc_quit ();
            }
        }
        else if (MAILTC_MODE_UNIQUE (mode))
        {
            if (is_running)
            {
                g_assert (gtk_main_level () > 0);
                mailtc_warning ("An instance of %s is already running.", PACKAGE);
            }
            else
            {
                config = g_new0 (mtc_config, 1);
                if (!mailtc_application_initialise (config, &error))
                    report_error = TRUE;
                else
                {
                    gtk_init (&argc, &args);

                    if (!mailtc_application_server_init (mtcapp, mode, config, &error) && error)
                        report_error = TRUE;
                }
                if (report_error)
                    mailtc_gerror (&error);
                if (!mailtc_application_terminate (mtcapp, config, &error))
                    mailtc_gerror (&error);
                if (!mailtc_application_cleanup (mtcapp, config, &error))
                    mailtc_gerror (&error);
            }
        }
    }

    g_free (argv);
    g_strfreev (args);

    g_application_set_inactivity_timeout (app, 0);
    g_application_release (app);

    return report_error ? 1 : 0;
}

static gboolean
mailtc_application_local_cmdline (GApplication* app,
                                  gchar***      arguments,
                                  gint*         exit_status)
{
    gchar **args;
    gchar **argv;
    gint argc;
    gint i;
    mtc_mode mode;
    GError* error = NULL;

    (void) app;

    args = *arguments;
    argc = g_strv_length (args);
    argv = g_new (gchar*, argc + 1);

    for (i = 0; i <= argc; i++)
        argv[i] = args[i];

    mode = mailtc_application_parse_config (&argc, &argv, &error);

    g_free (argv);

    if (mode == MAILTC_MODE_ERROR)
    {
        mailtc_gerror (&error);
        *exit_status = 1;
        return TRUE;
    }

    *exit_status = 0;

    return (mode == MAILTC_MODE_HELP) ? TRUE : FALSE;
}

static void
mailtc_application_finalize (GObject* object)
{
    MailtcApplication* app;
    MailtcApplicationPrivate* priv;

    app = MAILTC_APPLICATION (object);
    priv = app->priv;

    priv->is_running = FALSE;
    priv->source_id = 0;

    mailtc_application_unload_modules (app, NULL);

    G_OBJECT_CLASS (mailtc_application_parent_class)->finalize (object);
}

static void
mailtc_application_class_init (MailtcApplicationClass* class)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->finalize = mailtc_application_finalize;

    G_APPLICATION_CLASS (class)->local_command_line = mailtc_application_local_cmdline;

    g_type_class_add_private (class, sizeof (MailtcApplicationPrivate));
}

static void
mailtc_application_init (MailtcApplication* app)
{
    MailtcApplicationPrivate* priv;

    priv = app->priv = G_TYPE_INSTANCE_GET_PRIVATE (app, MAILTC_TYPE_APPLICATION, MailtcApplicationPrivate);

    priv->is_running = FALSE;
    priv->source_id = 0;

    g_signal_connect (app, "command-line",
        G_CALLBACK (mailtc_application_command_line_cb), NULL);

    g_application_set_inactivity_timeout (G_APPLICATION (app), 10000);
}

MailtcApplication*
mailtc_application_new (void)
{
    g_set_application_name (PACKAGE);
    mailtc_set_log_glib (NULL);

    g_type_init ();

    g_assert (g_application_id_is_valid (MAILTC_APPLICATION_ID));

    return g_object_new (MAILTC_TYPE_APPLICATION,
                         "application-id", MAILTC_APPLICATION_ID,
                         "flags", G_APPLICATION_FLAGS_NONE);
}
