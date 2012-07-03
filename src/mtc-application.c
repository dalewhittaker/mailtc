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
#include "mtc-configdialog.h"
#include "mtc-util.h"
#include "mtc-module.h"

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <signal.h>

#define MAILTC_APPLICATION_PROPERTY_DEBUG       "debug"
#define MAILTC_APPLICATION_PROPERTY_SETTINGS    "settings"
#define MAILTC_APPLICATION_PROPERTY_STATUS_ICON "statusicon"

#define MAILTC_APPLICATION_ID          "org." PACKAGE
#define MAILTC_APPLICATION_LOG_NAME    "log"
#define MAILTC_APPLICATION_CONFIG_NAME "config"

#define MAILTC_APPLICATION_ERROR        g_quark_from_string ("MAILTC_APPLICATION_ERROR")

/* Flag to mask modes that require GApplication */
#define MAILTC_MODE_UNIQUE(mode) (((mode) & 0xFC) == 0x04)

typedef enum
{
    MAILTC_MODE_HELP = 0,
    MAILTC_MODE_ERROR,
    MAILTC_MODE_NORMAL = 4,
    MAILTC_MODE_DEBUG,
    MAILTC_MODE_CONFIG,
    MAILTC_MODE_KILL
} MailtcMode;

typedef enum
{
    MAILTC_APPLICATION_ERROR_INVALID_OPTION = 0,
    MAILTC_APPLICATION_ERROR_MODULE_DIRECTORY,
    MAILTC_APPLICATION_ERROR_MODULE_COMPATIBILITY,
    MAILTC_APPLICATION_ERROR_MODULE_EMPTY
} MailtcApplicationError;

enum
{
    PROP_0,
    PROP_DEBUG,
    PROP_SETTINGS,
    PROP_STATUS_ICON
};

enum
{
    SIGNAL_CONFIGURE,
    SIGNAL_RUN,
    SIGNAL_TERMINATE,
    LAST_SIGNAL
};

struct _MailtcApplicationPrivate
{
    GIOChannel* log;
    GPtrArray* modules;
    gboolean is_running;
    gchar* directory;
};

struct _MailtcApplication
{
    GApplication parent_instance;

    MailtcApplicationPrivate* priv;
    MailtcSettings* settings;
    MailtcStatusIcon* statusicon;
    gboolean debug;
};

struct _MailtcApplicationClass
{
    GApplicationClass parent_class;
};

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE (MailtcApplication, mailtc_application, G_TYPE_APPLICATION)

static void
mailtc_application_glib_handler (const gchar*   log_domain,
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
mailtc_application_set_log_glib (MailtcApplication* app)
{
    GIOChannel* log = NULL;

    if (app)
    {
        g_assert (MAILTC_IS_APPLICATION (app));

        log = app->priv->log;
    }

    g_log_set_fatal_mask (NULL, G_LOG_LEVEL_ERROR);
    g_log_set_handler (NULL,
                       G_LOG_LEVEL_MASK |
                       G_LOG_FLAG_FATAL |
                       G_LOG_FLAG_RECURSION,
                       (GLogFunc) mailtc_application_glib_handler,
                       log);
}

static void
mailtc_application_gtk_handler (const gchar*              log_domain,
                                GLogLevelFlags            log_level,
                                const gchar*              message,
                                GIOChannel*               log)
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
        GList* l;
        GList* w;
        GtkWidget* toplevel;
        GtkWidget* dialog = NULL;

        w = gtk_window_list_toplevels ();

        for (l = w; l; l = l->next)
        {
            if (MAILTC_IS_CONFIG_DIALOG (l->data))
                dialog = GTK_WIDGET (l->data);

            g_list_free (w);
        }

        toplevel = dialog ? gtk_widget_get_toplevel (dialog) : NULL;

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
    mailtc_log (log, s);
    g_free (s);
}

void
mailtc_application_set_log_gtk (MailtcApplication* app)
{
    GIOChannel* log = NULL;

    if (app)
    {
        g_assert (MAILTC_IS_APPLICATION (app));

        log = app->priv->log;
    }
    g_log_set_fatal_mask (NULL, G_LOG_LEVEL_ERROR);
    g_log_set_handler (NULL,
                       G_LOG_LEVEL_MASK |
                       G_LOG_FLAG_FATAL |
                       G_LOG_FLAG_RECURSION,
                       (GLogFunc) mailtc_application_gtk_handler,
                       log);
}

