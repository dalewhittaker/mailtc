/* main.c
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

#include "mtc.h"
#include "mtc-config.h"
#include "mtc-util.h"
#include "mtc-file.h"
#include "mtc-plugin.h"
#include "mtc-mail.h"

#include <glib/gstdio.h> /* g_remove */
#include <unique/unique.h>
#include <signal.h>

#define LOG_NAME "log"

/*
 * See MAILTC_MODE_UNIQUE below.
 */
typedef enum
{
    MAILTC_MODE_VERSION = 0,
    MAILTC_MODE_ERROR,
    MAILTC_MODE_NORMAL = 4,
    MAILTC_MODE_DEBUG,
    MAILTC_MODE_CONFIG,
    MAILTC_MODE_KILL

} mtc_mode;

/* Flag to mask modes that require libunique */
#define MAILTC_MODE_UNIQUE(mode) (((mode) & 0xFC) == 0x04)

static void
mailtc_set_option_entry (GOptionEntry* entry,
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
mailtc_parse_config (int*     argc,
                     char***  argv,
                     GError** error)
{
    GOptionContext* context;
    GOptionEntry *entries;
    gboolean configure;
    gboolean debug;
    gboolean kill;
    gboolean version;

    configure = debug = kill = version = FALSE;

    entries = g_new0 (GOptionEntry, 5);
    mailtc_set_option_entry (&entries[0], "configure", 'c', "Configure mail details.", &configure);
    mailtc_set_option_entry (&entries[1], "debug", 'd', "Run in network debug mode.", &debug);
    mailtc_set_option_entry (&entries[2], "kill", 'k', "Kill all running " PACKAGE " processes.", &kill);
    mailtc_set_option_entry (&entries[3], "version", 'v', "Display program version.", &version);

    context = g_option_context_new ("");
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_set_ignore_unknown_options (context, FALSE);
    if (!g_option_context_parse (context, argc, argv, error))
    {
        g_free (entries);
        return MAILTC_MODE_ERROR;
    }
    g_free (entries);

    if (version)
    {
        g_print ("\n%s %s - Copyright (c) 2009 Dale Whittaker.\n\n", PACKAGE, VERSION);
        return MAILTC_MODE_VERSION;
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
mailtc_term_handler (gint signal)
{
    mailtc_quit ();
    mailtc_message ("\n%s: %s.", PACKAGE, g_strsignal (signal));
}

static UniqueResponse
mailtc_message_received_cb (UniqueApp*         app,
                            UniqueCommand      command,
                            UniqueMessageData* message_data,
                            guint              time_)
{
    (void) app;
    (void) time_;
    (void) message_data;

    switch (command)
    {
        case MAILTC_MODE_KILL:
            mailtc_quit ();
            break;
        default:
            ;
    }
    return UNIQUE_RESPONSE_OK;
}

static gboolean
mailtc_server_init (UniqueApp*  app,
                    mtc_mode    mode,
                    mtc_config* config,
                    GError**    error)
{
    mailtc_set_log_gtk (config);

    if (unique_app_is_running (app))
    {
        mailtc_warning ("An instance of %s is already running.", PACKAGE);
        return FALSE;
    }

    g_signal_connect (app, "message-received",
            G_CALLBACK (mailtc_message_received_cb), NULL);

    if (!mailtc_load_plugins (config, error) ||
        !mailtc_load_config (config, error))
    {
        if (g_error_matches (*error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_NOT_FOUND) ||
            g_error_matches (*error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND) ||
            g_error_matches (*error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND))
        {
            if (mode != MAILTC_MODE_CONFIG)
                mailtc_warning ("No accounts found.\nPlease enter a mail account");
        }
        else
            return FALSE;

        mode = MAILTC_MODE_CONFIG;
    }

    if (mode == MAILTC_MODE_NORMAL ||
        mode == MAILTC_MODE_DEBUG)
    {
        if (mode == MAILTC_MODE_DEBUG)
            config->debug = TRUE;
        mailtc_run_main_loop (config);
    }
    else if (mode == MAILTC_MODE_CONFIG)
        mailtc_config_dialog (config);

    signal (SIGABRT, mailtc_term_handler);
    signal (SIGTERM, mailtc_term_handler);
    signal (SIGINT, mailtc_term_handler);
    signal (SIGSEGV, mailtc_term_handler);
    signal (SIGHUP, mailtc_term_handler);
    signal (SIGQUIT, mailtc_term_handler);

    gtk_main ();
    mailtc_message ("bye");

    return TRUE;
}

static gboolean
mailtc_initialise (mtc_config* config,
                   GError**    error)
{
    gchar* filename;
    gchar* str_time;
    gchar* str_init;
    gsize bytes;

#if HAVE_OPENSSL
    SSL_load_error_strings ();
    SSL_library_init ();
#endif

    if (config)
    {
        filename = mailtc_file (config, LOG_NAME);
        if (g_file_test (filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
            g_remove (filename);

        config->log = g_io_channel_new_file (filename, "a+", error);
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
mailtc_terminate (mtc_config* config,
                  GError**    error)
{
    mtc_config* newconfig;
    gboolean status_error;

    newconfig = NULL;

    if (config->source_id > 0)
    {
        g_source_remove (config->source_id);
        config->source_id = 0;
    }
    if (config && config->log)
    {
        status_error = (g_io_channel_shutdown (config->log, TRUE, error) == G_IO_STATUS_ERROR);
        if (!status_error)
        {
            g_io_channel_unref (config->log);
            config->log = NULL;
            newconfig = config;
        }
    }
    mailtc_set_log_glib (newconfig);

    return newconfig ? TRUE : FALSE;
}

static gboolean
mailtc_cleanup (mtc_config* config,
                GError**    error)
{
    gboolean success;

    success = mailtc_free_config (config, error);

#if HAVE_OPENSSL
    ERR_free_strings();
    EVP_cleanup();
#endif
    return success;
}

int
main (int argc,
      char **argv)
{
    UniqueApp* app;
    mtc_mode mode;
    mtc_config* config;
    GError* error;
    gboolean report_error;

    config = NULL;
    error = NULL;
    report_error = FALSE;

    g_set_application_name (PACKAGE);
    mailtc_set_log_glib (NULL);

    mode = mailtc_parse_config (&argc, &argv, &error);

    if (MAILTC_MODE_UNIQUE (mode))
    {
        gtk_init (&argc, &argv);
        app = unique_app_new_with_commands (PACKAGE, NULL,
                                "kill", MAILTC_MODE_KILL, NULL);

        if (mode == MAILTC_MODE_KILL)
        {
            if (unique_app_is_running (app))
            {
                if (unique_app_send_message (app, MAILTC_MODE_KILL, NULL) != UNIQUE_RESPONSE_OK)
                    mailtc_error ("Error sending kill message.");
            }
        }
        else
        {
            config = g_new0 (mtc_config, 1);
            if (!mailtc_initialise (config, &error))
                report_error = TRUE;
            else
            {
                if (!mailtc_server_init (app, mode, config, &error) && error)
                    report_error = TRUE;
            }
        }
        g_object_unref (app);
    }
    else if (mode == MAILTC_MODE_ERROR)
        report_error = TRUE;

    if (report_error ||
        !mailtc_terminate (config, &error) ||
        !mailtc_cleanup (config, &error))
        mailtc_gerror (&error);

    mailtc_message ("theend");
    return report_error ? 1 : 0;
}