static gchar*
mailtc_application_file (MailtcApplication* app,
                         const gchar*       filename)
{
    MailtcApplicationPrivate* priv;
    gchar* directory;
    gchar* absfilename;

    g_assert (MAILTC_IS_APPLICATION (app));

    priv = app->priv;

    if (!priv->directory)
    {
        directory = mailtc_directory ();
        if (!directory)
        {
            mailtc_error ("Failed to create " PACKAGE  " directory");
            return NULL;
        }
        priv->directory = directory;
    }
    else
        directory = priv->directory;

    absfilename = g_build_filename (directory, filename, NULL);

    return absfilename;
}

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

                    plugin->directory = mailtc_application_file (app, filename);
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

static MailtcMode
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
                                MailtcMode         mode,
                                GError**           error)
{
    MailtcApplicationPrivate* priv;
    MailtcSettings* settings;
    gchar* filename;

    g_assert (MAILTC_IS_APPLICATION (app));

    mailtc_application_set_log_gtk (app);

    /* Initialise thread system */
    if (!g_thread_supported ())
        g_thread_init (NULL);

    if (!mailtc_application_load_modules (app, error))
        return FALSE;

    priv = app->priv;

    filename = mailtc_application_file (app, MAILTC_APPLICATION_CONFIG_NAME);
    settings = mailtc_settings_new (filename, priv->modules, error);
    g_free (filename);

    if (!settings)
        return FALSE;

    if (mode != MAILTC_MODE_CONFIG)
    {
        GPtrArray* accounts;

        accounts = mailtc_settings_get_accounts (settings);
        if (accounts->len == 0)
        {
            mailtc_warning ("Incomplete settings found.\nPlease enter configuration settings");
            mode = MAILTC_MODE_CONFIG;
            g_object_unref (accounts);
        }
    }

    mailtc_application_set_settings (app, settings);

    switch (mode)
    {
        case MAILTC_MODE_DEBUG:
            mailtc_application_set_debug (app, TRUE);
        case MAILTC_MODE_NORMAL:
            g_signal_emit (app, signals[SIGNAL_RUN], 0);
            break;

        case MAILTC_MODE_CONFIG:
            g_signal_emit (app, signals[SIGNAL_CONFIGURE], 0);
            break;

        default:
            if (error)
            {
                *error = g_error_new (MAILTC_APPLICATION_ERROR,
                                      MAILTC_APPLICATION_ERROR_INVALID_OPTION,
                                      "Error: invalid option");
            }
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
mailtc_application_initialise (MailtcApplication* app,
                               GError**           error)
{
    MailtcApplicationPrivate* priv;
    gchar* filename;
    gchar* str_time;
    gchar* str_init;
    gsize bytes;

    g_assert (MAILTC_IS_APPLICATION (app));

    priv = app->priv;

    filename = mailtc_application_file (app, MAILTC_APPLICATION_LOG_NAME);
    priv->log = g_io_channel_new_file (filename, "w", error);
    mailtc_application_set_log_glib (app);
    g_free (filename);

    if (*error)
        return FALSE;

    str_time = mailtc_current_time ();
    str_init = g_strdup_printf ("\n*******************************************\n"
                                PACKAGE " started %s\n\n"
                                "*******************************************\n",
                                str_time);

    g_free (str_time);
    g_io_channel_write_chars (priv->log, str_init, -1, &bytes, NULL);
    g_io_channel_flush (priv->log, NULL);
    g_free (str_init);

    return TRUE;
}

static gboolean
mailtc_application_terminate (MailtcApplication* app,
                              GError**           error)
{
    MailtcApplication* newapp = NULL;
    MailtcApplicationPrivate* priv;
    gboolean success = TRUE;

    g_assert (MAILTC_IS_APPLICATION (app));

    priv = app->priv;

    g_signal_emit (app, signals[SIGNAL_TERMINATE], 0);

    if (app->statusicon)
    {
        g_object_unref (app->statusicon);
        app->statusicon = NULL;
    }
    if (priv->log)
    {
        gboolean status_error;

        status_error = (g_io_channel_shutdown (priv->log, TRUE,
                                *error ? NULL : error) == G_IO_STATUS_ERROR);
        if (!status_error)
        {
            g_io_channel_unref (priv->log);
            priv->log = NULL;
            newapp = app;
        }
    }
    mailtc_application_set_log_glib (newapp);

    if (!newapp)
        success = FALSE;

    if (app->settings)
    {
        g_object_unref (app->settings);
        app->settings = NULL;
    }

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
    MailtcMode mode;
    gchar** args;
    gchar** argv;
    gint argc;
    gint i;
    gboolean is_running;
    gboolean report_error = FALSE;
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
                if (!mailtc_application_initialise (mtcapp, &error))
                    report_error = TRUE;
                else
                {
                    gtk_init (&argc, &args);

                    if (!mailtc_application_server_init (mtcapp, mode, &error) && error)
                        report_error = TRUE;
                }
                if (report_error)
                    mailtc_gerror (&error);

                if (!mailtc_application_terminate (mtcapp, &error))
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
    MailtcMode mode;
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

void
mailtc_application_set_debug (MailtcApplication* app,
                              gboolean           debug)
{
    g_assert (MAILTC_IS_APPLICATION (app));

    if (debug != app->debug)
    {
        app->debug = debug;
        g_object_notify (G_OBJECT (app), MAILTC_APPLICATION_PROPERTY_DEBUG);
    }
}

gboolean
mailtc_application_get_debug (MailtcApplication* app)
{
    g_assert (MAILTC_IS_APPLICATION (app));

    return app->debug;
}

void
mailtc_application_set_settings (MailtcApplication* app,
                                 MailtcSettings*    settings)
{
    g_assert (MAILTC_IS_APPLICATION (app));

    if (settings != app->settings)
    {
        if (app->settings)
            g_object_unref (app->settings);

        app->settings = settings ? g_object_ref (settings) : NULL;
        g_object_notify (G_OBJECT (app), MAILTC_APPLICATION_PROPERTY_SETTINGS);
    }
}

MailtcSettings*
mailtc_application_get_settings (MailtcApplication* app)
{
    g_assert (MAILTC_IS_APPLICATION (app));

    return app->settings ? g_object_ref (app->settings) : NULL;
}

void
mailtc_application_set_status_icon (MailtcApplication* app,
                                    MailtcStatusIcon*  statusicon)
{
    g_assert (MAILTC_IS_APPLICATION (app));

    if (statusicon != app->statusicon)
    {
        if (app->statusicon)
            g_object_unref (app->statusicon);

        app->statusicon = statusicon ? g_object_ref (statusicon) : NULL;
        g_object_notify (G_OBJECT (app), MAILTC_APPLICATION_PROPERTY_STATUS_ICON);
    }
}

MailtcStatusIcon*
mailtc_application_get_status_icon (MailtcApplication* app)
{
    g_assert (MAILTC_IS_APPLICATION (app));

    return app->statusicon ? g_object_ref (app->statusicon) : NULL;
}

static void
mailtc_application_set_property (GObject*      object,
                                 guint         prop_id,
                                 const GValue* value,
                                 GParamSpec*   pspec)
{
    MailtcApplication* app = MAILTC_APPLICATION (object);

    switch (prop_id)
    {
        case PROP_DEBUG:
            mailtc_application_set_debug (app, g_value_get_boolean (value));
            break;
        case PROP_SETTINGS:
            mailtc_application_set_settings (app, g_value_get_object (value));
            break;
        case PROP_STATUS_ICON:
            mailtc_application_set_status_icon (app, g_value_get_object (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_application_get_property (GObject*    object,
                                 guint       prop_id,
                                 GValue*     value,
                                 GParamSpec* pspec)
{
    MailtcApplication* app = MAILTC_APPLICATION (object);

    switch (prop_id)
    {
        case PROP_DEBUG:
            g_value_set_boolean (value, mailtc_application_get_debug (app));
            break;
        case PROP_SETTINGS:
            g_value_set_object (value, mailtc_application_get_settings (app));
            break;
        case PROP_STATUS_ICON:
            g_value_set_object (value, mailtc_application_get_status_icon (app));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
mailtc_application_finalize (GObject* object)
{
    MailtcApplication* app;
    MailtcApplicationPrivate* priv;

    app = MAILTC_APPLICATION (object);
    priv = app->priv;

    app->debug = FALSE;
    priv->is_running = FALSE;
    g_free (priv->directory);

    if (app->statusicon)
        g_object_unref (app->statusicon);
    if (app->settings)
        g_object_unref (app->settings);
    if (priv->log)
        g_io_channel_unref (priv->log);

    mailtc_application_unload_modules (app, NULL);

    G_OBJECT_CLASS (mailtc_application_parent_class)->finalize (object);
}

static void
mailtc_application_class_init (MailtcApplicationClass* class)
{
    GObjectClass* gobject_class;
    GParamFlags flags;

    gobject_class = G_OBJECT_CLASS (class);
    gobject_class->set_property = mailtc_application_set_property;
    gobject_class->get_property = mailtc_application_get_property;
    gobject_class->finalize = mailtc_application_finalize;

    G_APPLICATION_CLASS (class)->local_command_line = mailtc_application_local_cmdline;

    flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT;

    g_object_class_install_property (gobject_class,
                                     PROP_DEBUG,
                                     g_param_spec_boolean (
                                     MAILTC_APPLICATION_PROPERTY_DEBUG,
                                     "Debug",
                                     "Whether to enable debugging messages",
                                     FALSE,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_SETTINGS,
                                     g_param_spec_object (
                                     MAILTC_APPLICATION_PROPERTY_SETTINGS,
                                     "Settings",
                                     "The settings",
                                     MAILTC_TYPE_SETTINGS,
                                     flags));

    g_object_class_install_property (gobject_class,
                                     PROP_STATUS_ICON,
                                     g_param_spec_object (
                                     MAILTC_APPLICATION_PROPERTY_STATUS_ICON,
                                     "Statusicon",
                                     "The status icon",
                                     MAILTC_TYPE_STATUS_ICON,
                                     flags));

    signals[SIGNAL_CONFIGURE] = g_signal_new ("configure",
                                              G_TYPE_FROM_CLASS (gobject_class),
                                              G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                              0,
                                              NULL,
                                              NULL,
                                              g_cclosure_marshal_VOID__VOID,
                                              G_TYPE_NONE,
                                              0);

    signals[SIGNAL_RUN] = g_signal_new ("run",
                                        G_TYPE_FROM_CLASS (gobject_class),
                                        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                        0,
                                        NULL,
                                        NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE,
                                        0);

    signals[SIGNAL_TERMINATE] = g_signal_new ("terminate",
                                              G_TYPE_FROM_CLASS (gobject_class),
                                              G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                                              0,
                                              NULL,
                                              NULL,
                                              g_cclosure_marshal_VOID__VOID,
                                              G_TYPE_NONE,
                                              0);

    g_type_class_add_private (class, sizeof (MailtcApplicationPrivate));
}

static void
mailtc_application_init (MailtcApplication* app)
{
    MailtcApplicationPrivate* priv;

    priv = app->priv = G_TYPE_INSTANCE_GET_PRIVATE (app, MAILTC_TYPE_APPLICATION, MailtcApplicationPrivate);

    app->debug = FALSE;
    app->settings = NULL;
    app->statusicon = NULL;

    priv->is_running = FALSE;
    priv->modules = NULL;
    priv->directory = NULL;
    priv->log = NULL;

    g_signal_connect (app, "command-line",
        G_CALLBACK (mailtc_application_command_line_cb), NULL);

    g_application_set_inactivity_timeout (G_APPLICATION (app), 10000);
}

MailtcApplication*
mailtc_application_new (void)
{
    g_set_application_name (PACKAGE);
    mailtc_application_set_log_glib (NULL);

    g_type_init ();

    g_assert (g_application_id_is_valid (MAILTC_APPLICATION_ID));

    return g_object_new (MAILTC_TYPE_APPLICATION,
                         "application-id", MAILTC_APPLICATION_ID,
                         "flags", G_APPLICATION_FLAGS_NONE);
}
